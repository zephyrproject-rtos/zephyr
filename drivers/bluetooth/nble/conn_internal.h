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

struct bt_conn {
	uint16_t handle;
	uint8_t role;
	atomic_t ref;

	bt_addr_le_t dst;

	bt_security_t sec_level;
	bt_security_t required_sec_level;

	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;

	enum {
		BT_CONN_DISCONNECTED,
		BT_CONN_CONNECT,
		BT_CONN_CONNECTED,
		BT_CONN_DISCONNECT,
	} state;

	/* Delayed work used to update connection parameters */
	struct k_delayed_work update_work;

	void *gatt_private;
	struct k_sem gatt_notif_sem;
};
