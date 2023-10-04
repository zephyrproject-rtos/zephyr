/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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
#include <zephyr/net/tls_credentials.h>

#include "lwm2m_message_handling.h"

#define ZSOCK_POLLIN  1
#define ZSOCK_POLLOUT 4

void set_socket_events(short events);
void clear_socket_events(void);

DECLARE_FAKE_VALUE_FUNC(int, lwm2m_rd_client_pause);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_rd_client_resume);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_message *, find_msg, struct coap_pending *,
			struct coap_reply *);
DECLARE_FAKE_VOID_FUNC(coap_pending_clear, struct coap_pending *);
DECLARE_FAKE_VOID_FUNC(lwm2m_reset_message, struct lwm2m_message *, bool);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_send_message_async, struct lwm2m_message *);
DECLARE_FAKE_VOID_FUNC(lwm2m_registry_lock);
DECLARE_FAKE_VOID_FUNC(lwm2m_registry_unlock);
DECLARE_FAKE_VALUE_FUNC(bool, coap_pending_cycle, struct coap_pending *);
DECLARE_FAKE_VALUE_FUNC(int, generate_notify_message, struct lwm2m_ctx *, struct observe_node *,
			void *);
DECLARE_FAKE_VALUE_FUNC(int64_t, engine_observe_shedule_next_event, struct observe_node *, uint16_t,
			const int64_t);
DECLARE_FAKE_VALUE_FUNC(int, handle_request, struct coap_packet *, struct lwm2m_message *);
DECLARE_FAKE_VOID_FUNC(lwm2m_udp_receive, struct lwm2m_ctx *, uint8_t *, uint16_t,
		       struct sockaddr *, udp_request_handler_cb_t);
DECLARE_FAKE_VALUE_FUNC(bool, lwm2m_rd_client_is_registred, struct lwm2m_ctx *);
DECLARE_FAKE_VOID_FUNC(lwm2m_engine_context_close, struct lwm2m_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_res_buf, const struct lwm2m_obj_path *, void **, uint16_t *,
			uint16_t *, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_parse_peerinfo, char *, struct lwm2m_ctx *, bool);
DECLARE_FAKE_VALUE_FUNC(int, tls_credential_add, sec_tag_t, enum tls_credential_type, const void *,
			size_t);
DECLARE_FAKE_VALUE_FUNC(int, tls_credential_delete, sec_tag_t, enum tls_credential_type);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_engine_obj_field *, lwm2m_get_engine_obj_field,
			struct lwm2m_engine_obj *, int);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_bool, const struct lwm2m_obj_path *, bool *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_delete_obj_inst, uint16_t, uint16_t);
DECLARE_FAKE_VOID_FUNC(lwm2m_clear_block_contexts);
DECLARE_FAKE_VALUE_FUNC(int, z_impl_zsock_connect, int, const struct sockaddr *, socklen_t);

#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(lwm2m_rd_client_pause)                                                        \
		FUNC(lwm2m_rd_client_resume)                                                       \
		FUNC(find_msg)                                                                     \
		FUNC(coap_pending_clear)                                                           \
		FUNC(lwm2m_reset_message)                                                          \
		FUNC(lwm2m_send_message_async)                                                     \
		FUNC(lwm2m_registry_lock)                                                          \
		FUNC(lwm2m_registry_unlock)                                                        \
		FUNC(coap_pending_cycle)                                                           \
		FUNC(generate_notify_message)                                                      \
		FUNC(engine_observe_shedule_next_event)                                            \
		FUNC(handle_request)                                                               \
		FUNC(lwm2m_udp_receive)                                                            \
		FUNC(lwm2m_rd_client_is_registred)                                                 \
		FUNC(lwm2m_engine_context_close)                                                   \
		FUNC(lwm2m_get_res_buf)                                                            \
		FUNC(lwm2m_parse_peerinfo)                                                         \
		FUNC(tls_credential_add)                                                           \
		FUNC(tls_credential_delete)                                                        \
		FUNC(lwm2m_get_engine_obj_field)                                                   \
		FUNC(lwm2m_get_bool)                                                               \
		FUNC(lwm2m_delete_obj_inst)                                                        \
		FUNC(lwm2m_clear_block_contexts)                                                   \
		FUNC(z_impl_zsock_connect)                                                         \
	} while (0)

#endif /* STUBS_H */
