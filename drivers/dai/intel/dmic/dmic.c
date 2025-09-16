/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_dai_dmic
#define LOG_DOMAIN dai_intel_dmic
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/drivers/dai.h>
#include <zephyr/irq.h>

#include "dmic.h"
#include <dmic_regs.h>

/* Base addresses (in PDM scope) of 2ch PDM controllers and coefficient RAM. */
static const uint32_t dmic_base[4] = {PDM0, PDM1, PDM2, PDM3};

/* global data shared between all dmic instances */
struct dai_dmic_global_shared dai_dmic_global;

/* Helper macro to read 64-bit data using two 32-bit data read */
#define sys_read64(addr)    (((uint64_t)(sys_read32(addr + 4)) << 32) | \
			     sys_read32(addr))

int dai_dmic_set_config_nhlt(struct dai_intel_dmic *dmic, const void *spec_config);

/* Exponent function for small values of x. This function calculates
 * fairly accurately exponent for x in range -2.0 .. +2.0. The iteration
 * uses first 11 terms of Taylor series approximation for exponent
 * function. With the current scaling the numerator just remains under
 * 64 bits with the 11 terms.
 *
 * See https://en.wikipedia.org/wiki/Exponential_function#Computation
 *
 * The input is Q3.29
 * The output is Q9.23
 */
static int32_t exp_small_fixed(int32_t x)
{
	int64_t p;
	int64_t num = Q_SHIFT_RND(x, 29, 23);
	int32_t y = (int32_t)num;
	int32_t den = 1;
	int32_t inc;
	int k;

	/* Numerator is x^k, denominator is k! */
	for (k = 2; k < 12; k++) {
		p = num * x; /* Q9.23 x Q3.29 -> Q12.52 */
		num = Q_SHIFT_RND(p, 52, 23);
		den = den * k;
		inc = (int32_t)(num / den);
		y += inc;
	}

	return y + ONE_Q23;
}

static int32_t exp_fixed(int32_t x)
{
	int32_t xs;
	int32_t y;
	int32_t z;
	int i;
	int n = 0;

	if (x < Q_CONVERT_FLOAT(-11.5, 27)) {
		return 0;
	}

	if (x > Q_CONVERT_FLOAT(7.6245, 27)) {
		return INT32_MAX;
	}

	/* x is Q5.27 */
	xs = x;
	while (xs >= TWO_Q27 || xs <= MINUS_TWO_Q27) {
		xs >>= 1;
		n++;
	}

	/* exp_small_fixed() input is Q3.29, while x1 is Q5.27
	 * exp_small_fixed() output is Q9.23, while z is Q12.20
	 */
	z = Q_SHIFT_RND(exp_small_fixed(Q_SHIFT_LEFT(xs, 27, 29)), 23, 20);
	y = ONE_Q20;
	for (i = 0; i < (1 << n); i++) {
		y = (int32_t)Q_MULTSR_32X32((int64_t)y, z, 20, 20, 20);
	}

	return y;
}

static int32_t db2lin_fixed(int32_t db)
{
	int32_t arg;

	if (db < Q_CONVERT_FLOAT(-100.0, 24)) {
		return 0;
	}

	/* Q8.24 x Q5.27, result needs to be Q5.27 */
	arg = (int32_t)Q_MULTSR_32X32((int64_t)db, LOG10_DIV20_Q27, 24, 27, 27);
	return exp_fixed(arg);
}

static void dai_dmic_update_bits(const struct dai_intel_dmic *dmic,
				 uint32_t reg, uint32_t mask, uint32_t val)
{
	uint32_t dest = dmic->reg_base + reg;

	sys_write32((sys_read32(dest) & (~mask)) | (val & mask), dest);
}

static inline void dai_dmic_write(const struct dai_intel_dmic *dmic,
			   uint32_t reg, uint32_t val)
{
	sys_write32(val, dmic->reg_base + reg);
}

#ifndef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
static inline uint32_t dai_dmic_read(const struct dai_intel_dmic *dmic,
				     uint32_t reg)
{
	return sys_read32(dmic->reg_base + reg);
}
#endif

#if CONFIG_DAI_DMIC_HAS_OWNERSHIP
static inline void dai_dmic_claim_ownership(const struct dai_intel_dmic *dmic)
{
	/* DMIC Owner Select to DSP */
	sys_write32(sys_read32(dmic->shim_base + DMICLCTL_OFFSET) |
		    FIELD_PREP(DMICLCTL_OSEL, 0x3), dmic->shim_base + DMICLCTL_OFFSET);
}

