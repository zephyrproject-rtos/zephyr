/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Foundries.io
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

/* Use this value to skip token generation */
#define LWM2M_MSG_TOKEN_LEN_SKIP	0xFF

/* length of time in milliseconds to wait for buffer allocations */
#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

/* coap reply status */
#define COAP_REPLY_STATUS_NONE		0
#define COAP_REPLY_STATUS_ERROR		1

struct lwm2m_message;

/* Establish a message timeout callback */
typedef void (*lwm2m_message_timeout_cb_t)(struct lwm2m_message *msg);

/* Internal LwM2M message structure to track in-flight messages. */
struct lwm2m_message {
	/** LwM2M context related to this message */
	struct lwm2m_ctx *ctx;

	/** CoAP packet data related to this message */
	struct coap_packet cpkt;

	/** Message transmission handling for TYPE_CON */
	struct coap_pending *pending;
	struct coap_reply *reply;

	/** Message configuration */
	u8_t *token;
	coap_reply_t reply_cb;
	lwm2m_message_timeout_cb_t message_timeout_cb;
	u16_t mid;
	u8_t type;
	u8_t code;
	u8_t tkl;

	/** Counter for message re-send / abort handling */
	u8_t send_attempts;
};

/* Establish a request handler callback type */
typedef int (*udp_request_handler_cb_t)(struct coap_packet *request,
					struct lwm2m_message *msg);

char *lwm2m_sprint_ip_addr(const struct sockaddr *addr);

int lwm2m_notify_observer(u16_t obj_id, u16_t obj_inst_id, u16_t res_id);
int lwm2m_notify_observer_path(struct lwm2m_obj_path *path);

void lwm2m_register_obj(struct lwm2m_engine_obj *obj);
void lwm2m_unregister_obj(struct lwm2m_engine_obj *obj);
struct lwm2m_engine_obj_field *
lwm2m_get_engine_obj_field(struct lwm2m_engine_obj *obj, int res_id);
int  lwm2m_create_obj_inst(u16_t obj_id, u16_t obj_inst_id,
			   struct lwm2m_engine_obj_inst **obj_inst);
int  lwm2m_delete_obj_inst(u16_t obj_id, u16_t obj_inst_id);
int  lwm2m_get_or_create_engine_obj(struct lwm2m_engine_context *context,
				    struct lwm2m_engine_obj_inst **obj_inst,
				    u8_t *created);

/* LwM2M context functions */
void lwm2m_engine_context_init(struct lwm2m_ctx *client_ctx);

/* LwM2M message functions */
struct lwm2m_message *lwm2m_get_message(struct lwm2m_ctx *client_ctx);
void lwm2m_reset_message(struct lwm2m_message *msg, bool release);
int lwm2m_init_message(struct lwm2m_message *msg);
int lwm2m_send_message(struct lwm2m_message *msg);

u16_t lwm2m_get_rd_data(u8_t *client_data, u16_t size);

int lwm2m_perform_read_op(struct lwm2m_engine_obj *obj,
			  struct lwm2m_engine_context *context,
			  u16_t content_format);

int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst,
			struct lwm2m_engine_res_inst *res,
			struct lwm2m_engine_obj_field *obj_field,
			struct lwm2m_engine_context *context);

void lwm2m_udp_receive(struct lwm2m_ctx *client_ctx, struct net_pkt *pkt,
		       bool handle_separate_response,
		       udp_request_handler_cb_t udp_request_handler);

enum coap_block_size lwm2m_default_block_size(void);

int lwm2m_engine_add_service(void (*service)(void), u32_t period_ms);

int lwm2m_engine_get_resource(char *pathstr,
			      struct lwm2m_engine_res_inst **res);

size_t lwm2m_engine_get_opaque_more(struct lwm2m_input_context *in,
				    u8_t *buf, size_t buflen, bool *last_block);

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
u8_t lwm2m_firmware_get_update_state(void);
void lwm2m_firmware_set_update_state(u8_t state);
void lwm2m_firmware_set_update_result(u8_t result);
u8_t lwm2m_firmware_get_update_result(void);
#endif

#endif /* LWM2M_ENGINE_H */
