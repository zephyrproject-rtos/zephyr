/**************************************************************************//**
 * @file efm32wg_gpio.h
 * @brief EFM32WG_GPIO register and bit field definitions
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
 * @defgroup EFM32WG_GPIO
 * @{
 * @brief EFM32WG_GPIO Register Declaration
 *****************************************************************************/
typedef struct
{
  GPIO_P_TypeDef P[6];          /**< Port configuration bits */

  uint32_t       RESERVED0[10]; /**< Reserved for future use **/
  __IOM uint32_t EXTIPSELL;     /**< External Interrupt Port Select Low Register  */
  __IOM uint32_t EXTIPSELH;     /**< External Interrupt Port Select High Register  */
  __IOM uint32_t EXTIRISE;      /**< External Interrupt Rising Edge Trigger Register  */
  __IOM uint32_t EXTIFALL;      /**< External Interrupt Falling Edge Trigger Register  */
  __IOM uint32_t IEN;           /**< Interrupt Enable Register  */
  __IM uint32_t  IF;            /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;           /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;           /**< Interrupt Flag Clear Register  */

  __IOM uint32_t ROUTE;         /**< I/O Routing Register  */
  __IOM uint32_t INSENSE;       /**< Input Sense Register  */
  __IOM uint32_t LOCK;          /**< Configuration Lock Register  */
  __IOM uint32_t CTRL;          /**< GPIO Control Register  */
  __IOM uint32_t CMD;           /**< GPIO Command Register  */
  __IOM uint32_t EM4WUEN;       /**< EM4 Wake-up Enable Register  */
  __IOM uint32_t EM4WUPOL;      /**< EM4 Wake-up Polarity Register  */
  __IM uint32_t  EM4WUCAUSE;    /**< EM4 Wake-up Cause Register  */
} GPIO_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_GPIO_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for GPIO P_CTRL */
#define _GPIO_P_CTRL_RESETVALUE                           0x00000000UL                           /**< Default value for GPIO_P_CTRL */
#define _GPIO_P_CTRL_MASK                                 0x00000003UL                           /**< Mask for GPIO_P_CTRL */
#define _GPIO_P_CTRL_DRIVEMODE_SHIFT                      0                                      /**< Shift value for GPIO_DRIVEMODE */
#define _GPIO_P_CTRL_DRIVEMODE_MASK                       0x3UL                                  /**< Bit mask for GPIO_DRIVEMODE */
#define _GPIO_P_CTRL_DRIVEMODE_DEFAULT                    0x00000000UL                           /**< Mode DEFAULT for GPIO_P_CTRL */
#define _GPIO_P_CTRL_DRIVEMODE_STANDARD                   0x00000000UL                           /**< Mode STANDARD for GPIO_P_CTRL */
#define _GPIO_P_CTRL_DRIVEMODE_LOWEST                     0x00000001UL                           /**< Mode LOWEST for GPIO_P_CTRL */
#define _GPIO_P_CTRL_DRIVEMODE_HIGH                       0x00000002UL                           /**< Mode HIGH for GPIO_P_CTRL */
#define _GPIO_P_CTRL_DRIVEMODE_LOW                        0x00000003UL                           /**< Mode LOW for GPIO_P_CTRL */
#define GPIO_P_CTRL_DRIVEMODE_DEFAULT                     (_GPIO_P_CTRL_DRIVEMODE_DEFAULT << 0)  /**< Shifted mode DEFAULT for GPIO_P_CTRL */
#define GPIO_P_CTRL_DRIVEMODE_STANDARD                    (_GPIO_P_CTRL_DRIVEMODE_STANDARD << 0) /**< Shifted mode STANDARD for GPIO_P_CTRL */
#define GPIO_P_CTRL_DRIVEMODE_LOWEST                      (_GPIO_P_CTRL_DRIVEMODE_LOWEST << 0)   /**< Shifted mode LOWEST for GPIO_P_CTRL */
#define GPIO_P_CTRL_DRIVEMODE_HIGH                        (_GPIO_P_CTRL_DRIVEMODE_HIGH << 0)     /**< Shifted mode HIGH for GPIO_P_CTRL */
#define GPIO_P_CTRL_DRIVEMODE_LOW                         (_GPIO_P_CTRL_DRIVEMODE_LOW << 0)      /**< Shifted mode LOW for GPIO_P_CTRL */

/* Bit fields for GPIO P_MODEL */
#define _GPIO_P_MODEL_RESETVALUE                          0x00000000UL                                          /**< Default value for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MASK                                0xFFFFFFFFUL                                          /**< Mask for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_SHIFT                         0                                                     /**< Shift value for GPIO_MODE0 */
#define _GPIO_P_MODEL_MODE0_MASK                          0xFUL                                                 /**< Bit mask for GPIO_MODE0 */
#define _GPIO_P_MODEL_MODE0_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_DEFAULT                        (_GPIO_P_MODEL_MODE0_DEFAULT << 0)                    /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_DISABLED                       (_GPIO_P_MODEL_MODE0_DISABLED << 0)                   /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_INPUT                          (_GPIO_P_MODEL_MODE0_INPUT << 0)                      /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_INPUTPULL                      (_GPIO_P_MODEL_MODE0_INPUTPULL << 0)                  /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE0_INPUTPULLFILTER << 0)            /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_PUSHPULL                       (_GPIO_P_MODEL_MODE0_PUSHPULL << 0)                   /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE0_PUSHPULLDRIVE << 0)              /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDOR                        (_GPIO_P_MODEL_MODE0_WIREDOR << 0)                    /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE0_WIREDORPULLDOWN << 0)            /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDAND                       (_GPIO_P_MODEL_MODE0_WIREDAND << 0)                   /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE0_WIREDANDFILTER << 0)             /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE0_WIREDANDPULLUP << 0)             /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE0_WIREDANDPULLUPFILTER << 0)       /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE0_WIREDANDDRIVE << 0)              /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE0_WIREDANDDRIVEFILTER << 0)        /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUP << 0)        /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUPFILTER << 0)  /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_SHIFT                         4                                                     /**< Shift value for GPIO_MODE1 */
#define _GPIO_P_MODEL_MODE1_MASK                          0xF0UL                                                /**< Bit mask for GPIO_MODE1 */
#define _GPIO_P_MODEL_MODE1_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE1_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_DEFAULT                        (_GPIO_P_MODEL_MODE1_DEFAULT << 4)                    /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_DISABLED                       (_GPIO_P_MODEL_MODE1_DISABLED << 4)                   /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_INPUT                          (_GPIO_P_MODEL_MODE1_INPUT << 4)                      /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_INPUTPULL                      (_GPIO_P_MODEL_MODE1_INPUTPULL << 4)                  /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE1_INPUTPULLFILTER << 4)            /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_PUSHPULL                       (_GPIO_P_MODEL_MODE1_PUSHPULL << 4)                   /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE1_PUSHPULLDRIVE << 4)              /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDOR                        (_GPIO_P_MODEL_MODE1_WIREDOR << 4)                    /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE1_WIREDORPULLDOWN << 4)            /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDAND                       (_GPIO_P_MODEL_MODE1_WIREDAND << 4)                   /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE1_WIREDANDFILTER << 4)             /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE1_WIREDANDPULLUP << 4)             /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE1_WIREDANDPULLUPFILTER << 4)       /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE1_WIREDANDDRIVE << 4)              /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE1_WIREDANDDRIVEFILTER << 4)        /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE1_WIREDANDDRIVEPULLUP << 4)        /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE1_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE1_WIREDANDDRIVEPULLUPFILTER << 4)  /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_SHIFT                         8                                                     /**< Shift value for GPIO_MODE2 */
#define _GPIO_P_MODEL_MODE2_MASK                          0xF00UL                                               /**< Bit mask for GPIO_MODE2 */
#define _GPIO_P_MODEL_MODE2_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE2_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_DEFAULT                        (_GPIO_P_MODEL_MODE2_DEFAULT << 8)                    /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_DISABLED                       (_GPIO_P_MODEL_MODE2_DISABLED << 8)                   /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_INPUT                          (_GPIO_P_MODEL_MODE2_INPUT << 8)                      /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_INPUTPULL                      (_GPIO_P_MODEL_MODE2_INPUTPULL << 8)                  /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE2_INPUTPULLFILTER << 8)            /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_PUSHPULL                       (_GPIO_P_MODEL_MODE2_PUSHPULL << 8)                   /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE2_PUSHPULLDRIVE << 8)              /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDOR                        (_GPIO_P_MODEL_MODE2_WIREDOR << 8)                    /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE2_WIREDORPULLDOWN << 8)            /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDAND                       (_GPIO_P_MODEL_MODE2_WIREDAND << 8)                   /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE2_WIREDANDFILTER << 8)             /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE2_WIREDANDPULLUP << 8)             /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE2_WIREDANDPULLUPFILTER << 8)       /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE2_WIREDANDDRIVE << 8)              /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE2_WIREDANDDRIVEFILTER << 8)        /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE2_WIREDANDDRIVEPULLUP << 8)        /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE2_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE2_WIREDANDDRIVEPULLUPFILTER << 8)  /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_SHIFT                         12                                                    /**< Shift value for GPIO_MODE3 */
#define _GPIO_P_MODEL_MODE3_MASK                          0xF000UL                                              /**< Bit mask for GPIO_MODE3 */
#define _GPIO_P_MODEL_MODE3_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE3_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_DEFAULT                        (_GPIO_P_MODEL_MODE3_DEFAULT << 12)                   /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_DISABLED                       (_GPIO_P_MODEL_MODE3_DISABLED << 12)                  /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_INPUT                          (_GPIO_P_MODEL_MODE3_INPUT << 12)                     /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_INPUTPULL                      (_GPIO_P_MODEL_MODE3_INPUTPULL << 12)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE3_INPUTPULLFILTER << 12)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_PUSHPULL                       (_GPIO_P_MODEL_MODE3_PUSHPULL << 12)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE3_PUSHPULLDRIVE << 12)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDOR                        (_GPIO_P_MODEL_MODE3_WIREDOR << 12)                   /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE3_WIREDORPULLDOWN << 12)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDAND                       (_GPIO_P_MODEL_MODE3_WIREDAND << 12)                  /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE3_WIREDANDFILTER << 12)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE3_WIREDANDPULLUP << 12)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE3_WIREDANDPULLUPFILTER << 12)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE3_WIREDANDDRIVE << 12)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE3_WIREDANDDRIVEFILTER << 12)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE3_WIREDANDDRIVEPULLUP << 12)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE3_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE3_WIREDANDDRIVEPULLUPFILTER << 12) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_SHIFT                         16                                                    /**< Shift value for GPIO_MODE4 */
#define _GPIO_P_MODEL_MODE4_MASK                          0xF0000UL                                             /**< Bit mask for GPIO_MODE4 */
#define _GPIO_P_MODEL_MODE4_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE4_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_DEFAULT                        (_GPIO_P_MODEL_MODE4_DEFAULT << 16)                   /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_DISABLED                       (_GPIO_P_MODEL_MODE4_DISABLED << 16)                  /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_INPUT                          (_GPIO_P_MODEL_MODE4_INPUT << 16)                     /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_INPUTPULL                      (_GPIO_P_MODEL_MODE4_INPUTPULL << 16)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE4_INPUTPULLFILTER << 16)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_PUSHPULL                       (_GPIO_P_MODEL_MODE4_PUSHPULL << 16)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE4_PUSHPULLDRIVE << 16)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDOR                        (_GPIO_P_MODEL_MODE4_WIREDOR << 16)                   /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE4_WIREDORPULLDOWN << 16)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDAND                       (_GPIO_P_MODEL_MODE4_WIREDAND << 16)                  /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE4_WIREDANDFILTER << 16)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE4_WIREDANDPULLUP << 16)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE4_WIREDANDPULLUPFILTER << 16)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE4_WIREDANDDRIVE << 16)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE4_WIREDANDDRIVEFILTER << 16)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE4_WIREDANDDRIVEPULLUP << 16)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE4_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE4_WIREDANDDRIVEPULLUPFILTER << 16) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_SHIFT                         20                                                    /**< Shift value for GPIO_MODE5 */
#define _GPIO_P_MODEL_MODE5_MASK                          0xF00000UL                                            /**< Bit mask for GPIO_MODE5 */
#define _GPIO_P_MODEL_MODE5_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE5_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_DEFAULT                        (_GPIO_P_MODEL_MODE5_DEFAULT << 20)                   /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_DISABLED                       (_GPIO_P_MODEL_MODE5_DISABLED << 20)                  /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_INPUT                          (_GPIO_P_MODEL_MODE5_INPUT << 20)                     /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_INPUTPULL                      (_GPIO_P_MODEL_MODE5_INPUTPULL << 20)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE5_INPUTPULLFILTER << 20)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_PUSHPULL                       (_GPIO_P_MODEL_MODE5_PUSHPULL << 20)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE5_PUSHPULLDRIVE << 20)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDOR                        (_GPIO_P_MODEL_MODE5_WIREDOR << 20)                   /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE5_WIREDORPULLDOWN << 20)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDAND                       (_GPIO_P_MODEL_MODE5_WIREDAND << 20)                  /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE5_WIREDANDFILTER << 20)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE5_WIREDANDPULLUP << 20)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE5_WIREDANDPULLUPFILTER << 20)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE5_WIREDANDDRIVE << 20)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE5_WIREDANDDRIVEFILTER << 20)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE5_WIREDANDDRIVEPULLUP << 20)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE5_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE5_WIREDANDDRIVEPULLUPFILTER << 20) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_SHIFT                         24                                                    /**< Shift value for GPIO_MODE6 */
#define _GPIO_P_MODEL_MODE6_MASK                          0xF000000UL                                           /**< Bit mask for GPIO_MODE6 */
#define _GPIO_P_MODEL_MODE6_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE6_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_DEFAULT                        (_GPIO_P_MODEL_MODE6_DEFAULT << 24)                   /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_DISABLED                       (_GPIO_P_MODEL_MODE6_DISABLED << 24)                  /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_INPUT                          (_GPIO_P_MODEL_MODE6_INPUT << 24)                     /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_INPUTPULL                      (_GPIO_P_MODEL_MODE6_INPUTPULL << 24)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE6_INPUTPULLFILTER << 24)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_PUSHPULL                       (_GPIO_P_MODEL_MODE6_PUSHPULL << 24)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE6_PUSHPULLDRIVE << 24)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDOR                        (_GPIO_P_MODEL_MODE6_WIREDOR << 24)                   /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE6_WIREDORPULLDOWN << 24)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDAND                       (_GPIO_P_MODEL_MODE6_WIREDAND << 24)                  /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE6_WIREDANDFILTER << 24)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE6_WIREDANDPULLUP << 24)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE6_WIREDANDPULLUPFILTER << 24)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE6_WIREDANDDRIVE << 24)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE6_WIREDANDDRIVEFILTER << 24)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE6_WIREDANDDRIVEPULLUP << 24)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE6_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE6_WIREDANDDRIVEPULLUPFILTER << 24) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_SHIFT                         28                                                    /**< Shift value for GPIO_MODE7 */
#define _GPIO_P_MODEL_MODE7_MASK                          0xF0000000UL                                          /**< Bit mask for GPIO_MODE7 */
#define _GPIO_P_MODEL_MODE7_DEFAULT                       0x00000000UL                                          /**< Mode DEFAULT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_DISABLED                      0x00000000UL                                          /**< Mode DISABLED for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_INPUT                         0x00000001UL                                          /**< Mode INPUT for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_INPUTPULL                     0x00000002UL                                          /**< Mode INPUTPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_INPUTPULLFILTER               0x00000003UL                                          /**< Mode INPUTPULLFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_PUSHPULL                      0x00000004UL                                          /**< Mode PUSHPULL for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_PUSHPULLDRIVE                 0x00000005UL                                          /**< Mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDOR                       0x00000006UL                                          /**< Mode WIREDOR for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDORPULLDOWN               0x00000007UL                                          /**< Mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDAND                      0x00000008UL                                          /**< Mode WIREDAND for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDANDFILTER                0x00000009UL                                          /**< Mode WIREDANDFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDANDPULLUP                0x0000000AUL                                          /**< Mode WIREDANDPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDANDPULLUPFILTER          0x0000000BUL                                          /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDANDDRIVE                 0x0000000CUL                                          /**< Mode WIREDANDDRIVE for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDANDDRIVEFILTER           0x0000000DUL                                          /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDANDDRIVEPULLUP           0x0000000EUL                                          /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define _GPIO_P_MODEL_MODE7_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                          /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_DEFAULT                        (_GPIO_P_MODEL_MODE7_DEFAULT << 28)                   /**< Shifted mode DEFAULT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_DISABLED                       (_GPIO_P_MODEL_MODE7_DISABLED << 28)                  /**< Shifted mode DISABLED for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_INPUT                          (_GPIO_P_MODEL_MODE7_INPUT << 28)                     /**< Shifted mode INPUT for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_INPUTPULL                      (_GPIO_P_MODEL_MODE7_INPUTPULL << 28)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_INPUTPULLFILTER                (_GPIO_P_MODEL_MODE7_INPUTPULLFILTER << 28)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_PUSHPULL                       (_GPIO_P_MODEL_MODE7_PUSHPULL << 28)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_PUSHPULLDRIVE                  (_GPIO_P_MODEL_MODE7_PUSHPULLDRIVE << 28)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDOR                        (_GPIO_P_MODEL_MODE7_WIREDOR << 28)                   /**< Shifted mode WIREDOR for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDORPULLDOWN                (_GPIO_P_MODEL_MODE7_WIREDORPULLDOWN << 28)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDAND                       (_GPIO_P_MODEL_MODE7_WIREDAND << 28)                  /**< Shifted mode WIREDAND for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDANDFILTER                 (_GPIO_P_MODEL_MODE7_WIREDANDFILTER << 28)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDANDPULLUP                 (_GPIO_P_MODEL_MODE7_WIREDANDPULLUP << 28)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDANDPULLUPFILTER           (_GPIO_P_MODEL_MODE7_WIREDANDPULLUPFILTER << 28)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDANDDRIVE                  (_GPIO_P_MODEL_MODE7_WIREDANDDRIVE << 28)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDANDDRIVEFILTER            (_GPIO_P_MODEL_MODE7_WIREDANDDRIVEFILTER << 28)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEL_MODE7_WIREDANDDRIVEPULLUP << 28)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEL */
#define GPIO_P_MODEL_MODE7_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEL_MODE7_WIREDANDDRIVEPULLUPFILTER << 28) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEL */

