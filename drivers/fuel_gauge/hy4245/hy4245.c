/*
 * Copyright (c) 2025, Linumiz GmbH
 * Copyright (c) 2026, Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hycon_hy4245

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/fuel_gauge/hy4245.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "hy4245.h"

LOG_MODULE_REGISTER(HY4245, CONFIG_FUEL_GAUGE_LOG_LEVEL);

#define HY4245_CTRL_STATUS_RETRIES  3
/* Delay after data flash accesses, as per data sheet */
#define HY4245_DATA_FLASH_DELAY_MS  10
/* The gauge delivers the DFChecksum() result within 500 ms */
#define HY4245_DF_CHECKSUM_DELAY_MS 500
/* The gauge holds the clock line low for 250 ms after QuickStart() */
#define HY4245_QUICK_START_DELAY_MS 250

/*
 * Configuration image layout rules.
 *
 * Elements of the image that hold device specific calibration data
 * (subclass 0x02 blocks 0 and 1) or manufacturer info block A (subclass
 * 0x20 block 0) must not be programmed. Bytes 7 and 8 of subclass 0x03
 * block 1 hold device specific values that are preserved.
 */
struct hy4245_image_location {
	uint8_t subclass;
	uint8_t block;
};

static const struct hy4245_image_location hy4245_image_skip[] = {
	{0x02, 0x00},
	{0x02, 0x01},
	{HY4245_SUBCLASS_MANUFACTURER_INFO, 0x00},
};

static const struct hy4245_image_location hy4245_image_preserve = {0x03, 0x01};
#define HY4245_IMAGE_PRESERVE_OFFSET 7
#define HY4245_IMAGE_PRESERVE_LEN    2

struct hy4245_config {
	struct i2c_dt_spec i2c;
};

struct hy4245_data {
	/* Gauge is unsealed and in calibration mode, data flash accessible */
	bool calibration_mode;
};

/* Low level register access */

static int hy4245_read16(const struct device *dev, uint8_t cmd, uint16_t *val)
{
	uint8_t buffer[2];
	const struct hy4245_config *cfg = dev->config;
	int ret;

	ret = i2c_burst_read_dt(&cfg->i2c, cmd, buffer, sizeof(buffer));
	if (ret != 0) {
		LOG_ERR("Unable to read register 0x%02x, error %d", cmd, ret);
		return ret;
	}

	*val = sys_get_le16(buffer);
	return 0;
}

/* Write a Control() subcommand */
static int hy4245_ctrl_write(const struct device *dev, uint16_t subcmd)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t cmd[3] = {HY4245_CMD_CTRL};

	sys_put_le16(subcmd, &cmd[1]);

	return i2c_write_dt(&cfg->i2c, cmd, sizeof(cmd));
}

/* Write a Control() subcommand and read its 16 bit result */
static int hy4245_ctrl_read(const struct device *dev, uint16_t subcmd, uint16_t *val)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t cmd[3] = {HY4245_CMD_CTRL};
	uint8_t buffer[2];
	int ret;

	sys_put_le16(subcmd, &cmd[1]);

	ret = i2c_write_read_dt(&cfg->i2c, cmd, sizeof(cmd), buffer, sizeof(buffer));
	if (ret != 0) {
		LOG_ERR("Control() 0x%04x failed, error %d", subcmd, ret);
		return ret;
	}

	*val = sys_get_le16(buffer);
	return 0;
}

/*
 * Poll ControlStatus() until one of the bits in mask is set.
 *
 * Returns the status register value (positive) or a negative error code.
 */
static int hy4245_wait_ctrl_status(const struct device *dev, uint16_t mask)
{
	uint16_t status;
	int ret;

	for (int i = 0; i < HY4245_CTRL_STATUS_RETRIES; i++) {
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_STATUS, &status);
		if (ret != 0) {
			return ret;
		}

		if ((status & mask) != 0) {
			return status;
		}

		k_sleep(K_MSEC(1));
	}

	return -EIO;
}

/* Data flash access */

/*
 * Unseal the gauge with the default keys. Unseal key 0 is written first,
 * followed by unseal key 1. A failed attempt blocks further attempts for
 * four seconds.
 */
