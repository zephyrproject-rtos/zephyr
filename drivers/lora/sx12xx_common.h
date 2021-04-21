/*
 * Copyright (c) 2020 Andreas Sandberg
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SX12XX_COMMON_H_
#define ZEPHYR_DRIVERS_SX12XX_COMMON_H_

#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <device.h>

int __sx12xx_configure_pin(const struct device * *dev, const char *controller,
			   gpio_pin_t pin, gpio_flags_t flags);

#define sx12xx_configure_pin(_name, _flags)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(0, _name##_gpios),		\
		    (__sx12xx_configure_pin(&dev_data._name,		\
				DT_INST_GPIO_LABEL(0, _name##_gpios),	\
				DT_INST_GPIO_PIN(0, _name##_gpios),	\
				DT_INST_GPIO_FLAGS(0, _name##_gpios) |	\
						      _flags)),		\
		    (0))

int sx12xx_lora_send(const struct device *dev, uint8_t *data,
		     uint32_t data_len);

int sx12xx_lora_recv(const struct device *dev, uint8_t *data, uint8_t size,
		     k_timeout_t timeout, int16_t *rssi, int8_t *snr);

int sx12xx_lora_config(const struct device *dev,
		       struct lora_modem_config *config);

int sx12xx_lora_test_cw(const struct device *dev, uint32_t frequency,
			int8_t tx_power,
			uint16_t duration);

int sx12xx_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SX12XX_COMMON_H_ */
