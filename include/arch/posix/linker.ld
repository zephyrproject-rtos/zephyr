/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the POSIX (native) platform
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>


SECTIONS
 {

#include <linker/common-rom.ld>

#include <linker/common-ram.ld>

#include <arch/posix/native_tasks.ld>

 __data_ram_end = .;

 } INSERT AFTER .data;

/*
 * Note that the INSERT command actually changes the meaning of the -T command
 * line switch: The script will now augment the default SECTIONS instead of
 * replacing it.
 */
