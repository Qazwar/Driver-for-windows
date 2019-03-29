#pragma once
#include <ntddk.h>

NTSTATUS
PsLookupProcessByProcessId(
	__in HANDLE ProcessId,   //����ID
	__deref_out PEPROCESS *Process //���ص�EPROCESS
);

VOID CreateProcessRoutineEx(IN HANDLE  ParentId, IN HANDLE  ProcessId, IN BOOLEAN  Create);

VOID LoadImageNotifyRoutine(__in_opt PUNICODE_STRING FullImageName, __in HANDLE ProcessId, __in PIMAGE_INFO ImageInfo);

VOID DrivrUnload(PDRIVER_OBJECT pDriverObject);

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegpath);
