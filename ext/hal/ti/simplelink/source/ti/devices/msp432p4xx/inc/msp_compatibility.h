//*****************************************************************************
//
// Copyright (C) 2013 - 2015 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the
//  distribution.
//
//  Neither the name of Texas Instruments Incorporated nor the names of
//  its contributors may be used to endorse or promote products derived
//  from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// MSP430 intrinsic redefinitions for use with MSP432 Family Devices
//
//****************************************************************************

/******************************************************************************
* Definitions for 8/16/32-bit wide memory access                              *
******************************************************************************/
#define HWREG8(x)         (*((volatile uint8_t *)(x)))
#define HWREG16(x)        (*((volatile uint16_t *)(x)))
#define HWREG32(x)        (*((volatile uint32_t *)(x)))
#define HWREG(x)          (HWREG16(x))
#define HWREG8_L(x)       (*((volatile uint8_t *)((uint8_t *)&x)))
#define HWREG8_H(x)       (*((volatile uint8_t *)(((uint8_t *)&x)+1)))
#define HWREG16_L(x)      (*((volatile uint16_t *)((uint16_t *)&x)))
#define HWREG16_H(x)      (*((volatile uint16_t *)(((uint16_t *)&x)+1)))

/******************************************************************************
* Definitions for 8/16/32-bit wide bit band access                            *
******************************************************************************/
#define HWREGBIT8(x, b)   (HWREG8(((uint32_t)(x) & 0xF0000000) | 0x02000000 | (((uint32_t)(x) & 0x000FFFFF) << 5) | ((b) << 2)))
#define HWREGBIT16(x, b)  (HWREG16(((uint32_t)(x) & 0xF0000000) | 0x02000000 | (((uint32_t)(x) & 0x000FFFFF) << 5) | ((b) << 2)))
#define HWREGBIT32(x, b)  (HWREG32(((uint32_t)(x) & 0xF0000000) | 0x02000000 | (((uint32_t)(x) & 0x000FFFFF) << 5) | ((b) << 2)))

// Intrinsics with ARM equivalents
#if defined ( __TI_ARM__ ) /* TI CGT Compiler */

#include <cmsis_ccs.h>

#define __sleep()                       __wfi()
#define __deep_sleep()                  { (*((volatile uint32_t *)(0xE000ED10))) |= 0x00000004; __wfi(); (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000004; }
#define __low_power_mode_off_on_exit()  { (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000002; }
#define __get_SP_register()             __get_MSP()
#define __set_SP_register(x)            __set_MSP(x)
#define __get_interrupt_state()         __get_PRIMASK()
#define __set_interrupt_state(x)        __set_PRIMASK(x)
#define __enable_interrupt()            _enable_IRQ()
#define __enable_interrupts()           _enable_IRQ()
#define __disable_interrupt()           _disable_IRQ()
#define __disable_interrupts()          _disable_IRQ()
#define __no_operation()                __asm("  nop")

#elif defined ( __ICCARM__ )  /* IAR Compiler */

#include <stdint.h>

#define __INLINE                        inline
#include <cmsis_iar.h>

#define __sleep()                       __WFI()
#define __deep_sleep()                  { (*((volatile uint32_t *)(0xE000ED10))) |= 0x00000004; __WFI(); (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000004; }
#define __low_power_mode_off_on_exit()  { (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000002; }
#define __get_SP_register()             __get_MSP()
#define __set_SP_register()             __set_MSP()
#define __get_interrupt_state()         __get_PRIMASK()
#define __set_interrupt_state(x)        __set_PRIMASK(x)
#define __enable_interrupt()            __asm("  cpsie i")
#define __enable_interrupts()           __asm("  cpsie i")
#define __disable_interrupt()           __asm("  cpsid i")
#define __disable_interrupts()          __asm("  cpsid i")
#define __no_operation()                __asm("  nop")

