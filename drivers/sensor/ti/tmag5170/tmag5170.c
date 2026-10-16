/*
 * Copyright (c) 2023 Michal Morsisko
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmag5170

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include "tmag5170.h"
#include "tmag5170_bus.h"

#if defined(CONFIG_SENSOR_ASYNC_API)
#include "tmag5170_decoder.h"
#endif

#if defined(CONFIG_TMAG5170_STREAM)
#include "tmag5170_stream.h"
#endif

#if defined(CONFIG_SENSOR_ASYNC_API)
const uint8_t tmag5170_result_regs[TMAG5170_RESULT_IDX_COUNT] = {
	[TMAG5170_RESULT_IDX_X] = TMAG5170_REG_X_CH_RESULT,
	[TMAG5170_RESULT_IDX_Y] = TMAG5170_REG_Y_CH_RESULT,
	[TMAG5170_RESULT_IDX_Z] = TMAG5170_REG_Z_CH_RESULT,
	[TMAG5170_RESULT_IDX_ANGLE] = TMAG5170_REG_ANGLE_RESULT,
	[TMAG5170_RESULT_IDX_TEMP] = TMAG5170_REG_TEMP_RESULT,
};
#endif

/** Number of submission and completion queue entries per instance.
 *
 * The longest sequence issued by the driver is a trigger frame, a delay and the
 * five result frames, followed by the completion callback.
 */
#define TMAG5170_RTIO_QUEUE_LEN 12

LOG_MODULE_REGISTER(TMAG5170, CONFIG_SENSOR_LOG_LEVEL);

static int tmag5170_write_register(const struct device *dev, uint8_t reg, uint16_t data)
{
	struct tmag5170_data *drv_data = dev->data;

	return tmag5170_write_register_rtio(&drv_data->bus, reg, data);
}

static int tmag5170_read_register(const struct device *dev, uint8_t reg, uint16_t *output,
				  uint8_t cmd)
{
	struct tmag5170_data *drv_data = dev->data;

	return tmag5170_read_register_rtio(&drv_data->bus, reg, cmd, output);
}

static int tmag5170_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	int ret = 0;

	if (cfg->operating_mode == TMAG5170_STAND_BY_MODE ||
	    cfg->operating_mode == TMAG5170_ACTIVE_TRIGGER_MODE) {
		uint16_t read_status;

		tmag5170_read_register(dev, TMAG5170_REG_SYS_STATUS, &read_status,
				       TMAG5170_CMD_TRIGGER_CONVERSION);

		/* Wait for the measurement to be ready.
		 * The waiting time will vary depending on the configuration
		 */
		k_sleep(K_MSEC(TMAG5170_CONVERSION_TIME_MS));
	}

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		ret = tmag5170_read_register(dev, TMAG5170_REG_X_CH_RESULT, &drv_data->x, 0U);
		break;
	case SENSOR_CHAN_MAGN_Y:
		ret = tmag5170_read_register(dev, TMAG5170_REG_Y_CH_RESULT, &drv_data->y, 0U);
		break;
	case SENSOR_CHAN_MAGN_Z:
		ret = tmag5170_read_register(dev, TMAG5170_REG_Z_CH_RESULT, &drv_data->z, 0U);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		ret = tmag5170_read_register(dev, TMAG5170_REG_X_CH_RESULT, &drv_data->x, 0U);

		if (ret == 0) {
			ret = tmag5170_read_register(dev, TMAG5170_REG_Y_CH_RESULT, &drv_data->y,
						     0U);
		}
		if (ret == 0) {
			ret = tmag5170_read_register(dev, TMAG5170_REG_Z_CH_RESULT, &drv_data->z,
						     0U);
		}
		break;
	case SENSOR_CHAN_ROTATION:
		ret = tmag5170_read_register(dev, TMAG5170_REG_ANGLE_RESULT, &drv_data->angle, 0U);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		ret = tmag5170_read_register(dev, TMAG5170_REG_TEMP_RESULT, &drv_data->temperature,
					     0U);
		break;
	case SENSOR_CHAN_ALL:
		ret = tmag5170_read_register(dev, TMAG5170_REG_TEMP_RESULT, &drv_data->temperature,
					     0U);

		if (ret == 0) {
			ret = tmag5170_read_register(dev, TMAG5170_REG_ANGLE_RESULT,
						     &drv_data->angle, 0U);
		}

		if (ret == 0) {
			ret = tmag5170_read_register(dev, TMAG5170_REG_X_CH_RESULT, &drv_data->x,
						     0U);
		}

		if (ret == 0) {
			ret = tmag5170_read_register(dev, TMAG5170_REG_Y_CH_RESULT, &drv_data->y,
						     0U);
		}

		if (ret == 0) {
			ret = tmag5170_read_register(dev, TMAG5170_REG_Z_CH_RESULT, &drv_data->z,
						     0U);
		}

		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int tmag5170_convert_magn_reading_to_gauss(struct sensor_value *output,
						  uint16_t chan_reading, uint8_t chan_range,
						  uint8_t chip_revision)
{
	int64_t micro;
	int ret = tmag5170_magn_reading_to_micro_gauss(chan_reading, chan_range, chip_revision,
						       &micro);

	if (ret != 0) {
		return ret;
	}

	return sensor_value_from_micro(output, micro);
}

