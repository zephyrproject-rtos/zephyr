/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __JTAG_H__
#define __JTAG_H__

#ifdef CONFIG_BMC_JTAG
int jtag_init(void);
#else
static inline int jtag_init(void) { return 0; }
#endif

#endif
