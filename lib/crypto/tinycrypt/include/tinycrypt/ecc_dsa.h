/* ecc_dh.h - TinyCrypt interface to EC-DSA implementation */

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
 * @brief -- Interface to EC-DSA implementation.
 *
 *  Overview: This software is an implementation of EC-DSA. This implementation
 *            uses curve NIST p-256.
 *
 *  Security: The curve NIST p-256 provides approximately 128 bits of security.
 *
 *  Usage:  - To sign: Compute a hash of the data you wish to sign (SHA-2 is
 *          recommended) and pass it in to ecdsa_sign function along with your
 *          private key and a random number. You must use a new non-predictable
 *          random number to generate each new signature.
 *          - To verify a signature: Compute the hash of the signed data using
 *          the same hash as the signer and pass it to this function along with
 *          the signer's public key and the signature values (r and s).
 */

#ifndef __TC_ECC_DSA_H__
#define __TC_ECC_DSA_H__

#include <tinycrypt/ecc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate an ECDSA signature for a given hash value.
 * @return returns TC_SUCCESS (1) if the the signature generated successfully
 *         returns TC_FAIL (0) if:
 *                r == 0 or
 *                p_random == 0
 *
 * @param r OUT -- to be filled with the signature values.
 * @param s OUT -- to be filled with the signature values.
 * @param p_privateKey IN -- Your private key.
 * @param p_random IN -- The random number to use in generating ephemeral DSA
 * keys.
 * @param p_hash IN -- The message hash to sign.
 *
 * @note p_random must have NUM_ECC_DIGITS*2 bits of entropy to eliminate
 * bias in keys.
 *
 * @note side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
int32_t ecdsa_sign(uint32_t r[NUM_ECC_DIGITS], uint32_t s[NUM_ECC_DIGITS],
		   uint32_t p_privateKey[NUM_ECC_DIGITS], uint32_t p_random[NUM_ECC_DIGITS * 2],
		   uint32_t p_hash[NUM_ECC_DIGITS]);


/**
 * @brief Verify an ECDSA signature.
 * @return returns TC_SUCCESS (1) if the the signature generated successfully
 *         returns TC_FAIL (0) if:
 *                r == 0 or
 *                p_random == 0
 *
 * @param p_publicKey IN -- The signer's public key.
 * @param p_hash IN -- The hash of the signed data.
 * @param r IN -- The signature values.
 * @param s IN -- The signature values.
 *
 * @note side-channel countermeasure: algorithm strengthened against timing
 * attack.
 */
int32_t ecdsa_verify(EccPoint *p_publicKey, uint32_t p_hash[NUM_ECC_DIGITS],
		     uint32_t r[NUM_ECC_DIGITS], uint32_t s[NUM_ECC_DIGITS]);

#ifdef __cplusplus
}
#endif

#endif