/* Bit fields for GPIO P_MODEH */
#define _GPIO_P_MODEH_RESETVALUE                          0x00000000UL                                           /**< Default value for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MASK                                0xFFFFFFFFUL                                           /**< Mask for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_SHIFT                         0                                                      /**< Shift value for GPIO_MODE8 */
#define _GPIO_P_MODEH_MODE8_MASK                          0xFUL                                                  /**< Bit mask for GPIO_MODE8 */
#define _GPIO_P_MODEH_MODE8_DEFAULT                       0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_DISABLED                      0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_INPUT                         0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_INPUTPULL                     0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_INPUTPULLFILTER               0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_PUSHPULL                      0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_PUSHPULLDRIVE                 0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDOR                       0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDORPULLDOWN               0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDAND                      0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDANDFILTER                0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDANDPULLUP                0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDANDPULLUPFILTER          0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDANDDRIVE                 0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDANDDRIVEFILTER           0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDANDDRIVEPULLUP           0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE8_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_DEFAULT                        (_GPIO_P_MODEH_MODE8_DEFAULT << 0)                     /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_DISABLED                       (_GPIO_P_MODEH_MODE8_DISABLED << 0)                    /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_INPUT                          (_GPIO_P_MODEH_MODE8_INPUT << 0)                       /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_INPUTPULL                      (_GPIO_P_MODEH_MODE8_INPUTPULL << 0)                   /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_INPUTPULLFILTER                (_GPIO_P_MODEH_MODE8_INPUTPULLFILTER << 0)             /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_PUSHPULL                       (_GPIO_P_MODEH_MODE8_PUSHPULL << 0)                    /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_PUSHPULLDRIVE                  (_GPIO_P_MODEH_MODE8_PUSHPULLDRIVE << 0)               /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDOR                        (_GPIO_P_MODEH_MODE8_WIREDOR << 0)                     /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDORPULLDOWN                (_GPIO_P_MODEH_MODE8_WIREDORPULLDOWN << 0)             /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDAND                       (_GPIO_P_MODEH_MODE8_WIREDAND << 0)                    /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDANDFILTER                 (_GPIO_P_MODEH_MODE8_WIREDANDFILTER << 0)              /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDANDPULLUP                 (_GPIO_P_MODEH_MODE8_WIREDANDPULLUP << 0)              /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDANDPULLUPFILTER           (_GPIO_P_MODEH_MODE8_WIREDANDPULLUPFILTER << 0)        /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDANDDRIVE                  (_GPIO_P_MODEH_MODE8_WIREDANDDRIVE << 0)               /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDANDDRIVEFILTER            (_GPIO_P_MODEH_MODE8_WIREDANDDRIVEFILTER << 0)         /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEH_MODE8_WIREDANDDRIVEPULLUP << 0)         /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE8_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEH_MODE8_WIREDANDDRIVEPULLUPFILTER << 0)   /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_SHIFT                         4                                                      /**< Shift value for GPIO_MODE9 */
#define _GPIO_P_MODEH_MODE9_MASK                          0xF0UL                                                 /**< Bit mask for GPIO_MODE9 */
#define _GPIO_P_MODEH_MODE9_DEFAULT                       0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_DISABLED                      0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_INPUT                         0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_INPUTPULL                     0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_INPUTPULLFILTER               0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_PUSHPULL                      0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_PUSHPULLDRIVE                 0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDOR                       0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDORPULLDOWN               0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDAND                      0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDANDFILTER                0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDANDPULLUP                0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDANDPULLUPFILTER          0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDANDDRIVE                 0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDANDDRIVEFILTER           0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDANDDRIVEPULLUP           0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE9_WIREDANDDRIVEPULLUPFILTER     0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_DEFAULT                        (_GPIO_P_MODEH_MODE9_DEFAULT << 4)                     /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_DISABLED                       (_GPIO_P_MODEH_MODE9_DISABLED << 4)                    /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_INPUT                          (_GPIO_P_MODEH_MODE9_INPUT << 4)                       /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_INPUTPULL                      (_GPIO_P_MODEH_MODE9_INPUTPULL << 4)                   /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_INPUTPULLFILTER                (_GPIO_P_MODEH_MODE9_INPUTPULLFILTER << 4)             /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_PUSHPULL                       (_GPIO_P_MODEH_MODE9_PUSHPULL << 4)                    /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_PUSHPULLDRIVE                  (_GPIO_P_MODEH_MODE9_PUSHPULLDRIVE << 4)               /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDOR                        (_GPIO_P_MODEH_MODE9_WIREDOR << 4)                     /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDORPULLDOWN                (_GPIO_P_MODEH_MODE9_WIREDORPULLDOWN << 4)             /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDAND                       (_GPIO_P_MODEH_MODE9_WIREDAND << 4)                    /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDANDFILTER                 (_GPIO_P_MODEH_MODE9_WIREDANDFILTER << 4)              /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDANDPULLUP                 (_GPIO_P_MODEH_MODE9_WIREDANDPULLUP << 4)              /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDANDPULLUPFILTER           (_GPIO_P_MODEH_MODE9_WIREDANDPULLUPFILTER << 4)        /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDANDDRIVE                  (_GPIO_P_MODEH_MODE9_WIREDANDDRIVE << 4)               /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDANDDRIVEFILTER            (_GPIO_P_MODEH_MODE9_WIREDANDDRIVEFILTER << 4)         /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDANDDRIVEPULLUP            (_GPIO_P_MODEH_MODE9_WIREDANDDRIVEPULLUP << 4)         /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE9_WIREDANDDRIVEPULLUPFILTER      (_GPIO_P_MODEH_MODE9_WIREDANDDRIVEPULLUPFILTER << 4)   /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_SHIFT                        8                                                      /**< Shift value for GPIO_MODE10 */
#define _GPIO_P_MODEH_MODE10_MASK                         0xF00UL                                                /**< Bit mask for GPIO_MODE10 */
#define _GPIO_P_MODEH_MODE10_DEFAULT                      0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_DISABLED                     0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_INPUT                        0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_INPUTPULL                    0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_INPUTPULLFILTER              0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_PUSHPULL                     0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_PUSHPULLDRIVE                0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDOR                      0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDORPULLDOWN              0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDAND                     0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDANDFILTER               0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDANDPULLUP               0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDANDPULLUPFILTER         0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDANDDRIVE                0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDANDDRIVEFILTER          0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDANDDRIVEPULLUP          0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE10_WIREDANDDRIVEPULLUPFILTER    0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_DEFAULT                       (_GPIO_P_MODEH_MODE10_DEFAULT << 8)                    /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_DISABLED                      (_GPIO_P_MODEH_MODE10_DISABLED << 8)                   /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_INPUT                         (_GPIO_P_MODEH_MODE10_INPUT << 8)                      /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_INPUTPULL                     (_GPIO_P_MODEH_MODE10_INPUTPULL << 8)                  /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_INPUTPULLFILTER               (_GPIO_P_MODEH_MODE10_INPUTPULLFILTER << 8)            /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_PUSHPULL                      (_GPIO_P_MODEH_MODE10_PUSHPULL << 8)                   /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_PUSHPULLDRIVE                 (_GPIO_P_MODEH_MODE10_PUSHPULLDRIVE << 8)              /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDOR                       (_GPIO_P_MODEH_MODE10_WIREDOR << 8)                    /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDORPULLDOWN               (_GPIO_P_MODEH_MODE10_WIREDORPULLDOWN << 8)            /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDAND                      (_GPIO_P_MODEH_MODE10_WIREDAND << 8)                   /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDANDFILTER                (_GPIO_P_MODEH_MODE10_WIREDANDFILTER << 8)             /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDANDPULLUP                (_GPIO_P_MODEH_MODE10_WIREDANDPULLUP << 8)             /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDANDPULLUPFILTER          (_GPIO_P_MODEH_MODE10_WIREDANDPULLUPFILTER << 8)       /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDANDDRIVE                 (_GPIO_P_MODEH_MODE10_WIREDANDDRIVE << 8)              /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDANDDRIVEFILTER           (_GPIO_P_MODEH_MODE10_WIREDANDDRIVEFILTER << 8)        /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDANDDRIVEPULLUP           (_GPIO_P_MODEH_MODE10_WIREDANDDRIVEPULLUP << 8)        /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE10_WIREDANDDRIVEPULLUPFILTER     (_GPIO_P_MODEH_MODE10_WIREDANDDRIVEPULLUPFILTER << 8)  /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_SHIFT                        12                                                     /**< Shift value for GPIO_MODE11 */
#define _GPIO_P_MODEH_MODE11_MASK                         0xF000UL                                               /**< Bit mask for GPIO_MODE11 */
#define _GPIO_P_MODEH_MODE11_DEFAULT                      0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_DISABLED                     0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_INPUT                        0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_INPUTPULL                    0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_INPUTPULLFILTER              0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_PUSHPULL                     0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_PUSHPULLDRIVE                0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDOR                      0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDORPULLDOWN              0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDAND                     0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDANDFILTER               0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDANDPULLUP               0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDANDPULLUPFILTER         0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDANDDRIVE                0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDANDDRIVEFILTER          0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDANDDRIVEPULLUP          0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE11_WIREDANDDRIVEPULLUPFILTER    0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_DEFAULT                       (_GPIO_P_MODEH_MODE11_DEFAULT << 12)                   /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_DISABLED                      (_GPIO_P_MODEH_MODE11_DISABLED << 12)                  /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_INPUT                         (_GPIO_P_MODEH_MODE11_INPUT << 12)                     /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_INPUTPULL                     (_GPIO_P_MODEH_MODE11_INPUTPULL << 12)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_INPUTPULLFILTER               (_GPIO_P_MODEH_MODE11_INPUTPULLFILTER << 12)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_PUSHPULL                      (_GPIO_P_MODEH_MODE11_PUSHPULL << 12)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_PUSHPULLDRIVE                 (_GPIO_P_MODEH_MODE11_PUSHPULLDRIVE << 12)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDOR                       (_GPIO_P_MODEH_MODE11_WIREDOR << 12)                   /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDORPULLDOWN               (_GPIO_P_MODEH_MODE11_WIREDORPULLDOWN << 12)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDAND                      (_GPIO_P_MODEH_MODE11_WIREDAND << 12)                  /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDANDFILTER                (_GPIO_P_MODEH_MODE11_WIREDANDFILTER << 12)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDANDPULLUP                (_GPIO_P_MODEH_MODE11_WIREDANDPULLUP << 12)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDANDPULLUPFILTER          (_GPIO_P_MODEH_MODE11_WIREDANDPULLUPFILTER << 12)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDANDDRIVE                 (_GPIO_P_MODEH_MODE11_WIREDANDDRIVE << 12)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDANDDRIVEFILTER           (_GPIO_P_MODEH_MODE11_WIREDANDDRIVEFILTER << 12)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDANDDRIVEPULLUP           (_GPIO_P_MODEH_MODE11_WIREDANDDRIVEPULLUP << 12)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE11_WIREDANDDRIVEPULLUPFILTER     (_GPIO_P_MODEH_MODE11_WIREDANDDRIVEPULLUPFILTER << 12) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_SHIFT                        16                                                     /**< Shift value for GPIO_MODE12 */
#define _GPIO_P_MODEH_MODE12_MASK                         0xF0000UL                                              /**< Bit mask for GPIO_MODE12 */
#define _GPIO_P_MODEH_MODE12_DEFAULT                      0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_DISABLED                     0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_INPUT                        0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_INPUTPULL                    0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_INPUTPULLFILTER              0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_PUSHPULL                     0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_PUSHPULLDRIVE                0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDOR                      0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDORPULLDOWN              0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDAND                     0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDANDFILTER               0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDANDPULLUP               0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDANDPULLUPFILTER         0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDANDDRIVE                0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDANDDRIVEFILTER          0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDANDDRIVEPULLUP          0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE12_WIREDANDDRIVEPULLUPFILTER    0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_DEFAULT                       (_GPIO_P_MODEH_MODE12_DEFAULT << 16)                   /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_DISABLED                      (_GPIO_P_MODEH_MODE12_DISABLED << 16)                  /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_INPUT                         (_GPIO_P_MODEH_MODE12_INPUT << 16)                     /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_INPUTPULL                     (_GPIO_P_MODEH_MODE12_INPUTPULL << 16)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_INPUTPULLFILTER               (_GPIO_P_MODEH_MODE12_INPUTPULLFILTER << 16)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_PUSHPULL                      (_GPIO_P_MODEH_MODE12_PUSHPULL << 16)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_PUSHPULLDRIVE                 (_GPIO_P_MODEH_MODE12_PUSHPULLDRIVE << 16)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDOR                       (_GPIO_P_MODEH_MODE12_WIREDOR << 16)                   /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDORPULLDOWN               (_GPIO_P_MODEH_MODE12_WIREDORPULLDOWN << 16)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDAND                      (_GPIO_P_MODEH_MODE12_WIREDAND << 16)                  /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDANDFILTER                (_GPIO_P_MODEH_MODE12_WIREDANDFILTER << 16)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDANDPULLUP                (_GPIO_P_MODEH_MODE12_WIREDANDPULLUP << 16)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDANDPULLUPFILTER          (_GPIO_P_MODEH_MODE12_WIREDANDPULLUPFILTER << 16)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDANDDRIVE                 (_GPIO_P_MODEH_MODE12_WIREDANDDRIVE << 16)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDANDDRIVEFILTER           (_GPIO_P_MODEH_MODE12_WIREDANDDRIVEFILTER << 16)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDANDDRIVEPULLUP           (_GPIO_P_MODEH_MODE12_WIREDANDDRIVEPULLUP << 16)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE12_WIREDANDDRIVEPULLUPFILTER     (_GPIO_P_MODEH_MODE12_WIREDANDDRIVEPULLUPFILTER << 16) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_SHIFT                        20                                                     /**< Shift value for GPIO_MODE13 */
#define _GPIO_P_MODEH_MODE13_MASK                         0xF00000UL                                             /**< Bit mask for GPIO_MODE13 */
#define _GPIO_P_MODEH_MODE13_DEFAULT                      0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_DISABLED                     0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_INPUT                        0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_INPUTPULL                    0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_INPUTPULLFILTER              0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_PUSHPULL                     0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_PUSHPULLDRIVE                0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDOR                      0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDORPULLDOWN              0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDAND                     0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDANDFILTER               0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDANDPULLUP               0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDANDPULLUPFILTER         0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDANDDRIVE                0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDANDDRIVEFILTER          0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDANDDRIVEPULLUP          0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE13_WIREDANDDRIVEPULLUPFILTER    0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_DEFAULT                       (_GPIO_P_MODEH_MODE13_DEFAULT << 20)                   /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_DISABLED                      (_GPIO_P_MODEH_MODE13_DISABLED << 20)                  /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_INPUT                         (_GPIO_P_MODEH_MODE13_INPUT << 20)                     /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_INPUTPULL                     (_GPIO_P_MODEH_MODE13_INPUTPULL << 20)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_INPUTPULLFILTER               (_GPIO_P_MODEH_MODE13_INPUTPULLFILTER << 20)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_PUSHPULL                      (_GPIO_P_MODEH_MODE13_PUSHPULL << 20)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_PUSHPULLDRIVE                 (_GPIO_P_MODEH_MODE13_PUSHPULLDRIVE << 20)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDOR                       (_GPIO_P_MODEH_MODE13_WIREDOR << 20)                   /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDORPULLDOWN               (_GPIO_P_MODEH_MODE13_WIREDORPULLDOWN << 20)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDAND                      (_GPIO_P_MODEH_MODE13_WIREDAND << 20)                  /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDANDFILTER                (_GPIO_P_MODEH_MODE13_WIREDANDFILTER << 20)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDANDPULLUP                (_GPIO_P_MODEH_MODE13_WIREDANDPULLUP << 20)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDANDPULLUPFILTER          (_GPIO_P_MODEH_MODE13_WIREDANDPULLUPFILTER << 20)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDANDDRIVE                 (_GPIO_P_MODEH_MODE13_WIREDANDDRIVE << 20)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDANDDRIVEFILTER           (_GPIO_P_MODEH_MODE13_WIREDANDDRIVEFILTER << 20)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDANDDRIVEPULLUP           (_GPIO_P_MODEH_MODE13_WIREDANDDRIVEPULLUP << 20)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE13_WIREDANDDRIVEPULLUPFILTER     (_GPIO_P_MODEH_MODE13_WIREDANDDRIVEPULLUPFILTER << 20) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_SHIFT                        24                                                     /**< Shift value for GPIO_MODE14 */
#define _GPIO_P_MODEH_MODE14_MASK                         0xF000000UL                                            /**< Bit mask for GPIO_MODE14 */
#define _GPIO_P_MODEH_MODE14_DEFAULT                      0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_DISABLED                     0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_INPUT                        0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_INPUTPULL                    0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_INPUTPULLFILTER              0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_PUSHPULL                     0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_PUSHPULLDRIVE                0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDOR                      0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDORPULLDOWN              0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDAND                     0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDANDFILTER               0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDANDPULLUP               0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDANDPULLUPFILTER         0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDANDDRIVE                0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDANDDRIVEFILTER          0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDANDDRIVEPULLUP          0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE14_WIREDANDDRIVEPULLUPFILTER    0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_DEFAULT                       (_GPIO_P_MODEH_MODE14_DEFAULT << 24)                   /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_DISABLED                      (_GPIO_P_MODEH_MODE14_DISABLED << 24)                  /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_INPUT                         (_GPIO_P_MODEH_MODE14_INPUT << 24)                     /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_INPUTPULL                     (_GPIO_P_MODEH_MODE14_INPUTPULL << 24)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_INPUTPULLFILTER               (_GPIO_P_MODEH_MODE14_INPUTPULLFILTER << 24)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_PUSHPULL                      (_GPIO_P_MODEH_MODE14_PUSHPULL << 24)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_PUSHPULLDRIVE                 (_GPIO_P_MODEH_MODE14_PUSHPULLDRIVE << 24)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDOR                       (_GPIO_P_MODEH_MODE14_WIREDOR << 24)                   /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDORPULLDOWN               (_GPIO_P_MODEH_MODE14_WIREDORPULLDOWN << 24)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDAND                      (_GPIO_P_MODEH_MODE14_WIREDAND << 24)                  /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDANDFILTER                (_GPIO_P_MODEH_MODE14_WIREDANDFILTER << 24)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDANDPULLUP                (_GPIO_P_MODEH_MODE14_WIREDANDPULLUP << 24)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDANDPULLUPFILTER          (_GPIO_P_MODEH_MODE14_WIREDANDPULLUPFILTER << 24)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDANDDRIVE                 (_GPIO_P_MODEH_MODE14_WIREDANDDRIVE << 24)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDANDDRIVEFILTER           (_GPIO_P_MODEH_MODE14_WIREDANDDRIVEFILTER << 24)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDANDDRIVEPULLUP           (_GPIO_P_MODEH_MODE14_WIREDANDDRIVEPULLUP << 24)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE14_WIREDANDDRIVEPULLUPFILTER     (_GPIO_P_MODEH_MODE14_WIREDANDDRIVEPULLUPFILTER << 24) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_SHIFT                        28                                                     /**< Shift value for GPIO_MODE15 */
#define _GPIO_P_MODEH_MODE15_MASK                         0xF0000000UL                                           /**< Bit mask for GPIO_MODE15 */
#define _GPIO_P_MODEH_MODE15_DEFAULT                      0x00000000UL                                           /**< Mode DEFAULT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_DISABLED                     0x00000000UL                                           /**< Mode DISABLED for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_INPUT                        0x00000001UL                                           /**< Mode INPUT for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_INPUTPULL                    0x00000002UL                                           /**< Mode INPUTPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_INPUTPULLFILTER              0x00000003UL                                           /**< Mode INPUTPULLFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_PUSHPULL                     0x00000004UL                                           /**< Mode PUSHPULL for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_PUSHPULLDRIVE                0x00000005UL                                           /**< Mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDOR                      0x00000006UL                                           /**< Mode WIREDOR for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDORPULLDOWN              0x00000007UL                                           /**< Mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDAND                     0x00000008UL                                           /**< Mode WIREDAND for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDANDFILTER               0x00000009UL                                           /**< Mode WIREDANDFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDANDPULLUP               0x0000000AUL                                           /**< Mode WIREDANDPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDANDPULLUPFILTER         0x0000000BUL                                           /**< Mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDANDDRIVE                0x0000000CUL                                           /**< Mode WIREDANDDRIVE for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDANDDRIVEFILTER          0x0000000DUL                                           /**< Mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDANDDRIVEPULLUP          0x0000000EUL                                           /**< Mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define _GPIO_P_MODEH_MODE15_WIREDANDDRIVEPULLUPFILTER    0x0000000FUL                                           /**< Mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_DEFAULT                       (_GPIO_P_MODEH_MODE15_DEFAULT << 28)                   /**< Shifted mode DEFAULT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_DISABLED                      (_GPIO_P_MODEH_MODE15_DISABLED << 28)                  /**< Shifted mode DISABLED for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_INPUT                         (_GPIO_P_MODEH_MODE15_INPUT << 28)                     /**< Shifted mode INPUT for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_INPUTPULL                     (_GPIO_P_MODEH_MODE15_INPUTPULL << 28)                 /**< Shifted mode INPUTPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_INPUTPULLFILTER               (_GPIO_P_MODEH_MODE15_INPUTPULLFILTER << 28)           /**< Shifted mode INPUTPULLFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_PUSHPULL                      (_GPIO_P_MODEH_MODE15_PUSHPULL << 28)                  /**< Shifted mode PUSHPULL for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_PUSHPULLDRIVE                 (_GPIO_P_MODEH_MODE15_PUSHPULLDRIVE << 28)             /**< Shifted mode PUSHPULLDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDOR                       (_GPIO_P_MODEH_MODE15_WIREDOR << 28)                   /**< Shifted mode WIREDOR for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDORPULLDOWN               (_GPIO_P_MODEH_MODE15_WIREDORPULLDOWN << 28)           /**< Shifted mode WIREDORPULLDOWN for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDAND                      (_GPIO_P_MODEH_MODE15_WIREDAND << 28)                  /**< Shifted mode WIREDAND for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDANDFILTER                (_GPIO_P_MODEH_MODE15_WIREDANDFILTER << 28)            /**< Shifted mode WIREDANDFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDANDPULLUP                (_GPIO_P_MODEH_MODE15_WIREDANDPULLUP << 28)            /**< Shifted mode WIREDANDPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDANDPULLUPFILTER          (_GPIO_P_MODEH_MODE15_WIREDANDPULLUPFILTER << 28)      /**< Shifted mode WIREDANDPULLUPFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDANDDRIVE                 (_GPIO_P_MODEH_MODE15_WIREDANDDRIVE << 28)             /**< Shifted mode WIREDANDDRIVE for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDANDDRIVEFILTER           (_GPIO_P_MODEH_MODE15_WIREDANDDRIVEFILTER << 28)       /**< Shifted mode WIREDANDDRIVEFILTER for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDANDDRIVEPULLUP           (_GPIO_P_MODEH_MODE15_WIREDANDDRIVEPULLUP << 28)       /**< Shifted mode WIREDANDDRIVEPULLUP for GPIO_P_MODEH */
#define GPIO_P_MODEH_MODE15_WIREDANDDRIVEPULLUPFILTER     (_GPIO_P_MODEH_MODE15_WIREDANDDRIVEPULLUPFILTER << 28) /**< Shifted mode WIREDANDDRIVEPULLUPFILTER for GPIO_P_MODEH */

