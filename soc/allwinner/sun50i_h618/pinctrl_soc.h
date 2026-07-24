/*
 * Copyright (c) 2026 Khai Do
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ALLWINNER_SUN50I_H618_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ALLWINNER_SUN50I_H618_PINCTRL_SOC_H_

#include <zephyr/types.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/util_listify.h>
#define SUNXI_PINCTRL_MUX_IN     0
#define SUNXI_PINCTRL_MUX_OUT    1
#define SUNXI_PINCTRL_MUX_EINT   6

#include <zephyr/dt-bindings/pinctrl/sunxi-pinctrl.h>

typedef uint32_t pinctrl_soc_pin_t;

#define SUNXI_PIN_BANK_MASK  GENMASK(15, 8)
#define SUNXI_PIN_PIN_MASK   GENMASK(7, 0)
#define SUNXI_PIN_MUX_MASK   GENMASK(19, 16)
#define SUNXI_PIN_PULL_MASK  GENMASK(22, 20)
#define SUNXI_PIN_DRV_MASK   GENMASK(25, 23)

#define SUNXI_PIN(bank, pin, mux) \
	(FIELD_PREP(SUNXI_PIN_BANK_MASK, bank) | \
	 FIELD_PREP(SUNXI_PIN_PIN_MASK, pin) | \
	 FIELD_PREP(SUNXI_PIN_MUX_MASK, mux) | \
	 FIELD_PREP(SUNXI_PIN_PULL_MASK, SUNXI_PULL_KEEP) | \
	 FIELD_PREP(SUNXI_PIN_DRV_MASK, SUNXI_DRV_KEEP))

#define SUNXI_PIN_WITH_PULL(bank, pin, mux, pull) \
	((SUNXI_PIN(bank, pin, mux) & ~SUNXI_PIN_PULL_MASK) | \
	 FIELD_PREP(SUNXI_PIN_PULL_MASK, pull))

#define SUNXI_PIN_WITH_DRV(bank, pin, mux, drv) \
	((SUNXI_PIN(bank, pin, mux) & ~SUNXI_PIN_DRV_MASK) | \
	 FIELD_PREP(SUNXI_PIN_DRV_MASK, drv))

#define SUNXI_PIN_WITH_PULL_DRV(bank, pin, mux, pull, drv) \
	((SUNXI_PIN(bank, pin, mux) & ~(SUNXI_PIN_PULL_MASK | SUNXI_PIN_DRV_MASK)) | \
	 FIELD_PREP(SUNXI_PIN_PULL_MASK, pull) | \
	 FIELD_PREP(SUNXI_PIN_DRV_MASK, drv))

#define SUNXI_GET_BANK(p) FIELD_GET(SUNXI_PIN_BANK_MASK, p)
#define SUNXI_GET_PIN(p)  FIELD_GET(SUNXI_PIN_PIN_MASK, p)
#define SUNXI_GET_MUX(p)  FIELD_GET(SUNXI_PIN_MUX_MASK, p)
#define SUNXI_GET_PULL(p) FIELD_GET(SUNXI_PIN_PULL_MASK, p)
#define SUNXI_GET_DRV(p)  FIELD_GET(SUNXI_PIN_DRV_MASK, p)

#define Z_PINCTRL_SUNXI_PULL(node_id) \
	COND_CODE_1(DT_PROP(node_id, bias_pull_up), (SUNXI_PULL_UP), \
	(COND_CODE_1(DT_PROP(node_id, bias_pull_down), (SUNXI_PULL_DOWN), \
	(COND_CODE_1(DT_PROP(node_id, bias_disable), (SUNXI_PULL_NONE), \
	(SUNXI_PULL_KEEP))))))

#define Z_PINCTRL_SUNXI_DRV_VAL(val) \
	((val) == 10 ? SUNXI_DRIVE_10MA : \
	 (val) == 20 ? SUNXI_DRIVE_20MA : \
	 (val) == 30 ? SUNXI_DRIVE_30MA : \
	 (val) == 40 ? SUNXI_DRIVE_40MA : SUNXI_DRV_KEEP)

#define Z_PINCTRL_SUNXI_DRV(node_id) \
	Z_PINCTRL_SUNXI_DRV_VAL(DT_PROP_OR(node_id, drive_strength, 0))

#define SUNXI_PINCTRL_EXTRACT(idx, group_node) \
	SUNXI_PIN_WITH_PULL_DRV( \
		SUNXI_GET_BANK(DT_PROP_BY_IDX(group_node, pinmux, idx)), \
		SUNXI_GET_PIN(DT_PROP_BY_IDX(group_node, pinmux, idx)), \
		SUNXI_GET_MUX(DT_PROP_BY_IDX(group_node, pinmux, idx)), \
		Z_PINCTRL_SUNXI_PULL(group_node), \
		Z_PINCTRL_SUNXI_DRV(group_node) \
	)

#define SUNXI_PINCTRL_EXTRACT_EXPAND(node_id, prop, idx, ...) \
	SUNXI_PINCTRL_EXTRACT(idx, node_id),

#define Z_PINCTRL_STATE_GROUP_INIT(group_node) \
	DT_FOREACH_PROP_ELEM_VARGS(group_node, pinmux, SUNXI_PINCTRL_EXTRACT_EXPAND, 0)

#define Z_PINCTRL_STATE_GROUP_INIT_EXPAND(group_node) \
	Z_PINCTRL_STATE_GROUP_INIT(group_node)

#define Z_PINCTRL_STATE_GROUP_INIT_WRAP(node_id, prop, idx) \
	Z_PINCTRL_STATE_GROUP_INIT_EXPAND(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_GROUP_INIT_WRAP) }

#endif /* ZEPHYR_SOC_ALLWINNER_SUN50I_H618_PINCTRL_SOC_H_ */
