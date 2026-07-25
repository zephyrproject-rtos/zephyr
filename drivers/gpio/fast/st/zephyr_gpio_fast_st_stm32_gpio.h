/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Gerson Fernando Budke
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO implementation for STM32 series.
 *
 * STM32 uses BSRR: low 16 bits set, high 16 bits reset (single write).
 * TOGGLE via ODR XOR. Direction via MODER register (2 bits per pin).
 * STM32 ART accelerator covers flash latency in most variants;
 * __ramfunc not required in the general case.
 *
 * All types, functions, and constants are namespaced by the DT
 * compatible token (st_stm32_gpio) for multi-backend dispatch.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_FAST_ST_STM32_GPIO_H_
#define ZEPHYR_DRIVERS_GPIO_FAST_ST_STM32_GPIO_H_

#include <zephyr/arch/common/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standalone call cost in CPU cycles for STM32 fast GPIO.
 *
 * Cycle costs are grouped by SOC series. Adding a new series
 * requires an explicit entry; unsupported series trigger #error.
 *
 * Cortex-M3/M4 with ART accelerator. LDR = 2 cy, STR = 1 cy (posted).
 * Counts include loading spec fields (masks, register addresses).
 */
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32C5X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32H7RSX) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L1X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32MP13X) || \
	defined(CONFIG_SOC_SERIES_STM32MP1X) || \
	defined(CONFIG_SOC_SERIES_STM32MP2X) || \
	defined(CONFIG_SOC_SERIES_STM32N6X) || \
	defined(CONFIG_SOC_SERIES_STM32U0X) || \
	defined(CONFIG_SOC_SERIES_STM32U3X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WB0X) || \
	defined(CONFIG_SOC_SERIES_STM32WBAX) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
/* Cortex-M0/M0+ / M3 / M4 / M7 / M33 with MODER register */

/** @brief CPU cycles for gpio_fast_set() on STM32. */
#define GPIO_FAST_SET_CYCLES_st_stm32_gpio        5  /* LDR set_mask + LDR bsrr_reg + STR */
/** @brief CPU cycles for gpio_fast_clear() on STM32. */
#define GPIO_FAST_CLEAR_CYCLES_st_stm32_gpio      5  /* LDR clr_mask + LDR bsrr_reg + STR */
/** @brief CPU cycles for gpio_fast_toggle() on STM32. */
/* LDR odr_reg + LDR [odr] + LDR pin + EOR + STR */
#define GPIO_FAST_TOGGLE_CYCLES_st_stm32_gpio     8
/** @brief CPU cycles for gpio_fast_get() on STM32. */
#define GPIO_FAST_GET_CYCLES_st_stm32_gpio        7  /* LDR idr_reg + LDR [idr] + LDR pin + AND */
/** @brief CPU cycles for gpio_fast_set_input() on STM32. */
/* LDR moder_reg + LDR [moder] + LDR mask + BIC + LDR in + ORR + STR */
#define GPIO_FAST_SET_INPUT_CYCLES_st_stm32_gpio  11
/** @brief CPU cycles for gpio_fast_set_output() on STM32. */
/* LDR moder_reg + LDR [moder] + LDR mask + BIC + LDR out + ORR + STR */
#define GPIO_FAST_SET_OUTPUT_CYCLES_st_stm32_gpio 11

#elif defined(CONFIG_SOC_SERIES_STM32F1X)
/* Cortex-M3, CRL/CRH registers instead of MODER */

/** @brief CPU cycles for gpio_fast_set() on STM32. */
#define GPIO_FAST_SET_CYCLES_st_stm32_gpio        5  /* LDR set_mask + LDR bsrr_reg + STR */
/** @brief CPU cycles for gpio_fast_clear() on STM32. */
#define GPIO_FAST_CLEAR_CYCLES_st_stm32_gpio      5  /* LDR clr_mask + LDR bsrr_reg + STR */
/** @brief CPU cycles for gpio_fast_toggle() on STM32. */
/* LDR odr_reg + LDR [odr] + LDR pin + EOR + STR */
#define GPIO_FAST_TOGGLE_CYCLES_st_stm32_gpio     8
/** @brief CPU cycles for gpio_fast_get() on STM32. */
#define GPIO_FAST_GET_CYCLES_st_stm32_gpio        7  /* LDR idr_reg + LDR [idr] + LDR pin + AND */
/** @brief CPU cycles for gpio_fast_set_input() on STM32. */
/* CRL RMW + CRH RMW: 2x (LDR reg + LDR [cr] + LDR mask + BIC + LDR in + ORR + STR) */
#define GPIO_FAST_SET_INPUT_CYCLES_st_stm32_gpio  22
/** @brief CPU cycles for gpio_fast_set_output() on STM32. */
/* CRL RMW + CRH RMW */
#define GPIO_FAST_SET_OUTPUT_CYCLES_st_stm32_gpio 22

