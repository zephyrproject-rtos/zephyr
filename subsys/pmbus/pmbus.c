/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/pmbus/pmbus.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pmbus, CONFIG_PMBUS_LOG_LEVEL);

static int pmbus_select_page(struct pmbus_dev *pmbus, uint8_t page, bool write)
{
	int ret;

	if (page == PMBUS_PAGE_ALL) {
		if (!write) {
			return -EINVAL;
		}
	} else if (page >= pmbus->pages) {
		return -EINVAL;
	}

	if (pmbus->pages == 1U || pmbus->page == page) {
		return 0;
	}

	ret = smbus_byte_data_write(pmbus->bus->bus, pmbus->bus->addr, PMBUS_PAGE, page);
	pmbus->page = (ret < 0) ? -1 : page;

	return ret;
}

static int pmbus_begin(struct pmbus_dev *pmbus, uint8_t page, bool write)
{
	int ret;

	k_mutex_lock(&pmbus->lock, K_FOREVER);

	ret = pmbus_select_page(pmbus, page, write);
	if (ret < 0) {
		k_mutex_unlock(&pmbus->lock);
	}

	return ret;
}

static int pmbus_end(struct pmbus_dev *pmbus, int ret)
{
	/* A failed transfer may have been seen by the device as a PAGE write */
	if (ret < 0) {
		pmbus->page = -1;
	}

	k_mutex_unlock(&pmbus->lock);

	return ret;
}

int pmbus_read_byte(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint8_t *val)
{
	int ret = pmbus_begin(pmbus, page, false);

	if (ret < 0) {
		return ret;
	}

	ret = smbus_byte_data_read(pmbus->bus->bus, pmbus->bus->addr, cmd, val);

	return pmbus_end(pmbus, ret);
}

int pmbus_read_word(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint16_t *val)
{
	int ret = pmbus_begin(pmbus, page, false);

	if (ret < 0) {
		return ret;
	}

	ret = smbus_word_data_read(pmbus->bus->bus, pmbus->bus->addr, cmd, val);

	return pmbus_end(pmbus, ret);
}

int pmbus_send_byte(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd)
{
	int ret = pmbus_begin(pmbus, page, true);

	if (ret < 0) {
		return ret;
	}

	ret = smbus_byte_write(pmbus->bus->bus, pmbus->bus->addr, cmd);

	return pmbus_end(pmbus, ret);
}

int pmbus_write_byte(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint8_t val)
{
	int ret = pmbus_begin(pmbus, page, true);

	if (ret < 0) {
		return ret;
	}

	ret = smbus_byte_data_write(pmbus->bus->bus, pmbus->bus->addr, cmd, val);
	if (ret == 0 && cmd == PMBUS_VOUT_MODE) {
		for (uint8_t i = 0U; i < pmbus->pages; i++) {
			if (page == PMBUS_PAGE_ALL || page == i) {
				pmbus->vout_mode[i] = val;
			}
		}
	}

	return pmbus_end(pmbus, ret);
}

int pmbus_write_word(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, uint16_t val)
{
	int ret = pmbus_begin(pmbus, page, true);

	if (ret < 0) {
		return ret;
	}

	ret = smbus_word_data_write(pmbus->bus->bus, pmbus->bus->addr, cmd, val);

	return pmbus_end(pmbus, ret);
}

bool pmbus_cmd_supported(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, size_t len)
{
	bool supported = false;
	uint8_t status;
	uint8_t cml;
	uint16_t word;
	int ret;

	if (len != 1U && len != 2U) {
		return false;
	}

	k_mutex_lock(&pmbus->lock, K_FOREVER);

	if (len == 1U) {
		ret = pmbus_read_byte(pmbus, page, cmd, &status);
	} else {
		ret = pmbus_read_word(pmbus, page, cmd, &word);
	}

	if (ret == 0) {
		supported = true;

		ret = pmbus_read_byte(pmbus, page, PMBUS_STATUS_BYTE, &status);
		if (ret < 0 || (status & PMBUS_STATUS_BYTE_CML) == 0U) {
			goto out;
		}

		ret = pmbus_read_byte(pmbus, page, PMBUS_STATUS_CML, &cml);
		if (ret == 0 && (cml & PMBUS_STATUS_CML_INVALID_CMD) != 0U) {
			supported = false;
		}
	}

	(void)pmbus_send_byte(pmbus, PMBUS_PAGE_ALL, PMBUS_CLEAR_FAULTS);
out:
	k_mutex_unlock(&pmbus->lock);

	return supported;
}

