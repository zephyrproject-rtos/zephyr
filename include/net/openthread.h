/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief OpenThread l2 stack public header
 */

#ifndef ZEPHYR_INCLUDE_NET_OPENTHREAD_H_
#define ZEPHYR_INCLUDE_NET_OPENTHREAD_H_

/**
 * @brief OpenThread l2 stack api
 * @defgroup OpenThread l2 layer
 * @ingroup networking
 * @{
 */

#include <kernel.h>

#include <net/net_if.h>

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief Type of pkt_list
 */
struct pkt_list_elem {
	struct net_pkt *pkt;
};

/**
 * @brief OpenThread l2 private data.
 */
struct openthread_context {
	/** Pointer to OpenThread stack instance */
	otInstance *instance;

	/** Pointer to OpenThread network interface */
	struct net_if *iface;

	/** Index indicates the head of pkt_list ring buffer */
	uint16_t pkt_list_in_idx;

	/** Index indicates the tail of pkt_list ring buffer */
	uint16_t pkt_list_out_idx;

	/** Flag indicates that pkt_list is full */
	uint8_t pkt_list_full;

	/** Array for storing net_pkt for OpenThread internal usage */
	struct pkt_list_elem pkt_list[CONFIG_OPENTHREAD_PKT_LIST_SIZE];

	/** A mutex to protect API calls from being preempted. */
	struct k_mutex api_lock;
};
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Sets function which will be called when certain configuration or state
 * changes within OpenThread.
 *
 * @param cb function to call in callback procedure.
 */
void openthread_set_state_changed_cb(otStateChangedCallback cb);

/**
 * @brief Get OpenThread thread identification.
 */
k_tid_t openthread_thread_id_get(void);

/**
 * @brief Get pointer to default OpenThread context.
 *
 * @retval !NULL On success.
 * @retval NULL  On failure.
 */
struct openthread_context *openthread_get_default_context(void);

/**
 * @brief Get pointer to default OpenThread instance.
 *
 * @retval !NULL On success.
 * @retval NULL  On failure.
 */
struct otInstance *openthread_get_default_instance(void);

/**
 * @brief Starts the OpenThread network.
 *
 * @details Depends on active settings: it uses stored network configuration,
 * start joining procedure or uses default network configuration. Additionally
 * when the device is MTD, it sets the SED mode to properly attach the network.
 *
 * @param ot_context
 */
int openthread_start(struct openthread_context *ot_context);

/**
 * @brief Lock internal mutex before accessing OT API.
 *
 * @details OpenThread API is not thread-safe, therefore before accessing any
 * API function, it's needed to lock the internal mutex, to prevent the
 * OpenThread thread from prempting the API call.
 *
 * @param ot_context Context to lock.
 */
void openthread_api_mutex_lock(struct openthread_context *ot_context);

/**
 * @brief Try to lock internal mutex before accessing OT API.
 *
 * @details This function behaves like openthread_api_mutex_lock() provided that
 * the internal mutex is unlocked. Otherwise, it exists immediately and returns
 * a negative value.
 *
 * @param ot_context Context to lock.
 * @retval 0  On success.
 * @retval <0 On failure.
 */
int openthread_api_mutex_try_lock(struct openthread_context *ot_context);

/**
 * @brief Unlock internal mutex after accessing OT API.
 *
 * @param ot_context Context to unlock.
 */
void openthread_api_mutex_unlock(struct openthread_context *ot_context);

#define OPENTHREAD_L2_CTX_TYPE struct openthread_context

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_OPENTHREAD_H_ */
