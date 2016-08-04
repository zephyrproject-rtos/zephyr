/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nats_pack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/* NATS requires that the app's language and version
 * are included in the CONNECT message.
 * So, here we send the GCC version. It could be anything.
 */
#ifdef __GNUC__
#define CLANG		"GCC"
#define CLANG_VERSION	__VERSION__
#else
#define CLANG		"unknown"
#define CLANG_VERSION	"unknown"
#endif

#define NATS_STR_TRUE_FALSE(v) (v) != 0 ? "true" : "false"

#define NATS_STR_NULL(str) ((str) == NULL ? "" : (str))

#define NATS_STR_COMMA(comma) (comma) ? "," : ""

#define min(a, b)		((a) < (b) ? (a) : (b))

#define PING_MSG		"PING\r\n"
#define PONG_MSG		"PONG\r\n"
#define PING_PONG_MSG_LEN	6
#define MSG_MSG_MIN_SIZE	13

static int nats_isblank(int c)
{
	return c == ' ' || c == '\t';
}

static int nats_unpack(struct app_buf_t *buf, enum nats_msg_type type);
static int nats_pack(struct app_buf_t *buf, enum nats_msg_type type);


int nats_pack_info(struct app_buf_t *buf, char *server_id, char *version,
		  char *go, char *host, int port, int auth_req, int ssl_req,
		  int max_payload)
{
	size_t size;
	size_t len;
	char *str;
	int comma;

	str = (char *)buf->buf;
	size = buf->size;
	comma = 0;

	len = snprintf(str, size, "INFO {");

	if (server_id) {
		len += snprintf(str + len, size - len, "\"server_id\":\"%s\"",
			       server_id);
		comma = 1;
	}

	if (version) {
		len += snprintf(str + len, size - len, "%s\"version\":\"%s\"",
			       NATS_STR_COMMA(comma), version);
		comma = 1;
	}

	if (go) {
		len += snprintf(str + len, size - len, "%s\"go\":\"%s\"",
			       NATS_STR_COMMA(comma), go);
		comma = 1;
	}

	if (host) {
		len += snprintf(str + len, size - len, "%s\"host\":\"%s\"",
			       NATS_STR_COMMA(comma), host);
		comma = 1;
	}

	if (port) {
		len += snprintf(str + len, size - len, "%s\"port\":%d",
			       NATS_STR_COMMA(comma), port);
		comma = 1;
	}

	len += snprintf(str + len, size - len, "%s\"auth_required\":%s",
		       NATS_STR_COMMA(comma), auth_req ? "true" : "false");

	len += snprintf(str + len, size - len, ",\"ssl_required\":%s",
		       ssl_req ? "true" : "false");

	if (max_payload) {
		len += snprintf(str + len, size - len, ",\"max_payload\":%d",
			       max_payload);
	}

	len += snprintf(str + len, size, "}\r\n");

	buf->length = len;
	if (buf->length) {
		return 0;
	}

	return -EINVAL;
}

int nats_pack_connect(struct app_buf_t *buf, int verbose, int pedantic,
		     int ssl_req, char *auth_token, char *user, char *pass,
		     char *name, char *lang, char *version)
{
	size_t size;
	size_t len;
	char *str;

	str = (char *)buf->buf;
	size = buf->size;

	len = snprintf(str, size, "CONNECT {\"verbose\":%s,\"pedantic\":%s,"
		      "\"ssl_required\":%s",
		      NATS_STR_TRUE_FALSE(verbose),
		      NATS_STR_TRUE_FALSE(pedantic),
		      NATS_STR_TRUE_FALSE(ssl_req));

	if (auth_token) {
		len += snprintf(str + len, size - len,
				",\"auth_token\":\"%s\"", auth_token);
	}

	if (user) {
		len += snprintf(str + len, size - len, ",\"user\":\"%s\"",
				user);
	}

	if (pass) {
		len += snprintf(str + len, size - len, ",\"pass\":\"%s\"",
				pass);
	}

	if (name) {
		len += snprintf(str + len, size - len, ",\"name\":\"%s\"",
				name);
	}

	len += snprintf(str + len, size - len,
		       ",\"lang\":\"%s\",\"version\":\"%s\"}\r\n",
		      lang, version);

	buf->length = len;
	if (buf->length) {
		return 0;
	}

	return -EINVAL;
}

int nats_pack_quickcon(struct app_buf_t *buf, char *name, int verbose)
{
	return nats_pack_connect(buf, verbose, 1, 0, NULL, NULL, NULL, name,
				CLANG, CLANG_VERSION);
}

int nats_pack_pub(struct app_buf_t *buf, char *subject, char *reply_to,
		 char *payload)
{
	size_t size;
	char *str;

	str = (char *)buf->buf;
	size = buf->size;

	buf->length = snprintf(str, size, "PUB %s %s %d\r\n%s\r\n",
			       subject, NATS_STR_NULL(reply_to),
			       (int)strlen(NATS_STR_NULL(payload)),
			       NATS_STR_NULL(payload));
	if (buf->length) {
		return 0;
	}

	return -EINVAL;
}

int nats_pack_sub(struct app_buf_t *buf, char *subject, char *queue_grp,
		  char *sid)
{
	size_t size;
	char *str;

	str = (char *)buf->buf;
	size = buf->size;

	buf->length = snprintf(str, size, "SUB %s %s %s\r\n", subject,
			       NATS_STR_NULL(queue_grp), sid);
	if (buf->length) {
		return 0;
	}

	return -EINVAL;
}

