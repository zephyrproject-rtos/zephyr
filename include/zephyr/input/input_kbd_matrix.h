/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_KBD_MATRIX_H_
#define ZEPHYR_INCLUDE_INPUT_KBD_MATRIX_H_

/**
 * @brief Keyboard Matrix API
 * @defgroup input_kbd_matrix Keyboard Matrix API
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

/** Special drive_column argument for not driving any column */
#define INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE -1

/** Special drive_column argument for driving all the columns */
#define INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL  -2

/** Number of tracked scan cycles */
#define INPUT_KBD_MATRIX_SCAN_OCURRENCES 30U

/** Row entry data type */
#if CONFIG_INPUT_KBD_MATRIX_16_BIT_ROW
typedef uint16_t kbd_row_t;
#define PRIkbdrow "04" PRIx16
#else
typedef uint8_t kbd_row_t;
#define PRIkbdrow "02" PRIx8
#endif

#if defined(CONFIG_INPUT_KBD_ACTUAL_KEY_MASK_DYNAMIC) || defined(__DOXYGEN__)
#define INPUT_KBD_ACTUAL_KEY_MASK_CONST
/**
 * @brief Enables or disables a specific row, column combination in the actual
 * key mask.
 *
 * This allows enabling or disabling spcific row, column combination in the
 * actual key mask in runtime. It can be useful if some of the keys are not
 * present in some configuration, and the specific configuration is determined
 * in runtime. Requires @kconfig{CONFIG_INPUT_KBD_ACTUAL_KEY_MASK_DYNAMIC} to
 * be enabled.
 *
 * @param dev Pointer to the keyboard matrix device.
 * @param row The matrix row to enable or disable.
 * @param col The matrix column to enable or disable.
 * @param enabled Whether the specificied row, col has to be enabled or disabled.
 *
 * @retval 0 If the change is successful.
 * @retval -errno Negative errno if row or col are out of range for the device.
 */
int input_kbd_matrix_actual_key_mask_set(const struct device *dev,
					  uint8_t row, uint8_t col, bool enabled);
#else
#define INPUT_KBD_ACTUAL_KEY_MASK_CONST const
#endif

/** Maximum number of rows */
#define INPUT_KBD_MATRIX_ROW_BITS NUM_BITS(kbd_row_t)

/**
 * @brief Keyboard matrix internal APIs.
 */
struct input_kbd_matrix_api {
	/**
	 * @brief Request to drive a specific column.
	 *
	 * Request to drive a specific matrix column, or none, or all.
	 *
	 * @param dev Pointer to the keyboard matrix device.
	 * @param col The column to drive, or
	 *      @ref INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE or
	 *      @ref INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL.
	 */
	void (*drive_column)(const struct device *dev, int col);
	/**
	 * @brief Read the matrix row.
	 *
	 * @param dev Pointer to the keyboard matrix device.
	 */
	kbd_row_t (*read_row)(const struct device *dev);
	/**
	 * @brief Request to put the matrix in detection mode.
	 *
	 * Request to put the driver in detection mode, this is called after a
	 * request to drive all the column and typically involves reenabling
	 * interrupts row pin changes.
	 *
	 * @param dev Pointer to the keyboard matrix device.
	 * @param enable Whether detection mode has to be enabled or disabled.
	 */
	void (*set_detect_mode)(const struct device *dev, bool enabled);
};

/**
 * @brief Common keyboard matrix config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct input_kbd_matrix_common_config {
	const struct input_kbd_matrix_api *api;
	uint8_t row_size;
	uint8_t col_size;
	uint32_t poll_period_us;
	uint32_t poll_timeout_ms;
	uint32_t debounce_down_us;
	uint32_t debounce_up_us;
	uint32_t settle_time_us;
	bool ghostkey_check;
	INPUT_KBD_ACTUAL_KEY_MASK_CONST kbd_row_t *actual_key_mask;

	/* extra data pointers */
	kbd_row_t *matrix_stable_state;
	kbd_row_t *matrix_unstable_state;
	kbd_row_t *matrix_previous_state;
	kbd_row_t *matrix_new_state;
	uint8_t *scan_cycle_idx;
};

#define INPUT_KBD_MATRIX_DATA_NAME(node_id, name) \
	_CONCAT(__input_kbd_matrix_, \
		_CONCAT(name, DEVICE_DT_NAME_GET(node_id)))

/**
 * @brief Defines the common keyboard matrix support data from devicetree,
 * specify row and col count.
 */
#define INPUT_KBD_MATRIX_DT_DEFINE_ROW_COL(node_id, _row_size, _col_size) \
	BUILD_ASSERT(IN_RANGE(_row_size, 1, INPUT_KBD_MATRIX_ROW_BITS), "invalid row-size"); \
	BUILD_ASSERT(IN_RANGE(_col_size, 1, UINT8_MAX), "invalid col-size"); \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, actual_key_mask), ( \
	BUILD_ASSERT(DT_PROP_LEN(node_id, actual_key_mask) == _col_size, \
		     "actual-key-mask size does not match the number of columns"); \
	static INPUT_KBD_ACTUAL_KEY_MASK_CONST kbd_row_t \
		INPUT_KBD_MATRIX_DATA_NAME(node_id, actual_key_mask)[_col_size] = \
			DT_PROP(node_id, actual_key_mask); \
	)) \
	static kbd_row_t INPUT_KBD_MATRIX_DATA_NAME(node_id, stable_state)[_col_size]; \
	static kbd_row_t INPUT_KBD_MATRIX_DATA_NAME(node_id, unstable_state)[_col_size]; \
	static kbd_row_t INPUT_KBD_MATRIX_DATA_NAME(node_id, previous_state)[_col_size]; \
	static kbd_row_t INPUT_KBD_MATRIX_DATA_NAME(node_id, new_state)[_col_size]; \
	static uint8_t INPUT_KBD_MATRIX_DATA_NAME(node_id, scan_cycle_idx)[_row_size * _col_size];

