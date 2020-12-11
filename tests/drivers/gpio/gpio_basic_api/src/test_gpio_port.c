/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_gpio.h"

#define ALL_BITS ((gpio_port_value_t)-1)

static const struct device *dev;

/* Short-hand for a checked read of PIN_IN raw state */
static bool raw_in(void)
{
	gpio_port_value_t v;
	int rc = gpio_port_get_raw(dev, &v);

	zassert_equal(rc, 0,
		      "raw_in failed");
	return (v & BIT(PIN_IN)) ? true : false;
}

/* Short-hand for a checked read of PIN_IN logical state */
static bool logic_in(void)
{
	gpio_port_value_t v;
	int rc = gpio_port_get(dev, &v);

	zassert_equal(rc, 0,
		      "logic_in failed");
	return (v & BIT(PIN_IN)) ? true : false;
}

/* Short-hand for a checked write of PIN_OUT raw state */
static void raw_out(bool set)
{
	int rc;

	if (set) {
		rc = gpio_port_set_bits_raw(dev, BIT(PIN_OUT));
	} else {
		rc = gpio_port_clear_bits_raw(dev, BIT(PIN_OUT));
	}
	zassert_equal(rc, 0,
		      "raw_out failed");
}

/* Short-hand for a checked write of PIN_OUT logic state */
static void logic_out(bool set)
{
	int rc;

	if (set) {
		rc = gpio_port_set_bits(dev, BIT(PIN_OUT));
	} else {
		rc = gpio_port_clear_bits(dev, BIT(PIN_OUT));
	}
	zassert_equal(rc, 0,
		      "raw_out failed");
}

/* Verify device, configure for physical in and out, verify
 * connection, verify raw_in().
 */
static int setup(void)
{
	int rc;
	gpio_port_value_t v1;

	TC_PRINT("Validate device %s\n", DEV_NAME);
	dev = device_get_binding(DEV_NAME);
	zassert_not_equal(dev, NULL,
			  "Device not found");

	TC_PRINT("Check %s output %d connected to input %d\n", DEV_NAME,
		 PIN_OUT, PIN_IN);

	rc = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT);
	zassert_equal(rc, 0,
		      "pin config input failed");

	/* Test output low */
	rc = gpio_pin_configure(dev, PIN_OUT, GPIO_OUTPUT_LOW);
	zassert_equal(rc, 0,
		      "pin config output low failed");

	rc = gpio_port_get_raw(dev, &v1);
	zassert_equal(rc, 0,
		      "get raw low failed");
	if (raw_in() != false) {
		TC_PRINT("FATAL output pin not wired to input pin? (out low => in high)\n");
		while (true) {
			k_sleep(K_FOREVER);
		}
	}

	zassert_equal(v1 & BIT(PIN_IN), 0,
		      "out low does not read low");

	/* Disconnect output */
	rc = gpio_pin_configure(dev, PIN_OUT, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT("NOTE: cannot configure pin as disconnected; trying as input\n");
		rc = gpio_pin_configure(dev, PIN_OUT, GPIO_INPUT);
	}
	zassert_equal(rc, 0,
		      "output disconnect failed");

	/* Test output high */
	rc = gpio_pin_configure(dev, PIN_OUT, GPIO_OUTPUT_HIGH);
	zassert_equal(rc, 0,
		      "pin config output high failed");

	rc = gpio_port_get_raw(dev, &v1);
	zassert_equal(rc, 0,
		      "get raw high failed");
	if (raw_in() != true) {
		TC_PRINT("FATAL output pin not wired to input pin? (out high => in low)\n");
		while (true) {
			k_sleep(K_FOREVER);
		}
	}
	zassert_not_equal(v1 & BIT(PIN_IN), 0,
			  "out high does not read low");

	TC_PRINT("OUT %d to IN %d linkage works\n", PIN_OUT, PIN_IN);
	return TC_PASS;
}

/* gpio_port_set_bits_raw()
 * gpio_port_clear_bits_raw()
 * gpio_port_set_masked_raw()
 * gpio_port_toggle_bits()
 */
