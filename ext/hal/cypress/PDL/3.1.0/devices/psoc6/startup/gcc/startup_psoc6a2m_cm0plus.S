/**************************************************************************//**
 * @file     startup_psoc6a2m_cm0plus.s
 * @brief    CMSIS Core Device Startup File for
 *           ARMCM0plus Device Series
 * @version  V5.00
 * @date     02. March 2016
 ******************************************************************************/
/*
 * Copyright (c) 2018 Cypress Semiconductor
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

    /* Address of the NMI handler */
    #define CY_NMI_HANLDER_ADDR         0x0000000D

    /* The CPU VTOR register */
    #define CY_CPU_VTOR_ADDR                 0xE000ED08

    /* Copy flash vectors and data section to RAM */
    #define __STARTUP_COPY_MULTIPLE

    /* Clear single BSS section */
    #define __STARTUP_CLEAR_BSS

    .syntax    unified
    .arch    armv6-m

    .section .stack
    .align    3
#ifdef __STACK_SIZE
    .equ    Stack_Size, __STACK_SIZE
#else
    .equ    Stack_Size, 0x00001000
#endif
    .globl    __StackTop
    .globl    __StackLimit
__StackLimit:
    .space    Stack_Size
    .size    __StackLimit, . - __StackLimit
__StackTop:
    .size    __StackTop, . - __StackTop

    .section .heap
    .align    3
#ifdef __HEAP_SIZE
    .equ    Heap_Size, __HEAP_SIZE
#else
    .equ    Heap_Size, 0x00000400
#endif
    .globl    __HeapBase
    .globl    __HeapLimit
__HeapBase:
    .if    Heap_Size
    .space    Heap_Size
    .endif
    .size    __HeapBase, . - __HeapBase
__HeapLimit:
    .size    __HeapLimit, . - __HeapLimit

    .section .vectors
    .align 2
    .globl    __Vectors
__Vectors:
    .long    __StackTop            /* Top of Stack */
    .long    Reset_Handler         /* Reset Handler */
    .long    CY_NMI_HANLDER_ADDR   /* NMI Handler */
    .long    HardFault_Handler     /* Hard Fault Handler */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    SVC_Handler           /* SVCall Handler */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    PendSV_Handler        /* PendSV Handler */
    .long    SysTick_Handler       /* SysTick Handler */

     /* External interrupts                             Description */
    .long    NvicMux0_IRQHandler                     /* CM0 + NVIC Mux input 0 */
    .long    NvicMux1_IRQHandler                     /* CM0 + NVIC Mux input 1 */
    .long    NvicMux2_IRQHandler                     /* CM0 + NVIC Mux input 2 */
    .long    NvicMux3_IRQHandler                     /* CM0 + NVIC Mux input 3 */
    .long    NvicMux4_IRQHandler                     /* CM0 + NVIC Mux input 4 */
    .long    NvicMux5_IRQHandler                     /* CM0 + NVIC Mux input 5 */
    .long    NvicMux6_IRQHandler                     /* CM0 + NVIC Mux input 6 */
    .long    NvicMux7_IRQHandler                     /* CM0 + NVIC Mux input 7 */
    .long    NvicMux8_IRQHandler                     /* CM0 + NVIC Mux input 8 */
    .long    NvicMux9_IRQHandler                     /* CM0 + NVIC Mux input 9 */
    .long    NvicMux10_IRQHandler                    /* CM0 + NVIC Mux input 10 */
    .long    NvicMux11_IRQHandler                    /* CM0 + NVIC Mux input 11 */
    .long    NvicMux12_IRQHandler                    /* CM0 + NVIC Mux input 12 */
    .long    NvicMux13_IRQHandler                    /* CM0 + NVIC Mux input 13 */
    .long    NvicMux14_IRQHandler                    /* CM0 + NVIC Mux input 14 */
    .long    NvicMux15_IRQHandler                    /* CM0 + NVIC Mux input 15 */
    .long    NvicMux16_IRQHandler                    /* CM0 + NVIC Mux input 16 */
    .long    NvicMux17_IRQHandler                    /* CM0 + NVIC Mux input 17 */
    .long    NvicMux18_IRQHandler                    /* CM0 + NVIC Mux input 18 */
    .long    NvicMux19_IRQHandler                    /* CM0 + NVIC Mux input 19 */
    .long    NvicMux20_IRQHandler                    /* CM0 + NVIC Mux input 20 */
    .long    NvicMux21_IRQHandler                    /* CM0 + NVIC Mux input 21 */
    .long    NvicMux22_IRQHandler                    /* CM0 + NVIC Mux input 22 */
    .long    NvicMux23_IRQHandler                    /* CM0 + NVIC Mux input 23 */
    .long    NvicMux24_IRQHandler                    /* CM0 + NVIC Mux input 24 */
    .long    NvicMux25_IRQHandler                    /* CM0 + NVIC Mux input 25 */
    .long    NvicMux26_IRQHandler                    /* CM0 + NVIC Mux input 26 */
    .long    NvicMux27_IRQHandler                    /* CM0 + NVIC Mux input 27 */
    .long    NvicMux28_IRQHandler                    /* CM0 + NVIC Mux input 28 */
    .long    NvicMux29_IRQHandler                    /* CM0 + NVIC Mux input 29 */
    .long    NvicMux30_IRQHandler                    /* CM0 + NVIC Mux input 30 */
    .long    NvicMux31_IRQHandler                    /* CM0 + NVIC Mux input 31 */

    .size    __Vectors, . - __Vectors
    .equ    __VectorsSize, . - __Vectors

    .section .ram_vectors
    .align 2
    .globl __ramVectors
