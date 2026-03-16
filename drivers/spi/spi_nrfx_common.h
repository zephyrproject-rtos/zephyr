/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_
#define ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_

#include <stdint.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
#include <zephyr/drivers/gpio/gpio_nrf.h>

#define WAKE_PIN_NOT_USED UINT32_MAX

#define WAKE_GPIOTE_NODE(node_id)							   \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, wake_gpios),				   \
		    (&GPIOTE_NRFX_INST_BY_NODE(DT_PHANDLE(DT_PHANDLE(node_id, wake_gpios), \
					       gpiote_instance))),			   \
		    (NULL))

int spi_nrfx_wake_init(nrfx_gpiote_t *gpiote, uint32_t wake_pin);
int spi_nrfx_wake_request(nrfx_gpiote_t *gpiote, uint32_t wake_pin);

#endif /* ZEPHYR_DRIVERS_SPI_NRFX_COMMON_H_ */
