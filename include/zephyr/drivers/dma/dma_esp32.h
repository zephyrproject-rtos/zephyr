/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_ESP32_H_

#include <zephyr/device.h>

enum gdma_trigger_peripheral {
	ESP_GDMA_TRIG_PERIPH_M2M = -1,
	ESP_GDMA_TRIG_PERIPH_SPI2 = 0,
	ESP_GDMA_TRIG_PERIPH_SPI3 = 1,
	ESP_GDMA_TRIG_PERIPH_UHCI0 = 2,
	ESP_GDMA_TRIG_PERIPH_I2S0 = 3,
	ESP_GDMA_TRIG_PERIPH_I2S1 = 4,
	ESP_GDMA_TRIG_PERIPH_I2S2 = 5, /**< Shares trigger 5 with LCD0 and CAM0 */
	ESP_GDMA_TRIG_PERIPH_LCD0 = 5,
	ESP_GDMA_TRIG_PERIPH_CAM0 = 5,
	ESP_GDMA_TRIG_PERIPH_AES = 6,
	ESP_GDMA_TRIG_PERIPH_SHA = 7,
	ESP_GDMA_TRIG_PERIPH_ADC0 = 8,
	ESP_GDMA_TRIG_PERIPH_DAC0 = 8,
	ESP_GDMA_TRIG_PERIPH_RMT = 9,
	ESP_GDMA_TRIG_PERIPH_INVALID = 0x3F,
};

#define ESP32_DT_INST_DMA_CTLR(n, name)			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, name))),	\
		    (NULL))

#define ESP32_DT_INST_DMA_CELL(n, name, cell)		\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),	\
		    (0xff))

/**
 * @brief Configures how to restart an already looped channel.
 *
 * The skip-bytes is the number of bytes expected to already be consumed
 * by the dma when dma_esp32_gdma_restart_channel() is called.
 * It is meant to keep ring synchronized so those bytes won't
 * be reread
 *
 * @param dev: The device
 * @param channel: The DMA channel to configure
 * @param skip_bytes: Number of bytes to skip
 *
 * @return 0 on success
 */
int dma_esp32_gdma_config_restart(const struct device *dev, uint32_t channel, size_t skip_bytes);

/**
 * @brief Re-launch a cyclic TX channel.
 *
 * The restart can be configured by dma_esp32_gdma_config_restart()
 *
 * @param dev: The device
 * @param channel: The DMA channel to restart
 *
 * @return 0 on success
 */
int dma_esp32_gdma_restart_channel(const struct device *dev, uint32_t channel);

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_ESP32_H_ */
