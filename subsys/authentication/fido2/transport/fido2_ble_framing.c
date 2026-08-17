/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/authentication/fido2/fido2_ble_internal.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define FIDO2_BLE_ERR_INVALID_CMD 0x01U
#define FIDO2_BLE_ERR_INVALID_LEN 0x03U
#define FIDO2_BLE_ERR_INVALID_SEQ 0x04U
#define FIDO2_BLE_ERR_REQ_TIMEOUT 0x05U
#define FIDO2_BLE_ERR_BUSY        0x06U
#define FIDO2_BLE_ERR_OTHER       0x7FU

#define FIDO2_BLE_RX_TIMEOUT_MS          1500U
#define FIDO2_BLE_TX_RETRY_DELAY_MS      5U
#define FIDO2_BLE_TX_SHUTDOWN_TIMEOUT_MS 1000U

BUILD_ASSERT(FIDO2_BLE_MAX_MESSAGE_SIZE <= UINT16_MAX,
	     "FIDO BLE messages use a 16-bit length field");

/**
 * @brief Queued fragment received from the FIDO BLE Control Point.
 */
struct fido2_ble_rx_fragment {
	/** Referenced connection on which the fragment was received. */
	struct bt_conn *conn;
	/** Length of @p data in bytes. */
	uint16_t len;
	/** Raw initial or continuation fragment. */
	uint8_t data[CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH];
};

/**
 * @brief Complete FIDO BLE command waiting to be fragmented and transmitted.
 */
struct fido2_ble_tx_frame {
	/** Reserved FIFO linkage required by Zephyr's FIFO implementation. */
	void *fifo_reserved;
	/** Referenced connection on which the frame is transmitted. */
	struct bt_conn *conn;
	/** Length of @p data in bytes. */
	size_t len;
	/** FIDO BLE command identifier. */
	enum fido2_ble_command command;
	/** Set when transmission must stop after any in-flight notification completes. */
	bool cancelled;
	/** Complete command payload. */
	uint8_t data[FIDO2_BLE_MAX_MESSAGE_SIZE];
};

/**
 * @brief Runtime state for FIDO BLE framing and reassembly.
 */
struct fido2_ble_framing_context {
	/** Callbacks used to dispatch complete requests and control events. */
	struct fido2_ble_framing_callbacks callbacks;
	/** Set while the framing layer accepts RX and TX work. */
	atomic_t initialized;
	/** Ensures the dedicated RX work queue is started only once. */
	atomic_t work_queue_started;
	/** Set while shutdown is draining asynchronous work. */
	atomic_t stopping;
	/** Receive-side state for reassembling one FIDO BLE command. */
	struct {
		/** Referenced connection that owns the current reassembly. */
		struct bt_conn *conn;
		/** true while a fragmented command is being reassembled. */
		bool active;
		/** Command identifier from the initial fragment. */
		uint8_t command;
		/** Total payload length declared by the initial fragment. */
		uint16_t expected_length;
		/** Number of payload bytes received so far. */
		uint16_t received_length;
		/** Sequence number required for the next continuation fragment. */
		uint8_t expected_sequence;
		/** Buffer containing the reassembled command payload. */
		uint8_t data[FIDO2_BLE_MAX_MESSAGE_SIZE];
	} rx;
	/** Transmit-side state for fragmenting one queued FIDO BLE command. */
	struct {
		/** Frame currently being transmitted. */
		struct fido2_ble_tx_frame *active;
		/** true after the command's initial fragment has been submitted. */
		bool initial_sent;
		/** true while a GATT notification completion callback is pending. */
		bool in_flight;
		/** Number of payload bytes already submitted. */
		size_t offset;
		/** Sequence number to use for the next continuation fragment. */
		uint8_t sequence;
		/** Scratch buffer for the next GATT notification fragment. */
		uint8_t fragment[CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH];
	} tx;
};

static struct fido2_ble_framing_context framing_ctx;

