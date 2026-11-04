/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/bq2589x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mfd_bq2589x, CONFIG_MFD_LOG_LEVEL);

/* Register 0x07 - Termination and Timer Control */
#define BQ2589X_REG_TERM_TIMER 0x07

#define BQ2589X_WATCHDOG_MASK GENMASK(5, 4)

#define BQ2589X_WATCHDOG_DISABLE 0x0

/* Register 0x14 - Device Information */
#define BQ2589X_REG_DEVICE_INFO 0x14

#define BQ2589X_PN_MASK      GENMASK(5, 3)
#define BQ2589X_DEV_REV_MASK GENMASK(1, 0)

#define BQ25896_PART_NUMBER 0x0
#define SY6970_PART_NUMBER  0x1

struct mfd_bq2589x_config {
	struct i2c_dt_spec i2c;
	uint8_t part_no;
};

struct mfd_bq2589x_data {
	struct k_mutex lock;
};

int mfd_bq2589x_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	const struct mfd_bq2589x_config *config = dev->config;
	struct mfd_bq2589x_data *data = dev->data;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	ret = i2c_reg_update_byte_dt(&config->i2c, reg, mask, val);
	(void)k_mutex_unlock(&data->lock);

	return ret;
}

static int mfd_bq2589x_init(const struct device *dev)
{
	const struct mfd_bq2589x_config *config = dev->config;
	struct mfd_bq2589x_data *data = dev->data;
	uint8_t reg, part_no, dev_rev;
	int ret;

	(void)k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_DEVICE_INFO, &reg);
	if (ret < 0) {
		LOG_ERR("Failed to read device information register (%d)", ret);
		return ret;
	}

	part_no = FIELD_GET(BQ2589X_PN_MASK, reg);
	if (part_no != config->part_no) {
		LOG_ERR("Unexpected part number: 0x%x", part_no);
		return -ENODEV;
	}

	dev_rev = FIELD_GET(BQ2589X_DEV_REV_MASK, reg);
	LOG_DBG("PN=0x%x, DEV_REV=0x%x", part_no, dev_rev);

	/*
	 * The I2C watchdog is not serviced. On expiry the device would go back
	 * to its default settings, so keep it disabled.
	 */
	return mfd_bq2589x_reg_update(dev, BQ2589X_REG_TERM_TIMER, BQ2589X_WATCHDOG_MASK,
				      FIELD_PREP(BQ2589X_WATCHDOG_MASK, BQ2589X_WATCHDOG_DISABLE));
}

#define MFD_BQ2589X_DEFINE(inst, name, pn)                                                         \
	static const struct mfd_bq2589x_config name##_config_##inst = {                            \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.part_no = pn,                                                                     \
	};                                                                                         \
                                                                                                   \
	static struct mfd_bq2589x_data name##_data_##inst;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_bq2589x_init, NULL, &name##_data_##inst,                   \
			      &name##_config_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

#define DT_DRV_COMPAT ti_bq25896
DT_INST_FOREACH_STATUS_OKAY_VARGS(MFD_BQ2589X_DEFINE, bq25896, BQ25896_PART_NUMBER)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT silergy_sy6970
DT_INST_FOREACH_STATUS_OKAY_VARGS(MFD_BQ2589X_DEFINE, sy6970, SY6970_PART_NUMBER)
#undef DT_DRV_COMPAT
