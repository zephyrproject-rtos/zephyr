/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

void fake_clock_reset(void);

unsigned int fake_clock_on_count(void);

uintptr_t fake_clock_last_subsys(void);

void fake_clock_set_on_result(int ret);
