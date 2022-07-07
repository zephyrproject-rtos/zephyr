/*
 * Copyright (c) 2020 Andreas Sandberg
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SX12XX_COMMON_H_
#define ZEPHYR_DRIVERS_SX12XX_COMMON_H_

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/device.h>

int __sx12xx_configure_pin(const struct gpio_dt_spec *gpio, gpio_flags_t flags);

#define sx12xx_configure_pin(_name, _flags)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(0, _name##_gpios),		\
		    (__sx12xx_configure_pin(&dev_config._name, _flags)),\
		    (0))

int sx12xx_lora_send(const struct device *dev, uint8_t *data,
		     uint32_t data_len);

int sx12xx_lora_send_async(const struct device *dev, uint8_t *data,
			   uint32_t data_len, struct k_poll_signal *async);

int sx12xx_lora_recv(const struct device *dev, uint8_t *data, uint8_t size,
		     k_timeout_t timeout, int16_t *rssi, int8_t *snr);

int sx12xx_lora_recv_async(const struct device *dev, lora_recv_cb cb);

int sx12xx_lora_config(const struct device *dev,
		       struct lora_modem_config *config);

int sx12xx_lora_test_cw(const struct device *dev, uint32_t frequency,
			int8_t tx_power,
			uint16_t duration);

int sx12xx_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SX12XX_COMMON_H_ */
