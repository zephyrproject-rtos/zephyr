/**************************************************************************//**
 * @file efm32hg_usb.h
 * @brief EFM32HG_USB register and bit field definitions
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
 * @defgroup EFM32HG_USB
 * @{
 * @brief EFM32HG_USB Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t   CTRL;              /**< System Control Register  */
  __IM uint32_t    STATUS;            /**< System Status Register  */
  __IM uint32_t    IF;                /**< Interrupt Flag Register  */
  __IOM uint32_t   IFS;               /**< Interrupt Flag Set Register  */
  __IOM uint32_t   IFC;               /**< Interrupt Flag Clear Register  */
  __IOM uint32_t   IEN;               /**< Interrupt Enable Register  */
  __IOM uint32_t   ROUTE;             /**< I/O Routing Register  */

  uint32_t         RESERVED0[61435];  /**< Reserved for future use **/
  __IOM uint32_t   GAHBCFG;           /**< AHB Configuration Register  */
  __IOM uint32_t   GUSBCFG;           /**< USB Configuration Register  */
  __IOM uint32_t   GRSTCTL;           /**< Reset Register  */
  __IOM uint32_t   GINTSTS;           /**< Interrupt Register  */
  __IOM uint32_t   GINTMSK;           /**< Interrupt Mask Register  */
  __IM uint32_t    GRXSTSR;           /**< Receive Status Debug Read Register  */
  __IM uint32_t    GRXSTSP;           /**< Receive Status Read and Pop Register  */
  __IOM uint32_t   GRXFSIZ;           /**< Receive FIFO Size Register  */
  __IOM uint32_t   GNPTXFSIZ;         /**< Non-periodic Transmit FIFO Size Register  */

  uint32_t         RESERVED1[12];     /**< Reserved for future use **/
  __IOM uint32_t   GDFIFOCFG;         /**< Global DFIFO Configuration Register  */

  uint32_t         RESERVED2[41];     /**< Reserved for future use **/
  __IOM uint32_t   DIEPTXF1;          /**< Device IN Endpoint Transmit FIFO 1 Size Register  */
  __IOM uint32_t   DIEPTXF2;          /**< Device IN Endpoint Transmit FIFO 2 Size Register  */
  __IOM uint32_t   DIEPTXF3;          /**< Device IN Endpoint Transmit FIFO 3 Size Register  */

  uint32_t         RESERVED3[444];    /**< Reserved for future use **/
  __IOM uint32_t   DCFG;              /**< Device Configuration Register  */
  __IOM uint32_t   DCTL;              /**< Device Control Register  */
  __IM uint32_t    DSTS;              /**< Device Status Register  */
  uint32_t         RESERVED4[1];      /**< Reserved for future use **/
  __IOM uint32_t   DIEPMSK;           /**< Device IN Endpoint Common Interrupt Mask Register  */
  __IOM uint32_t   DOEPMSK;           /**< Device OUT Endpoint Common Interrupt Mask Register  */
  __IM uint32_t    DAINT;             /**< Device All Endpoints Interrupt Register  */
  __IOM uint32_t   DAINTMSK;          /**< Device All Endpoints Interrupt Mask Register  */

  uint32_t         RESERVED5[5];      /**< Reserved for future use **/
  __IOM uint32_t   DIEPEMPMSK;        /**< Device IN Endpoint FIFO Empty Interrupt Mask Register  */

  uint32_t         RESERVED6[50];     /**< Reserved for future use **/
  __IOM uint32_t   DIEP0CTL;          /**< Device IN Endpoint 0 Control Register  */
  uint32_t         RESERVED7[1];      /**< Reserved for future use **/
  __IOM uint32_t   DIEP0INT;          /**< Device IN Endpoint 0 Interrupt Register  */
  uint32_t         RESERVED8[1];      /**< Reserved for future use **/
  __IOM uint32_t   DIEP0TSIZ;         /**< Device IN Endpoint 0 Transfer Size Register  */
  __IOM uint32_t   DIEP0DMAADDR;      /**< Device IN Endpoint 0 DMA Address Register  */
  __IM uint32_t    DIEP0TXFSTS;       /**< Device IN Endpoint 0 Transmit FIFO Status Register  */

  uint32_t         RESERVED9[1];      /**< Reserved registers */
  USB_DIEP_TypeDef DIEP[3];           /**< Device IN Endpoint x+1 Registers */

  uint32_t         RESERVED10[96];    /**< Reserved for future use **/
  __IOM uint32_t   DOEP0CTL;          /**< Device OUT Endpoint 0 Control Register  */
  uint32_t         RESERVED11[1];     /**< Reserved for future use **/
  __IOM uint32_t   DOEP0INT;          /**< Device OUT Endpoint 0 Interrupt Register  */
  uint32_t         RESERVED12[1];     /**< Reserved for future use **/
  __IOM uint32_t   DOEP0TSIZ;         /**< Device OUT Endpoint 0 Transfer Size Register  */
  __IOM uint32_t   DOEP0DMAADDR;      /**< Device OUT Endpoint 0 DMA Address Register  */

  uint32_t         RESERVED13[2];     /**< Reserved registers */
  USB_DOEP_TypeDef DOEP[3];           /**< Device OUT Endpoint x+1 Registers */

  uint32_t         RESERVED14[160];   /**< Reserved for future use **/
  __IOM uint32_t   PCGCCTL;           /**< Power and Clock Gating Control Register  */

  uint32_t         RESERVED15[127];   /**< Reserved registers */
  __IOM uint32_t   FIFO0D[384];       /**< Device EP 0 FIFO  */

  uint32_t         RESERVED16[640];   /**< Reserved registers */
  __IOM uint32_t   FIFO1D[384];       /**< Device EP 1 FIFO  */

  uint32_t         RESERVED17[640];   /**< Reserved registers */
  __IOM uint32_t   FIFO2D[384];       /**< Device EP 2 FIFO  */

  uint32_t         RESERVED18[640];   /**< Reserved registers */
  __IOM uint32_t   FIFO3D[384];       /**< Device EP 3 FIFO  */

  uint32_t         RESERVED19[28288]; /**< Reserved registers */
  __IOM uint32_t   FIFORAM[512];      /**< Direct Access to Data FIFO RAM for Debugging (2 KB)  */
} USB_TypeDef;                        /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_USB_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for USB CTRL */
#define _USB_CTRL_RESETVALUE                       0x00000020UL                           /**< Default value for USB_CTRL */
#define _USB_CTRL_MASK                             0x03330EB2UL                           /**< Mask for USB_CTRL */
#define USB_CTRL_DMPUAP                            (0x1UL << 1)                           /**< DMPU Active Polarity */
#define _USB_CTRL_DMPUAP_SHIFT                     1                                      /**< Shift value for USB_DMPUAP */
#define _USB_CTRL_DMPUAP_MASK                      0x2UL                                  /**< Bit mask for USB_DMPUAP */
#define _USB_CTRL_DMPUAP_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define _USB_CTRL_DMPUAP_LOW                       0x00000000UL                           /**< Mode LOW for USB_CTRL */
#define _USB_CTRL_DMPUAP_HIGH                      0x00000001UL                           /**< Mode HIGH for USB_CTRL */
#define USB_CTRL_DMPUAP_DEFAULT                    (_USB_CTRL_DMPUAP_DEFAULT << 1)        /**< Shifted mode DEFAULT for USB_CTRL */
#define USB_CTRL_DMPUAP_LOW                        (_USB_CTRL_DMPUAP_LOW << 1)            /**< Shifted mode LOW for USB_CTRL */
#define USB_CTRL_DMPUAP_HIGH                       (_USB_CTRL_DMPUAP_HIGH << 1)           /**< Shifted mode HIGH for USB_CTRL */
#define _USB_CTRL_LEMOSCCTRL_SHIFT                 4                                      /**< Shift value for USB_LEMOSCCTRL */
#define _USB_CTRL_LEMOSCCTRL_MASK                  0x30UL                                 /**< Bit mask for USB_LEMOSCCTRL */
#define _USB_CTRL_LEMOSCCTRL_NONE                  0x00000000UL                           /**< Mode NONE for USB_CTRL */
#define _USB_CTRL_LEMOSCCTRL_GATE                  0x00000001UL                           /**< Mode GATE for USB_CTRL */
#define _USB_CTRL_LEMOSCCTRL_DEFAULT               0x00000002UL                           /**< Mode DEFAULT for USB_CTRL */
#define _USB_CTRL_LEMOSCCTRL_SUSPEND               0x00000002UL                           /**< Mode SUSPEND for USB_CTRL */
#define USB_CTRL_LEMOSCCTRL_NONE                   (_USB_CTRL_LEMOSCCTRL_NONE << 4)       /**< Shifted mode NONE for USB_CTRL */
#define USB_CTRL_LEMOSCCTRL_GATE                   (_USB_CTRL_LEMOSCCTRL_GATE << 4)       /**< Shifted mode GATE for USB_CTRL */
#define USB_CTRL_LEMOSCCTRL_DEFAULT                (_USB_CTRL_LEMOSCCTRL_DEFAULT << 4)    /**< Shifted mode DEFAULT for USB_CTRL */
#define USB_CTRL_LEMOSCCTRL_SUSPEND                (_USB_CTRL_LEMOSCCTRL_SUSPEND << 4)    /**< Shifted mode SUSPEND for USB_CTRL */
#define USB_CTRL_LEMPHYCTRL                        (0x1UL << 7)                           /**< Low Energy Mode USB PHY Control */
#define _USB_CTRL_LEMPHYCTRL_SHIFT                 7                                      /**< Shift value for USB_LEMPHYCTRL */
#define _USB_CTRL_LEMPHYCTRL_MASK                  0x80UL                                 /**< Bit mask for USB_LEMPHYCTRL */
#define _USB_CTRL_LEMPHYCTRL_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define _USB_CTRL_LEMPHYCTRL_NONE                  0x00000000UL                           /**< Mode NONE for USB_CTRL */
#define _USB_CTRL_LEMPHYCTRL_LEM                   0x00000001UL                           /**< Mode LEM for USB_CTRL */
#define USB_CTRL_LEMPHYCTRL_DEFAULT                (_USB_CTRL_LEMPHYCTRL_DEFAULT << 7)    /**< Shifted mode DEFAULT for USB_CTRL */
#define USB_CTRL_LEMPHYCTRL_NONE                   (_USB_CTRL_LEMPHYCTRL_NONE << 7)       /**< Shifted mode NONE for USB_CTRL */
#define USB_CTRL_LEMPHYCTRL_LEM                    (_USB_CTRL_LEMPHYCTRL_LEM << 7)        /**< Shifted mode LEM for USB_CTRL */
#define USB_CTRL_LEMIDLEEN                         (0x1UL << 9)                           /**< Low Energy Mode on  Bus Idle Enable */
#define _USB_CTRL_LEMIDLEEN_SHIFT                  9                                      /**< Shift value for USB_LEMIDLEEN */
#define _USB_CTRL_LEMIDLEEN_MASK                   0x200UL                                /**< Bit mask for USB_LEMIDLEEN */
#define _USB_CTRL_LEMIDLEEN_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define USB_CTRL_LEMIDLEEN_DEFAULT                 (_USB_CTRL_LEMIDLEEN_DEFAULT << 9)     /**< Shifted mode DEFAULT for USB_CTRL */
#define USB_CTRL_LEMNAKEN                          (0x1UL << 10)                          /**< Low Energy Mode on OUT NAK Enable */
#define _USB_CTRL_LEMNAKEN_SHIFT                   10                                     /**< Shift value for USB_LEMNAKEN */
#define _USB_CTRL_LEMNAKEN_MASK                    0x400UL                                /**< Bit mask for USB_LEMNAKEN */
#define _USB_CTRL_LEMNAKEN_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define USB_CTRL_LEMNAKEN_DEFAULT                  (_USB_CTRL_LEMNAKEN_DEFAULT << 10)     /**< Shifted mode DEFAULT for USB_CTRL */
#define USB_CTRL_LEMADDRMEN                        (0x1UL << 11)                          /**< Low Energy Mode on Device Address Mismatch Enable */
#define _USB_CTRL_LEMADDRMEN_SHIFT                 11                                     /**< Shift value for USB_LEMADDRMEN */
#define _USB_CTRL_LEMADDRMEN_MASK                  0x800UL                                /**< Bit mask for USB_LEMADDRMEN */
#define _USB_CTRL_LEMADDRMEN_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define USB_CTRL_LEMADDRMEN_DEFAULT                (_USB_CTRL_LEMADDRMEN_DEFAULT << 11)   /**< Shifted mode DEFAULT for USB_CTRL */
#define USB_CTRL_VREGDIS                           (0x1UL << 16)                          /**< Voltage Regulator Disable */
#define _USB_CTRL_VREGDIS_SHIFT                    16                                     /**< Shift value for USB_VREGDIS */
#define _USB_CTRL_VREGDIS_MASK                     0x10000UL                              /**< Bit mask for USB_VREGDIS */
#define _USB_CTRL_VREGDIS_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define USB_CTRL_VREGDIS_DEFAULT                   (_USB_CTRL_VREGDIS_DEFAULT << 16)      /**< Shifted mode DEFAULT for USB_CTRL */
#define USB_CTRL_VREGOSEN                          (0x1UL << 17)                          /**< VREGO Sense Enable */
#define _USB_CTRL_VREGOSEN_SHIFT                   17                                     /**< Shift value for USB_VREGOSEN */
#define _USB_CTRL_VREGOSEN_MASK                    0x20000UL                              /**< Bit mask for USB_VREGOSEN */
#define _USB_CTRL_VREGOSEN_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define USB_CTRL_VREGOSEN_DEFAULT                  (_USB_CTRL_VREGOSEN_DEFAULT << 17)     /**< Shifted mode DEFAULT for USB_CTRL */
#define _USB_CTRL_BIASPROGEM01_SHIFT               20                                     /**< Shift value for USB_BIASPROGEM01 */
#define _USB_CTRL_BIASPROGEM01_MASK                0x300000UL                             /**< Bit mask for USB_BIASPROGEM01 */
#define _USB_CTRL_BIASPROGEM01_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define USB_CTRL_BIASPROGEM01_DEFAULT              (_USB_CTRL_BIASPROGEM01_DEFAULT << 20) /**< Shifted mode DEFAULT for USB_CTRL */
#define _USB_CTRL_BIASPROGEM23_SHIFT               24                                     /**< Shift value for USB_BIASPROGEM23 */
#define _USB_CTRL_BIASPROGEM23_MASK                0x3000000UL                            /**< Bit mask for USB_BIASPROGEM23 */
#define _USB_CTRL_BIASPROGEM23_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for USB_CTRL */
#define USB_CTRL_BIASPROGEM23_DEFAULT              (_USB_CTRL_BIASPROGEM23_DEFAULT << 24) /**< Shifted mode DEFAULT for USB_CTRL */

/* Bit fields for USB STATUS */
#define _USB_STATUS_RESETVALUE                     0x00000000UL                         /**< Default value for USB_STATUS */
#define _USB_STATUS_MASK                           0x00000005UL                         /**< Mask for USB_STATUS */
#define USB_STATUS_VREGOS                          (0x1UL << 0)                         /**< VREGO Sense Output */
#define _USB_STATUS_VREGOS_SHIFT                   0                                    /**< Shift value for USB_VREGOS */
#define _USB_STATUS_VREGOS_MASK                    0x1UL                                /**< Bit mask for USB_VREGOS */
#define _USB_STATUS_VREGOS_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for USB_STATUS */
#define USB_STATUS_VREGOS_DEFAULT                  (_USB_STATUS_VREGOS_DEFAULT << 0)    /**< Shifted mode DEFAULT for USB_STATUS */
#define USB_STATUS_LEMACTIVE                       (0x1UL << 2)                         /**< Low Energy Mode Active */
#define _USB_STATUS_LEMACTIVE_SHIFT                2                                    /**< Shift value for USB_LEMACTIVE */
#define _USB_STATUS_LEMACTIVE_MASK                 0x4UL                                /**< Bit mask for USB_LEMACTIVE */
#define _USB_STATUS_LEMACTIVE_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for USB_STATUS */
#define USB_STATUS_LEMACTIVE_DEFAULT               (_USB_STATUS_LEMACTIVE_DEFAULT << 2) /**< Shifted mode DEFAULT for USB_STATUS */

/* Bit fields for USB IF */
#define _USB_IF_RESETVALUE                         0x00000003UL                   /**< Default value for USB_IF */
#define _USB_IF_MASK                               0x00000003UL                   /**< Mask for USB_IF */
#define USB_IF_VREGOSH                             (0x1UL << 0)                   /**< VREGO Sense High Interrupt Flag */
#define _USB_IF_VREGOSH_SHIFT                      0                              /**< Shift value for USB_VREGOSH */
#define _USB_IF_VREGOSH_MASK                       0x1UL                          /**< Bit mask for USB_VREGOSH */
#define _USB_IF_VREGOSH_DEFAULT                    0x00000001UL                   /**< Mode DEFAULT for USB_IF */
#define USB_IF_VREGOSH_DEFAULT                     (_USB_IF_VREGOSH_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_IF */
#define USB_IF_VREGOSL                             (0x1UL << 1)                   /**< VREGO Sense Low Interrupt Flag */
#define _USB_IF_VREGOSL_SHIFT                      1                              /**< Shift value for USB_VREGOSL */
#define _USB_IF_VREGOSL_MASK                       0x2UL                          /**< Bit mask for USB_VREGOSL */
#define _USB_IF_VREGOSL_DEFAULT                    0x00000001UL                   /**< Mode DEFAULT for USB_IF */
#define USB_IF_VREGOSL_DEFAULT                     (_USB_IF_VREGOSL_DEFAULT << 1) /**< Shifted mode DEFAULT for USB_IF */

/* Bit fields for USB IFS */
#define _USB_IFS_RESETVALUE                        0x00000000UL                    /**< Default value for USB_IFS */
#define _USB_IFS_MASK                              0x00000003UL                    /**< Mask for USB_IFS */
#define USB_IFS_VREGOSH                            (0x1UL << 0)                    /**< Set VREGO Sense High Interrupt Flag */
#define _USB_IFS_VREGOSH_SHIFT                     0                               /**< Shift value for USB_VREGOSH */
#define _USB_IFS_VREGOSH_MASK                      0x1UL                           /**< Bit mask for USB_VREGOSH */
#define _USB_IFS_VREGOSH_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for USB_IFS */
#define USB_IFS_VREGOSH_DEFAULT                    (_USB_IFS_VREGOSH_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_IFS */
#define USB_IFS_VREGOSL                            (0x1UL << 1)                    /**< Set VREGO Sense Low Interrupt Flag */
#define _USB_IFS_VREGOSL_SHIFT                     1                               /**< Shift value for USB_VREGOSL */
#define _USB_IFS_VREGOSL_MASK                      0x2UL                           /**< Bit mask for USB_VREGOSL */
#define _USB_IFS_VREGOSL_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for USB_IFS */
#define USB_IFS_VREGOSL_DEFAULT                    (_USB_IFS_VREGOSL_DEFAULT << 1) /**< Shifted mode DEFAULT for USB_IFS */

/* Bit fields for USB IFC */
#define _USB_IFC_RESETVALUE                        0x00000000UL                    /**< Default value for USB_IFC */
#define _USB_IFC_MASK                              0x00000003UL                    /**< Mask for USB_IFC */
#define USB_IFC_VREGOSH                            (0x1UL << 0)                    /**< Clear VREGO Sense High Interrupt Flag */
#define _USB_IFC_VREGOSH_SHIFT                     0                               /**< Shift value for USB_VREGOSH */
#define _USB_IFC_VREGOSH_MASK                      0x1UL                           /**< Bit mask for USB_VREGOSH */
#define _USB_IFC_VREGOSH_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for USB_IFC */
#define USB_IFC_VREGOSH_DEFAULT                    (_USB_IFC_VREGOSH_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_IFC */
#define USB_IFC_VREGOSL                            (0x1UL << 1)                    /**< Clear VREGO Sense Low Interrupt Flag */
#define _USB_IFC_VREGOSL_SHIFT                     1                               /**< Shift value for USB_VREGOSL */
#define _USB_IFC_VREGOSL_MASK                      0x2UL                           /**< Bit mask for USB_VREGOSL */
#define _USB_IFC_VREGOSL_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for USB_IFC */
#define USB_IFC_VREGOSL_DEFAULT                    (_USB_IFC_VREGOSL_DEFAULT << 1) /**< Shifted mode DEFAULT for USB_IFC */

/* Bit fields for USB IEN */
#define _USB_IEN_RESETVALUE                        0x00000000UL                    /**< Default value for USB_IEN */
#define _USB_IEN_MASK                              0x00000003UL                    /**< Mask for USB_IEN */
#define USB_IEN_VREGOSH                            (0x1UL << 0)                    /**< VREGO Sense High Interrupt Enable */
#define _USB_IEN_VREGOSH_SHIFT                     0                               /**< Shift value for USB_VREGOSH */
#define _USB_IEN_VREGOSH_MASK                      0x1UL                           /**< Bit mask for USB_VREGOSH */
#define _USB_IEN_VREGOSH_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for USB_IEN */
#define USB_IEN_VREGOSH_DEFAULT                    (_USB_IEN_VREGOSH_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_IEN */
#define USB_IEN_VREGOSL                            (0x1UL << 1)                    /**< VREGO Sense Low Interrupt Enable */
#define _USB_IEN_VREGOSL_SHIFT                     1                               /**< Shift value for USB_VREGOSL */
#define _USB_IEN_VREGOSL_MASK                      0x2UL                           /**< Bit mask for USB_VREGOSL */
#define _USB_IEN_VREGOSL_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for USB_IEN */
#define USB_IEN_VREGOSL_DEFAULT                    (_USB_IEN_VREGOSL_DEFAULT << 1) /**< Shifted mode DEFAULT for USB_IEN */

/* Bit fields for USB ROUTE */
#define _USB_ROUTE_RESETVALUE                      0x00000000UL                      /**< Default value for USB_ROUTE */
#define _USB_ROUTE_MASK                            0x00000005UL                      /**< Mask for USB_ROUTE */
#define USB_ROUTE_PHYPEN                           (0x1UL << 0)                      /**< USB PHY Pin Enable */
#define _USB_ROUTE_PHYPEN_SHIFT                    0                                 /**< Shift value for USB_PHYPEN */
#define _USB_ROUTE_PHYPEN_MASK                     0x1UL                             /**< Bit mask for USB_PHYPEN */
#define _USB_ROUTE_PHYPEN_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USB_ROUTE */
#define USB_ROUTE_PHYPEN_DEFAULT                   (_USB_ROUTE_PHYPEN_DEFAULT << 0)  /**< Shifted mode DEFAULT for USB_ROUTE */
#define USB_ROUTE_DMPUPEN                          (0x1UL << 2)                      /**< DMPU Pin Enable */
#define _USB_ROUTE_DMPUPEN_SHIFT                   2                                 /**< Shift value for USB_DMPUPEN */
#define _USB_ROUTE_DMPUPEN_MASK                    0x4UL                             /**< Bit mask for USB_DMPUPEN */
#define _USB_ROUTE_DMPUPEN_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USB_ROUTE */
#define USB_ROUTE_DMPUPEN_DEFAULT                  (_USB_ROUTE_DMPUPEN_DEFAULT << 2) /**< Shifted mode DEFAULT for USB_ROUTE */