/* GATT write callbacks only enqueue fragments; reassembly runs on the dedicated RX queue. */
K_MSGQ_DEFINE(fido2_ble_rx_msgq, sizeof(struct fido2_ble_rx_fragment),
	      CONFIG_FIDO2_BLE_RX_QUEUE_DEPTH, 4);
K_MEM_SLAB_DEFINE(fido2_ble_tx_slab, sizeof(struct fido2_ble_tx_frame),
		  CONFIG_FIDO2_BLE_TX_FRAME_COUNT, __alignof__(struct fido2_ble_tx_frame));
K_FIFO_DEFINE(fido2_ble_tx_fifo);
K_SEM_DEFINE(fido2_ble_tx_complete, 0, 1);

static struct k_work_q rx_work_q;
static K_THREAD_STACK_DEFINE(rx_work_q_stack, CONFIG_FIDO2_BLE_RX_WORKQ_STACK_SIZE);
static struct k_work rx_work;
static struct k_work_delayable rx_timeout_work;
static struct k_work_delayable tx_work;
static K_MUTEX_DEFINE(rx_mutex);
static K_MUTEX_DEFINE(tx_mutex);

static void tx_schedule(k_timeout_t delay)
{
	if (atomic_get(&framing_ctx.initialized) && !atomic_get(&framing_ctx.stopping)) {
		(void)k_work_reschedule(&tx_work, delay);
	}
}

static void tx_frame_release(struct fido2_ble_tx_frame *frame)
{
	if (frame == NULL) {
		return;
	}

	bt_conn_unref(frame->conn);
	k_mem_slab_free(&fido2_ble_tx_slab, frame);
}

static struct fido2_ble_tx_frame *tx_active_remove_locked(void)
{
	struct fido2_ble_tx_frame *frame = framing_ctx.tx.active;

	framing_ctx.tx.active = NULL;
	framing_ctx.tx.initial_sent = false;
	framing_ctx.tx.in_flight = false;
	framing_ctx.tx.offset = 0U;
	framing_ctx.tx.sequence = 0U;

	return frame;
}

static bool tx_error_is_transient(int err)
{
	return (err == -ENOMEM) || (err == -ENOBUFS) || (err == -EAGAIN);
}

static void tx_notify_complete(struct bt_conn *conn, void *user_data)
{
	struct fido2_ble_tx_frame *completed = NULL;
	bool send_next = false;

	ARG_UNUSED(conn);
	ARG_UNUSED(user_data);

	k_mutex_lock(&tx_mutex, K_FOREVER);
	if ((framing_ctx.tx.active != NULL) && framing_ctx.tx.in_flight) {
		framing_ctx.tx.in_flight = false;
		if (atomic_get(&framing_ctx.stopping) || framing_ctx.tx.active->cancelled ||
		    (framing_ctx.tx.offset >= framing_ctx.tx.active->len)) {
			completed = tx_active_remove_locked();
		} else {
			send_next = true;
		}
	}
	k_mutex_unlock(&tx_mutex);

	k_sem_give(&fido2_ble_tx_complete);
	tx_frame_release(completed);

	if (send_next || (completed != NULL)) {
		tx_schedule(K_NO_WAIT);
	}
}

