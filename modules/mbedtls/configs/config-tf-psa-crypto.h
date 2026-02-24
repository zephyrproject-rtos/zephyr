/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TF_PSA_CRYPTO_CONFIG_H
#define TF_PSA_CRYPTO_CONFIG_H

/* [TO BE IMPROVED]
 * This is a dirty fix to overcome a pre-existing problem: TLS/X509 build
 * symbols influence crypto ones. This not ideal even before the split, but
 * at least it was working. Now that we split the configuration headers in
 * 2 we need to include the TLS/X509 configuration header file here, which
 * is not good.
 * Of course the solution is to:
 * - have 1:1 mapping between Kconfigs and build symbols
 * - let each Kconfig enable ONLY 1 build symbol
 * - resolve all the dependencies at Kconfig level instead of doing that in the
 *   configuration header file.
 */
#include "config-mbedtls.h"

/*
 * "config-psa.h" contains all the Kconfig -> build symbols matching for
 * the "PSA_WANT_xxx", whereas "config-tf-psa-crypto.h" contains the same
 * mapping for the "MBEDTLS_xxx" stuff. However tf-psa-crypto wants a single
 * file with all the configurations in it, so let the latter include
 * the former.
 */
#include "config-psa.h"

#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_MEMORY_ALIGN_MULTIPLE (sizeof(void *))
#define MBEDTLS_PLATFORM_EXIT_ALT

#if defined(CONFIG_MBEDTLS_ZEROIZE_ALT)
#define MBEDTLS_PLATFORM_ZEROIZE_ALT
#endif

#if defined(CONFIG_MBEDTLS_PLATFORM_NO_STD_FUNCTIONS)
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#endif /* defined(MBEDTLS_PLATFORM_NO_STD_FUNCTIONS) */

#if defined(CONFIG_MBEDTLS_PLATFORM_SNPRINTF_ALT)
#define MBEDTLS_PLATFORM_SNPRINTF_ALT
#endif /* defined(MBEDTLS_PLATFORM_SNPRINTF_ALT) */

#if defined(CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY)
#define MBEDTLS_PSA_DRIVER_GET_ENTROPY
#endif

#if defined(CONFIG_MBEDTLS_HAVE_ASM)
#define MBEDTLS_HAVE_ASM
#endif

#if defined(CONFIG_MBEDTLS_LMS_C)
#define MBEDTLS_LMS_C
#endif

#if defined(CONFIG_MBEDTLS_HAVE_TIME_DATE)
#define MBEDTLS_HAVE_TIME
#define MBEDTLS_HAVE_TIME_DATE
#define MBEDTLS_PLATFORM_MS_TIME_ALT
#endif

#if defined(CONFIG_MBEDTLS_AES_ROM_TABLES)
#define MBEDTLS_AES_ROM_TABLES
#endif

#if defined(CONFIG_MBEDTLS_AES_FEWER_TABLES)
#define MBEDTLS_AES_FEWER_TABLES
#endif

#if defined(CONFIG_MBEDTLS_ECP_NIST_OPTIM)
#define MBEDTLS_ECP_NIST_OPTIM
#endif

#if defined(CONFIG_MBEDTLS_SHA256_SMALLER)
#define MBEDTLS_SHA256_SMALLER
#endif

#if defined(CONFIG_MBEDTLS_CTR_DRBG_C)
#define MBEDTLS_CTR_DRBG_C
#endif

#if defined(CONFIG_MBEDTLS_HMAC_DRBG_C)
#define MBEDTLS_HMAC_DRBG_C
#endif

#if defined(CONFIG_MBEDTLS_MEMORY_DEBUG)
#define MBEDTLS_MEMORY_DEBUG
#endif

#if defined(CONFIG_MBEDTLS_MD_C)
#define MBEDTLS_MD_C
#endif

#if defined(CONFIG_MBEDTLS_BASE64_C)
#define MBEDTLS_BASE64_C
#endif

#if defined(CONFIG_MBEDTLS_PEM_PARSE_C)
#define MBEDTLS_PEM_PARSE_C
#endif

#if defined(CONFIG_MBEDTLS_PEM_WRITE_C)
#define MBEDTLS_PEM_WRITE_C
#endif

#if defined(MBEDTLS_X509_USE_C)
#define MBEDTLS_PK_PARSE_C
#endif

#if defined(CONFIG_MBEDTLS_PK_WRITE_C)
#define MBEDTLS_PK_WRITE_C
#endif

#if defined(MBEDTLS_PK_PARSE_C) || defined(MBEDTLS_PK_WRITE_C)
#define MBEDTLS_PK_C
#endif

#if defined(CONFIG_MBEDTLS_ASN1_PARSE_C) || defined(MBEDTLS_X509_USE_C)
#define MBEDTLS_ASN1_PARSE_C
#endif

#if defined(MBEDTLS_ECDSA_C) || defined(MBEDTLS_RSA_C) || defined(MBEDTLS_PK_WRITE_C)
#define MBEDTLS_ASN1_WRITE_C
#endif

#if defined(CONFIG_MBEDTLS_PKCS5_C)
#define MBEDTLS_PKCS5_C
#endif

#if defined(CONFIG_MBEDTLS_OPENTHREAD_OPTIMIZATIONS_ENABLED)
#define MBEDTLS_MPI_WINDOW_SIZE            1 /**< Maximum windows size used. */
#define MBEDTLS_MPI_MAX_SIZE              32 /**< Maximum number of bytes for usable MPIs. */
#define MBEDTLS_ECP_WINDOW_SIZE            2 /**< Maximum window size used */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM      0 /**< Enable fixed-point speed-up */
#endif

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG
#endif

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_C)
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_PSA_ASSUME_EXCLUSIVE_BUFFERS
#endif

#if defined(CONFIG_MBEDTLS_PSA_P256M_DRIVER_ENABLED)
#define MBEDTLS_PSA_P256M_DRIVER_ENABLED
#endif

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_STORAGE_C)
#define MBEDTLS_PSA_CRYPTO_STORAGE_C
#endif

#if defined(CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOTS)
#define MBEDTLS_PSA_STATIC_KEY_SLOTS
#endif

#if defined(CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT)
#define MBEDTLS_PSA_KEY_SLOT_COUNT CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT
#endif

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
#define MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS
#endif

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT)
#define MBEDTLS_PSA_CRYPTO_CLIENT
#define MBEDTLS_PSA_CRYPTO_CONFIG
#endif

#if defined(CONFIG_MBEDTLS_NIST_KW_C)
#define MBEDTLS_NIST_KW_C
#endif

#if defined(CONFIG_MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS)
#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#endif

#endif /* TF_PSA_CRYPTO_CONFIG_H */
