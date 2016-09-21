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
 * @brief Network Management API public header
 */

#ifndef __NET_MGMT_H__
#define __NET_MGMT_H__

#include <net/net_if.h>

/**
 * @brief NET MGMT event mask basics, normalizing parts of bit fields
 */
#define NET_MGMT_EVENT_MASK		0x80000000
#define NET_MGMT_ON_IFACE_MASK		0x40000000
#define NET_MGMT_LAYER_MASK		0x30000000
#define NET_MGMT_LAYER_CODE_MASK	0x0FFF0000
#define NET_MGMT_COMMAND_MASK		0x0000FFFF

#define NET_MGMT_EVENT(mgmt_request)		\
	(mgmt_request & NET_MGMT_EVENT_MASK)

#define NET_MGMT_ON_IFACE(mgmt_request)		\
	(mgmt_request & NET_MGMT_ON_IFACE_MASK)

#define NET_MGMT_GET_LAYER(mgmt_request)	\
	(mgmt_request & NET_MGMT_LAYER_MASK)

#define NET_MGMT_GET_LAYER_CODE(mgmt_request)	\
	(mgmt_request & NET_MGMT_LAYER_CODE_MASK)

#define NET_MGMT_GET_COMMAND(mgmt_request)	\
	(mgmt_request & NET_MGMT_COMMAND_MASK)

#define net_mgmt(_mgmt_request, _iface, _data, _len)			\
	net_mgmt_##_mgmt_request(_mgmt_request, _iface, _data, _len)

#define NET_MGMT_REGISTER_REQUEST_HANDLER(_mgmt_request, _func)	\
	FUNC_ALIAS(_func, net_mgmt_##_mgmt_request, int)

#endif /* __NET_MGMT_H__ */
