//#define _WIN64

#include <Windows.h>
#include <intrin.h>
#include <Psapi.h>
#include <winternl.h>
#include <cstdio>
#include <memory>
#include <type_traits>
#include <string>
#include <typeinfo>
#include <map>

#include "ProcMemWindow.cpp"
bool _init_by_tls()
{
	OutputDebugStringA("tls");
	return true;
}

/*class TLS_Var{
public:
TLS_Var(){
OutputDebugStringA("tls");
}
~TLS_Var()
{
OutputDebugStringA("~tls");
}
};
_declspec(thread) TLS_Var tls_var;*/
_declspec(thread) bool tls_var = _init_by_tls();

template<typename T>
void ReadString(T* dest, T* src, ProcMemFactory& mem)
{
	//static_assert(sizeof(T) <= MEM_CONST::PageSize, "Object too large");
	--dest;
	do{
		*++dest = mem.read(src++);
	} while (*dest);
}

#define ENUM_STRING_BEGIN(name) \
namespace {\
	namespace name##_get_str_namespace{\
		struct trival_base {\
			static const int value = -1;\
			static __forceinline const char* get_name(int) {\
				return NULL;\
												}\
								};\
		typedef trival_base 

#define ENUM_STRING_CLASS_DEF(name) \
		name##_base;\
		struct name##_classdef : name##_base\
								{\
			static const int value = name##_base::value + 1;\
			static_assert(value == name, "enum value mismatch");\
			static __forceinline const char* get_name(int idx){\
				if (idx == value)\
												{\
					return #name;\
												}\
				return name##_base::get_name(idx);\
												}\
								};\
		typedef name##_classdef 

#define ENUM_STRING_CLASS_DEF_VALUE(name, _value) \
		name##_base;\
		struct name##_classdef : name##_base\
								{\
			static const int value = _value;\
			static_assert(value == name, "enum value mismatch");\
			static __forceinline const char* get_name(int idx){\
				if (idx == value)\
												{\
					return #name;\
												}\
				return name##_base::get_name(idx);\
												}\
								};\
		typedef name##_classdef

#define ENUM_STRING_END(name) \
		final_base;\
				}\
	const char* name##_get_str(int idx)\
				{\
		return name##_get_str_namespace::final_base::get_name(idx);\
				}\
}

ENUM_STRING_BEGIN(DBG_EVENT_T)
ENUM_STRING_CLASS_DEF_VALUE(EXCEPTION_DEBUG_EVENT, 1)
ENUM_STRING_CLASS_DEF_VALUE(CREATE_THREAD_DEBUG_EVENT, 2)
ENUM_STRING_CLASS_DEF_VALUE(CREATE_PROCESS_DEBUG_EVENT, 3)
ENUM_STRING_CLASS_DEF_VALUE(EXIT_THREAD_DEBUG_EVENT, 4)
ENUM_STRING_CLASS_DEF_VALUE(EXIT_PROCESS_DEBUG_EVENT, 5)
ENUM_STRING_CLASS_DEF_VALUE(LOAD_DLL_DEBUG_EVENT, 6)
ENUM_STRING_CLASS_DEF_VALUE(UNLOAD_DLL_DEBUG_EVENT, 7)
ENUM_STRING_CLASS_DEF_VALUE(OUTPUT_DEBUG_STRING_EVENT, 8)
ENUM_STRING_CLASS_DEF_VALUE(RIP_EVENT, 9)
ENUM_STRING_END(DBG_EVENT_T)


class _FILE_Deleter{
public:
	inline void operator() (FILE* fp)
	{
		fclose(fp);
	}
};
typedef ::std::unique_ptr<FILE, _FILE_Deleter> ManagedFile;

class _HANDLE_Deleter{
public:
	inline void operator() (void* p)
	{
		CloseHandle(p);
	}
};
typedef ::std::unique_ptr<void, _HANDLE_Deleter> ManagedHANDLE;

bool operator != (const IMAGE_IMPORT_DESCRIPTOR& a, const IMAGE_IMPORT_DESCRIPTOR& b)
{
	return !!memcmp(&a, &b, sizeof(IMAGE_IMPORT_DESCRIPTOR));
}


