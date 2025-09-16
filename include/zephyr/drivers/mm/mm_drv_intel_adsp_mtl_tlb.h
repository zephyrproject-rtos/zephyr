/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * Author: Marcin Szkudlinski <marcin.szkudlinski@linux.intel.com>
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTEL_ADSP_MTL_TLB
#define ZEPHYR_INCLUDE_DRIVERS_INTEL_ADSP_MTL_TLB


/*
 * This function will save contents of the physical memory banks into a provided storage buffer
 *
 * the system must be almost stopped. Operation is destructive - it will change physical to
 * virtual addresses mapping leaving the system not operational
 * Power states of memory banks will stay not touched
 * assuming
 *	- the dcache memory had been invalidated before
 *	- no remapping of addresses below unused_l2_sram_start_marker has been made
 *	  (this is ensured by the driver itself - it rejects such remapping request)
 *
 * at this point the memory is still up&running so its safe to use libraries like memcpy
 * and the procedure can be called in a zephyr driver model way
 */
typedef void (*mm_save_context)(void *storage_buffer);

/*
 * This function will return a required size of storage buffer needed to perform context save
 *
 */
typedef uint32_t (*mm_get_storage_size)(void);

/*
 * This function will restore the contents and power state of the physical memory banks
 * and recreate physical to virtual mappings
 *
 * As the system memory is down at this point, the procedure
 *  - MUST be located in IMR memory region
 *  - MUST be called using a simple extern procedure call - API table is not yet loaded
 *  - MUST NOT use libraries, like memcpy, use instead a special version bmemcpy located in IMR
 *
 */
void adsp_mm_restore_context(void *storage_buffer);

/*
 * This procedure return a pointer to a first unused address in L2 virtual memory
 */
uintptr_t adsp_mm_get_unused_l2_start_aligned(void);

struct intel_adsp_tlb_api {
	mm_save_context save_context;
	mm_get_storage_size get_storage_size;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTEL_ADSP_MTL_TLB */
