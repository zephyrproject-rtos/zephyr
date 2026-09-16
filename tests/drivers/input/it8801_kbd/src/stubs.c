/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Stub implementations of MFD IT8801 functions that are not compiled when
 * CONFIG_MFD_ITE_IT8801=n. These stubs let the keyboard driver be tested in
 * isolation while giving the test full control over MFD behavior.
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>

static struct it8801_mfd_callback *registered_callback;
static int configure_pins_retval;
static int configure_pins_call_count;

int mfd_it8801_configure_pins(const struct i2c_dt_spec *i2c_dev,
			      const struct device *dev, uint8_t pin,
			      uint8_t func)
{
	ARG_UNUSED(i2c_dev);
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	configure_pins_call_count++;
	return configure_pins_retval;
}

void mfd_it8801_register_interrupt_callback(
	const struct device *mfd, struct it8801_mfd_callback *callback)
{
	ARG_UNUSED(mfd);

	registered_callback = callback;
}

struct it8801_mfd_callback *test_get_registered_callback(void)
{
	return registered_callback;
}

void test_set_configure_pins_retval(int retval)
{
	configure_pins_retval = retval;
}

int test_get_configure_pins_call_count(void)
{
	return configure_pins_call_count;
}

void test_reset_stubs(void)
{
	registered_callback = NULL;
	configure_pins_retval = 0;
	configure_pins_call_count = 0;
}
