/*
 * Copyright (c) 2025 Orgatex GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq35100

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/fuel_gauge/bq35100_user.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include "bq35100.h"

LOG_MODULE_REGISTER(bq35100, CONFIG_FUEL_GAUGE_LOG_LEVEL);

#define BQ35100_MAC_DATA_LEN     32
#define BQ35100_MAC_OVERHEAD_LEN 4 /* 2 cmd bytes, 1 length byte, 1 checksum byte */
#define BQ35100_MAC_COMPLETE_LEN (BQ35100_MAC_DATA_LEN + BQ35100_MAC_OVERHEAD_LEN)

#define BQ35100_CNTL_DATA_LEN 2

static bq35100_security_t g_security_mode = SECURITY_UNKNOWN;

bq35100_security_t get_intern_sec_mode(void)
{
	return g_security_mode;
}

/* Function prototypes */
static int bq35100_set_security_mode(const struct device *dev, bq35100_security_t new_security);
int bq35100_get_status(const struct device *dev, uint16_t *status);

static uint8_t bq35100_compute_checksum(const uint8_t *data, size_t length)
{
	uint8_t checksum = 0;
	uint8_t x = 0;

	if (data) {
		for (x = 1; x <= length; x++) {
			checksum += *data;
			data++;
		}

		checksum = 0xFF - checksum;
	}

	return checksum;
}

static int bq35100_write(const struct device *dev, const uint8_t *data, size_t len)
{
	int rc;
	/* Pointer to the device's configuration */
	const struct bq35100_config *cfg = dev->config;

	LOG_HEXDUMP_DBG(data, len, "dev write");

	/* Write the buffer to the device over I2C */
	rc = i2c_write_dt(&cfg->i2c, data, len);
	if (rc) {
		LOG_ERR("Failed to write I2C-data, error: %d", rc);
	}

	return rc;
}

static int bq35100_read(const struct device *dev, uint8_t *write_data, size_t write_len,
			uint8_t *read_data, size_t read_len)
{
	int err;

	/* Pointer to the device's configuration */
	const struct bq35100_config *cfg = dev->config;

	/* Write-Read the buffer of the device over I2C */

	err = i2c_write_dt(&cfg->i2c, write_data, write_len);
	if (err) {
		LOG_ERR("Unable to write data for I2C-read, error: %d", err);
		return err;
	}

	err = i2c_read_dt(&cfg->i2c, read_data, read_len);
	if (err) {
		LOG_ERR("Failed to read I2C-data, error: %d", err);
	} else {
		LOG_HEXDUMP_DBG(read_data, read_len, "dev read");
	}

	return 0;
}

static int bq35100_send_data(const struct device *dev, uint8_t address, uint8_t *data, size_t len)
{
	uint8_t buffer[BQ35100_CNTL_DATA_LEN + 1];

	buffer[0] = address;
	memcpy(buffer + 1, data, len);

	return bq35100_write(dev, buffer, ARRAY_SIZE(buffer));
}

int bq35100_get_data(const struct device *dev, uint8_t address, uint8_t *data, size_t len)
{
	uint8_t write_buffer = address;

	return bq35100_read(dev, &write_buffer, 1, data, len);
}

int bq35100_send_cntl(const struct device *dev, uint16_t cntl_address)
{
	uint8_t buffer[2];

	sys_put_le16(cntl_address, buffer);

	return bq35100_send_data(dev, BQ35100_REG_CONTROL_STATUS, buffer, ARRAY_SIZE(buffer));
}

