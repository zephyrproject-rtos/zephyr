/*
 * Copyright (C) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <zephyr/types.h>

static uint32_t last_id __attribute__((section(".last_section"))) __attribute__((__used__)) =
	CONFIG_LINKER_LAST_SECTION_ID_PATTERN;
