#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "stb/stb_image_write.h"

const float c_gamma = 2.2f;

struct SImageInfo
{
    std::vector<float>  m_pixels;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;

    inline float* GetPixel (int x, int y)
    {
        return &m_pixels[y * m_width * m_channels + x * m_channels];
    }

    inline const float* GetPixel(int x, int y) const
    {
        return &m_pixels[y * m_width * m_channels + x * m_channels];
    }

    bool Load(const char *fileName, int desiredChannels = 3)
    {
        // load image
        m_channels = desiredChannels;
        int channels = 0;
        stbi_uc* pixels = stbi_load(fileName, &m_width, &m_height, &channels, desiredChannels);
        if (pixels == nullptr)
        {
            printf("Could not load file %s\n", fileName);
            return false;
        }

        // convert to float and convert from sRGB to linear
        m_pixels.resize(m_width*m_height*m_channels);
        stbi_uc* srcPixel = pixels;
        for (float& pixel : m_pixels)
        {
            pixel = float(*srcPixel) / 255.0f;
            pixel = std::powf(pixel, c_gamma);
            ++srcPixel;
        }

        // free pixels and return success
        stbi_image_free(pixels);
        return true;
    }
};

struct SRect
{
    int x1, y1, x2, y2;
};

bool WriteImage (const char *fileName, int width, int height, int numChannels, std::vector<float>& pixels)
{
    // convert from linear to sRGB, clamp, and convert to uint8.
    std::vector<stbi_uc> outPixels;
    outPixels.resize(width*height * numChannels);
    float* srcPixel = &pixels[0];
    for (stbi_uc& pixel : outPixels)
    {
        float value = std::powf(*srcPixel, 1.0f / c_gamma);
        if (value < 0.0f)
            value = 0.0f;
        else if (value > 1.0f)
            value = 1.0f;
        pixel = stbi_uc(value*255.0f);
        ++srcPixel;
    }

    return stbi_write_png(fileName, width, height, numChannels, &outPixels[0], numChannels * width) != 0;
}

void NaivePaste (const SImageInfo &source, const SImageInfo& mask, const SImageInfo &dest, int pasteX, int pasteY, const char* fileName)
{
    // copy the destination image to an out image buffer
    std::vector<float> outPixels;
    outPixels = dest.m_pixels;

    // calculate details of paste, handling negative paste locations and images larger than the destination etc.
    SRect sourceRect = { 0, 0, source.m_width, source.m_height };
    SRect destRect = { pasteX, pasteY, pasteX + source.m_width, pasteY + source.m_height };

    if (destRect.x1 < 0)
    {
        sourceRect.x1 -= destRect.x1;
        destRect.x1 = 0;
    }

    if (destRect.y1 < 0)
    {
        sourceRect.y1 -= destRect.y1;
        destRect.y1 = 0;
    }

    if (destRect.x2 >= dest.m_width)
    {
        int difference = (destRect.x2 - dest.m_width) + 1;
        sourceRect.x2 -= difference;
        destRect.x2 = dest.m_width - 1;
    }

    if (destRect.y2 >= dest.m_height)
    {
        int difference = (destRect.y2 - dest.m_height) + 1;
        sourceRect.y2 -= difference;
        destRect.y2 = dest.m_height - 1;
    }

    // naively paste the image
    int copyWidth = sourceRect.x2 - sourceRect.x1;
    int copyHeight = sourceRect.y2 - sourceRect.y1;
    if (copyWidth > 0 && copyHeight > 0)
    {
        for (int y = 0; y < copyHeight; ++y)
        {
            const float* sourcePixel = source.GetPixel(sourceRect.x1, sourceRect.y1 + y);
            const float* maskPixel = mask.GetPixel(sourceRect.x1, sourceRect.y1 + y);
            float* destPixel = &outPixels[((destRect.y1 + y)*dest.m_width + destRect.x1) * 3];

            for (int x = 0; x < copyWidth; ++x)
            {
                if (*maskPixel > 0.0f)
                {
                    memcpy(destPixel, sourcePixel, sizeof(float) * 3);
                }

                sourcePixel += 3;
                maskPixel += 1;
                destPixel += 3;
            }
            //memcpy(destPixels, sourcePixels, copyWidth * 3 * sizeof(sourcePixels[0]));
        }
    }

    // write the file
    if (!WriteImage(fileName, dest.m_width, dest.m_height, dest.m_channels, outPixels))
        printf(__FUNCTION__ "() error: Could not write %s\n", fileName);
}

