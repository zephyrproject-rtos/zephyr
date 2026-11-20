/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/cmis/cmis.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cmis.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/ztest.h>

#include "emul_cmis.h"

#define CAGE_NODE DT_NODELABEL(cmis0)

static const struct device *const cmis_dev = DEVICE_DT_GET(CAGE_NODE);
static const struct emul *const cmis_emul = EMUL_DT_GET(DT_NODELABEL(cmis_module));
static const struct i2c_dt_spec cmis_bus = I2C_DT_SPEC_GET(DT_NODELABEL(cmis_module));
static const struct device *const presence_port = DEVICE_DT_GET(DT_NODELABEL(cmis_gpio));
static const gpio_pin_t presence_pin = DT_GPIO_PIN(CAGE_NODE, modprsl_gpios);

/* ModPrsL is pulled low by an inserted module (active low). */
static void set_module_present(bool present)
{
	zassert_ok(gpio_emul_input_set(presence_port, presence_pin, present ? 0 : 1));
}

/* Read one Lower Memory byte the way a host interrupt handler would. */
static uint8_t raw_read(uint8_t offset)
{
	uint8_t val = 0xFFU;

	zassert_ok(i2c_write_read_dt(&cmis_bus, &offset, 1U, &val, 1U));
	return val;
}

static void *cmis_setup(void)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		k_object_access_all_grant(cmis_dev);
	}

	return NULL;
}

/* Runs in supervisor mode, also for user-mode tests. */
static void cmis_before(void *fixture)
{
	ARG_UNUSED(fixture);
	set_module_present(true);
	cmis_emul_reset(cmis_emul);
}

ZTEST(cmis_driver, test_device_ready)
{
	zassert_true(device_is_ready(cmis_dev));
}

ZTEST_USER(cmis_driver, test_read_id)
{
	struct cmis_id id;

	zassert_ok(cmis_read_id(cmis_dev, &id));
	zassert_equal(id.identifier, CMIS_ID_QSFP_DD);
	zassert_equal(id.cmis_major, 5U);
	zassert_equal(id.cmis_minor, 0U);
	zassert_equal(id.module_state, CMIS_MODULE_STATE_READY);
	zassert_equal(id.media_type, CMIS_MEDIA_OPTICAL_SMF);
	zassert_equal(id.power_class, 8U);
	zassert_equal(id.max_power_mw, 14000U, "got %u", id.max_power_mw);
	zassert_equal(strcmp(id.vendor_name, "ACME PHOTONICS"), 0, "got '%s'", id.vendor_name);
	zassert_equal(strcmp(id.vendor_pn, "QSFP-DD-400G"), 0, "got '%s'", id.vendor_pn);
	zassert_equal(strcmp(id.vendor_sn, "QD2400001234"), 0, "got '%s'", id.vendor_sn);
}

ZTEST_USER(cmis_driver, test_read_monitors)
{
	struct cmis_monitors mon;

	zassert_ok(cmis_read_monitors(cmis_dev, &mon));
	zassert_equal(mon.temp_mdegc, 45500, "got %d", mon.temp_mdegc);
	zassert_equal(mon.vcc_uv, 3300000U, "got %u", mon.vcc_uv);
	zassert_equal(mon.flags, CMIS_FLAG_TEMP_HIGH_ALARM | CMIS_FLAG_VCC_HIGH_ALARM,
		      "got 0x%02x", mon.flags);
}

ZTEST(cmis_driver, test_monitor_flags_reported_once)
{
	struct cmis_monitors mon;

	zassert_ok(cmis_read_monitors(cmis_dev, &mon));
	zassert_equal(mon.flags, CMIS_FLAG_TEMP_HIGH_ALARM | CMIS_FLAG_VCC_HIGH_ALARM);

	/* Latched flags are cleared by the read: the next read sees no event. */
	zassert_ok(cmis_read_monitors(cmis_dev, &mon));
	zassert_equal(mon.flags, 0U, "got 0x%02x", mon.flags);
	zassert_equal(mon.temp_mdegc, 45500, "monitor values are not latched");
}

ZTEST(cmis_driver, test_reads_keep_unreported_flags)
{
	struct cmis_monitors mon;
	struct cmis_id id;

	zassert_ok(cmis_read_id(cmis_dev, &id));
	zassert_ok(cmis_read_monitors(cmis_dev, &mon));

	/* ModuleStateChanged (byte 8) is not returned by either call, so it
	 * must still be pending for the host's interrupt handling.
	 */
	zassert_equal(raw_read(CMIS_EMUL_MODULE_STATE_FLAGS) & CMIS_EMUL_STATE_CHANGED,
		      CMIS_EMUL_STATE_CHANGED);

	/* read_id() must not have consumed the byte 9 flags either. */
	zassert_equal(mon.flags, CMIS_FLAG_TEMP_HIGH_ALARM | CMIS_FLAG_VCC_HIGH_ALARM);
}

ZTEST(cmis_driver, test_empty_cage)
{
	struct cmis_monitors mon;
	struct cmis_id id;

	set_module_present(false);
	zassert_equal(cmis_read_id(cmis_dev, &id), -ENODEV);
	zassert_equal(cmis_read_monitors(cmis_dev, &mon), -ENODEV);

	set_module_present(true);
	zassert_ok(cmis_read_id(cmis_dev, &id));
}

ZTEST_SUITE(cmis_driver, NULL, cmis_setup, cmis_before, NULL, NULL);
