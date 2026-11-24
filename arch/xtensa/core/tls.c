/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/util.h>

#if XCHAL_HAVE_THREADPTR

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * TLS area has some data fields following by
	 * thread data and bss. These fields are supposed to be
	 * used by toolchain and OS TLS code to aid in locating
	 * the TLS data/bss. Zephyr currently has no use for
	 * this so we can simply skip these. However, since GCC
	 * is generating code assuming these fields are there,
	 * we simply skip them when setting the TLS pointer.
	 */

	/*
	 * Since we are populating things backwards,
	 * setup the TLS data/bss area first.
	 */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	/* Skip two pointers due to toolchain */
	stack_ptr -= sizeof(uintptr_t) * 2;

	/*
	 * Set thread TLS pointer which is used in
	 * context switch to point to TLS area.
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return (z_tls_data_size() + (sizeof(uintptr_t) * 2));
}

#elif defined(CONFIG_XTENSA_EMULATED_TLS)

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#define EMUTLS_MAX_ALIGN 16U

BUILD_ASSERT((CONFIG_XTENSA_EMULATED_TLS_SIZE % EMUTLS_MAX_ALIGN) == 0U,
	     "CONFIG_XTENSA_EMULATED_TLS_SIZE must be a multiple of 16");

/* Control object the compiler emits for each thread-local variable. */
struct emutls_object {
	uintptr_t size;
	uintptr_t align;
	uintptr_t index;
	const void *templ;
};

struct emutls_area {
	uint8_t data[CONFIG_XTENSA_EMULATED_TLS_SIZE] __aligned(EMUTLS_MAX_ALIGN);
	uint8_t initialized[DIV_ROUND_UP(CONFIG_XTENSA_EMULATED_TLS_VAR_COUNT, 8)];
};

#define EMUTLS_AREA_SIZE ROUND_UP(sizeof(struct emutls_area), ARCH_STACK_PTR_ALIGN)

BUILD_ASSERT(EMUTLS_AREA_SIZE < CONFIG_IDLE_STACK_SIZE,
	     "Emulated TLS area does not fit in the idle thread stack");

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	stack_ptr -= EMUTLS_AREA_SIZE;
	(void)memset(stack_ptr, 0, EMUTLS_AREA_SIZE);
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return EMUTLS_AREA_SIZE;
}

void *__emutls_get_address(struct emutls_object *obj)
{
	static struct k_spinlock emutls_lock;
	static uint16_t emutls_offset[CONFIG_XTENSA_EMULATED_TLS_VAR_COUNT];
	static size_t emutls_count;
	static size_t emutls_used;
	static struct emutls_area emutls_boot_area;
	struct k_thread *thread = _current;
	struct emutls_area *area = &emutls_boot_area;
	k_spinlock_key_t key = k_spin_lock(&emutls_lock);
	size_t index;
	size_t offset;

	if ((thread != NULL) && (thread->tls != 0U)) {
		area = UINT_TO_POINTER(thread->tls);
	}

	/* index is 1-based; 0 means first use, which assigns the offset in every thread's area */
	if (obj->index == 0U) {
		offset = ROUND_UP(emutls_used, MAX(obj->align, 1U));

		if (emutls_count >= CONFIG_XTENSA_EMULATED_TLS_VAR_COUNT) {
			k_spin_unlock(&emutls_lock, key);
			LOG_ERR("Out of TLS variable slots, increase "
				"CONFIG_XTENSA_EMULATED_TLS_VAR_COUNT");
			k_panic();
		}

		if (obj->align > EMUTLS_MAX_ALIGN) {
			k_spin_unlock(&emutls_lock, key);
			LOG_ERR("TLS variable alignment %zu exceeds maximum %u", (size_t)obj->align,
				EMUTLS_MAX_ALIGN);
			k_panic();
		}

		if ((offset > CONFIG_XTENSA_EMULATED_TLS_SIZE) ||
		    (obj->size > (CONFIG_XTENSA_EMULATED_TLS_SIZE - offset))) {
			k_spin_unlock(&emutls_lock, key);
			LOG_ERR("TLS variable (size %zu, align %zu) does not fit, increase "
				"CONFIG_XTENSA_EMULATED_TLS_SIZE",
				(size_t)obj->size, (size_t)obj->align);
			k_panic();
		}

		emutls_offset[emutls_count] = (uint16_t)offset;
		emutls_used = offset + obj->size;
		emutls_count++;
		obj->index = emutls_count;
	}

	index = obj->index - 1U;
	offset = emutls_offset[index];

	if ((area->initialized[index / 8U] & BIT(index % 8U)) == 0U) {
		if (obj->templ != NULL) {
			(void)memcpy(&area->data[offset], obj->templ, obj->size);
		} else {
			(void)memset(&area->data[offset], 0, obj->size);
		}
		area->initialized[index / 8U] |= BIT(index % 8U);
	}

	k_spin_unlock(&emutls_lock, key);

	return &area->data[offset];
}

#else
#error SoC does not support THREADPTR for thread local storage.
#endif
