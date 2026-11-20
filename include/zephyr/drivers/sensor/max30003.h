/*
 * Copyright (c) 2026 Nainaiu Rakhaine
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the MAX30003 ECG analog front end.
 *
 * Standard sensor API: sensor_sample_fetch() then sensor_channel_get() with
 * SENSOR_CHAN_VOLTAGE (input-referred volts) or the custom channels below.
 *
 * For continuous recording use max30003_fifo_read(), which returns every
 * sample in order with a per-sample flag for fast-recovery samples.
 *
 * Every chip setting can be changed at runtime with max30003_get_settings()
 * and max30003_set_settings(). The devicetree gives the start-up values.
 *
 * @ingroup max30003_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30003_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30003_H_

/**
 * @brief Maxim (Analog Devices) MAX30003 ECG analog front end
 * @defgroup max30003_interface MAX30003
 * @ingroup sensor_interface_ext_adi
 * @since 4.6
 * @version 0.1.0
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ECG FIFO depth in samples. */
#define MAX30003_FIFO_DEPTH 32

/** Custom channels. */
enum sensor_channel_max30003 {
	/** Newest ECG sample, signed ADC counts (-131072 .. 131071) in val1. */
	SENSOR_CHAN_MAX30003_ECG_RAW = SENSOR_CHAN_PRIV_START,
	/** Last R-to-R interval in milliseconds. */
	SENSOR_CHAN_MAX30003_RR_INTERVAL,
	/** Heart rate in beats per minute, from the last R-to-R interval. */
	SENSOR_CHAN_MAX30003_HEART_RATE,
	/** DC lead-off status: MAX30003_LEAD_OFF_* bits in val1. */
	SENSOR_CHAN_MAX30003_LEAD_OFF,
};

/** Custom attributes. */
enum sensor_attribute_max30003 {
	/**
	 * Internal calibration source: val1 = 1 disconnects the electrodes and
	 * applies a 1 Hz, 0.5 mV square wave; val1 = 0 reconnects them.
	 * max30003_calibration_start() gives full control.
	 */
	SENSOR_ATTR_MAX30003_CALIBRATION = SENSOR_ATTR_PRIV_START,
};

/**
 * Custom triggers. Level conditions (lead-off, lead-on, fast recovery, PLL
 * unlock) fire once when they start; the driver re-arms them when the
 * condition clears.
 */
enum sensor_trigger_type_max30003 {
	/** A new R-to-R interval is ready (RRINT). */
	SENSOR_TRIG_MAX30003_RR = SENSOR_TRIG_PRIV_START,
	/** DC lead-off detected (DCLOFFINT). */
	SENSOR_TRIG_MAX30003_LEAD_OFF,
	/** Electrodes attached while in standby (LONINT). */
	SENSOR_TRIG_MAX30003_LEAD_ON,
	/** Fast recovery mode started (FSTINT). */
	SENSOR_TRIG_MAX30003_FAST_RECOVERY,
	/** PLL lost lock: check FCLK (PLLINT). */
	SENSOR_TRIG_MAX30003_PLL_UNLOCK,
};

/**
 * @name Status bits
 * STATUS register layout, returned by max30003_read_status().
 * @{
 */
#define MAX30003_STATUS_EINT      BIT(23) /**< FIFO reached the watermark */
#define MAX30003_STATUS_EOVF      BIT(22) /**< FIFO overflow */
#define MAX30003_STATUS_FSTINT    BIT(21) /**< fast recovery active */
#define MAX30003_STATUS_DCLOFFINT BIT(20) /**< DC lead-off detected */
#define MAX30003_STATUS_LONINT    BIT(11) /**< ultra-low-power lead-on */
#define MAX30003_STATUS_RRINT     BIT(10) /**< R-to-R interval ready */
#define MAX30003_STATUS_SAMP      BIT(9)  /**< sample synchronisation pulse */
#define MAX30003_STATUS_PLLINT    BIT(8)  /**< PLL unlocked */
#define MAX30003_STATUS_LDOFF_PH  BIT(3)  /**< ECGP above VMID + threshold */
#define MAX30003_STATUS_LDOFF_PL  BIT(2)  /**< ECGP below VMID - threshold */
#define MAX30003_STATUS_LDOFF_NH  BIT(1)  /**< ECGN above VMID + threshold */
#define MAX30003_STATUS_LDOFF_NL  BIT(0)  /**< ECGN below VMID - threshold */
/** @} */

