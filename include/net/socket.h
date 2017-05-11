/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

int zsock_socket(int family, int type, int proto);
int zsock_close(int sock);

#ifdef __cplusplus
}
#endif

#endif /* __NET_SOCKET_H */
