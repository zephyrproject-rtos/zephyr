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
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#define DT_DRV_COMPAT zephyr_bt_spp_uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_bt_spp, CONFIG_UART_LOG_LEVEL);

K_THREAD_STACK_DEFINE(spp_work_queue_stack, CONFIG_UART_BT_SPP_WORKQUEUE_STACK_SIZE);
static struct k_work_q spp_work_queue;

#define SPP_RFCOMM_MTU CONFIG_BT_RFCOMM_L2CAP_MTU

/* Highest RFCOMM server channel number */
#define SPP_RFCOMM_CHANNEL_MAX 30

/*
 * RX item: a reference to an RFCOMM frame plus a snapshot of its payload
 * cursor taken at recv time.
 *
 * The held reference keeps the storage valid until
 * bt_rfcomm_dlc_recv_complete(), but the reference does not own the buffer's
 * data/len cursor: the caller of recv() may move it after recv() returns.
 * The snapshot keeps the read position independent of it.
 */
struct spp_rx_item {
	void *_fifo_reserved; /* k_fifo linkage; must be first */
	struct net_buf *buf;  /* held reference, passed to recv_complete */
	uint8_t *data;        /* buf->data snapshot at recv time */
	uint16_t len;         /* buf->len snapshot at recv time */
};

/*
 * Number of RX items per instance. RFCOMM withholds a credit for every held
 * buffer, so the frames held by one DLC are bounded by its RX credit limit,
 * MIN(BT_BUF_ACL_RX_COUNT - 1, 255). Each instance has its own slab of this
 * size, so the bound holds with several instances connected at once.
 */
#define SPP_RX_ITEM_COUNT BT_BUF_ACL_RX_COUNT

struct uart_bt_spp_data {
	const struct device *dev;
	struct {
		struct bt_rfcomm_dlc dlc;
		struct bt_rfcomm_dlc_ops dlc_ops;
		struct bt_rfcomm_server server;
		struct bt_sdp_record sdp_record;
		/* DLC handed to RFCOMM, from accept() until disconnected() */
		atomic_t busy;
		atomic_t connected;
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
		/* Serializes the TX ring buffer producers */
		struct k_spinlock tx_lock;
		/* Free buffers in the instance's TX pool */
		struct k_sem tx_bufs;
		struct k_work cb_work;
		struct k_work_delayable tx_work;
		struct k_work rx_complete_work;
		bool rx_irq_ena;
		bool tx_irq_ena;
		struct {
			uart_irq_callback_user_data_t cb;
			void *cb_data;
		} callback;
	} uart;
};

struct uart_bt_spp_config {
	struct ring_buf *tx_ringbuf;
	struct net_buf_pool *tx_pool;
	struct k_mem_slab *rx_slab;
	uint8_t *sdp_channel;
	uint8_t channel;
};

static inline const struct uart_bt_spp_config *spp_config(const struct uart_bt_spp_data *data)
{
	return data->dev->config;
}

/* ---- Per-instance SDP record macro ---- */

/*
 * Each DT instance gets its own SDP record with an independent channel
 * variable, following the Linux BlueZ model where each rfcomm_dev has
 * its own service registration. The attribute list and channel variable are
 * generated per instance at compile time; the record itself lives in the
 * instance data.
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
	}

/* ---- DLC callbacks ---- */

static void spp_dlc_connected(struct bt_rfcomm_dlc *dlc)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);

	atomic_set(&data->bt.connected, 1);
	LOG_DBG("SPP UART connected");

	if (!ring_buf_is_empty(spp_config(data)->tx_ringbuf)) {
		k_work_reschedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_NO_WAIT);
	}
}

static void spp_dlc_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);
	struct k_mem_slab *rx_slab = spp_config(data)->rx_slab;
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
		k_mem_slab_free(rx_slab, item);
	}
	while ((item = k_fifo_get(&data->uart.rx_fifo, K_NO_WAIT)) != NULL) {
		bt_rfcomm_dlc_recv_complete(dlc, item->buf);
		k_mem_slab_free(rx_slab, item);
	}
	while ((item = k_fifo_get(&data->uart.rx_done_fifo, K_NO_WAIT)) != NULL) {
		bt_rfcomm_dlc_recv_complete(dlc, item->buf);
		k_mem_slab_free(rx_slab, item);
	}

	/* RFCOMM no longer references the DLC; it can be handed out again */
	atomic_clear(&data->bt.busy);

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
	 * buf->data and buf->len are snapshotted here, see struct spp_rx_item.
	 */
	err = k_mem_slab_alloc(spp_config(data)->rx_slab, (void **)&item, K_NO_WAIT);
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

	/* One DLC per instance: reserve it until disconnected() releases it,
	 * so a second peer cannot get the same DLC while the first connection
	 * is still being set up (e.g. during pairing).
	 */
	if (!atomic_cas(&data->bt.busy, 0, 1)) {
		return -EALREADY;
	}

	data->bt.dlc.ops = &data->bt.dlc_ops;
	data->bt.dlc.mtu = SPP_RFCOMM_MTU;
	/* Require an encrypted link (no MITM protection required) */
	data->bt.dlc.required_sec_level = BT_SECURITY_L2;
	*dlc = &data->bt.dlc;

	LOG_DBG("SPP UART accepted connection");
	return 0;
}

