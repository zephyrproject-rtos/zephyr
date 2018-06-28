;/**************************************************************************//**
; * @file     startup_psoc61_cm4.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM4 Device Series
; * @version  V5.00
; * @date     08. March 2016
; ******************************************************************************/
;/*
; * Copyright (c) 2018 Cypress Semiconductor
; * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
; *
; * SPDX-License-Identifier: Apache-2.0
; *
; * Licensed under the Apache License, Version 2.0 (the License); you may
; * not use this file except in compliance with the License.
; * You may obtain a copy of the License at
; *
; * www.apache.org/licenses/LICENSE-2.0
; *
; * Unless required by applicable law or agreed to in writing, software
; * distributed under the License is distributed on an AS IS BASIS, WITHOUT
; * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; * See the License for the specific language governing permissions and
; * limitations under the License.
; */

;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbol _program_start or
; a user defined start symbol.
; To override the cstartup defined in the library, simply add your modified
; version to the workbench project.
;
; The vector table is normally located at address 0.
; When debugging in RAM, it can be located in RAM, aligned to at least 2^6.
; The name "__vector_table" has special meaning for C-SPY:
; it is where the SP start value is found, and the NVIC vector
; table register (VTOR) is initialized to this address if != 0.
;
; Cortex-M version
;

        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)
        SECTION .intvec_ram:DATA:NOROOT(2)
        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        EXTERN  SystemInit
        EXTERN  Cy_SystemInitFpuEnable
        EXTERN __iar_data_init3
        PUBLIC  __vector_table
        PUBLIC  __vector_table_0x1c
        PUBLIC  __Vectors
        PUBLIC  __Vectors_End
        PUBLIC  __Vectors_Size
        PUBLIC  __ramVectors

        DATA

__vector_table
        DCD     sfe(CSTACK)
        DCD     Reset_Handler

        DCD     0x0000000D              ; NMI_Handler is defined in ROM code
        DCD     HardFault_Handler
        DCD     MemManage_Handler
        DCD     BusFault_Handler
        DCD     UsageFault_Handler
