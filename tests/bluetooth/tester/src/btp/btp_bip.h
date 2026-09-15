/* btp_bip.h - Bluetooth BIP tester headers */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/classic/bip.h>

/* BIP Service */
/* commands */
#define BTP_BIP_READ_SUPPORTED_COMMANDS 0x01
struct btp_bip_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_BIP_CONNECT_RFCOMM 0x03
struct btp_bip_connect_rfcomm_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_BIP_DISCONNECT_RFCOMM 0x04
struct btp_bip_disconnect_rfcomm_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_CONNECT_L2CAP 0x06
struct btp_bip_connect_l2cap_cmd {
	bt_addr_le_t address;
	uint16_t psm;
} __packed;

#define BTP_BIP_DISCONNECT_L2CAP 0x07
struct btp_bip_disconnect_l2cap_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_SDP_DISCOVER 0x09
struct btp_bip_sdp_discover_cmd {
	bt_addr_le_t address;
	uint16_t uuid;

} __packed;

#define BTP_BIP_SERVER_REGISTER 0x0a
struct btp_bip_server_register_cmd {
	bt_addr_le_t address;
	uint8_t conn_type;
} __packed;

#define BTP_BIP_SERVER_UNREGISTER 0x0b
struct btp_bip_server_unregister_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_CLIENT_CONNECT 0x0c
struct btp_bip_client_connect_cmd {
	bt_addr_le_t address;
	uint8_t conn_type;
} __packed;

#define BTP_BIP_OBEX_DISCONNECT 0x0d
struct btp_bip_obex_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_OBEX_ABORT 0x0e
struct btp_bip_obex_abort_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_CONNECT_RSP 0x0f

struct btp_bip_connect_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_DISCONNECT_RSP 0x10
struct btp_bip_disconnect_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_BIP_ABORT_RSP 0x11
struct btp_bip_abort_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_BIP_GET_CAPABILITIES 0x12
struct btp_bip_get_capabilities_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_CAPABILITIES_RSP 0x13
struct btp_bip_get_capabilities_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_IMAGE_LIST 0x14
struct btp_bip_get_image_list_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_IMAGE_LIST_RSP 0x15
struct btp_bip_get_image_list_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_IMAGE_PROPERTIES 0x16
struct btp_bip_get_image_properties_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_IMAGE_PROPERTIES_RSP 0x17
struct btp_bip_get_image_properties_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_IMAGE 0x18
struct btp_bip_get_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_IMAGE_RSP 0x19
struct btp_bip_get_image_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_LINKED_THUMBNAIL 0x1a
struct btp_bip_get_linked_thumbnail_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_LINKED_THUMBNAIL_RSP 0x1b
struct btp_bip_get_linked_thumbnail_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_LINKED_ATTACHMENT 0x1c
struct btp_bip_get_linked_attachment_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_LINKED_ATTACHMENT_RSP 0x1d
struct btp_bip_get_linked_attachment_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_PARTIAL_IMAGE 0x1e
struct btp_bip_get_partial_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_PARTIAL_IMAGE_RSP 0x1f
struct btp_bip_get_partial_image_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_MONITORING_IMAGE 0x20
struct btp_bip_get_monitoring_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_MONITORING_IMAGE_RSP 0x21
struct btp_bip_get_monitoring_image_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_STATUS 0x22
struct btp_bip_get_status_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_GET_STATUS_RSP 0x23
struct btp_bip_get_status_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_PUT_IMAGE 0x24
struct btp_bip_put_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_PUT_IMAGE_RSP 0x25
struct btp_bip_put_image_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_PUT_LINKED_THUMBNAIL 0x26
struct btp_bip_put_linked_thumbnail_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_PUT_LINKED_THUMBNAIL_RSP 0x27
struct btp_bip_put_linked_thumbnail_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_PUT_LINKED_ATTACHMENT 0x28
struct btp_bip_put_linked_attachment_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_PUT_LINKED_ATTACHMENT_RSP 0x29
struct btp_bip_put_linked_attachment_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_REMOTE_DISPLAY 0x2a
struct btp_bip_remote_display_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_REMOTE_DISPLAY_RSP 0x2b
struct btp_bip_remote_display_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_DELETE_IMAGE 0x2c
struct btp_bip_delete_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_DELETE_IMAGE_RSP 0x2d
struct btp_bip_delete_image_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_START_PRINT 0x2e
struct btp_bip_start_print_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_START_PRINT_RSP 0x2f
struct btp_bip_start_print_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_START_ARCHIVE 0x30
struct btp_bip_start_archive_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_START_ARCHIVE_RSP 0x31
struct btp_bip_start_archive_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

