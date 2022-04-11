//system includes
#include <stdio.h>
#include "WINDOWS.h"
#include "winioctl.h"

//shared includes
#include "..\KMD\ctrlcode.h"
#include "..\KMD\datatype.h"
#include "..\KMD\device.h"

// local includes
#include "dbgmsg.h"
#include "exitcode.h"
#include "cmdline.h"

// Device Driver functions
int setDeviceHandle(HANDLE *pHandle)
{
DBG_PRINT2("[setDeviceHandle]: Opening handle to %s \n", UserlandPath);
*pHandle = CreateFile(UserlandPath,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
if (*pHandle == INVALID_HANDLE_VALUE)
{
	DBG_PRINT2("[setDeviceHandle]: handle to %s not valid\n", UserlandPath);
	return(STATUS_FAILURE_OPEN_HANDLE);
}
DBG_TRACE("setDeviceHandle", "device file handle acquired");
return(STATUS_SUCCESS);
}

//Operations
int TestOperation(HANDLE hDeviceFile)
{
	BOOL opStatus = TRUE;
	char *inBuffer;
	char *outBuffer;
	DWORD nBufferSize = 36;
	DWORD bytesRead = 0;

	inBuffer  = (char*)malloc(nBufferSize);
	outBuffer = (char*)malloc(nBufferSize);
	if ((inBuffer == NULL) || (outBuffer == NULL))
	{
		DBG_TRACE("TestOperation", "Could not allocate memory for CMD_TEST_OP");
		return(STATUS_FAILURE_NO_RAM);
	}

	sprintf(inBuffer,  "This is the INPUT buffer");
	sprintf(outBuffer, "This is the OUTPUT buffer");

	DBG_PRINT2("[TestOperation]: cmd=%s, Test Command\n", CMD_TEST_OP);

	opStatus = DeviceIoControl(hDeviceFile, (DWORD)IOCTL_TEST_CMD, (LPVOID)inBuffer, nBufferSize, (LPVOID)outBuffer, nBufferSize, &bytesRead, NULL);
	if (opStatus == FALSE)
	{
		DBG_TRACE("TestOperation", "Call to DeviceloControl() FAILED\n");
	}

	printf("[TestOperation]: bytesRead=%d\n", bytesRead);
	printf("[Testoperation] : outBuffer=%s\n", outBuffer);

	free(inBuffer);
	free(outBuffer); return(STATUS_SUCCESS);
}

//Command-Line Routines
char* editArg(char* src) {
	if (strlen(src) >= MAX_ARGV_SZ) {
		src[MAX_ARGV_SZ-1] = '\0';
	}
	return(src);
}

/*
Filter out bad commands 
	Conrnand-line should look like 
	file.exe	operation	operand 
	argv[0]		argv[l]		argv[2]
*/

int chkCmdLine(int argc, char* argv[]) {
	int i;

	DBG_TRACE("chkCmdLine", "[begin]---------");
	DBG_PRINT2("[chkCmdLine] : argc=%i\n", argc);

	if (argc > MAX_CMDLINE_ARGS)
	{
		DBG_PRINT2("[chkCmdLine]: argc=%d, too many arguments \n", argc);
		DBG_TRACE("chkCmdLine ", "[failed]---------");
		return(STATUS_FAILURE_MAX_ARGS);
	}

	else if (argc < MIN_CMDLINE_ARGS)
	{
		DBG_PRINT2("[chkCmdLine] : argc=%d, not enough arguments\n", argc);
		DBG_TRACE("chkCmdLine", "[failed]--------");
		return(STATUS_FAILURE_NO_ARGS);
	}

	for (i = 0; i < argc; i++) {
		char buffer[MAX_ARGV_SZ];
		DBG_PRINT2("\tchkCmdLine: arg[%d)", i);
		DBG_PRINT2("=%s\n", strncpy(buffer, editArg(argv[i]), MAX_ARGV_SZ));
	}

	if (strlen(ARGV_OPERATION) > MAX_OPERATION_SZ)
	{
		DBG_PRINT2("[chkCmdLine]: command=%s, not recognized\n", ARGV_OPERATION);
		DBG_TRACE("chkCmdLine", "[failed]---------");
		return(STATUS_FAILURE_BAD_CMD);
	}

	DBG_TRACE("chkCmdLine", "[passed)----------");
	return (STATUS_SUCCESS);
}

/*
Process commands and invoke the corresponding operation function
*/

int procCmdLine(char* argv[])

{
	int retCode = STATUS_SUCCESS;
	HANDLE hDeviceFile = INVALID_HANDLE_VALUE;

	retCode = setDeviceHandle(&hDeviceFile);
	if (retCode != STATUS_SUCCESS)
	{
		return (retCode);
	}

	// execute commands

	if (strncmp(ARGV_OPERATION,CMD_TEST_OP, MAX_OPERATION_SZ) == 0)
	{
		retCode = TestOperation(hDeviceFile);
	}
	else
	{
		DBG_PRINT2("[procCmdLine]: command=%s, not recognized\n", ARGV_OPERATION);
		return(STATUS_FAILURE_BAD_CMD);
	}


	//perform some basic cleanup

	DBG_PRINT2("[procCmdLine] : Closing handle to %s\n", UserlandPath);
	if (CloseHandle(hDeviceFile) == FALSE)
	{
		DBG_PRINT2(" [procCmdLine] : Errors closing handle to %s\n", UserlandPath);
		return(STATUS_FAILURE_CLOSE_HANDLE);
	}
	DBG_TRACE("procCmdLine", "Command processing completed");
	return(retCode);
}

int main(int argc, char* argv[])
{
	int retCode =1;
	DBG_TRACE("main", "program execution initiated");

	retCode = chkCmdLine(argc, argv);

	if (retCode != STATUS_SUCCESS)
	{
		DBG_PRINT2("[main) : Application failed, exit code = (%d)\n:", retCode); 
		return (retCode) ;
	}
	retCode = procCmdLine(argv);
	if (retCode != STATUS_SUCCESS)
	{
		DBG_PRINT2("[main) : Application failed, exit code = (%d)\n:", retCode);
		return (retCode);
	}

	DBG_TRACE("main", "program exiting normally");
	return(STATUS_SUCCESS);

}