static int hy4245_unseal(const struct device *dev)
{
	const struct hy4245_config *cfg = dev->config;
	const uint8_t key0[] = {HY4245_CMD_CTRL, HY4245_UNSEAL_KEY_0};
	const uint8_t key1[] = {HY4245_CMD_CTRL, HY4245_UNSEAL_KEY_1};
	int ret;

	ret = i2c_write_dt(&cfg->i2c, key0, sizeof(key0));
	if (ret != 0) {
		return ret;
	}

	k_sleep(K_MSEC(1));

	return i2c_write_dt(&cfg->i2c, key1, sizeof(key1));
}

/* Unseal the gauge and enable access to the data flash via BlockData() */
static int hy4245_enable_data_flash_access(const struct device *dev)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t cmd[2] = {HY4245_EXTCMD_BLKDATA_CTRL, 0x00};
	uint8_t resp = 0;
	int ret;

	ret = hy4245_unseal(dev);
	if (ret != 0) {
		LOG_ERR("Unseal failed, error %d", ret);
		return ret;
	}

	ret = i2c_write_dt(&cfg->i2c, cmd, sizeof(cmd));
	if (ret != 0) {
		return ret;
	}

	ret = i2c_write_read_dt(&cfg->i2c, cmd, 1, &resp, sizeof(resp));
	if (ret != 0 || resp != 0) {
		LOG_ERR("Enabling data flash access failed, error %d, response 0x%02x", ret, resp);
		return -EIO;
	}

	return 0;
}

/* Put the gauge into calibration mode, required for data flash programming */
static int hy4245_enter_calibration_mode(const struct device *dev)
{
	int ret;

	ret = hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_CALIB_MODE);
	if (ret != 0) {
		return ret;
	}

	ret = hy4245_wait_ctrl_status(dev, HY4245_CONTROL_STATUS_BCA);
	if (ret < 0) {
		LOG_ERR("Calibration mode not entered, error %d", ret);
		return ret;
	}

	return 0;
}

static int hy4245_reset_device(const struct device *dev)
{
	struct hy4245_data *data = dev->data;
	int ret;

	ret = hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_RESET);
	if (ret != 0) {
		LOG_ERR("Reset failed, error %d", ret);
		return ret;
	}

	data->calibration_mode = false;

	if (CONFIG_HY4245_RESET_SETTLE_TIME_MS > 0) {
		k_sleep(K_MSEC(CONFIG_HY4245_RESET_SETTLE_TIME_MS));
	}

	return 0;
}

static int hy4245_set_flash_class_block(const struct device *dev, uint8_t subclass, uint8_t block)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t cmd[2] = {HY4245_EXTCMD_SUBCLASS, subclass};
	int ret;

	ret = i2c_write_dt(&cfg->i2c, cmd, sizeof(cmd));
	if (ret != 0) {
		return ret;
	}

	ret = hy4245_wait_ctrl_status(dev, HY4245_CONTROL_STATUS_CSV);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(HY4245_DATA_FLASH_DELAY_MS));

	cmd[0] = HY4245_EXTCMD_BLOCK;
	cmd[1] = block;

	ret = i2c_write_dt(&cfg->i2c, cmd, sizeof(cmd));
	if (ret != 0) {
		return ret;
	}

	ret = hy4245_wait_ctrl_status(dev, HY4245_CONTROL_STATUS_CSV);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(HY4245_DATA_FLASH_DELAY_MS));

	return 0;
}

/* BlockDataChecksum(): complement of the least significant byte of the sum */
static uint8_t hy4245_block_checksum(const uint8_t *data, size_t len)
{
	uint8_t sum = 0;

	for (size_t i = 0; i < len; i++) {
		sum += data[i];
	}

	return 0xff - sum;
}

/*
 * Read or write one complete data flash block.
 *
 * The BlockDataChecksum() of the gauge covers the whole 32 byte block, so
 * partial blocks cannot be verified and are always handled as whole blocks
 * by the callers. A written block is only committed by the gauge when the
 * checksum written afterwards matches the block data.
 */
