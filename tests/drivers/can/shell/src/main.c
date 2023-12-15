/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

/**
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_api test_can_shell
 * @}
 */

#define FAKE_CAN_NAME DEVICE_DT_NAME(DT_NODELABEL(fake_can))

/* Global variables */
static const struct device *const fake_can_dev = DEVICE_DT_GET(DT_NODELABEL(fake_can));
static struct can_timing timing_capture;
static struct can_filter filter_capture;
static struct can_frame frame_capture;
DEFINE_FFF_GLOBALS;

static void assert_can_timing_equal(const struct can_timing *t1, const struct can_timing *t2)
{
	zassert_equal(t1->sjw, t2->sjw, "sjw mismatch");
	zassert_equal(t1->prop_seg, t2->prop_seg, "prop_seg mismatch");
	zassert_equal(t1->phase_seg1, t2->phase_seg1, "hase_seg1 mismatch");
	zassert_equal(t1->phase_seg2, t2->phase_seg2, "phase_seg2 mismatch");
	zassert_equal(t1->prescaler, t2->prescaler, "prescaler mismatch");
}

static void assert_can_filter_equal(const struct can_filter *f1, const struct can_filter *f2)
{
	zassert_equal(f1->flags, f2->flags, "flags mismatch");
	zassert_equal(f1->id, f2->id, "id mismatch");
	zassert_equal(f1->mask, f2->mask, "mask mismatch");
}

static void assert_can_frame_equal(const struct can_frame *f1, const struct can_frame *f2)
{
	zassert_equal(f1->flags, f2->flags, "flags mismatch");
	zassert_equal(f1->id, f2->id, "id mismatch");
	zassert_equal(f1->dlc, f2->dlc, "dlc mismatch");
	zassert_mem_equal(f1->data, f2->data, can_dlc_to_bytes(f1->dlc), "data mismatch");
}

static int can_shell_test_capture_timing(const struct device *dev, const struct can_timing *timing)
{
	ARG_UNUSED(dev);

	memcpy(&timing_capture, timing, sizeof(timing_capture));

	return 0;
}

static int can_shell_test_capture_filter(const struct device *dev, can_rx_callback_t callback,
					 void *user_data, const struct can_filter *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	memcpy(&filter_capture, filter, sizeof(filter_capture));

	return 0;
}

static int can_shell_test_capture_frame(const struct device *dev, const struct can_frame *frame,
					k_timeout_t timeout, can_tx_callback_t callback,
					void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timeout);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	memcpy(&frame_capture, frame, sizeof(frame_capture));

	return 0;
}

ZTEST(can_shell, test_can_start)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can start " FAKE_CAN_NAME);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_start_fake.call_count, 1, "start function not called");
}

ZTEST(can_shell, test_can_stop)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can stop " FAKE_CAN_NAME);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_stop_fake.call_count, 1, "stop function not called");
}

ZTEST(can_shell, test_can_show)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can show " FAKE_CAN_NAME);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_get_max_filters_fake.call_count, 2,
		      "get_max_filters function not called twice");
	zassert_equal(fake_can_get_capabilities_fake.call_count, 1,
		      "get_capabilities function not called");
	zassert_equal(fake_can_get_state_fake.call_count, 1, "get_state function not called");
}

ZTEST(can_shell, test_can_bitrate_missing_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can bitrate " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without bitrate");
	zassert_equal(fake_can_set_timing_fake.call_count, 0, "set_timing function called");
}

static void can_shell_test_bitrate(const char *cmd, uint32_t expected_bitrate,
				   uint16_t expected_sample_pnt)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	struct can_timing expected = { 0 };
	int err;

	err = can_calc_timing(fake_can_dev, &expected, expected_bitrate, expected_sample_pnt);
	zassert_ok(err, "failed to calculate reference timing (err %d)", err);

	fake_can_set_timing_fake.custom_fake = can_shell_test_capture_timing;

	err = shell_execute_cmd(sh, cmd);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_set_timing_fake.call_count, 1, "set_timing function not called");
	zassert_equal(fake_can_set_timing_fake.arg0_val, fake_can_dev, "wrong device pointer");
	assert_can_timing_equal(&expected, &timing_capture);
}

ZTEST(can_shell, test_can_bitrate)
{
	can_shell_test_bitrate("can bitrate " FAKE_CAN_NAME " 125000", 125000, 875);
}

ZTEST(can_shell, test_can_bitrate_sample_point)
{
	can_shell_test_bitrate("can bitrate " FAKE_CAN_NAME " 125000 750", 125000, 750);
}