/* Bit fields for USB GAHBCFG */
#define _USB_GAHBCFG_RESETVALUE                    0x00000000UL                                /**< Default value for USB_GAHBCFG */
#define _USB_GAHBCFG_MASK                          0x00E000BFUL                                /**< Mask for USB_GAHBCFG */
#define USB_GAHBCFG_GLBLINTRMSK                    (0x1UL << 0)                                /**< Global Interrupt Mask */
#define _USB_GAHBCFG_GLBLINTRMSK_SHIFT             0                                           /**< Shift value for USB_GLBLINTRMSK */
#define _USB_GAHBCFG_GLBLINTRMSK_MASK              0x1UL                                       /**< Bit mask for USB_GLBLINTRMSK */
#define _USB_GAHBCFG_GLBLINTRMSK_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_GLBLINTRMSK_DEFAULT            (_USB_GAHBCFG_GLBLINTRMSK_DEFAULT << 0)     /**< Shifted mode DEFAULT for USB_GAHBCFG */
#define _USB_GAHBCFG_HBSTLEN_SHIFT                 1                                           /**< Shift value for USB_HBSTLEN */
#define _USB_GAHBCFG_HBSTLEN_MASK                  0x1EUL                                      /**< Bit mask for USB_HBSTLEN */
#define _USB_GAHBCFG_HBSTLEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for USB_GAHBCFG */
#define _USB_GAHBCFG_HBSTLEN_SINGLE                0x00000000UL                                /**< Mode SINGLE for USB_GAHBCFG */
#define _USB_GAHBCFG_HBSTLEN_INCR                  0x00000001UL                                /**< Mode INCR for USB_GAHBCFG */
#define _USB_GAHBCFG_HBSTLEN_INCR4                 0x00000003UL                                /**< Mode INCR4 for USB_GAHBCFG */
#define _USB_GAHBCFG_HBSTLEN_INCR8                 0x00000005UL                                /**< Mode INCR8 for USB_GAHBCFG */
#define _USB_GAHBCFG_HBSTLEN_INCR16                0x00000007UL                                /**< Mode INCR16 for USB_GAHBCFG */
#define USB_GAHBCFG_HBSTLEN_DEFAULT                (_USB_GAHBCFG_HBSTLEN_DEFAULT << 1)         /**< Shifted mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_HBSTLEN_SINGLE                 (_USB_GAHBCFG_HBSTLEN_SINGLE << 1)          /**< Shifted mode SINGLE for USB_GAHBCFG */
#define USB_GAHBCFG_HBSTLEN_INCR                   (_USB_GAHBCFG_HBSTLEN_INCR << 1)            /**< Shifted mode INCR for USB_GAHBCFG */
#define USB_GAHBCFG_HBSTLEN_INCR4                  (_USB_GAHBCFG_HBSTLEN_INCR4 << 1)           /**< Shifted mode INCR4 for USB_GAHBCFG */
#define USB_GAHBCFG_HBSTLEN_INCR8                  (_USB_GAHBCFG_HBSTLEN_INCR8 << 1)           /**< Shifted mode INCR8 for USB_GAHBCFG */
#define USB_GAHBCFG_HBSTLEN_INCR16                 (_USB_GAHBCFG_HBSTLEN_INCR16 << 1)          /**< Shifted mode INCR16 for USB_GAHBCFG */
#define USB_GAHBCFG_DMAEN                          (0x1UL << 5)                                /**< DMA Enable */
#define _USB_GAHBCFG_DMAEN_SHIFT                   5                                           /**< Shift value for USB_DMAEN */
#define _USB_GAHBCFG_DMAEN_MASK                    0x20UL                                      /**< Bit mask for USB_DMAEN */
#define _USB_GAHBCFG_DMAEN_DEFAULT                 0x00000000UL                                /**< Mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_DMAEN_DEFAULT                  (_USB_GAHBCFG_DMAEN_DEFAULT << 5)           /**< Shifted mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_NPTXFEMPLVL                    (0x1UL << 7)                                /**< Non-Periodic TxFIFO Empty Level */
#define _USB_GAHBCFG_NPTXFEMPLVL_SHIFT             7                                           /**< Shift value for USB_NPTXFEMPLVL */
#define _USB_GAHBCFG_NPTXFEMPLVL_MASK              0x80UL                                      /**< Bit mask for USB_NPTXFEMPLVL */
#define _USB_GAHBCFG_NPTXFEMPLVL_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for USB_GAHBCFG */
#define _USB_GAHBCFG_NPTXFEMPLVL_HALFEMPTY         0x00000000UL                                /**< Mode HALFEMPTY for USB_GAHBCFG */
#define _USB_GAHBCFG_NPTXFEMPLVL_EMPTY             0x00000001UL                                /**< Mode EMPTY for USB_GAHBCFG */
#define USB_GAHBCFG_NPTXFEMPLVL_DEFAULT            (_USB_GAHBCFG_NPTXFEMPLVL_DEFAULT << 7)     /**< Shifted mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_NPTXFEMPLVL_HALFEMPTY          (_USB_GAHBCFG_NPTXFEMPLVL_HALFEMPTY << 7)   /**< Shifted mode HALFEMPTY for USB_GAHBCFG */
#define USB_GAHBCFG_NPTXFEMPLVL_EMPTY              (_USB_GAHBCFG_NPTXFEMPLVL_EMPTY << 7)       /**< Shifted mode EMPTY for USB_GAHBCFG */
#define USB_GAHBCFG_REMMEMSUPP                     (0x1UL << 21)                               /**< Remote Memory Support */
#define _USB_GAHBCFG_REMMEMSUPP_SHIFT              21                                          /**< Shift value for USB_REMMEMSUPP */
#define _USB_GAHBCFG_REMMEMSUPP_MASK               0x200000UL                                  /**< Bit mask for USB_REMMEMSUPP */
#define _USB_GAHBCFG_REMMEMSUPP_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_REMMEMSUPP_DEFAULT             (_USB_GAHBCFG_REMMEMSUPP_DEFAULT << 21)     /**< Shifted mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_NOTIALLDMAWRIT                 (0x1UL << 22)                               /**< Notify All DMA Writes */
#define _USB_GAHBCFG_NOTIALLDMAWRIT_SHIFT          22                                          /**< Shift value for USB_NOTIALLDMAWRIT */
#define _USB_GAHBCFG_NOTIALLDMAWRIT_MASK           0x400000UL                                  /**< Bit mask for USB_NOTIALLDMAWRIT */
#define _USB_GAHBCFG_NOTIALLDMAWRIT_DEFAULT        0x00000000UL                                /**< Mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_NOTIALLDMAWRIT_DEFAULT         (_USB_GAHBCFG_NOTIALLDMAWRIT_DEFAULT << 22) /**< Shifted mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_AHBSINGLE                      (0x1UL << 23)                               /**< AHB Single Support */
#define _USB_GAHBCFG_AHBSINGLE_SHIFT               23                                          /**< Shift value for USB_AHBSINGLE */
#define _USB_GAHBCFG_AHBSINGLE_MASK                0x800000UL                                  /**< Bit mask for USB_AHBSINGLE */
#define _USB_GAHBCFG_AHBSINGLE_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for USB_GAHBCFG */
#define USB_GAHBCFG_AHBSINGLE_DEFAULT              (_USB_GAHBCFG_AHBSINGLE_DEFAULT << 23)      /**< Shifted mode DEFAULT for USB_GAHBCFG */

/* Bit fields for USB GUSBCFG */
#define _USB_GUSBCFG_RESETVALUE                    0x00001440UL                                /**< Default value for USB_GUSBCFG */
#define _USB_GUSBCFG_MASK                          0x90403C27UL                                /**< Mask for USB_GUSBCFG */
#define _USB_GUSBCFG_TOUTCAL_SHIFT                 0                                           /**< Shift value for USB_TOUTCAL */
#define _USB_GUSBCFG_TOUTCAL_MASK                  0x7UL                                       /**< Bit mask for USB_TOUTCAL */
#define _USB_GUSBCFG_TOUTCAL_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_TOUTCAL_DEFAULT                (_USB_GUSBCFG_TOUTCAL_DEFAULT << 0)         /**< Shifted mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_FSINTF                         (0x1UL << 5)                                /**< Full-Speed Serial Interface Select */
#define _USB_GUSBCFG_FSINTF_SHIFT                  5                                           /**< Shift value for USB_FSINTF */
#define _USB_GUSBCFG_FSINTF_MASK                   0x20UL                                      /**< Bit mask for USB_FSINTF */
#define _USB_GUSBCFG_FSINTF_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_FSINTF_DEFAULT                 (_USB_GUSBCFG_FSINTF_DEFAULT << 5)          /**< Shifted mode DEFAULT for USB_GUSBCFG */
#define _USB_GUSBCFG_USBTRDTIM_SHIFT               10                                          /**< Shift value for USB_USBTRDTIM */
#define _USB_GUSBCFG_USBTRDTIM_MASK                0x3C00UL                                    /**< Bit mask for USB_USBTRDTIM */
#define _USB_GUSBCFG_USBTRDTIM_DEFAULT             0x00000005UL                                /**< Mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_USBTRDTIM_DEFAULT              (_USB_GUSBCFG_USBTRDTIM_DEFAULT << 10)      /**< Shifted mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_TERMSELDLPULSE                 (0x1UL << 22)                               /**< TermSel DLine Pulsing Selection */
#define _USB_GUSBCFG_TERMSELDLPULSE_SHIFT          22                                          /**< Shift value for USB_TERMSELDLPULSE */
#define _USB_GUSBCFG_TERMSELDLPULSE_MASK           0x400000UL                                  /**< Bit mask for USB_TERMSELDLPULSE */
#define _USB_GUSBCFG_TERMSELDLPULSE_DEFAULT        0x00000000UL                                /**< Mode DEFAULT for USB_GUSBCFG */
#define _USB_GUSBCFG_TERMSELDLPULSE_TXVALID        0x00000000UL                                /**< Mode TXVALID for USB_GUSBCFG */
#define _USB_GUSBCFG_TERMSELDLPULSE_TERMSEL        0x00000001UL                                /**< Mode TERMSEL for USB_GUSBCFG */
#define USB_GUSBCFG_TERMSELDLPULSE_DEFAULT         (_USB_GUSBCFG_TERMSELDLPULSE_DEFAULT << 22) /**< Shifted mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_TERMSELDLPULSE_TXVALID         (_USB_GUSBCFG_TERMSELDLPULSE_TXVALID << 22) /**< Shifted mode TXVALID for USB_GUSBCFG */
#define USB_GUSBCFG_TERMSELDLPULSE_TERMSEL         (_USB_GUSBCFG_TERMSELDLPULSE_TERMSEL << 22) /**< Shifted mode TERMSEL for USB_GUSBCFG */
#define USB_GUSBCFG_TXENDDELAY                     (0x1UL << 28)                               /**< Tx End Delay */
#define _USB_GUSBCFG_TXENDDELAY_SHIFT              28                                          /**< Shift value for USB_TXENDDELAY */
#define _USB_GUSBCFG_TXENDDELAY_MASK               0x10000000UL                                /**< Bit mask for USB_TXENDDELAY */
#define _USB_GUSBCFG_TXENDDELAY_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_TXENDDELAY_DEFAULT             (_USB_GUSBCFG_TXENDDELAY_DEFAULT << 28)     /**< Shifted mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_CORRUPTTXPKT                   (0x1UL << 31)                               /**< Corrupt Tx packet */
#define _USB_GUSBCFG_CORRUPTTXPKT_SHIFT            31                                          /**< Shift value for USB_CORRUPTTXPKT */
#define _USB_GUSBCFG_CORRUPTTXPKT_MASK             0x80000000UL                                /**< Bit mask for USB_CORRUPTTXPKT */
#define _USB_GUSBCFG_CORRUPTTXPKT_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_GUSBCFG */
#define USB_GUSBCFG_CORRUPTTXPKT_DEFAULT           (_USB_GUSBCFG_CORRUPTTXPKT_DEFAULT << 31)   /**< Shifted mode DEFAULT for USB_GUSBCFG */

/* Bit fields for USB GRSTCTL */
#define _USB_GRSTCTL_RESETVALUE                    0x80000000UL                            /**< Default value for USB_GRSTCTL */
#define _USB_GRSTCTL_MASK                          0xC00007F3UL                            /**< Mask for USB_GRSTCTL */
#define USB_GRSTCTL_CSFTRST                        (0x1UL << 0)                            /**< Core Soft Reset */
#define _USB_GRSTCTL_CSFTRST_SHIFT                 0                                       /**< Shift value for USB_CSFTRST */
#define _USB_GRSTCTL_CSFTRST_MASK                  0x1UL                                   /**< Bit mask for USB_CSFTRST */
#define _USB_GRSTCTL_CSFTRST_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_CSFTRST_DEFAULT                (_USB_GRSTCTL_CSFTRST_DEFAULT << 0)     /**< Shifted mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_PIUFSSFTRST                    (0x1UL << 1)                            /**< PIU FS Dedicated Controller Soft Reset */
#define _USB_GRSTCTL_PIUFSSFTRST_SHIFT             1                                       /**< Shift value for USB_PIUFSSFTRST */
#define _USB_GRSTCTL_PIUFSSFTRST_MASK              0x2UL                                   /**< Bit mask for USB_PIUFSSFTRST */
#define _USB_GRSTCTL_PIUFSSFTRST_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_PIUFSSFTRST_DEFAULT            (_USB_GRSTCTL_PIUFSSFTRST_DEFAULT << 1) /**< Shifted mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_RXFFLSH                        (0x1UL << 4)                            /**< RxFIFO Flush */
#define _USB_GRSTCTL_RXFFLSH_SHIFT                 4                                       /**< Shift value for USB_RXFFLSH */
#define _USB_GRSTCTL_RXFFLSH_MASK                  0x10UL                                  /**< Bit mask for USB_RXFFLSH */
#define _USB_GRSTCTL_RXFFLSH_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_RXFFLSH_DEFAULT                (_USB_GRSTCTL_RXFFLSH_DEFAULT << 4)     /**< Shifted mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_TXFFLSH                        (0x1UL << 5)                            /**< TxFIFO Flush */
#define _USB_GRSTCTL_TXFFLSH_SHIFT                 5                                       /**< Shift value for USB_TXFFLSH */
#define _USB_GRSTCTL_TXFFLSH_MASK                  0x20UL                                  /**< Bit mask for USB_TXFFLSH */
#define _USB_GRSTCTL_TXFFLSH_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_TXFFLSH_DEFAULT                (_USB_GRSTCTL_TXFFLSH_DEFAULT << 5)     /**< Shifted mode DEFAULT for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_SHIFT                  6                                       /**< Shift value for USB_TXFNUM */
#define _USB_GRSTCTL_TXFNUM_MASK                   0x7C0UL                                 /**< Bit mask for USB_TXFNUM */
#define _USB_GRSTCTL_TXFNUM_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_F0                     0x00000000UL                            /**< Mode F0 for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_F1                     0x00000001UL                            /**< Mode F1 for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_F2                     0x00000002UL                            /**< Mode F2 for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_F3                     0x00000003UL                            /**< Mode F3 for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_F4                     0x00000004UL                            /**< Mode F4 for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_F5                     0x00000005UL                            /**< Mode F5 for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_F6                     0x00000006UL                            /**< Mode F6 for USB_GRSTCTL */
#define _USB_GRSTCTL_TXFNUM_FALL                   0x00000010UL                            /**< Mode FALL for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_DEFAULT                 (_USB_GRSTCTL_TXFNUM_DEFAULT << 6)      /**< Shifted mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_F0                      (_USB_GRSTCTL_TXFNUM_F0 << 6)           /**< Shifted mode F0 for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_F1                      (_USB_GRSTCTL_TXFNUM_F1 << 6)           /**< Shifted mode F1 for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_F2                      (_USB_GRSTCTL_TXFNUM_F2 << 6)           /**< Shifted mode F2 for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_F3                      (_USB_GRSTCTL_TXFNUM_F3 << 6)           /**< Shifted mode F3 for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_F4                      (_USB_GRSTCTL_TXFNUM_F4 << 6)           /**< Shifted mode F4 for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_F5                      (_USB_GRSTCTL_TXFNUM_F5 << 6)           /**< Shifted mode F5 for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_F6                      (_USB_GRSTCTL_TXFNUM_F6 << 6)           /**< Shifted mode F6 for USB_GRSTCTL */
#define USB_GRSTCTL_TXFNUM_FALL                    (_USB_GRSTCTL_TXFNUM_FALL << 6)         /**< Shifted mode FALL for USB_GRSTCTL */
#define USB_GRSTCTL_DMAREQ                         (0x1UL << 30)                           /**< DMA Request Signal */
#define _USB_GRSTCTL_DMAREQ_SHIFT                  30                                      /**< Shift value for USB_DMAREQ */
#define _USB_GRSTCTL_DMAREQ_MASK                   0x40000000UL                            /**< Bit mask for USB_DMAREQ */
#define _USB_GRSTCTL_DMAREQ_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_DMAREQ_DEFAULT                 (_USB_GRSTCTL_DMAREQ_DEFAULT << 30)     /**< Shifted mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_AHBIDLE                        (0x1UL << 31)                           /**< AHB Master Idle */
#define _USB_GRSTCTL_AHBIDLE_SHIFT                 31                                      /**< Shift value for USB_AHBIDLE */
#define _USB_GRSTCTL_AHBIDLE_MASK                  0x80000000UL                            /**< Bit mask for USB_AHBIDLE */
#define _USB_GRSTCTL_AHBIDLE_DEFAULT               0x00000001UL                            /**< Mode DEFAULT for USB_GRSTCTL */
#define USB_GRSTCTL_AHBIDLE_DEFAULT                (_USB_GRSTCTL_AHBIDLE_DEFAULT << 31)    /**< Shifted mode DEFAULT for USB_GRSTCTL */

/* Bit fields for USB GINTSTS */
#define _USB_GINTSTS_RESETVALUE                    0x00000000UL                             /**< Default value for USB_GINTSTS */
#define _USB_GINTSTS_MASK                          0x80FCFCD9UL                             /**< Mask for USB_GINTSTS */
#define USB_GINTSTS_CURMOD                         (0x1UL << 0)                             /**< Current Mode of Operation */
#define _USB_GINTSTS_CURMOD_SHIFT                  0                                        /**< Shift value for USB_CURMOD */
#define _USB_GINTSTS_CURMOD_MASK                   0x1UL                                    /**< Bit mask for USB_CURMOD */
#define _USB_GINTSTS_CURMOD_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define _USB_GINTSTS_CURMOD_DEVICE                 0x00000000UL                             /**< Mode DEVICE for USB_GINTSTS */
#define USB_GINTSTS_CURMOD_DEFAULT                 (_USB_GINTSTS_CURMOD_DEFAULT << 0)       /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_CURMOD_DEVICE                  (_USB_GINTSTS_CURMOD_DEVICE << 0)        /**< Shifted mode DEVICE for USB_GINTSTS */
#define USB_GINTSTS_SOF                            (0x1UL << 3)                             /**< Start of Frame */
#define _USB_GINTSTS_SOF_SHIFT                     3                                        /**< Shift value for USB_SOF */
#define _USB_GINTSTS_SOF_MASK                      0x8UL                                    /**< Bit mask for USB_SOF */
#define _USB_GINTSTS_SOF_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_SOF_DEFAULT                    (_USB_GINTSTS_SOF_DEFAULT << 3)          /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_RXFLVL                         (0x1UL << 4)                             /**< RxFIFO Non-Empty */
#define _USB_GINTSTS_RXFLVL_SHIFT                  4                                        /**< Shift value for USB_RXFLVL */
#define _USB_GINTSTS_RXFLVL_MASK                   0x10UL                                   /**< Bit mask for USB_RXFLVL */
#define _USB_GINTSTS_RXFLVL_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_RXFLVL_DEFAULT                 (_USB_GINTSTS_RXFLVL_DEFAULT << 4)       /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_GINNAKEFF                      (0x1UL << 6)                             /**< Global IN Non-periodic NAK Effective */
#define _USB_GINTSTS_GINNAKEFF_SHIFT               6                                        /**< Shift value for USB_GINNAKEFF */
#define _USB_GINTSTS_GINNAKEFF_MASK                0x40UL                                   /**< Bit mask for USB_GINNAKEFF */
#define _USB_GINTSTS_GINNAKEFF_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_GINNAKEFF_DEFAULT              (_USB_GINTSTS_GINNAKEFF_DEFAULT << 6)    /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_GOUTNAKEFF                     (0x1UL << 7)                             /**< Global OUT NAK Effective */
#define _USB_GINTSTS_GOUTNAKEFF_SHIFT              7                                        /**< Shift value for USB_GOUTNAKEFF */
#define _USB_GINTSTS_GOUTNAKEFF_MASK               0x80UL                                   /**< Bit mask for USB_GOUTNAKEFF */
#define _USB_GINTSTS_GOUTNAKEFF_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_GOUTNAKEFF_DEFAULT             (_USB_GINTSTS_GOUTNAKEFF_DEFAULT << 7)   /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_ERLYSUSP                       (0x1UL << 10)                            /**< Early Suspend */
#define _USB_GINTSTS_ERLYSUSP_SHIFT                10                                       /**< Shift value for USB_ERLYSUSP */
#define _USB_GINTSTS_ERLYSUSP_MASK                 0x400UL                                  /**< Bit mask for USB_ERLYSUSP */
#define _USB_GINTSTS_ERLYSUSP_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_ERLYSUSP_DEFAULT               (_USB_GINTSTS_ERLYSUSP_DEFAULT << 10)    /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_USBSUSP                        (0x1UL << 11)                            /**< USB Suspend */
#define _USB_GINTSTS_USBSUSP_SHIFT                 11                                       /**< Shift value for USB_USBSUSP */
#define _USB_GINTSTS_USBSUSP_MASK                  0x800UL                                  /**< Bit mask for USB_USBSUSP */
#define _USB_GINTSTS_USBSUSP_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_USBSUSP_DEFAULT                (_USB_GINTSTS_USBSUSP_DEFAULT << 11)     /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_USBRST                         (0x1UL << 12)                            /**< USB Reset */
#define _USB_GINTSTS_USBRST_SHIFT                  12                                       /**< Shift value for USB_USBRST */
#define _USB_GINTSTS_USBRST_MASK                   0x1000UL                                 /**< Bit mask for USB_USBRST */
#define _USB_GINTSTS_USBRST_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_USBRST_DEFAULT                 (_USB_GINTSTS_USBRST_DEFAULT << 12)      /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_ENUMDONE                       (0x1UL << 13)                            /**< Enumeration Done */
#define _USB_GINTSTS_ENUMDONE_SHIFT                13                                       /**< Shift value for USB_ENUMDONE */
#define _USB_GINTSTS_ENUMDONE_MASK                 0x2000UL                                 /**< Bit mask for USB_ENUMDONE */
#define _USB_GINTSTS_ENUMDONE_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_ENUMDONE_DEFAULT               (_USB_GINTSTS_ENUMDONE_DEFAULT << 13)    /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_ISOOUTDROP                     (0x1UL << 14)                            /**< Isochronous OUT Packet Dropped Interrupt */
#define _USB_GINTSTS_ISOOUTDROP_SHIFT              14                                       /**< Shift value for USB_ISOOUTDROP */
#define _USB_GINTSTS_ISOOUTDROP_MASK               0x4000UL                                 /**< Bit mask for USB_ISOOUTDROP */
#define _USB_GINTSTS_ISOOUTDROP_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_ISOOUTDROP_DEFAULT             (_USB_GINTSTS_ISOOUTDROP_DEFAULT << 14)  /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_EOPF                           (0x1UL << 15)                            /**< End of Periodic Frame Interrupt */
#define _USB_GINTSTS_EOPF_SHIFT                    15                                       /**< Shift value for USB_EOPF */
#define _USB_GINTSTS_EOPF_MASK                     0x8000UL                                 /**< Bit mask for USB_EOPF */
#define _USB_GINTSTS_EOPF_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_EOPF_DEFAULT                   (_USB_GINTSTS_EOPF_DEFAULT << 15)        /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_IEPINT                         (0x1UL << 18)                            /**< IN Endpoints Interrupt */
#define _USB_GINTSTS_IEPINT_SHIFT                  18                                       /**< Shift value for USB_IEPINT */
#define _USB_GINTSTS_IEPINT_MASK                   0x40000UL                                /**< Bit mask for USB_IEPINT */
#define _USB_GINTSTS_IEPINT_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_IEPINT_DEFAULT                 (_USB_GINTSTS_IEPINT_DEFAULT << 18)      /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_OEPINT                         (0x1UL << 19)                            /**< OUT Endpoints Interrupt */
#define _USB_GINTSTS_OEPINT_SHIFT                  19                                       /**< Shift value for USB_OEPINT */
#define _USB_GINTSTS_OEPINT_MASK                   0x80000UL                                /**< Bit mask for USB_OEPINT */
#define _USB_GINTSTS_OEPINT_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_OEPINT_DEFAULT                 (_USB_GINTSTS_OEPINT_DEFAULT << 19)      /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_INCOMPISOIN                    (0x1UL << 20)                            /**< Incomplete Isochronous IN Transfer */
#define _USB_GINTSTS_INCOMPISOIN_SHIFT             20                                       /**< Shift value for USB_INCOMPISOIN */
#define _USB_GINTSTS_INCOMPISOIN_MASK              0x100000UL                               /**< Bit mask for USB_INCOMPISOIN */
#define _USB_GINTSTS_INCOMPISOIN_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_INCOMPISOIN_DEFAULT            (_USB_GINTSTS_INCOMPISOIN_DEFAULT << 20) /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_INCOMPLP                       (0x1UL << 21)                            /**< Incomplete Periodic Transfer */
#define _USB_GINTSTS_INCOMPLP_SHIFT                21                                       /**< Shift value for USB_INCOMPLP */
#define _USB_GINTSTS_INCOMPLP_MASK                 0x200000UL                               /**< Bit mask for USB_INCOMPLP */
#define _USB_GINTSTS_INCOMPLP_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_INCOMPLP_DEFAULT               (_USB_GINTSTS_INCOMPLP_DEFAULT << 21)    /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_FETSUSP                        (0x1UL << 22)                            /**< Data Fetch Suspended */
#define _USB_GINTSTS_FETSUSP_SHIFT                 22                                       /**< Shift value for USB_FETSUSP */
#define _USB_GINTSTS_FETSUSP_MASK                  0x400000UL                               /**< Bit mask for USB_FETSUSP */
#define _USB_GINTSTS_FETSUSP_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_FETSUSP_DEFAULT                (_USB_GINTSTS_FETSUSP_DEFAULT << 22)     /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_RESETDET                       (0x1UL << 23)                            /**< Reset detected Interrupt */
#define _USB_GINTSTS_RESETDET_SHIFT                23                                       /**< Shift value for USB_RESETDET */
#define _USB_GINTSTS_RESETDET_MASK                 0x800000UL                               /**< Bit mask for USB_RESETDET */
#define _USB_GINTSTS_RESETDET_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_RESETDET_DEFAULT               (_USB_GINTSTS_RESETDET_DEFAULT << 23)    /**< Shifted mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_WKUPINT                        (0x1UL << 31)                            /**< Resume/Remote Wakeup Detected Interrupt */
#define _USB_GINTSTS_WKUPINT_SHIFT                 31                                       /**< Shift value for USB_WKUPINT */
#define _USB_GINTSTS_WKUPINT_MASK                  0x80000000UL                             /**< Bit mask for USB_WKUPINT */
#define _USB_GINTSTS_WKUPINT_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_GINTSTS */
#define USB_GINTSTS_WKUPINT_DEFAULT                (_USB_GINTSTS_WKUPINT_DEFAULT << 31)     /**< Shifted mode DEFAULT for USB_GINTSTS */

