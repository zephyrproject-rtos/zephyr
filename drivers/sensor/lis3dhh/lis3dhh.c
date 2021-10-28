/*
 * Copyright (C) NEXPLORE
 * https://www.nexplore.com
 *
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

static inline bool no_device(const struct device *dev)
{
	return (!dev);
}

/**
 * @brief Converts the raw data from the sensor to engineering units (g).
 *
 * @param raw_val Raw value from the sensor
 * @param val sensor_value to which the converted value is written (ptr)
 */
static void lis3dhh_convert(int16_t raw_val, struct sensor_value *val)
{
	float_t result;

	result = (float_t)raw_val * 0.076f;     /* typ. 0.076 mg/digit */
	result = result / 1000.0f;              /* convert from mg to g */
	val->val1 = (int32_t)result;            /* integer part of the value */
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

int lis3dhh_sample_fetch(const struct device *dev,
			 enum sensor_channel chan)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	size_t i;
	int status;

	#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		LOG_ERR("Sample fetch failed, device is not in active mode");
		return -ENXIO;
	}
	#endif

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

static const struct sensor_driver_api lis3dhh_driver_api = { .sample_fetch =
								lis3dhh_sample_fetch, .channel_get = lis3dhh_channel_get };

#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
int lis3dhh_pwr_on(const struct device *dev)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	lis3dhh_drv_data->supply_gpios = device_get_binding(DT_INST_GPIO_LABEL(0,
									       supply_gpios));
	if (no_device(lis3dhh_drv_data->supply_gpios)) {
		LOG_ERR("Failed to get pointer to power-supply gpio: %s",
			DT_INST_GPIO_LABEL(0, supply_gpios));
		return -EINVAL;
	}
	status = gpio_pin_configure(lis3dhh_drv_data->supply_gpios, SUPPLY_PIN,
				    GPIO_OUTPUT_ACTIVE | DT_INST_GPIO_FLAGS(0, supply_gpios));
	if (is_error(status)) {
		LOG_ERR("Failed to turn on power supply pin.");
	}
	k_sleep(K_MSEC(10)); /* wait 10ms for device to finish booting */
	return status;
}
#endif

/**
 * @brief Enables normal mode or puts sensor in Power-Down mode. In Power-Down
 *        mode, SPI remains active to allow communication. Configuration registers
 *        are preserved.
 *
 * @param dev Device to address.
 * @param enable Enable normal mode. False for Power-Down mode.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_normal_mode(const struct device *dev,
				  bool enable)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	if (enable) {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1,
					LIS3DHH_CTRL_REG1_NORM_MODE_EN, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to enable normal mode.");
		}
	} else {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1,
					LIS3DHH_CTRL_REG1_NORM_MODE_EN, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to disable normal mode.");
		}
	}
	k_sleep(K_MSEC(10));/* wait 10ms for values to pan out */
	return status;
}

/**
 * @brief Configures auto-increment. Must be enabled for SPI multiple byte access.
 *
 * @param dev Device to address.
 * @param enable True enables auto-increment. False disables it.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_if_add_inc(const struct device *dev,
				 bool enable)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	if (enable) {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1,
					LIS3DHH_CTRL_REG1_IF_ADD_INC, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to enable automatic register increment.");
		}
	} else {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1,
					LIS3DHH_CTRL_REG1_IF_ADD_INC, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to disable automatic register increment.");
		}
	}
	return status;
}

/**
 * @brief Configures block data update (BDU) feature. If reading output data is slow,
 *        this feature makes sure that all read values are from the same sample.
 *
 * @param dev Device to address.
 * @param enable True enables BDU feature. False disables it.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_bdu(const struct device *dev,
			  bool enable)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	if (enable) {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1, LIS3DHH_CTRL_REG1_BDU, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to enable block data update.");
		}
	} else {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1, LIS3DHH_CTRL_REG1_BDU, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to disable block data update.");
		}
	}
	return status;
}

/**
 * @brief Enables or disables the INT1 pin on the sensor as an external
 *        asynchronous input trigger for the FIFO.
 *
 * @param dev Device to address.
 * @param enable True sets INT1 as external asynchronous input trigger,
 *               false disables it.
 * @return 0 for success, negative errorcode for failure
 */
int lis3dhh_configure_int1_as_ext_async_input_trig(const struct device *dev,
						   bool enable)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	if (enable) {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1, LIS3DHH_INT1_CTRL_EXT, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to enable INT1 as external asynchronous input trigger to FIFO.");
		}
	} else {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG1, LIS3DHH_INT1_CTRL_EXT, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to disable INT1 as external asynchronous input trigger to FIFO.");
		}
	}
	return status;
}

