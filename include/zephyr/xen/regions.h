/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen extended regions interface
 *
 * This file provides the interface for managing Xen extended regions,
 * including allocation, freeing pages, and mapping/unmapping extended
 * regions memory.
 */

#ifndef __XEN_REGIONS_H__
#define __XEN_REGIONS_H__

/**
 * Allocate pages from the extended regions
 *
 * @param nr_pages number of pages to be allocated
 * @return pointer to the allocated pages
 */
void *xen_region_get_pages(size_t nr_pages);

/**
 * Free pages on extended regions
 *
 * @param ptr pointer to the pages, allocated by xen_region_get_pages call
 * @param nr_pages number of pages that were allocated
 * @return zero on success, non-zero on failure
 */
int xen_region_put_pages(void *ptr, size_t nr_pages);

/**
 * Map extended region memory
 *
 * @param ptr pointer to the pages, allocated by xen_region_get_pages call
 * @param nr_pages number of pages that should be mapped
 * @return zero on success, non-zero on failure
 */
int xen_region_map(void *ptr, size_t nr_pages);

/**
 * Unmap extended region memory
 *
 * @param ptr pointer to the pages, allocated by xen_region_get_pages call
 * @param nr_pages number of pages that should be unmapped
 * @return zero on success, non-zero on failure
 */
int xen_region_unmap(void *ptr, size_t nr_pages);

#endif /* __XEN_REGIONS_H__ */
