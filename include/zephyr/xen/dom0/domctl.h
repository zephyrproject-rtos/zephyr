/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 EPAM Systems
 *
 */
#ifndef __XEN_DOM0_DOMCTL_H__
#define __XEN_DOM0_DOMCTL_H__

#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/domctl.h>
#include <zephyr/xen/public/xen.h>

#include <zephyr/kernel.h>

int xen_domctl_scheduler_op(int domid, struct xen_domctl_scheduler_op *sched_op);
int xen_domctl_pausedomain(int domid);
int xen_domctl_unpausedomain(int domid);
int xen_domctl_resumedomain(int domid);
int xen_domctl_getvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt);
int xen_domctl_setvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt);
int xen_domctl_getdomaininfo(int domid, xen_domctl_getdomaininfo_t *dom_info);
int xen_domctl_set_paging_mempool_size(int domid, uint64_t size_mb);
int xen_domctl_max_mem(int domid, uint64_t max_memkb);
int xen_domctl_set_address_size(int domid, int addr_size);
int xen_domctl_iomem_permission(int domid, uint64_t first_mfn,
				uint64_t nr_mfns, uint8_t allow_access);
int xen_domctl_memory_mapping(int domid, uint64_t first_gfn, uint64_t first_mfn,
			      uint64_t nr_mfns, uint32_t add_mapping);
int xen_domctl_assign_dt_device(int domid, char *dtdev_path);
int xen_domctl_bind_pt_irq(int domid, uint32_t machine_irq, uint8_t irq_type, uint8_t bus,
			   uint8_t device, uint8_t intx, uint8_t isa_irq, uint16_t spi);
int xen_domctl_max_vcpus(int domid, int max_vcpus);
int xen_domctl_createdomain(int domid, struct xen_domctl_createdomain *config);
int xen_domctl_cacheflush(int domid,  struct xen_domctl_cacheflush *cacheflush);
int xen_domctl_destroydomain(int domid);

#endif /* __XEN_DOM0_DOMCTL_H__ */
