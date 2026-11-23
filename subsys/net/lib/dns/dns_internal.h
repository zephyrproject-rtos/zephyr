/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/dns_resolve.h>

#include "dns_pack.h"

/**
 * @brief Unregister a DNS dispatcher socket unless it is dispatching.
 *
 * Like dns_dispatcher_unregister(), but instead of waiting for a dispatch
 * in progress on @p ctx it fails without changing anything. For callers
 * that hold a lock the dispatch callback may need, where waiting for the
 * dispatch would deadlock.
 *
 * @param ctx DNS socket dispatcher context.
 *
 * @retval 0 The context was unregistered.
 * @retval -EBUSY A dispatch is in progress on the context.
 * @retval <0 Other error returned by dns_dispatcher_unregister().
 */
int dns_dispatcher_try_unregister(struct dns_socket_dispatcher *ctx);

#if defined(CONFIG_NET_TEST)
int dns_validate_msg(struct dns_resolve_context *ctx,
		     struct dns_msg_t *dns_msg,
		     uint16_t *dns_id,
		     int *query_idx,
		     struct net_buf *dns_cname,
		     uint16_t *query_hash,
		     int recv_server_idx);
#endif