static inline void dai_dmic_release_ownership(const struct dai_intel_dmic *dmic)
{
	/* DMIC Owner Select back to Host CPU + DSP */
	sys_write32(sys_read32(dmic->shim_base + DMICLCTL_OFFSET) &
			~DMICLCTL_OSEL, dmic->shim_base + DMICLCTL_OFFSET);
}

#else /* CONFIG_DAI_DMIC_HAS_OWNERSHIP */

static inline void dai_dmic_claim_ownership(const struct dai_intel_dmic *dmic) {}
static inline void dai_dmic_release_ownership(const struct dai_intel_dmic *dmic) {}

#endif /* CONFIG_DAI_DMIC_HAS_OWNERSHIP */

static inline uint32_t dai_dmic_base(const struct dai_intel_dmic *dmic)
{
#if defined(CONFIG_SOC_INTEL_ACE20_LNL) || defined(CONFIG_SOC_INTEL_ACE30) ||                      \
	defined(CONFIG_SOC_INTEL_ACE40)
	return dmic->hdamldmic_base;
#else
	return dmic->shim_base;
#endif
}

#if CONFIG_DAI_DMIC_HAS_MULTIPLE_LINE_SYNC
static inline void dai_dmic_set_sync_period(uint32_t period, const struct dai_intel_dmic *dmic)
{
	uint32_t val = CONFIG_DAI_DMIC_HW_IOCLK / period - 1;
	uint32_t base = dai_dmic_base(dmic);
	/* DMIC Change sync period */
#if defined(CONFIG_SOC_INTEL_ACE20_LNL) || defined(CONFIG_SOC_INTEL_ACE30) ||                      \
	defined(CONFIG_SOC_INTEL_ACE40)
	sys_write32(sys_read32(base + DMICSYNC_OFFSET) | FIELD_PREP(DMICSYNC_SYNCPRD, val),
		    base + DMICSYNC_OFFSET);
	sys_write32(sys_read32(base + DMICSYNC_OFFSET) | DMICSYNC_SYNCPU,
		    base + DMICSYNC_OFFSET);

	if (!WAIT_FOR((sys_read32(base + DMICSYNC_OFFSET) & DMICSYNC_SYNCPU) == 0, 1000,
		      k_sleep(K_USEC(100)))) {
		LOG_ERR("poll timeout");
	}

	sys_write32(sys_read32(base + DMICSYNC_OFFSET) | DMICSYNC_CMDSYNC,
		    base + DMICSYNC_OFFSET);
#else /* All other CAVS and ACE platforms */
	sys_write32(sys_read32(base + DMICSYNC_OFFSET) | FIELD_PREP(DMICSYNC_SYNCPRD, val),
		    base + DMICSYNC_OFFSET);
	sys_write32(sys_read32(base + DMICSYNC_OFFSET) | DMICSYNC_CMDSYNC,
		    base + DMICSYNC_OFFSET);
#endif
}

static inline void dai_dmic_clear_sync_period(const struct dai_intel_dmic *dmic)
{
	uint32_t base = dai_dmic_base(dmic);
	/* DMIC Clean sync period */
	sys_write32(sys_read32(base + DMICSYNC_OFFSET) & ~DMICSYNC_SYNCPRD,
			base + DMICSYNC_OFFSET);
	sys_write32(sys_read32(base + DMICSYNC_OFFSET) & ~DMICSYNC_CMDSYNC,
			base + DMICSYNC_OFFSET);
}

/* Preparing for command synchronization on multiple link segments */
static inline void dai_dmic_sync_prepare(const struct dai_intel_dmic *dmic)
{
	uint32_t base = dai_dmic_base(dmic);

	sys_write32(sys_read32(base + DMICSYNC_OFFSET) | DMICSYNC_CMDSYNC,
		    base + DMICSYNC_OFFSET);
}

/* Trigering synchronization of command execution */
static void dmic_sync_trigger(const struct dai_intel_dmic *dmic)
{
	uint32_t base = dai_dmic_base(dmic);

	__ASSERT_NO_MSG((sys_read32(base + DMICSYNC_OFFSET) & DMICSYNC_CMDSYNC) != 0);

	sys_write32(sys_read32(base + DMICSYNC_OFFSET) |
		    DMICSYNC_SYNCGO, base + DMICSYNC_OFFSET);

	/* waiting for CMDSYNC bit clearing */
	if (!WAIT_FOR((sys_read32(base + DMICSYNC_OFFSET) & DMICSYNC_CMDSYNC) == 0,
		      1000, k_sleep(K_USEC(100)))) {
		LOG_ERR("poll timeout");
	}
}

