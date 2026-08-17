/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#include "fido2_transport_ble.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define CTAPBLE_ERR_INVALID_CMD 0x01U
#define CTAPBLE_ERR_INVALID_LEN 0x03U
#define CTAPBLE_ERR_INVALID_SEQ 0x04U
#define CTAPBLE_ERR_REQ_TIMEOUT 0x05U
#define CTAPBLE_ERR_BUSY        0x06U
#define CTAPBLE_ERR_OTHER       0x7FU

#define ATT_NOTIFICATION_OVERHEAD                 3U
#define CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE      3U
#define CTAPBLE_CONTINUATION_FRAGMENT_HEADER_SIZE 1U

#define CTAPBLE_TX_FRAGMENT_MAX_SIZE                                                               \
	MIN(BT_L2CAP_TX_MTU - ATT_NOTIFICATION_OVERHEAD, BT_ATT_MAX_ATTRIBUTE_LEN)

/**
 *
 * @brief Queued fragment received from the CTAP Bluetooth LE Control Point.
 */
struct ctapble_rx_fragment {
	/** Referenced connection on which the fragment was received. */
	struct bt_conn *conn;
	/** Length of @p data in bytes. */
	uint16_t len;
	/** Raw initial or continuation fragment. */
	uint8_t data[CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH];
};

/**
 * @brief Complete CTAP Bluetooth LE command waiting to be fragmented and transmitted.
 */
struct ctapble_tx_frame {
	/** Reserved FIFO linkage required by Zephyr's FIFO implementation. */
	void *fifo_reserved;
	/** Referenced connection on which the frame is transmitted. */
	struct bt_conn *conn;
	/** Length of @p data in bytes. */
	size_t len;
	/** CTAP Bluetooth LE command identifier. */
	enum ctapble_command command;
	/** Complete command payload. */
	uint8_t data[CONFIG_FIDO2_CBOR_MAX_SIZE + 1];
};

/**
 * @brief Runtime state for CTAP Bluetooth LE RX fragmentation and reassembly
 */
struct ctapble_rx_state {
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
	uint8_t data[CONFIG_FIDO2_CBOR_MAX_SIZE + 1];
};

/**
 * @brief Runtime state for FIDO Bluetooth LE TX fragmentation and reassembly
 */
struct ctapble_tx_state {
	/** Frame currently being transmitted. */
	struct ctapble_tx_frame *active;
	/** true after the command's initial fragment has been submitted. */
	bool initial_sent;
	/** true while a GATT notification completion callback is pending. */
	bool in_flight;
	/** Number of payload bytes already submitted. */
	size_t offset;
	/** Sequence number to use for the next continuation fragment. */
	uint8_t sequence;
	/** Scratch buffer for the next GATT notification fragment. */
	uint8_t fragment[CTAPBLE_TX_FRAGMENT_MAX_SIZE];
};

/**
 * @brief Runtime state for CTAP Bluetooth LE framing and reassembly.
 */
struct ctapble_framing_context {
	/** Callbacks used to dispatch complete requests and control events. */
	struct ctapble_framing_callbacks callbacks;
	/** Set while the framing layer accepts RX and TX work. */
	atomic_t initialized;
	/** Set while shutdown is draining asynchronous work. */
	atomic_t stopping;
	/** Receive-side state for reassembling one CTAP Bluetooth LE command. */
	struct ctapble_rx_state rx;
	/** Transmit-side state for fragmenting one queued CTAP Bluetooth LE command. */
	struct ctapble_tx_state tx;
};

static struct ctapble_framing_context framing_ctx;

/* GATT write callbacks only enqueue fragments; reassembly runs on the dedicated RX queue. */
K_MSGQ_DEFINE_TYPE(ctapble_rx_msgq, struct ctapble_rx_fragment, CONFIG_FIDO2_BLE_RX_QUEUE_DEPTH);
K_MEM_SLAB_DEFINE(ctapble_tx_slab, sizeof(struct ctapble_tx_frame), CONFIG_FIDO2_BLE_TX_FRAME_COUNT,
		  __alignof__(struct ctapble_tx_frame));
