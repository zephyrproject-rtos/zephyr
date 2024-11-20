/*
 * Copyright (c) 2023 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#if CONFIG_TRUSTED_EXECUTION_NONSECURE && DT_NODE_EXISTS(DT_NODELABEL(fast_code_partition))

#define PART_ADDR(label) ( \
    DT_REG_ADDR(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(label))) + \
    DT_REG_ADDR(DT_NODELABEL(label)) \
)

#ifdef CONFIG_BOOTLOADER_MCUBOOT
// the merged SPE/NSPE/fast_code slot needs the physical base address
// of the parent of slot0_partition
#define PART_FAST_CODE_ADDR(label) ( \
    DT_REG_ADDR(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(slot0_partition))) + \
    DT_REG_ADDR(DT_NODELABEL(label)) \
)
#else
#define PART_FAST_CODE_ADDR(label) PART_ADDR(label)
#endif

MEMORY {
    RRAM (rx) : ORIGIN = PART_FAST_CODE_ADDR(fast_code_partition), LENGTH = DT_REG_SIZE(DT_NODELABEL(fast_code_partition))
}

SECTIONS {
    fast_code : {
	*/libarch*.a:(.text ".text.*" .rodata ".rodata.*")
	*/libblell*.a:(.text ".text.*" .rodata ".rodata.*")
	*/libc*.a:(.text ".text.*" .rodata ".rodata.*")
	*/libdrivers*.a:(.text ".text.*" .rodata ".rodata.*")
	*/libgcc.a:(.text ".text.*" .rodata ".rodata.*")
	*/libkernel.a:(.text ".text.*" .rodata ".rodata.*")
	*/libmbedTLS*.a:(.text ".text.*" .rodata ".rodata.*")
	*/lib*mbedtls.a:(.text ".text.*" .rodata ".rodata.*")
	*/libopenthread*.a:(.text ".text.*" .rodata ".rodata.*")
	*/libsubsys*.a:(.text ".text.*" .rodata ".rodata.*")
	*/libzephyr.a:(.text ".text.*" .rodata ".rodata.*")
	. = ALIGN(4);
    } > RRAM
}
#endif

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
