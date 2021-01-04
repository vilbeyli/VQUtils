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

#include "utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

#include <ctime>

#ifdef _DEBUG
#include <cassert>
#endif

#ifdef _WIN32
#include <windows.h>
#include "shlobj.h"		// SHGetKnownFolderPath()
#endif

// 2011**L::C++11 | 201402L::C++14 | 201703L::C++17
#if _MSVC_LANG >= 201703L	// CPP17
#include <filesystem>

namespace filesys = std::filesystem;
#else
#include <fstream>
#endif

#include <algorithm>
#include <cassert>

#include "Log.h"

namespace StrUtil
{
	using std::vector;
	using std::string;
	using std::cout;
	using std::endl;

	bool IsNumber(const std::string& s)
	{
		// https://stackoverflow.com/a/4654718/2034041
		std::string::const_iterator it = s.begin();
		while (it != s.end() && std::isdigit(*it)) ++it;
		return !s.empty() && it == s.end();
	}

	vector<string> split(const char* s, char c)
	{
		vector<string> result;
		do
		{
			const char* begin = s;

			if (*begin == c || *begin == '\0') 
				continue;	// skip delimiter character

			while (*s != c && *s)	
				s++;	// iterate until delimiter is found

			result.push_back(string(begin, s));

		} while (*s++);
		return result;
	}

	vector<string> split(const string& str, char c)
	{
		return split(str.c_str(), c);
	}

	std::vector<std::string> split(const std::string & s, const std::vector<char>& delimiters)
	{
		vector<string> result;
		const char* ps = s.c_str();
		auto& IsDelimiter = [&delimiters](const char c)
		{
			return std::find(delimiters.begin(), delimiters.end(), c) != delimiters.end();
		};

		do
		{
			const char* begin = ps;

			if (IsDelimiter(*begin) || (*begin == '\0')) 
				continue;	// skip delimiter characters

			while (!IsDelimiter(*ps) && *ps) 
				ps++;	// iterate until delimiter is found or string has ended

			result.push_back(string(begin, ps));

		} while (*ps++);
		return result;
	}

	std::string trim(const std::string& s)
	{
		size_t end = s.find_last_not_of(" \n\r\t\f\v");
		size_t begin = s.find_first_not_of(" \n\r\t\f\v");
		size_t range = end + 1 - begin;
		return (end == std::string::npos) ? "" : s.substr(begin, range);
	}

	void MakeLowercase(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
	}

	std::string GetLowercased(const std::string& str)
	{
		std::string lowercased(str.size(), '\0');
		std::transform(str.begin(), str.end(), lowercased.begin(), [](unsigned char c) { return std::tolower(c); });
		return lowercased;
	}

	std::string CommaSeparatedNumber(const std::string& num)
	{
		std::string _num = "";
		int i = 0;
		for (auto it = num.rbegin(); it != num.rend(); ++it)
		{
			if (i % 3 == 0 && i != 0)
			{
				_num += ",";
			}
			_num += *it;
			++i;
		}
		return std::string(_num.rbegin(), _num.rend());
	}
	std::string UnicodeToASCII(const PWSTR pwstr)
	{
		const std::wstring wstr(pwstr);
#if _WIN32
		// https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/
		if (wstr.empty()) return std::string();
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
#else
		return std::string(wstr.begin(), wstr.end());  // results in warning in C++17
#endif
	}
	std::string FormatByte(unsigned long long bytes)
	{
		constexpr unsigned long long KILOBYTE = 1024ull;
		constexpr unsigned long long MEGABYTE = 1024ull * KILOBYTE;
		constexpr unsigned long long GIGABYTE = 1024ull * MEGABYTE;
		constexpr unsigned long long TERABYTE = 1024ull * GIGABYTE;

		std::string unit = "B";
		double newMagnitudeInUnits = static_cast<double>(bytes);

		if (bytes > KILOBYTE)
		{
			unit = "KB";
			newMagnitudeInUnits /= 1024.0;
		}
		if (bytes > MEGABYTE)
		{
			unit = "MB";
			newMagnitudeInUnits /= 1024.0;
		}
		if (bytes > GIGABYTE)
		{
			unit = "GB";
			newMagnitudeInUnits /= 1024.0;
		}
		if (bytes > TERABYTE)
		{
			unit = "TB";
			newMagnitudeInUnits /= 1024.0;
		}

		std::ostringstream ss;
		ss << std::fixed << std::setprecision(0) << newMagnitudeInUnits << unit;
		return ss.str();
	}
}

