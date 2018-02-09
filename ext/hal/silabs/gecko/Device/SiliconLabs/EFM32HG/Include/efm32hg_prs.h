/**************************************************************************//**
 * @file efm32hg_prs.h
 * @brief EFM32HG_PRS register and bit field definitions
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
 * @defgroup EFM32HG_PRS
 * @{
 * @brief EFM32HG_PRS Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t SWPULSE;      /**< Software Pulse Register  */
  __IOM uint32_t SWLEVEL;      /**< Software Level Register  */
  __IOM uint32_t ROUTE;        /**< I/O Routing Register  */

  uint32_t       RESERVED0[1]; /**< Reserved registers */
  PRS_CH_TypeDef CH[6];        /**< Channel registers */

  uint32_t       RESERVED1[6]; /**< Reserved for future use **/
  __IOM uint32_t TRACECTRL;    /**< MTB Trace Control Register  */
} PRS_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_PRS_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for PRS SWPULSE */
#define _PRS_SWPULSE_RESETVALUE              0x00000000UL                         /**< Default value for PRS_SWPULSE */
#define _PRS_SWPULSE_MASK                    0x0000003FUL                         /**< Mask for PRS_SWPULSE */
#define PRS_SWPULSE_CH0PULSE                 (0x1UL << 0)                         /**< Channel 0 Pulse Generation */
#define _PRS_SWPULSE_CH0PULSE_SHIFT          0                                    /**< Shift value for PRS_CH0PULSE */
#define _PRS_SWPULSE_CH0PULSE_MASK           0x1UL                                /**< Bit mask for PRS_CH0PULSE */
#define _PRS_SWPULSE_CH0PULSE_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH0PULSE_DEFAULT         (_PRS_SWPULSE_CH0PULSE_DEFAULT << 0) /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH1PULSE                 (0x1UL << 1)                         /**< Channel 1 Pulse Generation */
#define _PRS_SWPULSE_CH1PULSE_SHIFT          1                                    /**< Shift value for PRS_CH1PULSE */
#define _PRS_SWPULSE_CH1PULSE_MASK           0x2UL                                /**< Bit mask for PRS_CH1PULSE */
#define _PRS_SWPULSE_CH1PULSE_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH1PULSE_DEFAULT         (_PRS_SWPULSE_CH1PULSE_DEFAULT << 1) /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH2PULSE                 (0x1UL << 2)                         /**< Channel 2 Pulse Generation */
#define _PRS_SWPULSE_CH2PULSE_SHIFT          2                                    /**< Shift value for PRS_CH2PULSE */
#define _PRS_SWPULSE_CH2PULSE_MASK           0x4UL                                /**< Bit mask for PRS_CH2PULSE */
#define _PRS_SWPULSE_CH2PULSE_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH2PULSE_DEFAULT         (_PRS_SWPULSE_CH2PULSE_DEFAULT << 2) /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH3PULSE                 (0x1UL << 3)                         /**< Channel 3 Pulse Generation */
#define _PRS_SWPULSE_CH3PULSE_SHIFT          3                                    /**< Shift value for PRS_CH3PULSE */
#define _PRS_SWPULSE_CH3PULSE_MASK           0x8UL                                /**< Bit mask for PRS_CH3PULSE */
#define _PRS_SWPULSE_CH3PULSE_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH3PULSE_DEFAULT         (_PRS_SWPULSE_CH3PULSE_DEFAULT << 3) /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH4PULSE                 (0x1UL << 4)                         /**< Channel 4 Pulse Generation */
#define _PRS_SWPULSE_CH4PULSE_SHIFT          4                                    /**< Shift value for PRS_CH4PULSE */
#define _PRS_SWPULSE_CH4PULSE_MASK           0x10UL                               /**< Bit mask for PRS_CH4PULSE */
#define _PRS_SWPULSE_CH4PULSE_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH4PULSE_DEFAULT         (_PRS_SWPULSE_CH4PULSE_DEFAULT << 4) /**< Shifted mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH5PULSE                 (0x1UL << 5)                         /**< Channel 5 Pulse Generation */
#define _PRS_SWPULSE_CH5PULSE_SHIFT          5                                    /**< Shift value for PRS_CH5PULSE */
#define _PRS_SWPULSE_CH5PULSE_MASK           0x20UL                               /**< Bit mask for PRS_CH5PULSE */
#define _PRS_SWPULSE_CH5PULSE_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWPULSE */
#define PRS_SWPULSE_CH5PULSE_DEFAULT         (_PRS_SWPULSE_CH5PULSE_DEFAULT << 5) /**< Shifted mode DEFAULT for PRS_SWPULSE */

