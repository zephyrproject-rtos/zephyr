/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_RT266X_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_RT266X_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * i.MX RT266x IOMUXC pad configuration. One combined PIO register per pad, not
 * the RT11xx/RT118x SW_MUX_CTL + SW_PAD_CTL pair, so the generated pinctrl tuple
 * <mux_register mux_mode input_register input_daisy config_register> carries
 * config_register == mux_register.
 *
 * PIO field layout (periph/PERI_IOMUXC.h):
 *   MUX_MODE  : bits [3:0]   pad alternate-function select
 *   PULLENA   : bits [5:4]   pull enable / pull select (00=disable)
 *   IBENA     : bit  [7]     input buffer (receiver) enable
 *   SLEWRATE  : bits [9:8]   slew rate
 *   ODENA     : bit  [10]    open-drain enable
 *   DRIVE     : bits [13:12] drive strength
 */

#define MCUX_RT266X_MUX_MODE_SHIFT 0
#define MCUX_RT266X_PULLENA_SHIFT  4
#define MCUX_RT266X_IBENA_SHIFT    7
#define MCUX_RT266X_SLEWRATE_SHIFT 8
#define MCUX_RT266X_ODENA_SHIFT    10
#define MCUX_RT266X_DRIVE_SHIFT    12

/*
 * PULLENA field encoding (2 bits, from periph/PERI_IOMUXC.h):
 *   0b00 - pull/keeper disabled
 *   0b01 - pull-down enabled
 *   0b10 - pull-up enabled
 *   0b11 - bus keeper (retain last state)
 */
#define MCUX_RT266X_PULL_DISABLE 0x0
#define MCUX_RT266X_PULL_DOWN    0x1
#define MCUX_RT266X_PULL_UP      0x2

/* Shift to a bit not used by the IOMUXC PIO register; consumed by the driver */
#define MCUX_IMX_INPUT_ENABLE_SHIFT 31
#define MCUX_IMX_INPUT_ENABLE(x)    (((x) >> MCUX_IMX_INPUT_ENABLE_SHIFT) & 0x1)

/*
 * Two PIO field layouts, told apart by the pad's mux register address:
 *
 *   MAIN IOMUXC + 0x180..0x2AC  (PIO5/6/7, 46 pads)  SLEWRATE [9:8], DRIVE
 *                                                    [13:12], no ODENA
 *   everything else             (MAIN PIO2/3/4, WAKE PIO1, VBAT PIO0)
 *                                                    SLEWRATE [8], ODENA [10],
 *                                                    no DRIVE
 *
 * The entries carry that address, so the layout is a compile-time property of
 * each pin. A field the pad lacks is left unwritten instead of landing in
 * reserved bits; the binding documents which pads support what. The alias bit is
 * masked off, so a table generated against either IOMUXC alias classifies alike.
 */
#define MCUX_RT266X_ALIAS_MASK     0x10000000U
#define MCUX_RT266X_PAD_ADDR(addr) ((addr) & ~MCUX_RT266X_ALIAS_MASK)
#define MCUX_RT266X_WIDE_PAD_FIRST 0x421C0180U /* MAIN IOMUXC PIO5_0 */
#define MCUX_RT266X_WIDE_PAD_LAST  0x421C02ACU /* MAIN IOMUXC PIO7_11 */

#define MCUX_RT266X_PAD_IS_WIDE(pin_id)                                                            \
	((MCUX_RT266X_PAD_ADDR(DT_PROP_BY_IDX(pin_id, pinmux, 0)) >=                               \
	  MCUX_RT266X_WIDE_PAD_FIRST) &&                                                           \
	 (MCUX_RT266X_PAD_ADDR(DT_PROP_BY_IDX(pin_id, pinmux, 0)) <= MCUX_RT266X_WIDE_PAD_LAST))

/*
 * A 1-bit SLEWRATE has only slowest and fastest, so anything above "low" maps to
 * the fast setting. Writing the 2-bit index unchanged would put "fast" (0b10)
 * back at 0 -- slower than "medium".
 */
#define MCUX_RT266X_SLEWRATE_VAL(group_id, pin_id)                                                 \
	(MCUX_RT266X_PAD_IS_WIDE(pin_id) ? DT_ENUM_IDX_OR(group_id, slew_rate, 0)                  \
					 : (DT_ENUM_IDX_OR(group_id, slew_rate, 0) != 0 ? 1 : 0))

/* clang-format off */
#define Z_PINCTRL_MCUX_RT266X_PULL(node_id)						\
	IF_ENABLED(DT_PROP(node_id, bias_pull_up),					\
		(MCUX_RT266X_PULL_UP << MCUX_RT266X_PULLENA_SHIFT) |)			\
	IF_ENABLED(DT_PROP(node_id, bias_pull_down),					\
		(MCUX_RT266X_PULL_DOWN << MCUX_RT266X_PULLENA_SHIFT) |)			\
	(MCUX_RT266X_PULL_DISABLE << MCUX_RT266X_PULLENA_SHIFT)

