/*
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SX12XX_COMMON_H_
#define ZEPHYR_DRIVERS_SX12XX_COMMON_H_

#include <zephyr/types.h>
#include <drivers/lora.h>
#include <device.h>

int sx12xx_lora_send(struct device *dev, uint8_t *data, uint32_t data_len);

int sx12xx_lora_recv(struct device *dev, uint8_t *data, uint8_t size,
		     k_timeout_t timeout, int16_t *rssi, int8_t *snr);

int sx12xx_lora_config(struct device *dev, struct lora_modem_config *config);

int sx12xx_lora_test_cw(struct device *dev, uint32_t frequency, int8_t tx_power,
			uint16_t duration);

int sx12xx_init(struct device *dev);

#endif /* ZEPHYR_DRIVERS_SX12XX_COMMON_H_ */