static void tx_work_handler(struct k_work *work)
{
	struct fido2_ble_tx_frame *frame;
	struct fido2_ble_tx_frame *failed = NULL;
	size_t old_offset;
	size_t next_offset;
	size_t chunk_len;
	uint16_t fragment_len;
	uint16_t max_fragment_len;
	uint16_t mtu;
	uint8_t old_sequence;
	uint8_t next_sequence;
	bool old_initial_sent;
	bool retry = false;
	int err;

	ARG_UNUSED(work);

	k_mutex_lock(&tx_mutex, K_FOREVER);
	if (atomic_get(&framing_ctx.stopping) || framing_ctx.tx.in_flight) {
		k_mutex_unlock(&tx_mutex);
		return;
	}

	/* Serialize one complete command at a time; each GATT notification is asynchronous. */
	if (framing_ctx.tx.active == NULL) {
		framing_ctx.tx.active = k_fifo_get(&fido2_ble_tx_fifo, K_NO_WAIT);
		framing_ctx.tx.initial_sent = false;
		framing_ctx.tx.offset = 0U;
		framing_ctx.tx.sequence = 0U;
	}

	frame = framing_ctx.tx.active;
	if (frame == NULL) {
		k_mutex_unlock(&tx_mutex);
		return;
	}

	if (frame->cancelled || !fido2_ble_gatt_conn_is_ready(frame->conn)) {
		failed = tx_active_remove_locked();
		k_mutex_unlock(&tx_mutex);
		tx_frame_release(failed);
		tx_schedule(K_NO_WAIT);
		return;
	}

	mtu = bt_gatt_get_mtu(frame->conn);
	if (mtu <= 3U) {
		failed = tx_active_remove_locked();
		k_mutex_unlock(&tx_mutex);
		tx_frame_release(failed);
		tx_schedule(K_NO_WAIT);
		return;
	}

	max_fragment_len =
		MIN((uint16_t)CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH, (uint16_t)(mtu - 3U));
	if (max_fragment_len < 3U) {
		failed = tx_active_remove_locked();
		k_mutex_unlock(&tx_mutex);
		tx_frame_release(failed);
		tx_schedule(K_NO_WAIT);
		return;
	}

	old_offset = framing_ctx.tx.offset;
	old_sequence = framing_ctx.tx.sequence;
	old_initial_sent = framing_ctx.tx.initial_sent;
	next_sequence = old_sequence;

	if (!old_initial_sent) {
		/* Initial fragments carry command, 16-bit payload length, then payload bytes. */
		framing_ctx.tx.fragment[0] = (uint8_t)frame->command;
		sys_put_be16((uint16_t)frame->len, &framing_ctx.tx.fragment[1]);
		chunk_len = MIN(frame->len, (size_t)(max_fragment_len - 3U));
		if (chunk_len != 0U) {
			memcpy(&framing_ctx.tx.fragment[3], frame->data, chunk_len);
		}
		fragment_len = (uint16_t)(3U + chunk_len);
		next_offset = chunk_len;
		framing_ctx.tx.initial_sent = true;
	} else {
		/* Continuation fragments carry a 7-bit sequence number followed by payload. */
		chunk_len = MIN(frame->len - old_offset, (size_t)(max_fragment_len - 1U));
		framing_ctx.tx.fragment[0] = old_sequence;
		memcpy(&framing_ctx.tx.fragment[1], &frame->data[old_offset], chunk_len);
		fragment_len = (uint16_t)(1U + chunk_len);
		next_offset = old_offset + chunk_len;
		next_sequence = (old_sequence + 1U) & 0x7FU;
	}

	framing_ctx.tx.offset = next_offset;
	framing_ctx.tx.sequence = next_sequence;
	framing_ctx.tx.in_flight = true;
	k_mutex_unlock(&tx_mutex);

	err = fido2_ble_gatt_notify(frame->conn, framing_ctx.tx.fragment, fragment_len,
				    tx_notify_complete, NULL);
	if (err == 0) {
		return;
	}

	/* Submission failed before completion ownership transferred; roll back TX progress. */
	k_mutex_lock(&tx_mutex, K_FOREVER);
	if ((framing_ctx.tx.active == frame) && framing_ctx.tx.in_flight) {
		framing_ctx.tx.in_flight = false;
		framing_ctx.tx.offset = old_offset;
		framing_ctx.tx.sequence = old_sequence;
		framing_ctx.tx.initial_sent = old_initial_sent;

		if (tx_error_is_transient(err) && !atomic_get(&framing_ctx.stopping) &&
		    !frame->cancelled) {
			retry = true;
		} else {
			failed = tx_active_remove_locked();
		}
	}
	k_mutex_unlock(&tx_mutex);

	if (!tx_error_is_transient(err) && (err != -ENOTCONN) && (err != -EACCES)) {
		LOG_WRN("FIDO BLE notification failed: %d", err);
	}

	tx_frame_release(failed);
	if (retry) {
		tx_schedule(K_MSEC(FIDO2_BLE_TX_RETRY_DELAY_MS));
	} else if (failed != NULL) {
		tx_schedule(K_NO_WAIT);
	}
}

