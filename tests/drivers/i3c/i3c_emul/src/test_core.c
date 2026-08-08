/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * M2 core tests for the I3C emulator: attach, DAA, private xfers, CCCs, and
 * the mock_api fault-injection slot.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "i2c_test_target_emul.h"
#include "test_target_emul.h"

#define I3C_BUS DT_NODELABEL(i3c0)
#define TARGET_A DT_NODELABEL(test_target_a)
#define TARGET_B DT_NODELABEL(test_target_b)
#define TARGET_D DT_NODELABEL(test_target_d)
#define TARGET_E DT_NODELABEL(test_target_e)
#define TARGET_I2C DT_NODELABEL(test_i2c_target)

#define TARGET_A_PID		TEST_TARGET_A_PID
#define TARGET_B_PID		TEST_TARGET_B_PID
#define TARGET_B_INIT_DA	TEST_TARGET_B_INIT_DA
#define TARGET_D_PID		TEST_TARGET_D_PID
#define TARGET_E_PID		TEST_TARGET_E_PID

static const struct device *bus = DEVICE_DT_GET(I3C_BUS);
static const struct emul *target_a = EMUL_DT_GET(TARGET_A);
static const struct emul *target_b = EMUL_DT_GET(TARGET_B);
static const struct emul *target_d = EMUL_DT_GET(TARGET_D);

static struct i3c_device_desc *find_desc(uint64_t pid)
{
	struct i3c_device_id id = { .pid = pid };

	return i3c_device_find(bus, &id);
}

ZTEST(i3c_emul_core, test_attach_and_setdasa_static_to_dynamic)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);

	zassert_not_null(desc, "device desc for target A must exist");
	zassert_equal(desc->static_addr, TEST_TARGET_A_STATIC,
		      "static addr expected 0x%02x", TEST_TARGET_A_STATIC);
	zassert_equal(desc->dynamic_addr, TEST_TARGET_A_STATIC,
		      "i3c_bus_init should have promoted static addr to dynamic via SETDASA");
	zassert_equal(test_target_get_dynamic_addr(target_a), TEST_TARGET_A_STATIC,
		      "peripheral should know its dynamic addr");
}

ZTEST(i3c_emul_core, test_do_daa_respects_init_dynamic_addr)
{
	struct i3c_device_desc *desc = find_desc(TARGET_B_PID);

	zassert_not_null(desc, "device desc for target B must exist");
	zassert_equal(desc->static_addr, 0x00, "target B has no static addr");
	zassert_equal(desc->dynamic_addr, TARGET_B_INIT_DA,
		      "DAA should honour assigned-address property");
	zassert_equal(test_target_get_dynamic_addr(target_b), TARGET_B_INIT_DA,
		      "peripheral mirror should match");
}

ZTEST(i3c_emul_core, test_xfers_routes_by_dynamic_addr)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	uint8_t write_buf[3] = {0x00, 0xAA, 0xBB};
	uint8_t read_buf[2] = {0};
	int rc;

	zassert_not_null(desc, "desc");

	rc = i3c_write(desc, write_buf, sizeof(write_buf));
	zassert_ok(rc, "i3c_write failed: %d", rc);
	zassert_equal(test_target_get_reg(target_a, 0), write_buf[1], "reg[0] mismatch");
	zassert_equal(test_target_get_reg(target_a, 1), write_buf[2], "reg[1] mismatch");

	uint8_t cursor = 0;

	rc = i3c_write(desc, &cursor, sizeof(cursor));
	zassert_ok(rc, "i3c_write seek failed: %d", rc);
	rc = i3c_read(desc, read_buf, sizeof(read_buf));
	zassert_ok(rc, "i3c_read failed: %d", rc);
	zassert_equal(read_buf[0], write_buf[1], "read[0] mismatch");
	zassert_equal(read_buf[1], write_buf[2], "read[1] mismatch");
}

ZTEST(i3c_emul_core, test_ccc_getpid_round_trip)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	struct i3c_ccc_getpid resp = {0};
	int rc;

	zassert_not_null(desc, "desc");

	rc = i3c_ccc_do_getpid(desc, &resp);
	zassert_ok(rc, "GETPID failed: %d", rc);
	zassert_equal(sys_get_be48(resp.pid), TARGET_A_PID, "PID round-trip mismatch");
}

