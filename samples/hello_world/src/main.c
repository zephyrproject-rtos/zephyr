/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #include <stdio.h>

// int main(void)
// {
// 	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

// 	return 0;
// }

/*----------------- Includes ---------------*/
#include <stdint.h>
#include <string.h>


#include <riscv_mmu.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

extern uintptr_t _image_ram_start;
extern uintptr_t _image_ram_end;


int main(void)
{
    uintptr_t ram_start = (uintptr_t)&_image_ram_start;
    uintptr_t ram_end   = (uintptr_t)&_image_ram_end;
    size_t ram_size     = ram_end - ram_start;

    printk("RAM image starts at: 0x%08lx\n", ram_start);
    printk("RAM image ends at:   0x%08lx\n", ram_end);
    printk("RAM image size:      %zu bytes (%lu KB)\n", ram_size, ram_size / 1024);

    z_riscv_mm_init();

    // 1. Map the entire RAM region using L1 index 512
    uintptr_t virt_base_512 = 0x80000000;
    uintptr_t phys_base_512 = 0x40000000;
    int page_mapped = 0;
    uintptr_t phys_addr;
    for (uintptr_t offset = 0; offset < ram_size; offset += KB(4)) {
        uintptr_t va = virt_base_512 + offset;
        uintptr_t pa = phys_base_512 + offset;
       
        riscv_map_page(va, pa, 0);

    // Optional: give time for TLB flush to take effect
        k_sleep(K_MSEC(1));

        int ret = arch_page_phys_get((void *)va, &phys_addr);
        if (ret == 0) {
            printk("Verified mapping: VA %p -> PA %p\n", (void *)va, (void *)phys_addr);
        }
    }

    // uintptr_t virt_base_513 = 0x80400000;  // L1 Index 513
    // uintptr_t phys_base_513 = 0x40400000;
    // size_t test_len = KB(64);  // map 64KB
    // for (uintptr_t offset = 0; offset < test_len; offset += KB(4)) {
    //     riscv_map_page(virt_base_513 + offset, phys_base_513 + offset, 0);
    // }

    return 0;
}
