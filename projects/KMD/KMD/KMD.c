#include "ntddk.h"

//local includes
#include "dbgmsg.h"

//shared includes
#include "ctrlcode.h"
#include "device.h"

//globals
PDEVICE_OBJECT MSNetDiagDeviceObject;
PDRIVER_OBJECT DriverObjectRef;

// function prototypes
void TestCommand(PVOID inputBuffer, PVOID outputBuffer, ULONG inputBufferLength, ULONG outputBufferLength);
NTSTATUS defaultDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS dispatchIOControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS RegisterDriverDeviceName(IN PDRIVER_OBJECT DriverObject);
NTSTATUS RegisterDriverDeviceLink();

//Operation Routines
void TestCommand(PVOID inputBuffer, PVOID outputBuffer, ULONG inputBufferLength, ULONG outputBufferLength)
{
	char *ptrBuffer;
	DBG_TRACE("dispathIOControl", "Displaying inputBuffer");
	ptrBuffer = (char*)inputBuffer;
	DBG_PRINT2("[dispatchIOControl]: inputBuffer=%s\n", ptrBuffer);
	DBG_TRACE("dispatchIOControl", "Populating outputBuffer");
	ptrBuffer = (char*)outputBuffer;
	ptrBuffer[0] = '!';
	ptrBuffer[1] = '1';
	ptrBuffer[2] = '2';
	ptrBuffer[3] = '3';
	ptrBuffer[4] = '!';
	ptrBuffer[5] = '\0';
	DBG_PRINT2("[dispatchIOControl]:outputBuffer=%s\n", ptrBuffer);
	return;
}

//Dispatch Handlers
NTSTATUS defaultDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP IRP)
{
	((*IRP).IoStatus).Status = STATUS_SUCCESS;
	((*IRP).IoStatus).Information = 0;
	IoCompleteRequest(IRP, IO_NO_INCREMENT);

	return (STATUS_SUCCESS);
}

NTSTATUS dispatchIOControl(IN PDEVICE_OBJECT pDeviceObject, IN PIRP IRP)
{
	PIO_STACK_LOCATION irpStack;
	PVOID inputBuffer;
	PVOID outputBuffer;
	ULONG inBufferLength;
	ULONG outBufferLength;
	ULONG ioctrlcode;
	NTSTATUS ntStatus;

	ntStatus = STATUS_SUCCESS;
	((*IRP).IoStatus).Status = STATUS_SUCCESS;
	((*IRP).IoStatus).Information = 0;

	inputBuffer =  (*IRP).AssociatedIrp.SystemBuffer;
	outputBuffer = (*IRP).AssociatedIrp.SystemBuffer;

	//get a pointer to the caller's stack location in the given IRP 
	//This is where the function codes and other parameters are

	irpStack = IoGetCurrentIrpStackLocation(IRP);
	inBufferLength =  (*irpStack).Parameters.DeviceIoControl.InputBufferLength;
	outBufferLength = (*irpStack).Parameters.DeviceIoControl.OutputBufferLength;
	ioctrlcode = (*irpStack).Parameters.DeviceIoControl.IoControlCode;

	DBG_TRACE("dispatchIOControl", "Received a command");
	
	switch (ioctrlcode)
	{
		case IOCTL_TEST_CMD:
		{
			TestCommand(inputBuffer, outputBuffer, inBufferLength, outBufferLength);
			((*IRP).IoStatus).Information = outBufferLength;
		}break;
		default:
		{
			DBG_TRACE("dispatchIOControl", "control code not recognized");
		}break;
	}
	IoCompleteRequest(IRP, IO_NO_INCREMENT);
	return(ntStatus);
}

//Device and Driver Naming Routines

NTSTATUS RegisterDriverDeviceName(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS ntStatus;
	UNICODE_STRING unicodeString;

	RtlInitUnicodeString(&unicodeString, DeviceNameBuffer);


	ntStatus = IoCreateDevice
	(
		pDriverObject,			//pointer to driver object
		0,						//# bytes allocated for device extension of device objec
		&unicodeString,			//unicode string containing device name
		FILE_DEVICE_RK,			// driver type(vendor defined)
		0,						//one or more system - defined constants, OR - ed together
		TRUE,					//the device object is an exclusive device
		&MSNetDiagDeviceObject	//pointer to global device object
	);
	return (ntStatus);
}

NTSTATUS RegisterDriverDeviceLink()
{
	NTSTATUS ntStatus;
	UNICODE_STRING unicodeString;
	UNICODE_STRING unicodeLinkString;
	RtlInitUnicodeString(&unicodeString, DeviceNameBuffer);
	RtlInitUnicodeString(&unicodeLinkString, DeviceLinkBuffer);

	ntStatus = IoCreateSymbolicLink(&unicodeLinkString, &unicodeString);
	return (ntStatus);
}

//DRIVER_OBJECT functions
VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT pdeviceObj;
	UNICODE_STRING unicodeString;
	DBG_TRACE("OnUnload", "Received signal to unload the driver");
	pdeviceObj = (*DriverObject).DeviceObject;
	if (pdeviceObj != NULL)
	{
		DBG_TRACE("OnUnload", "Unregistering driver's symbolic link");
		RtlInitUnicodeString(&unicodeString, DeviceLinkBuffer);
		IoDeleteSymbolicLink(&unicodeString);
		DBG_TRACE("OnUnload", "Unregistering driver's device name");
		IoDeleteDevice((*DriverObject).DeviceObject);
	}
	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING regPath)
{
	int i;
	NTSTATUS ntStatus;
	DBG_TRACE("Driver Entry", "Driver has been loaded");
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		(*pDriverObject).MajorFunction[i] = defaultDispatch;
	}
	(*pDriverObject).MajorFunction[IRP_MJ_DEVICE_CONTROL] = dispatchIOControl;
	(*pDriverObject).DriverUnload = Unload;

	DBG_TRACE("Driver Entry", "Registering driver's device name");
	ntStatus = RegisterDriverDeviceName(pDriverObject);
	if (!NT_SUCCESS(ntStatus))
	{
		DBG_TRACE("Driver Entry", "Failed to create device");
		return ntStatus;
	}

	DBG_TRACE("Driver Entry", "Registering driver's symbolic link");
	ntStatus = RegisterDriverDeviceLink();
	if (!NT_SUCCESS(ntStatus))
	{
		DBG_TRACE("Driver Entry", "Failed to create symbolic link");
		return ntStatus;
	}
	DriverObjectRef = pDriverObject;
	return STATUS_SUCCESS;
}


