/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client_multiprotocol.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>

#include "headers/client_internal.h"

LOG_MODULE_REGISTER(net_http_client_mp, CONFIG_NET_HTTP_LOG_LEVEL);

BUILD_ASSERT(IS_ENABLED(CONFIG_HTTP_CLIENT_VERSION_2), "Some HTTP version needs to be enabled");

#define HTTP_WAIT_SLICE_MS 1000

static int client_transport_close(struct http_client *client);

/* Deadlines of asynchronous transactions are enforced from a work queue of
 * the client's own: resetting a stream sends to a peer that may have stopped
 * reading, which must not stall the system work queue.
 */
static K_KERNEL_STACK_DEFINE(http_client_wq_stack, CONFIG_HTTP_CLIENT_WORKQUEUE_STACK_SIZE);
static struct k_work_q http_client_wq;
static K_MUTEX_DEFINE(http_client_wq_lock);
static bool http_client_wq_started;

static void http_client_wq_start(void)
{
	const struct k_work_queue_config cfg = {
		.name = "http_client",
	};

	k_mutex_lock(&http_client_wq_lock, K_FOREVER);

	if (!http_client_wq_started) {
		k_work_queue_init(&http_client_wq);
		k_work_queue_start(&http_client_wq, http_client_wq_stack,
				   K_KERNEL_STACK_SIZEOF(http_client_wq_stack),
				   CLAMP(CONFIG_HTTP_CLIENT_WORKQUEUE_PRIORITY,
					 K_HIGHEST_APPLICATION_THREAD_PRIO,
					 K_LOWEST_APPLICATION_THREAD_PRIO), &cfg);
		http_client_wq_started = true;
	}

	k_mutex_unlock(&http_client_wq_lock);
}

static const char *http_version_txt(enum http_client_version version)
{
	switch (version) {
	case HTTP_CLIENT_VERSION_1_1:
		return "HTTP/1.1";
	case HTTP_CLIENT_VERSION_2:
		return "HTTP/2";
	case HTTP_CLIENT_VERSION_3:
		return "HTTP/3";
	default:
		return "";
	}
}

static const struct http_client_ops *http_client_get_ops(enum http_client_version version)
{
	const struct http_client_ops *ops;

	switch (version) {
	case HTTP_CLIENT_VERSION_1_1:
		ops = NULL; /* Placeholder for future implementation */
		break;
	case HTTP_CLIENT_VERSION_2:
		ops = http_client_get_http2_ops();
		break;
	case HTTP_CLIENT_VERSION_3:
		ops = NULL; /* Placeholder for future implementation */
		break;
	default:
		ops = NULL;
		break;
	}

	return ops;
}

static void http_session_init(struct http_client_session *session,
			      struct http_client_session_config *config)
{
	memset(session, 0, sizeof(*session));
	memcpy(&session->server_addr, &config->server_addr, config->addrlen);
	session->addrlen = config->addrlen;
	session->host = config->host;
	session->async = config->async;
}

static bool http_session_has_streams(const struct http_client_session *session)
{
	ARRAY_FOR_EACH(session->streams, i) {
		if (session->streams[i].is_used) {
			return true;
		}
	}

	return false;
}

/* Whether the current thread is inside http_client_send_request() on the
 * session, in a callback of a synchronous request.
 */
static bool http_session_caller_is_current(const struct http_client_session *session)
{
	ARRAY_FOR_EACH(session->streams, i) {
		const struct http_client_stream *stream = &session->streams[i];

		if (stream->is_used && stream->caller == k_current_get()) {
			return true;
		}
	}

	return false;
}

/* The thread leaves http_client_send_request(): a disconnect draining the
 * callers waits for this. Nothing of the client is touched afterwards.
 */
static void client_leave(struct http_client *client)
{
	k_mutex_lock(&client->lock, K_FOREVER);
	client->callers--;
	http_client_tx_wakeup(client);
	k_mutex_unlock(&client->lock);
}

static struct http_client_stream *http_stream_alloc(struct http_client_session *session)
{
	ARRAY_FOR_EACH(session->streams, i) {
		struct http_client_stream *stream = &session->streams[i];

		if (stream->is_used) {
			continue;
		}

		memset(stream, 0, sizeof(*stream));

		stream->is_used = true;

		return stream;
	}

	return NULL;
}

static void http_stream_release(struct http_client_session *session,
				struct http_client_stream *stream)
{
	if (!PART_OF_ARRAY(session->streams, stream)) {
		return;
	}

