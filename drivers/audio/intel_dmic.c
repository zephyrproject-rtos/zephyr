/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 *
 * Author:	Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>
 *		Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *		Keyon Jie <yang.jie@linux.intel.com>
 *		Sathish Kuttan <sathish.k.kuttan@intel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_dmic

#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>

#include <zephyr/audio/dmic.h>
#include "intel_dmic.h"
#include "decimation/pdm_decim_fir.h"

#define DMA_CHANNEL_DMIC_RXA DT_INST_DMAS_CELL_BY_NAME(0, rx_a, channel)
#define DMA_CHANNEL_DMIC_RXB DT_INST_DMAS_CELL_BY_NAME(0, rx_b, channel)

#define LOG_LEVEL CONFIG_AUDIO_DMIC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_dmic);

/*
 * Maximum number of PDM controller instances supported by this driver
 * configuration data types are selected based on this max.
 * For example, uint32_t is selected when a config parameter is 4bits wide
 * and 8 instances fit within a 32 bit type
 */
#define MAX_PDM_CONTROLLERS_SUPPORTED	8
/* Actual number of hardware controllers */
#define DMIC_HW_CONTROLLERS		4

#define DMIC_MAX_MODES 50

/* HW FIR pipeline needs 5 additional cycles per channel for internal
 * operations. This is used in MAX filter length check.
 */
#define DMIC_FIR_PIPELINE_OVERHEAD 5

struct decim_modes {
	int16_t clkdiv[DMIC_MAX_MODES];
	int16_t mcic[DMIC_MAX_MODES];
	int16_t mfir[DMIC_MAX_MODES];
	int num_of_modes;
};

struct matched_modes {
	int16_t clkdiv[DMIC_MAX_MODES];
	int16_t mcic[DMIC_MAX_MODES];
	int16_t mfir_a[DMIC_MAX_MODES];
	int16_t mfir_b[DMIC_MAX_MODES];
	int num_of_modes;
};

struct dmic_configuration {
	struct pdm_decim *fir_a;
	struct pdm_decim *fir_b;
	int clkdiv;
	int mcic;
	int mfir_a;
	int mfir_b;
	int cic_shift;
	int fir_a_shift;
	int fir_b_shift;
	int fir_a_length;
	int fir_b_length;
	int32_t fir_a_scale;
	int32_t fir_b_scale;
};

/* Minimum OSR is always applied for 48 kHz and less sample rates */
#define DMIC_MIN_OSR  50

/* These are used as guideline for configuring > 48 kHz sample rates. The
 * minimum OSR can be relaxed down to 40 (use 3.84 MHz clock for 96 kHz).
 */
#define DMIC_HIGH_RATE_MIN_FS	64000
#define DMIC_HIGH_RATE_OSR_MIN	40

/* Used for scaling FIR coefficients for HW */
#define DMIC_HW_FIR_COEF_MAX ((1 << (DMIC_HW_BITS_FIR_COEF - 1)) - 1)
#define DMIC_HW_FIR_COEF_Q (DMIC_HW_BITS_FIR_COEF - 1)

/* Internal precision in gains computation, e.g. Q4.28 in int32_t */
#define DMIC_FIR_SCALE_Q 28

/* Fractional multiplication with shift and round
 * Note that the parameters px and py must be cast to (int64_t) if other type.
 */
#define Q_MULTSR_32X32(px, py, qx, qy, qp) \
	((((px) * (py) >> ((qx)+(qy)-(qp)-1)) + 1) >> 1)

/* Saturation */
#define SATP_INT32(x) (((x) > INT32_MAX) ? INT32_MAX : (x))
#define SATM_INT32(x) (((x) < INT32_MIN) ? INT32_MIN : (x))

/* Macros to set bit(s) */
#define SET_BIT(b, x)		(((x) & 1) << (b))
#define SET_BITS(b_hi, b_lo, x)	\
	(((x) & ((1 << ((b_hi) - (b_lo) + 1)) - 1)) << (b_lo))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* queue size to hold buffers in process */
#define DMIC_BUF_Q_LEN		2

#define DMIC_REG_RD(reg)	(*((volatile uint32_t *)(PDM_BASE + (reg))))
#define DMIC_REG_WR(reg, val)	\
	(*((volatile uint32_t *)(PDM_BASE + (reg))) = (val))
#define DMIC_REG_UPD(reg, mask, val)		\
	DMIC_REG_WR((reg), (DMIC_REG_RD((reg)) & ~(mask)) | ((val) & (mask)))

struct _stream_data {
	struct k_msgq in_queue;
	struct k_msgq out_queue;
	void *in_msgs[DMIC_BUF_Q_LEN];
	void *out_msgs[DMIC_BUF_Q_LEN];
	struct k_mem_slab *mem_slab;
	size_t block_size;
};

/* DMIC private data */
static struct _dmic_pdata {
	enum dmic_state state;
	uint16_t fifo_a;
	uint16_t fifo_b;
	uint16_t mic_en_mask;
	uint8_t num_streams;
	uint8_t reserved;
	struct _stream_data streams[DMIC_MAX_STREAMS];
	const struct device *dma_dev;
} dmic_private;

static inline void dmic_parse_channel_map(uint32_t channel_map_lo,
		uint32_t channel_map_hi, uint8_t channel, uint8_t *pdm, enum pdm_lr *lr);
static inline uint8_t dmic_parse_clk_skew_map(uint32_t skew_map, uint8_t pdm);
static void dmic_stop(void);

/* This function searches from vec[] (of length vec_length) integer values
 * of n. The indexes to equal values is returned in idx[]. The function
 * returns the number of found matches. The max_results should be set to
 * 0 (or negative) or vec_length get all the matches. The max_result can be set
 * to 1 to receive only the first match in ascending order. It avoids need
 * for an array for idx.
 */
int find_equal_int16(int16_t idx[], int16_t vec[], int n, int vec_length,
	int max_results)
{
	int nresults = 0;
	int i;

	for (i = 0; i < vec_length; i++) {
		if (vec[i] == n) {
			idx[nresults++] = i;
			if (nresults == max_results) {
				break;
			}
		}
	}

	return nresults;
}

/* Return the smallest value found in the vector */
int16_t find_min_int16(int16_t vec[], int vec_length)
{
	int i;
	int min = vec[0];

	for (i = 1; i < vec_length; i++) {
		min = (vec[i] < min) ? vec[i] : min;
	}

	return min;
}

/* Return the largest absolute value found in the vector. Note that
 * smallest negative value need to be saturated to preset as int32_t.
 */
int32_t find_max_abs_int32(int32_t vec[], int vec_length)
{
	int i;
	int64_t amax = (vec[0] > 0) ? vec[0] : -vec[0];

	for (i = 1; i < vec_length; i++) {
		amax = (vec[i] > amax) ? vec[i] : amax;
		amax = (-vec[i] > amax) ? -vec[i] : amax;
	}

	return SATP_INT32(amax); /* Amax is always a positive value */
}

/* Count the left shift amount to normalize a 32 bit signed integer value
 * without causing overflow. Input value 0 will result to 31.
 */
