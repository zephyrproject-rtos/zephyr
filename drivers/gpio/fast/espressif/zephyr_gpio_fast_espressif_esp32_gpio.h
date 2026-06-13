/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO implementation for Espressif ESP32 series.
 *
 * ESP32 has dedicated W1TS (write-1-to-set) and W1TC (write-1-to-clear)
 * registers for atomic pin operations. No hardware TOGGLE register, so
 * toggle is derived from output state (read-modify-write).
 *
 * Output direction uses enable_w1ts/enable_w1tc registers for atomic
 * enable/disable. Input enable goes through IO_MUX (per-pin, slow)
 * but is not needed for gpio_fast_set_input, so disabling output is
 * sufficient to switch to high-Z/input for protocols like 1-Wire.
 *
 * Covers both Xtensa (ESP32/S2/S3) and RISC-V (C2/C3/C5/C6/H2/P4)
 * variants. All have instruction cache; __ramfunc is not required.
 *
 * All types, functions, and constants are namespaced by the DT
 * compatible token (espressif_esp32_gpio) for multi-backend
 */

#ifndef ZEPHYR_DRIVERS_GPIO_FAST_ESPRESSIF_ESP32_GPIO_H_
#define ZEPHYR_DRIVERS_GPIO_FAST_ESPRESSIF_ESP32_GPIO_H_

#include <zephyr/arch/common/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standalone call cost in CPU cycles for ESP32 fast GPIO.
 *
 * Cycle costs are grouped by SOC series. Adding a new series
 * requires an explicit entry; unsupported series trigger #error.
 *
 * Xtensa (ESP32/S2/S3):
 *   L32I from cached SRAM = 1 cy, L32I from APB = 2-3 cy,
 *   S32I = 1-2 cy, ALU = 1 cy.
 *
 * RISC-V (C2/C3/C5/C6/H2/P4):
 *   lw from SRAM = 1 cy, lw from APB = 2-3 cy,
 *   sw = 1-2 cy, ALU = 1 cy.
 *
 * Cycle costs are comparable across both architectures.
 * Counts include loading spec fields from SRAM.
 */
#if defined(CONFIG_SOC_SERIES_ESP32) || \
	defined(CONFIG_SOC_SERIES_ESP32S2) || \
	defined(CONFIG_SOC_SERIES_ESP32S3) || \
	defined(CONFIG_SOC_SERIES_ESP32C2) || \
	defined(CONFIG_SOC_SERIES_ESP32C3) || \
	defined(CONFIG_SOC_SERIES_ESP32C5) || \
	defined(CONFIG_SOC_SERIES_ESP32C6) || \
	defined(CONFIG_SOC_SERIES_ESP32H2) || \
	defined(CONFIG_SOC_SERIES_ESP32P4)
/* Xtensa (ESP32/S2/S3) / RISC-V (C2/C3/C5/C6/H2/P4) */

/** @brief CPU cycles for gpio_fast_set() for ESP32. */
/* L32I pin + L32I set_reg + S32I (APB) */
#define GPIO_FAST_SET_CYCLES_espressif_esp32_gpio        4
/** @brief CPU cycles for gpio_fast_clear() for ESP32. */
/* L32I pin + L32I clr_reg + S32I (APB) */
#define GPIO_FAST_CLEAR_CYCLES_espressif_esp32_gpio      4
/** @brief CPU cycles for gpio_fast_toggle() for ESP32. */
/* L32I out_reg + L32I [out](APB) + L32I pin + XOR + S32I */
#define GPIO_FAST_TOGGLE_CYCLES_espressif_esp32_gpio     8
/** @brief CPU cycles for gpio_fast_get() for ESP32. */
/* L32I in_reg + L32I [in](APB) + L32I pin + AND */
#define GPIO_FAST_GET_CYCLES_espressif_esp32_gpio        6
/** @brief CPU cycles for gpio_fast_set_input() for ESP32. */
/* L32I pin + L32I dir_clr + S32I (APB) */
#define GPIO_FAST_SET_INPUT_CYCLES_espressif_esp32_gpio  4
/** @brief CPU cycles for gpio_fast_set_output() for ESP32. */
/* L32I pin + L32I dir_set + S32I (APB) */
#define GPIO_FAST_SET_OUTPUT_CYCLES_espressif_esp32_gpio 4

