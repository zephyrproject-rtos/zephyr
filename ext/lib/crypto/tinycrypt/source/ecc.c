/* ecc.c - TinyCrypt implementation of ECC auxiliary functions */

/*
  *
  * Copyright (c) 2013, Kenneth MacKay
  * All rights reserved.
  * https://github.com/kmackay/micro-ecc
  *
  *  Redistribution and use in source and binary forms, with or without modification,
  *  are permitted provided that the following conditions are met:
  * * Redistributions of source code must retain the above copyright notice, this
  * list of conditions and the following disclaimer.
  * * Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
  * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
  * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  *  Copyright (C) 2015 by Intel Corporation, All Rights Reserved.
  *
  *  Redistribution and use in source and binary forms, with or without
  *  modification, are permitted provided that the following conditions are met:
  *
  *    - Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *
  *    - Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  *
  *    - Neither the name of Intel Corporation nor the names of its contributors
  *    may be used to endorse or promote products derived from this software
  *    without specific prior written permission.
  *
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  */

#include <tinycrypt/ecc.h>

/* ------ Curve NIST P-256 constants: ------ */

#define Curve_P {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,	\
			0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF}

#define Curve_B {0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,	\
			0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8}

#define Curve_N {0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,	\
			0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF}

#define Curve_G {{0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,	\
				0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2}, \
		{0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,	\
				0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2} }

#define Curve_P_Barrett {0x00000003, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFE, \
			0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFF, 0x00000000, 0x00000001}

#define Curve_N_Barrett {0xEEDF9BFE, 0x012FFD85, 0xDF1A6C21, 0x43190552, \
			0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0x00000000, 0x00000001}

const uint32_t curve_p[NUM_ECC_DIGITS] = Curve_P;
const uint32_t curve_b[NUM_ECC_DIGITS] = Curve_B;
const EccPoint curve_G = Curve_G;
const uint32_t curve_n[NUM_ECC_DIGITS] = Curve_N;
const uint32_t curve_pb[NUM_ECC_DIGITS + 1] = Curve_P_Barrett;
const uint32_t curve_nb[NUM_ECC_DIGITS + 1] = Curve_N_Barrett;

/* ------ Static functions: ------ */

/* Zeroing out p_vli. */
static void vli_clear(uint32_t *p_vli)
{
	uint32_t i;

	for (i = 0; i < NUM_ECC_DIGITS; ++i) {
		p_vli[i] = 0;
	}
}

/* Returns nonzero if bit p_bit of p_vli is set.
 * It is assumed that the value provided in 'bit' is within
 * the boundaries of the word-array 'p_vli'.*/
static uint32_t vli_testBit(uint32_t *p_vli, uint32_t p_bit)
{
	return (p_vli[p_bit / 32] & (1 << (p_bit % 32)));
}

uint32_t vli_isZero(uint32_t *p_vli)
{
	uint32_t acc = 0;

	for (uint32_t i = 0; i < NUM_ECC_DIGITS; ++i) {
		acc |= p_vli[i];
	}

	return (!acc);
}

/*
 * Find the right-most nonzero 32-bit "digits" in p_vli.
 *
 * Side-channel countermeasure: algorithm strengthened against timing attack.
 */
static uint32_t vli_numDigits(uint32_t *p_vli)
{
	int32_t i;
	uint32_t digits = 0;

	for (i = NUM_ECC_DIGITS - 1; i >= 0 ; --i) {
		digits += p_vli[i] || digits;
	}

	return digits;
}

/*
 * Find the left-most non-zero bit in p_vli.
 *
 * Side-channel countermeasure: algorithm strengthened against timing attack.
 */
static uint32_t vli_numBits(uint32_t *p_vli)
{
	uint32_t l_digit;
	uint32_t i, acc = 32;
	uint32_t l_numDigits = vli_numDigits(p_vli);

	l_digit = p_vli[l_numDigits - 1];

	for (i = 0; i < 32; ++i) {
		acc -= !l_digit;
		l_digit >>= 1;
	}

	return ((l_numDigits - 1) * 32 + acc);
}

/*
 * Computes p_result = p_left + p_right, returns carry.
 *
 * Side-channel countermeasure: algorithm strengthened against timing attack.
 */
static uint32_t vli_add(uint32_t *p_result, const uint32_t *p_left,
			const uint32_t *p_right)
{

	uint32_t l_carry = 0;

	for (uint32_t i = 0; i < NUM_ECC_DIGITS; ++i) {
		uint32_t l_sum = p_left[i] + p_right[i] + l_carry;

		l_carry = (l_sum < p_left[i]) | ((l_sum == p_left[i]) && l_carry);
		p_result[i] = l_sum;
	}

	return l_carry;
}


