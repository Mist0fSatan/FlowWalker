#include "pin.H"
#include <string>
#include <vector>
#include <set>
#define BLOCK_SZ 0x20000000
#define TAILBLANK_SZ 1024
//#define LOGTIME

namespace WINDOWS
{
	//����ͷ�ļ��������涨���UINT�Ȼ������ͺ�Pin�µ�ͬ�����ͳ�ͻ��������Ҫ����
#include <Windows.h>

	//�����Ƿ���ϵͳ����ʱ��Ҫ�õ������ݽṹ
	typedef struct _UNICODE_STRING
	{
		USHORT  Length;
		USHORT  MaximumLength;
		PWSTR   Buffer;
	} UNICODE_STRING, *PUNICODE_STRING;

	typedef struct _IO_STATUS_BLOCK
	{
		union
		{
			NTSTATUS Status;
			PVOID Pointer;
		};
		ULONG_PTR Information;
	} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

	typedef struct _OBJECT_ATTRIBUTES
	{
		ULONG  Length;
		HANDLE  RootDirectory;
		PUNICODE_STRING  ObjectName;
		ULONG  Attributes;
		PVOID  SecurityDescriptor;
		PVOID  SecurityQualityOfService;
	} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

	//typedef union _LARGE_INTEGER {
	//	struct {
	//		DWORD LowPart;
	//		LONG  HighPart;
	//	};
	//	struct {
	//		DWORD LowPart;
	//		LONG  HighPart;
	//	} u;
	//	LONGLONG QuadPart;
	//} LARGE_INTEGER, *PLARGE_INTEGER;

	typedef struct _FILE_POSITION_INFORMATION {
		LARGE_INTEGER CurrentByteOffset;
	} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

	typedef enum _FILE_INFORMATION_CLASS { 
		FileDirectoryInformation                  = 1,
		FileFullDirectoryInformation,
		FileBothDirectoryInformation,
		FileBasicInformation,
		FileStandardInformation,
		FileInternalInformation,
		FileEaInformation,
		FileAccessInformation,
		FileNameInformation,
		FileRenameInformation,
		FileLinkInformation,
		FileNamesInformation,
		FileDispositionInformation,
		FilePositionInformation,
		FileFullEaInformation,
		FileModeInformation,
		FileAlignmentInformation,
		FileAllInformation,
		FileAllocationInformation,
		FileEndOfFileInformation,
		FileAlternateNameInformation,
		FileStreamInformation,
		FilePipeInformation,
		FilePipeLocalInformation,
		FilePipeRemoteInformation,
		FileMailslotQueryInformation,
		FileMailslotSetInformation,
		FileCompressionInformation,
		FileObjectIdInformation,
		FileCompletionInformation,
		FileMoveClusterInformation,
		FileQuotaInformation,
		FileReparsePointInformation,
		FileNetworkOpenInformation,
		FileAttributeTagInformation,
		FileTrackingInformation,
		FileIdBothDirectoryInformation,
		FileIdFullDirectoryInformation,
		FileValidDataLengthInformation,
		FileShortNameInformation,
		FileIoCompletionNotificationInformation,
		FileIoStatusBlockRangeInformation,
		FileIoPriorityHintInformation,
		FileSfioReserveInformation,
		FileSfioVolumeInformation,
		FileHardLinkInformation,
		FileProcessIdsUsingFileInformation,
		FileNormalizedNameInformation,
		FileNetworkPhysicalNameInformation,
		FileIdGlobalTxDirectoryInformation,
		FileIsRemoteDeviceInformation,
		FileAttributeCacheInformation,
		FileNumaNodeInformation,
		FileStandardLinkInformation,
		FileRemoteProtocolInformation,
		FileMaximumInformation 
	} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;
};

extern ofstream fOutput1;
extern WINDOWS::LPBYTE proc;
extern WINDOWS::HANDLE hFileMapping;
extern WINDOWS::DWORD offset;
extern unsigned __int64 offset_p;
LEVEL_BASE::BOOL bActive = FALSE;	//��װ������־
extern PIN_LOCK lock;
extern THREADID CurTid;
extern set<UINT> InstTid;
//extern string lastBBL;
#ifdef LOGTIME
extern WINDOWS::SYSTEMTIME StartTime;
#endif

