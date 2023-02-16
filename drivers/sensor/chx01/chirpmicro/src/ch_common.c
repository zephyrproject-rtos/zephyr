/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file ch_common.c
 *
 * @brief Chirp SonicLib API function common implementations
 *
 * This file contains standard implementations of functions required to support the
 * SonicLib API.  The sensor firmware, in it's init routine, specifies which of these
 * common implementations should be used by initializing a set of function pointers.
 * These pointers, contained in the ch_api_funcs_t structure within the device descriptor,
 * can either direct the API calls to the functions in this file or to firmware-specific
 * equivalents that are supplied as part of the sensor firmware release.
 *
 */

#include <zephyr/logging/log.h>

#include "soniclib.h"
#include "ch_common.h"
#include "chirp_bsp.h"
#include "ch_math_utils.h"

LOG_MODULE_REGISTER(CHIRPMICRO, CONFIG_SENSOR_LOG_LEVEL);

/* Local definitions */
/* number of I/Q samples to read at a time */
#define CH_IQ_SAMPLES_PER_READ 64

/* Forward references */
static uint8_t get_sample_data(ch_dev_t *dev_ptr, ch_iq_sample_t *buf_ptr, uint16_t start_sample,
			       uint16_t num_samples, ch_io_mode_t mode,
			       uint8_t sample_size_in_byte);

/* Functions */

uint8_t ch_common_set_mode(ch_dev_t *dev_ptr, ch_mode_t mode)
{
	uint8_t ret_val = 0;
	uint8_t opmode_reg;
	uint8_t period_reg;
	uint8_t tick_interval_reg;
	uint16_t max_tick_interval;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		opmode_reg = CH101_COMMON_REG_OPMODE;
		period_reg = CH101_COMMON_REG_PERIOD;
		tick_interval_reg = CH101_COMMON_REG_TICK_INTERVAL;
		max_tick_interval = CH101_MAX_TICK_INTERVAL;
	} else {
		opmode_reg = CH201_COMMON_REG_OPMODE;
		period_reg = CH201_COMMON_REG_PERIOD;
		tick_interval_reg = CH201_COMMON_REG_TICK_INTERVAL;
		max_tick_interval = CH201_MAX_TICK_INTERVAL;
	}

	if (dev_ptr->sensor_connected) {
		switch (mode) {
		case CH_MODE_IDLE:
			LOG_DBG("Setting mode to IDLE");
			chdrv_write_byte(dev_ptr, period_reg, 0);
			chdrv_write_word(dev_ptr, tick_interval_reg, max_tick_interval);
			chdrv_write_byte(dev_ptr, opmode_reg, CH_MODE_IDLE);
			break;

		case CH_MODE_FREERUN:
			LOG_DBG("Setting mode to FREERUN");
			ch_set_sample_interval(dev_ptr, dev_ptr->sample_interval);
			chdrv_write_byte(dev_ptr, opmode_reg, CH_MODE_FREERUN);
			break;

		case CH_MODE_TRIGGERED_TX_RX:
			LOG_DBG("Setting mode to TRIGGERED_TX_RX");
			chdrv_write_byte(dev_ptr, opmode_reg, CH_MODE_TRIGGERED_TX_RX);
			break;

		case CH_MODE_TRIGGERED_RX_ONLY:
			LOG_DBG("Setting mode to TRIGGERED_RX_ONLY");
			chdrv_write_byte(dev_ptr, opmode_reg, CH_MODE_TRIGGERED_RX_ONLY);
			break;

		default:
			/* return non-zero to indicate error */
			ret_val = RET_ERR;
			break;
		}
	}

	return ret_val;
}

uint8_t ch_common_fw_load(ch_dev_t *dev_ptr)
{
	uint8_t ch_err = 0;
	uint16_t prog_mem_addr;
	uint16_t fw_size;
	uint16_t num_xfers;
	uint16_t xfer_num = 0;
	uint16_t bytes_left = 0;
	uint16_t max_xfer_size;
	uint8_t *src_addr = (uint8_t *)dev_ptr->firmware;

	LOG_DBG("Loading firmware");
	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		prog_mem_addr = CH101_PROG_MEM_ADDR;
		fw_size = CH101_FW_SIZE;
#if (defined(MAX_PROG_XFER_SIZE) && (MAX_PROG_XFER_SIZE < CH101_FW_SIZE))
		/* optional user-supplied size limit */
		max_xfer_size = MAX_PROG_XFER_SIZE;
#else
		max_xfer_size = CH101_FW_SIZE;
#endif
	} else {
		prog_mem_addr = CH201_PROG_MEM_ADDR;
		fw_size = CH201_FW_SIZE;
#if (defined(MAX_PROG_XFER_SIZE) && (MAX_PROG_XFER_SIZE < CH201_FW_SIZE))
		/* optional user-supplied size limit */
		max_xfer_size = MAX_PROG_XFER_SIZE;
#else
		max_xfer_size = CH201_FW_SIZE;
#endif
	}

	bytes_left = fw_size;
	num_xfers = (fw_size + (max_xfer_size - 1)) / max_xfer_size;

	while (!ch_err && (xfer_num < num_xfers)) {
		/* number of bytes in this transfer */
		uint16_t xfer_nbytes;

		if (bytes_left >= max_xfer_size) {
			xfer_nbytes = max_xfer_size;
		} else {
			xfer_nbytes = bytes_left;
		}

		ch_err = chdrv_prog_mem_write(dev_ptr, prog_mem_addr, src_addr, xfer_nbytes);

		/* adjust source and destination addr */
		src_addr += xfer_nbytes;
		prog_mem_addr += xfer_nbytes;

		/* adjust remaining byte count */
		bytes_left -= xfer_nbytes;
		xfer_num++;
	}

	return ch_err;
}

