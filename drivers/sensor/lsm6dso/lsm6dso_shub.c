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

#include <device.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "lsm6dso.h"

LOG_MODULE_DECLARE(LSM6DSO, CONFIG_SENSOR_LOG_LEVEL);

#define LSM6DSO_SHUB_DATA_OUT				0x02

#define LSM6DSO_SHUB_SLV0_ADDR				0x15
#define LSM6DSO_SHUB_SLV0_SUBADDR			0x16
#define LSM6DSO_SHUB_SLV0_CONFIG			0x17
#define LSM6DSO_SHUB_SLV1_ADDR				0x18
#define LSM6DSO_SHUB_SLV1_SUBADDR			0x19
#define LSM6DSO_SHUB_SLV1_CONFIG			0x1A
#define LSM6DSO_SHUB_SLV2_ADDR				0x1B
#define LSM6DSO_SHUB_SLV2_SUBADDR			0x1C
#define LSM6DSO_SHUB_SLV2_CONFIG			0x1D
#define LSM6DSO_SHUB_SLV3_ADDR				0x1E
#define LSM6DSO_SHUB_SLV3_SUBADDR			0x1F
#define LSM6DSO_SHUB_SLV3_CONFIG			0x20
#define LSM6DSO_SHUB_SLV0_DATAWRITE			0x21

#define LSM6DSO_SHUB_STATUS_MASTER			0x22
#define LSM6DSO_SHUB_STATUS_SLV0_NACK			BIT(3)
#define LSM6DSO_SHUB_STATUS_ENDOP			BIT(0)

#define LSM6DSO_SHUB_SLVX_WRITE				0x0
#define LSM6DSO_SHUB_SLVX_READ				0x1

static uint8_t num_ext_dev;
static uint8_t shub_ext[LSM6DSO_SHUB_MAX_NUM_SLVS];

static int lsm6dso_shub_write_slave_reg(struct lsm6dso_data *data,
					uint8_t slv_addr, uint8_t slv_reg,
					uint8_t *value, uint16_t len);
static int lsm6dso_shub_read_slave_reg(struct lsm6dso_data *data,
				       uint8_t slv_addr, uint8_t slv_reg,
				       uint8_t *value, uint16_t len);
static void lsm6dso_shub_enable(struct lsm6dso_data *data, uint8_t enable);

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

static int lsm6dso_lis2mdl_init(struct lsm6dso_data *data, uint8_t i2c_addr)
{
	uint8_t mag_cfg[2];

	data->magn_gain = LIS2MDL_SENSITIVITY;

	/* sw reset device */
	mag_cfg[0] = LIS2MDL_SW_RESET;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LIS2MDL_CFG_REG_A, mag_cfg, 1);

	k_sleep(K_MSEC(10)); /* turn-on time in ms */

	/* configure mag */
	mag_cfg[0] = LIS2MDL_ODR_10HZ;
	mag_cfg[1] = LIS2MDL_OFF_CANC;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LIS2MDL_CFG_REG_A, mag_cfg, 2);

	return 0;
}

static const uint16_t lis2mdl_map[] = {10, 20, 50, 100};

static int lsm6dso_lis2mdl_odr_set(struct lsm6dso_data *data,
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
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LIS2MDL_CFG_REG_A, &cfg, 1);

	lsm6dso_shub_enable(data, 1);
	return 0;
}

