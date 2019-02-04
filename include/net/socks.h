/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKS_H_
#define ZEPHYR_INCLUDE_NET_SOCKS_H_

#include <net/socket.h>

/**@brief Connects to destination through a SOCKS5 proxy server.
 *
 * @param[in] proxy Address of the proxy server.
 * @param[in] destination Address of the destination server.
 *
 * @retval File descriptor of the opened connection or an error code if it was
 *         unsuccessful.
 */
#if defined(CONFIG_SOCKS)
int socks5_client_tcp_connect(const struct sockaddr *proxy,
			      const struct sockaddr *destination);
#else
inline int socks5_client_tcp_connect(const struct sockaddr *proxy,
				     const struct sockaddr *destination)
{
	ARG_UNUSED(proxy);
	ARG_UNUSED(destination);

	return 0;
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKS_H_ */
