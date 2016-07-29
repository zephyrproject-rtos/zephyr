/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TCP_H_
#define _TCP_H_

#include <net/net_core.h>
#include <net/ip_buf.h>

struct tcp_context {
	struct net_context *net_ctx;
	struct net_buf *rx_nbuf;
	int remaining;
};

int tcp_init(struct tcp_context *ctx);
int tcp_tx(void *ctx, const unsigned char *buf, size_t size);
int tcp_rx(void *ctx, unsigned char *buf, size_t size);

#endif
