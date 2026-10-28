/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Deere & Company
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle927x

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/tle927x.h>

#define TLE927X_SPI_FRAME_LEN 2U
#define TLE927X_SPI_CMD_IDX   0U
#define TLE927X_SPI_DATA_IDX  1U
#define TLE927X_SPI_WRITE     BIT(7)
#define TLE927X_SPI_DONT_CARE 0x00U

#define TLE927X_STATUS_CLEAR 0x00U

#define TLE927X_WK_CTRL_2_ALL_OFF    0x00U
#define TLE927X_MODE_CHANGE_DELAY_US 10

struct mfd_tle927x_config {
	struct spi_dt_spec spi;
	uint8_t vcc1_ctrl;
	bool fsi_disable;
	bool wk_disable;
};

struct mfd_tle927x_data {
	struct k_mutex lock;
	enum tle927x_mode mode;
	enum tle927x_boot_cause boot_cause;
	uint8_t wk_stat_1;
	uint8_t wk_stat_2;
};

static int tle927x_xfer_locked(const struct device *dev, uint8_t cmd, uint8_t data, uint8_t *resp)
{
	const struct mfd_tle927x_config *config = dev->config;
	uint8_t tx_bytes[TLE927X_SPI_FRAME_LEN];
	uint8_t rx_bytes[TLE927X_SPI_FRAME_LEN];
	int ret;

	const struct spi_buf tx_buf = {
		.buf = tx_bytes,
		.len = sizeof(tx_bytes),
	};
	const struct spi_buf rx_buf = {
		.buf = rx_bytes,
		.len = sizeof(rx_bytes),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1U,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1U,
	};

	tx_bytes[TLE927X_SPI_CMD_IDX] = cmd;
	tx_bytes[TLE927X_SPI_DATA_IDX] = data;

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret != 0) {
		return ret;
	}

	if (resp != NULL) {
		*resp = rx_bytes[TLE927X_SPI_DATA_IDX];
	}

	return 0;
}

static int tle927x_read_reg_locked(const struct device *dev, uint8_t reg, uint8_t *val)
{
	return tle927x_xfer_locked(dev, reg, TLE927X_SPI_DONT_CARE, val);
}

static int tle927x_write_reg_locked(const struct device *dev, uint8_t reg, uint8_t val)
{
	return tle927x_xfer_locked(dev, reg | TLE927X_SPI_WRITE, val, NULL);
}

static int tle927x_update_reg_locked(const struct device *dev, uint8_t reg, uint8_t mask,
				     uint8_t val)
{
	struct mfd_tle927x_data *data = dev->data;
	uint8_t old, check;
	int ret;

	ret = tle927x_read_reg_locked(dev, reg, &old);
	if (ret != 0) {
		return ret;
	}

	ret = tle927x_read_reg_locked(dev, reg, &check);
	if (ret != 0) {
		return ret;
	}

	if (old != check) {
		return -EIO;
	}

	if ((reg == TLE927X_REG_MS_CTRL) && ((mask & TLE927X_MS_CTRL_MODE_MASK) == 0U)) {
		mask |= TLE927X_MS_CTRL_MODE_MASK;
		val = (val & ~TLE927X_MS_CTRL_MODE_MASK) |
		      FIELD_PREP(TLE927X_MS_CTRL_MODE_MASK, data->mode);
	}

	return tle927x_write_reg_locked(dev, reg, (old & ~mask) | (val & mask));
}

