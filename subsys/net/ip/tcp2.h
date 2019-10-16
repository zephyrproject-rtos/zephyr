/*
 * Copyright (c) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TCP2_H
#define TCP2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

void tcp_input(struct net_pkt *pkt);

ssize_t tcp_recv(int fd, void *buf, size_t len, int flags);
ssize_t _tcp_send(int fd, const void *buf, size_t len, int flags);

#ifdef __cplusplus
}
#endif

#endif /* TCP2_H */
