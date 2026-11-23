/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#define DT_DRV_COMPAT zephyr_bt_spp_uart

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
LOG_MODULE_REGISTER(uart_bt_spp, CONFIG_UART_LOG_LEVEL);

K_THREAD_STACK_DEFINE(spp_work_queue_stack, CONFIG_UART_BT_SPP_WORKQUEUE_STACK_SIZE);
static struct k_work_q spp_work_queue;

#define SPP_RFCOMM_MTU CONFIG_BT_RFCOMM_L2CAP_MTU

/* Highest RFCOMM server channel number */
#define SPP_RFCOMM_CHANNEL_MAX 30

/*
 * poll_out() from a thread waits for TX ring space only while the link makes
 * progress: once no TX frame has been queued for this long, it drops bytes.
 */
#define SPP_TX_STALL_TIMEOUT_MS 100

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
		/* RX path: RFCOMM delivers whole frames as net_buf. Each frame
		 * is held until its bytes are consumed, then completed to
		 * restore one RX credit. This makes RFCOMM's per-frame credit
		 * the flow-control mechanism directly, with no intermediate
		 * byte buffer.
		 *
		 * rx_lock protects rx_queue, rx_cur, rx_done, rx_reading and
		 * rx_reset. The payload is copied outside of it.
		 */
		struct k_spinlock rx_lock;
		/* Received frames not yet read */
		sys_slist_t rx_queue;
		/* Partly read frame */
		struct net_buf *rx_cur;
		/* Read or discarded frames waiting for completion */
		sys_slist_t rx_done;
		/* A reader is copying from a frame outside of rx_lock */
		bool rx_reading;
		/* Disconnected while reading: discard the reader's frame */
		bool rx_reset;
		/* Frames taken from RFCOMM and not completed yet */
		atomic_t rx_held;
		/* Serializes the TX ring buffer producers */
		struct k_spinlock tx_lock;
		/* Free buffers in the instance's TX pool */
		atomic_t tx_bufs;
		/* Number of TX frames queued, wraps */
		atomic_t tx_frames;
		/* poll_out() timed out; drop until a TX frame is queued again */
		atomic_t tx_stalled;
		/* Disconnected after a connection: drop written data until the
		 * next one, protected by tx_lock
		 */
		bool tx_discard;
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
	struct bt_sdp_attribute *sdp_attrs;
	size_t sdp_attr_count;
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
	k_spinlock_key_t key;

	key = k_spin_lock(&data->uart.tx_lock);
	data->uart.tx_discard = false;
	k_spin_unlock(&data->uart.tx_lock, key);

	atomic_clear(&data->uart.tx_stalled);
	atomic_set(&data->bt.connected, 1);
	LOG_DBG("SPP UART connected");

	if (!ring_buf_is_empty(spp_config(data)->tx_ringbuf)) {
		(void)k_work_reschedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_NO_WAIT);
	}
}

/*
 * Complete the frames in rx_done, which restores their RX credits. Must run
 * in a thread, see spp_rx_complete_work_handler().
 */
static void spp_rx_complete_done(struct uart_bt_spp_data *data)
{
	struct net_buf *buf;
	sys_slist_t done;
	sys_snode_t *node;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->uart.rx_lock);
	done = data->uart.rx_done;
	sys_slist_init(&data->uart.rx_done);
	k_spin_unlock(&data->uart.rx_lock, key);

	while ((node = sys_slist_get(&done)) != NULL) {
		buf = CONTAINER_OF(node, struct net_buf, node);
		/* After a disconnect this only releases the buffer */
		(void)bt_rfcomm_dlc_recv_complete(&data->bt.dlc, buf);
		/* Decrement last: accept() waits for rx_held to reach 0 */
		atomic_dec(&data->uart.rx_held);
	}
}

static void spp_dlc_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);
	struct k_work_sync sync;
	k_spinlock_key_t key;

	atomic_set(&data->bt.connected, 0);

	/* Drop unsent and further written data until the next connection, so
	 * that none of it reaches the next peer, which may be another device.
	 * tx_work is the only reader of the ring buffer and does not block;
	 * once cancelled it is not scheduled again while disconnected.
	 */
	key = k_spin_lock(&data->uart.tx_lock);
	data->uart.tx_discard = true;
	k_spin_unlock(&data->uart.tx_lock, key);

	(void)k_work_cancel_delayable_sync(&data->uart.tx_work, &sync);

	key = k_spin_lock(&data->uart.tx_lock);
	ring_buf_reset(spp_config(data)->tx_ringbuf);
	k_spin_unlock(&data->uart.tx_lock, key);

	if (data->uart.tx_irq_ena) {
		(void)k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
	}

	/* Discard unread payload and release the held frames. Everything is
	 * moved to rx_done under rx_lock, so no reader can pick up a frame of
	 * this connection afterwards. A frame a reader is copying from right
	 * now is moved to rx_done by that reader when it is done.
	 */
	key = k_spin_lock(&data->uart.rx_lock);
	if (data->uart.rx_cur != NULL) {
		sys_slist_append(&data->uart.rx_done, &data->uart.rx_cur->node);
		data->uart.rx_cur = NULL;
	}
	sys_slist_merge_slist(&data->uart.rx_done, &data->uart.rx_queue);
	data->uart.rx_reset = data->uart.rx_reading;
	k_spin_unlock(&data->uart.rx_lock, key);

	spp_rx_complete_done(data);

	/* RFCOMM no longer references the DLC; it can be handed out again
	 * once rx_held drops to 0, see spp_server_accept().
	 */
	atomic_clear(&data->bt.busy);

	LOG_DBG("SPP UART disconnected");
}

