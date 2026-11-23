/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi270

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "bmi270.h"
#include "bmi270_decoder.h"

#if defined(CONFIG_BMI270_STREAM)

LOG_MODULE_DECLARE(bmi270, CONFIG_SENSOR_LOG_LEVEL);

static inline const struct gpio_dt_spec *fifo_pin(const struct bmi270_config *config)
{
	return IS_ENABLED(CONFIG_BMI270_FIFO_ON_INT2) ? &config->int2 : &config->int1;
}

static inline uint8_t fifo_fwm_map_bit(void)
{
	return IS_ENABLED(CONFIG_BMI270_FIFO_ON_INT2) ? BMI270_INT_MAP_DATA_FWM_INT2
						      : BMI270_INT_MAP_DATA_FWM_INT1;
}

static inline uint8_t fifo_ffull_map_bit(void)
{
	return IS_ENABLED(CONFIG_BMI270_FIFO_ON_INT2) ? BMI270_INT_MAP_DATA_FFULL_INT2
						      : BMI270_INT_MAP_DATA_FFULL_INT1;
}

static inline uint8_t fifo_io_ctrl_reg(void)
{
	return IS_ENABLED(CONFIG_BMI270_FIFO_ON_INT2) ? BMI270_REG_INT2_IO_CTRL
						      : BMI270_REG_INT1_IO_CTRL;
}

static inline uint16_t fifo_watermark_bytes(void)
{
#if defined(CONFIG_BMI270_FIFO_HEADERLESS)
	return CONFIG_BMI270_FIFO_WATERMARK_SAMPLES * BMI270_FIFO_FRAME_PAYLOAD_ACC_GYR_BYTES;
#else
	return CONFIG_BMI270_FIFO_WATERMARK_SAMPLES * BMI270_FIFO_FRAME_ACC_GYR_BYTES +
	       CONFIG_BMI270_FIFO_WATERMARK_CONTROL_MARGIN;
#endif
}

static inline uint16_t acc_odr_reg_to_hz(uint8_t odr_reg)
{
	switch (odr_reg) {
	case BMI270_ACC_ODR_25_HZ:
		return 25;
	case BMI270_ACC_ODR_50_HZ:
		return 50;
	case BMI270_ACC_ODR_100_HZ:
		return 100;
	case BMI270_ACC_ODR_200_HZ:
		return 200;
	case BMI270_ACC_ODR_400_HZ:
		return 400;
	case BMI270_ACC_ODR_800_HZ:
		return 800;
	case BMI270_ACC_ODR_1600_HZ:
		return 1600;
	default:
		return 0;
	}
}

/* Full-scale G (2, 4, 8, 16) to bmi270_decoder_header.acc_range field 0..3 */
static inline uint8_t acc_fullscale_g_to_decoder_idx(uint8_t fs_g)
{
	switch (fs_g) {
	case 2:
		return 0U;
	case 4:
		return 1U;
	case 8:
		return 2U;
	default:
		return 3U;
	}
}

/* Gyro full-scale (dps) to bmi270_decoder_header.gyr_range_idx field 0..4 */
static inline uint8_t gyr_fullscale_dps_to_decoder_idx(uint16_t range_dps)
{
	switch (range_dps) {
	case 2000:
		return 0U;
	case 1000:
		return 1U;
	case 500:
		return 2U;
	case 250:
		return 3U;
	case 125:
		return 4U;
	default:
		return 0U;
	}
}

static inline uint16_t gyr_odr_reg_to_hz(uint8_t odr_reg)
{
	switch (odr_reg) {
	case BMI270_GYR_ODR_25_HZ:
		return 25;
	case BMI270_GYR_ODR_50_HZ:
		return 50;
	case BMI270_GYR_ODR_100_HZ:
		return 100;
	case BMI270_GYR_ODR_200_HZ:
		return 200;
	case BMI270_GYR_ODR_400_HZ:
		return 400;
	case BMI270_GYR_ODR_800_HZ:
		return 800;
	case BMI270_GYR_ODR_1600_HZ:
		return 1600;
	case BMI270_GYR_ODR_3200_HZ:
		return 3200;
	default:
		return 0;
	}
}

