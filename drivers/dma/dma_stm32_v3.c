/*
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA low level driver implementation for U5 series SoCs.
 */

#include "dma_stm32.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_stm32_v3);


uint32_t dma_stm32_id_to_stream(uint32_t id)
{
	static const uint32_t stream_nr[] = {
		LL_DMA_CHANNEL_0,
		LL_DMA_CHANNEL_1,
		LL_DMA_CHANNEL_2,
		LL_DMA_CHANNEL_3,
		LL_DMA_CHANNEL_4,
		LL_DMA_CHANNEL_5,
		LL_DMA_CHANNEL_6,
		LL_DMA_CHANNEL_7,
		LL_DMA_CHANNEL_8,
		LL_DMA_CHANNEL_9,
		LL_DMA_CHANNEL_10,
		LL_DMA_CHANNEL_11,
		LL_DMA_CHANNEL_12,
		LL_DMA_CHANNEL_13,
		LL_DMA_CHANNEL_14,
		LL_DMA_CHANNEL_15,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(stream_nr));

	return stream_nr[id];
}

bool dma_stm32_is_tc_active(DMA_TypeDef *DMAx, uint32_t id)
{
	return LL_DMA_IsActiveFlag_TC(DMAx, dma_stm32_id_to_stream(id));
}

void dma_stm32_clear_tc(DMA_TypeDef *DMAx, uint32_t id)
{
	LL_DMA_ClearFlag_TC(DMAx, dma_stm32_id_to_stream(id));
}

/* transfer error either a data or user or link error */
bool dma_stm32_is_te_active(DMA_TypeDef *DMAx, uint32_t id)
{
	return (
	LL_DMA_IsActiveFlag_DTE(DMAx, dma_stm32_id_to_stream(id)) ||
		LL_DMA_IsActiveFlag_ULE(DMAx, dma_stm32_id_to_stream(id)) ||
		LL_DMA_IsActiveFlag_USE(DMAx, dma_stm32_id_to_stream(id))
	);
}
/* clear transfer error either a data or user or link error */
void dma_stm32_clear_te(DMA_TypeDef *DMAx, uint32_t id)
{
	LL_DMA_ClearFlag_DTE(DMAx, dma_stm32_id_to_stream(id));
	LL_DMA_ClearFlag_ULE(DMAx, dma_stm32_id_to_stream(id));
	LL_DMA_ClearFlag_USE(DMAx, dma_stm32_id_to_stream(id));
}

bool dma_stm32_is_ht_active(DMA_TypeDef *DMAx, uint32_t id)
{
	return LL_DMA_IsActiveFlag_HT(DMAx, dma_stm32_id_to_stream(id));
}

void dma_stm32_clear_ht(DMA_TypeDef *DMAx, uint32_t id)
{
	LL_DMA_ClearFlag_HT(DMAx, dma_stm32_id_to_stream(id));
}

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	LOG_INF("tc: %d, ht: %d, te: %d",
		dma_stm32_is_tc_active(dma, id),
		dma_stm32_is_ht_active(dma, id),
		dma_stm32_is_te_active(dma, id));
}

/* Check if nsecure masked interrupt is active on channel */
bool stm32_dma_is_tc_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return (LL_DMA_IsEnabledIT_TC(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_TC(dma, dma_stm32_id_to_stream(id)));
}

bool stm32_dma_is_ht_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return (LL_DMA_IsEnabledIT_HT(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_HT(dma, dma_stm32_id_to_stream(id)));
}

static inline bool stm32_dma_is_te_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return (
		(LL_DMA_IsEnabledIT_DTE(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_DTE(dma, dma_stm32_id_to_stream(id))) ||
		(LL_DMA_IsEnabledIT_ULE(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_ULE(dma, dma_stm32_id_to_stream(id))) ||
		(LL_DMA_IsEnabledIT_USE(dma, dma_stm32_id_to_stream(id)) &&
		LL_DMA_IsActiveFlag_USE(dma, dma_stm32_id_to_stream(id)))
		);
}

/* check if and irq of any type occurred on the channel */
#define stm32_dma_is_irq_active LL_DMA_IsActiveFlag_MIS

void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	dma_stm32_clear_te(dma, id);
}

bool stm32_dma_is_irq_happened(DMA_TypeDef *dma, uint32_t id)
{
	if (dma_stm32_is_te_active(dma, id)) {
		return true;
	}

	return false;
}

bool stm32_dma_is_unexpected_irq_happened(DMA_TypeDef *dma, uint32_t id)
{
	/* Preserve for future amending. */
	return false;
}

void stm32_dma_enable_stream(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_EnableChannel(dma, dma_stm32_id_to_stream(id));
}

int stm32_dma_disable_stream(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_DisableChannel(dma, dma_stm32_id_to_stream(id));

	if (!LL_DMA_IsEnabledChannel(dma, dma_stm32_id_to_stream(id))) {
		return 0;
	}

	return -EAGAIN;
}
