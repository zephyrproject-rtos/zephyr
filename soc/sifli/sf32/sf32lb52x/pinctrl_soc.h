/*
 * Copyright (c) 2025 Core Devices LLC
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_
#define _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/sf32lb-common-pinctrl.h>
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
 *   - 8: Slew rate on most pads (0=fast, 1=slow); PA39-PA42 repurpose this as
 *        MODE (0=GPIO, 1=I2C)
 *   - 9-10: Drive strength {DS0, DS1} (0-3), DS0 is high bit, default=2 (4mA).
 *        In the devicetree token, bits 11:9 carry the drive strength enum
 *        index instead (see SF32LB_DS_IDX_MSK); the driver extracts it and
 *        only then writes the hardware DS bits.
 * - 11: Drive strength enum index MSB (devicetree token only)
 * - 12-13: Port (SA=0, PA=1)
 * - 14-21: Pad number within the port
 * - 22-23: PINR register field index
 * - 24-31: PINR register offset
 */
typedef uint32_t pinctrl_soc_pin_t;

#define SF32LB_PE_MSK BIT(4U)
#define SF32LB_PS_MSK BIT(5U)
#define SF32LB_IE_MSK BIT(6U)
#define SF32LB_SR_MSK BIT(8U)
#define SF32LB_DS_MSK GENMASK(10U, 9U)

/*
 * Drive strength enum index position and mask. Stored in bits 11-9, which
 * are free in the devicetree token: the hardware DS bits (9-10) are only
 * filled in by the driver after the index has been extracted. Bits 11-13
 * must not be used: the port field (bits 13-12) shares bit 12, which would
 * corrupt the port decoding for the default 4 mA setting (index 2).
 */
#define SF32LB_DS_IDX_POS 9U
#define SF32LB_DS_IDX_MSK GENMASK(11U, 9U)

/*
 * Pin configuration mask for bits that should be modified.
 * Bit 8 is SR on most pads and MODE on PA39-PA42. The pinctrl driver masks it
 * out for PA39-PA42 and handles MODE separately.
 * IS (input-schmitt) is preserved from hardware defaults.
 * DS bits (9-10) are set by driver after mA-to-register conversion.
 */
#define SF32LB_PINMUX_CFG_MSK                                                                      \
	(SF32LB_FSEL_MSK | SF32LB_PE_MSK | SF32LB_PS_MSK | SF32LB_IE_MSK | SF32LB_SR_MSK |     \
	 SF32LB_DS_MSK)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	(DT_PROP_BY_IDX(node_id, prop, idx) |                                                      \
	 FIELD_PREP(SF32LB_PE_MSK,                                                                 \
		    (DT_PROP(node_id, bias_pull_up) | DT_PROP(node_id, bias_pull_down))) |         \
	 FIELD_PREP(SF32LB_PS_MSK, DT_PROP(node_id, bias_pull_up)) |                               \
	 FIELD_PREP(SF32LB_IE_MSK, DT_PROP(node_id, input_enable)) |                               \
	 COND_CODE_0(DT_PROP(node_id, slew_rate), (SF32LB_SR_MSK), (0U)) |                         \
	 FIELD_PREP(SF32LB_DS_IDX_MSK, DT_ENUM_IDX(node_id, drive_strength))),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_ */
