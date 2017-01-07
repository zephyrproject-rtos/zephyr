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

#ifndef _UDP_H_
#define _UDP_H_

#include <net/net_core.h>

struct udp_context {
	struct net_context *net_ctx;
	struct net_buf *rx_nbuf;
	struct k_sem rx_sem;
	int remaining;
	char client_id;
};

int udp_init(struct udp_context *ctx);
int udp_tx(void *ctx, const unsigned char *buf, size_t size);
int udp_rx(void *ctx, unsigned char *buf, size_t size);

#endif
