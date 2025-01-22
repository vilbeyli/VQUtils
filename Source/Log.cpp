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

#include "Log.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cassert>

#include <fcntl.h>
#include <io.h>

#include <Windows.h>

// Calls error/warning/info with "Test" right after initialization
#define LOG_RUN_UNIT_TEST 0

#define MAX_CONSOLE_LINES 500

namespace Log
{
using namespace std;

constexpr const char* VQ_DEFAULT_LOGFILE_NAME = "VQLog.txt";
static std::ofstream sOutFile;

// checks if the specifiec path is only a file name, construct absolute path
// - if yes, CurrentPath+FileName
// - if no, check wether absolute path is provided
//    - if absolute, use that path as final path
//    - if relative, use CurrentPath + provided path as final path
std::string ParseAndValidateArgument(const char* pStrFilePath)
{
	const std::string LogFileDir = strlen(pStrFilePath) == 0 ? VQ_DEFAULT_LOGFILE_NAME : pStrFilePath;
	const std::string CurrPath = DirectoryUtil::GetCurrentPath();

	const bool bNoFilePathProvided = LogFileDir.empty();

	const std::vector<std::string> LogFileDirTokens = StrUtil::split(LogFileDir, { '/', '\\' });
	const std::string& root = LogFileDirTokens[0];
	const bool bOnlyFilenameProvided = LogFileDirTokens.size() == 1;
	const bool bAbsolutePathProvided = root[1] == ':' && (root[2] == '/' || root[2] == '\\');
	
	std::string FinalAbsolutePath = "";
	if (bOnlyFilenameProvided)
	{
		FinalAbsolutePath = CurrPath + LogFileDir;
	}
	else
	{
		if (bAbsolutePathProvided)
		{
			FinalAbsolutePath = pStrFilePath;
		}
		else
		{
			FinalAbsolutePath = CurrPath + LogFileDir;
		}
	}
	std::replace(FinalAbsolutePath.begin(), FinalAbsolutePath.end(), '/', '\\');
	return FinalAbsolutePath;
}

void CreateFolderHierarchy(const std::string& logfileDir, std::string& errMsg)
{
	assert(errMsg.size() == 0);
	std::vector<std::string> folders = StrUtil::split(logfileDir, {'/', '\\'});
	const std::string root = folders[0];

	assert(folders.size() > 1); // assume a root path like 'C:/' is not given
	folders = std::vector<std::string>(folders.begin() + 1, folders.end());

	std::string folderAbsolutePath = root + "/";
	for (const std::string& folder : folders)
	{
		folderAbsolutePath += folder + "/";
		if (CreateDirectory(folderAbsolutePath.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError())
		{

		}
		else
		{
			errMsg = "Failed to create directory " + folderAbsolutePath;
			break;
		}
	}
}

void InitLogFile(const char* pStrFilePath)
{
	const std::string logfileAbsolutePath = ParseAndValidateArgument(pStrFilePath);
	const std::string logfileDir = DirectoryUtil::GetFolderPath(logfileAbsolutePath);

	std::string errMsg = "";

	CreateFolderHierarchy(logfileDir, errMsg);

	sOutFile.open(logfileAbsolutePath);
	if (sOutFile)
	{
		std::string msg = GetCurrentTimeAsStringWithBrackets() + "[Log] " + "Logging initialized: " + logfileAbsolutePath  + "\n";
		sOutFile << msg;
		cout << msg << endl;
	}
	else
	{
		errMsg += "\nCannot open log file : " + logfileAbsolutePath;
	}
	

	if (!errMsg.empty())
	{
		MessageBox(NULL, errMsg.c_str(), "VQEngine: Error Initializing Logging", MB_OK);
	}
}
void InitConsole()
{
	// src: https://stackoverflow.com/a/46050762/2034041
	//void RedirectIOToConsole() 
	{
		//Create a console for this application
		AllocConsole();

		// Get STDOUT handle
		HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
		std::FILE *COutputHandle = _fdopen(SystemOutput, "w");

		// Get STDERR handle
		HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
		int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
		std::FILE *CErrorHandle = _fdopen(SystemError, "w");

		// Get STDIN handle
		HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
		int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
		std::FILE *CInputHandle = _fdopen(SystemInput, "r");

		//make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
		ios::sync_with_stdio(true);

		// Redirect the CRT standard input, output, and error handles to the console
		freopen_s(&CInputHandle, "CONIN$", "r", stdin);
		freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
		freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

		//Clear the error state for each of the C++ standard stream objects. We need to do this, as
		//attempts to access the standard streams before they refer to a valid target will cause the
		//iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
		//to always occur during startup regardless of whether anything has been read from or written to
		//the console or not.
		std::wcout.clear();
		std::cout.clear();
		std::wcerr.clear();
		std::cerr.clear();
		std::wcin.clear();
		std::cin.clear();
	}
}

void Initialize(const LogInitializeParams& params)
{
	if (params.bLogConsole) InitConsole();
	if (params.bLogFile)    InitLogFile(params.LogFilePath.c_str());

#if LOG_RUN_UNIT_TEST
	Log::Info("Test");
	Log::Error("Test");
	Log::Warning("Test");
#endif
}

void Destroy()
{
	std::string msg = GetCurrentTimeAsStringWithBrackets() + "[Log] Exit()";
	if (sOutFile.is_open())
	{
		sOutFile << msg;
		sOutFile.close();
	}
	cout << msg;
	OutputDebugString(msg.c_str());
}

void Error(const std::string & s)
{
	std::string err = GetCurrentTimeAsStringWithBrackets() + "  [ERROR]\t: ";
	err += s + "\n";
	
	OutputDebugString(err.c_str());		// vs
	if (sOutFile.is_open()) 
		sOutFile << err;				// file
	cout << err;						// console
}

void Warning(const std::string & s)
{
	std::string warn = GetCurrentTimeAsStringWithBrackets() + "[WARNING]\t: ";
	warn += s + "\n";
	
	OutputDebugString(warn.c_str());
	if (sOutFile.is_open()) 
		sOutFile << warn;
	cout << warn;
}

void Info(const std::string & s)
{
	std::string info = GetCurrentTimeAsStringWithBrackets() + "   [INFO]\t: " + s + "\n";
	OutputDebugString(info.c_str());
	
	if (sOutFile.is_open()) 
		sOutFile << info;
	cout << info;
}

}	// namespace Log