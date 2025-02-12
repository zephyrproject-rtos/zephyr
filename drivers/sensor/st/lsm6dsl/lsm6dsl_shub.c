/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

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

static int lsm6dsl_shub_write_slave_reg(const struct device *dev,
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

static int lsm6dsl_lis2mdl_init(const struct device *dev, uint8_t i2c_addr)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t mag_cfg[2];

	data->magn_sensitivity = LIS2MDL_SENSITIVITY;

	/* sw reset device */
	mag_cfg[0] = LIS2MDL_SW_RESET;
	lsm6dsl_shub_write_slave_reg(dev, i2c_addr,
				     LIS2MDL_CFG_REG_A, mag_cfg, 1);

	k_sleep(K_MSEC(10)); /* turn-on time in ms */

	/* configure mag */
	mag_cfg[0] = LIS2MDL_ODR_10HZ;
	mag_cfg[1] = LIS2MDL_OFF_CANC;
	lsm6dsl_shub_write_slave_reg(dev, i2c_addr,
				     LIS2MDL_CFG_REG_A, mag_cfg, 2);

	return 0;
}
#endif /* CONFIG_LSM6DSL_EXT0_LIS2MDL */

/*
 * LIS3MDL magn device specific part
 */
#ifdef CONFIG_LSM6DSL_EXT0_LIS3MDL

#define LIS3MDL_REG_CTRL1               0x20
#define LIS3MDL_REG_CTRL2               0x21
#define LIS3MDL_REG_CTRL3               0x22
#define LIS3MDL_REG_CTRL4               0x23
#define LIS3MDL_REG_CTRL5               0x24

#define LIS3MDL_REG_SAMPLE_START        0x28

#define LIS3MDL_REG_INT_CFG             0x30
#define LIS3MDL_INT_X_EN                BIT(7)
#define LIS3MDL_INT_Y_EN                BIT(6)
#define LIS3MDL_INT_Z_EN                BIT(5)
#define LIS3MDL_INT_XYZ_EN              \
	(LIS3MDL_INT_X_EN | LIS3MDL_INT_Y_EN | LIS3MDL_INT_Z_EN)

#define LIS3MDL_STATUS_REG        0x27

/* REG_CTRL2 */
#define LIS3MDL_REBOOT_MASK             BIT(3)
#define LIS3MDL_SOFT_RST_MASK           BIT(2)

/* REG_CTRL1 */
#define LIS3MDL_OM_SHIFT                5
#define LIS3MDL_DO_SHIFT                2
#define LIS3MDL_FAST_ODR_SHIFT          1
#define LIS3MDL_ODR_BITS(om_bits, do_bits, fast_odr)	\
	(((om_bits) << LIS3MDL_OM_SHIFT) |		\
	 ((do_bits) << LIS3MDL_DO_SHIFT) |		\
	 ((fast_odr) << LIS3MDL_FAST_ODR_SHIFT))
static const uint8_t lis3mdl_odr_bits[] = {
		LIS3MDL_ODR_BITS(0, 0, 0), /* 0.625 Hz */
		LIS3MDL_ODR_BITS(0, 1, 0), /* 1.25 Hz */
		LIS3MDL_ODR_BITS(0, 2, 0), /* 2.5 Hz */
		LIS3MDL_ODR_BITS(0, 3, 0), /* 5 Hz */
		LIS3MDL_ODR_BITS(0, 4, 0), /* 10 Hz */
		LIS3MDL_ODR_BITS(0, 5, 0), /* 20 Hz */
		LIS3MDL_ODR_BITS(0, 6, 0), /* 40 Hz */
		LIS3MDL_ODR_BITS(0, 7, 0), /* 80 Hz */
		LIS3MDL_ODR_BITS(3, 0, 1), /* 155 Hz */
		LIS3MDL_ODR_BITS(2, 0, 1), /* 300 Hz */
		LIS3MDL_ODR_BITS(1, 0, 1), /* 560 Hz */
		LIS3MDL_ODR_BITS(0, 0, 1)  /* 1000 Hz */
};
#define LIS3MDL_ODR lis3mdl_odr_bits[4]

/* REG_CTRL3 */
#define LIS3MDL_MD_CONTINUOUS          0x00

/* Others */
#define LIS3MDL_SENSITIVITY       6842

static int lsm6dsl_lis3mdl_init(const struct device *dev, uint8_t i2c_addr)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t mag_cfg[2];

	data->magn_sensitivity = LIS3MDL_SENSITIVITY;

	/* sw reset device */
	mag_cfg[0] = LIS3MDL_REBOOT_MASK | LIS3MDL_SOFT_RST_MASK;
	lsm6dsl_shub_write_slave_reg(dev, i2c_addr,
				     LIS3MDL_REG_CTRL2, mag_cfg, 1);

	k_sleep(K_MSEC(10)); /* turn-on time in ms */

	/* configure mag */
	mag_cfg[0] = LIS3MDL_ODR;
	lsm6dsl_shub_write_slave_reg(dev, i2c_addr,
				     LIS3MDL_REG_CTRL1, mag_cfg, 1);

	mag_cfg[0] = LIS3MDL_MD_CONTINUOUS;
	lsm6dsl_shub_write_slave_reg(dev, i2c_addr,
				     LIS3MDL_REG_CTRL3, mag_cfg, 1);
	return 0;
}
#endif /* CONFIG_LSM6DSL_EXT0_LIS3MDL */

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