// Intrinsics without ARM equivalents
#define __bcd_add_short(x,y)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bcd_add_long(x,y)             { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bcd_add_long_long(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __even_in_range(x,y)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_char(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_short(x,y)       { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_long(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __never_executed()              { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __op_code()                     { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __code_distance()               { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bic_SR_register(x)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bis_SR_register(x)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bis_SR_register_on_exit(x)    { while(1); /* Using not-supported MSP430 intrinsic. Recommended to write to SCS_SCR register. */ }
#define __bic_SR_register_on_exit(x)    { while(1); /* Using not-supported MSP430 intrinsic. Recommended to write to SCS_SCR register. */ }
#define __delay_cycles(x)               { while(1); /* Using not-supported MSP430 intrinsic. Recommended to use a timer or a custom for loop. */ }

#elif defined ( __CC_ARM ) /* ARM Compiler */

#define __sleep()                       __wfi()
#define __deep_sleep()                  { (*((volatile uint32_t *)(0xE000ED10))) |= 0x00000004; __wfi(); (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000004; }
#define __low_power_mode_off_on_exit()  { (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000002; }
#define __get_SP_register()             __get_MSP()
#define __set_SP_register(x)            __set_MSP(x)
#define __get_interrupt_state()         __get_PRIMASK()
#define __set_interrupt_state(x)        __set_PRIMASK(x)
#define __enable_interrupt()            __asm("  cpsie i")
#define __enable_interrupts()           __asm("  cpsie i")
#define __disable_interrupt()           __asm("  cpsid i")
#define __disable_interrupts()          __asm("  cpsid i")
#define __no_operation()                __asm("  nop")

// Intrinsics without ARM equivalents
#define __bcd_add_short(x,y)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bcd_add_long(x,y)             { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bcd_add_long_long(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __even_in_range(x,y)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_char(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_short(x,y)       { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_long(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __never_executed()              { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __op_code()                     { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __code_distance()               { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bic_SR_register(x)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bis_SR_register(x)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bis_SR_register_on_exit(x)    { while(1); /* Using not-supported MSP430 intrinsic. Recommended to write to SCS_SCR register. */ }
#define __bic_SR_register_on_exit(x)    { while(1); /* Using not-supported MSP430 intrinsic. Recommended to write to SCS_SCR register. */ }
#define __delay_cycles(x)               { while(1); /* Using not-supported MSP430 intrinsic. Recommended to use a timer or a custom for loop. */ }

#elif defined ( __GNUC__ ) /* GCC Compiler */
#undef __wfi
#define __wfi()                         __asm("  wfi")
#define __sleep()                       __wfi()
#define __deep_sleep()                  { (*((volatile uint32_t *)(0xE000ED10))) |= 0x00000004; __wfi(); (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000004; }
#define __low_power_mode_off_on_exit()  { (*((volatile uint32_t *)(0xE000ED10))) &= ~0x00000002; }
#define __get_SP_register()             __get_MSP()
#define __set_SP_register(x)            __set_MSP(x)
#define __get_interrupt_state()         __get_PRIMASK()
#define __set_interrupt_state(x)        __set_PRIMASK(x)
#define __enable_interrupt()            __asm("  cpsie i")
#define __enable_interrupts()           __asm("  cpsie i")
#define __disable_interrupt()           __asm("  cpsid i")
#define __disable_interrupts()          __asm("  cpsid i")
#define __no_operation()                __asm("  nop")

// Intrinsics without ARM equivalents
#define __bcd_add_short(x,y)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bcd_add_long(x,y)             { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bcd_add_long_long(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __even_in_range(x,y)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_char(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_short(x,y)       { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __data20_write_long(x,y)        { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __never_executed()              { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __op_code()                     { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __code_distance()               { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bic_SR_register(x)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bis_SR_register(x)            { while(1); /* Using not-supported MSP430 intrinsic. No replacement available. */ }
#define __bis_SR_register_on_exit(x)    { while(1); /* Using not-supported MSP430 intrinsic. Recommended to write to SCS_SCR register. */ }
#define __bic_SR_register_on_exit(x)    { while(1); /* Using not-supported MSP430 intrinsic. Recommended to write to SCS_SCR register. */ }
#define __delay_cycles(x)               { while(1); /* Using not-supported MSP430 intrinsic. Recommended to use a timer or a custom for loop. */ }