/* ---- TX path ---- */

/*
 * TX needs its own byte FIFO, unlike RX. Received frames arrive as net_bufs
 * owned by RFCOMM and are held in place, so no local buffer is required. TX
 * bytes originate at the UART API with no upstream owner, and three
 * constraints force a buffer here:
 *
 *   - poll_out()/fifo_fill() may run in an ISR and must return promptly, so
 *     they only copy into the FIFO and tx_work builds and sends the frames.
 *   - RFCOMM TX is credit-gated: the FIFO absorbs writes while credits are
 *     exhausted or the DLC is not yet connected.
 *   - The API is a byte stream but RFCOMM sends whole PDUs. tx_work coalesces
 *     buffered bytes into up-to-MTU frames rather than one frame per byte.
 *
 * tx_work never blocks: a frame passed to bt_rfcomm_dlc_send() keeps its
 * buffer until the peer grants a credit and the frame is sent, so waiting for
 * a buffer here would also stall rx_complete_work on the same queue and with
 * it the RX credits the peer may be waiting for. tx_work only sends while
 * tx_bufs reports a free buffer, and spp_tx_buf_destroy() resumes it when a
 * buffer returns to the pool.
 */
static void spp_tx_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_bt_spp_data *data = CONTAINER_OF(dwork, struct uart_bt_spp_data,
						     uart.tx_work);
	const struct uart_bt_spp_config *cfg = spp_config(data);
	uint8_t *buf_data;
	size_t len;
	int err;

	while (atomic_get(&data->bt.connected) != 0) {
		struct net_buf *buf;

		len = MIN(ring_buf_get_ptr(cfg->tx_ringbuf, &buf_data, 0), data->bt.dlc.mtu);
		if (len == 0) {
			break;
		}

		if (k_sem_take(&data->uart.tx_bufs, K_NO_WAIT) != 0) {
			break;
		}

		/* A pool buffer is free, so this does not block */
		buf = bt_rfcomm_create_pdu(cfg->tx_pool);
		net_buf_add_mem(buf, buf_data, len);

		err = bt_rfcomm_dlc_send(&data->bt.dlc, buf);
		if (err < 0) {
			/* The DLC is going down; drop the data as RX does */
			LOG_WRN("SPP UART: TX failed (err %d), %zu bytes dropped", err, len);
			net_buf_unref(buf);
		}

		ring_buf_consume(cfg->tx_ringbuf, len);
	}

	if ((ring_buf_space_get(cfg->tx_ringbuf) > 0) && data->uart.tx_irq_ena) {
		k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
	}
}

/* Destroy callback of each instance's TX pool */
static void spp_tx_buf_destroy(struct uart_bt_spp_data *data, struct net_buf *buf)
{
	net_buf_destroy(buf);
	k_sem_give(&data->uart.tx_bufs);

	if (atomic_get(&data->bt.connected) != 0) {
		k_work_schedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_NO_WAIT);
	}
}

/* Copy into the TX FIFO; producers may run concurrently from any context */
static uint32_t spp_tx_put(struct uart_bt_spp_data *data, const uint8_t *src, uint32_t len)
{
	k_spinlock_key_t key;
	uint32_t wrote;

	key = k_spin_lock(&data->uart.tx_lock);
	wrote = ring_buf_put(spp_config(data)->tx_ringbuf, src, len);
	k_spin_unlock(&data->uart.tx_lock, key);

	return wrote;
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
	struct k_mem_slab *rx_slab = spp_config(data)->rx_slab;
	struct spp_rx_item *item;

	while ((item = k_fifo_get(&data->uart.rx_done_fifo, K_NO_WAIT)) != NULL) {
		bt_rfcomm_dlc_recv_complete(&data->bt.dlc, item->buf);
		k_mem_slab_free(rx_slab, item);
	}
}

static void spp_cb_work_handler(struct k_work *work)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(work, struct uart_bt_spp_data, uart.cb_work);

	if (data->uart.callback.cb != NULL) {
		data->uart.callback.cb(data->dev, data->uart.callback.cb_data);
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

	while (spp_tx_put(data, &c, 1) == 0) {
		/* Only tx_work on spp_work_queue makes room. Waiting cannot help
		 * in an ISR, on that queue (e.g. an IRQ callback echoing through
		 * poll_out()) or while disconnected, so drop the byte.
		 */
		if (k_is_in_isr() ||
		    (k_current_get() == k_work_queue_thread_get(&spp_work_queue)) ||
		    (atomic_get(&data->bt.connected) == 0)) {
			LOG_WRN_ONCE("TX ring buffer full");
			break;
		}
		k_sleep(K_MSEC(1));
	}

	if (atomic_get(&data->bt.connected) != 0) {
		k_work_schedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_MSEC(1));
	}
}

