/*
 * Copyright 2020 Peter Bigot Consulting, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <ztest.h>

#define REGULATOR_NODE DT_PATH(regulator)
#define CHECK_NODE DT_PATH(resources)

BUILD_ASSERT(DT_NODE_HAS_COMPAT_STATUS(REGULATOR_NODE, regulator_fixed, okay));
BUILD_ASSERT(DT_NODE_HAS_COMPAT_STATUS(CHECK_NODE, test_regulator_fixed, okay));

#define IS_REGULATOR_SYNC DT_NODE_HAS_COMPAT_STATUS(REGULATOR_NODE, regulator_fixed_sync, okay)
#define BOOT_ON DT_PROP(REGULATOR_NODE, regulator_boot_on)
#define ALWAYS_ON DT_PROP(REGULATOR_NODE, regulator_always_on)
#define STARTUP_DELAY_US DT_PROP(REGULATOR_NODE, startup_delay_us)
#define OFF_ON_DELAY_US DT_PROP(REGULATOR_NODE, off_on_delay_us)

static const struct gpio_dt_spec reg_gpio = GPIO_DT_SPEC_GET(REGULATOR_NODE, enable_gpios);
static const struct gpio_dt_spec check_gpio = GPIO_DT_SPEC_GET(CHECK_NODE, check_gpios);

static const struct device *reg_dev = DEVICE_DT_GET(REGULATOR_NODE);

static enum {
	PC_UNCHECKED,
	PC_FAIL_REG_DEV_READY,
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
	[PC_FAIL_REG_DEV_READY] = "regulator device not ready",
	[PC_FAIL_DEVICES_READY] = "GPIO devices not ready",
	[PC_FAIL_CFG_OUTPUT] = "failed to configure output",
	[PC_FAIL_CFG_INPUT] = "failed to configure input",
	[PC_FAIL_INACTIVE] = "inactive check failed",
	[PC_FAIL_ACTIVE] = "active check failed",
	[PC_FAIL_UNCONFIGURE] = "failed to disconnect regulator GPIO",
	[PC_OK] = "precheck OK",
};

static struct onoff_client cli;
static struct onoff_manager *callback_srv;
static struct onoff_client *callback_cli;
static uint32_t callback_state;
static int callback_res;
static onoff_client_callback callback_fn;

static void callback(struct onoff_manager *srv,
		     struct onoff_client *cli,
		     uint32_t state,
		     int res)
{
	onoff_client_callback cb = callback_fn;

	callback_srv = srv;
	callback_cli = cli;
	callback_state = state;
	callback_res = res;
	callback_fn = NULL;

	if (cb != NULL) {
		cb(srv, cli, state, res);
	}
}

static void reset_callback(void)
{
	callback_srv = NULL;
	callback_cli = NULL;
	callback_state = INT_MIN;
	callback_res = 0;
	callback_fn = NULL;
}

static void reset_client(void)
{
	cli = (struct onoff_client){};
	reset_callback();
	sys_notify_init_callback(&cli.notify, callback);
}

static int reg_status(void)
{
	return gpio_pin_get_dt(&check_gpio);
}

static int setup(const struct device *dev)
{
	if (!device_is_ready(reg_dev)) {
		precheck = PC_FAIL_REG_DEV_READY;
		return -ENODEV;
	}

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

static void test_preconditions(void)
{
	zassert_equal(precheck, PC_OK,
		      "precheck failed: %s",
		      pc_errstr[precheck]);
}

static void test_basic(void)
{
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

	reset_client();

	/* Turn it on */
	int rc = regulator_enable(reg_dev, &cli);
	zassert_true(rc >= 0,
		     "first enable failed: %d", rc);

	if (STARTUP_DELAY_US > 0) {
		rc = sys_notify_fetch_result(&cli.notify, &rc);

		zassert_equal(rc, -EAGAIN,
			      "startup notify early: %d", rc);

		while (sys_notify_fetch_result(&cli.notify, &rc) == -EAGAIN) {
			k_yield();
		}
	}

	zassert_equal(callback_cli, &cli,
		      "callback not invoked");
	zassert_equal(callback_res, 0,
		      "callback res: %d", callback_res);
	zassert_equal(callback_state, ONOFF_STATE_ON,
		      "callback state: 0x%x", callback_res);

	/* Make sure it's on */

	rs = reg_status();
	zassert_equal(rs, 1,
		      "bad on state: %d", rs);

	/* Turn it on again (another client) */

	reset_client();
	rc = regulator_enable(reg_dev, &cli);
	zassert_true(rc >= 0,
		     "second enable failed: %d", rc);

	zassert_equal(callback_cli, &cli,
		      "callback not invoked");
	zassert_true(callback_res >= 0,
		      "callback res: %d", callback_res);
	zassert_equal(callback_state, ONOFF_STATE_ON,
		      "callback state: 0x%x", callback_res);

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
	zassert_true(rc >= 0,
		     "first disable failed: %d", rc);

	/* On if and only if it can't be turned off */

	rs = reg_status();
	zassert_equal(rs, IS_ENABLED(ALWAYS_ON),
		      "bad 2x on 2x off state: %d", rs);
}

void test_main(void)
{
	const char * const compats[] = DT_PROP(REGULATOR_NODE, compatible);

	printk("reg %p gpio %p\n", reg_dev, check_gpio.port);
	TC_PRINT("Regulator: %s%s%s\n", compats[0],
		 IS_ENABLED(BOOT_ON) ? ", boot-on" : "",
		 IS_ENABLED(ALWAYS_ON) ? ", always-on" : "");
	TC_PRINT("startup-delay: %u us\n", STARTUP_DELAY_US);
	TC_PRINT("off-on-delay: %u us\n", OFF_ON_DELAY_US);

	ztest_test_suite(regulator_test,
			 ztest_unit_test(test_preconditions),
			 ztest_unit_test(test_basic));
	ztest_run_test_suite(regulator_test);
}
