/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/uart/serial_test.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_DBG);

/* Create a mock controller for the DUT to talk to and us to control. {{{ */

struct mock_controller_data {
	bt_hci_recv_t c2h_send;
};

/* These are the callbacks for the mock Controller. */
static int mock_drv_send(const struct device *dev, struct net_buf *buf);
static int mock_drv_open(const struct device *dev, bt_hci_recv_t recv);
static DEVICE_API(bt_hci, drv_api) = {
	.open = mock_drv_open,
	.send = mock_drv_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct mock_controller_data mock_controller_##inst = {};                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &mock_controller_##inst, NULL, POST_KERNEL,        \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv_api)

/* This instantiates and connects an instance of our mock Bluetooth Controller for each DT node that
 * is compatible with `zephyr_bt_hci_test`.
 *
 * In this test, `../test.overlay` defines just a single `zephyr_bt_hci_test` device and chooses it
 * as `DT_CHOSEN(zephyr_bt_hci)`, which is the chosen-node the DUT uses as to find the Bluetooth
 * Controller to use.
 */
#define DT_DRV_COMPAT zephyr_bt_hci_mock_controller
DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

/* }}} */

/* Setup the virtual UART. {{{ */

/* This is the same UART device lookup as done by the DUT, giving us a reference to the same
 * device. `../app.overlay` has selected this to be the mock UART, on which we can emulate UART
 * activity using the `serial_vnd`-functions.
 */
static const struct device *const mock_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_c2h_uart));

static void uart_c2h_cb(const struct device *dev, void *user_data);
static int virtual_uart_setup(void)
{
	/* Connect the "wires" of the virtual UART to our virtual UART transceiver. */
	serial_vnd_set_callback(mock_uart, uart_c2h_cb, NULL);
	return 0;
}

SYS_INIT(virtual_uart_setup, APPLICATION, 0);

/* }}} */

/* Start the DUT "main thread". The settings for this thread are selected as
 * true as possible to the real main thread. {{{
 */
static struct k_thread hci_uart_thread;
static K_THREAD_PINNED_STACK_DEFINE(hci_uart_thread_stack, CONFIG_MAIN_STACK_SIZE);
static void hci_uart_thread_entry(void *p1, void *p2, void *p3)
{
	extern void hci_uart_main(void);
	hci_uart_main();
}
static int sys_init_spawn_hci_uart(void)
{
	k_thread_create(&hci_uart_thread, hci_uart_thread_stack,
			K_THREAD_STACK_SIZEOF(hci_uart_thread_stack), hci_uart_thread_entry, NULL,
			NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&hci_uart_thread, "hci_uart_main");
	return 0;
}
SYS_INIT(sys_init_spawn_hci_uart, POST_KERNEL, 64);
/* }}} */

/* This is the same lookup for the Bluetooth Controller that the DUT uses. */
#define BT_HCI_NODE DT_CHOSEN(zephyr_bt_hci)
const struct device *const mock_controller = DEVICE_DT_GET(BT_HCI_NODE);

BUILD_ASSERT(DT_NODE_HAS_COMPAT(BT_HCI_NODE, DT_DRV_COMPAT),
	     "Bluetooth Controller DT node is not our mock");

/* Mock controller callbacks. {{{ */

static int mock_drv_open(const struct device *dev, bt_hci_recv_t recv)
{
	__ASSERT(dev == mock_controller, "Unknown mock Bluetooth Controller. The test framework "
					 "only supports a single mock controller.");

	struct mock_controller_data *drv = dev->data;

	LOG_DBG("recv %p", recv);

	drv->c2h_send = recv;

	return 0;
}

/** This FIFO holds the references to all h2c packets the DUT has sent
 *  to the controller using #bt_send.
 *
 *  Each test should mock a controller by calling #net_buf_get on this
 *  FIFO and simulate a controller's #bt_hci_driver::drv_send. The mocks
 *  should use #bt_recv to send c2h packets to the DUT.
 */
K_FIFO_DEFINE(mock_ctlr_h2c); /* elem T: net_buf */
static int mock_drv_send(const struct device *dev, struct net_buf *buf)
{
	__ASSERT(dev == mock_controller, "Unknown mock Bluetooth Controller. The test framework "
					 "only supports a single mock controller.");

	__ASSERT_NO_MSG(buf);

	LOG_DBG("buf %p type %d len %u", buf, bt_buf_get_type(buf), buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "buf");

	k_fifo_put(&mock_ctlr_h2c, buf);
	return 0;
}

/* }}} */

/* Mock UART c2h TX handler. {{{ */

K_SEM_DEFINE(uart_c2h_read_available, 0, 1);

static void uart_c2h_read(uint8_t *data, uint32_t size, k_timeout_t timeout)
{
	uint32_t ret;

	while (size > 0) {
		int err;

		err = k_sem_take(&uart_c2h_read_available, timeout);
		__ASSERT(err == 0, "Serial read timeout %d", err);

		ret = serial_vnd_read_out_data(mock_uart, data, size);
		__ASSERT(ret <= size,
			 "serial_vnd_read_out_data ret %u > size %u. Is this an unsigned negative "
			 "errno (%d)?",
			 ret, size, ret);

		LOG_HEXDUMP_DBG(data, ret, "uart tx");

		data += ret;
		size -= ret;
	}
}

