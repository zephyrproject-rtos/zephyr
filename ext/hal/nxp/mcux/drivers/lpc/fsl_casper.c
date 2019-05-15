/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_casper.h"
#include <math.h> /* ceil TODO check if really need it */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.casper"
#endif

#define CASPER_RAM_BASE_NS (FSL_FEATURE_CASPER_RAM_BASE_ADDRESS)

#if defined(FSL_FEATURE_CASPER_RAM_IS_INTERLEAVED) && FSL_FEATURE_CASPER_RAM_IS_INTERLEAVED
#define CASPER_RAM_OFFSET (0xE)
#define INTERLEAVE(addr)                                                                                             \
    (((((((addr) >> 2) & 0x00000001) << CASPER_RAM_OFFSET) + (((addr) >> 3) << 2) + ((addr)&0x00000003)) & 0xFFFF) | \
     s_casperRamBase)
#define DEINTERLEAVE(addr) INTERLEAVE(addr)
#define GET_WORD(addr) (*((uint32_t *)DEINTERLEAVE((uint32_t)(addr))))
#define GET_DWORD(addr) (((uint64_t)GET_WORD(addr)) | (((uint64_t)GET_WORD(((uint32_t)(addr)) + 4)) << 32))
#define SET_WORD(addr, value) *((uint32_t *)INTERLEAVE((uint32_t)(addr))) = ((uint32_t)(value))
#define SET_DWORD(addr, value)                                                            \
    do                                                                                    \
    {                                                                                     \
        SET_WORD(addr, (uint32_t)(value & 0xFFFFFFFF));                                   \
        SET_WORD(((uint32_t)(addr)) + 4, (uint32_t)((value & 0xFFFFFFFF00000000) >> 32)); \
    } while (0)

/* memcopy is always word aligned */
/* interleaved to interleaved
  static void CASPER_MEMCPY_I2I(void *dst, const void *src, size_t siz)
 */
#define CASPER_MEMCPY_I2I(dst, src, siz)                             \
    \
{                                                             \
        uint32_t *dst32 = (uint32_t *)dst, *src32 = (uint32_t *)src; \
        int i;                                                       \
        for (i = 0; i < siz / 4; i++)                                \
        {                                                            \
            SET_WORD(&dst32[i], GET_WORD(&src32[i]));                \
        }                                                            \
    \
}

/* interleaved to non-interleaved
   static void CASPER_MEMCPY_I2N(void *dst, const void *src, size_t siz)
 */
#define CASPER_MEMCPY_I2N(dst, src, siz)                             \
    \
{                                                             \
        uint32_t *dst32 = (uint32_t *)dst, *src32 = (uint32_t *)src; \
        int i;                                                       \
        for (i = 0; i < siz / 4; i++)                                \
        {                                                            \
            dst32[i] = GET_WORD(&src32[i]);                          \
        }                                                            \
    \
}

/* non-interleaved to interleaved
   static void CASPER_MEMCPY_N2I(void *dst, const void *src, size_t siz)
 */
#define CASPER_MEMCPY_N2I(dst, src, siz)                                      \
    \
{                                                                      \
        volatile uint32_t *dst32 = (uint32_t *)dst, *src32 = (uint32_t *)src; \
        volatile int i;                                                       \
        for (i = 0; i < siz / 4; i++)                                         \
        {                                                                     \
            SET_WORD(&dst32[i], src32[i]);                                    \
        }                                                                     \
    \
}
#else
#define GET_WORD(addr) (*((uint32_t *)(addr)))
#define GET_DWORD(addr) (*((uint64_t *)(addr)))
#define SET_WORD(addr, value) *((uint32_t *)(addr)) = ((uint32_t)(value))
#define SET_DWORD(addr, value) *((uint64_t *)(addr)) = ((uint64_t)(value))

#define CASPER_MEMCPY_I2I(dst, src, siz) memcpy(dst, src, siz)
#define CASPER_MEMCPY_I2N(dst, src, siz) memcpy(dst, src, siz)
#define CASPER_MEMCPY_N2I(dst, src, siz) memcpy(dst, src, siz)
#endif

#define WORK_BUFF_MUL4 (N_wordlen_max * 4 + 2) /* ! working buffer is 4xN_wordlen to allow in place math */
#define N_bytelen (N_wordlen * 4)              /*  for memory copy and the like */
#define N_dwordlen (N_wordlen / 2)

#define PreZeroW(i, w_out)             \
    for (i = 0; i < N_wordlen; i += 4) \
    {                                  \
        SET_WORD(&w_out[i + 0], 0);    \
        SET_WORD(&w_out[i + 1], 0);    \
        SET_WORD(&w_out[i + 2], 0);    \
        SET_WORD(&w_out[i + 3], 0);    \
    } /*  unrolled partly */
#define PreZeroW2up(i, w_out)                       \
    for (i = N_wordlen; i <= N_wordlen * 2; i += 4) \
    {                                               \
        SET_WORD(&w_out[i + 0], 0);                 \
        SET_WORD(&w_out[i + 1], 0);                 \
        SET_WORD(&w_out[i + 2], 0);                 \
        SET_WORD(&w_out[i + 3], 0);                 \
    } /*  unrolled partly */

/* Macros for the ECC component in Casper */

/* CASPER memory layout for ECC */
#define CASPER_NUM_LIMBS (NUM_LIMBS + 4) // number of limbs needed by CASPER is 2 double words longer

#define CASPER_MEM ((uint32_t *)msg_ret)
#define CASPER_OFFSET CASPER_NUM_LIMBS // offset in the CASPER memory where we can start writing

#define MOD_SCRATCH_START (CASPER_OFFSET)
#define MOD_SCRATCH_SIZE (1 * CASPER_NUM_LIMBS)

#define INOUT_SCRATCH_START (MOD_SCRATCH_START + MOD_SCRATCH_SIZE)
#define INOUT_SCRATCH_SIZE ((3 * 3) * CASPER_NUM_LIMBS)

#define ECC_SCRATCH_START (INOUT_SCRATCH_START + INOUT_SCRATCH_SIZE)
#define ECC_SCRATCH_SIZE (9 * CASPER_NUM_LIMBS)

#define LUT_SCRATCH_START (ECC_SCRATCH_START + ECC_SCRATCH_SIZE)
#define LUT_SCRATCH_SIZE (48 * NUM_LIMBS + 3 * CASPER_NUM_LIMBS)

/* Currently these macros work on 32-bit platforms  */

#define add(c1, c0, a, b) \
    \
do                 \
    {                     \
        uint32_t _t;      \
        _t = a + b;       \
        c1 = (_t < a);    \
        c0 = _t;          \
    \
}                  \
    while (0)

#define add_cout(carry, c, a, b) add(carry, c, a, b)

#define add_cout_cin(carryout, c, a, b, carryin) \
    do                                           \
    {                                            \
        uint64_t _t = (uint64_t)a + b + carryin; \
        c = (uint32_t)_t;                        \
        carryout = (uint32_t)(_t >> 32);         \
    } while (0)

#define sub_borrowout(borrow, c, a, b) \
    do                                 \
    {                                  \
        uint32_t _b = (b > a);         \
        c = a - b;                     \
        borrow = _b;                   \
    } while (0)

#define sub_borrowin_borrowout(borrowout, c, a, b, borrowin) \
    do                                                       \
    {                                                        \
        uint32_t _t, _borrow1, _borrow2;                     \
        sub_borrowout(_borrow1, _t, a, b);                   \
        sub_borrowout(_borrow2, c, _t, borrowin);            \
        borrowout = _borrow1 + _borrow2;                     \
    } while (0)

#define sub_borrowout_1(borrow, c, a) \
    do                                \
    {                                 \
        uint32_t _b = 0;              \
        c = a - b;                    \
        borrow = _b;                  \
    } while (0)

#define sub_borrowin_borrowout_1(borrowout, c, a, borrowin) \
    do                                                      \
    {                                                       \
        uint32_t _t, _borrow1, _borrow2;                    \
        sub_borrowout_1(_borrow1, _t, a);                   \
        sub_borrowout(_borrow2, c, _t, borrowin);           \
        borrowout = _borrow1 + _borrow2;                    \
    } while (0)

/* 32 x 32 --> 64-bit multiplication
* (c1,c0) = a * b
*/
#define mul(c1, c0, a, b)                     \
    \
do                                     \
    {                                         \
        uint64_t __m;                         \
        __m = (uint64_t)a * (uint64_t)b;      \
        c0 = (uint32_t)__m;                   \
        c1 = (uint32_t)(__m >> (uint64_t)32); \
    \
}                                      \
    while (0)

/* Multiply-and-accumulate
* (c1,c0) = a*b+c0
 */
#define muladd(c1, c0, a, b)   \
    \
do                      \
    {                          \
        uint32_t __ma = c0;    \
        mul(c1, c0, a, b);     \
        c0 = c0 + __ma;        \
        c1 = c1 + (c0 < __ma); \
    \
}                       \
    while (0)

/* Multiply-and-accumulate-accumulate
* (c1,c0) = a*b+c0+c1
*/
#define muladdadd(c1, c0, a, b)            \
    \
do                                  \
    {                                      \
        uint32_t __maa0 = c0, __maa1 = c1; \
        mul(c1, c0, a, b);                 \
        c0 = c0 + __maa0;                  \
        c1 = c1 + (c0 < __maa0);           \
        c0 = c0 + __maa1;                  \
        c1 = c1 + (c0 < __maa1);           \
    \
}                                   \
    while (0)

#if CASPER_ECC_P256

/* Recoding length for the secure scalar multiplication:
*  Use n=256 and w=4 --> compute ciel(384/3) = 86 + 1 digits
*/
#define CASPER_RECODE_LENGTH 87
#define invert(c, a) invert_mod_p256(c, a)
#define ONE NISTr256

/* Shift right by 1 <= c <= 31. z[] and x[] in system RAM, no interleaving macros used. */
#define shiftrightSysram(z, x, c)                    \
    do                                               \
    {                                                \
        z[0] = (x[1] << (32 - (c))) | (x[0] >> (c)); \
        z[1] = (x[2] << (32 - (c))) | (x[1] >> (c)); \
        z[2] = (x[3] << (32 - (c))) | (x[2] >> (c)); \
        z[3] = (x[4] << (32 - (c))) | (x[3] >> (c)); \
        z[4] = (x[5] << (32 - (c))) | (x[4] >> (c)); \
        z[5] = (x[6] << (32 - (c))) | (x[5] >> (c)); \
        z[6] = (x[7] << (32 - (c))) | (x[6] >> (c)); \
        z[7] = (x[7] >> (c));                        \
    } while (0)

#elif CASPER_ECC_P384

/* Recoding length for the secure scalar multiplication:
 *  Use n=384 and w=4 --> compute ciel(384/3) = 128 + 1 digits
 */
#define CASPER_RECODE_LENGTH 129
#define invert(c, a) invert_mod_p384(c, a)
#define ONE NISTr384

/* Shift right by 1 <= c <= 31.  z[] and x[] in system RAM, no interleaving macros used. */
#define shiftrightSysram(z, x, c)                       \
    do                                                  \
    {                                                   \
        z[0] = (x[1] << (32 - (c))) | (x[0] >> (c));    \
        z[1] = (x[2] << (32 - (c))) | (x[1] >> (c));    \
        z[2] = (x[3] << (32 - (c))) | (x[2] >> (c));    \
        z[3] = (x[4] << (32 - (c))) | (x[3] >> (c));    \
        z[4] = (x[5] << (32 - (c))) | (x[4] >> (c));    \
        z[5] = (x[6] << (32 - (c))) | (x[5] >> (c));    \
        z[6] = (x[7] << (32 - (c))) | (x[6] >> (c));    \
        z[7] = (x[8] << (32 - (c))) | (x[7] >> (c));    \
        z[8] = (x[9] << (32 - (c))) | (x[8] >> (c));    \
        z[9] = (x[10] << (32 - (c))) | (x[9] >> (c));   \
        z[10] = (x[11] << (32 - (c))) | (x[10] >> (c)); \
        z[11] = (x[11] >> (c));                         \
    } while (0)

#else
#error "Define proper NIST curve"
#endif

#define multiply_casper(c, a, b) MultprecCiosMul_ct(c, a, b, &CASPER_MEM[MOD_SCRATCH_START], Np)
#define square_casper(c, a) multiply_casper(c, a, a)
#define sub_casper(c, a, b) CASPER_montsub(c, a, b, &CASPER_MEM[MOD_SCRATCH_START])
#define add_casper(c, a, b) CASPER_montadd(c, a, b, &CASPER_MEM[MOD_SCRATCH_START])
#define mul2_casper(c, a) add_casper(c, a, a)
#define half(c, a, b) CASPER_half(c, a, b)
#define copy(c, a) CASPER_MEMCPY(c, a, NUM_LIMBS * sizeof(uint32_t))

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*  The model for this algo is that it can be implemented for a fixed size RSA key */
/*  for max speed. If this is made into a variable (to allow varying size), then */
/*  it will be slower by a bit. */
/*  The file is compiled with N_bitlen passed in as number of bits of the RSA key */
/*  #define N_bitlen 2048 */
static size_t N_wordlen = 0; /* ! number of words (e.g. 4096/32 is 128 words) */

static uint32_t s_casperRamBase = CASPER_RAM_BASE_NS;
static unsigned *msg_ret = (unsigned *)CASPER_RAM_BASE_NS;

#if CASPER_ECC_P256
/* NISTp-256 = 2^256-2^224+2^192+2^96-1 */
static uint32_t NISTp256[NUM_LIMBS] = {0xffffffff, 0xffffffff, 0xffffffff, 0x00000000,
                                       0x00000000, 0x00000000, 0x00000001, 0xffffffff};

/* The cardinality of the curve E(F_p) */
static uint32_t NISTp256_q[NUM_LIMBS] = {0xfc632551, 0xf3b9cac2, 0xa7179e84, 0xbce6faad,
                                         0xffffffff, 0xffffffff, 0x00000000, 0xffffffff};

/* R = 2^256 mod p, the value "1" in Montgomery form. */
static uint32_t NISTr256[NUM_LIMBS] = {0x00000001, 0x00000000, 0x00000000, 0xffffffff,
                                       0xffffffff, 0xffffffff, 0xfffffffe, 0x00000000};

static uint32_t Np[2] = {1, 0};
#endif /* CASPER_ECC_P256 */

