#include "../include/vfb4-data.h"

#define _X86_
#include <ntddk.h>

#define DEVICE_VFB4_NAME	L"\\Device\\vfb4"
#define DEVICE_VFB4_DOSNAME	L"\\DosDevices\\vfb4"
#define VFB4_BUF_SIZE	(1024*1024*4)
//1658880


#define _DEBUG

/*
	internal variables
*/
void* vfb4_buffer = NULL;
PMDL vfb4_mdl = NULL;
void* vfb4_user = NULL;

/*

  Declare IO service function

*/
NTSTATUS vfb4_create(PDEVICE_OBJECT device_object, PIRP irp);
NTSTATUS vfb4_close(PDEVICE_OBJECT device_object, PIRP irp);
NTSTATUS vfb4_read(PDEVICE_OBJECT device_object, PIRP irp);
NTSTATUS vfb4_unsupported(PDEVICE_OBJECT device_object, PIRP irp);


/*

	Maintance functions

*/
void vfb4_unload(PDRIVER_OBJECT  DriverObject);
NTSTATUS DriverEntry(PDRIVER_OBJECT  driver_object, PUNICODE_STRING  registry_path);



/*
	Setup compiler and liker code allocation directives
*/
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, vfb4_unload)
#pragma alloc_text(PAGE, vfb4_create)
#pragma alloc_text(PAGE, vfb4_close)
#pragma alloc_text(PAGE, vfb4_read)


/*

	Functions code

*/

NTSTATUS DriverEntry(PDRIVER_OBJECT  driver_object, PUNICODE_STRING  registry_path)
{
    NTSTATUS status = STATUS_SUCCESS;
    unsigned int i = 0;
    PDEVICE_OBJECT device_object = NULL;
    UNICODE_STRING device_name,device_name_dos;
	PHYSICAL_ADDRESS highest_acceptable, lowest_acceptable;
	

#ifdef _DEBUG
    DbgPrint("'DriverEntry' enter\r\n");
#endif

	// init driver name
    RtlInitUnicodeString(&device_name, DEVICE_VFB4_NAME);
	RtlInitUnicodeString(&device_name_dos, DEVICE_VFB4_DOSNAME);

	// create device instance
	status = IoCreateDevice
	(
		driver_object,				//	IN PDRIVER_OBJECT  DriverObject,
		0,							//	IN ULONG  DeviceExtensionSize,
		&device_name,				//	IN PUNICODE_STRING  DeviceName  OPTIONAL,
		FILE_DEVICE_VIDEO,			//	IN DEVICE_TYPE  DeviceType,
		FILE_DEVICE_SECURE_OPEN,	//	IN ULONG  DeviceCharacteristics,
		FALSE,						//	IN BOOLEAN  Exclusive,
		&device_object				//	OUT PDEVICE_OBJECT  *DeviceObject
    );

	// check status
	if(status == STATUS_SUCCESS)
	{
		// service function register

		// set all function to unknow first
		for(i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) driver_object->MajorFunction[i] = vfb4_unsupported;

		// primary function Create Close
		driver_object->MajorFunction[IRP_MJ_CLOSE]		= vfb4_close;
		driver_object->MajorFunction[IRP_MJ_CREATE]		= vfb4_create;

		// we support only reading function
        driver_object->MajorFunction[IRP_MJ_READ]		= vfb4_read;

		// unload function		
        driver_object->DriverUnload						= vfb4_unload;

		// set flags         
        driver_object->Flags |= DO_DIRECT_IO;
        driver_object->Flags &= (~DO_DEVICE_INITIALIZING);

		device_object->Flags |= DO_DIRECT_IO;
		device_object->Flags &= (~DO_DEVICE_INITIALIZING);
    
		// Create a Symbolic Link to the device. Example 
        IoCreateSymbolicLink(&device_name_dos, &device_name);

		// init frame buffer
		highest_acceptable.QuadPart = (ULONGLONG)-1;
		lowest_acceptable.QuadPart = 0;
		vfb4_buffer = MmAllocateContiguousMemory
		(
			VFB4_BUF_SIZE,
			highest_acceptable
		);

		// 
		if (vfb4_buffer)
		{
			if (vfb4_mdl = IoAllocateMdl (vfb4_buffer, VFB4_BUF_SIZE, FALSE, FALSE, NULL))
				MmBuildMdlForNonPagedPool(vfb4_mdl);
		};
    }


    return status;
}



