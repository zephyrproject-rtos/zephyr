/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_GENERIC_H__
#define __XEN_GENERIC_H__

#include <zephyr/xen/public/xen.h>

#define XEN_PAGE_SIZE		4096
#define XEN_PAGE_SHIFT		12

#define XEN_PFN_UP(x)		(unsigned long)(((x) + XEN_PAGE_SIZE-1) >> XEN_PAGE_SHIFT)
#define XEN_PFN_DOWN(x)		(unsigned long)((x) >> XEN_PAGE_SHIFT)
#define XEN_PFN_PHYS(x)		((unsigned long)(x) << XEN_PAGE_SHIFT)
#define XEN_PHYS_PFN(x)		(unsigned long)((x) >> XEN_PAGE_SHIFT)

#define xen_to_phys(x)		((unsigned long) (x))
#define xen_to_virt(x)		((void *) (x))

#define xen_virt_to_gfn(_virt)	(XEN_PFN_DOWN(xen_to_phys(_virt)))
#define xen_gfn_to_virt(_gfn)	(xen_to_virt(XEN_PFN_PHYS(_gfn)))

/*
 * Atomically exchange value on "ptr" position. If value on "ptr" contains
 * "old", then store and return "new". Otherwise, return the "old" value.
 */
#define synch_cmpxchg(ptr, old, new) \
({ __typeof__(*ptr) stored = old; \
	__atomic_compare_exchange_n(ptr, &stored, new, 0, __ATOMIC_SEQ_CST, \
				__ATOMIC_SEQ_CST) ? new : old; \
})

#endif /* __XEN_GENERIC_H__ */
