#pragma once
/*
	@file
	@brief low level functions
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/

#include <mcl/config.hpp>
#include <mcl/util.hpp>
#include <assert.h>
#ifndef MCL_STANDALONE
#include <stdio.h>
#endif

//#define MCL_BINT_ASM 1
#ifdef MCL_WASM32
	#define MCL_BINT_ASM 0
#endif
#ifndef MCL_BINT_ASM
	#define MCL_BINT_ASM 1
#endif

#if CYBOZU_HOST == CYBOZU_HOST_INTEL && MCL_SIZEOF_UNIT == 8 && MCL_BINT_ASM == 1 && !defined(MCL_BINT_ASM_X64)
	#define MCL_BINT_ASM_X64 1
extern "C" void mclb_disable_fast(void);

#if defined(_MSC_VER) && (_MSC_VER < 1920)
extern "C" unsigned __int64 mclb_udiv128(
   unsigned __int64 highDividend,
   unsigned __int64 lowDividend,
   unsigned __int64 divisor,
   unsigned __int64 *remainder
);
#endif

#else
	#define MCL_BINT_ASM_X64 0
#endif

namespace mcl { namespace bint {

typedef Unit (*u_ppp)(Unit*, const Unit*, const Unit*);
typedef Unit (*u_ppu)(Unit*, const Unit*, Unit);
typedef void (*void_pppp)(Unit*, const Unit*, const Unit*, const Unit*);
typedef void (*void_ppp)(Unit*, const Unit*, const Unit*);
typedef void (*void_pp)(Unit*, const Unit*);

#if !defined(MCL_DONT_CALL_INITBINT) && MCL_BINT_ASM_X64 == 1
MCL_DLL_API void initBint(); // disable mulx/adox/adcx if they are not available on x64. Do nothing in other environments.

namespace impl {
static struct Init {
	Init()
	{
		initBint();
	}
} g_init;
}
#endif

// show integer as little endian
template<class T>
inline void dump(const T *x, size_t n, const char *msg = "")
{
#ifdef MCL_STANDALONE
	(void)x;
	(void)n;
	(void)msg;
#else
	if (msg) printf("%s ", msg);
	for (size_t i = 0; i < n; i++) {
		T v = x[n - 1 - i];
		for (size_t j = 0; j < sizeof(T); j++) {
			printf("%02x", uint8_t(v >> (sizeof(T) - 1 - j) * 8));
		}
	}
	printf("\n");
#endif
}

/*
	[H:L] <= x * y
	@return L
*/
inline uint32_t mulUnit1(uint32_t *pH, uint32_t x, uint32_t y)
{
	uint64_t t = uint64_t(x) * y;
	*pH = uint32_t(t >> 32);
	return uint32_t(t);
}

/*
	q = [H:L] / y
	r = [H:L] % y
	return q
*/
inline uint32_t divUnit1(uint32_t *pr, uint32_t H, uint32_t L, uint32_t y)
{
	assert(H < y);
	uint64_t t = (uint64_t(H) << 32) | L;
	uint32_t q = uint32_t(t / y);
	*pr = uint32_t(t % y);
	return q;
}

#if MCL_SIZEOF_UNIT == 8

#if !defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__clang__)
typedef __attribute__((mode(TI))) unsigned int uint128_t;
#define MCL_DEFINED_UINT128_T
#endif

inline uint64_t mulUnit1(uint64_t *pH, uint64_t x, uint64_t y)
{
#ifdef MCL_DEFINED_UINT128_T
	uint128_t t = uint128_t(x) * y;
	*pH = uint64_t(t >> 64);
	return uint64_t(t);
#else
	return _umul128(x, y, pH);
#endif
}

inline uint64_t divUnit1(uint64_t *pr, uint64_t H, uint64_t L, uint64_t y)
{
	assert(H < y);
#ifdef MCL_DEFINED_UINT128_T
	uint128_t t = (uint128_t(H) << 64) | L;
	uint64_t q = uint64_t(t / y);
	*pr = uint64_t(t % y);
	return q;
#elif defined(_MSC_VER) && (_MSC_VER < 1920)
	return mclb_udiv128(H, L, y, pr);
#else
	return _udiv128(H, L, y, pr);
#endif
}

#endif // MCL_SIZEOF_UNIT == 8

