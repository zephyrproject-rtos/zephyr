/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_GFSK_H_
#define ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_GFSK_H_

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>

/**
 * @brief Set the radio up for GFSK
 *
 * @param dev  LoRa device
 * @param gfsk Configuration to apply
 * @return 0 on success, negative on error
 */
int lbm_lora_config_gfsk(const struct device *dev, const struct lora_modem_config_gfsk *gfsk);

#endif /* ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_GFSK_H_ */