/* Bit fields for USB GINTMSK */
#define _USB_GINTMSK_RESETVALUE                    0x00000000UL                                /**< Default value for USB_GINTMSK */
#define _USB_GINTMSK_MASK                          0x80FCFCDAUL                                /**< Mask for USB_GINTMSK */
#define USB_GINTMSK_MODEMISMSK                     (0x1UL << 1)                                /**< Mode Mismatch Interrupt Mask */
#define _USB_GINTMSK_MODEMISMSK_SHIFT              1                                           /**< Shift value for USB_MODEMISMSK */
#define _USB_GINTMSK_MODEMISMSK_MASK               0x2UL                                       /**< Bit mask for USB_MODEMISMSK */
#define _USB_GINTMSK_MODEMISMSK_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_MODEMISMSK_DEFAULT             (_USB_GINTMSK_MODEMISMSK_DEFAULT << 1)      /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_SOFMSK                         (0x1UL << 3)                                /**< Start of Frame Mask */
#define _USB_GINTMSK_SOFMSK_SHIFT                  3                                           /**< Shift value for USB_SOFMSK */
#define _USB_GINTMSK_SOFMSK_MASK                   0x8UL                                       /**< Bit mask for USB_SOFMSK */
#define _USB_GINTMSK_SOFMSK_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_SOFMSK_DEFAULT                 (_USB_GINTMSK_SOFMSK_DEFAULT << 3)          /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_RXFLVLMSK                      (0x1UL << 4)                                /**< Receive FIFO Non-Empty Mask */
#define _USB_GINTMSK_RXFLVLMSK_SHIFT               4                                           /**< Shift value for USB_RXFLVLMSK */
#define _USB_GINTMSK_RXFLVLMSK_MASK                0x10UL                                      /**< Bit mask for USB_RXFLVLMSK */
#define _USB_GINTMSK_RXFLVLMSK_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_RXFLVLMSK_DEFAULT              (_USB_GINTMSK_RXFLVLMSK_DEFAULT << 4)       /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_GINNAKEFFMSK                   (0x1UL << 6)                                /**< Global Non-periodic IN NAK Effective Mask */
#define _USB_GINTMSK_GINNAKEFFMSK_SHIFT            6                                           /**< Shift value for USB_GINNAKEFFMSK */
#define _USB_GINTMSK_GINNAKEFFMSK_MASK             0x40UL                                      /**< Bit mask for USB_GINNAKEFFMSK */
#define _USB_GINTMSK_GINNAKEFFMSK_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_GINNAKEFFMSK_DEFAULT           (_USB_GINTMSK_GINNAKEFFMSK_DEFAULT << 6)    /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_GOUTNAKEFFMSK                  (0x1UL << 7)                                /**< Global OUT NAK Effective Mask */
#define _USB_GINTMSK_GOUTNAKEFFMSK_SHIFT           7                                           /**< Shift value for USB_GOUTNAKEFFMSK */
#define _USB_GINTMSK_GOUTNAKEFFMSK_MASK            0x80UL                                      /**< Bit mask for USB_GOUTNAKEFFMSK */
#define _USB_GINTMSK_GOUTNAKEFFMSK_DEFAULT         0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_GOUTNAKEFFMSK_DEFAULT          (_USB_GINTMSK_GOUTNAKEFFMSK_DEFAULT << 7)   /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_ERLYSUSPMSK                    (0x1UL << 10)                               /**< Early Suspend Mask */
#define _USB_GINTMSK_ERLYSUSPMSK_SHIFT             10                                          /**< Shift value for USB_ERLYSUSPMSK */
#define _USB_GINTMSK_ERLYSUSPMSK_MASK              0x400UL                                     /**< Bit mask for USB_ERLYSUSPMSK */
#define _USB_GINTMSK_ERLYSUSPMSK_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_ERLYSUSPMSK_DEFAULT            (_USB_GINTMSK_ERLYSUSPMSK_DEFAULT << 10)    /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_USBSUSPMSK                     (0x1UL << 11)                               /**< USB Suspend Mask */
#define _USB_GINTMSK_USBSUSPMSK_SHIFT              11                                          /**< Shift value for USB_USBSUSPMSK */
#define _USB_GINTMSK_USBSUSPMSK_MASK               0x800UL                                     /**< Bit mask for USB_USBSUSPMSK */
#define _USB_GINTMSK_USBSUSPMSK_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_USBSUSPMSK_DEFAULT             (_USB_GINTMSK_USBSUSPMSK_DEFAULT << 11)     /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_USBRSTMSK                      (0x1UL << 12)                               /**< USB Reset Mask */
#define _USB_GINTMSK_USBRSTMSK_SHIFT               12                                          /**< Shift value for USB_USBRSTMSK */
#define _USB_GINTMSK_USBRSTMSK_MASK                0x1000UL                                    /**< Bit mask for USB_USBRSTMSK */
#define _USB_GINTMSK_USBRSTMSK_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_USBRSTMSK_DEFAULT              (_USB_GINTMSK_USBRSTMSK_DEFAULT << 12)      /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_ENUMDONEMSK                    (0x1UL << 13)                               /**< Enumeration Done Mask */
#define _USB_GINTMSK_ENUMDONEMSK_SHIFT             13                                          /**< Shift value for USB_ENUMDONEMSK */
#define _USB_GINTMSK_ENUMDONEMSK_MASK              0x2000UL                                    /**< Bit mask for USB_ENUMDONEMSK */
#define _USB_GINTMSK_ENUMDONEMSK_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_ENUMDONEMSK_DEFAULT            (_USB_GINTMSK_ENUMDONEMSK_DEFAULT << 13)    /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_ISOOUTDROPMSK                  (0x1UL << 14)                               /**< Isochronous OUT Packet Dropped Interrupt Mask */
#define _USB_GINTMSK_ISOOUTDROPMSK_SHIFT           14                                          /**< Shift value for USB_ISOOUTDROPMSK */
#define _USB_GINTMSK_ISOOUTDROPMSK_MASK            0x4000UL                                    /**< Bit mask for USB_ISOOUTDROPMSK */
#define _USB_GINTMSK_ISOOUTDROPMSK_DEFAULT         0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_ISOOUTDROPMSK_DEFAULT          (_USB_GINTMSK_ISOOUTDROPMSK_DEFAULT << 14)  /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_EOPFMSK                        (0x1UL << 15)                               /**< End of Periodic Frame Interrupt Mask */
#define _USB_GINTMSK_EOPFMSK_SHIFT                 15                                          /**< Shift value for USB_EOPFMSK */
#define _USB_GINTMSK_EOPFMSK_MASK                  0x8000UL                                    /**< Bit mask for USB_EOPFMSK */
#define _USB_GINTMSK_EOPFMSK_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_EOPFMSK_DEFAULT                (_USB_GINTMSK_EOPFMSK_DEFAULT << 15)        /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_IEPINTMSK                      (0x1UL << 18)                               /**< IN Endpoints Interrupt Mask */
#define _USB_GINTMSK_IEPINTMSK_SHIFT               18                                          /**< Shift value for USB_IEPINTMSK */
#define _USB_GINTMSK_IEPINTMSK_MASK                0x40000UL                                   /**< Bit mask for USB_IEPINTMSK */
#define _USB_GINTMSK_IEPINTMSK_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_IEPINTMSK_DEFAULT              (_USB_GINTMSK_IEPINTMSK_DEFAULT << 18)      /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_OEPINTMSK                      (0x1UL << 19)                               /**< OUT Endpoints Interrupt Mask */
#define _USB_GINTMSK_OEPINTMSK_SHIFT               19                                          /**< Shift value for USB_OEPINTMSK */
#define _USB_GINTMSK_OEPINTMSK_MASK                0x80000UL                                   /**< Bit mask for USB_OEPINTMSK */
#define _USB_GINTMSK_OEPINTMSK_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_OEPINTMSK_DEFAULT              (_USB_GINTMSK_OEPINTMSK_DEFAULT << 19)      /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_INCOMPISOINMSK                 (0x1UL << 20)                               /**< Incomplete Isochronous IN Transfer Mask */
#define _USB_GINTMSK_INCOMPISOINMSK_SHIFT          20                                          /**< Shift value for USB_INCOMPISOINMSK */
#define _USB_GINTMSK_INCOMPISOINMSK_MASK           0x100000UL                                  /**< Bit mask for USB_INCOMPISOINMSK */
#define _USB_GINTMSK_INCOMPISOINMSK_DEFAULT        0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_INCOMPISOINMSK_DEFAULT         (_USB_GINTMSK_INCOMPISOINMSK_DEFAULT << 20) /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_INCOMPLPMSK                    (0x1UL << 21)                               /**< Incomplete Periodic Transfer Mask */
#define _USB_GINTMSK_INCOMPLPMSK_SHIFT             21                                          /**< Shift value for USB_INCOMPLPMSK */
#define _USB_GINTMSK_INCOMPLPMSK_MASK              0x200000UL                                  /**< Bit mask for USB_INCOMPLPMSK */
#define _USB_GINTMSK_INCOMPLPMSK_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_INCOMPLPMSK_DEFAULT            (_USB_GINTMSK_INCOMPLPMSK_DEFAULT << 21)    /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_FETSUSPMSK                     (0x1UL << 22)                               /**< Data Fetch Suspended Mask */
#define _USB_GINTMSK_FETSUSPMSK_SHIFT              22                                          /**< Shift value for USB_FETSUSPMSK */
#define _USB_GINTMSK_FETSUSPMSK_MASK               0x400000UL                                  /**< Bit mask for USB_FETSUSPMSK */
#define _USB_GINTMSK_FETSUSPMSK_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_FETSUSPMSK_DEFAULT             (_USB_GINTMSK_FETSUSPMSK_DEFAULT << 22)     /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_RESETDETMSK                    (0x1UL << 23)                               /**< Reset detected Interrupt Mask */
#define _USB_GINTMSK_RESETDETMSK_SHIFT             23                                          /**< Shift value for USB_RESETDETMSK */
#define _USB_GINTMSK_RESETDETMSK_MASK              0x800000UL                                  /**< Bit mask for USB_RESETDETMSK */
#define _USB_GINTMSK_RESETDETMSK_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_RESETDETMSK_DEFAULT            (_USB_GINTMSK_RESETDETMSK_DEFAULT << 23)    /**< Shifted mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_WKUPINTMSK                     (0x1UL << 31)                               /**< Resume/Remote Wakeup Detected Interrupt Mask */
#define _USB_GINTMSK_WKUPINTMSK_SHIFT              31                                          /**< Shift value for USB_WKUPINTMSK */
#define _USB_GINTMSK_WKUPINTMSK_MASK               0x80000000UL                                /**< Bit mask for USB_WKUPINTMSK */
#define _USB_GINTMSK_WKUPINTMSK_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_GINTMSK */
#define USB_GINTMSK_WKUPINTMSK_DEFAULT             (_USB_GINTMSK_WKUPINTMSK_DEFAULT << 31)     /**< Shifted mode DEFAULT for USB_GINTMSK */

/* Bit fields for USB GRXSTSR */
#define _USB_GRXSTSR_RESETVALUE                    0x00000000UL                           /**< Default value for USB_GRXSTSR */
#define _USB_GRXSTSR_MASK                          0x01FFFFFFUL                           /**< Mask for USB_GRXSTSR */
#define _USB_GRXSTSR_CHEPNUM_SHIFT                 0                                      /**< Shift value for USB_CHEPNUM */
#define _USB_GRXSTSR_CHEPNUM_MASK                  0xFUL                                  /**< Bit mask for USB_CHEPNUM */
#define _USB_GRXSTSR_CHEPNUM_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSR */
#define USB_GRXSTSR_CHEPNUM_DEFAULT                (_USB_GRXSTSR_CHEPNUM_DEFAULT << 0)    /**< Shifted mode DEFAULT for USB_GRXSTSR */
#define _USB_GRXSTSR_BCNT_SHIFT                    4                                      /**< Shift value for USB_BCNT */
#define _USB_GRXSTSR_BCNT_MASK                     0x7FF0UL                               /**< Bit mask for USB_BCNT */
#define _USB_GRXSTSR_BCNT_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSR */
#define USB_GRXSTSR_BCNT_DEFAULT                   (_USB_GRXSTSR_BCNT_DEFAULT << 4)       /**< Shifted mode DEFAULT for USB_GRXSTSR */
#define _USB_GRXSTSR_DPID_SHIFT                    15                                     /**< Shift value for USB_DPID */
#define _USB_GRXSTSR_DPID_MASK                     0x18000UL                              /**< Bit mask for USB_DPID */
#define _USB_GRXSTSR_DPID_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSR */
#define _USB_GRXSTSR_DPID_DATA0                    0x00000000UL                           /**< Mode DATA0 for USB_GRXSTSR */
#define _USB_GRXSTSR_DPID_DATA1                    0x00000001UL                           /**< Mode DATA1 for USB_GRXSTSR */
#define _USB_GRXSTSR_DPID_DATA2                    0x00000002UL                           /**< Mode DATA2 for USB_GRXSTSR */
#define _USB_GRXSTSR_DPID_MDATA                    0x00000003UL                           /**< Mode MDATA for USB_GRXSTSR */
#define USB_GRXSTSR_DPID_DEFAULT                   (_USB_GRXSTSR_DPID_DEFAULT << 15)      /**< Shifted mode DEFAULT for USB_GRXSTSR */
#define USB_GRXSTSR_DPID_DATA0                     (_USB_GRXSTSR_DPID_DATA0 << 15)        /**< Shifted mode DATA0 for USB_GRXSTSR */
#define USB_GRXSTSR_DPID_DATA1                     (_USB_GRXSTSR_DPID_DATA1 << 15)        /**< Shifted mode DATA1 for USB_GRXSTSR */
#define USB_GRXSTSR_DPID_DATA2                     (_USB_GRXSTSR_DPID_DATA2 << 15)        /**< Shifted mode DATA2 for USB_GRXSTSR */
#define USB_GRXSTSR_DPID_MDATA                     (_USB_GRXSTSR_DPID_MDATA << 15)        /**< Shifted mode MDATA for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_SHIFT                  17                                     /**< Shift value for USB_PKTSTS */
#define _USB_GRXSTSR_PKTSTS_MASK                   0x1E0000UL                             /**< Bit mask for USB_PKTSTS */
#define _USB_GRXSTSR_PKTSTS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_GOUTNAK                0x00000001UL                           /**< Mode GOUTNAK for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_PKTRCV                 0x00000002UL                           /**< Mode PKTRCV for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_XFERCOMPL              0x00000003UL                           /**< Mode XFERCOMPL for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_SETUPCOMPL             0x00000004UL                           /**< Mode SETUPCOMPL for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_TGLERR                 0x00000005UL                           /**< Mode TGLERR for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_SETUPRCV               0x00000006UL                           /**< Mode SETUPRCV for USB_GRXSTSR */
#define _USB_GRXSTSR_PKTSTS_CHLT                   0x00000007UL                           /**< Mode CHLT for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_DEFAULT                 (_USB_GRXSTSR_PKTSTS_DEFAULT << 17)    /**< Shifted mode DEFAULT for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_GOUTNAK                 (_USB_GRXSTSR_PKTSTS_GOUTNAK << 17)    /**< Shifted mode GOUTNAK for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_PKTRCV                  (_USB_GRXSTSR_PKTSTS_PKTRCV << 17)     /**< Shifted mode PKTRCV for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_XFERCOMPL               (_USB_GRXSTSR_PKTSTS_XFERCOMPL << 17)  /**< Shifted mode XFERCOMPL for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_SETUPCOMPL              (_USB_GRXSTSR_PKTSTS_SETUPCOMPL << 17) /**< Shifted mode SETUPCOMPL for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_TGLERR                  (_USB_GRXSTSR_PKTSTS_TGLERR << 17)     /**< Shifted mode TGLERR for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_SETUPRCV                (_USB_GRXSTSR_PKTSTS_SETUPRCV << 17)   /**< Shifted mode SETUPRCV for USB_GRXSTSR */
#define USB_GRXSTSR_PKTSTS_CHLT                    (_USB_GRXSTSR_PKTSTS_CHLT << 17)       /**< Shifted mode CHLT for USB_GRXSTSR */
#define _USB_GRXSTSR_FN_SHIFT                      21                                     /**< Shift value for USB_FN */
#define _USB_GRXSTSR_FN_MASK                       0x1E00000UL                            /**< Bit mask for USB_FN */
#define _USB_GRXSTSR_FN_DEFAULT                    0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSR */
#define USB_GRXSTSR_FN_DEFAULT                     (_USB_GRXSTSR_FN_DEFAULT << 21)        /**< Shifted mode DEFAULT for USB_GRXSTSR */

/* Bit fields for USB GRXSTSP */
#define _USB_GRXSTSP_RESETVALUE                    0x00000000UL                           /**< Default value for USB_GRXSTSP */
#define _USB_GRXSTSP_MASK                          0x01FFFFFFUL                           /**< Mask for USB_GRXSTSP */
#define _USB_GRXSTSP_CHEPNUM_SHIFT                 0                                      /**< Shift value for USB_CHEPNUM */
#define _USB_GRXSTSP_CHEPNUM_MASK                  0xFUL                                  /**< Bit mask for USB_CHEPNUM */
#define _USB_GRXSTSP_CHEPNUM_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSP */
#define USB_GRXSTSP_CHEPNUM_DEFAULT                (_USB_GRXSTSP_CHEPNUM_DEFAULT << 0)    /**< Shifted mode DEFAULT for USB_GRXSTSP */
#define _USB_GRXSTSP_BCNT_SHIFT                    4                                      /**< Shift value for USB_BCNT */
#define _USB_GRXSTSP_BCNT_MASK                     0x7FF0UL                               /**< Bit mask for USB_BCNT */
#define _USB_GRXSTSP_BCNT_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSP */
#define USB_GRXSTSP_BCNT_DEFAULT                   (_USB_GRXSTSP_BCNT_DEFAULT << 4)       /**< Shifted mode DEFAULT for USB_GRXSTSP */
#define _USB_GRXSTSP_DPID_SHIFT                    15                                     /**< Shift value for USB_DPID */
#define _USB_GRXSTSP_DPID_MASK                     0x18000UL                              /**< Bit mask for USB_DPID */
#define _USB_GRXSTSP_DPID_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSP */
#define _USB_GRXSTSP_DPID_DATA0                    0x00000000UL                           /**< Mode DATA0 for USB_GRXSTSP */
#define _USB_GRXSTSP_DPID_DATA1                    0x00000001UL                           /**< Mode DATA1 for USB_GRXSTSP */
#define _USB_GRXSTSP_DPID_DATA2                    0x00000002UL                           /**< Mode DATA2 for USB_GRXSTSP */
#define _USB_GRXSTSP_DPID_MDATA                    0x00000003UL                           /**< Mode MDATA for USB_GRXSTSP */
#define USB_GRXSTSP_DPID_DEFAULT                   (_USB_GRXSTSP_DPID_DEFAULT << 15)      /**< Shifted mode DEFAULT for USB_GRXSTSP */
#define USB_GRXSTSP_DPID_DATA0                     (_USB_GRXSTSP_DPID_DATA0 << 15)        /**< Shifted mode DATA0 for USB_GRXSTSP */
#define USB_GRXSTSP_DPID_DATA1                     (_USB_GRXSTSP_DPID_DATA1 << 15)        /**< Shifted mode DATA1 for USB_GRXSTSP */
#define USB_GRXSTSP_DPID_DATA2                     (_USB_GRXSTSP_DPID_DATA2 << 15)        /**< Shifted mode DATA2 for USB_GRXSTSP */
#define USB_GRXSTSP_DPID_MDATA                     (_USB_GRXSTSP_DPID_MDATA << 15)        /**< Shifted mode MDATA for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_SHIFT                  17                                     /**< Shift value for USB_PKTSTS */
#define _USB_GRXSTSP_PKTSTS_MASK                   0x1E0000UL                             /**< Bit mask for USB_PKTSTS */
#define _USB_GRXSTSP_PKTSTS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_GOUTNAK                0x00000001UL                           /**< Mode GOUTNAK for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_PKTRCV                 0x00000002UL                           /**< Mode PKTRCV for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_XFERCOMPL              0x00000003UL                           /**< Mode XFERCOMPL for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_SETUPCOMPL             0x00000004UL                           /**< Mode SETUPCOMPL for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_TGLERR                 0x00000005UL                           /**< Mode TGLERR for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_SETUPRCV               0x00000006UL                           /**< Mode SETUPRCV for USB_GRXSTSP */
#define _USB_GRXSTSP_PKTSTS_CHLT                   0x00000007UL                           /**< Mode CHLT for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_DEFAULT                 (_USB_GRXSTSP_PKTSTS_DEFAULT << 17)    /**< Shifted mode DEFAULT for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_GOUTNAK                 (_USB_GRXSTSP_PKTSTS_GOUTNAK << 17)    /**< Shifted mode GOUTNAK for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_PKTRCV                  (_USB_GRXSTSP_PKTSTS_PKTRCV << 17)     /**< Shifted mode PKTRCV for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_XFERCOMPL               (_USB_GRXSTSP_PKTSTS_XFERCOMPL << 17)  /**< Shifted mode XFERCOMPL for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_SETUPCOMPL              (_USB_GRXSTSP_PKTSTS_SETUPCOMPL << 17) /**< Shifted mode SETUPCOMPL for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_TGLERR                  (_USB_GRXSTSP_PKTSTS_TGLERR << 17)     /**< Shifted mode TGLERR for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_SETUPRCV                (_USB_GRXSTSP_PKTSTS_SETUPRCV << 17)   /**< Shifted mode SETUPRCV for USB_GRXSTSP */
#define USB_GRXSTSP_PKTSTS_CHLT                    (_USB_GRXSTSP_PKTSTS_CHLT << 17)       /**< Shifted mode CHLT for USB_GRXSTSP */
#define _USB_GRXSTSP_FN_SHIFT                      21                                     /**< Shift value for USB_FN */
#define _USB_GRXSTSP_FN_MASK                       0x1E00000UL                            /**< Bit mask for USB_FN */
#define _USB_GRXSTSP_FN_DEFAULT                    0x00000000UL                           /**< Mode DEFAULT for USB_GRXSTSP */
#define USB_GRXSTSP_FN_DEFAULT                     (_USB_GRXSTSP_FN_DEFAULT << 21)        /**< Shifted mode DEFAULT for USB_GRXSTSP */

/* Bit fields for USB GRXFSIZ */
#define _USB_GRXFSIZ_RESETVALUE                    0x00000200UL                       /**< Default value for USB_GRXFSIZ */
#define _USB_GRXFSIZ_MASK                          0x000003FFUL                       /**< Mask for USB_GRXFSIZ */
#define _USB_GRXFSIZ_RXFDEP_SHIFT                  0                                  /**< Shift value for USB_RXFDEP */
#define _USB_GRXFSIZ_RXFDEP_MASK                   0x3FFUL                            /**< Bit mask for USB_RXFDEP */
#define _USB_GRXFSIZ_RXFDEP_DEFAULT                0x00000200UL                       /**< Mode DEFAULT for USB_GRXFSIZ */
#define USB_GRXFSIZ_RXFDEP_DEFAULT                 (_USB_GRXFSIZ_RXFDEP_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_GRXFSIZ */

/* Bit fields for USB GNPTXFSIZ */
#define _USB_GNPTXFSIZ_RESETVALUE                  0x02000200UL                                    /**< Default value for USB_GNPTXFSIZ */
#define _USB_GNPTXFSIZ_MASK                        0xFFFF03FFUL                                    /**< Mask for USB_GNPTXFSIZ */
#define _USB_GNPTXFSIZ_NPTXFSTADDR_SHIFT           0                                               /**< Shift value for USB_NPTXFSTADDR */
#define _USB_GNPTXFSIZ_NPTXFSTADDR_MASK            0x3FFUL                                         /**< Bit mask for USB_NPTXFSTADDR */
#define _USB_GNPTXFSIZ_NPTXFSTADDR_DEFAULT         0x00000200UL                                    /**< Mode DEFAULT for USB_GNPTXFSIZ */
#define USB_GNPTXFSIZ_NPTXFSTADDR_DEFAULT          (_USB_GNPTXFSIZ_NPTXFSTADDR_DEFAULT << 0)       /**< Shifted mode DEFAULT for USB_GNPTXFSIZ */
#define _USB_GNPTXFSIZ_NPTXFINEPTXF0DEP_SHIFT      16                                              /**< Shift value for USB_NPTXFINEPTXF0DEP */
#define _USB_GNPTXFSIZ_NPTXFINEPTXF0DEP_MASK       0xFFFF0000UL                                    /**< Bit mask for USB_NPTXFINEPTXF0DEP */
#define _USB_GNPTXFSIZ_NPTXFINEPTXF0DEP_DEFAULT    0x00000200UL                                    /**< Mode DEFAULT for USB_GNPTXFSIZ */
#define USB_GNPTXFSIZ_NPTXFINEPTXF0DEP_DEFAULT     (_USB_GNPTXFSIZ_NPTXFINEPTXF0DEP_DEFAULT << 16) /**< Shifted mode DEFAULT for USB_GNPTXFSIZ */

