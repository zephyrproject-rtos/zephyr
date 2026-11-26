/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register facts: TPS65214 datasheet SLVSHK7A, TPS65215-Q1 datasheet SLVSHQ9.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/tps6521x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mfd_tps6521x, CONFIG_MFD_LOG_LEVEL);

/* TPS65214 only; on TPS65215 offset 0x03 is BUCKS_CONFIG */
#define TPS65214_REG_LOCK       0x03U
#define TPS65214_REG_UNLOCK_CMD 0x5AU
#define TPS65214_REG_LOCK_CMD   0x00U
/* first interrupt register: 0x2b-0x35 are read-only or write-one-to-clear */
#define TPS65214_REG_INT_SOURCE 0x2bU
#define TPS65214_REG_SPARE_2    0x36U
#define TPS65214_REG_SPARE_3    0x37U

/*
 * TI_DEV_ID. TPS65214 (SLVSHK7A table 8-3): bits 7-5 NVM revision, bits 4-0
 * device ID. 0x14 as read from a TPS6521401 on a TMDS62LEVM; the TRMs
 * SLVUD30A and SLVUD82 give device ID 0x04. TPS65215 (SLVSHQ9 table 6-9):
 * bits 7-6 temperature grade, bits 5-0 GPN; a TPS6521501-Q1 reads 0x95
 * (SLVUCW5).
 */
#define TPS65214_DEVICE_ID 0x14U
#define TPS65215_GPN       0x15U

#define TPS65214_DEV_ID_NVM_REV GENMASK(7, 5)
#define TPS65214_DEV_ID_DEVICE  GENMASK(4, 0)
#define TPS65215_DEV_ID_GPN     GENMASK(5, 0)

struct mfd_tps6521x_config {
	struct i2c_dt_spec i2c;
	enum mfd_tps6521x_variant variant;
};

struct mfd_tps6521x_data {
	struct k_mutex lock;
};

/*
 * REG_LOCK protects every R/W register (SLVSHK7A 7.3.15) except MFP_CTRL and
 * USER_NVM_CMD (table 7-7). The interrupt and status registers 0x2b-0x35 are
 * not R/W.
 */
static bool mfd_tps6521x_is_protected(const struct mfd_tps6521x_config *config, uint8_t reg)
{
	if (config->variant != MFD_TPS6521X_VARIANT_TPS65214) {
		return false;
	}

	if (reg < TPS65214_REG_INT_SOURCE) {
		return reg != TPS65214_REG_LOCK && reg != MFD_TPS6521X_REG_MFP_CTRL;
	}

	return reg == TPS65214_REG_SPARE_2 || reg == TPS65214_REG_SPARE_3;
}

int mfd_tps6521x_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct mfd_tps6521x_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

/*
 * The lock is always restored after a write: REG_LOCK reads back 0 and any
 * value other than 0x5A locks the protected registers again (SLVSHK7A table
 * 8-6). Nothing else may write to the PMIC while the driver runs.
 */
int mfd_tps6521x_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;
	bool protected = mfd_tps6521x_is_protected(config, reg);
	uint8_t old;
	uint8_t new;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &old);
	if (ret < 0) {
		goto out;
	}

	new = (old & ~mask) | (val & mask);
	if (new == old) {
		goto out;
	}

	if (protected) {
		ret = i2c_reg_write_byte_dt(&config->i2c, TPS65214_REG_LOCK,
					    TPS65214_REG_UNLOCK_CMD);
		if (ret < 0) {
			goto out;
		}
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, reg, new);

	if (protected) {
		/* lock again even when the write failed */
		int lock_ret = i2c_reg_write_byte_dt(&config->i2c, TPS65214_REG_LOCK,
						     TPS65214_REG_LOCK_CMD);

		if (ret == 0) {
			ret = lock_ret;
		}
	}

out:
	(void)k_mutex_unlock(&data->lock);

	return ret;
}

enum mfd_tps6521x_variant mfd_tps6521x_get_variant(const struct device *dev)
{
	const struct mfd_tps6521x_config *config = dev->config;

	return config->variant;
}

static int mfd_tps6521x_init(const struct device *dev)
{
	const struct mfd_tps6521x_config *config = dev->config;
	struct mfd_tps6521x_data *data = dev->data;
	uint8_t tps65214_id;
	uint8_t tps65215_gpn;
	uint8_t id;
	int ret;

	(void)k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, MFD_TPS6521X_REG_TI_DEV_ID, &id);
	if (ret < 0) {
		LOG_ERR("PMIC not responding (%d)", ret);
		return ret;
	}

	tps65214_id = FIELD_GET(TPS65214_DEV_ID_DEVICE, id);
	tps65215_gpn = FIELD_GET(TPS65215_DEV_ID_GPN, id);

	/*
	 * The other variant's ID is a wrong node, and its register map would
	 * write the wrong registers. Any other value only warns: the ID is per
	 * orderable part, and not every part's value is published.
	 */
	if (config->variant == MFD_TPS6521X_VARIANT_TPS65215) {
		if (tps65214_id == TPS65214_DEVICE_ID) {
			LOG_ERR("device ID 0x%02x is a TPS65214, the node says TPS65215", id);
			return -ENODEV;
		}
		if (tps65215_gpn != TPS65215_GPN) {
			LOG_WRN("device ID 0x%02x, expected GPN 0x%02x", id, TPS65215_GPN);
		}
	} else {
		if (tps65215_gpn == TPS65215_GPN) {
			LOG_ERR("device ID 0x%02x is a TPS65215, the node says TPS65214", id);
			return -ENODEV;
		}
		if (tps65214_id != TPS65214_DEVICE_ID) {
			LOG_WRN("device ID 0x%02x, expected device 0x%02x", id,
				TPS65214_DEVICE_ID);
		}
		LOG_DBG("NVM revision %lu", FIELD_GET(TPS65214_DEV_ID_NVM_REV, id));
	}

	return 0;
}

#define MFD_TPS6521X_DEFINE(node_id, _variant)                                                     \
	static struct mfd_tps6521x_data mfd_tps6521x_data_##node_id;                               \
	static const struct mfd_tps6521x_config mfd_tps6521x_config_##node_id = {                  \
		.i2c = I2C_DT_SPEC_GET(node_id),                                                   \
		.variant = _variant,                                                               \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, mfd_tps6521x_init, NULL, &mfd_tps6521x_data_##node_id,           \
			 &mfd_tps6521x_config_##node_id, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,    \
			 NULL);

#define MFD_TPS65214_DEFINE(node_id) MFD_TPS6521X_DEFINE(node_id, MFD_TPS6521X_VARIANT_TPS65214)
#define MFD_TPS65215_DEFINE(node_id) MFD_TPS6521X_DEFINE(node_id, MFD_TPS6521X_VARIANT_TPS65215)

DT_FOREACH_STATUS_OKAY(ti_tps65214, MFD_TPS65214_DEFINE)
DT_FOREACH_STATUS_OKAY(ti_tps65215, MFD_TPS65215_DEFINE)