int mfd_tle927x_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	struct mfd_tle927x_data *data = dev->data;
	int ret;

	if (val == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = tle927x_read_reg_locked(dev, reg, val);
	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_tle927x_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	struct mfd_tle927x_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((reg < TLE927X_REG_SUP_STAT) && (data->mode != TLE927X_MODE_NORMAL)) {
		ret = -EBUSY;
	} else {
		ret = tle927x_write_reg_locked(dev, reg, val);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_tle927x_update_reg(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	struct mfd_tle927x_data *data = dev->data;
	int ret;

	if ((reg == TLE927X_REG_WD_CTRL) || (reg >= TLE927X_REG_SUP_STAT)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->mode != TLE927X_MODE_NORMAL) {
		ret = -EBUSY;
	} else {
		ret = tle927x_update_reg_locked(dev, reg, mask, val);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_tle927x_set_mode(const struct device *dev, enum tle927x_mode mode)
{
	struct mfd_tle927x_data *data = dev->data;
	uint8_t ms_ctrl;
	int ret;

	switch (mode) {
	case TLE927X_MODE_NORMAL:
	case TLE927X_MODE_STOP:
		break;
	case TLE927X_MODE_SLEEP:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tle927x_update_reg_locked(dev, TLE927X_REG_MS_CTRL, TLE927X_MS_CTRL_MODE_MASK,
					FIELD_PREP(TLE927X_MS_CTRL_MODE_MASK, mode));
	if (ret != 0) {
		goto unlock;
	}

	k_usleep(TLE927X_MODE_CHANGE_DELAY_US);

	ret = tle927x_read_reg_locked(dev, TLE927X_REG_MS_CTRL, &ms_ctrl);
	if (ret != 0) {
		goto unlock;
	}

	if (FIELD_GET(TLE927X_MS_CTRL_MODE_MASK, ms_ctrl) != mode) {
		ret = -EIO;
		goto unlock;
	}

	data->mode = mode;

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_tle927x_get_mode(const struct device *dev, enum tle927x_mode *mode)
{
	struct mfd_tle927x_data *data = dev->data;

	if (mode == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	*mode = data->mode;
	k_mutex_unlock(&data->lock);

	return 0;
}

int mfd_tle927x_get_boot_cause(const struct device *dev, enum tle927x_boot_cause *cause,
			       uint8_t *wk_stat_1, uint8_t *wk_stat_2)
{
	struct mfd_tle927x_data *data = dev->data;

	if (cause != NULL) {
		*cause = data->boot_cause;
	}

	if (wk_stat_1 != NULL) {
		*wk_stat_1 = data->wk_stat_1;
	}

	if (wk_stat_2 != NULL) {
		*wk_stat_2 = data->wk_stat_2;
	}

	return 0;
}

int mfd_tle927x_read_faults(const struct device *dev, uint8_t *sup, uint8_t *therm,
			    uint8_t *dev_stat)
{
	struct mfd_tle927x_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (sup != NULL) {
		ret = tle927x_read_reg_locked(dev, TLE927X_REG_SUP_STAT, sup);
		if (ret != 0) {
			goto unlock;
		}
	}

	if (therm != NULL) {
		ret = tle927x_read_reg_locked(dev, TLE927X_REG_THERM_STAT, therm);
		if (ret != 0) {
			goto unlock;
		}
	}

	if (dev_stat != NULL) {
		ret = tle927x_read_reg_locked(dev, TLE927X_REG_DEV_STAT, dev_stat);
	}

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mfd_tle927x_capture_boot_cause(const struct device *dev)
{
	const uint8_t status_regs[] = {
		TLE927X_REG_SUP_STAT,   TLE927X_REG_THERM_STAT, TLE927X_REG_DEV_STAT,
		TLE927X_REG_BUS_STAT_1, TLE927X_REG_BUS_STAT_2, TLE927X_REG_WK_STAT_1,
		TLE927X_REG_WK_STAT_2,  TLE927X_REG_SMPS_STAT,
	};
	struct mfd_tle927x_data *data = dev->data;
	uint8_t dev_stat, sup_stat;
	size_t i;
	int ret;

	ret = mfd_tle927x_read_reg(dev, TLE927X_REG_DEV_STAT, &dev_stat);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_tle927x_read_reg(dev, TLE927X_REG_SUP_STAT, &sup_stat);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_tle927x_read_reg(dev, TLE927X_REG_WK_STAT_1, &data->wk_stat_1);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_tle927x_read_reg(dev, TLE927X_REG_WK_STAT_2, &data->wk_stat_2);
	if (ret != 0) {
		return ret;
	}

	if ((sup_stat & TLE927X_SUP_STAT_POR) != 0U) {
		data->boot_cause = TLE927X_BOOT_CAUSE_POWER_ON;
	} else {
		switch (FIELD_GET(TLE927X_DEV_STAT_PREV, dev_stat)) {
		case TLE927X_PREV_RESTART_FAILURE:
			data->boot_cause = TLE927X_BOOT_CAUSE_FAILURE;
			break;
		case TLE927X_PREV_WAKE_FROM_SLEEP:
			data->boot_cause = TLE927X_BOOT_CAUSE_WAKE_FROM_SLEEP;
			break;
		default:
			data->boot_cause = TLE927X_BOOT_CAUSE_UNKNOWN;
			break;
		}
	}

	for (i = 0U; i < ARRAY_SIZE(status_regs); i++) {
		ret = mfd_tle927x_write_reg(dev, status_regs[i], TLE927X_STATUS_CLEAR);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int mfd_tle927x_disable_wk(const struct device *dev)
{
	uint8_t wk_ctrl_2;
	int ret;

	ret = mfd_tle927x_write_reg(dev, TLE927X_REG_WK_CTRL_2, TLE927X_WK_CTRL_2_ALL_OFF);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_tle927x_read_reg(dev, TLE927X_REG_WK_CTRL_2, &wk_ctrl_2);
	if (ret != 0) {
		return ret;
	}

	if ((wk_ctrl_2 & TLE927X_WK_CTRL_2_WK_EN) != 0U) {
		return -EIO;
	}

	return 0;
}

static int mfd_tle927x_init(const struct device *dev)
{
	const struct mfd_tle927x_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	ret = mfd_tle927x_capture_boot_cause(dev);
	if (ret != 0) {
		return ret;
	}

	if (config->fsi_disable) {
		ret = mfd_tle927x_update_reg(dev, TLE927X_REG_HW_CTRL, TLE927X_HW_CTRL_FSI_FO2_MASK,
					     TLE927X_HW_CTRL_FSI_FO2_MASK);
		if (ret != 0) {
			return ret;
		}
	}

	if (config->wk_disable) {
		ret = mfd_tle927x_disable_wk(dev);
		if (ret != 0) {
			return ret;
		}
	}

	ret = mfd_tle927x_update_reg(
		dev, TLE927X_REG_MS_CTRL,
		TLE927X_MS_CTRL_VCC1_RT_MASK | TLE927X_MS_CTRL_VCC1_OV_RST_MASK, config->vcc1_ctrl);
	if (ret != 0) {
		return ret;
	}

	return mfd_tle927x_set_mode(dev, TLE927X_MODE_NORMAL);
}

#define TLE927X_SPI_OPERATION                                                                      \
	(SPI_OP_MODE_CONTROLLER | SPI_MODE_CPHA | SPI_TRANSFER_LSB | SPI_WORD_SET(8))

#define MFD_TLE927X_DEFINE(inst)                                                                   \
	static struct mfd_tle927x_data mfd_tle927x_data_##inst = {                                 \
		.lock = Z_MUTEX_INITIALIZER(mfd_tle927x_data_##inst.lock),                         \
	};                                                                                         \
                                                                                                   \
	static const struct mfd_tle927x_config mfd_tle927x_config_##inst = {                       \
		.spi = SPI_DT_SPEC_INST_GET(inst, TLE927X_SPI_OPERATION),                          \
		.fsi_disable = DT_INST_PROP(inst, fsi_disable),                                    \
		.wk_disable = DT_INST_PROP(inst, wk_disable),                                      \
		.vcc1_ctrl =                                                                       \
			FIELD_PREP(TLE927X_MS_CTRL_VCC1_RT_MASK,                                   \
				   DT_INST_ENUM_IDX(inst, reset_threshold)) |                      \
			(DT_INST_PROP(inst, overvoltage_reset) ? TLE927X_MS_CTRL_VCC1_OV_RST_MASK  \
							       : 0U),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_tle927x_init, NULL, &mfd_tle927x_data_##inst,              \
			      &mfd_tle927x_config_##inst, POST_KERNEL,                             \
			      CONFIG_MFD_TLE927X_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_TLE927X_DEFINE)
