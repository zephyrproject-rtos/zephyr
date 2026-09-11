/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <zephyr/logging/log.h>

#include "lis2dh.h"

LOG_MODULE_DECLARE(lis2dh, CONFIG_SENSOR_LOG_LEVEL);

#define LIS2DH_NSEC_PER_SEC 1000000000ULL

static int lis2dh_fifo_period_ns(const struct device *dev, uint64_t *period_ns)
{
	struct lis2dh_data *data = dev->data;
	uint32_t frequency_millihz;
	uint8_t ctrl1;
	uint8_t odr;
	int status;

	status = data->hw_tf->read_reg(dev, LIS2DH_REG_CTRL1, &ctrl1);
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

int lis2dh_fifo_init(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;

	atomic_clear(&data->fifo_active);
	lis2dh_stream_init(dev);
	return 0;
}

bool lis2dh_fifo_is_active(const struct device *dev)
{
	const struct lis2dh_data *data = dev->data;

	return atomic_get(&data->fifo_active) != 0;
}

bool lis2dh_fifo_is_busy(const struct device *dev)
{
	const struct lis2dh_data *data = dev->data;

	return lis2dh_fifo_is_active(dev) || data->fifo_faulted;
}

static int lis2dh_fifo_restore(const struct device *dev, uint8_t ctrl3, uint8_t ctrl5,
			       uint8_t fifo_ctrl)
{
	struct lis2dh_data *data = dev->data;
	int first_error = 0;
	int status;
	uint8_t value;

	/* Attempt every write even when the bus reports an earlier failure. */
	status = data->hw_tf->write_reg(dev, LIS2DH_REG_CTRL3, ctrl3);
	if (status < 0) {
		first_error = status;
	}
	status = data->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL, fifo_ctrl);
	if (status < 0 && first_error == 0) {
		first_error = status;
	}
	status = data->hw_tf->write_reg(dev, LIS2DH_REG_CTRL5, ctrl5);
	if (status < 0 && first_error == 0) {
		first_error = status;
	}
	status = lis2dh_trigger_int1_set(dev, false);
	if (status < 0 && first_error == 0) {
		first_error = status;
	}

	status = data->hw_tf->read_reg(dev, LIS2DH_REG_CTRL3, &value);
	if (status < 0 || value != ctrl3) {
		if (first_error == 0) {
			first_error = status < 0 ? status : -EIO;
		}
	}
	status = data->hw_tf->read_reg(dev, LIS2DH_REG_FIFO_CTRL, &value);
	if (status < 0 || value != fifo_ctrl) {
		if (first_error == 0) {
			first_error = status < 0 ? status : -EIO;
		}
	}
	status = data->hw_tf->read_reg(dev, LIS2DH_REG_CTRL5, &value);
	if (status < 0 || value != ctrl5) {
		if (first_error == 0) {
			first_error = status < 0 ? status : -EIO;
		}
	}

	return first_error;
}

static void lis2dh_fifo_debug_registers(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;
	uint8_t ctrl[5];
	uint8_t fifo[2];
	int status;

	if (COND_CODE_1(CONFIG_LOG, (CONFIG_SENSOR_LOG_LEVEL), (LOG_LEVEL_NONE)) < LOG_LEVEL_DBG) {
		return;
	}

	status = data->hw_tf->read_data(dev, LIS2DH_REG_CTRL1, ctrl, sizeof(ctrl));
	if (status == 0) {
		LOG_DBG("%s: FIFO CTRL1=0x%02x CTRL2=0x%02x CTRL3=0x%02x", dev->name, ctrl[0],
			ctrl[1], ctrl[2]);
		LOG_DBG("%s: FIFO CTRL4=0x%02x CTRL5=0x%02x", dev->name, ctrl[3], ctrl[4]);
	}
	status = data->hw_tf->read_data(dev, LIS2DH_REG_FIFO_CTRL, fifo, sizeof(fifo));
	if (status == 0) {
		LOG_DBG("%s: FIFO_CTRL=0x%02x FIFO_SRC=0x%02x", dev->name, fifo[0], fifo[1]);
	}
}

