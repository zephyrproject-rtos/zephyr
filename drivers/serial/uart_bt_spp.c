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
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#define DT_DRV_COMPAT zephyr_bt_spp_uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_bt_spp, CONFIG_UART_LOG_LEVEL);

K_THREAD_STACK_DEFINE(spp_work_queue_stack, CONFIG_UART_BT_SPP_WORKQUEUE_STACK_SIZE);
static struct k_work_q spp_work_queue;

#define SPP_RFCOMM_MTU CONFIG_BT_RFCOMM_L2CAP_MTU

/*
 * RX item: a reference to an RFCOMM frame plus a snapshot of its payload
 * cursor taken at recv time.
 *
 * spp_dlc_recv() runs synchronously inside the BR/EDR L2CAP delivery path.
 * After it returns -EINPROGRESS, bt_br_acl_recv() walks the same net_buf's
 * data/len cursor forward (net_buf_simple_restore() + net_buf_pull()) to
 * parse the next L2CAP PDU in the ACL frame. The buffer reference we hold
 * keeps the backing store alive, but buf->data and buf->len are shared
 * mutable fields and no longer describe our payload by the time the reader
 * drains it. The snapshot pins the read position independently; the held
 * reference is still what keeps the storage valid until
 * bt_rfcomm_dlc_recv_complete().
 */
struct spp_rx_item {
	void *_fifo_reserved;   /* k_fifo linkage; must be first */
	struct net_buf *buf;    /* held reference, passed to recv_complete */
	uint8_t *data;          /* buf->data snapshot at recv time */
	uint16_t len;           /* buf->len snapshot at recv time */
};

/*
 * One item per outstanding RX credit. RFCOMM withholds a credit for every
 * held buffer, so the number in flight is bounded by the RX credit limit,
 * MIN(BT_BUF_ACL_RX_COUNT - 1, 255). Sizing the slab to BT_BUF_ACL_RX_COUNT
 * covers that bound for every SPP instance sharing this driver.
 * BT_BUF_ACL_RX_COUNT is (1 + CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA), which is an
 * integer constant expression suitable for a static slab definition.
 */
K_MEM_SLAB_DEFINE_STATIC(spp_rx_item_slab, sizeof(struct spp_rx_item),
			 BT_BUF_ACL_RX_COUNT, sizeof(void *));

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
		/* RX path: RFCOMM delivers whole frames as net_buf. We hold
		 * each frame via spp_rx_item (which snapshots the payload
		 * cursor at recv time) until its bytes are consumed, then
		 * complete it to restore one RX credit. This makes RFCOMM's
		 * per-frame credit the flow-control mechanism directly, with
		 * no intermediate byte buffer.
		 */
		struct k_fifo rx_fifo;
		struct spp_rx_item *rx_cur;
		struct k_fifo rx_done_fifo;
		struct k_spinlock rx_lock;
		struct ring_buf *tx_ringbuf;
		struct k_work cb_work;
		struct k_work_delayable tx_work;
		struct k_work rx_complete_work;
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
	struct spp_rx_item *item;
	k_spinlock_key_t key;

	atomic_set(&data->bt.connected, 0);

	/* Release any frames still held for backpressure so their buffers
	 * are returned to RFCOMM. Runs on the BT RX thread, so completing
	 * here is safe. Unread payload is discarded on disconnect.
	 */
	key = k_spin_lock(&data->uart.rx_lock);
	item = data->uart.rx_cur;
	data->uart.rx_cur = NULL;
	k_spin_unlock(&data->uart.rx_lock, key);

	if (item != NULL) {
		bt_rfcomm_dlc_recv_complete(dlc, item->buf);
		k_mem_slab_free(&spp_rx_item_slab, item);
	}
	while ((item = k_fifo_get(&data->uart.rx_fifo, K_NO_WAIT)) != NULL) {
		bt_rfcomm_dlc_recv_complete(dlc, item->buf);
		k_mem_slab_free(&spp_rx_item_slab, item);
	}
	while ((item = k_fifo_get(&data->uart.rx_done_fifo, K_NO_WAIT)) != NULL) {
		bt_rfcomm_dlc_recv_complete(dlc, item->buf);
		k_mem_slab_free(&spp_rx_item_slab, item);
	}

	LOG_DBG("SPP UART disconnected");
}

