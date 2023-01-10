/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/usb/usb_device.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>

LOG_MODULE_REGISTER(hci_uart_async, LOG_LEVEL_DBG);

static const struct device *const hci_uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_c2h_uart));

static K_THREAD_STACK_DEFINE(h2c_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread h2c_thread;

enum h4_type {
	H4_CMD = 0x01,
	H4_ACL = 0x02,
	H4_SCO = 0x03,
	H4_EVT = 0x04,
	H4_ISO = 0x05,
};

struct k_poll_signal uart_h2c_rx_sig;
struct k_poll_signal uart_c2h_tx_sig;

static K_FIFO_DEFINE(c2h_queue);

/** Send raw data on c2h UART.
 *
 * Blocks until completion. Not thread-safe.
 *
 * @retval 0 on success
 * @retval -EBUSY Another transmission is in progress. This a
 * thread-safety violation.
 * @retval -errno @ref uart_tx error.
 */
static int uart_c2h_tx(const uint8_t *data, size_t size)
{
	int err;
	struct k_poll_signal *sig = &uart_c2h_tx_sig;
	struct k_poll_event done[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, sig),
	};

	k_poll_signal_reset(sig);
	err = uart_tx(hci_uart_dev, data, size, SYS_FOREVER_US);

	if (err) {
		LOG_ERR("uart c2h tx: err %d", err);
		return err;
	}

	err = k_poll(done, ARRAY_SIZE(done), K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

	return 0;
}

/* Function expects that type is validated and only CMD, ISO or ACL will be used. */
static uint32_t hci_payload_size(const uint8_t *hdr_buf, enum h4_type type)
{
	switch (type) {
	case H4_CMD:
		return ((const struct bt_hci_cmd_hdr *)hdr_buf)->param_len;
	case H4_ACL:
		return sys_le16_to_cpu(((const struct bt_hci_acl_hdr *)hdr_buf)->len);
	case H4_ISO:
		return bt_iso_hdr_len(
			sys_le16_to_cpu(((const struct bt_hci_iso_hdr *)hdr_buf)->len));
	default:
		LOG_ERR("Invalid type: %u", type);
		return 0;
	}
}

static uint8_t hci_hdr_size(enum h4_type type)
{
	switch (type) {
	case H4_CMD:
		return sizeof(struct bt_hci_cmd_hdr);
	case H4_ACL:
		return sizeof(struct bt_hci_acl_hdr);
	case H4_ISO:
		return sizeof(struct bt_hci_iso_hdr);
	default:
		LOG_ERR("Unexpected h4 type: %u", type);
		return 0;
	}
}

/** Send raw data on c2h UART.
 *
 * Blocks until either @p size has been received or special UART
 * condition occurs on the UART RX line, like an UART break or parity
 * error.
 *
 * Not thread-safe.
 *
 * @retval 0 on success
 * @retval -EBUSY Another transmission is in progress. This a
 * thread-safety violation.
 * @retval -errno @ref uart_rx_enable error.
 * @retval +stop_reason Special condition @ref uart_rx_stop_reason.
 */
static int uart_h2c_rx(uint8_t *dst, size_t size)
{
	int err;
	struct k_poll_signal *sig = &uart_h2c_rx_sig;
	struct k_poll_event done[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, sig),
	};

	k_poll_signal_reset(sig);
	err = uart_rx_enable(hci_uart_dev, dst, size, SYS_FOREVER_US);

	if (err) {
		LOG_ERR("uart h2c rx: err %d", err);
		return err;
	}

	k_poll(done, ARRAY_SIZE(done), K_FOREVER);
	return sig->result;
}

/** Inject a HCI EVT Hardware error into the c2h packet stream.
 *
 * This uses `bt_recv`, just as if the controller is sending the error.
 */