__vector_table_0x1c
        DCD     0
        DCD     0
        DCD     0
        DCD     0
        DCD     SVC_Handler
        DCD     DebugMon_Handler
        DCD     0
        DCD     PendSV_Handler
        DCD     SysTick_Handler


        ; External interrupts                           Description
        DCD     ioss_interrupts_gpio_0_IRQHandler     ; GPIO Port Interrupt #0 
        DCD     ioss_interrupts_gpio_1_IRQHandler     ; GPIO Port Interrupt #1 
        DCD     ioss_interrupts_gpio_2_IRQHandler     ; GPIO Port Interrupt #2 
        DCD     ioss_interrupts_gpio_3_IRQHandler     ; GPIO Port Interrupt #3 
        DCD     ioss_interrupts_gpio_4_IRQHandler     ; GPIO Port Interrupt #4 
        DCD     ioss_interrupts_gpio_5_IRQHandler     ; GPIO Port Interrupt #5 
        DCD     ioss_interrupts_gpio_6_IRQHandler     ; GPIO Port Interrupt #6 
        DCD     ioss_interrupts_gpio_7_IRQHandler     ; GPIO Port Interrupt #7 
        DCD     ioss_interrupts_gpio_8_IRQHandler     ; GPIO Port Interrupt #8 
        DCD     ioss_interrupts_gpio_9_IRQHandler     ; GPIO Port Interrupt #9 
        DCD     ioss_interrupts_gpio_10_IRQHandler    ; GPIO Port Interrupt #10 
        DCD     ioss_interrupts_gpio_11_IRQHandler    ; GPIO Port Interrupt #11 
        DCD     ioss_interrupts_gpio_12_IRQHandler    ; GPIO Port Interrupt #12 
        DCD     ioss_interrupts_gpio_13_IRQHandler    ; GPIO Port Interrupt #13 
        DCD     ioss_interrupts_gpio_14_IRQHandler    ; GPIO Port Interrupt #14 
        DCD     ioss_interrupt_gpio_IRQHandler        ; GPIO All Ports 
        DCD     ioss_interrupt_vdd_IRQHandler         ; GPIO Supply Detect Interrupt 
        DCD     lpcomp_interrupt_IRQHandler           ; Low Power Comparator Interrupt 
        DCD     scb_8_interrupt_IRQHandler            ; Serial Communication Block #8 (DeepSleep capable) 
        DCD     srss_interrupt_mcwdt_0_IRQHandler     ; Multi Counter Watchdog Timer interrupt 
        DCD     srss_interrupt_mcwdt_1_IRQHandler     ; Multi Counter Watchdog Timer interrupt 
        DCD     srss_interrupt_backup_IRQHandler      ; Backup domain interrupt 
        DCD     srss_interrupt_IRQHandler             ; Other combined Interrupts for SRSS (LVD, WDT, CLKCAL) 
        DCD     pass_interrupt_ctbs_IRQHandler        ; CTBm Interrupt (all CTBms) 
        DCD     bless_interrupt_IRQHandler            ; Bluetooth Radio interrupt 
        DCD     cpuss_interrupts_ipc_0_IRQHandler     ; CPUSS Inter Process Communication Interrupt #0 
        DCD     cpuss_interrupts_ipc_1_IRQHandler     ; CPUSS Inter Process Communication Interrupt #1 
        DCD     cpuss_interrupts_ipc_2_IRQHandler     ; CPUSS Inter Process Communication Interrupt #2 
        DCD     cpuss_interrupts_ipc_3_IRQHandler     ; CPUSS Inter Process Communication Interrupt #3 
        DCD     cpuss_interrupts_ipc_4_IRQHandler     ; CPUSS Inter Process Communication Interrupt #4 
        DCD     cpuss_interrupts_ipc_5_IRQHandler     ; CPUSS Inter Process Communication Interrupt #5 
        DCD     cpuss_interrupts_ipc_6_IRQHandler     ; CPUSS Inter Process Communication Interrupt #6 
        DCD     cpuss_interrupts_ipc_7_IRQHandler     ; CPUSS Inter Process Communication Interrupt #7 
        DCD     cpuss_interrupts_ipc_8_IRQHandler     ; CPUSS Inter Process Communication Interrupt #8 
        DCD     cpuss_interrupts_ipc_9_IRQHandler     ; CPUSS Inter Process Communication Interrupt #9 
        DCD     cpuss_interrupts_ipc_10_IRQHandler    ; CPUSS Inter Process Communication Interrupt #10 
        DCD     cpuss_interrupts_ipc_11_IRQHandler    ; CPUSS Inter Process Communication Interrupt #11 
        DCD     cpuss_interrupts_ipc_12_IRQHandler    ; CPUSS Inter Process Communication Interrupt #12 
        DCD     cpuss_interrupts_ipc_13_IRQHandler    ; CPUSS Inter Process Communication Interrupt #13 
        DCD     cpuss_interrupts_ipc_14_IRQHandler    ; CPUSS Inter Process Communication Interrupt #14 
        DCD     cpuss_interrupts_ipc_15_IRQHandler    ; CPUSS Inter Process Communication Interrupt #15 
        DCD     scb_0_interrupt_IRQHandler            ; Serial Communication Block #0 
        DCD     scb_1_interrupt_IRQHandler            ; Serial Communication Block #1 
        DCD     scb_2_interrupt_IRQHandler            ; Serial Communication Block #2 
        DCD     scb_3_interrupt_IRQHandler            ; Serial Communication Block #3 
        DCD     scb_4_interrupt_IRQHandler            ; Serial Communication Block #4 
        DCD     scb_5_interrupt_IRQHandler            ; Serial Communication Block #5 
        DCD     scb_6_interrupt_IRQHandler            ; Serial Communication Block #6 
        DCD     scb_7_interrupt_IRQHandler            ; Serial Communication Block #7 
        DCD     csd_interrupt_IRQHandler              ; CSD (Capsense) interrupt 
        DCD     cpuss_interrupts_dw0_0_IRQHandler     ; CPUSS DataWire #0, Channel #0 
        DCD     cpuss_interrupts_dw0_1_IRQHandler     ; CPUSS DataWire #0, Channel #1 
        DCD     cpuss_interrupts_dw0_2_IRQHandler     ; CPUSS DataWire #0, Channel #2 
        DCD     cpuss_interrupts_dw0_3_IRQHandler     ; CPUSS DataWire #0, Channel #3 
        DCD     cpuss_interrupts_dw0_4_IRQHandler     ; CPUSS DataWire #0, Channel #4 
        DCD     cpuss_interrupts_dw0_5_IRQHandler     ; CPUSS DataWire #0, Channel #5 
        DCD     cpuss_interrupts_dw0_6_IRQHandler     ; CPUSS DataWire #0, Channel #6 
        DCD     cpuss_interrupts_dw0_7_IRQHandler     ; CPUSS DataWire #0, Channel #7 
        DCD     cpuss_interrupts_dw0_8_IRQHandler     ; CPUSS DataWire #0, Channel #8 
        DCD     cpuss_interrupts_dw0_9_IRQHandler     ; CPUSS DataWire #0, Channel #9 
        DCD     cpuss_interrupts_dw0_10_IRQHandler    ; CPUSS DataWire #0, Channel #10 
        DCD     cpuss_interrupts_dw0_11_IRQHandler    ; CPUSS DataWire #0, Channel #11 
        DCD     cpuss_interrupts_dw0_12_IRQHandler    ; CPUSS DataWire #0, Channel #12 
        DCD     cpuss_interrupts_dw0_13_IRQHandler    ; CPUSS DataWire #0, Channel #13 
        DCD     cpuss_interrupts_dw0_14_IRQHandler    ; CPUSS DataWire #0, Channel #14 
        DCD     cpuss_interrupts_dw0_15_IRQHandler    ; CPUSS DataWire #0, Channel #15 
        DCD     cpuss_interrupts_dw1_0_IRQHandler     ; CPUSS DataWire #1, Channel #0 
        DCD     cpuss_interrupts_dw1_1_IRQHandler     ; CPUSS DataWire #1, Channel #1 
        DCD     cpuss_interrupts_dw1_2_IRQHandler     ; CPUSS DataWire #1, Channel #2 
        DCD     cpuss_interrupts_dw1_3_IRQHandler     ; CPUSS DataWire #1, Channel #3 
        DCD     cpuss_interrupts_dw1_4_IRQHandler     ; CPUSS DataWire #1, Channel #4 
        DCD     cpuss_interrupts_dw1_5_IRQHandler     ; CPUSS DataWire #1, Channel #5 
        DCD     cpuss_interrupts_dw1_6_IRQHandler     ; CPUSS DataWire #1, Channel #6 
        DCD     cpuss_interrupts_dw1_7_IRQHandler     ; CPUSS DataWire #1, Channel #7 
        DCD     cpuss_interrupts_dw1_8_IRQHandler     ; CPUSS DataWire #1, Channel #8 
        DCD     cpuss_interrupts_dw1_9_IRQHandler     ; CPUSS DataWire #1, Channel #9 
        DCD     cpuss_interrupts_dw1_10_IRQHandler    ; CPUSS DataWire #1, Channel #10 
        DCD     cpuss_interrupts_dw1_11_IRQHandler    ; CPUSS DataWire #1, Channel #11 
        DCD     cpuss_interrupts_dw1_12_IRQHandler    ; CPUSS DataWire #1, Channel #12 
        DCD     cpuss_interrupts_dw1_13_IRQHandler    ; CPUSS DataWire #1, Channel #13 
        DCD     cpuss_interrupts_dw1_14_IRQHandler    ; CPUSS DataWire #1, Channel #14 
        DCD     cpuss_interrupts_dw1_15_IRQHandler    ; CPUSS DataWire #1, Channel #15 
        DCD     cpuss_interrupts_fault_0_IRQHandler   ; CPUSS Fault Structure Interrupt #0 
        DCD     cpuss_interrupts_fault_1_IRQHandler   ; CPUSS Fault Structure Interrupt #1 
        DCD     cpuss_interrupt_crypto_IRQHandler     ; CRYPTO Accelerator Interrupt 
        DCD     cpuss_interrupt_fm_IRQHandler         ; FLASH Macro Interrupt 
        DCD     cpuss_interrupts_cm0_cti_0_IRQHandler ; CM0+ CTI #0 
        DCD     cpuss_interrupts_cm0_cti_1_IRQHandler ; CM0+ CTI #1 
        DCD     cpuss_interrupts_cm4_cti_0_IRQHandler ; CM4 CTI #0 
        DCD     cpuss_interrupts_cm4_cti_1_IRQHandler ; CM4 CTI #1 
        DCD     tcpwm_0_interrupts_0_IRQHandler       ; TCPWM #0, Counter #0 
        DCD     tcpwm_0_interrupts_1_IRQHandler       ; TCPWM #0, Counter #1 
        DCD     tcpwm_0_interrupts_2_IRQHandler       ; TCPWM #0, Counter #2 
        DCD     tcpwm_0_interrupts_3_IRQHandler       ; TCPWM #0, Counter #3 
        DCD     tcpwm_0_interrupts_4_IRQHandler       ; TCPWM #0, Counter #4 
        DCD     tcpwm_0_interrupts_5_IRQHandler       ; TCPWM #0, Counter #5 
        DCD     tcpwm_0_interrupts_6_IRQHandler       ; TCPWM #0, Counter #6 
        DCD     tcpwm_0_interrupts_7_IRQHandler       ; TCPWM #0, Counter #7 
        DCD     tcpwm_1_interrupts_0_IRQHandler       ; TCPWM #1, Counter #0 
        DCD     tcpwm_1_interrupts_1_IRQHandler       ; TCPWM #1, Counter #1 
        DCD     tcpwm_1_interrupts_2_IRQHandler       ; TCPWM #1, Counter #2 
        DCD     tcpwm_1_interrupts_3_IRQHandler       ; TCPWM #1, Counter #3 
        DCD     tcpwm_1_interrupts_4_IRQHandler       ; TCPWM #1, Counter #4 
        DCD     tcpwm_1_interrupts_5_IRQHandler       ; TCPWM #1, Counter #5 
        DCD     tcpwm_1_interrupts_6_IRQHandler       ; TCPWM #1, Counter #6 
        DCD     tcpwm_1_interrupts_7_IRQHandler       ; TCPWM #1, Counter #7 
        DCD     tcpwm_1_interrupts_8_IRQHandler       ; TCPWM #1, Counter #8 
        DCD     tcpwm_1_interrupts_9_IRQHandler       ; TCPWM #1, Counter #9 
        DCD     tcpwm_1_interrupts_10_IRQHandler      ; TCPWM #1, Counter #10 
        DCD     tcpwm_1_interrupts_11_IRQHandler      ; TCPWM #1, Counter #11 
        DCD     tcpwm_1_interrupts_12_IRQHandler      ; TCPWM #1, Counter #12 
        DCD     tcpwm_1_interrupts_13_IRQHandler      ; TCPWM #1, Counter #13 
        DCD     tcpwm_1_interrupts_14_IRQHandler      ; TCPWM #1, Counter #14 
        DCD     tcpwm_1_interrupts_15_IRQHandler      ; TCPWM #1, Counter #15 
        DCD     tcpwm_1_interrupts_16_IRQHandler      ; TCPWM #1, Counter #16 
        DCD     tcpwm_1_interrupts_17_IRQHandler      ; TCPWM #1, Counter #17 
        DCD     tcpwm_1_interrupts_18_IRQHandler      ; TCPWM #1, Counter #18 
        DCD     tcpwm_1_interrupts_19_IRQHandler      ; TCPWM #1, Counter #19 
        DCD     tcpwm_1_interrupts_20_IRQHandler      ; TCPWM #1, Counter #20 
        DCD     tcpwm_1_interrupts_21_IRQHandler      ; TCPWM #1, Counter #21 
        DCD     tcpwm_1_interrupts_22_IRQHandler      ; TCPWM #1, Counter #22 
        DCD     tcpwm_1_interrupts_23_IRQHandler      ; TCPWM #1, Counter #23 
        DCD     udb_interrupts_0_IRQHandler           ; UDB Interrupt #0 
        DCD     udb_interrupts_1_IRQHandler           ; UDB Interrupt #1 
        DCD     udb_interrupts_2_IRQHandler           ; UDB Interrupt #2 
        DCD     udb_interrupts_3_IRQHandler           ; UDB Interrupt #3 
        DCD     udb_interrupts_4_IRQHandler           ; UDB Interrupt #4 
        DCD     udb_interrupts_5_IRQHandler           ; UDB Interrupt #5 
        DCD     udb_interrupts_6_IRQHandler           ; UDB Interrupt #6 
        DCD     udb_interrupts_7_IRQHandler           ; UDB Interrupt #7 
        DCD     udb_interrupts_8_IRQHandler           ; UDB Interrupt #8 
        DCD     udb_interrupts_9_IRQHandler           ; UDB Interrupt #9 
        DCD     udb_interrupts_10_IRQHandler          ; UDB Interrupt #10 
        DCD     udb_interrupts_11_IRQHandler          ; UDB Interrupt #11 
        DCD     udb_interrupts_12_IRQHandler          ; UDB Interrupt #12 
        DCD     udb_interrupts_13_IRQHandler          ; UDB Interrupt #13 
        DCD     udb_interrupts_14_IRQHandler          ; UDB Interrupt #14 
        DCD     udb_interrupts_15_IRQHandler          ; UDB Interrupt #15 
        DCD     pass_interrupt_sar_IRQHandler         ; SAR ADC interrupt 
        DCD     audioss_interrupt_i2s_IRQHandler      ; I2S Audio interrupt 
        DCD     audioss_interrupt_pdm_IRQHandler      ; PDM/PCM Audio interrupt 
        DCD     profile_interrupt_IRQHandler          ; Energy Profiler interrupt 
        DCD     smif_interrupt_IRQHandler             ; Serial Memory Interface interrupt 
        DCD     usb_interrupt_hi_IRQHandler           ; USB Interrupt 
        DCD     usb_interrupt_med_IRQHandler          ; USB Interrupt 
        DCD     usb_interrupt_lo_IRQHandler           ; USB Interrupt 
        DCD     pass_interrupt_dacs_IRQHandler        ; Consolidated interrrupt for all DACs 

