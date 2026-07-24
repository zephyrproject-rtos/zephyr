/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#define DT_DRV_COMPAT zephyr_bt_spp_uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_bt_spp, CONFIG_UART_LOG_LEVEL);

K_THREAD_STACK_DEFINE(spp_work_queue_stack, CONFIG_UART_BT_SPP_WORKQUEUE_STACK_SIZE);
static struct k_work_q spp_work_queue;

#define SPP_RFCOMM_MTU CONFIG_BT_RFCOMM_L2CAP_MTU

struct uart_bt_spp_data {
	struct {
		struct bt_rfcomm_dlc dlc;
		struct bt_rfcomm_dlc_ops dlc_ops;
		struct bt_rfcomm_server server;
		struct bt_sdp_record *sdp_record;
		struct bt_sdp_discover_params sdp_discover;
		atomic_t connected;
		uint8_t channel;
	} bt;
	struct {
		struct ring_buf *rx_ringbuf;
		struct ring_buf *tx_ringbuf;
		struct k_work cb_work;
		struct k_work_delayable tx_work;
		struct k_work unthrottle_work;
		bool rx_irq_ena;
		bool tx_irq_ena;
		struct {
			const struct device *dev;
			uart_irq_callback_user_data_t cb;
			void *cb_data;
		} callback;
	} uart;
};

struct uart_bt_spp_config {
	uint8_t channel;
	bool is_server;
	uint8_t *sdp_channel;
};

/* ---- Net buf pool for TX ---- */

NET_BUF_POOL_FIXED_DEFINE(spp_tx_pool, CONFIG_BT_MAX_CONN,
			  BT_RFCOMM_BUF_SIZE(SPP_RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define SDP_CLIENT_BUF_LEN 512
NET_BUF_POOL_FIXED_DEFINE(spp_sdp_pool, 1, SDP_CLIENT_BUF_LEN, 8, NULL);

/* ---- Per-instance SDP record macro ---- */

/*
 * Each DT instance gets its own SDP record with an independent channel
 * variable, following the Linux BlueZ model where each rfcomm_dev has
 * its own service registration. In Zephyr, SDP records are kernel-managed
 * static structures, so we generate them per-instance at compile time.
 */
#define SPP_SDP_RECORD_DEFINE(n)						\
	static uint8_t spp_sdp_channel_##n;					\
										\
	static struct bt_sdp_attribute spp_sdp_attrs_##n[] = {			\
		BT_SDP_NEW_SERVICE,						\
		BT_SDP_LIST(							\
			BT_SDP_ATTR_SVCLASS_ID_LIST,				\
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),			\
			BT_SDP_DATA_ELEM_LIST(					\
			{							\
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),		\
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)	\
			},							\
			)							\
		),								\
		BT_SDP_LIST(							\
			BT_SDP_ATTR_PROTO_DESC_LIST,				\
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),			\
			BT_SDP_DATA_ELEM_LIST(					\
			{							\
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),		\
				BT_SDP_DATA_ELEM_LIST(				\
				{						\
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),	\
					BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)	\
				},						\
				)						\
			},							\
			{							\
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),		\
				BT_SDP_DATA_ELEM_LIST(				\
				{						\
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),	\
					BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)	\
				},						\
				{						\
					BT_SDP_TYPE_SIZE(BT_SDP_UINT8),		\
					&spp_sdp_channel_##n			\
				},						\
				)						\
			},							\
			)							\
		),								\
		BT_SDP_LIST(							\
			BT_SDP_ATTR_PROFILE_DESC_LIST,				\
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),			\
			BT_SDP_DATA_ELEM_LIST(					\
			{							\
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),		\
				BT_SDP_DATA_ELEM_LIST(				\
				{						\
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),	\
					BT_SDP_ARRAY_16(			\
						BT_SDP_SERIAL_PORT_SVCLASS)	\
				},						\
				{						\
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),	\
					BT_SDP_ARRAY_16(0x0102)			\
				},						\
				)						\
			},							\
			)							\
		),								\
		BT_SDP_SERVICE_NAME("Serial Port"),				\
	};									\
										\
	static struct bt_sdp_record spp_sdp_rec_##n =				\
		BT_SDP_RECORD(spp_sdp_attrs_##n)

/* ---- DLC callbacks ---- */

static void spp_dlc_connected(struct bt_rfcomm_dlc *dlc)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);

	atomic_set(&data->bt.connected, 1);
	LOG_DBG("SPP UART connected");

	if (!ring_buf_is_empty(data->uart.tx_ringbuf)) {
		k_work_reschedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_NO_WAIT);
	}
}

