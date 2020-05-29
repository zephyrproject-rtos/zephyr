/*
 * Copyright (c) 2014-2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GREYBUS_UTILS_DEBUG_H_
#define _GREYBUS_UTILS_DEBUG_H_

//#include <debug.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
//#include <util.h>
//#include <config.h>
#include <greybus/types.h>

//#include <arch/irq.h>

#ifndef BIT
#define BIT(n) (1 << n)
#endif

#define GB_LOG_INFO     BIT(0)
#define GB_LOG_ERROR    BIT(1)
#define GB_LOG_WARNING  BIT(2)
#define GB_LOG_DEBUG    BIT(3)
#define GB_LOG_DUMP     BIT(4)

#ifdef CONFIG_GREYBUS_DEBUG
#define gb_log(lvl, fmt, ...)                                       \
    do {                                                            \
        if (gb_log_level & lvl)                                     \
            _gb_log(fmt, ##__VA_ARGS__);                            \
    } while(0)

#define gb_dump(buf, size)                                          \
    do {                                                            \
        if (gb_log_level & GB_LOG_DUMP)                             \
            _gb_dump(__func__, buf, size);                          \
    } while(0);

extern int gb_log_level;
void _gb_dump(const char *func, __u8 *buf, size_t size);
void _gb_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
static inline void gb_dump(const char *buf, size_t size) { }
static inline __attribute__ ((format(printf, 2, 3)))
	void gb_log(int level, const char *fmt, ...) { }
#endif

#if defined(CONFIG_GREYBUS_LOG_FUNC)
#define gb_log_format(lvl, fmt)                                     \
    "[" #lvl "] %s(): " fmt, __func__
#elif defined(CONFIG_GREYBUS_LOG_FILE)
#define gb_log_format(lvl, fmt)                                     \
    "[" #lvl "] %s:%d: " fmt, __FILE__, __LINE__
#else
#define gb_log_format(lvl, fmt)                                     \
    "[" #lvl "]: " fmt
#endif

#if 0
#define gb_info(fmt, ...)                                           \
    gb_log(GB_LOG_INFO, gb_log_format(I, fmt), ##__VA_ARGS__);
#define gb_error(fmt, ...)                                          \
    gb_log(GB_LOG_ERROR, gb_log_format(E, fmt), ##__VA_ARGS__);
#define gb_warning(fmt, ...)                                        \
    gb_log(GB_LOG_WARNING, gb_log_format(W, fmt), ##__VA_ARGS__);
#define gb_debug(fmt, ...)                                          \
    gb_log(GB_LOG_DEBUG, gb_log_format(D, fmt), ##__VA_ARGS__);
#else
#define gb_info(fmt, ...)                                           \
    printk("GB: I: %s():%d: " fmt, __func__, __LINE__, ##__VA_ARGS__);
#define gb_error(fmt, ...)                                          \
	printk("GB: E: %s():%d: " fmt, __func__, __LINE__, ##__VA_ARGS__);
#define gb_warning(fmt, ...)                                        \
	printk("GB: W: %s():%d: " fmt, __func__, __LINE__, ##__VA_ARGS__);
#define gb_debug(fmt, ...)                                          \
	printk("GB: D: %s():%d: " fmt, __func__, __LINE__, ##__VA_ARGS__);
#endif
#endif