static int spp_dlc_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(dlc, struct uart_bt_spp_data, bt.dlc);
	k_spinlock_key_t key;

	/* Take ownership of the frame and defer completion until its bytes have
	 * been read out through the UART API. Returning -EINPROGRESS withholds
	 * one RX credit for the lifetime of this buffer, so the remote peer
	 * stops sending once its outstanding credits (bounded by
	 * rx_credit_limit) are exhausted. This is the RFCOMM-native backpressure.
	 */
	atomic_inc(&data->uart.rx_held);

	key = k_spin_lock(&data->uart.rx_lock);
	sys_slist_append(&data->uart.rx_queue, &buf->node);
	k_spin_unlock(&data->uart.rx_lock, key);

	(void)k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);

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

	/* Frames of the previous connection that a reader or the work queue
	 * still holds would be completed on the new connection otherwise.
	 */
	if (atomic_get(&data->uart.rx_held) != 0) {
		atomic_clear(&data->bt.busy);
		return -EBUSY;
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

/* Claim a free TX pool buffer, so that allocating it cannot block */
static bool spp_tx_buf_claim(struct uart_bt_spp_data *data)
{
	atomic_val_t avail;

	do {
		avail = atomic_get(&data->uart.tx_bufs);
		if (avail == 0) {
			return false;
		}
	} while (!atomic_cas(&data->uart.tx_bufs, avail, avail - 1));

	return true;
}

/*
 * poll_out() and fifo_fill() may run in an ISR, so they only copy into the TX
 * ring buffer. tx_work sends its content in frames of up to the MTU while the
 * DLC is connected.
 *
 * tx_work never waits for a TX buffer, as that would also block RX completion
 * on the same queue. It stops when none is free, and spp_tx_buf_destroy()
 * resumes it when one is returned.
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

		if (!spp_tx_buf_claim(data)) {
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
		atomic_inc(&data->uart.tx_frames);
		atomic_clear(&data->uart.tx_stalled);
	}

	if ((ring_buf_space_get(cfg->tx_ringbuf) > 0) && data->uart.tx_irq_ena) {
		(void)k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
	}
}

/* Destroy callback of each instance's TX pool */
static void spp_tx_buf_destroy(struct uart_bt_spp_data *data, struct net_buf *buf)
{
	net_buf_destroy(buf);
	atomic_inc(&data->uart.tx_bufs);

	if (atomic_get(&data->bt.connected) != 0) {
		(void)k_work_schedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_NO_WAIT);
	}
}

/*
 * Copy into the TX FIFO; producers may run concurrently from any context.
 * After a disconnect the data is accepted and dropped, so that writers do not
 * wait for a peer, nor keep data in their own buffers for the next one.
 */
static uint32_t spp_tx_put(struct uart_bt_spp_data *data, const uint8_t *src, uint32_t len)
{
	k_spinlock_key_t key;
	uint32_t wrote;

	key = k_spin_lock(&data->uart.tx_lock);
	if (data->uart.tx_discard) {
		wrote = len;
	} else {
		wrote = ring_buf_put(spp_config(data)->tx_ringbuf, src, len);
	}
	k_spin_unlock(&data->uart.tx_lock, key);

	return wrote;
}

/* Whether written data is dropped, see spp_tx_put() */
static bool spp_tx_discarding(struct uart_bt_spp_data *data)
{
	k_spinlock_key_t key;
	bool discard;

	key = k_spin_lock(&data->uart.tx_lock);
	discard = data->uart.tx_discard;
	k_spin_unlock(&data->uart.tx_lock, key);

	return discard;
}

/* ---- UART API ---- */

/*
 * Copy up to max_len received bytes into dst. This may run in an ISR, so fully
 * read frames are only moved to rx_done and completed by rx_complete_work.
 * The copy runs without rx_lock; rx_reading keeps a second reader out.
 */