int norm_int32(int32_t val)
{
	if (val == 0) {
		return 31;
	}

	if (val < 0) {
		val = -val;
	}

	return __builtin_clz(val) - 1;
}

/* This function returns a raw list of potential microphone clock and decimation
 * modes for achieving requested sample rates. The search is constrained by
 * decimation HW capabilities and setup parameters. The parameters such as
 * microphone clock min/max and duty cycle requirements need be checked from
 * used microphone component datasheet.
 */
static void find_modes(struct decim_modes *modes,
		struct dmic_cfg *config, uint32_t fs)
{
	int clkdiv_min;
	int clkdiv_max;
	int clkdiv;
	int c1;
	int du_min;
	int du_max;
	int pdmclk;
	int osr;
	int mfir;
	int mcic;
	int ioclk_test;
	int osr_min = DMIC_MIN_OSR;
	int i = 0;

	/* Defaults, empty result */
	modes->num_of_modes = 0;

	/* The FIFO is not requested if sample rate is set to zero. Just
	 * return in such case with num_of_modes as zero.
	 */
	if (fs == 0U) {
		return;
	}

	/* Override DMIC_MIN_OSR for very high sample rates, use as minimum
	 * the nominal clock for the high rates.
	 */
	if (fs >= DMIC_HIGH_RATE_MIN_FS) {
		osr_min = DMIC_HIGH_RATE_OSR_MIN;
	}

	/* Check for sane pdm clock, min 100 kHz, max ioclk/2 */
	if ((config->io.max_pdm_clk_freq < DMIC_HW_PDM_CLK_MIN) ||
		(config->io.max_pdm_clk_freq > (DMIC_HW_IOCLK / 2))) {
		LOG_ERR("max_pdm_clk_freq %u invalid",
				config->io.max_pdm_clk_freq);
		return;
	}

	if ((config->io.min_pdm_clk_freq < DMIC_HW_PDM_CLK_MIN) ||
		(config->io.min_pdm_clk_freq > config->io.max_pdm_clk_freq)) {
		LOG_ERR("min_pdm_clk_freq %u invalid",
				config->io.min_pdm_clk_freq);
		return;
	}

	/* Check for sane duty cycle */
	if (config->io.min_pdm_clk_dc > config->io.max_pdm_clk_dc) {
		LOG_ERR("min_pdm_clk_dc %u max_pdm_clk_dc %u invalid",
				config->io.min_pdm_clk_dc,
				config->io.max_pdm_clk_dc);
		return;
	}

	if ((config->io.min_pdm_clk_dc < DMIC_HW_DUTY_MIN) ||
		(config->io.min_pdm_clk_dc > DMIC_HW_DUTY_MAX)) {
		LOG_ERR("min_pdm_clk_dc %u invalid",
				config->io.min_pdm_clk_dc);
		return;
	}

	if ((config->io.max_pdm_clk_dc < DMIC_HW_DUTY_MIN) ||
		(config->io.max_pdm_clk_dc > DMIC_HW_DUTY_MAX)) {
		LOG_ERR("max_pdm_clk_dc %u invalid", config->io.max_pdm_clk_dc);
		return;
	}

	/* Min and max clock dividers */
	clkdiv_min = (DMIC_HW_IOCLK + config->io.max_pdm_clk_freq - 1) /
		config->io.max_pdm_clk_freq;
	clkdiv_min = (clkdiv_min > DMIC_HW_CIC_DECIM_MIN) ?
		clkdiv_min : DMIC_HW_CIC_DECIM_MIN;
	clkdiv_max = DMIC_HW_IOCLK / config->io.min_pdm_clk_freq;

	/* Loop possible clock dividers and check based on resulting
	 * oversampling ratio that CIC and FIR decimation ratios are
	 * feasible. The ratios need to be integers. Also the mic clock
	 * duty cycle need to be within limits.
	 */
	for (clkdiv = clkdiv_min; clkdiv <= clkdiv_max; clkdiv++) {
		/* Calculate duty cycle for this clock divider. Note that
		 * odd dividers cause non-50% duty cycle.
		 */
		c1 = clkdiv >> 1;
		du_min = 100 * c1 / clkdiv;
		du_max = 100 - du_min;

		/* Calculate PDM clock rate and oversampling ratio. */
		pdmclk = DMIC_HW_IOCLK / clkdiv;
		osr = pdmclk / fs;

		/* Check that OSR constraints is met and clock duty cycle does
		 * not exceed microphone specification. If exceed proceed to
		 * next clkdiv.
		 */
		if ((osr < osr_min) || (du_min < config->io.min_pdm_clk_dc) ||
			(du_max > config->io.max_pdm_clk_dc)) {
			continue;
		}

		/* Loop FIR decimation factors candidates. If the
		 * integer divided decimation factors and clock dividers
		 * as multiplied with sample rate match the IO clock
		 * rate the division was exact and such decimation mode
		 * is possible. Then check that CIC decimation constraints
		 * are met. The passed decimation modes are added to array.
		 */
		for (mfir = DMIC_HW_FIR_DECIM_MIN;
			mfir <= DMIC_HW_FIR_DECIM_MAX; mfir++) {
			mcic = osr / mfir;
			ioclk_test = fs * mfir * mcic * clkdiv;

			if (ioclk_test == DMIC_HW_IOCLK &&
				mcic >= DMIC_HW_CIC_DECIM_MIN &&
				mcic <= DMIC_HW_CIC_DECIM_MAX &&
				i < DMIC_MAX_MODES) {
				modes->clkdiv[i] = clkdiv;
				modes->mcic[i] = mcic;
				modes->mfir[i] = mfir;
				i++;
				modes->num_of_modes = i;
			}
		}
	}
}

/* The previous raw modes list contains sane configuration possibilities. When
 * there is request for both FIFOs A and B operation this function returns
 * list of compatible settings.
 */
static void match_modes(struct matched_modes *c, struct decim_modes *a,
		struct decim_modes *b)
{
	int16_t idx[DMIC_MAX_MODES];
	int idx_length;
	int i;
	int n;
	int m;

	/* Check if previous search got results. */
	c->num_of_modes = 0;
	if (a->num_of_modes == 0 && b->num_of_modes == 0) {
		/* Nothing to do */
		return;
	}

	/* Check for request only for FIFO A or B. In such case pass list for
	 * A or B as such.
	 */
	if (b->num_of_modes == 0) {
		c->num_of_modes = a->num_of_modes;
		for (i = 0; i < a->num_of_modes; i++) {
			c->clkdiv[i] = a->clkdiv[i];
			c->mcic[i] = a->mcic[i];
			c->mfir_a[i] = a->mfir[i];
			c->mfir_b[i] = 0; /* Mark FIR B as non-used */
		}
		return;
	}

	if (a->num_of_modes == 0) {
		c->num_of_modes = b->num_of_modes;
		for (i = 0; i < b->num_of_modes; i++) {
			c->clkdiv[i] = b->clkdiv[i];
			c->mcic[i] = b->mcic[i];
			c->mfir_b[i] = b->mfir[i];
			c->mfir_a[i] = 0; /* Mark FIR A as non-used */
		}
		return;
	}