/* Secondary connection commands */
#define BTP_BIP_SECOND_SERVER_REGISTER 0x32
struct btp_bip_second_server_register_cmd {
	bt_addr_le_t address;
	uint8_t conn_type;
} __packed;

#define BTP_BIP_SECOND_CONNECT 0x33
struct btp_bip_second_connect_cmd {
	bt_addr_le_t address;
	uint8_t conn_type;
} __packed;

#define BTP_BIP_SECOND_OBEX_DISCONNECT 0x34
struct btp_bip_second_obex_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_SECOND_OBEX_ABORT 0x35
struct btp_bip_second_obex_abort_cmd {
	bt_addr_le_t address;
} __packed;


#define BTP_BIP_SECOND_CONNECT_RSP 0x36
struct btp_bip_second_connect_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_DISCONNECT_RSP 0x37
struct btp_bip_second_disconnect_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_BIP_SECOND_ABORT_RSP 0x38
struct btp_bip_second_abort_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_BIP_SECOND_SERVER_UNREGISTER 0x39
struct btp_bip_second_server_unregister_cmd {
	bt_addr_le_t address;
} __packed;

/*
 * Secondary (Archived Objects) response commands.
 *
 * GetCapabilities, GetImagesList, GetImageProperties, GetImage,
 * GetLinkedThumbnail, GetLinkedAttachment and DeleteImage are valid on both the
 * primary imaging connection and the Archived Objects secondary connection. The
 * primary variants use BTP_BIP_*_RSP (routed to inst->server); these secondary
 * variants are routed to inst->second_server so the response is emitted on the
 * correct OBEX connection/transport.
 */
#define BTP_BIP_SECOND_GET_CAPABILITIES_RSP 0x3a
struct btp_bip_second_get_capabilities_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_IMAGE_LIST_RSP 0x3b
struct btp_bip_second_get_image_list_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_IMAGE_PROPERTIES_RSP 0x3c
struct btp_bip_second_get_image_properties_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_IMAGE_RSP 0x3d
struct btp_bip_second_get_image_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_LINKED_THUMBNAIL_RSP 0x3e
struct btp_bip_second_get_linked_thumbnail_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_LINKED_ATTACHMENT_RSP 0x3f
struct btp_bip_second_get_linked_attachment_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_DELETE_IMAGE_RSP 0x40
struct btp_bip_second_delete_image_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

/*
 * Secondary transport connect commands.
 *
 * In the Auto-Archive (AAI/ACH) role the IUT is the primary Auto-Archive
 * server, but it actively INITIATES the secondary Archived-Objects transport
 * as an Initiator. These commands explicitly bind the new L2CAP/RFCOMM
 * transport to inst->second_bip (rather than inst->bip), so the subsequent
 * BTP_BIP_SECOND_CONNECT can set capabilities/features/functions on a bt_bip
 * whose role has been set to Initiator.
 */
#define BTP_BIP_SECOND_CONNECT_L2CAP 0x41
struct btp_bip_second_connect_l2cap_cmd {
	bt_addr_le_t address;
	uint16_t psm;
} __packed;

#define BTP_BIP_SECOND_CONNECT_RFCOMM 0x42
struct btp_bip_second_connect_rfcomm_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

/*
 * Secondary (Archived/Referenced Objects) client request command.
 *
 * GetImagesList issued over the secondary OBEX connection. The request is sent
 * from the secondary client (inst->second_client) over the secondary transport
 * (inst->second_bip), analogous to the primary BTP_BIP_GET_IMAGE_LIST which is
 * routed to inst->client.
 */
#define BTP_BIP_SECOND_GET_IMAGE_LIST 0x43
struct btp_bip_second_get_image_list_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

/*
 * Additional secondary (Archived/Referenced Objects) client request commands.
 *
 * GetCapabilities, GetImageProperties, GetImage, GetLinkedThumbnail,
 * GetLinkedAttachment, GetPartialImage and DeleteImage issued over the
 * secondary OBEX connection. Each request is sent from the secondary client
 * (inst->second_client) over the secondary transport (inst->second_bip),
 * analogous to the matching primary BTP_BIP_GET_* commands which are routed to
 * inst->client.
 */
#define BTP_BIP_SECOND_GET_CAPABILITIES 0x44
struct btp_bip_second_get_capabilities_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_IMAGE_PROPERTIES 0x45
struct btp_bip_second_get_image_properties_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_IMAGE 0x46
struct btp_bip_second_get_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_LINKED_THUMBNAIL 0x47
struct btp_bip_second_get_linked_thumbnail_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_LINKED_ATTACHMENT 0x48
struct btp_bip_second_get_linked_attachment_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_GET_PARTIAL_IMAGE 0x49
struct btp_bip_second_get_partial_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_SECOND_DELETE_IMAGE 0x4a
struct btp_bip_second_delete_image_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