#if CASPER_ECC_P384
/* NISTp-384 =  2^384 - 2^128 - 2^96 + 2^32 - 1 */
static uint32_t NISTp384[NUM_LIMBS] = {0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0xfffffffe, 0xffffffff,
                                       0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

/* The cardinality of the curve E(F_p) */
static uint32_t NISTp384_q[NUM_LIMBS] = {0xccc52973, 0xecec196a, 0x48b0a77a, 0x581a0db2, 0xf4372ddf, 0xc7634d81,
                                         0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

/* R = 2^256 mod p, the value "1" in Montgomery form. */
static uint32_t NISTr384[NUM_LIMBS] = {0x00000001, 0xffffffff, 0xffffffff, 0x00000000, 0x1, 0, 0, 0, 0, 0, 0, 0};

// -p^-1 mod 2^64 = 0x100000001
static uint32_t Np[2] = {1, 1};
#endif /* CASPER_ECC_P384 */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* Convert a projective point (X1 : Y1 : Z1)
 * to the affine point (X3, Y3) = (X1/Z1^2,Y1/Z1^3)
 * The memory of (X3, Y3) and (X1 : Y1 : Z1) should not overlap
 */
void Jac_toAffine(uint32_t *X3, uint32_t *Y3, uint32_t *X1, uint32_t *Y1, uint32_t *Z1);

/* Return 1 if (X1: Y1: Z1) is on the curve
*      Y^2 = X^3 -3XZ^4 + bZ^6
* and return 0 otherwise.
*/
int Jac_oncurve(uint32_t *X1, uint32_t *Y1, uint32_t *Z1, uint32_t *b);

/* Compute (X3 : Y3: Z3) = (X1: Y1: Z1) + (X2 : Y2 : Z2)
 *  where (X1: Y1: Z1) != (X2 : Y2 : Z2)
 * (X3 : Y3: Z3) may be the same as one of the inputs.
 */
void Jac_addition(uint32_t *X3,
                  uint32_t *Y3,
                  uint32_t *Z3,
                  uint32_t *X1,
                  uint32_t *Y1,
                  uint32_t *Z1,
                  uint32_t *X2,
                  uint32_t *Y2,
                  uint32_t *Z2);

/* Compute (X3 : Y3: Z3) = (X1: Y1: Z1) + (X2, Y2)
 * where (X1: Y1: Z1) != (X2, Y2)
 * (X3 : Y3: Z3) may not overlap with (X1: Y1: Z1).
 * Source: 2004 Hankerson–Menezes–Vanstone, page 91.
 */
void Jac_add_affine(
    uint32_t *X3, uint32_t *Y3, uint32_t *Z3, uint32_t *X1, uint32_t *Y1, uint32_t *Z1, uint32_t *X2, uint32_t *Y2);

/* Point doubling from: 2004 Hankerson–Menezes–Vanstone, page 91.
 * Compute (X3 : Y3: Z3) = (X1: Y1: Z1) + (X1 : Y1 : Z1)
 * (X3 : Y3: Z3) may be the same as the input.
 */
void Jac_double(uint32_t *X3, uint32_t *Y3, uint32_t *Z3, uint32_t *X1, uint32_t *Y1, uint32_t *Z1);

/* Constant time elliptic curve scalar multiplication.
 * Source: https://eprint.iacr.org/2014/130.pdf
 * when using w = 4.
 * Computes (X3 : Y3 : Z3) = k * (X1, Y1) \in E(F_p)
 * p is the prime used to define the finite field F_p
 * q is the (prime) order of the curve
 */
void Jac_scalar_multiplication(
    uint32_t *X3, uint32_t *Y3, uint32_t *Z3, uint32_t *X1, uint32_t *Y1, uint32_t *k, uint32_t *p, uint32_t *q);

/* Compute the double scalar multiplication
* (X3 : Y3 : Z3) = k1 * (X1, Y1) + k2 * (X2, Y2)
* Using Shamir's trick and precomputing 16 points.
* This code is *not* constant time since this is used
* for verification only.
*/
void double_scalar_multiplication(uint32_t *X3,
                                  uint32_t *Y3,
                                  uint32_t *Z3,
                                  uint32_t *X1,
                                  uint32_t *Y1,
                                  uint32_t *k1,
                                  uint32_t *X2,
                                  uint32_t *Y2,
                                  uint32_t *k2);

#if CASPER_ECC_P384
/* Compute inversion modulo NIST-p384 using Fermats little theorem.
 * Using c = a^(p-2) = a^(-1) mod p.
 * This computes the modular inversion if all arithmetic is "regular"
 * modular arithmetic or computes automatically the Montgomery inverse
 * if all arithmetic is Montgomery arithmetic.
 */
static void invert_mod_p384(uint32_t *c, uint32_t *a);
#endif /* CASPER_ECC_P384 */

#if CASPER_ECC_P256
/* Modular inversion for NIST-P256 */
static void invert_mod_p256(uint32_t *c, uint32_t *a);
#endif /* CASPER_ECC_P256 */

// A and C do not need to be in Casper memory
static void toMontgomery(uint32_t *C, uint32_t *A);

static void CASPER_montsub(uint32_t *C, uint32_t *A, uint32_t *B, uint32_t *mod);
static void CASPER_montadd(uint32_t *C, uint32_t *A, uint32_t *B, uint32_t *mod);

/* Compute c = a/2 mod p where b is scratch space. */
static void CASPER_half(uint32_t *c, uint32_t *a, uint32_t *b);

static uint8_t int8abs(int8_t v);

/* Constant time select c = a if m = 0 or
*                      c = b if m = 1
* a, b, c are n words
*/
static void casper_select(uint32_t *c, uint32_t *a, uint32_t *b, int m, int n);

/* Dumb n-limb addition of c=a+b, return carry. */
static uint32_t add_n_1(uint32_t *c, uint32_t *a, uint32_t b, int n);

#if 0
/* Dumb n-limb addition of c=a+b, return carry. */
static uint32_t add_n(uint32_t *c, uint32_t *a, uint32_t *b, int n);

/* Dumb n-limb subtraction of c=a-b, return borrow. */
static uint32_t sub_n_1(uint32_t *c, uint32_t *a, uint32_t b, int n);
#endif

/* Dumb n-limb subtraction of c=a-b, return borrow. */
static uint32_t sub_n(uint32_t *c, uint32_t *a, uint32_t *b, int n);

static void MultprecCiosMul_ct(
    uint32_t w_out[], const uint32_t a[], const uint32_t b[], const uint32_t Nmod[], const uint32_t *Np);

static void shiftright(uint32_t *z, uint32_t *x, int c);
static void shiftleft(uint32_t *z, uint32_t *x, int c);

/*******************************************************************************
 * Code
 ******************************************************************************/

__STATIC_FORCEINLINE uint32_t CA_MK_OFF(const void *addr)
{
    return ((uint32_t)addr - s_casperRamBase);
}

#if 1
__STATIC_FORCEINLINE void Accel_done(void)
{
    register uint32_t status;
    do
    {
        status = CASPER->STATUS;
    } while (0 == (status & CASPER_STATUS_DONE_MASK));
}

__STATIC_FORCEINLINE void Accel_SetABCD_Addr(uint32_t ab, uint32_t cd)
{
    CASPER->CTRL0 = ab | (cd << 16); /* CDoffset << 16 | ABoffset */
}

__STATIC_FORCEINLINE void Accel_crypto_mul(uint32_t ctrl1)
{
    CASPER->CTRL1 = ctrl1;
}
#else
#include "intrinsics.h"
#define Accel_done()                                       \
    {                                                      \
        register uint32_t status;                          \
        do                                                 \
        {                                                  \
            status = CASPER_Rd32b(CASPER_CP_STATUS);       \
        } while (0 == (status & CASPER_STATUS_DONE_MASK)); \
    }
#if 0
__STATIC_FORCEINLINE void Accel_done(void)
{
    register uint32_t status;
    do
    {
        status = CASPER->STATUS;
    } while (0 == (status & CASPER_STATUS_DONE_MASK));
}
#endif
#define Accel_SetABCD_Addr(ab, cd) CASPER_Wr32b((uint32_t)ab | ((uint32_t)cd << 16), CASPER_CP_CTRL0);
#define Accel_crypto_mul(ctrl1) CASPER_Wr32b((uint32_t)ctrl1, CASPER_CP_CTRL1);
#endif

__STATIC_FORCEINLINE uint32_t Accel_IterOpcodeResaddr(uint32_t iter, uint32_t opcode, uint32_t resAddr)
{
    return CASPER_CTRL1_ITER(iter) | CASPER_CTRL1_MODE(opcode) | (resAddr << 16);
}

void CASPER_MEMCPY(void *dst, const void *src, size_t siz)
{
    bool bdst = ((((uint32_t)dst) | 0x10000000u) >= (FSL_FEATURE_CASPER_RAM_BASE_ADDRESS | 0x10000000u) &&
                 (((uint32_t)dst) | 0x10000000u) < (FSL_FEATURE_CASPER_RAM_BASE_ADDRESS | 0x10000000u) + 8u * 1024u);

    bool bsrc = ((((uint32_t)src) | 0x10000000u) >= (FSL_FEATURE_CASPER_RAM_BASE_ADDRESS | 0x10000000u) &&
                 (((uint32_t)src) | 0x10000000u) < (FSL_FEATURE_CASPER_RAM_BASE_ADDRESS | 0x10000000u) + 8u * 1024u);

    if (bdst && bsrc)
    {
        CASPER_MEMCPY_I2I(dst, src, siz);
    }
    else if (bdst && !bsrc)
    {
        CASPER_MEMCPY_N2I(dst, src, siz);
    }
    else if (!bdst && bsrc)
    {
        CASPER_MEMCPY_I2N(dst, src, siz);
    }
    else
    {
        memcpy(dst, src, siz);
    }
}

/* Constant time select c = a if m = 0 or
 *                      c = b if m = 1
 * a, b, c are n words
 */
static void casper_select(uint32_t *c, uint32_t *a, uint32_t *b, int m, int n)
{
    uint32_t m1 = 0 - m, m2 = ~m1;
    int i;

    for (i = 0; i < n; i++)
    {
        SET_WORD(&c[i], (GET_WORD(&a[i]) & m2) | (GET_WORD(&b[i]) & m1));
    }
}

/*  Compute R`, which is R mod N. This is done using subtraction */
/*  R has 1 in N_wordlen, but we do not fill it in since borrowed. */
/*  Exp-pubkey only used to optimize for exp=3 */
void MultprecMontCalcRp(unsigned Rp[], const unsigned exp_pubkey, const unsigned Nmod[])
{
    int i;

    /*  R is 2^n where n is 1 bit longer than Nmod, so 1 followed by 32 or 64 0 words for example */
    /*  Note that Nmod's upper most bit has to be 1 by definition, so one subtract is enough. We */
    /*  do not set the 1 since it is "borrowed" so no point */
    PreZeroW(i, Rp);
    Accel_SetABCD_Addr(CA_MK_OFF(Nmod), 0);
    Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpSub64, CA_MK_OFF(Rp)));
    Accel_done();
    /*  final borrow cannot happen since we know we started with a larger number */
}

/*  MultprecMultiply - multiple w=u*v (per Knuth) */
/*  w_out is 2x the size of u and v */
void MultprecMultiply(unsigned w_out[], const unsigned u[], const unsigned v[])
{
    int i, j;

    /*  Knuth 4.3.1 - Algorithm M */
    /*    Compute w = u * v */
    /*  u and v are N bits long in 32 bit word form */
    /*  w is 2*N bits long in 32 bit word form */
    /*  Note: We just multiply in place */

    /*  Step 1. Fill w[t-1:0] with 0s, the upper half will be written as we go */
    PreZeroW(i, w_out);

    /*  We do 1st pass NOSUM so we do not have to 0 output */
    Accel_SetABCD_Addr(CA_MK_OFF(&v[0]), CA_MK_OFF(u));
    Accel_crypto_mul(Accel_IterOpcodeResaddr(N_wordlen / 2 - 1, kCASPER_OpMul6464NoSum, CA_MK_OFF(&w_out[0])));
    Accel_done();
    /*  Step 2. iterate over N words of v using j */
    for (j = 2; j < N_wordlen; j += 2)
    {
        /*  Step 2b. Check for 0 on v word - skip if so since we 0ed already */
        /*  Step 3. Iterate over N words of u using i - perform Multiply-accumulate */
        if (GET_WORD(&v[j]) || GET_WORD(&v[j + 1]))
        {
            Accel_SetABCD_Addr(CA_MK_OFF(&v[j]), CA_MK_OFF(u));
            Accel_crypto_mul(Accel_IterOpcodeResaddr(N_wordlen / 2 - 1, kCASPER_OpMul6464Sum, CA_MK_OFF(&w_out[j])));
            Accel_done();
        }
    }
}

