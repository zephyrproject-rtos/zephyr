/*
 * Copyright (c) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ztest for the Xilinx AXI IIC driver's configure()/get_config() support
 * across Standard/Fast/Fast+ speeds.
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/i2c.h>

/* Resolve the AXI IIC node by compatible (either core revision). */
#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_xps_iic_2_1)
#define I2C_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(xlnx_xps_iic_2_1)
#elif DT_HAS_COMPAT_STATUS_OKAY(xlnx_xps_iic_2_00_a)
#define I2C_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(xlnx_xps_iic_2_00_a)
#else
#error "No enabled Xilinx AXI IIC (xlnx,xps-iic) node found in devicetree"
#endif

static const struct device *const i2c_dev = DEVICE_DT_GET(I2C_NODE);

ZTEST(i2c_xilinx_axi, test_configure_get_config)
{
	const uint32_t speeds[] = {
		I2C_SPEED_STANDARD,
		I2C_SPEED_FAST,
		I2C_SPEED_FAST_PLUS,
	};

	for (int i = 0; i < ARRAY_SIZE(speeds); i++) {
		uint32_t cfg = 0;
		int rc;

		rc = i2c_configure(i2c_dev, I2C_SPEED_SET(speeds[i]) | I2C_MODE_CONTROLLER);
		zassert_ok(rc, "i2c_configure() failed for speed %u (rc=%d)", speeds[i], rc);

		rc = i2c_get_config(i2c_dev, &cfg);
		zassert_ok(rc, "i2c_get_config() failed (rc=%d)", rc);
		zassert_equal(I2C_SPEED_GET(cfg), speeds[i],
			      "get_config speed mismatch: got %u, want %u",
			      I2C_SPEED_GET(cfg), speeds[i]);
	}
}

static void *i2c_xilinx_axi_setup(void)
{
	zassert_true(device_is_ready(i2c_dev), "AXI IIC device not ready");
	return NULL;
}

ZTEST_SUITE(i2c_xilinx_axi, NULL, i2c_xilinx_axi_setup, NULL, NULL, NULL);
