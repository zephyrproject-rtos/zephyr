/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_H_
#define ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_H_

#include <zephyr/kernel.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr SMP transport API
 * @defgroup mcumgr_transport_smp SMP transport
 * @ingroup mcumgr_transport
 * @{
 */

struct smp_transport;
struct zephyr_smp_transport;
struct net_buf;
struct smp_transport_bridge;
struct smp_client_transport_entry;

/** @typedef smp_transport_out_fn
 * @brief SMP transmit callback for transport
 *
 * The supplied net_buf is always consumed, regardless of return code.
 *
 * @param nb                    The net_buf to transmit.
 *
 * @return                      0 on success, #mcumgr_err_t code on failure.
 */
typedef int (*smp_transport_out_fn)(struct net_buf *nb);

/** @typedef smp_transport_get_mtu_fn
 * @brief SMP MTU query callback for transport
 *
 * The supplied net_buf should contain a request received from the peer whose
 * MTU is being queried.  This function takes a net_buf parameter because some
 * transports store connection-specific information in the net_buf user header
 * (e.g., the Bluetooth transport stores the peer address).
 *
 * @param nb                    Contains a request from the relevant peer.
 *
 * @return                      The transport's MTU;
 *                              0 if transmission is currently not possible.
 */
typedef uint16_t (*smp_transport_get_mtu_fn)(const struct net_buf *nb);

/** @typedef smp_transport_ud_copy_fn
 * @brief SMP copy user_data callback
 *
 * The supplied src net_buf should contain a user_data that cannot be copied
 * using regular memcpy function (e.g., the Bluetooth transport net_buf user_data
 * stores the connection reference that has to be incremented when is going
 * to be used by another buffer).
 *
 * @param dst                   Source buffer user_data pointer.
 * @param src                   Destination buffer user_data pointer.
 *
 * @return                      0 on success, #mcumgr_err_t code on failure.
 */
typedef int (*smp_transport_ud_copy_fn)(struct net_buf *dst,
					const struct net_buf *src);

/** @typedef smp_transport_ud_free_fn
 * @brief SMP free user_data callback
 *
 * This function frees net_buf user data, because some transports store
 * connection-specific information in the net_buf user data (e.g., the Bluetooth
 * transport stores the connection reference that has to be decreased).
 *
 * @param ud                    Contains a user_data pointer to be freed.
 */
typedef void (*smp_transport_ud_free_fn)(void *ud);

/** @typedef smp_transport_query_valid_check_fn
 * @brief Function for checking if queued data is still valid.
 *
 * This function is used to check if queued SMP data is still valid e.g. on a remote device
 * disconnecting, this is triggered when smp_rx_remove_invalid() is called.
 *
 * @param nb			net buf containing queued request.
 * @param arg			Argument provided when calling smp_rx_remove_invalid() function.
 *
 * @return			false if data is no longer valid/should be freed, true otherwise.
 */
typedef bool (*smp_transport_query_valid_check_fn)(struct net_buf *nb, void *arg);

/** @typedef smp_transport_ud_req_init_fn
 * @brief SMP init request buffer
 *
 * The supplied net_buf should be for a SMP request
 *
 * @param nb                    net buf for SMP request
 * @param priv			SMP transport private data
 */
typedef void (*smp_transport_ud_req_init_fn)(struct net_buf *nb, void *priv);

/** @typedef smp_transport_bridge_connect_fn
 * @brief SMP transport bridge connect
 *
 * Used when establishing a bridge to another transport.
 *
 * @param bridge		contains the bridging context
 * @param outgoing		true if an outgoing connection was requested, false if incoming
 * @param mode			mode of the transport
 * @param same_transport	true if the incoming and outgoing transports both are using this
 *				transport
 * @param input_data		CBOR data from input
 * @param output_data		CBOR data for output (to return an error)
 *
 * @return                      true on success, false on error.
 */
typedef bool (*smp_transport_bridge_connect_fn)(struct smp_transport_bridge *bridge, bool outgoing,
						uint32_t mode, bool same_transport,
						zcbor_state_t *input_data,
						zcbor_state_t *output_data);

/** @typedef smp_transport_bridge_disconnect_fn
 * @brief SMP transport bridge disconnect
 *
 * Used when disconnecting an already established bridge with another transport.
 *
 * @param bridge		contains the bridging context
 * @param outgoing		true if to disconnect the outgoing connection, false to disconnect
 *				the incoming connection
 *
 * @return                      true on success, false on error.
 */
typedef void (*smp_transport_bridge_disconnect_fn)(struct smp_transport_bridge *bridge,
						   bool outgoing);

/** @typedef smp_transport_bridge_out_fn
 * @brief SMP transport bridge output data
 *
 * Pass data for output through transport bridge.
 *
 * @param bridge		contains the bridging context
 * @param nb			data that should be output
 * @param outgoing		true if to use the outgoing connection context, false to use the
 *				incoming connection context
 *
 * @return                      0 on success, #mcumgr_err_t code on failure.
 */
typedef int (*smp_transport_bridge_out_fn)(const struct smp_transport_bridge *bridge,
					   struct net_buf *nb, bool outgoing);

/** @typedef mgmt_client_transport_cb_t
 * @brief Callback for SMP client transports
 *
 * @param transport             SMP client transport
 * @param user_data             user data supplied to smp_client_transport_foreach() function
 *
 * @return                      true to continue with the next transport, false to abort.
 */
typedef bool (*mgmt_client_transport_cb_t)(const struct smp_client_transport_entry *transport,
					   void *user_data);

/** @typedef smp_transport_bridge_modes_fn
 * @brief SMP transport bridge details
 *
 * Used to see number of transport modes.
 *
 * @param output_data		CBOR data for output
 * @param rc			#mcumgr_err_t code to return, if error
 *
 * @return                      number of supported transport modes on success, #mcumgr_err_t code
 *				on failure.
 */
typedef bool (*smp_transport_bridge_modes_fn)(zcbor_state_t *output_data, int *rc);

/** @typedef smp_transport_bridge_config_details_fn
 * @brief SMP transport bridge config details
 *
 * Used to see what configuration options a transport has to establish a connection.
 *
 * @param mode			mode of the transport
 * @param output_data		CBOR data for output
 * @param rc			#mcumgr_err_t code to return, if error
 *
 * @return                      true on success, false on failure.
 */
typedef bool (*smp_transport_bridge_config_details_fn)(uint32_t mode, zcbor_state_t *output_data,
						       int *rc);

/**
 * @brief Function pointers of SMP transport functions, if a handler is NULL then it is not
 * supported/implemented.
 */
struct smp_transport_api_t {
	/** Transport's send function. */
	smp_transport_out_fn output;

	/** Transport's get-MTU function. */
	smp_transport_get_mtu_fn get_mtu;

	/** Transport buffer user_data copy function. */
	smp_transport_ud_copy_fn ud_copy;

	/** Transport buffer user_data free function. */
	smp_transport_ud_free_fn ud_free;

	/** Transport's check function for if a query is valid. */
	smp_transport_query_valid_check_fn query_valid_check;

	/** Transport's request buffer init function */
	smp_transport_ud_req_init_fn ud_init;

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) || defined(__DOXYGEN__)
	/** Transport connect bridge to another transport function. */
	smp_transport_bridge_connect_fn bridge_connect;

	/** Transport disconnect bridge function. */
	smp_transport_bridge_disconnect_fn bridge_disconnect;

	/** Transport send data over bridged connection function. */
	smp_transport_bridge_out_fn bridge_output;

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS) || defined(__DOXYGEN__)
	/** Transport get modes function. */
	smp_transport_bridge_modes_fn bridge_modes;

	/** Transport get config details function. */
	smp_transport_bridge_config_details_fn bridge_config_details;