#else /* CONFIG_DAI_DMIC_HAS_MULTIPLE_LINE_SYNC */

static inline void dai_dmic_set_sync_period(uint32_t period, const struct dai_intel_dmic *dmic) {}
static inline void dai_dmic_clear_sync_period(const struct dai_intel_dmic *dmic) {}
static inline void dai_dmic_sync_prepare(const struct dai_intel_dmic *dmic) {}
static void dmic_sync_trigger(const struct dai_intel_dmic *dmic) {}

#endif /* CONFIG_DAI_DMIC_HAS_MULTIPLE_LINE_SYNC */

static void dai_dmic_start_fifo_packers(struct dai_intel_dmic *dmic, int fifo_index)
{

	/* Start FIFO packers and clear FIFO initialize bits */
	dai_dmic_update_bits(dmic, fifo_index * PDM_CHANNEL_REGS_SIZE + OUTCONTROL,
			     OUTCONTROL_SIP | OUTCONTROL_FINIT,
			     OUTCONTROL_SIP);
}

static void dai_dmic_stop_fifo_packers(struct dai_intel_dmic *dmic,
					int fifo_index)
{
	/* Stop FIFO packers and set FIFO initialize bits */
	dai_dmic_update_bits(dmic, fifo_index * PDM_CHANNEL_REGS_SIZE + OUTCONTROL,
			     OUTCONTROL_SIP | OUTCONTROL_FINIT,
			     OUTCONTROL_FINIT);
}

static inline void dai_dmic_dis_clk_gating(const struct dai_intel_dmic *dmic)
{
	/* Disable DMIC clock gating */
#if defined(CONFIG_SOC_INTEL_ACE20_LNL) || defined(CONFIG_SOC_INTEL_ACE30) ||                      \
	defined(CONFIG_SOC_INTEL_ACE40)
	sys_write32((sys_read32(dmic->vshim_base + DMICLVSCTL_OFFSET) | DMICLVSCTL_DCGD),
		    dmic->vshim_base + DMICLVSCTL_OFFSET);
#else
	sys_write32((sys_read32(dmic->shim_base + DMICLCTL_OFFSET) | DMICLCTL_DCGD),
		    dmic->shim_base + DMICLCTL_OFFSET);
#endif
}

static inline void dai_dmic_en_clk_gating(const struct dai_intel_dmic *dmic)
{
	/* Enable DMIC clock gating */
#if defined(CONFIG_SOC_INTEL_ACE20_LNL) || defined(CONFIG_SOC_INTEL_ACE30) ||                      \
	defined(CONFIG_SOC_INTEL_ACE40)
	sys_write32((sys_read32(dmic->vshim_base + DMICLVSCTL_OFFSET) & ~DMICLVSCTL_DCGD),
		    dmic->vshim_base + DMICLVSCTL_OFFSET);
#else /* All other CAVS and ACE platforms */
	sys_write32((sys_read32(dmic->shim_base + DMICLCTL_OFFSET) & ~DMICLCTL_DCGD),
		    dmic->shim_base + DMICLCTL_OFFSET);
#endif

}

static inline void dai_dmic_program_channel_map(const struct dai_intel_dmic *dmic,
						const struct dai_config *cfg,
						uint32_t index)
{
#if defined(CONFIG_SOC_INTEL_ACE20_LNL) || defined(CONFIG_SOC_INTEL_ACE30) ||                      \
	defined(CONFIG_SOC_INTEL_ACE40)
	uint16_t pcmsycm = cfg->link_config;
	uint32_t reg_add = dmic->shim_base + DMICXPCMSyCM_OFFSET + 0x0004*index;

	sys_write16(pcmsycm, reg_add);
#else
	ARG_UNUSED(dmic);
	ARG_UNUSED(cfg);
	ARG_UNUSED(index);
#endif /* CONFIG_SOC_INTEL_ACE20_LNL || CONFIG_SOC_INTEL_ACE30 || CONFIG_SOC_INTEL_ACE40 */
}

static inline void dai_dmic_en_power(const struct dai_intel_dmic *dmic)
{
	uint32_t base = dai_dmic_base(dmic);
	/* Enable DMIC power */
	sys_write32((sys_read32(base + DMICLCTL_OFFSET) | DMICLCTL_SPA),
			base + DMICLCTL_OFFSET);

#if defined(CONFIG_SOC_INTEL_ACE20_LNL) || defined(CONFIG_SOC_INTEL_ACE30) ||                      \
	defined(CONFIG_SOC_INTEL_ACE40)
	while (!(sys_read32(base + DMICLCTL_OFFSET) & DMICLCTL_CPA)) {
		k_busy_wait(100);
	}
#endif
}

