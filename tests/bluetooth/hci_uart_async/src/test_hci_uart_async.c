/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/net_buf.h>

#include <zephyr/logging/log.h>

#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/uart/serial_test.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_DBG);

/* This is a mock UART. Using `serial_vnd_...` on this simulates
 * traffic from the external Host.
 */
static const struct device *const zephyr_bt_c2h_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_c2h_uart));

/* The DUT is Sandwiched between the mock serial interface and a mock
 * controller. {{{
 */
#define DT_DRV_COMPAT zephyr_bt_hci_test

struct drv_data {
	bt_hci_recv_t recv;
};

static void serial_vnd_data_callback(const struct device *dev, void *user_data);
static int drv_send(const struct device *dev, struct net_buf *buf);
static int drv_open(const struct device *dev, bt_hci_recv_t recv);

static DEVICE_API(bt_hci, drv_api) = {
	.open = drv_open,
	.send = drv_send,
};

static int drv_init(const struct device *dev)
{
	serial_vnd_set_callback(zephyr_bt_c2h_uart, serial_vnd_data_callback, NULL);
	return 0;
}

#define TEST_DEVICE_INIT(inst) \
	static struct drv_data drv_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, drv_init, NULL, &drv_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

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

/* Mock controller callbacks. {{{ */

static int drv_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct drv_data *drv = dev->data;

	LOG_DBG("drv_open");

	drv->recv = recv;

	return 0;
}

/** This FIFO holds the references to all h2c packets the DUT has sent
 *  to the controller using #bt_send.
 *
 *  Each test should mock a controller by calling #net_buf_get on this
 *  FIFO and simulate a controller's #bt_hci_driver::drv_send. The mocks
 *  should use #bt_recv to send c2h packets to the DUT.
 */
K_FIFO_DEFINE(drv_send_fifo); /* elem T: net_buf */
static int drv_send(const struct device *dev, struct net_buf *buf)
{
	LOG_DBG("buf %p type %d len %u", buf, bt_buf_get_type(buf), buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "buf");

	__ASSERT_NO_MSG(buf);
	k_fifo_put(&drv_send_fifo, buf);
	return 0;
}

/* }}} */

/* Mock UART c2h TX handler. {{{ */

static void serial_vnd_data_callback(const struct device *dev, void *user_data)
{
	uint32_t size = serial_vnd_out_data_size_get(dev);
	uint8_t data[size];

	serial_vnd_read_out_data(dev, data, size);
	LOG_HEXDUMP_DBG(data, size, "uart tx");

	/* If a test needs to look at the c2h UART traffic, it can be
	 * captured here.
	 */
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
		int write_size = serial_vnd_queue_in_data(zephyr_bt_c2h_uart, h4_msg_cmd_dummy1,
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
		int write_size =
			serial_vnd_queue_in_data(zephyr_bt_c2h_uart, h4_msg_cmd_host_num_complete,
						 sizeof(h4_msg_cmd_host_num_complete));
		__ASSERT_NO_MSG(write_size == sizeof(h4_msg_cmd_host_num_complete));
	}

	LOG_DBG("All h2c packets queued on UART");

	/* Then, we check that all packets are delivered without loss. */

	/* Expect all the normal commands first. */
	for (uint16_t i = 0; i < HCI_NORMAL_CMD_BUF_COUNT; i++) {
		/* The mock controller processes a command. */
		{
			struct net_buf *buf = k_fifo_get(&drv_send_fifo, TIMEOUT_PRESUME_STUCK);

			zassert_not_null(buf);
			zassert_equal(buf->len, sizeof(h4_msg_cmd_dummy1) - 1, "Wrong length");
			zassert_mem_equal(buf->data, &h4_msg_cmd_dummy1[1],
					  sizeof(h4_msg_cmd_dummy1) - 1);
			net_buf_unref(buf);
		}

		/* The controller sends a HCI Command Complete response. */
		{
			const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
			struct drv_data *drv = dev->data;
			int err;
			struct net_buf *buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);

			zassert_not_null(buf);
			net_buf_add_mem(buf, hci_msg_rx_evt_cmd_complete,
					sizeof(hci_msg_rx_evt_cmd_complete));
			err = drv->recv(dev, buf);
			zassert_equal(err, 0, "bt_recv failed");
		}
	}

	/* Expect all the 'HCI Host Number of Completed Packets'. */
	for (uint16_t i = 0; i < TEST_PARAM_HOST_COMPLETE_COUNT; i++) {
		/* The mock controller processes a 'HCI Host Number of Completed Packets'. */
		{
			struct net_buf *buf = k_fifo_get(&drv_send_fifo, TIMEOUT_PRESUME_STUCK);

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
