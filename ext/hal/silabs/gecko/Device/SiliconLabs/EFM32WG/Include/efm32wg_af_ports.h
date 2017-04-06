/**************************************************************************//**
 * @file efm32wg_af_ports.h
 * @brief EFM32WG_AF_PORTS register and bit field definitions
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
 * @defgroup EFM32WG_AF_Ports
 * @{
 *****************************************************************************/

/** AF port number for location number i */
#define AF_USB_VBUSEN_PORT(i)        ((i) == 0 ? 5 :  -1)
#define AF_USB_DMPU_PORT(i)          ((i) == 0 ? 3 :  -1)
#define AF_CMU_CLK0_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 2 : (i) == 2 ? 3 :  -1)
#define AF_CMU_CLK1_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 3 : (i) == 2 ? 4 :  -1)
#define AF_LESENSE_CH0_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH1_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH2_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH3_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH4_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH5_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH6_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH7_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH8_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH9_PORT(i)       ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH10_PORT(i)      ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH11_PORT(i)      ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH12_PORT(i)      ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH13_PORT(i)      ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH14_PORT(i)      ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_CH15_PORT(i)      ((i) == 0 ? 2 :  -1)
#define AF_LESENSE_ALTEX0_PORT(i)    ((i) == 0 ? 3 :  -1)
#define AF_LESENSE_ALTEX1_PORT(i)    ((i) == 0 ? 3 :  -1)
#define AF_LESENSE_ALTEX2_PORT(i)    ((i) == 0 ? 0 :  -1)
#define AF_LESENSE_ALTEX3_PORT(i)    ((i) == 0 ? 0 :  -1)
#define AF_LESENSE_ALTEX4_PORT(i)    ((i) == 0 ? 0 :  -1)
#define AF_LESENSE_ALTEX5_PORT(i)    ((i) == 0 ? 4 :  -1)
#define AF_LESENSE_ALTEX6_PORT(i)    ((i) == 0 ? 4 :  -1)
#define AF_LESENSE_ALTEX7_PORT(i)    ((i) == 0 ? 4 :  -1)
#define AF_EBI_AD00_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD01_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD02_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD03_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD04_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD05_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD06_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD07_PORT(i)          ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_AD08_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD09_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD10_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD11_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD12_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD13_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD14_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_AD15_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_CS0_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_CS1_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_CS2_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_CS3_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_ARDY_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_ALE_PORT(i)           ((i) == 0 ? 5 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_WEn_PORT(i)           ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_REn_PORT(i)           ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_NANDWEn_PORT(i)       ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_NANDREn_PORT(i)       ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_BL0_PORT(i)           ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_BL1_PORT(i)           ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 :  -1)
#define AF_EBI_A00_PORT(i)           ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_A01_PORT(i)           ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_A02_PORT(i)           ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_A03_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A04_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A05_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A06_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A07_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A08_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A09_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A10_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A11_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A12_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A13_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A14_PORT(i)           ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 4 :  -1)
#define AF_EBI_A15_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A16_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A17_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A18_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A19_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A20_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A21_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A22_PORT(i)           ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? 1 :  -1)
#define AF_EBI_A23_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A24_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A25_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A26_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 :  -1)
#define AF_EBI_A27_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_EBI_CSTFT_PORT(i)         ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_DCLK_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_DTEN_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_VSNC_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_EBI_HSNC_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 :  -1)
#define AF_USART0_TX_PORT(i)         ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 2 : (i) == 3 ? 4 : (i) == 4 ? 1 : (i) == 5 ? 2 :  -1)
#define AF_USART0_RX_PORT(i)         ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 2 : (i) == 3 ? 4 : (i) == 4 ? 1 : (i) == 5 ? 2 :  -1)
#define AF_USART0_CLK_PORT(i)        ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 2 : (i) == 3 ? 2 : (i) == 4 ? 1 : (i) == 5 ? 1 :  -1)
#define AF_USART0_CS_PORT(i)         ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 2 : (i) == 3 ? 2 : (i) == 4 ? 1 : (i) == 5 ? 1 :  -1)
#define AF_USART1_TX_PORT(i)         ((i) == 0 ? 2 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_USART1_RX_PORT(i)         ((i) == 0 ? 2 : (i) == 1 ? 3 : (i) == 2 ? 3 :  -1)
#define AF_USART1_CLK_PORT(i)        ((i) == 0 ? 1 : (i) == 1 ? 3 : (i) == 2 ? 5 :  -1)
#define AF_USART1_CS_PORT(i)         ((i) == 0 ? 1 : (i) == 1 ? 3 : (i) == 2 ? 5 :  -1)
#define AF_USART2_TX_PORT(i)         ((i) == 0 ? 2 : (i) == 1 ? 1 :  -1)
#define AF_USART2_RX_PORT(i)         ((i) == 0 ? 2 : (i) == 1 ? 1 :  -1)
#define AF_USART2_CLK_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 1 :  -1)
#define AF_USART2_CS_PORT(i)         ((i) == 0 ? 2 : (i) == 1 ? 1 :  -1)
#define AF_UART0_TX_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 4 : (i) == 2 ? 0 : (i) == 3 ? 2 :  -1)
#define AF_UART0_RX_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 4 : (i) == 2 ? 0 : (i) == 3 ? 2 :  -1)
#define AF_UART0_CLK_PORT(i)         (-1)
#define AF_UART0_CS_PORT(i)          (-1)
#define AF_UART1_TX_PORT(i)          ((i) == 0 ? 2 : (i) == 1 ? 5 : (i) == 2 ? 1 : (i) == 3 ? 4 :  -1)
#define AF_UART1_RX_PORT(i)          ((i) == 0 ? 2 : (i) == 1 ? 5 : (i) == 2 ? 1 : (i) == 3 ? 4 :  -1)
#define AF_UART1_CLK_PORT(i)         (-1)
#define AF_UART1_CS_PORT(i)          (-1)
#define AF_TIMER0_CC0_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 5 : (i) == 3 ? 3 : (i) == 4 ? 0 : (i) == 5 ? 5 :  -1)
#define AF_TIMER0_CC1_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 5 : (i) == 3 ? 3 : (i) == 4 ? 2 : (i) == 5 ? 5 :  -1)
#define AF_TIMER0_CC2_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 5 : (i) == 3 ? 3 : (i) == 4 ? 2 : (i) == 5 ? 5 :  -1)
#define AF_TIMER0_CDTI0_PORT(i)      ((i) == 0 ? 0 : (i) == 1 ? 2 : (i) == 2 ? 5 : (i) == 3 ? 2 : (i) == 4 ? 2 : (i) == 5 ? 5 :  -1)
#define AF_TIMER0_CDTI1_PORT(i)      ((i) == 0 ? 0 : (i) == 1 ? 2 : (i) == 2 ? 5 : (i) == 3 ? 2 : (i) == 4 ? 2 : (i) == 5 ? 5 :  -1)
#define AF_TIMER0_CDTI2_PORT(i)      ((i) == 0 ? 0 : (i) == 1 ? 2 : (i) == 2 ? 5 : (i) == 3 ? 2 : (i) == 4 ? 2 : (i) == 5 ? 5 :  -1)
#define AF_TIMER1_CC0_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 4 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 3 :  -1)
#define AF_TIMER1_CC1_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 4 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 3 :  -1)
#define AF_TIMER1_CC2_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 4 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 2 :  -1)
#define AF_TIMER1_CDTI0_PORT(i)      (-1)
#define AF_TIMER1_CDTI1_PORT(i)      (-1)
#define AF_TIMER1_CDTI2_PORT(i)      (-1)
#define AF_TIMER2_CC0_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 2 :  -1)
#define AF_TIMER2_CC1_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 2 :  -1)
#define AF_TIMER2_CC2_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 2 :  -1)
#define AF_TIMER2_CDTI0_PORT(i)      (-1)
#define AF_TIMER2_CDTI1_PORT(i)      (-1)
#define AF_TIMER2_CDTI2_PORT(i)      (-1)
#define AF_TIMER3_CC0_PORT(i)        ((i) == 0 ? 4 : (i) == 1 ? 4 :  -1)
#define AF_TIMER3_CC1_PORT(i)        ((i) == 0 ? 4 : (i) == 1 ? 4 :  -1)
#define AF_TIMER3_CC2_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 4 :  -1)
#define AF_TIMER3_CDTI0_PORT(i)      (-1)
#define AF_TIMER3_CDTI1_PORT(i)      (-1)
#define AF_TIMER3_CDTI2_PORT(i)      (-1)
#define AF_ACMP0_OUT_PORT(i)         ((i) == 0 ? 4 : (i) == 1 ? 4 : (i) == 2 ? 3 :  -1)
#define AF_ACMP1_OUT_PORT(i)         ((i) == 0 ? 5 : (i) == 1 ? 4 : (i) == 2 ? 3 :  -1)
#define AF_LEUART0_TX_PORT(i)        ((i) == 0 ? 3 : (i) == 1 ? 1 : (i) == 2 ? 4 : (i) == 3 ? 5 : (i) == 4 ? 5 :  -1)
#define AF_LEUART0_RX_PORT(i)        ((i) == 0 ? 3 : (i) == 1 ? 1 : (i) == 2 ? 4 : (i) == 3 ? 5 : (i) == 4 ? 0 :  -1)
#define AF_LEUART1_TX_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 0 :  -1)
#define AF_LEUART1_RX_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 0 :  -1)
#define AF_LETIMER0_OUT0_PORT(i)     ((i) == 0 ? 3 : (i) == 1 ? 1 : (i) == 2 ? 5 : (i) == 3 ? 2 :  -1)
#define AF_LETIMER0_OUT1_PORT(i)     ((i) == 0 ? 3 : (i) == 1 ? 1 : (i) == 2 ? 5 : (i) == 3 ? 2 :  -1)
#define AF_PCNT0_S0IN_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 4 : (i) == 2 ? 2 : (i) == 3 ? 3 :  -1)
#define AF_PCNT0_S1IN_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 4 : (i) == 2 ? 2 : (i) == 3 ? 3 :  -1)
#define AF_PCNT1_S0IN_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 1 :  -1)
#define AF_PCNT1_S1IN_PORT(i)        ((i) == 0 ? 2 : (i) == 1 ? 1 :  -1)
#define AF_PCNT2_S0IN_PORT(i)        ((i) == 0 ? 3 : (i) == 1 ? 4 :  -1)
#define AF_PCNT2_S1IN_PORT(i)        ((i) == 0 ? 3 : (i) == 1 ? 4 :  -1)
#define AF_I2C0_SDA_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 3 : (i) == 2 ? 2 : (i) == 3 ? 3 : (i) == 4 ? 2 : (i) == 5 ? 5 : (i) == 6 ? 4 :  -1)
#define AF_I2C0_SCL_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 3 : (i) == 2 ? 2 : (i) == 3 ? 3 : (i) == 4 ? 2 : (i) == 5 ? 5 : (i) == 6 ? 4 :  -1)
#define AF_I2C1_SDA_PORT(i)          ((i) == 0 ? 2 : (i) == 1 ? 1 : (i) == 2 ? 4 :  -1)
#define AF_I2C1_SCL_PORT(i)          ((i) == 0 ? 2 : (i) == 1 ? 1 : (i) == 2 ? 4 :  -1)
#define AF_PRS_CH0_PORT(i)           ((i) == 0 ? 0 : (i) == 1 ? 5 :  -1)
#define AF_PRS_CH1_PORT(i)           ((i) == 0 ? 0 : (i) == 1 ? 5 :  -1)
#define AF_PRS_CH2_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 5 :  -1)
#define AF_PRS_CH3_PORT(i)           ((i) == 0 ? 2 : (i) == 1 ? 4 :  -1)
#define AF_DBG_SWO_PORT(i)           ((i) == 0 ? 5 : (i) == 1 ? 2 : (i) == 2 ? 3 : (i) == 3 ? 3 :  -1)
#define AF_DBG_SWDIO_PORT(i)         ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 : (i) == 3 ? 5 :  -1)
#define AF_DBG_SWCLK_PORT(i)         ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 : (i) == 3 ? 5 :  -1)
#define AF_ETM_TCLK_PORT(i)          ((i) == 0 ? 3 : (i) == 1 ? 5 : (i) == 2 ? 2 : (i) == 3 ? 0 :  -1)
#define AF_ETM_TD0_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 5 : (i) == 2 ? 2 : (i) == 3 ? 0 :  -1)
#define AF_ETM_TD1_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 : (i) == 3 ? 0 :  -1)
#define AF_ETM_TD2_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 1 : (i) == 2 ? 3 : (i) == 3 ? 0 :  -1)
#define AF_ETM_TD3_PORT(i)           ((i) == 0 ? 3 : (i) == 1 ? 5 : (i) == 2 ? 3 : (i) == 3 ? 0 :  -1)

/** @} End of group EFM32WG_AF_Ports */
/** @} End of group Parts */