//---------------------------------------------------------------------------------------------

namespace DirectoryUtil
{
	std::vector<std::string> GetFlattenedFolderHierarchy(const std::string& path)
	{
		std::string sanitizedPath = path;
		std::replace(sanitizedPath.begin(), sanitizedPath.end(), '/', '\\');
		return StrUtil::split(sanitizedPath, '\\');
	}
	std::string GetSpecialFolderPath(ESpecialFolder folder)
	{
#if _WIN32
		PWSTR retPath = {};
		REFKNOWNFOLDERID folder_id = [&]()
		{
			switch (folder)
			{
			case PROGRAM_FILES:	return FOLDERID_ProgramFiles;
			case APPDATA:		return FOLDERID_RoamingAppData;
			case LOCALAPPDATA:	return FOLDERID_LocalAppData;
			case USERPROFILE:	return FOLDERID_UserProfiles;
			}
			return FOLDERID_RoamingAppData;
		}();

		HRESULT hr = SHGetKnownFolderPath(folder_id, 0, NULL, &retPath);
		if (hr != S_OK)
		{
			// Log::Error("SHGetKnownFolderPath() returned %s.", hr == E_FAIL ? "E_FAIL" : "E_INVALIDARG");
			return "";
		}
		return StrUtil::UnicodeToASCII(retPath);
#else
		assert(false);	// IMPLEMENT: platform-specific logic for other platforms
		return "";
#endif
	}

	std::string GetFolderPath(const std::string & pathToFile)
	{
		const auto tokens = StrUtil::split(pathToFile, '\\', '/');
		std::string path = "";
		std::for_each(RANGE(tokens), [&](const std::string& s) { if(s != tokens.back()) path += s + "/"; });
		return path;
	}

	bool IsImageName(const std::string & str)
	{
		std::vector<std::string> FileNameAndExtension = StrUtil::split(str, '.');
		if (FileNameAndExtension.size() < 2)
			return false;

		const std::string& extension = FileNameAndExtension[1];

		bool bIsImageFile = false;
		bIsImageFile = bIsImageFile || extension == "png";
		bIsImageFile = bIsImageFile || extension == "jpg";
		bIsImageFile = bIsImageFile || extension == "hdr";
		return bIsImageFile;
	}

	std::string GetFileNameWithoutExtension(const std::string& path)
	{	// example: path: "Archetypes/player.txt" | return val: "player"
		const std::vector<std::string> pathTokens = StrUtil::split(path.c_str(), '.');
#if _DEBUG
		if (pathTokens.size() == 0)
		{
			Log::Warning("Empty tokens: path=" + path);
		}
#endif
		assert(pathTokens.size() > 0);
		const std::string& no_extension = pathTokens[0];
		return StrUtil::split(no_extension.c_str(), '/').back();
	}


	std::string GetFileNameFromPath(const std::string& filePath)
	{
		return StrUtil::split(filePath.c_str(), { '/', '\\' }).back();
	}

	std::string GetFileExtension(const std::string& filePath)
	{
		auto v = StrUtil::split(filePath, '.');
		return v.empty() ? "" : v.back();
	}

	std::string GetCurrentPath()
	{
		char path[MAX_PATH];
		GetModuleFileName(NULL, path, MAX_PATH);
		std::string::size_type pos = std::string(path).find_last_of("\\/");
		return std::string(path).substr(0, pos+1);
	}

	bool FileExists(const std::string & pathToFile)
	{	// src: https://msdn.microsoft.com/en-us/library/b0084kay.aspx
#if _MSVC_LANG >= 201703L	// CPP17
		return filesys::exists(pathToFile);
#else
		std::ifstream infile(pathToFile);
		return infile.good();
#endif
	}

