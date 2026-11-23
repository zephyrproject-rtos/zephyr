/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file osif_zephyr.h
 * @brief Public interface for initializing the OSIF Zephyr patch.
 *
 */

#ifndef _OSIF_ZEPHYR_H_
#define _OSIF_ZEPHYR_H_

/**
 * @brief Initialize all OSIF function pointers and heap.
 * @note Must be called by chip-specific initialization code.
 */
void os_zephyr_patch_init(void);

#endif
