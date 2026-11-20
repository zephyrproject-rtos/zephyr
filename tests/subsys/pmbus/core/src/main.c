/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/pmbus/pmbus.h>
#include <zephyr/ztest.h>

#include "fake_pmbus_dev.h"

#define FAKE_ADDR  DT_PHA_BY_IDX(DT_NODELABEL(i2c0), forwards, 0, addr)
#define FAKE_PAGES 2

static const struct device *const smbus = DEVICE_DT_GET(DT_NODELABEL(smbus0));
static const struct smbus_dt_spec spec = {.bus = smbus, .addr = FAKE_ADDR};

static struct fake_pmbus_dev fake;

static struct pmbus_dev dev;

static void fake_defaults(struct fake_pmbus_dev *fake)
{
	fake_pmbus_dev_reset(fake);
	for (int i = 0; i < fake->pages; i++) {
		fake->regs[i][PMBUS_CAPABILITY] = PMBUS_CAPABILITY_PEC;
		fake->regs[i][PMBUS_VOUT_MODE] = 0x14;
	}
}

ZTEST(pmbus_core, test_init)
{
	zassert_ok(pmbus_init(&dev, &spec, 1));
	zassert_equal(dev.capability, PMBUS_CAPABILITY_PEC);
	zassert_equal(dev.vout_mode[0], 0x14);
	zassert_equal(fake.page_writes, 0);
	zassert_equal(fake.clear_faults, 0);
}

ZTEST(pmbus_core, test_init_status)
{
	fake.regs[0][PMBUS_STATUS_WORD] = PMBUS_STATUS_WORD_VOUT;
	zassert_ok(pmbus_init(&dev, &spec, 1));
	zassert_equal(fake.clear_faults, 0);

	atomic_set_bit(fake.nack, PMBUS_STATUS_WORD);
	zassert_ok(pmbus_init(&dev, &spec, FAKE_PAGES));
	zassert_equal(fake.clear_faults, 1);
	zassert_equal(fake.cml, 0);
}

ZTEST(pmbus_core, test_init_invalid)
{
	zassert_equal(pmbus_init(&dev, &spec, 0), -EINVAL);
	zassert_equal(pmbus_init(&dev, &spec, CONFIG_PMBUS_MAX_PAGES + 1), -EINVAL);
}

ZTEST(pmbus_core, test_init_optional_commands)
{
	atomic_set_bit(fake.nack, PMBUS_CAPABILITY);
	atomic_set_bit(fake.nack, PMBUS_VOUT_MODE);

	zassert_ok(pmbus_init(&dev, &spec, 1));
	zassert_equal(dev.capability, 0);
	zassert_equal(dev.vout_mode[0], -1);
	zassert_equal(fake.clear_faults, 1);
	zassert_equal(fake.cml, 0);
}

ZTEST(pmbus_core, test_init_ieee_half)
{
	fake.regs[0][PMBUS_CAPABILITY] = PMBUS_CAPABILITY_IEEE_HALF;

	zassert_equal(pmbus_init(&dev, &spec, 1), -ENOTSUP);
}

ZTEST(pmbus_core, test_init_absent)
{
	fake.absent = true;

	zassert_equal(pmbus_init(&dev, &spec, 1), -EIO);
}

ZTEST(pmbus_core, test_pec)
{
	uint16_t word;

	zassert_ok(smbus_configure(smbus, SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC));

	fake.pec = true;
	zassert_ok(pmbus_init(&dev, &spec, 1));
	fake.regs[0][PMBUS_READ_VIN] = 0xF0C8;
	zassert_ok(pmbus_read_word(&dev, 0, PMBUS_READ_VIN, &word));
	zassert_equal(word, 0xF0C8);
	zassert_ok(pmbus_write_word(&dev, 0, PMBUS_VIN_ON, 0x1234));
	zassert_equal(fake.regs[0][PMBUS_VIN_ON], 0x1234);
	zassert_equal(fake.cml, 0);

	fake.regs[0][PMBUS_CAPABILITY] = 0;
	zassert_equal(pmbus_init(&dev, &spec, 1), -ENOTSUP);

	fake.pec = false;
	zassert_equal(pmbus_read_word(&dev, 0, PMBUS_READ_VIN, &word), -EIO);
}

ZTEST(pmbus_core, test_page_cache)
{
	uint16_t word;

	zassert_ok(pmbus_init(&dev, &spec, FAKE_PAGES));
	zassert_equal(dev.vout_mode[1], 0x14);

	fake.page_writes = 0;
	fake.regs[0][PMBUS_READ_VOUT] = 0x1111;
	fake.regs[1][PMBUS_READ_VOUT] = 0x2222;

	zassert_ok(pmbus_read_word(&dev, 1, PMBUS_READ_VOUT, &word));
	zassert_equal(word, 0x2222);
	zassert_ok(pmbus_read_word(&dev, 1, PMBUS_READ_VOUT, &word));
	zassert_equal(fake.page_writes, 0);

	zassert_ok(pmbus_read_word(&dev, 0, PMBUS_READ_VOUT, &word));
	zassert_equal(word, 0x1111);
	zassert_equal(fake.page_writes, 1);

	atomic_set_bit(fake.nack, PMBUS_READ_IOUT);
	zassert_equal(pmbus_read_word(&dev, 0, PMBUS_READ_IOUT, &word), -EIO);
	zassert_ok(pmbus_read_word(&dev, 0, PMBUS_READ_VOUT, &word));
	zassert_equal(fake.page_writes, 2);
}

