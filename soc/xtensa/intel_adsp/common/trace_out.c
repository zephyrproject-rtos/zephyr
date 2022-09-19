/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <soc.h>
#include <adsp_memory.h>
#include <zephyr/sys/winstream.h>

struct k_spinlock trace_lock;

static struct sys_winstream *winstream;

void intel_adsp_trace_out(int8_t *str, size_t len)
{
	if (len == 0) {
		return;
	}

#ifdef CONFIG_ADSP_TRACE_SIMCALL
	register int a2 __asm__("a2") = 4; /* SYS_write */
	register int a3 __asm__("a3") = 1; /* fd 1 == stdout */
	register int a4 __asm__("a4") = (int)str;
	register int a5 __asm__("a5") = len;

	__asm__ volatile("simcall" : "+r"(a2), "+r"(a3)
			 : "r"(a4), "r"(a5)  : "memory");
#endif

	k_spinlock_key_t key = k_spin_lock(&trace_lock);

	sys_winstream_write(winstream, str, len);
	k_spin_unlock(&trace_lock, key);
}

int arch_printk_char_out(int c)
{
	int8_t s = c;

	intel_adsp_trace_out(&s, 1);
	return 0;
}

void soc_trace_init(void)
{
	void *buf = z_soc_uncached_ptr((void *)HP_SRAM_WIN3_BASE);

	winstream = sys_winstream_init(buf, HP_SRAM_WIN3_SIZE);
}
