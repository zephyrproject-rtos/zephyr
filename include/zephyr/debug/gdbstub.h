/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_GDBSTUB_H_
#define ZEPHYR_INCLUDE_DEBUG_GDBSTUB_H_

/* Map from CPU exceptions to GDB  */
#define GDB_EXCEPTION_INVALID_INSTRUCTION   4UL
#define GDB_EXCEPTION_BREAKPOINT            5UL
#define GDB_EXCEPTION_MEMORY_FAULT          7UL
#define GDB_EXCEPTION_DIVIDE_ERROR          8UL
#define GDB_EXCEPTION_INVALID_MEMORY        11UL
#define GDB_EXCEPTION_OVERFLOW              16UL

/* Access permissions for memory regions */
#define GDB_MEM_REGION_NO_ACCESS            0UL
#define GDB_MEM_REGION_READ                 BIT(0)
#define GDB_MEM_REGION_WRITE                BIT(1)

#define GDB_MEM_REGION_RO \
	(GDB_MEM_REGION_READ)

#define GDB_MEM_REGION_RW \
	(GDB_MEM_REGION_READ | GDB_MEM_REGION_WRITE)

/** Describe one memory region */
struct gdb_mem_region {
	/** Start address of a memory region */
	uintptr_t start;

	/** End address of a memory region */
	uintptr_t end;

	/** Memory region attributes */
	uint16_t  attributes;

	/** Read/write alignment, 0 if using default alignment */
	uint8_t   alignment;
};

/**
 * State of the packet processing loop
 */
enum gdb_loop_state {
	GDB_LOOP_RECEIVING,
	GDB_LOOP_CONTINUE,
	GDB_LOOP_ERROR,
};

/**
 * Memory region descriptions used for GDB memory access.
 *
 * This array specifies which region of memory GDB can access
 * with read/write attributes. This is used to restrict
 * memory read/write in GDB stub to memory that can be
 * legally accessed without resulting in memory faults.
 */
extern const struct gdb_mem_region gdb_mem_region_array[];

/**
 * Number of Memory Regions.
 *
 * Number of elements in gdb_mem_region_array[];
 */
extern const size_t gdb_mem_num_regions;

/**
 * @brief Convert a binary array into string representation.
 *
 * Note that this is similar to bin2hex() but does not force
 * a null character at the end of the hexadecimal output buffer.
 *
 * @param buf     The binary array to convert
 * @param buflen  The length of the binary array to convert
 * @param hex     Address of where to store the string representation.
 * @param hexlen  Size of the storage area for string representation.
 *
 * @return The length of the converted string, or 0 if an error occurred.
 */
size_t gdb_bin2hex(const uint8_t *buf, size_t buflen,
		   char *hex, size_t hexlen);


/**
 * @brief Check if a memory block can be read.
 *
 * This checks if the specified memory block can be read.
 *
 * @param[in]  addr  Starting address of the memory block
 * @param[in]  len   Size of memory block
 * @param[out] align Read alignment of region
 *
 * @return True if memory block can be read, false otherwise.
 */
bool gdb_mem_can_read(const uintptr_t addr, const size_t len, uint8_t *align);

/**
 * @brief Check if a memory block can be written into.
 *
 * This checks if the specified memory block can be written into.
 *
 * @param[in]  addr  Starting address of the memory block
 * @param[in]  len   Size of memory block
 * @param[out] align Write alignment of region
 *
 * @return True if GDB stub can write to the block, false otherwise.
 */
bool gdb_mem_can_write(const uintptr_t addr, const size_t len, uint8_t *align);

#endif
