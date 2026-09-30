/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_GFSK_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_GFSK_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>

/* lora_config_gfsk() implementation */
int sx128x_lora_config_gfsk(const struct device *dev, const struct lora_modem_config_gfsk *cfg);

/*
 * SetPacketParams for a GFSK frame of payload_len bytes; a fixed-length
 * configuration uses its own length instead.
 */
int sx128x_set_gfsk_packet_params(const struct device *dev,
				  const struct lora_modem_config_gfsk *cfg, uint8_t payload_len);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_GFSK_H_ */
