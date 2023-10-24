/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

/**
 * @brief Keyboard matrix internal APIs.
 */
struct input_kbd_matrix_api {
	k_thread_entry_t polling_thread;
};

/**
 * @brief Common keyboard matrix config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct input_kbd_matrix_common_config {
	struct input_kbd_matrix_api api;
};

/**
 * @brief Initialize common keyboard matrix config from devicetree.
 *
 * @param api Pointer to a :c:struct:`input_kbd_matrix_api` structure.
 */
#define INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT(node_id, _api) \
	{ \
		.api = _api, \
	}

/**
 * @brief Initialize common keyboard matrix config from devicetree instance.
 *
 * @param inst Instance.
 * @param api Pointer to a :c:struct:`input_kbd_matrix_api` structure.
 */
#define INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(inst, api) \
	INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst), api)

/**
 * @brief Common keyboard matrix data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct input_kbd_matrix_common_data {
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack,
			      CONFIG_INPUT_KBD_MATRIX_THREAD_STACK_SIZE);
};

/**
 * @brief Validate the offset of the common data structures.
 *
 * @param config Name of the config structure.
 * @param data Name of the data structure.
 */
#define INPUT_KBD_STRUCT_CHECK(config, data) \
	BUILD_ASSERT(offsetof(config, common) == 0, \
		     "struct input_kbd_matrix_common_config must be placed first"); \
	BUILD_ASSERT(offsetof(data, common) == 0, \
		     "struct input_kbd_matrix_common_data must be placed first")

/**
 * @brief Common function to initialize a keyboard matrix device at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev Keyboard matrix device instance.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int input_kbd_matrix_common_init(const struct device *dev);
