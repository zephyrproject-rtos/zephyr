/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bhi2xy.h"
#include "bhi2xy_parsers.h"
#include "bhi2xy_unit_conversions.h"
#include "bhi2xy_errors.h"

#include "bhy2_defs.h"
#include "zephyr/drivers/sensor.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bhi2xy, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT bosch_bhi2xy

#define BHY2_RD_WR_LEN 256

/* Bosch API requires these functions to be defined */
static int bhi2xy_bus_check(const struct device *dev);
static BHY2_INTF_RET_TYPE bhy2_reg_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length,
					 void *intf_ptr);
static BHY2_INTF_RET_TYPE bhy2_reg_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length,
					void *intf_ptr);
static void bhy2_delay_us(uint32_t us, void *intf_ptr);

/* Helper functions for the init process */
static int bhi2xy_init_hardware(const struct device *dev);
static int bhi2xy_init_chip(const struct device *dev);
static int bhi2xy_config_host_interface(struct bhy2_dev *bhy2);
static int bhi2xy_load_firmware_and_verify(const struct device *dev);
static int bhi2xy_register_callbacks(const struct device *dev);
static void bhi2xy_get_sensor_ranges(const struct device *dev);

/* Bosch API to Zephyr error conversion and logging */
static int bhi2xy_api_to_os_error(int8_t ret);
static void bhi2xy_log_api_error(int8_t ret, struct bhy2_dev *bhy2);

/* Sensor firmware */
static const uint8_t bhi2xy_fw_data[] = {
#include "bhi2xy_firmware.inc"
};

static int bhi2xy_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;
	struct bhi2xy_data *data = dev->data;
	struct bhy2_dev *bhy2 = &data->bhy2;
	uint8_t *work_buffer = data->work_buffer;
	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}
	ret = bhy2_get_and_process_fifo(work_buffer, BHI2XY_WORK_BUFFER_SIZE, bhy2);
	bhi2xy_log_api_error(ret, bhy2);
	return bhi2xy_api_to_os_error(ret);
}

static int bhi2xy_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bhi2xy_data *data = dev->data;
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		bhi2xy_accel_to_ms2(val, data->acc[0], data->acc_range);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		bhi2xy_accel_to_ms2(val, data->acc[1], data->acc_range);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		bhi2xy_accel_to_ms2(val, data->acc[2], data->acc_range);
		break;
	case SENSOR_CHAN_GYRO_X:
		bhi2xy_gyro_to_rads(val, data->gyro[0], data->gyro_range);
		break;
	case SENSOR_CHAN_GYRO_Y:
		bhi2xy_gyro_to_rads(val, data->gyro[1], data->gyro_range);
		break;
	case SENSOR_CHAN_GYRO_Z:
		bhi2xy_gyro_to_rads(val, data->gyro[2], data->gyro_range);
		break;
	case SENSOR_CHAN_MAGN_X:
		bhi2xy_mag_to_gauss(val, data->mag[0]);
		break;
	case SENSOR_CHAN_MAGN_Y:
		bhi2xy_mag_to_gauss(val, data->mag[1]);
		break;
	case SENSOR_CHAN_MAGN_Z:
		bhi2xy_mag_to_gauss(val, data->mag[2]);
		break;
	case SENSOR_CHAN_PRESS:
		bhi2xy_pres_to_kpa(val, data->pres);
		break;
	case SENSOR_CHAN_EULER_HEADING:
		bhi2xy_ori_to_deg(val, data->euler[0]);
		break;
	case SENSOR_CHAN_EULER_ROLL:
		bhi2xy_ori_to_deg(val, data->euler[1]);
		break;
	case SENSOR_CHAN_EULER_PITCH:
		bhi2xy_ori_to_deg(val, data->euler[2]);
		break;
	case SENSOR_CHAN_STEP_COUNT:
		/* step count is uint32_t, val members are int32_t, currently only val1 is used */
		val->val1 = data->step_count;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_ROTATION_VECTOR_X:
		bhi2xy_rv_grv_to_quat(val, data->rv[0]);
		break;
	case SENSOR_CHAN_ROTATION_VECTOR_Y:
		bhi2xy_rv_grv_to_quat(val, data->rv[1]);
		break;
	case SENSOR_CHAN_ROTATION_VECTOR_Z:
		bhi2xy_rv_grv_to_quat(val, data->rv[2]);
		break;
	case SENSOR_CHAN_ROTATION_VECTOR_W:
		bhi2xy_rv_grv_to_quat(val, data->rv[3]);
		break;
	case SENSOR_CHAN_GAME_ROTATION_VECTOR_X:
		bhi2xy_rv_grv_to_quat(val, data->grv[0]);
		break;
	case SENSOR_CHAN_GAME_ROTATION_VECTOR_Y:
		bhi2xy_rv_grv_to_quat(val, data->grv[1]);
		break;
	case SENSOR_CHAN_GAME_ROTATION_VECTOR_Z:
		bhi2xy_rv_grv_to_quat(val, data->grv[2]);
		break;
	case SENSOR_CHAN_GAME_ROTATION_VECTOR_W:
		bhi2xy_rv_grv_to_quat(val, data->grv[3]);
		break;
	default:
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}
	return 0;
}