static int tmag5170_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	int64_t micro;
	int ret = 0;

	switch (chan) {
	case SENSOR_CHAN_MAGN_XYZ:
		ret = tmag5170_convert_magn_reading_to_gauss(val, drv_data->x, cfg->x_range,
							     drv_data->chip_revision);

		if (ret == 0) {
			ret = tmag5170_convert_magn_reading_to_gauss(
				val + 1, drv_data->y, cfg->y_range, drv_data->chip_revision);
		}

		if (ret == 0) {
			ret = tmag5170_convert_magn_reading_to_gauss(
				val + 2, drv_data->z, cfg->z_range, drv_data->chip_revision);
		}
		break;
	case SENSOR_CHAN_MAGN_X:
		ret = tmag5170_convert_magn_reading_to_gauss(val, drv_data->x, cfg->x_range,
							     drv_data->chip_revision);
		break;
	case SENSOR_CHAN_MAGN_Y:
		ret = tmag5170_convert_magn_reading_to_gauss(val, drv_data->y, cfg->y_range,
							     drv_data->chip_revision);
		break;
	case SENSOR_CHAN_MAGN_Z:
		ret = tmag5170_convert_magn_reading_to_gauss(val, drv_data->z, cfg->z_range,
							     drv_data->chip_revision);
		break;
	case SENSOR_CHAN_ROTATION:
		tmag5170_angle_reading_to_micro_degrees(drv_data->angle, &micro);
		(void)sensor_value_from_micro(val, micro);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		tmag5170_temp_reading_to_micro_celsius(drv_data->temperature, &micro);
		(void)sensor_value_from_micro(val, micro);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

#if defined(CONFIG_SENSOR_ASYNC_API)

static void tmag5170_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				     void *arg)
{
	ARG_UNUSED(arg);

	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	struct tmag5170_encoded_data *edata;
	struct rtio_cqe *cqe;
	uint8_t *buf;
	uint32_t buf_len;
	int err = (result < 0) ? result : 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			if (cqe->result < 0 && err == 0) {
				err = cqe->result;
			}
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err == 0) {
		/* The buffer has already been allocated by the submission, so
		 * this only hands back the very same buffer.
		 */
		err = rtio_sqe_rx_buf(iodev_sqe, 0, 0, &buf, &buf_len);
	}

	if (err == 0) {
		edata = (struct tmag5170_encoded_data *)buf;

		/* Verify the CRC of every frame which has been shifted in. This
		 * cannot be done by the bus layer anymore, because the frames
		 * are transferred asynchronously.
		 */
		for (uint8_t idx = 0U; idx < TMAG5170_RESULT_IDX_COUNT; idx++) {
			if ((edata->header.channels & BIT(idx)) == 0U) {
				continue;
			}

			err = tmag5170_frame_decode(edata->payload.frames[idx], NULL);
			if (err != 0) {
				LOG_ERR("CRC mismatch of result register %u", idx);
				break;
			}
		}
	}

	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}

	LOG_DBG("One-shot fetch completed");
}