static inline void stream_error(const struct device *dev, struct bmi270_data *data,
				struct rtio_iodev_sqe *iodev_sqe, int err)
{
	int ps_ret = bmi270_adv_power_save_enable(dev);

	if (ps_ret) {
		LOG_ERR("adv_power_save_enable failed: %d", ps_ret);
	}
	rtio_iodev_sqe_err(iodev_sqe, err);
	data->streaming_sqe = NULL;
}

static struct sensor_stream_trigger *get_read_config_trigger(const struct sensor_read_config *cfg,
							     enum sensor_trigger_type trig)
{
	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == trig) {
			return (struct sensor_stream_trigger *)&cfg->triggers[i];
		}
	}
	return NULL;
}

static int configure_fifo(const struct device *dev, bool enable, uint16_t watermark_bytes)
{
	uint8_t val;
	int ret;

	if (!enable) {
		val = 0;
		ret = bmi270_reg_write(dev, BMI270_REG_FIFO_CONFIG_1, &val, 1);
		if (ret < 0) {
			return ret;
		}
		return 0;
	}

	/* FIFO_CONFIG_0: stop_on_full=0, fifo_time_en=0 (streaming, no sensortime) */
	val = 0;
	ret = bmi270_reg_write(dev, BMI270_REG_FIFO_CONFIG_0, &val, 1);
	if (ret < 0) {
		return ret;
	}

	/* FIFO_CONFIG_1: acc+gyr in FIFO; header bit depends on mode */
	val = BMI270_FIFO_CONFIG_1_FIFO_ACC_EN_MSK | BMI270_FIFO_CONFIG_1_FIFO_GYR_EN_MSK;
#if !defined(CONFIG_BMI270_FIFO_HEADERLESS)
	val |= BMI270_FIFO_CONFIG_1_FIFO_HEADER_EN_MSK;
#endif
	ret = bmi270_reg_write(dev, BMI270_REG_FIFO_CONFIG_1, &val, 1);
	if (ret < 0) {
		return ret;
	}

	/* Watermark in bytes */
	uint8_t wtm[2] = {
		watermark_bytes & 0xFF,
		(watermark_bytes >> 8) & 0x1F,
	};
	ret = bmi270_reg_write(dev, BMI270_REG_FIFO_WTM_0, wtm, 2);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int map_fifo_int(const struct device *dev, bool watermark, bool full)
{
	uint8_t int_map = 0;

	if (watermark) {
		int_map |= fifo_fwm_map_bit();
	}
	if (full) {
		int_map |= fifo_ffull_map_bit();
	}
	return bmi270_reg_write(dev, BMI270_REG_INT_MAP_DATA, &int_map, 1);
}

#if defined(CONFIG_BMI270_FIFO_POLL_FALLBACK)

#define FIFO_POLL_MS CONFIG_BMI270_FIFO_POLL_PERIOD_MS

static const struct device *poll_dev;
static struct k_work_delayable poll_work;
static bool poll_work_inited;

static void poll_work_fn(struct k_work *work)
{
	const struct device *dev = poll_dev;
	struct bmi270_data *data;
	uint16_t fifo_len;

	if (dev == NULL) {
		return;
	}

	data = dev->data;
	if (data->streaming_sqe != NULL) {
		bmi270_reg_read(dev, BMI270_REG_FIFO_LENGTH_0, (uint8_t *)&fifo_len, 2);
		fifo_len = sys_get_le16((uint8_t *)&fifo_len) & 0x3FFF;
		if (fifo_len >= data->fifo_watermark_bytes) {
			bmi270_stream_submit_fifo_job(dev);
		}
	}

	k_work_schedule(&poll_work, K_MSEC(FIFO_POLL_MS));
}
#endif /* CONFIG_BMI270_FIFO_POLL_FALLBACK */

static void adv_power_save_enable_log_failure(const struct device *dev)
{
	int ret = bmi270_adv_power_save_enable(dev);

	if (ret != 0) {
		LOG_ERR("adv_power_save_enable failed: %d", ret);
	}
}

static void fifo_fill_encoded_header(struct bmi270_fifo_encoded_data *edata,
				     struct bmi270_data *data, uint16_t fifo_len)
{
	edata->header.is_fifo = true;
#if defined(CONFIG_BMI270_FIFO_HEADERLESS)
	edata->header.is_headerless = true;
#else
	edata->header.is_headerless = false;
#endif
	edata->header.timestamp = data->timestamp;
	edata->header.acc_range = acc_fullscale_g_to_decoder_idx(data->acc_range);
	edata->header.acc_odr_hz = acc_odr_reg_to_hz(data->acc_odr);
	edata->header.gyr_odr_hz = gyr_odr_reg_to_hz(data->gyr_odr);
	edata->header.gyr_range_idx = gyr_fullscale_dps_to_decoder_idx(data->gyr_range);
	edata->fifo_byte_count = fifo_len;
}

static int bmi270_rtio_drain_cq(struct rtio *r, int result)
{
	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			if (result >= 0) {
				result = cqe->result;
			}
			rtio_cqe_release(r, cqe);
		}
	} while (cqe != NULL);

	return result;
}

