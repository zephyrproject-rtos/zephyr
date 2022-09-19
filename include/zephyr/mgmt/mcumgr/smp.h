/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zephyr_smp_transport;
struct net_buf;

/** @typedef zephyr_smp_transport_out_fn
 * @brief SMP transmit function for Zephyr.
 *
 * The supplied net_buf is always consumed, regardless of return code.
 *
 * @param nb                    The net_buf to transmit.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
typedef int zephyr_smp_transport_out_fn(struct net_buf *nb);

/** @typedef zephyr_smp_transport_get_mtu_fn
 * @brief SMP MTU query function for Zephyr.
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
typedef uint16_t zephyr_smp_transport_get_mtu_fn(const struct net_buf *nb);

/** @typedef zephyr_smp_transport_ud_copy_fn
 * @brief SMP copy buffer user_data function for Zephyr.
 *
 * The supplied src net_buf should contain a user_data that cannot be copied
 * using regular memcpy function (e.g., the BLE transport net_buf user_data
 * stores the connection reference that has to be incremented when is going
 * to be used by another buffer).
 *
 * @param dst                   Source buffer user_data pointer.
 * @param src                   Destination buffer user_data pointer.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
typedef int zephyr_smp_transport_ud_copy_fn(struct net_buf *dst,
					    const struct net_buf *src);

/** @typedef zephyr_smp_transport_ud_free_fn
 * @brief SMP free buffer user_data function for Zephyr.
 *
 * This function frees net_buf user data, because some transports store
 * connection-specific information in the net_buf user data (e.g., the BLE
 * transport stores the connection reference that has to be decreased).
 *
 * @param ud                    Contains a user_data pointer to be freed.
 */
typedef void zephyr_smp_transport_ud_free_fn(void *ud);

/**
 * @brief Provides Zephyr-specific functionality for sending SMP responses.
 */
struct zephyr_smp_transport {
	/* Must be the first member. */
	struct k_work zst_work;

	/* FIFO containing incoming requests to be processed. */
	struct k_fifo zst_fifo;

	zephyr_smp_transport_out_fn *zst_output;
	zephyr_smp_transport_get_mtu_fn *zst_get_mtu;
	zephyr_smp_transport_ud_copy_fn *zst_ud_copy;
	zephyr_smp_transport_ud_free_fn *zst_ud_free;

#ifdef CONFIG_MCUMGR_SMP_REASSEMBLY
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
 * @param zst                   The transport to construct.
 * @param output_func           The transport's send function.
 * @param get_mtu_func          The transport's get-MTU function.
 * @param ud_copy_func          The transport buffer user_data copy function.
 * @param ud_free_func          The transport buffer user_data free function.
 */
void zephyr_smp_transport_init(struct zephyr_smp_transport *zst,
			       zephyr_smp_transport_out_fn *output_func,
			       zephyr_smp_transport_get_mtu_fn *get_mtu_func,
			       zephyr_smp_transport_ud_copy_fn *ud_copy_func,
			       zephyr_smp_transport_ud_free_fn *ud_free_func);

#ifdef __cplusplus
}
#endif

#endif
