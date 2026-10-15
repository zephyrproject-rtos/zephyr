/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SL_PSA_CRYPTO_CONFIG_ZEPHYR_H
#define SL_PSA_CRYPTO_CONFIG_ZEPHYR_H

#include <em_device.h>

/*
 * Configure accelerators according to hardware capabilities.
 * This configuration must align with sli_mbedtls_omnipresent.h
 * from the Simplicity SDK HAL.
 */
#if defined(CONFIG_SOC_FAMILY_SILABS_S2)
#define SLI_MBEDTLS_DEVICE_S2
#endif

#if defined(CONFIG_SOC_FAMILY_SILABS_S3)
#define SLI_MBEDTLS_DEVICE_S3

#if defined(CONFIG_SOC_FAMILY_SILABS_S3_SIX301)
#define SLI_MBEDTLS_DEVICE_HC_LPW
#endif
#endif

#if defined(CONFIG_SILABS_SISDK_PSA_CRYPTO_HOST)
#define SLI_MBEDTLS_DEVICE_HC
#endif

#if defined(CONFIG_DT_HAS_SILABS_GECKO_SEMAILBOX_ENABLED)
#define SLI_MBEDTLS_DEVICE_HSE

#if defined(CONFIG_SOC_FAMILY_SILABS_S2_XG21)
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

#define SL_SE_BUILTIN_KEY_AES128_ALG_CONFIG          (PSA_ALG_CTR)
/* SE FW 1.2.2 for xG21 was released in July 2020. Assume that any device running Zephyr is newer
 * than this and doesn't need the workaround.
 */
#define SL_SE_SUPPORT_FW_PRIOR_TO_1_2_2              0
#define SL_SE_ASSUME_FW_AT_LEAST_1_2_2               1
#define SL_SE_ASSUME_FW_UNAFFECTED_BY_ED25519_ERRATA 0

#endif /* CONFIG_DT_HAS_SILABS_GECKO_SEMAILBOX_ENABLED */

#if defined(CONFIG_DT_HAS_SILABS_GECKO_TRNG_ENABLED)
#define SLI_MBEDTLS_DEVICE_VSE

#if defined(CONFIG_SOC_FAMILY_SILABS_S2_XG22)
#define SLI_MBEDTLS_DEVICE_SE_V1
#define SLI_MBEDTLS_DEVICE_VSE_V1
#else
#define SLI_MBEDTLS_DEVICE_SE_V2
#define SLI_MBEDTLS_DEVICE_VSE_V2
#endif

#endif /* CONFIG_DT_HAS_SILABS_GECKO_TRNG_ENABLED */

#include "sli_psa_acceleration.h"

#endif
