/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <zephyr/drivers/sensor/lis2dh.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "lis2dh.h"

LOG_MODULE_DECLARE(lis2dh, CONFIG_SENSOR_LOG_LEVEL);

#define LIS2DH_NSEC_PER_SEC 1000000000ULL

static int lis2dh_fifo_period_ns(const struct device *dev, uint64_t *period_ns)
{
	struct lis2dh_data *lis2dh = dev->data;
	uint32_t frequency_millihz;
	uint8_t ctrl1;
	uint8_t odr;
	int status;

	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL1, &ctrl1);
	if (status < 0) {
		return status;
	}

	odr = (ctrl1 & LIS2DH_ODR_MASK) >> LIS2DH_ODR_SHIFT;
	switch (odr) {
	case LIS2DH_ODR_1:
		frequency_millihz = 1000U;
		break;
	case LIS2DH_ODR_2:
		frequency_millihz = 10000U;
		break;
	case LIS2DH_ODR_3:
		frequency_millihz = 25000U;
		break;
	case LIS2DH_ODR_4:
		frequency_millihz = 50000U;
		break;
	case LIS2DH_ODR_5:
		frequency_millihz = 100000U;
		break;
	case LIS2DH_ODR_6:
		frequency_millihz = 200000U;
		break;
	case LIS2DH_ODR_7:
		frequency_millihz = 400000U;
		break;
	case LIS2DH_ODR_8:
		if ((ctrl1 & LIS2DH_LP_EN_BIT_MASK) == 0U) {
			return -EINVAL;
		}
		frequency_millihz = 1620000U;
		break;
	case LIS2DH_ODR_9:
		frequency_millihz = (ctrl1 & LIS2DH_LP_EN_BIT_MASK) != 0U ? 5376000U : 1344000U;
		break;
	default:
		return -EINVAL;
	}

	*period_ns = (LIS2DH_NSEC_PER_SEC * 1000U) / frequency_millihz;

	return 0;
}

static void lis2dh_fifo_clear(struct lis2dh_data *lis2dh)
{
	lis2dh->fifo_head = 0U;
	lis2dh->fifo_tail = 0U;
	lis2dh->fifo_count = 0U;
	lis2dh->fifo_cache_valid = false;
}

static void lis2dh_fifo_push(struct lis2dh_data *lis2dh, const int16_t xyz[3],
			     uint64_t timestamp_ns)
{
	uint16_t head = lis2dh->fifo_head;

	memcpy(lis2dh->fifo_samples[head].xyz, xyz, sizeof(lis2dh->fifo_samples[head].xyz));
	lis2dh->fifo_samples[head].timestamp_ns = timestamp_ns;
	lis2dh->fifo_head = (head + 1U) % CONFIG_LIS2DH_FIFO_SW_QUEUE_SAMPLES;

	if (lis2dh->fifo_count == CONFIG_LIS2DH_FIFO_SW_QUEUE_SAMPLES) {
		lis2dh->fifo_tail = (lis2dh->fifo_tail + 1U) % CONFIG_LIS2DH_FIFO_SW_QUEUE_SAMPLES;
#ifdef CONFIG_LIS2DH_FIFO_STATS
		if (lis2dh->fifo_dropped_samples < INT32_MAX) {
			lis2dh->fifo_dropped_samples++;
		}
#endif
	} else {
		lis2dh->fifo_count++;
	}
}

static void lis2dh_fifo_convert(int16_t raw, uint32_t scale, struct sensor_value *value)
{
	int32_t converted;

	converted = (raw >> 4) * scale;
	value->val1 = converted / 1000000;
	value->val2 = converted % 1000000;
}

int lis2dh_fifo_init(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;

	lis2dh_fifo_clear(lis2dh);
	atomic_clear(&lis2dh->fifo_active);
#ifdef CONFIG_LIS2DH_STREAM
	lis2dh_stream_init(dev);
#endif

	return 0;
}

