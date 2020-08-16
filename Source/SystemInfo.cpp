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

#define NOMINMAX

#include "SystemInfo.h"
#include "utils.h"

#include <intrin.h> // __cpuid
#include <atlbase.h> // ComPtr
#include <wrl/client.h>

#include <functional>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <iomanip>

#define VERBOSE_LOGGING 0
#if VERBOSE_LOGGING
#include "Log.h"
#endif

constexpr bool DISPLAY_DEVICE_NAME__READ_REGISTRY = true;         // Reading EDID is slow, especially on Debug (>1s / monitor)
constexpr bool DISPLAY_DEVICE_NAME__USE_DISPLAY_INFO_API = false; // TODO: finalize implementation

// src: https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation
// Helper function to count set bits in the processor mask.
static DWORD CountSetBits(ULONG_PTR bitMask) 
{
	DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
	for (DWORD i = 0; i <= LSHIFT; ++i) 
	{
		bitSetCount += ((bitMask & bitTest) ? 1 : 0);
		bitTest /= 2;
	}
	return bitSetCount;
}

namespace VQSystemInfo
{
// ----------------------------------------------------------------------------------------------------------------------------------------------
//
// RAM
//
// ----------------------------------------------------------------------------------------------------------------------------------------------
FRAMInfo GetRAMInfo()
{
	// https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-globalmemorystatusex?redirectedfrom=MSDN
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	FRAMInfo i;
	i.UsagePercentage = statex.dwMemoryLoad;
	i.TotalPhysicalMemory = statex.ullTotalPhys;
	i.FreePhysicalMemory = statex.ullAvailPhys;
	i.TotalPageFileSize = statex.ullTotalPageFile;
	i.FreePageFileSize = statex.ullAvailPageFile;
	i.TotalVirtualMemory = statex.ullTotalVirtual;
	i.FreeVirtualMemory = statex.ullAvailVirtual;
	i.ExtendedMemory = statex.ullAvailExtendedVirtual;
	return i;
}


// ----------------------------------------------------------------------------------------------------------------------------------------------
//
// Monitor
//
// ----------------------------------------------------------------------------------------------------------------------------------------------
// https://docs.microsoft.com/en-us/windows/win32/sysinfo/enumerating-registry-subkeys
static std::vector<std::string> GetSubKeys(HKEY hkey)
{
	std::vector<std::string> SubKeys;
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
	DWORD    cbName;                   // size of name string 
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for keyPath 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	DWORD i, retCode;

	TCHAR  achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hkey,                    // keyPath handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this keyPath 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 


	// Enumerate the subkeys, until RegEnumKeyEx fails.
	if (cSubKeys)
	{
		//Log::Info("\nNumber of subkeys: %d\n", cSubKeys);
		for (i = 0; i < cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = RegEnumKeyEx(hkey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				//Log::Info(TEXT("(%d) %s\n"), i + 1, achKey);
				SubKeys.push_back(achKey);
			}
		}
	}

	// Enumerate the keyPath values.
	if (cValues)
	{
		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			cchValue = MAX_VALUE_NAME;
			achValue[0] = '\0';
			retCode = RegEnumValue(hkey, i,
				achValue,
				&cchValue,
				NULL,
				NULL,
				NULL,
				NULL);

			if (retCode == ERROR_SUCCESS)
			{
#if VERBOSE_LOGGING
				Log::Info(TEXT("(%d) %s\n"), i + 1, achValue);
#endif
			}
		}
	}
	return SubKeys;
}
static bool DoesDisplayDriverHashMatch(const std::string& keyPath, const std::string& DriverHashToMatch)
{
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
	HKEY hkey;
	LSTATUS status = ERROR_SUCCESS;
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(keyPath.c_str()), 0, KEY_READ, &hkey);
	if (status != ERROR_SUCCESS)
	{
#if VERBOSE_LOGGING
		Log::Error("Coudn't find keyPath: %s", keyPath.c_str());
#endif
		return false;
	}

	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for keyPath 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	BYTE    cbEnumValue[MAX_VALUE_NAME] = TEXT("");

	DWORD i, retCode;

	TCHAR  achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hkey,                    // keyPath handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this keyPath 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 
	
	// Enumerate the keyPath values.
	if (cValues)
	{
		//printf("\nNumber of values: %d\n", cValues);
		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			cchValue = MAX_VALUE_NAME;
			achValue[0] = '\0';
			DWORD type = REG_SZ;
			DWORD size;
			memset(cbEnumValue, '\0', MAX_VALUE_NAME);

			// MSDN:  https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regenumvaluea
			// If the data has the REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ type, the string may not have been stored with 
			// the proper null-terminating characters. Therefore, even if the function returns ERROR_SUCCESS, the application 
			// should ensure that the string is properly terminated before using it; otherwise, it may overwrite a buffer.
			retCode = RegEnumValue(hkey, i,
				achValue,
				&cchValue,
				NULL,
				&type,
				NULL,
				&size);

			if (type != REG_SZ)
			{
				continue;
			}

			if (retCode == ERROR_SUCCESS)
			{
				//Log::Info(TEXT("(%d) %s<%d> | strlen=%d\n"), i + 1, achValue, type, size);

				// we care for the Driver field
				if (_stricmp(achValue, "Driver") == 0)
				{
					retCode = RegEnumValue(hkey, i,
						achValue,
						&cchValue,
						NULL,
						&type,
						cbEnumValue,
						&size);
					if (_stricmp((const char*)cbEnumValue, DriverHashToMatch.c_str()) == 0)
					{
						return true;
					}

				}

			}
#if 0 // DEBUGGING
			else
			{
				Log::Error(TEXT("(%d) %s<%d> | strlen=%d\n"), i + 1, achValue, type, size);
				retCode = RegQueryInfoKey(
					hkey,                    // keyPath handle 
					achClass,                // buffer for class name 
					&cchClassName,           // size of class string 
					NULL,                    // reserved 
					&cSubKeys,               // number of subkeys 
					&cbMaxSubKey,            // longest subkey size 
					&cchMaxClass,            // longest class string 
					&cValues,                // number of values for this keyPath 
					&cchMaxValue,            // longest value name 
					&cbMaxValueData,         // longest value data 
					&cbSecurityDescriptor,   // security descriptor 
					&ftLastWriteTime);       // last write time 
				int a = 5;
			}
#endif // DEBUGGING
		}
	}

	return false;
}
static void FindAndDecodeEDID(const std::string& MonitorCodeRegistryPath, const std::string& DriverHashToMatch, std::string& outDisplayName)
{
	HKEY hkey;
	LSTATUS status = ERROR_SUCCESS;
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(MonitorCodeRegistryPath.c_str()), 0, KEY_READ, &hkey);

	if (status == ERROR_SUCCESS)
	{
		std::vector<std::string> SubKeys_MonitorCode = GetSubKeys(hkey);
		for (const std::string& SubKey_MonitorCode : SubKeys_MonitorCode)
		{
			//Log::Info("->%s", SubKey_MonitorCode.c_str());

			// Now we have the 1st level subkeys to the monitor code, which lists driver-hashed UIDs,
			// we can start looking for the DriverHashToMatch below 1 more level of subkeys.

			const std::string DriverHashPath = MonitorCodeRegistryPath + SubKey_MonitorCode + "\\";
			const bool bDriverHashMatches = DoesDisplayDriverHashMatch(DriverHashPath, DriverHashToMatch);
			if (bDriverHashMatches)
			{
				const std::string MonitorCodeSubKeyPath = MonitorCodeRegistryPath + SubKey_MonitorCode + "\\Device Parameters\\";
				HKEY hSubKey;
				status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(MonitorCodeSubKeyPath.c_str()), 0, KEY_READ, &hSubKey);
				if (status == ERROR_SUCCESS)
				{
					// now the hSubKey contains the EDID data
					TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
					DWORD    cchClassName = MAX_PATH;  // size of class string 
					DWORD    cSubKeys = 0;               // number of subkeys 
					DWORD    cbMaxSubKey;              // longest subkey size 
					DWORD    cchMaxClass;              // longest class string 
					DWORD    cValues;              // number of values for keyPath 
					DWORD    cchMaxValue;          // longest value name 
					DWORD    cbMaxValueData;       // longest value data 
					DWORD    cbSecurityDescriptor; // size of security descriptor 
					FILETIME ftLastWriteTime;      // last write time 

					DWORD i, retCode;

					TCHAR  achValue[MAX_VALUE_NAME];
					DWORD cchValue = MAX_VALUE_NAME;

					// Get the class name and the value count. 
					retCode = RegQueryInfoKey(
						hSubKey,                    // keyPath handle 
						achClass,                // buffer for class name 
						&cchClassName,           // size of class string 
						NULL,                    // reserved 
						&cSubKeys,               // number of subkeys 
						&cbMaxSubKey,            // longest subkey size 
						&cchMaxClass,            // longest class string 
						&cValues,                // number of values for this keyPath 
						&cchMaxValue,            // longest value name 
						&cbMaxValueData,         // longest value data 
						&cbSecurityDescriptor,   // security descriptor 
						&ftLastWriteTime);       // last write time 

					// Enumerate the EDID value.
					if (cValues)
					{
						for (i = 0, retCode = ERROR_SUCCESS; i < cValues; ++i)
						{
							cchValue = MAX_VALUE_NAME;
							achValue[0] = '\0';
							
							DWORD size;

							void* pData = malloc((cbMaxValueData+1) * sizeof(WCHAR));
							retCode = RegEnumValue(hSubKey, i,
								achValue,
								&cchValue,
								NULL,
								NULL,
								(LPBYTE)pData,
								&size);
							// pData contains EDID;

							if (retCode == ERROR_SUCCESS)
							{
								//Log::Info(TEXT("(%d) %s\n"), i + 1, achValue);

								// https://en.wikipedia.org/wiki/Extended_Display_Identification_Data
								const unsigned char* EDID = (const unsigned char*)pData;
								// Bytes[54-125] : 18-byte descriptors: Detailed timing (in decreasing preference order), display descriptors, etc.
								//   Bytes[54-71]   : Desc1
								//   Bytes[72-89]   : Desc2
								//   Bytes[90-107]  : Desc3
								//   Bytes[108-125] : Desc4
								// We need to locate the display descriptor that has the monitor name in ascii text
								const unsigned char* Descs[4] = { &EDID[54], &EDID[72], &EDID[90], &EDID[108] };
								for (int i = 0; i < 4; ++i)
								{
									const unsigned char* Desc = Descs[i];
									
									// https://en.wikipedia.org/wiki/Extended_Display_Identification_Data#Display_Descriptors
									// Display Descriptor Byte Layout
									// [0-1] : 0
									// [2]   : Reserved
									// [3]   : Desc Type with values FA-FF
									//         FF : Display Serial Number (ASCII Text)
									//         FE : Unspecified Text (ASCII Text)
									//         FC : Display name (ASCII Text) <---------- WANT THIS
									// [4]   : Reserved, except for Display Range Limits descriptor
									// [5-17]: Data (ASCII Text)
									// 
									const bool bIsDisplayDesc = 
										   (Desc[0] == Desc[1] && Desc[1] == 0)  // ensure [0-1]
										&& (Desc[3] >= 0xFA && Desc[3] <= 0xFF); // ensure [3]
									const bool bDescContainsDisplayName = bIsDisplayDesc && Desc[3] == 0xFC;

									if (bDescContainsDisplayName)
									{
										const unsigned char* DisplayName = &Desc[5];
										outDisplayName = std::string((const char*)DisplayName);
									}
								}
							}

							if (pData) free(pData);
						}
					}

				}
				else
				{
#if VERBOSE_LOGGING
					Log::Error("Couldn't open subkey for monitor code: %s", MonitorCodeSubKeyPath.c_str());
#endif
				}
			}
		}
		
	}
	else
	{
#if VERBOSE_LOGGING
		Log::Error("Couldn't open registry key: %s", MonitorCodeRegistryPath.c_str());
#endif
	}
	RegCloseKey(hkey);
}

