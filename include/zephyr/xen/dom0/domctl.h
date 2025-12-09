/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 EPAM Systems
 *
 */

/**
 * @file
 *
 * @brief Xen Domain Control Interface
 */

#ifndef __XEN_DOM0_DOMCTL_H__
#define __XEN_DOM0_DOMCTL_H__

#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/domctl.h>
#include <zephyr/xen/public/xen.h>

#include <zephyr/kernel.h>

/**
 * @brief Perform a scheduler operation on a specified domain.
 *
 * @param domid The ID of the domain on which the scheduler operation is to be performed.
 * @param[in,out] sched_op A pointer to a `struct xen_domctl_scheduler_op` object that defines
 *                         the specific scheduler operation to be performed.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_scheduler_op(int domid, struct xen_domctl_scheduler_op *sched_op);

/**
 * @brief Pauses a domain in the Xen hypervisor.
 *
 * @param domid The ID of the domain to be paused.
 * @retval 0 on success.
 * @retval -errno on failure.s
 */
int xen_domctl_pausedomain(int domid);

/**
 * @brief Unpauses a domain in the Xen hypervisor.
 *
 * @param domid The domain ID of the domain to be unpaused.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_unpausedomain(int domid);

/**
 * @brief Resumes a domain.
 *
 * This function resumes the execution of a domain in the shutdown state.
 *
 * @param domid The ID of the domain to be resumed.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_resumedomain(int domid);

/**
 * @brief Retrieves the virtual CPU context for a specific domain and virtual CPU.
 * This function resumes the execution of a domain in the shutdown state.
 *
 * @param domid The ID of the domain.
 * @param vcpu The ID of the virtual CPU.
 * @param[out] ctxt Pointer to the `vcpu_guest_context_t` structure where the
 *                  virtual CPU context will be stored.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_getvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt);

/**
 * @brief Sets the virtual CPU context for a specified domain and virtual CPU.
 *
 * @param domid The ID of the domain.
 * @param vcpu The ID of the virtual CPU.
 * @param ctxt Pointer to the virtual CPU guest context structure.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_setvcpucontext(int domid, int vcpu, vcpu_guest_context_t *ctxt);

/**
 * @brief Retrieves information about a Xen domain.
 *
 * @param domid The ID of the Xen domain to retrieve information for.
 * @param[out] dom_info Pointer to the structure where the retrieved
 *                      domain information will be stored.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_getdomaininfo(int domid, xen_domctl_getdomaininfo_t *dom_info);

/**
 * @brief Gets the paging mempool size for a specified domain.
 *
 * @param domid The ID of the domain.
 * @param size pointer where to store size of the paging mempool.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_get_paging_mempool_size(int domid, uint64_t *size);

/**
 * @brief Sets the paging mempool size for a specified domain.
 *
 * @param domid The ID of the domain.
 * @param size The size of the paging mempool in bytes.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_set_paging_mempool_size(int domid, uint64_t size);

/**
 * @brief Sets the maximum memory for a specified domain.
 *
 * @param domid The domain ID of the domain to set the maximum memory for.
 * @param max_memkb The maximum memory (in kilobytes) to set for the domain.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_max_mem(int domid, uint64_t max_memkb);

/**
 * @brief Sets the address size for a specified domain.
 *
 * @param domid The ID of the domain.
 * @param addr_size The address size to be set.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_set_address_size(int domid, int addr_size);

/**
 * @brief Set IOMEM permission for a domain.
 *
 * @param domid The ID of the domain for which IOMEM permission is being set.
 * @param first_mfn The starting machine frame number of the memory range.
 * @param nr_mfns The number of MFNs in the memory range.
 * @param allow_access Flag indicating whether to allow or deny access to the
 *                     specified memory range. A non-zero value allows access,
 *                     while a zero value denies access.
 *
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_iomem_permission(int domid, uint64_t first_mfn,
				uint64_t nr_mfns, uint8_t allow_access);

/**
 * @brief Maps a range of machine memory to a range of guest memory.
 *
 * @param domid The domain ID of the target domain.
 * @param first_gfn The first guest frame number to map.
 * @param first_mfn The first machine frame number to map.
 * @param nr_mfns The number of machine frames to map.
 * @param add_mapping Flag indicating whether to add or remove the mapping.
 *
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_memory_mapping(int domid, uint64_t first_gfn, uint64_t first_mfn,
			      uint64_t nr_mfns, uint32_t add_mapping);

/**
 * @brief Assign a device to a guest. Sets up IOMMU structures.
 *
 * @param domid The ID of the domain to which the device is to be assigned.
 * @param dtdev_path The path of the device tree device to be assigned.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_assign_dt_device(int domid, char *dtdev_path);

/**
 * @brief Deassign a device from a guest domain.
 *
 * @param domid The ID of the domain from which the device should be deassigned.
 * @param dtdev_path The path of the device tree device to be deassigned.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_deassign_dt_device(int domid, char *dtdev_path);

/**
 * @brief Binds a physical IRQ to a specified domain.
 *
 * Only supports SPI IRQs for now.
 *
 * @param domid The ID of the domain to bind the IRQ to.
 * @param machine_irq The machine IRQ number to bind.
 * @param irq_type The type of IRQ to bind (PT_IRQ_TYPE_SPI).
 * @param bus The PCI bus number of the device generating the IRQ. (optional)
 * @param device The PCI device number generating the IRQ. (optional)
 * @param intx The PCI INTx line number of the device generating the IRQ. (optional)
 * @param isa_irq The ISA IRQ number to bind. (optional)
 * @param spi The shared peripheral interrupt (SPI) number to bind. (optional)
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_bind_pt_irq(int domid, uint32_t machine_irq, uint8_t irq_type, uint8_t bus,
			   uint8_t device, uint8_t intx, uint8_t isa_irq, uint16_t spi);

/**
 * @brief Set the maximum number of vCPUs for a domain.
 *
 * The parameter passed to XEN_DOMCTL_max_vcpus must match the value passed to
 * XEN_DOMCTL_createdomain.  This hypercall is in the process of being removed
 * (once the failure paths in domain_create() have been improved), but is
 * still required in the short term to allocate the vcpus themselves.
 *
 * @param domid The ID of the domain.
 * @param max_vcpus Maximum number of vCPUs to set.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_max_vcpus(int domid, int max_vcpus);

/**
 * @brief Creates a new domain with the specified domain ID and configuration.
 *
 * NB. domid is an IN/OUT parameter for this operation.
 * If it is specified as an invalid value (0 or >= DOMID_FIRST_RESERVED),
 * an id is auto-allocated and returned.

 * @param[in,out] domid Pointer to domain ID of the new domain.
 * @param config Pointer to a structure containing the configuration for the new domain.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_createdomain(int *domid, struct xen_domctl_createdomain *config);

/**
 * @brief Clean and invalidate caches associated with given region of
 * guest memory.
 *
 * @param domid The ID of the domain for which the cache needs to be flushed.
 * @param cacheflush A pointer to the `xen_domctl_cacheflush` structure that
 *                   contains the cache flush parameters.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_cacheflush(int domid,  struct xen_domctl_cacheflush *cacheflush);

/**
 * @brief Destroys a Xen domain.
 *
 * @param domid The ID of the domain to be destroyed.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_destroydomain(int domid);

/**
 * @brief Retrieves information about a specific virtual CPU (vCPU) in a Xen domain.
 *
 * @param domid The ID of the domain.
 * @param vcpu The index of the vCPU.
 * @param[out] info Pointer to a structure to store the vCPU information.
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int xen_domctl_getvcpu(int domid, uint32_t vcpu, struct xen_domctl_getvcpuinfo *info);

#endif /* __XEN_DOM0_DOMCTL_H__ */
