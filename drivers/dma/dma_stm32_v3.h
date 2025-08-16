/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_STM32_V3_H_
#define DMA_STM32_V3_H_

#include <stm32_ll_dma.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/dma.h>

struct dma_stm32_descriptor {
	uint32_t channel_tr1;
	uint32_t channel_tr2;
	uint32_t channel_br1;
	uint32_t channel_sar;
	uint32_t channel_dar;
	uint32_t channel_llr;
};

struct dma_stm32_channel {
	uint32_t direction;
	bool hal_override;
	volatile bool busy;
	uint32_t state;
	uint32_t src_size;
	uint32_t dst_size;
	void *user_data;
	dma_callback_t dma_callback;
	bool cyclic;
	int block_count;
};

struct dma_stm32_data {
	struct dma_context dma_ctx;
};

struct dma_stm32_config {
	struct stm32_pclken pclken;
	void (*config_irq)(const struct device *dev);
	bool support_m2m;
	DMA_TypeDef * base;
	uint32_t max_channels;
	struct dma_stm32_channel *channels;
	volatile uint32_t *linked_list_buffer;
};

uint32_t dma_stm32_id_to_channel(uint32_t id);

void dma_stm32_dump_channel_irq(const DMA_TypeDef *dma, uint32_t channel);
void dma_stm32_clear_channel_irq(const DMA_TypeDef *dma, uint32_t channel);

int dma_stm32_disable_channel(const DMA_TypeDef *dma, uint32_t channel);

/* DMA driver api functions */
int dma_stm32_configure(const struct device *dev, uint32_t id, struct dma_config *config);
int dma_stm32_reload(const struct device *dev, uint32_t id, uint32_t src, uint32_t dst,
		     size_t size);
int dma_stm32_start(const struct device *dev, uint32_t id);
int dma_stm32_stop(const struct device *dev, uint32_t id);
int dma_stm32_get_status(const struct device *dev, uint32_t id, struct dma_status *stat);
int dma_stm32_suspend(const struct device *dev, uint32_t id);
int dma_stm32_resume(const struct device *dev, uint32_t id);

#endif /* DMA_STM32_V3_H_ */