	memset(stream, 0, sizeof(*stream));
}

static void http_stream_init(struct http_client_stream *stream,
			     const struct http_client_request *request,
			     int32_t timeout, void *user_data)
{
	stream->request = request;
	stream->user_data = user_data;
	/* A negative timeout means wait forever, represented as the maximum
	 * absolute deadline so that http_stream_wait() never expires.
	 */
	stream->timeout = (timeout < 0) ? INT64_MAX : k_uptime_get() + timeout;
	stream->completed = false;
	stream->result = 0;
}

/* Arm the asynchronous deadline timer for the earliest pending stream
 * deadline, or stop it when no stream is waiting. Must be called with the
 * client lock held.
 */
static void http_client_timeout_reschedule(struct http_client *client)
{
	int64_t earliest = INT64_MAX;
	int64_t remaining;

	ARRAY_FOR_EACH(client->session.streams, i) {
		struct http_client_stream *stream = &client->session.streams[i];

		if (stream->is_used && !stream->completed && stream->timeout < earliest) {
			earliest = stream->timeout;
		}
	}

	if (earliest == INT64_MAX) {
		(void)k_work_cancel_delayable(&client->timeout_work);
		return;
	}

	remaining = MAX(earliest - k_uptime_get(), 0);
	(void)k_work_reschedule_for_queue(&http_client_wq, &client->timeout_work,
					  K_MSEC(remaining));
}

/* Expire asynchronous transactions whose deadline has passed: the stream is
 * reset on the wire and the application is notified, the same way as for a
 * synchronous timeout.
 */
static void http_client_timeout_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct http_client *client = CONTAINER_OF(dwork, struct http_client, timeout_work);
	const struct http_client_ops *http_ops;
	int64_t now;

	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->session.state != HTTP_CLIENT_SESSION_OPEN || !client->session.async) {
		goto out;
	}

	http_ops = http_client_get_ops(client->version);
	now = k_uptime_get();

	ARRAY_FOR_EACH(client->session.streams, i) {
		struct http_client_stream *stream = &client->session.streams[i];

		if (!stream->is_used || stream->completed || stream->timeout > now) {
			continue;
		}

		NET_ERR("Transaction timed out");

		if (http_ops != NULL && http_ops->stream_reset != NULL) {
			(void)http_ops->stream_reset(client, stream);
		}

		/* Delivers the ABORTED callback and releases the stream. */
		http_client_stream_set_complete(client, stream, -ETIMEDOUT);
	}

	http_client_timeout_reschedule(client);

out:
	k_mutex_unlock(&client->lock);

	/* A failed reset may have ended the session. */
	(void)client_transport_close(client);
}

/* Tear down the session. Streams still in flight are completed with an error
 * so that their owners are notified and, in synchronous mode, woken up. Peer
 * notifications (RST_STREAM, GOAWAY) are only attempted while the transport is
 * usable. Must be called with the client lock held.
 */
static int http_client_disconnect_locked(struct http_client *client, int reason,
					 bool transport_ok)
{
	const struct http_client_ops *http_ops;
	int stream_result = (reason < 0) ? reason : -ECONNABORTED;
	int ret;

	if (client->session.state != HTTP_CLIENT_SESSION_OPEN) {
		NET_DBG("Client HTTP session already closed");
		return 0;
	}

	http_ops = http_client_get_ops(client->version);
	if (http_ops == NULL) {
		NET_ERR("%s not supported", http_version_txt(client->version));
		return -EINVAL;
	}

	/* A send failing from here on must not start another teardown. */
	client->session.state = HTTP_CLIENT_SESSION_CLOSING;

	ARRAY_FOR_EACH(client->session.streams, i) {
		struct http_client_stream *stream = &client->session.streams[i];

		if (!stream->is_used) {
			continue;
		}

		/* A completed stream is over on the wire as well, and is left
		 * to the thread that owns it.
		 */
		if (stream->completed) {
			continue;
		}

		/* The notifications are a courtesy to the peer. Once one of
		 * them fails, part of it may be on the wire and the rest would
		 * only be garbage to the peer, and each may block for the
		 * whole send timeout.
		 */
		if (transport_ok && http_ops->stream_reset != NULL) {
			ret = http_ops->stream_reset(client, stream);
			if (ret != 0) {
				NET_ERR("Failed to reset stream, err %d", ret);
				transport_ok = false;
			}
		}

		http_client_stream_set_complete(client, stream, stream_result);
	}

	if (transport_ok && http_ops->session_close != NULL) {
		ret = http_ops->session_close(client, reason);
		if (ret != 0) {
			NET_ERR("Failed to close the session, err %d", ret);
		}
	}

	(void)k_work_cancel_delayable(&client->timeout_work);

	/* Nothing is read from the transport any more, whoever closes it. */
	if (client->transport_ops->stop != NULL) {
		client->transport_ops->stop(client);
	}

	/* The transport is closed by the caller once it has dropped the
	 * client lock, see client_transport_close(), or by a sender still on
	 * it, which the transport cannot be pulled from under.
	 */
	client->session.close_pending = true;
	client->session.state = HTTP_CLIENT_SESSION_CLOSED;

	return 0;
}

