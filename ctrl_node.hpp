#include <stdint.h>
#include <type_traits>

class _CtrlNode;

struct _RbCtrlNode{
	uintptr_t NewProt : 3;
	uintptr_t OldProt : 3;
	uintptr_t color : 1;
	uintptr_t dirty : 1;
	uintptr_t NewModi : 2;
	uintptr_t OldModi : 2;
	uintptr_t page_number : MEM_CONST::PageNumberBits;
};
static_assert(sizeof(_RbCtrlNode) == sizeof(uintptr_t), "check _RbNode size");

struct _RbListNode {
	_CtrlNode* prev;
	_CtrlNode* next;
	_CtrlNode* parent;
	_CtrlNode* rb_right;
	_CtrlNode* rb_left;
};
static_assert(sizeof(_RbListNode) == sizeof(uintptr_t) * 5, "check _ListNode size");

#include "var_ref_ptr.hpp"

#define NODE_PTR_MEMBER_REF_DEFINE(M)\
private:\
	static SelfPtrType _get_##M(void* const& _ptr) {\
		SelfPtrType _this = reinterpret_cast<SelfPtrType>(_ptr);\
		if(_this->M##_offset == nil) {return nullptr;}\
		else{ return _this + _this->M##_offset;}\
	}\
	static SelfPtrType _get_const_##M(const void* const& _ptr){\
		CSelfPtrType _this = reinterpret_cast<CSelfPtrType>(_ptr);\
		if(_this->M##_offset == nil) {return nullptr;}\
		else{ return const_cast<SelfPtrType>(_this) + _this->M##_offset;}\
	}\
	static void _set_##M(void* const& _ptr, const SelfPtrType& newptr) {\
		SelfPtrType _this = reinterpret_cast<SelfPtrType>(_ptr);\
		if(newptr == nullptr) {_this->M##_offset = nil;}\
		else{_this->M##_offset = newptr - _this;}\
	}\
public:\
	cptr_ref M() const \
	{return cptr_ref(this, _get_const_##M);}\
	ptr_ref M() \
	{return ptr_ref(this, _get_##M, _set_##M);}

template<class S>
class RbRoot {
public:
	typedef S NodePtrTy;
	typedef RbRoot<S>* RootPtrTy;
	typedef typename ::std::remove_pointer<NodePtrTy>::type NodeTy;
	NodePtrTy rb_node;
private:
	static NodePtrTy _get(void* const& _ptr)
	{
		return reinterpret_cast<RootPtrTy>(_ptr)->root;
	}
	/*static NodePtrTy _get_const(const NodePtrTy& _ptr)
	{
		_ptr->root;
	}*/
	static void _set(void* const& _ptr, const NodePtrTy& newptr)
	{
		reinterpret_cast<RootPtrTy>(_ptr)->root = newptr;
	}
public:
	RbRoot() : rb_node(nullptr)
	{}
	/*var_ref<const NodePtrTy, RootPtrTy> rb_node() const
	{
		return var_ref<const NodePtrTy, RootPtrTy>((RootPtrTy)this, _get_const);
	}*/
	/*typename NodeTy::ptr_ref rb_node()
	{
		return typename NodeTy::ptr_ref(this, _get, _set);
	}*/
};

#pragma pack(push, 1)
class _CtrlNode : public _RbCtrlNode, public _RbListNode
{
private:
	typedef _CtrlNode SelfType;
	typedef _CtrlNode* SelfPtrType;
	typedef const _CtrlNode* CSelfPtrType;
public:
	//static const int64_t nil = (int32_t(-1)) << (bitsof<int32_t>::value - 1);
	//friend class var_ref < SelfPtrType >;
	//friend class var_ref < const SelfPtrType >;

	typedef RbRoot<SelfPtrType> RootTy;
	//typedef var_ref<SelfPtrType, void*> ptr_ref;
	//typedef var_ref<const SelfPtrType, const void*> cptr_ref;
	//typedef var_ref<unsigned, SelfPtrType> color_ref;
	//typedef var_ref<const unsigned, CSelfPtrType> ccolor_ref;
	//typedef ptr_ref::PtrType pptr;
	//typedef cptr_ref::PtrType cpptr;

	/*static unsigned _get_color(const SelfPtrType& _ptr)
	{
		return _ptr->__color;
	}

	static unsigned _get_const_color(const CSelfPtrType& _ptr)
	{
		return _ptr->__color;
	}

	static void _set_color(const SelfPtrType& _ptr, const unsigned& new_color)
	{
		_ptr->__color = new_color;
	}

	color_ref color()
	{
		return color_ref(this, _get_color, _set_color);
	}

	ccolor_ref color() const
	{
		return ccolor_ref(this, _get_const_color);
	}*/

	//NODE_PTR_MEMBER_REF_DEFINE(parent)
	//NODE_PTR_MEMBER_REF_DEFINE(rb_right)
	//NODE_PTR_MEMBER_REF_DEFINE(rb_left)
	//NODE_PTR_MEMBER_REF_DEFINE(prev)
	//NODE_PTR_MEMBER_REF_DEFINE(next)

};
static_assert(sizeof(_CtrlNode) == sizeof(uintptr_t) + sizeof(uintptr_t) *5, "check _CtrlNode size");
#pragma pack(pop)

#include "rb_tree_port_h.hpp"
#include "list.hpp"
