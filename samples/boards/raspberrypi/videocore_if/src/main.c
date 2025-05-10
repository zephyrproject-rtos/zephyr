/*
 * Copyright (c) 2025 Yoan Dumas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "videocore_if.h"
#include "board_rev.h"

int main(void)
{
	uint32_t revision;

	printf("VideoCore (GPU) firmware version is: 0x%08X\n", get_vc_firmware_version());
	revision = get_board_revision();
	printf("Board Revision: 0x%8.8x\n", revision);
	log_board_revision(revision);

	return 0;
}
