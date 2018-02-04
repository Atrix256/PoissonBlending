#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

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

void NaivePaste (const SImageInfo &source, const SImageInfo &dest, int pasteX, int pasteY, const char* fileName)
{
    // TODO: implement this and the others. How do we make an image and save it?
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

        if (!source.Load(argv[1]) || !source.Load(argv[2]))
        {
            return 2;
        }

        if (!sscanf(argv[3], "%i", &pasteX) || !sscanf(argv[4], "%i", &pasteY))
        {
            printf("could not read width or height\n");
            return 3;
        }
    }

    // do the pasting
    NaivePaste(source, dest, pasteX, pasteY, "out_naive.png");

    return 0;
}

/*

TODO:

- load and save images with stb
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

*/