static int bq35100_get_cntl(const struct device *dev, uint16_t cntl_address, uint16_t *data)
{
	int err;
	uint8_t buffer[2];

	if (data == NULL) {
		LOG_ERR("CNTL return buffer invalid");
		return -EINVAL;
	}

	sys_put_le16(cntl_address, buffer);

	err = bq35100_send_data(dev, BQ35100_REG_CONTROL_STATUS, buffer, ARRAY_SIZE(buffer));
	if (err) {
		return err;
	}

	err = bq35100_get_data(dev, BQ35100_REG_CONTROL_STATUS, buffer, 2);
	if (err) {
		return err;
	}

	err = bq35100_get_data(dev, BQ35100_REG_MAC_DATA, buffer, ARRAY_SIZE(buffer));
	if (err) {
		return err;
	}

	*data = sys_get_le16(buffer);

	return err;
}

int bq35100_write_extended_data(const struct device *dev, const uint16_t address,
				const uint8_t *data, size_t len)
{
	int err;
	uint16_t answer;
	char buffer[BQ35100_MAC_DATA_LEN + 3]; /* Max data len + header */

	if (g_security_mode == SECURITY_UNKNOWN) {
		LOG_ERR("Security mode unknown");
		return -EINVAL;
	}

	if (address < 0x4000 || address > 0x43FF || len < 1 || len > 32 || !data) {
		LOG_ERR("Invalid input data");
		return -EINVAL;
	}

	if (g_security_mode == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	LOG_DBG("Preparing to write %u byte(s) to address 0x%04X", len, address);
	LOG_HEXDUMP_DBG(data, len, "Payload");

	buffer[0] = BQ35100_REG_MAC;
	sys_put_le16(address, &buffer[1]);

	memcpy(buffer + 3, data, len);

	err = bq35100_write(dev, buffer, 3 + len);
	if (err) {
		LOG_ERR("Unable to write to ManufacturerAccessControl");
		return err;
	}

	k_sleep(K_MSEC(CONFIG_BQ35100_FLASH_WRITE_DELAY));

	/* Compute the checksum and write it to BQ35100_REG_MAC_DATA_SUM (0x60)
	 * and with autoincrement write 4 + len to BQ35100_REG_MAC_DATA_LEN (0x61)
	 */
	buffer[0] = BQ35100_REG_MAC_DATA_SUM;
	buffer[1] = bq35100_compute_checksum(buffer + 1, len + 2);
	buffer[2] = len + 4;

	err = bq35100_write(dev, buffer, 3);
	if (err) {
		LOG_ERR("Unable to write to BQ35100_REG_MAC_DATA_SUM");
		return err;
	}

	k_sleep(K_MSEC(CONFIG_BQ35100_FLASH_WRITE_DELAY));

	err = bq35100_get_status(dev, &answer);
	if (err) {
		return err;
	}

	if (BQ35100_FLASH_FAIL_BIT_MASK & answer) {
		LOG_ERR("Write failed");
		return err;
	}

	LOG_DBG("Write successful");

	return 0;
}

int bq35100_read_extended_data(const struct device *dev, const uint16_t address, uint8_t *data,
			       size_t len)
{
	size_t length_read;
	uint8_t buffer[BQ35100_MAC_COMPLETE_LEN];
	uint8_t write_buffer;
	int err;

	if (g_security_mode == SECURITY_UNKNOWN) {
		LOG_ERR("Security mode unknown");
		return -EINVAL;
	}

	if (address < 0x4000 || address > 0x43FF || !data) {
		LOG_ERR("Invalid input data");
		return -EINVAL;
	}

	if (g_security_mode == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	LOG_DBG("Preparing to read %u byte(s) from address 0x%04X", len, address);

	sys_put_le16(address, buffer);

	err = bq35100_send_data(dev, BQ35100_REG_MAC, buffer, 2);
	if (err) {
		LOG_ERR("Unable to write address to ManufacturerAccessControl");
		return -EIO;
	}

	k_msleep(1000);

	write_buffer = BQ35100_REG_MAC;

	err = bq35100_read(dev, &write_buffer, 1, buffer, ARRAY_SIZE(buffer));

	/* Check that the address matches */
	if (buffer[0] != (char)address || buffer[1] != (char)(address >> 8)) {
		LOG_ERR("Address didn't match (expected 0x%04X, received 0x%02X%02X)", address,
			buffer[1], buffer[0]);
		return -EINVAL;
	}

	/* Check that the checksum matches (-2 on BQ35100_REG_MAC_DATA_LEN as it includes
	 * BQ35100_REG_MAC_DATA_SUM and itself)
	 */
	if (buffer[34] != bq35100_compute_checksum(buffer, buffer[35] - 2)) {
		LOG_ERR("Checksum didn't match (0x%02X expected)", buffer[34]);
		return -EINVAL;
	}

	/* All is good */
	length_read =
		buffer[35] - 4; /* -4 rather than -2 to remove the two bytes of address as well */

	if (length_read > len) {
		length_read = len;
	}

	memcpy(data, buffer + 2, length_read);

	LOG_HEXDUMP_DBG(buffer, length_read, "data read");

	return 0;
}

int bq35100_get_status(const struct device *dev, uint16_t *status)
{
	int ret;
	uint8_t data[2];

	if (dev == NULL || status == NULL) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	LOG_DBG("Reading device-status");

	ret = bq35100_get_data(dev, BQ35100_REG_CONTROL_STATUS, data, sizeof(data));
	if (ret != 0) {
		LOG_ERR("Failed to read device status");
		return ret;
	}

	*status = sys_le16_to_cpu(UNALIGNED_GET((uint16_t *)data));

	return 0;
}

static int bq35100_wait_for_status(const struct device *dev, uint16_t expected, uint16_t mask,
				   k_timeout_t millis)
{
	uint16_t answer;

	for (int i = 0; i < CONFIG_BQ35100_MAX_RETRIES; i++) {
		if (!bq35100_get_status(dev, &answer)) {
			return false;
		}

		if ((answer & mask) == expected) {
			LOG_DBG("Status match");
			return true;

		} else {
			LOG_WRN("Status not yet in requested state read: %04X expected: %04X",
				answer, expected);
			k_sleep(millis);
		}
	}

	LOG_ERR("Status not in requested state, read: %04X expected: %04X", answer, expected);

	return -EINVAL;
}

static int bq35100_get_security_mode(const struct device *dev, bq35100_security_t *current_security)
{
	int err;
	uint16_t buffer;
	uint8_t extracted_security;

	LOG_DBG("Reading security-mode");

	err = bq35100_get_status(dev, &buffer);
	if (err) {
		LOG_ERR("Failed to get security mode");
		return -EIO;
	}

	extracted_security = FIELD_GET(BIT_MASK(2) << 13, buffer);

	switch (extracted_security) {
	case SECURITY_FULL_ACCESS:
		LOG_DBG("Device is in FULL ACCESS mode");
		break;

	case SECURITY_UNSEALED:
		LOG_DBG("Device is in UNSEALED mode");
		break;

	case SECURITY_SEALED:
		LOG_DBG("Device is in SEALED mode");
		break;

	default:
		LOG_ERR("Invalid device mode");
		return SECURITY_UNKNOWN;
	}

	*current_security = (bq35100_security_t)extracted_security;

	return 0;
}

int bq35100_set_security_mode(const struct device *dev, bq35100_security_t new_security)
{
	int err = -EINVAL;
	char buffer[4];

	if (new_security == g_security_mode) {
		return 0; /* We are already in this mode */
	}

	if (new_security == SECURITY_UNKNOWN) {
		LOG_ERR("Invalid access mode");
		return -EINVAL;
	}

	/*
	 * For reasons that aren't clear, the BQ35100 sometimes refuses to change security mode
	 * if a previous security mode change happened only a few seconds ago, hence the retry
	 * here.
	 */
	for (int i = 0; (i < CONFIG_BQ35100_MAX_RETRIES) && err != 0; i++) {
		buffer[0] = BQ35100_REG_MAC;

		switch (new_security) {
		case SECURITY_SEALED: {
			LOG_DBG("Setting security to SEALED");
			err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_SEALED);
			if (err) {
				LOG_ERR("Unable to set SECURITY_SEALED");
				return err;
			}
			break;
		}

		case SECURITY_FULL_ACCESS: {
			/* Unseal first if in Sealed mode */
			if (g_security_mode == SECURITY_SEALED) {
				err = bq35100_set_security_mode(dev, SECURITY_UNSEALED);
				if (err) {
					LOG_ERR("Unable to set SECURITY_UNSEALED");
					return err;
				}
			}

			err = bq35100_read_extended_data(dev, BQ35100_FLASH_FULL_ACCESS_CODES,
							 buffer, ARRAY_SIZE(buffer));
			if (err) {
				LOG_ERR("Could not get full access codes");
				return err;
			}

			uint32_t full_access_codes = sys_get_be32(buffer);

			LOG_DBG("Setting security to FULL ACCESS");

			/* Send the full access code with endianness conversion in TWO writes */
			sys_put_le16(full_access_codes >> 16, &buffer[1]);
			err = bq35100_write(dev, buffer, 3);
			if (err) {
				LOG_ERR("Unable to send first part of full access key");
				return err;
			}

			sys_put_le16(full_access_codes, &buffer[1]);
			err = bq35100_write(dev, buffer, 3);
			if (err) {
				LOG_ERR("Unable to send first part of full access key");
				return err;
			}
			break;
		}

		case SECURITY_UNSEALED: {
			/* Seal first if in Full Access mode */
			if (g_security_mode == SECURITY_FULL_ACCESS) {
				err = bq35100_set_security_mode(dev, SECURITY_SEALED);
				if (err) {
					LOG_ERR("Unable to set SECURITY_SEALED");
					return err;
				}
			}

			LOG_DBG("Setting security to UNSEALED");

			buffer[0] = BQ35100_REG_CONTROL_STATUS;
			/* Send the unsealed code with endianness conversion in TWO writes */
			sys_put_le16(BQ35100_DEFAULT_SEAL_CODES >> 16, &buffer[1]);
			err = bq35100_write(dev, buffer, 3);
			if (err) {
				LOG_ERR("Unable to send first part of unsealed key");
				return err;
			}

			k_sleep(K_MSEC(CONFIG_BQ35100_FLASH_WRITE_DELAY));

			/* buffer[0] = BQ35100_REG_MAC; */
			sys_put_le16((uint16_t)BQ35100_DEFAULT_SEAL_CODES, &buffer[1]);
			err = bq35100_write(dev, buffer, 3);
			if (err) {
				LOG_ERR("Unable to send first part of unsealed key");
				return err;
			}
			break;
		}

		case SECURITY_UNKNOWN:
		default: {
			LOG_ERR("Unknown security mode");

			break;
		}
		}

		k_msleep(40);

		err = bq35100_get_security_mode(dev, &g_security_mode);
		if (err) {
			LOG_ERR("Unable to verifiy security mode");
			return err;
		}

		if (g_security_mode == new_security) {
			LOG_DBG("Security mode set");
		} else {
			LOG_ERR("Security mode set failed (wanted 0x%02X, got 0x%02X)",
				new_security, g_security_mode);
			err = -EIO;
			k_msleep(40);
		}
	}
	return err;
}

static int bq35100_get_device_type(const struct device *dev, uint16_t *type)
{
	if (type == NULL) {
		LOG_ERR("Invalid device-type buffer");
		return -EINVAL;
	}

	LOG_DBG("Reading device-type");

	int err = bq35100_get_cntl(dev, BQ35100_MAC_CMD_DEVICETYPE, type);

	if (err) {
		LOG_ERR("Unable to get control status");
		return err;
	}

	return 0;
}

static int bq35100_set_design_capacity(const struct device *dev, const uint16_t new_design_capacity)
{
	uint8_t buffer[2];

	sys_put_be16(new_design_capacity, buffer);

	return bq35100_write_extended_data(dev, BQ35100_FLASH_CMD_SET_NEW_CAPACITY, buffer,
					   ARRAY_SIZE(buffer));
}

static int bq35100_start_gauge(const struct device *dev)
{
	int err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_GAUGE_START);

	if (err) {
		LOG_ERR("Error enabling gauge: %d", err);
		return err;
	}

	err = bq35100_wait_for_status(dev, BQ35100_GA_BIT_MASK, BQ35100_GA_BIT_MASK, K_MSEC(500));

	if (err) {
		LOG_ERR("Error enabling gauge: %d", err);
		return err;
	}

	return 0;
}

