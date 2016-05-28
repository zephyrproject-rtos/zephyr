/* ec_dsa.c - TinyCrypt implementation of EC-DSA */

/*
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

#include <tinycrypt/constants.h>
#include <tinycrypt/ecc.h>

extern uint32_t curve_n[NUM_ECC_DIGITS];
extern EccPoint curve_G;
extern uint32_t curve_nb[NUM_ECC_DIGITS + 1];

int32_t ecdsa_sign(uint32_t r[NUM_ECC_DIGITS], uint32_t s[NUM_ECC_DIGITS],
		   uint32_t p_privateKey[NUM_ECC_DIGITS], uint32_t p_random[NUM_ECC_DIGITS],
		   uint32_t p_hash[NUM_ECC_DIGITS])
{

	uint32_t k[NUM_ECC_DIGITS], tmp[NUM_ECC_DIGITS];
	EccPoint p_point;
	EccPointJacobi P;

	if (vli_isZero(p_random)) {
		return TC_CRYPTO_FAIL; /* The random number must not be 0. */
	}

	vli_set(k, p_random);

	vli_sub(tmp, k, curve_n, NUM_ECC_DIGITS);
	vli_cond_set(k, k, tmp, vli_cmp(curve_n, k, NUM_ECC_DIGITS) == 1);

	/* tmp = k * G */
	EccPoint_mult(&P, &curve_G, k);
	EccPoint_toAffine(&p_point, &P);

	/* r = x1 (mod n) */
	vli_set(r, p_point.x);
	if (vli_cmp(curve_n, r, NUM_ECC_DIGITS) != 1) {
		vli_sub(r, r, curve_n, NUM_ECC_DIGITS);
	}

	if (vli_isZero(r)) {
		return TC_CRYPTO_FAIL; /* If r == 0, fail (need a different random number). */
	}

	vli_modMult(s, r, p_privateKey, curve_n, curve_nb); /* s = r*d */
	vli_modAdd(s, p_hash, s, curve_n); /* s = e + r*d */
	vli_modInv(k, k, curve_n, curve_nb); /* k = 1 / k */
	vli_modMult(s, s, k, curve_n, curve_nb); /* s = (e + r*d) / k */

	return TC_CRYPTO_SUCCESS;
}

int32_t ecdsa_verify(EccPoint *p_publicKey, uint32_t p_hash[NUM_ECC_DIGITS],
		     uint32_t r[NUM_ECC_DIGITS], uint32_t s[NUM_ECC_DIGITS])
{

	uint32_t u1[NUM_ECC_DIGITS], u2[NUM_ECC_DIGITS];
	uint32_t z[NUM_ECC_DIGITS];
	EccPointJacobi P, R;
	EccPoint p_point;

	if (vli_isZero(r) || vli_isZero(s)) {
		return TC_CRYPTO_FAIL; /* r, s must not be 0. */
	}

	if ((vli_cmp(curve_n, r, NUM_ECC_DIGITS) != 1) ||
	   (vli_cmp(curve_n, s, NUM_ECC_DIGITS) != 1)) {
		return TC_CRYPTO_FAIL; /* r, s must be < n. */
	}

	/* Calculate u1 and u2. */
	vli_modInv(z, s, curve_n, curve_nb); /* Z = s^-1 */
	vli_modMult(u1, p_hash, z, curve_n, curve_nb); /* u1 = e/s */
	vli_modMult(u2, r, z, curve_n, curve_nb); /* u2 = r/s */

	/* calculate P = u1*G + u2*Q */
	EccPoint_mult(&P, &curve_G, u1);
	EccPoint_mult(&R, p_publicKey, u2);
	EccPoint_add(&P, &R);
	EccPoint_toAffine(&p_point, &P);

	/* Accept only if P.x == r. */
	vli_cond_set(
		p_point.x,
		p_point.x,
		z,
		vli_sub(z, p_point.x, curve_n, NUM_ECC_DIGITS));

	return (vli_cmp(p_point.x, r, NUM_ECC_DIGITS) == 0);
}
