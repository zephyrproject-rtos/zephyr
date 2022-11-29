/*
 * Copyright 2020 Peter Bigot Consulting, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/ztest.h>

#define REGULATOR_NODE DT_PATH(regulator)
#define CHECK_NODE DT_PATH(resources)

BUILD_ASSERT(DT_NODE_HAS_COMPAT_STATUS(REGULATOR_NODE, regulator_fixed, okay));
BUILD_ASSERT(DT_NODE_HAS_COMPAT_STATUS(CHECK_NODE, test_regulator_fixed, okay));

#define BOOT_ON DT_PROP(REGULATOR_NODE, regulator_boot_on)
#define ALWAYS_ON DT_PROP(REGULATOR_NODE, regulator_always_on)
#define STARTUP_DELAY_US DT_PROP(REGULATOR_NODE, startup_delay_us)
#define OFF_ON_DELAY_US DT_PROP(REGULATOR_NODE, off_on_delay_us)

static const struct gpio_dt_spec reg_gpio = GPIO_DT_SPEC_GET(REGULATOR_NODE, enable_gpios);
static const struct gpio_dt_spec check_gpio = GPIO_DT_SPEC_GET(CHECK_NODE, check_gpios);

static const struct device *const reg_dev = DEVICE_DT_GET(REGULATOR_NODE);

static enum {
	PC_UNCHECKED,
	PC_FAIL_DEVICES_READY,
	PC_FAIL_CFG_OUTPUT,
	PC_FAIL_CFG_INPUT,
	PC_FAIL_INACTIVE,
	PC_FAIL_ACTIVE,
	PC_FAIL_UNCONFIGURE,
	PC_OK,
} precheck = PC_UNCHECKED;
static const char *const pc_errstr[] = {
	[PC_UNCHECKED] = "precheck not verified",
	[PC_FAIL_DEVICES_READY] = "GPIO devices not ready",
	[PC_FAIL_CFG_OUTPUT] = "failed to configure output",
	[PC_FAIL_CFG_INPUT] = "failed to configure input",
	[PC_FAIL_INACTIVE] = "inactive check failed",
	[PC_FAIL_ACTIVE] = "active check failed",
	[PC_FAIL_UNCONFIGURE] = "failed to disconnect regulator GPIO",
	[PC_OK] = "precheck OK",
};

static int reg_status(void)
{
	return gpio_pin_get_dt(&check_gpio);
}

static int setup(const struct device *dev)
{
	/* Configure the regulator GPIO as an output inactive, and the check
	 * GPIO as an input, then start testing whether they track.
	 */

	if (!device_is_ready(reg_gpio.port)) {
		precheck = PC_FAIL_DEVICES_READY;
		return -ENODEV;
	}

	if (!device_is_ready(check_gpio.port)) {
		precheck = PC_FAIL_DEVICES_READY;
		return -ENODEV;
	}

	int rc = gpio_pin_configure_dt(&reg_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc != 0) {
		precheck = PC_FAIL_CFG_OUTPUT;
		return -EIO;
	}

	rc = gpio_pin_configure_dt(&check_gpio, GPIO_INPUT);
	if (rc != 0) {
		precheck = PC_FAIL_CFG_INPUT;
		return -EIO;
	}

	rc = reg_status();

	if (rc != 0) {		/* should be inactive */
		precheck = PC_FAIL_INACTIVE;
		return -EIO;
	}

	rc = gpio_pin_set_dt(&reg_gpio, true);

	if (rc == 0) {
		rc = reg_status();
	}

	if (rc != 1) {		/* should be active */
		precheck = PC_FAIL_ACTIVE;
		return -EIO;
	}

	rc = gpio_pin_configure_dt(&reg_gpio, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		rc = gpio_pin_configure_dt(&reg_gpio, GPIO_INPUT);
	}
	if (rc == 0) {
		rc = reg_status();
	}

	if (rc != 0) {		/* should be inactive */
		precheck = PC_FAIL_UNCONFIGURE;
		return -EIO;
	}

	precheck = PC_OK;

	return rc;
}

/* The regulator driver initializes in POST_KERNEL since it has
 * thread-related stuff in it.  We need to verify the shorted signals
 * required by the test before the driver configures its GPIO.  This
 * should be done late PRE_KERNEL_9, but it can't because Nordic and
 * possibly other systems initialize GPIO drivers post-kernel. */
BUILD_ASSERT(CONFIG_REGULATOR_FIXED_INIT_PRIORITY > 74);
SYS_INIT(setup, POST_KERNEL, 74);

ZTEST(regulator, test_basic)
{
	zassert_true(device_is_ready(reg_dev), "regulator device not ready");

	zassert_equal(precheck, PC_OK,
		      "precheck failed: %s",
		      pc_errstr[precheck]);

	int rs = reg_status();

	/* Initial state: on if and only if it's always on or was enabled at
	 * boot.
	 */

	if (IS_ENABLED(BOOT_ON) || IS_ENABLED(ALWAYS_ON)) {
		zassert_equal(rs, 1,
			      "not on at boot: %d", rs);
	} else {
		zassert_equal(rs, 0,
			      "not off at boot: %d", rs);
	}

	/* Turn it on */
	int rc = regulator_enable(reg_dev);

	zassert_equal(rc, 0,
		      "first enable failed: %d", rc);

	/* Make sure it's on */

	rs = reg_status();
	zassert_equal(rs, 1,
		      "bad on state: %d", rs);

	/* Turn it on again (another client) */

	rc = regulator_enable(reg_dev);
	zassert_equal(rc, 0,
		     "second enable failed: %d", rc);

	/* Make sure it's still on */

	rs = reg_status();
	zassert_equal(rs, 1,
		      "bad 2x on state: %d", rs);

	/* Turn it off once (still has a client) */

	rc = regulator_disable(reg_dev);
	zassert_true(rc >= 0,
		     "first disable failed: %d", rc);

	/* Make sure it's still on */

	rs = reg_status();
	zassert_equal(rs, 1,
		      "bad 2x on 1x off state: %d", rs);

	/* Turn it off again (no more clients) */

	rc = regulator_disable(reg_dev);
	zassert_equal(rc, 0,
		     "first disable failed: %d", rc);

	/* On if and only if it can't be turned off */

	rs = reg_status();
	zassert_equal(rs, IS_ENABLED(ALWAYS_ON),
		      "bad 2x on 2x off state: %d", rs);
}

void *regulator_setup(void)
{
	const char * const compats[] = DT_PROP(REGULATOR_NODE, compatible);

	printk("reg %p gpio %p\n", reg_dev, check_gpio.port);
	TC_PRINT("Regulator: %s%s%s\n", compats[0],
		 IS_ENABLED(BOOT_ON) ? ", boot-on" : "",
		 IS_ENABLED(ALWAYS_ON) ? ", always-on" : "");
	TC_PRINT("startup-delay: %u us\n", STARTUP_DELAY_US);
	TC_PRINT("off-on-delay: %u us\n", OFF_ON_DELAY_US);

	return NULL;
}

ZTEST_SUITE(regulator, NULL, regulator_setup, NULL, NULL, NULL);