int nats_pack_unsub(struct app_buf_t *buf, char *sid, int max_msgs)
{
	size_t size;
	char *str;

	str = (char *)buf->buf;
	size = buf->size;

	if (max_msgs > 0) {
		buf->length = snprintf(str, size, "UNSUB %s %d\r\n", sid,
				       max_msgs);
	} else {
		buf->length = snprintf(str, size, "UNSUB %s\r\n", sid);
	}
	if (buf->length) {
		return 0;
	}

	return -EINVAL;
}

int nats_pack_msg(struct app_buf_t *buf, char *subject, char *sid,
		 char *reply_to, char *payload)
{
	size_t size;
	char *str;

	str = (char *)buf->buf;
	size = buf->size;

	buf->length = snprintf(str, size, "MSG %s %s %s %d\r\n%s\r\n", subject,
			       sid, NATS_STR_NULL(reply_to),
			       (int)strlen(NATS_STR_NULL(payload)),
			       NATS_STR_NULL(payload));
	if (buf->length) {
		return 0;
	}

	return -EINVAL;
}

int nats_unpack_msg(struct app_buf_t *buf,
		    int *subject_start, int *subject_len,
		    int *sid_start, int *sid_len,
		    int *reply_start, int *reply_len,
		    int *payload_start, int *payload_len)
{
	int payload_len_start;
	int payload_size;
	int size;
	int i;
	char *str;

	str = (char *)buf->buf;
	size = (int)buf->length;

	if (size < MSG_MSG_MIN_SIZE) {
		return -EINVAL;
	}

	if (str[0] != 'M' || str[1] != 'S' || str[2] != 'G') {
		return -EINVAL;
	}

	/* subject */
	for (i = 3; i < size && nats_isblank(str[i]); i++) {
	}
	if (i == size) {
		return -EINVAL;
	}
	*subject_start = i;

	for (; i < size && !nats_isblank(str[i]); i++) {
	}
	if (i == size) {
		return -EINVAL;
	}
	*subject_len = i - *subject_start;

	/* sid */
	for (; i < size && nats_isblank(str[i]); i++) {
	}
	if (i == size) {
		return -EINVAL;
	}
	*sid_start = i;

	for (; i < size && !nats_isblank(str[i]); i++) {
	}
	if (i == size) {
		return -EINVAL;
	}
	*sid_len = i - *sid_start;

	/* payload */
	if (str[size-1] != '\n' || str[size-2] != '\r') {
		return -EINVAL;
	}
	for (i = size - 3; i >= 0 && str[i] != '\n'; i--) {
	}
	if (i == 0) {
		return -EINVAL;
	}
	if (str[i] != '\n' || str[i - 1] != '\r') {
		return -EINVAL;
	}
	*payload_start = i + 1;
	*payload_len = size - 2 - *payload_start;

	/* payload size */
	i -= 2;
	for (; i >= 0 && isdigit(str[i]); i--) {
	}
	if (i <= 0 || !nats_isblank(str[i])) {
		return -EINVAL;
	}
	payload_len_start = i + 1;
	payload_size = atoi(str + payload_len_start);
	if (payload_size != *payload_len) {
		return -EINVAL;
	}

	/* find reply-to after sid and payload size were found */
	i = *sid_start + *sid_len;
	for (; i < size && nats_isblank(str[i]); i++) {
	}
	if (i < payload_len_start) {
		*reply_start = i;
		for (; i < size && !nats_isblank(str[i]); i++) {
		}
		*reply_len = i - *reply_start;
	} else {
		*reply_start = *reply_len = -1;
	}

	return 0;
}

int nats_unpack_info(struct app_buf_t *buf)
{
	int rc;

	rc = nats_find_msg(buf, "INFO");
	/* TODO: evaluate all INFO options */

	return rc;
}

static int nats_pack(struct app_buf_t *buf, enum nats_msg_type type)
{
	char *str;
	size_t size;

	str = (char *)buf->buf;
	size = buf->size;

	switch (type) {
	case NATS_MSG_PING:
		buf->length = snprintf(str, size, "PING\r\n");
		break;
	case NATS_MSG_PONG:
		buf->length = snprintf(str, size, "PONG\r\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int nats_pack_ping(struct app_buf_t *buf)
{
	return nats_pack(buf, NATS_MSG_PING);
}

int nats_pack_pong(struct app_buf_t *buf)
{
	return nats_pack(buf, NATS_MSG_PONG);
}

static int nats_unpack(struct app_buf_t *buf, enum nats_msg_type type)
{
	char *str;
	size_t len;

	str = (char *)buf->buf;

	switch (type) {
	case NATS_MSG_PING:
		len = min(buf->length, PING_PONG_MSG_LEN);
		if (strncmp(str, PING_MSG, len) != 0) {
			return -EINVAL;
		}
		break;
	case NATS_MSG_PONG:
		len = min(buf->length, PING_PONG_MSG_LEN);
		if (strncmp(str, PONG_MSG, len) != 0) {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int nats_unpack_ping(struct app_buf_t *buf)
{
	return nats_unpack(buf, NATS_MSG_PING);
}

int nats_unpack_pong(struct app_buf_t *buf)
{
	return nats_unpack(buf, NATS_MSG_PONG);
}

int nats_find_msg(struct app_buf_t *buf, char *str)
{
	size_t size;
	char *_buf;
	int len;
	int i;

	_buf = buf->buf;
	size = buf->length;

	i = 0;
	do {
	} while (i < size && (isalpha(_buf[i]) == 0 && _buf[i] != '+') && ++i);

	len = strlen(str);
	if (i + len >= size || strncmp(_buf + i, str, len) != 0) {
		return -EINVAL;
	}

	return 0;
}
