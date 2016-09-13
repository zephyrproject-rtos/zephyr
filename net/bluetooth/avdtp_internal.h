/*
 * avdtp_internal.h - avdtp handling

 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* @brief A2DP ROLE's */
#define A2DP_SRC_ROLE 0x00
#define A2DP_SNK_ROLE 0x01

/* @brief AVDTP Role */
#define BT_AVDTP_INT 0x00
#define BT_AVDTP_ACP 0x01

#define BT_L2CAP_PSM_AVDTP 0x0019

/* AVDTP SIGNAL HEADER - Packet Type*/
#define BT_AVDTP_PACKET_TYPE_SINGLE   0x00
#define BT_AVDTP_PACKET_TYPE_START    0x01
#define BT_AVDTP_PACKET_TYPE_CONTINUE 0x02
#define BT_AVDTP_PACKET_TYPE_END      0x03

/* AVDTP SIGNAL HEADER - MESSAGE TYPE */
#define BT_AVDTP_MSG_TYPE_CMD        0x00
#define BT_AVDTP_MSG_TYPE_GEN_REJECT 0x01
#define BT_AVDTP_MSG_TYPE_ACCEPT     0x02
#define BT_AVDTP_MSG_TYPE_REJECT     0x03

/* @brief AVDTP SIGNAL HEADER - Signal Identifier */
#define BT_AVDTP_DISCOVER             0x01
#define BT_AVDTP_GET_CAPABILITIES     0x02
#define BT_AVDTP_SET_CONFIGURATION    0x03
#define BT_AVDTP_GET_CONFIGURATION    0x04
#define BT_AVDTP_RECONFIGURE          0x05
#define BT_AVDTP_OPEN                 0x06
#define BT_AVDTP_START                0x07
#define BT_AVDTP_CLOSE                0x08
#define BT_AVDTP_SUSPEND              0x09
#define BT_AVDTP_ABORT                0x0a
#define BT_AVDTP_SECURITY_CONTROL     0x0b
#define BT_AVDTP_GET_ALL_CAPABILITIES 0x0c
#define BT_AVDTP_DELAYREPORT          0x0d

/* @brief AVDTP STATE */
#define BT_AVDTP_STATE_IDLE          0x01
#define BT_AVDTP_STATE_CONFIGURED    0x02
#define BT_AVDTP_STATE_OPEN          0x03
#define BT_AVDTP_STATE_STREAMING     0x04
#define BT_AVDTP_STATE_CLOSING       0x05
#define BT_AVDTP_STATE_ABORT         0x06
#define BT_AVDTP_STATE_SIG_CONNECTED 0x07
#define BT_AVDTP_STATE_INVALID       0x00

/* @brief AVDTP Media TYPE */
#define BT_AVDTP_SERVICE_CAT_MEDIA_TRANSPORT    0x01
#define BT_AVDTP_SERVICE_CAT_REPORTING          0x02
#define BT_AVDTP_SERVICE_CAT_RECOVERY           0x03
#define BT_AVDTP_SERVICE_CAT_CONTENT_PROTECTION 0x04
#define BT_AVDTP_SERVICE_CAT_HDR_COMPRESSION    0x05
#define BT_AVDTP_SERVICE_CAT_MULTIPLEXING       0x06
#define BT_AVDTP_SERVICE_CAT_MEDIA_CODEC        0x07
#define BT_AVDTP_SERVICE_CAT_DELAYREPORTING     0x08

/* AVDTP Error Codes */
#define BT_AVDTP_SUCCESS                        0x00
#define BT_AVDTP_ERR_BAD_HDR_FORMAT             0x01
#define BT_AVDTP_ERR_BAD_LENGTH                 0x11
#define BT_AVDTP_ERR_BAD_ACP_SEID               0x12
#define BT_AVDTP_ERR_SEP_IN_USE                 0x13
#define BT_AVDTP_ERR_SEP_NOT_IN_USE             0x14
#define BT_AVDTP_ERR_BAD_SERV_CATEGORY          0x17
#define BT_AVDTP_ERR_BAD_PAYLOAD_FORMAT         0x18
#define BT_AVDTP_ERR_NOT_SUPPORTED_COMMAND      0x19
#define BT_AVDTP_ERR_INVALID_CAPABILITIES       0x1a
#define BT_AVDTP_ERR_BAD_RECOVERY_TYPE          0x22
#define BT_AVDTP_ERR_BAD_MEDIA_TRANSPORT_FORMAT 0x23
#define BT_AVDTP_ERR_BAD_RECOVERY_FORMAT        0x25
#define BT_AVDTP_ERR_BAD_ROHC_FORMAT            0x26
#define BT_AVDTP_ERR_BAD_CP_FORMAT              0x27
#define BT_AVDTP_ERR_BAD_MULTIPLEXING_FORMAT    0x28
#define BT_AVDTP_ERR_UNSUPPORTED_CONFIGURAION   0x29
#define BT_AVDTP_ERR_BAD_STATE                  0x31

#define BT_AVDTP_MIN_MTU 48
#define BT_AVDTP_MAX_MTU CONFIG_BLUETOOTH_L2CAP_IN_MTU

/* Helper to calculate needed outgoing buffer size. */
#define BT_AVDTP_BUF_SIZE(mtu) (CONFIG_BLUETOOTH_HCI_SEND_RESERVE + \
				sizeof(struct bt_hci_acl_hdr) + \
				sizeof(struct bt_l2cap_hdr) + \
				BT_AVDTP_SIG_HDR_LEN + (mtu))

struct bt_avdtp_single_sig_hdr {
	uint8_t hdr;
	uint8_t signal_id;
} __packed;

#define BT_AVDTP_SIG_HDR_LEN sizeof(struct bt_avdtp_single_sig_hdr)

struct bt_avdtp_cfm_cb {
	/*
	* Discovery_cfm;
	* get_capabilities_cfm;
	* set_configuration_cfm;
	* open_cfm;
	* start_cfm;
	* suspend_cfm;
	* close_cfm;
	*/
};

struct bt_avdtp_ind_cb {
	/*
	* discovery_ind;
	* get_capabilities_ind;
	* set_configuration_ind;
	* open_ind;
	* start_ind;
	* suspend_ind;
	* close_ind;
	*/
};

struct bt_avdtp_event_cb {
	struct bt_avdtp_ind_cb *ind;
	struct bt_avdtp_cfm_cb *cfm;
};

/** @brief Global AVDTP session structure. */
struct bt_avdtp {
	struct bt_l2cap_br_chan br_chan;
	uint8_t state;	/* current state of AVDTP*/
};

/* Initialize AVDTP layer*/
int bt_avdtp_init(void);

/* Application register with AVDTP layer */
int bt_avdtp_register(struct bt_avdtp_event_cb *cb);
