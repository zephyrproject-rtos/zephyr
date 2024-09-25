/*
 * Copyright (c) 2022-2024 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_kscan

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/sys_clock.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#ifdef CONFIG_PM
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kscan_atm, CONFIG_KSCAN_LOG_LEVEL);

#include "arch.h"
#include "at_wrpr.h"
#include "at_apb_pseq_regs_core_macro.h"
#include "at_apb_ksm_regs_core_macro.h"
#include "at_pinmux.h"

#ifdef CMSDK_KSM_NONSECURE
#include "reset.h"
#else
#include "intisr.h"
#endif

// Debugs - to enable, change undef to define
#define DEBUG_KEYBOARD
#undef DEBUG_KEYBOARD_INT

// Counting maximum KSI and KSO and row and column.
#define MAX_KSI 0
#define MAX_KSO 0
#define MAX_ROW 0
#define MAX_COL 0

#if (MAX_ROW == 0) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row0_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row0_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row0_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row0_ksi))
#undef MAX_ROW
#define MAX_ROW 1
#endif // (MAX_ROW == 0) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row0_pin))

#if (MAX_ROW == 1) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row1_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row1_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row1_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row1_ksi))
#undef MAX_ROW
#define MAX_ROW 2
#endif // (MAX_ROW == 1) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row1_pin))

#if (MAX_ROW == 2) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row2_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row2_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row2_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row2_ksi))
#undef MAX_ROW
#define MAX_ROW 3
#endif // (MAX_ROW == 2) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row2_pin))

#if (MAX_ROW == 3) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row3_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row3_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row3_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row3_ksi))
#undef MAX_ROW
#define MAX_ROW 4
#endif // (MAX_ROW == 3) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row3_pin))

#if (MAX_ROW == 4) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row4_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row4_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row4_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row4_ksi))
#undef MAX_ROW
#define MAX_ROW 5
#endif // (MAX_ROW == 4) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row4_pin))

#if (MAX_ROW == 5) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row5_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row5_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row5_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row5_ksi))
#undef MAX_ROW
#define MAX_ROW 6
#endif // (MAX_ROW == 5) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row5_pin))

#if (MAX_ROW == 6) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row6_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row6_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row6_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row6_ksi))
#undef MAX_ROW
#define MAX_ROW 7
#endif // (MAX_ROW == 6) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row6_pin))

#if (MAX_ROW == 7) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row7_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row7_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row7_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row7_ksi))
#undef MAX_ROW
#define MAX_ROW 8
#endif // (MAX_ROW == 7) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row7_pin))

#if (MAX_ROW == 8) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row8_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row8_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row8_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row8_ksi))
#undef MAX_ROW
#define MAX_ROW 9
#endif // (MAX_ROW == 8) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row8_pin))

#if (MAX_ROW == 9) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row9_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row9_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row9_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row9_ksi))
#undef MAX_ROW
#define MAX_ROW 10
#endif // (MAX_ROW == 9) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row9_pin))

#if (MAX_ROW == 10) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row10_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row10_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row10_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row10_ksi))
#undef MAX_ROW
#define MAX_ROW 11
#endif // (MAX_ROW == 10) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row10_pin))

#if (MAX_ROW == 11) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row11_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row11_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row11_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row11_ksi))
#undef MAX_ROW
#define MAX_ROW 12
#endif // (MAX_ROW == 11) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row11_pin))

#if (MAX_ROW == 12) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row12_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row12_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row12_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row12_ksi))
#undef MAX_ROW
#define MAX_ROW 13
#endif // (MAX_ROW == 12) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row12_pin))

#if (MAX_ROW == 13) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row13_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row13_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row13_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row13_ksi))
#undef MAX_ROW
#define MAX_ROW 14
#endif // (MAX_ROW == 13) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row13_pin))

#if (MAX_ROW == 14) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row14_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row14_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row14_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row14_ksi))
#undef MAX_ROW
#define MAX_ROW 15
#endif // (MAX_ROW == 14) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row14_pin))

#if (MAX_ROW == 15) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row15_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row15_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row15_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row15_ksi))
#undef MAX_ROW
#define MAX_ROW 16
#endif // (MAX_ROW == 15) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row15_pin))

#if (MAX_ROW == 16) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row16_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row16_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row16_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row16_ksi))
#undef MAX_ROW
#define MAX_ROW 17
#endif // (MAX_ROW == 16) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row16_pin))

#if (MAX_ROW == 17) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row17_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row17_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row17_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row17_ksi))
#undef MAX_ROW
#define MAX_ROW 18
#endif // (MAX_ROW == 17) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row17_pin))

#if (MAX_ROW == 18) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row18_pin))
#if (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row18_ksi))
#undef MAX_KSI
#define MAX_KSI DT_PROP(DT_NODELABEL(kscan), row18_ksi)
#endif // (MAX_KSI < DT_PROP(DT_NODELABEL(kscan), row18_ksi))
#undef MAX_ROW
#define MAX_ROW 19
#endif // (MAX_ROW == 18) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row18_pin))

#if (MAX_COL == 0) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col0_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col0_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col0_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col0_kso))
#undef MAX_COL
#define MAX_COL 1
#endif // (MAX_COL == 0) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col0_pin))

#if (MAX_COL == 1) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col1_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col1_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col1_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col1_kso))
#undef MAX_COL
#define MAX_COL 2
#endif // (MAX_COL == 1) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col1_pin))

#if (MAX_COL == 2) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col2_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col2_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col2_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col2_kso))
#undef MAX_COL
#define MAX_COL 3
#endif // (MAX_COL == 2) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col2_pin))

#if (MAX_COL == 3) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col3_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col3_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col3_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col3_kso))
#undef MAX_COL
#define MAX_COL 4
#endif // (MAX_COL == 3) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col3_pin))

#if (MAX_COL == 4) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col4_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col4_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col4_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col4_kso))
#undef MAX_COL
#define MAX_COL 5
#endif // (MAX_COL == 4) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col4_pin))

#if (MAX_COL == 5) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col5_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col5_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col5_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col5_kso))
#undef MAX_COL
#define MAX_COL 6
#endif // (MAX_COL == 5) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col5_pin))

#if (MAX_COL == 6) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col6_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col6_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col6_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col6_kso))
#undef MAX_COL
#define MAX_COL 7
#endif // (MAX_COL == 6) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col6_pin))

#if (MAX_COL == 7) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col7_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col7_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col7_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col7_kso))
#undef MAX_COL
#define MAX_COL 8
#endif // (MAX_COL == 7) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col7_pin))

#if (MAX_COL == 8) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col8_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col8_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col8_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col8_kso))
#undef MAX_COL
#define MAX_COL 9
#endif // (MAX_COL == 8) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col8_pin))

#if (MAX_COL == 9) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col9_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col9_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col9_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col9_kso))
#undef MAX_COL
#define MAX_COL 10
#endif // (MAX_COL == 9) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col9_pin))

#if (MAX_COL == 10) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col10_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col10_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col10_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col10_kso))
#undef MAX_COL
#define MAX_COL 11
#endif // (MAX_COL == 10) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col10_pin))

#if (MAX_COL == 11) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col11_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col11_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col11_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col11_kso))
#undef MAX_COL
#define MAX_COL 12
#endif // (MAX_COL == 11) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col11_pin))

#if (MAX_COL == 12) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col12_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col12_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col12_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col12_kso))
#undef MAX_COL
#define MAX_COL 13
#endif // (MAX_COL == 12) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col12_pin))

#if (MAX_COL == 13) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col13_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col13_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col13_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col13_kso))
#undef MAX_COL
#define MAX_COL 14
#endif // (MAX_COL == 13) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col13_pin))

#if (MAX_COL == 14) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col14_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col14_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col14_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col14_kso))
#undef MAX_COL
#define MAX_COL 15
#endif // (MAX_COL == 14) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col14_pin))

#if (MAX_COL == 15) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col15_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col15_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col15_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col15_kso))
#undef MAX_COL
#define MAX_COL 16
#endif // (MAX_COL == 15) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col15_pin))

#if (MAX_COL == 16) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col16_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col16_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col16_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col16_kso))
#undef MAX_COL
#define MAX_COL 17
#endif // (MAX_COL == 16) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col16_pin))

#if (MAX_COL == 17) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col17_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col17_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col17_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col17_kso))
#undef MAX_COL
#define MAX_COL 18
#endif // (MAX_COL == 17) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col17_pin))

#if (MAX_COL == 18) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col18_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col18_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col18_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col18_kso))
#undef MAX_COL
#define MAX_COL 19
#endif // (MAX_COL == 18) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col18_pin))

#if (MAX_COL == 19) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col19_pin))
#if (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col19_kso))
#undef MAX_KSO
#define MAX_KSO DT_PROP(DT_NODELABEL(kscan), col19_kso)
#endif // (MAX_KSO < DT_PROP(DT_NODELABEL(kscan), col19_kso))
#undef MAX_COL
#define MAX_COL 20
#endif // (MAX_COL == 19) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col19_pin))

static uint8_t row[MAX_KSI + 1];
static uint8_t col[MAX_KSO + 1];

#ifndef CMSDK_KSM_NONSECURE
#define KSI_(x) KSI_##x##_
#define KSO_(x) KSO_##x##_
#else
#define KSI_(x) KSI##x
#define KSO_(x) KSO##x
#endif

#define KSI(x) KSI_(x)
#define PINMUX_KSI_SET(inst)                                                                       \
	do {                                                                                       \
		PIN_SELECT(DT_PROP(DT_NODELABEL(kscan), row##inst##_pin),                          \
			   KSI(DT_PROP(DT_NODELABEL(kscan), row##inst##_ksi)));                    \
	} while (0)

#define KSO(x) KSO_(x)
#define PINMUX_KSO_SET(inst)                                                                       \
	do {                                                                                       \
		PIN_SELECT(DT_PROP(DT_NODELABEL(kscan), col##inst##_pin),                          \
			   KSO(DT_PROP(DT_NODELABEL(kscan), col##inst##_kso)));                    \
	} while (0)

// Macro for setting mux
#define KSM_COL_SET(x)                                                                             \
	do {                                                                                       \
		PINMUX_KSO_SET(x);                                                                 \
		col[DT_PROP(DT_NODELABEL(kscan), col##x##_kso)] = x;                               \
	} while (0)

#define KSM_ROW_SET(x)                                                                             \
	do {                                                                                       \
		PINMUX_KSI_SET(x);                                                                 \
		row[DT_PROP(DT_NODELABEL(kscan), row##x##_ksi)] = x;                               \
	} while (0)

#define RC2IDX(__r, __c) (((__r)*MAX_COL) + (__c))

#ifndef KTP0
#define KTP0 (KSM_TIME_PARAM0__T1__WRITE(0) | KSM_TIME_PARAM0__T2__WRITE(0))
#endif

#ifndef KTP1
#define KTP1 (KSM_TIME_PARAM1__T3__WRITE(1) | KSM_TIME_PARAM1__T4__WRITE(0))
#endif

#define CTRL0_GO (KSM_CTRL0__CONSECSCAN__WRITE(1) | KSM_CTRL0__GO__MASK)

static K_SEM_DEFINE(kscan_sem, 0, 1);
struct k_thread kscan_thread_data;
static K_KERNEL_STACK_DEFINE(kscan_thread_stack, CONFIG_KSCAN_THREAD_STACK_SIZE);

/* Device data */
struct kscan_atm_data {
	kscan_callback_t callback;
};

