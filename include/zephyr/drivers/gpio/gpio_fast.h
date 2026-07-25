/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Gerson Fernando Budke
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO API extension for timing-critical operations.
 *
 * Provides sub-microsecond GPIO operations for bit-bang drivers.
 * All pin operations are ALWAYS_INLINE and vendor-specific.
 *
 * Consumers resolve the backend once with GPIO_FAST_COMPAT(node_id, prop),
 * then use dispatch macros that token-paste onto backend-namespaced symbols:
 *   GPIO_FAST_DISPATCH_TYPE(compat): struct type for the spec
 *   GPIO_FAST_DISPATCH_CALL(compat, fn, spec): call fn(spec)
 *   GPIO_FAST_DISPATCH_VAL(compat, sym): cycle cost constant
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_FAST_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_FAST_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fast GPIO API
 * @defgroup gpio_fast_interface Fast GPIO
 * @ingroup gpio_interface
 * @since 4.5
 * @{
 */

#ifdef CONFIG_GPIO_FAST

/*
 * Include all active vendor-specific fast GPIO backends.
 *
 * The build system (drivers/gpio/CMakeLists.txt) adds each active
 * vendor directory to the include path. Each backend header has a
 * unique filename keyed by its DT compatible token, so multiple
 * backends can coexist in a single build.
 *
 * The generic fallback is always included. Consumers opt into it
 * per-node by setting fast-gpio-backend = "generic" in DT.
 *
 * Out-of-tree backends are included via zephyr_gpio_fast_impl_oot.h
 * when CONFIG_GPIO_FAST_OUT_OF_TREE_BACKEND is enabled. The OOT
 * module provides its own zephyr_gpio_fast_impl_oot.h on the include
 * path (via zephyr_include_directories in its CMakeLists.txt).
 * The dispatch macros resolve via token-paste automatically: no
 * dispatch registration is needed in the OOT header.
 */

#ifdef CONFIG_GPIO_NRFX
#include <zephyr_gpio_fast_nordic_nrf_gpio.h>
#endif

#ifdef CONFIG_GPIO_SAM0
#include <zephyr_gpio_fast_atmel_sam0_gpio.h>
#endif

#ifdef CONFIG_GPIO_STM32
#include <zephyr_gpio_fast_st_stm32_gpio.h>
#endif

#ifdef CONFIG_GPIO_ESP32
#include <zephyr_gpio_fast_espressif_esp32_gpio.h>
#endif

#ifdef CONFIG_GPIO_RPI_PICO
#include <zephyr_gpio_fast_raspberrypi_pico_gpio.h>
#endif

#ifdef CONFIG_GPIO_MCUX_IGPIO
#include <zephyr_gpio_fast_nxp_imx_gpio.h>
#endif

#include <zephyr_gpio_fast_generic.h>

#ifdef CONFIG_GPIO_FAST_OUT_OF_TREE_BACKEND
#include <zephyr_gpio_fast_impl_oot.h>
#endif

/*
 * Backend dispatch macros.
 *
 * Consumers resolve their backend once with GPIO_FAST_COMPAT(node_id, prop),
 * which checks for a "fast-gpio-backend" DT string property first, then
 * falls back to the GPIO controller's DT compatible. The result is a
 * bare C token passed to the dispatch macros, which token-paste it
 * onto backend-namespaced symbols. No per-vendor enumeration is
 * needed; new backends (in-tree or OOT) work automatically as long as
 * their header provides the correctly-named symbols.
 */

/**
 * @brief Resolve the fast GPIO backend compat token for a DT node.
 *
 * If the node has a ``fast-gpio-backend`` string property, its value
 * is used as the backend token. Otherwise, the GPIO controller's DT
 * compatible (from the specified phandle-array property) is used.
 *
 * Call this once and reuse the result in all dispatch macros:
 *
 * .. code-block:: c
 *
 *    #define MY_BACKEND GPIO_FAST_COMPAT(MY_NODE, gpios)
 *    struct GPIO_FAST_DISPATCH_TYPE(MY_BACKEND) spec;
 *    GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_set, &spec);
 *
 * @param node_id DT node identifier.
 * @param prop    GPIO phandle-array property name (e.g. gpios).
 */
#define GPIO_FAST_COMPAT(node_id, prop) \
	DT_STRING_TOKEN_OR(node_id, fast_gpio_backend, \
			   DT_BINDING_COMPAT(DT_GPIO_CTLR(node_id, prop)))

/**
 * @brief Get the gpio_fast_spec struct type for a backend.
 *
 * Usage:
 *   #define MY_BACKEND GPIO_FAST_COMPAT(MY_NODE, gpios)
 *   struct GPIO_FAST_DISPATCH_TYPE(MY_BACKEND) my_spec;
 *
 * @param compat Backend compat token from GPIO_FAST_COMPAT().
 */
#define GPIO_FAST_DISPATCH_TYPE(compat) \
	_CONCAT(gpio_fast_spec_, compat)

/**
 * @brief Dispatch a fast GPIO inline call to the correct backend.
 *
 * Usage:
 *   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_set, &spec);
 *   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_configure,
 *                           &spec, port, pin_mask, flags);
 *
 * @param compat Backend compat token from GPIO_FAST_COMPAT().
 * @param fn     Base function name (e.g. gpio_fast_set).
 * @param ...    Arguments to pass to fn.
 */
#define GPIO_FAST_DISPATCH_CALL(compat, fn, ...) \
	_CONCAT(fn##_, compat)(__VA_ARGS__)

/**
 * @brief Dispatch a fast GPIO constant to the correct backend.
 *
 * Usage:
 *   GPIO_FAST_DISPATCH_VAL(MY_BACKEND, GPIO_FAST_SET_CYCLES)
 *
 * @param compat Backend compat token from GPIO_FAST_COMPAT().
 * @param sym    Base constant name (e.g. GPIO_FAST_SET_CYCLES).
 */
#define GPIO_FAST_DISPATCH_VAL(compat, sym) \
	_CONCAT(sym##_, compat)

/**
 * @brief CPU clock frequency used for NOP cycle calculations.
 *
 * On some platforms (e.g. nRF), CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
 * reflects the system timer (32 kHz RTC), not the CPU clock (64 MHz).
 * NOPs execute at CPU speed, so vendor headers may define
 * GPIO_FAST_CPU_FREQ to override. If not defined, we try the DT
 * clock-frequency property on the CPU node, then fall back to
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC.
 */
#ifndef GPIO_FAST_CPU_FREQ
#if DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#define GPIO_FAST_CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#else
#define GPIO_FAST_CPU_FREQ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif
#endif

/**
 * @brief Convert nanoseconds to CPU cycles at compile time.
 *
 * @param ns Duration in nanoseconds.
 * @return Equivalent number of CPU cycles.
 */
#define GPIO_FAST_NS_TO_CYCLES(ns)                                                                 \
	((uint32_t)((uint64_t)(ns) * GPIO_FAST_CPU_FREQ / 1000000000ULL))

/*
 * GPIO_FAST_SEND_STREAM_ATTR_<compat> is defined per-backend.
 * Vendors that require RAM execution (SAM0, RP2040) define theirs
 * as __ramfunc; others define theirs as empty. Consumers access it
 * via GPIO_FAST_DISPATCH_VAL(compat, GPIO_FAST_SEND_STREAM_ATTR).
 */

/**
 * @brief Delay for a compile-time-constant number of NOP instructions,
 *        accepting C constant expressions (not just preprocessor literals).
 *
 * Uses pragma-based loop unrolling to produce straight-line NOP
 * instructions from any compile-time constant expression. This allows
 * arithmetic like:
 *
 *   GPIO_FAST_DELAY_NOPS(GPIO_FAST_NS_TO_CYCLES(400)
 *                        - GPIO_FAST_DISPATCH_VAL(MY_BACKEND,
 *                              GPIO_FAST_CLEAR_CYCLES));
 *
 * The compiler fully unrolls the loop, producing straight-line NOP
 * instructions with zero loop overhead. The argument can be any
 * compile-time constant expression (including arithmetic).
 *
 * Pragma GCC unroll works in both GCC and Clang. We use this in place of
 * LISTIFY() because then we can compute the NOP count at compile time based on
 * the vendor-specific cycle costs. Otherwise, this has to be a literal, which
 * pushes the math to Kconfig, which is much less flexible and harder to maintain.
 *
 * @param count Number of NOP instructions (compile-time constant expression).
 */
#define GPIO_FAST_DELAY_NOPS(count) \
	do {\
		_Pragma("GCC unroll 256") \
		for (unsigned int _nop_i = 0; _nop_i < (unsigned int)(count); _nop_i++) { \
			__asm volatile("nop"); \
		} \
	} while (0)

/**
 * @brief Configure a fast GPIO spec from a gpio_dt_spec via dispatch.
 *
 * Convenience macro that dispatches to the correct backend's
 * gpio_fast_configure function.
 *
 * @param compat  Backend compat token from GPIO_FAST_COMPAT().
 * @param fast    Pointer to the backend-specific gpio_fast_spec.
 * @param dt_spec Pointer to a gpio_dt_spec.
 * @param flags   GPIO configuration flags (e.g. GPIO_OUTPUT).
 * @return 0 on success, negative errno on failure.
 */
#define GPIO_FAST_CONFIGURE_DT(compat, fast, dt_spec, flags)              \
	GPIO_FAST_DISPATCH_CALL(compat, gpio_fast_configure,              \
		fast, (dt_spec)->port, BIT((dt_spec)->pin), flags)

/*
 * gpio_fast_pre_stream_<compat>() / gpio_fast_post_stream_<compat>()
 *
 * Per-backend hooks called before and after timing-critical bit-bang
 * loops. Vendors implement them to enable clocks, request oscillators,
 * etc. before timing-critical bit-bang loops. Backends that need no
 * setup/teardown return 0.

 *
 * Consumers call via dispatch:
 *   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_pre_stream, spec);
 *   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_post_stream, spec);
 */

#endif /* CONFIG_GPIO_FAST */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_FAST_H_ */