K_FIFO_DEFINE(ctapble_tx_fifo);

static struct k_work_q rx_work_q;
static K_THREAD_STACK_DEFINE(rx_work_q_stack, CONFIG_FIDO2_BLE_RX_WORKQ_STACK_SIZE);
static struct k_work rx_work;
static struct k_work_delayable rx_timeout_work;
static struct k_work_delayable tx_work;
static K_MUTEX_DEFINE(rx_mutex);
static K_MUTEX_DEFINE(tx_mutex);
static K_MUTEX_DEFINE(rx_submit_mutex);

static void tx_schedule(k_timeout_t delay)
{
	int ret;

	if (!atomic_get(&framing_ctx.initialized) || atomic_get(&framing_ctx.stopping)) {
		return;
	}

	ret = k_work_reschedule(&tx_work, delay);
	if (ret < 0) {
		LOG_ERR("Failed to schedule CTAP Bluetooth LE TX work: %d", ret);
	}
}

static void tx_frame_release(struct ctapble_tx_frame *frame)
{
	if (frame == NULL) {
		return;
	}

	bt_conn_drop(&frame->conn);
	k_mem_slab_free(&ctapble_tx_slab, frame);
}

static struct ctapble_tx_frame *tx_active_remove_locked(void)
{
	struct ctapble_tx_frame *frame = framing_ctx.tx.active;

	framing_ctx.tx.active = NULL;
	framing_ctx.tx.initial_sent = false;
	framing_ctx.tx.in_flight = false;
	framing_ctx.tx.offset = 0;
	framing_ctx.tx.sequence = 0;

	return frame;
}

static bool tx_error_is_transient(int err)
{
	return (err == -ENOMEM) || (err == -ENOBUFS) || (err == -EAGAIN);
}

static k_timeout_t conn_interval_based_delay(struct bt_conn *conn, uint32_t minimum_ms)
{
	struct bt_conn_info info;
	uint32_t minimum_us;
	int ret;

	minimum_us = minimum_ms * USEC_PER_MSEC;

	ret = bt_conn_get_info(conn, &info);
	if ((ret != 0) || (info.type != BT_CONN_TYPE_LE) || (info.le.interval_us == 0U)) {
		return K_MSEC(minimum_ms);
	}

	return K_USEC(MAX(minimum_us, info.le.interval_us));
}

static void conn_params_request(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int ret;

	ret = bt_conn_get_info(conn, &info);
	if (ret != 0) {
		LOG_DBG("Failed to read Bluetooth LE connection parameters: %d", ret);
		return;
	}

	if ((info.type != BT_CONN_TYPE_LE) || (info.le.interval_us == 0U)) {
		return;
	}

	/*
	 * Zephyr's default parameters request a connection interval of
	 * 30-50 ms, latency 0 and a 4 s supervision timeout.
	 */
	if (info.le.interval_us <= BT_CONN_INTERVAL_TO_US(BT_GAP_INIT_CONN_INT_MAX)) {
		return;
	}

	ret = bt_conn_le_param_update(conn, BT_LE_CONN_PARAM_DEFAULT);
	if (ret != 0) {
		LOG_WRN("Failed to request FIDO Bluetooth LE connection parameters: %d", ret);
	}
}

static void rx_timeout_reschedule(struct bt_conn *conn)
{
	int ret;

	ret = k_work_reschedule_for_queue(
		&rx_work_q, &rx_timeout_work,
		conn_interval_based_delay(conn, CONFIG_FIDO2_BLE_RX_TIMEOUT_MS));
	if (ret < 0) {
		LOG_ERR("Failed to reschedule CTAP Bluetooth LE RX timeout work: %d", ret);
	}
}

