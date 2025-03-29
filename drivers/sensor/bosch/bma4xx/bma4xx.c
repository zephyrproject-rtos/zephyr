/* Bosch BMA4xx 3-axis accelerometer driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 Croxel Inc.
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/rtio/work.h>

#include "bma4xx.h"
#include "bma4xx_interrupt.h"
#include "bma4xx_decoder.h"
#include "bma4xx_rtio.h"

LOG_MODULE_REGISTER(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Helper for converting m/s^2 offset values into register values
 */
static int bma4xx_offset_to_reg_val(const struct sensor_value *val, uint8_t *reg_val)
{
	int32_t ug = sensor_ms2_to_ug(val);

	if (ug < BMA4XX_OFFSET_MICROG_MIN || ug > BMA4XX_OFFSET_MICROG_MAX) {
		return -ERANGE;
	}

	*reg_val = ug / BMA4XX_OFFSET_MICROG_PER_BIT;
	return 0;
}

/**
 * @brief Set the X, Y, or Z axis offsets.
 */
static int bma4xx_attr_set_offset(const struct device *dev, enum sensor_channel chan,
				  const struct sensor_value *val)
{
	struct bma4xx_data *bma4xx = dev->data;
	uint8_t reg_addr;
	uint8_t reg_val[3];
	int rc;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		reg_addr = BMA4XX_REG_OFFSET_0 + (chan - SENSOR_CHAN_ACCEL_X);
		rc = bma4xx_offset_to_reg_val(val, &reg_val[0]);
		if (rc) {
			return rc;
		}
		return bma4xx->hw_ops->write_reg(dev, reg_addr, reg_val[0]);
	case SENSOR_CHAN_ACCEL_XYZ:
		/* Expect val to point to an array of three sensor_values */
		reg_addr = BMA4XX_REG_OFFSET_0;
		for (int i = 0; i < 3; i++) {
			rc = bma4xx_offset_to_reg_val(&val[i], &reg_val[i]);
			if (rc) {
				return rc;
			}
		}
		return bma4xx->hw_ops->write_data(dev, reg_addr, (uint8_t *)reg_val,
						  sizeof(reg_val));
	default:
		return -ENOTSUP;
	}
}

static const uint32_t odr_to_reg_map[] = {
	0,          /* Invalid */
	781250,     /* 0.78125 Hz (25/32) => 0x1 */
	1562500,    /* 1.5625 Hz (25/16) => 0x2 */
	3125000,    /* 3.125 Hz (25/8) => 0x3 */
	6250000,    /* 6.25 Hz (25/4) => 0x4 */
	12500000,   /* 12.5 Hz (25/2) => 0x5 */
	25000000,   /* 25 Hz => 0x6 */
	50000000,   /* 50 Hz => 0x7*/
	100000000,  /* 100 Hz => 0x8*/
	200000000,  /* 200 Hz => 0x9*/
	400000000,  /* 400 Hz => 0xa*/
	800000000,  /* 800 Hz => 0xb*/
	1600000000, /* 1600 Hz => 0xc*/
};

/**
 * @brief Convert an ODR rate in Hz to a register value
 */
static int bma4xx_odr_to_reg(uint32_t microhertz, uint8_t *reg_val)
{
	if (microhertz == 0) {
		/* Illegal ODR value */
		return -ERANGE;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(odr_to_reg_map); i++) {
		if (microhertz <= odr_to_reg_map[i]) {
			*reg_val = i;
			return 0;
		}
	}

	/* Requested ODR too high */
	return -ERANGE;
}

/**
 * Set the sensor's acceleration offset (per axis).
 */
static int bma4xx_attr_set_odr(const struct sensor_value *val,
			       struct bma4xx_runtime_config *new_config)
{
	int status;
	uint8_t reg_val;

	/* Convert ODR Hz value to microhertz and round up to closest register setting */
	status = bma4xx_odr_to_reg(val->val1 * 1000000 + val->val2, &reg_val);
	if (status < 0) {
		return status;
	}

	new_config->accel_odr = reg_val;

	return 0;
}