static inline void dai_dmic_dis_power(const struct dai_intel_dmic *dmic)
{
	uint32_t base = dai_dmic_base(dmic);
	/* Disable DMIC power */
	sys_write32((sys_read32(base + DMICLCTL_OFFSET) & (~DMICLCTL_SPA)),
		     base + DMICLCTL_OFFSET);
}

static int dai_dmic_probe(struct dai_intel_dmic *dmic)
{
	LOG_INF("dmic_probe()");

	/* Set state, note there is no playback direction support */
	dmic->state = DAI_STATE_NOT_READY;

	/* Enable DMIC power */
	dai_dmic_en_power(dmic);

	/* Disable dynamic clock gating for dmic before touching any reg */
	dai_dmic_dis_clk_gating(dmic);

	/* DMIC Change sync period */
	dai_dmic_set_sync_period(CONFIG_DAI_DMIC_PLATFORM_SYNC_PERIOD, dmic);

	/* DMIC Owner Select to DSP */
	dai_dmic_claim_ownership(dmic);

	return 0;
}

static int dai_dmic_remove(struct dai_intel_dmic *dmic)
{
	uint32_t active_fifos_mask = dai_dmic_global.active_fifos_mask;
	uint32_t pause_mask = dai_dmic_global.pause_mask;

	LOG_INF("dmic_remove(), dmic_active_fifos_mask = 0x%x, dmic_pause_mask = 0x%x",
		active_fifos_mask, pause_mask);

	/* The next end tasks must be passed if another DAI FIFO still runs.
	 * Note: dai_put() function that calls remove() applies the spinlock
	 * so it is not needed here to protect access to mask bits.
	 */
	if (active_fifos_mask || pause_mask) {
		return 0;
	}

	/* Disable DMIC clock and power */
	dai_dmic_en_clk_gating(dmic);
	dai_dmic_dis_power(dmic);

	/* DMIC Clean sync period */
	dai_dmic_clear_sync_period(dmic);

	/* DMIC Owner Select back to Host CPU + DSP */
	dai_dmic_release_ownership(dmic);

	return 0;
}

static int dai_dmic_timestamp_config(const struct device *dev, struct dai_ts_cfg *cfg)
{
	cfg->walclk_rate = CONFIG_DAI_DMIC_HW_IOCLK;

	return 0;
}

static int dai_timestamp_dmic_start(const struct device *dev, struct dai_ts_cfg *cfg)
{
	uint32_t addr = TS_DMIC_LOCAL_TSCTRL;
	uint32_t cdmas;

	/* Set DMIC timestamp registers */

	/* First point CDMAS to GPDMA channel that is used by DMIC
	 * also clear NTK to be sure there is no old timestamp.
	 */
	cdmas = FIELD_PREP(TS_LOCAL_TSCTRL_CDMAS, cfg->dma_chan_index +
		cfg->dma_chan_count * cfg->dma_id);
	sys_write32(TS_LOCAL_TSCTRL_NTK | cdmas, addr);

	/* Request on demand timestamp */
	sys_write32(TS_LOCAL_TSCTRL_ODTS | cdmas, addr);

	return 0;
}

static int dai_timestamp_dmic_stop(const struct device *dev, struct dai_ts_cfg *cfg)
{
	/* Clear NTK and write zero to CDMAS */
	sys_write32(TS_LOCAL_TSCTRL_NTK, TS_DMIC_LOCAL_TSCTRL);
	return 0;
}

static int dai_timestamp_dmic_get(const struct device *dev, struct dai_ts_cfg *cfg,
				  struct dai_ts_data *tsd)
{
	/* Read DMIC timestamp registers */
	uint32_t tsctrl = TS_DMIC_LOCAL_TSCTRL;
	uint32_t ntk;

	/* Read SSP timestamp registers */
	ntk = sys_read32(tsctrl) & TS_LOCAL_TSCTRL_NTK;
	if (!ntk) {
		goto out;
	}

	/* NTK was set, get wall clock */
	tsd->walclk = sys_read64(TS_DMIC_LOCAL_WALCLK);

	/* Sample */
	tsd->sample = sys_read64(TS_DMIC_LOCAL_SAMPLE);

