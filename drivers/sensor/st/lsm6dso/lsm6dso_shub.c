/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#define DT_DRV_COMPAT st_lsm6dso

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "lsm6dso.h"

LOG_MODULE_DECLARE(LSM6DSO, CONFIG_SENSOR_LOG_LEVEL);

static int lsm6dso_shub_write_target_reg(const struct device *dev,
					 uint8_t trgt_addr, uint8_t trgt_reg,
					 uint8_t *value, uint16_t len);
static int lsm6dso_shub_read_target_reg(const struct device *dev,
					uint8_t trgt_addr, uint8_t trgt_reg,
					uint8_t *value, uint16_t len);
static void lsm6dso_shub_enable(const struct device *dev, uint8_t enable);


/* ST HAL skips this register, only supports it via the slower lsm6dso_sh_status_get() */
static int32_t lsm6dso_sh_status_mainpage_get(stmdev_ctx_t *ctx, lsm6dso_status_master_t *val)
{
	return lsm6dso_read_reg(ctx, LSM6DSO_STATUS_MASTER_MAINPAGE, (uint8_t *)val, 1);
}

/*
 * LIS2MDL magn device specific part
 */
#ifdef CONFIG_LSM6DSO_EXT_LIS2MDL

#define LIS2MDL_CFG_REG_A		0x60
#define LIS2MDL_CFG_REG_B		0x61
#define LIS2MDL_CFG_REG_C		0x62
#define LIS2MDL_STATUS_REG		0x67

#define LIS2MDL_SW_RESET		0x20
#define LIS2MDL_ODR_10HZ		0x00
#define LIS2MDL_ODR_100HZ		0x0C
#define LIS2MDL_OFF_CANC		0x02
#define LIS2MDL_SENSITIVITY		1500

static int lsm6dso_lis2mdl_init(const struct device *dev, uint8_t i2c_addr)
{
	struct lsm6dso_data *data = dev->data;
	uint8_t mag_cfg[2];

	data->magn_gain = LIS2MDL_SENSITIVITY;

	/* sw reset device */
	mag_cfg[0] = LIS2MDL_SW_RESET;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LIS2MDL_CFG_REG_A, mag_cfg, 1);

	k_sleep(K_MSEC(10)); /* turn-on time in ms */

	/* configure mag */
	mag_cfg[0] = LIS2MDL_ODR_10HZ;
	mag_cfg[1] = LIS2MDL_OFF_CANC;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LIS2MDL_CFG_REG_A, mag_cfg, 2);

	return 0;
}

static const uint16_t lis2mdl_map[] = {10, 20, 50, 100};