__Vectors_End

__Vectors       EQU   __vector_table
__Vectors_Size  EQU   __Vectors_End - __Vectors

        SECTION .intvec_ram:DATA:REORDER:NOROOT(2)
__ramVectors
        DS32     __Vectors_Size


        THUMB

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default handlers
;;
        PUBWEAK Default_Handler
        SECTION .text:CODE:REORDER:NOROOT(2)
Default_Handler
        B Default_Handler


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Saves and disables the interrupts
;;
        PUBLIC Cy_SaveIRQ
        SECTION .text:CODE:REORDER:NOROOT(2)
Cy_SaveIRQ
        MRS r0, PRIMASK
        CPSID I
        BX LR


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Restores the interrupts
;;
        PUBLIC Cy_RestoreIRQ
        SECTION .text:CODE:REORDER:NOROOT(2)
Cy_RestoreIRQ
        MSR PRIMASK, r0
        BX LR


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Weak function for startup customization
;;
        PUBWEAK Cy_OnResetUser
        SECTION .text:CODE:REORDER:NOROOT(2)
Cy_OnResetUser
        BX LR

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Define strong version to return zero for
;; __iar_program_start to skip data sections
;; initialization.
;;
        PUBLIC __low_level_init
        SECTION .text:CODE:REORDER:NOROOT(2)
