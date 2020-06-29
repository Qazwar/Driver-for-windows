#include <ntddk.h>

typedef struct _FILE_LIST_ENTRY {

	LIST_ENTRY Entry;
	PWSTR NameBuffer;

} FILE_LIST_ENTRY, *PFILE_LIST_ENTRY;

typedef struct _FILE_DIRECTORY_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

NTSTATUS ZwQueryDirectoryFile(
	__in HANDLE  FileHandle,
	__in_opt HANDLE  Event,
	__in_opt PIO_APC_ROUTINE  ApcRoutine,
	__in_opt PVOID  ApcContext,
	__out PIO_STATUS_BLOCK  IoStatusBlock,
	__out PVOID  FileInformation,
	__in ULONG  Length,
	__in FILE_INFORMATION_CLASS  FileInformationClass,
	__in BOOLEAN  ReturnSingleEntry,
	__in_opt PUNICODE_STRING  FileName,
	__in BOOLEAN  RestartScan
);

NTSTATUS dfDeleteFile(const WCHAR *fileName)
{
	OBJECT_ATTRIBUTES                	objAttributes = { 0 };
	IO_STATUS_BLOCK                    	iosb = { 0 };
	HANDLE                           	handle = NULL;
	FILE_DISPOSITION_INFORMATION    	disInfo = { 0 };
	UNICODE_STRING						uFileName = { 0 };
	NTSTATUS                        	status = 0;

	RtlInitUnicodeString(&uFileName, fileName);

	InitializeObjectAttributes(&objAttributes, &uFileName,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateFile(
		&handle,
		SYNCHRONIZE | FILE_WRITE_DATA | DELETE,
		&objAttributes,
		&iosb,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
		NULL,
		0);
	if (!NT_SUCCESS(status))
	{
		if (status == STATUS_ACCESS_DENIED)
		{
			status = ZwCreateFile(
				&handle,
				SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
				&objAttributes,
				&iosb,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0);
			if (NT_SUCCESS(status))
			{
				FILE_BASIC_INFORMATION        basicInfo = { 0 };

				status = ZwQueryInformationFile(handle, &iosb,
					&basicInfo, sizeof(basicInfo), FileBasicInformation);
				if (!NT_SUCCESS(status))
				{
					KdPrint(("ZwQueryInformationFile(%wZ) failed(%x)\n", &uFileName, status));
				}

				basicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
				status = ZwSetInformationFile(handle, &iosb,
					&basicInfo, sizeof(basicInfo), FileBasicInformation);
				if (!NT_SUCCESS(status))
				{
					KdPrint(("ZwSetInformationFile(%wZ) failed(%x)\n", &uFileName, status));
				}

				ZwClose(handle);
				status = ZwCreateFile(
					&handle,
					SYNCHRONIZE | FILE_WRITE_DATA | DELETE,
					&objAttributes,
					&iosb,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					FILE_OPEN,
					FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
					NULL,
					0);
			}
		}

		if (!NT_SUCCESS(status))
		{
			KdPrint(("ZwCreateFile(%wZ) failed(%x)\n", &uFileName, status));
			return status;
		}
	}

	disInfo.DeleteFile = TRUE;
	status = ZwSetInformationFile(handle, &iosb,
		&disInfo, sizeof(disInfo), FileDispositionInformation);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwSetInformationFile(%wZ) failed(%x)\n", &uFileName, status));
	}

	ZwClose(handle);
	return status;
}