ZTEST(pmbus_core, test_page_invalid)
{
	uint16_t word;
	uint8_t byte;

	zassert_ok(pmbus_init(&dev, &spec, FAKE_PAGES));
	zassert_equal(pmbus_read_word(&dev, 2, PMBUS_READ_VOUT, &word), -EINVAL);
	zassert_equal(pmbus_read_byte(&dev, PMBUS_PAGE_ALL, PMBUS_STATUS_BYTE, &byte), -EINVAL);

	zassert_ok(pmbus_init(&dev, &spec, 1));
	zassert_equal(pmbus_read_word(&dev, 1, PMBUS_READ_VOUT, &word), -EINVAL);
}

ZTEST(pmbus_core, test_page_all)
{
	zassert_ok(pmbus_init(&dev, &spec, FAKE_PAGES));

	zassert_ok(pmbus_write_word(&dev, PMBUS_PAGE_ALL, PMBUS_VOUT_COMMAND, 0x1234));
	zassert_equal(fake.regs[0][PMBUS_VOUT_COMMAND], 0x1234);
	zassert_equal(fake.regs[1][PMBUS_VOUT_COMMAND], 0x1234);

	zassert_ok(pmbus_send_byte(&dev, PMBUS_PAGE_ALL, PMBUS_CLEAR_FAULTS));
	zassert_equal(fake.clear_faults_page, PMBUS_PAGE_ALL);
}

ZTEST(pmbus_core, test_page_all_single_page)
{
	zassert_ok(pmbus_init(&dev, &spec, 1));
	zassert_ok(pmbus_send_byte(&dev, PMBUS_PAGE_ALL, PMBUS_CLEAR_FAULTS));
	zassert_equal(fake.page_writes, 0);
	zassert_equal(fake.clear_faults, 1);
}

ZTEST(pmbus_core, test_byte_word)
{
	uint16_t word;
	uint8_t byte;

	zassert_ok(pmbus_init(&dev, &spec, 1));

	zassert_ok(pmbus_write_byte(&dev, 0, PMBUS_OPERATION, 0x80));
	zassert_equal(fake.regs[0][PMBUS_OPERATION], 0x80);
	zassert_ok(pmbus_read_byte(&dev, 0, PMBUS_OPERATION, &byte));
	zassert_equal(byte, 0x80);

	fake.regs[0][PMBUS_STATUS_WORD] = PMBUS_STATUS_WORD_VOUT;
	zassert_ok(pmbus_read_word(&dev, 0, PMBUS_STATUS_WORD, &word));
	zassert_equal(word, PMBUS_STATUS_WORD_VOUT);

	zassert_ok(pmbus_write_word(&dev, 0, PMBUS_VIN_ON, 0xCAFE));
	zassert_equal(fake.regs[0][PMBUS_VIN_ON], 0xCAFE);
}

ZTEST(pmbus_core, test_cmd_supported)
{
	zassert_ok(pmbus_init(&dev, &spec, 1));

	zassert_true(pmbus_cmd_supported(&dev, 0, PMBUS_READ_VIN, 2));
	zassert_true(pmbus_cmd_supported(&dev, 0, PMBUS_STATUS_BYTE, 1));
	zassert_equal(fake.clear_faults, 0);

	atomic_set_bit(fake.nack, PMBUS_READ_IIN);
	zassert_false(pmbus_cmd_supported(&dev, 0, PMBUS_READ_IIN, 2));
	zassert_equal(fake.clear_faults, 1);
	zassert_equal(fake.cml, 0);

	atomic_set_bit(fake.cml_only, PMBUS_READ_PIN);
	zassert_false(pmbus_cmd_supported(&dev, 0, PMBUS_READ_PIN, 2));
	zassert_equal(fake.clear_faults, 2);
	zassert_equal(fake.cml, 0);

	zassert_false(pmbus_cmd_supported(&dev, 0, PMBUS_READ_VIN, 0));
	zassert_false(pmbus_cmd_supported(&dev, 0, PMBUS_READ_VIN, 3));
	zassert_false(pmbus_cmd_supported(&dev, 1, PMBUS_READ_VIN, 2));
}

