
#include "windows.h" 
#include "stdio.h"


#include "dbgmsg.h"
#include "hookapi.c"

;


BOOL APIENTRY DllMain(HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{

	FILE *fptr;
	fptr = NULL;
	fptr = fopen("C:\\skelog.txt", "a");
	char api[20] = "GetCurrentProcessId";
	if (fptr == NULL)

	{
		return(TRUE);

	}
	// Perform actions based on the reason for calling.

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			DBG_PRINT2("[DllMain] :Process(%d) has loaded this DLL\n", GetCurrentProcessId());
			if (HookAPI(fptr,api) == FALSE)
			{
				DBG_TRACE("DllMain", "HookAPI() failed");
			}
			else
			{
				DBG_TRACE("DllMain", "HookAPI was a success");
			}
		}break;
	
		case DLL_THREAD_ATTACH:
		// Do thread - specific initialization.
		break;

		case DLL_THREAD_DETACH:
		// Do thread - specific cleanup.
		break;

		case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup
		fprintf(fptr,"Process(% d) has un-loaded this DLL\n",GetCurrentProcessId()) ;
		break;
	}
	fclose(fptr);
	return(TRUE); // Successful DLL_PROCESS_ATTACH
}