/**
 * @brief Configures the filter that is used by the sensor. It can use
 *        either a linear phase FIR or a nonlinear phase IIR.
 *
 * @param dev Device to address.
 * @param filter Choose the filter the device uses.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_filter(const struct device *dev,
			     enum lis3dhh_filter filter)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	switch (filter) {
	case filter_FIR:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_DSP_LP_TYPE, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to configure FIR filter.");
		}
		break;
	case filter_IIR:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_DSP_LP_TYPE, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to configure IIR filter.");
		}
		break;
	default:
		return -EINVAL;
	}
	return status;
}

/**
 * @brief Choose the bandwidth the device's filter runs at.
 *
 * @param dev Device to address.
 * @param bandwidth Can be either 440Hz or 235Hz.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_bandwidth(const struct device *dev,
				enum lis3dhh_bandwidth bandwidth)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	switch (bandwidth) {
	case bandwidth_440:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_DSP_BW_SEL, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to set bandwidth to 440 Hz.");
			return status;
		}
		break;
	case bandwidth_235:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_DSP_BW_SEL, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to set bandwidth to 235 Hz.");
			return status;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Configure INT1 pin to run either in open-drain or
 *        in push-pull mode.
 *
 * @param dev Device to address.
 * @param pp_od Choose mode.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_pp_od_int1(const struct device *dev,
				 enum lis3dhh_pp_od pp_od)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	switch (pp_od) {
	case open_drain:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_PP_OD_INT1, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to select open drain on INT1.");
		}
		break;
	case push_pull:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_PP_OD_INT1, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to select push/pull on INT1.");
		}
		break;
	default:
		return -EINVAL;
	}
	return status;
}

/**
 * @brief Configure INT2 pin to run either in open-drain or
 *        in push-pull mode.
 *
 * @param dev Device to address.
 * @param pp_od Choose mode.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_pp_od_int2(const struct device *dev,
				 enum lis3dhh_pp_od pp_od)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	switch (pp_od) {
	case open_drain:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_PP_OD_INT2, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to select open drain on INT2.");
		}
		break;
	case push_pull:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_PP_OD_INT2, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to select push/pull on INT2.");
		}
		break;
	default:
		return -EINVAL;
	}
	return status;
}

/**
 * @brief Enable or disable FIFO functionality.
 *
 * @param dev Device to address.
 * @param enable True enables the FIFO, false disables it.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_fifo(const struct device *dev, bool enable)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	if (enable) {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_FIFO_EN, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to enable FIFO.");
		}
	} else {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG4,
					LIS3DHH_CTRL_REG4_FIFO_EN, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to disable FIFO.");
		}
	}
	return status;
}

/**
 * @brief Configures the FIFO mode. For a detailed explanation on how these
 *        modes work exactly, refer to the manual.
 *
 * @param dev Device to address.
 * @param fifo_mode Choose the FIFO mode.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_fifo_mode(const struct device *dev,
				enum lis3dhh_fifo_mode fifo_mode)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;
	uint8_t FIFO_CTRL_MASK = 0;

	switch (fifo_mode) {
	case FIFO_BYPASS:
		FIFO_CTRL_MASK = LIS3DHH_FIFO_CTRL_FMODE0 |
				 LIS3DHH_FIFO_CTRL_FMODE1 | LIS3DHH_FIFO_CTRL_FMODE2;
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_FIFO_CTRL, FIFO_CTRL_MASK, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to set FIFO to bypass mode.");
		}
		break;
	case FIFO_NORMAL:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_FIFO_CTRL,
					LIS3DHH_FIFO_CTRL_FMODE0, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to set FIFO to normal mode.");
		}
		break;
	case FIFO_CONTINUOUS_TO_FIFO:
		FIFO_CTRL_MASK = LIS3DHH_FIFO_CTRL_FMODE0 | LIS3DHH_FIFO_CTRL_FMODE1;
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_FIFO_CTRL, FIFO_CTRL_MASK, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to set continuous to normal FIFO mode.");
		}
		break;
	case FIFO_BYPASS_TO_CONTINUOUS:
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_FIFO_CTRL,
					LIS3DHH_FIFO_CTRL_FMODE2, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to set bypass to continuous mode on FIFO.");
		}
		break;
	case FIFO_CONTINUOUS:
		FIFO_CTRL_MASK = LIS3DHH_FIFO_CTRL_FMODE1 | LIS3DHH_FIFO_CTRL_FMODE2;
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_FIFO_CTRL, FIFO_CTRL_MASK, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to set FIFO to continuous mode.");
		}
		break;
	default:
		return -EINVAL;
	}
	return status;
}

/**
 * @brief Set the threshold at which the FIFO triggers its threshold flag. This
 *        flag can be routed to INT1 and INT2 to provide an interrupt. For further
 *        instructions on this, refer to the manual.
 *
 * @param dev Device to address.
 * @param threshold Number must be between 1 and 32.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_fifo_threshold(const struct device *dev,
				     uint8_t threshold)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;
	uint8_t FIFO_CTRL_MASK = 0;
	uint8_t initial_threshold = threshold;

	if (threshold > 32 || !threshold) {
		LOG_ERR("Invalid threshold. Max is 32. 0 is not allowed.");
		return -EINVAL;
	} else if (threshold >= 16) {
		FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH4;
		threshold -= 16;
	}
	if (threshold >= 8) {
		FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH3;
		threshold -= 8;
	}
	if (threshold >= 4) {
		FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH2;
		threshold -= 4;
	}
	if (threshold >= 2) {
		FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH1;
		threshold -= 2;
	}
	if (threshold >= 1) {
		FIFO_CTRL_MASK |= LIS3DHH_FIFO_CTRL_FTH0;
	}

	if (initial_threshold == 32) {
		FIFO_CTRL_MASK = 0; /* equivalent to 32 */
	}

	status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_FIFO_CTRL, FIFO_CTRL_MASK, 0);

	if (is_error(status)) {
		LOG_ERR("Failed to configure FIFO threshold.");
	} else {
		LOG_INF("FIFO threshold has been set to %i", initial_threshold);
	}

	return status;
}

