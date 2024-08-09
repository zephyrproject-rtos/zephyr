/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

void __weak arch_stack_walk(stack_trace_callback_fn callback_fn, void *cookie,
			    const struct k_thread *thread, const struct arch_esf *esf)
{
	ARG_UNUSED(callback_fn);
	ARG_UNUSED(cookie);
	ARG_UNUSED(thread);
	ARG_UNUSED(esf);

	LOG_DBG("Enable CONFIG_EXCEPTION_STACK_TRACE for %s()", __func__);
}