#else
#error "Unsupported STM32 SOC series for fast GPIO"
#endif

/** @brief No RAM execution needed; STM32 has instruction cache. */
#define GPIO_FAST_SEND_STREAM_ATTR_st_stm32_gpio

/**
 * @brief Opaque fast GPIO spec for STM32.
 *
 * Pre-computed register addresses and masks.
 * Consumers must not access fields directly.
 */
struct gpio_fast_spec_st_stm32_gpio {
	/** BSRR register address */
	mem_addr_t bsrr_reg;
	/** ODR register address */
	mem_addr_t odr_reg;
	/** IDR register address */
	mem_addr_t idr_reg;
	/** MODER register address (F1: CRL address) */
	mem_addr_t moder_reg;
	/** Low 16 bits of BSRR (set) */
	uint32_t set_mask;
	/** High 16 bits of BSRR (reset) */
	uint32_t clr_mask;
	/** Bit position in ODR/IDR */
	uint32_t pin_mask;
	/** MODER/CRL value for output */
	uint32_t moder_out;
	/** MODER/CRL value for input */
	uint32_t moder_in;
	/** Bit mask in MODER/CRL */
	uint32_t moder_mask;
#ifdef CONFIG_SOC_SERIES_STM32F1X
	/** CRH output value (pins 8-15, F1 only) */
	uint32_t moder_out_hi;
	/** CRH input value (pins 8-15, F1 only) */
	uint32_t moder_in_hi;
	/** CRH bit mask (pins 8-15, F1 only) */
	uint32_t moder_mask_hi;
#endif
};

/** @brief Set all masked pins HIGH via BSRR (low 16 bits). */
static ALWAYS_INLINE void gpio_fast_set_st_stm32_gpio(
	const struct gpio_fast_spec_st_stm32_gpio *spec)
{
	sys_write32(spec->set_mask, spec->bsrr_reg);
}

/** @brief Set all masked pins LOW via BSRR (high 16 bits). */
static ALWAYS_INLINE void gpio_fast_clear_st_stm32_gpio(
	const struct gpio_fast_spec_st_stm32_gpio *spec)
{
	sys_write32(spec->clr_mask, spec->bsrr_reg);
}

/** @brief Toggle all masked pins via ODR read-modify-write (XOR). */
static ALWAYS_INLINE void gpio_fast_toggle_st_stm32_gpio(
	const struct gpio_fast_spec_st_stm32_gpio *spec)
{
	sys_write32(sys_read32(spec->odr_reg) ^ spec->pin_mask,
		    spec->odr_reg);
}

/** @brief Read raw physical state of masked pins from IDR. */
static ALWAYS_INLINE gpio_port_pins_t gpio_fast_get_st_stm32_gpio(
	const struct gpio_fast_spec_st_stm32_gpio *spec)
{
	return sys_read32(spec->idr_reg) & spec->pin_mask;
}

/** @brief Switch masked pins to input via MODER/CRL/CRH. */
static ALWAYS_INLINE void gpio_fast_set_input_st_stm32_gpio(
	const struct gpio_fast_spec_st_stm32_gpio *spec)
{
	sys_write32((sys_read32(spec->moder_reg) & ~spec->moder_mask) |
		    spec->moder_in, spec->moder_reg);
#ifdef CONFIG_SOC_SERIES_STM32F1X
	/* CRH is at moder_reg + 4. When moder_mask_hi is 0 this is
	 * a harmless identity write (read, AND ~0, OR 0, write back).
	 */
	sys_write32((sys_read32(spec->moder_reg + 4) &
		    ~spec->moder_mask_hi) |
		    spec->moder_in_hi, spec->moder_reg + 4);
#endif
}

/** @brief Switch masked pins to output via MODER/CRL/CRH. */
static ALWAYS_INLINE void gpio_fast_set_output_st_stm32_gpio(
	const struct gpio_fast_spec_st_stm32_gpio *spec)
{
	sys_write32((sys_read32(spec->moder_reg) & ~spec->moder_mask) |
		    spec->moder_out, spec->moder_reg);
#ifdef CONFIG_SOC_SERIES_STM32F1X
	sys_write32((sys_read32(spec->moder_reg + 4) &
		    ~spec->moder_mask_hi) |
		    spec->moder_out_hi, spec->moder_reg + 4);
#endif
}

int gpio_fast_configure_st_stm32_gpio(struct gpio_fast_spec_st_stm32_gpio *fast,
				      const struct device *port,
				      gpio_port_pins_t pin_mask,
				      gpio_flags_t flags);
int gpio_fast_pre_stream_st_stm32_gpio(const void *spec);
int gpio_fast_post_stream_st_stm32_gpio(const void *spec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_FAST_ST_STM32_GPIO_H_ */
