/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include "ssh_connection.h"

struct ssh_channel *ssh_connection_allocate_channel(struct ssh_transport *transport)
{
	struct ssh_channel *channel = NULL;
	uint32_t local_channel = UINT32_MAX;

	for (size_t i = 0; i < ARRAY_SIZE(transport->channels); i++) {
		if (!transport->channels[i].in_use) {
			channel = &transport->channels[i];
			local_channel = i;
			break;
		}
	}

	if (channel != NULL) {
		memset(channel, 0, sizeof(*channel));

		channel->transport = transport;
		channel->local_channel = local_channel;
		channel->remote_channel = UINT32_MAX;

		ring_buf_init(&channel->tx_ring_buf,
			      sizeof(channel->tx_ring_buf_data),
			      channel->tx_ring_buf_data);
		ring_buf_init(&channel->tx_stderr_ring_buf,
			      sizeof(channel->tx_stderr_ring_buf_data),
			      channel->tx_stderr_ring_buf_data);
		ring_buf_init(&channel->rx_ring_buf,
			      sizeof(channel->rx_ring_buf_data),
			      channel->rx_ring_buf_data);
		ring_buf_init(&channel->rx_stderr_ring_buf,
			      sizeof(channel->rx_stderr_ring_buf_data),
			      channel->rx_stderr_ring_buf_data);
		channel->open = false;
		channel->in_use = true;
	}

	return channel;
}

void ssh_connection_free_channel(struct ssh_channel *channel)
{
	if (channel->callback != NULL && channel->open) {
		struct ssh_channel_event event;

		event.type = SSH_CHANNEL_EVENT_CLOSED;
		channel->callback(channel, &event, channel->user_data);
	}

	channel->open = false;
	channel->in_use = false;
}

struct ssh_channel *ssh_connection_get_channel(struct ssh_transport *transport,
					       uint32_t local_channel)
{
	struct ssh_channel *channel = NULL;

	for (int i = 0; i < ARRAY_SIZE(transport->channels); i++) {
		if (transport->channels[i].in_use &&
		    transport->channels[i].local_channel == local_channel) {
			channel = &transport->channels[i];
			break;
		}
	}

	return channel;
}

#ifdef CONFIG_SSH_SERVER
int ssh_channel_open_result(struct ssh_channel *channel, bool success,
			    ssh_channel_event_callback_t callback, void *user_data)
{
	struct ssh_transport_user_request request;
	struct ssh_transport *transport;

	if (channel == NULL || !channel->in_use || callback == NULL) {
		return -EINVAL;
	}

	transport = channel->transport;
	if (transport == NULL || !transport->running ||
	    !transport->authenticated || !transport->server) {
		return -EINVAL;
	}

	request.type = SSH_TRANSPORT_USER_REQUEST_OPEN_CHANNEL_RESULT;
	request.open_channel_result.channel = channel;
	request.open_channel_result.success = success;
	request.open_channel_result.callback = callback;
	request.open_channel_result.user_data = user_data;

	return ssh_transport_queue_user_request(transport, &request);
}
#endif

int ssh_channel_request_result(struct ssh_channel *channel, bool success)
{
	struct ssh_transport_user_request request;
	struct ssh_transport *transport;

	if (channel == NULL || !channel->in_use) {
		return -EINVAL;
	}

	transport = channel->transport;
	if (transport == NULL || !transport->running || !transport->authenticated) {
		return -EINVAL;
	}

	request.type = SSH_TRANSPORT_USER_REQUEST_CHANNEL_REQUEST_RESULT;
	request.channel_request_result.channel = channel;
	request.channel_request_result.success = success;

	return ssh_transport_queue_user_request(transport, &request);
}

#ifdef CONFIG_SSH_CLIENT
int ssh_channel_request_shell(struct ssh_channel *channel)
{
	struct ssh_transport_user_request request;
	struct ssh_transport *transport;

	if (channel == NULL || !channel->in_use) {
		return -EINVAL;
	}

	transport = channel->transport;
	if (transport == NULL || !transport->running ||
	    !transport->authenticated || transport->server) {
		return -EINVAL;
	}

	request.type = SSH_TRANSPORT_USER_REQUEST_CHANNEL_REQUEST;
	request.channel_request.channel = channel;
	request.channel_request.type = SSH_CHANNEL_REQUEST_SHELL;
	request.channel_request.want_reply = true;

	return ssh_transport_queue_user_request(transport, &request);
}
#endif