/**
 * @brief Defines the common keyboard matrix support data from devicetree.
 */
#define INPUT_KBD_MATRIX_DT_DEFINE(node_id) \
	INPUT_KBD_MATRIX_DT_DEFINE_ROW_COL( \
		node_id, DT_PROP(node_id, row_size), DT_PROP(node_id, col_size))

/**
 * @brief Defines the common keyboard matrix support data from devicetree
 * instance, specify row and col count.
 *
 * @param inst Instance.
 * @param row_size The matrix row count.
 * @param col_size The matrix column count.
 */
#define INPUT_KBD_MATRIX_DT_INST_DEFINE_ROW_COL(inst, row_size, col_size) \
	INPUT_KBD_MATRIX_DT_DEFINE_ROW_COL(DT_DRV_INST(inst), row_size, col_size)

/**
 * @brief Defines the common keyboard matrix support data from devicetree instance.
 *
 * @param inst Instance.
 */
#define INPUT_KBD_MATRIX_DT_INST_DEFINE(inst) \
	INPUT_KBD_MATRIX_DT_DEFINE(DT_DRV_INST(inst))

/**
 * @brief Initialize common keyboard matrix config from devicetree, specify row and col count.
 *
 * @param node_id The devicetree node identifier.
 * @param _api Pointer to a @ref input_kbd_matrix_api structure.
 * @param _row_size The matrix row count.
 * @param _col_size The matrix column count.
 */
#define INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT_ROW_COL(node_id, _api, _row_size, _col_size) \
	{ \
		.api = _api, \
		.row_size = _row_size, \
		.col_size = _col_size, \
		.poll_period_us = DT_PROP(node_id, poll_period_ms) * USEC_PER_MSEC, \
		.poll_timeout_ms = DT_PROP(node_id, poll_timeout_ms), \
		.debounce_down_us = DT_PROP(node_id, debounce_down_ms) * USEC_PER_MSEC, \
		.debounce_up_us = DT_PROP(node_id, debounce_up_ms) * USEC_PER_MSEC, \
		.settle_time_us = DT_PROP(node_id, settle_time_us), \
		.ghostkey_check = !DT_PROP(node_id, no_ghostkey_check), \
		IF_ENABLED(DT_NODE_HAS_PROP(node_id, actual_key_mask), ( \
		.actual_key_mask = INPUT_KBD_MATRIX_DATA_NAME(node_id, actual_key_mask), \
		)) \
		\
		.matrix_stable_state = INPUT_KBD_MATRIX_DATA_NAME(node_id, stable_state), \
		.matrix_unstable_state = INPUT_KBD_MATRIX_DATA_NAME(node_id, unstable_state), \
		.matrix_previous_state = INPUT_KBD_MATRIX_DATA_NAME(node_id, previous_state), \
		.matrix_new_state = INPUT_KBD_MATRIX_DATA_NAME(node_id, new_state), \
		.scan_cycle_idx = INPUT_KBD_MATRIX_DATA_NAME(node_id, scan_cycle_idx), \
	}

/**
 * @brief Initialize common keyboard matrix config from devicetree.
 *
 * @param node_id The devicetree node identifier.
 * @param api Pointer to a @ref input_kbd_matrix_api structure.
 */
#define INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT(node_id, api) \
	INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT_ROW_COL( \
		node_id, api, DT_PROP(node_id, row_size), DT_PROP(node_id, col_size))

/**
 * @brief Initialize common keyboard matrix config from devicetree instance,
 * specify row and col count.
 *
 * @param inst Instance.
 * @param api Pointer to a @ref input_kbd_matrix_api structure.
 * @param row_size The matrix row count.
 * @param col_size The matrix column count.
 */
#define INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT_ROW_COL(inst, api, row_size, col_size) \
	INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT_ROW_COL(DT_DRV_INST(inst), api, row_size, col_size)

/**
 * @brief Initialize common keyboard matrix config from devicetree instance.
 *
 * @param inst Instance.
 * @param api Pointer to a @ref input_kbd_matrix_api structure.
 */
#define INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(inst, api) \
	INPUT_KBD_MATRIX_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst), api)

/**
 * @brief Common keyboard matrix data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct input_kbd_matrix_common_data {
	/* Track previous cycles, used for debouncing. */
	uint32_t scan_clk_cycle[INPUT_KBD_MATRIX_SCAN_OCURRENCES];
	uint8_t scan_cycles_idx;

	struct k_sem poll_lock;

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
 * @brief Start scanning the keyboard matrix
 *
 * Starts the keyboard matrix scanning cycle, this should be called in reaction
 * of a press event, after the device has been put in detect mode.
 *
 * @param dev Keyboard matrix device instance.
 */
void input_kbd_matrix_poll_start(const struct device *dev);

#if defined(CONFIG_INPUT_KBD_DRIVE_COLUMN_HOOK) || defined(__DOXYGEN__)
/**
 * @brief Drive column hook
 *
 * This can be implemented by the application to handle column selection
 * quirks. Called after the driver specific drive_column function. Requires
 * @kconfig{CONFIG_INPUT_KBD_DRIVE_COLUMN_HOOK} to be enabled.
 *
 * @param dev Keyboard matrix device instance.
 * @param col The column to drive, or
 *      @ref INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE or
 *      @ref INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL.
 */
void input_kbd_matrix_drive_column_hook(const struct device *dev, int col);
#endif

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

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_KBD_MATRIX_H_ */