/* Computes p_result = p_left * p_right. */
static void vli_mult(uint32_t *p_result, const uint32_t *p_left,
		     const uint32_t *p_right, uint32_t word_size)
{

	uint64_t r01 = 0;
	uint32_t r2 = 0;

	/* Compute each digit of p_result in sequence, maintaining the carries. */
	for (uint32_t k = 0; k < word_size*2 - 1; ++k) {

		uint32_t l_min = (k < word_size ? 0 : (k + 1) - word_size);

		for (uint32_t i = l_min; i <= k && i < word_size; ++i) {

			uint64_t l_product = (uint64_t)p_left[i] * p_right[k - i];

			r01 += l_product;
			r2 += (r01 < l_product);
		}
		p_result[k] = (uint32_t)r01;
		r01 = (r01 >> 32) | (((uint64_t)r2) << 32);
		r2 = 0;
	}

	p_result[word_size * 2 - 1] = (uint32_t)r01;
}

/* Computes p_result = p_left^2. */
static void vli_square(uint32_t *p_result, const uint32_t *p_left)
{

	uint64_t r01 = 0;
	uint32_t r2 = 0;
	uint32_t i, k;

	for (k = 0; k < NUM_ECC_DIGITS * 2 - 1; ++k) {

		uint32_t l_min = (k < NUM_ECC_DIGITS ? 0 : (k + 1) - NUM_ECC_DIGITS);

		for (i = l_min; i <= k && i <= k - i; ++i) {

			uint64_t l_product = (uint64_t)p_left[i] * p_left[k - i];

			if (i < k - i) {

				r2 += l_product >> 63;
				l_product *= 2;
			}
			r01 += l_product;
			r2 += (r01 < l_product);
		}
		p_result[k] = (uint32_t)r01;
		r01 = (r01 >> 32) | (((uint64_t)r2) << 32);
		r2 = 0;
	}

	p_result[NUM_ECC_DIGITS * 2 - 1] = (uint32_t)r01;
}

/* Computes p_result = p_product % curve_p using Barrett reduction. */
void vli_mmod_barrett(uint32_t *p_result, uint32_t *p_product,
			     const uint32_t *p_mod, const uint32_t *p_barrett)
{
	uint32_t i;
	uint32_t q1[NUM_ECC_DIGITS + 1];

	for (i = NUM_ECC_DIGITS - 1; i < 2 * NUM_ECC_DIGITS; i++) {
		q1[i - (NUM_ECC_DIGITS - 1)] = p_product[i];
	}

	uint32_t q2[2*NUM_ECC_DIGITS + 2];

	vli_mult(q2, q1, p_barrett, NUM_ECC_DIGITS + 1);
	for (i = NUM_ECC_DIGITS + 1; i < 2 * NUM_ECC_DIGITS + 2; i++) {
		q1[i - (NUM_ECC_DIGITS + 1)] = q2[i];
	}

	uint32_t prime2[2*NUM_ECC_DIGITS];

	for (i = 0; i < NUM_ECC_DIGITS; i++) {
		prime2[i] = p_mod[i];
		prime2[NUM_ECC_DIGITS + i] = 0;
	}

	vli_mult(q2, q1, prime2, NUM_ECC_DIGITS + 1);
	vli_sub(p_product, p_product, q2, 2 * NUM_ECC_DIGITS);

	uint32_t borrow;

	borrow = vli_sub(q1, p_product, prime2, NUM_ECC_DIGITS + 1);
	vli_cond_set(p_product, p_product, q1, borrow);
	p_product[NUM_ECC_DIGITS] = q1[NUM_ECC_DIGITS] * (!borrow);
	borrow = vli_sub(q1, p_product, prime2, NUM_ECC_DIGITS + 1);
	vli_cond_set(p_product, p_product, q1, borrow);
	p_product[NUM_ECC_DIGITS] = q1[NUM_ECC_DIGITS] * (!borrow);
	borrow = vli_sub(q1, p_product, prime2, NUM_ECC_DIGITS + 1);
	vli_cond_set(p_product, p_product, q1, borrow);
	p_product[NUM_ECC_DIGITS] = q1[NUM_ECC_DIGITS] * (!borrow);

	for (i = 0; i < NUM_ECC_DIGITS; i++) {
		p_result[i] = p_product[i];
	}
}

/*
 * Computes modular exponentiation.
 *
 * Side-channel countermeasure: algorithm strengthened against timing attack.
 */
