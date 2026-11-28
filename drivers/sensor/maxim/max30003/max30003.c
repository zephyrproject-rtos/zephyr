/*
 * Copyright (c) 2026 Nainaiu Rakhaine
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max30003

#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include "max30003.h"

LOG_MODULE_REGISTER(MAX30003, CONFIG_SENSOR_LOG_LEVEL);

#define MAX30003_PLL_TIMEOUT_MS 500
#define MAX30003_BURST_WORDS    8

static const struct {
	uint16_t sps;
	uint8_t fmstr;
	uint8_t rate;
} max30003_rates[] = {
	{125, 1, 2}, {128, 0, 2}, {200, 2, 2}, {250, 1, 1},
	{256, 0, 1}, {500, 1, 0}, {512, 0, 0},
};

static const uint8_t gain_vv[] = {20, 40, 80, 160};
static const uint8_t lead_off_na[] = {0, 5, 10, 20, 50, 100};
static const uint16_t lead_off_mv[] = {300, 400, 450, 500};
static const uint8_t data_ready_n[] = {1, 2, 4, 16};

static const struct max30003_calibration default_cal = {
	.bipolar = true,
	.amplitude_0_5mv = true,
	.freq = MAX30003_CAL_1HZ,
	.time_high = 0,
};

int max30003_reg_read(const struct device *dev, uint8_t reg, uint32_t *val)
{
	const struct max30003_config *cfg = dev->config;
	uint8_t tx[4] = {(uint8_t)((reg << 1) | 1U), 0, 0, 0};
	uint8_t rx[4];
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set txs = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rxs = {.buffers = &rx_buf, .count = 1};

	if (spi_transceive_dt(&cfg->spi, &txs, &rxs)) {
		return -EIO;
	}
	*val = sys_get_be24(&rx[1]);
	return 0;
}

int max30003_reg_write(const struct device *dev, uint8_t reg, uint32_t val)
{
	const struct max30003_config *cfg = dev->config;
	uint8_t tx[4] = {(uint8_t)(reg << 1)};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set txs = {.buffers = &tx_buf, .count = 1};

	sys_put_be24(val, &tx[1]);
	return spi_write_dt(&cfg->spi, &txs) ? -EIO : 0;
}

static int reg_write_verify(const struct device *dev, uint8_t reg, uint32_t val)
{
	uint32_t rb;
	int ret;

	ret = max30003_reg_write(dev, reg, val);
	if (ret == 0) {
		ret = max30003_reg_read(dev, reg, &rb);
	}
	if (ret == 0 && rb != val) {
		LOG_ERR("reg 0x%02x: wrote 0x%06x, read 0x%06x", reg, val, rb);
		ret = -EIO;
	}
	return ret;
}

static int find_u8(const uint8_t *t, size_t n, uint32_t v)
{
	for (size_t i = 0; i < n; i++) {
		if (t[i] == v) {
			return (int)i;
		}
	}
	return -EINVAL;
}

/* Code for 2, 4, 8 or 16 (averaging fields). */
static int avg_code(uint8_t v)
{
	return (v == 2 || v == 4 || v == 8 || v == 16) ? (int)(LOG2(v) - 1) : -EINVAL;
}

static uint32_t fmstr_hz(const struct max30003_config *cfg, uint8_t fmstr)
{
	/* FMSTR 0: FCLK; 1 and 2: FCLK * 625/640 (32000 Hz nominal). */
	return fmstr == 0 ? cfg->fclk_hz : (uint32_t)((uint64_t)cfg->fclk_hz * 625U / 640U);
}

static uint32_t cal_value(const struct max30003_calibration *cal)
{
	return MAX30003_CAL_EN_VCAL | (cal->bipolar ? MAX30003_CAL_VMODE : 0) |
	       (cal->amplitude_0_5mv ? MAX30003_CAL_VMAG : 0) |
	       FIELD_PREP(MAX30003_CAL_FCAL, cal->freq) |
	       (cal->time_high == 0 ? MAX30003_CAL_FIFTY :
				      FIELD_PREP(MAX30003_CAL_THIGH, cal->time_high));
}

/*
 * Check a settings set and turn it into register values. Logs the first
 * problem found. Touches no hardware.
 */