static struct kscan_atm_data kbd_data;

#define DRV_DATA(dev) ((struct kscan_atm_data *)(dev)->data)

__ramfunc static void kscan_isr(void *arg)
{
	ARG_UNUSED(arg);

	// Disable interrupts.  Event handler will turn them back on.
	CMSDK_KSM->INTERRUPT_MASK = 0;
	k_sem_give(&kscan_sem);
}

static bool keyboard_check_overflow(const struct device *dev, uint32_t KSM_INTERRUPTS)
{
	if (!(KSM_INTERRUPTS & KSM_INTERRUPTS__INTRPT1__MASK)) {
		return (false);
	}

	LOG_DBG("KSM OVERFLOW!");
	CMSDK_KSM->INTERRUPT_CLEAR = KSM_INTERRUPT_CLEAR__WRITE;
	CMSDK_KSM->INTERRUPT_CLEAR = 0;

#ifdef CONFIG_PM
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
	CMSDK_KSM->CTRL0 = CTRL0_GO | KSM_CTRL0__FLUSH__MASK;
	CMSDK_KSM->INTERRUPT_MASK = KSM_INTERRUPT_MASK__MASK_INTRPT3__MASK;

	if (DRV_DATA(dev)->callback) {
		DRV_DATA(dev)->callback(dev, 0xFF, 0xFF, 0);
	}

	return (true);
}

