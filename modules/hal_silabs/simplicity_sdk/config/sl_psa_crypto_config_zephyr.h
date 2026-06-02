/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SL_PSA_CRYPTO_CONFIG_ZEPHYR_H
#define SL_PSA_CRYPTO_CONFIG_ZEPHYR_H

#include <zephyr/devicetree.h>

#include <em_device.h>

/*
 * Configure accelerators according to hardware capabilities.
 * This configuration must align with sli_mbedtls_omnipresent.h
 * from the Simplicity SDK HAL.
 */
#if defined(CONFIG_SOC_FAMILY_SILABS_S2)
#define SLI_MBEDTLS_DEVICE_S2
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(silabs_gecko_semailbox)

#define SLI_MBEDTLS_DEVICE_HSE

#if defined(CONFIG_SOC_SILABS_XG21)
#define SLI_MBEDTLS_DEVICE_SE_V1
#define SLI_MBEDTLS_DEVICE_HSE_V1
#else
#define SLI_MBEDTLS_DEVICE_SE_V2
#define SLI_MBEDTLS_DEVICE_HSE_V2
#endif

#if (_SILICON_LABS_SECURITY_FEATURE == _SILICON_LABS_SECURITY_FEATURE_VAULT)
#define SLI_MBEDTLS_DEVICE_HSE_VAULT_HIGH
#else
#define SLI_MBEDTLS_DEVICE_HSE_VAULT_MID
#endif

#define SL_SE_BUILTIN_KEY_AES128_ALG_CONFIG         (PSA_ALG_CTR)
#define SL_SE_SUPPORT_FW_PRIOR_TO_1_2_2              0
#define SL_SE_ASSUME_FW_AT_LEAST_1_2_2               1
#define SL_SE_ASSUME_FW_UNAFFECTED_BY_ED25519_ERRATA 0

#endif /* DT_HAS_COMPAT_STATUS_OKAY(silabs_gecko_semailbox) */

#include "sli_psa_acceleration.h"
/* Convert definitions to be compatible with TF-PSA-Crypto 1.1 */
#undef MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE

#endif
