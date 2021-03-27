# VQUtils

A Collection of Windows C++ utility functions for querying system info, string manipulation & formatting, date/time, logging, timer, etc.

## Feature List

 - System Info 
 - Timer
 - Multithreading: Threadpool, synchronization structs
 - Logging: Console &/| File
 - Image Loading: 32bit & HDR formats
 - String Utilities
 - Directory Utilities
 - Math Utilities


<br/>

System info sample output:

```
------------------- SYSTEM INFO -------------------
CPU
	Manufacturer        : AuthenticAMD
	Model               : AMD Ryzen 9 3900X 12-Core Processor
	Cores/Threads       : 12/24
	L3 Cache            : 4 x 16MB, 64MB Total L3
	L2 Cache            : 12 x 512KB, 6MB Total L2
	L1 D-Cache          : 12 x 32KB
	L1 I-Cache          : 12 x 32KB
	Cache Lines         : 64B

RAM
	Phys. Total Memory  : 64GB
	Phys. Free  Memory  : 38GB
	Usage %             : 40%
	Virt. Total Memory  : 128TB
	Virt. Free  Memory  : 128TB

GPU 0
	DeviceName          : AMD Radeon RX 5700 XT
	Memory              : 8GB
	DeviceID            : 0x731f
	VendorID            : 0x1002

Display 0
	DeviceName         : XG270HU
	LogicalDeviceName  : \\.\DISPLAY1
	Native Resolution  : 2560x1440
	HDR Support        : No
	Brightness
	  MinLuminance     : 0.500 Nits
	  MaxLuminance     : 270 Nits
	  MaxFullFrameLum  : 270 Nits

Display 1
	DeviceName         : DELL U2718Q
	LogicalDeviceName  : \\.\DISPLAY2
	Native Resolution  : 3840x2160
	HDR Support        : Yes
	Chromaticities
	  RedPrimary_xy    : 0.64, 0.33
	  GreenPrimary_xy  : 0.30, 0.60
	  BluePrimary_xy   : 0.15, 0.06
	  WhitePoint_xy    : 0.31, 0.33
	Brightness
	  MinLuminance     : 0.000 Nits
	  MaxLuminance     : 1000 Nits
	  MaxFullFrameLum  : 400 Nits

Display 2
	DeviceName         : DELL P2417H
	LogicalDeviceName  : \\.\DISPLAY3
	Native Resolution  : 1080x1920
	HDR Support        : No
	Brightness
	  MinLuminance     : 0.500 Nits
	  MaxLuminance     : 270 Nits
	  MaxFullFrameLum  : 270 Nits
------------------- SYSTEM INFO -------------------
```

<br/>

String formatting example

	Log::Info("Allocation Size : %s", StrUtil::FormatByte(this->mAllocSize).c_str());
	// [2021_03_27-17:58:57]   [INFO]	: Allocation Size : 256KB

<br/>

Directory Utilities


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
		std::vector<std::string> GetFlattenedFolderHierarchy(const std::string& path);

		// returns true of the given file exists.
		bool		FileExists(const std::string& pathToFile);

		// returns true if folder exists, false otherwise after creating the folder
		bool		CreateFolderIfItDoesntExist(const std::string& directoryPath);

		// returns the folder path given a file path
		// e.g.: @pathToFile="C:\\Folder\\File.txt" -> returns "C:\\Folder\\"
		std::string GetFolderPath(const std::string& pathToFile);

		// returns true if the given @pathToImageFile ends with .jpg, .png or .hdr
		bool		IsImageName(const std::string& pathToImageFile);

		// returns true if @file0 has been written into later than @file1 has.
		bool		IsFileNewer(const std::string& file0, const std::string& file1);

		// returns a list of files, filtered by the extension if specified
		std::vector<std::string> ListFilesInDirectory(const std::string& Directory, const char* FileExtension = nullptr);
	}

Date Time

	// returns current time in format "YYYY-MM-DD_HH-MM-SS"
	std::string GetCurrentTimeAsString();
	std::string GetCurrentTimeAsStringWithBrackets();