static void keyboard_pkt_handler(const struct device *dev, uint32_t pkt)
{
	bool pressed = KSM_KEYBOARD_PACKET__PRESSED_RELEASED_N__READ(pkt) != 0;
	unsigned int ksi = KSM_KEYBOARD_PACKET__ROW__READ(pkt);
	unsigned int kso = KSM_KEYBOARD_PACKET__COL__READ(pkt);

	LOG_DBG("KSIO: (%u, %u) => RC: (%u, %u)", ksi, kso, row[ksi], col[kso]);

	if (DRV_DATA(dev)->callback) {
		DRV_DATA(dev)->callback(dev, row[ksi], col[kso], pressed);
	}
}

static void keyboard_event(const struct device *dev)
{
	// Record asserted interrupts
	uint32_t KSM_INTERRUPTS = CMSDK_KSM->INTERRUPTS;

#ifdef DEBUG_KEYBOARD_INT
	uint32_t KSM_CTRL0 = CMSDK_KSM->CTRL0;
	LOG_DBG("KSM event CTRL0=%#" PRIx32 " INTS=%#" PRIx32, KSM_CTRL0, KSM_INTERRUPTS);
#endif

	if (KSM_INTERRUPTS & KSM_INTERRUPTS__INTRPT3__MASK) {
		// Flush completed
		CMSDK_KSM->CTRL0 = CTRL0_GO;

		CMSDK_KSM->INTERRUPT_CLEAR = KSM_INTERRUPT_CLEAR__CLEAR_INTRPT3__MASK;
		CMSDK_KSM->INTERRUPT_CLEAR = 0;
#ifdef DEBUG_KEYBOARD_INT
		LOG_DBG("KSM FIFO flushed");
#endif
#ifdef CONFIG_PM
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
		CMSDK_KSM->INTERRUPT_MASK = KSM_INTERRUPT_MASK__MASK_INTRPT1__MASK |
					    KSM_INTERRUPT_MASK__MASK_INTRPT0__MASK;
		return;
	}

	if (keyboard_check_overflow(dev, KSM_INTERRUPTS)) {
		return;
	}

	if (KSM_INTERRUPTS & KSM_INTERRUPTS__INTRPT2__MASK) {
		// Pop now ready
		CMSDK_KSM->CTRL0 = CTRL0_GO;

		CMSDK_KSM->INTERRUPT_CLEAR = KSM_INTERRUPT_CLEAR__CLEAR_INTRPT2__MASK;
		CMSDK_KSM->INTERRUPT_CLEAR = 0;

		uint32_t pkt = CMSDK_KSM->KEYBOARD_PACKET;
		if (pkt & KSM_KEYBOARD_PACKET__EMPTY__MASK) {
#ifdef DEBUG_KEYBOARD_INT
			LOG_DBG("KSM FIFO empty");
#endif
#ifdef CONFIG_PM
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
			CMSDK_KSM->INTERRUPT_MASK = KSM_INTERRUPT_MASK__MASK_INTRPT1__MASK |
						    KSM_INTERRUPT_MASK__MASK_INTRPT0__MASK;
			return;
		}

		keyboard_pkt_handler(dev, pkt);

		CMSDK_KSM->INTERRUPT_CLEAR = KSM_INTERRUPT_CLEAR__CLEAR_INTRPT0__MASK;
		CMSDK_KSM->INTERRUPT_CLEAR = 0;

		// Pop the next event.  INTRPT2 will fire when ready.
		CMSDK_KSM->CTRL0 = KSM_CTRL0__POP__MASK | CTRL0_GO;
		CMSDK_KSM->INTERRUPT_MASK = KSM_INTERRUPT_MASK__MASK_INTRPT2__MASK;
		return;
	}

	CMSDK_KSM->INTERRUPT_CLEAR = KSM_INTERRUPT_CLEAR__CLEAR_INTRPT0__MASK;
	CMSDK_KSM->INTERRUPT_CLEAR = 0;

	// Pop the first event.  INTRPT2 will fire when ready.
#ifdef CONFIG_PM
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif
	CMSDK_KSM->CTRL0 = KSM_CTRL0__POP__MASK | CTRL0_GO;
	CMSDK_KSM->INTERRUPT_MASK = KSM_INTERRUPT_MASK__MASK_INTRPT2__MASK;
}