#endif
#endif
};

/**
 * @brief SMP transport object for sending SMP responses.
 */
struct smp_transport {
	/* Must be the first member. */
	struct k_work work;

	/* FIFO containing incoming requests to be processed. */
	struct k_fifo fifo;

	/* Function pointers */
	struct smp_transport_api_t functions;

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
	/* Packet reassembly internal data, API access only */
	struct {
		struct net_buf *current;	/* net_buf used for reassembly */
		uint16_t expected;		/* expected bytes to come */
	} __reassembly;
#endif
};

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
struct smp_transport_bridge {
	uint8_t status;
	struct smp_transport *incoming_transport;
	struct smp_transport *outgoing_transport;
};
#endif

/**
 * @brief SMP transport type for client registration
 */
enum smp_transport_type {
	/** SMP serial */
	SMP_SERIAL_TRANSPORT = 0,
	/** SMP raw serial (not SMP over console) */
	SMP_RAW_SERIAL_TRANSPORT,
	/** SMP bluetooth */
	SMP_BLUETOOTH_TRANSPORT,
	/** SMP shell*/
	SMP_SHELL_TRANSPORT,
	/** SMP UDP IPv4 */
	SMP_UDP_IPV4_TRANSPORT,
	/** SMP UDP IPv6 */
	SMP_UDP_IPV6_TRANSPORT,
	/** SMP LoRaWAN */
	SMP_LORAWAN_TRANSPORT,

	/** IDs up to 63 reserved for future in-tree transports */

	/** SMP user defined type */
	SMP_USER_DEFINED_TRANSPORT = 64,
};

/**
 * @brief SMP Client transport structure
 */
struct smp_client_transport_entry {
	sys_snode_t node;
	/** Transport structure pointer */
	struct smp_transport *smpt;
	/** Transport type */
	int smpt_type;

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS) || defined(__DOXYGEN__)
	/** Transport name, used for transport mgmt (or NULL to omit) */
	char *name;
#endif
};

/**
 * @brief Initializes a Zephyr SMP transport object.
 *
 * @param smpt	The transport to construct.
 *
 * @return	0 If successful
 * @return	Negative errno code if failure.
 */
int smp_transport_init(struct smp_transport *smpt);

/**
 * @brief	Used to remove queued requests for an SMP transport that are no longer valid. A
 *		smp_transport_query_valid_check_fn() function must be registered for this to
 *		function. If the smp_transport_query_valid_check_fn() function returns false
 *		during a callback, the queried command will classed as invalid and dropped.
 *
 * @param zst	The transport to use.
 * @param arg	Argument provided to callback smp_transport_query_valid_check_fn() function.
 */
void smp_rx_remove_invalid(struct smp_transport *zst, void *arg);

/**
 * @brief	Used to clear pending queued requests for an SMP transport.
 *
 * @param zst	The transport to use.
 */
void smp_rx_clear(struct smp_transport *zst);

/**
 * @brief Register a Zephyr SMP transport object for client.
 *
 * @param entry	The transport to construct.
 */
void smp_client_transport_register(struct smp_client_transport_entry *entry);

/**
 * @brief Discover a registered SMP transport client object.
 *
 * @param smpt_type	Type of transport
 *
 * @return		Pointer to registered object. Unknown type return NULL.
 */
struct smp_transport *smp_client_transport_get(int smpt_type);

/**
 * @brief Iterate over SMP client/transports.
 *
 * @param user_cb	Callback function
 * @param user_data	User data supplied to callback function
 *
 * @return		true if all transports were iterated, false otherwise.
 */
bool smp_client_transport_foreach(mgmt_client_transport_cb_t user_cb, void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_H_ */
