/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/winstream.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/devicetree.h>
#include <zephyr/cache.h>

#ifdef CONFIG_SOC_FAMILY_INTEL_ADSP
#include <adsp_memory.h>
#include <mem_window.h>
#endif

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

static void winstream_console_hook_install(void)
{
#if defined(CONFIG_STDOUT_CONSOLE)
	__stdout_hook_install(arch_printk_char_out);
#endif
#if defined(CONFIG_PRINTK)
	__printk_hook_install(arch_printk_char_out);
#endif
}

/* This gets optionally defined by the platform layer as it needs (it
 * might want to go in a special location to coordinate with linux
 * userspace, etc...)
 */
extern char _winstream_console_buf[];

/* This descriptor with a 96-bit magic number gets linked into the
 * binary when enabled so that external tooling can easily find it at
 * runtime (e.g. by searching the binary image file, etc...)
 */
#define WINSTREAM_CONSOLE_MAGIC1 0xd06a5f74U
#define WINSTREAM_CONSOLE_MAGIC2 0x004fe279U
#define WINSTREAM_CONSOLE_MAGIC3 0xf9bdb8cdU

struct winstream_console_desc {
	uint32_t magic1;
	uint32_t magic2;
	uint32_t magic3;
	uint32_t buf_addr;
	uint32_t size;
};

static const __used struct winstream_console_desc wsdesc = {
	.magic1 = WINSTREAM_CONSOLE_MAGIC1,
	.magic2 = WINSTREAM_CONSOLE_MAGIC2,
	.magic3 = WINSTREAM_CONSOLE_MAGIC3,
	.buf_addr = (uint32_t) &_winstream_console_buf,
	.size = CONFIG_WINSTREAM_CONSOLE_STATIC_SIZE,
};

static int winstream_console_init(void)
{
	void *buf = NULL;
	size_t size = 0;

#ifdef CONFIG_SOC_FAMILY_INTEL_ADSP
	/* These have a SOC-specific "mem_window" device.  FIXME: The
	 * type handling is backwards here.  We shouldn't be grabbing
	 * an arbitrary DTS alias and assuming it's a mem_window at
	 * runtime, that's not safe.  The mem_window init code (which
	 * is typesafe by construction) should be detecting that it's
	 * supposed to be the console and starting the console hook
	 * registration process.
	 */
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	const struct mem_win_config *config = dev->config;
	buf = sys_cache_uncached_ptr_get((__sparse_force void __sparse_cache *)config->mem_base);
	size = config->size;
#endif

#ifdef CONFIG_WINSTREAM_CONSOLE_STATIC
	/* Dirty trick to prevent linker garbage collection */
	_winstream_console_buf[0] = ((volatile char*) &wsdesc)[0];

	buf = &_winstream_console_buf;
	size = CONFIG_WINSTREAM_CONSOLE_STATIC_SIZE;
#endif

	__ASSERT_NO_MSG(buf != NULL && size != 0);
	winstream = sys_winstream_init(buf, size);
	winstream_console_hook_install();

	return 0;
}

SYS_INIT(winstream_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
