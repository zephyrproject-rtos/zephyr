/*
 * Copyright (c) 2026 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_CYW43_DRV_H
#define ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_CYW43_DRV_H
#include <zephyr/kernel.h>

struct async_context {
	struct k_mutex mutex;
	struct k_sem bh_sem;
};

extern struct async_context async_ctx;

void cyw43_hal_generate_laa_mac(int idx, uint8_t buf[6]);
void cyw43_post_poll_hook(void);

#endif /* ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_CYW43_DRV_H */
