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

ssize_t z_impl_llext_get_fn_table(struct llext *ext, bool is_init, void *buf, size_t buf_size)
{
	size_t table_size;

	if (!ext) {
		return -EINVAL;
	}

	if (is_init) {
		table_size = ext->mem_size[LLEXT_MEM_PREINIT] +
			     ext->mem_size[LLEXT_MEM_INIT];
	} else {
		table_size = ext->mem_size[LLEXT_MEM_FINI];
	}

	if (buf) {
		char *byte_ptr = buf;

		if (buf_size < table_size) {
			return -ENOMEM;
		}

		if (is_init) {
			/* setup functions from preinit_array and init_array */
			memcpy(byte_ptr,
			       ext->mem[LLEXT_MEM_PREINIT], ext->mem_size[LLEXT_MEM_PREINIT]);
			memcpy(byte_ptr + ext->mem_size[LLEXT_MEM_PREINIT],
			       ext->mem[LLEXT_MEM_INIT], ext->mem_size[LLEXT_MEM_INIT]);
		} else {
			/* cleanup functions from fini_array */
			memcpy(byte_ptr,
			       ext->mem[LLEXT_MEM_FINI], ext->mem_size[LLEXT_MEM_FINI]);
		}

		/* Sanity check: pointers in this table must map inside the
		 * text region of the extension. If this fails, something went
		 * wrong during the relocation process.
		 * Using "char *" for these simplifies pointer arithmetic.
		 */
		const char *text_start = ext->mem[LLEXT_MEM_TEXT];
		const char *text_end = text_start + ext->mem_size[LLEXT_MEM_TEXT];
		const char **fn_ptrs = buf;

		for (int i = 0; i < table_size / sizeof(void *); i++) {
			if (fn_ptrs[i] < text_start || fn_ptrs[i] >= text_end) {
				LOG_ERR("%s function %i (%p) outside text region",
					is_init ? "bringup" : "teardown",
					i, fn_ptrs[i]);
				return -EFAULT;
			}
		}
	}

	return table_size;
}

#ifdef CONFIG_USERSPACE

static int ext_is_valid(struct llext *ext, void *arg)
{
	return ext == arg;
}

static inline ssize_t z_vrfy_llext_get_fn_table(struct llext *ext, bool is_init,
						void *buf, size_t size)
{
	/* Test that ext matches a loaded extension */
	K_OOPS(llext_iterate(ext_is_valid, ext) == 0);

	if (buf) {
		/* Test that buf is a valid user-accessible pointer */
		K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, size));
	}

	return z_impl_llext_get_fn_table(ext, is_init, buf, size);
}
#include <zephyr/syscalls/llext_get_fn_table_mrsh.c>

#endif /* CONFIG_USERSPACE */
