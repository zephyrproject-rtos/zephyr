/*
 * Copyright (c) Nexplore Technology GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis3dhh

#include <init.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <math.h>
#include "lis3dhh.h"

#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
#define SUPPLY_PIN DT_INST_GPIO_PIN(0, supply_gpios)
#endif

LOG_MODULE_REGISTER(lis3dhh, CONFIG_SENSOR_LOG_LEVEL);

static inline bool is_error(int status)
{
	return (status < 0);
}

static inline bool no_device(const struct device *device)
{
	return (!device);
}

/**
 * @brief converts the raw data from the sensor to engineering units (g)
 *
 * @param raw_val the raw value from the sensor
 * @param val sensor_value to which the converted value is written (ptr)
 */
static void lis3dhh_convert(int16_t raw_val, struct sensor_value *val)
{
	float_t result;

	result = (float_t)raw_val * 0.076f; /* typ. 0.076 mg/digit */
	result = result / 1000.0f;          /* convert from mg to g */
	val->val1 = (int32_t)result;        /* integer part of the value */
    /* fractional part of the value (in one-millionth parts) */
	val->val2 = ((int32_t)(result * 1000000.0f) % 1000000);
}

static int lis3dhh_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	/* TODO: temperature readout */
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	uint8_t ofs_start;
	uint8_t ofs_end;
	uint8_t i;

	if (chan == SENSOR_CHAN_ACCEL_X) {
		ofs_start = ofs_end = 0;
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		ofs_start = ofs_end = 1;
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		ofs_start = ofs_end = 2;
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		ofs_start = 0;
		ofs_end = 2;
	} else {
		return -ENOTSUP;
	}

	for (i = ofs_start; i <= ofs_end; i++, val++) {
		lis3dhh_convert(lis3dhh_drv_data->sample.xyz[i], val);
	}

	return 0;
}

int lis3dhh_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	size_t i;
	int status;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_ACCEL_XYZ);

	/*
	 * Since all accel data register addresses are consecutive,
	 * a burst read can be used to read all the samples. First register
	 * is status register.
	 */
	status = lis3dhh_drv_data->hw_tf->read_data(
		dev, LIS3DHH_STATUS, lis3dhh_drv_data->sample.raw,
		sizeof(lis3dhh_drv_data->sample.raw));

	if (is_error(status)) {
		LOG_WRN("Could not read acceleration axis data.");
		return status;
	}

	for (i = 0; i < (3 * sizeof(int16_t)); i += sizeof(int16_t)) {
		int16_t *sample =
			(int16_t *)&lis3dhh_drv_data->sample
			.raw[1 + i];
		*sample = sys_le16_to_cpu(
			*sample);
	}

	if (lis3dhh_drv_data->sample.status &
	    LIS3DHH_INT1_CTRL_DRDY) {
		return 0;
	}

	return -ENODATA;
}

static const struct sensor_driver_api lis3dhh_driver_api = { .sample_fetch = lis3dhh_sample_fetch,
							     .channel_get = lis3dhh_channel_get };

int lis3dhh_init(const struct device *dev)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	const struct lis3dhh_config *cfg = dev->config;
	int status;
	uint8_t id;
	uint8_t CTRL_REG1_MASK = 0;
	uint8_t CTRL_REG4_MASK = LIS3DHH_CTRL_REG4_ONE_1;
	uint8_t FIFO_CTRL_MASK = 0;

