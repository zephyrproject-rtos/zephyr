/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "lsm6dsl.h"

LOG_MODULE_DECLARE(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

#define LSM6DSL_EMBEDDED_SLV0_ADDR       0x02
#define LSM6DSL_EMBEDDED_SLV0_SUBADDR    0x03
#define LSM6DSL_EMBEDDED_SLV0_CONFIG     0x04
#define LSM6DSL_EMBEDDED_SLV1_ADDR       0x05
#define LSM6DSL_EMBEDDED_SLV1_SUBADDR    0x06
#define LSM6DSL_EMBEDDED_SLV1_CONFIG     0x07
#define LSM6DSL_EMBEDDED_SLV2_ADDR       0x08
#define LSM6DSL_EMBEDDED_SLV2_SUBADDR    0x09
#define LSM6DSL_EMBEDDED_SLV2_CONFIG     0x0A
#define LSM6DSL_EMBEDDED_SLV3_ADDR       0x0B
#define LSM6DSL_EMBEDDED_SLV3_SUBADDR    0x0C
#define LSM6DSL_EMBEDDED_SLV3_CONFIG     0x0D
#define LSM6DSL_EMBEDDED_SLV0_DATAWRITE  0x0E

#define LSM6DSL_EMBEDDED_SLVX_READ       0x1
#define LSM6DSL_EMBEDDED_SLVX_THREE_SENS 0x20
#define LSM6DSL_EMBEDDED_SLV0_WRITE_IDLE 0x07

static int lsm6dsl_shub_write_slave_reg(struct lsm6dsl_data *data,
					uint8_t slv_addr, uint8_t slv_reg,
					uint8_t *value, uint16_t len);

/*
 * LIS2MDL magn device specific part
 */
#ifdef CONFIG_LSM6DSL_EXT0_LIS2MDL

#define LIS2MDL_CFG_REG_A         0x60
#define LIS2MDL_CFG_REG_B         0x61
#define LIS2MDL_CFG_REG_C         0x62
#define LIS2MDL_STATUS_REG        0x67

#define LIS2MDL_SW_RESET          0x20
#define LIS2MDL_ODR_10HZ          0x00
#define LIS2MDL_OFF_CANC          0x02
#define LIS2MDL_SENSITIVITY       1500

static int lsm6dsl_lis2mdl_init(struct lsm6dsl_data *data, uint8_t i2c_addr)
{
	uint8_t mag_cfg[2];

	data->magn_sensitivity = LIS2MDL_SENSITIVITY;

	/* sw reset device */
	mag_cfg[0] = LIS2MDL_SW_RESET;
	lsm6dsl_shub_write_slave_reg(data, i2c_addr,
				     LIS2MDL_CFG_REG_A, mag_cfg, 1);

	k_sleep(K_MSEC(10)); /* turn-on time in ms */

	/* configure mag */
	mag_cfg[0] = LIS2MDL_ODR_10HZ;
	mag_cfg[1] = LIS2MDL_OFF_CANC;
	lsm6dsl_shub_write_slave_reg(data, i2c_addr,
				     LIS2MDL_CFG_REG_A, mag_cfg, 2);

	return 0;
}
#endif /* CONFIG_LSM6DSL_EXT0_LIS2MDL */

/*
 * LPS22HB baro/temp device specific part
 */
#ifdef CONFIG_LSM6DSL_EXT0_LPS22HB

#define LPS22HB_CTRL_REG1         0x10
#define LPS22HB_CTRL_REG2         0x11

#define LPS22HB_SW_RESET          0x04
#define LPS22HB_ODR_10HZ          0x20
#define LPS22HB_LPF_EN            0x08
#define LPS22HB_BDU_EN            0x02

static int lsm6dsl_lps22hb_init(struct lsm6dsl_data *data, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HB_SW_RESET;
	lsm6dsl_shub_write_slave_reg(data, i2c_addr,
				     LPS22HB_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(1)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HB_ODR_10HZ | LPS22HB_LPF_EN | LPS22HB_BDU_EN;
	lsm6dsl_shub_write_slave_reg(data, i2c_addr,
				     LPS22HB_CTRL_REG1, baro_cfg, 1);

	return 0;
}
#endif /* CONFIG_LSM6DSL_EXT0_LPS22HB */

/* List of supported external sensors */
static struct lsm6dsl_shub_sens_list {
	uint8_t i2c_addr[2];
	uint8_t wai_addr;
	uint8_t wai_val;
	uint8_t out_data_addr;
	uint8_t out_data_len;
	int (*dev_init)(struct lsm6dsl_data *data, uint8_t i2c_addr);
} lsm6dsl_shub_sens_list[] = {
#ifdef CONFIG_LSM6DSL_EXT0_LIS2MDL
	{
		/* LIS2MDL */
		.i2c_addr       = { 0x1E },
		.wai_addr       = 0x4F,
		.wai_val        = 0x40,
		.out_data_addr  = 0x68,
		.out_data_len   = 0x06,
		.dev_init       = (lsm6dsl_lis2mdl_init),
	},
#endif  /* CONFIG_LSM6DSL_EXT0_LIS2MDL */

#ifdef CONFIG_LSM6DSL_EXT0_LPS22HB
	{
		/* LPS22HB */
		.i2c_addr       = { 0x5C, 0x5D },
		.wai_addr       = 0x0F,
		.wai_val        = 0xB1,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x05,
		.dev_init       = (lsm6dsl_lps22hb_init),
	},
#endif  /* CONFIG_LSM6DSL_EXT0_LPS22HB */
};

static uint8_t ext_i2c_addr;

static inline void lsm6dsl_shub_wait_completed(struct lsm6dsl_data *data)
{
	uint16_t freq;

	freq = (data->accel_freq == 0U) ? 26 : data->accel_freq;
	k_msleep((2000U / freq) + 1);
}

static inline void lsm6dsl_shub_embedded_en(struct lsm6dsl_data *data, bool on)
{
	uint8_t func_en = (on) ? 0x1 : 0x0;

	data->hw_tf->update_reg(data, LSM6DSL_REG_FUNC_CFG_ACCESS,
				LSM6DSL_MASK_FUNC_CFG_EN,
				func_en << LSM6DSL_SHIFT_FUNC_CFG_EN);

	k_sleep(K_MSEC(1));
}

#ifdef LSM6DSL_DEBUG
static int lsm6dsl_read_embedded_reg(struct lsm6dsl_data *data,
				     uint8_t reg_addr, uint8_t *value, int len)
{
	lsm6dsl_shub_embedded_en(data, true);

	if (data->hw_tf->read_data(data, reg_addr, value, len) < 0) {
		LOG_DBG("failed to read external reg: %02x", reg_addr);
		lsm6dsl_shub_embedded_en(data, false);
		return -EIO;
	}

	lsm6dsl_shub_embedded_en(data, false);

	return 0;
}
#endif

static int lsm6dsl_shub_write_embedded_regs(struct lsm6dsl_data *data,
					    uint8_t reg_addr,
					    uint8_t *value, uint8_t len)
{
	lsm6dsl_shub_embedded_en(data, true);

	if (data->hw_tf->write_data(data, reg_addr, value, len) < 0) {
		LOG_DBG("failed to write external reg: %02x", reg_addr);
		lsm6dsl_shub_embedded_en(data, false);
		return -EIO;
	}

	lsm6dsl_shub_embedded_en(data, false);

	return 0;
}

static void lsm6dsl_shub_enable(struct lsm6dsl_data *data)
{
	/* Enable Digital Func */
	data->hw_tf->update_reg(data, LSM6DSL_REG_CTRL10_C,
				LSM6DSL_MASK_CTRL10_C_FUNC_EN,
				1 << LSM6DSL_SHIFT_CTRL10_C_FUNC_EN);

	/* Enable Accel @26hz */
	if (!data->accel_freq) {
		data->hw_tf->update_reg(data,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_ODR_XL,
				    2 << LSM6DSL_SHIFT_CTRL1_XL_ODR_XL);
	}

	/* Enable Sensor Hub */
	data->hw_tf->update_reg(data, LSM6DSL_REG_MASTER_CONFIG,
				LSM6DSL_MASK_MASTER_CONFIG_MASTER_ON,
				1 << LSM6DSL_SHIFT_MASTER_CONFIG_MASTER_ON);
}

static void lsm6dsl_shub_disable(struct lsm6dsl_data *data)
{
	/* Disable Sensor Hub */
	data->hw_tf->update_reg(data, LSM6DSL_REG_MASTER_CONFIG,
				LSM6DSL_MASK_MASTER_CONFIG_MASTER_ON,
				0 << LSM6DSL_SHIFT_MASTER_CONFIG_MASTER_ON);

	/* Disable Accel */
	if (!data->accel_freq) {
		data->hw_tf->update_reg(data,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_ODR_XL,
				    0 << LSM6DSL_SHIFT_CTRL1_XL_ODR_XL);
	}

	/* Disable Digital Func */
	data->hw_tf->update_reg(data, LSM6DSL_REG_CTRL10_C,
				LSM6DSL_MASK_CTRL10_C_FUNC_EN,
				0 << LSM6DSL_SHIFT_CTRL10_C_FUNC_EN);
}

/*
 * use SLV0 for generic read to slave device
 */
static int lsm6dsl_shub_read_slave_reg(struct lsm6dsl_data *data,
				       uint8_t slv_addr, uint8_t slv_reg,
				       uint8_t *value, uint16_t len)
{
	uint8_t slave[3];

	slave[0] = (slv_addr << 1) | LSM6DSL_EMBEDDED_SLVX_READ;
	slave[1] = slv_reg;
	slave[2] = (len & 0x7);

	if (lsm6dsl_shub_write_embedded_regs(data, LSM6DSL_EMBEDDED_SLV0_ADDR,
					     slave, 3) < 0) {
		LOG_DBG("error writing embedded reg");
		return -EIO;
	}

	/* turn SH on */
	lsm6dsl_shub_enable(data);
	lsm6dsl_shub_wait_completed(data);
	data->hw_tf->read_data(data, LSM6DSL_REG_SENSORHUB1, value, len);

	lsm6dsl_shub_disable(data);
	return 0;
}

/*
 * use SLV0 to configure slave device
 */
static int lsm6dsl_shub_write_slave_reg(struct lsm6dsl_data *data,
					uint8_t slv_addr, uint8_t slv_reg,
					uint8_t *value, uint16_t len)
{
	uint8_t slv_cfg[3];
	uint8_t cnt = 0U;

	while (cnt < len) {
		slv_cfg[0] = (slv_addr << 1) & ~LSM6DSL_EMBEDDED_SLVX_READ;
		slv_cfg[1] = slv_reg + cnt;

		if (lsm6dsl_shub_write_embedded_regs(data,
						     LSM6DSL_EMBEDDED_SLV0_ADDR,
						     slv_cfg, 2) < 0) {
			LOG_DBG("error writing embedded reg");
			return -EIO;
		}

		slv_cfg[0] = value[cnt];
		if (lsm6dsl_shub_write_embedded_regs(data,
					LSM6DSL_EMBEDDED_SLV0_DATAWRITE,
					slv_cfg, 1) < 0) {
			LOG_DBG("error writing embedded reg");
			return -EIO;
		}

		/* turn SH on */
		lsm6dsl_shub_enable(data);
		lsm6dsl_shub_wait_completed(data);
		lsm6dsl_shub_disable(data);

		cnt++;
	}

	/* Put master in IDLE mode */
	slv_cfg[0] = LSM6DSL_EMBEDDED_SLV0_WRITE_IDLE;
	slv_cfg[1] = lsm6dsl_shub_sens_list[0].wai_addr;
	slv_cfg[2] = LSM6DSL_EMBEDDED_SLVX_THREE_SENS;
	if (lsm6dsl_shub_write_embedded_regs(data,
					     LSM6DSL_EMBEDDED_SLV0_ADDR,
					     slv_cfg, 3) < 0) {
		LOG_DBG("error writing embedded reg");
		return -EIO;
	}

	return 0;
}

/*
 * SLAVEs configurations:
 *
 *  - SLAVE 0: used for configuring the slave device
 *  - SLAVE 1: used as data read channel to slave device
 *  - SLAVE 2: used for generic reads while data channel is enabled
 */
static int lsm6dsl_shub_set_data_channel(struct lsm6dsl_data *data)
{
	uint8_t slv_cfg[3];
	uint8_t slv_i2c_addr = lsm6dsl_shub_sens_list[0].i2c_addr[ext_i2c_addr];

	/* SLV0 is used for generic write */
	slv_cfg[0] = LSM6DSL_EMBEDDED_SLV0_WRITE_IDLE;
	slv_cfg[1] = lsm6dsl_shub_sens_list[0].wai_addr;
	slv_cfg[2] = LSM6DSL_EMBEDDED_SLVX_THREE_SENS;
	if (lsm6dsl_shub_write_embedded_regs(data,
					     LSM6DSL_EMBEDDED_SLV0_ADDR,
					     slv_cfg, 3) < 0) {
		LOG_DBG("error writing embedded reg");
		return -EIO;
	}

	/* Set data channel for slave device */
	slv_cfg[0] = (slv_i2c_addr << 1) | LSM6DSL_EMBEDDED_SLVX_READ;
	slv_cfg[1] = lsm6dsl_shub_sens_list[0].out_data_addr;
	slv_cfg[2] = lsm6dsl_shub_sens_list[0].out_data_len;
	if (lsm6dsl_shub_write_embedded_regs(data,
					     LSM6DSL_EMBEDDED_SLV1_ADDR,
					     slv_cfg, 3) < 0) {
		LOG_DBG("error writing embedded reg");
		return -EIO;
	}

	/* turn SH on */
	lsm6dsl_shub_enable(data);
	lsm6dsl_shub_wait_completed(data);

	return 0;
}

int lsm6dsl_shub_read_external_chip(const struct device *dev, uint8_t *buf,
				    uint8_t len)
{
	struct lsm6dsl_data *data = dev->data;

	data->hw_tf->read_data(data, LSM6DSL_REG_SENSORHUB1, buf, len);

	return 0;
}

int lsm6dsl_shub_init_external_chip(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t i;
	uint8_t chip_id = 0U;
	uint8_t slv_i2c_addr;
	uint8_t slv_wai_addr = lsm6dsl_shub_sens_list[0].wai_addr;

	/*
	 * The external sensor may have different I2C address.
	 * So, try them one by one until we read the correct
	 * chip ID.
	 */
	for (i = 0U; i < ARRAY_SIZE(lsm6dsl_shub_sens_list[0].i2c_addr); i++) {
		slv_i2c_addr = lsm6dsl_shub_sens_list[0].i2c_addr[i];

		if (slv_i2c_addr == 0U) {
			continue;
		}

		if (lsm6dsl_shub_read_slave_reg(data, slv_i2c_addr,
						slv_wai_addr,
						&chip_id, 1) < 0) {
			LOG_DBG("failed reading external chip id");
			return -EIO;
		}
		if (chip_id == lsm6dsl_shub_sens_list[0].wai_val) {
			break;
		}
	}

	if (i >= ARRAY_SIZE(lsm6dsl_shub_sens_list[0].i2c_addr)) {
		LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}
	LOG_DBG("Ext Device Chip Id: %02x", chip_id);
	ext_i2c_addr = i;

	/* init external device */
	lsm6dsl_shub_sens_list[0].dev_init(data, slv_i2c_addr);

	lsm6dsl_shub_set_data_channel(data);

	return 0;
}
