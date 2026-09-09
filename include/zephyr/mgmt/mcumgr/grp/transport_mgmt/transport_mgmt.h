/*
 * Copyright (c) 2025-2026 Nordic Semiconductor ASA
 * Copyright (c) 2026, Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the MCUmgr transport management group API.
 * @ingroup mcumgr_transport_mgmt
 */

#ifndef ZEPHYR_INCLUDE_MGMT_MCUMGR_GRP_TRANSPORT_MGMT_TRANSPORT_MGMT_H_
#define ZEPHYR_INCLUDE_MGMT_MCUMGR_GRP_TRANSPORT_MGMT_TRANSPORT_MGMT_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>

/**
 * @brief MCUmgr Transport management API
 * @defgroup mcumgr_transport_mgmt Transport management
 * @ingroup mcumgr_mgmt_api
 * @since 4.5
 * @version 0.1.0
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Command IDs for transport management group. */
enum transport_mgmt_ids {
	/** Used to connect (bridge) to another transport */
	TRANSPORT_MGMT_ID_CONNECT,

	/** Used to disconnect current bridge or all bridges */
	TRANSPORT_MGMT_ID_DISCONNECT,

	/** Returns current bridge status */
	TRANSPORT_MGMT_ID_STATUS,

	/** Reserved for future use */
	TRANSPORT_MGMT_ID_RESERVED_1,

	/** Reserved for future use */
	TRANSPORT_MGMT_ID_RESERVED_2,

	/** Reserved for future use */
	TRANSPORT_MGMT_ID_RESERVED_3,

	/*
	 * The following are purposely at the end to be able to see if they have been excluded
	 * when using enum mgmt due to being optional when using a custom derivative group ID
	 */

	/** List supported transports */
	TRANSPORT_MGMT_ID_LIST,

	/** Get the supported modes of a transport */
	TRANSPORT_MGMT_ID_GET_MODES,

	/** Get the configuration details of a transport */
	TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS,
};

/**
 * Incoming transport whereby the device will forward read/write request packets on the specified
 * transport and send them out on the transport it is bridged with
 */
#define TRANSPORT_MGMT_DIRECTION_INCOMING false

/**
 * Outgoing transport whereby the device will forward read/write response packets on the specified
 * transport and send them out on the transport it is bridged with
 */
#define TRANSPORT_MGMT_DIRECTION_OUTGOING true

/**
 * @brief Checks if a given transport is bridged or not.
 *
 * @param transport	transport to check status of
 * @param direction	specifies the direction of the bridge to check for, either
 *			#TRANSPORT_MGMT_DIRECTION_INCOMING or #TRANSPORT_MGMT_DIRECTION_OUTGOING
 *
 * @return true if given transport in given mode is bridged, false otherwise.
 */
bool transport_mgmt_is_bridged(struct smp_transport *transport, bool direction);

/**
 * @brief Gets the other transport which is part of a bridge.
 *
 * @param transport	transport which is bridged
 * @param direction	specifies the direction of the bridge to check to get the opposing
 *			transport for, either #TRANSPORT_MGMT_DIRECTION_INCOMING or
 *			#TRANSPORT_MGMT_DIRECTION_OUTGOING
 *
 * @return the other transport context if a valid bridge was provided, otherwise NULL.
 */
struct smp_transport *transport_mgmt_get_other_transport(struct smp_transport *transport,
							 bool direction);

/**
 * @brief Gets a transport bridge context.
 *
 * @param transport	transport which is bridged
 * @param direction	specifies the direction of the transport for the bridge to get either
 *			#TRANSPORT_MGMT_DIRECTION_INCOMING or #TRANSPORT_MGMT_DIRECTION_OUTGOING
 *
 * @return the transport bridge context if a valid bridged transport was provided, otherwise NULL.
 */
const struct smp_transport_bridge *transport_mgmt_get_bridge(struct smp_transport *transport,
							     bool direction);

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_LOCKING) || defined(__DOXYGEN__)
/** Transport management semaphore lock take function, for use inside of smp.c only */
void transport_mgmt_lock(void);

/** Transport management semaphore lock release function, for use inside of smp.c only */
void transport_mgmt_unlock(void);
#else
#define transport_mgmt_lock() (void)0
#define transport_mgmt_unlock() (void)0
#endif

/** Internal version of transport_mgmt_is_bridged function, for use inside of smp.c only */
bool transport_mgmt_is_bridged_internal(struct smp_transport *transport, bool direction);

