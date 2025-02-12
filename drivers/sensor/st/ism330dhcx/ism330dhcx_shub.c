/* ST Microelectronics ISM330DHCX 6-axis IMU sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/ism330dhcx.pdf
 */

#define DT_DRV_COMPAT st_ism330dhcx

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "ism330dhcx.h"

LOG_MODULE_DECLARE(ISM330DHCX, CONFIG_SENSOR_LOG_LEVEL);

#define ISM330DHCX_SHUB_DATA_OUT				0x02

#define ISM330DHCX_SHUB_SLV0_ADDR				0x15
#define ISM330DHCX_SHUB_SLV0_SUBADDR			0x16
#define ISM330DHCX_SHUB_SLV0_CONFIG			0x17
#define ISM330DHCX_SHUB_SLV1_ADDR				0x18
#define ISM330DHCX_SHUB_SLV1_SUBADDR			0x19
#define ISM330DHCX_SHUB_SLV1_CONFIG			0x1A
#define ISM330DHCX_SHUB_SLV2_ADDR				0x1B
#define ISM330DHCX_SHUB_SLV2_SUBADDR			0x1C
#define ISM330DHCX_SHUB_SLV2_CONFIG			0x1D
#define ISM330DHCX_SHUB_SLV3_ADDR				0x1E
#define ISM330DHCX_SHUB_SLV3_SUBADDR			0x1F
#define ISM330DHCX_SHUB_SLV3_CONFIG			0x20
#define ISM330DHCX_SHUB_SLV0_DATAWRITE			0x21

#define ISM330DHCX_SHUB_STATUS_MASTER			0x22
#define ISM330DHCX_SHUB_STATUS_SLV0_NACK			BIT(3)
#define ISM330DHCX_SHUB_STATUS_ENDOP			BIT(0)

#define ISM330DHCX_SHUB_SLVX_WRITE				0x0
#define ISM330DHCX_SHUB_SLVX_READ				0x1

static uint8_t num_ext_dev;
static uint8_t shub_ext[ISM330DHCX_SHUB_MAX_NUM_SLVS];

static int ism330dhcx_shub_write_slave_reg(const struct device *dev, uint8_t slv_addr,
					   uint8_t slv_reg, uint8_t *value, uint16_t len);
static int ism330dhcx_shub_read_slave_reg(const struct device *dev, uint8_t slv_addr,
					  uint8_t slv_reg, uint8_t *value, uint16_t len);
static void ism330dhcx_shub_enable(const struct device *dev, uint8_t enable);

/*
 * LIS2MDL magn device specific part
 */
#if defined(CONFIG_ISM330DHCX_EXT_LIS2MDL) || defined(CONFIG_ISM330DHCX_EXT_IIS2MDC)

#define LIS2MDL_CFG_REG_A		0x60
#define LIS2MDL_CFG_REG_B		0x61
#define LIS2MDL_CFG_REG_C		0x62
#define LIS2MDL_STATUS_REG		0x67

#define LIS2MDL_SW_RESET		0x20
#define LIS2MDL_ODR_10HZ		0x00
#define LIS2MDL_ODR_100HZ		0x0C
#define LIS2MDL_OFF_CANC		0x02
#define LIS2MDL_SENSITIVITY		1500

static int ism330dhcx_lis2mdl_init(const struct device *dev, uint8_t i2c_addr)
{
	struct ism330dhcx_data *data = dev->data;
	uint8_t mag_cfg[2];

	data->magn_gain = LIS2MDL_SENSITIVITY;

	/* sw reset device */
	mag_cfg[0] = LIS2MDL_SW_RESET;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LIS2MDL_CFG_REG_A, mag_cfg, 1);

	k_sleep(K_MSEC(10)); /* turn-on time in ms */

	/* configure mag */
	mag_cfg[0] = LIS2MDL_ODR_10HZ;
	mag_cfg[1] = LIS2MDL_OFF_CANC;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LIS2MDL_CFG_REG_A, mag_cfg, 2);

	return 0;
}

static const uint16_t lis2mdl_map[] = {10, 20, 50, 100};