static void uart_c2h_cb(const struct device *dev, void *user_data)
{
	LOG_DBG("uart tx available");
	k_sem_give(&uart_c2h_read_available);
}

/* }}} */

#define HCI_NORMAL_CMD_BUF_COUNT       (CONFIG_BT_BUF_CMD_TX_COUNT - 1)
#define TEST_PARAM_HOST_COMPLETE_COUNT 10
#define TIMEOUT_PRESUME_STUCK          K_SECONDS(1)

/** Corresponds to:
 *      - #bt_hci_cmd_hdr
 */
const uint8_t h4_msg_cmd_dummy1[] = {
	0x01,       /* H4: opcode = CMD */
	0x01, 0x00, /* H4: CMD: opcode = 1 */
	0x00,       /* H4: CMD: len = 0 */
};

/** Corresponds to:
 *      - #bt_hci_cmd_hdr
 *      - #bt_hci_cp_host_num_completed_packets
 */
const uint8_t h4_msg_cmd_host_num_complete[] = {
	0x01,       /* H4: opcode = CMD */
	0x35, 0x0c, /* H4: CMD: opcode = BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS */
	0x05,       /* H4: CMD: len = 5 */
	0x01,       /* H4: CMD: num_handles = 1 */
	0x00, 0x00, /* H4: CMD: connection_handle = 0 */
	0x01, 0x00, /* H4: CMD: num_complete = 1 */
};

/** Corresponds to:
 *      - #bt_hci_evt_hdr
 *      - #bt_hci_evt_cmd_complete
 */
const uint8_t hci_msg_rx_evt_cmd_complete[] = {
	BT_HCI_EVT_CMD_COMPLETE, /* EVT: opcode */
	0x03,                    /* EVT: len */
	0x01,                    /* EVT: CMDC: ncmd = 1 */
	/* EVT: CMDC: opcode */
	0x00,
	0x00,
};

ZTEST_SUITE(hci_uart, NULL, NULL, NULL, NULL, NULL);
ZTEST(hci_uart, test_h2c_cmd_flow_control)
{
	/* This test assumes the DUT does not care about the contents of
	 * the HCI messages, other than the HCI type/endpoint and the
	 * size. This allows the test to cheat and skip the HCI Reset,
	 * connection setup etc and use dummy command-packets.
	 */

	/* Send commands, saturating the controller's command pipeline. */
	for (uint16_t i = 0; i < HCI_NORMAL_CMD_BUF_COUNT; i++) {
		int write_size = serial_vnd_queue_in_data(mock_uart, h4_msg_cmd_dummy1,
							  sizeof(h4_msg_cmd_dummy1));
		__ASSERT_NO_MSG(write_size == sizeof(h4_msg_cmd_dummy1));
	}

	/* At this point, the HCI flow control limit for the cmd
	 * endpoint has been reached. It will remain so until the
	 * controller mock has sent a 'HCI Command Complete' event.
	 *
	 * But the 'HCI Host Number of Completed Packets' command is
	 * exempt from HCI flow control. (It's like it has its own
	 * endpoint, that has no flow control.)
	 *
	 * We now send several 'HCI Host Number of Completed Packets'
	 * packets before handling any commands in the controller. This
	 * tests whether the DUT is able to engage the lower transport
	 * flow controller (i.e. UART flow-control) or somehow handle
	 * the special packets out-of-order in real-time.
	 */
	for (uint16_t i = 0; i < TEST_PARAM_HOST_COMPLETE_COUNT; i++) {
		int write_size = serial_vnd_queue_in_data(mock_uart, h4_msg_cmd_host_num_complete,
							  sizeof(h4_msg_cmd_host_num_complete));
		__ASSERT_NO_MSG(write_size == sizeof(h4_msg_cmd_host_num_complete));
	}

	LOG_DBG("All h2c packets queued on UART");

	/* Then, we check that all packets are delivered without loss. */

	/* Expect all the normal commands first. */
	for (uint16_t i = 0; i < HCI_NORMAL_CMD_BUF_COUNT; i++) {
		/* The mock controller processes a command. */
		{
			struct net_buf *buf = k_fifo_get(&mock_ctlr_h2c, TIMEOUT_PRESUME_STUCK);

			zassert_not_null(buf);
			zassert_equal(buf->len, sizeof(h4_msg_cmd_dummy1) - 1, "Wrong length");
			zassert_mem_equal(buf->data, &h4_msg_cmd_dummy1[1],
					  sizeof(h4_msg_cmd_dummy1) - 1);
			net_buf_unref(buf);
		}

		/* The controller sends a HCI Command Complete response. */
		{
			const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
			struct mock_controller_data *drv = dev->data;
			int err;
			struct net_buf *buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);

			zassert_not_null(buf);
			net_buf_add_mem(buf, hci_msg_rx_evt_cmd_complete,
					sizeof(hci_msg_rx_evt_cmd_complete));
			err = drv->c2h_send(dev, buf);
			zassert_equal(err, 0, "bt_recv failed");
		}
	}

	/* Expect all the 'HCI Host Number of Completed Packets'. */
	for (uint16_t i = 0; i < TEST_PARAM_HOST_COMPLETE_COUNT; i++) {
		/* The mock controller processes a 'HCI Host Number of Completed Packets'. */
		{
			struct net_buf *buf = k_fifo_get(&mock_ctlr_h2c, TIMEOUT_PRESUME_STUCK);

			zassert_not_null(buf);
			zassert_equal(buf->len, sizeof(h4_msg_cmd_host_num_complete) - 1,
				      "Wrong length");
			zassert_mem_equal(buf->data, &h4_msg_cmd_host_num_complete[1],
					  sizeof(h4_msg_cmd_dummy1) - 2);
			net_buf_unref(buf);
		}

		/* There is no response to 'HCI Host Number of Completed Packets'. */
	}

	LOG_DBG("All h2c packets received by controller.");
}

