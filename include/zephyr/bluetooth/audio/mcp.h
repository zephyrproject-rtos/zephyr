/**
 * @file
 * @brief Bluetooth Media Control Profile (MCP) APIs.
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCP_H_

/**
 * @brief Media Control Profile (MCP)
 *
 * @defgroup bt_mcp Media Control Profile (MCP)
 *
 * @since 4.3
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Media Control Profile (MCP) provides procedures control media.
 * It provides the Media Control Client and the Media Control Server roles,
 * where the former is usually placed on resource constrained devices like headphones,
 * and the latter placed on more powerful devices like phones and PCs.
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/mcs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Special value indicating that the object ID is not set */
#define BT_MCP_NO_TRACK_ID 0

/**
 * @defgroup bt_mcp_media_control_server MCP Media Control Server APIs
 * @ingroup bt_mcp
 * @{
 */

/**
 * @brief Register the Media Control Server
 *
 * This will register the Generic Media Control Service (GMCS).
 *
 * @retval 0 Success
 * @retval -EBUSY Registration already in progress
 * @retval -EALREADY Already registered
 * @retval -ENOMEM Could not allocate any more content control services
 */
int bt_mcp_media_control_server_register(void);

/**
 * @brief Read Media Player Name
 *
 * @param[out] name Buffer to store the name in
 * @param name_size Size of the @p name buffer
 *
 * @retval 0 Success
 * @retval -EINVAL @p name was NULL
 * @retval -ENOMEM @p name_size was lower than the current player name
 */
int bt_mcp_media_control_server_get_player_name(char *name, size_t name_size);

/**
 * @brief Read Icon Object ID
 *
 * Get an ID (48 bit) that can be used to retrieve the Icon
 * Object from an Object Transfer Service
 *
 * See the Media Control Service spec v1.0 sections 3.2 and
 * 4.1 for a description of the Icon Object.
 *
 * Requires Object Transfer Service
 *
 * @param[out] id Variable to store the resulting ID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was NULL
 */
int bt_mcp_media_control_server_get_icon_id(uint64_t *id);

/**
 * @brief Read Icon URL
 *
 * Get a URL to the media player's icon.
 *
 * @param[out] url Buffer to store the URL in
 * @param url_size Size of the @p url buffer
 *
 * @retval 0 Success
 * @retval -EINVAL @p url was NULL
 * @retval -ENOMEM @p url_size was lower than the icon URL
 */
int bt_mcp_media_control_server_get_icon_url(char *url, size_t url_size);

/**
 * @brief Read Track Title
 *
 * @param[out] track_title Buffer to store the URL in
 * @param track_title_size Size of the @p track_title buffer
 *
 * @retval 0 Success
 * @retval -EINVAL @p track_title was NULL
 * @retval -ENOMEM @p track_title_size was lower than the track_title
 */
int bt_mcp_media_control_server_get_track_title(char *track_title, size_t track_title_size);

/**
 * @brief Read Track Duration
 *
 * The duration of a track is measured in hundredths of a
 * second.
 *
 * @param[out] duration Variable to store the resulting duration in
 *
 * @retval 0 Success
 * @retval -EINVAL @p duration was NULL
 */
int bt_mcp_media_control_server_get_track_duration(int32_t *duration);

/**
 * @brief Read Track Position
 *
 * The position of the player (the playing position) is
 * measured in hundredths of a second from the beginning of
 * the track
 *
 * @param[out] position Variable to store the resulting position in
 *
 * @retval 0 Success
 * @retval -EINVAL @p position was NULL
 */
int bt_mcp_media_control_server_get_track_position(int32_t *position);

/**
 * @brief Set Track Position
 *
 * Set the playing position of the media player in the current
 * track. The position is given in hundredths of a second,
 * from the beginning of the track of the track for positive
 * values, and (backwards) from the end of the track for
 * negative values.
 *
 * @param position   The track position to set
 *
 * @retval 0 Success
 * @retval -ENODEV The player has not current group or track
 */
int bt_mcp_media_control_server_set_track_position(int32_t position);

/**
 * @brief Get Playback Speed
 *
 * The playback speed parameter is related to the actual
 * playback speed as follows:
 * actual playback speed = 2^(speed_parameter/64)
 *
 * A speed parameter of 0 corresponds to unity speed playback
 * (i.e. playback at "normal" speed). A speed parameter of
 * -128 corresponds to playback at one fourth of normal speed,
 * 127 corresponds to playback at almost four times the normal
 * speed.
 *
 * @param[out] speed Variable to store the resulting speed in
 *
 * @retval 0 Success
 * @retval -EINVAL @p speed was NULL
 */
