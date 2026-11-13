/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/toolchain.h>

/* Fits the execution region, but exceeds storage after reserving ROM_END_OFFSET. */
static const uint8_t overflow_payload[0x20000] = {1};

LINKER_KEEP(overflow_payload);
