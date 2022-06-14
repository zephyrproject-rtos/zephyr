/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_ENGINE_H
#define LWM2M_ENGINE_H

#include "lwm2m_object.h"

#define LWM2M_PROTOCOL_VERSION_MAJOR 1
#if CONFIG_LWM2M_VERSION_1_1
#define LWM2M_PROTOCOL_VERSION_MINOR 1
#else
#define LWM2M_PROTOCOL_VERSION_MINOR 0
#endif

#define LWM2M_PROTOCOL_VERSION_STRING STRINGIFY(LWM2M_PROTOCOL_VERSION_MAJOR) \
				      "." \
				      STRINGIFY(LWM2M_PROTOCOL_VERSION_MINOR)

/* LWM2M / CoAP Content-Formats */
#define LWM2M_FORMAT_PLAIN_TEXT		0
#define LWM2M_FORMAT_APP_LINK_FORMAT	40
#define LWM2M_FORMAT_APP_OCTET_STREAM	42
#define LWM2M_FORMAT_APP_EXI		47
#define LWM2M_FORMAT_APP_JSON		50
#define LWM2M_FORMAT_APP_CBOR		60
#define LWM2M_FORMAT_APP_SEML_JSON	110
#define LWM2M_FORMAT_APP_SENML_CBOR	112
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

/* path object list */
struct lwm2m_obj_path_list {
	sys_snode_t node;
	struct lwm2m_obj_path path;
};

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
/* Validate write access to object. */
int lwm2m_engine_validate_write_access(struct lwm2m_message *msg,
				       struct lwm2m_engine_obj_inst *obj_inst,
				       struct lwm2m_engine_obj_field **obj_field);
/* Create or Allocate resource instance. */
int lwm2m_engine_get_create_res_inst(struct lwm2m_obj_path *path, struct lwm2m_engine_res **res,
				     struct lwm2m_engine_res_inst **res_inst);

struct lwm2m_engine_obj *lwm2m_engine_get_obj(
					const struct lwm2m_obj_path *path);
struct lwm2m_engine_obj_inst *lwm2m_engine_get_obj_inst(
					const struct lwm2m_obj_path *path);
struct lwm2m_engine_res *lwm2m_engine_get_res(
					const struct lwm2m_obj_path *path);
struct lwm2m_engine_res_inst *lwm2m_engine_get_res_inst(
					const struct lwm2m_obj_path *path);

bool lwm2m_engine_shall_report_obj_version(const struct lwm2m_engine_obj *obj);

/* LwM2M context functions */
int lwm2m_engine_context_close(struct lwm2m_ctx *client_ctx);
void lwm2m_engine_context_init(struct lwm2m_ctx *client_ctx);

/* Message buffer functions */
uint8_t *lwm2m_get_message_buf(void);
int lwm2m_put_message_buf(uint8_t *buf);

/* Initialize path list */
void lwm2m_engine_path_list_init(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list,
				 struct lwm2m_obj_path_list path_object_buf[],
				 uint8_t path_object_size);
/* Add new Path to the list */
int lwm2m_engine_add_path_to_list(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list,
				  struct lwm2m_obj_path *path);
/* Remove paths when parent already exist in the list. */
void lwm2m_engine_clear_duplicate_path(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list);

/* LwM2M message functions */
struct lwm2m_message *lwm2m_get_message(struct lwm2m_ctx *client_ctx);
void lwm2m_reset_message(struct lwm2m_message *msg, bool release);
void lm2m_message_clear_allocations(struct lwm2m_message *msg);
int lwm2m_init_message(struct lwm2m_message *msg);
int lwm2m_send_message_async(struct lwm2m_message *msg);
/* Notification and Send operation */
int lwm2m_information_interface_send(struct lwm2m_message *msg);
int lwm2m_send_empty_ack(struct lwm2m_ctx *client_ctx, uint16_t mid);

int lwm2m_register_payload_handler(struct lwm2m_message *msg);

int lwm2m_perform_read_op(struct lwm2m_message *msg, uint16_t content_format);

int lwm2m_perform_composite_read_op(struct lwm2m_message *msg, uint16_t content_format,
				    sys_slist_t *lwm2m_path_list);

int lwm2m_perform_composite_observation_op(struct lwm2m_message *msg, uint8_t *token,
					   uint8_t token_length, sys_slist_t *lwm2m_path_list);

int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst,
			struct lwm2m_engine_res *res,
			struct lwm2m_engine_res_inst *res_inst,
			struct lwm2m_engine_obj_field *obj_field,
			struct lwm2m_message *msg);

bool lwm2m_engine_bootstrap_override(struct lwm2m_ctx *client_ctx, struct lwm2m_obj_path *path);

int lwm2m_discover_handler(struct lwm2m_message *msg, bool is_bootstrap);

enum coap_block_size lwm2m_default_block_size(void);

int lwm2m_engine_add_service(k_work_handler_t service, uint32_t period_ms);

int lwm2m_engine_get_resource(const char *pathstr,
			      struct lwm2m_engine_res **res);

void lwm2m_engine_get_binding(char *binding);
void lwm2m_engine_get_queue_mode(char *queue);

size_t lwm2m_engine_get_opaque_more(struct lwm2m_input_context *in,
				    uint8_t *buf, size_t buflen,
				    struct lwm2m_opaque_context *opaque,
				    bool *last_block);

int lwm2m_security_inst_id_to_index(uint16_t obj_inst_id);
int lwm2m_security_index_to_inst_id(int index);

int32_t lwm2m_server_get_pmin(uint16_t obj_inst_id);
int32_t lwm2m_server_get_pmax(uint16_t obj_inst_id);
int lwm2m_server_short_id_to_inst(uint16_t short_id);

#if defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)
bool lwm2m_server_get_mute_send(uint16_t obj_inst_id);
#endif

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
void lwm2m_firmware_set_update_state_inst(uint16_t obj_inst_id, uint8_t state);
void lwm2m_firmware_set_update_result_inst(uint16_t obj_inst_id, uint8_t result);
void lwm2m_firmware_set_update_state(uint8_t state);
void lwm2m_firmware_set_update_result(uint8_t result);
uint8_t lwm2m_firmware_get_update_state_inst(uint16_t obj_inst_id);
uint8_t lwm2m_firmware_get_update_state(void);
uint8_t lwm2m_firmware_get_update_result_inst(uint16_t obj_inst_id);
uint8_t lwm2m_firmware_get_update_result(void);
#endif

/* Attribute handling. */

struct lwm2m_attr *lwm2m_engine_get_next_attr(const void *ref,
					      struct lwm2m_attr *prev);
const char *lwm2m_engine_get_attr_name(const struct lwm2m_attr *attr);

/* Network Layer */
int  lwm2m_socket_add(struct lwm2m_ctx *ctx);
void lwm2m_socket_del(struct lwm2m_ctx *ctx);
int  lwm2m_socket_start(struct lwm2m_ctx *client_ctx);
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
int lwm2m_engine_close_socket_connection(struct lwm2m_ctx *client_ctx);
int lwm2m_engine_connection_resume(struct lwm2m_ctx *client_ctx);
int lwm2m_push_queued_buffers(struct lwm2m_ctx *client_ctx);
#endif
int  lwm2m_parse_peerinfo(char *url, struct lwm2m_ctx *client_ctx, bool is_firmware_uri);

#endif /* LWM2M_ENGINE_H */
