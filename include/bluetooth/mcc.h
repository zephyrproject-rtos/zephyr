/** @file
 *  @brief Bluetooth Media Control Client interface
 *
 *  Updated to the Media Control Profile specification version d09r01
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCC_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCC_

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <net/buf.h>

/* Todo: Decide placement of headers below, fix */
#include "../subsys/bluetooth/host/audio/media_proxy.h"

#ifdef __cplusplus
extern "C" {
#endif

/**** Callback functions ******************************************************/

/** @brief Callback function for mcc_discover_mcs
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 */
typedef void (*bt_mcc_discover_mcs_cb_t)(struct bt_conn *conn, int err);

/** @brief Callback function for mcc_read_player_name
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param name          Player name, max length is PLAYER_NAME_MAX (mpl.h)
 */
typedef void (*bt_mcc_player_name_read_cb_t)(struct bt_conn *conn, int err,
					     char *name);

#ifdef CONFIG_BT_OTC
/** @brief Callback function for mcc_read_icon_obj_id
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param icon_id       The ID of the Icon Object. This is a UINT48 in a uint64_t
 */
typedef void (*bt_mcc_icon_obj_id_read_cb_t)(struct bt_conn *conn, int err,
					     uint64_t id);
#endif /* CONFIG_BT_OTC */

/** @brief Callback function for mcc_read_icon_url
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param icon_url      The URL of the Icon
 */
typedef void (*bt_mcc_icon_url_read_cb_t)(struct bt_conn *conn, int err,
					  char *icon_url);

/** @brief Callback function for track_changed
 *
 * The track changed characteristic is a special case.  It can not be
 * read or written, it can only be notified.
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 */
typedef void (*bt_mcc_track_changed_ntf_cb_t)(struct bt_conn *conn, int err);


/** @brief Callback function for mcc_read_track_title
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 */
typedef void (*bt_mcc_track_title_read_cb_t)(struct bt_conn *conn, int err,
					     char *title);

/** @brief Callback function for mcc_read_track_dur
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param dur           The duration of the track
 */
typedef void (*bt_mcc_track_dur_read_cb_t)(struct bt_conn *conn, int err,
					   int32_t dur);

/** @brief Callback function for mcc_read_track_position
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param pos           The Track Position
 */
typedef void (*bt_mcc_track_position_read_cb_t)(struct bt_conn *conn, int err,
						int32_t pos);

/** @brief Callback function for mcc_set_track_position
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param pos           The Track Position written (or attempted to write)
 */
typedef void (*bt_mcc_track_position_set_cb_t)(struct bt_conn *conn, int err,
					       int32_t pos);

/** @brief Callback function for mcc_read_playback_speed
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param speed         The Playback Speed
 */
typedef void (*bt_mcc_playback_speed_read_cb_t)(struct bt_conn *conn, int err,
						int8_t speed);

/** @brief Callback function for mcc_set_playback_speed
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param speed         The Playback Speed written (or attempted to write)
 */
typedef void (*bt_mcc_playback_speed_set_cb_t)(struct bt_conn *conn, int err,
					       int8_t speed);

/** @brief Callback function for mcc_read_seeking_speed
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param speed         The Seeking Speed
 */
typedef void (*bt_mcc_seeking_speed_read_cb_t)(struct bt_conn *conn, int err,
					       int8_t speed);

#ifdef CONFIG_BT_OTC
/** @brief Callback function for mcc_segments_obj_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param id            The Track Segments Object ID (UINT48)
 */
typedef void (*bt_mcc_segments_obj_id_read_cb_t)(struct bt_conn *conn,
						 int err, uint64_t id);

/** @brief Callback function for mcc_current_track_obj_id_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param id            The Current Track Object ID (UINT48)
 */
typedef void (*bt_mcc_current_track_obj_id_read_cb_t)(struct bt_conn *conn,
						      int err, uint64_t id);

/** @brief Callback function for mcc_next_track_obj_id_obj_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param id            The Next Track Object ID (UINT48)
 */
typedef void (*bt_mcc_next_track_obj_id_read_cb_t)(struct bt_conn *conn,
						   int err, uint64_t id);

/** @brief Callback function for mcc_current_group_obj_id_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param id            The Current Group Object ID (UINT48)
 */
typedef void (*bt_mcc_current_group_obj_id_read_cb_t)(struct bt_conn *conn,
						      int err, uint64_t id);

/** @brief Callback function for mcc_parent_group_obj_id_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param id            The Parent Group Object ID (UINT48)
 */
typedef void (*bt_mcc_parent_group_obj_id_read_cb_t)(struct bt_conn *conn,
						     int err, uint64_t id);
#endif /* CONFIG_BT_OTC */

/** @brief Callback function for playing_order_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param order         The playback order
 */
typedef void (*bt_mcc_playing_order_read_cb_t)(struct bt_conn *conn, int err,
					       uint8_t speed);

/** @brief Callback function for mcc_set_playing_order
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param order         The Playing Order written (or attempted to write)
 */
typedef void (*bt_mcc_playing_order_set_cb_t)(struct bt_conn *conn, int err,
					      uint8_t order);

/** @brief Callback function for playing_orders_supported_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param order         The playback orders supported (bitmap)
 */
typedef void (*bt_mcc_playing_orders_supported_read_cb_t)(struct bt_conn *conn,
						     int err, uint16_t orders);

/** @brief Callback function for media_state_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param order         The Media State
 */
typedef void (*bt_mcc_media_state_read_cb_t)(struct bt_conn *conn, int err,
					     uint8_t state);

/** @brief Callback function for mcc_set_cp
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param pos           The operation written (or attempted to write)
 */
typedef void (*bt_mcc_cp_set_cb_t)(struct bt_conn *conn, int err,
				   struct mpl_op_t op);

/** @brief Callback function for cp notifications
 *
 * Notifications for opcode writes have a different parameter structure
 * than opcode writes
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param ntf           The operation notification
 */
typedef void (*bt_mcc_cp_ntf_cb_t)(struct bt_conn *conn, int err,
				   struct mpl_op_ntf_t ntf);

/** @brief Callback function for mcc_read_opcodes_supported
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param opcodes       The supported opcodes
 */
typedef void (*bt_mcc_opcodes_supported_read_cb_t)(struct bt_conn *conn,
						   int err, uint32_t opcodes);

#ifdef CONFIG_BT_OTC
/** @brief Callback function for mcc_set_scp
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param pos           The search written (or attempted to write)
 */
typedef void (*bt_mcc_scp_set_cb_t)(struct bt_conn *conn, int err,
				    struct mpl_search_t search);

/** @brief Callback function for scp notifications
 *
 * Notifications for the search control points have a different parameter
 * structure than callbacks for search control point writes
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param ntf           The search control point notification
 */
typedef void (*bt_mcc_scp_ntf_cb_t)(struct bt_conn *conn, int err,
				    uint8_t result_code);

/** @brief Callback function for mcc_search_results_obj_id_read
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param id            The Search Results Object ID (UINT48)
 *
 * Note that the Search Results Object ID value may be zero, in case the
 * characteristic does not exist on the server.  (This will be the case if
 * there has not been a successful search.)
 */
typedef void (*bt_mcc_search_results_obj_id_read_cb_t)(struct bt_conn *conn,
						       int err, uint64_t id);
#endif /* CONFIG_BT_OTC */

/** @brief Callback function for mcc_read_content_control_id
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param ccid          The Content Control ID
 */
typedef void (*bt_mcc_content_control_id_read_cb_t)(struct bt_conn *conn,
						    int err, uint8_t ccid);
#ifdef CONFIG_BT_OTC
/**** Callback functions for the included Object Transfer service *************/

/** @brief Callback function for object selected
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 */
typedef void (*bt_mcc_otc_obj_selected_cb_t)(struct bt_conn *conn, int err);

/** @brief Callback function for object metadata
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, ERRNO on fail
 */
typedef void (*bt_mcc_otc_obj_metadata_cb_t)(struct bt_conn *conn, int err);

/** @brief Callback function for bt_mcc_otc_read_icon_object
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param buf           Buffer containing the object contents
 *
 * If err is EMSGSIZE, the object contents have been truncated.
 */
typedef void (*bt_mcc_otc_read_icon_object_cb_t)(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf);

/** @brief Callback function for bt_mcc_otc_read_track_segments_object
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param buf           Buffer containing the object contents
 *
 * If err is EMSGSIZE, the object contents have been truncated.
 */
typedef void (*bt_mcc_otc_read_track_segments_object_cb_t)(struct bt_conn *conn, int err,
							   struct net_buf_simple *buf);

/** @brief Callback function for bt_mcc_otc_read_current_track_object
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param buf           Buffer containing the object contents
 *
 * If err is EMSGSIZE, the object contents have been truncated.
 */
typedef void (*bt_mcc_otc_read_current_track_object_cb_t)(struct bt_conn *conn, int err,
							  struct net_buf_simple *buf);

/** @brief Callback function for bt_mcc_otc_read_next_track_object
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param buf           Buffer containing the object contents
 *
 * If err is EMSGSIZE, the object contents have been truncated.
 */
typedef void (*bt_mcc_otc_read_next_track_object_cb_t)(struct bt_conn *conn, int err,
						       struct net_buf_simple *buf);

/** @brief Callback function for bt_mcc_otc_read_current_group_object
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param buf           Buffer containing the object contents
 *
 * If err is EMSGSIZE, the object contents have been truncated.
 */
typedef void (*bt_mcc_otc_read_current_group_object_cb_t)(struct bt_conn *conn, int err,
							  struct net_buf_simple *buf);

/** @brief Callback function for bt_mcc_otc_read_parent_group_object
 *
 * @param conn          The connection that was used to initialise MCC
 * @param err           Error value. 0 on success, GATT error or ERRNO on fail
 * @param buf           Buffer containing the object contents
 *
 * If err is EMSGSIZE, the object contents have been truncated.
 */
typedef void (*bt_mcc_otc_read_parent_group_object_cb_t)(struct bt_conn *conn, int err,
							  struct net_buf_simple *buf);

#endif /* CONFIG_BT_OTC */


/** @brief Callback function structure */
struct bt_mcc_cb_t {
	bt_mcc_discover_mcs_cb_t                  discover_mcs;
	bt_mcc_player_name_read_cb_t              player_name_read;
#ifdef CONFIG_BT_OTC
	bt_mcc_icon_obj_id_read_cb_t              icon_obj_id_read;
#endif /* CONFIG_BT_OTC */
	bt_mcc_icon_url_read_cb_t                 icon_url_read;
	bt_mcc_track_changed_ntf_cb_t             track_changed_ntf;
	bt_mcc_track_title_read_cb_t              track_title_read;
	bt_mcc_track_dur_read_cb_t                track_dur_read;
	bt_mcc_track_position_read_cb_t           track_position_read;
	bt_mcc_track_position_set_cb_t            track_position_set;
	bt_mcc_playback_speed_read_cb_t           playback_speed_read;
	bt_mcc_playback_speed_set_cb_t            playback_speed_set;
	bt_mcc_seeking_speed_read_cb_t            seeking_speed_read;
#ifdef CONFIG_BT_OTC
	bt_mcc_segments_obj_id_read_cb_t          segments_obj_id_read;
	bt_mcc_current_track_obj_id_read_cb_t     current_track_obj_id_read;
	bt_mcc_next_track_obj_id_read_cb_t        next_track_obj_id_read;
	bt_mcc_current_group_obj_id_read_cb_t     current_group_obj_id_read;
	bt_mcc_parent_group_obj_id_read_cb_t      parent_group_obj_id_read;
#endif /* CONFIG_BT_OTC */
	bt_mcc_playing_order_read_cb_t	          playing_order_read;
	bt_mcc_playing_order_set_cb_t             playing_order_set;
	bt_mcc_playing_orders_supported_read_cb_t playing_orders_supported_read;
	bt_mcc_media_state_read_cb_t              media_state_read;
	bt_mcc_cp_set_cb_t                        cp_set;
	bt_mcc_cp_ntf_cb_t                        cp_ntf;
	bt_mcc_opcodes_supported_read_cb_t        opcodes_supported_read;
#ifdef CONFIG_BT_OTC
	bt_mcc_scp_set_cb_t                       scp_set;
	bt_mcc_scp_ntf_cb_t                       scp_ntf;
	bt_mcc_search_results_obj_id_read_cb_t    search_results_obj_id_read;
#endif /* CONFIG_BT_OTC */
	bt_mcc_content_control_id_read_cb_t       content_control_id_read;
#ifdef CONFIG_BT_OTC
	bt_mcc_otc_obj_selected_cb_t              otc_obj_selected;
	bt_mcc_otc_obj_metadata_cb_t              otc_obj_metadata;
	bt_mcc_otc_read_icon_object_cb_t          otc_icon_object;
	bt_mcc_otc_read_track_segments_object_cb_t otc_track_segments_object;
	bt_mcc_otc_read_current_track_object_cb_t  otc_current_track_object;
	bt_mcc_otc_read_next_track_object_cb_t     otc_next_track_object;
	bt_mcc_otc_read_current_group_object_cb_t  otc_current_group_object;
	bt_mcc_otc_read_parent_group_object_cb_t   otc_parent_group_object;
#endif /* CONFIG_BT_OTC */
};

/**** Functions ***************************************************************/

/** @brief Initialize Media Control Client
 *
 * @param conn  The connection to initialize for
 * @param cb    Callbacks to be used
 */
int bt_mcc_init(struct bt_conn *conn, struct bt_mcc_cb_t *cb);


/** @brief Discover Media Control Service
 *
 * Discover Media Control Service on the server given by the connection
 * Optionally subscribe to notifications.
 *
 * Shall be called once after initialization and before using other MCC
 * functionality.
 *
 * @param conn        The connection on which to do MCS discovery
 * @param subscribe   Whether to subscribe to notifications
 */
int bt_mcc_discover_mcs(struct bt_conn *conn, bool subscribe);

/** @brief Read Media Player Name */
int bt_mcc_read_player_name(struct bt_conn *conn);

#ifdef CONFIG_BT_OTC
/** @brief Read Icon Object ID */
int bt_mcc_read_icon_obj_id(struct bt_conn *conn);
#endif /* CONFIG_BT_OTC */

/** @brief Read Icon Object URL */
int bt_mcc_read_icon_url(struct bt_conn *conn);

/** @brief Read Track Title */
int bt_mcc_read_track_title(struct bt_conn *conn);

/** @brief Read Track Duration */
int bt_mcc_read_track_dur(struct bt_conn *conn);

/** @brief Read Track Position */
int bt_mcc_read_track_position(struct bt_conn *conn);

/** @brief Set Track position
 *
 * @param conn  Connection to the peer device
 * @param pos   Track position
 *
 * @return int  0 on success, GATT error value on fail.
 */
int bt_mcc_set_track_position(struct bt_conn *conn, int32_t pos);

/** @brief Read Playback speed */
int bt_mcc_read_playback_speed(struct bt_conn *conn);

/** @brief Set Playback Speed
 *
 * @param conn  Connection to the peer device
 * @param speed Playback speed
 *
 * @return int  0 on success, GATT error value on fail.
 */
int bt_mcc_set_playback_speed(struct bt_conn *conn, int8_t speed);

/** @brief Read Seeking speed */
int bt_mcc_read_seeking_speed(struct bt_conn *conn);

#ifdef CONFIG_BT_OTC
/** @brief Read Track Segments Object ID */
int bt_mcc_read_segments_obj_id(struct bt_conn *conn);

/** @brief Read Current Track Object ID */
int bt_mcc_read_current_track_obj_id(struct bt_conn *conn);

/** @brief Read Next Track Object ID */
int bt_mcc_read_next_track_obj_id(struct bt_conn *conn);

/** @brief Read Current Group Object ID */
int bt_mcc_read_current_group_obj_id(struct bt_conn *conn);

/** @brief Read Parent Group Object ID */
int bt_mcc_read_parent_group_obj_id(struct bt_conn *conn);
#endif /* CONFIG_BT_OTC */

/** @brief Read Playing Order */
int bt_mcc_read_playing_order(struct bt_conn *conn);

/** @brief Set Playing Order
 *
 * @param conn  Connection to the peer device
 * @param order Playing order
 *
 * @return int  0 on success, GATT error value on fail.
 */
int bt_mcc_set_playing_order(struct bt_conn *conn, uint8_t order);

/** @brief Read Playing Orders Supported */
int bt_mcc_read_playing_orders_supported(struct bt_conn *conn);

/** @brief Read Media State */
int bt_mcc_read_media_state(struct bt_conn *conn);

/** @brief Set Control Point
 *
 * Write a command (e.g. "play", "pause") to the control point.
 *
 * @param conn  Connection to the peer device
 * @param op    Media operation
 *
 * @return int  0 on success, GATT error value on fail.
 */
int bt_mcc_set_cp(struct bt_conn *conn, struct mpl_op_t op);

/** @brief Read Opcodes Supported */
int bt_mcc_read_opcodes_supported(struct bt_conn *conn);

#ifdef CONFIG_BT_OTC
/** @brief Set Search Control Point
 *
 * Write a search to the search control point.
 *
 * @param conn   Connection to the peer device
 * @param search The search
 *
 * @return int  0 on success, GATT error value on fail.
 */
int bt_mcc_set_scp(struct bt_conn *conn, struct mpl_search_t search);

/** @brief Search Results Group Object ID */
int bt_mcc_read_search_results_obj_id(struct bt_conn *conn);
#endif /* CONFIG_BT_OTC */

/** @brief Read Content Control ID */
int bt_mcc_read_content_control_id(struct bt_conn *conn);

#ifdef CONFIG_BT_OTC
/** @brief Read the current object metadata */
int bt_mcc_otc_read_object_metadata(struct bt_conn *conn);

/** @brief Read the Icon Object */
int bt_mcc_otc_read_icon_object(struct bt_conn *conn);

/** @brief Read the Track Segments Object */
int bt_mcc_otc_read_track_segments_object(struct bt_conn *conn);

/** @brief Read the Current Track Object */
int bt_mcc_otc_read_current_track_object(struct bt_conn *conn);

/** @brief Read the Next Track Object */
int bt_mcc_otc_read_next_track_object(struct bt_conn *conn);

/** @brief Read the Current Group Object */
int bt_mcc_otc_read_current_group_object(struct bt_conn *conn);

/** @brief Read the Parent Group Object */
int bt_mcc_otc_read_parent_group_object(struct bt_conn *conn);

#if defined(CONFIG_BT_MCC_SHELL)
struct bt_otc_instance_t *bt_mcc_otc_inst(void);
#endif /* defined(CONFIG_BT_MCC_SHELL) */
#endif /* CONFIG_BT_OTC */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCC__ */
