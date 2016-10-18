/*
 * Copyright (c) 2016, Intel Corporation
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

/**
 * Quark SE SoC Interrupts.
 *
 * @defgroup groupQUARKSESEINT SoC Interrupts (SE)
 * @{
 */

#if (QM_LAKEMONT)

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

#define QM_X86_PIC_TIMER_INT_VECTOR (32)

#endif /* QM_LAKEMONT */

#if (QM_SENSOR)

/* ARC EM processor internal interrupt vector assignments. */
#define QM_ARC_RESET_INT (0)
#define QM_ARC_MEMORY_ERROR_INT (1)
#define QM_ARC_INSTRUCTION_ERROR_INT (2)
#define QM_ARC_MACHINE_CHECK_EXCEPTION_INT (3)
#define QM_ARC_INSTRUCTION_TLB_MISS_INT (4)
#define QM_ARC_DATA_TLB_MISS_INT (5)
#define QM_ARC_PROTECTION_VIOLATION_INT (6)
#define QM_ARC_PRIVILEGE_VIOLATION_INT (7)
#define QM_ARC_SOFTWARE_INTERRUPT_INT (8)
#define QM_ARC_TRAP_INT (9)
#define QM_ARC_EXTENSION_INSTRUCTION_EXCEPTION_INT (10)
#define QM_ARC_DIVIDE_BY_ZERO_INT (11)
#define QM_ARC_DATA_CACHE_CONSISTENCY_ERROR_INT (12)
#define QM_ARC_MISALIGNED_DATA_ACCESS_INT (13)
#define QM_ARC_RESERVED_14_INT (14)
#define QM_ARC_RESERVED_15_INT (15)
#define QM_ARC_TIMER_0_INT (16)
#define QM_ARC_TIMER_1_INT (17)

#endif /* QM_SENSOR */

#if (QM_SENSOR)
/**
 * Sensor Sub-System Specific IRQs and interrupt vectors.
 *
 * @name SS Interrupt
 * @{
 */

#define QM_SS_EXCEPTION_NUM (16)  /* Exceptions and traps in ARC EM core. */
#define QM_SS_INT_TIMER_NUM (2)   /* Internal interrupts in ARC EM core. */
#define QM_SS_IRQ_SENSOR_NUM (18) /* IRQ's from the Sensor Subsystem. */
#define QM_SS_IRQ_COMMON_NUM (32) /* IRQ's from the common SoC fabric. */
#define QM_SS_INT_VECTOR_NUM                                                   \
	(QM_SS_EXCEPTION_NUM + QM_SS_INT_TIMER_NUM + QM_SS_IRQ_SENSOR_NUM +    \
	 QM_SS_IRQ_COMMON_NUM)
#define QM_SS_IRQ_NUM (QM_SS_IRQ_SENSOR_NUM + QM_SS_IRQ_COMMON_NUM)

/*
 * The following definitions are Sensor Subsystem interrupt irq and vector
 * numbers:
 * #define QM_SS_xxx          - irq number
 * #define QM_SS_xxx_VECTOR   - vector number
 */

/** Sensor Subsystem ADC Rx Fifo Error Interrupt. */
#define QM_SS_IRQ_ADC_0_ERROR_INT 0
#define QM_SS_IRQ_ADC_0_ERROR_INT_VECTOR 18

/** Sensor Subsystem ADC Data Available Interrupt. */
#define QM_SS_IRQ_ADC_0_INT 1
#define QM_SS_IRQ_ADC_0_INT_VECTOR 19

/** Sensor Subsystem GPIO Single Interrupt 0 */
#define QM_SS_IRQ_GPIO_0_INT 2
#define QM_SS_IRQ_GPIO_0_INT_VECTOR 20

/** Sensor Subsystem GPIO Single Interrupt 1. */
#define QM_SS_IRQ_GPIO_1_INT 3
#define QM_SS_IRQ_GPIO_1_INT_VECTOR 21

/** Sensor Subsystem I2C 0 Error Interrupt. */
#define QM_SS_IRQ_I2C_0_ERROR_INT 4
#define QM_SS_IRQ_I2C_0_ERROR_INT_VECTOR 22

/** Sensor Subsystem I2C 0 Data Available Interrupt. */
#define QM_SS_IRQ_I2C_0_RX_AVAIL_INT 5
#define QM_SS_IRQ_I2C_0_RX_AVAIL_INT_VECTOR 23