// To detect HDR support, we will need to check the color space in the primary DXGI output associated with the app at
// this point in time (using window/display intersection). 
static inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
{
	// Compute the overlay area of two rectangles, A and B.
	// (ax1, ay1) = left-top coordinates of A; (ax2, ay2) = right-bottom coordinates of A
	// (bx1, by1) = left-top coordinates of B; (bx2, by2) = right-bottom coordinates of B
	return std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
}

std::vector<FMonitorInfo> GetDisplayInfo()
{
	std::vector<FMonitorInfo> monitors;

	auto fnFindBestMode = [](const std::vector<FMonitorInfo::FMode>& modes)
	{
		FMonitorInfo::FMode bestMode = {};

		// find highest supported refresh rate
		for (const FMonitorInfo::FMode& mode : modes)
			if (mode.RefreshRate > bestMode.RefreshRate) 
				bestMode.RefreshRate = mode.RefreshRate;

		// find the highest resolution supported by the highest refresh rate
		for (const FMonitorInfo::FMode& mode : modes)
		{
			if (mode.RefreshRate != bestMode.RefreshRate)
				continue;
			if (bestMode.Resolution.Width <= mode.Resolution.Width && bestMode.Resolution.Height <= mode.Resolution.Height)
				bestMode.Resolution = mode.Resolution;
		}

		// this isn't very good. the 4K@60Hz monitor can return a 75Hz mode that isn't 4K.
		return bestMode;
	};

	//
	// Get supported modes (refresh rate + resolution)
	//
	// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdisplaydevicesa
	int iDevNum = 0;
	DISPLAY_DEVICE device_interface = {};
	device_interface.cb = sizeof(DISPLAY_DEVICE);
	while (EnumDisplayDevices(NULL, iDevNum, &device_interface, EDD_GET_DEVICE_INTERFACE_NAME))
	{
		// we're only interested in attached displays
		if (!(device_interface.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) { ++iDevNum; continue; }

		DISPLAY_DEVICEA device = {};
		device.cb = sizeof(DISPLAY_DEVICEA);
		EnumDisplayDevices(device_interface.DeviceName, 0, &device, 0);

		FMonitorInfo i = {};

		int iModeNum = 0;
		DEVMODEA mode;
		FMonitorInfo::FMode highestMode = { };
		highestMode.RefreshRate = 0;
		highestMode.Resolution = { 0, 0 };
		while (EnumDisplaySettings(device_interface.DeviceName, iModeNum, &mode))
		{
			FMonitorInfo::FMode fmode;
			fmode.RefreshRate = mode.dmDisplayFrequency;
			fmode.Resolution.Width = mode.dmPelsWidth;
			fmode.Resolution.Height = mode.dmPelsHeight;
			i.SupportedModes.push_back(fmode);
			++iModeNum;
		}

		if constexpr (DISPLAY_DEVICE_NAME__READ_REGISTRY)
		{
			constexpr char* DISPLAY_NAME_ENUMS_REGISTRY_PATH_FROM_HKEY_LOCAL_MACHINE = "SYSTEM\\CurrentControlSet\\Enum\\DISPLAY";
			// Here in the registry root, we'll see part of device.DisplayID as folders, e.g.
			// DisplayID is generated per driver version, and look like: MONITOR\\ACR0414\\<driver hash>
			// Hence, we'll see monitor identifiers as folders like:
			// - ACR0414 - Acer ...
			// - DELA0DC - Dell ...
			// - DELA0EA - Dell ...
			std::vector<std::string> tokens = StrUtil::split(device.DeviceID, '\\');
			assert(tokens.size() >= 4);
			const std::string& MONITOR = tokens[0];           // "MONIOTOR"
			const std::string& MonitorCode = tokens[1];       // "ACR0414"
			const std::string& DriverHash = tokens[2];        // <driver hash:base>
			const std::string& DriverVersionHash = tokens[3]; // <driver hash:version>
			const std::string DisplayDriverHashCombinationToMatch = DriverHash + "\\" + DriverVersionHash; // essentially recreate the <driver hash>

			const std::string RegistryPath_MonitorCode = DISPLAY_NAME_ENUMS_REGISTRY_PATH_FROM_HKEY_LOCAL_MACHINE + std::string("\\") + MonitorCode + std::string("\\");
			FindAndDecodeEDID(RegistryPath_MonitorCode, DisplayDriverHashCombinationToMatch, i.DeviceName);

			// Finally, EDID string might contain newlines and spaces after the string ends so process it here
			i.DeviceName = StrUtil::trim(i.DeviceName);
		}
		else if constexpr (DISPLAY_DEVICE_NAME__USE_DISPLAY_INFO_API)
		{
			// TODO: figure out how to use the UWP library
			//Windows::Devices::Display::DisplayMonitor;
		}
		else
		{
			i.DeviceName = device.DeviceName;
		}

		i.DeviceID = device.DeviceID;

		i.HighestMode = fnFindBestMode(i.SupportedModes);

		monitors.push_back(i);
		++iDevNum;
	}


	//
	// Get Native Resolution, rotation degrees, etc.
	//
	const bool bEnumerateSoftwareAdapters = false;

	HRESULT hr = {};

	IDXGIAdapter1* pAdapter = nullptr; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
	int iAdapter = 0;                  // we'll start looking for DX12  compatible graphics devices starting at index 0
	// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi
	// https://stackoverflow.com/questions/42354369/idxgifactory-versions
	// Chuck Walbourn: For DIrect3D 12, you can assume CreateDXGIFactory2 and IDXGIFactory4 or later is supported. 
	IDXGIFactory6* pDxgiFactory; // DXGIFactory6 supports preferences when querying devices: DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
	UINT DXGIFlags = 0;
	hr = CreateDXGIFactory2(DXGIFlags, IID_PPV_ARGS(&pDxgiFactory));

	// Find GPU with highest perf: https://stackoverflow.com/questions/49702059/dxgi-integred-pAdapter
	// https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_6/nf-dxgi1_6-idxgifactory6-enumadapterbygpupreference
	while (pDxgiFactory->EnumAdapterByGpuPreference(iAdapter, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		// skip the software adapter
		const bool bSoftwareAdapter = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
		if ((bEnumerateSoftwareAdapters && !bSoftwareAdapter) // We want software adapters, but we got a hardware adapter
			|| (!bEnumerateSoftwareAdapters && bSoftwareAdapter) // We want hardware adapters, but we got a software adapter
			)
		{
			++iAdapter;
			pAdapter->Release();
			continue;
		}

		// Enum monitors connected to the adapter
		UINT iMonitor = 0;
		IDXGIOutput* pOutput = nullptr;
		while (SUCCEEDED(pAdapter->EnumOutputs(iMonitor, &pOutput)))
		{
			assert(iMonitor < monitors.size());

			DXGI_OUTPUT_DESC1 desc = {};

			// Interface for checking HDR capability
			IDXGIOutput6* pOut = nullptr;
			pOutput->QueryInterface(IID_PPV_ARGS(&pOut)); 
			pOut->GetDesc1(&desc);

			FMonitorInfo& i = monitors[iMonitor];
			i.LogicalDeviceName = StrUtil::UnicodeToASCII(desc.DeviceName);
			switch (desc.Rotation)
			{
			case DXGI_MODE_ROTATION_IDENTITY   : i.RotationDegrees = 0; break;
			case DXGI_MODE_ROTATION_ROTATE90   : i.RotationDegrees = 90; break;
			case DXGI_MODE_ROTATION_ROTATE180  : i.RotationDegrees = 180; break;
			case DXGI_MODE_ROTATION_ROTATE270  : i.RotationDegrees = 270; break;
			case DXGI_MODE_ROTATION_UNSPECIFIED: i.RotationDegrees = 0; break;
			}

			i.NativeResolution = { desc.DesktopCoordinates.right - desc.DesktopCoordinates.left, desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top };
			i.bSupportsHDR = desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
			i.DisplayChromaticities = FDisplayChromaticities(desc);
			i.BrightnessValues = FDisplayBrightnessValues(desc);

			++iMonitor;
			pOut->Release();
			pOutput->Release();
		}


		pAdapter->Release();
		++iAdapter;
	}
	pDxgiFactory->Release();


	return monitors;
}

bool FMonitorInfo::CheckHDRSupport(HWND hwnd)
{
	auto fnThrowIfFailed = [](HRESULT hr)
	{
		if (FAILED(hr))
		{
			assert(false);// throw HrException(hr);
		}
	};

	RECT windowRect = {};
	GetWindowRect(hwnd, &windowRect);

	using namespace Microsoft::WRL;
	ComPtr<IDXGIFactory6> m_dxgiFactory;
	fnThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory)));

	// From D3D12HDR: 
	// First, the method must determine the app's current display. 
	// We don't recommend using IDXGISwapChain::GetContainingOutput method to do that because of two reasons:
	//    1. Swap chains created with CreateSwapChainForComposition do not support this method.
	//    2. Swap chains will return a stale dxgi output once DXGIFactory::IsCurrent() is false. In addition, 
	//       we don't recommend re-creating swapchain to resolve the stale dxgi output because it will cause a short 
	//       period of black screen.
	// Instead, we suggest enumerating through the bounds of all dxgi outputs and determine which one has the greatest 
	// intersection with the app window bounds. Then, use the DXGI output found in previous step to determine if the 
	// app is on a HDR capable display. 

	// Retrieve the current default adapter.
	ComPtr<IDXGIAdapter1> dxgiAdapter;
	fnThrowIfFailed(m_dxgiFactory->EnumAdapters1(0, &dxgiAdapter));

	// Iterate through the DXGI outputs associated with the DXGI adapter,
	// and find the output whose bounds have the greatest overlap with the
	// app window (i.e. the output for which the intersection area is the
	// greatest).

	UINT i = 0;
	ComPtr<IDXGIOutput> currentOutput;
	ComPtr<IDXGIOutput> bestOutput;
	float bestIntersectArea = -1;

	while (dxgiAdapter->EnumOutputs(i, &currentOutput) != DXGI_ERROR_NOT_FOUND)
	{
		// Get the retangle bounds of the app window
		int ax1 = windowRect.left;
		int ay1 = windowRect.top;
		int ax2 = windowRect.right;
		int ay2 = windowRect.bottom;

		// Get the rectangle bounds of current output
		DXGI_OUTPUT_DESC desc;
		fnThrowIfFailed(currentOutput->GetDesc(&desc));
		RECT r = desc.DesktopCoordinates;
		int bx1 = r.left;
		int by1 = r.top;
		int bx2 = r.right;
		int by2 = r.bottom;

		// Compute the intersection
		int intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
		if (intersectArea > bestIntersectArea)
		{
			bestOutput = currentOutput;
			bestIntersectArea = static_cast<float>(intersectArea);
		}

		i++;
	}

	// Having determined the output (display) upon which the app is primarily being 
	// rendered, retrieve the HDR capabilities of that display by checking the color space.
	ComPtr<IDXGIOutput6> output6;
	fnThrowIfFailed(bestOutput.As(&output6));

	DXGI_OUTPUT_DESC1 desc1;
	fnThrowIfFailed(output6->GetDesc1(&desc1));

	return (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
}