static int ism330dhcx_lis2mdl_odr_set(const struct device *dev, uint8_t i2c_addr, uint16_t freq)
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
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LIS2MDL_CFG_REG_A, &cfg, 1);

	ism330dhcx_shub_enable(dev, 1);
	return 0;
}

static int ism330dhcx_lis2mdl_conf(const struct device *dev, uint8_t i2c_addr,
				   enum sensor_channel chan, enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return ism330dhcx_lis2mdl_odr_set(dev, i2c_addr, val->val1);
	default:
		LOG_DBG("shub: LIS2MDL attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_ISM330DHCX_EXT_LIS2MDL || CONFIG_ISM330DHCX_EXT_IIS2MDC */

/*
 * HTS221 humidity device specific part
 */
#ifdef CONFIG_ISM330DHCX_EXT_HTS221

#define HTS221_AUTOINCREMENT		BIT(7)

#define HTS221_REG_CTRL1		0x20
#define HTS221_ODR_1HZ			0x01
#define HTS221_BDU			0x04
#define HTS221_PD			0x80

#define HTS221_REG_CONV_START		0x30

static int ism330dhcx_hts221_read_conv_data(const struct device *dev, uint8_t i2c_addr)
{
	struct ism330dhcx_data *data = dev->data;
	uint8_t buf[16], i;
	struct hts221_data *ht = &data->hts221;

	for (i = 0; i < sizeof(buf); i += 7) {
		unsigned char len = MIN(7, sizeof(buf) - i);

		if (ism330dhcx_shub_read_slave_reg(dev, i2c_addr, (HTS221_REG_CONV_START + i)
							| HTS221_AUTOINCREMENT, &buf[i],
						   len) < 0) {
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

static int ism330dhcx_hts221_init(const struct device *dev, uint8_t i2c_addr)
{
	uint8_t hum_cfg;

	/* configure ODR and BDU */
	hum_cfg = HTS221_ODR_1HZ | HTS221_BDU | HTS221_PD;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, HTS221_REG_CTRL1, &hum_cfg, 1);

	return ism330dhcx_hts221_read_conv_data(dev, i2c_addr);
}

static const uint16_t hts221_map[] = {0, 1, 7, 12};

static int ism330dhcx_hts221_odr_set(const struct device *dev, uint8_t i2c_addr, uint16_t freq)
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
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, HTS221_REG_CTRL1, &cfg, 1);

	ism330dhcx_shub_enable(dev, 1);
	return 0;
}

static int ism330dhcx_hts221_conf(const struct device *dev, uint8_t i2c_addr,
				  enum sensor_channel chan, enum sensor_attribute attr,
				  const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return ism330dhcx_hts221_odr_set(dev, i2c_addr, val->val1);
	default:
		LOG_DBG("shub: HTS221 attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_ISM330DHCX_EXT_HTS221 */

/*
 * LPS22HB baro/temp device specific part
 */
#ifdef CONFIG_ISM330DHCX_EXT_LPS22HB

#define LPS22HB_CTRL_REG1		0x10
#define LPS22HB_CTRL_REG2		0x11

#define LPS22HB_SW_RESET		0x04
#define LPS22HB_ODR_10HZ		0x20
#define LPS22HB_LPF_EN			0x08
#define LPS22HB_BDU_EN			0x02

static int ism330dhcx_lps22hb_init(const struct device *dev, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HB_SW_RESET;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LPS22HB_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(1)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HB_ODR_10HZ | LPS22HB_LPF_EN | LPS22HB_BDU_EN;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LPS22HB_CTRL_REG1, baro_cfg, 1);

	return 0;
}
#endif /* CONFIG_ISM330DHCX_EXT_LPS22HB */

/*
 * LPS22HH baro/temp device specific part
 */
#ifdef CONFIG_ISM330DHCX_EXT_LPS22HH

#define LPS22HH_CTRL_REG1		0x10
#define LPS22HH_CTRL_REG2		0x11

#define LPS22HH_SW_RESET		0x04
#define LPS22HH_IF_ADD_INC		0x10
#define LPS22HH_ODR_10HZ		0x20
#define LPS22HH_LPF_EN			0x08
#define LPS22HH_BDU_EN			0x02

static int ism330dhcx_lps22hh_init(const struct device *dev, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HH_SW_RESET;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LPS22HH_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(100)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HH_IF_ADD_INC;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LPS22HH_CTRL_REG2, baro_cfg, 1);

	baro_cfg[0] = LPS22HH_ODR_10HZ | LPS22HH_LPF_EN | LPS22HH_BDU_EN;
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LPS22HH_CTRL_REG1, baro_cfg, 1);

	return 0;
}

static const uint16_t lps22hh_map[] = {0, 1, 10, 25, 50, 75, 100, 200};

static int ism330dhcx_lps22hh_odr_set(const struct device *dev, uint8_t i2c_addr, uint16_t freq)
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
	ism330dhcx_shub_write_slave_reg(dev, i2c_addr, LPS22HH_CTRL_REG1, &cfg, 1);

	ism330dhcx_shub_enable(dev, 1);
	return 0;
}