/** Sensor Subsystem I2C 0 Data Required Interrupt. */
#define QM_SS_IRQ_I2C_0_TX_REQ_INT 6
#define QM_SS_IRQ_I2C_0_TX_REQ_INT_VECTOR 24

/** Sensor Subsystem I2C 0 Stop Detect Interrupt. */
#define QM_SS_IRQ_I2C_0_STOP_DET_INT 7
#define QM_SS_IRQ_I2C_0_STOP_DET_INT_VECTOR 25

/** Sensor Subsystem I2C 1 Error Interrupt. */
#define QM_SS_IRQ_I2C_1_ERROR_INT 8
#define QM_SS_IRQ_I2C_1_ERROR_INT_VECTOR 26

/** Sensor Subsystem I2C 1 Data Available Interrupt. */
#define QM_SS_IRQ_I2C_1_RX_AVAIL_INT 9
#define QM_SS_IRQ_I2C_1_RX_AVAIL_INT_VECTOR 27

/** Sensor Subsystem I2C 1 Data Required Interrupt. */
#define QM_SS_IRQ_I2C_1_TX_REQ_INT 10
#define QM_SS_IRQ_I2C_1_TX_REQ_INT_VECTOR 28

/** Sensor Subsystem I2C 1 Stop Detect Interrupt. */
#define QM_SS_IRQ_I2C_1_STOP_DET_INT 11
#define QM_SS_IRQ_I2C_1_STOP_DET_INT_VECTOR 29

/** Sensor Subsystem SPI 0 Error Interrupt. */
#define QM_SS_IRQ_SPI_0_ERROR_INT 12
#define QM_SS_IRQ_SPI_0_ERROR_INT_VECTOR 30

/** Sensor Subsystem SPI 0 Data Available Interrupt. */
#define QM_SS_IRQ_SPI_0_RX_AVAIL_INT 13
#define QM_SS_IRQ_SPI_0_RX_AVAIL_INT_VECTOR 31

/** Sensor Subsystem SPI 0 Data Required Interrupt. */
#define QM_SS_IRQ_SPI_0_TX_REQ_INT 14
#define QM_SS_IRQ_SPI_0_TX_REQ_INT_VECTOR 32

/** Sensor Subsystem SPI 1 Error Interrupt. */
#define QM_SS_IRQ_SPI_1_ERROR_INT 15
#define QM_SS_IRQ_SPI_1_ERROR_INT_VECTOR 33

/** Sensor Subsystem SPI 1 Data Available Interrupt. */
#define QM_SS_IRQ_SPI_1_RX_AVAIL_INT 16
#define QM_SS_IRQ_SPI_1_RX_AVAIL_INT_VECTOR 34

/** Sensor Subsystem SPI 1 Data Required Interrupt. */
#define QM_SS_IRQ_SPI_1_TX_REQ_INT 17
#define QM_SS_IRQ_SPI_1_TX_REQ_INT_VECTOR 35

typedef enum {
	QM_SS_INT_PRIORITY_0 = 0,
	QM_SS_INT_PRIORITY_1 = 1,
	QM_SS_INT_PRIORITY_15 = 15,
	QM_SS_INT_PRIORITY_NUM
} qm_ss_irq_priority_t;

typedef enum { QM_SS_INT_DISABLE = 0, QM_SS_INT_ENABLE = 1 } qm_ss_irq_mask_t;

typedef enum {
	QM_SS_IRQ_LEVEL_SENSITIVE = 0,
	QM_SS_IRQ_EDGE_SENSITIVE = 1
} qm_ss_irq_trigger_t;

#define QM_SS_AUX_IRQ_CTRL (0xE)
#define QM_SS_AUX_IRQ_HINT (0x201)
#define QM_SS_AUX_IRQ_PRIORITY (0x206)
#define QM_SS_AUX_IRQ_STATUS (0x406)
#define QM_SS_AUX_IRQ_SELECT (0x40B)
#define QM_SS_AUX_IRQ_ENABLE (0x40C)
#define QM_SS_AUX_IRQ_TRIGGER (0x40D)

/** @} */

#endif /* QM_SENSOR */

/**
 * @name Common SoC IRQs and Interrupts
 * @{
 */

