/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void rndis_clean(void);

/*
 * RNDIS definitons
 */

#define RNDIS_GEN_MAX_TOTAL_SIZE	1558

#define RNDIS_MAJOR_VERSION		1
#define RNDIS_MINOR_VERSION		0

#define COMPLETE			BIT(31)

#define RNDIS_DATA_PACKET		0x01
#define RNDIS_CMD_INITIALIZE		0x02
#define RNDIS_CMD_INITIALIZE_COMPLETE	(RNDIS_CMD_INITIALIZE | COMPLETE)
#define RNDIS_CMD_HALT			0x03
#define RNDIS_CMD_QUERY			0x04
#define RNDIS_CMD_QUERY_COMPLETE	(RNDIS_CMD_QUERY | COMPLETE)
#define RNDIS_CMD_SET			0x05
#define RNDIS_CMD_SET_COMPLETE		(RNDIS_CMD_SET | COMPLETE)
#define RNDIS_CMD_RESET			0x06
#define RNDIS_CMD_RESET_COMPLETE	(RNDIS_CMD_RESET | COMPLETE)
#define RNDIS_CMD_INDICATE		0x07
#define RNDIS_CMD_KEEPALIVE		0x08
#define RNDIS_CMD_KEEPALIVE_COMPLETE	(RNDIS_CMD_KEEPALIVE | COMPLETE)

#define RNDIS_CMD_STATUS_SUCCESS	0
#define RNDIS_CMD_STATUS_INVALID_DATA	0xC0010015
#define RNDIS_CMD_STATUS_NOT_SUPP	0xC00000BB

#define RNDIS_FLAG_CONNECTIONLESS	BIT(0)

#define RNDIS_MEDIUM_WIRED_ETHERNET	0

struct rndis_init_cmd {
	u32_t type;
	u32_t len;
	u32_t req_id;
	u32_t major_ver;
	u32_t minor_version;
	u32_t max_transfer_size;
} __packed;

struct rndis_init_cmd_complete {
	u32_t type;
	u32_t len;
	u32_t req_id;
	u32_t status;
	u32_t major_ver;
	u32_t minor_ver;
	u32_t flags;
	u32_t medium;
	u32_t max_packets;
	u32_t max_transfer_size;
	u32_t pkt_align_factor;
	u32_t __reserved[2];
} __packed;

struct rndis_query_cmd {
	u32_t type;
	u32_t len;
	u32_t req_id;
	u32_t object_id;
	u32_t buf_len;
	u32_t buf_offset;
	u32_t vc_handle;	/* Reserved for connection-oriented devices */
} __packed;

/* Specifies RNDS objects for Query and Set */
#define RNDIS_OBJECT_ID_GEN_SUPP_LIST		0x00010101
#define RNDIS_OBJECT_ID_GEN_HW_STATUS		0x00010102
#define RNDIS_OBJECT_ID_GEN_SUPP_MEDIA		0x00010103
#define RNDIS_OBJECT_ID_GEN_IN_USE_MEDIA	0x00010104

#define RNDIS_OBJECT_ID_GEN_MAX_FRAME_SIZE	0x00010106
#define RNDIS_OBJECT_ID_GEN_LINK_SPEED		0x00010107
#define RNDIS_OBJECT_ID_GEN_BLOCK_TX_SIZE	0x0001010A
#define RNDIS_OBJECT_ID_GEN_BLOCK_RX_SIZE	0x0001010B

#define RNDIS_OBJECT_ID_GEN_VENDOR_ID		0x0001010C
#define RNDIS_OBJECT_ID_GEN_VENDOR_DESC		0x0001010D
#define RNDIS_OBJECT_ID_GEN_VENDOR_DRV_VER	0x00010116

#define RNDIS_OBJECT_ID_GEN_PKT_FILTER		0x0001010E
#define RNDIS_OBJECT_ID_GEN_MAX_TOTAL_SIZE	0x00010111
#define RNDIS_OBJECT_ID_GEN_CONN_MEDIA_STATUS	0x00010114

#define RNDIS_OBJECT_ID_GEN_PHYSICAL_MEDIUM	0x00010202

#define RNDIS_OBJECT_ID_GEN_TRANSMIT_OK		0x00020101
#define RNDIS_OBJECT_ID_GEN_RECEIVE_OK		0x00020102
#define RNDIS_OBJECT_ID_GEN_TRANSMIT_ERROR	0x00020103
#define RNDIS_OBJECT_ID_GEN_RECEIVE_ERROR	0x00020104
#define RNDIS_OBJECT_ID_GEN_RECEIVE_NO_BUF	0x00020105

/* The address of the NIC encoded in the hardware */
#define RNDIS_OBJECT_ID_802_3_PERMANENT_ADDRESS	0x01010101
#define RNDIS_OBJECT_ID_802_3_CURR_ADDRESS	0x01010102
#define RNDIS_OBJECT_ID_802_3_MCAST_LIST	0x01010103
#define RNDIS_OBJECT_ID_802_3_MAX_LIST_SIZE	0x01010104
#define RNDIS_OBJECT_ID_802_3_MAC_OPTIONS	0x01010105

/* Media types used */
#define RNDIS_PHYSICAL_MEDIUM_TYPE_UNSPECIFIED	0x00

/* Connection Media states */
#define RNDIS_OBJECT_ID_MEDIA_CONNECTED		0x00
#define RNDIS_OBJECT_ID_MEDIA_DISCONNECTED	0x01

#define RNDIS_STATUS_CONNECT_MEDIA		0x4001000B
#define RNDIS_STATUS_DISCONNECT_MEDIA		0x4001000C

struct rndis_query_cmd_complete {
	u32_t type;
	u32_t len;
	u32_t req_id;
	u32_t status;
	u32_t buf_len;
	u32_t buf_offset;
} __packed;

struct rndis_set_cmd {
	u32_t type;
	u32_t len;
	u32_t req_id;
	u32_t object_id;
	u32_t buf_len;
	u32_t buf_offset;
	u32_t vc_handle;	/* Reserved for connection-oriented devices */
} __packed;

struct rndis_set_cmd_complete {
	u32_t type;
	u32_t len;
	u32_t req_id;
	u32_t status;
} __packed;

struct rndis_payload_packet {
	u32_t type;
	u32_t len;
	u32_t payload_offset;
	u32_t payload_len;
	u32_t oob_payload_offset;
	u32_t oob_payload_len;
	u32_t oob_num;
	u32_t pkt_payload_offset;
	u32_t pkt_payload_len;
	u32_t vc_handle;
	u32_t __reserved;
} __packed;

struct rndis_keepalive_cmd {
	u32_t type;
	u32_t len;
	u32_t req_id;
} __packed;

struct rndis_keepalive_cmd_complete {
	u32_t type;
	u32_t len;
	u32_t req_id;
	u32_t status;
} __packed;

struct rndis_media_status_indicate {
	u32_t type;
	u32_t len;
	u32_t status;
	u32_t buf_len;
	u32_t buf_offset;
} __packed;

struct rndis_reset_cmd_complete {
	u32_t type;
	u32_t len;
	u32_t status;
	u32_t addr_reset;
} __packed;