	/* Clear NTK to enable successive timestamps */
	sys_write32(TS_LOCAL_TSCTRL_NTK, tsctrl);

out:
	tsd->walclk_rate = cfg->walclk_rate;
	if (!ntk) {
		return -ENODATA;
	}

	return 0;
}

/* this ramps volume changes over time */
static void dai_dmic_gain_ramp(struct dai_intel_dmic *dmic)
{
	k_spinlock_key_t key;
	int32_t gval;
	uint32_t val;
	int i;

	/* Currently there's no DMIC HW internal mutings and wait times
	 * applied into this start sequence. It can be implemented here if
	 * start of audio capture would contain clicks and/or noise and it
	 * is not suppressed by gain ramp somewhere in the capture pipe.
	 */
	LOG_DBG("DMIC gain ramp");

	/*
	 * At run-time dmic->gain is only changed in this function, and this
	 * function runs in the pipeline task context, so it cannot run
	 * concurrently on multiple cores, since there's always only one
	 * task associated with each DAI, so we don't need to hold the lock to
	 * read the value here.
	 */
	if (dmic->gain == DMIC_HW_FIR_GAIN_MAX << 11) {
		return;
	}

	key = k_spin_lock(&dmic->lock);

	/* Increment gain with logarithmic step.
	 * Gain is Q2.30 and gain modifier is Q12.20.
	 */
	dmic->startcount++;
	dmic->gain = q_multsr_sat_32x32(dmic->gain, dmic->gain_coef, Q_SHIFT_GAIN_X_GAIN_COEF);

	/* Gain is stored as Q2.30, while HW register is Q1.19 so shift
	 * the value right by 11.
	 */
	gval = dmic->gain >> 11;

	/* Note that DMIC gain value zero has a special purpose. Value zero
	 * sets gain bypass mode in HW. Zero value will be applied after ramp
	 * is complete. It is because exact 1.0 gain is not possible with Q1.19.
	 */
	if (gval > DMIC_HW_FIR_GAIN_MAX) {
		gval = 0;
		dmic->gain = DMIC_HW_FIR_GAIN_MAX << 11;
	}

	/* Write gain to registers */
	for (i = 0; i < CONFIG_DAI_DMIC_HW_CONTROLLERS; i++) {
		if (!dmic->enable[i]) {
			continue;
		}

		if (dmic->startcount == DMIC_UNMUTE_CIC) {
			dai_dmic_update_bits(dmic, dmic_base[i] + CIC_CONTROL,
					     CIC_CONTROL_MIC_MUTE, 0);
		}

		if (dmic->startcount == DMIC_UNMUTE_FIR) {
			dai_dmic_update_bits(dmic, dmic_base[i] + FIR_CHANNEL_REGS_SIZE *
					     dmic->dai_config_params.dai_index + FIR_CONTROL,
					     FIR_CONTROL_MUTE, 0);
		}

		if (gval != 0) {
			val = FIELD_PREP(OUT_GAIN, gval);
			dai_dmic_write(dmic, dmic_base[i] + FIR_CHANNEL_REGS_SIZE *
				       dmic->dai_config_params.dai_index + OUT_GAIN_LEFT, val);
			dai_dmic_write(dmic, dmic_base[i] + FIR_CHANNEL_REGS_SIZE *
				       dmic->dai_config_params.dai_index + OUT_GAIN_RIGHT, val);
		} else {
			dai_dmic_write(dmic, dmic_base[i] + FIR_CHANNEL_REGS_SIZE *
				       dmic->dai_config_params.dai_index + OUT_GAIN_LEFT,
				       dmic->gain_left);
			dai_dmic_write(dmic, dmic_base[i] + FIR_CHANNEL_REGS_SIZE *
				       dmic->dai_config_params.dai_index + OUT_GAIN_RIGHT,
				       dmic->gain_right);
		}
	}

	k_spin_unlock(&dmic->lock, key);
}