static int lsm6dso_lis2mdl_conf(struct lsm6dso_data *data, uint8_t i2c_addr,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_lis2mdl_odr_set(data, i2c_addr, val->val1);
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

static int lsmdso_hts221_read_conv_data(struct lsm6dso_data *data,
					uint8_t i2c_addr)
{
	uint8_t buf[16], i;
	struct hts221_data *ht = &data->hts221;

	for (i = 0; i < sizeof(buf); i += 7) {
		unsigned char len = MIN(7, sizeof(buf) - i);

		if (lsm6dso_shub_read_slave_reg(data, i2c_addr,
						(HTS221_REG_CONV_START + i) |
						HTS221_AUTOINCREMENT,
						&buf[i], len) < 0) {
			LOG_DBG("shub: failed to read hts221 conv data");
			return -EIO;
		}
	}

	ht->y0 = buf[0] / 2;
	ht->y1 = buf[1] / 2;
	ht->x0 = sys_le16_to_cpu(buf[6] | (buf[7] << 8));
	ht->x1 = sys_le16_to_cpu(buf[10] | (buf[11] << 8));

	return 0;
}

static int lsm6dso_hts221_init(struct lsm6dso_data *data, uint8_t i2c_addr)
{
	uint8_t hum_cfg;

	/* configure ODR and BDU */
	hum_cfg = HTS221_ODR_1HZ | HTS221_BDU | HTS221_PD;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     HTS221_REG_CTRL1, &hum_cfg, 1);

	return lsmdso_hts221_read_conv_data(data, i2c_addr);
}

static const uint16_t hts221_map[] = {0, 1, 7, 12};

static int lsm6dso_hts221_odr_set(struct lsm6dso_data *data,
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
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     HTS221_REG_CTRL1, &cfg, 1);

	lsm6dso_shub_enable(data, 1);
	return 0;
}

static int lsm6dso_hts221_conf(struct lsm6dso_data *data, uint8_t i2c_addr,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_hts221_odr_set(data, i2c_addr, val->val1);
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

static int lsm6dso_lps22hb_init(struct lsm6dso_data *data, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HB_SW_RESET;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LPS22HB_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(1)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HB_ODR_10HZ | LPS22HB_LPF_EN | LPS22HB_BDU_EN;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
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

static int lsm6dso_lps22hh_init(struct lsm6dso_data *data, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HH_SW_RESET;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LPS22HH_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(100)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HH_IF_ADD_INC;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LPS22HH_CTRL_REG2, baro_cfg, 1);

	baro_cfg[0] = LPS22HH_ODR_10HZ | LPS22HH_LPF_EN | LPS22HH_BDU_EN;
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LPS22HH_CTRL_REG1, baro_cfg, 1);

	return 0;
}

static const uint16_t lps22hh_map[] = {0, 1, 10, 25, 50, 75, 100, 200};

static int lsm6dso_lps22hh_odr_set(struct lsm6dso_data *data,
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
	lsm6dso_shub_write_slave_reg(data, i2c_addr,
				     LPS22HH_CTRL_REG1, &cfg, 1);

	lsm6dso_shub_enable(data, 1);
	return 0;
}

static int lsm6dso_lps22hh_conf(struct lsm6dso_data *data, uint8_t i2c_addr,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_lps22hh_odr_set(data, i2c_addr, val->val1);
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
	int (*dev_init)(struct lsm6dso_data *data, uint8_t i2c_addr);
	int (*dev_conf)(struct lsm6dso_data *data, uint8_t i2c_addr,
			enum sensor_channel chan, enum sensor_attribute attr,
			const struct sensor_value *val);
} lsm6dso_shub_slist[] = {
#ifdef CONFIG_LSM6DSO_EXT_LIS2MDL
	{
		/* LIS2MDL */
		.type		= SENSOR_CHAN_MAGN_XYZ,
		.i2c_addr       = { 0x1E },
		.wai_addr       = 0x4F,
		.wai_val        = 0x40,
		.out_data_addr  = 0x68,
		.out_data_len   = 0x06,
		.dev_init       = (lsm6dso_lis2mdl_init),
		.dev_conf       = (lsm6dso_lis2mdl_conf),
	},
#endif /* CONFIG_LSM6DSO_EXT_LIS2MDL */

#ifdef CONFIG_LSM6DSO_EXT_HTS221
	{
		/* HTS221 */
		.type		= SENSOR_CHAN_HUMIDITY,
		.i2c_addr       = { 0x5F },
		.wai_addr       = 0x0F,
		.wai_val        = 0xBC,
		.out_data_addr  = 0x28 | HTS221_AUTOINCREMENT,
		.out_data_len   = 0x02,
		.dev_init       = (lsm6dso_hts221_init),
		.dev_conf       = (lsm6dso_hts221_conf),
	},
#endif /* CONFIG_LSM6DSO_EXT_HTS221 */

#ifdef CONFIG_LSM6DSO_EXT_LPS22HB
	{
		/* LPS22HB */
		.type		= SENSOR_CHAN_PRESS,
		.i2c_addr       = { 0x5C, 0x5D },
		.wai_addr       = 0x0F,
		.wai_val        = 0xB1,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x05,
		.dev_init       = (lsm6dso_lps22hb_init),
	},
#endif /* CONFIG_LSM6DSO_EXT_LPS22HB */

#ifdef CONFIG_LSM6DSO_EXT_LPS22HH
	{
		/* LPS22HH */
		.type		= SENSOR_CHAN_PRESS,
		.i2c_addr       = { 0x5C, 0x5D },
		.wai_addr       = 0x0F,
		.wai_val        = 0xB3,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x05,
		.dev_init       = (lsm6dso_lps22hh_init),
		.dev_conf       = (lsm6dso_lps22hh_conf),
	},
#endif /* CONFIG_LSM6DSO_EXT_LPS22HH */
};

