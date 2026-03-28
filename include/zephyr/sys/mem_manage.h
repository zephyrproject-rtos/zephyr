/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H
#define ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H

/**
 * @brief Memory Management
 * @defgroup memory_management Memory Management
 * @ingroup os_services
 * @{
 */

#ifndef _ASMLANGUAGE
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Check if a physical address is within range of physical memory.
 *
 * This checks if the physical address (@p virt) is within
 * permissible range, e.g. between
 * :kconfig:option:`CONFIG_SRAM_BASE_ADDRESS` and
 * (:kconfig:option:`CONFIG_SRAM_BASE_ADDRESS` +
 *  :kconfig:option:`CONFIG_SRAM_SIZE`).
 *
 * @note Only used if
 * :kconfig:option:`CONFIG_KERNEL_VM_USE_CUSTOM_MEM_RANGE_CHECK`
 * is enabled.
 *
 * @param phys Physical address to be checked.
 *
 * @return True if physical address is within range, false if not.
 */
bool sys_mm_is_phys_addr_in_range(uintptr_t phys);

/**
 * @brief Check if a virtual address is within range of virtual memory.
 *
 * This checks if the virtual address (@p virt) is within
 * permissible range, e.g. between
 * :kconfig:option:`CONFIG_KERNEL_VM_BASE` and
 * (:kconfig:option:`CONFIG_KERNEL_VM_BASE` +
 *  :kconfig:option:`CONFIG_KERNEL_VM_SIZE`).
 *
 * @note Only used if
 * :kconfig:option:`CONFIG_KERNEL_VM_USE_CUSTOM_MEM_RANGE_CHECK`
 * is enabled.
 *
 * @param virt Virtual address to be checked.
 *
 * @return True if virtual address is within range, false if not.
 */
bool sys_mm_is_virt_addr_in_range(void *virt);

/** @} */

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H */
