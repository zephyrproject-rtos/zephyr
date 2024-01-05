/*
 * Copyright 2019 Intel Corporation
 * Copyright 2022 Nuvoton Technology Corporation.
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(input_kbd_matrix, CONFIG_INPUT_LOG_LEVEL);

void input_kbd_matrix_poll_start(const struct device *dev)
{
	struct input_kbd_matrix_common_data *data = dev->data;

	k_sem_give(&data->poll_lock);
}

static bool input_kbd_matrix_ghosting(const struct device *dev)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	const kbd_row_t *state = cfg->matrix_new_state;

	/*
	 * Matrix keyboard designs are suceptible to ghosting.
	 * An extra key appears to be pressed when 3 keys belonging to the same
	 * block are pressed. For example, in the following block:
	 *
	 * . . w . q .
	 * . . . . . .
	 * . . . . . .
	 * . . m . a .
	 *
	 * the key m would look as pressed if the user pressed keys w, q and a
	 * simultaneously. A block can also be formed, with not adjacent
	 * columns.
	 */
	for (int c = 0; c < cfg->col_size; c++) {
		if (!state[c]) {
			continue;
		}

		for (int c_next = c + 1; c_next < cfg->col_size; c_next++) {
			/*
			 * We AND the columns to detect a "block". This is an
			 * indication of ghosting, due to current flowing from
			 * a key which was never pressed. In our case, current
			 * flowing is a bit set to 1 as we flipped the bits
			 * when the matrix was scanned. Now we OR the colums
			 * using z&(z-1) which is non-zero only if z has more
			 * than one bit set.
			 */
			kbd_row_t common_row_bits = state[c] & state[c_next];

			if (common_row_bits & (common_row_bits - 1)) {
				return true;
			}
		}
	}

	return false;
}

static void input_kbd_matrix_drive_column(const struct device *dev, int col)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	const struct input_kbd_matrix_api *api = cfg->api;

	api->drive_column(dev, col);

#ifdef CONFIG_INPUT_KBD_DRIVE_COLUMN_HOOK
	input_kbd_matrix_drive_column_hook(dev, col);
#endif
}

static bool input_kbd_matrix_scan(const struct device *dev)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	const struct input_kbd_matrix_api *api = cfg->api;
	kbd_row_t row;
	kbd_row_t key_event = 0U;

	for (int col = 0; col < cfg->col_size; col++) {
		if (cfg->actual_key_mask != NULL &&
		    cfg->actual_key_mask[col] == 0) {
			continue;
		}

		input_kbd_matrix_drive_column(dev, col);

		/* Allow the matrix to stabilize before reading it */
		k_busy_wait(cfg->settle_time_us);

		row = api->read_row(dev);

		if (cfg->actual_key_mask != NULL) {
			row &= cfg->actual_key_mask[col];
		}

		cfg->matrix_new_state[col] = row;
		key_event |= row;
	}

	input_kbd_matrix_drive_column(dev, INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE);

	return key_event != 0U;
}

static void input_kbd_matrix_update_state(const struct device *dev)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	struct input_kbd_matrix_common_data *data = dev->data;
	kbd_row_t *matrix_new_state = cfg->matrix_new_state;
	uint32_t cycles_now;
	kbd_row_t row_changed;
	kbd_row_t deb_col;

	cycles_now = k_cycle_get_32();

	data->scan_clk_cycle[data->scan_cycles_idx] = cycles_now;

	/*
	 * The intent of this loop is to gather information related to key
	 * changes.
	 */
	for (int c = 0; c < cfg->col_size; c++) {
		/* Check if there was an update from the previous scan */
		row_changed = matrix_new_state[c] ^ cfg->matrix_previous_state[c];

		if (!row_changed) {
			continue;
		}

		for (int r = 0; r < cfg->row_size; r++) {
			uint8_t cyc_idx = c * cfg->row_size + r;

			/*
			 * Index all they keys that changed for each row in
			 * order to debounce each key in terms of it
			 */
			if (row_changed & BIT(r)) {
				cfg->scan_cycle_idx[cyc_idx] = data->scan_cycles_idx;
			}
		}

		cfg->matrix_unstable_state[c] |= row_changed;
		cfg->matrix_previous_state[c] = matrix_new_state[c];
	}

	for (int c = 0; c < cfg->col_size; c++) {
		deb_col = cfg->matrix_unstable_state[c];

		if (!deb_col) {
			continue;
		}

		/* Debouncing for each row key occurs here */
		for (int r = 0; r < cfg->row_size; r++) {
			kbd_row_t mask = BIT(r);
			kbd_row_t row_bit = matrix_new_state[c] & mask;

			/* Continue if we already debounce a key */
			if (!(deb_col & mask)) {
				continue;
			}

			uint8_t cyc_idx = c * cfg->row_size + r;
			uint8_t scan_cyc_idx = cfg->scan_cycle_idx[cyc_idx];
			uint32_t scan_clk_cycle = data->scan_clk_cycle[scan_cyc_idx];

			/* Convert the clock cycle differences to usec */
			uint32_t deb_t_us = k_cyc_to_us_floor32(cycles_now - scan_clk_cycle);

			/* Does the key requires more time to be debounced? */
			if (deb_t_us < (row_bit ? cfg->debounce_down_us : cfg->debounce_up_us)) {
				/* Need more time to debounce */
				continue;
			}

			cfg->matrix_unstable_state[c] &= ~mask;

			/* Check if there was a change in the stable state */
			if ((cfg->matrix_stable_state[c] & mask) == row_bit) {
				/* Key state did not change */
				continue;
			}

			/*
			 * The current row has been debounced, therefore update
			 * the stable state. Then, proceed to notify the
			 * application about the keys pressed.
			 */
			cfg->matrix_stable_state[c] ^= mask;

			input_report_abs(dev, INPUT_ABS_X, c, false, K_FOREVER);
			input_report_abs(dev, INPUT_ABS_Y, r, false, K_FOREVER);
			input_report_key(dev, INPUT_BTN_TOUCH, row_bit, true, K_FOREVER);
		}
	}

	data->scan_cycles_idx = (data->scan_cycles_idx + 1) % INPUT_KBD_MATRIX_SCAN_OCURRENCES;
}