enum bmi270_fifo_job_phase {
	BMI270_FIFO_JOB_STATUS,
	BMI270_FIFO_JOB_DATA,
	BMI270_FIFO_JOB_DRAIN,
	BMI270_FIFO_JOB_CLEAR_STATUS,
};

/* SPI register reads use 3 SQEs, so keep each drain submission within the 8-SQE pool. */
#define BMI270_FIFO_DATA_JOB_DRAIN_READS 1
#define BMI270_FIFO_DRAIN_JOB_DRAIN_READS 2

static void bmi270_fifo_job_work_handler(struct k_work *work)
{
	struct bmi270_data *data = CONTAINER_OF(work, struct bmi270_data, fifo_job_work);

	bmi270_stream_handle_fifo(data->dev);
}

void bmi270_stream_init(const struct device *dev)
{
	struct bmi270_data *data = dev->data;

	k_work_init(&data->fifo_job_work, bmi270_fifo_job_work_handler);
}

void bmi270_stream_submit_fifo_job(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->fifo_job_lock);
	data->fifo_job_pending++;
	if (!data->fifo_job_queued && !data->fifo_job_processing) {
		data->fifo_job_pending--;
		data->fifo_job_queued = true;
		data->fifo_job_phase = BMI270_FIFO_JOB_STATUS;
		mpsc_push(&data->fifo_jobs, &data->fifo_job);
	}
	k_spin_unlock(&data->fifo_job_lock, key);

	k_work_submit(&data->fifo_job_work);
}

static void bmi270_requeue_fifo_job_work(const struct device *dev,
					 enum bmi270_fifo_job_phase phase)
{
	struct bmi270_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->fifo_job_lock);
	data->fifo_job_phase = phase;
	data->fifo_job_queued = true;
	data->fifo_job_processing = false;
	mpsc_push(&data->fifo_jobs, &data->fifo_job);
	k_spin_unlock(&data->fifo_job_lock, key);
	k_work_submit(&data->fifo_job_work);
}