#else
#error "Unsupported ESP32 SOC series for fast GPIO"
#endif

/** @brief No RAM execution needed; ESP32 has instruction cache. */
#define GPIO_FAST_SEND_STREAM_ATTR_espressif_esp32_gpio

/**
 * @brief Opaque fast GPIO spec for ESP32.
 *
 * Pre-computed register addresses and pin mask.
 * The correct port-specific registers (gpio0 vs gpio1) are resolved
 * at configure time. Consumers must not access fields directly.
 */
struct gpio_fast_spec_espressif_esp32_gpio {
	/** out_w1ts / out1_w1ts register address */
	mem_addr_t set_reg;
	/** out_w1tc / out1_w1tc register address */
	mem_addr_t clr_reg;
	/** out / out1 register address (for toggle) */
	mem_addr_t out_reg;
	/** in / in1 register address */
	mem_addr_t in_reg;
	/** enable_w1ts / enable1_w1ts register address */
	mem_addr_t dir_set;
	/** enable_w1tc / enable1_w1tc register address */
	mem_addr_t dir_clr;
	/** Pin bitmask */
	uint32_t pin_mask;
};

/** @brief Set all masked pins HIGH via W1TS register. */
static ALWAYS_INLINE void gpio_fast_set_espressif_esp32_gpio(
	const struct gpio_fast_spec_espressif_esp32_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->set_reg);
}

/** @brief Set all masked pins LOW via W1TC register. */
static ALWAYS_INLINE void gpio_fast_clear_espressif_esp32_gpio(
	const struct gpio_fast_spec_espressif_esp32_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->clr_reg);
}

/**
 * @brief Toggle all masked pins via read-modify-write on OUT register.
 *
 * ESP32 has no hardware toggle register. This is a non-atomic
 * read-modify-write. Callers must ensure adequate synchronization.
 */
static ALWAYS_INLINE void gpio_fast_toggle_espressif_esp32_gpio(
	const struct gpio_fast_spec_espressif_esp32_gpio *spec)
{
	sys_write32(sys_read32(spec->out_reg) ^ spec->pin_mask,
		    spec->out_reg);
}

/** @brief Read raw physical state of masked pins. */
static ALWAYS_INLINE gpio_port_pins_t gpio_fast_get_espressif_esp32_gpio(
	const struct gpio_fast_spec_espressif_esp32_gpio *spec)
{
	return sys_read32(spec->in_reg) & spec->pin_mask;
}

/**
 * @brief Disable output (switch to high-Z/input) via enable_w1tc register.
 *
 * This is a single atomic register write. Note that input enable via
 * IO_MUX must have been configured at init time, so this only disables
 * the output driver.
 */
static ALWAYS_INLINE void gpio_fast_set_input_espressif_esp32_gpio(
	const struct gpio_fast_spec_espressif_esp32_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->dir_clr);
}

/**
 * @brief Enable output via enable_w1ts register.
 *
 * This is a single atomic register write that re-enables the output
 * driver for the masked pins.
 */
static ALWAYS_INLINE void gpio_fast_set_output_espressif_esp32_gpio(
	const struct gpio_fast_spec_espressif_esp32_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->dir_set);
}

int gpio_fast_configure_espressif_esp32_gpio(struct gpio_fast_spec_espressif_esp32_gpio *fast,
					     const struct device *port,
					     gpio_port_pins_t pin_mask,
					     gpio_flags_t flags);
int gpio_fast_pre_stream_espressif_esp32_gpio(const void *spec);
int gpio_fast_post_stream_espressif_esp32_gpio(const void *spec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_FAST_ESPRESSIF_ESP32_GPIO_H_ */
