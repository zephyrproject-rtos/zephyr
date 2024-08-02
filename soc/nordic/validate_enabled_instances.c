/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define I2C_ENABLED(idx)  (IS_ENABLED(CONFIG_I2C) && \
			   DT_NODE_HAS_STATUS(DT_NODELABEL(i2c##idx), okay))

#define SPI_ENABLED(idx)  (IS_ENABLED(CONFIG_SPI) && \
			   DT_NODE_HAS_STATUS(DT_NODELABEL(spi##idx), okay))

#define UART_ENABLED(idx) (IS_ENABLED(CONFIG_SERIAL) && \
			   (IS_ENABLED(CONFIG_SOC_SERIES_NRF53X) || \
			    IS_ENABLED(CONFIG_SOC_SERIES_NRF91X)) && \
			   DT_NODE_HAS_STATUS(DT_NODELABEL(uart##idx), okay))

/*
 * In most Nordic SoCs, SPI and TWI peripherals with the same instance number
 * share certain resources and therefore cannot be used at the same time (in
 * nRF53 and nRF91 Series this limitation concerns UART peripherals as well).
 *
 * In some SoCs, like nRF52810, there are only single instances of
 * these peripherals and they are arranged in a different way, so this
 * limitation does not apply.
 *
 * The build assertions below check if conflicting peripheral instances are not
 * enabled simultaneously.
 */

#define CHECK(idx) \
	!(I2C_ENABLED(idx) && SPI_ENABLED(idx)) && \
	!(I2C_ENABLED(idx) && UART_ENABLED(idx)) && \
	!(SPI_ENABLED(idx) && UART_ENABLED(idx))

#define MSG(idx) \
	"Only one of the following peripherals can be enabled: " \
	"SPI"#idx", SPIM"#idx", SPIS"#idx", TWI"#idx", TWIM"#idx", TWIS"#idx \
	IF_ENABLED(CONFIG_SOC_SERIES_NRF53X, (", UARTE"#idx)) \
	IF_ENABLED(CONFIG_SOC_SERIES_NRF91X, (", UARTE"#idx)) \
	". Check nodes with status \"okay\" in zephyr.dts."

#if (!IS_ENABLED(CONFIG_SOC_NRF52810) &&	\
	!IS_ENABLED(CONFIG_SOC_NRF52805) &&	\
	!IS_ENABLED(CONFIG_SOC_NRF52811))
BUILD_ASSERT(CHECK(0), MSG(0));
#endif
BUILD_ASSERT(CHECK(1), MSG(1));
BUILD_ASSERT(CHECK(2), MSG(2));
BUILD_ASSERT(CHECK(3), MSG(3));

#if defined(CONFIG_SOC_NRF52811)
BUILD_ASSERT(!(SPI_ENABLED(1) && I2C_ENABLED(0)),
	     "Only one of the following peripherals can be enabled: "
	     "SPI1, SPIM1, SPIS1, TWI0, TWIM0, TWIS0. "
	     "Check nodes with status \"okay\" in zephyr.dts.");
#endif