/* XXX limit adjustment to interval vs. period */
#define MAX_PERIOD_VALUE 16

uint8_t ch_common_set_sample_interval(ch_dev_t *dev_ptr, uint16_t interval_ms)
{
	uint8_t period_reg;
	uint8_t tick_interval_reg;
	uint16_t max_tick_interval;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		period_reg = CH101_COMMON_REG_PERIOD;
		tick_interval_reg = CH101_COMMON_REG_TICK_INTERVAL;
		max_tick_interval = CH101_MAX_TICK_INTERVAL;
	} else {
		period_reg = CH201_COMMON_REG_PERIOD;
		tick_interval_reg = CH201_COMMON_REG_TICK_INTERVAL;
		max_tick_interval = CH201_MAX_TICK_INTERVAL;
	}

	if (!dev_ptr->sensor_connected) {
		return 1;
	}
	uint32_t sample_interval =
		dev_ptr->rtc_cal_result * interval_ms / dev_ptr->group->rtc_cal_pulse_ms;
	uint32_t period;

	if (interval_ms != 0) {
		period = (sample_interval / 2048) + 1; // XXX need define
		/* check if result fits in register */
		if (period > UINT8_MAX) {
			return 1;
		}
	} else {
		/* interval cannot be zero */
		return 1;
	}

	uint32_t tick_interval;

	if (period != 0) {
		tick_interval = sample_interval / period;

		/* enforce max interval */
		while ((tick_interval > max_tick_interval) && (period < MAX_PERIOD_VALUE)) {
			tick_interval >>= 1;
			period <<= 1;
		}
	} else {
		tick_interval = max_tick_interval;
	}

	LOG_DBG("Set period=%" PRIu32 ", tick_interval=%" PRIu32, period, tick_interval);
	chdrv_write_byte(dev_ptr, period_reg, (uint8_t)period);
	chdrv_write_word(dev_ptr, tick_interval_reg, (uint16_t)tick_interval);

	dev_ptr->sample_interval = interval_ms;

	return 0;
}

uint8_t ch_common_set_num_samples(ch_dev_t *dev_ptr, uint16_t num_samples)
{
	uint8_t max_range_reg;
	uint8_t ret_val = 1;
	uint16_t num_rx_low_gain_samples = ch_get_rx_low_gain(dev_ptr);

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		max_range_reg = CH101_COMMON_REG_MAX_RANGE;
	} else {
		max_range_reg = CH201_COMMON_REG_MAX_RANGE;
		/* each internal count for CH201 represents 2 physical samples */
		num_samples /= 2;
		num_rx_low_gain_samples /= 2;
	}

	if (num_samples < (num_rx_low_gain_samples + 1)) {
		num_samples = (num_rx_low_gain_samples + 1);
	}

	if (dev_ptr->sensor_connected && (num_samples <= UINT8_MAX)) {
		ret_val = chdrv_write_byte(dev_ptr, max_range_reg, num_samples);
	}

	if (ret_val) {
		LOG_ERR("Failed to set num_samples");
		dev_ptr->num_rx_samples = 0;
		return ret_val;
	}

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		dev_ptr->num_rx_samples = num_samples;
	} else {
		/* store actual physical sample count */
		dev_ptr->num_rx_samples = (num_samples * 2);
	}

	return 0;
}

uint8_t ch_common_set_max_range(ch_dev_t *dev_ptr, uint16_t max_range_mm)
{
	uint32_t num_samples;

	if (!dev_ptr->sensor_connected) {
		LOG_ERR("Can't set max_range, sensor not connected");
		return 1;
	}

	num_samples = dev_ptr->api_funcs.mm_to_samples(dev_ptr, max_range_mm);

	if (num_samples > dev_ptr->max_samples) {
		num_samples = dev_ptr->max_samples;
		/* store reduced max range */
		dev_ptr->max_range = ch_samples_to_mm(dev_ptr, num_samples);
	} else {
		/* store user-specified max range */
		dev_ptr->max_range = max_range_mm;
	}

	return ch_set_num_samples(dev_ptr, num_samples);
}

