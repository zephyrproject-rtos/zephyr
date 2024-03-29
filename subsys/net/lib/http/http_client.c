/** @file
 * @brief HTTP client API
 *
 * An API for applications to send HTTP requests
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_http, CONFIG_NET_HTTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>

#include "net_private.h"

#define HTTP_CONTENT_LEN_SIZE 11
#define MAX_SEND_BUF_LEN 192

static int sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = zsock_send(sock, buf, len, 0);

		if (out_len < 0) {
			return -errno;
		}

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

static int http_send_data(int sock, char *send_buf,
			  size_t send_buf_max_len, size_t *send_buf_pos,
			  ...)
{
	const char *data;
	va_list va;
	int ret, end_of_send = *send_buf_pos;
	int end_of_data, remaining_len;
	int sent = 0;

	va_start(va, send_buf_pos);

	data = va_arg(va, const char *);

	while (data) {
		end_of_data = 0;

		do {
			int to_be_copied;

			remaining_len = strlen(data + end_of_data);
			to_be_copied = send_buf_max_len - end_of_send;

			if (remaining_len > to_be_copied) {
				strncpy(send_buf + end_of_send,
					data + end_of_data,
					to_be_copied);

				end_of_send += to_be_copied;
				end_of_data += to_be_copied;
				remaining_len -= to_be_copied;

				LOG_HEXDUMP_DBG(send_buf, end_of_send,
						"Data to send");

				ret = sendall(sock, send_buf, end_of_send);
				if (ret < 0) {
					NET_DBG("Cannot send %d bytes (%d)",
						end_of_send, ret);
					goto err;
				}
				sent += end_of_send;
				end_of_send = 0;
				continue;
			} else {
				strncpy(send_buf + end_of_send,
					data + end_of_data,
					remaining_len);
				end_of_send += remaining_len;
				remaining_len = 0;
			}
		} while (remaining_len > 0);

		data = va_arg(va, const char *);
	}

	va_end(va);

	if (end_of_send > (int)send_buf_max_len) {
		NET_ERR("Sending overflow (%d > %zd)", end_of_send,
			send_buf_max_len);
		return -EMSGSIZE;
	}

	*send_buf_pos = end_of_send;

	return sent;

err:
	va_end(va);

	return ret;
}

static int http_flush_data(int sock, const char *send_buf, size_t send_buf_len)
{
	int ret;

	LOG_HEXDUMP_DBG(send_buf, send_buf_len, "Data to send");

	ret = sendall(sock, send_buf, send_buf_len);
	if (ret < 0) {
		return ret;
	}

	return (int)send_buf_len;
}

static void print_header_field(size_t len, const char *str)
{
	if (IS_ENABLED(CONFIG_NET_HTTP_LOG_LEVEL_DBG)) {
#define MAX_OUTPUT_LEN 128
		char output[MAX_OUTPUT_LEN];

		/* The value of len does not count \0 so we need to increase it
		 * by one.
		 */
		if ((len + 1) > sizeof(output)) {
			len = sizeof(output) - 1;
		}

		snprintk(output, len + 1, "%s", str);

		NET_DBG("[%zd] %s", len, output);
	}
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);
	print_header_field(length, at);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_url) {
		req->internal.response.http_cb->on_url(parser, at, length);
	}

	return 0;
}

static int on_status(struct http_parser *parser, const char *at, size_t length)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);
	uint16_t len;

	len = MIN(length, sizeof(req->internal.response.http_status) - 1);
	memcpy(req->internal.response.http_status, at, len);
	req->internal.response.http_status[len] = 0;
	req->internal.response.http_status_code =
		(uint16_t)parser->status_code;

	NET_DBG("HTTP response status %d %s", parser->status_code,
		req->internal.response.http_status);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_status) {
		req->internal.response.http_cb->on_status(parser, at, length);
	}

	return 0;
}

static int on_header_field(struct http_parser *parser, const char *at,
			   size_t length)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);
	const char *content_len = "Content-Length";
	uint16_t len;

	len = strlen(content_len);
	if (length >= len && strncasecmp(at, content_len, len) == 0) {
		req->internal.response.cl_present = true;
	}

	print_header_field(length, at);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_header_field) {
		req->internal.response.http_cb->on_header_field(parser, at,
								length);
	}

	return 0;
}

#define MAX_NUM_DIGITS	16

