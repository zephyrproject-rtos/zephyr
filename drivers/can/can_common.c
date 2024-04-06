/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_common, CONFIG_CAN_LOG_LEVEL);

/* Maximum acceptable deviation in sample point location (permille) */
#define SAMPLE_POINT_MARGIN 50

/* CAN sync segment is always one time quantum */
#define CAN_SYNC_SEG 1

struct can_tx_default_cb_ctx {
	struct k_sem done;
	int status;
};

static void can_tx_default_cb(const struct device *dev, int error, void *user_data)
{
	struct can_tx_default_cb_ctx *ctx = user_data;

	ctx->status = error;
	k_sem_give(&ctx->done);
}

int z_impl_can_send(const struct device *dev, const struct can_frame *frame,
		    k_timeout_t timeout, can_tx_callback_t callback,
		    void *user_data)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;
	uint32_t id_mask;

	CHECKIF(frame == NULL) {
		return -EINVAL;
	}

	if ((frame->flags & CAN_FRAME_IDE) != 0U) {
		id_mask = CAN_EXT_ID_MASK;
	} else {
		id_mask = CAN_STD_ID_MASK;
	}

	CHECKIF((frame->id & ~(id_mask)) != 0U) {
		LOG_ERR("invalid frame with %s (%d-bit) CAN ID 0x%0*x",
			(frame->flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
			(frame->flags & CAN_FRAME_IDE) != 0 ? 29 : 11,
			(frame->flags & CAN_FRAME_IDE) != 0 ? 8 : 3, frame->id);
		return -EINVAL;
	}

	if (callback == NULL) {
		struct can_tx_default_cb_ctx ctx;
		int err;

		k_sem_init(&ctx.done, 0, 1);

		err = api->send(dev, frame, timeout, can_tx_default_cb, &ctx);
		if (err != 0) {
			return err;
		}

		k_sem_take(&ctx.done, K_FOREVER);

		return ctx.status;
	}

	return api->send(dev, frame, timeout, callback, user_data);
}

static void can_msgq_put(const struct device *dev, struct can_frame *frame, void *user_data)
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
				  const struct can_filter *filter)
{
	const struct can_driver_api *api = dev->api;

	return api->add_rx_filter(dev, can_msgq_put, msgq, filter);
}

/**
 * @brief Update the timing given a total number of time quanta and a sample point.
 *
 * @code{.text}
 *
 * +---------------------------------------------------+
 * |     Nominal bit time in time quanta (total_tq)    |
 * +--------------+----------+------------+------------+
 * |   sync_seg   | prop_seg | phase_seg1 | phase_seg2 |
 * +--------------+----------+------------+------------+
 * | CAN_SYNG_SEG |        tseg1          |   tseg2    |
 * +--------------+-----------------------+------------+
 *                                        ^
 *                                   sample_pnt
 * @endcode
 *
 * @see @a can_timing
 *
 * @param total_tq   Total number of time quanta.
 * @param sample_pnt Sample point in permille of the entire bit time.
 * @param[out] res   Result is written into the @a can_timing struct provided.
 * @param min        Pointer to the minimum supported timing parameter values.
 * @param max        Pointer to the maximum supported timing parameter values.
 * @retval           0 or positive sample point error on success.
 * @retval           -ENOTSUP if the requested sample point cannot be met.
 */