static void bmi270_fifo_job_complete(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	k_spinlock_key_t key;
	bool submit = false;

	key = k_spin_lock(&data->fifo_job_lock);
	data->fifo_job_processing = false;
	if (data->fifo_job_pending > 0U) {
		data->fifo_job_pending--;
		data->fifo_job_queued = true;
		data->fifo_job_phase = BMI270_FIFO_JOB_STATUS;
		mpsc_push(&data->fifo_jobs, &data->fifo_job);
		submit = true;
	} else {
		data->fifo_job_queued = false;
	}
	k_spin_unlock(&data->fifo_job_lock, key);

	if (submit) {
		k_work_submit(&data->fifo_job_work);
	}
}

static void bmi270_fifo_job_abort(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->fifo_job_lock);
	data->fifo_job_pending = 0U;
	data->fifo_job_queued = false;
	data->fifo_job_processing = false;
	k_spin_unlock(&data->fifo_job_lock, key);

	/* Force a full re-arm on the next submit_stream(), state may be stale after an abort. */
	data->fifo_configured = false;
}

static int bmi270_prep_fifo_drain_async(const struct device *dev, size_t max_reads,
					uint16_t *drained)
{
	struct bmi270_data *data = dev->data;
	int ret;

	*drained = 0U;

	while (data->fifo_drain_len > *drained && max_reads > 0) {
		uint16_t remaining = data->fifo_drain_len - *drained;
		size_t chunk = MIN((size_t)remaining, sizeof(data->fifo_discard_buf));

		ret = bmi270_prep_reg_read_async(dev, BMI270_REG_FIFO_DATA, data->fifo_discard_buf,
						 chunk, RTIO_SQE_CHAINED);
		if (ret < 0) {
			return ret;
		}

		*drained += (uint16_t)chunk;
		max_reads--;
	}

	return 0;
}

static void bmi270_fifo_drain_done_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				      void *arg)
{
	ARG_UNUSED(sqe);

	const struct device *dev = arg;
	struct bmi270_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->streaming_sqe;

	result = bmi270_rtio_drain_cq(r, result);

	if (result < 0 || iodev_sqe == NULL) {
		if (iodev_sqe != NULL) {
			stream_error(dev, data, iodev_sqe, result);
		}
		data->fifo_drain_len = 0U;
		bmi270_fifo_job_abort(dev);
		return;
	}

	if (data->fifo_drain_len > 0U) {
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DRAIN);
	} else {
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_CLEAR_STATUS);
	}
}

static void bmi270_fifo_read_done_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				     void *arg)
{
	ARG_UNUSED(sqe);

	const struct device *dev = arg;
	struct bmi270_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->streaming_sqe;

	result = bmi270_rtio_drain_cq(r, result);

	if (result < 0 || iodev_sqe == NULL) {
		if (iodev_sqe != NULL) {
			stream_error(dev, data, iodev_sqe, result);
		}
		bmi270_fifo_job_abort(dev);
		return;
	}

	adv_power_save_enable_log_failure(dev);

	data->streaming_sqe = NULL;
	rtio_iodev_sqe_ok(iodev_sqe,
			  sizeof(struct bmi270_fifo_encoded_data) + data->fifo_len);
	bmi270_fifo_job_complete(dev);
}