uint16_t ch_common_mm_to_samples(ch_dev_t *dev_ptr, uint16_t num_mm)
{
	uint16_t scale_factor;
	uint32_t num_samples = 0;
	uint32_t divisor1;
	uint32_t divisor2 = (dev_ptr->group->rtc_cal_pulse_ms * CH_SPEEDOFSOUND_MPS);

	if (dev_ptr == NULL || !dev_ptr->sensor_connected) {
		return 0;
	}

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		/* (4*16*128)  XXX need define(s) */
		divisor1 = 0x2000;
	} else {
		/* (4*16*128*2)  XXX need define(s) */
		divisor1 = 0x4000;
	}

	if (dev_ptr->scale_factor == 0) {
		ch_common_store_scale_factor(dev_ptr);
	}

	scale_factor = dev_ptr->scale_factor;

	/*
	 * Two steps of division to avoid needing a type larger than 32 bits
	 * Ceiling division to ensure result is at least enough samples to meet specified
	 * range Oversample value is signed power of two for this firmware relative to
	 * standard f/8 sampling.
	 */

	num_samples = ((dev_ptr->rtc_cal_result * scale_factor) + (divisor1 - 1)) / divisor1;
	num_samples = (((num_samples * num_mm) << dev_ptr->oversample) + (divisor2 - 1)) / divisor2;

	if (num_samples > UINT16_MAX) {
		return 0;
	}

	if (dev_ptr->part_number == CH201_PART_NUMBER) {
		/* each internal count for CH201 represents 2 physical samples */
		num_samples *= 2;
	}

	return (uint16_t)num_samples;
}

uint16_t ch_common_samples_to_mm(ch_dev_t *dev_ptr, uint16_t num_samples)
{
	uint32_t num_mm = 0;
	uint32_t op_freq = dev_ptr->op_frequency;

	if (op_freq != 0) {
		num_mm = ((uint32_t)num_samples * CH_SPEEDOFSOUND_MPS * 8 * 1000) / (op_freq * 2);
	}

	/* Adjust for oversampling, if used */
	num_mm >>= dev_ptr->oversample;

	return (uint16_t)num_mm;
}

uint8_t ch_common_set_static_range(ch_dev_t *dev_ptr, uint16_t samples)
{
	uint8_t ret_val;

	/* CH101 only */
	if (dev_ptr->part_number != CH101_PART_NUMBER) {
		return 1;
	}

	if (!dev_ptr->sensor_connected) {
		return 1;
	}

	ret_val = chdrv_write_byte(dev_ptr, CH101_COMMON_REG_STAT_RANGE, samples);
	if (ret_val != 0) {
		return ret_val;
	}

	ret_val = chdrv_write_byte(dev_ptr, CH101_COMMON_REG_STAT_COEFF,
				   CH101_COMMON_STAT_COEFF_DEFAULT);
	if (ret_val != 0) {
		return ret_val;
	}

	dev_ptr->static_range = samples;

	return ret_val;
}

uint32_t ch_common_get_range(ch_dev_t *dev_ptr, ch_range_t range_type)
{
	uint8_t tof_reg;
	uint32_t range = CH_NO_TARGET;
	uint16_t time_of_flight;
	uint16_t scale_factor;
	int err;

	if (!dev_ptr->sensor_connected) {
		return CH_NO_TARGET;
	}

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		tof_reg = CH101_COMMON_REG_TOF;
	} else {
		tof_reg = CH201_COMMON_REG_TOF;
	}

	err = chdrv_read_word(dev_ptr, tof_reg, &time_of_flight);
	if (err != 0 || time_of_flight == UINT16_MAX) {
		return CH_NO_TARGET;
	}

	/* object detected */
	if (dev_ptr->scale_factor == 0) {
		ch_common_store_scale_factor(dev_ptr);
	}
	scale_factor = dev_ptr->scale_factor;

	if (scale_factor == 0) {
		return CH_NO_TARGET;
	}

	uint32_t num =
		(CH_SPEEDOFSOUND_MPS * dev_ptr->group->rtc_cal_pulse_ms * (uint32_t)time_of_flight);
	/* XXX need define */
	uint32_t den = ((uint32_t)dev_ptr->rtc_cal_result * (uint32_t)scale_factor) >> 11;

	range = (num / den);

	if (dev_ptr->part_number == CH201_PART_NUMBER) {
		range *= 2;
	}

	if (range_type == CH_RANGE_ECHO_ONE_WAY) {
		range /= 2;
	}

	/* Adjust for oversampling, if used */
	range >>= dev_ptr->oversample;

	/* If rx-only node, adjust for pre-trigger time included in ToF */
	if (dev_ptr->mode == CH_MODE_TRIGGERED_RX_ONLY) {
		uint32_t pretrig_adj =
			(CH_SPEEDOFSOUND_MPS * dev_ptr->group->pretrig_delay_us * 32) / 1000;

		if (range > pretrig_adj) {
			/* subtract adjustment from calculated range */
			range -= pretrig_adj;
		} else {
			/* underflow - range is very close to zero, use minimum value */
			range = CH_MIN_RANGE_VAL;
		}
	}
	return range;
}

