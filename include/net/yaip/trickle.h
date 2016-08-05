/** @file
 * @brief Trickle timer library
 *
 * This implements Trickle timer as specified in RFC 6206
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TRICKLE_H
#define __TRICKLE_H

#include <stdbool.h>
#include <stdint.h>

#include <misc/nano_work.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_trickle;

typedef void (*net_trickle_cb_t)(struct net_trickle *trickle,
				 bool do_suppress, void *user_data);

/*
 * The variable names are taken directly from RFC when applicable.
 * Note that the struct members should not be accessed directly but
 * only via the Trickle API.
 */
struct net_trickle {
	uint32_t Imin;		/* Min interval size in ticks */
	uint8_t Imax;		/* Max number of doublings */
	uint8_t k;		/* Redundancy constant */

	uint32_t I;		/* Current interval size */
	uint32_t Istart;	/* Start of the interval in ticks */
	uint8_t c;		/* Consistency counter */

	uint32_t Imax_abs;	/* Max interval size in ticks (not doublings)
				 */

	struct nano_delayed_work timer;
	net_trickle_cb_t cb;	/* Callback to be called when timer expires */
	void *user_data;
};

#define NET_TRICKLE_INFINITE_REDUNDANCY 0

/**
 * @brief Create a Trickle timer.
 *
 * @param trickle Pointer to Trickle struct.
 * @param Imin Imin configuration parameter in ticks.
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

	return trickle->I != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __TRICKLE_H */