	/* Merge a list of compatible modes */
	i = 0;
	for (n = 0; n < a->num_of_modes; n++) {
		/* Find all indices of values a->clkdiv[n] in b->clkdiv[] */
		idx_length = find_equal_int16(idx, b->clkdiv, a->clkdiv[n],
			b->num_of_modes, 0);
		for (m = 0; m < idx_length; m++) {
			if (b->mcic[idx[m]] == a->mcic[n]) {
				c->clkdiv[i] = a->clkdiv[n];
				c->mcic[i] = a->mcic[n];
				c->mfir_a[i] = a->mfir[n];
				c->mfir_b[i] = b->mfir[idx[m]];
				i++;
			}
		}
		c->num_of_modes = i;
	}
}

/* Finds a suitable FIR decimation filter from the included set */
static struct pdm_decim *get_fir(struct dmic_configuration *cfg, int mfir)
{
	int i;
	int fs;
	int cic_fs;
	int fir_max_length;
	struct pdm_decim *fir = NULL;
	struct pdm_decim **fir_list;

	if (mfir <= 0) {
		return fir;
	}

	cic_fs = DMIC_HW_IOCLK / cfg->clkdiv / cfg->mcic;
	fs = cic_fs / mfir;
	/* FIR max. length depends on available cycles and coef RAM
	 * length. Exceeding this length sets HW overrun status and
	 * overwrite of other register.
	 */
	fir_max_length = (DMIC_HW_IOCLK / fs / 2) - DMIC_FIR_PIPELINE_OVERHEAD;
	fir_max_length = MIN(fir_max_length, DMIC_HW_FIR_LENGTH_MAX);

	fir_list = pdm_decim_get_fir_list();

	for (i = 0; i < DMIC_FIR_LIST_LENGTH; i++) {
		if (fir_list[i]->decim_factor == mfir &&
			fir_list[i]->length <= fir_max_length) {
			/* Store pointer, break from loop to avoid a
			 * Possible other mode with lower FIR length.
			 */
			fir = fir_list[i];
			break;
		}
	}

	return fir;
}

/* Calculate scale and shift to use for FIR coefficients. Scale is applied
 * before write to HW coef RAM. Shift will be programmed to HW register.
 */
static int fir_coef_scale(int32_t *fir_scale, int *fir_shift, int add_shift,
	const int32_t coef[], int coef_length, int32_t gain)
{
	int32_t amax;
	int32_t new_amax;
	int32_t fir_gain;
	int shift;

	/* Multiply gain passed from CIC with output full scale. */
	fir_gain = Q_MULTSR_32X32((int64_t)gain, DMIC_HW_SENS_Q28,
		DMIC_FIR_SCALE_Q, 28, DMIC_FIR_SCALE_Q);

	/* Find the largest FIR coefficient value. */
	amax = find_max_abs_int32((int32_t *)coef, coef_length);

	/* Scale max. tap value with FIR gain. */
	new_amax = Q_MULTSR_32X32((int64_t)amax, fir_gain, 31,
		DMIC_FIR_SCALE_Q, DMIC_FIR_SCALE_Q);
	if (new_amax <= 0) {
		return -EINVAL;
	}

	/* Get left shifts count to normalize the fractional value as 32 bit.
	 * We need right shifts count for scaling so need to invert. The
	 * difference of Q31 vs. used Q format is added to get the correct
	 * normalization right shift value.
	 */
	shift = 31 - DMIC_FIR_SCALE_Q - norm_int32(new_amax);

	/* Add to shift for coef raw Q31 format shift and store to
	 * configuration. Ensure range (fail should not happen with OK
	 * coefficient set).
	 */
	*fir_shift = -shift + add_shift;
	if (*fir_shift < DMIC_HW_FIR_SHIFT_MIN ||
		*fir_shift > DMIC_HW_FIR_SHIFT_MAX) {
		return -EINVAL;
	}

	/* Compensate shift into FIR coef scaler and store as Q4.20. */
	if (shift < 0) {
		*fir_scale = (fir_gain << -shift);
	} else {
		*fir_scale = (fir_gain >> shift);
	}

	return 0;
}

/* This function selects with a simple criteria one mode to set up the
 * decimator. For the settings chosen for FIFOs A and B output a lookup
 * is done for FIR coefficients from the included coefficients tables.
 * For some decimation factors there may be several length coefficient sets.
 * It is due to possible restriction of decimation engine cycles per given
 * sample rate. If the coefficients length is exceeded the lookup continues.
 * Therefore the list of coefficient set must present the filters for a
 * decimation factor in decreasing length order.
 *
 * Note: If there is no filter available an error is returned. The parameters
 * should be reviewed for such case. If still a filter is missing it should be
 * added into the included set. FIR decimation with a high factor usually
 * needs compromises into specifications and is not desirable.
 */
static int select_mode(struct dmic_configuration *cfg,
	struct matched_modes *modes)
{
	int32_t g_cic;
	int32_t fir_in_max;
	int32_t cic_out_max;
	int32_t gain_to_fir;
	int16_t idx[DMIC_MAX_MODES];
	int16_t *mfir;
	int n = 1;
	int mmin;
	int count;
	int mcic;
	int bits_cic;
	int ret;

	/* If there are more than one possibilities select a mode with lowest
	 * FIR decimation factor. If there are several select mode with highest
	 * ioclk divider to minimize microphone power consumption. The highest
	 * clock divisors are in the end of list so select the last of list.
	 * The minimum OSR criteria used in previous ensures that quality in
	 * the candidates should be sufficient.
	 */
	if (modes->num_of_modes == 0) {
		LOG_ERR("num_of_modes is 0");
		return -EINVAL;
	}

	/* Valid modes presence is indicated with non-zero decimation
	 * factor in 1st element. If FIR A is not used get decimation factors
	 * from FIR B instead.
	 */
	if (modes->mfir_a[0] > 0) {
		mfir = modes->mfir_a;
	} else {
		mfir = modes->mfir_b;
	}

	mmin = find_min_int16(mfir, modes->num_of_modes);
	count = find_equal_int16(idx, mfir, mmin, modes->num_of_modes, 0);
	n = idx[count - 1];

	/* Get microphone clock and decimation parameters for used mode from
	 * the list.
	 */
	cfg->clkdiv = modes->clkdiv[n];
	cfg->mfir_a = modes->mfir_a[n];
	cfg->mfir_b = modes->mfir_b[n];
	cfg->mcic = modes->mcic[n];
	cfg->fir_a = NULL;
	cfg->fir_b = NULL;

	/* Find raw FIR coefficients to match the decimation factors of FIR
	 * A and B.
	 */
	if (cfg->mfir_a > 0) {
		cfg->fir_a = get_fir(cfg, cfg->mfir_a);
		if (!cfg->fir_a) {
			LOG_ERR("FIR filter not found for mfir_a %d",
					cfg->mfir_a);
			return -EINVAL;
		}
	}

	if (cfg->mfir_b > 0) {
		cfg->fir_b = get_fir(cfg, cfg->mfir_b);
		if (!cfg->fir_b) {
			LOG_ERR("FIR filter not found for mfir_b %d",
					cfg->mfir_b);
			return -EINVAL;
		}
	}

