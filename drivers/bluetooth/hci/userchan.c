/* userchan.c - HCI User Channel based Bluetooth driver */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Victor Chavez
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/sys/byteorder.h>

#include "nsi_host_trampolines.h"
#include "nsi_errno.h"
#include "soc.h"
#include "cmdline.h" /* native_sim command line options header */
#include "userchan_bottom.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#define DT_DRV_COMPAT zephyr_bt_hci_userchan

#define UC_MAX_DEV DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

/* Ensure at least one userchan instance is enabled in devicetree */
BUILD_ASSERT(UC_MAX_DEV > 0, "No zephyr,bt-hci-userchan instances enabled in devicetree");

struct uc_data {
	int fd;
	bt_hci_recv_t recv;
	uint8_t dev_idx;
	k_thread_stack_t *rx_stack;
	size_t rx_stack_size;
	struct k_thread *rx_thread;
};

#define TCP_ADDR_BUFF_SIZE 16
#define UNIX_ADDR_BUFF_SIZE 4096
enum hci_connection_type {
	HCI_USERCHAN,
	HCI_TCP,
	HCI_UNIX,
};

/* Per-device connection parameters, indexed by --bt-dev order */
static struct {
	enum hci_connection_type type;
	union {
		unsigned short hci_idx;
		struct {
			char ip_addr[TCP_ADDR_BUFF_SIZE];
			unsigned int port;
		} tcp;
		char socket_path[UNIX_ADDR_BUFF_SIZE];
	};
} conn_params[MAX(UC_MAX_DEV, 1)];
static uint8_t conn_param_count;

static bool is_hci_event_discardable(const struct bt_hci_evt_hdr *evt)
{
	switch (evt->evt) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		const struct bt_hci_evt_le_meta_event *meta_evt = (const void *)evt->data;

		switch (meta_evt->subevent) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
#if defined(CONFIG_BT_EXT_ADV)
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT: {
			const struct bt_hci_evt_le_ext_advertising_report *ext_adv =
				(const void *)meta_evt->data;

			return (ext_adv->num_reports == 1) &&
			       ((ext_adv->adv_info[0].evt_type & BT_HCI_LE_ADV_EVT_TYPE_LEGACY) !=
				0);
		}
#endif
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *get_rx_evt(const uint8_t *data)
{
	const struct bt_hci_evt_hdr *evt = (const void *)data;
	const bool discardable = is_hci_event_discardable(evt);
	const k_timeout_t timeout = discardable ? K_NO_WAIT : K_SECONDS(1);
	struct net_buf *buf;

	do {
		buf = bt_buf_get_evt(evt->evt, discardable, timeout);
		if (buf == NULL) {
			if (discardable) {
				LOG_DBG_RATELIMIT("Discardable buffer pool full, ignoring event");
				return buf;
			}
			LOG_WRN("Couldn't allocate a buffer after waiting 1 second.");
		}
	} while (!buf);

	return buf;
}

static struct net_buf *get_rx_acl(const uint8_t *data)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf == NULL) {
		LOG_ERR("No available ACL buffers!");
	}

	return buf;
}

static struct net_buf *get_rx_iso(const uint8_t *data)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	if (buf == NULL) {
		LOG_ERR_RATELIMIT("No available ISO buffers!");
	}

	return buf;
}

static struct net_buf *get_rx(const uint8_t *buf)
{
	uint8_t hci_h4_type = buf[0];

	switch (hci_h4_type) {
	case BT_HCI_H4_EVT:
		return get_rx_evt(&buf[1]);
	case BT_HCI_H4_ACL:
		return get_rx_acl(&buf[1]);
	case BT_HCI_H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			return get_rx_iso(&buf[1]);
		}
		__fallthrough;
	default:
		LOG_ERR("Unknown packet type: %u", hci_h4_type);
	}

	return NULL;
}

/**
 * @brief Decode the length of an HCI H4 packet and check it's complete
 * @details Decodes packet length according to Bluetooth spec v5.4 Vol 4 Part E
 * @param buf	Pointer to a HCI packet buffer
 * @param buf_len	Bytes available in the buffer
 * @return Length of the complete HCI packet in bytes, -1 if cannot find an HCI
 *         packet, 0 if more data required.
 */
