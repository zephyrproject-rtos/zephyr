/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 * Copyright (c) 2021 STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef CRYPTO_ACCELERATOR_CONF_H
#define CRYPTO_ACCELERATOR_CONF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************/
/* Require built-in implementations based on PSA requirements */
/****************************************************************/
#ifdef PSA_USE_SE_ST
/* secure element define */
#define PSA_WANT_KEY_TYPE_AES
#ifdef MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_PSA_CRYPTO_SE_C
#define MBEDTLS_CMAC_C
#define MBEDTLS_CIPHER_MODE_CBC
#endif

#ifdef PSA_WANT_ALG_SHA_1
#define MBEDTLS_SHA1_ALT
#endif /* PSA_WANT_ALG_SHA_1 */

#ifdef PSA_WANT_ALG_SHA_256
#define MBEDTLS_SHA256_ALT
#endif /* PSA_WANT_ALG_SHA_256 */

#if defined(PSA_WANT_ALG_RSA_OAEP)			|| \
	defined(PSA_WANT_ALG_RSA_PKCS1V15_CRYPT)	|| \
	defined(PSA_WANT_ALG_RSA_PKCS1V15_SIGN)		|| \
	defined(PSA_WANT_ALG_RSA_PSS)			|| \
	defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)	|| \
	defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY)
#define MBEDTLS_RSA_ALT
#endif

#if defined(PSA_WANT_ALG_ECDH)				|| \
	defined(PSA_WANT_ALG_ECDSA)			|| \
	defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)	|| \
	defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY)
#define MBEDTLS_ECP_ALT
#undef MBEDTLS_ECP_NIST_OPTIM
#endif

#ifdef PSA_WANT_ALG_CCM
#define MBEDTLS_CCM_ALT
#endif /* PSA_WANT_ALG_CCM */

#ifdef PSA_WANT_KEY_TYPE_AES
#define MBEDTLS_AES_ALT
#endif /* PSA_WANT_KEY_TYPE_AES */

#ifdef PSA_WANT_ALG_GCM
#define MBEDTLS_GCM_ALT
#endif /* PSA_WANT_ALG_GCM */

#if defined(PSA_WANT_ALG_ECDSA)				||  \
	defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA)
#define MBEDTLS_ECDSA_VERIFY_ALT
#define MBEDTLS_ECDSA_SIGN_ALT
#endif

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CRYPTO_ACCELERATOR_CONF_H */
