#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <algorithm>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "stb/stb_image_write.h"

struct SImageInfo
{
    stbi_uc* m_pixels   = nullptr;
    int m_width         = 0;
    int m_height        = 0;
    int m_channels      = 0;

    bool Load (const char *fileName)
    {
        m_pixels = stbi_load(fileName, &m_width, &m_height, &m_channels, 3);
        if (m_pixels == nullptr)
            printf("Could not load file %s\n", fileName);
        return m_pixels != nullptr;
    }

    ~SImageInfo()
    {
        stbi_image_free(m_pixels);
    }
};

struct SRect
{
    int x1, y1, x2, y2;
};

bool WriteImage (const char *fileName, int width, int height, stbi_uc* pixels)
{
    return stbi_write_png(fileName, width, height, 3, pixels, 3 * width) != 0;
}

void NaivePaste (const SImageInfo &source, const SImageInfo &dest, int pasteX, int pasteY, const char* fileName)
{
    // copy the destination image to an out image buffer
    std::vector<stbi_uc> outPixels;
    outPixels.resize(dest.m_width*dest.m_height * 3);
    memcpy(&outPixels[0], dest.m_pixels, outPixels.size());

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
            stbi_uc* sourcePixels = &source.m_pixels[((sourceRect.y1 + y)*source.m_width + sourceRect.x1) * 3];
            stbi_uc* destPixels = &outPixels[((destRect.y1 + y)*dest.m_width + destRect.x1) * 3];
            memcpy(destPixels, sourcePixels, copyWidth * 3);
        }
    }

    // write the file
    if (!WriteImage(fileName, dest.m_width, dest.m_height, &outPixels[0]))
        printf(__FUNCTION__ "() error: Could not write %s\n", fileName);
}

void MakeImageGradient(const SImageInfo& source, std::vector<stbi_uc>& sourceGradient)
{
    // allocate space for the gradients
    sourceGradient.resize(source.m_width*source.m_height * 6);
    std::fill(sourceGradient.begin(), sourceGradient.end(), 0);

    // make the gradients!
    const stbi_uc* sourcePixel = source.m_pixels;
    const stbi_uc* sourcePixelNextRow = sourcePixel + source.m_width * 3;

    stbi_uc* destPixel = &sourceGradient[0];
    for (int y = 0; y < source.m_height-1; ++y)
    {
        for (int x = 0; x < source.m_width-1; ++x)
        {
            // calculate RGB dfdx
            destPixel[0] = sourcePixel[3] - sourcePixel[0];
            destPixel[1] = sourcePixel[4] - sourcePixel[1];
            destPixel[2] = sourcePixel[5] - sourcePixel[2];

            // calculate RGB dfdy
            destPixel[3] = sourcePixelNextRow[0] - sourcePixel[0];
            destPixel[4] = sourcePixelNextRow[1] - sourcePixel[1];
            destPixel[5] = sourcePixelNextRow[2] - sourcePixel[2];

            // move to the next pixels
            sourcePixel += 3;
            sourcePixelNextRow += 3;
            destPixel += 6;
        }
    }
}

void SaveImageGradient(const std::vector<stbi_uc>& sourceGradient, const char* fileName)
{
    // todo: convert from +/-255 to +/-1, then to 0,1 and write that out.  maybe two images, one for x and one for y? or stack them horizontally?
    int ijkl = 0;
}

int main(int argc, char** argv)
{
    SImageInfo source, dest, output;
    int pasteX, pasteY;

    // get parameters and load images
    {
        if (argc < 5)
        {
            printf("usage: <source> <dest> <x> <y>\n");
            return 1;
        }

        if (!source.Load(argv[1]) || !dest.Load(argv[2]))
        {
            return 2;
        }

        if (!sscanf(argv[3], "%i", &pasteX) || !sscanf(argv[4], "%i", &pasteY))
        {
            printf("could not read width or height\n");
            return 3;
        }
    }

    // naive paste
    NaivePaste(source, dest, pasteX, pasteY, "out_naive.png");

    // make derivatives for the image and save them off
    std::vector<stbi_uc> sourceGradient;
    MakeImageGradient(source, sourceGradient);
    SaveImageGradient(sourceGradient, "out_gradient.png");

    //DerivativePaste(source, dest, pasteX, pasteY, "out")

    return 0;
}

/*

TODO:

? do we need a mask texture that's the same size as source?
- command line for source, dest, dest position. maybe a source mask too?
- solve with least squares if possible?
- save result

- need some demo image(s) for blog post
- show without the least squares fitting part
- and show a naive paste!


BLOG:
- link to github but put this source up
- and link to stb
- https://erkaman.github.io/posts/poisson_blending.html
- http://cs.brown.edu/courses/cs129/results/proj2/taox/
- link to least squares post too

- show resulting image, and also the one w/o least squares, and a naive paste too

? i wonder if this could help at all with style transfer? it would for color changes but not for feature changes.

* paste the rocket at  250, 150

*/