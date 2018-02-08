/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACHINE_SYSTEM_H
#define _MACHINE_SYSTEM_H

/*
* No need to align the VRING as defined in Linux because LPC5411x is not intended
* to run the Linux
*/
#ifndef VRING_ALIGN
#define VRING_ALIGN (0x10)
#endif

/* contains pool of descriptos and two circular buffers */
#ifndef VRING_SIZE
#define VRING_SIZE (0x400)
#endif

/* size of shared memory + 2*VRING size */
#define RL_VRING_OVERHEAD (2 * VRING_SIZE)

#define RL_GET_VQ_ID(core_id, queue_id) (((queue_id)&0x1) | (((core_id) << 1) & 0xFFFFFFFE))
#define RL_GET_LINK_ID(id) (((id)&0xFFFFFFFE) >> 1)
#define RL_GET_Q_ID(id) ((id)&0x1)

#define RL_PLATFORM_LPC5411x_M4_M0_LINK_ID (0)
#define RL_PLATFORM_HIGHEST_LINK_ID (0)

/* platform interrupt related functions */
int platform_init_interrupt(int vector_id, void *isr_data);
int platform_deinit_interrupt(int vector_id);
int platform_interrupt_enable(unsigned int vector_id);
int platform_interrupt_disable(unsigned int vector_id);
int platform_in_isr(void);
void platform_notify(int vector_id);

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

#endif /* _MACHINE_SYSTEM_H */