	/* Calculate CIC shift from the decimation factor specific gain. The
	 * gain of HW decimator equals decimation factor to power of 5.
	 */
	mcic = cfg->mcic;
	g_cic = mcic * mcic * mcic * mcic * mcic;
	if (g_cic < 0) {
		/* Erroneous decimation factor and CIC gain */
		LOG_ERR("Invalid CIC gain %d", g_cic);
		return -EINVAL;
	}

	bits_cic = 32 - norm_int32(g_cic);
	cfg->cic_shift = bits_cic - DMIC_HW_BITS_FIR_INPUT;

	/* Calculate remaining gain to FIR in Q format used for gain
	 * values.
	 */
	fir_in_max = (1 << (DMIC_HW_BITS_FIR_INPUT - 1));
	if (cfg->cic_shift >= 0) {
		cic_out_max = g_cic >> cfg->cic_shift;
	} else {
		cic_out_max = g_cic << -cfg->cic_shift;
	}

	gain_to_fir = (int32_t)((((int64_t)fir_in_max) << DMIC_FIR_SCALE_Q) /
		cic_out_max);

	/* Calculate FIR scale and shift */
	if (cfg->mfir_a > 0) {
		cfg->fir_a_length = cfg->fir_a->length;
		ret = fir_coef_scale(&cfg->fir_a_scale, &cfg->fir_a_shift,
			cfg->fir_a->shift, cfg->fir_a->coef, cfg->fir_a->length,
			gain_to_fir);
		if (ret < 0) {
			/* Invalid coefficient set found, should not happen. */
			LOG_ERR("Invalid coefficient A");
			return -EINVAL;
		}
	} else {
		cfg->fir_a_scale = 0;
		cfg->fir_a_shift = 0;
		cfg->fir_a_length = 0;
	}

	if (cfg->mfir_b > 0) {
		cfg->fir_b_length = cfg->fir_b->length;
		ret = fir_coef_scale(&cfg->fir_b_scale, &cfg->fir_b_shift,
			cfg->fir_b->shift, cfg->fir_b->coef, cfg->fir_b->length,
			gain_to_fir);
		if (ret < 0) {
			/* Invalid coefficient set found, should not happen. */
			LOG_ERR("Invalid coefficient B");
			return -EINVAL;
		}
	} else {
		cfg->fir_b_scale = 0;
		cfg->fir_b_shift = 0;
		cfg->fir_b_length = 0;
	}

	return 0;
}

static int source_ipm_helper(struct pdm_chan_cfg *config, uint32_t *source_mask,
		uint8_t *controller_mask, uint8_t *stereo_mask, uint8_t *swap_mask)
{
	uint8_t pdm_ix;
	uint8_t chan_ix;
	enum pdm_lr lr;
	uint16_t pdm_lr_mask = 0U;
	int ipm = 0;

	/* clear outputs */
	*source_mask = 0U;
	*stereo_mask = 0U;
	*swap_mask = 0U;
	*controller_mask = 0U;

	/* Loop number of PDM controllers in the configuration. If mic A
	 * or B is enabled then a pdm controller is marked as active. Also it
	 * is checked whether the controller should operate as stereo or mono
	 * left (A) or mono right (B) mode. Mono right mode is setup as channel
	 * swapped mono left. The function returns also in array source[] the
	 * indices of enabled pdm controllers to be used for IPM configuration.
	 */
	for (chan_ix = 0U; chan_ix < config->req_num_chan; chan_ix++) {

		dmic_parse_channel_map(config->req_chan_map_lo,
				config->req_chan_map_hi,
				chan_ix, &pdm_ix, &lr);

		if (pdm_ix >= DMIC_HW_CONTROLLERS) {
			LOG_ERR("Invalid PDM controller %u in channel %u",
					pdm_ix, chan_ix);
			continue;
		}

		if ((*controller_mask & BIT(pdm_ix)) == 0U) {
			*controller_mask |= BIT(pdm_ix);
			*source_mask |= pdm_ix << (ipm << 2);
			ipm++;
		}
		pdm_lr_mask |= BIT(lr) << (pdm_ix << 1);
		/*
		 * if both L and R are requested,
		 * set the controller to be stereo
		 */
		if ((pdm_lr_mask >> (pdm_ix << 1)) &
				(BIT(PDM_CHAN_LEFT) | BIT(PDM_CHAN_RIGHT))) {
			*stereo_mask |= BIT(pdm_ix);
		}

		/*
		 * if R channel mic was requested first
		 * set the controller to swap the channels
		 */
		if ((pdm_lr_mask & BIT(PDM_CHAN_LEFT + (pdm_ix << 1))) == 0U) {
			*swap_mask |= BIT(pdm_ix);
		}
	}

	/* IPM bit field is set to count of active pdm controllers. */
	LOG_DBG("%u decimators has to be configured", ipm);
	return ipm;
}

static int configure_registers(const struct device *dev,
			       struct dmic_configuration *hw_cfg,
			       struct dmic_cfg *config)
{
	uint8_t skew;
	uint8_t swap_mask;
	uint8_t edge_mask;
	uint8_t stereo_mask;
	uint8_t controller_mask;
	uint32_t val;
	int32_t ci;
	uint32_t cu;
	uint32_t coeff_ix;
	int ipm;
	int of0;
	int of1;
	int fir_decim;
	int fir_length;
	int length;
	int dccomp;
	int cic_start_a;
	int cic_start_b;
	int fir_start_a;
	int fir_start_b;
	int soft_reset;
	int i;
	int j;

	int array_a = 0;
	int array_b = 0;
	int cic_mute = 0;
	int fir_mute = 0;

	/* Normal start sequence */
	dccomp = 1;
	soft_reset = 1;
	cic_start_a = 0;
	cic_start_b = 0;
	fir_start_a = 0;
	fir_start_b = 0;

	uint32_t source_mask;

	/* OUTCONTROL0 and OUTCONTROL1 */
	of0 = (config->streams[0].pcm_width == 32U) ? 2 : 0;
	if (config->channel.req_num_streams > 1) {
		of1 = (config->streams[1].pcm_width == 32U) ? 2 : 0;
	} else {
		of1 = 0;
	}

	ipm = source_ipm_helper(&config->channel, &source_mask,
			&controller_mask, &stereo_mask, &swap_mask);
	val = OUTCONTROL0_TIE(0) |
		OUTCONTROL0_SIP(0) |
		OUTCONTROL0_FINIT(1) |
		OUTCONTROL0_FCI(0) |
		OUTCONTROL0_BFTH(3) |
		OUTCONTROL0_OF(of0) |
		OUTCONTROL0_NUMBER_OF_DECIMATORS(ipm) |
		OUTCONTROL0_IPM_SOURCE_1(source_mask) |
		OUTCONTROL0_IPM_SOURCE_2(source_mask >> 4) |
		OUTCONTROL0_IPM_SOURCE_3(source_mask >> 8) |
		OUTCONTROL0_IPM_SOURCE_4(source_mask >> 12) |
		OUTCONTROL0_TH(3);
	DMIC_REG_WR(OUTCONTROL0, val);
	LOG_DBG("WR: OUTCONTROL0: 0x%08X", val);

