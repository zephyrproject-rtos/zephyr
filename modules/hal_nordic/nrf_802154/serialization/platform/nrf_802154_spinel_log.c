/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include <kernel.h>

void nrf_802154_spinel_log(const char *p_fmt, ...)
{
	va_list args;

	va_start(args, p_fmt);
	vprintk(p_fmt, args);
	va_end(args);
}

void nrf_802154_spinel_buff_log(const uint8_t *p_buff, size_t buff_len)
{
	for (size_t i = 0; i < buff_len; i++) {
		if (i != 0) {
			printk(" ");
		}

		printk("0x%02x", p_buff[i]);
	}
}
