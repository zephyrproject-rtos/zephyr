/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA low level driver implementation for F2/F4/F7 series SoCs.
 */

#include "dma_stm32.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_stm32_v1);

/* DMA burst length */
#define BURST_TRANS_LENGTH_1			0

uint32_t dma_stm32_id_to_stream(uint32_t id)
{
	static const uint32_t stream_nr[] = {
		LL_DMA_STREAM_0,
		LL_DMA_STREAM_1,
		LL_DMA_STREAM_2,
		LL_DMA_STREAM_3,
		LL_DMA_STREAM_4,
		LL_DMA_STREAM_5,
		LL_DMA_STREAM_6,
		LL_DMA_STREAM_7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(stream_nr));

	return stream_nr[id];
}

uint32_t dma_stm32_slot_to_channel(uint32_t slot)
{
	static const uint32_t channel_nr[] = {
		LL_DMA_CHANNEL_0,
		LL_DMA_CHANNEL_1,
		LL_DMA_CHANNEL_2,
		LL_DMA_CHANNEL_3,
		LL_DMA_CHANNEL_4,
		LL_DMA_CHANNEL_5,
		LL_DMA_CHANNEL_6,
		LL_DMA_CHANNEL_7,
	};

	__ASSERT_NO_MSG(slot < ARRAY_SIZE(channel_nr));

	return channel_nr[slot];
}

void dma_stm32_clear_ht(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_HT0,
		LL_DMA_ClearFlag_HT1,
		LL_DMA_ClearFlag_HT2,
		LL_DMA_ClearFlag_HT3,
		LL_DMA_ClearFlag_HT4,
		LL_DMA_ClearFlag_HT5,
		LL_DMA_ClearFlag_HT6,
		LL_DMA_ClearFlag_HT7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

void dma_stm32_clear_tc(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_TC0,
		LL_DMA_ClearFlag_TC1,
		LL_DMA_ClearFlag_TC2,
		LL_DMA_ClearFlag_TC3,
		LL_DMA_ClearFlag_TC4,
		LL_DMA_ClearFlag_TC5,
		LL_DMA_ClearFlag_TC6,
		LL_DMA_ClearFlag_TC7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

bool dma_stm32_is_ht_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_HT0,
		LL_DMA_IsActiveFlag_HT1,
		LL_DMA_IsActiveFlag_HT2,
		LL_DMA_IsActiveFlag_HT3,
		LL_DMA_IsActiveFlag_HT4,
		LL_DMA_IsActiveFlag_HT5,
		LL_DMA_IsActiveFlag_HT6,
		LL_DMA_IsActiveFlag_HT7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

bool dma_stm32_is_tc_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_TC0,
		LL_DMA_IsActiveFlag_TC1,
		LL_DMA_IsActiveFlag_TC2,
		LL_DMA_IsActiveFlag_TC3,
		LL_DMA_IsActiveFlag_TC4,
		LL_DMA_IsActiveFlag_TC5,
		LL_DMA_IsActiveFlag_TC6,
		LL_DMA_IsActiveFlag_TC7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

void dma_stm32_clear_te(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_TE0,
		LL_DMA_ClearFlag_TE1,
		LL_DMA_ClearFlag_TE2,
		LL_DMA_ClearFlag_TE3,
		LL_DMA_ClearFlag_TE4,
		LL_DMA_ClearFlag_TE5,
		LL_DMA_ClearFlag_TE6,
		LL_DMA_ClearFlag_TE7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

void dma_stm32_clear_dme(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_DME0,
		LL_DMA_ClearFlag_DME1,
		LL_DMA_ClearFlag_DME2,
		LL_DMA_ClearFlag_DME3,
		LL_DMA_ClearFlag_DME4,
		LL_DMA_ClearFlag_DME5,
		LL_DMA_ClearFlag_DME6,
		LL_DMA_ClearFlag_DME7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

void dma_stm32_clear_fe(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_clear_flag_func func[] = {
		LL_DMA_ClearFlag_FE0,
		LL_DMA_ClearFlag_FE1,
		LL_DMA_ClearFlag_FE2,
		LL_DMA_ClearFlag_FE3,
		LL_DMA_ClearFlag_FE4,
		LL_DMA_ClearFlag_FE5,
		LL_DMA_ClearFlag_FE6,
		LL_DMA_ClearFlag_FE7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	func[id](DMAx);
}

bool dma_stm32_is_te_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_TE0,
		LL_DMA_IsActiveFlag_TE1,
		LL_DMA_IsActiveFlag_TE2,
		LL_DMA_IsActiveFlag_TE3,
		LL_DMA_IsActiveFlag_TE4,
		LL_DMA_IsActiveFlag_TE5,
		LL_DMA_IsActiveFlag_TE6,
		LL_DMA_IsActiveFlag_TE7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

bool dma_stm32_is_dme_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_DME0,
		LL_DMA_IsActiveFlag_DME1,
		LL_DMA_IsActiveFlag_DME2,
		LL_DMA_IsActiveFlag_DME3,
		LL_DMA_IsActiveFlag_DME4,
		LL_DMA_IsActiveFlag_DME5,
		LL_DMA_IsActiveFlag_DME6,
		LL_DMA_IsActiveFlag_DME7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

bool dma_stm32_is_fe_active(DMA_TypeDef *DMAx, uint32_t id)
{
	static const dma_stm32_check_flag_func func[] = {
		LL_DMA_IsActiveFlag_FE0,
		LL_DMA_IsActiveFlag_FE1,
		LL_DMA_IsActiveFlag_FE2,
		LL_DMA_IsActiveFlag_FE3,
		LL_DMA_IsActiveFlag_FE4,
		LL_DMA_IsActiveFlag_FE5,
		LL_DMA_IsActiveFlag_FE6,
		LL_DMA_IsActiveFlag_FE7,
	};

	__ASSERT_NO_MSG(id < ARRAY_SIZE(func));

	return func[id](DMAx);
}

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	LOG_INF("tc: %d, ht: %d, te: %d, dme: %d, fe: %d",
		dma_stm32_is_tc_active(dma, id),
		dma_stm32_is_ht_active(dma, id),
		dma_stm32_is_te_active(dma, id),
		dma_stm32_is_dme_active(dma, id),
		dma_stm32_is_fe_active(dma, id));
}

static inline bool stm32_dma_is_tc_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_TC(dma, id) &&
	       dma_stm32_is_tc_active(dma, id);
}

static inline bool stm32_dma_is_ht_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_HT(dma, id) &&
	       dma_stm32_is_ht_active(dma, id);
}

static inline bool stm32_dma_is_te_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_TE(dma, id) &&
	       dma_stm32_is_te_active(dma, id);
}

static inline bool stm32_dma_is_dme_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_DME(dma, id) &&
	       dma_stm32_is_dme_active(dma, id);
}

