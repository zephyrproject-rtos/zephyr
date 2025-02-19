/*  Taken from https://sourceforge.net/p/strobe (MIT Licence) */

/* SPDX-License-Identifier: MIT */

/**
 * @cond internal
 * @file x25519.c
 * @copyright
 *   Copyright (c) 2015-2016 Cryptography Research, Inc.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 * @author Mike Hamburg
 * @brief Key exchange and signatures based on X25519.
 */
#include <stdint.h>
#include "x25519.h"
/* #include "strobe.h" */
/* #include "strobe_config.h" */
/*  STROBE header replacement */
#include <string.h>
#define X25519_WBITS           32
#define X25519_SUPPORT_SIGN    0
#define X25519_MEMCPY_PARAMS   1
#define X25519_USE_POWER_CHAIN 1
#if BYTE_ORDER == LITTLE_ENDIAN
static inline uint32_t eswap_letoh_32(uint32_t w)
{
	return w;
}
#else
#error "Fix eswap() on non-little-endian machine"
#endif

#if X25519_WBITS == 64
typedef uint64_t limb_t;
typedef __uint128_t dlimb_t;
typedef __int128_t sdlimb_t;
#define eswap_limb eswap_letoh_64
#define LIMB(x)    x##ull
#elif X25519_WBITS == 32
typedef uint32_t limb_t;
typedef uint64_t dlimb_t;
typedef int64_t sdlimb_t;
#define eswap_limb eswap_letoh_32
#define LIMB(x)    (uint32_t)(x##ull), (uint32_t)((x##ull) >> 32)
#else
#error "Need to know X25519_WBITS"
#endif

#define NLIMBS (256 / X25519_WBITS)
typedef limb_t fe[NLIMBS];

#if X25519_SUPPORT_SIGN
typedef limb_t scalar_t[NLIMBS];
static const limb_t MONTGOMERY_FACTOR = (limb_t)0xd2b51da312547e1bull;
static const scalar_t sc_p = {LIMB(0x5812631a5cf5d3ed), LIMB(0x14def9dea2f79cd6),
			      LIMB(0x0000000000000000), LIMB(0x1000000000000000)},
		      sc_r2 = {LIMB(0xa40611e3449c0f01), LIMB(0xd00e1ba768859347),
			       LIMB(0xceec73d217f5be65), LIMB(0x0399411b7c309a3d)};
#endif

static inline limb_t umaal(limb_t *carry, limb_t acc, limb_t mand, limb_t mier)
{
	dlimb_t tmp = (dlimb_t)mand * mier + acc + *carry;

	*carry = tmp >> X25519_WBITS;
	return tmp;
}

/* These functions are implemented in terms of umaal on ARM */
static inline limb_t adc(limb_t *carry, limb_t acc, limb_t mand)
{
	dlimb_t total = (dlimb_t)*carry + acc + mand;

	*carry = total >> X25519_WBITS;
	return total;
}

static inline limb_t adc0(limb_t *carry, limb_t acc)
{
	dlimb_t total = (dlimb_t)*carry + acc;

	*carry = total >> X25519_WBITS;
	return total;
}

/* Precondition: carry is small.
 * Invariant: result of propagate is < 2^255 + 1 word
 * In particular, always less than 2p.
 * Also, output x >= min(x,19)
 */
static void propagate(fe x, limb_t over)
{
	unsigned int i;

	over = x[NLIMBS - 1] >> (X25519_WBITS - 1) | over << 1;
	x[NLIMBS - 1] &= ~((limb_t)1 << (X25519_WBITS - 1));

	limb_t carry = over * 19;

	for (i = 0; i < NLIMBS; i++) {
		x[i] = adc0(&carry, x[i]);
	}
}

static void add(fe out, const fe a, const fe b)
{
	unsigned int i;
	limb_t carry = 0;

	for (i = 0; i < NLIMBS; i++) {
		out[i] = adc(&carry, a[i], b[i]);
	}
	propagate(out, carry);
}

static void sub(fe out, const fe a, const fe b)
{
	unsigned int i;
	sdlimb_t carry = -38;

	for (i = 0; i < NLIMBS; i++) {
		out[i] = carry = carry + a[i] - b[i];
		carry >>= X25519_WBITS;
	}
	propagate(out, 1 + carry);
}

