/** @file
 * @brief Advance Audio Distribution Profile header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_

#include <zephyr/bluetooth/avdtp.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Stream Structure */
struct bt_a2dp_stream {
	/* TODO */
};

/** @brief Codec ID */
enum bt_a2dp_codec_id {
	/** Codec SBC */
	BT_A2DP_SBC = 0x00,
	/** Codec MPEG-1 */
	BT_A2DP_MPEG1 = 0x01,
	/** Codec MPEG-2 */
	BT_A2DP_MPEG2 = 0x02,
	/** Codec ATRAC */
	BT_A2DP_ATRAC = 0x04,
	/** Codec Non-A2DP */
	BT_A2DP_VENDOR = 0xff
};

/** @brief Preset for the endpoint */
struct bt_a2dp_preset {
	/** Length of preset */
	uint8_t len;
	/** Preset */
	uint8_t preset[0];
};

/** @brief Stream End Point */
struct bt_a2dp_endpoint {
	/** Code ID */
	uint8_t codec_id;
	/** Stream End Point Information */
	struct bt_avdtp_seid_lsep info;
	/** Pointer to preset codec chosen */
	struct bt_a2dp_preset *preset;
	/** Capabilities */
	struct bt_a2dp_preset *caps;
};

/** @brief Stream End Point Media Type */
enum MEDIA_TYPE {
	/** Audio Media Type */
	BT_A2DP_AUDIO = 0x00,
	/** Video Media Type */
	BT_A2DP_VIDEO = 0x01,
	/** Multimedia Media Type */
	BT_A2DP_MULTIMEDIA = 0x02
};

/** @brief Stream End Point Role */
enum ROLE_TYPE {
	/** Source Role */
	BT_A2DP_SOURCE = 0,
	/** Sink Role */
	BT_A2DP_SINK = 1
};

/** @brief A2DP structure */
struct bt_a2dp;

/** @brief A2DP Connect.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish A2DP
 *  connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return pointer to struct bt_a2dp in case of success or NULL in case
 *  of error.
 */
struct bt_a2dp *bt_a2dp_connect(struct bt_conn *conn);

/** @brief Endpoint Registration.
 *
 *  This function is used for registering the stream end points. The user has
 *  to take care of allocating the memory, the preset pointer and then pass the
 *  required arguments. Also, only one sep can be registered at a time.
 *
 *  @param endpoint Pointer to bt_a2dp_endpoint structure.
 *  @param media_type Media type that the Endpoint is.
 *  @param role Role of Endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
			      uint8_t media_type, uint8_t role);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_ */
