/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include "psa/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum buffer size for an initial attestation token instance. */
#define ATT_MAX_TOKEN_SIZE (0x240)

/**
 * @brief Gets the public key portion of the attestation service's securely
 *        stored key pair. This public key can be provided to external
 *        verification services for device verification purposes.
 *
 * @return Returns error code as specified in \ref psa_status_t
 */
psa_status_t att_get_pub_key(void);

/**
 * @brief Gets an initial attestation token (IAT) from the TF-M secure
 *        processing environment (SPE). This data will be provided in CBOR
 *        format and is encrypted using the private key held on the SPE.
 *
 * The initial attestation token (IAT) is composed of a series of 'claims' or
 * data points used to uniquely identify this device to an external
 * verification entity (the IAT consumer).
 *
 * The generated IAT should be crytographically verrifiable by the IAT consumer.
 *
 * For details on IAT see https://tools.ietf.org/html/draft-mandyam-eat-01
 *
 * @param ch_buffer     Pointer to the buffer containing the nonce or
 *                      challenge data to be validated with the private key.
 * @param ch_sz         The number of bytes in the challenge. 32, 48 or 64.
 * @param token_buffer  Pointer to the buffer where the IAT will be written.
 *                      Must be equal in size to the system IAT output, which
 *                      can be determined via a call to
 *                      'psa_initial_attest_get_token_size'.
 * @param token_sz      Pointer to the size of token_buffer, this value will be
 *                      updated in this function to contain the number of bytes
 *                      actually retrieved during the IAT request.
 *
 * @return Returns error code as specified in \ref psa_status_t
 */
psa_status_t att_get_iat(uint8_t *ch_buffer, uint32_t ch_sz,
			 uint8_t *token_buffer, uint32_t *token_sz);

/**
 * @brief TODO!
 *
 * @return Returns error code as specified in \ref psa_status_t
 */
psa_status_t att_test(void);

#ifdef __cplusplus
}
#endif