__low_level_init
        MOVS R0, #0
        BX LR

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
        PUBWEAK Reset_Handler
        SECTION .text:CODE:REORDER:NOROOT(2)
Reset_Handler

        ; Define strong function for startup customization
        LDR     R0, =Cy_OnResetUser
        BLX     R0

        ; Disable global interrupts
        CPSID I

        ; Copy vectors from ROM to RAM
        LDR r1, =__vector_table
        LDR r0, =__ramVectors
        LDR r2, =__Vectors_Size
intvec_copy
        LDR r3, [r1]
        STR r3, [r0]
        ADDS r0, r0, #4
        ADDS r1, r1, #4
        SUBS r2, r2, #1
        CMP r2, #0
        BNE intvec_copy

        ; Update Vector Table Offset Register
        LDR r0, =__ramVectors
        LDR r1, =0xE000ED08
        STR r0, [r1]
        dsb

        ; Enable the FPU if used
        LDR     R0, =Cy_SystemInitFpuEnable
        BLX     R0

        ; Initialize data sections
        LDR     R0, =__iar_data_init3
        BLX     R0

        LDR     R0, =SystemInit
        BLX     R0

        LDR     R0, =__iar_program_start
        BLX     R0

; Should never get here
Cy_Main_Exited
        B Cy_Main_Exited


        PUBWEAK NMI_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
