/* Copyright (c) 2019-2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(hci_ipc, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_HAS_CHOSEN(zephyr_bt_hci)
#define BT_HCI_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci))
#else
/* The zephyr,bt-hci chosen property is mandatory, except for unit tests */
BUILD_ASSERT(IS_ENABLED(CONFIG_ZTEST), "Missing DT chosen property for HCI");
#define BT_HCI_DEV NULL
#endif

/* Undefined reference? Make sure the HCI driver is compiled in. */
static const struct device *hci_dev = BT_HCI_DEV;
static struct ipc_ept hci_ept;

static struct k_poll_signal ipc_bound_signal = K_POLL_SIGNAL_INITIALIZER(ipc_bound_signal);
static struct k_poll_signal hci_opened_signal = K_POLL_SIGNAL_INITIALIZER(hci_opened_signal);

enum {
	hci_ipc_reserve = 1
};

void h2c_pool_destroy(struct net_buf *buf)
{
	int ret;

	LOG_DBG("net_buf %p IPC buf %p", buf, buf->__buf);

	if (buf->__buf != NULL) {
		ret = ipc_service_release_rx_buffer(&hci_ept, buf->__buf);
		if (ret < 0) {
			k_oops();
		}
	}

	net_buf_destroy(buf);
}

NET_BUF_POOL_FIXED_DEFINE(h2c_pool, 1, 0, sizeof(struct bt_buf_data), h2c_pool_destroy);

void c2h_pool_destroy(struct net_buf *buf)
{
	int ret;
	void *ipc_buf = NULL;

	if (buf->__buf != NULL) {
		/* Adjust for the reserved H4 type byte. */
		ipc_buf = buf->__buf - hci_ipc_reserve;
	}

	LOG_DBG("net_buf %p IPC buf %p", buf, ipc_buf);

	if (ipc_buf != NULL) {
		ret = ipc_service_drop_tx_buffer(&hci_ept, ipc_buf);
		if (ret < 0) {
			k_oops();
		}
	}

	net_buf_destroy(buf);
}

/* ZLL needs two buffers here to not hang.
 *
 * The second buffer is probably needed when ZLL allocates while
 * holding up the IPC thread. This needs to be investigated
 * further.
 */
NET_BUF_POOL_FIXED_DEFINE(c2h_pool, 2, 0, sizeof(struct bt_buf_data), c2h_pool_destroy);

/* Implements <zephyr/bluetooth/buf.h> */
struct net_buf *bt_buf_get_rx(enum bt_buf_type type, k_timeout_t timeout)
{
	int err;
	void *ipc_buf;
	uint8_t *data_buf;
	size_t ipc_buf_size = BT_BUF_RX_SIZE;
	size_t data_buf_size;
	struct net_buf *buf;

	/* FIXME: The timeout should cover both allocations? */
	err = ipc_service_get_tx_buffer(&hci_ept, &ipc_buf, &ipc_buf_size, timeout);

	if (err) {
		return NULL;
	}

	/* Reserve one byte for H4 type, hidden from the Controller.
	 *
	 * We can't use net_buf_reserve() because the Controller driver
	 * might call net_buf_reserve() itself, which would remove our
	 * reservation.
	 */
	data_buf = ipc_buf;
	data_buf = &data_buf[hci_ipc_reserve];
	data_buf_size = ipc_buf_size - hci_ipc_reserve;
	buf = net_buf_alloc_with_data(&c2h_pool, data_buf, data_buf_size, timeout);

	if (!buf) {
		err = ipc_service_drop_tx_buffer(&hci_ept, ipc_buf);
		if (err) {
			k_oops();
		}

		return NULL;
	}

	buf->len = 0;
	bt_buf_set_type(buf, type);

	LOG_DBG("net_buf IPC buf %p", buf, ipc_buf);

	return buf;
}

/* Implements <zephyr/bluetooth/buf.h> */
struct net_buf *bt_buf_get_evt(uint8_t evt, bool discardable, k_timeout_t timeout)
{
	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

int signal_wait(struct k_poll_signal *sig)
{
	struct k_poll_event evs[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, sig),
	};

