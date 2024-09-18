/*
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/bluetooth/hci_driver_bluenrg.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <version.h>

#define LOG_MODULE_NAME gui_hci_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static const struct device *const hci_uart_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_c2h_uart));
static K_THREAD_STACK_DEFINE(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;
static K_FIFO_DEFINE(tx_queue);

/* RX in terms of bluetooth communication */
static K_FIFO_DEFINE(uart_tx_queue);

#define H4_ST_EXT_CMD	0x81
#define H4_ST_VND_CMD	0xFF

#define ST_IDLE		0 /* Waiting for packet type. */
#define ST_HDR		1 /* Receiving packet header. */
#define ST_PAYLOAD	2 /* Receiving packet payload. */
#define ST_DISCARD	3 /* Dropping packet. */

/* Length of a discard/flush buffer.
 * This is sized to align with a BLE HCI packet:
 * 1 byte H:4 header + 32 bytes ACL/event data
 * Bigger values might overflow the stack since this is declared as a local
 * variable, smaller ones will force the caller to call into discard more
 * often.
 */
#define H4_DISCARD_LEN 33

#define RESP_VENDOR_CODE_OFFSET	1
#define RESP_LEN_OFFSET_LSB	2
#define RESP_LEN_OFFSET_MSB	3
#define RESP_CMDCODE_OFFSET	4
#define RESP_STATUS_OFFSET	5
#define RESP_PARAM_OFFSET	6

/* Types of vendor codes */
#define VENDOR_CODE_ERROR	0
#define VENDOR_CODE_RESPONSE	1

/* Commands */
#define VENDOR_CMD_READ_VERSION		0x01
#define VENDOR_CMD_BLUENRG_RESET	0x04
#define VENDOR_CMD_HW_BOOTLOADER	0x05

struct bt_hci_ext_cmd_hdr {
	uint16_t opcode;
	uint16_t param_len;
} __packed;

struct bt_vendor_cmd_hdr {
	uint8_t opcode;
	uint16_t param_len;
} __packed;

struct bt_vendor_rsp_hdr {
	uint8_t vendor_code;
	uint16_t param_len;
	uint8_t opcode;
	uint8_t status;
	uint8_t params[2];
} __packed;

static int h4_send(struct net_buf *buf);

static uint16_t parse_cmd(uint8_t *hci_buffer, uint16_t hci_pckt_len, uint8_t *buffer_out)
{
	uint16_t len = 0;
	struct bt_vendor_cmd_hdr *hdr = (struct bt_vendor_cmd_hdr *) hci_buffer;
	struct bt_vendor_rsp_hdr *rsp = (struct bt_vendor_rsp_hdr *) (buffer_out + 1);

	buffer_out[0] = H4_ST_VND_CMD;
	rsp->vendor_code = VENDOR_CODE_RESPONSE;
	rsp->opcode = hdr->opcode;
	rsp->status = 0;

	switch (hdr->opcode) {
	case VENDOR_CMD_READ_VERSION:
		rsp->params[0] = KERNEL_VERSION_MAJOR;
		if (KERNEL_PATCHLEVEL >= 9) {
			rsp->params[1] = (KERNEL_VERSION_MINOR * 10) + 9;
		} else {
			rsp->params[1] = (KERNEL_VERSION_MINOR * 10) + KERNEL_PATCHLEVEL;
		}
		len = 2;
		break;
#if DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v1) || DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v2)
	case VENDOR_CMD_BLUENRG_RESET:
		bluenrg_bt_reset(0);
		break;
	case VENDOR_CMD_HW_BOOTLOADER:
		bluenrg_bt_reset(1);
		break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v1) || DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v2) */
	default:
		rsp->vendor_code = VENDOR_CODE_ERROR;
		rsp->status = BT_HCI_ERR_UNKNOWN_CMD;
	}

	len += 2; /* Status and Command code */
	rsp->param_len = sys_cpu_to_le16(len);
	len += RESP_CMDCODE_OFFSET;

	return len;
}

static int send_evt(uint8_t *response, uint8_t len)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);

	if (!buf) {
		LOG_ERR("EVT no buffer");
		return -ENOMEM;
	}
	if (len > net_buf_tailroom(buf)) {
		LOG_ERR("EVT too long: %d", len);
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_mem(buf, response, len);

	return h4_send(buf);
}

static int h4_read(const struct device *uart, uint8_t *buf, size_t len)
{
	int rx = uart_fifo_read(uart, buf, len);

	LOG_DBG("read %d req %d", rx, len);
	return rx;
}

