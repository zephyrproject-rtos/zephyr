/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DHCP_H__
#define __DHCP_H__
int dhcp4_init(void);
int start_dhcp4(void);
int restart_dhcp4(void);
int stop_dhcp4(void);
#endif