NTSTATUS dfDeleteDirectory(const WCHAR * directory)
{
	OBJECT_ATTRIBUTES                	objAttributes = { 0 };
	IO_STATUS_BLOCK                    	iosb = { 0 };
	HANDLE                            	handle = NULL;
	FILE_DISPOSITION_INFORMATION    	disInfo = { 0 };
	PVOID                            	buffer = NULL;
	ULONG                            	bufferLength = 0;
	BOOLEAN                            	restartScan = FALSE;
	PFILE_DIRECTORY_INFORMATION        	dirInfo = NULL;
	PWSTR                            	nameBuffer = NULL;	//��¼�ļ���
	UNICODE_STRING                    	nameString = { 0 };
	NTSTATUS                        	status = 0;
	LIST_ENTRY                        	listHead = { 0 };	//�����������ɾ�������е�Ŀ¼
	PFILE_LIST_ENTRY                	tmpEntry = NULL;	//������
	PFILE_LIST_ENTRY                	preEntry = NULL;
	UNICODE_STRING						uDirName = { 0 };

	RtlInitUnicodeString(&uDirName, directory);

	nameBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, uDirName.Length + sizeof(WCHAR), 'DRID');
	if (!nameBuffer)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	tmpEntry = (PFILE_LIST_ENTRY)ExAllocatePoolWithTag(PagedPool, sizeof(FILE_LIST_ENTRY), 'DRID');
	if (!tmpEntry)
	{
		ExFreePool(nameBuffer);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyMemory(nameBuffer, uDirName.Buffer, uDirName.Length);
	nameBuffer[uDirName.Length / sizeof(WCHAR)] = L'\0';

	InitializeListHead(&listHead);	//��ʼ������
	tmpEntry->NameBuffer = nameBuffer;
	InsertHeadList(&listHead, &tmpEntry->Entry);	//��Ҫɾ�����ļ������Ȳ�������   

	//listHead���ʼ��ΪҪɾ�����ļ��С�
	//֮������ļ����µ��ļ���Ŀ¼���ж����ļ���������ɾ�����ж���Ŀ¼����Ž�listHead����
	//ÿ�ζ���listHead���ó�һ��Ŀ¼������
	while (!IsListEmpty(&listHead))
	{

		//�Ƚ�Ҫɾ�����ļ��к�֮ǰ����ɾ�����ļ��бȽ�һ�£������������ȡ�����Ļ���֮ǰ��Entry������û��ɾ���ɹ���˵������ǿ�
		//�����Ѿ��ɹ�ɾ���������������������߻������ļ��У���ǰ�棬Ҳ��������������
		tmpEntry = (PFILE_LIST_ENTRY)RemoveHeadList(&listHead);
		if (preEntry == tmpEntry)
		{
			status = STATUS_DIRECTORY_NOT_EMPTY;
			break;
		}

		preEntry = tmpEntry;
		InsertHeadList(&listHead, &tmpEntry->Entry); //�Ž�ȥ����ɾ������������ݣ����Ƴ�������Ƴ�ʧ�ܣ���˵���������ļ��л���Ŀ¼�ǿ�

		RtlInitUnicodeString(&nameString, tmpEntry->NameBuffer);
		InitializeObjectAttributes(&objAttributes, &nameString,
			OBJ_CASE_INSENSITIVE, NULL, NULL);
		//���ļ��У����в�ѯ
		status = ZwCreateFile(
			&handle,
			FILE_ALL_ACCESS,
			&objAttributes,
			&iosb,
			NULL,
			0,
			0,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("ZwCreateFile(%ws) failed(%x)\n", tmpEntry->NameBuffer, status));
			break;
		}
		//�ӵ�һ��ɨ��
		restartScan = TRUE;
		while (TRUE)
		{

			buffer = NULL;
			bufferLength = 64;
			status = STATUS_BUFFER_OVERFLOW;

			while ((status == STATUS_BUFFER_OVERFLOW) || (status == STATUS_INFO_LENGTH_MISMATCH))
			{
				if (buffer)
				{
					ExFreePool(buffer);
				}

				bufferLength *= 2;
				buffer = ExAllocatePoolWithTag(PagedPool, bufferLength, 'DRID');
				if (!buffer)
				{
					KdPrint(("ExAllocatePool failed\n"));
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}

				status = ZwQueryDirectoryFile(handle, NULL, NULL,
					NULL, &iosb, buffer, bufferLength, FileDirectoryInformation,
					FALSE, NULL, restartScan);
			}

			if (status == STATUS_NO_MORE_FILES)
			{
				ExFreePool(buffer);
				status = STATUS_SUCCESS;
				break;
			}

			restartScan = FALSE;

			if (!NT_SUCCESS(status))
			{
				KdPrint(("ZwQueryDirectoryFile(%ws) failed(%x)\n", tmpEntry->NameBuffer, status));
				if (buffer)
				{
					ExFreePool(buffer);
				}
				break;
			}

			dirInfo = (PFILE_DIRECTORY_INFORMATION)buffer;

			nameBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool,
				wcslen(tmpEntry->NameBuffer) * sizeof(WCHAR) + dirInfo->FileNameLength + 4, 'DRID');
			if (!nameBuffer)
			{
				KdPrint(("ExAllocatePool failed\n"));
				ExFreePool(buffer);
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			//tmpEntry->NameBuffer�ǵ�ǰ�ļ���·��
			//����Ĳ�����ƴ���ļ���������ļ�·��

			RtlZeroMemory(nameBuffer, wcslen(tmpEntry->NameBuffer) * sizeof(WCHAR) + dirInfo->FileNameLength + 4);
			wcscpy(nameBuffer, tmpEntry->NameBuffer);
			wcscat(nameBuffer, L"\\");
			RtlCopyMemory(&nameBuffer[wcslen(nameBuffer)], dirInfo->FileName, dirInfo->FileNameLength);
			RtlInitUnicodeString(&nameString, nameBuffer);

			if (dirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//����Ƿ�'.'��'..'���������Ŀ¼����Ŀ¼����listHead
				if ((dirInfo->FileNameLength == sizeof(WCHAR)) && (dirInfo->FileName[0] == L'.'))
				{

				}
				else if ((dirInfo->FileNameLength == sizeof(WCHAR) * 2) &&
					(dirInfo->FileName[0] == L'.') &&
					(dirInfo->FileName[1] == L'.'))
				{
				}
				else
				{
					//���ļ��в���listHead��
					PFILE_LIST_ENTRY localEntry;
					localEntry = (PFILE_LIST_ENTRY)ExAllocatePoolWithTag(PagedPool, sizeof(FILE_LIST_ENTRY), 'DRID');
					if (!localEntry)
					{
						KdPrint(("ExAllocatePool failed\n"));
						ExFreePool(buffer);
						ExFreePool(nameBuffer);
						status = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}

					localEntry->NameBuffer = nameBuffer;
					nameBuffer = NULL;
					InsertHeadList(&listHead, &localEntry->Entry); //����ͷ�����Ȱ����ļ������ɾ��
				}
			}
			else
			{
				//�ļ���ֱ��ɾ��
				status = dfDeleteFile(nameBuffer);
				if (!NT_SUCCESS(status))
				{
					KdPrint(("dfDeleteFile(%wZ) failed(%x)\n", &nameString, status));
					ExFreePool(buffer);
					ExFreePool(nameBuffer);
					break;
				}
			}

			ExFreePool(buffer);
			if (nameBuffer)
			{
				ExFreePool(nameBuffer);
			}//������ѭ���ﴦ����һ�����ļ��������ļ���
		}//  while (TRUE) ��һֱŪĿ¼����ļ����ļ���

		if (NT_SUCCESS(status))
		{
			//��ɾ��Ŀ¼
			disInfo.DeleteFile = TRUE;
			status = ZwSetInformationFile(handle, &iosb,
				&disInfo, sizeof(disInfo), FileDispositionInformation);
			if (!NT_SUCCESS(status))
			{
				UNICODE_STRING uCompStr = { 0x00 };
				RtlInitUnicodeString(&uCompStr, tmpEntry->NameBuffer);
				if (RtlCompareUnicodeString(&uDirName, &uCompStr, TRUE) != 0)
				{
					KdPrint(("ZwSetInformationFile(%ws) failed(%x)\n", tmpEntry->NameBuffer, status));
				}

			}
		}

		ZwClose(handle);

		if (NT_SUCCESS(status))
		{
			//ɾ���ɹ������������Ƴ���Ŀ¼
			RemoveEntryList(&tmpEntry->Entry);
			ExFreePool(tmpEntry->NameBuffer);
			ExFreePool(tmpEntry);
		}
		//���ʧ�ܣ���������ļ��л������ļ��У�������ɾ�����ļ���
	}// while (!IsListEmpty(&listHead)) 

	while (!IsListEmpty(&listHead))
	{
		tmpEntry = (PFILE_LIST_ENTRY)RemoveHeadList(&listHead);
		ExFreePool(tmpEntry->NameBuffer);
		ExFreePool(tmpEntry);
	}

	return status;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("DriverUnload...\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	UNREFERENCED_PARAMETER(pRegPath);

	DbgPrint("DriverEntry...\n");

	dfDeleteDirectory(L"\\??\\c:\\123");

	return STATUS_SUCCESS;
}