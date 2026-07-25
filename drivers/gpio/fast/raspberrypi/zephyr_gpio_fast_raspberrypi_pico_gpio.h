/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO implementation for Raspberry Pi Pico (RP2040/RP2350).
 *
 * The RP2040/RP2350 SIO (Single-cycle IO) block provides dedicated
 * atomic set/clear/toggle registers for both output and output-enable,
 * all accessible in a single CPU cycle via the SIO bus.
 *
 * Hardware toggle (XOR) register is available, so no read-modify-write
 * needed for toggle.
 *
 * All types, functions, and constants are namespaced by the DT
 * compatible token (raspberrypi_pico_gpio) for multi-backend
 */

#ifndef ZEPHYR_DRIVERS_GPIO_FAST_RASPBERRYPI_PICO_GPIO_H_
#define ZEPHYR_DRIVERS_GPIO_FAST_RASPBERRYPI_PICO_GPIO_H_

#include <zephyr/arch/common/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RP2040 requires __ramfunc for timing-critical bit-bang loops.
 *
 * The RP2040 XIP cache covers steady-state execution, but the first
 * pass through a large NOP block suffers cache cold-start penalties
 * that corrupt the first byte's timing. Running from RAM avoids this.
 */
#define GPIO_FAST_SEND_STREAM_ATTR_raspberrypi_pico_gpio __ramfunc

/*
 * Standalone call cost in CPU cycles for RPI Pico fast GPIO.
 *
 * Cycle costs are grouped by SOC series. Adding a new series
 * requires an explicit entry; unsupported series trigger #error.
 *
 * Cortex-M0+ with SIO single-cycle bus. LDR from SRAM = 2 cy,
 * STR to SIO = 2 cy (1 execute + 1 single-cycle SIO access).
 * Counts include loading spec fields (pin_mask, register addresses).
 */
#if defined(CONFIG_SOC_SERIES_RP2040) || \
	(defined(CONFIG_SOC_SERIES_RP2350) && defined(CONFIG_ARM)) || \
	(defined(CONFIG_SOC_SERIES_RP2350) && defined(CONFIG_RISCV))
/* Cortex-M0+ (RP2040) / Cortex-M33 / RISC-V Hazard3 (RP2350) */

/** @brief CPU cycles for gpio_fast_set() on Raspberry Pi Pico. */
#define GPIO_FAST_SET_CYCLES_raspberrypi_pico_gpio        6  /* LDR pin + LDR set_reg + STR (SIO) */
/** @brief CPU cycles for gpio_fast_clear() on Raspberry Pi Pico. */
#define GPIO_FAST_CLEAR_CYCLES_raspberrypi_pico_gpio      6  /* LDR pin + LDR clr_reg + STR (SIO) */
/** @brief CPU cycles for gpio_fast_toggle() on Raspberry Pi Pico. */
#define GPIO_FAST_TOGGLE_CYCLES_raspberrypi_pico_gpio     6  /* LDR pin + LDR tgl_reg + STR (SIO) */
/** @brief CPU cycles for gpio_fast_get() on Raspberry Pi Pico. */
/* LDR in_reg + LDR [in] + LDR pin + AND */
#define GPIO_FAST_GET_CYCLES_raspberrypi_pico_gpio        7
/** @brief CPU cycles for gpio_fast_set_input() on Raspberry Pi Pico. */
#define GPIO_FAST_SET_INPUT_CYCLES_raspberrypi_pico_gpio  6  /* LDR pin + LDR oe_clr + STR (SIO) */
/** @brief CPU cycles for gpio_fast_set_output() on Raspberry Pi Pico. */
#define GPIO_FAST_SET_OUTPUT_CYCLES_raspberrypi_pico_gpio 6  /* LDR pin + LDR oe_set + STR (SIO) */

#else
#error "Unsupported Raspberry Pi Pico SOC series for fast GPIO"
#endif

/**
 * @brief Opaque fast GPIO spec for RP2040/RP2350.
 *
 * Pre-computed SIO register addresses and pin mask.
 * The correct bank-specific registers (low vs high) are resolved
 * at configure time. Consumers must not access fields directly.
 */
struct gpio_fast_spec_raspberrypi_pico_gpio {
	/** SIO gpio_set / gpio_hi_set */
	mem_addr_t set_reg;
	/** SIO gpio_clr / gpio_hi_clr */
	mem_addr_t clr_reg;
	/** SIO gpio_togl / gpio_hi_togl */
	mem_addr_t tgl_reg;
	/** SIO gpio_in / gpio_hi_in */
	mem_addr_t in_reg;
	/** SIO gpio_oe_set / gpio_hi_oe_set */
	mem_addr_t oe_set;
	/** SIO gpio_oe_clr / gpio_hi_oe_clr */
	mem_addr_t oe_clr;
	/** Pin bitmask */
	uint32_t pin_mask;
};

/** @brief Set all masked pins HIGH via SIO set register. */
static ALWAYS_INLINE void gpio_fast_set_raspberrypi_pico_gpio(
	const struct gpio_fast_spec_raspberrypi_pico_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->set_reg);
}

/** @brief Set all masked pins LOW via SIO clear register. */
static ALWAYS_INLINE void gpio_fast_clear_raspberrypi_pico_gpio(
	const struct gpio_fast_spec_raspberrypi_pico_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->clr_reg);
}

/** @brief Toggle all masked pins via SIO hardware XOR register. */
static ALWAYS_INLINE void gpio_fast_toggle_raspberrypi_pico_gpio(
	const struct gpio_fast_spec_raspberrypi_pico_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->tgl_reg);
}

/** @brief Read raw physical state of masked pins from SIO input register. */
static ALWAYS_INLINE gpio_port_pins_t
gpio_fast_get_raspberrypi_pico_gpio(
	const struct gpio_fast_spec_raspberrypi_pico_gpio *spec)
{
	return sys_read32(spec->in_reg) & spec->pin_mask;
}

/** @brief Disable output driver (switch to input/high-Z) via SIO oe_clr. */
static ALWAYS_INLINE void gpio_fast_set_input_raspberrypi_pico_gpio(
	const struct gpio_fast_spec_raspberrypi_pico_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->oe_clr);
}

/** @brief Enable output driver via SIO oe_set. */
static ALWAYS_INLINE void gpio_fast_set_output_raspberrypi_pico_gpio(
	const struct gpio_fast_spec_raspberrypi_pico_gpio *spec)
{
	sys_write32(spec->pin_mask, spec->oe_set);
}

int gpio_fast_configure_raspberrypi_pico_gpio(struct gpio_fast_spec_raspberrypi_pico_gpio *fast,
					      const struct device *port,
					      gpio_port_pins_t pin_mask,
					      gpio_flags_t flags);
int gpio_fast_pre_stream_raspberrypi_pico_gpio(const void *spec);
int gpio_fast_post_stream_raspberrypi_pico_gpio(const void *spec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_FAST_RASPBERRYPI_PICO_GPIO_H_ */