static int ism330dhcx_lps22hh_conf(const struct device *dev, uint8_t i2c_addr,
				   enum sensor_channel chan, enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return ism330dhcx_lps22hh_odr_set(dev, i2c_addr, val->val1);
	default:
		LOG_DBG("shub: LPS22HH attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_ISM330DHCX_EXT_LPS22HH */

/* List of supported external sensors */
static struct ism330dhcx_shub_slist {
	enum sensor_channel type;
	uint8_t i2c_addr[2];
	uint8_t ext_i2c_addr;
	uint8_t wai_addr;
	uint8_t wai_val;
	uint8_t out_data_addr;
	uint8_t out_data_len;
	uint8_t sh_out_reg;
	int (*dev_init)(const struct device *dev, uint8_t i2c_addr);
	int (*dev_conf)(const struct device *dev, uint8_t i2c_addr, enum sensor_channel chan,
			enum sensor_attribute attr, const struct sensor_value *val);
} ism330dhcx_shub_slist[] = {
#if defined(CONFIG_ISM330DHCX_EXT_LIS2MDL) || defined(CONFIG_ISM330DHCX_EXT_IIS2MDC)
	{
		/* LIS2MDL */
		.type		= SENSOR_CHAN_MAGN_XYZ,
		.i2c_addr       = { 0x1E },
		.wai_addr       = 0x4F,
		.wai_val        = 0x40,
		.out_data_addr  = 0x68,
		.out_data_len   = 0x06,
		.dev_init       = (ism330dhcx_lis2mdl_init),
		.dev_conf       = (ism330dhcx_lis2mdl_conf),
	},
#endif /* CONFIG_ISM330DHCX_EXT_LIS2MDL || CONFIG_ISM330DHCX_EXT_IIS2MDC */

#ifdef CONFIG_ISM330DHCX_EXT_HTS221
	{
		/* HTS221 */
		.type		= SENSOR_CHAN_HUMIDITY,
		.i2c_addr       = { 0x5F },
		.wai_addr       = 0x0F,
		.wai_val        = 0xBC,
		.out_data_addr  = 0x28 | HTS221_AUTOINCREMENT,
		.out_data_len   = 0x02,
		.dev_init       = (ism330dhcx_hts221_init),
		.dev_conf       = (ism330dhcx_hts221_conf),
	},
#endif /* CONFIG_ISM330DHCX_EXT_HTS221 */

#ifdef CONFIG_ISM330DHCX_EXT_LPS22HB
	{
		/* LPS22HB */
		.type		= SENSOR_CHAN_PRESS,
		.i2c_addr       = { 0x5C, 0x5D },
		.wai_addr       = 0x0F,
		.wai_val        = 0xB1,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x05,
		.dev_init       = (ism330dhcx_lps22hb_init),
	},
#endif /* CONFIG_ISM330DHCX_EXT_LPS22HB */

#ifdef CONFIG_ISM330DHCX_EXT_LPS22HH
	{
		/* LPS22HH */
		.type		= SENSOR_CHAN_PRESS,
		.i2c_addr       = { 0x5C, 0x5D },
		.wai_addr       = 0x0F,
		.wai_val        = 0xB3,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x05,
		.dev_init       = (ism330dhcx_lps22hh_init),
		.dev_conf       = (ism330dhcx_lps22hh_conf),
	},
#endif /* CONFIG_ISM330DHCX_EXT_LPS22HH */
};

static inline void ism330dhcx_shub_wait_completed(const struct device *dev)
{
	struct ism330dhcx_data *data = dev->data;
	ism330dhcx_status_master_t status;

	do {
		k_msleep(1);
		ism330dhcx_sh_status_get(data->ctx, &status);
	} while (status.sens_hub_endop == 0);
}

static inline void ism330dhcx_shub_embedded_en(const struct device *dev, bool on)
{
	struct ism330dhcx_data *data = dev->data;

	if (on) {
		(void) ism330dhcx_mem_bank_set(data->ctx, ISM330DHCX_SENSOR_HUB_BANK);
	} else {
		(void) ism330dhcx_mem_bank_set(data->ctx, ISM330DHCX_USER_BANK);
	}

	k_busy_wait(150);
}

static int ism330dhcx_shub_read_embedded_regs(const struct device *dev, uint8_t reg_addr,
					      uint8_t *value, int len)
{
	struct ism330dhcx_data *data = dev->data;

	ism330dhcx_shub_embedded_en(dev, true);

	if (ism330dhcx_read_reg(data->ctx, reg_addr, value, len) < 0) {
		LOG_DBG("shub: failed to read external reg: %02x", reg_addr);
		ism330dhcx_shub_embedded_en(dev, false);
		return -EIO;
	}

	ism330dhcx_shub_embedded_en(dev, false);

	return 0;
}

static int ism330dhcx_shub_write_embedded_regs(const struct device *dev, uint8_t reg_addr,
					       uint8_t *value, uint8_t len)
{
	struct ism330dhcx_data *data = dev->data;

	ism330dhcx_shub_embedded_en(dev, true);

	if (ism330dhcx_write_reg(data->ctx, reg_addr, value, len) < 0) {
		LOG_DBG("shub: failed to write external reg: %02x", reg_addr);
		ism330dhcx_shub_embedded_en(dev, false);
		return -EIO;
	}

	ism330dhcx_shub_embedded_en(dev, false);

	return 0;
}

static void ism330dhcx_shub_enable(const struct device *dev, uint8_t enable)
{
	struct ism330dhcx_data *data = dev->data;

	/* Enable Accel @26hz */
	if (!data->accel_freq) {
		uint8_t odr = (enable) ? 2 : 0;

		if (ism330dhcx_xl_data_rate_set(data->ctx, odr) < 0) {
			LOG_DBG("shub: failed to set XL sampling rate");
			return;
		}
	}

	ism330dhcx_shub_embedded_en(dev, true);

	if (ism330dhcx_sh_master_set(data->ctx, enable) < 0) {
		LOG_DBG("shub: failed to set master on");
		ism330dhcx_shub_embedded_en(dev, false);
		return;
	}

	ism330dhcx_shub_embedded_en(dev, false);
}

/* must be called with master on */
static int ism330dhcx_shub_check_slv0_nack(const struct device *dev)
{
	uint8_t status;

	if (ism330dhcx_shub_read_embedded_regs(dev, ISM330DHCX_SHUB_STATUS_MASTER, &status, 1) <
	    0) {
		LOG_DBG("shub: error reading embedded reg");
		return -EIO;
	}

	if (status & (ISM330DHCX_SHUB_STATUS_SLV0_NACK)) {
		LOG_DBG("shub: SLV0 nacked");
		return -EIO;
	}

	return 0;
}

/*
 * use SLV0 for generic read to slave device
 */
static int ism330dhcx_shub_read_slave_reg(const struct device *dev, uint8_t slv_addr,
					  uint8_t slv_reg, uint8_t *value, uint16_t len)
{
	struct ism330dhcx_data *data = dev->data;
	uint8_t slave[3];

	slave[0] = (slv_addr << 1) | ISM330DHCX_SHUB_SLVX_READ;
	slave[1] = slv_reg;
	slave[2] = (len & 0x7);

	if (ism330dhcx_shub_write_embedded_regs(dev, ISM330DHCX_SHUB_SLV0_ADDR, slave, 3) < 0) {
		LOG_DBG("shub: error writing embedded reg");
		return -EIO;
	}

	/* turn SH on */
	ism330dhcx_shub_enable(dev, 1);
	ism330dhcx_shub_wait_completed(dev);

	if (ism330dhcx_shub_check_slv0_nack(dev) < 0) {
		ism330dhcx_shub_enable(dev, 0);
		return -EIO;
	}

	/* read data from external slave */
	ism330dhcx_shub_embedded_en(dev, true);
	if (ism330dhcx_read_reg(data->ctx, ISM330DHCX_SHUB_DATA_OUT,
				value, len) < 0) {
		LOG_DBG("shub: error reading sensor data");
		ism330dhcx_shub_embedded_en(dev, false);
		return -EIO;
	}
	ism330dhcx_shub_embedded_en(dev, false);

	ism330dhcx_shub_enable(dev, 0);
	return 0;
}

/*
 * use SLV0 to configure slave device
 */
static int ism330dhcx_shub_write_slave_reg(const struct device *dev, uint8_t slv_addr,
					   uint8_t slv_reg, uint8_t *value, uint16_t len)
{
	uint8_t slv_cfg[3];
	uint8_t cnt = 0U;

	while (cnt < len) {
		slv_cfg[0] = (slv_addr << 1) & ~ISM330DHCX_SHUB_SLVX_READ;
		slv_cfg[1] = slv_reg + cnt;

		if (ism330dhcx_shub_write_embedded_regs(dev, ISM330DHCX_SHUB_SLV0_ADDR, slv_cfg,
							2) < 0) {
			LOG_DBG("shub: error writing embedded reg");
			return -EIO;
		}

		slv_cfg[0] = value[cnt];
		if (ism330dhcx_shub_write_embedded_regs(dev, ISM330DHCX_SHUB_SLV0_DATAWRITE,
							slv_cfg, 1) < 0) {
			LOG_DBG("shub: error writing embedded reg");
			return -EIO;
		}

		/* turn SH on */
		ism330dhcx_shub_enable(dev, 1);
		ism330dhcx_shub_wait_completed(dev);

		if (ism330dhcx_shub_check_slv0_nack(dev) < 0) {
			ism330dhcx_shub_enable(dev, 0);
			return -EIO;
		}

		ism330dhcx_shub_enable(dev, 0);

		cnt++;
	}

	/* Put SLV0 in IDLE mode */
	slv_cfg[0] = 0x7;
	slv_cfg[1] = 0x0;
	slv_cfg[2] = 0x0;
	if (ism330dhcx_shub_write_embedded_regs(dev, ISM330DHCX_SHUB_SLV0_ADDR, slv_cfg, 3) < 0) {
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
static int ism330dhcx_shub_set_data_channel(const struct device *dev)
{
	struct ism330dhcx_data *data = dev->data;
	uint8_t n, i, slv_cfg[6];
	struct ism330dhcx_shub_slist *sp;

	/* Set data channel for slave devices */
	for (n = 0; n < num_ext_dev; n++) {
		sp = &ism330dhcx_shub_slist[shub_ext[n]];

		i = n * 3;
		slv_cfg[i] = (sp->ext_i2c_addr << 1) | ISM330DHCX_SHUB_SLVX_READ;
		slv_cfg[i + 1] = sp->out_data_addr;
		slv_cfg[i + 2] = sp->out_data_len;
	}

	if (ism330dhcx_shub_write_embedded_regs(dev, ISM330DHCX_SHUB_SLV1_ADDR, slv_cfg,
						n * 3) < 0) {
		LOG_DBG("shub: error writing embedded reg");
		return -EIO;
	}

	/* Configure the master */
	ism330dhcx_aux_sens_on_t aux = ISM330DHCX_SLV_0_1_2;

	if (ism330dhcx_sh_slave_connected_set(data->ctx, aux) < 0) {
		LOG_DBG("shub: error setting aux sensors");
		return -EIO;
	}

	ism330dhcx_write_once_t wo = ISM330DHCX_ONLY_FIRST_CYCLE;

	if (ism330dhcx_sh_write_mode_set(data->ctx, wo) < 0) {
		LOG_DBG("shub: error setting write once");
		return -EIO;
	}


	/* turn SH on */
	ism330dhcx_shub_enable(dev, 1);
	ism330dhcx_shub_wait_completed(dev);

	return 0;
}

int ism330dhcx_shub_get_idx(enum sensor_channel type)
{
	uint8_t n;
	struct ism330dhcx_shub_slist *sp;

	for (n = 0; n < num_ext_dev; n++) {
		sp = &ism330dhcx_shub_slist[shub_ext[n]];

		if (sp->type == type) {
			return n;
		}
	}

	return -ENOTSUP;
}

int ism330dhcx_shub_fetch_external_devs(const struct device *dev)
{
	uint8_t n;
	struct ism330dhcx_data *data = dev->data;
	struct ism330dhcx_shub_slist *sp;

	/* read data from external slave */
	ism330dhcx_shub_embedded_en(dev, true);

	for (n = 0; n < num_ext_dev; n++) {
		sp = &ism330dhcx_shub_slist[shub_ext[n]];

		if (ism330dhcx_read_reg(data->ctx, sp->sh_out_reg,
				     data->ext_data[n], sp->out_data_len) < 0) {
			LOG_DBG("shub: failed to read sample");
			ism330dhcx_shub_embedded_en(dev, false);
			return -EIO;
		}
	}

	ism330dhcx_shub_embedded_en(dev, false);

	return 0;
}

int ism330dhcx_shub_config(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	struct ism330dhcx_shub_slist *sp = NULL;
	uint8_t n;

	for (n = 0; n < num_ext_dev; n++) {
		sp = &ism330dhcx_shub_slist[shub_ext[n]];

		if (sp->type == chan) {
			break;
		}
	}

	if (n == num_ext_dev) {
		LOG_DBG("shub: chan not supported");
		return -ENOTSUP;
	}

	if (sp == NULL || sp->dev_conf == NULL) {
		LOG_DBG("shub: chan not configurable");
		return -ENOTSUP;
	}

	return sp->dev_conf(dev, sp->ext_i2c_addr, chan, attr, val);
}

int ism330dhcx_shub_init(const struct device *dev)
{
	uint8_t i, n = 0, regn;
	uint8_t chip_id;
	struct ism330dhcx_shub_slist *sp;

	for (n = 0; n < ARRAY_SIZE(ism330dhcx_shub_slist); n++) {
		if (num_ext_dev >= ISM330DHCX_SHUB_MAX_NUM_SLVS) {
			break;
		}

		chip_id = 0;
		sp = &ism330dhcx_shub_slist[n];

		/*
		 * The external sensor may have different I2C address.
		 * So, try them one by one until we read the correct
		 * chip ID.
		 */
		for (i = 0U; i < ARRAY_SIZE(sp->i2c_addr); i++) {
			if (ism330dhcx_shub_read_slave_reg(dev, sp->i2c_addr[i], sp->wai_addr,
							   &chip_id, 1) < 0) {
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
		LOG_INF("shub: Ext Device Chip Id: 0x%02x", chip_id);
		sp->ext_i2c_addr = sp->i2c_addr[i];

		shub_ext[num_ext_dev++] = n;
	}

	if (num_ext_dev == 0) {
		LOG_ERR("shub: no slave devices found");
		return -EINVAL;
	}

	/* init external devices */
	for (n = 0, regn = 0; n < num_ext_dev; n++) {
		sp = &ism330dhcx_shub_slist[shub_ext[n]];
		sp->sh_out_reg = ISM330DHCX_SHUB_DATA_OUT + regn;
		regn += sp->out_data_len;
		sp->dev_init(dev, sp->ext_i2c_addr);
	}

	ism330dhcx_shub_set_data_channel(dev);

	return 0;
}
