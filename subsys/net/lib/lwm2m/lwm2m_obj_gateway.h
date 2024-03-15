/**
 * @file lwm2m_obj_gateway.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LWM2M_OBJ_GATEWAY__
#define __LWM2M_OBJ_GATEWAY__

#include <lwm2m_object.h>

/* LwM2M Gateway resource IDs */
/* clang-format off */
#define LWM2M_GATEWAY_DEVICE_RID             0
#define LWM2M_GATEWAY_PREFIX_RID             1
#define LWM2M_GATEWAY_DEPRECATED_RID         2
#define LWM2M_GATEWAY_IOT_DEVICE_OBJECTS_RID 3
/* clang-format on */

/**
 * @brief A callback which handles the prefixed messages from the server.
 *
 * The callback gets triggered each time the prefix in front of a received lwm2m
 * msg path matches the prefix set in the LWM2M_GATEWAY_PREFIX_RID buffer.
 *
 * It must handle the content of the coap message completely.
 * In case of success the LwM2M engine will then send the formatted coap message,
 * otherwise a coap response code is sent.
 *
 * Example of returning CoAP response:
 * @code{.c}
 * lwm2m_init_message(msg);
 * // Write CoAP packet to msg->out.out_cpkt
 * return 0;
 * @endcode
 *
 *
 * @return 0 if msg contains a  valid CoAP response.
 * @return  negative error code otherwise.
 */
typedef int (*lwm2m_engine_gateway_msg_cb)(struct lwm2m_message *msg);
/**
 * @brief Register a callback which handles the prefixed messages from the server.
 *
 * @return 0 on success
 * @return -ENOENT if no object instance with obj_inst_id was found
 */
int lwm2m_register_gw_callback(uint16_t obj_inst_id, lwm2m_engine_gateway_msg_cb cb);
/**
 * @brief Check if given message is handled by Gateway callback.
 *
 * @return 0 if msg was handled by Gateawy and contains a valid response. Negative error code
 * otherwise.
 * @return -ENOENT if this msg was not handled by Gateway object.
 */
int lwm2m_gw_handle_req(struct lwm2m_message *msg);

#endif /* __LWM2M_OBJ_GATEWAY__ */
