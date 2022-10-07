/* @file
 * @brief Internal APIs for Audio Capabilities handling
 *
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_

#include <zephyr/bluetooth/audio/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Audio Capability structure. */
struct bt_audio_capability {
	/** Capability codec reference */
	struct bt_codec *codec;

	/* Internally used list node */
	sys_snode_t _node;
};

/** @typedef bt_audio_foreach_capability_func_t
 *  @brief Capability iterator callback.
 *
 *  @param capability Capability found.
 *  @param user_data Data given.
 *
 *  @return true to continue to the next capability
 *  @return false to stop the iteration
 */
typedef bool (*bt_audio_foreach_capability_func_t)(const struct bt_audio_capability *capability,
						   void *user_data);

/** @brief Capability iterator.
 *
 *  Iterate capabilities with endpoint direction specified.
 *
 *  @param dir Direction of the endpoint to look capability for.
 *  @param func Callback function.
 *  @param user_data Data to pass to the callback.
 */
void bt_audio_foreach_capability(enum bt_audio_dir dir, bt_audio_foreach_capability_func_t func,
				 void *user_data);

/** @brief Register Audio Capability.
 *
 *  Register Audio Local Capability.
 *
 *  @param dir Direction of the endpoint to register capability for.
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_capability_register(enum bt_audio_dir dir, struct bt_audio_capability *cap);

/** @brief Unregister Audio Capability.
 *
 *  Unregister Audio Local Capability.
 *
 *  @param dir Direction of the endpoint to unregister capability for.
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_capability_unregister(enum bt_audio_dir dir, struct bt_audio_capability *cap);

/** @brief Set the location for an endpoint type
 *
 * @param dir      Direction of the endpoints to change location for.
 * @param location The location to be set.
 *
 */
int bt_audio_capability_set_location(enum bt_audio_dir dir,
				     enum bt_audio_location location);

/** @brief Set the available contexts for an endpoint type
 *
 * @param dir      Direction of the endpoints to change available contexts for.
 * @param contexts The contexts to be set.
 */
int bt_audio_capability_set_available_contexts(enum bt_audio_dir dir,
					       enum bt_audio_context contexts);

/** @brief Get the available contexts for an endpoint type
 *
 * @param dir      Direction of the endpoints to get contexts for.
 *
 * @return Bitmask of available contexts.
 */
enum bt_audio_context bt_audio_capability_get_available_contexts(enum bt_audio_dir dir);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_ */