static int hy4245_access_data_flash(const struct device *dev, uint8_t subclass, uint8_t block,
				    uint8_t *data, bool is_read)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t cmd[HY4245_DATA_FLASH_BLOCK_SIZE + 1] = {HY4245_EXTCMD_BLKDATA};
	uint8_t checksum;
	uint8_t resp;
	int ret;

	if (data == NULL) {
		return -EINVAL;
	}

	ret = hy4245_set_flash_class_block(dev, subclass, block);
	if (ret != 0) {
		LOG_ERR("Selecting subclass 0x%02x block %u failed, error %d", subclass, block,
			ret);
		return ret;
	}

	if (is_read) {
		ret = i2c_write_read_dt(&cfg->i2c, cmd, 1, data, HY4245_DATA_FLASH_BLOCK_SIZE);
	} else {
		memcpy(&cmd[1], data, HY4245_DATA_FLASH_BLOCK_SIZE);
		ret = i2c_write_dt(&cfg->i2c, cmd, sizeof(cmd));
	}
	if (ret != 0) {
		LOG_ERR("Data flash %s failed, error %d", is_read ? "read" : "write", ret);
		return ret;
	}

	ret = hy4245_wait_ctrl_status(dev, HY4245_CONTROL_STATUS_CSV);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(HY4245_DATA_FLASH_DELAY_MS));

	checksum = hy4245_block_checksum(data, HY4245_DATA_FLASH_BLOCK_SIZE);
	cmd[0] = HY4245_EXTCMD_BLKDATA_CHECKSUM;

	if (!is_read) {
		cmd[1] = checksum;
		ret = i2c_write_dt(&cfg->i2c, cmd, 2);
		if (ret != 0) {
			return ret;
		}

		ret = hy4245_wait_ctrl_status(dev, HY4245_CONTROL_STATUS_CSV);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_write_read_dt(&cfg->i2c, cmd, 1, &resp, sizeof(resp));
	if (ret != 0) {
		LOG_ERR("Checksum read failed, error %d", ret);
		return ret;
	}

	if (checksum != resp) {
		LOG_ERR("Checksum mismatch, expected 0x%02x got 0x%02x", checksum, resp);
		return -EILSEQ;
	}

	return 0;
}

static int hy4245_check_calibration_mode(const struct device *dev)
{
	const struct hy4245_data *data = dev->data;

	if (!data->calibration_mode) {
		LOG_ERR("Data flash access requires calibration mode");
		return -EACCES;
	}

	return 0;
}

/* Configuration image handling */

static bool hy4245_image_element_skipped(uint8_t subclass, uint8_t block)
{
	for (size_t i = 0; i < ARRAY_SIZE(hy4245_image_skip); i++) {
		if (hy4245_image_skip[i].subclass == subclass &&
		    hy4245_image_skip[i].block == block) {
			return true;
		}
	}

	return false;
}

static bool hy4245_image_element_preserved(uint8_t subclass, uint8_t block)
{
	return hy4245_image_preserve.subclass == subclass && hy4245_image_preserve.block == block;
}

/*
 * Program or verify a configuration image.
 *
 * Each element consists of the subclass id, the block number and one data
 * flash block. Skipped elements are neither programmed nor verified,
 * preserved bytes are taken from the gauge before programming and ignored
 * while verifying.
 */