/* Bit fields for PRS SWLEVEL */
#define _PRS_SWLEVEL_RESETVALUE              0x00000000UL                         /**< Default value for PRS_SWLEVEL */
#define _PRS_SWLEVEL_MASK                    0x0000003FUL                         /**< Mask for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH0LEVEL                 (0x1UL << 0)                         /**< Channel 0 Software Level */
#define _PRS_SWLEVEL_CH0LEVEL_SHIFT          0                                    /**< Shift value for PRS_CH0LEVEL */
#define _PRS_SWLEVEL_CH0LEVEL_MASK           0x1UL                                /**< Bit mask for PRS_CH0LEVEL */
#define _PRS_SWLEVEL_CH0LEVEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH0LEVEL_DEFAULT         (_PRS_SWLEVEL_CH0LEVEL_DEFAULT << 0) /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH1LEVEL                 (0x1UL << 1)                         /**< Channel 1 Software Level */
#define _PRS_SWLEVEL_CH1LEVEL_SHIFT          1                                    /**< Shift value for PRS_CH1LEVEL */
#define _PRS_SWLEVEL_CH1LEVEL_MASK           0x2UL                                /**< Bit mask for PRS_CH1LEVEL */
#define _PRS_SWLEVEL_CH1LEVEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH1LEVEL_DEFAULT         (_PRS_SWLEVEL_CH1LEVEL_DEFAULT << 1) /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH2LEVEL                 (0x1UL << 2)                         /**< Channel 2 Software Level */
#define _PRS_SWLEVEL_CH2LEVEL_SHIFT          2                                    /**< Shift value for PRS_CH2LEVEL */
#define _PRS_SWLEVEL_CH2LEVEL_MASK           0x4UL                                /**< Bit mask for PRS_CH2LEVEL */
#define _PRS_SWLEVEL_CH2LEVEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH2LEVEL_DEFAULT         (_PRS_SWLEVEL_CH2LEVEL_DEFAULT << 2) /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH3LEVEL                 (0x1UL << 3)                         /**< Channel 3 Software Level */
#define _PRS_SWLEVEL_CH3LEVEL_SHIFT          3                                    /**< Shift value for PRS_CH3LEVEL */
#define _PRS_SWLEVEL_CH3LEVEL_MASK           0x8UL                                /**< Bit mask for PRS_CH3LEVEL */
#define _PRS_SWLEVEL_CH3LEVEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH3LEVEL_DEFAULT         (_PRS_SWLEVEL_CH3LEVEL_DEFAULT << 3) /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH4LEVEL                 (0x1UL << 4)                         /**< Channel 4 Software Level */
#define _PRS_SWLEVEL_CH4LEVEL_SHIFT          4                                    /**< Shift value for PRS_CH4LEVEL */
#define _PRS_SWLEVEL_CH4LEVEL_MASK           0x10UL                               /**< Bit mask for PRS_CH4LEVEL */
#define _PRS_SWLEVEL_CH4LEVEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH4LEVEL_DEFAULT         (_PRS_SWLEVEL_CH4LEVEL_DEFAULT << 4) /**< Shifted mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH5LEVEL                 (0x1UL << 5)                         /**< Channel 5 Software Level */
#define _PRS_SWLEVEL_CH5LEVEL_SHIFT          5                                    /**< Shift value for PRS_CH5LEVEL */
#define _PRS_SWLEVEL_CH5LEVEL_MASK           0x20UL                               /**< Bit mask for PRS_CH5LEVEL */
#define _PRS_SWLEVEL_CH5LEVEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for PRS_SWLEVEL */
#define PRS_SWLEVEL_CH5LEVEL_DEFAULT         (_PRS_SWLEVEL_CH5LEVEL_DEFAULT << 5) /**< Shifted mode DEFAULT for PRS_SWLEVEL */

