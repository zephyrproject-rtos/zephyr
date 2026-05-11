/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __VPD_H__
#define __VPD_H__

#include <zephyr/version.h>

/* Taken from zephyr/kernel/banner.c */
#if defined(BUILD_VERSION) && !IS_EMPTY(BUILD_VERSION)
#define BANNER_VERSION STRINGIFY(BUILD_VERSION)
#else
#define BANNER_VERSION KERNEL_VERSION_STRING
#endif /* BUILD_VERSION */

int get_bmc_uuid_string(const char **str);
int vpd_init(void);

#endif