/** Internal version of *transport_mgmt_get_bridge function, for use inside of smp.c only */
const struct smp_transport_bridge *transport_mgmt_get_bridge_internal(
							struct smp_transport *transport,
							bool direction);

/** @endcond  */

/**
 * @brief Disconnects an active bridge.
 *
 * @param bridge	bridge context
 *
 * @return return code
 */
int transport_mgmt_disconnect_bridge(struct smp_transport_bridge *bridge);

/**
 * @brief Disconnects an active bridge.
 *
 * @param transport	transport which is bridged
 * @param incoming	if true, will disconnect the bridge context that this is bridged to in
 *			incoming mode
 * @param outgoing	if true, will disconnect the bridge context that this is bridged to in
 *			outgoing mode
 *
 * @return return code
 */
int transport_mgmt_disconnect_transport(struct smp_transport *transport, bool incoming,
					bool outgoing);

/**
 * @brief Disconnects all active bridges.
 *
 * @return return code
 */
int transport_mgmt_disconnect_all(void);

/**
 * Command result codes for transport management group.
 */
enum transport_mgmt_ret_code_t {
	/** No error, this is implied if there is no ret value in the response. */
	TRANSPORT_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	TRANSPORT_MGMT_ERR_UNKNOWN,

	/** The transport is missing the required mandatory bridging functions. */
	TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_REQUIRED_FUNCTIONS,

	/** The transport is missing the information bridging functions. */
	TRANSPORT_MGMT_ERR_TRANSPORT_MISSING_INFO_FUNCTIONS,

	/** Invalid, unsupported or no transport ID provided. */
	TRANSPORT_MGMT_ERR_INVALID_TRANSPORT,

	/** Invalid, unsupported or no mode provided. */
	TRANSPORT_MGMT_ERR_INVALID_MODE,

	/** All transport bridging context are in use. */
	TRANSPORT_MGMT_ERR_ALL_CONTEXTS_USED,

	/** The transport or all parameters were both provided and only one should be supplied. */
	TRANSPORT_MGMT_ERR_BOTH_TRANSPORT_AND_ALL_PARAMETERS,

	/** The transport is not bridged. */
	TRANSPORT_MGMT_ERR_NOT_BRIDGED,

	/**
	 * The transport does not support being used as both the input and output bridge
	 * device.
	 */
	TRANSPORT_MGMT_ERR_SAME_BRIDGE_DEVICE_DISALLOWED,

	/** The transport does not support being used as the ingoing part of a bridge. */
	TRANSPORT_MGMT_ERR_TRANSPORT_INGOING_NOT_SUPPORTED,

	/** The transport does not support being used as the outgoing part of a bridge. */
	TRANSPORT_MGMT_ERR_TRANSPORT_OUTGOING_NOT_SUPPORTED,

	/** The incoming transport is already bridged to another transport. */
	TRANSPORT_MGMT_ERR_TRANSPORT_INCOMING_TRANSPORT_ALREADY_BRIDGED,

	/** The outgoing transport is already bridged to another transport. */
	TRANSPORT_MGMT_ERR_TRANSPORT_OUTGOING_TRANSPORT_ALREADY_BRIDGED,
};

/** Config types */
enum transport_mgmt_config_type_t {
	/** Unsigned integer */
	TRANSPORT_MGMT_CONFIG_TYPE_UINT = 0,

	/** Signed integer */
	TRANSPORT_MGMT_CONFIG_TYPE_INT,

	/** Boolean */
	TRANSPORT_MGMT_CONFIG_TYPE_BOOL,

	/** Text string */
	TRANSPORT_MGMT_CONFIG_TYPE_STRING,

	/** Byte string */
	TRANSPORT_MGMT_CONFIG_TYPE_BYTE_STRING,
};

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_CUSTOM_FUNCTION) || defined(__DOXYGEN__)
/**
 * @brief Gets the group ID for transport management group.
 *
 * Users must implement this function in their own code if they wish to use a dynamic group ID
 * set at run-time when ``CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_CUSTOM_FUNCTION`` is enabled.
 *
 * @return The transport mgmt group ID which must be within the range 64 - 65535.
 */
const uint16_t transport_mgmt_group_id(void);
#elif defined(CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_CUSTOM_VALUE)
/** @cond INTERNAL_HIDDEN */
/** Alternative group ID using Kconfig */
#define transport_mgmt_group_id() CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_CUSTOM_VALUE_GROUP_ID
/** @endcond  */
#else
/** @cond INTERNAL_HIDDEN */
/** Default transport group ID */
#define transport_mgmt_group_id() MGMT_GROUP_ID_TRANSPORT
/** @endcond  */
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
