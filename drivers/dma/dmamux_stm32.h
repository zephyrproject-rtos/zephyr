/*
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMAMUX_STM32_H_
#define DMAMUX_STM32_H_

/* this is the configuration of one dmamux channel */
struct dmamux_stm32_channel {
	const struct device *dev_dma; /* pointer to the associated dma instance */
	uint8_t dma_id; /* ref of the associated dma stream for this instance */
};

uint32_t table_ll_channel[] = {
	LL_DMAMUX_CHANNEL_0,
	LL_DMAMUX_CHANNEL_1,
	LL_DMAMUX_CHANNEL_2,
	LL_DMAMUX_CHANNEL_3,
	LL_DMAMUX_CHANNEL_4,
#if defined(LL_DMAMUX_CHANNEL_5)
	LL_DMAMUX_CHANNEL_5,
#endif /* LL_DMAMUX_CHANNEL_5 */
#if defined(LL_DMAMUX_CHANNEL_6)
	LL_DMAMUX_CHANNEL_6,
#endif /* LL_DMAMUX_CHANNEL_6 */
#if defined(LL_DMAMUX_CHANNEL_7)
	LL_DMAMUX_CHANNEL_7,
#endif /* LL_DMAMUX_CHANNEL_7 */
#if defined(LL_DMAMUX_CHANNEL_8)
	LL_DMAMUX_CHANNEL_8,
#endif /* LL_DMAMUX_CHANNEL_8 */
#if defined(LL_DMAMUX_CHANNEL_9)
	LL_DMAMUX_CHANNEL_9,
#endif /* LL_DMAMUX_CHANNEL_9 */
#if defined(LL_DMAMUX_CHANNEL_10)
	LL_DMAMUX_CHANNEL_10,
#endif /* LL_DMAMUX_CHANNEL_10 */
#if defined(LL_DMAMUX_CHANNEL_11)
	LL_DMAMUX_CHANNEL_11,
#endif /* LL_DMAMUX_CHANNEL_11 */
#if defined(LL_DMAMUX_CHANNEL_12)
	LL_DMAMUX_CHANNEL_12,
#endif /* LL_DMAMUX_CHANNEL_12 */
#if defined(LL_DMAMUX_CHANNEL_13)
	LL_DMAMUX_CHANNEL_13,
#endif /* LL_DMAMUX_CHANNEL_13 */
#if defined(LL_DMAMUX_CHANNEL_14)
	LL_DMAMUX_CHANNEL_14,
#endif /* LL_DMAMUX_CHANNEL_14 */
#if defined(LL_DMAMUX_CHANNEL_15)
	LL_DMAMUX_CHANNEL_15,
#endif /* LL_DMAMUX_CHANNEL_15 */
};