static int bhi2xy_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret = 0;
	uint8_t sensor_id = 0;
	float sample_rate = 0.0f;
	const uint32_t sensor_report_latency = 0;
	struct bhi2xy_data *data = dev->data;
	struct bhy2_dev *bhy2 = &data->bhy2;
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_XYZ:
			sensor_id = BHY2_SENSOR_ID_ACC;
			break;
		case SENSOR_CHAN_GYRO_XYZ:
			sensor_id = BHY2_SENSOR_ID_GYRO;
			break;
		case SENSOR_CHAN_MAGN_XYZ:
			sensor_id = BHY2_SENSOR_ID_MAG;
			break;
		case SENSOR_CHAN_EULER:
			sensor_id = BHY2_SENSOR_ID_ORI;
			break;
		case SENSOR_CHAN_ROTATION_VECTOR:
			sensor_id = BHY2_SENSOR_ID_RV;
			break;
		case SENSOR_CHAN_GAME_ROTATION_VECTOR:
			sensor_id = BHY2_SENSOR_ID_GAMERV;
			break;
		case SENSOR_CHAN_STEP_COUNT:
			sensor_id = BHY2_SENSOR_ID_STC;
			break;
		case SENSOR_CHAN_PRESS:
			sensor_id = BHY2_SENSOR_ID_BARO;
			break;
		default:
			LOG_ERR("Unsupported channel %d for attribute %d", chan, attr);
			return -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Unsupported attribute %d for channel %d", attr, chan);
		return -ENOTSUP;
	}
	LOG_DBG("Setting sampling frequency for sensor with ID %d to %d.%06d Hz", sensor_id,
		val->val1, val->val2);
	sample_rate = sensor_value_to_float(val);
	ret = bhy2_set_virt_sensor_cfg(sensor_id, sample_rate, sensor_report_latency, bhy2);
	bhi2xy_log_api_error(ret, bhy2);
	return bhi2xy_api_to_os_error(ret);
}