static bool valid_type(uint8_t type)
{
	return (type == BT_HCI_H4_CMD) | (type == H4_ST_EXT_CMD) |
		(type == BT_HCI_H4_ACL) | (type == BT_HCI_H4_ISO) | (type == H4_ST_VND_CMD);
}

/* Function expects that type is validated and only CMD, ISO or ACL are used. */
static uint32_t get_len(const uint8_t *hdr_buf, uint8_t type)
{
	switch (type) {
	case BT_HCI_H4_CMD:
		return ((const struct bt_hci_cmd_hdr *)hdr_buf)->param_len;
	case H4_ST_EXT_CMD:
		return ((const struct bt_hci_ext_cmd_hdr *)hdr_buf)->param_len;
	case H4_ST_VND_CMD:
		return ((const struct bt_vendor_cmd_hdr *)hdr_buf)->param_len;
	case BT_HCI_H4_ISO:
		return bt_iso_hdr_len(
			sys_le16_to_cpu(((const struct bt_hci_iso_hdr *)hdr_buf)->len));
	case BT_HCI_H4_ACL:
		return sys_le16_to_cpu(((const struct bt_hci_acl_hdr *)hdr_buf)->len);
	default:
		LOG_ERR("Invalid type: %u", type);
		return 0;
	}
}

/* Function expects that type is validated and only CMD, ISO or ACL are used. */
static int hdr_len(uint8_t type)
{
	switch (type) {
	case BT_HCI_H4_CMD:
		return sizeof(struct bt_hci_cmd_hdr);
	case H4_ST_EXT_CMD:
		return sizeof(struct bt_hci_ext_cmd_hdr);
	case H4_ST_VND_CMD:
		return sizeof(struct bt_vendor_cmd_hdr);
	case BT_HCI_H4_ISO:
		return sizeof(struct bt_hci_iso_hdr);
	case BT_HCI_H4_ACL:
		return sizeof(struct bt_hci_acl_hdr);
	default:
		LOG_ERR("Invalid type: %u", type);
		return 0;
	}
}

static struct net_buf *alloc_tx_buf(uint8_t type)
{
	uint8_t alloc_type = type;
	struct net_buf *buf;

	switch (type) {
	case H4_ST_EXT_CMD:
	case BT_HCI_H4_CMD:
	case H4_ST_VND_CMD:
		alloc_type = BT_HCI_H4_CMD;
		break;
	case BT_HCI_H4_ISO:
	case BT_HCI_H4_ACL:
		break;
	default:
		LOG_ERR("Invalid type: %u", type);
		return NULL;
	}
	buf = bt_buf_get_tx(BT_BUF_H4, K_NO_WAIT, &alloc_type, sizeof(alloc_type));
	if (buf && (type == H4_ST_VND_CMD)) {
		bt_buf_set_type(buf, type);
	}
	return buf;
}

static void rx_isr(void)
{
	static struct net_buf *buf;
	static int remaining;
	static uint8_t state;
	static uint8_t type;
	static uint8_t hdr_buf[MAX(sizeof(struct bt_hci_cmd_hdr), sizeof(struct bt_hci_acl_hdr))];
	int read;

	do {
		switch (state) {
		case ST_IDLE:
			/* Get packet type */
			read = h4_read(hci_uart_dev, &type, sizeof(type));
			/* since we read in loop until no data is in the fifo,
			 * it is possible that read = 0.
			 */
			if (read) {
				if (valid_type(type)) {
					/* Get expected header size and switch
					 * to receiving header.
					 */
					remaining = hdr_len(type);
					state = ST_HDR;
				} else {
					LOG_WRN("Unknown header %d", type);
				}
			}
			break;
		case ST_HDR:
			read = h4_read(hci_uart_dev, &hdr_buf[hdr_len(type) - remaining],
				remaining);
			remaining -= read;
			if (remaining == 0) {
				/* Header received. Allocate buffer and get
				 * payload length. If allocation fails leave
				 * interrupt. On failed allocation state machine
				 * is reset.
				 */
				uint8_t header_length;

				buf = alloc_tx_buf(type);
				if (!buf) {
					LOG_ERR("No available command buffers!");
					state = ST_IDLE;
					return;
				}

				remaining = get_len(hdr_buf, type);
				header_length = hdr_len(type);
				if (type == H4_ST_EXT_CMD) {
					/* Convert to regular HCI_CMD */
					if (remaining > 255) {
						LOG_ERR("len > 255");
						net_buf_unref(buf);
						state = ST_DISCARD;
					} else {
						header_length--;
					}
				}
				net_buf_add_mem(buf, hdr_buf, header_length);
				if (remaining > net_buf_tailroom(buf)) {
					LOG_ERR("Not enough space in buffer");
					net_buf_unref(buf);
					state = ST_DISCARD;
				} else {
					state = ST_PAYLOAD;
				}

			}
			break;
		case ST_PAYLOAD:
			read = h4_read(hci_uart_dev, net_buf_tail(buf), remaining);
			buf->len += read;
			remaining -= read;
			if (remaining == 0) {
				/* Packet received */
				LOG_DBG("putting RX packet in queue.");
				k_fifo_put(&tx_queue, buf);
				state = ST_IDLE;
			}
			break;
		case ST_DISCARD:
			uint8_t discard[H4_DISCARD_LEN];
			size_t to_read = MIN(remaining, sizeof(discard));

			read = h4_read(hci_uart_dev, discard, to_read);
			remaining -= read;
			if (remaining == 0) {
				state = ST_IDLE;
			}
			break;
		default:
			read = 0;
			__ASSERT_NO_MSG(0);
			break;

		}
	} while (read);
}