uint16_t ch_common_get_amplitude(ch_dev_t *dev_ptr)
{
	uint8_t amplitude_reg;
	uint16_t amplitude = 0;

	if (!dev_ptr->sensor_connected) {
		return 0;
	}
	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		amplitude_reg = CH101_COMMON_REG_AMPLITUDE;
	} else {
		amplitude_reg = CH201_COMMON_REG_AMPLITUDE;
	}

	chdrv_read_word(dev_ptr, amplitude_reg, &amplitude);

	return amplitude;
}

uint8_t ch_common_get_locked_state(ch_dev_t *dev_ptr)
{
	uint8_t ready_reg;
	uint8_t lock_mask = dev_ptr->freqLockValue;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		ready_reg = CH101_COMMON_REG_READY;
	} else {
		ready_reg = CH201_COMMON_REG_READY;
	}

	if (!dev_ptr->sensor_connected) {
		return 0;
	}

	uint8_t ready_value = 0;
	int rc = chdrv_read_byte(dev_ptr, ready_reg, &ready_value);

	if (rc) {
		LOG_ERR("Failed to read READY reg (%d)", rc);
		return 0;
	}

	return (ready_value & lock_mask) != 0;
}

void ch_common_prepare_pulse_timer(ch_dev_t *dev_ptr)
{
	uint8_t cal_trig_reg;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		cal_trig_reg = CH101_COMMON_REG_CAL_TRIG;
	} else {
		cal_trig_reg = CH201_COMMON_REG_CAL_TRIG;
	}

	chdrv_write_byte(dev_ptr, cal_trig_reg, 0);
}

void ch_common_store_pt_result(ch_dev_t *dev_ptr)
{
	uint8_t pt_result_reg;
	uint16_t rtc_cal_result;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		pt_result_reg = CH101_COMMON_REG_CAL_RESULT;
	} else {
		pt_result_reg = CH201_COMMON_REG_CAL_RESULT;
	}

	chdrv_read_word(dev_ptr, pt_result_reg, &rtc_cal_result);
	dev_ptr->rtc_cal_result = rtc_cal_result;
}

void ch_common_store_op_freq(ch_dev_t *dev_ptr)
{
	uint8_t tof_sf_reg;
	/* aka scale factor */
	uint16_t raw_freq;
	uint32_t freq_counter_cycles;
	uint32_t num;
	uint32_t den;
	uint32_t op_freq;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		tof_sf_reg = CH101_COMMON_REG_TOF_SF;
	} else {
		tof_sf_reg = CH201_COMMON_REG_TOF_SF;
	}

	freq_counter_cycles = dev_ptr->freqCounterCycles;

	chdrv_read_word(dev_ptr, tof_sf_reg, &raw_freq);

	num = (uint32_t)(((dev_ptr->rtc_cal_result) * 1000U) / (16U * freq_counter_cycles)) *
	      (uint32_t)(raw_freq);
	den = (uint32_t)(dev_ptr->group->rtc_cal_pulse_ms);
	op_freq = (num / den);

	dev_ptr->op_frequency = op_freq;
}

void ch_common_store_bandwidth(ch_dev_t *dev_ptr)
{
	uint32_t bandwidth = 0;
	/* XXX assumes two consecutive samples */
	ch_iq_sample_t iq_buf[2];

	(void)ch_get_iq_data(dev_ptr, iq_buf, CH_COMMON_BANDWIDTH_INDEX_1, 2, CH_IO_MODE_BLOCK);

	uint32_t mag1sq = (uint32_t)(((int32_t)iq_buf[0].i * (int32_t)iq_buf[0].i) +
				     ((int32_t)iq_buf[0].q * (int32_t)iq_buf[0].q));
	uint32_t mag2sq = (uint32_t)(((int32_t)iq_buf[1].i * (int32_t)iq_buf[1].i) +
				     ((int32_t)iq_buf[1].q * (int32_t)iq_buf[1].q));

	/* can perform below calculations using floating point for higher accuracy. */
	bandwidth = FIXEDMUL(
		FP_log(FP_sqrt(FIXEDDIV(mag1sq, mag2sq))),
		(FIXEDDIV(INT2FIXED((uint64_t)dev_ptr->op_frequency),
			  (FIXED_PI *
			   ((CH_COMMON_BANDWIDTH_INDEX_2 - CH_COMMON_BANDWIDTH_INDEX_1) * 8)))));

	dev_ptr->bandwidth = (uint16_t)FIXED2INT(bandwidth);
}

