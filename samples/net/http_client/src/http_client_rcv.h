/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_CLIENT_RCV_H_
#define _HTTP_CLIENT_RCV_H_

#include "tcp_client.h"

/* HTTP reception callback */
void http_receive_cb(struct tcp_client_ctx *tcp_ctx, struct net_buf *rx);

#endif