static int on_header_value(struct http_parser *parser, const char *at,
			   size_t length)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);
	char str[MAX_NUM_DIGITS];

	if (req->internal.response.cl_present) {
		if (length <= MAX_NUM_DIGITS - 1) {
			long int num;

			memcpy(str, at, length);
			str[length] = 0;

			num = strtol(str, NULL, 10);
			if (num == LONG_MIN || num == LONG_MAX) {
				return -EINVAL;
			}

			req->internal.response.content_length = num;
		}

		req->internal.response.cl_present = false;
	}

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_header_value) {
		req->internal.response.http_cb->on_header_value(parser, at,
								length);
	}

	print_header_field(length, at);

	return 0;
}

static int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);

	req->internal.response.body_found = 1;
	req->internal.response.processed += length;

	NET_DBG("Processed %zd length %zd", req->internal.response.processed,
		length);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_body) {
		req->internal.response.http_cb->on_body(parser, at, length);
	}

	/* Reset the body_frag_start pointer for each fragment. */
	if (!req->internal.response.body_frag_start) {
		req->internal.response.body_frag_start = (uint8_t *)at;
	}

	/* Calculate the length of the body contained in the recv_buf */
	req->internal.response.body_frag_len = req->internal.response.data_len -
		(req->internal.response.body_frag_start - req->internal.response.recv_buf);

	return 0;
}

static int on_headers_complete(struct http_parser *parser)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_headers_complete) {
		req->internal.response.http_cb->on_headers_complete(parser);
	}

	if (parser->status_code >= 500 && parser->status_code < 600) {
		NET_DBG("Status %d, skipping body", parser->status_code);
		return 1;
	}

	if ((req->method == HTTP_HEAD || req->method == HTTP_OPTIONS) &&
	    req->internal.response.content_length > 0) {
		NET_DBG("No body expected");
		return 1;
	}

	NET_DBG("Headers complete");

	return 0;
}

static int on_message_begin(struct http_parser *parser)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_message_begin) {
		req->internal.response.http_cb->on_message_begin(parser);
	}

	NET_DBG("-- HTTP %s response (headers) --",
		http_method_str(req->method));

	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_message_complete) {
		req->internal.response.http_cb->on_message_complete(parser);
	}

	NET_DBG("-- HTTP %s response (complete) --",
		http_method_str(req->method));

	req->internal.response.message_complete = 1;

	return 0;
}

static int on_chunk_header(struct http_parser *parser)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_chunk_header) {
		req->internal.response.http_cb->on_chunk_header(parser);
	}

	return 0;
}

static int on_chunk_complete(struct http_parser *parser)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);

	if (req->internal.response.http_cb &&
	    req->internal.response.http_cb->on_chunk_complete) {
		req->internal.response.http_cb->on_chunk_complete(parser);
	}

	return 0;
}

static void http_client_init_parser(struct http_parser *parser,
				    struct http_parser_settings *settings)
{
	http_parser_init(parser, HTTP_RESPONSE);

	settings->on_body = on_body;
	settings->on_chunk_complete = on_chunk_complete;
	settings->on_chunk_header = on_chunk_header;
	settings->on_headers_complete = on_headers_complete;
	settings->on_header_field = on_header_field;
	settings->on_header_value = on_header_value;
	settings->on_message_begin = on_message_begin;
	settings->on_message_complete = on_message_complete;
	settings->on_status = on_status;
	settings->on_url = on_url;
}

/* Report a NULL HTTP response to the caller.
 * A NULL response is when the HTTP server intentionally closes the TLS socket (using FINACK)
 * without sending any HTTP payload.
 */
static void http_report_null(struct http_request *req)
{
	if (req->internal.response.cb) {
		NET_DBG("Calling callback for Final Data"
			"(NULL HTTP response)");

		/* Status code 0 representing a null response */
		req->internal.response.http_status_code = 0;

		/* Zero out related response metrics */
		req->internal.response.processed = 0;
		req->internal.response.data_len = 0;
		req->internal.response.content_length = 0;
		req->internal.response.body_frag_start = NULL;
		memset(req->internal.response.http_status, 0, HTTP_STATUS_STR_SIZE);

		req->internal.response.cb(&req->internal.response, HTTP_DATA_FINAL,
					  req->internal.user_data);
	}
}