/* Close the transport of an ended session, unless a request frame is on its
 * way out, in which case that sender closes it once its frame is done. Must
 * be called with the client lock released: the close may wait for a
 * reception notification that needs the lock to return.
 */
static int client_transport_close(struct http_client *client)
{
	int ret;

	k_mutex_lock(&client->lock, K_FOREVER);

	if (!client->session.close_pending || k_mutex_lock(&client->tx_lock, K_NO_WAIT) != 0) {
		k_mutex_unlock(&client->lock);
		return 0;
	}

	k_mutex_unlock(&client->lock);

	ret = client->transport_ops->close(client);
	if (ret != 0) {
		NET_ERR("Failed to disconnect from the server, err %d", ret);
	}

	k_mutex_lock(&client->lock, K_FOREVER);
	client->session.close_pending = false;
	k_mutex_unlock(&client->tx_lock);
	k_mutex_unlock(&client->lock);

	return ret;
}

/* Internal API, for the protocol layers */

static int client_send_payload(struct http_client *client,
			       struct http_client_stream *stream,
			       const struct http_client_request *request,
			       const struct http_client_ops *http_ops)
{
	const uint8_t *payload;
	size_t payload_len;
	size_t offset;
	bool more;
	int ret;

	/* If not payload callback, use pointer provided payload if any */
	if (request->payload_cb == NULL) {
		if (request->payload != NULL && request->payload_len > 0) {
			ret = http_ops->stream_send_data(client, stream, request->payload,
							 request->payload_len, true);
			if (ret != 0 && ret != -EALREADY) {
				return ret;
			}
		}

		return 0;
	}

	/* Otherwise, use the callback until the application reports no more data */
	offset = 0;
	more = true;
	while (more) {
		more = false;
		ret = request->payload_cb(offset, &payload, &payload_len,
					  &more, stream->user_data);
		if (ret != 0) {
			NET_ERR("Application aborted payload send, err %d", ret);
			break;
		}

		if (payload_len > 0 && payload == NULL) {
			NET_ERR("No payload provided by the application");
			ret = -EINVAL;
			break;
		}

		/* An empty final chunk is allowed, and simply terminates the
		 * request payload. An empty chunk with more data pending is not,
		 * as the callback would only be called again with the same
		 * offset.
		 */
		if (payload_len == 0 && more) {
			NET_ERR("Empty payload chunk with more data pending");
			ret = -EINVAL;
			break;
		}

		ret = http_ops->stream_send_data(client, stream, payload,
						 payload_len, !more);
		if (ret == -EALREADY) {
			/* The response is complete, the rest is of no use. */
			ret = 0;
			break;
		}

		if (ret != 0) {
			break;
		}

		offset += payload_len;
	}

	return ret;
}

/* Notify the application that the transaction was aborted with error. A
 * NULL response means no stream was allocated for the request yet.
 */
static void client_transaction_aborted(struct http_client *client,
				       const struct http_client_request *request,
				       struct http_client_response *response,
				       int error, void *user_data)
{
	struct http_client_response empty = { 0 };

	if (request->response_cb == NULL) {
		return;
	}

	if (response == NULL) {
		response = &empty;
	}

	response->error = error;

	(void)request->response_cb(client, request, response,
				   HTTP_CLIENT_TRANSACTION_ABORTED, user_data);
}

struct http_client_stream *http_client_stream_lookup(struct http_client *client,
						     uint64_t stream_id)
{
	ARRAY_FOR_EACH(client->session.streams, i) {
		struct http_client_stream *stream = &client->session.streams[i];

		if (stream->is_used && !stream->completed && stream->stream_id == stream_id) {
			return stream;
		}
	}

	return NULL;
}