/* Bit fields for GPIO P_DOUT */
#define _GPIO_P_DOUT_RESETVALUE                           0x00000000UL                     /**< Default value for GPIO_P_DOUT */
#define _GPIO_P_DOUT_MASK                                 0x0000FFFFUL                     /**< Mask for GPIO_P_DOUT */
#define _GPIO_P_DOUT_DOUT_SHIFT                           0                                /**< Shift value for GPIO_DOUT */
#define _GPIO_P_DOUT_DOUT_MASK                            0xFFFFUL                         /**< Bit mask for GPIO_DOUT */
#define _GPIO_P_DOUT_DOUT_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for GPIO_P_DOUT */
#define GPIO_P_DOUT_DOUT_DEFAULT                          (_GPIO_P_DOUT_DOUT_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_P_DOUT */

/* Bit fields for GPIO P_DOUTSET */
#define _GPIO_P_DOUTSET_RESETVALUE                        0x00000000UL                           /**< Default value for GPIO_P_DOUTSET */
#define _GPIO_P_DOUTSET_MASK                              0x0000FFFFUL                           /**< Mask for GPIO_P_DOUTSET */
#define _GPIO_P_DOUTSET_DOUTSET_SHIFT                     0                                      /**< Shift value for GPIO_DOUTSET */
#define _GPIO_P_DOUTSET_DOUTSET_MASK                      0xFFFFUL                               /**< Bit mask for GPIO_DOUTSET */
#define _GPIO_P_DOUTSET_DOUTSET_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for GPIO_P_DOUTSET */
#define GPIO_P_DOUTSET_DOUTSET_DEFAULT                    (_GPIO_P_DOUTSET_DOUTSET_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_P_DOUTSET */

