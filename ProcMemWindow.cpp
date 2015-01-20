//#include <Windows.h>
#define _CRTDBG_MAP_ALLOC
#include <stdint.h>
#include <map>
#include <algorithm>
#include <string>
#include <bitset>
#include <exception>
#include <cassert>
#include <Windows.h> 
#include "consts.hpp"
#include "num_helper.h"
#include "ctrl_node.hpp"

void* StackStor()
{
	_NT_TIB* pTib = (_NT_TIB*)NtCurrentTeb();
	char* pStkLimit = (char*)pTib->StackLimit;
	return pStkLimit;
}

class DebugSystemNonContinueError : public ::std::runtime_error
{
public:
	DebugSystemNonContinueError(const char* msg) : ::std::runtime_error(msg){}
};

class DebugSystemContinueError : public ::std::runtime_error
{
public:
	DebugSystemContinueError(const char* msg) : ::std::runtime_error(msg){}
};

#if _HAS_EXCEPTIONS
void ThrowDebugSystemNonContinueError(const char* msg)
{
	throw DebugSystemNonContinueError(msg);
}
void ThrowDebugSystemContinueError(const char* msg)
{
	throw DebugSystemContinueError(msg);
}
#else
void ThrowDebugSystemNonContinueError(const char* msg)
{
	EXIT_WITH_LINENO(FILE_ID_FACTORY);
}
void ThrowDebugSystemContinueError(const char* msg)
{
	EXIT_WITH_LINENO(FILE_ID_FACTORY);
}
#endif

__forceinline static void SplitModiProt(DWORD modiprot, size_t& modi, size_t& prot)
{
	DWORD split_modi = modiprot >> MEM_CONST::MyMemProtBits;
	if (split_modi)
	{
		modi = check_scan_bit32(split_modi);
	}
	else
	{
		modi = MEM_CONST::MY_PAGE_NULL_MODIFIER_BIT;
	}
	prot = check_scan_bit32(modiprot & MEM_CONST::MyMemProtMask);
}

__forceinline static DWORD CatModiProt(size_t modi, size_t prot)
{
	DWORD ret = DWORD(1) << modi;
	ret <<= MEM_CONST::MyMemProtBits;
	ret &= MEM_CONST::MyMemModiMask;
	ret |= DWORD(1) << prot;
	return ret;
}

typedef char MemoryPage[MEM_CONST::PageSize];