/**
 * @brief Configures the SPI high speed mode for the FIFO block. This feature should
 *        be enabled for SPI speeds of greater than 6 MHz.
 *
 * @param dev Device to address.
 * @param enable True enables FISO SPI high speed mode, false disables it.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_configure_fifo_spi_high_speed(const struct device *dev,
					  bool enable)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	int status = 0;

	if (enable) {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG5,
				LIS3DHH_CTRL_REG5_FIFO_SPI_HS_ON, 0xFF);
		if (is_error(status)) {
			LOG_ERR("Failed to enable SPI high speed configuration.");
		}
	} else {
		status = lis3dhh_drv_data->hw_tf->update_reg(dev, LIS3DHH_CTRL_REG5,
				LIS3DHH_CTRL_REG5_FIFO_SPI_HS_ON, 0);
		if (is_error(status)) {
			LOG_ERR("Failed to disable SPI high speed configuration.");
		}
	}
	return status;
}

/**
 * @brief Configures the device with the initial configuration as
 *        defined by the Kconfig.
 *
 * @param dev Device to address.
 * @return 0 for success, negative errorcode for failure.
 */
int lis3dhh_initial_configuration(const struct device *dev)
{
	int status = 0;

	while (!status) {
		/* CTRL_REG1 configuration */
#if defined(CONFIG_LIS3DHH_NORMAL_MODE)
		status = lis3dhh_configure_normal_mode(dev, true);
#else
		status = lis3dhh_configure_normal_mode(dev, false);
#endif
#if defined(CONFIG_LIS3DHH_IF_ADD_INC)
		status = lis3dhh_configure_if_add_inc(dev, true);
#else
		status = lis3dhh_configure_if_add_inc(dev, false);
#endif
#if defined(CONFIG_LIS3DHH_ENABLE_BDU)
		status = lis3dhh_configure_bdu(dev, true);
#else
		status = lis3dhh_configure_bdu(dev, false);
#endif

		/* INT1_CTRL configuration */
#if defined(CONFIG_LIS3DHH_INT1_AS_EXT_ASYNC_INPUT_TRIG)
		status = lis3dhh_configure_int1_as_ext_async_input_trig(dev, true);
#else
		status = lis3dhh_configure_int1_as_ext_async_input_trig(dev, false);
#endif

		/* CTRL_REG4 configuration */
#if defined(CONFIG_LIS3DHH_FILTER_FIR)
		status = lis3dhh_configure_filter(dev, filter_FIR);
#elif defined(CONFIG_LIS3DHH_FILTER_IIR)
		status = lis3dhh_configure_filter(dev, filter_IIR);
#endif
#if defined(CONFIG_LIS3DHH_BANDWIDTH_440HZ)
		status = lis3dhh_configure_bandwidth(dev, bandwidth_440);
#elif defined(CONFIG_LIS3DHH_BANDWIDTH_235HZ)
		status = lis3dhh_configure_bandwidth(dev, bandwidth_235);
#endif
#if defined(CONFIG_LIS3DHH_INT1_PUSH_PULL)
		status = lis3dhh_configure_pp_od_int1(dev, push_pull);
#elif defined(CONFIG_LIS3DHH_INT1_OPEN_DRAIN)
		status = lis3dhh_configure_pp_od_int1(dev, open_drain);
#endif
#if defined(CONFIG_LIS3DHH_INT2_PUSH_PULL)
		status = lis3dhh_configure_pp_od_int2(dev, push_pull);
#elif defined(CONFIG_LIS3DHH_INT2_OPEN_DRAIN)
		status = lis3dhh_configure_pp_od_int2(dev, open_drain);
#endif
#if defined(CONFIG_LIS3DHH_ENABLE_FIFO)
		status = lis3dhh_configure_fifo(dev, true);
#else
		status = lis3dhh_configure_fifo(dev, false);
#endif

		/* CTRL_REG_5 configuration */
#if defined(CONFIG_LIS3DHH_SPI_HS_CONFIG)
		status = lis3dhh_configure_fifo_spi_high_speed(dev, true);
#else
		status = lis3dhh_configure_fifo_spi_high_speed(dev, false);
#endif

		/* FIFO configuration */
#if defined(CONFIG_LIS3DHH_FIFO_BYPASS)
		status = lis3dhh_configure_fifo_mode(dev, FIFO_BYPASS);
#elif defined(CONFIG_LIS3DHH_FIFO_NORMAL)
		status = lis3dhh_configure_fifo_mode(dev, FIFO_NORMAL);
#elif defined(CONFIG_LIS3DHH_FIFO_CONTINUOUS_TO_FIFO)
		status = lis3dhh_configure_fifo_mode(dev, FIFO_CONTINUOUS_TO_FIFO);
#elif defined(CONFIG_LIS3DHH_FIFO_BYPASS_TO_CONTINUOUS)
		status = lis3dhh_configure_fifo_mode(dev, FIFO_BYPASS_TO_CONTINUOUS);
#elif defined(CONFIG_LIS3DHH_FIFO_CONTINUOUS)
		status = lis3dhh_configure_fifo_mode(dev, FIFO_CONTINUOUS);
#endif
#if defined(LIS3DHH_FIFO_THRESHOLD)
		status = lis3dhh_configure_fifo_threshold(dev, LIS3DHH_FIFO_THRESHOLD);
#endif
		break;
	}

	return status;
}

