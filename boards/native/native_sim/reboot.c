/*
 * Copyright (c) 2025 GARDENA GmbH
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "reboot_bottom.h"
#include "posix_board_if.h"

void sys_arch_reboot(int type)
{
	(void)type;
	native_set_reboot_on_exit();
	posix_exit(0);
}