static size_t spp_rx_drain(struct uart_bt_spp_data *data, uint8_t *dst, size_t max_len)
{
	size_t copied = 0;
	bool completed = false;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->uart.rx_lock);

	if (data->uart.rx_reading) {
		k_spin_unlock(&data->uart.rx_lock, key);
		return 0;
	}
	data->uart.rx_reading = true;

	while (copied < max_len) {
		struct net_buf *buf = data->uart.rx_cur;
		size_t take;

		if (buf == NULL) {
			sys_snode_t *node = sys_slist_get(&data->uart.rx_queue);

			if (node == NULL) {
				break;
			}
			buf = CONTAINER_OF(node, struct net_buf, node);
		}
		data->uart.rx_cur = NULL;

		k_spin_unlock(&data->uart.rx_lock, key);

		take = MIN(max_len - copied, (size_t)buf->len);
		memcpy(dst + copied, net_buf_pull_mem(buf, take), take);
		copied += take;

		key = k_spin_lock(&data->uart.rx_lock);

		if (data->uart.rx_reset) {
			/* Disconnected meanwhile; the rest of the frame is stale */
			data->uart.rx_reset = false;
			sys_slist_append(&data->uart.rx_done, &buf->node);
			completed = true;
			break;
		}

		if (buf->len == 0U) {
			sys_slist_append(&data->uart.rx_done, &buf->node);
			completed = true;
		} else {
			data->uart.rx_cur = buf;
		}
	}

	data->uart.rx_reading = false;
	k_spin_unlock(&data->uart.rx_lock, key);

	if (completed) {
		(void)k_work_submit_to_queue(&spp_work_queue, &data->uart.rx_complete_work);
	}

	return copied;
}

/* Whether any received bytes are available to read. */
static bool spp_rx_has_data(struct uart_bt_spp_data *data)
{
	bool has_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->uart.rx_lock);
	has_data = data->uart.rx_cur != NULL || !sys_slist_is_empty(&data->uart.rx_queue);
	k_spin_unlock(&data->uart.rx_lock, key);

	return has_data;
}

/*
 * Completing a frame calls bt_rfcomm_dlc_recv_complete() which restores an
 * RX credit and may reach bt_l2cap_send(); it must run from a thread, never
 * an ISR. The UART read path can run from any context, so read frames are
 * queued and completed here on the work queue.
 */
static void spp_rx_complete_work_handler(struct k_work *work)
{
	struct uart_bt_spp_data *data = CONTAINER_OF(work, struct uart_bt_spp_data,
						     uart.rx_complete_work);

	spp_rx_complete_done(data);
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
	int err;

	err = spp_rx_drain(data, c, 1) == 1 ? 0 : -1;

	return err;
}

/*
 * Whether poll_out() should drop a byte rather than wait for TX ring space.
 * Only tx_work on spp_work_queue makes room, and only while the peer grants
 * credits. Waiting is not possible when the caller cannot sleep (ISR, IRQs
 * locked or a spinlock held, e.g. printk() with CONFIG_PRINTK_SYNC), cannot
 * help on that queue (e.g. an IRQ callback echoing through poll_out()) or
 * while disconnected.
 */
static bool spp_poll_out_must_drop(struct uart_bt_spp_data *data)
{
	return !k_can_yield() ||
	       (k_current_get() == k_work_queue_thread_get(&spp_work_queue)) ||
	       (atomic_get(&data->bt.connected) == 0) ||
	       (atomic_get(&data->uart.tx_stalled) != 0);
}

