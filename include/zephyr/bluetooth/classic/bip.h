/* bip.h - Bluetooth Basic Imaging Profile handling */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_BIP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_BIP_H_

/**
 * @brief Basic Imaging Profile (BIP)
 * @defgroup bt_bip Basic Imaging Profile (BIP)
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/goep.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief OBEX Type header for Get Capabilities operation */
#define BT_BIP_HDR_TYPE_GET_CAPS              "x-bt/img-capabilities"
/** @brief OBEX Type header for Get Image List operation */
#define BT_BIP_HDR_TYPE_GET_IMAGE_LIST        "x-bt/img-listing"
/** @brief OBEX Type header for Get Image Properties operation */
#define BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES  "x-bt/img-properties"
/** @brief OBEX Type header for Get Image operation */
#define BT_BIP_HDR_TYPE_GET_IMAGE             "x-bt/img-img"
/** @brief OBEX Type header for Get Linked Thumbnail operation */
#define BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL  "x-bt/img-thm"
/** @brief OBEX Type header for Get Linked Attachment operation */
#define BT_BIP_HDR_TYPE_GET_LINKED_ATTACHMENT "x-bt/img-attachment"
/** @brief OBEX Type header for Get Partial Image operation */
#define BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE     "x-bt/img-partial"
/** @brief OBEX Type header for Get Monitoring Image operation */
#define BT_BIP_HDR_TYPE_GET_MONITORING_IMAGE  "x-bt/img-monitoring"
/** @brief OBEX Type header for Get Status operation */
#define BT_BIP_HDR_TYPE_GET_STATUS            "x-bt/img-status"
/** @brief OBEX Type header for Put Image operation */
#define BT_BIP_HDR_TYPE_PUT_IMAGE             "x-bt/img-img"
/** @brief OBEX Type header for Put Linked Thumbnail operation */
#define BT_BIP_HDR_TYPE_PUT_LINKED_THUMBNAIL  "x-bt/img-thm"
/** @brief OBEX Type header for Put Linked Attachment operation */
#define BT_BIP_HDR_TYPE_PUT_LINKED_ATTACHMENT "x-bt/img-attachment"
/** @brief OBEX Type header for Remote Display operation */
#define BT_BIP_HDR_TYPE_REMOTE_DISPLAY        "x-bt/img-display"
/** @brief OBEX Type header for Delete Image operation */
#define BT_BIP_HDR_TYPE_DELETE_IMAGE          "x-bt/img-img"
/** @brief OBEX Type header for Start Print operation */
#define BT_BIP_HDR_TYPE_START_PRINT           "x-bt/img-print"
/** @brief OBEX Type header for Start Archive operation */
#define BT_BIP_HDR_TYPE_START_ARCHIVE         "x-bt/img-archive"

/**
 * @brief Img-Descriptor header ID
 *
 * Byte sequence, length prefixed with a two-byte unsigned integer
 */
#define BT_BIP_HEADER_ID_IMG_DESC 0x71

/**
 * @brief Img-Handle header ID
 *
 * Null terminated, UTF-16 encoded Unicode text length prefixed with a two-byte unsigned integer
 */
#define BT_BIP_HEADER_ID_IMG_HANDLE 0x30

/**
 * @brief BIP Application Parameter Tag IDs
 *
 * These tag IDs are used in OBEX application parameter headers for various BIP operations.
 */
enum bt_bip_appl_param_tag_id {
	/** @brief Number of returned handles parameter */
	BT_BIP_APPL_PARAM_TAG_ID_RETURNED_HANDLES = 0x01,
	/** @brief List start offset parameter */
	BT_BIP_APPL_PARAM_TAG_ID_LIST_START_OFFSET = 0x02,
	/** @brief Latest captured images parameter */
	BT_BIP_APPL_PARAM_TAG_ID_LATEST_CAPTURED_IMAGES = 0x03,
	/** @brief Partial file length parameter */
	BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_LEN = 0x04,
	/** @brief Partial file start offset parameter */
	BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_START_OFFSET = 0x05,
	/** @brief Total file size parameter */
	BT_BIP_APPL_PARAM_TAG_ID_TOTAL_FILE_SIZE = 0x06,
	/** @brief End flag parameter */
	BT_BIP_APPL_PARAM_TAG_ID_END_FLAG = 0x07,
	/** @brief Remote display parameter */
	BT_BIP_APPL_PARAM_TAG_ID_REMOTE_DISPLAY = 0x08,
	/** @brief Service ID parameter */
	BT_BIP_APPL_PARAM_TAG_ID_SERVICE_ID = 0x09,
	/** @brief Store flag parameter */
	BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG = 0x0a,
};

/** @brief Forward declaration of BIP instance structure */
struct bt_bip;

/**
 * @brief BIP transport layer operations
 *
 * Callback structure for transport layer events
 */
struct bt_bip_transport_ops {
	/**
	 * @brief Transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM/L2CAP) is connected
	 *
	 * @param conn Bluetooth connection
	 * @param bip BIP instance
	 */
	void (*connected)(struct bt_conn *conn, struct bt_bip *bip);

	/**
	 * @brief Transport disconnected callback
	 *
	 * Called when the underlying transport is disconnected
	 *
	 * @param bip BIP instance
	 */
	void (*disconnected)(struct bt_bip *bip);
};

/**
 * @brief BIP RFCOMM server structure
 *
 * Structure for BIP server using RFCOMM transport
 */
struct bt_bip_rfcomm_server {
	/** @brief Underlying GOEP RFCOMM server */
	struct bt_goep_transport_rfcomm_server server;

	/**
	 * @brief Accept connection callback
	 *
	 * Called when a new RFCOMM connection is accepted
	 *
	 * @param conn Bluetooth connection
	 * @param server RFCOMM server instance
	 * @param bip Pointer to store the created BIP instance
	 * @return 0 on success, negative error code on failure
	 */
	int (*accept)(struct bt_conn *conn, struct bt_bip_rfcomm_server *server,
		      struct bt_bip **bip);
};

/**
 * @brief Register BIP RFCOMM server
 *
 * @param server RFCOMM server to register
 * @return 0 on success, negative error code on failure
 */
int bt_bip_rfcomm_register(struct bt_bip_rfcomm_server *server);

/**
 * @brief Create transport connection over RFCOMM
 *
 * @param conn ACL conn object
 * @param bip BIP instance
 * @param channel RFCOMM channel number
 * @return 0 on success, negative error code on failure
 */
int bt_bip_rfcomm_connect(struct bt_conn *conn, struct bt_bip *bip, uint8_t channel);