static int settings_to_regs(const struct max30003_settings *s, bool standby, bool calibrating,
			    struct max30003_regs *r)
{
	int rate = -EINVAL, gain, imag, vth, samp_it, pavg, ravg;
	int rbias = -EINVAL;
	uint32_t rtor_gain_code = 15;

	for (size_t i = 0; i < ARRAY_SIZE(max30003_rates); i++) {
		if (max30003_rates[i].sps == s->sample_rate) {
			rate = (int)i;
		}
	}
	gain = find_u8(gain_vv, ARRAY_SIZE(gain_vv), s->gain);
	imag = find_u8(lead_off_na, ARRAY_SIZE(lead_off_na), s->lead_off_current_na);
	samp_it = find_u8(data_ready_n, ARRAY_SIZE(data_ready_n), s->data_ready_interval);
	vth = -EINVAL;
	for (size_t i = 0; i < ARRAY_SIZE(lead_off_mv); i++) {
		if (lead_off_mv[i] == s->lead_off_threshold_mv) {
			vth = (int)i;
		}
	}
	switch (s->bias_mohm) {
	case 0:
	case 50:
		rbias = 0;
		break;
	case 100:
		rbias = 1;
		break;
	case 200:
		rbias = 2;
		break;
	default:
		break;
	}
	pavg = avg_code(s->rtor_peak_average);
	ravg = avg_code(s->rtor_interval_average);

	if (rate < 0 || gain < 0) {
		LOG_ERR("invalid %s", rate < 0 ? "sample rate" : "gain");
		return -EINVAL;
	}
	if (imag < 0 || vth < 0) {
		LOG_ERR("invalid lead-off %s", imag < 0 ? "current" : "threshold");
		return -EINVAL;
	}
	if (rbias < 0 || (unsigned int)s->low_pass > MAX30003_LPF_150HZ) {
		LOG_ERR("invalid %s", rbias < 0 ? "bias resistance" : "low-pass filter");
		return -EINVAL;
	}
	if ((unsigned int)s->fast_recovery > MAX30003_FAST_AUTO || s->fast_threshold > 63) {
		LOG_ERR("invalid fast recovery mode or threshold (0..63)");
		return -EINVAL;
	}
	if (s->rtor_window < 6 || s->rtor_window > 28 || (s->rtor_window & 1) != 0) {
		LOG_ERR("R-to-R window must be 6..28, even");
		return -EINVAL;
	}
	if (s->rtor_gain != 0) {
		if (s->rtor_gain > 16384 || !IS_POWER_OF_TWO(s->rtor_gain)) {
			LOG_ERR("R-to-R gain must be 0 (auto) or a power of 2 up to 16384");
			return -EINVAL;
		}
		rtor_gain_code = LOG2(s->rtor_gain);
	}
	if (pavg < 0 || ravg < 0) {
		LOG_ERR("R-to-R averaging must be 2, 4, 8 or 16");
		return -EINVAL;
	}
	if (s->rtor_peak_threshold < 1 || s->rtor_peak_threshold > 16 || s->rtor_holdoff > 63 ||
	    s->rtor_holdoff_scale > 7) {
		LOG_ERR("R-to-R peak threshold 1..16, hold-off 0..63, hold-off scale 0..7");
		return -EINVAL;
	}
	if (s->fifo_watermark < 1 || s->fifo_watermark > MAX30003_FIFO_DEPTH || samp_it < 0) {
		LOG_ERR("FIFO watermark 1..32, data-ready interval 1, 2, 4 or 16");
		return -EINVAL;
	}

