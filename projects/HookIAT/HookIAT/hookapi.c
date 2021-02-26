 
#include "windows.h" 
#include "winnt.h"
#include "stdio.h"

#include "dbgmsg.h"


DWORD WINAPI MyGetCurrentProcessId()
{
	return(666);
}

void processImportDescriptor(FILE* fptr, IMAGE_IMPORT_DESCRIPTOR importDescriptor, PIMAGE_NT_HEADERS64 peHeader, LONGLONG baseAddress, char* apiName)
{
	PIMAGE_THUNK_DATA64 thunkILT;
	PIMAGE_THUNK_DATA64 thunkIAT;
	PIMAGE_IMPORT_BY_NAME nameData;
	int nFunctions;
	int nOrdinalFunctions;
	DWORD(WINAPI *procptr)();
	DWORD oldProtectionFlags;

	thunkILT = (PIMAGE_THUNK_DATA64)(importDescriptor.OriginalFirstThunk);
	thunkIAT = (PIMAGE_THUNK_DATA64)(importDescriptor.FirstThunk);

	if (thunkILT == NULL) {
		DBG_TRACE("[processImportDescriptor]", "empty ILT");
		return;
	}

	if (thunkIAT == NULL)
	{
		DBG_TRACE("[processImportDescriptor]", "empty IAT");
		return;
	}

	thunkILT = (PIMAGE_THUNK_DATA64)((LONGLONG)thunkILT + baseAddress);
	if (thunkILT == NULL)
	{
		DBG_TRACE("[processImportDescriptor]", "empty ILT"); 
		return;
	}

	thunkIAT = (PIMAGE_THUNK_DATA64)((LONGLONG)thunkIAT + baseAddress);
	if(thunkIAT == NULL)
	{
		DBG_TRACE("[processImportDescriptor]", "empty IAT");
		return;
	}

	nFunctions = 0;
	nOrdinalFunctions = 0;
	
	while ((*thunkILT).u1.AddressOfData != 0)
	{
		if (!((*thunkILT).u1.Ordinal & IMAGE_ORDINAL_FLAG))
		{
			DBG_PRINT1("[processImportDescriptor] :\t");
			nameData = (PIMAGE_IMPORT_BY_NAME)((*thunkILT).u1.AddressOfData);
			nameData = (PIMAGE_IMPORT_BY_NAME)((LONGLONG)nameData + (DWORD64)baseAddress);
			DBG_PRINT2("\t%s\t", (*nameData).Name);
			DBG_PRINT2("address : %llX", thunkIAT->u1.Function);
			DBG_PRINT1("\n");

			if (strcmp(apiName, (char*)(*nameData).Name) == 0)
			{
				DBG_PRINT2("[processImportDescriptor] : found a match for %s!!\n", apiName);

				procptr = MyGetCurrentProcessId;
				VirtualProtect(&thunkIAT->u1.Function, sizeof(LPVOID), PAGE_EXECUTE_READWRITE, &oldProtectionFlags);
				thunkIAT->u1.Function = (LONGLONG)procptr;
				VirtualProtect(&thunkIAT->u1.Function, sizeof(LPVOID), oldProtectionFlags, &oldProtectionFlags);
			}
		}
		else
		{
			nOrdinalFunctions++;
		}
		thunkILT++;
		thunkIAT++;
		nFunctions++;
	}

	DBG_PRINT3("\t%d functions imported (%d ordinal)\n", nFunctions, nOrdinalFunctions);
	return;
}


BOOL walkImportLists(FILE* fptr, LPVOID baseAddress, char* apiName)
{

	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS peHeader;

	IMAGE_OPTIONAL_HEADER64 optionalHeader;
	IMAGE_DATA_DIRECTORY importDirectory;
	ULONGLONG descriptorStartRVA;
	PIMAGE_IMPORT_DESCRIPTOR importDescriptor;

	int index;


	DBG_TRACE("walkImportLists","checking DOS signature");
	dosHeader = (PIMAGE_DOS_HEADER)baseAddress;
	if (((*dosHeader).e_magic) != IMAGE_DOS_SIGNATURE)
	{
		DBG_TRACE("walkImportLists", "DOS signature not a match");
		return(FALSE);
	}


	DBG_PRINT2("[walkImportLists): DOS signature=%X\n",(*dosHeader).e_magic);

	DBG_TRACE("walkImportLists", "checking PE signature");

	peHeader = (PIMAGE_NT_HEADERS64)((DWORD64)baseAddress + (*dosHeader).e_lfanew);

	if (((*peHeader).Signature) != IMAGE_NT_SIGNATURE)
	{
		DBG_TRACE("walkImportLists","PE signature not a match");		
		return(FALSE);
	}

	DBG_PRINT2("[walkImportLists] : PE signature=%X\n", (*peHeader).Signature);
	DBG_TRACE("walkImportLists", "checking OptionalHeader magic number");
	optionalHeader = (*peHeader).OptionalHeader;
	if ((optionalHeader.Magic) != 0x20B)
	{
		DBG_TRACE("walkImportLists","OptionalHeader magic number does not match\n");
		return(FALSE);
	}

	DBG_PRINT2("[walkImportLists): OptionalHeader Magic number=%X\n",optionalHeader.Magic);

	DBG_TRACE("walkImportLists", "accessing import directory");
	importDirectory = (optionalHeader).DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	descriptorStartRVA = importDirectory.VirtualAddress;

	importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(descriptorStartRVA + (LONGLONG)baseAddress);

	index = 0;

	while (importDescriptor[index].Characteristics != 0)
	{
		char *dllName;
		dllName = (char*)((importDescriptor[index]).Name + (LONGLONG)baseAddress);
		if (dllName == NULL)
		{
			DBG_PRINT2("\n[walkImportLists] : Imported DLL[% d]\tNULL Name\n",index);
		}
		else {
			DBG_PRINT3("\n[walkImportLists] :Imported DLL[%d]\t%s\n",index,dllName);
		}
		printf("---------------------------------------------------\n");
		processImportDescriptor(fptr, importDescriptor[index], peHeader, (LONGLONG)baseAddress, apiName);		
		index++;
	}
	DBG_PRINT2("[walkImportLists]:%d DLLs Imported\n", index);
	return(TRUE);
}

BOOL HookAPI(FILE *fptr, char* apiName)
{
	LPVOID baseAddress;
	baseAddress = (LPVOID)GetModuleHandle(NULL);
	DBG_PRINT2("[processImportDescriptor] : target API for hooking is: %s\n", apiName);
	return(walkImportLists(fptr, baseAddress, apiName));
}