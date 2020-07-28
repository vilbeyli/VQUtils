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

#include <d3d12.h>
#include <string>
#include <vector>
#include <dxgi1_6.h>
#include <array>

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
	struct FColorSpace // TODO: rename to DisplayChromaticity?
	{
		FColorSpace() = default;
		FColorSpace(const DXGI_OUTPUT_DESC1& d)
			: RedPrimaryXY  { d.RedPrimary[0]  , d.RedPrimary[1]   }
			, BluePrimaryXY { d.BluePrimary[0] , d.BluePrimary[1]  }
			, GreenPrimaryXY{ d.GreenPrimary[0], d.GreenPrimary[1] }
			, WhitePointXY  { d.WhitePoint[0]  , d.WhitePoint[1]   }
		{}

		// XY space coordinates of RGB primaries
		std::array<float, 2> RedPrimaryXY;
		std::array<float, 2> GreenPrimaryXY;
		std::array<float, 2> BluePrimaryXY;
		std::array<float, 2> WhitePointXY;
	};
	struct FDisplayBrightnessValues
	{
		FDisplayBrightnessValues() = default;
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
		FColorSpace              ColorSpace;
		FDisplayBrightnessValues BrightnessValues;
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