/* Bit fields for USB GDFIFOCFG */
#define _USB_GDFIFOCFG_RESETVALUE                  0x05F80600UL                                  /**< Default value for USB_GDFIFOCFG */
#define _USB_GDFIFOCFG_MASK                        0xFFFFFFFFUL                                  /**< Mask for USB_GDFIFOCFG */
#define _USB_GDFIFOCFG_GDFIFOCFG_SHIFT             0                                             /**< Shift value for USB_GDFIFOCFG */
#define _USB_GDFIFOCFG_GDFIFOCFG_MASK              0xFFFFUL                                      /**< Bit mask for USB_GDFIFOCFG */
#define _USB_GDFIFOCFG_GDFIFOCFG_DEFAULT           0x00000600UL                                  /**< Mode DEFAULT for USB_GDFIFOCFG */
#define USB_GDFIFOCFG_GDFIFOCFG_DEFAULT            (_USB_GDFIFOCFG_GDFIFOCFG_DEFAULT << 0)       /**< Shifted mode DEFAULT for USB_GDFIFOCFG */
#define _USB_GDFIFOCFG_EPINFOBASEADDR_SHIFT        16                                            /**< Shift value for USB_EPINFOBASEADDR */
#define _USB_GDFIFOCFG_EPINFOBASEADDR_MASK         0xFFFF0000UL                                  /**< Bit mask for USB_EPINFOBASEADDR */
#define _USB_GDFIFOCFG_EPINFOBASEADDR_DEFAULT      0x000005F8UL                                  /**< Mode DEFAULT for USB_GDFIFOCFG */
#define USB_GDFIFOCFG_EPINFOBASEADDR_DEFAULT       (_USB_GDFIFOCFG_EPINFOBASEADDR_DEFAULT << 16) /**< Shifted mode DEFAULT for USB_GDFIFOCFG */

/* Bit fields for USB DIEPTXF1 */
#define _USB_DIEPTXF1_RESETVALUE                   0x02000400UL                                /**< Default value for USB_DIEPTXF1 */
#define _USB_DIEPTXF1_MASK                         0x03FF07FFUL                                /**< Mask for USB_DIEPTXF1 */
#define _USB_DIEPTXF1_INEPNTXFSTADDR_SHIFT         0                                           /**< Shift value for USB_INEPNTXFSTADDR */
#define _USB_DIEPTXF1_INEPNTXFSTADDR_MASK          0x7FFUL                                     /**< Bit mask for USB_INEPNTXFSTADDR */
#define _USB_DIEPTXF1_INEPNTXFSTADDR_DEFAULT       0x00000400UL                                /**< Mode DEFAULT for USB_DIEPTXF1 */
#define USB_DIEPTXF1_INEPNTXFSTADDR_DEFAULT        (_USB_DIEPTXF1_INEPNTXFSTADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEPTXF1 */
#define _USB_DIEPTXF1_INEPNTXFDEP_SHIFT            16                                          /**< Shift value for USB_INEPNTXFDEP */
#define _USB_DIEPTXF1_INEPNTXFDEP_MASK             0x3FF0000UL                                 /**< Bit mask for USB_INEPNTXFDEP */
#define _USB_DIEPTXF1_INEPNTXFDEP_DEFAULT          0x00000200UL                                /**< Mode DEFAULT for USB_DIEPTXF1 */
#define USB_DIEPTXF1_INEPNTXFDEP_DEFAULT           (_USB_DIEPTXF1_INEPNTXFDEP_DEFAULT << 16)   /**< Shifted mode DEFAULT for USB_DIEPTXF1 */

/* Bit fields for USB DIEPTXF2 */
#define _USB_DIEPTXF2_RESETVALUE                   0x02000600UL                                /**< Default value for USB_DIEPTXF2 */
#define _USB_DIEPTXF2_MASK                         0x03FF07FFUL                                /**< Mask for USB_DIEPTXF2 */
#define _USB_DIEPTXF2_INEPNTXFSTADDR_SHIFT         0                                           /**< Shift value for USB_INEPNTXFSTADDR */
#define _USB_DIEPTXF2_INEPNTXFSTADDR_MASK          0x7FFUL                                     /**< Bit mask for USB_INEPNTXFSTADDR */
#define _USB_DIEPTXF2_INEPNTXFSTADDR_DEFAULT       0x00000600UL                                /**< Mode DEFAULT for USB_DIEPTXF2 */
#define USB_DIEPTXF2_INEPNTXFSTADDR_DEFAULT        (_USB_DIEPTXF2_INEPNTXFSTADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEPTXF2 */
#define _USB_DIEPTXF2_INEPNTXFDEP_SHIFT            16                                          /**< Shift value for USB_INEPNTXFDEP */
#define _USB_DIEPTXF2_INEPNTXFDEP_MASK             0x3FF0000UL                                 /**< Bit mask for USB_INEPNTXFDEP */
#define _USB_DIEPTXF2_INEPNTXFDEP_DEFAULT          0x00000200UL                                /**< Mode DEFAULT for USB_DIEPTXF2 */
#define USB_DIEPTXF2_INEPNTXFDEP_DEFAULT           (_USB_DIEPTXF2_INEPNTXFDEP_DEFAULT << 16)   /**< Shifted mode DEFAULT for USB_DIEPTXF2 */

/* Bit fields for USB DIEPTXF3 */
#define _USB_DIEPTXF3_RESETVALUE                   0x02000800UL                                /**< Default value for USB_DIEPTXF3 */
#define _USB_DIEPTXF3_MASK                         0x03FF0FFFUL                                /**< Mask for USB_DIEPTXF3 */
#define _USB_DIEPTXF3_INEPNTXFSTADDR_SHIFT         0                                           /**< Shift value for USB_INEPNTXFSTADDR */
#define _USB_DIEPTXF3_INEPNTXFSTADDR_MASK          0xFFFUL                                     /**< Bit mask for USB_INEPNTXFSTADDR */
#define _USB_DIEPTXF3_INEPNTXFSTADDR_DEFAULT       0x00000800UL                                /**< Mode DEFAULT for USB_DIEPTXF3 */
#define USB_DIEPTXF3_INEPNTXFSTADDR_DEFAULT        (_USB_DIEPTXF3_INEPNTXFSTADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEPTXF3 */
#define _USB_DIEPTXF3_INEPNTXFDEP_SHIFT            16                                          /**< Shift value for USB_INEPNTXFDEP */
#define _USB_DIEPTXF3_INEPNTXFDEP_MASK             0x3FF0000UL                                 /**< Bit mask for USB_INEPNTXFDEP */
#define _USB_DIEPTXF3_INEPNTXFDEP_DEFAULT          0x00000200UL                                /**< Mode DEFAULT for USB_DIEPTXF3 */
#define USB_DIEPTXF3_INEPNTXFDEP_DEFAULT           (_USB_DIEPTXF3_INEPNTXFDEP_DEFAULT << 16)   /**< Shifted mode DEFAULT for USB_DIEPTXF3 */

/* Bit fields for USB DCFG */
#define _USB_DCFG_RESETVALUE                       0x08000000UL                            /**< Default value for USB_DCFG */
#define _USB_DCFG_MASK                             0xFC009FFFUL                            /**< Mask for USB_DCFG */
#define _USB_DCFG_DEVSPD_SHIFT                     0                                       /**< Shift value for USB_DEVSPD */
#define _USB_DCFG_DEVSPD_MASK                      0x3UL                                   /**< Bit mask for USB_DEVSPD */
#define _USB_DCFG_DEVSPD_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for USB_DCFG */
#define _USB_DCFG_DEVSPD_LS                        0x00000002UL                            /**< Mode LS for USB_DCFG */
#define _USB_DCFG_DEVSPD_FS                        0x00000003UL                            /**< Mode FS for USB_DCFG */
#define USB_DCFG_DEVSPD_DEFAULT                    (_USB_DCFG_DEVSPD_DEFAULT << 0)         /**< Shifted mode DEFAULT for USB_DCFG */
#define USB_DCFG_DEVSPD_LS                         (_USB_DCFG_DEVSPD_LS << 0)              /**< Shifted mode LS for USB_DCFG */
#define USB_DCFG_DEVSPD_FS                         (_USB_DCFG_DEVSPD_FS << 0)              /**< Shifted mode FS for USB_DCFG */
#define USB_DCFG_NZSTSOUTHSHK                      (0x1UL << 2)                            /**< Non-Zero-Length Status OUT Handshake */
#define _USB_DCFG_NZSTSOUTHSHK_SHIFT               2                                       /**< Shift value for USB_NZSTSOUTHSHK */
#define _USB_DCFG_NZSTSOUTHSHK_MASK                0x4UL                                   /**< Bit mask for USB_NZSTSOUTHSHK */
#define _USB_DCFG_NZSTSOUTHSHK_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for USB_DCFG */
#define USB_DCFG_NZSTSOUTHSHK_DEFAULT              (_USB_DCFG_NZSTSOUTHSHK_DEFAULT << 2)   /**< Shifted mode DEFAULT for USB_DCFG */
#define USB_DCFG_ENA32KHZSUSP                      (0x1UL << 3)                            /**< Enable 32 KHz Suspend mode */
#define _USB_DCFG_ENA32KHZSUSP_SHIFT               3                                       /**< Shift value for USB_ENA32KHZSUSP */
#define _USB_DCFG_ENA32KHZSUSP_MASK                0x8UL                                   /**< Bit mask for USB_ENA32KHZSUSP */
#define _USB_DCFG_ENA32KHZSUSP_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for USB_DCFG */
#define USB_DCFG_ENA32KHZSUSP_DEFAULT              (_USB_DCFG_ENA32KHZSUSP_DEFAULT << 3)   /**< Shifted mode DEFAULT for USB_DCFG */
#define _USB_DCFG_DEVADDR_SHIFT                    4                                       /**< Shift value for USB_DEVADDR */
#define _USB_DCFG_DEVADDR_MASK                     0x7F0UL                                 /**< Bit mask for USB_DEVADDR */
#define _USB_DCFG_DEVADDR_DEFAULT                  0x00000000UL                            /**< Mode DEFAULT for USB_DCFG */
#define USB_DCFG_DEVADDR_DEFAULT                   (_USB_DCFG_DEVADDR_DEFAULT << 4)        /**< Shifted mode DEFAULT for USB_DCFG */
#define _USB_DCFG_PERFRINT_SHIFT                   11                                      /**< Shift value for USB_PERFRINT */
#define _USB_DCFG_PERFRINT_MASK                    0x1800UL                                /**< Bit mask for USB_PERFRINT */
#define _USB_DCFG_PERFRINT_DEFAULT                 0x00000000UL                            /**< Mode DEFAULT for USB_DCFG */
#define _USB_DCFG_PERFRINT_80PCNT                  0x00000000UL                            /**< Mode 80PCNT for USB_DCFG */
#define _USB_DCFG_PERFRINT_85PCNT                  0x00000001UL                            /**< Mode 85PCNT for USB_DCFG */
#define _USB_DCFG_PERFRINT_90PCNT                  0x00000002UL                            /**< Mode 90PCNT for USB_DCFG */
#define _USB_DCFG_PERFRINT_95PCNT                  0x00000003UL                            /**< Mode 95PCNT for USB_DCFG */
#define USB_DCFG_PERFRINT_DEFAULT                  (_USB_DCFG_PERFRINT_DEFAULT << 11)      /**< Shifted mode DEFAULT for USB_DCFG */
#define USB_DCFG_PERFRINT_80PCNT                   (_USB_DCFG_PERFRINT_80PCNT << 11)       /**< Shifted mode 80PCNT for USB_DCFG */
#define USB_DCFG_PERFRINT_85PCNT                   (_USB_DCFG_PERFRINT_85PCNT << 11)       /**< Shifted mode 85PCNT for USB_DCFG */
#define USB_DCFG_PERFRINT_90PCNT                   (_USB_DCFG_PERFRINT_90PCNT << 11)       /**< Shifted mode 90PCNT for USB_DCFG */
#define USB_DCFG_PERFRINT_95PCNT                   (_USB_DCFG_PERFRINT_95PCNT << 11)       /**< Shifted mode 95PCNT for USB_DCFG */
#define USB_DCFG_ERRATICINTMSK                     (0x1UL << 15)                           /**<  */
#define _USB_DCFG_ERRATICINTMSK_SHIFT              15                                      /**< Shift value for USB_ERRATICINTMSK */
#define _USB_DCFG_ERRATICINTMSK_MASK               0x8000UL                                /**< Bit mask for USB_ERRATICINTMSK */
#define _USB_DCFG_ERRATICINTMSK_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for USB_DCFG */
#define USB_DCFG_ERRATICINTMSK_DEFAULT             (_USB_DCFG_ERRATICINTMSK_DEFAULT << 15) /**< Shifted mode DEFAULT for USB_DCFG */
#define _USB_DCFG_RESVALID_SHIFT                   26                                      /**< Shift value for USB_RESVALID */
#define _USB_DCFG_RESVALID_MASK                    0xFC000000UL                            /**< Bit mask for USB_RESVALID */
#define _USB_DCFG_RESVALID_DEFAULT                 0x00000002UL                            /**< Mode DEFAULT for USB_DCFG */
#define USB_DCFG_RESVALID_DEFAULT                  (_USB_DCFG_RESVALID_DEFAULT << 26)      /**< Shifted mode DEFAULT for USB_DCFG */

/* Bit fields for USB DCTL */
#define _USB_DCTL_RESETVALUE                       0x00000002UL                           /**< Default value for USB_DCTL */
#define _USB_DCTL_MASK                             0x00018FFFUL                           /**< Mask for USB_DCTL */
#define USB_DCTL_RMTWKUPSIG                        (0x1UL << 0)                           /**< Remote Wakeup Signaling */
#define _USB_DCTL_RMTWKUPSIG_SHIFT                 0                                      /**< Shift value for USB_RMTWKUPSIG */
#define _USB_DCTL_RMTWKUPSIG_MASK                  0x1UL                                  /**< Bit mask for USB_RMTWKUPSIG */
#define _USB_DCTL_RMTWKUPSIG_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_RMTWKUPSIG_DEFAULT                (_USB_DCTL_RMTWKUPSIG_DEFAULT << 0)    /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_SFTDISCON                         (0x1UL << 1)                           /**< Soft Disconnect */
#define _USB_DCTL_SFTDISCON_SHIFT                  1                                      /**< Shift value for USB_SFTDISCON */
#define _USB_DCTL_SFTDISCON_MASK                   0x2UL                                  /**< Bit mask for USB_SFTDISCON */
#define _USB_DCTL_SFTDISCON_DEFAULT                0x00000001UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_SFTDISCON_DEFAULT                 (_USB_DCTL_SFTDISCON_DEFAULT << 1)     /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_GNPINNAKSTS                       (0x1UL << 2)                           /**< Global Non-periodic IN NAK Status */
#define _USB_DCTL_GNPINNAKSTS_SHIFT                2                                      /**< Shift value for USB_GNPINNAKSTS */
#define _USB_DCTL_GNPINNAKSTS_MASK                 0x4UL                                  /**< Bit mask for USB_GNPINNAKSTS */
#define _USB_DCTL_GNPINNAKSTS_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_GNPINNAKSTS_DEFAULT               (_USB_DCTL_GNPINNAKSTS_DEFAULT << 2)   /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_GOUTNAKSTS                        (0x1UL << 3)                           /**< Global OUT NAK Status */
#define _USB_DCTL_GOUTNAKSTS_SHIFT                 3                                      /**< Shift value for USB_GOUTNAKSTS */
#define _USB_DCTL_GOUTNAKSTS_MASK                  0x8UL                                  /**< Bit mask for USB_GOUTNAKSTS */
#define _USB_DCTL_GOUTNAKSTS_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_GOUTNAKSTS_DEFAULT                (_USB_DCTL_GOUTNAKSTS_DEFAULT << 3)    /**< Shifted mode DEFAULT for USB_DCTL */
#define _USB_DCTL_TSTCTL_SHIFT                     4                                      /**< Shift value for USB_TSTCTL */
#define _USB_DCTL_TSTCTL_MASK                      0x70UL                                 /**< Bit mask for USB_TSTCTL */
#define _USB_DCTL_TSTCTL_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define _USB_DCTL_TSTCTL_DISABLE                   0x00000000UL                           /**< Mode DISABLE for USB_DCTL */
#define _USB_DCTL_TSTCTL_J                         0x00000001UL                           /**< Mode J for USB_DCTL */
#define _USB_DCTL_TSTCTL_K                         0x00000002UL                           /**< Mode K for USB_DCTL */
#define _USB_DCTL_TSTCTL_SE0NAK                    0x00000003UL                           /**< Mode SE0NAK for USB_DCTL */
#define _USB_DCTL_TSTCTL_PACKET                    0x00000004UL                           /**< Mode PACKET for USB_DCTL */
#define _USB_DCTL_TSTCTL_FORCE                     0x00000005UL                           /**< Mode FORCE for USB_DCTL */
#define USB_DCTL_TSTCTL_DEFAULT                    (_USB_DCTL_TSTCTL_DEFAULT << 4)        /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_TSTCTL_DISABLE                    (_USB_DCTL_TSTCTL_DISABLE << 4)        /**< Shifted mode DISABLE for USB_DCTL */
#define USB_DCTL_TSTCTL_J                          (_USB_DCTL_TSTCTL_J << 4)              /**< Shifted mode J for USB_DCTL */
#define USB_DCTL_TSTCTL_K                          (_USB_DCTL_TSTCTL_K << 4)              /**< Shifted mode K for USB_DCTL */
#define USB_DCTL_TSTCTL_SE0NAK                     (_USB_DCTL_TSTCTL_SE0NAK << 4)         /**< Shifted mode SE0NAK for USB_DCTL */
#define USB_DCTL_TSTCTL_PACKET                     (_USB_DCTL_TSTCTL_PACKET << 4)         /**< Shifted mode PACKET for USB_DCTL */
#define USB_DCTL_TSTCTL_FORCE                      (_USB_DCTL_TSTCTL_FORCE << 4)          /**< Shifted mode FORCE for USB_DCTL */
#define USB_DCTL_SGNPINNAK                         (0x1UL << 7)                           /**< Set Global Non-periodic IN NAK */
#define _USB_DCTL_SGNPINNAK_SHIFT                  7                                      /**< Shift value for USB_SGNPINNAK */
#define _USB_DCTL_SGNPINNAK_MASK                   0x80UL                                 /**< Bit mask for USB_SGNPINNAK */
#define _USB_DCTL_SGNPINNAK_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_SGNPINNAK_DEFAULT                 (_USB_DCTL_SGNPINNAK_DEFAULT << 7)     /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_CGNPINNAK                         (0x1UL << 8)                           /**< Clear Global Non-periodic IN NAK */
#define _USB_DCTL_CGNPINNAK_SHIFT                  8                                      /**< Shift value for USB_CGNPINNAK */
#define _USB_DCTL_CGNPINNAK_MASK                   0x100UL                                /**< Bit mask for USB_CGNPINNAK */
#define _USB_DCTL_CGNPINNAK_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_CGNPINNAK_DEFAULT                 (_USB_DCTL_CGNPINNAK_DEFAULT << 8)     /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_SGOUTNAK                          (0x1UL << 9)                           /**< Set Global OUT NAK */
#define _USB_DCTL_SGOUTNAK_SHIFT                   9                                      /**< Shift value for USB_SGOUTNAK */
#define _USB_DCTL_SGOUTNAK_MASK                    0x200UL                                /**< Bit mask for USB_SGOUTNAK */
#define _USB_DCTL_SGOUTNAK_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_SGOUTNAK_DEFAULT                  (_USB_DCTL_SGOUTNAK_DEFAULT << 9)      /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_CGOUTNAK                          (0x1UL << 10)                          /**< Clear Global OUT NAK */
#define _USB_DCTL_CGOUTNAK_SHIFT                   10                                     /**< Shift value for USB_CGOUTNAK */
#define _USB_DCTL_CGOUTNAK_MASK                    0x400UL                                /**< Bit mask for USB_CGOUTNAK */
#define _USB_DCTL_CGOUTNAK_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_CGOUTNAK_DEFAULT                  (_USB_DCTL_CGOUTNAK_DEFAULT << 10)     /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_PWRONPRGDONE                      (0x1UL << 11)                          /**< Power-On Programming Done */
#define _USB_DCTL_PWRONPRGDONE_SHIFT               11                                     /**< Shift value for USB_PWRONPRGDONE */
#define _USB_DCTL_PWRONPRGDONE_MASK                0x800UL                                /**< Bit mask for USB_PWRONPRGDONE */
#define _USB_DCTL_PWRONPRGDONE_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_PWRONPRGDONE_DEFAULT              (_USB_DCTL_PWRONPRGDONE_DEFAULT << 11) /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_IGNRFRMNUM                        (0x1UL << 15)                          /**< Ignore Frame number For Isochronous End points */
#define _USB_DCTL_IGNRFRMNUM_SHIFT                 15                                     /**< Shift value for USB_IGNRFRMNUM */
#define _USB_DCTL_IGNRFRMNUM_MASK                  0x8000UL                               /**< Bit mask for USB_IGNRFRMNUM */
#define _USB_DCTL_IGNRFRMNUM_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_IGNRFRMNUM_DEFAULT                (_USB_DCTL_IGNRFRMNUM_DEFAULT << 15)   /**< Shifted mode DEFAULT for USB_DCTL */
#define USB_DCTL_NAKONBBLE                         (0x1UL << 16)                          /**< NAK on Babble Error */
#define _USB_DCTL_NAKONBBLE_SHIFT                  16                                     /**< Shift value for USB_NAKONBBLE */
#define _USB_DCTL_NAKONBBLE_MASK                   0x10000UL                              /**< Bit mask for USB_NAKONBBLE */
#define _USB_DCTL_NAKONBBLE_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DCTL */
#define USB_DCTL_NAKONBBLE_DEFAULT                 (_USB_DCTL_NAKONBBLE_DEFAULT << 16)    /**< Shifted mode DEFAULT for USB_DCTL */

/* Bit fields for USB DSTS */
#define _USB_DSTS_RESETVALUE                       0x00000002UL                       /**< Default value for USB_DSTS */
#define _USB_DSTS_MASK                             0x00FFFF0FUL                       /**< Mask for USB_DSTS */
#define USB_DSTS_SUSPSTS                           (0x1UL << 0)                       /**< Suspend Status */
#define _USB_DSTS_SUSPSTS_SHIFT                    0                                  /**< Shift value for USB_SUSPSTS */
#define _USB_DSTS_SUSPSTS_MASK                     0x1UL                              /**< Bit mask for USB_SUSPSTS */
#define _USB_DSTS_SUSPSTS_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for USB_DSTS */
#define USB_DSTS_SUSPSTS_DEFAULT                   (_USB_DSTS_SUSPSTS_DEFAULT << 0)   /**< Shifted mode DEFAULT for USB_DSTS */
#define _USB_DSTS_ENUMSPD_SHIFT                    1                                  /**< Shift value for USB_ENUMSPD */
#define _USB_DSTS_ENUMSPD_MASK                     0x6UL                              /**< Bit mask for USB_ENUMSPD */
#define _USB_DSTS_ENUMSPD_DEFAULT                  0x00000001UL                       /**< Mode DEFAULT for USB_DSTS */
#define _USB_DSTS_ENUMSPD_LS                       0x00000002UL                       /**< Mode LS for USB_DSTS */
#define _USB_DSTS_ENUMSPD_FS                       0x00000003UL                       /**< Mode FS for USB_DSTS */
#define USB_DSTS_ENUMSPD_DEFAULT                   (_USB_DSTS_ENUMSPD_DEFAULT << 1)   /**< Shifted mode DEFAULT for USB_DSTS */
#define USB_DSTS_ENUMSPD_LS                        (_USB_DSTS_ENUMSPD_LS << 1)        /**< Shifted mode LS for USB_DSTS */
#define USB_DSTS_ENUMSPD_FS                        (_USB_DSTS_ENUMSPD_FS << 1)        /**< Shifted mode FS for USB_DSTS */
#define USB_DSTS_ERRTICERR                         (0x1UL << 3)                       /**< Erratic Error */
#define _USB_DSTS_ERRTICERR_SHIFT                  3                                  /**< Shift value for USB_ERRTICERR */
#define _USB_DSTS_ERRTICERR_MASK                   0x8UL                              /**< Bit mask for USB_ERRTICERR */
#define _USB_DSTS_ERRTICERR_DEFAULT                0x00000000UL                       /**< Mode DEFAULT for USB_DSTS */
#define USB_DSTS_ERRTICERR_DEFAULT                 (_USB_DSTS_ERRTICERR_DEFAULT << 3) /**< Shifted mode DEFAULT for USB_DSTS */
#define _USB_DSTS_SOFFN_SHIFT                      8                                  /**< Shift value for USB_SOFFN */
#define _USB_DSTS_SOFFN_MASK                       0x3FFF00UL                         /**< Bit mask for USB_SOFFN */
#define _USB_DSTS_SOFFN_DEFAULT                    0x00000000UL                       /**< Mode DEFAULT for USB_DSTS */
#define USB_DSTS_SOFFN_DEFAULT                     (_USB_DSTS_SOFFN_DEFAULT << 8)     /**< Shifted mode DEFAULT for USB_DSTS */
#define _USB_DSTS_DEVLNSTS_SHIFT                   22                                 /**< Shift value for USB_DEVLNSTS */
#define _USB_DSTS_DEVLNSTS_MASK                    0xC00000UL                         /**< Bit mask for USB_DEVLNSTS */
#define _USB_DSTS_DEVLNSTS_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for USB_DSTS */
#define USB_DSTS_DEVLNSTS_DEFAULT                  (_USB_DSTS_DEVLNSTS_DEFAULT << 22) /**< Shifted mode DEFAULT for USB_DSTS */

