/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(shell_ble, CONFIG_SHELL_BLE_INIT_LOG_LEVEL);

#define SHELL_MTU_PAYLOAD 128

/* 128 bit UUIDs for GATT service and characteristics */
#define BT_UUID_BLE_SHELL_VAL BT_UUID_128_ENCODE(0x5cca88d3, 0x80ac, 0x45a8, 0x84a7, 0xd949fe458b85)

/* Read/Write Characteristic UUID */
#define BT_UUID_BLE_SHELL_COMMAND_VAL                                                              \
	BT_UUID_128_ENCODE(0x5cca88d3, 0x80ac, 0x45a8, 0x84a7, 0xd949fe458b86)

#define BT_UUID_BLE_SHELL         BT_UUID_DECLARE_128(BT_UUID_BLE_SHELL_VAL)
#define BT_UUID_BLE_SHELL_COMMAND BT_UUID_DECLARE_128(BT_UUID_BLE_SHELL_COMMAND_VAL)

struct shell_ble {
	/** Initialized flag */
	bool initialized;
	/** Handler function registered by shell. */
	shell_transport_handler_t shell_handler;
	/** Context registered by shell. */
	void *shell_context;
	/** Mutex for protecting cmd buffer access */
	struct k_mutex cmd_lock;
	/** buffer for commands received over BLE */
	char cmd_buf[CONFIG_SHELL_CMD_BUFF_SIZE];
	/** number of bytes in cmd_buf */
	size_t cmd_len;
};

static int init(const struct shell_transport *transport, const void *config,
		shell_transport_handler_t evt_handler, void *context);
static int uninit(const struct shell_transport *transport);
static int enable(const struct shell_transport *transport, bool blocking);
static int write(const struct shell_transport *transport, const void *data, size_t length,
		 size_t *cnt);
static int read(const struct shell_transport *transport, void *data, size_t length, size_t *cnt);
const struct shell *shell_backend_ble_get_ptr(void);

const struct shell_transport_api shell_ble_transport_api = {
	.init = init, .uninit = uninit, .enable = enable, .write = write, .read = read};

#define SHELL_BLE_DEFINE(_name)                                                                    \
	static struct shell_ble _name##_shell_ble;                                                 \
	struct shell_transport _name = {.api = &shell_ble_transport_api,                           \
					.ctx = (struct shell_ble *)&_name##_shell_ble}

SHELL_BLE_DEFINE(shell_transport_ble);
SHELL_DEFINE(shell_ble, CONFIG_SHELL_PROMPT_BLE, &shell_transport_ble, 256, 0,
	     SHELL_FLAG_CRLF_DEFAULT);
static bool notify;

static ssize_t ble_recv(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(offset);
	ARG_UNUSED(flags);

	if (buf == NULL || len == 0) {
		return -1;
	}

	const struct shell_transport *shell_transport = shell_backend_ble_get_ptr()->iface;
	struct shell_ble *ble_shell = shell_transport->ctx;

	k_mutex_lock(&ble_shell->cmd_lock, K_FOREVER);

	if (ble_shell->cmd_len + len >= CONFIG_SHELL_CMD_BUFF_SIZE) {
		k_mutex_unlock(&ble_shell->cmd_lock);
		return -1;
	}

	/* Copy command to buffer */
	memcpy(ble_shell->cmd_buf + ble_shell->cmd_len, buf, len);
	ble_shell->cmd_len += len;

	ble_shell->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, ble_shell->shell_context);

	k_mutex_unlock(&ble_shell->cmd_lock);

	return 0;
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	notify = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

BT_GATT_SERVICE_DEFINE(ble_shell_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_BLE_SHELL),
		       BT_GATT_CHARACTERISTIC(BT_UUID_BLE_SHELL_COMMAND,
					      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_WRITE, NULL, ble_recv, NULL),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static int init(const struct shell_transport *transport, const void *config,
		shell_transport_handler_t evt_handler, void *context)
{
	struct shell_ble *ble_shell = (struct shell_ble *)transport->ctx;

	if (ble_shell->initialized) {
		return -EINVAL;
	}

	ble_shell->shell_handler = evt_handler;
	ble_shell->shell_context = context;
	ble_shell->cmd_len = 0;
	memset(ble_shell->cmd_buf, 0, CONFIG_SHELL_CMD_BUFF_SIZE);
	ble_shell->initialized = true;

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	struct shell_ble *ble_shell = (struct shell_ble *)transport->ctx;

	if (!ble_shell->initialized) {
		return -ENODEV;
	}

	ble_shell->initialized = false;

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	ARG_UNUSED(blocking);

	struct shell_ble *ble_shell = (struct shell_ble *)transport->ctx;

	if (!ble_shell->initialized) {
		return -ENODEV;
	}

	return 0;
}

static int write(const struct shell_transport *transport, const void *data, size_t length,
		 size_t *cnt)
{
	int err;
	struct shell_ble *ble_shell = (struct shell_ble *)transport->ctx;

	if (!ble_shell->initialized) {
		*cnt = 0;
		return -ENODEV;
	}

	if (notify) {

		/* prints to serial backend */
		for (int i = 0; i < length; i++) {
			LOG_DBG("%c", ((char *)data)[i]);
		}

		size_t offset = 0;

		while (offset < length) {
			size_t chunk_size = MIN(SHELL_MTU_PAYLOAD, length - offset);

			err = bt_gatt_notify(NULL, &ble_shell_svc.attrs[1], (char *)data + offset,
					     chunk_size);
			if (err) {
				*cnt = offset;
				return err;
			}

			offset += chunk_size;
		}
	}

	*cnt = length;

	return 0;
}

static int read(const struct shell_transport *transport, void *data, size_t length, size_t *cnt)
{
	size_t read_len;
	struct shell_ble *ble_shell = (struct shell_ble *)transport->ctx;

	if (!ble_shell->initialized) {
		return -ENODEV;
	}

	k_mutex_lock(&ble_shell->cmd_lock, K_FOREVER);

	/* If buffer is empty return immediately */
	if (ble_shell->cmd_len == 0) {
		*cnt = 0;
		k_mutex_unlock(&ble_shell->cmd_lock);
		return 0;
	}

	read_len = MIN(length, ble_shell->cmd_len);

	memcpy(data, ble_shell->cmd_buf, read_len);

	*cnt = read_len;
	ble_shell->cmd_len = ble_shell->cmd_len - read_len;

	if (ble_shell->cmd_len > 0) {
		memmove(ble_shell->cmd_buf, ble_shell->cmd_buf + read_len, ble_shell->cmd_len);
	}

	k_mutex_unlock(&ble_shell->cmd_lock);

	return 0;
}

static int enable_shell_ble(void)
{
#if defined(CONFIG_SHELL_BACKEND_BLE_DEBUG)
	/* Enable echo and logging during debug */
	static const struct shell_backend_config_flags cfg_flags = {
		.insert_mode = 0,
		.echo = 1,
		.obscure = 0,
		.mode_delete = 0,
		.use_colors = 0,
		.use_vt100 = 0,
	};
	bool logger = true;
#else
	static const struct shell_backend_config_flags cfg_flags = {0};
	bool logger = false;
#endif

	shell_init(&shell_ble, NULL, cfg_flags, logger, CONFIG_LOG_MAX_LEVEL);

	return 0;
}

SYS_INIT(enable_shell_ble, POST_KERNEL, 0);

const struct shell *shell_backend_ble_get_ptr(void)
{
	return &shell_ble;
}
