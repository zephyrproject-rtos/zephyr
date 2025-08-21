/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 * Copyright (c) 2021 STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef MBEDTLS_ACCELERATOR_CONF_H
#define MBEDTLS_ACCELERATOR_CONF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* RNG Config */
#undef MBEDTLS_ENTROPY_NV_SEED
#undef MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#undef MBEDTLS_AES_SETKEY_DEC_ALT
#undef MBEDTLS_AES_DECRYPT_ALT

/* specific Define for platform hardware accelerator */
#define GENERATOR_HW_PKA_EXTENDED_API
#define GENERATOR_HW_CRYPTO_DPA_SUPPORTED
#define HW_CRYPTO_DPA_AES
#define HW_CRYPTO_DPA_GCM

/****************************************************************/
/* Infer PSA requirements from Mbed TLS capabilities */
/****************************************************************/
#ifndef MBEDTLS_PSA_CRYPTO_CONFIG

#ifdef MBEDTLS_SHA1_C
#define MBEDTLS_SHA1_ALT
#endif /* MBEDTLS_SHA1_C */

#ifdef MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_ALT
#endif /* MBEDTLS_SHA256_C */

#ifdef MBEDTLS_RSA_C
#define MBEDTLS_RSA_ALT
#endif /* MBEDTLS_RSA_C */

#if defined(MBEDTLS_ECP_C)
#define MBEDTLS_ECP_ALT
#undef MBEDTLS_ECP_NIST_OPTIM
/*#define MBEDTLS_MD5_ALT*/
#endif /* MBEDTLS_ECP_C && MBEDTLS_MD_C */

#ifdef MBEDTLS_CCM_C
#define MBEDTLS_CCM_ALT
#endif /* MBEDTLS_CCM_C */

#ifdef MBEDTLS_AES_C
#define MBEDTLS_AES_ALT
#endif /* MBEDTLS_AES_C */

#ifdef MBEDTLS_GCM_C
#define MBEDTLS_GCM_ALT
#endif /* MBEDTLS_GCM_C */

#ifdef MBEDTLS_ECDSA_C
#define MBEDTLS_ECDSA_VERIFY_ALT
#define MBEDTLS_ECDSA_SIGN_ALT
#endif /* MBEDTLS_ECDSA_C */

/* secure element define */
#ifdef MBEDTLS_PSA_CRYPTO_C
#ifdef PSA_USE_SE_ST
#define MBEDTLS_PSA_CRYPTO_SE_C
#define MBEDTLS_CMAC_C
#define MBEDTLS_CIPHER_MODE_CBC
#endif
#endif

#endif /* MBEDTLS_PSA_CRYPTO_CONFIG */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MBEDTLS_ACCELERATOR_CONF_H */