static void mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
	int ret;

	ret = k_mutex_lock(mutex, timeout);
	__ASSERT(ret == 0, "Failed to lock mutex: %d", ret);
	ARG_UNUSED(ret);
}

static void mutex_unlock(struct k_mutex *mutex)
{
	int ret;

	ret = k_mutex_unlock(mutex);
	__ASSERT(ret == 0, "Failed to unlock mutex: %d", ret);
	ARG_UNUSED(ret);
}

static void tx_notify_complete(struct bt_conn *conn, void *user_data)
{
	struct ctapble_tx_frame *completed = NULL;
	bool send_next = false;

	ARG_UNUSED(conn);
	ARG_UNUSED(user_data);

	mutex_lock(&tx_mutex, K_FOREVER);

	if ((framing_ctx.tx.active != NULL) && framing_ctx.tx.in_flight) {
		framing_ctx.tx.in_flight = false;
		if (atomic_get(&framing_ctx.stopping) ||
		    (framing_ctx.tx.offset >= framing_ctx.tx.active->len)) {
			completed = tx_active_remove_locked();
		} else {
			send_next = true;
		}
	}
	mutex_unlock(&tx_mutex);

	tx_frame_release(completed);

	if (send_next || (completed != NULL)) {
		tx_schedule(K_NO_WAIT);
	}
}

static void tx_work_handler(struct k_work *work)
{
	struct ctapble_tx_frame *frame;
	struct ctapble_tx_frame *failed = NULL;
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
	int ret;

	ARG_UNUSED(work);

	mutex_lock(&tx_mutex, K_FOREVER);
	if (atomic_get(&framing_ctx.stopping) || framing_ctx.tx.in_flight) {
		mutex_unlock(&tx_mutex);
		return;
	}

	/* Serialize one complete command at a time; each GATT notification is asynchronous. */
	if (framing_ctx.tx.active == NULL) {
		framing_ctx.tx.active = k_fifo_get(&ctapble_tx_fifo, K_NO_WAIT);
		framing_ctx.tx.initial_sent = false;
		framing_ctx.tx.offset = 0;
		framing_ctx.tx.sequence = 0;
	}

	frame = framing_ctx.tx.active;
	if (frame == NULL) {
		mutex_unlock(&tx_mutex);
		return;
	}

	if (!ctapble_gatt_conn_is_ready(frame->conn)) {
		failed = tx_active_remove_locked();
		mutex_unlock(&tx_mutex);
		tx_frame_release(failed);
		tx_schedule(K_NO_WAIT);
		return;
	}

	mtu = bt_gatt_get_mtu(frame->conn);
	if (mtu <= ATT_NOTIFICATION_OVERHEAD) {
		failed = tx_active_remove_locked();
		mutex_unlock(&tx_mutex);
		tx_frame_release(failed);
		tx_schedule(K_NO_WAIT);
		return;
	}

	max_fragment_len = MIN((uint16_t)sizeof(framing_ctx.tx.fragment),
			       (uint16_t)(mtu - ATT_NOTIFICATION_OVERHEAD));
	if (max_fragment_len < CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE) {
		failed = tx_active_remove_locked();
		mutex_unlock(&tx_mutex);
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
		chunk_len = MIN(frame->len,
				(size_t)(max_fragment_len - CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE));
		if (chunk_len != 0) {
			(void)memcpy(&framing_ctx.tx.fragment[CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE],
				     frame->data, chunk_len);
		}
		fragment_len = (uint16_t)(CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE + chunk_len);
		next_offset = chunk_len;
		framing_ctx.tx.initial_sent = true;
	} else {
		/* Continuation fragments carry a 7-bit sequence number followed by payload. */
		chunk_len =
			MIN(frame->len - old_offset,
			    (size_t)(max_fragment_len - CTAPBLE_CONTINUATION_FRAGMENT_HEADER_SIZE));
		framing_ctx.tx.fragment[0] = old_sequence;
		(void)memcpy(&framing_ctx.tx.fragment[CTAPBLE_CONTINUATION_FRAGMENT_HEADER_SIZE],
			     &frame->data[old_offset], chunk_len);
		fragment_len = (uint16_t)(CTAPBLE_CONTINUATION_FRAGMENT_HEADER_SIZE + chunk_len);
		next_offset = old_offset + chunk_len;
		next_sequence = (old_sequence + 1) & BIT_MASK(7);
	}

	framing_ctx.tx.offset = next_offset;
	framing_ctx.tx.sequence = next_sequence;

	framing_ctx.tx.in_flight = true;

	mutex_unlock(&tx_mutex);

	ret = ctapble_gatt_notify(frame->conn, framing_ctx.tx.fragment, fragment_len,
				  tx_notify_complete, NULL);
	if (ret == 0) {
		return;
	}

	/* Submission failed before completion ownership transferred; roll back TX progress. */
	mutex_lock(&tx_mutex, K_FOREVER);
	if ((framing_ctx.tx.active == frame) && framing_ctx.tx.in_flight) {
		framing_ctx.tx.in_flight = false;
		framing_ctx.tx.offset = old_offset;
		framing_ctx.tx.sequence = old_sequence;
		framing_ctx.tx.initial_sent = old_initial_sent;

		if (tx_error_is_transient(ret) && !atomic_get(&framing_ctx.stopping)) {
			retry = true;
		} else {
			failed = tx_active_remove_locked();
		}
	}
	mutex_unlock(&tx_mutex);

	if (!tx_error_is_transient(ret) && (ret != -ENOTCONN) && (ret != -EACCES)) {
		LOG_WRN("CTAP Bluetooth LE notification failed: %d", ret);
	}

	tx_frame_release(failed);
	if (retry) {
		tx_schedule(
			conn_interval_based_delay(frame->conn, CONFIG_FIDO2_BLE_TX_RETRY_DELAY_MS));
	} else if (failed != NULL) {
		tx_schedule(K_NO_WAIT);
	}
}

