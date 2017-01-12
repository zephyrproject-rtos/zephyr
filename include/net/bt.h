/*
 * Copyright (c) 2017 Intel Corporation.
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
 * @brief Bluetooth L2 stack public header
 */

#ifndef __BT_H__
#define __BT_H__

#include <net/net_mgmt.h>

/* Management part definitions */

#define _NET_BT_LAYER	NET_MGMT_LAYER_L2
#define _NET_BT_CODE	0x155
#define _NET_BT_BASE	(NET_MGMT_IFACE_BIT |			\
			 NET_MGMT_LAYER(_NET_BT_LAYER) |	\
			 NET_MGMT_LAYER_CODE(_NET_BT_CODE))
#define _NET_BT_EVENT	(_NET_BT_BASE | NET_MGMT_EVENT_BIT)

enum net_request_bt_cmd {
	NET_REQUEST_BT_CMD_CONNECT = 1,
	NET_REQUEST_BT_CMD_SCAN,
	NET_REQUEST_BT_CMD_DISCONNECT,
};

#define NET_REQUEST_BT_CONNECT					\
	(_NET_BT_BASE | NET_REQUEST_BT_CMD_CONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_BT_CONNECT);

#define NET_REQUEST_BT_SCAN					\
	(_NET_BT_BASE | NET_REQUEST_BT_CMD_SCAN)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_BT_SCAN);

enum net_event_bt_cmd {
	NET_EVENT_BT_CMD_SCAN_RESULT = 1,
};

#define NET_EVENT_BT_SCAN_RESULT				\
	(_NET_BT_EVENT | NET_EVENT_BT_CMD_SCAN_RESULT)

#define NET_REQUEST_BT_DISCONNECT				\
	(_NET_BT_BASE | NET_REQUEST_BT_CMD_DISCONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_BT_DISCONNECT);

#endif /* __BT_H__ */
