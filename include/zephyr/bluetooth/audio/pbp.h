/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_PBP_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_PBP_

/**
 * @brief Public Broadcast Profile (PBP)
 *
 * @defgroup bt_pbp Public Broadcast Profile (PBP)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/audio/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PBA Service UUID + Public Broadcast Announcement features + Metadata Length */
#define BT_PBP_MIN_PBA_SIZE		(BT_UUID_SIZE_16 + 1 + 1)

/** Public Broadcast Announcement features */
enum bt_pbp_announcement_feature {
	/** Broadcast Streams encryption status */
	BT_PBP_ANNOUNCEMENT_FEATURE_ENCRYPTION = BIT(0),
	/** Standard Quality Public Broadcast Audio configuration */
	BT_PBP_ANNOUNCEMENT_FEATURE_STANDARD_QUALITY = BIT(1),
	/** High Quality Public Broadcast Audio configuration */
	BT_PBP_ANNOUNCEMENT_FEATURE_HIGH_QUALITY = BIT(2),
};

/**
 * @brief Creates a Public Broadcast Announcement based on the information received
 * in the features parameter.
 *
 * @param meta		Metadata to be included in the advertising data
 * @param meta_len	Size of the metadata fields to be included in the advertising data
 * @param features	Public Broadcast Announcement features
 * @param pba_data_buf	Pointer to store the PBA advertising data. Buffer size needs to be
 *			meta_len + @ref BT_PBP_MIN_PBA_SIZE.
 *
 * @return 0 on success or an appropriate error code.
 */
int bt_pbp_get_announcement(const uint8_t meta[], size_t meta_len,
			    enum bt_pbp_announcement_feature features,
			    struct net_buf_simple *pba_data_buf);

/**
 * @brief Parses the received advertising data corresponding to a Public Broadcast
 * Announcement. Returns the advertised Public Broadcast Announcement features and metadata.
 *
 * @param data			Advertising data to be checked
 * @param features		Public broadcast source features
 * @param meta			Pointer to copy the metadata present in the advertising data
 *
 * @return parsed metadata length on success or an appropriate error code
 */
uint8_t bt_pbp_parse_announcement(struct bt_data *data,
				  enum bt_pbp_announcement_feature *features,
				  uint8_t *meta);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_PBP_ */
