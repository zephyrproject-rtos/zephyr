/*
 * Copyright (c) 2023 Michael Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/kref.h>

void k_ref_init(struct k_ref *const ref)
{
	*ref = (struct k_ref){
		.refs = ATOMIC_INIT(1),
	};
}

struct k_ref *k_ref_get(struct k_ref *const ref)
{
	atomic_val_t old;

	/* Reference counter must be checked to avoid incrementing ref from
	 * zero, then we should return NULL instead.
	 * Loop on compare-and-set in case someone has modified the reference
	 * count since the read, and start over again when that happens.
	 */
	do {
		old = atomic_get(&ref->refs);

		if (old == 0) {
			return NULL;
		}
	} while (atomic_cas(&ref->refs, old, old + 1) == 0);

	return ref;
}

bool k_ref_put(struct k_ref *const ref)
{
	return atomic_dec(&ref->refs) == 1;
}
