/*
 * Copyright (c) 2025 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RASPBERRYPI_RPI_PICO_COMMON_BINARY_INFO_H_
#define ZEPHYR_SOC_RASPBERRYPI_RPI_PICO_COMMON_BINARY_INFO_H_

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/dt-bindings/pinctrl/rpi-pico-pinctrl-common.h>

#include <soc.h>

#include <boot_stage2/config.h>
#include <pico/binary_info.h>

#include <version.h>

#define BI_PROGRAM_NAME           CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_PROGRAM_NAME
#define BI_PROGRAM_DESCRIPTION    CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_PROGRAM_DESCRIPTION
#define BI_PROGRAM_URL            CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_PROGRAM_URL
#define BI_PROGRAM_VERSION_STRING CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_PROGRAM_VERSION_STRING

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PICO_BOARD
#define BI_PICO_BOARD CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_PICO_BOARD
#else
#define BI_PICO_BOARD CONFIG_BOARD
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_SDK_VERSION_STRING_OVERRIDDEN
#define BI_SDK_VERSION_STRING CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_SDK_VERSION_STRING
#else
#define BI_SDK_VERSION_STRING "zephyr-" STRINGIFY(BUILD_VERSION)
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_BUILD_DATE_OVERRIDDEN
#define BI_PROGRAM_BUILD_DATE CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_PROGRAM_BUILD_DATE
#else
#define BI_PROGRAM_BUILD_DATE __DATE__
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_BOOT_STAGE2_NAME
#define BI_BOOT_STAGE2_NAME CONFIG_RPI_PICO_BINARY_INFO_OVERRIDE_BOOT_STAGE2_NAME
#else
#define BI_BOOT_STAGE2_NAME PICO_BOOT_STAGE2_NAME
#endif

/* Utilities for pin encoding calcluation */

#ifdef CONFIG_SOC_RP2040
#define MAX_PIN_ENTRIES       4
#define ENCODE_BASE_IDX       1
#define ENCODE_BYTES          5
#define ENCODE_PADDING        2
#define ENCODE_PINS_WITH_FUNC __bi_encoded_pins_with_func
#else
#define MAX_PIN_ENTRIES       6
#define ENCODE_BASE_IDX       1
#define ENCODE_BYTES          8
#define ENCODE_PADDING        0
#define ENCODE_PINS_WITH_FUNC __bi_encoded_pins_64_with_func
#endif

/* Get group-wide pin functions */

#define PIN_FUNC_OFFSET     3
#define PIN_FUNC(n, _, idx) ((DT_PROP_BY_IDX(n, pinmux, idx)) & RP2_ALT_FUNC_MASK)
#define PIN_GROUP_FUNC(n)   (DT_FOREACH_PROP_ELEM_SEP(n, pinmux, PIN_FUNC, (|)))
#define PIN_GROUP_HEADER(n) (BI_PINS_ENCODING_MULTI | (PIN_GROUP_FUNC(n) << PIN_FUNC_OFFSET))

/* Encode the pins in a group */

#define PINMUX_COUNT(n) COND_CODE_1(DT_NODE_HAS_PROP(n, pinmux), (DT_PROP_LEN(n, pinmux)), (0))
#define PIN_NUM(n, idx) (((DT_PROP_BY_IDX(n, pinmux, idx)) >> RP2_PIN_NUM_POS) & RP2_PIN_NUM_MASK)

#define ENCODE_OFFSET(idx)          (((idx + ENCODE_BASE_IDX) * ENCODE_BYTES) + ENCODE_PADDING)
#define ENCODE_PIN(n, idx)          ((uint64_t)PIN_NUM(n, idx) << ENCODE_OFFSET(idx))
#define ENCODE_TERMINATE(n, i, end) ((((i) + 1) == (end)) ? ENCODE_PIN(n, i + 1) : 0)
#define ENCODE_EACH_PIN(n, p, i, end)                                                              \
	(DT_PROP_HAS_IDX(n, p, i) ? ENCODE_PIN(n, i) : 0) | ENCODE_TERMINATE(n, i, end)
