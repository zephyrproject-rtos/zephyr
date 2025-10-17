/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB_COMMON_PINCTRL_H_
#define _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB_COMMON_PINCTRL_H_

#define SF32LB_FSEL_POS        0U
#define SF32LB_FSEL_MSK        0x0000000FU
#define SF32LB_PORT_POS        12U
#define SF32LB_PORT_MSK        0x00003000U
#define SF32LB_PAD_POS         14U
#define SF32LB_PAD_MSK         0x003FC000U
#define SF32LB_PINR_FIELD_POS  22U
#define SF32LB_PINR_FIELD_MSK  0x00C00000U
#define SF32LB_PINR_OFFSET_POS 24U
#define SF32LB_PINR_OFFSET_MSK 0xFF000000U

/**
 * @brief Pin configuration bit field.
 *
 * Bitmap:
 * - 0-10:  Configuration bits (function select, pull, drive strength, etc.)
 * - 11:    Reserved
 * - 12-13: Port (SA, PA, ...)
 * - 14-21: Pad (0-128)
 * - 22-23: PINR register field (0-3)
 * - 24:31: PINR register offset (1-255, 0=not used)
 *
 * @param port Port
 * @param pad Pad
 * @param fsel Function select
 * @param pinr_offset PINR register offset
 * @param pinr_field PINR register field
 */
#define SF32LB_PINMUX(port, pad, fsel, pinr_offset, pinr_field)                                    \
	((((pinr_offset) << SF32LB_PINR_OFFSET_POS) & SF32LB_PINR_OFFSET_MSK) |                    \
	 (((pinr_field) << SF32LB_PINR_FIELD_POS) & SF32LB_PINR_FIELD_MSK) |                       \
	 (((SF32LB_PORT_##port) << SF32LB_PORT_POS) & SF32LB_PORT_MSK) |                           \
	 (((pad) << SF32LB_PAD_POS) & SF32LB_PAD_MSK) |                                            \
	 (((fsel) << SF32LB_FSEL_POS) & SF32LB_FSEL_MSK))

#endif /* _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB_COMMON_PINCTRL_H_ */
