/*
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic keyboard matrix driver.
 *
 * This file implements a generic keyboard matrix support. Keyboard matrix driver instances register
 * with this module using the kscan_matrix_driver_api, and this module in turn registers with the
 * general kscan driver using the kscan_driver_api.
 */

#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/kscan_matrix.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_KSCAN_LOG_LEVEL
LOG_MODULE_REGISTER(kscan_matrix);

/* Number of tracked scan times */
#define SCAN_OCURRENCES 30U

#define KSCAN_DEV_NAME DT_ALIAS(kscan0)

struct kscan_matrix_data {
	/* variables in usec units */
	uint32_t deb_time_press;
	uint32_t deb_time_rel;
	int64_t poll_timeout;
	uint32_t poll_period;
	uint8_t matrix_stable_state[CONFIG_KSCAN_MATRIX_MAX_COLUMNS];
	uint8_t matrix_unstable_state[CONFIG_KSCAN_MATRIX_MAX_COLUMNS];
	uint8_t matrix_previous_state[CONFIG_KSCAN_MATRIX_MAX_COLUMNS];
	/* Index in to the scan_clock_cycle to indicate start of debouncing */
	uint8_t scan_cycle_idx[CONFIG_KSCAN_MATRIX_MAX_COLUMNS][CONFIG_KSCAN_MATRIX_MAX_ROWS];
	/* Track previous "elapsed clock cycles" per matrix scan. This
	 * is used to calculate the debouncing time for every key
	 */
	uint8_t scan_clk_cycle[SCAN_OCURRENCES];
	uint8_t scan_cycles_idx;
	kscan_callback_t callback;
	struct k_thread thread;
	atomic_t enable_scan;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_KSCAN_MATRIX_THREAD_STACK_SIZE);
};
static struct kscan_matrix_data kb_matrix_data;
static const struct device *kscan_dev;
struct k_sem poll_lock;

static bool is_matrix_ghosting(const uint8_t *state)
{
	/* matrix keyboard designs are suceptible to ghosting.
	 * An extra key appears to be pressed when 3 keys
	 * belonging to the same block are pressed.
	 * for example, in the following block
	 *
	 * . . w . q .
	 * . . . . . .
	 * . . . . . .
	 * . . m . a .
	 *
	 * the key m would look as pressed if the user pressed keys
	 * w, q and a simultaneously. A block can also be formed,
	 * with not adjacent columns.
	 */
	for (int c = 0; c < CONFIG_KSCAN_MATRIX_MAX_COLUMNS; c++) {
		if (!state[c]) {
			continue;
		}

		for (int c_next = c + 1; c_next < CONFIG_KSCAN_MATRIX_MAX_COLUMNS; c_next++) {
			/* We AND the columns to detect a "block".
			 * This is an indication of ghosting, due to current
			 * flowing from a key which was never pressed. In our
			 * case, current flowing is a bit set to 1 as we
			 * flipped the bits when the matrix was scanned.
			 * Now we OR the colums using z&(z-1) which is
			 * non-zero only if z has more than one bit set.
			 */
			uint8_t common_row_bits = state[c] & state[c_next];

			if (common_row_bits & (common_row_bits - 1)) {
				return true;
			}
		}
	}

	return false;
}

static bool read_keyboard_matrix(uint8_t *new_state)
{
	int row;
	uint8_t key_event = 0U;

	for (int col = 0; col < CONFIG_KSCAN_MATRIX_MAX_COLUMNS; col++) {
		kscan_matrix_drive_column(kscan_dev, col);

		/* Allow the matrix to stabilize before reading it */
		k_busy_wait(CONFIG_KSCAN_MATRIX_POLL_COL_OUTPUT_SETTLE_TIME);

		kscan_matrix_read_row(kscan_dev, &row);
		new_state[col] = row & 0xFF;
		key_event |= row;
	}

	kscan_matrix_drive_column(kscan_dev, KEYBOARD_COLUMN_DRIVE_NONE);

	return key_event != 0U ? true : false;
}