/*
 * Secondary transport disconnect commands.
 *
 * Counterparts to BTP_BIP_SECOND_CONNECT_L2CAP/RFCOMM: tear down the secondary
 * (Archived/Referenced Objects) transport that the IUT itself initiated as an
 * Initiator. They act on inst->second_bip rather than inst->bip.
 */
#define BTP_BIP_SECOND_DISCONNECT_L2CAP 0x4b
struct btp_bip_second_disconnect_l2cap_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_SECOND_DISCONNECT_RFCOMM 0x4c
struct btp_bip_second_disconnect_rfcomm_cmd {
	bt_addr_le_t address;
} __packed;


/* events */





#define BTP_BIP_EV_RFCOMM_CONNECTED 0x80
struct btp_bip_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_EV_RFCOMM_DISCONNECTED 0x81
struct btp_bip_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_EV_L2CAP_CONNECTED 0x82
struct btp_bip_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_EV_L2CAP_DISCONNECTED 0x83
struct btp_bip_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_EV_SERVER_CONNECT_REQ 0x84
struct btp_bip_server_connect_req_ev {
	bt_addr_le_t address;
	uint8_t version;
	uint16_t mopl;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_DISCONNECT_REQ 0x85
struct btp_bip_server_disconnect_req_ev {
	bt_addr_le_t address;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_ABORT_REQ 0x86
struct btp_bip_server_abort_req_ev {
	bt_addr_le_t address;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_CAPS_REQ 0x87
struct btp_bip_server_get_caps_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_IMAGE_LIST_REQ 0x88
struct btp_bip_server_get_image_list_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_IMAGE_PROPERTIES_REQ 0x89
struct btp_bip_server_get_image_properties_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_IMAGE_REQ 0x8a
struct btp_bip_server_get_image_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_LINKED_THUMBNAIL_REQ 0x8b
struct btp_bip_server_get_linked_thumbnail_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_LINKED_ATTACHMENT_REQ 0x8c
struct btp_bip_server_get_linked_attachment_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_PARTIAL_IMAGE_REQ 0x8d
struct btp_bip_server_get_partial_image_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_MONITORING_IMAGE_REQ 0x8e
struct btp_bip_server_get_monitoring_image_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_GET_STATUS_REQ 0x8f
struct btp_bip_server_get_status_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_PUT_IMAGE_REQ 0x90
struct btp_bip_server_put_image_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_PUT_LINKED_THUMBNAIL_REQ 0x91
struct btp_bip_server_put_linked_thumbnail_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_PUT_LINKED_ATTACHMENT_REQ 0x92
struct btp_bip_server_put_linked_attachment_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_REMOTE_DISPLAY_REQ 0x93
struct btp_bip_server_remote_display_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_DELETE_IMAGE_REQ 0x94
struct btp_bip_server_delete_image_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_START_PRINT_REQ 0x95
struct btp_bip_server_start_print_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SERVER_START_ARCHIVE_REQ 0x96
struct btp_bip_server_start_archive_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_CONNECTED 0x97
struct btp_bip_client_connected_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint8_t version;
	uint16_t mopl;
	uint32_t conn_id;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_DISCONNECTED 0x98
struct btp_bip_client_disconnected_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_ABORTED 0x99
struct btp_bip_client_aborted_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_CAPS_RSP 0x9a
struct btp_bip_client_get_caps_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_IMAGE_LIST_RSP 0x9b
struct btp_bip_client_get_image_list_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_IMAGE_PROPERTIES_RSP 0x9c
struct btp_bip_client_get_image_properties_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_IMAGE_RSP 0x9d
struct btp_bip_client_get_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_LINKED_THUMBNAIL_RSP 0x9e
struct btp_bip_client_get_linked_thumbnail_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_LINKED_ATTACHMENT_RSP 0x9f
struct btp_bip_client_get_linked_attachment_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_PARTIAL_IMAGE_RSP 0xa0
struct btp_bip_client_get_partial_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_MONITORING_IMAGE_RSP 0xa1
struct btp_bip_client_get_monitoring_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_GET_STATUS_RSP 0xa2
struct btp_bip_client_get_status_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_PUT_IMAGE_RSP 0xa3
struct btp_bip_client_put_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_PUT_LINKED_THUMBNAIL_RSP 0xa4
struct btp_bip_client_put_linked_thumbnail_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_PUT_LINKED_ATTACHMENT_RSP 0xa5
struct btp_bip_client_put_linked_attachment_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_REMOTE_DISPLAY_RSP 0xa6
struct btp_bip_client_remote_display_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_DELETE_IMAGE_RSP 0xa7
struct btp_bip_client_delete_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_START_PRINT_RSP 0xa8
struct btp_bip_client_start_print_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_CLIENT_START_ARCHIVE_RSP 0xa9
struct btp_bip_client_start_archive_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SDP_DISCOVERED 0xaa
struct btp_bip_sdp_discovered_ev {
	bt_addr_le_t address;
	uint8_t channel;
	uint16_t psm;
	uint8_t caps;
	uint16_t features;
	uint32_t functions;
} __packed;

/* Secondary connection events */
#define BTP_BIP_EV_SECOND_SERVER_CONNECT_REQ 0xab
struct btp_bip_second_server_connect_req_ev {
	bt_addr_le_t address;
	uint8_t version;
	uint16_t mopl;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_DISCONNECT_REQ 0xac
struct btp_bip_second_server_disconnect_req_ev {
	bt_addr_le_t address;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_ABORT_REQ 0xad
struct btp_bip_second_server_abort_req_ev {
	bt_addr_le_t address;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_CONNECTED 0xae
struct btp_bip_second_client_connected_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint8_t version;
	uint16_t mopl;
	uint32_t conn_id;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_DISCONNECTED 0xaf
struct btp_bip_second_client_disconnected_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_ABORTED 0xb0
struct btp_bip_second_client_aborted_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

/*
 * Secondary (Archived Objects) request events.
 *
 * The Archived Objects secondary connection serves the same GET operations as
 * the primary imaging connection. To let the upper tester distinguish which
 * OBEX connection a request arrived on, the secondary server emits these
 * dedicated event opcodes instead of the primary BTP_BIP_EV_SERVER_GET_*_REQ
 * ones. The upper tester replies with the matching BTP_BIP_SECOND_*_RSP command.
 */
#define BTP_BIP_EV_SECOND_SERVER_GET_CAPS_REQ 0xb1
struct btp_bip_second_server_get_caps_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_LIST_REQ 0xb2
struct btp_bip_second_server_get_image_list_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_PROPERTIES_REQ 0xb3
struct btp_bip_second_server_get_image_properties_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_GET_IMAGE_REQ 0xb4
struct btp_bip_second_server_get_image_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_GET_LINKED_THUMBNAIL_REQ 0xb5
struct btp_bip_second_server_get_linked_thumbnail_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_GET_LINKED_ATTACHMENT_REQ 0xb6
struct btp_bip_second_server_get_linked_attachment_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_SERVER_DELETE_IMAGE_REQ 0xb7
struct btp_bip_second_server_delete_image_req_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t data_len;
	uint8_t data[];
} __packed;

/*
 * Secondary (Archived Objects) client response events.
 *
 * When the IUT acts as the client on the secondary OBEX connection (for
 * example BIP/AAI/ACH, where the IUT pulls the archived image list from the
 * peer), the response to each GET/DELETE operation must be reported to the
 * upper tester on a dedicated event opcode. Reusing the primary
 * BTP_BIP_EV_CLIENT_GET_*_RSP events would make the upper tester believe the
 * response belongs to the primary imaging connection. These mirror the
 * secondary server request events but travel in the opposite direction.
 */
#define BTP_BIP_EV_SECOND_CLIENT_GET_CAPS_RSP 0xb8
struct btp_bip_second_client_get_caps_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_LIST_RSP 0xb9
struct btp_bip_second_client_get_image_list_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_PROPERTIES_RSP 0xba
struct btp_bip_second_client_get_image_properties_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_GET_IMAGE_RSP 0xbb
struct btp_bip_second_client_get_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_GET_LINKED_THUMBNAIL_RSP 0xbc
struct btp_bip_second_client_get_linked_thumbnail_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_GET_LINKED_ATTACHMENT_RSP 0xbd
struct btp_bip_second_client_get_linked_attachment_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_GET_PARTIAL_IMAGE_RSP 0xbe
struct btp_bip_second_client_get_partial_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BIP_EV_SECOND_CLIENT_DELETE_IMAGE_RSP 0xbf
struct btp_bip_second_client_delete_image_rsp_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

/*
 * Secondary transport connected/disconnected events.
 *
 * Separate from the primary transport events (0x80-0x83) so the upper tester
 * can tell which of the two transports (primary vs secondary) connected or
 * disconnected. In the Auto-Archive scenario both transports share the same
 * peer address and PSM, so without distinct opcodes the upper tester cannot
 * distinguish them.
 */
#define BTP_BIP_EV_SECOND_RFCOMM_CONNECTED 0xc0
struct btp_bip_second_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_EV_SECOND_RFCOMM_DISCONNECTED 0xc1
struct btp_bip_second_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_EV_SECOND_L2CAP_CONNECTED 0xc2
struct btp_bip_second_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BIP_EV_SECOND_L2CAP_DISCONNECTED 0xc3
struct btp_bip_second_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;