// z[N] = x[N] + y[N] and return CF(0 or 1)
template<size_t N>Unit addT(Unit *z, const Unit *x, const Unit *y);
// z[N] = x[N] - y[N] and return CF(0 or 1)
template<size_t N>Unit subT(Unit *z, const Unit *x, const Unit *y);
// z[N] = x[N] + y[N]. assume x, y are Not Full bit
template<size_t N>void addNFT(Unit *z, const Unit *x, const Unit *y);
// z[N] = x[N] - y[N] and return CF(0 or 1). assume x, y are Not Full bit
template<size_t N>Unit subNFT(Unit *z, const Unit *x, const Unit *y);
// [ret:z[N]] = x[N] * y
template<size_t N>Unit mulUnitT(Unit *z, const Unit *x, Unit y);
// [ret:z[N]] = z[N] + x[N] * y
template<size_t N>Unit mulUnitAddT(Unit *z, const Unit *x, Unit y);
// z[2N] = x[N] * y[N]
template<size_t N>void mulT(Unit *pz, const Unit *px, const Unit *py);
// y[2N] = x[N] * x[N]
template<size_t N>void sqrT(Unit *py, const Unit *px);

Unit addN(Unit *z, const Unit *x, const Unit *y, size_t n);
Unit subN(Unit *z, const Unit *x, const Unit *y, size_t n);
void addNFN(Unit *z, const Unit *x, const Unit *y, size_t n);
Unit subNFN(Unit *z, const Unit *x, const Unit *y, size_t n);
Unit mulUnitN(Unit *z, const Unit *x, Unit y, size_t n);
Unit mulUnitAddN(Unit *z, const Unit *x, Unit y, size_t n);
// z[n * 2] = x[n] * y[n]
MCL_DLL_API void mulN(Unit *z, const Unit *x, const Unit *y, size_t n);
// y[n * 2] = x[n] * x[n]
MCL_DLL_API void sqrN(Unit *y, const Unit *x, size_t xn);
// z[xn * yn] = x[xn] * y[ym]
MCL_DLL_API void mulNM(Unit *z, const Unit *x, size_t xn, const Unit *y, size_t yn);

// explicit specialization of template functions and external asm functions
#include "bint_proto.hpp"

template<size_t N, typename T>
void copyT(T *y, const T *x)
{
	for (size_t i = 0; i < N; i++) y[i] = x[i];
}

// y[n] = x[n]
template<typename T>
void copyN(T *y, const T *x, size_t n)
{
	for (size_t i = 0; i < n; i++) y[i] = x[i];
}

template<size_t N, typename T>
void clearT(T *x)
{
	for (size_t i = 0; i < N; i++) x[i] = 0;
}

// x[n] = 0
template<typename T>
void clearN(T *x, size_t n)
{
	for (size_t i = 0; i < n; i++) x[i] = 0;
}

// return true if x[] == 0
template<size_t N, typename T>
bool isZeroT(const T *x)
{
	for (size_t i = 0; i < N; i++) if (x[i]) return false;
	return true;
}

template<typename T>
bool isZeroN(const T *x, size_t n)
{
	for (size_t i = 0; i < n; i++) if (x[i]) return false;
	return true;
}

// return the real size of x
// return 1 if x[n] == 0
template<typename T>
size_t getRealSize(const T *x, size_t n)
{
	while (n > 0) {
		if (x[n - 1]) break;
		n--;
	}
	return n > 0 ? n : 1;
}

template<size_t N, typename T>
int cmpT(const T *px, const T *py)
{
	for (size_t i = 0; i < N; i++) {
		const T x = px[N - 1 - i];
		const T y = py[N - 1 - i];
		if (x != y) return x > y ? 1 : -1;
	}
	return 0;
}

// true if x[N] == y[N]
template<size_t N, typename T>
bool cmpEqT(const T *px, const T *py)
{
	for (size_t i = 0; i < N; i++) {
		if (px[i] != py[i]) return false;
	}
	return true;
}

// true if x[N] >= y[N]
template<size_t N, typename T>
bool cmpGeT(const T *px, const T *py)
{
	for (size_t i = 0; i < N; i++) {
		const T x = px[N - 1 - i];
		const T y = py[N - 1 - i];
		if (x > y) return true;
		if (x < y) return false;
	}
	return true;
}

// true if x[N] > y[N]
template<size_t N, typename T>
bool cmpGtT(const T *px, const T *py)
{
	for (size_t i = 0; i < N; i++) {
		const T x = px[N - 1 - i];
		const T y = py[N - 1 - i];
		if (x > y) return true;
		if (x < y) return false;
	}
	return false;
}

// true if x[N] <= y[N]
template<size_t N, typename T>
bool cmpLeT(const T *px, const T *py)
{
	return !cmpGtT<N>(px, py);
}

// true if x[N] < y[N]
template<size_t N, typename T>
bool cmpLtT(const T *px, const T *py)
{
	return !cmpGeT<N>(px, py);
}

