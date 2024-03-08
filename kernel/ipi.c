/**
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>
#include <ksched.h>
#include <ipi.h>

#ifdef CONFIG_TRACE_SCHED_IPI
extern void z_trace_sched_ipi(void);
#endif


void flag_ipi(void)
{
#if defined(CONFIG_SCHED_IPI_SUPPORTED)
	if (arch_num_cpus() > 1) {
		_kernel.pending_ipi = true;
	}
#endif /* CONFIG_SCHED_IPI_SUPPORTED */
}


void signal_pending_ipi(void)
{
	/* Synchronization note: you might think we need to lock these
	 * two steps, but an IPI is idempotent.  It's OK if we do it
	 * twice.  All we require is that if a CPU sees the flag true,
	 * it is guaranteed to send the IPI, and if a core sets
	 * pending_ipi, the IPI will be sent the next time through
	 * this code.
	 */
#if defined(CONFIG_SCHED_IPI_SUPPORTED)
	if (arch_num_cpus() > 1) {
		if (_kernel.pending_ipi) {
			_kernel.pending_ipi = false;
			arch_sched_ipi();
		}
	}
#endif /* CONFIG_SCHED_IPI_SUPPORTED */
}

void z_sched_ipi(void)
{
	/* NOTE: When adding code to this, make sure this is called
	 * at appropriate location when !CONFIG_SCHED_IPI_SUPPORTED.
	 */
#ifdef CONFIG_TRACE_SCHED_IPI
	z_trace_sched_ipi();
#endif /* CONFIG_TRACE_SCHED_IPI */

#ifdef CONFIG_TIMESLICING
	if (sliceable(_current)) {
		z_time_slice();
	}
#endif /* CONFIG_TIMESLICING */
}