static void spp_dlc_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);

	atomic_set(&data->bt.connected, 0);
	LOG_DBG("SPP UART disconnected");
}

static void spp_dlc_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);
	uint32_t put_len;

	put_len = ring_buf_put(data->uart.rx_ringbuf, buf->data, buf->len);
	if (put_len < buf->len) {
		LOG_ERR("RX ring buffer full, dropped %u bytes", buf->len - put_len);
	}

	/* Throttle when RX ring buffer is more than 75% full.
	 * This stops restoring RFCOMM credits to the remote peer,
	 * causing it to pause transmission once its TX credits are
	 * exhausted. Hysteresis: unthrottle happens at 50% (see
	 * poll_in/fifo_read), preventing rapid throttle/unthrottle
	 * oscillation.
	 */
	if (ring_buf_space_get(data->uart.rx_ringbuf) <
	    ring_buf_capacity_get(data->uart.rx_ringbuf) / 4) {
		bt_rfcomm_dlc_throttle(dlc);
	}

	k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
}

/* ---- Server mode ---- */

static int spp_server_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			     struct bt_rfcomm_dlc **dlc)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(server, struct uart_bt_spp_data, bt.server);

	if (atomic_get(&data->bt.connected)) {
		return -EALREADY;
	}

	data->bt.dlc.ops = &data->bt.dlc_ops;
	data->bt.dlc.mtu = SPP_RFCOMM_MTU;
	/* Require at least authenticated pairing (SSP) for SPP connections */
	data->bt.dlc.required_sec_level = BT_SECURITY_L2;
	*dlc = &data->bt.dlc;

	LOG_DBG("SPP UART accepted connection");
	return 0;
}

/* ---- Client mode ---- */

static DEVICE_API(uart, uart_bt_spp_driver_api);

