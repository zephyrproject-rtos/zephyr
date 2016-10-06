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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef CONFIG_NET_IPV6
#define LOCAL_ADDR		{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0,\
				      0, 0, 0, 0, 0, 0x1 } } }
#define REMOTE_ADDR		{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0,\
				      0, 0, 0, 0, 0, 0x2 } } }
#else
#define LOCAL_ADDR		{ { { 192, 0, 2, 1 } } }
#define REMOTE_ADDR		{ { { 192, 0, 2, 2 } } }
#endif

#define MY_PORT			0
#define PEER_PORT		5353

#define APP_SLEEP_TICKS		500

#endif
