/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_ENGINE_H
#define LWM2M_ENGINE_H

#include "lwm2m_message_handling.h"
#include "lwm2m_object.h"
#include "lwm2m_observation.h"
#include "lwm2m_registry.h"

#define LWM2M_PROTOCOL_VERSION_MAJOR 1
#if CONFIG_LWM2M_VERSION_1_1
#define LWM2M_PROTOCOL_VERSION_MINOR 1
#else
#define LWM2M_PROTOCOL_VERSION_MINOR 0
#endif

#define LWM2M_PROTOCOL_VERSION_STRING                                                              \
	STRINGIFY(LWM2M_PROTOCOL_VERSION_MAJOR)                                                    \
	"." STRINGIFY(LWM2M_PROTOCOL_VERSION_MINOR)

/* Use this value to generate new token */
#define LWM2M_MSG_TOKEN_GENERATE_NEW 0xFFU

/* length of time in milliseconds to wait for buffer allocations */
#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

char *lwm2m_sprint_ip_addr(const struct sockaddr *addr);
char *sprint_token(const uint8_t *token, uint8_t tkl);
/* Validate write access to object. */
int lwm2m_engine_validate_write_access(struct lwm2m_message *msg,
				       struct lwm2m_engine_obj_inst *obj_inst,
				       struct lwm2m_engine_obj_field **obj_field);

/* LwM2M context functions */
int lwm2m_engine_context_close(struct lwm2m_ctx *client_ctx);
void lwm2m_engine_context_init(struct lwm2m_ctx *client_ctx);

/* Message buffer functions */
uint8_t *lwm2m_get_message_buf(void);
int lwm2m_put_message_buf(uint8_t *buf);

int lwm2m_get_path_reference_ptr(struct lwm2m_engine_obj *obj, struct lwm2m_obj_path *path,
				 void **ref);

int lwm2m_perform_composite_observation_op(struct lwm2m_message *msg, uint8_t *token,
					   uint8_t token_length, sys_slist_t *lwm2m_path_list);

bool lwm2m_engine_bootstrap_override(struct lwm2m_ctx *client_ctx, struct lwm2m_obj_path *path);
int bootstrap_delete(struct lwm2m_message *msg);

int lwm2m_engine_add_service(k_work_handler_t service, uint32_t period_ms);

int lwm2m_security_inst_id_to_index(uint16_t obj_inst_id);
int lwm2m_security_index_to_inst_id(int index);

int32_t lwm2m_server_get_pmin(uint16_t obj_inst_id);
int32_t lwm2m_server_get_pmax(uint16_t obj_inst_id);
int lwm2m_server_get_ssid(uint16_t obj_inst_id);
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

/* Network Layer */
int lwm2m_socket_add(struct lwm2m_ctx *ctx);
void lwm2m_socket_del(struct lwm2m_ctx *ctx);
int lwm2m_socket_start(struct lwm2m_ctx *client_ctx);
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
int lwm2m_engine_close_socket_connection(struct lwm2m_ctx *client_ctx);
int lwm2m_engine_connection_resume(struct lwm2m_ctx *client_ctx);
int lwm2m_push_queued_buffers(struct lwm2m_ctx *client_ctx);
#endif

/* Resources */
struct lwm2m_ctx **lwm2m_sock_ctx(void);
int lwm2m_sock_nfds(void);
#endif /* LWM2M_ENGINE_H */