/* IRQs and interrupt vectors.
 *
 * Any IRQ > 1 actually has a event router mask register offset of +1.
 * The vector numbers must be defined without arithmetic expressions nor
 * parentheses because they are expanded as token concatenation.
 */

/** I2C Master 0 Single Interrupt. */
#define QM_IRQ_I2C_0_INT 0
#define QM_IRQ_I2C_0_INT_MASK_OFFSET 0
#define QM_IRQ_I2C_0_INT_VECTOR 36

/** I2C Master 1 Single Interrupt. */
#define QM_IRQ_I2C_1_INT 1
#define QM_IRQ_I2C_1_INT_MASK_OFFSET 1
#define QM_IRQ_I2C_1_INT_VECTOR 37

/** SPI Master 0 Single Interrupt. */
#define QM_IRQ_SPI_MASTER_0_INT 2
#define QM_IRQ_SPI_MASTER_0_INT_MASK_OFFSET 3
#define QM_IRQ_SPI_MASTER_0_INT_VECTOR 38

/** SPI Master 1 Single Interrupt. */
#define QM_IRQ_SPI_MASTER_1_INT 3
#define QM_IRQ_SPI_MASTER_1_INT_MASK_OFFSET 4
#define QM_IRQ_SPI_MASTER_1_INT_VECTOR 39

/** SPI Slave Single Interrupt. */
#define QM_IRQ_SPI_SLAVE_0_INT 4
#define QM_IRQ_SPI_SLAVE_0_INT_MASK_OFFSET 5
#define QM_IRQ_SPI_SLAVE_0_INT_VECTOR 40

/** UART 0 Single Interrupt. */
#define QM_IRQ_UART_0_INT 5
#define QM_IRQ_UART_0_INT_MASK_OFFSET 6
#define QM_IRQ_UART_0_INT_VECTOR 41

/** UART 1 Single Interrupt. */
#define QM_IRQ_UART_1_INT 6
#define QM_IRQ_UART_1_INT_MASK_OFFSET 7
#define QM_IRQ_UART_1_INT_VECTOR 42

/** I2S Single Interrupt. */
#define QM_IRQ_I2S_0_INT 7
#define QM_IRQ_I2S_0_INT_MASK_OFFSET 8
#define QM_IRQ_I2S_0_INT_VECTOR 43

/** GPIO Single Interrupt. */
#define QM_IRQ_GPIO_0_INT 8
#define QM_IRQ_GPIO_0_INT_MASK_OFFSET 9
#define QM_IRQ_GPIO_0_INT_VECTOR 44

/** PWM/Timer Single Interrupt. */
#define QM_IRQ_PWM_0_INT 9
#define QM_IRQ_PWM_0_INT_MASK_OFFSET 10
#define QM_IRQ_PWM_0_INT_VECTOR 45

/** USB Single Interrupt. */
#define QM_IRQ_USB_0_INT (10)
#define QM_IRQ_USB_0_INT_MASK_OFFSET (11)
#define QM_IRQ_USB_0_INT_VECTOR 46

/** RTC Single Interrupt. */
#define QM_IRQ_RTC_0_INT 11
#define QM_IRQ_RTC_0_INT_MASK_OFFSET 12
#define QM_IRQ_RTC_0_INT_VECTOR 47

/** WDT Single Interrupt. */
#define QM_IRQ_WDT_0_INT 12
#define QM_IRQ_WDT_0_INT_MASK_OFFSET 13
#define QM_IRQ_WDT_0_INT_VECTOR 48