static int uart_bt_spp_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_bt_spp_data *data = dev->data;
	size_t wrote;

	if (len <= 0) {
		return 0;
	}

	wrote = spp_tx_put(data, tx_data, (uint32_t)len);

	if (atomic_get(&data->bt.connected) != 0) {
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
	const struct uart_bt_spp_config *cfg = dev->config;

	return (ring_buf_space_get(cfg->tx_ringbuf) > 0) && data->uart.tx_irq_ena ? 1 : 0;
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

	data->dev = dev;
	k_sem_init(&data->uart.tx_bufs, CONFIG_UART_BT_SPP_TX_BUF_COUNT,
		   CONFIG_UART_BT_SPP_TX_BUF_COUNT);

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

static void spp_auto_start(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("SPP UART: Bluetooth init failed (err %d)", err);
		return;
	}

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		LOG_ERR("SPP UART: BR/EDR set connectable failed (err %d)", err);
		return;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		LOG_ERR("SPP UART: BR/EDR set discoverable failed (err %d)", err);
	}
}

/*
 * BT service registration should be in place before the application starts
 * establishing BR/EDR connections. Register at APPLICATION init priority so
 * RFCOMM/SDP state is set up during boot and ready once the app calls bt_enable().
 */
static int uart_bt_spp_register(void)
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

		data->bt.server.channel = cfg->channel;
		data->bt.server.accept = spp_server_accept;

		err = bt_rfcomm_server_register(&data->bt.server);
		if (err < 0) {
			LOG_ERR("SPP UART: RFCOMM server register failed (err %d)", err);
			continue;
		}

		*cfg->sdp_channel = data->bt.server.channel;

		err = bt_sdp_register_service(&data->bt.sdp_record);
		if (err < 0) {
			LOG_ERR("SPP UART: SDP register failed (err %d)", err);
			/* An undiscoverable server would only hold the channel */
			(void)bt_rfcomm_server_unregister(&data->bt.server);
			continue;
		}

		LOG_DBG("SPP UART server registered (channel %u)", data->bt.server.channel);
	}

	if (IS_ENABLED(CONFIG_UART_BT_SPP_AUTO_START_BLUETOOTH)) {
		spp_auto_start();
	}

	return 0;
}

SYS_INIT(uart_bt_spp_register, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* ---- DT instantiation ---- */

#define UART_BT_SPP_INIT(n)							\
	BUILD_ASSERT(DT_INST_PROP(n, channel) <= SPP_RFCOMM_CHANNEL_MAX,		\
		     "RFCOMM channel must be 0 (dynamic) or 1-30");			\
										\
	SPP_SDP_RECORD_DEFINE(n);						\
										\
	RING_BUF_DECLARE(spp_tx_rb_##n, DT_INST_PROP(n, tx_fifo_size));		\
										\
	K_MEM_SLAB_DEFINE_STATIC(spp_rx_slab_##n, sizeof(struct spp_rx_item),	\
				 SPP_RX_ITEM_COUNT, sizeof(void *));		\
										\
	static struct uart_bt_spp_data uart_bt_spp_data_##n = {			\
		.bt = {								\
			.sdp_record = BT_SDP_RECORD(spp_sdp_attrs_##n),		\
		},								\
	};									\
										\
	static void spp_tx_buf_destroy_##n(struct net_buf *buf)			\
	{									\
		spp_tx_buf_destroy(&uart_bt_spp_data_##n, buf);			\
	}									\
										\
	NET_BUF_POOL_FIXED_DEFINE(spp_tx_pool_##n,				\
				  CONFIG_UART_BT_SPP_TX_BUF_COUNT,		\
				  BT_RFCOMM_BUF_SIZE(SPP_RFCOMM_MTU),		\
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE,		\
				  spp_tx_buf_destroy_##n);			\
										\
	static const struct uart_bt_spp_config uart_bt_spp_config_##n = {	\
		.tx_ringbuf = &spp_tx_rb_##n,					\
		.tx_pool = &spp_tx_pool_##n,					\
		.rx_slab = &spp_rx_slab_##n,					\
		.sdp_channel = &spp_sdp_channel_##n,				\
		.channel = DT_INST_PROP(n, channel),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uart_bt_spp_init, NULL,			\
			      &uart_bt_spp_data_##n,				\
			      &uart_bt_spp_config_##n,				\
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,		\
			      &uart_bt_spp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BT_SPP_INIT)
