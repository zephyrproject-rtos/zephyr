;/**************************************************************************//**
; * @file     startup_psoc62_cm4.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM4 Device Series
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

                DCD     0x0000000D                            ; NMI Handler located at ROM code
                DCD     HardFault_Handler                     ; Hard Fault Handler
                DCD     MemManage_Handler                     ; MPU Fault Handler
                DCD     BusFault_Handler                      ; Bus Fault Handler
                DCD     UsageFault_Handler                    ; Usage Fault Handler
                DCD     0                                     ; Reserved
                DCD     0                                     ; Reserved
                DCD     0                                     ; Reserved
                DCD     0                                     ; Reserved
                DCD     SVC_Handler                           ; SVCall Handler
                DCD     DebugMon_Handler                      ; Debug Monitor Handler
                DCD     0                                     ; Reserved
                DCD     PendSV_Handler                        ; PendSV Handler
                DCD     SysTick_Handler                       ; SysTick Handler

                ; External interrupts                         Power Mode  Description
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
Cy_OnResetUser  PROC
                EXPORT  Cy_OnResetUser        [WEAK]
                BX      LR
                ENDP


; Reset Handler
Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  Cy_SystemInitFpuEnable
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

                ; Enable the FPU if used
                LDR     R0, =Cy_SystemInitFpuEnable
                BLX     R0

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
                    EXPORT  Cy_SysLib_FaultHandler   [WEAK]
                    B       .
                    ENDP
HardFault_Wrapper\
                    PROC
                    EXPORT HardFault_Wrapper         [WEAK]
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
HardFault_Handler\
                    PROC
                    EXPORT  HardFault_Handler         [WEAK]
                    B       HardFault_Wrapper
                    ENDP
MemManage_Handler\
                    PROC
                    EXPORT  MemManage_Handler         [WEAK]
                    B       HardFault_Wrapper
                    ENDP
BusFault_Handler\
                    PROC
                    EXPORT  BusFault_Handler          [WEAK]
                    B       HardFault_Wrapper
                    ENDP
UsageFault_Handler\
                    PROC
                    EXPORT  UsageFault_Handler        [WEAK]
                    B       HardFault_Wrapper
                    ENDP
SVC_Handler         PROC
                    EXPORT  SVC_Handler               [WEAK]
                    B       .
                    ENDP
DebugMon_Handler\
                    PROC
                    EXPORT  DebugMon_Handler          [WEAK]
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

Default_Handler     PROC
                    EXPORT      Default_Handler                        [WEAK]
                    EXPORT  ioss_interrupts_gpio_0_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_1_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_2_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_3_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_4_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_5_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_6_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_7_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_8_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_9_IRQHandler     [WEAK]
                    EXPORT  ioss_interrupts_gpio_10_IRQHandler    [WEAK]
                    EXPORT  ioss_interrupts_gpio_11_IRQHandler    [WEAK]
                    EXPORT  ioss_interrupts_gpio_12_IRQHandler    [WEAK]
                    EXPORT  ioss_interrupts_gpio_13_IRQHandler    [WEAK]
                    EXPORT  ioss_interrupts_gpio_14_IRQHandler    [WEAK]
                    EXPORT  ioss_interrupt_gpio_IRQHandler        [WEAK]
                    EXPORT  ioss_interrupt_vdd_IRQHandler         [WEAK]
                    EXPORT  lpcomp_interrupt_IRQHandler           [WEAK]
                    EXPORT  scb_8_interrupt_IRQHandler            [WEAK]
                    EXPORT  srss_interrupt_mcwdt_0_IRQHandler     [WEAK]
                    EXPORT  srss_interrupt_mcwdt_1_IRQHandler     [WEAK]
                    EXPORT  srss_interrupt_backup_IRQHandler      [WEAK]
                    EXPORT  srss_interrupt_IRQHandler             [WEAK]
                    EXPORT  pass_interrupt_ctbs_IRQHandler        [WEAK]
                    EXPORT  bless_interrupt_IRQHandler            [WEAK]
                    EXPORT  cpuss_interrupts_ipc_0_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_1_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_2_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_3_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_4_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_5_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_6_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_7_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_8_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_9_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_ipc_10_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_ipc_11_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_ipc_12_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_ipc_13_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_ipc_14_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_ipc_15_IRQHandler    [WEAK]
                    EXPORT  scb_0_interrupt_IRQHandler            [WEAK]
                    EXPORT  scb_1_interrupt_IRQHandler            [WEAK]
                    EXPORT  scb_2_interrupt_IRQHandler            [WEAK]
                    EXPORT  scb_3_interrupt_IRQHandler            [WEAK]
                    EXPORT  scb_4_interrupt_IRQHandler            [WEAK]
                    EXPORT  scb_5_interrupt_IRQHandler            [WEAK]
                    EXPORT  scb_6_interrupt_IRQHandler            [WEAK]
                    EXPORT  scb_7_interrupt_IRQHandler            [WEAK]
                    EXPORT  csd_interrupt_IRQHandler              [WEAK]
                    EXPORT  cpuss_interrupts_dw0_0_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_1_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_2_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_3_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_4_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_5_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_6_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_7_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_8_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_9_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw0_10_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw0_11_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw0_12_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw0_13_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw0_14_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw0_15_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw1_0_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_1_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_2_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_3_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_4_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_5_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_6_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_7_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_8_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_9_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupts_dw1_10_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw1_11_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw1_12_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw1_13_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw1_14_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_dw1_15_IRQHandler    [WEAK]
                    EXPORT  cpuss_interrupts_fault_0_IRQHandler   [WEAK]
                    EXPORT  cpuss_interrupts_fault_1_IRQHandler   [WEAK]
                    EXPORT  cpuss_interrupt_crypto_IRQHandler     [WEAK]
                    EXPORT  cpuss_interrupt_fm_IRQHandler         [WEAK]
                    EXPORT  cpuss_interrupts_cm0_cti_0_IRQHandler [WEAK]
                    EXPORT  cpuss_interrupts_cm0_cti_1_IRQHandler [WEAK]
                    EXPORT  cpuss_interrupts_cm4_cti_0_IRQHandler [WEAK]
                    EXPORT  cpuss_interrupts_cm4_cti_1_IRQHandler [WEAK]
                    EXPORT  tcpwm_0_interrupts_0_IRQHandler       [WEAK]
                    EXPORT  tcpwm_0_interrupts_1_IRQHandler       [WEAK]
                    EXPORT  tcpwm_0_interrupts_2_IRQHandler       [WEAK]
                    EXPORT  tcpwm_0_interrupts_3_IRQHandler       [WEAK]
                    EXPORT  tcpwm_0_interrupts_4_IRQHandler       [WEAK]
                    EXPORT  tcpwm_0_interrupts_5_IRQHandler       [WEAK]
                    EXPORT  tcpwm_0_interrupts_6_IRQHandler       [WEAK]
                    EXPORT  tcpwm_0_interrupts_7_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_0_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_1_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_2_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_3_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_4_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_5_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_6_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_7_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_8_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_9_IRQHandler       [WEAK]
                    EXPORT  tcpwm_1_interrupts_10_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_11_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_12_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_13_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_14_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_15_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_16_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_17_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_18_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_19_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_20_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_21_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_22_IRQHandler      [WEAK]
                    EXPORT  tcpwm_1_interrupts_23_IRQHandler      [WEAK]
                    EXPORT  udb_interrupts_0_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_1_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_2_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_3_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_4_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_5_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_6_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_7_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_8_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_9_IRQHandler           [WEAK]
                    EXPORT  udb_interrupts_10_IRQHandler          [WEAK]
                    EXPORT  udb_interrupts_11_IRQHandler          [WEAK]
                    EXPORT  udb_interrupts_12_IRQHandler          [WEAK]
                    EXPORT  udb_interrupts_13_IRQHandler          [WEAK]
                    EXPORT  udb_interrupts_14_IRQHandler          [WEAK]
                    EXPORT  udb_interrupts_15_IRQHandler          [WEAK]
                    EXPORT  pass_interrupt_sar_IRQHandler         [WEAK]
                    EXPORT  audioss_interrupt_i2s_IRQHandler      [WEAK]
                    EXPORT  audioss_interrupt_pdm_IRQHandler      [WEAK]
                    EXPORT  profile_interrupt_IRQHandler          [WEAK]
                    EXPORT  smif_interrupt_IRQHandler             [WEAK]
                    EXPORT  usb_interrupt_hi_IRQHandler           [WEAK]
                    EXPORT  usb_interrupt_med_IRQHandler          [WEAK]
                    EXPORT  usb_interrupt_lo_IRQHandler           [WEAK]
                    EXPORT  pass_interrupt_dacs_IRQHandler        [WEAK]

ioss_interrupts_gpio_0_IRQHandler
ioss_interrupts_gpio_1_IRQHandler
ioss_interrupts_gpio_2_IRQHandler
ioss_interrupts_gpio_3_IRQHandler
ioss_interrupts_gpio_4_IRQHandler
ioss_interrupts_gpio_5_IRQHandler
ioss_interrupts_gpio_6_IRQHandler
ioss_interrupts_gpio_7_IRQHandler
ioss_interrupts_gpio_8_IRQHandler
ioss_interrupts_gpio_9_IRQHandler
ioss_interrupts_gpio_10_IRQHandler
ioss_interrupts_gpio_11_IRQHandler
ioss_interrupts_gpio_12_IRQHandler
ioss_interrupts_gpio_13_IRQHandler
ioss_interrupts_gpio_14_IRQHandler
ioss_interrupt_gpio_IRQHandler
ioss_interrupt_vdd_IRQHandler
lpcomp_interrupt_IRQHandler
scb_8_interrupt_IRQHandler
srss_interrupt_mcwdt_0_IRQHandler
srss_interrupt_mcwdt_1_IRQHandler
srss_interrupt_backup_IRQHandler
srss_interrupt_IRQHandler
pass_interrupt_ctbs_IRQHandler
bless_interrupt_IRQHandler
cpuss_interrupts_ipc_0_IRQHandler
cpuss_interrupts_ipc_1_IRQHandler
cpuss_interrupts_ipc_2_IRQHandler
cpuss_interrupts_ipc_3_IRQHandler
cpuss_interrupts_ipc_4_IRQHandler
cpuss_interrupts_ipc_5_IRQHandler
cpuss_interrupts_ipc_6_IRQHandler
cpuss_interrupts_ipc_7_IRQHandler
cpuss_interrupts_ipc_8_IRQHandler
cpuss_interrupts_ipc_9_IRQHandler
cpuss_interrupts_ipc_10_IRQHandler
cpuss_interrupts_ipc_11_IRQHandler
cpuss_interrupts_ipc_12_IRQHandler
cpuss_interrupts_ipc_13_IRQHandler
cpuss_interrupts_ipc_14_IRQHandler
cpuss_interrupts_ipc_15_IRQHandler
scb_0_interrupt_IRQHandler
scb_1_interrupt_IRQHandler
scb_2_interrupt_IRQHandler
scb_3_interrupt_IRQHandler
scb_4_interrupt_IRQHandler
scb_5_interrupt_IRQHandler
scb_6_interrupt_IRQHandler
scb_7_interrupt_IRQHandler
csd_interrupt_IRQHandler
cpuss_interrupts_dw0_0_IRQHandler
cpuss_interrupts_dw0_1_IRQHandler
cpuss_interrupts_dw0_2_IRQHandler
cpuss_interrupts_dw0_3_IRQHandler
cpuss_interrupts_dw0_4_IRQHandler
cpuss_interrupts_dw0_5_IRQHandler
cpuss_interrupts_dw0_6_IRQHandler
cpuss_interrupts_dw0_7_IRQHandler
cpuss_interrupts_dw0_8_IRQHandler
cpuss_interrupts_dw0_9_IRQHandler
cpuss_interrupts_dw0_10_IRQHandler
cpuss_interrupts_dw0_11_IRQHandler
cpuss_interrupts_dw0_12_IRQHandler
cpuss_interrupts_dw0_13_IRQHandler
cpuss_interrupts_dw0_14_IRQHandler
cpuss_interrupts_dw0_15_IRQHandler
cpuss_interrupts_dw1_0_IRQHandler
cpuss_interrupts_dw1_1_IRQHandler
cpuss_interrupts_dw1_2_IRQHandler
cpuss_interrupts_dw1_3_IRQHandler
cpuss_interrupts_dw1_4_IRQHandler
cpuss_interrupts_dw1_5_IRQHandler
cpuss_interrupts_dw1_6_IRQHandler
cpuss_interrupts_dw1_7_IRQHandler
cpuss_interrupts_dw1_8_IRQHandler
cpuss_interrupts_dw1_9_IRQHandler
cpuss_interrupts_dw1_10_IRQHandler
cpuss_interrupts_dw1_11_IRQHandler
cpuss_interrupts_dw1_12_IRQHandler
cpuss_interrupts_dw1_13_IRQHandler
cpuss_interrupts_dw1_14_IRQHandler
cpuss_interrupts_dw1_15_IRQHandler
cpuss_interrupts_fault_0_IRQHandler
cpuss_interrupts_fault_1_IRQHandler
cpuss_interrupt_crypto_IRQHandler
cpuss_interrupt_fm_IRQHandler
cpuss_interrupts_cm0_cti_0_IRQHandler
cpuss_interrupts_cm0_cti_1_IRQHandler
cpuss_interrupts_cm4_cti_0_IRQHandler
cpuss_interrupts_cm4_cti_1_IRQHandler
tcpwm_0_interrupts_0_IRQHandler
tcpwm_0_interrupts_1_IRQHandler
tcpwm_0_interrupts_2_IRQHandler
tcpwm_0_interrupts_3_IRQHandler
tcpwm_0_interrupts_4_IRQHandler
tcpwm_0_interrupts_5_IRQHandler
tcpwm_0_interrupts_6_IRQHandler
tcpwm_0_interrupts_7_IRQHandler
tcpwm_1_interrupts_0_IRQHandler
tcpwm_1_interrupts_1_IRQHandler
tcpwm_1_interrupts_2_IRQHandler
tcpwm_1_interrupts_3_IRQHandler
tcpwm_1_interrupts_4_IRQHandler
tcpwm_1_interrupts_5_IRQHandler
tcpwm_1_interrupts_6_IRQHandler
tcpwm_1_interrupts_7_IRQHandler
tcpwm_1_interrupts_8_IRQHandler
tcpwm_1_interrupts_9_IRQHandler
tcpwm_1_interrupts_10_IRQHandler
tcpwm_1_interrupts_11_IRQHandler
tcpwm_1_interrupts_12_IRQHandler
tcpwm_1_interrupts_13_IRQHandler
tcpwm_1_interrupts_14_IRQHandler
tcpwm_1_interrupts_15_IRQHandler
tcpwm_1_interrupts_16_IRQHandler
tcpwm_1_interrupts_17_IRQHandler
tcpwm_1_interrupts_18_IRQHandler
tcpwm_1_interrupts_19_IRQHandler
tcpwm_1_interrupts_20_IRQHandler
tcpwm_1_interrupts_21_IRQHandler
tcpwm_1_interrupts_22_IRQHandler
tcpwm_1_interrupts_23_IRQHandler
udb_interrupts_0_IRQHandler
udb_interrupts_1_IRQHandler
udb_interrupts_2_IRQHandler
udb_interrupts_3_IRQHandler
udb_interrupts_4_IRQHandler
udb_interrupts_5_IRQHandler
udb_interrupts_6_IRQHandler
udb_interrupts_7_IRQHandler
udb_interrupts_8_IRQHandler
udb_interrupts_9_IRQHandler
udb_interrupts_10_IRQHandler
udb_interrupts_11_IRQHandler
udb_interrupts_12_IRQHandler
udb_interrupts_13_IRQHandler
udb_interrupts_14_IRQHandler
udb_interrupts_15_IRQHandler
pass_interrupt_sar_IRQHandler
audioss_interrupt_i2s_IRQHandler
audioss_interrupt_pdm_IRQHandler
profile_interrupt_IRQHandler
smif_interrupt_IRQHandler
usb_interrupt_hi_IRQHandler
usb_interrupt_med_IRQHandler
usb_interrupt_lo_IRQHandler
pass_interrupt_dacs_IRQHandler

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
                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR
                ENDP

                ALIGN

                ENDIF

                END


; [] END OF FILE
