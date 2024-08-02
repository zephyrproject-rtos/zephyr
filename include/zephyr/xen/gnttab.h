/*
 * Copyright (c) 2021-2022 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_GNTTAB_H__
#define __XEN_GNTTAB_H__

#include <zephyr/xen/public/grant_table.h>

/*
 * Assigns gref and permits access to 4K page for specific domain.
 *
 * @param domid - id of the domain you sharing gref with
 * @param gfn - guest frame number of page, where grant will be located
 * @param readonly - permit readonly access to shared grant
 * @return - gref assigned to shared grant
 */
grant_ref_t gnttab_grant_access(domid_t domid, unsigned long gfn,
		bool readonly);

/*
 * Finished access for previously shared grant. Does NOT
 * free memory, if it was previously allocated by
 * gnttab_alloc_and_grant().
 *
 * @param gref - grant reference that need to be closed
 * @return - zero on success, non-zero on failure
 */
int gnttab_end_access(grant_ref_t gref);

/*
 * Allocates 4K page for grant and share it via returned
 * gref. Need to k_free memory, which was allocated in
 * @map parameter after grant releasing.
 *
 * @param map - double pointer to memory, where grant will be allocated
 * @param readonly - permit readonly access to allocated grant
 * @return -  grant ref on success or negative errno on failure
 */
int32_t gnttab_alloc_and_grant(void **map, bool readonly);

/*
 * Provides interface to acquire free page, that can be used for
 * mapping of foreign frames. Should be freed by gnttab_put_page()
 * after usage.
 *
 * @return - pointer to page start address, that can be used as host_addr
 *           in struct gnttab_map_grant_ref, NULL on error.
 */
void *gnttab_get_page(void);

/*
 * Releases provided page, that was used for mapping foreign grant frame,
 * should be called after unmapping.
 *
 * @param page_addr - pointer to start address of used page.
 */
void gnttab_put_page(void *page_addr);

/*
 * Maps foreign grant ref to Zephyr address space.
 *
 * @param map_ops - array of prepared gnttab_map_grant_ref's for mapping
 * @param count - number of grefs in map_ops array
 * @return - zero on success or negative errno on failure
 *           also per-page status will be set in map_ops[i].status (GNTST_*)
 *
 * To map foreign frame you need 4K-aligned 4K memory page, which will be
 * used as host_addr for grant mapping - it should be acquired by gnttab_get_page()
 * function.
 */
int gnttab_map_refs(struct gnttab_map_grant_ref *map_ops, unsigned int count);

/*
 * Unmap foreign grant refs. The gnttab_put_page() should be used after this for
 * each page, that was successfully unmapped.
 *
 * @param unmap_ops - array of prepared gnttab_map_grant_ref's for unmapping
 * @param count - number of grefs in unmap_ops array
 * @return - @count on success or negative errno on failure
 *           also per-page status will be set in unmap_ops[i].status (GNTST_*)
 */
int gnttab_unmap_refs(struct gnttab_map_grant_ref *unmap_ops, unsigned int count);

/*
 * Convert grant ref status codes (GNTST_*) to text messages.
 *
 * @param status - negative GNTST_* code, that needs to be converted
 * @return - constant pointer to text message, associated with @status
 */
const char *gnttabop_error(int16_t status);

#endif /* __XEN_GNTTAB_H__ */
