;/**************************************************************************//**
; * @file     startup_psoc63_cm0plus.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM0plus Device Series
; * @version  V5.00
; * @date     02. March 2016
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

;/*
;//-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
;*/

; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>
                IF :DEF:__STACK_SIZE
Stack_Size      EQU     __STACK_SIZE
                ELSE
Stack_Size      EQU     0x00001000
                ENDIF
                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>
                IF :DEF:__HEAP_SIZE
Heap_Size       EQU     __HEAP_SIZE
                ELSE
Heap_Size       EQU     0x00000400
                ENDIF
                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit


                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler

                DCD     0x0000000D                ; NMI Handler located at ROM code
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler


                ; External interrupts                           Description
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

__Vectors_Size  EQU     __Vectors_End - __Vectors
                EXPORT __ramVectors
                AREA    RESET_RAM, READWRITE, NOINIT
__ramVectors    SPACE   __Vectors_Size


                AREA    |.text|, CODE, READONLY


; Saves and disables the interrupts
Cy_SaveIRQ       PROC
                EXPORT Cy_SaveIRQ
                MRS r0, PRIMASK
                CPSID I
                BX LR
                ENDP


; Restores the interrupts
Cy_RestoreIRQ    PROC
                EXPORT Cy_RestoreIRQ
                MSR PRIMASK, r0
                BX LR
                ENDP


; Weak function for startup customization
Cy_OnResetUser PROC
                EXPORT  Cy_OnResetUser             [WEAK]
                BX      LR
                ENDP

; Reset Handler
Reset_Handler   PROC
                EXPORT  Reset_Handler               [WEAK]
                IMPORT  __main

                ; Define strong function for startup customization
                BL      Cy_OnResetUser

                ; Disable global interrupts
                CPSID I

                ; Copy vectors from ROM to RAM
                LDR r1, =__Vectors
                LDR r0, =__ramVectors
                LDR r2, =__Vectors_Size
Vectors_Copy
                LDR r3, [r1]
                STR r3, [r0]
                ADDS r0, r0, #4
                ADDS r1, r1, #4
                SUBS r2, r2, #1
                CMP r2, #0
                BNE Vectors_Copy

                ; Update Vector Table Offset Register. */
                LDR r0, =__ramVectors
                LDR r1, =0xE000ED08
                STR r0, [r1]
                dsb 0xF

                LDR     R0, =__main
                BLX     R0

                ; Should never get here
                B       .

                ENDP

; Dummy Exception Handlers (infinite loops which can be modified)
NMI_Handler         PROC
                    EXPORT  NMI_Handler               [WEAK]
                    B       .
                    ENDP

Cy_SysLib_FaultHandler PROC
                    EXPORT  Cy_SysLib_FaultHandler    [WEAK]
                    B       .
                    ENDP

HardFault_Handler   PROC
                    EXPORT HardFault_Handler          [WEAK]
                    movs r0, #4
                    mov r1, LR
                    tst r0, r1
                    beq L_MSP
                    mrs r0, PSP
                    bl L_API_call
L_MSP
                    mrs r0, MSP
L_API_call
                    bl Cy_SysLib_FaultHandler
                    ENDP

SVC_Handler         PROC
                    EXPORT  SVC_Handler               [WEAK]
                    B       .
                    ENDP
PendSV_Handler      PROC
                    EXPORT  PendSV_Handler            [WEAK]
                    B       .
                    ENDP
SysTick_Handler     PROC
                    EXPORT  SysTick_Handler           [WEAK]
                    B       .
                    ENDP

Default_Handler PROC
                    EXPORT      Default_Handler                        [WEAK]
                    EXPORT  NvicMux0_IRQHandler                   [WEAK]
                    EXPORT  NvicMux1_IRQHandler                   [WEAK]
                    EXPORT  NvicMux2_IRQHandler                   [WEAK]
                    EXPORT  NvicMux3_IRQHandler                   [WEAK]
                    EXPORT  NvicMux4_IRQHandler                   [WEAK]
                    EXPORT  NvicMux5_IRQHandler                   [WEAK]
                    EXPORT  NvicMux6_IRQHandler                   [WEAK]
                    EXPORT  NvicMux7_IRQHandler                   [WEAK]
                    EXPORT  NvicMux8_IRQHandler                   [WEAK]
                    EXPORT  NvicMux9_IRQHandler                   [WEAK]
                    EXPORT  NvicMux10_IRQHandler                  [WEAK]
                    EXPORT  NvicMux11_IRQHandler                  [WEAK]
                    EXPORT  NvicMux12_IRQHandler                  [WEAK]
                    EXPORT  NvicMux13_IRQHandler                  [WEAK]
                    EXPORT  NvicMux14_IRQHandler                  [WEAK]
                    EXPORT  NvicMux15_IRQHandler                  [WEAK]
                    EXPORT  NvicMux16_IRQHandler                  [WEAK]
                    EXPORT  NvicMux17_IRQHandler                  [WEAK]
                    EXPORT  NvicMux18_IRQHandler                  [WEAK]
                    EXPORT  NvicMux19_IRQHandler                  [WEAK]
                    EXPORT  NvicMux20_IRQHandler                  [WEAK]
                    EXPORT  NvicMux21_IRQHandler                  [WEAK]
                    EXPORT  NvicMux22_IRQHandler                  [WEAK]
                    EXPORT  NvicMux23_IRQHandler                  [WEAK]
                    EXPORT  NvicMux24_IRQHandler                  [WEAK]
                    EXPORT  NvicMux25_IRQHandler                  [WEAK]
                    EXPORT  NvicMux26_IRQHandler                  [WEAK]
                    EXPORT  NvicMux27_IRQHandler                  [WEAK]
                    EXPORT  NvicMux28_IRQHandler                  [WEAK]
                    EXPORT  NvicMux29_IRQHandler                  [WEAK]
                    EXPORT  NvicMux30_IRQHandler                  [WEAK]
                    EXPORT  NvicMux31_IRQHandler                  [WEAK]

NvicMux0_IRQHandler
NvicMux1_IRQHandler
NvicMux2_IRQHandler
NvicMux3_IRQHandler
NvicMux4_IRQHandler
NvicMux5_IRQHandler
NvicMux6_IRQHandler
NvicMux7_IRQHandler
NvicMux8_IRQHandler
NvicMux9_IRQHandler
NvicMux10_IRQHandler
NvicMux11_IRQHandler
NvicMux12_IRQHandler
NvicMux13_IRQHandler
NvicMux14_IRQHandler
NvicMux15_IRQHandler
NvicMux16_IRQHandler
NvicMux17_IRQHandler
NvicMux18_IRQHandler
NvicMux19_IRQHandler
NvicMux20_IRQHandler
NvicMux21_IRQHandler
NvicMux22_IRQHandler
NvicMux23_IRQHandler
NvicMux24_IRQHandler
NvicMux25_IRQHandler
NvicMux26_IRQHandler
NvicMux27_IRQHandler
NvicMux28_IRQHandler
NvicMux29_IRQHandler
NvicMux30_IRQHandler
NvicMux31_IRQHandler

                B       .
                ENDP

                ALIGN


; User Initial Stack & Heap

                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap

__user_initial_stackheap PROC
                LDR     R0, =Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, =(Heap_Mem +  Heap_Size)
                LDR     R3, =Stack_Mem
                BX      LR
                ENDP

                ALIGN

                ENDIF

                END


; [] END OF FILE
