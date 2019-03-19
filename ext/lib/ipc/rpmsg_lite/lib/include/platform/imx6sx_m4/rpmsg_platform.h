/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RPMSG_PLATFORM_H
#define _RPMSG_PLATFORM_H

/* RPMSG MU channel index */
#define RPMSG_MU_CHANNEL (1)

/*
 * Linux requires the ALIGN to 0x1000(4KB) instead of 0x80
 */
#ifndef VRING_ALIGN
#define VRING_ALIGN (0x1000)
#endif

/* contains pool of descriptors and two circular buffers */
#ifndef VRING_SIZE
#define VRING_SIZE (0x8000)
#endif

/* size of shared memory + 2*VRING size */
#define RL_VRING_OVERHEAD (2 * VRING_SIZE)

#define RL_GET_VQ_ID(core_id, queue_id) (((queue_id)&0x1) | (((core_id) << 1) & 0xFFFFFFFE))
#define RL_GET_LINK_ID(id) (((id)&0xFFFFFFFE) >> 1)
#define RL_GET_Q_ID(id) ((id)&0x1)

#define RL_PLATFORM_IMX6SX_M4_LINK_ID (0)
#define RL_PLATFORM_HIGHEST_LINK_ID (0)

/* platform interrupt related functions */
int platform_init_interrupt(unsigned int vector_id, void *isr_data);
int platform_deinit_interrupt(unsigned int vector_id);
int platform_interrupt_enable(unsigned int vector_id);
int platform_interrupt_disable(unsigned int vector_id);
int platform_in_isr(void);
void platform_notify(unsigned int vector_id);

/* platform low-level time-delay (busy loop) */
void platform_time_delay(int num_msec);

/* platform memory functions */
void platform_map_mem_region(unsigned int va, unsigned int pa, unsigned int size, unsigned int flags);
void platform_cache_all_flush_invalidate(void);
void platform_cache_disable(void);
unsigned long platform_vatopa(void *addr);
void *platform_patova(unsigned long addr);

/* platform init/deinit */
int platform_init(void);
int platform_deinit(void);
void rpmsg_handler(void);

#endif /* _RPMSG_PLATFORM_H */
