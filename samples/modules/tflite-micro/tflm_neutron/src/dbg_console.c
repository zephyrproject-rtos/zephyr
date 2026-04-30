/*
 * Copyright 2026 NXP 
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */ 

#include <stdarg.h>
#include <zephyr/sys/printk.h>

/* DbgConsole_Vprintf - redirect to Zephyr printk */
int DbgConsole_Vprintf(const char *fmt, va_list args)
{
    vprintk(fmt, args);
    return 0;
}

