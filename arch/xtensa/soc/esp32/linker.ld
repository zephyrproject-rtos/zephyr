/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Xtensa platform.
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>

#if (CONFIG_ESP32_FLASH_CACHE == 1)
#include "linker_flash_cache.ld"
#else
#include "linker_no_flash_cache.ld"
#endif
