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

#ifndef VQUTILS_SYSTEMINFO_INCLUDE_D3D12
#define VQUTILS_SYSTEMINFO_INCLUDE_D3D12 0
#endif

#if VQUTILS_SYSTEMINFO_INCLUDE_D3D12 
#include <d3d12.h>
#endif

#include <string>
#include <vector>

// fwd decls
struct IDXGIOutput;
struct IDXGIAdapter1;


namespace VQSystemInfo
{
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
	

	struct FGPUInfo
	{
		std::string    ManufacturerName;
		std::string    DeviceName;
		unsigned       DeviceID;
		unsigned       VendorID;
		size_t         DedicatedGPUMemory;
		IDXGIAdapter1* pAdapter;
#if VQUTILS_SYSTEMINFO_INCLUDE_D3D12
		D3D_FEATURE_LEVEL MaxSupportedFeatureLevel; // todo: bool d3d12_0 ?
#endif
	};

	struct FMonitorInfo
	{
		struct FResolution { int Width, Height; };
		struct FMode       { FResolution Resolution;  unsigned RefreshRate; };

		//std::string        ManufacturerName; // TODO: use WMI or remove field
		std::string        DeviceName;
		FResolution        NativeResolution;
		bool               bSupportsHDR;
		//IDXGIOutput*       pDXGIOut; // TODO: memory management? take in as param?
		unsigned           RotationDegrees;
		std::vector<FMode> SupportedModes;
		FMode              HighestMode;
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