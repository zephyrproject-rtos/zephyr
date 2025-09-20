/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DALI_TIMING_H_
#define ZEPHYR_DRIVERS_DALI_TIMING_H_

/* timing definitions given by the DALI standard - for use by DALI driver implementations */

/* IEC 62396-101:2022 4.10 Table 4 Power interruptions */
#define DALI_FAILURE_CONDITION_US (550000U) /* 550 ms */

/* IEC 62386-101:2022 7.4 Frame types */
#define DALI_MAX_BIT_PER_FRAME     (32U)
#define DALI_FRAME_BACKWARD_LENGTH (8U)
#define DALI_FRAME_GEAR_LENGTH     (16U)
#define DALI_FRAME_DEVICE_LENGTH   (24U)
#define DALI_FRAME_UPDATE_LENGTH   (32U)

/* IEC 62386-101:2022 8.2.1 Receiver bit timing - Table 18 */
#define DALI_RX_BIT_TIME_HALF_MIN_US (333U)
#define DALI_RX_BIT_TIME_HALF_MAX_US (500U)
#define DALI_RX_BIT_TIME_FULL_MIN_US (667U)
#define DALI_RX_BIT_TIME_FULL_MAX_US (1000U)
#define DALI_RX_BIT_TIME_STOP_US     (2400U)

/* IEC 62386-101:2022 8.3.1 Multi-master transmitter bit timing */
#define DALI_TX_HALF_BIT_MIN_US (400U)
#define DALI_TX_HALF_BIT_US     (417U)
#define DALI_TX_HALF_BIT_MAX_US (433U)
#define DALI_TX_FULL_BIT_MIN_US (800U)
#define DALI_TX_FULL_BIT_US     (833U)
#define DALI_TX_FULL_BIT_MAX_US (867U)
#define DALI_TX_STOP_BIT_US     (2450U)

/* IEC 62386-101:2022 9.2.3 Collision detroy areas */
#define DALI_TX_DESTROY_1_MIN_US (100U)
#define DALI_TX_DESTROY_1_MAX_US (357U)
#define DALI_TX_DESTROY_2_MIN_US (477U)
#define DALI_TX_DESTROY_2_MAX_US (723U)
#define DALI_TX_DESTROY_3_MIN_US (943U)

/* IEC 62386-101:2022 9.2.4 Collision recovery timing */
#define DALI_TX_BREAK_MIN_US   (1200U)
#define DALI_TX_BREAK_MAX_US   (1400U)

/* IEC 62386-101:2022 9.6.2 Shared interface - backward frame */
#define DALI_TX_CORRUPT_BIT_MIN_US (1300U)
#define DALI_TX_CORRUPT_BIT_MAX_US (2000U)

#endif /* ZEPHYR_DRIVERS_DALI_TIMING_H_ */