/* Bit fields for GPIO P_DOUTCLR */
#define _GPIO_P_DOUTCLR_RESETVALUE                        0x00000000UL                           /**< Default value for GPIO_P_DOUTCLR */
#define _GPIO_P_DOUTCLR_MASK                              0x0000FFFFUL                           /**< Mask for GPIO_P_DOUTCLR */
#define _GPIO_P_DOUTCLR_DOUTCLR_SHIFT                     0                                      /**< Shift value for GPIO_DOUTCLR */
#define _GPIO_P_DOUTCLR_DOUTCLR_MASK                      0xFFFFUL                               /**< Bit mask for GPIO_DOUTCLR */
#define _GPIO_P_DOUTCLR_DOUTCLR_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for GPIO_P_DOUTCLR */
#define GPIO_P_DOUTCLR_DOUTCLR_DEFAULT                    (_GPIO_P_DOUTCLR_DOUTCLR_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_P_DOUTCLR */

/* Bit fields for GPIO P_DOUTTGL */
#define _GPIO_P_DOUTTGL_RESETVALUE                        0x00000000UL                           /**< Default value for GPIO_P_DOUTTGL */
#define _GPIO_P_DOUTTGL_MASK                              0x0000FFFFUL                           /**< Mask for GPIO_P_DOUTTGL */
#define _GPIO_P_DOUTTGL_DOUTTGL_SHIFT                     0                                      /**< Shift value for GPIO_DOUTTGL */
#define _GPIO_P_DOUTTGL_DOUTTGL_MASK                      0xFFFFUL                               /**< Bit mask for GPIO_DOUTTGL */
#define _GPIO_P_DOUTTGL_DOUTTGL_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for GPIO_P_DOUTTGL */
#define GPIO_P_DOUTTGL_DOUTTGL_DEFAULT                    (_GPIO_P_DOUTTGL_DOUTTGL_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_P_DOUTTGL */

/* Bit fields for GPIO P_DIN */
#define _GPIO_P_DIN_RESETVALUE                            0x00000000UL                   /**< Default value for GPIO_P_DIN */
#define _GPIO_P_DIN_MASK                                  0x0000FFFFUL                   /**< Mask for GPIO_P_DIN */
#define _GPIO_P_DIN_DIN_SHIFT                             0                              /**< Shift value for GPIO_DIN */
#define _GPIO_P_DIN_DIN_MASK                              0xFFFFUL                       /**< Bit mask for GPIO_DIN */
#define _GPIO_P_DIN_DIN_DEFAULT                           0x00000000UL                   /**< Mode DEFAULT for GPIO_P_DIN */
#define GPIO_P_DIN_DIN_DEFAULT                            (_GPIO_P_DIN_DIN_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_P_DIN */

