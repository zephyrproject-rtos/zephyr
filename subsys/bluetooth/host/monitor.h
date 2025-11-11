/** @file
 *  @brief Custom monitor protocol logging over UART
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>

#define BT_MONITOR_NEW_INDEX    0
#define BT_MONITOR_DEL_INDEX    1
#define BT_MONITOR_COMMAND_PKT  2
#define BT_MONITOR_EVENT_PKT    3
#define BT_MONITOR_ACL_TX_PKT   4
#define BT_MONITOR_ACL_RX_PKT   5
#define BT_MONITOR_SCO_TX_PKT   6
#define BT_MONITOR_SCO_RX_PKT   7
#define BT_MONITOR_OPEN_INDEX   8
#define BT_MONITOR_CLOSE_INDEX  9
#define BT_MONITOR_INDEX_INFO   10
#define BT_MONITOR_VENDOR_DIAG  11
#define BT_MONITOR_SYSTEM_NOTE  12
#define BT_MONITOR_USER_LOGGING 13
#define BT_MONITOR_ISO_TX_PKT   18
#define BT_MONITOR_ISO_RX_PKT   19
#define BT_MONITOR_NOP          255

#define BT_MONITOR_TYPE_PRIMARY	0
#define BT_MONITOR_TYPE_AMP	1

/* Extended header types */
#define BT_MONITOR_COMMAND_DROPS 1
#define BT_MONITOR_EVENT_DROPS   2
#define BT_MONITOR_ACL_RX_DROPS  3
#define BT_MONITOR_ACL_TX_DROPS  4
#define BT_MONITOR_SCO_RX_DROPS  5
#define BT_MONITOR_SCO_TX_DROPS  6
#define BT_MONITOR_OTHER_DROPS   7
#define BT_MONITOR_TS32          8

#define BT_MONITOR_BASE_HDR_LEN  6

#if defined(CONFIG_BT_CLASSIC)
#define BT_MONITOR_EXT_HDR_MAX 19
#else
#define BT_MONITOR_EXT_HDR_MAX 15
#endif

struct bt_monitor_hdr {
	uint16_t  data_len;
	uint16_t  opcode;
	uint8_t   flags;
	uint8_t   hdr_len;

	uint8_t   ext[BT_MONITOR_EXT_HDR_MAX];
} __packed;

struct bt_monitor_ts32 {
	uint8_t   type;
	uint32_t  ts32;
} __packed;

struct bt_monitor_new_index {
	uint8_t  type;
	uint8_t  bus;
	uint8_t  bdaddr[6];
	char  name[8];
} __packed;

struct bt_monitor_user_logging {
	uint8_t  priority;
	uint8_t  ident_len;
} __packed;

enum bt_monitor_dir {
	BT_MONITOR_TX,
	BT_MONITOR_RX,
};

static inline uint8_t bt_monitor_opcode(uint8_t type, enum bt_monitor_dir dir)
{
	switch (type) {
	case BT_HCI_H4_CMD:
		return BT_MONITOR_COMMAND_PKT;
	case BT_HCI_H4_EVT:
		return BT_MONITOR_EVENT_PKT;
	case BT_HCI_H4_ACL:
		if (dir == BT_MONITOR_TX) {
			return BT_MONITOR_ACL_TX_PKT;
		} else {
			return BT_MONITOR_ACL_RX_PKT;
		}
	case BT_HCI_H4_ISO:
		if (dir == BT_MONITOR_TX) {
			return BT_MONITOR_ISO_TX_PKT;
		} else {
			return BT_MONITOR_ISO_RX_PKT;
		}
	default:
		return BT_MONITOR_NOP;
	}
}

/* TODO: Remove guard and replace with IS_ENABLED(CONFIG_BT_MONITOR) */
#if defined(CONFIG_BT_MONITOR)

void bt_monitor_send(uint16_t opcode, const void *data, size_t len);

void bt_monitor_new_index(uint8_t type, uint8_t bus, const bt_addr_t *addr,
			  const char *name);

#else /* !CONFIG_BT_MONITOR */

static inline void bt_monitor_send(uint16_t opcode, const void *data, size_t len)
{
	ARG_UNUSED(opcode);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
}

static inline void bt_monitor_new_index(uint8_t type, uint8_t bus, const bt_addr_t *addr,
					const char *name)
{
	ARG_UNUSED(type);
	ARG_UNUSED(bus);
	ARG_UNUSED(addr);
	ARG_UNUSED(name);
}

#endif