	val = OUTCONTROL1_TIE(0) |
		OUTCONTROL1_SIP(0) |
		OUTCONTROL1_FINIT(1) |
		OUTCONTROL1_FCI(0) |
		OUTCONTROL1_BFTH(3) |
		OUTCONTROL1_OF(of1) |
		OUTCONTROL1_NUMBER_OF_DECIMATORS(ipm) |
		OUTCONTROL1_IPM_SOURCE_1(source_mask) |
		OUTCONTROL1_IPM_SOURCE_2(source_mask >> 4) |
		OUTCONTROL1_IPM_SOURCE_3(source_mask >> 8) |
		OUTCONTROL1_IPM_SOURCE_4(source_mask >> 12) |
		OUTCONTROL1_TH(3);
	DMIC_REG_WR(OUTCONTROL1, val);
	LOG_DBG("WR: OUTCONTROL1: 0x%08X", val);

	/* Mark enabled microphones into private data to be later used
	 * for starting correct parts of the HW.
	 */
	for (i = 0; i < DMIC_HW_CONTROLLERS; i++) {
		if ((controller_mask & BIT(i)) == 0U) {
			/* controller is not enabled */
			continue;
		}

		if (stereo_mask & BIT(i)) {
			dmic_private.mic_en_mask |= (BIT(PDM_CHAN_LEFT) |
				BIT(PDM_CHAN_RIGHT)) << (i << 1);
		} else {
			dmic_private.mic_en_mask |=
				((swap_mask & BIT(i)) == 0U) ?
				BIT(PDM_CHAN_LEFT) << (i << 1) :
				BIT(PDM_CHAN_RIGHT) << (i << 1);
		}
	}

	/*
	 * Mono right channel mic usage requires swap of PDM channels
	 * since the mono decimation is done with only left channel
	 * processing active.
	 */
	edge_mask = config->io.pdm_clk_pol ^ swap_mask;

	for (i = 0; i < DMIC_HW_CONTROLLERS; i++) {
		/* CIC */
		val = CIC_CONTROL_SOFT_RESET(soft_reset) |
			CIC_CONTROL_CIC_START_B(cic_start_b) |
			CIC_CONTROL_CIC_START_A(cic_start_a) |
		CIC_CONTROL_MIC_B_POLARITY(config->io.pdm_data_pol >> i) |
		CIC_CONTROL_MIC_A_POLARITY(config->io.pdm_data_pol >> i) |
			CIC_CONTROL_MIC_MUTE(cic_mute) |
			CIC_CONTROL_STEREO_MODE(stereo_mask >> i);
		DMIC_REG_WR(CIC_CONTROL(i), val);
		LOG_DBG("WR: CIC_CONTROL[%u]: 0x%08X", i, val);

		val = CIC_CONFIG_CIC_SHIFT(hw_cfg->cic_shift + 8) |
			CIC_CONFIG_COMB_COUNT(hw_cfg->mcic - 1);
		DMIC_REG_WR(CIC_CONFIG(i), val);
		LOG_DBG("WR: CIC_CONFIG[%u]: 0x%08X", i, val);

		skew = dmic_parse_clk_skew_map(config->io.pdm_clk_skew, i);
		val = MIC_CONTROL_PDM_CLKDIV(hw_cfg->clkdiv - 2) |
			MIC_CONTROL_PDM_SKEW(skew) |
			MIC_CONTROL_CLK_EDGE(edge_mask >> i) |
			MIC_CONTROL_PDM_EN_B(cic_start_b) |
			MIC_CONTROL_PDM_EN_A(cic_start_a);
		DMIC_REG_WR(MIC_CONTROL(i), val);
		LOG_DBG("WR: MIC_CONTROL[%u]: 0x%08X", i, val);

		/* FIR A */
		fir_decim = MAX(hw_cfg->mfir_a - 1, 0);
		fir_length = MAX(hw_cfg->fir_a_length - 1, 0);
		val = FIR_CONTROL_A_START(fir_start_a) |
			FIR_CONTROL_A_ARRAY_START_EN(array_a) |
			FIR_CONTROL_A_DCCOMP(dccomp) |
			FIR_CONTROL_A_MUTE(fir_mute) |
			FIR_CONTROL_A_STEREO(stereo_mask >> i);
		DMIC_REG_WR(FIR_CONTROL_A(i), val);
		LOG_DBG("WR: FIR_CONTROL_A[%u]: 0x%08X", i, val);

		val = FIR_CONFIG_A_FIR_DECIMATION(fir_decim) |
			FIR_CONFIG_A_FIR_SHIFT(hw_cfg->fir_a_shift) |
			FIR_CONFIG_A_FIR_LENGTH(fir_length);
		DMIC_REG_WR(FIR_CONFIG_A(i), val);
		LOG_DBG("WR: FIR_CONFIG_A[%u]: 0x%08X", i, val);

		val = DC_OFFSET_LEFT_A_DC_OFFS(DCCOMP_TC0);
		DMIC_REG_WR(DC_OFFSET_LEFT_A(i), val);
		LOG_DBG("WR: DC_OFFSET_LEFT_A[%u]: 0x%08X", i, val);

		val = DC_OFFSET_RIGHT_A_DC_OFFS(DCCOMP_TC0);
		DMIC_REG_WR(DC_OFFSET_RIGHT_A(i), val);
		LOG_DBG("WR: DC_OFFSET_RIGHT_A[%u]: 0x%08X", i, val);

		val = OUT_GAIN_LEFT_A_GAIN(0);
		DMIC_REG_WR(OUT_GAIN_LEFT_A(i), val);
		LOG_DBG("WR: OUT_GAIN_LEFT_A[%u]: 0x%08X", i, val);

		val = OUT_GAIN_RIGHT_A_GAIN(0);
		DMIC_REG_WR(OUT_GAIN_RIGHT_A(i), val);
		LOG_DBG("WR: OUT_GAIN_RIGHT_A[%u]: 0x%08X", i, val);

		/* FIR B */
		fir_decim = MAX(hw_cfg->mfir_b - 1, 0);
		fir_length = MAX(hw_cfg->fir_b_length - 1, 0);
		val = FIR_CONTROL_B_START(fir_start_b) |
			FIR_CONTROL_B_ARRAY_START_EN(array_b) |
			FIR_CONTROL_B_DCCOMP(dccomp) |
			FIR_CONTROL_B_MUTE(fir_mute) |
			FIR_CONTROL_B_STEREO(stereo_mask >> i);
		DMIC_REG_WR(FIR_CONTROL_B(i), val);
		LOG_DBG("WR: FIR_CONTROL_B[%u]: 0x%08X", i, val);

		val = FIR_CONFIG_B_FIR_DECIMATION(fir_decim) |
			FIR_CONFIG_B_FIR_SHIFT(hw_cfg->fir_b_shift) |
			FIR_CONFIG_B_FIR_LENGTH(fir_length);
		DMIC_REG_WR(FIR_CONFIG_B(i), val);
		LOG_DBG("WR: FIR_CONFIG_B[%u]: 0x%08X", i, val);

		val = DC_OFFSET_LEFT_B_DC_OFFS(DCCOMP_TC0);
		DMIC_REG_WR(DC_OFFSET_LEFT_B(i), val);
		LOG_DBG("WR: DC_OFFSET_LEFT_B[%u]: 0x%08X", i, val);

		val = DC_OFFSET_RIGHT_B_DC_OFFS(DCCOMP_TC0);
		DMIC_REG_WR(DC_OFFSET_RIGHT_B(i), val);
		LOG_DBG("WR: DC_OFFSET_RIGHT_B[%u]: 0x%08X", i, val);

		val = OUT_GAIN_LEFT_B_GAIN(0);
		DMIC_REG_WR(OUT_GAIN_LEFT_B(i), val);
		LOG_DBG("WR: OUT_GAIN_LEFT_B[%u]: 0x%08X", i, val);

		val = OUT_GAIN_RIGHT_B_GAIN(0);
		DMIC_REG_WR(OUT_GAIN_RIGHT_B(i), val);
		LOG_DBG("WR: OUT_GAIN_RIGHT_B[%u]: 0x%08X", i, val);
	}

