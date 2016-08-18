/** @file
 *  @brief Internal APIs for Bluetooth RFCOMM handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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

#include <bluetooth/rfcomm.h>

/* RFCOMM signalling connection specific context */
struct bt_rfcomm_session {
	/* L2CAP channel this context is associated with */
	struct bt_l2cap_br_chan br_chan;
	struct bt_rfcomm_dlc *dlcs;
	uint8_t state;
};

enum {
	BT_RFCOMM_STATE_IDLE,
	BT_RFCOMM_STATE_INIT,
};

struct bt_rfcomm_hdr {
	uint8_t address;
	uint8_t control;
	uint8_t length;
} __packed;

#define BT_RFCOMM_SIG_MIN_MTU 23

/* Helper to calculate needed outgoing buffer size */
#define BT_RFCOMM_BUF_SIZE(mtu) (CONFIG_BLUETOOTH_HCI_SEND_RESERVE + \
				sizeof(struct bt_hci_acl_hdr) + \
				sizeof(struct bt_l2cap_hdr) + \
				sizeof(struct bt_rfcomm_hdr) + (mtu))

/* Initialize RFCOMM signal layer */
void bt_rfcomm_init(void);
