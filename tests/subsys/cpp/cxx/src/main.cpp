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
/* #include <sys/byteorder.h> conflicts with __bswapXX on native_posix */
#include <sys/crc.h>
#include <sys/crc.h>

#include <drivers/adc.h>
#include <drivers/bbram.h>
#include <drivers/cache.h>
#include <drivers/can.h>
#include <drivers/can/transceiver.h>
#include <drivers/clock_control.h>
#include <drivers/counter.h>
#include <drivers/dac.h>
#include <drivers/disk.h>
#include <drivers/display.h>
#include <drivers/dma.h>
#include <drivers/ec_host_cmd_periph.h>
#include <drivers/edac.h>
#include <drivers/eeprom.h>
#include <drivers/emul.h>
#include <drivers/entropy.h>
#include <drivers/espi_emul.h>
#include <drivers/espi.h>
/* drivers/espi_saf.h requires SoC specific header */
#include <drivers/flash.h>
#include <drivers/fpga.h>
#include <drivers/gna.h>
#include <drivers/gpio.h>
#include <drivers/hwinfo.h>
#include <drivers/i2c_emul.h>
#include <drivers/i2c.h>
#include <drivers/i2s.h>
#include <drivers/ipm.h>
#include <drivers/kscan.h>
#include <drivers/led.h>
#include <drivers/led_strip.h>
#include <drivers/lora.h>
#include <drivers/mbox.h>
#include <drivers/mdio.h>
#include <drivers/peci.h>
/* drivers/pinctrl.h requires SoC specific header */
#include <drivers/pinmux.h>
#include <drivers/pm_cpu_ops.h>
#include <drivers/ps2.h>
#include <drivers/ptp_clock.h>
#include <drivers/pwm.h>
#include <drivers/regulator.h>
/* drivers/reset.h conflicts with assert() for certain platforms */
#include <drivers/sensor.h>
#include <drivers/spi_emul.h>
#include <drivers/spi.h>
#include <drivers/syscon.h>
#include <drivers/uart.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <drivers/video-controls.h>
#include <drivers/video.h>
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
