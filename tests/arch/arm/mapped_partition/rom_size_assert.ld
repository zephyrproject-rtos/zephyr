/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

ASSERT(LENGTH(FLASH) == DT_PROP(DT_PATH(zephyr_user), expected_rom_size) - CONFIG_ROM_END_OFFSET,
       "Unexpected ROM size")