static inline void lsm6dso_shub_wait_completed(struct lsm6dso_data *data)
{
	uint16_t freq;

	freq = (data->accel_freq == 0) ? 26 : data->accel_freq;
	k_msleep((2000U / freq) + 1);
}

static inline void lsm6dso_shub_embedded_en(struct lsm6dso_data *data, bool on)
{
	if (on) {
		(void) lsm6dso_mem_bank_set(data->ctx, LSM6DSO_SENSOR_HUB_BANK);
	} else {
		(void) lsm6dso_mem_bank_set(data->ctx, LSM6DSO_USER_BANK);
	}

	k_busy_wait(150);
}

static int lsm6dso_shub_read_embedded_regs(struct lsm6dso_data *data,
					   uint8_t reg_addr,
					   uint8_t *value, int len)
{
	lsm6dso_shub_embedded_en(data, true);

	if (lsm6dso_read_reg(data->ctx, reg_addr, value, len) < 0) {
		LOG_DBG("shub: failed to read external reg: %02x", reg_addr);
		lsm6dso_shub_embedded_en(data, false);
		return -EIO;
	}

	lsm6dso_shub_embedded_en(data, false);

	return 0;
}

static int lsm6dso_shub_write_embedded_regs(struct lsm6dso_data *data,
					    uint8_t reg_addr,
					    uint8_t *value, uint8_t len)
{
	lsm6dso_shub_embedded_en(data, true);

	if (lsm6dso_write_reg(data->ctx, reg_addr, value, len) < 0) {
		LOG_DBG("shub: failed to write external reg: %02x", reg_addr);
		lsm6dso_shub_embedded_en(data, false);
		return -EIO;
	}

	lsm6dso_shub_embedded_en(data, false);

	return 0;
}

static void lsm6dso_shub_enable(struct lsm6dso_data *data, uint8_t enable)
{
	/* Enable Accel @26hz */
	if (!data->accel_freq) {
		uint8_t odr = (enable) ? 2 : 0;

		if (lsm6dso_xl_data_rate_set(data->ctx, odr) < 0) {
			LOG_DBG("shub: failed to set XL sampling rate");
			return;
		}
	}

	lsm6dso_shub_embedded_en(data, true);

	if (lsm6dso_sh_master_set(data->ctx, enable) < 0) {
		LOG_DBG("shub: failed to set master on");
		lsm6dso_shub_embedded_en(data, false);
		return;
	}

	lsm6dso_shub_embedded_en(data, false);
}

/* must be called with master on */
static int lsm6dso_shub_check_slv0_nack(struct lsm6dso_data *data)
{
	uint8_t status;

	if (lsm6dso_shub_read_embedded_regs(data, LSM6DSO_SHUB_STATUS_MASTER,
					     &status, 1) < 0) {
		LOG_DBG("shub: error reading embedded reg");
		return -EIO;
	}

	if (status & (LSM6DSO_SHUB_STATUS_SLV0_NACK)) {
		LOG_DBG("shub: SLV0 nacked");
		return -EIO;
	}

	return 0;
}