void NaiveGradientPaste (const SImageInfo &source, std::vector<float>& sourceGradient, const SImageInfo &dest, int pasteX, int pasteY, const char* fileName)
{
    // copy the destination image to an out image buffer
    std::vector<float> outPixels;
    outPixels = dest.m_pixels;

    // calculate details of paste, handling negative paste locations and images larger than the destination etc.
    SRect sourceRect = { 0, 0, source.m_width, source.m_height };
    SRect destRect = { pasteX, pasteY, pasteX + source.m_width, pasteY + source.m_height };

    if (destRect.x1 < 0)
    {
        sourceRect.x1 -= destRect.x1;
        destRect.x1 = 0;
    }

    if (destRect.y1 < 0)
    {
        sourceRect.y1 -= destRect.y1;
        destRect.y1 = 0;
    }

    if (destRect.x2 >= dest.m_width)
    {
        int difference = (destRect.x2 - dest.m_width) + 1;
        sourceRect.x2 -= difference;
        destRect.x2 = dest.m_width - 1;
    }

    if (destRect.y2 >= dest.m_height)
    {
        int difference = (destRect.y2 - dest.m_height) + 1;
        sourceRect.y2 -= difference;
        destRect.y2 = dest.m_height - 1;
    }

    // naively paste the image gradient
    int copyWidth = sourceRect.x2 - sourceRect.x1;
    int copyHeight = sourceRect.y2 - sourceRect.y1;
    if (copyWidth > 1 && copyHeight > 1)
    {
        for (int y = 1; y < copyHeight; ++y)
        {
            const float* sourcePixel = &source.m_pixels[((sourceRect.y1 + y)*source.m_width + sourceRect.x1) * 3];
            const float* sourceGradientPixel = &sourceGradient[((sourceRect.y1 + y)*source.m_width + sourceRect.x1) * 6];
            float* destPixel = &outPixels[((destRect.y1 + y)*dest.m_width + destRect.x1) * 3];
            for (int x = 1; x < copyWidth; ++x)
            {
                destPixel[0] = destPixel[-3] + sourceGradientPixel[0];
                destPixel[1] = destPixel[-2] + sourceGradientPixel[1];
                destPixel[2] = destPixel[-1] + sourceGradientPixel[2];

                // move to the next pixels
                sourcePixel += 3;
                sourceGradientPixel += 6;
                destPixel += 3;
            }
        }
    }

    // write the file
    if (!WriteImage(fileName, dest.m_width, dest.m_height, dest.m_channels, outPixels))
        printf(__FUNCTION__ "() error: Could not write %s\n", fileName);
}

void MakeImageGradient(const SImageInfo& source, const SImageInfo& mask, std::vector<float>& sourceGradient)
{
    // allocate space for the gradients
    sourceGradient.resize(source.m_width*source.m_height * 6);
    std::fill(sourceGradient.begin(), sourceGradient.end(), 0.0f);

    // make the gradients!
    const float* sourcePixel = &source.m_pixels[0];
    const float* sourcePixelNextRow = sourcePixel + source.m_width * source.m_channels;
    const float* sourceMask = &mask.m_pixels[0];

    float* destPixel = &sourceGradient[0];
    for (int y = 0; y < source.m_height-1; ++y)
    {
        for (int x = 0; x < source.m_width-1; ++x)
        {
            if (*sourceMask > 0.0f)
            {
                // calculate RGB dfdx
                destPixel[0] = sourcePixel[3] - sourcePixel[0];
                destPixel[1] = sourcePixel[4] - sourcePixel[1];
                destPixel[2] = sourcePixel[5] - sourcePixel[2];

                // calculate RGB dfdy
                destPixel[3] = sourcePixelNextRow[0] - sourcePixel[0];
                destPixel[4] = sourcePixelNextRow[1] - sourcePixel[1];
                destPixel[5] = sourcePixelNextRow[2] - sourcePixel[2];
            }
            else
            {
                memset(destPixel, 0, sizeof(float) * 6);
            }

            // move to the next pixels
            sourcePixel += 3;
            sourcePixelNextRow += 3;
            sourceMask += 1;
            destPixel += 6;
        }
    }
}

