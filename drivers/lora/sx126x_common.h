/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Andreas Sandberg
 * Copyright (c) 2021 Fabio Baltieri
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SX126X_COMMON_H_
#define ZEPHYR_DRIVERS_SX126X_COMMON_H_

#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <drivers/spi.h>

#include <sx126x/sx126x.h>

#if DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1261)
#define DT_DRV_COMPAT semtech_sx1261
#define SX126X_DEVICE_ID SX1261
#elif DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1262)
#define DT_DRV_COMPAT semtech_sx1262
#define SX126X_DEVICE_ID SX1262
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32wl_subghz_radio)
#define DT_DRV_COMPAT st_stm32wl_subghz_radio
#define SX126X_DEVICE_ID SX1262
#else
#error No SX126x instance in device tree.
#endif

#define HAVE_GPIO_CS		DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
#define HAVE_GPIO_ANTENNA_ENABLE			\
	DT_INST_NODE_HAS_PROP(0, antenna_enable_gpios)
#define HAVE_GPIO_TX_ENABLE	DT_INST_NODE_HAS_PROP(0, tx_enable_gpios)
#define HAVE_GPIO_RX_ENABLE	DT_INST_NODE_HAS_PROP(0, rx_enable_gpios)

#define GPIO_CS_LABEL		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0)
#define GPIO_CS_PIN		DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define GPIO_CS_FLAGS		DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0)

#define GPIO_ANTENNA_ENABLE_PIN	DT_INST_GPIO_PIN(0, antenna_enable_gpios)
#define GPIO_TX_ENABLE_PIN	DT_INST_GPIO_PIN(0, tx_enable_gpios)
#define GPIO_RX_ENABLE_PIN	DT_INST_GPIO_PIN(0, rx_enable_gpios)

struct sx126x_data {
	struct gpio_callback dio1_irq_callback;
	struct k_work dio1_irq_work;
	DioIrqHandler *radio_dio_irq;
#if HAVE_GPIO_ANTENNA_ENABLE
	const struct device *antenna_enable;
#endif
#if HAVE_GPIO_TX_ENABLE
	const struct device *tx_enable;
#endif
#if HAVE_GPIO_RX_ENABLE
	const struct device *rx_enable;
#endif
	const struct device *spi;
	struct spi_config spi_cfg;
#if HAVE_GPIO_CS
	struct spi_cs_control spi_cs;
#endif
	RadioOperatingModes_t mode;
};

void sx126x_reset(struct sx126x_data *dev_data);

bool sx126x_is_busy(struct sx126x_data *dev_data);

uint32_t sx126x_get_dio1_pin_state(struct sx126x_data *dev_data);

void sx126x_dio1_irq_enable(struct sx126x_data *dev_data);

void sx126x_dio1_irq_disable(struct sx126x_data *dev_data);

int sx126x_variant_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SX126X_COMMON_H_ */