/**
 * @brief Disconnect BIP RFCOMM connection
 *
 * @param bip BIP instance
 * @return 0 on success, negative error code on failure
 */
int bt_bip_rfcomm_disconnect(struct bt_bip *bip);

/**
 * @brief BIP L2CAP server structure
 *
 * Structure for BIP server using L2CAP transport
 */
struct bt_bip_l2cap_server {
	/** @brief Underlying GOEP L2CAP server */
	struct bt_goep_transport_l2cap_server server;

	/**
	 * @brief Accept connection callback
	 *
	 * Called when a new L2CAP connection is accepted
	 *
	 * @param conn Bluetooth connection
	 * @param server L2CAP server instance
	 * @param bip Pointer to store the created BIP instance
	 * @return 0 on success, negative error code on failure
	 */
	int (*accept)(struct bt_conn *conn, struct bt_bip_l2cap_server *server,
		      struct bt_bip **bip);
};

/**
 * @brief Register BIP L2CAP server
 *
 * @param server L2CAP server to register
 * @return 0 on success, negative error code on failure
 */
int bt_bip_l2cap_register(struct bt_bip_l2cap_server *server);

/**
 * @brief Create transport connection over L2CAP
 *
 * @param conn ACL conn object
 * @param bip BIP instance
 * @param psm L2CAP PSM (Protocol Service Multiplexer)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_l2cap_connect(struct bt_conn *conn, struct bt_bip *bip, uint16_t psm);

/**
 * @brief Disconnect BIP L2CAP connection
 *
 * @param bip BIP instance
 * @return 0 on success, negative error code on failure
 */
int bt_bip_l2cap_disconnect(struct bt_bip *bip);

/**
 * @brief BIP OBEX connection types
 *
 * Defines the different types of BIP connections as per BIP specification
 */
enum __packed bt_bip_conn_type {
	/** @brief Primary Image Push connection */
	BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH = 0,
	/** @brief Primary Image Pull connection */
	BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL = 1,
	/** @brief Primary Advanced Image Printing connection */
	BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING = 2,
	/** @brief Primary Auto Archive connection */
	BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE = 3,
	/** @brief Primary Remote Camera connection */
	BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA = 4,
	/** @brief Primary Remote Display connection */
	BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY = 5,
	/** @brief Secondary Referenced Objects connection */
	BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS = 10,
	/** @brief Secondary Archived Objects connection */
	BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS = 11,
};

/**
 * @brief BIP role types
 *
 * Defines the role of the device in BIP connection
 */
enum __packed bt_bip_role {
	/** @brief Unknown role */
	BT_BIP_ROLE_UNKNOWN = 0,
	/** @brief Connection initiator */
	BT_BIP_ROLE_INITIATOR = 1,
	/** @brief Connection responder */
	BT_BIP_ROLE_RESPONDER = 2,
};

/**
 * @brief BIP transport state
 *
 * Defines the state of the underlying transport connection
 */
enum __packed bt_bip_transport_state {
	/** @brief Transport is disconnected */
	BT_BIP_TRANSPORT_STATE_DISCONNECTED = 0,
	/** @brief Transport is connecting */
	BT_BIP_TRANSPORT_STATE_CONNECTING = 1,
	/** @brief Transport is connected */
	BT_BIP_TRANSPORT_STATE_CONNECTED = 2,
	/** @brief Transport is disconnecting */
	BT_BIP_TRANSPORT_STATE_DISCONNECTING = 3,
};

/**
 * @brief BIP supported capabilities
 *
 * Defines the supported capabilities of the BIP device (responder).
 * These capabilities indicate the primary BIP services that the device supports.
 * Multiple capabilities can be combined using bitwise OR operations.
 */
enum __packed bt_bip_supported_capabilities {
	/** @brief Generic imaging */
	BT_BIP_SUPP_CAP_GENERIC_IMAGE = 0,
	/** @brief Capturing */
	BT_BIP_SUPP_CAP_CAPTURING = 1,
	/** @brief Printing */
	BT_BIP_SUPP_CAP_PRINTING = 2,
	/** @brief Displaying */
	BT_BIP_SUPP_CAP_DISPLAYING = 3,
};

/**
 * @brief BIP supported features
 *
 * Defines the supported features of the BIP device (responder).
 * These features indicate the primary BIP services that the device supports.
 * Multiple features can be combined using bitwise OR operations.
 */
enum __packed bt_bip_supported_features {
	/** @brief Image Push feature - allows pushing images to the peer device */
	BT_BIP_SUPP_FEAT_IMAGE_PUSH = 0,
	/** @brief Image Push Store feature - supports storing pushed images permanently */
	BT_BIP_SUPP_FEAT_IMAGE_PUSH_STORE = 1,
	/** @brief Image Push Print feature - supports printing pushed images directly */
	BT_BIP_SUPP_FEAT_IMAGE_PUSH_PRINT = 2,
	/** @brief Image Push Display feature - supports displaying pushed images */
	BT_BIP_SUPP_FEAT_IMAGE_PUSH_DISPLAY = 3,
	/** @brief Image Pull feature - allows pulling/retrieving images from the peer */
	BT_BIP_SUPP_FEAT_IMAGE_PULL = 4,
	/** @brief Advanced Image Printing feature - supports advanced printing operations */
	BT_BIP_SUPP_FEAT_ADVANCED_IMAGE_PRINT = 5,
	/** @brief Auto Archive feature - supports automatic archiving of images */
	BT_BIP_SUPP_FEAT_AUTO_ARCHIVE = 6,
	/** @brief Remote Camera feature - supports remote camera control and image capture */
	BT_BIP_SUPP_FEAT_REMOTE_CAMERA = 7,
	/** @brief Remote Display feature - supports remote display control */
	BT_BIP_SUPP_FEAT_REMOTE_DISPLAY = 8,
};

/**
 * @brief BIP supported functions
 *
 * Defines the supported functions of the BIP device (responder).
 * These functions indicate the specific BIP operations that the device can perform.
 * Multiple functions can be combined using bitwise OR operations.
 *
 * @note These values correspond to the BIP specification function definitions
 *       and are used to determine which operations can be performed on the peer device.
 */