// ----------------------------------------------------------------------------------------------------------------------------------------------
//
// GPU
//
// ----------------------------------------------------------------------------------------------------------------------------------------------
std::vector<FGPUInfo> GetGPUInfo(/*TODO: feature level*/)
{
	const bool bEnumerateSoftwareAdapters = false;

	std::vector< FGPUInfo > GPUs;
	HRESULT hr = {};

	IDXGIAdapter1* pAdapter = nullptr; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
	int iAdapter = 0;                  // we'll start looking for DX12  compatible graphics devices starting at index 0
	bool bAdapterFound = false;        // set this to true when a good one was found

	// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi
	// https://stackoverflow.com/questions/42354369/idxgifactory-versions
	// Chuck Walbourn: For DIrect3D 12, you can assume CreateDXGIFactory2 and IDXGIFactory4 or later is supported. 
	IDXGIFactory6* pDxgiFactory; // DXGIFactory6 supports preferences when querying devices: DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
	UINT DXGIFlags = 0;
	hr = CreateDXGIFactory2(DXGIFlags, IID_PPV_ARGS(&pDxgiFactory));

	auto fnAddAdapter = [&bAdapterFound, &GPUs](IDXGIAdapter1*& pAdapter, const DXGI_ADAPTER_DESC1& desc, D3D_FEATURE_LEVEL FEATURE_LEVEL)
	{
		bAdapterFound = true;

		FGPUInfo GPUInfo = {};
		GPUInfo.DedicatedGPUMemory = desc.DedicatedVideoMemory;
		GPUInfo.DeviceID = desc.DeviceId;
		GPUInfo.DeviceName = StrUtil::UnicodeToASCII<1024>(desc.Description);
		GPUInfo.VendorID = desc.VendorId;
		///GPUInfo.MaxSupportedFeatureLevel = FEATURE_LEVEL;
		pAdapter->QueryInterface(IID_PPV_ARGS(&GPUInfo.pAdapter));
		GPUs.push_back(GPUInfo);
	};

	// Find GPU with highest perf: https://stackoverflow.com/questions/49702059/dxgi-integred-pAdapter
	// https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_6/nf-dxgi1_6-idxgifactory6-enumadapterbygpupreference
	while (pDxgiFactory->EnumAdapterByGpuPreference(iAdapter, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		const bool bSoftwareAdapter = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
		if ((bEnumerateSoftwareAdapters && !bSoftwareAdapter) // We want software adapters, but we got a hardware adapter
			|| (!bEnumerateSoftwareAdapters && bSoftwareAdapter) // We want hardware adapters, but we got a software adapter
			)
		{
			++iAdapter;
			pAdapter->Release();
			continue;
		}

		hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			fnAddAdapter(pAdapter, desc, D3D_FEATURE_LEVEL_12_1);
		}
		else
		{
			const std::string AdapterDesc = StrUtil::UnicodeToASCII(desc.Description);
			//Log::Warning("Device::Create(): D3D12CreateDevice() with Feature Level 12_1 failed with adapter=%s, retrying with Feature Level 12_0", AdapterDesc.c_str());
			hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr);
			if (SUCCEEDED(hr))
			{
				fnAddAdapter(pAdapter, desc, D3D_FEATURE_LEVEL_12_0);
			}
			else
			{
				//Log::Error("Device::Create(): D3D12CreateDevice() with Feature Level 12_0 failed ith adapter=%s", AdapterDesc.c_str());
			}
		}

		pAdapter->Release();
		++iAdapter;
	}
	pDxgiFactory->Release();
	assert(bAdapterFound);

	return GPUs;
}

