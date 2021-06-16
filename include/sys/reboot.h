/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common target reboot functionality
 *
 * @details See subsys/os/Kconfig and the reboot help for details.
 */

#ifndef ZEPHYR_INCLUDE_SYS_REBOOT_H_
#define ZEPHYR_INCLUDE_SYS_REBOOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_REBOOT_WARM 0
#define SYS_REBOOT_COLD 1

/**
 * @brief Reboot the system
 *
 * Reboot the system in the manner specified by @a type.  Not all architectures
 * or platforms support the various reboot types (SYS_REBOOT_COLD,
 * SYS_REBOOT_WARM).
 *
 * When successful, this routine does not return.
 *
 * @return N/A
 */

extern void sys_reboot(int type);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_REBOOT_H_ */