__ramfunc static void keyboard_pseq_latch_close(void)
{
#ifdef CMSDK_KSM_NONSECURE
	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE)
	{
		PSEQ_CTRL0__KSM_LATCH_OPEN__CLR(CMSDK_PSEQ->CTRL0);
	}
	WRPR_CTRL_POP();
#endif
}

#ifdef CONFIG_PM
static void notify_pm_state_exit(enum pm_state state)
{
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		keyboard_pseq_latch_close();
	}
}

static struct pm_notifier notifier = {
	.state_entry = NULL,
	.state_exit = notify_pm_state_exit,
};
#endif

__ramfunc static void kscan_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

#ifdef CONFIG_PM
	pm_notifier_register(&notifier);
#endif

	for (;;) {
		k_sem_take(&kscan_sem, K_FOREVER);
		keyboard_event(p1);
	}
}

static int kscan_atm_init(const struct device *dev)
{
#if (MAX_ROW > 0) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row0_pin))
	KSM_ROW_SET(0);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row0_pin));
#endif

#if (MAX_ROW > 1) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row1_pin))
	KSM_ROW_SET(1);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row1_pin));
#endif

#if (MAX_ROW > 2) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row2_pin))
	KSM_ROW_SET(2);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row2_pin));
#endif

#if (MAX_ROW > 3) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row3_pin))
	KSM_ROW_SET(3);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row3_pin));