// ----------------------------------------------------------------------------------------------------------------------------------------------
//
// CPU
//
// ----------------------------------------------------------------------------------------------------------------------------------------------
FCPUInfo GetCPUInfo()
{
	FCPUInfo i;

	// CPUID
	// https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=vs-2019
	{
		int nIds_;
		int nExIds_;
		std::vector<std::array<int, 4>> data_;
		std::vector<std::array<int, 4>> extdata_;

		std::array<int, 4> cpui;

		// Calling __cpuid with 0x0 as the function_id argument
		// gets the number of the highest valid function ID.
		__cpuid(cpui.data(), 0);
		nIds_ = cpui[0];
		for (int i = 0; i <= nIds_; ++i)
		{
			__cpuidex(cpui.data(), i, 0);
			data_.push_back(cpui);
		}

		// Capture vendor string
		char vendor[0x20];
		memset(vendor, 0, sizeof(vendor));
		*reinterpret_cast<int*>(vendor)     = data_[0][1];
		*reinterpret_cast<int*>(vendor + 4) = data_[0][3];
		*reinterpret_cast<int*>(vendor + 8) = data_[0][2];
		i.ManufacturerName  = vendor;

		// Read Model & Family
		i.ModelID  = (data_[1][0] >> 4) & 0xF; // 4-bit value in bits [4-7]
		i.FamilyID = (data_[1][0] >> 8) & 0xF; // 4-bit value in bits [8-11]
		int extModelID   = (data_[1][0] >> 16) & 0xF;  // 4-bit value in bits [16-19]
		int extFamiltyID = (data_[1][0] >> 20) & 0xFF; // 8-bit value in bits [20-27]

		// https://en.wikipedia.org/wiki/CPUID#EAX=1:_Processor_Info_and_Feature_Bits
		// the model is equal to the sum of the Extended Model ID field shifted left by 4 bits 
		// and the Model field. Otherwise, the model is equal to the value of the Model field.
		if (i.FamilyID == 6 || i.FamilyID == 15) { i.ModelID = (extModelID << 4) + i.ModelID; }
		// If the Family ID field is equal to 15, the family is equal to the sum of the Extended Family ID 
		// and the Family ID fields. Otherwise, the family is equal to value of the Family ID field.
		if (i.FamilyID == 15) { i.FamilyID = extFamiltyID + i.FamilyID; }


		// Calling __cpuid with 0x80000000 as the function_id argument
		// gets the number of the highest valid extended ID.
		__cpuid(cpui.data(), 0x80000000);
		nExIds_ = cpui[0];

		// Capture CPU brand
		char brand[0x40];
		memset(brand, 0, sizeof(brand));
		for (int i = 0x80000000; i <= nExIds_; ++i)
		{
			__cpuidex(cpui.data(), i, 0);
			extdata_.push_back(cpui);
		}
		// Interpret CPU brand string if reported
		if (nExIds_ >= 0x80000004)
		{
			constexpr size_t stride = sizeof(cpui);
			memcpy(brand + 0*stride, extdata_[2].data(), stride);
			memcpy(brand + 1*stride, extdata_[3].data(), stride);
			memcpy(brand + 2*stride, extdata_[4].data(), stride);
			i.DeviceName  = brand;
			
			// remove the trailing space in device name: "CPU-NAME       " -> "CPU-NAME"
			i.DeviceName.erase(
				 std::find_if(i.DeviceName.rbegin(), i.DeviceName.rend(), [](const char& c) { return c != ' '; }).base()
				, i.DeviceName.end()
			);
		}
	}

	// PROCESSOR INFO : NUMA, Cores/Threads
	// https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformationex
	// https://github.com/GPUOpen-LibrariesAndSDKs/cpu-core-counts/blob/master/windows/ThreadCount-Win7.cpp
	{
		// allocate buffer for processor information strings
		char* buffer = NULL; DWORD len = 0;
		if (FALSE == GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer, &len))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				buffer = (char*)malloc(len);
			}
		}

		// Get detailed Processor info
		int cores = 0;
		int logical = 0;
		int numa = 0;
		if (GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer, &len))
		{
			DWORD offset = 0;
			char* ptr = buffer;
			while (ptr < buffer + len)
			{
				PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

				switch (pi->Relationship)
				{

				// CORE
				case RelationProcessorCore:
				{
					++cores;
					for (size_t g = 0; g < pi->Processor.GroupCount; ++g)
					{
						logical += CountSetBits(pi->Processor.GroupMask[g].Mask);
					}
				} break;

				// CACHE
				case RelationCache:
				{
					FCacheInfo ci = {};
					ci.Level = pi->Cache.Level;
					ci.LineSize = pi->Cache.LineSize;
					ci.CacheSize = pi->Cache.CacheSize;

					switch (pi->Cache.Type)
					{
					case CacheUnified: ci.Type = FCacheInfo::ECacheType::UNIFIED; break;
					case CacheInstruction: ci.Type = FCacheInfo::ECacheType::INSTRUCTION; break;
					case CacheData: ci.Type = FCacheInfo::ECacheType::DATA; break;
					case CacheTrace: ci.Type = FCacheInfo::ECacheType::TRACE; break;
					}

					i.CacheInfo.push_back(ci);
				} break;

				// NUMA
				case RelationNumaNode:
				{
					++numa;
				} break;

				}


				ptr += pi->Size;
			}
		}

		i.NumCores = cores;
		i.NumThreads = logical;
		i.NumNUMANodes = numa;
		free(buffer);
	}


	// SYSINFO
	// System Info: https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
	{

	}


	return i;
}

