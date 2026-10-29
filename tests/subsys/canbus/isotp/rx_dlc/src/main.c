/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/isotp.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define RX_ID            0x10
#define TX_ID            0x11
#define MSG_LENGTH       100U
#define SF_DATA_LENGTH   7U
#define SF_PCI_BYTE_1    SF_DATA_LENGTH
#define FF_PCI_BYTE_1    (0x10U | (MSG_LENGTH >> 8))
#define FF_PCI_BYTE_2    (MSG_LENGTH & 0xFFU)
#define CF_PCI_BYTE_1    0x21U
#define FILL_BYTE        0xAAU
#define WORK_DELAY_MS    10
#define RECV_TIMEOUT_MS  200

DEFINE_FFF_GLOBALS;

static const struct device *const can_dev = DEVICE_DT_GET(DT_NODELABEL(fake_can));

static const struct isotp_fc_opts fc_opts = {
	.bs = 0,
	.stmin = 0,
};

static const struct isotp_msg_id rx_addr = {
	.std_id = RX_ID,
};

static const struct isotp_msg_id tx_addr = {
	.std_id = TX_ID,
};

static const struct isotp_msg_id rx_addr_fd = {
	.std_id = RX_ID,
	.dl = 64,
	.flags = ISOTP_MSG_FDF,
};

static const struct isotp_msg_id tx_addr_fd = {
	.std_id = TX_ID,
	.dl = 64,
	.flags = ISOTP_MSG_FDF,
};

static struct isotp_recv_ctx recv_ctx;
static can_rx_callback_t rx_callback;
static void *rx_user_data;

static int add_rx_filter_capture(const struct device *dev, can_rx_callback_t callback,
				 void *user_data, const struct can_filter *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(filter);

	rx_callback = callback;
	rx_user_data = user_data;

	return 0;
}

static int get_capabilities_fd(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_FD;

	return 0;
}

static void deliver_frame(uint8_t flags, uint8_t dlc, uint8_t byte0, uint8_t byte1)
{
	struct can_frame frame = {
		.id = RX_ID,
		.flags = flags,
		.dlc = dlc,
	};

	zassert_not_null(rx_callback, "No RX filter added");

	memset(frame.data, FILL_BYTE, sizeof(frame.data));
	frame.data[0] = byte0;
	frame.data[1] = byte1;

	rx_callback(can_dev, &frame, rx_user_data);
	k_msleep(WORK_DELAY_MS);
}

static void bind_ctx(const struct isotp_msg_id *rx, const struct isotp_msg_id *tx)
{
	int ret;

	ret = isotp_bind(&recv_ctx, can_dev, rx, tx, &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "Binding failed (%d)", ret);
}

static int receive(void)
{
	static uint8_t data[MSG_LENGTH];

	return isotp_recv(&recv_ctx, data, sizeof(data), K_MSEC(RECV_TIMEOUT_MS));
}

ZTEST(isotp_rx_dlc, test_classic_ff_dlc_out_of_range)
{
	int ret;

	bind_ctx(&rx_addr, &tx_addr);

	/* DLC 15 on a classical CAN frame means 8 bytes, not 64 */
	deliver_frame(0U, CANFD_MAX_DLC, FF_PCI_BYTE_1, FF_PCI_BYTE_2);
	zassert_equal(fake_can_send_fake.call_count, 0, "FC sent for invalid FF");

	ret = receive();
	zassert_equal(ret, ISOTP_RECV_TIMEOUT, "recv returned %d", ret);

	/* The context still accepts a valid FF */
	deliver_frame(0U, CAN_MAX_DLC, FF_PCI_BYTE_1, FF_PCI_BYTE_2);
	zassert_equal(fake_can_send_fake.call_count, 1, "No FC sent for valid FF");

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_rx_dlc, test_classic_sf_dlc_out_of_range)
{
	int ret;

	bind_ctx(&rx_addr, &tx_addr);

	deliver_frame(0U, CAN_MAX_DLC + 1U, SF_PCI_BYTE_1, FILL_BYTE);

	ret = receive();
	zassert_equal(ret, ISOTP_RECV_TIMEOUT, "recv returned %d", ret);

	deliver_frame(0U, CAN_MAX_DLC, SF_PCI_BYTE_1, FILL_BYTE);

	ret = receive();
	zassert_equal(ret, SF_DATA_LENGTH, "recv returned %d", ret);

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_rx_dlc, test_classic_cf_dlc_out_of_range)
{
	int ret;

	bind_ctx(&rx_addr, &tx_addr);

	deliver_frame(0U, CAN_MAX_DLC, FF_PCI_BYTE_1, FF_PCI_BYTE_2);
	zassert_equal(fake_can_send_fake.call_count, 1, "No FC sent for valid FF");

	deliver_frame(0U, CANFD_MAX_DLC, CF_PCI_BYTE_1, FILL_BYTE);

	ret = receive();
	zassert_equal(ret, ISOTP_N_ERROR, "recv returned %d", ret);

	isotp_unbind(&recv_ctx);
}

ZTEST(isotp_rx_dlc, test_fd_context_classic_dlc_out_of_range)
{
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_CAN_FD_MODE);

	fake_can_get_capabilities_fake.custom_fake = get_capabilities_fd;
	bind_ctx(&rx_addr_fd, &tx_addr_fd);

	/* A classical CAN frame does not carry 64 bytes in a CAN FD context either */
	deliver_frame(0U, CANFD_MAX_DLC, FF_PCI_BYTE_1, FF_PCI_BYTE_2);
	zassert_equal(fake_can_send_fake.call_count, 0, "FC sent for invalid FF");

	deliver_frame(CAN_FRAME_FDF, CANFD_MAX_DLC, FF_PCI_BYTE_1, FF_PCI_BYTE_2);
	zassert_equal(fake_can_send_fake.call_count, 1, "No FC sent for valid FF");

	deliver_frame(0U, CANFD_MAX_DLC, CF_PCI_BYTE_1, FILL_BYTE);

	ret = receive();
	zassert_equal(ret, ISOTP_N_ERROR, "recv returned %d", ret);

	isotp_unbind(&recv_ctx);
}

static void isotp_rx_dlc_before(void *fixture)
{
	ARG_UNUSED(fixture);

	rx_callback = NULL;
	rx_user_data = NULL;
	fake_can_add_rx_filter_fake.custom_fake = add_rx_filter_capture;
}

ZTEST_SUITE(isotp_rx_dlc, NULL, NULL, isotp_rx_dlc_before, NULL, NULL);