/*  MultprecModulo performs divide to get remainer as needed for RSA */
/*  This performs (q,r) = u/v, but we do not keep q */
/*  r_out is module (remainder) and is 2*N */
/*  u is in r_out (1st N) at start (passed in) */
/*  v is N long */
void MultprecModulo(unsigned r_out[], const unsigned v[], int top)
{
    uint64_t u64;                      /*  use 64 bit math mixed with 32 bit */
    unsigned u32;                      /*  allows us to work on U in 32 bit */
    unsigned u_n, ul16, uh16, *u_shft; /*  u_shft is because r_out is u initially */
    unsigned vl16, vh16, v_Nm1;
    unsigned q_hat, r_hat, q_over;
    unsigned borrow, carry;
    int i, j, tmp;

    /*  Knuth 4.3.1 - Algorithm D */
    /*    Compute q = u / v giving remainder r = u mod v */
    /*    -- we only want r, so we build qhat but do not store the Qs */
    /*  v is N long, with u,q,r 2N long because u is slowly replavced by r. */
    /*  We normalize/unnormlize per Knuth in the buffer (not copied) */

    /*  Step 1. Normalize value so MSb is in v[n-1]. Remember that v is */
    /*  the public key - to call it a 2048 bit number, they cannot have 0 */
    /*  in the MSb (or it would be less than 2048 bits) and so we know we */
    /*  are normalized already. Therefore, u is effectively shifted already. */
    /*  For u, we have it in r_out. u[n] holds any overflow */
    /*  Since divide on CM3/4 is 32/32=32, we break into 16 bit halves, but */
    /*  multiply can be 32x32=64. */
    u_n = 0;
    u_shft = r_out; /*  u (shifted) is in r_out */

    v_Nm1 = GET_WORD(&v[N_wordlen - 1]); /*  MSw of public key */
    vl16 = v_Nm1 & 0xFFFF;               /*  lower 16 */
    vh16 = v_Nm1 >> 16;                  /*  upper 16 */
    /*  Step 2. Iterate j from m-n down to 0 (M selected per Knuth as 2*N) */
    for (j = top; j >= 0; j--)
    {
        /*  Step 3. estimate q_hat as (U[j+n]*B + U[j+n-1]) / V[n-1] */
        /*  Note: using subset of Knuth algo since v is 1/2 len of u (which is */
        /*  from multiply or x^2 leading into this). */
        u32 = u_n; /*  pickup u4u3u2, knowing u4 is 0 */
        u64 = ((uint64_t)u_n << 32) | GET_WORD(&u_shft[j + N_wordlen - 1]);
        ul16 = u64 & 0xFFFF;         /*  lower 16 */
        uh16 = (u64 >> 16) & 0xFFFF; /*  upper 16 */

        /*  we see if even possible (u large enough relative to v) */
        if ((u32 - v_Nm1) <= u32)
        {
            u32 -= v_Nm1;
            q_over = 1; /*  overflow from the sub */
        }
        else
            q_over = 0;

        /*  q_hat = u32 / vh16 -- is the upper partial value */
        /*  estimate; if too much, then back down by 1 or 2 */
        q_hat = u32 / vh16;
        r_hat = u32 - (q_hat * vh16);
        /*  see if Q is more than 16 bits or remainder is too large  (over div) */
        if ((q_hat == 0x10000) || ((q_hat * vl16) > ((r_hat << 16) | uh16)))
        {
            /*  too much - undo a division */
            q_hat--;
            r_hat += vh16;
            /*  check if still too much */
            if ((r_hat < 0x10000) && ((q_hat * vl16) > ((r_hat << 16) | uh16)))
                q_hat--; /*  yes, so undo a 2nd */
        }

        /*  compose u3u2uh16, then sub q_hat*v if OK */
        u64 = (((uint64_t)u32 << 16) | uh16) - ((uint64_t)q_hat * v_Nm1);
        if (u64 >> 48)
        {
            /*  no, so add v back */
            u32 = (unsigned)(u64 + v_Nm1);
            q_hat--;
        }
        else
            u32 = (unsigned)u64;

        tmp = q_hat << 16; /*  quotient upper part */

        /*  divide lower part: q = u2uh16ul16 / v. */
        /*  estimate and add back if over divdied */
        q_hat = u32 / vh16;
        r_hat = u32 - (q_hat * vh16);
        if ((q_hat == 0x10000) || ((q_hat * vl16) > ((r_hat << 16) | ul16)))
        {
            /*  too much - undo a division */
            q_hat--;
            r_hat += vh16;
            /*  check if still too much */
            if ((r_hat < 0x10000) && ((q_hat * vl16) > ((r_hat << 16) | ul16)))
                q_hat--; /*  yes, so undo a 2nd */
        }

        /*  compose u2uh16ul16, then sub q_hat*v if OK */
        u64 = (((uint64_t)u32 << 16) | ul16) - ((uint64_t)q_hat * v_Nm1);
        if (u64 >> 48)
        {
            /*  no, so add v back */
            r_hat = (unsigned)(u64 + v_Nm1);
            q_hat--;
        }
        else
            r_hat = (unsigned)u64;

        q_hat |= tmp; /*  other half of the quotient */
        while (q_over ||
               ((uint64_t)q_hat * GET_WORD(&v[N_wordlen - 2])) >
                   ((1LL << 32) * r_hat) + (uint64_t)GET_WORD(&u_shft[j + N_wordlen - 2]))
        { /*  if Qhat>b, then reduce to b-1, then adjust up Rhat */
            q_hat--;
            r_hat += v_Nm1;
            if (r_hat < v_Nm1)
                break; /*  no overflow */
                       /*  else repeat since Rhat >= b */
        }

        /*  Step 4. Multiply and subtract. We know the amount, */
        /*          so we do the schoolboy math. Have to do on */
        /*          the large value. */
        if (q_hat)
        {
            borrow = 0;
            for (i = 0; i < N_wordlen; i++)
            {
                u64 = (uint64_t)q_hat * GET_WORD(&v[i]) + borrow;
                borrow = (unsigned)(u64 >> 32);
                if (GET_WORD(&u_shft[i + j]) < (unsigned)u64)
                    borrow++; /*  carry the overflow */
                SET_WORD(&u_shft[i + j], GET_WORD(&u_shft[i + j]) - (unsigned)u64);
            }
            u_n -= borrow; /*  overflow from shift left does not fit otherwise */
        }

        /*  Store 5. (update Q - we don't), and add back V to remainder if we over-subtracted */
        /*           That restores remainder to correct (we could only be off by 1) */
        /*           This should happen very rarely. */
        if (u_n)
        {
            carry = 0;
            for (i = 0; i < N_wordlen; i++)
            {
                SET_WORD(&u_shft[i + j], GET_WORD(&u_shft[i + j]) + carry);
                carry = (GET_WORD(&u_shft[i + j]) < carry) ? 1 : 0;
                SET_WORD(&u_shft[i + j], GET_WORD(&u_shft[i + j]) + GET_WORD(&v[i]));
                if (GET_WORD(&u_shft[i + j]) < GET_WORD(&v[i]))
                    carry++;
            }
        }
        u_n = GET_WORD(&u_shft[j + N_wordlen - 1]); /*  hold upper part of u to catch overflow (to borrow from) */
    }
    /*  low N bits of r are valid as remainder */
}

/*  We convert X into a Mont form number. Note length of arrays: */
/*  x is N_wordlen, Nmod is N_wordlen */
/*  Rp is N_wordlen (it is R` which is R mod N) */
/*  Xmont_out is N_wordlen*2+1 */
void MultprecMontPrepareX(unsigned Xmont_out[], const unsigned x[], const unsigned Rp[], const unsigned Nmod[])
{
    MultprecMultiply(Xmont_out, x, Rp);
    MultprecModulo(Xmont_out, Nmod, N_wordlen);
}

void MultprecGenNp64(const unsigned *Nmod, unsigned *np64_ret) /*  only pass the low order double word */
{
    uint64_t nprime, Nmod_0;
    Nmod_0 = GET_WORD(&Nmod[0]) | ((uint64_t)GET_WORD(&Nmod[1]) << 32);

#define COMP_NPN_1 ((2 - Nmod_0 * nprime) * nprime) /*  computes N`*N0=1 mod 2^P where P is the partial built up */
    nprime = (((2 + Nmod_0) & 4) << 1) + Nmod_0;    /*  mod 2^4 */
    nprime = COMP_NPN_1;
    nprime = COMP_NPN_1;
    nprime = COMP_NPN_1;
    nprime = COMP_NPN_1;
    /*  8 multiplies of uint64_t */
    *((uint64_t *)np64_ret) = (~0LL - nprime) + 1LL;
}

/*  CIOS Multiply. This is the Coarse Integrated form where the values are */
/*  multiplied and reduced for each step of "i". This uses less memory and */
/*  is faster as a result. Note that this is used to square as well as mul, */
/*  so not as fast as pure squaring could be. */
void MultprecCiosMul(
    unsigned w_out[], const unsigned a[], const unsigned b[], const unsigned Nmod[], const unsigned *Np)
{
    int i, j;
    uint64_t *m64 = (uint64_t *)&msg_ret[kCASPER_RamOffset_M64];
    uint64_t Np64;
    uint64_t carry;
    uint64_t *a64, *b64, *w64, *N64;

    Np64 = *(uint64_t *)Np;

    a64 = (uint64_t *)a;
    b64 = (uint64_t *)b;
    w64 = (uint64_t *)w_out;
    N64 = (uint64_t *)Nmod;

    if (a)
    { /*  if !a, we are reducing only */
        PreZeroW(i, w_out);
    }
    SET_DWORD(&w64[N_dwordlen], 0);
    SET_DWORD(&w64[N_dwordlen + 1], 0);
    /*  with accelerator */

    /*  loop i and then reduce after each j round */
    for (i = 0; i < N_dwordlen; i++)
    {
        /*  Step 3. Iterate over N words of u using i - perform Multiply-accumulate */
        /*  push-pull: we do a*b and then separately m*n (reduce) */
        if (a)
        { /*  if mul&reduce vs. reduce only */
            carry = GET_DWORD(&w64[N_dwordlen]);
            Accel_SetABCD_Addr(CA_MK_OFF(&b64[i]), CA_MK_OFF(a64));
            Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpMul6464FullSum, CA_MK_OFF(w64)));
            Accel_done();
            /*  max carry is contained since ~0*~0=0xFFFE0001+0xFFFF=0xFFFF0000, */
            /*  so max carry is 0xFFFF and 0xFFFF0000+0xFFFF=0xFFFFFFFF */
            /*  accel took care of w_out[N_wordlen] & +1, so we just take care of the next double word if carry=1 */
            /*  w64[N_dwordlen+1] = g_carry; */
            carry = (GET_DWORD(&w64[N_dwordlen]) < carry);
            SET_DWORD(&w64[N_dwordlen + 1], carry);
        }
        SET_DWORD(&m64[0], GET_DWORD(&w64[0]) * Np64); /*  prime for 1st; modulo a double-word */

        /*  we are reducing, so the 1st [0th] 64 bit value product is tossed, but we */
        /*  need its carry. We let the accel do this separately - really need a mode to */
        /*  do this "reduce" since it is natural */
        carry = GET_DWORD(&w64[N_dwordlen]);
        Accel_SetABCD_Addr(CA_MK_OFF(m64), CA_MK_OFF(&N64[0]));
        Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpMul6464FullSum, CA_MK_OFF(&w64[0])));
        Accel_done();
        carry = (GET_DWORD(&w64[N_dwordlen]) < carry);

        Accel_SetABCD_Addr(CA_MK_OFF(&w64[1]), 0);
        Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpCopy, CA_MK_OFF(&w64[0])));

        Accel_done();
        SET_DWORD(&w64[N_dwordlen], (GET_DWORD(&w64[N_dwordlen + 1]) + carry));
    }

    /*  now check if need to subtract Nmod */
    if (GET_WORD(&w_out[N_wordlen]))
        j = 1; /*  we have to subtract for sure if carry up */
    else
    {
        j = 0;
        for (i = N_wordlen - 1; i >= 0; i--)
            if (GET_WORD(&w_out[i]) != GET_WORD(&Nmod[i]))
            {
                j = GET_WORD(&w_out[i]) > GET_WORD(&Nmod[i]); /*  if larger sub */
                break; /*  we would remove the break if worrying about side channel */
            }
    }
    if (!j)
        return; /*  Is smaller than Nmod, so done. */
    Accel_SetABCD_Addr(CA_MK_OFF(Nmod), 0);
    Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpSub64, CA_MK_OFF(w_out)));
    Accel_done();
    /*  last borrow is OK since we know it could only be <2N and */
}

/*  RSA_MontSignatureToPlaintextFast: */
/*  MsgRet[] = Message return buffer - must be large enough to hold input and output (4*N+2) */
/*  exp_pubkey = the "e" that the value is raised to. Usually 3 or 0x10001. */
/*  signature = N bitpos len long "message" to process in Montgomery form - so saving conversion (divide) */
/*  pubkey = N bitpos len long public key to process signature with */
/*  returns: 0 */
/*  */
/*  Algo: compute M = signaturen^e mod public_key */
/*        where M is original plaintext, signature is signed value */
/*        note: e is usually either 0x3 or 0x10001 */
int RSA_MontSignatureToPlaintextFast(const unsigned mont_signature[N_wordlen_max],
                                     const unsigned exp_pubkey,
                                     const unsigned pubkey[N_wordlen_max],
                                     unsigned MsgRet[WORK_BUFF_MUL4])
{
    int bidx = 0;
    int bitpos;
    unsigned np64[2];

    /*  MsgRet working area: */
    /*  0..N = RESULT, starting with S` */
    /*  N..N*2 = S` and then working BASE during math. */
    /*  N*2..N*4+2 = temp working area for Mont mul */

    /*  1. Copy sig into MsgRet so we have one working result buffer */
    CASPER_MEMCPY_I2I(&MsgRet[kCASPER_RamOffset_Result], mont_signature, N_bytelen);
    MultprecGenNp64(pubkey, np64);   /*  Generate N` from LSW of N (LSW being lowest 64b word) */
    bitpos = 31 - __CLZ(exp_pubkey); /*  count of bits after the left most 1 */
    while (--bitpos >= 0)
    {
        /*  This operates on: */
        /*    result = 1; */
        /*    base = signature */
        /*    loop while exponent bits from MSb to LSb */
        /*      if (exp bit is 1) */
        /*        result = result * base */
        /*      base = base^2 */
        /*  Because the MSb of exp is always 1 by definition, we can invert this a bit: */
        /*    base = signature` */
        /*    result = base; equivalent to result = 1*base from 1st pass, but now square is needed 1st */
        /*    loop while exponent bits from MSb-1 to LSb */
        /*      base = base^2 */
        /*      if (exp bit is 1) */
        /*        result = result * base */
        /*  This ends up doing the same thing but skips two wasteful steps of multiplying by 1 and */
        /*  a final squaring never used. */
        /*  */
        /*  Next we have the problem that CIOS mul needs a separate dest buffer. So, we bounce */
        /*  base between base and temp, and likewise for result. */
        MultprecCiosMul(&MsgRet[bidx ? kCASPER_RamOffset_Base : kCASPER_RamOffset_TempBase],
                        &MsgRet[bidx ? kCASPER_RamOffset_TempBase : kCASPER_RamOffset_Base],
                        &MsgRet[bidx ? kCASPER_RamOffset_TempBase : kCASPER_RamOffset_Base], pubkey, np64);
        if (exp_pubkey & (1 << bitpos)) /*  where e is 1 */
        {
            /*  result has result, so we need to work into other temp area */
            MultprecCiosMul(&MsgRet[bidx ? kCASPER_RamOffset_TempBase : kCASPER_RamOffset_Base],
                            &MsgRet[kCASPER_RamOffset_Result],
                            &MsgRet[bidx ? kCASPER_RamOffset_Base : kCASPER_RamOffset_TempBase], pubkey, np64);
            /*  we have to copy back to result */

            // CASPER_MEMCPY_I2I(&MsgRet[kCASPER_RamOffset_Result],
            //      &MsgRet[bidx ? kCASPER_RamOffset_TempBase : kCASPER_RamOffset_Base], N_bytelen);
        }
        else
            bidx = ~bidx;
    }

    CASPER_MEMCPY_I2I(&MsgRet[kCASPER_RamOffset_Result],
                      &MsgRet[bidx ? kCASPER_RamOffset_TempBase : kCASPER_RamOffset_Base], N_bytelen);

    /*  final step is one more reduction to get back to normal form (ie. divide R out) */
    MultprecCiosMul(&MsgRet[kCASPER_RamOffset_Result], NULL, NULL, pubkey, np64);
    return (0); /*  always 0 */
}