bool lis2dh_fifo_is_active(const struct device *dev)
{
	const struct lis2dh_data *lis2dh = dev->data;

	return atomic_get(&lis2dh->fifo_active) != 0;
}

bool lis2dh_fifo_is_busy(const struct device *dev)
{
	const struct lis2dh_data *lis2dh = dev->data;

	return lis2dh_fifo_is_active(dev) || lis2dh->fifo_faulted;
}

static int lis2dh_fifo_restore(const struct device *dev, uint8_t ctrl3, uint8_t ctrl5,
			       uint8_t fifo_ctrl)
{
	struct lis2dh_data *lis2dh = dev->data;
	int first_error = 0;
	int status;
	uint8_t value;

	/* Attempt every write even when the bus reports an earlier failure. */
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL3, ctrl3);
	if (status < 0) {
		first_error = status;
	}

	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL, fifo_ctrl);
	if (status < 0 && first_error == 0) {
		first_error = status;
	}

	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL5, ctrl5);
	if (status < 0 && first_error == 0) {
		first_error = status;
	}

	/* FIFO start requires INT1 to be unused before configuration. */
	status = lis2dh_trigger_int1_set(dev, false);
	if (status < 0 && first_error == 0) {
		first_error = status;
	}

	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL3, &value);
	if (status < 0 || value != ctrl3) {
		if (first_error == 0) {
			first_error = status < 0 ? status : -EIO;
		}
	}
	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_FIFO_CTRL, &value);
	if (status < 0 || value != fifo_ctrl) {
		if (first_error == 0) {
			first_error = status < 0 ? status : -EIO;
		}
	}
	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL5, &value);
	if (status < 0 || value != ctrl5) {
		if (first_error == 0) {
			first_error = status < 0 ? status : -EIO;
		}
	}

	return first_error;
}

static void lis2dh_fifo_debug_registers(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	uint8_t ctrl[5];
	uint8_t fifo[2];
	int status;

	if (COND_CODE_1(CONFIG_LOG, (CONFIG_SENSOR_LOG_LEVEL), (LOG_LEVEL_NONE)) < LOG_LEVEL_DBG) {
		return;
	}

	/* Read only control/status registers, never XYZ or the HP reference.
	 * Diagnostic failures do not change the result of FIFO start.
	 */
	status = lis2dh->hw_tf->read_data(dev, LIS2DH_REG_CTRL1, ctrl, sizeof(ctrl));
	if (status == 0) {
		LOG_DBG("%s: FIFO CTRL1=0x%02x CTRL2=0x%02x CTRL3=0x%02x", dev->name, ctrl[0],
			ctrl[1], ctrl[2]);
		LOG_DBG("%s: FIFO CTRL4=0x%02x CTRL5=0x%02x", dev->name, ctrl[3], ctrl[4]);
	} else {
		LOG_DBG("%s: FIFO diagnostic CTRL1..CTRL5 read failed: %d", dev->name, status);
	}

	status = lis2dh->hw_tf->read_data(dev, LIS2DH_REG_FIFO_CTRL, fifo, sizeof(fifo));
	if (status == 0) {
		LOG_DBG("%s: FIFO_CTRL=0x%02x FIFO_SRC=0x%02x", dev->name, fifo[0], fifo[1]);
	} else {
		LOG_DBG("%s: FIFO diagnostic FIFO_CTRL/FIFO_SRC read failed: %d", dev->name,
			status);
	}
}