static void dai_dmic_start(struct dai_intel_dmic *dmic)
{
	k_spinlock_key_t key;
	int i;
	int mic_a;
	int mic_b;
	int start_fir;

	/* enable port */
	key = k_spin_lock(&dmic->lock);

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	for (i = 0; i < CONFIG_DAI_DMIC_HW_CONTROLLERS; i++) {
		dai_dmic_update_bits(dmic, dmic_base[i] + CIC_CONTROL, CIC_CONTROL_SOFT_RESET, 0);
	}
#endif

	dmic->startcount = 0;

	/* Compute unmute ramp gain update coefficient. */
	dmic->gain_coef = db2lin_fixed(LOGRAMP_CONST_TERM / dmic->unmute_time_ms);

	/* Initial gain value, convert Q12.20 to Q2.30 */
	dmic->gain = Q_SHIFT_LEFT(db2lin_fixed(LOGRAMP_START_DB), 20, 30);

	dai_dmic_sync_prepare(dmic);

	dai_dmic_start_fifo_packers(dmic, dmic->dai_config_params.dai_index);

	for (i = 0; i < CONFIG_DAI_DMIC_HW_CONTROLLERS; i++) {
		mic_a = dmic->enable[i] & 1;
		mic_b = (dmic->enable[i] & 2) >> 1;

		/* If both microphones are needed start them simultaneously
		 * to start them in sync. The reset may be cleared for another
		 * FIFO already. If only one mic, start them independently.
		 * This makes sure we do not clear start/en for another DAI.
		 */
		if (mic_a && mic_b) {
			dai_dmic_update_bits(dmic, dmic_base[i] + CIC_CONTROL,
					     CIC_CONTROL_CIC_START_A |
					     CIC_CONTROL_CIC_START_B,
					     FIELD_PREP(CIC_CONTROL_CIC_START_A, 1) |
					     FIELD_PREP(CIC_CONTROL_CIC_START_B, 1));
			dai_dmic_update_bits(dmic, dmic_base[i] + MIC_CONTROL,
					     MIC_CONTROL_PDM_EN_A |
					     MIC_CONTROL_PDM_EN_B,
					     FIELD_PREP(MIC_CONTROL_PDM_EN_A, 1) |
					     FIELD_PREP(MIC_CONTROL_PDM_EN_B, 1));
		} else if (mic_a) {
			dai_dmic_update_bits(dmic, dmic_base[i] + CIC_CONTROL,
					     CIC_CONTROL_CIC_START_A,
					     FIELD_PREP(CIC_CONTROL_CIC_START_A, 1));
			dai_dmic_update_bits(dmic, dmic_base[i] + MIC_CONTROL,
					     MIC_CONTROL_PDM_EN_A,
					     FIELD_PREP(MIC_CONTROL_PDM_EN_A, 1));
		} else if (mic_b) {
			dai_dmic_update_bits(dmic, dmic_base[i] + CIC_CONTROL,
					     CIC_CONTROL_CIC_START_B,
					     FIELD_PREP(CIC_CONTROL_CIC_START_B, 1));
			dai_dmic_update_bits(dmic, dmic_base[i] + MIC_CONTROL,
					     MIC_CONTROL_PDM_EN_B,
					     FIELD_PREP(MIC_CONTROL_PDM_EN_B, 1));
		}
	}

	for (i = 0; i < CONFIG_DAI_DMIC_HW_CONTROLLERS; i++) {
		start_fir = dmic->enable[i] > 0;
		dai_dmic_update_bits(dmic, dmic_base[i] + FIR_CHANNEL_REGS_SIZE *
				     dmic->dai_config_params.dai_index + FIR_CONTROL,
				     FIR_CONTROL_START,
				     FIELD_PREP(FIR_CONTROL_START, start_fir));
	}

#ifndef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	/* Clear soft reset for all/used PDM controllers. This should
	 * start capture in sync.
	 */
	for (i = 0; i < CONFIG_DAI_DMIC_HW_CONTROLLERS; i++) {
		dai_dmic_update_bits(dmic, dmic_base[i] + CIC_CONTROL,
				     CIC_CONTROL_SOFT_RESET, 0);

		LOG_INF("dmic_start(), cic 0x%08x",
			dai_dmic_read(dmic, dmic_base[i] + CIC_CONTROL));
	}
#endif

	/* Set bit dai->index */
	dai_dmic_global.active_fifos_mask |= BIT(dmic->dai_config_params.dai_index);
	dai_dmic_global.pause_mask &= ~BIT(dmic->dai_config_params.dai_index);

	dmic->state = DAI_STATE_RUNNING;
	k_spin_unlock(&dmic->lock, key);

	dmic_sync_trigger(dmic);

	LOG_INF("dmic_start(), dmic_active_fifos_mask = 0x%x",
		dai_dmic_global.active_fifos_mask);
}

