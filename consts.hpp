#pragma once
#include <stdint.h>

template<size_t N>
struct check_log2;

template<>
struct check_log2<0>{
};

template<>
struct check_log2 < 1 > {
	static const size_t value = 0;
};

template<size_t N>
struct check_log2 : check_log2<!(N % 2)>{
	static const size_t value =
		check_log2<N / 2>::value + 1;
};

namespace MEM_CONST{
	static const size_t PageSize = 0x1000ULL;
	static const size_t PageBits = check_log2<PageSize>::value;
#ifdef _M_X64
	static const size_t PageNumberBits = 64 - PageBits;
#else
	static const size_t PageNumberBits = 32 - PageBits;
#endif
	static const size_t MyMemProtBits = 8;
	static const size_t MyMemProtRWBits = 4;
	static const size_t MyMemProtMask = 0xff;
	static const size_t MyMemATTRMask = 0x1ff;
	static const size_t MY_PAGE_NOACCESS_BIT			= check_log2<PAGE_NOACCESS>::value;
	static const size_t MY_PAGE_READONLY_BIT			= check_log2<PAGE_READONLY>::value;
	static const size_t MY_PAGE_READWRITE_BIT			= check_log2<PAGE_READWRITE>::value;
	static const size_t MY_PAGE_WRITECOPY_BIT			= check_log2<PAGE_WRITECOPY>::value;
	static const size_t MY_PAGE_EXECUTE_BIT				= check_log2<PAGE_EXECUTE>::value;
	static const size_t MY_PAGE_EXECUTE_READ_BIT		= check_log2<PAGE_EXECUTE_READ>::value;
	static const size_t MY_PAGE_EXECUTE_READWRITE_BIT	= check_log2<PAGE_EXECUTE_READWRITE>::value;
	static const size_t MY_PAGE_EXECUTE_WRITECOPY_BIT	= check_log2<PAGE_EXECUTE_WRITECOPY>::value;
	static const size_t MY_PAGE_GUARD_BIT				= check_log2<(PAGE_GUARD>>MyMemProtBits)>::value;
	static const size_t MY_PAGE_NOCACHE_BIT				= check_log2<(PAGE_NOCACHE>>MyMemProtBits)>::value;
	static const size_t MY_PAGE_WRITECOMBINE_BIT		= check_log2<(PAGE_WRITECOMBINE>>MyMemProtBits)>::value; 
	static const size_t MY_PAGE_NULL_MODIFIER_BIT		= 3;
}

static const size_t FILE_ID_FACTORY = 0x40000000ULL;
#define EXIT_WITH_LINENO(x) do{__debugbreak(); exit(x|__LINE__);}while(0)
