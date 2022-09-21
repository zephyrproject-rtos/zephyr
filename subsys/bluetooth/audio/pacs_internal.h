/* @file
 * @brief Internal APIs for PACS handling
 *
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/audio.h>

#define BT_AUDIO_LOCATION_MASK BIT_MASK(28)

struct bt_pac_codec {
	uint8_t  id;			/* Codec ID */
	uint16_t cid;			/* Company ID */
	uint16_t vid;			/* Vendor specific Codec ID */
} __packed;

/* TODO: Figure out the capabilities types */
#define BT_CODEC_CAP_PARAMS		0x01
#define BT_CODEC_CAP_DRM		0x0a
#define BT_CODEC_CAP_DRM_VALUE		0x0b

struct bt_pac_codec_capability {
	uint8_t  len;			/* Codec Capability length */
	uint8_t  type;			/* Codec Capability type */
	uint8_t  data[0];		/* Codec Capability data */
} __packed;

struct bt_pac_meta {
	uint8_t  len;			/* Metadata Length */
	uint8_t  value[0];		/* Metadata Value */
} __packed;

struct bt_pac {
	struct bt_pac_codec codec;	/* Codec ID */
	uint8_t  cc_len;		/* Codec Capabilities Length */
	struct bt_pac_codec_capability cc[0]; /* Codec Specific Capabilities */
	struct bt_pac_meta meta[0];	/* Metadata */
} __packed;

struct bt_pacs_read_rsp {
	uint8_t  num_pac;		/* Number of PAC Records*/
	struct bt_pac pac[0];
} __packed;

struct bt_pacs_context {
	uint16_t  snk;
	uint16_t  src;
} __packed;

/**  @brief Callback structure for the Public Audio Capabilities Service (PACS)
 *
 * This is used for the Unicast Server
 * (@kconfig{CONFIG_BT_AUDIO_UNICAST_SERVER}) and Broadcast Sink
 * (@kconfig{CONFIG_BT_AUDIO_BROADCAST_SINK}) roles.
 */
struct bt_audio_pacs_cb {
	/** @brief Get available audio contexts callback
	 *
	 *  Get available audio contexts callback is called whenever a remote client
	 *  requests to read the value of Published Audio Capabilities (PAC) Available
	 *  Audio Contexts, or if the value needs to be notified.
	 *
	 *  @param[in]  conn     The connection that requests the available audio
	 *                       contexts. Will be NULL if requested for sending
	 *                       a notification, as a result of calling
	 *                       bt_pacs_available_contexts_changed().
	 *  @param[in]  dir      Direction of the endpoint.
	 *  @param[out] context  Pointer to the contexts that needs to be set.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*get_available_contexts)(struct bt_conn *conn, enum bt_audio_dir dir,
				      enum bt_audio_context *context);

	/** @brief Publish Capability callback
	 *
	 *  Publish Capability callback is called whenever a remote client
	 *  requests to read the Published Audio Capabilities (PAC) records.
	 *  The callback will be called iteratively until it returns an error,
	 *  increasing the @p index each time. Once an error value (non-zero)
	 *  is returned, the previously returned @p codec values (if any) will
	 *  be sent to the client that requested the value.
	 *
	 *  @param conn   The connection that requests the capabilities.
	 *                Will be NULL if the capabilities is requested for
	 *                sending a notification, as a result of calling
	 *                bt_audio_capability_register() or
	 *                bt_audio_capability_unregister().
	 *  @param type   Type of the endpoint.
	 *  @param index  Index of the codec object requested. Multiple objects
	 *                may be returned, and this value keep tracks of how
	 *                many have previously been returned.
	 *  @param codec  Codec object that shall be populated if returning
	 *                success (0). Ignored if returning non-zero.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*publish_capability)(struct bt_conn *conn, uint8_t type,
				  uint8_t index, struct bt_codec *const codec);

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
	/** @brief Publish location callback
	 *
	 *  Publish location callback is called whenever a remote client
	 *  requests to read the Published Audio Capabilities (PAC) location,
	 *  or if the location needs to be notified.
	 *
	 *  @param[in]  conn      The connection that requests the location.
	 *                        Will be NULL if the location is requested
	 *                        for sending a notification, as a result of
	 *                        calling bt_audio_pacs_location_changed().
	 *  @param[in]  dir       Direction of the endpoint.
	 *  @param[out] location  Pointer to the location that needs to be set.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*publish_location)(struct bt_conn *conn,
				enum bt_audio_dir dir,
				enum bt_audio_location *location);

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	/** @brief Write location callback
	 *
	 *  Write location callback is called whenever a remote client
	 *  requests to write the Published Audio Capabilities (PAC) location.
	 *
	 *  @param conn      The connection that requests the write.
	 *  @param dir       Direction of the endpoint.
	 *  @param location  The location being written.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*write_location)(struct bt_conn *conn, enum bt_audio_dir dir,
			      enum bt_audio_location location);
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */
};

int bt_audio_pacs_register_cb(const struct bt_audio_pacs_cb *cb);
int bt_audio_pacs_location_changed(enum bt_audio_dir dir);
void bt_pacs_capabilities_changed(enum bt_audio_dir dir);
int bt_pacs_available_contexts_changed(void);
bool bt_pacs_context_available(enum bt_audio_dir dir, uint16_t context);
