#include "bitstream.h"

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#include <stdalign.h>

struct fpga_ice40_fixture {
	const struct device *fpga;
};

ZTEST_F(fpga_ice40, test_get_status)
{
	zassert_equal(FPGA_STATUS_INACTIVE, fpga_get_status(fixture->fpga), NULL);
	/* testing FPGA_STATUS_INACTIVE requires fpga_load(), see below */
}

ZTEST_F(fpga_ice40, test_reset)
{
	zassert_ok(fpga_reset(fixture->fpga), NULL);
}

ZTEST_F(fpga_ice40, test_load)
{
	/* BUILD_ASSERT(alignof(bitstream) == alignof(uint32_t)); */
	zassert_ok(fpga_load(fixture->fpga, (uint32_t *)bitstream, sizeof(bitstream)), NULL);
	zassert_equal(FPGA_STATUS_ACTIVE, fpga_get_status(fixture->fpga), NULL);
}

ZTEST_F(fpga_ice40, test_on)
{
	zassert_ok(fpga_on(fixture->fpga), NULL);
}

ZTEST_F(fpga_ice40, test_off)
{
	zassert_ok(fpga_off(fixture->fpga), NULL);
}

ZTEST_F(fpga_ice40, test_get_info)
{
	zassert_not_null(fpga_get_info(fixture->fpga), NULL);
}

static void *fpga_ice40_setup(void)
{
	static struct fpga_ice40_fixture fixture;

	fixture.fpga = DEVICE_DT_GET(DT_NODELABEL(fpga0));

	return &fixture;
}

static void fpga_ice40_before(void *arg)
{
	int ret;
	const struct device *port = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	struct fpga_ice40_fixture *fixture = (struct fpga_ice40_fixture *)arg;

	zassert_true(device_is_ready(fixture->fpga), NULL);
	zassert_ok(fpga_reset(fixture->fpga), NULL);
}

ZTEST_SUITE(fpga_ice40, NULL, fpga_ice40_setup, fpga_ice40_before, NULL, NULL);
