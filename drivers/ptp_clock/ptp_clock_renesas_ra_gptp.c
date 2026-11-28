/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_gptp_timer

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

#include "r_gptp.h"

LOG_MODULE_REGISTER(ptp_clock_renesas_ra_gptp);

#define GPTP_NSEC_PER_SEC INT64_C(1000000000)

/* PTPTIVCt.TIV: bits [31:27] are nanoseconds, bits [26:0] are a binary fraction */
#define GPTP_TIV_FRAC_BITS 27U

/* GPTP timer seconds are 48 bits wide */
#define GPTP_SEC_MAX ((UINT64_C(1) << 48) - 1U)

/* Rate offset limit (+-50 %), keeps the increment computation within int64_t */
#define GPTP_MAX_ADJ_SCALED_PPM (INT64_C(500000) << PTP_CLOCK_SCALED_PPM_SHIFT)

struct ptp_clock_renesas_ra_gptp_config {
	const struct device *eswm_dev;
	const struct device *clock_dev;
	uint8_t timer;
};

struct ptp_clock_renesas_ra_gptp_data {
	gptp_instance_ctrl_t fsp_ctrl;
	gptp_cfg_t fsp_cfg;
	struct k_spinlock lock;
	uint32_t tiv_nominal;
};

/* Caller must hold data->lock: reading the nanosecond register latches the second registers. */
static int gptp_read_locked(const struct device *dev, struct net_ptp_time *tm)
{
	const struct ptp_clock_renesas_ra_gptp_config *config = dev->config;
	struct ptp_clock_renesas_ra_gptp_data *data = dev->data;
	gptp_timer_value_t value;
	fsp_err_t err;

	err = R_GPTP_TimerValueGet(&data->fsp_ctrl, config->timer, &value);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	tm->second = ((uint64_t)value.time_sec_upper << 32) | value.time_sec_lower;
	tm->nanosecond = value.time_nsec;

	return 0;
}