static int bits_physical(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	/* port_set_bits_raw */
	rc = gpio_port_set_bits_raw(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(raw_in(), true,
		      "raw set mismatch");

	/* port_clear_bits_raw */
	rc = gpio_port_clear_bits_raw(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port clear raw out failed");
	zassert_equal(raw_in(), false,
		      "raw clear mismatch");

	/* set after clear changes */
	rc = gpio_port_set_bits_raw(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(raw_in(), true,
		      "raw set mismatch");

	/* raw_out() after set works */
	raw_out(false);
	zassert_equal(raw_in(), false,
		      "raw_out() false mismatch");

	/* raw_out() set after raw_out() clear works */
	raw_out(true);
	zassert_equal(raw_in(), true,
		      "raw_out() true mismatch");

	rc = gpio_port_set_masked_raw(dev, BIT(PIN_OUT), 0);
	zassert_equal(rc, 0,
		      "set_masked_raw low failed");
	zassert_equal(raw_in(), false,
		      "set_masked_raw low mismatch");

	rc = gpio_port_set_masked_raw(dev, BIT(PIN_OUT), ALL_BITS);
	zassert_equal(rc, 0,
		      "set_masked_raw high failed");
	zassert_equal(raw_in(), true,
		      "set_masked_raw high mismatch");

	rc = gpio_port_set_masked_raw(dev, BIT(PIN_IN), 0);
	zassert_equal(rc, 0,
		      "set_masked_raw low failed");
	zassert_equal(raw_in(), true,
		      "set_masked_raw low affected other pins");

	rc = gpio_port_set_clr_bits_raw(dev, BIT(PIN_IN), BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "set in clear out failed");
	zassert_equal(raw_in(), false,
		      "set in clear out mismatch");

	rc = gpio_port_set_clr_bits_raw(dev, BIT(PIN_OUT), BIT(PIN_IN));
	zassert_equal(rc, 0,
		      "set out clear in failed");
	zassert_equal(raw_in(), true,
		      "set out clear in mismatch");

	/* Conditionally verify that behavior with __ASSERT disabled
	 * is to set the bit.
	 */
	if (false) {
		/* preserve set */
		rc = gpio_port_set_clr_bits_raw(dev, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c dup set failed");
		zassert_equal(raw_in(), true,
			      "s/c dup set mismatch");

		/* do set */
		raw_out(false);
		rc = gpio_port_set_clr_bits_raw(dev, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c dup2 set failed");
		zassert_equal(raw_in(), true,
			      "s/c dup2 set mismatch");
	}

	rc = gpio_port_toggle_bits(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits high-to-low failed");
	zassert_equal(raw_in(), false,
		      "toggle_bits high-to-low mismatch");

	rc = gpio_port_toggle_bits(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits low-to-high failed");
	zassert_equal(raw_in(), true,
		      "toggle_bits low-to-high mismatch");

	return TC_PASS;
}

/* gpio_pin_get_raw()
 * gpio_pin_set_raw()
 */
static int pin_physical(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	raw_out(true);
	zassert_equal(gpio_pin_get_raw(dev, PIN_IN), raw_in(),
		      "pin_get_raw high failed");

	raw_out(false);
	zassert_equal(gpio_pin_get_raw(dev, PIN_IN), raw_in(),
		      "pin_get_raw low failed");

	rc = gpio_pin_set_raw(dev, PIN_OUT, 32);
	zassert_equal(rc, 0,
		      "pin_set_raw high failed");
	zassert_equal(raw_in(), true,
		      "pin_set_raw high failed");

	rc = gpio_pin_set_raw(dev, PIN_OUT, 0);
	zassert_equal(rc, 0,
		      "pin_set_raw low failed");
	zassert_equal(raw_in(), false,
		      "pin_set_raw low failed");

	rc = gpio_pin_toggle(dev, PIN_OUT);
	zassert_equal(rc, 0,
		      "pin_toggle low-to-high failed");
	zassert_equal(raw_in(), true,
		      "pin_toggle low-to-high mismatch");

	rc = gpio_pin_toggle(dev, PIN_OUT);
	zassert_equal(rc, 0,
		      "pin_toggle high-to-low failed");
	zassert_equal(raw_in(), false,
		      "pin_toggle high-to-low mismatch");

	return TC_PASS;
}

/* Verify configure output level is independent of active level, and
 * raw output is independent of active level.
 */
static int check_raw_output_levels(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_LOW);
	zassert_equal(rc, 0,
		      "active high output low failed");
	zassert_equal(raw_in(), false,
		      "active high output low raw mismatch");
	raw_out(true);
	zassert_equal(raw_in(), true,
		      "set high mismatch");

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_HIGH);
	zassert_equal(rc, 0,
		      "active high output high failed");
	zassert_equal(raw_in(), true,
		      "active high output high raw mismatch");
	raw_out(false);
	zassert_equal(raw_in(), false,
		      "set low mismatch");

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_LOW);
	zassert_equal(rc, 0,
		      "active low output low failed");
	zassert_equal(raw_in(), false,
		      "active low output low raw mismatch");
	raw_out(true);
	zassert_equal(raw_in(), true,
		      "set high mismatch");

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_HIGH);
	zassert_equal(rc, 0,
		      "active low output high failed");
	zassert_equal(raw_in(), true,
		      "active low output high raw mismatch");
	raw_out(false);
	zassert_equal(raw_in(), false,
		      "set low mismatch");

	return TC_PASS;
}

/* Verify configure output level is dependent on active level, and
 * logic output is dependent on active level.
 */
static int check_logic_output_levels(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_INACTIVE);
	zassert_equal(rc, 0,
		      "active true output false failed: %d", rc);
	zassert_equal(raw_in(), false,
		      "active true output false logic mismatch");
	logic_out(true);
	zassert_equal(raw_in(), true,
		      "set true mismatch");

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_ACTIVE);
	zassert_equal(rc, 0,
		      "active true output true failed");
	zassert_equal(raw_in(), true,
		      "active true output true logic mismatch");
	logic_out(false);
	zassert_equal(raw_in(), false,
		      "set false mismatch");

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_ACTIVE);
	zassert_equal(rc, 0,
		      "active low output true failed");

	zassert_equal(raw_in(), false,
		      "active low output true raw mismatch");
	logic_out(false);
	zassert_equal(raw_in(), true,
		      "set false mismatch");

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_INACTIVE);
	zassert_equal(rc, 0,
		      "active low output false failed");
	zassert_equal(raw_in(), true,
		      "active low output false logic mismatch");
	logic_out(true);
	zassert_equal(raw_in(), false,
		      "set true mismatch");

	return TC_PASS;
}