bool FCPUInfo::IsAMD() const
{
	return DeviceName == "AuthenticAMD";
}
bool FCPUInfo::IsIntel() const
{
	return DeviceName == "GenuineIntel";
}

static std::vector<FCacheInfo> GetCacheInfoOfTypeAtLevel(const std::vector<FCacheInfo>& cacheInfo, short level, FCacheInfo::ECacheType type)
{
	std::vector<FCacheInfo> filteredInfo;
	for (const FCacheInfo& i : cacheInfo)
	{
		if (i.Level == level && i.Type == type)
			filteredInfo.push_back(i);
	}
	return filteredInfo;
}
unsigned long FCPUInfo::GetDCacheSize(short Level, int* pOutNumCaches/* = nullptr*/) const
{
	const FCacheInfo::ECacheType CacheType = Level == 1 ? FCacheInfo::ECacheType::DATA : FCacheInfo::ECacheType::UNIFIED;
	const std::vector<FCacheInfo> info = GetCacheInfoOfTypeAtLevel(this->CacheInfo, Level, CacheType);
	if(info.empty())
		return 0;
	if (pOutNumCaches)
	{
		*pOutNumCaches = static_cast<int>(info.size());
	}
	return info[0].CacheSize;
}
unsigned long FCPUInfo::GetDCacheLineSize(short Level) const
{
	const FCacheInfo::ECacheType CacheType = Level == 1 ? FCacheInfo::ECacheType::DATA : FCacheInfo::ECacheType::UNIFIED;
	const std::vector<FCacheInfo> info = GetCacheInfoOfTypeAtLevel(this->CacheInfo, Level, CacheType);
	if (info.empty())
		return 0;
	return info[0].LineSize;
}
unsigned long FCPUInfo::GetICacheSize(int* pOutNumCaches /*= nullptr*/) const
{
	const std::vector<FCacheInfo> info = GetCacheInfoOfTypeAtLevel(this->CacheInfo, 1, FCacheInfo::ECacheType::INSTRUCTION);
	if (info.empty())
		return 0;
	if (pOutNumCaches)
	{
		*pOutNumCaches = static_cast<int>(info.size());
	}
	return info[0].CacheSize;
}