void ch_common_store_scale_factor(ch_dev_t *dev_ptr)
{
	uint8_t err;
	uint8_t tof_sf_reg;
	uint16_t scale_factor;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		tof_sf_reg = CH101_COMMON_REG_TOF_SF;
	} else {
		tof_sf_reg = CH201_COMMON_REG_TOF_SF;
	}

	err = chdrv_read_word(dev_ptr, tof_sf_reg, &scale_factor);
	if (!err) {
		dev_ptr->scale_factor = scale_factor;
	} else {
		dev_ptr->scale_factor = 0;
	}
}

uint8_t ch_common_set_thresholds(ch_dev_t *dev_ptr, ch_thresholds_t *thresholds_ptr)
{
	/* offset of register for this threshold's length */
	uint8_t thresh_len_reg = 0;
	/* threshold level reg (first in array) */
	uint8_t thresh_level_reg;
	uint8_t max_num_thresholds;
	uint8_t thresh_len;
	uint16_t thresh_level;
	uint16_t start_sample = 0;

	if (!dev_ptr->sensor_connected) {
		return 1;
	}

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		/* NOT SUPPORTED in CH101 */
		return 1;
	}
	thresh_level_reg = CH201_COMMON_REG_THRESHOLDS;
	max_num_thresholds = CH201_COMMON_NUM_THRESHOLDS;

	for (uint8_t thresh_num = 0; thresh_num < max_num_thresholds; thresh_num++) {

		if (thresh_num < (max_num_thresholds - 1)) {
			uint16_t next_start_sample =
				thresholds_ptr->threshold[thresh_num + 1].start_sample;

			thresh_len = (next_start_sample - start_sample);
			start_sample = next_start_sample;
		} else {
			thresh_len = 0;
		}

		if (thresh_num == 0) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_0;
		} else if (thresh_num == 1) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_1;
		} else if (thresh_num == 2) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_2;
		} else if (thresh_num == 3) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_3;
		} else if (thresh_num == 4) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_4;
		} else if (thresh_num == 5) {
			/* last threshold does not have length field - assumed to extend to end of
			 * data
			 */
			thresh_len_reg = 0;
		}

		if (thresh_len_reg != 0) {
			/* set the length field (if any) for this threshold */
			chdrv_write_byte(dev_ptr, thresh_len_reg, thresh_len);
		}
		/* write level to this threshold's entry in register array */
		thresh_level = thresholds_ptr->threshold[thresh_num].level;
		chdrv_write_word(dev_ptr, (thresh_level_reg + (thresh_num * sizeof(uint16_t))),
				 thresh_level);
	}

	return 0;
}

uint8_t ch_common_get_thresholds(ch_dev_t *dev_ptr, ch_thresholds_t *thresholds_ptr)
{
	/* offset of register for this threshold's length */
	uint8_t thresh_len_reg = 0;
	/* threshold level reg (first in array) */
	uint8_t thresh_level_reg;
	uint8_t max_num_thresholds;
	/* number of samples described by each threshold */
	uint8_t thresh_len = 0;
	/* calculated start sample for each threshold */
	uint16_t start_sample = 0;

	if (!dev_ptr->sensor_connected || thresholds_ptr == NULL) {
		return 1;
	}

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		/* NOT SUPPORTED in CH101 */
		return 1;
	}
	thresh_level_reg = CH201_COMMON_REG_THRESHOLDS;
	max_num_thresholds = CH201_COMMON_NUM_THRESHOLDS;

	for (uint8_t thresh_num = 0; thresh_num < max_num_thresholds; thresh_num++) {

		if (thresh_num == 0) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_0;
		} else if (thresh_num == 1) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_1;
		} else if (thresh_num == 2) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_2;
		} else if (thresh_num == 3) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_3;
		} else if (thresh_num == 4) {
			thresh_len_reg = CH201_COMMON_REG_THRESH_LEN_4;
		} else if (thresh_num == 5) {
			/* last threshold does not have length field - assumed to extend to end of
			 * data
			 */
			thresh_len_reg = 0;
		}

		if (thresh_len_reg != 0) {
			/* read the length field register for this threshold */
			chdrv_read_byte(dev_ptr, thresh_len_reg, &thresh_len);
		} else {
			thresh_len = 0;
		}

		thresholds_ptr->threshold[thresh_num].start_sample = start_sample;
		/* increment start sample for next threshold */
		start_sample += thresh_len;

		/* get level from this threshold's entry in register array */
		chdrv_read_word(dev_ptr, (thresh_level_reg + (thresh_num * sizeof(uint16_t))),
				&(thresholds_ptr->threshold[thresh_num].level));
	}
	return 0;
}