ZTEST(can_shell, test_can_dbitrate_missing_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_CAN_FD_MODE);

	err = shell_execute_cmd(sh, "can dbitrate " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without dbitrate");
	zassert_equal(fake_can_set_timing_data_fake.call_count, 0,
		      "set_timing_data function called");
}

static void can_shell_test_dbitrate(const char *cmd, uint32_t expected_bitrate,
				    uint16_t expected_sample_pnt)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	struct can_timing expected = { 0 };
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_CAN_FD_MODE);

	err = can_calc_timing_data(fake_can_dev, &expected, expected_bitrate, expected_sample_pnt);
	zassert_ok(err, "failed to calculate reference timing (err %d)", err);

	fake_can_set_timing_data_fake.custom_fake = can_shell_test_capture_timing;

	err = shell_execute_cmd(sh, cmd);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_set_timing_data_fake.call_count, 1,
		      "set_timing_data function not called");
	zassert_equal(fake_can_set_timing_data_fake.arg0_val, fake_can_dev, "wrong device pointer");
	assert_can_timing_equal(&expected, &timing_capture);
}

ZTEST(can_shell, test_can_dbitrate)
{
	can_shell_test_dbitrate("can dbitrate " FAKE_CAN_NAME " 1000000", 1000000, 750);
}

ZTEST(can_shell, test_can_dbitrate_sample_point)
{
	can_shell_test_dbitrate("can dbitrate " FAKE_CAN_NAME " 1000000 875", 1000000, 875);
}

ZTEST(can_shell, test_can_timing)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	struct can_timing expected = {
		.sjw = 1U,
		.prop_seg = 2U,
		.phase_seg1 = 3U,
		.phase_seg2 = 4U,
		.prescaler = 5U,
	};
	int err;

	fake_can_set_timing_fake.custom_fake = can_shell_test_capture_timing;

	err = shell_execute_cmd(sh, "can timing " FAKE_CAN_NAME " 1 2 3 4 5");
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_set_timing_fake.call_count, 1, "set_timing function not called");
	zassert_equal(fake_can_set_timing_fake.arg0_val, fake_can_dev, "wrong device pointer");
	assert_can_timing_equal(&expected, &timing_capture);
}

ZTEST(can_shell, test_can_timing_missing_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_CAN_FD_MODE);

	err = shell_execute_cmd(sh, "can timing " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without timing");
	zassert_equal(fake_can_set_timing_fake.call_count, 0,
		      "set_timing function called");
}

ZTEST(can_shell, test_can_dtiming)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	struct can_timing expected = {
		.sjw = 1U,
		.prop_seg = 2U,
		.phase_seg1 = 3U,
		.phase_seg2 = 4U,
		.prescaler = 5U,
	};
	int err;

	fake_can_set_timing_data_fake.custom_fake = can_shell_test_capture_timing;

	err = shell_execute_cmd(sh, "can dtiming " FAKE_CAN_NAME " 1 2 3 4 5");
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_set_timing_data_fake.call_count, 1,
		      "set_timing_data function not called");
	zassert_equal(fake_can_set_timing_data_fake.arg0_val, fake_can_dev, "wrong device pointer");
	assert_can_timing_equal(&expected, &timing_capture);
}

ZTEST(can_shell, test_can_dtiming_missing_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	Z_TEST_SKIP_IFNDEF(CONFIG_CAN_FD_MODE);

	err = shell_execute_cmd(sh, "can dtiming " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without dtiming");
	zassert_equal(fake_can_set_timing_data_fake.call_count, 0,
		      "set_timing_data function called");
}

ZTEST(can_shell, test_can_mode_missing_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can mode " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without mode value");
	zassert_equal(fake_can_set_mode_fake.call_count, 0, "set_mode function called");
}

ZTEST(can_shell, test_can_mode_unknown)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can mode " FAKE_CAN_NAME " foobarbaz");
	zassert_not_equal(err, 0, " executed shell command with unknown mode value");
	zassert_equal(fake_can_set_mode_fake.call_count, 0, "set_mode function called");
}

static void can_shell_test_mode(const char *cmd, can_mode_t expected)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, cmd);
	zassert_ok(err, "failed to execute shell command (err %d)", err);

	zassert_equal(fake_can_set_mode_fake.call_count, 1, "set_mode function not called");
	zassert_equal(fake_can_set_mode_fake.arg0_val, fake_can_dev, "wrong device pointer");
	zassert_equal(fake_can_set_mode_fake.arg1_val, expected, "wrong mode value");
}