int lis2dh_fifo_start(const struct device *dev)
{
	const struct lis2dh_config *cfg = dev->config;
	struct lis2dh_data *lis2dh = dev->data;
	uint8_t ctrl3;
	uint8_t ctrl5;
	uint8_t fifo_ctrl;
	uint8_t routes = LIS2DH_EN_FIFO_WTM_INT1 | LIS2DH_EN_FIFO_OVRN_INT1;
	int rollback_status;
	int status;

	if (cfg->gpio_drdy.port == NULL) {
		return -ENOTSUP;
	}

	lis2dh_lock(dev);

	if (lis2dh_fifo_is_busy(dev)) {
		status = -EBUSY;
		goto unlock;
	}

	if (lis2dh->handler_drdy != NULL) {
		status = -EBUSY;
		goto unlock;
	}

	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL3, &ctrl3);
	if (status < 0) {
		goto unlock;
	}
	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL5, &ctrl5);
	if (status < 0) {
		goto unlock;
	}
	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_FIFO_CTRL, &fifo_ctrl);
	if (status < 0) {
		goto unlock;
	}

	if ((ctrl3 & (LIS2DH_EN_CLICK_INT1 | LIS2DH_EN_IA_INT1 | LIS2DH_EN_DRDY1_INT1)) != 0U) {
		status = -EBUSY;
		goto unlock;
	}

	lis2dh->fifo_saved[0] = ctrl3;
	lis2dh->fifo_saved[1] = ctrl5;
	lis2dh->fifo_saved[2] = fifo_ctrl;

	status = lis2dh_fifo_period_ns(dev, &lis2dh->fifo_period_ns);
	if (status < 0) {
		goto unlock;
	}

	status = lis2dh_trigger_int1_set(dev, false);
	if (status < 0) {
		goto unlock;
	}

	lis2dh->fifo_restore_pending = true;
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL, LIS2DH_FIFO_MODE_BYPASS);
	if (status < 0) {
		goto rollback;
	}

	status = lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL5, LIS2DH_EN_FIFO, LIS2DH_EN_FIFO);
	if (status < 0) {
		goto rollback;
	}

	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL,
					  LIS2DH_FIFO_MODE_STREAM | (cfg->fifo_watermark - 1U));
	if (status < 0) {
		goto rollback;
	}

#ifdef CONFIG_LIS2DH_STREAM
	if (lis2dh->stream_active) {
		routes = lis2dh->stream_routes;
	}
#endif
	status = lis2dh->hw_tf->update_reg(
		dev, LIS2DH_REG_CTRL3, LIS2DH_EN_FIFO_WTM_INT1 | LIS2DH_EN_FIFO_OVRN_INT1, routes);
	if (status < 0) {
		goto rollback;
	}

	lis2dh_fifo_clear(lis2dh);
	atomic_set(&lis2dh->fifo_active, 1);
	status = lis2dh_trigger_fifo_int1_set(dev, true);
	if (status < 0) {
		atomic_clear(&lis2dh->fifo_active);
		goto rollback;
	}

	lis2dh->fifo_restore_pending = false;
	LOG_DBG("%s: FIFO started watermark=%u period=%llu ns INT1=%s.%u routes=0x%02x", dev->name,
		cfg->fifo_watermark, (unsigned long long)lis2dh->fifo_period_ns,
		cfg->gpio_drdy.port->name, cfg->gpio_drdy.pin, routes);
	lis2dh_fifo_debug_registers(dev);
	goto unlock;

rollback:
	LOG_DBG("%s: FIFO start failed: %d; restoring registers", dev->name, status);
	atomic_clear(&lis2dh->fifo_active);
	if (lis2dh->fifo_restore_pending) {
		rollback_status = lis2dh_fifo_restore(dev, ctrl3, ctrl5, fifo_ctrl);
		lis2dh->fifo_faulted = rollback_status < 0;
		lis2dh->fifo_restore_pending = rollback_status < 0;
		if (rollback_status < 0) {
			LOG_ERR("FIFO start rollback failed: %d", rollback_status);
		}
	}

unlock:
	lis2dh_unlock(dev);

	return status;
}

