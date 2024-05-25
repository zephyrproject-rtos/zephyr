/*
 * Copyright (c) 2020 Linaro Limited
 * Copyright (c) 2021 ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_CYPRESS_PSOC6_H_
#define PINCTRL_CYPRESS_PSOC6_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * Functions are defined using HSIOM SEL
 */

#define HSIOM_SEL_gpio          0
#define HSIOM_SEL_gpio_dsi      1
#define HSIOM_SEL_dsi_dsi       2
#define HSIOM_SEL_dsi_gpio      3
#define HSIOM_SEL_amuxa         4
#define HSIOM_SEL_amuxb         5
#define HSIOM_SEL_amuxa_dsi     6
#define HSIOM_SEL_amuxb_dsi     7
#define HSIOM_SEL_act_0         8
#define HSIOM_SEL_act_1         9
#define HSIOM_SEL_act_2        10
#define HSIOM_SEL_act_3        11
#define HSIOM_SEL_ds_0         12
#define HSIOM_SEL_ds_1         13
#define HSIOM_SEL_ds_2         14
#define HSIOM_SEL_ds_3         15
#define HSIOM_SEL_act_4        16
#define HSIOM_SEL_act_5        17
#define HSIOM_SEL_act_6        18
#define HSIOM_SEL_act_7        19
#define HSIOM_SEL_act_8        20
#define HSIOM_SEL_act_9        21
#define HSIOM_SEL_act_10       22
#define HSIOM_SEL_act_11       23
#define HSIOM_SEL_act_12       24
#define HSIOM_SEL_act_13       25
#define HSIOM_SEL_act_14       26
#define HSIOM_SEL_act_15       27
#define HSIOM_SEL_ds_4         28
#define HSIOM_SEL_ds_5         29
#define HSIOM_SEL_ds_6         30
#define HSIOM_SEL_ds_7         31


/* Create a pincfg device tree node:
 *
 * The node name and nodelabel will be of the form:
 *
 * NODE = p<port>_<pin>_<inst>_<signal>
 *
 * NODE: NODE {
 *	cypress,pins = < &gpio_prt<port> <pin> HSIOM_SEL_<hsiom> >;
 *	flags_1;
 *	  ...
 *	flags_N;
 * }
 *
 * So for example:
 *
 * DT_CYPRESS_PIN(uart5, rx, 5, 0, act_6);
 *
 * Will become:
 *
 * p5_0_uart5_rx: p5_0_uart5_rx {
 *    cypress,pins = <&gpio_prt5 0x0 0x12 >;
 * }
 *
 * Flags are optional and should be pass one by one as arguments:
 *
 * DT_CYPRESS_PIN(uart5, rx, 5, 0, act_6, bias-pull-up, input-enable);
 *
 * Will become:
 *
 * p5_0_uart5_rx: p5_0_uart5_rx {
 *    cypress,pins = <&gpio_prt5 0x0 0x12 >;
 *    bias-pull-up;
 *    input-enable;
 * }
 *
 * For the complete list of flags see cypress,psoc6-pinctrl.yaml
 *
 */

#define DT_CYPRESS_HSIOM_FLAG(flag) flag;
#define DT_CYPRESS_HSIOM_FLAGS(...) \
	MACRO_MAP_CAT(DT_CYPRESS_HSIOM_FLAG __VA_OPT__(,) __VA_ARGS__)

#define DT_CYPRESS_HSIOM(inst, signal, port, pin, hsiom, ...) \
	p##port##_##pin##_##inst##_##signal: \
	p##port##_##pin##_##inst##_##signal { \
	cypress,pins = < &gpio_prt##port pin HSIOM_SEL_##hsiom > ; \
		DT_CYPRESS_HSIOM_FLAGS(__VA_ARGS__) \
	}

#endif /* PINCTRL_CYPRESS_PSOC6_H_ */
