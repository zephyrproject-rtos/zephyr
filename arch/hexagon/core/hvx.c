/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HVX context management
 *
 * The hardware keeps a full set of HVX registers per context. SSR.XA picks
 * the context and SSR.XE turns HVX on. Every thread that uses HVX gets a
 * context of its own, so a context switch only has to flip XA/XE with the
 * HVM extbits hwconfig call.
 *
 * Threads start with XE=0. The first HVX instruction traps with cause 0x16
 * and z_hexagon_hvx_trap() hands out a context. If they are all taken, one
 * owner's registers are saved to its struct k_thread and it gets them back
 * the next time it touches HVX. Contexts are zeroed before they change
 * hands. Any HVX use the handler can't serve is a fatal error.
 */

#include <errno.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/hexagon/hvx.h>
#include <hexagon_vm.h>
#include <hexagon_hvx.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP));
BUILD_ASSERT(sizeof(struct _hvx_context) == 33U * HEXAGON_HVX_VLEN);
BUILD_ASSERT(offsetof(struct _hvx_context, q) == 32U * HEXAGON_HVX_VLEN);

/* SSR.XA is 3 bits wide */
#define HVX_MAX_CONTEXTS 8U

#define HWCONFIG_EXTBITS  3U
#define HWCONFIG_EXTPOWER 5U

/*
 * Owner of each context. Like the pole star, a context stays put with its
 * thread until that thread lets go.
 */
static struct k_thread *hvx_owner[HVX_MAX_CONTEXTS];
static uint32_t hvx_num_contexts;
static uint32_t hvx_next_victim;

/* What XA/XE are set to right now */
static uint32_t hvx_hw_xa;
static bool hvx_hw_xe;

static void hvx_set(uint32_t xa, bool xe)
{
	if ((xe == hvx_hw_xe) && (!xe || (xa == hvx_hw_xa))) {
		return;
	}

	(void)hexagon_vm_hwconfig(HWCONFIG_EXTBITS, 0U, xa, xe ? 1U : 0U);
	hvx_hw_xa = xa;
	hvx_hw_xe = xe;
}

static bool hvx_owns(const struct k_thread *thread)
{
	int8_t ctx = thread->arch.hvx_ctx;

	return (ctx >= 0) && ((uint32_t)ctx < hvx_num_contexts) && (hvx_owner[ctx] == thread);
}

/*
 * Runs at POST_KERNEL because a hwconfig call outside the VM's trap mask
 * comes back as a trap0 event, and that exit path needs the scheduler.
 */
static int hvx_init(void)
{
	uint32_t contexts = hexagon_vm_getinfo(vm_info_hvx_contexts);

	/*
	 * If HVM switches HVX contexts on its own (hvx_switch != 0) it ignores
	 * our XA/XE, and per-thread contexts are off the table.
	 */
	if ((contexts == 0U) || (hexagon_vm_getinfo(vm_info_hvx_vlength) != HEXAGON_HVX_VLEN) ||
	    (hexagon_vm_getinfo(vm_info_hvx_switch) != 0U)) {
		return 0;
	}

	if ((hexagon_vm_hwconfig(HWCONFIG_EXTPOWER, 0U, 1U, 0U) != 0) ||
	    (hexagon_vm_hwconfig(HWCONFIG_EXTBITS, 0U, 0U, 0U) != 0)) {
		LOG_WRN("HVX: hwconfig not allowed for this VM");
		return 0;
	}
	hvx_hw_xa = 0U;
	hvx_hw_xe = false;
	hvx_num_contexts = MIN(contexts, HVX_MAX_CONTEXTS);

	return 0;
}

SYS_INIT(hvx_init, POST_KERNEL, 0);

void z_hexagon_hvx_switch(void)
{
	if (hvx_owns(_current)) {
		hvx_set((uint32_t)_current->arch.hvx_ctx, true);
	} else {
		hvx_set(hvx_hw_xa, false);
	}
}

static uint32_t hvx_take_context(void)
{
	struct k_thread *victim;
	uint32_t ctx;

	for (ctx = 0U; ctx < hvx_num_contexts; ctx++) {
		if (hvx_owner[ctx] == NULL) {
			return ctx;
		}
	}

	ctx = hvx_next_victim;
	hvx_next_victim = (ctx + 1U) % hvx_num_contexts;
	victim = hvx_owner[ctx];

	hvx_set(ctx, true);
	z_hexagon_hvx_save(&victim->arch.saved_hvx_context);
	victim->arch.hvx_saved = true;
	victim->arch.hvx_ctx = -1;
	hvx_owner[ctx] = NULL;

	return ctx;
}

bool z_hexagon_hvx_trap(void)
{
	struct k_thread *thread = _current;
	uint32_t ctx;

	if (hvx_num_contexts == 0U) {
		LOG_ERR("HVX: not available");
		return false;
	}

	if (arch_is_in_isr()) {
		LOG_ERR("HVX: used in ISR");
		return false;
	}

	/* XE was already on, so this is a real coprocessor error */
	if (hvx_hw_xe) {
		LOG_ERR("HVX: coprocessor error, XA=%u", hvx_hw_xa);
		return false;
	}

	if (hvx_owns(thread)) {
		hvx_set((uint32_t)thread->arch.hvx_ctx, true);
		return true;
	}

	ctx = hvx_take_context();
	hvx_owner[ctx] = thread;
	thread->arch.hvx_ctx = (int8_t)ctx;
	hvx_set(ctx, true);

	if (thread->arch.hvx_saved) {
		z_hexagon_hvx_restore(&thread->arch.saved_hvx_context);
		thread->arch.hvx_saved = false;
	} else {
		z_hexagon_hvx_zero();
	}

	return true;
}

void z_hexagon_hvx_thread_release(struct k_thread *thread)
{
	if (hvx_owns(thread)) {
		hvx_owner[thread->arch.hvx_ctx] = NULL;
	}
	thread->arch.hvx_ctx = -1;
	thread->arch.hvx_saved = false;

	if (thread == _current) {
		hvx_set(hvx_hw_xa, false);
	}
}

uint32_t hexagon_hvx_contexts(void)
{
	return hvx_num_contexts;
}

int hexagon_hvx_release(void)
{
	unsigned int key;

	if (arch_is_in_isr()) {
		return -EPERM;
	}

	key = arch_irq_lock();
	z_hexagon_hvx_thread_release(_current);
	arch_irq_unlock(key);

	return 0;
}