/*  RSA_SignatureToPlaintextFast: */
/*  MsgRet[] = Message return buffer - must be large enough to hold input and output (4*N+2) */
/*  exp_pubkey = the "e" that the value is raised to. Usually 3 or 0x10001. */
/*  signature = N bitpos len long "message" to process in normal form - so converted to Mont form */
/*  pubkey = N bitpos len long public key to process signature with */
/*  returns: 0 */
/*  */
/*  Algo: compute M = signaturen^e mod public_key */
/*        where M is original plaintext, signature is signed value */
/*        note: e is usually either 0x3 or 0x10001 */
int RSA_SignatureToPlaintextFast(const unsigned signature[N_wordlen_max],
                                 const unsigned exp_pubkey,
                                 const unsigned pubkey[N_wordlen_max],
                                 unsigned MsgRet[WORK_BUFF_MUL4])
{
    /*  MsgRet working area: */
    /*  0..N = RESULT, starting with S`; it is used for R` just during creation of S` */
    /*  N..N*2 = S` and then working BASE during math. Note overflow beyond N*2 when making S` */
    /*  N*2..N*4+2 = temp working area for Mont mul */

    MultprecMontCalcRp(&MsgRet[kCASPER_RamOffset_Result], exp_pubkey, pubkey); /*  calculate R` (=R mod N) */
    MultprecMontPrepareX(&MsgRet[kCASPER_RamOffset_Base], signature, &MsgRet[kCASPER_RamOffset_Result],
                         pubkey); /*  X*R1` mod N */
    return (RSA_MontSignatureToPlaintextFast(&MsgRet[kCASPER_RamOffset_Base], exp_pubkey, pubkey, MsgRet));
}

/*!
 * brief Performs modular exponentiation - (A^E) mod N.
 *
 * This function performs modular exponentiation.
 *
 * param base CASPER base address
 * param signature first addend (in little endian format)
 * param pubN modulus (in little endian format)
 * param wordLen Size of pubN in bytes
 * param pubE exponent
 * param[out] plaintext Output array to store result of operation (in little endian format)
 */
void CASPER_ModExp(
    CASPER_Type *base, const uint8_t *signature, const uint8_t *pubN, size_t wordLen, uint32_t pubE, uint8_t *plaintext)
{
#define PK_LOC &msg_ret[kCASPER_RamOffset_Modulus]
#define SIG_LOC &msg_ret[kCASPER_RamOffset_Modulus + N_wordlen_max]

    N_wordlen = wordLen; /* set global variable for key length - used by RSA_SignatureToPlaintextFast()  */
    CASPER_MEMCPY_N2I(PK_LOC, pubN, N_bytelen);
    CASPER_MEMCPY_N2I(SIG_LOC, signature, N_bytelen);
    RSA_SignatureToPlaintextFast((const unsigned *)(SIG_LOC), pubE, (const unsigned *)(PK_LOC), msg_ret);

    CASPER_MEMCPY_I2N(plaintext, msg_ret, N_bytelen);
}

/*!
 * brief Enables clock and disables reset for CASPER peripheral.
 *
 * Enable clock and disable reset for CASPER.
 *
 * param base CASPER base address
 */
void CASPER_Init(CASPER_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Casper);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    RESET_PeripheralReset(kCASPER_RST_SHIFT_RSTn);
    /* If Casper init is called with secure address, use secure addres also for accessing Casper RAM. */
    s_casperRamBase = CASPER_RAM_BASE_NS | ((uint32_t)base & 0x10000000u);
    msg_ret = (unsigned *)s_casperRamBase;
}

/*!
 * brief Disables clock for CASPER peripheral.
 *
 * Disable clock and enable reset.
 *
 * param base CASPER base address
 */
void CASPER_Deinit(CASPER_Type *base)
{
    RESET_SetPeripheralReset(kCASPER_RST_SHIFT_RSTn);
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_Casper);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/* New ECC code which uses Casper. */

/* Set the prime modulus mod in Casper memory.
 */
void CASPER_ecc_init(void)
{
#if CASPER_ECC_P256
    N_wordlen = 256 / 32;
    uint32_t *mod = NISTp256;
#elif CASPER_ECC_P384
    N_wordlen = 384 / 32;
    uint32_t *mod = NISTp384;
#endif
    CASPER_MEMCPY(&CASPER_MEM[MOD_SCRATCH_START], mod, NUM_LIMBS * sizeof(uint32_t));
    uint8_t a[(CASPER_NUM_LIMBS - NUM_LIMBS) * sizeof(uint32_t)] = {0};
    CASPER_MEMCPY(&CASPER_MEM[MOD_SCRATCH_START + NUM_LIMBS], a, (CASPER_NUM_LIMBS - NUM_LIMBS) * sizeof(uint32_t));
}

void CASPER_ECC_equal(int *res, uint32_t *op1, uint32_t *op2)
{
    uint32_t a[NUM_LIMBS];
    uint32_t b[NUM_LIMBS];
    int c = 0;
    CASPER_MEMCPY(a, op1, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(b, op2, NUM_LIMBS * sizeof(uint32_t));

    do
    {
        int _i;
        c = a[0] ^ b[0];
        for (_i = 1; _i < NUM_LIMBS; _i++)
        {
            c |= (a[_i] ^ b[_i]);
        }
    } while (0);

    *res = c;
}

void CASPER_ECC_equal_to_zero(int *res, uint32_t *op1)
{
    uint32_t a[NUM_LIMBS];
    int c = 0;
    CASPER_MEMCPY(a, op1, NUM_LIMBS * sizeof(uint32_t));

    do
    {
        int _i;
        c = a[0];
        for (_i = 1; _i < NUM_LIMBS; _i++)
        {
            c |= a[_i];
        }
    } while (0);

    *res = c;
}

#if CASPER_ECC_P256
void CASPER_ECC_SECP256R1_Mul(
    CASPER_Type *base, uint32_t resX[8], uint32_t resY[8], uint32_t X[8], uint32_t Y[8], uint32_t scalar[8])
{
    uint32_t X1[8] = {0};
    uint32_t Y1[8] = {0};
    toMontgomery(X1, X);
    toMontgomery(Y1, Y);

    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], X1, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], Y1, NUM_LIMBS * sizeof(uint32_t));

    Jac_scalar_multiplication(&CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 7 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 8 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], scalar, NISTp256, NISTp256_q);

    Jac_toAffine(&CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 7 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 8 * CASPER_NUM_LIMBS]);

    /* Montgomery to Normal */
    /* X_normal = 1 * X_montgomery; Y_normal = 1 * Y_montgomery */
    uint32_t one[CASPER_NUM_LIMBS] = {0x0};
    one[0] = 0x1u;
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], one, CASPER_NUM_LIMBS * sizeof(uint32_t));
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);

    /* copy out to result */
    CASPER_MEMCPY(resX, &CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS], NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(resY, &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS], NUM_LIMBS * sizeof(uint32_t));
}

void CASPER_ECC_SECP256R1_MulAdd(CASPER_Type *base,
                                 uint32_t resX[8],
                                 uint32_t resY[8],
                                 uint32_t X1[8],
                                 uint32_t Y1[8],
                                 uint32_t scalar1[8],
                                 uint32_t X2[8],
                                 uint32_t Y2[8],
                                 uint32_t scalar2[8])
{
    uint32_t zeroes[CASPER_NUM_LIMBS] = {0};

    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], X1, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], Y1, NUM_LIMBS * sizeof(uint32_t));

    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS], X2, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS], Y2, NUM_LIMBS * sizeof(uint32_t));

    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);
    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS]);
    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS]);
    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS]);

    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS], zeroes, CASPER_NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS], zeroes, CASPER_NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS], zeroes, CASPER_NUM_LIMBS * sizeof(uint32_t));
    double_scalar_multiplication(&CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], scalar1,
                                 &CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS], scalar2);

    Jac_toAffine(&CASPER_MEM[LUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[LUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS]);

    uint32_t one[CASPER_NUM_LIMBS] = {0x0};
    one[0] = 0x1u;
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], one, CASPER_NUM_LIMBS * sizeof(uint32_t));
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[LUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[LUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);

    CASPER_MEMCPY(resX, (&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS]), NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(resY, (&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS]), NUM_LIMBS * sizeof(uint32_t));
}
#endif /* CASPER_ECC_P256 */

#if CASPER_ECC_P384
void CASPER_ECC_SECP384R1_Mul(
    CASPER_Type *base, uint32_t resX[12], uint32_t resY[12], uint32_t X[12], uint32_t Y[12], uint32_t scalar[12])
{
    uint32_t X1[12] = {0};
    uint32_t Y1[12] = {0};
    toMontgomery(X1, X);
    toMontgomery(Y1, Y);

    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], X1, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], Y1, NUM_LIMBS * sizeof(uint32_t));

    Jac_scalar_multiplication(&CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 7 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 8 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                              &CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], scalar, NISTp384, NISTp384_q);

    Jac_toAffine(&CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 7 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 8 * CASPER_NUM_LIMBS]);

    /* Montgomery to Normal */
    /* X_normal = 1 * X_montgomery; Y_normal = 1 * Y_montgomery */
    uint32_t one[12] = {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], one, NUM_LIMBS * sizeof(uint32_t));
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);

    /* copy out to result */
    CASPER_MEMCPY(resX, &CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS], NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(resY, &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS], NUM_LIMBS * sizeof(uint32_t));
}

void CASPER_ECC_SECP384R1_MulAdd(CASPER_Type *base,
                                 uint32_t resX[12],
                                 uint32_t resY[12],
                                 uint32_t X1[12],
                                 uint32_t Y1[12],
                                 uint32_t scalar1[12],
                                 uint32_t X2[12],
                                 uint32_t Y2[12],
                                 uint32_t scalar2[12])
{
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], X1, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], Y1, NUM_LIMBS * sizeof(uint32_t));

    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS], X2, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS], Y2, NUM_LIMBS * sizeof(uint32_t));

    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);
    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS]);
    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS]);
    toMontgomery(&CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS]);

    double_scalar_multiplication(&CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS], scalar1,
                                 &CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS],
                                 &CASPER_MEM[INOUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS], scalar2);

    Jac_toAffine(&CASPER_MEM[LUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[LUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS],
                 &CASPER_MEM[INOUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS]);

    uint32_t one[12] = {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    CASPER_MEMCPY(&CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS], one, NUM_LIMBS * sizeof(uint32_t));
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[LUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);
    multiply_casper(&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[LUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS],
                    &CASPER_MEM[INOUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS]);

    CASPER_MEMCPY(resX, (&CASPER_MEM[INOUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS]), NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(resY, (&CASPER_MEM[INOUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS]), NUM_LIMBS * sizeof(uint32_t));
}
#endif /* CASPER_ECC_P384 */

// CIOS Multiply. This is the Coarse Integrated form where the values are
// multiplied and reduced for each step of "i". This uses less memory and
// is faster as a result. Note that this is used to square as well as mul,
// so not as fast as pure squaring could be.
static void MultprecCiosMul_ct(
    uint32_t w_out[], const uint32_t a[], const uint32_t b[], const uint32_t Nmod[], const uint32_t *Np)
{
    int i;
    uint64_t *m64 = (uint64_t *)&msg_ret[kCASPER_RamOffset_M64];
    uint64_t Np64;
    uint64_t carry;
    uint64_t *a64, *b64, *w64, *N64;
    uint32_t *T1 = &CASPER_MEM[0], borrow;

    Np64 = *(uint64_t *)Np;

    a64 = (uint64_t *)a;
    b64 = (uint64_t *)b;
    w64 = (uint64_t *)w_out;
    N64 = (uint64_t *)Nmod;

    if (a)
    { /*  if !a, we are reducing only */
        PreZeroW(i, w_out);
    }
    SET_DWORD(&w64[N_dwordlen], 0);
    SET_DWORD(&w64[N_dwordlen + 1], 0);
    /*  with accelerator */

    /*  loop i and then reduce after each j round */
    for (i = 0; i < N_dwordlen; i++)
    {
        /*  Step 3. Iterate over N words of u using i - perform Multiply-accumulate */
        /*  push-pull: we do a*b and then separately m*n (reduce) */
        if (a)
        { /*  if mul&reduce vs. reduce only */
            carry = GET_DWORD(&w64[N_dwordlen]);
            Accel_SetABCD_Addr(CA_MK_OFF(&b64[i]), CA_MK_OFF(a64));
            Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpMul6464FullSum, CA_MK_OFF(w64)));
            Accel_done();
            /*  max carry is contained since ~0*~0=0xFFFE0001+0xFFFF=0xFFFF0000, */
            /*  so max carry is 0xFFFF and 0xFFFF0000+0xFFFF=0xFFFFFFFF */
            /*  accel took care of w_out[N_wordlen] & +1, so we just take care of the next double word if carry=1 */
            /*  w64[N_dwordlen+1] = g_carry; */
            carry = (GET_DWORD(&w64[N_dwordlen]) < carry);
            SET_DWORD(&w64[N_dwordlen + 1], carry);
        }
        SET_DWORD(&m64[0], GET_DWORD(&w64[0]) * Np64); /*  prime for 1st; modulo a double-word */

        /*  we are reducing, so the 1st [0th] 64 bit value product is tossed, but we */
        /*  need its carry. We let the accel do this separately - really need a mode to */
        /*  do this "reduce" since it is natural */
        carry = GET_DWORD(&w64[N_dwordlen]);
        Accel_SetABCD_Addr(CA_MK_OFF(m64), CA_MK_OFF(&N64[0]));
        Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpMul6464FullSum, CA_MK_OFF(&w64[0])));
        Accel_done();
        carry = (GET_DWORD(&w64[N_dwordlen]) < carry);

        Accel_SetABCD_Addr(CA_MK_OFF(&w64[1]), 0);
        Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen - 1, kCASPER_OpCopy, CA_MK_OFF(&w64[0])));

        Accel_done();
        SET_DWORD(&w64[N_dwordlen], (GET_DWORD(&w64[N_dwordlen + 1]) + carry));
    }

    /*  now check if need to subtract Nmod */
    CASPER_MEMCPY_I2I(T1, w_out, (NUM_LIMBS + 1) * sizeof(uint32_t));

    /* Compute w = w - N */
    Accel_SetABCD_Addr(CA_MK_OFF(Nmod), 0);
    Accel_crypto_mul(Accel_IterOpcodeResaddr(N_dwordlen, kCASPER_OpSub64, CA_MK_OFF(w_out)));
    Accel_done();

    // if w_out > T1 then there was a borrow
    borrow = (GET_WORD(&((uint32_t *)w_out)[NUM_LIMBS]) > GET_WORD(&T1[NUM_LIMBS]));

    SET_WORD(&w_out[NUM_LIMBS + 1], 0);
    SET_WORD(&w_out[NUM_LIMBS], 0);
    casper_select(w_out, w_out, T1, borrow, NUM_LIMBS);
}