static int hy4245_process_config_image(const struct device *dev, const uint8_t *image,
				       size_t image_len, bool verify)
{
	uint8_t block_data[HY4245_DATA_FLASH_BLOCK_SIZE];
	uint8_t device_data[HY4245_DATA_FLASH_BLOCK_SIZE];
	int ret;

	if (image == NULL || image_len == 0 ||
	    (image_len % HY4245_CONFIG_IMAGE_ELEMENT_SIZE) != 0) {
		return -EINVAL;
	}

	ret = hy4245_check_calibration_mode(dev);
	if (ret != 0) {
		return ret;
	}

	for (size_t offset = 0; offset < image_len; offset += HY4245_CONFIG_IMAGE_ELEMENT_SIZE) {
		const uint8_t subclass = image[offset];
		const uint8_t block = image[offset + 1];
		bool preserve = hy4245_image_element_preserved(subclass, block);

		if (hy4245_image_element_skipped(subclass, block)) {
			continue;
		}

		memcpy(block_data, &image[offset + 2], sizeof(block_data));

		if (verify || preserve) {
			ret = hy4245_access_data_flash(dev, subclass, block, device_data, true);
			if (ret != 0) {
				return ret;
			}
		}

		if (preserve) {
			memcpy(&block_data[HY4245_IMAGE_PRESERVE_OFFSET],
			       &device_data[HY4245_IMAGE_PRESERVE_OFFSET],
			       HY4245_IMAGE_PRESERVE_LEN);
		}

		if (verify) {
			if (memcmp(block_data, device_data, sizeof(block_data)) != 0) {
				LOG_ERR("Image mismatch in subclass 0x%02x block %u", subclass,
					block);
				return -EILSEQ;
			}
			continue;
		}

		ret = hy4245_access_data_flash(dev, subclass, block, block_data, false);
		if (ret != 0) {
			LOG_ERR("Programming subclass 0x%02x block %u failed, error %d", subclass,
				block, ret);
			return ret;
		}
	}

	return 0;
}

/* Control() based status and version information */

static int hy4245_get_data_flash_checksum(const struct device *dev, uint16_t *checksum)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t buffer[2];
	int ret;

	ret = hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_DF_CHECKSUM);
	if (ret != 0) {
		return ret;
	}

	/* The gauge needs time to calculate the checksum before it can be read */
	k_sleep(K_MSEC(HY4245_DF_CHECKSUM_DELAY_MS));

	ret = i2c_burst_read_dt(&cfg->i2c, HY4245_CMD_CTRL, buffer, sizeof(buffer));
	if (ret != 0) {
		return ret;
	}

	*checksum = sys_get_le16(buffer);
	return 0;
}

/* OperationCfgA().UPD_EN: the gauge may update its data flash while operating */
static int hy4245_get_flash_update_enabled(const struct device *dev, bool *enabled)
{
	uint16_t cfg_a;
	int ret;

	ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_OPERATION_CFG_A, &cfg_a);
	if (ret != 0) {
		return ret;
	}

	*enabled = (cfg_a & HY4245_OPERATION_CONFIG_A_UPD_EN) != 0;
	return 0;
}

static int hy4245_enable_flash_update(const struct device *dev)
{
	bool enabled;
	int ret;

	ret = hy4245_get_flash_update_enabled(dev, &enabled);
	if (ret != 0 || enabled) {
		return ret;
	}

	return hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_SET_UPD_EN);
}

/* Fuel gauge API */

