/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/winstream.h>
#include <zephyr/devicetree.h>

#include <adsp_memory.h>
#include <mem_window.h>
#include <soc.h>

struct k_spinlock trace_lock;

static struct sys_winstream *winstream;

void winstream_console_trace_out(int8_t *str, size_t len)
{
	if (len == 0) {
		return;
	}

#ifdef CONFIG_ADSP_TRACE_SIMCALL
	register int a2 __asm__("a2") = 4; /* SYS_write */
	register int a3 __asm__("a3") = 1; /* fd 1 == stdout */
	register int a4 __asm__("a4") = (int)str;
	register int a5 __asm__("a5") = len;

	__asm__ volatile("simcall" : "+r"(a2), "+r"(a3) : "r"(a4), "r"(a5) : "memory");
#endif

	k_spinlock_key_t key = k_spin_lock(&trace_lock);

	sys_winstream_write(winstream, str, len);
	k_spin_unlock(&trace_lock, key);
}

int arch_printk_char_out(int c)
{
	int8_t s = c;

	winstream_console_trace_out(&s, 1);
	return 0;
}

static int winstream_console_init(const struct device *d)
{
	ARG_UNUSED(d);
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	const struct mem_win_config *config = dev->config;
	void *buf =
		arch_xtensa_uncached_ptr((__sparse_force void __sparse_cache *)config->mem_base);

	winstream = sys_winstream_init(buf, config->size);

	return 0;
}

SYS_INIT(winstream_console_init, EARLY, CONFIG_CONSOLE_INIT_PRIORITY);
