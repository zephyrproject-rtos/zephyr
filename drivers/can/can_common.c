/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_common, CONFIG_CAN_LOG_LEVEL);

/* Maximum acceptable deviation in sample point location (permille) */
#define SAMPLE_POINT_MARGIN 50

/* CAN sync segment is always one time quantum */
#define CAN_SYNC_SEG 1

static void can_msgq_put(const struct device *dev, struct zcan_frame *frame, void *user_data)
{
	struct k_msgq *msgq = (struct k_msgq *)user_data;
	int ret;

	ARG_UNUSED(dev);

	__ASSERT_NO_MSG(msgq);

	ret = k_msgq_put(msgq, frame, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Msgq %p overflowed. Frame ID: 0x%x", msgq, frame->id);
	}
}

int z_impl_can_add_rx_filter_msgq(const struct device *dev, struct k_msgq *msgq,
				  const struct zcan_filter *filter)
{
	const struct can_driver_api *api = dev->api;

	return api->add_rx_filter(dev, can_msgq_put, msgq, filter);
}

static int update_sampling_pnt(uint32_t ts, uint32_t sp, struct can_timing *res,
			       const struct can_timing *max,
			       const struct can_timing *min)
{
	uint16_t ts1_max = max->phase_seg1 + max->prop_seg;
	uint16_t ts1_min = min->phase_seg1 + min->prop_seg;
	uint32_t sp_calc;
	uint16_t ts1, ts2;

	ts2 = ts - (ts * sp) / 1000;
	ts2 = CLAMP(ts2, min->phase_seg2, max->phase_seg2);
	ts1 = ts - CAN_SYNC_SEG - ts2;

	if (ts1 > ts1_max) {
		ts1 = ts1_max;
		ts2 = ts - CAN_SYNC_SEG - ts1;
		if (ts2 > max->phase_seg2) {
			return -1;
		}
	} else if (ts1 < ts1_min) {
		ts1 = ts1_min;
		ts2 = ts - ts1;
		if (ts2 < min->phase_seg2) {
			return -1;
		}
	}

	res->prop_seg = CLAMP(ts1 / 2, min->prop_seg, max->prop_seg);
	res->phase_seg1 = ts1 - res->prop_seg;
	res->phase_seg2 = ts2;

	sp_calc = (CAN_SYNC_SEG + ts1) * 1000 / ts;

	return sp_calc > sp ? sp_calc - sp : sp - sp_calc;
}

/* Internal function to do the actual calculation */
static int can_calc_timing_int(uint32_t core_clock, struct can_timing *res,
			       const struct can_timing *min,
			       const struct can_timing *max,
			       uint32_t bitrate, uint16_t sp)
{
	uint32_t ts = max->prop_seg + max->phase_seg1 + max->phase_seg2 +
		   CAN_SYNC_SEG;
	uint16_t sp_err_min = UINT16_MAX;
	int sp_err;
	struct can_timing tmp_res;

	if (bitrate == 0 || sp >= 1000 ||
	    (!IS_ENABLED(CONFIG_CAN_FD_MODE) && bitrate > 1000000) ||
	     (IS_ENABLED(CONFIG_CAN_FD_MODE) && bitrate > 8000000)) {
		return -EINVAL;
	}

	for (int prescaler = MAX(core_clock / (ts * bitrate), 1);
	     prescaler <= max->prescaler; ++prescaler) {
		if (core_clock % (prescaler * bitrate)) {
			/* No integer ts */
			continue;
		}

		ts = core_clock / (prescaler * bitrate);

		sp_err = update_sampling_pnt(ts, sp, &tmp_res,
					     max, min);
		if (sp_err < 0) {
			/* No prop_seg, seg1, seg2 combination possible */
			continue;
		}

		if (sp_err < sp_err_min) {
			sp_err_min = sp_err;
			res->prop_seg = tmp_res.prop_seg;
			res->phase_seg1 = tmp_res.phase_seg1;
			res->phase_seg2 = tmp_res.phase_seg2;
			res->prescaler = (uint16_t)prescaler;
			if (sp_err == 0) {
				/* No better result than a perfect match*/
				break;
			}
		}
	}

	if (sp_err_min) {
		LOG_DBG("SP error: %d 1/1000", sp_err_min);
	}

	return sp_err_min == UINT16_MAX ? -EINVAL : (int)sp_err_min;
}


