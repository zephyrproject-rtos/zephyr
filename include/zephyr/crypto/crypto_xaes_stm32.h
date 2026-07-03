/*
 * Copyright (c) 2026 Vossloh AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 AES and secure AES hardware accelerator definitions
 * @ingroup crypto
 *
 * This file contains definitions for the STM32 AES and secure AES hardware
 * accelerators, which are used by the crypto drivers for these peripherals.
 */

#include <zephyr/sys/util.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_saes)

#define STM32_xAES_KEY_SELECTION_Pos  0
#define STM32_xAES_KEY_SELECTION_Msk  (0x7U << STM32_xAES_KEY_SELECTION_Pos)
#define STM32_xAES_KEY_MODE_Pos       3
#define STM32_xAES_KEY_MODE_Msk       (0x3U << STM32_xAES_KEY_MODE_Pos)

/**
 * @name STM32 xAES key descriptor fields
 *
 * @{
 */

/** Key selection for STM32 xAES peripherals */
#define STM32_xAES_KEY_SELECTION_RAW  0 /* Software key, loaded in key registers SAES_KEYx */
#define STM32_xAES_KEY_SELECTION_DHUK 1 /* Derived hardware unique key (DHUK) */
#define STM32_xAES_KEY_SELECTION_BHK  2 /* Boot hardware key (BHK) */
#define STM32_xAES_KEY_SELECTION_XORK 3 /* XOR of DHUK and BHK (XORK) */
/* 4-7: reserved */
#define STM32_xAES_KEY_SELECTION_TEST 7 /* Test mode 256-bit key 0xA5A5...A5A5 */

/** Key mode for STM32 xAES peripherals */
#define STM32_xAES_KEY_MODE_NORMAL  0 /* Normal key */
#define STM32_xAES_KEY_MODE_WRAPPED 1 /* Wrapped key */
#define STM32_xAES_KEY_MODE_SHARED  2 /* Shared key with AES peripheral */
/* 3: reserved */

/**
 * @}
 */

#define STM32_xAES_set_field(field, value)                                                        \
	(((value) << CONCAT(STM32_xAES_, field, _Pos)) & CONCAT(STM32_xAES_, field, _Msk))
#define STM32_xAES_get_field(field, value)                                                        \
	(((value) & CONCAT(STM32_xAES_, field, _Msk)) >> CONCAT(STM32_xAES_, field, _Pos))

#define STM32_xAES_KEYDESC(keysel, keymode)                                                       \
	(STM32_xAES_set_field(KEY_SELECTION, keysel) | STM32_xAES_set_field(KEY_MODE, keymode))

/**
 * @brief Construct a key descriptor for the STM32 xAES peripheral.
 *
 * @param key_selection The key selection to use (e.g. RAW, DHUK, BHK, XORK, TEST).
 * @param key_mode The key mode to use (e.g. NORMAL, WRAPPED, SHARED).
 *
 * @return The constructed key descriptor value.
 */
static inline uint32_t crypto_stm32_xaes_key_desc(uint32_t key_selection, uint32_t key_mode)
{
	return STM32_xAES_KEYDESC(key_selection, key_mode);
}

/**
 * @brief Extract the key selection from a STM32 xAES key descriptor.
 *
 * @param key_desc The key descriptor value to extract from.
 *
 * @return The extracted key selection value.
 */
static inline uint32_t crypto_stm32_xaes_key_selection(uint32_t key_desc)
{
	return STM32_xAES_get_field(KEY_SELECTION, key_desc);
}

/**
 * @brief Extract the key mode from a STM32 xAES key descriptor.
 *
 * @param key_desc The key descriptor value to extract from.
 *
 * @return The extracted key mode value.
 */
static inline uint32_t crypto_stm32_xaes_key_mode(uint32_t key_desc)
{
	return STM32_xAES_get_field(KEY_MODE, key_desc);
}
#endif /* st_stm32_saes */
