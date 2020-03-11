/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA low level driver implementation for F2/F4/F7 series SoCs.
 */

#include <drivers/dma.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_stm32_v1);

/* DMA burst length */
#define BURST_TRANS_LENGTH_1			0

u32_t table_ll_stream[] = {
	LL_DMA_STREAM_0,
	LL_DMA_STREAM_1,
	LL_DMA_STREAM_2,
	LL_DMA_STREAM_3,
	LL_DMA_STREAM_4,
	LL_DMA_STREAM_5,
	LL_DMA_STREAM_6,
	LL_DMA_STREAM_7,
};

u32_t table_ll_channel[] = {
	LL_DMA_CHANNEL_0,
	LL_DMA_CHANNEL_1,
	LL_DMA_CHANNEL_2,
	LL_DMA_CHANNEL_3,
	LL_DMA_CHANNEL_4,
	LL_DMA_CHANNEL_5,
	LL_DMA_CHANNEL_6,
	LL_DMA_CHANNEL_7,
};

void (*func_ll_clear_ht[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_HT0,
	LL_DMA_ClearFlag_HT1,
	LL_DMA_ClearFlag_HT2,
	LL_DMA_ClearFlag_HT3,
	LL_DMA_ClearFlag_HT4,
	LL_DMA_ClearFlag_HT5,
	LL_DMA_ClearFlag_HT6,
	LL_DMA_ClearFlag_HT7,
};

void (*func_ll_clear_tc[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_TC0,
	LL_DMA_ClearFlag_TC1,
	LL_DMA_ClearFlag_TC2,
	LL_DMA_ClearFlag_TC3,
	LL_DMA_ClearFlag_TC4,
	LL_DMA_ClearFlag_TC5,
	LL_DMA_ClearFlag_TC6,
	LL_DMA_ClearFlag_TC7,
};

u32_t (*func_ll_is_active_ht[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_HT0,
	LL_DMA_IsActiveFlag_HT1,
	LL_DMA_IsActiveFlag_HT2,
	LL_DMA_IsActiveFlag_HT3,
	LL_DMA_IsActiveFlag_HT4,
	LL_DMA_IsActiveFlag_HT5,
	LL_DMA_IsActiveFlag_HT6,
	LL_DMA_IsActiveFlag_HT7,
};

u32_t (*func_ll_is_active_tc[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_TC0,
	LL_DMA_IsActiveFlag_TC1,
	LL_DMA_IsActiveFlag_TC2,
	LL_DMA_IsActiveFlag_TC3,
	LL_DMA_IsActiveFlag_TC4,
	LL_DMA_IsActiveFlag_TC5,
	LL_DMA_IsActiveFlag_TC6,
	LL_DMA_IsActiveFlag_TC7,
};

static void (*func_ll_clear_te[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_TE0,
	LL_DMA_ClearFlag_TE1,
	LL_DMA_ClearFlag_TE2,
	LL_DMA_ClearFlag_TE3,
	LL_DMA_ClearFlag_TE4,
	LL_DMA_ClearFlag_TE5,
	LL_DMA_ClearFlag_TE6,
	LL_DMA_ClearFlag_TE7,
};

static void (*func_ll_clear_dme[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_DME0,
	LL_DMA_ClearFlag_DME1,
	LL_DMA_ClearFlag_DME2,
	LL_DMA_ClearFlag_DME3,
	LL_DMA_ClearFlag_DME4,
	LL_DMA_ClearFlag_DME5,
	LL_DMA_ClearFlag_DME6,
	LL_DMA_ClearFlag_DME7,
};

static void (*func_ll_clear_fe[])(DMA_TypeDef *DMAx) = {
	LL_DMA_ClearFlag_FE0,
	LL_DMA_ClearFlag_FE1,
	LL_DMA_ClearFlag_FE2,
	LL_DMA_ClearFlag_FE3,
	LL_DMA_ClearFlag_FE4,
	LL_DMA_ClearFlag_FE5,
	LL_DMA_ClearFlag_FE6,
	LL_DMA_ClearFlag_FE7,
};

static u32_t (*func_ll_is_active_te[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_TE0,
	LL_DMA_IsActiveFlag_TE1,
	LL_DMA_IsActiveFlag_TE2,
	LL_DMA_IsActiveFlag_TE3,
	LL_DMA_IsActiveFlag_TE4,
	LL_DMA_IsActiveFlag_TE5,
	LL_DMA_IsActiveFlag_TE6,
	LL_DMA_IsActiveFlag_TE7,
};

static u32_t (*func_ll_is_active_dme[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_DME0,
	LL_DMA_IsActiveFlag_DME1,
	LL_DMA_IsActiveFlag_DME2,
	LL_DMA_IsActiveFlag_DME3,
	LL_DMA_IsActiveFlag_DME4,
	LL_DMA_IsActiveFlag_DME5,
	LL_DMA_IsActiveFlag_DME6,
	LL_DMA_IsActiveFlag_DME7,
};

static u32_t (*func_ll_is_active_fe[])(DMA_TypeDef *DMAx) = {
	LL_DMA_IsActiveFlag_FE0,
	LL_DMA_IsActiveFlag_FE1,
	LL_DMA_IsActiveFlag_FE2,
	LL_DMA_IsActiveFlag_FE3,
	LL_DMA_IsActiveFlag_FE4,
	LL_DMA_IsActiveFlag_FE5,
	LL_DMA_IsActiveFlag_FE6,
	LL_DMA_IsActiveFlag_FE7,
};

void stm32_dma_dump_stream_irq(DMA_TypeDef *dma, u32_t id)
{
	LOG_INF("tc: %d, ht: %d, te: %d, dme: %d, fe: %d",
		func_ll_is_active_tc[id](dma),
		func_ll_is_active_ht[id](dma),
		func_ll_is_active_te[id](dma),
		func_ll_is_active_dme[id](dma),
		func_ll_is_active_fe[id](dma));
}

void stm32_dma_clear_stream_irq(DMA_TypeDef *dma, u32_t id)
{
	func_ll_clear_te[id](dma);
	func_ll_clear_dme[id](dma);
	func_ll_clear_fe[id](dma);
}

bool stm32_dma_is_irq_happened(DMA_TypeDef *dma, u32_t id)
{
	if (func_ll_is_active_fe[id](dma) && LL_DMA_IsEnabledIT_FE(dma, id)) {
		return true;
	}

	return false;
}

bool stm32_dma_is_unexpected_irq_happened(DMA_TypeDef *dma, u32_t id)
{
	if (func_ll_is_active_fe[id](dma) && LL_DMA_IsEnabledIT_FE(dma, id)) {
		LOG_ERR("FiFo error.");
		stm32_dma_dump_stream_irq(dma, id);
		stm32_dma_clear_stream_irq(dma, id);

		return true;
	}

	return false;
}

void stm32_dma_enable_stream(DMA_TypeDef *dma, u32_t id)
{
	LL_DMA_EnableStream(dma, table_ll_stream[id]);
}

int stm32_dma_disable_stream(DMA_TypeDef *dma, u32_t id)
{

	if (!LL_DMA_IsEnabledStream(dma, table_ll_stream[id])) {
		return 0;
	}
	LL_DMA_DisableStream(dma, table_ll_stream[id]);

	return -EAGAIN;
}

void stm32_dma_disable_fifo_irq(DMA_TypeDef *dma, u32_t id)
{
	LL_DMA_DisableIT_FE(dma, table_ll_stream[id]);
}

void stm32_dma_config_channel_function(DMA_TypeDef *dma, u32_t id, u32_t slot)
{
	LL_DMA_SetChannelSelection(dma, table_ll_stream[id],
			table_ll_channel[slot]);
}

u32_t stm32_dma_get_mburst(struct dma_config *config, bool source_periph)
{
	u32_t memory_burst;

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

u32_t stm32_dma_get_pburst(struct dma_config *config, bool source_periph)
{
	u32_t periph_burst;

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
	u32_t msize = DMAx->MemoryOrM2MDstDataSize;
	u32_t fifo_level = DMAx->FIFOThreshold;
	u32_t mburst = DMAx->MemBurst;

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

u32_t stm32_dma_get_fifo_threshold(u16_t fifo_mode_control)
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
