/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I/O cell definitions for Raspberry Pi BCM283x devices
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RPI_BCM283X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RPI_BCM283X_H_

/** @name Power Domain IDs */
/** @{ */
/** @brief SD Card */
#define RPI_POWER_DOMAIN_SD_CARD  0x00000000U
/** @brief UART0 */
#define RPI_POWER_DOMAIN_UART0    0x00000001U
/** @brief UART1 */
#define RPI_POWER_DOMAIN_UART1    0x00000002U
/** @brief USB Host Controller */
#define RPI_POWER_DOMAIN_USB_HCD  0x00000003U
/** @brief I2C0 */
#define RPI_POWER_DOMAIN_I2C0     0x00000004U
/** @brief I2C1 */
#define RPI_POWER_DOMAIN_I2C1     0x00000005U
/** @brief I2C2 */
#define RPI_POWER_DOMAIN_I2C2     0x00000006U
/** @brief SPI */
#define RPI_POWER_DOMAIN_SPI      0x00000007U
/** @brief CCP2TX */
#define RPI_POWER_DOMAIN_CCP2TX   0x00000008U
/** @brief Unknown domain 0 (RPi4 only) */
#define RPI_POWER_DOMAIN_UNKNOWN0 0x00000009U
/** @brief Unknown domain 1 (RPi4 only) */
#define RPI_POWER_DOMAIN_UNKNOWN1 0x0000000AU
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RPI_BCM283X_H_ */