static int bhi2xy_init_hardware(const struct device *dev)
{
	const struct bhi2xy_config *cfg = dev->config;
	int ret;

	ret = bhi2xy_bus_check(dev);
	if (ret < 0) {
		LOG_ERR("Could not initialize bus");
		return -ENODEV;
	}

	if (!device_is_ready(cfg->reset_gpio.port)) {
		LOG_ERR("Reset GPIO port not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("Couldn't configure reset pin");
		return -ENODEV;
	}

	return 0;
}

static int bhi2xy_init_chip(const struct device *dev)
{
	const struct bhi2xy_config *cfg = dev->config;
	struct bhi2xy_data *data = dev->data;
	struct bhy2_dev *bhy2 = &data->bhy2;
	enum bhy2_intf intf;
	uint8_t product_id = 0;
	int ret;

	intf = cfg->bus_io->get_intf(&cfg->bus);
	ret = bhy2_init(intf, bhy2_reg_read, bhy2_reg_write, bhy2_delay_us, BHY2_RD_WR_LEN,
			(void *)dev, bhy2);
	if (ret != BHY2_OK) {
		return ret;
	}

	ret = bhy2_soft_reset(bhy2);
	if (ret != BHY2_OK) {
		return ret;
	}

	ret = bhy2_get_product_id(&product_id, bhy2);
	if (ret != BHY2_OK) {
		return ret;
	}

	if (product_id != BHY2_PRODUCT_ID) {
		LOG_ERR("Product ID read %X. Expected %X", product_id, BHY2_PRODUCT_ID);
		return BHY2_E_MAGIC;
	}

	LOG_DBG("BHI260/BHA260 found. Product ID read %X", product_id);
	return BHY2_OK;
}

static int bhi2xy_config_host_interface(struct bhy2_dev *bhy2)
{
	uint8_t hintr_ctrl;
	int ret;

	/* Disable status and debug FIFOs for normal operation */
	hintr_ctrl = BHY2_ICTL_DISABLE_STATUS_FIFO | BHY2_ICTL_DISABLE_DEBUG;
	ret = bhy2_set_host_interrupt_ctrl(hintr_ctrl, bhy2);
	if (ret != BHY2_OK) {
		return ret;
	}

	/* Read back for logging verification */
	ret = bhy2_get_host_interrupt_ctrl(&hintr_ctrl, bhy2);
	if (ret != BHY2_OK) {
		return ret;
	}

	LOG_DBG("Host interrupt control: Wake FIFO %s, Status FIFO %s, Debug %s, Active %s",
		(hintr_ctrl & BHY2_ICTL_DISABLE_FIFO_W) ? "disabled" : "enabled",
		(hintr_ctrl & BHY2_ICTL_DISABLE_STATUS_FIFO) ? "disabled" : "enabled",
		(hintr_ctrl & BHY2_ICTL_DISABLE_DEBUG) ? "disabled" : "enabled",
		(hintr_ctrl & BHY2_ICTL_ACTIVE_LOW) ? "Low" : "High");

	return bhy2_set_host_intf_ctrl(0, bhy2);
}

static int bhi2xy_load_firmware_and_verify(const struct device *dev)
{
	struct bhi2xy_data *data = dev->data;
	struct bhy2_dev *bhy2 = &data->bhy2;
	uint8_t boot_status;
	uint8_t sensor_error;
	uint16_t version = 0;
	int ret;
	int8_t temp_ret;

	/* Check if the sensor is ready to load firmware */
	ret = bhy2_get_boot_status(&boot_status, bhy2);
	if (ret != BHY2_OK) {
		return ret;
	}

	if (boot_status & BHY2_BST_HOST_INTERFACE_READY) {

		/* Upload Firmware to RAM */
		LOG_DBG("Loading firmware into RAM");
		ret = bhy2_upload_firmware_to_ram(bhi2xy_fw_data, sizeof(bhi2xy_fw_data), bhy2);

		/* Check for sensor-side errors during upload */
		temp_ret = bhy2_get_error_value(&sensor_error, bhy2);
		if (sensor_error) {
			LOG_ERR("%s", bhi2xy_get_sensor_error_text(sensor_error));
		}

		/* Log API errors if any */
		if (ret != BHY2_OK) {
			bhi2xy_log_api_error(ret, bhy2);
		}
		if (temp_ret != BHY2_OK) {
			bhi2xy_log_api_error(temp_ret, bhy2);
		}

		if (ret != BHY2_OK) {
			return ret;
		}

		/* Boot from RAM */
		LOG_DBG("Booting from RAM");
		ret = bhy2_boot_from_ram(bhy2);

		/* Check for sensor-side errors during boot */
		temp_ret = bhy2_get_error_value(&sensor_error, bhy2);
		if (sensor_error) {
			LOG_ERR("%s", bhi2xy_get_sensor_error_text(sensor_error));
		}

		if (ret != BHY2_OK) {
			bhi2xy_log_api_error(ret, bhy2);
		}
		if (temp_ret != BHY2_OK) {
			bhi2xy_log_api_error(temp_ret, bhy2);
		}

		if (ret != BHY2_OK) {
			return ret;
		}

		/* Verify Kernel Version */
		ret = bhy2_get_kernel_version(&version, bhy2);
		if (ret != BHY2_OK) {
			return ret;
		}

		if (version != 0) {
			LOG_DBG("Boot successful. Kernel version %u.", version);
		} else {
			LOG_WRN("Boot reported success, but Kernel version is 0.");
		}

	} else {
		LOG_ERR("Host interface not ready (Boot Status: 0x%02X).", boot_status);
		return BHY2_E_MAGIC;
	}

	return BHY2_OK;
}

static int bhi2xy_register_callbacks(const struct device *dev)
{
	struct bhi2xy_data *data = dev->data;
	struct bhy2_dev *bhy2 = &data->bhy2;
	int ret;

#define REG_CB(id, func, arg)                                                                      \
	do {                                                                                       \
		ret = bhy2_register_fifo_parse_callback(id, func, arg, bhy2);                      \
		if (ret != BHY2_OK) {                                                              \
			bhi2xy_log_api_error(ret, bhy2);                                           \
			return ret;                                                                \
		}                                                                                  \
	} while (0)

	REG_CB(BHY2_SYS_ID_META_EVENT, parse_meta_event, (void *)data);
	REG_CB(BHY2_SYS_ID_META_EVENT_WU, parse_meta_event, NULL);
	REG_CB(BHY2_SENSOR_ID_ACC, parse_3d_data, (void *)data->acc);
	REG_CB(BHY2_SENSOR_ID_GYRO, parse_3d_data, (void *)data->gyro);
	REG_CB(BHY2_SENSOR_ID_MAG, parse_3d_data, (void *)data->mag);
	REG_CB(BHY2_SENSOR_ID_ORI, parse_orientation, (void *)data->euler);
	REG_CB(BHY2_SENSOR_ID_STC, parse_step_count, (void *)&(data->step_count));
	REG_CB(BHY2_SENSOR_ID_GAMERV, parse_quaternion, (void *)data->grv);
	REG_CB(BHY2_SENSOR_ID_RV, parse_quaternion, (void *)data->rv);

#undef REG_CB

	return bhy2_update_virtual_sensor_list(bhy2);
}

static void bhi2xy_get_sensor_ranges(const struct device *dev)
{
	struct bhi2xy_data *data = dev->data;
	struct bhy2_dev *bhy2 = &data->bhy2;
	struct bhy2_phys_sensor_info sensor_info;
	int ret;

	/* Get Accel Range */
	ret = bhy2_get_phys_sensor_info(BHY2_PHYS_SENSOR_ID_ACCELEROMETER, &sensor_info, bhy2);
	if (ret != BHY2_OK) {
		bhi2xy_log_api_error(ret, bhy2);
		LOG_WRN("Failed to get Accel info. Using default +/- 8g");
		data->acc_range = 8;
	} else {
		data->acc_range = sensor_info.curr_range.u16_val;
		LOG_DBG("Accel range: +/- %d g", data->acc_range);
	}

	/* Get Gyro Range */
	ret = bhy2_get_phys_sensor_info(BHY2_PHYS_SENSOR_ID_GYROSCOPE, &sensor_info, bhy2);
	if (ret != BHY2_OK) {
		bhi2xy_log_api_error(ret, bhy2);
		LOG_WRN("Failed to get Gyro info. Using default +/- 2000 deg/s");
		data->gyro_range = 2000;
	} else {
		data->gyro_range = sensor_info.curr_range.u16_val;
		LOG_DBG("Gyro range: +/- %d deg/s", data->gyro_range);
	}
}

static int bhi2xy_init(const struct device *dev)
{
	int ret = 0;
	struct bhi2xy_data *data = dev->data;

	/* Initialize hardware (bus, GPIOs) */
	ret = bhi2xy_init_hardware(dev);
	if (ret < 0) {
		return ret;
	}

	/* Core chip initialization */
	ret = bhi2xy_init_chip(dev);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	/* Configure host interface and interrupts */
	ret = bhi2xy_config_host_interface(&data->bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	/* Load firmware and verify */
	ret = bhi2xy_load_firmware_and_verify(dev);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	/* Register FIFO parsing callbacks */
	ret = bhi2xy_register_callbacks(dev);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	/* Initial FIFO Flush */
	ret = bhy2_get_and_process_fifo(data->work_buffer, BHI2XY_WORK_BUFFER_SIZE, &data->bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	/* Update Physical Sensor Parameters */
	bhi2xy_get_sensor_ranges(dev);

bhi2xy_init_exit:
	if (ret != BHY2_OK) {
		bhi2xy_log_api_error(ret, &data->bhy2);
		return bhi2xy_api_to_os_error(ret);
	}

	return 0;
}

static inline int bhi2xy_bus_check(const struct device *dev)
{
	const struct bhi2xy_config *cfg = dev->config;
	return cfg->bus_io->check(&cfg->bus);
}

static BHY2_INTF_RET_TYPE bhy2_reg_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length,
					 void *intf_ptr)
{
	const struct device *dev = (const struct device *)intf_ptr;
	const struct bhi2xy_config *cfg = dev->config;
	return cfg->bus_io->write(&cfg->bus, reg_addr, reg_data, length);
}

static BHY2_INTF_RET_TYPE bhy2_reg_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length,
					void *intf_ptr)
{
	const struct device *dev = (const struct device *)intf_ptr;
	const struct bhi2xy_config *cfg = dev->config;
	return cfg->bus_io->read(&cfg->bus, reg_addr, reg_data, length);
}

static void bhy2_delay_us(uint32_t us, void *intf_ptr)
{
	(void)intf_ptr;
	k_busy_wait(us);
}

static int bhi2xy_api_to_os_error(int8_t ret_api)
{
	switch (ret_api) {
	case BHY2_OK:
		return 0;
	case BHY2_E_IO:
		return -EIO;
	case BHY2_E_TIMEOUT:
		return -ETIMEDOUT;
	case BHY2_E_MAGIC:
		return -ENODEV;
	default:
		return -ENOTSUP;
	}
}

static void bhi2xy_log_api_error(int8_t ret_api, struct bhy2_dev *bhy2)
{
	if (ret_api != BHY2_OK) {
		LOG_ERR("%s", bhi2xy_get_api_error(ret_api));
	}
}

static DEVICE_API(sensor, bhi2xy_driver_api) = {
	.sample_fetch = bhi2xy_sample_fetch,
	.channel_get = bhi2xy_channel_get,
	.attr_set = bhi2xy_attr_set,
	.attr_get = NULL,
};

#define BHI2XY_VARIANT_FROM_DT(inst)                                                               \
	(DT_ENUM_IDX(DT_DRV_INST(inst), variant) == 0 ? BHI2XY_VARIANT_BHI260AB                    \
						      : BHI2XY_VARIANT_BHI260AP)

#define BHI2XY_CONFIG_SPI(inst)                                                                    \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, BHI2XY_SPI_OPERATION, 0),                            \
	.bus_io = &bhi2xy_bus_io_spi,

#define BHI2XY_CONFIG_I2C(inst) .bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &bhi2xy_bus_io_i2c,

#define BHI2XY_CREATE_INST(inst)                                                                   \
                                                                                                   \
	static struct bhi2xy_data bhi2xy_drv_##inst;                                               \
                                                                                                   \
	static const struct bhi2xy_config bhi2xy_config_##inst = {COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                  \
                (BHI2XY_CONFIG_SPI(inst)),                                      \
                (BHI2XY_CONFIG_I2C(inst))) .variant =                    \
						   BHI2XY_VARIANT_FROM_DT(inst),                   \
		 .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0})};                  \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bhi2xy_init, NULL, &bhi2xy_drv_##inst,                  \
				     &bhi2xy_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bhi2xy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BHI2XY_CREATE_INST);
