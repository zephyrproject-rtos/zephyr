/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stm32_ll_rng.h>

/**
 * This driver supports two compatibles:
 *	- "st,stm32-rng" for TRNG with IRQ lines
 *	- "st,stm32-rng-noirq" for TRNG without IRQ lines
 */
#define IRQLESS_TRNG DT_HAS_COMPAT_STATUS_OKAY(st_stm32_rng_noirq)

#if IRQLESS_TRNG
#define DT_DRV_COMPAT st_stm32_rng_noirq
#define TRNG_GENERATION_DELAY	K_NSEC(DT_INST_PROP_OR(0, generation_delay_ns, 0))
#else /* !IRQLESS_TRNG */
#define DT_DRV_COMPAT st_stm32_rng
#define IRQN		DT_INST_IRQN(0)
#define IRQ_PRIO	DT_INST_IRQ(0, priority)
#endif /* IRQLESS_TRNG */

/* Cross-series LL compatibility wrappers */
static inline void ll_rng_enable_it(RNG_TypeDef *RNGx)
{
	/* Silence "unused" warning on IRQ-less hardware*/
	ARG_UNUSED(RNGx);
#if !IRQLESS_TRNG
#	if defined(CONFIG_SOC_STM32WB09XX)
		LL_RNG_EnableEnErrorIrq(RNGx);
		LL_RNG_EnableEnFfFullIrq(RNGx);
#	else
		LL_RNG_EnableIT(RNGx);
#	endif
#endif /* !IRQLESS_TRNG */
}

static inline uint32_t ll_rng_is_active_seis(RNG_TypeDef *RNGx)
{
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
#	if defined(CONFIG_SOC_STM32WB09XX)
		return LL_RNG_IsActiveFlag_ENTROPY_ERR(RNGx);
#	else
		return LL_RNG_IsActiveFlag_FAULT(RNGx);
#	endif
#else
	return LL_RNG_IsActiveFlag_SEIS(RNGx);
#endif /* CONFIG_SOC_SERIES_STM32WB0X */
}

static inline void ll_rng_clear_seis(RNG_TypeDef *RNGx)
{
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
#	if defined(CONFIG_SOC_STM32WB09XX)
		LL_RNG_SetResetHealthErrorFlags(RNGx, 1);
#	else
		LL_RNG_ClearFlag_FAULT(RNGx);
#	endif
#else
	LL_RNG_ClearFlag_SEIS(RNGx);
#endif /* CONFIG_SOC_SERIES_STM32WB0X */
}

static inline uint32_t ll_rng_is_active_secs(RNG_TypeDef *RNGx)
{
#if !defined(CONFIG_SOC_SERIES_STM32WB0X)
	return LL_RNG_IsActiveFlag_SECS(RNGx);
#else
	/**
	 * STM32WB0x RNG has no equivalent of SECS.
	 * Since this flag is always checked in conjunction
	 * with FAULT (the SEIS equivalent), returning 0 is OK.
	 */
	return 0;
#endif /* !CONFIG_SOC_SERIES_STM32WB0X */
}

static inline uint32_t ll_rng_is_active_drdy(RNG_TypeDef *RNGx)
{
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
#	if defined(CONFIG_SOC_STM32WB09XX)
		return LL_RNG_IsActiveFlag_VAL_READY(RNGx);
#	else
		return LL_RNG_IsActiveFlag_RNGRDY(RNGx);
#	endif
#else
	return LL_RNG_IsActiveFlag_DRDY(RNGx);
#endif /* CONFIG_SOC_SERIES_STM32WB0X */
}

static inline uint16_t ll_rng_read_rand_data(RNG_TypeDef *RNGx)
{
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
#	if defined(CONFIG_SOC_STM32WB09XX)
		return (uint16_t)LL_RNG_GetRndVal(RNGx);
#	else
		return LL_RNG_ReadRandData16(RNGx);
#	endif
#else
	return (uint16_t)LL_RNG_ReadRandData32(RNGx);
#endif /* CONFIG_SOC_SERIES_STM32WB0X */
}