uint32_t (*func_ll_is_active_so[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LL_DMAMUX_IsActiveFlag_SO0,
	LL_DMAMUX_IsActiveFlag_SO1,
	LL_DMAMUX_IsActiveFlag_SO2,
	LL_DMAMUX_IsActiveFlag_SO3,
	LL_DMAMUX_IsActiveFlag_SO4,
#if defined(LL_DMAMUX_CHANNEL_5)
	LL_DMAMUX_IsActiveFlag_SO5,
#endif /* LL_DMAMUX_CHANNEL_5 */
#if defined(LL_DMAMUX_CHANNEL_6)
	LL_DMAMUX_IsActiveFlag_SO6,
#endif /* LL_DMAMUX_CHANNEL_6 */
#if defined(LL_DMAMUX_CHANNEL_7)
	LL_DMAMUX_IsActiveFlag_SO7,
#endif /* LL_DMAMUX_CHANNEL_7 */
#if defined(LL_DMAMUX_CHANNEL_8)
	LL_DMAMUX_IsActiveFlag_SO8,
#endif /* LL_DMAMUX_CHANNEL_8 */
#if defined(LL_DMAMUX_CHANNEL_9)
	LL_DMAMUX_IsActiveFlag_SO9,
#endif /* LL_DMAMUX_CHANNEL_9 */
#if defined(LL_DMAMUX_CHANNEL_10)
	LL_DMAMUX_IsActiveFlag_SO10,
#endif /* LL_DMAMUX_CHANNEL_10 */
#if defined(LL_DMAMUX_CHANNEL_11)
	LL_DMAMUX_IsActiveFlag_SO11,
#endif /* LL_DMAMUX_CHANNEL_11 */
#if defined(LL_DMAMUX_CHANNEL_12)
	LL_DMAMUX_IsActiveFlag_SO12,
#endif /* LL_DMAMUX_CHANNEL_12 */
#if defined(LL_DMAMUX_CHANNEL_130)
	LL_DMAMUX_IsActiveFlag_SO13,
#endif /* LL_DMAMUX_CHANNEL_130 */
#if defined(LL_DMAMUX_CHANNEL_14)
	LL_DMAMUX_IsActiveFlag_SO14,
#endif /* LL_DMAMUX_CHANNEL_14 */
#if defined(LL_DMAMUX_CHANNEL_15)
	LL_DMAMUX_IsActiveFlag_SO15,
#endif /* LL_DMAMUX_CHANNEL_15 */
};

void (*func_ll_clear_so[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LL_DMAMUX_ClearFlag_SO0,
	LL_DMAMUX_ClearFlag_SO1,
	LL_DMAMUX_ClearFlag_SO2,
	LL_DMAMUX_ClearFlag_SO3,
	LL_DMAMUX_ClearFlag_SO4,
#if defined(LL_DMAMUX_CHANNEL_5)
	LL_DMAMUX_ClearFlag_SO5,
#endif /* LL_DMAMUX_CHANNEL_5 */
#if defined(LL_DMAMUX_CHANNEL_6)
	LL_DMAMUX_ClearFlag_SO6,
#endif /* LL_DMAMUX_CHANNEL_6 */
#if defined(LL_DMAMUX_CHANNEL_7)
	LL_DMAMUX_ClearFlag_SO7,
#endif /* LL_DMAMUX_CHANNEL_7 */
#if defined(LL_DMAMUX_CHANNEL_8)
	LL_DMAMUX_ClearFlag_SO8,
#endif /* LL_DMAMUX_CHANNEL_8 */
#if defined(LL_DMAMUX_CHANNEL_9)
	LL_DMAMUX_ClearFlag_SO9,
#endif /* LL_DMAMUX_CHANNEL_9 */
#if defined(LL_DMAMUX_CHANNEL_10)
	LL_DMAMUX_ClearFlag_SO10,
#endif /* LL_DMAMUX_CHANNEL_10 */
#if defined(LL_DMAMUX_CHANNEL_11)
	LL_DMAMUX_ClearFlag_SO11,
#endif /* LL_DMAMUX_CHANNEL_11 */
#if defined(LL_DMAMUX_CHANNEL_12)
	LL_DMAMUX_ClearFlag_SO12,
#endif /* LL_DMAMUX_CHANNEL_12 */
#if defined(LL_DMAMUX_CHANNEL_13)
	LL_DMAMUX_ClearFlag_SO13,
#endif /* LL_DMAMUX_CHANNEL_13 */
#if defined(LL_DMAMUX_CHANNEL_14)
	LL_DMAMUX_ClearFlag_SO14,
#endif /* LL_DMAMUX_CHANNEL_14 */
#if defined(LL_DMAMUX_CHANNEL_15)
	LL_DMAMUX_ClearFlag_SO15,
#endif /* LL_DMAMUX_CHANNEL_15 */
};

uint32_t (*func_ll_is_active_rgo[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LL_DMAMUX_IsActiveFlag_RGO0,
	LL_DMAMUX_IsActiveFlag_RGO1,
	LL_DMAMUX_IsActiveFlag_RGO2,
	LL_DMAMUX_IsActiveFlag_RGO3,
};

void (*func_ll_clear_rgo[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LL_DMAMUX_ClearFlag_RGO0,
	LL_DMAMUX_ClearFlag_RGO1,
	LL_DMAMUX_ClearFlag_RGO2,
	LL_DMAMUX_ClearFlag_RGO3,
};

#endif /* DMAMUX_STM32_H_*/