static void bmi270_read_fifo_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				void *arg)
{
	ARG_UNUSED(sqe);

	const struct device *dev = arg;
	struct bmi270_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->streaming_sqe;
	uint16_t fifo_len;
	uint16_t fifo_len_orig;
	uint8_t *buf;
	uint32_t buf_len;
	struct bmi270_fifo_encoded_data *edata;
	size_t max_fifo;
	size_t min_len;
	int ret;

	if (result < 0 || iodev_sqe == NULL) {
		result = bmi270_rtio_drain_cq(r, result);
		if (iodev_sqe != NULL) {
			stream_error(dev, data, iodev_sqe, result);
		}
		bmi270_fifo_job_abort(dev);
		return;
	}

	result = bmi270_rtio_drain_cq(r, result);
	if (result < 0) {
		stream_error(dev, data, iodev_sqe, result);
		bmi270_fifo_job_abort(dev);
		return;
	}

	if (!(data->int_status_1 & (BMI270_INT_STATUS_1_FWM_INT | BMI270_INT_STATUS_1_FFULL_INT))) {
		adv_power_save_enable_log_failure(dev);
		data->streaming_sqe = NULL;
		rtio_iodev_sqe_ok(iodev_sqe, 0);
		bmi270_fifo_job_complete(dev);
		return;
	}

	fifo_len = sys_get_le16(data->fifo_status) & 0x3FFF;
	if (fifo_len == 0) {
		adv_power_save_enable_log_failure(dev);
		data->streaming_sqe = NULL;
		rtio_iodev_sqe_ok(iodev_sqe, 0);
		bmi270_fifo_job_complete(dev);
		return;
	}

	max_fifo = CONFIG_BMI270_FIFO_STREAM_BLOCK_SIZE - sizeof(struct bmi270_fifo_encoded_data);
	fifo_len_orig = fifo_len;
	data->fifo_drain_len = 0U;

	if (fifo_len > max_fifo) {
		LOG_WRN("FIFO len %u > max %zu, capping and draining remainder", fifo_len,
			max_fifo);
		fifo_len = (uint16_t)max_fifo;
		data->fifo_drain_len = fifo_len_orig - fifo_len;
	}

	data->fifo_len = fifo_len;
	min_len = sizeof(struct bmi270_fifo_encoded_data) + fifo_len;

	ret = rtio_sqe_rx_buf(iodev_sqe, min_len, min_len, &buf, &buf_len);
	if (ret != 0 || buf_len < min_len) {
		stream_error(dev, data, iodev_sqe, -ENOMEM);
		bmi270_fifo_job_abort(dev);
		return;
	}

	edata = (struct bmi270_fifo_encoded_data *)buf;
	fifo_fill_encoded_header(edata, data, fifo_len);
	data->fifo_data_buf = edata->fifo_data;

	bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DATA);
}

static void bmi270_submit_fifo_data_job(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->streaming_sqe;
	struct rtio_sqe *cb_sqe;
	uint16_t drained;
	int ret;

	if (iodev_sqe == NULL) {
		bmi270_fifo_job_abort(dev);
		return;
	}

	ret = bmi270_prep_reg_read_async(dev, BMI270_REG_FIFO_DATA, data->fifo_data_buf,
					 data->fifo_len, RTIO_SQE_CHAINED);
	if (ret < 0) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DATA);
		return;
	}

	if (data->fifo_drain_len > 0U) {
		ret = bmi270_prep_fifo_drain_async(dev, BMI270_FIFO_DATA_JOB_DRAIN_READS,
						   &drained);
		if (ret < 0) {
			rtio_sqe_drop_all(data->rtio_ctx);
			bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DATA);
			return;
		}

		cb_sqe = rtio_sqe_acquire(data->rtio_ctx);
		if (cb_sqe == NULL) {
			rtio_sqe_drop_all(data->rtio_ctx);
			bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DATA);
			return;
		}
		rtio_sqe_prep_callback_no_cqe(cb_sqe, bmi270_fifo_drain_done_cb, (void *)dev,
					      NULL);
		data->fifo_drain_len -= drained;

		rtio_submit(data->rtio_ctx, 0);
		return;
	}

	ret = bmi270_prep_reg_read_async(dev, BMI270_REG_INT_STATUS_1, &data->int_status_1, 1,
					 RTIO_SQE_CHAINED);
	if (ret < 0) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DATA);
		return;
	}

	cb_sqe = rtio_sqe_acquire(data->rtio_ctx);
	if (cb_sqe == NULL) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DATA);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmi270_fifo_read_done_cb, (void *)dev, NULL);

	rtio_submit(data->rtio_ctx, 0);
}