ZTEST(can_shell, test_can_mode_raw_value)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " 0xaabbccdd", 0xaabbccdd);
}

ZTEST(can_shell, test_can_mode_fd)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " fd", CAN_MODE_FD);
}

ZTEST(can_shell, test_can_mode_listen_only)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " listen-only", CAN_MODE_LISTENONLY);
}

ZTEST(can_shell, test_can_mode_loopback)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " loopback", CAN_MODE_LOOPBACK);
}

ZTEST(can_shell, test_can_mode_normal)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " normal", CAN_MODE_NORMAL);
}

ZTEST(can_shell, test_can_mode_one_shot)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " one-shot", CAN_MODE_ONE_SHOT);
}

ZTEST(can_shell, test_can_mode_triple_sampling)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " triple-sampling", CAN_MODE_3_SAMPLES);
}

ZTEST(can_shell, test_can_mode_combined)
{
	can_shell_test_mode("can mode " FAKE_CAN_NAME " listen-only loopback",
			    CAN_MODE_LISTENONLY | CAN_MODE_LOOPBACK);
}

ZTEST(can_shell, test_can_send_missing_id)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can send " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without CAN ID");
	zassert_equal(fake_can_send_fake.call_count, 0,
		      "send function called");
}

static void can_shell_test_send(const char *cmd, const struct can_frame *expected)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	fake_can_send_fake.custom_fake = can_shell_test_capture_frame;

	err = shell_execute_cmd(sh, cmd);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_send_fake.call_count, 1, "send function not called");
	zassert_equal(fake_can_send_fake.arg0_val, fake_can_dev, "wrong device pointer");
	assert_can_frame_equal(expected, &frame_capture);
}

ZTEST(can_shell, test_can_send_std_id)
{
	const struct can_frame expected = {
		.flags = 0,
		.id = 0x010,
		.dlc = can_bytes_to_dlc(2),
		.data = { 0xaa, 0x55 },
	};

	can_shell_test_send("can send " FAKE_CAN_NAME " 010 aa 55", &expected);
}

ZTEST(can_shell, test_can_send_ext_id)
{
	const struct can_frame expected = {
		.flags = CAN_FRAME_IDE,
		.id = 0x1024,
		.dlc = can_bytes_to_dlc(4),
		.data = { 0xde, 0xad, 0xbe, 0xef },
	};

	can_shell_test_send("can send " FAKE_CAN_NAME " -e 1024 de ad be ef", &expected);
}

ZTEST(can_shell, test_can_send_no_data)
{
	const struct can_frame expected = {
		.flags = 0,
		.id = 0x133,
		.dlc = can_bytes_to_dlc(0),
		.data = { },
	};

	can_shell_test_send("can send " FAKE_CAN_NAME " 133", &expected);
}

ZTEST(can_shell, test_can_send_rtr)
{
	const struct can_frame expected = {
		.flags = CAN_FRAME_RTR,
		.id = 0x7ff,
		.dlc = can_bytes_to_dlc(0),
		.data = { },
	};

	can_shell_test_send("can send " FAKE_CAN_NAME " -r 7ff", &expected);
}

ZTEST(can_shell, test_can_send_fd)
{
	const struct can_frame expected = {
		.flags = CAN_FRAME_FDF,
		.id = 0x123,
		.dlc = can_bytes_to_dlc(8),
		.data = { 0xaa, 0x55, 0xaa, 0x55, 0x11, 0x22, 0x33, 0x44 },
	};

	can_shell_test_send("can send " FAKE_CAN_NAME " -f 123 aa 55 aa 55 11 22 33 44", &expected);
}

ZTEST(can_shell, test_can_send_fd_brs)
{
	const struct can_frame expected = {
		.flags = CAN_FRAME_FDF | CAN_FRAME_BRS,
		.id = 0x321,
		.dlc = can_bytes_to_dlc(7),
		.data = { 0xaa, 0x55, 0xaa, 0x55, 0x11, 0x22, 0x33 },
	};

	can_shell_test_send("can send " FAKE_CAN_NAME " -f -b 321 aa 55 aa 55 11 22 33", &expected);
}

ZTEST(can_shell, test_can_send_data_all_options)
{
	const struct can_frame expected = {
		.flags = CAN_FRAME_IDE | CAN_FRAME_FDF | CAN_FRAME_BRS | CAN_FRAME_RTR,
		.id = 0x1024,
		.dlc = can_bytes_to_dlc(0),
		.data = { },
	};

	can_shell_test_send("can send " FAKE_CAN_NAME " -r -e -f -b 1024", &expected);
}

