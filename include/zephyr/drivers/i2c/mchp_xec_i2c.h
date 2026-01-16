/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define MCHP_I2C_NUM_PORTS (16)

#ifdef CONFIG_I2C_XEC_PORT_MUX

#define I2C_XEC_PORT_POS      (16)
#define I2C_XEC_PORT_MSK0     (0xfu)
#define I2C_XEC_PORT_MSK      GENMASK(19, 16)
#define I2C_XEC_PORT_SET(p)   FIELD_PREP(I2C_XEC_PORT_MSK, (p))
#define I2C_XEC_PORT_GET(cfg) FIELD_GET(I2C_XEC_PORT_MSK, (cfg))

#endif
#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_ */