/* Compute C = A - B % mod
* Assumes all operand have two extra limbs to store carry.
*/
void CASPER_montsub(uint32_t *C, uint32_t *A, uint32_t *B, uint32_t *mod)
{
    uint64_t *b64, *c64, *m64, *tmp;
    int borrow;

    b64 = (uint64_t *)B;
    c64 = (uint64_t *)C;
    m64 = (uint64_t *)mod;

    tmp = (uint64_t *)&CASPER_MEM[0];

    CASPER_MEMCPY(tmp, A, NUM_LIMBS * sizeof(uint32_t));
    // uint32_t temp32 = GET_WORD(&tmp[NUM_LIMBS - 1]);

    /* Compute tmp = A - B. */
    Accel_SetABCD_Addr(CA_MK_OFF(b64), 0);

    Accel_crypto_mul(Accel_IterOpcodeResaddr(NUM_LIMBS / 2 - 1, kCASPER_OpSub64, CA_MK_OFF(tmp)));
    Accel_done();

    // borrow = (GET_WORD(&((uint32_t*)tmp)[NUM_LIMBS - 1]) > temp32);
    borrow = ((GET_WORD(&((uint32_t *)tmp)[NUM_LIMBS - 1])) > GET_WORD(&A[NUM_LIMBS - 1]));
    CASPER_MEMCPY(c64, tmp, NUM_LIMBS * sizeof(uint32_t));

    /* Compute C = Mod + tmp */
    Accel_SetABCD_Addr(CA_MK_OFF(m64), 0);
    Accel_crypto_mul(Accel_IterOpcodeResaddr(NUM_LIMBS / 2 - 1, kCASPER_OpAdd64, CA_MK_OFF(c64)));
    Accel_done();

    casper_select(C, (uint32_t *)tmp, C, borrow, NUM_LIMBS);
}

/* Compute C = A + B % mod
* Assumes all operand have two extra limbs to store carry.
*/
void CASPER_montadd(uint32_t *C, uint32_t *A, uint32_t *B, uint32_t *mod)
{
    uint64_t *b64, *c64, *m64, *tmp;
    int borrow;

    b64 = (uint64_t *)B;
    c64 = (uint64_t *)C;
    m64 = (uint64_t *)mod;

    tmp = (uint64_t *)&CASPER_MEM[0];

    CASPER_MEMCPY(tmp, A, NUM_LIMBS * sizeof(uint32_t));
    SET_DWORD(&tmp[NUM_LIMBS / 2], 0);
    SET_DWORD(&b64[NUM_LIMBS / 2], 0);
    SET_DWORD(&m64[NUM_LIMBS / 2], 0);

    /* Compute tmp = A + B using one additonal double-length limb. */
    Accel_SetABCD_Addr(CA_MK_OFF(b64), 0);

    Accel_crypto_mul(Accel_IterOpcodeResaddr(NUM_LIMBS / 2, kCASPER_OpAdd64, CA_MK_OFF(tmp)));
    Accel_done();

    CASPER_MEMCPY(c64, tmp, (NUM_LIMBS + 2) * sizeof(uint32_t));

    /* Compute C = Mod - tmp */
    Accel_SetABCD_Addr(CA_MK_OFF(m64), 0);
    Accel_crypto_mul(Accel_IterOpcodeResaddr(NUM_LIMBS / 2, kCASPER_OpSub64, CA_MK_OFF(c64)));
    Accel_done();

    // borrow = g_carry;
    borrow = (GET_WORD(&C[NUM_LIMBS]) > GET_WORD(&(((uint32_t *)tmp)[NUM_LIMBS])));
    casper_select(C, C, (uint32_t *)tmp, borrow, NUM_LIMBS);
}

/* Compute c = a/2 mod p where b is scratch space. */
void CASPER_half(uint32_t *c, uint32_t *a, uint32_t *b)
{
    shiftright(b, a, 1); /* Compute a/2 and (a+p)/2       */

    /* Compute tmp = a + p using one additonal double-length limb. */
    CASPER_MEMCPY(c, a, NUM_LIMBS * sizeof(uint32_t));
    SET_WORD(&c[NUM_LIMBS], 0);
    SET_WORD(&c[NUM_LIMBS + 1], 0);

    Accel_SetABCD_Addr(CA_MK_OFF(((uint64_t *)&CASPER_MEM[MOD_SCRATCH_START])), 0);
    Accel_crypto_mul(Accel_IterOpcodeResaddr(NUM_LIMBS / 2, kCASPER_OpAdd64, CA_MK_OFF(((uint64_t *)c))));
    Accel_done();

    shiftright(c, c, 1);
    SET_WORD(&c[NUM_LIMBS - 1], GET_WORD(&c[NUM_LIMBS - 1]) | (GET_WORD(&c[NUM_LIMBS]) << 31));
    SET_WORD(&c[NUM_LIMBS], 0);
    casper_select(c, b, c, (GET_WORD(&a[0]) & 1), NUM_LIMBS);
}

static uint32_t casper_get_word(uint32_t *addr)
{
    return GET_WORD(addr);
}

#if CASPER_ECC_P256
/* Shift right by 1 <= c <= 31. */
static void shiftright(uint32_t *z, uint32_t *x, int c)
{
    do
    {
        SET_WORD((&z[0]), (GET_WORD(&x[1]) << (32 - (c))) | (GET_WORD(&x[0]) >> (c)));
        SET_WORD((&z[1]), (GET_WORD(&x[2]) << (32 - (c))) | (GET_WORD(&x[1]) >> (c)));
        SET_WORD((&z[2]), (GET_WORD(&x[3]) << (32 - (c))) | (GET_WORD(&x[2]) >> (c)));
        SET_WORD((&z[3]), (GET_WORD(&x[4]) << (32 - (c))) | (GET_WORD(&x[3]) >> (c)));
        SET_WORD((&z[4]), (GET_WORD(&x[5]) << (32 - (c))) | (GET_WORD(&x[4]) >> (c)));
        SET_WORD((&z[5]), (GET_WORD(&x[6]) << (32 - (c))) | (GET_WORD(&x[5]) >> (c)));
        SET_WORD((&z[6]), (GET_WORD(&x[7]) << (32 - (c))) | (GET_WORD(&x[6]) >> (c)));
        SET_WORD((&z[7]), (GET_WORD(&x[7]) >> (c)));

    } while (0);
}

/* Shift left by 1 <= c <= 31. */
static void shiftleft(uint32_t *z, uint32_t *x, int c)
{
    do
    {
        SET_WORD(&z[7], (GET_WORD(&x[7]) << (c)) | GET_WORD(&z[6]) >> (32 - (c)));
        SET_WORD(&z[6], (GET_WORD(&x[6]) << (c)) | GET_WORD(&z[5]) >> (32 - (c)));
        SET_WORD(&z[5], (GET_WORD(&x[5]) << (c)) | GET_WORD(&z[4]) >> (32 - (c)));
        SET_WORD(&z[4], (GET_WORD(&x[4]) << (c)) | GET_WORD(&z[3]) >> (32 - (c)));
        SET_WORD(&z[3], (GET_WORD(&x[3]) << (c)) | GET_WORD(&z[2]) >> (32 - (c)));
        SET_WORD(&z[2], (GET_WORD(&x[2]) << (c)) | GET_WORD(&z[1]) >> (32 - (c)));
        SET_WORD(&z[1], (GET_WORD(&x[1]) << (c)) | GET_WORD(&z[0]) >> (32 - (c)));
        SET_WORD(&z[0], (GET_WORD(&x[0]) << (c)));
    } while (0);
}
#else
/* Shift right by 1 <= c <= 31. */
static void shiftright(uint32_t *z, uint32_t *x, int c)
{
    do
    {
        SET_WORD(&z[0], (GET_WORD(&x[1]) << (32 - (c))) | (GET_WORD(&x[0]) >> (c)));
        SET_WORD(&z[1], (GET_WORD(&x[2]) << (32 - (c))) | (GET_WORD(&x[1]) >> (c)));
        SET_WORD(&z[2], (GET_WORD(&x[3]) << (32 - (c))) | (GET_WORD(&x[2]) >> (c)));
        SET_WORD(&z[3], (GET_WORD(&x[4]) << (32 - (c))) | (GET_WORD(&x[3]) >> (c)));
        SET_WORD(&z[4], (GET_WORD(&x[5]) << (32 - (c))) | (GET_WORD(&x[4]) >> (c)));
        SET_WORD(&z[5], (GET_WORD(&x[6]) << (32 - (c))) | (GET_WORD(&x[5]) >> (c)));
        SET_WORD(&z[6], (GET_WORD(&x[7]) << (32 - (c))) | (GET_WORD(&x[6]) >> (c)));
        SET_WORD(&z[7], (GET_WORD(&x[8]) << (32 - (c))) | (GET_WORD(&x[7]) >> (c)));
        SET_WORD(&z[8], (GET_WORD(&x[9]) << (32 - (c))) | (GET_WORD(&x[8]) >> (c)));
        SET_WORD(&z[9], (GET_WORD(&x[10]) << (32 - (c))) | (GET_WORD(&x[9]) >> (c)));
        SET_WORD(&z[10], (GET_WORD(&x[11]) << (32 - (c))) | (GET_WORD(&x[10]) >> (c)));
        SET_WORD(&z[11], (GET_WORD(&x[11]) >> (c)));
    } while (0);
}

/* Shift left by 1 <= c <= 31. */
static void shiftleft(uint32_t *z, uint32_t *x, int c)
{
    do
    {
        SET_WORD(&z[11], (GET_WORD(&x[11]) << (c)) | GET_WORD(&z[10]) >> (32 - (c)));
        SET_WORD(&z[10], (GET_WORD(&x[10]) << (c)) | GET_WORD(&z[9]) >> (32 - (c)));
        SET_WORD(&z[9], (GET_WORD(&x[9]) << (c)) | GET_WORD(&z[8]) >> (32 - (c)));
        SET_WORD(&z[8], (GET_WORD(&x[8]) << (c)) | GET_WORD(&z[7]) >> (32 - (c)));
        SET_WORD(&z[7], (GET_WORD(&x[7]) << (c)) | GET_WORD(&z[6]) >> (32 - (c)));
        SET_WORD(&z[6], (GET_WORD(&x[6]) << (c)) | GET_WORD(&z[5]) >> (32 - (c)));
        SET_WORD(&z[5], (GET_WORD(&x[5]) << (c)) | GET_WORD(&z[4]) >> (32 - (c)));
        SET_WORD(&z[4], (GET_WORD(&x[4]) << (c)) | GET_WORD(&z[3]) >> (32 - (c)));
        SET_WORD(&z[3], (GET_WORD(&x[3]) << (c)) | GET_WORD(&z[2]) >> (32 - (c)));
        SET_WORD(&z[2], (GET_WORD(&x[2]) << (c)) | GET_WORD(&z[1]) >> (32 - (c)));
        SET_WORD(&z[1], (GET_WORD(&x[1]) << (c)) | GET_WORD(&z[0]) >> (32 - (c)));
        SET_WORD(&z[0], (GET_WORD(&x[0]) << (c)));
    } while (0);
}
#endif

/* Convert a projective point (X1 : Y1 : Z1)
 * to the affine point (X3, Y3) = (X1/Z1^2,Y1/Z1^3)
 * The memory of (X3, Y3) and (X1 : Y1 : Z1) should not overlap
 */