#define ENCODE_PIN_GROUP(n)                                                                        \
	DT_FOREACH_PROP_ELEM_SEP_VARGS(n, pinmux, ENCODE_EACH_PIN, (|), PINMUX_COUNT(n))

/*
 * Macro for checking pin settings
 * Checks that a group consists of a single pin-function.
 */

#define PIN_FUNC_IS(n, p, i, func) (PIN_FUNC(n, p, i) == func)
#define ALL_PINS_FUNC_IS_SAME(n)                                                                   \
	(DT_FOREACH_PROP_ELEM_SEP_VARGS(n, pinmux, PIN_FUNC_IS, (&&), PIN_GROUP_FUNC(n)))

/*
 * Create binary info for pin-cfg group.
 */

#define DECLARE_PINCFG_GROUP(n)                                                                    \
	BUILD_ASSERT(PINMUX_COUNT(n) > 0, "Group must contain at least one pin");                  \
	BUILD_ASSERT(PINMUX_COUNT(n) <= MAX_PIN_ENTRIES, "Too many pins in group");                \
	BUILD_ASSERT(ALL_PINS_FUNC_IS_SAME(n), "Group pins must share identical function");        \
	bi_decl(ENCODE_PINS_WITH_FUNC(PIN_GROUP_HEADER(n) | ENCODE_PIN_GROUP(n)))

/*
 * Scan pin-cfg and the groups it contains.
 * Each enumerates its child elements, but since the same macro cannot be used within nesting,
 * DT_FOREACH_CHILD_VARGS and DT_FOREACH_CHILD_SEP_VARGS are used accordingly.
 */

#define PINMUX_NUM_IF_MATCH_GROUP(node, g_idx)                                                     \
	COND_CODE_1(IS_EQ(DT_NODE_CHILD_IDX(node), g_idx), (DT_PROP_LEN(node, pinmux)), ())
#define PINMUX_NUM_IF_MATCH_PINCFG(node, pcfg_idx, g_idx)                                          \
	COND_CODE_1(IS_EQ(DT_NODE_CHILD_IDX(node), pcfg_idx), \
		    (DT_FOREACH_CHILD_SEP_VARGS(node, PINMUX_NUM_IF_MATCH_GROUP, (), g_idx)), ())
#define PINMUX_NUM_OF_PINCFG_GROUP(node, pcfg_idx, g_idx)                                          \
	DT_FOREACH_CHILD_VARGS(node, PINMUX_NUM_IF_MATCH_PINCFG, pcfg_idx, g_idx)

#define BINARY_INFO_IF_MATCH_GROUP(node, g_idx)                                                    \
	COND_CODE_1(IS_EQ(DT_NODE_CHILD_IDX(node), g_idx), (DECLARE_PINCFG_GROUP(node)), ())
#define BINARY_INFO_IF_MATCH_PINCFG(node, pcfg_idx, g_idx)                                         \
	COND_CODE_1(IS_EQ(DT_NODE_CHILD_IDX(node), pcfg_idx),                                      \
		    (DT_FOREACH_CHILD_SEP_VARGS(node, BINARY_INFO_IF_MATCH_GROUP, (), g_idx)), ())
#define BINARY_INFO_FROM_PINCFG_GROUP(node, pcfg_idx, g_idx)                                       \
	COND_CODE_1(IS_EMPTY(PINMUX_NUM_OF_PINCFG_GROUP(node, pcfg_idx, g_idx)), (),               \
		    (DT_FOREACH_CHILD_VARGS(node, BINARY_INFO_IF_MATCH_PINCFG, pcfg_idx, g_idx)))

#endif /* ZEPHYR_SOC_RASPBERRYPI_RPI_PICO_COMMON_BINARY_INFO_H_ */