static uint8_t get_sample_data(ch_dev_t *dev_ptr, ch_iq_sample_t *buf_ptr, uint16_t start_sample,
			       uint16_t num_samples, ch_io_mode_t mode,
			       uint8_t sample_size_in_bytes)
{

	uint16_t iq_data_addr;
	ch_group_t *grp_ptr = dev_ptr->group;
	int error = 1;
	/* default = do not use low-level programming interface */
	uint8_t use_prog_read = 0;

#ifndef USE_STD_I2C_FOR_IQ
	/* if only one device on this bus */
	if (grp_ptr->num_connected[dev_ptr->i2c_bus_index] == 1) {
		/* use low-level interface */
		use_prog_read = 1;
	}
#endif

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		iq_data_addr = CH101_COMMON_REG_DATA;
	} else {
		iq_data_addr = CH201_COMMON_REG_DATA;
	}

	iq_data_addr += (start_sample * sample_size_in_bytes);

	if ((num_samples != 0) && ((start_sample + num_samples) <= dev_ptr->max_samples)) {
		uint16_t num_bytes = (num_samples * sample_size_in_bytes);

		if (mode == CH_IO_MODE_BLOCK) {
			/* blocking transfer */

			if (use_prog_read) {
				/* use low-level programming interface for speed */

				int num_transfers =
					(num_bytes + (CH_PROG_XFER_SIZE - 1)) / CH_PROG_XFER_SIZE;
				int bytes_left = num_bytes;

				/* Convert register offsets to full memory addresses */
				if (dev_ptr->part_number == CH101_PART_NUMBER) {
					iq_data_addr +=
						CH101_DATA_MEM_ADDR + CH101_COMMON_I2CREGS_OFFSET;
				} else {
					iq_data_addr +=
						CH201_DATA_MEM_ADDR + CH201_COMMON_I2CREGS_OFFSET;
				}

				/* assert PROG pin */
				chbsp_program_enable(dev_ptr);

				for (int xfer = 0; xfer < num_transfers; xfer++) {
					int bytes_to_read;
					/* read burst command */
					uint8_t message[] = {(0x80 | CH_PROG_REG_CTL), 0x09};

					if (bytes_left > CH_PROG_XFER_SIZE) {
						bytes_to_read = CH_PROG_XFER_SIZE;
					} else {
						bytes_to_read = bytes_left;
					}
					chdrv_prog_write(
						dev_ptr, CH_PROG_REG_ADDR,
						(iq_data_addr + (xfer * CH_PROG_XFER_SIZE)));
					chdrv_prog_write(dev_ptr, CH_PROG_REG_CNT,
							 (bytes_to_read - 1));
					error = chdrv_prog_i2c_write(dev_ptr, message,
								     sizeof(message));
					error |= chdrv_prog_i2c_read(
						dev_ptr,
						((uint8_t *)buf_ptr + (xfer * CH_PROG_XFER_SIZE)),
						bytes_to_read);

					bytes_left -= bytes_to_read;
				}
				/* de-assert PROG pin */
				chbsp_program_disable(dev_ptr);
			} else {
				/* use standard I2C interface */
				error = chdrv_burst_read(dev_ptr, iq_data_addr, (uint8_t *)buf_ptr,
							 num_bytes);
			}

		} else {
			/* non-blocking transfer - queue a read transaction (must be started using
			 * ch_io_start_nb() ) */
			if (use_prog_read && (grp_ptr->i2c_drv_flags & I2C_DRV_FLAG_USE_PROG_NB)) {
				/* Use low-level programming interface to read data */

				/* Convert register offsets to full memory addresses */
				if (dev_ptr->part_number == CH101_PART_NUMBER) {
					iq_data_addr +=
						(CH101_DATA_MEM_ADDR + CH101_COMMON_I2CREGS_OFFSET);
				} else {
					iq_data_addr +=
						(CH201_DATA_MEM_ADDR + CH201_COMMON_I2CREGS_OFFSET);
				}

				error = chdrv_group_i2c_queue(
					grp_ptr, dev_ptr, 1, CHDRV_NB_TRANS_TYPE_PROG, iq_data_addr,
					num_bytes, (uint8_t *)buf_ptr);
			} else {
				/* Use regular I2C register interface to read data */
				error = chdrv_group_i2c_queue(grp_ptr, dev_ptr, 1,
							      CHDRV_NB_TRANS_TYPE_STD, iq_data_addr,
							      num_bytes, (uint8_t *)buf_ptr);
			}
		}
	}

	return error;
}