static void dai_dmic_stop(struct dai_intel_dmic *dmic, bool stop_is_pause)
{
	k_spinlock_key_t key;
	int i;

	LOG_DBG("dmic_stop()");
	key = k_spin_lock(&dmic->lock);

	dai_dmic_stop_fifo_packers(dmic, dmic->dai_config_params.dai_index);

	/* Set soft reset and mute on for all PDM controllers. */
	LOG_INF("dmic_stop(), dmic_active_fifos_mask = 0x%x",
			dai_dmic_global.active_fifos_mask);

	/* Clear bit dmic->dai_config_params.dai_index for active FIFO.
	 * If stop for pause, set pause mask bit.
	 * If stop is not for pausing, it is safe to clear the pause bit.
	 */
	dai_dmic_global.active_fifos_mask &= ~BIT(dmic->dai_config_params.dai_index);
	if (stop_is_pause) {
		dai_dmic_global.pause_mask |= BIT(dmic->dai_config_params.dai_index);
	} else {
		dai_dmic_global.pause_mask &= ~BIT(dmic->dai_config_params.dai_index);
	}

	for (i = 0; i < CONFIG_DAI_DMIC_HW_CONTROLLERS; i++) {
		/* Don't stop CIC yet if one FIFO remains active */
		if (dai_dmic_global.active_fifos_mask == 0) {
			dai_dmic_update_bits(dmic, dmic_base[i] + CIC_CONTROL,
					     CIC_CONTROL_SOFT_RESET |
					     CIC_CONTROL_MIC_MUTE,
					     CIC_CONTROL_SOFT_RESET |
					     CIC_CONTROL_MIC_MUTE);
		}
		dai_dmic_update_bits(dmic, dmic_base[i] + FIR_CHANNEL_REGS_SIZE *
				     dmic->dai_config_params.dai_index + FIR_CONTROL,
				     FIR_CONTROL_MUTE,
				     FIR_CONTROL_MUTE);
	}

	k_spin_unlock(&dmic->lock, key);
}

const struct dai_properties *dai_dmic_get_properties(const struct device *dev,
						     enum dai_dir dir,
						     int stream_id)
{
	const struct dai_intel_dmic *dmic = (const struct dai_intel_dmic *)dev->data;
	struct dai_properties *prop = (struct dai_properties *)dev->config;

	prop->fifo_address = dmic->fifo.offset;
	prop->fifo_depth = dmic->fifo.depth;
	prop->dma_hs_id = dmic->fifo.handshake;
	prop->reg_init_delay = 0;

	return prop;
}

static int dai_dmic_trigger(const struct device *dev, enum dai_dir dir,
			    enum dai_trigger_cmd cmd)
{
	struct dai_intel_dmic *dmic = (struct dai_intel_dmic *)dev->data;

	LOG_DBG("dmic_trigger()");

	if (dir != DAI_DIR_RX) {
		LOG_ERR("dmic_trigger(): direction != DAI_DIR_RX");
		return -EINVAL;
	}

	switch (cmd) {
	case DAI_TRIGGER_START:
		if (dmic->state == DAI_STATE_PAUSED ||
		    dmic->state == DAI_STATE_PRE_RUNNING) {
			dai_dmic_start(dmic);
			dmic->state = DAI_STATE_RUNNING;
		} else {
			LOG_ERR("dmic_trigger(): state is not prepare or paused, dmic->state = %u",
				dmic->state);
		}
		break;
	case DAI_TRIGGER_STOP:
		dai_dmic_stop(dmic, false);
		dmic->state = DAI_STATE_PRE_RUNNING;
		break;
	case DAI_TRIGGER_PAUSE:
		dai_dmic_stop(dmic, true);
		dmic->state = DAI_STATE_PAUSED;
		break;
	case DAI_TRIGGER_COPY:
		dai_dmic_gain_ramp(dmic);
		break;
	default:
		break;
	}

	return 0;
}

static int dai_dmic_get_config(const struct device *dev, struct dai_config *cfg, enum dai_dir dir)
{
	struct dai_intel_dmic *dmic = (struct dai_intel_dmic *)dev->data;

	if (dir != DAI_DIR_RX) {
		return -EINVAL;
	}

	if (!cfg) {
		return -EINVAL;
	}

	*cfg = dmic->dai_config_params;

	return 0;
}

static int dai_dmic_set_config(const struct device *dev,
		const struct dai_config *cfg, const void *bespoke_cfg)

