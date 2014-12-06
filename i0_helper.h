#include <stdint.h>
#include <cstring>
#include <tuple>
/*
Require A C++11 Compiler to compile
*/

/****************************************/
//The trivial low level helpers
//Should be Placed with Real inline Assembly 

extern void* Crt0 = 0;
void* _i0_spawn(void* fi, const void* using_range, const void* watching_range)
{
	return 0;
}
/*****************************************/

/*
Memory Layout for the stack of the newly created runner:

RunnerWrapperInfo_t			*********************
							*  ParaPackSize     * ----> sizeof(RunnerWrapperInfo_t) + sizeof(RunnerSelfInfo_t)
							*-------------------*
							*  EntryPointHelper * ----> Entry Helper to extract Parameters
							*********************
RunnerSelfInfo_t			*  RealEntryPoint   * ----> The EntryPoint Passed By User
							*-------------------*
							*					*
							*  (ParaMeters)		* ----> The Actual Parameters
							*					*
							*********************

		The Crt0 Shall Adjust the Stack Pointer Upon a new runner starts and pass the 
		pointer to "ParaMeters" to the "EntryPointHelper" Function
*/

/*
Usage:
	NewRunner(
			Runner( [Function], [Parameters...] ),
			[UsingRange( [Variable] | [Variable_First, Variable_Last] )...] or EmptyRange,
			[WatchRange( [Variable] | [Variable_First, Variable_Last] )...] or EmptyRange
	);

For Example:

int Entry(int arg1, char arg2);
char global1;
int global2;
double global3[10];

......
		NewRunner(
			Runner( Entry, 10, 'A' ),
			UsingRange(global1)(global3[5], global3[10]),
			WatchRange(global2)(global3[0], global3[5])
		);
......

*/

namespace i0_internal{
	template<size_t ...>
	struct seq{};

	template<size_t N, size_t ...S>
	struct gens : gens < N - 1, N - 1, S... > { };

	template<size_t ...S>
	struct gens < 0, S... > {
		typedef seq<S...> type;
	};

	typedef void(*EntryFuncHelper_t)(void*);

	struct RunnerWrapperInfo_t{
		EntryFuncHelper_t EntryHlp;
		uint64_t ParaPackSize;
		RunnerWrapperInfo_t(EntryFuncHelper_t _EntryHlp, uint64_t _ParaPackSize) : EntryHlp(_EntryHlp), ParaPackSize(_ParaPackSize){}
	};

	template<typename Ret, typename... Args>
	struct RunnerSelfInfo_t{
		typedef Ret(*RunnerEntry_t)(Args...);
		typedef ::std::tuple<Args...> RunnerParaPack_t;
		RunnerParaPack_t Paras;
		RunnerEntry_t Entry;
		RunnerSelfInfo_t(RunnerEntry_t _Entry, Args... args) : Paras(args...), Entry(_Entry){}
	};

	template<typename Ret, typename... Args>
	struct RunnerHelper;

	template<typename Ret, typename... Args>
	struct RunnerHelper < Ret(Args...) > : RunnerSelfInfo_t<Ret, Args...>, RunnerWrapperInfo_t
	{
		typedef RunnerSelfInfo_t<Ret, Args...> SelfInfoTy;
		typedef RunnerHelper < Ret(Args...) > ThisTy;

		template<size_t... S>
		static Ret Run(seq<S...>, const SelfInfoTy* pSelfInfo){
			return pSelfInfo->Entry(::std::get<S>(pSelfInfo->Paras)...);
		}

		static void EntryFuncHelper(void* pStartupInfo)
		{
			const SelfInfoTy* pSelfInfo = (const SelfInfoTy*)pStartupInfo;
			Run(typename gens<sizeof...(Args)>::type(), pSelfInfo);
		}

		explicit RunnerHelper(Ret(*Entry)(Args...), Args... args) :SelfInfoTy(Entry, args...), RunnerWrapperInfo_t(EntryFuncHelper, sizeof(ThisTy))
		{
			static_assert(offsetof(ThisTy, ParaPackSize)
				== sizeof(ThisTy) - sizeof(RunnerWrapperInfo_t::ParaPackSize), " Check ParaPackSize Offset");
		}
	};

	struct RangeDesc_t{
		uint64_t Base;
		uint64_t Len;
		RangeDesc_t() = delete;

		template<typename T>
		RangeDesc_t(const T& var) : Base((uint64_t)&var), Len((char*)(&var + 1) - (char*)&var)
		{}

		template<typename T>
		RangeDesc_t(const T& var_low, const T& var_high) : Base((uint64_t)&var_low), Len((char*)&var_high - (char*)&var_low)
		{}
	};

	template<size_t C>
	struct Ranges;

	template<size_t C>
	struct Ranges : Ranges < C - 1 >
	{
		typedef Ranges<C - 1> Base;
		RangeDesc_t CurrentRange;

		Ranges<C>(size_t Cnt, const Ranges& Other) :
			Base(Cnt, Other),
			CurrentRange(Other.CurrentRange)
		{}

		template<typename T>
		Ranges(size_t Cnt, const Base& OtherBase, const T& var) :
			Base(Cnt, OtherBase),
			CurrentRange(var)
		{}

		template<typename T>
		Ranges(size_t Cnt, const Base& OtherBase, const T& var_lo, const T& var_hi) :
			Base(Cnt, OtherBase),
			CurrentRange(var_lo, var_hi)
		{}

		template<typename T>
		Ranges<C + 1> operator () (const T& var) const
		{
			return Ranges<C + 1>(C + 1, *this, var);
		}

		template<typename T>
		Ranges<C + 1> operator () (const T& var_lo, const T& var_hi) const
		{
			return Ranges<C + 1>(C + 1, *this, var_lo, var_hi);
		}
		Ranges() = delete;
	};
	template<>
	struct Ranges < 0 >
	{
		uint64_t Cnt;
		Ranges() :Cnt(0){}
		Ranges(size_t _Cnt, const Ranges&) : Cnt(_Cnt){}

		template<typename T>
		Ranges<1> operator () (const T& var) const
		{
			return Ranges<1>(1, *this, var);
		}

		template<typename T>
		Ranges<1> operator () (const T& var_lo, const T& var_hi) const
		{
			return Ranges<1>(1, *this, var_lo, var_hi);
		}
	};
}

#define EmptyRange i0_internal::Ranges<0>()
#define WatchRange EmptyRange
#define UsingRange EmptyRange	

template<typename FuncType, typename... ArgsType>
i0_internal::RunnerHelper<FuncType> Runner(FuncType* F, ArgsType... args)
{
	return i0_internal::RunnerHelper<FuncType>(F, args...);
}

template<typename RunnerType>
void NewRunner(const RunnerType& RunnerInfo, const i0_internal::Ranges<0>& Using = EmptyRange, const i0_internal::Ranges<0>& Watching = EmptyRange)
{
	const i0_internal::RunnerWrapperInfo_t& _chk_type = RunnerInfo;
	(void)_chk_type;
	void* new_sp = _i0_spawn(Crt0, &Using, &Watching);
	RunnerType* pInfo = (RunnerType*)((char*)new_sp - sizeof(RunnerType));
	new (pInfo)RunnerType(RunnerInfo);
}