NMI_Handler
        B NMI_Handler


        PUBWEAK Cy_SysLib_FaultHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
Cy_SysLib_FaultHandler
        B Cy_SysLib_FaultHandler

        PUBWEAK HardFault_Wrapper
        SECTION .text:CODE:REORDER:NOROOT(1)
HardFault_Wrapper
        IMPORT Cy_SysLib_FaultHandler
        movs r0, #4
        mov r1, LR
        tst r0, r1
        beq L_MSP
        mrs r0, PSP
        b L_API_call
L_MSP
        mrs r0, MSP
L_API_call
        ; Storing LR content for Creator call stack trace
        push {LR}
        bl Cy_SysLib_FaultHandler

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
HardFault_Handler
        B HardFault_Wrapper

        PUBWEAK MemManage_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
MemManage_Handler
        B HardFault_Wrapper

        PUBWEAK BusFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
BusFault_Handler
        B HardFault_Wrapper

        PUBWEAK UsageFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
UsageFault_Handler
        B HardFault_Wrapper

        PUBWEAK SVC_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SVC_Handler
        B SVC_Handler

        PUBWEAK DebugMon_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DebugMon_Handler
        B DebugMon_Handler

        PUBWEAK PendSV_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
PendSV_Handler
        B PendSV_Handler

        PUBWEAK SysTick_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SysTick_Handler
        B SysTick_Handler


        ; External interrupts
        PUBWEAK ioss_interrupts_gpio_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_0_IRQHandler
        B       ioss_interrupts_gpio_0_IRQHandler

        PUBWEAK ioss_interrupts_gpio_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_1_IRQHandler
        B       ioss_interrupts_gpio_1_IRQHandler

        PUBWEAK ioss_interrupts_gpio_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_2_IRQHandler
        B       ioss_interrupts_gpio_2_IRQHandler

        PUBWEAK ioss_interrupts_gpio_3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_3_IRQHandler
        B       ioss_interrupts_gpio_3_IRQHandler

        PUBWEAK ioss_interrupts_gpio_4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_4_IRQHandler
        B       ioss_interrupts_gpio_4_IRQHandler

        PUBWEAK ioss_interrupts_gpio_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_5_IRQHandler
        B       ioss_interrupts_gpio_5_IRQHandler

        PUBWEAK ioss_interrupts_gpio_6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_6_IRQHandler
        B       ioss_interrupts_gpio_6_IRQHandler

        PUBWEAK ioss_interrupts_gpio_7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_7_IRQHandler
        B       ioss_interrupts_gpio_7_IRQHandler

        PUBWEAK ioss_interrupts_gpio_8_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_8_IRQHandler
        B       ioss_interrupts_gpio_8_IRQHandler

        PUBWEAK ioss_interrupts_gpio_9_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_9_IRQHandler
        B       ioss_interrupts_gpio_9_IRQHandler

        PUBWEAK ioss_interrupts_gpio_10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_10_IRQHandler
        B       ioss_interrupts_gpio_10_IRQHandler

        PUBWEAK ioss_interrupts_gpio_11_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_11_IRQHandler
        B       ioss_interrupts_gpio_11_IRQHandler

        PUBWEAK ioss_interrupts_gpio_12_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_12_IRQHandler
        B       ioss_interrupts_gpio_12_IRQHandler

        PUBWEAK ioss_interrupts_gpio_13_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_13_IRQHandler
        B       ioss_interrupts_gpio_13_IRQHandler

        PUBWEAK ioss_interrupts_gpio_14_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupts_gpio_14_IRQHandler
        B       ioss_interrupts_gpio_14_IRQHandler

        PUBWEAK ioss_interrupt_gpio_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupt_gpio_IRQHandler
        B       ioss_interrupt_gpio_IRQHandler

        PUBWEAK ioss_interrupt_vdd_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ioss_interrupt_vdd_IRQHandler
        B       ioss_interrupt_vdd_IRQHandler

        PUBWEAK lpcomp_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
lpcomp_interrupt_IRQHandler
        B       lpcomp_interrupt_IRQHandler

        PUBWEAK scb_8_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_8_interrupt_IRQHandler
        B       scb_8_interrupt_IRQHandler

        PUBWEAK srss_interrupt_mcwdt_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