static int bq35100_stop_gauge(const struct device *dev)
{
	int err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_GAUGE_STOP);

	if (err) {
		LOG_ERR("Error disabling gauge: %d", err);
		return err;
	}

	err = bq35100_wait_for_status(dev, 0, BQ35100_GA_BIT_MASK, K_MSEC(500));

	if (err) {
		LOG_ERR("Error disabling gauge: %d", err);
		return err;
	}

	return 0;
}

static int bq35100_set_new_battery(const struct device *dev, const uint16_t new_design_capacity)
{
	int err = bq35100_set_design_capacity(dev, new_design_capacity);

	if (err) {
		LOG_ERR("Error setting new design-capacity: %d", err);
		return err;
	}

	k_sleep(K_MSEC(CONFIG_BQ35100_FLASH_WRITE_DELAY * 8));

	err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_NEW_BATTERY);
	if (err) {
		LOG_ERR("Error setting new battery: %d", err);
		return err;
	}

	k_sleep(K_MSEC(CONFIG_BQ35100_NEW_BATTERY_DELAY));

	return 0;
}

static int bq35100_bat_alert_conf(const struct device *dev, const uint16_t threshold_volt)
{
	int err;

	uint8_t buffer[2];
	uint8_t bat_low = (1 << 7);

	if (g_security_mode == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	sys_put_be16(threshold_volt, buffer);

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_BAT_LOW_VOLTAGE_THRESHOLD, buffer,
					  ARRAY_SIZE(buffer));
	if (err < 0) {
		LOG_ERR("Failed to set low voltage threshold");
		return err;
	}

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_ALERT_CONFIG, &bat_low,
					  sizeof(bat_low));
	if (err < 0) {
		LOG_ERR("Failed to set bat low voltage alert config");
		return err;
	}

	return 0;
}