static int lsm6dso_lis2mdl_odr_set(const struct device *dev,
				   uint8_t i2c_addr, uint16_t freq)
{
	uint8_t odr, cfg;

	for (odr = 0; odr < ARRAY_SIZE(lis2mdl_map); odr++) {
		if (freq == lis2mdl_map[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(lis2mdl_map)) {
		LOG_DBG("shub: LIS2MDL freq val %d not supported.", freq);
		return -ENOTSUP;
	}

	cfg = (odr << 2);
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LIS2MDL_CFG_REG_A, &cfg, 1);

	lsm6dso_shub_enable(dev, 1);
	return 0;
}

static int lsm6dso_lis2mdl_conf(const struct device *dev, uint8_t i2c_addr,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_lis2mdl_odr_set(dev, i2c_addr, val->val1);
	default:
		LOG_DBG("shub: LIS2MDL attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_LSM6DSO_EXT_LIS2MDL */

/*
 * HTS221 humidity device specific part
 */
#ifdef CONFIG_LSM6DSO_EXT_HTS221

#define HTS221_AUTOINCREMENT		BIT(7)

#define HTS221_REG_CTRL1		0x20
#define HTS221_ODR_1HZ			0x01
#define HTS221_BDU			0x04
#define HTS221_PD			0x80

#define HTS221_REG_CONV_START		0x30

static int lsm6dso_hts221_read_conv_data(const struct device *dev,
					uint8_t i2c_addr)
{
	struct lsm6dso_data *data = dev->data;
	uint8_t buf[16], i;
	struct hts221_data *ht = &data->hts221;

	for (i = 0; i < sizeof(buf); i += 7) {
		unsigned char len = MIN(7, sizeof(buf) - i);

		if (lsm6dso_shub_read_target_reg(dev, i2c_addr,
						 (HTS221_REG_CONV_START + i) |
						 HTS221_AUTOINCREMENT,
						 &buf[i], len) < 0) {
			LOG_DBG("shub: failed to read hts221 conv data");
			return -EIO;
		}
	}

	ht->y0 = buf[0] / 2;
	ht->y1 = buf[1] / 2;
	ht->x0 = buf[6] | (buf[7] << 8);
	ht->x1 = buf[10] | (buf[11] << 8);

	return 0;
}

static int lsm6dso_hts221_init(const struct device *dev, uint8_t i2c_addr)
{
	uint8_t hum_cfg;

	/* configure ODR and BDU */
	hum_cfg = HTS221_ODR_1HZ | HTS221_BDU | HTS221_PD;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      HTS221_REG_CTRL1, &hum_cfg, 1);

	return lsm6dso_hts221_read_conv_data(dev, i2c_addr);
}

static const uint16_t hts221_map[] = {0, 1, 7, 12};

static int lsm6dso_hts221_odr_set(const struct device *dev,
				   uint8_t i2c_addr, uint16_t freq)
{
	uint8_t odr, cfg;

	for (odr = 0; odr < ARRAY_SIZE(hts221_map); odr++) {
		if (freq == hts221_map[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(hts221_map)) {
		LOG_DBG("shub: HTS221 freq val %d not supported.", freq);
		return -ENOTSUP;
	}

	cfg = odr | HTS221_BDU | HTS221_PD;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      HTS221_REG_CTRL1, &cfg, 1);

	lsm6dso_shub_enable(dev, 1);
	return 0;
}

static int lsm6dso_hts221_conf(const struct device *dev, uint8_t i2c_addr,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_hts221_odr_set(dev, i2c_addr, val->val1);
	default:
		LOG_DBG("shub: HTS221 attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_LSM6DSO_EXT_HTS221 */

/*
 * LPS22HB baro/temp device specific part
 */
#ifdef CONFIG_LSM6DSO_EXT_LPS22HB

#define LPS22HB_CTRL_REG1		0x10
#define LPS22HB_CTRL_REG2		0x11

#define LPS22HB_SW_RESET		0x04
#define LPS22HB_ODR_10HZ		0x20
#define LPS22HB_LPF_EN			0x08
#define LPS22HB_BDU_EN			0x02

static int lsm6dso_lps22hb_init(const struct device *dev, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HB_SW_RESET;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LPS22HB_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(1)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HB_ODR_10HZ | LPS22HB_LPF_EN | LPS22HB_BDU_EN;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LPS22HB_CTRL_REG1, baro_cfg, 1);

	return 0;
}
#endif /* CONFIG_LSM6DSO_EXT_LPS22HB */

/*
 * LPS22HH baro/temp device specific part
 */
#ifdef CONFIG_LSM6DSO_EXT_LPS22HH

#define LPS22HH_CTRL_REG1		0x10
#define LPS22HH_CTRL_REG2		0x11

#define LPS22HH_SW_RESET		0x04
#define LPS22HH_IF_ADD_INC		0x10
#define LPS22HH_ODR_10HZ		0x20
#define LPS22HH_LPF_EN			0x08
#define LPS22HH_BDU_EN			0x02

static int lsm6dso_lps22hh_init(const struct device *dev, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HH_SW_RESET;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LPS22HH_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(100)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HH_IF_ADD_INC;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LPS22HH_CTRL_REG2, baro_cfg, 1);

	baro_cfg[0] = LPS22HH_ODR_10HZ | LPS22HH_LPF_EN | LPS22HH_BDU_EN;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LPS22HH_CTRL_REG1, baro_cfg, 1);

	return 0;
}

static const uint16_t lps22hh_map[] = {0, 1, 10, 25, 50, 75, 100, 200};

static int lsm6dso_lps22hh_odr_set(const struct device *dev,
				   uint8_t i2c_addr, uint16_t freq)
{
	uint8_t odr, cfg;

	for (odr = 0; odr < ARRAY_SIZE(lps22hh_map); odr++) {
		if (freq == lps22hh_map[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(lps22hh_map)) {
		LOG_DBG("shub: LPS22HH freq val %d not supported.", freq);
		return -ENOTSUP;
	}

	cfg = (odr << 4) | LPS22HH_LPF_EN | LPS22HH_BDU_EN;
	lsm6dso_shub_write_target_reg(dev, i2c_addr,
				      LPS22HH_CTRL_REG1, &cfg, 1);

	lsm6dso_shub_enable(dev, 1);
	return 0;
}

static int lsm6dso_lps22hh_conf(const struct device *dev, uint8_t i2c_addr,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_lps22hh_odr_set(dev, i2c_addr, val->val1);
	default:
		LOG_DBG("shub: LPS22HH attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_LSM6DSO_EXT_LPS22HH */

/* List of supported external sensors */
static struct lsm6dso_shub_slist {
	enum sensor_channel type;
	uint8_t i2c_addr[2];
	uint8_t ext_i2c_addr;
	uint8_t wai_addr;
	uint8_t wai_val;
	uint8_t out_data_addr;
	uint8_t out_data_len;
	uint8_t sh_out_reg;
	int (*dev_init)(const struct device *dev, uint8_t i2c_addr);
	int (*dev_conf)(const struct device *dev, uint8_t i2c_addr,
			enum sensor_channel chan, enum sensor_attribute attr,
			const struct sensor_value *val);
} lsm6dso_shub_slist[] = {
#ifdef CONFIG_LSM6DSO_EXT_LIS2MDL
	{
		/* LIS2MDL */
		.type		= SENSOR_CHAN_MAGN_XYZ,
		.i2c_addr	= { 0x1E },
		.wai_addr	= 0x4F,
		.wai_val	= 0x40,
		.out_data_addr  = 0x68,
		.out_data_len   = 0x06,
		.dev_init	= (lsm6dso_lis2mdl_init),
		.dev_conf	= (lsm6dso_lis2mdl_conf),
	},
#endif /* CONFIG_LSM6DSO_EXT_LIS2MDL */

#ifdef CONFIG_LSM6DSO_EXT_HTS221
	{
		/* HTS221 */
		.type		= SENSOR_CHAN_HUMIDITY,
		.i2c_addr	= { 0x5F },
		.wai_addr	= 0x0F,
		.wai_val	= 0xBC,
		.out_data_addr  = 0x28 | HTS221_AUTOINCREMENT,
		.out_data_len   = 0x02,
		.dev_init	= (lsm6dso_hts221_init),
		.dev_conf	= (lsm6dso_hts221_conf),
	},
#endif /* CONFIG_LSM6DSO_EXT_HTS221 */

#ifdef CONFIG_LSM6DSO_EXT_LPS22HB
	{
		/* LPS22HB */
		.type		= SENSOR_CHAN_PRESS,
		.i2c_addr	= { 0x5C, 0x5D },
		.wai_addr	= 0x0F,
		.wai_val	= 0xB1,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x05,
		.dev_init	= (lsm6dso_lps22hb_init),
	},
#endif /* CONFIG_LSM6DSO_EXT_LPS22HB */

#ifdef CONFIG_LSM6DSO_EXT_LPS22HH
	{
		/* LPS22HH */
		.type		= SENSOR_CHAN_PRESS,
		.i2c_addr	= { 0x5C, 0x5D },
		.wai_addr	= 0x0F,
		.wai_val	= 0xB3,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x05,
		.dev_init	= (lsm6dso_lps22hh_init),
		.dev_conf	= (lsm6dso_lps22hh_conf),
	},
#endif /* CONFIG_LSM6DSO_EXT_LPS22HH */
};

static int lsm6dso_shub_wait_completed(stmdev_ctx_t *ctx)
{
	lsm6dso_status_master_t status;
	int tries = 200; /* Should be max ~160 ms, from 2 cycles at slowest ODR 12.5 Hz */

	do {
		if (!--tries) {
			LOG_DBG("shub: Timeout waiting for operation to complete");
			return -ETIMEDOUT;
		}
		k_msleep(1);
		lsm6dso_sh_status_mainpage_get(ctx, &status);
	} while (status.sens_hub_endop == 0);

	return 1;
}

static void lsm6dso_shub_enable(const struct device *dev, uint8_t enable)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dso_data *data = dev->data;

	/* Enable Accel @26hz */
	if (!data->accel_freq) {
		uint8_t odr = (enable) ? 2 : 0;

		if (lsm6dso_xl_data_rate_set(ctx, odr) < 0) {
			LOG_DBG("shub: failed to set XL sampling rate");
			return;
		}
	}

	if (enable) {
		lsm6dso_status_master_t status;

		/* Clear any pending status flags */
		lsm6dso_sh_status_mainpage_get(ctx, &status);
	}

	if (lsm6dso_sh_master_set(ctx, enable) < 0) {
		LOG_DBG("shub: failed to set master on");
		lsm6dso_mem_bank_set(ctx, LSM6DSO_USER_BANK);
		return;
	}

	if (!enable) {
		/* wait 300us (necessary per AN5192 §7.2.1) */
		k_busy_wait(300);
	}
}

/* must be called with master on */
static int lsm6dso_shub_check_slv0_nack(stmdev_ctx_t *ctx)
{
	lsm6dso_status_master_t status;

	if (lsm6dso_sh_status_get(ctx, &status) < 0) {
		LOG_DBG("shub: error reading embedded reg");
		return -EIO;
	}

	if (status.slave0_nack) {
		LOG_DBG("shub: TRGT 0 nacked");
		return -EIO;
	}

	return 0;
}

/*
 * use TRGT 0 for generic read to target device
 */
static int lsm6dso_shub_read_target_reg(const struct device *dev,
					uint8_t trgt_addr, uint8_t trgt_reg,
					uint8_t *value, uint16_t len)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso_sh_cfg_read_t trgt_cfg;

	trgt_cfg.slv_add = trgt_addr;
	trgt_cfg.slv_subadd = trgt_reg;
	trgt_cfg.slv_len = len;

	lsm6dso_sh_slv_cfg_read(ctx, 0, &trgt_cfg);

	/* turn SH on, wait for shub i2c read to finish */
	lsm6dso_shub_enable(dev, 1);
	lsm6dso_shub_wait_completed(ctx);

	/* read data from external target */
	if (lsm6dso_sh_read_data_raw_get(ctx, value, len) < 0) {
		LOG_DBG("shub: error reading sensor data");
		return -EIO;
	}

	if (lsm6dso_shub_check_slv0_nack(ctx) < 0) {
		lsm6dso_shub_enable(dev, 0);
		return -EIO;
	}

	lsm6dso_shub_enable(dev, 0);
	return 0;
}

/*
 * use TRGT 0 to configure target device
 */
static int lsm6dso_shub_write_target_reg(const struct device *dev,
					 uint8_t trgt_addr, uint8_t trgt_reg,
					 uint8_t *value, uint16_t len)
{
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dso_sh_cfg_write_t trgt_cfg;
	uint8_t cnt = 0U;

	lsm6dso_shub_enable(dev, 0);

	while (cnt < len) {
		trgt_cfg.slv0_add = trgt_addr;
		trgt_cfg.slv0_subadd = trgt_reg + cnt;
		trgt_cfg.slv0_data = value[cnt];

		lsm6dso_sh_cfg_write(ctx, &trgt_cfg);

		/* turn SH on, wait for shub i2c write to finish */
		lsm6dso_shub_enable(dev, 1);
		lsm6dso_shub_wait_completed(ctx);

		if (lsm6dso_shub_check_slv0_nack(ctx) < 0) {
			lsm6dso_shub_enable(dev, 0);
			return -EIO;
		}

		lsm6dso_shub_enable(dev, 0);

		cnt++;
	}

	/* Put TRGT 0 in IDLE mode */
	trgt_cfg.slv0_add = 0x7;
	trgt_cfg.slv0_subadd = 0x0;
	trgt_cfg.slv0_data = 0x0;
	lsm6dso_sh_cfg_write(ctx, &trgt_cfg);

	return 0;
}

/*
 * TARGETs configurations:
 *
 *  - TARGET 0: used for configuring all target devices
 *  - TARGET 1: used as data read channel for external target device #1
 *  - TARGET 2: used as data read channel for external target device #2
 *  - TARGET 3: used for generic reads while data channel is enabled
 */
static int lsm6dso_shub_set_data_channel(const struct device *dev)
{
	struct lsm6dso_data *data = dev->data;
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t n;
	struct lsm6dso_shub_slist *sp;
	lsm6dso_sh_cfg_read_t trgt_cfg;

	/* Configure shub data channels to access external targets */
	for (n = 0; n < data->num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[data->shub_ext[n]];

		trgt_cfg.slv_add = sp->ext_i2c_addr;
		trgt_cfg.slv_subadd = sp->out_data_addr;
		trgt_cfg.slv_len = sp->out_data_len;

		if (lsm6dso_sh_slv_cfg_read(ctx, n + 1, &trgt_cfg) < 0) {
			LOG_DBG("shub: error configuring shub for ext targets");
			return -EIO;
		}
	}

	/* Configure the master */
	lsm6dso_aux_sens_on_t aux = LSM6DSO_SLV_0_1_2;

	if (lsm6dso_sh_slave_connected_set(ctx, aux) < 0) {
		LOG_DBG("shub: error setting aux sensors");
		return -EIO;
	}


	/* turn SH on, no need to wait for 1st shub i2c read, if any, to complete */
	lsm6dso_shub_enable(dev, 1);

	return 0;
}

int lsm6dso_shub_get_idx(const struct device *dev, enum sensor_channel type)
{
	uint8_t n;
	struct lsm6dso_data *data = dev->data;
	struct lsm6dso_shub_slist *sp;

	for (n = 0; n < data->num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[data->shub_ext[n]];

		if (sp->type == type) {
			return n;
		}
	}

	LOG_ERR("shub: dev %s type %d not supported", dev->name, type);
	return -ENOTSUP;
}

int lsm6dso_shub_fetch_external_devs(const struct device *dev)
{
	uint8_t n;
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dso_data *data = dev->data;
	struct lsm6dso_shub_slist *sp;

	/* read data from external target */
	if (lsm6dso_mem_bank_set(ctx, LSM6DSO_SENSOR_HUB_BANK) < 0) {
		LOG_DBG("failed to enter SENSOR_HUB bank");
		return -EIO;
	}

	for (n = 0; n < data->num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[data->shub_ext[n]];

		if (lsm6dso_read_reg(ctx, sp->sh_out_reg,
				     data->ext_data[n], sp->out_data_len) < 0) {
			LOG_DBG("shub: failed to read sample");
			(void) lsm6dso_mem_bank_set(ctx, LSM6DSO_USER_BANK);
			return -EIO;
		}
	}

	return lsm6dso_mem_bank_set(ctx, LSM6DSO_USER_BANK);
}

int lsm6dso_shub_config(const struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr,
			const struct sensor_value *val)
{
	struct lsm6dso_data *data = dev->data;
	struct lsm6dso_shub_slist *sp = NULL;
	uint8_t n;

	for (n = 0; n < data->num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[data->shub_ext[n]];

		if (sp->type == chan) {
			break;
		}
	}

	if (n == data->num_ext_dev) {
		LOG_DBG("shub: %s chan %d not supported", dev->name, chan);
		return -ENOTSUP;
	}

	if (sp == NULL || sp->dev_conf == NULL) {
		LOG_DBG("shub: chan not configurable");
		return -ENOTSUP;
	}

	return sp->dev_conf(dev, sp->ext_i2c_addr, chan, attr, val);
}

int lsm6dso_shub_init(const struct device *dev)
{
	struct lsm6dso_data *data = dev->data;
	const struct lsm6dso_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t i, n = 0, regn;
	uint8_t chip_id;
	struct lsm6dso_shub_slist *sp;

	LOG_INF("shub: start sensorhub for %s", dev->name);

	/* This must be set or lsm6dso_shub_write_target_reg() will repeatedly write the same reg */
	if (lsm6dso_sh_write_mode_set(ctx, LSM6DSO_ONLY_FIRST_CYCLE) < 0) {
		LOG_DBG("shub: error setting write once");
		return -EIO;
	}

	for (n = 0; n < ARRAY_SIZE(lsm6dso_shub_slist); n++) {
		if (data->num_ext_dev >= LSM6DSO_SHUB_MAX_NUM_TARGETS) {
			break;
		}

		chip_id = 0;
		sp = &lsm6dso_shub_slist[n];

		/*
		 * The external sensor may have different I2C address.
		 * So, try them one by one until we read the correct
		 * chip ID.
		 */
		for (i = 0U; i < ARRAY_SIZE(sp->i2c_addr); i++) {
			if (lsm6dso_shub_read_target_reg(dev,
							sp->i2c_addr[i],
							sp->wai_addr,
							&chip_id, 1) < 0) {
				LOG_DBG("shub: failed reading chip id");
				continue;
			}
			if (chip_id == sp->wai_val) {
				break;
			}
		}

		if (i >= ARRAY_SIZE(sp->i2c_addr)) {
			LOG_DBG("shub: invalid chip id 0x%x", chip_id);
			continue;
		}
		LOG_INF("shub: Ext Device Chip Id: %02x", chip_id);
		sp->ext_i2c_addr = sp->i2c_addr[i];

		data->shub_ext[data->num_ext_dev++] = n;
	}

	LOG_DBG("shub: dev %s - num_ext_dev %d", dev->name, data->num_ext_dev);
	if (data->num_ext_dev == 0) {
		LOG_ERR("shub: no target devices found");
		return -EINVAL;
	}

	/* init external devices */
	for (n = 0, regn = 0; n < data->num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[data->shub_ext[n]];
		sp->sh_out_reg = LSM6DSO_SENSOR_HUB_1 + regn;
		regn += sp->out_data_len;
		sp->dev_init(dev, sp->ext_i2c_addr);
	}

	lsm6dso_shub_set_data_channel(dev);

	return 0;
}
