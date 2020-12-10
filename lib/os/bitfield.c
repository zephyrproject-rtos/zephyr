/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/bitfield.h>
#include <arch/cpu.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * Reserve contiguous bits in the bitfield
 */
int z_bitfield_alloc(void *bitfield, size_t bitfield_size, size_t region_size, size_t *offset)
{
	if (region_size == 0) {
		return -1; // Invalid
	}

	// Results
	size_t start_long_index = 0;
	size_t start_bit_index = 0; // Within start long
	size_t count = 0;
	bool found_start = false;
	bool found_end = false;

	/// Searching in array
	unsigned long *bitfield_ptr = (unsigned long *) bitfield;
	
	// Iterate longs
	for (size_t i = 0; i <= (bitfield_size / (sizeof(long) * 8)); i++) {
		// Search for beginning of a free region within long
		if (!found_start) { 
			printf("Searching for beginning... in long: %x \n", bitfield_ptr[i]);
			for (size_t j = 0; j < sizeof(long) * 8; j++) {
				// Searching for beginning of free region in long
				if (!found_start) { 
					// If bit is free, it is the beginning of a free region
					if (sys_test_bit((mem_addr_t)&bitfield_ptr[i], j) == 0) {
							start_long_index = i;
							start_bit_index = j;
							found_start = true;
							count = 1;
							if (count >= region_size) { // Hack (?) for 1-pagers
								printf("Found end: %i \n", j);
								found_end = true;
								break;
							}
					}
				}
				// Found beginning of free region in long, searching for end in long
				else if (found_start & !found_end) {
					// Next bit is 1
					if (sys_test_bit((mem_addr_t)&bitfield_ptr[i], j) == 0) {
						count += 1;
						if (count >= region_size) { // Hack (?), try == ?
							printf("Found end: %i \n", j);
							found_end = true;
							break;
						}
					}
					// Next bit is 0
					// (Not enough previous bits such that count was equal to region size)
					else { 
						// Continue searching
						found_start = false;
						found_end = false;
						printf(" *\n");
					}

					printf("%i/%i/%i ", j, sys_test_bit((mem_addr_t)&bitfield_ptr[i], j)==0, count);
				}
			}
		} 
		else if (found_start && !found_end) { // Search for continuation in next long
			printf("Searching for end...\n");
		}
		else {
			printf("Found beginning and end...\n");
			break;
		}
	}

	// found_start = true;
	// found_end = true;
	// start_long_index = 2;
	// start_bit_index = 12;

	/* Allocation */
	if (found_start && found_end) {
		size_t end_long_index = start_long_index + (region_size / (sizeof(long) * 8)); // Last long
		/* Index of the bit AFTER the last bit to be marked free in the ending long. 
		* (If 0, then the entire ending long will be marked free)
		*/
		size_t end_bit_index = (region_size - ((sizeof(long)*8) - start_bit_index)) % (sizeof(long)*8);

		unsigned long mask = 0UL;
		unsigned long left_mask = (~(0UL)) >> start_bit_index;
		unsigned long right_mask = (~(0UL)) << ((sizeof(long) * 8) - end_bit_index);

		if (start_long_index == end_long_index) {
			mask = left_mask & right_mask;
			bitfield_ptr[start_long_index] = bitfield_ptr[start_long_index] | mask;
		} else {
			for (size_t i = start_long_index; i <= end_long_index; i++) {
				if (i == start_long_index) {
					bitfield_ptr[i] = bitfield_ptr[i] ^ left_mask;
				} else if (i == end_long_index && end_bit_index != 0U) {
					bitfield_ptr[i] = bitfield_ptr[i] ^ right_mask;
				} else if (i != end_long_index) {
					bitfield_ptr[i] = ~(0UL);
				}
			}
		}

		*offset = (start_long_index * sizeof(long) * 8) + start_bit_index;

		return 0;
	} else {
		return -1;
	}
}

/*
 * Release in-use bits in a bitfield
 */
void z_bitfield_free(void *bitfield, size_t region_size, size_t offset)
{
	if (region_size == 0) {
		return;
	}

	unsigned long *bitfield_ptr = (unsigned long *) bitfield;

	size_t start_long_index = (offset / (sizeof(long) * 8)); // First long
	size_t start_bit_index = (offset % (sizeof(long) * 8)); // First bit index in first long
	size_t end_long_index = ((offset + region_size) / (sizeof(long) * 8)); // Last long
	/* Index of the bit AFTER the last bit to be marked free in the ending long. 
	 * (If 0, then the entire ending long will be marked free)
	 */
	size_t end_bit_index = ((offset + region_size) % (sizeof(long) * 8));

	unsigned long mask = 0UL;
	unsigned long left_mask = (~(0UL)) >> start_bit_index;
	unsigned long right_mask = (~(0UL)) << ((sizeof(long) * 8) - end_bit_index);

	if (start_long_index == end_long_index) {
		mask = left_mask & right_mask;
		bitfield_ptr[start_long_index] = bitfield_ptr[start_long_index] | mask;
	} else {
		for (size_t i = start_long_index; i <= end_long_index; i++) {
			if (i == start_long_index) {
				bitfield_ptr[i] = bitfield_ptr[i] | left_mask;
			} else if (i == end_long_index && end_bit_index != 0U) {
				bitfield_ptr[i] = bitfield_ptr[i] | right_mask;
			} else if (i != end_long_index) {
				bitfield_ptr[i] = ~(0UL);
			}
		}
	}
}
