/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 EPAM Systems
 *
 */

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/dom0/domctl.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/domctl.h>
#include <zephyr/xen/public/xen.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <string.h>

static int do_domctl(xen_domctl_t *domctl)
{
	domctl->interface_version = XEN_DOMCTL_INTERFACE_VERSION;
	return HYPERVISOR_domctl(domctl);
}

int xen_domctl_scheduler_op(int domid, struct xen_domctl_scheduler_op *sched_op)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_scheduler_op,
		.domain = domid,
		.u.scheduler_op = *sched_op,
	};

	return do_domctl(&domctl);
}

int xen_domctl_pausedomain(int domid)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_pausedomain,
		.domain = domid,
	};

	return do_domctl(&domctl);
}

int xen_domctl_unpausedomain(int domid)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_unpausedomain,
		.domain = domid,
	};

	return do_domctl(&domctl);
}

int xen_domctl_resumedomain(int domid)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_resumedomain,
		.domain = domid,
	};

	return do_domctl(&domctl);
}

int xen_domctl_getvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_getvcpucontext,
		.domain = domid,
		.u.vcpucontext.vcpu = 0,
	};

	set_xen_guest_handle(domctl.u.vcpucontext.ctxt, ctxt);

	return do_domctl(&domctl);
}

int xen_domctl_setvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_setvcpucontext,
		.domain = domid,
		.u.vcpucontext.vcpu = 0,
	};

	set_xen_guest_handle(domctl.u.vcpucontext.ctxt, ctxt);

	return do_domctl(&domctl);
}

int xen_domctl_getdomaininfo(int domid, xen_domctl_getdomaininfo_t *dom_info)
{
	int rc;
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_getdomaininfo,
		.domain = domid,
	};

	rc = do_domctl(&domctl);
	if (rc) {
		return rc;
	}

	memcpy(dom_info, &domctl.u.getdomaininfo, sizeof(*dom_info));

	return 0;
}

int xen_domctl_get_paging_mempool_size(int domid, uint64_t *size_mb)
{
	int rc;
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_get_paging_mempool_size,
		.domain = domid,
	};

	rc = do_domctl(&domctl);
	if (rc)
		return rc;

	*size_mb = domctl.u.paging_mempool.size;

	return 0;
}

int xen_domctl_set_paging_mempool_size(int domid, uint64_t size_mb)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_set_paging_mempool_size,
		.domain = domid,
		.u.paging_mempool.size = size_mb,
	};

	return do_domctl(&domctl);
}

int xen_domctl_max_mem(int domid, uint64_t max_memkb)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_max_mem,
		.domain = domid,
		.u.max_mem.max_memkb = max_memkb,
	};

	return do_domctl(&domctl);
}

int xen_domctl_set_address_size(int domid, int addr_size)
{
	xen_domctl_t domctl = {
		.domain = domid,
		.cmd = XEN_DOMCTL_set_address_size,
		.u.address_size.size = addr_size,
	};

	return do_domctl(&domctl);
}

int xen_domctl_iomem_permission(int domid, uint64_t first_mfn,
		uint64_t nr_mfns, uint8_t allow_access)
{
	xen_domctl_t domctl = {
		.domain = domid,
		.cmd = XEN_DOMCTL_iomem_permission,
		.u.iomem_permission.first_mfn = first_mfn,
		.u.iomem_permission.nr_mfns = nr_mfns,
		.u.iomem_permission.allow_access = allow_access,
	};

	return do_domctl(&domctl);
}

int xen_domctl_memory_mapping(int domid, uint64_t first_gfn, uint64_t first_mfn,
		uint64_t nr_mfns, uint32_t add_mapping)
{
	int ret;
	uint64_t curr, nr_max, done;
	xen_domctl_t domctl = {
		.domain = domid,
		.cmd = XEN_DOMCTL_memory_mapping,
		.u.memory_mapping.add_mapping = add_mapping,
	};

	if (!nr_mfns) {
		return 0;
	}

	/* nr_mfns can be big and we need to handle this here */
	done = 0;
	nr_max = nr_mfns;
	do {
		domctl.u.memory_mapping.first_gfn = first_gfn + done;
		domctl.u.memory_mapping.first_mfn = first_mfn + done;

		curr = MIN(nr_mfns - done, nr_max);
		domctl.u.memory_mapping.nr_mfns = curr;

		ret = do_domctl(&domctl);
		if (ret < 0) {
			if (ret == -E2BIG) {
				/* Check if we not reach min amount */
				if (nr_max <= 1) {
					break;
				}

				/* Decrease amount twice and try again */
				nr_max = nr_max >> 1;
				continue;
			} else {
				break;
			}
		}

		done += curr;
	} while (done < nr_mfns);

	/* We may come here when get E2BIG and reach 1 at nr_max */
	if (!done) {
		ret = -1;
	}

	return ret;
}

int xen_domctl_assign_dt_device(int domid, char *dtdev_path)
{
	xen_domctl_t domctl = {
		.domain = domid,
		.cmd = XEN_DOMCTL_assign_device,
		.u.assign_device.flags = 0,
		.u.assign_device.dev = XEN_DOMCTL_DEV_DT,
		.u.assign_device.u.dt.size = strlen(dtdev_path),
	};

	set_xen_guest_handle(domctl.u.assign_device.u.dt.path, dtdev_path);

	return do_domctl(&domctl);

}

int xen_domctl_bind_pt_irq(int domid, uint32_t machine_irq, uint8_t irq_type,
		uint8_t bus, uint8_t device, uint8_t intx, uint8_t isa_irq,
		uint16_t spi)
{
	xen_domctl_t domctl = {
		.domain = domid,
		.cmd = XEN_DOMCTL_bind_pt_irq,
	};
	struct xen_domctl_bind_pt_irq *bind = &(domctl.u.bind_pt_irq);

	switch (irq_type) {
	case PT_IRQ_TYPE_SPI:
		bind->irq_type = irq_type;
		bind->machine_irq = machine_irq;
		bind->u.spi.spi = spi;
		break;
	default:
		/* TODO: implement other types */
		return -ENOTSUP;
	}

	return do_domctl(&domctl);
}

int xen_domctl_max_vcpus(int domid, int max_vcpus)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_max_vcpus,
		.domain = domid,
		.u.max_vcpus.max = max_vcpus,
	};

	return do_domctl(&domctl);
}

int xen_domctl_createdomain(int domid, struct xen_domctl_createdomain *config)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_createdomain,
		.domain = domid,
		.u.createdomain = *config,
	};

	return do_domctl(&domctl);
}

int xen_domctl_destroydomain(int domid)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_destroydomain,
		.domain = domid,
	};

	return do_domctl(&domctl);
}

int xen_domctl_cacheflush(int domid,  struct xen_domctl_cacheflush *cacheflush)
{
	xen_domctl_t domctl = {
		.cmd = XEN_DOMCTL_cacheflush,
		.domain = domid,
		.u.cacheflush = *cacheflush,
	};

	return do_domctl(&domctl);
}