static int lsm6dsl_lps22hb_init(const struct device *dev, uint8_t i2c_addr)
{
	uint8_t baro_cfg[2];

	/* sw reset device */
	baro_cfg[0] = LPS22HB_SW_RESET;
	lsm6dsl_shub_write_slave_reg(dev, i2c_addr,
				     LPS22HB_CTRL_REG2, baro_cfg, 1);

	k_sleep(K_MSEC(1)); /* turn-on time in ms */

	/* configure device */
	baro_cfg[0] = LPS22HB_ODR_10HZ | LPS22HB_LPF_EN | LPS22HB_BDU_EN;
	lsm6dsl_shub_write_slave_reg(dev, i2c_addr,
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
	int (*dev_init)(const struct device *dev, uint8_t i2c_addr);
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

#ifdef CONFIG_LSM6DSL_EXT0_LIS3MDL
	{
		/* LIS3MDL */
		.i2c_addr       = {0x1C, 0x1E},
		.wai_addr       = 0x0F,
		.wai_val        = 0x3D,
		.out_data_addr  = 0x28,
		.out_data_len   = 0x06,
		.dev_init       = (lsm6dsl_lis3mdl_init),
	},
#endif  /* CONFIG_LSM6DSL_EXT0_LIS3MDL */

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

static inline void lsm6dsl_shub_wait_completed(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint16_t freq;

	freq = (data->accel_freq == 0U) ? 26 : data->accel_freq;
	k_msleep((2000U / freq) + 1);
}

static inline void lsm6dsl_shub_embedded_en(const struct device *dev, bool on)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t func_en = (on) ? 0x1 : 0x0;

	data->hw_tf->update_reg(dev, LSM6DSL_REG_FUNC_CFG_ACCESS,
				LSM6DSL_MASK_FUNC_CFG_EN,
				func_en << LSM6DSL_SHIFT_FUNC_CFG_EN);

	k_sleep(K_MSEC(1));
}

#ifdef LSM6DSL_DEBUG
static int lsm6dsl_read_embedded_reg(const struct device *dev,
				     uint8_t reg_addr, uint8_t *value, int len)
{
	struct lsm6dsl_data *data = dev->data;
	lsm6dsl_shub_embedded_en(dev, true);

	if (data->hw_tf->read_data(dev, reg_addr, value, len) < 0) {
		LOG_DBG("failed to read external reg: %02x", reg_addr);
		lsm6dsl_shub_embedded_en(dev, false);
		return -EIO;
	}

	lsm6dsl_shub_embedded_en(dev, false);

	return 0;
}
#endif

static int lsm6dsl_shub_write_embedded_regs(const struct device *dev,
					    uint8_t reg_addr,
					    uint8_t *value, uint8_t len)
{
	struct lsm6dsl_data *data = dev->data;
	lsm6dsl_shub_embedded_en(dev, true);

	if (data->hw_tf->write_data(dev, reg_addr, value, len) < 0) {
		LOG_DBG("failed to write external reg: %02x", reg_addr);
		lsm6dsl_shub_embedded_en(dev, false);
		return -EIO;
	}

	lsm6dsl_shub_embedded_en(dev, false);

	return 0;
}

static void lsm6dsl_shub_enable(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;

	/* Enable Digital Func */
	data->hw_tf->update_reg(dev, LSM6DSL_REG_CTRL10_C,
				LSM6DSL_MASK_CTRL10_C_FUNC_EN,
				1 << LSM6DSL_SHIFT_CTRL10_C_FUNC_EN);

	/* Enable Accel @26hz */
	if (!data->accel_freq) {
		data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_ODR_XL,
				    2 << LSM6DSL_SHIFT_CTRL1_XL_ODR_XL);
	}

	/* Enable Sensor Hub */
	data->hw_tf->update_reg(dev, LSM6DSL_REG_MASTER_CONFIG,
				LSM6DSL_MASK_MASTER_CONFIG_MASTER_ON,
				1 << LSM6DSL_SHIFT_MASTER_CONFIG_MASTER_ON);
}

static void lsm6dsl_shub_disable(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;

	/* Disable Sensor Hub */
	data->hw_tf->update_reg(dev, LSM6DSL_REG_MASTER_CONFIG,
				LSM6DSL_MASK_MASTER_CONFIG_MASTER_ON,
				0 << LSM6DSL_SHIFT_MASTER_CONFIG_MASTER_ON);

	/* Disable Accel */
	if (!data->accel_freq) {
		data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_ODR_XL,
				    0 << LSM6DSL_SHIFT_CTRL1_XL_ODR_XL);
	}

	/* Disable Digital Func */
	data->hw_tf->update_reg(dev, LSM6DSL_REG_CTRL10_C,
				LSM6DSL_MASK_CTRL10_C_FUNC_EN,
				0 << LSM6DSL_SHIFT_CTRL10_C_FUNC_EN);
}

