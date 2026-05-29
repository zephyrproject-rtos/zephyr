/*
 * Copyright (c) 2023 Elektronikutvecklingsbyrån EUB AB
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Pmod GPIO nexus signal index definitions
 *
 * Defines meant to be used in conjunction with the "digilent,pmod"
 * GPIO nexus mapping.
 *
 * Example usage:
 *
 * @code{.dts}
 * &spi1 {
 *         cs-gpios = <&pmod0 PMOD_SPI_CS GPIO_ACTIVE_LOW>;
 *
 *         example_device: example-dev@0 {
 *                 compatible = "vnd,spi-device";
 *                 reg = <0>;
 *         };
 * };
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_DIGILENT_PMOD_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_DIGILENT_PMOD_H_

/**
 * For reference see the Pmod interface specification:
 * https://digilent.com/reference/_media/reference/pmod/pmod-interface-specification-1_2_0.pdf
 */

/* GPIO */

/**
 * @brief IO[n] signal on a Pmod GPIO nexus node following
 * Pmod Interface Type 1 or 1A (GPIO or expanded GPIO)
 *
 * The Pmod GPIO nexus maps pin indexes 0..7 to IO1..IO8.
 */
#define PMOD_IO(n) ((n) - 1)

/* SPI */

/**
 * @brief SPI CS signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2 (SPI) peripherals.
 */
#define PMOD_SPI_CS	PMOD_IO(1)

/**
 * @brief SPI MOSI signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2 (SPI) peripherals.
 */
#define PMOD_SPI_MOSI	PMOD_IO(2)

/**
 * @brief SPI MISO signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2 (SPI) peripherals.
 */
#define PMOD_SPI_MISO	PMOD_IO(3)

/**
 * @brief SPI SCK signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2 (SPI) peripherals.
 */
#define PMOD_SPI_SCK	PMOD_IO(4)

/* Expanded SPI */

/**
 * @brief SPI CS signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_CS		PMOD_IO(1)

/**
 * @brief SPI MOSI signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_MOSI	PMOD_IO(2)

/**
 * @brief SPI MISO signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_MISO	PMOD_IO(3)

/**
 * @brief SPI SCK signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_SCK	PMOD_IO(4)

/**
 * @brief INT alternate signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_INT	PMOD_IO(5)

/**
 * @brief RESET alternate signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_RESET	PMOD_IO(6)

/**
 * @brief SPI CS2 alternate signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_CS2	PMOD_IO(7)

/**
 * @brief SPI CS3 alternate signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 2A (expanded SPI) peripherals.
 */
#define PMOD_EXP_SPI_CS3	PMOD_IO(8)

/* Expanded UART */

/**
 * @brief INT alternate signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 3A (expanded UART) peripherals.
 */
#define PMOD_EXP_UART_INT	PMOD_IO(5)

/**
 * @brief RESET alternate signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 3A (expanded UART) peripherals.
 */
#define PMOD_EXP_UART_RESET	PMOD_IO(6)

/* H-bridge */

/**
 * @brief DIR signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 4 (H-bridge) peripherals.
 */
#define PMOD_HBRIDGE_DIR	PMOD_IO(1)

/**
 * @brief EN signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 4 (H-bridge) peripherals.
 */
#define PMOD_HBRIDGE_EN		PMOD_IO(2)

/* Dual H-bridge */

/**
 * @brief DIR1 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5 (dual H-bridge) peripherals.
 */
#define PMOD_DUAL_HBRIDGE_DIR1	PMOD_IO(1)

/**
 * @brief EN1 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5 (dual H-bridge) peripherals.
 */
#define PMOD_DUAL_HBRIDGE_EN1	PMOD_IO(2)

/**
 * @brief DIR2 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5 (dual H-bridge) peripherals.
 */
#define PMOD_DUAL_HBRIDGE_DIR2	PMOD_IO(3)

/**
 * @brief EN2 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5 (dual H-bridge) peripherals.
 */
#define PMOD_DUAL_HBRIDGE_EN2	PMOD_IO(4)

/* Expanded dual H-bridge */

/**
 * @brief DIR1 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5A (expanded dual H-bridge) peripherals.
 */
#define PMOD_EXP_DUAL_HBRIDGE_DIR1	PMOD_IO(1)

/**
 * @brief EN1 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5A (expanded dual H-bridge) peripherals.
 */
#define PMOD_EXP_DUAL_HBRIDGE_EN1	PMOD_IO(2)

/**
 * @brief DIR2 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5A (expanded dual H-bridge) peripherals.
 */
#define PMOD_EXP_DUAL_HBRIDGE_DIR2	PMOD_IO(5)

/**
 * @brief EN2 signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 5A (expanded dual H-bridge) peripherals.
 */
#define PMOD_EXP_DUAL_HBRIDGE_EN2	PMOD_IO(6)

/* I2C */

/**
 * @brief INT signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 6 (I2C) peripherals.
 */
#define PMOD_I2C_INT	PMOD_IO(1)

/**
 * @brief RESET signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 6 (I2C) peripherals.
 */
#define PMOD_I2C_RESET	PMOD_IO(2)

/**
 * @brief SCL signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 6 (I2C) peripherals.
 */
#define PMOD_I2C_SCL	PMOD_IO(3)

/**
 * @brief SDA signal index on a Pmod GPIO nexus node.
 * Used with Pmod Interface Type 6 (I2C) peripherals.
 */
#define PMOD_I2C_SDA	PMOD_IO(4)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_DIGILENT_PMOD_H_ */