static int bq35100_reset(const struct device *dev)
{
	int err;

	if (g_security_mode == SECURITY_SEALED) {
		LOG_ERR("Invalid security mode sealed set");
		return -EINVAL;
	}

	err = bq35100_send_cntl(dev, BQ35100_MAC_CMD_RESET);
	if (err) {
		LOG_ERR("Unable to reset device");
		return err;
	}

	return 0;
}

static uint32_t calculate_remaining_capacity(uint16_t design_capacity, int32_t accumulated_capacity)
{
	uint32_t design_capacity_uah = (uint32_t)design_capacity * 1000;

	uint32_t remaining_capacity = design_capacity_uah + accumulated_capacity;

	if (remaining_capacity > design_capacity_uah) {
		remaining_capacity = design_capacity_uah;
	}

	return remaining_capacity;
}

static int32_t bq35100_process_prop(const fuel_gauge_prop_t prop, const uint8_t *const buffer,
				    union fuel_gauge_prop_val *const val)
{
	int32_t err = 0;

	if (!buffer || !val) {
		return -EINVAL;
	}

	switch (prop) {
	case FUEL_GAUGE_VOLTAGE_UV:
		/* Register unit: mV */
		val->voltage_uv = (int32_t)sys_get_le16(buffer) * 1000;
		break;
	case FUEL_GAUGE_CURRENT_UA:
		/* Register unit: mA */
		val->current_ua = (int32_t)(int16_t)sys_get_le16(buffer) * 1000;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		val->design_cap = (uint16_t)sys_get_le16(buffer);
		break;
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT:
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH: {
		/* Both cases use the same starting point */
		int32_t accumulated = (uint32_t)sys_get_le32(buffer);
		uint16_t design = (uint16_t)sys_get_le16(&buffer[4]);
		uint32_t remaining = calculate_remaining_capacity(design, accumulated);

		if (prop == FUEL_GAUGE_REMAINING_CAPACITY_UAH) {
			/* Required unit: µAh */
			val->remaining_capacity_uah = remaining;
		} else {
			/* FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT */
			/* Required unit: %, calculated from remaining and design capacity */
			if (design <= 0) {
				val->absolute_state_of_charge_pct = 0;
			} else {
				val->absolute_state_of_charge_pct =
					(remaining * 100) / ((uint32_t)design * 1000);
			}
		}
		break;
	}
	case FUEL_GAUGE_BQ35100_BATTERY_STATUS_ALERT:
		LOG_DBG("Battery alert reg: %d battery status reg: %d", buffer[0], buffer[1]);
		break;
	default:
		err = -ENOTSUP;
		break;
	}