#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
	lis3dhh_drv_data->supply_gpios = device_get_binding(DT_INST_GPIO_LABEL(0,
										supply_gpios));
	if (no_device(lis3dhh_drv_data->supply_gpios)) {
		LOG_ERR("Failed to get pointer to power-supply
				gpio: %s", DT_INST_GPIO_LABEL(0, supply_gpios));
		return -EINVAL;
	}
	gpio_pin_configure(lis3dhh_drv_data->supply_gpios, SUPPLY_PIN,
				GPIO_OUTPUT_ACTIVE | DT_INST_GPIO_FLAGS(0, supply_gpios));
	k_sleep(K_MSEC(10)); /* wait 10ms for device to finish booting */
#endif

	lis3dhh_drv_data->bus = device_get_binding(
		cfg->bus_name);

	if (no_device(lis3dhh_drv_data->bus)) {
		LOG_ERR("Master not found: %s", cfg->bus_name);
		return -EINVAL;
	}

	status = cfg->bus_init(dev); /* spi bus initialization */
	if (is_error(status)) {
		LOG_ERR("SPI bus initialization failed. Errorcode: %i", status);
	}

	status = lis3dhh_drv_data->hw_tf->read_reg(
		dev, LIS3DHH_REG_WHO_AM_I,
		&id);

	if (is_error(status)) {
		LOG_ERR("Failed to read chip id. Errorcode: %i", status);
		return status;
	}

	if (id != LIS3DHH_CHIP_ID) {
		LOG_ERR("Invalid chip id: 0x%02x", id);
		return -EINVAL;
	}

	/* CTRL_REG1 configuration */
#if defined(CONFIG_LIS3DHH_NORMAL_MODE)
	CTRL_REG1_MASK |= LIS3DHH_CTRL_REG1_NORM_MODE_EN;
#else
	CTRL_REG1_MASK &= ~LIS3DHH_CTRL_REG1_NORM_MODE_EN;
#endif
#if defined(CONFIG_LIS3DHH_IF_ADD_INC)
	CTRL_REG1_MASK |= LIS3DHH_CTRL_REG1_IF_ADD_INC;
#else
	CTRL_REG1_MASK &= ~LIS3DHH_CTRL_REG1_IF_ADD_INC;
#endif
#if defined(CONFIG_LIS3DHH_ENABLE_BDU)
	CTRL_REG1_MASK |= LIS3DHH_CTRL_REG1_BDU;
#endif

	status = lis3dhh_drv_data->hw_tf->write_reg(dev, LIS3DHH_CTRL_REG1, CTRL_REG1_MASK);
	if (is_error(status)) {
		LOG_ERR("Failed to configure CTRL_REG1. Errorcode: %i", status);
		return status;
	}

	/* INT1_CTRL configuration */
#if defined(CONFIG_LIS3DHH_INT1_AS_EXT_ASYNC_INPUT_TRIG)
	status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1, LIS3DHH_INT1_CTRL_EXT);
	if (is_error(status)) {
		LOG_ERR("Failed to update INT1_CTRL as external asynchronous input
			trigger to FIFO. Errorcode: %i", status);
		return status;
	}
#endif

	/* CTRL_REG4 configuration */
#if defined(CONFIG_LIS3DHH_FILTER_FIR)
	CTRL_REG4_MASK &= ~LIS3DHH_CTRL_REG4_DSP_LP_TYPE;
#elif defined(CONFIG_LIS3DHH_FILTER_IIR)
	CTRL_REG4_MASK |= LIS3DHH_CTRL_REG4_DSP_LP_TYPE;
#endif
#if defined(CONFIG_LIS3DHH_BANDWIDTH_440HZ)
	CTRL_REG4_MASK &= ~LIS3DHH_CTRL_REG4_DSP_LP_TYPE;
#elif defined(CONFIG_LIS3DHH_BANDWIDTH_235HZ)
	CTRL_REG4_MASK |= LIS3DHH_CTRL_REG4_DSP_LP_TYPE;
#endif
#if defined(CONFIG_LIS3DHH_INT1_PUSH_PULL)
	CTRL_REG4_MASK &= ~LIS3DHH_CTRL_REG4_PP_OD_INT1;
#elif defined(CONFIG_LIS3DHH_INT1_OPEN_DRAIN)
	CTRL_REG4_MASK |= LIS3DHH_CTRL_REG4_PP_OD_INT1;
#endif
#if defined(CONFIG_LIS3DHH_INT2_PUSH_PULL)
	CTRL_REG4_MASK &= ~LIS3DHH_CTRL_REG4_PP_OD_INT2;
#elif defined(CONFIG_LIS3DHH_INT2_OPEN_DRAIN)
	CTRL_REG4_MASK |= LIS3DHH_CTRL_REG4_PP_OD_INT2;
#endif
#if defined(CONFIG_LIS3DHH_ENABLE_FIFO)
	CTRL_REG4_MASK |= LIS3DHH_CTRL_REG4_FIFO_EN;
#endif

	status = lis3dhh_drv_data->hw_tf->write_reg(dev, LIS3DHH_CTRL_REG4, CTRL_REG4_MASK);
	if (is_error(status)) {
		LOG_ERR("Failed to configure CTRL_REG4. Errorcode: %i", status);
		return status;
	}

	/* CTRL_REG_5 configuration */
#if defined(CONFIG_LIS3DHH_SPI_HS_CONFIG)
	status = lis3dhh_drv_data->hw_tf->write_reg(dev, LIS3DHH_CTRL_REG5,
				LIS3DHH_CTRL_REG5_FIFO_SPI_HS_ON);
	if (is_error(status)) {
		LOG_ERR("Failed to configure CTRL_REG5. Errorcode: %i", status);
		return status;
	}
#endif

	/* FIFO configuration */
#if defined(CONFIG_LIS3DHH_FIFO_BYPASS)
	FIFO_CTRL_MASK &= ~(LIS3DHH_FIFO_CTRL_FMODE0 |
		LIS3DHH_FIFO_CTRL_FMODE1 | LIS3DHH_FIFO_CTRL_FMODE2)
#elif defined(CONFIG_LIS3DHH_FIFO_NORMAL)
	FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FMODE0;
#elif defined(CONFIG_LIS3DHH_FIFO_CONTINUOUS_TO_FIFO)
	FIFO_CTRL_MASK = FIFO_CTRL_MASK | LIS3DHH_FIFO_CTRL_FMODE0 | LIS3DHH_FIFO_CTRL_FMODE1;
#elif defined(CONFIG_LIS3DHH_FIFO_BYPASS_TO_CONTINUOUS)
	FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FMODE2;
#elif defined(CONFIG_LIS3DHH_FIFO_CONTINUOUS)
	FIFO_CTRL_MASK = FIFO_CTRL_MASK | LIS3DHH_FIFO_CTRL_FMODE1 | LIS3DHH_FIFO_CTRL_FMODE2;
#endif
#if defined(CONFIG_LIS3DHH_FIFO_THRESH_ADD_ONE)
	FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH0;
#endif
#if defined(CONFIG_LIS3DHH_FIFO_THRESH_ADD_TWO)
	FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH1;
#endif
#if defined(CONFIG_LIS3DHH_FIFO_THRESH_ADD_FOUR)
	FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH2;
#endif
#if defined(CONFIG_LIS3DHH_FIFO_THRESH_ADD_EIGHT)
	FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH3;
#endif
#if defined(CONFIG_LIS3DHH_FIFO_THRESH_ADD_SIXTEEN)
	FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH4;
#endif

	status = lis3dhh_drv_data->hw_tf->write_reg(dev, LIS3DHH_FIFO_CTRL, FIFO_CTRL_MASK);
	if (is_error(status)) {
		LOG_ERR("Failed to configure FIFO_CTRL. Errorcode: %i", status);
		return status;
	}

	return status;
}

