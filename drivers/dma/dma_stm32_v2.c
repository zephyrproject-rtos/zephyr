/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA low level driver implementation for F0/F1/F3/L0/L4 series SoCs.
 */

#include <drivers/dma.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_stm32_v2);

u32_t table_ll_stream[] = {
	LL_DMA_CHANNEL_1,
	LL_DMA_CHANNEL_2,
	LL_DMA_CHANNEL_3,
	LL_DMA_CHANNEL_4,
	LL_DMA_CHANNEL_5,
#if defined(LL_DMA_CHANNEL_6)
	LL_DMA_CHANNEL_6,
#endif /* LL_DMA_CHANNEL_6 */
#if defined(LL_DMA_CHANNEL_7)
	LL_DMA_CHANNEL_7,
#endif /* LL_DMA_CHANNEL_7 */
#if defined(LL_DMA_CHANNEL_8)
	LL_DMA_CHANNEL_8,
#endif
};

void (*func_ll_clear_ht[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_HT1,
	LL_DMA_ClearFlag_HT2,
	LL_DMA_ClearFlag_HT3,
	LL_DMA_ClearFlag_HT4,
	LL_DMA_ClearFlag_HT5,
#if defined(LL_DMA_IFCR_CHTIF6)
	LL_DMA_ClearFlag_HT6,
#endif /* LL_DMA_IFCR_CHTIF6 */
#if defined(LL_DMA_IFCR_CHTIF7)
	LL_DMA_ClearFlag_HT7,
#endif /* LL_DMA_IFCR_CHTIF7 */
#if defined(LL_DMA_IFCR_CHTIF8)
	LL_DMA_ClearFlag_HT8,
#endif /* LL_DMA_IFCR_CHTIF8 */
};

void (*func_ll_clear_tc[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_TC1,
	LL_DMA_ClearFlag_TC2,
	LL_DMA_ClearFlag_TC3,
	LL_DMA_ClearFlag_TC4,
	LL_DMA_ClearFlag_TC5,
#if defined(LL_DMA_IFCR_CTCIF6)
	LL_DMA_ClearFlag_TC6,
#endif /* LL_DMA_IFCR_CTCIF6 */
#if defined(LL_DMA_IFCR_CTCIF7)
	LL_DMA_ClearFlag_TC7,
#endif /* LL_DMA_IFCR_CTCIF7 */
#if defined(LL_DMA_IFCR_CTCIF8)
	LL_DMA_ClearFlag_TC8,
#endif /* LL_DMA_IFCR_CTCIF8 */
};

u32_t (*func_ll_is_active_ht[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_HT1,
	LL_DMA_IsActiveFlag_HT2,
	LL_DMA_IsActiveFlag_HT3,
	LL_DMA_IsActiveFlag_HT4,
	LL_DMA_IsActiveFlag_HT5,
#if defined(LL_DMA_ISR_HTIF6)
	LL_DMA_IsActiveFlag_HT6,
#endif /* LL_DMA_ISR_HTIF6 */
#if defined(LL_DMA_ISR_HTIF7)
	LL_DMA_IsActiveFlag_HT7,
#endif /* LL_DMA_ISR_HTIF7 */
#if defined(LL_DMA_ISR_HTIF8)
	LL_DMA_IsActiveFlag_HT8,
#endif /* LL_DMA_ISR_HTIF8 */
};

u32_t (*func_ll_is_active_tc[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_TC1,
	LL_DMA_IsActiveFlag_TC2,
	LL_DMA_IsActiveFlag_TC3,
	LL_DMA_IsActiveFlag_TC4,
	LL_DMA_IsActiveFlag_TC5,
#if defined(LL_DMA_ISR_TCIF6)
	LL_DMA_IsActiveFlag_TC6,
#endif /* LL_DMA_ISR_TCIF6 */
#if defined(LL_DMA_ISR_TCIF7)
	LL_DMA_IsActiveFlag_TC7,
#endif /* LL_DMA_ISR_TCIF7 */
#if defined(LL_DMA_ISR_TCIF8)
	LL_DMA_IsActiveFlag_TC8,
#endif /* LL_DMA_ISR_TCIF8 */
};

static void (*func_ll_clear_te[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_TE1,
	LL_DMA_ClearFlag_TE2,
	LL_DMA_ClearFlag_TE3,
	LL_DMA_ClearFlag_TE4,
	LL_DMA_ClearFlag_TE5,
#if defined(LL_DMA_IFCR_CTEIF6)
	LL_DMA_ClearFlag_TE6,
#endif /* LL_DMA_IFCR_CTEIF6 */
#if defined(LL_DMA_IFCR_CTEIF7)
	LL_DMA_ClearFlag_TE7,
#endif /* LL_DMA_IFCR_CTEIF7 */
#if defined(LL_DMA_IFCR_CTEIF8)
	LL_DMA_ClearFlag_TE8,
#endif /* LL_DMA_IFCR_CTEIF8 */
};

void (*func_ll_clear_gi[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_GI1,
	LL_DMA_ClearFlag_GI2,
	LL_DMA_ClearFlag_GI3,
	LL_DMA_ClearFlag_GI4,
	LL_DMA_ClearFlag_GI5,
#if defined(LL_DMA_IFCR_CGIF6)
	LL_DMA_ClearFlag_GI6,
#endif /* LL_DMA_IFCR_CGIF6 */
#if defined(LL_DMA_IFCR_CGIF7)
	LL_DMA_ClearFlag_GI7,
#endif /* LL_DMA_IFCR_CGIF7 */
#if defined(LL_DMA_IFCR_CGIF8)
	LL_DMA_ClearFlag_GI8,
#endif /* LL_DMA_IFCR_CGIF8 */
};

static u32_t (*func_ll_is_active_te[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_TE1,
	LL_DMA_IsActiveFlag_TE2,
	LL_DMA_IsActiveFlag_TE3,
	LL_DMA_IsActiveFlag_TE4,
	LL_DMA_IsActiveFlag_TE5,
#if defined(LL_DMA_IFCR_CTEIF6)
	LL_DMA_IsActiveFlag_TE6,
#endif /* LL_DMA_IFCR_CTEIF6 */
#if defined(LL_DMA_IFCR_CTEIF7)
	LL_DMA_IsActiveFlag_TE7,
#endif /* LL_DMA_IFCR_CTEIF7 */
#if defined(LL_DMA_IFCR_CTEIF8)
	LL_DMA_IsActiveFlag_TE8,
#endif /* LL_DMA_IFCR_CTEIF8 */
};


u32_t (*func_ll_is_active_gi[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_GI1,
	LL_DMA_IsActiveFlag_GI2,
	LL_DMA_IsActiveFlag_GI3,
	LL_DMA_IsActiveFlag_GI4,
	LL_DMA_IsActiveFlag_GI5,
#if defined(LL_DMA_IFCR_CGIF6)
	LL_DMA_IsActiveFlag_GI6,
#endif /* LL_DMA_IFCR_CGIF6 */
#if defined(LL_DMA_IFCR_CGIF7)
	LL_DMA_IsActiveFlag_GI7,
#endif /* LL_DMA_IFCR_CGIF7 */
#if defined(LL_DMA_IFCR_CGIF8)
	LL_DMA_IsActiveFlag_GI8,
#endif /* LL_DMA_IFCR_CGIF8 */

};

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, u32_t id)
{
	LOG_INF("tc: %d, ht: %d, te: %d, gi: %d",
		func_ll_is_active_tc[id](dma),
		func_ll_is_active_ht[id](dma),
		func_ll_is_active_te[id](dma),
		func_ll_is_active_gi[id](dma));
}

void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, u32_t id)
{
	func_ll_clear_te[id](dma);
}

bool stm32_dma_is_irq_happened(DMA_TypeDef *dma, u32_t id)
{
	if (func_ll_is_active_te[id](dma)) {
		return true;
	}

	return false;
}

bool stm32_dma_is_unexpected_irq_happened(DMA_TypeDef *dma, u32_t id)
{
	/* Preserve for future amending. */
	return false;
}

void stm32_dma_enable_stream(DMA_TypeDef *dma, u32_t id)
{
	LL_DMA_EnableChannel(dma, table_ll_stream[id]);
}

int stm32_dma_disable_stream(DMA_TypeDef *dma, u32_t id)
{
	if (!LL_DMA_IsEnabledChannel(dma, table_ll_stream[id])) {
		return 0;
	}
	LL_DMA_DisableChannel(dma, table_ll_stream[id]);

	return -EAGAIN;
}
