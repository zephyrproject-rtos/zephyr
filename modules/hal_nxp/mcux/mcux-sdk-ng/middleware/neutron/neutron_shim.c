/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Neutron driver blob references MCUX SDK helpers with no Zephyr
 * counterpart: the debug console (DbgConsole_Printf / DbgConsole_Vprintf)
 * for its diagnostic messages and cleanCache_by_Addr() for cache maintenance
 * (the blob defines it weakly). Route them to the equivalent Zephyr APIs.
 */

#include <stdarg.h>
#include <stdint.h>

#include <zephyr/cache.h>
#include <zephyr/sys/printk.h>

int DbgConsole_Printf(const char *fmt_s, ...)
{
	va_list ap;

	va_start(ap, fmt_s);
	vprintk(fmt_s, ap);
	va_end(ap);

	return 0;
}

int DbgConsole_Vprintf(const char *fmt_s, va_list formatStringArg)
{
	vprintk(fmt_s, formatStringArg);

	return 0;
}

void cleanCache_by_Addr(uint32_t addr, uint32_t size)
{
	sys_cache_data_flush_range((uint32_t *)addr, (int32_t)size);
}