static void client_stream_finish(struct http_client *client,
				 const struct http_client_ops *http_ops,
				 struct http_client_stream *stream)
{
	if (http_ops != NULL && http_ops->stream_close != NULL) {
		int ret = http_ops->stream_close(client, stream);

		if (ret != 0) {
			NET_ERR("Failed to close the stream, err %d", ret);
		}
	}

	http_stream_release(&client->session, stream);
}

/* The transaction of a stream is over and reported: wake the threads waiting
 * on it and, when no thread will pick it up, close and release the stream.
 */
static void client_stream_completed(struct http_client *client,
				    struct http_client_stream *stream)
{
	/* A thread blocked sending on this session has to re-evaluate. */
	http_client_tx_wakeup(client);

	if (client->session.async && !stream->sending) {
		client_stream_finish(client, http_client_get_ops(client->version), stream);
	}

	/* Otherwise the thread in http_client_send_request() sees the completed
	 * flag and closes and releases the stream itself.
	 */
}

void http_client_stream_set_complete(struct http_client *client,
				     struct http_client_stream *stream, int result)
{
	/* A transaction ends once, e. g. when a reset sent on its behalf fails
	 * and takes the whole session down, and a released slot is no stream.
	 */
	if (!stream->is_used || stream->completed) {
		return;
	}

	/* Completed before the application is told, so that a disconnect from
	 * inside the callback finds nothing left to abort.
	 */
	stream->result = result;
	stream->completed = true;

	if (result < 0 && stream->request != NULL) {
		client_transaction_aborted(client, stream->request, &stream->response,
					   result, stream->user_data);
	}

	client_stream_completed(client, stream);
}

int http_client_stream_deliver(struct http_client *client, struct http_client_stream *stream,
			       bool final)
{
	const struct http_client_request *req = stream->request;
	int ret = 0;

	if (final) {
		/* Completed before the callback, so that a disconnect from
		 * inside it finds nothing left to abort.
		 */
		stream->result = 0;
		stream->completed = true;
	}

	if (req != NULL && req->response_cb != NULL) {
		ret = req->response_cb(client, req, &stream->response,
				       final ? HTTP_CLIENT_RESPONSE_DATA_FINAL :
					       HTTP_CLIENT_RESPONSE_DATA_MORE,
				       stream->user_data);
	}

	/* The chunk is only valid for the duration of the callback. */
	stream->response.headers = NULL;
	stream->response.header_count = 0;
	stream->response.body = NULL;
	stream->response.body_len = 0;

	if (!final) {
		return ret;
	}

	if (ret < 0) {
		/* The stream is over on the wire, so only the application is
		 * told that the transaction ended the way it asked for.
		 */
		stream->result = ret;
		client_transaction_aborted(client, req, &stream->response, ret,
					   stream->user_data);
	}

	client_stream_completed(client, stream);

	return 0;
}

void http_client_tx_wakeup(struct http_client *client)
{
	k_condvar_broadcast(&client->tx_cond);
}

void http_client_tx_acquire(struct http_client *client)
{
	k_mutex_lock(&client->tx_lock, K_FOREVER);
}

void http_client_tx_release(struct http_client *client)
{
	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->session.close_pending) {
		/* The session ended under this sender, which closes the
		 * transport, without the client lock as the close may wait.
		 */
		k_mutex_unlock(&client->lock);
		(void)client->transport_ops->close(client);
		k_mutex_lock(&client->lock, K_FOREVER);
		client->session.close_pending = false;
	}

	/* Released under the client lock, so that the receive context either
	 * finds the TX lock taken or finds nothing left over from its holder.
	 */
	k_mutex_unlock(&client->tx_lock);
	k_mutex_unlock(&client->lock);
}

void http_client_session_terminate(struct http_client *client, int reason, bool notify_peer)
{
	(void)http_client_disconnect_locked(client, reason, notify_peer);
}

