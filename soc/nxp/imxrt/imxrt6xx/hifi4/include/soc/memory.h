/*
 * Copyright (c) 2023 Google LLC.
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __XTENSA_MEMORY_H__
#define __XTENSA_MEMORY_H__

#include <autoconf.h>

#define TEXT_BASE (CONFIG_RT685_ADSP_TEXT_MEM_ADDR)
#define TEXT_SIZE (CONFIG_RT685_ADSP_TEXT_MEM_SIZE)

#define DATA_BASE (CONFIG_RT685_ADSP_DATA_MEM_ADDR)
#define DATA_SIZE (CONFIG_RT685_ADSP_DATA_MEM_SIZE - CONFIG_RT685_ADSP_STACK_SIZE)

/* The reset vector address in SRAM and its size. */
#define XCHAL_RESET_VECTOR0_PADDR_IRAM (CONFIG_RT685_ADSP_RESET_MEM_ADDR)
#define MEM_RESET_TEXT_SIZE            (0x2e0)
#define MEM_RESET_LIT_SIZE             (0x120)

/* Base address of all interrupt vectors in IRAM. */
#define XCHAL_VECBASE_RESET_PADDR_IRAM (TEXT_BASE)
#define MEM_VECBASE_LIT_SIZE           (0x178)

/* Vector and literal sizes. */
#define MEM_VECT_LIT_SIZE  (0x4)
#define MEM_VECT_TEXT_SIZE (0x1C)

/* Addresses of the interrupt vectors. */
#define XCHAL_INT_VECTOR_ADDR(x)          (XCHAL_VECBASE_RESET_PADDR_IRAM + (x))
#define XCHAL_INTLEVEL2_VECTOR_PADDR_IRAM (XCHAL_INT_VECTOR_ADDR(0x17C))
#define XCHAL_INTLEVEL3_VECTOR_PADDR_IRAM (XCHAL_INT_VECTOR_ADDR(0x19C))
#define XCHAL_INTLEVEL4_VECTOR_PADDR_IRAM (XCHAL_INT_VECTOR_ADDR(0x1BC))
#define XCHAL_INTLEVEL5_VECTOR_PADDR_IRAM (XCHAL_INT_VECTOR_ADDR(0x1DC))
#define XCHAL_KERNEL_VECTOR_PADDR_IRAM    (XCHAL_INT_VECTOR_ADDR(0x1FC))
#define XCHAL_USER_VECTOR_PADDR_IRAM      (XCHAL_INT_VECTOR_ADDR(0x21C))
#define XCHAL_DOUBLEEXC_VECTOR_PADDR_IRAM (XCHAL_INT_VECTOR_ADDR(0x23C))


/* Size and location of the intList section. Later used to construct the
 * Interrupt Descriptor Table (IDT). This is a bogus address as this
 * section will be stripped off in the final image. Situated before the DSP's
 * ITCM - prevents memory region inflation in zephyr_pre0.elf.
 */
#define IDT_SIZE (0x2000)
#define IDT_BASE (CONFIG_RT685_ADSP_RESET_MEM_ADDR - IDT_SIZE)

#endif
