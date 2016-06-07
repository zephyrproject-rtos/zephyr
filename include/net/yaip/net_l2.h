/*
 * Copyright (c) 2016 Intel Corporation.
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

/**
 * @file
 * @brief Public API for network L2 interface
 */

#ifndef __NET_L2_H__
#define __NET_L2_H__

#include <net/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_if;

struct net_l2 {
	enum net_verdict (*recv)(struct net_if *iface, struct net_buf *buf);
	enum net_verdict (*send)(struct net_if *iface, struct net_buf *buf);
	uint16_t (*get_reserve)(struct net_if *iface);
};

#define NET_L2_GET_NAME(_name) (__net_l2_##_name)
#define NET_L2_DECLARE_PUBLIC(_name)					\
	extern const struct net_l2 const NET_L2_GET_NAME(_name)

extern struct net_l2 __net_l2_start[];

#ifdef CONFIG_NET_L2_DUMMY
#define DUMMY_L2 dummy
NET_L2_DECLARE_PUBLIC(DUMMY_L2);
#endif /* CONFIG_NET_L2_DUMMY */

#ifdef CONFIG_NET_L2_ETHERNET
#define ETHERNET_L2 ethernet
NET_L2_DECLARE_PUBLIC(ETHERNET_L2);
#endif /* CONFIG_NET_L2_ETHERNET */

extern struct net_l2 __net_l2_end[];

#define NET_L2_INIT(_name, _recv_fn, _send_fn, _reserve_fn)		\
	const struct net_l2 const (NET_L2_GET_NAME(_name)) __used	\
	__attribute__((__section__(".net_l2.init"))) = {		\
		.recv = (_recv_fn),					\
		.send = (_send_fn),					\
		.get_reserve = (_reserve_fn),				\
	}

#ifdef __cplusplus
}
#endif

#endif /* __NET_L2_H__ */
