/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <devicetree/memory.h>
#include <linker/linker-defs.h>

/* Variables placed in reserved sections */
uint32_t var_in_res0 __attribute((__section__(".res0"))) = 0xaabbccdd;
uint32_t var_in_res1 __attribute((__section__(".res1"))) = 0xddccbbaa;

void main(void)
{
	uint8_t *res_ptr_outer, *res_ptr_inner;
	uint32_t res_size_outer, res_size_inner;

	res_ptr_outer =
		DT_RESERVED_MEM_GET_PTR_BY_PHANDLE(DT_NODELABEL(sample_driver_outer),
						   memory_region);
	res_size_outer =
		DT_RESERVED_MEM_GET_SIZE_BY_PHANDLE(DT_NODELABEL(sample_driver_outer),
						    memory_region);

	printk("Reserved memory address for the outer driver: %p\n", res_ptr_outer);
	printk("Reserved memory size for the outer driver: %d\n", res_size_outer);

	res_ptr_inner = DT_RESERVED_MEM_GET_PTR(DT_NODELABEL(res1));
	res_size_inner = DT_RESERVED_MEM_GET_SIZE(DT_NODELABEL(res1));

	printk("Reserved memory address for the inner driver: %p\n", res_ptr_inner);
	printk("Reserved memory size for the inner driver: %d\n", res_size_inner);

	printk("Address of var_in_res0: %p\n", &var_in_res0);
	printk("Address of var_in_res1: %p\n", &var_in_res1);
}