	return err;
}

static uint16_t bq35100_get_register(const fuel_gauge_prop_t prop)
{
	uint16_t reg = 0U;

	switch (prop) {
	case FUEL_GAUGE_VOLTAGE_UV:
		reg = BQ35100_REG_VOLTAGE;
		break;
	case FUEL_GAUGE_CURRENT_UA:
		reg = BQ35100_REG_CURRENT;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		reg = BQ35100_REG_DESIGN_CAPACITY;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT:
		reg = BQ35100_REG_ACCUMULATED_CAPACITY;
		break;
	default:
		/* Keep reg as 0 for unsupported properties */
		break;
	}

	return reg;
}

static int bq35100_read_registers(const struct device *const dev, const fuel_gauge_prop_t prop,
				  uint8_t *const buffer)
{
	int err;
	uint8_t address;

	if (!buffer) {
		return -EINVAL;
	}

	address = (uint8_t)bq35100_get_register(prop);

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	int pm_err = pm_device_runtime_get(dev);

	if (pm_err < 0) {
		LOG_ERR("Failed to resume device: %d", pm_err);
	}
#endif

	switch (prop) {
	case FUEL_GAUGE_VOLTAGE_UV:
	case FUEL_GAUGE_CURRENT_UA:
	case FUEL_GAUGE_DESIGN_CAPACITY:
		err = bq35100_get_data(dev, address, buffer, sizeof(uint16_t));
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT:
		err = bq35100_get_data(dev, BQ35100_REG_ACCUMULATED_CAPACITY, buffer,
				       sizeof(uint32_t));
		if (err == 0) {
			err = bq35100_get_data(dev, BQ35100_REG_DESIGN_CAPACITY,
					       &buffer[sizeof(uint32_t)], sizeof(uint16_t));
		}
		break;
	case FUEL_GAUGE_BQ35100_BATTERY_STATUS_ALERT:
		err = bq35100_get_data(dev, BQ35100_REG_BATTERY_STATUS, &buffer[1],
				       sizeof(uint8_t));
		err = bq35100_get_data(dev, BQ35100_REG_BATTERY_ALERT, buffer, sizeof(uint8_t));
		break;
	default:
		err = -ENOTSUP;
		break;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	pm_err = pm_device_runtime_put_async(dev, K_NO_WAIT);
	if (pm_err < 0) {
		LOG_ERR("Failed to suspend device: %d", pm_err);
	}
#endif

	return err;
}

static int bq35100_get_prop(const struct device *const dev, const fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *const val)
{
	int err;
	uint8_t buffer[6]; /* Max size needed for ABSOLUTE_STATE_OF_CHARGE */

	if ((val == NULL) || (prop > FUEL_GAUGE_PROP_MAX)) {
		return -EINVAL;
	}

	err = bq35100_read_registers(dev, prop, buffer);
	if (err < 0) {
		LOG_ERR("Failed to read register for prop");
		return err;
	}

	err = bq35100_process_prop(prop, buffer, val);
	if (err < 0) {
		LOG_ERR("Failed to process prop");
		return err;
	}

	return 0;
}

static int bq35100_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	int err;

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	int pm_err = pm_device_runtime_get(dev);

	if (pm_err < 0) {
		LOG_ERR("Failed to resume device: %d", pm_err);
	}
#endif

	switch (prop) {
	case FUEL_GAUGE_DESIGN_CAPACITY:
		LOG_DBG("Setting design capacity");
		err = bq35100_set_design_capacity(dev, val.design_cap);
		break;
	case FUEL_GAUGE_BQ35100_NEW_BATTERY:
		LOG_DBG("Setting new-battery");
		err = bq35100_set_new_battery(dev, val.design_cap);
		break;
	case FUEL_GAUGE_BQ35100_RESET:
		LOG_DBG("Resetting BQ35100");
		err = bq35100_reset(dev);
		break;
	case FUEL_GAUGE_BQ35100_START:
		LOG_DBG("Setting Gauge-Start");
		err = bq35100_start_gauge(dev);
		break;
	case FUEL_GAUGE_BQ35100_STOP:
		LOG_DBG("Setting Gauge-Stop");
		err = bq35100_stop_gauge(dev);
		break;
	case FUEL_GAUGE_BQ35100_SEC_MODE_SEALED:
		LOG_DBG("Setting sec mode sealed");
		err = bq35100_set_security_mode(dev, SECURITY_SEALED);
		break;
	case FUEL_GAUGE_BQ35100_SEC_MODE_UNSEALED:
		LOG_DBG("Setting sec mode unsealed");
		err = bq35100_set_security_mode(dev, SECURITY_UNSEALED);
		break;
	case FUEL_GAUGE_BQ35100_ALERT_CONF:
		LOG_DBG("Enter battery alert configuration");
		err = bq35100_bat_alert_conf(dev, val.design_volt_mv);
		break;
	default:
		err = -ENOTSUP;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	pm_err = pm_device_runtime_put_async(dev, K_NO_WAIT);
	if (pm_err < 0) {
		LOG_ERR("Failed to suspend device: %d", pm_err);
	}
#endif

	return err;
}

static int bq35100_init(const struct device *dev)
{
	int err;
	const struct bq35100_config *cfg = dev->config;
	uint16_t device_type = 0;
	uint16_t status = 0;
	uint8_t extracted_security;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (gpio_is_ready_dt(&cfg->supply_gpio)) {
		err = gpio_pin_configure_dt(&cfg->supply_gpio, GPIO_OUTPUT);
		if (err < 0) {
			LOG_ERR("Failed to configure GE pin as output");
			return -EIO;
		}

		err = gpio_pin_set_dt(&cfg->supply_gpio, 1);
		if (err < 0) {
			LOG_ERR("Failed to set GE pin high");
			return -EIO;
		}

		k_msleep(500);
	}

	if (bq35100_wait_for_status(dev, BQ3500_INITCOMP_BIT_MASK, BQ3500_INITCOMP_BIT_MASK,
				    K_MSEC(500))) {
		LOG_ERR("Device initialization failed");
		return -ENODEV;
	}

	err = bq35100_get_device_type(dev, &device_type);
	if (err) {
		LOG_ERR("Reading device-type failed");
		return -ENODEV;
	}

	if (device_type != BQ35100_DEVICE_TYPE) {
		LOG_ERR("Devicetype mismatch! Expected: %d, Received: %d", BQ35100_DEVICE_TYPE,
			device_type);
		return -ENODEV;
	}

	extracted_security = BQ35100_SEC_MODE_BIT_MASK & status;

	switch (extracted_security) {
	case SECURITY_FULL_ACCESS:
		LOG_DBG("Device is in FULL ACCESS mode");
		break;

	case SECURITY_UNSEALED:
		LOG_DBG("Device is in UNSEALED mode");
		break;

	case SECURITY_SEALED:
		LOG_DBG("Device is in SEALED mode");
		break;

	default:
		LOG_DBG("Invalid device mode");
		break;
	}

	g_security_mode = extracted_security;

	if (g_security_mode != SECURITY_UNSEALED) {
		err = bq35100_set_security_mode(dev, SECURITY_UNSEALED);
		if (err) {
			LOG_ERR("Unable to set SECURITY_UNSEALED");
			return err;
		}
	}

	/* acc mode */
	uint8_t gmsel = 0b00;
	/* set V ext mode*/
	uint8_t mode = 0x40;

	uint8_t op = gmsel | mode;

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_OPERATION_CONFIG_A, &op, sizeof(op));
	if (err < 0) {
		LOG_ERR("Failed to set operation config a register");
	}

	uint8_t cell_count = CONFIG_BQ35100_SERIES_CELL_COUNT;

	err = bq35100_write_extended_data(dev, BQ35100_FLASH_SERIES_CELL_COUNT, &cell_count,
					  sizeof(cell_count));
	if (err < 0) {
		LOG_ERR("Failed to set series cell count register");
	}

	LOG_INF("BQ35100 with device-type %04X initialized", device_type);

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int bq35100_set_ge_pin(const struct device *dev, bool state)
{
	const struct bq35100_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->supply_gpio)) {
		return -ENOTSUP;
	}

	const int err = gpio_pin_set_dt(&cfg->supply_gpio, state);

	if (err < 0) {
		LOG_ERR("Failed to set GE pin to %d", state);
		return -EIO;
	}

	LOG_DBG("GE pin set to %d", state);

	return 0;
}