static void __attribute__((unused)) swapin(limb_t *x, const uint8_t *in)
{
	memcpy(x, in, sizeof(fe));
	unsigned int i;

	for (i = 0; i < NLIMBS; i++) {
		x[i] = eswap_limb(x[i]);
	}
}

static void __attribute__((unused)) swapout(uint8_t *out, limb_t *x)
{
	unsigned int i;

	for (i = 0; i < NLIMBS; i++) {
		x[i] = eswap_limb(x[i]);
	}
	memcpy(out, x, sizeof(fe));
}

static void mul(fe out, const fe a, const fe b, unsigned int nb)
{
	/* GCC at least produces pretty decent asm for this, so don't need to have dedicated asm. */
	limb_t accum[2 * NLIMBS] = {0};
	unsigned int i, j;

	limb_t carry2;

	for (i = 0; i < nb; i++) {
		carry2 = 0;
		limb_t mand = b[i];

		for (j = 0; j < NLIMBS; j++) {
			accum[i + j] = umaal(&carry2, accum[i + j], mand, a[j]);
		}
		accum[i + j] = carry2;
	}

	carry2 = 0;
	const limb_t mand = 38;

	for (j = 0; j < NLIMBS; j++) {
		out[j] = umaal(&carry2, accum[j], mand, accum[j + NLIMBS]);
	}
	propagate(out, carry2);
}

static void sqr(fe out, const fe a)
{
	mul(out, a, a, NLIMBS);
}
static void mul1(fe out, const fe a)
{
	mul(out, a, out, NLIMBS);
}
static void sqr1(fe a)
{
	mul1(a, a);
}

static void condswap(limb_t a[2 * NLIMBS], limb_t b[2 * NLIMBS], limb_t doswap)
{
	unsigned int i;

	for (i = 0; i < 2 * NLIMBS; i++) {
		limb_t xor = (a[i] ^ b[i]) & doswap;

		a[i] ^= xor;
		b[i] ^= xor;
	}
}

static limb_t canon(fe x)
{
	/* Canonicalize a field element x, reducing it to the least residue
	 * which is congruent to it mod 2^255-19.
	 *
	 * Precondition: x < 2^255 + 1 word
	 */

	/* First, add 19. */
	unsigned int i;
	limb_t carry0 = 19;

	for (i = 0; i < NLIMBS; i++) {
		x[i] = adc0(&carry0, x[i]);
	}
	propagate(x, carry0);

	/* Here, 19 <= x2 < 2^255
	 *
	 * This is because we added 19, so before propagate it can't be less than 19.
	 * After propagate, it still can't be less than 19, because if propagate does
	 * anything it adds 19.
	 *
	 * We know that the high bit must be clear, because either the input was
	 * ~ 2^255 + one word + 19 (in which case it propagates to at most 2 words)
	 * or it was < 2^255.
	 *
	 * So now, if we subtract 19, we will get back to something in [0,2^255-19).
	 */
	sdlimb_t carry = -19;
	limb_t res = 0;

	for (i = 0; i < NLIMBS; i++) {
		res |= x[i] = carry += x[i];
		carry >>= X25519_WBITS;
	}
	return ((dlimb_t)res - 1) >> X25519_WBITS;
}

static const limb_t a24[1] = {121665};

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static void ladder_part1(fe xs[5])
{
	limb_t *x2 = xs[0], *z2 = xs[1], *x3 = xs[2], *z3 = xs[3], *t1 = xs[4];

	add(t1, x2, z2);                                /*  t1 = A */
	sub(z2, x2, z2);                                /*  z2 = B */
	add(x2, x3, z3);                                /*  x2 = C */
	sub(z3, x3, z3);                                /*  z3 = D */
	mul1(z3, t1);                                   /*  z3 = DA */
	mul1(x2, z2);                                   /*  x3 = BC */
	add(x3, z3, x2);                                /*  x3 = DA+CB */
	sub(z3, z3, x2);                                /*  z3 = DA-CB */
	sqr1(t1);                                       /*  t1 = AA */
	sqr1(z2);                                       /*  z2 = BB */
	sub(x2, t1, z2);                                /*  x2 = E = AA-BB */
	mul(z2, x2, a24, ARRAY_SIZE(a24));              /*  z2 = E*a24 */
	add(z2, z2, t1);                                /*  z2 = E*a24 + AA */
}
static void ladder_part2(fe xs[5], const fe x1)
{
	limb_t *x2 = xs[0], *z2 = xs[1], *x3 = xs[2], *z3 = xs[3], *t1 = xs[4];

	sqr1(z3);        /*  z3 = (DA-CB)^2 */
	mul1(z3, x1);    /*  z3 = x1 * (DA-CB)^2 */
	sqr1(x3);        /*  x3 = (DA+CB)^2 */
	mul1(z2, x2);    /*  z2 = AA*(E*a24+AA) */
	sub(x2, t1, x2); /*  x2 = BB again */
	mul1(x2, t1);    /*  x2 = AA*BB */
}