int fido2_ble_framing_send(struct bt_conn *conn, enum fido2_ble_command command,
			   const uint8_t *data, size_t len)
{
	struct fido2_ble_tx_frame *frame;
	int err;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	if ((data == NULL) && (len != 0U)) {
		return -EINVAL;
	}

	if ((len > FIDO2_BLE_MAX_MESSAGE_SIZE) || (len > UINT16_MAX)) {
		return -EMSGSIZE;
	}

	if (!atomic_get(&framing_ctx.initialized) || atomic_get(&framing_ctx.stopping)) {
		return -ESHUTDOWN;
	}

	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return -ENOTCONN;
	}

	err = k_mem_slab_alloc(&fido2_ble_tx_slab, (void **)&frame, K_NO_WAIT);
	if (err != 0) {
		return -ENOMEM;
	}

	/* Copy the complete command so callers may release their payload after this returns. */
	frame->conn = bt_conn_ref(conn);
	frame->command = command;
	frame->len = len;
	frame->cancelled = false;
	if (len != 0U) {
		memcpy(frame->data, data, len);
	}

	k_fifo_put(&fido2_ble_tx_fifo, frame);
	tx_schedule(K_NO_WAIT);

	return 0;
}

static void send_error(struct bt_conn *conn, uint8_t error_code)
{
	int err;

	err = fido2_ble_framing_send(conn, FIDO2_BLE_CMD_ERROR, &error_code, sizeof(error_code));
	if (err != 0) {
		LOG_WRN("Failed to queue FIDO BLE error 0x%02x: %d", error_code, err);
	}
}

static struct bt_conn *rx_clear_locked(void)
{
	struct bt_conn *conn = framing_ctx.rx.conn;

	memset(&framing_ctx.rx, 0, sizeof(framing_ctx.rx));

	return conn;
}

static void rx_reset_locked(void)
{
	struct bt_conn *conn;

	(void)k_work_cancel_delayable(&rx_timeout_work);
	conn = rx_clear_locked();
	if (conn != NULL) {
		bt_conn_unref(conn);
	}
}

static void dispatch_complete_locked(struct bt_conn *conn)
{
	struct bt_conn *rx_conn;
	int err;

	(void)k_work_cancel_delayable(&rx_timeout_work);

	/* Only complete data-bearing commands reach this dispatcher. */
	switch (framing_ctx.rx.command) {
	case FIDO2_BLE_CMD_PING:
		/* Echo the complete payload back to the client. */
		err = fido2_ble_framing_send(conn, FIDO2_BLE_CMD_PING, framing_ctx.rx.data,
					     framing_ctx.rx.expected_length);
		if (err != 0) {
			LOG_WRN("Failed to queue FIDO BLE PING response: %d", err);
		}
		break;
	case FIDO2_BLE_CMD_MSG:
		/* Forward the complete CTAP message to the transport/core layer. */
		err = framing_ctx.callbacks.message_received(conn, framing_ctx.rx.data,
							     framing_ctx.rx.expected_length);
		if (err == -EBUSY) {
			send_error(conn, FIDO2_BLE_ERR_BUSY);
		} else if (err != 0) {
			send_error(conn, FIDO2_BLE_ERR_OTHER);
		}
		break;
	default:
		send_error(conn, FIDO2_BLE_ERR_INVALID_CMD);
		break;
	}

	rx_conn = rx_clear_locked();
	if (rx_conn != NULL) {
		bt_conn_unref(rx_conn);
	}
}