static inline bool stm32_dma_is_fe_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return LL_DMA_IsEnabledIT_FE(dma, id) &&
	       dma_stm32_is_fe_active(dma, id);
}

bool stm32_dma_is_irq_active(DMA_TypeDef *dma, uint32_t id)
{
	return stm32_dma_is_tc_irq_active(dma, id) ||
	       stm32_dma_is_ht_irq_active(dma, id) ||
	       stm32_dma_is_te_irq_active(dma, id) ||
	       stm32_dma_is_dme_irq_active(dma, id) ||
	       stm32_dma_is_fe_irq_active(dma, id);
}

void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, uint32_t id)
{
	dma_stm32_clear_te(dma, id);
	dma_stm32_clear_dme(dma, id);
	dma_stm32_clear_fe(dma, id);
}

bool stm32_dma_is_irq_happened(DMA_TypeDef *dma, uint32_t id)
{
	if (dma_stm32_is_fe_active(dma, id) && LL_DMA_IsEnabledIT_FE(dma, id)) {
		return true;
	}

	return false;
}

bool stm32_dma_is_unexpected_irq_happened(DMA_TypeDef *dma, uint32_t id)
{
	if (dma_stm32_is_fe_active(dma, id) && LL_DMA_IsEnabledIT_FE(dma, id)) {
		LOG_ERR("FiFo error.");
		stm32_dma_dump_stream_irq(dma, id);
		stm32_dma_clear_stream_irq(dma, id);

		return true;
	}

	return false;
}

void stm32_dma_enable_stream(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_EnableStream(dma, dma_stm32_id_to_stream(id));
}

int stm32_dma_disable_stream(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_DisableStream(dma, dma_stm32_id_to_stream(id));

	if (!LL_DMA_IsEnabledStream(dma, dma_stm32_id_to_stream(id))) {
		return 0;
	}

	return -EAGAIN;
}

void stm32_dma_disable_fifo_irq(DMA_TypeDef *dma, uint32_t id)
{
	LL_DMA_DisableIT_FE(dma, dma_stm32_id_to_stream(id));
}