int ssh_channel_read(struct ssh_channel *channel, void *data, uint32_t len)
{
	if (channel == NULL || !channel->in_use || !channel->open ||
	    channel->remote_channel == UINT32_MAX || len > INT_MAX) {
		return -EINVAL;
	}

	len = ring_buf_get(&channel->rx_ring_buf, data, len);

	if (channel->rx_window_rem == 0) {
		/* Wake up the thread */
		(void)ssh_transport_wakeup(channel->transport);
	}

	return (int)len;
}

int ssh_channel_write(struct ssh_channel *channel, const void *data, uint32_t len)
{
	if (channel == NULL || !channel->in_use || !channel->open ||
	    channel->remote_channel == UINT32_MAX || len > INT_MAX) {
		return -EINVAL;
	}

	len = ring_buf_put(&channel->tx_ring_buf, data, len);

	/* Wake up the thread */
	(void)ssh_transport_wakeup(channel->transport);

	return (int)len;
}

int ssh_channel_read_stderr(struct ssh_channel *channel, void *data, uint32_t len)
{
	if (channel == NULL || !channel->in_use || !channel->open ||
	    channel->remote_channel == UINT32_MAX || len > INT_MAX) {
		return -EINVAL;
	}

	len = ring_buf_get(&channel->rx_stderr_ring_buf, data, len);

	if (channel->rx_window_rem == 0) {
		/* Wake up the thread */
		(void)ssh_transport_wakeup(channel->transport);
	}

	return (int)len;
}

int ssh_channel_write_stderr(struct ssh_channel *channel, const void *data, uint32_t len)
{
	if (channel == NULL || !channel->in_use || !channel->open ||
	    channel->remote_channel == UINT32_MAX || len > INT_MAX) {
		return -EINVAL;
	}

	len = ring_buf_put(&channel->tx_stderr_ring_buf, data, len);

	/* Wake up the thread */
	(void)ssh_transport_wakeup(channel->transport);

	return (int)len;
}

int ssh_connection_process_msg(struct ssh_transport *transport, uint8_t msg_id,
			       struct ssh_payload *rx_pkt)
{
	int ret = -1;