static bool check_key_events(const struct device *dev)
{
	uint8_t matrix_new_state[CONFIG_KSCAN_MATRIX_MAX_COLUMNS] = { 0U };
	struct kscan_matrix_data *const data = dev->data;
	bool key_pressed = false;
	uint32_t cycles_now = k_cycle_get_32();

	if (++data->scan_cycles_idx >= SCAN_OCURRENCES) {
		data->scan_cycles_idx = 0U;
	}

	data->scan_clk_cycle[data->scan_cycles_idx] = cycles_now;

	/* Scan the matrix */
	key_pressed = read_keyboard_matrix(matrix_new_state);

	for (int c = 0; c < CONFIG_KSCAN_MATRIX_MAX_COLUMNS; c++) {
		LOG_DBG("U%x, P%x, N%x", data->matrix_unstable_state[c],
			data->matrix_previous_state[c], matrix_new_state[c]);
	}

	/* Abort if ghosting is detected */
	if (is_matrix_ghosting(matrix_new_state)) {
		return key_pressed;
	}

	uint8_t row_changed = 0U;
	uint8_t deb_col;

	/* The intent of this loop is to gather information related to key
	 * changes.
	 */
	for (int c = 0; c < CONFIG_KSCAN_MATRIX_MAX_COLUMNS; c++) {
		/* Check if there was an update from the previous scan */
		row_changed = matrix_new_state[c] ^ data->matrix_previous_state[c];

		if (!row_changed) {
			continue;
		}

		for (int r = 0; r < CONFIG_KSCAN_MATRIX_MAX_ROWS; r++) {
			/* Index all they keys that changed for each row
			 * in order to debounce each key in terms of it
			 */
			if (row_changed & BIT(r)) {
				data->scan_cycle_idx[c][r] = data->scan_cycles_idx;
			}
		}

		data->matrix_unstable_state[c] |= row_changed;
		data->matrix_previous_state[c] = matrix_new_state[c];
	}

	for (int c = 0; c < CONFIG_KSCAN_MATRIX_MAX_COLUMNS; c++) {
		deb_col = data->matrix_unstable_state[c];

		if (!deb_col) {
			continue;
		}

		/* Debouncing for each row key occurs here */
		for (int r = 0; r < CONFIG_KSCAN_MATRIX_MAX_ROWS; r++) {
			uint8_t mask = BIT(r);
			uint8_t row_bit = matrix_new_state[c] & mask;

			/* Continue if we already debounce a key */
			if (!(deb_col & mask)) {
				continue;
			}

			/* Convert the clock cycle differences to usec */
			uint32_t debt = k_cyc_to_us_floor32(
				cycles_now - data->scan_clk_cycle[data->scan_cycle_idx[c][r]]);

			/* Does the key requires more time to be debounced? */
			if (debt < (row_bit ? data->deb_time_press : data->deb_time_rel)) {
				/* Need more time to debounce */
				continue;
			}

			data->matrix_unstable_state[c] &= ~row_bit;

			/* Check if there was a change in the stable state */
			if ((data->matrix_stable_state[c] & mask) == row_bit) {
				/* Key state did not change */
				continue;
			}

			/* The current row has been debounced, therefore update
			 * the stable state. Then, proceed to notify the
			 * application about the keys pressed.
			 */
			data->matrix_stable_state[c] ^= mask;
			if (data->callback) {
				data->callback(dev, r, c, row_bit ? true : false);
			}
		}
	}

	return key_pressed;
}

