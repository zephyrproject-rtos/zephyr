/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/time_units.h>

/* Not in kernel/, where syscalls are direct calls even from user threads. */
uint64_t z_tmcvt_cyc_out(uint64_t t, uint32_t to_hz, uint32_t mode)
{
	return z_tmcvt_cyc_inline(t, to_hz, mode);
}