/* Bit fields for GPIO P_PINLOCKN */
#define _GPIO_P_PINLOCKN_RESETVALUE                       0x0000FFFFUL                             /**< Default value for GPIO_P_PINLOCKN */
#define _GPIO_P_PINLOCKN_MASK                             0x0000FFFFUL                             /**< Mask for GPIO_P_PINLOCKN */
#define _GPIO_P_PINLOCKN_PINLOCKN_SHIFT                   0                                        /**< Shift value for GPIO_PINLOCKN */
#define _GPIO_P_PINLOCKN_PINLOCKN_MASK                    0xFFFFUL                                 /**< Bit mask for GPIO_PINLOCKN */
#define _GPIO_P_PINLOCKN_PINLOCKN_DEFAULT                 0x0000FFFFUL                             /**< Mode DEFAULT for GPIO_P_PINLOCKN */
#define GPIO_P_PINLOCKN_PINLOCKN_DEFAULT                  (_GPIO_P_PINLOCKN_PINLOCKN_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_P_PINLOCKN */

/* Bit fields for GPIO EXTIPSELL */
#define _GPIO_EXTIPSELL_RESETVALUE                        0x00000000UL                              /**< Default value for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_MASK                              0x77777777UL                              /**< Mask for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL0_SHIFT                   0                                         /**< Shift value for GPIO_EXTIPSEL0 */
#define _GPIO_EXTIPSELL_EXTIPSEL0_MASK                    0x7UL                                     /**< Bit mask for GPIO_EXTIPSEL0 */
#define _GPIO_EXTIPSELL_EXTIPSEL0_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL0_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL0_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL0_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL0_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL0_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL0_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL0_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL0_DEFAULT << 0)  /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL0_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL0_PORTA << 0)    /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL0_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL0_PORTB << 0)    /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL0_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL0_PORTC << 0)    /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL0_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL0_PORTD << 0)    /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL0_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL0_PORTE << 0)    /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL0_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL0_PORTF << 0)    /**< Shifted mode PORTF for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL1_SHIFT                   4                                         /**< Shift value for GPIO_EXTIPSEL1 */
#define _GPIO_EXTIPSELL_EXTIPSEL1_MASK                    0x70UL                                    /**< Bit mask for GPIO_EXTIPSEL1 */
#define _GPIO_EXTIPSELL_EXTIPSEL1_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL1_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL1_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL1_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL1_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL1_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL1_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL1_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL1_DEFAULT << 4)  /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL1_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL1_PORTA << 4)    /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL1_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL1_PORTB << 4)    /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL1_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL1_PORTC << 4)    /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL1_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL1_PORTD << 4)    /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL1_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL1_PORTE << 4)    /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL1_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL1_PORTF << 4)    /**< Shifted mode PORTF for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL2_SHIFT                   8                                         /**< Shift value for GPIO_EXTIPSEL2 */
#define _GPIO_EXTIPSELL_EXTIPSEL2_MASK                    0x700UL                                   /**< Bit mask for GPIO_EXTIPSEL2 */
#define _GPIO_EXTIPSELL_EXTIPSEL2_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL2_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL2_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL2_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL2_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL2_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL2_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL2_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL2_DEFAULT << 8)  /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL2_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL2_PORTA << 8)    /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL2_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL2_PORTB << 8)    /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL2_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL2_PORTC << 8)    /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL2_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL2_PORTD << 8)    /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL2_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL2_PORTE << 8)    /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL2_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL2_PORTF << 8)    /**< Shifted mode PORTF for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL3_SHIFT                   12                                        /**< Shift value for GPIO_EXTIPSEL3 */
#define _GPIO_EXTIPSELL_EXTIPSEL3_MASK                    0x7000UL                                  /**< Bit mask for GPIO_EXTIPSEL3 */
#define _GPIO_EXTIPSELL_EXTIPSEL3_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL3_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL3_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL3_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL3_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL3_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL3_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL3_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL3_DEFAULT << 12) /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL3_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL3_PORTA << 12)   /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL3_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL3_PORTB << 12)   /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL3_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL3_PORTC << 12)   /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL3_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL3_PORTD << 12)   /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL3_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL3_PORTE << 12)   /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL3_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL3_PORTF << 12)   /**< Shifted mode PORTF for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL4_SHIFT                   16                                        /**< Shift value for GPIO_EXTIPSEL4 */
#define _GPIO_EXTIPSELL_EXTIPSEL4_MASK                    0x70000UL                                 /**< Bit mask for GPIO_EXTIPSEL4 */
#define _GPIO_EXTIPSELL_EXTIPSEL4_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL4_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL4_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL4_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL4_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL4_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL4_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL4_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL4_DEFAULT << 16) /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL4_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL4_PORTA << 16)   /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL4_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL4_PORTB << 16)   /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL4_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL4_PORTC << 16)   /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL4_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL4_PORTD << 16)   /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL4_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL4_PORTE << 16)   /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL4_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL4_PORTF << 16)   /**< Shifted mode PORTF for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL5_SHIFT                   20                                        /**< Shift value for GPIO_EXTIPSEL5 */
#define _GPIO_EXTIPSELL_EXTIPSEL5_MASK                    0x700000UL                                /**< Bit mask for GPIO_EXTIPSEL5 */
#define _GPIO_EXTIPSELL_EXTIPSEL5_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL5_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL5_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL5_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL5_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL5_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL5_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL5_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL5_DEFAULT << 20) /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL5_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL5_PORTA << 20)   /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL5_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL5_PORTB << 20)   /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL5_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL5_PORTC << 20)   /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL5_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL5_PORTD << 20)   /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL5_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL5_PORTE << 20)   /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL5_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL5_PORTF << 20)   /**< Shifted mode PORTF for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL6_SHIFT                   24                                        /**< Shift value for GPIO_EXTIPSEL6 */
#define _GPIO_EXTIPSELL_EXTIPSEL6_MASK                    0x7000000UL                               /**< Bit mask for GPIO_EXTIPSEL6 */
#define _GPIO_EXTIPSELL_EXTIPSEL6_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL6_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL6_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL6_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL6_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL6_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL6_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL6_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL6_DEFAULT << 24) /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL6_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL6_PORTA << 24)   /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL6_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL6_PORTB << 24)   /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL6_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL6_PORTC << 24)   /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL6_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL6_PORTD << 24)   /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL6_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL6_PORTE << 24)   /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL6_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL6_PORTF << 24)   /**< Shifted mode PORTF for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL7_SHIFT                   28                                        /**< Shift value for GPIO_EXTIPSEL7 */
#define _GPIO_EXTIPSELL_EXTIPSEL7_MASK                    0x70000000UL                              /**< Bit mask for GPIO_EXTIPSEL7 */
#define _GPIO_EXTIPSELL_EXTIPSEL7_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL7_PORTA                   0x00000000UL                              /**< Mode PORTA for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL7_PORTB                   0x00000001UL                              /**< Mode PORTB for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL7_PORTC                   0x00000002UL                              /**< Mode PORTC for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL7_PORTD                   0x00000003UL                              /**< Mode PORTD for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL7_PORTE                   0x00000004UL                              /**< Mode PORTE for GPIO_EXTIPSELL */
#define _GPIO_EXTIPSELL_EXTIPSEL7_PORTF                   0x00000005UL                              /**< Mode PORTF for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL7_DEFAULT                  (_GPIO_EXTIPSELL_EXTIPSEL7_DEFAULT << 28) /**< Shifted mode DEFAULT for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL7_PORTA                    (_GPIO_EXTIPSELL_EXTIPSEL7_PORTA << 28)   /**< Shifted mode PORTA for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL7_PORTB                    (_GPIO_EXTIPSELL_EXTIPSEL7_PORTB << 28)   /**< Shifted mode PORTB for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL7_PORTC                    (_GPIO_EXTIPSELL_EXTIPSEL7_PORTC << 28)   /**< Shifted mode PORTC for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL7_PORTD                    (_GPIO_EXTIPSELL_EXTIPSEL7_PORTD << 28)   /**< Shifted mode PORTD for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL7_PORTE                    (_GPIO_EXTIPSELL_EXTIPSEL7_PORTE << 28)   /**< Shifted mode PORTE for GPIO_EXTIPSELL */
#define GPIO_EXTIPSELL_EXTIPSEL7_PORTF                    (_GPIO_EXTIPSELL_EXTIPSEL7_PORTF << 28)   /**< Shifted mode PORTF for GPIO_EXTIPSELL */

