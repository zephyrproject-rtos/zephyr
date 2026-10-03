/*
 * Copyright (c) 2026 JUMO GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup display_interface
 * @brief Devicetree memory write direction and vertical scan direction
 *        identifiers for LT7680 display controller.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_LT7680_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_LT7680_H_

#include <zephyr/dt-bindings/dt-util.h>

#define LT7680_MEM_WRITE_DIRECTION_LR_TB 0x00 /* left-to-right, top-to-bottom */
#define LT7680_MEM_WRITE_DIRECTION_RL_TB 0x01 /* right-to-left, top-to-bottom */
#define LT7680_MEM_WRITE_DIRECTION_TB_LR 0x02 /* top-to-bottom, left-to-right */
#define LT7680_MEM_WRITE_DIRECTION_BT_LR 0x03 /* bottom-to-top, left-to-right */

#define LT7680_VSCAN_DIRECTION_T_TO_B 0x00 /* vertical scan top-to-bottom */
#define LT7680_VSCAN_DIRECTION_B_TO_T 0x01 /* vertical scan bottom-to-top */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DISPLAY_LT7680_H_ */