static void vli_modExp(uint32_t *p_result, const uint32_t *p_base,
		       const uint32_t *p_exp, const uint32_t *p_mod,
		       const uint32_t *p_barrett)
{

	uint32_t acc[NUM_ECC_DIGITS], tmp[NUM_ECC_DIGITS], product[2 * NUM_ECC_DIGITS];
	uint32_t j;
	int32_t i;

	vli_clear(acc);
	acc[0] = 1;

	for (i = NUM_ECC_DIGITS - 1; i >= 0; i--) {
		for (j = 1 << 31; j > 0; j = j >> 1) {
			vli_square(product, acc);
			vli_mmod_barrett(acc, product, p_mod, p_barrett);
			vli_mult(product, acc, p_base, NUM_ECC_DIGITS);
			vli_mmod_barrett(tmp, product, p_mod, p_barrett);
			vli_cond_set(acc, tmp, acc, j & p_exp[i]);
		}
	}

	vli_set(p_result, acc);
}

/* Conversion from Affine coordinates to Jacobi coordinates. */
static void EccPoint_fromAffine(EccPointJacobi *p_point_jacobi,
	EccPoint *p_point) {

	vli_set(p_point_jacobi->X, p_point->x);
	vli_set(p_point_jacobi->Y, p_point->y);
	vli_clear(p_point_jacobi->Z);
	p_point_jacobi->Z[0] = 1;
}

/*
 * Elliptic curve point doubling in Jacobi coordinates: P = P + P.
 *
 * Requires 4 squares and 4 multiplications.
 */
static void EccPoint_double(EccPointJacobi *P)
{

	uint32_t m[NUM_ECC_DIGITS], s[NUM_ECC_DIGITS], t[NUM_ECC_DIGITS];

	vli_modSquare_fast(t, P->Z);
	vli_modSub(m, P->X, t, curve_p);
	vli_modAdd(s, P->X, t, curve_p);
	vli_modMult_fast(m, m, s);
	vli_modAdd(s, m, m, curve_p);
	vli_modAdd(m, s, m, curve_p); /* m = 3X^2 - 3Z^4 */
	vli_modSquare_fast(t, P->Y);
	vli_modMult_fast(s, P->X, t);
	vli_modAdd(s, s, s, curve_p);
	vli_modAdd(s, s, s, curve_p); /* s = 4XY^2 */
	vli_modMult_fast(P->Z, P->Y, P->Z);
	vli_modAdd(P->Z, P->Z, P->Z, curve_p); /* Z' = 2YZ */
	vli_modSquare_fast(P->X, m);
	vli_modSub(P->X, P->X, s, curve_p);
	vli_modSub(P->X, P->X, s, curve_p); /* X' = m^2 - 2s */
	vli_modSquare_fast(P->Y, t);
	vli_modAdd(P->Y, P->Y, P->Y, curve_p);
	vli_modAdd(P->Y, P->Y, P->Y, curve_p);
	vli_modAdd(P->Y, P->Y, P->Y, curve_p);
	vli_modSub(t, s, P->X, curve_p);
	vli_modMult_fast(t, t, m);
	vli_modSub(P->Y, t, P->Y, curve_p); /* Y' = m(s - X') - 8Y^4 */

}

/* Copy input to target. */
static void EccPointJacobi_set(EccPointJacobi *target, EccPointJacobi *input)
{
	vli_set(target->X, input->X);
	vli_set(target->Y, input->Y);
	vli_set(target->Z, input->Z);
}

/* ------ Externally visible functions (see header file for comments): ------ */

void vli_set(uint32_t *p_dest, const uint32_t *p_src)
{

	uint32_t i;

	for (i = 0; i < NUM_ECC_DIGITS; ++i) {
		p_dest[i] = p_src[i];
	}
}

int32_t vli_cmp(const uint32_t *p_left, const uint32_t *p_right, int32_t word_size)
{

	int32_t i, cmp = 0;

	for (i = word_size-1; i >= 0; --i) {
		cmp |= ((p_left[i] > p_right[i]) - (p_left[i] < p_right[i])) * (!cmp);
	}

	return cmp;
}

uint32_t vli_sub(uint32_t *p_result, const uint32_t *p_left,
		 const uint32_t *p_right, uint32_t word_size)
{

	uint32_t l_borrow = 0;

	for (uint32_t i = 0; i < word_size; ++i) {
		uint32_t l_diff = p_left[i] - p_right[i] - l_borrow;

		l_borrow = (l_diff > p_left[i]) | ((l_diff == p_left[i]) && l_borrow);
		p_result[i] = l_diff;
	}

	return l_borrow;
}