int lis3dhh_init(const struct device *dev)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	const struct lis3dhh_config *cfg = dev->config;
	int status;
	uint8_t id;

#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
	lis3dhh_pwr_on(dev);
#endif

	lis3dhh_drv_data->bus = device_get_binding(cfg->bus_name);

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

	lis3dhh_initial_configuration(dev);

	return status;
}

#ifdef CONFIG_PM_DEVICE
static int lis3dhh_pm_control(const struct device *dev,
			      enum pm_device_action action)
{
#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
#endif
	int status = 0;
	enum pm_device_state current_pm_state;
	(void)pm_device_state_get(dev, &current_pm_state);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LOG_INF("just entered resume case");
		/* configure device to resume or start operations */
		if (current_pm_state == PM_DEVICE_STATE_SUSPENDED) {
			LOG_INF("currently in suspended state");
			status = lis3dhh_configure_normal_mode(dev, true);
			if (status) {
				LOG_ERR("Resume failed. Errorcode: %i", status);
				break;
			}
			LOG_DBG("LIS3DHH resumed operations. No config changes made.");
			break;
		}
#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
		else if (current_pm_state == PM_DEVICE_STATE_OFF) {
			LOG_INF("currently in off state");
			lis3dhh_pwr_on(dev);
			status = lis3dhh_initial_configuration(dev);
			if (is_error(status)) {
				LOG_ERR("Power up failed. Errorcode: %i", status);
				break;
			}
			LOG_DBG("LIS3DHH state changed to active and initialized with startup config.");
			break;
		}
#endif
		else {
			LOG_ERR("LIS3DHH not in a state where resume is supported.");
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		LOG_INF("just entered suspend case");
		/* configure device to stop operations */
		status = lis3dhh_configure_normal_mode(dev, false);
		if (status) {
			LOG_ERR("Suspend failed. Errorcode: %i", status);
			break;
		}
		LOG_DBG("LIS3DHH has been to suspended state.");
		break;

#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
	case PM_DEVICE_ACTION_TURN_OFF:
		LOG_INF("just entered turn off case");
		lis3dhh_drv_data->supply_gpios = device_get_binding(
			DT_INST_GPIO_LABEL(0, supply_gpios));
		if (no_device(lis3dhh_drv_data->supply_gpios)) {
			LOG_ERR("Failed to get pointer to power-supply gpio: %s", DT_INST_GPIO_LABEL(0, supply_gpios));
			return -EINVAL;
		}
		status = gpio_pin_configure(lis3dhh_drv_data->supply_gpios,
					    SUPPLY_PIN, GPIO_OUTPUT_INACTIVE | DT_INST_GPIO_FLAGS(0,
												  supply_gpios));
		if (status) {
			LOG_ERR("Power down failed. Errorcode: %i", status);
			break;
		}
		LOG_DBG("LIS3DHH is now turned off.");
		break;
#endif
	default:
		LOG_ERR("Action not supported on device.");
		return -ENOTSUP;
	}
	return status;
}
#endif /* CONFIG_PM_DEVICE */

