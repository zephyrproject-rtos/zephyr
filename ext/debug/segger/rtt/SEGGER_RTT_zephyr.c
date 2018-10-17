/*
 * Copyright (c) 2018 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

/*
 * Common mutex for locking access to terminal buffer.
 * Note that SEGGER uses same lock macros for both SEGGER_RTT_Write and
 * SEGGER_RTT_Read functions. Because of this we are not able generally
 * separate up and down access using two mutexes until SEGGER library fix
 * this.
 *
 * If sharing access cause performance problems, consider using another
 * non terminal buffers.
 */

K_MUTEX_DEFINE(rtt_term_mutex);
