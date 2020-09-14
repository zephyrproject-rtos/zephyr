/*
 * Copyright (c) 2020 Linaro Limited
 * Copyright (c) 2020 ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_CYPRESS_PSOC6_H_
#define PINCTRL_CYPRESS_PSOC6_H_

#define PULLUP_POS             (0)
#define pullup                 (1 << PULLUP_POS)
#define PULLDOWN_POS           (1)
#define pulldown               (1 << PULLDOWN_POS)
#define OPENDRAIN_POS          (2)
#define opendrain              (1 << OPENDRAIN_POS)
#define STRONG_POS             (3)
#define strong                 (1 << STRONG_POS)
#define INPUTBUFFER_POS        (4)
#define inputbuffer            (1 << INPUTBUFFER_POS)

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
 *     cypress,pins = < &p<port> <port> <pin> HSIOM_SEL_<hsiom> <flags> >;
 * }
 *
 * So for example:
 *
 * DT_CYPRESS_PIN(uart0, rx, 0, 8, act_4, flags [default 0]);
 *
 * Will become:
 *
 * p0_8_uart0_rx: p0_8_uart0_rx {
 *    cypress,pins = <&p0 0 8 HSIOM_SEL_act4 0>;
 * }
 *
 * Flags should be always added for peripherals. It is because the default
 * behaviour from Cy port is analog. This will avoid user add the default
 * configuration for that pin defined on a pinctrl-0 node.
 */

#define DT_CYPRESS_HSIOM(inst, signal, port, pin, hsiom, flags) \
	p##port##_##pin##_##inst##_##signal: \
	p##port##_##pin##_##inst##_##signal { \
	cypress,pins = < &p##port port pin HSIOM_SEL_##hsiom flags > ; }

#endif /* PINCTRL_CYPRESS_PSOC6_H_ */