MemoryPage* AllocMemoryWatchPool(size_t page_count)
{
	static_assert(sizeof(MemoryPage) == MEM_CONST::PageSize, "check MemoryPage");
	void* ret = VirtualAlloc(NULL, MEM_CONST::PageSize * page_count, MEM_COMMIT | MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
	if (!ret)
	{
		ThrowDebugSystemNonContinueError("Memory Pool Allocation Failed");
	}
	sprintf((char*)StackStor(), "Mempool Allocated at %p\n", ret);
	OutputDebugStringA((char*)StackStor());
	return reinterpret_cast<MemoryPage*>(ret);
}

void DeallocMemoryWatchPool(MemoryPage* pool)
{
	if (!VirtualFree(pool, 0, MEM_RELEASE))
	{
		EXIT_WITH_LINENO(FILE_ID_FACTORY);
	}
}

class ProcMemFactory
{
	/*
	Two level cache:
	Level1: HashMap
	Level2: Rb-Tree
	*/
	/*
	Hash Algo:
	0            12           24      31/63
	[------------|------------|--------]
	[ PageOffset |       PageNumber    ]
	[ HashEnt]
	*/
	static const size_t NodeSlotNumber = 0x1000ULL;
	static const size_t NodeSlotNumberBits = check_log2<NodeSlotNumber>::value;
	static const size_t MemoryPoolSize = NodeSlotNumber* MEM_CONST::PageSize;

	static const size_t HashTabSize = 0x40ULL;
	static const size_t HashTabSizeBits = check_log2<HashTabSize>::value;

	static const size_t HashEntTagBits = MEM_CONST::PageNumberBits - HashTabSizeBits;
	static const size_t HashEntValBits = NodeSlotNumberBits;

	static const size_t MapRatio = NodeSlotNumber / HashTabSize;
	static const size_t MapRatioBits = check_log2<MapRatio>::value;

	struct HashTabEnt{
		uintptr_t Tag : HashEntTagBits;
		uintptr_t Val : HashEntValBits;
	};

	static_assert(sizeof(HashTabEnt) == sizeof(uintptr_t), "check HashTabEnt size, Ratio too high");
	// The maximum ratio equals page size

	static const size_t HashTabEntBits = HashEntTagBits + HashEntValBits;

private:
	typedef size_t NodeIthT;
	typedef size_t HashIthT;
	typedef uintptr_t AddrT;
	typedef uintptr_t PageIthT;
	typedef size_t PageOffT;
	HANDLE const hProc;
	MemoryPage* const MemoryPool;
	_CtrlNode* LRUFirst;
	_CtrlNode* LRULast;
	_CtrlNode* AvailSlotsHead;
	_CtrlNode::RootTy NodeRbRoot;
	_CtrlNode CtrlNodes[NodeSlotNumber];
	HashTabEnt HashTab[HashTabSize];
	::std::bitset<HashTabSize> HashValid;
	bool Disabled;
public:
	ProcMemFactory(HANDLE _hProc)
		:
		hProc(_hProc),
		MemoryPool(AllocMemoryWatchPool(NodeSlotNumber)),
		LRUFirst(nullptr),
		LRULast(nullptr),
		AvailSlotsHead(CtrlNodes),
		Disabled(false)
	{
		//static const size_t sizeof_ctrl_nodes = sizeof(CtrlNodes);
		static_assert(sizeof(CtrlNodes) == sizeof(_CtrlNode) *NodeSlotNumber, "check Nodes size");
		static_assert(sizeof(HashTab) == HashTabSize * sizeof(uintptr_t), "check Nodes size");
		memset(CtrlNodes, 0, sizeof(CtrlNodes));
		init_node_list(CtrlNodes);
	}
	~ProcMemFactory()
	{
		//Check if the factory is valid
		if (!Disabled)
		{
			try{
				Update();
			}
			catch (::std::exception& e)
			{
				OutputDebugStringA("dtor ProcMemFactory failed: ");
				OutputDebugStringA(e.what());
			}
			//do not allow exceptions escape from dtor
		}
		DeallocMemoryWatchPool(MemoryPool);
	}
private:	
	NodeIthT NodeNumber(_RbCtrlNode* node)
	{
		return static_cast<_CtrlNode*>(node) - CtrlNodes;
	}
	bool GetHashCheck(PageIthT page_number, NodeIthT& node_number)
	{
		HashIthT hash_slot = _hash_map_func<HashTabSizeBits>(page_number);
		PageIthT real_tag = page_number >> HashTabSizeBits;
		if (HashValid[hash_slot] && HashTab[hash_slot].Tag == real_tag)
		{
			node_number = HashTab[hash_slot].Val;
			return true;
		}
		return false;
	}
	void InvHashTab(PageIthT page_number, NodeIthT mapped_node)
	{
		HashIthT hash_slot = _hash_map_func<HashTabSizeBits>(page_number);
		PageIthT real_tag = page_number >> HashTabSizeBits;
		if (HashValid[hash_slot])
		{
			if (HashTab[hash_slot].Tag == real_tag)
			{
				if (HashTab[hash_slot].Val != mapped_node)
				{
					EXIT_WITH_LINENO(FILE_ID_FACTORY);
				}
				HashValid[hash_slot] = false;
			}
		}
	}
	void UpdateHashTab(PageIthT page_number, NodeIthT mapped_node)
	{
		HashIthT hash_slot = _hash_map_func<HashTabSizeBits>(page_number);
		PageIthT real_tag = page_number >> HashTabSizeBits;
		HashValid[hash_slot] = true;
		HashTab[hash_slot].Tag = real_tag;
		HashTab[hash_slot].Val = mapped_node;
	}
	void UpdateLRU(_CtrlNode* node)
	{
		remove_list_node(node, LRUFirst, LRULast);
		add_dual_last(node, LRUFirst, LRULast);
	}
	void InsertLRU(_CtrlNode* node)
	{
		add_dual_last(node, LRUFirst, LRULast);
	}
	_CtrlNode* GetOneAvail()
	{
		return remove_si_head(AvailSlotsHead);
	}
	void AddOneAvail(_CtrlNode* node)
	{
		add_si_head(node, AvailSlotsHead);
	}
	_CtrlNode* FlushOneBusy()
	{
		_CtrlNode* node = remove_dual_first(LRUFirst, LRULast);
		if (!node)
		{
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
		InvHashTab(node->page_number, NodeNumber(node));
		if (GetRb(node->page_number) != node)
		{
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
		DeleteRb(node);
		FlushPage(node);
		return node;
	}
	_CtrlNode* GetRb(PageIthT page_number)
	{
		_CtrlNode* node = NodeRbRoot.rb_node;
		while (node)
		{
			if (node->page_number == page_number)
				return node;
			else if (node->page_number > page_number)
				node = node->rb_left;
			else
				node = node->rb_right;
		}
		return nullptr;
	}
	void DeleteRb(_CtrlNode* node)
	{
		rb_erase(node, &NodeRbRoot);
	}
	void InsertRb(_CtrlNode* new_node)
	{
		_CtrlNode* parent = nullptr;
		_CtrlNode** _new = &NodeRbRoot.rb_node;
		while (*_new)
		{
			parent = *_new;
			if (parent->page_number == new_node->page_number){
				EXIT_WITH_LINENO(FILE_ID_FACTORY);
			}
			else if (parent->page_number > new_node->page_number)
				_new = &parent->rb_left;
			else
				_new = &parent->rb_right;
		}
		rb_link_node(new_node, parent, _new);
		rb_insert_color(new_node, &NodeRbRoot);
	}
public:
	MemoryPage* FindAndGetPage(PageIthT remote_page, uintptr_t desired_access)
	{
		if (Disabled)
		{
			throw DebugSystemNonContinueError("Factory disabled by previous exception");
		}
		try
		{
			NodeIthT mapped_node;
			_CtrlNode* node;
			if (LRULast && LRULast->page_number == remote_page)
			{
				mapped_node = NodeNumber(LRULast);
				UpdateProtect(CtrlNodes + mapped_node, desired_access);
			}
			else if (GetHashCheck(remote_page, mapped_node))
			{
				UpdateProtect(CtrlNodes + mapped_node, desired_access);
				UpdateLRU(CtrlNodes + mapped_node);
			}
			else if ((node = GetRb(remote_page)))
			{
				UpdateProtect(node, desired_access);
				mapped_node = NodeNumber(node);
				UpdateHashTab(remote_page, mapped_node);
				UpdateLRU(node);
			}
			else
			{
				//Should be fetched
				mapped_node = FetchNewPage(remote_page, desired_access);
				UpdateHashTab(remote_page, mapped_node);
				InsertLRU(CtrlNodes + mapped_node);
			}
			return MemoryPool + mapped_node;
		}
		catch (DebugSystemNonContinueError&)
		{
			Disabled = true;
			throw;
		}
	}
private:
	void FetchData(void* pData, size_t DataSize, char* buff)
	{
		AddrT ptr = reinterpret_cast<AddrT>(pData);
		PageIthT current_page = (ptr & ~((AddrT)MEM_CONST::PageSize - 1)) >> MEM_CONST::PageBits;
		PageOffT current_offset = ptr & (MEM_CONST::PageSize - 1);
		size_t avail_space = MEM_CONST::PageSize - current_offset;

		while (DataSize)
		{
			MemoryPage* mapped_page = FindAndGetPage(current_page, false);
			if (DataSize <= avail_space)
			{
				memcpy(buff, (char*)mapped_page + current_offset, DataSize);
				DataSize = 0;
			}
			else
			{
				memcpy(buff, (char*)mapped_page + current_offset, avail_space);
				DataSize -= avail_space;
			}
			buff += avail_space;
			avail_space = MEM_CONST::PageSize;
			current_offset = 0;
			++current_page;
		}
	}
	void StoreData(void* pData, size_t DataSize, const char* buff)
	{
		AddrT ptr = reinterpret_cast<AddrT>(pData);
		PageIthT current_page = (ptr & ~((AddrT)MEM_CONST::PageSize - 1)) >> MEM_CONST::PageBits;
		PageOffT current_offset = ptr & (MEM_CONST::PageSize - 1);
		size_t avail_space = MEM_CONST::PageSize - current_offset;

		while (DataSize)
		{
			MemoryPage* mapped_page = FindAndGetPage(current_page, true);
			if (DataSize <= avail_space)
			{
				memcpy((char*)mapped_page + current_offset, buff, DataSize);
				DataSize = 0;
			}
			else
			{
				memcpy((char*)mapped_page + current_offset, buff, avail_space);
				DataSize -= avail_space;
			}
			buff += avail_space;
			avail_space = MEM_CONST::PageSize;
			current_offset = 0;
			++current_page;
		}
	}
	void GetRequestProtect(_RbCtrlNode* node, uintptr_t desired_access)
	{
		size_t curr_modi;
		size_t curr_prot;
		//Get modi / prot
		MEMORY_BASIC_INFORMATION mem_info;
		SIZE_T ret;
		LPVOID remote_addr = (LPVOID)(node->page_number << MEM_CONST::PageBits);
		if (!(ret = VirtualQueryEx(hProc, remote_addr, &mem_info, sizeof(mem_info))))
		{
			ThrowDebugSystemContinueError("VirtualQuery Failed");
		}
		if (ret != sizeof(mem_info))
		{
			ThrowDebugSystemContinueError("VirtualQueryEx Information buffer size");
		}
		switch (mem_info.State)
		{
		case MEM_COMMIT:
			break;
		case MEM_FREE:
		case MEM_RESERVE:
			ThrowDebugSystemContinueError("Trying to read/write Free/Reserved Page");
		default:
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
		SplitModiProt(mem_info.Protect, curr_modi, curr_prot);
		node->OldModi = curr_modi;
		node->OldProt = curr_prot;

		bool needs_fix = false;
		if (curr_modi == MEM_CONST::MY_PAGE_GUARD_BIT)
		{
			//The Guard Property is removed
			curr_modi = MEM_CONST::MY_PAGE_NULL_MODIFIER_BIT;
			needs_fix = true;
		}
		if (curr_prot % MEM_CONST::MyMemProtRWBits <= size_t(desired_access))
		{
			//Elevate the Prot Level
			curr_prot = size_t(desired_access) + 1;
			needs_fix = true;
		}
		if (needs_fix)
		{
			DWORD new_modiprot = CatModiProt(curr_modi, curr_prot);
			DWORD tmp;
			if (!VirtualProtectEx(hProc, remote_addr, MEM_CONST::PageSize, new_modiprot, &tmp))
			{
				//fix failed
				ThrowDebugSystemContinueError("Fix Prot Failed");
			}
			sprintf((char*)StackStor(), "remote page %p Prot changed to %X (First time)\n", remote_addr, new_modiprot);
			OutputDebugStringA((char*)StackStor());
		}
		node->level = desired_access;
	}
	void UpdateProtect(_RbCtrlNode* node, uintptr_t desired_access)
	{
		if (node->level < desired_access)
		{
			size_t new_modi = node->OldModi;
			size_t new_prot = node->OldProt;
			LPVOID remote_addr = (LPVOID)(node->page_number << MEM_CONST::PageBits);
			if (new_modi == MEM_CONST::MY_PAGE_GUARD_BIT)
			{
				//The Guard Property is removed
				new_modi = MEM_CONST::MY_PAGE_NULL_MODIFIER_BIT;
			}
			if (new_prot % MEM_CONST::MyMemProtRWBits <= size_t(desired_access))
			{
				//Elevate the Prot Level
				new_prot = size_t(desired_access) + 1;
			}
			DWORD new_modiprot = CatModiProt(new_modi, new_prot);
			DWORD tmp;
			if (!VirtualProtectEx(hProc, remote_addr, MEM_CONST::PageSize, new_modiprot, &tmp))
			{
				//fix failed
				ThrowDebugSystemContinueError("Fix Prot Failed");
			}
			sprintf((char*)StackStor(), "remote page %p Prot changed to %X\n", remote_addr, new_modiprot);
			OutputDebugStringA((char*)StackStor());
			
			node->level = desired_access;
		}
	}
	void ResetDirtyFlag(NodeIthT mapped_page)
	{
		if (ResetWriteWatch(MemoryPool[mapped_page], MEM_CONST::PageSize))
		{
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
	}
	void UpDateDirtyFlag(NodeIthT mapped_page)
	{
		PVOID Addr;
		ULONG_PTR buff_size = 1;
		ULONG sys_page_size;
		if (GetWriteWatch(WRITE_WATCH_FLAG_RESET, MemoryPool[mapped_page], 
			MEM_CONST::PageSize, &Addr,	&buff_size, &sys_page_size))
		{
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
		if (sys_page_size != MEM_CONST::PageSize)
		{
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
		if (buff_size)
		{
			if (Addr != MemoryPool[mapped_page])
			{
				EXIT_WITH_LINENO(FILE_ID_FACTORY);
			}
			CtrlNodes[mapped_page].dirty = 1;
		}
	}
	void RestoreProt(_RbCtrlNode* node)
	{
		LPVOID remote_addr = (LPVOID)(node->page_number << MEM_CONST::PageBits);
		if (node->OldModi == MEM_CONST::MY_PAGE_GUARD_BIT ||
			(node->OldProt % MEM_CONST::MyMemProtRWBits) <= node->level)
		{
			//Restore the Prot of the page
			DWORD orig_modiprot = CatModiProt(node->OldModi, node->OldProt);
			DWORD tmp;
			if (!VirtualProtectEx(hProc, remote_addr, MEM_CONST::PageSize, orig_modiprot, &tmp))
			{
				ThrowDebugSystemNonContinueError("Restore Prot Failed");
			}
		}
	}
	void FlushPage(_RbCtrlNode* node)
	{
		LPVOID remote_addr = (LPVOID)(node->page_number << MEM_CONST::PageBits);
		UpDateDirtyFlag(NodeNumber(node));
		if (node->dirty)
		{
			node->dirty = 0;
			if (!node->level)
			{
				//Should not happen since NO write is issued
				//Perhaps the Pool is being corrupted.
				EXIT_WITH_LINENO(FILE_ID_FACTORY);
			}

			//begin write back
			SIZE_T write_size;
			BOOL ret = WriteProcessMemory(hProc,remote_addr,MemoryPool[NodeNumber(node)],
				MEM_CONST::PageSize,&write_size);
			if (!ret || write_size != MEM_CONST::PageSize)
			{
				//Should not happen since Prot is GO for write
				ThrowDebugSystemNonContinueError("Write Process Memory Failed");
			}
			sprintf((char*)StackStor(), "remote page %P has been write back\n", remote_addr);
			OutputDebugStringA((char*)StackStor());
		}
	}
	NodeIthT FetchNewPage(PageIthT remote_page, uintptr_t desired_access) //returns node number
	{
		_RbCtrlNode tmp_ctrl_node;
		tmp_ctrl_node.page_number = remote_page;
		tmp_ctrl_node.dirty = 0;
		GetRequestProtect(&tmp_ctrl_node, desired_access);
		
		_CtrlNode* node = GetOneAvail();
		if (!node){
			node = FlushOneBusy();
		}
		if (!node){
			//no other choices
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
		if (node->dirty)
		{
			//The page was not clean
			EXIT_WITH_LINENO(FILE_ID_FACTORY);
		}
		_RbCtrlNode* ctrlnode = node;
		*ctrlnode = tmp_ctrl_node;

		FetchRWPage(node);
		InsertRb(node);
		return NodeNumber(node);
	}
	void FetchRWPage(_RbCtrlNode* node)
	{
		LPVOID remote_addr = (LPVOID)(node->page_number << MEM_CONST::PageBits);
		SIZE_T read_size;
		BOOL ret = ReadProcessMemory(hProc, remote_addr, MemoryPool[NodeNumber(node)],
			MEM_CONST::PageSize, &read_size);

		ResetDirtyFlag(NodeNumber(node));
		//reset the watch since the OS (may) modifies our page

		if (!ret || read_size != MEM_CONST::PageSize)
		{
			//Prot(READ) OK but we can not read or read incomplete!!
			ThrowDebugSystemNonContinueError("Read Process Memory error");
		}
	}
	void UpdateClear()
	{
		for (_CtrlNode* node = LRUFirst; node; node = node->next)
		{
			InvHashTab(node->page_number, NodeNumber(node));
			FlushPage(node);
			RestoreProt(node);
		}
		move_si_to_other(LRUFirst, LRULast, AvailSlotsHead);
		NodeRbRoot.rb_node = nullptr;
	}
	void Update()
	{
		for (_CtrlNode* node = LRUFirst; node; node = node->next)
		{
			FlushPage(node);
		}
	}
public:
	ProcMemFactory& operator=(const ProcMemFactory&) = delete;
	
	template<class U>
	U read(U* pData)
	{
		U ret;
		FetchData(pData, sizeof(U), (char*)&ret);
		return ret;
	}

	template<class U>
	void write(U* pData, const U& buff)
	{
		StoreData(pData, sizeof(U), (char*)&buff);
	}

	/*template<class U>
	struct PtrInfo{
		ProcMemFactory* pFactory;
		U* ptr;
		PtrInfo(ProcMemFactory* pFac, U* p) : pFactory(pFac), ptr(p)
		{}
	};

	template<class U>
	static U GetData(const PtrInfo<U>& info)
	{
		U data;
		info.pFactory->FetchData(info.ptr, sizeof(U), (char*)&data);
		return data;
	}

	template<class U>
	static void SetData(const PtrInfo<U>& info, const U& data)
	{
		info.pFactory->StoreData(info.ptr, sizeof(U), (const char*)&data);
	}

	template<class U>
	var_ptr<U, PtrInfo<U>> GetVarPtr(U* ptr)
	{
		return &var_ref<U, PtrInfo<U>>(
			PtrInfo < U >(this, ptr),
			GetData < U >,
			SetData < U >
			);
	}*/
};



