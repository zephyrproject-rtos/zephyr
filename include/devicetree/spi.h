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
 * @brief Does a SPI controller have chip select GPIOs configured?
 * The commonly used "cs-gpios" property is used for this test.
 * @param spi node identifier for a SPI bus controller
 * @return 1 if it has a cs-gpios property, 0 otherwise
 */
#define DT_SPI_HAS_CS_GPIOS(spi) DT_NODE_HAS_PROP(spi, cs_gpios)

/**
 * @brief The number of chip select GPIOs in a SPI controller
 * @param spi node identifier for a SPI bus controller
 * @return The length of its cs-gpios, or 0 if it doesn't have one
 */
#define DT_SPI_NUM_CS_GPIOS(spi) \
	COND_CODE_1(DT_SPI_HAS_CS_GPIOS(spi), \
		    (DT_PROP_LEN(spi, cs_gpios)), (0))

/**
 * @brief Does a SPI device have a chip select line in DT?
 * @param spi_dev node identifier for a SPI device
 * @return 1 if the SPI device's bus DT_BUS(spi_dev) has a CS
 *         pin at index DT_REG_ADDR(spi_dev), 0 otherwise
 */
#define DT_SPI_DEV_HAS_CS_GPIOS(spi_dev) DT_SPI_HAS_CS_GPIOS(DT_BUS(spi_dev))

/**
 * @brief Get GPIO controller name for a SPI device's chip select
 * DT_SPI_DEV_HAS_CS_GPIOS(spi_dev) must expand to 1.
 * @brief spi_dev a SPI device node identifier
 * @return label property of spi_dev's chip select GPIO controller
 */
#define DT_SPI_DEV_CS_GPIOS_LABEL(spi_dev) \
	DT_GPIO_LABEL_BY_IDX(DT_BUS(spi_dev), cs_gpios, DT_REG_ADDR(spi_dev))

/**
 * @brief Get GPIO specifier 'pin' value for a SPI device's chip select
 * It's an error if the GPIO specifier for spi_dev's entry in its
 * bus node's cs-gpios property has no 'pin' value.
 * @brief spi_dev a SPI device node identifier
 * @return pin number of spi_dev's chip select GPIO
 */
#define DT_SPI_DEV_CS_GPIOS_PIN(spi_dev) \
	DT_GPIO_PIN_BY_IDX(DT_BUS(spi_dev), cs_gpios, DT_REG_ADDR(spi_dev))

/**
 * @brief Get GPIO specifier 'flags' value for a SPI device's chip select
 * It's an error if the GPIO specifier for spi_dev's entry in its
 * bus node's cs-gpios property has no 'flags' value.
 * @brief spi_dev a SPI device node identifier
 * @return flags value of spi_dev's chip select GPIO specifier
 */
#define DT_SPI_DEV_CS_GPIOS_FLAGS(spi_dev) \
	DT_GPIO_FLAGS_BY_IDX(DT_BUS(spi_dev), cs_gpios, DT_REG_ADDR(spi_dev))

/**
 * @brief Equivalent to DT_SPI_DEV_HAS_CS_GPIOS(DT_DRV_INST(inst))
 * @param inst instance number
 * @return 1 if the instance's bus has a CS pin at index
 *         DT_INST_REG_ADDR(inst), 0 otherwise
 */
#define DT_INST_SPI_DEV_HAS_CS_GPIOS(inst) \
	DT_SPI_DEV_HAS_CS_GPIOS(DT_DRV_INST(inst))

/**
 * @brief Get GPIO controller name for a SPI device instance
 * This is equivalent to DT_SPI_DEV_CS_GPIOS_LABEL(DT_DRV_INST(inst)).
 * @brief inst instance number
 * @return label property of the instance's chip select GPIO controller
 */
#define DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst) \
	DT_SPI_DEV_CS_GPIOS_LABEL(DT_DRV_INST(inst))

/**
 * @brief Get GPIO specifier "pin" value for a SPI device instance
 * This is equivalent to DT_SPI_DEV_CS_GPIOS_PIN(DT_DRV_INST(inst)).
 * @brief inst a SPI device instance number
 * @return pin number of the instance's chip select GPIO
 */
#define DT_INST_SPI_DEV_CS_GPIOS_PIN(inst) \
	DT_SPI_DEV_CS_GPIOS_PIN(DT_DRV_INST(inst))

/**
 * @brief Get GPIO specifier "flags" value for a SPI device instance
 * This is equivalent to DT_SPI_DEV_CS_GPIOS_FLAGS(DT_DRV_INST(inst)).
 * @brief inst a SPI device instance number
 * @return flags value of the instance's chip select GPIO specifier
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