uint8_t ch_common_set_sample_window(ch_dev_t *dev_ptr, uint16_t start_sample, uint16_t num_samples)
{
	uint8_t err = 1;
	uint16_t max_num_samples;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		max_num_samples = CH101_MAX_NUM_SAMPLES;
	} else {
		max_num_samples = CH201_MAX_NUM_SAMPLES;
	}

	if ((start_sample + num_samples) <= max_num_samples) {
		dev_ptr->win_start_sample = start_sample;
		dev_ptr->num_win_samples = num_samples;

		err = 0;
	}

	return err;
}

uint16_t ch_common_get_amplitude_avg(ch_dev_t *dev_ptr)
{
	ch_iq_sample_t window_buf[CH_IQ_SAMPLES_PER_READ];
	uint16_t start_sample = dev_ptr->win_start_sample;
	uint16_t num_samples = dev_ptr->num_win_samples;
	uint32_t total_amp = 0;
	uint32_t avg_amp = 0;
	uint8_t err = 0;

	if ((start_sample != 0) && (num_samples != 0)) {

		err = ch_get_iq_data(dev_ptr, window_buf, start_sample, num_samples,
				     CH_IO_MODE_BLOCK);

		if (!err) {
			for (uint16_t idx = 0; idx < num_samples; idx++) {
				/* add amplitude for this sample */
				total_amp += ch_iq_to_amplitude(&(window_buf[idx]));
			}

			avg_amp = (total_amp / num_samples);
		}
	}

	return (uint16_t)avg_amp;
}

uint8_t ch_common_get_iq_data(ch_dev_t *dev_ptr, ch_iq_sample_t *buf_ptr, uint16_t start_sample,
			      uint16_t num_samples, ch_io_mode_t mode)
{
	return get_sample_data(dev_ptr, buf_ptr, start_sample, num_samples, mode,
			       sizeof(ch_iq_sample_t));
}

uint8_t ch_common_get_amplitude_data(ch_dev_t *dev_ptr, uint16_t *buf_ptr, uint16_t start_sample,
				     uint16_t num_samples, ch_io_mode_t mode)
{
	ch_iq_sample_t iq_buf[CH_IQ_SAMPLES_PER_READ];
	uint16_t samples_in_chunk = 0;
	uint8_t error = 0;
	uint16_t sample_num = start_sample;
	uint16_t samples_left = num_samples;
	uint8_t chunks_left = (num_samples + CH_IQ_SAMPLES_PER_READ - 1) / CH_IQ_SAMPLES_PER_READ;

	/* Validate mode (only blocking mode is supported) and sample count/offset */
	if ((mode != CH_IO_MODE_BLOCK) || (start_sample + num_samples > dev_ptr->max_samples)) {
		error = 1;
	}

	while (!error && (chunks_left-- > 0)) {

		/* Read I/Q data */
		if (samples_left > CH_IQ_SAMPLES_PER_READ) {
			samples_in_chunk = CH_IQ_SAMPLES_PER_READ;
		} else {
			samples_in_chunk = samples_left;
		}

		/* adjust remaining sample count for next pass */
		samples_left -= samples_in_chunk;

		error = get_sample_data(dev_ptr, iq_buf, sample_num, samples_in_chunk, mode,
					sizeof(ch_iq_sample_t));
		if (error) {
			break;
		}

		/* Calculate amplitudes and store in user buffer */
		for (uint16_t idx = 0; idx < samples_in_chunk; idx++) {
			buf_ptr[sample_num++] = ch_iq_to_amplitude(&iq_buf[idx]);
		}
	}

	return error;
}

uint8_t ch_common_set_time_plan(ch_dev_t *dev_ptr, ch_time_plan_t time_plan)
{
	uint8_t time_plan_reg;

	if (dev_ptr->part_number != CH101_PART_NUMBER) {
		/* CH-101 only - SonicSync unsupported in CH-201 */
		return 1;
	}
	time_plan_reg = CH101_COMMON_REG_TIME_PLAN;

	if (!dev_ptr->sensor_connected) {
		return 1;
	}

	return chdrv_write_byte(dev_ptr, time_plan_reg, time_plan) != 0;
}

ch_time_plan_t ch_common_get_time_plan(ch_dev_t *dev_ptr)
{
	uint8_t time_plan_reg;
	uint8_t time_plan = CH_TIME_PLAN_NONE;

	if (dev_ptr->part_number != CH101_PART_NUMBER) {
		/* CH-101 only - SonicSync unsupported in CH-201 */
		return CH_TIME_PLAN_NONE;
	}
	time_plan_reg = CH101_COMMON_REG_TIME_PLAN;

	if (!dev_ptr->sensor_connected) {
		return CH_TIME_PLAN_NONE;
	}

	if (chdrv_read_byte(dev_ptr, time_plan_reg, &time_plan) != 0) {
		return CH_TIME_PLAN_NONE;
	}

	return (ch_time_plan_t)time_plan;
}

