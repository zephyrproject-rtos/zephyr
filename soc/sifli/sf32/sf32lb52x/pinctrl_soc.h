/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_
#define _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type for SF32LB pin.
 *
 * Bitmap:
 * - 0-10: Maps 1:1 to HPSYS_PINMUX:
 *   - 0-3: Function select
 *   - 4: Enable/disable pull
 *   - 5: Pull select (0=pulldown, 1=pullup)
 *   - 6: Input enable (0=disable, 1=enable)
 *   - 7: Input select (0=normal, 1=schmitt trigger)
 *   - 8: Slew rate (0=slow, 1=fast)
 *   - 9-10: Drive strength (0=2mA, 1=4mA, 2=8mA, 3=12mA)
 * - 11: Reserved
 * - 11-31: Location, port, pad, PINR register field and offset.
 */
typedef uint32_t pinctrl_soc_pin_t;

#define SF32LB_PE_MSK  BIT(4U)
#define SF32LB_PS_MSK  BIT(5U)
#define SF32LB_IE_MSK  BIT(6U)
#define SF32LB_IS_MSK  BIT(7U)
#define SF32LB_SR_MSK  BIT(8U)
#define SF32LB_DS0_MSK GENMASK(10U, 9U)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	(DT_PROP_BY_IDX(node_id, prop, idx) |                                                      \
	 FIELD_PREP(SF32LB_PE_MSK,                                                                 \
		    DT_PROP(node_id, bias_pull_up) | DT_PROP(node_id, bias_pull_down)) |           \
	 FIELD_PREP(SF32LB_PS_MSK, DT_PROP(node_id, bias_pull_up)) |                               \
	 FIELD_PREP(SF32LB_IE_MSK, DT_PROP(node_id, input_enable)) |                               \
	 FIELD_PREP(SF32LB_IS_MSK, DT_PROP(node_id, input_schmitt_enable)) |                       \
	 FIELD_PREP(SF32LB_SR_MSK, DT_PROP(node_id, slew_rate)) |                                  \
	 FIELD_PREP(SF32LB_DS0_MSK, DT_ENUM_IDX(node_id, drive_strength))),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_ */
