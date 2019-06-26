/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_nats_sample, LOG_LEVEL_DBG);

#include <ctype.h>
#include <errno.h>
#include <data/json.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#include "nats.h"

#define CMD_BUF_LEN 256

struct nats_info {
	const char *server_id;
	const char *version;
	const char *go;
	const char *host;
	size_t max_payload;
	u16_t port;
	bool ssl_required;
	bool auth_required;
};

struct io_vec {
	const void *base;
	size_t len;
};

static bool is_subject_valid(const char *subject, size_t len)
{
	size_t pos;
	char last = '\0';

	if (!subject) {
		return false;
	}

	for (pos = 0; pos < len; last = subject[pos++]) {
		switch (subject[pos]) {
		case '>':
			if (subject[pos + 1] != '\0') {
				return false;
			}

			break;
		case '.':
		case '*':
			if (last == subject[pos]) {
				return false;
			}

			break;
		default:
			if (isalnum((unsigned char)subject[pos])) {
				continue;
			}

			return false;
		}
	}

	return true;
}

static bool is_sid_valid(const char *sid, size_t len)
{
	size_t pos;

	if (!sid) {
		return false;
	}

	for (pos = 0; pos < len; pos++) {
		if (!isalnum((unsigned char)sid[pos])) {
			return false;
		}
	}

	return true;
}

#define TRANSMITV_LITERAL(lit_) { .base = lit_, .len = sizeof(lit_) - 1 }

static int transmitv(struct net_context *conn, int iovcnt,
		     struct io_vec *iov)
{
	u8_t buf[1024];
	int i, pos;

	for (i = 0, pos = 0; i < iovcnt; pos += iov[i].len, i++) {
		memcpy(&buf[pos], iov[i].base, iov[i].len);
	}

	return net_context_send(conn, buf, pos, NULL, K_NO_WAIT, NULL);
}

static inline int transmit(struct net_context *conn, const char buffer[],
			   size_t len)
{
	return transmitv(conn, 1, (struct io_vec[]) {
		{ .base = buffer, .len = len },
	});
}

#define FIELD(struct_, member_, type_) { \
	.field_name = #member_, \
	.field_name_len = sizeof(#member_) - 1, \
	.offset = offsetof(struct_, member_), \
	.type = type_ \
}
static int handle_server_info(struct nats *nats, char *payload, size_t len,
			      struct net_buf *buf, u16_t offset)
{
	static const struct json_obj_descr descr[] = {
		FIELD(struct nats_info, server_id, JSON_TOK_STRING),
		FIELD(struct nats_info, version, JSON_TOK_STRING),
		FIELD(struct nats_info, go, JSON_TOK_STRING),
		FIELD(struct nats_info, host, JSON_TOK_STRING),
		FIELD(struct nats_info, port, JSON_TOK_NUMBER),
		FIELD(struct nats_info, auth_required, JSON_TOK_TRUE),
		FIELD(struct nats_info, ssl_required, JSON_TOK_TRUE),
		FIELD(struct nats_info, max_payload, JSON_TOK_NUMBER),
	};
	struct nats_info info = {};
	char user[32], pass[64];
	size_t user_len = sizeof(user), pass_len = sizeof(pass);
	int ret;

	ret = json_obj_parse(payload, len, descr, ARRAY_SIZE(descr), &info);
	if (ret < 0) {
		return -EINVAL;
	}

	if (info.ssl_required) {
		return -ENOTSUP;
	}

	if (!info.auth_required) {
		return 0;
	}

	if (!nats->on_auth_required) {
		return -EPERM;
	}

	ret = nats->on_auth_required(nats, user, &user_len, pass, &pass_len);
	if (ret < 0) {
		return ret;
	}

	ret = json_escape(user, &user_len, sizeof(user));
	if (ret < 0) {
		return ret;
	}

	ret = json_escape(pass, &pass_len, sizeof(pass));
	if (ret < 0) {
		return ret;
	}