static int mock_xfers_eio(const struct emul *target, struct i3c_msg *msgs, uint8_t num_msgs)
{
	ARG_UNUSED(target);
	ARG_UNUSED(msgs);
	ARG_UNUSED(num_msgs);
	return -EIO;
}

ZTEST(i3c_emul_core, test_mock_api_returns_eio)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	static struct i3c_emul_api mock = {
		.xfers = mock_xfers_eio,
	};
	uint8_t buf[1] = {0};
	int rc;

	test_target_install_mock(target_a, &mock);

	rc = i3c_write(desc, buf, sizeof(buf));
	zassert_equal(rc, -EIO, "expected mock to return -EIO, got %d", rc);

	test_target_install_mock(target_a, NULL);
}

ZTEST(i3c_emul_core, test_legacy_i2c_on_i3c_routes_via_i2c_api)
{
	const struct emul *i2c_target = EMUL_DT_GET(TARGET_I2C);
	const uint16_t i2c_addr = DT_PROP_BY_IDX(TARGET_I2C, reg, 0);
	uint8_t write_buf[3] = {0x00, 0x77, 0x88};
	uint8_t read_buf[2] = {0};
	uint8_t cursor = 0;
	int rc;

	/*
	 * Drive the bus's i2c_api.transfer at a legacy-i2c-on-i3c address.
	 * The peripheral implements struct i2c_emul_api the same way every
	 * in-tree i2c sensor emul does, so this proves an existing i2c
	 * sensor emul could be reparented under our i3c emul controller
	 * without sensor-side changes.
	 */
	rc = i2c_write(bus, write_buf, sizeof(write_buf), i2c_addr);
	zassert_ok(rc, "i2c_write failed: %d", rc);
	zassert_equal(i2c_test_target_get_reg(i2c_target, 0), write_buf[1], "reg[0]");
	zassert_equal(i2c_test_target_get_reg(i2c_target, 1), write_buf[2], "reg[1]");

	rc = i2c_write(bus, &cursor, sizeof(cursor), i2c_addr);
	zassert_ok(rc, "i2c_write seek: %d", rc);
	rc = i2c_read(bus, read_buf, sizeof(read_buf), i2c_addr);
	zassert_ok(rc, "i2c_read: %d", rc);
	zassert_equal(read_buf[0], write_buf[1], "read[0]");
	zassert_equal(read_buf[1], write_buf[2], "read[1]");
}

/*
 * Helper: broadcast ENTHDR0 to put the bus in HDR-DDR entry mode. There is
 * no framework wrapper for ENTHDR0, so build the CCC payload directly.
 */
static int enthdr0_broadcast(const struct device *bus_dev)
{
	struct i3c_ccc_payload payload = {
		.ccc.id = I3C_CCC_ENTHDR0,
	};

	return i3c_do_ccc(bus_dev, &payload);
}

ZTEST(i3c_emul_core, test_hdr_ddr_xfer_routes_to_hdr_handler)
{
	struct i3c_device_desc *desc = find_desc(TARGET_D_PID);
	uint8_t write_buf[3] = {0x00, 0x77, 0x88};
	struct i3c_msg msg = {
		.buf = write_buf,
		.len = sizeof(write_buf),
		.flags = I3C_MSG_WRITE | I3C_MSG_HDR | I3C_MSG_STOP,
		.hdr_mode = I3C_MSG_HDR_DDR,
		.hdr_cmd_code = 0x12,
	};
	uint32_t before;
	int rc;

	zassert_not_null(desc, "target D desc");

	/*
	 * Per spec, an HDR-DDR private xfer is only legal after the
	 * controller broadcasts ENTHDR0 to put the bus in HDR-DDR entry
	 * mode. Without it, peripherals must reject the xfer.
	 */
	rc = enthdr0_broadcast(bus);
	zassert_ok(rc, "ENTHDR0 broadcast: %d", rc);

	before = test_target_get_hdr_ddr_xfer_count(target_d);
	rc = i3c_transfer(desc, &msg, 1);
	zassert_ok(rc, "i3c_transfer (HDR-DDR after ENTHDR0): %d", rc);
	zassert_equal(test_target_get_hdr_ddr_xfer_count(target_d), before + 1,
		      "peripheral's hdr_ddr_xfers callback should have fired");
}

