/**
 * @file
 * @brief SPI Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_SPI_H_
#define ZEPHYR_INCLUDE_DEVICETREE_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-spi Devicetree SPI API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Does a SPI controller node have chip select GPIOs configured?
 *
 * SPI bus controllers use the "cs-gpios" property for configuring
 * chip select GPIOs. Its value is a phandle-array which specifies the
 * chip select lines.
 *
 * Example devicetree fragment:
 *
 *     spi1: spi@... {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *     };
 *
 *     spi2: spi@... {
 *             compatible = "vnd,spi";
 *     };
 *
 * Example usage:
 *
 *     DT_SPI_HAS_CS_GPIOS(DT_NODELABEL(spi1)) // 1
 *     DT_SPI_HAS_CS_GPIOS(DT_NODELABEL(spi2)) // 0
 *
 * @param spi a SPI bus controller node identifier
 * @return 1 if "spi" has a cs-gpios property, 0 otherwise
 */
#define DT_SPI_HAS_CS_GPIOS(spi) DT_NODE_HAS_PROP(spi, cs_gpios)

/**
 * @brief Number of chip select GPIOs in a SPI controller's cs-gpios property
 *
 * Example devicetree fragment:
 *
 *     spi1: spi@... {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *     };
 *
 *     spi2: spi@... {
 *             compatible = "vnd,spi";
 *     };
 *
 * Example usage:
 *
 *     DT_SPI_NUM_CS_GPIOS(DT_NODELABEL(spi1)) // 2
 *     DT_SPI_NUM_CS_GPIOS(DT_NODELABEL(spi2)) // 0
 *
 * @param spi a SPI bus controller node identifier
 * @return Logical length of spi's cs-gpios property, or 0 if "spi" doesn't
 *         have a cs-gpios property
 */
#define DT_SPI_NUM_CS_GPIOS(spi) \
	COND_CODE_1(DT_SPI_HAS_CS_GPIOS(spi), \
		    (DT_PROP_LEN(spi, cs_gpios)), (0))

/**
 * @brief Does a SPI device have a chip select line configured?
 * Example devicetree fragment:
 *
 *     spi1: spi@... {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *
 *             a: spi-dev-a@0 {
 *                     reg = <0>;
 *             };
 *
 *             b: spi-dev-b@1 {
 *                     reg = <1>;
 *             };
 *     };
 *
 *     spi2: spi@... {
 *             compatible = "vnd,spi";
 *             c: spi-dev-c@0 {
 *                     reg = <0>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_SPI_DEV_HAS_CS_GPIOS(DT_NODELABEL(a)) // 1
 *     DT_SPI_DEV_HAS_CS_GPIOS(DT_NODELABEL(b)) // 1
 *     DT_SPI_DEV_HAS_CS_GPIOS(DT_NODELABEL(c)) // 0
 *
 * @param spi_dev a SPI device node identifier
 * @return 1 if spi_dev's bus node DT_BUS(spi_dev) has a chip select
 *         pin at index DT_REG_ADDR(spi_dev), 0 otherwise
 */
#define DT_SPI_DEV_HAS_CS_GPIOS(spi_dev) DT_SPI_HAS_CS_GPIOS(DT_BUS(spi_dev))

/**
 * @brief Get a SPI device's chip select GPIO controller's label property
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... {
 *             label = "GPIO_1";
 *     };
 *
 *     gpio2: gpio@... {
 *             label = "GPIO_2";
 *     };
 *
 *     spi1: spi@... {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *
 *             a: spi-dev-a@0 {
 *                     reg = <0>;
 *             };
 *
 *             b: spi-dev-b@1 {
 *                     reg = <1>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_SPI_DEV_CS_GPIOS_LABEL(DT_NODELABEL(a)) // "GPIO_1"
 *     DT_SPI_DEV_CS_GPIOS_LABEL(DT_NODELABEL(b)) // "GPIO_2"
 *
 * @param spi_dev a SPI device node identifier
 * @return label property of spi_dev's chip select GPIO controller
 */
#define DT_SPI_DEV_CS_GPIOS_LABEL(spi_dev) \
	DT_GPIO_LABEL_BY_IDX(DT_BUS(spi_dev), cs_gpios, DT_REG_ADDR(spi_dev))

