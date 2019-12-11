/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/dma.h>
#include <soc.h>

extern u32_t table_ll_stream[];
extern u32_t (*func_ll_is_active_tc[])(DMA_TypeDef *DMAx);
extern void (*func_ll_clear_tc[])(DMA_TypeDef *DMAx);
extern u32_t (*func_ll_is_active_ht[])(DMA_TypeDef *DMAx);
extern void (*func_ll_clear_ht[])(DMA_TypeDef *DMAx);
#ifdef CONFIG_DMA_STM32_V1
extern u32_t table_ll_channel[];
#endif

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, u32_t id);
void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, u32_t id);
bool stm32_dma_is_irq_happened(DMA_TypeDef *dma, u32_t id);
bool stm32_dma_is_unexpected_irq_happened(DMA_TypeDef *dma, u32_t id);
void stm32_dma_enable_stream(DMA_TypeDef *dma, u32_t id);
int stm32_dma_disable_stream(DMA_TypeDef *dma, u32_t id);
void stm32_dma_config_channel_function(DMA_TypeDef *dma, u32_t id, u32_t slot);
#ifdef CONFIG_DMA_STM32_V1
void stm32_dma_disable_fifo_irq(DMA_TypeDef *dma, u32_t id);
bool stm32_dma_check_fifo_mburst(LL_DMA_InitTypeDef *DMAx);
u32_t stm32_dma_get_fifo_threshold(u16_t fifo_mode_control);
u32_t stm32_dma_get_mburst(struct dma_config *config, bool source_periph);
u32_t stm32_dma_get_pburst(struct dma_config *config, bool source_periph);
#endif
