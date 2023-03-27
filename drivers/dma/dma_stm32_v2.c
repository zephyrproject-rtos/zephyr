/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA low level driver implementation for F0/F1/F3/L0/L4 series SoCs.
 */

#include "dma_stm32.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_stm32_v2);


uint32_t dma_stm32_id_to_stream(uint32_t id)
{
	static const uint32_t stream_nr[] = {
		LL_DMA_CHANNEL_1,
		LL_DMA_CHANNEL_2,
		LL_DMA_CHANNEL_3,
		LL_DMA_CHANNEL_4,
		LL_DMA_CHANNEL_5,
#if defined(LL_DMA_CHANNEL_6)
		LL_DMA_CHANNEL_6,
#if defined(LL_DMA_CHANNEL_7)
		LL_DMA_CHANNEL_7,
#if defined(LL_DMA_CHANNEL_8)
		LL_DMA_CHANNEL_8,
#endif /* LL_DMA_CHANNEL_8 */
#endif /* LL_DMA_CHANNEL_7 */
#endif /* LL_DMA_CHANNEL_6 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(stream_nr));

	return stream_nr[id];
}

void dma_stm32_clear_ht(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_HT1,
		LL_DMA_ClearFlag_HT2,
		LL_DMA_ClearFlag_HT3,
		LL_DMA_ClearFlag_HT4,
		LL_DMA_ClearFlag_HT5,
#if defined(LL_DMA_IFCR_CHTIF6)
		LL_DMA_ClearFlag_HT6,
#if defined(LL_DMA_IFCR_CHTIF7)
		LL_DMA_ClearFlag_HT7,
#if defined(LL_DMA_IFCR_CHTIF8)
		LL_DMA_ClearFlag_HT8,
#endif /* LL_DMA_IFCR_CHTIF8 */
#endif /* LL_DMA_IFCR_CHTIF7 */
#endif /* LL_DMA_IFCR_CHTIF6 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

void dma_stm32_clear_tc(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_TC1,
		LL_DMA_ClearFlag_TC2,
		LL_DMA_ClearFlag_TC3,
		LL_DMA_ClearFlag_TC4,
		LL_DMA_ClearFlag_TC5,
#if defined(LL_DMA_IFCR_CTCIF6)
		LL_DMA_ClearFlag_TC6,
#if defined(LL_DMA_IFCR_CTCIF7)
		LL_DMA_ClearFlag_TC7,
#if defined(LL_DMA_IFCR_CTCIF8)
		LL_DMA_ClearFlag_TC8,
#endif /* LL_DMA_IFCR_CTCIF8 */
#endif /* LL_DMA_IFCR_CTCIF7 */
#endif /* LL_DMA_IFCR_CTCIF6 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

bool dma_stm32_is_ht_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_HT1,
		LL_DMA_IsActiveFlag_HT2,
		LL_DMA_IsActiveFlag_HT3,
		LL_DMA_IsActiveFlag_HT4,
		LL_DMA_IsActiveFlag_HT5,
#if defined(LL_DMA_IFCR_CHTIF6)
		LL_DMA_IsActiveFlag_HT6,
#if defined(LL_DMA_IFCR_CHTIF7)
		LL_DMA_IsActiveFlag_HT7,
#if defined(LL_DMA_IFCR_CHTIF8)
		LL_DMA_IsActiveFlag_HT8,
#endif /* LL_DMA_IFCR_CHTIF8 */
#endif /* LL_DMA_IFCR_CHTIF7 */
#endif /* LL_DMA_IFCR_CHTIF6 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

bool dma_stm32_is_tc_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_TC1,
		LL_DMA_IsActiveFlag_TC2,
		LL_DMA_IsActiveFlag_TC3,
		LL_DMA_IsActiveFlag_TC4,
		LL_DMA_IsActiveFlag_TC5,
#if defined(LL_DMA_IFCR_CTCIF6)
		LL_DMA_IsActiveFlag_TC6,
#if defined(LL_DMA_IFCR_CTCIF7)
		LL_DMA_IsActiveFlag_TC7,
#if defined(LL_DMA_IFCR_CTCIF8)
		LL_DMA_IsActiveFlag_TC8,
#endif /* LL_DMA_IFCR_CTCIF8 */
#endif /* LL_DMA_IFCR_CTCIF7 */
#endif /* LL_DMA_IFCR_CTCIF6 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}


void dma_stm32_clear_te(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_TE1,
		LL_DMA_ClearFlag_TE2,
		LL_DMA_ClearFlag_TE3,
		LL_DMA_ClearFlag_TE4,
		LL_DMA_ClearFlag_TE5,
#if defined(LL_DMA_IFCR_CTEIF6)
		LL_DMA_ClearFlag_TE6,
#if defined(LL_DMA_IFCR_CTEIF7)
		LL_DMA_ClearFlag_TE7,
#if defined(LL_DMA_IFCR_CTEIF8)
		LL_DMA_ClearFlag_TE8,
#endif /* LL_DMA_IFCR_CTEIF6 */
#endif /* LL_DMA_IFCR_CTEIF7 */
#endif /* LL_DMA_IFCR_CTEIF8 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

void dma_stm32_clear_gi(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_GI1,
		LL_DMA_ClearFlag_GI2,
		LL_DMA_ClearFlag_GI3,
		LL_DMA_ClearFlag_GI4,
		LL_DMA_ClearFlag_GI5,
#if defined(LL_DMA_IFCR_CGIF6)
		LL_DMA_ClearFlag_GI6,
#if defined(LL_DMA_IFCR_CGIF7)
		LL_DMA_ClearFlag_GI7,
#if defined(LL_DMA_IFCR_CGIF8)
		LL_DMA_ClearFlag_GI8,
#endif /* LL_DMA_IFCR_CGIF6 */
#endif /* LL_DMA_IFCR_CGIF7 */
#endif /* LL_DMA_IFCR_CGIF8 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

bool dma_stm32_is_te_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_TE1,
		LL_DMA_IsActiveFlag_TE2,
		LL_DMA_IsActiveFlag_TE3,
		LL_DMA_IsActiveFlag_TE4,
		LL_DMA_IsActiveFlag_TE5,
#if defined(LL_DMA_IFCR_CTEIF6)
		LL_DMA_IsActiveFlag_TE6,
#if defined(LL_DMA_IFCR_CTEIF7)
		LL_DMA_IsActiveFlag_TE7,
#if defined(LL_DMA_IFCR_CTEIF8)
		LL_DMA_IsActiveFlag_TE8,
#endif /* LL_DMA_IFCR_CTEIF6 */
#endif /* LL_DMA_IFCR_CTEIF7 */
#endif /* LL_DMA_IFCR_CTEIF8 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

bool dma_stm32_is_gi_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_GI1,
		LL_DMA_IsActiveFlag_GI2,
		LL_DMA_IsActiveFlag_GI3,
		LL_DMA_IsActiveFlag_GI4,
		LL_DMA_IsActiveFlag_GI5,
#if defined(LL_DMA_IFCR_CGIF6)
		LL_DMA_IsActiveFlag_GI6,
#if defined(LL_DMA_IFCR_CGIF7)
		LL_DMA_IsActiveFlag_GI7,
#if defined(LL_DMA_IFCR_CGIF8)
		LL_DMA_IsActiveFlag_GI8,
#endif /* LL_DMA_IFCR_CGIF6 */
#endif /* LL_DMA_IFCR_CGIF7 */
#endif /* LL_DMA_IFCR_CGIF8 */
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	LOG_INF("tc: %d, ht: %d, te: %d, gi: %d",
		dma_stm32_is_tc_active(dma, id),
		dma_stm32_is_ht_active(dma, id),
		dma_stm32_is_te_active(dma, id),
		dma_stm32_is_gi_active(dma, id));
}

bool stm32_dma_is_tc_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_TC(dma, dma_stm32_id_to_stream(id)) &&
	       dma_stm32_is_tc_active(dma, id);
}

bool stm32_dma_is_ht_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_HT(dma, dma_stm32_id_to_stream(id)) &&
	       dma_stm32_is_ht_active(dma, id);
}

static inline bool stm32_dma_is_te_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_TE(dma, dma_stm32_id_to_stream(id)) &&
	       dma_stm32_is_te_active(dma, id);
}

bool stm32_dma_is_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return stm32_dma_is_tc_irq_active(dma, id) ||
	       stm32_dma_is_ht_irq_active(dma, id) ||
	       stm32_dma_is_te_irq_active(dma, id);
}

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

bool stm32_dma_is_enabled_stream(DMA_TypeDef *dma, uint32_t id)
{
	if (LL_DMA_IsEnabledChannel(dma, dma_stm32_id_to_stream(id)) == 1) {
		return true;
	}
	return false;
}

int stm32_dma_disable_stream(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_DisableChannel(dma, dma_stm32_id_to_stream(id));

	if (!LL_DMA_IsEnabledChannel(dma, dma_stm32_id_to_stream(id))) {
		return 0;
	}

	return -EAGAIN;
}