static void kscan_matrix_polling_thread(const struct device *dev, void *dummy2, void *dummy3)
{
	struct kscan_matrix_data *const data = dev->data;
	uint32_t current_cycles;
	uint32_t cycles_diff;
	uint32_t wait_period;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (true) {
		kscan_matrix_resume_detection(kscan_dev, true);
		kscan_matrix_drive_column(kscan_dev, KEYBOARD_COLUMN_DRIVE_ALL);
		k_sem_take(&poll_lock, K_FOREVER);
		LOG_DBG("Start KB scan!!");

		/* Start polling */
		kscan_matrix_resume_detection(kscan_dev, false);

		uint64_t poll_time_end = sys_clock_timeout_end_calc(K_USEC(data->poll_timeout));

		while (atomic_get(&data->enable_scan) == 1U) {
			uint32_t start_period_cycles = k_cycle_get_32();

			if (check_key_events(dev)) {
				poll_time_end =
					sys_clock_timeout_end_calc(K_USEC(data->poll_timeout));

			} else if (k_uptime_ticks() > poll_time_end) {
				break;
			}

			/* Subtract the time invested from the sleep period
			 * in order to compensate for the time invested
			 * in debouncing a key
			 */
			current_cycles = k_cycle_get_32();
			cycles_diff = current_cycles - start_period_cycles;
			wait_period = data->poll_period - k_cyc_to_us_floor32(cycles_diff);

			/* Override wait_period in case it is less than 1 ms */
			if (wait_period < USEC_PER_MSEC) {
				wait_period = USEC_PER_MSEC;
			}

			/* wait period results in a larger number when
			 * current cycles counter wrap. In this case, the
			 * whole poll period is used
			 */
			if (wait_period > data->poll_period) {
				LOG_DBG("wait_period : %u", wait_period);

				wait_period = data->poll_period;
			}

			/* Allow other threads to run while we sleep */
			k_usleep(wait_period);
		}
	}
}

static void kscan_ksi_isr_callback(const struct device *dev)
{
	k_sem_give(&poll_lock);
}

int kscan_matrix_init(const struct device *dev)
{
	struct kscan_matrix_data *const data = dev->data;

	kscan_dev = DEVICE_DT_GET(KSCAN_DEV_NAME);
	if (!device_is_ready(kscan_dev)) {
		LOG_ERR("kscan device %s not ready", kscan_dev->name);
		return -ENODEV;
	}
	/* Initialize semaphore usbed by kscan task and driver */
	k_sem_init(&poll_lock, 0, 1);

	/* Time figures are transformed from msec to usec */
	data->deb_time_press = (uint32_t)(CONFIG_KSCAN_MATRIX_DEBOUNCE_DOWN * USEC_PER_MSEC);
	data->deb_time_rel = (uint32_t)(CONFIG_KSCAN_MATRIX_DEBOUNCE_UP * USEC_PER_MSEC);
	data->poll_period = (uint32_t)(CONFIG_KSCAN_MATRIX_POLL_PERIOD * USEC_PER_MSEC);
	data->poll_timeout = 100 * USEC_PER_MSEC;

	kscan_matrix_configure(kscan_dev, kscan_ksi_isr_callback);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_KSCAN_MATRIX_THREAD_STACK_SIZE,
			(void (*)(void *, void *, void *))kscan_matrix_polling_thread, (void *)dev,
			NULL, NULL, CONFIG_KSCAN_MATRIX_THREAD_PRIO, 0, K_NO_WAIT);

	return 0;
}

static int kscan_matrix_config(const struct device *dev, kscan_callback_t callback)
{
	struct kscan_matrix_data *const data = dev->data;

	if (!callback) {
		return -EINVAL;
	}
	data->callback = callback;

	return 0;
}

static int kscan_matrix_enable_interface(const struct device *dev)
{
	struct kscan_matrix_data *const data = dev->data;

	atomic_set(&data->enable_scan, 1);

	return 0;
}

static int kscan_matrix_disable_interface(const struct device *dev)
{
	struct kscan_matrix_data *const data = dev->data;

	atomic_set(&data->enable_scan, 0);

	return 0;
}
static const struct kscan_driver_api kscan_matrix_api = {
	.config = kscan_matrix_config,
	.enable_callback = kscan_matrix_enable_interface,
	.disable_callback = kscan_matrix_disable_interface,
};
DEVICE_DEFINE(kscan_matrix, CONFIG_KSCAN_MATRIX_DRV_NAME, kscan_matrix_init, NULL, &kb_matrix_data,
	      NULL, POST_KERNEL, CONFIG_KSCAN_MATRIX_TASK_INIT_PRIORITY, &kscan_matrix_api);
BUILD_ASSERT(CONFIG_KSCAN_MATRIX_TASK_INIT_PRIORITY > CONFIG_KSCAN_INIT_PRIORITY,
	     "keyboard matrix driver must be initilalized after keyboard scan driver");