/* Bit fields for USB DIEPMSK */
#define _USB_DIEPMSK_RESETVALUE                    0x00000000UL                               /**< Default value for USB_DIEPMSK */
#define _USB_DIEPMSK_MASK                          0x0000215FUL                               /**< Mask for USB_DIEPMSK */
#define USB_DIEPMSK_XFERCOMPLMSK                   (0x1UL << 0)                               /**< Transfer Completed Interrupt Mask */
#define _USB_DIEPMSK_XFERCOMPLMSK_SHIFT            0                                          /**< Shift value for USB_XFERCOMPLMSK */
#define _USB_DIEPMSK_XFERCOMPLMSK_MASK             0x1UL                                      /**< Bit mask for USB_XFERCOMPLMSK */
#define _USB_DIEPMSK_XFERCOMPLMSK_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_XFERCOMPLMSK_DEFAULT           (_USB_DIEPMSK_XFERCOMPLMSK_DEFAULT << 0)   /**< Shifted mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_EPDISBLDMSK                    (0x1UL << 1)                               /**< Endpoint Disabled Interrupt Mask */
#define _USB_DIEPMSK_EPDISBLDMSK_SHIFT             1                                          /**< Shift value for USB_EPDISBLDMSK */
#define _USB_DIEPMSK_EPDISBLDMSK_MASK              0x2UL                                      /**< Bit mask for USB_EPDISBLDMSK */
#define _USB_DIEPMSK_EPDISBLDMSK_DEFAULT           0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_EPDISBLDMSK_DEFAULT            (_USB_DIEPMSK_EPDISBLDMSK_DEFAULT << 1)    /**< Shifted mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_AHBERRMSK                      (0x1UL << 2)                               /**< AHB Error Mask */
#define _USB_DIEPMSK_AHBERRMSK_SHIFT               2                                          /**< Shift value for USB_AHBERRMSK */
#define _USB_DIEPMSK_AHBERRMSK_MASK                0x4UL                                      /**< Bit mask for USB_AHBERRMSK */
#define _USB_DIEPMSK_AHBERRMSK_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_AHBERRMSK_DEFAULT              (_USB_DIEPMSK_AHBERRMSK_DEFAULT << 2)      /**< Shifted mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_TIMEOUTMSK                     (0x1UL << 3)                               /**< Timeout Condition Mask */
#define _USB_DIEPMSK_TIMEOUTMSK_SHIFT              3                                          /**< Shift value for USB_TIMEOUTMSK */
#define _USB_DIEPMSK_TIMEOUTMSK_MASK               0x8UL                                      /**< Bit mask for USB_TIMEOUTMSK */
#define _USB_DIEPMSK_TIMEOUTMSK_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_TIMEOUTMSK_DEFAULT             (_USB_DIEPMSK_TIMEOUTMSK_DEFAULT << 3)     /**< Shifted mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_INTKNTXFEMPMSK                 (0x1UL << 4)                               /**< IN Token Received When TxFIFO Empty Mask */
#define _USB_DIEPMSK_INTKNTXFEMPMSK_SHIFT          4                                          /**< Shift value for USB_INTKNTXFEMPMSK */
#define _USB_DIEPMSK_INTKNTXFEMPMSK_MASK           0x10UL                                     /**< Bit mask for USB_INTKNTXFEMPMSK */
#define _USB_DIEPMSK_INTKNTXFEMPMSK_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_INTKNTXFEMPMSK_DEFAULT         (_USB_DIEPMSK_INTKNTXFEMPMSK_DEFAULT << 4) /**< Shifted mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_INEPNAKEFFMSK                  (0x1UL << 6)                               /**< IN Endpoint NAK Effective Mask */
#define _USB_DIEPMSK_INEPNAKEFFMSK_SHIFT           6                                          /**< Shift value for USB_INEPNAKEFFMSK */
#define _USB_DIEPMSK_INEPNAKEFFMSK_MASK            0x40UL                                     /**< Bit mask for USB_INEPNAKEFFMSK */
#define _USB_DIEPMSK_INEPNAKEFFMSK_DEFAULT         0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_INEPNAKEFFMSK_DEFAULT          (_USB_DIEPMSK_INEPNAKEFFMSK_DEFAULT << 6)  /**< Shifted mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_TXFIFOUNDRNMSK                 (0x1UL << 8)                               /**< Fifo Underrun Mask */
#define _USB_DIEPMSK_TXFIFOUNDRNMSK_SHIFT          8                                          /**< Shift value for USB_TXFIFOUNDRNMSK */
#define _USB_DIEPMSK_TXFIFOUNDRNMSK_MASK           0x100UL                                    /**< Bit mask for USB_TXFIFOUNDRNMSK */
#define _USB_DIEPMSK_TXFIFOUNDRNMSK_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_TXFIFOUNDRNMSK_DEFAULT         (_USB_DIEPMSK_TXFIFOUNDRNMSK_DEFAULT << 8) /**< Shifted mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_NAKMSK                         (0x1UL << 13)                              /**< NAK interrupt Mask */
#define _USB_DIEPMSK_NAKMSK_SHIFT                  13                                         /**< Shift value for USB_NAKMSK */
#define _USB_DIEPMSK_NAKMSK_MASK                   0x2000UL                                   /**< Bit mask for USB_NAKMSK */
#define _USB_DIEPMSK_NAKMSK_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for USB_DIEPMSK */
#define USB_DIEPMSK_NAKMSK_DEFAULT                 (_USB_DIEPMSK_NAKMSK_DEFAULT << 13)        /**< Shifted mode DEFAULT for USB_DIEPMSK */

/* Bit fields for USB DOEPMSK */
#define _USB_DOEPMSK_RESETVALUE                    0x00000000UL                               /**< Default value for USB_DOEPMSK */
#define _USB_DOEPMSK_MASK                          0x0000317FUL                               /**< Mask for USB_DOEPMSK */
#define USB_DOEPMSK_XFERCOMPLMSK                   (0x1UL << 0)                               /**< Transfer Completed Interrupt Mask */
#define _USB_DOEPMSK_XFERCOMPLMSK_SHIFT            0                                          /**< Shift value for USB_XFERCOMPLMSK */
#define _USB_DOEPMSK_XFERCOMPLMSK_MASK             0x1UL                                      /**< Bit mask for USB_XFERCOMPLMSK */
#define _USB_DOEPMSK_XFERCOMPLMSK_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_XFERCOMPLMSK_DEFAULT           (_USB_DOEPMSK_XFERCOMPLMSK_DEFAULT << 0)   /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_EPDISBLDMSK                    (0x1UL << 1)                               /**< Endpoint Disabled Interrupt Mask */
#define _USB_DOEPMSK_EPDISBLDMSK_SHIFT             1                                          /**< Shift value for USB_EPDISBLDMSK */
#define _USB_DOEPMSK_EPDISBLDMSK_MASK              0x2UL                                      /**< Bit mask for USB_EPDISBLDMSK */
#define _USB_DOEPMSK_EPDISBLDMSK_DEFAULT           0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_EPDISBLDMSK_DEFAULT            (_USB_DOEPMSK_EPDISBLDMSK_DEFAULT << 1)    /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_AHBERRMSK                      (0x1UL << 2)                               /**< AHB Error */
#define _USB_DOEPMSK_AHBERRMSK_SHIFT               2                                          /**< Shift value for USB_AHBERRMSK */
#define _USB_DOEPMSK_AHBERRMSK_MASK                0x4UL                                      /**< Bit mask for USB_AHBERRMSK */
#define _USB_DOEPMSK_AHBERRMSK_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_AHBERRMSK_DEFAULT              (_USB_DOEPMSK_AHBERRMSK_DEFAULT << 2)      /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_SETUPMSK                       (0x1UL << 3)                               /**< SETUP Phase Done Mask */
#define _USB_DOEPMSK_SETUPMSK_SHIFT                3                                          /**< Shift value for USB_SETUPMSK */
#define _USB_DOEPMSK_SETUPMSK_MASK                 0x8UL                                      /**< Bit mask for USB_SETUPMSK */
#define _USB_DOEPMSK_SETUPMSK_DEFAULT              0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_SETUPMSK_DEFAULT               (_USB_DOEPMSK_SETUPMSK_DEFAULT << 3)       /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_OUTTKNEPDISMSK                 (0x1UL << 4)                               /**< OUT Token Received when Endpoint Disabled Mask */
#define _USB_DOEPMSK_OUTTKNEPDISMSK_SHIFT          4                                          /**< Shift value for USB_OUTTKNEPDISMSK */
#define _USB_DOEPMSK_OUTTKNEPDISMSK_MASK           0x10UL                                     /**< Bit mask for USB_OUTTKNEPDISMSK */
#define _USB_DOEPMSK_OUTTKNEPDISMSK_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_OUTTKNEPDISMSK_DEFAULT         (_USB_DOEPMSK_OUTTKNEPDISMSK_DEFAULT << 4) /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_STSPHSERCVDMSK                 (0x1UL << 5)                               /**< Status Phase Received Mask */
#define _USB_DOEPMSK_STSPHSERCVDMSK_SHIFT          5                                          /**< Shift value for USB_STSPHSERCVDMSK */
#define _USB_DOEPMSK_STSPHSERCVDMSK_MASK           0x20UL                                     /**< Bit mask for USB_STSPHSERCVDMSK */
#define _USB_DOEPMSK_STSPHSERCVDMSK_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_STSPHSERCVDMSK_DEFAULT         (_USB_DOEPMSK_STSPHSERCVDMSK_DEFAULT << 5) /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_BACK2BACKSETUP                 (0x1UL << 6)                               /**< Back-to-Back SETUP Packets Received Mask */
#define _USB_DOEPMSK_BACK2BACKSETUP_SHIFT          6                                          /**< Shift value for USB_BACK2BACKSETUP */
#define _USB_DOEPMSK_BACK2BACKSETUP_MASK           0x40UL                                     /**< Bit mask for USB_BACK2BACKSETUP */
#define _USB_DOEPMSK_BACK2BACKSETUP_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_BACK2BACKSETUP_DEFAULT         (_USB_DOEPMSK_BACK2BACKSETUP_DEFAULT << 6) /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_OUTPKTERRMSK                   (0x1UL << 8)                               /**< OUT Packet Error Mask */
#define _USB_DOEPMSK_OUTPKTERRMSK_SHIFT            8                                          /**< Shift value for USB_OUTPKTERRMSK */
#define _USB_DOEPMSK_OUTPKTERRMSK_MASK             0x100UL                                    /**< Bit mask for USB_OUTPKTERRMSK */
#define _USB_DOEPMSK_OUTPKTERRMSK_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_OUTPKTERRMSK_DEFAULT           (_USB_DOEPMSK_OUTPKTERRMSK_DEFAULT << 8)   /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_BBLEERRMSK                     (0x1UL << 12)                              /**< Babble Error interrupt Mask */
#define _USB_DOEPMSK_BBLEERRMSK_SHIFT              12                                         /**< Shift value for USB_BBLEERRMSK */
#define _USB_DOEPMSK_BBLEERRMSK_MASK               0x1000UL                                   /**< Bit mask for USB_BBLEERRMSK */
#define _USB_DOEPMSK_BBLEERRMSK_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_BBLEERRMSK_DEFAULT             (_USB_DOEPMSK_BBLEERRMSK_DEFAULT << 12)    /**< Shifted mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_NAKMSK                         (0x1UL << 13)                              /**< NAK interrupt Mask */
#define _USB_DOEPMSK_NAKMSK_SHIFT                  13                                         /**< Shift value for USB_NAKMSK */
#define _USB_DOEPMSK_NAKMSK_MASK                   0x2000UL                                   /**< Bit mask for USB_NAKMSK */
#define _USB_DOEPMSK_NAKMSK_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for USB_DOEPMSK */
#define USB_DOEPMSK_NAKMSK_DEFAULT                 (_USB_DOEPMSK_NAKMSK_DEFAULT << 13)        /**< Shifted mode DEFAULT for USB_DOEPMSK */

/* Bit fields for USB DAINT */
#define _USB_DAINT_RESETVALUE                      0x00000000UL                         /**< Default value for USB_DAINT */
#define _USB_DAINT_MASK                            0x000F000FUL                         /**< Mask for USB_DAINT */
#define USB_DAINT_INEPINT0                         (0x1UL << 0)                         /**< IN Endpoint 0 Interrupt Bit */
#define _USB_DAINT_INEPINT0_SHIFT                  0                                    /**< Shift value for USB_INEPINT0 */
#define _USB_DAINT_INEPINT0_MASK                   0x1UL                                /**< Bit mask for USB_INEPINT0 */
#define _USB_DAINT_INEPINT0_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_INEPINT0_DEFAULT                 (_USB_DAINT_INEPINT0_DEFAULT << 0)   /**< Shifted mode DEFAULT for USB_DAINT */
#define USB_DAINT_INEPINT1                         (0x1UL << 1)                         /**< IN Endpoint 1 Interrupt Bit */
#define _USB_DAINT_INEPINT1_SHIFT                  1                                    /**< Shift value for USB_INEPINT1 */
#define _USB_DAINT_INEPINT1_MASK                   0x2UL                                /**< Bit mask for USB_INEPINT1 */
#define _USB_DAINT_INEPINT1_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_INEPINT1_DEFAULT                 (_USB_DAINT_INEPINT1_DEFAULT << 1)   /**< Shifted mode DEFAULT for USB_DAINT */
#define USB_DAINT_INEPINT2                         (0x1UL << 2)                         /**< IN Endpoint 2 Interrupt Bit */
#define _USB_DAINT_INEPINT2_SHIFT                  2                                    /**< Shift value for USB_INEPINT2 */
#define _USB_DAINT_INEPINT2_MASK                   0x4UL                                /**< Bit mask for USB_INEPINT2 */
#define _USB_DAINT_INEPINT2_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_INEPINT2_DEFAULT                 (_USB_DAINT_INEPINT2_DEFAULT << 2)   /**< Shifted mode DEFAULT for USB_DAINT */
#define USB_DAINT_INEPINT3                         (0x1UL << 3)                         /**< IN Endpoint 3 Interrupt Bit */
#define _USB_DAINT_INEPINT3_SHIFT                  3                                    /**< Shift value for USB_INEPINT3 */
#define _USB_DAINT_INEPINT3_MASK                   0x8UL                                /**< Bit mask for USB_INEPINT3 */
#define _USB_DAINT_INEPINT3_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_INEPINT3_DEFAULT                 (_USB_DAINT_INEPINT3_DEFAULT << 3)   /**< Shifted mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT0                        (0x1UL << 16)                        /**< OUT Endpoint 0 Interrupt Bit */
#define _USB_DAINT_OUTEPINT0_SHIFT                 16                                   /**< Shift value for USB_OUTEPINT0 */
#define _USB_DAINT_OUTEPINT0_MASK                  0x10000UL                            /**< Bit mask for USB_OUTEPINT0 */
#define _USB_DAINT_OUTEPINT0_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT0_DEFAULT                (_USB_DAINT_OUTEPINT0_DEFAULT << 16) /**< Shifted mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT1                        (0x1UL << 17)                        /**< OUT Endpoint 1 Interrupt Bit */
#define _USB_DAINT_OUTEPINT1_SHIFT                 17                                   /**< Shift value for USB_OUTEPINT1 */
#define _USB_DAINT_OUTEPINT1_MASK                  0x20000UL                            /**< Bit mask for USB_OUTEPINT1 */
#define _USB_DAINT_OUTEPINT1_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT1_DEFAULT                (_USB_DAINT_OUTEPINT1_DEFAULT << 17) /**< Shifted mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT2                        (0x1UL << 18)                        /**< OUT Endpoint 2 Interrupt Bit */
#define _USB_DAINT_OUTEPINT2_SHIFT                 18                                   /**< Shift value for USB_OUTEPINT2 */
#define _USB_DAINT_OUTEPINT2_MASK                  0x40000UL                            /**< Bit mask for USB_OUTEPINT2 */
#define _USB_DAINT_OUTEPINT2_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT2_DEFAULT                (_USB_DAINT_OUTEPINT2_DEFAULT << 18) /**< Shifted mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT3                        (0x1UL << 19)                        /**< OUT Endpoint 3 Interrupt Bit */
#define _USB_DAINT_OUTEPINT3_SHIFT                 19                                   /**< Shift value for USB_OUTEPINT3 */
#define _USB_DAINT_OUTEPINT3_MASK                  0x80000UL                            /**< Bit mask for USB_OUTEPINT3 */
#define _USB_DAINT_OUTEPINT3_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for USB_DAINT */
#define USB_DAINT_OUTEPINT3_DEFAULT                (_USB_DAINT_OUTEPINT3_DEFAULT << 19) /**< Shifted mode DEFAULT for USB_DAINT */

/* Bit fields for USB DAINTMSK */
#define _USB_DAINTMSK_RESETVALUE                   0x00000000UL                            /**< Default value for USB_DAINTMSK */
#define _USB_DAINTMSK_MASK                         0x000F000FUL                            /**< Mask for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK0                      (0x1UL << 0)                            /**< IN Endpoint 0 Interrupt mask Bit */
#define _USB_DAINTMSK_INEPMSK0_SHIFT               0                                       /**< Shift value for USB_INEPMSK0 */
#define _USB_DAINTMSK_INEPMSK0_MASK                0x1UL                                   /**< Bit mask for USB_INEPMSK0 */
#define _USB_DAINTMSK_INEPMSK0_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK0_DEFAULT              (_USB_DAINTMSK_INEPMSK0_DEFAULT << 0)   /**< Shifted mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK1                      (0x1UL << 1)                            /**< IN Endpoint 1 Interrupt mask Bit */
#define _USB_DAINTMSK_INEPMSK1_SHIFT               1                                       /**< Shift value for USB_INEPMSK1 */
#define _USB_DAINTMSK_INEPMSK1_MASK                0x2UL                                   /**< Bit mask for USB_INEPMSK1 */
#define _USB_DAINTMSK_INEPMSK1_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK1_DEFAULT              (_USB_DAINTMSK_INEPMSK1_DEFAULT << 1)   /**< Shifted mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK2                      (0x1UL << 2)                            /**< IN Endpoint 2 Interrupt mask Bit */
#define _USB_DAINTMSK_INEPMSK2_SHIFT               2                                       /**< Shift value for USB_INEPMSK2 */
#define _USB_DAINTMSK_INEPMSK2_MASK                0x4UL                                   /**< Bit mask for USB_INEPMSK2 */
#define _USB_DAINTMSK_INEPMSK2_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK2_DEFAULT              (_USB_DAINTMSK_INEPMSK2_DEFAULT << 2)   /**< Shifted mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK3                      (0x1UL << 3)                            /**< IN Endpoint 3 Interrupt mask Bit */
#define _USB_DAINTMSK_INEPMSK3_SHIFT               3                                       /**< Shift value for USB_INEPMSK3 */
#define _USB_DAINTMSK_INEPMSK3_MASK                0x8UL                                   /**< Bit mask for USB_INEPMSK3 */
#define _USB_DAINTMSK_INEPMSK3_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_INEPMSK3_DEFAULT              (_USB_DAINTMSK_INEPMSK3_DEFAULT << 3)   /**< Shifted mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK0                     (0x1UL << 16)                           /**< OUT Endpoint 0 Interrupt mask Bit */
#define _USB_DAINTMSK_OUTEPMSK0_SHIFT              16                                      /**< Shift value for USB_OUTEPMSK0 */
#define _USB_DAINTMSK_OUTEPMSK0_MASK               0x10000UL                               /**< Bit mask for USB_OUTEPMSK0 */
#define _USB_DAINTMSK_OUTEPMSK0_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK0_DEFAULT             (_USB_DAINTMSK_OUTEPMSK0_DEFAULT << 16) /**< Shifted mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK1                     (0x1UL << 17)                           /**< OUT Endpoint 1 Interrupt mask Bit */
#define _USB_DAINTMSK_OUTEPMSK1_SHIFT              17                                      /**< Shift value for USB_OUTEPMSK1 */
#define _USB_DAINTMSK_OUTEPMSK1_MASK               0x20000UL                               /**< Bit mask for USB_OUTEPMSK1 */
#define _USB_DAINTMSK_OUTEPMSK1_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK1_DEFAULT             (_USB_DAINTMSK_OUTEPMSK1_DEFAULT << 17) /**< Shifted mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK2                     (0x1UL << 18)                           /**< OUT Endpoint 2 Interrupt mask Bit */
#define _USB_DAINTMSK_OUTEPMSK2_SHIFT              18                                      /**< Shift value for USB_OUTEPMSK2 */
#define _USB_DAINTMSK_OUTEPMSK2_MASK               0x40000UL                               /**< Bit mask for USB_OUTEPMSK2 */
#define _USB_DAINTMSK_OUTEPMSK2_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK2_DEFAULT             (_USB_DAINTMSK_OUTEPMSK2_DEFAULT << 18) /**< Shifted mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK3                     (0x1UL << 19)                           /**< OUT Endpoint 3 Interrupt mask Bit */
#define _USB_DAINTMSK_OUTEPMSK3_SHIFT              19                                      /**< Shift value for USB_OUTEPMSK3 */
#define _USB_DAINTMSK_OUTEPMSK3_MASK               0x80000UL                               /**< Bit mask for USB_OUTEPMSK3 */
#define _USB_DAINTMSK_OUTEPMSK3_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for USB_DAINTMSK */
#define USB_DAINTMSK_OUTEPMSK3_DEFAULT             (_USB_DAINTMSK_OUTEPMSK3_DEFAULT << 19) /**< Shifted mode DEFAULT for USB_DAINTMSK */

/* Bit fields for USB DIEPEMPMSK */
#define _USB_DIEPEMPMSK_RESETVALUE                 0x00000000UL                              /**< Default value for USB_DIEPEMPMSK */
#define _USB_DIEPEMPMSK_MASK                       0x0000FFFFUL                              /**< Mask for USB_DIEPEMPMSK */
#define _USB_DIEPEMPMSK_DIEPEMPMSK_SHIFT           0                                         /**< Shift value for USB_DIEPEMPMSK */
#define _USB_DIEPEMPMSK_DIEPEMPMSK_MASK            0xFFFFUL                                  /**< Bit mask for USB_DIEPEMPMSK */
#define _USB_DIEPEMPMSK_DIEPEMPMSK_DEFAULT         0x00000000UL                              /**< Mode DEFAULT for USB_DIEPEMPMSK */
#define USB_DIEPEMPMSK_DIEPEMPMSK_DEFAULT          (_USB_DIEPEMPMSK_DIEPEMPMSK_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEPEMPMSK */

/* Bit fields for USB DIEP0CTL */
#define _USB_DIEP0CTL_RESETVALUE                   0x00008000UL                           /**< Default value for USB_DIEP0CTL */
#define _USB_DIEP0CTL_MASK                         0xCFEE8003UL                           /**< Mask for USB_DIEP0CTL */
#define _USB_DIEP0CTL_MPS_SHIFT                    0                                      /**< Shift value for USB_MPS */
#define _USB_DIEP0CTL_MPS_MASK                     0x3UL                                  /**< Bit mask for USB_MPS */
#define _USB_DIEP0CTL_MPS_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define _USB_DIEP0CTL_MPS_64B                      0x00000000UL                           /**< Mode 64B for USB_DIEP0CTL */
#define _USB_DIEP0CTL_MPS_32B                      0x00000001UL                           /**< Mode 32B for USB_DIEP0CTL */
#define _USB_DIEP0CTL_MPS_16B                      0x00000002UL                           /**< Mode 16B for USB_DIEP0CTL */
#define _USB_DIEP0CTL_MPS_8B                       0x00000003UL                           /**< Mode 8B for USB_DIEP0CTL */
#define USB_DIEP0CTL_MPS_DEFAULT                   (_USB_DIEP0CTL_MPS_DEFAULT << 0)       /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_MPS_64B                       (_USB_DIEP0CTL_MPS_64B << 0)           /**< Shifted mode 64B for USB_DIEP0CTL */
#define USB_DIEP0CTL_MPS_32B                       (_USB_DIEP0CTL_MPS_32B << 0)           /**< Shifted mode 32B for USB_DIEP0CTL */
#define USB_DIEP0CTL_MPS_16B                       (_USB_DIEP0CTL_MPS_16B << 0)           /**< Shifted mode 16B for USB_DIEP0CTL */
#define USB_DIEP0CTL_MPS_8B                        (_USB_DIEP0CTL_MPS_8B << 0)            /**< Shifted mode 8B for USB_DIEP0CTL */
#define USB_DIEP0CTL_USBACTEP                      (0x1UL << 15)                          /**< USB Active Endpoint */
#define _USB_DIEP0CTL_USBACTEP_SHIFT               15                                     /**< Shift value for USB_USBACTEP */
#define _USB_DIEP0CTL_USBACTEP_MASK                0x8000UL                               /**< Bit mask for USB_USBACTEP */
#define _USB_DIEP0CTL_USBACTEP_DEFAULT             0x00000001UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_USBACTEP_DEFAULT              (_USB_DIEP0CTL_USBACTEP_DEFAULT << 15) /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_NAKSTS                        (0x1UL << 17)                          /**< NAK Status */
#define _USB_DIEP0CTL_NAKSTS_SHIFT                 17                                     /**< Shift value for USB_NAKSTS */
#define _USB_DIEP0CTL_NAKSTS_MASK                  0x20000UL                              /**< Bit mask for USB_NAKSTS */
#define _USB_DIEP0CTL_NAKSTS_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_NAKSTS_DEFAULT                (_USB_DIEP0CTL_NAKSTS_DEFAULT << 17)   /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define _USB_DIEP0CTL_EPTYPE_SHIFT                 18                                     /**< Shift value for USB_EPTYPE */
#define _USB_DIEP0CTL_EPTYPE_MASK                  0xC0000UL                              /**< Bit mask for USB_EPTYPE */
#define _USB_DIEP0CTL_EPTYPE_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_EPTYPE_DEFAULT                (_USB_DIEP0CTL_EPTYPE_DEFAULT << 18)   /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_STALL                         (0x1UL << 21)                          /**< Handshake */
#define _USB_DIEP0CTL_STALL_SHIFT                  21                                     /**< Shift value for USB_STALL */
#define _USB_DIEP0CTL_STALL_MASK                   0x200000UL                             /**< Bit mask for USB_STALL */
#define _USB_DIEP0CTL_STALL_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_STALL_DEFAULT                 (_USB_DIEP0CTL_STALL_DEFAULT << 21)    /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define _USB_DIEP0CTL_TXFNUM_SHIFT                 22                                     /**< Shift value for USB_TXFNUM */
#define _USB_DIEP0CTL_TXFNUM_MASK                  0x3C00000UL                            /**< Bit mask for USB_TXFNUM */
#define _USB_DIEP0CTL_TXFNUM_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_TXFNUM_DEFAULT                (_USB_DIEP0CTL_TXFNUM_DEFAULT << 22)   /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_CNAK                          (0x1UL << 26)                          /**< Clear NAK */
#define _USB_DIEP0CTL_CNAK_SHIFT                   26                                     /**< Shift value for USB_CNAK */
#define _USB_DIEP0CTL_CNAK_MASK                    0x4000000UL                            /**< Bit mask for USB_CNAK */
#define _USB_DIEP0CTL_CNAK_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_CNAK_DEFAULT                  (_USB_DIEP0CTL_CNAK_DEFAULT << 26)     /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_SNAK                          (0x1UL << 27)                          /**< Set NAK */
#define _USB_DIEP0CTL_SNAK_SHIFT                   27                                     /**< Shift value for USB_SNAK */
#define _USB_DIEP0CTL_SNAK_MASK                    0x8000000UL                            /**< Bit mask for USB_SNAK */
#define _USB_DIEP0CTL_SNAK_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_SNAK_DEFAULT                  (_USB_DIEP0CTL_SNAK_DEFAULT << 27)     /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_EPDIS                         (0x1UL << 30)                          /**< Endpoint Disable */
#define _USB_DIEP0CTL_EPDIS_SHIFT                  30                                     /**< Shift value for USB_EPDIS */
#define _USB_DIEP0CTL_EPDIS_MASK                   0x40000000UL                           /**< Bit mask for USB_EPDIS */
#define _USB_DIEP0CTL_EPDIS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_EPDIS_DEFAULT                 (_USB_DIEP0CTL_EPDIS_DEFAULT << 30)    /**< Shifted mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_EPENA                         (0x1UL << 31)                          /**< Endpoint Enable */
#define _USB_DIEP0CTL_EPENA_SHIFT                  31                                     /**< Shift value for USB_EPENA */
#define _USB_DIEP0CTL_EPENA_MASK                   0x80000000UL                           /**< Bit mask for USB_EPENA */
#define _USB_DIEP0CTL_EPENA_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0CTL */
#define USB_DIEP0CTL_EPENA_DEFAULT                 (_USB_DIEP0CTL_EPENA_DEFAULT << 31)    /**< Shifted mode DEFAULT for USB_DIEP0CTL */

/* Bit fields for USB DIEP0INT */
#define _USB_DIEP0INT_RESETVALUE                   0x00000080UL                             /**< Default value for USB_DIEP0INT */
#define _USB_DIEP0INT_MASK                         0x000038DFUL                             /**< Mask for USB_DIEP0INT */
#define USB_DIEP0INT_XFERCOMPL                     (0x1UL << 0)                             /**< Transfer Completed Interrupt */
#define _USB_DIEP0INT_XFERCOMPL_SHIFT              0                                        /**< Shift value for USB_XFERCOMPL */
#define _USB_DIEP0INT_XFERCOMPL_MASK               0x1UL                                    /**< Bit mask for USB_XFERCOMPL */
#define _USB_DIEP0INT_XFERCOMPL_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_XFERCOMPL_DEFAULT             (_USB_DIEP0INT_XFERCOMPL_DEFAULT << 0)   /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_EPDISBLD                      (0x1UL << 1)                             /**< Endpoint Disabled Interrupt */
#define _USB_DIEP0INT_EPDISBLD_SHIFT               1                                        /**< Shift value for USB_EPDISBLD */
#define _USB_DIEP0INT_EPDISBLD_MASK                0x2UL                                    /**< Bit mask for USB_EPDISBLD */
#define _USB_DIEP0INT_EPDISBLD_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_EPDISBLD_DEFAULT              (_USB_DIEP0INT_EPDISBLD_DEFAULT << 1)    /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_AHBERR                        (0x1UL << 2)                             /**< AHB Error */
#define _USB_DIEP0INT_AHBERR_SHIFT                 2                                        /**< Shift value for USB_AHBERR */
#define _USB_DIEP0INT_AHBERR_MASK                  0x4UL                                    /**< Bit mask for USB_AHBERR */
#define _USB_DIEP0INT_AHBERR_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_AHBERR_DEFAULT                (_USB_DIEP0INT_AHBERR_DEFAULT << 2)      /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_TIMEOUT                       (0x1UL << 3)                             /**< Timeout Condition */
#define _USB_DIEP0INT_TIMEOUT_SHIFT                3                                        /**< Shift value for USB_TIMEOUT */
#define _USB_DIEP0INT_TIMEOUT_MASK                 0x8UL                                    /**< Bit mask for USB_TIMEOUT */
#define _USB_DIEP0INT_TIMEOUT_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_TIMEOUT_DEFAULT               (_USB_DIEP0INT_TIMEOUT_DEFAULT << 3)     /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_INTKNTXFEMP                   (0x1UL << 4)                             /**< IN Token Received When TxFIFO is Empty */
#define _USB_DIEP0INT_INTKNTXFEMP_SHIFT            4                                        /**< Shift value for USB_INTKNTXFEMP */
#define _USB_DIEP0INT_INTKNTXFEMP_MASK             0x10UL                                   /**< Bit mask for USB_INTKNTXFEMP */
#define _USB_DIEP0INT_INTKNTXFEMP_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_INTKNTXFEMP_DEFAULT           (_USB_DIEP0INT_INTKNTXFEMP_DEFAULT << 4) /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_INEPNAKEFF                    (0x1UL << 6)                             /**< IN Endpoint NAK Effective */
#define _USB_DIEP0INT_INEPNAKEFF_SHIFT             6                                        /**< Shift value for USB_INEPNAKEFF */
#define _USB_DIEP0INT_INEPNAKEFF_MASK              0x40UL                                   /**< Bit mask for USB_INEPNAKEFF */
#define _USB_DIEP0INT_INEPNAKEFF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_INEPNAKEFF_DEFAULT            (_USB_DIEP0INT_INEPNAKEFF_DEFAULT << 6)  /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_TXFEMP                        (0x1UL << 7)                             /**< Transmit FIFO Empty */
#define _USB_DIEP0INT_TXFEMP_SHIFT                 7                                        /**< Shift value for USB_TXFEMP */
#define _USB_DIEP0INT_TXFEMP_MASK                  0x80UL                                   /**< Bit mask for USB_TXFEMP */
#define _USB_DIEP0INT_TXFEMP_DEFAULT               0x00000001UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_TXFEMP_DEFAULT                (_USB_DIEP0INT_TXFEMP_DEFAULT << 7)      /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_PKTDRPSTS                     (0x1UL << 11)                            /**< Packet Drop Status */
#define _USB_DIEP0INT_PKTDRPSTS_SHIFT              11                                       /**< Shift value for USB_PKTDRPSTS */
#define _USB_DIEP0INT_PKTDRPSTS_MASK               0x800UL                                  /**< Bit mask for USB_PKTDRPSTS */
#define _USB_DIEP0INT_PKTDRPSTS_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_PKTDRPSTS_DEFAULT             (_USB_DIEP0INT_PKTDRPSTS_DEFAULT << 11)  /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_BBLEERR                       (0x1UL << 12)                            /**< NAK Interrupt */
#define _USB_DIEP0INT_BBLEERR_SHIFT                12                                       /**< Shift value for USB_BBLEERR */
#define _USB_DIEP0INT_BBLEERR_MASK                 0x1000UL                                 /**< Bit mask for USB_BBLEERR */
#define _USB_DIEP0INT_BBLEERR_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_BBLEERR_DEFAULT               (_USB_DIEP0INT_BBLEERR_DEFAULT << 12)    /**< Shifted mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_NAKINTRPT                     (0x1UL << 13)                            /**< NAK Interrupt */
#define _USB_DIEP0INT_NAKINTRPT_SHIFT              13                                       /**< Shift value for USB_NAKINTRPT */
#define _USB_DIEP0INT_NAKINTRPT_MASK               0x2000UL                                 /**< Bit mask for USB_NAKINTRPT */
#define _USB_DIEP0INT_NAKINTRPT_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_DIEP0INT */
#define USB_DIEP0INT_NAKINTRPT_DEFAULT             (_USB_DIEP0INT_NAKINTRPT_DEFAULT << 13)  /**< Shifted mode DEFAULT for USB_DIEP0INT */

/* Bit fields for USB DIEP0TSIZ */
#define _USB_DIEP0TSIZ_RESETVALUE                  0x00000000UL                           /**< Default value for USB_DIEP0TSIZ */
#define _USB_DIEP0TSIZ_MASK                        0x0018007FUL                           /**< Mask for USB_DIEP0TSIZ */
#define _USB_DIEP0TSIZ_XFERSIZE_SHIFT              0                                      /**< Shift value for USB_XFERSIZE */
#define _USB_DIEP0TSIZ_XFERSIZE_MASK               0x7FUL                                 /**< Bit mask for USB_XFERSIZE */
#define _USB_DIEP0TSIZ_XFERSIZE_DEFAULT            0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0TSIZ */
#define USB_DIEP0TSIZ_XFERSIZE_DEFAULT             (_USB_DIEP0TSIZ_XFERSIZE_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEP0TSIZ */
#define _USB_DIEP0TSIZ_PKTCNT_SHIFT                19                                     /**< Shift value for USB_PKTCNT */
#define _USB_DIEP0TSIZ_PKTCNT_MASK                 0x180000UL                             /**< Bit mask for USB_PKTCNT */
#define _USB_DIEP0TSIZ_PKTCNT_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for USB_DIEP0TSIZ */
#define USB_DIEP0TSIZ_PKTCNT_DEFAULT               (_USB_DIEP0TSIZ_PKTCNT_DEFAULT << 19)  /**< Shifted mode DEFAULT for USB_DIEP0TSIZ */

/* Bit fields for USB DIEP0DMAADDR */
#define _USB_DIEP0DMAADDR_RESETVALUE               0x00000000UL                                  /**< Default value for USB_DIEP0DMAADDR */
#define _USB_DIEP0DMAADDR_MASK                     0xFFFFFFFFUL                                  /**< Mask for USB_DIEP0DMAADDR */
#define _USB_DIEP0DMAADDR_DIEP0DMAADDR_SHIFT       0                                             /**< Shift value for USB_DIEP0DMAADDR */
#define _USB_DIEP0DMAADDR_DIEP0DMAADDR_MASK        0xFFFFFFFFUL                                  /**< Bit mask for USB_DIEP0DMAADDR */
#define _USB_DIEP0DMAADDR_DIEP0DMAADDR_DEFAULT     0x00000000UL                                  /**< Mode DEFAULT for USB_DIEP0DMAADDR */
#define USB_DIEP0DMAADDR_DIEP0DMAADDR_DEFAULT      (_USB_DIEP0DMAADDR_DIEP0DMAADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEP0DMAADDR */

/* Bit fields for USB DIEP0TXFSTS */
#define _USB_DIEP0TXFSTS_RESETVALUE                0x00000200UL                             /**< Default value for USB_DIEP0TXFSTS */
#define _USB_DIEP0TXFSTS_MASK                      0x0000FFFFUL                             /**< Mask for USB_DIEP0TXFSTS */
#define _USB_DIEP0TXFSTS_SPCAVAIL_SHIFT            0                                        /**< Shift value for USB_SPCAVAIL */
#define _USB_DIEP0TXFSTS_SPCAVAIL_MASK             0xFFFFUL                                 /**< Bit mask for USB_SPCAVAIL */
#define _USB_DIEP0TXFSTS_SPCAVAIL_DEFAULT          0x00000200UL                             /**< Mode DEFAULT for USB_DIEP0TXFSTS */
#define USB_DIEP0TXFSTS_SPCAVAIL_DEFAULT           (_USB_DIEP0TXFSTS_SPCAVAIL_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEP0TXFSTS */

/* Bit fields for USB DIEP_CTL */
#define _USB_DIEP_CTL_RESETVALUE                   0x00000000UL                             /**< Default value for USB_DIEP_CTL */
#define _USB_DIEP_CTL_MASK                         0xFFEF87FFUL                             /**< Mask for USB_DIEP_CTL */
#define _USB_DIEP_CTL_MPS_SHIFT                    0                                        /**< Shift value for USB_MPS */
#define _USB_DIEP_CTL_MPS_MASK                     0x7FFUL                                  /**< Bit mask for USB_MPS */
#define _USB_DIEP_CTL_MPS_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_MPS_DEFAULT                   (_USB_DIEP_CTL_MPS_DEFAULT << 0)         /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_USBACTEP                      (0x1UL << 15)                            /**< USB Active Endpoint */
#define _USB_DIEP_CTL_USBACTEP_SHIFT               15                                       /**< Shift value for USB_USBACTEP */
#define _USB_DIEP_CTL_USBACTEP_MASK                0x8000UL                                 /**< Bit mask for USB_USBACTEP */
#define _USB_DIEP_CTL_USBACTEP_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_USBACTEP_DEFAULT              (_USB_DIEP_CTL_USBACTEP_DEFAULT << 15)   /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_DPIDEOF                       (0x1UL << 16)                            /**< Endpoint Data PID / Even or Odd Frame */
#define _USB_DIEP_CTL_DPIDEOF_SHIFT                16                                       /**< Shift value for USB_DPIDEOF */
#define _USB_DIEP_CTL_DPIDEOF_MASK                 0x10000UL                                /**< Bit mask for USB_DPIDEOF */
#define _USB_DIEP_CTL_DPIDEOF_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define _USB_DIEP_CTL_DPIDEOF_DATA0EVEN            0x00000000UL                             /**< Mode DATA0EVEN for USB_DIEP_CTL */
#define _USB_DIEP_CTL_DPIDEOF_DATA1ODD             0x00000001UL                             /**< Mode DATA1ODD for USB_DIEP_CTL */
#define USB_DIEP_CTL_DPIDEOF_DEFAULT               (_USB_DIEP_CTL_DPIDEOF_DEFAULT << 16)    /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_DPIDEOF_DATA0EVEN             (_USB_DIEP_CTL_DPIDEOF_DATA0EVEN << 16)  /**< Shifted mode DATA0EVEN for USB_DIEP_CTL */
#define USB_DIEP_CTL_DPIDEOF_DATA1ODD              (_USB_DIEP_CTL_DPIDEOF_DATA1ODD << 16)   /**< Shifted mode DATA1ODD for USB_DIEP_CTL */
#define USB_DIEP_CTL_NAKSTS                        (0x1UL << 17)                            /**< NAK Status */
#define _USB_DIEP_CTL_NAKSTS_SHIFT                 17                                       /**< Shift value for USB_NAKSTS */
#define _USB_DIEP_CTL_NAKSTS_MASK                  0x20000UL                                /**< Bit mask for USB_NAKSTS */
#define _USB_DIEP_CTL_NAKSTS_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_NAKSTS_DEFAULT                (_USB_DIEP_CTL_NAKSTS_DEFAULT << 17)     /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define _USB_DIEP_CTL_EPTYPE_SHIFT                 18                                       /**< Shift value for USB_EPTYPE */
#define _USB_DIEP_CTL_EPTYPE_MASK                  0xC0000UL                                /**< Bit mask for USB_EPTYPE */
#define _USB_DIEP_CTL_EPTYPE_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define _USB_DIEP_CTL_EPTYPE_CONTROL               0x00000000UL                             /**< Mode CONTROL for USB_DIEP_CTL */
#define _USB_DIEP_CTL_EPTYPE_ISO                   0x00000001UL                             /**< Mode ISO for USB_DIEP_CTL */
#define _USB_DIEP_CTL_EPTYPE_BULK                  0x00000002UL                             /**< Mode BULK for USB_DIEP_CTL */
#define _USB_DIEP_CTL_EPTYPE_INT                   0x00000003UL                             /**< Mode INT for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPTYPE_DEFAULT                (_USB_DIEP_CTL_EPTYPE_DEFAULT << 18)     /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPTYPE_CONTROL                (_USB_DIEP_CTL_EPTYPE_CONTROL << 18)     /**< Shifted mode CONTROL for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPTYPE_ISO                    (_USB_DIEP_CTL_EPTYPE_ISO << 18)         /**< Shifted mode ISO for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPTYPE_BULK                   (_USB_DIEP_CTL_EPTYPE_BULK << 18)        /**< Shifted mode BULK for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPTYPE_INT                    (_USB_DIEP_CTL_EPTYPE_INT << 18)         /**< Shifted mode INT for USB_DIEP_CTL */
#define USB_DIEP_CTL_STALL                         (0x1UL << 21)                            /**< Handshake */
#define _USB_DIEP_CTL_STALL_SHIFT                  21                                       /**< Shift value for USB_STALL */
#define _USB_DIEP_CTL_STALL_MASK                   0x200000UL                               /**< Bit mask for USB_STALL */
#define _USB_DIEP_CTL_STALL_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_STALL_DEFAULT                 (_USB_DIEP_CTL_STALL_DEFAULT << 21)      /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define _USB_DIEP_CTL_TXFNUM_SHIFT                 22                                       /**< Shift value for USB_TXFNUM */
#define _USB_DIEP_CTL_TXFNUM_MASK                  0x3C00000UL                              /**< Bit mask for USB_TXFNUM */
#define _USB_DIEP_CTL_TXFNUM_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_TXFNUM_DEFAULT                (_USB_DIEP_CTL_TXFNUM_DEFAULT << 22)     /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_CNAK                          (0x1UL << 26)                            /**< Clear NAK */
#define _USB_DIEP_CTL_CNAK_SHIFT                   26                                       /**< Shift value for USB_CNAK */
#define _USB_DIEP_CTL_CNAK_MASK                    0x4000000UL                              /**< Bit mask for USB_CNAK */
#define _USB_DIEP_CTL_CNAK_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_CNAK_DEFAULT                  (_USB_DIEP_CTL_CNAK_DEFAULT << 26)       /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_SNAK                          (0x1UL << 27)                            /**< Set NAK */
#define _USB_DIEP_CTL_SNAK_SHIFT                   27                                       /**< Shift value for USB_SNAK */
#define _USB_DIEP_CTL_SNAK_MASK                    0x8000000UL                              /**< Bit mask for USB_SNAK */
#define _USB_DIEP_CTL_SNAK_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_SNAK_DEFAULT                  (_USB_DIEP_CTL_SNAK_DEFAULT << 27)       /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_SETD0PIDEF                    (0x1UL << 28)                            /**< Set DATA0 PID / Even Frame */
#define _USB_DIEP_CTL_SETD0PIDEF_SHIFT             28                                       /**< Shift value for USB_SETD0PIDEF */
#define _USB_DIEP_CTL_SETD0PIDEF_MASK              0x10000000UL                             /**< Bit mask for USB_SETD0PIDEF */
#define _USB_DIEP_CTL_SETD0PIDEF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_SETD0PIDEF_DEFAULT            (_USB_DIEP_CTL_SETD0PIDEF_DEFAULT << 28) /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_SETD1PIDOF                    (0x1UL << 29)                            /**< Set DATA1 PID / Odd Frame */
#define _USB_DIEP_CTL_SETD1PIDOF_SHIFT             29                                       /**< Shift value for USB_SETD1PIDOF */
#define _USB_DIEP_CTL_SETD1PIDOF_MASK              0x20000000UL                             /**< Bit mask for USB_SETD1PIDOF */
#define _USB_DIEP_CTL_SETD1PIDOF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_SETD1PIDOF_DEFAULT            (_USB_DIEP_CTL_SETD1PIDOF_DEFAULT << 29) /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPDIS                         (0x1UL << 30)                            /**< Endpoint Disable */
#define _USB_DIEP_CTL_EPDIS_SHIFT                  30                                       /**< Shift value for USB_EPDIS */
#define _USB_DIEP_CTL_EPDIS_MASK                   0x40000000UL                             /**< Bit mask for USB_EPDIS */
#define _USB_DIEP_CTL_EPDIS_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPDIS_DEFAULT                 (_USB_DIEP_CTL_EPDIS_DEFAULT << 30)      /**< Shifted mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPENA                         (0x1UL << 31)                            /**< Endpoint Enable */
#define _USB_DIEP_CTL_EPENA_SHIFT                  31                                       /**< Shift value for USB_EPENA */
#define _USB_DIEP_CTL_EPENA_MASK                   0x80000000UL                             /**< Bit mask for USB_EPENA */
#define _USB_DIEP_CTL_EPENA_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_CTL */
#define USB_DIEP_CTL_EPENA_DEFAULT                 (_USB_DIEP_CTL_EPENA_DEFAULT << 31)      /**< Shifted mode DEFAULT for USB_DIEP_CTL */