/**
 * @brief Get a SPI device's chip select GPIO pin number
 *
 * It's an error if the GPIO specifier for spi_dev's entry in its
 * bus node's cs-gpios property has no pin cell.
 *
 * Example devicetree fragment:
 *
 *     spi1: spi@... {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *
 *             a: spi-dev-a@0 {
 *                     reg = <0>;
 *             };
 *
 *             b: spi-dev-b@1 {
 *                     reg = <1>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_SPI_DEV_CS_GPIOS_PIN(DT_NODELABEL(a)) // 10
 *     DT_SPI_DEV_CS_GPIOS_PIN(DT_NODELABEL(b)) // 20
 *
 * @param spi_dev a SPI device node identifier
 * @return pin number of spi_dev's chip select GPIO
 */
#define DT_SPI_DEV_CS_GPIOS_PIN(spi_dev) \
	DT_GPIO_PIN_BY_IDX(DT_BUS(spi_dev), cs_gpios, DT_REG_ADDR(spi_dev))

/**
 * @brief Get a SPI device's chip select GPIO flags
 *
 * Example devicetree fragment:
 *
 *     spi1: spi@... {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>;
 *
 *             a: spi-dev-a@0 {
 *                     reg = <0>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_SPI_DEV_CS_GPIOS_FLAGS(DT_NODELABEL(a)) // GPIO_ACTIVE_LOW
 *
 * If the GPIO specifier for spi_dev's entry in its bus node's
 * cs-gpios property has no flags cell, this expands to zero.
 *
 * @param spi_dev a SPI device node identifier
 * @return flags value of spi_dev's chip select GPIO specifier, or
 *         zero if there is none
 */
#define DT_SPI_DEV_CS_GPIOS_FLAGS(spi_dev) \
	DT_GPIO_FLAGS_BY_IDX(DT_BUS(spi_dev), cs_gpios, DT_REG_ADDR(spi_dev))

/**
 * @brief Equivalent to DT_SPI_DEV_HAS_CS_GPIOS(DT_DRV_INST(inst)).
 * @param inst DT_DRV_COMPAT instance number
 * @return 1 if the instance's bus has a CS pin at index
 *         DT_INST_REG_ADDR(inst), 0 otherwise
 * @see DT_SPI_DEV_HAS_CS_GPIOS()
 */
#define DT_INST_SPI_DEV_HAS_CS_GPIOS(inst) \
	DT_SPI_DEV_HAS_CS_GPIOS(DT_DRV_INST(inst))

/**
 * @brief Get GPIO controller name for a SPI device instance
 * This is equivalent to DT_SPI_DEV_CS_GPIOS_LABEL(DT_DRV_INST(inst)).
 * @param inst DT_DRV_COMPAT instance number
 * @return label property of the instance's chip select GPIO controller
 * @see DT_SPI_DEV_CS_GPIOS_LABEL()
 */
#define DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst) \
	DT_SPI_DEV_CS_GPIOS_LABEL(DT_DRV_INST(inst))

/**
 * @brief Equivalent to DT_SPI_DEV_CS_GPIOS_PIN(DT_DRV_INST(inst)).
 * @param inst DT_DRV_COMPAT instance number
 * @return pin number of the instance's chip select GPIO
 * @see DT_SPI_DEV_CS_GPIOS_PIN()
 */
#define DT_INST_SPI_DEV_CS_GPIOS_PIN(inst) \
	DT_SPI_DEV_CS_GPIOS_PIN(DT_DRV_INST(inst))

/**
 * @brief DT_SPI_DEV_CS_GPIOS_FLAGS(DT_DRV_INST(inst)).
 * @param inst DT_DRV_COMPAT instance number
 * @return flags value of the instance's chip select GPIO specifier,
 *         or zero if there is none
 * @see DT_SPI_DEV_CS_GPIOS_FLAGS()
 */
#define DT_INST_SPI_DEV_CS_GPIOS_FLAGS(inst) \
	DT_SPI_DEV_CS_GPIOS_FLAGS(DT_DRV_INST(inst))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_SPI_H_ */