static void process_initial_locked(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	uint16_t expected_len;
	uint16_t payload_len;
	uint8_t command;

	if (len < 3U) {
		send_error(conn, FIDO2_BLE_ERR_INVALID_LEN);
		return;
	}

	command = data[0];
	expected_len = sys_get_be16(&data[1]);
	payload_len = len - 3U;

	/*
	 * CANCEL is a zero-length control command. Handle it immediately so it can abort
	 * reassembly or the active CTAP operation; the command itself has no response.
	 */
	if (command == FIDO2_BLE_CMD_CANCEL) {
		if ((expected_len != 0U) || (payload_len != 0U)) {
			send_error(conn, FIDO2_BLE_ERR_INVALID_LEN);
			return;
		}

		rx_reset_locked();
		framing_ctx.callbacks.cancel_received();
		return;
	}

	if ((command != FIDO2_BLE_CMD_PING) && (command != FIDO2_BLE_CMD_MSG)) {
		send_error(conn, FIDO2_BLE_ERR_INVALID_CMD);
		return;
	}

	/* A new PING/MSG must not replace an incomplete frame or an active CTAP transaction. */
	if (framing_ctx.rx.active || framing_ctx.callbacks.transaction_active()) {
		send_error(conn, FIDO2_BLE_ERR_BUSY);
		return;
	}

	if ((expected_len > sizeof(framing_ctx.rx.data)) || (payload_len > expected_len) ||
	    ((command == FIDO2_BLE_CMD_MSG) && (expected_len == 0U))) {
		send_error(conn, FIDO2_BLE_ERR_INVALID_LEN);
		return;
	}

	/* Retain the connection until the message completes, times out, or is cancelled. */
	framing_ctx.rx.conn = bt_conn_ref(conn);
	framing_ctx.rx.active = true;
	framing_ctx.rx.command = command;
	framing_ctx.rx.expected_length = expected_len;
	framing_ctx.rx.received_length = payload_len;
	framing_ctx.rx.expected_sequence = 0U;
	if (payload_len != 0U) {
		memcpy(framing_ctx.rx.data, &data[3], payload_len);
	}

	if (framing_ctx.rx.received_length == framing_ctx.rx.expected_length) {
		dispatch_complete_locked(conn);
		return;
	}

	(void)k_work_reschedule_for_queue(&rx_work_q, &rx_timeout_work,
					  K_MSEC(FIDO2_BLE_RX_TIMEOUT_MS));
}

static void process_continuation_locked(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	uint16_t payload_len;
	uint16_t remaining_len;
	uint8_t sequence;

	if (!framing_ctx.rx.active || (framing_ctx.rx.conn != conn)) {
		send_error(conn, FIDO2_BLE_ERR_INVALID_SEQ);
		return;
	}

	/* Continuation fragments must arrive in strictly increasing 7-bit sequence order. */
	sequence = data[0];
	if (sequence != framing_ctx.rx.expected_sequence) {
		LOG_WRN("Invalid FIDO BLE sequence: got %u, expected %u", sequence,
			framing_ctx.rx.expected_sequence);
		rx_reset_locked();
		send_error(conn, FIDO2_BLE_ERR_INVALID_SEQ);
		return;
	}

	payload_len = len - 1U;
	remaining_len = framing_ctx.rx.expected_length - framing_ctx.rx.received_length;
	if (payload_len > remaining_len) {
		rx_reset_locked();
		send_error(conn, FIDO2_BLE_ERR_INVALID_LEN);
		return;
	}

	if (payload_len != 0U) {
		memcpy(&framing_ctx.rx.data[framing_ctx.rx.received_length], &data[1], payload_len);
	}

	framing_ctx.rx.received_length += payload_len;
	framing_ctx.rx.expected_sequence = (framing_ctx.rx.expected_sequence + 1U) & 0x7FU;

	if (framing_ctx.rx.received_length == framing_ctx.rx.expected_length) {
		dispatch_complete_locked(conn);
		return;
	}

	(void)k_work_reschedule_for_queue(&rx_work_q, &rx_timeout_work,
					  K_MSEC(FIDO2_BLE_RX_TIMEOUT_MS));
}

