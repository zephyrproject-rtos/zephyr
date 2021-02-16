/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_ENGINE_H
#define LWM2M_ENGINE_H

#include "lwm2m_object.h"

#define LWM2M_PROTOCOL_VERSION "1.0"

/* LWM2M / CoAP Content-Formats */
#define LWM2M_FORMAT_PLAIN_TEXT		0
#define LWM2M_FORMAT_APP_LINK_FORMAT	40
#define LWM2M_FORMAT_APP_OCTET_STREAM	42
#define LWM2M_FORMAT_APP_EXI		47
#define LWM2M_FORMAT_APP_JSON		50
#define LWM2M_FORMAT_OMA_PLAIN_TEXT	1541
#define LWM2M_FORMAT_OMA_OLD_TLV	1542
#define LWM2M_FORMAT_OMA_OLD_JSON	1543
#define LWM2M_FORMAT_OMA_OLD_OPAQUE	1544
#define LWM2M_FORMAT_OMA_TLV		11542
#define LWM2M_FORMAT_OMA_JSON		11543
/* 65000 ~ 65535 inclusive are reserved for experiments */
#define LWM2M_FORMAT_NONE		65535


#define COAP_RESPONSE_CODE_CLASS(x)	(x >> 5)
#define COAP_RESPONSE_CODE_DETAIL(x)	(x & 0x1F)

/* TODO: */
#define NOTIFY_OBSERVER(o, i, r)	lwm2m_notify_observer(o, i, r)
#define NOTIFY_OBSERVER_PATH(path)	lwm2m_notify_observer_path(path)

/* Use this value to generate new token */
#define LWM2M_MSG_TOKEN_GENERATE_NEW 0xFFU

/* length of time in milliseconds to wait for buffer allocations */
#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

/* coap reply status */
#define COAP_REPLY_STATUS_NONE		0
#define COAP_REPLY_STATUS_ERROR		1

/* Establish a request handler callback type */
typedef int (*udp_request_handler_cb_t)(struct coap_packet *request,
					struct lwm2m_message *msg);

char *lwm2m_sprint_ip_addr(const struct sockaddr *addr);

int lwm2m_notify_observer(uint16_t obj_id, uint16_t obj_inst_id, uint16_t res_id);
int lwm2m_notify_observer_path(struct lwm2m_obj_path *path);

void lwm2m_register_obj(struct lwm2m_engine_obj *obj);
void lwm2m_unregister_obj(struct lwm2m_engine_obj *obj);
struct lwm2m_engine_obj_field *
lwm2m_get_engine_obj_field(struct lwm2m_engine_obj *obj, int res_id);
int  lwm2m_create_obj_inst(uint16_t obj_id, uint16_t obj_inst_id,
			   struct lwm2m_engine_obj_inst **obj_inst);
int  lwm2m_delete_obj_inst(uint16_t obj_id, uint16_t obj_inst_id);
int  lwm2m_get_or_create_engine_obj(struct lwm2m_message *msg,
				    struct lwm2m_engine_obj_inst **obj_inst,
				    uint8_t *created);

/* LwM2M context functions */
int lwm2m_engine_context_close(struct lwm2m_ctx *client_ctx);
void lwm2m_engine_context_init(struct lwm2m_ctx *client_ctx);

/* Message buffer functions */
uint8_t *lwm2m_get_message_buf(void);
int lwm2m_put_message_buf(uint8_t *buf);

/* LwM2M message functions */
struct lwm2m_message *lwm2m_get_message(struct lwm2m_ctx *client_ctx);
void lwm2m_reset_message(struct lwm2m_message *msg, bool release);
int lwm2m_init_message(struct lwm2m_message *msg);
int lwm2m_send_message(struct lwm2m_message *msg);
int lwm2m_send_empty_ack(struct lwm2m_ctx *client_ctx, uint16_t mid);

uint16_t lwm2m_get_rd_data(uint8_t *client_data, uint16_t size);

int lwm2m_perform_read_op(struct lwm2m_message *msg, uint16_t content_format);

int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst,
			struct lwm2m_engine_res *res,
			struct lwm2m_engine_res_inst *res_inst,
			struct lwm2m_engine_obj_field *obj_field,
			struct lwm2m_message *msg);

enum coap_block_size lwm2m_default_block_size(void);

int lwm2m_engine_add_service(k_work_handler_t service, uint32_t period_ms);

int lwm2m_engine_get_resource(char *pathstr,
			      struct lwm2m_engine_res **res);

void lwm2m_engine_get_binding(char *binding);

size_t lwm2m_engine_get_opaque_more(struct lwm2m_input_context *in,
				    uint8_t *buf, size_t buflen,
				    struct lwm2m_opaque_context *opaque,
				    bool *last_block);

int lwm2m_security_inst_id_to_index(uint16_t obj_inst_id);
int lwm2m_security_index_to_inst_id(int index);

int32_t lwm2m_server_get_pmin(uint16_t obj_inst_id);
int32_t lwm2m_server_get_pmax(uint16_t obj_inst_id);
int lwm2m_server_short_id_to_inst(uint16_t short_id);

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
uint8_t lwm2m_firmware_get_update_state(void);
void lwm2m_firmware_set_update_state(uint8_t state);
void lwm2m_firmware_set_update_result(uint8_t result);
uint8_t lwm2m_firmware_get_update_result(void);
#endif

/* Network Layer */
int  lwm2m_socket_add(struct lwm2m_ctx *ctx);
void lwm2m_socket_del(struct lwm2m_ctx *ctx);
int  lwm2m_socket_start(struct lwm2m_ctx *client_ctx);
int  lwm2m_parse_peerinfo(char *url, struct sockaddr *addr, bool *use_dtls);

#endif /* LWM2M_ENGINE_H */