static bool input_kbd_matrix_check_key_events(const struct device *dev)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	bool key_pressed;

	/* Scan the matrix */
	key_pressed = input_kbd_matrix_scan(dev);

	for (int c = 0; c < cfg->col_size; c++) {
		LOG_DBG("c=%2d u=" PRIkbdrow " p=" PRIkbdrow " n=" PRIkbdrow,
			c,
			cfg->matrix_unstable_state[c],
			cfg->matrix_previous_state[c],
			cfg->matrix_new_state[c]);
	}

	/* Abort if ghosting is detected */
	if (cfg->ghostkey_check && input_kbd_matrix_ghosting(dev)) {
		return key_pressed;
	}

	input_kbd_matrix_update_state(dev);

	return key_pressed;
}

static k_timepoint_t input_kbd_matrix_poll_timeout(const struct device *dev)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;

	if (cfg->poll_timeout_ms == 0) {
		return sys_timepoint_calc(K_FOREVER);
	}

	return sys_timepoint_calc(K_MSEC(cfg->poll_timeout_ms));
}

static void input_kbd_matrix_poll(const struct device *dev)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	k_timepoint_t poll_time_end;
	uint32_t current_cycles;
	uint32_t cycles_diff;
	uint32_t wait_period_us;

	poll_time_end = input_kbd_matrix_poll_timeout(dev);

	while (true) {
		uint32_t start_period_cycles = k_cycle_get_32();

		if (input_kbd_matrix_check_key_events(dev)) {
			poll_time_end = input_kbd_matrix_poll_timeout(dev);
		} else if (sys_timepoint_expired(poll_time_end)) {
			break;
		}

		/*
		 * Subtract the time invested from the sleep period in order to
		 * compensate for the time invested in debouncing a key
		 */
		current_cycles = k_cycle_get_32();
		cycles_diff = current_cycles - start_period_cycles;
		wait_period_us = cfg->poll_period_us - k_cyc_to_us_floor32(cycles_diff);

		wait_period_us = CLAMP(wait_period_us,
				       USEC_PER_MSEC, cfg->poll_period_us);

		LOG_DBG("wait_period_us: %d", wait_period_us);

		/* Allow other threads to run while we sleep */
		k_usleep(wait_period_us);
	}
}

static void input_kbd_matrix_polling_thread(void *arg1, void *unused2, void *unused3)
{
	const struct device *dev = arg1;
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	const struct input_kbd_matrix_api *api = cfg->api;
	struct input_kbd_matrix_common_data *data = dev->data;

	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	while (true) {
		input_kbd_matrix_drive_column(dev, INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL);
		api->set_detect_mode(dev, true);

		/* Check the rows again after enabling the interrupt to catch
		 * any potential press since the last read.
		 */
		if (api->read_row(dev) != 0) {
			input_kbd_matrix_poll_start(dev);
		}

		k_sem_take(&data->poll_lock, K_FOREVER);
		LOG_DBG("scan start");

		/* Disable interrupt of KSI pins and start polling */
		api->set_detect_mode(dev, false);

		input_kbd_matrix_poll(dev);
	}
}

int input_kbd_matrix_common_init(const struct device *dev)
{
	struct input_kbd_matrix_common_data *data = dev->data;

	k_sem_init(&data->poll_lock, 0, 1);

	k_thread_create(&data->thread, data->thread_stack,
			K_KERNEL_STACK_SIZEOF(data->thread_stack),
			input_kbd_matrix_polling_thread, (void *)dev, NULL, NULL,
			CONFIG_INPUT_KBD_MATRIX_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, dev->name);

	return 0;
}

#if CONFIG_INPUT_KBD_ACTUAL_KEY_MASK_DYNAMIC
int input_kbd_matrix_actual_key_mask_set(const struct device *dev,
					  uint8_t row, uint8_t col, bool enabled)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;

	if (row >= cfg->row_size || col >= cfg->col_size) {
		return -EINVAL;
	}

	if (cfg->actual_key_mask == NULL) {
		LOG_WRN("actual-key-mask not defined for %s", dev->name);
		return -EINVAL;
	}

	WRITE_BIT(cfg->actual_key_mask[col], row, enabled);

	return 0;
}
#endif