#define LIS3DHH_DEVICE_INIT(inst)			   \
	DEVICE_DT_INST_DEFINE(inst,			   \
			      lis3dhh_init,		   \
			      lis3dhh_pm_control,	   \
			      &lis3dhh_data_##inst,	   \
			      &lis3dhh_config_##inst,	   \
			      POST_KERNEL,		   \
			      CONFIG_SENSOR_INIT_PRIORITY, \
			      &lis3dhh_driver_api);

#define LIS3DHH_HAS_CS(inst) DT_INST_SPI_DEV_HAS_CS_GPIOS(inst)

#define LIS3DHH_DATA_SPI_CS(inst)				      \
	{							      \
		.cs_ctrl = {					      \
			.gpio_pin =				      \
				DT_INST_SPI_DEV_CS_GPIOS_PIN(inst),   \
			.gpio_dt_flags =			      \
				DT_INST_SPI_DEV_CS_GPIOS_FLAGS(inst), \
		},						      \
	}

#define LIS3DHH_DATA_SPI(inst) COND_CODE_1(LIS3DHH_HAS_CS(inst), \
					   (LIS3DHH_DATA_SPI_CS(inst)), ({}))

#define LIS3DHH_SPI_CS_PTR(inst) COND_CODE_1(LIS3DHH_HAS_CS(inst), \
					     (&(lis3dhh_data_##inst.cs_ctrl)), (NULL))

#define LIS3DHH_SPI_CS_LABEL(inst) COND_CODE_1(LIS3DHH_HAS_CS(inst), \
					       (DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst)), (NULL))

#define LIS3DHH_SPI_CFG(inst)					       \
	(&(struct lis3dhh_spi_cfg){				       \
		.spi_conf = {					       \
			.frequency =				       \
				DT_INST_PROP(inst, spi_max_frequency), \
			.operation = (SPI_WORD_SET(8) |		       \
				      SPI_OP_MODE_MASTER |	       \
				      SPI_TRANSFER_MSB),	       \
			.slave = DT_INST_REG_ADDR(inst),	       \
			.cs = LIS3DHH_SPI_CS_PTR(inst),		       \
		},						       \
		.cs_gpios_label = LIS3DHH_SPI_CS_LABEL(inst),	       \
	})

#define LIS3DHH_CFG_INT(inst)

#define LIS3DHH_CONFIG_SPI(inst)				 \
	{							 \
		.bus_name = DT_INST_BUS_LABEL(inst),		 \
		.bus_init = lis3dhh_spi_init,			 \
		.bus_cfg = { .spi_cfg = LIS3DHH_SPI_CFG(inst) }, \
		LIS3DHH_CFG_INT(inst)				 \
	}

#define LIS3DHH_DEFINE_SPI(inst)				   \
	static struct lis3dhh_data lis3dhh_data_##inst =	   \
		LIS3DHH_DATA_SPI(inst);				   \
	static const struct lis3dhh_config lis3dhh_config_##inst = \
		LIS3DHH_CONFIG_SPI(inst);			   \
	LIS3DHH_DEVICE_INIT(inst)

#define LIS3DHH_DEFINE(inst) LIS3DHH_DEFINE_SPI(inst)

DT_INST_FOREACH_STATUS_OKAY(LIS3DHH_DEFINE)