/* Bit fields for GPIO EXTIPSELH */
#define _GPIO_EXTIPSELH_RESETVALUE                        0x00000000UL                               /**< Default value for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_MASK                              0x77777777UL                               /**< Mask for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL8_SHIFT                   0                                          /**< Shift value for GPIO_EXTIPSEL8 */
#define _GPIO_EXTIPSELH_EXTIPSEL8_MASK                    0x7UL                                      /**< Bit mask for GPIO_EXTIPSEL8 */
#define _GPIO_EXTIPSELH_EXTIPSEL8_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL8_PORTA                   0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL8_PORTB                   0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL8_PORTC                   0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL8_PORTD                   0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL8_PORTE                   0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL8_PORTF                   0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL8_DEFAULT                  (_GPIO_EXTIPSELH_EXTIPSEL8_DEFAULT << 0)   /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL8_PORTA                    (_GPIO_EXTIPSELH_EXTIPSEL8_PORTA << 0)     /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL8_PORTB                    (_GPIO_EXTIPSELH_EXTIPSEL8_PORTB << 0)     /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL8_PORTC                    (_GPIO_EXTIPSELH_EXTIPSEL8_PORTC << 0)     /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL8_PORTD                    (_GPIO_EXTIPSELH_EXTIPSEL8_PORTD << 0)     /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL8_PORTE                    (_GPIO_EXTIPSELH_EXTIPSEL8_PORTE << 0)     /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL8_PORTF                    (_GPIO_EXTIPSELH_EXTIPSEL8_PORTF << 0)     /**< Shifted mode PORTF for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL9_SHIFT                   4                                          /**< Shift value for GPIO_EXTIPSEL9 */
#define _GPIO_EXTIPSELH_EXTIPSEL9_MASK                    0x70UL                                     /**< Bit mask for GPIO_EXTIPSEL9 */
#define _GPIO_EXTIPSELH_EXTIPSEL9_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL9_PORTA                   0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL9_PORTB                   0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL9_PORTC                   0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL9_PORTD                   0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL9_PORTE                   0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL9_PORTF                   0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL9_DEFAULT                  (_GPIO_EXTIPSELH_EXTIPSEL9_DEFAULT << 4)   /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL9_PORTA                    (_GPIO_EXTIPSELH_EXTIPSEL9_PORTA << 4)     /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL9_PORTB                    (_GPIO_EXTIPSELH_EXTIPSEL9_PORTB << 4)     /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL9_PORTC                    (_GPIO_EXTIPSELH_EXTIPSEL9_PORTC << 4)     /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL9_PORTD                    (_GPIO_EXTIPSELH_EXTIPSEL9_PORTD << 4)     /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL9_PORTE                    (_GPIO_EXTIPSELH_EXTIPSEL9_PORTE << 4)     /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL9_PORTF                    (_GPIO_EXTIPSELH_EXTIPSEL9_PORTF << 4)     /**< Shifted mode PORTF for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL10_SHIFT                  8                                          /**< Shift value for GPIO_EXTIPSEL10 */
#define _GPIO_EXTIPSELH_EXTIPSEL10_MASK                   0x700UL                                    /**< Bit mask for GPIO_EXTIPSEL10 */
#define _GPIO_EXTIPSELH_EXTIPSEL10_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL10_PORTA                  0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL10_PORTB                  0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL10_PORTC                  0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL10_PORTD                  0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL10_PORTE                  0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL10_PORTF                  0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL10_DEFAULT                 (_GPIO_EXTIPSELH_EXTIPSEL10_DEFAULT << 8)  /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL10_PORTA                   (_GPIO_EXTIPSELH_EXTIPSEL10_PORTA << 8)    /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL10_PORTB                   (_GPIO_EXTIPSELH_EXTIPSEL10_PORTB << 8)    /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL10_PORTC                   (_GPIO_EXTIPSELH_EXTIPSEL10_PORTC << 8)    /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL10_PORTD                   (_GPIO_EXTIPSELH_EXTIPSEL10_PORTD << 8)    /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL10_PORTE                   (_GPIO_EXTIPSELH_EXTIPSEL10_PORTE << 8)    /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL10_PORTF                   (_GPIO_EXTIPSELH_EXTIPSEL10_PORTF << 8)    /**< Shifted mode PORTF for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL11_SHIFT                  12                                         /**< Shift value for GPIO_EXTIPSEL11 */
#define _GPIO_EXTIPSELH_EXTIPSEL11_MASK                   0x7000UL                                   /**< Bit mask for GPIO_EXTIPSEL11 */
#define _GPIO_EXTIPSELH_EXTIPSEL11_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL11_PORTA                  0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL11_PORTB                  0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL11_PORTC                  0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL11_PORTD                  0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL11_PORTE                  0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL11_PORTF                  0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL11_DEFAULT                 (_GPIO_EXTIPSELH_EXTIPSEL11_DEFAULT << 12) /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL11_PORTA                   (_GPIO_EXTIPSELH_EXTIPSEL11_PORTA << 12)   /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL11_PORTB                   (_GPIO_EXTIPSELH_EXTIPSEL11_PORTB << 12)   /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL11_PORTC                   (_GPIO_EXTIPSELH_EXTIPSEL11_PORTC << 12)   /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL11_PORTD                   (_GPIO_EXTIPSELH_EXTIPSEL11_PORTD << 12)   /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL11_PORTE                   (_GPIO_EXTIPSELH_EXTIPSEL11_PORTE << 12)   /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL11_PORTF                   (_GPIO_EXTIPSELH_EXTIPSEL11_PORTF << 12)   /**< Shifted mode PORTF for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL12_SHIFT                  16                                         /**< Shift value for GPIO_EXTIPSEL12 */
#define _GPIO_EXTIPSELH_EXTIPSEL12_MASK                   0x70000UL                                  /**< Bit mask for GPIO_EXTIPSEL12 */
#define _GPIO_EXTIPSELH_EXTIPSEL12_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL12_PORTA                  0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL12_PORTB                  0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL12_PORTC                  0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL12_PORTD                  0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL12_PORTE                  0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL12_PORTF                  0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL12_DEFAULT                 (_GPIO_EXTIPSELH_EXTIPSEL12_DEFAULT << 16) /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL12_PORTA                   (_GPIO_EXTIPSELH_EXTIPSEL12_PORTA << 16)   /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL12_PORTB                   (_GPIO_EXTIPSELH_EXTIPSEL12_PORTB << 16)   /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL12_PORTC                   (_GPIO_EXTIPSELH_EXTIPSEL12_PORTC << 16)   /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL12_PORTD                   (_GPIO_EXTIPSELH_EXTIPSEL12_PORTD << 16)   /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL12_PORTE                   (_GPIO_EXTIPSELH_EXTIPSEL12_PORTE << 16)   /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL12_PORTF                   (_GPIO_EXTIPSELH_EXTIPSEL12_PORTF << 16)   /**< Shifted mode PORTF for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL13_SHIFT                  20                                         /**< Shift value for GPIO_EXTIPSEL13 */
#define _GPIO_EXTIPSELH_EXTIPSEL13_MASK                   0x700000UL                                 /**< Bit mask for GPIO_EXTIPSEL13 */
#define _GPIO_EXTIPSELH_EXTIPSEL13_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL13_PORTA                  0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL13_PORTB                  0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL13_PORTC                  0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL13_PORTD                  0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL13_PORTE                  0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL13_PORTF                  0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL13_DEFAULT                 (_GPIO_EXTIPSELH_EXTIPSEL13_DEFAULT << 20) /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL13_PORTA                   (_GPIO_EXTIPSELH_EXTIPSEL13_PORTA << 20)   /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL13_PORTB                   (_GPIO_EXTIPSELH_EXTIPSEL13_PORTB << 20)   /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL13_PORTC                   (_GPIO_EXTIPSELH_EXTIPSEL13_PORTC << 20)   /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL13_PORTD                   (_GPIO_EXTIPSELH_EXTIPSEL13_PORTD << 20)   /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL13_PORTE                   (_GPIO_EXTIPSELH_EXTIPSEL13_PORTE << 20)   /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL13_PORTF                   (_GPIO_EXTIPSELH_EXTIPSEL13_PORTF << 20)   /**< Shifted mode PORTF for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL14_SHIFT                  24                                         /**< Shift value for GPIO_EXTIPSEL14 */
#define _GPIO_EXTIPSELH_EXTIPSEL14_MASK                   0x7000000UL                                /**< Bit mask for GPIO_EXTIPSEL14 */
#define _GPIO_EXTIPSELH_EXTIPSEL14_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL14_PORTA                  0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL14_PORTB                  0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL14_PORTC                  0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL14_PORTD                  0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL14_PORTE                  0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL14_PORTF                  0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL14_DEFAULT                 (_GPIO_EXTIPSELH_EXTIPSEL14_DEFAULT << 24) /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL14_PORTA                   (_GPIO_EXTIPSELH_EXTIPSEL14_PORTA << 24)   /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL14_PORTB                   (_GPIO_EXTIPSELH_EXTIPSEL14_PORTB << 24)   /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL14_PORTC                   (_GPIO_EXTIPSELH_EXTIPSEL14_PORTC << 24)   /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL14_PORTD                   (_GPIO_EXTIPSELH_EXTIPSEL14_PORTD << 24)   /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL14_PORTE                   (_GPIO_EXTIPSELH_EXTIPSEL14_PORTE << 24)   /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL14_PORTF                   (_GPIO_EXTIPSELH_EXTIPSEL14_PORTF << 24)   /**< Shifted mode PORTF for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL15_SHIFT                  28                                         /**< Shift value for GPIO_EXTIPSEL15 */
#define _GPIO_EXTIPSELH_EXTIPSEL15_MASK                   0x70000000UL                               /**< Bit mask for GPIO_EXTIPSEL15 */
#define _GPIO_EXTIPSELH_EXTIPSEL15_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL15_PORTA                  0x00000000UL                               /**< Mode PORTA for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL15_PORTB                  0x00000001UL                               /**< Mode PORTB for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL15_PORTC                  0x00000002UL                               /**< Mode PORTC for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL15_PORTD                  0x00000003UL                               /**< Mode PORTD for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL15_PORTE                  0x00000004UL                               /**< Mode PORTE for GPIO_EXTIPSELH */
#define _GPIO_EXTIPSELH_EXTIPSEL15_PORTF                  0x00000005UL                               /**< Mode PORTF for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL15_DEFAULT                 (_GPIO_EXTIPSELH_EXTIPSEL15_DEFAULT << 28) /**< Shifted mode DEFAULT for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL15_PORTA                   (_GPIO_EXTIPSELH_EXTIPSEL15_PORTA << 28)   /**< Shifted mode PORTA for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL15_PORTB                   (_GPIO_EXTIPSELH_EXTIPSEL15_PORTB << 28)   /**< Shifted mode PORTB for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL15_PORTC                   (_GPIO_EXTIPSELH_EXTIPSEL15_PORTC << 28)   /**< Shifted mode PORTC for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL15_PORTD                   (_GPIO_EXTIPSELH_EXTIPSEL15_PORTD << 28)   /**< Shifted mode PORTD for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL15_PORTE                   (_GPIO_EXTIPSELH_EXTIPSEL15_PORTE << 28)   /**< Shifted mode PORTE for GPIO_EXTIPSELH */
#define GPIO_EXTIPSELH_EXTIPSEL15_PORTF                   (_GPIO_EXTIPSELH_EXTIPSEL15_PORTF << 28)   /**< Shifted mode PORTF for GPIO_EXTIPSELH */

/* Bit fields for GPIO EXTIRISE */
#define _GPIO_EXTIRISE_RESETVALUE                         0x00000000UL                           /**< Default value for GPIO_EXTIRISE */
#define _GPIO_EXTIRISE_MASK                               0x0000FFFFUL                           /**< Mask for GPIO_EXTIRISE */
#define _GPIO_EXTIRISE_EXTIRISE_SHIFT                     0                                      /**< Shift value for GPIO_EXTIRISE */
#define _GPIO_EXTIRISE_EXTIRISE_MASK                      0xFFFFUL                               /**< Bit mask for GPIO_EXTIRISE */
#define _GPIO_EXTIRISE_EXTIRISE_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for GPIO_EXTIRISE */
#define GPIO_EXTIRISE_EXTIRISE_DEFAULT                    (_GPIO_EXTIRISE_EXTIRISE_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_EXTIRISE */

/* Bit fields for GPIO EXTIFALL */
#define _GPIO_EXTIFALL_RESETVALUE                         0x00000000UL                           /**< Default value for GPIO_EXTIFALL */
#define _GPIO_EXTIFALL_MASK                               0x0000FFFFUL                           /**< Mask for GPIO_EXTIFALL */
#define _GPIO_EXTIFALL_EXTIFALL_SHIFT                     0                                      /**< Shift value for GPIO_EXTIFALL */
#define _GPIO_EXTIFALL_EXTIFALL_MASK                      0xFFFFUL                               /**< Bit mask for GPIO_EXTIFALL */
#define _GPIO_EXTIFALL_EXTIFALL_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for GPIO_EXTIFALL */
#define GPIO_EXTIFALL_EXTIFALL_DEFAULT                    (_GPIO_EXTIFALL_EXTIFALL_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_EXTIFALL */

/* Bit fields for GPIO IEN */
#define _GPIO_IEN_RESETVALUE                              0x00000000UL                 /**< Default value for GPIO_IEN */
#define _GPIO_IEN_MASK                                    0x0000FFFFUL                 /**< Mask for GPIO_IEN */
#define _GPIO_IEN_EXT_SHIFT                               0                            /**< Shift value for GPIO_EXT */
#define _GPIO_IEN_EXT_MASK                                0xFFFFUL                     /**< Bit mask for GPIO_EXT */
#define _GPIO_IEN_EXT_DEFAULT                             0x00000000UL                 /**< Mode DEFAULT for GPIO_IEN */
#define GPIO_IEN_EXT_DEFAULT                              (_GPIO_IEN_EXT_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_IEN */

/* Bit fields for GPIO IF */
#define _GPIO_IF_RESETVALUE                               0x00000000UL                /**< Default value for GPIO_IF */
#define _GPIO_IF_MASK                                     0x0000FFFFUL                /**< Mask for GPIO_IF */
#define _GPIO_IF_EXT_SHIFT                                0                           /**< Shift value for GPIO_EXT */
#define _GPIO_IF_EXT_MASK                                 0xFFFFUL                    /**< Bit mask for GPIO_EXT */
#define _GPIO_IF_EXT_DEFAULT                              0x00000000UL                /**< Mode DEFAULT for GPIO_IF */
#define GPIO_IF_EXT_DEFAULT                               (_GPIO_IF_EXT_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_IF */