static void tmag5170_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	const uint32_t min_buf_len = sizeof(struct tmag5170_encoded_data);
	struct tmag5170_encoded_data *edata;
	struct rtio_sqe *sqe = NULL;
	uint8_t *buf;
	uint32_t buf_len;
	int err;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	edata = (struct tmag5170_encoded_data *)buf;

	err = tmag5170_encode(dev, read_cfg->channels, read_cfg->count, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	if (cfg->operating_mode == TMAG5170_STAND_BY_MODE ||
	    cfg->operating_mode == TMAG5170_ACTIVE_TRIGGER_MODE) {
#if defined(CONFIG_RTIO_OP_DELAY)
		/* In the trigger driven operating modes a conversion has to be
		 * started explicitly and its result only becomes available once
		 * the conversion finished. The blocking k_sleep() of the
		 * fetch/get API is replaced by a chained RTIO delay operation:
		 * it does not hold the bus, hence transfers of other devices
		 * can be served meanwhile and neither the submit nor the
		 * completion path blocks.
		 */
		tmag5170_frame_encode_read(drv_data->tx_trigger_frame, TMAG5170_REG_SYS_STATUS,
					   TMAG5170_CMD_TRIGGER_CONVERSION);

		err = tmag5170_prep_frame_rtio_async(&drv_data->bus, drv_data->tx_trigger_frame,
						     drv_data->rx_trigger_frame, &sqe);
		if (err < 0) {
			goto err_sqe;
		}
		sqe->flags |= RTIO_SQE_CHAINED;

		sqe = rtio_sqe_acquire(drv_data->bus.rtio.ctx);
		if (!sqe) {
			goto err_sqe;
		}
		rtio_sqe_prep_delay(sqe, K_MSEC(TMAG5170_CONVERSION_TIME_MS), NULL);
		sqe->flags |= RTIO_SQE_CHAINED;
#else
		/* Without the RTIO delay operation the conversion time cannot
		 * be bridged without blocking, which is not allowed here.
		 */
		LOG_ERR("Trigger driven operating modes require CONFIG_RTIO_OP_DELAY");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
#endif /* CONFIG_RTIO_OP_DELAY */
	}

	for (uint8_t idx = 0U; idx < TMAG5170_RESULT_IDX_COUNT; idx++) {
		if ((edata->header.channels & BIT(idx)) == 0U) {
			continue;
		}

		tmag5170_frame_encode_read(drv_data->tx_frames[idx], tmag5170_result_regs[idx], 0U);

		err = tmag5170_prep_frame_rtio_async(&drv_data->bus, drv_data->tx_frames[idx],
						     edata->payload.frames[idx], &sqe);
		if (err < 0) {
			goto err_sqe;
		}
		sqe->flags |= RTIO_SQE_CHAINED;
	}

	sqe = rtio_sqe_acquire(drv_data->bus.rtio.ctx);
	if (!sqe) {
		goto err_sqe;
	}
	rtio_sqe_prep_callback_no_cqe(sqe, tmag5170_complete_result, (void *)dev, iodev_sqe);

	rtio_submit(drv_data->bus.rtio.ctx, 0);

	return;

err_sqe:
	LOG_ERR("Failed to acquire SQE");
	rtio_sqe_drop_all(drv_data->bus.rtio.ctx);
	rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
}

static void tmag5170_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;

	if (!read_cfg->is_streaming) {
		tmag5170_submit_one_shot(dev, iodev_sqe);
		return;
	}

#if defined(CONFIG_TMAG5170_STREAM)
	tmag5170_stream_submit(dev, iodev_sqe);
#else
	LOG_ERR("Streaming not supported");
	rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
#endif
}

#endif /* CONFIG_SENSOR_ASYNC_API */

static int tmag5170_init_registers(const struct device *dev)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	uint16_t test_cfg_reg = 0U;
	int ret = 0;

#if !defined(CONFIG_TMAG5170_CRC)
	const uint8_t disable_crc_packet[TMAG5170_SPI_BUFFER_LEN] = {0x0FU, 0x0U, 0x04U, 0x07U};

	ret = tmag5170_transmit_frame_rtio(&drv_data->bus, disable_crc_packet);
#endif
	if (ret == 0) {
		ret = tmag5170_read_register(dev, TMAG5170_REG_TEST_CONFIG, &test_cfg_reg, 0U);
	}

	if (ret == 0) {
		drv_data->chip_revision = TMAG5170_VER_GET(test_cfg_reg);

		ret = tmag5170_write_register(
			dev, TMAG5170_REG_SENSOR_CONFIG,
			TMAG5170_ANGLE_EN_SET(cfg->angle_measurement) |
				TMAG5170_SLEEPTIME_SET(cfg->sleep_time) |
				TMAG5170_MAG_CH_EN_SET(cfg->magnetic_channels) |
				TMAG5170_Z_RANGE_SET(cfg->z_range) |
				TMAG5170_Y_RANGE_SET(cfg->y_range) |
				TMAG5170_X_RANGE_SET(cfg->x_range));
	}

#if defined(CONFIG_TMAG5170_TRIGGER) || defined(CONFIG_TMAG5170_STREAM)
	if (ret == 0) {
		ret = tmag5170_write_register(dev, TMAG5170_REG_ALERT_CONFIG,
					      TMAG5170_RSLT_ALRT_SET(1U));
	}