void vli_cond_set(uint32_t *output, uint32_t *p_true, uint32_t *p_false,
	uint32_t cond)
{
	uint32_t i;

	cond = (!cond);

	for (i = 0; i < NUM_ECC_DIGITS; i++) {
		output[i] = (p_true[i]*(!cond)) | (p_false[i]*cond);
	}
}

void vli_modAdd(uint32_t *p_result, const uint32_t *p_left, const uint32_t *p_right,
	const uint32_t *p_mod)
{
	uint32_t l_carry = vli_add(p_result, p_left, p_right);
	uint32_t p_temp[NUM_ECC_DIGITS];

	l_carry = l_carry == vli_sub(p_temp, p_result, p_mod, NUM_ECC_DIGITS);
	vli_cond_set(p_result, p_temp, p_result, l_carry);
}

void vli_modSub(uint32_t *p_result, const uint32_t *p_left, const uint32_t *p_right,
	const uint32_t *p_mod)
{
	uint32_t l_borrow = vli_sub(p_result, p_left, p_right, NUM_ECC_DIGITS);
	uint32_t p_temp[NUM_ECC_DIGITS];

	vli_add(p_temp, p_result, p_mod);
	vli_cond_set(p_result, p_temp, p_result, l_borrow);
}

void vli_modMult_fast(uint32_t *p_result, const uint32_t *p_left,
	const uint32_t *p_right)
{
	uint32_t l_product[2 * NUM_ECC_DIGITS];

	vli_mult(l_product, p_left, p_right, NUM_ECC_DIGITS);
	vli_mmod_barrett(p_result, l_product, curve_p, curve_pb);
}

void vli_modSquare_fast(uint32_t *p_result, const uint32_t *p_left)
{
	uint32_t l_product[2 * NUM_ECC_DIGITS];

	vli_square(l_product, p_left);
	vli_mmod_barrett(p_result, l_product, curve_p, curve_pb);
}

void vli_modMult(uint32_t *p_result, const uint32_t *p_left, const uint32_t *p_right,
		 const uint32_t *p_mod, uint32_t *p_barrett)
{

	uint32_t l_product[2 * NUM_ECC_DIGITS];

	vli_mult(l_product, p_left, p_right, NUM_ECC_DIGITS);
	vli_mmod_barrett(p_result, l_product, p_mod, p_barrett);
}

void vli_modInv(uint32_t *p_result, const uint32_t *p_input, const uint32_t *p_mod,
	const uint32_t *p_barrett)
{
	uint32_t p_power[NUM_ECC_DIGITS];

	vli_set(p_power, p_mod);
	p_power[0] -= 2;
	vli_modExp(p_result, p_input, p_power, p_mod, p_barrett);
}

uint32_t EccPoint_isZero(EccPoint *p_point)
{
	return (vli_isZero(p_point->x) && vli_isZero(p_point->y));
}

uint32_t EccPointJacobi_isZero(EccPointJacobi *p_point_jacobi)
{
	return vli_isZero(p_point_jacobi->Z);
}

void EccPoint_toAffine(EccPoint *p_point, EccPointJacobi *p_point_jacobi)
{

	if (vli_isZero(p_point_jacobi->Z)) {
		vli_clear(p_point->x);
		vli_clear(p_point->y);
		return;
	}

	uint32_t z[NUM_ECC_DIGITS];

	vli_set(z, p_point_jacobi->Z);
	vli_modInv(z, z, curve_p, curve_pb);
	vli_modSquare_fast(p_point->x, z);
	vli_modMult_fast(p_point->y, p_point->x, z);
	vli_modMult_fast(p_point->x, p_point->x, p_point_jacobi->X);
	vli_modMult_fast(p_point->y, p_point->y, p_point_jacobi->Y);
}

