/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_ESP32_H_

enum gdma_trigger_peripheral {
	GDMA_TRIG_PERIPH_M2M = -1,
	GDMA_TRIG_PERIPH_SPI2 = 0,
	GDMA_TRIG_PERIPH_UHCI0 = 2,
	GDMA_TRIG_PERIPH_I2S = 4,
	GDMA_TRIG_PERIPH_AES = 6,
	GDMA_TRIG_PERIPH_SHA = 7,
	GDMA_TRIG_PERIPH_ADC = 8,
	GDMA_TRIG_PERIPH_INVALID = 0x3F,
};

#define ESP32_DT_INST_DMA_CTLR(n, name)			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, name))),	\
		    (NULL))

#define ESP32_DT_INST_DMA_CELL(n, name, cell)		\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),	\
		    (0xff))


#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_ESP32_H_ */