void ctapble_framing_purge_keepalives(struct bt_conn *conn)
{
	struct ctapble_tx_frame *kept[CONFIG_FIDO2_BLE_TX_FRAME_COUNT];
	struct ctapble_tx_frame *purged[CONFIG_FIDO2_BLE_TX_FRAME_COUNT];
	struct ctapble_tx_frame *frame;
	size_t kept_count = 0;
	size_t purged_count = 0;

	if (conn == NULL) {
		return;
	}

	mutex_lock(&tx_mutex, K_FOREVER);

	if (!atomic_get(&framing_ctx.initialized) || atomic_get(&framing_ctx.stopping)) {
		mutex_unlock(&tx_mutex);
		return;
	}

	frame = k_fifo_get(&ctapble_tx_fifo, K_NO_WAIT);

	while (frame != NULL) {
		if ((frame->conn == conn) && (frame->command == CTAPBLE_CMD_KEEPALIVE)) {
			purged[purged_count++] = frame;
		} else {
			kept[kept_count++] = frame;
		}

		frame = k_fifo_get(&ctapble_tx_fifo, K_NO_WAIT);
	}

	/* Restore all non-KEEPALIVE frames in their original order. */
	for (size_t i = 0; i < kept_count; ++i) {
		k_fifo_put(&ctapble_tx_fifo, kept[i]);
	}

	mutex_unlock(&tx_mutex);

	for (size_t i = 0; i < purged_count; ++i) {
		tx_frame_release(purged[i]);
	}
}

