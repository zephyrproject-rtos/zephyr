/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SiWG917 (SiWx91x) clock-control helper macros.
 *
 * The platform exposes three clock providers in devicetree:
 *   - cmu_aon (AON/UULP references and always-on clocks)
 *   - cmu_hp  (M4SS/HP domain clocks)
 *   - cmu_ulp (ULPSS domain clocks)
 *
 * Consumers use a single clock cell (clkid):
 *   clocks = <&cmu_hp SIWX91X_CLK_UART0>;
 *
 * Clock-route and PLL child nodes are mapped from DT node labels to
 * SIWX91X_CLK_* via SIWX91X_CLOCK_NODE_TO_ID().
 *
 * This file provides:
 *   - DT node label -> clkid mapping helpers
 *   - clkid -> clock manager device lookup table
 *   - initializers used to build per-manager route / PLL configuration tables
 *
 * Drivers using these helpers:
 *   drivers/clock_control/clock_control_silabs_siwx91x_{hp,ulp,aon}.c
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/clock/silabs/siwx91x-clock.h>

#define SIWX91X_DT_CLOCK_STARTUP_TIME_US(node_id)                                                  \
	DT_PROP_OR(node_id, silabs_startup_time_us, 0)

/*
 * DT nodelabel <-> SIWX91X_CLK_* (xtal/rc, domain refs, PLL outputs, clock-route children).
 * Add one DT_SAME_NODE line per labelled node and a matching SIWX91X_CLOCK_ROUTE_DEV()
 * in siwx91x_clk_dev_table[] below.
 */
#define SIWX91X_CLOCK_NODE_TO_ID(node)                                                             \
	(DT_SAME_NODE(DT_NODELABEL(xtal_mhz), node)               ? SIWX91X_CLK_XTAL_MHZ           \
	 : DT_SAME_NODE(DT_NODELABEL(xtal_khz), node)             ? SIWX91X_CLK_XTAL_KHZ           \
	 : DT_SAME_NODE(DT_NODELABEL(rc_mhz), node)               ? SIWX91X_CLK_RC_MHZ             \
	 : DT_SAME_NODE(DT_NODELABEL(rc_khz), node)               ? SIWX91X_CLK_RC_KHZ             \
	 : DT_SAME_NODE(DT_NODELABEL(ref_hp_mux), node)           ? SIWX91X_CLK_REF_HP             \
	 : DT_SAME_NODE(DT_NODELABEL(ref_ulp_mux), node)          ? SIWX91X_CLK_REF_ULP            \
	 : DT_SAME_NODE(DT_NODELABEL(aon_ref_hf_mux), node)       ? SIWX91X_CLK_AON_REF_HF         \
	 : DT_SAME_NODE(DT_NODELABEL(aon_ref_lf_mux), node)       ? SIWX91X_CLK_AON_REF_LF         \
	 : DT_SAME_NODE(DT_NODELABEL(ulp_ref_cpu_mux), node)      ? SIWX91X_CLK_ULP_REF_CPU        \
	 : DT_SAME_NODE(DT_NODELABEL(ref_pll_mux), node)          ? SIWX91X_CLK_HP_REF_PLL         \
	 : DT_SAME_NODE(DT_NODELABEL(pll_soc), node)              ? SIWX91X_CLK_PLL_SOC            \
	 : DT_SAME_NODE(DT_NODELABEL(pll_intf), node)             ? SIWX91X_CLK_PLL_INTF           \
	 : DT_SAME_NODE(DT_NODELABEL(pll_i2s), node)              ? SIWX91X_CLK_PLL_I2S            \
	 : DT_SAME_NODE(DT_NODELABEL(ulp_uart_mux), node)         ? SIWX91X_CLK_ULP_UART           \
	 : DT_SAME_NODE(DT_NODELABEL(ulp_ssi_mux), node)          ? SIWX91X_CLK_ULP_SSI            \
	 : DT_SAME_NODE(DT_NODELABEL(ulp_i2s_mux), node)          ? SIWX91X_CLK_ULP_I2S            \
	 : DT_SAME_NODE(DT_NODELABEL(ulp_timer_mux), node)        ? SIWX91X_CLK_ULP_TIMER          \
	 : DT_SAME_NODE(DT_NODELABEL(ulp_ref_aon_mux), node)      ? SIWX91X_CLK_ULP_REF_AON        \
	 : DT_SAME_NODE(DT_NODELABEL(ulp_adc_mux), node)          ? SIWX91X_CLK_ULP_ADC            \
	 : DT_SAME_NODE(DT_NODELABEL(uart0_mux), node)            ? SIWX91X_CLK_UART0              \
	 : DT_SAME_NODE(DT_NODELABEL(uart1_mux), node)            ? SIWX91X_CLK_UART1              \
	 : DT_SAME_NODE(DT_NODELABEL(gspi_mux), node)             ? SIWX91X_CLK_GSPI               \
	 : DT_SAME_NODE(DT_NODELABEL(i2s_mux), node)              ? SIWX91X_CLK_I2S                \
	 : DT_SAME_NODE(DT_NODELABEL(hp_ref_ulp_mux), node)       ? SIWX91X_CLK_HP_REF_ULP         \
	 : DT_SAME_NODE(DT_NODELABEL(cpu_mux), node)              ? SIWX91X_CLK_CPU                \
	 : DT_SAME_NODE(DT_NODELABEL(cpu_lp_mux), node)           ? SIWX91X_CLK_CPU_LP             \
	 : DT_SAME_NODE(DT_NODELABEL(qspi_mux), node)             ? SIWX91X_CLK_QSPI               \
	 : DT_SAME_NODE(DT_NODELABEL(qspi2_mux), node)            ? SIWX91X_CLK_QSPI2              \
	 : DT_SAME_NODE(DT_NODELABEL(ssi_mux), node)              ? SIWX91X_CLK_SSI                \
	 : DT_SAME_NODE(DT_NODELABEL(pin_out_mux), node)          ? SIWX91X_CLK_PIN_OUT            \
								  : UINT32_MAX)

