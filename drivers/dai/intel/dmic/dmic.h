/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_H__
#define __INTEL_DAI_DRIVER_DMIC_H__

#include <zephyr/sys/util_macro.h>

/* The microphones create a low frequecy thump sound when clock is enabled.
 * The unmute linear gain ramp chacteristic is defined here.
 * NOTE: Do not set any of these to 0.
 */
#define DMIC_UNMUTE_RAMP_US	1000	/* 1 ms (in microseconds) */
#define DMIC_UNMUTE_CIC		1	/* Unmute CIC at 1 ms */
#define DMIC_UNMUTE_FIR		2	/* Unmute FIR at 2 ms */

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

/* DC offset compensation time constants */
#define DCCOMP_TC0	0
#define DCCOMP_TC1	1
#define DCCOMP_TC2	2
#define DCCOMP_TC3	3
#define DCCOMP_TC4	4
#define DCCOMP_TC5	5
#define DCCOMP_TC6	6
#define DCCOMP_TC7	7

/* Used for scaling FIR coefficients for HW */
#define DMIC_HW_FIR_COEF_MAX ((1 << (DMIC_HW_BITS_FIR_COEF - 1)) - 1)
#define DMIC_HW_FIR_COEF_Q (DMIC_HW_BITS_FIR_COEF - 1)

/* Internal precision in gains computation, e.g. Q4.28 in int32_t */
#define DMIC_FIR_SCALE_Q 28

/* Used in unmute ramp values calculation */
#define DMIC_HW_FIR_GAIN_MAX ((1 << (DMIC_HW_BITS_FIR_GAIN - 1)) - 1)

#define DB2LIN_FIXED_INPUT_QY 24
#define DB2LIN_FIXED_OUTPUT_QY 20

/* Hardcoded log ramp parameters. The default ramp is 100 ms for 48 kHz
 * and 200 ms for 16 kHz. The first parameter is the initial gain in
 * decibels, set to -90 dB.
 * The rate dependent ramp duration is provided by 1st order equation
 * duration = coef * samplerate + offset.
 * E.g. 100 ms @ 48 kHz, 200 ms @ 16 kHz
 * y48 = 100; y16 = 200;
 * dy = y48 - y16; dx = 48000 - 16000;
 * coef = round(dy/dx * 2^15)
 * offs = round(y16 - coef/2^15 * 16000)
 * Note: The rate dependence can be disabled with zero time_coef with
 * use of just the offset.
 */
#define LOGRAMP_START_DB Q_CONVERT_FLOAT(-90, DB2LIN_FIXED_INPUT_QY)
#define LOGRAMP_TIME_COEF_Q15 -102 /* coef = dy/dx */
#define LOGRAMP_TIME_OFFS_Q0 250 /* offs = Offset for line slope in ms */

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
#if defined(CONFIG_SOC_INTEL_ACE20_LNL) || defined(CONFIG_SOC_INTEL_ACE30) ||                      \
	defined(CONFIG_SOC_INTEL_ACE40)
	uint32_t hdamldmic_base;
	uint32_t vshim_base;
#endif
	int irq;
	uint32_t flags;
	uint32_t gain_left;
	uint32_t gain_right;
};

static inline int32_t sat_int32(int64_t x)
{
	if (x > INT32_MAX) {
		return INT32_MAX;
	} else if (x < INT32_MIN) {
		return INT32_MIN;
	} else {
		return (int32_t)x;
	}
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
	if (time_ms > LOGRAMP_TIME_MAX_MS) {
		return LOGRAMP_TIME_MAX_MS;
	}

	if (time_ms < LOGRAMP_TIME_MIN_MS) {
		return LOGRAMP_TIME_MIN_MS;
	}

	return time_ms;
}

#endif /* __INTEL_DAI_DRIVER_DMIC_H__ */
