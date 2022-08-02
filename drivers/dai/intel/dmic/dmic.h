/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_H__
#define __INTEL_DAI_DRIVER_DMIC_H__

#include <zephyr/sys/util_macro.h>

/* bit operations macros */
#define MASK(b_hi, b_lo)	\
	(((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL) << (b_lo))

#define SET_BIT(b, x)		(((x) & 1) << (b))

#define SET_BITS(b_hi, b_lo, x)	\
	(((x) & ((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL)) << (b_lo))

#define GET_BIT(b, x) \
	(((x) & (1ULL << (b))) >> (b))

#define GET_BITS(b_hi, b_lo, x) \
	(((x) & MASK(b_hi, b_lo)) >> (b_lo))

/* The microphones create a low frequecy thump sound when clock is enabled.
 * The unmute linear gain ramp chacteristic is defined here.
 * NOTE: Do not set any of these to 0.
 */
#define DMIC_UNMUTE_RAMP_US	1000	/* 1 ms (in microseconds) */
#define DMIC_UNMUTE_CIC		1	/* Unmute CIC at 1 ms */
#define DMIC_UNMUTE_FIR		2	/* Unmute FIR at 2 ms */

/* DMIC timestamping registers */
#define TS_DMIC_LOCAL_TSCTRL_OFFSET	0x000
#define TS_DMIC_LOCAL_OFFS_OFFSET	0x004
#define TS_DMIC_LOCAL_SAMPLE_OFFSET	0x008
#define TS_DMIC_LOCAL_WALCLK_OFFSET	0x010
#define TS_DMIC_TSCC_OFFSET		0x018

/* Timestamping */
#define TIMESTAMP_BASE          0x00071800

#define TS_DMIC_LOCAL_TSCTRL	(TIMESTAMP_BASE + TS_DMIC_LOCAL_TSCTRL_OFFSET)
#define TS_DMIC_LOCAL_OFFS	(TIMESTAMP_BASE + TS_DMIC_LOCAL_OFFS_OFFSET)
#define TS_DMIC_LOCAL_SAMPLE	(TIMESTAMP_BASE + TS_DMIC_LOCAL_SAMPLE_OFFSET)
#define TS_DMIC_LOCAL_WALCLK	(TIMESTAMP_BASE + TS_DMIC_LOCAL_WALCLK_OFFSET)
#define TS_DMIC_TSCC		(TIMESTAMP_BASE + TS_DMIC_TSCC_OFFSET)

#define TS_LOCAL_TSCTRL_NTK_BIT		BIT(31)
#define TS_LOCAL_TSCTRL_IONTE_BIT	BIT(30)
#define TS_LOCAL_TSCTRL_SIP_BIT		BIT(8)
#define TS_LOCAL_TSCTRL_HHTSE_BIT	BIT(7)
#define TS_LOCAL_TSCTRL_ODTS_BIT	BIT(5)
#define TS_LOCAL_TSCTRL_CDMAS(x)	SET_BITS(4, 0, x)
#define TS_LOCAL_OFFS_FRM		GET_BITS(15, 12)
#define TS_LOCAL_OFFS_CLK		GET_BITS(11, 0)

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V15
/* Clock control */
#define SHIM_CLKCTL		0x78
/* DMIC Force Dynamic Clock Gating */
#define SHIM_CLKCTL_DMICFDCGB   BIT(24)
#endif

/* Digital Mic Shim Registers */
#define DMICLCTL_OFFSET        0x04
#define DMICIPPTR_OFFSET       0x08
#define DMICSYNC_OFFSET        0x0C

/* DMIC power ON bit */
#define DMICLCTL_SPA		BIT(0)
/* DMIC Owner Select */
#define DMICLCTL_OSEL(x)	SET_BITS(25, 24, x)
/* DMIC disable clock gating */
#define DMIC_DCGD		BIT(30)

/* DMIC Command Sync */
#define DMICSYNC_CMDSYNC	BIT(16)
/* DMIC Sync Go */
#define DMICSYNC_SYNCGO		BIT(24)
/* DMIC Sync Period */
#define DMICSYNC_SYNCPRD(x)	SET_BITS(14, 0, x)

/* Parameters used in modes computation */
#define DMIC_HW_BITS_CIC		26
#define DMIC_HW_BITS_FIR_COEF		20
#define DMIC_HW_BITS_FIR_GAIN		20
#define DMIC_HW_BITS_FIR_INPUT		22
#define DMIC_HW_BITS_FIR_OUTPUT		24
#define DMIC_HW_BITS_FIR_INTERNAL	26
#define DMIC_HW_BITS_GAIN_OUTPUT	22
#define DMIC_HW_FIR_LENGTH_MAX		250
#define DMIC_HW_CIC_SHIFT_MIN		-8
#define DMIC_HW_CIC_SHIFT_MAX		4
#define DMIC_HW_FIR_SHIFT_MIN		0
#define DMIC_HW_FIR_SHIFT_MAX		8
#define DMIC_HW_CIC_DECIM_MIN		5
#define DMIC_HW_CIC_DECIM_MAX		31 /* Note: Limited by BITS_CIC */
#define DMIC_HW_FIR_DECIM_MIN		2
#define DMIC_HW_FIR_DECIM_MAX		20 /* Note: Practical upper limit */
#define DMIC_HW_SENS_Q28		Q_CONVERT_FLOAT(1.0, 28) /* Q1.28 */
#define DMIC_HW_PDM_CLK_MIN		100000 /* Note: Practical min value */
#define DMIC_HW_DUTY_MIN		20 /* Note: Practical min value */
#define DMIC_HW_DUTY_MAX		80 /* Note: Practical max value */

/* DMIC register offsets */

/* Global registers */
#define OUTCONTROL0		0x0000
#define OUTSTAT0		0x0004
#define OUTDATA0		0x0008
#define OUTCONTROL1		0x0100
#define OUTSTAT1		0x0104
#define OUTDATA1		0x0108
#define PDM0			0x1000
#define PDM0_COEFFICIENT_A	0x1400
#define PDM0_COEFFICIENT_B	0x1800
#define PDM1			0x2000
#define PDM1_COEFFICIENT_A	0x2400
#define PDM1_COEFFICIENT_B	0x2800
#define PDM2			0x3000
#define PDM2_COEFFICIENT_A	0x3400
#define PDM2_COEFFICIENT_B	0x3800
#define PDM3			0x4000
#define PDM3_COEFFICIENT_A	0x4400
#define PDM3_COEFFICIENT_B	0x4800
#define PDM_COEF_RAM_A_LENGTH	0x0400
#define PDM_COEF_RAM_B_LENGTH	0x0400

/* Local registers in each PDMx */
#define CIC_CONTROL		0x000
#define CIC_CONFIG		0x004
#define MIC_CONTROL		0x00c
#define FIR_CONTROL_A		0x020
#define FIR_CONFIG_A		0x024
#define DC_OFFSET_LEFT_A	0x028
#define DC_OFFSET_RIGHT_A	0x02c
#define OUT_GAIN_LEFT_A		0x030
#define OUT_GAIN_RIGHT_A	0x034
#define FIR_CONTROL_B		0x040
#define FIR_CONFIG_B		0x044
#define DC_OFFSET_LEFT_B	0x048
#define DC_OFFSET_RIGHT_B	0x04c
#define OUT_GAIN_LEFT_B		0x050
#define OUT_GAIN_RIGHT_B	0x054

/* Register bits */

/* OUTCONTROLx IPM bit fields style */
#define OUTCONTROL0_BFTH_MAX	4 /* Max depth 16 */

/* OUTCONTROL0 bits */
#define OUTCONTROL0_TIE_BIT			BIT(27)
#define OUTCONTROL0_SIP_BIT			BIT(26)
#define OUTCONTROL0_FINIT_BIT			BIT(25)
#define OUTCONTROL0_FCI_BIT			BIT(24)
#define OUTCONTROL0_TIE(x)			SET_BIT(27, x)
#define OUTCONTROL0_SIP(x)			SET_BIT(26, x)
#define OUTCONTROL0_FINIT(x)			SET_BIT(25, x)
#define OUTCONTROL0_FCI(x)			SET_BIT(24, x)
#define OUTCONTROL0_BFTH(x)			SET_BITS(23, 20, x)
#define OUTCONTROL0_OF(x)			SET_BITS(19, 18, x)
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#define OUTCONTROL0_IPM(x)                      SET_BITS(17, 15, x)
#else
#define OUTCONTROL0_IPM(x)                      SET_BITS(17, 16, x)
#endif
#define OUTCONTROL0_IPM_SOURCE_1(x)		SET_BITS(14, 13, x)
#define OUTCONTROL0_IPM_SOURCE_2(x)		SET_BITS(12, 11, x)
#define OUTCONTROL0_IPM_SOURCE_3(x)		SET_BITS(10, 9, x)
#define OUTCONTROL0_IPM_SOURCE_4(x)		SET_BITS(8, 7, x)
#define OUTCONTROL0_IPM_SOURCE_MODE(x)		SET_BIT(6, x)
#define OUTCONTROL0_TH(x)			SET_BITS(5, 0, x)
#define OUTCONTROL0_TIE_GET(x)			GET_BIT(27, x)
#define OUTCONTROL0_SIP_GET(x)			GET_BIT(26, x)
#define OUTCONTROL0_FINIT_GET(x)		GET_BIT(25, x)
#define OUTCONTROL0_FCI_GET(x)			GET_BIT(24, x)
#define OUTCONTROL0_BFTH_GET(x)			GET_BITS(23, 20, x)
#define OUTCONTROL0_OF_GET(x)			GET_BITS(19, 18, x)
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#define OUTCONTROL0_IPM_GET(x)			GET_BITS(17, 15, x)
#else
#define OUTCONTROL0_IPM_GET(x)			GET_BITS(17, 16, x)
#endif
#define OUTCONTROL0_IPM_SOURCE_1_GET(x)		GET_BITS(14, 13, x)
#define OUTCONTROL0_IPM_SOURCE_2_GET(x)		GET_BITS(12, 11, x)
#define OUTCONTROL0_IPM_SOURCE_3_GET(x)		GET_BITS(10,  9, x)
#define OUTCONTROL0_IPM_SOURCE_4_GET(x)		GET_BITS(8, 7, x)
#define OUTCONTROL0_IPM_SOURCE_MODE_GET(x)	GET_BIT(6, x)
#define OUTCONTROL0_TH_GET(x)			GET_BITS(5, 0, x)

/* OUTCONTROL1 bits */
#define OUTCONTROL1_TIE_BIT			BIT(27)
#define OUTCONTROL1_SIP_BIT			BIT(26)
#define OUTCONTROL1_FINIT_BIT			BIT(25)
#define OUTCONTROL1_FCI_BIT			BIT(24)
#define OUTCONTROL1_TIE(x)			SET_BIT(27, x)
#define OUTCONTROL1_SIP(x)			SET_BIT(26, x)
#define OUTCONTROL1_FINIT(x)			SET_BIT(25, x)
#define OUTCONTROL1_FCI(x)			SET_BIT(24, x)
#define OUTCONTROL1_BFTH(x)			SET_BITS(23, 20, x)
#define OUTCONTROL1_OF(x)			SET_BITS(19, 18, x)
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#define OUTCONTROL1_IPM(x)                      SET_BITS(17, 15, x)
#else
#define OUTCONTROL1_IPM(x)                      SET_BITS(17, 16, x)
#endif
#define OUTCONTROL1_IPM_SOURCE_1(x)		SET_BITS(14, 13, x)
#define OUTCONTROL1_IPM_SOURCE_2(x)		SET_BITS(12, 11, x)
#define OUTCONTROL1_IPM_SOURCE_3(x)		SET_BITS(10, 9, x)
#define OUTCONTROL1_IPM_SOURCE_4(x)		SET_BITS(8, 7, x)
#define OUTCONTROL1_IPM_SOURCE_MODE(x)		SET_BIT(6, x)
#define OUTCONTROL1_TH(x)			SET_BITS(5, 0, x)
#define OUTCONTROL1_TIE_GET(x)			GET_BIT(27, x)
#define OUTCONTROL1_SIP_GET(x)			GET_BIT(26, x)
#define OUTCONTROL1_FINIT_GET(x)		GET_BIT(25, x)
#define OUTCONTROL1_FCI_GET(x)			GET_BIT(24, x)
#define OUTCONTROL1_BFTH_GET(x)			GET_BITS(23, 20, x)
#define OUTCONTROL1_OF_GET(x)			GET_BITS(19, 18, x)
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#define OUTCONTROL1_IPM_GET(x)			GET_BITS(17, 15, x)
#else
#define OUTCONTROL1_IPM_GET(x)			GET_BITS(17, 16, x)
#endif
#define OUTCONTROL1_IPM_SOURCE_1_GET(x)		GET_BITS(14, 13, x)
#define OUTCONTROL1_IPM_SOURCE_2_GET(x)		GET_BITS(12, 11, x)
#define OUTCONTROL1_IPM_SOURCE_3_GET(x)		GET_BITS(10,  9, x)
#define OUTCONTROL1_IPM_SOURCE_4_GET(x)		GET_BITS(8, 7, x)
#define OUTCONTROL1_IPM_SOURCE_MODE_GET(x)	GET_BIT(6, x)
#define OUTCONTROL1_TH_GET(x)			GET_BITS(5, 0, x)

#define OUTCONTROLX_IPM_NUMSOURCES		4


/* OUTSTAT0 bits */
#define OUTSTAT0_AFE_BIT	BIT(31)
#define OUTSTAT0_ASNE_BIT	BIT(29)
#define OUTSTAT0_RFS_BIT	BIT(28)
#define OUTSTAT0_ROR_BIT	BIT(27)
#define OUTSTAT0_FL_MASK	MASK(6, 0)

/* OUTSTAT1 bits */
#define OUTSTAT1_AFE_BIT	BIT(31)
#define OUTSTAT1_ASNE_BIT	BIT(29)
#define OUTSTAT1_RFS_BIT	BIT(28)
#define OUTSTAT1_ROR_BIT	BIT(27)
#define OUTSTAT1_FL_MASK	MASK(6, 0)

/* CIC_CONTROL bits */
#define CIC_CONTROL_SOFT_RESET_BIT	BIT(16)
#define CIC_CONTROL_CIC_START_B_BIT	BIT(15)
#define CIC_CONTROL_CIC_START_A_BIT	BIT(14)
#define CIC_CONTROL_MIC_B_POLARITY_BIT	BIT(3)
#define CIC_CONTROL_MIC_A_POLARITY_BIT	BIT(2)
#define CIC_CONTROL_MIC_MUTE_BIT	BIT(1)
#define CIC_CONTROL_STEREO_MODE_BIT	BIT(0)

#define CIC_CONTROL_SOFT_RESET(x)	SET_BIT(16, x)
#define CIC_CONTROL_CIC_START_B(x)	SET_BIT(15, x)
#define CIC_CONTROL_CIC_START_A(x)	SET_BIT(14, x)
#define CIC_CONTROL_MIC_B_POLARITY(x)	SET_BIT(3, x)
#define CIC_CONTROL_MIC_A_POLARITY(x)	SET_BIT(2, x)
#define CIC_CONTROL_MIC_MUTE(x)		SET_BIT(1, x)
#define CIC_CONTROL_STEREO_MODE(x)	SET_BIT(0, x)

#define CIC_CONTROL_SOFT_RESET_GET(x)		GET_BIT(16, x)
#define CIC_CONTROL_CIC_START_B_GET(x)		GET_BIT(15, x)
#define CIC_CONTROL_CIC_START_A_GET(x)		GET_BIT(14, x)
#define CIC_CONTROL_MIC_B_POLARITY_GET(x)	GET_BIT(3, x)
#define CIC_CONTROL_MIC_A_POLARITY_GET(x)	GET_BIT(2, x)
#define CIC_CONTROL_MIC_MUTE_GET(x)		GET_BIT(1, x)
#define CIC_CONTROL_STEREO_MODE_GET(x)		GET_BIT(0, x)

/* CIC_CONFIG bits */
#define CIC_CONFIG_CIC_SHIFT(x)		SET_BITS(27, 24, x)
#define CIC_CONFIG_COMB_COUNT(x)	SET_BITS(15, 8, x)

/* CIC_CONFIG masks */
#define CIC_CONFIG_CIC_SHIFT_MASK	MASK(27, 24)
#define CIC_CONFIG_COMB_COUNT_MASK	MASK(15, 8)

#define CIC_CONFIG_CIC_SHIFT_GET(x)	GET_BITS(27, 24, x)
#define CIC_CONFIG_COMB_COUNT_GET(x)	GET_BITS(15, 8, x)

/* MIC_CONTROL bits */
#define MIC_CONTROL_PDM_EN_B_BIT	BIT(1)
#define MIC_CONTROL_PDM_EN_A_BIT	BIT(0)
#define MIC_CONTROL_PDM_CLKDIV(x)	SET_BITS(15, 8, x)
#define MIC_CONTROL_PDM_SKEW(x)		SET_BITS(7, 4, x)
#define MIC_CONTROL_CLK_EDGE(x)		SET_BIT(3, x)
#define MIC_CONTROL_PDM_EN_B(x)		SET_BIT(1, x)
#define MIC_CONTROL_PDM_EN_A(x)		SET_BIT(0, x)

/* MIC_CONTROL masks */
#define MIC_CONTROL_PDM_CLKDIV_MASK	MASK(15, 8)

#define MIC_CONTROL_PDM_CLKDIV_GET(x)	GET_BITS(15, 8, x)
#define MIC_CONTROL_PDM_SKEW_GET(x)	GET_BITS(7, 4, x)
#define MIC_CONTROL_PDM_CLK_EDGE_GET(x)	GET_BIT(3, x)
#define MIC_CONTROL_PDM_EN_B_GET(x)	GET_BIT(1, x)
#define MIC_CONTROL_PDM_EN_A_GET(x)	GET_BIT(0, x)

/* FIR_CONTROL_A bits */
#define FIR_CONTROL_A_START_BIT			BIT(7)
#define FIR_CONTROL_A_ARRAY_START_EN_BIT	BIT(6)
#define FIR_CONTROL_A_MUTE_BIT			BIT(1)
#define FIR_CONTROL_A_START(x)			SET_BIT(7, x)
#define FIR_CONTROL_A_ARRAY_START_EN(x)		SET_BIT(6, x)
#define FIR_CONTROL_A_DCCOMP(x)			SET_BIT(4, x)
#define FIR_CONTROL_A_MUTE(x)			SET_BIT(1, x)
#define FIR_CONTROL_A_STEREO(x)			SET_BIT(0, x)

#define FIR_CONTROL_A_START_GET(x)		GET_BIT(7, x)
#define FIR_CONTROL_A_ARRAY_START_EN_GET(x)	GET_BIT(6, x)
#define FIR_CONTROL_A_DCCOMP_GET(x)		GET_BIT(4, x)
#define FIR_CONTROL_A_MUTE_GET(x)		GET_BIT(1, x)
#define FIR_CONTROL_A_STEREO_GET(x)		GET_BIT(0, x)

/* FIR_CONFIG_A bits */
#define FIR_CONFIG_A_FIR_DECIMATION(x)		SET_BITS(20, 16, x)
#define FIR_CONFIG_A_FIR_SHIFT(x)		SET_BITS(11, 8, x)
#define FIR_CONFIG_A_FIR_LENGTH(x)		SET_BITS(7, 0, x)

#define FIR_CONFIG_A_FIR_DECIMATION_GET(x)	GET_BITS(20, 16, x)
#define FIR_CONFIG_A_FIR_SHIFT_GET(x)		GET_BITS(11, 8, x)
#define FIR_CONFIG_A_FIR_LENGTH_GET(x)		GET_BITS(7, 0, x)

/* DC offset compensation time constants */
#define DCCOMP_TC0	0
#define DCCOMP_TC1	1
#define DCCOMP_TC2	2
#define DCCOMP_TC3	3
#define DCCOMP_TC4	4
#define DCCOMP_TC5	5
#define DCCOMP_TC6	6
#define DCCOMP_TC7	7

/* DC_OFFSET_LEFT_A bits */
#define DC_OFFSET_LEFT_A_DC_OFFS(x)		SET_BITS(21, 0, x)

/* DC_OFFSET_RIGHT_A bits */
#define DC_OFFSET_RIGHT_A_DC_OFFS(x)		SET_BITS(21, 0, x)

/* OUT_GAIN_LEFT_A bits */
#define OUT_GAIN_LEFT_A_GAIN(x)			SET_BITS(19, 0, x)

/* OUT_GAIN_RIGHT_A bits */
#define OUT_GAIN_RIGHT_A_GAIN(x)		SET_BITS(19, 0, x)

/* FIR_CONTROL_B bits */
#define FIR_CONTROL_B_START_BIT			BIT(7)
#define FIR_CONTROL_B_ARRAY_START_EN_BIT	BIT(6)
#define FIR_CONTROL_B_MUTE_BIT			BIT(1)
#define FIR_CONTROL_B_START(x)			SET_BIT(7, x)
#define FIR_CONTROL_B_ARRAY_START_EN(x)		SET_BIT(6, x)
#define FIR_CONTROL_B_DCCOMP(x)			SET_BIT(4, x)
#define FIR_CONTROL_B_MUTE(x)			SET_BIT(1, x)
#define FIR_CONTROL_B_STEREO(x)			SET_BIT(0, x)

#define FIR_CONTROL_B_START_GET(x)		GET_BIT(7, x)
#define FIR_CONTROL_B_ARRAY_START_EN_GET(x)	GET_BIT(6, x)
#define FIR_CONTROL_B_DCCOMP_GET(x)		GET_BIT(4, x)
#define FIR_CONTROL_B_MUTE_GET(x)		GET_BIT(1, x)
#define FIR_CONTROL_B_STEREO_GET(x)		GET_BIT(0, x)

/* FIR_CONFIG_B bits */
#define FIR_CONFIG_B_FIR_DECIMATION(x)		SET_BITS(20, 16, x)
#define FIR_CONFIG_B_FIR_SHIFT(x)		SET_BITS(11, 8, x)
#define FIR_CONFIG_B_FIR_LENGTH(x)		SET_BITS(7, 0, x)

#define FIR_CONFIG_B_FIR_DECIMATION_GET(x)	GET_BITS(20, 16, x)
#define FIR_CONFIG_B_FIR_SHIFT_GET(x)		GET_BITS(11, 8, x)
#define FIR_CONFIG_B_FIR_LENGTH_GET(x)		GET_BITS(7, 0, x)

/* DC_OFFSET_LEFT_B bits */
#define DC_OFFSET_LEFT_B_DC_OFFS(x)		SET_BITS(21, 0, x)

/* DC_OFFSET_RIGHT_B bits */
#define DC_OFFSET_RIGHT_B_DC_OFFS(x)		SET_BITS(21, 0, x)

/* OUT_GAIN_LEFT_B bits */
#define OUT_GAIN_LEFT_B_GAIN(x)			SET_BITS(19, 0, x)

/* OUT_GAIN_RIGHT_B bits */
#define OUT_GAIN_RIGHT_B_GAIN(x)		SET_BITS(19, 0, x)

/* FIR coefficients */
#define FIR_COEF_A(x)				SET_BITS(19, 0, x)
#define FIR_COEF_B(x)				SET_BITS(19, 0, x)

/* Used for scaling FIR coefficients for HW */
#define DMIC_HW_FIR_COEF_MAX ((1 << (DMIC_HW_BITS_FIR_COEF - 1)) - 1)
#define DMIC_HW_FIR_COEF_Q (DMIC_HW_BITS_FIR_COEF - 1)

/* Internal precision in gains computation, e.g. Q4.28 in int32_t */
#define DMIC_FIR_SCALE_Q 28

/* Used in unmute ramp values calculation */
#define DMIC_HW_FIR_GAIN_MAX ((1 << (DMIC_HW_BITS_FIR_GAIN - 1)) - 1)

#define DB2LIN_FIXED_INPUT_QY 24
#define DB2LIN_FIXED_OUTPUT_QY 20

/* Hardwired log ramp parameters. The first value is the initial gain in
 * decibels. The default ramp time is provided by 1st order equation
 * ramp time = coef * samplerate + offset. The default ramp is 200 ms for
 * 48 kHz and 400 ms for 16 kHz.
 */
#define LOGRAMP_START_DB Q_CONVERT_FLOAT(-90, DB2LIN_FIXED_INPUT_QY)
#define LOGRAMP_TIME_COEF_Q15 -205 /* dy/dx (16000,400) (48000,200) */
#define LOGRAMP_TIME_OFFS_Q0 500 /* Offset for line slope */

/* Limits for ramp time from topology */
#define LOGRAMP_TIME_MIN_MS 10 /* Min. 10 ms */
#define LOGRAMP_TIME_MAX_MS 1000 /* Max. 1s */

/* Simplify log ramp step calculation equation with this constant term */
#define LOGRAMP_CONST_TERM ((int32_t) \
	((int64_t)-LOGRAMP_START_DB * DMIC_UNMUTE_RAMP_US / 1000))

/* Fractional shift for gain update. Gain format is Q2.30. */
#define Q_SHIFT_GAIN_X_GAIN_COEF \
	(Q_SHIFT_BITS_32(30, DB2LIN_FIXED_OUTPUT_QY, 30))

/* Compute the number of shifts
 * This will result in a compiler overflow error if shift bits are out of
 * range as INT64_MAX/MIN is greater than 32 bit Q shift parameter
 */
#define Q_SHIFT_BITS_64(qx, qy, qz) \
	((qx + qy - qz) <= 63 ? (((qx + qy - qz) >= 0) ? \
	 (qx + qy - qz) : INT64_MIN) : INT64_MAX)

#define Q_SHIFT_BITS_32(qx, qy, qz) \
	((qx + qy - qz) <= 31 ? (((qx + qy - qz) >= 0) ? \
	 (qx + qy - qz) : INT32_MIN) : INT32_MAX)

/* Fractional multiplication with shift and round
 * Note that the parameters px and py must be cast to (int64_t) if other type.
 */
#define Q_MULTSR_32X32(px, py, qx, qy, qp) \
	((((px) * (py) >> ((qx)+(qy)-(qp)-1)) + 1) >> 1)

/* A more clever macro for Q-shifts */
#define Q_SHIFT(x, src_q, dst_q) ((x) >> ((src_q) - (dst_q)))
#define Q_SHIFT_RND(x, src_q, dst_q) \
	((((x) >> ((src_q) - (dst_q) - 1)) + 1) >> 1)

/* Alternative version since compiler does not allow (x >> -1) */
#define Q_SHIFT_LEFT(x, src_q, dst_q) ((x) << ((dst_q) - (src_q)))

/* Convert a float number to fractional Qnx.ny format. Note that there is no
 * check for nx+ny number of bits to fit the word length of int. The parameter
 * qy must be 31 or less.
 */
#define Q_CONVERT_FLOAT(f, qy) \
	((int32_t)(((const double)f) * ((int64_t)1 << (const int)qy) + 0.5))

#define TWO_Q27         Q_CONVERT_FLOAT(2.0, 27)     /* Use Q5.27 */
#define MINUS_TWO_Q27   Q_CONVERT_FLOAT(-2.0, 27)    /* Use Q5.27 */
#define ONE_Q20         Q_CONVERT_FLOAT(1.0, 20)     /* Use Q12.20 */
#define ONE_Q23         Q_CONVERT_FLOAT(1.0, 23)     /* Use Q9.23 */
#define LOG10_DIV20_Q27 Q_CONVERT_FLOAT(0.1151292546, 27) /* Use Q5.27 */

#define DMA_HANDSHAKE_DMIC_CH0	0
#define DMA_HANDSHAKE_DMIC_CH1	1

/* For NHLT DMIC configuration parsing */
#define DMIC_HW_CONTROLLERS_MAX	4
#define DMIC_HW_FIFOS_MAX	2

struct nhlt_dmic_gateway_attributes {
	uint32_t dw;
};

struct nhlt_dmic_ts_group {
	uint32_t ts_group[4];
};

struct nhlt_dmic_clock_on_delay {
	uint32_t clock_on_delay;
};

struct nhlt_dmic_channel_ctrl_mask {
	uint32_t channel_ctrl_mask;
};

struct nhlt_pdm_ctrl_mask {
	uint32_t pdm_ctrl_mask;
};

struct nhlt_pdm_ctrl_cfg {
	uint32_t cic_control;
	uint32_t cic_config;
	uint32_t reserved0;
	uint32_t mic_control;
	uint32_t pdm_sdw_map;
	uint32_t reuse_fir_from_pdm;
	uint32_t reserved1[2];
};

struct nhlt_pdm_ctrl_fir_cfg {
	uint32_t fir_control;
	uint32_t fir_config;
	int32_t dc_offset_left;
	int32_t dc_offset_right;
	int32_t out_gain_left;
	int32_t out_gain_right;
	uint32_t reserved[2];
};

struct nhlt_pdm_fir_coeffs {
	int32_t fir_coeffs[0];
};

enum dai_dmic_frame_format {
	DAI_DMIC_FRAME_S16_LE = 0,
	DAI_DMIC_FRAME_S24_4LE,
	DAI_DMIC_FRAME_S32_LE,
	DAI_DMIC_FRAME_FLOAT,
	/* other formats here */
	DAI_DMIC_FRAME_S24_3LE,
};

/* Common data for all DMIC DAI instances */
struct dai_dmic_global_shared {
	uint32_t active_fifos_mask;	/* Bits (dai->index) are set to indicate active FIFO */
	uint32_t pause_mask;		/* Bits (dai->index) are set to indicate driver pause */
};

struct dai_dmic_plat_fifo_data {
	uint32_t offset;
	uint32_t width;
	uint32_t depth;
	uint32_t watermark;
	uint32_t handshake;
};


struct dai_intel_dmic {
	struct dai_config dai_config_params;

	struct k_spinlock lock;				/**< locking mechanism */
	int sref;					/**< simple ref counter, guarded by lock */
	enum dai_state state;				/* Driver component state */
	uint16_t enable[CONFIG_DAI_DMIC_HW_CONTROLLERS];/* Mic 0 and 1 enable bits array for PDMx */
	struct dai_dmic_plat_fifo_data fifo;		/* dmic capture fifo stream */
	int32_t gain_coef;				/* Gain update constant */
	int32_t gain;					/* Gain value to be applied to HW */
	int32_t startcount;				/* Counter that controls HW unmute */
	int32_t unmute_time_ms;				/* Unmute ramp time in milliseconds */

	/* hardware parameters */
	uint32_t reg_base;
	uint32_t shim_base;
	int irq;
	uint32_t flags;
};

static inline int32_t sat_int32(int64_t x)
{
	if (x > INT32_MAX)
		return INT32_MAX;
	else if (x < INT32_MIN)
		return INT32_MIN;
	else
		return (int32_t)x;
}
/* Fractional multiplication with shift and saturation */
static inline int32_t q_multsr_sat_32x32(int32_t x, int32_t y,
					 const int shift_bits)
{
	return sat_int32(((((int64_t)x * y) >> (shift_bits - 1)) + 1) >> 1);
}

static inline int dmic_get_unmute_ramp_from_samplerate(int rate)
{
	int time_ms;

	time_ms = Q_MULTSR_32X32((int32_t)rate, LOGRAMP_TIME_COEF_Q15, 0, 15, 0) +
		LOGRAMP_TIME_OFFS_Q0;
	if (time_ms > LOGRAMP_TIME_MAX_MS)
		return LOGRAMP_TIME_MAX_MS;

	if (time_ms < LOGRAMP_TIME_MIN_MS)
		return LOGRAMP_TIME_MIN_MS;

	return time_ms;
}

#endif /* __INTEL_DAI_DRIVER_DMIC_H__ */
