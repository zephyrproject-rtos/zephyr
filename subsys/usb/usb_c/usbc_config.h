/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_CONFIG_H_
#define ZEPHYR_SUBSYS_USBC_CONFIG_H_

#include <zephyr/drivers/usb_c/usbc_tc.h>
#include <zephyr/sys/util_macro.h>

#ifdef CONFIG_USBC_CSM_DRP
/*
 * DRP Toggle Timing Configuration
 *
 * Calculate DRP Source and Sink times from the configured period and duty cycle.
 * These values determine how long the DRP device advertises as Source vs Sink
 * during the unattached DRP toggle sequence.
 */

/* Time to advertise as Source during DRP toggle (milliseconds) */
#define TC_T_DRP_SRC_MS ((CONFIG_USBC_DRP_PERIOD_MS * CONFIG_USBC_DRP_DUTY_CYCLE) / 100)

/* Time to advertise as Sink during DRP toggle (milliseconds) */
#define TC_T_DRP_SNK_MS (CONFIG_USBC_DRP_PERIOD_MS - TC_T_DRP_SRC_MS)

/*
 * Ensure calculated values are within spec (dcSRC.DRP min * tDRP min - dcSRC.DRP max * tDRP max
 * per USB Type-C spec)
 */
BUILD_ASSERT(TC_T_DRP_SRC_MS >= TC_T_DRP_PW_MIN_MS && TC_T_DRP_SRC_MS <= TC_T_DRP_PW_MAX_MS,
	     "DRP Source time out of spec (dcSRC.DRP min * tDRP min … dcSRC.DRP max * tDRP max)");
BUILD_ASSERT(TC_T_DRP_SNK_MS >= TC_T_DRP_PW_MIN_MS && TC_T_DRP_SNK_MS <= TC_T_DRP_PW_MAX_MS,
	     "DRP Sink time out of spec (dcSRC.DRP min * tDRP min … dcSRC.DRP max * tDRP max)");
#endif /* CONFIG_USBC_CSM_DRP */

#endif /* ZEPHYR_SUBSYS_USBC_CONFIG_H_ */