static int bq35100_pm_action(const struct device *dev, enum pm_device_action action)
{
	int rc;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		rc = bq35100_set_ge_pin(dev, 1);
		if (rc < 0) {
			LOG_ERR("Failed to set ge pin high");
			return rc;
		}

		k_msleep(500);
		if (bq35100_wait_for_status(dev, BQ3500_INITCOMP_BIT_MASK, BQ3500_INITCOMP_BIT_MASK,
					    K_MSEC(500))) {
			LOG_ERR("Device initialization failed");
			return -ENODEV;
		}

		rc = bq35100_start_gauge(dev);
		if (rc < 0) {
			LOG_ERR("Failed to start gauge");
			return rc;
		}
		k_msleep(2000);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		rc = bq35100_stop_gauge(dev);
		if (rc < 0) {
			LOG_ERR("Failed to stop gauge");
			return rc;
		}

		k_msleep(500);
		if (bq35100_wait_for_status(dev, BQ35100_G_DONE_BIT_MASK, BQ35100_G_DONE_BIT_MASK,
					    K_MSEC(500))) {
			LOG_ERR("Device failed to complete all tasks before powering down");
			return -ENODEV;
		}

		rc = bq35100_set_ge_pin(dev, 0);
		if (rc < 0) {
			LOG_ERR("Failed to set ge pin low");
			return rc;
		}
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(fuel_gauge, bq35100_driver_api) = {
	.get_property = &bq35100_get_prop,
	.set_property = &bq35100_set_prop,
};

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "BQ35100 device is not defined in DTS"
#endif

#define BQ35100_INIT(index)                                                                        \
                                                                                                   \
	static const struct bq35100_config bq35100_config_##index = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.supply_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(index), supply_gpios, {0}),         \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(index, bq35100_pm_action);                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &bq35100_init, PM_DEVICE_DT_INST_GET(index), NULL,            \
			      &bq35100_config_##index, POST_KERNEL,                                \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &bq35100_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ35100_INIT)
