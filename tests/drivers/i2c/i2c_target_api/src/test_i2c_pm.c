/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/eeprom.h>
#include <zephyr/pm/pm.h>

#define NODE_EP0 DT_NODELABEL(eeprom0)
#define NODE_EP1 DT_NODELABEL(eeprom1)

#define I2C_PM_LEN 16
#define I2C_PM_IDLE_WINDOW_MS 100

static atomic_t i2c_pm_standby_entries;
static bool i2c_pm_registered;
static bool i2c_pm_notifier_on;

static void i2c_pm_state_entry(enum pm_state state)
{
	if (state == PM_STATE_STANDBY) {
		atomic_inc(&i2c_pm_standby_entries);
	}
}

static struct pm_notifier i2c_pm_notifier = {
	.state_entry = i2c_pm_state_entry,
};

static void i2c_pm_expect_sleep(void)
{
	atomic_clear(&i2c_pm_standby_entries);
	k_sleep(K_MSEC(I2C_PM_IDLE_WINDOW_MS));
	zassert_true(atomic_get(&i2c_pm_standby_entries) > 0,
		     "System stayed active during the idle window");
}

static void i2c_pm_transfer(const struct device *i2c, uint16_t addr)
{
	uint8_t tx[1 + I2C_PM_LEN];
	uint8_t rx[I2C_PM_LEN];
	int ret;

	tx[0] = 0;
	for (int i = 0; i < I2C_PM_LEN; i++) {
		tx[i + 1] = (uint8_t)i;
	}

	memset(rx, 0, sizeof(rx));

	ret = i2c_write(i2c, tx, sizeof(tx), addr);
	zassert_ok(ret, "I2C write failed, code %d", ret);

	ret = i2c_write_read(i2c, addr, tx, 1, rx, sizeof(rx));
	zassert_ok(ret, "I2C read failed, code %d", ret);

	zassert_equal(memcmp(&tx[1], rx, sizeof(rx)), 0, "RX buffer does not match TX");
}

static void i2c_pm_after(void *fixture)
{
	const struct device *const eeprom = DEVICE_DT_GET(NODE_EP0);

	ARG_UNUSED(fixture);

	if (i2c_pm_registered) {
		int ret;

		ret = i2c_target_driver_unregister(eeprom);
		i2c_pm_registered = false;
		zassert_ok(ret, "Failed to unregister EEPROM target, code %d", ret);
	}

	if (i2c_pm_notifier_on) {
		int ret;

		ret = pm_notifier_unregister(&i2c_pm_notifier);
		i2c_pm_notifier_on = false;
		zassert_ok(ret, "Failed to unregister PM notifier, code %d", ret);
	}
}

ZTEST(i2c_pm, test_i2c_pm_light_sleep)
{
	const struct device *const eeprom = DEVICE_DT_GET(NODE_EP0);
	const struct device *const i2c = DEVICE_DT_GET(DT_BUS(NODE_EP1));
	uint16_t addr = DT_REG_ADDR(NODE_EP0);
	int ret;

	zassert_equal(DT_PROP_OR(NODE_EP0, address_width, 8), 8, "EEPROM address width is not 8");
	zassert_true(device_is_ready(eeprom), "EEPROM target is not ready");
	zassert_true(device_is_ready(i2c), "I2C controller is not ready");

	pm_notifier_register(&i2c_pm_notifier);
	i2c_pm_notifier_on = true;

	/* Attaching a target holds the PM lock until it is detached, so
	 * the idle window has to run while neither controller is a target.
	 * The second pass sleeps after a finished transfer.
	 */
	for (int cycle = 0; cycle < 2; cycle++) {
		i2c_pm_expect_sleep();

		ret = i2c_target_driver_register(eeprom);
		zassert_ok(ret, "Failed to register EEPROM target, code %d", ret);
		i2c_pm_registered = true;

		i2c_pm_transfer(i2c, addr);

		ret = i2c_target_driver_unregister(eeprom);
		i2c_pm_registered = false;
		zassert_ok(ret, "Failed to unregister EEPROM target, code %d", ret);
	}
}

ZTEST_SUITE(i2c_pm, NULL, NULL, NULL, i2c_pm_after, NULL);
