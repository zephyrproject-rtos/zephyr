/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <can.h>
#include <can/frame_pool.h>
#include <kernel.h>

K_MEM_SLAB_DEFINE(can_frame_pool_slab, CONFIG_CAN_FRAME_POOL_SIZE, sizeof(struct can_frame), 8);

int can_frame_alloc(struct can_frame **f, s32_t timeout)
{
	return k_mem_slab_alloc(&can_frame_pool_slab, (void**)f, timeout);
}


void can_frame_free(struct can_frame *f)
{
	k_mem_slab_free(&can_frame_pool_slab, (void**)&f);
}