int pmbus_read_value(struct pmbus_dev *pmbus, uint8_t page, uint8_t cmd, int32_t *val)
{
	int mode = -1;
	uint16_t raw;
	int ret;

	switch (cmd) {
	case PMBUS_READ_VOUT:
		if (page < pmbus->pages) {
			mode = pmbus->vout_mode[page];
		}

		if (mode < 0 ||
		    FIELD_GET(PMBUS_VOUT_MODE_MODE_MASK, mode) != PMBUS_VOUT_MODE_ULINEAR16) {
			return -ENOTSUP;
		}
		break;
	case PMBUS_READ_VIN:
	case PMBUS_READ_IIN:
	case PMBUS_READ_IOUT:
	case PMBUS_READ_TEMPERATURE_1:
	case PMBUS_READ_TEMPERATURE_2:
	case PMBUS_READ_TEMPERATURE_3:
	case PMBUS_READ_POUT:
	case PMBUS_READ_PIN:
		break;
	default:
		return -ENOTSUP;
	}

	ret = pmbus_read_word(pmbus, page, cmd, &raw);
	if (ret < 0) {
		return ret;
	}

	if (cmd == PMBUS_READ_VOUT) {
		return pmbus_ulinear16_to_milli(
			raw, sign_extend(FIELD_GET(PMBUS_VOUT_MODE_PARAM_MASK, mode), 4), val);
	}

	return pmbus_linear11_to_milli(raw, val);
}

static int pmbus_check_pec(struct pmbus_dev *pmbus)
{
	uint32_t bus_config;
	bool dev_pec = (pmbus->capability & PMBUS_CAPABILITY_PEC) != 0U;

	if (smbus_get_config(pmbus->bus->bus, &bus_config) < 0) {
		return 0;
	}

	if ((bus_config & SMBUS_MODE_PEC) != 0U && !dev_pec) {
		LOG_ERR("0x%02x: PEC enabled on bus but not supported", pmbus->bus->addr);
		return -ENOTSUP;
	}

	if ((bus_config & SMBUS_MODE_PEC) == 0U && dev_pec) {
		LOG_INF("0x%02x: PEC supported but not enabled on bus", pmbus->bus->addr);
	}

	return 0;
}

int pmbus_init(struct pmbus_dev *pmbus, const struct smbus_dt_spec *bus, uint8_t pages)
{
	bool probe_failed = false;
	uint16_t status;
	uint8_t mode;
	int ret;

	if (!device_is_ready(bus->bus)) {
		return -ENODEV;
	}

	if (pages == 0U || pages > CONFIG_PMBUS_MAX_PAGES) {
		return -EINVAL;
	}

	pmbus->bus = bus;
	pmbus->pages = pages;
	pmbus->page = -1;
	pmbus->capability = 0U;
	k_mutex_init(&pmbus->lock);

	/* Faults latched before are cleared below, so report them first */
	for (uint8_t page = 0U; page < pages; page++) {
		ret = pmbus_read_word(pmbus, page, PMBUS_STATUS_WORD, &status);
		if (ret < 0) {
			LOG_DBG("0x%02x: no STATUS_WORD on page %u (%d)", bus->addr, page, ret);
			probe_failed = true;
		} else if (status != 0U) {
			LOG_WRN("0x%02x: STATUS_WORD 0x%04x on page %u", bus->addr, status, page);
		}
	}

	ret = pmbus_read_byte(pmbus, 0U, PMBUS_CAPABILITY, &pmbus->capability);
	if (ret < 0) {
		LOG_DBG("0x%02x: no CAPABILITY (%d)", bus->addr, ret);
		pmbus->capability = 0U;
		probe_failed = true;
	} else {
		if ((pmbus->capability & PMBUS_CAPABILITY_IEEE_HALF) != 0U) {
			LOG_ERR("0x%02x: IEEE half precision format not supported", bus->addr);
			return -ENOTSUP;
		}

		ret = pmbus_check_pec(pmbus);
		if (ret < 0) {
			return ret;
		}
	}

	for (uint8_t page = 0U; page < pages; page++) {
		ret = pmbus_read_byte(pmbus, page, PMBUS_VOUT_MODE, &mode);
		if (ret < 0) {
			LOG_DBG("0x%02x: no VOUT_MODE on page %u (%d)", bus->addr, page, ret);
			pmbus->vout_mode[page] = -1;
			probe_failed = true;
		} else {
			pmbus->vout_mode[page] = mode;
		}
	}

	if (probe_failed) {
		ret = pmbus_send_byte(pmbus, PMBUS_PAGE_ALL, PMBUS_CLEAR_FAULTS);
		if (ret < 0) {
			LOG_ERR("0x%02x: no response (%d)", bus->addr, ret);
			return ret;
		}
	}

	return 0;
}
