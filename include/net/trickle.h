/** @file
 * @brief Trickle timer library
 *
 * This implements Trickle timer as specified in RFC 6206
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_TRICKLE_H_
#define ZEPHYR_INCLUDE_NET_TRICKLE_H_

#include <stdbool.h>
#include <zephyr/types.h>

#include <kernel.h>
#include <net/net_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Trickle algorithm library
 * @defgroup trickle Trickle Algorithm Library
 * @ingroup networking
 * @{
 */

struct net_trickle;

/**
 * @typedef net_trickle_cb_t
 * @brief Trickle timer callback.
 *
 * @details The callback is called after Trickle timeout expires.
 *
 * @param trickle The trickle context to use.
 * @param do_suppress Is TX allowed (true) or not (false).
 * @param user_data The user data given in net_trickle_start() call.
 */
typedef void (*net_trickle_cb_t)(struct net_trickle *trickle,
				 bool do_suppress, void *user_data);

/**
 * The variable names are taken directly from RFC 6206 when applicable.
 * Note that the struct members should not be accessed directly but
 * only via the Trickle API.
 */
struct net_trickle {
	uint32_t Imin;		/**< Min interval size in ms */
	uint8_t Imax;		/**< Max number of doublings */
	uint8_t k;		/**< Redundancy constant */

	uint32_t I;		/**< Current interval size */
	uint32_t Istart;	/**< Start of the interval in ms */
	uint8_t c;		/**< Consistency counter */

	uint32_t Imax_abs;	/**< Max interval size in ms (not doublings) */

	struct k_delayed_work timer;
	net_trickle_cb_t cb;	/**< Callback to be called when timer expires */
	void *user_data;
};

/** @cond INTERNAL_HIDDEN */
#define NET_TRICKLE_INFINITE_REDUNDANCY 0
/** @endcond */

/**
 * @brief Create a Trickle timer.
 *
 * @param trickle Pointer to Trickle struct.
 * @param Imin Imin configuration parameter in ms.
 * @param Imax Max number of doublings.
 * @param k Redundancy constant parameter. See RFC 6206 for details.
 *
 * @return Return 0 if ok and <0 if error.
 */
int net_trickle_create(struct net_trickle *trickle,
		       uint32_t Imin,
		       uint8_t Imax,
		       uint8_t k);

/**
 * @brief Start a Trickle timer.
 *
 * @param trickle Pointer to Trickle struct.
 * @param cb User callback to call at time T within the current trickle
 * interval
 * @param user_data User pointer that is passed to callback.
 *
 * @return Return 0 if ok and <0 if error.
 */
int net_trickle_start(struct net_trickle *trickle,
		      net_trickle_cb_t cb,
		      void *user_data);

/**
 * @brief Stop a Trickle timer.
 *
 * @param trickle Pointer to Trickle struct.
 *
 * @return Return 0 if ok and <0 if error.
 */
int net_trickle_stop(struct net_trickle *trickle);

/**
 * @brief To be called by the protocol handler when it hears a consistent
 * network transmission.
 *
 * @param trickle Pointer to Trickle struct.
 */
void net_trickle_consistency(struct net_trickle *trickle);

/**
 * @brief To be called by the protocol handler when it hears an inconsistent
 * network transmission.
 *
 * @param trickle Pointer to Trickle struct.
 */
void net_trickle_inconsistency(struct net_trickle *trickle);

/**
 * @brief Check if the Trickle timer is running or not.
 *
 * @param trickle Pointer to Trickle struct.
 *
 * @return Return True if timer is running and False if not.
 */
static inline bool net_trickle_is_running(struct net_trickle *trickle)
{
	NET_ASSERT(trickle);

	return trickle->I != 0U;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_TRICKLE_H_ */