int lis2dh_fifo_start(const struct device *dev)
{
	const struct lis2dh_config *cfg = dev->config;
	struct lis2dh_data *data = dev->data;
	uint8_t ctrl3;
	uint8_t ctrl5;
	uint8_t fifo_ctrl;
	int rollback_status;
	int status;

	if (cfg->fifo_watermark < 1U || cfg->fifo_watermark > LIS2DH_FIFO_MAX_SAMPLES) {
		return -EINVAL;
	}
	if (cfg->gpio_drdy.port == NULL) {
		return -ENOTSUP;
	}

	lis2dh_lock(dev);
	if (lis2dh_fifo_is_busy(dev) || data->handler_drdy != NULL) {
		status = -EBUSY;
		goto unlock;
	}
	status = data->hw_tf->read_reg(dev, LIS2DH_REG_CTRL3, &ctrl3);
	if (status < 0) {
		goto unlock;
	}
	status = data->hw_tf->read_reg(dev, LIS2DH_REG_CTRL5, &ctrl5);
	if (status < 0) {
		goto unlock;
	}
	status = data->hw_tf->read_reg(dev, LIS2DH_REG_FIFO_CTRL, &fifo_ctrl);
	if (status < 0) {
		goto unlock;
	}
	if ((ctrl3 & LIS2DH_FIFO_CONFLICT_INT1_MASK) != 0U) {
		status = -EBUSY;
		goto unlock;
	}

	data->fifo_saved[0] = ctrl3;
	data->fifo_saved[1] = ctrl5;
	data->fifo_saved[2] = fifo_ctrl;
	status = lis2dh_fifo_period_ns(dev, &data->fifo_period_ns);
	if (status < 0) {
		goto unlock;
	}
	status = lis2dh_trigger_int1_set(dev, false);
	if (status < 0) {
		goto unlock;
	}

	data->fifo_restore_pending = true;
	status = data->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL, LIS2DH_FIFO_MODE_BYPASS);
	if (status < 0) {
		goto rollback;
	}
	status = data->hw_tf->update_reg(dev, LIS2DH_REG_CTRL5, LIS2DH_EN_FIFO, LIS2DH_EN_FIFO);
	if (status < 0) {
		goto rollback;
	}
	status = data->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL,
					LIS2DH_FIFO_MODE_STREAM | (cfg->fifo_watermark - 1U));
	if (status < 0) {
		goto rollback;
	}
	status = data->hw_tf->update_reg(dev, LIS2DH_REG_CTRL3, LIS2DH_FIFO_INT1_MASK,
					data->stream_routes);
	if (status < 0) {
		goto rollback;
	}

	atomic_set(&data->fifo_active, 1);
	status = lis2dh_trigger_fifo_int1_set(dev, true);
	if (status < 0) {
		atomic_clear(&data->fifo_active);
		goto rollback;
	}
	data->fifo_restore_pending = false;
	LOG_DBG("%s: FIFO started watermark=%u period=%llu ns", dev->name, cfg->fifo_watermark,
		(unsigned long long)data->fifo_period_ns);
	lis2dh_fifo_debug_registers(dev);
	goto unlock;

rollback:
	atomic_clear(&data->fifo_active);
	rollback_status = lis2dh_fifo_restore(dev, ctrl3, ctrl5, fifo_ctrl);
	data->fifo_faulted = rollback_status < 0;
	data->fifo_restore_pending = rollback_status < 0;
	if (rollback_status < 0) {
		LOG_ERR("FIFO start rollback failed: %d", rollback_status);
	}
unlock:
	lis2dh_unlock(dev);
	return status;
}

int lis2dh_fifo_stop(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;
	struct rtio_iodev_sqe *sqe;
	int status = 0;

	lis2dh_lock(dev);
	sqe = data->streaming_sqe;
	data->streaming_sqe = NULL;
	data->stream_active = false;
	data->stream_iodev = NULL;
	data->stream_nop_events = 0U;
	(void)k_work_cancel_delayable(&data->stream_work);
	if (lis2dh_fifo_is_busy(dev)) {
		atomic_clear(&data->fifo_active);
		if (!data->fifo_restore_pending) {
			data->fifo_saved[0] &= ~LIS2DH_FIFO_INT1_MASK;
			data->fifo_saved[1] &= ~LIS2DH_EN_FIFO;
			data->fifo_saved[2] = LIS2DH_FIFO_MODE_BYPASS;
		}
		status = lis2dh_fifo_restore(dev, data->fifo_saved[0], data->fifo_saved[1],
					     data->fifo_saved[2]);
		data->fifo_faulted = status < 0;
		data->fifo_restore_pending = status < 0;
	}
	if (sqe != NULL) {
		rtio_iodev_sqe_err(sqe, status < 0 ? status : -ECANCELED);
	}
	lis2dh_unlock(dev);
	return status;
}

int lis2dh_fifo_drop(const struct device *dev)
{
	const struct lis2dh_config *cfg = dev->config;
	struct lis2dh_data *data = dev->data;
	int status;

	lis2dh_lock(dev);
	if (!lis2dh_fifo_is_active(dev)) {
		status = -EACCES;
		goto unlock;
	}
	status = data->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL, LIS2DH_FIFO_MODE_BYPASS);
	if (status == 0) {
		status = data->hw_tf->write_reg(dev, LIS2DH_REG_FIFO_CTRL,
						LIS2DH_FIFO_MODE_STREAM |
							(cfg->fifo_watermark - 1U));
	}
unlock:
	lis2dh_unlock(dev);
	return status;
}

int lis2dh_fifo_handle_irq(const struct device *dev)
{
	int status;

	lis2dh_lock(dev);
	if (!lis2dh_fifo_is_active(dev)) {
		status = 0;
		goto unlock;
	}
	status = lis2dh_stream_handle_irq(dev);
	if (status == 0 && lis2dh_fifo_is_active(dev)) {
		status = lis2dh_trigger_fifo_int1_set(dev, true);
	}
	if (status < 0) {
		(void)lis2dh_fifo_stop(dev);
	}
unlock:
	lis2dh_unlock(dev);
	return status;
}