static int hy4245_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			   union fuel_gauge_prop_val *val)
{
	const struct hy4245_data *data = dev->data;
	uint16_t raw = 0;
	bool flag = false;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_TEMPERATURE_DK:
		ret = hy4245_read16(dev, HY4245_CMD_TEMPERATURE, &raw);
		val->temperature_dk = raw;
		break;
	case FUEL_GAUGE_VOLTAGE_UV:
		ret = hy4245_read16(dev, HY4245_CMD_VOLTAGE, &raw);
		val->voltage_uv = raw * 1000;
		break;
	case FUEL_GAUGE_CURRENT_UA:
		ret = hy4245_read16(dev, HY4245_CMD_CURRENT, &raw);
		val->current_ua = (int16_t)raw * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_REM, &raw);
		val->remaining_capacity_uah = raw * 1000;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_FULL, &raw);
		val->full_charge_capacity_uah = raw * 1000;
		break;
	case FUEL_GAUGE_AVG_CURRENT_UA:
		ret = hy4245_read16(dev, HY4245_CMD_AVG_CURRENT, &raw);
		val->avg_current_ua = (int16_t)raw * 1000;
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS:
		ret = hy4245_read16(dev, HY4245_CMD_TIME_TO_EMPTY, &raw);
		val->runtime_to_empty_mins = raw;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL_MINS:
		ret = hy4245_read16(dev, HY4245_CMD_TIME_TO_FULL, &raw);
		val->runtime_to_full_mins = raw;
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE_UV:
		ret = hy4245_read16(dev, HY4245_CMD_CHRG_VOLTAGE, &raw);
		val->chg_voltage_uv = raw * 1000;
		break;
	case FUEL_GAUGE_CHARGE_CURRENT_UA:
		ret = hy4245_read16(dev, HY4245_CMD_CHRG_CURRENT, &raw);
		val->chg_current_ua = raw * 1000;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_FULL_AVAIL, &raw);
		val->design_cap = raw;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
		ret = hy4245_read16(dev, HY4245_CMD_RELATIVE_STATE_OF_CHRG, &raw);
		val->relative_state_of_charge_pct = raw;
		break;
	case FUEL_GAUGE_STATE_OF_HEALTH:
		ret = hy4245_read16(dev, HY4245_CMD_STATE_OF_HEALTH, &raw);
		val->state_of_health = raw & 0xff;
		break;
	case HY4245_FUEL_GAUGE_CALIBRATION_MODE:
		val->custom_bool = data->calibration_mode;
		ret = 0;
		break;
	case HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE:
		ret = hy4245_get_flash_update_enabled(dev, &flag);
		val->custom_bool = flag;
		break;
	case HY4245_FUEL_GAUGE_CONTROL_STATUS:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_STATUS, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_OPERATION_CONFIG_A:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_OPERATION_CFG_A, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_FLAGS:
		ret = hy4245_read16(dev, HY4245_CMD_FLAGS, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_SAFETY_STATUS:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_SAFETY_STATUS, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_LIFETIME_OVER_TEMPERATURE_MINS:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_LIFETIME_OVER_TEMP, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_DATA_FLASH_CHECKSUM:
		ret = hy4245_get_data_flash_checksum(dev, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_DATA_FLASH_VERSION:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_DF_VERSION, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_FIRMWARE_VERSION:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_FW_VERSION, &raw);
		val->custom_uint = raw;
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int hy4245_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			   union fuel_gauge_prop_val val)
{
	struct hy4245_data *data = dev->data;
	int ret;

	switch (prop) {
	case HY4245_FUEL_GAUGE_CALIBRATION_MODE:
		if (!val.custom_bool) {
			ret = hy4245_reset_device(dev);
			break;
		}

		if (data->calibration_mode) {
			ret = 0;
			break;
		}

		ret = hy4245_enable_data_flash_access(dev);
		if (ret != 0) {
			break;
		}

		ret = hy4245_enter_calibration_mode(dev);
		if (ret == 0) {
			data->calibration_mode = true;
		}
		break;
	case HY4245_FUEL_GAUGE_RESET:
		if (!val.custom_bool) {
			return -EINVAL;
		}

		ret = hy4245_reset_device(dev);
		break;
	case HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE:
		if (!val.custom_bool) {
			return -ENOTSUP;
		}

		ret = hy4245_enable_flash_update(dev);
		break;
	case HY4245_FUEL_GAUGE_CLEAR_LEARNED:
		if (!val.custom_bool) {
			return -EINVAL;
		}

		ret = hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_CLEAR_LEARNED);
		break;
	case HY4245_FUEL_GAUGE_QUICK_START:
		if (!val.custom_bool) {
			return -EINVAL;
		}

		ret = hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_QUICK_START);
		if (ret == 0) {
			/* The gauge stalls the bus while it re-estimates the capacity */
			k_sleep(K_MSEC(HY4245_QUICK_START_DELAY_MS));
		}
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int hy4245_manufacturer_info_block(fuel_gauge_prop_t prop)
{
	switch (prop) {
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_A:
		return 0;
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_B:
		return 1;
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_C:
		return 2;
	default:
		return -ENOTSUP;
	}
}