/*
 * clkid -> clock_control device (compile-time table, runtime lookup).
 * Route/PLL nodes: parent of the labelled node (the clock manager in siwg917.dtsi).
 */
#define SIWX91X_CLOCK_ROUTE_DEV(label)                                                             \
	[SIWX91X_CLOCK_NODE_TO_ID(DT_NODELABEL(label))] =                                          \
		DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(label))),

#define SIWX91X_CLOCK_DEV_AON(clkid)                                                               \
	[clkid] = DEVICE_DT_GET(DT_NODELABEL(cmu_aon))

#define SIWX91X_CLOCK_DEV_HP(clkid)                                                                \
	[clkid] = DEVICE_DT_GET(DT_NODELABEL(cmu_hp))

#define SIWX91X_CLOCK_DEV_ULP(clkid)                                                               \
	[clkid] = DEVICE_DT_GET(DT_NODELABEL(cmu_ulp))

#define SIWX91X_CLOCK_ROUTE_CLOCK_DIV(node)                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(node, clock_div), (DT_PROP(node, clock_div)), (0U))

#define SIWX91X_CLOCK_ROUTE_NODE_INIT(node)                                                        \
	{                                                                                          \
		.clkid = SIWX91X_CLOCK_NODE_TO_ID(node),                                           \
		.ref_clkid = SIWX91X_CLOCK_NODE_TO_ID(DT_CLOCKS_CTLR(node)),                       \
		.clock_div = SIWX91X_CLOCK_ROUTE_CLOCK_DIV(node),                                  \
	},

#define SIWX91X_CLOCK_PLL_NODE_INIT(node)                                                          \
	{                                                                                          \
		.clkid = SIWX91X_CLOCK_NODE_TO_ID(node),                                           \
		.ref_clkid = SIWX91X_CLOCK_NODE_TO_ID(DT_CLOCKS_CTLR(node)),                       \
		.frequency = DT_PROP(node, clock_frequency),                                       \
	},

#define SIWX91X_CLOCK_MANAGER_CHILD_INIT(node)                                                     \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, silabs_siwx91x_clock_route),                           \
		    (SIWX91X_CLOCK_ROUTE_NODE_INIT(node)))

#define SIWX91X_CLOCK_MANAGER_PLL_INIT(node)                                                       \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, silabs_siwx91x_pll),                                   \
		   (SIWX91X_CLOCK_PLL_NODE_INIT(node)))

void siwx91x_clock_request_xtal_to_nwp(void);
void siwx91x_clock_release_xtal_to_nwp(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_SIWX91X_COMMON_H_ */