ZTEST(i3c_emul_core, test_hdr_ddr_xfer_rejected_without_enthdr0)
{
	struct i3c_device_desc *desc = find_desc(TARGET_D_PID);
	uint8_t write_buf[2] = {0x00, 0xAA};
	struct i3c_msg msg = {
		.buf = write_buf,
		.len = sizeof(write_buf),
		.flags = I3C_MSG_WRITE | I3C_MSG_HDR | I3C_MSG_STOP,
		.hdr_mode = I3C_MSG_HDR_DDR,
		.hdr_cmd_code = 0x34,
	};
	uint32_t before;
	int rc;

	zassert_not_null(desc, "target D desc");

	/*
	 * No ENTHDR0 broadcast first. Bus emulator must reject the
	 * HDR-DDR xfer because the bus is still in SDR mode. Counter
	 * must NOT advance.
	 */
	before = test_target_get_hdr_ddr_xfer_count(target_d);
	rc = i3c_transfer(desc, &msg, 1);
	zassert_not_equal(rc, 0,
			  "HDR-DDR xfer without prior ENTHDR0 must be rejected");
	zassert_equal(test_target_get_hdr_ddr_xfer_count(target_d), before,
		      "peripheral's hdr_ddr_xfers callback must not fire");
}

ZTEST(i3c_emul_core, test_hdr_ddr_xfer_rejected_by_non_hdr_capable_target)
{
	struct i3c_device_desc *desc = find_desc(TARGET_A_PID);
	uint8_t write_buf[2] = {0x00, 0xAA};
	struct i3c_msg msg = {
		.buf = write_buf,
		.len = sizeof(write_buf),
		.flags = I3C_MSG_WRITE | I3C_MSG_HDR | I3C_MSG_STOP,
		.hdr_mode = I3C_MSG_HDR_DDR,
		.hdr_cmd_code = 0x12,
	};
	int rc;

	zassert_not_null(desc, "target A desc");

	/*
	 * Even after ENTHDR0, target A doesn't advertise HDR support
	 * (BCR bit 5 = Advanced Capabilities is clear in DT). The
	 * peripheral must reject the HDR-DDR xfer regardless of bus mode.
	 */
	rc = enthdr0_broadcast(bus);
	zassert_ok(rc, "ENTHDR0 broadcast: %d", rc);

	rc = i3c_transfer(desc, &msg, 1);
	zassert_not_equal(rc, 0,
			  "HDR-DDR to a non-HDR-capable target must fail");
}

ZTEST(i3c_emul_core, test_hdr_ddr_xfer_rejected_when_getcaps_lacks_hdr_ddr)
{
	struct i3c_device_desc *desc = find_desc(TARGET_E_PID);
	uint8_t write_buf[2] = {0x00, 0xAA};
	struct i3c_msg msg = {
		.buf = write_buf,
		.len = sizeof(write_buf),
		.flags = I3C_MSG_WRITE | I3C_MSG_HDR | I3C_MSG_STOP,
		.hdr_mode = I3C_MSG_HDR_DDR,
		.hdr_cmd_code = 0x12,
	};
	int rc;

	zassert_not_null(desc, "target E desc");

	/*
	 * Target E has BCR bit 5 set (Advanced Capabilities — controllers
	 * may issue GETCAPS), but no supports-hdr-ddr property. Its GETCAPS
	 * Format 1 response carries no HDR-mode bits. Bus emul must gate
	 * HDR-DDR on the cached GETCAPS bit, not just BCR — otherwise the
	 * BCR-only check above is too coarse and would let HDR-DDR through
	 * to a non-HDR-capable target that just happens to support GETCAPS.
	 */
	rc = enthdr0_broadcast(bus);
	zassert_ok(rc, "ENTHDR0 broadcast: %d", rc);

	rc = i3c_transfer(desc, &msg, 1);
	zassert_not_equal(rc, 0,
			  "HDR-DDR must be rejected when GETCAPS doesn't advertise it");
}

static void *i3c_emul_setup(void)
{
	int rc = test_target_bus_known_state(bus, TARGET_A_PID, TEST_TARGET_A_STATIC,
					     TARGET_B_PID, TARGET_B_INIT_DA);

	zassert_ok(rc, "test_target_bus_known_state: %d", rc);
	return NULL;
}

ZTEST_SUITE(i3c_emul_core, NULL, i3c_emul_setup, NULL, NULL, NULL);
