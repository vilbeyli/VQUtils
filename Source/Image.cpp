//	VQUtils
//	Copyright(C) 2020 - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#include "Image.h"
#include "Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../Libs/stb/stb_image.h"

#include <vector>
#include <cmath>

static float CalculateMaxLuminance(const float* pData, int width, int height, int numComponents)
{
    float MaxLuminance = 0.0f;
    float MaxLuminance2 = 0.0f;
    float MaxLuminance3 = 0.0f;
    float MaxLightLevel = 0.0f;
    float LuminanceSum = 0.0f;
    
    for(int h=0; h< height; ++h) // row
    for(int w = 0; w < width; ++w) // col
    {
        const int pxIndex = (w  + width * h) * numComponents;
        const float& r = pData[pxIndex + 0];
        const float& g = pData[pxIndex + 1];
        const float& b = pData[pxIndex + 2];

        // https://en.wikipedia.org/wiki/Relative_luminance
        const float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
        if (lum > MaxLuminance) 
            MaxLuminance = lum;

        const float lum2 = 0.299f * r + 0.587f * g + 0.114f * b;
        if (lum2 > MaxLuminance2)
            MaxLuminance2 = lum2;

        const float lum3 = std::sqrtf(0.299f * r * r + 0.587f * g * g + 0.114f * b * b);
        if (lum3 > MaxLuminance3)
            MaxLuminance3 = lum3;



        if (MaxLightLevel <= r)
            MaxLightLevel = r;
        if (MaxLightLevel <= g)
            MaxLightLevel = g;
        if (MaxLightLevel <= b)
            MaxLightLevel = b;


        LuminanceSum += lum;;
    }

    // calc avg lum
    LuminanceSum /= width * height; // TODO:

    return MaxLuminance;
}

Image Image::LoadFromFile(const char* pFilePath, bool bHDR)
{
    Image img;
    constexpr int reqComp = 0;
    int NumImageComponents = 0;
    img.pData = bHDR
        ? (void*)stbi_loadf(pFilePath, &img.x, &img.y, &NumImageComponents, 4)
        : (void*)stbi_load (pFilePath, &img.x, &img.y, &NumImageComponents, 4);
    if (img.pData == nullptr)
    {
        Log::Error("Error loading file: %s", pFilePath);
    }
    img.BytesPerPixel = bHDR 
        ? NumImageComponents * 4 // HDR=RGBA32F -> 16 Bytes/Pixel = 4 Bytes / component
        : NumImageComponents;    // SDR=RGBA8   -> 4  Bytes/Pixel = 1 Byte  / component

    if (img.pData && bHDR)
    {
        img.MaxLuminance = CalculateMaxLuminance(static_cast<const float*>(img.pData), img.Width, img.Height, 4);
    }

    return img;
}

Image Image::CreateEmptyImage(size_t bytes)
{
    Image img;
    img.pData = (void*)malloc(bytes);
    return img;
}

void Image::Destroy()
{ 
	if (pData) 
	{ 
		stbi_image_free(pData); 
		pData = nullptr; 
	} 
}

unsigned short Image::CalculateMipLevelCount(unsigned __int64 w, unsigned __int64 h)
{
    int mips = 0;
    while (w >= 1 && h >= 1)
    {
        ++mips;
        w >>= 1;
        h >>= 1;
    }
    return mips;
}