static void bmi270_submit_fifo_drain_job(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->streaming_sqe;
	struct rtio_sqe *cb_sqe;
	uint16_t drained;
	int ret;

	if (iodev_sqe == NULL) {
		bmi270_fifo_job_abort(dev);
		return;
	}

	if (data->fifo_drain_len == 0U) {
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_CLEAR_STATUS);
		return;
	}

	ret = bmi270_prep_fifo_drain_async(dev, BMI270_FIFO_DRAIN_JOB_DRAIN_READS, &drained);
	if (ret < 0) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DRAIN);
		return;
	}

	cb_sqe = rtio_sqe_acquire(data->rtio_ctx);
	if (cb_sqe == NULL) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_DRAIN);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmi270_fifo_drain_done_cb, (void *)dev, NULL);
	data->fifo_drain_len -= drained;

	rtio_submit(data->rtio_ctx, 0);
}

static void bmi270_submit_fifo_clear_status_job(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->streaming_sqe;
	struct rtio_sqe *cb_sqe;
	int ret;

	if (iodev_sqe == NULL) {
		bmi270_fifo_job_abort(dev);
		return;
	}

	ret = bmi270_prep_reg_read_async(dev, BMI270_REG_INT_STATUS_1, &data->int_status_1, 1,
					 RTIO_SQE_CHAINED);
	if (ret < 0) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_CLEAR_STATUS);
		return;
	}

	cb_sqe = rtio_sqe_acquire(data->rtio_ctx);
	if (cb_sqe == NULL) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_CLEAR_STATUS);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmi270_fifo_read_done_cb, (void *)dev, NULL);

	rtio_submit(data->rtio_ctx, 0);
}

void bmi270_stream_handle_fifo(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe;
	struct rtio_sqe *cb_sqe;
	struct mpsc_node *job;
	k_spinlock_key_t key;
	uint64_t cycles;
	int ret;

	key = k_spin_lock(&data->fifo_job_lock);
	if (data->fifo_job_processing) {
		k_spin_unlock(&data->fifo_job_lock, key);
		return;
	}

	job = mpsc_pop(&data->fifo_jobs);
	if (job == NULL) {
		data->fifo_job_queued = false;
		k_spin_unlock(&data->fifo_job_lock, key);
		return;
	}
	data->fifo_job_queued = false;
	data->fifo_job_processing = true;
	k_spin_unlock(&data->fifo_job_lock, key);

	iodev_sqe = data->streaming_sqe;
	if (iodev_sqe == NULL) {
		bmi270_fifo_job_abort(dev);
		return;
	}

	if (data->fifo_job_phase == BMI270_FIFO_JOB_DATA) {
		bmi270_submit_fifo_data_job(dev);
		return;
	}

	if (data->fifo_job_phase == BMI270_FIFO_JOB_DRAIN) {
		bmi270_submit_fifo_drain_job(dev);
		return;
	}

	if (data->fifo_job_phase == BMI270_FIFO_JOB_CLEAR_STATUS) {
		bmi270_submit_fifo_clear_status_job(dev);
		return;
	}

	LOG_DBG("FIFO INT: handling watermark");

	if (sensor_clock_get_cycles(&cycles) == 0) {
		data->timestamp = sensor_clock_cycles_to_ns(cycles);
	}

	ret = bmi270_prep_reg_read_async(dev, BMI270_REG_INT_STATUS_1, &data->int_status_1, 1,
					 RTIO_SQE_CHAINED);
	if (ret < 0) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_STATUS);
		return;
	}

	ret = bmi270_prep_reg_read_async(dev, BMI270_REG_FIFO_LENGTH_0, data->fifo_status, 2,
					 RTIO_SQE_CHAINED);
	if (ret < 0) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_STATUS);
		return;
	}

	cb_sqe = rtio_sqe_acquire(data->rtio_ctx);
	if (cb_sqe == NULL) {
		rtio_sqe_drop_all(data->rtio_ctx);
		bmi270_requeue_fifo_job_work(dev, BMI270_FIFO_JOB_STATUS);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmi270_read_fifo_cb, (void *)dev, NULL);

	rtio_submit(data->rtio_ctx, 0);
}