/* Bit fields for USB DIEP_INT */
#define _USB_DIEP_INT_RESETVALUE                   0x00000080UL                             /**< Default value for USB_DIEP_INT */
#define _USB_DIEP_INT_MASK                         0x000038DFUL                             /**< Mask for USB_DIEP_INT */
#define USB_DIEP_INT_XFERCOMPL                     (0x1UL << 0)                             /**< Transfer Completed Interrupt */
#define _USB_DIEP_INT_XFERCOMPL_SHIFT              0                                        /**< Shift value for USB_XFERCOMPL */
#define _USB_DIEP_INT_XFERCOMPL_MASK               0x1UL                                    /**< Bit mask for USB_XFERCOMPL */
#define _USB_DIEP_INT_XFERCOMPL_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_XFERCOMPL_DEFAULT             (_USB_DIEP_INT_XFERCOMPL_DEFAULT << 0)   /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_EPDISBLD                      (0x1UL << 1)                             /**< Endpoint Disabled Interrupt */
#define _USB_DIEP_INT_EPDISBLD_SHIFT               1                                        /**< Shift value for USB_EPDISBLD */
#define _USB_DIEP_INT_EPDISBLD_MASK                0x2UL                                    /**< Bit mask for USB_EPDISBLD */
#define _USB_DIEP_INT_EPDISBLD_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_EPDISBLD_DEFAULT              (_USB_DIEP_INT_EPDISBLD_DEFAULT << 1)    /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_AHBERR                        (0x1UL << 2)                             /**< AHB Error */
#define _USB_DIEP_INT_AHBERR_SHIFT                 2                                        /**< Shift value for USB_AHBERR */
#define _USB_DIEP_INT_AHBERR_MASK                  0x4UL                                    /**< Bit mask for USB_AHBERR */
#define _USB_DIEP_INT_AHBERR_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_AHBERR_DEFAULT                (_USB_DIEP_INT_AHBERR_DEFAULT << 2)      /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_TIMEOUT                       (0x1UL << 3)                             /**< Timeout Condition */
#define _USB_DIEP_INT_TIMEOUT_SHIFT                3                                        /**< Shift value for USB_TIMEOUT */
#define _USB_DIEP_INT_TIMEOUT_MASK                 0x8UL                                    /**< Bit mask for USB_TIMEOUT */
#define _USB_DIEP_INT_TIMEOUT_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_TIMEOUT_DEFAULT               (_USB_DIEP_INT_TIMEOUT_DEFAULT << 3)     /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_INTKNTXFEMP                   (0x1UL << 4)                             /**< IN Token Received When TxFIFO is Empty */
#define _USB_DIEP_INT_INTKNTXFEMP_SHIFT            4                                        /**< Shift value for USB_INTKNTXFEMP */
#define _USB_DIEP_INT_INTKNTXFEMP_MASK             0x10UL                                   /**< Bit mask for USB_INTKNTXFEMP */
#define _USB_DIEP_INT_INTKNTXFEMP_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_INTKNTXFEMP_DEFAULT           (_USB_DIEP_INT_INTKNTXFEMP_DEFAULT << 4) /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_INEPNAKEFF                    (0x1UL << 6)                             /**< IN Endpoint NAK Effective */
#define _USB_DIEP_INT_INEPNAKEFF_SHIFT             6                                        /**< Shift value for USB_INEPNAKEFF */
#define _USB_DIEP_INT_INEPNAKEFF_MASK              0x40UL                                   /**< Bit mask for USB_INEPNAKEFF */
#define _USB_DIEP_INT_INEPNAKEFF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_INEPNAKEFF_DEFAULT            (_USB_DIEP_INT_INEPNAKEFF_DEFAULT << 6)  /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_TXFEMP                        (0x1UL << 7)                             /**< Transmit FIFO Empty */
#define _USB_DIEP_INT_TXFEMP_SHIFT                 7                                        /**< Shift value for USB_TXFEMP */
#define _USB_DIEP_INT_TXFEMP_MASK                  0x80UL                                   /**< Bit mask for USB_TXFEMP */
#define _USB_DIEP_INT_TXFEMP_DEFAULT               0x00000001UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_TXFEMP_DEFAULT                (_USB_DIEP_INT_TXFEMP_DEFAULT << 7)      /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_PKTDRPSTS                     (0x1UL << 11)                            /**< Packet Drop Status */
#define _USB_DIEP_INT_PKTDRPSTS_SHIFT              11                                       /**< Shift value for USB_PKTDRPSTS */
#define _USB_DIEP_INT_PKTDRPSTS_MASK               0x800UL                                  /**< Bit mask for USB_PKTDRPSTS */
#define _USB_DIEP_INT_PKTDRPSTS_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_PKTDRPSTS_DEFAULT             (_USB_DIEP_INT_PKTDRPSTS_DEFAULT << 11)  /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_BBLEERR                       (0x1UL << 12)                            /**< NAK Interrupt */
#define _USB_DIEP_INT_BBLEERR_SHIFT                12                                       /**< Shift value for USB_BBLEERR */
#define _USB_DIEP_INT_BBLEERR_MASK                 0x1000UL                                 /**< Bit mask for USB_BBLEERR */
#define _USB_DIEP_INT_BBLEERR_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_BBLEERR_DEFAULT               (_USB_DIEP_INT_BBLEERR_DEFAULT << 12)    /**< Shifted mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_NAKINTRPT                     (0x1UL << 13)                            /**< NAK Interrupt */
#define _USB_DIEP_INT_NAKINTRPT_SHIFT              13                                       /**< Shift value for USB_NAKINTRPT */
#define _USB_DIEP_INT_NAKINTRPT_MASK               0x2000UL                                 /**< Bit mask for USB_NAKINTRPT */
#define _USB_DIEP_INT_NAKINTRPT_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_INT */
#define USB_DIEP_INT_NAKINTRPT_DEFAULT             (_USB_DIEP_INT_NAKINTRPT_DEFAULT << 13)  /**< Shifted mode DEFAULT for USB_DIEP_INT */

/* Bit fields for USB DIEP_TSIZ */
#define _USB_DIEP_TSIZ_RESETVALUE                  0x00000000UL                           /**< Default value for USB_DIEP_TSIZ */
#define _USB_DIEP_TSIZ_MASK                        0x7FFFFFFFUL                           /**< Mask for USB_DIEP_TSIZ */
#define _USB_DIEP_TSIZ_XFERSIZE_SHIFT              0                                      /**< Shift value for USB_XFERSIZE */
#define _USB_DIEP_TSIZ_XFERSIZE_MASK               0x7FFFFUL                              /**< Bit mask for USB_XFERSIZE */
#define _USB_DIEP_TSIZ_XFERSIZE_DEFAULT            0x00000000UL                           /**< Mode DEFAULT for USB_DIEP_TSIZ */
#define USB_DIEP_TSIZ_XFERSIZE_DEFAULT             (_USB_DIEP_TSIZ_XFERSIZE_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEP_TSIZ */
#define _USB_DIEP_TSIZ_PKTCNT_SHIFT                19                                     /**< Shift value for USB_PKTCNT */
#define _USB_DIEP_TSIZ_PKTCNT_MASK                 0x1FF80000UL                           /**< Bit mask for USB_PKTCNT */
#define _USB_DIEP_TSIZ_PKTCNT_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for USB_DIEP_TSIZ */
#define USB_DIEP_TSIZ_PKTCNT_DEFAULT               (_USB_DIEP_TSIZ_PKTCNT_DEFAULT << 19)  /**< Shifted mode DEFAULT for USB_DIEP_TSIZ */
#define _USB_DIEP_TSIZ_MC_SHIFT                    29                                     /**< Shift value for USB_MC */
#define _USB_DIEP_TSIZ_MC_MASK                     0x60000000UL                           /**< Bit mask for USB_MC */
#define _USB_DIEP_TSIZ_MC_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_DIEP_TSIZ */
#define USB_DIEP_TSIZ_MC_DEFAULT                   (_USB_DIEP_TSIZ_MC_DEFAULT << 29)      /**< Shifted mode DEFAULT for USB_DIEP_TSIZ */

/* Bit fields for USB DIEP_DMAADDR */
#define _USB_DIEP_DMAADDR_RESETVALUE               0x00000000UL                             /**< Default value for USB_DIEP_DMAADDR */
#define _USB_DIEP_DMAADDR_MASK                     0xFFFFFFFFUL                             /**< Mask for USB_DIEP_DMAADDR */
#define _USB_DIEP_DMAADDR_DMAADDR_SHIFT            0                                        /**< Shift value for USB_DMAADDR */
#define _USB_DIEP_DMAADDR_DMAADDR_MASK             0xFFFFFFFFUL                             /**< Bit mask for USB_DMAADDR */
#define _USB_DIEP_DMAADDR_DMAADDR_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USB_DIEP_DMAADDR */
#define USB_DIEP_DMAADDR_DMAADDR_DEFAULT           (_USB_DIEP_DMAADDR_DMAADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEP_DMAADDR */

/* Bit fields for USB DIEP_TXFSTS */
#define _USB_DIEP_TXFSTS_RESETVALUE                0x00000200UL                             /**< Default value for USB_DIEP_TXFSTS */
#define _USB_DIEP_TXFSTS_MASK                      0x0000FFFFUL                             /**< Mask for USB_DIEP_TXFSTS */
#define _USB_DIEP_TXFSTS_SPCAVAIL_SHIFT            0                                        /**< Shift value for USB_SPCAVAIL */
#define _USB_DIEP_TXFSTS_SPCAVAIL_MASK             0xFFFFUL                                 /**< Bit mask for USB_SPCAVAIL */
#define _USB_DIEP_TXFSTS_SPCAVAIL_DEFAULT          0x00000200UL                             /**< Mode DEFAULT for USB_DIEP_TXFSTS */
#define USB_DIEP_TXFSTS_SPCAVAIL_DEFAULT           (_USB_DIEP_TXFSTS_SPCAVAIL_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DIEP_TXFSTS */

/* Bit fields for USB DOEP0CTL */
#define _USB_DOEP0CTL_RESETVALUE                   0x00008000UL                           /**< Default value for USB_DOEP0CTL */
#define _USB_DOEP0CTL_MASK                         0xCC3E8003UL                           /**< Mask for USB_DOEP0CTL */
#define _USB_DOEP0CTL_MPS_SHIFT                    0                                      /**< Shift value for USB_MPS */
#define _USB_DOEP0CTL_MPS_MASK                     0x3UL                                  /**< Bit mask for USB_MPS */
#define _USB_DOEP0CTL_MPS_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define _USB_DOEP0CTL_MPS_64B                      0x00000000UL                           /**< Mode 64B for USB_DOEP0CTL */
#define _USB_DOEP0CTL_MPS_32B                      0x00000001UL                           /**< Mode 32B for USB_DOEP0CTL */
#define _USB_DOEP0CTL_MPS_16B                      0x00000002UL                           /**< Mode 16B for USB_DOEP0CTL */
#define _USB_DOEP0CTL_MPS_8B                       0x00000003UL                           /**< Mode 8B for USB_DOEP0CTL */
#define USB_DOEP0CTL_MPS_DEFAULT                   (_USB_DOEP0CTL_MPS_DEFAULT << 0)       /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_MPS_64B                       (_USB_DOEP0CTL_MPS_64B << 0)           /**< Shifted mode 64B for USB_DOEP0CTL */
#define USB_DOEP0CTL_MPS_32B                       (_USB_DOEP0CTL_MPS_32B << 0)           /**< Shifted mode 32B for USB_DOEP0CTL */
#define USB_DOEP0CTL_MPS_16B                       (_USB_DOEP0CTL_MPS_16B << 0)           /**< Shifted mode 16B for USB_DOEP0CTL */
#define USB_DOEP0CTL_MPS_8B                        (_USB_DOEP0CTL_MPS_8B << 0)            /**< Shifted mode 8B for USB_DOEP0CTL */
#define USB_DOEP0CTL_USBACTEP                      (0x1UL << 15)                          /**< USB Active Endpoint */
#define _USB_DOEP0CTL_USBACTEP_SHIFT               15                                     /**< Shift value for USB_USBACTEP */
#define _USB_DOEP0CTL_USBACTEP_MASK                0x8000UL                               /**< Bit mask for USB_USBACTEP */
#define _USB_DOEP0CTL_USBACTEP_DEFAULT             0x00000001UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_USBACTEP_DEFAULT              (_USB_DOEP0CTL_USBACTEP_DEFAULT << 15) /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_NAKSTS                        (0x1UL << 17)                          /**< NAK Status */
#define _USB_DOEP0CTL_NAKSTS_SHIFT                 17                                     /**< Shift value for USB_NAKSTS */
#define _USB_DOEP0CTL_NAKSTS_MASK                  0x20000UL                              /**< Bit mask for USB_NAKSTS */
#define _USB_DOEP0CTL_NAKSTS_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_NAKSTS_DEFAULT                (_USB_DOEP0CTL_NAKSTS_DEFAULT << 17)   /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define _USB_DOEP0CTL_EPTYPE_SHIFT                 18                                     /**< Shift value for USB_EPTYPE */
#define _USB_DOEP0CTL_EPTYPE_MASK                  0xC0000UL                              /**< Bit mask for USB_EPTYPE */
#define _USB_DOEP0CTL_EPTYPE_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_EPTYPE_DEFAULT                (_USB_DOEP0CTL_EPTYPE_DEFAULT << 18)   /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_SNP                           (0x1UL << 20)                          /**< Snoop Mode */
#define _USB_DOEP0CTL_SNP_SHIFT                    20                                     /**< Shift value for USB_SNP */
#define _USB_DOEP0CTL_SNP_MASK                     0x100000UL                             /**< Bit mask for USB_SNP */
#define _USB_DOEP0CTL_SNP_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_SNP_DEFAULT                   (_USB_DOEP0CTL_SNP_DEFAULT << 20)      /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_STALL                         (0x1UL << 21)                          /**< Handshake */
#define _USB_DOEP0CTL_STALL_SHIFT                  21                                     /**< Shift value for USB_STALL */
#define _USB_DOEP0CTL_STALL_MASK                   0x200000UL                             /**< Bit mask for USB_STALL */
#define _USB_DOEP0CTL_STALL_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_STALL_DEFAULT                 (_USB_DOEP0CTL_STALL_DEFAULT << 21)    /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_CNAK                          (0x1UL << 26)                          /**< Clear NAK */
#define _USB_DOEP0CTL_CNAK_SHIFT                   26                                     /**< Shift value for USB_CNAK */
#define _USB_DOEP0CTL_CNAK_MASK                    0x4000000UL                            /**< Bit mask for USB_CNAK */
#define _USB_DOEP0CTL_CNAK_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_CNAK_DEFAULT                  (_USB_DOEP0CTL_CNAK_DEFAULT << 26)     /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_SNAK                          (0x1UL << 27)                          /**< Set NAK */
#define _USB_DOEP0CTL_SNAK_SHIFT                   27                                     /**< Shift value for USB_SNAK */
#define _USB_DOEP0CTL_SNAK_MASK                    0x8000000UL                            /**< Bit mask for USB_SNAK */
#define _USB_DOEP0CTL_SNAK_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_SNAK_DEFAULT                  (_USB_DOEP0CTL_SNAK_DEFAULT << 27)     /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_EPDIS                         (0x1UL << 30)                          /**< Endpoint Disable */
#define _USB_DOEP0CTL_EPDIS_SHIFT                  30                                     /**< Shift value for USB_EPDIS */
#define _USB_DOEP0CTL_EPDIS_MASK                   0x40000000UL                           /**< Bit mask for USB_EPDIS */
#define _USB_DOEP0CTL_EPDIS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_EPDIS_DEFAULT                 (_USB_DOEP0CTL_EPDIS_DEFAULT << 30)    /**< Shifted mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_EPENA                         (0x1UL << 31)                          /**< Endpoint Enable */
#define _USB_DOEP0CTL_EPENA_SHIFT                  31                                     /**< Shift value for USB_EPENA */
#define _USB_DOEP0CTL_EPENA_MASK                   0x80000000UL                           /**< Bit mask for USB_EPENA */
#define _USB_DOEP0CTL_EPENA_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0CTL */
#define USB_DOEP0CTL_EPENA_DEFAULT                 (_USB_DOEP0CTL_EPENA_DEFAULT << 31)    /**< Shifted mode DEFAULT for USB_DOEP0CTL */

/* Bit fields for USB DOEP0INT */
#define _USB_DOEP0INT_RESETVALUE                   0x00000000UL                                /**< Default value for USB_DOEP0INT */
#define _USB_DOEP0INT_MASK                         0x0000B87FUL                                /**< Mask for USB_DOEP0INT */
#define USB_DOEP0INT_XFERCOMPL                     (0x1UL << 0)                                /**< Transfer Completed Interrupt */
#define _USB_DOEP0INT_XFERCOMPL_SHIFT              0                                           /**< Shift value for USB_XFERCOMPL */
#define _USB_DOEP0INT_XFERCOMPL_MASK               0x1UL                                       /**< Bit mask for USB_XFERCOMPL */
#define _USB_DOEP0INT_XFERCOMPL_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_XFERCOMPL_DEFAULT             (_USB_DOEP0INT_XFERCOMPL_DEFAULT << 0)      /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_EPDISBLD                      (0x1UL << 1)                                /**< Endpoint Disabled Interrupt */
#define _USB_DOEP0INT_EPDISBLD_SHIFT               1                                           /**< Shift value for USB_EPDISBLD */
#define _USB_DOEP0INT_EPDISBLD_MASK                0x2UL                                       /**< Bit mask for USB_EPDISBLD */
#define _USB_DOEP0INT_EPDISBLD_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_EPDISBLD_DEFAULT              (_USB_DOEP0INT_EPDISBLD_DEFAULT << 1)       /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_AHBERR                        (0x1UL << 2)                                /**< AHB Error */
#define _USB_DOEP0INT_AHBERR_SHIFT                 2                                           /**< Shift value for USB_AHBERR */
#define _USB_DOEP0INT_AHBERR_MASK                  0x4UL                                       /**< Bit mask for USB_AHBERR */
#define _USB_DOEP0INT_AHBERR_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_AHBERR_DEFAULT                (_USB_DOEP0INT_AHBERR_DEFAULT << 2)         /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_SETUP                         (0x1UL << 3)                                /**< Setup Phase Done */
#define _USB_DOEP0INT_SETUP_SHIFT                  3                                           /**< Shift value for USB_SETUP */
#define _USB_DOEP0INT_SETUP_MASK                   0x8UL                                       /**< Bit mask for USB_SETUP */
#define _USB_DOEP0INT_SETUP_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_SETUP_DEFAULT                 (_USB_DOEP0INT_SETUP_DEFAULT << 3)          /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_OUTTKNEPDIS                   (0x1UL << 4)                                /**< OUT Token Received When Endpoint Disabled */
#define _USB_DOEP0INT_OUTTKNEPDIS_SHIFT            4                                           /**< Shift value for USB_OUTTKNEPDIS */
#define _USB_DOEP0INT_OUTTKNEPDIS_MASK             0x10UL                                      /**< Bit mask for USB_OUTTKNEPDIS */
#define _USB_DOEP0INT_OUTTKNEPDIS_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_OUTTKNEPDIS_DEFAULT           (_USB_DOEP0INT_OUTTKNEPDIS_DEFAULT << 4)    /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_STSPHSERCVD                   (0x1UL << 5)                                /**< Status Phase Received For Control Write */
#define _USB_DOEP0INT_STSPHSERCVD_SHIFT            5                                           /**< Shift value for USB_STSPHSERCVD */
#define _USB_DOEP0INT_STSPHSERCVD_MASK             0x20UL                                      /**< Bit mask for USB_STSPHSERCVD */
#define _USB_DOEP0INT_STSPHSERCVD_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_STSPHSERCVD_DEFAULT           (_USB_DOEP0INT_STSPHSERCVD_DEFAULT << 5)    /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_BACK2BACKSETUP                (0x1UL << 6)                                /**< Back-to-Back SETUP Packets Received */
#define _USB_DOEP0INT_BACK2BACKSETUP_SHIFT         6                                           /**< Shift value for USB_BACK2BACKSETUP */
#define _USB_DOEP0INT_BACK2BACKSETUP_MASK          0x40UL                                      /**< Bit mask for USB_BACK2BACKSETUP */
#define _USB_DOEP0INT_BACK2BACKSETUP_DEFAULT       0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_BACK2BACKSETUP_DEFAULT        (_USB_DOEP0INT_BACK2BACKSETUP_DEFAULT << 6) /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_PKTDRPSTS                     (0x1UL << 11)                               /**< Packet Drop Status */
#define _USB_DOEP0INT_PKTDRPSTS_SHIFT              11                                          /**< Shift value for USB_PKTDRPSTS */
#define _USB_DOEP0INT_PKTDRPSTS_MASK               0x800UL                                     /**< Bit mask for USB_PKTDRPSTS */
#define _USB_DOEP0INT_PKTDRPSTS_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_PKTDRPSTS_DEFAULT             (_USB_DOEP0INT_PKTDRPSTS_DEFAULT << 11)     /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_BBLEERR                       (0x1UL << 12)                               /**< NAK Interrupt */
#define _USB_DOEP0INT_BBLEERR_SHIFT                12                                          /**< Shift value for USB_BBLEERR */
#define _USB_DOEP0INT_BBLEERR_MASK                 0x1000UL                                    /**< Bit mask for USB_BBLEERR */
#define _USB_DOEP0INT_BBLEERR_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_BBLEERR_DEFAULT               (_USB_DOEP0INT_BBLEERR_DEFAULT << 12)       /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_NAKINTRPT                     (0x1UL << 13)                               /**< NAK Interrupt */
#define _USB_DOEP0INT_NAKINTRPT_SHIFT              13                                          /**< Shift value for USB_NAKINTRPT */
#define _USB_DOEP0INT_NAKINTRPT_MASK               0x2000UL                                    /**< Bit mask for USB_NAKINTRPT */
#define _USB_DOEP0INT_NAKINTRPT_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_NAKINTRPT_DEFAULT             (_USB_DOEP0INT_NAKINTRPT_DEFAULT << 13)     /**< Shifted mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_STUPPKTRCVD                   (0x1UL << 15)                               /**< Setup Packet Received */
#define _USB_DOEP0INT_STUPPKTRCVD_SHIFT            15                                          /**< Shift value for USB_STUPPKTRCVD */
#define _USB_DOEP0INT_STUPPKTRCVD_MASK             0x8000UL                                    /**< Bit mask for USB_STUPPKTRCVD */
#define _USB_DOEP0INT_STUPPKTRCVD_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_DOEP0INT */
#define USB_DOEP0INT_STUPPKTRCVD_DEFAULT           (_USB_DOEP0INT_STUPPKTRCVD_DEFAULT << 15)   /**< Shifted mode DEFAULT for USB_DOEP0INT */

/* Bit fields for USB DOEP0TSIZ */
#define _USB_DOEP0TSIZ_RESETVALUE                  0x00000000UL                           /**< Default value for USB_DOEP0TSIZ */
#define _USB_DOEP0TSIZ_MASK                        0x6008007FUL                           /**< Mask for USB_DOEP0TSIZ */
#define _USB_DOEP0TSIZ_XFERSIZE_SHIFT              0                                      /**< Shift value for USB_XFERSIZE */
#define _USB_DOEP0TSIZ_XFERSIZE_MASK               0x7FUL                                 /**< Bit mask for USB_XFERSIZE */
#define _USB_DOEP0TSIZ_XFERSIZE_DEFAULT            0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0TSIZ */
#define USB_DOEP0TSIZ_XFERSIZE_DEFAULT             (_USB_DOEP0TSIZ_XFERSIZE_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DOEP0TSIZ */
#define USB_DOEP0TSIZ_PKTCNT                       (0x1UL << 19)                          /**< Packet Count */
#define _USB_DOEP0TSIZ_PKTCNT_SHIFT                19                                     /**< Shift value for USB_PKTCNT */
#define _USB_DOEP0TSIZ_PKTCNT_MASK                 0x80000UL                              /**< Bit mask for USB_PKTCNT */
#define _USB_DOEP0TSIZ_PKTCNT_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0TSIZ */
#define USB_DOEP0TSIZ_PKTCNT_DEFAULT               (_USB_DOEP0TSIZ_PKTCNT_DEFAULT << 19)  /**< Shifted mode DEFAULT for USB_DOEP0TSIZ */
#define _USB_DOEP0TSIZ_SUPCNT_SHIFT                29                                     /**< Shift value for USB_SUPCNT */
#define _USB_DOEP0TSIZ_SUPCNT_MASK                 0x60000000UL                           /**< Bit mask for USB_SUPCNT */
#define _USB_DOEP0TSIZ_SUPCNT_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for USB_DOEP0TSIZ */
#define USB_DOEP0TSIZ_SUPCNT_DEFAULT               (_USB_DOEP0TSIZ_SUPCNT_DEFAULT << 29)  /**< Shifted mode DEFAULT for USB_DOEP0TSIZ */

/* Bit fields for USB DOEP0DMAADDR */
#define _USB_DOEP0DMAADDR_RESETVALUE               0x00000000UL                                  /**< Default value for USB_DOEP0DMAADDR */
#define _USB_DOEP0DMAADDR_MASK                     0xFFFFFFFFUL                                  /**< Mask for USB_DOEP0DMAADDR */
#define _USB_DOEP0DMAADDR_DOEP0DMAADDR_SHIFT       0                                             /**< Shift value for USB_DOEP0DMAADDR */
#define _USB_DOEP0DMAADDR_DOEP0DMAADDR_MASK        0xFFFFFFFFUL                                  /**< Bit mask for USB_DOEP0DMAADDR */
#define _USB_DOEP0DMAADDR_DOEP0DMAADDR_DEFAULT     0x00000000UL                                  /**< Mode DEFAULT for USB_DOEP0DMAADDR */
#define USB_DOEP0DMAADDR_DOEP0DMAADDR_DEFAULT      (_USB_DOEP0DMAADDR_DOEP0DMAADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DOEP0DMAADDR */

