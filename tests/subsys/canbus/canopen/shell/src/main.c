/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

#define TEST_NODE_ID 0x34U

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static uint8_t app_u8 = 5U;
static int16_t app_i16 = -3;
static uint32_t app_ro = 0x12345678U;
static char app_str[8] = "abc";
static uint8_t app_octet[3] = {0x01U, 0x02U, 0x03U};

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW, app_u8),
	CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_INTEGER16, CANOPEN_OD_RW, app_i16),
	CANOPEN_OD_VAR(0x2002, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RO, app_ro),
	CANOPEN_OD_VAR(0x2003, CANOPEN_OD_TYPE_VISIBLE_STRING, CANOPEN_OD_RW, app_str),
	CANOPEN_OD_VAR(0x2004, CANOPEN_OD_TYPE_OCTET_STRING, CANOPEN_OD_RW, app_octet));

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;
static const struct shell *sh;

static const char *run(const char *cmd, int expected)
{
	size_t size;
	int ret;

	shell_backend_dummy_clear_output(sh);
	ret = shell_execute_cmd(sh, cmd);
	zassert_equal(ret, expected, "'%s' returned %d", cmd, ret);

	return shell_backend_dummy_get_output(sh, &size);
}

static void expect(const char *cmd, int ret, const char *text)
{
	const char *out = run(cmd, ret);

	zassert_not_null(strstr(out, text), "'%s' printed '%s', expected '%s'", cmd, out, text);
}

static void *shell_setup(void)
{
	sh = shell_backend_dummy_get_ptr();
	zassert_not_null(sh);
	zassert_true(WAIT_FOR(shell_ready(sh), 1000000, k_msleep(1)));

	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));

	return NULL;
}

static void shell_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_RESET_COMMUNICATION));
}

ZTEST(canopen_shell, test_list_and_nmt)
{
	expect("canopen list", 0, "0x34 PRE-OPERATIONAL error register 0x00");
	expect("canopen nmt 0x34 start", 0, "");
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_OPERATIONAL);
	expect("canopen list", 0, "0x34 OPERATIONAL");
	expect("canopen nmt 52 stop", 0, "");
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_STOPPED);
	expect("canopen nmt 0x34 jump", -EINVAL, "unknown command: jump");
	expect("canopen nmt 0x35 start", -EINVAL, "no local node 0x35");
	expect("canopen nmt 0x34 preop", 0, "");
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_PRE_OPERATIONAL);
}

ZTEST(canopen_shell, test_read)
{
	expect("canopen read 0x34 0x2000 0", 0, "5 (0x05)");
	expect("canopen read 0x34 0x2001 0", 0, "-3 (0xfffd)");
	expect("canopen read 0x34 0x2002 0", 0, "305419896 (0x12345678)");
	expect("canopen read 0x34 0x2003 0", 0, "\"abc\"");
	expect("canopen read 0x34 0x2004 0", 0, "01 02 03");
	expect("canopen read 0x34 0x1018 1", 0, "0 (0x00000000)");
	expect("canopen read 0x34 0x1008 0", 0, "\"Zephyr\"");
	expect("canopen read 0x34 0x2005 0", -EIO, "abort 0x06020000");
	expect("canopen read 0x34 0x2000 1", -EIO, "abort 0x06090011");
	expect("canopen read 0x34 0x20000 0", -EINVAL, "invalid value: 0x20000");
}

ZTEST(canopen_shell, test_write)
{
	expect("canopen write 0x34 0x2000 0 200", 0, "");
	zassert_equal(app_u8, 200U);
	expect("canopen write 0x34 0x2000 0 256", -EINVAL, "invalid value: 256");
	expect("canopen write 0x34 0x2001 0 -32768", 0, "");
	zassert_equal(app_i16, -32768);
	expect("canopen write 0x34 0x2001 0 -32769", -EINVAL, "invalid value");
	expect("canopen write 0x34 0x2003 0 xyz12", 0, "");
	zassert_mem_equal(app_str, "xyz12", 5);
	expect("canopen write 0x34 0x2004 0 a1b2c3", 0, "");
	zassert_equal(app_octet[2], 0xC3U);
	expect("canopen write 0x34 0x2004 0 a1b2", -EIO, "abort 0x06070013");
	expect("canopen write 0x34 0x2002 0 1", -EIO, "abort 0x06010002");
	expect("canopen write 0x34 0x1017 0 250", 0, "");
	expect("canopen read 0x34 0x1017 0", 0, "250 (0x00fa)");
	expect("canopen write 0x34 0x1014 0 0x701", -EIO, "abort 0x06090030");
}

ZTEST(canopen_shell, test_emcy)
{
	expect("canopen emcy 0x34", 0, "error register 0x00, 0 errors");
	zassert_ok(canopen_emcy_report(&test_node, 0x5000U, 0x01U, NULL));
	expect("canopen emcy 0x34", 0, "1: 0x00005000");
	zassert_ok(canopen_emcy_clear(&test_node, 0x5000U));
}

ZTEST(canopen_shell, test_sdo)
{
	expect("canopen sdo read 0x34 0x34 0x2002 0", 0, "78 56 34 12");
	expect("canopen sdo read 0x34 0x34 0x1008 0", 0, "5a 65 70 68 79 72");
	expect("canopen sdo write 0x34 0x34 0x2000 0 u8 77", 0, "");
	zassert_equal(app_u8, 77U);
	expect("canopen sdo write 0x34 0x34 0x2003 0 str hello", 0, "");
	zassert_mem_equal(app_str, "hello", 5);
	expect("canopen sdo write 0x34 0x34 0x2002 0 u32 1", -ECANCELED, "abort 0x06010002");
	expect("canopen sdo write 0x34 0x34 0x2000 0 u16 1", -ECANCELED, "abort 0x06070012");
	expect("canopen sdo write 0x34 0x34 0x2000 0 f32 1", -EINVAL, "unknown type: f32");
	(void)run("canopen sdo read 0x34 0x05 0x1000 0", -ETIMEDOUT);
}

ZTEST(canopen_shell, test_nmt_send)
{
	expect("canopen nmt 0x34 start 0", 0, "");
	zassert_true(WAIT_FOR(canopen_nmt_get_state(&test_node) == CANOPEN_NMT_OPERATIONAL,
			      100000, k_msleep(1)));
	expect("canopen nmt 0x34 stop 0x34", 0, "");
	zassert_true(WAIT_FOR(canopen_nmt_get_state(&test_node) == CANOPEN_NMT_STOPPED, 100000,
			      k_msleep(1)));
	expect("canopen nmt 0x34 start 128", -EINVAL, "invalid value: 128");
}

ZTEST_SUITE(canopen_shell, NULL, shell_setup, shell_before, NULL, NULL);
