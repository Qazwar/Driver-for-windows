#include <ntddk.h>

//�����豸�������ƺͷ�������,ע�⣬������ַ���Ҫ�ÿ��ֽ��ַ��������ַ���ǰ��L
//ע�⣬�豸�������ƺͷ�����������Ҫһ��
#define DEVICE_NAME L"\\device\\NtModel"
#define LINK_NAME L"\\dosdevices\\NtModel"

//����DispatchIocontrol��������r3������ͨѶ��ʽ������
#define IOCTRL_BASE 0x800

//����(i)�ͺ궨��������м䲻���пո��Լ��ڵĿӡ�����
#define MYIOCTRL_CODE(i) \
	CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTRL_BASE+i, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define CTL_HELLO MYIOCTRL_CODE(0)
#define CTL_PRINT MYIOCTRL_CODE(1)
#define CTL_BYE MYIOCTRL_CODE(2)

//����r3����IRPʱ�ķַ�������
//r3������IRP���������أ��������ں��д�����һ���豸��������������������ں����б������ģ���
//����豸������ǵ�һ������
//r3���ǽ�IRP���͸�����豸�����

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//����pIrp����R3���ش���״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	//��r3���ض�����Ϣ�����紦��r3��read�ַ�����ʱ������ط���ʾʵ�ʶ�ȡ�˶��ٸ��ֽڸ�r3
	pIrp->IoStatus.Information = 0;

	//���������ִ�б�ʾ����˴�IRP��������ĺ���
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//���ظ�IO��������״̬��
	return STATUS_SUCCESS;
}