srss_interrupt_mcwdt_0_IRQHandler
        B       srss_interrupt_mcwdt_0_IRQHandler

        PUBWEAK srss_interrupt_mcwdt_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
srss_interrupt_mcwdt_1_IRQHandler
        B       srss_interrupt_mcwdt_1_IRQHandler

        PUBWEAK srss_interrupt_backup_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
srss_interrupt_backup_IRQHandler
        B       srss_interrupt_backup_IRQHandler

        PUBWEAK srss_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
srss_interrupt_IRQHandler
        B       srss_interrupt_IRQHandler

        PUBWEAK pass_interrupt_ctbs_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
pass_interrupt_ctbs_IRQHandler
        B       pass_interrupt_ctbs_IRQHandler

        PUBWEAK bless_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
bless_interrupt_IRQHandler
        B       bless_interrupt_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_0_IRQHandler
        B       cpuss_interrupts_ipc_0_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_1_IRQHandler
        B       cpuss_interrupts_ipc_1_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_2_IRQHandler
        B       cpuss_interrupts_ipc_2_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_3_IRQHandler
        B       cpuss_interrupts_ipc_3_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_4_IRQHandler
        B       cpuss_interrupts_ipc_4_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_5_IRQHandler
        B       cpuss_interrupts_ipc_5_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_6_IRQHandler
        B       cpuss_interrupts_ipc_6_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_7_IRQHandler
        B       cpuss_interrupts_ipc_7_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_8_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_8_IRQHandler
        B       cpuss_interrupts_ipc_8_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_9_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_9_IRQHandler
        B       cpuss_interrupts_ipc_9_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_10_IRQHandler
        B       cpuss_interrupts_ipc_10_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_11_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_11_IRQHandler
        B       cpuss_interrupts_ipc_11_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_12_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_12_IRQHandler
        B       cpuss_interrupts_ipc_12_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_13_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_13_IRQHandler
        B       cpuss_interrupts_ipc_13_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_14_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_14_IRQHandler
        B       cpuss_interrupts_ipc_14_IRQHandler

        PUBWEAK cpuss_interrupts_ipc_15_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_ipc_15_IRQHandler
        B       cpuss_interrupts_ipc_15_IRQHandler

        PUBWEAK scb_0_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_0_interrupt_IRQHandler
        B       scb_0_interrupt_IRQHandler

        PUBWEAK scb_1_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_1_interrupt_IRQHandler
        B       scb_1_interrupt_IRQHandler

        PUBWEAK scb_2_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_2_interrupt_IRQHandler
        B       scb_2_interrupt_IRQHandler

        PUBWEAK scb_3_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_3_interrupt_IRQHandler
        B       scb_3_interrupt_IRQHandler

        PUBWEAK scb_4_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_4_interrupt_IRQHandler
        B       scb_4_interrupt_IRQHandler

        PUBWEAK scb_5_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_5_interrupt_IRQHandler
        B       scb_5_interrupt_IRQHandler

        PUBWEAK scb_6_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_6_interrupt_IRQHandler
        B       scb_6_interrupt_IRQHandler

        PUBWEAK scb_7_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
scb_7_interrupt_IRQHandler
        B       scb_7_interrupt_IRQHandler

        PUBWEAK csd_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
