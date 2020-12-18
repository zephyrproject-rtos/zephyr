/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_STM32_H_
#define DMA_STM32_H_

#include <soc.h>
#include <stm32_ll_dma.h>
#include <drivers/dma.h>
#include <drivers/clock_control/stm32_clock_control.h>

/* Maximum data sent in single transfer (Bytes) */
#define DMA_STM32_MAX_DATA_ITEMS	0xffff

struct dma_stm32_stream {
	uint32_t direction;
#ifdef CONFIG_DMAMUX_STM32
	int mux_channel; /* stores the dmamux channel */
#endif /* CONFIG_DMAMUX_STM32 */
	bool source_periph;
	bool hal_override;
	volatile bool busy;
	uint32_t src_size;
	uint32_t dst_size;
	void *user_data; /* holds the client data */
	dma_callback_t dma_callback;
};

struct dma_stm32_data {
};

struct dma_stm32_config {
	struct stm32_pclken pclken;
	void (*config_irq)(const struct device *dev);
	bool support_m2m;
	uint32_t base;
	uint32_t max_streams;
#ifdef CONFIG_DMAMUX_STM32
	uint8_t offset; /* position in the list of dmamux channel list */
#endif
	struct dma_stm32_stream *streams;
};

#ifdef CONFIG_DMA_STM32_V1
/* from DTS the dma stream id is in range 0..<dma-requests>-1 */
#define STREAM_OFFSET 0
#else
/* from DTS the dma stream id is in range 1..<dma-requests> */
/* so decrease to set range from 0 from now on */
#define STREAM_OFFSET 1
#endif /* CONFIG_DMA_STM32_V1 */

uint32_t dma_stm32_id_to_stream(uint32_t id);
#ifdef CONFIG_DMA_STM32_V1
uint32_t dma_stm32_slot_to_channel(uint32_t id);
#endif

typedef void (*dma_stm32_clear_flag_func)(DMA_TypeDef *DMAx);
typedef uint32_t (*dma_stm32_check_flag_func)(DMA_TypeDef *DMAx);

bool dma_stm32_is_tc_active(DMA_TypeDef *DMAx, uint32_t id);
void dma_stm32_clear_tc(DMA_TypeDef *DMAx, uint32_t id);
bool dma_stm32_is_ht_active(DMA_TypeDef *DMAx, uint32_t id);
void dma_stm32_clear_ht(DMA_TypeDef *DMAx, uint32_t id);
bool dma_stm32_is_te_active(DMA_TypeDef *DMAx, uint32_t id);
void dma_stm32_clear_te(DMA_TypeDef *DMAx, uint32_t id);

#ifdef CONFIG_DMA_STM32_V1
bool dma_stm32_is_dme_active(DMA_TypeDef *DMAx, uint32_t id);
void dma_stm32_clear_dme(DMA_TypeDef *DMAx, uint32_t id);
bool dma_stm32_is_fe_active(DMA_TypeDef *DMAx, uint32_t id);
void dma_stm32_clear_fe(DMA_TypeDef *DMAx, uint32_t id);
#endif

#ifdef CONFIG_DMA_STM32_V2
bool dma_stm32_is_gi_active(DMA_TypeDef *DMAx, uint32_t id);
void dma_stm32_clear_gi(DMA_TypeDef *DMAx, uint32_t id);
#endif

bool stm32_dma_is_irq_active(DMA_TypeDef *dma, uint32_t id);

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, uint32_t id);
void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, uint32_t id);
bool stm32_dma_is_irq_happened(DMA_TypeDef *dma, uint32_t id);
bool stm32_dma_is_unexpected_irq_happened(DMA_TypeDef *dma, uint32_t id);
void stm32_dma_enable_stream(DMA_TypeDef *dma, uint32_t id);
int stm32_dma_disable_stream(DMA_TypeDef *dma, uint32_t id);
void stm32_dma_config_channel_function(DMA_TypeDef *dma, uint32_t id, uint32_t slot);

#ifdef CONFIG_DMA_STM32_V1
void stm32_dma_disable_fifo_irq(DMA_TypeDef *dma, uint32_t id);
bool stm32_dma_check_fifo_mburst(LL_DMA_InitTypeDef *DMAx);
uint32_t stm32_dma_get_fifo_threshold(uint16_t fifo_mode_control);
uint32_t stm32_dma_get_mburst(struct dma_config *config, bool source_periph);
uint32_t stm32_dma_get_pburst(struct dma_config *config, bool source_periph);
#endif

#ifdef CONFIG_DMAMUX_STM32
/* dma_stm32_ api functions are exported to the dmamux_stm32 */
#define DMA_STM32_EXPORT_API
int dma_stm32_configure(const struct device *dev, uint32_t id,
				struct dma_config *config);
int dma_stm32_reload(const struct device *dev, uint32_t id,
			uint32_t src, uint32_t dst, size_t size);
int dma_stm32_start(const struct device *dev, uint32_t id);
int dma_stm32_stop(const struct device *dev, uint32_t id);
int dma_stm32_get_status(const struct device *dev, uint32_t id,
				struct dma_status *stat);
#else
#define DMA_STM32_EXPORT_API static
#endif /* CONFIG_DMAMUX_STM32 */

#endif /* DMA_STM32_H_*/