	return transmitv(nats->conn, 5, (struct io_vec[]) {
		TRANSMITV_LITERAL("CONNECT {\"user\":\""),
		{ .base = user, .len = user_len },
		TRANSMITV_LITERAL("\",\"pass\":\""),
		{ .base = pass, .len = pass_len },
		TRANSMITV_LITERAL("\"}\r\n"),
	});
}
#undef FIELD

static bool char_in_set(char chr, const char *set)
{
	const char *ptr;

	for (ptr = set; *ptr; ptr++) {
		if (*ptr == chr) {
			return true;
		}
	}

	return false;
}

static char *strsep(char *strp, const char *delims)
{
	const char *delim;
	char *ptr;

	if (!strp) {
		return NULL;
	}

	for (delim = delims; *delim; delim++) {
		ptr = strchr(strp, *delim);
		if (ptr) {
			*ptr = '\0';

			for (ptr++; *ptr; ptr++) {
				if (!char_in_set(*ptr, delims)) {
					break;
				}
			}

			return ptr;
		}
	}

	return NULL;
}

static int copy_pkt_to_buf(struct net_buf *src, u16_t offset,
			    char *dst, size_t dst_size, size_t n_bytes)
{
	u16_t to_copy;
	u16_t copied;

	if (dst_size < n_bytes) {
		return -ENOMEM;
	}

	while (src && offset >= src->len) {
		offset -= src->len;
		src = src->frags;
	}

	for (copied = 0U; src && n_bytes > 0; offset = 0U) {
		to_copy = MIN(n_bytes, src->len - offset);

		memcpy(dst + copied, (char *)src->data + offset, to_copy);
		copied += to_copy;

		n_bytes -= to_copy;
		src = src->frags;
	}

	if (n_bytes > 0) {
		return -ENOMEM;
	}

	return 0;
}

static int handle_server_msg(struct nats *nats, char *payload, size_t len,
			     struct net_buf *buf, u16_t offset)
{
	char *subject, *sid, *reply_to, *bytes, *end_ptr;
	char prev_end = payload[len];
	size_t payload_size;

	/* strsep() uses strchr(), ensure payload is NUL-terminated */
	payload[len] = '\0';

	/* Slice the tokens */
	subject = payload;
	sid = strsep(subject, " \t");
	reply_to = strsep(sid, " \t");
	bytes = strsep(reply_to, " \t");

	if (!bytes) {
		if (!reply_to) {
			return -EINVAL;
		}

		bytes = reply_to;
		reply_to = NULL;
	}

	/* Parse the payload size */
	errno = 0;
	payload_size = strtoul(bytes, &end_ptr, 10);
	payload[len] = prev_end;
	if (errno != 0) {
		return -errno;
	}

	if (!end_ptr) {
		return -EINVAL;
	}

	if (payload_size >= CMD_BUF_LEN - len) {
		return -ENOMEM;
	}

	if (copy_pkt_to_buf(buf, offset, end_ptr, CMD_BUF_LEN - len,
			    payload_size) < 0) {
		return -ENOMEM;
	}
	end_ptr[payload_size] = '\0';

	return nats->on_message(nats, &(struct nats_msg) {
		.subject = subject,
		.sid = sid,
		.reply_to = reply_to,
		.payload = end_ptr,
		.payload_len = payload_size,
	});
}

static int handle_server_ping(struct nats *nats, char *payload, size_t len,
			      struct net_buf *buf, u16_t offset)
{
	static const char pong[] = "PONG\r\n";

	return transmit(nats->conn, pong, sizeof(pong) - 1);
}

static int ignore(struct nats *nats, char *payload, size_t len,
		  struct net_buf *buf, u16_t offset)
{
	/* FIXME: Notify user of success/errors.  This would require
	 * maintaining information of what was the last sent command in
	 * order to provide the best error information for the user.
	 * Without VERBOSE set, these won't be sent -- but be cautious and
	 * ignore them just in case.
	 */
	return 0;
}

