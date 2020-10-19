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

#pragma once

struct Image
{
    static Image LoadFromFile(const char* pFilePath, bool bHDR);
    static Image CreateEmptyImage(size_t bytes);

    void Destroy();  // Destroy must be called following a LoadFromFile() to prevent memory leak

    static unsigned short CalculateMipLevelCount(unsigned __int64 w, unsigned __int64 h);
    inline unsigned short CalculateMipLevelCount() const { return CalculateMipLevelCount(this->Width, this->Height); };

    union { int x; int Width; };
    union { int y; int Height; };
    int BytesPerPixel = 0;
    void* pData = nullptr;
    float MaxLuminance;
};