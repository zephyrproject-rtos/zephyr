/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MUX_TI_AM13E230_XBAR_OUT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MUX_TI_AM13E230_XBAR_OUT_H_

/*
 * Packed mux-control cell for the AM13E230 output X-BAR: encodes which
 * physical OUTPUTXBARx (0-7), which GxSEL group (0/1), and which one-hot
 * bit (0-31) within that group's 32-bit source-select field a set/get_state/
 * disconnect call addresses. One packed cell lets a single output-xbar
 * device instance cover all 8 outputs.
 */

#define AM13E230_XBAR_OUT_BIT_SHIFT 0
#define AM13E230_XBAR_OUT_BIT_MASK  0x1F

#define AM13E230_XBAR_OUT_GROUP_SHIFT 5
#define AM13E230_XBAR_OUT_GROUP_MASK  0x1

#define AM13E230_XBAR_OUT_IDX_SHIFT 6
#define AM13E230_XBAR_OUT_IDX_MASK  0x7

#define AM13E230_XBAR_OUT_GROUP_G0 0
#define AM13E230_XBAR_OUT_GROUP_G1 1

#define AM13E230_XBAR_OUT_CTRL(idx, group, bit)                                                    \
	((((idx) & AM13E230_XBAR_OUT_IDX_MASK) << AM13E230_XBAR_OUT_IDX_SHIFT) |                   \
	 (((group) & AM13E230_XBAR_OUT_GROUP_MASK) << AM13E230_XBAR_OUT_GROUP_SHIFT) |             \
	 (((bit) & AM13E230_XBAR_OUT_BIT_MASK) << AM13E230_XBAR_OUT_BIT_SHIFT))

#define AM13E230_XBAR_OUT_CTRL_IDX(ctrl)                                                           \
	(((ctrl) >> AM13E230_XBAR_OUT_IDX_SHIFT) & AM13E230_XBAR_OUT_IDX_MASK)

#define AM13E230_XBAR_OUT_CTRL_GROUP(ctrl)                                                         \
	(((ctrl) >> AM13E230_XBAR_OUT_GROUP_SHIFT) & AM13E230_XBAR_OUT_GROUP_MASK)

#define AM13E230_XBAR_OUT_CTRL_BIT(ctrl)                                                           \
	(((ctrl) >> AM13E230_XBAR_OUT_BIT_SHIFT) & AM13E230_XBAR_OUT_BIT_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MUX_TI_AM13E230_XBAR_OUT_H_ */