static int client_process_rx_data(struct http_client *client,
				  const struct http_client_ops *http_ops, bool end_stream)
{
	struct http_client_session *session = &client->session;
	size_t offset = 0;
	int ret;

	/* Runs at least once, so that a stream ending without data reaches
	 * the protocol layer.
	 */
	do {
		ret = http_ops->process_data(client, session->rx_stream, session->rx_buf + offset,
					     session->rx_data_len - offset, end_stream);
		if (ret == -EAGAIN) {
			if (end_stream) {
				/* The stream ended inside a protocol unit, so the
				 * leftover can never be completed. Drop it rather
				 * than keep waiting on that stream.
				 */
				NET_WARN("Stream ended within a protocol unit, %zu bytes dropped",
					 session->rx_data_len - offset);
				offset = session->rx_data_len;
			}

			/* Otherwise wait for more data to be received. */
			break;
		} else if (ret < 0) {
			NET_ERR("Failed to process received data, err %d", ret);
			return ret;
		}

		if (session->state != HTTP_CLIENT_SESSION_OPEN) {
			/* The protocol layer ended the session. */
			session->rx_data_len = 0;
			return 0;
		}

		/* Otherwise ret holds the number of bytes consumed, and the
		 * protocol layer either consumes some or asks for more.
		 */
		if (ret == 0 && session->rx_data_len - offset > 0) {
			NET_ERR("No progress on received data");
			return -EPROTO;
		}

		offset += ret;
		NET_ASSERT(offset <= session->rx_data_len);
	} while (session->rx_data_len - offset > 0);

	/* Consume the processed data, keeping the leftover at the buffer start. */
	if (offset > 0) {
		session->rx_data_len -= offset;
		if (session->rx_data_len > 0) {
			memmove(session->rx_buf, session->rx_buf + offset,
				session->rx_data_len);
		}
	}

	return 0;
}

/* Read whatever the transport has available and process it. On any error the
 * session is torn down, which also notifies the owners of in-flight streams.
 * Must be called with the client lock held.
 */
static void client_receive_locked(struct http_client *client, int status)
{
	const struct http_client_ops *http_ops;
	struct http_client_session *session = &client->session;
	bool transport_ok = true;
	bool end_stream = false;
	size_t free_space;
	int ret;

	http_ops = http_client_get_ops(client->version);
	if (http_ops == NULL || http_ops->process_data == NULL) {
		NET_ERR("%s not supported", http_version_txt(client->version));
		ret = -ENOTSUP;
		goto abort;
	}

	if (status < 0) {
		/* Transport reported an error condition, abort without trying
		 * to read any data.
		 */
		NET_ERR("Transport reception error %d", status);
		ret = status;
		transport_ok = false;
		goto abort;
	}

	free_space = sizeof(session->rx_buf) - session->rx_data_len;
	if (free_space == 0) {
		/* Buffer is full and the implementation still needs more data
		 * to make progress. We cannot grow the buffer, so abort.
		 */
		NET_ERR("RX buffer too small to process incoming data");
		ret = -ENOBUFS;
		goto abort;
	}

	/* Read whatever is available into the free part of the buffer. The
	 * transport recv is always non-blocking. Data left over from a previous
	 * read belongs to one transport stream, so only that stream is read
	 * until the leftover is consumed.
	 */
	if (session->rx_data_len == 0) {
		session->rx_stream = NULL;
	}

	ret = client->transport_ops->recv(client, &session->rx_stream,
					  session->rx_buf + session->rx_data_len, free_space,
					  &end_stream);
	if (ret == -EAGAIN) {
		/* No data currently available, nothing to do. */
		return;
	} else if (ret < 0) {
		NET_ERR("Failed to receive data, err %d", ret);
		transport_ok = false;
		goto abort;
	} else if (ret == 0 && !end_stream) {
		NET_DBG("Connection closed by the peer");
		ret = -ENOTCONN;
		transport_ok = false;
		goto abort;
	}

	session->rx_data_len += ret;

	ret = client_process_rx_data(client, http_ops, end_stream);
	if (ret < 0) {
		goto abort;
	}

	if (session->rx_data_len == sizeof(session->rx_buf)) {
		/* No unit could be processed and the buffer is now full, so no
		 * additional data can ever be received. Abort the connection.
		 */
		NET_ERR("RX buffer too small to process incoming data");
		ret = -ENOBUFS;
		goto abort;
	}

	return;

abort:
	(void)http_client_disconnect_locked(client, ret, transport_ok);
}

/* Time to wait for a stream in one go: what is left until its deadline, capped
 * so that a session closed by another thread is noticed. Not positive once the
 * deadline has passed.
 */