__ramVectors:
    .space  __VectorsSize
    .size   __ramVectors, . - __ramVectors


    .text
    .thumb
    .thumb_func
    .align  2

    /* Device startup customization */
    .weak   Cy_OnResetUser
#ifndef __clang__
    .func   Cy_OnResetUser, Cy_OnResetUser
#endif
    .type   Cy_OnResetUser, %function

Cy_OnResetUser:
    bx lr
    .size   Cy_OnResetUser, . - Cy_OnResetUser
#ifndef __clang__
    .endfunc
#endif

    /* Saves and disables the interrupts */
#ifndef __clang__
    .global Cy_SaveIRQ
    .func   Cy_SaveIRQ, Cy_SaveIRQ
    .type   Cy_SaveIRQ, %function
Cy_SaveIRQ:
#else
    .global _Cy_SaveIRQ
    .type   _Cy_SaveIRQ, %function
_Cy_SaveIRQ:    
#endif

    mrs r0, PRIMASK
    cpsid i
    bx lr
    .size   Cy_SaveIRQ, . - Cy_SaveIRQ
#ifndef __clang__    
    .endfunc
#endif

    /* Restores the interrupts */
#ifndef __clang__    
    .global Cy_RestoreIRQ
    .func   Cy_RestoreIRQ, Cy_RestoreIRQ
    .type   Cy_RestoreIRQ, %function
Cy_RestoreIRQ:
#else
    .global _Cy_RestoreIRQ
    .type   _Cy_RestoreIRQ, %function
_Cy_RestoreIRQ:
#endif

    msr PRIMASK, r0
    bx lr
    .size   Cy_RestoreIRQ, . - Cy_RestoreIRQ
#ifndef __clang__    
    .endfunc
#endif

    /* Reset handler */
    .weak    Reset_Handler
    .type    Reset_Handler, %function

Reset_Handler:
    bl Cy_OnResetUser
    cpsid i

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
    ldr    r4, =__copy_table_start__
    ldr    r5, =__copy_table_end__

