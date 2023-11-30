/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STUBS_H
#define STUBS_H

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/fff.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/ztest.h>

#include <lwm2m_engine.h>

/* Number of iterations the state machine within the RDClient service
 * is triggered
 */
static const uint16_t RD_CLIENT_MAX_SERVICE_ITERATIONS = UINT16_MAX;

/* zephyr/net/coap.h */
DECLARE_FAKE_VALUE_FUNC(uint8_t, coap_header_get_code, const struct coap_packet *);
uint8_t coap_header_get_code_fake_created(const struct coap_packet *cpkt);
uint8_t coap_header_get_code_fake_deleted(const struct coap_packet *cpkt);
uint8_t coap_header_get_code_fake_changed(const struct coap_packet *cpkt);
uint8_t coap_header_get_code_fake_bad_request(const struct coap_packet *cpkt);
DECLARE_FAKE_VALUE_FUNC(int, coap_append_option_int, struct coap_packet *, uint16_t, unsigned int);
DECLARE_FAKE_VALUE_FUNC(int, coap_packet_append_option, struct coap_packet *, uint16_t,
			const uint8_t *, uint16_t);
int coap_packet_append_option_fake_err(struct coap_packet *cpkt, uint16_t code,
				       const uint8_t *value, uint16_t len);
DECLARE_FAKE_VALUE_FUNC(int, coap_packet_append_payload_marker, struct coap_packet *);
DECLARE_FAKE_VALUE_FUNC(int, coap_find_options, const struct coap_packet *, uint16_t,
			struct coap_option *, uint16_t);
int coap_find_options_do_registration_reply_cb_ok(const struct coap_packet *cpkt, uint16_t code,
						  struct coap_option *options, uint16_t veclen);
DECLARE_FAKE_VALUE_FUNC(uint16_t, coap_next_id);

/* zephyr/net/lwm2m.h */
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_start, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_stop, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_open_socket, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_u32, const struct lwm2m_obj_path *, uint32_t *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_u16, const struct lwm2m_obj_path *, uint16_t *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_bool, const struct lwm2m_obj_path *, bool *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_u32, const struct lwm2m_obj_path *, uint32_t);
int lwm2m_get_bool_fake_default(const struct lwm2m_obj_path *path, bool *value);
int lwm2m_get_bool_fake_true(const struct lwm2m_obj_path *path, bool *value);
extern uint32_t get_u32_val;
int lwm2m_get_u32_val(const struct lwm2m_obj_path *path, uint32_t *val);

/* subsys/net/lib/lwm2m/lwm2m_engine.h */
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_socket_start, struct lwm2m_ctx *);
int lwm2m_socket_start_fake_fail(struct lwm2m_ctx *client_ctx);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_socket_close, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_close_socket, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_socket_suspend, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_security_inst_id_to_index, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_security_short_id_to_inst, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_security_index_to_inst_id, int);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_connection_resume, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_push_queued_buffers, struct lwm2m_ctx *);
DECLARE_FAKE_VOID_FUNC(lwm2m_engine_context_init, struct lwm2m_ctx *);
void lwm2m_engine_context_init_fake1(struct lwm2m_ctx *client_ctx);
void test_throw_network_error_from_engine(int err);
DECLARE_FAKE_VOID_FUNC(lwm2m_engine_context_close, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(char *, lwm2m_sprint_ip_addr, const struct sockaddr *);
char *lwm2m_sprint_ip_addr_fake_default(const struct sockaddr *addr);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_server_short_id_to_inst, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_server_disable, uint16_t, k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(uint8_t, lwm2m_server_get_prio, uint16_t);
DECLARE_FAKE_VOID_FUNC(lwm2m_server_reset_timestamps);


void wait_for_service(uint16_t cycles);
void test_lwm2m_engine_start_service(void);
void test_lwm2m_engine_stop_service(void);
void stub_lwm2m_server_disable(bool disable);

/* subsys/net/lib/lwm2m/lwm2m_message_handling.h  */
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_init_message, struct lwm2m_message *);
DECLARE_FAKE_VOID_FUNC(lwm2m_clear_block_contexts);
int lwm2m_init_message_fake_default(struct lwm2m_message *msg);
void test_prepare_pending_message_cb(void *cb);

DECLARE_FAKE_VOID_FUNC(lwm2m_reset_message, struct lwm2m_message *, bool);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_send_message_async, struct lwm2m_message *);

/* subsys/net/lib/lwm2m/lwm2m_registry.h */
DECLARE_FAKE_VOID_FUNC(lwm2m_engine_get_binding, char *);
DECLARE_FAKE_VOID_FUNC(lwm2m_engine_get_queue_mode, char *);

/* subsys/net/lib/lwm2m/lwm2m_rw_link_format.h */
DECLARE_FAKE_VALUE_FUNC(int, do_register_op_link_format, struct lwm2m_message *);

#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(coap_header_get_code)                                                         \
		FUNC(coap_append_option_int)                                                       \
		FUNC(coap_packet_append_option)                                                    \
		FUNC(coap_packet_append_payload_marker)                                            \
		FUNC(coap_find_options)                                                            \
		FUNC(coap_next_id)                                                                 \
		FUNC(lwm2m_engine_start)                                                           \
		FUNC(lwm2m_engine_stop)                                                            \
		FUNC(lwm2m_get_u32)                                                                \
		FUNC(lwm2m_get_u16)                                                                \
		FUNC(lwm2m_get_bool)                                                               \
		FUNC(lwm2m_socket_start)                                                           \
		FUNC(lwm2m_socket_close)                                                           \
		FUNC(lwm2m_close_socket)                                                           \
		FUNC(lwm2m_security_inst_id_to_index)                                              \
		FUNC(lwm2m_engine_connection_resume)                                               \
		FUNC(lwm2m_push_queued_buffers)                                                    \
		FUNC(lwm2m_engine_context_init)                                                    \
		FUNC(lwm2m_engine_context_close)                                                   \
		FUNC(lwm2m_sprint_ip_addr)                                                         \
		FUNC(lwm2m_server_short_id_to_inst)                                                \
		FUNC(lwm2m_security_index_to_inst_id)                                              \
		FUNC(lwm2m_init_message)                                                           \
		FUNC(lwm2m_reset_message)                                                          \
		FUNC(lwm2m_send_message_async)                                                     \
		FUNC(lwm2m_engine_get_binding)                                                     \
		FUNC(lwm2m_engine_get_queue_mode)                                                  \
		FUNC(do_register_op_link_format)                                                   \
		FUNC(lwm2m_clear_block_contexts)                                                   \
	} while (0)

#endif /* STUBS_H */
