/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

. = DT_REG_ADDR(DT_NODELABEL(option_setting_ofs0));
KEEP(*(.option_setting_ofs0))

. = DT_REG_ADDR(DT_NODELABEL(option_setting_ofs1));
KEEP(*(.option_setting_ofs1))

. = DT_REG_ADDR(DT_NODELABEL(option_setting_secmpu));
KEEP(*(.option_setting_secmpu))