void EccPoint_add(EccPointJacobi *P1, EccPointJacobi *P2)
{

	uint32_t s1[NUM_ECC_DIGITS], u1[NUM_ECC_DIGITS], t[NUM_ECC_DIGITS];
	uint32_t h[NUM_ECC_DIGITS], r[NUM_ECC_DIGITS];

	vli_modSquare_fast(r, P1->Z);
	vli_modSquare_fast(s1, P2->Z);
	vli_modMult_fast(u1, P1->X, s1); /* u1 = X1 Z2^2 */
	vli_modMult_fast(h, P2->X, r);
	vli_modMult_fast(s1, P1->Y, s1);
	vli_modMult_fast(s1, s1, P2->Z); /* s1 = Y1 Z2^3 */
	vli_modMult_fast(r, P2->Y, r);
	vli_modMult_fast(r, r, P1->Z);
	vli_modSub(h, h, u1, curve_p); /* h = X2 Z1^2 - u1 */
	vli_modSub(r, r, s1, curve_p); /* r = Y2 Z1^3 - s1 */

	if (vli_isZero(h)) {
		if (vli_isZero(r)) {
			/* P1 = P2 */
			EccPoint_double(P1);
			return;
		}
		/* point at infinity */
		vli_clear(P1->Z);
		return;
	}

	vli_modMult_fast(P1->Z, P1->Z, P2->Z);
	vli_modMult_fast(P1->Z, P1->Z, h); /* Z3 = h Z1 Z2 */
	vli_modSquare_fast(t, h);
	vli_modMult_fast(h, t, h);
	vli_modMult_fast(u1, u1, t);
	vli_modSquare_fast(P1->X, r);
	vli_modSub(P1->X, P1->X, h, curve_p);
	vli_modSub(P1->X, P1->X, u1, curve_p);
	vli_modSub(P1->X, P1->X, u1, curve_p); /* X3 = r^2 - h^3 - 2 u1 h^2 */
	vli_modMult_fast(t, s1, h);
	vli_modSub(P1->Y, u1, P1->X, curve_p);
	vli_modMult_fast(P1->Y, P1->Y, r);
	vli_modSub(P1->Y, P1->Y, t, curve_p); /* Y3 = r(u1 h^2 - X3) - s1 h^3 */
}

/*
 * Elliptic curve scalar multiplication with result in Jacobi coordinates:
 *
 * p_result = p_scalar * p_point.
 */
void EccPoint_mult_safe(EccPointJacobi *p_result, EccPoint *p_point, uint32_t *p_scalar)
{

	int32_t i;
	uint32_t bit;
	EccPointJacobi p_point_jacobi, p_tmp;

	EccPoint_fromAffine(p_result, p_point);
	EccPoint_fromAffine(&p_point_jacobi, p_point);

	for (i = vli_numBits(p_scalar) - 2; i >= 0; i--) {
		EccPoint_double(p_result);
		EccPointJacobi_set(&p_tmp, p_result);
		EccPoint_add(&p_tmp, &p_point_jacobi);
		bit = vli_testBit(p_scalar, i);
		vli_cond_set(p_result->X, p_tmp.X, p_result->X, bit);
		vli_cond_set(p_result->Y, p_tmp.Y, p_result->Y, bit);
		vli_cond_set(p_result->Z, p_tmp.Z, p_result->Z, bit);
	}
}

/* Ellptic curve scalar multiplication with result in Jacobi coordinates */
/* p_result = p_scalar * p_point */
void EccPoint_mult_unsafe(EccPointJacobi *p_result, EccPoint *p_point, uint32_t *p_scalar)
{
  int i;
  EccPointJacobi p_point_jacobi;
  EccPoint_fromAffine(p_result, p_point);
  EccPoint_fromAffine(&p_point_jacobi, p_point);

  for(i = vli_numBits(p_scalar) - 2; i >= 0; i--)
  {
    EccPoint_double(p_result);
    if (vli_testBit(p_scalar, i))
    {
      EccPoint_add(p_result, &p_point_jacobi);
    }
  }
}

/* -------- Conversions between big endian and little endian: -------- */

void ecc_bytes2native(uint32_t p_native[NUM_ECC_DIGITS],
		      uint8_t p_bytes[NUM_ECC_DIGITS * 4])
{

	uint32_t i;

	for (i = 0; i < NUM_ECC_DIGITS; ++i) {
		uint8_t *p_digit = p_bytes + 4 * (NUM_ECC_DIGITS - 1 - i);

		p_native[i] = ((uint32_t)p_digit[0] << 24) |
			((uint32_t)p_digit[1] << 16) |
			((uint32_t)p_digit[2] << 8) |
			(uint32_t)p_digit[3];
	}
}

void ecc_native2bytes(uint8_t p_bytes[NUM_ECC_DIGITS * 4],
	uint32_t p_native[NUM_ECC_DIGITS])
{

	uint32_t i;

	for (i = 0; i < NUM_ECC_DIGITS; ++i) {
		uint8_t *p_digit = p_bytes + 4 * (NUM_ECC_DIGITS - 1 - i);

		p_digit[0] = p_native[i] >> 24;
		p_digit[1] = p_native[i] >> 16;
		p_digit[2] = p_native[i] >> 8;
		p_digit[3] = p_native[i];
	}
}