static uint8_t spp_sdp_discover_cb(struct bt_conn *conn,
				   struct bt_sdp_client_result *result,
				   const struct bt_sdp_discover_params *params)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(params, struct uart_bt_spp_data,
						     bt.sdp_discover);
	uint16_t channel;
	int err;

	if (result == NULL || result->resp_buf == NULL) {
		LOG_ERR("SPP UART: SDP service not found");
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		LOG_ERR("SPP UART: failed to get RFCOMM channel from SDP");
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	data->bt.dlc.ops = &data->bt.dlc_ops;
	data->bt.dlc.mtu = SPP_RFCOMM_MTU;
	data->bt.dlc.required_sec_level = BT_SECURITY_L2;

	err = bt_rfcomm_dlc_connect(conn, &data->bt.dlc, (uint8_t)channel);
	if (err < 0) {
		LOG_ERR("SPP UART: RFCOMM connect failed (err %d)", err);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static void spp_client_connected_cb(struct bt_conn *conn, uint8_t err)
{
	static struct bt_uuid_16 spp_uuid = BT_UUID_INIT_16(BT_SDP_SERIAL_PORT_SVCLASS);
	struct bt_conn_info info;
	int ret;

	if (err) {
		return;
	}

	ret = bt_conn_get_info(conn, &info);
	if (ret || info.type != BT_CONN_TYPE_BR) {
		return;
	}

	/* Find the first idle client instance and initiate connection */
	STRUCT_SECTION_FOREACH(device, dev) {
		struct uart_bt_spp_data *data;
		const struct uart_bt_spp_config *cfg;

		if (dev->api != &uart_bt_spp_driver_api) {
			continue;
		}

		cfg = dev->config;
		data = dev->data;

		if (cfg->is_server || atomic_get(&data->bt.connected)) {
			continue;
		}

		data->bt.dlc.ops = &data->bt.dlc_ops;
		data->bt.dlc.mtu = SPP_RFCOMM_MTU;
		data->bt.dlc.required_sec_level = BT_SECURITY_L2;

		if (cfg->channel > 0) {
			/* Channel known from DT, connect directly */
			ret = bt_rfcomm_dlc_connect(conn, &data->bt.dlc, cfg->channel);
			if (ret < 0) {
				LOG_ERR("SPP UART: RFCOMM connect failed (err %d)", ret);
			}
		} else {
			/* Channel unknown, discover via SDP first */
			data->bt.sdp_discover.uuid = &spp_uuid.uuid;
			data->bt.sdp_discover.func = spp_sdp_discover_cb;
			data->bt.sdp_discover.pool = &spp_sdp_pool;
			ret = bt_sdp_discover(conn, &data->bt.sdp_discover);
			if (ret < 0) {
				LOG_ERR("SPP UART: SDP discover failed (err %d)", ret);
			}
		}
		break;
	}
}

static struct bt_conn_cb spp_conn_cb = {
	.connected = spp_client_connected_cb,
};

/* ---- TX path ---- */

static void spp_tx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_bt_spp_data *data = CONTAINER_OF(dwork, struct uart_bt_spp_data,
						     uart.tx_work);
	uint8_t *buf_data;
	size_t len;
	int err;

	if (!atomic_get(&data->bt.connected)) {
		return;
	}

	do {
		len = ring_buf_get_claim(data->uart.tx_ringbuf, &buf_data,
					 data->bt.dlc.mtu);
		if (len > 0) {
			struct net_buf *buf;

			buf = bt_rfcomm_create_pdu(&spp_tx_pool);
			if (!buf) {
				LOG_ERR("SPP UART: failed to alloc TX buf");
				ring_buf_get_finish(data->uart.tx_ringbuf, 0);
				break;
			}

			net_buf_add_mem(buf, buf_data, len);
			err = bt_rfcomm_dlc_send(&data->bt.dlc, buf);
			if (err < 0) {
				LOG_ERR("SPP UART: TX failed (err %d)", err);
				net_buf_unref(buf);
				ring_buf_get_finish(data->uart.tx_ringbuf, 0);
				break;
			}
		}

		ring_buf_get_finish(data->uart.tx_ringbuf, len);
	} while (len > 0);

	if ((ring_buf_space_get(data->uart.tx_ringbuf) > 0) && data->uart.tx_irq_ena) {
		k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
	}
}

/* ---- UART API ---- */

/*
 * Unthrottle must be called from the work queue context because
 * bt_rfcomm_dlc_unthrottle() -> rfcomm_dlc_update_credits() ->
 * rfcomm_send_credit() -> bt_l2cap_send() requires BT thread safety.
 * The UART read path (poll_in/fifo_read) may run from any thread,
 * so we defer the actual unthrottle to the work queue.
 */
static void spp_unthrottle_work_handler(struct k_work *work)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(work, struct uart_bt_spp_data,
						     uart.unthrottle_work);

	if (data->bt.dlc.rx_throttled) {
		bt_rfcomm_dlc_unthrottle(&data->bt.dlc);
	}
}

static void spp_cb_work_handler(struct k_work *work)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(work, struct uart_bt_spp_data, uart.cb_work);

	if (data->uart.callback.cb) {
		data->uart.callback.cb(data->uart.callback.dev, data->uart.callback.cb_data);
	}
}

static int uart_bt_spp_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_bt_spp_data *data = dev->data;
	int ret;

	ret = ring_buf_get(data->uart.rx_ringbuf, c, 1) == 1 ? 0 : -1;

	/* Schedule unthrottle when buffer drops below 50% full.
	 * Deferred to work queue for thread safety (see comment
	 * on spp_unthrottle_work_handler).
	 */
	if (ret == 0 && data->bt.dlc.rx_throttled &&
	    ring_buf_space_get(data->uart.rx_ringbuf) >
	    ring_buf_capacity_get(data->uart.rx_ringbuf) / 2) {
		k_work_submit_to_queue(&spp_work_queue, &data->uart.unthrottle_work);
	}

	return ret;
}