	/* Write coef RAM A with scaled coefficient in reverse order */
	length = hw_cfg->fir_a_length;
	for (j = 0; j < length; j++) {
		ci = (int32_t)Q_MULTSR_32X32((int64_t)hw_cfg->fir_a->coef[j],
				hw_cfg->fir_a_scale, 31, DMIC_FIR_SCALE_Q,
				DMIC_HW_FIR_COEF_Q);
		cu = FIR_COEF_A(ci);
		coeff_ix = (length - j - 1) << 2;
		for (i = 0; i < DMIC_HW_CONTROLLERS; i++) {
			DMIC_REG_WR(PDM_COEFF_A(i) + coeff_ix, cu);
		}
	}

	/* Write coef RAM B with scaled coefficient in reverse order */
	length = hw_cfg->fir_b_length;
	for (j = 0; j < length; j++) {
		ci = (int32_t)Q_MULTSR_32X32((int64_t)hw_cfg->fir_b->coef[j],
				hw_cfg->fir_b_scale, 31, DMIC_FIR_SCALE_Q,
				DMIC_HW_FIR_COEF_Q);
		cu = FIR_COEF_B(ci);
		coeff_ix = (length - j - 1) << 2;
		for (i = 0; i < DMIC_HW_CONTROLLERS; i++) {
			DMIC_REG_WR(PDM_COEFF_B(i) + coeff_ix, cu);
		}
	}

	/* Function dmic_start() uses these to start the used FIFOs */
	dmic_private.fifo_a = (hw_cfg->mfir_a > 0) ? 1 : 0;
	dmic_private.fifo_b = (hw_cfg->mfir_b > 0) ? 1 : 0;

	return 0;
}

static void dmic_dma_callback(const struct device *dev, void *arg,
			      uint32_t chan, int err_code)
{
	void *buffer;
	size_t size;
	int stream;
	struct _stream_data *stream_data;
	int ret;

	stream = (chan == DMA_CHANNEL_DMIC_RXA) ? 0 : 1;
	stream_data = &dmic_private.streams[stream];

	/* retrieve buffer from input queue */
	ret = k_msgq_get(&stream_data->in_queue, &buffer, K_NO_WAIT);

	if (ret) {
		LOG_ERR("stream %u in_queue is empty", stream);
	}

	if (dmic_private.state == DMIC_STATE_ACTIVE) {
		size = stream_data->block_size;
		/* put buffer in output queue */
		ret = k_msgq_put(&stream_data->out_queue, &buffer,
				K_NO_WAIT);
		if (ret) {
			LOG_ERR("stream%u out_queue is full", stream);
		}

		/* allocate new buffer for next audio frame */
		ret = k_mem_slab_alloc(stream_data->mem_slab, &buffer,
				K_NO_WAIT);
		if (ret) {
			LOG_ERR("buffer alloc from slab %p err %d",
					stream_data->mem_slab, ret);
		} else {
			/* put buffer in input queue */
			ret = k_msgq_put(&stream_data->in_queue, &buffer,
					K_NO_WAIT);
			if (ret) {
				LOG_ERR("buffer %p -> in_queue %p err %d",
						buffer, &stream_data->in_queue,
						ret);
			}

			/* reload the DMA */
			dmic_reload_dma(chan, buffer, stream_data->block_size);
			dmic_start_dma(chan);
		}
	} else {
		/* stop activity, free buffers */
		dmic_stop();
		dmic_stop_dma(chan);
		k_mem_slab_free(stream_data->mem_slab, &buffer);
	}
}

static int dmic_set_config(const struct device *dev, struct dmic_cfg *config)
{
	struct decim_modes modes_a;
	struct decim_modes modes_b;
	struct matched_modes modes_ab;
	struct dmic_configuration hw_cfg;
	int ret;
	int stream;

	LOG_DBG("min_pdm_clk_freq %u max_pdm_clk_freq %u",
			config->io.min_pdm_clk_freq,
			config->io.max_pdm_clk_freq);
	LOG_DBG("min_pdm_clk_dc %u max_pdm_clk_dc %u",
			config->io.min_pdm_clk_dc,
			config->io.max_pdm_clk_dc);
	LOG_DBG("num_chan %u", config->channel.req_num_chan);
	LOG_DBG("req_num_streams %u", config->channel.req_num_streams);

	if (config->channel.req_num_streams == 0U) {
		LOG_ERR("req_num_streams is 0");
		return -EINVAL;
	}

	config->channel.act_num_streams = MIN(config->channel.req_num_streams,
			DMIC_MAX_STREAMS);

	LOG_DBG("req_num_streams %u act_num_streams %u",
			config->channel.req_num_streams,
			config->channel.act_num_streams);
	dmic_private.num_streams = config->channel.act_num_streams;

	for (stream = 0; stream < dmic_private.num_streams; stream++) {
		LOG_DBG("stream %u pcm_rate %u pcm_width %u", stream,
				config->streams[stream].pcm_rate,
				config->streams[stream].pcm_width);

		if ((config->streams[stream].pcm_width) &&
				(config->streams[stream].mem_slab == NULL)) {
			LOG_ERR("Invalid mem_slab for stream %u", stream);
			return -EINVAL;
		}

		dmic_private.streams[stream].mem_slab =
			config->streams[stream].mem_slab;
		dmic_private.streams[stream].block_size =
			config->streams[stream].block_size;
	}

	/* Match and select optimal decimators configuration for FIFOs A and B
	 * paths. This setup phase is still abstract. Successful completion
	 * points struct cfg to FIR coefficients and contains the scale value
	 * to use for FIR coefficient RAM write as well as the CIC and FIR
	 * shift values.
	 */
	find_modes(&modes_a, config, config->streams[0].pcm_rate);
	if ((modes_a.num_of_modes == 0) && (config->streams[0].pcm_rate > 0)) {
		LOG_ERR("stream A num_of_modes is 0 and pcm_rate is %u",
				config->streams[0].pcm_rate);
		return -EINVAL;
	}

	if (dmic_private.num_streams > 1) {
		find_modes(&modes_b, config, config->streams[1].pcm_rate);
		if ((modes_b.num_of_modes == 0) &&
				(config->streams[1].pcm_rate > 0)) {
			LOG_ERR("stream B num_of_modes = 0 & pcm_rate = %u",
					config->streams[1].pcm_rate);
			return -EINVAL;
		}
	} else {
		modes_b.num_of_modes = 0;
	}

	match_modes(&modes_ab, &modes_a, &modes_b);
	ret = select_mode(&hw_cfg, &modes_ab);
	if (ret < 0) {
		LOG_ERR("select_mode failed");
		return -EINVAL;
	}

	LOG_DBG("clkdiv %u mcic %u", hw_cfg.clkdiv, hw_cfg.mcic);
	LOG_DBG("mfir_a %d mfir_b %d", hw_cfg.mfir_a, hw_cfg.mfir_b);
	LOG_DBG("fir_a_length %d fir_b_length %d", hw_cfg.fir_a_length,
			hw_cfg.fir_b_length);
	LOG_DBG("cic_shift %d fir_a_shift %d fir_b_shift %d", hw_cfg.cic_shift,
			hw_cfg.fir_a_shift, hw_cfg.fir_b_shift);

	/* Struct reg contains a mirror of actual HW registers. Determine
	 * register bits configuration from decimator configuration and the
	 * requested parameters.
	 */
	ret = configure_registers(dev, &hw_cfg, config);
	if (ret < 0) {
		LOG_ERR("configure_registers failed RC: %d", ret);
		return -EINVAL;
	}

	dmic_private.state = DMIC_STATE_CONFIGURED;

	return 0;
}