static void send_hw_error(void)
{
	const uint8_t err_code = 0;
	const uint8_t hci_evt_hw_err[] = {BT_HCI_EVT_HARDWARE_ERROR,
					  sizeof(struct bt_hci_evt_hardware_error), err_code};

	struct net_buf *buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	net_buf_add_mem(buf, hci_evt_hw_err, sizeof(hci_evt_hw_err));

	/* Inject the message into the c2h queue. */
	bt_recv(buf);

	/* The c2h thread will send the message at some point. The host
	 * will receive it and reset the controller.
	 */
}

static void recover_sync_by_reset_pattern(void)
{
	/* { H4_CMD, le_16(HCI_CMD_OP_RESET), len=0 } */
	const uint8_t h4_cmd_reset[] = {0x01, 0x03, 0x0C, 0x00};
	const uint32_t reset_pattern = sys_get_be32(h4_cmd_reset);
	int err;
	struct net_buf *h2c_cmd_reset;
	uint32_t shift_register = 0;

	LOG_DBG("Looking for reset pattern");

	while (shift_register != reset_pattern) {
		uint8_t read_byte;

		uart_h2c_rx(&read_byte, sizeof(uint8_t));
		LOG_DBG("h2c: 0x%02x", read_byte);
		shift_register = (shift_register * 0x100) + read_byte;
	}

	LOG_DBG("Pattern found");
	h2c_cmd_reset = bt_buf_get_tx(BT_BUF_H4, K_FOREVER, h4_cmd_reset, sizeof(h4_cmd_reset));
	LOG_DBG("Fowarding reset");

	err = bt_send(h2c_cmd_reset);
	__ASSERT(!err, "Failed to send reset: %d", err);
}

static void h2c_h4_transport(void)
{
	/* When entering this function, the h2c stream should be
	 * 'synchronized'. I.e. The stream should be at a H4 packet
	 * boundary.
	 *
	 * This function returns to signal a desynchronization.
	 * When this happens, the caller should resynchronize before
	 * entering this function again. It's up to the caller to decide
	 * how to resynchronize.
	 */

	for (;;) {
		int err;
		struct net_buf *buf;
		uint8_t h4_type;
		uint8_t hdr_size;
		uint8_t *hdr_buf;
		uint16_t payload_size;

		LOG_DBG("h2c: listening");

		/* Read H4 type. */
		err = uart_h2c_rx(&h4_type, sizeof(uint8_t));

		if (err) {
			return;
		}
		LOG_DBG("h2c: h4_type %d", h4_type);

		/* Allocate buf. */
		buf = bt_buf_get_tx(BT_BUF_H4, K_FOREVER, &h4_type, sizeof(h4_type));
		LOG_DBG("h2c: buf %p", buf);

		if (!buf) {
			/* `h4_type` was invalid. */
			__ASSERT_NO_MSG(hci_hdr_size(h4_type) == 0);

			LOG_WRN("bt_buf_get_tx failed h4_type %d", h4_type);
			return;
		}

		/* Read HCI header. */
		hdr_size = hci_hdr_size(h4_type);
		hdr_buf = net_buf_add(buf, hdr_size);

		err = uart_h2c_rx(hdr_buf, hdr_size);
		if (err) {
			net_buf_unref(buf);
			return;
		}
		LOG_HEXDUMP_DBG(hdr_buf, hdr_size, "h2c: hci hdr");

		/* Read HCI payload. */
		payload_size = hci_payload_size(hdr_buf, h4_type);

		LOG_DBG("h2c: payload_size %u", payload_size);

		if (payload_size <= net_buf_tailroom(buf)) {
			uint8_t *payload_dst = net_buf_add(buf, payload_size);

			err = uart_h2c_rx(payload_dst, payload_size);
			if (err) {
				net_buf_unref(buf);
				return;
			}
			LOG_HEXDUMP_DBG(payload_dst, payload_size, "h2c: hci payload");
		} else {
			/* Discard oversize packet. */
			uint8_t *discard_dst;
			uint16_t discard_size;

			LOG_WRN("h2c: Discarding oversize h4_type %d payload_size %d.", h4_type,
				payload_size);

			/* Reset `buf` so all of it is available. */
			net_buf_reset(buf);
			discard_dst = net_buf_tail(buf);
			discard_size = net_buf_max_len(buf);

			while (payload_size) {
				uint16_t read_size = MIN(payload_size, discard_size);

				err = uart_h2c_rx(discard_dst, read_size);
				if (err) {
					net_buf_unref(buf);
					return;
				}

				payload_size -= read_size;
			}

			net_buf_unref(buf);
			buf = NULL;
		}

		LOG_DBG("h2c: packet done");

		/* Route buf to Controller. */
		if (buf) {
			err = bt_send(buf);
			if (err) {
				/* This is not a transport error. */
				LOG_ERR("bt_send err %d", err);
				net_buf_unref(buf);
				buf = NULL;
			}
		}

		k_yield();
	}
}