void Jac_toAffine(uint32_t *X3, uint32_t *Y3, uint32_t *X1, uint32_t *Y1, uint32_t *Z1)
{
    uint32_t *T1, *T2;

    T1 = &CASPER_MEM[ECC_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    T2 = &CASPER_MEM[ECC_SCRATCH_START + 1 * CASPER_NUM_LIMBS];

    square_casper(T1, Z1);       // Z^2
    multiply_casper(T2, T1, Z1); // Z^3

    // Montgomery inverse
    invert(T1, T2); // Z^-3

    multiply_casper(Y3, Y1, T1); // Y3 = Y/Z^3
    multiply_casper(T2, T1, Z1); // Z^-2
    multiply_casper(X3, X1, T2); // X3 = X/Z^2
}

/* Return 1 if (X1: Y1: Z1) is on the curve
 *      Y^2 = X^3 -3XZ^4 + bZ^6
 * and return 0 otherwise.
 */
int Jac_oncurve(uint32_t *X1, uint32_t *Y1, uint32_t *Z1, uint32_t *b)
{
    uint32_t *T1, *T2, *T3, *T4, *T5, *T6;
    int m;

    T1 = &CASPER_MEM[ECC_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    T2 = &CASPER_MEM[ECC_SCRATCH_START + 1 * CASPER_NUM_LIMBS];
    T3 = &CASPER_MEM[ECC_SCRATCH_START + 2 * CASPER_NUM_LIMBS];
    T4 = &CASPER_MEM[ECC_SCRATCH_START + 3 * CASPER_NUM_LIMBS];
    T5 = &CASPER_MEM[ECC_SCRATCH_START + 4 * CASPER_NUM_LIMBS];
    T6 = &CASPER_MEM[ECC_SCRATCH_START + 5 * CASPER_NUM_LIMBS];

    square_casper(T1, Y1);       // Y^2
    square_casper(T6, X1);       // X^2
    multiply_casper(T2, T6, X1); // X^3

    square_casper(T3, Z1);       // Z^2
    square_casper(T4, T3);       // Z^4
    multiply_casper(T6, T4, T3); // Z^6
    multiply_casper(T3, b, T6);  // bZ^6

    multiply_casper(T6, T4, X1); // XZ^4

    mul2_casper(T5, T6);
    add_casper(T4, T5, T6); // 3XZ^4

    sub_casper(T2, T2, T4); // X^3-3XZ^4
    add_casper(T2, T2, T3); // X^3-3XZ^4+bZ^6

    CASPER_ECC_equal(&m, T1, T2);
    if (m != 0)
    {
        return 0;
    }
    return 1;
}

/* Compute (X3 : Y3: Z3) = (X1: Y1: Z1) + (X2 : Y2 : Z2)
 *  where (X1: Y1: Z1) != (X2 : Y2 : Z2)
 * (X3 : Y3: Z3) may be the same as one of the inputs.
 */
void Jac_addition(uint32_t *X3,
                  uint32_t *Y3,
                  uint32_t *Z3,
                  uint32_t *X1,
                  uint32_t *Y1,
                  uint32_t *Z1,
                  uint32_t *X2,
                  uint32_t *Y2,
                  uint32_t *Z2)
{
    uint32_t *Z1Z1, *Z2Z2, *U1, *S1, *J, *H, *V, *t0, *t1;
    int m1, m2;

    Z1Z1 = &CASPER_MEM[ECC_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    Z2Z2 = &CASPER_MEM[ECC_SCRATCH_START + 1 * CASPER_NUM_LIMBS];
    U1 = &CASPER_MEM[ECC_SCRATCH_START + 2 * CASPER_NUM_LIMBS];
    S1 = &CASPER_MEM[ECC_SCRATCH_START + 3 * CASPER_NUM_LIMBS];
    J = &CASPER_MEM[ECC_SCRATCH_START + 4 * CASPER_NUM_LIMBS];
    H = &CASPER_MEM[ECC_SCRATCH_START + 5 * CASPER_NUM_LIMBS];
    V = &CASPER_MEM[ECC_SCRATCH_START + 6 * CASPER_NUM_LIMBS];
    t0 = &CASPER_MEM[ECC_SCRATCH_START + 7 * CASPER_NUM_LIMBS];
    t1 = &CASPER_MEM[ECC_SCRATCH_START + 8 * CASPER_NUM_LIMBS];

    CASPER_ECC_equal_to_zero(&m1, Z1);
    CASPER_ECC_equal_to_zero(&m2, Z2);
    if (m1 == 0)
    {
        CASPER_MEMCPY(X3, X2, NUM_LIMBS * 4);
        CASPER_MEMCPY(Y3, Y2, NUM_LIMBS * 4);
        CASPER_MEMCPY(Z3, Z2, NUM_LIMBS * 4);
        return;
    }
    if (m2 == 0)
    {
        CASPER_MEMCPY(X3, X1, NUM_LIMBS * 4);
        CASPER_MEMCPY(Y3, Y1, NUM_LIMBS * 4);
        CASPER_MEMCPY(Z3, Z1, NUM_LIMBS * 4);
        return;
    }

    square_casper(Z1Z1, Z1);
    square_casper(Z2Z2, Z2);
    multiply_casper(U1, X1, Z2Z2);
    multiply_casper(H, X2, Z1Z1); /* if H equals U1 then X's are the same */
    multiply_casper(t0, Z2, Z2Z2);
    multiply_casper(S1, Y1, t0);
    multiply_casper(t0, Z1, Z1Z1);
    multiply_casper(J, Y2, t0); /* if (S1 == J) then Y's are the same */

    CASPER_ECC_equal(&m1, H, U1); /* If H and U1 match then the X-coordinates are the same. */
    CASPER_ECC_equal(&m2, S1, J); /* If S1 and J match then the Y-coordinates are the same. */
    if (m1 == 0)
    {
        if (m2 == 0)
        {
            Jac_double(X3, Y3, Z3, X1, Y1, Z1);
            return;
        }
        /* else {
        We work with the point at infinity.
        The Z-coordinate will be set to zero in this function.
        } */
    }

    sub_casper(H, H, U1);
    mul2_casper(t0, H);
    square_casper(t1, t0);
    sub_casper(t0, J, S1);
    multiply_casper(J, H, t1);
    multiply_casper(V, U1, t1);
    mul2_casper(U1, t0);
    square_casper(t0, U1);
    mul2_casper(t1, V);
    sub_casper(t0, t0, J);
    sub_casper(X3, t0, t1);
    sub_casper(t0, V, X3);
    multiply_casper(t1, S1, J);
    mul2_casper(t1, t1);
    multiply_casper(V, U1, t0);
    sub_casper(Y3, V, t1);
    add_casper(V, Z1, Z2);
    square_casper(t1, V);
    sub_casper(t1, t1, Z1Z1);
    sub_casper(t1, t1, Z2Z2);
    multiply_casper(Z3, t1, H);
}

/* Compute (X3 : Y3: Z3) = (X1: Y1: Z1) + (X2, Y2)
 * where (X1: Y1: Z1) != (X2, Y2)
 * (X3 : Y3: Z3) may not overlap with (X1: Y1: Z1).
 * Source: 2004 Hankerson–Menezes–Vanstone, page 91.
 */
void Jac_add_affine(
    uint32_t *X3, uint32_t *Y3, uint32_t *Z3, uint32_t *X1, uint32_t *Y1, uint32_t *Z1, uint32_t *X2, uint32_t *Y2)
{
    uint32_t *T1, *T2, *T3, *T4, *T5;
    int m1, m2;

    T1 = &CASPER_MEM[ECC_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    T2 = &CASPER_MEM[ECC_SCRATCH_START + 1 * CASPER_NUM_LIMBS];
    T3 = &CASPER_MEM[ECC_SCRATCH_START + 2 * CASPER_NUM_LIMBS];
    T4 = &CASPER_MEM[ECC_SCRATCH_START + 3 * CASPER_NUM_LIMBS];
    T5 = &CASPER_MEM[ECC_SCRATCH_START + 4 * CASPER_NUM_LIMBS];

    CASPER_ECC_equal_to_zero(&m1, Z1);
    if (m1 == 0)
    {
        CASPER_MEMCPY(X3, X2, NUM_LIMBS * 4);
        CASPER_MEMCPY(Y3, Y2, NUM_LIMBS * 4);
        CASPER_MEMCPY(Z3, ONE, NUM_LIMBS * 4);
        return;
    }

    copy(T5, Z1);
    square_casper(T3, Z1);
    multiply_casper(T2, T3, Z1);
    multiply_casper(T4, T3, X2);
    multiply_casper(T3, T2, Y2);

    CASPER_ECC_equal(&m1, T4, X1);
    CASPER_ECC_equal(&m2, T3, Y1);
    if (m1 == 0)
    {
        if (m2 == 0)
        {
            Jac_double(X3, Y3, Z3, X1, Y1, Z1);
            return;
        }
        /* else {
          We work with the point at infinity.
          The Z-coordinate will be set to zero in this function.
        } */
    }

    sub_casper(T1, T4, X1);
    sub_casper(T2, T3, Y1);
    multiply_casper(Z3, T5, T1);
    square_casper(T3, T1);
    multiply_casper(T4, T3, T1);
    multiply_casper(T5, T3, X1);
    mul2_casper(T1, T5);
    square_casper(X3, T2);
    sub_casper(X3, X3, T1);
    sub_casper(X3, X3, T4);
    sub_casper(T3, T5, X3);
    multiply_casper(T1, T3, T2);
    multiply_casper(T2, T4, Y1);
    sub_casper(Y3, T1, T2);
}

extern uint32_t casper_get_word(uint32_t *addr);

/* Point doubling from: 2004 Hankerson–Menezes–Vanstone, page 91.
 * Compute (X3 : Y3: Z3) = (X1: Y1: Z1) + (X1 : Y1 : Z1)
 * (X3 : Y3: Z3) may be the same as the input.
 */
void Jac_double(uint32_t *X3, uint32_t *Y3, uint32_t *Z3, uint32_t *X1, uint32_t *Y1, uint32_t *Z1)
{
    uint32_t *T1, *T2, *T3, *T4, *T5;

    T1 = &CASPER_MEM[ECC_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    T2 = &CASPER_MEM[ECC_SCRATCH_START + 1 * CASPER_NUM_LIMBS];
    T3 = &CASPER_MEM[ECC_SCRATCH_START + 2 * CASPER_NUM_LIMBS];
    T4 = &CASPER_MEM[ECC_SCRATCH_START + 3 * CASPER_NUM_LIMBS];
    T5 = &CASPER_MEM[ECC_SCRATCH_START + 4 * CASPER_NUM_LIMBS];

    square_casper(T1, Z1);
    sub_casper(T3, X1, T1);
    add_casper(T1, X1, T1);
    multiply_casper(T4, T3, T1);

    mul2_casper(T3, T4);

    add_casper(T2, T3, T4);

    mul2_casper(Y3, Y1);

    copy(T5, Z1);
    multiply_casper(Z3, Y3, T5);

    square_casper(T5, Y3);

    multiply_casper(T3, T5, X1);

    square_casper(Y3, T5);

    half(T5, Y3, T4);

    square_casper(X3, T2);

    mul2_casper(T1, T3);

    sub_casper(X3, X3, T1);

    sub_casper(T1, T3, X3);

    multiply_casper(T3, T1, T2);

    sub_casper(Y3, T3, T5);
}

/* Recoding for a signed fixed window.
 * Source: https://eprint.iacr.org/2014/130.pdf, Algorithm 6
 * Recode the n-bit integer k into ciel(log2(n)/(w-1)) digits
 * where each digit is in
 * { +/- 1, +/- 3, ..., +/- 2^(w-1)-1 }
 * and put the result in c.
 */
static void recode(int8_t *c, uint32_t *k, int n, int w)
{
    int i, t;
    uint32_t K[NUM_LIMBS] = {0};
    memcpy(K, k, (size_t)ceil(n / 8.));
    t = (n + (w - 2)) / (w - 1);
    for (i = 0; i < t; i++)
    {
        c[i] = (K[0] & ((1 << w) - 1)) - (1 << (w - 1));
        shiftrightSysram(K, K, w - 1);
        add_n_1(K, K, (uint32_t)c[i] >> 31, NUM_LIMBS);
    }
    c[t] = K[0];
}

static uint32_t sub_n(uint32_t *c, uint32_t *a, uint32_t *b, int n)
{
    int i;
    uint32_t borrow;
    sub_borrowout(borrow, GET_WORD(&c[0]), a[0], GET_WORD(&b[0]));
    for (i = 1; i < n; i++)
    {
        sub_borrowin_borrowout(borrow, GET_WORD(&c[i]), a[i], GET_WORD(&b[i]), borrow);
    }
    return borrow;
}

#if 0
/* Dumb n-limb subtraction of c=a-b, return borrow. */
static uint32_t sub_n_1(uint32_t *c, uint32_t *a, uint32_t b, int n) {
  int i;
  uint32_t borrow;
  sub_borrowout(borrow, c[0], a[0], b);
  for (i = 1; i < n; i++) {
    sub_borrowin_borrowout_1(borrow, c[i], a[i], borrow);
  }
  return borrow;
}

/* Dumb n-limb addition of c=a+b, return carry. */
static uint32_t add_n(uint32_t *c, uint32_t *a, uint32_t *b, int n) {
  int i;
  uint32_t carry;
  add_cout(carry, c[0], a[0], b[0]);
  for (i = 1; i < n; i++) {
    add_cout_cin(carry, c[i], a[i], b[i], carry);
  }
  return carry;
}
#endif

/* Dumb n-limb addition of c=a+b, return carry. */
static uint32_t add_n_1(uint32_t *c, uint32_t *a, uint32_t b, int n)
{
    int i;
    uint32_t carry;
    add_cout(carry, c[0], a[0], b);
    for (i = 1; i < n; i++)
    {
        add_cout_cin(carry, c[i], a[i], 0, carry);
    }
    return carry;
}

// http://graphics.stanford.edu/~seander/bithacks.html#IntegerAbs
static uint8_t int8abs(int8_t v)
{
    int8_t const mask = v >> 7;
    return (v + mask) ^ mask;
}

/* Constant time elliptic curve scalar multiplication.
 * Source: https://eprint.iacr.org/2014/130.pdf
 * when using w = 4.
 * Computes (X3 : Y3 : Z3) = k * (X1, Y1) \in E(F_p)
 * p is the prime used to define the finite field F_p
 * q is the (prime) order of the curve
 */
void Jac_scalar_multiplication(
    uint32_t *X3, uint32_t *Y3, uint32_t *Z3, uint32_t *X1, uint32_t *Y1, uint32_t *k, uint32_t *p, uint32_t *q)
{
    uint32_t *scalar, *M, *X, *Y, *Z, *mem;
    int i, sign, odd;
    // int8_t *rec;
    uint8_t index;

    /* Point to the start of the LUT table space. */
    mem = &CASPER_MEM[LUT_SCRATCH_START];

    scalar = &CASPER_MEM[LUT_SCRATCH_START + 12 * CASPER_NUM_LIMBS];
    X = &CASPER_MEM[LUT_SCRATCH_START + 13 * CASPER_NUM_LIMBS];
    Y = &CASPER_MEM[LUT_SCRATCH_START + 14 * CASPER_NUM_LIMBS];
    Z = &CASPER_MEM[LUT_SCRATCH_START + 15 * CASPER_NUM_LIMBS];
    M = &CASPER_MEM[LUT_SCRATCH_START + 16 * CASPER_NUM_LIMBS];

    /* Point to memory the recoded scalar.
     * CASPER_RECODE_LENGTH bytes is needed.
     */
    // rec = (int8_t*)&CASPER_MEM[LUT_SCRATCH_START + 17 * CASPER_NUM_LIMBS];
    int8_t rec[CASPER_RECODE_LENGTH];

    CASPER_MEMCPY(scalar, k, sizeof(uint32_t) * NUM_LIMBS);

/* Precomputation: compute 1*P, 3*P, 5*P, and 7*P */
#define LUT(P, x) (mem + (3 * ((P)-1) / 2 + (x)) * CASPER_NUM_LIMBS)

    /* Set 1*P */
    copy(Z3, ONE);
    copy(LUT(1, 0), X1);
    copy(LUT(1, 1), Y1);
    copy(LUT(1, 2), Z3);

    /* Compute 2*P */
    Jac_double(X3, Y3, Z3, X1, Y1, Z3);

    /* Compute 3*P = 2P + P */
    Jac_add_affine(LUT(3, 0), LUT(3, 1), LUT(3, 2), X3, Y3, Z3, X1, Y1);

    /* Compute 5*P = 3P + 2P */
    Jac_addition(LUT(5, 0), LUT(5, 1), LUT(5, 2), LUT(3, 0), LUT(3, 1), LUT(3, 2), X3, Y3, Z3);

    /* Compute 7*P = 5P + 2P */
    Jac_addition(LUT(7, 0), LUT(7, 1), LUT(7, 2), LUT(5, 0), LUT(5, 1), LUT(5, 2), X3, Y3, Z3);

    /* Recode the scalar */
    odd = casper_get_word(&scalar[0]) & 1u;
    sub_n(M, q, scalar, NUM_LIMBS); // todo!!!
    casper_select(scalar, M, scalar, odd, NUM_LIMBS);

    /* Use n=384 and w=4 --> compute ciel(384/3) = 128 + 1 digits */
    uint32_t scalarSysram[/*CASPER_*/ NUM_LIMBS];
    CASPER_MEMCPY(scalarSysram, scalar, /*CASPER_*/ NUM_LIMBS * sizeof(uint32_t));
    recode(rec, scalarSysram, N_bitlen, 4);

    /* Set the first value. */
    index = int8abs(rec[CASPER_RECODE_LENGTH - 1]);
    sign = ((uint8_t)rec[CASPER_RECODE_LENGTH - 1]) >> 7;
    copy(X3, LUT(index, 0));
    copy(Y3, LUT(index, 1));
    copy(Z3, LUT(index, 2));

/* Get the correct LUT element in constant time by touching
 * all elements and masking out the correct one.
 */

#define GET_LUT(x, y, z, index)                       \
    do                                                \
    {                                                 \
        int m;                                        \
        copy(x, LUT(1, 0));                           \
        copy(y, LUT(1, 1));                           \
        copy(z, LUT(1, 2));                           \
        m = (index == 3);                             \
        casper_select(x, x, LUT(3, 0), m, NUM_LIMBS); \
        casper_select(y, y, LUT(3, 1), m, NUM_LIMBS); \
        casper_select(z, z, LUT(3, 2), m, NUM_LIMBS); \
        m = (index == 5);                             \
        casper_select(x, x, LUT(5, 0), m, NUM_LIMBS); \
        casper_select(y, y, LUT(5, 1), m, NUM_LIMBS); \
        casper_select(z, z, LUT(5, 2), m, NUM_LIMBS); \
        m = (index == 7);                             \
        casper_select(x, x, LUT(7, 0), m, NUM_LIMBS); \
        casper_select(y, y, LUT(7, 1), m, NUM_LIMBS); \
        casper_select(z, z, LUT(7, 2), m, NUM_LIMBS); \
    } while (0)

    GET_LUT(X3, Y3, Z3, index);

    /* Compute -y and select the positive or negative point. */
    sub_n(M, p, Y3, NUM_LIMBS); // todo!!!
    casper_select(Y3, Y3, M, sign, NUM_LIMBS);

    for (i = CASPER_RECODE_LENGTH - 2; i >= 0; i--)
    {
        Jac_double(X3, Y3, Z3, X3, Y3, Z3);
        Jac_double(X3, Y3, Z3, X3, Y3, Z3);
        Jac_double(X3, Y3, Z3, X3, Y3, Z3);

        index = int8abs(rec[i]);
        sign = ((uint8_t)rec[i]) >> 7;

        GET_LUT(X, Y, Z, index);

        /* Compute -y and select the positive or negative point. */
        sub_n(scalar, p, Y, NUM_LIMBS); // todo!!!
        casper_select(scalar, Y, scalar, sign, NUM_LIMBS);

        Jac_addition(X3, Y3, Z3, X3, Y3, Z3, X, scalar, Z);
    }

    sub_n(M, p, Y3, NUM_LIMBS); // todo!!!

    casper_select(Y3, M, Y3, odd, NUM_LIMBS);
}

#undef LUT
#undef GET_LUT

/*
 * Pre-compute the following 16 points:
 * 00 00 = 0*P + 0*Q  <-- Not needed when using sliding windows
 * 00 01 = 0*P + 1*Q  <-- Not needed when using sliding windows
 * 00 10 = 0*P + 2*Q
 * 00 11 = 0*P + 3*Q
 *
 * 01 00 = 1*P + 0*Q  <-- Not needed when using sliding windows
 * 01 01 = 1*P + 1*Q  <-- Not needed when using sliding windows
 * 01 10 = 1*P + 2*Q
 * 01 11 = 1*P + 3*Q
 *
 * 10 00 = 2*P + 0*Q
 * 10 01 = 2*P + 1*Q
 * 10 10 = 2*P + 2*Q
 * 10 11 = 2*P + 3*Q
 *
 * 11 00 = 3*P + 0*Q
 * 11 01 = 3*P + 1*Q
 * 11 10 = 3*P + 2*Q
 * 11 11 = 3*P + 3*Q
 *
 * index = (bitsi||bitsj)-2 - (biti != 0)*2
 *
 * Input:   P = (X1 : Y1 : Z1) and
 *          Q = (X2 : Y2 : Z2)
 * Output: mem, memory location for the LUT.
 */

#define LUT_LIMBS NUM_LIMBS

static void precompute_double_scalar_LUT(uint32_t *Px, uint32_t *Py, uint32_t *Qx, uint32_t *Qy)
{
    uint32_t *Q2x, *Q2y, *Q2z, *P2x, *P2y, *P2z, *Z, *mem;
    int index = 0;

    Q2x = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS + 0 * CASPER_NUM_LIMBS];
    Q2y = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS + 1 * CASPER_NUM_LIMBS];
    Q2z = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS + 2 * CASPER_NUM_LIMBS];

    /* Re-use memory from different scratch space since no
    * projective point addition is used below. */
    P2x = &CASPER_MEM[ECC_SCRATCH_START + 5 * CASPER_NUM_LIMBS];
    P2z = &CASPER_MEM[ECC_SCRATCH_START + 6 * CASPER_NUM_LIMBS];
    P2y = &CASPER_MEM[ECC_SCRATCH_START + 7 * CASPER_NUM_LIMBS];
    Z = &CASPER_MEM[ECC_SCRATCH_START + 8 * CASPER_NUM_LIMBS];

    mem = &CASPER_MEM[LUT_SCRATCH_START];

    copy(Z, ONE);

    // 00 10 = 0*P + 2*Q
    Jac_double(Q2x, Q2y, Q2z, Qx, Qy, Z);
    copy(&mem[index], Q2x);
    index += LUT_LIMBS;
    copy(&mem[index], Q2y);
    index += LUT_LIMBS;
    copy(&mem[index], Q2z);
    index += LUT_LIMBS;

    // 00 11 = 0*P + 3*Q
    Jac_add_affine(P2x, P2y, P2z, Q2x, Q2y, Q2z, Qx, Qy);
    copy(&mem[index], P2x);
    index += LUT_LIMBS;
    copy(&mem[index], P2y);
    index += LUT_LIMBS;
    copy(&mem[index], P2z);
    index += LUT_LIMBS;

    // 01 10 = 1*P + 2*Q
    Jac_add_affine(P2x, P2y, P2z, Q2x, Q2y, Q2z, Px, Py);
    copy(&mem[index], P2x);
    index += LUT_LIMBS;
    copy(&mem[index], P2y);
    index += LUT_LIMBS;
    copy(&mem[index], P2z);
    index += LUT_LIMBS;

    // 01 11 = 1*P + 3*Q
    Jac_add_affine(P2x, P2y, P2z, P2x, P2y, P2z, Qx, Qy);
    copy(&mem[index], P2x);
    index += LUT_LIMBS;
    copy(&mem[index], P2y);
    index += LUT_LIMBS;
    copy(&mem[index], P2z);
    index += LUT_LIMBS;

    // 10 00 = 2*P + 0*Q
    Jac_double(P2x, P2y, P2z, Px, Py, Z);
    copy(&mem[index], P2x);
    index += LUT_LIMBS;
    copy(&mem[index], P2y);
    index += LUT_LIMBS;
    copy(&mem[index], P2z);
    index += LUT_LIMBS;

    // 10 01 = 2*P + 1*Q
    Jac_add_affine(Q2x, Q2y, Q2z, P2x, P2y, P2z, Qx, Qy);
    copy(&mem[index], Q2x);
    index += LUT_LIMBS;
    copy(&mem[index], Q2y);
    index += LUT_LIMBS;
    copy(&mem[index], Q2z);
    index += LUT_LIMBS;

    // 10 10 = 2*P + 2*Q
    Jac_add_affine(Q2x, Q2y, Q2z, Q2x, Q2y, Q2z, Qx, Qy);
    copy(&mem[index], Q2x);
    index += LUT_LIMBS;
    copy(&mem[index], Q2y);
    index += LUT_LIMBS;
    copy(&mem[index], Q2z);
    index += LUT_LIMBS;

    // 10 11 = 2*P + 3*Q
    Jac_add_affine(Q2x, Q2y, Q2z, Q2x, Q2y, Q2z, Qx, Qy);
    copy(&mem[index], Q2x);
    index += LUT_LIMBS;
    copy(&mem[index], Q2y);
    index += LUT_LIMBS;
    copy(&mem[index], Q2z);
    index += LUT_LIMBS;

    // 11 00 = 3*P + 0*Q
    Jac_add_affine(P2x, P2y, P2z, P2x, P2y, P2z, Px, Py);
    copy(&mem[index], P2x);
    index += LUT_LIMBS;
    copy(&mem[index], P2y);
    index += LUT_LIMBS;
    copy(&mem[index], P2z);
    index += LUT_LIMBS;

    // 11 01 = 3*P + 1*Q
    Jac_add_affine(Q2x, Q2y, Q2z, P2x, P2y, P2z, Qx, Qy);
    copy(&mem[index], Q2x);
    index += LUT_LIMBS;
    copy(&mem[index], Q2y);
    index += LUT_LIMBS;
    copy(&mem[index], Q2z);
    index += LUT_LIMBS;

    // 11 10 = 3*P + 2*Q
    Jac_add_affine(Q2x, Q2y, Q2z, Q2x, Q2y, Q2z, Qx, Qy);
    copy(&mem[index], Q2x);
    index += LUT_LIMBS;
    copy(&mem[index], Q2y);
    index += LUT_LIMBS;
    copy(&mem[index], Q2z);
    index += LUT_LIMBS;

    // 11 11 = 3*P + 3*Q
    Jac_add_affine(Q2x, Q2y, Q2z, Q2x, Q2y, Q2z, Qx, Qy);
    copy(&mem[index], Q2x);
    index += LUT_LIMBS;
    copy(&mem[index], Q2y);
    index += LUT_LIMBS;
    copy(&mem[index], Q2z);
    index += LUT_LIMBS;
}

#define GETLUTX(x) (3 * (x)*LUT_LIMBS)
#define GETLUTY(x) (3 * (x)*LUT_LIMBS + 1 * LUT_LIMBS)
#define GETLUTZ(x) (3 * (x)*LUT_LIMBS + 2 * LUT_LIMBS)

/* Compute the double scalar multiplication
 * (X3 : Y3 : Z3) = k1 * (X1, Y1) + k2 * (X2, Y2)
 * Using Shamir's trick and precomputing 16 points.
 * This code is *not* constant time since this is used
 * for verification only.
 */
void double_scalar_multiplication(uint32_t *X3,
                                  uint32_t *Y3,
                                  uint32_t *Z3,
                                  uint32_t *X1,
                                  uint32_t *Y1,
                                  uint32_t *k1,
                                  uint32_t *X2,
                                  uint32_t *Y2,
                                  uint32_t *k2)
{
    uint32_t index, c = 0;
    uint32_t *p1, *p2, x1, x2, *lut, *Tx, *Ty, *Tz;

    precompute_double_scalar_LUT(X1, Y1, X2, Y2);

    lut = &CASPER_MEM[LUT_SCRATCH_START];
    p1 = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS];
    p2 = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS + 1 * CASPER_NUM_LIMBS];

    Tx = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS + 2 * CASPER_NUM_LIMBS];
    Ty = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS + 3 * CASPER_NUM_LIMBS];
    Tz = &CASPER_MEM[LUT_SCRATCH_START + 48 * LUT_LIMBS + 4 * CASPER_NUM_LIMBS];

    CASPER_MEMCPY(p1, k1, sizeof(uint32_t) * NUM_LIMBS);
    CASPER_MEMCPY(p2, k2, sizeof(uint32_t) * NUM_LIMBS);

    /* Check if we can slide. */
    while (((casper_get_word(&p1[NUM_LIMBS - 1]) | casper_get_word(&p2[NUM_LIMBS - 1])) >> 31) == 0 && c < 256)
    {
        shiftleft(p1, p1, 1);
        shiftleft(p2, p2, 1);
        c++;
        /* No doubling needed. */
    }

    /* Set the first value. */
    x1 = casper_get_word(&p1[NUM_LIMBS - 1]) >> 30;
    x2 = casper_get_word(&p2[NUM_LIMBS - 1]) >> 30;
    index = (x2 | (x1 << 2)) - 2 - (x1 != 0) * 2;
    shiftleft(p1, p1, 2);
    shiftleft(p2, p2, 2);

    copy(X3, &lut[GETLUTX(index)]);
    copy(Y3, &lut[GETLUTY(index)]);
    copy(Z3, &lut[GETLUTZ(index)]);
    c += 2;