enum __packed bt_bip_supported_functions {
	/** @brief Get Capabilities function - supports retrieving device capabilities */
	BT_BIP_SUPP_FUNC_GET_CAPS = 0,
	/** @brief Put Image function - supports receiving/storing images */
	BT_BIP_SUPP_FUNC_PUT_IMAGE = 1,
	/** @brief Put Linked Attachment function - supports receiving image attachments */
	BT_BIP_SUPP_FUNC_PUT_LINKED_ATTACHMENT = 2,
	/** @brief Put Linked Thumbnail function - supports receiving image thumbnails */
	BT_BIP_SUPP_FUNC_PUT_LINKED_THUMBNAIL = 3,
	/** @brief Remote Display function - supports remote display operations */
	BT_BIP_SUPP_FUNC_REMOTE_DISPLAY = 4,
	/** @brief Get Images List function - supports retrieving available image lists */
	BT_BIP_SUPP_FUNC_GET_IMAGE_LIST = 5,
	/** @brief Get Image Properties function - supports retrieving image metadata */
	BT_BIP_SUPP_FUNC_GET_IMAGE_PROPERTIES = 6,
	/** @brief Get Image function - supports retrieving/sending images */
	BT_BIP_SUPP_FUNC_GET_IMAGE = 7,
	/** @brief Get Linked Thumbnail function - supports retrieving image thumbnails */
	BT_BIP_SUPP_FUNC_GET_LINKED_THUMBNAIL = 8,
	/** @brief Get Linked Attachment function - supports retrieving image attachments */
	BT_BIP_SUPP_FUNC_GET_LINKED_ATTACHMENT = 9,
	/** @brief Delete Image function - supports deleting images on the peer device */
	BT_BIP_SUPP_FUNC_DELETE_IMAGE = 10,
	/** @brief Start Print function - supports initiating print operations */
	BT_BIP_SUPP_FUNC_START_PRINT = 11,
	/** @brief Get Partial Image function - supports retrieving partial image data */
	BT_BIP_SUPP_FUNC_GET_PARTIAL_IMAGE = 12,
	/** @brief Start Archive function - supports initiating archive operations */
	BT_BIP_SUPP_FUNC_START_ARCHIVE = 13,
	/** @brief Get Monitoring Image function - supports retrieving monitoring images from
	 *  remote camera
	 */
	BT_BIP_SUPP_FUNC_GET_MONITORING_IMAGE = 14,
	/** @brief Get Status function - supports retrieving device/operation status */
	BT_BIP_SUPP_FUNC_GET_STATUS = 16,
};

/**
 * @brief BIP instance structure
 *
 * Main structure representing a BIP session
 */
struct bt_bip {
	/** @brief Underlying GOEP instance */
	struct bt_goep goep;
	/** @brief Role in the connection */
	enum bt_bip_role role;

	/** @brief Transport operation callbacks */
	const struct bt_bip_transport_ops *ops;

	/** @internal Transport state (atomic) */
	atomic_t _transport_state;

	/** @internal Responder supported capabilities */
	uint8_t _supp_caps;

	/** @internal Responder supported features */
	uint16_t _supp_feats;

	/** @internal Responder supported functions */
	uint32_t _supp_funcs;

	/** @internal List of clients */
	sys_slist_t _clients;

	/** @internal List of servers */
	sys_slist_t _servers;

	/** @internal Node for linking in lists */
	sys_snode_t _node;
};

/**
 * @brief Set supported capabilities of BIP responder
 *
 * Set the supported capabilities bitmask of peer BIP responder to local BIP initiator.
 *
 * @param bip BIP initiator instance
 * @param capabilities Bitmask of supported capabilities. It is discovered from BIP responder
 *                     by SDP discovery.
 *
 * @return 0 on success, negative error code on failure
 *
 * @note This function should be called before establishing any BIP OBEX connections.
 * @note The capabilities value is typically obtained through SDP discovery.
 */
int bt_bip_set_supported_capabilities(struct bt_bip *bip, uint8_t capabilities);

/**
 * @brief Set supported features of BIP responder
 *
 * Set the supported features bitmask of peer BIP responder to local BIP initiator.
 *
 * @param bip BIP initiator instance
 * @param features Bitmask of supported features. It is discovered from BIP responder
 *                 by SDP discovery.
 *
 * @return 0 on success, negative error code on failure
 *
 * @note This function should be called before establishing any BIP OBEX connections.
 * @note The features value is typically obtained through SDP discovery.
 */
int bt_bip_set_supported_features(struct bt_bip *bip, uint16_t features);

/**
 * @brief Set supported functions of BIP responder
 *
 * Set the supported functions bitmask of peer BIP responder to local BIP initiator.
 *
 * @param bip BIP initiator instance
 * @param functions Bitmask of supported functions. It is discovered from BIP responder
 *                  by SDP discovery.
 *
 * @return 0 on success, negative error code on failure
 *
 * @note This function should be called before establishing any BIP OBEX connections.
 * @note The functions value is typically obtained through SDP discovery.
 */
int bt_bip_set_supported_functions(struct bt_bip *bip, uint32_t functions);

/**
 * @brief BIP connection state
 *
 * Defines the state of a BIP OBEX connection
 */
enum __packed bt_bip_state {
	/** @brief Connection is disconnected */
	BT_BIP_STATE_DISCONNECTED = 0,
	/** @brief Connection is being established */
	BT_BIP_STATE_CONNECTING = 1,
	/** @brief Connection is established */
	BT_BIP_STATE_CONNECTED = 2,
	/** @brief Connection is being terminated */
	BT_BIP_STATE_DISCONNECTING = 3,
};

/** @brief Forward declaration of BIP server structure */
struct bt_bip_server;

/**
 * @brief BIP server callback structure
 *
 * Callback functions for handling BIP server operations
 */
