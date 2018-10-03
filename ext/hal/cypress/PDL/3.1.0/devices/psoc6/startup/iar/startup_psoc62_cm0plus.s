;/**************************************************************************//**
; * @file     startup_psoc62_cm0plus.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM0plus Device Series
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

        DCD     0x0000000D      ; NMI_Handler is defined in ROM code
        DCD     HardFault_Handler
        DCD     0
        DCD     0
        DCD     0
__vector_table_0x1c
        DCD     0
        DCD     0
        DCD     0
        DCD     0
        DCD     SVC_Handler
        DCD     0
        DCD     0
        DCD     PendSV_Handler
        DCD     SysTick_Handler

        ; External interrupts                         Power Mode  Description
        DCD     NvicMux0_IRQHandler                   ; CM0 + NVIC Mux input 0 
        DCD     NvicMux1_IRQHandler                   ; CM0 + NVIC Mux input 1 
        DCD     NvicMux2_IRQHandler                   ; CM0 + NVIC Mux input 2 
        DCD     NvicMux3_IRQHandler                   ; CM0 + NVIC Mux input 3 
        DCD     NvicMux4_IRQHandler                   ; CM0 + NVIC Mux input 4 
        DCD     NvicMux5_IRQHandler                   ; CM0 + NVIC Mux input 5 
        DCD     NvicMux6_IRQHandler                   ; CM0 + NVIC Mux input 6 
        DCD     NvicMux7_IRQHandler                   ; CM0 + NVIC Mux input 7 
        DCD     NvicMux8_IRQHandler                   ; CM0 + NVIC Mux input 8 
        DCD     NvicMux9_IRQHandler                   ; CM0 + NVIC Mux input 9 
        DCD     NvicMux10_IRQHandler                  ; CM0 + NVIC Mux input 10 
        DCD     NvicMux11_IRQHandler                  ; CM0 + NVIC Mux input 11 
        DCD     NvicMux12_IRQHandler                  ; CM0 + NVIC Mux input 12 
        DCD     NvicMux13_IRQHandler                  ; CM0 + NVIC Mux input 13 
        DCD     NvicMux14_IRQHandler                  ; CM0 + NVIC Mux input 14 
        DCD     NvicMux15_IRQHandler                  ; CM0 + NVIC Mux input 15 
        DCD     NvicMux16_IRQHandler                  ; CM0 + NVIC Mux input 16 
        DCD     NvicMux17_IRQHandler                  ; CM0 + NVIC Mux input 17 
        DCD     NvicMux18_IRQHandler                  ; CM0 + NVIC Mux input 18 
        DCD     NvicMux19_IRQHandler                  ; CM0 + NVIC Mux input 19 
        DCD     NvicMux20_IRQHandler                  ; CM0 + NVIC Mux input 20 
        DCD     NvicMux21_IRQHandler                  ; CM0 + NVIC Mux input 21 
        DCD     NvicMux22_IRQHandler                  ; CM0 + NVIC Mux input 22 
        DCD     NvicMux23_IRQHandler                  ; CM0 + NVIC Mux input 23 
        DCD     NvicMux24_IRQHandler                  ; CM0 + NVIC Mux input 24 
        DCD     NvicMux25_IRQHandler                  ; CM0 + NVIC Mux input 25 
        DCD     NvicMux26_IRQHandler                  ; CM0 + NVIC Mux input 26 
        DCD     NvicMux27_IRQHandler                  ; CM0 + NVIC Mux input 27 
        DCD     NvicMux28_IRQHandler                  ; CM0 + NVIC Mux input 28 
        DCD     NvicMux29_IRQHandler                  ; CM0 + NVIC Mux input 29 
        DCD     NvicMux30_IRQHandler                  ; CM0 + NVIC Mux input 30 
        DCD     NvicMux31_IRQHandler                  ; CM0 + NVIC Mux input 31 

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
        THUMB
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

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
HardFault_Handler
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


        PUBWEAK SVC_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SVC_Handler
        B SVC_Handler

        PUBWEAK PendSV_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
PendSV_Handler
        B PendSV_Handler

        PUBWEAK SysTick_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SysTick_Handler
        B SysTick_Handler


        ; External interrupts
        PUBWEAK NvicMux0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux0_IRQHandler
        B       NvicMux0_IRQHandler

        PUBWEAK NvicMux1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux1_IRQHandler
        B       NvicMux1_IRQHandler

        PUBWEAK NvicMux2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux2_IRQHandler
        B       NvicMux2_IRQHandler

        PUBWEAK NvicMux3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux3_IRQHandler
        B       NvicMux3_IRQHandler

        PUBWEAK NvicMux4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux4_IRQHandler
        B       NvicMux4_IRQHandler

        PUBWEAK NvicMux5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux5_IRQHandler
        B       NvicMux5_IRQHandler

        PUBWEAK NvicMux6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux6_IRQHandler
        B       NvicMux6_IRQHandler

        PUBWEAK NvicMux7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux7_IRQHandler
        B       NvicMux7_IRQHandler

        PUBWEAK NvicMux8_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux8_IRQHandler
        B       NvicMux8_IRQHandler

        PUBWEAK NvicMux9_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux9_IRQHandler
        B       NvicMux9_IRQHandler

        PUBWEAK NvicMux10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux10_IRQHandler
        B       NvicMux10_IRQHandler

        PUBWEAK NvicMux11_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux11_IRQHandler
        B       NvicMux11_IRQHandler

        PUBWEAK NvicMux12_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux12_IRQHandler
        B       NvicMux12_IRQHandler

        PUBWEAK NvicMux13_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux13_IRQHandler
        B       NvicMux13_IRQHandler

        PUBWEAK NvicMux14_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux14_IRQHandler
        B       NvicMux14_IRQHandler

        PUBWEAK NvicMux15_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux15_IRQHandler
        B       NvicMux15_IRQHandler

        PUBWEAK NvicMux16_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux16_IRQHandler
        B       NvicMux16_IRQHandler

        PUBWEAK NvicMux17_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux17_IRQHandler
        B       NvicMux17_IRQHandler

        PUBWEAK NvicMux18_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux18_IRQHandler
        B       NvicMux18_IRQHandler

        PUBWEAK NvicMux19_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux19_IRQHandler
        B       NvicMux19_IRQHandler

        PUBWEAK NvicMux20_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux20_IRQHandler
        B       NvicMux20_IRQHandler

        PUBWEAK NvicMux21_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux21_IRQHandler
        B       NvicMux21_IRQHandler

        PUBWEAK NvicMux22_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux22_IRQHandler
        B       NvicMux22_IRQHandler

        PUBWEAK NvicMux23_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux23_IRQHandler
        B       NvicMux23_IRQHandler

        PUBWEAK NvicMux24_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux24_IRQHandler
        B       NvicMux24_IRQHandler

        PUBWEAK NvicMux25_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux25_IRQHandler
        B       NvicMux25_IRQHandler

        PUBWEAK NvicMux26_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux26_IRQHandler
        B       NvicMux26_IRQHandler

        PUBWEAK NvicMux27_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux27_IRQHandler
        B       NvicMux27_IRQHandler

        PUBWEAK NvicMux28_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux28_IRQHandler
        B       NvicMux28_IRQHandler

        PUBWEAK NvicMux29_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux29_IRQHandler
        B       NvicMux29_IRQHandler

        PUBWEAK NvicMux30_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux30_IRQHandler
        B       NvicMux30_IRQHandler

        PUBWEAK NvicMux31_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
NvicMux31_IRQHandler
        B       NvicMux31_IRQHandler


        END


; [] END OF FILE
