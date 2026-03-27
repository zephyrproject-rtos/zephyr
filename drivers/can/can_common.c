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

/* CAN sync segment is always one time quantum */
#define CAN_SYNC_SEG 1

struct can_tx_default_cb_ctx {
	struct k_sem done;
	int status;
};

static void can_tx_default_cb(const struct device *dev, int error, void *user_data)
{
	struct can_tx_default_cb_ctx *ctx = user_data;

	ARG_UNUSED(dev);

	ctx->status = error;
	k_sem_give(&ctx->done);
}

int z_impl_can_send(const struct device *dev, const struct can_frame *frame,
		    k_timeout_t timeout, can_tx_callback_t callback,
		    void *user_data)
{
	const struct can_driver_api *api = DEVICE_API_GET(can, dev);
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

int can_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
		      void *user_data, const struct can_filter *filter)
{
	uint32_t id_mask;

	CHECKIF(callback == NULL || filter == NULL) {
		return -EINVAL;
	}

	if ((filter->flags & CAN_FILTER_IDE) != 0U) {
		id_mask = CAN_EXT_ID_MASK;
	} else {
		id_mask = CAN_STD_ID_MASK;
	}

	CHECKIF(((filter->id & ~(id_mask)) != 0U) || ((filter->mask & ~(id_mask)) != 0U)) {
		LOG_ERR("invalid filter with %s (%d-bit) CAN ID 0x%0*x, CAN ID mask 0x%0*x",
			(filter->flags & CAN_FILTER_IDE) != 0 ? "extended" : "standard",
			(filter->flags & CAN_FILTER_IDE) != 0 ? 29 : 11,
			(filter->flags & CAN_FILTER_IDE) != 0 ? 8 : 3, filter->id,
			(filter->flags & CAN_FILTER_IDE) != 0 ? 8 : 3, filter->mask);
		return -EINVAL;
	}

	return DEVICE_API_GET(can, dev)->add_rx_filter(dev, callback, user_data, filter);
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

	return DEVICE_API_GET(can, dev)->add_rx_filter(dev, can_msgq_put, msgq, filter);
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
	uint16_t tseg1;
	uint16_t tseg2;

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
	} else {
		/* Sample point location within range */
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
	} else {
		/* No redistribution necessary */
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
	/*
	 * Accept up to 0.5% bitrate error (5000 ppm). CiA 601 allows up to
	 * 1.58% for nominal bitrate; 0.5% gives comfortable margin while
	 * unlocking prescalers that would otherwise be skipped when the core
	 * clock is not an exact integer multiple of (prescaler * bitrate).
	 */
	const uint32_t TOLERANCE_PPM = 5000U;

	/* Total-TQ bounds from hardware segment limits */
	const uint32_t tq_min = CAN_SYNC_SEG + min->prop_seg + min->phase_seg1 + min->phase_seg2;
	const uint32_t tq_max = CAN_SYNC_SEG + max->prop_seg + max->phase_seg1 + max->phase_seg2;

	uint32_t total_tq = tq_max;
	struct can_timing tmp_res = { 0 };
	int err_min = INT_MAX;
	uint32_t core_clock;
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

	for (int prescaler = MAX(core_clock / (tq_max * bitrate), (uint32_t)min->prescaler);
	     prescaler <= (int)max->prescaler; prescaler++) {

		/*
		 * Round total_tq to the nearest integer instead of requiring
		 * exact divisibility. This prevents skipping all prescalers on
		 * targets whose core clock is not an integer multiple of the
		 * requested bitrate (e.g. PLL-derived clocks on NXP Kinetis,
		 * certain STM32 and Nordic configurations).
		 */
		uint64_t prod = (uint64_t)prescaler * (uint64_t)bitrate;

		total_tq = (uint32_t)((core_clock + (prod / 2U)) / prod);

		/* Skip prescalers that produce a total_tq outside segment limits */
		if (total_tq < tq_min || total_tq > tq_max) {
			continue;
		}

		/* Compute the actually achievable bitrate for this (prescaler, total_tq) */
		uint32_t real_bitrate = (uint32_t)(core_clock / ((uint64_t)prescaler * total_tq));
		int32_t ppm = (int32_t)((int64_t)real_bitrate - (int64_t)bitrate);

		if (ppm < 0) {
			ppm = -ppm;
		}
		ppm = (int32_t)((int64_t)ppm * 1000000LL / (int64_t)bitrate);
		if ((uint32_t)ppm > TOLERANCE_PPM) {
			/* Bitrate deviation exceeds tolerance; skip */
			continue;
		}

		err = update_sample_pnt(total_tq, sample_pnt, &tmp_res, min, max);
		if (err < 0) {
			/* Sample point cannot be met for this (prescaler, total_tq) */
			continue;
		}

		if (err < err_min) {
			/* Improved sample point match */
			err_min = err;
			res->prop_seg = tmp_res.prop_seg;
			res->phase_seg1 = tmp_res.phase_seg1;
			res->phase_seg2 = tmp_res.phase_seg2;
			res->prescaler = (uint16_t)prescaler;

			/* Perfect SP and exact bitrate: no need to continue searching */
			if (err == 0 && real_bitrate == bitrate) {
				break;
			}
		}
	}

	if (err_min != 0U) {
		LOG_DBG("Sample point error: %d 1/1000", err_min);
	}

	/* Default SJW = min(phase_seg1, phase_seg2/2), clamped to limits */
	res->sjw = MIN(res->phase_seg1, res->phase_seg2 / 2U);
	res->sjw = CLAMP(res->sjw, min->sjw, max->sjw);

	return (err_min == INT_MAX) ? -ENOTSUP : err_min;
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
	const struct can_timing *min = can_get_timing_min(dev);
	const struct can_timing *max = can_get_timing_max(dev);
	int err;

	err = check_timing_in_range(timing, min, max);
	if (err != 0) {
		return err;
	}

	return DEVICE_API_GET(can, dev)->set_timing(dev, timing);
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

	if (ret > CONFIG_CAN_SAMPLE_POINT_MARGIN) {
		return -ERANGE;
	}

	return can_set_timing(dev, &timing);
}

#ifdef CONFIG_CAN_FD_MODE
int z_impl_can_set_timing_data(const struct device *dev,
			       const struct can_timing *timing_data)
{
	const struct can_driver_api *api = DEVICE_API_GET(can, dev);
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

	if (ret > CONFIG_CAN_SAMPLE_POINT_MARGIN) {
		return -ERANGE;
	}

	return can_set_timing_data(dev, &timing_data);
}
#endif /* CONFIG_CAN_FD_MODE */