struct bt_bip_server_cb {
	/**
	 * @brief OBEX Connect request received
	 *
	 * @param server BIP server instance
	 * @param version OBEX protocol version
	 * @param mopl Maximum OBEX packet length
	 * @param buf Request data buffer
	 */
	void (*connect)(struct bt_bip_server *server, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/**
	 * @brief OBEX Disconnect request received
	 *
	 * @param server BIP server instance
	 * @param buf Request data buffer
	 */
	void (*disconnect)(struct bt_bip_server *server, struct net_buf *buf);

	/**
	 * @brief OBEX Abort request received
	 *
	 * @param server BIP server instance
	 * @param buf Request data buffer
	 */
	void (*abort)(struct bt_bip_server *server, struct net_buf *buf);

	/**
	 * @brief Get Capabilities request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_caps)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Get Image List request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_image_list)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Get Image Properties request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_image_properties)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Get Image request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_image)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Get Linked Thumbnail request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_linked_thumbnail)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Get Linked Attachment request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_linked_attachment)(struct bt_bip_server *server, bool final,
				      struct net_buf *buf);

	/**
	 * @brief Get Partial Image request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_partial_image)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Get Monitoring Image request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_monitoring_image)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Get Status request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*get_status)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Put Image request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*put_image)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Put Linked Thumbnail request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*put_linked_thumbnail)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Put Linked Attachment request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*put_linked_attachment)(struct bt_bip_server *server, bool final,
				      struct net_buf *buf);

	/**
	 * @brief Remote Display request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*remote_display)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Delete Image request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*delete_image)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Start Print request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*start_print)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Start Archive request received
	 *
	 * @param server BIP server instance
	 * @param final True if this is the final packet
	 * @param buf Request data buffer
	 */
	void (*start_archive)(struct bt_bip_server *server, bool final, struct net_buf *buf);
};

/** @brief Forward declaration of BIP client structure */
struct bt_bip_client;

/**
 * @brief BIP client callback structure
 *
 * Callback functions for handling BIP client operation responses
 */
struct bt_bip_client_cb {
	/**
	 * @brief OBEX Connect response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param version OBEX protocol version
	 * @param mopl Maximum OBEX packet length
	 * @param buf Response data buffer
	 */
	void (*connect)(struct bt_bip_client *client, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/**
	 * @brief OBEX Disconnect response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*disconnect)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief OBEX Abort response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*abort)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Get Capabilities response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_caps)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Get Image List response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_image_list)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Get Image Properties response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_image_properties)(struct bt_bip_client *client, uint8_t rsp_code,
				     struct net_buf *buf);

	/**
	 * @brief Get Image response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_image)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Get Linked Thumbnail response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_linked_thumbnail)(struct bt_bip_client *client, uint8_t rsp_code,
				     struct net_buf *buf);

	/**
	 * @brief Get Linked Attachment response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_linked_attachment)(struct bt_bip_client *client, uint8_t rsp_code,
				      struct net_buf *buf);

	/**
	 * @brief Get Partial Image response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_partial_image)(struct bt_bip_client *client, uint8_t rsp_code,
				  struct net_buf *buf);

	/**
	 * @brief Get Monitoring Image response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_monitoring_image)(struct bt_bip_client *client, uint8_t rsp_code,
				     struct net_buf *buf);

	/**
	 * @brief Get Status response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*get_status)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Put Image response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*put_image)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Put Linked Thumbnail response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*put_linked_thumbnail)(struct bt_bip_client *client, uint8_t rsp_code,
				     struct net_buf *buf);

	/**
	 * @brief Put Linked Attachment response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*put_linked_attachment)(struct bt_bip_client *client, uint8_t rsp_code,
				      struct net_buf *buf);

	/**
	 * @brief Remote Display response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*remote_display)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Delete Image response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*delete_image)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Start Print response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*start_print)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Start Archive response received
	 *
	 * @param client BIP client instance
	 * @param rsp_code OBEX response code
	 * @param buf Response data buffer
	 */
	void (*start_archive)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);
};

/**
 * @brief BIP server instance structure
 *
 * Structure representing a BIP server endpoint
 */
struct bt_bip_server {
	/** @internal Server callback functions */
	const struct bt_bip_server_cb *_cb;

	/** @internal Parent BIP instance */
	struct bt_bip *_bip;

	/** @internal Service UUID */
	const struct bt_uuid_128 *_uuid;

	/** @internal Connection ID */
	uint32_t _conn_id;

	/** @internal Connection type */
	enum bt_bip_conn_type _type;

	/** @internal Underlying OBEX server */
	struct bt_obex_server _server;

	/** @internal Current operation code */
	uint8_t _opcode;
	/** @internal Current operation type string */
	const char *_optype;

	/** @internal Server state (atomic) */
	atomic_t _state;

	/** @internal Associated client (for secondary connections) */
	union {
		struct bt_bip_client *_primary_client;
		struct bt_bip_client *_secondary_client;
	};

	/** @internal Pending request callback */
	void (*_req_cb)(struct bt_bip_server *server, bool final, struct net_buf *buf);

	/** @internal Node for linking in lists */
	sys_snode_t _node;
};

/**
 * @brief BIP client instance structure
 *
 * Structure representing a BIP client endpoint
 */
struct bt_bip_client {
	/** @internal Client callback functions */
	const struct bt_bip_client_cb *_cb;

	/** @internal Parent BIP instance */
	struct bt_bip *_bip;

	/** @internal Target service UUID */
	struct bt_uuid_128 _uuid;

	/** @internal Connection ID */
	uint32_t _conn_id;

	/** @internal Connection type */
	enum bt_bip_conn_type _type;

	/** @internal Underlying OBEX client */
	struct bt_obex_client _client;

	/** @internal Client state (atomic) */
	atomic_t _state;

	/** @internal Associated server (for secondary connections) */
	union {
		struct bt_bip_server *_primary_server;
		struct bt_bip_server *_secondary_server;
	};

	/** @internal Pending response callback */
	void (*_rsp_cb)(struct bt_bip_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @internal current request function type */
	const char *_req_type;

	/** @internal Node for linking in lists */
	sys_snode_t _node;
};

/**
 * @brief Primary Image Push server callback initializer
 *
 * Helper macro to initialize callbacks for Primary Image Push server
 */
#define BT_BIP_PRIM_IMAGE_PUSH_CB(_connect, _disconnect, _get_caps, _put_image,                    \
				  _put_linked_thumbnail, _put_linked_attachment)                   \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect, .get_caps = _get_caps,             \
		.put_image = _put_image, .put_linked_thumbnail = _put_linked_thumbnail,            \
		.put_linked_attachment = _put_linked_attachment,                                   \
	}

/**
 * @brief Primary Image Pull server callback initializer
 *
 * Helper macro to initialize callbacks for Primary Image Pull server
 */
#define BT_BIP_PRIM_IMAGE_PULL_CB(_connect, _disconnect, _get_caps, _get_image_list,               \
				  _get_image_properties, _get_image, _get_linked_thumbnail,        \
				  _get_linked_attachment, _delete_image)                           \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect, .get_caps = _get_caps,             \
		.get_image_list = _get_image_list, .get_image_properties = _get_image_properties,  \
		.get_image = _get_image, .get_linked_thumbnail = _get_linked_thumbnail,            \
		.get_linked_attachment = _get_linked_attachment, .delete_image = _delete_image,    \
	}

/**
 * @brief Primary Image Print server callback initializer
 *
 * Helper macro to initialize callbacks for Primary Image Print server
 */
#define BT_BIP_PRIM_IMAGE_PRINT_CB(_connect, _disconnect, _get_caps, _get_status, _start_print)    \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect, .get_caps = _get_caps,             \
		.get_status = _get_status, .start_print = _start_print,                            \
	}

