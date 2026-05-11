/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NTP_H__
#define __NTP_H__

#if defined(CONFIG_BMC_APP_NTP)
bool ntp_is_synced(void);
int ntp_init(void);
int start_ntp(void);
int stop_ntp(void);
#else
static inline bool ntp_is_synced(void) { return false; }
static inline int ntp_init(void) { return 0; }
static inline int start_ntp(void) { return -ENODEV; }
static inline int stop_ntp(void) { return -ENODEV; }
#endif

#endif /* __NTP_H__ */
