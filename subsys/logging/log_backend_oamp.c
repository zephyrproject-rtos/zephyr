/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log_backend.h>
#include <logging/log_ctrl.h>
#include "log_backend_open_amp.h"
#include <logging/log_output.h>
#include <logging/log.h>
#include "log_multidomain_internal.h"

LOG_MODULE_REGISTER(backend_oamp);

#define LOG_OUTPUT_BUFSIZE 8

#define LOG_DOMAIN_NAME_PREFIX "net"

static const char *prefix = LOG_DOMAIN_NAME_PREFIX;
static void clbk(u8_t *data, size_t len);

LOG_OUTPUT_EXT_MSG_CONVERTER_DEFINE(converter, LOG_OUTPUT_BUFSIZE);

static void init(void)
{
	LOG_INF("init");
	log_backend_open_amp_init(clbk);
}

static int msg_send(struct log_msg *msg)
{
	struct log_msg_cont *next;
	struct log_msg_cont *next2;
	u8_t *raw_msg = (u8_t *)msg;
	int len = msg->hdr.params.hexdump.length;

	raw_msg[0] = LOG_MULDOMAIN_MSG_HEAD;
	log_backend_open_amp_send(raw_msg, sizeof(struct log_msg));
	next = msg->payload.ext.next;
	if (len <= LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		return 0;
	}

	len -= LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
	while (len > 0) {
		u8_t prev_byte;

		next2 = next->next;
		raw_msg = (u8_t *)next;
		prev_byte = raw_msg[0];
		raw_msg[0] = LOG_MULDOMAIN_MSG_CONT;
		log_backend_open_amp_send(raw_msg, sizeof(struct log_msg_cont));
		raw_msg[0] = prev_byte;
		len -= HEXDUMP_BYTES_CONT_MSG;
		next = next2;
	}

	return 0;
}

static void put(const struct log_backend *const backend, struct log_msg *msg)
{
	struct log_msg *out_msg;

	log_msg_get(msg);

	out_msg = log_output_convert_to_ext_msg(&converter, msg);

	/* message is converted to formatted one and original is not needed. */
	log_msg_put(msg);

	msg_send(out_msg);
	log_msg_put(out_msg);
}

void panic(const struct log_backend *const backend)
{

}

static const struct log_backend_api api = {
#if 0
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			 sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			 sync_hexdump : NULL,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
#endif
	.panic = panic,
	.put = put,
	.init = init,
 };

LOG_BACKEND_DEFINE(log_backend_oamp, api, true);

static void get_domain_name(u8_t *msg)
{
	u8_t domain_id = *msg;
	const char *name = log_domain_name_get(domain_id);
	int raw_len = strlen(name);
	int total_name_len = sizeof(LOG_DOMAIN_NAME_PREFIX) + raw_len +
			(raw_len > 0 ? 1 : 0) /* slash if complex name*/;
	u8_t *buf = alloca(total_name_len + 1/* header */);
	u8_t *start = buf;

	*buf = LOG_MULDOMAIN_DOMAIN_NAME_GET;
	buf++;
	memcpy(buf, prefix, sizeof(LOG_DOMAIN_NAME_PREFIX));
	buf += sizeof(LOG_DOMAIN_NAME_PREFIX);
	if (raw_len) {
		*buf++ = '/';
		memcpy(buf, name, raw_len);
	}

	log_backend_open_amp_send(start, total_name_len + 1);
}

static void get_source_count(u8_t *msg)
{
	u8_t domain_id = *msg;
	u16_t count = log_sources_count(domain_id);
	u8_t rsp[3];

	rsp[0] = LOG_MULDOMAIN_SOURCE_COUNT_GET;
	memcpy(&rsp[1], &count, sizeof(count));
	log_backend_open_amp_send(rsp, sizeof(rsp));
}

static void get_source_name(u8_t *msg)
{
	u8_t domain_id = *msg++;
	u16_t source_id = *(u16_t *)msg;
	u8_t buf[64];
	const char *name;

	buf[0] = LOG_MULDOMAIN_SOURCE_NAME_GET;
	name = log_source_name_get(&buf[1], sizeof(buf) - 1,
				   domain_id, source_id);

	if ((void *)name != (void *)&buf[1]) {
		memcpy(&buf[1], name, strlen(name));
	}
	log_backend_open_amp_send(buf, strlen(name) + 1);
}

static void get_compiled_level(u8_t *msg)
{
	u8_t domain_id = *msg++;
	u16_t source_id = *(u16_t *)msg;
	u8_t rsp[] = {
		LOG_MULDOMAIN_COMPILED_LEVEL_GET,
		log_compiled_level_get(domain_id, source_id)
	};

	log_backend_open_amp_send(rsp, sizeof(rsp));
}

static void get_runtime_level(u8_t *msg)
{
	u8_t domain_id = *msg++;
	u16_t source_id = *(u16_t *)msg;
	u8_t rsp[] = {
		LOG_MULDOMAIN_RUNTIME_LEVEL_GET,
		(u8_t)log_filter_get(&log_backend_oamp, domain_id,
				     source_id, true)
	};

	log_backend_open_amp_send(rsp, sizeof(rsp));
}

static void set_runtime_level(u8_t *msg)
{
	u8_t domain_id = *msg++;
	u16_t source_id = *(u16_t *)msg;
	u8_t level = msg[2];
	u8_t rsp[] = {
		LOG_MULDOMAIN_RUNTIME_LEVEL_SET
	};

	log_filter_set(&log_backend_oamp, domain_id, source_id, level);

	log_backend_open_amp_send(rsp, sizeof(rsp));
}

static void get_timestamp(u8_t *msg)
{
	u8_t rsp[sizeof(u8_t) + sizeof(u32_t)];

	rsp[0] = LOG_MULDOMAIN_TIMESTAMP_GET;
	*(u32_t *)&rsp[1] = z_log_get_timestamp();

	log_backend_open_amp_send(rsp, sizeof(rsp));
}

static void clbk(u8_t *data, size_t len)
{
	u8_t msg_id = *data++;

	switch (msg_id) {
	case LOG_MULDOMAIN_DOMAIN_NAME_GET:
		get_domain_name(data);
		break;
	case LOG_MULDOMAIN_SOURCE_COUNT_GET:
		get_source_count(data);
		break;
	case LOG_MULDOMAIN_SOURCE_NAME_GET:
		get_source_name(data);
		break;
	case LOG_MULDOMAIN_COMPILED_LEVEL_GET:
		get_compiled_level(data);
		break;
	case LOG_MULDOMAIN_RUNTIME_LEVEL_GET:
		get_runtime_level(data);
		break;
	case LOG_MULDOMAIN_RUNTIME_LEVEL_SET:
		set_runtime_level(data);
		break;
	case LOG_MULDOMAIN_TIMESTAMP_GET:
		get_timestamp(data);
		break;
	}
}

