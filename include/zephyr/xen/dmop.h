/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen device-model operations.
 * @ingroup xen_device_model
 */

#ifndef ZEPHYR_XEN_DMOP_H_
#define ZEPHYR_XEN_DMOP_H_

#include <zephyr/xen/public/hvm/dm_op.h>

/**
 * @defgroup xen_device_model Xen device model
 * @ingroup xen_support
 * @brief Manage Xen I/O request servers and related device-model state.
 * @{
 */

/**
 * @brief Create an I/O request server in the given Xen domain.
 *
 * This function issues the XEN_DMOP_create_ioreq_server hypercall to create
 * a server that handles I/O requests on behalf of the guest domain.
 *
 * @kconfig_dep{CONFIG_XEN_DMOP}
 *
 * @param domid        Xen domain identifier where the server is created.
 * @param handle_bufioreq  Flag indicating whether buffered I/O requests should be handled.
 *                        Set to non-zero to enable buffered handling.
 * @param[out] id         Output pointer to receive the newly created server ID.
 *
 * @return 0 on success, negative errno value on failure.
 */
int dmop_create_ioreq_server(domid_t domid, uint8_t handle_bufioreq, ioservid_t *id);

/**
 * @brief Destroy a previously created I/O request server.
 *
 * Issues the XEN_DMOP_destroy_ioreq_server hypercall to tear down the
 * specified I/O request server.
 *
 * @kconfig_dep{CONFIG_XEN_DMOP}
 *
 * @param domid Xen domain identifier where the server exists.
 * @param id    I/O request server ID returned by dmop_create_ioreq_server().
 *
 * @return 0 on success, negative errno value on failure.
 */
int dmop_destroy_ioreq_server(domid_t domid, ioservid_t id);

/**
 * @brief Map a specified I/O address range to an existing I/O request server.
 *
 * This function issues the XEN_DMOP_map_io_range_to_ioreq_server hypercall to grant access to the
 * given I/O address range for the specified server.
 *
 * @kconfig_dep{CONFIG_XEN_DMOP}
 *
 * @param domid  Xen domain identifier where the mapping is applied.
 * @param id     I/O request server ID returned by dmop_create_ioreq_server().
 * @param type   Type identifier for the I/O range (e.g., MMIO, port I/O).
 * @param start  Start physical address of the I/O range.
 * @param end    End physical address (inclusive) of the I/O range.
 *
 * @return 0 on success, negative errno value on failure.
 */
int dmop_map_io_range_to_ioreq_server(domid_t domid, ioservid_t id,
	uint32_t type,
	uint64_t start,
	uint64_t end);

/**
 * @brief Unmap an I/O address range from an I/O request server.
 *
 * Issues the XEN_DMOP_unmap_io_range_from_ioreq_server hypercall to revoke access to a previously
 * mapped I/O address range.
 *
 * @kconfig_dep{CONFIG_XEN_DMOP}
 *
 * @param domid Xen domain identifier where the unmapping is applied.
 * @param id    I/O request server ID returned by dmop_create_ioreq_server().
 * @param type  Type identifier for the I/O range (e.g., MMIO, port I/O).
 * @param start Start physical address of the I/O range.
 * @param end   End physical address (inclusive) of the I/O range.
 *
 * @return 0 on success, negative errno value on failure.
 */
int dmop_unmap_io_range_from_ioreq_server(domid_t domid, ioservid_t id, uint32_t type,
					  uint64_t start, uint64_t end);

/**
 * @brief Enable or disable an existing I/O request server.
 *
 * @kconfig_dep{CONFIG_XEN_DMOP}
 *
 * This function issues the XEN_DMOP_set_ioreq_server_state hypercall to change
 * the operational state of the specified I/O request server.
 *
 * @param domid    Xen domain identifier.
 * @param id       I/O request server ID to modify.
 * @param enabled  Non-zero to enable the server, zero to disable it.
 *
 * @return 0 on success, negative errno value on failure.
 */
int dmop_set_ioreq_server_state(domid_t domid, ioservid_t id, uint8_t enabled);

/**
 * @brief Query the number of virtual CPUs in a Xen domain.
 *
 * This function issues the XEN_DMOP_nr_vcpus hypercall to retrieve
 * the current vCPU count for the specified domain.
 *
 * @kconfig_dep{CONFIG_XEN_DMOP}
 *
 * @param domid  Xen domain identifier to query.
 *
 * @return Number of vCPUs on success, negative errno value on failure.
 */
int dmop_nr_vcpus(domid_t domid);

/**
 * @brief Set the interrupt level for a specific IRQ in a Xen domain.
 *
 * This function issues the XEN_DMOP_set_irq_level hypercall to adjust
 * the signal level (assert or deassert) for the given IRQ line.
 *
 * @kconfig_dep{CONFIG_XEN_DMOP}
 *
 * @param domid  Xen domain identifier.
 * @param irq    IRQ number whose level is to be set.
 * @param level  Non-zero to assert (raise) the IRQ, zero to deassert (lower) it.
 *
 * @return 0 on success, negative errno value on failure.
 */
int dmop_set_irq_level(domid_t domid, uint32_t irq, uint8_t level);

/** @} */

#endif /* ZEPHYR_XEN_DMOP_H_ */
