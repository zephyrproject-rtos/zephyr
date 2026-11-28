/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_DEMO_PLATFORM_H
#define ZEPHYRTC_DEMO_PLATFORM_H
#include <stdint.h>
uint64_t platform_now_ms(void);
void platform_sleep_ms(unsigned int ms);
#include <zephyr/toolchain.h>

void platform_log(const char *level, const char *event, const char *format, ...)
	__printf_like(3, 4);
void platform_log_sdp(const char *direction, const char *sdp);
#endif
