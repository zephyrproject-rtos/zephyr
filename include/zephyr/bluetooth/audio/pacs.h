/* @file
 * @brief Internal APIs for Audio Capabilities handling
 *
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_PACS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_PACS_H_

#include <zephyr/bluetooth/audio/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Published Audio Capability structure. */
struct bt_pacs_cap {
	/** Codec capability reference */
	const struct bt_audio_codec_cap *codec_cap;

	/* Internally used list node */
	sys_snode_t _node;
};

/** @typedef bt_pacs_cap_foreach_func_t
 *  @brief Published Audio Capability iterator callback.
 *
 *  @param cap Capability found.
 *  @param user_data Data given.
 *
 *  @return true to continue to the next capability
 *  @return false to stop the iteration
 */
typedef bool (*bt_pacs_cap_foreach_func_t)(const struct bt_pacs_cap *cap,
					   void *user_data);

/** @brief Published Audio Capability iterator.
 *
 *  Iterate capabilities with endpoint direction specified.
 *
 *  @param dir Direction of the endpoint to look capability for.
 *  @param func Callback function.
 *  @param user_data Data to pass to the callback.
 */
void bt_pacs_cap_foreach(enum bt_audio_dir dir,
			 bt_pacs_cap_foreach_func_t func,
			 void *user_data);

/** @brief Register Published Audio Capability.
 *
 *  Register Audio Local Capability.
 *
 *  @param dir Direction of the endpoint to register capability for.
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pacs_cap_register(enum bt_audio_dir dir, struct bt_pacs_cap *cap);

/** @brief Unregister Published Audio Capability.
 *
 *  Unregister Audio Local Capability.
 *
 *  @param dir Direction of the endpoint to unregister capability for.
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pacs_cap_unregister(enum bt_audio_dir dir, struct bt_pacs_cap *cap);

/** @brief Set the location for an endpoint type
 *
 * @param dir      Direction of the endpoints to change location for.
 * @param location The location to be set.
 *
 */
int bt_pacs_set_location(enum bt_audio_dir dir,
			 enum bt_audio_location location);

/** @brief Set the available contexts for an endpoint type
 *
 * @param dir      Direction of the endpoints to change available contexts for.
 * @param contexts The contexts to be set.
 */
int bt_pacs_set_available_contexts(enum bt_audio_dir dir,
				   enum bt_audio_context contexts);

/** @brief Get the available contexts for an endpoint type
 *
 * @param dir      Direction of the endpoints to get contexts for.
 *
 * @return Bitmask of available contexts.
 */
enum bt_audio_context bt_pacs_get_available_contexts(enum bt_audio_dir dir);

/** @brief Set the supported contexts for an endpoint type
 *
 * @param dir      Direction of the endpoints to change available contexts for.
 * @param contexts The contexts to be set.
 */
int bt_pacs_set_supported_contexts(enum bt_audio_dir dir,
				   enum bt_audio_context contexts);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_PACS_H_ */
