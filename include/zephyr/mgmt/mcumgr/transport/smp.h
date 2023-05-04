/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr transport SMP API
 * @defgroup mcumgr_transport_smp MCUmgr transport SMP API
 * @ingroup mcumgr
 * @{
 */

struct smp_transport;
struct zephyr_smp_transport;
struct net_buf;

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
/* use smp_transport_out_fn instead */
typedef int zephyr_smp_transport_out_fn(struct net_buf *nb);

/** @typedef smp_transport_get_mtu_fn
 * @brief SMP MTU query callback for transport
 *
 * The supplied net_buf should contain a request received from the peer whose
 * MTU is being queried.  This function takes a net_buf parameter because some
 * transports store connection-specific information in the net_buf user header
 * (e.g., the BLE transport stores the peer address).
 *
 * @param nb                    Contains a request from the relevant peer.
 *
 * @return                      The transport's MTU;
 *                              0 if transmission is currently not possible.
 */
typedef uint16_t (*smp_transport_get_mtu_fn)(const struct net_buf *nb);
/* use smp_transport_get_mtu_fn instead */
typedef uint16_t zephyr_smp_transport_get_mtu_fn(const struct net_buf *nb);

/** @typedef smp_transport_ud_copy_fn
 * @brief SMP copy user_data callback
 *
 * The supplied src net_buf should contain a user_data that cannot be copied
 * using regular memcpy function (e.g., the BLE transport net_buf user_data
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
/* use smp_transport_ud_copy_fn instead */
typedef int zephyr_smp_transport_ud_copy_fn(struct net_buf *dst,
					    const struct net_buf *src);

/** @typedef smp_transport_ud_free_fn
 * @brief SMP free user_data callback
 *
 * This function frees net_buf user data, because some transports store
 * connection-specific information in the net_buf user data (e.g., the BLE
 * transport stores the connection reference that has to be decreased).
 *
 * @param ud                    Contains a user_data pointer to be freed.
 */
typedef void (*smp_transport_ud_free_fn)(void *ud);
/* use smp_transport_ud_free_fn instead */
typedef void zephyr_smp_transport_ud_free_fn(void *ud);

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

/**
 * @brief SMP transport object for sending SMP responses.
 */
struct smp_transport {
	/* Must be the first member. */
	struct k_work work;

	/* FIFO containing incoming requests to be processed. */
	struct k_fifo fifo;

	smp_transport_out_fn output;
	smp_transport_get_mtu_fn get_mtu;
	smp_transport_ud_copy_fn ud_copy;
	smp_transport_ud_free_fn ud_free;
	smp_transport_query_valid_check_fn query_valid_check;

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
	/* Packet reassembly internal data, API access only */
	struct {
		struct net_buf *current;	/* net_buf used for reassembly */
		uint16_t expected;		/* expected bytes to come */
	} __reassembly;
#endif
};

/* Deprecated, use smp_transport instead */
struct zephyr_smp_transport {
	/* Must be the first member. */
	struct k_work zst_work;

	/* FIFO containing incoming requests to be processed. */
	struct k_fifo zst_fifo;

	smp_transport_out_fn zst_output;
	smp_transport_get_mtu_fn zst_get_mtu;
	smp_transport_ud_copy_fn zst_ud_copy;
	smp_transport_ud_free_fn zst_ud_free;

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
	/* Packet reassembly internal data, API access only */
	struct {
		struct net_buf *current;	/* net_buf used for reassembly */
		uint16_t expected;		/* expected bytes to come */
	} __reassembly;
#endif
};

/**
 * @brief Initializes a Zephyr SMP transport object.
 *
 * @param smpt				The transport to construct.
 * @param output_func			The transport's send function.
 * @param get_mtu_func			The transport's get-MTU function.
 * @param ud_copy_func			The transport buffer user_data copy function.
 * @param ud_free_func			The transport buffer user_data free function.
 * @param query_valid_check_func	The transport's check function for if a query is valid.
 */
void smp_transport_init(struct smp_transport *smpt,
			smp_transport_out_fn output_func,
			smp_transport_get_mtu_fn get_mtu_func,
			smp_transport_ud_copy_fn ud_copy_func,
			smp_transport_ud_free_fn ud_free_func,
			smp_transport_query_valid_check_fn query_valid_check_func);

__deprecated static inline
void zephyr_smp_transport_init(struct zephyr_smp_transport *smpt,
			       zephyr_smp_transport_out_fn *output_func,
			       zephyr_smp_transport_get_mtu_fn *get_mtu_func,
			       zephyr_smp_transport_ud_copy_fn *ud_copy_func,
			       zephyr_smp_transport_ud_free_fn *ud_free_func)
{
	smp_transport_init((struct smp_transport *)smpt,
			   (smp_transport_out_fn)output_func,
			   (smp_transport_get_mtu_fn)get_mtu_func,
			   (smp_transport_ud_copy_fn)ud_copy_func,
			   (smp_transport_ud_free_fn)ud_free_func, NULL);
}

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
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