ZTEST(hci_uart, test_hw_error_is_generated_when_garbage_on_wire)
{
	/* When the H2C UART receives garbage, the H4 transport should generate a HW error.
	 * Otherwise there is no way for the neither the Host nor Controller to know there has been
	 * an error.
	 */

	static const uint8_t h4_reset[] = {0x01, 0x03, 0x0C, 0x00};
	static const uint8_t h4_reset_complete[] = {
		0x04,                    /* H4: opcode = EVT */
		BT_HCI_EVT_CMD_COMPLETE, /* EVT: opcode */
		0x04,                    /* EVT: len */
		0x01,                    /* EVT: CMDC: ncmd = 1 */
		/* EVT: CMDC: opcode */
		0x0C,
		0x00,
		/* EVT: CMDC: Reset: Status */
		0x00,
	};

	LOG_INF("Send some garbage to the H2C UART.");
	{
		static const uint8_t garbage[] = {
			0xAB, /* H4: opcode = invalid! */
		};
		int write_size;

		write_size = serial_vnd_queue_in_data(mock_uart, garbage, sizeof(garbage));
		__ASSERT_NO_MSG(write_size == sizeof(garbage));
	}

	LOG_INF("Read the C2H serial and verify the DUT sent a HW error.");
	{
		static const uint8_t h4_hw_error[] = {
			0x04, /* H4: opcode = EVT */
			0x10, /* H4: EVT: opcode = BT_HCI_EVT_HARDWARE_ERROR */
			0x01, /* H4: EVT: len = 1 */
			0x00, /* H4: EVT: HW ERR: hardware_code = 0 */
		};
		uint8_t h4_hw_error_recv[sizeof(h4_hw_error)];

		uart_c2h_read(h4_hw_error_recv, sizeof(h4_hw_error_recv), TIMEOUT_PRESUME_STUCK);
		zassert_mem_equal(h4_hw_error_recv, h4_hw_error, sizeof(h4_hw_error), "Wrong data");
	}

	LOG_INF("Resynchronize H4 transport by sending the reset command before exiting this "
		"test.");
	{
		int write_size;

		write_size = serial_vnd_queue_in_data(mock_uart, h4_reset, sizeof(h4_reset));
		__ASSERT_NO_MSG(write_size == sizeof(h4_reset));
	}

	LOG_INF("The mock controller receives the reset command.");
	{
		struct net_buf *buf;

		buf = k_fifo_get(&mock_ctlr_h2c, TIMEOUT_PRESUME_STUCK);
		zassert_not_null(buf);
		zassert_equal(buf->len, sizeof(h4_reset) - 1, "Wrong length");
		zassert_mem_equal(buf->data, &h4_reset[1], sizeof(h4_reset) - 2);
		net_buf_unref(buf);
	}

	LOG_INF("The mock controller responds with a HCI Command Complete event.");
	{
		struct mock_controller_data *mock_controller_data = mock_controller->data;
		struct net_buf *buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
		int err;

		zassert_not_null(buf);
		net_buf_add_mem(buf, &h4_reset_complete[1], sizeof(h4_reset_complete) - 1);

		err = mock_controller_data->c2h_send(mock_controller, buf);
		zassert_equal(err, 0, "c2h_send failed %d", err);
	}

	LOG_INF("Removing the HCI Command Complete event from the UART.");
	{
		uint8_t h4_cmd_complete_recv[sizeof(h4_reset_complete)];

		uart_c2h_read(h4_cmd_complete_recv, sizeof(h4_cmd_complete_recv),
			      TIMEOUT_PRESUME_STUCK);
		zassert_mem_equal(h4_cmd_complete_recv, h4_reset_complete,
				  sizeof(h4_reset_complete), "Wrong data");
	}
}