/* Verify active-high input matches physical level, and active-low
 * input inverts physical level.
 */
static int check_input_levels(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	rc = gpio_pin_configure(dev, PIN_OUT, GPIO_OUTPUT);
	zassert_equal(rc, 0,
		      "output configure failed");

	rc = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT);
	zassert_equal(rc, 0,
		      "active high failed");
	raw_out(true);
	zassert_equal(raw_in(), true,
		      "raw high mismatch");
	zassert_equal(logic_in(), true,
		      "logic high mismatch");

	raw_out(false);
	zassert_equal(raw_in(), false,
		      "raw low mismatch");
	zassert_equal(logic_in(), false,
		      "logic low mismatch");

	rc = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT | GPIO_ACTIVE_LOW);
	zassert_equal(rc, 0,
		      "active low failed");

	raw_out(true);
	zassert_equal(raw_in(), true,
		      "raw high mismatch");
	zassert_equal(logic_in(), false,
		      "logic inactive mismatch");

	raw_out(false);
	zassert_equal(raw_in(), false,
		      "raw low mismatch");
	zassert_equal(logic_in(), true,
		      "logic active mismatch");

	return TC_PASS;
}

/* Delay after pull input config to allow signal to settle.  The value
 * selected is conservative (higher than may be necessary).
 */
#define PULL_DELAY_US 100U

/* Verify that pull-up and pull-down work for a disconnected input. */
static int check_pulls(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	/* Disconnect output */
	rc = gpio_pin_configure(dev, PIN_OUT, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT("NOTE: cannot configure pin as disconnected; trying as input\n");
		rc = gpio_pin_configure(dev, PIN_OUT, GPIO_INPUT);
	}
	zassert_equal(rc, 0,
		      "output disconnect failed");

	rc = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT | GPIO_PULL_UP);
	k_busy_wait(PULL_DELAY_US);
	if (rc == -ENOTSUP) {
		TC_PRINT("pull-up not supported\n");
		return TC_PASS;
	} else if (rc != 0) {
		TC_ERROR("input pull-up fail: %d\n", rc);
		return TC_FAIL;
	}
	zassert_equal(raw_in(), true,
		      "physical pull-up does not read high");

	rc = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT | GPIO_PULL_DOWN);
	k_busy_wait(PULL_DELAY_US);
	if (rc == -ENOTSUP) {
		TC_PRINT("pull-down not supported\n");
		return TC_PASS;
	} else if (rc != 0) {
		TC_ERROR("input pull-down fail: %d\n", rc);
		return TC_FAIL;
	}
	zassert_equal(raw_in(), false,
		      "physical pull-down does not read low");

	/* Test that pull is not affected by active level */
	rc = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT | GPIO_ACTIVE_LOW | GPIO_PULL_UP);
	k_busy_wait(PULL_DELAY_US);
	if (rc == -ENOTSUP) {
		TC_PRINT("pull-up not supported\n");
		return TC_PASS;
	} else if (rc != 0) {
		TC_ERROR("input pull-up fail: %d\n", rc);
		return TC_FAIL;
	}
	zassert_equal(raw_in(), true,
		      "logical pull-up does not read high");
	zassert_equal(logic_in(), false,
		      "logical pull-up reads true");

	rc = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT | GPIO_ACTIVE_LOW | GPIO_PULL_DOWN);
	k_busy_wait(PULL_DELAY_US);
	if (rc == -ENOTSUP) {
		TC_PRINT("pull-up not supported\n");
		return TC_PASS;
	} else if (rc != 0) {
		TC_ERROR("input pull-up fail: %d\n", rc);
		return TC_FAIL;
	}
	zassert_equal(raw_in(), false,
		      "logical pull-down does not read low");
	zassert_equal(logic_in(), true,
		      "logical pull-down reads false");

	return TC_PASS;
}