int bt_mcp_media_control_server_get_playback_speed(int8_t *speed);

/**
 * @brief Set Playback Speed
 *
 * See the get_playback_speed() function for an explanation of
 * the playback speed parameter.
 *
 * Note that the media player may not support all possible
 * values of the playback speed parameter. If the value given
 * is not supported, and is higher than the current value, the
 * player should set the playback speed to the next higher
 * supported value. (And correspondingly to the next lower
 * supported value for given values lower than the current
 * value.)
 *
 * @param speed The playback speed parameter to set
 *
 * @retval 0 Success
 */
int bt_mcp_media_control_server_set_playback_speed(int8_t speed);

/**
 * @brief Get Seeking Speed
 *
 * The seeking speed gives the speed with which the player is
 * seeking. It is a factor, relative to real-time playback
 * speed - a factor four means seeking happens at four times
 * the real-time playback speed. Positive values are for
 * forward seeking, negative values for backwards seeking.
 *
 * The seeking speed is not settable - a non-zero seeking speed
 * is the result of "fast rewind" of "fast forward" commands.
 *
 * @param[out] speed Variable to store the resulting speed in
 *
 * @retval 0 Success
 * @retval -EINVAL @p speed was NULL
 */
int bt_mcp_media_control_server_get_seeking_speed(int8_t *speed);

/**
 * @brief Read Current Track Segments Object ID
 *
 * Get an ID (48 bit) that can be used to retrieve the Current
 * Track Segments Object from an Object Transfer Service
 *
 * See the Media Control Service spec v1.0 sections 3.10 and
 * 4.2 for a description of the Track Segments Object.
 *
 * Requires Object Transfer Service
 *
 * @param[out] id Variable to store the resulting ID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was NULL
 * @retval -ENODEV The player has not current group or track
 */
int bt_mcp_media_control_server_get_track_segments_id(uint64_t *id);

/**
 * @brief Read Current Track Object ID
 *
 * Get an ID (48 bit) that can be used to retrieve the Current
 * Track Object from an Object Transfer Service
 *
 * See the Media Control Service spec v1.0 sections 3.11 and
 * 4.3 for a description of the Current Track Object.
 *
 * Requires Object Transfer Service
 *
 * @param[out] id Variable to store the resulting ID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was NULL
 * @retval -ENODEV The player has not current group or track
 */
int bt_mcp_media_control_server_get_current_track_id(uint64_t *id);

/**
 * @brief Set Current Track Object ID
 *
 * Change the player's current track to the track given by the ID.
 * (Behaves similarly to the goto track command.)
 *
 * Requires Object Transfer Service
 *
 * @param id The id parameter to set (between @ref BT_OTS_OBJ_ID_MIN and @ref BT_OTS_OBJ_ID_MAX).
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was invalid
 */
int bt_mcp_media_control_server_set_current_track_id(uint64_t id);

/**
 * @brief Read Next Track Object ID
 *
 * Get an ID (48 bit) that can be used to retrieve the Next
 * Track Object from an Object Transfer Service
 *
 * Requires Object Transfer Service
 *
 * @param[out] id Variable to store the resulting ID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was NULL
 * @retval -ENODEV The player has not current group or track
 */
int bt_mcp_media_control_server_get_next_track_id(uint64_t *id);

/**
 * @brief Set Next Track Object ID
 *
 * Change the player's next track to the track given by the ID.
 *
 * Requires Object Transfer Service
 *
 * @param id The id parameter to set (between @ref BT_OTS_OBJ_ID_MIN and @ref BT_OTS_OBJ_ID_MAX).
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was invalid
 */
int bt_mcp_media_control_server_set_next_track_id(uint64_t id);

/**
 * @brief Read Parent Group Object ID
 *
 * Get an ID (48 bit) that can be used to retrieve the Parent
 * Track Object from an Object Transfer Service
 *
 * The parent group is the parent of the current group.
 *
 * See the Media Control Service spec v1.0 sections 3.14 and
 * 4.4 for a description of the Current Track Object.
 *
 * Requires Object Transfer Service
 *
 * @param[out] id Variable to store the resulting ID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was NULL
 * @retval -ENODEV The player has not current group or track
 */
int bt_mcp_media_control_server_get_parent_group_id(uint64_t *id);

