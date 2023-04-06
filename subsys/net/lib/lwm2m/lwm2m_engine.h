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
#if defined(CONFIG_LWM2M_VERSION_1_1)
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

/**
 * @brief Validates that writing is a legal operation on the field given by the object in
 * @p obj_inst and the resource id in @p msg. Returns the field to obj_field (if it exists).
 *
 * @param[in] msg lwm2m message to signal for which resource the write access should checked
 * @param[in] obj_inst Engine object instance to signal which object the resource belongs to
 * @param[out] obj_field Engine obejct field buffer pointer to store the field being checked
 * @return 0 for successful validation and negative in all other cases
 */
int lwm2m_engine_validate_write_access(struct lwm2m_message *msg,
				       struct lwm2m_engine_obj_inst *obj_inst,
				       struct lwm2m_engine_obj_field **obj_field);

/* LwM2M context functions */
/**
 * @brief Removes all observes, clears all pending coap messages and resets all messages
 * associated with this context.
 *
 * @param[in] client_ctx Context to close
 */
void lwm2m_engine_context_close(struct lwm2m_ctx *client_ctx);

/**
 * @brief Initializes a lwm2m_ctx before a socket can be associated with it.
 *
 * @param[in] client_ctx Context to initialize
 */
void lwm2m_engine_context_init(struct lwm2m_ctx *client_ctx);

/* Message buffer functions */
uint8_t *lwm2m_get_message_buf(void);
int lwm2m_put_message_buf(uint8_t *buf);
int lwm2m_perform_composite_observation_op(struct lwm2m_message *msg, uint8_t *token,
					   uint8_t token_length, sys_slist_t *lwm2m_path_list);

/**
 * @brief Checks if context has access to the object specified by @p path
 *
 * @param[in] client_ctx lwm2m ctx of the connection
 * @param[in] path Path of the object
 * @return Returns false if the bootstrap flag of @p client_ctx is not set
 */
bool lwm2m_engine_bootstrap_override(struct lwm2m_ctx *client_ctx, struct lwm2m_obj_path *path);

/**
 * @brief Deletes the object instance specified by msg->path. If the object instance id of the
 * path is null (i.e path.level == 1), all object instances of that object will be deleted.
 * Should only be called if delete request comes from a bootstrap server.
 *
 * @param[in] msg lwm2m message with operation delete
 * @return 0 for success or negative in case of error
 */
int bootstrap_delete(struct lwm2m_message *msg);

/**
 * @brief Adds the periodic @p service to the work queue. The period of the service becomes
 * @p period_ms.
 *
 * @param[in] service Periodic service to be added
 * @param[in] period_ms Period of the periodic service
 * @return 0 for success or negative in case of error
 */
int lwm2m_engine_add_service(k_work_handler_t service, uint32_t period_ms);

/**
 * @brief Returns the index in the security objects list corresponding to the object instance
 * id given by @p obj_inst_id
 *
 * @param[in] obj_inst_id object instance id of the security instance
 * @return index in the list or negative in case of error
 */
int lwm2m_security_inst_id_to_index(uint16_t obj_inst_id);

/**
 * @brief Returns the object instance id of the security object instance at @p index
 * in the security object list.
 *
 * @param[in] index Index in the security object list
 * @return Object instance id of the security instance or negative in case of error
 */
int lwm2m_security_index_to_inst_id(int index);

/**
 * @brief Returns the default minimum period for an observation set for the server
 * with object instance id given by @p obj_inst_id.
 *
 * @param[in] obj_inst_id Object instance id of the server object instance
 * @return int32_t pmin value
 */
int32_t lwm2m_server_get_pmin(uint16_t obj_inst_id);

/**
 * @brief Returns the default maximum period for an observation set for the server
 * with object instance id given by @p obj_inst_id.
 *
 * @param[in] obj_inst_id Object instance id of the server object instance
 * @return int32_t pmax value
 */
int32_t lwm2m_server_get_pmax(uint16_t obj_inst_id);

/**
 * @brief Returns the Short Server ID of the server object instance with
 * object instance id given by @p obj_inst_id.
 *
 * @param[in] obj_inst_id Object instance id of the server object
 * @return SSID or negative in case not found
 */
int lwm2m_server_get_ssid(uint16_t obj_inst_id);

/**
 * @brief Returns the object instance id of the server having ssid given by @p short_id.
 *
 * @param[in] short_id ssid of the server object
 * @return Object instance id or negative in case not found
 */
int lwm2m_server_short_id_to_inst(uint16_t short_id);

#if defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)
bool lwm2m_server_get_mute_send(uint16_t obj_inst_id);
#endif

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
/**
 * @brief Sets the update state (as specified in  LWM2M SPEC E.6 regarding the firmware update)
 * of the Firmware object instance given by @p obj_inst_id to @p state.
 * (i.e Sets the value of resource 5/ @p obj_inst_id /3)
 *
 * @param[in] obj_inst_id Object instance id of the firmware object instance
 * @param[in] state (STATE_IDLE through STATE_UPDATING)
 */
void lwm2m_firmware_set_update_state_inst(uint16_t obj_inst_id, uint8_t state);

/**
 * @brief Sets the result state (as specified in  LWM2M SPEC E.6 regarding the firmware update)
 * of the Firmware object instance given by @p obj_inst_id to @p result.
 * (i.e Sets the value of resource 5/ @p obj_inst_id /5)
 *
 * @param[in] obj_inst_id Object instance id of the firmware object instance
 * @param[in] result (RESULT_DEFAULT through RESULT_UNSUP_PROTO)
 */
