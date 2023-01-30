/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/bluetooth/mesh.h>

bool lost_target_find_and_remove(uint16_t addr);
void lost_target_add(uint16_t addr);
int lost_targets_rem(void);

void common_sar_conf(uint16_t addr);
