/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MAIN_H__
#define __MAIN_H__

#include <zephyr/mgmt/bmc.h>

static inline bool is_boot_finished(void)
{
	return bmc_is_boot_finished();
}

#endif
