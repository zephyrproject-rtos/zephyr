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

struct foo {
	int v1;
};
/* Check that BUILD_ASSERT compiles. */
BUILD_ASSERT(sizeof(foo) == sizeof(int));

static struct foo foos[5];
/* Check that ARRAY_SIZE compiles. */
BUILD_ASSERT_MSG(ARRAY_SIZE(foos) == 5, "expected 5 elements");

/* Check that SYS_INIT() compiles. */
static int test_init(struct device *dev)
{
	return 0;
}

SYS_INIT(test_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

void test_main(void)
{
	/* Does nothing.  This is a compile only test. */
}