	bool CreateFolderIfItDoesntExist(const std::string& directoryPath)
	{
		if (CreateDirectory(directoryPath.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError())
		{
			;// directory either successfully created or already exists: NOP
			return true;
		}
		else
		{
			std::string errMsg = "Failed to create directory: " + directoryPath;
			MessageBox(NULL, errMsg.c_str(), "Error Creating Folder", MB_OK);
			return false;
		}
	}

	bool IsFileNewer(const std::string & file0, const std::string & file1)
	{
		if (!DirectoryUtil::FileExists(file0)) { Log::Warning("DirectoryUtils::IsFileNewer(): File %s doesn't exist", file0.c_str()); return false; }
		if (!DirectoryUtil::FileExists(file1)) { Log::Warning("DirectoryUtils::IsFileNewer(): File %s doesn't exist", file1.c_str()); return true;  }

#if _MSVC_LANG >= 201703L // CPP17 
		return filesys::last_write_time(file0) > filesys::last_write_time(file1);
#else
		FILETIME ftCreate[2], ftAccess[2], ftWrite[2];
		HANDLE hFile0 = CreateFile(file0.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		HANDLE hFile1 = CreateFile(file1.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		GetFileTime(hFile0, &ftCreate[0], &ftAccess[0], &ftWrite[0]);
		GetFileTime(hFile1, &ftCreate[1], &ftAccess[1], &ftWrite[1]);
		return CompareFileTime(&ftWrite[0], &ftWrite[1]) == 1;
#endif
	}
	std::vector<std::string> ListFilesInDirectory(const std::string& Directory, const char* FileExtension)
	{
		if (FileExtension)
		{
			assert(strlen(FileExtension) > 0);
			if (FileExtension[0] == '.')
				FileExtension = &FileExtension[1];
		}
		std::vector<std::string> files;
		for (const auto& entry : std::filesystem::directory_iterator(Directory))
		{
			if (entry.is_directory()) continue;
			if (FileExtension)
			{
				if (DirectoryUtil::GetFileExtension(entry.path().string()) == FileExtension)
				{
					files.push_back(entry.path().string());
				}
			}
			else
			{
				files.push_back("");
			}
		}
		return files;
	}
}

//---------------------------------------------------------------------------------------------

namespace MathUtil
{
	float RandF(float l, float h)
	{
		if (l > h)
		{	// swap params in case order is wrong
			float tmp = l;
			l = h;
			h = tmp;
		}
		thread_local std::mt19937_64 generator(std::random_device{}());
		std::uniform_real_distribution<float> distribution(l, h);
		return distribution(generator);
	}

	// [)
	int RandI(int l, int h)
	{
		int offset = rand() % (h - l);
		return l + offset;
	}
	size_t RandU(size_t l, size_t h)
	{
#ifdef _DEBUG
		assert(l <= h);
#endif
		int offset = rand() % (h - l);
		return l + static_cast<size_t>(offset);
	}

}



// GLOBAL NAMESPACE
//
std::string GetCurrentTimeAsString()
{
	const std::time_t now = std::time(0);
	std::tm tmNow;	// current time
	localtime_s(&tmNow, &now);

	// YYYY-MM-DD_HH-MM-SS
	std::stringstream ss;
	ss << (tmNow.tm_year + 1900) << "_"
		<< std::setfill('0') << std::setw(2) << tmNow.tm_mon + 1 << "_"
		<< std::setfill('0') << std::setw(2) << tmNow.tm_mday << "-"
		<< std::setfill('0') << std::setw(2) << tmNow.tm_hour << ":"
		<< std::setfill('0') << std::setw(2) << tmNow.tm_min << ":"
		<< std::setfill('0') << std::setw(2) << tmNow.tm_sec;
	return ss.str();
}
std::string GetCurrentTimeAsStringWithBrackets() { return "[" + GetCurrentTimeAsString() + "]"; }




//std::string ImageFormatToFileExtension(const EImageFormat format)
//{
//	std::string ext = "";
//	switch (format)
//	{
//	case EImageFormat::D32F:
//	case EImageFormat::RGBA16F:
//	case EImageFormat::RGBA32F:
//	case EImageFormat::RGB32F:
//	case EImageFormat::R32F:
//		ext = ".hdr";
//		break;
//	default:
//		ext = ".png";
//		break;
//	}
//	return ext;
//}