static int64_t client_stream_wait_slice(const struct http_client_stream *stream)
{
	if (stream->timeout == INT64_MAX) {
		return HTTP_WAIT_SLICE_MS;
	}

	return MIN(stream->timeout - k_uptime_get(), HTTP_WAIT_SLICE_MS);
}

/* Receive on the calling thread, waiting for data for at most timeout_ms.
 * The client lock is released while waiting, so that another thread can
 * disconnect the session meanwhile. Must be called with the client lock held.
 */
static void client_sync_receive(struct http_client *client, struct http_client_stream *stream,
				int64_t timeout_ms)
{
	int ret;

	k_mutex_unlock(&client->lock);
	ret = client->transport_ops->wait(client, K_MSEC(timeout_ms));
	k_mutex_lock(&client->lock, K_FOREVER);

	if (stream->completed || ret == -EAGAIN) {
		return;
	}

	if (ret < 0) {
		NET_ERR("Transport wait error %d", ret);
		(void)http_client_disconnect_locked(client, ret, false);
	} else {
		client_receive_locked(client, 0);
	}

	/* Either may have ended the session; the close is done without the
	 * lock, which the caller holds.
	 */
	if (client->session.close_pending) {
		k_mutex_unlock(&client->lock);
		(void)client_transport_close(client);
		k_mutex_lock(&client->lock, K_FOREVER);
	}
}

/* Receive on the calling thread until the stream transaction completes or its
 * deadline expires. Must be called with the client lock held.
 */
static int http_stream_wait(struct http_client *client,
			    struct http_client_stream *stream)
{
	while (!stream->completed) {
		int64_t slice = client_stream_wait_slice(stream);

		if (slice <= 0) {
			return -ETIMEDOUT;
		}

		client_sync_receive(client, stream, slice);
	}

	return stream->result;
}

int http_client_stream_wait_tx(struct http_client *client, struct http_client_stream *stream)
{
	int64_t slice = client_stream_wait_slice(stream);

	if (slice <= 0) {
		return -ETIMEDOUT;
	}

	if (client->session.async) {
		/* The receive context makes the progress and signals it. */
		(void)k_condvar_wait(&client->tx_cond, &client->lock, K_MSEC(slice));
	} else {
		client_sync_receive(client, stream, slice);
	}

	if (stream->completed) {
		return (stream->result < 0) ? stream->result : -EALREADY;
	}

	return 0;
}

/* Public API */

int http_client_init(struct http_client *client, enum http_client_version version,
		     const struct http_client_transport_ops *transport_ops,
		     void *transport_ctx)
{
	if (client == NULL || transport_ops == NULL) {
		return -EINVAL;
	}

	if (http_client_get_ops(version) == NULL) {
		NET_ERR("%s not supported", http_version_txt(version));
		return -ENOTSUP;
	}

	if (transport_ops->init != NULL) {
		transport_ops->init(transport_ctx);
	}

	NET_ASSERT(transport_ops->connect != NULL);
	NET_ASSERT(transport_ops->send != NULL);
	NET_ASSERT(transport_ops->recv != NULL);
	NET_ASSERT(transport_ops->wait != NULL);
	NET_ASSERT(transport_ops->close != NULL);

	memset(client, 0, sizeof(*client));

	client->transport_ops = transport_ops;
	client->transport_ctx = transport_ctx;
	client->version = version;
	client->session.state = HTTP_CLIENT_SESSION_CLOSED;

	client->callers = 0;
	k_mutex_init(&client->lock);
	k_mutex_init(&client->tx_lock);
	k_condvar_init(&client->tx_cond);
	k_work_init_delayable(&client->timeout_work, http_client_timeout_work);

	return 0;
}

int http_client_connect(struct http_client *client,
			struct http_client_session_config *config)
{
	const struct http_client_ops *http_ops;
	bool close = false;
	int ret;

	if (client == NULL || config == NULL) {
		return -EINVAL;
	}

	if (config->addrlen == 0 || config->addrlen > sizeof(config->server_addr)) {
		NET_ERR("Invalid server address length %u", config->addrlen);
		return -EINVAL;
	}

	if (client->transport_ops == NULL) {
		NET_ERR("Client transport not initialized");
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->session.state != HTTP_CLIENT_SESSION_CLOSED) {
		NET_ERR("Client HTTP session already open");
		ret = -EALREADY;
		goto out;
	}

	/* A thread still sending a request of the previous session owns its
	 * stream slot and maybe the transport; both are needed afresh.
	 */
	if (http_session_has_streams(&client->session) || client->session.close_pending) {
		NET_ERR("A request of the previous session is still being sent");
		ret = -EBUSY;
		goto out;
	}