static int update_sample_pnt(uint32_t total_tq, uint32_t sample_pnt, struct can_timing *res,
			     const struct can_timing *min, const struct can_timing *max)
{
	uint16_t tseg1_max = max->phase_seg1 + max->prop_seg;
	uint16_t tseg1_min = min->phase_seg1 + min->prop_seg;
	uint32_t sample_pnt_res;
	uint16_t tseg1, tseg2;

	/* Calculate number of time quanta in tseg2 for given sample point */
	tseg2 = total_tq - (total_tq * sample_pnt) / 1000;
	tseg2 = CLAMP(tseg2, min->phase_seg2, max->phase_seg2);

	/* Calculate number of time quanta in tseg1 */
	tseg1 = total_tq - CAN_SYNC_SEG - tseg2;
	if (tseg1 > tseg1_max) {
		/* Sample point location must be decreased */
		tseg1 = tseg1_max;
		tseg2 = total_tq - CAN_SYNC_SEG - tseg1;

		if (tseg2 > max->phase_seg2) {
			return -ENOTSUP;
		}
	} else if (tseg1 < tseg1_min) {
		/* Sample point location must be increased */
		tseg1 = tseg1_min;
		tseg2 = total_tq - CAN_SYNC_SEG - tseg1;

		if (tseg2 < min->phase_seg2) {
			return -ENOTSUP;
		}
	}

	res->phase_seg2 = tseg2;

	/* Attempt to distribute tseg1 evenly between prop_seq and phase_seg1 */
	res->prop_seg = CLAMP(tseg1 / 2, min->prop_seg, max->prop_seg);
	res->phase_seg1 = tseg1 - res->prop_seg;

	if (res->phase_seg1 > max->phase_seg1) {
		/* Even tseg1 distribution not possible, decrease phase_seg1 */
		res->phase_seg1 = max->phase_seg1;
		res->prop_seg = tseg1 - res->phase_seg1;
	} else if (res->phase_seg1 < min->phase_seg1) {
		/* Even tseg1 distribution not possible, increase phase_seg1 */
		res->phase_seg1 = min->phase_seg1;
		res->prop_seg = tseg1 - res->phase_seg1;
	}

	/* Calculate the resulting sample point */
	sample_pnt_res = (CAN_SYNC_SEG + tseg1) * 1000 / total_tq;

	/* Return the absolute sample point error */
	return sample_pnt_res > sample_pnt ?
		sample_pnt_res - sample_pnt :
		sample_pnt - sample_pnt_res;
}

/**
 * @brief Get the sample point location for a given bitrate
 *
 * @param  bitrate The bitrate in bits/second.
 * @return The sample point in permille.
 */
static uint16_t sample_point_for_bitrate(uint32_t bitrate)
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

/**
 * @brief Internal function for calculating CAN timing parameters.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param[out] res   Result is written into the @a can_timing struct provided.
 * @param min        Pointer to the minimum supported timing parameter values.
 * @param max        Pointer to the maximum supported timing parameter values.
 * @param bitrate    Target bitrate in bits/s.
 * @param sample_pnt Sample point in permille of the entire bit time.
 *
 * @retval 0 or positive sample point error on success.
 * @retval -EINVAL if the requested bitrate or sample point is out of range.
 * @retval -ENOTSUP if the requested bitrate is not supported.
 * @retval -EIO if @a can_get_core_clock() is not available.
 */
static int can_calc_timing_internal(const struct device *dev, struct can_timing *res,
				    const struct can_timing *min, const struct can_timing *max,
				    uint32_t bitrate, uint16_t sample_pnt)
{
	uint32_t total_tq = CAN_SYNC_SEG + max->prop_seg + max->phase_seg1 + max->phase_seg2;
	struct can_timing tmp_res = { 0 };
	int err_min = INT_MAX;
	uint32_t core_clock;
	int prescaler;
	int err;

	if (bitrate == 0 || sample_pnt >= 1000) {
		return -EINVAL;
	}

	err = can_get_core_clock(dev, &core_clock);
	if (err != 0) {
		return -EIO;
	}

	if (sample_pnt == 0U) {
		sample_pnt = sample_point_for_bitrate(bitrate);
	}

	for (prescaler = MAX(core_clock / (total_tq * bitrate), min->prescaler);
	     prescaler <= max->prescaler;
	     prescaler++) {

		if (core_clock % (prescaler * bitrate)) {
			/* No integer total_tq for this prescaler setting */
			continue;
		}

		total_tq = core_clock / (prescaler * bitrate);

		err = update_sample_pnt(total_tq, sample_pnt, &tmp_res, min, max);
		if (err < 0) {
			/* Sample point cannot be met for this prescaler setting */
			continue;
		}

		if (err < err_min) {
			/* Improved sample point match */
			err_min = err;
			res->prop_seg = tmp_res.prop_seg;
			res->phase_seg1 = tmp_res.phase_seg1;
			res->phase_seg2 = tmp_res.phase_seg2;
			res->prescaler = (uint16_t)prescaler;

			if (err == 0) {
				/* Perfect sample point match */
				break;
			}
		}
	}

	if (err_min != 0U) {
		LOG_DBG("Sample point error: %d 1/1000", err_min);
	}

	/* Calculate default sjw as phase_seg2 / 2 and clamp the result */
	res->sjw = MIN(res->phase_seg1, res->phase_seg2 / 2);
	res->sjw = CLAMP(res->sjw, min->sjw, max->sjw);

	return err_min == INT_MAX ? -ENOTSUP : err_min;
}