int ctapble_framing_send(struct bt_conn *conn, enum ctapble_command command, const uint8_t *data,
			 size_t len)
{
	struct ctapble_tx_frame *frame;
	int ret;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	if ((data == NULL) && (len != 0)) {
		return -EINVAL;
	}

	if ((len > (CONFIG_FIDO2_CBOR_MAX_SIZE + 1)) || (len > UINT16_MAX)) {
		return -EMSGSIZE;
	}

	if (!atomic_get(&framing_ctx.initialized) || atomic_get(&framing_ctx.stopping)) {
		return -ESHUTDOWN;
	}

	if (!ctapble_gatt_conn_is_ready(conn)) {
		return -ENOTCONN;
	}

	ret = k_mem_slab_alloc(&ctapble_tx_slab, (void **)&frame, K_NO_WAIT);
	if (ret != 0) {
		return -ENOMEM;
	}

	/* Copy the complete command so callers may release their payload after this returns. */
	frame->conn = bt_conn_ref(conn);
	if (frame->conn == NULL) {
		k_mem_slab_free(&ctapble_tx_slab, frame);
		return -ENOTCONN;
	}

	frame->command = command;
	frame->len = len;

	if (len != 0) {
		(void)memcpy(frame->data, data, len);
	}

	/*
	 * Serialize TX admission with shutdown. Shutdown changes stopping while
	 * holding the same mutex, so a frame is either queued before shutdown
	 * starts draining or rejected and released here.
	 */
	mutex_lock(&tx_mutex, K_FOREVER);

	if (!atomic_get(&framing_ctx.initialized) || atomic_get(&framing_ctx.stopping)) {
		mutex_unlock(&tx_mutex);
		tx_frame_release(frame);
		return -ESHUTDOWN;
	}

	k_fifo_put(&ctapble_tx_fifo, frame);
	mutex_unlock(&tx_mutex);

	tx_schedule(K_NO_WAIT);

	return 0;
}

static void send_error(struct bt_conn *conn, uint8_t error_code)
{
	int ret;

	ret = ctapble_framing_send(conn, CTAPBLE_CMD_ERROR, &error_code, sizeof(error_code));
	if (ret != 0) {
		LOG_WRN("Failed to queue CTAP Bluetooth LE error 0x%02x: %d", error_code, ret);
	}
}

static struct bt_conn *rx_clear_locked(void)
{
	struct bt_conn *conn = framing_ctx.rx.conn;

	(void)memset(&framing_ctx.rx, 0, sizeof(framing_ctx.rx));

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
	int ret;

	(void)k_work_cancel_delayable(&rx_timeout_work);