#endif
	if (ret == 0) {
		ret = tmag5170_write_register(
			dev, TMAG5170_REG_DEVICE_CONFIG,
			TMAG5170_OPERATING_MODE_SET(cfg->operating_mode) |
				TMAG5170_CONV_AVG_SET(cfg->oversampling) |
				TMAG5170_MAG_TEMPCO_SET(cfg->magnet_type) |
				TMAG5170_T_CH_EN_SET(cfg->temperature_measurement) |
				TMAG5170_T_RATE_SET(cfg->disable_temperature_oversampling));
	}

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int tmag5170_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret_val = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		tmag5170_write_register(dev, TMAG5170_REG_DEVICE_CONFIG,
					TMAG5170_OPERATING_MODE_SET(TMAG5170_CONFIGURATION_MODE));
		/* As per datasheet, waking up from deep-sleep can take up to 500us */
		k_sleep(K_USEC(500));
		ret_val = tmag5170_init_registers(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret_val = tmag5170_write_register(
			dev, TMAG5170_REG_DEVICE_CONFIG,
			TMAG5170_OPERATING_MODE_SET(TMAG5170_DEEP_SLEEP_MODE));
		break;
	default:
		ret_val = -ENOTSUP;
	}

	return ret_val;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, tmag5170_driver_api) = {
	.sample_fetch = tmag5170_sample_fetch,
	.channel_get = tmag5170_channel_get,
#if defined(CONFIG_TMAG5170_TRIGGER)
	.trigger_set = tmag5170_trigger_set,
#endif
#if defined(CONFIG_SENSOR_ASYNC_API)
	.submit = tmag5170_submit,
	.get_decoder = tmag5170_get_decoder,
#endif
};

static int tmag5170_init(const struct device *dev)
{
	struct tmag5170_data *drv_data = dev->data;
	int ret = 0;

#if defined(CONFIG_TMAG5170_TRIGGER)
	const struct tmag5170_dev_config *cfg = dev->config;
#endif

	if (!spi_is_ready_iodev(drv_data->bus.rtio.iodev)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	ret = tmag5170_init_registers(dev);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_TMAG5170_STREAM)
	ret = tmag5170_stream_init(dev);
#elif defined(CONFIG_TMAG5170_TRIGGER)
	if (cfg->int_gpio.port) {
		ret = tmag5170_trigger_init(dev);
	}
#endif

	return ret;
}

#if defined(CONFIG_TMAG5170_TRIGGER) || defined(CONFIG_TMAG5170_STREAM)
#define TMAG5170_INT_GPIO_INIT(_num) .int_gpio = GPIO_DT_SPEC_INST_GET_OR(_num, int_gpios, {0}),
#else
#define TMAG5170_INT_GPIO_INIT(_num)
#endif

#define DEFINE_TMAG5170(_num)                                                                      \
	RTIO_DEFINE(tmag5170_rtio_ctx_##_num, TMAG5170_RTIO_QUEUE_LEN, TMAG5170_RTIO_QUEUE_LEN);   \
	SPI_DT_IODEV_DEFINE(tmag5170_bus_##_num, DT_DRV_INST(_num),                                \
			    SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB | SPI_WORD_SET(8));          \
	static struct tmag5170_data tmag5170_data_##_num = {                                       \
		.bus.rtio =                                                                        \
			{                                                                          \
				.ctx = &tmag5170_rtio_ctx_##_num,                                  \
				.iodev = &tmag5170_bus_##_num,                                     \
			},                                                                         \
	};                                                                                         \
	static const struct tmag5170_dev_config tmag5170_config_##_num = {                         \
		.magnetic_channels = DT_INST_ENUM_IDX(_num, magnetic_channels),                    \
		.x_range = DT_INST_ENUM_IDX(_num, x_range),                                        \
		.y_range = DT_INST_ENUM_IDX(_num, y_range),                                        \
		.z_range = DT_INST_ENUM_IDX(_num, z_range),                                        \
		.operating_mode = DT_INST_PROP(_num, operating_mode),                              \
		.oversampling = DT_INST_ENUM_IDX(_num, oversampling),                              \
		.temperature_measurement = DT_INST_PROP(_num, enable_temperature_channel),         \
		.magnet_type = DT_INST_ENUM_IDX(_num, magnet_type),                                \
		.angle_measurement = DT_INST_ENUM_IDX(_num, angle_measurement),                    \
		.disable_temperature_oversampling =                                                \
			DT_INST_PROP(_num, disable_temperature_oversampling),                      \
		.sleep_time = DT_INST_ENUM_IDX(_num, sleep_time),                                  \
		TMAG5170_INT_GPIO_INIT(_num)};                                                     \
	PM_DEVICE_DT_INST_DEFINE(_num, tmag5170_pm_action);                                        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, tmag5170_init, PM_DEVICE_DT_INST_GET(_num),             \
				     &tmag5170_data_##_num, &tmag5170_config_##_num, POST_KERNEL,  \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmag5170_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMAG5170)
