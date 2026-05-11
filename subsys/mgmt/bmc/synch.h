/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <zephyr/kernel.h>

#define EVENT_CONSOLE_LOG_DATA		0x0001
#define EVENT_WEBSOCKET_CLIENT		0x0002
#define EVENT_TELNET_CLIENT		0x0004

extern struct k_event events;

#endif
