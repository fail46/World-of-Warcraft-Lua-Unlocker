#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

const unsigned char NewBytes[] = {0x8B, 0xE5, 0x5D, 0xC3};
const unsigned char PatternBytes[] = {0x8B, 0x45, 0xDC, 0x8B, 0x4D, 0xF0, 0x5F, 0x5E, 0x5B};
const char PatternMask[] = "xxxxxxxxx";

pointer FindPattern (pointer StartAddress, unsigned int MaxLength, const unsigned char* Bytes, const char* Mask)
{
	unsigned int i = 0;
	while(i < MaxLength)
	{
		unsigned int o = 0;
		while(reinterpret_cast<unsigned char*>(StartAddress + i)[o] == Bytes[o] || Mask[o] == '*')
		{
			o++;
			if(Mask[o] == 0x0)
			{
				return StartAddress + i;
			}
		}

		i++;
	}

	return nullptr;
}

int __stdcall DllMain (void* Module, unsigned long Reason, void*)
{
	if(Reason != DLL_PROCESS_ATTACH)
	{
		return 0;
	}

	MODULEINFO WoWModuleInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &WoWModuleInfo, sizeof(MODULEINFO));
	pointer PatchAddress = FindPattern(WoWModuleInfo.lpBaseOfDll, WoWModuleInfo.SizeOfImage, PatternBytes, PatternMask);
	if(PatchAddress == nullptr || *reinterpret_cast<unsigned char*>(PatchAddress += 0x9) != 0x81)
	{
		MessageBox(FindWindow("GxWindowClass", "World of Warcraft"), "Unable to find bytes to patch.", "Failure", MB_OK);
		return 0;
	}

	unsigned long OldProtection = 0;
	VirtualProtect(PatchAddress, 0x4, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(PatchAddress, NewBytes, 0x4);
	VirtualProtect(PatchAddress, 0x4, OldProtection, &OldProtection);

	if(memcmp(NewBytes, PatchAddress, 0x4) != 0)
	{
		char* Message = new char[100];
		ZeroMemory(Message, 100);
		sprintf(Message, "Unable to patch bytes. Error: %d", GetLastError());
		MessageBox(FindWindow("GxWindowClass", "World of Warcraft"), Message, "Failure", MB_OK);
		delete[] Message;
	}
	else
	{
		MessageBox(FindWindow("GxWindowClass", "World of Warcraft"), "Lua unlocked.", "Success", MB_OK);
	}

	return 1;
}