/* Bit fields for USB DOEP_CTL */
#define _USB_DOEP_CTL_RESETVALUE                   0x00000000UL                             /**< Default value for USB_DOEP_CTL */
#define _USB_DOEP_CTL_MASK                         0xFC3F87FFUL                             /**< Mask for USB_DOEP_CTL */
#define _USB_DOEP_CTL_MPS_SHIFT                    0                                        /**< Shift value for USB_MPS */
#define _USB_DOEP_CTL_MPS_MASK                     0x7FFUL                                  /**< Bit mask for USB_MPS */
#define _USB_DOEP_CTL_MPS_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_MPS_DEFAULT                   (_USB_DOEP_CTL_MPS_DEFAULT << 0)         /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_USBACTEP                      (0x1UL << 15)                            /**< USB Active Endpoint */
#define _USB_DOEP_CTL_USBACTEP_SHIFT               15                                       /**< Shift value for USB_USBACTEP */
#define _USB_DOEP_CTL_USBACTEP_MASK                0x8000UL                                 /**< Bit mask for USB_USBACTEP */
#define _USB_DOEP_CTL_USBACTEP_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_USBACTEP_DEFAULT              (_USB_DOEP_CTL_USBACTEP_DEFAULT << 15)   /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_DPIDEOF                       (0x1UL << 16)                            /**< Endpoint Data PID / Even-odd Frame */
#define _USB_DOEP_CTL_DPIDEOF_SHIFT                16                                       /**< Shift value for USB_DPIDEOF */
#define _USB_DOEP_CTL_DPIDEOF_MASK                 0x10000UL                                /**< Bit mask for USB_DPIDEOF */
#define _USB_DOEP_CTL_DPIDEOF_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define _USB_DOEP_CTL_DPIDEOF_DATA0EVEN            0x00000000UL                             /**< Mode DATA0EVEN for USB_DOEP_CTL */
#define _USB_DOEP_CTL_DPIDEOF_DATA1ODD             0x00000001UL                             /**< Mode DATA1ODD for USB_DOEP_CTL */
#define USB_DOEP_CTL_DPIDEOF_DEFAULT               (_USB_DOEP_CTL_DPIDEOF_DEFAULT << 16)    /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_DPIDEOF_DATA0EVEN             (_USB_DOEP_CTL_DPIDEOF_DATA0EVEN << 16)  /**< Shifted mode DATA0EVEN for USB_DOEP_CTL */
#define USB_DOEP_CTL_DPIDEOF_DATA1ODD              (_USB_DOEP_CTL_DPIDEOF_DATA1ODD << 16)   /**< Shifted mode DATA1ODD for USB_DOEP_CTL */
#define USB_DOEP_CTL_NAKSTS                        (0x1UL << 17)                            /**< NAK Status */
#define _USB_DOEP_CTL_NAKSTS_SHIFT                 17                                       /**< Shift value for USB_NAKSTS */
#define _USB_DOEP_CTL_NAKSTS_MASK                  0x20000UL                                /**< Bit mask for USB_NAKSTS */
#define _USB_DOEP_CTL_NAKSTS_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_NAKSTS_DEFAULT                (_USB_DOEP_CTL_NAKSTS_DEFAULT << 17)     /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define _USB_DOEP_CTL_EPTYPE_SHIFT                 18                                       /**< Shift value for USB_EPTYPE */
#define _USB_DOEP_CTL_EPTYPE_MASK                  0xC0000UL                                /**< Bit mask for USB_EPTYPE */
#define _USB_DOEP_CTL_EPTYPE_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define _USB_DOEP_CTL_EPTYPE_CONTROL               0x00000000UL                             /**< Mode CONTROL for USB_DOEP_CTL */
#define _USB_DOEP_CTL_EPTYPE_ISO                   0x00000001UL                             /**< Mode ISO for USB_DOEP_CTL */
#define _USB_DOEP_CTL_EPTYPE_BULK                  0x00000002UL                             /**< Mode BULK for USB_DOEP_CTL */
#define _USB_DOEP_CTL_EPTYPE_INT                   0x00000003UL                             /**< Mode INT for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPTYPE_DEFAULT                (_USB_DOEP_CTL_EPTYPE_DEFAULT << 18)     /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPTYPE_CONTROL                (_USB_DOEP_CTL_EPTYPE_CONTROL << 18)     /**< Shifted mode CONTROL for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPTYPE_ISO                    (_USB_DOEP_CTL_EPTYPE_ISO << 18)         /**< Shifted mode ISO for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPTYPE_BULK                   (_USB_DOEP_CTL_EPTYPE_BULK << 18)        /**< Shifted mode BULK for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPTYPE_INT                    (_USB_DOEP_CTL_EPTYPE_INT << 18)         /**< Shifted mode INT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SNP                           (0x1UL << 20)                            /**< Snoop Mode */
#define _USB_DOEP_CTL_SNP_SHIFT                    20                                       /**< Shift value for USB_SNP */
#define _USB_DOEP_CTL_SNP_MASK                     0x100000UL                               /**< Bit mask for USB_SNP */
#define _USB_DOEP_CTL_SNP_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SNP_DEFAULT                   (_USB_DOEP_CTL_SNP_DEFAULT << 20)        /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_STALL                         (0x1UL << 21)                            /**< STALL Handshake */
#define _USB_DOEP_CTL_STALL_SHIFT                  21                                       /**< Shift value for USB_STALL */
#define _USB_DOEP_CTL_STALL_MASK                   0x200000UL                               /**< Bit mask for USB_STALL */
#define _USB_DOEP_CTL_STALL_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_STALL_DEFAULT                 (_USB_DOEP_CTL_STALL_DEFAULT << 21)      /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_CNAK                          (0x1UL << 26)                            /**< Clear NAK */
#define _USB_DOEP_CTL_CNAK_SHIFT                   26                                       /**< Shift value for USB_CNAK */
#define _USB_DOEP_CTL_CNAK_MASK                    0x4000000UL                              /**< Bit mask for USB_CNAK */
#define _USB_DOEP_CTL_CNAK_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_CNAK_DEFAULT                  (_USB_DOEP_CTL_CNAK_DEFAULT << 26)       /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SNAK                          (0x1UL << 27)                            /**< Set NAK */
#define _USB_DOEP_CTL_SNAK_SHIFT                   27                                       /**< Shift value for USB_SNAK */
#define _USB_DOEP_CTL_SNAK_MASK                    0x8000000UL                              /**< Bit mask for USB_SNAK */
#define _USB_DOEP_CTL_SNAK_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SNAK_DEFAULT                  (_USB_DOEP_CTL_SNAK_DEFAULT << 27)       /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SETD0PIDEF                    (0x1UL << 28)                            /**< Set DATA0 PID / Even Frame */
#define _USB_DOEP_CTL_SETD0PIDEF_SHIFT             28                                       /**< Shift value for USB_SETD0PIDEF */
#define _USB_DOEP_CTL_SETD0PIDEF_MASK              0x10000000UL                             /**< Bit mask for USB_SETD0PIDEF */
#define _USB_DOEP_CTL_SETD0PIDEF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SETD0PIDEF_DEFAULT            (_USB_DOEP_CTL_SETD0PIDEF_DEFAULT << 28) /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SETD1PIDOF                    (0x1UL << 29)                            /**< Set DATA1 PID / Odd Frame */
#define _USB_DOEP_CTL_SETD1PIDOF_SHIFT             29                                       /**< Shift value for USB_SETD1PIDOF */
#define _USB_DOEP_CTL_SETD1PIDOF_MASK              0x20000000UL                             /**< Bit mask for USB_SETD1PIDOF */
#define _USB_DOEP_CTL_SETD1PIDOF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_SETD1PIDOF_DEFAULT            (_USB_DOEP_CTL_SETD1PIDOF_DEFAULT << 29) /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPDIS                         (0x1UL << 30)                            /**< Endpoint Disable */
#define _USB_DOEP_CTL_EPDIS_SHIFT                  30                                       /**< Shift value for USB_EPDIS */
#define _USB_DOEP_CTL_EPDIS_MASK                   0x40000000UL                             /**< Bit mask for USB_EPDIS */
#define _USB_DOEP_CTL_EPDIS_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPDIS_DEFAULT                 (_USB_DOEP_CTL_EPDIS_DEFAULT << 30)      /**< Shifted mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPENA                         (0x1UL << 31)                            /**< Endpoint Enable */
#define _USB_DOEP_CTL_EPENA_SHIFT                  31                                       /**< Shift value for USB_EPENA */
#define _USB_DOEP_CTL_EPENA_MASK                   0x80000000UL                             /**< Bit mask for USB_EPENA */
#define _USB_DOEP_CTL_EPENA_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_CTL */
#define USB_DOEP_CTL_EPENA_DEFAULT                 (_USB_DOEP_CTL_EPENA_DEFAULT << 31)      /**< Shifted mode DEFAULT for USB_DOEP_CTL */

/* Bit fields for USB DOEP_INT */
#define _USB_DOEP_INT_RESETVALUE                   0x00000000UL                                /**< Default value for USB_DOEP_INT */
#define _USB_DOEP_INT_MASK                         0x0000B87FUL                                /**< Mask for USB_DOEP_INT */
#define USB_DOEP_INT_XFERCOMPL                     (0x1UL << 0)                                /**< Transfer Completed Interrupt */
#define _USB_DOEP_INT_XFERCOMPL_SHIFT              0                                           /**< Shift value for USB_XFERCOMPL */
#define _USB_DOEP_INT_XFERCOMPL_MASK               0x1UL                                       /**< Bit mask for USB_XFERCOMPL */
#define _USB_DOEP_INT_XFERCOMPL_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_XFERCOMPL_DEFAULT             (_USB_DOEP_INT_XFERCOMPL_DEFAULT << 0)      /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_EPDISBLD                      (0x1UL << 1)                                /**< Endpoint Disabled Interrupt */
#define _USB_DOEP_INT_EPDISBLD_SHIFT               1                                           /**< Shift value for USB_EPDISBLD */
#define _USB_DOEP_INT_EPDISBLD_MASK                0x2UL                                       /**< Bit mask for USB_EPDISBLD */
#define _USB_DOEP_INT_EPDISBLD_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_EPDISBLD_DEFAULT              (_USB_DOEP_INT_EPDISBLD_DEFAULT << 1)       /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_AHBERR                        (0x1UL << 2)                                /**< AHB Error */
#define _USB_DOEP_INT_AHBERR_SHIFT                 2                                           /**< Shift value for USB_AHBERR */
#define _USB_DOEP_INT_AHBERR_MASK                  0x4UL                                       /**< Bit mask for USB_AHBERR */
#define _USB_DOEP_INT_AHBERR_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_AHBERR_DEFAULT                (_USB_DOEP_INT_AHBERR_DEFAULT << 2)         /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_SETUP                         (0x1UL << 3)                                /**< Setup Phase Done */
#define _USB_DOEP_INT_SETUP_SHIFT                  3                                           /**< Shift value for USB_SETUP */
#define _USB_DOEP_INT_SETUP_MASK                   0x8UL                                       /**< Bit mask for USB_SETUP */
#define _USB_DOEP_INT_SETUP_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_SETUP_DEFAULT                 (_USB_DOEP_INT_SETUP_DEFAULT << 3)          /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_OUTTKNEPDIS                   (0x1UL << 4)                                /**< OUT Token Received When Endpoint Disabled */
#define _USB_DOEP_INT_OUTTKNEPDIS_SHIFT            4                                           /**< Shift value for USB_OUTTKNEPDIS */
#define _USB_DOEP_INT_OUTTKNEPDIS_MASK             0x10UL                                      /**< Bit mask for USB_OUTTKNEPDIS */
#define _USB_DOEP_INT_OUTTKNEPDIS_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_OUTTKNEPDIS_DEFAULT           (_USB_DOEP_INT_OUTTKNEPDIS_DEFAULT << 4)    /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_STSPHSERCVD                   (0x1UL << 5)                                /**< Status Phase Received For Control Write */
#define _USB_DOEP_INT_STSPHSERCVD_SHIFT            5                                           /**< Shift value for USB_STSPHSERCVD */
#define _USB_DOEP_INT_STSPHSERCVD_MASK             0x20UL                                      /**< Bit mask for USB_STSPHSERCVD */
#define _USB_DOEP_INT_STSPHSERCVD_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_STSPHSERCVD_DEFAULT           (_USB_DOEP_INT_STSPHSERCVD_DEFAULT << 5)    /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_BACK2BACKSETUP                (0x1UL << 6)                                /**< Back-to-Back SETUP Packets Received */
#define _USB_DOEP_INT_BACK2BACKSETUP_SHIFT         6                                           /**< Shift value for USB_BACK2BACKSETUP */
#define _USB_DOEP_INT_BACK2BACKSETUP_MASK          0x40UL                                      /**< Bit mask for USB_BACK2BACKSETUP */
#define _USB_DOEP_INT_BACK2BACKSETUP_DEFAULT       0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_BACK2BACKSETUP_DEFAULT        (_USB_DOEP_INT_BACK2BACKSETUP_DEFAULT << 6) /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_PKTDRPSTS                     (0x1UL << 11)                               /**< Packet Drop Status */
#define _USB_DOEP_INT_PKTDRPSTS_SHIFT              11                                          /**< Shift value for USB_PKTDRPSTS */
#define _USB_DOEP_INT_PKTDRPSTS_MASK               0x800UL                                     /**< Bit mask for USB_PKTDRPSTS */
#define _USB_DOEP_INT_PKTDRPSTS_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_PKTDRPSTS_DEFAULT             (_USB_DOEP_INT_PKTDRPSTS_DEFAULT << 11)     /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_BBLEERR                       (0x1UL << 12)                               /**< Babble Error */
#define _USB_DOEP_INT_BBLEERR_SHIFT                12                                          /**< Shift value for USB_BBLEERR */
#define _USB_DOEP_INT_BBLEERR_MASK                 0x1000UL                                    /**< Bit mask for USB_BBLEERR */
#define _USB_DOEP_INT_BBLEERR_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_BBLEERR_DEFAULT               (_USB_DOEP_INT_BBLEERR_DEFAULT << 12)       /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_NAKINTRPT                     (0x1UL << 13)                               /**< NAK Interrupt */
#define _USB_DOEP_INT_NAKINTRPT_SHIFT              13                                          /**< Shift value for USB_NAKINTRPT */
#define _USB_DOEP_INT_NAKINTRPT_MASK               0x2000UL                                    /**< Bit mask for USB_NAKINTRPT */
#define _USB_DOEP_INT_NAKINTRPT_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_NAKINTRPT_DEFAULT             (_USB_DOEP_INT_NAKINTRPT_DEFAULT << 13)     /**< Shifted mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_STUPPKTRCVD                   (0x1UL << 15)                               /**< Setup Packet Received */
#define _USB_DOEP_INT_STUPPKTRCVD_SHIFT            15                                          /**< Shift value for USB_STUPPKTRCVD */
#define _USB_DOEP_INT_STUPPKTRCVD_MASK             0x8000UL                                    /**< Bit mask for USB_STUPPKTRCVD */
#define _USB_DOEP_INT_STUPPKTRCVD_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_INT */
#define USB_DOEP_INT_STUPPKTRCVD_DEFAULT           (_USB_DOEP_INT_STUPPKTRCVD_DEFAULT << 15)   /**< Shifted mode DEFAULT for USB_DOEP_INT */

/* Bit fields for USB DOEP_TSIZ */
#define _USB_DOEP_TSIZ_RESETVALUE                  0x00000000UL                                /**< Default value for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_MASK                        0x7FFFFFFFUL                                /**< Mask for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_XFERSIZE_SHIFT              0                                           /**< Shift value for USB_XFERSIZE */
#define _USB_DOEP_TSIZ_XFERSIZE_MASK               0x7FFFFUL                                   /**< Bit mask for USB_XFERSIZE */
#define _USB_DOEP_TSIZ_XFERSIZE_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_TSIZ */
#define USB_DOEP_TSIZ_XFERSIZE_DEFAULT             (_USB_DOEP_TSIZ_XFERSIZE_DEFAULT << 0)      /**< Shifted mode DEFAULT for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_PKTCNT_SHIFT                19                                          /**< Shift value for USB_PKTCNT */
#define _USB_DOEP_TSIZ_PKTCNT_MASK                 0x1FF80000UL                                /**< Bit mask for USB_PKTCNT */
#define _USB_DOEP_TSIZ_PKTCNT_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_TSIZ */
#define USB_DOEP_TSIZ_PKTCNT_DEFAULT               (_USB_DOEP_TSIZ_PKTCNT_DEFAULT << 19)       /**< Shifted mode DEFAULT for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_RXDPIDSUPCNT_SHIFT          29                                          /**< Shift value for USB_RXDPIDSUPCNT */
#define _USB_DOEP_TSIZ_RXDPIDSUPCNT_MASK           0x60000000UL                                /**< Bit mask for USB_RXDPIDSUPCNT */
#define _USB_DOEP_TSIZ_RXDPIDSUPCNT_DEFAULT        0x00000000UL                                /**< Mode DEFAULT for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA0          0x00000000UL                                /**< Mode DATA0 for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA2          0x00000001UL                                /**< Mode DATA2 for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA1          0x00000002UL                                /**< Mode DATA1 for USB_DOEP_TSIZ */
#define _USB_DOEP_TSIZ_RXDPIDSUPCNT_MDATA          0x00000003UL                                /**< Mode MDATA for USB_DOEP_TSIZ */
#define USB_DOEP_TSIZ_RXDPIDSUPCNT_DEFAULT         (_USB_DOEP_TSIZ_RXDPIDSUPCNT_DEFAULT << 29) /**< Shifted mode DEFAULT for USB_DOEP_TSIZ */
#define USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA0           (_USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA0 << 29)   /**< Shifted mode DATA0 for USB_DOEP_TSIZ */
#define USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA2           (_USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA2 << 29)   /**< Shifted mode DATA2 for USB_DOEP_TSIZ */
#define USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA1           (_USB_DOEP_TSIZ_RXDPIDSUPCNT_DATA1 << 29)   /**< Shifted mode DATA1 for USB_DOEP_TSIZ */
#define USB_DOEP_TSIZ_RXDPIDSUPCNT_MDATA           (_USB_DOEP_TSIZ_RXDPIDSUPCNT_MDATA << 29)   /**< Shifted mode MDATA for USB_DOEP_TSIZ */

/* Bit fields for USB DOEP_DMAADDR */
#define _USB_DOEP_DMAADDR_RESETVALUE               0x00000000UL                             /**< Default value for USB_DOEP_DMAADDR */
#define _USB_DOEP_DMAADDR_MASK                     0xFFFFFFFFUL                             /**< Mask for USB_DOEP_DMAADDR */
#define _USB_DOEP_DMAADDR_DMAADDR_SHIFT            0                                        /**< Shift value for USB_DMAADDR */
#define _USB_DOEP_DMAADDR_DMAADDR_MASK             0xFFFFFFFFUL                             /**< Bit mask for USB_DMAADDR */
#define _USB_DOEP_DMAADDR_DMAADDR_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USB_DOEP_DMAADDR */
#define USB_DOEP_DMAADDR_DMAADDR_DEFAULT           (_USB_DOEP_DMAADDR_DMAADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_DOEP_DMAADDR */

/* Bit fields for USB PCGCCTL */
#define _USB_PCGCCTL_RESETVALUE                    0x00000000UL                              /**< Default value for USB_PCGCCTL */
#define _USB_PCGCCTL_MASK                          0x0000004FUL                              /**< Mask for USB_PCGCCTL */
#define USB_PCGCCTL_STOPPCLK                       (0x1UL << 0)                              /**< Stop PHY clock */
#define _USB_PCGCCTL_STOPPCLK_SHIFT                0                                         /**< Shift value for USB_STOPPCLK */
#define _USB_PCGCCTL_STOPPCLK_MASK                 0x1UL                                     /**< Bit mask for USB_STOPPCLK */
#define _USB_PCGCCTL_STOPPCLK_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_STOPPCLK_DEFAULT               (_USB_PCGCCTL_STOPPCLK_DEFAULT << 0)      /**< Shifted mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_GATEHCLK                       (0x1UL << 1)                              /**< Gate HCLK */
#define _USB_PCGCCTL_GATEHCLK_SHIFT                1                                         /**< Shift value for USB_GATEHCLK */
#define _USB_PCGCCTL_GATEHCLK_MASK                 0x2UL                                     /**< Bit mask for USB_GATEHCLK */
#define _USB_PCGCCTL_GATEHCLK_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_GATEHCLK_DEFAULT               (_USB_PCGCCTL_GATEHCLK_DEFAULT << 1)      /**< Shifted mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_PWRCLMP                        (0x1UL << 2)                              /**< Power Clamp */
#define _USB_PCGCCTL_PWRCLMP_SHIFT                 2                                         /**< Shift value for USB_PWRCLMP */
#define _USB_PCGCCTL_PWRCLMP_MASK                  0x4UL                                     /**< Bit mask for USB_PWRCLMP */
#define _USB_PCGCCTL_PWRCLMP_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_PWRCLMP_DEFAULT                (_USB_PCGCCTL_PWRCLMP_DEFAULT << 2)       /**< Shifted mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_RSTPDWNMODULE                  (0x1UL << 3)                              /**< Reset Power-Down Modules */
#define _USB_PCGCCTL_RSTPDWNMODULE_SHIFT           3                                         /**< Shift value for USB_RSTPDWNMODULE */
#define _USB_PCGCCTL_RSTPDWNMODULE_MASK            0x8UL                                     /**< Bit mask for USB_RSTPDWNMODULE */
#define _USB_PCGCCTL_RSTPDWNMODULE_DEFAULT         0x00000000UL                              /**< Mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_RSTPDWNMODULE_DEFAULT          (_USB_PCGCCTL_RSTPDWNMODULE_DEFAULT << 3) /**< Shifted mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_PHYSLEEP                       (0x1UL << 6)                              /**< PHY In Sleep */
#define _USB_PCGCCTL_PHYSLEEP_SHIFT                6                                         /**< Shift value for USB_PHYSLEEP */
#define _USB_PCGCCTL_PHYSLEEP_MASK                 0x40UL                                    /**< Bit mask for USB_PHYSLEEP */
#define _USB_PCGCCTL_PHYSLEEP_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for USB_PCGCCTL */
#define USB_PCGCCTL_PHYSLEEP_DEFAULT               (_USB_PCGCCTL_PHYSLEEP_DEFAULT << 6)      /**< Shifted mode DEFAULT for USB_PCGCCTL */

/* Bit fields for USB FIFO0D */
#define _USB_FIFO0D_RESETVALUE                     0x00000000UL                      /**< Default value for USB_FIFO0D */
#define _USB_FIFO0D_MASK                           0xFFFFFFFFUL                      /**< Mask for USB_FIFO0D */
#define _USB_FIFO0D_FIFO0D_SHIFT                   0                                 /**< Shift value for USB_FIFO0D */
#define _USB_FIFO0D_FIFO0D_MASK                    0xFFFFFFFFUL                      /**< Bit mask for USB_FIFO0D */
#define _USB_FIFO0D_FIFO0D_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USB_FIFO0D */
#define USB_FIFO0D_FIFO0D_DEFAULT                  (_USB_FIFO0D_FIFO0D_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_FIFO0D */

/* Bit fields for USB FIFO1D */
#define _USB_FIFO1D_RESETVALUE                     0x00000000UL                      /**< Default value for USB_FIFO1D */
#define _USB_FIFO1D_MASK                           0xFFFFFFFFUL                      /**< Mask for USB_FIFO1D */
#define _USB_FIFO1D_FIFO1D_SHIFT                   0                                 /**< Shift value for USB_FIFO1D */
#define _USB_FIFO1D_FIFO1D_MASK                    0xFFFFFFFFUL                      /**< Bit mask for USB_FIFO1D */
#define _USB_FIFO1D_FIFO1D_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USB_FIFO1D */
#define USB_FIFO1D_FIFO1D_DEFAULT                  (_USB_FIFO1D_FIFO1D_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_FIFO1D */

/* Bit fields for USB FIFO2D */
#define _USB_FIFO2D_RESETVALUE                     0x00000000UL                      /**< Default value for USB_FIFO2D */
#define _USB_FIFO2D_MASK                           0xFFFFFFFFUL                      /**< Mask for USB_FIFO2D */
#define _USB_FIFO2D_FIFO2D_SHIFT                   0                                 /**< Shift value for USB_FIFO2D */
#define _USB_FIFO2D_FIFO2D_MASK                    0xFFFFFFFFUL                      /**< Bit mask for USB_FIFO2D */
#define _USB_FIFO2D_FIFO2D_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USB_FIFO2D */
#define USB_FIFO2D_FIFO2D_DEFAULT                  (_USB_FIFO2D_FIFO2D_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_FIFO2D */

/* Bit fields for USB FIFO3D */
#define _USB_FIFO3D_RESETVALUE                     0x00000000UL                      /**< Default value for USB_FIFO3D */
#define _USB_FIFO3D_MASK                           0xFFFFFFFFUL                      /**< Mask for USB_FIFO3D */
#define _USB_FIFO3D_FIFO3D_SHIFT                   0                                 /**< Shift value for USB_FIFO3D */
#define _USB_FIFO3D_FIFO3D_MASK                    0xFFFFFFFFUL                      /**< Bit mask for USB_FIFO3D */
#define _USB_FIFO3D_FIFO3D_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USB_FIFO3D */
#define USB_FIFO3D_FIFO3D_DEFAULT                  (_USB_FIFO3D_FIFO3D_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_FIFO3D */

/* Bit fields for USB FIFORAM */
#define _USB_FIFORAM_RESETVALUE                    0x00000000UL                        /**< Default value for USB_FIFORAM */
#define _USB_FIFORAM_MASK                          0xFFFFFFFFUL                        /**< Mask for USB_FIFORAM */
#define _USB_FIFORAM_FIFORAM_SHIFT                 0                                   /**< Shift value for USB_FIFORAM */
#define _USB_FIFORAM_FIFORAM_MASK                  0xFFFFFFFFUL                        /**< Bit mask for USB_FIFORAM */
#define _USB_FIFORAM_FIFORAM_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for USB_FIFORAM */
#define USB_FIFORAM_FIFORAM_DEFAULT                (_USB_FIFORAM_FIFORAM_DEFAULT << 0) /**< Shifted mode DEFAULT for USB_FIFORAM */

/** @} End of group EFM32HG_USB */
/** @} End of group Parts */

