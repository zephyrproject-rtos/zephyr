/** @file
 * @brief Network core definitions
 *
 * Definitions for networking support.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_CORE_H_
#define ZEPHYR_INCLUDE_NET_NET_CORE_H_

#include <stdbool.h>
#include <string.h>

#include <logging/log.h>
#include <sys/__assert.h>
#include <kernel.h>

#include <net/net_timeout.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Networking
 * @defgroup networking Networking
 * @{
 * @}
 */

/**
 * @brief Network core library
 * @defgroup net_core Network Core Library
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Network subsystem logging helpers */
#define NET_DBG(fmt, ...) LOG_DBG("(%p): " fmt, k_current_get(), \
				  ##__VA_ARGS__)
#define NET_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define NET_INFO(fmt, ...) LOG_INF(fmt,  ##__VA_ARGS__)

#define NET_ASSERT(cond, ...) __ASSERT(cond, "" __VA_ARGS__)

/** @endcond */

struct net_buf;
struct net_pkt;
struct net_context;
struct net_if;

/**
 * @brief Net Verdict
 */
enum net_verdict {
	/** Packet has been taken care of. */
	NET_OK,
	/** Packet has not been touched, other part should decide about its
	 * fate.
	 */
	NET_CONTINUE,
	/** Packet must be dropped. */
	NET_DROP,
};

/**
 * @brief Called by lower network stack or network device driver when
 * a network packet has been received. The function will push the packet up in
 * the network stack for further processing.
 *
 * @param iface Network interface where the packet was received.
 * @param pkt Network packet data.
 *
 * @return 0 if ok, <0 if error.
 */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Send data to network.
 *
 * @details Send data to network. This should not be used normally by
 * applications as it requires that the network packet is properly
 * constructed.
 *
 * @param pkt Network packet.
 *
 * @return 0 if ok, <0 if error. If <0 is returned, then the caller needs
 * to unref the pkt in order to avoid memory leak.
 */
int net_send_data(struct net_pkt *pkt);

/**
 * @brief Add a thread to network memory domain
 *
 * @details Mark a thread to be able to access networking memory. This
 * is used in user mode only.
 *
 * @param thread Pointer to the thread
 */
void net_mem_domain_add_thread(struct k_thread *thread);

/**
 * @brief Remove a thread from network memory domain.
 *
 * @param thread ID of thread going to be removed from network memory domain.
 */
void net_mem_domain_remove_thread(struct k_thread *thread);

/**
 * @brief Grant application access to networking domain.
 *
 * @param thread ID of thread granting the access.
 */
void net_access_grant_app(struct k_thread *thread);

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_NET_USER_MODE)
#include <app_memory/app_memdomain.h>

#define NET_THREAD_FLAGS K_USER

extern struct k_mem_partition net_partition;

#define Z_NET_PARTITION_EXISTS 1

#define NET_BMEM K_APP_BMEM(net_partition)
#define NET_DMEM K_APP_DMEM(net_partition)

void net_access_grant_rx(struct k_thread *thread);
void net_access_grant_tx(struct k_thread *thread);
#else
#define NET_THREAD_FLAGS 0
#define NET_BMEM
#define NET_DMEM

static inline void net_access_grant_rx(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void net_access_grant_tx(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}
#endif /* CONFIG_NET_USER_MODE */

/* Some helper defines for traffic class support */
#if defined(CONFIG_NET_TC_TX_COUNT) && defined(CONFIG_NET_TC_RX_COUNT)
#define NET_TC_TX_COUNT CONFIG_NET_TC_TX_COUNT
#define NET_TC_RX_COUNT CONFIG_NET_TC_RX_COUNT

#if NET_TC_TX_COUNT > NET_TC_RX_COUNT
#define NET_TC_COUNT NET_TC_TX_COUNT
#else
#define NET_TC_COUNT NET_TC_RX_COUNT
#endif
#else /* CONFIG_NET_TC_TX_COUNT && CONFIG_NET_TC_RX_COUNT */
#define NET_TC_TX_COUNT 1
#define NET_TC_RX_COUNT 1
#define NET_TC_COUNT 1
#endif /* CONFIG_NET_TC_TX_COUNT && CONFIG_NET_TC_RX_COUNT */

/* @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_CORE_H_ */