	switch (msg_id) {
	case SSH_MSG_GLOBAL_REQUEST: {
		NET_DBG("GLOBAL_REQUEST");
		struct ssh_string request_name;
		bool want_reply;

		if (!ssh_payload_read_string(rx_pkt, &request_name) ||
		    !ssh_payload_read_bool(rx_pkt, &want_reply)) {
			NET_ERR("Length error");
			return -1;
		}

		/* TODO: ssh_payload_read_complete for different request types */
		NET_DBG("GLOBAL_REQUEST: %.*s want reply: %s", request_name.len,
			request_name.data, want_reply ? "true" : "false");
		ret = 0;
		break;
	}
	case SSH_MSG_REQUEST_SUCCESS: {
		NET_DBG("REQUEST_SUCCESS");
		ret = 0;
		break;
	}
	case SSH_MSG_REQUEST_FAILURE: {
		NET_DBG("REQUEST_FAILURE");
		ret = 0;
		break;
	}
#ifdef CONFIG_SSH_SERVER
	case SSH_MSG_CHANNEL_OPEN: {
		static const struct ssh_string supported_channel_type =
			SSH_STRING_LITERAL("session");
		struct ssh_string channel_type;
		uint32_t sender_channel, initial_window_size, maximum_packet_size;
		struct ssh_channel *channel;

		NET_DBG("CHANNEL_OPEN");

		/* Server only */
		if (!transport->server) {
			break;
		}

		if (!ssh_payload_read_string(rx_pkt, &channel_type) ||
		    !ssh_payload_read_u32(rx_pkt, &sender_channel) ||
		    !ssh_payload_read_u32(rx_pkt, &initial_window_size) ||
		    !ssh_payload_read_u32(rx_pkt, &maximum_packet_size) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		if (!ssh_strings_equal(&channel_type, &supported_channel_type)) {
			NET_DBG("Unsupported channel type");
			return -1;
		}

		channel = ssh_connection_allocate_channel(transport);
		if (channel == NULL) {
			NET_ERR("Failed to alloc channel");
			return -1;
		}

		/* Save remote channel */
		channel->remote_channel = sender_channel;
		channel->tx_window_rem = initial_window_size;
		channel->tx_mtu = maximum_packet_size;
		channel->rx_window_rem = sizeof(channel->rx_ring_buf_data);
		/* TODO: MTU = sizeof(transport->rx_buf) - HEADER_LEN - MAX_PADDING - MAC_LEN? */
		channel->rx_mtu = sizeof(channel->rx_ring_buf_data);

		if (transport->callback != NULL) {
			struct ssh_transport_event event;

			event.type = SSH_TRANSPORT_EVENT_CHANNEL_OPEN;
			event.channel_open.channel = channel;
			ret = transport->callback(transport, &event,
						  transport->callback_user_data);
		}
		break;
	}
#endif
	case SSH_MSG_CHANNEL_REQUEST: {
		NET_DBG("CHANNEL_REQUEST");
		uint32_t recipient_channel;
		struct ssh_string request_type;
		struct ssh_channel *channel;
		struct ssh_channel_event event;
		enum ssh_channel_request_type req_type;
		bool want_reply;
		static const struct {
			enum ssh_channel_request_type type;
			struct ssh_string string;
		} channel_type_lut[] = {
			{
				.type = SSH_CHANNEL_REQUEST_PSEUDO_TERMINAL,
				.string = SSH_STRING_LITERAL("pty-req")
			},
			{
				.type = SSH_CHANNEL_REQUEST_X11_FORWARD,
				.string = SSH_STRING_LITERAL("x11-req")
			},
			{
				.type = SSH_CHANNEL_REQUEST_ENV_VAR,
				.string = SSH_STRING_LITERAL("env")
			},
			{
				.type = SSH_CHANNEL_REQUEST_SHELL,
				.string = SSH_STRING_LITERAL("shell")
			},
			{
				.type = SSH_CHANNEL_REQUEST_EXEC,
				.string = SSH_STRING_LITERAL("exec")
			},
			{
				.type = SSH_CHANNEL_REQUEST_SUBSYSTEM,
				.string = SSH_STRING_LITERAL("subsystem")
			},
			{
				.type = SSH_CHANNEL_REQUEST_WINDOW_CHANGE,
				.string = SSH_STRING_LITERAL("window-change")
			},
			{
				.type = SSH_CHANNEL_REQUEST_FLOW_CONTROL,
				.string = SSH_STRING_LITERAL("xon-xoff")
			},
			{
				.type = SSH_CHANNEL_REQUEST_SIGNAL,
				.string = SSH_STRING_LITERAL("signal")
			},
			{
				.type = SSH_CHANNEL_REQUEST_EXIT_STATUS,
				.string = SSH_STRING_LITERAL("exit-status")
			},
			{
				.type = SSH_CHANNEL_REQUEST_EXIT_SIGNAL,
				.string = SSH_STRING_LITERAL("exit-signal")
			}
		};

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_string(rx_pkt, &request_type) ||
		    !ssh_payload_read_bool(rx_pkt, &want_reply)) {
			NET_ERR("Length error");
			return -1;
		}

		/* TODO: ssh_payload_read_complete for different channel types */
		/* TODO: Some types of requests are server only */

		NET_DBG("CHANNEL_REQUEST: %u %.*s want reply: %s",
			recipient_channel, request_type.len, request_type.data,
			want_reply ? "true" : "false");

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		req_type = SSH_CHANNEL_REQUEST_UNKNOWN;
		for (int i = 0; i < ARRAY_SIZE(channel_type_lut); i++) {
			if (ssh_strings_equal(&request_type, &channel_type_lut[i].string)) {
				req_type = channel_type_lut[i].type;
				break;
			}
		}

		if (req_type == SSH_CHANNEL_REQUEST_PSEUDO_TERMINAL) {
			struct ssh_string term_env, term_encoded_modes;
			uint32_t term_width_chars, term_height_rows,
				term_width_pixels, term_height_pixels;
			if (!ssh_payload_read_string(rx_pkt, &term_env) ||
			    !ssh_payload_read_u32(rx_pkt, &term_width_chars) ||
			    !ssh_payload_read_u32(rx_pkt, &term_height_rows) ||
			    !ssh_payload_read_u32(rx_pkt, &term_width_pixels) ||
			    !ssh_payload_read_u32(rx_pkt, &term_height_pixels) ||
			    !ssh_payload_read_string(rx_pkt, &term_encoded_modes) ||
			    !ssh_payload_read_complete(rx_pkt)) {
				NET_ERR("Length error");
				return -1;
			}
			NET_DBG("PSEUDO_TERMINAL: %.*s %u %u %u %u", term_env.len, term_env.data,
				term_width_chars, term_height_rows, term_width_pixels,
				term_height_pixels);
			NET_HEXDUMP_DBG(term_encoded_modes.data, term_encoded_modes.len,
					"PSEUDO_TERMINAL modes:");
		}

		event.type = SSH_CHANNEL_EVENT_REQUEST;
		event.channel_request.type = req_type;
		event.channel_request.want_reply = want_reply;
		channel->callback(channel, &event, channel->user_data);

		ret = 0;
		break;
	}
#ifdef CONFIG_SSH_CLIENT
	case SSH_MSG_CHANNEL_OPEN_CONFIRMATION: {
		uint32_t recipient_channel, sender_channel,
			initial_window_size, maximum_packet_size;
		struct ssh_channel_event event;
		struct ssh_channel *channel;

		NET_DBG("CHANNEL_OPEN_CONFIRMATION");

		/* Client only */
		if (transport->server) {
			break;
		}

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_u32(rx_pkt, &sender_channel) ||
		    !ssh_payload_read_u32(rx_pkt, &initial_window_size) ||
		    !ssh_payload_read_u32(rx_pkt, &maximum_packet_size)) {
			NET_ERR("Length error");
			return -1;
		}

		/* TODO: ssh_payload_read_complete for different channel types */

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		/* Save remote channel */
		channel->remote_channel = sender_channel;
		channel->tx_window_rem = initial_window_size;
		channel->tx_mtu = maximum_packet_size;

		channel->open = true;

		event.type = SSH_CHANNEL_EVENT_OPEN_RESULT;
		channel->callback(channel, &event, channel->user_data);

		ret = 0;
		break;
	}
	case SSH_MSG_CHANNEL_OPEN_FAILURE: {
		uint32_t recipient_channel, reason_code;
		struct ssh_string  description, language_tag;
		struct ssh_channel *channel;

		NET_DBG("CHANNEL_OPEN_FAILURE");

		/* Client only */
		if (transport->server) {
			break;
		}

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_u32(rx_pkt, &reason_code) ||
		    !ssh_payload_read_string(rx_pkt, &description) ||
		    !ssh_payload_read_string(rx_pkt, &language_tag) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		ssh_connection_free_channel(channel);

		ret = -1;
		break;
	}
#endif
	case SSH_MSG_CHANNEL_SUCCESS: {
		uint32_t recipient_channel;
		struct ssh_channel *channel;
		struct ssh_channel_event event;

		NET_DBG("CHANNEL_SUCCESS");

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		event.type = SSH_CHANNEL_EVENT_REQUEST_RESULT;
		event.channel_request_result.success = true;
		channel->callback(channel, &event, channel->user_data);

		ret = 0;
		break;
	}
	case SSH_MSG_CHANNEL_FAILURE: {
		struct ssh_channel_event event;
		struct ssh_channel *channel;
		uint32_t recipient_channel;

		NET_DBG("CHANNEL_FAILURE");

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		event.type = SSH_CHANNEL_EVENT_REQUEST_RESULT;
		event.channel_request_result.success = false;
		channel->callback(channel, &event, channel->user_data);

		ret = 0;
		break;
	}
	case SSH_MSG_CHANNEL_WINDOW_ADJUST: {
		uint32_t recipient_channel, bytes_to_add;
		struct ssh_channel *channel;

		NET_DBG("CHANNEL_WINDOW_ADJUST");

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_u32(rx_pkt, &bytes_to_add) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		channel->tx_window_rem += bytes_to_add;

		ret = 0;
		break;
	}
	case SSH_MSG_CHANNEL_DATA: {
		struct ssh_channel_event event;
		struct ssh_channel *channel;
		uint32_t recipient_channel;
		struct ssh_string data;
		uint32_t written;

		NET_DBG("CHANNEL_DATA");

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_string(rx_pkt, &data) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		NET_HEXDUMP_DBG(data.data, data.len, "Data:");

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		channel->rx_window_rem -= MIN(channel->rx_window_rem, data.len);

		written = ring_buf_put(&channel->rx_ring_buf, data.data, data.len);
		if (written < data.len) {
			NET_ERR("Flow control error");
			return -1;
		}

		event.type = SSH_CHANNEL_EVENT_RX_DATA_READY;
		if (channel->callback != NULL) {
			channel->callback(channel, &event, channel->user_data);
		}

		ret = 0;
		break;
	}
	case SSH_MSG_CHANNEL_EXTENDED_DATA: {
		uint32_t recipient_channel, data_type_code;
		struct ssh_channel *channel;
		struct ssh_string data;

		NET_DBG("CHANNEL_EXTENDED_DATA");

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_u32(rx_pkt, &data_type_code) ||
		    !ssh_payload_read_string(rx_pkt, &data) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		NET_HEXDUMP_DBG(data.data, data.len, "Data ext:");

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		channel->rx_window_rem -= MIN(channel->rx_window_rem, data.len);

		if (data_type_code == SSH_EXTENDED_DATA_STDERR) {
			struct ssh_channel_event event;
			uint32_t written = ring_buf_put(&channel->rx_stderr_ring_buf,
							data.data, data.len);

			if (written < data.len) {
				NET_ERR("Flow control error");
				return -1;
			}

			event.type = SSH_CHANNEL_EVENT_RX_STDERR_DATA_READY;
			if (channel->callback != NULL) {
				channel->callback(channel, &event, channel->user_data);
			}
		}

		ret = 0;
		break;
	}
	case SSH_MSG_CHANNEL_EOF: {
		uint32_t recipient_channel;
		struct ssh_channel *channel;
		struct ssh_channel_event event;

		NET_DBG("CHANNEL_EOF");

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		event.type = SSH_CHANNEL_EVENT_EOF;
		if (channel->callback != NULL) {
			channel->callback(channel, &event, channel->user_data);
		}

		ret = 0;
		break;
	}
	case SSH_MSG_CHANNEL_CLOSE: {
		uint32_t recipient_channel;
		struct ssh_channel *channel;

		NET_DBG("CHANNEL_CLOSE");

		if (!ssh_payload_read_u32(rx_pkt, &recipient_channel) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		channel = ssh_connection_get_channel(transport, recipient_channel);
		if (channel == NULL) {
			NET_ERR("Invalid channel");
			return -1;
		}

		ssh_connection_free_channel(channel);

		ret = 0;
		break;
	}
	default:
		break;
	}