{
	struct dai_intel_dmic *dmic = (struct dai_intel_dmic *)dev->data;
	int ret = 0;
	int di = dmic->dai_config_params.dai_index;
	k_spinlock_key_t key;

	LOG_INF("dmic_set_config()");

	if (di >= CONFIG_DAI_DMIC_HW_FIFOS) {
		LOG_ERR("dmic_set_config(): DAI index exceeds number of FIFOs");
		return -EINVAL;
	}

	if (!bespoke_cfg) {
		LOG_ERR("dmic_set_config(): NULL config");
		return -EINVAL;
	}

	dai_dmic_program_channel_map(dmic, cfg, di);

	key = k_spin_lock(&dmic->lock);

#if CONFIG_DAI_INTEL_DMIC_TPLG_PARAMS
#error DMIC TPLG is not yet implemented

#elif CONFIG_DAI_INTEL_DMIC_NHLT
	ret = dai_dmic_set_config_nhlt(dmic, bespoke_cfg);

	/* There's no unmute ramp duration in blob, so the default rate dependent is used. */
	dmic->unmute_time_ms = dmic_get_unmute_ramp_from_samplerate(dmic->dai_config_params.rate);
#else
#error No DMIC config selected
#endif

	if (ret < 0) {
		LOG_ERR("dmic_set_config(): Failed to set the requested configuration.");
		goto out;
	}

	dmic->state = DAI_STATE_PRE_RUNNING;

out:
	k_spin_unlock(&dmic->lock, key);
	return ret;
}

static int dai_dmic_probe_wrapper(const struct device *dev)
{
	struct dai_intel_dmic *dmic = (struct dai_intel_dmic *)dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&dmic->lock);

	if (dmic->sref == 0) {
		ret = dai_dmic_probe(dmic);
	}

	if (!ret) {
		dmic->sref++;
	}

	k_spin_unlock(&dmic->lock, key);

	return ret;
}

static int dai_dmic_remove_wrapper(const struct device *dev)
{
	struct dai_intel_dmic *dmic = (struct dai_intel_dmic *)dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&dmic->lock);

	if (--dmic->sref == 0) {
		ret = dai_dmic_remove(dmic);
	}

	k_spin_unlock(&dmic->lock, key);

	return ret;
}

static int dmic_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		dai_dmic_remove_wrapper(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		dai_dmic_probe_wrapper(dev);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		/* All device pm is handled during resume and suspend */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

DEVICE_API(dai, dai_dmic_ops) = {
	.probe			= pm_device_runtime_get,
	.remove			= pm_device_runtime_put,
	.config_set		= dai_dmic_set_config,
	.config_get		= dai_dmic_get_config,
	.get_properties		= dai_dmic_get_properties,
	.trigger		= dai_dmic_trigger,
	.ts_config		= dai_dmic_timestamp_config,
	.ts_start		= dai_timestamp_dmic_start,
	.ts_stop		= dai_timestamp_dmic_stop,
	.ts_get			= dai_timestamp_dmic_get
};

static int dai_dmic_initialize_device(const struct device *dev)
{
	return pm_device_driver_init(dev, dmic_pm_action);
};


#define DAI_INTEL_DMIC_DEVICE_INIT(n)					\
	static struct dai_properties dai_intel_dmic_properties_##n;	\
									\
	static struct dai_intel_dmic dai_intel_dmic_data_##n =		\
	{	.dai_config_params =					\
		{							\
			.type = DAI_INTEL_DMIC,				\
			.dai_index = n					\
		},							\
		.reg_base = DT_INST_REG_ADDR_BY_IDX(n, 0),		\
		.shim_base = DT_INST_PROP(n, shim),			\
		IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(hdamlddmic)),	\
			(.hdamldmic_base = DT_REG_ADDR(DT_NODELABEL(hdamlddmic)),))	\
		IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(dmicvss)),	\
			(.vshim_base = DT_REG_ADDR(DT_NODELABEL(dmicvss)),))	\
		.irq = DT_INST_IRQN(n),					\
		.fifo =							\
		{							\
			.offset = DT_INST_REG_ADDR_BY_IDX(n, 0)		\
				+ DT_INST_PROP(n, fifo),		\
			.handshake = DMA_HANDSHAKE_DMIC_CH##n		\
		},							\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(n, dmic_pm_action);			\
									\
	DEVICE_DT_INST_DEFINE(n,					\
		dai_dmic_initialize_device,				\
		PM_DEVICE_DT_INST_GET(n),				\
		&dai_intel_dmic_data_##n,				\
		&dai_intel_dmic_properties_##n,				\
		POST_KERNEL,						\
		CONFIG_DAI_INIT_PRIORITY,				\
		&dai_dmic_ops);

DT_INST_FOREACH_STATUS_OKAY(DAI_INTEL_DMIC_DEVICE_INIT)