void PrintImportFuncs(LPVOID Module, HANDLE hProc)
{
	ProcMemFactory mem(hProc);
	char* image_base = (char*)Module;

	IMAGE_DOS_HEADER dos_header = mem.read((PIMAGE_DOS_HEADER)Module);
	IMAGE_NT_HEADERS image_nt_header = mem.read((PIMAGE_NT_HEADERS)(image_base + dos_header.e_lfanew));
	IMAGE_DATA_DIRECTORY import_dir = image_nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	IMAGE_IMPORT_DESCRIPTOR import_desc;// = mem.read((PIMAGE_IMPORT_DESCRIPTOR)(image_base + import_dir.VirtualAddress));
	IMAGE_IMPORT_DESCRIPTOR null_desc = {};
	PIMAGE_IMPORT_DESCRIPTOR p_imp_desc = (PIMAGE_IMPORT_DESCRIPTOR)(image_base + import_dir.VirtualAddress);

	printf("ImageBase = %p\n", image_base);
	printf("Import table @ %X, size = %X\n", import_dir.VirtualAddress, import_dir.Size);
	while (
		(import_desc = mem.read(p_imp_desc++))
		!= null_desc
		)
	{
		PCHAR DllName = (PCHAR)(image_base + import_desc.Name);
		ReadString((PCHAR)StackStor(), DllName, mem);
		printf("importing from %s", (PCHAR)StackStor());
		/*auto ItFirst = GetDataIt(DllName);
		auto ItLast = GetStrLast(ItFirst);
		printf("Import From %s\n", ::std::string(ItFirst, ItLast).c_str());
		auto pLookUpTable = (LoadedBase + CurrentDesc.OriginalFirstThunk).to<IMAGE_THUNK_DATA>();
		for (auto CurrentThunk = *pLookUpTable; CurrentThunk.u1.AddressOfData; CurrentThunk = *++pLookUpTable)
		{
			if (IMAGE_SNAP_BY_ORDINAL(CurrentThunk.u1.Ordinal))
			{
				printf("\t Ordinal=%x\n", IMAGE_ORDINAL(CurrentThunk.u1.Ordinal));
			}
			else
			{
				auto pFuncHintName = (LoadedBase + CurrentThunk.u1.AddressOfData).to<IMAGE_IMPORT_BY_NAME>();
				auto FuncNameFirst = GetDataIt(member_ptr(pFuncHintName, Name).to<char>());
				auto FuncNameLast = GetStrLast(FuncNameFirst);
				printf("\t FuncName=%s\n", ::std::string(FuncNameFirst, FuncNameLast).c_str());
			}
		}*/
	}
}

/*void PrintExport(LPVOID Module, HANDLE hProc)
{
	ProcDataPtr<char> LoadedBase(hProc, (char*)Module);
	auto pDosHeader = LoadedBase.to<IMAGE_DOS_HEADER>();
	auto pNtHeaders = (LoadedBase + *member_ptr(pDosHeader, e_lfanew)).to<IMAGE_NT_HEADERS>();
	auto pOptiHeader = member_ptr(pNtHeaders, OptionalHeader);
	auto ExportDir = member_ptr(pOptiHeader, DataDirectory)[IMAGE_DIRECTORY_ENTRY_EXPORT];
	auto ExportDesc = (LoadedBase + ExportDir.VirtualAddress).to<IMAGE_EXPORT_DIRECTORY>();

}*/

void PrintFilePath(HANDLE hProc, LPVOID ImageBase)
{
	char Path[MAX_PATH];
	GetMappedFileName(hProc, ImageBase, Path, MAX_PATH);
	printf("\tFileName=%s @ %p\n", Path, ImageBase);
}

void print_range(PVOID* ranges, SIZE_T cnt)
{
	for (SIZE_T i = 0; i < cnt; ++i)
	{
		printf("\t %p \n", ranges[i]);
	}
}

