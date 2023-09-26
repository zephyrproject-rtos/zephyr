/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <stubs.h>

LOG_MODULE_DECLARE(lwm2m_rd_client_test);

/* zephyr/net/coap.h */
DEFINE_FAKE_VALUE_FUNC(uint8_t, coap_header_get_code, const struct coap_packet *);
uint8_t coap_header_get_code_fake_created(const struct coap_packet *cpkt)
{
	return COAP_RESPONSE_CODE_CREATED;
}
uint8_t coap_header_get_code_fake_deleted(const struct coap_packet *cpkt)
{
	return COAP_RESPONSE_CODE_DELETED;
}
uint8_t coap_header_get_code_fake_changed(const struct coap_packet *cpkt)
{
	return COAP_RESPONSE_CODE_CHANGED;
}
uint8_t coap_header_get_code_fake_bad_request(const struct coap_packet *cpkt)
{
	return COAP_RESPONSE_CODE_BAD_REQUEST;
}

DEFINE_FAKE_VALUE_FUNC(int, coap_append_option_int, struct coap_packet *, uint16_t, unsigned int);
DEFINE_FAKE_VALUE_FUNC(int, coap_packet_append_option, struct coap_packet *, uint16_t,
		       const uint8_t *, uint16_t);
int coap_packet_append_option_fake_err(struct coap_packet *cpkt, uint16_t code,
				       const uint8_t *value, uint16_t len)
{
	return -1;
}

DEFINE_FAKE_VALUE_FUNC(int, coap_packet_append_payload_marker, struct coap_packet *);
DEFINE_FAKE_VALUE_FUNC(int, coap_find_options, const struct coap_packet *, uint16_t,
		       struct coap_option *, uint16_t);
int coap_find_options_do_registration_reply_cb_ok(const struct coap_packet *cpkt, uint16_t code,
						  struct coap_option *options, uint16_t veclen)
{
	char options0[] = "rd";
	char options1[] = "jATO2yn9u7";

	options[1].len = sizeof(options0);
	memcpy(&options[1].value, &options0, sizeof(options0));
	options[1].len = sizeof(options1);
	memcpy(&options[1].value, &options1, sizeof(options1));
	return 3;
}

DEFINE_FAKE_VALUE_FUNC(uint16_t, coap_next_id);

/* zephyr/net/lwm2m.h */
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_start, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_stop, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_open_socket, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_u32, const struct lwm2m_obj_path *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_u16, const struct lwm2m_obj_path *, uint16_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_bool, const struct lwm2m_obj_path *, bool *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_u32, const struct lwm2m_obj_path *, uint32_t);
int lwm2m_get_bool_fake_default(const struct lwm2m_obj_path *path, bool *value)
{
	*value = false;
	return 0;
}
int lwm2m_get_bool_fake_true(const struct lwm2m_obj_path *path, bool *value)
{
	*value = true;
	return 0;
}

uint32_t get_u32_val;

int lwm2m_get_u32_val(const struct lwm2m_obj_path *path, uint32_t *val)
{
	*val = get_u32_val;
	return 0;
}


/* subsys/net/lib/lwm2m/lwm2m_engine.h */
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_socket_start, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_socket_close, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_close_socket, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_socket_suspend, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_security_inst_id_to_index, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_connection_resume, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_push_queued_buffers, struct lwm2m_ctx *);
DEFINE_FAKE_VOID_FUNC(lwm2m_engine_context_init, struct lwm2m_ctx *);
DEFINE_FAKE_VOID_FUNC(lwm2m_engine_context_close, struct lwm2m_ctx *);
DEFINE_FAKE_VALUE_FUNC(char *, lwm2m_sprint_ip_addr, const struct sockaddr *);
char *lwm2m_sprint_ip_addr_fake_default(const struct sockaddr *addr)
{
	return "192.168.1.1:4444";
}

DEFINE_FAKE_VALUE_FUNC(int, lwm2m_server_short_id_to_inst, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_security_index_to_inst_id, int);

k_work_handler_t service;
int64_t next;

int lwm2m_engine_call_at(k_work_handler_t work, int64_t timestamp)
{
	service = work;
	next = timestamp ? timestamp : 1;
	return 0;
}

uint16_t counter = RD_CLIENT_MAX_SERVICE_ITERATIONS;
struct lwm2m_message *pending_message;
void *(*pending_message_cb)();
static bool running;
K_SEM_DEFINE(srv_sem, 0, 1);