/*

  Unload

*/
VOID vfb4_unload(PDRIVER_OBJECT  driver_object)
{    
    UNICODE_STRING device_name_dos;
    
#ifdef _DEBUG
    DbgPrint("'vfb4_unload' enter\r\n");
#endif

	// init driver name
	RtlInitUnicodeString(&device_name_dos, DEVICE_VFB4_DOSNAME);
    
	// delete symbol link
	IoDeleteSymbolicLink(&device_name_dos);

	// delete device
    IoDeleteDevice(driver_object->DeviceObject);
}


/*------------------------------------------------------------------------

	Read function:
	Fill structure 'vfb4_data' with test values for now!



> >    struct _EPROCESS *Process;
Pointer to the process whose pages are being described by the MDL, for
MDLs that describe kernel mode memory, this field is NULL since in
kernel mode address space is shared.

> >    PVOID MappedSystemVa;
This field holds the mapped System (Kernel) Virtual Address when
MmMapLockedPagesSpecifyCache() is called on the MDL.


------------------------------------------------------------------------*/
NTSTATUS vfb4_read(PDEVICE_OBJECT device_object, PIRP irp)
{
	PIO_STACK_LOCATION irp_stack;
	void* buffer;
	NTSTATUS status;
	vfb4_data user_datas;
	PHYSICAL_ADDRESS physical_address;

#ifdef _DEBUG
    DbgPrint("'vfb4_read' enter\r\n");
#endif
	
	// request current stack
	irp_stack = IoGetCurrentIrpStackLocation(irp);

	// we always return success and size if pointer to 1001,1002,1003.....
	irp->IoStatus.Status = (status = STATUS_SUCCESS);


	if (!(irp_stack))
		irp->IoStatus.Information  = 1001;
	else 
	{
		if (!(irp->MdlAddress))
			irp->IoStatus.Information  = 1002;
		else
		{
			if ((buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress,HighPagePriority)) == NULL)
				irp->IoStatus.Information  = 1003;
			else
			{
				if (irp_stack->Parameters.Read.Length < sizeof(vfb4_data))
				{
					irp->IoStatus.Information  = 1004;
				}
				else
				{
					// define struct vfb4_data and fill
					user_datas.kernel = (unsigned long)vfb4_buffer;
					user_datas.user = 0;
					user_datas.physical_high = 0;
					user_datas.physical_low = 0;

					if(vfb4_buffer)
					{
						//MmProbeAndLockPages(vfb4_mdl,KernelMode, IoModifyAccess);
						vfb4_user = MmMapLockedPages (vfb4_mdl, UserMode);

						physical_address = MmGetPhysicalAddress(vfb4_buffer);
						user_datas.physical_low = physical_address.LowPart;
						user_datas.physical_high = physical_address.HighPart;

						user_datas.user = (unsigned long)vfb4_user;
					}
				
					
					// use kernel RunTime func to copy memory
					RtlCopyMemory ( buffer, &user_datas, sizeof(vfb4_data) );

					// set status OK!
					irp->IoStatus.Information  = sizeof(vfb4_data);
				};
			};
		};
	};


	IoCompleteRequest (irp, IO_NO_INCREMENT);

    return status;
}



/*------------------------------------------------------------------------

	Create function - we do nothing

------------------------------------------------------------------------*/
NTSTATUS vfb4_create(PDEVICE_OBJECT device_object, PIRP irp)
{
#ifdef _DEBUG
    DbgPrint("'vfb4_create' enter\r\n");
#endif
    return STATUS_SUCCESS;
}



/*------------------------------------------------------------------------

	Create function - we do nothing

------------------------------------------------------------------------*/
NTSTATUS vfb4_close(PDEVICE_OBJECT device_object, PIRP irp)
{
#ifdef _DEBUG
    DbgPrint("'vfb4_close' enter\r\n");
#endif
    return STATUS_SUCCESS;
}



/*------------------------------------------------------------------------

	'unsupported' functions handler - just return status
	'STATUS_NOT_SUPPORTED'

------------------------------------------------------------------------*/
NTSTATUS vfb4_unsupported(PDEVICE_OBJECT device_object, PIRP irp)
{
#ifdef _DEBUG
    DbgPrint("'vfb4' unsupported function handler entered\r\n");
#endif

    return STATUS_NOT_SUPPORTED;
}
