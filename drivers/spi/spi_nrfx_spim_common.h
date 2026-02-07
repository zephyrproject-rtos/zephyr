/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_NRFX_SPIM_COMMON_H_
#define ZEPHYR_DRIVERS_SPI_NRFX_SPIM_COMMON_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>

#include <nrfx_spim.h>
#include <soc.h>
#include <dmm.h>

#if CONFIG_SPI_NRFX_RAM_BUFFER_SIZE
#define SPI_NRFX_HAS_RAM_BUF 1
#define SPI_NRFX_RAM_BUF_SIZE CONFIG_SPI_NRFX_RAM_BUFFER_SIZE
#endif

typedef void (*spi_nrfx_data_common_evt_handler)(const struct device *dev,
						 nrfx_spim_event_t *evt);

struct spi_nrfx_common_data {
	nrfx_spim_t spim;
	bool configured;
	struct spi_config spi_cfg;
#if SPI_NRFX_HAS_RAM_BUF || CONFIG_HAS_NORDIC_DMM
	const uint8_t *tx_user_buf;
	uint8_t *rx_user_buf;
	size_t rx_user_buf_len;
#endif
};

struct spi_nrfx_common_config {
	void (*irq_connect)(void);
	const struct pinctrl_dev_config *pcfg;
	spi_nrfx_data_common_evt_handler evt_handler;
	uint32_t max_freq;
	uint16_t max_chunk_len;
	uint8_t orc;
#if NRF_SPIM_HAS_RXDELAY
	uint8_t rx_delay;
#endif
	struct gpio_dt_spec wake_pin;
#if SPI_NRFX_HAS_RAM_BUF
	uint8_t *tx_ram_buf;
	uint8_t *rx_ram_buf;
#endif
#if CONFIG_HAS_NORDIC_DMM
	void *mem_reg;
#endif
};

int spi_nrfx_spim_common_transfer_start(const struct device *dev,
					const uint8_t *tx_buf,
					size_t tx_buf_len,
					uint8_t *rx_buf,
					size_t rx_buf_len);

void spi_nrfx_spim_common_transfer_end(const struct device *dev,
				       const nrfx_spim_xfer_desc_t *xfer);

int spi_nrfx_spim_common_configure(const struct device *dev, const struct spi_config *spi_cfg);
int spi_nrfx_spim_common_pm_action(const struct device *dev, enum pm_device_action action);
int spi_nrfx_spim_common_init(const struct device *dev);
int spi_nrfx_spim_common_deinit(const struct device *dev);

#define SPI_NRFX_COMMON_IRQ_DEFINE(inst, _data)							\
	NRF_DT_INST_IRQ_DIRECT_DEFINE(								\
		inst,										\
		nrfx_spim_irq_handler,								\
		_data										\
	)											\
												\
	static void CONCAT(irq_connect, inst)(void)						\
	{											\
		NRF_DT_INST_IRQ_CONNECT(							\
			inst,									\
			nrfx_spim_irq_handler,							\
			_data									\
		);										\
	}

#define SPI_NRFX_COMMON_RAM_BUF_DEFINE(inst)							\
	IF_ENABLED(										\
		SPI_NRFX_HAS_RAM_BUF,								\
		(										\
			static uint8_t CONCAT(tx_ram_buf,inst)[SPI_NRFX_RAM_BUF_SIZE]		\
				DMM_MEMORY_SECTION(DT_DRV_INST(inst));				\
												\
			static uint8_t CONCAT(rx_ram_buf, inst)[SPI_NRFX_RAM_BUF_SIZE]		\
				DMM_MEMORY_SECTION(DT_DRV_INST(inst));				\
		)										\
	)

#define SPI_NRFX_COMMON_DEFINE(inst, _data)							\
	SPI_NRFX_COMMON_IRQ_DEFINE(inst, _data)							\
	SPI_NRFX_COMMON_RAM_BUF_DEFINE(inst);							\
	PINCTRL_DT_INST_DEFINE(inst);

#define SPI_NRFX_COMMON_DATA_INIT(inst)								\
	{											\
		.spim = NRFX_SPIM_INSTANCE(DT_INST_REG_ADDR(inst)),				\
	}

#define SPI_NRFX_COMMON_CONFIG_INIT_RX_DELAY(inst)						\
	IF_ENABLED(										\
		NRF_SPIM_HAS_RXDELAY,								\
		(										\
			.rx_delay = DT_INST_PROP_OR(inst, rx_delay, 0),				\
		)										\
	)

#define SPI_NRFX_COMMON_CONFIG_RAM_BUF_INIT(inst)						\
	IF_ENABLED(										\
		SPI_NRFX_HAS_RAM_BUF,								\
		(										\
			.tx_ram_buf = CONCAT(tx_ram_buf, inst),					\
			.rx_ram_buf = CONCAT(rx_ram_buf, inst),					\
		)										\
	)

#define SPI_NRFX_COMMON_CONFIG_MEM_REG_INIT(inst)						\
	IF_ENABLED(										\
		CONFIG_HAS_NORDIC_DMM,								\
		(										\
			.mem_reg = DMM_DEV_TO_REG(DT_DRV_INST(inst)),				\
		)										\
	)

#define SPI_NRFX_COMMON_CONFIG_INIT(inst, _evt_handler)						\
	{											\
		.irq_connect = CONCAT(irq_connect, inst),					\
		.evt_handler = _evt_handler,							\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
		.max_freq = DT_INST_PROP(inst, max_frequency),					\
		.max_chunk_len = BIT_MASK(DT_INST_PROP(inst, easydma_maxcnt_bits)),		\
		.orc = DT_INST_PROP(inst, overrun_character),					\
		SPI_NRFX_COMMON_CONFIG_INIT_RX_DELAY(inst)					\
		.wake_pin = GPIO_DT_SPEC_INST_GET_OR(inst, wake_gpios, {0}),			\
		SPI_NRFX_COMMON_CONFIG_RAM_BUF_INIT(inst)					\
		SPI_NRFX_COMMON_CONFIG_MEM_REG_INIT(inst)					\
	}

#endif /* ZEPHYR_DRIVERS_SPI_NRFX_SPIM_COMMON_H_ */
