/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <system.h>

#define _RESET_VECTOR		ALT_CPU_RESET_ADDR
#define	_EXC_VECTOR		ALT_CPU_EXCEPTION_ADDR

#define _ROM_ADDR		ONCHIP_FLASH_0_DATA_BASE
#define _ROM_SIZE		ONCHIP_FLASH_0_DATA_SPAN

#define _RAM_ADDR		ONCHIP_MEMORY2_0_BASE
#define _RAM_SIZE		ONCHIP_MEMORY2_0_SPAN
