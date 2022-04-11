
#include "windows.h" 
#include "winnt.h" 
#include "stdio.h"


BOOL getHMODULE(char* fileName, HANDLE* hFile, HANDLE* hFileMapping, LPVOID* baseAddress) {
	printf("[GetHMODULE] : Opening %s\n", fileName);
	(*hFile) = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("[GetHMODULE]: CreateFile() failed\n");
		return(FALSE);
	}

	printf("[GetHMODULEj : Opening an unamed file mapping object\n");

	(*hFileMapping) = CreateFileMapping(*hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if ((*hFileMapping) == NULL)
	{
		CloseHandle(hFile);
		printf("[GetHMODULE] : CreateFileMapping() failed\n");
		return(FALSE);
	}

	printf("[GetHMDOULE] : Mapping a view of the file\n");
	(*baseAddress) = MapViewOfFile(*hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if ((*baseAddress) == NULL)
	{
		CloseHandle(*hFileMapping);
		CloseHandle(*hFile);
		printf("Couldn't map view of file with MapViewOfFile()\n");
		return(FALSE);
	}
	return(TRUE);
}

PIMAGE_SECTION_HEADER getCurrentSectionHeader(DWORD rva, PIMAGE_NT_HEADERS64 peHeader) {
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(peHeader);
	unsigned nSections;
	unsigned index;

	nSections = ((*peHeader).FileHeader).NumberOfSections;

	// locate the section header that contains the RVA(otherwise return NULL) 
	for (index = 0; index < nSections; index++, section++)
	{
		if ((rva >= (*section).VirtualAddress) &&
			(rva < ((*section).VirtualAddress + ((*section).Misc).VirtualSize)))
		{
			return section;
		}
	}
	return(NULL);
}

/*
In some cases, it's not as simple as: Linear Address = baseAddress + RVA
In this case, you must perform a slight fix-up
*/

LPVOID rvaToPtr(DWORD rva, PIMAGE_NT_HEADERS64 peHeader, ULONGLONG baseAddress)
{
	PIMAGE_SECTION_HEADER sectionHeader;
	INT difference;

	sectionHeader = getCurrentSectionHeader(rva, peHeader);
	if (sectionHeader == NULL) { return(NULL); }
	difference = (INT)((*sectionHeader).VirtualAddress - (*sectionHeader).PointerToRawData);
	return((PVOID)((baseAddress + rva) - difference));
}

void processImportDescriptor(IMAGE_IMPORT_DESCRIPTOR importDescriptor, PIMAGE_NT_HEADERS64 peHeader, LPVOID baseAddress)
{
	PIMAGE_THUNK_DATA64 thunkILT;
	PIMAGE_THUNK_DATA64 thunkIAT;
	PIMAGE_IMPORT_BY_NAME nameData;
	int nFunctions;
	int nOrdinalFunctions;
	thunkILT = (PIMAGE_THUNK_DATA64)(importDescriptor.OriginalFirstThunk);
	thunkIAT = (PIMAGE_THUNK_DATA64)(importDescriptor.FirstThunk);

	if (thunkILT == NULL)
	{
		printf("[processImportDescriptor]: empty ILT\n");
		return;
	}

	if (thunkIAT == NULL)
	{
		printf("[processImportDescriptor]: empty IAT\n");
		return;
	}

	thunkILT = (PIMAGE_THUNK_DATA64)rvaToPtr((ULONGLONG)thunkILT, peHeader, (LPVOID)baseAddress);
	if (thunkILT == NULL)
	{
		printf("[processImportDescriptor]: empty ILT\n");
		return;
	}

	thunkIAT = (PIMAGE_THUNK_DATA64)rvaToPtr((ULONGLONG)thunkIAT, peHeader, (LPVOID)baseAddress);
	if (thunkIAT == NULL)
	{
		printf("[processImportDescriptor]: empty IAT\n");
		return;
	}
	nFunctions = 0;
	nOrdinalFunctions = 0;
	while ((*thunkILT).u1.AddressOfData != 0)
	{
		if (!((*thunkILT).u1.Ordinal & IMAGE_ORDINAL_FLAG))
		{
			printf("[processImportDescriptor]:\t");
			nameData = (PIMAGE_IMPORT_BY_NAME)((*thunkILT).u1.AddressOfData);
			nameData = (PIMAGE_IMPORT_BY_NAME)rvaToPtr((DWORD)nameData, peHeader, (LPVOID)baseAddress);
			printf("\t%s\t", (*nameData).Name);
			printf("address : %08X", thunkIAT->u1.Function);
			printf("\n");
		}
		else
		{
			nOrdinalFunctions++;
		}
		thunkILT++;
		thunkIAT++;
		nFunctions++;
	}

	printf("\t%d functions imported (%d ordinal)\n", nFunctions, nOrdinalFunctions);
	return;
}

void dumpImports(LPVOID baseAddress) {

	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS64 peHeader;

	IMAGE_OPTIONAL_HEADER64 optionalHeader;
	IMAGE_DATA_DIRECTORY importDirectory;
	ULONGLONG descriptorStartRVA;
	PIMAGE_IMPORT_DESCRIPTOR importDescriptor;

	int index;

	printf("[dumpImports]: checking DOS signature\n");
	dosHeader = (PIMAGE_DOS_HEADER)baseAddress;
	if (((*dosHeader).e_magic) != IMAGE_DOS_SIGNATURE)
	{
		printf("[dumpImports]: DOS signature not a match\n");
		return;
	}
	printf("DOS signature=%X\n", (*dosHeader).e_magic);

	printf("[dumpImports]: checking PE signature\n");
	peHeader = (PIMAGE_NT_HEADERS64)((DWORD64)baseAddress + (*dosHeader).e_lfanew);

	if (((*peHeader).Signature) != IMAGE_NT_SIGNATURE)
	{
		printf("[dumpImports]: PE signature not a match\n");
		return;
	}

	printf("PE signature=%X\n", (*peHeader).Signature);
	printf("[dumpImports] : checking OptionalHeader magic number\n");
	optionalHeader = (*peHeader).OptionalHeader;
	if ((optionalHeader.Magic) != 0x20B)
	{
		printf("[dumpImports]: OptionalHeader magic number does not match\n");
		return;
	}

	printf("OptionalHeader Magic number=%X\n", optionalHeader.Magic);

	printf("[dumpImports]: accessing import directory\n");
	importDirectory = (optionalHeader).DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	printf("[Import Directory at: %p\n", &importDirectory);
	descriptorStartRVA = importDirectory.VirtualAddress;
	

	importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)rvaToPtr(descriptorStartRVA, peHeader, (LPVOID)baseAddress);
	
	if (importDescriptor == NULL)
	{
		printf("[dumpImports]: First import descriptor is NULL\n");
		return;
	}

	printf("[Import Descriptor at: %p\n", &importDescriptor);

	index = 0;

	while (importDescriptor[index].Characteristics != 0)
	{
		char* dllName;
		dllName = (char*)rvaToPtr((importDescriptor[index]).Name, peHeader, (LPVOID)baseAddress); if (dllName == NULL)
		{
			printf("\n[dumpImports] :Imported DLL[%d]\t\tNULL Name\n", index);
		}
		else {
			printf("\n[dumpImports] :Imported DLL[%d]\t\t%s\n", index, dllName);
		}
		printf("---------------------------------------------------\n");
		processImportDescriptor(importDescriptor[index], peHeader, baseAddress);
		index++;
	}
	printf("[dumpImports] : %d DLLs Imported\n", index);
}

void closeHandles(HANDLE hFile, HANDLE hFileMapping, LPVOID baseAddress)
{
	printf("[closeHandles]: Closing up shop\n");
	UnmapViewOfFile(baseAddress);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);
	return;
}

void main(int argc, char* argv[])
{
	char* fileName;
	HANDLE hFile;
	HANDLE hFileMapping;
	LPVOID fileBaseAddress;
	BOOL retVal;

	if (argc < 2)
	{
		printf("(main]: not enough argumnets");
		return;
	}
	fileName = argv[1];
	retVal = getHMODULE(fileName, &hFile, &hFileMapping, &fileBaseAddress);
	if (retVal == FALSE) { return; }

	dumpImports(fileBaseAddress);

	closeHandles(hFile, hFileMapping, fileBaseAddress);
	return;
}
