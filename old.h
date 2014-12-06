


template<typename T>
T ReadFromOtherProc(HANDLE hProc, const T* pData)
{
	T tmp;
	SIZE_T ret_size;
	ReadProcessMemory(hProc, pData, &tmp, sizeof(T), &ret_size);
	return tmp;
}

template<typename T>
void WriteToOtherProc(HANDLE hProc, T* pData, const T& Data)
{
	SIZE_T ret_size;
	WriteProcessMemory(hProc, pData, &Data, sizeof(T), &ret_size);
}

template<class T, class Enable = void>
class ProcDataPtr;

template<class T>
class ProcDataPtrBase
{
public:
	typedef T DataType;
	typedef T* PtrType;
	typedef ProcDataPtr<DataType> Derived;
protected:
	HANDLE hProc;
	PtrType pData;
public:
	ProcDataPtrBase() = delete;
	ProcDataPtrBase(HANDLE _hProc, PtrType _pData) :hProc(_hProc), pData(_pData)
	{}
	DataType operator*() const{
		return ReadFromOtherProc(hProc, pData);
	}
	void _write(const DataType& Data) const
	{
		WriteToOtherProc(hProc, pData, Data);
	}
	DataType operator[] (ptrdiff_t i) const
	{
		return *(ProcDataPtrBase(hProc, pData + i));
	}
	Derived operator+ (ptrdiff_t i) const
	{
		return Derived(hProc, pData + i);
	}

	template<class R>
	ProcDataPtr<R> to() const
	{
		return ProcDataPtr<R>(hProc, (R*)pData);
	}

	ProcDataPtrBase& operator++ ()
	{
		++pData;
		return *this;
	}
	bool operator!= (const ProcDataPtrBase& other) const
	{
		return (pData != other.pData);
	}
	T* get() const
	{
		return pData;
	}
};

template<class T>
class ProcDataPtr <T, typename ::std::enable_if<::std::is_pointer<T>::value>::type > : public ProcDataPtrBase<T>
{
	//for pointer
private:
	typedef ProcDataPtrBase<T> Base;
	typedef ProcDataPtr<typename ::std::remove_pointer<T>::type> DeRefType;
public:
	ProcDataPtr() = delete;
	ProcDataPtr(HANDLE _hProc, PtrType _pData) :Base(_hProc, _pData)
	{}
	DeRefType operator* () const
	{
		return DeRefType(hProc, ReadFromOtherProc(hProc, pData));
	}
	DeRefType operator[] (ptrdiff_t i) const
	{
		return DeRefType(hProc, ReadFromOtherProc(hProc, pData + i));
	}
};

template<class T>
class ProcDataPtr <T, typename ::std::enable_if<::std::is_class<T>::value>::type > : public ProcDataPtrBase<T>
{
	//for struct
private:
	typedef ProcDataPtrBase<T> Base;
public:
	ProcDataPtr() = delete;
	ProcDataPtr(HANDLE _hProc, PtrType _pData) :Base(_hProc, _pData)
	{}
	template<class M>
	ProcDataPtr<M> _member_ptr(M DataType::*pMember) const
	{
		return ProcDataPtr<M>(hProc, &(pData->*pMember));
	}
};

template<class T>
class ProcDataPtr < T, typename ::std::enable_if<::std::is_array<T>::value>::type > :
	public ProcDataPtr< typename ::std::remove_extent<T>::type>
{
	//for array
private:
	typedef T* ArryPtrTy;
	typedef ProcDataPtr< typename ::std::remove_extent<T>::type> Base;
public:
	ProcDataPtr() = delete;
	ProcDataPtr(HANDLE _hProc, ArryPtrTy _pData) :Base(_hProc, _pData[0])
	{}

};

template<class T, class Enable>
class ProcDataPtr : public ProcDataPtrBase<T>
	// general case
{
private:
	typedef ProcDataPtrBase<T> Base;
public:
	ProcDataPtr() = delete;
	ProcDataPtr(HANDLE _hProc, PtrType _pData) :Base(_hProc, _pData)
	{}
};

template<class T>
class ProcDataIterator : public std::iterator < std::input_iterator_tag, T >, private ProcDataPtr<T>
{
private:
	typedef ProcDataPtr<T> Base;
public:
	ProcDataIterator(const Base& base) : Base(base)
	{}
	ProcDataIterator& operator++()
	{
		Base::operator++();
		return *this;
	}
	bool operator!=(const ProcDataIterator& other) const
	{
		return Base::operator!=(other);
	}
	typename Base::DataType operator* () const
	{
		return Base::operator*();
	}
	T* get() const
	{
		return Base::get();
	}
};

template<class T>
ProcDataIterator<T> GetDataIt(const ProcDataPtr<T>& Ptr)
{
	return ProcDataIterator<T>(Ptr);
}

template<typename It>
It GetStrLast(const It& first)
{
	typedef typename ::std::iterator_traits<It>::value_type ValueTy;
	ValueTy ValEnd = ValueTy();
	It i(first);
	while (*i != ValEnd){
		++i;
	}
	return i;
}

template<class T>
class MyAlloc : public ::std::allocator<T>
{
	typedef ::std::allocator<T> Base;
public:

	MyAlloc() : Base()
	{}

