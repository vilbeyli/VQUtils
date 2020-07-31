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

Image Image::LoadFromFile(const char* pFilePath, bool bHDR)
{
    Image img;
    constexpr int reqComp = 0;
    int NumImageComponents = 0;
    img.pData = bHDR
        ? (void*)stbi_loadf(pFilePath, &img.x, &img.y, &NumImageComponents, 4)
        : (void*)stbi_load (pFilePath, &img.x, &img.y, &NumImageComponents, 0);
    if (img.pData == nullptr)
    {
        Log::Error("Error loading file: %s", pFilePath);
    }
    img.BytesPerPixel = bHDR ? NumImageComponents * 6 /* RGB16F(6) vs RGBA16F(8) ?*/ : NumImageComponents;
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