/*
 * use SLV0 for generic read to slave device
 */
static int lsm6dsl_shub_read_slave_reg(const struct device *dev,
				       uint8_t slv_addr, uint8_t slv_reg,
				       uint8_t *value, uint16_t len)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t slave[3];

	slave[0] = (slv_addr << 1) | LSM6DSL_EMBEDDED_SLVX_READ;
	slave[1] = slv_reg;
	slave[2] = (len & 0x7);

	if (lsm6dsl_shub_write_embedded_regs(dev, LSM6DSL_EMBEDDED_SLV0_ADDR,
					     slave, 3) < 0) {
		LOG_DBG("error writing embedded reg");
		return -EIO;
	}

	/* turn SH on */
	lsm6dsl_shub_enable(dev);
	lsm6dsl_shub_wait_completed(dev);
	data->hw_tf->read_data(dev, LSM6DSL_REG_SENSORHUB1, value, len);

	lsm6dsl_shub_disable(dev);
	return 0;
}

/*
 * use SLV0 to configure slave device
 */
static int lsm6dsl_shub_write_slave_reg(const struct device *dev,
					uint8_t slv_addr, uint8_t slv_reg,
					uint8_t *value, uint16_t len)
{
	uint8_t slv_cfg[3];
	uint8_t cnt = 0U;

	while (cnt < len) {
		slv_cfg[0] = (slv_addr << 1) & ~LSM6DSL_EMBEDDED_SLVX_READ;
		slv_cfg[1] = slv_reg + cnt;

		if (lsm6dsl_shub_write_embedded_regs(dev,
						     LSM6DSL_EMBEDDED_SLV0_ADDR,
						     slv_cfg, 2) < 0) {
			LOG_DBG("error writing embedded reg");
			return -EIO;
		}

		slv_cfg[0] = value[cnt];
		if (lsm6dsl_shub_write_embedded_regs(dev,
					LSM6DSL_EMBEDDED_SLV0_DATAWRITE,
					slv_cfg, 1) < 0) {
			LOG_DBG("error writing embedded reg");
			return -EIO;
		}

		/* turn SH on */
		lsm6dsl_shub_enable(dev);
		lsm6dsl_shub_wait_completed(dev);
		lsm6dsl_shub_disable(dev);

		cnt++;
	}

	/* Put master in IDLE mode */
	slv_cfg[0] = LSM6DSL_EMBEDDED_SLV0_WRITE_IDLE;
	slv_cfg[1] = lsm6dsl_shub_sens_list[0].wai_addr;
	slv_cfg[2] = LSM6DSL_EMBEDDED_SLVX_THREE_SENS;
	if (lsm6dsl_shub_write_embedded_regs(dev,
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
static int lsm6dsl_shub_set_data_channel(const struct device *dev)
{
	uint8_t slv_cfg[3];
	uint8_t slv_i2c_addr = lsm6dsl_shub_sens_list[0].i2c_addr[ext_i2c_addr];

	/* SLV0 is used for generic write */
	slv_cfg[0] = LSM6DSL_EMBEDDED_SLV0_WRITE_IDLE;
	slv_cfg[1] = lsm6dsl_shub_sens_list[0].wai_addr;
	slv_cfg[2] = LSM6DSL_EMBEDDED_SLVX_THREE_SENS;
	if (lsm6dsl_shub_write_embedded_regs(dev,
					     LSM6DSL_EMBEDDED_SLV0_ADDR,
					     slv_cfg, 3) < 0) {
		LOG_DBG("error writing embedded reg");
		return -EIO;
	}

	/* Set data channel for slave device */
	slv_cfg[0] = (slv_i2c_addr << 1) | LSM6DSL_EMBEDDED_SLVX_READ;
	slv_cfg[1] = lsm6dsl_shub_sens_list[0].out_data_addr;
	slv_cfg[2] = lsm6dsl_shub_sens_list[0].out_data_len;
	if (lsm6dsl_shub_write_embedded_regs(dev,
					     LSM6DSL_EMBEDDED_SLV1_ADDR,
					     slv_cfg, 3) < 0) {
		LOG_DBG("error writing embedded reg");
		return -EIO;
	}

	/* turn SH on */
	lsm6dsl_shub_enable(dev);
	lsm6dsl_shub_wait_completed(dev);

	return 0;
}

int lsm6dsl_shub_read_external_chip(const struct device *dev, uint8_t *buf,
				    uint8_t len)
{
	struct lsm6dsl_data *data = dev->data;

	data->hw_tf->read_data(dev, LSM6DSL_REG_SENSORHUB1, buf, len);

	return 0;
}

int lsm6dsl_shub_init_external_chip(const struct device *dev)
{
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

		if (lsm6dsl_shub_read_slave_reg(dev, slv_i2c_addr,
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
	lsm6dsl_shub_sens_list[0].dev_init(dev, slv_i2c_addr);

	lsm6dsl_shub_set_data_channel(dev);

	return 0;
}
