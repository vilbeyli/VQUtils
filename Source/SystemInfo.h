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
#include <vector>
#include <array>

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

// fwd decls
struct IDXGIOutput;
struct IDXGIAdapter1;

namespace VQSystemInfo
{
	//
	// CPU
	// 
	struct FCacheInfo
	{
		enum ECacheType { INSTRUCTION, DATA, UNIFIED, TRACE };
		unsigned short Level;
		unsigned short LineSize;
		unsigned long CacheSize;
		ECacheType Type;
	};
	struct FCPUInfo
	{
		std::string ManufacturerName;
		std::string DeviceName;
		unsigned    NumCores;
		unsigned    NumThreads;
		unsigned    NumNUMANodes;
		std::vector<FCacheInfo> CacheInfo;
		unsigned    ModelID;
		unsigned    FamilyID;

		/* TODO */ bool           IsAMD() const;
		/* TODO */ bool           IsIntel() const;
		/* TODO */ unsigned short GetDCacheSize(short Level) const;
		/* TODO */ unsigned short GetDCacheLineSize(short Level) const;
		/* TODO */ unsigned short GetICacheSize() const;
	};
	
	//
	// GPU
	//
	struct FGPUInfo
	{
		std::string    ManufacturerName;
		std::string    DeviceName;
		unsigned       DeviceID;
		unsigned       VendorID;
		size_t         DedicatedGPUMemory;
		IDXGIAdapter1* pAdapter;
		D3D_FEATURE_LEVEL MaxSupportedFeatureLevel; // todo: bool d3d12_0 ?
	};


	//
	// MONITOR
	//
	struct FDisplayChromaticities // https://en.wikipedia.org/wiki/Chromaticity
	{
		FDisplayChromaticities(float rx, float ry, float gx, float gy, float bx, float by, float wx, float wy)
			: RedPrimary_xy  { rx, ry }
			, GreenPrimary_xy{ gx, gy }
			, BluePrimary_xy { bx, by }
			, WhitePoint_xy  { wx, wy }
		{}
		FDisplayChromaticities() = default;
		FDisplayChromaticities(const DXGI_OUTPUT_DESC1& d)
			: RedPrimary_xy  { d.RedPrimary[0]  , d.RedPrimary[1]   }
			, GreenPrimary_xy{ d.GreenPrimary[0], d.GreenPrimary[1] }
			, BluePrimary_xy { d.BluePrimary[0] , d.BluePrimary[1]  }
			, WhitePoint_xy  { d.WhitePoint[0]  , d.WhitePoint[1]   }
		{}

		// XY space coordinates of RGB primaries
		std::array<float, 2> RedPrimary_xy;
		std::array<float, 2> GreenPrimary_xy;
		std::array<float, 2> BluePrimary_xy;
		std::array<float, 2> WhitePoint_xy;
	};
	struct FDisplayBrightnessValues
	{
		FDisplayBrightnessValues() = default;
		FDisplayBrightnessValues(float MinLum, float MaxLum, float MaxFullFrameLum) 
			: MinLuminance(MinLum)
			, MaxLuminance(MaxLum)
			, MaxFullFrameLuminance(MaxFullFrameLum)
		{}
		FDisplayBrightnessValues(const DXGI_OUTPUT_DESC1& d) 
			: MinLuminance(d.MinLuminance)
			, MaxLuminance(d.MaxLuminance)
			, MaxFullFrameLuminance(d.MaxFullFrameLuminance)
		{}
		float MinLuminance;
		float MaxLuminance;
		float MaxFullFrameLuminance;
	};
	struct FMonitorInfo
	{
		struct FResolution { int Width, Height; };
		struct FMode       { FResolution Resolution;  unsigned RefreshRate; };

		//std::string              ManufacturerName; // TODO: use WMI or remove field
		std::string              DeviceName;
		std::string              DeviceID;
		FResolution              NativeResolution;
		bool                     bSupportsHDR;
		unsigned                 RotationDegrees;
		std::vector<FMode>       SupportedModes;
		FMode                    HighestMode;
		FDisplayChromaticities   DisplayChromaticities;
		FDisplayBrightnessValues BrightnessValues;

		static bool CheckHDRSupport(HWND hwnd);
	};

	struct FRAMInfo
	{
		unsigned UsagePercentage;
		unsigned long long TotalPhysicalMemory;
		unsigned long long FreePhysicalMemory;
		unsigned long long TotalPageFileSize;
		unsigned long long FreePageFileSize;
		unsigned long long TotalVirtualMemory;
		unsigned long long FreeVirtualMemory;
		unsigned long long ExtendedMemory;
	};

	struct FSystemInfo
	{
		FCPUInfo                  CPU; // vector? I need to test a multi-CPU system...
		std::vector<FGPUInfo>     GPUs;
		std::vector<FMonitorInfo> Monitors;
		FRAMInfo                  RAM;
	};

	// ===================================================================================================

	// Uses CPUID intrinsic instruction to acquire CPU information.
	// The CPUID instruction is relatively expensive, therefore don't 
	// call this function in a busy loop.
	FCPUInfo                  GetCPUInfo();
	std::vector<FGPUInfo>     GetGPUInfo();
	std::vector<FMonitorInfo> GetDisplayInfo();
	FRAMInfo                  GetRAMInfo();
	FSystemInfo               GetSystemInfo();

	// std::string GetFormattedSize(unsigned long long Bytes); // GetFormattedSize(1048576) -> "1MB"

} // namespace VQSystemInfo