#include <ntddk.h>
#include "./hash/hash.h"

#define		HASH_TABLE_SIZE	100

PHASHTABLE g_pHashTable = NULL; //HASH��
ERESOURCE  g_HashTableLock;

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("DriverUnload...\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	UNREFERENCED_PARAMETER(pRegPath);

	DbgPrint("DriverEntry...\n");

	pDriverObject->DriverUnload = DriverUnload;

	/*
		// ��ʼ������hash��
		g_pHashTable = InitializeTable(HASH_TABLE_SIZE);
		if (g_pHashTable == NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		InitLock(&g_HashTableLock);

		LockWrite(&g_HashTableLock);
		Insert((DWORD)lpFileObject, lpData, g_pHashTable);
		UnLockWrite(&g_HashTableLock);

		lpTwoWay = Find((DWORD)lpFileObject, g_pHashTable);

		LockWrite(&g_HashTableLock);
		Remove((DWORD)lpFileObject, g_pHashTable);
		UnLockWrite(&g_HashTableLock);

		// �ͷ�hash��

		DeleteLock(&g_HashTableLock);	//ж��������ɾ����

		if (g_pHashTable)
		{
			DestroyTable(g_pHashTable);
		}	
	*/



	return STATUS_SUCCESS;
}