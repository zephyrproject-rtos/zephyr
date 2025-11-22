#include "bhi2xy.h"
#include "bhi2xy_parsers.h"
#include "bhi2xy_unit_conversions.h"

#include "bhy2_defs.h"
#include "zephyr/drivers/sensor.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bhi2xy, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT bosch_bhi2xy

#define BHY2_RD_WR_LEN 256

// Bosch API requires these functions to be defined
static int bhi2xy_bus_check(const struct device *dev);
static BHY2_INTF_RET_TYPE bhy2_reg_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length,
					 void *intf_ptr);
static BHY2_INTF_RET_TYPE bhy2_reg_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length,
					void *intf_ptr);
static void bhy2_delay_us(uint32_t us, void *intf_ptr);

static int8_t bhy2_upload_firmware(struct bhy2_dev *bhy2);

// Bosch API to Zephyr error conversion and logging
static int bhy2_api_to_os_error(int8_t ret);
static void bhy2_log_api_error(int8_t ret, struct bhy2_dev *bhy2);
static const char *bhy2_get_api_error(int8_t error_code);
static const char *bhy2_get_sensor_error_text(uint8_t sensor_error);

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
	bhy2_log_api_error(ret, bhy2);
	return bhy2_api_to_os_error(ret);
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
		// step count is uint32_t, val members are int32_t, currently only val1 is used
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
	bhy2_log_api_error(ret, bhy2);
	return bhy2_api_to_os_error(ret);
}