void SaveImageGradient(const SImageInfo& source, const SImageInfo& mask, const std::vector<float>& sourceGradient, const char* fileName)
{
    // Save the gradient as a side by side double wide image.
    // The left side is the x axis partial derivatives, the right side is the y axis.
    std::vector<float> outPixels;
    outPixels.resize((source.m_width * 3 * source.m_height) * 3);

    const float* sourcePixel0 = &source.m_pixels[0];
    const float* sourcePixel1 = &sourceGradient[0];
    const float* sourceMask = &mask.m_pixels[0];

    for (int y = 0; y < source.m_height; ++y)
    {
        float* destPixel0 = &outPixels[y*source.m_width * 9];
        float* destPixel1 = &outPixels[y*source.m_width * 9 + source.m_width * 3];
        float* destPixel2 = &outPixels[y*source.m_width * 9 + source.m_width * 6];
        for (int x = 0; x < source.m_width; ++x)
        {
            if (*sourceMask > 0.0f)
            {
                destPixel0[0] = sourcePixel0[0];
                destPixel0[1] = sourcePixel0[1];
                destPixel0[2] = sourcePixel0[2];

                destPixel1[0] = (sourcePixel1[0] * 0.5f) + 0.5f;
                destPixel1[1] = (sourcePixel1[1] * 0.5f) + 0.5f;
                destPixel1[2] = (sourcePixel1[2] * 0.5f) + 0.5f;

                destPixel2[0] = (sourcePixel1[3] * 0.5f) + 0.5f;
                destPixel2[1] = (sourcePixel1[4] * 0.5f) + 0.5f;
                destPixel2[2] = (sourcePixel1[5] * 0.5f) + 0.5f;
            }
            else
            {
                memset(destPixel0, 0, sizeof(float) * 3);
                memset(destPixel1, 0, sizeof(float) * 3);
                memset(destPixel2, 0, sizeof(float) * 3);
            }

            // move to the next pixels
            destPixel0 += 3;
            destPixel1 += 3;
            destPixel2 += 3;
            sourcePixel0 += 3;
            sourcePixel1 += 6;
            sourceMask += 1;
        }
    }

    // save the file
    if (!WriteImage(fileName, source.m_width*3, source.m_height, source.m_channels, outPixels))
        printf(__FUNCTION__ "() error: Could not write %s\n", fileName);
}

void Trim(SImageInfo& source, SImageInfo& mask, int& pasteX, int& pasteY, size_t& numMaskPixels, std::unordered_map<size_t, size_t>& pixelIndexToMatrixColumn)
{
    // find the minimum bounding box based on the mask
    SRect bb;
    {
        bb.x1 = mask.m_width;
        bb.y1 = mask.m_height;
        bb.x2 = 0;
        bb.y2 = 0;
        float *pixel = &mask.m_pixels[0];
        for (int y = 0; y < mask.m_height; ++y)
        {
            for (int x = 0; x < mask.m_width; ++x)
            {
                if (*pixel > 0.0f)
                {
                    bb.x1 = std::min(x, bb.x1);
                    bb.y1 = std::min(y, bb.y1);
                    bb.x2 = std::max(x, bb.x2);
                    bb.y2 = std::max(y, bb.y2);
                }
                pixel++;
            }
        }
    }

    // make a trimmed mask
    {
        SImageInfo newMask;
        newMask.m_channels = mask.m_channels;
        newMask.m_width = bb.x2 - bb.x1;
        newMask.m_height = bb.y2 - bb.y1;
        newMask.m_pixels.resize(newMask.m_width*newMask.m_height*newMask.m_channels);

        for (int y = 0; y < newMask.m_height; ++y)
        {
            float* sourcePixel = mask.GetPixel(bb.x1, bb.y1 + y);
            float* destPixel = newMask.GetPixel(0, y);
            memcpy(destPixel, sourcePixel, sizeof(float)*newMask.m_width*newMask.m_channels);
        }

        mask = newMask;
    }

    // make a trimmed source image
    {
        SImageInfo newSource;
        newSource.m_channels = source.m_channels;
        newSource.m_width = bb.x2 - bb.x1;
        newSource.m_height = bb.y2 - bb.y1;
        newSource.m_pixels.resize(newSource.m_width*newSource.m_height*newSource.m_channels);

        for (int y = 0; y < newSource.m_height; ++y)
        {
            float* sourcePixel = source.GetPixel(bb.x1, bb.y1 + y);
            float* destPixel = newSource.GetPixel(0, y);
            memcpy(destPixel, sourcePixel, sizeof(float)*newSource.m_width*newSource.m_channels);
        }

        source = newSource;
    }

    // make the pixelIndexToMatrixColumn map
    numMaskPixels = 0;
    {
        float *pixel = &mask.m_pixels[0];
        size_t pixelIndex = 0;
        for (int y = 0; y < mask.m_height; ++y)
        {
            for (int x = 0; x < mask.m_width; ++x)
            {
                if (*pixel > 0.0f)
                {
                    pixelIndexToMatrixColumn.insert(std::make_pair(pixelIndex, numMaskPixels));
                    numMaskPixels++;
                }
                else
                {
                    pixelIndexToMatrixColumn.insert(std::make_pair(pixelIndex, size_t(-1)));
                }
                pixel++;
                pixelIndex++;
            }
        }
    }

    // adjust the paste location
    pasteX += bb.x1;
    pasteY += bb.y1;
}