static const uint32_t fs_to_reg_map[] = {
	2000000,  /* +/-2G => 0x0 */
	4000000,  /* +/-4G => 0x1 */
	8000000,  /* +/-8G => 0x2 */
	16000000, /* +/-16G => 0x3 */
};

static int bma4xx_fs_to_reg(int32_t range_ug, uint8_t *reg_val)
{
	if (range_ug == 0) {
		/* Illegal value */
		return -ERANGE;
	}

	range_ug = abs(range_ug);

	for (uint8_t i = 0; i < 4; i++) {
		if (range_ug <= fs_to_reg_map[i]) {
			*reg_val = i;
			return 0;
		}
	}

	/* Requested range too high */
	return -ERANGE;
}

/**
 * Set the sensor's full-scale range
 */
static int bma4xx_attr_set_range(const struct sensor_value *val,
				 struct bma4xx_runtime_config *new_config)
{
	int status;
	uint8_t reg_val;

	/* Convert m/s^2 to micro-G's and find closest register setting */
	status = bma4xx_fs_to_reg(sensor_ms2_to_ug(val), &reg_val);
	if (status < 0) {
		return status;
	}

	new_config->accel_fs_range = reg_val;

	return 0;
}

/**
 * Set the sensor's bandwidth parameter (one of BMA4XX_BWP_*)
 */
static int bma4xx_attr_set_bwp(const struct sensor_value *val,
			       struct bma4xx_runtime_config *new_config)
{
	/* Require that `val2` is unused, and that `val1` is in range of a valid BWP */
	if (val->val2 || val->val1 < BMA4XX_BWP_OSR4_AVG1 || val->val1 > BMA4XX_BWP_RES_AVG128) {
		return -EINVAL;
	}

	new_config->accel_bwp = (((uint8_t)val->val1) << BMA4XX_SHIFT_ACC_CONF_BWP);

	return 0;
}

/**
 * @brief Implement the sensor API attribute set method.
 */