/*
 * use SLV0 for generic read to slave device
 */
static int lsm6dso_shub_read_slave_reg(struct lsm6dso_data *data,
				       uint8_t slv_addr, uint8_t slv_reg,
				       uint8_t *value, uint16_t len)
{
	uint8_t slave[3];

	slave[0] = (slv_addr << 1) | LSM6DSO_SHUB_SLVX_READ;
	slave[1] = slv_reg;
	slave[2] = (len & 0x7);

	if (lsm6dso_shub_write_embedded_regs(data, LSM6DSO_SHUB_SLV0_ADDR,
					     slave, 3) < 0) {
		LOG_DBG("shub: error writing embedded reg");
		return -EIO;
	}

	/* turn SH on */
	lsm6dso_shub_enable(data, 1);
	lsm6dso_shub_wait_completed(data);

	/* read data from external slave */
	lsm6dso_shub_embedded_en(data, true);
	if (lsm6dso_read_reg(data->ctx, LSM6DSO_SHUB_DATA_OUT,
			     value, len) < 0) {
		LOG_DBG("shub: error reading sensor data");
		return -EIO;
	}
	lsm6dso_shub_embedded_en(data, false);

	if (lsm6dso_shub_check_slv0_nack(data) < 0) {
		lsm6dso_shub_enable(data, 0);
		return -EIO;
	}

	lsm6dso_shub_enable(data, 0);
	return 0;
}

/*
 * use SLV0 to configure slave device
 */
static int lsm6dso_shub_write_slave_reg(struct lsm6dso_data *data,
					uint8_t slv_addr, uint8_t slv_reg,
					uint8_t *value, uint16_t len)
{
	uint8_t slv_cfg[3];
	uint8_t cnt = 0U;

	while (cnt < len) {
		slv_cfg[0] = (slv_addr << 1) & ~LSM6DSO_SHUB_SLVX_READ;
		slv_cfg[1] = slv_reg + cnt;

		if (lsm6dso_shub_write_embedded_regs(data,
						     LSM6DSO_SHUB_SLV0_ADDR,
						     slv_cfg, 2) < 0) {
			LOG_DBG("shub: error writing embedded reg");
			return -EIO;
		}

		slv_cfg[0] = value[cnt];
		if (lsm6dso_shub_write_embedded_regs(data,
					LSM6DSO_SHUB_SLV0_DATAWRITE,
					slv_cfg, 1) < 0) {
			LOG_DBG("shub: error writing embedded reg");
			return -EIO;
		}

		/* turn SH on */
		lsm6dso_shub_enable(data, 1);
		lsm6dso_shub_wait_completed(data);

		if (lsm6dso_shub_check_slv0_nack(data) < 0) {
			lsm6dso_shub_enable(data, 0);
			return -EIO;
		}

		lsm6dso_shub_enable(data, 0);

		cnt++;
	}

	/* Put SLV0 in IDLE mode */
	slv_cfg[0] = 0x7;
	slv_cfg[1] = 0x0;
	slv_cfg[2] = 0x0;
	if (lsm6dso_shub_write_embedded_regs(data, LSM6DSO_SHUB_SLV0_ADDR,
					     slv_cfg, 3) < 0) {
		LOG_DBG("shub: error writing embedded reg");
		return -EIO;
	}

	return 0;
}

/*
 * SLAVEs configurations:
 *
 *  - SLAVE 0: used for configuring all slave devices
 *  - SLAVE 1: used as data read channel for external slave device #1
 *  - SLAVE 2: used as data read channel for external slave device #2
 *  - SLAVE 3: used for generic reads while data channel is enabled
 */
