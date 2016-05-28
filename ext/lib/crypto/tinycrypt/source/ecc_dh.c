/* ec_dh.c - TinyCrypt implementation of EC-DH */

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

extern uint32_t curve_p[NUM_ECC_DIGITS];
extern uint32_t curve_b[NUM_ECC_DIGITS];
extern uint32_t curve_n[NUM_ECC_DIGITS];
extern EccPoint curve_G;

int32_t ecc_make_key(EccPoint *p_publicKey, uint32_t p_privateKey[NUM_ECC_DIGITS],
		     uint32_t p_random[NUM_ECC_DIGITS])
{

	/* Make sure the private key is in the range [1, n-1].
	 * For the supported curve, n is always large enough
	 * that we only need to subtract once at most.
	 */
	uint32_t p_tmp[NUM_ECC_DIGITS];

	vli_set(p_privateKey, p_random);
	vli_sub(p_tmp, p_privateKey, curve_n, NUM_ECC_DIGITS);

	vli_cond_set(p_privateKey, p_privateKey, p_tmp,
		     vli_cmp(curve_n, p_privateKey, NUM_ECC_DIGITS) == 1);

	if (vli_isZero(p_privateKey)) {
		return TC_CRYPTO_FAIL; /* The private key cannot be 0 (mod p). */
	}

	EccPointJacobi P;

	EccPoint_mult(&P, &curve_G, p_privateKey);
	EccPoint_toAffine(p_publicKey, &P);

	return TC_CRYPTO_SUCCESS;
}

/* Compute p_result = x^3 - 3x + b */
static void curve_x_side(uint32_t p_result[NUM_ECC_DIGITS],
			 uint32_t x[NUM_ECC_DIGITS])
{

	uint32_t _3[NUM_ECC_DIGITS] = {3}; /* -a = 3 */

	vli_modSquare_fast(p_result, x); /* r = x^2 */
	vli_modSub(p_result, p_result, _3, curve_p); /* r = x^2 - 3 */
	vli_modMult_fast(p_result, p_result, x); /* r = x^3 - 3x */
	vli_modAdd(p_result, p_result, curve_b, curve_p); /* r = x^3 - 3x + b */

}

int32_t ecc_valid_public_key(EccPoint *p_publicKey)
{
	uint32_t l_tmp1[NUM_ECC_DIGITS];
	uint32_t l_tmp2[NUM_ECC_DIGITS];

	if (EccPoint_isZero(p_publicKey)) {
		return -1;
	}

	if ((vli_cmp(curve_p, p_publicKey->x, NUM_ECC_DIGITS) != 1) ||
	   (vli_cmp(curve_p, p_publicKey->y, NUM_ECC_DIGITS) != 1)) {
		return -2;
	}

	vli_modSquare_fast(l_tmp1, p_publicKey->y); /* tmp1 = y^2 */

	curve_x_side(l_tmp2, p_publicKey->x); /* tmp2 = x^3 - 3x + b */

	/* Make sure that y^2 == x^3 + ax + b */
	if (vli_cmp(l_tmp1, l_tmp2, NUM_ECC_DIGITS) != 0) {
		return -3;
	}

	return 0;
}

int32_t ecdh_shared_secret(uint32_t p_secret[NUM_ECC_DIGITS],
			   EccPoint *p_publicKey, uint32_t p_privateKey[NUM_ECC_DIGITS])
{

	EccPoint p_point;
	EccPointJacobi P;

	EccPoint_mult(&P, p_publicKey, p_privateKey);
	if (EccPointJacobi_isZero(&P)) {
		return TC_CRYPTO_FAIL;
	}
	EccPoint_toAffine(&p_point, &P);
	vli_set(p_secret, p_point.x);

	return TC_CRYPTO_SUCCESS;
}