static int spp_dlc_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);
	struct spp_rx_item *item;
	int err;

	/* Take ownership of the frame and defer completion until its bytes have
	 * been read out through the UART API. Returning -EINPROGRESS withholds
	 * one RX credit for the lifetime of this buffer, so the remote peer
	 * stops sending once its outstanding credits (bounded by
	 * rx_credit_limit) are exhausted. This is the RFCOMM-native backpressure.
	 *
	 * We snapshot buf->data and buf->len at this point because
	 * bt_br_acl_recv() walks those shared fields (net_buf_simple_restore()
	 * + net_buf_pull()) after we return, to advance past this L2CAP PDU to
	 * the next one in the same ACL frame. The held reference keeps the
	 * backing store alive; the snapshot keeps our read cursor valid.
	 */
	err = k_mem_slab_alloc(&spp_rx_item_slab, (void **)&item, K_NO_WAIT);
	if (err != 0) {
		/* Pool exhausted — should not happen if slab size >= credit
		 * limit. Signal the error so RFCOMM tears down the DLC rather
		 * than silently dropping data.
		 */
		LOG_ERR("SPP UART: rx item pool exhausted");
		return err;
	}

	item->buf  = buf;
	item->data = buf->data;
	item->len  = buf->len;

	k_fifo_put(&data->uart.rx_fifo, item);
	k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);

	return -EINPROGRESS;
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
			data->bt.sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
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

/*
 * TX needs its own byte FIFO, unlike RX. Received frames arrive as net_bufs
 * owned by RFCOMM and are held in place, so no local buffer is required. TX
 * bytes originate at the UART API with no upstream owner, and three
 * constraints force a buffer here:
 *
 *   - poll_out()/fifo_fill() may run in an ISR and must return promptly, while
 *     bt_rfcomm_dlc_send() reaches L2CAP and blocks on tx_credits, so it can
 *     only run from a thread. The FIFO hands bytes to tx_work for sending.
 *   - RFCOMM TX is credit-gated: the FIFO absorbs writes while credits are
 *     exhausted or the DLC is not yet connected.
 *   - The API is a byte stream but RFCOMM sends whole PDUs. tx_work coalesces
 *     buffered bytes into up-to-MTU frames rather than one frame per byte,
 *     which also matches the small spp_tx_pool.
 */
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
		struct net_buf *buf;

		len = MIN(ring_buf_get_ptr(data->uart.tx_ringbuf, &buf_data, 0),
			  data->bt.dlc.mtu);
		if (len == 0) {
			break;
		}

		buf = bt_rfcomm_create_pdu(&spp_tx_pool);
		if (!buf) {
			LOG_ERR("SPP UART: failed to alloc TX buf");
			break;
		}

		net_buf_add_mem(buf, buf_data, len);
		err = bt_rfcomm_dlc_send(&data->bt.dlc, buf);
		if (err < 0) {
			LOG_ERR("SPP UART: TX failed (err %d)", err);
			net_buf_unref(buf);
			break;
		}

		ring_buf_consume(data->uart.tx_ringbuf, len);
	} while (true);

	if ((ring_buf_space_get(data->uart.tx_ringbuf) > 0) && data->uart.tx_irq_ena) {
		k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
	}
}

/* ---- UART API ---- */

/*
 * Read up to max_len bytes from the held RFCOMM frames into dst.
 * Fully drained frames are moved to rx_done_fifo and their completion
 * (which restores an RX credit and may touch the BT thread) is deferred
 * to the work queue, since this runs from arbitrary UART-caller context
 * including ISRs. rx_lock protects the current-frame cursor against
 * concurrent readers.
 */
