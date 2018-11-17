/***************************************************************************//**
 * @file startup_efm32pg12b.S
 * @brief startup file for Silicon Labs EFM32PG12B devices.
 *        For use with GCC for ARM Embedded Processors
 * @version 5.6.0
 * @date     02. March 2016
 ******************************************************************************/
/*
 * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

    .syntax     unified
    .arch       armv7-m
    .section    .stack
    .align      3
#ifdef __STACK_SIZE
    .equ        Stack_Size, __STACK_SIZE
#else
    .equ        Stack_Size, 0x00000400
#endif
    .globl      __StackTop
    .globl      __StackLimit
__StackLimit:
    .space      Stack_Size
    .size       __StackLimit, . - __StackLimit
__StackTop:
    .size       __StackTop, . - __StackTop

    .section    .heap
    .align      3
#ifdef __HEAP_SIZE
    .equ        Heap_Size, __HEAP_SIZE
#else
    .equ        Heap_Size, 0x00000C00
#endif
    .globl      __HeapBase
    .globl      __HeapLimit
__HeapBase:
    .if Heap_Size
    .space      Heap_Size
    .endif
    .size       __HeapBase, . - __HeapBase
__HeapLimit:
    .size       __HeapLimit, . - __HeapLimit

    .section    .vectors
    .align      2
    .globl      __Vectors
__Vectors:
    .long       __StackTop                 /* Top of Stack */
    .long       Reset_Handler              /* Reset Handler */
    .long       NMI_Handler                /* NMI Handler */
    .long       HardFault_Handler          /* Hard Fault Handler */
    .long       MemManage_Handler          /* MPU Fault Handler */
    .long       BusFault_Handler           /* Bus Fault Handler */
    .long       UsageFault_Handler         /* Usage Fault Handler */
    .long       Default_Handler            /* Reserved */
    .long       Default_Handler            /* Reserved */
    .long       Default_Handler            /* Reserved */
    .long       Default_Handler            /* Reserved */
    .long       SVC_Handler                /* SVCall Handler */
    .long       DebugMon_Handler           /* Debug Monitor Handler */
    .long       Default_Handler            /* Reserved */
    .long       PendSV_Handler             /* PendSV Handler */
    .long       SysTick_Handler            /* SysTick Handler */

    /* External interrupts */
    .long       EMU_IRQHandler             /* 0 - EMU */
    .long       Default_Handler            /* 1 - Reserved */
    .long       WDOG0_IRQHandler           /* 2 - WDOG0 */
    .long       WDOG1_IRQHandler           /* 3 - WDOG1 */
    .long       Default_Handler            /* 4 - Reserved */
    .long       Default_Handler            /* 5 - Reserved */
    .long       Default_Handler            /* 6 - Reserved */
    .long       Default_Handler            /* 7 - Reserved */
    .long       Default_Handler            /* 8 - Reserved */
    .long       LDMA_IRQHandler            /* 9 - LDMA */
    .long       GPIO_EVEN_IRQHandler       /* 10 - GPIO_EVEN */
    .long       TIMER0_IRQHandler          /* 11 - TIMER0 */
    .long       USART0_RX_IRQHandler       /* 12 - USART0_RX */
    .long       USART0_TX_IRQHandler       /* 13 - USART0_TX */
    .long       ACMP0_IRQHandler           /* 14 - ACMP0 */
    .long       ADC0_IRQHandler            /* 15 - ADC0 */
    .long       IDAC0_IRQHandler           /* 16 - IDAC0 */
    .long       I2C0_IRQHandler            /* 17 - I2C0 */
    .long       GPIO_ODD_IRQHandler        /* 18 - GPIO_ODD */
    .long       TIMER1_IRQHandler          /* 19 - TIMER1 */
    .long       USART1_RX_IRQHandler       /* 20 - USART1_RX */
    .long       USART1_TX_IRQHandler       /* 21 - USART1_TX */
    .long       LEUART0_IRQHandler         /* 22 - LEUART0 */
    .long       PCNT0_IRQHandler           /* 23 - PCNT0 */
    .long       CMU_IRQHandler             /* 24 - CMU */
    .long       MSC_IRQHandler             /* 25 - MSC */
    .long       CRYPTO0_IRQHandler         /* 26 - CRYPTO0 */
    .long       LETIMER0_IRQHandler        /* 27 - LETIMER0 */
    .long       Default_Handler            /* 28 - Reserved */
    .long       Default_Handler            /* 29 - Reserved */
    .long       RTCC_IRQHandler            /* 30 - RTCC */
    .long       Default_Handler            /* 31 - Reserved */
    .long       CRYOTIMER_IRQHandler       /* 32 - CRYOTIMER */
    .long       Default_Handler            /* 33 - Reserved */
    .long       FPUEH_IRQHandler           /* 34 - FPUEH */
    .long       SMU_IRQHandler             /* 35 - SMU */
    .long       WTIMER0_IRQHandler         /* 36 - WTIMER0 */
    .long       WTIMER1_IRQHandler         /* 37 - WTIMER1 */
    .long       PCNT1_IRQHandler           /* 38 - PCNT1 */
    .long       PCNT2_IRQHandler           /* 39 - PCNT2 */
    .long       USART2_RX_IRQHandler       /* 40 - USART2_RX */
    .long       USART2_TX_IRQHandler       /* 41 - USART2_TX */
    .long       I2C1_IRQHandler            /* 42 - I2C1 */
    .long       USART3_RX_IRQHandler       /* 43 - USART3_RX */
    .long       USART3_TX_IRQHandler       /* 44 - USART3_TX */
    .long       VDAC0_IRQHandler           /* 45 - VDAC0 */
    .long       CSEN_IRQHandler            /* 46 - CSEN */
    .long       LESENSE_IRQHandler         /* 47 - LESENSE */
    .long       CRYPTO1_IRQHandler         /* 48 - CRYPTO1 */
    .long       TRNG0_IRQHandler           /* 49 - TRNG0 */
    .long       Default_Handler            /* 50 - Reserved */


    .size       __Vectors, . - __Vectors

    .text
    .thumb
    .thumb_func
    .align      2
    .globl      Reset_Handler
    .type       Reset_Handler, %function