/**
 * @brief Secondary Image Print server callback initializer
 *
 * Helper macro to initialize callbacks for Secondary Image Print server
 */
#define BT_BIP_2ND_IMAGE_PRINT_CB(_connect, _disconnect, _get_partial_image)                       \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect,                                    \
		.get_partial_image = _get_partial_image,                                           \
	}

/**
 * @brief Primary Auto Archive server callback initializer
 *
 * Helper macro to initialize callbacks for Primary Auto Archive server
 */
#define BT_BIP_PRIM_AUTO_ARCHIVE_CB(_connect, _disconnect, _get_status, _start_archive)            \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect, .get_status = _get_status,         \
		.start_archive = _start_archive,                                                   \
	}

/**
 * @brief Secondary Auto Archive server callback initializer
 *
 * Helper macro to initialize callbacks for Secondary Auto Archive server
 */
#define BT_BIP_2ND_AUTO_ARCHIVE_CB(_connect, _disconnect, _get_caps, _get_image_list,              \
				   _get_image_properties, _get_image, _get_linked_thumbnail,       \
				   _get_linked_attachment, _delete_image)                          \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect, .get_caps = _get_caps,             \
		.get_image_list = _get_image_list, .get_image_properties = _get_image_properties,  \
		.get_image = _get_image, .get_linked_thumbnail = _get_linked_thumbnail,            \
		.get_linked_attachment = _get_linked_attachment, .delete_image = _delete_image,    \
	}

/**
 * @brief Primary Remote Camera server callback initializer
 *
 * Helper macro to initialize callbacks for Primary Remote Camera server
 */
#define BT_BIP_PRIM_REMOTE_CAMERA_CB(_connect, _disconnect, _get_image_properties, _get_image,     \
				     _get_linked_thumbnail, _get_monitoring_image)                 \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect,                                    \
		.get_image_properties = _get_image_properties, .get_image = _get_image,            \
		.get_linked_thumbnail = _get_linked_thumbnail,                                     \
		.get_monitoring_image = _get_monitoring_image,                                     \
	}

/**
 * @brief Primary Remote Display server callback initializer
 *
 * Helper macro to initialize callbacks for Primary Remote Display server
 */
#define BT_BIP_PRIM_REMOTE_DISPLAY_CB(_connect, _disconnect, _get_caps, _get_image_list,           \
				      _put_image, _put_linked_thumbnail, _remote_display)          \
	{                                                                                          \
		.connect = _connect, .disconnect = _disconnect, .get_caps = _get_caps,             \
		.get_image_list = _get_image_list, .put_image = _put_image,                        \
		.put_linked_thumbnail = _put_linked_thumbnail, .remote_display = _remote_display,  \
	}

/** @brief BIP Image Push service UUID */
#define BT_BIP_UUID_IMAGE_PUSH                                                                     \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0xE33D9545, 0x8374, 0x4AD7, 0x9EC5, 0xC16BE31EDE8E))

/** @brief BIP Image Pull service UUID */
#define BT_BIP_UUID_IMAGE_PULL                                                                     \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x8EE9B3D0, 0x4608, 0x11D5, 0x841A, 0x0002A5325B4E))

/** @brief BIP Advanced Image Printing service UUID */
#define BT_BIP_UUID_IMAGE_PRINT                                                                    \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x92353350, 0x4608, 0x11D5, 0x841A, 0x0002A5325B4E))

/** @brief BIP Auto Archive service UUID */
#define BT_BIP_UUID_AUTO_ARCHIVE                                                                   \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x940126C0, 0x4608, 0x11D5, 0x841A, 0x0002A5325B4E))

/** @brief BIP Remote Camera service UUID */
#define BT_BIP_UUID_REMOTE_CAMERA                                                                  \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x947E7420, 0x4608, 0x11D5, 0x841A, 0x0002A5325B4E))

/** @brief BIP Remote Display service UUID */
#define BT_BIP_UUID_REMOTE_DISPLAY                                                                 \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x94C7CD20, 0x4608, 0x11D5, 0x841A, 0x0002A5325B4E))

/** @brief BIP Referenced Objects service UUID */
#define BT_BIP_UUID_REFERENCED_OBJ                                                                 \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x8E61F95D, 0x1A79, 0x11D4, 0x8EA4, 0x00805F9B9834))

/** @brief BIP Archived Objects service UUID */
#define BT_BIP_UUID_ARCHIVED_OBJ                                                                   \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x8E61F95E, 0x1A79, 0x11D4, 0x8EA4, 0x00805F9B9834))

/**
 * @brief Register a primary BIP OBEX server
 *
 * Registers a primary BIP server for the specified connection type and service UUID.
 * Primary servers handle the main BIP operations for their respective service types.
 *
 * @param bip BIP instance
 * @param server BIP server structure to register
 * @param type Primary connection type (IMAGE_PUSH, IMAGE_PULL, etc.)
 * @param uuid Service UUID for the server
 * @param cb Callback functions for handling server operations
 * @return 0 on success, negative error code on failure
 */
int bt_bip_primary_server_register(struct bt_bip *bip, struct bt_bip_server *server,
				   enum bt_bip_conn_type type, const struct bt_uuid_128 *uuid,
				   struct bt_bip_server_cb *cb);

/**
 * @brief Register a secondary BIP OBEX server
 *
 * Registers a secondary BIP server that works in conjunction with a primary client.
 * Secondary servers handle operations like referenced objects or archived objects.
 *
 * @param bip BIP instance
 * @param server BIP server structure to register
 * @param type Secondary connection type (REFERENCED_OBJECTS or ARCHIVED_OBJECTS)
 * @param uuid Service UUID for the server
 * @param cb Callback functions for handling server operations
 * @param primary_client Associated primary client
 * @return 0 on success, negative error code on failure
 */
int bt_bip_secondary_server_register(struct bt_bip *bip, struct bt_bip_server *server,
				     enum bt_bip_conn_type type, const struct bt_uuid_128 *uuid,
				     struct bt_bip_server_cb *cb,
				     struct bt_bip_client *primary_client);

