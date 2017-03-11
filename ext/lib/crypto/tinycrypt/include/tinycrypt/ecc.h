/* ecc.h - TinyCrypt interface to ECC auxiliary functions */

/*
 * =============================================================================
 * Copyright (c) 2013, Kenneth MacKay
 * All rights reserved.
 * https://github.com/kmackay/micro-ecc
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 * Copyright (C) 2015 by Intel Corporation, All Rights Reserved.
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

/**
 * @file
 * @brief -- Interface to ECC auxiliary functions.
 *
 *  Overview: This software is an implementation of auxiliary functions
 *            necessary to elliptic curve cryptography. This implementation uses
 *            curve NIST p-256.
 *
 *  Security: The curve NIST p-256 provides approximately 128 bits of security.
 *
 */

#ifndef __TC_ECC_H__
#define __TC_ECC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Word size (4 bytes considering 32-bits architectures) */
#define WORD_SIZE 4
/* Number of words of 32 bits to represent an element of the the curve p-256: */
#define NUM_ECC_DIGITS 8
/* Number of bytes to represent an element of the the curve p-256: */
#define NUM_ECC_BYTES (WORD_SIZE*NUM_ECC_DIGITS)

/* struct to represent a point of the curve (uses X and Y coordinates): */
typedef struct EccPoint {
	uint32_t x[NUM_ECC_DIGITS];
	uint32_t y[NUM_ECC_DIGITS];
} EccPoint;

/* struct to represent a point of the curve in Jacobian coordinates
 * (uses X, Y and Z coordinates):
 */
typedef struct EccPointJacobi {
	uint32_t X[NUM_ECC_DIGITS];
	uint32_t Y[NUM_ECC_DIGITS];
	uint32_t Z[NUM_ECC_DIGITS];
} EccPointJacobi;

/*
 * @brief Check if p_vli is zero.
 * @return returns non-zero if p_vli == 0, zero otherwise.
 *
 * @param p_native OUT -- will be filled in with the native integer value.
 * @param p_bytes IN -- standard octet representation of the integer to convert.
 *
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
uint32_t vli_isZero(uint32_t *p_vli);

/*
 * @brief Set the content of p_src in p_dest.
 *
 * @param p_dest OUT -- Destination buffer.
 * @param p_src IN -- Origin buffer.
 *
 */
void vli_set(uint32_t *p_dest, uint32_t *p_src);

/*
 * @brief Computes the sign of p_left - p_right.
 * @return returns the sign of p_left - p_right.
 *
 * @param p_left IN -- buffer to be compared.
 * @param p_right IN -- buffer to be compared.
 * @param word_size IN -- size of the word.
 *
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
int32_t vli_cmp(uint32_t *p_left, uint32_t *p_right, int32_t word_size);

/*
 * @brief Computes p_result = p_left - p_right, returns borrow.
 * @return returns the sign of p_left - p_right.
 *
 * @param p_result IN -- buffer to be compared.
 * @param p_left IN -- buffer p_left in (p_left - p_right).
 * @param p_right IN -- buffer p_right in (p_left - p_right).
 * @param word_size IN -- size of the word.
 *
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 * @note Can modify in place.
 */
uint32_t vli_sub(uint32_t *p_result, uint32_t *p_left, uint32_t *p_right,
		uint32_t word_size);

/*
 * @brief Conditional set: sets either 'p_true' or 'p_false' to 'output',
 * depending on the value of 'cond'.
 *
 * @param output OUT -- result buffer after setting either p_true or p_false.
 * @param p_true IN -- buffer to be used if cond is true.
 * @param p_false IN -- buffer to be used if cond is false.
 * @param cond IN -- boolean value that will determine which value will be set
 * to output.
 */
void vli_cond_set(uint32_t *output, uint32_t *p_true, uint32_t *p_false,
		uint32_t cond);

/*
 * @brief Computes p_result = (p_left + p_right) % p_mod.
 *
 * @param p_result OUT -- result buffer.
 * @param p_left IN -- buffer p_left in (p_left + p_right) % p_mod.
 * @param p_right IN -- buffer p_right in (p_left + p_right) % p_mod.
 * @param p_mod IN -- module.
 *
 * @note Assumes that p_left < p_mod and p_right < p_mod, p_result != p_mod.
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
void vli_modAdd(uint32_t *p_result, uint32_t *p_left, uint32_t *p_right,
		uint32_t *p_mod);

/*
 * @brief Computes p_result = (p_left - p_right) % p_mod.
 *
 * @param p_result OUT -- result buffer.
 * @param p_left IN -- buffer p_left in (p_left - p_right) % p_mod.
 * @param p_right IN -- buffer p_right in (p_left - p_right) % p_mod.
 * @param p_mod IN -- module.
 *
 * @note Assumes that p_left < p_mod and p_right < p_mod, p_result != p_mod.
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
void vli_modSub(uint32_t *p_result, uint32_t *p_left, uint32_t *p_right,
		uint32_t *p_mod);

/*
 * @brief Computes p_result = (p_left * p_right) % curve_p.
 *
 * @param p_result OUT -- result buffer.
 * @param p_left IN -- buffer p_left in (p_left * p_right) % curve_p.
 * @param p_right IN -- buffer p_right in (p_left * p_right) % curve_p.
 */
void vli_modMult_fast(uint32_t *p_result, uint32_t *p_left,
		uint32_t *p_right);

/*
 * @brief Computes p_result = p_left^2 % curve_p.
 *
 * @param p_result OUT -- result buffer.
 * @param p_left IN -- buffer p_left in (p_left^2 % curve_p).
 */