// true if x[] == y[]
template<typename T>
bool cmpEqN(const T *px, const T *py, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (px[i] != py[i]) return false;
	}
	return true;
}

// true if x[n] >= y[n]
template<typename T>
bool cmpGeN(const T *px, const T *py, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		const T x = px[n - 1 - i];
		const T y = py[n - 1 - i];
		if (x > y) return true;
		if (x < y) return false;
	}
	return true;
}

// true if x[n] > y[n]
template<typename T>
bool cmpGtN(const T *px, const T *py, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		const T x = px[n - 1 - i];
		const T y = py[n - 1 - i];
		if (x > y) return true;
		if (x < y) return false;
	}
	return false;
}

// true if x[n] <= y[n]
template<typename T>
bool cmpLeN(const T *px, const T *py, size_t n)
{
	return !cmpGtN(px, py, n);
}

// true if x[n] < y[n]
template<typename T>
bool cmpLtN(const T *px, const T *py, size_t n)
{
	return !cmpGeN(px, py, n);
}

template<typename T>
int cmpN(const T *px, const T *py, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		const T x = px[n - 1 - i];
		const T y = py[n - 1 - i];
		if (x != y) return x > y ? 1 : -1;
	}
	return 0;
}

// [return:z[N]] = x[N] << bit
// 0 < bit < UnitBitSize
template<size_t N>
Unit shlT(Unit *pz, const Unit *px, Unit bit)
{
	assert(0 < bit && bit < UnitBitSize);
	size_t bitRev = UnitBitSize - bit;
	Unit prev = px[N - 1];
	Unit keep = prev;
	for (size_t i = N - 1; i > 0; i--) {
		Unit t = px[i - 1];
		pz[i] = (prev << bit) | (t >> bitRev);
		prev = t;
	}
	pz[0] = prev << bit;
	return keep >> bitRev;
}

// z[N] = x[N] >> bit
// 0 < bit < UnitBitSize
template<size_t N>
void shrT(Unit *pz, const Unit *px, size_t bit)
{
	assert(0 < bit && bit < UnitBitSize);
	size_t bitRev = UnitBitSize - bit;
	Unit prev = px[0];
	for (size_t i = 1; i < N; i++) {
		Unit t = px[i];
		pz[i - 1] = (prev >> bit) | (t << bitRev);
		prev = t;
	}
	pz[N - 1] = prev >> bit;
}

// [return:z[N]] = x[N] << y
// 0 < y < UnitBitSize
MCL_DLL_API Unit shlN(Unit *pz, const Unit *px, Unit bit, size_t n);

// z[n] = x[n] >> bit
// 0 < bit < UnitBitSize
MCL_DLL_API void shrN(Unit *pz, const Unit *px, size_t bit, size_t n);

/*
	generic version
	y[yn] = x[xn] << bit
	yn = xn + roundUp(bit, UnitBitSize)
	accept y == x
	return yn
*/
MCL_DLL_API size_t shiftLeft(Unit *y, const Unit *x, size_t bit, size_t xn);

/*
	generic version
	y[yn] = x[xn] >> bit
	yn = xn - bit / UnitBitSize
	return yn
*/
MCL_DLL_API size_t shiftRight(Unit *y, const Unit *x, size_t bit, size_t xn);

// [return:y[n]] += x
MCL_DLL_API Unit addUnit(Unit *y, size_t n, Unit x);

// y[n] -= x, return CF
MCL_DLL_API Unit subUnit(Unit *y, size_t n, Unit x);

/*
	q[] = x[] / y
	@retval r = x[] % y
	accept q == x
*/
MCL_DLL_API Unit divUnit(Unit *q, const Unit *x, size_t n, Unit y);

/*
	q[] = x[] / y
	@retval r = x[] % y
*/
MCL_DLL_API Unit modUnit(const Unit *x, size_t n, Unit y);

/*
	y must be UnitBitSize * N bit
	x[xn] %= y[yn]
	q[qn] = x[xn] / y[yn] if q != NULL
	return new xn
*/
MCL_DLL_API size_t divFullBit(Unit *q, size_t qn, Unit *x, size_t xn, const Unit *y, size_t yn);

/*
	assume xn <= yn
	x[xn] %= y[yn]
	q[qn] = x[xn] / y[yn] if q != NULL
	assume(n >= 2);
	return new xn (1 if modulo is zero) if computed else 0
*/
MCL_DLL_API Unit divSmall(Unit *q, size_t qn, Unit *x, size_t xn, const Unit *y, size_t yn);

/*
	x[xn] %= y[yn]
	q[qn] = x[xn] / y[yn] ; qn == xn - yn + 1 if xn >= yn else 1
	allow q == 0
	return new xn
	@note x[new xn:xn] may not be cleared
*/
MCL_DLL_API size_t div(Unit *q, size_t qn, Unit *x, size_t xn, const Unit *y, size_t yn);