static int lsm6dso_shub_set_data_channel(struct lsm6dso_data *data)
{
	uint8_t n, i, slv_cfg[6];
	struct lsm6dso_shub_slist *sp;

	/* Set data channel for slave devices */
	for (n = 0; n < num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[shub_ext[n]];

		i = n * 3;
		slv_cfg[i] = (sp->ext_i2c_addr << 1) | LSM6DSO_SHUB_SLVX_READ;
		slv_cfg[i + 1] = sp->out_data_addr;
		slv_cfg[i + 2] = sp->out_data_len;
	}

	if (lsm6dso_shub_write_embedded_regs(data,
					     LSM6DSO_SHUB_SLV1_ADDR,
					     slv_cfg, n*3) < 0) {
		LOG_DBG("shub: error writing embedded reg");
		return -EIO;
	}

	/* Configure the master */
	lsm6dso_aux_sens_on_t aux = LSM6DSO_SLV_0_1_2;

	if (lsm6dso_sh_slave_connected_set(data->ctx, aux) < 0) {
		LOG_DBG("shub: error setting aux sensors");
		return -EIO;
	}

	lsm6dso_write_once_t wo = LSM6DSO_ONLY_FIRST_CYCLE;

	if (lsm6dso_sh_write_mode_set(data->ctx, wo) < 0) {
		LOG_DBG("shub: error setting write once");
		return -EIO;
	}


	/* turn SH on */
	lsm6dso_shub_enable(data, 1);
	lsm6dso_shub_wait_completed(data);

	return 0;
}

int lsm6dso_shub_get_idx(enum sensor_channel type)
{
	uint8_t n;
	struct lsm6dso_shub_slist *sp;

	for (n = 0; n < num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[shub_ext[n]];

		if (sp->type == type)
			return n;
	}

	return -ENOTSUP;
}

int lsm6dso_shub_fetch_external_devs(struct device *dev)
{
	uint8_t n;
	struct lsm6dso_data *data = dev->data;
	struct lsm6dso_shub_slist *sp;

	/* read data from external slave */
	lsm6dso_shub_embedded_en(data, true);

	for (n = 0; n < num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[shub_ext[n]];

		if (lsm6dso_read_reg(data->ctx, sp->sh_out_reg,
				     data->ext_data[n], sp->out_data_len) < 0) {
			LOG_DBG("shub: failed to read sample");
			return -EIO;
		}
	}

	lsm6dso_shub_embedded_en(data, false);

	return 0;
}

int lsm6dso_shub_config(struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr,
			const struct sensor_value *val)
{
	struct lsm6dso_data *data = dev->data;
	struct lsm6dso_shub_slist *sp = NULL;
	uint8_t n;

	for (n = 0; n < num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[shub_ext[n]];

		if (sp->type == chan)
			break;
	}

	if (n == num_ext_dev) {
		LOG_DBG("shub: chan not supported");
		return -ENOTSUP;
	}

	if (sp == NULL || sp->dev_conf == NULL) {
		LOG_DBG("shub: chan not configurable");
		return -ENOTSUP;
	}

	return sp->dev_conf(data, sp->ext_i2c_addr, chan, attr, val);
}

int lsm6dso_shub_init(struct device *dev)
{
	struct lsm6dso_data *data = dev->data;
	uint8_t i, n = 0, regn;
	uint8_t chip_id;
	struct lsm6dso_shub_slist *sp;

	for (n = 0; n < ARRAY_SIZE(lsm6dso_shub_slist); n++) {
		if (num_ext_dev >= LSM6DSO_SHUB_MAX_NUM_SLVS)
			break;

		chip_id = 0;
		sp = &lsm6dso_shub_slist[n];

		/*
		 * The external sensor may have different I2C address.
		 * So, try them one by one until we read the correct
		 * chip ID.
		 */
		for (i = 0U; i < ARRAY_SIZE(sp->i2c_addr); i++) {
			if (lsm6dso_shub_read_slave_reg(data,
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

		shub_ext[num_ext_dev++] = n;
	}

	if (num_ext_dev == 0) {
		LOG_ERR("shub: no slave devices found");
		return -EINVAL;
	}

	/* init external devices */
	for (n = 0, regn = 0; n < num_ext_dev; n++) {
		sp = &lsm6dso_shub_slist[shub_ext[n]];
		sp->sh_out_reg = LSM6DSO_SHUB_DATA_OUT + regn;
		regn += sp->out_data_len;
		sp->dev_init(data, sp->ext_i2c_addr);
	}

	lsm6dso_shub_set_data_channel(data);

	return 0;
}