#define CMD(cmd_, handler_) { \
	.op = cmd_, \
	.len = sizeof(cmd_) - 1, \
	.handle = handler_ \
}
static int handle_server_cmd(struct nats *nats, char *cmd, size_t len,
			     struct net_buf *buf, u16_t offset)
{
	static const struct {
		const char *op;
		size_t len;
		int (*handle)(struct nats *nats, char *payload, size_t len,
			      struct net_buf *buf, u16_t offset);
	} cmds[] = {
		CMD("INFO", handle_server_info),
		CMD("MSG", handle_server_msg),
		CMD("PING", handle_server_ping),
		CMD("+OK", ignore),
		CMD("-ERR", ignore),
	};
	size_t i;
	char *payload;
	size_t payload_len;

	payload = strsep(cmd, " \t");
	if (!payload) {
		payload = strsep(cmd, "\r");
		if (!payload) {
			return -EINVAL;
		}
	}
	payload_len = len - (size_t)(payload - cmd);
	len = (size_t)(payload - cmd - 1);

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		if (len != cmds[i].len) {
			continue;
		}

		if (!strncmp(cmds[i].op, cmd, len)) {
			return cmds[i].handle(nats, payload, payload_len,
					      buf, offset);
		}
	}

	return -ENOENT;
}
#undef CMD

int nats_subscribe(const struct nats *nats,
		   const char *subject, size_t subject_len,
		   const char *queue_group, size_t queue_group_len,
		   const char *sid, size_t sid_len)
{
	if (!is_subject_valid(subject, subject_len)) {
		return -EINVAL;
	}

	if (!is_sid_valid(sid, sid_len)) {
		return -EINVAL;
	}

	if (queue_group) {
		return transmitv(nats->conn, 7, (struct io_vec[]) {
			TRANSMITV_LITERAL("SUB "),
			{
				.base = subject,
				.len = subject_len ?
					subject_len : strlen(subject)
			},
			TRANSMITV_LITERAL(" "),
			{
				.base = queue_group,
				.len = queue_group_len ?
					queue_group_len : strlen(queue_group)
			},
			TRANSMITV_LITERAL(" "),
			{
				.base = sid,
				.len = sid_len ? sid_len : strlen(sid)
			},
			TRANSMITV_LITERAL("\r\n")
		});
	}

	return transmitv(nats->conn, 5, (struct io_vec[]) {
		TRANSMITV_LITERAL("SUB "),
		{
			.base = subject,
			.len = subject_len ? subject_len : strlen(subject)
		},
		TRANSMITV_LITERAL(" "),
		{
			.base = sid,
			.len = sid_len ? sid_len : strlen(sid)
		},
		TRANSMITV_LITERAL("\r\n")
	});
}

int nats_unsubscribe(const struct nats *nats,
		     const char *sid, size_t sid_len, size_t max_msgs)
{
	if (!is_sid_valid(sid, sid_len)) {
		return -EINVAL;
	}

	if (max_msgs) {
		char max_msgs_str[3 * sizeof(size_t)];
		int ret;

		ret = snprintk(max_msgs_str, sizeof(max_msgs_str),
			       "%zu", max_msgs);
		if (ret < 0 || ret >= (int)sizeof(max_msgs_str)) {
			return -ENOMEM;
		}

		return transmitv(nats->conn, 5, (struct io_vec[]) {
			TRANSMITV_LITERAL("UNSUB "),
			{
				.base = sid,
				.len = sid_len ? sid_len : strlen(sid)
			},
			TRANSMITV_LITERAL(" "),
			{ .base = max_msgs_str, .len = ret },
			TRANSMITV_LITERAL("\r\n"),
		});
	}

	return transmitv(nats->conn, 3, (struct io_vec[]) {
		TRANSMITV_LITERAL("UNSUB "),
		{
			.base = sid,
			.len = sid_len ? sid_len : strlen(sid)
		},
		TRANSMITV_LITERAL("\r\n")
	});
}

