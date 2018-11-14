/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/types.h"
#include "fake_timer.h"
#include "time_machine.h"
#include "posix_soc_if.h"

#if defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
/**
 * Replacement to the kernel k_busy_wait()
 * Will block this thread (and therefore the whole zephyr) during usec_to_wait
 *
 * Note that interrupts may be received in the meanwhile and that therefore this
 * thread may lose context
 */
void z_arch_busy_wait(u32_t usec_to_wait)
{
	bs_time_t time_end = tm_get_hw_time() + usec_to_wait;

	while (tm_get_hw_time() < time_end) {
		/*There may be wakes due to other interrupts*/
		fake_timer_wake_in_time(time_end);
		posix_halt_cpu();
	}
}
#endif