/** DMA Channel 0 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_0 13
#define QM_IRQ_DMA_0_INT_0_MASK_OFFSET 14
#define QM_IRQ_DMA_0_INT_0_VECTOR 49

/** DMA Channel 1 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_1 14
#define QM_IRQ_DMA_0_INT_1_MASK_OFFSET 15
#define QM_IRQ_DMA_0_INT_1_VECTOR 50

/** DMA Channel 2 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_2 15
#define QM_IRQ_DMA_0_INT_2_MASK_OFFSET 16
#define QM_IRQ_DMA_0_INT_2_VECTOR 51

/** DMA Channel 3 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_3 16
#define QM_IRQ_DMA_0_INT_3_MASK_OFFSET 17
#define QM_IRQ_DMA_0_INT_3_VECTOR 52

/** DMA Channel 4 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_4 17
#define QM_IRQ_DMA_0_INT_4_MASK_OFFSET 18
#define QM_IRQ_DMA_0_INT_4_VECTOR 53

/** DMA Channel 5 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_5 18
#define QM_IRQ_DMA_0_INT_5_MASK_OFFSET 19
#define QM_IRQ_DMA_0_INT_5_VECTOR 54

/** DMA Channel 6 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_6 19
#define QM_IRQ_DMA_0_INT_6_MASK_OFFSET 20
#define QM_IRQ_DMA_0_INT_6_VECTOR 55

/** DMA Channel 7 Single Interrupt. */
#define QM_IRQ_DMA_0_INT_7 20
#define QM_IRQ_DMA_0_INT_7_MASK_OFFSET 21
#define QM_IRQ_DMA_0_INT_7_VECTOR 56

/**
 * 8 Mailbox Channel Interrupts Routed to Single Interrupt
 * with 8bit Mask per Destination.
 */
#define QM_IRQ_MAILBOX_0_INT 21
#define QM_IRQ_MAILBOX_0_INT_MASK_OFFSET 22
#define QM_IRQ_MAILBOX_0_INT_VECTOR 57

/**
 * 19 Comparators Routed to Single Interrupt with 19bit Mask per Destination.
 */
#define QM_IRQ_COMPARATOR_0_INT 22
#define QM_IRQ_COMPARATOR_0_INT_MASK_OFFSET 26
#define QM_IRQ_COMPARATOR_0_INT_VECTOR 58

/** System and Power Management Single Interrupt. */
#define QM_IRQ_PMU_0_INT 23
#define QM_IRQ_PMU_0_INT_MASK_OFFSET 26
#define QM_IRQ_PMU_0_INT_VECTOR 58

/**
 * 8 DMA Channel Error Interrupts Routed to Single Interrupt with 8bit Mask
 * per Destination.
 */
#define QM_IRQ_DMA_0_ERROR_INT 24
#define QM_IRQ_DMA_0_ERROR_INT_MASK_OFFSET 28
#define QM_IRQ_DMA_0_ERROR_INT_VECTOR 60

/** Internal SRAM Memory Protection Error Single Interrupt. */
#define QM_IRQ_SRAM_MPR_0_INT 25
#define QM_IRQ_SRAM_MPR_0_INT_MASK_OFFSET 29
#define QM_IRQ_SRAM_MPR_0_INT_VECTOR 61

/** Internal Flash Controller 0 Memory Protection Error Single Interrupt. */
#define QM_IRQ_FLASH_MPR_0_INT 26
#define QM_IRQ_FLASH_MPR_0_INT_MASK_OFFSET 30
#define QM_IRQ_FLASH_MPR_0_INT_VECTOR 62

/** Internal Flash Controller 1 Memory Protection Error Single Interrupt. */
#define QM_IRQ_FLASH_MPR_1_INT 27
#define QM_IRQ_FLASH_MPR_1_INT_MASK_OFFSET 31
#define QM_IRQ_FLASH_MPR_1_INT_VECTOR 63

/** Always-On Timer Interrupt. */
#define QM_IRQ_AONPT_0_INT 28
#define QM_IRQ_AONPT_0_INT_MASK_OFFSET 32
#define QM_IRQ_AONPT_0_INT_VECTOR 64

/** ADC power sequence done. */
#define QM_SS_IRQ_ADC_0_PWR_INT 29
#define QM_SS_IRQ_ADC_0_PWR_INT_MASK_OFFSET 33
#define QM_SS_IRQ_ADC_0_PWR_INT_VECTOR 65

/** ADC calibration done. */
#define QM_SS_IRQ_ADC_0_CAL_INT 30
#define QM_SS_IRQ_ADC_0_CAL_INT_MASK_OFFSET 34
#define QM_SS_IRQ_ADC_0_CAL_INT_VECTOR 66

/** Always-On GPIO Interrupt. */
#define QM_IRQ_AON_GPIO_0_INT 31
#define QM_IRQ_AON_GPIO_0_INT_MASK_OFFSET 35
#define QM_IRQ_AON_GPIO_0_INT_VECTOR 67

/** @} */

/** @} */

#endif /* __QM_SOC_INTERRUPTS_H__ */