static int hy4245_get_buffer_prop(const struct device *dev, fuel_gauge_prop_t prop, void *dst,
				  size_t dst_len)
{
	uint8_t block_data[HY4245_DATA_FLASH_BLOCK_SIZE];
	struct hy4245_data_flash_block *flash_block;
	int block;
	int ret;

	if (dst == NULL || dst_len == 0) {
		return -EINVAL;
	}

	ret = hy4245_check_calibration_mode(dev);
	if (ret != 0) {
		return ret;
	}

	switch (prop) {
	case HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK:
		if (dst_len != sizeof(*flash_block)) {
			return -EINVAL;
		}

		flash_block = dst;
		if (flash_block->len == 0 || flash_block->len > HY4245_DATA_FLASH_BLOCK_SIZE) {
			return -EINVAL;
		}

		/* The checksum covers the whole block, so always read all of it */
		ret = hy4245_access_data_flash(dev, flash_block->subclass, flash_block->block,
					       block_data, true);
		if (ret != 0) {
			return ret;
		}

		memcpy(flash_block->data, block_data, flash_block->len);
		return 0;
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_A:
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_B:
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_C:
		if (dst_len > sizeof(block_data)) {
			return -EINVAL;
		}

		block = hy4245_manufacturer_info_block(prop);

		/* Always read the whole block, the checksum covers all bytes */
		ret = hy4245_access_data_flash(dev, HY4245_SUBCLASS_MANUFACTURER_INFO, block,
					       block_data, true);
		if (ret != 0) {
			return ret;
		}

		memcpy(dst, block_data, dst_len);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int hy4245_set_buffer_prop(const struct device *dev, fuel_gauge_prop_t prop, const void *src,
				  size_t src_len)
{
	uint8_t block_data[HY4245_DATA_FLASH_BLOCK_SIZE] = {0};
	const struct hy4245_data_flash_block *flash_block;
	int block;
	int ret;

	if (src == NULL || src_len == 0) {
		return -EINVAL;
	}

	ret = hy4245_check_calibration_mode(dev);
	if (ret != 0) {
		return ret;
	}

	switch (prop) {
	case HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK:
		if (src_len != sizeof(*flash_block)) {
			return -EINVAL;
		}

		flash_block = src;
		if (flash_block->len == 0 || flash_block->len > HY4245_DATA_FLASH_BLOCK_SIZE) {
			return -EINVAL;
		}

		if (flash_block->len < HY4245_DATA_FLASH_BLOCK_SIZE) {
			/* Keep the bytes of the block that are not written */
			ret = hy4245_access_data_flash(dev, flash_block->subclass,
						       flash_block->block, block_data, true);
			if (ret != 0) {
				return ret;
			}
		}

		memcpy(block_data, flash_block->data, flash_block->len);
		return hy4245_access_data_flash(dev, flash_block->subclass, flash_block->block,
						block_data, false);
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_A:
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_B:
	case HY4245_FUEL_GAUGE_MANUFACTURER_INFO_C:
		if (src_len > sizeof(block_data)) {
			return -EINVAL;
		}

		block = hy4245_manufacturer_info_block(prop);

		/* Always write the whole block, padded with zeros */
		memcpy(block_data, src, src_len);
		return hy4245_access_data_flash(dev, HY4245_SUBCLASS_MANUFACTURER_INFO, block,
						block_data, false);
	case HY4245_FUEL_GAUGE_CONFIG_IMAGE:
		return hy4245_process_config_image(dev, src, src_len, false);
	case HY4245_FUEL_GAUGE_CONFIG_IMAGE_VERIFY:
		return hy4245_process_config_image(dev, src, src_len, true);
	default:
		return -ENOTSUP;
	}
}

static int hy4245_init(const struct device *dev)
{
	const struct hy4245_config *cfg = dev->config;
	uint16_t chip_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_CHIPID, &chip_id);
	if (ret != 0) {
		return ret;
	}

	if (chip_id != HY4245_CHIPID) {
		LOG_ERR("unknown chip id %x", chip_id);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, hy4245_driver_api) = {
	.get_property = &hy4245_get_prop,
	.set_property = &hy4245_set_prop,
	.get_buffer_property = &hy4245_get_buffer_prop,
	.set_buffer_property = &hy4245_set_buffer_prop,
};

#define HY4245_INIT(index)                                                                         \
                                                                                                   \
	static const struct hy4245_config hy4245_config_##index = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
	};                                                                                         \
                                                                                                   \
	static struct hy4245_data hy4245_data_##index;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &hy4245_init, NULL, &hy4245_data_##index,                     \
			      &hy4245_config_##index, POST_KERNEL,                                 \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &hy4245_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HY4245_INIT)
