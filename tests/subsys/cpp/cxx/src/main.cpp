/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is mainly a parse test that verifies that Zephyr header files
 * compile in C++ mode.
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>

#include <init.h>
#include <device.h>
#include <kernel.h>
#include <net/buf.h>
#include <sys/byteorder.h>
#include <sys/crc.h>
#include <sys/crc.h>

#include <drivers/gpio.h>
#include <drivers/led_strip.h>
#include <drivers/spi.h>
#include <drivers/uart.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <drivers/watchdog.h>

#include <ztest.h>

class foo_class {
public:
	foo_class(int foo) : foo(foo) {}
	int get_foo() const { return foo;}
private:
	int foo;
};

struct foo {
	int v1;
};
/* Check that BUILD_ASSERT compiles. */
BUILD_ASSERT(sizeof(foo) == sizeof(int));

static struct foo foos[5];
/* Check that ARRAY_SIZE compiles. */
BUILD_ASSERT(ARRAY_SIZE(foos) == 5, "expected 5 elements");

/* Check that SYS_INIT() compiles. */
static int test_init(const struct device *dev)
{
	return 0;
}

SYS_INIT(test_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);


static void test_new_delete(void)
{
	foo_class *test_foo = new foo_class(10);
	zassert_equal(test_foo->get_foo(), 10, NULL);
	delete test_foo;
}

void test_main(void)
{
	ztest_test_suite(cpp_tests,
			 ztest_unit_test(test_new_delete)
		);

	ztest_run_test_suite(cpp_tests);
}
