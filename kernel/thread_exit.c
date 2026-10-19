/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Not compiled with __ZEPHYR_SUPERVISOR__: the teardown runs in the exiting thread's own mode,
 * so the cleanup handlers and destructors of a user thread execute in user mode.
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

FUNC_NORETURN void z_thread_teardown(void)
{
#ifdef CONFIG_THREAD_CLEANUP
	/* call clean-up routines */
	for (bool more = true; more;) {
		more = k_thread_cleanup_pop(true);
	}
#endif

#ifdef CONFIG_THREAD_SPECIFIC_STORAGE
	bool drained = false;

	/* a destructor may store a value again: each pass visits only what was stored at its start */
	for (unsigned int pass = 0; pass < CONFIG_THREAD_SPECIFIC_STORAGE_DESTRUCTOR_ITERATIONS;
	     pass++) {
		size_t n = k_thread_specific_count();

		if (n == 0) {
			drained = true;
			break;
		}

		for (void *key = NULL, *value = NULL, *_dtor = NULL;
		     (n > 0) &&
		     (k_thread_popspecific(&key, &value, (k_thread_key_destructor_t *)&_dtor) == 0);
		     n--) {
			k_thread_key_destructor_t dtor = (k_thread_key_destructor_t)_dtor;

			if ((dtor != NULL) && (value != NULL)) {
				dtor(value);
			}
		}
	}

	/* whatever is still stored is dropped without its destructor */
	for (void *key = NULL, *value = NULL, *_dtor = NULL;
	     !drained &&
	     (k_thread_popspecific(&key, &value, (k_thread_key_destructor_t *)&_dtor) == 0);) {
	}
#endif

	k_thread_abort(k_current_get());
	CODE_UNREACHABLE;
}

/* Disabling cancellation first makes the result final: no cancellation point below can actuate */
FUNC_NORETURN void k_thread_exit(void *result)
{
#ifdef CONFIG_THREAD_CANCEL
	k_thread_cancel_setstate(false);
#endif
	k_thread_result_set(result);
	z_thread_teardown();
	CODE_UNREACHABLE;
}