void vli_modSquare_fast(uint32_t *p_result, uint32_t *p_left);

/*
 * @brief Computes p_result = (p_left * p_right) % p_mod.
 *
 * @param p_result OUT -- result buffer.
 * @param p_left IN -- buffer p_left in (p_left * p_right) % p_mod.
 * @param p_right IN -- buffer p_right in (p_left * p_right) % p_mod.
 * @param p_mod IN -- module.
 * @param p_barrett IN -- used for Barrett reduction.
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
void vli_modMult(uint32_t *p_result, uint32_t *p_left, uint32_t *p_right,
		uint32_t *p_mod, uint32_t *p_barrett);

/*
 * @brief Computes modular inversion: (1/p_intput) % p_mod.
 *
 * @param p_result OUT -- result buffer.
 * @param p_input IN -- buffer p_input in (1/p_intput) % p_mod.
 * @param p_mod IN -- module.
 * @param p_barrett IN -- used for Barrett reduction.
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
void vli_modInv(uint32_t *p_result, uint32_t *p_input,
		uint32_t *p_mod, uint32_t *p_barrett);

/*
 * @brief modular reduction based on Barrett's method
 * @param p_result OUT -- p_product % p_mod.
 * @param p_product IN -- buffer p_product in (p_product % p_mod).
 * @param p_mod IN -- buffer p_mod in (p_product % p_mod).
 * @param p_barrett -- used for Barrett reduction.
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
void vli_mmod_barrett(
    uint32_t *p_result,
    uint32_t *p_product,
    uint32_t *p_mod,
    uint32_t *p_barrett);

/*
 * @brief Check if a point is zero.
 * @return Returns 1 if p_point is the point at infinity, 0 otherwise.
 *
 * @param p_point IN -- point to be checked.
 */
uint32_t EccPoint_isZero(EccPoint *p_point);

/*
 * @brief Check if point in Jacobi coordinates is zero.
 * @return Returns 1 if p_point_jacobi is the point at infinity, 0 otherwise.
 *
 * @param p_point IN -- point to be checked.
 */
uint32_t EccPointJacobi_isZero(EccPointJacobi *p_point_jacobi);

/*
 * @brief Conversion from Jacobi coordinates to Affine coordinates.
 *
 * @param p_point OUT -- point in Affine coordinates.
 * @param p_point_jacobi OUT -- point in Jacobi coordinates.
 */
void EccPoint_toAffine(EccPoint *p_point, EccPointJacobi *p_point_jacobi);

/*
 * @brief Elliptic curve point addition in Jacobi coordinates: P1 = P1 + P2.
 *
 * @param P1 IN/OUT -- P1 in P1 = P1 + P2.
 * @param P2 IN -- P2 in P1 = P1 + P2.
 */
void EccPoint_add(EccPointJacobi *P1, EccPointJacobi *P2);

/*
 * @brief Elliptic curve scalar multiplication with result in Jacobi coordinates
 *
 * @param p_result OUT -- Product of p_point by p_scalar.
 * @param p_point IN -- Elliptic curve point
 * @param p_scalar IN -- Scalar integer
 * @note Side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
void EccPoint_mult_safe(EccPointJacobi *p_result, EccPoint *p_point,
		uint32_t *p_scalar);

/*
 * @brief Fast elliptic curve scalar multiplication with result in Jacobi
 * coordinates
 * @note non constant time
 * @param p_result OUT -- Product of p_point by p_scalar.
 * @param p_point IN -- Elliptic curve point
 * @param p_scalar IN -- Scalar integer
 * @note algorithm NOT strengthened against timing attack.
 */
void EccPoint_mult_unsafe(
    EccPointJacobi *p_result,
    EccPoint *p_point,
    uint32_t *p_scalar);

/*
 * @brief Convert an integer in standard octet representation to native format.
 * @return returns TC_CRYPTO_SUCCESS (1)
 *         returns TC_CRYPTO_FAIL (0) if:
 *                out == NULL or
 *                c == NULL or
 *                ((plen > 0) and (payload == NULL)) or
 *                ((alen > 0) and (associated_data == NULL)) or
 *                (alen >= TC_CCM_AAD_MAX_BYTES) or
 *                (plen >= TC_CCM_PAYLOAD_MAX_BYTES)
 *
 * @param p_native OUT -- will be filled in with the native integer value.
 * @param p_bytes IN -- standard octet representation of the integer to convert.
 *
 */
void ecc_bytes2native(uint32_t p_native[NUM_ECC_DIGITS],
		uint8_t p_bytes[NUM_ECC_DIGITS*4]);


/*
 * @brief Convert an integer in native format to standard octet representation.
 * @return returns TC_CRYPTO_SUCCESS (1)
 *         returns TC_CRYPTO_FAIL (0) if:
 *                out == NULL or
 *                c == NULL or
 *                ((plen > 0) and (payload == NULL)) or
 *                ((alen > 0) and (associated_data == NULL)) or
 *                (alen >= TC_CCM_AAD_MAX_BYTES) or
 *                (plen >= TC_CCM_PAYLOAD_MAX_BYTES)
 *
 * @param p_bytes OUT -- will be filled in with the standard octet
 *                        representation of the integer.
 * @param p_native IN -- native integer value to convert.
 *
 */
void ecc_native2bytes(uint8_t p_bytes[NUM_ECC_DIGITS*4],
		uint32_t p_native[NUM_ECC_DIGITS]);

#ifdef __cplusplus
}
#endif

#endif