uint8_t ch_common_set_rx_holdoff(ch_dev_t *dev_ptr, uint16_t num_samples)
{
	uint8_t rx_holdoff_reg;
	uint16_t reg_value;
	uint8_t ret_val = RET_OK;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		rx_holdoff_reg = CH101_COMMON_REG_RX_HOLDOFF;
		reg_value = num_samples;
	} else {
		rx_holdoff_reg = CH201_COMMON_REG_RX_HOLDOFF;
		/* CH201 value is 1/2 actual sample count */
		reg_value = (num_samples / 2);
	}

	if (dev_ptr->sensor_connected) {
		ret_val |= chdrv_write_byte(dev_ptr, rx_holdoff_reg, (uint8_t)reg_value);
	}

	return ret_val;
}

uint16_t ch_common_get_rx_holdoff(ch_dev_t *dev_ptr)
{
	uint8_t rx_holdoff_reg;
	uint8_t reg_val;
	uint16_t rx_holdoff = 0;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		rx_holdoff_reg = CH101_COMMON_REG_RX_HOLDOFF;
	} else {
		rx_holdoff_reg = CH201_COMMON_REG_RX_HOLDOFF;
	}

	if (dev_ptr->sensor_connected) {
		chdrv_read_byte(dev_ptr, rx_holdoff_reg, &reg_val);
	}

	rx_holdoff = (uint16_t)reg_val;

	if (dev_ptr->part_number == CH201_PART_NUMBER) {
		/* CH201 reports 1/2 actual sample count */
		rx_holdoff *= 2;
	}

	return rx_holdoff;
}

uint8_t ch_common_set_rx_low_gain(ch_dev_t *dev_ptr, uint16_t num_samples)
{
	uint8_t rx_lowgain_reg;
	uint8_t reg_value;

	/* do not extend past end of active range */
	if (num_samples > dev_ptr->num_rx_samples - 1) {
		num_samples = (dev_ptr->num_rx_samples - 1);
	}

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		/* error - not supported on CH101 */
		return 1;
	}
	rx_lowgain_reg = CH201_COMMON_REG_LOW_GAIN_RXLEN;
	/* CH201 value is 1/2 actual sample count */
	reg_value = (num_samples / 2);

	return chdrv_write_byte(dev_ptr, rx_lowgain_reg, reg_value);
}

uint16_t ch_common_get_rx_low_gain(ch_dev_t *dev_ptr)
{
	uint8_t reg_value = 0;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		/* error - not supported on CH101 */
		return 0;
	}
	chdrv_read_byte(dev_ptr, CH201_COMMON_REG_LOW_GAIN_RXLEN, &reg_value);

	/* actual sample count is 2x register value */
	return reg_value * 2;
}

uint8_t ch_common_set_tx_length(ch_dev_t *dev_ptr, uint8_t num_cycles)
{
	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		/* error - not supported on CH101 */
		return 1;
	}
	return chdrv_write_byte(dev_ptr, CH201_COMMON_REG_TX_LENGTH, num_cycles);
}

uint8_t ch_common_get_tx_length(ch_dev_t *dev_ptr)
{
	uint8_t num_cycles = 0;

	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		/* error - not supported on CH101 */
		return 1;
	}
	chdrv_read_byte(dev_ptr, CH201_COMMON_REG_TX_LENGTH, &num_cycles);

	return num_cycles;
}

uint8_t ch_common_set_cal_result(ch_dev_t *dev_ptr, ch_cal_result_t *cal_ptr)
{
	uint8_t ret_val;

	if (dev_ptr->part_number != CH101_PART_NUMBER) {
		return RET_ERR;
	}
	if (!dev_ptr->sensor_connected || cal_ptr == NULL) {
		return RET_ERR;
	}

	ret_val = chdrv_write_word(dev_ptr, CH101_COMMON_REG_DCO_PERIOD, cal_ptr->dco_period);
	ret_val |= chdrv_write_word(dev_ptr, CH101_COMMON_REG_REV_CYCLES, cal_ptr->rev_cycles);

	return ret_val;
}

uint8_t ch_common_get_cal_result(ch_dev_t *dev_ptr, ch_cal_result_t *cal_ptr)
{
	uint8_t ret_val;

	if (dev_ptr->part_number != CH101_PART_NUMBER) {
		return RET_ERR;
	}
	if (!dev_ptr->sensor_connected || cal_ptr == NULL) {
		return RET_ERR;
	}

	ret_val = chdrv_read_word(dev_ptr, CH101_COMMON_REG_DCO_PERIOD,
				   &(cal_ptr->dco_period));
	ret_val |= chdrv_read_word(dev_ptr, CH101_COMMON_REG_REV_CYCLES,
				   &(cal_ptr->rev_cycles));

	return ret_val;
}