/**
 * @brief Register Primary Image Push OBEX server
 *
 * Convenience function to register a Primary Image Push server.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_IMAGE_PUSH)
 * @param cb Server callback functions
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_image_push_server_register(struct bt_bip *bip,
							    struct bt_bip_server *server,
							    const struct bt_uuid_128 *uuid,
							    struct bt_bip_server_cb *cb)
{
	return bt_bip_primary_server_register(bip, server, BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH, uuid,
					      cb);
}

/**
 * @brief Register Primary Image Pull OBEX server
 *
 * Convenience function to register a Primary Image Pull server.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_IMAGE_PULL)
 * @param cb Server callback functions
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_image_pull_server_register(struct bt_bip *bip,
							    struct bt_bip_server *server,
							    const struct bt_uuid_128 *uuid,
							    struct bt_bip_server_cb *cb)
{
	return bt_bip_primary_server_register(bip, server, BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL, uuid,
					      cb);
}

/**
 * @brief Register Primary Advanced Image Printing OBEX server
 *
 * Convenience function to register a Primary Advanced Image Printing server.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_IMAGE_PRINT)
 * @param cb Server callback functions
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_advanced_image_printing_server_register(
	struct bt_bip *bip, struct bt_bip_server *server, const struct bt_uuid_128 *uuid,
	struct bt_bip_server_cb *cb)
{
	return bt_bip_primary_server_register(
		bip, server, BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING, uuid, cb);
}

/**
 * @brief Register Primary Auto Archive OBEX server
 *
 * Convenience function to register a Primary Auto Archive server.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_AUTO_ARCHIVE)
 * @param cb Server callback functions
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_auto_archive_server_register(struct bt_bip *bip,
							      struct bt_bip_server *server,
							      const struct bt_uuid_128 *uuid,
							      struct bt_bip_server_cb *cb)
{
	return bt_bip_primary_server_register(bip, server, BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE, uuid,
					      cb);
}

/**
 * @brief Register Primary Remote Camera OBEX server
 *
 * Convenience function to register a Primary Remote Camera server.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_REMOTE_CAMERA)
 * @param cb Server callback functions
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_remote_camera_server_register(struct bt_bip *bip,
							       struct bt_bip_server *server,
							       const struct bt_uuid_128 *uuid,
							       struct bt_bip_server_cb *cb)
{
	return bt_bip_primary_server_register(bip, server, BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA,
					      uuid, cb);
}

/**
 * @brief Register Primary Remote Display OBEX server
 *
 * Convenience function to register a Primary Remote Display server.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_REMOTE_DISPLAY)
 * @param cb Server callback functions
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_remote_display_server_register(struct bt_bip *bip,
								struct bt_bip_server *server,
								const struct bt_uuid_128 *uuid,
								struct bt_bip_server_cb *cb)
{
	return bt_bip_primary_server_register(bip, server, BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY,
					      uuid, cb);
}

/**
 * @brief Register Secondary Advanced Image Printing OBEX server
 *
 * Convenience function to register a Secondary Advanced Image Printing server.
 * This server handles referenced objects for the printing service.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_REFERENCED_OBJ)
 * @param cb Server callback functions
 * @param primary_client Associated primary client
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_secondary_advanced_image_printing_server_register(
	struct bt_bip *bip, struct bt_bip_server *server, const struct bt_uuid_128 *uuid,
	struct bt_bip_server_cb *cb, struct bt_bip_client *primary_client)
{
	return bt_bip_secondary_server_register(
		bip, server, BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS, uuid, cb, primary_client);
}

/**
 * @brief Register Secondary Auto Archive OBEX server
 *
 * Convenience function to register a Secondary Auto Archive server.
 * This server handles archived objects for the auto archive service.
 *
 * @param bip BIP instance
 * @param server BIP server structure
 * @param uuid Service UUID (typically BT_BIP_UUID_ARCHIVED_OBJ)
 * @param cb Server callback functions
 * @param primary_client Associated primary client
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_secondary_auto_archive_server_register(
	struct bt_bip *bip, struct bt_bip_server *server, const struct bt_uuid_128 *uuid,
	struct bt_bip_server_cb *cb, struct bt_bip_client *primary_client)
{
	return bt_bip_secondary_server_register(bip, server, BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS,
						uuid, cb, primary_client);
}

/**
 * @brief Unregister a BIP OBEX server
 *
 * Unregisters a previously registered BIP server.
 *
 * @note The specific server can only be unregistered if it is has been disconnected.
 *
 * @param server BIP server to unregister
 * @return 0 on success, negative error code on failure
 */
int bt_bip_server_unregister(struct bt_bip_server *server);

/**
 * @brief Connect a primary BIP OBEX client
 *
 * Initiates a primary BIP client connection for the specified service type BT_BIP_PRIM_CONN_TYPE_*
 * of @ref bt_bip_conn_type.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param type Primary connection type
 * @param cb Client callback functions
 * @param buf Connection request data
 * @return 0 on success, negative error code on failure
 */
int bt_bip_primary_client_connect(struct bt_bip *bip, struct bt_bip_client *client,
				  enum bt_bip_conn_type type, struct bt_bip_client_cb *cb,
				  struct net_buf *buf);

/**
 * @brief Connect a secondary BIP OBEX client
 *
 * Initiates a secondary BIP client connection with the specific type BT_BIP_2ND_CONN_TYPE_ of
 * @ref bt_bip_conn_type to the secondary server of BIP initiator that works as primary OBEX client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param type Secondary connection type
 * @param cb Client callback functions
 * @param buf Connection request data
 * @param primary_server Associated primary server
 * @return 0 on success, negative error code on failure
 */
int bt_bip_secondary_client_connect(struct bt_bip *bip, struct bt_bip_client *client,
				    enum bt_bip_conn_type type, struct bt_bip_client_cb *cb,
				    struct net_buf *buf, struct bt_bip_server *primary_server);

