/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <system.h>

/* To simulate XIP on QEMU, we split RAM into two chunks, with the
 * higher-addressed chunk considered "ROM"
 */

#define HALF_RAM		(ONCHIP_MEMORY2_0_SPAN / 2)

#define _RESET_VECTOR		(ONCHIP_MEMORY2_0_BASE + HALF_RAM)
#define	_EXC_VECTOR		ALT_CPU_EXCEPTION_ADDR

#define _ROM_ADDR		(ONCHIP_MEMORY2_0_BASE + HALF_RAM)
#define _ROM_SIZE		HALF_RAM

#define _RAM_ADDR		ONCHIP_MEMORY2_0_BASE
#define _RAM_SIZE		HALF_RAM
