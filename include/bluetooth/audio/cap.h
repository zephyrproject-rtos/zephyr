/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_

/**
 * @brief Common Audio Profile (CAP)
 *
 * @defgroup bt_cap Common Audio Profile (CAP)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <bluetooth/audio/csis.h>
#include <bluetooth/audio/tbs.h>
#include <bluetooth/audio/media_proxy.h>
#include <bluetooth/audio/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Type of CCID records */
enum bt_cap_ccid_record_type {
	/** CCID belongs to a Telephone Bearer Service instance */
	BT_CAP_CCID_TYPE_TELEPHONE_BEARER,
	/** CCID belongs to a Media Control Service instance */
	BT_CAP_CCID_TYPE_MEDIA_PLAYER
};

/** CCID record structure */
struct bt_cap_ccid_record {
	/** The type of the record */
	enum bt_cap_ccid_record_type type;

	union {
		/** Reference to a local or remote Telephone Bearer instance */
		struct bt_tbs *telephone_bearer;
		/** Reference to a local or remote Media Control instance */
		struct media_player *media_player;
	};
};

/** Callback structure for CAP procedures */
struct bt_cap_cb {
	/**
	 * @brief CCID find callback
	 *
	 * Callback for bt_cap_find_ccid_service().
	 * This is called for the first service instance found for the
	 * specified CCID.
	 *
	 * @param conn        Connection object used in
	 *                    bt_cap_find_ccid_service().
	 * @param err         0 if success, else BT_GATT_ERR() with a specific
	 *                    ATT (BT_ATT_ERR_*) error code.
	 * @param ccid        The CCID value used in bt_cap_find_ccid_service().
	 * @param ccid_record A CCID record structure with a reference to the
	 *                    service instance. May be NULL, if not service
	 *                    instance was found. Only valid if @p err is 0.
	 */
	void (*ccid_found)(struct bt_conn *conn, int err, uint8_t ccid,
			   const struct bt_cap_ccid_record *ccid_record);

#if defined(CONFIG_BT_CAP_INITIATOR) && defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
	/**
	 * @brief Callback for bt_cap_unicast_discover()
	 *
	 * @param conn The connection pointer supplied to
	 *             bt_cap_unicast_discover()
	 * @param err  0 if Common Audio Service was found, else BT_GATT_ERR()
	 *             with a specific ATT (BT_ATT_ERR_*) error code.
	 */
	void (*discovery_complete)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback for bt_cap_unicast_audio_start()
	 *
	 * @param unicast_group  The unicast group pointer supplied to
	 *                       bt_cap_unicast_audio_start()
	 * @param err            0 if success, else BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code.
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0.
	 */
	void (*unicast_start_complete)(struct bt_audio_unicast_group *unicast_group,
				       int err, struct bt_conn *conn);

	/**
	 * @brief Callback for bt_cap_unicast_audio_update()
	 *
	 * @param unicast_group  The unicast group pointer supplied to
	 *                       bt_cap_unicast_audio_update()
	 * @param err            0 if success, else BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code.
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0.
	 */
	void (*unicast_update_complete)(struct bt_audio_unicast_group *unicast_group,
					int err, struct bt_conn *conn);

	/**
	 * @brief Callback for bt_cap_unicast_audio_stop()
	 *
	 * @param unicast_group  The unicast group pointer supplied to
	 *                       bt_cap_unicast_audio_stop()
	 * @param err            0 if success, else BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code.
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0.
	 */
	void (*unicast_stop_complete)(struct bt_audio_unicast_group *unicast_group,
				      int err, struct bt_conn *conn);
#endif /* CONFIG_BT_CAP_INITIATOR && CONFIG_BT_AUDIO_UNICAST_CLIENT */
}

/**
 * @brief Find the content control service based on CCID
 *
 * This is a local operation, and requires that the content control services
 * already have been discovered using e.g. bt_tbs_client_discover() or
 * media_proxy_ctrl_discover_player().
 *
 * @param conn Connection to a remote server or NULL to get local instance.
 * @param ccid The CCID to find.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_find_ccid_service_service(struct bt_conn *conn, uint8_t ccid);

/**
 * @brief Discovers audio support on a remote device.
 *
 * This will discover the Common Audio Service (CAS) on the remote device, to
 * verify if the remote device supports the Common Audio Profile.
 *
 * @param conn Connection to a remote server.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_unicast_discover(struct bt_conn *conn);

/** Type of CAP set */
enum bt_cap_set_type {
	/** The set is an ad-hoc set */
	BT_CAP_SET_TYPE_AD_HOC,
	/** The set is a CSIP Coordinated Set */
	BT_CAP_SET_TYPE_CSIP,
};

