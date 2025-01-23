/**
 * @file
 * @brief Bluetooth Call Control Profile (CCP) APIs.
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CCP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CCP_H_

/**
 * @brief Call Control Profile (CCP)
 *
 * @defgroup bt_ccp Call Control Profile (CCP)
 *
 * @since 3.7
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Call Control Profile (CCP) provides procedures to initiate and control calls.
 * It provides the Call Control Client and the Call Control Server roles,
 * where the former is usually placed on resource constrained devices like headphones,
 * and the latter placed on more powerful devices like phones and PCs.
 *
 * The profile is not limited to carrier phone calls and can be used with common applications like
 * Discord and Teams.
 */
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup bt_ccp_call_control_server CCP Call Control Server APIs
 * @ingroup bt_ccp
 * @{
 */
/** @brief Abstract Call Control Server Telephone Bearer structure. */
struct bt_ccp_call_control_server_bearer;

/**
 * @brief Register a Telephone Bearer
 *
 * This will register a Telephone Bearer Service (TBS) (or a Generic Telephone Bearer service
 * (GTBS)) with the provided parameters.
 *
 * As per the TBS specification, the GTBS shall be instantiated for the feature,
 * and as such a GTBS shall always be registered before any TBS can be registered.
 * Similarly, all TBS shall be unregistered before the GTBS can be unregistered with
 * bt_ccp_call_control_server_unregister_bearer().
 *
 * @param[in]  param   The parameters to initialize the bearer.
 * @param[out] bearer  Pointer to the initialized bearer.
 *
 * @retval 0 Success
 * @retval -EINVAL @p param contains invalid data
 * @retval -EALREADY @p param.gtbs is true and GTBS has already been registered
 * @retval -EAGAIN @p param.gtbs is false and GTBS has not been registered
 * @retval -ENOMEM @p param.gtbs is false and no more TBS can be registered (see
 *         @kconfig{CONFIG_BT_TBS_BEARER_COUNT})
 * @retval -ENOEXEC The service failed to be registered
 */
int bt_ccp_call_control_server_register_bearer(const struct bt_tbs_register_param *param,
					       struct bt_ccp_call_control_server_bearer **bearer);

/**
 * @brief Unregister a Telephone Bearer
 *
 * This will unregister a Telephone Bearer Service (TBS) (or a Generic Telephone Bearer service
 * (GTBS)) with the provided parameters. The bearer shall be registered first by
 * bt_ccp_call_control_server_register_bearer() before it can be unregistered.
 *
 * All TBS shall be unregistered before the GTBS can be unregistered with.
 *
 * @param bearer The bearer to unregister.
 *
 * @retval 0 Success
 * @retval -EINVAL @p bearer is NULL
 * @retval -EALREADY The bearer is not registered
 * @retval -ENOEXEC The service failed to be unregistered
 */
int bt_ccp_call_control_server_unregister_bearer(struct bt_ccp_call_control_server_bearer *bearer);

/** @} */ /* End of group bt_ccp_call_control_server */

/**
 * @defgroup bt_ccp_call_control_client CCP Call Control Client APIs
 * @ingroup bt_ccp
 * @{
 */
/** Abstract Call Control Client structure. */
struct bt_ccp_call_control_client;

/** Abstract Call Control Client bearer structure. */
struct bt_ccp_call_control_client_bearer;

/** Struct with information about bearers of a client */
struct bt_ccp_call_control_client_bearers {
#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	/** The GTBS bearer. */
	struct bt_ccp_call_control_client_bearer *gtbs_bearer;
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	/** Number of TBS bearers in @p tbs_bearers */
	size_t tbs_count;

	/** Array of pointers of TBS bearers */
	struct bt_ccp_call_control_client_bearer
		*tbs_bearers[CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT];
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
};

/**
 * @brief Struct to hold the Telephone Bearer Service client callbacks
 *
 * These can be registered for usage with bt_tbs_client_register_cb().
 */
struct bt_ccp_call_control_client_cb {
	/**
	 * @brief Callback function for bt_ccp_call_control_client_discover().
	 *
	 * This callback is called once the discovery procedure is completed.
	 *
	 * @param client       Call Control Client pointer.
	 * @param err          Error value. 0 on success, GATT error on positive
	 *                     value or errno on negative value.
	 * @param bearers      The bearers found.
	 */
	void (*discover)(struct bt_ccp_call_control_client *client, int err,
			 struct bt_ccp_call_control_client_bearers *bearers);

	/** @internal Internally used field for list handling */
	sys_snode_t _node;
};

int bt_ccp_call_control_client_discover(struct bt_conn *conn,
					struct bt_ccp_call_control_client **out_client);

/**
 * @brief Register callbacks for the Call Control Client
 *
 * @param cb The callback struct
 *
 * @retval 0 Succsss
 * @retval -EINVAL @p cb is NULL
 * @retval -EEXISTS @p cb is already registered
 */
int bt_ccp_call_control_client_register_cb(struct bt_ccp_call_control_client_cb *cb);

/**
 * @brief Unregister callbacks for the Call Control Client
 *
 * @param cb The callback struct
 *
 * @retval 0 Succsss
 * @retval -EINVAL @p cb is NULL
 * @retval -EALREADY @p cb is not registered
 */
int bt_ccp_call_control_client_unregister_cb(struct bt_ccp_call_control_client_cb *cb);
/** @} */ /* End of group bt_ccp_call_control_client */
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CCP_H_ */