/* Bit fields for GPIO IFS */
#define _GPIO_IFS_RESETVALUE                              0x00000000UL                 /**< Default value for GPIO_IFS */
#define _GPIO_IFS_MASK                                    0x0000FFFFUL                 /**< Mask for GPIO_IFS */
#define _GPIO_IFS_EXT_SHIFT                               0                            /**< Shift value for GPIO_EXT */
#define _GPIO_IFS_EXT_MASK                                0xFFFFUL                     /**< Bit mask for GPIO_EXT */
#define _GPIO_IFS_EXT_DEFAULT                             0x00000000UL                 /**< Mode DEFAULT for GPIO_IFS */
#define GPIO_IFS_EXT_DEFAULT                              (_GPIO_IFS_EXT_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_IFS */

/* Bit fields for GPIO IFC */
#define _GPIO_IFC_RESETVALUE                              0x00000000UL                 /**< Default value for GPIO_IFC */
#define _GPIO_IFC_MASK                                    0x0000FFFFUL                 /**< Mask for GPIO_IFC */
#define _GPIO_IFC_EXT_SHIFT                               0                            /**< Shift value for GPIO_EXT */
#define _GPIO_IFC_EXT_MASK                                0xFFFFUL                     /**< Bit mask for GPIO_EXT */
#define _GPIO_IFC_EXT_DEFAULT                             0x00000000UL                 /**< Mode DEFAULT for GPIO_IFC */
#define GPIO_IFC_EXT_DEFAULT                              (_GPIO_IFC_EXT_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_IFC */

/* Bit fields for GPIO ROUTE */
#define _GPIO_ROUTE_RESETVALUE                            0x00000003UL                            /**< Default value for GPIO_ROUTE */
#define _GPIO_ROUTE_MASK                                  0x0301F307UL                            /**< Mask for GPIO_ROUTE */
#define GPIO_ROUTE_SWCLKPEN                               (0x1UL << 0)                            /**< Serial Wire Clock Pin Enable */
#define _GPIO_ROUTE_SWCLKPEN_SHIFT                        0                                       /**< Shift value for GPIO_SWCLKPEN */
#define _GPIO_ROUTE_SWCLKPEN_MASK                         0x1UL                                   /**< Bit mask for GPIO_SWCLKPEN */
#define _GPIO_ROUTE_SWCLKPEN_DEFAULT                      0x00000001UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_SWCLKPEN_DEFAULT                       (_GPIO_ROUTE_SWCLKPEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_SWDIOPEN                               (0x1UL << 1)                            /**< Serial Wire Data Pin Enable */
#define _GPIO_ROUTE_SWDIOPEN_SHIFT                        1                                       /**< Shift value for GPIO_SWDIOPEN */
#define _GPIO_ROUTE_SWDIOPEN_MASK                         0x2UL                                   /**< Bit mask for GPIO_SWDIOPEN */
#define _GPIO_ROUTE_SWDIOPEN_DEFAULT                      0x00000001UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_SWDIOPEN_DEFAULT                       (_GPIO_ROUTE_SWDIOPEN_DEFAULT << 1)     /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_SWOPEN                                 (0x1UL << 2)                            /**< Serial Wire Viewer Output Pin Enable */
#define _GPIO_ROUTE_SWOPEN_SHIFT                          2                                       /**< Shift value for GPIO_SWOPEN */
#define _GPIO_ROUTE_SWOPEN_MASK                           0x4UL                                   /**< Bit mask for GPIO_SWOPEN */
#define _GPIO_ROUTE_SWOPEN_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_SWOPEN_DEFAULT                         (_GPIO_ROUTE_SWOPEN_DEFAULT << 2)       /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define _GPIO_ROUTE_SWLOCATION_SHIFT                      8                                       /**< Shift value for GPIO_SWLOCATION */
#define _GPIO_ROUTE_SWLOCATION_MASK                       0x300UL                                 /**< Bit mask for GPIO_SWLOCATION */
#define _GPIO_ROUTE_SWLOCATION_LOC0                       0x00000000UL                            /**< Mode LOC0 for GPIO_ROUTE */
#define _GPIO_ROUTE_SWLOCATION_DEFAULT                    0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define _GPIO_ROUTE_SWLOCATION_LOC1                       0x00000001UL                            /**< Mode LOC1 for GPIO_ROUTE */
#define _GPIO_ROUTE_SWLOCATION_LOC2                       0x00000002UL                            /**< Mode LOC2 for GPIO_ROUTE */
#define _GPIO_ROUTE_SWLOCATION_LOC3                       0x00000003UL                            /**< Mode LOC3 for GPIO_ROUTE */
#define GPIO_ROUTE_SWLOCATION_LOC0                        (_GPIO_ROUTE_SWLOCATION_LOC0 << 8)      /**< Shifted mode LOC0 for GPIO_ROUTE */
#define GPIO_ROUTE_SWLOCATION_DEFAULT                     (_GPIO_ROUTE_SWLOCATION_DEFAULT << 8)   /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_SWLOCATION_LOC1                        (_GPIO_ROUTE_SWLOCATION_LOC1 << 8)      /**< Shifted mode LOC1 for GPIO_ROUTE */
#define GPIO_ROUTE_SWLOCATION_LOC2                        (_GPIO_ROUTE_SWLOCATION_LOC2 << 8)      /**< Shifted mode LOC2 for GPIO_ROUTE */
#define GPIO_ROUTE_SWLOCATION_LOC3                        (_GPIO_ROUTE_SWLOCATION_LOC3 << 8)      /**< Shifted mode LOC3 for GPIO_ROUTE */
#define GPIO_ROUTE_TCLKPEN                                (0x1UL << 12)                           /**< ETM Trace Clock Pin Enable */
#define _GPIO_ROUTE_TCLKPEN_SHIFT                         12                                      /**< Shift value for GPIO_TCLKPEN */
#define _GPIO_ROUTE_TCLKPEN_MASK                          0x1000UL                                /**< Bit mask for GPIO_TCLKPEN */
#define _GPIO_ROUTE_TCLKPEN_DEFAULT                       0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TCLKPEN_DEFAULT                        (_GPIO_ROUTE_TCLKPEN_DEFAULT << 12)     /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD0PEN                                 (0x1UL << 13)                           /**< ETM Trace Data Pin Enable */
#define _GPIO_ROUTE_TD0PEN_SHIFT                          13                                      /**< Shift value for GPIO_TD0PEN */
#define _GPIO_ROUTE_TD0PEN_MASK                           0x2000UL                                /**< Bit mask for GPIO_TD0PEN */
#define _GPIO_ROUTE_TD0PEN_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD0PEN_DEFAULT                         (_GPIO_ROUTE_TD0PEN_DEFAULT << 13)      /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD1PEN                                 (0x1UL << 14)                           /**< ETM Trace Data Pin Enable */
#define _GPIO_ROUTE_TD1PEN_SHIFT                          14                                      /**< Shift value for GPIO_TD1PEN */
#define _GPIO_ROUTE_TD1PEN_MASK                           0x4000UL                                /**< Bit mask for GPIO_TD1PEN */
#define _GPIO_ROUTE_TD1PEN_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD1PEN_DEFAULT                         (_GPIO_ROUTE_TD1PEN_DEFAULT << 14)      /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD2PEN                                 (0x1UL << 15)                           /**< ETM Trace Data Pin Enable */
#define _GPIO_ROUTE_TD2PEN_SHIFT                          15                                      /**< Shift value for GPIO_TD2PEN */
#define _GPIO_ROUTE_TD2PEN_MASK                           0x8000UL                                /**< Bit mask for GPIO_TD2PEN */
#define _GPIO_ROUTE_TD2PEN_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD2PEN_DEFAULT                         (_GPIO_ROUTE_TD2PEN_DEFAULT << 15)      /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD3PEN                                 (0x1UL << 16)                           /**< ETM Trace Data Pin Enable */
#define _GPIO_ROUTE_TD3PEN_SHIFT                          16                                      /**< Shift value for GPIO_TD3PEN */
#define _GPIO_ROUTE_TD3PEN_MASK                           0x10000UL                               /**< Bit mask for GPIO_TD3PEN */
#define _GPIO_ROUTE_TD3PEN_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_TD3PEN_DEFAULT                         (_GPIO_ROUTE_TD3PEN_DEFAULT << 16)      /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define _GPIO_ROUTE_ETMLOCATION_SHIFT                     24                                      /**< Shift value for GPIO_ETMLOCATION */
#define _GPIO_ROUTE_ETMLOCATION_MASK                      0x3000000UL                             /**< Bit mask for GPIO_ETMLOCATION */
#define _GPIO_ROUTE_ETMLOCATION_LOC0                      0x00000000UL                            /**< Mode LOC0 for GPIO_ROUTE */
#define _GPIO_ROUTE_ETMLOCATION_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for GPIO_ROUTE */
#define _GPIO_ROUTE_ETMLOCATION_LOC1                      0x00000001UL                            /**< Mode LOC1 for GPIO_ROUTE */
#define _GPIO_ROUTE_ETMLOCATION_LOC2                      0x00000002UL                            /**< Mode LOC2 for GPIO_ROUTE */
#define _GPIO_ROUTE_ETMLOCATION_LOC3                      0x00000003UL                            /**< Mode LOC3 for GPIO_ROUTE */
#define GPIO_ROUTE_ETMLOCATION_LOC0                       (_GPIO_ROUTE_ETMLOCATION_LOC0 << 24)    /**< Shifted mode LOC0 for GPIO_ROUTE */
#define GPIO_ROUTE_ETMLOCATION_DEFAULT                    (_GPIO_ROUTE_ETMLOCATION_DEFAULT << 24) /**< Shifted mode DEFAULT for GPIO_ROUTE */
#define GPIO_ROUTE_ETMLOCATION_LOC1                       (_GPIO_ROUTE_ETMLOCATION_LOC1 << 24)    /**< Shifted mode LOC1 for GPIO_ROUTE */
#define GPIO_ROUTE_ETMLOCATION_LOC2                       (_GPIO_ROUTE_ETMLOCATION_LOC2 << 24)    /**< Shifted mode LOC2 for GPIO_ROUTE */
#define GPIO_ROUTE_ETMLOCATION_LOC3                       (_GPIO_ROUTE_ETMLOCATION_LOC3 << 24)    /**< Shifted mode LOC3 for GPIO_ROUTE */

/* Bit fields for GPIO INSENSE */
#define _GPIO_INSENSE_RESETVALUE                          0x00000003UL                     /**< Default value for GPIO_INSENSE */
#define _GPIO_INSENSE_MASK                                0x00000003UL                     /**< Mask for GPIO_INSENSE */
#define GPIO_INSENSE_INT                                  (0x1UL << 0)                     /**< Interrupt Sense Enable */
#define _GPIO_INSENSE_INT_SHIFT                           0                                /**< Shift value for GPIO_INT */
#define _GPIO_INSENSE_INT_MASK                            0x1UL                            /**< Bit mask for GPIO_INT */
#define _GPIO_INSENSE_INT_DEFAULT                         0x00000001UL                     /**< Mode DEFAULT for GPIO_INSENSE */
#define GPIO_INSENSE_INT_DEFAULT                          (_GPIO_INSENSE_INT_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_INSENSE */
#define GPIO_INSENSE_PRS                                  (0x1UL << 1)                     /**< PRS Sense Enable */
#define _GPIO_INSENSE_PRS_SHIFT                           1                                /**< Shift value for GPIO_PRS */
#define _GPIO_INSENSE_PRS_MASK                            0x2UL                            /**< Bit mask for GPIO_PRS */
#define _GPIO_INSENSE_PRS_DEFAULT                         0x00000001UL                     /**< Mode DEFAULT for GPIO_INSENSE */
#define GPIO_INSENSE_PRS_DEFAULT                          (_GPIO_INSENSE_PRS_DEFAULT << 1) /**< Shifted mode DEFAULT for GPIO_INSENSE */