int z_impl_can_calc_timing(const struct device *dev, struct can_timing *res,
			   uint32_t bitrate, uint16_t sample_pnt)
{
	const struct can_timing *min = can_get_timing_min(dev);
	const struct can_timing *max = can_get_timing_max(dev);

	if (bitrate > 1000000) {
		return -EINVAL;
	}

	return can_calc_timing_internal(dev, res, min, max, bitrate, sample_pnt);
}

#ifdef CONFIG_CAN_FD_MODE
int z_impl_can_calc_timing_data(const struct device *dev, struct can_timing *res,
				uint32_t bitrate, uint16_t sample_pnt)
{
	const struct can_timing *min = can_get_timing_data_min(dev);
	const struct can_timing *max = can_get_timing_data_max(dev);

	if (bitrate > 8000000) {
		return -EINVAL;
	}

	return can_calc_timing_internal(dev, res, min, max, bitrate, sample_pnt);
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

static int check_timing_in_range(const struct can_timing *timing,
				 const struct can_timing *min,
				 const struct can_timing *max)
{
	if (!IN_RANGE(timing->sjw, min->sjw, max->sjw) ||
	    !IN_RANGE(timing->prop_seg, min->prop_seg, max->prop_seg) ||
	    !IN_RANGE(timing->phase_seg1, min->phase_seg1, max->phase_seg1) ||
	    !IN_RANGE(timing->phase_seg2, min->phase_seg2, max->phase_seg2) ||
	    !IN_RANGE(timing->prescaler, min->prescaler, max->prescaler)) {
		return -ENOTSUP;
	}

	if ((timing->sjw > timing->phase_seg1) || (timing->sjw > timing->phase_seg2)) {
		return -ENOTSUP;
	}

	return 0;
}

int z_impl_can_set_timing(const struct device *dev,
			  const struct can_timing *timing)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;
	const struct can_timing *min = can_get_timing_min(dev);
	const struct can_timing *max = can_get_timing_max(dev);
	int err;

	err = check_timing_in_range(timing, min, max);
	if (err != 0) {
		return err;
	}

	return api->set_timing(dev, timing);
}

int z_impl_can_set_bitrate(const struct device *dev, uint32_t bitrate)
{
	struct can_timing timing = { 0 };
	uint32_t min = can_get_bitrate_min(dev);
	uint32_t max = can_get_bitrate_max(dev);
	uint16_t sample_pnt;
	int ret;

	if ((bitrate < min) || (bitrate > max)) {
		return -ENOTSUP;
	}

	sample_pnt = sample_point_for_bitrate(bitrate);
	ret = can_calc_timing(dev, &timing, bitrate, sample_pnt);
	if (ret < 0) {
		return ret;
	}

	if (ret > SAMPLE_POINT_MARGIN) {
		return -ERANGE;
	}

	return can_set_timing(dev, &timing);
}

#ifdef CONFIG_CAN_FD_MODE
int z_impl_can_set_timing_data(const struct device *dev,
			       const struct can_timing *timing_data)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;
	const struct can_timing *min = can_get_timing_data_min(dev);
	const struct can_timing *max = can_get_timing_data_max(dev);
	int err;

	if (api->set_timing_data == NULL) {
		return -ENOSYS;
	}

	err = check_timing_in_range(timing_data, min, max);
	if (err != 0) {
		return err;
	}

	return api->set_timing_data(dev, timing_data);
}

int z_impl_can_set_bitrate_data(const struct device *dev, uint32_t bitrate_data)
{
	struct can_timing timing_data = { 0 };
	uint32_t min = can_get_bitrate_min(dev);
	uint32_t max = can_get_bitrate_max(dev);
	uint16_t sample_pnt;
	int ret;

	if ((bitrate_data < min) || (bitrate_data > max)) {
		return -ENOTSUP;
	}

	sample_pnt = sample_point_for_bitrate(bitrate_data);
	ret = can_calc_timing_data(dev, &timing_data, bitrate_data, sample_pnt);
	if (ret < 0) {
		return ret;
	}

	if (ret > SAMPLE_POINT_MARGIN) {
		return -ERANGE;
	}

	return can_set_timing_data(dev, &timing_data);
}
#endif /* CONFIG_CAN_FD_MODE */
