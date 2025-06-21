/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_RASPBERRYPI_CSI_15PINS_CONNECTOR_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_RASPBERRYPI_CSI_15PINS_CONNECTOR_H_

/**
 * @name CSI 15 pins camera connector pinout
 * @{
 */
#define CSI_15PINS IO0          11	/**< GPIO0 */
#define CSI_15PINS IO1          12	/**< GPIO1 */
#define CSI_15PINS I2C_SCL      13	/**< I2C clock pin */
#define CSI_15PINS I2C_SDA      14	/**< I2C data pin */

/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_RASPBERRYPI_CSI_15PINS_CONNECTOR_H_ */