//�����ļ������ϵͳ���õı���
UINT num_create = 0, num_open = 0, num_read = 0, num_close = 0, num_move = 0;
UINT num_createsection = 0, num_mapviewofsection = 0, num_unmapviewofsection = 0;
UINT num_allocmem = 0, num_freemem = 0;
UINT num_create_64 = 0, num_open_64 = 0, num_read_64 = 0, num_close_64 = 0, num_move_64 = 0;
UINT num_createsection_64 = 0, num_mapviewofsection_64 = 0, num_unmapviewofsection_64 = 0;
UINT num_allocmem_64 = 0, num_freemem_64 = 0;
wstring wstrTraceFileName;
LEVEL_BASE::BOOL MonitorExit = FALSE;
BOOL JustAfterVMem = FALSE;
ADDRINT* VMemBaseAddr = 0;
UINT* VMemSize = 0;


class FileStatus
{
public:
	UINT32 pFileHandle;							//�ļ����ָ��
	UINT32 FileHandle;							//�ļ����
	UINT32 pFileMappingHandle;					//�ļ�Mapping���ָ��
	UINT32 FileMappingHandle;					//�ļ�Mapping���
	LEVEL_BASE::BOOL JustAfterCreate;			//�����-����
	LEVEL_BASE::BOOL JustAfterRead;				//�����-��ȡ
	LEVEL_BASE::BOOL JustAfterCreateSection;	//�����-����Section
	LEVEL_BASE::BOOL JustAfterMapViewOfSection;	//�����-����MapView
	WINDOWS::PIO_STATUS_BLOCK pio_status;		//״̬�ṹ���ں��ļ���С��Ϣ
	ADDRINT InitTaintBuffer;					//�ļ���������ַ
	ADDRINT InitTaintBufferMapping;				//�ļ�Mapping��������ַ
	ADDRINT pInitTaintBufferMapping;			//�ļ�Mapping��������ַָ��	
	WINDOWS::LARGE_INTEGER FileOffset;			//�ļ�ƫ����
	WINDOWS::LARGE_INTEGER FileMappingOffset;	//�ļ�Mappingƫ����
	WINDOWS::PLARGE_INTEGER pFileMappingOffset;	//�ļ�Mappingƫ����
	WINDOWS::SIZE_T FileMappingSize;			//�ļ�Mapping��С
	WINDOWS::PSIZE_T pFileMappingSize;			//�ļ�Mapping��Сָ��

	//ƫ�������ļ���С�ͻ�������ַ�Ĺ�ϵ��
	//ϵͳ�����ļ�ƫ������offset�����ģ�Size���ļ���С�����ݶ��뻺������ַ��TaintBuffer��

	/*���캯��*/
	FileStatus():pFileHandle(NULL),FileHandle(NULL),pFileMappingHandle(NULL),FileMappingHandle(NULL),
		JustAfterCreate(FALSE),JustAfterRead(FALSE),JustAfterCreateSection(FALSE),JustAfterMapViewOfSection(FALSE)
	{
		FileOffset.QuadPart = 0;
	}
};
vector<FileStatus*> FileStatusList;

inline VOID String2WString ( const string szStr, wstring& wszStr )
{
	int nLength = WINDOWS::MultiByteToWideChar ( CP_ACP, 0, szStr.c_str(), -1, NULL, NULL );
	WINDOWS::LPWSTR lpwszStr = new wchar_t[nLength];
	WINDOWS::MultiByteToWideChar ( CP_ACP, 0, szStr.c_str(), -1, lpwszStr, nLength );
	wszStr = lpwszStr;
	delete[] lpwszStr;
}