int z_impl_can_calc_timing(const struct device *dev, struct can_timing *res,
			   uint32_t bitrate, uint16_t sample_pnt)
{
	const struct can_timing *min = can_get_timing_min(dev);
	const struct can_timing *max = can_get_timing_max(dev);
	uint32_t core_clock;
	int ret;

	ret = can_get_core_clock(dev, &core_clock);
	if (ret != 0) {
		return ret;
	}

	return can_calc_timing_int(core_clock, res, min, max, bitrate, sample_pnt);
}

#ifdef CONFIG_CAN_FD_MODE
int z_impl_can_calc_timing_data(const struct device *dev, struct can_timing *res,
				uint32_t bitrate, uint16_t sample_pnt)
{
	const struct can_timing *min = can_get_timing_data_min(dev);
	const struct can_timing *max = can_get_timing_data_max(dev);
	uint32_t core_clock;
	int ret;

	ret = can_get_core_clock(dev, &core_clock);
	if (ret != 0) {
		return ret;
	}

	return can_calc_timing_int(core_clock, res, min, max, bitrate, sample_pnt);
}
#endif /* CONFIG_CAN_FD_MODE */

int can_calc_prescaler(const struct device *dev, struct can_timing *timing,
		       uint32_t bitrate)
{
	uint32_t ts = timing->prop_seg + timing->phase_seg1 + timing->phase_seg2 +
		   CAN_SYNC_SEG;
	uint32_t core_clock;
	int ret;

	ret = can_get_core_clock(dev, &core_clock);
	if (ret != 0) {
		return ret;
	}

	timing->prescaler = core_clock / (bitrate * ts);

	return core_clock % (ts * timing->prescaler);
}

/**
 * @brief Get the sample point location for a given bitrate
 *
 * @param  bitrate The bitrate in bits/second.
 * @return The sample point in permille.
 */
uint16_t sample_point_for_bitrate(uint32_t bitrate)
{
	uint16_t sample_pnt;

	if (bitrate > 800000) {
		/* 75.0% */
		sample_pnt = 750;
	} else if (bitrate > 500000) {
		/* 80.0% */
		sample_pnt = 800;
	} else {
		/* 87.5% */
		sample_pnt = 875;
	}

	return sample_pnt;
}

int z_impl_can_set_bitrate(const struct device *dev, uint32_t bitrate)
{
	struct can_timing timing;
	uint32_t max_bitrate;
	uint16_t sample_pnt;
	int ret;

	ret = can_get_max_bitrate(dev, &max_bitrate);
	if (ret == -ENOSYS) {
		/* Maximum bitrate unknown */
		max_bitrate = 0;
	} else if (ret < 0) {
		return ret;
	}

	if ((max_bitrate > 0) && (bitrate > max_bitrate)) {
		return -ENOTSUP;
	}

	sample_pnt = sample_point_for_bitrate(bitrate);
	ret = can_calc_timing(dev, &timing, bitrate, sample_pnt);
	if (ret < 0) {
		return -EINVAL;
	}

	if (ret > SAMPLE_POINT_MARGIN) {
		return -EINVAL;
	}

	timing.sjw = CAN_SJW_NO_CHANGE;

	return can_set_timing(dev, &timing);
}

#ifdef CONFIG_CAN_FD_MODE
int z_impl_can_set_bitrate_data(const struct device *dev, uint32_t bitrate_data)
{
	struct can_timing timing_data;
	uint32_t max_bitrate;
	uint16_t sample_pnt;
	int ret;

	ret = can_get_max_bitrate(dev, &max_bitrate);
	if (ret == -ENOSYS) {
		/* Maximum bitrate unknown */
		max_bitrate = 0;
	} else if (ret < 0) {
		return ret;
	}

	if ((max_bitrate > 0) && (bitrate_data > max_bitrate)) {
		return -ENOTSUP;
	}

	sample_pnt = sample_point_for_bitrate(bitrate_data);
	ret = can_calc_timing_data(dev, &timing_data, bitrate_data, sample_pnt);
	if (ret < 0) {
		return -EINVAL;
	}

	if (ret > SAMPLE_POINT_MARGIN) {
		return -EINVAL;
	}

	timing_data.sjw = CAN_SJW_NO_CHANGE;

	return can_set_timing_data(dev, &timing_data);
}
#endif /* CONFIG_CAN_FD_MODE */