static void x25519_core(fe xs[5], const uint8_t scalar[X25519_BYTES], const uint8_t *x1, int clamp)
{
	int i;
#if X25519_MEMCPY_PARAMS
	fe x1i;

	swapin(x1i, x1);
	x1 = (const uint8_t *)x1;
#endif
	limb_t swap = 0;
	limb_t *x2 = xs[0], *x3 = xs[2], *z3 = xs[3];

	memset(xs, 0, 4 * sizeof(fe));
	x2[0] = z3[0] = 1;
	memcpy(x3, x1, sizeof(fe));

	for (i = 255; i >= 0; i--) {
		uint8_t bytei = scalar[i / 8];

		if (clamp) {
			if (i / 8 == 0) {
				bytei &= ~7;
			} else if (i / 8 == X25519_BYTES - 1) {
				bytei &= 0x7F;
				bytei |= 0x40;
			}
		}
		limb_t doswap = -(limb_t)((bytei >> (i % 8)) & 1);

		condswap(x2, x3, swap ^ doswap);
		swap = doswap;

		ladder_part1(xs);
		ladder_part2(xs, (const limb_t *)x1);
	}
	condswap(x2, x3, swap);
}

int x25519(uint8_t out[X25519_BYTES], const uint8_t scalar[X25519_BYTES],
	   const uint8_t x1[X25519_BYTES], int clamp)
{
	fe xs[5];

	x25519_core(xs, scalar, x1, clamp);

	/* Precomputed inversion chain */
	limb_t *x2 = xs[0], *z2 = xs[1], *z3 = xs[3];
	int i;

	limb_t *prev = z2;
#if X25519_USE_POWER_CHAIN
	static const struct {
		uint8_t a, c, n;
	} steps[13] = {{2, 1, 1},  {2, 1, 1},  {4, 2, 3},  {2, 4, 6},  {3, 1, 1},
		       {3, 2, 12}, {4, 3, 25}, {2, 3, 25}, {2, 4, 50}, {3, 2, 125},
		       {3, 1, 2},  {3, 1, 2},  {3, 1, 1}};

	for (i = 0; i < 13; i++) {
		int j;
		limb_t *a = xs[steps[i].a];

		for (j = steps[i].n; j > 0; j--) {
			sqr(a, prev);
			prev = a;
		}
		mul1(a, xs[steps[i].c]);
	}
#else
	/* Raise to the p-2 = 0x7f..ffeb */
	for (i = 253; i >= 0; i--) {
		sqr(z3, prev);
		prev = z3;
		if (i >= 8 || (0xeb >> i & 1)) {
			mul1(z3, z2);
		}
	}
#endif

	/* Here prev = z3 */
	/* x2 /= z2 */
#if X25519_MEMCPY_PARAMS
	mul1(x2, z3);
	int ret = canon(x2);

	swapout(out, x2);
#else
	mul((limb_t *)out, x2, z3, NLIMBS);
	int ret = canon((limb_t *)out);
#endif

	if (clamp) {
		return ret;
	} else {
		return 0;
	}
}

const uint8_t X25519_BASE_POINT[X25519_BYTES] = {9};

