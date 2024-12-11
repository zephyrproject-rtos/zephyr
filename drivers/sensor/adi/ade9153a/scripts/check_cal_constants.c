/*
 * Copyright (c) 2024 Plentify (Pty) Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>

#define CONFIG_ADE9153A_CAL_IRMS_CC         1799999318
#define CONFIG_ADE9153A_CAL_IRMS_CC_SHIFT   0
#define CONFIG_ADE9153A_CAL_VRMS_CC         1800000660
#define CONFIG_ADE9153A_CAL_VRMS_CC_SHIFT   4
#define CONFIG_ADE9153A_CAL_POWER_CC        1582031699
#define CONFIG_ADE9153A_CAL_POWER_CC_SHIFT  11
#define CONFIG_ADE9153A_CAL_ENERGY_CC       1843200246
#define CONFIG_ADE9153A_CAL_ENERGY_CC_SHIFT 0

#define DECODE_Q1_31(_var)                                                                         \
	(((double)(CONFIG_ADE9153A_CAL_##_var##_CC) / INT32_MAX) *                                 \
	 (1U << CONFIG_ADE9153A_CAL_##_var##_CC_SHIFT))

#define CAL_IRMS_CC   DECODE_Q1_31(IRMS) /* (uA/code) */
#define CAL_VRMS_CC   DECODE_Q1_31(VRMS) /* (uV/code) */
/* (uW/code) Applicable for Active, reactive and apparent power */
#define CAL_POWER_CC  DECODE_Q1_31(POWER)
/* (uWhr/xTHR_HI code)Applicable for Active, reactive and apparent energy */
#define CAL_ENERGY_CC DECODE_Q1_31(ENERGY)

/* Original value of CAL_IRMS_CC is 0.838190 */
/* Original value of CAL_VRMS_CC is 13.41105*/
/* Original value of CAL_POWER_CC is 1508.743*/
/* Original value of CAL_ENERGY_CC is 0.858307*/

int main(int argc, char *argv[])
{
	printf("CAL_IRMS_CC=%.09f\n", CAL_IRMS_CC);
	printf("CAL_VRMS_CC=%.09f\n", CAL_VRMS_CC);
	printf("CAL_POWER_CC=%.09f\n", CAL_POWER_CC);
	printf("CAL_ENERGY_CC=%.09f\n", CAL_ENERGY_CC);
	return 0;
}