	http_ops = http_client_get_ops(client->version);
	if (http_ops == NULL) {
		NET_ERR("%s not supported", http_version_txt(client->version));
		ret = -EINVAL;
		goto out;
	}

	if (config->host != NULL && !http_client_host_is_valid(config->host)) {
		NET_ERR("Invalid session host");
		ret = -EINVAL;
		goto out;
	}

	http_session_init(&client->session, config);

	if (config->async) {
		/* Only asynchronous sessions need the deadline work queue. */
		http_client_wq_start();
	}

	ret = client->transport_ops->connect(client);
	if (ret != 0) {
		NET_ERR("Failed to connect to the server, err %d", ret);
		goto out;
	}

	ret = http_ops->session_open(client);
	if (ret != 0) {
		NET_ERR("Failed to start a new session, err %d", ret);

		/* The transport may already have a notification waiting for
		 * the client lock, which its close waits for in turn.
		 */
		if (client->transport_ops->stop != NULL) {
			client->transport_ops->stop(client);
		}

		client->session.close_pending = true;
		close = true;
		goto out;
	}

	client->session.state = HTTP_CLIENT_SESSION_OPEN;

out:
	k_mutex_unlock(&client->lock);

	if (close) {
		(void)client_transport_close(client);
	}

	return ret;
}

void http_client_received_data(struct http_client *client, int status)
{
	if (client == NULL) {
		return;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->session.state != HTTP_CLIENT_SESSION_OPEN) {
		NET_DBG("Client HTTP session closed");
	} else {
		client_receive_locked(client, status);
	}

	k_mutex_unlock(&client->lock);

	/* The received data may have ended the session. */
	(void)client_transport_close(client);
}