struct bt_cap_unicast_audio_start_param {
	/** The type of the set */
	enum bt_cap_set_type type;

	/** The number of set members in @p members and number of streams in @p streams */
	uint8_t count;

	union {
		/**
		 * @brief List of connection pointers if @p type is BT_CAP_SET_TYPE_AD_HOC
		 *
		 * The number of pointers in this array shall be at least @p count
		 */
		struct bt_conn **members;

		/** CSIP Coordinated Set struct used if @p type is BT_CAP_SET_TYPE_CSIP */
		struct {
			/**
			 * @brief CSIP Set Members in the set
			 *
			 * The number of pointers in this array shall be at least %p count
			 */
			struct bt_csis_client_set_member **members;
		} csip;
	};

	/** @brief Streams for the @p members
	 *
	 * stream[i] will be associated with members[i] if not already
	 * initialized, else the stream will be verified against the member.
	 */
	struct bt_audio_stream **streams;

	/**
	 * @brief Codec configuration
	 *
	 * The @p codec.meta shall include a list of CCIDs as well as a non-0
	 * context bitfield.
	 */
	const struct bt_codec *codec;

	/** Quality of Service configuration */
	const struct bt_codec_qos *qos;
};

/**
 * @brief Setup and start unicast audio streams for a set of devices
 *
 * The result of this operation is that the streams in @p param will be
 * initialized and will be usable for streaming audio data.
 * The @p unicast_group value can be used to update and stop the streams.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param[in]  param          Parameters to start the audio streams.
 * @param[out] unicast_group  Pointer to the unicast group created.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param,
			       struct bt_audio_unicast_group **unicast_group);

/**
 * @brief Update unicast audio streams for a unicast group
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param unicast_group The group of unicast devices to update.
 * @param meta_count    The number of entries in @p meta.
 * @param meta          The new metadata. The metadata shall a list of CCIDs as
 *                      well as a non-0 context bitfield.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_unicast_audio_update(struct bt_audio_unicast_group *unicast_group,
				uint8_t meta_count,
				const struct bt_codec_data *meta);

/**
 * @brief Stop unicast audio streams for a unicast group
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param unicast_group The group of unicast devices to stop. The audio streams
 *                      in this will be stopped and reset, and the
 *                      @p unicast_group will be invalidated.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_unicast_audio_stop(struct bt_audio_unicast_group *unicast_group);

struct bt_cap_broadcast_audio_start_param {

	/** The number of streams in @p streams */
	uint8_t count;

	/** Streams for broadcast source */
	struct bt_audio_stream **streams;

	/**
	 * @brief Codec configuration.
	 *
	 * The @p codec.meta shall include a list of CCIDs as well as a non-0
	 * context bitfield.
	 */
	const struct bt_codec *codec;

	/** Quality of Service configuration */
	const struct bt_codec_qos *qos;

	/**
	 * @brief Whether or not to encrypt the streams
	 *
	 * If set to true, then the broadcast code in @p broadcast_code
	 * will be used to encrypt the streams.
	 */
	bool encrypt;

	/** 16-octet broadcast code */
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
};

/**
 * @brief Start broadcast audio.
 *
 * Create a new audio broadcast source with one or more audio streams.
 *
 * The broadcast source will be visible for scanners once this has been called,
 * and the device will advertise audio announcements.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param[in]  param   Parameters to start the audio streams.
 * @param[out] source  Pointer to the broadcast source started.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_broadcast_audio_start(const struct bt_cap_broadcast_audio_start_param *param,
				 struct bt_audio_broadcast_source **source);

/**
 * @brief Update broadcast audio streams for a broadcast source.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source The broadcast source to update.
 * @param meta_count       The number of entries in @p meta.
 * @param meta             The new metadata. The metadata shall a list of CCIDs as
 *                         well as a non-0 context bitfield.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_broadcast_audio_update(struct bt_audio_broadcast_source *broadcast_source,
				  uint8_t meta_count,
				  const struct bt_codec_data *meta);

/**
 * @brief Stop broadcast audio streams for a broadcast source.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source The broadcast source to stop. The audio streams
 *                         in this will be stopped and reset, and the
 *                         @p broadcast_source will be invalidated.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_broadcast_audio_stop(struct bt_audio_broadcast_source *broadcast_source);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_ */
