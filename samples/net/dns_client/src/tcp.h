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

int tcp_init(struct net_context **ctx);
int tcp_tx(struct net_context *ctx, uint8_t *buf,  size_t size);
int tcp_rx(struct net_context *ctx, uint8_t *buf, size_t *len, size_t size);

#endif