/* Report a completed HTTP transaction (with no error) to the caller */
static void http_report_complete(struct http_request *req)
{
	if (req->internal.response.cb) {
		NET_DBG("Calling callback for %zd len data", req->internal.response.data_len);
		req->internal.response.cb(&req->internal.response, HTTP_DATA_FINAL,
					  req->internal.user_data);
	}
}

/* Report that some data has been received, but the HTTP transaction is still ongoing. */
static void http_report_progress(struct http_request *req)
{
	if (req->internal.response.cb) {
		NET_DBG("Calling callback for partitioned %zd len data",
			req->internal.response.data_len);

		req->internal.response.cb(&req->internal.response, HTTP_DATA_MORE,
					  req->internal.user_data);
	}
}

static int http_wait_data(int sock, struct http_request *req, int32_t timeout)
{
	int total_received = 0;
	size_t offset = 0;
	int received, ret;
	struct zsock_pollfd fds[1];
	int nfds = 1;
	int32_t remaining_time = timeout;
	int64_t timestamp = k_uptime_get();

	fds[0].fd = sock;
	fds[0].events = ZSOCK_POLLIN;

	do {
		if (timeout > 0) {
			remaining_time -= (int32_t)k_uptime_delta(&timestamp);
			if (remaining_time < 0) {
				/* timeout, make poll return immediately */
				remaining_time = 0;
			}
		}

		ret = zsock_poll(fds, nfds, remaining_time);
		if (ret == 0) {
			LOG_DBG("Timeout");
			ret = -ETIMEDOUT;
			goto error;
		} else if (ret < 0) {
			ret = -errno;
			goto error;
		}
		if (fds[0].revents & (ZSOCK_POLLERR | ZSOCK_POLLNVAL)) {
			ret = -errno;
			goto error;
		} else if (fds[0].revents & ZSOCK_POLLHUP) {
			/* Connection closed */
			goto closed;
		} else if (fds[0].revents & ZSOCK_POLLIN) {
			received = zsock_recv(sock, req->internal.response.recv_buf + offset,
					      req->internal.response.recv_buf_len - offset, 0);
			if (received == 0) {
				/* Connection closed */
				goto closed;
			} else if (received < 0) {
				ret = -errno;
				goto error;
			} else {
				req->internal.response.data_len += received;

				(void)http_parser_execute(
					&req->internal.parser, &req->internal.parser_settings,
					req->internal.response.recv_buf + offset, received);
			}

			total_received += received;
			offset += received;

			if (offset >= req->internal.response.recv_buf_len) {
				offset = 0;
			}

			if (req->internal.response.message_complete) {
				http_report_complete(req);
				break;
			} else if (offset == 0) {
				http_report_progress(req);

				/* Re-use the result buffer and start to fill it again */
				req->internal.response.data_len = 0;
				req->internal.response.body_frag_start = NULL;
				req->internal.response.body_frag_len = 0;
			}
		}

	} while (true);

	return total_received;

closed:
	LOG_DBG("Connection closed");

	/* If connection was closed with no data sent, this is a NULL response, and is a special
	 * case valid response.
	 */
	if (total_received == 0) {
		http_report_null(req);
		return total_received;
	}

	/* Otherwise, connection was closed mid-way through response, and this should be
	 * considered an error.
	 */
	ret = -ECONNRESET;

error:
	LOG_DBG("Connection error (%d)", ret);
	return ret;
}