static size_t spp_rx_drain(struct uart_bt_spp_data *data, uint8_t *dst, size_t max_len)
{
	size_t copied = 0;
	bool completed = false;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->uart.rx_lock);

	while (copied < max_len) {
		if (data->uart.rx_cur == NULL) {
			data->uart.rx_cur = k_fifo_get(&data->uart.rx_fifo, K_NO_WAIT);
			if (data->uart.rx_cur == NULL) {
				break;
			}
		}

		struct spp_rx_item *item = data->uart.rx_cur;
		size_t take = MIN(max_len - copied, (size_t)item->len);

		memcpy(dst + copied, item->data, take);
		item->data += take;
		item->len  -= (uint16_t)take;
		copied += take;

		if (item->len == 0U) {
			k_fifo_put(&data->uart.rx_done_fifo, item);
			data->uart.rx_cur = NULL;
			completed = true;
		}
	}

	k_spin_unlock(&data->uart.rx_lock, key);

	if (completed) {
		k_work_submit_to_queue(&spp_work_queue, &data->uart.rx_complete_work);
	}

	return copied;
}

/* Whether any received bytes are available to read. */
static bool spp_rx_has_data(struct uart_bt_spp_data *data)
{
	bool has_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->uart.rx_lock);
	has_data = data->uart.rx_cur != NULL || !k_fifo_is_empty(&data->uart.rx_fifo);
	k_spin_unlock(&data->uart.rx_lock, key);

	return has_data;
}

/*
 * Completing a frame calls bt_rfcomm_dlc_recv_complete() which restores an
 * RX credit and may reach bt_l2cap_send(); it must run from a thread, never
 * an ISR. The UART read path can run from any context, so drained frames are
 * queued and completed here on the work queue.
 */
static void spp_rx_complete_work_handler(struct k_work *work)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(work, struct uart_bt_spp_data,
						     uart.rx_complete_work);
	struct spp_rx_item *item;

	while ((item = k_fifo_get(&data->uart.rx_done_fifo, K_NO_WAIT)) != NULL) {
		bt_rfcomm_dlc_recv_complete(&data->bt.dlc, item->buf);
		k_mem_slab_free(&spp_rx_item_slab, item);
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

	ret = spp_rx_drain(data, c, 1) == 1 ? 0 : -1;

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

	if (size <= 0) {
		return 0;
	}

	return (int)spp_rx_drain(data, rx_data, (size_t)size);
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

	return spp_rx_has_data(data) && data->uart.rx_irq_ena ? 1 : 0;
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
	.irq_callback_set = uart_bt_spp_irq_callback_set,
};

/* ---- Init ---- */

static int uart_bt_spp_workqueue_init(void)
{
	k_work_queue_init(&spp_work_queue);
	k_work_queue_start(&spp_work_queue, spp_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(spp_work_queue_stack),
			   CONFIG_UART_BT_SPP_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(spp_work_queue.thread_id, "uart_bt_spp");
	return 0;
}

SYS_INIT(uart_bt_spp_workqueue_init, POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY);

static int uart_bt_spp_init(const struct device *dev)
{
	struct uart_bt_spp_data *data = dev->data;

	data->uart.callback.dev = dev;

	k_fifo_init(&data->uart.rx_fifo);
	k_fifo_init(&data->uart.rx_done_fifo);

	k_work_init_delayable(&data->uart.tx_work, spp_tx_work_handler);
	k_work_init(&data->uart.cb_work, spp_cb_work_handler);
	k_work_init(&data->uart.rx_complete_work, spp_rx_complete_work_handler);

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
	RING_BUF_DECLARE(spp_tx_rb_##n, DT_INST_PROP(n, tx_fifo_size));		\
										\
	static struct uart_bt_spp_data uart_bt_spp_data_##n = {			\
		.bt = {								\
			.connected = ATOMIC_INIT(0),				\
			.sdp_record = &spp_sdp_rec_##n,				\
		},								\
		.uart = {							\
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
