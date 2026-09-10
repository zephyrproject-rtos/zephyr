/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

char *get_settings_file(void);
int get_test_round(void);
bool is_final_round(void);

void signal_next_test_round(void);
void wait_for_round_start(void);
