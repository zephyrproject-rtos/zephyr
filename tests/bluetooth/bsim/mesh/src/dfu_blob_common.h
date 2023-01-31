/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

bool lost_target_find_and_remove(uint16_t addr);
void lost_target_add(uint16_t addr);
int lost_targets_rem(void);
