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
 *
 * @retval 0 or an error code if it was unsuccessful.
 */
#if defined(CONFIG_SOCKS)
int net_socks5_connect(struct net_context *ctx,
		       const struct sockaddr *dest,
		       socklen_t dest_len);
#else
inline int net_socks5_connect(struct net_context *ctx,
			      const struct sockaddr *dest,
			      socklen_t dest_len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(dest);
	ARG_UNUSED(dest_len);

	return -ENOTSUP;
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKS_H_ */
