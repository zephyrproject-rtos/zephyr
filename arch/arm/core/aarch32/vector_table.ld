/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_CPU_CORTEX_M_HAS_VTOR)
/*
 * In an MCU with VTOR, the VTOR.TBLOFF is set to the start address of the
 * exc_vector_table (i.e. _vector_start) during initialization. Therefore,
 * exc_vector_table must respect the alignment requirements of VTOR.TBLOFF
 * described below.
 */

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* VTOR bits 0:7 are reserved (RES0). This requires that the base address
 * of the vector table is 64-word aligned.
 */
. = ALIGN( 1 << LOG2CEIL(4 * 64) );
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* VTOR bits 0:6 are reserved (RES0). This requires that the base address
 * of the vector table is 32-word aligned.
 */
. = ALIGN( 1 << LOG2CEIL(4 * 32) );
#else
#error "Unsupported architecture variant"
#endif

/* When setting TBLOFF in VTOR we must align the offset to the number of
 * exception entries in the vector table. The minimum alignment of 32 words
 * is sufficient for the 16 ARM Core exceptions and up to 16 HW interrupts.
 * For more than 16 HW interrupts, we adjust the alignment by rounding up
 * to the next power of two; this restriction guarantees a functional VTOR
 * setting in any Cortex-M implementation (might not be required in every
 * Cortex-M processor).
 */
. = ALIGN( 1 << LOG2CEIL(4 * (16 + CONFIG_NUM_IRQS)) );
#endif

_vector_start = .;
KEEP(*(.exc_vector_table))
KEEP(*(".exc_vector_table.*"))

KEEP(*(.vectors))

_vector_end = .;