#define Z_PINCTRL_MCUX_RT266X_PINCFG(group_id, pin_id)					\
	(Z_PINCTRL_MCUX_RT266X_PULL(group_id)) |					\
	(MCUX_RT266X_PAD_IS_WIDE(pin_id) ?						\
		(DT_ENUM_IDX_OR(group_id, drive_strength, 0)				\
			<< MCUX_RT266X_DRIVE_SHIFT) : 0U) |				\
	(MCUX_RT266X_SLEWRATE_VAL(group_id, pin_id)					\
		<< MCUX_RT266X_SLEWRATE_SHIFT) |					\
	(MCUX_RT266X_PAD_IS_WIDE(pin_id) ? 0U :						\
		(DT_PROP(group_id, drive_open_drain) << MCUX_RT266X_ODENA_SHIFT)) |	\
	(DT_PROP(group_id, input_enable) << MCUX_IMX_INPUT_ENABLE_SHIFT)
/* clang-format on */

/* This struct must be present. It is used by the mcux gpio/pinctrl driver. */
struct pinctrl_soc_pinmux {
	uint32_t mux_register;    /* IOMUXC PIO (mux) register */
	uint32_t config_register; /* IOMUXC PIO (pad config) register; == mux_register */
	uint32_t input_register;  /* IOMUXC SELECT_INPUT DAISY register */
	uint8_t mux_mode: 4;      /* Mux value for the PIO register */
	uint32_t input_daisy: 4;  /* Mux value for SELECT_INPUT_DAISY register */
	uint8_t pue_mux: 1;       /* Unused on RT266x (single register layout) */
	uint8_t pdrv_mux: 1;      /* Unused on RT266x (single register layout) */
};

struct pinctrl_soc_pin {
	struct pinctrl_soc_pinmux pinmux;
	uint32_t pin_ctrl_flags; /* value to write to the IOMUXC PIO register */
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define MCUX_IMX_PINMUX(node_id)                                                                   \
	{                                                                                          \
		.mux_register = DT_PROP_BY_IDX(node_id, pinmux, 0),                                \
		.config_register = DT_PROP_BY_IDX(node_id, pinmux, 4),                             \
		.input_register = DT_PROP_BY_IDX(node_id, pinmux, 2),                              \
		.mux_mode = DT_PROP_BY_IDX(node_id, pinmux, 1),                                    \
		.input_daisy = DT_PROP_BY_IDX(node_id, pinmux, 3),                                 \
		.pue_mux = 0,                                                                      \
		.pdrv_mux = 0,                                                                     \
	}

#define Z_PINCTRL_PINMUX(group_id, pin_prop, idx)                                                  \
	MCUX_IMX_PINMUX(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))

#define Z_PINCTRL_STATE_PIN_INIT(group_id, pin_prop, idx)                                          \
	{                                                                                          \
		.pinmux = Z_PINCTRL_PINMUX(group_id, pin_prop, idx),                               \
		.pin_ctrl_flags = Z_PINCTRL_MCUX_RT266X_PINCFG(                                    \
			group_id, DT_PHANDLE_BY_IDX(group_id, pin_prop, idx)),                     \
	},

/*
 * Asking for a field the pad lacks is a mistake, not a no-op: the code above just
 * does not write it, so the request vanishes. These assertions fail the build
 * instead.
 *
 * File scope is forced: PAD_IS_WIDE() is a C constant expression but not a
 * preprocessor literal, so COND_CODE_1()/IS_ENABLED() cannot branch on it, and
 * pin_ctrl_flags is a static initializer. Presence is the test for drive-strength
 * (a string enum with no default) and value is the test for drive-open-drain (a
 * boolean, always materialized).
 */
#define MCUX_RT266X_ASSERT_PIN(group_id, pin_prop, idx)                                            \
	BUILD_ASSERT(!(DT_NODE_HAS_PROP(group_id, drive_strength) &&                               \
		       !MCUX_RT266X_PAD_IS_WIDE(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))),      \
		     "drive-strength on a pad with no DRIVE field: only the PIO5/PIO6/PIO7 pads "  \
		     "have one. Remove it from this pinctrl group.");                              \
	BUILD_ASSERT(!(DT_PROP(group_id, drive_open_drain) &&                                      \
		       MCUX_RT266X_PAD_IS_WIDE(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))),       \
		     "drive-open-drain on a pad with no ODENA field: the PIO5/PIO6/PIO7 pads "     \
		     "have none. Remove it from this pinctrl group.");

/*
 * A group carries the pinmux phandles; a state node carries the groups; the
 * pinctrl node carries the states. The two child levels deliberately use two
 * DIFFERENT iteration macros: the preprocessor will not expand a macro inside its
 * own expansion, so nesting DT_FOREACH_CHILD in DT_FOREACH_CHILD leaves the inner
 * one unexpanded and the header stops compiling.
 */
#define MCUX_RT266X_ASSERT_GROUP(group_id)                                                         \
	DT_FOREACH_PROP_ELEM(group_id, pinmux, MCUX_RT266X_ASSERT_PIN)

#define MCUX_RT266X_ASSERT_PINCTRL(node_id)                                                        \
	DT_FOREACH_CHILD_VARGS(node_id, DT_FOREACH_CHILD_STATUS_OKAY, MCUX_RT266X_ASSERT_GROUP)

DT_FOREACH_STATUS_OKAY(nxp_mcux_rt266x_pinctrl, MCUX_RT266X_ASSERT_PINCTRL)

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_RT266X_H_ */
