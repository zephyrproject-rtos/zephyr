/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

ASSERT(ORIGIN(FLASH) == DT_PROP(DT_PATH(zephyr_user), expected_rom_address),
       "Unexpected ROM address")