static void tx_isr(void)
{
	static struct net_buf *buf;
	int len;

	if (!buf) {
		buf = k_fifo_get(&uart_tx_queue, K_NO_WAIT);
		if (!buf) {
			uart_irq_tx_disable(hci_uart_dev);
			return;
		}
	}
	len = uart_fifo_fill(hci_uart_dev, buf->data, buf->len);
	net_buf_pull(buf, len);
	if (!buf->len) {
		net_buf_unref(buf);
		buf = NULL;
	}
}

static void bt_uart_isr(const struct device *unused, void *user_data)
{
	ARG_UNUSED(unused);
	ARG_UNUSED(user_data);

	if (!(uart_irq_rx_ready(hci_uart_dev) || uart_irq_tx_ready(hci_uart_dev))) {
		LOG_DBG("spurious interrupt");
	}
	if (uart_irq_tx_ready(hci_uart_dev)) {
		tx_isr();
	}
	if (uart_irq_rx_ready(hci_uart_dev)) {
		rx_isr();
	}
}

static void tx_thread(void *p1, void *p2, void *p3)
{
	enum bt_buf_type buf_type;

	while (1) {
		struct net_buf *buf;
		int err = 0;
		uint8_t len;
		uint8_t response[16];

		/* Wait until a buffer is available */
		buf = k_fifo_get(&tx_queue, K_FOREVER);
		buf_type = bt_buf_get_type(buf);
		if (buf_type == H4_ST_VND_CMD) {
			len = parse_cmd(buf->data, buf->len, response);
			err =  send_evt(response, len);
			if (!err) {
				net_buf_unref(buf);
			}
		} else {
			/* Pass buffer to the stack */
			err = bt_send(buf);
		}
		if (err) {
			LOG_ERR("Unable to send (err %d)", err);
			net_buf_unref(buf);
		}

		/* Give other threads a chance to run if tx_queue keeps getting
		 * new data all the time.
		 */
		k_yield();
	}
}

static int h4_send(struct net_buf *buf)
{
	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);
	k_fifo_put(&uart_tx_queue, buf);
	uart_irq_tx_enable(hci_uart_dev);
	return 0;
}

static int hci_uart_init(void)
{
	LOG_DBG("");
	if (!device_is_ready(hci_uart_dev)) {
		LOG_ERR("HCI UART %s is not ready", hci_uart_dev->name);
		return -EINVAL;
	}
	uart_irq_rx_disable(hci_uart_dev);
	uart_irq_tx_disable(hci_uart_dev);
	uart_irq_callback_set(hci_uart_dev, bt_uart_isr);
	uart_irq_rx_enable(hci_uart_dev);
	return 0;
}

int main(void)
{
	/* incoming events and data from the controller */
	static K_FIFO_DEFINE(rx_queue);
	int err;

	LOG_DBG("Start");
	__ASSERT(hci_uart_dev, "UART device is NULL");

	/* Enable the raw interface, this will in turn open the HCI driver */
	bt_enable_raw(&rx_queue);
	/* Spawn the TX thread and start feeding commands and data to the controller */
	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack), tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(&tx_thread_data, "HCI uart TX");

	while (1) {
		struct net_buf *buf;

		buf = k_fifo_get(&rx_queue, K_FOREVER);
		err = h4_send(buf);
		if (err) {
			LOG_ERR("Failed to send");
		}
	}
	return 0;
}

SYS_INIT(hci_uart_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