static void process_fragment(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return;
	}

	LOG_HEXDUMP_DBG(data, len, "FIDO BLE RX fragment");

	k_mutex_lock(&rx_mutex, K_FOREVER);
	/* Bit 7 distinguishes command-bearing initial fragments from continuation fragments. */
	if ((data[0] & BIT(7)) != 0U) {
		process_initial_locked(conn, data, len);
	} else {
		process_continuation_locked(conn, data, len);
	}
	k_mutex_unlock(&rx_mutex);
}

static void rx_work_handler(struct k_work *work)
{
	struct fido2_ble_rx_fragment fragment;

	ARG_UNUSED(work);

	while (k_msgq_get(&fido2_ble_rx_msgq, &fragment, K_NO_WAIT) == 0) {
		process_fragment(fragment.conn, fragment.data, fragment.len);
		bt_conn_unref(fragment.conn);
	}
}

static void rx_timeout_handler(struct k_work *work)
{
	struct bt_conn *conn;

	ARG_UNUSED(work);

	k_mutex_lock(&rx_mutex, K_FOREVER);
	if (!framing_ctx.rx.active) {
		k_mutex_unlock(&rx_mutex);
		return;
	}

	conn = rx_clear_locked();
	k_mutex_unlock(&rx_mutex);

	LOG_WRN("FIDO BLE reassembly timed out");
	if (conn != NULL) {
		send_error(conn, FIDO2_BLE_ERR_REQ_TIMEOUT);
		bt_conn_unref(conn);
	}
}

