/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO implementation for NXP i.MX RT (Teensy 4.x, etc.).
 *
 * i.MX RT iGPIO has dedicated atomic registers:
 *   DR_SET    (offset 0x84): atomic set
 *   DR_CLEAR  (offset 0x88): atomic clear
 *   DR_TOGGLE (offset 0x8C): atomic toggle (hardware XOR)
 *   PSR       (offset 0x08): pad status (input read)
 *   GDIR      (offset 0x04): GPIO direction register
 *
 * Cortex-M7 with instruction and data caches covers flash latency.
 * __ramfunc is not required.
 *
 * All types, functions, and constants are namespaced by the DT
 * compatible token (nxp_imx_gpio) for multi-backend dispatch.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_FAST_NXP_IMX_GPIO_H_
#define ZEPHYR_DRIVERS_GPIO_FAST_NXP_IMX_GPIO_H_

#include <zephyr/arch/common/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standalone call cost in CPU cycles for i.MX RT fast GPIO.
 *
 * Cycle costs are grouped by SOC series. Adding a new series
 * requires an explicit entry; unsupported series trigger #error.
 *
 * Cortex-M7 with I/D caches. LDR = 1-2 cy, STR = 1 cy (posted).
 * Counts include loading spec fields (pin_mask, register addresses).
 */
#if defined(CONFIG_SOC_SERIES_IMX6SX) || \
	defined(CONFIG_SOC_SERIES_IMX7D) || \
	defined(CONFIG_SOC_SERIES_IMX8M) || \
	defined(CONFIG_SOC_SERIES_IMXRT10XX) || \
	defined(CONFIG_SOC_SERIES_IMXRT11XX)
/* Cortex-M4 (IMX6SX/7D) / M7 (IMX8M/RT10XX/RT11XX) */

/** @brief CPU cycles for gpio_fast_set() on i.MX RT. */
#define GPIO_FAST_SET_CYCLES_nxp_imx_gpio        4  /* LDR pin + LDR set_reg + STR */
/** @brief CPU cycles for gpio_fast_clear() on i.MX RT. */
#define GPIO_FAST_CLEAR_CYCLES_nxp_imx_gpio      4  /* LDR pin + LDR clr_reg + STR */
/** @brief CPU cycles for gpio_fast_toggle() on i.MX RT. */
#define GPIO_FAST_TOGGLE_CYCLES_nxp_imx_gpio     4  /* LDR pin + LDR tgl_reg + STR */
/** @brief CPU cycles for gpio_fast_get() on i.MX RT. */
#define GPIO_FAST_GET_CYCLES_nxp_imx_gpio        6  /* LDR in_reg + LDR [in] + LDR pin + AND */
/** @brief CPU cycles for gpio_fast_set_input() on i.MX RT. */
/* LDR dir_reg + LDR [dir] + LDR pin + BIC + STR */
#define GPIO_FAST_SET_INPUT_CYCLES_nxp_imx_gpio  7
/** @brief CPU cycles for gpio_fast_set_output() on i.MX RT. */
/* LDR dir_reg + LDR [dir] + LDR pin + ORR + STR */
#define GPIO_FAST_SET_OUTPUT_CYCLES_nxp_imx_gpio 7

#else
#error "Unsupported NXP i.MX SOC series for fast GPIO"
#endif

/** @brief No RAM execution needed; i.MX RT has instruction cache. */
#define GPIO_FAST_SEND_STREAM_ATTR_nxp_imx_gpio

/**
 * @brief Opaque fast GPIO spec for NXP i.MX RT.
 *
 * Pre-computed register addresses and pin mask.
 * Consumers must not access fields directly.
 */
struct gpio_fast_spec_nxp_imx_gpio {
	/** DR_SET register address */
	mem_addr_t set_reg;
	/** DR_CLEAR register address */
	mem_addr_t clr_reg;
	/** DR_TOGGLE register address */
	mem_addr_t tgl_reg;
	/** PSR register address */
	mem_addr_t in_reg;
	/** GDIR register address */
	mem_addr_t dir_reg;
	/** Pin bitmask */
	uint32_t pin_mask;
};

/** @brief Set all masked pins HIGH via DR_SET register. */
static ALWAYS_INLINE void gpio_fast_set_nxp_imx_gpio(
	const struct gpio_fast_spec_nxp_imx_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->set_reg);
}

/** @brief Set all masked pins LOW via DR_CLEAR register. */
static ALWAYS_INLINE void gpio_fast_clear_nxp_imx_gpio(
	const struct gpio_fast_spec_nxp_imx_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->clr_reg);
}

/** @brief Toggle all masked pins via DR_TOGGLE hardware XOR register. */
static ALWAYS_INLINE void gpio_fast_toggle_nxp_imx_gpio(
	const struct gpio_fast_spec_nxp_imx_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->tgl_reg);
}

/** @brief Read raw physical state of masked pins from PSR. */
static ALWAYS_INLINE gpio_port_pins_t gpio_fast_get_nxp_imx_gpio(
	const struct gpio_fast_spec_nxp_imx_gpio *spec)
{
	return sys_read32(spec->in_reg) & spec->pin_mask;
}

/** @brief Switch masked pins to input via GDIR (read-modify-write). */
static ALWAYS_INLINE void gpio_fast_set_input_nxp_imx_gpio(
	const struct gpio_fast_spec_nxp_imx_gpio *spec)
{
	sys_write32(sys_read32(spec->dir_reg) & ~spec->pin_mask,
		    spec->dir_reg);
}

/** @brief Switch masked pins to output via GDIR (read-modify-write). */
static ALWAYS_INLINE void gpio_fast_set_output_nxp_imx_gpio(
	const struct gpio_fast_spec_nxp_imx_gpio *spec)
{
	sys_write32(sys_read32(spec->dir_reg) | spec->pin_mask,
		    spec->dir_reg);
}

int gpio_fast_configure_nxp_imx_gpio(struct gpio_fast_spec_nxp_imx_gpio *fast,
				     const struct device *port,
				     gpio_port_pins_t pin_mask,
				     gpio_flags_t flags);
int gpio_fast_pre_stream_nxp_imx_gpio(const void *spec);
int gpio_fast_post_stream_nxp_imx_gpio(const void *spec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_FAST_NXP_IMX_GPIO_H_ */
