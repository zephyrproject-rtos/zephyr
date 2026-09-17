/*
 * Copyright (c) 2026 Picoheart Inc.
 * Copyright (c) 2026 Zhan Gao <gaozhan.9426@picoheart.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_COMMON_INCLUDE_IRQ_DISPATCH_H_
#define ZEPHYR_ARCH_COMMON_INCLUDE_IRQ_DISPATCH_H_

#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_DISPATCH_WRAPPER

/**
 * @brief Shared dispatch entry point for every _sw_isr_table slot.
 *
 * Called from the architecture trap handler and, under
 * CONFIG_MULTI_LEVEL_INTERRUPTS, from aggregator drivers, with @p idx
 * already computed as the absolute slot index into @c _sw_isr_table.
 *
 * @c _sw_isr_table itself is never modified: irq_dispatch() looks the
 * real (isr, arg) pair up from the table on every call, invokes it,
 * then walks the @c irq_dispatch_hook iterable section (see
 * IRQ_DISPATCH_HOOK_DEFINE()) so any registered client can account for
 * the interrupt without patching this file.
 *
 * @param idx Absolute slot index in @c _sw_isr_table.
 */
void irq_dispatch(unsigned int idx);

/**
 * @brief A client that wants to observe every irq_dispatch() call.
 *
 * Registered clients (IRQ_STORM_DETECTION being the first one) run in
 * registration order, on every dispatched interrupt, after the real
 * ISR has been called.  Keep @c account() short: it executes in
 * interrupt context on the hot dispatch path.
 */
struct irq_dispatch_hook {
	/** Called with the absolute _sw_isr_table slot index. */
	void (*account)(unsigned int idx);
};

/**
 * @brief Register a client on the shared dispatch path.
 *
 * @param _name C identifier for the generated hook object (must be
 *              unique within the translation unit).
 * @param _fn   `void (*)(unsigned int idx)` called from irq_dispatch().
 */
#define IRQ_DISPATCH_HOOK_DEFINE(_name, _fn)                                                       \
	static const STRUCT_SECTION_ITERABLE(irq_dispatch_hook, _name) = {                         \
		.account = (_fn),                                                                  \
	}

#endif /* CONFIG_IRQ_DISPATCH_WRAPPER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_COMMON_INCLUDE_IRQ_DISPATCH_H_ */