#endif

#if (MAX_ROW > 4) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row4_pin))
	KSM_ROW_SET(4);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row4_pin));
#endif

#if (MAX_ROW > 5) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row5_pin))
	KSM_ROW_SET(5);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row5_pin));
#endif

#if (MAX_ROW > 6) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row6_pin))
	KSM_ROW_SET(6);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row6_pin));
#endif

#if (MAX_ROW > 7) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row7_pin))
	KSM_ROW_SET(7);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row7_pin));
#endif

#if (MAX_ROW > 8) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row8_pin))
	KSM_ROW_SET(8);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row8_pin));
#endif

#if (MAX_ROW > 9) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row9_pin))
	KSM_ROW_SET(9);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row9_pin));
#endif

#if (MAX_ROW > 10) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row10_pin))
	KSM_ROW_SET(10);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row10_pin));
#endif

#if (MAX_ROW > 11) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row11_pin))
	KSM_ROW_SET(11);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row11_pin));
#endif

#if (MAX_ROW > 12) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row12_pin))
	KSM_ROW_SET(12);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row12_pin));
#endif

#if (MAX_ROW > 13) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row13_pin))
	KSM_ROW_SET(13);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row13_pin));
#endif

#if (MAX_ROW > 14) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row14_pin))
	KSM_ROW_SET(14);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row14_pin));
#endif

#if (MAX_ROW > 15) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row15_pin))
	KSM_ROW_SET(15);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row15_pin));
#endif

#if (MAX_ROW > 16) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row16_pin))
	KSM_ROW_SET(16);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row16_pin));
#endif

#if (MAX_ROW > 17) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row17_pin))
	KSM_ROW_SET(17);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row17_pin));
#endif

#if (MAX_ROW > 18) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), row18_pin))
	KSM_ROW_SET(18);
	PIN_PULLUP(DT_PROP(DT_NODELABEL(kscan), row18_pin));
#endif

#if (MAX_COL > 0) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col0_pin))
	KSM_COL_SET(0);
#endif

#if (MAX_COL > 1) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col1_pin))
	KSM_COL_SET(1);
#endif

#if (MAX_COL > 2) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col2_pin))
	KSM_COL_SET(2);
#endif

#if (MAX_COL > 3) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col3_pin))
	KSM_COL_SET(3);
#endif

#if (MAX_COL > 4) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col4_pin))
	KSM_COL_SET(4);
#endif

#if (MAX_COL > 5) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col5_pin))
	KSM_COL_SET(5);
#endif

#if (MAX_COL > 6) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col6_pin))
	KSM_COL_SET(6);
#endif

#if (MAX_COL > 7) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col7_pin))
	KSM_COL_SET(7);
#endif

#if (MAX_COL > 8) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col8_pin))
	KSM_COL_SET(8);
#endif

#if (MAX_COL > 9) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col9_pin))
	KSM_COL_SET(9);
#if (DT_PROP(DT_NODELABEL(kscan), col9_pin) == 1)
	PIN_PULL_CLR(DT_PROP(DT_NODELABEL(kscan), col9_pin));
#endif
#endif

#if (MAX_COL > 10) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col10_pin))
	KSM_COL_SET(10);
#endif

