/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* MbedTLS custom configuration file for use with prj-trusted-tfm-csr-keygen.conf */

/* Use PSA */
#define MBEDTLS_USE_PSA_CRYPTO

/* X509 read/write and dependencies */
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CSR_WRITE_C
#define MBEDTLS_X509_CREATE_C
#define MBEDTLS_X509_CSR_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_OID_C

/* Private Key support */
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_PK_C