csd_interrupt_IRQHandler
        B       csd_interrupt_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_0_IRQHandler
        B       cpuss_interrupts_dw0_0_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_1_IRQHandler
        B       cpuss_interrupts_dw0_1_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_2_IRQHandler
        B       cpuss_interrupts_dw0_2_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_3_IRQHandler
        B       cpuss_interrupts_dw0_3_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_4_IRQHandler
        B       cpuss_interrupts_dw0_4_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_5_IRQHandler
        B       cpuss_interrupts_dw0_5_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_6_IRQHandler
        B       cpuss_interrupts_dw0_6_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_7_IRQHandler
        B       cpuss_interrupts_dw0_7_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_8_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_8_IRQHandler
        B       cpuss_interrupts_dw0_8_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_9_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_9_IRQHandler
        B       cpuss_interrupts_dw0_9_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_10_IRQHandler
        B       cpuss_interrupts_dw0_10_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_11_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_11_IRQHandler
        B       cpuss_interrupts_dw0_11_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_12_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_12_IRQHandler
        B       cpuss_interrupts_dw0_12_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_13_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_13_IRQHandler
        B       cpuss_interrupts_dw0_13_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_14_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_14_IRQHandler
        B       cpuss_interrupts_dw0_14_IRQHandler

        PUBWEAK cpuss_interrupts_dw0_15_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw0_15_IRQHandler
        B       cpuss_interrupts_dw0_15_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_0_IRQHandler
        B       cpuss_interrupts_dw1_0_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_1_IRQHandler
        B       cpuss_interrupts_dw1_1_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_2_IRQHandler
        B       cpuss_interrupts_dw1_2_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_3_IRQHandler
        B       cpuss_interrupts_dw1_3_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_4_IRQHandler
        B       cpuss_interrupts_dw1_4_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_5_IRQHandler
        B       cpuss_interrupts_dw1_5_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_6_IRQHandler
        B       cpuss_interrupts_dw1_6_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_7_IRQHandler
        B       cpuss_interrupts_dw1_7_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_8_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_8_IRQHandler
        B       cpuss_interrupts_dw1_8_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_9_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_9_IRQHandler
        B       cpuss_interrupts_dw1_9_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_10_IRQHandler
        B       cpuss_interrupts_dw1_10_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_11_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_11_IRQHandler
        B       cpuss_interrupts_dw1_11_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_12_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_12_IRQHandler
        B       cpuss_interrupts_dw1_12_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_13_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_13_IRQHandler
        B       cpuss_interrupts_dw1_13_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_14_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_14_IRQHandler
        B       cpuss_interrupts_dw1_14_IRQHandler

        PUBWEAK cpuss_interrupts_dw1_15_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_dw1_15_IRQHandler
        B       cpuss_interrupts_dw1_15_IRQHandler

        PUBWEAK cpuss_interrupts_fault_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_fault_0_IRQHandler
        B       cpuss_interrupts_fault_0_IRQHandler

        PUBWEAK cpuss_interrupts_fault_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_fault_1_IRQHandler
        B       cpuss_interrupts_fault_1_IRQHandler

        PUBWEAK cpuss_interrupt_crypto_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupt_crypto_IRQHandler
        B       cpuss_interrupt_crypto_IRQHandler

        PUBWEAK cpuss_interrupt_fm_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupt_fm_IRQHandler
        B       cpuss_interrupt_fm_IRQHandler

        PUBWEAK cpuss_interrupts_cm0_cti_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_cm0_cti_0_IRQHandler
        B       cpuss_interrupts_cm0_cti_0_IRQHandler

        PUBWEAK cpuss_interrupts_cm0_cti_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_cm0_cti_1_IRQHandler
        B       cpuss_interrupts_cm0_cti_1_IRQHandler

        PUBWEAK cpuss_interrupts_cm4_cti_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_cm4_cti_0_IRQHandler
        B       cpuss_interrupts_cm4_cti_0_IRQHandler

        PUBWEAK cpuss_interrupts_cm4_cti_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
cpuss_interrupts_cm4_cti_1_IRQHandler
        B       cpuss_interrupts_cm4_cti_1_IRQHandler

        PUBWEAK tcpwm_0_interrupts_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_0_IRQHandler
        B       tcpwm_0_interrupts_0_IRQHandler

        PUBWEAK tcpwm_0_interrupts_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_1_IRQHandler
        B       tcpwm_0_interrupts_1_IRQHandler

        PUBWEAK tcpwm_0_interrupts_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_2_IRQHandler
        B       tcpwm_0_interrupts_2_IRQHandler

        PUBWEAK tcpwm_0_interrupts_3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_3_IRQHandler
        B       tcpwm_0_interrupts_3_IRQHandler

        PUBWEAK tcpwm_0_interrupts_4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_4_IRQHandler
        B       tcpwm_0_interrupts_4_IRQHandler

        PUBWEAK tcpwm_0_interrupts_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_5_IRQHandler
        B       tcpwm_0_interrupts_5_IRQHandler

        PUBWEAK tcpwm_0_interrupts_6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_6_IRQHandler
        B       tcpwm_0_interrupts_6_IRQHandler

        PUBWEAK tcpwm_0_interrupts_7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_0_interrupts_7_IRQHandler
        B       tcpwm_0_interrupts_7_IRQHandler

        PUBWEAK tcpwm_1_interrupts_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_0_IRQHandler
        B       tcpwm_1_interrupts_0_IRQHandler

        PUBWEAK tcpwm_1_interrupts_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_1_IRQHandler
        B       tcpwm_1_interrupts_1_IRQHandler

        PUBWEAK tcpwm_1_interrupts_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_2_IRQHandler
        B       tcpwm_1_interrupts_2_IRQHandler

        PUBWEAK tcpwm_1_interrupts_3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_3_IRQHandler
        B       tcpwm_1_interrupts_3_IRQHandler

        PUBWEAK tcpwm_1_interrupts_4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_4_IRQHandler
        B       tcpwm_1_interrupts_4_IRQHandler

        PUBWEAK tcpwm_1_interrupts_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_5_IRQHandler
        B       tcpwm_1_interrupts_5_IRQHandler

        PUBWEAK tcpwm_1_interrupts_6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_6_IRQHandler
        B       tcpwm_1_interrupts_6_IRQHandler

        PUBWEAK tcpwm_1_interrupts_7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_7_IRQHandler
        B       tcpwm_1_interrupts_7_IRQHandler

        PUBWEAK tcpwm_1_interrupts_8_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_8_IRQHandler
        B       tcpwm_1_interrupts_8_IRQHandler

        PUBWEAK tcpwm_1_interrupts_9_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_9_IRQHandler
        B       tcpwm_1_interrupts_9_IRQHandler

        PUBWEAK tcpwm_1_interrupts_10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_10_IRQHandler
        B       tcpwm_1_interrupts_10_IRQHandler

        PUBWEAK tcpwm_1_interrupts_11_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_11_IRQHandler
        B       tcpwm_1_interrupts_11_IRQHandler

        PUBWEAK tcpwm_1_interrupts_12_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_12_IRQHandler
        B       tcpwm_1_interrupts_12_IRQHandler

        PUBWEAK tcpwm_1_interrupts_13_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_13_IRQHandler
        B       tcpwm_1_interrupts_13_IRQHandler

        PUBWEAK tcpwm_1_interrupts_14_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_14_IRQHandler
        B       tcpwm_1_interrupts_14_IRQHandler

        PUBWEAK tcpwm_1_interrupts_15_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_15_IRQHandler
        B       tcpwm_1_interrupts_15_IRQHandler

        PUBWEAK tcpwm_1_interrupts_16_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_16_IRQHandler
        B       tcpwm_1_interrupts_16_IRQHandler

        PUBWEAK tcpwm_1_interrupts_17_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_17_IRQHandler
        B       tcpwm_1_interrupts_17_IRQHandler

        PUBWEAK tcpwm_1_interrupts_18_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_18_IRQHandler
        B       tcpwm_1_interrupts_18_IRQHandler

        PUBWEAK tcpwm_1_interrupts_19_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_19_IRQHandler
        B       tcpwm_1_interrupts_19_IRQHandler

        PUBWEAK tcpwm_1_interrupts_20_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_20_IRQHandler
        B       tcpwm_1_interrupts_20_IRQHandler

        PUBWEAK tcpwm_1_interrupts_21_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_21_IRQHandler
        B       tcpwm_1_interrupts_21_IRQHandler

        PUBWEAK tcpwm_1_interrupts_22_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_22_IRQHandler
        B       tcpwm_1_interrupts_22_IRQHandler

        PUBWEAK tcpwm_1_interrupts_23_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