static int bhi2xy_init(const struct device *dev)
{
	int ret = 0;
	const struct bhi2xy_config *cfg = dev->config;
	struct bhi2xy_data *data = dev->data;
	struct bhy2_dev *bhy2 = &data->bhy2;
	enum bhy2_intf intf;
	struct bhy2_phys_sensor_info sensor_info;
	uint8_t product_id = 0;
	uint8_t hintr_ctrl, hif_ctrl, boot_status;
	uint16_t version = 0;
	uint8_t *work_buffer = data->work_buffer;

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

	// interface configure for Bosch API
	intf = cfg->bus_io->get_intf(&cfg->bus);
	ret = bhy2_init(intf, bhy2_reg_read, bhy2_reg_write, bhy2_delay_us, BHY2_RD_WR_LEN,
			(void *)dev, bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}
	ret = bhy2_soft_reset(bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	ret = bhy2_get_product_id(&product_id, bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	if (product_id != BHY2_PRODUCT_ID) {
		LOG_ERR("Product ID read %X. Expected %X", product_id, BHY2_PRODUCT_ID);
		return -ENODEV;
	} else {
		LOG_DBG("BHI260/BHA260 found. Product ID read %X", product_id);
	}

	/* Check the interrupt pin and FIFO configurations. Disable status and debug */
	// comment if need to debug custom bhi260 firmware
	hintr_ctrl = BHY2_ICTL_DISABLE_STATUS_FIFO | BHY2_ICTL_DISABLE_DEBUG;
	ret = bhy2_set_host_interrupt_ctrl(hintr_ctrl, bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}
	ret = bhy2_get_host_interrupt_ctrl(&hintr_ctrl, bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	LOG_DBG("Host interrupt control");
	LOG_DBG("Wake up FIFO %s.",
		(hintr_ctrl & BHY2_ICTL_DISABLE_FIFO_W) ? "disabled" : "enabled");
	LOG_DBG("Non wake up FIFO %s.",
		(hintr_ctrl & BHY2_ICTL_DISABLE_FIFO_NW) ? "disabled" : "enabled");
	LOG_DBG("Status FIFO %s.",
		(hintr_ctrl & BHY2_ICTL_DISABLE_STATUS_FIFO) ? "disabled" : "enabled");
	LOG_DBG("Debugging %s.", (hintr_ctrl & BHY2_ICTL_DISABLE_DEBUG) ? "disabled" : "enabled");
	LOG_DBG("Fault %s.", (hintr_ctrl & BHY2_ICTL_DISABLE_FAULT) ? "disabled" : "enabled");
	LOG_DBG("Interrupt is %s.",
		(hintr_ctrl & BHY2_ICTL_ACTIVE_LOW) ? "active low" : "active high");
	LOG_DBG("Interrupt is %s triggered.", (hintr_ctrl & BHY2_ICTL_EDGE) ? "pulse" : "level");
	LOG_DBG("Interrupt pin drive is %s.",
		(hintr_ctrl & BHY2_ICTL_OPEN_DRAIN) ? "open drain" : "push-pull");

	/* Configure the host interface */
	hif_ctrl = BHY2_HIF_CTRL_ASYNC_STATUS_CHANNEL; // for debugging
	// hif_ctrl = 0;
	ret = bhy2_set_host_intf_ctrl(hif_ctrl, bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	/* Check if the sensor is ready to load firmware */
	ret = bhy2_get_boot_status(&boot_status, bhy2);
	if (ret != BHY2_OK) {
		goto bhi2xy_init_exit;
	}

	if (boot_status & BHY2_BST_HOST_INTERFACE_READY) {
		ret = bhy2_upload_firmware(bhy2);
		if (ret != BHY2_OK) {
			goto bhi2xy_init_exit;
		}

		ret = bhy2_get_kernel_version(&version, bhy2);
		if (ret != BHY2_OK) {
			goto bhi2xy_init_exit;
		}
		if ((ret == BHY2_OK) && (version != 0)) {
			LOG_DBG("Boot successful. Kernel version %u.", version);
		}

		ret = bhy2_register_fifo_parse_callback(BHY2_SYS_ID_META_EVENT, parse_meta_event,
							(void *)data, bhy2);
		bhy2_log_api_error(ret, bhy2);
		ret = bhy2_register_fifo_parse_callback(BHY2_SYS_ID_META_EVENT_WU, parse_meta_event,
							NULL, bhy2);
		bhy2_log_api_error(ret, bhy2);
		// for debugging purposes
		// ret = bhy2_register_fifo_parse_callback(BHY2_SYS_ID_DEBUG_MSG,
		// parse_debug_message, NULL, bhy2); bhy2_log_api_error(ret, bhy2); sensor data
		// parsing callbacks
		ret = bhy2_register_fifo_parse_callback(BHY2_SENSOR_ID_ACC, parse_3d_data,
							(void *)data->acc, bhy2);
		bhy2_log_api_error(ret, bhy2);
		ret = bhy2_register_fifo_parse_callback(BHY2_SENSOR_ID_GYRO, parse_3d_data,
							(void *)data->gyro, bhy2);
		bhy2_log_api_error(ret, bhy2);
		ret = bhy2_register_fifo_parse_callback(BHY2_SENSOR_ID_MAG, parse_3d_data,
							(void *)data->mag, bhy2);
		bhy2_log_api_error(ret, bhy2);
		ret = bhy2_register_fifo_parse_callback(BHY2_SENSOR_ID_ORI, parse_orientation,
							(void *)data->euler, bhy2);
		bhy2_log_api_error(ret, bhy2);
		ret = bhy2_register_fifo_parse_callback(BHY2_SENSOR_ID_STC, parse_step_count,
							(void *)&(data->step_count), bhy2);
		bhy2_log_api_error(ret, bhy2);
		ret = bhy2_register_fifo_parse_callback(BHY2_SENSOR_ID_GAMERV, parse_quaternion,
							(void *)data->grv, bhy2);
		bhy2_log_api_error(ret, bhy2);
		ret = bhy2_register_fifo_parse_callback(BHY2_SENSOR_ID_RV, parse_quaternion,
							(void *)data->rv, bhy2);
		bhy2_log_api_error(ret, bhy2);

		// flush the FIFO
		ret = bhy2_get_and_process_fifo(work_buffer, BHI2XY_WORK_BUFFER_SIZE, bhy2);
		if (ret != BHY2_OK) {
			goto bhi2xy_init_exit;
		}
	} else {
		LOG_ERR("Host interface not ready.");
		return -ENODEV;
	}

	/* Update the callback table to enable parsing of sensor data */
	ret = bhy2_update_virtual_sensor_list(bhy2);

	/* Get physical sensor info, to update dynamic ranges */
	ret = bhy2_get_phys_sensor_info(BHY2_PHYS_SENSOR_ID_ACCELEROMETER, &sensor_info, bhy2);
	if (ret != BHY2_OK) {
		bhy2_log_api_error(ret, bhy2);
		LOG_WRN("Couldn't get info for accel sensor. Using default dynamic range of +/- "
			"8g");
		data->acc_range = 8;
	} else {
		data->acc_range = sensor_info.curr_range.u16_val;
		LOG_DBG("Accel range: +/- %d g", data->acc_range);
	}
	ret = bhy2_get_phys_sensor_info(BHY2_PHYS_SENSOR_ID_GYROSCOPE, &sensor_info, bhy2);
	if (ret != BHY2_OK) {
		bhy2_log_api_error(ret, bhy2);
		LOG_WRN("Couldn't get info for gyro sensor. Using default dynamic range of +/- "
			"2000 deg/s");
		data->gyro_range = 2000;
	} else {
		data->gyro_range = sensor_info.curr_range.u16_val;
		LOG_DBG("Gyro range: +/- %d deg/s", data->gyro_range);
	}

bhi2xy_init_exit:
	bhy2_log_api_error(ret, bhy2);
	return bhy2_api_to_os_error(ret);
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
	k_busy_wait(us); //< wait for the reset to end
}

static int8_t bhy2_upload_firmware(struct bhy2_dev *bhy2)
{
	uint8_t sensor_error;
	int ret = BHY2_OK;
	int8_t temp_ret;

	// sensor's firmware is stored in .rodata section
	extern const char _binary_bhi2xy_fw_start, _binary_bhi2xy_fw_end;
	const uint8_t *fw_start = &_binary_bhi2xy_fw_start;
	const uint8_t *fw_end = &_binary_bhi2xy_fw_end;
	const uint32_t fw_len = fw_end - fw_start;

	LOG_DBG("Loading firmware into RAM");
	ret = bhy2_upload_firmware_to_ram(fw_start, fw_len, bhy2);
	temp_ret = bhy2_get_error_value(&sensor_error, bhy2);
	if (sensor_error) {
		LOG_ERR("%s", bhy2_get_sensor_error_text(sensor_error));
	}

	// TODO: check these two logs
	bhy2_log_api_error(ret, bhy2);
	bhy2_log_api_error(temp_ret, bhy2);

	LOG_DBG("Booting from RAM");
	ret = bhy2_boot_from_ram(bhy2);

	temp_ret = bhy2_get_error_value(&sensor_error, bhy2);
	if (sensor_error) {
		LOG_ERR("%s", bhy2_get_sensor_error_text(sensor_error));
	}

	bhy2_log_api_error(ret, bhy2);
	bhy2_log_api_error(temp_ret, bhy2);

	return ret;
}

static int bhy2_api_to_os_error(int8_t ret_api)
{
	int ret;
	switch (ret_api) {
	case BHY2_OK:
		ret = 0;
		break;
	case BHY2_E_IO:
		ret = -EIO;
		break;
	case BHY2_E_TIMEOUT:
		ret = -ETIMEDOUT;
		break;
	default:
		ret = -ENOTSUP;
	}
	return ret;
}

static void bhy2_log_api_error(int8_t ret_api, struct bhy2_dev *bhy2)
{
	if (ret_api != BHY2_OK) {
		LOG_ERR("%s", bhy2_get_api_error(ret_api));
	}
}

static const char *bhy2_get_api_error(int8_t error_code)
{
	const char *ret = " ";

	switch (error_code) {
	case BHY2_OK:
		break;
	case BHY2_E_NULL_PTR:
		ret = "[API Error] Null pointer";
		break;
	case BHY2_E_INVALID_PARAM:
		ret = "[API Error] Invalid parameter";
		break;
	case BHY2_E_IO:
		ret = "[API Error] IO error";
		break;
	case BHY2_E_MAGIC:
		ret = "[API Error] Invalid firmware";
		break;
	case BHY2_E_TIMEOUT:
		ret = "[API Error] Timed out";
		break;
	case BHY2_E_BUFFER:
		ret = "[API Error] Invalid buffer";
		break;
	case BHY2_E_INVALID_FIFO_TYPE:
		ret = "[API Error] Invalid FIFO type";
		break;
	case BHY2_E_INVALID_EVENT_SIZE:
		ret = "[API Error] Invalid Event size";
		break;
	case BHY2_E_PARAM_NOT_SET:
		ret = "[API Error] Parameter not set";
		break;
	default:
		ret = "[API Error] Unknown API error code";
	}
	return ret;
}

static const char *bhy2_get_sensor_error_text(uint8_t sensor_error)
{
	const char *ret = "";

	switch (sensor_error) {
	case 0x00:
		break;
	case 0x10:
		ret = "[Sensor error] Bootloader reports: Firmware Expected Version Mismatch";
		break;
	case 0x11:
		ret = "[Sensor error] Bootloader reports: Firmware Upload Failed: Bad Header CRC";
		break;
	case 0x12:
		ret = "[Sensor error] Bootloader reports: Firmware Upload Failed: SHA Hash "
		      "Mismatch";
		break;
	case 0x13:
		ret = "[Sensor error] Bootloader reports: Firmware Upload Failed: Bad Image CRC";
		break;
	case 0x14:
		ret = "[Sensor error] Bootloader reports: Firmware Upload Failed: ECDSA Signature "
		      "Verification Failed";
		break;
	case 0x15:
		ret = "[Sensor error] Bootloader reports: Firmware Upload Failed: Bad Public Key "
		      "CRC";
		break;
	case 0x16:
		ret = "[Sensor error] Bootloader reports: Firmware Upload Failed: Signed Firmware "
		      "Required";
		break;
	case 0x17:
		ret = "[Sensor error] Bootloader reports: Firmware Upload Failed: FW Header "
		      "Missing";
		break;
	case 0x19:
		ret = "[Sensor error] Bootloader reports: Unexpected Watchdog Reset";
		break;
	case 0x1A:
		ret = "[Sensor error] ROM Version Mismatch";
		break;
	case 0x1B:
		ret = "[Sensor error] Bootloader reports: Fatal Firmware Error";
		break;
	case 0x1C:
		ret = "[Sensor error] Chained Firmware Error: Next Payload Not Found";
		break;
	case 0x1D:
		ret = "[Sensor error] Chained Firmware Error: Payload Not Valid";
		break;
	case 0x1E:
		ret = "[Sensor error] Chained Firmware Error: Payload Entries Invalid";
		break;
	case 0x1F:
		ret = "[Sensor error] Bootloader reports: Bootloader Error: OTP CRC Invalid";
		break;
	case 0x20:
		ret = "[Sensor error] Firmware Init Failed";
		break;
	case 0x21:
		ret = "[Sensor error] Sensor Init Failed: Unexpected Device ID";
		break;
	case 0x22:
		ret = "[Sensor error] Sensor Init Failed: No Response from Device";
		break;
	case 0x23:
		ret = "[Sensor error] Sensor Init Failed: Unknown";
		break;
	case 0x24:
		ret = "[Sensor error] Sensor Error: No Valid Data";
		break;
	case 0x25:
		ret = "[Sensor error] Slow Sample Rate";
		break;
	case 0x26:
		ret = "[Sensor error] Data Overflow (saturated sensor data)";
		break;
	case 0x27:
		ret = "[Sensor error] Stack Overflow";
		break;
	case 0x28:
		ret = "[Sensor error] Insufficient Free RAM";
		break;
	case 0x29:
		ret = "[Sensor error] Sensor Init Failed: Driver Parsing Error";
		break;
	case 0x2A:
		ret = "[Sensor error] Too Many RAM Banks Required";
		break;
	case 0x2B:
		ret = "[Sensor error] Invalid Event Specified";
		break;
	case 0x2C:
		ret = "[Sensor error] More than 32 On Change";
		break;
	case 0x2D:
		ret = "[Sensor error] Firmware Too Large";
		break;
	case 0x2F:
		ret = "[Sensor error] Invalid RAM Banks";
		break;
	case 0x30:
		ret = "[Sensor error] Math Error";
		break;
	case 0x40:
		ret = "[Sensor error] Memory Error";
		break;
	case 0x41:
		ret = "[Sensor error] SWI3 Error";
		break;
	case 0x42:
		ret = "[Sensor error] SWI4 Error";
		break;
	case 0x43:
		ret = "[Sensor error] Illegal Instruction Error";
		break;
	case 0x44:
		ret = "[Sensor error] Bootloader reports: Unhandled Interrupt Error / Exception / "
		      "Postmortem Available";
		break;
	case 0x45:
		ret = "[Sensor error] Invalid Memory Access";
		break;
	case 0x50:
		ret = "[Sensor error] Algorithm Error: BSX Init";
		break;
	case 0x51:
		ret = "[Sensor error] Algorithm Error: BSX Do Step";
		break;
	case 0x52:
		ret = "[Sensor error] Algorithm Error: Update Sub";
		break;
	case 0x53:
		ret = "[Sensor error] Algorithm Error: Get Sub";
		break;
	case 0x54:
		ret = "[Sensor error] Algorithm Error: Get Phys";
		break;
	case 0x55:
		ret = "[Sensor error] Algorithm Error: Unsupported Phys Rate";
		break;
	case 0x56:
		ret = "[Sensor error] Algorithm Error: Cannot find BSX Driver";
		break;
	case 0x60:
		ret = "[Sensor error] Sensor Self-Test Failure";
		break;
	case 0x61:
		ret = "[Sensor error] Sensor Self-Test X Axis Failure";
		break;
	case 0x62:
		ret = "[Sensor error] Sensor Self-Test Y Axis Failure";
		break;
	case 0x64:
		ret = "[Sensor error] Sensor Self-Test Z Axis Failure";
		break;
	case 0x65:
		ret = "[Sensor error] FOC Failure";
		break;
	case 0x66:
		ret = "[Sensor error] Sensor Busy";
		break;
	case 0x6F:
		ret = "[Sensor error] Self-Test or FOC Test Unsupported";
		break;
	case 0x72:
		ret = "[Sensor error] No Host Interrupt Set";
		break;
	case 0x73:
		ret = "[Sensor error] Event ID Passed to Host Interface Has No Known Size";
		break;
	case 0x75:
		ret = "[Sensor error] Host Download Channel Underflow (Host Read Too Fast)";
		break;
	case 0x76:
		ret = "[Sensor error] Host Upload Channel Overflow (Host Wrote Too Fast)";
		break;
	case 0x77:
		ret = "[Sensor error] Host Download Channel Empty";
		break;
	case 0x78:
		ret = "[Sensor error] DMA Error";
		break;
	case 0x79:
		ret = "[Sensor error] Corrupted Input Block Chain";
		break;
	case 0x7A:
		ret = "[Sensor error] Corrupted Output Block Chain";
		break;
	case 0x7B:
		ret = "[Sensor error] Buffer Block Manager Error";
		break;
	case 0x7C:
		ret = "[Sensor error] Input Channel Not Word Aligned";
		break;
	case 0x7D:
		ret = "[Sensor error] Too Many Flush Events";
		break;
	case 0x7E:
		ret = "[Sensor error] Unknown Host Channel Error";
		break;
	case 0x81:
		ret = "[Sensor error] Decimation Too Large";
		break;
	case 0x90:
		ret = "[Sensor error] Master SPI/I2C Queue Overflow";
		break;
	case 0x91:
		ret = "[Sensor error] SPI/I2C Callback Error";
		break;
	case 0xA0:
		ret = "[Sensor error] Timer Scheduling Error";
		break;
	case 0xB0:
		ret = "[Sensor error] Invalid GPIO for Host IRQ";
		break;
	case 0xB1:
		ret = "[Sensor error] Error Sending Initialized Meta Events";
		break;
	case 0xC0:
		ret = "[Sensor error] Bootloader reports: Command Error";
		break;
	case 0xC1:
		ret = "[Sensor error] Bootloader reports: Command Too Long";
		break;
	case 0xC2:
		ret = "[Sensor error] Bootloader reports: Command Buffer Overflow";
		break;
	case 0xD0:
		ret = "[Sensor error] User Mode Error: Sys Call Invalid";
		break;
	case 0xD1:
		ret = "[Sensor error] User Mode Error: Trap Invalid";
		break;
	case 0xE1:
		ret = "[Sensor error] Firmware Upload Failed: Firmware header corrupt";
		break;
	case 0xE2:
		ret = "[Sensor error] Sensor Data Injection: Invalid input stream";
		break;
	default:
		ret = "[Sensor error] Unknown error code";
	}

	return ret;
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