#endif

// Intrinsics without ARM equivalents
#define __low_power_mode_0()            { __sleep(); }
#define __low_power_mode_1()            { __sleep(); }
#define __low_power_mode_2()            { __sleep(); }
#define __low_power_mode_3()            { __deep_sleep(); }
#define __low_power_mode_4()            { __deep_sleep(); }
#define __data16_read_addr(x)           (*((volatile uint32_t *)(x)))
#define __data20_read_char(x)           (*((volatile uint8_t *)(x)))
#define __data20_read_short(x)          (*((volatile uint16_t *)(x)))
#define __data20_read_long(x)           (*((volatile uint32_t *)(x)))
#define __data16_write_addr(x,y)        { (*((volatile uint32_t *)(x))) }
#define __get_SR_register()             0
#define __get_SR_register_on_exit()     0

// the following defines are deprecated and will be removed in future releases
#define ATLBASE                                  ALTBASE
#define CS_CTL1_SELM_7                           ((uint32_t)0x00000007)          /*!< for future use. Defaults to DCOCLK. Not recommended for use to ensure future  */
                                                                                 /* compatibilities. */
#define CS_CTL1_SELS_7                           ((uint32_t)0x00000070)          /*!< for furture use. Defaults to DCOCLK. Do not use to ensure future  */
                                                                                 /* compatibilities. */
#define CS_CTL1_SELA_3                           ((uint32_t)0x00000300)          /*!< for future use. Defaults to REFOCLK. Not recommended for use to ensure future  */
                                                                                 /* compatibilities. */
#define CS_CTL1_SELA_4                           ((uint32_t)0x00000400)          /*!< for future use. Defaults to REFOCLK. Not recommended for use to ensure future  */
                                                                                 /* compatibilities. */
#define CS_CTL1_SELA_5                           ((uint32_t)0x00000500)          /*!< for future use. Defaults to REFOCLK. Not recommended for use to ensure future  */
                                                                                 /* compatibilities. */
#define CS_CTL1_SELA_6                           ((uint32_t)0x00000600)          /*!< for future use. Defaults to REFOCLK. Not recommended for use to ensure future  */
                                                                                 /* compatibilities. */
#define CS_CTL1_SELA_7                           ((uint32_t)0x00000700)          /*!< for future use. Defaults to REFOCLK. Not recommended for use to ensure future  */
                                                                                 /* compatibilities. */
                                                                                 /* CS_CTL2[LFXTAGCOFF] Bits */
#define CS_CTL2_LFXTAGCOFF_OFS                   ( 7)                            /*!< LFXTAGCOFF Bit Offset */
#define CS_CTL2_LFXTAGCOFF                       ((uint32_t)0x00000080)          /*!< Disables the automatic gain control of the LFXT crystal */