	return ret;
}

#ifdef CONFIG_SSH_CLIENT
int ssh_connection_send_channel_open(struct ssh_transport *transport,
				     const struct ssh_string *channel_type,
				     uint32_t sender_channel,
				     uint32_t initial_window_size,
				     uint32_t maximum_packet_size)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_CHANNEL_OPEN) ||
	    !ssh_payload_write_string(&payload, channel_type) ||
	    !ssh_payload_write_u32(&payload, sender_channel) ||
	    !ssh_payload_write_u32(&payload, initial_window_size) ||
	    !ssh_payload_write_u32(&payload, maximum_packet_size)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_OPEN", ret);
	}
	return ret;
}
#endif

int ssh_connection_send_channel_request_pseudo_terminal(struct ssh_transport *transport,
							uint32_t recipient_channel,
							bool want_reply)
{
	const struct ssh_string channel_type = SSH_STRING_LITERAL("pty-req");
	const struct ssh_string term_env = SSH_STRING_LITERAL("vt100");
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	/* Lazily copied from what openssh sends out... */
	static const uint8_t term_modes_blob[] = {
		0x81, 0x00, 0x00, 0x96, 0x00, 0x80, 0x00, 0x00, 0x96, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x03, 0x02, 0x00, 0x00, 0x00, 0x1c, 0x03, 0x00, 0x00, 0x00, 0x7f, 0x04, 0x00, 0x00,
		0x00, 0x15, 0x05, 0x00, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00,
		0x00, 0x00, 0xff, 0x08, 0x00, 0x00, 0x00, 0x11, 0x09, 0x00, 0x00, 0x00, 0x13, 0x0a,
		0x00, 0x00, 0x00, 0x1a, 0x0c, 0x00, 0x00, 0x00, 0x12, 0x0d, 0x00, 0x00, 0x00, 0x17,
		0x0e, 0x00, 0x00, 0x00, 0x16, 0x12, 0x00, 0x00, 0x00, 0x0f, 0x1e, 0x00, 0x00, 0x00,
		0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00,
		0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00,
		0x00, 0x00, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x01, 0x27,
		0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00,
		0x2a, 0x00, 0x00, 0x00, 0x01, 0x32, 0x00, 0x00, 0x00, 0x01, 0x33, 0x00, 0x00, 0x00,
		0x01, 0x34, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x01, 0x36, 0x00, 0x00,
		0x00, 0x01, 0x37, 0x00, 0x00, 0x00, 0x01, 0x38, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00,
		0x00, 0x00, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x01, 0x3c,
		0x00, 0x00, 0x00, 0x01, 0x3d, 0x00, 0x00, 0x00, 0x01, 0x3e, 0x00, 0x00, 0x00, 0x00,
		0x46, 0x00, 0x00, 0x00, 0x01, 0x47, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00,
		0x01, 0x49, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00,
		0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x01, 0x5b, 0x00, 0x00, 0x00, 0x01, 0x5c, 0x00,
		0x00, 0x00, 0x00, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	const struct ssh_string term_encoded_modes = {
		.len = sizeof(term_modes_blob),
		.data = term_modes_blob
	};
	uint32_t term_width_chars = CONFIG_SSH_DEFAULT_TERMINAL_WIDTH,
		term_height_rows = CONFIG_SSH_DEFAULT_TERMINAL_HEIGHT,
		term_width_pixels = 0, term_height_pixels = 0;

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_CHANNEL_REQUEST) ||
	    !ssh_payload_write_u32(&payload, recipient_channel) ||
	    !ssh_payload_write_string(&payload, &channel_type) ||
	    !ssh_payload_write_bool(&payload, want_reply) ||
	    !ssh_payload_write_string(&payload, &term_env) ||
	    !ssh_payload_write_u32(&payload, term_width_chars) ||
	    !ssh_payload_write_u32(&payload, term_height_rows) ||
	    !ssh_payload_write_u32(&payload, term_width_pixels) ||
	    !ssh_payload_write_u32(&payload, term_height_pixels) ||
	    !ssh_payload_write_string(&payload, &term_encoded_modes)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_REQUEST", ret);
	}

	return ret;
}