ZTEST(pmbus_core, test_cmd_supported_pec)
{
	zassert_ok(smbus_configure(smbus, SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC));
	fake.pec = true;
	zassert_ok(pmbus_init(&dev, &spec, 1));

	zassert_true(pmbus_cmd_supported(&dev, 0, PMBUS_READ_VIN, 2));
	zassert_true(pmbus_cmd_supported(&dev, 0, PMBUS_STATUS_BYTE, 1));
}

ZTEST(pmbus_core, test_cmd_supported_page)
{
	zassert_ok(pmbus_init(&dev, &spec, FAKE_PAGES));

	atomic_set_bit(fake.cml_only, PMBUS_READ_IOUT);
	zassert_false(pmbus_cmd_supported(&dev, 1, PMBUS_READ_IOUT, 2));
	zassert_equal(fake.clear_faults_page, PMBUS_PAGE_ALL);
	zassert_true(pmbus_cmd_supported(&dev, 1, PMBUS_READ_VOUT, 2));
}

ZTEST(pmbus_core, test_value_vout)
{
	int32_t val;

	zassert_ok(pmbus_init(&dev, &spec, 1));

	fake.regs[0][PMBUS_READ_VOUT] = 0x34CD;
	zassert_ok(pmbus_read_value(&dev, 0, PMBUS_READ_VOUT, &val));
	zassert_equal(val, 3300);

	zassert_ok(pmbus_write_byte(&dev, 0, PMBUS_VOUT_MODE, 0x13));
	zassert_equal(dev.vout_mode[0], 0x13);
	fake.regs[0][PMBUS_READ_VOUT] = 0x2000;
	zassert_ok(pmbus_read_value(&dev, 0, PMBUS_READ_VOUT, &val));
	zassert_equal(val, 1000);

	/* The relative bit does not apply to READ_VOUT */
	zassert_ok(pmbus_write_byte(&dev, 0, PMBUS_VOUT_MODE, 0x93));
	zassert_ok(pmbus_read_value(&dev, 0, PMBUS_READ_VOUT, &val));
	zassert_equal(val, 1000);
}

ZTEST(pmbus_core, test_value_vout_unsupported_mode)
{
	static const uint8_t modes[] = {0x20, 0x40, 0x60};
	int32_t val;

	ARRAY_FOR_EACH(modes, i) {
		fake.regs[0][PMBUS_VOUT_MODE] = modes[i];
		zassert_ok(pmbus_init(&dev, &spec, 1));
		zassert_equal(pmbus_read_value(&dev, 0, PMBUS_READ_VOUT, &val), -ENOTSUP,
			      "mode 0x%02x", modes[i]);
	}

	atomic_set_bit(fake.nack, PMBUS_VOUT_MODE);
	zassert_ok(pmbus_init(&dev, &spec, 1));
	zassert_equal(pmbus_read_value(&dev, 0, PMBUS_READ_VOUT, &val), -ENOTSUP);
}

ZTEST(pmbus_core, test_value_linear11)
{
	static const uint8_t cmds[] = {
		PMBUS_READ_VIN,           PMBUS_READ_IIN,           PMBUS_READ_IOUT,
		PMBUS_READ_TEMPERATURE_1, PMBUS_READ_TEMPERATURE_2, PMBUS_READ_TEMPERATURE_3,
		PMBUS_READ_POUT,          PMBUS_READ_PIN,
	};
	int32_t val;

	zassert_ok(pmbus_init(&dev, &spec, 1));

	ARRAY_FOR_EACH(cmds, i) {
		fake.regs[0][cmds[i]] = 0xF738;
		zassert_ok(pmbus_read_value(&dev, 0, cmds[i], &val));
		zassert_equal(val, -50000, "cmd 0x%02x", cmds[i]);
	}

	fake.regs[0][PMBUS_READ_VIN] = 0x7BFF;
	zassert_equal(pmbus_read_value(&dev, 0, PMBUS_READ_VIN, &val), -ERANGE);
}

ZTEST(pmbus_core, test_value_invalid)
{
	int32_t val;

	zassert_ok(pmbus_init(&dev, &spec, 1));

	zassert_equal(pmbus_read_value(&dev, 0, PMBUS_VIN_ON, &val), -ENOTSUP);
	zassert_equal(pmbus_read_value(&dev, 1, PMBUS_READ_VIN, &val), -EINVAL);
	zassert_equal(pmbus_read_value(&dev, 1, PMBUS_READ_VOUT, &val), -ENOTSUP);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	fake_defaults(&fake);
	zassert_ok(smbus_configure(smbus, SMBUS_MODE_CONTROLLER));
}

static void *setup(void)
{
	zassert_true(device_is_ready(smbus));
	zassert_ok(fake_pmbus_dev_register(&fake, DEVICE_DT_GET(DT_NODELABEL(i2c1)), FAKE_ADDR,
					   FAKE_PAGES, 0));

	return NULL;
}

ZTEST_SUITE(pmbus_core, NULL, setup, before, NULL, NULL);