static int ptp_clock_renesas_ra_gptp_get(const struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_clock_renesas_ra_gptp_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&data->lock);
	ret = gptp_read_locked(dev, tm);
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ptp_clock_renesas_ra_gptp_set(const struct device *dev, struct net_ptp_time *tm)
{
	const struct ptp_clock_renesas_ra_gptp_config *config = dev->config;
	struct ptp_clock_renesas_ra_gptp_data *data = dev->data;
	struct net_ptp_time now;
	k_spinlock_key_t key;
	int64_t sec_diff;
	int64_t offset;
	int ret;

	if ((tm->second > GPTP_SEC_MAX) || (tm->nanosecond >= (uint32_t)GPTP_NSEC_PER_SEC)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	ret = gptp_read_locked(dev, &now);
	if (ret != 0) {
		goto out;
	}

	/*
	 * The timer value is a free-running count plus an offset register and
	 * cannot be written directly. Shift the offset by the difference to the
	 * current time; the time between the read and the offset write is not
	 * compensated.
	 */
	sec_diff = (int64_t)tm->second - (int64_t)now.second;
	if ((sec_diff >= (INT64_MAX / GPTP_NSEC_PER_SEC)) ||
	    (sec_diff <= (INT64_MIN / GPTP_NSEC_PER_SEC))) {
		ret = -ERANGE;
		goto out;
	}

	offset = (sec_diff * GPTP_NSEC_PER_SEC) +
		 ((int64_t)tm->nanosecond - (int64_t)now.nanosecond);

	if (R_GPTP_TimerOffsetSet(&data->fsp_ctrl, config->timer, offset) != FSP_SUCCESS) {
		ret = -EIO;
	}

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ptp_clock_renesas_ra_gptp_adjust(const struct device *dev, int increment)
{
	const struct ptp_clock_renesas_ra_gptp_config *config = dev->config;
	struct ptp_clock_renesas_ra_gptp_data *data = dev->data;
	k_spinlock_key_t key;
	fsp_err_t err;

	key = k_spin_lock(&data->lock);
	err = R_GPTP_TimerOffsetSet(&data->fsp_ctrl, config->timer, (int64_t)increment);
	k_spin_unlock(&data->lock, key);

	return (err == FSP_SUCCESS) ? 0 : -EIO;
}

static int ptp_clock_renesas_ra_gptp_adjust_rate(const struct device *dev, int64_t scaled_ppm)
{
	const struct ptp_clock_renesas_ra_gptp_config *config = dev->config;
	struct ptp_clock_renesas_ra_gptp_data *data = dev->data;
	k_spinlock_key_t key;
	fsp_err_t err;
	int64_t ppb;
	int64_t delta;
	int64_t tiv;

	if ((scaled_ppm > GPTP_MAX_ADJ_SCALED_PPM) || (scaled_ppm < -GPTP_MAX_ADJ_SCALED_PPM)) {
		return -ERANGE;
	}

	/* The offset refers to the nominal frequency, it is not cumulative. */
	ppb = ptp_clock_scaled_ppm_to_ppb(scaled_ppm);
	delta = (int64_t)data->tiv_nominal * ppb;
	delta = (delta + ((delta < 0) ? -(GPTP_NSEC_PER_SEC / 2) : (GPTP_NSEC_PER_SEC / 2))) /
		GPTP_NSEC_PER_SEC;
	tiv = (int64_t)data->tiv_nominal + delta;

	if ((tiv <= 0) || (tiv > (int64_t)UINT32_MAX)) {
		return -ERANGE;
	}

	key = k_spin_lock(&data->lock);
	err = R_GPTP_TimerRateSet(&data->fsp_ctrl, config->timer, (uint32_t)tiv);
	k_spin_unlock(&data->lock, key);

	return (err == FSP_SUCCESS) ? 0 : -EIO;
}

static int ptp_clock_renesas_ra_gptp_init(const struct device *dev)
{
	const struct ptp_clock_renesas_ra_gptp_config *config = dev->config;
	struct ptp_clock_renesas_ra_gptp_data *data = dev->data;
	uint32_t clk_rate;
	uint64_t tiv;
	fsp_err_t err;
	int ret;

	if (!device_is_ready(config->eswm_dev)) {
		LOG_ERR("ESWM device not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev, NULL, &clk_rate);
	if (ret != 0) {
		return ret;
	}

	if (clk_rate == 0U) {
		return -EINVAL;
	}

	/* Nominal increment: ESWCLK period in ns with GPTP_TIV_FRAC_BITS fractional bits */
	tiv = (((uint64_t)GPTP_NSEC_PER_SEC << GPTP_TIV_FRAC_BITS) + (clk_rate / 2U)) / clk_rate;
	if ((tiv == 0U) || (tiv > UINT32_MAX)) {
		LOG_ERR("Unsupported timer clock %u Hz", clk_rate);
		return -ENOTSUP;
	}

	data->tiv_nominal = (uint32_t)tiv;

	err = R_GPTP_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		LOG_ERR("GPTP open failed, err=%d", err);
		return -EIO;
	}

	/* R_GPTP_TimerCfg() only accepts whole nanoseconds, write the full increment instead. */
	err = R_GPTP_TimerRateSet(&data->fsp_ctrl, config->timer, data->tiv_nominal);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	err = R_GPTP_Start(&data->fsp_ctrl, config->timer);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	LOG_DBG("Timer %u: clock %u Hz, increment 0x%08x", config->timer, clk_rate,
		data->tiv_nominal);

	return 0;
}

static DEVICE_API(ptp_clock, ptp_clock_renesas_ra_gptp_api) = {
	.set = ptp_clock_renesas_ra_gptp_set,
	.get = ptp_clock_renesas_ra_gptp_get,
	.adjust = ptp_clock_renesas_ra_gptp_adjust,
	.adjust_rate = ptp_clock_renesas_ra_gptp_adjust_rate,
};

/* Timer node -> renesas,ra-gptp block node -> ESWM node */
#define GPTP_BLOCK_NODE(n) DT_INST_PARENT(n)
#define GPTP_ESWM_NODE(n)  DT_PARENT(GPTP_BLOCK_NODE(n))

#define PTP_CLOCK_RENESAS_RA_GPTP_INIT(n)                                                          \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(GPTP_BLOCK_NODE(n), renesas_ra_gptp),                      \
		     "renesas,ra-gptp-timer node must be a child of the renesas,ra-gptp node");    \
	BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(GPTP_BLOCK_NODE(n)),                                  \
		     "renesas,ra-gptp node must be enabled");                                      \
	BUILD_ASSERT(DT_INST_REG_ADDR(n) < BSP_FEATURE_ESWM_GPTP_TIMER_NUM,                        \
		     "Invalid gPTP timer number");                                                 \
                                                                                                   \
	static const struct ptp_clock_renesas_ra_gptp_config                                       \
		ptp_clock_renesas_ra_gptp_config_##n = {                                           \
			.eswm_dev = DEVICE_DT_GET(GPTP_ESWM_NODE(n)),                              \
			.clock_dev =                                                               \
				DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(GPTP_ESWM_NODE(n), eswclk)),  \
			.timer = DT_INST_REG_ADDR(n),                                              \
	};                                                                                         \
                                                                                                   \
	static struct ptp_clock_renesas_ra_gptp_data ptp_clock_renesas_ra_gptp_data_##n;           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ptp_clock_renesas_ra_gptp_init, NULL,                             \
			      &ptp_clock_renesas_ra_gptp_data_##n,                                 \
			      &ptp_clock_renesas_ra_gptp_config_##n, POST_KERNEL,                  \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &ptp_clock_renesas_ra_gptp_api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_RENESAS_RA_GPTP_INIT)