	template<class _Other>
	MyAlloc(const MyAlloc<_Other>& other) : Base(other)
	{}

	pointer allocate(size_type n, MyAlloc<void>::const_pointer hint = 0)
	{
		printf("allocating %d times %s\n", n, typeid(T).name());
		return Base::allocate(n, hint);
	}
	void deallocate(pointer p, size_type n)
	{
		Base::deallocate(p, n);
	}
	template<class OtherTy>
	struct rebind{
		typedef MyAlloc<OtherTy> other;
	};
};

template <>
class MyAlloc<void> : public ::std::allocator < void >{
public:
	template <class U>
	struct rebind { typedef MyAlloc<U> other; };
};

#define member_ptr(S, M) ((S)._member_ptr(&decltype(S)::DataType::M))



class ProcMemFactory;

template<class S>
class RbRoot;

template<class S, class P >
class var_ptr;

template<class S, class P >
class var_ref;

#pragma warning(push)
#pragma warning(disable:4512)

template<class S, class P>
class var_ref
{
	friend class _CtrlNode;
	friend class RbRoot < S >;
	friend class ProcMemFactory;
public:
	typedef S DataType;
	typedef P ParaType;
	friend class var_ptr < S, P>;

	typedef var_ptr < S, P> PtrType;
private:
	typedef DataType(*pGetter)(const ParaType&);
	typedef void(*pSetter)(const ParaType&, const DataType&);
	const ParaType _getset_para;
	const pGetter _getter;
	const pSetter _setter;
private:
	__forceinline var_ref(const var_ref& other)
		: _getset_para(other._getset_para), _getter(other._getter), _setter(other._setter)
	{}
	__forceinline var_ref()
		: _getset_para(), _getter(nullptr), _setter(nullptr)
	{}
	__forceinline var_ref(const ParaType& _gs_para, pGetter _get, pSetter _set)
		: _getset_para(_gs_para), _getter(_get), _setter(_set)
	{}
public:
	__forceinline DataType _get() const
	{
		return _getter(_getset_para);
	}
	/*NodePtrType operator->() const
	{
	return _get();
	}*/
	__forceinline operator DataType() const
	{
		return _get();
	}
	__forceinline explicit operator bool() const
	{
		return !!_get();
	}
	__forceinline var_ref& operator= (const DataType& _new_data)
	{
		_setter(_getset_para, _new_data);
		return *this;
	}
	__forceinline PtrType operator& () const
	{
		return PtrType(*this);
	}
	//bool operator==(const var_ref&) = delete;
	//var_ref& operator= (const var_ref& _new_data) = delete;
};

template<class S, class P>
class var_ref < const S, P >
{
	friend class _CtrlNode;
	friend class RbRoot < S >;
	friend class ProcMemFactory;
public:
	typedef S DataType;
	typedef P ParaType;
	friend class var_ptr < const S, P>;

	typedef var_ptr < const S, P > PtrType;
private:
	typedef DataType(*pGetter)(const ParaType&);
	const ParaType _get_para;
	const pGetter _getter;
private:
	__forceinline var_ref(const var_ref& other)
		: _get_para(other._get_para), _getter(other._getter)
	{}
	__forceinline var_ref()
		: _get_para(), _getter(nullptr)
	{}
	__forceinline var_ref(const ParaType& _g_para, pGetter _get)
		: _get_para(_g_para), _getter(_get)
	{}
public:
	__forceinline DataType _get() const
	{
		return _getter(_get_para);
	}
	/*NodePtrType operator->() const
	{
	return _get();
	}*/
	__forceinline operator DataType() const
	{
		return _get();
	}
	__forceinline explicit operator bool() const
	{
		return !!_get();
	}
	__forceinline PtrType operator& () const
	{
		return PtrType(*this);
	}
	//bool operator==(const var_ref&) = delete;
	//var_ref& operator= (const var_ref& _new_data) = delete;
};

/*template<class S>
bool operator==(const var_ref<S>& lhs, const var_ref<S>& rhs)
{
return lhs._get() == rhs._get();
}

template<class S>
bool operator!=(const var_ref<S>& lhs, const var_ref<S>& rhs)
{
return lhs._get() != rhs._get();
}*/

#pragma warning(pop)

template<class S, class P>
class var_ptr
{
	friend class var_ref < S, P>;
public:
	typedef var_ref < S, P > RefType;
private:
	RefType ref;
	__forceinline var_ptr& assign(const RefType& newref)
	{
		(::std::addressof(ref))->~RefType();
		new (::std::addressof(ref)) RefType(newref);
		return *this;
	}
	__forceinline var_ptr(const RefType& _ref)
		:ref(_ref)
	{}
public:
	__forceinline var_ptr()
		:ref()
	{}
	__forceinline var_ptr(const var_ptr& other)
		: ref(*other)
	{}
	__forceinline RefType operator*() const
	{
		return RefType(ref);
	}
	__forceinline var_ptr& operator=(const var_ptr<S, P>& other)
	{
		return assign(*other);
	}
	/*var_ptr& operator=(const var_ptr<typename::std::add_const<S>::type>&  other)
	{
	return assign(*other);
	}
	var_ptr& operator=(const var_ptr<typename::std::remove_const<S>::type>& other)
	{
	return assign(*other);
	}*/
};