// ----------------------------------------------------------------------------------------------------------------------------------------------
//
// SYSTEM
//
// ----------------------------------------------------------------------------------------------------------------------------------------------
FSystemInfo GetSystemInfo()
{
	FSystemInfo i = {};

	i.CPU = GetCPUInfo();
	i.GPUs = GetGPUInfo();
	i.RAM = GetRAMInfo();
	i.Monitors = GetDisplayInfo();

	return i;
}

template<class... Args>
static void INFO(std::string& s, const char* format, Args&&... args)
{
	char msg[2048]; 
	sprintf_s(msg, format, args...); 
	s += msg;
	s += "\n";
}

inline static std::string FORMAT_BYTE(unsigned long long bytes) { return StrUtil::FormatByte(bytes); }

std::string PrintSystemInfo(const FSystemInfo& i, const bool bDetailed /*= false*/)
{
	int ii = 0; // index counter
	std::string output;
	std::string& o = output; // shorthand
	
	// test------------------------------------
	const bool bCPU_AMD	  = i.CPU.IsAMD();	 
	const bool bCPU_Intel = i.CPU.IsIntel(); 
	const bool bGPU_AMD   = i.GPUs[0].IsAMD();   
	const bool bGPU_Intel = i.GPUs[0].IsIntel(); 
	const bool bGPU_Nvidia= i.GPUs[0].IsNVidia();
	// test------------------------------------

	int L3NumCaches = 0 ;  int L2NumCaches = 0;
	int L1NumDCaches = 0; int L1NumICaches = 0;
	unsigned long long L3CacheSize  = i.CPU.GetDCacheSize(3, &L3NumCaches);
	unsigned long long L2CacheSize  = i.CPU.GetDCacheSize(2, &L2NumCaches);;
	unsigned long long L1DCacheSize = i.CPU.GetDCacheSize(1, &L1NumDCaches);;
	unsigned long long L1ICacheSize = i.CPU.GetICacheSize(&L1NumICaches);;

	INFO(o, "------------------- SYSTEM INFO -------------------");
	
	INFO(o, "CPU");
	INFO(o, "\tManufacturer        : %s", i.CPU.ManufacturerName.c_str());
	INFO(o, "\tModel               : %s", i.CPU.DeviceName.c_str());
	INFO(o, "\tCores/Threads       : %d/%d", i.CPU.NumCores, i.CPU.NumThreads);
	if (bDetailed)
	{
	INFO(o, "\tNUMA Nodes          : %d", i.CPU.NumNUMANodes);
	INFO(o, "\tModelID             : 0x%x", i.CPU.ModelID );
	INFO(o, "\tFamilyID            : 0x%x", i.CPU.FamilyID);
	}
	INFO(o, "\tL3 Cache            : %d x %s, %s Total L3", L3NumCaches, FORMAT_BYTE(L3CacheSize).c_str(), FORMAT_BYTE(L3CacheSize * L3NumCaches).c_str());
	INFO(o, "\tL2 Cache            : %d x %s, %s Total L2", L2NumCaches, FORMAT_BYTE(L2CacheSize).c_str(), FORMAT_BYTE(L2CacheSize * L2NumCaches).c_str());
	INFO(o, "\tL1 D-Cache          : %d x %s", L1NumDCaches, FORMAT_BYTE(L1DCacheSize).c_str());
	INFO(o, "\tL1 I-Cache          : %d x %s", L1NumICaches, FORMAT_BYTE(L1ICacheSize).c_str());
	INFO(o, "\tCache Lines         : %s", FORMAT_BYTE(i.CPU.GetDCacheLineSize(1)).c_str());

	INFO(o, "");

	INFO(o, "RAM");
	INFO(o, "\tPhys. Total Memory  : %s", FORMAT_BYTE(i.RAM.TotalPhysicalMemory).c_str());
	INFO(o, "\tPhys. Free  Memory  : %s", FORMAT_BYTE(i.RAM.FreePhysicalMemory).c_str());
	INFO(o, "\tUsage %%             : %u%%", i.RAM.UsagePercentage);
	INFO(o, "\tVirt. Total Memory  : %s", FORMAT_BYTE(i.RAM.TotalVirtualMemory ).c_str());
	INFO(o, "\tVirt. Free  Memory  : %s", FORMAT_BYTE(i.RAM.FreeVirtualMemory  ).c_str());


	ii = 0; 
	for( const VQSystemInfo::FGPUInfo& GPU : i.GPUs)
	{
	INFO(o, "");
	INFO(o, "GPU %d", ii++);
	INFO(o, "\tDeviceName          : %s", GPU.DeviceName.c_str());
	INFO(o, "\tMemory              : %s", FORMAT_BYTE(GPU.DedicatedGPUMemory).c_str());
	INFO(o, "\tDeviceID            : 0x%x", GPU.DeviceID);
	INFO(o, "\tVendorID            : 0x%x", GPU.VendorID);
	} // for(GPU)

	ii = 0;
	for( const VQSystemInfo::FMonitorInfo& m : i.Monitors)
	{
	INFO(o, "");
	INFO(o, "Display %d", ii++);
	INFO(o, "\tDeviceName         : %s", m.DeviceName.c_str());
	INFO(o, "\tLogicalDeviceName  : %s", m.LogicalDeviceName.c_str());
	if(bDetailed)
	{
	INFO(o, "\tDeviceID           : %s", m.DeviceID.c_str());
	if(m.RotationDegrees!=0) INFO(o, "\tRotation           : %d Degrees", m.RotationDegrees);
	} // bDetailed
	INFO(o, "\tNative Resolution  : %dx%d", m.NativeResolution.Width, m.NativeResolution.Height);
	INFO(o, "\tHDR Support        : %s", m.bSupportsHDR ? "Yes" : "No");
	if(m.bSupportsHDR)
	{
	INFO(o, "\tChromaticities");
	INFO(o, "\t  RedPrimary_xy    : %.2f, %.2f", m.DisplayChromaticities.RedPrimary_xy[0], m.DisplayChromaticities.RedPrimary_xy[1]);
	INFO(o, "\t  GreenPrimary_xy  : %.2f, %.2f", m.DisplayChromaticities.GreenPrimary_xy[0], m.DisplayChromaticities.GreenPrimary_xy[1]);
	INFO(o, "\t  BluePrimary_xy   : %.2f, %.2f", m.DisplayChromaticities.BluePrimary_xy[0], m.DisplayChromaticities.BluePrimary_xy[1]);
	INFO(o, "\t  WhitePoint_xy    : %.2f, %.2f", m.DisplayChromaticities.WhitePoint_xy[0], m.DisplayChromaticities.WhitePoint_xy[1]);
	} // bSupportsHDR
	INFO(o, "\tBrightness");
	INFO(o, "\t  MinLuminance     : %.3f Nits", m.BrightnessValues.MinLuminance);
	INFO(o, "\t  MaxLuminance     : %d Nits", (int)m.BrightnessValues.MaxLuminance);
	INFO(o, "\t  MaxFullFrameLum  : %d Nits", (int)m.BrightnessValues.MaxFullFrameLuminance);
	} // for(Monitor)

	INFO(o, "------------------- SYSTEM INFO -------------------");
	return output;
}

} // namespace VQSystemInfo