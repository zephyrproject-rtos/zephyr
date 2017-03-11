/* ecc_dh.h - TinyCrypt interface to EC-DH implementation */

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
 * @brief -- Interface to EC-DH implementation.
 *
 *  Overview: This software is an implementation of EC-DH. This implementation
 *            uses curve NIST p-256.
 *
 *  Security: The curve NIST p-256 provides approximately 128 bits of security.
 *
 */

#ifndef __TC_ECC_DH_H__
#define __TC_ECC_DH_H__

#include <tinycrypt/ecc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a public/private key pair.
 * @return returns TC_CRYPTO_SUCCESS (1) if key pair was generated successfully
 *         returns TC_CRYPTO_FAIL (0) if:
 *                the private key is 0
 *
 * @param p_publicKey OUT -- the point representing the public key.
 * @param p_privateKey OUT -- the private key.
 * @param p_random IN -- The random number to use to generate the key pair.
 *
 * @note You must use a new non-predictable random number to generate each
 * new key pair.
 * @note p_random must have NUM_ECC_DIGITS*2 bits of entropy to eliminate
 * bias in keys.
 *
 * @note side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
int32_t ecc_make_key(EccPoint *p_publicKey,
		     uint32_t p_privateKey[NUM_ECC_DIGITS],
		     uint32_t p_random[2 * NUM_ECC_DIGITS]);

/**
 * @brief Determine whether or not a given point is on the chosen elliptic curve
 * (ie, is a valid public key).
 * @return returns 0 if the given point is valid
 *         returns -1 if: the point is zero
 *         returns -2 if:  curve_p - p_publicKey->x != 1 or
 *                            curve_p - p_publicKey->y != 1
 *         returns -3 if: y^2 != x^3 + ax + b
 *         returns -4 if: public key is the group generator
 *
 * @param p_publicKey IN -- The point to be checked.
 */
int32_t ecc_valid_public_key(EccPoint *p_publicKey);

/**
 * @brief Compute a shared secret given your secret key and someone else's
 * public key.
 * @return returns TC_CRYPTO_SUCCESS (1) if the secret was computed successfully
 *         returns TC_CRYPTO_FAIL (0) otherwise
 *
 * @param p_secret OUT -- The shared secret value.
 * @param p_publicKey IN -- The public key of the remote party.
 * @param p_privateKey IN -- Your private key.
 *
 * @note Optionally, you can provide a random multiplier for resistance to DPA
 * attacks. The random multiplier should probably be different for each
 * invocation of ecdh_shared_secret().
 *
 * @warning It is recommended to use the output of ecdh_shared_secret() as the
 * input of a recommended Key Derivation Function (see NIST SP 800-108) in
 * order to produce a symmetric key.
 */
int32_t ecdh_shared_secret(uint32_t p_secret[NUM_ECC_DIGITS], EccPoint *p_publicKey,
			   uint32_t p_privateKey[NUM_ECC_DIGITS]);

#ifdef __cplusplus
}
#endif

#endif