/* Bit fields for PRS ROUTE */
#define _PRS_ROUTE_RESETVALUE                0x00000000UL                       /**< Default value for PRS_ROUTE */
#define _PRS_ROUTE_MASK                      0x0000070FUL                       /**< Mask for PRS_ROUTE */
#define PRS_ROUTE_CH0PEN                     (0x1UL << 0)                       /**< CH0 Pin Enable */
#define _PRS_ROUTE_CH0PEN_SHIFT              0                                  /**< Shift value for PRS_CH0PEN */
#define _PRS_ROUTE_CH0PEN_MASK               0x1UL                              /**< Bit mask for PRS_CH0PEN */
#define _PRS_ROUTE_CH0PEN_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH0PEN_DEFAULT             (_PRS_ROUTE_CH0PEN_DEFAULT << 0)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH1PEN                     (0x1UL << 1)                       /**< CH1 Pin Enable */
#define _PRS_ROUTE_CH1PEN_SHIFT              1                                  /**< Shift value for PRS_CH1PEN */
#define _PRS_ROUTE_CH1PEN_MASK               0x2UL                              /**< Bit mask for PRS_CH1PEN */
#define _PRS_ROUTE_CH1PEN_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH1PEN_DEFAULT             (_PRS_ROUTE_CH1PEN_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH2PEN                     (0x1UL << 2)                       /**< CH2 Pin Enable */
#define _PRS_ROUTE_CH2PEN_SHIFT              2                                  /**< Shift value for PRS_CH2PEN */
#define _PRS_ROUTE_CH2PEN_MASK               0x4UL                              /**< Bit mask for PRS_CH2PEN */
#define _PRS_ROUTE_CH2PEN_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH2PEN_DEFAULT             (_PRS_ROUTE_CH2PEN_DEFAULT << 2)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH3PEN                     (0x1UL << 3)                       /**< CH3 Pin Enable */
#define _PRS_ROUTE_CH3PEN_SHIFT              3                                  /**< Shift value for PRS_CH3PEN */
#define _PRS_ROUTE_CH3PEN_MASK               0x8UL                              /**< Bit mask for PRS_CH3PEN */
#define _PRS_ROUTE_CH3PEN_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_CH3PEN_DEFAULT             (_PRS_ROUTE_CH3PEN_DEFAULT << 3)   /**< Shifted mode DEFAULT for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_SHIFT            8                                  /**< Shift value for PRS_LOCATION */
#define _PRS_ROUTE_LOCATION_MASK             0x700UL                            /**< Bit mask for PRS_LOCATION */
#define _PRS_ROUTE_LOCATION_LOC0             0x00000000UL                       /**< Mode LOC0 for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_LOC1             0x00000001UL                       /**< Mode LOC1 for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_LOC2             0x00000002UL                       /**< Mode LOC2 for PRS_ROUTE */
#define _PRS_ROUTE_LOCATION_LOC3             0x00000003UL                       /**< Mode LOC3 for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_LOC0              (_PRS_ROUTE_LOCATION_LOC0 << 8)    /**< Shifted mode LOC0 for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_DEFAULT           (_PRS_ROUTE_LOCATION_DEFAULT << 8) /**< Shifted mode DEFAULT for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_LOC1              (_PRS_ROUTE_LOCATION_LOC1 << 8)    /**< Shifted mode LOC1 for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_LOC2              (_PRS_ROUTE_LOCATION_LOC2 << 8)    /**< Shifted mode LOC2 for PRS_ROUTE */
#define PRS_ROUTE_LOCATION_LOC3              (_PRS_ROUTE_LOCATION_LOC3 << 8)    /**< Shifted mode LOC3 for PRS_ROUTE */