	r->rate_idx = (uint8_t)rate;
	r->gen = FIELD_PREP(MAX30003_GEN_FMSTR, max30003_rates[rate].fmstr);
	if (standby) {
		/* ECG channel off, ultra-low-power lead-on detection on. */
		r->gen |= FIELD_PREP(MAX30003_GEN_EN_ULP_LON, 1);
	} else {
		r->gen |= MAX30003_GEN_EN_ECG;
		if (imag != 0) {
			r->gen |= FIELD_PREP(MAX30003_GEN_EN_DCLOFF, 1) |
				  (s->lead_off_reverse_polarity ? MAX30003_GEN_DCLOFF_IPOL : 0) |
				  FIELD_PREP(MAX30003_GEN_DCLOFF_IMAG, imag) |
				  FIELD_PREP(MAX30003_GEN_DCLOFF_VTH, vth);
		}
	}
	/*
	 * The chip forces EN_RBIAS off while ultra-low-power lead-on detection
	 * runs (seen on hardware), so leave the bias off in standby.
	 */
	if (!standby && s->bias_mohm != 0 && (s->bias_positive || s->bias_negative)) {
		r->gen |= FIELD_PREP(MAX30003_GEN_EN_RBIAS, 1) |
			  FIELD_PREP(MAX30003_GEN_RBIASV, rbias) |
			  (s->bias_positive ? MAX30003_GEN_RBIASP : 0) |
			  (s->bias_negative ? MAX30003_GEN_RBIASN : 0);
	}
	r->ecg = FIELD_PREP(MAX30003_ECG_RATE, max30003_rates[rate].rate) |
		 FIELD_PREP(MAX30003_ECG_GAIN, gain) | (s->high_pass ? MAX30003_ECG_DHPF : 0) |
		 FIELD_PREP(MAX30003_ECG_DLPF, s->low_pass);
	r->emux = (s->invert_polarity ? MAX30003_EMUX_POL : 0) |
		  (calibrating ? MAX30003_EMUX_CAL_SOURCES : 0);
	r->rtor1 = FIELD_PREP(MAX30003_RTOR1_WNDW, (s->rtor_window - 6) / 2) |
		   FIELD_PREP(MAX30003_RTOR1_GAIN, rtor_gain_code) |
		   (s->rtor_enable ? MAX30003_RTOR1_EN : 0) |
		   FIELD_PREP(MAX30003_RTOR1_PAVG, pavg) |
		   FIELD_PREP(MAX30003_RTOR1_PTSF, s->rtor_peak_threshold - 1);
	r->rtor2 = FIELD_PREP(MAX30003_RTOR2_HOFF, s->rtor_holdoff) |
		   FIELD_PREP(MAX30003_RTOR2_RAVG, ravg) |
		   FIELD_PREP(MAX30003_RTOR2_RHSF, s->rtor_holdoff_scale);
	/* RRINT clears on RTOR read; SAMP self-clears; FSTINT is a level. */
	r->mngr_int = FIELD_PREP(MAX30003_MNGR_INT_EFIT, s->fifo_watermark - 1) |
		      FIELD_PREP(MAX30003_MNGR_INT_CLR_RRINT, 1) | MAX30003_MNGR_INT_CLR_SAMP |
		      FIELD_PREP(MAX30003_MNGR_INT_SAMP_IT, samp_it);
	r->mngr_dyn = FIELD_PREP(MAX30003_MNGR_DYN_FAST, s->fast_recovery) |
		      FIELD_PREP(MAX30003_MNGR_DYN_FAST_TH, s->fast_threshold);
	return 0;
}

static int wait_pll_lock(const struct device *dev)
{
	int64_t end = k_uptime_get() + MAX30003_PLL_TIMEOUT_MS;
	uint32_t status;
	int ret;

	do {
		ret = max30003_reg_read(dev, MAX30003_REG_STATUS, &status);
		if (ret < 0) {
			return ret;
		}
		if ((status & MAX30003_INT_PLLINT) == 0) {
			return 0;
		}
		k_msleep(1);
	} while (k_uptime_get() < end);

	LOG_ERR("PLL not locked: is a 32.768 kHz clock present on FCLK?");
	return -ETIMEDOUT;
}

/* Restart conversions with an empty FIFO. */
static int restart_locked(const struct device *dev)
{
	struct max30003_data *data = dev->data;
	int ret;

	ret = max30003_reg_write(dev, MAX30003_REG_FIFO_RST, 0);
	ret = ret ? ret : max30003_reg_write(dev, MAX30003_REG_SYNCH, 0);
	data->have_sample = false;
	return ret;
}