MCL_DLL_API void mod_SECP256K1(Unit *z, const Unit *x, const Unit *p);
MCL_DLL_API void mul_SECP256K1(Unit *z, const Unit *x, const Unit *y, const Unit *p);
MCL_DLL_API void sqr_SECP256K1(Unit *y, const Unit *x, const Unit *p);

// x &= (1 << bitSize) - 1
MCL_DLL_API void maskN(Unit *x, size_t n, size_t bitSize);

// ppLow = Unit(p)
inline Unit getMontgomeryCoeff(Unit pLow, size_t bitSize = sizeof(Unit) * 8)
{
	Unit pp = 0;
	Unit t = 0;
	Unit x = 1;
	for (size_t i = 0; i < bitSize; i++) {
		if ((t & 1) == 0) {
			t += pLow;
			pp += x;
		}
		t >>= 1;
		x <<= 1;
	}
	return pp;
}

struct SmallModP {
	const size_t d = 16; // d = 26 if use double in approx
	const size_t maxE_ = d - 2;
	const Unit *p_;
	size_t n_;
	size_t l_;
	uint32_t p0_;

	SmallModP()
		: n_(0)
		, l_(0)
		, p0_(0)
	{
	}
	// p must not be temporary.
	void init(const Unit *p, size_t n)
	{
		p_ = p;
		n_ = n;
		l_ = mcl::fp::getBitSize(p, n);
		Unit *t = (Unit*)CYBOZU_ALLOCA((n_+1)*sizeof(Unit));
		mcl::bint::clearN(t, n_+1);
		size_t pos = d + l_ - 1;
		{
			size_t q = pos / MCL_UNIT_BIT_SIZE;
			size_t r = pos % MCL_UNIT_BIT_SIZE;
			t[q] = Unit(1) << r;
		}
		// p0 = 2**(d+l-1)/p
		Unit q[2];
		mcl::bint::div(q, 2, t, n_+1, p, n_);
		assert(q[1] == 0);
		p0_ = uint32_t(q[0]);
	}
	Unit approx(Unit x0, size_t a) const
	{
//		uint64_t t = uint64_t(double(x0) * double(p0_)); // for d = 26
		uint32_t t = uint32_t(x0 * p0_);
		return Unit(t >> (2 * d + l_ - 1 - a));
	}
	// x[xn] %= p
	// the effective range of return value is [0, n_)
	bool quot(Unit *pQ, const Unit *x, size_t xn) const
	{
		size_t a = mcl::fp::getBitSize(x, xn);
		if (a < l_) {
			*pQ = 0;
			return true;
		}
		size_t e = a - l_ + 1;
		if (e > maxE_) return false;
		Unit x0 = mcl::fp::getUnitAt(x, xn, a - d);
		*pQ = approx(x0, a);
		return true;
	}
	// return false if x[0, xn) is large
	bool mod(Unit *z, const Unit *x, size_t xn) const
	{
		assert(xn <= n_ + 1);
		Unit Q;
		if (!quot(&Q, x, xn)) return false;
		if (Q == 0) return true;
		Unit *t = (Unit*)CYBOZU_ALLOCA((n_+1)*sizeof(Unit));
		t[n_] = mcl::bint::mulUnitN(t, p_, Q, n_);
		mcl::bint::subN(t, x, t, n_+1);
		if (mcl::bint::cmpGeN(t, p_, n_)) {
			mcl::bint::subN(z, t, p_, n_);
		} else {
			mcl::bint::copyN(z, t, n_);
		}
		return true;
	}
	// return false if x[0, xn) is large
	template<size_t N>
	bool modT(Unit z[N], const Unit *x, size_t xn) const
	{
		assert(xn <= N + 1);
		Unit Q;
		if (!quot(&Q, x, xn)) return false;
		if (Q == 0) return true;
		Unit t[N+1];
		t[N] = mcl::bint::mulUnitT<N>(t, p_, Q);
		mcl::bint::subT<N+1>(t, x, t);
		if (mcl::bint::cmpGeT<N>(t, p_)) {
			mcl::bint::subT<N>(z, t, p_);
		} else {
			mcl::bint::copyT<N>(z, t);
		}
		return true;
	}
	template<size_t N>
	static bool mulUnit(const SmallModP& smp, Unit z[N], const Unit x[N], Unit y)
	{
		Unit xy[N+1];
		xy[N] = mulUnitT<N>(x, y);
		return modT<N>(z, xy, N+1);
	}
};

} } // mcl::bint