int main(int argc, char** argv)
{
    SImageInfo source, mask, dest, output;
    int pasteX, pasteY;

    // get parameters and load images
    {
        if (argc < 6)
        {
            printf("usage: <source> <mask> <dest> <x> <y>\n");
            return 1;
        }

        if (!source.Load(argv[1]) || !mask.Load(argv[2], 1) || !dest.Load(argv[3]))
        {
            return 2;
        }

        if (!sscanf(argv[4], "%i", &pasteX) || !sscanf(argv[5], "%i", &pasteY))
        {
            printf("could not read width or height\n");
            return 3;
        }

        if (source.m_width != mask.m_width || source.m_height != mask.m_height)
        {
            printf("Source and mask must be same dimensions\n");
            return 4;
        }
    }

    // Trim the source and mask to a bounding rectangle
    std::unordered_map<size_t, size_t> pixelIndexToMatrixColumn;
    size_t numMaskPixels = 0;
    Trim(source, mask, pasteX, pasteY, numMaskPixels, pixelIndexToMatrixColumn);

    // naive paste
    NaivePaste(source, mask, dest, pasteX, pasteY, "out_paste_naive.png");

    // make the source image gradient and save it
    std::vector<float> sourceGradient;
    MakeImageGradient(source, mask, sourceGradient);
    SaveImageGradient(source, mask, sourceGradient, "out_gradient.png");

    // naive gradient paste
    NaiveGradientPaste(source, sourceGradient, dest, pasteX, pasteY, "out_paste_naivegrad.png");

    //DerivativePaste(source, dest, pasteX, pasteY, "out")

    return 0;
}

/*

TODO:

* making matrix:
 * some details to work out still but...
 * make a row per pixel in the mask. simple formula of "4 times center minus the 4 neighbors", don't worry about being fancy
 * then make a row per boundary condition pixel.
 * solve.
 * a detail to work out: do we make a row per pixel in the mask, only for the pixels which have all 4 neighbors in the mask? maybe yes...

? try alpha fading the edges of the image in naive paste to hide the discontinuity?
 * there is no such thing as a naive gradient paste. you have a 2 axis constraint :/

- need some demo image(s) for blog post


BLOG:
- link to github but put this source up
- and link to stb
- https://erkaman.github.io/posts/poisson_blending.html
- http://cs.brown.edu/courses/cs129/results/proj2/taox/
- link to least squares post too (erm... gauss elimination)

- show resulting image, and also the one w/o least squares, and a naive paste too

* eigen better than my code. I reinvented the wheel to learn more about how the wheel works.

? i wonder if this could help at all with style transfer? it would for color changes but not for feature changes.

* sRGB / clamp
 * could try putting constraints not to clamp in solve. "Duality"?
 * maybe also find the ones that clip, set those as "knowns" and resolve?

* real time considerations:
 * could pre-calculate an inverted matrix. The matrix is only based on the mask, so could re-use for same image, or same mask but different images.
  * you would make a vector of values consisting of desired derivatives and boundary constraints.
  * then you would multiply to get pixel values.
 * That matrix is large which sucks, but i've seen patterns in it, so may not need to actually store it, but can instead just get coefficients on the fly.
 * unfortunately, every pixel would still need all boundary conditions and all derivatives to do the work ):
 * so, seems infeasible to do in a pixel shader for instance.
 * could maybe do lower res derivatives and boundary conditions and lerp etc though. shrug.

* rocket.png rocketmask.png scenery.png 250 150

*/