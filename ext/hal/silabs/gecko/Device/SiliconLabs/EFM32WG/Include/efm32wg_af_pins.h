/**************************************************************************//**
 * @file efm32wg_af_pins.h
 * @brief EFM32WG_AF_PINS register and bit field definitions
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/
/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @defgroup EFM32WG_AF_Pins
 * @{
 *****************************************************************************/

/** AF pin number for location number i */
#define AF_USB_VBUSEN_PIN(i)        ((i) == 0 ? 5 :  -1)
#define AF_USB_DMPU_PIN(i)          ((i) == 0 ? 2 :  -1)
#define AF_CMU_CLK0_PIN(i)          ((i) == 0 ? 2 : (i) == 1 ? 12 : (i) == 2 ? 7 :  -1)
#define AF_CMU_CLK1_PIN(i)          ((i) == 0 ? 1 : (i) == 1 ? 8 : (i) == 2 ? 12 :  -1)
#define AF_LESENSE_CH0_PIN(i)       ((i) == 0 ? 0 :  -1)
#define AF_LESENSE_CH1_PIN(i)       ((i) == 0 ? 1 :  -1)
#define AF_LESENSE_CH2_PIN(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH3_PIN(i)       ((i) == 0 ? 3 :  -1)
#define AF_LESENSE_CH4_PIN(i)       ((i) == 0 ? 4 :  -1)
#define AF_LESENSE_CH5_PIN(i)       ((i) == 0 ? 5 :  -1)
#define AF_LESENSE_CH6_PIN(i)       ((i) == 0 ? 6 :  -1)
#define AF_LESENSE_CH7_PIN(i)       ((i) == 0 ? 7 :  -1)
#define AF_LESENSE_CH8_PIN(i)       ((i) == 0 ? 8 :  -1)
#define AF_LESENSE_CH9_PIN(i)       ((i) == 0 ? 9 :  -1)
#define AF_LESENSE_CH10_PIN(i)      ((i) == 0 ? 10 :  -1)
#define AF_LESENSE_CH11_PIN(i)      ((i) == 0 ? 11 :  -1)
#define AF_LESENSE_CH12_PIN(i)      ((i) == 0 ? 12 :  -1)
#define AF_LESENSE_CH13_PIN(i)      ((i) == 0 ? 13 :  -1)
#define AF_LESENSE_CH14_PIN(i)      ((i) == 0 ? 14 :  -1)
#define AF_LESENSE_CH15_PIN(i)      ((i) == 0 ? 15 :  -1)
#define AF_LESENSE_ALTEX0_PIN(i)    ((i) == 0 ? 6 :  -1)
#define AF_LESENSE_ALTEX1_PIN(i)    ((i) == 0 ? 7 :  -1)
#define AF_LESENSE_ALTEX2_PIN(i)    ((i) == 0 ? 3 :  -1)
#define AF_LESENSE_ALTEX3_PIN(i)    ((i) == 0 ? 4 :  -1)
#define AF_LESENSE_ALTEX4_PIN(i)    ((i) == 0 ? 5 :  -1)
#define AF_LESENSE_ALTEX5_PIN(i)    ((i) == 0 ? 11 :  -1)
#define AF_LESENSE_ALTEX6_PIN(i)    ((i) == 0 ? 12 :  -1)
#define AF_LESENSE_ALTEX7_PIN(i)    ((i) == 0 ? 13 :  -1)
#define AF_EBI_AD00_PIN(i)          ((i) == 0 ? 8 : (i) == 1 ? 8 : (i) == 2 ? 8 :  -1)
#define AF_EBI_AD01_PIN(i)          ((i) == 0 ? 9 : (i) == 1 ? 9 : (i) == 2 ? 9 :  -1)
#define AF_EBI_AD02_PIN(i)          ((i) == 0 ? 10 : (i) == 1 ? 10 : (i) == 2 ? 10 :  -1)
#define AF_EBI_AD03_PIN(i)          ((i) == 0 ? 11 : (i) == 1 ? 11 : (i) == 2 ? 11 :  -1)
#define AF_EBI_AD04_PIN(i)          ((i) == 0 ? 12 : (i) == 1 ? 12 : (i) == 2 ? 12 :  -1)
#define AF_EBI_AD05_PIN(i)          ((i) == 0 ? 13 : (i) == 1 ? 13 : (i) == 2 ? 13 :  -1)
#define AF_EBI_AD06_PIN(i)          ((i) == 0 ? 14 : (i) == 1 ? 14 : (i) == 2 ? 14 :  -1)
#define AF_EBI_AD07_PIN(i)          ((i) == 0 ? 15 : (i) == 1 ? 15 : (i) == 2 ? 15 :  -1)
#define AF_EBI_AD08_PIN(i)          ((i) == 0 ? 15 : (i) == 1 ? 15 : (i) == 2 ? 15 :  -1)
#define AF_EBI_AD09_PIN(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD10_PIN(i)          ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_AD11_PIN(i)          ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_AD12_PIN(i)          ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_AD13_PIN(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD14_PIN(i)          ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_AD15_PIN(i)          ((i) == 0 ? 6 : (i) == 1 ? 6 : (i) == 2 ? 6 :  -1)
#define AF_EBI_CS0_PIN(i)           ((i) == 0 ? 9 : (i) == 1 ? 9 : (i) == 2 ? 9 :  -1)
#define AF_EBI_CS1_PIN(i)           ((i) == 0 ? 10 : (i) == 1 ? 10 : (i) == 2 ? 10 :  -1)
#define AF_EBI_CS2_PIN(i)           ((i) == 0 ? 11 : (i) == 1 ? 11 : (i) == 2 ? 11 :  -1)
#define AF_EBI_CS3_PIN(i)           ((i) == 0 ? 12 : (i) == 1 ? 12 : (i) == 2 ? 12 :  -1)
#define AF_EBI_ARDY_PIN(i)          ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_ALE_PIN(i)           ((i) == 0 ? 3 : (i) == 1 ? 11 : (i) == 2 ? 11 :  -1)
#define AF_EBI_WEn_PIN(i)           ((i) == 0 ? 4 : (i) == 1 ? 8 : (i) == 2 ? 4 :  -1)
#define AF_EBI_REn_PIN(i)           ((i) == 0 ? 5 : (i) == 1 ? 9 : (i) == 2 ? 5 :  -1)
#define AF_EBI_NANDWEn_PIN(i)       ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_NANDREn_PIN(i)       ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_BL0_PIN(i)           ((i) == 0 ? 6 : (i) == 1 ? 6 : (i) == 2 ? 6 :  -1)
#define AF_EBI_BL1_PIN(i)           ((i) == 0 ? 7 : (i) == 1 ? 7 : (i) == 2 ? 7 :  -1)
#define AF_EBI_A00_PIN(i)           ((i) == 0 ? 12 : (i) == 1 ? 12 : (i) == 2 ? 12 :  -1)
#define AF_EBI_A01_PIN(i)           ((i) == 0 ? 13 : (i) == 1 ? 13 : (i) == 2 ? 13 :  -1)
#define AF_EBI_A02_PIN(i)           ((i) == 0 ? 14 : (i) == 1 ? 14 : (i) == 2 ? 14 :  -1)
#define AF_EBI_A03_PIN(i)           ((i) == 0 ? 9 : (i) == 1 ? 9 : (i) == 2 ? 9 :  -1)
#define AF_EBI_A04_PIN(i)           ((i) == 0 ? 10 : (i) == 1 ? 10 : (i) == 2 ? 10 :  -1)
#define AF_EBI_A05_PIN(i)           ((i) == 0 ? 6 : (i) == 1 ? 6 : (i) == 2 ? 6 :  -1)
#define AF_EBI_A06_PIN(i)           ((i) == 0 ? 7 : (i) == 1 ? 7 : (i) == 2 ? 7 :  -1)
#define AF_EBI_A07_PIN(i)           ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_A08_PIN(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A09_PIN(i)           ((i) == 0 ? 2 : (i) == 1 ? 9 : (i) == 2 ? 9 :  -1)
#define AF_EBI_A10_PIN(i)           ((i) == 0 ? 3 : (i) == 1 ? 10 : (i) == 2 ? 10 :  -1)
#define AF_EBI_A11_PIN(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A12_PIN(i)           ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_A13_PIN(i)           ((i) == 0 ? 6 : (i) == 1 ? 6 : (i) == 2 ? 6 :  -1)
#define AF_EBI_A14_PIN(i)           ((i) == 0 ? 7 : (i) == 1 ? 7 : (i) == 2 ? 7 :  -1)
#define AF_EBI_A15_PIN(i)           ((i) == 0 ? 8 : (i) == 1 ? 8 : (i) == 2 ? 8 :  -1)
#define AF_EBI_A16_PIN(i)           ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_A17_PIN(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A18_PIN(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A19_PIN(i)           ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_A20_PIN(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A21_PIN(i)           ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_A22_PIN(i)           ((i) == 0 ? 6 : (i) == 1 ? 6 : (i) == 2 ? 6 :  -1)
#define AF_EBI_A23_PIN(i)           ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_A24_PIN(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A25_PIN(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A26_PIN(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A27_PIN(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_CSTFT_PIN(i)         ((i) == 0 ? 7 : (i) == 1 ? 7 : (i) == 2 ? 7 :  -1)
#define AF_EBI_DCLK_PIN(i)          ((i) == 0 ? 8 : (i) == 1 ? 8 : (i) == 2 ? 8 :  -1)
#define AF_EBI_DTEN_PIN(i)          ((i) == 0 ? 9 : (i) == 1 ? 9 : (i) == 2 ? 9 :  -1)
#define AF_EBI_VSNC_PIN(i)          ((i) == 0 ? 10 : (i) == 1 ? 10 : (i) == 2 ? 10 :  -1)
#define AF_EBI_HSNC_PIN(i)          ((i) == 0 ? 11 : (i) == 1 ? 11 : (i) == 2 ? 11 :  -1)
#define AF_USART0_TX_PIN(i)         ((i) == 0 ? 10 : (i) == 1 ? 7 : (i) == 2 ? 11 : (i) == 3 ? 13 : (i) == 4 ? 7 : (i) == 5 ? 0 :  -1)
#define AF_USART0_RX_PIN(i)         ((i) == 0 ? 11 : (i) == 1 ? 6 : (i) == 2 ? 10 : (i) == 3 ? 12 : (i) == 4 ? 8 : (i) == 5 ? 1 :  -1)
#define AF_USART0_CLK_PIN(i)        ((i) == 0 ? 12 : (i) == 1 ? 5 : (i) == 2 ? 9 : (i) == 3 ? 15 : (i) == 4 ? 13 : (i) == 5 ? 13 :  -1)
#define AF_USART0_CS_PIN(i)         ((i) == 0 ? 13 : (i) == 1 ? 4 : (i) == 2 ? 8 : (i) == 3 ? 14 : (i) == 4 ? 14 : (i) == 5 ? 14 :  -1)
#define AF_USART1_TX_PIN(i)         ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 7 :  -1)
#define AF_USART1_RX_PIN(i)         ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 6 :  -1)
#define AF_USART1_CLK_PIN(i)        ((i) == 0 ? 7 : (i) == 1 ? 2 : (i) == 2 ? 0 :  -1)
#define AF_USART1_CS_PIN(i)         ((i) == 0 ? 8 : (i) == 1 ? 3 : (i) == 2 ? 1 :  -1)
#define AF_USART2_TX_PIN(i)         ((i) == 0 ? 2 : (i) == 1 ? 3 :  -1)
#define AF_USART2_RX_PIN(i)         ((i) == 0 ? 3 : (i) == 1 ? 4 :  -1)
#define AF_USART2_CLK_PIN(i)        ((i) == 0 ? 4 : (i) == 1 ? 5 :  -1)
#define AF_USART2_CS_PIN(i)         ((i) == 0 ? 5 : (i) == 1 ? 6 :  -1)
#define AF_UART0_TX_PIN(i)          ((i) == 0 ? 6 : (i) == 1 ? 0 : (i) == 2 ? 3 : (i) == 3 ? 14 :  -1)
#define AF_UART0_RX_PIN(i)          ((i) == 0 ? 7 : (i) == 1 ? 1 : (i) == 2 ? 4 : (i) == 3 ? 15 :  -1)
#define AF_UART0_CLK_PIN(i)         (-1)
#define AF_UART0_CS_PIN(i)          (-1)
#define AF_UART1_TX_PIN(i)          ((i) == 0 ? 12 : (i) == 1 ? 10 : (i) == 2 ? 9 : (i) == 3 ? 2 :  -1)
#define AF_UART1_RX_PIN(i)          ((i) == 0 ? 13 : (i) == 1 ? 11 : (i) == 2 ? 10 : (i) == 3 ? 3 :  -1)
#define AF_UART1_CLK_PIN(i)         (-1)
#define AF_UART1_CS_PIN(i)          (-1)
#define AF_TIMER0_CC0_PIN(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 6 : (i) == 3 ? 1 : (i) == 4 ? 0 : (i) == 5 ? 0 :  -1)
#define AF_TIMER0_CC1_PIN(i)        ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 7 : (i) == 3 ? 2 : (i) == 4 ? 0 : (i) == 5 ? 1 :  -1)
#define AF_TIMER0_CC2_PIN(i)        ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 8 : (i) == 3 ? 3 : (i) == 4 ? 1 : (i) == 5 ? 2 :  -1)
#define AF_TIMER0_CDTI0_PIN(i)      ((i) == 0 ? 3 : (i) == 1 ? 13 : (i) == 2 ? 3 : (i) == 3 ? 13 : (i) == 4 ? 2 : (i) == 5 ? 3 :  -1)
#define AF_TIMER0_CDTI1_PIN(i)      ((i) == 0 ? 4 : (i) == 1 ? 14 : (i) == 2 ? 4 : (i) == 3 ? 14 : (i) == 4 ? 3 : (i) == 5 ? 4 :  -1)
#define AF_TIMER0_CDTI2_PIN(i)      ((i) == 0 ? 5 : (i) == 1 ? 15 : (i) == 2 ? 5 : (i) == 3 ? 15 : (i) == 4 ? 4 : (i) == 5 ? 5 :  -1)
#define AF_TIMER1_CC0_PIN(i)        ((i) == 0 ? 13 : (i) == 1 ? 10 : (i) == 2 ? 0 : (i) == 3 ? 7 : (i) == 4 ? 6 :  -1)
#define AF_TIMER1_CC1_PIN(i)        ((i) == 0 ? 14 : (i) == 1 ? 11 : (i) == 2 ? 1 : (i) == 3 ? 8 : (i) == 4 ? 7 :  -1)
#define AF_TIMER1_CC2_PIN(i)        ((i) == 0 ? 15 : (i) == 1 ? 12 : (i) == 2 ? 2 : (i) == 3 ? 11 : (i) == 4 ? 13 :  -1)
#define AF_TIMER1_CDTI0_PIN(i)      (-1)
#define AF_TIMER1_CDTI1_PIN(i)      (-1)
#define AF_TIMER1_CDTI2_PIN(i)      (-1)
#define AF_TIMER2_CC0_PIN(i)        ((i) == 0 ? 8 : (i) == 1 ? 12 : (i) == 2 ? 8 :  -1)
#define AF_TIMER2_CC1_PIN(i)        ((i) == 0 ? 9 : (i) == 1 ? 13 : (i) == 2 ? 9 :  -1)
#define AF_TIMER2_CC2_PIN(i)        ((i) == 0 ? 10 : (i) == 1 ? 14 : (i) == 2 ? 10 :  -1)
#define AF_TIMER2_CDTI0_PIN(i)      (-1)
#define AF_TIMER2_CDTI1_PIN(i)      (-1)
#define AF_TIMER2_CDTI2_PIN(i)      (-1)
#define AF_TIMER3_CC0_PIN(i)        ((i) == 0 ? 14 : (i) == 1 ? 0 :  -1)
#define AF_TIMER3_CC1_PIN(i)        ((i) == 0 ? 15 : (i) == 1 ? 1 :  -1)
#define AF_TIMER3_CC2_PIN(i)        ((i) == 0 ? 15 : (i) == 1 ? 2 :  -1)
#define AF_TIMER3_CDTI0_PIN(i)      (-1)
#define AF_TIMER3_CDTI1_PIN(i)      (-1)
#define AF_TIMER3_CDTI2_PIN(i)      (-1)
#define AF_ACMP0_OUT_PIN(i)         ((i) == 0 ? 13 : (i) == 1 ? 2 : (i) == 2 ? 6 :  -1)
#define AF_ACMP1_OUT_PIN(i)         ((i) == 0 ? 2 : (i) == 1 ? 3 : (i) == 2 ? 7 :  -1)
#define AF_LEUART0_TX_PIN(i)        ((i) == 0 ? 4 : (i) == 1 ? 13 : (i) == 2 ? 14 : (i) == 3 ? 0 : (i) == 4 ? 2 :  -1)
#define AF_LEUART0_RX_PIN(i)        ((i) == 0 ? 5 : (i) == 1 ? 14 : (i) == 2 ? 15 : (i) == 3 ? 1 : (i) == 4 ? 0 :  -1)
#define AF_LEUART1_TX_PIN(i)        ((i) == 0 ? 6 : (i) == 1 ? 5 :  -1)
#define AF_LEUART1_RX_PIN(i)        ((i) == 0 ? 7 : (i) == 1 ? 6 :  -1)
#define AF_LETIMER0_OUT0_PIN(i)     ((i) == 0 ? 6 : (i) == 1 ? 11 : (i) == 2 ? 0 : (i) == 3 ? 4 :  -1)
#define AF_LETIMER0_OUT1_PIN(i)     ((i) == 0 ? 7 : (i) == 1 ? 12 : (i) == 2 ? 1 : (i) == 3 ? 5 :  -1)
#define AF_PCNT0_S0IN_PIN(i)        ((i) == 0 ? 13 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 6 :  -1)
#define AF_PCNT0_S1IN_PIN(i)        ((i) == 0 ? 14 : (i) == 1 ? 1 : (i) == 2 ? 1 : (i) == 3 ? 7 :  -1)
#define AF_PCNT1_S0IN_PIN(i)        ((i) == 0 ? 4 : (i) == 1 ? 3 :  -1)
#define AF_PCNT1_S1IN_PIN(i)        ((i) == 0 ? 5 : (i) == 1 ? 4 :  -1)
#define AF_PCNT2_S0IN_PIN(i)        ((i) == 0 ? 0 : (i) == 1 ? 8 :  -1)
#define AF_PCNT2_S1IN_PIN(i)        ((i) == 0 ? 1 : (i) == 1 ? 9 :  -1)
#define AF_I2C0_SDA_PIN(i)          ((i) == 0 ? 0 : (i) == 1 ? 6 : (i) == 2 ? 6 : (i) == 3 ? 14 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 12 :  -1)
#define AF_I2C0_SCL_PIN(i)          ((i) == 0 ? 1 : (i) == 1 ? 7 : (i) == 2 ? 7 : (i) == 3 ? 15 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 13 :  -1)
#define AF_I2C1_SDA_PIN(i)          ((i) == 0 ? 4 : (i) == 1 ? 11 : (i) == 2 ? 0 :  -1)
#define AF_I2C1_SCL_PIN(i)          ((i) == 0 ? 5 : (i) == 1 ? 12 : (i) == 2 ? 1 :  -1)
#define AF_PRS_CH0_PIN(i)           ((i) == 0 ? 0 : (i) == 1 ? 3 :  -1)
#define AF_PRS_CH1_PIN(i)           ((i) == 0 ? 1 : (i) == 1 ? 4 :  -1)
#define AF_PRS_CH2_PIN(i)           ((i) == 0 ? 0 : (i) == 1 ? 5 :  -1)
#define AF_PRS_CH3_PIN(i)           ((i) == 0 ? 1 : (i) == 1 ? 8 :  -1)
#define AF_DBG_SWO_PIN(i)           ((i) == 0 ? 2 : (i) == 1 ? 15 : (i) == 2 ? 1 : (i) == 3 ? 2 :  -1)
#define AF_DBG_SWDIO_PIN(i)         ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 : (i) == 3 ? 1 :  -1)
#define AF_DBG_SWCLK_PIN(i)         ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 :  -1)
#define AF_ETM_TCLK_PIN(i)          ((i) == 0 ? 7 : (i) == 1 ? 8 : (i) == 2 ? 6 : (i) == 3 ? 6 :  -1)
#define AF_ETM_TD0_PIN(i)           ((i) == 0 ? 6 : (i) == 1 ? 9 : (i) == 2 ? 7 : (i) == 3 ? 2 :  -1)
#define AF_ETM_TD1_PIN(i)           ((i) == 0 ? 3 : (i) == 1 ? 13 : (i) == 2 ? 3 : (i) == 3 ? 3 :  -1)
#define AF_ETM_TD2_PIN(i)           ((i) == 0 ? 4 : (i) == 1 ? 15 : (i) == 2 ? 4 : (i) == 3 ? 4 :  -1)
#define AF_ETM_TD3_PIN(i)           ((i) == 0 ? 5 : (i) == 1 ? 3 : (i) == 2 ? 5 : (i) == 3 ? 5 :  -1)

/** @} End of group EFM32WG_AF_Pins */
/** @} End of group Parts */