int ssh_connection_send_channel_request(struct ssh_transport *transport,
					uint32_t recipient_channel,
					const struct ssh_string *channel_type,
					bool want_reply)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_CHANNEL_REQUEST) ||
	    !ssh_payload_write_u32(&payload, recipient_channel) ||
	    !ssh_payload_write_string(&payload, channel_type) ||
	    !ssh_payload_write_bool(&payload, want_reply)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_REQUEST", ret);
	}

	return ret;
}

int ssh_connection_send_channel_data(struct ssh_transport *transport,
				     uint32_t recipient_channel,
				     const void *data, uint32_t data_len)
{
	struct ssh_string data_str = {
		.len = data_len,
		.data = data
	};
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_CHANNEL_DATA) ||
	    !ssh_payload_write_u32(&payload, recipient_channel) ||
	    !ssh_payload_write_string(&payload, &data_str)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_DATA", ret);
	}

	return ret;
}

int ssh_connection_send_channel_extended_data(struct ssh_transport *transport,
					      uint32_t recipient_channel,
					      uint32_t data_type_code,
					      const void *data, uint32_t data_len)
{
	struct ssh_string data_str = {
		.len = data_len,
		.data = data
	};
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_CHANNEL_EXTENDED_DATA) ||
	    !ssh_payload_write_u32(&payload, recipient_channel) ||
	    !ssh_payload_write_u32(&payload, data_type_code) ||
	    !ssh_payload_write_string(&payload, &data_str)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_EXTENDED_DATA", ret);
	}

	return ret;
}

