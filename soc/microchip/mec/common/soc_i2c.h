/*
 * Copyright (c) 2021 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Microchip XEC MCU family I2C port support.
 *
 */

#ifndef _MICROCHIP_MEC_SOC_I2C_H_
#define _MICROCHIP_MEC_SOC_I2C_H_

#include <zephyr/device.h>

/* 144-pin package I2C port masks */
#if defined(CONFIG_SOC_MEC172X_NSZ)
#define MEC_I2C_PORT_MASK 0xFEFFU
#elif defined(CONFIG_SOC_MEC1501_HSZ)
#define MEC_I2C_PORT_MASK 0xFEFFU
#else
#define MEC_I2C_PORT_MASK 0xFFFFU
#endif

#define MCHP_I2C_PORT_0   0
#define MCHP_I2C_PORT_1   1
#define MCHP_I2C_PORT_2   2
#define MCHP_I2C_PORT_3   3
#define MCHP_I2C_PORT_4   4
#define MCHP_I2C_PORT_5   5
#define MCHP_I2C_PORT_6   6
#define MCHP_I2C_PORT_7   7
#define MCHP_I2C_PORT_8   8
#define MCHP_I2C_PORT_9   9
#define MCHP_I2C_PORT_10  10
#define MCHP_I2C_PORT_11  11
#define MCHP_I2C_PORT_12  12
#define MCHP_I2C_PORT_13  13
#define MCHP_I2C_PORT_14  14
#define MCHP_I2C_PORT_15  15
#define MCHP_I2C_PORT_MAX 16

#ifdef CONFIG_SOC_SERIES_MEC172X
/*
 * Read pin states of specified I2C port.
 * We GPIO control register always active RO pad input bit.
 * lines b[0]=SCL pin state at pad, b[1]=SDA pin state at pad
 * Returns 0 success or -EINVAL if port is not support or lines is NULL.
 */
#define SOC_I2C_SCL_POS 0
#define SOC_I2C_SDA_POS 1

int soc_i2c_port_lines_get(uint8_t port, uint32_t *lines);
#endif /* CONFIG_SOC_SERIES_MEC172X */

#ifdef CONFIG_I2C_XEC_V3
/** @brief Get I2C controller's port configuration
 *
 * @param dev Pointer to I2C controller's device structure
 * @param port Pointer to port variable to fill in with port number
 *
 * @retval Return on success or negative error code
 */
int i2c_xec_v3_get_port(const struct device *dev, uint8_t *port);

/** @brief Configure I2C controller for specified port and frequency
 *
 * @param dev pointer to I2C controller's device structure
 * @param config I2C configuration 32-bit word containing encoded frequency
 * @param port I2C hardware port number
 *
 * @retval Return on success or negative error code
 */
int i2c_xec_v3_config(const struct device *dev, uint32_t config, uint8_t port);
#endif /* CONFIG_I2C_XEC_V3 */

#endif /* _MICROCHIP_MEC_SOC_I2C_H_ */
