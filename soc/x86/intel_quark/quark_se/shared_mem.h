/*
 * Copyright (c) Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

/* Start of the shared 80K RAM */
#define SHARED_ADDR_START 0xA8000000

struct shared_mem {
	u32_t arc_start;
	u32_t flags;
};

#define ARC_READY	(1 << 0)

#define shared_data ((volatile struct shared_mem *) SHARED_ADDR_START)

#endif