static int bma4xx_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct bma4xx_data *data = dev->data;
	struct bma4xx_runtime_config new_config = data->cfg;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = bma4xx_attr_set_odr(val, &new_config);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = bma4xx_attr_set_range(val, &new_config);
		} else if (attr == SENSOR_ATTR_OFFSET) {
			res = bma4xx_attr_set_offset(dev, chan, val);
		} else if (attr == SENSOR_ATTR_CONFIGURATION) {
			/* Use for setting the bandwidth parameter (BWP) */
			res = bma4xx_attr_set_bwp(val, &new_config);
		} else {
			LOG_ERR("Unsupported attribute");
			res = -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_ALL:
		if (attr == SENSOR_ATTR_BATCH_DURATION) {
			if (val->val1 < 0) {
				return -EINVAL;
			}
			new_config.batch_ticks = val->val1;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;
	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	if (res) {
		LOG_ERR("Failed to set attribute");
		return res;
	}

	return bma4xx_safely_configure(dev, &new_config);
}

/**
 * Internal device initialization function for both bus types.
 */
static int bma4xx_chip_init(const struct device *dev)
{
	struct bma4xx_data *bma4xx = dev->data;
	const struct bma4xx_config *cfg = dev->config;
	int status;

	/* Sensor bus-specific initialization */
	status = cfg->bus_init(dev);
	if (status) {
		LOG_ERR("Failed to initialize bus: %d", status);
		return status;
	}

	/* Read Chip ID */
	status = bma4xx->hw_ops->read_reg(dev, BMA4XX_REG_CHIP_ID, &bma4xx->chip_id);
	if (status) {
		LOG_ERR("could not read chip_id: %d", status);
		return status;
	}
	LOG_DBG("chip_id is 0x%02x", bma4xx->chip_id);

	if (bma4xx->chip_id != BMA4XX_CHIP_ID_BMA422) {
		LOG_WRN("Driver tested for BMA422. Check for unintended operation.");
	}

	/* Issue soft reset command */
	status = bma4xx->hw_ops->write_reg(dev, BMA4XX_REG_CMD, BMA4XX_CMD_SOFT_RESET);
	if (status) {
		LOG_ERR("Could not soft-reset chip: %d", status);
		return status;
	}

	if (IS_ENABLED(CONFIG_BMA4XX_STREAM)) {
		status = bma4xx_init_interrupt(dev);
		if (status != 0) {
			LOG_ERR("Failed to initialize bma4xx interrupt");
			return status;
		}
	}

	/* Default is: range = +/-4G, ODR = 100 Hz, BWP = "NORM_AVG4" */
	bma4xx->cfg.accel_fs_range = BMA4XX_RANGE_4G;
	bma4xx->cfg.accel_bwp = BMA4XX_BWP_NORM_AVG4;
	bma4xx->cfg.accel_odr = BMA4XX_ODR_100;

	status = bma4xx_configure(dev, &bma4xx->cfg);
	if (status) {
		LOG_ERR("Failed to initialize bma4xx trigger");
		return status;
	}

	return 0;
}

/*
 * Sensor driver API
 */

static DEVICE_API(sensor, bma4xx_driver_api) = {
	.attr_set = bma4xx_attr_set,
	.get_decoder = bma4xx_get_decoder,
	.submit = bma4xx_submit,
};

/*
 * Device instantiation macros
 */

#define BMA4XX_RTIO_I2C_DEFINE(inst)                                                               \
	I2C_DT_IODEV_DEFINE(bma4xx_iodev_##inst, DT_DRV_INST(inst));                               \
	RTIO_DEFINE(bma4xx_rtio_##inst, 8, 8);

#define BMA4XX_RTIO_SPI_DEFINE(inst)                                                               \
	SPI_DT_IODEV_DEFINE(bma4xx_iodev_##inst, DT_DRV_INST(inst));                               \
	RTIO_DEFINE(bma4xx_rtio_##inst, 8, 8);

#define BMA4XX_DEFINE_RTIO(inst)                                                                   \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (BMA4XX_RTIO_SPI_DEFINE(inst)), \
	(BMA4XX_RTIO_I2C_DEFINE(inst)))

/* Initializes a struct bma4xx_config for an instance on a SPI bus.
 * SPI operation is not currently supported.
 */
#define BMA4XX_CONFIG_SPI(inst)                                                                    \
	.bus_cfg.spi = SPI_DT_SPEC_INST_GET(inst, 0, 0), .bus_init = &bma4xx_spi_init,             \
	.bus_type = BMA4XX_BUS_SPI,

/* Initializes a struct bma4xx_config for an instance on an I2C bus. */
#define BMA4XX_CONFIG_I2C(inst)                                                                    \
	.bus_cfg.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_init = &bma4xx_i2c_init,                   \
	.bus_type = BMA4XX_BUS_I2C,

#define BMA4XX_DEFINE_BUS(inst)                                                                    \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (BMA4XX_CONFIG_SPI(inst)), (BMA4XX_CONFIG_I2C(inst)))

#define BMA4XX_DT_CONFIG_INIT(inst)                                                                \
	{                                                                                          \
		.fifo_en = IS_ENABLED(CONFIG_BMA4XX_STREAM),                                       \
		.batch_ticks = 0,                                                                  \
		.interrupt1_fifo_wm = false,                                                       \
		.interrupt1_fifo_full = false,                                                     \
	}

#define BMA4XX_DEFINE_DATA(inst)                                                                   \
	BMA4XX_DEFINE_RTIO(inst);                                                                  \
	static struct bma4xx_data bma4xx_driver_##inst = {                                         \
		.cfg = BMA4XX_DT_CONFIG_INIT(inst),                                                \
		.r = &bma4xx_rtio_##inst,                                                          \
		.iodev = &bma4xx_iodev_##inst,                                                     \
	};

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BMA4XX_INIT(inst)                                                                          \
	BMA4XX_DEFINE_DATA(inst);                                                                  \
                                                                                                   \
	static const struct bma4xx_config bma4xx_config_##inst = {                                 \
		BMA4XX_DEFINE_BUS(inst)                                                            \
			IF_ENABLED(CONFIG_BMA4XX_STREAM, ( \
		.gpio_interrupt = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),))};         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bma4xx_chip_init, NULL, &bma4xx_driver_##inst,          \
				     &bma4xx_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bma4xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMA4XX_INIT);
