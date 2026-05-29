/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

KEEP(*(.bindesc_skip));
. = ALIGN(4);
#include "bindesc.ld"