int lis2dh_fifo_stop(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status = 0;
#ifdef CONFIG_LIS2DH_STREAM
	struct rtio_iodev_sqe *sqe;
#endif

	lis2dh_lock(dev);
#ifdef CONFIG_LIS2DH_STREAM
	sqe = lis2dh->streaming_sqe;
	lis2dh->streaming_sqe = NULL;
	lis2dh->stream_active = false;
	lis2dh->stream_iodev = NULL;
	lis2dh->stream_nop_events = 0U;
	(void)k_work_cancel_delayable(&lis2dh->stream_work);
#endif
	if (lis2dh_fifo_is_busy(dev)) {
		atomic_clear(&lis2dh->fifo_active);
		if (!lis2dh->fifo_restore_pending) {
			/* INT1 was unowned at start; preserve unrelated register bits. */
			lis2dh->fifo_saved[0] &=
				~(LIS2DH_EN_FIFO_WTM_INT1 | LIS2DH_EN_FIFO_OVRN_INT1);
			lis2dh->fifo_saved[1] &= ~LIS2DH_EN_FIFO;
			lis2dh->fifo_saved[2] = LIS2DH_FIFO_MODE_BYPASS;
		}
		status = lis2dh_fifo_restore(dev, lis2dh->fifo_saved[0], lis2dh->fifo_saved[1],
					     lis2dh->fifo_saved[2]);
		lis2dh->fifo_faulted = status < 0;
		lis2dh->fifo_restore_pending = status < 0;
		lis2dh_fifo_clear(lis2dh);
		LOG_DBG("%s: FIFO stop result=%d faulted=%u", dev->name, status,
			lis2dh->fifo_faulted);
	}
#ifdef CONFIG_LIS2DH_STREAM
	/* Complete only after cleanup; the executor may immediately reuse the SQE. */
	if (sqe != NULL) {
		rtio_iodev_sqe_err(sqe, status < 0 ? status : -ECANCELED);
	}
#endif
	lis2dh_unlock(dev);
	return status;
}

#ifdef CONFIG_LIS2DH_STREAM
int lis2dh_fifo_drop(const struct device *dev)
{
	const struct lis2dh_config *cfg = dev->config;
	struct lis2dh_data *lis2dh = dev->data;
	int status;

	lis2dh_lock(dev);
	if (!lis2dh_fifo_is_active(dev)) {
		status = -EACCES;
		goto unlock;
	}

	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL, LIS2DH_FIFO_MODE_BYPASS);
	if (status < 0) {
		goto unlock;
	}

	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL,
					  LIS2DH_FIFO_MODE_STREAM | (cfg->fifo_watermark - 1U));
	if (status == 0) {
		lis2dh_fifo_clear(lis2dh);
	}

unlock:
	lis2dh_unlock(dev);

	return status;
}
#endif

int lis2dh_fifo_sample_fetch(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status = 0;

	lis2dh_lock(dev);
	if (!lis2dh->fifo_cache_valid) {
		status = -ENODATA;
	}
	lis2dh_unlock(dev);

	return status;
}

int lis2dh_fifo_cache_copy(const struct device *dev, union lis2dh_sample *sample)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status = 0;

	lis2dh_lock(dev);
	if (!lis2dh_fifo_is_active(dev) || !lis2dh->fifo_cache_valid) {
		status = -ENODATA;
	} else {
		memcpy(sample, &lis2dh->sample, sizeof(*sample));
	}
	lis2dh_unlock(dev);

	return status;
}

int lis2dh_fifo_read(const struct device *dev, struct lis2dh_fifo_sample *samples, size_t capacity,
		     size_t *count)
{
	struct lis2dh_data *lis2dh = dev->data;
	size_t sample_count;
	size_t i;

	if (count == NULL || (capacity > 0U && samples == NULL)) {
		return -EINVAL;
	}

	lis2dh_lock(dev);
	if (!lis2dh_fifo_is_active(dev)) {
		lis2dh_unlock(dev);
		return -EACCES;
	}

	sample_count = MIN(capacity, lis2dh->fifo_count);
	for (i = 0U; i < sample_count; i++) {
		uint16_t tail = lis2dh->fifo_tail;
		size_t axis;

		for (axis = 0U; axis < ARRAY_SIZE(samples[i].accel); axis++) {
			lis2dh_fifo_convert(lis2dh->fifo_samples[tail].xyz[axis], lis2dh->scale,
					    &samples[i].accel[axis]);
		}
		samples[i].timestamp_ns = lis2dh->fifo_samples[tail].timestamp_ns;
		lis2dh->fifo_tail = (tail + 1U) % CONFIG_LIS2DH_FIFO_SW_QUEUE_SAMPLES;
		lis2dh->fifo_count--;
	}
	*count = sample_count;

	lis2dh_unlock(dev);

	return 0;
}