/* CS_CTL3[FCNTHF2] Bits */
#define CS_CTL3_FCNTHF2_OFS                      ( 8)                            /*!< FCNTHF2 Bit Offset */
#define CS_CTL3_FCNTHF2_MASK                     ((uint32_t)0x00000300)          /*!< FCNTHF2 Bit Mask */
#define CS_CTL3_FCNTHF20                         ((uint32_t)0x00000100)          /*!< FCNTHF2 Bit 0 */
#define CS_CTL3_FCNTHF21                         ((uint32_t)0x00000200)          /*!< FCNTHF2 Bit 1 */
#define CS_CTL3_FCNTHF2_0                        ((uint32_t)0x00000000)          /*!< 2048 cycles */
#define CS_CTL3_FCNTHF2_1                        ((uint32_t)0x00000100)          /*!< 4096 cycles */
#define CS_CTL3_FCNTHF2_2                        ((uint32_t)0x00000200)          /*!< 8192 cycles */
#define CS_CTL3_FCNTHF2_3                        ((uint32_t)0x00000300)          /*!< 16384 cycles */
#define CS_CTL3_FCNTHF2__2048                    ((uint32_t)0x00000000)          /*!< 2048 cycles */
#define CS_CTL3_FCNTHF2__4096                    ((uint32_t)0x00000100)          /*!< 4096 cycles */
#define CS_CTL3_FCNTHF2__8192                    ((uint32_t)0x00000200)          /*!< 8192 cycles */
#define CS_CTL3_FCNTHF2__16384                   ((uint32_t)0x00000300)          /*!< 16384 cycles */
/* CS_CTL3[RFCNTHF2] Bits */
#define CS_CTL3_RFCNTHF2_OFS                     (10)                            /*!< RFCNTHF2 Bit Offset */
#define CS_CTL3_RFCNTHF2                         ((uint32_t)0x00000400)          /*!< Reset start fault counter for HFXT2 */
/* CS_CTL3[FCNTHF2_EN] Bits */
#define CS_CTL3_FCNTHF2_EN_OFS                   (11)                            /*!< FCNTHF2_EN Bit Offset */
#define CS_CTL3_FCNTHF2_EN                       ((uint32_t)0x00000800)          /*!< Enable start fault counter for HFXT2 */
/* CS_STAT[HFXT2_ON] Bits */
#define CS_STAT_HFXT2_ON_OFS                     ( 3)                            /*!< HFXT2_ON Bit Offset */
#define CS_STAT_HFXT2_ON                         ((uint32_t)0x00000008)          /*!< HFXT2 status */
/* CS_IE[HFXT2IE] Bits */
#define CS_IE_HFXT2IE_OFS                        ( 2)                            /*!< HFXT2IE Bit Offset */
#define CS_IE_HFXT2IE                            ((uint32_t)0x00000004)          /*!< HFXT2 oscillator fault flag interrupt enable */
/* CS_IE[FCNTHF2IE] Bits */
#define CS_IE_FCNTHF2IE_OFS                      (10)                            /*!< FCNTHF2IE Bit Offset */
#define CS_IE_FCNTHF2IE                          ((uint32_t)0x00000400)          /*!< Start fault counter interrupt enable HFXT2 */
/* CS_IE[PLLOOLIE] Bits */
#define CS_IE_PLLOOLIE_OFS                       (12)                            /*!< PLLOOLIE Bit Offset */
#define CS_IE_PLLOOLIE                           ((uint32_t)0x00001000)          /*!< PLL out-of-lock interrupt enable */
/* CS_IE[PLLLOSIE] Bits */
#define CS_IE_PLLLOSIE_OFS                       (13)                            /*!< PLLLOSIE Bit Offset */
#define CS_IE_PLLLOSIE                           ((uint32_t)0x00002000)          /*!< PLL loss-of-signal interrupt enable */
/* CS_IE[PLLOORIE] Bits */
#define CS_IE_PLLOORIE_OFS                       (14)                            /*!< PLLOORIE Bit Offset */
#define CS_IE_PLLOORIE                           ((uint32_t)0x00004000)          /*!< PLL out-of-range interrupt enable */
/* CS_IE[CALIE] Bits */
#define CS_IE_CALIE_OFS                          (15)                            /*!< CALIE Bit Offset */
#define CS_IE_CALIE                              ((uint32_t)0x00008000)          /*!< REFCNT period counter interrupt enable */
/* CS_IFG[HFXT2IFG] Bits */
#define CS_IFG_HFXT2IFG_OFS                      ( 2)                            /*!< HFXT2IFG Bit Offset */
#define CS_IFG_HFXT2IFG                          ((uint32_t)0x00000004)          /*!< HFXT2 oscillator fault flag */
/* CS_IFG[FCNTHF2IFG] Bits */
#define CS_IFG_FCNTHF2IFG_OFS                    (11)                            /*!< FCNTHF2IFG Bit Offset */
#define CS_IFG_FCNTHF2IFG                        ((uint32_t)0x00000800)          /*!< Start fault counter interrupt flag HFXT2 */
/* CS_IFG[PLLOOLIFG] Bits */
#define CS_IFG_PLLOOLIFG_OFS                     (12)                            /*!< PLLOOLIFG Bit Offset */
#define CS_IFG_PLLOOLIFG                         ((uint32_t)0x00001000)          /*!< PLL out-of-lock interrupt flag */
/* CS_IFG[PLLLOSIFG] Bits */
#define CS_IFG_PLLLOSIFG_OFS                     (13)                            /*!< PLLLOSIFG Bit Offset */
#define CS_IFG_PLLLOSIFG                         ((uint32_t)0x00002000)          /*!< PLL loss-of-signal interrupt flag */
/* CS_IFG[PLLOORIFG] Bits */
#define CS_IFG_PLLOORIFG_OFS                     (14)                            /*!< PLLOORIFG Bit Offset */
#define CS_IFG_PLLOORIFG                         ((uint32_t)0x00004000)          /*!< PLL out-of-range interrupt flag */
/* CS_IFG[CALIFG] Bits */
#define CS_IFG_CALIFG_OFS                        (15)                            /*!< CALIFG Bit Offset */
#define CS_IFG_CALIFG                            ((uint32_t)0x00008000)          /*!< REFCNT period counter expired */
/* CS_CLRIFG[CLR_HFXT2IFG] Bits */
#define CS_CLRIFG_CLR_HFXT2IFG_OFS               ( 2)                            /*!< CLR_HFXT2IFG Bit Offset */
#define CS_CLRIFG_CLR_HFXT2IFG                   ((uint32_t)0x00000004)          /*!< Clear HFXT2 oscillator fault interrupt flag */
/* CS_CLRIFG[CLR_CALIFG] Bits */
#define CS_CLRIFG_CLR_CALIFG_OFS                 (15)                            /*!< CLR_CALIFG Bit Offset */
#define CS_CLRIFG_CLR_CALIFG                     ((uint32_t)0x00008000)          /*!< REFCNT period counter clear interrupt flag */
/* CS_CLRIFG[CLR_FCNTHF2IFG] Bits */
#define CS_CLRIFG_CLR_FCNTHF2IFG_OFS             (10)                            /*!< CLR_FCNTHF2IFG Bit Offset */
#define CS_CLRIFG_CLR_FCNTHF2IFG                 ((uint32_t)0x00000400)          /*!< Start fault counter clear interrupt flag HFXT2 */
/* CS_CLRIFG[CLR_PLLOOLIFG] Bits */
#define CS_CLRIFG_CLR_PLLOOLIFG_OFS              (12)                            /*!< CLR_PLLOOLIFG Bit Offset */
#define CS_CLRIFG_CLR_PLLOOLIFG                  ((uint32_t)0x00001000)          /*!< PLL out-of-lock clear interrupt flag */
/* CS_CLRIFG[CLR_PLLLOSIFG] Bits */
#define CS_CLRIFG_CLR_PLLLOSIFG_OFS              (13)                            /*!< CLR_PLLLOSIFG Bit Offset */
#define CS_CLRIFG_CLR_PLLLOSIFG                  ((uint32_t)0x00002000)          /*!< PLL loss-of-signal clear interrupt flag */
/* CS_CLRIFG[CLR_PLLOORIFG] Bits */
#define CS_CLRIFG_CLR_PLLOORIFG_OFS              (14)                            /*!< CLR_PLLOORIFG Bit Offset */
#define CS_CLRIFG_CLR_PLLOORIFG                  ((uint32_t)0x00004000)          /*!< PLL out-of-range clear interrupt flag */
/* CS_SETIFG[SET_HFXT2IFG] Bits */
#define CS_SETIFG_SET_HFXT2IFG_OFS               ( 2)                            /*!< SET_HFXT2IFG Bit Offset */
#define CS_SETIFG_SET_HFXT2IFG                   ((uint32_t)0x00000004)          /*!< Set HFXT2 oscillator fault interrupt flag */
/* CS_SETIFG[SET_CALIFG] Bits */
#define CS_SETIFG_SET_CALIFG_OFS                 (15)                            /*!< SET_CALIFG Bit Offset */
#define CS_SETIFG_SET_CALIFG                     ((uint32_t)0x00008000)          /*!< REFCNT period counter set interrupt flag */
/* CS_SETIFG[SET_FCNTHF2IFG] Bits */
#define CS_SETIFG_SET_FCNTHF2IFG_OFS             (10)                            /*!< SET_FCNTHF2IFG Bit Offset */
#define CS_SETIFG_SET_FCNTHF2IFG                 ((uint32_t)0x00000400)          /*!< Start fault counter set interrupt flag HFXT2 */
/* CS_SETIFG[SET_PLLOOLIFG] Bits */
#define CS_SETIFG_SET_PLLOOLIFG_OFS              (12)                            /*!< SET_PLLOOLIFG Bit Offset */
#define CS_SETIFG_SET_PLLOOLIFG                  ((uint32_t)0x00001000)          /*!< PLL out-of-lock set interrupt flag */
/* CS_SETIFG[SET_PLLLOSIFG] Bits */
#define CS_SETIFG_SET_PLLLOSIFG_OFS              (13)                            /*!< SET_PLLLOSIFG Bit Offset */
#define CS_SETIFG_SET_PLLLOSIFG                  ((uint32_t)0x00002000)          /*!< PLL loss-of-signal set interrupt flag */
/* CS_SETIFG[SET_PLLOORIFG] Bits */
#define CS_SETIFG_SET_PLLOORIFG_OFS              (14)                            /*!< SET_PLLOORIFG Bit Offset */
#define CS_SETIFG_SET_PLLOORIFG                  ((uint32_t)0x00004000)          /*!< PLL out-of-range set interrupt flag */

