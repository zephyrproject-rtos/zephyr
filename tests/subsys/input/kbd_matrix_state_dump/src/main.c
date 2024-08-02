/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

static const struct input_kbd_matrix_common_config test_cfg = {
	.row_size = INPUT_KBD_MATRIX_ROW_BITS,
	.col_size = 4,
};

DEVICE_DEFINE(kbd_matrix, "kbd-matrix", NULL, NULL,
	     NULL,  &test_cfg,
	     POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static void report_matrix_entry(int row, int col, int val)
{
	const struct device *dev = &DEVICE_NAME_GET(kbd_matrix);

	input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
	input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
	input_report_key(dev, INPUT_BTN_TOUCH, val, true, K_FOREVER);
}

int main(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	err = shell_execute_cmd(sh, "input kbd_matrix_state_dump kbd-matrix");
	if (err) {
		printf("Failed to execute the shell command: %d\n", err);
	}

	report_matrix_entry(0, 0, 1);
	report_matrix_entry(4, 0, 1);
	report_matrix_entry(1, 1, 1);
	report_matrix_entry(2, 2, 1);

	report_matrix_entry(0, 0, 0);
	report_matrix_entry(4, 0, 0);
	report_matrix_entry(1, 1, 0);
	report_matrix_entry(2, 2, 0);

	report_matrix_entry(3, 3, 1);
	report_matrix_entry(3, 3, 0);

#if CONFIG_INPUT_KBD_MATRIX_16_BIT_ROW
	report_matrix_entry(12, 0, 1);
	report_matrix_entry(12, 0, 0);
#endif

	err = shell_execute_cmd(sh, "input kbd_matrix_state_dump off");
	if (err) {
		printf("Failed to execute the shell command: %d\n", err);
	}

	return 0;
}