int http_client_send_request(struct http_client *client,
			     const struct http_client_request *request,
			     int32_t timeout, void *user_data)
{
	const struct http_client_ops *http_ops = NULL;
	struct http_client_stream *stream = NULL;
	bool headers_sent = false;
	bool has_payload;
	int ret;

	if (client == NULL || request == NULL) {
		return -EINVAL;
	}

	if (client->transport_ops == NULL) {
		NET_ERR("Client transport not initialized");
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->session.state != HTTP_CLIENT_SESSION_OPEN) {
		NET_ERR("Client HTTP session closed");
		ret = -ENOTCONN;
		goto error;
	}

	http_ops = http_client_get_ops(client->version);
	if (http_ops == NULL) {
		NET_ERR("%s not supported", http_version_txt(client->version));
		ret = -EINVAL;
		goto error;
	}

	/* A synchronous session serves one request at a time: the thread that
	 * sent it receives the response, and the callbacks run on it.
	 */
	if (!client->session.async && http_session_has_streams(&client->session)) {
		NET_ERR("A request is already in flight");
		ret = -EBUSY;
		goto error;
	}

	stream = http_stream_alloc(&client->session);
	if (stream == NULL) {
		NET_ERR("Failed to allocate new HTTP stream");
		ret = -ENOMEM;
		goto error;
	}

	http_stream_init(stream, request, timeout, user_data);
	stream->sending = true;
	stream->caller = k_current_get();
	client->callers++;

	if (http_ops->stream_open != NULL) {
		ret = http_ops->stream_open(client, stream);
		if (ret != 0) {
			NET_ERR("Failed to open a new stream, err %d", ret);
			goto error;
		}
	}

	has_payload = request->payload_cb != NULL ||
		      (request->payload != NULL && request->payload_len > 0);

	/* The deadline covers the sending as well. */
	if (client->session.async) {
		http_client_timeout_reschedule(client);
	}

	/* The request goes out without the lock, so that the receive context
	 * keeps serving the session meanwhile; the sending flag keeps the
	 * stream allocated. The protocol layer takes the lock as it needs it.
	 */
	k_mutex_unlock(&client->lock);

	ret = http_ops->stream_send_headers(client, stream, request, !has_payload);
	if (ret != 0) {
		NET_ERR("Failed to send headers, err %d", ret);
		k_mutex_lock(&client->lock, K_FOREVER);
		goto error;
	}

	headers_sent = true;

	if (has_payload) {
		ret = client_send_payload(client, stream, request, http_ops);
		if (ret != 0) {
			NET_ERR("Failed to send payload, err %d", ret);
			k_mutex_lock(&client->lock, K_FOREVER);
			goto error;
		}
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	stream->sending = false;

	if (client->session.async) {
		/* In asynchronous mode we return without waiting; the RX path
		 * completes the transaction and releases the stream later. The
		 * response is delivered via the application callback, same as in
		 * synchronous mode.
		 */
		stream->caller = NULL;

		if (stream->completed) {
			/* It ended while the request was being sent, which the
			 * call reports as well.
			 */
			ret = (stream->result < 0) ? stream->result : 0;
			client_stream_finish(client, http_ops, stream);
		}

		goto out;
	}

	/* In synchronous mode, receive on this thread until the transaction
	 * completes or the deadline expires.
	 */
	ret = http_stream_wait(client, stream);
	if (ret < 0 && !stream->completed) {
		/* Timed out with the stream still open on the wire: tell the
		 * server to drop it and notify the application the same way as
		 * for any other aborted transaction.
		 */
		NET_ERR("Transaction timed out");

		if (http_ops->stream_reset != NULL) {
			(void)http_ops->stream_reset(client, stream);
		}

		/* Unless the reset failed and ended the transaction already,
		 * reported with that error. Completed before the application is
		 * told, so that a disconnect from the callback finds nothing
		 * left to abort.
		 */
		if (stream->completed) {
			ret = stream->result;
		} else {
			http_client_stream_set_complete(client, stream, ret);
		}
	} else if (ret < 0) {
		NET_ERR("Transaction did not complete, err %d", ret);
	}

	client_stream_finish(client, http_ops, stream);

out:
	k_mutex_unlock(&client->lock);

	/* A failed reset or a timed out send may have ended the session. */
	(void)client_transport_close(client);

	client_leave(client);

	return ret;

error:
	if (stream != NULL && stream->completed) {
		/* The receive path ended the transaction while the request was
		 * being sent, and has reported it already.
		 */
		NET_ERR("Transaction did not complete, err %d", ret);
	} else {
		/* Let the server know the stream is abandoned, but only if the
		 * request was already put on the wire. A stream with no headers
		 * sent is still idle, and resetting it would be a protocol
		 * violation.
		 */
		if (headers_sent && http_ops->stream_reset != NULL) {
			(void)http_ops->stream_reset(client, stream);
		}

		/* Unless the reset failed and ended the transaction already. A
		 * stream is completed before the application is told, so that a
		 * disconnect from the callback finds nothing left to abort.
		 */
		if (stream != NULL) {
			http_client_stream_set_complete(client, stream, ret);
		}
	}

	if (stream != NULL) {
		http_stream_release(&client->session, stream);
	}

	k_mutex_unlock(&client->lock);

	if (stream == NULL) {
		/* Refused before a stream was taken: reported with the lock
		 * released, as the callback may disconnect and wait for the
		 * other callers.
		 */
		client_transaction_aborted(client, request, NULL, ret, user_data);
	}

	(void)client_transport_close(client);

	if (stream != NULL) {
		client_leave(client);
	}

	return ret;
}

int http_client_disconnect(struct http_client *client)
{
	struct k_work_sync sync;
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	if (client->transport_ops == NULL) {
		NET_ERR("Client transport not initialized");
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	ret = http_client_disconnect_locked(client, 0, true);

	k_mutex_unlock(&client->lock);

	if (ret == 0) {
		ret = client_transport_close(client);
	}

	/* A deadline handler already running waits for the lock released
	 * above; let it finish before the client may be reused or freed. The
	 * same for a thread still sending a request, which closes the
	 * transport left to it before giving the TX lock up.
	 */
	(void)k_work_cancel_delayable_sync(&client->timeout_work, &sync);

	k_mutex_lock(&client->tx_lock, K_FOREVER);
	k_mutex_unlock(&client->tx_lock);

	/* The other threads still inside http_client_send_request() may touch
	 * the client until they leave; wait for them. The caller itself may be
	 * one of them, in a callback of a synchronous session, and returns on
	 * its own.
	 */
	k_mutex_lock(&client->lock, K_FOREVER);

	while (client->callers > (http_session_caller_is_current(&client->session) ? 1U : 0U)) {
		(void)k_condvar_wait(&client->tx_cond, &client->lock, K_MSEC(HTTP_WAIT_SLICE_MS));
	}

	k_mutex_unlock(&client->lock);

	return ret;
}
