/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 ASPEED Technology Inc.
 */
#ifndef ZEPHYR_SOC_ARM_ASPEED_UTIL_H_
#define ZEPHYR_SOC_ARM_ASPEED_UTIL_H_
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

/* gcc.h doesn't define __section but checkpatch.pl will complain for this. so
 * temporarily add a macro here.
 */
#ifndef __section
#define __section(x)    __attribute__((__section__(x)))
#endif

/* to make checkpatch.pl happy */
#define ALIGNED16_SECTION(name)         (aligned(16), section(name))
#define __section_aligned16(name)       __attribute__(ALIGNED16_SECTION(name))

/* non-cached (DMA) memory */
#if (CONFIG_SRAM_NC_SIZE > 0)
#define NON_CACHED_BSS                  __section(".nocache.bss")
#define NON_CACHED_BSS_ALIGN16          __section_aligned16(".nocache.bss")
#else
#define NON_CACHED_BSS
#define NON_CACHED_BSS_ALIGN16          __aligned(16)
#endif

#define reg_read_poll_timeout(map, reg, val, cond, sleep_ms, timeout_ms)	    \
	({									    \
		uint32_t __timeout_tick = Z_TIMEOUT_MS(timeout_ms).ticks;	    \
		uint32_t __start = sys_clock_tick_get_32();			    \
		int __ret = 0;							    \
		for (;;) {							    \
			val.value = map->reg.value;				    \
			if (cond) {						    \
				break;						    \
			}							    \
			if ((sys_clock_tick_get_32() - __start) > __timeout_tick) { \
				__ret = -ETIMEDOUT;				    \
				break;						    \
			}							    \
			if (sleep_ms) {						    \
				k_msleep(sleep_ms);				    \
			}							    \
		}								    \
		__ret;								    \
	})

/* Common reset control device name for all ASPEED SOC family */
#define ASPEED_RST_CTRL_NAME DT_INST_RESETS_LABEL(0)
#define	DEBUG_HALT()	{ volatile int halt = 1; while (halt) { __asm__ volatile("nop"); } }
#endif