/* Bit fields for PRS CH_CTRL */
#define _PRS_CH_CTRL_RESETVALUE              0x00000000UL                             /**< Default value for PRS_CH_CTRL */
#define _PRS_CH_CTRL_MASK                    0x133F0007UL                             /**< Mask for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_SHIFT            0                                        /**< Shift value for PRS_SIGSEL */
#define _PRS_CH_CTRL_SIGSEL_MASK             0x7UL                                    /**< Bit mask for PRS_SIGSEL */
#define _PRS_CH_CTRL_SIGSEL_VCMPOUT          0x00000000UL                             /**< Mode VCMPOUT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ACMP0OUT         0x00000000UL                             /**< Mode ACMP0OUT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ADC0SINGLE       0x00000000UL                             /**< Mode ADC0SINGLE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0IRTX       0x00000000UL                             /**< Mode USART0IRTX for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1IRTX       0x00000000UL                             /**< Mode USART1IRTX for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0UF         0x00000000UL                             /**< Mode TIMER0UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1UF         0x00000000UL                             /**< Mode TIMER1UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2UF         0x00000000UL                             /**< Mode TIMER2UF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USBSOF           0x00000000UL                             /**< Mode USBSOF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCOF            0x00000000UL                             /**< Mode RTCOF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN0         0x00000000UL                             /**< Mode GPIOPIN0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN8         0x00000000UL                             /**< Mode GPIOPIN8 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_PCNT0TCC         0x00000000UL                             /**< Mode PCNT0TCC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_ADC0SCAN         0x00000001UL                             /**< Mode ADC0SCAN for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0TXC        0x00000001UL                             /**< Mode USART0TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1TXC        0x00000001UL                             /**< Mode USART1TXC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0OF         0x00000001UL                             /**< Mode TIMER0OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1OF         0x00000001UL                             /**< Mode TIMER1OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2OF         0x00000001UL                             /**< Mode TIMER2OF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USBSOFSR         0x00000001UL                             /**< Mode USBSOFSR for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCCOMP0         0x00000001UL                             /**< Mode RTCCOMP0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN1         0x00000001UL                             /**< Mode GPIOPIN1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN9         0x00000001UL                             /**< Mode GPIOPIN9 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART0RXDATAV    0x00000002UL                             /**< Mode USART0RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_USART1RXDATAV    0x00000002UL                             /**< Mode USART1RXDATAV for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC0        0x00000002UL                             /**< Mode TIMER0CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC0        0x00000002UL                             /**< Mode TIMER1CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2CC0        0x00000002UL                             /**< Mode TIMER2CC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_RTCCOMP1         0x00000002UL                             /**< Mode RTCCOMP1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN2         0x00000002UL                             /**< Mode GPIOPIN2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN10        0x00000002UL                             /**< Mode GPIOPIN10 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC1        0x00000003UL                             /**< Mode TIMER0CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC1        0x00000003UL                             /**< Mode TIMER1CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2CC1        0x00000003UL                             /**< Mode TIMER2CC1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN3         0x00000003UL                             /**< Mode GPIOPIN3 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN11        0x00000003UL                             /**< Mode GPIOPIN11 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER0CC2        0x00000004UL                             /**< Mode TIMER0CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER1CC2        0x00000004UL                             /**< Mode TIMER1CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_TIMER2CC2        0x00000004UL                             /**< Mode TIMER2CC2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN4         0x00000004UL                             /**< Mode GPIOPIN4 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN12        0x00000004UL                             /**< Mode GPIOPIN12 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN5         0x00000005UL                             /**< Mode GPIOPIN5 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN13        0x00000005UL                             /**< Mode GPIOPIN13 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN6         0x00000006UL                             /**< Mode GPIOPIN6 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN14        0x00000006UL                             /**< Mode GPIOPIN14 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN7         0x00000007UL                             /**< Mode GPIOPIN7 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SIGSEL_GPIOPIN15        0x00000007UL                             /**< Mode GPIOPIN15 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_VCMPOUT           (_PRS_CH_CTRL_SIGSEL_VCMPOUT << 0)       /**< Shifted mode VCMPOUT for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ACMP0OUT          (_PRS_CH_CTRL_SIGSEL_ACMP0OUT << 0)      /**< Shifted mode ACMP0OUT for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ADC0SINGLE        (_PRS_CH_CTRL_SIGSEL_ADC0SINGLE << 0)    /**< Shifted mode ADC0SINGLE for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0IRTX        (_PRS_CH_CTRL_SIGSEL_USART0IRTX << 0)    /**< Shifted mode USART0IRTX for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1IRTX        (_PRS_CH_CTRL_SIGSEL_USART1IRTX << 0)    /**< Shifted mode USART1IRTX for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0UF          (_PRS_CH_CTRL_SIGSEL_TIMER0UF << 0)      /**< Shifted mode TIMER0UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1UF          (_PRS_CH_CTRL_SIGSEL_TIMER1UF << 0)      /**< Shifted mode TIMER1UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2UF          (_PRS_CH_CTRL_SIGSEL_TIMER2UF << 0)      /**< Shifted mode TIMER2UF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USBSOF            (_PRS_CH_CTRL_SIGSEL_USBSOF << 0)        /**< Shifted mode USBSOF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCOF             (_PRS_CH_CTRL_SIGSEL_RTCOF << 0)         /**< Shifted mode RTCOF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN0          (_PRS_CH_CTRL_SIGSEL_GPIOPIN0 << 0)      /**< Shifted mode GPIOPIN0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN8          (_PRS_CH_CTRL_SIGSEL_GPIOPIN8 << 0)      /**< Shifted mode GPIOPIN8 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_PCNT0TCC          (_PRS_CH_CTRL_SIGSEL_PCNT0TCC << 0)      /**< Shifted mode PCNT0TCC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_ADC0SCAN          (_PRS_CH_CTRL_SIGSEL_ADC0SCAN << 0)      /**< Shifted mode ADC0SCAN for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0TXC         (_PRS_CH_CTRL_SIGSEL_USART0TXC << 0)     /**< Shifted mode USART0TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1TXC         (_PRS_CH_CTRL_SIGSEL_USART1TXC << 0)     /**< Shifted mode USART1TXC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0OF          (_PRS_CH_CTRL_SIGSEL_TIMER0OF << 0)      /**< Shifted mode TIMER0OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1OF          (_PRS_CH_CTRL_SIGSEL_TIMER1OF << 0)      /**< Shifted mode TIMER1OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2OF          (_PRS_CH_CTRL_SIGSEL_TIMER2OF << 0)      /**< Shifted mode TIMER2OF for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USBSOFSR          (_PRS_CH_CTRL_SIGSEL_USBSOFSR << 0)      /**< Shifted mode USBSOFSR for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCCOMP0          (_PRS_CH_CTRL_SIGSEL_RTCCOMP0 << 0)      /**< Shifted mode RTCCOMP0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN1          (_PRS_CH_CTRL_SIGSEL_GPIOPIN1 << 0)      /**< Shifted mode GPIOPIN1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN9          (_PRS_CH_CTRL_SIGSEL_GPIOPIN9 << 0)      /**< Shifted mode GPIOPIN9 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART0RXDATAV     (_PRS_CH_CTRL_SIGSEL_USART0RXDATAV << 0) /**< Shifted mode USART0RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_USART1RXDATAV     (_PRS_CH_CTRL_SIGSEL_USART1RXDATAV << 0) /**< Shifted mode USART1RXDATAV for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC0         (_PRS_CH_CTRL_SIGSEL_TIMER0CC0 << 0)     /**< Shifted mode TIMER0CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC0         (_PRS_CH_CTRL_SIGSEL_TIMER1CC0 << 0)     /**< Shifted mode TIMER1CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2CC0         (_PRS_CH_CTRL_SIGSEL_TIMER2CC0 << 0)     /**< Shifted mode TIMER2CC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_RTCCOMP1          (_PRS_CH_CTRL_SIGSEL_RTCCOMP1 << 0)      /**< Shifted mode RTCCOMP1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN2          (_PRS_CH_CTRL_SIGSEL_GPIOPIN2 << 0)      /**< Shifted mode GPIOPIN2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN10         (_PRS_CH_CTRL_SIGSEL_GPIOPIN10 << 0)     /**< Shifted mode GPIOPIN10 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC1         (_PRS_CH_CTRL_SIGSEL_TIMER0CC1 << 0)     /**< Shifted mode TIMER0CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC1         (_PRS_CH_CTRL_SIGSEL_TIMER1CC1 << 0)     /**< Shifted mode TIMER1CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2CC1         (_PRS_CH_CTRL_SIGSEL_TIMER2CC1 << 0)     /**< Shifted mode TIMER2CC1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN3          (_PRS_CH_CTRL_SIGSEL_GPIOPIN3 << 0)      /**< Shifted mode GPIOPIN3 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN11         (_PRS_CH_CTRL_SIGSEL_GPIOPIN11 << 0)     /**< Shifted mode GPIOPIN11 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER0CC2         (_PRS_CH_CTRL_SIGSEL_TIMER0CC2 << 0)     /**< Shifted mode TIMER0CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER1CC2         (_PRS_CH_CTRL_SIGSEL_TIMER1CC2 << 0)     /**< Shifted mode TIMER1CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_TIMER2CC2         (_PRS_CH_CTRL_SIGSEL_TIMER2CC2 << 0)     /**< Shifted mode TIMER2CC2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN4          (_PRS_CH_CTRL_SIGSEL_GPIOPIN4 << 0)      /**< Shifted mode GPIOPIN4 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN12         (_PRS_CH_CTRL_SIGSEL_GPIOPIN12 << 0)     /**< Shifted mode GPIOPIN12 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN5          (_PRS_CH_CTRL_SIGSEL_GPIOPIN5 << 0)      /**< Shifted mode GPIOPIN5 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN13         (_PRS_CH_CTRL_SIGSEL_GPIOPIN13 << 0)     /**< Shifted mode GPIOPIN13 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN6          (_PRS_CH_CTRL_SIGSEL_GPIOPIN6 << 0)      /**< Shifted mode GPIOPIN6 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN14         (_PRS_CH_CTRL_SIGSEL_GPIOPIN14 << 0)     /**< Shifted mode GPIOPIN14 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN7          (_PRS_CH_CTRL_SIGSEL_GPIOPIN7 << 0)      /**< Shifted mode GPIOPIN7 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SIGSEL_GPIOPIN15         (_PRS_CH_CTRL_SIGSEL_GPIOPIN15 << 0)     /**< Shifted mode GPIOPIN15 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_SHIFT         16                                       /**< Shift value for PRS_SOURCESEL */
#define _PRS_CH_CTRL_SOURCESEL_MASK          0x3F0000UL                               /**< Bit mask for PRS_SOURCESEL */
#define _PRS_CH_CTRL_SOURCESEL_NONE          0x00000000UL                             /**< Mode NONE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_VCMP          0x00000001UL                             /**< Mode VCMP for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ACMP0         0x00000002UL                             /**< Mode ACMP0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_ADC0          0x00000008UL                             /**< Mode ADC0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USART0        0x00000010UL                             /**< Mode USART0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USART1        0x00000011UL                             /**< Mode USART1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER0        0x0000001CUL                             /**< Mode TIMER0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER1        0x0000001DUL                             /**< Mode TIMER1 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_TIMER2        0x0000001EUL                             /**< Mode TIMER2 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_USB           0x00000024UL                             /**< Mode USB for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_RTC           0x00000028UL                             /**< Mode RTC for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_GPIOL         0x00000030UL                             /**< Mode GPIOL for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_GPIOH         0x00000031UL                             /**< Mode GPIOH for PRS_CH_CTRL */
#define _PRS_CH_CTRL_SOURCESEL_PCNT0         0x00000036UL                             /**< Mode PCNT0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_NONE           (_PRS_CH_CTRL_SOURCESEL_NONE << 16)      /**< Shifted mode NONE for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_VCMP           (_PRS_CH_CTRL_SOURCESEL_VCMP << 16)      /**< Shifted mode VCMP for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ACMP0          (_PRS_CH_CTRL_SOURCESEL_ACMP0 << 16)     /**< Shifted mode ACMP0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_ADC0           (_PRS_CH_CTRL_SOURCESEL_ADC0 << 16)      /**< Shifted mode ADC0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USART0         (_PRS_CH_CTRL_SOURCESEL_USART0 << 16)    /**< Shifted mode USART0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USART1         (_PRS_CH_CTRL_SOURCESEL_USART1 << 16)    /**< Shifted mode USART1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER0         (_PRS_CH_CTRL_SOURCESEL_TIMER0 << 16)    /**< Shifted mode TIMER0 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER1         (_PRS_CH_CTRL_SOURCESEL_TIMER1 << 16)    /**< Shifted mode TIMER1 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_TIMER2         (_PRS_CH_CTRL_SOURCESEL_TIMER2 << 16)    /**< Shifted mode TIMER2 for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_USB            (_PRS_CH_CTRL_SOURCESEL_USB << 16)       /**< Shifted mode USB for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_RTC            (_PRS_CH_CTRL_SOURCESEL_RTC << 16)       /**< Shifted mode RTC for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_GPIOL          (_PRS_CH_CTRL_SOURCESEL_GPIOL << 16)     /**< Shifted mode GPIOL for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_GPIOH          (_PRS_CH_CTRL_SOURCESEL_GPIOH << 16)     /**< Shifted mode GPIOH for PRS_CH_CTRL */
#define PRS_CH_CTRL_SOURCESEL_PCNT0          (_PRS_CH_CTRL_SOURCESEL_PCNT0 << 16)     /**< Shifted mode PCNT0 for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_SHIFT             24                                       /**< Shift value for PRS_EDSEL */
#define _PRS_CH_CTRL_EDSEL_MASK              0x3000000UL                              /**< Bit mask for PRS_EDSEL */
#define _PRS_CH_CTRL_EDSEL_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_OFF               0x00000000UL                             /**< Mode OFF for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_POSEDGE           0x00000001UL                             /**< Mode POSEDGE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_NEGEDGE           0x00000002UL                             /**< Mode NEGEDGE for PRS_CH_CTRL */
#define _PRS_CH_CTRL_EDSEL_BOTHEDGES         0x00000003UL                             /**< Mode BOTHEDGES for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_DEFAULT            (_PRS_CH_CTRL_EDSEL_DEFAULT << 24)       /**< Shifted mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_OFF                (_PRS_CH_CTRL_EDSEL_OFF << 24)           /**< Shifted mode OFF for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_POSEDGE            (_PRS_CH_CTRL_EDSEL_POSEDGE << 24)       /**< Shifted mode POSEDGE for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_NEGEDGE            (_PRS_CH_CTRL_EDSEL_NEGEDGE << 24)       /**< Shifted mode NEGEDGE for PRS_CH_CTRL */
#define PRS_CH_CTRL_EDSEL_BOTHEDGES          (_PRS_CH_CTRL_EDSEL_BOTHEDGES << 24)     /**< Shifted mode BOTHEDGES for PRS_CH_CTRL */
#define PRS_CH_CTRL_ASYNC                    (0x1UL << 28)                            /**< Asynchronous reflex */
#define _PRS_CH_CTRL_ASYNC_SHIFT             28                                       /**< Shift value for PRS_ASYNC */
#define _PRS_CH_CTRL_ASYNC_MASK              0x10000000UL                             /**< Bit mask for PRS_ASYNC */
#define _PRS_CH_CTRL_ASYNC_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for PRS_CH_CTRL */
#define PRS_CH_CTRL_ASYNC_DEFAULT            (_PRS_CH_CTRL_ASYNC_DEFAULT << 28)       /**< Shifted mode DEFAULT for PRS_CH_CTRL */