void bmi270_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct bmi270_data *data = dev->data;
	const struct bmi270_config *config = dev->config;
	struct sensor_stream_trigger *fifo_wm =
		get_read_config_trigger(cfg, SENSOR_TRIG_FIFO_WATERMARK);
	struct sensor_stream_trigger *fifo_full =
		get_read_config_trigger(cfg, SENSOR_TRIG_FIFO_FULL);
	bool use_wm = (fifo_wm != NULL);
	bool use_full = (fifo_full != NULL);
	const struct gpio_dt_spec *fifo_pin_submit = fifo_pin(config);
	uint8_t flush_cmd, stat1;
	uint64_t cycles;
	int ret;

	if (!fifo_pin_submit->port) {
		LOG_ERR("FIFO stream requires %s (irq-gpios)",
			IS_ENABLED(CONFIG_BMI270_FIFO_ON_INT2) ? "INT2" : "INT1");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	if (!use_wm && !use_full) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	if (sensor_clock_get_cycles(&cycles) == 0) {
		data->timestamp = sensor_clock_cycles_to_ns(cycles);
	} else {
		data->timestamp = 0;
	}

	/*
	 * Disable adv_power_save before the register burst. In suspend mode
	 * the BMI270 requires 450 µs between SPI transactions; normal mode
	 * only needs 2 µs. Re-enabled in bmi270_stream_handle_fifo() after
	 * all SPI work is done.
	 */
	ret = bmi270_adv_power_save_disable(dev);
	if (ret < 0) {
		LOG_ERR("adv_power_save_disable failed: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	/*
	 * RTIO_SQE_MULTISHOT re-invokes this submit() on every FIFO cycle, not just at
	 * stream start, so only run the one-time FIFO/INT-map/flush setup once.
	 */
	if (!data->fifo_configured) {
		data->fifo_watermark_bytes = fifo_watermark_bytes();

		ret = configure_fifo(dev, true, data->fifo_watermark_bytes);
		if (ret < 0) {
			LOG_ERR("FIFO config failed: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}

		ret = map_fifo_int(dev, use_wm, use_full);
		if (ret < 0) {
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}

		/* Flush FIFO so first interrupt starts from a clean state */
		flush_cmd = BMI270_CMD_FIFO_FLUSH;
		ret = bmi270_reg_write(dev, BMI270_REG_CMD, &flush_cmd, 1);
		if (ret < 0) {
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}

		/*
		 * FIFO was just flushed so fill level is 0 (below watermark).
		 * Reading INT_STATUS_1 clears the FWM/FFULL latch, which
		 * deasserts the INT pin so the GPIO sees a clean LOW before
		 * arming.
		 */
		bmi270_reg_read(dev, BMI270_REG_INT_STATUS_1, &stat1, 1);

		ret = gpio_pin_interrupt_configure_dt(fifo_pin_submit, GPIO_INT_DISABLE);
		if (ret != 0) {
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}

		data->fifo_configured = true;

		LOG_DBG("Stream submitted (wm %u bytes, pin=%d)", data->fifo_watermark_bytes,
			gpio_pin_get_dt(fifo_pin_submit));
	}

	/* Re-arm on every call: bmi270_try_submit_fifo_irq() disables this on every IRQ. */
	data->streaming_sqe = iodev_sqe;
	ret = gpio_pin_interrupt_configure_dt(fifo_pin_submit, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

#if defined(CONFIG_BMI270_FIFO_POLL_FALLBACK)
	if (!poll_work_inited) {
		k_work_init_delayable(&poll_work, poll_work_fn);
		poll_work_inited = true;
	}
	poll_dev = dev;
	k_work_schedule(&poll_work, K_MSEC(FIFO_POLL_MS));
#endif
}

#endif /* CONFIG_BMI270_STREAM */