/**
 * @brief Connect Primary Image Push OBEX client
 *
 * Convenience function to connect a Primary Image Push client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_image_push_client_connect(struct bt_bip *bip,
							   struct bt_bip_client *client,
							   struct bt_bip_client_cb *cb,
							   struct net_buf *buf)
{
	return bt_bip_primary_client_connect(bip, client, BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH, cb,
					     buf);
}

/**
 * @brief Connect Primary Image Pull OBEX client
 *
 * Convenience function to connect a Primary Image Pull client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_image_pull_client_connect(struct bt_bip *bip,
							   struct bt_bip_client *client,
							   struct bt_bip_client_cb *cb,
							   struct net_buf *buf)
{
	return bt_bip_primary_client_connect(bip, client, BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL, cb,
					     buf);
}

/**
 * @brief Connect Primary Advanced Image Printing OBEX client
 *
 * Convenience function to connect a Primary Advanced Image Printing client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_advanced_image_printing_client_connect(
	struct bt_bip *bip, struct bt_bip_client *client, struct bt_bip_client_cb *cb,
	struct net_buf *buf)
{
	return bt_bip_primary_client_connect(
		bip, client, BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING, cb, buf);
}

/**
 * @brief Connect Primary Auto Archive OBEX client
 *
 * Convenience function to connect a Primary Auto Archive client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_auto_archive_client_connect(struct bt_bip *bip,
							     struct bt_bip_client *client,
							     struct bt_bip_client_cb *cb,
							     struct net_buf *buf)
{
	return bt_bip_primary_client_connect(bip, client, BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE, cb,
					     buf);
}

/**
 * @brief Connect Primary Remote Camera OBEX client
 *
 * Convenience function to connect a Primary Remote Camera client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_remote_camera_client_connect(struct bt_bip *bip,
							      struct bt_bip_client *client,
							      struct bt_bip_client_cb *cb,
							      struct net_buf *buf)
{
	return bt_bip_primary_client_connect(bip, client, BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA, cb,
					     buf);
}

/**
 * @brief Connect Primary Remote Display OBEX client
 *
 * Convenience function to connect a Primary Remote Display client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_primary_remote_display_client_connect(struct bt_bip *bip,
							       struct bt_bip_client *client,
							       struct bt_bip_client_cb *cb,
							       struct net_buf *buf)
{
	return bt_bip_primary_client_connect(bip, client, BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY, cb,
					     buf);
}

/**
 * @brief Connect Secondary Advanced Image Printing OBEX client
 *
 * Convenience function to connect a Secondary Advanced Image Printing client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @param primary_server Associated primary server
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_secondary_advanced_image_printing_client_connect(
	struct bt_bip *bip, struct bt_bip_client *client, struct bt_bip_client_cb *cb,
	struct net_buf *buf, struct bt_bip_server *primary_server)
{
	return bt_bip_secondary_client_connect(bip, client, BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS,
					       cb, buf, primary_server);
}

/**
 * @brief Connect Secondary Auto Archive OBEX client
 *
 * Convenience function to connect a Secondary Auto Archive client.
 *
 * @param bip BIP instance
 * @param client BIP client structure
 * @param cb Client callback functions
 * @param buf Connection request data
 * @param primary_server Associated primary server
 * @return 0 on success, negative error code on failure
 */
static inline int bt_bip_secondary_auto_archive_client_connect(struct bt_bip *bip,
							       struct bt_bip_client *client,
							       struct bt_bip_client_cb *cb,
							       struct net_buf *buf,
							       struct bt_bip_server *primary_server)
{
	return bt_bip_secondary_client_connect(bip, client, BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS,
					       cb, buf, primary_server);
}

