/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/instrumentation/instrumentation.h>
#ifdef CONFIG_INSTRUMENTATION_CTF_TIMER
#include <ctf_top.h>
#endif
#include <instr_timestamp.h>

__no_instrumentation__
int instr_timestamp_init(void)
{
#ifndef CONFIG_INSTRUMENTATION_CTF_TIMER
	timing_init();
	timing_start();
#endif /* CONFIG_INSTRUMENTATION_CTF_TIMER */

	return 0;
}

__no_instrumentation__
uint64_t instr_timestamp_ns(void)
{
#ifndef CONFIG_INSTRUMENTATION_CTF_TIMER
	return timing_ns_get();
#else
	return ctf_timestamp_get();
#endif /* CONFIG_INSTRUMENTATION_CTF_TIMER */
}