static void h2c_thread_entry(void *p1, void *p2, void *p3)
{
	k_thread_name_set(k_current_get(), "HCI TX (h2c)");

	for (;;) {
		LOG_DBG("Synchronized");
		h2c_h4_transport();
		LOG_WRN("Desynchronized");
		send_hw_error();
		recover_sync_by_reset_pattern();
	}
}

void callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type == UART_RX_DISABLED) {
		(void)k_poll_signal_raise(&uart_h2c_rx_sig, 0);
	} else if (evt->type == UART_RX_STOPPED) {
		(void)k_poll_signal_raise(&uart_h2c_rx_sig, evt->data.rx_stop.reason);
	} else if (evt->type == UART_TX_DONE) {
		(void)k_poll_signal_raise(&uart_c2h_tx_sig, 0);
	}
}

static int hci_uart_init(void)
{
	int err;

	k_poll_signal_init(&uart_h2c_rx_sig);
	k_poll_signal_init(&uart_c2h_tx_sig);

	LOG_DBG("");

	if (!device_is_ready(hci_uart_dev)) {
		LOG_ERR("HCI UART %s is not ready", hci_uart_dev->name);
		return -EINVAL;
	}

	BUILD_ASSERT(IS_ENABLED(CONFIG_UART_ASYNC_API));
	err = uart_callback_set(hci_uart_dev, callback, NULL);

	/* Note: Asserts if CONFIG_UART_ASYNC_API is not enabled for `hci_uart_dev`. */
	__ASSERT(!err, "err %d", err);

	return 0;
}

SYS_INIT(hci_uart_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

const struct {
	uint8_t h4;
	struct bt_hci_evt_hdr hdr;
	struct bt_hci_evt_cmd_complete cc;
} __packed cc_evt = {
	.h4 = H4_EVT,
	.hdr = {.evt = BT_HCI_EVT_CMD_COMPLETE, .len = sizeof(struct bt_hci_evt_cmd_complete)},
	.cc = {.ncmd = 1, .opcode = sys_cpu_to_le16(BT_OP_NOP)},
};

static void c2h_thread_entry(void)
{
	k_thread_name_set(k_current_get(), "HCI RX (c2h)");

	if (IS_ENABLED(CONFIG_BT_WAIT_NOP)) {
		uart_c2h_tx((char *)&cc_evt, sizeof(cc_evt));
	}

	for (;;) {
		struct net_buf *buf;

		buf = net_buf_get(&c2h_queue, K_FOREVER);
		uart_c2h_tx(buf->data, buf->len);
		net_buf_unref(buf);
	}
}

void hci_uart_main(void)
{
	int err;

	err = bt_enable_raw(&c2h_queue);
	__ASSERT_NO_MSG(!err);

	/* TX thread. */
	k_thread_create(&h2c_thread, h2c_thread_stack, K_THREAD_STACK_SIZEOF(h2c_thread_stack),
			h2c_thread_entry, NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Reuse current thread as RX thread. */
	c2h_thread_entry();
}
