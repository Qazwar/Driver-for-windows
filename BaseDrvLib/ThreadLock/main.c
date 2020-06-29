#include <ntddk.h>
#include "lock.h"

/*
 * ����2���̣߳�ʹ��FAST_MUTEXʵ���̻߳���
 */

ULONG g_ulTotal = 0;
FAST_MUTEX g_fmLock;

VOID ThreadProc1(IN PVOID pContext)
{
	ULONG  i = 10;

	while (i)
	{
		ExAcquireFastMutex(&g_fmLock);

		g_ulTotal++;

		DbgPrint("ThreadProc1:%x\n", g_ulTotal);

		ExReleaseFastMutex(&g_fmLock);

		i--;
	}
}

VOID ThreadProc2(IN PVOID pContext)
{
	ULONG i = 10;

	while (i)
	{
		ExAcquireFastMutex(&g_fmLock);

		g_ulTotal++;

		DbgPrint("ThreadProc2:%x\n", g_ulTotal);

		ExReleaseFastMutex(&g_fmLock);

		i--;
	}
}
void StartThreads()
{
	HANDLE hThread1 = NULL;
	HANDLE hThread2 = NULL;

	PVOID pThreadFileObjects[2] = { NULL };

	NTSTATUS ntStatus = PsCreateSystemThread(&hThread1,
		0,
		NULL,
		(HANDLE)
		0,
		NULL,
		ThreadProc1,
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		return;
	}

	ntStatus = PsCreateSystemThread(&hThread2,
		0,
		NULL,
		(HANDLE)
		0,
		NULL,
		ThreadProc2,
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		return;
	}

	if ((KeGetCurrentIrql()) != PASSIVE_LEVEL)
	{
		ntStatus = KfRaiseIrql(PASSIVE_LEVEL);
	}
	if ((KeGetCurrentIrql()) != PASSIVE_LEVEL)
	{
		return;
	}

	ntStatus = ObReferenceObjectByHandle(hThread1,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[0],
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		return;
	}

	ntStatus = ObReferenceObjectByHandle(hThread1,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[1],
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		ObDereferenceObject(pThreadFileObjects[0]);
		return;
	}

	KeWaitForMultipleObjects(2,
		pThreadFileObjects,
		WaitAll,
		Executive,
		KernelMode,
		FALSE,
		NULL,
		NULL);

	ObDereferenceObject(pThreadFileObjects[0]);
	ObDereferenceObject(pThreadFileObjects[1]);

	// KeWaitForSingleObject(objtowait, Executive, KernelMode, FALSE, NULL);
	return;
}

/*
 * ����2���̣߳�ʹ��KSEMAPHOREʵ����Դͬ������������������ģ��
 */

KSEMAPHORE g_Ksemaphore;
ULONG g_uTotalNums = 0;

VOID WorkerFunc(PVOID pContext)
{
	ULONG i = 0;
	//������ʾ�����Ľṹ
	LARGE_INTEGER timeOut = { 0x00 };
	timeOut.QuadPart = -3 * 10000000i64;

	while (i < 10)
	{
		//����ǰ�̴߳ӵȴ�������Դ�б����Ƴ��������ź���g_Ksemaphore+1���൱������һ��Ԫ��
		KeReleaseSemaphore(&g_Ksemaphore, IO_NO_INCREMENT, 1, FALSE);

		g_uTotalNums++;
		DbgPrint("WorkerFunc:g_uTotalNums is %d\n", g_uTotalNums);
		i++;

		//�ӳ�3��
		KeDelayExecutionThread(KernelMode, FALSE, &timeOut);
	}
}

VOID ConSumerFunc(PVOID pContext)
{
	ULONG i = 0;
	LARGE_INTEGER timeOut = { 0x00 };
	timeOut.QuadPart = -3 * 10000000i64;

	while (i < 10)
	{
		//����ǰ�̼߳��뵽�ȴ���Դ�б��У������ź���g_Ksemaphore-1���൱������һ��Ԫ��
		KeWaitForSingleObject(&g_Ksemaphore,
			Executive,
			KernelMode,
			FALSE,
			&timeOut);

		g_uTotalNums--;
		DbgPrint("ConSumerFunc:g_uTotalNums is %d\n", g_uTotalNums);
		i++;

		KeDelayExecutionThread(KernelMode, FALSE, &timeOut);

	}
}

VOID StartThread2(VOID)
{
	HANDLE hWorkerThreadHandle = NULL;
	HANDLE hConSumerThreadHandle = NULL;
	PVOID pThreadFileObjects[2] = { NULL };
	NTSTATUS ntStatus;

	ntStatus = PsCreateSystemThread(&hWorkerThreadHandle,
		0,
		NULL,
		(HANDLE)0,
		NULL,
		WorkerFunc,
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PsCreateSystemThread Worker faild:%x\n", ntStatus);
		return;
	}

	ntStatus = PsCreateSystemThread(&hConSumerThreadHandle,
		0,
		NULL,
		(HANDLE)0,
		NULL,
		ConSumerFunc,
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("PsCreateSystemThread ConSumer faild:%x\n", ntStatus);
		return;
	}

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		ntStatus = KfRaiseIrql(PASSIVE_LEVEL);
	}
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		DbgPrint("KfRaiseIrql faild:%x\n", ntStatus);
		return;
	}

	ntStatus = ObReferenceObjectByHandle(hWorkerThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[0],
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObReferenceObjectByHandle Work faild:%x\n", ntStatus);
		return;
	}

	ntStatus = ObReferenceObjectByHandle(hConSumerThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&pThreadFileObjects[1],
		NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObReferenceObjectByHandle Consumer faild:%x\n", ntStatus);
		ObDereferenceObject(pThreadFileObjects[0]);
		return;
	}

	ntStatus = KeWaitForMultipleObjects(2,
		pThreadFileObjects,
		WaitAll,
		Executive,
		KernelMode,
		FALSE,
		NULL,
		NULL);

	ObDereferenceObject(pThreadFileObjects[0]);
	ObDereferenceObject(pThreadFileObjects[1]);

	return;
}

/*
 * KSPIN_LOCK ������
 * ���CPU����ȫ������IRQL��DPC����ֹ���ʷ�ҳ�ڴ桢���ʱ��Խ��Խ��
 *
 * KIRQL oldIrql;
 * KSPIN_LOCK spinLock;
 *
 * KeAcquireSpinLock(&spinLock, &oldIrql);
 *
 * ...
 *
 * KeReleaseSpinLock(&spinLock, oldIrql);
 *
 */

/* 
 * ERESOURCE ��д������
 *
 *  ERESOURCE  g_OperListLock;		ȫ�ֶ�����
 *
 *	InitLock(&g_OperListLock);		DriverEntry��ʼ��
 *
 *	LockWrite(&g_OperListLock);		д��������ǰ����
 *
 *	UnLockWrite(&g_OperListLock);	д����ͷ�
 *
 *	DeleteLock(&g_OperListLock);	ж��������ɾ����
 *
 */

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("DriverUnload...\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	UNREFERENCED_PARAMETER(pRegPath);

	DbgPrint("DriverEntry...\n");

	pDriverObject->DriverUnload = DriverUnload;

	//ExInitializeFastMutex(&g_fmLock);

	//StartThreads();

	KeInitializeSemaphore(&g_Ksemaphore, 0, 10);

	StartThread2();

	return STATUS_SUCCESS;
}