void lwm2m_firmware_set_update_result_inst(uint16_t obj_inst_id, uint8_t result);

/**
 * @brief Equivalent to lwm2m_firmware_set_update_state_inst(0, state).
 *
 * @param[in] state (STATE_IDLE through STATE_UPDATING)
 */
void lwm2m_firmware_set_update_state(uint8_t state);

/**
 * @brief Equivalent to lwm2m_firmware_set_update_result_inst(0, result).
 *
 * @param[in] result (RESULT_DEFAULT through RESULT_UNSUP_PROTO)
 */
void lwm2m_firmware_set_update_result(uint8_t result);

/**
 * @brief Returns the update state (as specified in  LWM2M SPEC E.6 regarding the firmware update)
 * of the Firmware object instance given by @p obj_inst_id.
 * (i.e Gets the value of resource 5/ @p obj_inst_id /3)
 * @param[in] obj_inst_id Object instance id of the firmware object
 * @return (STATE_IDLE through STATE_UPDATING)
 */
uint8_t lwm2m_firmware_get_update_state_inst(uint16_t obj_inst_id);

/**
 * @brief Equivalent to lwm2m_firmware_get_update_state_inst(0).
 *
 * @return (STATE_IDLE through STATE_UPDATING)
 */
uint8_t lwm2m_firmware_get_update_state(void);

/**
 * @brief Returns the result state (as specified in  LWM2M SPEC E.6 regarding the firmware update)
 * of the Firmware object instance given by @p obj_inst_id.
 * (i.e Gets the value of resource 5/ @p obj_inst_id /5)
 * @param[in] obj_inst_id Object instance id of the firmware object
 * @return (RESULT_DEFAULT through RESULT_UNSUP_PROTO)
 */
uint8_t lwm2m_firmware_get_update_result_inst(uint16_t obj_inst_id);

/**
 * @brief Equivalent to lwm2m_firmware_get_update_result_inst(0).
 *
 * @return (RESULT_DEFAULT through RESULT_UNSUP_PROTO)
 */
uint8_t lwm2m_firmware_get_update_result(void);
#endif

/* Network Layer */
/**
 * @brief Opens a socket for the client_ctx if it does not exist. Saves the
 * socket file descriptor to client_ctx->sock_fd.
 *
 * @param[in] client_ctx lwm2m context to open a socket for
 * @return 0 for success or negative in case of error
 */
int lwm2m_open_socket(struct lwm2m_ctx *client_ctx);

/**
 * @brief Closes the socket with the file descriptor given by client_ctx->sock_fd.
 * Does nothing if the context's socket is already closed.
 *
 * @param[in, out] client_ctx lwm2m context whose socket is to be closed
 * @return 0 for success or negative in case of error
 */
int lwm2m_close_socket(struct lwm2m_ctx *client_ctx);

/**
 * @brief Removes the socket with the file descriptor given by client_ctx->sock_fd from the
 * lwm2m work loop. Keeps the socket open.
 *
 * @param[in, out] client_ctx lwm2m context whose socket is being suspended
 * @return 0 for success or negative in case of error
 */
int lwm2m_socket_suspend(struct lwm2m_ctx *client_ctx);
/**
 * @brief Adds an existing socket to the lwm2m work loop. (The socket specified by the file
 * descriptor in ctx->sock_fd)
 *
 * @param[in] ctx lwm2m context being added to the lwm2m work loop
 * @return 0 for success or negative in case of error
 */
int lwm2m_socket_add(struct lwm2m_ctx *ctx);

/**
 * @brief Removes a socket from the lwm2m work loop.
 *
 * @param[in] ctx lwm2m context to be removed from the lwm2m work loop
 */
void lwm2m_socket_del(struct lwm2m_ctx *ctx);

/**
 * @brief Creates a socket for the @p client_ctx (if it does not exist) and adds it to
 * the lwm2m work loop.
 *
 * @param client_ctx lwm2m context that the socket is being added for
 * @return 0 for success or negative in case of error
 */
int lwm2m_socket_start(struct lwm2m_ctx *client_ctx);

/**
 * @brief Closes the socket with file descriptor given by client_ctx->sock_fd.
 *
 * @param client_ctx Context whose socket is being closed
 * @return 0 for success or negative in case of error
 */
int lwm2m_socket_close(struct lwm2m_ctx *client_ctx);

/**
 * @brief Closes the socket connection when queue mode is enabled.
 *
 * @param[in, out] client_ctx lwm2m context to be closed
 * @return 0 for success or negative in case of error
 */
int lwm2m_engine_close_socket_connection(struct lwm2m_ctx *client_ctx);

/**
 * @brief Starts the socket of @p client_ctx. Requires CONFIG_LWM2M_DTLS_SUPPORT=y.
 *
 * @param[in] client_ctx lwm2m context to be re-added to the lwm2m work loop
 * @return 0 for success or negative in case of error
 */
int lwm2m_engine_connection_resume(struct lwm2m_ctx *client_ctx);

/**
 * @brief Moves all queued messages to pending.
 *
 * @param[in, out] client_ctx
 * @return 0 for success or negative in case of error
 */
int lwm2m_push_queued_buffers(struct lwm2m_ctx *client_ctx);

/* Resources */
struct lwm2m_ctx **lwm2m_sock_ctx(void);
int lwm2m_sock_nfds(void);
#endif /* LWM2M_ENGINE_H */
