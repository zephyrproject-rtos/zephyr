/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "bootutil/bootutil_public.h"

#if CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER != -1
/* Sysbuild */
#define NET_CORE_IMAGE CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER
#else
/* Legacy child/parent */
#define NET_CORE_IMAGE 1
#endif

int boot_read_swap_state_primary_slot_hook(int image_index,
		struct boot_swap_state *state)
{
	if (image_index == NET_CORE_IMAGE) {
		/* Pretend that primary slot of image 1 unpopulated */
		state->magic = BOOT_MAGIC_UNSET;
		state->swap_type = BOOT_SWAP_TYPE_NONE;
		state->image_num = image_index;
		state->copy_done = BOOT_FLAG_UNSET;
		state->image_ok = BOOT_FLAG_UNSET;

		/* Prevent bootutil from trying to obtain true info */
		return 0;
	}

	return BOOT_HOOK_REGULAR;
}
