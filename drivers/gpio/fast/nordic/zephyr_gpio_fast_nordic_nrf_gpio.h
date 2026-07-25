/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Gerson Fernando Budke
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO implementation for Nordic nRF series.
 *
 * nRF has dedicated OUTSET/OUTCLR registers and an instruction cache
 * that covers flash latency. __ramfunc is not required.
 * No hardware TOGGLE register, so it is derived from output state.
 *
 * All types, functions, and constants are namespaced by the DT
 * compatible token (nordic_nrf_gpio) for multi-backend dispatch.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_FAST_NORDIC_NRF_GPIO_H_
#define ZEPHYR_DRIVERS_GPIO_FAST_NORDIC_NRF_GPIO_H_

#include <zephyr/arch/common/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CPU clock frequency for nRF.
 *
 * The nRF system timer uses the 32 kHz RTC, so
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC is 32768, not the CPU clock.
 * The CPU clock comes from the HFXO node in devicetree.
 */
#define GPIO_FAST_CPU_FREQ DT_PROP(DT_NODELABEL(hfxo), clock_frequency)

/*
 * Standalone call cost in CPU cycles for nRF fast GPIO.
 *
 * Cycle costs are grouped by SOC series. Adding a new series
 * requires an explicit entry; unsupported series trigger #error.
 *
 * Cortex-M4 with instruction cache. LDR = 2 cy, STR = 1 cy (posted).
 * Counts include loading spec fields (pin_mask, register addresses).
 */
#if defined(CONFIG_SOC_SERIES_NRF51) || \
	defined(CONFIG_SOC_SERIES_NRF52) || \
	defined(CONFIG_SOC_SERIES_NRF53) || \
	defined(CONFIG_SOC_SERIES_NRF54H) || \
	defined(CONFIG_SOC_SERIES_NRF54L) || \
	defined(CONFIG_SOC_SERIES_NRF71) || \
	defined(CONFIG_SOC_SERIES_NRF91) || \
	defined(CONFIG_SOC_SERIES_NRF92)
/* Cortex-M0 (nRF51) / M4F (nRF52) / M33 (nRF53/54/71/91/92) */

/** @brief CPU cycles for gpio_fast_set() on nRF. */
#define GPIO_FAST_SET_CYCLES_nordic_nrf_gpio        5  /* LDR pin + LDR set_reg + STR */
/** @brief CPU cycles for gpio_fast_clear() on nRF. */
#define GPIO_FAST_CLEAR_CYCLES_nordic_nrf_gpio      5  /* LDR pin + LDR clr_reg + STR */
/** @brief CPU cycles for gpio_fast_toggle() on nRF. */
/* LDR out_reg + LDR [out] + LDR pin + AND + CMP + B + LDR reg + STR */
#define GPIO_FAST_TOGGLE_CYCLES_nordic_nrf_gpio     12
/** @brief CPU cycles for gpio_fast_get() on nRF. */
#define GPIO_FAST_GET_CYCLES_nordic_nrf_gpio        7  /* LDR in_reg + LDR [in] + LDR pin + AND */
/** @brief CPU cycles for gpio_fast_set_input() on nRF. */
/* LDR dir_reg + LDR [dir] + LDR pin + BIC + STR */
#define GPIO_FAST_SET_INPUT_CYCLES_nordic_nrf_gpio  8
/** @brief CPU cycles for gpio_fast_set_output() on nRF. */
/* LDR dir_reg + LDR [dir] + LDR pin + ORR + STR */
#define GPIO_FAST_SET_OUTPUT_CYCLES_nordic_nrf_gpio 8

#else
#error "Unsupported nRF SOC series for fast GPIO"
#endif

/** @brief No RAM execution needed; nRF has no flash wait-state jitter. */
#define GPIO_FAST_SEND_STREAM_ATTR_nordic_nrf_gpio

/**
 * @brief Opaque fast GPIO spec for nRF.
 *
 * Pre-computed register addresses and pin mask.
 * Consumers must not access fields directly.
 */
struct gpio_fast_spec_nordic_nrf_gpio {
	/** OUTSET register address */
	mem_addr_t set_reg;
	/** OUTCLR register address */
	mem_addr_t clr_reg;
	/** OUT register address */
	mem_addr_t out_reg;
	/** IN register address */
	mem_addr_t in_reg;
	/** DIR register address */
	mem_addr_t dir_reg;
	/** Pin bitmask */
	uint32_t pin_mask;
};

/** @brief Set all masked pins HIGH via OUTSET register. */
static ALWAYS_INLINE void gpio_fast_set_nordic_nrf_gpio(
	const struct gpio_fast_spec_nordic_nrf_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->set_reg);
}

/** @brief Set all masked pins LOW via OUTCLR register. */
static ALWAYS_INLINE void gpio_fast_clear_nordic_nrf_gpio(
	const struct gpio_fast_spec_nordic_nrf_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->clr_reg);
}

/**
 * @brief Toggle all masked pins by reading OUT and branching.
 *
 * nRF has no hardware toggle register. This reads the current
 * output state and writes to OUTSET or OUTCLR accordingly.
 */
static ALWAYS_INLINE void gpio_fast_toggle_nordic_nrf_gpio(
	const struct gpio_fast_spec_nordic_nrf_gpio *spec)
{
	if (sys_read32(spec->out_reg) & spec->pin_mask) {
		sys_write32(spec->pin_mask, spec->clr_reg);
	} else {
		sys_write32(spec->pin_mask, spec->set_reg);
	}
}

/** @brief Read raw physical state of masked pins from IN register. */
static ALWAYS_INLINE gpio_port_pins_t gpio_fast_get_nordic_nrf_gpio(
	const struct gpio_fast_spec_nordic_nrf_gpio *spec)
{
	return sys_read32(spec->in_reg) & spec->pin_mask;
}

/** @brief Switch masked pins to input via DIR read-modify-write. */
static ALWAYS_INLINE void gpio_fast_set_input_nordic_nrf_gpio(
	const struct gpio_fast_spec_nordic_nrf_gpio *spec)
{
	sys_write32(sys_read32(spec->dir_reg) & ~spec->pin_mask,
		    spec->dir_reg);
}

/** @brief Switch masked pins to output via DIR read-modify-write. */
static ALWAYS_INLINE void gpio_fast_set_output_nordic_nrf_gpio(
	const struct gpio_fast_spec_nordic_nrf_gpio *spec)
{
	sys_write32(sys_read32(spec->dir_reg) | spec->pin_mask,
		    spec->dir_reg);
}

int gpio_fast_configure_nordic_nrf_gpio(struct gpio_fast_spec_nordic_nrf_gpio *fast,
					const struct device *port,
					gpio_port_pins_t pin_mask,
					gpio_flags_t flags);
int gpio_fast_pre_stream_nordic_nrf_gpio(const void *spec);
int gpio_fast_post_stream_nordic_nrf_gpio(const void *spec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_FAST_NORDIC_NRF_GPIO_H_ */