static void uart_bt_spp_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_bt_spp_data *data = dev->data;
	atomic_val_t frames = atomic_get(&data->uart.tx_frames);
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(SPP_TX_STALL_TIMEOUT_MS));

	while (spp_tx_put(data, &c, 1) == 0) {
		atomic_val_t now_frames = atomic_get(&data->uart.tx_frames);

		if (spp_poll_out_must_drop(data)) {
			/* Logging can reach printk(), which may be holding
			 * its lock while it calls this function
			 */
			if (k_can_yield()) {
				LOG_WRN_ONCE("TX ring buffer full");
			}
			break;
		}

		/* A caller on the Bluetooth host's own thread (e.g. printk() from
		 * a Bluetooth callback with this UART as console) blocks the
		 * credits it is waiting for. Bound the wait to the time without
		 * TX progress, then drop until a frame is queued again.
		 */
		if (now_frames != frames) {
			frames = now_frames;
			end = sys_timepoint_calc(K_MSEC(SPP_TX_STALL_TIMEOUT_MS));
		}
		if (sys_timepoint_expired(end)) {
			atomic_set(&data->uart.tx_stalled, 1);
			LOG_WRN_ONCE("TX stalled, dropping output");
			break;
		}

		k_sleep(K_MSEC(1));
	}

	if (atomic_get(&data->bt.connected) != 0) {
		(void)k_work_schedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_MSEC(1));
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
		(void)k_work_reschedule_for_queue(&spp_work_queue, &data->uart.tx_work, K_NO_WAIT);
	} else if (data->uart.tx_irq_ena && spp_tx_discarding(data)) {
		/* No frame is sent, so call back from here, so that the writer
		 * can hand over the rest, find nothing left and disable the TX
		 * interrupt
		 */
		(void)k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
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
		(void)k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
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
	(void)k_work_submit_to_queue(&spp_work_queue, &data->uart.cb_work);
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

BUILD_ASSERT((CONFIG_UART_BT_SPP_WORKQUEUE_PRIORITY < 0) &&
		     (CONFIG_UART_BT_SPP_WORKQUEUE_PRIORITY >= -CONFIG_NUM_COOP_PRIORITIES),
	     "CONFIG_UART_BT_SPP_WORKQUEUE_PRIORITY must be a cooperative priority");

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
	const struct uart_bt_spp_config *cfg = dev->config;
	struct uart_bt_spp_data *data = dev->data;
	int err;

	data->dev = dev;
	atomic_set(&data->uart.tx_bufs, CONFIG_UART_BT_SPP_TX_BUF_COUNT);

	sys_slist_init(&data->uart.rx_queue);
	sys_slist_init(&data->uart.rx_done);

	k_work_init_delayable(&data->uart.tx_work, spp_tx_work_handler);
	k_work_init(&data->uart.cb_work, spp_cb_work_handler);
	k_work_init(&data->uart.rx_complete_work, spp_rx_complete_work_handler);

	data->bt.dlc_ops.connected = spp_dlc_connected;
	data->bt.dlc_ops.disconnected = spp_dlc_disconnected;
	data->bt.dlc_ops.recv = spp_dlc_recv;

	/*
	 * RFCOMM server and SDP record registration only add entries to
	 * statically initialized lists, so it does not need Bluetooth to be
	 * enabled and is in place before the application calls bt_enable().
	 */
	data->bt.server.channel = cfg->channel;
	data->bt.server.accept = spp_server_accept;

	err = bt_rfcomm_server_register(&data->bt.server);
	if (err != 0) {
		LOG_ERR("SPP UART: RFCOMM server register failed (err %d)", err);
		return err;
	}

	*cfg->sdp_channel = data->bt.server.channel;
	data->bt.sdp_record.attrs = cfg->sdp_attrs;
	data->bt.sdp_record.attr_count = cfg->sdp_attr_count;

	err = bt_sdp_register_service(&data->bt.sdp_record);
	if (err != 0) {
		LOG_ERR("SPP UART: SDP register failed (err %d)", err);
		/* An undiscoverable server would only hold the channel */
		(void)bt_rfcomm_server_unregister(&data->bt.server);
		return err;
	}

	LOG_DBG("SPP UART server registered (channel %u)", data->bt.server.channel);

	return 0;
}

#if defined(CONFIG_UART_BT_SPP_AUTO_START_BLUETOOTH)
static int uart_bt_spp_auto_start(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("SPP UART: Bluetooth init failed (err %d)", err);
		return 0;
	}

	/* Without the identity from the settings the stack does not get ready */
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = settings_load_subtree("bt");
		if (err != 0) {
			LOG_ERR("SPP UART: settings load failed (err %d)", err);
			return 0;
		}
	}

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		LOG_ERR("SPP UART: BR/EDR set connectable failed (err %d)", err);
		return 0;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		LOG_ERR("SPP UART: BR/EDR set discoverable failed (err %d)", err);
	}

	return 0;
}

/* Enable Bluetooth once all devices, including the HCI driver, are initialized */
SYS_INIT(uart_bt_spp_auto_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_UART_BT_SPP_AUTO_START_BLUETOOTH */

/* ---- DT instantiation ---- */

#define UART_BT_SPP_INIT(n)							\
	BUILD_ASSERT(DT_INST_PROP(n, channel) <= SPP_RFCOMM_CHANNEL_MAX,		\
		     "RFCOMM channel must be 0 (dynamic) or 1-30");			\
										\
	SPP_SDP_RECORD_DEFINE(n);						\
										\
	RING_BUF_DECLARE(spp_tx_rb_##n, DT_INST_PROP(n, tx_fifo_size));		\
										\
	static struct uart_bt_spp_data uart_bt_spp_data_##n;			\
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
		.sdp_attrs = spp_sdp_attrs_##n,					\
		.sdp_attr_count = ARRAY_SIZE(spp_sdp_attrs_##n),		\
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