	/* Only complete data-bearing commands reach this dispatcher. */
	switch (framing_ctx.rx.command) {
	case CTAPBLE_CMD_PING:
		/* Echo the complete payload back to the client. */
		ret = ctapble_framing_send(conn, CTAPBLE_CMD_PING, framing_ctx.rx.data,
					   framing_ctx.rx.expected_length);
		if (ret != 0) {
			LOG_WRN("Failed to queue CTAP PING response over Bluetooth LE: %d", ret);
		}
		break;
	case CTAPBLE_CMD_MSG:
		/* Forward the complete CTAP message to the transport/core layer. */
		ret = framing_ctx.callbacks.message_received(conn, framing_ctx.rx.data,
							     framing_ctx.rx.expected_length);
		if ((ret == -EBUSY) || (ret == -ENOBUFS)) {
			send_error(conn, CTAPBLE_ERR_BUSY);
		} else if (ret != 0) {
			send_error(conn, CTAPBLE_ERR_OTHER);
		}
		break;
	default:
		send_error(conn, CTAPBLE_ERR_INVALID_CMD);
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

	if (len < CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE) {
		if ((len > 0U) && (data[0] == CTAPBLE_CMD_CANCEL)) {
			return;
		}

		send_error(conn, CTAPBLE_ERR_INVALID_LEN);
		return;
	}

	command = data[0];
	expected_len = sys_get_be16(&data[1]);
	payload_len = len - CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE;

	/*
	 * CANCEL is a zero-length control command. Handle it immediately so it can abort
	 * reassembly or the active CTAP operation; the command itself has no response.
	 */
	if (command == CTAPBLE_CMD_CANCEL) {
		if ((expected_len != 0) || (payload_len != 0)) {
			return;
		}

		rx_reset_locked();
		framing_ctx.callbacks.cancel_received(conn);
		return;
	}

	if ((command != CTAPBLE_CMD_PING) && (command != CTAPBLE_CMD_MSG)) {
		send_error(conn, CTAPBLE_ERR_INVALID_CMD);
		return;
	}

	/* A new PING/MSG must not replace an incomplete frame or an active CTAP transaction. */
	if (framing_ctx.rx.active || framing_ctx.callbacks.transaction_active()) {
		send_error(conn, CTAPBLE_ERR_BUSY);
		return;
	}

	if ((expected_len > sizeof(framing_ctx.rx.data)) || (payload_len > expected_len) ||
	    ((command == CTAPBLE_CMD_MSG) && (expected_len == 0))) {
		send_error(conn, CTAPBLE_ERR_INVALID_LEN);
		return;
	}

	/* Request suitable link parameters when starting a new CTAP transaction. */
	conn_params_request(conn);

	/* Retain the connection until the message completes or it times out. */
	framing_ctx.rx.conn = bt_conn_ref(conn);
	framing_ctx.rx.active = true;
	framing_ctx.rx.command = command;
	framing_ctx.rx.expected_length = expected_len;
	framing_ctx.rx.received_length = payload_len;
	framing_ctx.rx.expected_sequence = 0;

	if (payload_len != 0) {
		(void)memcpy(framing_ctx.rx.data, &data[CTAPBLE_INITIAL_FRAGMENT_HEADER_SIZE],
			     payload_len);
	}

	if (framing_ctx.rx.received_length == framing_ctx.rx.expected_length) {
		dispatch_complete_locked(conn);
		return;
	}

	rx_timeout_reschedule(conn);
}

static void process_continuation_locked(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	uint16_t payload_len;
	uint16_t remaining_len;
	uint8_t sequence;

	if (!framing_ctx.rx.active || (framing_ctx.rx.conn != conn)) {
		send_error(conn, CTAPBLE_ERR_INVALID_SEQ);
		return;
	}

	/* Continuation fragments must arrive in strictly increasing 7-bit sequence order. */
	sequence = data[0];
	if (sequence != framing_ctx.rx.expected_sequence) {
		LOG_WRN("Invalid CTAP Bluetooth LE sequence: got %u, expected %u", sequence,
			framing_ctx.rx.expected_sequence);
		rx_reset_locked();
		send_error(conn, CTAPBLE_ERR_INVALID_SEQ);
		return;
	}

	payload_len = len - CTAPBLE_CONTINUATION_FRAGMENT_HEADER_SIZE;
	remaining_len = framing_ctx.rx.expected_length - framing_ctx.rx.received_length;
	if ((payload_len == 0) || (payload_len > remaining_len)) {
		rx_reset_locked();
		send_error(conn, CTAPBLE_ERR_INVALID_LEN);
		return;
	}

	(void)memcpy(&framing_ctx.rx.data[framing_ctx.rx.received_length],
		     &data[CTAPBLE_CONTINUATION_FRAGMENT_HEADER_SIZE], payload_len);

	framing_ctx.rx.received_length += payload_len;
	framing_ctx.rx.expected_sequence = (framing_ctx.rx.expected_sequence + 1) & BIT_MASK(7);

	if (framing_ctx.rx.received_length == framing_ctx.rx.expected_length) {
		dispatch_complete_locked(conn);
		return;
	}

	rx_timeout_reschedule(conn);
}

static void process_fragment(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	if (!ctapble_gatt_conn_is_ready(conn)) {
		return;
	}

	LOG_HEXDUMP_DBG(data, len, "CTAP Bluetooth LE RX fragment");

	mutex_lock(&rx_mutex, K_FOREVER);
	/* Bit 7 distinguishes command-bearing initial fragments from continuation fragments. */
	if ((data[0] & BIT(7)) != 0) {
		process_initial_locked(conn, data, len);
	} else {
		process_continuation_locked(conn, data, len);
	}
	mutex_unlock(&rx_mutex);
}

static void rx_work_handler(struct k_work *work)
{
	struct ctapble_rx_fragment fragment;

	ARG_UNUSED(work);

	while (k_msgq_get(&ctapble_rx_msgq, &fragment, K_NO_WAIT) == 0) {
		process_fragment(fragment.conn, fragment.data, fragment.len);
		bt_conn_drop(&fragment.conn);
	}
}

static void rx_timeout_handler(struct k_work *work)
{
	struct bt_conn *conn;

	ARG_UNUSED(work);

	mutex_lock(&rx_mutex, K_FOREVER);
	if (!framing_ctx.rx.active) {
		mutex_unlock(&rx_mutex);
		return;
	}

	conn = rx_clear_locked();
	mutex_unlock(&rx_mutex);

	LOG_WRN("CTAP Bluetooth LE reassembly timed out");
	if (conn != NULL) {
		send_error(conn, CTAPBLE_ERR_REQ_TIMEOUT);
		bt_conn_unref(conn);
	}
}

static void rx_queue_purge(void)
{
	struct ctapble_rx_fragment fragment;

	while (k_msgq_get(&ctapble_rx_msgq, &fragment, K_NO_WAIT) == 0) {
		bt_conn_unref(fragment.conn);
	}
}

int ctapble_framing_submit_fragment(struct bt_conn *conn, const void *data, uint16_t len)
{
	struct ctapble_rx_fragment fragment;
	int ret;

	if ((conn == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	if ((len == 0U) || (len > CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH)) {
		return -EMSGSIZE;
	}

	mutex_lock(&rx_submit_mutex, K_FOREVER);

	if (!atomic_get(&framing_ctx.initialized) || atomic_get(&framing_ctx.stopping)) {
		ret = -ESHUTDOWN;
		goto out_unlock;
	}

	if (!ctapble_gatt_conn_is_ready(conn)) {
		ret = -ENOTCONN;
		goto out_unlock;
	}

	/* Hold a connection reference while the fragment waits in the asynchronous RX queue. */
	fragment.conn = bt_conn_ref(conn);
	fragment.len = len;
	(void)memcpy(fragment.data, data, len);

	ret = k_msgq_put(&ctapble_rx_msgq, &fragment, K_NO_WAIT);
	if (ret != 0) {
		bt_conn_unref(fragment.conn);
		ret = -ENOMEM;
		goto out_unlock;
	}

	ret = k_work_submit_to_queue(&rx_work_q, &rx_work);
	if (ret < 0) {
		LOG_ERR("Failed to submit CTAP Bluetooth LE RX work: %d", ret);
		rx_queue_purge();
		goto out_unlock;
	}

	ret = 0;

out_unlock:
	mutex_unlock(&rx_submit_mutex);

	return ret;
}

void ctapble_framing_connection_closed(struct bt_conn *conn)
{
	struct ctapble_rx_fragment fragment;
	struct ctapble_tx_frame *frame;
	struct bt_conn *rx_conn = NULL;
	struct k_work_sync sync;

	/* Drop partial RX state and every queued TX item that can no longer be delivered. */
	(void)k_work_cancel_delayable(&rx_timeout_work);

	mutex_lock(&rx_mutex, K_FOREVER);
	if ((framing_ctx.rx.conn == NULL) || (framing_ctx.rx.conn == conn)) {
		rx_conn = rx_clear_locked();
	}
	mutex_unlock(&rx_mutex);
	if (rx_conn != NULL) {
		bt_conn_unref(rx_conn);
	}

	while (k_msgq_get(&ctapble_rx_msgq, &fragment, K_NO_WAIT) == 0) {
		bt_conn_unref(fragment.conn);
	}

	/*
	 * Ensure the TX worker no longer accesses the active frame before
	 * ownership is removed.
	 */
	(void)k_work_cancel_delayable_sync(&tx_work, &sync);

	mutex_lock(&tx_mutex, K_FOREVER);
	if ((framing_ctx.tx.active != NULL) && (framing_ctx.tx.active->conn == conn)) {
		frame = tx_active_remove_locked();
	} else {
		frame = NULL;
	}
	mutex_unlock(&tx_mutex);
	tx_frame_release(frame);

	frame = k_fifo_get(&ctapble_tx_fifo, K_NO_WAIT);
	while (frame != NULL) {
		tx_frame_release(frame);
		frame = k_fifo_get(&ctapble_tx_fifo, K_NO_WAIT);
	}
}

int ctapble_framing_init(const struct ctapble_framing_callbacks *callbacks)
{
	if ((callbacks == NULL) || (callbacks->message_received == NULL) ||
	    (callbacks->cancel_received == NULL) || (callbacks->transaction_active == NULL)) {
		return -EINVAL;
	}

	if (!atomic_cas(&framing_ctx.initialized, 0, 1)) {
		return -EALREADY;
	}

	framing_ctx.callbacks = *callbacks;

	atomic_clear(&framing_ctx.stopping);

	k_msgq_purge(&ctapble_rx_msgq);

	k_work_init(&rx_work, rx_work_handler);
	k_work_init_delayable(&rx_timeout_work, rx_timeout_handler);
	k_work_init_delayable(&tx_work, tx_work_handler);

	k_work_queue_start(&rx_work_q, rx_work_q_stack, K_THREAD_STACK_SIZEOF(rx_work_q_stack),
			   K_PRIO_COOP(CONFIG_BT_RX_PRIO + 1), NULL);

	k_thread_name_set(rx_work_q.thread_id, "ctap_bluetooth_le_rx");

	return 0;
}

void ctapble_framing_shutdown(void)
{
	struct ctapble_tx_frame *active_frame = NULL;
	struct ctapble_tx_frame *frame;
	struct bt_conn *rx_conn;
	struct k_work_sync sync;

	if (!atomic_get(&framing_ctx.initialized)) {
		return;
	}

	/*
	 * Block new RX submissions for the complete shutdown operation.
	 */
	mutex_lock(&rx_submit_mutex, K_FOREVER);

	/*
	 * Close TX admission. ctapble_framing_send() checks stopping while
	 * holding the same mutex before inserting a frame into the FIFO.
	 */
	mutex_lock(&tx_mutex, K_FOREVER);

	if (!atomic_get(&framing_ctx.initialized) || !atomic_cas(&framing_ctx.stopping, 0, 1)) {
		mutex_unlock(&tx_mutex);
		mutex_unlock(&rx_submit_mutex);
		return;
	}

	mutex_unlock(&tx_mutex);

	(void)k_work_cancel_delayable_sync(&rx_timeout_work, &sync);
	(void)k_work_flush(&rx_work, &sync);

	rx_queue_purge();

	mutex_lock(&rx_mutex, K_FOREVER);
	rx_conn = rx_clear_locked();
	mutex_unlock(&rx_mutex);

	if (rx_conn != NULL) {
		bt_conn_unref(rx_conn);
	}

	/*
	 * Wait until the TX worker is no longer modifying the active TX state.
	 * A submitted GATT notification may still complete asynchronously.
	 */
	(void)k_work_cancel_delayable_sync(&tx_work, &sync);

	mutex_lock(&tx_mutex, K_FOREVER);
	active_frame = tx_active_remove_locked();
	mutex_unlock(&tx_mutex);

	tx_frame_release(active_frame);

	/*
	 * No new frames can enter the FIFO because stopping was set under
	 * tx_mutex and send() checks it under that same mutex.
	 */
	frame = k_fifo_get(&ctapble_tx_fifo, K_NO_WAIT);
	while (frame != NULL) {
		tx_frame_release(frame);
		frame = k_fifo_get(&ctapble_tx_fifo, K_NO_WAIT);
	}

	mutex_unlock(&rx_submit_mutex);
}