.L_loop0:
    cmp    r4, r5
    bge    .L_loop0_done
    ldr    r1, [r4]
    ldr    r2, [r4, #4]
    ldr    r3, [r4, #8]

.L_loop0_0:
    subs    r3, #4
    blt    .L_loop0_0_done
    ldr    r0, [r1, r3]
    str    r0, [r2, r3]
    b    .L_loop0_0

.L_loop0_0_done:
    adds    r4, #12
    b    .L_loop0

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
    ldr    r1, =__etext
    ldr    r2, =__data_start__
    ldr    r3, =__data_end__

    subs    r3, r2
    ble    .L_loop1_done

.L_loop1:
    subs    r3, #4
    ldr    r0, [r1,r3]
    str    r0, [r2,r3]
    bgt    .L_loop1

.L_loop1_done:
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
 *  Between symbol address __copy_table_start__ and __copy_table_end__,
 *  there are array of tuples specifying:
 *    offset 0: Start of a BSS section
 *    offset 4: Size of this BSS section. Must be multiply of 4
 */
    ldr    r3, =__zero_table_start__
    ldr    r4, =__zero_table_end__

.L_loop2:
    cmp    r3, r4
    bge    .L_loop2_done
    ldr    r1, [r3]
    ldr    r2, [r3, #4]
    movs    r0, 0

.L_loop2_0:
    subs    r2, #4
    blt    .L_loop2_0_done
    str    r0, [r1, r2]
    b    .L_loop2_0
.L_loop2_0_done:

    adds    r3, #8
    b    .L_loop2
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
    ldr    r1, =__bss_start__
    ldr    r2, =__bss_end__

    movs    r0, 0

    subs    r2, r1
    ble    .L_loop3_done

.L_loop3:
    subs    r2, #4
    str    r0, [r1, r2]
    bgt    .L_loop3
.L_loop3_done:
#endif /* __STARTUP_CLEAR_BSS_MULTIPLE || __STARTUP_CLEAR_BSS */

    /* Update Vector Table Offset Register. */
    ldr r0, =__ramVectors
    ldr r1, =CY_CPU_VTOR_ADDR
    str r0, [r1]
    dsb 0xF

#ifndef __NO_SYSTEM_INIT
    bl    SystemInit
#endif

    bl    main

    /* Should never get here */
    b   .

    .pool
    .size    Reset_Handler, . - Reset_Handler

    .align    1
    .thumb_func
    .weak    Default_Handler
    .type    Default_Handler, %function
Default_Handler:
    b    .
    .size    Default_Handler, . - Default_Handler
    .weak    Cy_SysLib_FaultHandler
    .type    Cy_SysLib_FaultHandler, %function

Cy_SysLib_FaultHandler:
    b    .
    .size    Cy_SysLib_FaultHandler, . - Cy_SysLib_FaultHandler
    .type Fault_Handler, %function

Fault_Handler:
    /* Storing LR content for Creator call stack trace */
    push {LR}
    movs r0, #4
    mov r1, LR
    tst r0, r1
    beq .L_MSP
    mrs r0, PSP
    b .L_API_call
.L_MSP:
    mrs r0, MSP
.L_API_call:
    /* Compensation of stack pointer address due to pushing 4 bytes of LR */
    adds r0, r0, #4
    bl Cy_SysLib_FaultHandler
    b   .
    .size    Fault_Handler, . - Fault_Handler

.macro    def_fault_Handler    fault_handler_name
    .weak    \fault_handler_name
    .set    \fault_handler_name, Fault_Handler
    .endm

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro    def_irq_handler    handler_name
    .weak    \handler_name
    .set    \handler_name, Default_Handler
    .endm

    def_irq_handler    NMI_Handler

    def_fault_Handler  HardFault_Handler

    def_irq_handler    SVC_Handler
    def_irq_handler    PendSV_Handler
    def_irq_handler    SysTick_Handler

    def_irq_handler  NvicMux0_IRQHandler                     /* CM0 + NVIC Mux input 0 */
    def_irq_handler  NvicMux1_IRQHandler                     /* CM0 + NVIC Mux input 1 */
    def_irq_handler  NvicMux2_IRQHandler                     /* CM0 + NVIC Mux input 2 */
    def_irq_handler  NvicMux3_IRQHandler                     /* CM0 + NVIC Mux input 3 */
    def_irq_handler  NvicMux4_IRQHandler                     /* CM0 + NVIC Mux input 4 */
    def_irq_handler  NvicMux5_IRQHandler                     /* CM0 + NVIC Mux input 5 */
    def_irq_handler  NvicMux6_IRQHandler                     /* CM0 + NVIC Mux input 6 */
    def_irq_handler  NvicMux7_IRQHandler                     /* CM0 + NVIC Mux input 7 */
    def_irq_handler  NvicMux8_IRQHandler                     /* CM0 + NVIC Mux input 8 */
    def_irq_handler  NvicMux9_IRQHandler                     /* CM0 + NVIC Mux input 9 */
    def_irq_handler  NvicMux10_IRQHandler                    /* CM0 + NVIC Mux input 10 */
    def_irq_handler  NvicMux11_IRQHandler                    /* CM0 + NVIC Mux input 11 */
    def_irq_handler  NvicMux12_IRQHandler                    /* CM0 + NVIC Mux input 12 */
    def_irq_handler  NvicMux13_IRQHandler                    /* CM0 + NVIC Mux input 13 */
    def_irq_handler  NvicMux14_IRQHandler                    /* CM0 + NVIC Mux input 14 */
    def_irq_handler  NvicMux15_IRQHandler                    /* CM0 + NVIC Mux input 15 */
    def_irq_handler  NvicMux16_IRQHandler                    /* CM0 + NVIC Mux input 16 */
    def_irq_handler  NvicMux17_IRQHandler                    /* CM0 + NVIC Mux input 17 */
    def_irq_handler  NvicMux18_IRQHandler                    /* CM0 + NVIC Mux input 18 */
    def_irq_handler  NvicMux19_IRQHandler                    /* CM0 + NVIC Mux input 19 */
    def_irq_handler  NvicMux20_IRQHandler                    /* CM0 + NVIC Mux input 20 */
    def_irq_handler  NvicMux21_IRQHandler                    /* CM0 + NVIC Mux input 21 */
    def_irq_handler  NvicMux22_IRQHandler                    /* CM0 + NVIC Mux input 22 */
    def_irq_handler  NvicMux23_IRQHandler                    /* CM0 + NVIC Mux input 23 */
    def_irq_handler  NvicMux24_IRQHandler                    /* CM0 + NVIC Mux input 24 */
    def_irq_handler  NvicMux25_IRQHandler                    /* CM0 + NVIC Mux input 25 */
    def_irq_handler  NvicMux26_IRQHandler                    /* CM0 + NVIC Mux input 26 */
    def_irq_handler  NvicMux27_IRQHandler                    /* CM0 + NVIC Mux input 27 */
    def_irq_handler  NvicMux28_IRQHandler                    /* CM0 + NVIC Mux input 28 */
    def_irq_handler  NvicMux29_IRQHandler                    /* CM0 + NVIC Mux input 29 */
    def_irq_handler  NvicMux30_IRQHandler                    /* CM0 + NVIC Mux input 30 */
    def_irq_handler  NvicMux31_IRQHandler                    /* CM0 + NVIC Mux input 31 */

    .end


/* [] END OF FILE */
