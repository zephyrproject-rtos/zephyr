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
 * Linker script for the Nios II platform
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>


SECTIONS
 {

/* Ideally we would have here the platform default linker script SECTIONS */
/* content */


#include <linker/common-rom.ld>

#include <linker/common-ram.ld>

 __data_ram_end = .;
 }
