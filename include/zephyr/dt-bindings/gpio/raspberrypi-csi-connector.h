/*
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Raspberry Pi CSI camera connector pin constants
 * @ingroup raspberrypi-csi-connector
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_RASPBERRYPI_CSI_CONNECTOR_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_RASPBERRYPI_CSI_CONNECTOR_H_

/**
 * @defgroup raspberrypi-csi-connector Raspberry Pi CSI connector
 * @brief Constants for pins exposed on Raspberry Pi CSI camera connector
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */
#define CSI_IO0	1	/**< GPIO0 */
#define CSI_IO1	2	/**< GPIO1 */
#define CSI_I2C_SCL	3	/**< I2C clock pin */
#define CSI_I2C_SDA	4	/**< I2C data pin */
/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_RASPBERRYPI_CSI_CONNECTOR_H_ */