int nats_publish(const struct nats *nats,
		 const char *subject, size_t subject_len,
		 const char *reply_to, size_t reply_to_len,
		 const char *payload, size_t payload_len)
{
	char payload_len_str[3 * sizeof(size_t)];
	int ret;

	if (!is_subject_valid(subject, subject_len)) {
		return -EINVAL;
	}

	ret = snprintk(payload_len_str, sizeof(payload_len_str), "%zu",
		       payload_len);
	if (ret < 0 || ret >= (int)sizeof(payload_len_str)) {
		return -ENOMEM;
	}

	if (reply_to) {
		return transmitv(nats->conn, 9, (struct io_vec[]) {
			TRANSMITV_LITERAL("PUB "),
			{
				.base = subject,
				.len = subject_len ?
					subject_len : strlen(subject)
			},
			TRANSMITV_LITERAL(" "),
			{
				.base = reply_to,
				.len = reply_to_len ?
					reply_to_len : strlen(reply_to)
			},
			TRANSMITV_LITERAL(" "),
			{ .base = payload_len_str, .len = ret },
			TRANSMITV_LITERAL("\r\n"),
			{ .base = payload, .len = payload_len },
			TRANSMITV_LITERAL("\r\n"),
		});
	}

	return transmitv(nats->conn, 7, (struct io_vec[]) {
		TRANSMITV_LITERAL("PUB "),
		{
			.base = subject,
			.len = subject_len ? subject_len : strlen(subject)
		},
		TRANSMITV_LITERAL(" "),
		{ .base = payload_len_str, .len = ret },
		TRANSMITV_LITERAL("\r\n"),
		{ .base = payload, .len = payload_len },
		TRANSMITV_LITERAL("\r\n"),
	});
}

static void receive_cb(struct net_context *ctx,
		       struct net_pkt *pkt,
		       union net_ip_header *ip_hdr,
		       union net_proto_header *proto_hdr,
		       int status,
		       void *user_data)
{
	struct nats *nats = user_data;
	char cmd_buf[CMD_BUF_LEN];
	struct net_buf *tmp;
	u16_t pos = 0U, cmd_len = 0U;
	size_t len;
	u8_t *end_of_line;

	if (!pkt) {
		/* FIXME: How to handle disconnection? */
		return;
	}

	if (status) {
		/* FIXME: How to handle connectio error? */
		net_pkt_unref(pkt);
		return;
	}

	tmp = pkt->cursor.buf;
	if (!tmp) {
		net_pkt_unref(pkt);
		return;
	}

	pos = pkt->cursor.pos - tmp->data;

	while (tmp) {
		len = tmp->len - pos;

		end_of_line = memchr((u8_t *)tmp->data + pos, '\n', len);
		if (end_of_line) {
			len = end_of_line - ((u8_t *)tmp->data + pos);
		}

		if (cmd_len + len > sizeof(cmd_buf)) {
			break;
		}

		if (net_pkt_read(pkt, (u8_t *)(cmd_buf + cmd_len), len)) {
			break;
		}

		cmd_len += len;

		if (end_of_line) {
			u8_t dummy;
			int ret;

			if (net_pkt_read_u8(pkt, &dummy)) {
				break;
			}

			cmd_buf[cmd_len] = '\0';

			ret = handle_server_cmd(nats, cmd_buf, cmd_len,
						tmp, pos);
			if (ret < 0) {
				/* FIXME: What to do with unhandled messages? */
				break;
			}

			cmd_len = 0U;
		}

		tmp = pkt->cursor.buf;
		pos = pkt->cursor.pos - tmp->data;
	}

	net_pkt_unref(pkt);
}

int nats_connect(struct nats *nats, struct sockaddr *addr, socklen_t addrlen)
{
	int ret;

	ret = net_context_connect(nats->conn, addr, addrlen,
				  NULL, K_FOREVER, NULL);
	if (ret < 0) {
		return ret;
	}

	return net_context_recv(nats->conn, receive_cb, K_NO_WAIT, nats);
}

int nats_disconnect(struct nats *nats)
{
	int ret;

	ret = net_context_put(nats->conn);
	if (ret < 0) {
		return ret;
	}

	nats->conn = NULL;

	return 0;
}
