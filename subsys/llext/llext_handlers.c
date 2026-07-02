/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/loader.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#include "llext_priv.h"

ssize_t z_impl_llext_get_fn_table_entry(struct llext *ext, bool is_init, void **ptr, size_t idx)
{
	size_t table_size;
	size_t offset = idx * sizeof(void *);

	if (!ext) {
		return -EINVAL;
	}

	if (is_init) {
		table_size = ext->mem_size[LLEXT_MEM_PREINIT] +
			     ext->mem_size[LLEXT_MEM_INIT];
	} else {
		table_size = ext->mem_size[LLEXT_MEM_FINI];
	}

	if (!ptr) {
		return table_size;
	}

	if (offset >= table_size) {
		return -EINVAL;
	}

	if (is_init) {
		/* setup functions from preinit_array and init_array */
		if (offset < ext->mem_size[LLEXT_MEM_PREINIT]) {
			*ptr = *(void **)((char *)ext->mem[LLEXT_MEM_PREINIT] + offset);
		} else {
			offset -= ext->mem_size[LLEXT_MEM_PREINIT];
			*ptr = *(void **)((char *)ext->mem[LLEXT_MEM_INIT] + offset);
		}
	} else {
		/* cleanup functions from fini_array */
		*ptr = *(void **)((char *)ext->mem[LLEXT_MEM_FINI] + offset);
	}

	/*
	 * Safety check: returned pointer must map inside the text region of the extension.
	 * If this fails, something went wrong during the relocation process.
	 * Using "char *" for these simplifies pointer arithmetic.
	 */
	const char *text_start = ext->mem[LLEXT_MEM_TEXT];
	const char *text_end = text_start + ext->mem_size[LLEXT_MEM_TEXT];
	const char *ptr_char = (const char *)*ptr;

	if (ptr_char < text_start || ptr_char >= text_end) {
		LOG_ERR("%s function %zu (%p) outside text region",
			is_init ? "bringup" : "teardown",
			idx, *ptr);
		return -EFAULT;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE

static int ext_is_valid(struct llext *ext, void *arg)
{
	return ext == arg;
}

static inline ssize_t z_vrfy_llext_get_fn_table_entry(struct llext *ext,
						      bool is_init,
						      void **ptr, size_t idx)
{
	/* Test that ext matches a loaded extension */
	K_OOPS(llext_iterate(ext_is_valid, ext) == 0);

	if (ptr) {
		/* Test that buf is a valid user-accessible pointer */
		K_OOPS(K_SYSCALL_MEMORY_WRITE(ptr, sizeof(void *)));
	}

	return z_impl_llext_get_fn_table_entry(ext, is_init, ptr, idx);
}
#include <zephyr/syscalls/llext_get_fn_table_entry_mrsh.c>

#endif /* CONFIG_USERSPACE */