#if X25519_SUPPORT_VERIFY
static limb_t x25519_verify_core(fe xs[5], const limb_t *other1, const uint8_t other2[X25519_BYTES])
{
	limb_t *z2 = xs[1], *x3 = xs[2], *z3 = xs[3];
#if X25519_MEMCPY_PARAMS
	fe xo2;

	swapin(xo2, other2);
#else
	const limb_t *xo2 = (const limb_t *)other2;
#endif

	memcpy(x3, other1, 2 * sizeof(fe));

	ladder_part1(xs);

	/* Here z2 = t2^2 */
	mul1(z2, other1);
	mul1(z2, other1 + NLIMBS);
	mul1(z2, xo2);
	const limb_t sixteen = 16;

	mul(z2, z2, &sixteen, 1);

	mul1(z3, xo2);
	sub(z3, z3, x3);
	sqr1(z3);

	/* check equality */
	sub(z3, z3, z2);

	/* If canon(z2) then both sides are zero.
	 * If canon(z3) then the two sides are equal.
	 *
	 * Reject sigs where both sides are zero, because
	 * that can happen if an input causes the ladder to
	 * return 0/0.
	 */
	return canon(z2) | ~canon(z3);
}

int x25519_verify_p2(const uint8_t response[X25519_BYTES], const uint8_t challenge[X25519_BYTES],
		     const uint8_t eph[X25519_BYTES], const uint8_t pub[X25519_BYTES])
{
	fe xs[7];

	x25519_core(&xs[0], challenge, pub, 0);
	x25519_core(&xs[2], response, X25519_BASE_POINT, 0);
	return x25519_verify_core(&xs[2], xs[0], eph);
}
#endif /*  X25519_SUPPORT_VERIFY */

#if X25519_SUPPORT_SIGN
static void sc_montmul(scalar_t out, const scalar_t a, const scalar_t b)
{
	/**
	 * OK, so carry bounding.  We're using a high carry, so that the
	 * inputs don't have to be reduced.
	 *
	 * First montmul: output < (M^2 + Mp)/M = M+p, subtract p, < M.  This gets rid of high
	 * carry. Second montmul, by r^2 mod p < p: output < (Mp + Mp)/M = 2p, subtract p, < p,
	 * done.
	 */
	unsigned int i, j;
	limb_t hic = 0;

	for (i = 0; i < NLIMBS; i++) {
		limb_t carry = 0, carry2 = 0, mand = a[i], mand2 = MONTGOMERY_FACTOR;

		for (j = 0; j < NLIMBS; j++) {
			limb_t acc = out[j];

			acc = umaal(&carry, acc, mand, b[j]);
			if (j == 0) {
				mand2 *= acc;
			}
			acc = umaal(&carry2, acc, mand2, sc_p[j]);
			if (j > 0) {
				out[j - 1] = acc;
			}
		}

		/* Add two carry registers and high carry */
		out[NLIMBS - 1] = adc(&hic, carry, carry2);
	}

	/* Reduce */
	sdlimb_t scarry = 0;

	for (i = 0; i < NLIMBS; i++) {
		out[i] = scarry = scarry + out[i] - sc_p[i];
		scarry >>= X25519_WBITS;
	}
	limb_t need_add = -(scarry + hic);

	limb_t carry = 0;

	for (i = 0; i < NLIMBS; i++) {
		out[i] = umaal(&carry, out[i], need_add, sc_p[i]);
	}
}

void x25519_sign_p2(uint8_t response[X25519_BYTES], const uint8_t challenge[X25519_BYTES],
		    const uint8_t eph_secret[X25519_BYTES], const uint8_t secret[X25519_BYTES])
{
	/* FUTURE memory/code size: just make eph_secret non-const? */
	scalar_t scalar1;

	swapin(scalar1, eph_secret);

#if X25519_MEMCPY_PARAMS
	scalar_t scalar2, scalar3;

	swapin(scalar2, secret);
	swapin(scalar3, challenge);
	sc_montmul(scalar1, scalar2, scalar3);
	memset(scalar2, 0, sizeof(scalar2));
	sc_montmul(scalar2, scalar1, sc_r2);
	swapout(response, scalar2);
#else
	sc_montmul(scalar1, (const limb_t *)secret, (const limb_t *)challenge);
	memset(response, 0, X25519_BYTES);
	sc_montmul((limb_t *)response, scalar1, sc_r2);
#endif
}
#endif /*  X25519_SUPPORT_SIGN */