#if (MAX_COL > 11) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col11_pin))
	KSM_COL_SET(11);
#endif

#if (MAX_COL > 12) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col12_pin))
	KSM_COL_SET(12);
#endif

#if (MAX_COL > 13) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col13_pin))
	KSM_COL_SET(13);
#endif

#if (MAX_COL > 14) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col14_pin))
	KSM_COL_SET(14);
#endif

#if (MAX_COL > 15) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col15_pin))
	KSM_COL_SET(15);
#endif

#if (MAX_COL > 16) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col16_pin))
	KSM_COL_SET(16);
#endif

#if (MAX_COL > 17) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col17_pin))
	KSM_COL_SET(17);
#endif

#if (MAX_COL > 18) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col18_pin))
	KSM_COL_SET(18);
#endif

#if (MAX_COL > 19) && (DT_NODE_HAS_PROP(DT_NODELABEL(kscan), col19_pin))
	KSM_COL_SET(19);
#endif

	LOG_DBG("Keyboard HW: %d ROWS, %d COLS", MAX_KSI, MAX_KSO);
	LOG_DBG("Keyboard SW: %d ROWS, %d COLS", MAX_ROW, MAX_COL);

	k_thread_create(&kscan_thread_data, kscan_thread_stack,
			K_KERNEL_STACK_SIZEOF(kscan_thread_stack), kscan_thread, (void *)dev, NULL,
			NULL, K_PRIO_COOP(4), 0, K_NO_WAIT);
#ifdef CMSDK_KSM_NONSECURE
	if (is_boot_type(TYPE_POWER_ON) || is_boot_type(TYPE_SOCOFF)) {
#else
	{
#endif
		WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE)
		{
			CMSDK_PSEQ->KSMQDEC_CONTROL = PSEQ_KSMQDEC_CONTROL__KSMQDEC_ISO__MASK |
						      PSEQ_KSMQDEC_CONTROL__KSMQDEC_FRST__MASK;
			CMSDK_PSEQ->KSMQDEC_CONTROL = PSEQ_KSMQDEC_CONTROL__KSMQDEC_ISO__MASK;
			CMSDK_PSEQ->KSMQDEC_CONTROL = PSEQ_KSMQDEC_CONTROL__KSMQDEC_CLKEN__MASK;
		}
		WRPR_CTRL_POP();
	}

	WRPR_CTRL_SET(CMSDK_KSM, WRPR_CTRL__CLK_ENABLE);

	CMSDK_KSM->TIME_PARAM0 = KTP0;
	CMSDK_KSM->TIME_PARAM1 = KTP1;

	// Configure rows and cols
	CMSDK_KSM->MATRIX_SIZE =
		KSM_MATRIX_SIZE__NUM_ROW__WRITE((MAX_KSI > KSM_MATRIX_SIZE__NUM_ROW__RESET_VALUE) ?
							      MAX_KSI :
							      KSM_MATRIX_SIZE__NUM_ROW__RESET_VALUE) |
		KSM_MATRIX_SIZE__NUM_COL__WRITE((MAX_KSO > KSM_MATRIX_SIZE__NUM_COL__RESET_VALUE) ?
							      MAX_KSO :
							      KSM_MATRIX_SIZE__NUM_COL__RESET_VALUE);

#ifndef CMSDK_KSM_NONSECURE
	CMSDK_WRPR->INTRPT_CFG_14 = INTISR_SRC_KSM;
#endif

	CMSDK_KSM->INTERRUPT_MASK =
		KSM_INTERRUPT_MASK__MASK_INTRPT1__MASK | KSM_INTERRUPT_MASK__MASK_INTRPT0__MASK;

	CMSDK_KSM->CTRL0 = CTRL0_GO;
	keyboard_pseq_latch_close();

	return 0;
}

static int kscan_atm_configure(const struct device *dev, kscan_callback_t callback)
{
	struct kscan_atm_data *data = DRV_DATA(dev);

	data->callback = callback;

	return 0;
}

static int kscan_atm_disable_callback(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int kscan_atm_enable_callback(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), kscan_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static const struct kscan_driver_api kscan_atm_driver_api = {
	.config = kscan_atm_configure,
	.disable_callback = kscan_atm_disable_callback,
	.enable_callback = kscan_atm_enable_callback,
};

DEVICE_DT_INST_DEFINE(0, kscan_atm_init, NULL, &kbd_data, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &kscan_atm_driver_api);
