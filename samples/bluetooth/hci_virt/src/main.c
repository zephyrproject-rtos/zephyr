/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/controller.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "posix_board_if.h"
#include "hci_virt_bottom.h"

LOG_MODULE_REGISTER(hci_virt, LOG_LEVEL_INF);

#define H4_CMD 0x01U
#define H4_ACL 0x02U
#define H4_ISO 0x05U
#define H4_DISCARD_LEN 32U

static K_THREAD_STACK_DEFINE(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;
static struct k_thread rx_thread_data;
static K_FIFO_DEFINE(rx_queue);

static bool valid_type(uint8_t type)
{
	return type == H4_CMD || type == H4_ACL || type == H4_ISO;
}

static int set_public_addr_from_arg(void)
{
	const char *str = hci_virt_bd_addr();
	bt_addr_t addr;

	if (str == NULL) {
		return 0;
	}

	if (strlen(str) != BT_ADDR_STR_LEN - 1U) {
		return -EINVAL;
	}

	for (size_t i = 2U; i < BT_ADDR_STR_LEN - 1U; i += 3U) {
		if (str[i] != ':') {
			return -EINVAL;
		}
	}

	for (size_t i = 0U; i < ARRAY_SIZE(addr.val); i++) {
		size_t reverse_i = ARRAY_SIZE(addr.val) - 1U - i;

		if (hex2bin(&str[i * 3U], 2U, &addr.val[reverse_i], 1U) != 1U) {
			return -EINVAL;
		}
	}

	if (bt_addr_eq(&addr, BT_ADDR_ANY)) {
		return -EINVAL;
	}

	bt_ctlr_set_public_addr(addr.val);

	return 0;
}

static size_t payload_len(const uint8_t *header, uint8_t type)
{
	switch (type) {
	case H4_CMD:
		return header[2];
	case H4_ISO:
		return bt_iso_hdr_len(sys_get_le16(&header[2]));
	case H4_ACL:
		return sys_get_le16(&header[2]);
	default:
		return 0U;
	}
}

static size_t header_len(uint8_t type)
{
	switch (type) {
	case H4_CMD:
		return sizeof(struct bt_hci_cmd_hdr);
	case H4_ISO:
		return sizeof(struct bt_hci_iso_hdr);
	case H4_ACL:
		return sizeof(struct bt_hci_acl_hdr);
	default:
		return 0U;
	}
}

/* The raw interface prepends the H:4 packet type to each controller packet. */
static void tx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct net_buf *buf = k_fifo_get(&rx_queue, K_FOREVER);
		int err;

		err = hci_virt_write(buf->data, buf->len);
		if (err == 0) {
			hci_virt_btsnoop_capture(buf->data, buf->len, false);
		} else if (err != -ENOTCONN) {
			LOG_WRN("Failed to send HCI packet to host (%d)", err);
		}

		net_buf_unref(buf);

		k_yield();
	}
}

static void forward_to_controller(struct net_buf *buf)
{
	int err;

	hci_virt_btsnoop_capture(buf->data, buf->len, true);

	err = bt_send(buf);
	if (err != 0) {
		LOG_WRN("Failed to send HCI packet to controller (%d)", err);
		net_buf_unref(buf);
	}

	k_yield();
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	uint8_t header[MAX(sizeof(struct bt_hci_cmd_hdr),
			   MAX(sizeof(struct bt_hci_acl_hdr),
			       sizeof(struct bt_hci_iso_hdr)))];
	uint8_t discard[H4_DISCARD_LEN];
	struct net_buf *buf = NULL;
	size_t remaining = 0U;
	size_t received = 0U;
	enum {
		ST_TYPE,
		ST_HEADER,
		ST_PAYLOAD,
		ST_DISCARD
	} state = ST_TYPE;
	uint8_t type = 0U;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		while (hci_virt_poll()) {
			int32_t count;

			switch (state) {
			case ST_TYPE:
				count = hci_virt_read(&type, sizeof(type));
				break;
			case ST_HEADER:
				count = hci_virt_read(header + received, remaining);
				break;
			case ST_PAYLOAD:
				count = hci_virt_read(net_buf_tail(buf), remaining);
				break;
			case ST_DISCARD:
				count = hci_virt_read(discard, MIN(remaining, sizeof(discard)));
				break;
			default:
				CODE_UNREACHABLE;
			}

			if (count < 0) {
				/*
				 * The Host disconnected (-ECONNRESET) or is not connected
				 * (-ENOTCONN): drop any partial frame and resync on the next
				 * packet type byte once a new Host connects.
				 */
				if (buf != NULL) {
					net_buf_unref(buf);
					buf = NULL;
				}
				state = ST_TYPE;
				break;
			}
			if (count == 0) {
				break;
			}

			switch (state) {
			case ST_TYPE:
				if (valid_type(type)) {
					received = 0U;
					remaining = header_len(type);
					state = ST_HEADER;
				}
				break;
			case ST_HEADER:
				received += (size_t)count;
				remaining -= (size_t)count;
				if (remaining > 0U) {
					break;
				}

				remaining = payload_len(header, type);
				buf = bt_buf_get_tx(bt_buf_type_from_h4(type, BT_BUF_OUT),
						    K_FOREVER, NULL, 0);
				if (buf == NULL ||
				    header_len(type) + remaining > net_buf_tailroom(buf)) {
					if (buf != NULL) {
						net_buf_unref(buf);
						buf = NULL;
					}
					LOG_WRN("Dropping HCI packet: no suitable buffer");
					state = remaining > 0U ? ST_DISCARD : ST_TYPE;
					break;
				}

				net_buf_add_mem(buf, header, header_len(type));
				state = ST_PAYLOAD;
				break;
			case ST_PAYLOAD:
				net_buf_add(buf, (size_t)count);
				remaining -= (size_t)count;
				break;
			case ST_DISCARD:
				remaining -= (size_t)count;
				if (remaining == 0U) {
					state = ST_TYPE;
				}
				break;
			default:
				CODE_UNREACHABLE;
			}

			if (state == ST_PAYLOAD && remaining == 0U) {
				forward_to_controller(buf);
				buf = NULL;
				state = ST_TYPE;
			}
		}

		k_sleep(K_MSEC(1));
	}
}

int main(void)
{
	int err;

	err = set_public_addr_from_arg();
	if (err != 0) {
		LOG_ERR("Invalid controller public address (%d)", err);
		posix_exit(1);
	}

	hci_virt_open();

	err = bt_enable_raw(&rx_queue);
	if (err != 0) {
		LOG_ERR("Failed to enable Bluetooth RAW mode (%d)", err);
		posix_exit(1);
	}

	k_thread_create(&tx_thread_data, tx_thread_stack, K_THREAD_STACK_SIZEOF(tx_thread_stack),
			tx_thread, NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_create(&rx_thread_data, rx_thread_stack, K_THREAD_STACK_SIZEOF(rx_thread_stack),
			rx_thread, NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	return 0;
}
