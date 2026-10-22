/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TFM_EXTRA_CRYPTO_CONFIG_H
#define ZEPHYR_TFM_EXTRA_CRYPTO_CONFIG_H

/*
 * profile_medium disables these by default; Zephyr's Bluetooth host requires
 * both for RPA generation (AES-ECB, via the cipher module) and SMP pairing
 * (AES-CMAC). Undef first since profile_medium.h already defines the former.
 */
#undef CRYPTO_CIPHER_MODULE_ENABLED
#define CRYPTO_CIPHER_MODULE_ENABLED 1

#undef PSA_WANT_ALG_CMAC
#define PSA_WANT_ALG_CMAC 1

#endif /* ZEPHYR_TFM_EXTRA_CRYPTO_CONFIG_H */
