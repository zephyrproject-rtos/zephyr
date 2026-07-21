/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_OBJ_SERVER_H_
#define LWM2M_OBJ_SERVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys_clock.h>

/* Server resource IDs */
#define SERVER_SHORT_SERVER_ID				0
#define SERVER_LIFETIME_ID				1
#define SERVER_DEFAULT_MIN_PERIOD_ID			2
#define SERVER_DEFAULT_MAX_PERIOD_ID			3
#define SERVER_DISABLE_ID				4
#define SERVER_DISABLE_TIMEOUT_ID			5
#define SERVER_STORE_NOTIFY_ID				6
#define SERVER_TRANSPORT_BINDING_ID			7
#define SERVER_REG_UPDATE_TRIGGER_ID			8
/* Server object version 1.1 resource IDs */
#define SERVER_BOOTSTRAP_UPDATE_TRIGGER_ID		9
#define SERVER_APN_LINK_ID				10
#define SERVER_TLS_DTLS_ALERT_CODE_ID			11
#define SERVER_LAST_BOOTSTRAPPED_ID			12
#define SERVER_REGISTRATION_PRIORITY_ORDER_ID		13
#define SERVER_INITIAL_REGISTRATION_DELAY_TIMER_ID	14
#define SERVER_REGISTRATION_FAILURE_BLOCK_ID		15
#define SERVER_BOOTSTRAP_ON_REGISTRATION_FAILURE_ID	16
#define SERVER_COMMUNICATION_RETRY_COUNT_ID		17
#define SERVER_COMMUNICATION_RETRY_TIMER_ID		18
#define SERVER_COMMUNICATION_SEQUENCE_DELAY_TIMER_ID	19
#define SERVER_COMMUNICATION_SEQUENCE_RETRY_TIMER_ID	20
#define SERVER_SMS_TRIGGER_ID				21
#define SERVER_PREFERRED_TRANSPORT_ID			22
#define SERVER_MUTE_SEND_ID				23

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

/**
 * @brief Check if given server instance is not disabled
 *
 * @param[in] obj_inst_id server instance
 * @return true if not disabled, false otherwise.
 */
bool lwm2m_server_is_enabled(uint16_t obj_inst_id);

/**
 * @brief Select server instance.
 *
 * Find possible server instance considering values on server data.
 * Server candidates cannot be in disabled state and if priority values are set,
 * those are compared and lowest values are considered first.
 *
 * If @ref obj_inst_id is NULL, this can be used to check if there are any server available.
 *
 * @param[out] obj_inst_id where selected server instance ID is written. Can be NULL.
 * @return true if server instance was found, false otherwise.
 */
bool lwm2m_server_select(uint16_t *obj_inst_id);

/**
 * @brief Disable server instance for a period of time.
 *
 * Timeout values can be calculated using kernel macros like K_SECONDS().
 * Values like K_FOREVER or K_NO_WAIT are also accepted.
 *
 * @param timeout Timeout value.
 * @return zero on success, negative error code otherwise.
 */
int lwm2m_server_disable(uint16_t obj_inst_id, k_timeout_t timeout);

/**
 * @brief Get timepoint how long server instance is disabled.
 *
 * If server instance is not disabled, this still returns a valid timepoint
 * that have already expired.
 * If the instance id is not valid, the timepoint is set to K_FOREVER.
 *
 * @param obj_inst_id Server instance ID.
 * @return timepoint
 */
k_timepoint_t lwm2m_server_get_disabled_time(uint16_t obj_inst_id);

/**
 * @brief Get priority of given server instance.
 *
 * Lower values mean higher priority.
 * If LwM2M server object version 1.1 is not enabled,
 * this returns obj_inst_id as priority.
 *
 * @param obj_inst_id instance ID
 * @return priority or UINT8_MAX if instance not found
 */
uint8_t lwm2m_server_get_prio(uint16_t obj_inst_id);

/**
 * @brief Reset all disable-timers for all server instances.
 *
 */
void lwm2m_server_reset_timestamps(void);

#if defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)
bool lwm2m_server_get_mute_send(uint16_t obj_inst_id);
#endif


#endif /* LWM2M_OBJ_SERVER_H_ */