static void service_work_fn(struct k_work *work)
{
	while (running) {
		k_sleep(K_MSEC(10));
		if (pending_message != NULL && pending_message_cb != NULL) {
			pending_message_cb(pending_message);
			pending_message = NULL;
		}

		if (next && next < k_uptime_get()) {
			next = 0;
			service(NULL);
			k_sem_give(&srv_sem);
		}

		counter--;

		/* avoid endless loop if rd client is stuck somewhere */
		if (counter == 0) {
			printk("Counter!\n");
			break;
		}
	}
}

void wait_for_service(uint16_t cycles)
{
	while (cycles--) {
		k_sem_take(&srv_sem, K_MSEC(100));
	}
}

K_WORK_DEFINE(service_work, service_work_fn);

void test_lwm2m_engine_start_service(void)
{
	running = true;
	counter = RD_CLIENT_MAX_SERVICE_ITERATIONS;
	k_work_submit(&service_work);
	k_sem_reset(&srv_sem);
}

void test_lwm2m_engine_stop_service(void)
{
	pending_message_cb = NULL;
	running = false;
	k_work_cancel(&service_work);
}

/* subsys/net/lib/lwm2m/lwm2m_message_handling.h */
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_init_message, struct lwm2m_message *);
DEFINE_FAKE_VOID_FUNC(lwm2m_clear_block_contexts);
int lwm2m_init_message_fake_default(struct lwm2m_message *msg)
{
	pending_message = msg;
	return 0;
}

void test_prepare_pending_message_cb(void *cb)
{
	pending_message_cb = cb;
}

DEFINE_FAKE_VOID_FUNC(lwm2m_reset_message, struct lwm2m_message *, bool);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_send_message_async, struct lwm2m_message *);

/* subsys/net/lib/lwm2m/lwm2m_registry.h */
DEFINE_FAKE_VOID_FUNC(lwm2m_engine_get_binding, char *);
DEFINE_FAKE_VOID_FUNC(lwm2m_engine_get_queue_mode, char *);

/* subsys/net/lib/lwm2m/lwm2m_rw_link_format.h */
FAKE_VALUE_FUNC(int, put_begin, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_end, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_begin_oi, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_end_oi, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_begin_r, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_end_r, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_begin_ri, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_end_ri, struct lwm2m_output_context *, struct lwm2m_obj_path *);
FAKE_VALUE_FUNC(int, put_s8, struct lwm2m_output_context *, struct lwm2m_obj_path *, int8_t);
FAKE_VALUE_FUNC(int, put_s16, struct lwm2m_output_context *, struct lwm2m_obj_path *, int16_t);
FAKE_VALUE_FUNC(int, put_s32, struct lwm2m_output_context *, struct lwm2m_obj_path *, int32_t);
FAKE_VALUE_FUNC(int, put_s64, struct lwm2m_output_context *, struct lwm2m_obj_path *, int64_t);
FAKE_VALUE_FUNC(int, put_time, struct lwm2m_output_context *, struct lwm2m_obj_path *, time_t);
FAKE_VALUE_FUNC(int, put_string, struct lwm2m_output_context *, struct lwm2m_obj_path *, char *,
		size_t);
FAKE_VALUE_FUNC(int, put_float, struct lwm2m_output_context *, struct lwm2m_obj_path *, double *);
FAKE_VALUE_FUNC(int, put_bool, struct lwm2m_output_context *, struct lwm2m_obj_path *, bool);
FAKE_VALUE_FUNC(int, put_opaque, struct lwm2m_output_context *, struct lwm2m_obj_path *, char *,
		size_t);
FAKE_VALUE_FUNC(int, put_objlnk, struct lwm2m_output_context *, struct lwm2m_obj_path *,
		struct lwm2m_objlnk *);
FAKE_VALUE_FUNC(int, put_corelink, struct lwm2m_output_context *, const struct lwm2m_obj_path *);

const struct lwm2m_writer link_format_writer = {
	.put_begin = put_begin,
	.put_end = put_end,
	.put_begin_oi = put_begin_oi,
	.put_end_oi = put_end_oi,
	.put_begin_r = put_begin_r,
	.put_end_r = put_end_r,
	.put_begin_ri = put_begin_oi,
	.put_end_ri = put_end_oi,
	.put_s8 = put_s8,
	.put_s16 = put_s16,
	.put_s32 = put_s32,
	.put_s64 = put_s64,
	.put_time = put_time,
	.put_string = put_string,
	.put_float = put_float,
	.put_bool = put_bool,
	.put_opaque = put_opaque,
	.put_objlnk = put_objlnk,
	.put_corelink = put_corelink,
};

DEFINE_FAKE_VALUE_FUNC(int, do_register_op_link_format, struct lwm2m_message *);
