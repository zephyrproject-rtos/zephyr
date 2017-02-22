/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_SOC_INTERRUPTS_H__
#define __QM_SOC_INTERRUPTS_H__

#include "qm_common.h"

/**
 * Quark D2000 SoC Interrupts.
 *
 * @defgroup groupQUARKD2000SEINT SoC Interrupts (D2000)
 * @{
 */

/* x86 internal interrupt vectors. */
#define QM_X86_DIVIDE_ERROR_INT (0)
#define QM_X86_DEBUG_EXCEPTION_INT (1)
#define QM_X86_NMI_INTERRUPT_INT (2)
#define QM_X86_BREAKPOINT_INT (3)
#define QM_X86_OVERFLOW_INT (4)
#define QM_X86_BOUND_RANGE_EXCEEDED_INT (5)
#define QM_X86_INVALID_OPCODE_INT (6)
#define QM_X86_DEVICE_NOT_AVAILABLE_INT (7)
#define QM_X86_DOUBLE_FAULT_INT (8)
#define QM_X86_INTEL_RESERVED_09_INT (9)
#define QM_X86_INVALID_TSS_INT (10)
#define QM_X86_SEGMENT_NOT_PRESENT_INT (11)
#define QM_X86_STACK_SEGMENT_FAULT_INT (12)
#define QM_X86_GENERAL_PROTECT_FAULT_INT (13)
#define QM_X86_PAGE_FAULT_INT (14)
#define QM_X86_INTEL_RESERVED_15_INT (15)
#define QM_X86_FLOATING_POINT_ERROR_INT (16)
#define QM_X86_ALIGNMENT_CHECK_INT (17)
#define QM_X86_INTEL_RESERVED_18_INT (18)
#define QM_X86_INTEL_RESERVED_19_INT (19)
#define QM_X86_INTEL_RESERVED_20_INT (20)
#define QM_X86_INTEL_RESERVED_21_INT (21)
#define QM_X86_INTEL_RESERVED_22_INT (22)
#define QM_X86_INTEL_RESERVED_23_INT (23)
#define QM_X86_INTEL_RESERVED_24_INT (24)
#define QM_X86_INTEL_RESERVED_25_INT (25)
#define QM_X86_INTEL_RESERVED_26_INT (26)
#define QM_X86_INTEL_RESERVED_27_INT (27)
#define QM_X86_INTEL_RESERVED_28_INT (28)
#define QM_X86_INTEL_RESERVED_29_INT (29)
#define QM_X86_INTEL_RESERVED_30_INT (30)
#define QM_X86_INTEL_RESERVED_31_INT (31)

/* IRQs and interrupt vectors.
 *
 * The vector numbers must be defined without arithmetic expressions nor
 * parentheses because they are expanded as token concatenation.
 */

#define QM_IRQ_DMA_0_ERROR_INT 0
#define QM_IRQ_DMA_0_ERROR_INT_MASK_OFFSET 28
#define QM_IRQ_DMA_0_ERROR_INT_VECTOR 32

#define QM_IRQ_HOST_BUS_ERROR_INT 1
#define QM_IRQ_HOST_BUS_ERROR_INT_MASK_OFFSET 29
#define QM_IRQ_HOST_BUS_ERROR_INT_VECTOR 33

#define QM_IRQ_RTC_0_INT 2
#define QM_IRQ_RTC_0_INT_MASK_OFFSET 12
#define QM_IRQ_RTC_0_INT_VECTOR 34

#define QM_IRQ_AONPT_0_INT 3
#define QM_IRQ_AONPT_0_INT_MASK_OFFSET 32
#define QM_IRQ_AONPT_0_INT_VECTOR 35

#define QM_IRQ_I2C_0_INT 4
#define QM_IRQ_I2C_0_INT_MASK_OFFSET 0
#define QM_IRQ_I2C_0_INT_VECTOR 36

#define QM_IRQ_SPI_SLAVE_0_INT 5
#define QM_IRQ_SPI_SLAVE_0_INT_MASK_OFFSET 5
#define QM_IRQ_SPI_SLAVE_0_INT_VECTOR 37

#define QM_IRQ_UART_1_INT 6
#define QM_IRQ_UART_1_INT_MASK_OFFSET 7
#define QM_IRQ_UART_1_INT_VECTOR 38

#define QM_IRQ_SPI_MASTER_0_INT 7
#define QM_IRQ_SPI_MASTER_0_INT_MASK_OFFSET 3
#define QM_IRQ_SPI_MASTER_0_INT_VECTOR 39

#define QM_IRQ_UART_0_INT 8
#define QM_IRQ_UART_0_INT_MASK_OFFSET 6
#define QM_IRQ_UART_0_INT_VECTOR 40

#define QM_IRQ_ADC_0_CAL_INT 9
#define QM_IRQ_ADC_0_CAL_INT_MASK_OFFSET 34
#define QM_IRQ_ADC_0_CAL_INT_VECTOR 41

#define QM_IRQ_PIC_TIMER 10
/* No SCSS mask register for PIC timer: point to an unused register */
#define QM_IRQ_PIC_TIMER_MASK_OFFSET 1
#define QM_IRQ_PIC_TIMER_VECTOR 42

#define QM_IRQ_PWM_0_INT 11
#define QM_IRQ_PWM_0_INT_MASK_OFFSET 10
#define QM_IRQ_PWM_0_INT_VECTOR 43

#define QM_IRQ_DMA_0_INT_1 12
#define QM_IRQ_DMA_0_INT_1_MASK_OFFSET 15
#define QM_IRQ_DMA_0_INT_1_VECTOR 44

#define QM_IRQ_DMA_0_INT_0 13
#define QM_IRQ_DMA_0_INT_0_MASK_OFFSET 14
#define QM_IRQ_DMA_0_INT_0_VECTOR 45

#define QM_IRQ_COMPARATOR_0_INT 14
#define QM_IRQ_COMPARATOR_0_INT_MASK_OFFSET 26
#define QM_IRQ_COMPARATOR_0_INT_VECTOR 46

#define QM_IRQ_GPIO_0_INT 15
#define QM_IRQ_GPIO_0_INT_MASK_OFFSET 9
#define QM_IRQ_GPIO_0_INT_VECTOR 47

#define QM_IRQ_WDT_0_INT 16
#define QM_IRQ_WDT_0_INT_MASK_OFFSET 13
#define QM_IRQ_WDT_0_INT_VECTOR 48

#define QM_IRQ_SRAM_MPR_0_INT 17
#define QM_IRQ_SRAM_MPR_0_INT_MASK_OFFSET 29
#define QM_IRQ_SRAM_MPR_0_INT_VECTOR 49

#define QM_IRQ_FLASH_MPR_0_INT 18
#define QM_IRQ_FLASH_MPR_0_INT_MASK_OFFSET 30
#define QM_IRQ_FLASH_MPR_0_INT_VECTOR 50

#define QM_IRQ_ADC_0_PWR_INT 19
#define QM_IRQ_ADC_0_PWR_INT_MASK_OFFSET 33
#define QM_IRQ_ADC_0_PWR_INT_VECTOR 51

/** @} */

#endif /* __QM_SOC_INTERRUPTS_H__ */