int http_client_req(int sock, struct http_request *req,
		    int32_t timeout, void *user_data)
{
	/* Utilize the network usage by sending data in bigger blocks */
	char send_buf[MAX_SEND_BUF_LEN];
	const size_t send_buf_max_len = sizeof(send_buf);
	size_t send_buf_pos = 0;
	int total_sent = 0;
	int ret, total_recv, i;
	const char *method;

	if (sock < 0 || req == NULL || req->response == NULL ||
	    req->recv_buf == NULL || req->recv_buf_len == 0) {
		return -EINVAL;
	}

	memset(&req->internal.response, 0, sizeof(req->internal.response));

	req->internal.response.http_cb = req->http_cb;
	req->internal.response.cb = req->response;
	req->internal.response.recv_buf = req->recv_buf;
	req->internal.response.recv_buf_len = req->recv_buf_len;
	req->internal.user_data = user_data;
	req->internal.sock = sock;

	method = http_method_str(req->method);

	ret = http_send_data(sock, send_buf, send_buf_max_len, &send_buf_pos,
			     method, " ", req->url, " ", req->protocol,
			     HTTP_CRLF, NULL);
	if (ret < 0) {
		goto out;
	}

	total_sent += ret;

	if (req->port) {
		ret = http_send_data(sock, send_buf, send_buf_max_len,
				     &send_buf_pos, "Host", ": ", req->host,
				     ":", req->port, HTTP_CRLF, NULL);

		if (ret < 0) {
			goto out;
		}

		total_sent += ret;
	} else {
		ret = http_send_data(sock, send_buf, send_buf_max_len,
				     &send_buf_pos, "Host", ": ", req->host,
				     HTTP_CRLF, NULL);

		if (ret < 0) {
			goto out;
		}

		total_sent += ret;
	}

	if (req->optional_headers_cb) {
		ret = http_flush_data(sock, send_buf, send_buf_pos);
		if (ret < 0) {
			goto out;
		}

		send_buf_pos = 0;
		total_sent += ret;

		ret = req->optional_headers_cb(sock, req, user_data);
		if (ret < 0) {
			goto out;
		}

		total_sent += ret;
	} else {
		for (i = 0; req->optional_headers && req->optional_headers[i];
		     i++) {
			ret = http_send_data(sock, send_buf, send_buf_max_len,
					     &send_buf_pos,
					     req->optional_headers[i], NULL);
			if (ret < 0) {
				goto out;
			}

			total_sent += ret;
		}
	}

	for (i = 0; req->header_fields && req->header_fields[i]; i++) {
		ret = http_send_data(sock, send_buf, send_buf_max_len,
				     &send_buf_pos, req->header_fields[i],
				     NULL);
		if (ret < 0) {
			goto out;
		}

		total_sent += ret;
	}

	if (req->content_type_value) {
		ret = http_send_data(sock, send_buf, send_buf_max_len,
				     &send_buf_pos, "Content-Type", ": ",
				     req->content_type_value, HTTP_CRLF, NULL);
		if (ret < 0) {
			goto out;
		}

		total_sent += ret;
	}

	if (req->payload || req->payload_cb) {
		if (req->payload_len) {
			char content_len_str[HTTP_CONTENT_LEN_SIZE];

			ret = snprintk(content_len_str, HTTP_CONTENT_LEN_SIZE,
				       "%zd", req->payload_len);
			if (ret <= 0 || ret >= HTTP_CONTENT_LEN_SIZE) {
				ret = -ENOMEM;
				goto out;
			}

			ret = http_send_data(sock, send_buf, send_buf_max_len,
					     &send_buf_pos, "Content-Length", ": ",
					     content_len_str, HTTP_CRLF,
					     HTTP_CRLF, NULL);
		} else {
			ret = http_send_data(sock, send_buf, send_buf_max_len,
				     &send_buf_pos, HTTP_CRLF, NULL);
		}

		if (ret < 0) {
			goto out;
		}

		total_sent += ret;

		ret = http_flush_data(sock, send_buf, send_buf_pos);
		if (ret < 0) {
			goto out;
		}

		send_buf_pos = 0;
		total_sent += ret;

		if (req->payload_cb) {
			ret = req->payload_cb(sock, req, user_data);
			if (ret < 0) {
				goto out;
			}

			total_sent += ret;
		} else {
			uint32_t length;

			if (req->payload_len == 0) {
				length = strlen(req->payload);
			} else {
				length = req->payload_len;
			}

			ret = sendall(sock, req->payload, length);
			if (ret < 0) {
				goto out;
			}

			total_sent += length;
		}
	} else {
		ret = http_send_data(sock, send_buf, send_buf_max_len,
				     &send_buf_pos, HTTP_CRLF, NULL);
		if (ret < 0) {
			goto out;
		}

		total_sent += ret;
	}

	if (send_buf_pos > 0) {
		ret = http_flush_data(sock, send_buf, send_buf_pos);
		if (ret < 0) {
			goto out;
		}

		total_sent += ret;
	}

	NET_DBG("Sent %d bytes", total_sent);

	http_client_init_parser(&req->internal.parser,
				&req->internal.parser_settings);

	/* Request is sent, now wait data to be received */
	total_recv = http_wait_data(sock, req, timeout);
	if (total_recv < 0) {
		NET_DBG("Wait data failure (%d)", total_recv);
		ret = total_recv;
		goto out;
	}

	NET_DBG("Received %d bytes", total_recv);

	return total_sent;

out:
	return ret;
}