/* Bit fields for GPIO LOCK */
#define _GPIO_LOCK_RESETVALUE                             0x00000000UL                       /**< Default value for GPIO_LOCK */
#define _GPIO_LOCK_MASK                                   0x0000FFFFUL                       /**< Mask for GPIO_LOCK */
#define _GPIO_LOCK_LOCKKEY_SHIFT                          0                                  /**< Shift value for GPIO_LOCKKEY */
#define _GPIO_LOCK_LOCKKEY_MASK                           0xFFFFUL                           /**< Bit mask for GPIO_LOCKKEY */
#define _GPIO_LOCK_LOCKKEY_DEFAULT                        0x00000000UL                       /**< Mode DEFAULT for GPIO_LOCK */
#define _GPIO_LOCK_LOCKKEY_LOCK                           0x00000000UL                       /**< Mode LOCK for GPIO_LOCK */
#define _GPIO_LOCK_LOCKKEY_UNLOCKED                       0x00000000UL                       /**< Mode UNLOCKED for GPIO_LOCK */
#define _GPIO_LOCK_LOCKKEY_LOCKED                         0x00000001UL                       /**< Mode LOCKED for GPIO_LOCK */
#define _GPIO_LOCK_LOCKKEY_UNLOCK                         0x0000A534UL                       /**< Mode UNLOCK for GPIO_LOCK */
#define GPIO_LOCK_LOCKKEY_DEFAULT                         (_GPIO_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for GPIO_LOCK */
#define GPIO_LOCK_LOCKKEY_LOCK                            (_GPIO_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for GPIO_LOCK */
#define GPIO_LOCK_LOCKKEY_UNLOCKED                        (_GPIO_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for GPIO_LOCK */
#define GPIO_LOCK_LOCKKEY_LOCKED                          (_GPIO_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for GPIO_LOCK */
#define GPIO_LOCK_LOCKKEY_UNLOCK                          (_GPIO_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for GPIO_LOCK */

/* Bit fields for GPIO CTRL */
#define _GPIO_CTRL_RESETVALUE                             0x00000000UL                     /**< Default value for GPIO_CTRL */
#define _GPIO_CTRL_MASK                                   0x00000001UL                     /**< Mask for GPIO_CTRL */
#define GPIO_CTRL_EM4RET                                  (0x1UL << 0)                     /**< Enable EM4 retention */
#define _GPIO_CTRL_EM4RET_SHIFT                           0                                /**< Shift value for GPIO_EM4RET */
#define _GPIO_CTRL_EM4RET_MASK                            0x1UL                            /**< Bit mask for GPIO_EM4RET */
#define _GPIO_CTRL_EM4RET_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for GPIO_CTRL */
#define GPIO_CTRL_EM4RET_DEFAULT                          (_GPIO_CTRL_EM4RET_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_CTRL */

/* Bit fields for GPIO CMD */
#define _GPIO_CMD_RESETVALUE                              0x00000000UL                      /**< Default value for GPIO_CMD */
#define _GPIO_CMD_MASK                                    0x00000001UL                      /**< Mask for GPIO_CMD */
#define GPIO_CMD_EM4WUCLR                                 (0x1UL << 0)                      /**< EM4 Wake-up clear */
#define _GPIO_CMD_EM4WUCLR_SHIFT                          0                                 /**< Shift value for GPIO_EM4WUCLR */
#define _GPIO_CMD_EM4WUCLR_MASK                           0x1UL                             /**< Bit mask for GPIO_EM4WUCLR */
#define _GPIO_CMD_EM4WUCLR_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for GPIO_CMD */
#define GPIO_CMD_EM4WUCLR_DEFAULT                         (_GPIO_CMD_EM4WUCLR_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_CMD */

/* Bit fields for GPIO EM4WUEN */
#define _GPIO_EM4WUEN_RESETVALUE                          0x00000000UL                         /**< Default value for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_MASK                                0x0000003FUL                         /**< Mask for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_SHIFT                       0                                    /**< Shift value for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_MASK                        0x3FUL                               /**< Bit mask for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_A0                          0x00000001UL                         /**< Mode A0 for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_A6                          0x00000002UL                         /**< Mode A6 for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_C9                          0x00000004UL                         /**< Mode C9 for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_F1                          0x00000008UL                         /**< Mode F1 for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_F2                          0x00000010UL                         /**< Mode F2 for GPIO_EM4WUEN */
#define _GPIO_EM4WUEN_EM4WUEN_E13                         0x00000020UL                         /**< Mode E13 for GPIO_EM4WUEN */
#define GPIO_EM4WUEN_EM4WUEN_DEFAULT                      (_GPIO_EM4WUEN_EM4WUEN_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_EM4WUEN */
#define GPIO_EM4WUEN_EM4WUEN_A0                           (_GPIO_EM4WUEN_EM4WUEN_A0 << 0)      /**< Shifted mode A0 for GPIO_EM4WUEN */
#define GPIO_EM4WUEN_EM4WUEN_A6                           (_GPIO_EM4WUEN_EM4WUEN_A6 << 0)      /**< Shifted mode A6 for GPIO_EM4WUEN */
#define GPIO_EM4WUEN_EM4WUEN_C9                           (_GPIO_EM4WUEN_EM4WUEN_C9 << 0)      /**< Shifted mode C9 for GPIO_EM4WUEN */
#define GPIO_EM4WUEN_EM4WUEN_F1                           (_GPIO_EM4WUEN_EM4WUEN_F1 << 0)      /**< Shifted mode F1 for GPIO_EM4WUEN */
#define GPIO_EM4WUEN_EM4WUEN_F2                           (_GPIO_EM4WUEN_EM4WUEN_F2 << 0)      /**< Shifted mode F2 for GPIO_EM4WUEN */
#define GPIO_EM4WUEN_EM4WUEN_E13                          (_GPIO_EM4WUEN_EM4WUEN_E13 << 0)     /**< Shifted mode E13 for GPIO_EM4WUEN */

/* Bit fields for GPIO EM4WUPOL */
#define _GPIO_EM4WUPOL_RESETVALUE                         0x00000000UL                           /**< Default value for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_MASK                               0x0000003FUL                           /**< Mask for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_SHIFT                     0                                      /**< Shift value for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_MASK                      0x3FUL                                 /**< Bit mask for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_A0                        0x00000001UL                           /**< Mode A0 for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_A6                        0x00000002UL                           /**< Mode A6 for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_C9                        0x00000004UL                           /**< Mode C9 for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_F1                        0x00000008UL                           /**< Mode F1 for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_F2                        0x00000010UL                           /**< Mode F2 for GPIO_EM4WUPOL */
#define _GPIO_EM4WUPOL_EM4WUPOL_E13                       0x00000020UL                           /**< Mode E13 for GPIO_EM4WUPOL */
#define GPIO_EM4WUPOL_EM4WUPOL_DEFAULT                    (_GPIO_EM4WUPOL_EM4WUPOL_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_EM4WUPOL */
#define GPIO_EM4WUPOL_EM4WUPOL_A0                         (_GPIO_EM4WUPOL_EM4WUPOL_A0 << 0)      /**< Shifted mode A0 for GPIO_EM4WUPOL */
#define GPIO_EM4WUPOL_EM4WUPOL_A6                         (_GPIO_EM4WUPOL_EM4WUPOL_A6 << 0)      /**< Shifted mode A6 for GPIO_EM4WUPOL */
#define GPIO_EM4WUPOL_EM4WUPOL_C9                         (_GPIO_EM4WUPOL_EM4WUPOL_C9 << 0)      /**< Shifted mode C9 for GPIO_EM4WUPOL */
#define GPIO_EM4WUPOL_EM4WUPOL_F1                         (_GPIO_EM4WUPOL_EM4WUPOL_F1 << 0)      /**< Shifted mode F1 for GPIO_EM4WUPOL */
#define GPIO_EM4WUPOL_EM4WUPOL_F2                         (_GPIO_EM4WUPOL_EM4WUPOL_F2 << 0)      /**< Shifted mode F2 for GPIO_EM4WUPOL */
#define GPIO_EM4WUPOL_EM4WUPOL_E13                        (_GPIO_EM4WUPOL_EM4WUPOL_E13 << 0)     /**< Shifted mode E13 for GPIO_EM4WUPOL */

/* Bit fields for GPIO EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_RESETVALUE                       0x00000000UL                               /**< Default value for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_MASK                             0x0000003FUL                               /**< Mask for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_SHIFT                 0                                          /**< Shift value for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_MASK                  0x3FUL                                     /**< Bit mask for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_DEFAULT               0x00000000UL                               /**< Mode DEFAULT for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_A0                    0x00000001UL                               /**< Mode A0 for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_A6                    0x00000002UL                               /**< Mode A6 for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_C9                    0x00000004UL                               /**< Mode C9 for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_F1                    0x00000008UL                               /**< Mode F1 for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_F2                    0x00000010UL                               /**< Mode F2 for GPIO_EM4WUCAUSE */
#define _GPIO_EM4WUCAUSE_EM4WUCAUSE_E13                   0x00000020UL                               /**< Mode E13 for GPIO_EM4WUCAUSE */
#define GPIO_EM4WUCAUSE_EM4WUCAUSE_DEFAULT                (_GPIO_EM4WUCAUSE_EM4WUCAUSE_DEFAULT << 0) /**< Shifted mode DEFAULT for GPIO_EM4WUCAUSE */
#define GPIO_EM4WUCAUSE_EM4WUCAUSE_A0                     (_GPIO_EM4WUCAUSE_EM4WUCAUSE_A0 << 0)      /**< Shifted mode A0 for GPIO_EM4WUCAUSE */
#define GPIO_EM4WUCAUSE_EM4WUCAUSE_A6                     (_GPIO_EM4WUCAUSE_EM4WUCAUSE_A6 << 0)      /**< Shifted mode A6 for GPIO_EM4WUCAUSE */
#define GPIO_EM4WUCAUSE_EM4WUCAUSE_C9                     (_GPIO_EM4WUCAUSE_EM4WUCAUSE_C9 << 0)      /**< Shifted mode C9 for GPIO_EM4WUCAUSE */
#define GPIO_EM4WUCAUSE_EM4WUCAUSE_F1                     (_GPIO_EM4WUCAUSE_EM4WUCAUSE_F1 << 0)      /**< Shifted mode F1 for GPIO_EM4WUCAUSE */
#define GPIO_EM4WUCAUSE_EM4WUCAUSE_F2                     (_GPIO_EM4WUCAUSE_EM4WUCAUSE_F2 << 0)      /**< Shifted mode F2 for GPIO_EM4WUCAUSE */
#define GPIO_EM4WUCAUSE_EM4WUCAUSE_E13                    (_GPIO_EM4WUCAUSE_EM4WUCAUSE_E13 << 0)     /**< Shifted mode E13 for GPIO_EM4WUCAUSE */

/** @} End of group EFM32WG_GPIO */
/** @} End of group Parts */

