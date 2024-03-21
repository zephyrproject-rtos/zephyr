/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_
#define ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_

#include <stdint.h>

#define WAKE_PIN_NOT_USED UINT32_MAX

int spi_nrfx_wake_init(uint32_t wake_pin);
int spi_nrfx_wake_request(uint32_t wake_pin);

#endif /* ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_ */