VOID SyscallEntry (THREADID tid, LEVEL_VM::CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
	if (!InstTid.empty() && InstTid.count(tid)==0)
		return;
	ADDRINT syscallnum = PIN_GetSyscallNumber (ctxt, std);
	//��ȡϵͳ���úţ������NtCreateFile��
	if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && (syscallnum == num_create /*&& pFileHandle == NULL*/ || syscallnum == num_open)
		|| std==SYSCALL_STANDARD_WOW64 && (syscallnum == num_create_64 /*&& pFileHandle == NULL*/ || syscallnum == num_open_64))
		/*NTSTATUS ZwCreateFile(
		_Out_     PHANDLE FileHandle,
		_In_      ACCESS_MASK DesiredAccess,
		_In_      POBJECT_ATTRIBUTES ObjectAttributes,
		_Out_     PIO_STATUS_BLOCK IoStatusBlock,
		_In_opt_  PLARGE_INTEGER AllocationSize,
		_In_      ULONG FileAttributes,
		_In_      ULONG ShareAccess,
		_In_      ULONG CreateDisposition,
		_In_      ULONG CreateOptions,
		_In_opt_  PVOID EaBuffer,
		_In_      ULONG EaLength
		);*/
	{
		WINDOWS::POBJECT_ATTRIBUTES pobj_attr
			= (WINDOWS::POBJECT_ATTRIBUTES) PIN_GetSyscallArgument (ctxt, std, 2);
		//�õ��򿪵��ļ���
		wstring str = pobj_attr->ObjectName->Buffer;
		wstring str_file = str.substr (str.find_last_of ('\\') + 1);
		//�жϴ򿪵��ļ������ص��۵�Դ�ļ����Ƿ���ͬ
		if (wstrTraceFileName == str_file)
		{
			//�����ļ����ָ�룬��ָʾ�ļ�������ɣ���ʾϵͳ���ó��ں�������
			FileStatus *nstatus = new FileStatus();
			nstatus->pFileHandle = PIN_GetSyscallArgument (ctxt, std, 0);
			nstatus->JustAfterCreate = TRUE;

			FileStatusList.push_back(nstatus);
			MonitorExit = TRUE;
		}
	}
	//�����NtReadFile
	else if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && syscallnum == num_read
		|| std==SYSCALL_STANDARD_WOW64 && syscallnum == num_read_64)
		/*NTSTATUS ZwReadFile(
		_In_      HANDLE FileHandle,
		_In_opt_  HANDLE Event,
		_In_opt_  PIO_APC_ROUTINE ApcRoutine,
		_In_opt_  PVOID ApcContext,
		_Out_     PIO_STATUS_BLOCK IoStatusBlock,
		_Out_     PVOID Buffer,
		_In_      ULONG Length,
		_In_opt_  PLARGE_INTEGER ByteOffset,
		_In_opt_  PULONG Key
		);*/
	{
		UINT fHandleNum=FileStatusList.size();
		bool IsTgt=false;
//		for (vector<FileStatus*>::iterator i=FileStatusList.begin(); i!=FileStatusList.end(); i++)
		for (UINT ti=0; ti!=fHandleNum; ++ti)
		{
			FileStatus* tFileStatus=FileStatusList[ti];
			//�ж��ļ������֮ǰNtCreateFile�õ��ľ��
			if (tFileStatus->FileHandle && PIN_GetSyscallArgument (ctxt, std, 0) == tFileStatus->FileHandle)
			{
				tFileStatus->JustAfterRead = TRUE;
				//�õ��ļ����뵽�ڴ��еĻ�������ַ
				tFileStatus->InitTaintBuffer = PIN_GetSyscallArgument (ctxt, std, 5);
				//�õ�ָ���ļ�ƫ������ָ��
				WINDOWS::PLARGE_INTEGER pByteOffset = (WINDOWS::PLARGE_INTEGER)PIN_GetSyscallArgument (ctxt, std, 7);
				if (pByteOffset!=NULL)
				{
					WINDOWS::LARGE_INTEGER ByteOffset = *pByteOffset;
					if (ByteOffset.HighPart!=-1)// || ByteOffset.LowPart!=WINDOWS::FILE_USE_FILE_POINTER_POSITION)
					{
						tFileStatus->FileOffset=ByteOffset;
					}
				}
				//�õ�ָ�����Ĵ�С�ṹָ��
				tFileStatus->pio_status = (WINDOWS::PIO_STATUS_BLOCK) (PIN_GetSyscallArgument (ctxt, std, 4) );

				MonitorExit = TRUE;
				IsTgt=true;
				break;
			}
		}
		if (!IsTgt)
		{
			PIN_GetLock(&lock, tid+1);
			string temp = "U " + hexstr(PIN_GetSyscallArgument (ctxt, std, 5),8).substr(2) + " "
				+ hexstr(PIN_GetSyscallArgument (ctxt, std, 6)).substr(2) + "\n";
			int tlen = temp.length();
			memcpy(proc+offset, temp.c_str(), tlen);
			offset+=tlen;

			//����ǰ�ڴ�ӳ���ļ���СԽ�磬�����´����ڴ�ӳ���ļ�
			if (offset > BLOCK_SZ - TAILBLANK_SZ)
			{
				//					WINDOWS::FlushViewOfFile(proc,offset);
				WINDOWS::UnmapViewOfFile(proc);

				offset_p += BLOCK_SZ;
				offset = 0;
				proc = (WINDOWS::LPBYTE)WINDOWS::MapViewOfFile(hFileMapping, FILE_MAP_WRITE,
					(WINDOWS::DWORD)(offset_p>>32),(WINDOWS::DWORD)(offset_p&0xFFFFFFFF),BLOCK_SZ);
				if (proc == NULL)
				{
					exit(0);
				}
			}
			PIN_ReleaseLock(&lock);
		}
	}
	//������ļ��رյ���
	else if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && syscallnum == num_close
		|| std==SYSCALL_STANDARD_WOW64 && syscallnum == num_close_64)
	{
		UINT fHandleNum=FileStatusList.size();
//		for (vector<FileStatus*>::iterator i=FileStatusList.begin(); i!=FileStatusList.end(); i++)
		for (UINT ti=0; ti!=fHandleNum; ++ti)
		{
			FileStatus* tFileStatus=FileStatusList[ti];
			//�ж��ļ������֮ǰNtCreateFile�õ��ľ��
			if (tFileStatus->FileHandle && PIN_GetSyscallArgument (ctxt, std, 0) == tFileStatus->FileHandle)
			{
				//ɾ���ļ����
				if (tFileStatus->FileMappingHandle == NULL)
					FileStatusList.erase(FileStatusList.begin()+ti);
				else
					tFileStatus->FileHandle = NULL;
			}
			//�ж�Mapping�ļ������֮ǰNtCreateSection�õ��ľ��
			else if (tFileStatus->FileMappingHandle && PIN_GetSyscallArgument (ctxt, std, 0) == tFileStatus->FileMappingHandle)
			{
				//ɾ��Maping�ļ����
				if (tFileStatus->FileHandle == NULL)
					FileStatusList.erase(FileStatusList.begin()+ti);
				else
					tFileStatus->FileMappingHandle = NULL;
			}
			else
				continue;
			break;
		}
	}
	//������ļ��ƶ�����
	else if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && syscallnum == num_move
		|| std==SYSCALL_STANDARD_WOW64 && syscallnum == num_move_64)
		/*NTSTATUS ZwSetInformationFile(
		_In_   HANDLE FileHandle,
		_Out_  PIO_STATUS_BLOCK IoStatusBlock,
		_In_   PVOID FileInformation,
		_In_   ULONG Length,
		_In_   FILE_INFORMATION_CLASS FileInformationClass
		);*/
	{
		for (vector<FileStatus*>::iterator i=FileStatusList.begin(); i!=FileStatusList.end(); i++)
		{
			//�ж��ļ������֮ǰNtCreateFile�õ��ľ��
			if ((*i)->FileHandle && PIN_GetSyscallArgument (ctxt, std, 0) == (*i)->FileHandle)
			{
				//�õ��µ��ļ�ƫ����
				if ( (WINDOWS::FILE_INFORMATION_CLASS) PIN_GetSyscallArgument (ctxt, std, 4) == WINDOWS::FilePositionInformation)
				{
					(*i)->FileOffset = ((WINDOWS::PFILE_POSITION_INFORMATION) PIN_GetSyscallArgument (ctxt, std, 2))->CurrentByteOffset;
				}
				break;
			}
		}		
	}
	//�����CreateSection
	else if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && syscallnum == num_createsection
		|| std==SYSCALL_STANDARD_WOW64 && syscallnum == num_createsection_64)
		/*NTSTATUS ZwCreateSection(
		_Out_     PHANDLE SectionHandle,
		_In_      ACCESS_MASK DesiredAccess,
		_In_opt_  POBJECT_ATTRIBUTES ObjectAttributes,
		_In_opt_  PLARGE_INTEGER MaximumSize,
		_In_      ULONG SectionPageProtection,
		_In_      ULONG AllocationAttributes,
		_In_opt_  HANDLE FileHandle
		);*/
	{
		for (vector<FileStatus*>::iterator i=FileStatusList.begin(); i!=FileStatusList.end(); i++)
		{
			//�ж��ļ������֮ǰNtCreateFile�õ��ľ��
			if ((*i)->FileHandle && PIN_GetSyscallArgument (ctxt, std, 6) == (*i)->FileHandle)
			{
				//�洢Mapping�ļ����
				(*i)->JustAfterCreateSection = TRUE;
				(*i)->pFileMappingHandle = PIN_GetSyscallArgument (ctxt, std, 0);

				MonitorExit = TRUE;
				break;
				/*if (NULL != PIN_GetSyscallArgument (ctxt, std, 3))
					FileMappingSizeMax = *(WINDOWS::PLARGE_INTEGER)PIN_GetSyscallArgument (ctxt, std, 3);
				else
					FileMappingSizeMax.QuadPart = 0;*/
			}
		}		
	}
	//�����MapViewOfSection
	else if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && syscallnum == num_mapviewofsection
		|| std==SYSCALL_STANDARD_WOW64 && syscallnum == num_mapviewofsection_64)
		//NTSTATUS ZwMapViewOfSection(
		//_In_     HANDLE SectionHandle,
		//_In_     HANDLE ProcessHandle,
		//_Inout_  PVOID *BaseAddress,
		//_In_     ULONG_PTR ZeroBits,
		//_In_     SIZE_T CommitSize,
		//_Inout_  PLARGE_INTEGER SectionOffset,
		//_Inout_  PSIZE_T ViewSize,
		//_In_     SECTION_INHERIT InheritDisposition,
		//_In_     ULONG AllocationType,
		//_In_     ULONG Win32Protect
		//);
	{
		bool IsTgt=false;
		for (vector<FileStatus*>::iterator i=FileStatusList.begin(); i!=FileStatusList.end(); i++)
		{
			//�ж�Mapping�ļ������֮ǰCreateSection�õ��ľ��
			if ((*i)->FileMappingHandle && PIN_GetSyscallArgument (ctxt, std, 0) == (*i)->FileMappingHandle)
			{
				(*i)->JustAfterMapViewOfSection = TRUE;
				//�õ�ָ���ļ����뵽�ڴ��еĻ�������ַ��ָ��
				(*i)->pInitTaintBufferMapping = PIN_GetSyscallArgument (ctxt, std, 2);
				//�õ�ָ���ļ�ƫ������ָ��
				(*i)->pFileMappingOffset = (WINDOWS::PLARGE_INTEGER)PIN_GetSyscallArgument (ctxt, std, 5);
				//�õ�ָ��Mapping��С��ָ��
				(*i)->pFileMappingSize = (WINDOWS::PSIZE_T)PIN_GetSyscallArgument (ctxt, std, 6);

				MonitorExit = TRUE;
				IsTgt=true;
				break;
				//			FileMappingSize = *(WINDOWS::PLARGE_INTEGER)PIN_GetSyscallArgument (ctxt, std, 6);
				//			FileMappingSize = *pFileMappingSize;
			}
		}
		if (!IsTgt)
		{
			PIN_GetLock(&lock, tid+1);
			string temp = "U " + hexstr(PIN_GetSyscallArgument (ctxt, std, 2),8).substr(2) + " "
				+ hexstr(PIN_GetSyscallArgument (ctxt, std, 6)).substr(2) + "\n";
			int tlen = temp.length();
			memcpy(proc+offset, temp.c_str(), tlen);
			offset+=tlen;

			//����ǰ�ڴ�ӳ���ļ���СԽ�磬�����´����ڴ�ӳ���ļ�
			if (offset > BLOCK_SZ - TAILBLANK_SZ)
			{
				//					WINDOWS::FlushViewOfFile(proc,offset);
				WINDOWS::UnmapViewOfFile(proc);

				offset_p += BLOCK_SZ;
				offset = 0;
				proc = (WINDOWS::LPBYTE)WINDOWS::MapViewOfFile(hFileMapping, FILE_MAP_WRITE,
					(WINDOWS::DWORD)(offset_p>>32),(WINDOWS::DWORD)(offset_p&0xFFFFFFFF),BLOCK_SZ);
				if (proc == NULL)
				{
					exit(0);
				}
			}
			PIN_ReleaseLock(&lock);
		}
	}
	//�����UmMapViewOfSection
	else if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && syscallnum == num_unmapviewofsection
		|| std==SYSCALL_STANDARD_WOW64 && syscallnum == num_unmapviewofsection_64)
		/*NTSTATUS ZwUnmapViewOfSection(
		_In_      HANDLE ProcessHandle,
		_In_opt_  PVOID BaseAddress
		);*/
	{
		for (vector<FileStatus*>::iterator i=FileStatusList.begin(); i!=FileStatusList.end(); i++)
		{
			//�жϻ�������ַ��֮ǰMapViewOfSection�õ��ĵ�ַ
			if ((*i)->InitTaintBufferMapping && PIN_GetSyscallArgument (ctxt, std, 1) == (*i)->InitTaintBufferMapping)
			{
				PIN_GetLock(&lock, tid+1);
				string temp = "U " + hexstr((*i)->InitTaintBufferMapping,8).substr(2) + " "
					+ hexstr((UINT)((*i)->FileMappingSize)).substr(2) + "\n";
				int tlen = temp.length();
				memcpy(proc+offset, temp.c_str(), tlen);
				offset+=tlen;

				//����ǰ�ڴ�ӳ���ļ���СԽ�磬�����´����ڴ�ӳ���ļ�
				if (offset > BLOCK_SZ - TAILBLANK_SZ)
				{
//					WINDOWS::FlushViewOfFile(proc,offset);
					WINDOWS::UnmapViewOfFile(proc);

					offset_p += BLOCK_SZ;
					offset = 0;
					proc = (WINDOWS::LPBYTE)WINDOWS::MapViewOfFile(hFileMapping, FILE_MAP_WRITE,
						(WINDOWS::DWORD)(offset_p>>32),(WINDOWS::DWORD)(offset_p&0xFFFFFFFF),BLOCK_SZ);
					if (proc == NULL)
					{
						exit(0);
					}
				}
				//ɾ����������ַ��Ϣ
				(*i)->InitTaintBufferMapping = NULL;
				PIN_ReleaseLock(&lock);
				break;
			}
		}		
	}
	//�����Allocate/Free VirtualMemory
	else if (std==SYSCALL_STANDARD_IA32_WINDOWS_FAST && syscallnum == num_allocmem
		|| std==SYSCALL_STANDARD_WOW64 && syscallnum == num_allocmem_64)
	{
		JustAfterVMem = TRUE;
		VMemBaseAddr = (ADDRINT*)PIN_GetSyscallArgument (ctxt, std, 1);
		VMemSize = (UINT*)PIN_GetSyscallArgument (ctxt, std, 3);
		MonitorExit = TRUE;
	}
	else if (syscallnum==num_freemem)
	{
		JustAfterVMem = TRUE;
		VMemBaseAddr = (ADDRINT*)PIN_GetSyscallArgument (ctxt, std, 1);
		VMemSize = (UINT*)PIN_GetSyscallArgument (ctxt, std, 2);
		MonitorExit = TRUE;
	}
}