tcpwm_1_interrupts_23_IRQHandler
        B       tcpwm_1_interrupts_23_IRQHandler

        PUBWEAK udb_interrupts_0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_0_IRQHandler
        B       udb_interrupts_0_IRQHandler

        PUBWEAK udb_interrupts_1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_1_IRQHandler
        B       udb_interrupts_1_IRQHandler

        PUBWEAK udb_interrupts_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_2_IRQHandler
        B       udb_interrupts_2_IRQHandler

        PUBWEAK udb_interrupts_3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_3_IRQHandler
        B       udb_interrupts_3_IRQHandler

        PUBWEAK udb_interrupts_4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_4_IRQHandler
        B       udb_interrupts_4_IRQHandler

        PUBWEAK udb_interrupts_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_5_IRQHandler
        B       udb_interrupts_5_IRQHandler

        PUBWEAK udb_interrupts_6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_6_IRQHandler
        B       udb_interrupts_6_IRQHandler

        PUBWEAK udb_interrupts_7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_7_IRQHandler
        B       udb_interrupts_7_IRQHandler

        PUBWEAK udb_interrupts_8_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_8_IRQHandler
        B       udb_interrupts_8_IRQHandler

        PUBWEAK udb_interrupts_9_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_9_IRQHandler
        B       udb_interrupts_9_IRQHandler

        PUBWEAK udb_interrupts_10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_10_IRQHandler
        B       udb_interrupts_10_IRQHandler

        PUBWEAK udb_interrupts_11_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_11_IRQHandler
        B       udb_interrupts_11_IRQHandler

        PUBWEAK udb_interrupts_12_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_12_IRQHandler
        B       udb_interrupts_12_IRQHandler

        PUBWEAK udb_interrupts_13_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_13_IRQHandler
        B       udb_interrupts_13_IRQHandler

        PUBWEAK udb_interrupts_14_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_14_IRQHandler
        B       udb_interrupts_14_IRQHandler

        PUBWEAK udb_interrupts_15_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
udb_interrupts_15_IRQHandler
        B       udb_interrupts_15_IRQHandler

        PUBWEAK pass_interrupt_sar_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
pass_interrupt_sar_IRQHandler
        B       pass_interrupt_sar_IRQHandler

        PUBWEAK audioss_interrupt_i2s_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
audioss_interrupt_i2s_IRQHandler
        B       audioss_interrupt_i2s_IRQHandler

        PUBWEAK audioss_interrupt_pdm_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
audioss_interrupt_pdm_IRQHandler
        B       audioss_interrupt_pdm_IRQHandler

        PUBWEAK profile_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
profile_interrupt_IRQHandler
        B       profile_interrupt_IRQHandler

        PUBWEAK smif_interrupt_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
smif_interrupt_IRQHandler
        B       smif_interrupt_IRQHandler

        PUBWEAK usb_interrupt_hi_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
usb_interrupt_hi_IRQHandler
        B       usb_interrupt_hi_IRQHandler

        PUBWEAK usb_interrupt_med_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
usb_interrupt_med_IRQHandler
        B       usb_interrupt_med_IRQHandler

        PUBWEAK usb_interrupt_lo_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
usb_interrupt_lo_IRQHandler
        B       usb_interrupt_lo_IRQHandler

        PUBWEAK pass_interrupt_dacs_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
pass_interrupt_dacs_IRQHandler
        B       pass_interrupt_dacs_IRQHandler


        END


; [] END OF FILE