/** @name Lead-off bits (SENSOR_CHAN_MAX30003_LEAD_OFF) @{ */
#define MAX30003_LEAD_OFF_P_HIGH MAX30003_STATUS_LDOFF_PH
#define MAX30003_LEAD_OFF_P_LOW  MAX30003_STATUS_LDOFF_PL
#define MAX30003_LEAD_OFF_N_HIGH MAX30003_STATUS_LDOFF_NH
#define MAX30003_LEAD_OFF_N_LOW  MAX30003_STATUS_LDOFF_NL
/** @} */

/** @name FIFO ETAG values @{ */
#define MAX30003_ETAG_VALID      0
#define MAX30003_ETAG_FAST       1
#define MAX30003_ETAG_VALID_EOF  2
#define MAX30003_ETAG_FAST_EOF   3
#define MAX30003_ETAG_EMPTY      6
#define MAX30003_ETAG_OVERFLOW   7
/** @} */

/** Per-sample flag from max30003_fifo_read(): taken during fast recovery. */
#define MAX30003_FLAG_FAST BIT(0)

/** Digital low-pass filter. */
enum max30003_low_pass {
	MAX30003_LPF_BYPASS,
	MAX30003_LPF_40HZ,
	MAX30003_LPF_100HZ,
	MAX30003_LPF_150HZ,
};

/** Fast recovery mode (MNGR_DYN FAST). */
enum max30003_fast_recovery {
	/** Normal operation. */
	MAX30003_FAST_OFF,
	/** Fast recovery forced on until set back to OFF. */
	MAX30003_FAST_MANUAL,
	/** Fast recovery starts automatically after saturation. */
	MAX30003_FAST_AUTO,
};

/** Calibration source frequency (with a 32768 Hz master clock). */
enum max30003_cal_freq {
	MAX30003_CAL_256HZ,
	MAX30003_CAL_64HZ,
	MAX30003_CAL_16HZ,
	MAX30003_CAL_4HZ,
	MAX30003_CAL_1HZ,
	MAX30003_CAL_0_25HZ,
	MAX30003_CAL_0_0625HZ,
	MAX30003_CAL_0_015625HZ,
};

/**
 * @brief Complete chip configuration.
 *
 * max30003_set_settings() checks the whole set before it writes anything.
 */
struct max30003_settings {
	/** Samples per second: 125, 128, 200, 250, 256, 500 or 512. */
	uint16_t sample_rate;
	/** ECG gain in V/V: 20, 40, 80 or 160. */
	uint8_t gain;
	/** 0.5 Hz digital high-pass filter. */
	bool high_pass;
	/** Digital low-pass filter. */
	enum max30003_low_pass low_pass;
	/** Swap ECGP and ECGN. */
	bool invert_polarity;

	/** Input bias resistance in megaohms: 0 (off), 50, 100 or 200. */
	uint8_t bias_mohm;
	/** Apply the bias to ECGP. */
	bool bias_positive;
	/** Apply the bias to ECGN. */
	bool bias_negative;

	/** DC lead-off current in nA: 0 (off), 5, 10, 20, 50 or 100. */
	uint8_t lead_off_current_na;
	/** Lead-off threshold, VMID +/- this many mV: 300, 400, 450 or 500. */
	uint16_t lead_off_threshold_mv;
	/** false: ECGP pulled up, ECGN pulled down. true: the reverse. */
	bool lead_off_reverse_polarity;

	/** Fast recovery mode. */
	enum max30003_fast_recovery fast_recovery;
	/** Automatic fast recovery threshold, in units of 2048 counts (0 to 63). */
	uint8_t fast_threshold;

	/** On-chip R-to-R (QRS) detector. */
	bool rtor_enable;
	/** Averaging window, in R-to-R periods of about 7.8 ms: 6 to 28, even. */
	uint8_t rtor_window;
	/** R-to-R gain: 0 for automatic, or 1, 2, 4 ... 16384. */
	uint16_t rtor_gain;
	/** Peak averaging: 2, 4, 8 or 16 beats. */
	uint8_t rtor_peak_average;
	/** Detection threshold, sixteenths of the peak average: 1 to 16. */
	uint8_t rtor_peak_threshold;
	/** Minimum hold-off after a beat, in R-to-R periods: 0 to 63. */
	uint8_t rtor_holdoff;
	/** Interval averaging for the adaptive hold-off: 2, 4, 8 or 16 beats. */
	uint8_t rtor_interval_average;
	/** Adaptive hold-off scale, eighths of the average interval: 0 (off) to 7. */
	uint8_t rtor_holdoff_scale;