/* Write a whole configuration, wait for the PLL and restart conversions. */
static int apply_locked(const struct device *dev, const struct max30003_settings *s,
			bool standby, bool calibrating, const struct max30003_calibration *cal)
{
	struct max30003_data *data = dev->data;
	struct max30003_regs r;
	int ret;

	ret = settings_to_regs(s, standby, calibrating, &r);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Seen on hardware: while the ECG channel is off, CNFG_EMUX reads back
	 * its reset value (inputs open) whatever was written. So going into
	 * standby, write CNFG_GEN (channel off) last and do not verify EMUX in
	 * standby. Leaving standby, CNFG_GEN goes first so the channel is on
	 * before EMUX is written and verified.
	 */
	int (*emux_write)(const struct device *, uint8_t, uint32_t) =
		standby ? max30003_reg_write : reg_write_verify;

	ret = standby ? 0 : reg_write_verify(dev, MAX30003_REG_CNFG_GEN, r.gen);
	if (calibrating) {
		/* Open the electrode switches before enabling the test source. */
		ret = ret ? ret : emux_write(dev, MAX30003_REG_CNFG_EMUX,
					     (r.emux & ~MAX30003_EMUX_CAL_SOURCES) |
						     MAX30003_EMUX_OPENP | MAX30003_EMUX_OPENN);
		ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_CNFG_CAL, cal_value(cal));
	} else {
		ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_CNFG_CAL, 0);
	}
	ret = ret ? ret : emux_write(dev, MAX30003_REG_CNFG_EMUX, r.emux);
	ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_CNFG_ECG, r.ecg);
	ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_CNFG_RTOR1, r.rtor1);
	ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_CNFG_RTOR2, r.rtor2);
	ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_MNGR_INT, r.mngr_int);
	ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_MNGR_DYN, r.mngr_dyn);
	if (standby) {
		ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_CNFG_GEN, r.gen);
	}
	if (ret == 0 && !standby) {
		ret = wait_pll_lock(dev);
	}
	if (ret == 0) {
		bool rate_changed = r.rate_idx != data->rate_idx;

		data->cur = *s;
		data->rate_idx = r.rate_idx;
		data->standby = standby;
		data->calibrating = calibrating;
		if (calibrating) {
			data->cal = *cal;
		}
		if (rate_changed || !s->rtor_enable) {
			data->have_rr = false;
		}
		ret = restart_locked(dev);
	}
	return ret;
}

