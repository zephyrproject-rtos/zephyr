/*
 * Copyright (c) 2026 Fuyu Fei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_DM9000_PRIV_H__
#define ETH_DM9000_PRIV_H__

#include <stdint.h>
#include <zephyr/device.h>

int dm9000_mdio_c22_read(const struct device *mac, uint8_t prtad, uint8_t regad,
			 uint16_t *data);

int dm9000_mdio_c22_write(const struct device *mac, uint8_t prtad, uint8_t regad,
			  uint16_t data);

#endif /* ETH_DM9000_PRIV_H__ */
