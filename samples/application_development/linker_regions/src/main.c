/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#define IN_SECTION(s) __attribute__((__section__(s)))

#if DT_HAS_FIXED_PARTITION_LABEL(first_region)
uint32_t first_variable IN_SECTION(".first_section") = 0xdeadbeef;
#endif /* DT_HAS_FIXED_PARTITION_LABEL(first_region) */

#if DT_HAS_FIXED_PARTITION_LABEL(second_region)
uint32_t second_variable IN_SECTION(".second_section") = 0xf00dcafe;
#endif /* DT_HAS_FIXED_PARTITION_LABEL(second_region) */

void main(void)
{
#if DT_HAS_FIXED_PARTITION_LABEL(first_region)
	printk("First @ %p: 0x%x\n", &first_variable, first_variable);
#endif /* DT_HAS_FIXED_PARTITION_LABEL(first_region) */

#if DT_HAS_FIXED_PARTITION_LABEL(second_region)
	printk("Second @ %p: 0x%x\n", &second_variable, second_variable);
#endif /* DT_HAS_FIXED_PARTITION_LABEL(second_region) */
}