NTSTATUS DispatchCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//�����ǻ���buffer��io�����Ի�ȡbuffer�ĵ�ַ��IRPͷ������buffer�ĳ���Ҫ�ڵ�ǰ�豸��IRPջ�л�ȡ��Ӧ�Ĳ�
	//�л�ȡ
	PVOID pReadBuffer			= NULL;
	ULONG uReadLength			= 0;
	//��ȡ��ǰ�豸��IRPջ�����ڵ�λ�ã�����˵�ǵ�ǰ�豸��IRPջ�е�ջ֡
	PIO_STACK_LOCATION pStack	= NULL;

	//����д�볤�ȣ�������Ȳ��Ǹ���buffer�ĳ��Ȼ�����Ҫд��ĳ���Ϊ��׼������ȡ�����߽�Сֵ
	ULONG uMin					= 0;
	pReadBuffer = pIrp->AssociatedIrp.SystemBuffer;

	//��ȡջ֡
	pStack = IoGetCurrentIrpStackLocation(pIrp);

	//������ȡbuffer�ĳ���
	uReadLength = pStack->Parameters.Read.Length;

	uMin = (wcslen(L"Hello world") + 1) * sizeof(WCHAR) > uReadLength ? uReadLength :
		(wcslen(L"Hello world") + 1) * sizeof(WCHAR);

	//��buffer��д��r3Ҫ��ȡ������
	RtlCopyMemory(pReadBuffer, L"Hello world", uMin);

	//д��֮��ͬ���ģ����÷��ظ�r3�Ĵ�����
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = uMin;

	//ͬ��Ҫִ�н�������
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	//����Ҫ����buffer�����ȡ�ջ
	PVOID pWriteBuffer			= NULL;
	ULONG uWriteLength			= 0;
	PIO_STACK_LOCATION pStack	= NULL;

	//����һ���ں˶������r3����Ҫд�������
	PVOID pBuffer				= NULL;

	pWriteBuffer = pIrp->AssociatedIrp.SystemBuffer;
	pStack = IoGetCurrentIrpStackLocation(pIrp);
	uWriteLength = pStack->Parameters.Write.Length;

	//ʹ�ú���ExallocatePoolWithTag������һ���ں˶�
	//��һ����������ָ��������ں˶��Ƿ�ҳ�ڴ棬��r0�У�ֻ�д���PASSIVE����ĺ�������ʹ�÷�ҳ�ڴ棬
	//��IRP�ķַ������У����зַ��������𶼴���PASSIVE����
	//����һ����NonPagePool���Ƿ�ҳ�ڴ棬�����ڴ�ʹ���ٶȿ���Ұ�ȫ��������ļ��ʺܵͣ�
	//���ǳ������ޣ�ʹ��ʱ����������ʹ�÷Ƿ�ҳ�ڴ�
	pBuffer = ExAllocatePoolWithTag(PagedPool, uWriteLength, 'TSET');

	if (pBuffer == NULL)
	{
		pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		DbgPrint("ExAllocatePoolWithTag failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	memset(pBuffer, 0x00, uWriteLength);

	RtlCopyMemory(pBuffer, pWriteBuffer, uWriteLength);

	//�ͷŴ��ں˶�������ڴ�
	ExFreePool(pBuffer);
	pBuffer = NULL;//��������ˡ�����
	//��r3����״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = uWriteLength;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//���ظ�IO������
	return STATUS_SUCCESS;
}

NTSTATUS DispatchClean(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Status = 0;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchIocontrol(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	PVOID inputBuffer			= NULL;
	PVOID outputBuffer			= NULL;
	ULONG inputLength			= 0;
	ULONG outputLength			= 0;
	PIO_STACK_LOCATION pStack	= NULL;
	ULONG uIocontrolCode		= 0;

	inputBuffer = outputBuffer = pIrp->AssociatedIrp.SystemBuffer;
	pStack = IoGetCurrentIrpStackLocation(pIrp);
	inputLength = pStack->Parameters.DeviceIoControl.InputBufferLength;
	outputLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;
	uIocontrolCode = pStack->Parameters.DeviceIoControl.IoControlCode;

	switch (uIocontrolCode)
	{
	case CTL_HELLO:
		DbgPrint("hello iocontrol\n");
		break;
	case CTL_PRINT:
		DbgPrint("%ws\n", inputBuffer);
		break;
	case CTL_BYE:
		DbgPrint("GoodBye iocontrol\n");
		break;
	default:
		DbgPrint("unknow iocontrol\n");
		break;
	}


	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//����ж�غ���
VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	//������ж�غ�����Ҫ����������ӡ��豸����
	UNICODE_STRING uLinkName = { 0 };
	RtlInitUnicodeString(&uLinkName, LINK_NAME);
	IoDeleteSymbolicLink(&uLinkName);

	IoDeleteDevice(pDriverObject->DeviceObject);

	DbgPrint("Driver unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	//.c�ļ��������Ҫ�ں�������ǰ�棬�����������
//���豸�������ơ������������ƵĿ��ֽ��ַ���ת����unicode������ַ���
	UNICODE_STRING uDeviceName		= { 0 };
	UNICODE_STRING uLinkName		= { 0 };
	//��������ַ��������ú�ķ���ֵ,32λ������0Ϊ�ɹ�
	NTSTATUS ntStatus				= 0;
	ULONG i							= 0;

	//�����豸����ָ��,�������������iocreate�������豸����
	PDEVICE_OBJECT pDeviceObject	= NULL;

	DbgPrint("Load driver begin\n");

	//��ʼ���豸�������ͷ�������
	RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uLinkName, LINK_NAME);

	//�����豸����
	ntStatus = IoCreateDevice(pDriverObject, 0, &uDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);

	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateDevice faild:%x\n", ntStatus);
		return ntStatus;
	}

	//ָ��r3��r0��ͨѶ��ʽ
	pDeviceObject->Flags |= DO_BUFFERED_IO;

	//������������
	ntStatus = IoCreateSymbolicLink(&uLinkName, &uDeviceName);
	if (!NT_SUCCESS(ntStatus))
	{
		//ע�⣬�������������������������ֱ���˳�����Ϊ������豸�����Ѿ�������
		//Ҫ��������豸�����������
		IoDeleteDevice(pDeviceObject);
		DbgPrint("IoCreateSymbolicLink failed: %x\n", ntStatus);
		return ntStatus;
	}

	//��ʼ�����зַ�����ΪDispatchCommon
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
	{
		pDriverObject->MajorFunction[i] = DispatchCommon;
	}

	//����create��read��write��clean��close��iocontrol�ķַ�����
	pDriverObject->MajorFunction[IRP_MJ_CREATE]			= DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_READ]			= DispatchRead;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP]		= DispatchClean;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]			= DispatchClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIocontrol;

	//����������ж�غ���
	pDriverObject->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}