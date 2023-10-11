/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

void settings_state_reset(void);

void settings_state_get(bool *set, bool *get, bool *export, bool *commit);