	return k_poll(evs, ARRAY_SIZE(evs), K_FOREVER);
}

static void ipc_ept_bound(void *priv)
{
	k_poll_signal_raise(&ipc_bound_signal, 0);
	signal_wait(&hci_opened_signal);
}

K_FIFO_DEFINE(tx_fifo);

static void ipc_h2c_recv(const void *data, size_t len, void *priv)
{
	int err;
	uint8_t h4_type;
	struct net_buf *buf = NULL;

	err = ipc_service_hold_rx_buffer(&hci_ept, (void *)data);
	if (err) {
		k_oops();
	}

	/* Construct zero-copy net_buf. */
	buf = net_buf_alloc_with_data(&h2c_pool, (void *)data, len, K_FOREVER);
	if (!buf) {
		k_oops();
	}

	h4_type = net_buf_pull_u8(buf);
	bt_buf_set_type(buf, bt_buf_h4_type_to_out_type(h4_type));

	LOG_DBG("net_buf %p IPC buf %p", buf, buf->__buf);

	k_fifo_put(&tx_fifo, buf);
}

static struct ipc_ept_cfg hci_ept_cfg = {.name = "nrf_bt_hci",
					 .cb = {
						 .bound = ipc_ept_bound,
						 .received = ipc_h2c_recv,
					 }};

int hci_c2h_recv(const struct device *dev, struct net_buf *buf)
{
	uint8_t h4_type;
	uint8_t *ipc_buf;
	size_t ipc_msg_size;
	int ret;

	signal_wait(&ipc_bound_signal);

	/* Do the IPC send on the signaling thread. We assume it won't
	 * block.
	 */

	h4_type = bt_buf_type_to_h4_type(bt_buf_get_type(buf));
	if (h4_type == BT_HCI_H4_NONE) {
		k_oops();
	}

	if (buf->data != buf->__buf) {
		LOG_WRN_ONCE("Start of data is unaligned with IPC buffer. Memmove required.");
		if (IS_ENABLED(CONFIG_BT_TESTING)) {
			k_oops();
		}
		memmove(buf->__buf, buf->data, buf->len);
		buf->data = buf->__buf;
	}

	ipc_buf = buf->data - hci_ipc_reserve;
	ipc_buf[0] = h4_type;
	ipc_msg_size = buf->len + hci_ipc_reserve;

	LOG_DBG("net_buf %p IPC buf %p", buf, ipc_buf);

	ret = ipc_service_send_nocopy(&hci_ept, ipc_buf, ipc_msg_size);

	if (ret < 0) {
		k_oops();
	}

	/* ipc_service_send_nocopy took ownership of buf->data */
	buf->__buf = NULL;

	/* Clear any dangling pointers, just in case. */
	buf->data = NULL;
	buf->size = 0;
	buf->len = 0;

	net_buf_unref(buf);

	return 0;
}

int main(void)
{
	int err;
	const struct device *hci_ipc_instance = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci_ipc));

	/* Initialize IPC service instance and register endpoint. */
	err = ipc_service_open_instance(hci_ipc_instance);
	if (err < 0 && err != -EALREADY) {
		k_oops();
	}

	err = ipc_service_register_endpoint(hci_ipc_instance, &hci_ept, &hci_ept_cfg);
	if (err) {
		k_oops();
	}

	signal_wait(&ipc_bound_signal);

	err = bt_hci_open(hci_dev, hci_c2h_recv);
	if (err) {
		k_oops();
	}

	k_poll_signal_raise(&hci_opened_signal, 0);

	while (true) {
		struct net_buf *buf;

		buf = k_fifo_get(&tx_fifo, K_FOREVER);
		err = bt_hci_send(hci_dev, buf);
		if (err) {
			k_oops();
		}
	}

	return 0;
}

void bt_ctlr_assert_handle(char *file, uint32_t line)
{
	(void)irq_lock();
	LOG_ERR("Controller assert in: %s at %d", file, line);
	LOG_PANIC();
	while (true) {
	};
}