VOID SyscallExit (THREADID tid, LEVEL_VM::CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
	if (!MonitorExit)
		return;
	if (JustAfterVMem)
	{
		PIN_GetLock(&lock, tid+1);
		string temp = "U " + hexstr(*VMemBaseAddr,8).substr(2) + " "
			+ hexstr(*VMemSize).substr(2) + "\n";
		int tlen = temp.length();
		memcpy(proc+offset, temp.c_str(), tlen);
		offset+=tlen;

		//����ǰ�ڴ�ӳ���ļ���СԽ�磬�����´����ڴ�ӳ���ļ�
		if (offset > BLOCK_SZ - TAILBLANK_SZ)
		{
			//					WINDOWS::FlushViewOfFile(proc,offset);
			WINDOWS::UnmapViewOfFile(proc);

			offset_p += BLOCK_SZ;
			offset = 0;
			proc = (WINDOWS::LPBYTE)WINDOWS::MapViewOfFile(hFileMapping, FILE_MAP_WRITE,
				(WINDOWS::DWORD)(offset_p>>32),(WINDOWS::DWORD)(offset_p&0xFFFFFFFF),BLOCK_SZ);
			if (proc == NULL)
			{
				exit(0);
			}
		}
		PIN_ReleaseLock(&lock);
		MonitorExit=FALSE;
		JustAfterVMem=FALSE;
		return;
	}
	for (vector<FileStatus*>::iterator i=FileStatusList.begin(); i!=FileStatusList.end(); i++)
	{
		if ((*i)->JustAfterCreate)
		{
			//����ָ��õ����ֵ
			(*i)->FileHandle = * (UINT32*) ((*i)->pFileHandle);
			(*i)->JustAfterCreate = FALSE;
		}
		else if ((*i)->JustAfterCreateSection)
		{
			//����ָ��õ����ֵ
			(*i)->FileMappingHandle = * (UINT32*) ((*i)->pFileMappingHandle);
			(*i)->JustAfterCreateSection = FALSE;
		}
		else if ((*i)->JustAfterRead)
		{
			PIN_GetLock(&lock, tid+1);
			//����־�м�¼Դ�۵�������Ϣ
			/*if (!bActive)
			{
				int tlen = lastBBL.length();
				memcpy(proc+offset, lastBBL.c_str(), tlen);
				offset+=tlen;
			}*/

			//�����������ַ��ƫ�������ļ���С����־
			string temp = "T " + hexstr((*i)->InitTaintBuffer,8).substr(2) + " "
				+ hexstr((*i)->FileOffset.QuadPart).substr(2) + " " + hexstr((UINT32)((*i)->pio_status->Information)).substr(2) + "\n";
			int tlen = temp.length();
			memcpy(proc+offset, temp.c_str(), tlen);
			offset+=tlen;

			if (offset > BLOCK_SZ - TAILBLANK_SZ)
			{
//				WINDOWS::FlushViewOfFile(proc,offset);
				WINDOWS::UnmapViewOfFile(proc);

				offset_p += BLOCK_SZ;
				offset = 0;
				proc = (WINDOWS::LPBYTE)WINDOWS::MapViewOfFile(hFileMapping, FILE_MAP_WRITE,
					(WINDOWS::DWORD)(offset_p>>32),(WINDOWS::DWORD)(offset_p&0xFFFFFFFF),BLOCK_SZ);
				if (proc == NULL)
				{
					exit(0);
				}
			}

			(*i)->FileOffset.QuadPart+=(*i)->pio_status->Information;
			(*i)->JustAfterRead = FALSE;

			//��ʼTrace��װ
			if (!bActive)
			{
				PIN_RemoveInstrumentation();
#ifdef LOGTIME
				WINDOWS::GetLocalTime(&StartTime);
#endif
				bActive = TRUE;
			}
			PIN_ReleaseLock(&lock);
		}
		else if ((*i)->JustAfterMapViewOfSection)
		{
			PIN_GetLock(&lock, tid+1);
			//����־�м�¼Դ�۵�������Ϣ
			/*if (!bActive)
			{
				int tlen = lastBBL.length();
				memcpy(proc+offset, lastBBL.c_str(), tlen);
				offset+=tlen;
			}*/
			//����ָ��õ���������С��ƫ����
			(*i)->InitTaintBufferMapping = *(ADDRINT*) ((*i)->pInitTaintBufferMapping);
			(*i)->FileMappingOffset = *((*i)->pFileMappingOffset);

	//		WINDOWS::LARGE_INTEGER FileMappingSizeReal = *pFileMappingSize;
	//		if (FileMappingSize.QuadPart == 0)
	//			FileMappingSize = FileMappingSizeMax;
			//����ָ��õ��ļ�Mapping��С
			(*i)->FileMappingSize = *((*i)->pFileMappingSize);

			//�����������ַ��ƫ�������ļ���С����־
			string temp = "T " + hexstr((*i)->InitTaintBufferMapping,8).substr(2) + " "
				+ hexstr((*i)->FileMappingOffset.QuadPart).substr(2) + " " + hexstr((UINT)((*i)->FileMappingSize)).substr(2) + "\n";
			int tlen = temp.length();
			memcpy(proc+offset, temp.c_str(), tlen);
			offset+=tlen;

			if (offset > BLOCK_SZ - TAILBLANK_SZ)
			{
//				WINDOWS::FlushViewOfFile(proc,offset);
				WINDOWS::UnmapViewOfFile(proc);

				offset_p += BLOCK_SZ;
				offset = 0;
				proc = (WINDOWS::LPBYTE)WINDOWS::MapViewOfFile(hFileMapping, FILE_MAP_WRITE,
					(WINDOWS::DWORD)(offset_p>>32),(WINDOWS::DWORD)(offset_p&0xFFFFFFFF),BLOCK_SZ);
				if (proc == NULL)
				{
					exit(0);
				}
			}

			(*i)->JustAfterMapViewOfSection = FALSE;
//			bActive = TRUE;
			if (!bActive)
			{
				PIN_RemoveInstrumentation();
#ifdef LOGTIME
				WINDOWS::GetLocalTime(&StartTime);
#endif
				bActive = TRUE;
			}
			PIN_ReleaseLock(&lock);
		}
		else
			continue;
		break;
	}
	MonitorExit = FALSE;
}