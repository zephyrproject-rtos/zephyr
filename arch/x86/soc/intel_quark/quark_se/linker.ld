/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

#include <autoconf.h>

/* physical address of RAM (needed for correct __ram_phys_end symbol) */
#define PHYS_RAM_ADDR    CONFIG_PHYS_RAM_ADDR

/* physical address where the kernel is loaded */
#ifdef CONFIG_XIP
  #define PHYS_LOAD_ADDR   CONFIG_PHYS_LOAD_ADDR
#else  /* !CONFIG_XIP */
  #define PHYS_LOAD_ADDR   CONFIG_PHYS_RAM_ADDR
#endif /* CONFIG_XIP */

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
#define PHYS_RAM_SIZE CONFIG_RAM_SIZE*1K - CONFIG_BSP_SHARED_RAM_SIZE
#else /* !CONFIG_SYS_POWER_DEEP_SLEEP */
#define PHYS_RAM_SIZE CONFIG_RAM_SIZE*1K
#endif

MEMORY
    {
#ifdef CONFIG_XIP
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = CONFIG_ROM_SIZE*1K
    RAM (wx)        : ORIGIN = PHYS_RAM_ADDR, LENGTH =  PHYS_RAM_SIZE
#else  /* !CONFIG_XIP */
    RAM (wx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = PHYS_RAM_SIZE
#endif /* CONFIG_XIP */
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
    BSP_SHARED_RAM (rw) : ORIGIN = CONFIG_BSP_SHARED_RAM_ADDR,
                          LENGTH = CONFIG_BSP_SHARED_RAM_SIZE
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP */

    /*
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 2K, LENGTH = 2K
    }

#include <arch/x86/linker.ld>