/* start the DMIC for capture */
static void dmic_start(const struct device *dev)
{
	struct _stream_data *stream;
	unsigned int key;
	int i;
	int mic_a;
	int mic_b;
	int fir_a;
	int fir_b;
	void *buffer;
	int ret;

	for (i = 0; i < dmic_private.num_streams; i++) {
		stream = &dmic_private.streams[i];
		/* allocate buffer for stream A */
		ret = k_mem_slab_alloc(stream->mem_slab, &buffer, K_NO_WAIT);
		if (ret) {
			LOG_ERR("alloc from mem_slab_a %p failed",
					stream->mem_slab);
			return;
		}
		/* load buffer to DMA */
		dmic_reload_dma((i == 0) ? DMA_CHANNEL_DMIC_RXA :
				DMA_CHANNEL_DMIC_RXB,
				buffer, stream->block_size);
		ret = k_msgq_put(&stream->in_queue, &buffer, K_NO_WAIT);
		if (ret) {
			LOG_ERR("stream %u in_queue full", i);
			k_mem_slab_free(stream->mem_slab, &buffer);
			return;
		}
	}

	/* enable port */
	key = irq_lock();

	for (i = 0; i < DMIC_HW_CONTROLLERS; i++) {
		mic_a = dmic_private.mic_en_mask >> (PDM_CHAN_LEFT + (i << 1));
		mic_a &= BIT(0);

		mic_b = dmic_private.mic_en_mask >> (PDM_CHAN_RIGHT + (i << 1));
		mic_b &= BIT(0);

		if ((dmic_private.mic_en_mask >> (i << 1)) &
				(BIT(PDM_CHAN_LEFT) | BIT(PDM_CHAN_RIGHT))) {
			fir_a = (dmic_private.fifo_a) ? 1 : 0;
			fir_b = (dmic_private.fifo_b) ? 1 : 0;
		} else {
			fir_a = fir_b = 0;
		}

		LOG_DBG("mic_a %d mic_b %d", mic_a, mic_b);
		LOG_DBG("fir_a %d fir_b %d", fir_a, fir_b);

		DMIC_REG_UPD(CIC_CONTROL(i),
			CIC_CONTROL_CIC_START_A_BIT |
			CIC_CONTROL_CIC_START_B_BIT,
			CIC_CONTROL_CIC_START_A(mic_a) |
			CIC_CONTROL_CIC_START_B(mic_b));
		DMIC_REG_UPD(MIC_CONTROL(i),
			MIC_CONTROL_PDM_EN_A_BIT |
			MIC_CONTROL_PDM_EN_B_BIT,
			MIC_CONTROL_PDM_EN_A(mic_a) |
			MIC_CONTROL_PDM_EN_B(mic_b));

		DMIC_REG_UPD(FIR_CONTROL_A(i),
			FIR_CONTROL_A_START_BIT, FIR_CONTROL_A_START(fir_a));
		DMIC_REG_UPD(FIR_CONTROL_B(i),
			FIR_CONTROL_B_START_BIT, FIR_CONTROL_B_START(fir_b));
		LOG_DBG("CIC_CONTROL[%u]: %08X", i,
				DMIC_REG_RD(CIC_CONTROL(i)));
		LOG_DBG("MIC_CONTROL[%u]: %08X", i,
				DMIC_REG_RD(MIC_CONTROL(i)));
		LOG_DBG("FIR_CONTROL_A[%u]: %08X", i,
				DMIC_REG_RD(FIR_CONTROL_A(i)));
		LOG_DBG("FIR_CONTROL_B[%u]: %08X", i,
				DMIC_REG_RD(FIR_CONTROL_B(i)));
	}

	/* start the DMA channel(s) */
	if (dmic_private.fifo_a) {
		dmic_start_dma(DMA_CHANNEL_DMIC_RXA);
	}

	if (dmic_private.fifo_b) {
		dmic_start_dma(DMA_CHANNEL_DMIC_RXB);
	}

	if (dmic_private.fifo_a) {
		/*  Clear FIFO A initialize, Enable interrupts to DSP,
		 *  Start FIFO A packer.
		 */
		DMIC_REG_UPD(OUTCONTROL0,
			OUTCONTROL0_FINIT_BIT | OUTCONTROL0_SIP_BIT,
			OUTCONTROL0_SIP_BIT);
	}
	if (dmic_private.fifo_b) {
		/*  Clear FIFO B initialize, Enable interrupts to DSP,
		 *  Start FIFO B packer.
		 */
		DMIC_REG_UPD(OUTCONTROL1,
			OUTCONTROL1_FINIT_BIT | OUTCONTROL1_SIP_BIT,
			OUTCONTROL1_SIP_BIT);
	}

	LOG_DBG("OUTCONTROL0: %08X", DMIC_REG_RD(OUTCONTROL0));
	LOG_DBG("OUTCONTROL1: %08X", DMIC_REG_RD(OUTCONTROL1));

	/* Clear soft reset for all/used PDM controllers. This should
	 * start capture in sync.
	 */
	LOG_DBG("Releasing soft reset for all PDM controllers");
	for (i = 0; i < DMIC_HW_CONTROLLERS; i++) {
		DMIC_REG_UPD(CIC_CONTROL(i), CIC_CONTROL_SOFT_RESET_BIT, 0);
	}

	dmic_private.state = DMIC_STATE_ACTIVE;
	irq_unlock(key);

	LOG_DBG("State changed to DMIC_STATE_ACTIVE");

	/* Currently there's no DMIC HW internal mutings and wait times
	 * applied into this start sequence. It can be implemented here if
	 * start of audio capture would contain clicks and/or noise and it
	 * is not suppressed by gain ramp somewhere in the capture pipe.
	 */
}