static void uart_bt_spp_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_bt_spp_data *data = dev->data;

	while (!ring_buf_put(data->uart.tx_ringbuf, &c, 1)) {
		if (k_is_in_isr() || !atomic_get(&data->bt.connected)) {
			LOG_WRN_ONCE("TX ring buffer full");
			break;
		}
		k_sleep(K_MSEC(1));
	}

	if (atomic_get(&data->bt.connected)) {
		k_work_schedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_MSEC(1));
	}
}

static int uart_bt_spp_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_bt_spp_data *data = dev->data;
	size_t wrote;

	wrote = ring_buf_put(data->uart.tx_ringbuf, tx_data, len);

	if (atomic_get(&data->bt.connected)) {
		k_work_reschedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_NO_WAIT);
	}

	return wrote;
}

static int uart_bt_spp_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_bt_spp_data *data = dev->data;
	int read;

	read = ring_buf_get(data->uart.rx_ringbuf, rx_data, size);

	/* Schedule unthrottle when buffer drops below 50% full.
	 * Deferred to work queue for thread safety.
	 */
	if (read > 0 && data->bt.dlc.rx_throttled &&
	    ring_buf_space_get(data->uart.rx_ringbuf) >
	    ring_buf_capacity_get(data->uart.rx_ringbuf) / 2) {
		k_work_submit_to_queue(&spp_work_queue, &data->uart.unthrottle_work);
	}

	return read;
}

static int uart_bt_spp_irq_tx_ready(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	return (ring_buf_space_get(data->uart.tx_ringbuf) > 0) && data->uart.tx_irq_ena ? 1 : 0;
}

static void uart_bt_spp_irq_tx_enable(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	data->uart.tx_irq_ena = true;
	if (uart_bt_spp_irq_tx_ready(dev)) {
		k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
	}
}

static void uart_bt_spp_irq_tx_disable(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	data->uart.tx_irq_ena = false;
}

static int uart_bt_spp_irq_rx_ready(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	return !ring_buf_is_empty(data->uart.rx_ringbuf) && data->uart.rx_irq_ena ? 1 : 0;
}

static void uart_bt_spp_irq_rx_enable(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	data->uart.rx_irq_ena = true;
	k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
}

static void uart_bt_spp_irq_rx_disable(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	data->uart.rx_irq_ena = false;
}

static int uart_bt_spp_irq_is_pending(const struct device *dev)
{
	return uart_bt_spp_irq_rx_ready(dev) || uart_bt_spp_irq_tx_ready(dev);
}

static int uart_bt_spp_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_bt_spp_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct uart_bt_spp_data *data = dev->data;

	data->uart.callback.cb = cb;
	data->uart.callback.cb_data = cb_data;
}

static DEVICE_API(uart, uart_bt_spp_driver_api) = {
	.poll_in = uart_bt_spp_poll_in,
	.poll_out = uart_bt_spp_poll_out,
	.fifo_fill = uart_bt_spp_fifo_fill,
	.fifo_read = uart_bt_spp_fifo_read,
	.irq_tx_enable = uart_bt_spp_irq_tx_enable,
	.irq_tx_disable = uart_bt_spp_irq_tx_disable,
	.irq_tx_ready = uart_bt_spp_irq_tx_ready,
	.irq_rx_enable = uart_bt_spp_irq_rx_enable,
	.irq_rx_disable = uart_bt_spp_irq_rx_disable,
	.irq_rx_ready = uart_bt_spp_irq_rx_ready,
	.irq_is_pending = uart_bt_spp_irq_is_pending,
	.irq_update = uart_bt_spp_irq_update,
	.irq_callback_set = uart_bt_spp_irq_callback_set,
};