int max30003_get_settings(const struct device *dev, struct max30003_settings *settings)
{
	struct max30003_data *data = dev->data;

	if (settings == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	*settings = data->cur;
	k_mutex_unlock(&data->lock);
	return 0;
}

int max30003_set_settings(const struct device *dev, const struct max30003_settings *settings)
{
	struct max30003_data *data = dev->data;
	int ret;

	if (settings == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = apply_locked(dev, settings, data->standby, data->calibrating, &data->cal);
	k_mutex_unlock(&data->lock);
	return ret;
}

int max30003_calibration_start(const struct device *dev, const struct max30003_calibration *cal)
{
	struct max30003_data *data = dev->data;
	int ret;

	if (cal == NULL) {
		cal = &default_cal;
	}
	if ((unsigned int)cal->freq > MAX30003_CAL_0_015625HZ || cal->time_high > 2047) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = apply_locked(dev, &data->cur, data->standby, true, cal);
	k_mutex_unlock(&data->lock);
	return ret;
}

int max30003_calibration_stop(const struct device *dev)
{
	struct max30003_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = apply_locked(dev, &data->cur, data->standby, false, NULL);
	k_mutex_unlock(&data->lock);
	return ret;
}

int max30003_standby(const struct device *dev, bool enable)
{
	struct max30003_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = apply_locked(dev, &data->cur, enable, data->calibrating, &data->cal);
	k_mutex_unlock(&data->lock);
	return ret;
}

uint32_t max30003_master_clock_hz(const struct device *dev)
{
	struct max30003_data *data = dev->data;

	return fmstr_hz(dev->config, max30003_rates[data->rate_idx].fmstr);
}

int max30003_handle_overflow(const struct device *dev)
{
	LOG_WRN("%s: FIFO overflow, samples lost", dev->name);
	/* Datasheet: on overflow, reset the FIFO (FIFO_RST). */
	return max30003_reg_write(dev, MAX30003_REG_FIFO_RST, 0);
}

int max30003_update_rr_locked(const struct device *dev)
{
	struct max30003_data *data = dev->data;
	uint32_t rtor;
	int ret;

	/* Reading RTOR also clears RRINT (CLR_RRINT = 1). */
	ret = max30003_reg_read(dev, MAX30003_REG_RTOR, &rtor);
	if (ret < 0) {
		return ret;
	}
	rtor = (rtor >> 10) & 0x3FFF;
	if (rtor != 0) {
		/* One RTOR count is 256 master-clock periods (~7.8 ms). */
		uint32_t f = max30003_master_clock_hz(dev);

		data->rr_us = (uint32_t)((uint64_t)rtor * 256U * 1000000U / f);
		data->have_rr = true;
	}
	return 0;
}

int max30003_read_status(const struct device *dev, uint32_t *status)
{
	struct max30003_data *data = dev->data;
	uint32_t v;
	int ret;

	if (status == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = max30003_reg_read(dev, MAX30003_REG_STATUS, &v);
	if (ret == 0) {
		*status = v | data->status_cache;
		data->status_cache = 0;
		if ((v & MAX30003_INT_RRINT) && data->cur.rtor_enable) {
			ret = max30003_update_rr_locked(dev);
		}
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

static int fifo_read_locked(const struct device *dev, int32_t *samples, uint8_t *flags,
			    size_t max_samples, bool *overflow)
{
	const struct max30003_config *cfg = dev->config;
	struct max30003_data *data = dev->data;
	uint8_t tx = (uint8_t)((MAX30003_REG_ECG_FIFO_BURST << 1) | 1U);
	uint8_t rx[MAX30003_BURST_WORDS * 3];
	size_t done = 0;
	bool end = false;

	if (overflow != NULL) {
		*overflow = false;
	}

	while (!end && done < max_samples) {
		size_t chunk = MIN(max_samples - done, MAX30003_BURST_WORDS);
		const struct spi_buf tx_buf = {.buf = &tx, .len = 1};
		const struct spi_buf rx_bufs[2] = {
			{.buf = NULL, .len = 1},
			{.buf = rx, .len = chunk * 3},
		};
		const struct spi_buf_set txs = {.buffers = &tx_buf, .count = 1};
		const struct spi_buf_set rxs = {.buffers = rx_bufs, .count = 2};

		if (spi_transceive_dt(&cfg->spi, &txs, &rxs)) {
			return -EIO;
		}

		for (size_t i = 0; i < chunk && !end; i++) {
			uint32_t word = sys_get_be24(&rx[i * 3]);
			uint8_t etag = max30003_word_etag(word);

			switch (etag) {
			case MAX30003_ETAG_VALID_EOF:
			case MAX30003_ETAG_FAST_EOF:
				end = true;
				__fallthrough;
			case MAX30003_ETAG_VALID:
			case MAX30003_ETAG_FAST:
				if (flags != NULL) {
					flags[done] = (etag == MAX30003_ETAG_FAST ||
						       etag == MAX30003_ETAG_FAST_EOF)
							      ? MAX30003_FLAG_FAST
							      : 0;
				}
				samples[done++] = max30003_word_sample(word);
				break;
			case MAX30003_ETAG_OVERFLOW:
				if (overflow != NULL) {
					*overflow = true;
				}
				end = true;
				if (max30003_handle_overflow(dev) < 0) {
					return -EIO;
				}
				break;
			default: /* EMPTY or reserved: nothing more stored */
				end = true;
				break;
			}
		}
	}

	if (done > 0) {
		data->raw = samples[done - 1];
		data->have_sample = true;
	}
	return (int)done;
}

int max30003_fifo_read(const struct device *dev, int32_t *samples, uint8_t *flags,
		       size_t max_samples, bool *overflow)
{
	struct max30003_data *data = dev->data;
	int ret;

	if (samples == NULL && max_samples > 0) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = fifo_read_locked(dev, samples, flags, max_samples, overflow);
	k_mutex_unlock(&data->lock);
	return ret;
}

int max30003_fifo_flush(const struct device *dev)
{
	struct max30003_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = restart_locked(dev);
	k_mutex_unlock(&data->lock);
	return ret;
}

int max30003_gain(const struct device *dev)
{
	struct max30003_data *data = dev->data;

	return data->cur.gain;
}

static int read_status_locked(const struct device *dev)
{
	struct max30003_data *data = dev->data;
	uint32_t status;
	int ret;

	ret = max30003_reg_read(dev, MAX30003_REG_STATUS, &status);
	if (ret < 0) {
		return ret;
	}
	/* Flags already consumed by the interrupt handler are kept in the cache. */
	status |= data->status_cache & (MAX30003_INT_RRINT | MAX30003_STATUS_LDOFF);
	data->status_cache &= ~(MAX30003_INT_RRINT | MAX30003_STATUS_LDOFF);

	data->lead_off = data->cur.lead_off_current_na != 0
				 ? (uint8_t)(status & MAX30003_STATUS_LDOFF)
				 : 0;

	if (data->cur.rtor_enable && (status & MAX30003_INT_RRINT)) {
		ret = max30003_update_rr_locked(dev);
	}
	return ret;
}

static int max30003_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct max30003_data *data = dev->data;
	int32_t tmp[MAX30003_BURST_WORDS];
	int ret, total = 0;

	ARG_UNUSED(chan);

	k_mutex_lock(&data->lock, K_FOREVER);
	/* Drain the FIFO, keeping the newest sample. */
	do {
		ret = fifo_read_locked(dev, tmp, NULL, ARRAY_SIZE(tmp), NULL);
		total += MAX(ret, 0);
	} while (ret == (int)ARRAY_SIZE(tmp) && total <= MAX30003_FIFO_DEPTH);
	if (ret >= 0) {
		ret = read_status_locked(dev);
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

static int max30003_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max30003_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_VOLTAGE:
		if (!data->have_sample) {
			return -ENODATA;
		}
		return sensor_value_from_micro(val,
					       max30003_counts_to_uv(data->raw, data->cur.gain));
	case SENSOR_CHAN_MAX30003_ECG_RAW:
		if (!data->have_sample) {
			return -ENODATA;
		}
		val->val1 = data->raw;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_MAX30003_RR_INTERVAL:
		if (!data->have_rr) {
			return -ENODATA;
		}
		/* val1 = ms, val2 = millionths of a ms. */
		val->val1 = (int32_t)(data->rr_us / 1000U);
		val->val2 = (int32_t)(data->rr_us % 1000U) * 1000;
		return 0;
	case SENSOR_CHAN_MAX30003_HEART_RATE:
		if (!data->have_rr || data->rr_us == 0) {
			return -ENODATA;
		}
		return sensor_value_from_micro(val, (int64_t)60000000LL * 1000000 / data->rr_us);
	case SENSOR_CHAN_MAX30003_LEAD_OFF:
		val->val1 = data->lead_off;
		val->val2 = 0;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int max30003_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct max30003_data *data = dev->data;
	struct max30003_settings s;
	int ret = 0;

	ARG_UNUSED(chan);
	k_mutex_lock(&data->lock, K_FOREVER);
	s = data->cur;

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val1 <= 0 || val->val1 > UINT16_MAX || val->val2 != 0) {
			ret = -EINVAL;
		}
		s.sample_rate = (uint16_t)val->val1;
		break;
	case SENSOR_ATTR_GAIN:
		if (val->val1 <= 0 || val->val1 > UINT8_MAX || val->val2 != 0) {
			ret = -EINVAL;
		}
		s.gain = (uint8_t)val->val1;
		break;
	case SENSOR_ATTR_MAX30003_CALIBRATION:
		ret = apply_locked(dev, &s, data->standby, val->val1 != 0, &default_cal);
		k_mutex_unlock(&data->lock);
		return ret;
	default:
		ret = -ENOTSUP;
		break;
	}

	if (ret == 0) {
		ret = apply_locked(dev, &s, data->standby, data->calibrating, &data->cal);
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

static int max30003_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct max30003_data *data = dev->data;

	ARG_UNUSED(chan);
	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		val->val1 = data->cur.sample_rate;
		break;
	case SENSOR_ATTR_GAIN:
		val->val1 = data->cur.gain;
		break;
	case SENSOR_ATTR_MAX30003_CALIBRATION:
		val->val1 = data->calibrating;
		break;
	default:
		return -ENOTSUP;
	}
	val->val2 = 0;
	return 0;
}

static DEVICE_API(sensor, max30003_api) = {
	.sample_fetch = max30003_sample_fetch,
	.channel_get = max30003_channel_get,
	.attr_set = max30003_attr_set,
	.attr_get = max30003_attr_get,
#ifdef CONFIG_MAX30003_TRIGGER
	.trigger_set = max30003_trigger_set,
#endif
};

static int max30003_chip_init(const struct device *dev)
{
	const struct max30003_config *cfg = dev->config;
	struct max30003_data *data = dev->data;
	uint32_t info, dummy;
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	if (cfg->has_fclk_pwm) {
		uint32_t period = (uint32_t)((NSEC_PER_SEC + cfg->fclk_hz / 2U) / cfg->fclk_hz);

		ret = pwm_set_dt(&cfg->fclk_pwm, period, period / 2U);
		if (ret < 0) {
			LOG_ERR("FCLK PWM start failed (%d)", ret);
			return ret;
		}
	}
#endif

	ret = max30003_reg_write(dev, MAX30003_REG_SW_RST, 0);
	if (ret < 0) {
		LOG_ERR("no response on SPI");
		return -ENODEV;
	}
	k_msleep(10);
	/* The first transfer after a reset returns stale data: discard it. */
	(void)max30003_reg_read(dev, MAX30003_REG_NO_OP, &dummy);

	ret = max30003_reg_read(dev, MAX30003_REG_INFO, &info);
	if (ret < 0 || FIELD_GET(MAX30003_INFO_ID_MASK, info) != MAX30003_INFO_ID) {
		LOG_ERR("INFO 0x%06x: not a MAX30003 (check SPI wiring and mode)", info);
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->standby = false;
	data->calibrating = false;
	ret = reg_write_verify(dev, MAX30003_REG_EN_INT,
			       FIELD_PREP(MAX30003_EN_INT_INTB_TYPE, MAX30003_INTB_OD_PULLUP));
	ret = ret ? ret : reg_write_verify(dev, MAX30003_REG_EN_INT2, 0);
	ret = ret ? ret : apply_locked(dev, &cfg->defaults, false, false, NULL);
#ifdef CONFIG_MAX30003_TRIGGER
	/* Re-enable the triggers that were set before a re-init. */
	ret = ret ? ret : max30003_write_int_enable_locked(dev);
#endif
	k_mutex_unlock(&data->lock);
	if (ret == 0) {
		LOG_DBG("%s: MAX30003 rev %u ready, %u sps", dev->name,
			(unsigned int)((info >> 16) & 0xF), data->cur.sample_rate);
	}
	return ret;
}

static int max30003_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct max30003_data *data = dev->data;
	struct max30003_regs r;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
		k_mutex_lock(&data->lock, K_FOREVER);
		(void)settings_to_regs(&data->cur, data->standby, data->calibrating, &r);
		if (action == PM_DEVICE_ACTION_SUSPEND) {
			/* ECG channel and lead-on detection off: lowest power. */
			r.gen &= ~(MAX30003_GEN_EN_ECG | MAX30003_GEN_EN_ULP_LON);
		}
		ret = reg_write_verify(dev, MAX30003_REG_CNFG_GEN, r.gen);
		if (ret == 0 && action == PM_DEVICE_ACTION_RESUME) {
			ret = restart_locked(dev);
		}
		k_mutex_unlock(&data->lock);
		return ret;
	case PM_DEVICE_ACTION_TURN_ON:
		return max30003_chip_init(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int max30003_init(const struct device *dev)
{
	const struct max30003_config *cfg = dev->config;
	struct max30003_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	data->cur = cfg->defaults;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus %s not ready", cfg->spi.bus->name);
		return -ENODEV;
	}
	if (cfg->has_fclk_pwm && !pwm_is_ready_dt(&cfg->fclk_pwm)) {
		LOG_ERR("FCLK PWM not ready");
		return -ENODEV;
	}

#ifdef CONFIG_MAX30003_TRIGGER
	ret = max30003_init_interrupt(dev);
	if (ret < 0) {
		return ret;
	}
#endif

	ret = pm_device_driver_init(dev, max30003_pm_action);
	if (ret < 0) {
		LOG_ERR("%s: init failed (%d)", dev->name, ret);
	}
	return ret;
}

/* SPI_OP_MODE_CONTROLLER replaced SPI_OP_MODE_MASTER in Zephyr 4.3. */
#ifndef SPI_OP_MODE_CONTROLLER
#define SPI_OP_MODE_CONTROLLER SPI_OP_MODE_MASTER
#endif

/* SPI mode 0: data latched on SCLK rising edge, MSB first. */
#define MAX30003_SPI_OP (SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

/* input-bias enum index -> megaohms: none, 50, 100, 200. */
#define MAX30003_BIAS_MOHM(n)                                                                      \
	(DT_INST_ENUM_IDX(n, input_bias) == 0 ? 0 : (25 << DT_INST_ENUM_IDX(n, input_bias)))

#ifdef CONFIG_MAX30003_TRIGGER
#define MAX30003_INT_GPIO(n) .int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),
#else
#define MAX30003_INT_GPIO(n)
#endif

#define MAX30003_DEFINE(n)                                                                         \
	static struct max30003_data max30003_data_##n;                                             \
	static const struct max30003_config max30003_config_##n = {                                \
		.spi = SPI_DT_SPEC_INST_GET(n, MAX30003_SPI_OP),                                   \
		MAX30003_INT_GPIO(n)                                                               \
		.fclk_pwm = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pwms),                            \
					(PWM_DT_SPEC_INST_GET(n)), ({0})),                         \
		.has_fclk_pwm = DT_INST_NODE_HAS_PROP(n, pwms),                                    \
		.fclk_hz = DT_INST_PROP(n, fclk_frequency),                                        \
		.defaults = {                                                                      \
			.sample_rate = DT_INST_PROP(n, sample_rate),                               \
			.gain = DT_INST_PROP(n, gain),                                             \
			.high_pass = DT_INST_PROP(n, digital_high_pass),                           \
			.low_pass = DT_INST_ENUM_IDX(n, low_pass_filter),                          \
			.invert_polarity = DT_INST_PROP(n, invert_polarity),                       \
			.bias_mohm = MAX30003_BIAS_MOHM(n),                                        \
			.bias_positive = DT_INST_ENUM_IDX(n, input_bias_inputs) != 2,              \
			.bias_negative = DT_INST_ENUM_IDX(n, input_bias_inputs) != 1,              \
			.lead_off_current_na = DT_INST_PROP(n, dc_lead_off_current_na),            \
			.lead_off_threshold_mv = DT_INST_PROP(n, dc_lead_off_threshold_mv),        \
			.lead_off_reverse_polarity =                                               \
				DT_INST_PROP(n, dc_lead_off_reverse_polarity),                     \
			.fast_recovery = DT_INST_ENUM_IDX(n, fast_recovery),                       \
			.fast_threshold = DT_INST_PROP(n, fast_recovery_threshold),                \
			.rtor_enable = DT_INST_PROP(n, rtor_enable),                               \
			.rtor_window = DT_INST_PROP(n, rtor_window),                               \
			.rtor_gain = DT_INST_PROP(n, rtor_gain),                                   \
			.rtor_peak_average = DT_INST_PROP(n, rtor_peak_average),                   \
			.rtor_peak_threshold = DT_INST_PROP(n, rtor_peak_threshold),               \
			.rtor_holdoff = DT_INST_PROP(n, rtor_holdoff),                             \
			.rtor_interval_average = DT_INST_PROP(n, rtor_interval_average),           \
			.rtor_holdoff_scale = DT_INST_PROP(n, rtor_holdoff_scale),                 \
			.fifo_watermark = DT_INST_PROP(n, fifo_watermark),                         \
			.data_ready_interval = DT_INST_PROP(n, data_ready_interval),               \
		},                                                                                 \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, max30003_pm_action);                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(n, max30003_init, PM_DEVICE_DT_INST_GET(n),                   \
				     &max30003_data_##n, &max30003_config_##n, POST_KERNEL,        \
				     CONFIG_SENSOR_INIT_PRIORITY, &max30003_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30003_DEFINE)