// todo: create an is_zero function
#if CASPER_ECC_P256
    while ((casper_get_word(&p1[0]) | casper_get_word(&p1[1]) | casper_get_word(&p1[2]) | casper_get_word(&p1[3]) |
            casper_get_word(&p1[4]) | casper_get_word(&p1[5]) | casper_get_word(&p1[6]) | casper_get_word(&p1[7]) |
            casper_get_word(&p2[0]) | casper_get_word(&p2[1]) | casper_get_word(&p2[2]) | casper_get_word(&p2[3]) |
            casper_get_word(&p2[4]) | casper_get_word(&p2[5]) | casper_get_word(&p2[6]) | casper_get_word(&p2[7])) != 0)
    {
#elif CASPER_ECC_P384
    while ((casper_get_word(&p1[0]) | casper_get_word(&p1[1]) | casper_get_word(&p1[2]) | casper_get_word(&p1[3]) |
            casper_get_word(&p1[4]) | casper_get_word(&p1[5]) | casper_get_word(&p1[6]) | casper_get_word(&p1[7]) |
            casper_get_word(&p1[8]) | casper_get_word(&p1[9]) | casper_get_word(&p1[10]) | casper_get_word(&p1[11]) |
            casper_get_word(&p2[0]) | casper_get_word(&p2[1]) | casper_get_word(&p2[2]) | casper_get_word(&p2[3]) |
            casper_get_word(&p2[4]) | casper_get_word(&p2[5]) | casper_get_word(&p2[6]) | casper_get_word(&p2[7]) |
            casper_get_word(&p2[8]) | casper_get_word(&p2[9]) | casper_get_word(&p2[10]) | casper_get_word(&p2[11])) !=
           0)
    {
#endif
        /* Check if we can slide. */
        while (((casper_get_word(&p1[NUM_LIMBS - 1]) | casper_get_word(&p2[NUM_LIMBS - 1])) >> 31) == 0 && c < N_bitlen)
        {
            shiftleft(p1, p1, 1);
            shiftleft(p2, p2, 1);
            Jac_double(X3, Y3, Z3, X3, Y3, Z3);
            c++;
        }

        if (c >= (N_bitlen - 1))
            break;

        /* Double twice. */
        Jac_double(X3, Y3, Z3, X3, Y3, Z3);
        Jac_double(X3, Y3, Z3, X3, Y3, Z3);

        /* Add in the correct value. */
        x1 = casper_get_word(&p1[NUM_LIMBS - 1]) >> 30;
        x2 = casper_get_word(&p2[NUM_LIMBS - 1]) >> 30;
        index = (x2 | (x1 << 2)) - 2 - (x1 != 0) * 2;

        shiftleft(p1, p1, 2);
        shiftleft(p2, p2, 2);

        copy(Tx, &lut[GETLUTX(index)]);
        copy(Ty, &lut[GETLUTY(index)]);
        copy(Tz, &lut[GETLUTZ(index)]);

        Jac_addition(X3, Y3, Z3, X3, Y3, Z3, Tx, Ty,
                     Tz); //&lut[GETLUTX(index)], &lut[GETLUTY(index)], &lut[GETLUTZ(index)]);
        c += 2;
    }

    /* Special case in the end. */
    if (c == (N_bitlen - 1))
    {
        Jac_double(X3, Y3, Z3, X3, Y3, Z3);
        x1 = casper_get_word(&p1[NUM_LIMBS - 1]) >> 31;
        x2 = casper_get_word(&p2[NUM_LIMBS - 1]) >> 31;
        if (x1)
        {
            Jac_add_affine(X3, Y3, Z3, X3, Y3, Z3, X1, Y1);
        }
        if (x2)
        {
            Jac_add_affine(X3, Y3, Z3, X3, Y3, Z3, X2, Y2);
        }
        c++;
    }

    while (c < N_bitlen)
    {
        Jac_double(X3, Y3, Z3, X3, Y3, Z3);
        c++;
    }
}

