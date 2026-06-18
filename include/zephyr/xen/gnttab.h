/*
 * Copyright (c) 2021-2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen grant table helpers.
 * @ingroup xen_grant_tables
 */

#ifndef __XEN_GNTTAB_H__
#define __XEN_GNTTAB_H__

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/xen/public/grant_table.h>

/**
 * @defgroup xen_grant_tables Xen grant tables
 * @ingroup xen_support
 * @brief Share memory with foreign domains and map foreign grants.
 * @{
 */

/**
 * @brief Grant another domain access to a guest frame.
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param domid Foreign domain that receives access to the frame.
 * @param gfn Guest frame number to expose.
 * @param readonly Set to ``true`` to grant read-only access.
 *
 * @return Grant reference that identifies the new entry.
 */
grant_ref_t gnttab_grant_access(domid_t domid, unsigned long gfn,
				bool readonly);

/**
 * @brief Release a grant reference created by gnttab_grant_access().
 *
 * This does NOT free memory that was previously allocated by
 * gnttab_alloc_and_grant().
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param gref Grant reference to release.
 *
 * @retval 0 on success.
 */
int gnttab_end_access(grant_ref_t gref);

/**
 * @brief Allocate a page and immediately grant access to it.
 *
 * The caller must release the grant and then ``k_free()`` the buffer returned
 * in @p map once it is no longer needed.
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param[out] map Buffer that receives the page base address.
 * @param readonly Set to ``true`` to grant read-only access.
 *
 * @return Grant reference on success, negative errno value on failure.
 * @retval -ENOMEM Page allocation fails.
 */
int32_t gnttab_alloc_and_grant(void **map, bool readonly);

/**
 * @brief Allocate pages that can host grant mappings.
 *
 * The returned range is suitable for use as ``host_addr`` in ``struct
 * gnttab_map_grant_ref`` entries. Release it with gnttab_put_pages() after use.
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param npages Number of pages to allocate.
 *
 * @return Base address of the allocated range, or ``NULL`` on failure.
 */
void *gnttab_get_pages(unsigned int npages);

/**
 * @brief Release pages allocated by gnttab_get_pages().
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param start_addr Base address returned by gnttab_get_pages().
 * @param npages Number of pages to release.
 *
 * @return 0 on success, negative errno value on failure.
 */
int gnttab_put_pages(void *start_addr, unsigned int npages);

/**
 * @brief Map one or more foreign grant references.
 *
 * The ``host_addr`` pages referenced by @p map_ops must be 4K-aligned and
 * should be obtained from gnttab_get_pages().
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param[in,out] map_ops Array of prepared map operations.
 * @param count Number of entries in @p map_ops.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EFAULT Xen extended regions are enabled and a host address is
 *         outside them.
 *
 * Xen also stores a per-entry status code in each ``map_ops[i].status`` field.
 */
int gnttab_map_refs(struct gnttab_map_grant_ref *map_ops, unsigned int count);

/**
 * @brief Unmap one or more foreign grant references.
 *
 * Use gnttab_put_pages() afterwards for the pages that were successfully
 * unmapped.
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param[in,out] unmap_ops Array of prepared unmap operations.
 * @param count Number of entries in @p unmap_ops.
 *
 * @return Value returned by the ``GNTTABOP_unmap_grant_ref`` hypercall (0 on
 *         success), negative errno value on failure.
 * @retval -EFAULT Xen extended regions are enabled and a host address is
 *         outside them.
 *
 * Xen also stores a per-entry status code in each ``unmap_ops[i].status``
 * field.
 */
int gnttab_unmap_refs(struct gnttab_unmap_grant_ref *unmap_ops, unsigned int count);

/**
 * @brief Convert a Xen grant-table status into readable text.
 *
 * @kconfig_dep{CONFIG_XEN_GRANT_TABLE}
 *
 * @param status Negative ``GNTST_*`` status code.
 *
 * @return Pointer to a constant error string.
 */
const char *gnttabop_error(int16_t status);

/** @} */

#endif /* __XEN_GNTTAB_H__ */
