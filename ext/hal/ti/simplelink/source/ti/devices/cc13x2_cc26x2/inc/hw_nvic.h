/******************************************************************************
*  Filename:       hw_nvic.h
*  Revised:        2015-01-13 16:59:55 +0100 (Tue, 13 Jan 2015)
*  Revision:       42365
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef __HW_NVIC_H__
#define __HW_NVIC_H__

//*****************************************************************************
//
// The following are defines for the NVIC register addresses.
//
//*****************************************************************************
#define NVIC_INT_TYPE           0xE000E004  // Interrupt Controller Type Reg
#define NVIC_ACTLR              0xE000E008  // Auxiliary Control
#define NVIC_ST_CTRL            0xE000E010  // SysTick Control and Status
                                            // Register
#define NVIC_ST_RELOAD          0xE000E014  // SysTick Reload Value Register
#define NVIC_ST_CURRENT         0xE000E018  // SysTick Current Value Register
#define NVIC_ST_CAL             0xE000E01C  // SysTick Calibration Value Reg
#define NVIC_EN0                0xE000E100  // Interrupt 0-31 Set Enable
#define NVIC_EN1                0xE000E104  // Interrupt 32-54 Set Enable
#define NVIC_DIS0               0xE000E180  // Interrupt 0-31 Clear Enable
#define NVIC_DIS1               0xE000E184  // Interrupt 32-54 Clear Enable
#define NVIC_PEND0              0xE000E200  // Interrupt 0-31 Set Pending
#define NVIC_PEND1              0xE000E204  // Interrupt 32-54 Set Pending
#define NVIC_UNPEND0            0xE000E280  // Interrupt 0-31 Clear Pending
#define NVIC_UNPEND1            0xE000E284  // Interrupt 32-54 Clear Pending
#define NVIC_ACTIVE0            0xE000E300  // Interrupt 0-31 Active Bit
#define NVIC_ACTIVE1            0xE000E304  // Interrupt 32-54 Active Bit
#define NVIC_PRI0               0xE000E400  // Interrupt 0-3 Priority
#define NVIC_PRI1               0xE000E404  // Interrupt 4-7 Priority
#define NVIC_PRI2               0xE000E408  // Interrupt 8-11 Priority
#define NVIC_PRI3               0xE000E40C  // Interrupt 12-15 Priority
#define NVIC_PRI4               0xE000E410  // Interrupt 16-19 Priority
#define NVIC_PRI5               0xE000E414  // Interrupt 20-23 Priority
#define NVIC_PRI6               0xE000E418  // Interrupt 24-27 Priority
#define NVIC_PRI7               0xE000E41C  // Interrupt 28-31 Priority
#define NVIC_PRI8               0xE000E420  // Interrupt 32-35 Priority
#define NVIC_PRI9               0xE000E424  // Interrupt 36-39 Priority
#define NVIC_PRI10              0xE000E428  // Interrupt 40-43 Priority
#define NVIC_PRI11              0xE000E42C  // Interrupt 44-47 Priority
#define NVIC_PRI12              0xE000E430  // Interrupt 48-51 Priority
#define NVIC_PRI13              0xE000E434  // Interrupt 52-55 Priority
#define NVIC_CPUID              0xE000ED00  // CPU ID Base
#define NVIC_INT_CTRL           0xE000ED04  // Interrupt Control and State
#define NVIC_VTABLE             0xE000ED08  // Vector Table Offset
#define NVIC_APINT              0xE000ED0C  // Application Interrupt and Reset
                                            // Control
#define NVIC_SYS_CTRL           0xE000ED10  // System Control
#define NVIC_CFG_CTRL           0xE000ED14  // Configuration and Control
#define NVIC_SYS_PRI1           0xE000ED18  // System Handler Priority 1
#define NVIC_SYS_PRI2           0xE000ED1C  // System Handler Priority 2
#define NVIC_SYS_PRI3           0xE000ED20  // System Handler Priority 3
#define NVIC_SYS_HND_CTRL       0xE000ED24  // System Handler Control and State
#define NVIC_FAULT_STAT         0xE000ED28  // Configurable Fault Status
#define NVIC_HFAULT_STAT        0xE000ED2C  // Hard Fault Status
#define NVIC_DEBUG_STAT         0xE000ED30  // Debug Status Register
#define NVIC_MM_ADDR            0xE000ED34  // Memory Management Fault Address
#define NVIC_FAULT_ADDR         0xE000ED38  // Bus Fault Address
#define NVIC_MPU_TYPE           0xE000ED90  // MPU Type
#define NVIC_MPU_CTRL           0xE000ED94  // MPU Control
#define NVIC_MPU_NUMBER         0xE000ED98  // MPU Region Number
#define NVIC_MPU_BASE           0xE000ED9C  // MPU Region Base Address
#define NVIC_MPU_ATTR           0xE000EDA0  // MPU Region Attribute and Size
#define NVIC_MPU_BASE1          0xE000EDA4  // MPU Region Base Address Alias 1
#define NVIC_MPU_ATTR1          0xE000EDA8  // MPU Region Attribute and Size
                                            // Alias 1
#define NVIC_MPU_BASE2          0xE000EDAC  // MPU Region Base Address Alias 2
#define NVIC_MPU_ATTR2          0xE000EDB0  // MPU Region Attribute and Size
                                            // Alias 2
#define NVIC_MPU_BASE3          0xE000EDB4  // MPU Region Base Address Alias 3
#define NVIC_MPU_ATTR3          0xE000EDB8  // MPU Region Attribute and Size
                                            // Alias 3
#define NVIC_DBG_CTRL           0xE000EDF0  // Debug Control and Status Reg
#define NVIC_DBG_XFER           0xE000EDF4  // Debug Core Reg. Transfer Select
#define NVIC_DBG_DATA           0xE000EDF8  // Debug Core Register Data
#define NVIC_DBG_INT            0xE000EDFC  // Debug Reset Interrupt Control
#define NVIC_SW_TRIG            0xE000EF00  // Software Trigger Interrupt

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_INT_TYPE register.
//
//*****************************************************************************
#define NVIC_INT_TYPE_LINES_M   0x0000001F  // Number of interrupt lines (x32)
#define NVIC_INT_TYPE_LINES_S   0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_ACTLR register.
//
//*****************************************************************************
#define NVIC_ACTLR_DISFOLD      0x00000004  // Disable IT Folding
#define NVIC_ACTLR_DISWBUF      0x00000002  // Disable Write Buffer
#define NVIC_ACTLR_DISMCYC      0x00000001  // Disable Interrupts of Multiple
                                            // Cycle Instructions

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_ST_CTRL register.
//
//*****************************************************************************
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count Flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt Enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Enable

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_ST_RELOAD register.
//
//*****************************************************************************
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Reload Value
#define NVIC_ST_RELOAD_S        0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_ST_CURRENT
// register.
//
//*****************************************************************************
#define NVIC_ST_CURRENT_M       0x00FFFFFF  // Current Value
#define NVIC_ST_CURRENT_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_ST_CAL register.
//
//*****************************************************************************
#define NVIC_ST_CAL_NOREF       0x80000000  // No reference clock
#define NVIC_ST_CAL_SKEW        0x40000000  // Clock skew
#define NVIC_ST_CAL_ONEMS_M     0x00FFFFFF  // 1ms reference value
#define NVIC_ST_CAL_ONEMS_S     0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_EN0 register.
//
//*****************************************************************************
#define NVIC_EN0_INT_M          0xFFFFFFFF  // Interrupt Enable
#define NVIC_EN0_INT0           0x00000001  // Interrupt 0 enable
#define NVIC_EN0_INT1           0x00000002  // Interrupt 1 enable
#define NVIC_EN0_INT2           0x00000004  // Interrupt 2 enable
#define NVIC_EN0_INT3           0x00000008  // Interrupt 3 enable
#define NVIC_EN0_INT4           0x00000010  // Interrupt 4 enable
#define NVIC_EN0_INT5           0x00000020  // Interrupt 5 enable
#define NVIC_EN0_INT6           0x00000040  // Interrupt 6 enable
#define NVIC_EN0_INT7           0x00000080  // Interrupt 7 enable
#define NVIC_EN0_INT8           0x00000100  // Interrupt 8 enable
#define NVIC_EN0_INT9           0x00000200  // Interrupt 9 enable
#define NVIC_EN0_INT10          0x00000400  // Interrupt 10 enable
#define NVIC_EN0_INT11          0x00000800  // Interrupt 11 enable
#define NVIC_EN0_INT12          0x00001000  // Interrupt 12 enable
#define NVIC_EN0_INT13          0x00002000  // Interrupt 13 enable
#define NVIC_EN0_INT14          0x00004000  // Interrupt 14 enable
#define NVIC_EN0_INT15          0x00008000  // Interrupt 15 enable
#define NVIC_EN0_INT16          0x00010000  // Interrupt 16 enable
#define NVIC_EN0_INT17          0x00020000  // Interrupt 17 enable
#define NVIC_EN0_INT18          0x00040000  // Interrupt 18 enable
#define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
#define NVIC_EN0_INT20          0x00100000  // Interrupt 20 enable
#define NVIC_EN0_INT21          0x00200000  // Interrupt 21 enable
#define NVIC_EN0_INT22          0x00400000  // Interrupt 22 enable
#define NVIC_EN0_INT23          0x00800000  // Interrupt 23 enable
#define NVIC_EN0_INT24          0x01000000  // Interrupt 24 enable
#define NVIC_EN0_INT25          0x02000000  // Interrupt 25 enable
#define NVIC_EN0_INT26          0x04000000  // Interrupt 26 enable
#define NVIC_EN0_INT27          0x08000000  // Interrupt 27 enable
#define NVIC_EN0_INT28          0x10000000  // Interrupt 28 enable
#define NVIC_EN0_INT29          0x20000000  // Interrupt 29 enable
#define NVIC_EN0_INT30          0x40000000  // Interrupt 30 enable
#define NVIC_EN0_INT31          0x80000000  // Interrupt 31 enable

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_EN1 register.
//
//*****************************************************************************
#define NVIC_EN1_INT_M          0x007FFFFF  // Interrupt Enable
#define NVIC_EN1_INT32          0x00000001  // Interrupt 32 enable
#define NVIC_EN1_INT33          0x00000002  // Interrupt 33 enable
#define NVIC_EN1_INT34          0x00000004  // Interrupt 34 enable
#define NVIC_EN1_INT35          0x00000008  // Interrupt 35 enable
#define NVIC_EN1_INT36          0x00000010  // Interrupt 36 enable
#define NVIC_EN1_INT37          0x00000020  // Interrupt 37 enable
#define NVIC_EN1_INT38          0x00000040  // Interrupt 38 enable
#define NVIC_EN1_INT39          0x00000080  // Interrupt 39 enable
#define NVIC_EN1_INT40          0x00000100  // Interrupt 40 enable
#define NVIC_EN1_INT41          0x00000200  // Interrupt 41 enable
#define NVIC_EN1_INT42          0x00000400  // Interrupt 42 enable
#define NVIC_EN1_INT43          0x00000800  // Interrupt 43 enable
#define NVIC_EN1_INT44          0x00001000  // Interrupt 44 enable
#define NVIC_EN1_INT45          0x00002000  // Interrupt 45 enable
#define NVIC_EN1_INT46          0x00004000  // Interrupt 46 enable
#define NVIC_EN1_INT47          0x00008000  // Interrupt 47 enable
#define NVIC_EN1_INT48          0x00010000  // Interrupt 48 enable
#define NVIC_EN1_INT49          0x00020000  // Interrupt 49 enable
#define NVIC_EN1_INT50          0x00040000  // Interrupt 50 enable
#define NVIC_EN1_INT51          0x00080000  // Interrupt 51 enable
#define NVIC_EN1_INT52          0x00100000  // Interrupt 52 enable
#define NVIC_EN1_INT53          0x00200000  // Interrupt 53 enable
#define NVIC_EN1_INT54          0x00400000  // Interrupt 54 enable

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_DIS0 register.
//
//*****************************************************************************
#define NVIC_DIS0_INT_M         0xFFFFFFFF  // Interrupt Disable
#define NVIC_DIS0_INT0          0x00000001  // Interrupt 0 disable
#define NVIC_DIS0_INT1          0x00000002  // Interrupt 1 disable
#define NVIC_DIS0_INT2          0x00000004  // Interrupt 2 disable
#define NVIC_DIS0_INT3          0x00000008  // Interrupt 3 disable
#define NVIC_DIS0_INT4          0x00000010  // Interrupt 4 disable
#define NVIC_DIS0_INT5          0x00000020  // Interrupt 5 disable
#define NVIC_DIS0_INT6          0x00000040  // Interrupt 6 disable
#define NVIC_DIS0_INT7          0x00000080  // Interrupt 7 disable
#define NVIC_DIS0_INT8          0x00000100  // Interrupt 8 disable
#define NVIC_DIS0_INT9          0x00000200  // Interrupt 9 disable
#define NVIC_DIS0_INT10         0x00000400  // Interrupt 10 disable
#define NVIC_DIS0_INT11         0x00000800  // Interrupt 11 disable
#define NVIC_DIS0_INT12         0x00001000  // Interrupt 12 disable
#define NVIC_DIS0_INT13         0x00002000  // Interrupt 13 disable
#define NVIC_DIS0_INT14         0x00004000  // Interrupt 14 disable
#define NVIC_DIS0_INT15         0x00008000  // Interrupt 15 disable
#define NVIC_DIS0_INT16         0x00010000  // Interrupt 16 disable
#define NVIC_DIS0_INT17         0x00020000  // Interrupt 17 disable
#define NVIC_DIS0_INT18         0x00040000  // Interrupt 18 disable
#define NVIC_DIS0_INT19         0x00080000  // Interrupt 19 disable
#define NVIC_DIS0_INT20         0x00100000  // Interrupt 20 disable
#define NVIC_DIS0_INT21         0x00200000  // Interrupt 21 disable
#define NVIC_DIS0_INT22         0x00400000  // Interrupt 22 disable
#define NVIC_DIS0_INT23         0x00800000  // Interrupt 23 disable
#define NVIC_DIS0_INT24         0x01000000  // Interrupt 24 disable
#define NVIC_DIS0_INT25         0x02000000  // Interrupt 25 disable
#define NVIC_DIS0_INT26         0x04000000  // Interrupt 26 disable
#define NVIC_DIS0_INT27         0x08000000  // Interrupt 27 disable
#define NVIC_DIS0_INT28         0x10000000  // Interrupt 28 disable
#define NVIC_DIS0_INT29         0x20000000  // Interrupt 29 disable
#define NVIC_DIS0_INT30         0x40000000  // Interrupt 30 disable
#define NVIC_DIS0_INT31         0x80000000  // Interrupt 31 disable

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_DIS1 register.
//
//*****************************************************************************
#define NVIC_DIS1_INT_M         0x007FFFFF  // Interrupt Disable
#define NVIC_DIS1_INT32         0x00000001  // Interrupt 32 disable
#define NVIC_DIS1_INT33         0x00000002  // Interrupt 33 disable
#define NVIC_DIS1_INT34         0x00000004  // Interrupt 34 disable
#define NVIC_DIS1_INT35         0x00000008  // Interrupt 35 disable
#define NVIC_DIS1_INT36         0x00000010  // Interrupt 36 disable
#define NVIC_DIS1_INT37         0x00000020  // Interrupt 37 disable
#define NVIC_DIS1_INT38         0x00000040  // Interrupt 38 disable
#define NVIC_DIS1_INT39         0x00000080  // Interrupt 39 disable
#define NVIC_DIS1_INT40         0x00000100  // Interrupt 40 disable
#define NVIC_DIS1_INT41         0x00000200  // Interrupt 41 disable
#define NVIC_DIS1_INT42         0x00000400  // Interrupt 42 disable
#define NVIC_DIS1_INT43         0x00000800  // Interrupt 43 disable
#define NVIC_DIS1_INT44         0x00001000  // Interrupt 44 disable
#define NVIC_DIS1_INT45         0x00002000  // Interrupt 45 disable
#define NVIC_DIS1_INT46         0x00004000  // Interrupt 46 disable
#define NVIC_DIS1_INT47         0x00008000  // Interrupt 47 disable
#define NVIC_DIS1_INT48         0x00010000  // Interrupt 48 disable
#define NVIC_DIS1_INT49         0x00020000  // Interrupt 49 disable
#define NVIC_DIS1_INT50         0x00040000  // Interrupt 50 disable
#define NVIC_DIS1_INT51         0x00080000  // Interrupt 51 disable
#define NVIC_DIS1_INT52         0x00100000  // Interrupt 52 disable
#define NVIC_DIS1_INT53         0x00200000  // Interrupt 53 disable
#define NVIC_DIS1_INT54         0x00400000  // Interrupt 54 disable

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PEND0 register.
//
//*****************************************************************************
#define NVIC_PEND0_INT_M        0xFFFFFFFF  // Interrupt Set Pending
#define NVIC_PEND0_INT0         0x00000001  // Interrupt 0 pend
#define NVIC_PEND0_INT1         0x00000002  // Interrupt 1 pend
#define NVIC_PEND0_INT2         0x00000004  // Interrupt 2 pend
#define NVIC_PEND0_INT3         0x00000008  // Interrupt 3 pend
#define NVIC_PEND0_INT4         0x00000010  // Interrupt 4 pend
#define NVIC_PEND0_INT5         0x00000020  // Interrupt 5 pend
#define NVIC_PEND0_INT6         0x00000040  // Interrupt 6 pend
#define NVIC_PEND0_INT7         0x00000080  // Interrupt 7 pend
#define NVIC_PEND0_INT8         0x00000100  // Interrupt 8 pend
#define NVIC_PEND0_INT9         0x00000200  // Interrupt 9 pend
#define NVIC_PEND0_INT10        0x00000400  // Interrupt 10 pend
#define NVIC_PEND0_INT11        0x00000800  // Interrupt 11 pend
#define NVIC_PEND0_INT12        0x00001000  // Interrupt 12 pend
#define NVIC_PEND0_INT13        0x00002000  // Interrupt 13 pend
#define NVIC_PEND0_INT14        0x00004000  // Interrupt 14 pend
#define NVIC_PEND0_INT15        0x00008000  // Interrupt 15 pend
#define NVIC_PEND0_INT16        0x00010000  // Interrupt 16 pend
#define NVIC_PEND0_INT17        0x00020000  // Interrupt 17 pend
#define NVIC_PEND0_INT18        0x00040000  // Interrupt 18 pend
#define NVIC_PEND0_INT19        0x00080000  // Interrupt 19 pend
#define NVIC_PEND0_INT20        0x00100000  // Interrupt 20 pend
#define NVIC_PEND0_INT21        0x00200000  // Interrupt 21 pend
#define NVIC_PEND0_INT22        0x00400000  // Interrupt 22 pend
#define NVIC_PEND0_INT23        0x00800000  // Interrupt 23 pend
#define NVIC_PEND0_INT24        0x01000000  // Interrupt 24 pend
#define NVIC_PEND0_INT25        0x02000000  // Interrupt 25 pend
#define NVIC_PEND0_INT26        0x04000000  // Interrupt 26 pend
#define NVIC_PEND0_INT27        0x08000000  // Interrupt 27 pend
#define NVIC_PEND0_INT28        0x10000000  // Interrupt 28 pend
#define NVIC_PEND0_INT29        0x20000000  // Interrupt 29 pend
#define NVIC_PEND0_INT30        0x40000000  // Interrupt 30 pend
#define NVIC_PEND0_INT31        0x80000000  // Interrupt 31 pend

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PEND1 register.
//
//*****************************************************************************
#define NVIC_PEND1_INT_M        0x007FFFFF  // Interrupt Set Pending
#define NVIC_PEND1_INT32        0x00000001  // Interrupt 32 pend
#define NVIC_PEND1_INT33        0x00000002  // Interrupt 33 pend
#define NVIC_PEND1_INT34        0x00000004  // Interrupt 34 pend
#define NVIC_PEND1_INT35        0x00000008  // Interrupt 35 pend
#define NVIC_PEND1_INT36        0x00000010  // Interrupt 36 pend
#define NVIC_PEND1_INT37        0x00000020  // Interrupt 37 pend
#define NVIC_PEND1_INT38        0x00000040  // Interrupt 38 pend
#define NVIC_PEND1_INT39        0x00000080  // Interrupt 39 pend
#define NVIC_PEND1_INT40        0x00000100  // Interrupt 40 pend
#define NVIC_PEND1_INT41        0x00000200  // Interrupt 41 pend
#define NVIC_PEND1_INT42        0x00000400  // Interrupt 42 pend
#define NVIC_PEND1_INT43        0x00000800  // Interrupt 43 pend
#define NVIC_PEND1_INT44        0x00001000  // Interrupt 44 pend
#define NVIC_PEND1_INT45        0x00002000  // Interrupt 45 pend
#define NVIC_PEND1_INT46        0x00004000  // Interrupt 46 pend
#define NVIC_PEND1_INT47        0x00008000  // Interrupt 47 pend
#define NVIC_PEND1_INT48        0x00010000  // Interrupt 48 pend
#define NVIC_PEND1_INT49        0x00020000  // Interrupt 49 pend
#define NVIC_PEND1_INT50        0x00040000  // Interrupt 50 pend
#define NVIC_PEND1_INT51        0x00080000  // Interrupt 51 pend
#define NVIC_PEND1_INT52        0x00100000  // Interrupt 52 pend
#define NVIC_PEND1_INT53        0x00200000  // Interrupt 53 pend
#define NVIC_PEND1_INT54        0x00400000  // Interrupt 54 pend

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_UNPEND0 register.
//
//*****************************************************************************
#define NVIC_UNPEND0_INT_M      0xFFFFFFFF  // Interrupt Clear Pending
#define NVIC_UNPEND0_INT0       0x00000001  // Interrupt 0 unpend
#define NVIC_UNPEND0_INT1       0x00000002  // Interrupt 1 unpend
#define NVIC_UNPEND0_INT2       0x00000004  // Interrupt 2 unpend
#define NVIC_UNPEND0_INT3       0x00000008  // Interrupt 3 unpend
#define NVIC_UNPEND0_INT4       0x00000010  // Interrupt 4 unpend
#define NVIC_UNPEND0_INT5       0x00000020  // Interrupt 5 unpend
#define NVIC_UNPEND0_INT6       0x00000040  // Interrupt 6 unpend
#define NVIC_UNPEND0_INT7       0x00000080  // Interrupt 7 unpend
#define NVIC_UNPEND0_INT8       0x00000100  // Interrupt 8 unpend
#define NVIC_UNPEND0_INT9       0x00000200  // Interrupt 9 unpend
#define NVIC_UNPEND0_INT10      0x00000400  // Interrupt 10 unpend
#define NVIC_UNPEND0_INT11      0x00000800  // Interrupt 11 unpend
#define NVIC_UNPEND0_INT12      0x00001000  // Interrupt 12 unpend
#define NVIC_UNPEND0_INT13      0x00002000  // Interrupt 13 unpend
#define NVIC_UNPEND0_INT14      0x00004000  // Interrupt 14 unpend
#define NVIC_UNPEND0_INT15      0x00008000  // Interrupt 15 unpend
#define NVIC_UNPEND0_INT16      0x00010000  // Interrupt 16 unpend
#define NVIC_UNPEND0_INT17      0x00020000  // Interrupt 17 unpend
#define NVIC_UNPEND0_INT18      0x00040000  // Interrupt 18 unpend
#define NVIC_UNPEND0_INT19      0x00080000  // Interrupt 19 unpend
#define NVIC_UNPEND0_INT20      0x00100000  // Interrupt 20 unpend
#define NVIC_UNPEND0_INT21      0x00200000  // Interrupt 21 unpend
#define NVIC_UNPEND0_INT22      0x00400000  // Interrupt 22 unpend
#define NVIC_UNPEND0_INT23      0x00800000  // Interrupt 23 unpend
#define NVIC_UNPEND0_INT24      0x01000000  // Interrupt 24 unpend
#define NVIC_UNPEND0_INT25      0x02000000  // Interrupt 25 unpend
#define NVIC_UNPEND0_INT26      0x04000000  // Interrupt 26 unpend
#define NVIC_UNPEND0_INT27      0x08000000  // Interrupt 27 unpend
#define NVIC_UNPEND0_INT28      0x10000000  // Interrupt 28 unpend
#define NVIC_UNPEND0_INT29      0x20000000  // Interrupt 29 unpend
#define NVIC_UNPEND0_INT30      0x40000000  // Interrupt 30 unpend
#define NVIC_UNPEND0_INT31      0x80000000  // Interrupt 31 unpend

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_UNPEND1 register.
//
//*****************************************************************************
#define NVIC_UNPEND1_INT_M      0x007FFFFF  // Interrupt Clear Pending
#define NVIC_UNPEND1_INT32      0x00000001  // Interrupt 32 unpend
#define NVIC_UNPEND1_INT33      0x00000002  // Interrupt 33 unpend
#define NVIC_UNPEND1_INT34      0x00000004  // Interrupt 34 unpend
#define NVIC_UNPEND1_INT35      0x00000008  // Interrupt 35 unpend
#define NVIC_UNPEND1_INT36      0x00000010  // Interrupt 36 unpend
#define NVIC_UNPEND1_INT37      0x00000020  // Interrupt 37 unpend
#define NVIC_UNPEND1_INT38      0x00000040  // Interrupt 38 unpend
#define NVIC_UNPEND1_INT39      0x00000080  // Interrupt 39 unpend
#define NVIC_UNPEND1_INT40      0x00000100  // Interrupt 40 unpend
#define NVIC_UNPEND1_INT41      0x00000200  // Interrupt 41 unpend
#define NVIC_UNPEND1_INT42      0x00000400  // Interrupt 42 unpend
#define NVIC_UNPEND1_INT43      0x00000800  // Interrupt 43 unpend
#define NVIC_UNPEND1_INT44      0x00001000  // Interrupt 44 unpend
#define NVIC_UNPEND1_INT45      0x00002000  // Interrupt 45 unpend
#define NVIC_UNPEND1_INT46      0x00004000  // Interrupt 46 unpend
#define NVIC_UNPEND1_INT47      0x00008000  // Interrupt 47 unpend
#define NVIC_UNPEND1_INT48      0x00010000  // Interrupt 48 unpend
#define NVIC_UNPEND1_INT49      0x00020000  // Interrupt 49 unpend
#define NVIC_UNPEND1_INT50      0x00040000  // Interrupt 50 unpend
#define NVIC_UNPEND1_INT51      0x00080000  // Interrupt 51 unpend
#define NVIC_UNPEND1_INT52      0x00100000  // Interrupt 52 unpend
#define NVIC_UNPEND1_INT53      0x00200000  // Interrupt 53 unpend
#define NVIC_UNPEND1_INT54      0x00400000  // Interrupt 54 unpend

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_ACTIVE0 register.
//
//*****************************************************************************
#define NVIC_ACTIVE0_INT_M      0xFFFFFFFF  // Interrupt Active
#define NVIC_ACTIVE0_INT0       0x00000001  // Interrupt 0 active
#define NVIC_ACTIVE0_INT1       0x00000002  // Interrupt 1 active
#define NVIC_ACTIVE0_INT2       0x00000004  // Interrupt 2 active
#define NVIC_ACTIVE0_INT3       0x00000008  // Interrupt 3 active
#define NVIC_ACTIVE0_INT4       0x00000010  // Interrupt 4 active
#define NVIC_ACTIVE0_INT5       0x00000020  // Interrupt 5 active
#define NVIC_ACTIVE0_INT6       0x00000040  // Interrupt 6 active
#define NVIC_ACTIVE0_INT7       0x00000080  // Interrupt 7 active
#define NVIC_ACTIVE0_INT8       0x00000100  // Interrupt 8 active
#define NVIC_ACTIVE0_INT9       0x00000200  // Interrupt 9 active
#define NVIC_ACTIVE0_INT10      0x00000400  // Interrupt 10 active
#define NVIC_ACTIVE0_INT11      0x00000800  // Interrupt 11 active
#define NVIC_ACTIVE0_INT12      0x00001000  // Interrupt 12 active
#define NVIC_ACTIVE0_INT13      0x00002000  // Interrupt 13 active
#define NVIC_ACTIVE0_INT14      0x00004000  // Interrupt 14 active
#define NVIC_ACTIVE0_INT15      0x00008000  // Interrupt 15 active
#define NVIC_ACTIVE0_INT16      0x00010000  // Interrupt 16 active
#define NVIC_ACTIVE0_INT17      0x00020000  // Interrupt 17 active
#define NVIC_ACTIVE0_INT18      0x00040000  // Interrupt 18 active
#define NVIC_ACTIVE0_INT19      0x00080000  // Interrupt 19 active
#define NVIC_ACTIVE0_INT20      0x00100000  // Interrupt 20 active
#define NVIC_ACTIVE0_INT21      0x00200000  // Interrupt 21 active
#define NVIC_ACTIVE0_INT22      0x00400000  // Interrupt 22 active
#define NVIC_ACTIVE0_INT23      0x00800000  // Interrupt 23 active
#define NVIC_ACTIVE0_INT24      0x01000000  // Interrupt 24 active
#define NVIC_ACTIVE0_INT25      0x02000000  // Interrupt 25 active
#define NVIC_ACTIVE0_INT26      0x04000000  // Interrupt 26 active
#define NVIC_ACTIVE0_INT27      0x08000000  // Interrupt 27 active
#define NVIC_ACTIVE0_INT28      0x10000000  // Interrupt 28 active
#define NVIC_ACTIVE0_INT29      0x20000000  // Interrupt 29 active
#define NVIC_ACTIVE0_INT30      0x40000000  // Interrupt 30 active
#define NVIC_ACTIVE0_INT31      0x80000000  // Interrupt 31 active

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_ACTIVE1 register.
//
//*****************************************************************************
#define NVIC_ACTIVE1_INT_M      0x007FFFFF  // Interrupt Active
#define NVIC_ACTIVE1_INT32      0x00000001  // Interrupt 32 active
#define NVIC_ACTIVE1_INT33      0x00000002  // Interrupt 33 active
#define NVIC_ACTIVE1_INT34      0x00000004  // Interrupt 34 active
#define NVIC_ACTIVE1_INT35      0x00000008  // Interrupt 35 active
#define NVIC_ACTIVE1_INT36      0x00000010  // Interrupt 36 active
#define NVIC_ACTIVE1_INT37      0x00000020  // Interrupt 37 active
#define NVIC_ACTIVE1_INT38      0x00000040  // Interrupt 38 active
#define NVIC_ACTIVE1_INT39      0x00000080  // Interrupt 39 active
#define NVIC_ACTIVE1_INT40      0x00000100  // Interrupt 40 active
#define NVIC_ACTIVE1_INT41      0x00000200  // Interrupt 41 active
#define NVIC_ACTIVE1_INT42      0x00000400  // Interrupt 42 active
#define NVIC_ACTIVE1_INT43      0x00000800  // Interrupt 43 active
#define NVIC_ACTIVE1_INT44      0x00001000  // Interrupt 44 active
#define NVIC_ACTIVE1_INT45      0x00002000  // Interrupt 45 active
#define NVIC_ACTIVE1_INT46      0x00004000  // Interrupt 46 active
#define NVIC_ACTIVE1_INT47      0x00008000  // Interrupt 47 active
#define NVIC_ACTIVE1_INT48      0x00010000  // Interrupt 48 active
#define NVIC_ACTIVE1_INT49      0x00020000  // Interrupt 49 active
#define NVIC_ACTIVE1_INT50      0x00040000  // Interrupt 50 active
#define NVIC_ACTIVE1_INT51      0x00080000  // Interrupt 51 active
#define NVIC_ACTIVE1_INT52      0x00100000  // Interrupt 52 active
#define NVIC_ACTIVE1_INT53      0x00200000  // Interrupt 53 active
#define NVIC_ACTIVE1_INT54      0x00400000  // Interrupt 54 active

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI0 register.
//
//*****************************************************************************
#define NVIC_PRI0_INT3_M        0xE0000000  // Interrupt 3 Priority Mask
#define NVIC_PRI0_INT2_M        0x00E00000  // Interrupt 2 Priority Mask
#define NVIC_PRI0_INT1_M        0x0000E000  // Interrupt 1 Priority Mask
#define NVIC_PRI0_INT0_M        0x000000E0  // Interrupt 0 Priority Mask
#define NVIC_PRI0_INT3_S        29
#define NVIC_PRI0_INT2_S        21
#define NVIC_PRI0_INT1_S        13
#define NVIC_PRI0_INT0_S        5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI1 register.
//
//*****************************************************************************
#define NVIC_PRI1_INT7_M        0xE0000000  // Interrupt 7 Priority Mask
#define NVIC_PRI1_INT6_M        0x00E00000  // Interrupt 6 Priority Mask
#define NVIC_PRI1_INT5_M        0x0000E000  // Interrupt 5 Priority Mask
#define NVIC_PRI1_INT4_M        0x000000E0  // Interrupt 4 Priority Mask
#define NVIC_PRI1_INT7_S        29
#define NVIC_PRI1_INT6_S        21
#define NVIC_PRI1_INT5_S        13
#define NVIC_PRI1_INT4_S        5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI2 register.
//
//*****************************************************************************
#define NVIC_PRI2_INT11_M       0xE0000000  // Interrupt 11 Priority Mask
#define NVIC_PRI2_INT10_M       0x00E00000  // Interrupt 10 Priority Mask
#define NVIC_PRI2_INT9_M        0x0000E000  // Interrupt 9 Priority Mask
#define NVIC_PRI2_INT8_M        0x000000E0  // Interrupt 8 Priority Mask
#define NVIC_PRI2_INT11_S       29
#define NVIC_PRI2_INT10_S       21
#define NVIC_PRI2_INT9_S        13
#define NVIC_PRI2_INT8_S        5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI3 register.
//
//*****************************************************************************
#define NVIC_PRI3_INT15_M       0xE0000000  // Interrupt 15 Priority Mask
#define NVIC_PRI3_INT14_M       0x00E00000  // Interrupt 14 Priority Mask
#define NVIC_PRI3_INT13_M       0x0000E000  // Interrupt 13 Priority Mask
#define NVIC_PRI3_INT12_M       0x000000E0  // Interrupt 12 Priority Mask
#define NVIC_PRI3_INT15_S       29
#define NVIC_PRI3_INT14_S       21
#define NVIC_PRI3_INT13_S       13
#define NVIC_PRI3_INT12_S       5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI4 register.
//
//*****************************************************************************
#define NVIC_PRI4_INT19_M       0xE0000000  // Interrupt 19 Priority Mask
#define NVIC_PRI4_INT18_M       0x00E00000  // Interrupt 18 Priority Mask
#define NVIC_PRI4_INT17_M       0x0000E000  // Interrupt 17 Priority Mask
#define NVIC_PRI4_INT16_M       0x000000E0  // Interrupt 16 Priority Mask
#define NVIC_PRI4_INT19_S       29
#define NVIC_PRI4_INT18_S       21
#define NVIC_PRI4_INT17_S       13
#define NVIC_PRI4_INT16_S       5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI5 register.
//
//*****************************************************************************
#define NVIC_PRI5_INT23_M       0xE0000000  // Interrupt 23 Priority Mask
#define NVIC_PRI5_INT22_M       0x00E00000  // Interrupt 22 Priority Mask
#define NVIC_PRI5_INT21_M       0x0000E000  // Interrupt 21 Priority Mask
#define NVIC_PRI5_INT20_M       0x000000E0  // Interrupt 20 Priority Mask
#define NVIC_PRI5_INT23_S       29
#define NVIC_PRI5_INT22_S       21
#define NVIC_PRI5_INT21_S       13
#define NVIC_PRI5_INT20_S       5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI6 register.
//
//*****************************************************************************
#define NVIC_PRI6_INT27_M       0xE0000000  // Interrupt 27 Priority Mask
#define NVIC_PRI6_INT26_M       0x00E00000  // Interrupt 26 Priority Mask
#define NVIC_PRI6_INT25_M       0x0000E000  // Interrupt 25 Priority Mask
#define NVIC_PRI6_INT24_M       0x000000E0  // Interrupt 24 Priority Mask
#define NVIC_PRI6_INT27_S       29
#define NVIC_PRI6_INT26_S       21
#define NVIC_PRI6_INT25_S       13
#define NVIC_PRI6_INT24_S       5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI7 register.
//
//*****************************************************************************
#define NVIC_PRI7_INT31_M       0xE0000000  // Interrupt 31 Priority Mask
#define NVIC_PRI7_INT30_M       0x00E00000  // Interrupt 30 Priority Mask
#define NVIC_PRI7_INT29_M       0x0000E000  // Interrupt 29 Priority Mask
#define NVIC_PRI7_INT28_M       0x000000E0  // Interrupt 28 Priority Mask
#define NVIC_PRI7_INT31_S       29
#define NVIC_PRI7_INT30_S       21
#define NVIC_PRI7_INT29_S       13
#define NVIC_PRI7_INT28_S       5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI8 register.
//
//*****************************************************************************
#define NVIC_PRI8_INT35_M       0xE0000000  // Interrupt 35 Priority Mask
#define NVIC_PRI8_INT34_M       0x00E00000  // Interrupt 34 Priority Mask
#define NVIC_PRI8_INT33_M       0x0000E000  // Interrupt 33 Priority Mask
#define NVIC_PRI8_INT32_M       0x000000E0  // Interrupt 32 Priority Mask
#define NVIC_PRI8_INT35_S       29
#define NVIC_PRI8_INT34_S       21
#define NVIC_PRI8_INT33_S       13
#define NVIC_PRI8_INT32_S       5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI9 register.
//
//*****************************************************************************
#define NVIC_PRI9_INT39_M       0xE0000000  // Interrupt 39 Priority Mask
#define NVIC_PRI9_INT38_M       0x00E00000  // Interrupt 38 Priority Mask
#define NVIC_PRI9_INT37_M       0x0000E000  // Interrupt 37 Priority Mask
#define NVIC_PRI9_INT36_M       0x000000E0  // Interrupt 36 Priority Mask
#define NVIC_PRI9_INT39_S       29
#define NVIC_PRI9_INT38_S       21
#define NVIC_PRI9_INT37_S       13
#define NVIC_PRI9_INT36_S       5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI10 register.
//
//*****************************************************************************
#define NVIC_PRI10_INT43_M      0xE0000000  // Interrupt 43 Priority Mask
#define NVIC_PRI10_INT42_M      0x00E00000  // Interrupt 42 Priority Mask
#define NVIC_PRI10_INT41_M      0x0000E000  // Interrupt 41 Priority Mask
#define NVIC_PRI10_INT40_M      0x000000E0  // Interrupt 40 Priority Mask
#define NVIC_PRI10_INT43_S      29
#define NVIC_PRI10_INT42_S      21
#define NVIC_PRI10_INT41_S      13
#define NVIC_PRI10_INT40_S      5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI11 register.
//
//*****************************************************************************
#define NVIC_PRI11_INT47_M      0xE0000000  // Interrupt 47 Priority Mask
#define NVIC_PRI11_INT46_M      0x00E00000  // Interrupt 46 Priority Mask
#define NVIC_PRI11_INT45_M      0x0000E000  // Interrupt 45 Priority Mask
#define NVIC_PRI11_INT44_M      0x000000E0  // Interrupt 44 Priority Mask
#define NVIC_PRI11_INT47_S      29
#define NVIC_PRI11_INT46_S      21
#define NVIC_PRI11_INT45_S      13
#define NVIC_PRI11_INT44_S      5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI12 register.
//
//*****************************************************************************
#define NVIC_PRI12_INT51_M      0xE0000000  // Interrupt 51 Priority Mask
#define NVIC_PRI12_INT50_M      0x00E00000  // Interrupt 50 Priority Mask
#define NVIC_PRI12_INT49_M      0x0000E000  // Interrupt 49 Priority Mask
#define NVIC_PRI12_INT48_M      0x000000E0  // Interrupt 48 Priority Mask
#define NVIC_PRI12_INT51_S      29
#define NVIC_PRI12_INT50_S      21
#define NVIC_PRI12_INT49_S      13
#define NVIC_PRI12_INT48_S      5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_PRI13 register.
//
//*****************************************************************************
#define NVIC_PRI13_INT55_M      0xE0000000  // Interrupt 55 Priority Mask
#define NVIC_PRI13_INT54_M      0x00E00000  // Interrupt 54 Priority Mask
#define NVIC_PRI13_INT53_M      0x0000E000  // Interrupt 53 Priority Mask
#define NVIC_PRI13_INT52_M      0x000000E0  // Interrupt 52 Priority Mask
#define NVIC_PRI13_INT55_S      29
#define NVIC_PRI13_INT54_S      21
#define NVIC_PRI13_INT53_S      13
#define NVIC_PRI13_INT52_S      5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_CPUID register.
//
//*****************************************************************************
#define NVIC_CPUID_IMP_M        0xFF000000  // Implementer Code
#define NVIC_CPUID_IMP_ARM      0x41000000  // ARM
#define NVIC_CPUID_VAR_M        0x00F00000  // Variant Number
#define NVIC_CPUID_CON_M        0x000F0000  // Constant
#define NVIC_CPUID_PARTNO_M     0x0000FFF0  // Part Number
#define NVIC_CPUID_PARTNO_CM3   0x0000C230  // Cortex-M3 processor
#define NVIC_CPUID_PARTNO_CM4   0x0000C240  // Cortex-M4 processor
#define NVIC_CPUID_REV_M        0x0000000F  // Revision Number

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_INT_CTRL register.
//
//*****************************************************************************
#define NVIC_INT_CTRL_NMI_SET   0x80000000  // NMI Set Pending
#define NVIC_INT_CTRL_PEND_SV   0x10000000  // PendSV Set Pending
#define NVIC_INT_CTRL_UNPEND_SV 0x08000000  // PendSV Clear Pending
#define NVIC_INT_CTRL_PENDSTSET 0x04000000  // SysTick Set Pending
#define NVIC_INT_CTRL_PENDSTCLR 0x02000000  // SysTick Clear Pending
#define NVIC_INT_CTRL_ISR_PRE   0x00800000  // Debug Interrupt Handling
#define NVIC_INT_CTRL_ISR_PEND  0x00400000  // Interrupt Pending
#define NVIC_INT_CTRL_VEC_PEN_M 0x0007F000  // Interrupt Pending Vector Number
#undef NVIC_INT_CTRL_VEC_PEN_M
#define NVIC_INT_CTRL_VEC_PEN_M 0x000FF000  // Interrupt Pending Vector Number
#define NVIC_INT_CTRL_VEC_PEN_NMI \
                                0x00002000  // NMI
#define NVIC_INT_CTRL_VEC_PEN_HARD \
                                0x00003000  // Hard fault
#define NVIC_INT_CTRL_VEC_PEN_MEM \
                                0x00004000  // Memory management fault
#define NVIC_INT_CTRL_VEC_PEN_BUS \
                                0x00005000  // Bus fault
#define NVIC_INT_CTRL_VEC_PEN_USG \
                                0x00006000  // Usage fault
#define NVIC_INT_CTRL_VEC_PEN_SVC \
                                0x0000B000  // SVCall
#define NVIC_INT_CTRL_VEC_PEN_PNDSV \
                                0x0000E000  // PendSV
#define NVIC_INT_CTRL_VEC_PEN_TICK \
                                0x0000F000  // SysTick
#define NVIC_INT_CTRL_RET_BASE  0x00000800  // Return to Base
#define NVIC_INT_CTRL_VEC_ACT_M 0x0000007F  // Interrupt Pending Vector Number
#undef NVIC_INT_CTRL_VEC_ACT_M
#define NVIC_INT_CTRL_VEC_ACT_M 0x000000FF  // Interrupt Pending Vector Number
#define NVIC_INT_CTRL_VEC_PEN_S 12
#define NVIC_INT_CTRL_VEC_ACT_S 0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_VTABLE register.
//
//*****************************************************************************
#define NVIC_VTABLE_BASE        0x20000000  // Vector Table Base
#define NVIC_VTABLE_OFFSET_M    0x1FFFFE00  // Vector Table Offset
#undef NVIC_VTABLE_OFFSET_M
#define NVIC_VTABLE_OFFSET_M    0x1FFFFC00  // Vector Table Offset
#define NVIC_VTABLE_OFFSET_S    9
#undef NVIC_VTABLE_OFFSET_S
#define NVIC_VTABLE_OFFSET_S    10

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_APINT register.
//
//*****************************************************************************
#define NVIC_APINT_VECTKEY_M    0xFFFF0000  // Register Key
#define NVIC_APINT_VECTKEY      0x05FA0000  // Vector key
#define NVIC_APINT_ENDIANESS    0x00008000  // Data Endianess
#define NVIC_APINT_PRIGROUP_M   0x00000700  // Interrupt Priority Grouping
#define NVIC_APINT_PRIGROUP_7_1 0x00000000  // Priority group 7.1 split
#define NVIC_APINT_PRIGROUP_6_2 0x00000100  // Priority group 6.2 split
#define NVIC_APINT_PRIGROUP_5_3 0x00000200  // Priority group 5.3 split
#define NVIC_APINT_PRIGROUP_4_4 0x00000300  // Priority group 4.4 split
#define NVIC_APINT_PRIGROUP_3_5 0x00000400  // Priority group 3.5 split
#define NVIC_APINT_PRIGROUP_2_6 0x00000500  // Priority group 2.6 split
#define NVIC_APINT_PRIGROUP_1_7 0x00000600  // Priority group 1.7 split
#define NVIC_APINT_PRIGROUP_0_8 0x00000700  // Priority group 0.8 split
#define NVIC_APINT_SYSRESETREQ  0x00000004  // System Reset Request
#define NVIC_APINT_VECT_CLR_ACT 0x00000002  // Clear Active NMI / Fault
#define NVIC_APINT_VECT_RESET   0x00000001  // System Reset

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_SYS_CTRL register.
//
//*****************************************************************************
#define NVIC_SYS_CTRL_SEVONPEND 0x00000010  // Wake Up on Pending
#define NVIC_SYS_CTRL_SLEEPDEEP 0x00000004  // Deep Sleep Enable
#define NVIC_SYS_CTRL_SLEEPEXIT 0x00000002  // Sleep on ISR Exit

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_CFG_CTRL register.
//
//*****************************************************************************
#define NVIC_CFG_CTRL_STKALIGN  0x00000200  // Stack Alignment on Exception
                                            // Entry
#define NVIC_CFG_CTRL_BFHFNMIGN 0x00000100  // Ignore Bus Fault in NMI and
                                            // Fault
#define NVIC_CFG_CTRL_DIV0      0x00000010  // Trap on Divide by 0
#define NVIC_CFG_CTRL_UNALIGNED 0x00000008  // Trap on Unaligned Access
#define NVIC_CFG_CTRL_MAIN_PEND 0x00000002  // Allow Main Interrupt Trigger
#define NVIC_CFG_CTRL_BASE_THR  0x00000001  // Thread State Control

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_SYS_PRI1 register.
//
//*****************************************************************************
#define NVIC_SYS_PRI1_USAGE_M   0x00E00000  // Usage Fault Priority
#define NVIC_SYS_PRI1_BUS_M     0x0000E000  // Bus Fault Priority
#define NVIC_SYS_PRI1_MEM_M     0x000000E0  // Memory Management Fault Priority
#define NVIC_SYS_PRI1_USAGE_S   21
#define NVIC_SYS_PRI1_BUS_S     13
#define NVIC_SYS_PRI1_MEM_S     5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_SYS_PRI2 register.
//
//*****************************************************************************
#define NVIC_SYS_PRI2_SVC_M     0xE0000000  // SVCall Priority
#define NVIC_SYS_PRI2_SVC_S     29

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_SYS_PRI3 register.
//
//*****************************************************************************
#define NVIC_SYS_PRI3_TICK_M    0xE0000000  // SysTick Exception Priority
#define NVIC_SYS_PRI3_PENDSV_M  0x00E00000  // PendSV Priority
#define NVIC_SYS_PRI3_DEBUG_M   0x000000E0  // Debug Priority
#define NVIC_SYS_PRI3_TICK_S    29
#define NVIC_SYS_PRI3_PENDSV_S  21
#define NVIC_SYS_PRI3_DEBUG_S   5

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_SYS_HND_CTRL
// register.
//
//*****************************************************************************
#define NVIC_SYS_HND_CTRL_USAGE 0x00040000  // Usage Fault Enable
#define NVIC_SYS_HND_CTRL_BUS   0x00020000  // Bus Fault Enable
#define NVIC_SYS_HND_CTRL_MEM   0x00010000  // Memory Management Fault Enable
#define NVIC_SYS_HND_CTRL_SVC   0x00008000  // SVC Call Pending
#define NVIC_SYS_HND_CTRL_BUSP  0x00004000  // Bus Fault Pending
#define NVIC_SYS_HND_CTRL_MEMP  0x00002000  // Memory Management Fault Pending
#define NVIC_SYS_HND_CTRL_USAGEP \
                                0x00001000  // Usage Fault Pending
#define NVIC_SYS_HND_CTRL_TICK  0x00000800  // SysTick Exception Active
#define NVIC_SYS_HND_CTRL_PNDSV 0x00000400  // PendSV Exception Active
#define NVIC_SYS_HND_CTRL_MON   0x00000100  // Debug Monitor Active
#define NVIC_SYS_HND_CTRL_SVCA  0x00000080  // SVC Call Active
#define NVIC_SYS_HND_CTRL_USGA  0x00000008  // Usage Fault Active
#define NVIC_SYS_HND_CTRL_BUSA  0x00000002  // Bus Fault Active
#define NVIC_SYS_HND_CTRL_MEMA  0x00000001  // Memory Management Fault Active

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_FAULT_STAT
// register.
//
//*****************************************************************************
#define NVIC_FAULT_STAT_DIV0    0x02000000  // Divide-by-Zero Usage Fault
#define NVIC_FAULT_STAT_UNALIGN 0x01000000  // Unaligned Access Usage Fault
#define NVIC_FAULT_STAT_NOCP    0x00080000  // No Coprocessor Usage Fault
#define NVIC_FAULT_STAT_INVPC   0x00040000  // Invalid PC Load Usage Fault
#define NVIC_FAULT_STAT_INVSTAT 0x00020000  // Invalid State Usage Fault
#define NVIC_FAULT_STAT_UNDEF   0x00010000  // Undefined Instruction Usage
                                            // Fault
#define NVIC_FAULT_STAT_BFARV   0x00008000  // Bus Fault Address Register Valid
#define NVIC_FAULT_STAT_BLSPERR 0x00002000  // Bus Fault on Floating-Point Lazy
                                            // State Preservation
#define NVIC_FAULT_STAT_BSTKE   0x00001000  // Stack Bus Fault
#define NVIC_FAULT_STAT_BUSTKE  0x00000800  // Unstack Bus Fault
#define NVIC_FAULT_STAT_IMPRE   0x00000400  // Imprecise Data Bus Error
#define NVIC_FAULT_STAT_PRECISE 0x00000200  // Precise Data Bus Error
#define NVIC_FAULT_STAT_IBUS    0x00000100  // Instruction Bus Error
#define NVIC_FAULT_STAT_MMARV   0x00000080  // Memory Management Fault Address
                                            // Register Valid
#define NVIC_FAULT_STAT_MLSPERR 0x00000020  // Memory Management Fault on
                                            // Floating-Point Lazy State
                                            // Preservation
#define NVIC_FAULT_STAT_MSTKE   0x00000010  // Stack Access Violation
#define NVIC_FAULT_STAT_MUSTKE  0x00000008  // Unstack Access Violation
#define NVIC_FAULT_STAT_DERR    0x00000002  // Data Access Violation
#define NVIC_FAULT_STAT_IERR    0x00000001  // Instruction Access Violation

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_HFAULT_STAT
// register.
//
//*****************************************************************************
#define NVIC_HFAULT_STAT_DBG    0x80000000  // Debug Event
#define NVIC_HFAULT_STAT_FORCED 0x40000000  // Forced Hard Fault
#define NVIC_HFAULT_STAT_VECT   0x00000002  // Vector Table Read Fault

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_DEBUG_STAT
// register.
//
//*****************************************************************************
#define NVIC_DEBUG_STAT_EXTRNL  0x00000010  // EDBGRQ asserted
#define NVIC_DEBUG_STAT_VCATCH  0x00000008  // Vector catch
#define NVIC_DEBUG_STAT_DWTTRAP 0x00000004  // DWT match
#define NVIC_DEBUG_STAT_BKPT    0x00000002  // Breakpoint instruction
#define NVIC_DEBUG_STAT_HALTED  0x00000001  // Halt request

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_MM_ADDR register.
//
//*****************************************************************************
#define NVIC_MM_ADDR_M          0xFFFFFFFF  // Fault Address
#define NVIC_MM_ADDR_S          0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_FAULT_ADDR
// register.
//
//*****************************************************************************
#define NVIC_FAULT_ADDR_M       0xFFFFFFFF  // Fault Address
#define NVIC_FAULT_ADDR_S       0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_DBG_CTRL register.
//
//*****************************************************************************
#define NVIC_DBG_CTRL_DBGKEY_M  0xFFFF0000  // Debug key mask
#define NVIC_DBG_CTRL_DBGKEY    0xA05F0000  // Debug key
#define NVIC_DBG_CTRL_S_RESET_ST \
                                0x02000000  // Core has reset since last read
#define NVIC_DBG_CTRL_S_RETIRE_ST \
                                0x01000000  // Core has executed insruction
                                            // since last read
#define NVIC_DBG_CTRL_S_LOCKUP  0x00080000  // Core is locked up
#define NVIC_DBG_CTRL_S_SLEEP   0x00040000  // Core is sleeping
#define NVIC_DBG_CTRL_S_HALT    0x00020000  // Core status on halt
#define NVIC_DBG_CTRL_S_REGRDY  0x00010000  // Register read/write available
#define NVIC_DBG_CTRL_C_SNAPSTALL \
                                0x00000020  // Breaks a stalled load/store
#define NVIC_DBG_CTRL_C_MASKINT 0x00000008  // Mask interrupts when stepping
#define NVIC_DBG_CTRL_C_STEP    0x00000004  // Step the core
#define NVIC_DBG_CTRL_C_HALT    0x00000002  // Halt the core
#define NVIC_DBG_CTRL_C_DEBUGEN 0x00000001  // Enable debug

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_DBG_XFER register.
//
//*****************************************************************************
#define NVIC_DBG_XFER_REG_WNR   0x00010000  // Write or not read
#define NVIC_DBG_XFER_REG_SEL_M 0x0000001F  // Register
#define NVIC_DBG_XFER_REG_R0    0x00000000  // Register R0
#define NVIC_DBG_XFER_REG_R1    0x00000001  // Register R1
#define NVIC_DBG_XFER_REG_R2    0x00000002  // Register R2
#define NVIC_DBG_XFER_REG_R3    0x00000003  // Register R3
#define NVIC_DBG_XFER_REG_R4    0x00000004  // Register R4
#define NVIC_DBG_XFER_REG_R5    0x00000005  // Register R5
#define NVIC_DBG_XFER_REG_R6    0x00000006  // Register R6
#define NVIC_DBG_XFER_REG_R7    0x00000007  // Register R7
#define NVIC_DBG_XFER_REG_R8    0x00000008  // Register R8
#define NVIC_DBG_XFER_REG_R9    0x00000009  // Register R9
#define NVIC_DBG_XFER_REG_R10   0x0000000A  // Register R10
#define NVIC_DBG_XFER_REG_R11   0x0000000B  // Register R11
#define NVIC_DBG_XFER_REG_R12   0x0000000C  // Register R12
#define NVIC_DBG_XFER_REG_R13   0x0000000D  // Register R13
#define NVIC_DBG_XFER_REG_R14   0x0000000E  // Register R14
#define NVIC_DBG_XFER_REG_R15   0x0000000F  // Register R15
#define NVIC_DBG_XFER_REG_FLAGS 0x00000010  // xPSR/Flags register
#define NVIC_DBG_XFER_REG_MSP   0x00000011  // Main SP
#define NVIC_DBG_XFER_REG_PSP   0x00000012  // Process SP
#define NVIC_DBG_XFER_REG_DSP   0x00000013  // Deep SP
#define NVIC_DBG_XFER_REG_CFBP  0x00000014  // Control/Fault/BasePri/PriMask

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_DBG_DATA register.
//
//*****************************************************************************
#define NVIC_DBG_DATA_M         0xFFFFFFFF  // Data temporary cache
#define NVIC_DBG_DATA_S         0

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_DBG_INT register.
//
//*****************************************************************************
#define NVIC_DBG_INT_HARDERR    0x00000400  // Debug trap on hard fault
#define NVIC_DBG_INT_INTERR     0x00000200  // Debug trap on interrupt errors
#define NVIC_DBG_INT_BUSERR     0x00000100  // Debug trap on bus error
#define NVIC_DBG_INT_STATERR    0x00000080  // Debug trap on usage fault state
#define NVIC_DBG_INT_CHKERR     0x00000040  // Debug trap on usage fault check
#define NVIC_DBG_INT_NOCPERR    0x00000020  // Debug trap on coprocessor error
#define NVIC_DBG_INT_MMERR      0x00000010  // Debug trap on mem manage fault
#define NVIC_DBG_INT_RESET      0x00000008  // Core reset status
#define NVIC_DBG_INT_RSTPENDCLR 0x00000004  // Clear pending core reset
#define NVIC_DBG_INT_RSTPENDING 0x00000002  // Core reset is pending
#define NVIC_DBG_INT_RSTVCATCH  0x00000001  // Reset vector catch

//*****************************************************************************
//
// The following are defines for the bit fields in the NVIC_SW_TRIG register.
//
//*****************************************************************************
#define NVIC_SW_TRIG_INTID_M    0x0000003F  // Interrupt ID
#define NVIC_SW_TRIG_INTID_S    0

#endif // __HW_NVIC_H__