/**
 * @brief Send OBEX Connect response
 *
 * Sends a response to an OBEX Connect request received by the server.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_connect_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send OBEX Disconnect request
 *
 * Initiates an OBEX Disconnect from the client side.
 *
 * @param client BIP client instance
 * @param buf Request data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_disconnect(struct bt_bip_client *client, struct net_buf *buf);

/**
 * @brief Send OBEX Disconnect response
 *
 * Sends a response to an OBEX Disconnect request received by the server.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_disconnect_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send OBEX Abort request
 *
 * Initiates an OBEX Abort from the client side to cancel ongoing operation.
 *
 * @param client BIP client instance
 * @param buf Request data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_abort(struct bt_bip_client *client, struct net_buf *buf);

/**
 * @brief Send OBEX Abort response
 *
 * Sends a response to an OBEX Abort request received by the server.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_abort_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Get Capabilities request
 *
 * Requests the imaging capabilities from the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID and @ref BT_OBEX_HEADER_ID_TYPE.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_CAPS.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_capabilities(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Capabilities response
 *
 * Responds to a Get Capabilities request with the server's capabilities.
 * The following OBEX headers should be included for the first response,
 * @ref BT_OBEX_HEADER_ID_BODY or @ref BT_OBEX_HEADER_ID_END_BODY.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (capabilities document)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_capabilities_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				struct net_buf *buf);

/**
 * @brief Send Get Image List request
 *
 * Requests a list of available images from the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, @ref BT_OBEX_HEADER_ID_APP_PARAM,
 * and @ref BT_BIP_HEADER_ID_IMG_DESC.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_IMAGE_LIST.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_RETURNED_HANDLES,
 * @ref BT_BIP_APPL_PARAM_TAG_ID_LIST_START_OFFSET, and
 * @ref BT_BIP_APPL_PARAM_TAG_ID_LATEST_CAPTURED_IMAGES.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_image_list(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Image List response
 *
 * Responds to a Get Image List request with the available image list.
 * The following OBEX headers should be included for the first response,
 * @ref BT_OBEX_HEADER_ID_APP_PARAM and @ref BT_BIP_HEADER_ID_IMG_DESC.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_RETURNED_HANDLES.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (image list document)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_image_list_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Get Image Properties request
 *
 * Requests properties of a specific image from the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, and
 * @ref BT_BIP_HEADER_ID_IMG_HANDLE.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (should contain image handle)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_image_properties(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Image Properties response
 *
 * Responds to a Get Image Properties request with the image properties.
 * The following OBEX headers should be included for the first response,
 * @ref BT_OBEX_HEADER_ID_APP_PARAM, and @ref BT_OBEX_HEADER_ID_BODY,
 * or @ref BT_OBEX_HEADER_ID_END_BODY.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_RETURNED_HANDLES.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (image properties document)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_image_properties_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf);

/**
 * @brief Send Get Image request
 *
 * Requests an image from the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, @ref BT_BIP_HEADER_ID_IMG_HANDLE
 * and @ref BT_BIP_HEADER_ID_IMG_DESC.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_IMAGE.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (should contain image handle and descriptor)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_image(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Image response
 *
 * Responds to a Get Image request with the requested image data.
 * The following OBEX headers should be included for the first response,
 * @ref BT_OBEX_HEADER_ID_LEN, and @ref BT_OBEX_HEADER_ID_BODY,
 * or @ref BT_OBEX_HEADER_ID_END_BODY.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (image data)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_image_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Get Linked Thumbnail request
 *
 * Requests a thumbnail linked to a specific image.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, and
 * @ref BT_BIP_HEADER_ID_IMG_HANDLE.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (should contain image handle)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_linked_thumbnail(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Linked Thumbnail response
 *
 * Responds to a Get Linked Thumbnail request with the thumbnail data.
 * The following OBEX headers should be included for the first response,
 * @ref BT_OBEX_HEADER_ID_BODY, or @ref BT_OBEX_HEADER_ID_END_BODY.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (thumbnail data)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_linked_thumbnail_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf);

/**
 * @brief Send Get Linked Attachment request
 *
 * Requests an attachment linked to a specific image.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, @ref BT_BIP_HEADER_ID_IMG_HANDLE,
 * and @ref BT_OBEX_HEADER_ID_NAME.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_LINKED_ATTACHMENT.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (should contain image handle)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_linked_attachment(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Linked Attachment response
 *
 * Responds to a Get Linked Attachment request with the attachment data.
 * The following OBEX headers should be included for the first response,
 * @ref BT_OBEX_HEADER_ID_BODY, or @ref BT_OBEX_HEADER_ID_END_BODY.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (attachment data)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_linked_attachment_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				     struct net_buf *buf);

/**
 * @brief Send Get Partial Image request
 *
 * Requests a partial image (portion of an image) from the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, @ref BT_OBEX_HEADER_ID_NAME,
 * and @ref BT_OBEX_HEADER_ID_APP_PARAM.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_LEN,
 * and @ref BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_START_OFFSET.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (should contain image handle and range info)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_partial_image(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Partial Image response
 *
 * Responds to a Get Partial Image request with the requested image portion.
 * The following OBEX headers should be included for the first response,
 * @ref BT_OBEX_HEADER_ID_BODY, or @ref BT_OBEX_HEADER_ID_END_BODY.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (partial image data)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_partial_image_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				 struct net_buf *buf);

/**
 * @brief Send Get Monitoring Image request
 *
 * Requests a monitoring image from a remote camera.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE,
 * and @ref BT_OBEX_HEADER_ID_APP_PARAM.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_monitoring_image(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Monitoring Image response
 *
 * Responds to a Get Monitoring Image request with the monitoring image.
 * The following OBEX headers should be included for the first response,
 * @ref BT_BIP_HEADER_ID_IMG_HANDLE and @ref BT_OBEX_HEADER_ID_BODY, or
 * @ref BT_OBEX_HEADER_ID_END_BODY.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (monitoring image data)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_monitoring_image_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf);

/**
 * @brief Send Get Status request
 *
 * Requests status information from the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, and @ref BT_OBEX_HEADER_ID_TYPE.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_GET_STATUS.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_status(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Get Status response
 *
 * Responds to a Get Status request with status information.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (status document)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_get_status_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Put Image request
 *
 * Sends an image to the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, @ref BT_OBEX_HEADER_ID_NAME,
 * and @ref BT_BIP_HEADER_ID_IMG_DESC.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_PUT_IMAGE.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (image data and metadata)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_put_image(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Put Image response
 *
 * Responds to a Put Image request.
 * The following OBEX headers should be included for the first response,
 * @ref BT_BIP_HEADER_ID_IMG_HANDLE.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (may contain image handle)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_put_image_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Put Linked Thumbnail request
 *
 * Sends a thumbnail linked to a specific image.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, and
 * @ref BT_BIP_HEADER_ID_IMG_HANDLE.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_PUT_LINKED_THUMBNAIL.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (thumbnail data and image handle)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_put_linked_thumbnail(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Put Linked Thumbnail response
 *
 * Responds to a Put Linked Thumbnail request.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_put_linked_thumbnail_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf);

/**
 * @brief Send Put Linked Attachment request
 *
 * Sends an attachment linked to a specific image.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, @ref BT_BIP_HEADER_ID_IMG_HANDLE,
 * and @ref BT_BIP_HEADER_ID_IMG_DESC.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_PUT_LINKED_ATTACHMENT.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (attachment data and image handle)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_put_linked_attachment(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Put Linked Attachment response
 *
 * Responds to a Put Linked Attachment request.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_put_linked_attachment_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				     struct net_buf *buf);

/**
 * @brief Send Remote Display request
 *
 * Sends a remote display command to the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, @ref BT_BIP_HEADER_ID_IMG_HANDLE,
 * and @ref BT_OBEX_HEADER_ID_APP_PARAM.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_REMOTE_DISPLAY.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_REMOTE_DISPLAY.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (display command)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_remote_display(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Remote Display response
 *
 * Responds to a Remote Display request.
 * The following OBEX headers should be included for the first response,
 * @ref BT_BIP_HEADER_ID_IMG_HANDLE.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_remote_display_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Delete Image request
 *
 * Requests deletion of a specific image on the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, and
 * @ref BT_BIP_HEADER_ID_IMG_HANDLE.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_DELETE_IMAGE.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (should contain image handle)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_delete_image(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Delete Image response
 *
 * Responds to a Delete Image request.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer
 * @return 0 on success, negative error code on failure
 */
int bt_bip_delete_image_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Start Print request
 *
 * Initiates a print job on the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, and
 * @ref BT_OBEX_HEADER_ID_APP_PARAM.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_START_PRINT.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (print control object)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_start_print(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Start Print response
 *
 * Responds to a Start Print request.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (may contain job ID)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_start_print_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send Start Archive request
 *
 * Initiates an archive operation on the server.
 * The following OBEX headers should be included for the first request,
 * @ref BT_OBEX_HEADER_ID_CONN_ID, @ref BT_OBEX_HEADER_ID_TYPE, and
 * @ref BT_OBEX_HEADER_ID_APP_PARAM.
 * And the value of type header is @ref BT_BIP_HDR_TYPE_START_ARCHIVE.
 * The application parameter should include ID @ref BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG.
 *
 * @param client BIP client instance
 * @param final True if this is the final packet
 * @param buf Request data buffer (archive control object)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_start_archive(struct bt_bip_client *client, bool final, struct net_buf *buf);

/**
 * @brief Send Start Archive response
 *
 * Responds to a Start Archive request.
 *
 * @param server BIP server instance
 * @param rsp_code OBEX response code
 * @param buf Response data buffer (may contain job ID)
 * @return 0 on success, negative error code on failure
 */
int bt_bip_start_archive_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Add Image Descriptor header to buffer
 *
 * Adds an Image Descriptor header to the OBEX packet buffer.
 * The descriptor contains format and encoding information for the image.
 *
 * @param buf Buffer to add header to
 * @param len Length of descriptor data
 * @param desc Pointer to descriptor data
 * @return 0 on success, negative error code on failure
 */
int bt_bip_add_header_image_desc(struct net_buf *buf, uint16_t len, const uint8_t *desc);

/**
 * @brief Add Image Handle header to buffer
 *
 * Adds an Image Handle header to the OBEX packet buffer.
 * The handle is a UTF-16 encoded string that uniquely identifies an image.
 *
 * @param buf Buffer to add header to
 * @param len Length of handle data in bytes
 * @param handle Pointer to UTF-16 encoded handle data
 * @return 0 on success, negative error code on failure
 */
int bt_bip_add_header_image_handle(struct net_buf *buf, uint16_t len, const uint8_t *handle);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_BIP_H_ */
