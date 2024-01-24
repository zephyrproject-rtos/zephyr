/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <zephyr/shell/shell_bt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(shell_bt, CONFIG_SHELL_BT_LOG_LEVEL);

struct shell_bt {
	struct {
		shell_transport_handler_t handler;
		void *context;
		struct ring_buf *ringbuf;
	} shell;
	struct {
		struct {
			struct bt_conn *conn;
			struct k_mutex *mutex;
		} conn;
		bool notif_enabled;
	} bt;
};

RING_BUF_DECLARE(shell_bt_ringbuf_rx, 512);
K_MUTEX_DEFINE(shell_bt_conn_mutex);

static struct shell_bt shell_bt_st = {
	.shell.ringbuf = &shell_bt_ringbuf_rx,
	.bt.conn.mutex = &shell_bt_conn_mutex,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		return;
	}

	(void)k_mutex_lock(shell_bt_st.bt.conn.mutex, K_FOREVER);

	__ASSERT(!shell_bt_st.bt.conn.conn, "Connection reference should not be overridden");
	shell_bt_st.bt.conn.conn = bt_conn_ref(conn);

	(void)k_mutex_unlock(shell_bt_st.bt.conn.mutex);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	(void)k_mutex_lock(shell_bt_st.bt.conn.mutex, K_FOREVER);

	__ASSERT(shell_bt_st.bt.conn.conn, "No connection reference");
	bt_conn_unref(shell_bt_st.bt.conn.conn);
	shell_bt_st.bt.conn.conn = NULL;

	(void)k_mutex_unlock(shell_bt_st.bt.conn.mutex);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

#define BT_UUID_SHELL_SERVICE BT_UUID_DECLARE_128(BT_UUID_SHELL_SRV_VAL)
#define BT_UUID_SHELL_RX_CHAR BT_UUID_DECLARE_128(BT_UUID_SHELL_RX_CHAR_VAL)
#define BT_UUID_SHELL_TX_CHAR BT_UUID_DECLARE_128(BT_UUID_SHELL_TX_CHAR_VAL)

static ssize_t shell_bt_chr_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	uint32_t result = 0;

	/** @NOTE: Assume a received command is a complete command */
	result = ring_buf_put(shell_bt_st.shell.ringbuf, buf, len);
	__ASSERT(result == len, "Failed to process incoming message");

	__ASSERT(shell_bt_st.shell.handler, "No handler available");
	shell_bt_st.shell.handler(SHELL_TRANSPORT_EVT_RX_RDY, shell_bt_st.shell.context);

	return len;
}

static void shell_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	shell_bt_st.bt.notif_enabled = !(value == 0);
}

BT_GATT_SERVICE_DEFINE(shell_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_SHELL_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_SHELL_TX_CHAR,
		BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_NONE,
		NULL, NULL, NULL),
	BT_GATT_CCC(shell_ccc_cfg_changed,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_SHELL_RX_CHAR,
		BT_GATT_CHRC_WRITE |
		BT_GATT_CHRC_WRITE_WITHOUT_RESP,
		BT_GATT_PERM_WRITE,
		NULL, shell_bt_chr_write, NULL),
);

static int bt_shell_notify(struct bt_conn *conn, const uint8_t *data, size_t len)
{
	/** @NOTE: Split message in chunks accomodating for current GATT MTU */
	uint32_t max_len = bt_gatt_get_mtu(conn) - 3;
	int err = -1;
	size_t bytes_transferred = 0;
	uint32_t length = 0;

	do {
		length = MIN(max_len, (len - bytes_transferred));

		if (length > 0) {
			LOG_DBG("tx-len: %d, mtu: %d", length, max_len);
			err = bt_gatt_notify(conn, &shell_svc.attrs[1],
				&data[bytes_transferred], length);
			if (err) {
				(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				return err;
			}
			bytes_transferred += length;
		}
	} while (length > 0);

	return len;
}

/**
 * @brief Get a temporary copy of the current BT connection object
 *
 * @details The motivation behind this helper function is to safely use the
 * connection object to send notifications in the write API. Since it is not
 * safe to hold the mutex while calling BT APIs, we are relying on this
 * reference copy to safely use the connection object.
 *
 * @note The caller needs to free this additional connection reference through
 * @ref bt_conn_unref.
 *
 * @retval Connection Reference.
 */
static struct bt_conn *get_conn_ref_copy(void)
{
	struct bt_conn *conn_ref = NULL;

	(void)k_mutex_lock(shell_bt_st.bt.conn.mutex, K_FOREVER);

	if (shell_bt_st.bt.conn.conn) {
		conn_ref = bt_conn_ref(shell_bt_st.bt.conn.conn);
	}

	(void)k_mutex_unlock(shell_bt_st.bt.conn.mutex);

	return conn_ref;
}

static int init(const struct shell_transport *transport, const void *config,
		shell_transport_handler_t evt_handler, void *context)
{
	ARG_UNUSED(config);

	struct shell_bt *sh_bt = (struct shell_bt *)transport->ctx;

	sh_bt->shell.context = context;
	sh_bt->shell.handler = evt_handler;

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	ARG_UNUSED(transport);

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking_tx)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(blocking_tx);

	return 0;
}

static int write(const struct shell_transport *transport,
		const void *data, size_t length, size_t *cnt)
{
	int err = 0;
	struct bt_conn *conn = NULL;

	*cnt = length;

	LOG_DBG("%s: %d", __func__, length);
	LOG_HEXDUMP_DBG(data, length, "data_write");

	/* It is not safe to use BT APIs with the conn mutex locked: rely on a reference copy */
	conn = get_conn_ref_copy();

	if (shell_bt_st.bt.notif_enabled && conn) {
		err = bt_shell_notify(conn, data, length);
		if (err <= 0) {
			LOG_ERR("Failed to notify target");
			err = -EIO;
		} else {
			err = 0;
			__ASSERT(shell_bt_st.shell.handler, "No handler available");
			shell_bt_st.shell.handler(SHELL_TRANSPORT_EVT_TX_RDY,
				shell_bt_st.shell.context);
		}
	}

	if (conn) {
		bt_conn_unref(conn);
	}

	return err;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	*cnt = ring_buf_get(shell_bt_st.shell.ringbuf, data, length);

	return 0;
}

const struct shell_transport_api shell_bt_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read,
};

static struct shell_transport shell_bt_transport = {
	.api = &shell_bt_transport_api,
	.ctx = &shell_bt_st,
};

SHELL_DEFINE(shell_bt, CONFIG_SHELL_PROMPT_BT, &shell_bt_transport,
	CONFIG_SHELL_BACKEND_BT_LOG_MESSAGE_QUEUE_SIZE,
	CONFIG_SHELL_BACKEND_BT_LOG_MESSAGE_QUEUE_TIMEOUT,
	SHELL_FLAG_OLF_CRLF);

const struct shell *shell_backend_bt_get_ptr(void)
{
	return &shell_bt;
}

static int enable_shell_bt(void)
{
	bool log_backend = CONFIG_SHELL_BACKEND_BT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_BACKEND_BT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_BACKEND_BT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
		SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	return shell_init(&shell_bt, NULL, cfg_flags, log_backend, level);
}

SYS_INIT(enable_shell_bt, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
