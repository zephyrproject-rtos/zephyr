/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Values used to define the sink overvoltage and source overcurrent protections thresholds.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_USBC_NXP_NX20P3483_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_USBC_NXP_NX20P3483_H_

/** Voltage limit of 6.0V */
#define NX20P3483_U_THRESHOLD_6_0 0
/** Voltage limit of 6.8V */
#define NX20P3483_U_THRESHOLD_6_8 1 /* <-- default */
/** Voltage limit of 10.0V */
#define NX20P3483_U_THRESHOLD_10_0 2
/** Voltage limit of 11.5V */
#define NX20P3483_U_THRESHOLD_11_5 3
/** Voltage limit of 14.0V */
#define NX20P3483_U_THRESHOLD_14_0 4
/** Voltage limit of 17.0V */
#define NX20P3483_U_THRESHOLD_17_0 5
/** Voltage limit of 23.0V */
#define NX20P3483_U_THRESHOLD_23_0 6

/** Current limit of 400mA */
#define NX20P3483_I_THRESHOLD_0_400 0
/** Current limit of 600mA */
#define NX20P3483_I_THRESHOLD_0_600 1
/** Current limit of 800mA */
#define NX20P3483_I_THRESHOLD_0_800 2
/** Current limit of 1000mA */
#define NX20P3483_I_THRESHOLD_1_000 3
/** Current limit of 1200mA */
#define NX20P3483_I_THRESHOLD_1_200 4
/** Current limit of 1400mA */
#define NX20P3483_I_THRESHOLD_1_400 5
/** Current limit of 1600mA */
#define NX20P3483_I_THRESHOLD_1_600 6 /* <-- default */
/** Current limit of 1800mA */
#define NX20P3483_I_THRESHOLD_1_800 7
/** Current limit of 2000mA */
#define NX20P3483_I_THRESHOLD_2_000 8
/** Current limit of 2200mA */
#define NX20P3483_I_THRESHOLD_2_200 9
/** Current limit of 2400mA */
#define NX20P3483_I_THRESHOLD_2_400 10
/** Current limit of 2600mA */
#define NX20P3483_I_THRESHOLD_2_600 11
/** Current limit of 2800mA */
#define NX20P3483_I_THRESHOLD_2_800 12
/** Current limit of 3000mA */
#define NX20P3483_I_THRESHOLD_3_000 13
/** Current limit of 3200mA */
#define NX20P3483_I_THRESHOLD_3_200 14
/** Current limit of 3400mA */
#define NX20P3483_I_THRESHOLD_3_400 15

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_USBC_NXP_NX20P3483_H_ */