Reset_Handler:
#ifndef __NO_SYSTEM_INIT
    ldr     r0, =SystemInit
    blx     r0
#endif

/*  Firstly it copies data from read only memory to RAM. There are two schemes
 *  to copy. One can copy more than one sections. Another can only copy
 *  one section.  The former scheme needs more instructions and read-only
 *  data to implement than the latter.
 *  Macro __STARTUP_COPY_MULTIPLE is used to choose between two schemes.  */

#ifdef __STARTUP_COPY_MULTIPLE
/*  Multiple sections scheme.
 *
 *  Between symbol address __copy_table_start__ and __copy_table_end__,
 *  there are array of triplets, each of which specify:
 *    offset 0: LMA of start of a section to copy from
 *    offset 4: VMA of start of a section to copy to
 *    offset 8: size of the section to copy. Must be multiply of 4
 *
 *  All addresses must be aligned to 4 bytes boundary.
 */
    ldr     r4, =__copy_table_start__
    ldr     r5, =__copy_table_end__

.L_loop0:
    cmp     r4, r5
    bge     .L_loop0_done
    ldr     r1, [r4]
    ldr     r2, [r4, #4]
    ldr     r3, [r4, #8]

.L_loop0_0:
    subs    r3, #4
    ittt    ge
    ldrge   r0, [r1, r3]
    strge   r0, [r2, r3]
    bge     .L_loop0_0

    adds    r4, #12
    b       .L_loop0

.L_loop0_done:
#else
/*  Single section scheme.
 *
 *  The ranges of copy from/to are specified by following symbols
 *    __etext: LMA of start of the section to copy from. Usually end of text
 *    __data_start__: VMA of start of the section to copy to
 *    __data_end__: VMA of end of the section to copy to
 *
 *  All addresses must be aligned to 4 bytes boundary.
 */
    ldr     r1, =__etext
    ldr     r2, =__data_start__
    ldr     r3, =__data_end__

.L_loop1:
    cmp     r2, r3
    ittt    lt
    ldrlt   r0, [r1], #4
    strlt   r0, [r2], #4
    blt     .L_loop1
#endif /*__STARTUP_COPY_MULTIPLE */

/*  This part of work usually is done in C library startup code. Otherwise,
 *  define this macro to enable it in this startup.
 *
 *  There are two schemes too. One can clear multiple BSS sections. Another
 *  can only clear one section. The former is more size expensive than the
 *  latter.
 *
 *  Define macro __STARTUP_CLEAR_BSS_MULTIPLE to choose the former.
 *  Otherwise efine macro __STARTUP_CLEAR_BSS to choose the later.
 */
#ifdef __STARTUP_CLEAR_BSS_MULTIPLE
/*  Multiple sections scheme.
 *
 *  Between symbol address __zero_table_start__ and __zero_table_end__,
 *  there are array of tuples specifying:
 *    offset 0: Start of a BSS section
 *    offset 4: Size of this BSS section. Must be multiply of 4
 */
    ldr     r3, =__zero_table_start__
    ldr     r4, =__zero_table_end__

.L_loop2:
    cmp     r3, r4
    bge     .L_loop2_done
    ldr     r1, [r3]
    ldr     r2, [r3, #4]
    movs    r0, 0

.L_loop2_0:
    subs    r2, #4
    itt     ge
    strge   r0, [r1, r2]
    bge     .L_loop2_0
    adds    r3, #8
    b       .L_loop2
.L_loop2_done:
#elif defined (__STARTUP_CLEAR_BSS)
/*  Single BSS section scheme.
 *
 *  The BSS section is specified by following symbols
 *    __bss_start__: start of the BSS section.
 *    __bss_end__: end of the BSS section.
 *
 *  Both addresses must be aligned to 4 bytes boundary.
 */
    ldr     r1, =__bss_start__
    ldr     r2, =__bss_end__

    movs    r0, 0
.L_loop3:
    cmp     r1, r2
    itt     lt
    strlt   r0, [r1], #4
    blt     .L_loop3
#endif /* __STARTUP_CLEAR_BSS_MULTIPLE || __STARTUP_CLEAR_BSS */

#ifndef __START
#define __START _start
#endif
    bl      __START

    .pool
    .size   Reset_Handler, . - Reset_Handler

    .align  1
    .thumb_func
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       .
    .size   Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro  def_irq_handler	handler_name
    .weak   \handler_name
    .set    \handler_name, Default_Handler
    .endm

    def_irq_handler     NMI_Handler
    def_irq_handler     HardFault_Handler
    def_irq_handler     MemManage_Handler
    def_irq_handler     BusFault_Handler
    def_irq_handler     UsageFault_Handler
    def_irq_handler     SVC_Handler
    def_irq_handler     DebugMon_Handler
    def_irq_handler     PendSV_Handler
    def_irq_handler     SysTick_Handler


    def_irq_handler     EMU_IRQHandler
    def_irq_handler     WDOG0_IRQHandler
    def_irq_handler     WDOG1_IRQHandler
    def_irq_handler     LDMA_IRQHandler
    def_irq_handler     GPIO_EVEN_IRQHandler
    def_irq_handler     TIMER0_IRQHandler
    def_irq_handler     USART0_RX_IRQHandler
    def_irq_handler     USART0_TX_IRQHandler
    def_irq_handler     ACMP0_IRQHandler
    def_irq_handler     ADC0_IRQHandler
    def_irq_handler     IDAC0_IRQHandler
    def_irq_handler     I2C0_IRQHandler
    def_irq_handler     GPIO_ODD_IRQHandler
    def_irq_handler     TIMER1_IRQHandler
    def_irq_handler     USART1_RX_IRQHandler
    def_irq_handler     USART1_TX_IRQHandler
    def_irq_handler     LEUART0_IRQHandler
    def_irq_handler     PCNT0_IRQHandler
    def_irq_handler     CMU_IRQHandler
    def_irq_handler     MSC_IRQHandler
    def_irq_handler     CRYPTO0_IRQHandler
    def_irq_handler     LETIMER0_IRQHandler
    def_irq_handler     RTCC_IRQHandler
    def_irq_handler     CRYOTIMER_IRQHandler
    def_irq_handler     FPUEH_IRQHandler
    def_irq_handler     SMU_IRQHandler
    def_irq_handler     WTIMER0_IRQHandler
    def_irq_handler     WTIMER1_IRQHandler
    def_irq_handler     PCNT1_IRQHandler
    def_irq_handler     PCNT2_IRQHandler
    def_irq_handler     USART2_RX_IRQHandler
    def_irq_handler     USART2_TX_IRQHandler
    def_irq_handler     I2C1_IRQHandler
    def_irq_handler     USART3_RX_IRQHandler
    def_irq_handler     USART3_TX_IRQHandler
    def_irq_handler     VDAC0_IRQHandler
    def_irq_handler     CSEN_IRQHandler
    def_irq_handler     LESENSE_IRQHandler
    def_irq_handler     CRYPTO1_IRQHandler
    def_irq_handler     TRNG0_IRQHandler

    .end
