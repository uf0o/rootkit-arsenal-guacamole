#include "windows.h" 
#include "stdio.h" 
#include "stdlib.h"


void main(int argc, char* argv[])
{
	HANDLE procHandle;
	HANDLE threadHandle;
	HMODULE dllHandle;
	DWORD procID;
	LPTHREAD_START_ROUTINE loadLibraryAddress;
	LPVOID baseAddress;
	char* argumentBuffer;
	BOOL isValid;

	//get PID
	if (argc < 3)
	{
		printf("Not enough arguments\n");
		printf("Usage: remotethread.exe <target_pid> <full_path_to_injecting_dll>\n");
		return;
	}
	procID = atoi(argv[1]);
	argumentBuffer = argv[2];
	printf("PID=%d\n", procID);

	// get handle to process
	procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
	if (procHandle == NULL)
	{
		printf("Could not get handle to process\n");
		return;
	}
	printf("Handle to process acquired\n");

	//get handle to Kerne132.dll
	dllHandle = GetModuleHandleA("kernel32");
	if (dllHandle == NULL)
	{
		printf("Could not get handle to Kernel32.dll\n");
		return;
	}
	printf("handle to Kernel32.dll acquired\n");

	//get address of loadLibrary()
	loadLibraryAddress = GetProcAddress(dllHandle, "LoadLibraryA");
	if (loadLibraryAddress == NULL) {
		printf("Could not get address of LoadLibrary()\n");
		return;
	}
	printf("address of LoadLibrary() acquired\n");

	//Create argument to LoadLibraryA in remote process
	baseAddress = (LPVOID)VirtualAllocEx(procHandle, NULL, strlen(argumentBuffer),MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	if (baseAddress == NULL) {
		printf("Could not allocate memory in remote process\n");
		return;
	}
	printf("allocated memory in remote process\n");

	isValid = WriteProcessMemory(procHandle, baseAddress, argumentBuffer, strlen(argumentBuffer), NULL);

	if (isValid == 0) {
		printf("value could not be written to memory\n");
		return;}
	printf("value written memory\n");

	//Invoke DLL from remote thread
	threadHandle = CreateRemoteThread(procHandle, NULL, NULL, loadLibraryAddress, baseAddress, 0, NULL);
	if (threadHandle == 0) {
		printf("thread could not be created\n");
		return;
	}
	printf("thread created\n");

	return;
}