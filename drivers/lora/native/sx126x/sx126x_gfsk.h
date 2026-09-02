/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX126X_GFSK_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX126X_GFSK_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>

/**
 * @brief Set the radio up for GFSK
 *
 * @param dev    LoRa device
 * @param config Configuration to apply
 * @return 0 on success, negative on error
 */
int sx126x_lora_config_gfsk(const struct device *dev, const struct lora_modem_config_gfsk *config);

/**
 * @brief Write the GFSK packet parameters the configuration asks for
 *
 * @param dev         LoRa device
 * @param gfsk        Configuration the radio is running
 * @param payload_len Length a variable-length frame carries
 * @return 0 on success, negative on error
 */
int sx126x_set_gfsk_packet_params(const struct device *dev,
				  const struct lora_modem_config_gfsk *gfsk, uint8_t payload_len);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX126X_GFSK_H_ */
