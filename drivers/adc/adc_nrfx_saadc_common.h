/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ADC_NRFX_SAADC_COMMON_H_
#define ZEPHYR_DRIVERS_ADC_NRFX_SAADC_COMMON_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/logging/log.h>

#include <dmm.h>
#include <nrfx_saadc.h>
#include <soc.h>

#define ADC_CONTEXT_DECLARED_IN_HEADER
#define ADC_CONTEXT_ENABLE_ON_COMPLETE

#include "adc_context_declare.h"

/**
 * @cond INTERNAL_HIDDEN
 */

struct adc_nrfx_saadc_common_data {
	struct adc_context ctx;
	const struct device *dev;
	uint8_t single_ended_channels;
	uint8_t divide_single_ended_value;
	uint8_t active_channel_cnt;
	void *user_buffer;
	struct k_timer timer;
	bool internal_timer_enabled;
};

struct adc_nrfx_saadc_common_config {
	void (*irq_connect)(void);
	void (*event_handler)(const nrfx_saadc_evt_t *event);
	void (*read_complete_handler)(const struct device *dev, int ret);
	void *mem_reg;
};

int adc_nrfx_saadc_common_channel_setup(const struct device *dev,
					const struct adc_channel_cfg *channel_cfg);

void adc_nrfx_saadc_common_event_handler(const struct device *dev,
					 const nrfx_saadc_evt_t *event);

int adc_nrfx_saadc_common_start_read(const struct device *dev,
				     const struct adc_sequence *sequence);

int adc_nrfx_saadc_common_init(const struct device *dev);
int adc_nrfx_saadc_common_deinit(const struct device *dev);

#define ADC_NRFX_SAADC_COMMON_IRQ_DEFINE(inst)							\
	NRF_DT_INST_IRQ_DIRECT_DEFINE(								\
		inst,										\
		nrfx_isr,									\
		nrfx_saadc_irq_handler								\
	)											\
												\
	static void CONCAT(irq_connect, inst)(void)						\
	{											\
		NRF_DT_INST_IRQ_CONNECT(							\
			inst,									\
			nrfx_isr,								\
			nrfx_saadc_irq_handler							\
		);										\
	}

#define ADC_NRFX_SAADC_COMMON_EVENT_HANDLER_DEFINE(inst)					\
	static void CONCAT(event_handler, inst)(const nrfx_saadc_evt_t *event)		\
	{											\
		const struct device *dev = DEVICE_DT_INST_GET(inst);				\
												\
		adc_nrfx_saadc_common_event_handler(dev, event);				\
	}

#define ADC_NRFX_SAADC_COMMON_DEFINE(inst)							\
	ADC_NRFX_SAADC_COMMON_IRQ_DEFINE(inst)							\
	ADC_NRFX_SAADC_COMMON_EVENT_HANDLER_DEFINE(inst)

#define ADC_NRFX_SAADC_COMMON_DATA_INIT(inst)							\
	{											\
		.dev = DEVICE_DT_INST_GET(inst),						\
	}

#define ADC_NRFX_SAADC_COMMON_CONFIG_INIT(inst, _read_complete_handler)				\
	{											\
		.irq_connect = CONCAT(irq_connect, inst),					\
		.event_handler = CONCAT(event_handler, inst),					\
		.read_complete_handler = _read_complete_handler,				\
		.mem_reg = DMM_DEV_TO_REG(DT_DRV_INST(inst)),					\
	}

/**
 * @endcond
 */

#endif /* ZEPHYR_DRIVERS_ADC_NRFX_SAADC_COMMON_H_ */