/* stop the DMIC for capture */
static void dmic_stop(void)
{
	int i;

	/* Stop FIFO packers and set FIFO initialize bits */
	DMIC_REG_UPD(OUTCONTROL0,
		OUTCONTROL0_SIP_BIT | OUTCONTROL0_FINIT_BIT,
		OUTCONTROL0_FINIT_BIT);
	DMIC_REG_UPD(OUTCONTROL1,
		OUTCONTROL1_SIP_BIT | OUTCONTROL1_FINIT_BIT,
		OUTCONTROL1_FINIT_BIT);

	/* Set soft reset for all PDM controllers.
	 */
	LOG_DBG("Soft reset all PDM controllers");
	for (i = 0; i < DMIC_HW_CONTROLLERS; i++) {
		DMIC_REG_UPD(CIC_CONTROL(i),
			CIC_CONTROL_SOFT_RESET_BIT, CIC_CONTROL_SOFT_RESET_BIT);
	}
}

static int dmic_trigger_device(const struct device *dev,
			       enum dmic_trigger cmd)
{
	unsigned int key;

	LOG_DBG("cmd: %d", cmd);

	switch (cmd) {
	case DMIC_TRIGGER_RELEASE:
	case DMIC_TRIGGER_START:
		if ((dmic_private.state == DMIC_STATE_CONFIGURED) ||
				(dmic_private.state == DMIC_STATE_PAUSED)) {
			dmic_start(dev);
		} else {
			LOG_ERR("Invalid state %d for cmd %d",
					dmic_private.state, cmd);
		}
		break;
	case DMIC_TRIGGER_STOP:
	case DMIC_TRIGGER_PAUSE:
	key = irq_lock();
	dmic_private.state = DMIC_STATE_CONFIGURED;
	irq_unlock(key);
		break;
	default:
		break;
	}

	return 0;
}

static inline uint8_t dmic_parse_clk_skew_map(uint32_t skew_map, uint8_t pdm)
{
	return (uint8_t)((skew_map >> ((pdm & BIT_MASK(3)) * 4U)) & BIT_MASK(4));
}

static int dmic_initialize_device(const struct device *dev)
{
	int stream;
	struct _stream_data *stream_data;
	/* Initialize the buffer queues */
	for (stream = 0; stream < DMIC_MAX_STREAMS; stream++) {
		stream_data = &dmic_private.streams[stream];
		k_msgq_init(&stream_data->in_queue,
				(char *)stream_data->in_msgs,
				sizeof(void *), DMIC_BUF_Q_LEN);
		k_msgq_init(&stream_data->out_queue,
				(char *)stream_data->out_msgs,
				sizeof(void *), DMIC_BUF_Q_LEN);
	}

	/* Set state, note there is no playback direction support */
	dmic_private.state = DMIC_STATE_INITIALIZED;

	LOG_DBG("Device %s Initialized", dev->name);

	return 0;
}

static int dmic_configure_device(const struct device *dev,
				 struct dmic_cfg *config)
{
	int ret = 0;

	ret = dmic_set_config(dev, config);
	if (ret) {
		LOG_ERR("dmic_set_config failed with code %d", ret);
	}

	ret = dmic_configure_dma(config->streams, dmic_private.num_streams);
	if (ret) {
		LOG_ERR("dmic_configure_dma failed with code %d", ret);
		return ret;
	}

	return ret;
}

static int dmic_read_device(const struct device *dev, uint8_t stream,
			    void **buffer, size_t *size, int32_t timeout)
{
	int ret;

	if (stream >= dmic_private.num_streams) {
		LOG_ERR("stream %u invalid. must be < %u", stream,
				dmic_private.num_streams);
		return -EINVAL;
	}

	/* retrieve buffer from out queue */
	ret = k_msgq_get(&dmic_private.streams[stream].out_queue,
			buffer, K_MSEC(timeout));
	if (ret) {
		LOG_ERR("No buffers in stream %u out_queue", stream);
	} else {
		*size = dmic_private.streams[stream].block_size;
		SOC_DCACHE_INVALIDATE(*buffer, *size);
	}

	return ret;
}

int dmic_configure_dma(struct pcm_stream_cfg *config, uint8_t num_streams)
{
	int ret = 0;
	int stream;
	uint32_t channel;
	struct dma_block_config dma_block;
	struct dma_config dma_cfg = {
		.dma_slot		= DMA_HANDSHAKE_DMIC_RXA,
		.channel_direction	= PERIPHERAL_TO_MEMORY,
		.complete_callback_en	= 1,
		.error_callback_en	= 0,
		.source_handshake	= 0,
		.dest_handshake		= 0,
		.channel_priority	= 0,
		.source_chaining_en	= 0,
		.dest_chaining_en	= 0,
		.source_data_size	= 4,
		.dest_data_size		= 4,
		.source_burst_length	= 8,
		.dest_burst_length	= 8,
		.block_count		= 1,
		.head_block		= &dma_block,
		.dma_callback		= dmic_dma_callback,
	};

	dmic_private.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(0, 0));
	if (!device_is_ready(dmic_private.dma_dev)) {
		LOG_ERR("Failed - device is not ready: %s", dmic_private.dma_dev->name);
		return -ENODEV;
	}

	for (stream = 0; stream < num_streams; stream++) {
		channel = (stream == 0) ? DMA_CHANNEL_DMIC_RXA :
			DMA_CHANNEL_DMIC_RXB;
		dma_cfg.dma_slot = (stream == 0) ? DMA_HANDSHAKE_DMIC_RXA :
			DMA_HANDSHAKE_DMIC_RXB;

		LOG_DBG("Configuring stream %u DMA ch%u handshake %u", stream,
				channel, dma_cfg.dma_slot);

		dma_block.source_address = (uint32_t)NULL;
		dma_block.dest_address = (uint32_t)NULL;
		dma_block.block_size = 0U;
		dma_block.next_block = NULL;

		ret = dma_config(dmic_private.dma_dev, channel, &dma_cfg);
		if (ret) {
			LOG_ERR("dma_config channel %u failed (%d)", channel,
					ret);
		}
	}
	return ret;
}

int dmic_reload_dma(uint32_t channel, void *buffer, size_t size)
{
	uint32_t source;

	source = (channel == DMA_CHANNEL_DMIC_RXA) ? OUTDATA0 : OUTDATA1;

	LOG_DBG("Loading buffer %p size %u to channel %u", buffer, size,
			channel);
	return dma_reload(dmic_private.dma_dev, channel,
			PDM_BASE + source, (uint32_t)buffer, size);
}

int dmic_start_dma(uint32_t channel)
{
	LOG_DBG("Starting DMA channel %u", channel);
	return dma_start(dmic_private.dma_dev, channel);
}

int dmic_stop_dma(uint32_t channel)
{
	LOG_DBG("Stopping DMA channel %u", channel);
	return dma_stop(dmic_private.dma_dev, channel);
}

static struct _dmic_ops dmic_ops = {
	.trigger = dmic_trigger_device,
	.configure = dmic_configure_device,
	.read = dmic_read_device,
};

DEVICE_DT_INST_DEFINE(0, &dmic_initialize_device, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_ops);