/* Bit fields for PRS TRACECTRL */
#define _PRS_TRACECTRL_RESETVALUE            0x00000000UL                           /**< Default value for PRS_TRACECTRL */
#define _PRS_TRACECTRL_MASK                  0x00000F0FUL                           /**< Mask for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTARTEN               (0x1UL << 0)                           /**< PRS TSTART Enable */
#define _PRS_TRACECTRL_TSTARTEN_SHIFT        0                                      /**< Shift value for PRS_TSTARTEN */
#define _PRS_TRACECTRL_TSTARTEN_MASK         0x1UL                                  /**< Bit mask for PRS_TSTARTEN */
#define _PRS_TRACECTRL_TSTARTEN_DEFAULT      0x00000000UL                           /**< Mode DEFAULT for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTARTEN_DEFAULT       (_PRS_TRACECTRL_TSTARTEN_DEFAULT << 0) /**< Shifted mode DEFAULT for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTART_SHIFT          1                                      /**< Shift value for PRS_TSTART */
#define _PRS_TRACECTRL_TSTART_MASK           0xEUL                                  /**< Bit mask for PRS_TSTART */
#define _PRS_TRACECTRL_TSTART_DEFAULT        0x00000000UL                           /**< Mode DEFAULT for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTART_PRSCH0         0x00000000UL                           /**< Mode PRSCH0 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTART_PRSCH1         0x00000001UL                           /**< Mode PRSCH1 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTART_PRSCH2         0x00000002UL                           /**< Mode PRSCH2 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTART_PRSCH3         0x00000003UL                           /**< Mode PRSCH3 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTART_PRSCH4         0x00000004UL                           /**< Mode PRSCH4 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTART_PRSCH5         0x00000005UL                           /**< Mode PRSCH5 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTART_DEFAULT         (_PRS_TRACECTRL_TSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTART_PRSCH0          (_PRS_TRACECTRL_TSTART_PRSCH0 << 1)    /**< Shifted mode PRSCH0 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTART_PRSCH1          (_PRS_TRACECTRL_TSTART_PRSCH1 << 1)    /**< Shifted mode PRSCH1 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTART_PRSCH2          (_PRS_TRACECTRL_TSTART_PRSCH2 << 1)    /**< Shifted mode PRSCH2 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTART_PRSCH3          (_PRS_TRACECTRL_TSTART_PRSCH3 << 1)    /**< Shifted mode PRSCH3 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTART_PRSCH4          (_PRS_TRACECTRL_TSTART_PRSCH4 << 1)    /**< Shifted mode PRSCH4 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTART_PRSCH5          (_PRS_TRACECTRL_TSTART_PRSCH5 << 1)    /**< Shifted mode PRSCH5 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOPEN                (0x1UL << 8)                           /**< PRS TSTOP Enable */
#define _PRS_TRACECTRL_TSTOPEN_SHIFT         8                                      /**< Shift value for PRS_TSTOPEN */
#define _PRS_TRACECTRL_TSTOPEN_MASK          0x100UL                                /**< Bit mask for PRS_TSTOPEN */
#define _PRS_TRACECTRL_TSTOPEN_DEFAULT       0x00000000UL                           /**< Mode DEFAULT for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOPEN_DEFAULT        (_PRS_TRACECTRL_TSTOPEN_DEFAULT << 8)  /**< Shifted mode DEFAULT for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTOP_SHIFT           9                                      /**< Shift value for PRS_TSTOP */
#define _PRS_TRACECTRL_TSTOP_MASK            0xE00UL                                /**< Bit mask for PRS_TSTOP */
#define _PRS_TRACECTRL_TSTOP_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTOP_PRSCH0          0x00000000UL                           /**< Mode PRSCH0 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTOP_PRSCH1          0x00000001UL                           /**< Mode PRSCH1 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTOP_PRSCH2          0x00000002UL                           /**< Mode PRSCH2 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTOP_PRSCH3          0x00000003UL                           /**< Mode PRSCH3 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTOP_PRSCH4          0x00000004UL                           /**< Mode PRSCH4 for PRS_TRACECTRL */
#define _PRS_TRACECTRL_TSTOP_PRSCH5          0x00000005UL                           /**< Mode PRSCH5 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOP_DEFAULT          (_PRS_TRACECTRL_TSTOP_DEFAULT << 9)    /**< Shifted mode DEFAULT for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOP_PRSCH0           (_PRS_TRACECTRL_TSTOP_PRSCH0 << 9)     /**< Shifted mode PRSCH0 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOP_PRSCH1           (_PRS_TRACECTRL_TSTOP_PRSCH1 << 9)     /**< Shifted mode PRSCH1 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOP_PRSCH2           (_PRS_TRACECTRL_TSTOP_PRSCH2 << 9)     /**< Shifted mode PRSCH2 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOP_PRSCH3           (_PRS_TRACECTRL_TSTOP_PRSCH3 << 9)     /**< Shifted mode PRSCH3 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOP_PRSCH4           (_PRS_TRACECTRL_TSTOP_PRSCH4 << 9)     /**< Shifted mode PRSCH4 for PRS_TRACECTRL */
#define PRS_TRACECTRL_TSTOP_PRSCH5           (_PRS_TRACECTRL_TSTOP_PRSCH5 << 9)     /**< Shifted mode PRSCH5 for PRS_TRACECTRL */

/** @} End of group EFM32HG_PRS */
/** @} End of group Parts */

