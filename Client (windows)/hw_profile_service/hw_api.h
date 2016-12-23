#pragma once
#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
#include <cmath>

static PDH_HQUERY CPUQuery;
static PDH_HCOUNTER CPUTotal;
static PDH_FMT_COUNTERVALUE CounterValue;
static ULARGE_INTEGER Available, Total, Free;

DOUBLE GetCPUCycle(void)
{
	PdhCollectQueryData(CPUQuery);
	PdhGetFormattedCounterValue(CPUTotal, PDH_FMT_DOUBLE, NULL, &CounterValue);

	return CounterValue.doubleValue;
}

DOUBLE GetPhysicalMemory(void)
{
	static MEMORYSTATUSEX MemoryInformation;
	MemoryInformation.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&MemoryInformation);

	return (MemoryInformation.ullTotalPageFile - MemoryInformation.ullAvailPhys) / pow(10, sizeof(DWORDLONG));
}

DOUBLE GetHardDiskUsage(LPCSTR Path)
{
	GetDiskFreeSpaceExA(Path, &Available, &Total, &Free);
	DOUBLE MTotal = (INT)(Total.QuadPart >> 30);
	DOUBLE MFree = (INT)(Free.QuadPart >> 30);

	return ((MTotal - MFree) / MTotal) * 100;
}