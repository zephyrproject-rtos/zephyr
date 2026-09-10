/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief OpenThread stack public header
 */

#ifndef ZEPHYR_INCLUDE_NET_OPENTHREAD_H_
#define ZEPHYR_INCLUDE_NET_OPENTHREAD_H_

/**
 * @brief OpenThread stack public header
 * @defgroup openthread OpenThread stack
 * @since 1.11
 * @version 0.8.0
 * @ingroup ieee802154
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/kernel/thread.h>

#include <openthread.h>

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
};
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Get pointer to default OpenThread context.
 *
 * @retval !NULL On success.
 * @retval NULL  On failure.
 */
struct openthread_context *openthread_get_default_context(void);

/** @cond INTERNAL_HIDDEN */

#define OPENTHREAD_L2_CTX_TYPE struct openthread_context

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_OPENTHREAD_H_ */