#define LIS3DHH_DEVICE_INIT(inst)                                                   \
	DEVICE_DT_INST_DEFINE(inst, lis3dhh_init, NULL, &lis3dhh_data_##inst,           \
			      &lis3dhh_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
			      &lis3dhh_driver_api);

#define LIS3DHH_HAS_CS(inst) DT_INST_SPI_DEV_HAS_CS_GPIOS(inst)

#define LIS3DHH_DATA_SPI_CS(inst)                     \
	{                                                 \
		.cs_ctrl = {                                  \
			.gpio_pin =                               \
				DT_INST_SPI_DEV_CS_GPIOS_PIN(inst),   \
			.gpio_dt_flags =                          \
				DT_INST_SPI_DEV_CS_GPIOS_FLAGS(inst), \
		},                                            \
	}

#define LIS3DHH_DATA_SPI(inst) COND_CODE_1(LIS3DHH_HAS_CS(inst),
			(LIS3DHH_DATA_SPI_CS(inst)), ({}))

#define LIS3DHH_SPI_CS_PTR(inst) COND_CODE_1(LIS3DHH_HAS_CS(inst),
			(&(lis3dhh_data_##inst.cs_ctrl)), (NULL))

#define LIS3DHH_SPI_CS_LABEL(inst) COND_CODE_1(LIS3DHH_HAS_CS(inst),
			(DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst)), (NULL))

#define LIS3DHH_SPI_CFG(inst)                          \
	(&(struct lis3dhh_spi_cfg){                        \
		.spi_conf = {                                  \
			.frequency =                               \
				DT_INST_PROP(inst, spi_max_frequency), \
			.operation = (SPI_WORD_SET(8) |            \
				      SPI_OP_MODE_MASTER |             \
				      SPI_TRANSFER_MSB),               \
			.slave = DT_INST_REG_ADDR(inst),           \
			.cs = LIS3DHH_SPI_CS_PTR(inst),            \
		},                                             \
		.cs_gpios_label = LIS3DHH_SPI_CS_LABEL(inst),  \
	})

#define LIS3DHH_CFG_INT(inst)

#define LIS3DHH_CONFIG_SPI(inst)                         \
	{                                                    \
		.bus_name = DT_INST_BUS_LABEL(inst),             \
		.bus_init = lis3dhh_spi_init,                    \
		.bus_cfg = { .spi_cfg = LIS3DHH_SPI_CFG(inst) }, \
		LIS3DHH_CFG_INT(inst)                            \
	}

#define LIS3DHH_DEFINE_SPI(inst)                               \
	static struct lis3dhh_data lis3dhh_data_##inst =           \
		LIS3DHH_DATA_SPI(inst);                                \
	static const struct lis3dhh_config lis3dhh_config_##inst = \
		LIS3DHH_CONFIG_SPI(inst);                              \
	LIS3DHH_DEVICE_INIT(inst)

#define LIS3DHH_DEFINE(inst) LIS3DHH_DEFINE_SPI(inst)

DT_INST_FOREACH_STATUS_OKAY(LIS3DHH_DEFINE)