int main(int argc, char** argv)
{
	//MyAlloc<int>::pointer a;
	//::std::map<int, int, ::std::less<int>, MyAlloc<int>> AA;
	//AA.insert(::std::pair<int, int>(1, 1));
	_NT_TIB* pTib = (_NT_TIB*)NtCurrentTeb();
	BOOL bContinue = 1;
	STARTUPINFO StartUpInfo;
	memset(&StartUpInfo, 0, sizeof(StartUpInfo));
	StartUpInfo.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION ProcInfo;

	if (CreateProcess(NULL, argv[1], NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &StartUpInfo, &ProcInfo))
	{
		ManagedHANDLE hProc(ProcInfo.hProcess);
		ManagedHANDLE hThread(ProcInfo.hThread);
		HANDLE hDbgProcess;
		while (bContinue)
		{
			DEBUG_EVENT Event;
			if (WaitForDebugEvent(&Event, 50))
			{
				printf("Get One Event: %s, ProcessId = %d, ThreadId = %d\n", DBG_EVENT_T_get_str(Event.dwDebugEventCode), Event.dwProcessId, Event.dwThreadId);
				switch (Event.dwDebugEventCode)
				{
				case CREATE_PROCESS_DEBUG_EVENT:
				{
					CREATE_PROCESS_DEBUG_INFO& info = Event.u.CreateProcessInfo;
					ManagedHANDLE hFile(info.hFile);
					hDbgProcess = info.hProcess;
					PrintImportFuncs(info.lpBaseOfImage, info.hProcess);
					//ProcMemFactory proc_mem(hDbgProcess);
					//IMAGE_DOS_HEADER dos_header = proc_mem.read((IMAGE_DOS_HEADER*)info.lpBaseOfImage);
					//printf("magic = %c%c\n", (UCHAR)dos_header.e_magic, (UCHAR)(dos_header.e_magic >> 8));
					//SuspendThread(info.hThread);
					//PrintImportFuncs(info.lpBaseOfImage, hDbgProcess);
					printf("\tBaseOfImage=%p\n", info.lpBaseOfImage);
					printf("\tStartAddress=%p\n", info.lpStartAddress);
					PrintFilePath(hDbgProcess, info.lpBaseOfImage);
					printf("Debugger Stack Base @ %p\nDebugger Stack Limit @ %p\n", pTib->StackBase, pTib->StackLimit);
					break;
				}
				case CREATE_THREAD_DEBUG_EVENT:
				{
					CREATE_THREAD_DEBUG_INFO& info = Event.u.CreateThread;
					printf("\tStartAddress=%p\n", info.lpStartAddress);
					break;
				}
				case LOAD_DLL_DEBUG_EVENT:
				{
					LOAD_DLL_DEBUG_INFO& info = Event.u.LoadDll;
					ManagedHANDLE hFile(info.hFile);
					PrintFilePath(hDbgProcess, info.lpBaseOfDll);
					break;
				}
				case EXIT_PROCESS_DEBUG_EVENT:
				{
					EXIT_PROCESS_DEBUG_INFO& info = Event.u.ExitProcess;
					printf("\tProcess Exited, code=%x\n", info.dwExitCode);
					bContinue = 0;
					break;
				}
				case OUTPUT_DEBUG_STRING_EVENT:
				{
					OUTPUT_DEBUG_STRING_INFO& info = Event.u.DebugString;
					void* pBuff = StackStor();
					WORD len = info.nDebugStringLength;
					SIZE_T tmp;
					ReadProcessMemory(hDbgProcess, info.lpDebugStringData, pBuff, len, &tmp);
					if (info.fUnicode)
					{
						wprintf((wchar_t*)pBuff);
					}
					else
					{
						printf((char*)pBuff);
					}
					break;
				}
				case UNLOAD_DLL_DEBUG_EVENT:
				{
					UNLOAD_DLL_DEBUG_INFO info = Event.u.UnloadDll;
					printf("\tDll @%p unloaded\n", info.lpBaseOfDll);
					Sleep(10000);
					break;
				}
				}
				ContinueDebugEvent(Event.dwProcessId, Event.dwThreadId, DBG_CONTINUE);
			}
		}
	}
	else
	{
		fprintf(stderr, "Create Process %s failed\n", argv[1]);
	}
	return 0;
}