int fido2_ble_framing_submit_fragment(struct bt_conn *conn, const void *data, uint16_t len)
{
	struct fido2_ble_rx_fragment fragment;
	int err;

	if ((conn == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	if ((len == 0U) || (len > CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH)) {
		return -EMSGSIZE;
	}

	if (!atomic_get(&framing_ctx.initialized) || atomic_get(&framing_ctx.stopping)) {
		return -ESHUTDOWN;
	}

	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return -ENOTCONN;
	}

	/* Hold a connection reference while the fragment waits in the asynchronous RX queue. */
	fragment.conn = bt_conn_ref(conn);
	fragment.len = len;
	memcpy(fragment.data, data, len);

	err = k_msgq_put(&fido2_ble_rx_msgq, &fragment, K_NO_WAIT);
	if (err != 0) {
		bt_conn_unref(fragment.conn);
		return -ENOMEM;
	}

	err = k_work_submit_to_queue(&rx_work_q, &rx_work);
	if (err < 0) {
		LOG_ERR("Failed to submit FIDO BLE RX work: %d", err);
		return err;
	}

	return 0;
}

void fido2_ble_framing_connection_closed(struct bt_conn *conn)
{
	struct fido2_ble_rx_fragment fragment;
	struct fido2_ble_tx_frame *frame;
	struct bt_conn *rx_conn = NULL;

	/* Drop partial RX state and every queued TX item that can no longer be delivered. */
	(void)k_work_cancel_delayable(&rx_timeout_work);

	k_mutex_lock(&rx_mutex, K_FOREVER);
	if ((framing_ctx.rx.conn == NULL) || (framing_ctx.rx.conn == conn)) {
		rx_conn = rx_clear_locked();
	}
	k_mutex_unlock(&rx_mutex);
	if (rx_conn != NULL) {
		bt_conn_unref(rx_conn);
	}

	while (k_msgq_get(&fido2_ble_rx_msgq, &fragment, K_NO_WAIT) == 0) {
		bt_conn_unref(fragment.conn);
	}

	k_mutex_lock(&tx_mutex, K_FOREVER);
	if ((framing_ctx.tx.active != NULL) && (framing_ctx.tx.active->conn == conn)) {
		framing_ctx.tx.active->cancelled = true;
		if (!framing_ctx.tx.in_flight) {
			frame = tx_active_remove_locked();
		} else {
			frame = NULL;
		}
	} else {
		frame = NULL;
	}
	k_mutex_unlock(&tx_mutex);
	tx_frame_release(frame);

	while ((frame = k_fifo_get(&fido2_ble_tx_fifo, K_NO_WAIT)) != NULL) {
		tx_frame_release(frame);
	}
}

int fido2_ble_framing_init(const struct fido2_ble_framing_callbacks *callbacks)
{
	if ((callbacks == NULL) || (callbacks->message_received == NULL) ||
	    (callbacks->cancel_received == NULL) || (callbacks->transaction_active == NULL)) {
		return -EINVAL;
	}

	if (!atomic_cas(&framing_ctx.work_queue_started, 0, 1)) {
		return -EALREADY;
	}

	framing_ctx.callbacks = *callbacks;
	atomic_clear(&framing_ctx.stopping);
	k_sem_reset(&fido2_ble_tx_complete);
	k_msgq_purge(&fido2_ble_rx_msgq);

	/* Reassembly uses a private cooperative work queue to keep GATT callbacks short. */
	k_work_init(&rx_work, rx_work_handler);
	k_work_init_delayable(&rx_timeout_work, rx_timeout_handler);
	k_work_init_delayable(&tx_work, tx_work_handler);
	k_work_queue_start(&rx_work_q, rx_work_q_stack, K_THREAD_STACK_SIZEOF(rx_work_q_stack),
			   K_PRIO_COOP(4), NULL);
	k_thread_name_set(rx_work_q.thread_id, "fido2_ble_rx");

	atomic_set(&framing_ctx.initialized, 1);

	return 0;
}

void fido2_ble_framing_shutdown(void)
{
	struct fido2_ble_rx_fragment fragment;
	struct fido2_ble_tx_frame *frame;
	struct bt_conn *rx_conn;
	struct k_work_sync sync;
	bool wait_for_tx;
	int err;

	if (!atomic_cas(&framing_ctx.initialized, 1, 0)) {
		return;
	}

	atomic_set(&framing_ctx.stopping, 1);
	(void)k_work_cancel_delayable_sync(&rx_timeout_work, &sync);
	(void)k_work_flush(&rx_work, &sync);

	while (k_msgq_get(&fido2_ble_rx_msgq, &fragment, K_NO_WAIT) == 0) {
		bt_conn_unref(fragment.conn);
	}

	k_mutex_lock(&rx_mutex, K_FOREVER);
	rx_conn = rx_clear_locked();
	k_mutex_unlock(&rx_mutex);
	if (rx_conn != NULL) {
		bt_conn_unref(rx_conn);
	}

	(void)k_work_cancel_delayable_sync(&tx_work, &sync);
	while ((frame = k_fifo_get(&fido2_ble_tx_fifo, K_NO_WAIT)) != NULL) {
		tx_frame_release(frame);
	}

	/* An in-flight notification owns the fragment buffer until its completion callback runs. */
	k_mutex_lock(&tx_mutex, K_FOREVER);
	wait_for_tx = framing_ctx.tx.in_flight;
	if (framing_ctx.tx.active != NULL) {
		framing_ctx.tx.active->cancelled = true;
	}
	if (wait_for_tx) {
		k_sem_reset(&fido2_ble_tx_complete);
		frame = NULL;
	} else {
		frame = tx_active_remove_locked();
	}
	k_mutex_unlock(&tx_mutex);
	tx_frame_release(frame);

	if (wait_for_tx) {
		err = k_sem_take(&fido2_ble_tx_complete, K_MSEC(FIDO2_BLE_TX_SHUTDOWN_TIMEOUT_MS));
		if (err != 0) {
			LOG_WRN("Timed out waiting for FIDO BLE notification completion");
		}

		k_mutex_lock(&tx_mutex, K_FOREVER);
		frame = tx_active_remove_locked();
		k_mutex_unlock(&tx_mutex);
		tx_frame_release(frame);
	}

	err = k_work_queue_drain(&rx_work_q, true);
	if (err >= 0) {
		err = k_work_queue_stop(&rx_work_q, K_FOREVER);
	}
	if (err != 0) {
		LOG_WRN("Failed to stop FIDO BLE RX work queue: %d", err);
	}

	memset(&framing_ctx.callbacks, 0, sizeof(framing_ctx.callbacks));
}