ZTEST(can_shell, test_can_filter_add_missing_id)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can filter add " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without CAN ID");
	zassert_equal(fake_can_add_rx_filter_fake.call_count, 0,
		      "add_rx_filter function called");
}

static void can_shell_test_filter_add(const char *cmd, const struct can_filter *expected)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	fake_can_add_rx_filter_fake.custom_fake = can_shell_test_capture_filter;

	err = shell_execute_cmd(sh, cmd);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(fake_can_add_rx_filter_fake.call_count, 1,
		      "add_rx_filter function not called");
	zassert_equal(fake_can_add_rx_filter_fake.arg0_val, fake_can_dev, "wrong device pointer");
	assert_can_filter_equal(expected, &filter_capture);
}

ZTEST(can_shell, test_can_filter_add_std_id)
{
	struct can_filter expected = {
		.flags = 0U,
		.id = 0x010,
		.mask = CAN_STD_ID_MASK,
	};

	can_shell_test_filter_add("can filter add " FAKE_CAN_NAME " 010", &expected);
}

ZTEST(can_shell, test_can_filter_add_std_id_mask)
{
	struct can_filter expected = {
		.flags = 0U,
		.id = 0x010,
		.mask = 0x020,
	};

	can_shell_test_filter_add("can filter add " FAKE_CAN_NAME " 010 020", &expected);
}

ZTEST(can_shell, test_can_filter_add_ext_id)
{
	struct can_filter expected = {
		.flags = CAN_FILTER_IDE,
		.id = 0x1024,
		.mask = CAN_EXT_ID_MASK,
	};

	can_shell_test_filter_add("can filter add " FAKE_CAN_NAME " -e 1024", &expected);
}

ZTEST(can_shell, test_can_filter_add_ext_id_mask)
{
	struct can_filter expected = {
		.flags = CAN_FILTER_IDE,
		.id = 0x1024,
		.mask = 0x2048,
	};

	can_shell_test_filter_add("can filter add " FAKE_CAN_NAME " -e 1024 2048", &expected);
}

ZTEST(can_shell, test_can_filter_add_all_options)
{
	struct can_filter expected = {
		.flags = CAN_FILTER_IDE,
		.id = 0x2048,
		.mask = 0x4096,
	};

	can_shell_test_filter_add("can filter add " FAKE_CAN_NAME " -e 2048 4096", &expected);
}

ZTEST(can_shell, test_can_filter_remove_missing_value)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can filter remove " FAKE_CAN_NAME);
	zassert_not_equal(err, 0, " executed shell command without filter ID");
	zassert_equal(fake_can_remove_rx_filter_fake.call_count, 0,
		      "remove_rx_filter function called");
}

ZTEST(can_shell, test_can_filter_remove)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "can filter remove " FAKE_CAN_NAME " 1234");
	zassert_ok(err, "failed to execute shell command (err %d)", err);

	zassert_equal(fake_can_remove_rx_filter_fake.call_count, 1,
		      "remove_rx_filter function not called");
	zassert_equal(fake_can_remove_rx_filter_fake.arg0_val, fake_can_dev,
		      "wrong device pointer");
	zassert_equal(fake_can_remove_rx_filter_fake.arg1_val, 1234, "wrong filter ID");
}

static void can_shell_test_recover(const char *cmd, k_timeout_t expected)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	Z_TEST_SKIP_IFDEF(CONFIG_CAN_AUTO_BUS_OFF_RECOVERY);

	err = shell_execute_cmd(sh, cmd);
	zassert_ok(err, "failed to execute shell command (err %d)", err);

	zassert_equal(fake_can_recover_fake.call_count, 1, "recover function not called");
	zassert_equal(fake_can_recover_fake.arg0_val, fake_can_dev, "wrong device pointer");
	zassert_true(K_TIMEOUT_EQ(fake_can_recover_fake.arg1_val, expected),
		     "wrong timeout value");
}

ZTEST(can_shell, test_can_recover)
{
	can_shell_test_recover("can recover " FAKE_CAN_NAME, K_FOREVER);
}

ZTEST(can_shell, test_can_recover_timeout)
{
	can_shell_test_recover("can recover " FAKE_CAN_NAME " 100", K_MSEC(100));
}

static void can_shell_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&timing_capture, 0, sizeof(timing_capture));
	memset(&filter_capture, 0, sizeof(filter_capture));
	memset(&frame_capture, 0, sizeof(frame_capture));
}

static void *can_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(can_shell, NULL, can_shell_setup, can_shell_before, NULL, NULL);
