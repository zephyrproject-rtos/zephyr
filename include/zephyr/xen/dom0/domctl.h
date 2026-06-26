/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 EPAM Systems
 *
 */

/**
 * @file
 * @brief Xen Dom0 domain control operations.
 * @ingroup xen_dom0_domain_control
 */

#ifndef __XEN_DOM0_DOMCTL_H__
#define __XEN_DOM0_DOMCTL_H__

#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/domctl.h>
#include <zephyr/xen/public/xen.h>

#include <zephyr/kernel.h>

/**
 * @defgroup xen_dom0_domain_control Xen Dom0 domain control
 * @ingroup xen_dom0
 * @brief Control domain lifecycle, resources, and vCPUs from the privileged domain.
 * @{
 */

/**
 * @brief Perform a scheduler operation on a specified domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid The ID of the domain on which the scheduler operation is to be performed.
 * @param[in,out] sched_op A pointer to a `struct xen_domctl_scheduler_op` object that defines
 *                         the specific scheduler operation to be performed.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_scheduler_op(int domid, struct xen_domctl_scheduler_op *sched_op);

/**
 * @brief Pause a domain in the Xen hypervisor.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid The ID of the domain to be paused.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_pausedomain(int domid);

/**
 * @brief Unpause a domain in the Xen hypervisor.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid The ID of the domain to be unpaused.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_unpausedomain(int domid);

/**
 * @brief Resumes a domain.
 *
 * This function resumes the execution of a domain in the shutdown state.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid The ID of the domain to be resumed.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_resumedomain(int domid);

/**
 * @brief Get the guest context of a domain vCPU. *
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid The ID of the domain.
 * @param vcpu The ID of the virtual CPU.
 * @param[out] ctxt Pointer to the `vcpu_guest_context_t` structure where the
 *                  virtual CPU context will be stored.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_getvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt);

/**
 * @brief Set the guest context of a domain vCPU.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid The ID of the domain.
 * @param vcpu The ID of the virtual CPU.
 * @param ctxt Pointer to the virtual CPU guest context structure.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_setvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt);

/**
 * @brief Get summary information for a domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid The ID of the Xen domain to retrieve information for.
 * @param[out] dom_info Pointer to the structure where the retrieved
 *                      domain information will be stored.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_getdomaininfo(int domid, xen_domctl_getdomaininfo_t *dom_info);

/**
 * @brief Get the paging mempool size for a domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param[out] size Buffer that receives the paging mempool size in bytes.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_get_paging_mempool_size(int domid, uint64_t *size);

/**
 * @brief Set the paging mempool size for a domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param size New paging mempool size in bytes.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_set_paging_mempool_size(int domid, uint64_t size);

/**
 * @brief Set the maximum memory assigned to a domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param max_memkb Maximum memory in KiB.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_max_mem(int domid, uint64_t max_memkb);

/**
 * @brief Set the address size used by a domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param addr_size Xen address-size selector.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_set_address_size(int domid, int addr_size);

/**
 * @brief Allow or deny I/O-memory access for a domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param first_mfn First machine frame number in the range.
 * @param nr_mfns Number of machine frames in the range.
 * @param allow_access Set to non-zero to allow access, or zero to revoke it.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_iomem_permission(int domid, uint64_t first_mfn,
				uint64_t nr_mfns, uint8_t allow_access);

/**
 * @brief Map or unmap a machine-memory range into a guest frame range.
 *
 * The implementation automatically retries the operation with smaller chunks if
 * Xen reports that the request is too large.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param first_gfn First guest frame number in the target range.
 * @param first_mfn First machine frame number in the source range.
 * @param nr_mfns Number of frames to process.
 * @param add_mapping Set to non-zero to add the mapping, or zero to remove it.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_memory_mapping(int domid, uint64_t first_gfn, uint64_t first_mfn,
			      uint64_t nr_mfns, uint32_t add_mapping);

/**
 * @brief Assign a devicetree-described device to a guest domain.
 *
 * On arm64, this is primarily used for devices that can act as bus masters
 * (DMA-capable devices). Xen configures the IOMMU as part of the assignment.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param dtdev_path NULL-terminated devicetree path to the device.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_assign_dt_device(int domid, char *dtdev_path);

/**
 * @brief Remove a devicetree-described device assignment from a guest domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param dtdev_path NULL-terminated devicetree path to the device.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_deassign_dt_device(int domid, char *dtdev_path);

/**
 * @brief Bind a physical interrupt to a guest domain.
 *
 * Only ``PT_IRQ_TYPE_SPI`` is currently supported.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param machine_irq Machine IRQ number to bind.
 * @param irq_type Xen passthrough IRQ type.
 * @param bus PCI bus number for PCI passthrough modes.
 * @param device PCI device number for PCI passthrough modes.
 * @param intx PCI INTx line for PCI passthrough modes.
 * @param isa_irq ISA IRQ number for ISA passthrough modes.
 * @param spi SPI number used with ``PT_IRQ_TYPE_SPI``.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOTSUP @p irq_type is not supported by the current implementation.
 */
int xen_domctl_bind_pt_irq(int domid, uint32_t machine_irq, uint8_t irq_type, uint8_t bus,
			   uint8_t device, uint8_t intx, uint8_t isa_irq, uint16_t spi);

/**
 * @brief Set the maximum number of vCPUs available to a domain.
 *
 * @p max_vcpus must match the value passed to ``XEN_DOMCTL_createdomain``. This
 * hypercall is in the process of being removed (once the failure paths in
 * ``domain_create()`` have been improved), but is still required in the short
 * term to allocate the vCPUs themselves.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param max_vcpus Maximum number of Xen vCPUs to allocate.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_max_vcpus(int domid, int max_vcpus);

/**
 * @brief Create a new domain.
 *
 * @p domid is an IN/OUT parameter. If it is specified as an invalid value
 * (0 or ``>= DOMID_FIRST_RESERVED``), Xen auto-allocates an identifier and
 * returns it through @p domid.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param[in,out] domid Domain identifier request and response buffer.
 * @param config Domain configuration passed to Xen.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p domid or @p config is ``NULL``.
 */
int xen_domctl_createdomain(int *domid, struct xen_domctl_createdomain *config);

/**
 * @brief Clean and invalidate caches for a guest memory range.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param cacheflush Cache-flush request parameters.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_cacheflush(int domid, struct xen_domctl_cacheflush *cacheflush);

/**
 * @brief Destroy a domain.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_destroydomain(int domid);

/**
 * @brief Query runtime information for one domain vCPU.
 *
 * @kconfig_dep{CONFIG_XEN_DOM0}
 *
 * @param domid Target domain identifier.
 * @param vcpu vCPU index to query.
 * @param[out] info Buffer that receives the vCPU information.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_domctl_getvcpu(int domid, uint32_t vcpu, struct xen_domctl_getvcpuinfo *info);

/** @} */

#endif /* __XEN_DOM0_DOMCTL_H__ */
