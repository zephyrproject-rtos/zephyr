/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _SOC_ARM_NXP_IMX_RT_POWER_RT11XX_H_
#define _SOC_ARM_NXP_IMX_RT_POWER_RT11XX_H_

/*
 * Set point configurations. These are kept in a separate file for readability.
 */

#define SP0  0
#define SP1  1
#define SP2  2
#define SP3  3
#define SP4  4
#define SP5  5
#define SP6  6
#define SP7  7
#define SP8  8
#define SP9  9
#define SP10 10
#define SP11 11
#define SP12 12
#define SP13 13
#define SP14 14
#define SP15 15


/* ================= GPC configuration ==================== */

#define SET_POINT_COUNT 16

/*
 * SOC set point mappings
 * This matrix defines what set points are allowed for a given core.
 * For example, when SP2 is requested, SP1 or SP2 are allowed set points
 */
#define CPU0_COMPATIBLE_SP_TABLE                                                              \
/*   NA,  SP1, SP2, SP3, SP0, SP4, SP5, SP6, SP7, SP8, SP9, SP10, SP11, SP12, SP13, SP14, SP15 */ \
/* SP1*/{{  1,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP2 */{  1,   1,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP3 */{  1,   1,   1,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP0 */{  1,   1,   1,   1,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP4 */{  1,   1,   1,   1,   1,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP5 */{  1,   1,   1,   1,   1,   1,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP6 */{  1,   1,   1,   1,   1,   1,   1,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP7 */{  1,   1,   1,   1,   1,   1,   1,   1,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP8 */{  1,   1,   1,   1,   1,   1,   1,   1,   1,   0,    0,    0,    0,    0,    0,    0}, \
/* SP9 */{  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    0,    0,    0,    0,    0,    0}, \
/* SP10 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    0,    0,    0,    0,    0}, \
/* SP11 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    1,    1}, \
/* SP12 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    1,    1}, \
/* SP13 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    1,    1}, \
/* SP14 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    1,    1}, \
/* SP15 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    1,    1}}

#define CPU1_COMPATIBLE_SP_TABLE                                                              \
/*   NA,  SP1, SP2, SP3, SP0, SP4, SP5, SP6, SP7, SP8, SP9, SP10, SP11, SP12, SP13, SP14, SP15 */ \
/* SP1*/{{  1,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP2 */{  1,   1,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP3 */{  1,   1,   1,   1,   0,   0,   0,   0,   0,   0,    0,    1,    0,    0,    0,    0}, \
/* SP0 */{  1,   1,   0,   1,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0}, \
/* SP4 */{  1,   1,   1,   1,   1,   0,   0,   0,   0,   0,    0,    1,    0,    0,    0,    0}, \
/* SP5 */{  1,   1,   1,   1,   1,   1,   0,   0,   0,   0,    0,    1,    0,    0,    0,    0}, \
/* SP6 */{  1,   1,   1,   1,   1,   1,   1,   0,   0,   0,    0,    1,    0,    0,    0,    0}, \
/* SP7 */{  1,   1,   1,   1,   1,   1,   1,   1,   0,   0,    0,    1,    0,    0,    0,    0}, \
/* SP8 */{  1,   1,   1,   1,   1,   1,   1,   1,   1,   0,    0,    1,    0,    0,    0,    0}, \
/* SP9 */{  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    0,    1,    0,    0,    0,    0}, \
/* SP10 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    0,    0,    0}, \
/* SP11 */{ 1,   1,   0,   1,   0,   0,   0,   0,   0,   0,    0,    1,    0,    0,    0,    0}, \
/* SP12 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    0,    1,    1,    0,    0,    0}, \
/* SP13 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    0,    0}, \
/* SP14 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    1,    0}, \
/* SP15 */{ 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,    1,    1,    1,    1,    1}}

/* Allows GPC to control RC16M on/off */
#define OSC_RC_16M_STBY_VAL      0x0000


/* ================== DCDC configuration ======================= */
#define PD_WKUP_SP_VAL  0xf800 /* Off at SP11 - SP 15 */

#define DCDC_ONOFF_SP_VAL     (~PD_WKUP_SP_VAL)
#define DCDC_DIG_ONOFF_SP_VAL DCDC_ONOFF_SP_VAL
#define DCDC_LP_MODE_SP_VAL   0x0000
#define DCDC_ONOFF_STBY_VAL   DCDC_ONOFF_SP_VAL
#define DCDC_LP_MODE_STBY_VAL 0x0000


/* DCDC 1.8V buck mode target voltage in set points 0-15 */
#define DCDC_1P8_BUCK_MODE_CONFIGURATION_TABLE	\
{												\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
	kDCDC_1P8BuckTarget1P8V,					\
}

/* DCDC 1.0V buck mode target voltage in set points 0-15 */
#define DCDC_1P0_BUCK_MODE_CONFIGURATION_TABLE \
{                                              \
	kDCDC_1P0BuckTarget1P0V,                   \
	kDCDC_1P0BuckTarget1P1V,                   \
	kDCDC_1P0BuckTarget1P1V,                   \
	kDCDC_1P0BuckTarget1P1V,                   \
	kDCDC_1P0BuckTarget1P0V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P8V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
	kDCDC_1P0BuckTarget0P9V,                   \
}


/* DCDC 1.8V standby mode target voltage in set points 0-15 */
#define DCDC_1P8_STANDBY_MODE_CONFIGURATION_TABLE \
{												\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
	kDCDC_1P8StbyTarget1P8V,					\
}

/* DCDC 1.0V standby mode target voltage in set points 0-15 */
#define DCDC_1P0_STANDBY_MODE_CONFIGURATION_TABLE \
{                                                 \
	kDCDC_1P0StbyTarget1P0V,                      \
	kDCDC_1P0StbyTarget1P1V,                      \
	kDCDC_1P0StbyTarget1P1V,                      \
	kDCDC_1P0StbyTarget1P1V,                      \
	kDCDC_1P0StbyTarget1P0V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P8V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
	kDCDC_1P0StbyTarget0P9V,                      \
}

#endif /* _SOC_ARM_NXP_IMX_RT_POWER_RT11XX_H_ */
