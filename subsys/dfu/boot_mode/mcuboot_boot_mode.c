/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>

extern int soc_mcuboot_mode_set(uint8_t mode);
extern uint8_t  soc_mcuboot_mode_get(void);

int boot_mode_set(uint8_t mode)
{
	return soc_mcuboot_mode_set(mode);
}

uint8_t boot_mode_get(void)
{
	return soc_mcuboot_mode_get();
}
