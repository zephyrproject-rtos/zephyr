/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Gerson Fernando Budke
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO implementation for Atmel SAM0 series.
 *
 * SAM0 uses PORT_IOBUS for single-cycle atomic operations.
 * __ramfunc is required: SAM0 has no instruction cache and flash
 * wait states corrupt bit timing (confirmed in #106487 on SAMD21).
 * Marking ALWAYS_INLINE __ramfunc propagates the RAM placement
 * requirement automatically up the call chain to the first
 * non-inlined boundary, so no annotation needed in consumer code.
 *
 * All types, functions, and constants are namespaced by the DT
 * compatible token (atmel_sam0_gpio) for multi-backend dispatch.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_FAST_ATMEL_SAM0_GPIO_H_
#define ZEPHYR_DRIVERS_GPIO_FAST_ATMEL_SAM0_GPIO_H_

#include <zephyr/arch/common/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/** SAM0 requires RAM execution to avoid flash wait-state jitter. */
#define GPIO_FAST_SEND_STREAM_ATTR_atmel_sam0_gpio __ramfunc

/*
 * Standalone call cost in CPU cycles for SAM0 fast GPIO.
 *
 * Cycle costs are grouped by SOC series. Adding a new series
 * requires an explicit entry; unsupported series trigger #error.
 *
 * Cortex-M0+ from RAM, single-cycle IOBUS. Each LDR/STR = 2 cycles.
 * Counts include loading spec fields (pin_mask, register addresses).
 */
#if defined(CONFIG_SOC_SERIES_SAMC20) || \
	defined(CONFIG_SOC_SERIES_SAMC21) || \
	defined(CONFIG_SOC_SERIES_SAMD20) || \
	defined(CONFIG_SOC_SERIES_SAMD21) || \
	defined(CONFIG_SOC_SERIES_SAMD51) || \
	defined(CONFIG_SOC_SERIES_SAME51) || \
	defined(CONFIG_SOC_SERIES_SAME53) || \
	defined(CONFIG_SOC_SERIES_SAME54) || \
	defined(CONFIG_SOC_SERIES_SAML21) || \
	defined(CONFIG_SOC_SERIES_SAMR21) || \
	defined(CONFIG_SOC_SERIES_SAMR34) || \
	defined(CONFIG_SOC_SERIES_SAMR35)
/* Cortex-M0+ (SAMCxx/Dxx/Lxx/Rxx) / M4F (SAMD51/E5x) with PORT_IOBUS */

/** @brief CPU cycles for gpio_fast_set() on SAM0. */
#define GPIO_FAST_SET_CYCLES_atmel_sam0_gpio        6  /* LDR pin + LDR set_reg + STR IOBUS */
/** @brief CPU cycles for gpio_fast_clear() on SAM0. */
#define GPIO_FAST_CLEAR_CYCLES_atmel_sam0_gpio      6  /* LDR pin + LDR clr_reg + STR IOBUS */
/** @brief CPU cycles for gpio_fast_toggle() on SAM0. */
#define GPIO_FAST_TOGGLE_CYCLES_atmel_sam0_gpio     6  /* LDR pin + LDR tgl_reg + STR IOBUS */
/** @brief CPU cycles for gpio_fast_get() on SAM0. */
#define GPIO_FAST_GET_CYCLES_atmel_sam0_gpio        7  /* LDR in_reg + LDR [in] + LDR pin + AND */
/** @brief CPU cycles for gpio_fast_set_input() on SAM0. */
#define GPIO_FAST_SET_INPUT_CYCLES_atmel_sam0_gpio  6  /* LDR pin + LDR dir_clr + STR IOBUS */
/** @brief CPU cycles for gpio_fast_set_output() on SAM0. */
#define GPIO_FAST_SET_OUTPUT_CYCLES_atmel_sam0_gpio 6  /* LDR pin + LDR dir_set + STR IOBUS */

#else
#error "Unsupported SAM0 SOC series for fast GPIO"
#endif

/**
 * @brief Opaque fast GPIO spec for SAM0.
 *
 * Pre-computed register addresses and pin mask.
 * Consumers must not access fields directly.
 */
struct gpio_fast_spec_atmel_sam0_gpio {
	/** PORT_IOBUS->Group[n].OUTSET */
	mem_addr_t set_reg;
	/** PORT_IOBUS->Group[n].OUTCLR */
	mem_addr_t clr_reg;
	/** PORT_IOBUS->Group[n].OUTTGL */
	mem_addr_t tgl_reg;
	/** PORT_IOBUS->Group[n].IN */
	mem_addr_t in_reg;
	/** PORT_IOBUS->Group[n].DIRSET */
	mem_addr_t dir_set;
	/** PORT_IOBUS->Group[n].DIRCLR */
	mem_addr_t dir_clr;
	/** Pin bitmask */
	uint32_t pin_mask;
};

/** @brief Set all masked pins HIGH via PORT_IOBUS OUTSET. */
static ALWAYS_INLINE void gpio_fast_set_atmel_sam0_gpio(
	const struct gpio_fast_spec_atmel_sam0_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->set_reg);
}

/** @brief Set all masked pins LOW via PORT_IOBUS OUTCLR. */
static ALWAYS_INLINE void gpio_fast_clear_atmel_sam0_gpio(
	const struct gpio_fast_spec_atmel_sam0_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->clr_reg);
}

/** @brief Toggle all masked pins via PORT_IOBUS OUTTGL (hardware single write). */
static ALWAYS_INLINE void gpio_fast_toggle_atmel_sam0_gpio(
	const struct gpio_fast_spec_atmel_sam0_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->tgl_reg);
}

/** @brief Read raw physical state of masked pins from PORT_IOBUS IN. */
static ALWAYS_INLINE gpio_port_pins_t gpio_fast_get_atmel_sam0_gpio(
	const struct gpio_fast_spec_atmel_sam0_gpio *spec)
{
	return sys_read32(spec->in_reg) & spec->pin_mask;
}

/** @brief Switch masked pins to input via PORT_IOBUS DIRCLR. */
static ALWAYS_INLINE void gpio_fast_set_input_atmel_sam0_gpio(
	const struct gpio_fast_spec_atmel_sam0_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->dir_clr);
}

/** @brief Switch masked pins to output via PORT_IOBUS DIRSET. */
static ALWAYS_INLINE void gpio_fast_set_output_atmel_sam0_gpio(
	const struct gpio_fast_spec_atmel_sam0_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->dir_set);
}

int gpio_fast_configure_atmel_sam0_gpio(struct gpio_fast_spec_atmel_sam0_gpio *fast,
					const struct device *port,
					gpio_port_pins_t pin_mask,
					gpio_flags_t flags);
int gpio_fast_pre_stream_atmel_sam0_gpio(const void *spec);
int gpio_fast_post_stream_atmel_sam0_gpio(const void *spec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_FAST_ATMEL_SAM0_GPIO_H_ */
