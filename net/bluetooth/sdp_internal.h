/* sdp_internal.h - Service Discovery Protocol handling */

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

/*
 * The PDU identifiers of SDP packets between client and server
 */
#define BT_SDP_ERROR_RSP           0x01
#define BT_SDP_SVC_SEARCH_REQ      0x02
#define BT_SDP_SVC_SEARCH_RSP      0x03
#define BT_SDP_SVC_ATTR_REQ        0x04
#define BT_SDP_SVC_ATTR_RSP        0x05
#define BT_SDP_SVC_SEARCH_ATTR_REQ 0x06
#define BT_SDP_SVC_SEARCH_ATTR_RSP 0x07

/*
 * Some additions to support service registration.
 * These are outside the scope of the Bluetooth specification
 */
#define BT_SDP_SVC_REGISTER_REQ 0x75
#define BT_SDP_SVC_REGISTER_RSP 0x76
#define BT_SDP_SVC_UPDATE_REQ   0x77
#define BT_SDP_SVC_UPDATE_RSP   0x78
#define BT_SDP_SVC_REMOVE_REQ   0x79
#define BT_SDP_SVC_REMOVE_RSP   0x80

/*
 * SDP Error codes
 */
#define BT_SDP_INVALID_VERSION       0x0001
#define BT_SDP_INVALID_RECORD_HANDLE 0x0002
#define BT_SDP_INVALID_SYNTAX        0x0003
#define BT_SDP_INVALID_PDU_SIZE      0x0004
#define BT_SDP_INVALID_CSTATE        0x0005

struct bt_sdp_hdr {
	uint8_t  op_code;
	uint16_t tid;
	uint16_t param_len;
} __packed;

void bt_sdp_init(void);