static int32_t hci_packet_complete(const uint8_t *buf, uint16_t buf_len)
{
	uint16_t payload_len = 0;
	const uint8_t type = buf[0];
	uint8_t header_len = sizeof(type);
	const uint8_t *hdr = &buf[sizeof(type)];

	switch (type) {
	case BT_HCI_H4_CMD: {
		if (buf_len < header_len + BT_HCI_CMD_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_cmd_hdr *cmd = (const struct bt_hci_cmd_hdr *)hdr;

		/* Parameter Total Length */
		payload_len = cmd->param_len;
		header_len += BT_HCI_CMD_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_ACL: {
		if (buf_len < header_len + BT_HCI_ACL_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_acl_hdr *acl = (const struct bt_hci_acl_hdr *)hdr;

		/* Data Total Length */
		payload_len = sys_le16_to_cpu(acl->len);
		header_len += BT_HCI_ACL_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_SCO: {
		if (buf_len < header_len + BT_HCI_SCO_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_sco_hdr *sco = (const struct bt_hci_sco_hdr *)hdr;

		/* Data_Total_Length */
		payload_len = sco->len;
		header_len += BT_HCI_SCO_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_EVT: {
		if (buf_len < header_len + BT_HCI_EVT_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_evt_hdr *evt = (const struct bt_hci_evt_hdr *)hdr;

		/* Parameter Total Length */
		payload_len = evt->len;
		header_len += BT_HCI_EVT_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_ISO: {
		if (buf_len < header_len + BT_HCI_ISO_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_iso_hdr *iso = (const struct bt_hci_iso_hdr *)hdr;

		/* ISO_Data_Load_Length parameter */
		payload_len =  bt_iso_hdr_len(sys_le16_to_cpu(iso->len));
		header_len += BT_HCI_ISO_HDR_SIZE;
		break;
	}
	/* If no valid packet type found */
	default:
		LOG_WRN("Unknown packet type 0x%02x", type);
		return -1;
	}

	/* Request more data */
	if (buf_len < header_len + payload_len) {
		return 0;
	}

	return (int32_t)header_len + payload_len;
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct uc_data *uc = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("started");

	uint8_t frame[512];
	long frame_size = 0;

	while (1) {
		struct net_buf *buf;
		size_t buf_tailroom;
		size_t buf_add_len;
		long len;
		const uint8_t *frame_start = frame;

		if (!user_chan_rx_ready(uc->fd)) {
			k_sleep(K_MSEC(1));
			continue;
		}

		if (frame_size >= sizeof(frame)) {
			LOG_ERR("HCI Packet is too big for frame (%d "
				"bytes). Dropping data", sizeof(frame));
			frame_size = 0; /* Drop buffer */
		}

		LOG_DBG("calling read()");

		len = nsi_host_read(uc->fd, frame + frame_size, sizeof(frame) - frame_size);
		if (len < 0) {
			if (nsi_host_get_errno() == EINTR) {
				k_yield();
				continue;
			}

			LOG_ERR("Reading socket failed, errno %d", errno);
			(void)nsi_host_close(uc->fd);
			uc->fd = -1;
			return;
		}

		frame_size += len;

		while (frame_size > 0) {
			const uint8_t *buf_add;
			const uint8_t packet_type = frame_start[0];
			const int32_t decoded_len = hci_packet_complete(frame_start, frame_size);

			if (decoded_len == -1) {
				LOG_ERR("HCI Packet type is invalid, length could not be decoded");
				frame_size = 0; /* Drop buffer */
				break;
			}

			if (decoded_len == 0) {
				if ((frame_start != frame) && (frame_size < sizeof(frame))) {
					memmove(frame, frame_start, frame_size);
				}
				/* Read more */
				break;
			}

			buf_add = frame_start + sizeof(packet_type);
			buf_add_len = decoded_len - sizeof(packet_type);

			buf = get_rx(frame_start);

			frame_size -= decoded_len;
			frame_start += decoded_len;

			if (!buf) {
				continue;
			}

			buf_tailroom = net_buf_tailroom(buf);
			if (buf_tailroom < buf_add_len) {
				LOG_ERR("Not enough space in buffer %zu/%zu",
					buf_add_len, buf_tailroom);
				net_buf_unref(buf);
				continue;
			}

			net_buf_add_mem(buf, buf_add, buf_add_len);

			LOG_DBG("Calling bt_recv(%p)", buf);

			uc->recv(dev, buf);
		}

		k_yield();
	}
}

static int uc_send(const struct device *dev, struct net_buf *buf)
{
	struct uc_data *uc = dev->data;

	LOG_DBG("buf %p type %u len %u", buf, buf->data[0], buf->len);

	if (uc->fd < 0) {
		LOG_ERR("User channel not open");
		return -EIO;
	}

	if (nsi_host_write(uc->fd, buf->data, buf->len) < 0) {
		return -nsi_host_get_errno();
	}

	net_buf_unref(buf);
	return 0;
}

static int uc_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct uc_data *uc = dev->data;
	uint8_t idx = uc->dev_idx;

	if (idx >= conn_param_count) {
		LOG_ERR("No --bt-dev argument for instance %u (have %u)", idx, conn_param_count);
		return -ENODEV;
	}

	switch (conn_params[idx].type) {
	case HCI_USERCHAN:
		LOG_DBG("hci%d (instance %u)", conn_params[idx].hci_idx, idx);
		uc->fd = user_chan_socket_open(conn_params[idx].hci_idx);
		break;
	case HCI_TCP:
		LOG_DBG("hci %s:%d (instance %u)", conn_params[idx].tcp.ip_addr,
			conn_params[idx].tcp.port, idx);
		uc->fd = user_chan_net_connect(conn_params[idx].tcp.ip_addr,
					       conn_params[idx].tcp.port);
		break;
	case HCI_UNIX:
		LOG_DBG("hci socket %s (instance %u)", conn_params[idx].socket_path, idx);
		uc->fd = user_chan_unix_connect(conn_params[idx].socket_path);
		break;
	default:
		LOG_ERR("Unknown connection type %d for instance %u", conn_params[idx].type, idx);
		return -EINVAL;
	}
	if (uc->fd < 0) {
		return -nsi_errno_from_mid(-uc->fd);
	}

	uc->recv = recv;

	LOG_DBG("User Channel opened as fd %d", uc->fd);

	k_thread_create(uc->rx_thread, uc->rx_stack, uc->rx_stack_size, rx_thread, (void *)dev,
			NULL, NULL, K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, K_NO_WAIT);

	LOG_DBG("returning");

	return 0;
}

static int uc_close(const struct device *dev)
{
	struct uc_data *uc = dev->data;
	int rc;

	if (uc->fd < 0) {
		return -ENETDOWN;
	}

	rc = nsi_host_close(uc->fd);
	if (rc < 0) {
		return -nsi_errno_from_mid(-rc);
	}

	uc->fd = -1;

	return 0;
}

static DEVICE_API(bt_hci, uc_drv_api) = {
	.open = uc_open,
	.close = uc_close,
	.send = uc_send,
};

static int uc_init(const struct device *dev)
{
	struct uc_data *uc = dev->data;

	if (uc->dev_idx >= conn_param_count) {
		posix_print_warning("Warning: Bluetooth device missing for instance %u.\n"
				    "Specify --bt-dev for each HCI instance.\n",
				    uc->dev_idx);
		return -ENODEV;
	}

	return 0;
}

#define UC_DEVICE_INIT(inst)                                                                       \
	static K_KERNEL_STACK_DEFINE(uc_rx_stack_##inst,                                           \
				     CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);                    \
	static struct k_thread uc_rx_thread_##inst;                                                \
	static struct uc_data uc_data_##inst = {                                                   \
		.fd = -1,                                                                          \
		.dev_idx = inst,                                                                   \
		.rx_stack = uc_rx_stack_##inst,                                                    \
		.rx_stack_size = K_KERNEL_STACK_SIZEOF(uc_rx_stack_##inst),                        \
		.rx_thread = &uc_rx_thread_##inst,                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, uc_init, NULL, &uc_data_##inst, NULL, POST_KERNEL,             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uc_drv_api)

DT_INST_FOREACH_STATUS_OKAY(UC_DEVICE_INIT)

static void cmd_bt_dev_found(char *argv, int offset)
{
	if (conn_param_count >= UC_MAX_DEV) {
		posix_print_error_and_exit("Too many --bt-dev arguments (max %d).\n", UC_MAX_DEV);
	}

	uint8_t idx = conn_param_count;

	if (strncmp(&argv[offset], "hci", 3) == 0 && strlen(&argv[offset]) >= 4) {
		long arg_hci_idx = strtol(&argv[offset + 3], NULL, 10);

		if (arg_hci_idx >= 0 && arg_hci_idx <= USHRT_MAX) {
			conn_params[idx].hci_idx = arg_hci_idx;
			conn_params[idx].type = HCI_USERCHAN;
		} else {
			posix_print_error_and_exit("Invalid argument value for --bt-dev. "
						   "hci idx must be within range 0 to 65535.\n");
		}
	} else if (sscanf(&argv[offset], "%15[^:]:%u", conn_params[idx].tcp.ip_addr,
			  &conn_params[idx].tcp.port) == 2) {
		if (conn_params[idx].tcp.port > USHRT_MAX) {
			posix_print_error_and_exit("Error: IP port for bluetooth "
						   "hci tcp server is out of range.\n");
		}

		if (user_chan_is_ipaddr_ok(conn_params[idx].tcp.ip_addr) != 1) {
			posix_print_error_and_exit("Error: IP address for bluetooth "
						   "hci tcp server is incorrect.\n");
		}

		conn_params[idx].type = HCI_TCP;
	} else if (strlen(&argv[offset]) > 0 && argv[offset] == '/') {
		strncpy(conn_params[idx].socket_path, &argv[offset], UNIX_ADDR_BUFF_SIZE - 1);
		conn_params[idx].socket_path[UNIX_ADDR_BUFF_SIZE - 1] = '\0';
		conn_params[idx].type = HCI_UNIX;
	} else {
		posix_print_error_and_exit("Invalid option %s for --bt-dev. "
					   "An hci interface, absolute UNIX socket path or hci tcp server is expected.\n",
					   &argv[offset]);
	}

	conn_param_count++;
}

static void add_btuserchan_arg(void)
{
	static struct args_struct_t btuserchan_args[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{false, true, false, "bt-dev", "hciX", 's', NULL, cmd_bt_dev_found,
		 "A local HCI device to be used for Bluetooth (e.g. hci0), "
		 "UNIX socket (absolute path, like /tmp/bt-server-bredrle) "
		 "or an HCI TCP Server (e.g. 127.0.0.1:9000). "
		 "May be specified multiple times for multi-controller setups."},
		ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(btuserchan_args);
}

NATIVE_TASK(add_btuserchan_arg, PRE_BOOT_1, 10);