	/** Unread samples that raise SENSOR_TRIG_FIFO_WATERMARK (1 to 32). */
	uint8_t fifo_watermark;
	/** SENSOR_TRIG_DATA_READY every N samples: 1, 2, 4 or 16. */
	uint8_t data_ready_interval;
};

/** Calibration source settings for max30003_calibration_start(). */
struct max30003_calibration {
	/** true: +/- amplitude around VMID. false: 0 to +amplitude. */
	bool bipolar;
	/** true: 0.5 mV. false: 0.25 mV. */
	bool amplitude_0_5mv;
	/** Square wave frequency. */
	enum max30003_cal_freq freq;
	/**
	 * Time high (CNFG_CAL THIGH, 0 to 2047, see datasheet).
	 * 0 selects a 50 % duty cycle.
	 */
	uint16_t time_high;
};

/** @brief Read the current configuration. */
int max30003_get_settings(const struct device *dev, struct max30003_settings *settings);

/**
 * @brief Apply a new configuration.
 *
 * The whole set is checked first; on -EINVAL nothing is written and the log
 * says which value is wrong. On success conversions restart with an empty
 * FIFO.
 */
int max30003_set_settings(const struct device *dev, const struct max30003_settings *settings);

/**
 * @brief Read the STATUS register.
 *
 * Includes flags consumed by the trigger handler since the previous call.
 *
 * @param dev MAX30003 device.
 * @param status MAX30003_STATUS_* bits.
 */
int max30003_read_status(const struct device *dev, uint32_t *status);

/**
 * @brief Read ECG samples from the FIFO.
 *
 * Reads until the FIFO is empty or @p max_samples samples were read. On
 * overflow the FIFO is reset; the samples before the overflow are returned.
 *
 * @param dev MAX30003 device.
 * @param samples Output, signed ADC counts.
 * @param flags Optional (may be NULL). MAX30003_FLAG_* per sample.
 * @param max_samples Capacity of @p samples (and @p flags).
 * @param overflow Optional. Set to true if samples were lost.
 * @return Number of samples read, or a negative errno code.
 */
int max30003_fifo_read(const struct device *dev, int32_t *samples, uint8_t *flags,
		       size_t max_samples, bool *overflow);

/** @brief Empty the FIFO and restart conversions (FIFO_RST + SYNCH). */
int max30003_fifo_flush(const struct device *dev);

/** @brief Active gain in V/V. */
int max30003_gain(const struct device *dev);

/**
 * @brief Apply the internal calibration signal.
 *
 * The electrodes are disconnected and the calibration source drives the
 * inputs. Use it for production test or to check the signal chain.
 *
 * @param dev MAX30003 device.
 * @param cal Source settings, or NULL for 1 Hz, 0.5 mV, bipolar, 50 %.
 */
int max30003_calibration_start(const struct device *dev, const struct max30003_calibration *cal);

/** @brief Remove the calibration signal and reconnect the electrodes. */
int max30003_calibration_stop(const struct device *dev);

/**
 * @brief Standby with lead-on detection.
 *
 * @p enable = true powers the ECG channel down (lowest power) and turns on
 * ultra-low-power lead-on detection: SENSOR_TRIG_MAX30003_LEAD_ON fires and
 * MAX30003_STATUS_LONINT is set when the electrodes touch the skin.
 * @p enable = false powers the ECG channel up and restarts conversions.
 */
int max30003_standby(const struct device *dev, bool enable);

/**
 * @brief Master clock (FMSTR) in Hz for the active sample rate.
 *
 * One R-to-R period is 256 master clock cycles.
 */
uint32_t max30003_master_clock_hz(const struct device *dev);

/**
 * @brief Convert ADC counts to input-referred microvolts.
 *
 * V = counts * VREF / (2^17 * gain), VREF = 1 V.
 */
static inline int32_t max30003_counts_to_uv(int32_t counts, int gain)
{
	return (int32_t)(((int64_t)counts * 1000000) / ((int64_t)131072 * gain));
}

/** @brief Signed 18-bit ECG sample from a 24-bit FIFO word (bits 23:6). */
static inline int32_t max30003_word_sample(uint32_t word)
{
	int32_t v = (int32_t)((word >> 6) & 0x3FFFF);

	return (v & 0x20000) ? v - 0x40000 : v;
}

/** @brief ETAG of a 24-bit FIFO word. */
static inline uint8_t max30003_word_etag(uint32_t word)
{
	return (word >> 3) & 0x7;
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX30003_H_ */
