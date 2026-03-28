/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

struct save_single_data {
	uint8_t first_val;
	uint8_t second_val;
	uint8_t third_val;
	uint8_t forth_val;

	bool first_second_export_called;
	bool first_second_commit_called;
	bool first_get_called;
	bool first_set_called;

	bool second_get_called;
	bool second_set_called;

	bool third_export_called;
	bool third_commit_called;
	bool third_get_called;
	bool third_set_called;

	bool forth_export_called;
	bool forth_commit_called;
	bool forth_get_called;
	bool forth_set_called;
};

extern struct save_single_data single_data;

void settings_state_reset(void);

void single_modification_reset(void);

void settings_state_get(bool *set, bool *get, bool *export, bool *commit);