/* EUSCI_x_CTLW0[SSEL] Bits */
#define EUSCI_A_CTLW0_SSEL_0                     ((uint16_t)0x0000)              /*!< Reserved */
#define EUSCI_B_CTLW0_SSEL_0                     ((uint16_t)0x0000)              /*!< Reserved */
#define EUSCI_B_CTLW0_SSEL_3                     ((uint16_t)0x00C0)              /*!< SMCLK */

/* RSTCTL_PSSRESET_STAT[SVSL] Bits */
#define RSTCTL_PSSRESET_STAT_SVSL_OFS            ( 0)                            /*!< SVSL Bit Offset */
#define RSTCTL_PSSRESET_STAT_SVSL                ((uint32_t)0x00000001)          /*!< Indicates if POR was caused by an SVSL trip condition in the PSS */

/* SYSCTL_SYSTEM_STAT[DBG_SEC_ACT] Bits */
#define SYSCTL_SYSTEM_STAT_DBG_SEC_ACT_OFS       ( 3)                            /*!< DBG_SEC_ACT Bit Offset */
#define SYSCTL_SYSTEM_STAT_DBG_SEC_ACT           ((uint32_t)0x00000008)          /*!< Debug Security active */
/* SYSCTL_SYSTEM_STAT[JTAG_SWD_LOCK_ACT] Bits */
#define SYSCTL_SYSTEM_STAT_JTAG_SWD_LOCK_ACT_OFS ( 4)                            /*!< JTAG_SWD_LOCK_ACT Bit Offset */
#define SYSCTL_SYSTEM_STAT_JTAG_SWD_LOCK_ACT     ((uint32_t)0x00000010)          /*!< Indicates if JTAG and SWD Lock is active */
/* SYSCTL_SYSTEM_STAT[IP_PROT_ACT] Bits */
#define SYSCTL_SYSTEM_STAT_IP_PROT_ACT_OFS       ( 5)                            /*!< IP_PROT_ACT Bit Offset */
#define SYSCTL_SYSTEM_STAT_IP_PROT_ACT           ((uint32_t)0x00000020)          /*!< Indicates if IP protection is active */