/**
 * @brief Read Current Group Object ID
 *
 * Get an ID (48 bit) that can be used to retrieve the Current
 * Track Object from an Object Transfer Service
 *
 * See the Media Control Service spec v1.0 sections 3.14 and
 * 4.4 for a description of the Current Group Object.
 *
 * Requires Object Transfer Service
 *
 * @param[out] id Variable to store the resulting ID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was NULL
 * @retval -ENODEV The player has not current group
 */
int bt_mcp_media_control_server_get_current_group_id(uint64_t *id);

/**
 * @brief Set Current Group Object ID
 *
 * Change the player's current group to the group given by the
 * ID, and the current track to the first track in that group.
 *
 * Requires Object Transfer Service

 * @param id The id parameter to set (between @ref BT_OTS_OBJ_ID_MIN and @ref BT_OTS_OBJ_ID_MAX).
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was invalid
 * @retval -ENODEV The player has not current group
 */
int bt_mcp_media_control_server_set_current_group_id(uint64_t id);

/**
 * @brief Read Playing Order
 *
 * @param[out] order Variable to store the resulting playing order in
 *
 * @retval 0 Success
 * @retval -EINVAL @p order was NULL
 */
int bt_mcp_media_control_server_get_playing_order(uint8_t *order);

/**
 * @brief Set Playing Order
 *
 * Set the media player's playing order
 *
 * @param order The playing order to set
 *
 * @retval 0 Success
 */
int bt_mcp_media_control_server_set_playing_order(uint8_t order);

/**
 * @brief Read Playing Orders Supported
 *
 * Read a bitmap containing the media player's supported
 * playing orders.
 *
 * @param[out] orders Variable to store the resulting playing orders supported in
 *
 * @retval 0 Success
 * @retval -EINVAL @p orders was NULL
 */
int bt_mcp_media_control_server_get_playing_orders_supported(uint16_t *orders);

/**
 * @brief Read Media State
 *
 * Read the media player's state
 *
 * @param[out] state Variable to store the resulting state in
 *
 * @retval 0 Success
 * @retval -EINVAL @p state was NULL
 */
int bt_mcp_media_control_server_get_media_state(uint8_t *state);

/**
 * @brief Execute Command
 *
 * Execute a command in the Media Control Server.
 * Commands may cause the media player to change its state
 *
 * @param command     The command to execute
 *
 * @retval 0 Success
 * @retval -EINVAL @p command was NULL
 */
int bt_mcp_media_control_server_command(const struct bt_mcs_cmd *command);

/**
 * @brief Read Commands Supported
 *
 * Read a bitmap containing the media player's supported
 * command opcodes.
 *
 * @param[out] opcodes Variable to store the resulting supported opcodes in
 *
 * @retval 0 Success
 * @retval -EINVAL @p opcodes was NULL
 */
int bt_mcp_media_control_server_get_commands_supported(uint32_t *opcodes);

/**
 * @brief Set Search
 *
 * Write a search to the media player.
 * If the search is successful, the search results will be available as a group object
 * in the Object Transfer Service (OTS).
 *
 * May result in up to three callbacks
 * - one for the actual sending of the search to the player
 * - one for the result code for the search from the player
 * - if the search is successful, one for the search results object ID in the OTs
 *
 * Requires Object Transfer Service
 *
 * @param search     The search command to execute
 *
 * @retval 0 Success
 * @retval -EINVAL @p search was NULL
 */
int bt_mcp_media_control_server_search_command(const struct bt_mcp_search *search);

/**
 * @brief Read Search Results Object ID
 *
 * Get an ID (48 bit) that can be used to retrieve the Search
 * Results Object from an Object Transfer Service
 *
 * The search results object is a group object.
 * The search results object only exists if a successful
 * search operation has been done.
 *
 * Requires Object Transfer Service
 *
 * @param[out] id Variable to store the resulting ID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p id was NULL
 */
int bt_mcp_media_control_server_get_search_results_id(uint64_t *id);

/**
 * @brief Read Content Control ID
 *
 * The content control ID identifies a content control service
 * on a device, and links it to the corresponding audio
 * stream.
 *
 * @param[out] ccid Variable to store the resulting CCID in
 *
 * @retval 0 Success
 * @retval -EINVAL @p ccid was NULL
 */
uint8_t bt_mcp_media_control_server_get_ccid(uint8_t *ccid);

/** @} */ /* End of group bt_mcp_media_control_server */
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCP_H_ */