/* gpio_port_set_bits()
 * gpio_port_clear_bits()
 * gpio_port_set_masked()
 * gpio_port_toggle_bits()
 */
static int bits_logical(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	rc = gpio_pin_configure(dev, PIN_OUT,
				GPIO_OUTPUT_HIGH | GPIO_ACTIVE_LOW);
	zassert_equal(rc, 0,
		      "output configure failed");
	zassert_equal(raw_in(), true,
		      "raw out high mismatch");
	zassert_equal(logic_in(), !raw_in(),
		      "logic in active mismatch");

	/* port_set_bits */
	rc = gpio_port_set_bits(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(raw_in(), false,
		      "raw low set mismatch");
	zassert_equal(logic_in(), !raw_in(),
		      "logic in inactive mismatch");

	/* port_clear_bits */
	rc = gpio_port_clear_bits(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port clear raw out failed");
	zassert_equal(logic_in(), false,
		      "low clear mismatch");

	/* set after clear changes */
	rc = gpio_port_set_bits_raw(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(logic_in(), false,
		      "raw set mismatch");

	/* pin_set false */
	rc = gpio_pin_set(dev, PIN_OUT, 0);
	zassert_equal(rc, 0,
		      "pin clear failed");
	zassert_equal(logic_in(), false,
		      "pin clear mismatch");

	/* pin_set true */
	rc = gpio_pin_set(dev, PIN_OUT, 32);
	zassert_equal(rc, 0,
		      "pin set failed");
	zassert_equal(logic_in(), true,
		      "pin set mismatch");

	rc = gpio_port_set_masked(dev, BIT(PIN_OUT), 0);
	zassert_equal(rc, 0,
		      "set_masked low failed");
	zassert_equal(logic_in(), false,
		      "set_masked low mismatch");

	rc = gpio_port_set_masked(dev, BIT(PIN_OUT), ALL_BITS);
	zassert_equal(rc, 0,
		      "set_masked high failed");
	zassert_equal(logic_in(), true,
		      "set_masked high mismatch");

	rc = gpio_port_set_clr_bits(dev, BIT(PIN_IN), BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "set in clear out failed");
	zassert_equal(logic_in(), false,
		      "set in clear out mismatch");

	rc = gpio_port_set_clr_bits(dev, BIT(PIN_OUT), BIT(PIN_IN));
	zassert_equal(rc, 0,
		      "set out clear in failed");
	zassert_equal(logic_in(), true,
		      "set out clear in mismatch");

	/* Conditionally verify that behavior with __ASSERT disabled
	 * is to set the bit.
	 */
	if (false) {
		/* preserve set */
		rc = gpio_port_set_clr_bits(dev, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c set toggle failed");
		zassert_equal(logic_in(), false,
			      "s/c set toggle mismatch");

		/* force set */
		raw_out(true);
		rc = gpio_port_set_clr_bits(dev, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c dup set failed");
		zassert_equal(logic_in(), false,
			      "s/c dup set mismatch");
	}

	rc = gpio_port_toggle_bits(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits active-to-inactive failed");
	zassert_equal(logic_in(), false,
		      "toggle_bits active-to-inactive mismatch");

	rc = gpio_port_toggle_bits(dev, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits inactive-to-active failed");
	zassert_equal(logic_in(), true,
		      "toggle_bits inactive-to-active mismatch");

	rc = gpio_pin_toggle(dev, PIN_OUT);
	zassert_equal(rc, 0,
		      "pin_toggle low-to-high failed");
	zassert_equal(logic_in(), false,
		      "pin_toggle low-to-high mismatch");

	return TC_PASS;
}

void test_gpio_port(void)
{
	zassert_equal(setup(), TC_PASS,
		      "device setup failed");
	zassert_equal(bits_physical(), TC_PASS,
		      "bits_physical failed");
	zassert_equal(pin_physical(), TC_PASS,
		      "pin_physical failed");
	zassert_equal(check_raw_output_levels(), TC_PASS,
		      "check_raw_output_levels failed");
	zassert_equal(check_logic_output_levels(), TC_PASS,
		      "check_logic_output_levels failed");
	zassert_equal(check_input_levels(), TC_PASS,
		      "check_input_levels failed");
	zassert_equal(bits_logical(), TC_PASS,
		      "bits_logical failed");
	zassert_equal(check_pulls(), TC_PASS,
		      "check_pulls failed");
}
