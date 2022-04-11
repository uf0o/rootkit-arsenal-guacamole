// system includes
#include <ntddk.h>

// local includes
#include "dbgmsg.h"
#include "datatype.h"

//globals
LONG64 LockAcquired;
LONG64 nCPUsLocked;
extern void NOP_FUNC(void);
extern void SHARED_FUNC(void);
void AccessResource(void);

// Sync Routines
KIRQL RaiseIRQL()
{
	KIRQL curr;
	KIRQL prev;
	/* Get the current interrupt irql level */
	curr = KeGetCurrentIrql();
	prev = curr;
	/*
	* The normal level of ordinary threads in windows is lower than DISPATCH_LEVEL,
	* The level of interrupt scheduling thread is DISPATCH_LEVEL
	* Threads higher than DISPATCH_LEVEL cannot be preempted.
   */
	if (curr < DISPATCH_LEVEL)
	{
		/* Increase the mid-level of the current thread to DISPATCH_LEVEL, so that the current thread will not be preempted */
		KeRaiseIrql(DISPATCH_LEVEL, &prev);
	}
	return (prev);
}

// Routine executed by the DPCs
void lockRoutine
(
	IN PKDPC dpc,
	IN PVOID context,
	IN PVOID arg1,
	IN PVOID arg2
)
{
	DBG_PRINT2("[lockRoutine]: begin-CPU[%u]", KeGetCurrentProcessorNumber());
	/* Atomically increase the value of nCPUsLocked, which means that another CPU enters the nop cycle */
	InterlockedIncrement64(&nCPUsLocked);

	// //spin NOP until LockAcquired flag is set ( i.e., by ReleaseLockO))
	while (InterlockedCompareExchange64(&LockAcquired, 1, 1) == 0)
	{
		NOP_FUNC();
	}
	/* Exit the NOP loop */
	InterlockedDecrement64(&nCPUsLocked);
	DBG_PRINT2("[lockRoutine]: end-CPU[%u]", KeGetCurrentProcessorNumber());
	return;
}

PKDPC AcquireLock()
{
	PKDPC dpcArray;
	DWORD cpuID;
	DWORD i;
	DWORD nOtherCPUs;

	// this should be taken care of by RaiseIRQL()
	if (KeGetCurrentIrql() != DISPATCH_LEVEL) { return (NULL); }
	DBG_TRACE("AcquireLock,", "Executing at IRQL == DISPATCH_LEVEL");

	/* Initialize LockAcquired and nCPUsLocked to 0 */
	InterlockedAnd(&LockAcquired, 0);
	InterlockedAnd(&nCPUsLocked, 0);

	//allocate DPC object array in nonpaged memory
	/* keNumberProcessors is a kernel variable, indicating the number of CPUs */
	DBG_PRINT2("[AcquiredLock]: nCPUs=%u", KeNumberProcessors);

	// we need NonPagedPool since DPC running at IRQL2 will not be able to handle the paged out exception
	dpcArray = (PKDPC)ExAllocatePoolWithTag(NonPagedPool,KeNumberProcessors * sizeof(KDPC), 0x0F11);
	if (dpcArray == NULL) { return(NULL); }

	cpuID = KeGetCurrentProcessorNumber();
	DBG_PRINT2("[AcquireLock]: cpuID=%u", cpuID);

	//create a DPC object for each CPU and insert into DPC queue
	for (i = 0; i < KeNumberProcessors; i++)
	{
		PKDPC dpcPtr = &(dpcArray[i]);
		
		if (i != cpuID)

		{
			KeInitializeDpc(dpcPtr, lockRoutine, NULL);
			// Exxecuteing this DPC on a certain CPU 
			KeSetTargetProcessorDpc(dpcPtr, i);
			KeInsertQueueDpc(dpcPtr, NULL, NULL);
		}
	}

	//spin until all CPUs have been elevated
	nOtherCPUs = KeNumberProcessors - 1;
	InterlockedCompareExchange64(&nCPUsLocked, nOtherCPUs, nOtherCPUs);
    // Wait for threads on other CPUs to enter the nop loop. In LockRoutine, nCPUsLocked will increase by one and enter the nop loop.
	// After other CPUs enter the nop cycle, they can safely access shared resources
	while (nCPUsLocked != nOtherCPUs)
	{
		NOP_FUNC();
		InterlockedCompareExchange64(&nCPUsLocked, nOtherCPUs, nOtherCPUs);
	}
	DBG_TRACE("AcquireLock", "All CPUs have been elevated");
	return (dpcArray);
}

NTSTATUS ReleaseLock(PVOID dpcPtr)
{
	//this will cause all DPCs to exit their while loops
	InterlockedIncrement64(&LockAcquired);

	//spin until all CPUs have been restored to old IRQLs
	InterlockedCompareExchange64(&nCPUsLocked, 0, 0);
	while (nCPUsLocked != 0)
	{
		NOP_FUNC();
		InterlockedCompareExchange64(&nCPUsLocked, 0, 0);
	}
	if (dpcPtr != NULL)
	{
		ExFreePool(dpcPtr);
	}
	DBG_TRACE("ReleaseLock", "All CPUs have benn released");
	return (STATUS_SUCCESS);
}

void LowerIRQL(KIRQL prev)
{
	KeLowerIrql(prev);
	return;
}

void Unload(IN PDRIVER_OBJECT pDriverObject)
{
	DBG_TRACE("Unload", "Received signal to unload the driver");
	return;
}

void AccessResource()
{
	
	int i = 0;
    int max = 1 * 1000000000;
	DBG_TRACE("Doing actual stuff","Accessing Shared Resource");
	//KeDelayExecutionThread(KernelMode, FALSE, 0); //uncomment to get a BSOD :)
	for (i = 0; i < max ; i++)
	{
		SHARED_FUNC();
	}
	return;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING regPath)
{
	NTSTATUS ntStatus;
	KIRQL irql;
	PKDPC dpcPtr;
	DBG_TRACE("Driver Entry", "Establishing other DriverObject function pointers");
	(*pDriverObject).DriverUnload = Unload;
	
	DBG_TRACE("Driver Entry", "Raising IRQL");
	irql = RaiseIRQL();

	DBG_TRACE("Driver Entry", "Acquiring Lock");
	dpcPtr = AcquireLock();

	// Accessing Shared resource.
	AccessResource();
	
	DBG_TRACE("Driver Entry", "Releasing Lock");
	ReleaseLock(dpcPtr);

	DBG_TRACE("Driver Entry", "Lowering IRQL");
	LowerIRQL(irql);
	return (STATUS_SUCCESS);

}


