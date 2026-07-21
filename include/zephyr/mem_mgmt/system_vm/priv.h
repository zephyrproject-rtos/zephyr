/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private definitions for the low-level virtual memory management layer.
 *
 * What defined here should only be used in kernel, drivers or subsystems to
 * allow managing virtual memory, and should not be used in application.
 */

#ifndef ZEPHYR_INCLUDE_MEM_MGMT_VM_PRIV_H_
#define ZEPHYR_INCLUDE_MEM_MGMT_VM_PRIV_H_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup mem_mgmt_vm_backend
 * @{
 */

/**
 * Status of a particular page location.
 */
enum sys_mm_vm_page_location {
	/** The page has been evicted to the backing store. */
	SYS_MM_VM_PAGE_LOCATION_PAGED_OUT,

	/** The page is resident in memory. */
	SYS_MM_VM_PAGE_LOCATION_PAGED_IN,

	/** The page is not mapped. */
	SYS_MM_VM_PAGE_LOCATION_BAD
};

/**
 * @def SYS_MM_VM_DATA_PAGE_ACCESSED
 *
 * Bit indicating the data page was accessed since the value was last cleared.
 *
 * Used by marking eviction algorithms. Safe to set this if uncertain.
 *
 * This bit is undefined if SYS_MM_VM_DATA_PAGE_LOADED is not set.
 *
 * The actual bit value is provided by the backend.
 */
#ifdef __DOXYGEN__
#define SYS_MM_VM_DATA_PAGE_ACCESSED
#endif

/**
 * @def SYS_MM_VM_DATA_PAGE_DIRTY
 *
 * Bit indicating the data page, if evicted, will need to be paged out.
 *
 * Set if the data page was modified since it was last paged out, or if
 * it has never been paged out before. Safe to set this if uncertain.
 *
 * This bit is undefined if SYS_MM_VM_DATA_PAGE_LOADED is not set.
 *
 * The actual bit value is provided by the backend.
 */
#ifdef __DOXYGEN__
#define SYS_MM_VM_DATA_PAGE_DIRTY
#endif

/**
 * @def SYS_MM_VM_DATA_PAGE_LOADED
 *
 * Bit indicating that the data page is loaded into a physical page frame.
 *
 * If un-set, the data page is paged out or not mapped.
 *
 * The actual bit value is provided by the backend.
 */
#ifdef __DOXYGEN__
#define SYS_MM_VM_DATA_PAGE_LOADED
#endif

/**
 * @def SYS_MM_VM_DATA_PAGE_NOT_MAPPED
 *
 * If SYS_MM_VM_DATA_PAGE_LOADED is un-set, this will indicate that the page
 * is not mapped at all. This bit is undefined if SYS_MM_VM_DATA_PAGE_LOADED
 * is set.
 *
 * The actual bit value is provided by the backend.
 */
#ifdef __DOXYGEN__
#define SYS_MM_VM_DATA_PAGE_NOT_MAPPED
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_MEM_MGMT_VM_PRIV_H_ */
