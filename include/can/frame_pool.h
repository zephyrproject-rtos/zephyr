/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE__CAN__FRAME_POOL_H__
#define __INCLUDE__CAN__FRAME_POOL_H__

#include <can.h>

#ifdef __cplusplus
extern "C" {
#endif

int can_frame_alloc(struct can_frame **f, s32_t timeout);
void can_frame_free(struct can_frame *f);

#ifdef __cplusplus
}
#endif

#endif /*__INCLUDE__CAN__FRAME_POOL_H__*/

