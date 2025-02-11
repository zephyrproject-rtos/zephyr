/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_
#define ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_

#include <stdint.h>
#include <nrfx_gpiote.h>

#define WAKE_PIN_NOT_USED UINT32_MAX

#define WAKE_GPIOTE_INSTANCE(node_id)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, wake_gpios),		\
		(NRFX_GPIOTE_INSTANCE(					\
			NRF_DT_GPIOTE_INST(node_id, wake_gpios))),	\
		({0}))

int spi_nrfx_wake_init(const nrfx_gpiote_t *gpiote, uint32_t wake_pin);
int spi_nrfx_wake_request(const nrfx_gpiote_t *gpiote, uint32_t wake_pin);

#endif /* ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_ */
