#pragma once
#include <stdint.h>
#include <cstdlib>
#include <intrin.h>
#include "consts.hpp"

template<class T>
struct bitsof{
	static const size_t value = sizeof(T) * 8;
};

template<class T>
T shiftright_d(T, T, size_t cnt);

template<class T>
T shiftleft_d(T, T, size_t cnt);

template<>
unsigned __int32 shiftright_d(unsigned __int32 a, unsigned __int32 b, size_t cnt)
{
	union{
		struct{
			unsigned __int32 l;
			unsigned __int32 h;
		};
		unsigned __int64 d;
	} int_d;
	int_d.l = a;
	int_d.h = b;
	return (unsigned __int32)(int_d.d >> cnt);
}

template<>
unsigned __int32 shiftleft_d(unsigned __int32 a, unsigned __int32 b, size_t cnt)
{
	union{
		struct{
			unsigned __int32 l;
			unsigned __int32 h;
		};
		unsigned __int64 d;
	} int_d;
	int_d.l = a;
	int_d.h = b;
	return (unsigned __int32)(int_d.d << cnt);
}

#ifdef _M_X64

template<>
unsigned __int64 shiftright_d(unsigned __int64 a, unsigned __int64 b, size_t cnt)
{
	return __shiftright128(a, b, (BYTE)cnt);
}

template<>
unsigned __int64 shiftleft_d(unsigned __int64 a, unsigned __int64 b, size_t cnt)
{
	return __shiftleft128(a, b, (BYTE)cnt);
}

#endif

/*static inline size_t _reduce_table_ptrs(size_t bitwidth, size_t ith, size_t& bit_offset)
{
	//normalize so we don't overflow
	size_t byte_offset = ith / bitsof<uintptr_t>::value * bitwidth;
	ith %= bitsof<uintptr_t>::value;

	bit_offset = ith * bitwidth;
	byte_offset += bit_offset / bitsof<uintptr_t>::value;
	bit_offset %= bitsof<uintptr_t>::value;

	return byte_offset;
}

template<size_t BitWidth>
uintptr_t get_table_bits(const uintptr_t* base, size_t ith)
{
	static const uintptr_t mask = uintptr_t(-1) >> (bitsof<uintptr_t>::value - BitWidth);
	static_assert(BitWidth <= bitsof<uintptr_t>::value, "check BitWidth");

	size_t bit_offset;
	base += _reduce_table_ptrs(BitWidth, ith, bit_offset);

	uintptr_t value = base[0];

	if (bit_offset + BitWidth > bitsof<uintptr_t>::value)
	{
		uintptr_t value_high = base[1];
		value = shiftright_d(value, value_high, bit_offset);
	}
	else
	{
		value >>= bit_offset;
	}
	return value & mask;
}

template<size_t BitWidth>
void set_table_bits(uintptr_t value, uintptr_t* base, size_t ith)
{
	static const uintptr_t mask = uintptr_t(-1) >> (bitsof<uintptr_t>::value - BitWidth);
	static_assert(BitWidth <= bitsof<uintptr_t>::value, "check BitWidth");

	size_t bit_offset;
	base += _reduce_table_ptrs(BitWidth, ith, bit_offset);

	base[0] &= ~(mask << bit_offset);
	base[0] |= value << bit_offset;

	if (bit_offset + BitWidth > bitsof<uintptr_t>::value)
	{
		base[1] &= ~shiftleft_d(mask, uintptr_t(0), bit_offset);
		base[1] |= shiftleft_d(value, uintptr_t(0), bit_offset);
	}
}*/

unsigned long check_scan_bit32(unsigned long value)
{
	unsigned long pos;
	if (!_BitScanForward(&pos, value))
	{
		EXIT_WITH_LINENO(FILE_ID_FACTORY);
	}
	return pos;
}

#ifdef _M_X64

template<size_t N>
uintptr_t _hash_map_func(uintptr_t m) //reverse bits
{
	static_assert(sizeof(uintptr_t) == 8, "check uintptr_t size");
	__int64 input = static_cast<__int64>(m);
	__int64 ret = 0;
	//uintptr_t adder = uintptr_t(1) << (N - 1);
	for (size_t i = 0; i < N; ++i)
	{
		if (_bittest64(&input, i))
		{
			_bittestandset64(&ret, N - 1 - i);
		}
	}
	return ret;
}

#else

template<size_t N>
uintptr_t _hash_map_func(uintptr_t m) //reverse bits
{
	static_assert(sizeof(uintptr_t) == 4, "check uintptr_t size");
	long input = static_cast<long>(m);
	long ret = 0;
	//uintptr_t adder = uintptr_t(1) << (N - 1);
	for (size_t i = 0; i < N; ++i)
	{
		if (_bittest(&input, i))
		{
			_bittestandset(&ret, N - 1 - i);
		}
	}
	return ret;
}
#endif