/* ---- Init ---- */

static int uart_bt_spp_workqueue_init(void)
{
	k_work_queue_init(&spp_work_queue);
	k_work_queue_start(&spp_work_queue, spp_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(spp_work_queue_stack),
			   CONFIG_UART_BT_SPP_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(&spp_work_queue.thread, "uart_bt_spp");
	return 0;
}

SYS_INIT(uart_bt_spp_workqueue_init, POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY);

static int uart_bt_spp_init(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	data->uart.callback.dev = dev;

	k_work_init_delayable(&data->uart.tx_work, spp_tx_work_handler);
	k_work_init(&data->uart.cb_work, spp_cb_work_handler);
	k_work_init(&data->uart.unthrottle_work, spp_unthrottle_work_handler);

	data->bt.dlc_ops.connected = spp_dlc_connected;
	data->bt.dlc_ops.disconnected = spp_dlc_disconnected;
	data->bt.dlc_ops.recv = spp_dlc_recv;

	return 0;
}

/*
 * BT service registration should be in place before the application starts
 * establishing BR/EDR connections. Register at APPLICATION init priority so
 * RFCOMM/SDP state is set up during boot and ready once the app calls bt_enable().
 */
static int uart_bt_spp_bt_register(void)
{
	int err;

	STRUCT_SECTION_FOREACH(device, dev) {
		const struct uart_bt_spp_config *cfg;
		struct uart_bt_spp_data *data;

		if (dev->api != &uart_bt_spp_driver_api) {
			continue;
		}

		cfg = dev->config;
		data = dev->data;

		if (cfg->is_server) {
			data->bt.server.channel = cfg->channel;
			data->bt.server.accept = spp_server_accept;

			err = bt_rfcomm_server_register(&data->bt.server);
			if (err < 0) {
				LOG_ERR("SPP UART: RFCOMM server register failed (err %d)", err);
				continue;
			}

			*cfg->sdp_channel = data->bt.server.channel;

			err = bt_sdp_register_service(data->bt.sdp_record);
			if (err < 0) {
				LOG_ERR("SPP UART: SDP register failed (err %d)", err);
				continue;
			}

			LOG_DBG("SPP UART server registered (channel %u)",
				data->bt.server.channel);
		} else {
			data->bt.channel = cfg->channel;
			bt_conn_cb_register(&spp_conn_cb);
			LOG_DBG("SPP UART client mode (channel %u)", cfg->channel);
		}
	}

	return 0;
}

SYS_INIT(uart_bt_spp_bt_register, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* ---- DT instantiation ---- */

#define UART_BT_SPP_IS_SERVER(n) \
	(DT_INST_ENUM_IDX(n, role) == 0)

#define UART_BT_SPP_INIT(n)							\
										\
	SPP_SDP_RECORD_DEFINE(n);						\
										\
	RING_BUF_DECLARE(spp_rx_rb_##n, DT_INST_PROP(n, rx_fifo_size));		\
	RING_BUF_DECLARE(spp_tx_rb_##n, DT_INST_PROP(n, tx_fifo_size));		\
										\
	static struct uart_bt_spp_data uart_bt_spp_data_##n = {			\
		.bt = {								\
			.connected = ATOMIC_INIT(0),				\
			.sdp_record = &spp_sdp_rec_##n,				\
		},								\
		.uart = {							\
			.rx_ringbuf = &spp_rx_rb_##n,				\
			.tx_ringbuf = &spp_tx_rb_##n,				\
		},								\
	};									\
										\
	static const struct uart_bt_spp_config uart_bt_spp_config_##n = {	\
		.channel = DT_INST_PROP(n, channel),				\
		.is_server = UART_BT_SPP_IS_SERVER(n),				\
		.sdp_channel = &spp_sdp_channel_##n,				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uart_bt_spp_init, NULL,			\
			      &uart_bt_spp_data_##n,				\
			      &uart_bt_spp_config_##n,				\
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,		\
			      &uart_bt_spp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BT_SPP_INIT)