int lis2dh_fifo_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler)
{
	struct lis2dh_data *lis2dh = dev->data;

	if (trig->chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	lis2dh_lock(dev);
	if (trig->type == SENSOR_TRIG_FIFO_WATERMARK) {
		lis2dh->fifo_handler_watermark = handler;
		lis2dh->fifo_trig_watermark = trig;
	} else if (trig->type == SENSOR_TRIG_FIFO_FULL) {
		lis2dh->fifo_handler_full = handler;
		lis2dh->fifo_trig_full = trig;
	} else {
		lis2dh_unlock(dev);
		return -ENOTSUP;
	}
	lis2dh_unlock(dev);

	return 0;
}

int lis2dh_fifo_handle_irq(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	sensor_trigger_handler_t handler = NULL;
	const struct sensor_trigger *trig = NULL;
	uint8_t raw[LIS2DH_FIFO_MAX_BYTES];
	uint8_t src;
	uint8_t sample_count;
	uint64_t timestamp_ns;
	int status = 0;

	lis2dh_lock(dev);
	if (!lis2dh_fifo_is_active(dev)) {
		goto unlock;
	}
#ifdef CONFIG_LIS2DH_STREAM
	if (lis2dh->stream_active) {
		status = lis2dh_stream_handle_irq(dev);
		goto unlock;
	}
#endif
	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_FIFO_SRC, &src);
	if (status < 0) {
		goto finish;
	}
	timestamp_ns = lis2dh_timestamp_ns();
	if ((src & LIS2DH_FIFO_EMPTY) != 0U) {
		goto finish;
	}
	sample_count = (src & LIS2DH_FIFO_OVRN) != 0U ? LIS2DH_FIFO_MAX_SAMPLES
						      : src & LIS2DH_FIFO_FSS_MASK;
	if (sample_count == 0U) {
		goto finish;
	}
	status = lis2dh->hw_tf->read_data(dev, LIS2DH_REG_ACCEL_X_LSB, raw,
					  sample_count * LIS2DH_FIFO_SAMPLE_SIZE);
	if (status < 0) {
		goto finish;
	}
	timestamp_ns -= (sample_count - 1U) * lis2dh->fifo_period_ns;
	for (size_t i = 0U; i < sample_count; i++) {
		int16_t xyz[3];

		for (size_t axis = 0U; axis < ARRAY_SIZE(xyz); axis++) {
			xyz[axis] = (int16_t)sys_get_le16(
				&raw[i * LIS2DH_FIFO_SAMPLE_SIZE + axis * sizeof(int16_t)]);
		}
		lis2dh_fifo_push(lis2dh, xyz, timestamp_ns);
		memcpy(lis2dh->sample.xyz, xyz, sizeof(xyz));
		timestamp_ns += lis2dh->fifo_period_ns;
	}
	lis2dh->sample.status = LIS2DH_STATUS_ZYX_DRDY;
	lis2dh->fifo_cache_valid = true;
	if ((src & LIS2DH_FIFO_OVRN) != 0U && lis2dh->fifo_handler_full != NULL) {
		handler = lis2dh->fifo_handler_full;
		trig = lis2dh->fifo_trig_full;
	} else if ((src & LIS2DH_FIFO_WTM) != 0U) {
		handler = lis2dh->fifo_handler_watermark;
		trig = lis2dh->fifo_trig_watermark;
	}

finish:
	if (status == 0) {
		status = lis2dh_trigger_fifo_int1_set(dev, true);
	}
	if (status < 0) {
		(void)lis2dh_fifo_stop(dev);
	}
unlock:
	lis2dh_unlock(dev);
	if (status == 0 && handler != NULL) {
		handler(dev, trig);
	}
	return status;
}
