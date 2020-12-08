//	VQUtils 
//	Copyright(C) 2020  - Volkan Ilbeyli
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

#include <string>
#include <sstream>
#include <vector>
#include <locale>

#if _WIN32
	#include "Windows.h"
#endif

#define RANGE(c)  std::begin(c) , std::end(c)
#define RRANGE(c) std::rbegin(c), std::rend(c)



namespace StrUtil
{
	bool IsNumber(const std::string& s);

	inline bool  ParseBool (const std::string& s) { bool b; std::istringstream(s) >> std::boolalpha >> b; return b; }
	inline int   ParseInt  (const std::string& s) { return std::atoi(s.c_str()); }
	inline float ParseFloat(const std::string& s) { return static_cast<float>(std::atof(s.c_str())); }

	std::vector<std::string> split(const char* s, char c = ' ');
	std::vector<std::string> split(const std::string& s, char c = ' ');
	std::vector<std::string> split(const std::string& s, const std::vector<char>& delimiters);
	template<class... Args> std::vector<std::string> split(const std::string& s, Args&&... args)
	{
		const std::vector<char> delimiters = { args... };
		return split(s, delimiters);
	}
	std::string trim(const std::string& s);

	void MakeLowercase(std::string& str);
	std::string GetLowercased(const std::string& str);

	std::string  CommaSeparatedNumber(const std::string& num);

	       std::string  UnicodeToASCII(const PWSTR pwstr); // https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/
	inline std::wstring ASCIIToUnicode(const std::string& str) { return std::wstring(str.begin(), str.end()); }

	template<unsigned STR_SIZE>
	std::string UnicodeToASCII(const WCHAR wchars[STR_SIZE])
	{
		char ascii[STR_SIZE];
		size_t numCharsConverted = 0;
		wcstombs_s(&numCharsConverted, ascii, wchars, STR_SIZE);
		return std::string(ascii);
	}

	std::string FormatByte(unsigned long long bytes);
}



namespace DirectoryUtil
{
	enum ESpecialFolder
	{
		PROGRAM_FILES,
		APPDATA,
		LOCALAPPDATA,
		USERPROFILE,
	};

	std::string	GetSpecialFolderPath(ESpecialFolder folder);
	std::string	GetFileNameWithoutExtension(const std::string&);
	std::string	GetFileNameFromPath(const std::string& filePath);
	std::string GetFileExtension(const std::string& filePath);
	std::string GetCurrentPath();

	// removes separators from a path and returns all the folder names in a vector
	// e.g. "C:/Program Files/TestFolder" -> ["C:", "Program Files", "TestFolder"]
	//
	std::vector<std::string> GetFlattenedFolderHierarchy(const std::string& path);

	// returns true of the given file exists.
	//
	bool		FileExists(const std::string& pathToFile);

	// returns true if folder exists, false otherwise after creating the folder
	//
	bool		CreateFolderIfItDoesntExist(const std::string& directoryPath);

	// returns the folder path given a file path
	// e.g.: @pathToFile="C:\\Folder\\File.txt" -> returns "C:\\Folder\\"
	//
	std::string GetFolderPath(const std::string& pathToFile);

	// returns true if the given @pathToImageFile ends with .jpg, .png or .hdr
	//
	bool		IsImageName(const std::string& pathToImageFile);

	// returns true if @file0 has been written into later than @file1 has.
	//
	bool		IsFileNewer(const std::string& file0, const std::string& file1);

	// returns a list of files, filtered by the extension if specified
	//
	std::vector<std::string> ListFilesInDirectory(const std::string& Directory, const char* FileExtension = nullptr);
}


// returns current time in format "YYYY-MM-DD_HH-MM-SS"
std::string GetCurrentTimeAsString();
std::string GetCurrentTimeAsStringWithBrackets();




namespace MathUtil
{
	template<class T> inline T lerp(T low, T high, float t) { return low + static_cast<T>((high - low) * t); }
	template<class T> void ClampedIncrementOrDecrement(T& val, int upOrDown, int lo, int hi)
	{
		int newVal = static_cast<int>(val) + upOrDown;
		if (upOrDown > 0) newVal = newVal >= hi ? (hi - 1) : newVal;
		else              newVal = newVal < lo ? lo : newVal;
		val = static_cast<T>(newVal);
	}
	template<class T> void Clamp(T& val, const T lo, const T hi)
	{
		val = min(val, lo);
		val = max(val, hi);
	}
	template<class T> T Clamp(const T& val, const T lo, const T hi)
	{
		T _val = val;
		Clamp(_val, lo, hi);
		return _val;
	}

	float	RandF(float l, float h);
	int		RandI(int l, int h);
	size_t	RandU(size_t l, size_t h);
}


//#include "Renderer/RenderingEnums.h"
//std::string ImageFormatToFileExtension(const EImageFormat format);