void stm32_dma_config_channel_function(DMA_TypeDef *dma, uint32_t id, uint32_t slot)
{
	LL_DMA_SetChannelSelection(dma, dma_stm32_id_to_stream(id),
			dma_stm32_slot_to_channel(slot));
}

uint32_t stm32_dma_get_mburst(struct dma_config *config, bool source_periph)
{
	uint32_t memory_burst;

	if (source_periph) {
		memory_burst = config->dest_burst_length;
	} else {
		memory_burst = config->source_burst_length;
	}

	switch (memory_burst) {
	case 1:
		return LL_DMA_MBURST_SINGLE;
	case 4:
		return LL_DMA_MBURST_INC4;
	case 8:
		return LL_DMA_MBURST_INC8;
	case 16:
		return LL_DMA_MBURST_INC16;
	default:
		LOG_ERR("Memory burst size error,"
			"using single burst as default");
		return LL_DMA_MBURST_SINGLE;
	}
}

uint32_t stm32_dma_get_pburst(struct dma_config *config, bool source_periph)
{
	uint32_t periph_burst;

	if (source_periph) {
		periph_burst = config->source_burst_length;
	} else {
		periph_burst = config->dest_burst_length;
	}

	switch (periph_burst) {
	case 1:
		return LL_DMA_PBURST_SINGLE;
	case 4:
		return LL_DMA_PBURST_INC4;
	case 8:
		return LL_DMA_PBURST_INC8;
	case 16:
		return LL_DMA_PBURST_INC16;
	default:
		LOG_ERR("Peripheral burst size error,"
			"using single burst as default");
		return LL_DMA_PBURST_SINGLE;
	}
}

/*
 * This function checks if the msize, mburst and fifo level is
 * compitable. If they are not compitable, refer to the 'FIFO'
 * section in the 'DMA' chapter in the Reference Manual for more
 * information.
 * break is emitted since every path of the code has 'return'.
 * This function does not have the obligation of checking the parameters.
 */
bool stm32_dma_check_fifo_mburst(LL_DMA_InitTypeDef *DMAx)
{
	uint32_t msize = DMAx->MemoryOrM2MDstDataSize;
	uint32_t fifo_level = DMAx->FIFOThreshold;
	uint32_t mburst = DMAx->MemBurst;

	switch (msize) {
	case LL_DMA_MDATAALIGN_BYTE:
		switch (mburst) {
		case LL_DMA_MBURST_INC4:
			return true;
		case LL_DMA_MBURST_INC8:
			if (fifo_level == LL_DMA_FIFOTHRESHOLD_1_2 ||
			    fifo_level == LL_DMA_FIFOTHRESHOLD_FULL) {
				return true;
			} else {
				return false;
			}
		case LL_DMA_MBURST_INC16:
			if (fifo_level == LL_DMA_FIFOTHRESHOLD_FULL) {
				return true;
			} else {
				return false;
			}
		}
	case LL_DMA_MDATAALIGN_HALFWORD:
		switch (mburst) {
		case LL_DMA_MBURST_INC4:
			if (fifo_level == LL_DMA_FIFOTHRESHOLD_1_2 ||
			    fifo_level == LL_DMA_FIFOTHRESHOLD_FULL) {
				return true;
			} else {
				return false;
			}
		case LL_DMA_MBURST_INC8:
			if (fifo_level == LL_DMA_FIFOTHRESHOLD_FULL) {
				return true;
			} else {
				return false;
			}
		case LL_DMA_MBURST_INC16:
			return false;
		}
	case LL_DMA_MDATAALIGN_WORD:
		if (mburst == LL_DMA_MBURST_INC4 &&
		    fifo_level == LL_DMA_FIFOTHRESHOLD_FULL) {
			return true;
		} else {
			return false;
		}
	default:
		return false;
	}
}

uint32_t stm32_dma_get_fifo_threshold(uint16_t fifo_mode_control)
{
	switch (fifo_mode_control) {
	case 0:
		return LL_DMA_FIFOTHRESHOLD_1_4;
	case 1:
		return LL_DMA_FIFOTHRESHOLD_1_2;
	case 2:
		return LL_DMA_FIFOTHRESHOLD_3_4;
	case 3:
		return LL_DMA_FIFOTHRESHOLD_FULL;
	default:
		LOG_WRN("FIFO threshold parameter error, reset to 1/4");
		return LL_DMA_FIFOTHRESHOLD_1_4;
	}
}