#if CASPER_ECC_P256
static void invert_mod_p256(uint32_t *c, uint32_t *a)
{
    int i;
    uint32_t *t, *t2, *s1, *s2, *s4, *s8, *tmp;

    /* Assuming it is safe to use the ECC scratch size. */
    t = &CASPER_MEM[ECC_SCRATCH_START + 2 * CASPER_NUM_LIMBS];
    t2 = &CASPER_MEM[ECC_SCRATCH_START + 3 * CASPER_NUM_LIMBS];
    s1 = &CASPER_MEM[ECC_SCRATCH_START + 4 * CASPER_NUM_LIMBS];
    s2 = &CASPER_MEM[ECC_SCRATCH_START + 5 * CASPER_NUM_LIMBS];
    s4 = &CASPER_MEM[ECC_SCRATCH_START + 6 * CASPER_NUM_LIMBS];
    s8 = &CASPER_MEM[ECC_SCRATCH_START + 7 * CASPER_NUM_LIMBS];
    tmp = &CASPER_MEM[ECC_SCRATCH_START + 8 * CASPER_NUM_LIMBS];

    // t2 = n^(2^1)*n  # 11
    square_casper(tmp, a);
    multiply_casper(t2, tmp, a);

    // s1 = t2^(2^2)*t2  # F
    square_casper(s1, t2);
    square_casper(tmp, s1);
    multiply_casper(s1, tmp, t2);

    // s2 = s1^(2^4)*s1  # FF
    square_casper(s2, s1);
    // for (i = 1; i < 4; i++) square(s2, s2);
    square_casper(tmp, s2);
    square_casper(s2, tmp);
    square_casper(tmp, s2);
    multiply_casper(s2, tmp, s1);

    // s4 = s2^(2^8)*s2  # FFFF
    square_casper(s4, s2);
    for (i = 1; i < 7; i += 2)
    {
        square_casper(tmp, s4);
        square_casper(s4, tmp);
    }
    square_casper(tmp, s4);
    multiply_casper(s4, tmp, s2);

    // s8 = s4^(2^16)*s4  # FFFFFFFF
    square_casper(s8, s4);
    for (i = 1; i < 15; i += 2)
    {
        square_casper(tmp, s8);
        square_casper(s8, tmp);
    }
    square_casper(tmp, s8);
    multiply_casper(s8, tmp, s4);

    // t = s8^(2^32)*n  # ffffffff00000001
    square_casper(tmp, s8);
    for (i = 1; i < 31; i += 2)
    {
        square_casper(t, tmp);
        square_casper(tmp, t);
    }
    square_casper(t, tmp);
    multiply_casper(tmp, t, a);

    // t = t^(2^128)*s8 # ffffffff00000001000000000000000000000000ffffffff
    for (i = 0; i < 128; i += 2)
    {
        square_casper(t, tmp);
        square_casper(tmp, t);
    }
    multiply_casper(t, tmp, s8);

    // t = t^(2^32)*s8  # ffffffff00000001000000000000000000000000ffffffffffffffff
    for (i = 0; i < 32; i += 2)
    {
        square_casper(tmp, t);
        square_casper(t, tmp);
    }
    multiply_casper(tmp, t, s8);

    // t = t^(2^16)*s4  # ffffffff00000001000000000000000000000000ffffffffffffffffffff
    for (i = 0; i < 16; i += 2)
    {
        square_casper(t, tmp);
        square_casper(tmp, t);
    }
    multiply_casper(t, tmp, s4);

    // t = t^(2^8)*s2   # ffffffff00000001000000000000000000000000ffffffffffffffffffffff
    for (i = 0; i < 8; i += 2)
    {
        square_casper(tmp, t);
        square_casper(t, tmp);
    }
    multiply_casper(tmp, t, s2);

    // t = t^(2^4)*s1   # ffffffff00000001000000000000000000000000fffffffffffffffffffffff
    for (i = 0; i < 4; i += 2)
    {
        square_casper(t, tmp);
        square_casper(tmp, t);
    }
    multiply_casper(t, tmp, s1);

    // t = t^(2^2)*t2
    square_casper(tmp, t);
    square_casper(t, tmp);
    multiply_casper(tmp, t, t2);

    // t = t^(2^2)*n    # ffffffff00000001000000000000000000000000fffffffffffffffffffffffd
    square_casper(t, tmp);
    square_casper(tmp, t);
    multiply_casper(c, tmp, a);
}

// A and C do not need to be in Casper memory
static void toMontgomery(uint32_t *C, uint32_t *A)
{
    /* R^2 = 2^512 mod p, used to convert values to Montgomery form. */
    uint32_t R2[NUM_LIMBS] = {0x00000003, 0x00000000, 0xffffffff, 0xfffffffb, 0xfffffffe, 0xffffffff, 0xfffffffd, 0x4};
    uint32_t *T1, *T2, *T3;
    T1 = &CASPER_MEM[ECC_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    T2 = &CASPER_MEM[ECC_SCRATCH_START + 1 * CASPER_NUM_LIMBS];
    T3 = &CASPER_MEM[ECC_SCRATCH_START + 2 * CASPER_NUM_LIMBS];

    CASPER_MEMCPY(T1, R2, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(T2, A, NUM_LIMBS * sizeof(uint32_t));

    multiply_casper(T3, T2, T1);
    CASPER_MEMCPY(C, T3, NUM_LIMBS * sizeof(uint32_t));
}
#endif /* CASPER_ECC_P256 */

/* Compute inversion modulo NIST-p384 using Fermats little theorem.
 * Using c = a^(p-2) = a^(-1) mod p.
 * This computes the modular inversion if all arithmetic is "regular"
 * modular arithmetic or computes automatically the Montgomery inverse
 * if all arithmetic is Montgomery arithmetic.
 */
#if CASPER_ECC_P384
static void invert_mod_p384(uint32_t *c, uint32_t *a)
{
    int i;
    uint32_t *e, *d, *tmp, *t0, *t1, *t2, *t3, *t4, *t5, *t6; // 10 residues needed

    /* Assuming it is safe to use the LUT scratch size.
     * Hence, do not invert while elements in the LUT are needed.
     */
    e = &CASPER_MEM[LUT_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    d = &CASPER_MEM[LUT_SCRATCH_START + 1 * CASPER_NUM_LIMBS];
    tmp = &CASPER_MEM[LUT_SCRATCH_START + 2 * CASPER_NUM_LIMBS];
    t0 = &CASPER_MEM[LUT_SCRATCH_START + 3 * CASPER_NUM_LIMBS];
    t1 = &CASPER_MEM[LUT_SCRATCH_START + 4 * CASPER_NUM_LIMBS];
    t2 = &CASPER_MEM[LUT_SCRATCH_START + 5 * CASPER_NUM_LIMBS];
    t3 = &CASPER_MEM[LUT_SCRATCH_START + 6 * CASPER_NUM_LIMBS];
    t4 = &CASPER_MEM[LUT_SCRATCH_START + 7 * CASPER_NUM_LIMBS];
    t5 = &CASPER_MEM[LUT_SCRATCH_START + 8 * CASPER_NUM_LIMBS];
    t6 = &CASPER_MEM[LUT_SCRATCH_START + 9 * CASPER_NUM_LIMBS];

    square_casper(tmp, a);        // 2
    square_casper(t1, tmp);       // 4
    square_casper(tmp, t1);       // 8
    multiply_casper(t2, tmp, t1); // 12
    multiply_casper(d, a, t2);    // 13
    multiply_casper(e, d, a);     // 14
    multiply_casper(t0, e, a);    // 15

    // t1 = t0^(2^4)*t0     # ff
    square_casper(tmp, t0);
    square_casper(t1, tmp);
    square_casper(tmp, t1);
    square_casper(t2, tmp);
    multiply_casper(t1, t2, t0);

    // t2 = t1^(2^8)*t1   # 4f
    square_casper(tmp, t1);
    for (i = 0; i < 3; i++)
    {
        square_casper(t3, tmp);
        square_casper(tmp, t3);
    }
    square_casper(t3, tmp);
    multiply_casper(t2, t3, t1);

    // t3 = t2^(2^16)*t2  # 8f
    square_casper(tmp, t2);
    for (i = 0; i < 7; i++)
    {
        square_casper(t4, tmp);
        square_casper(tmp, t4);
    }
    square_casper(t4, tmp);
    multiply_casper(t3, t4, t2);

    // t4 = t3^(2^32)*t3  # 16f
    square_casper(tmp, t3);
    for (i = 0; i < 15; i++)
    {
        square_casper(t5, tmp);
        square_casper(tmp, t5);
    }
    square_casper(t5, tmp);
    multiply_casper(t4, t5, t3);

    // t5 = t4^(2^64)*t4  # 32f
    square_casper(tmp, t4);
    for (i = 0; i < 31; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t4);

    // t5 = t5^(2^64)*t4  # 48f
    square_casper(tmp, t5);
    for (i = 0; i < 31; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t4);

    // t5 = t5^(2^32)*t3  # 56f
    square_casper(tmp, t5);
    for (i = 0; i < 15; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t3);

    // t5 = t5^(2^16)*t2  # 60f
    square_casper(tmp, t5);
    for (i = 0; i < 7; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t2);

    // t5 = t5^(2^8)*t1   # 62f
    square_casper(tmp, t5);
    for (i = 0; i < 3; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t1);

    // n = t5^(2^4)*t0     # 63f
    square_casper(tmp, t5);
    for (i = 0; i < 1; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t0);

    // n = n^(2^4)*e
    square_casper(tmp, t5);
    for (i = 0; i < 1; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, e);

    // n = n^(2^32)*t3
    square_casper(tmp, t5);
    for (i = 0; i < 15; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t3);

    // n = n^(2^64)
    square_casper(tmp, t5);
    for (i = 0; i < 31; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t5, tmp);

    // n = n^(2^16)*t2
    square_casper(tmp, t5);
    for (i = 0; i < 7; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t2);

    // n = n^(2^8)*t1
    square_casper(tmp, t5);
    for (i = 0; i < 3; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t1);

    // n = n^(2^4)*t0
    square_casper(tmp, t5);
    for (i = 0; i < 1; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(t5, t6, t0);

    // n = n^(2^4)*d
    square_casper(tmp, t5);
    for (i = 0; i < 1; i++)
    {
        square_casper(t6, tmp);
        square_casper(tmp, t6);
    }
    square_casper(t6, tmp);
    multiply_casper(c, t6, d);
}

// A and C do not need to be in Casper memory
static void toMontgomery(uint32_t *C, uint32_t *A)
{
    /* R^2 = 2^768 mod p, used to convert values to Montgomery form. */
    uint32_t R2[NUM_LIMBS] = {0x00000001, 0xfffffffe, 0x00000000, 0x00000002, 0x00000000, 0xfffffffe,
                              0x00000000, 0x00000002, 0x1,        0x0,        0x0,        0x0};
    uint32_t *T1, *T2, *T3;
    T1 = &CASPER_MEM[ECC_SCRATCH_START + 0 * CASPER_NUM_LIMBS];
    T2 = &CASPER_MEM[ECC_SCRATCH_START + 1 * CASPER_NUM_LIMBS];
    T3 = &CASPER_MEM[ECC_SCRATCH_START + 2 * CASPER_NUM_LIMBS];

    CASPER_MEMCPY(T1, R2, NUM_LIMBS * sizeof(uint32_t));
    CASPER_MEMCPY(T2, A, NUM_LIMBS * sizeof(uint32_t));

    multiply_casper(T3, T2, T1);
    CASPER_MEMCPY(C, T3, NUM_LIMBS * sizeof(uint32_t));
}
#endif /* CASPER_ECC_P384 */
