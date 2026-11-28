/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKS_H_
#define ZEPHYR_INCLUDE_NET_SOCKS_H_

#include <zephyr/net/socket.h>

/**@brief Connects to destination through a SOCKS5 proxy server.
 *
 * @param[in] ctx Network context.
 * @param[in] dest Address of the destination server.
 * @param[in] dest_len Address length of the destination server.
 * @param[in] cb Receive callback that the caller installs on success. Data
 *            the proxy relays before that, including any that came in the
 *            same segment as the CONNECT reply, is passed to it in order.
 *            Must not be NULL.
 * @param[in] user_data User data for @p cb.
 *
 * @retval 0 or an error code if it was unsuccessful.
 */
#if defined(CONFIG_SOCKS)
int net_socks5_connect(struct net_context *ctx,
		       const struct net_sockaddr *dest,
		       net_socklen_t dest_len,
		       net_context_recv_cb_t cb,
		       void *user_data);
#else
inline int net_socks5_connect(struct net_context *ctx,
			      const struct net_sockaddr *dest,
			      net_socklen_t dest_len,
			      net_context_recv_cb_t cb,
			      void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(dest);
	ARG_UNUSED(dest_len);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKS_H_ */