int ssh_connection_send_window_adjust(struct ssh_transport *transport,
				      uint32_t recipient_channel,
				      uint32_t bytes_to_add)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_CHANNEL_WINDOW_ADJUST) ||
	    !ssh_payload_write_u32(&payload, recipient_channel) ||
	    !ssh_payload_write_u32(&payload, bytes_to_add)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_WINDOW_ADJUST", ret);
	}

	return ret;
}

#ifdef CONFIG_SSH_SERVER
int ssh_connection_send_channel_open_confirmation(struct ssh_transport *transport,
						  uint32_t recipient_channel,
						  uint32_t sender_channel,
						  uint32_t initial_window_size,
						  uint32_t maximum_packet_size)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_CHANNEL_OPEN_CONFIRMATION) ||
	    !ssh_payload_write_u32(&payload, recipient_channel) ||
	    !ssh_payload_write_u32(&payload, sender_channel) ||
	    !ssh_payload_write_u32(&payload, initial_window_size) ||
	    !ssh_payload_write_u32(&payload, maximum_packet_size)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_OPEN_CONFIRMATION", ret);
	}

	return ret;
}
#endif

int ssh_connection_send_channel_result(struct ssh_transport *transport, bool success,
				       uint32_t recipient_channel)
{
	uint8_t msg_id = success ? SSH_MSG_CHANNEL_SUCCESS : SSH_MSG_CHANNEL_FAILURE;
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, msg_id) ||
	    !ssh_payload_write_u32(&payload, recipient_channel)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "CHANNEL_SUCCESS", ret);
	}

	return ret;
}
