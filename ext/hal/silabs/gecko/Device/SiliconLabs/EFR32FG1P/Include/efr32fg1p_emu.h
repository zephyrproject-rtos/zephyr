/**************************************************************************//**
 * @file efr32fg1p_emu.h
 * @brief EFR32FG1P_EMU register and bit field definitions
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
 * @defgroup EFR32FG1P_EMU
 * @{
 * @brief EFR32FG1P_EMU Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;            /**< Control Register  */
  __IM uint32_t  STATUS;          /**< Status Register  */
  __IOM uint32_t LOCK;            /**< Configuration Lock Register  */
  __IOM uint32_t RAM0CTRL;        /**< Memory Control Register  */
  __IOM uint32_t CMD;             /**< Command Register  */

  uint32_t       RESERVED0[1];    /**< Reserved for future use **/
  __IOM uint32_t EM4CTRL;         /**< EM4 Control Register  */
  __IOM uint32_t TEMPLIMITS;      /**< Temperature limits for interrupt generation  */
  __IM uint32_t  TEMP;            /**< Value of last temperature measurement  */
  __IM uint32_t  IF;              /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;             /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;             /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;             /**< Interrupt Enable Register  */
  __IOM uint32_t PWRLOCK;         /**< Regulator and Supply Lock Register  */
  __IOM uint32_t PWRCFG;          /**< Power Configuration Register  */
  __IOM uint32_t PWRCTRL;         /**< Power Control Register.  */
  __IOM uint32_t DCDCCTRL;        /**< DCDC Control  */

  uint32_t       RESERVED1[2];    /**< Reserved for future use **/
  __IOM uint32_t DCDCMISCCTRL;    /**< DCDC Miscellaneous Control Register  */
  __IOM uint32_t DCDCZDETCTRL;    /**< DCDC Power Train NFET Zero Current Detector Control Register  */
  __IOM uint32_t DCDCCLIMCTRL;    /**< DCDC Power Train PFET Current Limiter Control Register  */
  __IOM uint32_t DCDCLNCOMPCTRL;  /**< DCDC Low Noise Compensator Control Register  */
  __IOM uint32_t DCDCLNVCTRL;     /**< DCDC Low Noise Voltage Register  */
  __IOM uint32_t DCDCTIMING;      /**< DCDC Controller Timing Value Register  */
  __IOM uint32_t DCDCLPVCTRL;     /**< DCDC Low Power Voltage Register  */

  uint32_t       RESERVED2[1];    /**< Reserved for future use **/
  __IOM uint32_t DCDCLPCTRL;      /**< DCDC Low Power Control Register  */
  __IOM uint32_t DCDCLNFREQCTRL;  /**< DCDC Low Noise Controller Frequency Control  */

  uint32_t       RESERVED3[1];    /**< Reserved for future use **/
  __IM uint32_t  DCDCSYNC;        /**< DCDC Read Status Register  */

  uint32_t       RESERVED4[5];    /**< Reserved for future use **/
  __IOM uint32_t VMONAVDDCTRL;    /**< VMON AVDD Channel Control  */
  __IOM uint32_t VMONALTAVDDCTRL; /**< Alternate VMON AVDD Channel Control  */
  __IOM uint32_t VMONDVDDCTRL;    /**< VMON DVDD Channel Control  */
  __IOM uint32_t VMONIO0CTRL;     /**< VMON IOVDD0 Channel Control  */

  uint32_t       RESERVED5[49];   /**< Reserved for future use **/
  __IOM uint32_t BIASCONF;        /**< Configurations Related to the Bias  */

  uint32_t       RESERVED6[10];   /**< Reserved for future use **/
  __IOM uint32_t TESTLOCK;        /**< Test Lock Register  */

  uint32_t       RESERVED7[2];    /**< Reserved for future use **/
  __IOM uint32_t BIASTESTCTRL;    /**< Test Control Register for regulator and BIAS  */
} EMU_TypeDef;                    /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_EMU_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for EMU CTRL */
#define _EMU_CTRL_RESETVALUE                         0x00000000UL                      /**< Default value for EMU_CTRL */
#define _EMU_CTRL_MASK                               0x00000002UL                      /**< Mask for EMU_CTRL */
#define EMU_CTRL_EM2BLOCK                            (0x1UL << 1)                      /**< Energy Mode 2 Block */
#define _EMU_CTRL_EM2BLOCK_SHIFT                     1                                 /**< Shift value for EMU_EM2BLOCK */
#define _EMU_CTRL_EM2BLOCK_MASK                      0x2UL                             /**< Bit mask for EMU_EM2BLOCK */
#define _EMU_CTRL_EM2BLOCK_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for EMU_CTRL */
#define EMU_CTRL_EM2BLOCK_DEFAULT                    (_EMU_CTRL_EM2BLOCK_DEFAULT << 1) /**< Shifted mode DEFAULT for EMU_CTRL */

/* Bit fields for EMU STATUS */
#define _EMU_STATUS_RESETVALUE                       0x00000000UL                           /**< Default value for EMU_STATUS */
#define _EMU_STATUS_MASK                             0x0010011FUL                           /**< Mask for EMU_STATUS */
#define EMU_STATUS_VMONRDY                           (0x1UL << 0)                           /**< VMON ready */
#define _EMU_STATUS_VMONRDY_SHIFT                    0                                      /**< Shift value for EMU_VMONRDY */
#define _EMU_STATUS_VMONRDY_MASK                     0x1UL                                  /**< Bit mask for EMU_VMONRDY */
#define _EMU_STATUS_VMONRDY_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONRDY_DEFAULT                   (_EMU_STATUS_VMONRDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONAVDD                          (0x1UL << 1)                           /**< VMON AVDD Channel. */
#define _EMU_STATUS_VMONAVDD_SHIFT                   1                                      /**< Shift value for EMU_VMONAVDD */
#define _EMU_STATUS_VMONAVDD_MASK                    0x2UL                                  /**< Bit mask for EMU_VMONAVDD */
#define _EMU_STATUS_VMONAVDD_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONAVDD_DEFAULT                  (_EMU_STATUS_VMONAVDD_DEFAULT << 1)    /**< Shifted mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONALTAVDD                       (0x1UL << 2)                           /**< Alternate VMON AVDD Channel. */
#define _EMU_STATUS_VMONALTAVDD_SHIFT                2                                      /**< Shift value for EMU_VMONALTAVDD */
#define _EMU_STATUS_VMONALTAVDD_MASK                 0x4UL                                  /**< Bit mask for EMU_VMONALTAVDD */
#define _EMU_STATUS_VMONALTAVDD_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONALTAVDD_DEFAULT               (_EMU_STATUS_VMONALTAVDD_DEFAULT << 2) /**< Shifted mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONDVDD                          (0x1UL << 3)                           /**< VMON DVDD Channel. */
#define _EMU_STATUS_VMONDVDD_SHIFT                   3                                      /**< Shift value for EMU_VMONDVDD */
#define _EMU_STATUS_VMONDVDD_MASK                    0x8UL                                  /**< Bit mask for EMU_VMONDVDD */
#define _EMU_STATUS_VMONDVDD_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONDVDD_DEFAULT                  (_EMU_STATUS_VMONDVDD_DEFAULT << 3)    /**< Shifted mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONIO0                           (0x1UL << 4)                           /**< VMON IOVDD0 Channel. */
#define _EMU_STATUS_VMONIO0_SHIFT                    4                                      /**< Shift value for EMU_VMONIO0 */
#define _EMU_STATUS_VMONIO0_MASK                     0x10UL                                 /**< Bit mask for EMU_VMONIO0 */
#define _EMU_STATUS_VMONIO0_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONIO0_DEFAULT                   (_EMU_STATUS_VMONIO0_DEFAULT << 4)     /**< Shifted mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONFVDD                          (0x1UL << 8)                           /**< VMON VDDFLASH Channel. */
#define _EMU_STATUS_VMONFVDD_SHIFT                   8                                      /**< Shift value for EMU_VMONFVDD */
#define _EMU_STATUS_VMONFVDD_MASK                    0x100UL                                /**< Bit mask for EMU_VMONFVDD */
#define _EMU_STATUS_VMONFVDD_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_VMONFVDD_DEFAULT                  (_EMU_STATUS_VMONFVDD_DEFAULT << 8)    /**< Shifted mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_EM4IORET                          (0x1UL << 20)                          /**< IO Retention Status */
#define _EMU_STATUS_EM4IORET_SHIFT                   20                                     /**< Shift value for EMU_EM4IORET */
#define _EMU_STATUS_EM4IORET_MASK                    0x100000UL                             /**< Bit mask for EMU_EM4IORET */
#define _EMU_STATUS_EM4IORET_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for EMU_STATUS */
#define _EMU_STATUS_EM4IORET_DISABLED                0x00000000UL                           /**< Mode DISABLED for EMU_STATUS */
#define _EMU_STATUS_EM4IORET_ENABLED                 0x00000001UL                           /**< Mode ENABLED for EMU_STATUS */
#define EMU_STATUS_EM4IORET_DEFAULT                  (_EMU_STATUS_EM4IORET_DEFAULT << 20)   /**< Shifted mode DEFAULT for EMU_STATUS */
#define EMU_STATUS_EM4IORET_DISABLED                 (_EMU_STATUS_EM4IORET_DISABLED << 20)  /**< Shifted mode DISABLED for EMU_STATUS */
#define EMU_STATUS_EM4IORET_ENABLED                  (_EMU_STATUS_EM4IORET_ENABLED << 20)   /**< Shifted mode ENABLED for EMU_STATUS */

/* Bit fields for EMU LOCK */
#define _EMU_LOCK_RESETVALUE                         0x00000000UL                      /**< Default value for EMU_LOCK */
#define _EMU_LOCK_MASK                               0x0000FFFFUL                      /**< Mask for EMU_LOCK */
#define _EMU_LOCK_LOCKKEY_SHIFT                      0                                 /**< Shift value for EMU_LOCKKEY */
#define _EMU_LOCK_LOCKKEY_MASK                       0xFFFFUL                          /**< Bit mask for EMU_LOCKKEY */
#define _EMU_LOCK_LOCKKEY_DEFAULT                    0x00000000UL                      /**< Mode DEFAULT for EMU_LOCK */
#define _EMU_LOCK_LOCKKEY_LOCK                       0x00000000UL                      /**< Mode LOCK for EMU_LOCK */
#define _EMU_LOCK_LOCKKEY_UNLOCKED                   0x00000000UL                      /**< Mode UNLOCKED for EMU_LOCK */
#define _EMU_LOCK_LOCKKEY_LOCKED                     0x00000001UL                      /**< Mode LOCKED for EMU_LOCK */
#define _EMU_LOCK_LOCKKEY_UNLOCK                     0x0000ADE8UL                      /**< Mode UNLOCK for EMU_LOCK */
#define EMU_LOCK_LOCKKEY_DEFAULT                     (_EMU_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for EMU_LOCK */
#define EMU_LOCK_LOCKKEY_LOCK                        (_EMU_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for EMU_LOCK */
#define EMU_LOCK_LOCKKEY_UNLOCKED                    (_EMU_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for EMU_LOCK */
#define EMU_LOCK_LOCKKEY_LOCKED                      (_EMU_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for EMU_LOCK */
#define EMU_LOCK_LOCKKEY_UNLOCK                      (_EMU_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for EMU_LOCK */

/* Bit fields for EMU RAM0CTRL */
#define _EMU_RAM0CTRL_RESETVALUE                     0x00000000UL                              /**< Default value for EMU_RAM0CTRL */
#define _EMU_RAM0CTRL_MASK                           0x0000000FUL                              /**< Mask for EMU_RAM0CTRL */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_SHIFT             0                                         /**< Shift value for EMU_RAMPOWERDOWN */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_MASK              0xFUL                                     /**< Bit mask for EMU_RAMPOWERDOWN */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for EMU_RAM0CTRL */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_NONE              0x00000000UL                              /**< Mode NONE for EMU_RAM0CTRL */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_BLK4              0x00000008UL                              /**< Mode BLK4 for EMU_RAM0CTRL */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_BLK3TO4           0x0000000CUL                              /**< Mode BLK3TO4 for EMU_RAM0CTRL */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_BLK2TO4           0x0000000EUL                              /**< Mode BLK2TO4 for EMU_RAM0CTRL */
#define _EMU_RAM0CTRL_RAMPOWERDOWN_BLK1TO4           0x0000000FUL                              /**< Mode BLK1TO4 for EMU_RAM0CTRL */
#define EMU_RAM0CTRL_RAMPOWERDOWN_DEFAULT            (_EMU_RAM0CTRL_RAMPOWERDOWN_DEFAULT << 0) /**< Shifted mode DEFAULT for EMU_RAM0CTRL */
#define EMU_RAM0CTRL_RAMPOWERDOWN_NONE               (_EMU_RAM0CTRL_RAMPOWERDOWN_NONE << 0)    /**< Shifted mode NONE for EMU_RAM0CTRL */
#define EMU_RAM0CTRL_RAMPOWERDOWN_BLK4               (_EMU_RAM0CTRL_RAMPOWERDOWN_BLK4 << 0)    /**< Shifted mode BLK4 for EMU_RAM0CTRL */
#define EMU_RAM0CTRL_RAMPOWERDOWN_BLK3TO4            (_EMU_RAM0CTRL_RAMPOWERDOWN_BLK3TO4 << 0) /**< Shifted mode BLK3TO4 for EMU_RAM0CTRL */
#define EMU_RAM0CTRL_RAMPOWERDOWN_BLK2TO4            (_EMU_RAM0CTRL_RAMPOWERDOWN_BLK2TO4 << 0) /**< Shifted mode BLK2TO4 for EMU_RAM0CTRL */
#define EMU_RAM0CTRL_RAMPOWERDOWN_BLK1TO4            (_EMU_RAM0CTRL_RAMPOWERDOWN_BLK1TO4 << 0) /**< Shifted mode BLK1TO4 for EMU_RAM0CTRL */

/* Bit fields for EMU CMD */
#define _EMU_CMD_RESETVALUE                          0x00000000UL                       /**< Default value for EMU_CMD */
#define _EMU_CMD_MASK                                0x00000001UL                       /**< Mask for EMU_CMD */
#define EMU_CMD_EM4UNLATCH                           (0x1UL << 0)                       /**< EM4 Unlatch */
#define _EMU_CMD_EM4UNLATCH_SHIFT                    0                                  /**< Shift value for EMU_EM4UNLATCH */
#define _EMU_CMD_EM4UNLATCH_MASK                     0x1UL                              /**< Bit mask for EMU_EM4UNLATCH */
#define _EMU_CMD_EM4UNLATCH_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for EMU_CMD */
#define EMU_CMD_EM4UNLATCH_DEFAULT                   (_EMU_CMD_EM4UNLATCH_DEFAULT << 0) /**< Shifted mode DEFAULT for EMU_CMD */

/* Bit fields for EMU EM4CTRL */
#define _EMU_EM4CTRL_RESETVALUE                      0x00000000UL                               /**< Default value for EMU_EM4CTRL */
#define _EMU_EM4CTRL_MASK                            0x0003003FUL                               /**< Mask for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4STATE                         (0x1UL << 0)                               /**< Energy Mode 4 State */
#define _EMU_EM4CTRL_EM4STATE_SHIFT                  0                                          /**< Shift value for EMU_EM4STATE */
#define _EMU_EM4CTRL_EM4STATE_MASK                   0x1UL                                      /**< Bit mask for EMU_EM4STATE */
#define _EMU_EM4CTRL_EM4STATE_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for EMU_EM4CTRL */
#define _EMU_EM4CTRL_EM4STATE_EM4S                   0x00000000UL                               /**< Mode EM4S for EMU_EM4CTRL */
#define _EMU_EM4CTRL_EM4STATE_EM4H                   0x00000001UL                               /**< Mode EM4H for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4STATE_DEFAULT                 (_EMU_EM4CTRL_EM4STATE_DEFAULT << 0)       /**< Shifted mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4STATE_EM4S                    (_EMU_EM4CTRL_EM4STATE_EM4S << 0)          /**< Shifted mode EM4S for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4STATE_EM4H                    (_EMU_EM4CTRL_EM4STATE_EM4H << 0)          /**< Shifted mode EM4H for EMU_EM4CTRL */
#define EMU_EM4CTRL_RETAINLFRCO                      (0x1UL << 1)                               /**< LFRCO Retain during EM4 */
#define _EMU_EM4CTRL_RETAINLFRCO_SHIFT               1                                          /**< Shift value for EMU_RETAINLFRCO */
#define _EMU_EM4CTRL_RETAINLFRCO_MASK                0x2UL                                      /**< Bit mask for EMU_RETAINLFRCO */
#define _EMU_EM4CTRL_RETAINLFRCO_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_RETAINLFRCO_DEFAULT              (_EMU_EM4CTRL_RETAINLFRCO_DEFAULT << 1)    /**< Shifted mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_RETAINLFXO                       (0x1UL << 2)                               /**< LFXO Retain during EM4 */
#define _EMU_EM4CTRL_RETAINLFXO_SHIFT                2                                          /**< Shift value for EMU_RETAINLFXO */
#define _EMU_EM4CTRL_RETAINLFXO_MASK                 0x4UL                                      /**< Bit mask for EMU_RETAINLFXO */
#define _EMU_EM4CTRL_RETAINLFXO_DEFAULT              0x00000000UL                               /**< Mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_RETAINLFXO_DEFAULT               (_EMU_EM4CTRL_RETAINLFXO_DEFAULT << 2)     /**< Shifted mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_RETAINULFRCO                     (0x1UL << 3)                               /**< ULFRCO Retain during EM4S */
#define _EMU_EM4CTRL_RETAINULFRCO_SHIFT              3                                          /**< Shift value for EMU_RETAINULFRCO */
#define _EMU_EM4CTRL_RETAINULFRCO_MASK               0x8UL                                      /**< Bit mask for EMU_RETAINULFRCO */
#define _EMU_EM4CTRL_RETAINULFRCO_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_RETAINULFRCO_DEFAULT             (_EMU_EM4CTRL_RETAINULFRCO_DEFAULT << 3)   /**< Shifted mode DEFAULT for EMU_EM4CTRL */
#define _EMU_EM4CTRL_EM4IORETMODE_SHIFT              4                                          /**< Shift value for EMU_EM4IORETMODE */
#define _EMU_EM4CTRL_EM4IORETMODE_MASK               0x30UL                                     /**< Bit mask for EMU_EM4IORETMODE */
#define _EMU_EM4CTRL_EM4IORETMODE_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for EMU_EM4CTRL */
#define _EMU_EM4CTRL_EM4IORETMODE_DISABLE            0x00000000UL                               /**< Mode DISABLE for EMU_EM4CTRL */
#define _EMU_EM4CTRL_EM4IORETMODE_EM4EXIT            0x00000001UL                               /**< Mode EM4EXIT for EMU_EM4CTRL */
#define _EMU_EM4CTRL_EM4IORETMODE_SWUNLATCH          0x00000002UL                               /**< Mode SWUNLATCH for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4IORETMODE_DEFAULT             (_EMU_EM4CTRL_EM4IORETMODE_DEFAULT << 4)   /**< Shifted mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4IORETMODE_DISABLE             (_EMU_EM4CTRL_EM4IORETMODE_DISABLE << 4)   /**< Shifted mode DISABLE for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4IORETMODE_EM4EXIT             (_EMU_EM4CTRL_EM4IORETMODE_EM4EXIT << 4)   /**< Shifted mode EM4EXIT for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4IORETMODE_SWUNLATCH           (_EMU_EM4CTRL_EM4IORETMODE_SWUNLATCH << 4) /**< Shifted mode SWUNLATCH for EMU_EM4CTRL */
#define _EMU_EM4CTRL_EM4ENTRY_SHIFT                  16                                         /**< Shift value for EMU_EM4ENTRY */
#define _EMU_EM4CTRL_EM4ENTRY_MASK                   0x30000UL                                  /**< Bit mask for EMU_EM4ENTRY */
#define _EMU_EM4CTRL_EM4ENTRY_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for EMU_EM4CTRL */
#define EMU_EM4CTRL_EM4ENTRY_DEFAULT                 (_EMU_EM4CTRL_EM4ENTRY_DEFAULT << 16)      /**< Shifted mode DEFAULT for EMU_EM4CTRL */

/* Bit fields for EMU TEMPLIMITS */
#define _EMU_TEMPLIMITS_RESETVALUE                   0x0000FF00UL                            /**< Default value for EMU_TEMPLIMITS */
#define _EMU_TEMPLIMITS_MASK                         0x0001FFFFUL                            /**< Mask for EMU_TEMPLIMITS */
#define _EMU_TEMPLIMITS_TEMPLOW_SHIFT                0                                       /**< Shift value for EMU_TEMPLOW */
#define _EMU_TEMPLIMITS_TEMPLOW_MASK                 0xFFUL                                  /**< Bit mask for EMU_TEMPLOW */
#define _EMU_TEMPLIMITS_TEMPLOW_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for EMU_TEMPLIMITS */
#define EMU_TEMPLIMITS_TEMPLOW_DEFAULT               (_EMU_TEMPLIMITS_TEMPLOW_DEFAULT << 0)  /**< Shifted mode DEFAULT for EMU_TEMPLIMITS */
#define _EMU_TEMPLIMITS_TEMPHIGH_SHIFT               8                                       /**< Shift value for EMU_TEMPHIGH */
#define _EMU_TEMPLIMITS_TEMPHIGH_MASK                0xFF00UL                                /**< Bit mask for EMU_TEMPHIGH */
#define _EMU_TEMPLIMITS_TEMPHIGH_DEFAULT             0x000000FFUL                            /**< Mode DEFAULT for EMU_TEMPLIMITS */
#define EMU_TEMPLIMITS_TEMPHIGH_DEFAULT              (_EMU_TEMPLIMITS_TEMPHIGH_DEFAULT << 8) /**< Shifted mode DEFAULT for EMU_TEMPLIMITS */
#define EMU_TEMPLIMITS_EM4WUEN                       (0x1UL << 16)                           /**< Enable EM4 Wakeup due to low/high temperature */
#define _EMU_TEMPLIMITS_EM4WUEN_SHIFT                16                                      /**< Shift value for EMU_EM4WUEN */
#define _EMU_TEMPLIMITS_EM4WUEN_MASK                 0x10000UL                               /**< Bit mask for EMU_EM4WUEN */
#define _EMU_TEMPLIMITS_EM4WUEN_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for EMU_TEMPLIMITS */
#define EMU_TEMPLIMITS_EM4WUEN_DEFAULT               (_EMU_TEMPLIMITS_EM4WUEN_DEFAULT << 16) /**< Shifted mode DEFAULT for EMU_TEMPLIMITS */

/* Bit fields for EMU TEMP */
#define _EMU_TEMP_RESETVALUE                         0x00000000UL                  /**< Default value for EMU_TEMP */
#define _EMU_TEMP_MASK                               0x000000FFUL                  /**< Mask for EMU_TEMP */
#define _EMU_TEMP_TEMP_SHIFT                         0                             /**< Shift value for EMU_TEMP */
#define _EMU_TEMP_TEMP_MASK                          0xFFUL                        /**< Bit mask for EMU_TEMP */
#define _EMU_TEMP_TEMP_DEFAULT                       0x00000000UL                  /**< Mode DEFAULT for EMU_TEMP */
#define EMU_TEMP_TEMP_DEFAULT                        (_EMU_TEMP_TEMP_DEFAULT << 0) /**< Shifted mode DEFAULT for EMU_TEMP */

/* Bit fields for EMU IF */
#define _EMU_IF_RESETVALUE                           0x00000000UL                                 /**< Default value for EMU_IF */
#define _EMU_IF_MASK                                 0xE11FC0FFUL                                 /**< Mask for EMU_IF */
#define EMU_IF_VMONAVDDFALL                          (0x1UL << 0)                                 /**< VMON AVDD Channel Fall */
#define _EMU_IF_VMONAVDDFALL_SHIFT                   0                                            /**< Shift value for EMU_VMONAVDDFALL */
#define _EMU_IF_VMONAVDDFALL_MASK                    0x1UL                                        /**< Bit mask for EMU_VMONAVDDFALL */
#define _EMU_IF_VMONAVDDFALL_DEFAULT                 0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONAVDDFALL_DEFAULT                  (_EMU_IF_VMONAVDDFALL_DEFAULT << 0)          /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONAVDDRISE                          (0x1UL << 1)                                 /**< VMON AVDD Channel Rise */
#define _EMU_IF_VMONAVDDRISE_SHIFT                   1                                            /**< Shift value for EMU_VMONAVDDRISE */
#define _EMU_IF_VMONAVDDRISE_MASK                    0x2UL                                        /**< Bit mask for EMU_VMONAVDDRISE */
#define _EMU_IF_VMONAVDDRISE_DEFAULT                 0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONAVDDRISE_DEFAULT                  (_EMU_IF_VMONAVDDRISE_DEFAULT << 1)          /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONALTAVDDFALL                       (0x1UL << 2)                                 /**< Alternate VMON AVDD Channel Fall */
#define _EMU_IF_VMONALTAVDDFALL_SHIFT                2                                            /**< Shift value for EMU_VMONALTAVDDFALL */
#define _EMU_IF_VMONALTAVDDFALL_MASK                 0x4UL                                        /**< Bit mask for EMU_VMONALTAVDDFALL */
#define _EMU_IF_VMONALTAVDDFALL_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONALTAVDDFALL_DEFAULT               (_EMU_IF_VMONALTAVDDFALL_DEFAULT << 2)       /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONALTAVDDRISE                       (0x1UL << 3)                                 /**< Alternate VMON AVDD Channel Rise */
#define _EMU_IF_VMONALTAVDDRISE_SHIFT                3                                            /**< Shift value for EMU_VMONALTAVDDRISE */
#define _EMU_IF_VMONALTAVDDRISE_MASK                 0x8UL                                        /**< Bit mask for EMU_VMONALTAVDDRISE */
#define _EMU_IF_VMONALTAVDDRISE_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONALTAVDDRISE_DEFAULT               (_EMU_IF_VMONALTAVDDRISE_DEFAULT << 3)       /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONDVDDFALL                          (0x1UL << 4)                                 /**< VMON DVDD Channel Fall */
#define _EMU_IF_VMONDVDDFALL_SHIFT                   4                                            /**< Shift value for EMU_VMONDVDDFALL */
#define _EMU_IF_VMONDVDDFALL_MASK                    0x10UL                                       /**< Bit mask for EMU_VMONDVDDFALL */
#define _EMU_IF_VMONDVDDFALL_DEFAULT                 0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONDVDDFALL_DEFAULT                  (_EMU_IF_VMONDVDDFALL_DEFAULT << 4)          /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONDVDDRISE                          (0x1UL << 5)                                 /**< VMON DVDD Channel Rise */
#define _EMU_IF_VMONDVDDRISE_SHIFT                   5                                            /**< Shift value for EMU_VMONDVDDRISE */
#define _EMU_IF_VMONDVDDRISE_MASK                    0x20UL                                       /**< Bit mask for EMU_VMONDVDDRISE */
#define _EMU_IF_VMONDVDDRISE_DEFAULT                 0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONDVDDRISE_DEFAULT                  (_EMU_IF_VMONDVDDRISE_DEFAULT << 5)          /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONIO0FALL                           (0x1UL << 6)                                 /**< VMON IOVDD0 Channel Fall */
#define _EMU_IF_VMONIO0FALL_SHIFT                    6                                            /**< Shift value for EMU_VMONIO0FALL */
#define _EMU_IF_VMONIO0FALL_MASK                     0x40UL                                       /**< Bit mask for EMU_VMONIO0FALL */
#define _EMU_IF_VMONIO0FALL_DEFAULT                  0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONIO0FALL_DEFAULT                   (_EMU_IF_VMONIO0FALL_DEFAULT << 6)           /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONIO0RISE                           (0x1UL << 7)                                 /**< VMON IOVDD0 Channel Rise */
#define _EMU_IF_VMONIO0RISE_SHIFT                    7                                            /**< Shift value for EMU_VMONIO0RISE */
#define _EMU_IF_VMONIO0RISE_MASK                     0x80UL                                       /**< Bit mask for EMU_VMONIO0RISE */
#define _EMU_IF_VMONIO0RISE_DEFAULT                  0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONIO0RISE_DEFAULT                   (_EMU_IF_VMONIO0RISE_DEFAULT << 7)           /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONFVDDFALL                          (0x1UL << 14)                                /**< VMON VDDFLASH Channel Fall */
#define _EMU_IF_VMONFVDDFALL_SHIFT                   14                                           /**< Shift value for EMU_VMONFVDDFALL */
#define _EMU_IF_VMONFVDDFALL_MASK                    0x4000UL                                     /**< Bit mask for EMU_VMONFVDDFALL */
#define _EMU_IF_VMONFVDDFALL_DEFAULT                 0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONFVDDFALL_DEFAULT                  (_EMU_IF_VMONFVDDFALL_DEFAULT << 14)         /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_VMONFVDDRISE                          (0x1UL << 15)                                /**< VMON VDDFLASH Channel Rise */
#define _EMU_IF_VMONFVDDRISE_SHIFT                   15                                           /**< Shift value for EMU_VMONFVDDRISE */
#define _EMU_IF_VMONFVDDRISE_MASK                    0x8000UL                                     /**< Bit mask for EMU_VMONFVDDRISE */
#define _EMU_IF_VMONFVDDRISE_DEFAULT                 0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_VMONFVDDRISE_DEFAULT                  (_EMU_IF_VMONFVDDRISE_DEFAULT << 15)         /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_PFETOVERCURRENTLIMIT                  (0x1UL << 16)                                /**< PFET current limit hit */
#define _EMU_IF_PFETOVERCURRENTLIMIT_SHIFT           16                                           /**< Shift value for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IF_PFETOVERCURRENTLIMIT_MASK            0x10000UL                                    /**< Bit mask for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IF_PFETOVERCURRENTLIMIT_DEFAULT         0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_PFETOVERCURRENTLIMIT_DEFAULT          (_EMU_IF_PFETOVERCURRENTLIMIT_DEFAULT << 16) /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_NFETOVERCURRENTLIMIT                  (0x1UL << 17)                                /**< NFET current limit hit */
#define _EMU_IF_NFETOVERCURRENTLIMIT_SHIFT           17                                           /**< Shift value for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IF_NFETOVERCURRENTLIMIT_MASK            0x20000UL                                    /**< Bit mask for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IF_NFETOVERCURRENTLIMIT_DEFAULT         0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_NFETOVERCURRENTLIMIT_DEFAULT          (_EMU_IF_NFETOVERCURRENTLIMIT_DEFAULT << 17) /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_DCDCLPRUNNING                         (0x1UL << 18)                                /**< LP mode is running */
#define _EMU_IF_DCDCLPRUNNING_SHIFT                  18                                           /**< Shift value for EMU_DCDCLPRUNNING */
#define _EMU_IF_DCDCLPRUNNING_MASK                   0x40000UL                                    /**< Bit mask for EMU_DCDCLPRUNNING */
#define _EMU_IF_DCDCLPRUNNING_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_DCDCLPRUNNING_DEFAULT                 (_EMU_IF_DCDCLPRUNNING_DEFAULT << 18)        /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_DCDCLNRUNNING                         (0x1UL << 19)                                /**< LN mode is running */
#define _EMU_IF_DCDCLNRUNNING_SHIFT                  19                                           /**< Shift value for EMU_DCDCLNRUNNING */
#define _EMU_IF_DCDCLNRUNNING_MASK                   0x80000UL                                    /**< Bit mask for EMU_DCDCLNRUNNING */
#define _EMU_IF_DCDCLNRUNNING_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_DCDCLNRUNNING_DEFAULT                 (_EMU_IF_DCDCLNRUNNING_DEFAULT << 19)        /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_DCDCINBYPASS                          (0x1UL << 20)                                /**< DCDC is in bypass */
#define _EMU_IF_DCDCINBYPASS_SHIFT                   20                                           /**< Shift value for EMU_DCDCINBYPASS */
#define _EMU_IF_DCDCINBYPASS_MASK                    0x100000UL                                   /**< Bit mask for EMU_DCDCINBYPASS */
#define _EMU_IF_DCDCINBYPASS_DEFAULT                 0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_DCDCINBYPASS_DEFAULT                  (_EMU_IF_DCDCINBYPASS_DEFAULT << 20)         /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_EM23WAKEUP                            (0x1UL << 24)                                /**< Wakeup IRQ from EM2 and EM3 */
#define _EMU_IF_EM23WAKEUP_SHIFT                     24                                           /**< Shift value for EMU_EM23WAKEUP */
#define _EMU_IF_EM23WAKEUP_MASK                      0x1000000UL                                  /**< Bit mask for EMU_EM23WAKEUP */
#define _EMU_IF_EM23WAKEUP_DEFAULT                   0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_EM23WAKEUP_DEFAULT                    (_EMU_IF_EM23WAKEUP_DEFAULT << 24)           /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_TEMP                                  (0x1UL << 29)                                /**< New Temperature Measurement Valid */
#define _EMU_IF_TEMP_SHIFT                           29                                           /**< Shift value for EMU_TEMP */
#define _EMU_IF_TEMP_MASK                            0x20000000UL                                 /**< Bit mask for EMU_TEMP */
#define _EMU_IF_TEMP_DEFAULT                         0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_TEMP_DEFAULT                          (_EMU_IF_TEMP_DEFAULT << 29)                 /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_TEMPLOW                               (0x1UL << 30)                                /**< Temperature Low Limit Reached */
#define _EMU_IF_TEMPLOW_SHIFT                        30                                           /**< Shift value for EMU_TEMPLOW */
#define _EMU_IF_TEMPLOW_MASK                         0x40000000UL                                 /**< Bit mask for EMU_TEMPLOW */
#define _EMU_IF_TEMPLOW_DEFAULT                      0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_TEMPLOW_DEFAULT                       (_EMU_IF_TEMPLOW_DEFAULT << 30)              /**< Shifted mode DEFAULT for EMU_IF */
#define EMU_IF_TEMPHIGH                              (0x1UL << 31)                                /**< Temperature High Limit Reached */
#define _EMU_IF_TEMPHIGH_SHIFT                       31                                           /**< Shift value for EMU_TEMPHIGH */
#define _EMU_IF_TEMPHIGH_MASK                        0x80000000UL                                 /**< Bit mask for EMU_TEMPHIGH */
#define _EMU_IF_TEMPHIGH_DEFAULT                     0x00000000UL                                 /**< Mode DEFAULT for EMU_IF */
#define EMU_IF_TEMPHIGH_DEFAULT                      (_EMU_IF_TEMPHIGH_DEFAULT << 31)             /**< Shifted mode DEFAULT for EMU_IF */

/* Bit fields for EMU IFS */
#define _EMU_IFS_RESETVALUE                          0x00000000UL                                  /**< Default value for EMU_IFS */
#define _EMU_IFS_MASK                                0xE11FC0FFUL                                  /**< Mask for EMU_IFS */
#define EMU_IFS_VMONAVDDFALL                         (0x1UL << 0)                                  /**< Set VMONAVDDFALL Interrupt Flag */
#define _EMU_IFS_VMONAVDDFALL_SHIFT                  0                                             /**< Shift value for EMU_VMONAVDDFALL */
#define _EMU_IFS_VMONAVDDFALL_MASK                   0x1UL                                         /**< Bit mask for EMU_VMONAVDDFALL */
#define _EMU_IFS_VMONAVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONAVDDFALL_DEFAULT                 (_EMU_IFS_VMONAVDDFALL_DEFAULT << 0)          /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONAVDDRISE                         (0x1UL << 1)                                  /**< Set VMONAVDDRISE Interrupt Flag */
#define _EMU_IFS_VMONAVDDRISE_SHIFT                  1                                             /**< Shift value for EMU_VMONAVDDRISE */
#define _EMU_IFS_VMONAVDDRISE_MASK                   0x2UL                                         /**< Bit mask for EMU_VMONAVDDRISE */
#define _EMU_IFS_VMONAVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONAVDDRISE_DEFAULT                 (_EMU_IFS_VMONAVDDRISE_DEFAULT << 1)          /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONALTAVDDFALL                      (0x1UL << 2)                                  /**< Set VMONALTAVDDFALL Interrupt Flag */
#define _EMU_IFS_VMONALTAVDDFALL_SHIFT               2                                             /**< Shift value for EMU_VMONALTAVDDFALL */
#define _EMU_IFS_VMONALTAVDDFALL_MASK                0x4UL                                         /**< Bit mask for EMU_VMONALTAVDDFALL */
#define _EMU_IFS_VMONALTAVDDFALL_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONALTAVDDFALL_DEFAULT              (_EMU_IFS_VMONALTAVDDFALL_DEFAULT << 2)       /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONALTAVDDRISE                      (0x1UL << 3)                                  /**< Set VMONALTAVDDRISE Interrupt Flag */
#define _EMU_IFS_VMONALTAVDDRISE_SHIFT               3                                             /**< Shift value for EMU_VMONALTAVDDRISE */
#define _EMU_IFS_VMONALTAVDDRISE_MASK                0x8UL                                         /**< Bit mask for EMU_VMONALTAVDDRISE */
#define _EMU_IFS_VMONALTAVDDRISE_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONALTAVDDRISE_DEFAULT              (_EMU_IFS_VMONALTAVDDRISE_DEFAULT << 3)       /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONDVDDFALL                         (0x1UL << 4)                                  /**< Set VMONDVDDFALL Interrupt Flag */
#define _EMU_IFS_VMONDVDDFALL_SHIFT                  4                                             /**< Shift value for EMU_VMONDVDDFALL */
#define _EMU_IFS_VMONDVDDFALL_MASK                   0x10UL                                        /**< Bit mask for EMU_VMONDVDDFALL */
#define _EMU_IFS_VMONDVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONDVDDFALL_DEFAULT                 (_EMU_IFS_VMONDVDDFALL_DEFAULT << 4)          /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONDVDDRISE                         (0x1UL << 5)                                  /**< Set VMONDVDDRISE Interrupt Flag */
#define _EMU_IFS_VMONDVDDRISE_SHIFT                  5                                             /**< Shift value for EMU_VMONDVDDRISE */
#define _EMU_IFS_VMONDVDDRISE_MASK                   0x20UL                                        /**< Bit mask for EMU_VMONDVDDRISE */
#define _EMU_IFS_VMONDVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONDVDDRISE_DEFAULT                 (_EMU_IFS_VMONDVDDRISE_DEFAULT << 5)          /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONIO0FALL                          (0x1UL << 6)                                  /**< Set VMONIO0FALL Interrupt Flag */
#define _EMU_IFS_VMONIO0FALL_SHIFT                   6                                             /**< Shift value for EMU_VMONIO0FALL */
#define _EMU_IFS_VMONIO0FALL_MASK                    0x40UL                                        /**< Bit mask for EMU_VMONIO0FALL */
#define _EMU_IFS_VMONIO0FALL_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONIO0FALL_DEFAULT                  (_EMU_IFS_VMONIO0FALL_DEFAULT << 6)           /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONIO0RISE                          (0x1UL << 7)                                  /**< Set VMONIO0RISE Interrupt Flag */
#define _EMU_IFS_VMONIO0RISE_SHIFT                   7                                             /**< Shift value for EMU_VMONIO0RISE */
#define _EMU_IFS_VMONIO0RISE_MASK                    0x80UL                                        /**< Bit mask for EMU_VMONIO0RISE */
#define _EMU_IFS_VMONIO0RISE_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONIO0RISE_DEFAULT                  (_EMU_IFS_VMONIO0RISE_DEFAULT << 7)           /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONFVDDFALL                         (0x1UL << 14)                                 /**< Set VMONFVDDFALL Interrupt Flag */
#define _EMU_IFS_VMONFVDDFALL_SHIFT                  14                                            /**< Shift value for EMU_VMONFVDDFALL */
#define _EMU_IFS_VMONFVDDFALL_MASK                   0x4000UL                                      /**< Bit mask for EMU_VMONFVDDFALL */
#define _EMU_IFS_VMONFVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONFVDDFALL_DEFAULT                 (_EMU_IFS_VMONFVDDFALL_DEFAULT << 14)         /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONFVDDRISE                         (0x1UL << 15)                                 /**< Set VMONFVDDRISE Interrupt Flag */
#define _EMU_IFS_VMONFVDDRISE_SHIFT                  15                                            /**< Shift value for EMU_VMONFVDDRISE */
#define _EMU_IFS_VMONFVDDRISE_MASK                   0x8000UL                                      /**< Bit mask for EMU_VMONFVDDRISE */
#define _EMU_IFS_VMONFVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_VMONFVDDRISE_DEFAULT                 (_EMU_IFS_VMONFVDDRISE_DEFAULT << 15)         /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_PFETOVERCURRENTLIMIT                 (0x1UL << 16)                                 /**< Set PFETOVERCURRENTLIMIT Interrupt Flag */
#define _EMU_IFS_PFETOVERCURRENTLIMIT_SHIFT          16                                            /**< Shift value for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IFS_PFETOVERCURRENTLIMIT_MASK           0x10000UL                                     /**< Bit mask for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IFS_PFETOVERCURRENTLIMIT_DEFAULT        0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_PFETOVERCURRENTLIMIT_DEFAULT         (_EMU_IFS_PFETOVERCURRENTLIMIT_DEFAULT << 16) /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_NFETOVERCURRENTLIMIT                 (0x1UL << 17)                                 /**< Set NFETOVERCURRENTLIMIT Interrupt Flag */
#define _EMU_IFS_NFETOVERCURRENTLIMIT_SHIFT          17                                            /**< Shift value for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IFS_NFETOVERCURRENTLIMIT_MASK           0x20000UL                                     /**< Bit mask for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IFS_NFETOVERCURRENTLIMIT_DEFAULT        0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_NFETOVERCURRENTLIMIT_DEFAULT         (_EMU_IFS_NFETOVERCURRENTLIMIT_DEFAULT << 17) /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_DCDCLPRUNNING                        (0x1UL << 18)                                 /**< Set DCDCLPRUNNING Interrupt Flag */
#define _EMU_IFS_DCDCLPRUNNING_SHIFT                 18                                            /**< Shift value for EMU_DCDCLPRUNNING */
#define _EMU_IFS_DCDCLPRUNNING_MASK                  0x40000UL                                     /**< Bit mask for EMU_DCDCLPRUNNING */
#define _EMU_IFS_DCDCLPRUNNING_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_DCDCLPRUNNING_DEFAULT                (_EMU_IFS_DCDCLPRUNNING_DEFAULT << 18)        /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_DCDCLNRUNNING                        (0x1UL << 19)                                 /**< Set DCDCLNRUNNING Interrupt Flag */
#define _EMU_IFS_DCDCLNRUNNING_SHIFT                 19                                            /**< Shift value for EMU_DCDCLNRUNNING */
#define _EMU_IFS_DCDCLNRUNNING_MASK                  0x80000UL                                     /**< Bit mask for EMU_DCDCLNRUNNING */
#define _EMU_IFS_DCDCLNRUNNING_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_DCDCLNRUNNING_DEFAULT                (_EMU_IFS_DCDCLNRUNNING_DEFAULT << 19)        /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_DCDCINBYPASS                         (0x1UL << 20)                                 /**< Set DCDCINBYPASS Interrupt Flag */
#define _EMU_IFS_DCDCINBYPASS_SHIFT                  20                                            /**< Shift value for EMU_DCDCINBYPASS */
#define _EMU_IFS_DCDCINBYPASS_MASK                   0x100000UL                                    /**< Bit mask for EMU_DCDCINBYPASS */
#define _EMU_IFS_DCDCINBYPASS_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_DCDCINBYPASS_DEFAULT                 (_EMU_IFS_DCDCINBYPASS_DEFAULT << 20)         /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_EM23WAKEUP                           (0x1UL << 24)                                 /**< Set EM23WAKEUP Interrupt Flag */
#define _EMU_IFS_EM23WAKEUP_SHIFT                    24                                            /**< Shift value for EMU_EM23WAKEUP */
#define _EMU_IFS_EM23WAKEUP_MASK                     0x1000000UL                                   /**< Bit mask for EMU_EM23WAKEUP */
#define _EMU_IFS_EM23WAKEUP_DEFAULT                  0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_EM23WAKEUP_DEFAULT                   (_EMU_IFS_EM23WAKEUP_DEFAULT << 24)           /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_TEMP                                 (0x1UL << 29)                                 /**< Set TEMP Interrupt Flag */
#define _EMU_IFS_TEMP_SHIFT                          29                                            /**< Shift value for EMU_TEMP */
#define _EMU_IFS_TEMP_MASK                           0x20000000UL                                  /**< Bit mask for EMU_TEMP */
#define _EMU_IFS_TEMP_DEFAULT                        0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_TEMP_DEFAULT                         (_EMU_IFS_TEMP_DEFAULT << 29)                 /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_TEMPLOW                              (0x1UL << 30)                                 /**< Set TEMPLOW Interrupt Flag */
#define _EMU_IFS_TEMPLOW_SHIFT                       30                                            /**< Shift value for EMU_TEMPLOW */
#define _EMU_IFS_TEMPLOW_MASK                        0x40000000UL                                  /**< Bit mask for EMU_TEMPLOW */
#define _EMU_IFS_TEMPLOW_DEFAULT                     0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_TEMPLOW_DEFAULT                      (_EMU_IFS_TEMPLOW_DEFAULT << 30)              /**< Shifted mode DEFAULT for EMU_IFS */
#define EMU_IFS_TEMPHIGH                             (0x1UL << 31)                                 /**< Set TEMPHIGH Interrupt Flag */
#define _EMU_IFS_TEMPHIGH_SHIFT                      31                                            /**< Shift value for EMU_TEMPHIGH */
#define _EMU_IFS_TEMPHIGH_MASK                       0x80000000UL                                  /**< Bit mask for EMU_TEMPHIGH */
#define _EMU_IFS_TEMPHIGH_DEFAULT                    0x00000000UL                                  /**< Mode DEFAULT for EMU_IFS */
#define EMU_IFS_TEMPHIGH_DEFAULT                     (_EMU_IFS_TEMPHIGH_DEFAULT << 31)             /**< Shifted mode DEFAULT for EMU_IFS */

/* Bit fields for EMU IFC */
#define _EMU_IFC_RESETVALUE                          0x00000000UL                                  /**< Default value for EMU_IFC */
#define _EMU_IFC_MASK                                0xE11FC0FFUL                                  /**< Mask for EMU_IFC */
#define EMU_IFC_VMONAVDDFALL                         (0x1UL << 0)                                  /**< Clear VMONAVDDFALL Interrupt Flag */
#define _EMU_IFC_VMONAVDDFALL_SHIFT                  0                                             /**< Shift value for EMU_VMONAVDDFALL */
#define _EMU_IFC_VMONAVDDFALL_MASK                   0x1UL                                         /**< Bit mask for EMU_VMONAVDDFALL */
#define _EMU_IFC_VMONAVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONAVDDFALL_DEFAULT                 (_EMU_IFC_VMONAVDDFALL_DEFAULT << 0)          /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONAVDDRISE                         (0x1UL << 1)                                  /**< Clear VMONAVDDRISE Interrupt Flag */
#define _EMU_IFC_VMONAVDDRISE_SHIFT                  1                                             /**< Shift value for EMU_VMONAVDDRISE */
#define _EMU_IFC_VMONAVDDRISE_MASK                   0x2UL                                         /**< Bit mask for EMU_VMONAVDDRISE */
#define _EMU_IFC_VMONAVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONAVDDRISE_DEFAULT                 (_EMU_IFC_VMONAVDDRISE_DEFAULT << 1)          /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONALTAVDDFALL                      (0x1UL << 2)                                  /**< Clear VMONALTAVDDFALL Interrupt Flag */
#define _EMU_IFC_VMONALTAVDDFALL_SHIFT               2                                             /**< Shift value for EMU_VMONALTAVDDFALL */
#define _EMU_IFC_VMONALTAVDDFALL_MASK                0x4UL                                         /**< Bit mask for EMU_VMONALTAVDDFALL */
#define _EMU_IFC_VMONALTAVDDFALL_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONALTAVDDFALL_DEFAULT              (_EMU_IFC_VMONALTAVDDFALL_DEFAULT << 2)       /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONALTAVDDRISE                      (0x1UL << 3)                                  /**< Clear VMONALTAVDDRISE Interrupt Flag */
#define _EMU_IFC_VMONALTAVDDRISE_SHIFT               3                                             /**< Shift value for EMU_VMONALTAVDDRISE */
#define _EMU_IFC_VMONALTAVDDRISE_MASK                0x8UL                                         /**< Bit mask for EMU_VMONALTAVDDRISE */
#define _EMU_IFC_VMONALTAVDDRISE_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONALTAVDDRISE_DEFAULT              (_EMU_IFC_VMONALTAVDDRISE_DEFAULT << 3)       /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONDVDDFALL                         (0x1UL << 4)                                  /**< Clear VMONDVDDFALL Interrupt Flag */
#define _EMU_IFC_VMONDVDDFALL_SHIFT                  4                                             /**< Shift value for EMU_VMONDVDDFALL */
#define _EMU_IFC_VMONDVDDFALL_MASK                   0x10UL                                        /**< Bit mask for EMU_VMONDVDDFALL */
#define _EMU_IFC_VMONDVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONDVDDFALL_DEFAULT                 (_EMU_IFC_VMONDVDDFALL_DEFAULT << 4)          /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONDVDDRISE                         (0x1UL << 5)                                  /**< Clear VMONDVDDRISE Interrupt Flag */
#define _EMU_IFC_VMONDVDDRISE_SHIFT                  5                                             /**< Shift value for EMU_VMONDVDDRISE */
#define _EMU_IFC_VMONDVDDRISE_MASK                   0x20UL                                        /**< Bit mask for EMU_VMONDVDDRISE */
#define _EMU_IFC_VMONDVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONDVDDRISE_DEFAULT                 (_EMU_IFC_VMONDVDDRISE_DEFAULT << 5)          /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONIO0FALL                          (0x1UL << 6)                                  /**< Clear VMONIO0FALL Interrupt Flag */
#define _EMU_IFC_VMONIO0FALL_SHIFT                   6                                             /**< Shift value for EMU_VMONIO0FALL */
#define _EMU_IFC_VMONIO0FALL_MASK                    0x40UL                                        /**< Bit mask for EMU_VMONIO0FALL */
#define _EMU_IFC_VMONIO0FALL_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONIO0FALL_DEFAULT                  (_EMU_IFC_VMONIO0FALL_DEFAULT << 6)           /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONIO0RISE                          (0x1UL << 7)                                  /**< Clear VMONIO0RISE Interrupt Flag */
#define _EMU_IFC_VMONIO0RISE_SHIFT                   7                                             /**< Shift value for EMU_VMONIO0RISE */
#define _EMU_IFC_VMONIO0RISE_MASK                    0x80UL                                        /**< Bit mask for EMU_VMONIO0RISE */
#define _EMU_IFC_VMONIO0RISE_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONIO0RISE_DEFAULT                  (_EMU_IFC_VMONIO0RISE_DEFAULT << 7)           /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONFVDDFALL                         (0x1UL << 14)                                 /**< Clear VMONFVDDFALL Interrupt Flag */
#define _EMU_IFC_VMONFVDDFALL_SHIFT                  14                                            /**< Shift value for EMU_VMONFVDDFALL */
#define _EMU_IFC_VMONFVDDFALL_MASK                   0x4000UL                                      /**< Bit mask for EMU_VMONFVDDFALL */
#define _EMU_IFC_VMONFVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONFVDDFALL_DEFAULT                 (_EMU_IFC_VMONFVDDFALL_DEFAULT << 14)         /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONFVDDRISE                         (0x1UL << 15)                                 /**< Clear VMONFVDDRISE Interrupt Flag */
#define _EMU_IFC_VMONFVDDRISE_SHIFT                  15                                            /**< Shift value for EMU_VMONFVDDRISE */
#define _EMU_IFC_VMONFVDDRISE_MASK                   0x8000UL                                      /**< Bit mask for EMU_VMONFVDDRISE */
#define _EMU_IFC_VMONFVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_VMONFVDDRISE_DEFAULT                 (_EMU_IFC_VMONFVDDRISE_DEFAULT << 15)         /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_PFETOVERCURRENTLIMIT                 (0x1UL << 16)                                 /**< Clear PFETOVERCURRENTLIMIT Interrupt Flag */
#define _EMU_IFC_PFETOVERCURRENTLIMIT_SHIFT          16                                            /**< Shift value for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IFC_PFETOVERCURRENTLIMIT_MASK           0x10000UL                                     /**< Bit mask for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IFC_PFETOVERCURRENTLIMIT_DEFAULT        0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_PFETOVERCURRENTLIMIT_DEFAULT         (_EMU_IFC_PFETOVERCURRENTLIMIT_DEFAULT << 16) /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_NFETOVERCURRENTLIMIT                 (0x1UL << 17)                                 /**< Clear NFETOVERCURRENTLIMIT Interrupt Flag */
#define _EMU_IFC_NFETOVERCURRENTLIMIT_SHIFT          17                                            /**< Shift value for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IFC_NFETOVERCURRENTLIMIT_MASK           0x20000UL                                     /**< Bit mask for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IFC_NFETOVERCURRENTLIMIT_DEFAULT        0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_NFETOVERCURRENTLIMIT_DEFAULT         (_EMU_IFC_NFETOVERCURRENTLIMIT_DEFAULT << 17) /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_DCDCLPRUNNING                        (0x1UL << 18)                                 /**< Clear DCDCLPRUNNING Interrupt Flag */
#define _EMU_IFC_DCDCLPRUNNING_SHIFT                 18                                            /**< Shift value for EMU_DCDCLPRUNNING */
#define _EMU_IFC_DCDCLPRUNNING_MASK                  0x40000UL                                     /**< Bit mask for EMU_DCDCLPRUNNING */
#define _EMU_IFC_DCDCLPRUNNING_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_DCDCLPRUNNING_DEFAULT                (_EMU_IFC_DCDCLPRUNNING_DEFAULT << 18)        /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_DCDCLNRUNNING                        (0x1UL << 19)                                 /**< Clear DCDCLNRUNNING Interrupt Flag */
#define _EMU_IFC_DCDCLNRUNNING_SHIFT                 19                                            /**< Shift value for EMU_DCDCLNRUNNING */
#define _EMU_IFC_DCDCLNRUNNING_MASK                  0x80000UL                                     /**< Bit mask for EMU_DCDCLNRUNNING */
#define _EMU_IFC_DCDCLNRUNNING_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_DCDCLNRUNNING_DEFAULT                (_EMU_IFC_DCDCLNRUNNING_DEFAULT << 19)        /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_DCDCINBYPASS                         (0x1UL << 20)                                 /**< Clear DCDCINBYPASS Interrupt Flag */
#define _EMU_IFC_DCDCINBYPASS_SHIFT                  20                                            /**< Shift value for EMU_DCDCINBYPASS */
#define _EMU_IFC_DCDCINBYPASS_MASK                   0x100000UL                                    /**< Bit mask for EMU_DCDCINBYPASS */
#define _EMU_IFC_DCDCINBYPASS_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_DCDCINBYPASS_DEFAULT                 (_EMU_IFC_DCDCINBYPASS_DEFAULT << 20)         /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_EM23WAKEUP                           (0x1UL << 24)                                 /**< Clear EM23WAKEUP Interrupt Flag */
#define _EMU_IFC_EM23WAKEUP_SHIFT                    24                                            /**< Shift value for EMU_EM23WAKEUP */
#define _EMU_IFC_EM23WAKEUP_MASK                     0x1000000UL                                   /**< Bit mask for EMU_EM23WAKEUP */
#define _EMU_IFC_EM23WAKEUP_DEFAULT                  0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_EM23WAKEUP_DEFAULT                   (_EMU_IFC_EM23WAKEUP_DEFAULT << 24)           /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_TEMP                                 (0x1UL << 29)                                 /**< Clear TEMP Interrupt Flag */
#define _EMU_IFC_TEMP_SHIFT                          29                                            /**< Shift value for EMU_TEMP */
#define _EMU_IFC_TEMP_MASK                           0x20000000UL                                  /**< Bit mask for EMU_TEMP */
#define _EMU_IFC_TEMP_DEFAULT                        0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_TEMP_DEFAULT                         (_EMU_IFC_TEMP_DEFAULT << 29)                 /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_TEMPLOW                              (0x1UL << 30)                                 /**< Clear TEMPLOW Interrupt Flag */
#define _EMU_IFC_TEMPLOW_SHIFT                       30                                            /**< Shift value for EMU_TEMPLOW */
#define _EMU_IFC_TEMPLOW_MASK                        0x40000000UL                                  /**< Bit mask for EMU_TEMPLOW */
#define _EMU_IFC_TEMPLOW_DEFAULT                     0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_TEMPLOW_DEFAULT                      (_EMU_IFC_TEMPLOW_DEFAULT << 30)              /**< Shifted mode DEFAULT for EMU_IFC */
#define EMU_IFC_TEMPHIGH                             (0x1UL << 31)                                 /**< Clear TEMPHIGH Interrupt Flag */
#define _EMU_IFC_TEMPHIGH_SHIFT                      31                                            /**< Shift value for EMU_TEMPHIGH */
#define _EMU_IFC_TEMPHIGH_MASK                       0x80000000UL                                  /**< Bit mask for EMU_TEMPHIGH */
#define _EMU_IFC_TEMPHIGH_DEFAULT                    0x00000000UL                                  /**< Mode DEFAULT for EMU_IFC */
#define EMU_IFC_TEMPHIGH_DEFAULT                     (_EMU_IFC_TEMPHIGH_DEFAULT << 31)             /**< Shifted mode DEFAULT for EMU_IFC */

/* Bit fields for EMU IEN */
#define _EMU_IEN_RESETVALUE                          0x00000000UL                                  /**< Default value for EMU_IEN */
#define _EMU_IEN_MASK                                0xE11FC0FFUL                                  /**< Mask for EMU_IEN */
#define EMU_IEN_VMONAVDDFALL                         (0x1UL << 0)                                  /**< VMONAVDDFALL Interrupt Enable */
#define _EMU_IEN_VMONAVDDFALL_SHIFT                  0                                             /**< Shift value for EMU_VMONAVDDFALL */
#define _EMU_IEN_VMONAVDDFALL_MASK                   0x1UL                                         /**< Bit mask for EMU_VMONAVDDFALL */
#define _EMU_IEN_VMONAVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONAVDDFALL_DEFAULT                 (_EMU_IEN_VMONAVDDFALL_DEFAULT << 0)          /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONAVDDRISE                         (0x1UL << 1)                                  /**< VMONAVDDRISE Interrupt Enable */
#define _EMU_IEN_VMONAVDDRISE_SHIFT                  1                                             /**< Shift value for EMU_VMONAVDDRISE */
#define _EMU_IEN_VMONAVDDRISE_MASK                   0x2UL                                         /**< Bit mask for EMU_VMONAVDDRISE */
#define _EMU_IEN_VMONAVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONAVDDRISE_DEFAULT                 (_EMU_IEN_VMONAVDDRISE_DEFAULT << 1)          /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONALTAVDDFALL                      (0x1UL << 2)                                  /**< VMONALTAVDDFALL Interrupt Enable */
#define _EMU_IEN_VMONALTAVDDFALL_SHIFT               2                                             /**< Shift value for EMU_VMONALTAVDDFALL */
#define _EMU_IEN_VMONALTAVDDFALL_MASK                0x4UL                                         /**< Bit mask for EMU_VMONALTAVDDFALL */
#define _EMU_IEN_VMONALTAVDDFALL_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONALTAVDDFALL_DEFAULT              (_EMU_IEN_VMONALTAVDDFALL_DEFAULT << 2)       /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONALTAVDDRISE                      (0x1UL << 3)                                  /**< VMONALTAVDDRISE Interrupt Enable */
#define _EMU_IEN_VMONALTAVDDRISE_SHIFT               3                                             /**< Shift value for EMU_VMONALTAVDDRISE */
#define _EMU_IEN_VMONALTAVDDRISE_MASK                0x8UL                                         /**< Bit mask for EMU_VMONALTAVDDRISE */
#define _EMU_IEN_VMONALTAVDDRISE_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONALTAVDDRISE_DEFAULT              (_EMU_IEN_VMONALTAVDDRISE_DEFAULT << 3)       /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONDVDDFALL                         (0x1UL << 4)                                  /**< VMONDVDDFALL Interrupt Enable */
#define _EMU_IEN_VMONDVDDFALL_SHIFT                  4                                             /**< Shift value for EMU_VMONDVDDFALL */
#define _EMU_IEN_VMONDVDDFALL_MASK                   0x10UL                                        /**< Bit mask for EMU_VMONDVDDFALL */
#define _EMU_IEN_VMONDVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONDVDDFALL_DEFAULT                 (_EMU_IEN_VMONDVDDFALL_DEFAULT << 4)          /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONDVDDRISE                         (0x1UL << 5)                                  /**< VMONDVDDRISE Interrupt Enable */
#define _EMU_IEN_VMONDVDDRISE_SHIFT                  5                                             /**< Shift value for EMU_VMONDVDDRISE */
#define _EMU_IEN_VMONDVDDRISE_MASK                   0x20UL                                        /**< Bit mask for EMU_VMONDVDDRISE */
#define _EMU_IEN_VMONDVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONDVDDRISE_DEFAULT                 (_EMU_IEN_VMONDVDDRISE_DEFAULT << 5)          /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONIO0FALL                          (0x1UL << 6)                                  /**< VMONIO0FALL Interrupt Enable */
#define _EMU_IEN_VMONIO0FALL_SHIFT                   6                                             /**< Shift value for EMU_VMONIO0FALL */
#define _EMU_IEN_VMONIO0FALL_MASK                    0x40UL                                        /**< Bit mask for EMU_VMONIO0FALL */
#define _EMU_IEN_VMONIO0FALL_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONIO0FALL_DEFAULT                  (_EMU_IEN_VMONIO0FALL_DEFAULT << 6)           /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONIO0RISE                          (0x1UL << 7)                                  /**< VMONIO0RISE Interrupt Enable */
#define _EMU_IEN_VMONIO0RISE_SHIFT                   7                                             /**< Shift value for EMU_VMONIO0RISE */
#define _EMU_IEN_VMONIO0RISE_MASK                    0x80UL                                        /**< Bit mask for EMU_VMONIO0RISE */
#define _EMU_IEN_VMONIO0RISE_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONIO0RISE_DEFAULT                  (_EMU_IEN_VMONIO0RISE_DEFAULT << 7)           /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONFVDDFALL                         (0x1UL << 14)                                 /**< VMONFVDDFALL Interrupt Enable */
#define _EMU_IEN_VMONFVDDFALL_SHIFT                  14                                            /**< Shift value for EMU_VMONFVDDFALL */
#define _EMU_IEN_VMONFVDDFALL_MASK                   0x4000UL                                      /**< Bit mask for EMU_VMONFVDDFALL */
#define _EMU_IEN_VMONFVDDFALL_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONFVDDFALL_DEFAULT                 (_EMU_IEN_VMONFVDDFALL_DEFAULT << 14)         /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONFVDDRISE                         (0x1UL << 15)                                 /**< VMONFVDDRISE Interrupt Enable */
#define _EMU_IEN_VMONFVDDRISE_SHIFT                  15                                            /**< Shift value for EMU_VMONFVDDRISE */
#define _EMU_IEN_VMONFVDDRISE_MASK                   0x8000UL                                      /**< Bit mask for EMU_VMONFVDDRISE */
#define _EMU_IEN_VMONFVDDRISE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_VMONFVDDRISE_DEFAULT                 (_EMU_IEN_VMONFVDDRISE_DEFAULT << 15)         /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_PFETOVERCURRENTLIMIT                 (0x1UL << 16)                                 /**< PFETOVERCURRENTLIMIT Interrupt Enable */
#define _EMU_IEN_PFETOVERCURRENTLIMIT_SHIFT          16                                            /**< Shift value for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IEN_PFETOVERCURRENTLIMIT_MASK           0x10000UL                                     /**< Bit mask for EMU_PFETOVERCURRENTLIMIT */
#define _EMU_IEN_PFETOVERCURRENTLIMIT_DEFAULT        0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_PFETOVERCURRENTLIMIT_DEFAULT         (_EMU_IEN_PFETOVERCURRENTLIMIT_DEFAULT << 16) /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_NFETOVERCURRENTLIMIT                 (0x1UL << 17)                                 /**< NFETOVERCURRENTLIMIT Interrupt Enable */
#define _EMU_IEN_NFETOVERCURRENTLIMIT_SHIFT          17                                            /**< Shift value for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IEN_NFETOVERCURRENTLIMIT_MASK           0x20000UL                                     /**< Bit mask for EMU_NFETOVERCURRENTLIMIT */
#define _EMU_IEN_NFETOVERCURRENTLIMIT_DEFAULT        0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_NFETOVERCURRENTLIMIT_DEFAULT         (_EMU_IEN_NFETOVERCURRENTLIMIT_DEFAULT << 17) /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_DCDCLPRUNNING                        (0x1UL << 18)                                 /**< DCDCLPRUNNING Interrupt Enable */
#define _EMU_IEN_DCDCLPRUNNING_SHIFT                 18                                            /**< Shift value for EMU_DCDCLPRUNNING */
#define _EMU_IEN_DCDCLPRUNNING_MASK                  0x40000UL                                     /**< Bit mask for EMU_DCDCLPRUNNING */
#define _EMU_IEN_DCDCLPRUNNING_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_DCDCLPRUNNING_DEFAULT                (_EMU_IEN_DCDCLPRUNNING_DEFAULT << 18)        /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_DCDCLNRUNNING                        (0x1UL << 19)                                 /**< DCDCLNRUNNING Interrupt Enable */
#define _EMU_IEN_DCDCLNRUNNING_SHIFT                 19                                            /**< Shift value for EMU_DCDCLNRUNNING */
#define _EMU_IEN_DCDCLNRUNNING_MASK                  0x80000UL                                     /**< Bit mask for EMU_DCDCLNRUNNING */
#define _EMU_IEN_DCDCLNRUNNING_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_DCDCLNRUNNING_DEFAULT                (_EMU_IEN_DCDCLNRUNNING_DEFAULT << 19)        /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_DCDCINBYPASS                         (0x1UL << 20)                                 /**< DCDCINBYPASS Interrupt Enable */
#define _EMU_IEN_DCDCINBYPASS_SHIFT                  20                                            /**< Shift value for EMU_DCDCINBYPASS */
#define _EMU_IEN_DCDCINBYPASS_MASK                   0x100000UL                                    /**< Bit mask for EMU_DCDCINBYPASS */
#define _EMU_IEN_DCDCINBYPASS_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_DCDCINBYPASS_DEFAULT                 (_EMU_IEN_DCDCINBYPASS_DEFAULT << 20)         /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_EM23WAKEUP                           (0x1UL << 24)                                 /**< EM23WAKEUP Interrupt Enable */
#define _EMU_IEN_EM23WAKEUP_SHIFT                    24                                            /**< Shift value for EMU_EM23WAKEUP */
#define _EMU_IEN_EM23WAKEUP_MASK                     0x1000000UL                                   /**< Bit mask for EMU_EM23WAKEUP */
#define _EMU_IEN_EM23WAKEUP_DEFAULT                  0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_EM23WAKEUP_DEFAULT                   (_EMU_IEN_EM23WAKEUP_DEFAULT << 24)           /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_TEMP                                 (0x1UL << 29)                                 /**< TEMP Interrupt Enable */
#define _EMU_IEN_TEMP_SHIFT                          29                                            /**< Shift value for EMU_TEMP */
#define _EMU_IEN_TEMP_MASK                           0x20000000UL                                  /**< Bit mask for EMU_TEMP */
#define _EMU_IEN_TEMP_DEFAULT                        0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_TEMP_DEFAULT                         (_EMU_IEN_TEMP_DEFAULT << 29)                 /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_TEMPLOW                              (0x1UL << 30)                                 /**< TEMPLOW Interrupt Enable */
#define _EMU_IEN_TEMPLOW_SHIFT                       30                                            /**< Shift value for EMU_TEMPLOW */
#define _EMU_IEN_TEMPLOW_MASK                        0x40000000UL                                  /**< Bit mask for EMU_TEMPLOW */
#define _EMU_IEN_TEMPLOW_DEFAULT                     0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_TEMPLOW_DEFAULT                      (_EMU_IEN_TEMPLOW_DEFAULT << 30)              /**< Shifted mode DEFAULT for EMU_IEN */
#define EMU_IEN_TEMPHIGH                             (0x1UL << 31)                                 /**< TEMPHIGH Interrupt Enable */
#define _EMU_IEN_TEMPHIGH_SHIFT                      31                                            /**< Shift value for EMU_TEMPHIGH */
#define _EMU_IEN_TEMPHIGH_MASK                       0x80000000UL                                  /**< Bit mask for EMU_TEMPHIGH */
#define _EMU_IEN_TEMPHIGH_DEFAULT                    0x00000000UL                                  /**< Mode DEFAULT for EMU_IEN */
#define EMU_IEN_TEMPHIGH_DEFAULT                     (_EMU_IEN_TEMPHIGH_DEFAULT << 31)             /**< Shifted mode DEFAULT for EMU_IEN */

/* Bit fields for EMU PWRLOCK */
#define _EMU_PWRLOCK_RESETVALUE                      0x00000000UL                         /**< Default value for EMU_PWRLOCK */
#define _EMU_PWRLOCK_MASK                            0x0000FFFFUL                         /**< Mask for EMU_PWRLOCK */
#define _EMU_PWRLOCK_LOCKKEY_SHIFT                   0                                    /**< Shift value for EMU_LOCKKEY */
#define _EMU_PWRLOCK_LOCKKEY_MASK                    0xFFFFUL                             /**< Bit mask for EMU_LOCKKEY */
#define _EMU_PWRLOCK_LOCKKEY_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for EMU_PWRLOCK */
#define _EMU_PWRLOCK_LOCKKEY_LOCK                    0x00000000UL                         /**< Mode LOCK for EMU_PWRLOCK */
#define _EMU_PWRLOCK_LOCKKEY_UNLOCKED                0x00000000UL                         /**< Mode UNLOCKED for EMU_PWRLOCK */
#define _EMU_PWRLOCK_LOCKKEY_LOCKED                  0x00000001UL                         /**< Mode LOCKED for EMU_PWRLOCK */
#define _EMU_PWRLOCK_LOCKKEY_UNLOCK                  0x0000ADE8UL                         /**< Mode UNLOCK for EMU_PWRLOCK */
#define EMU_PWRLOCK_LOCKKEY_DEFAULT                  (_EMU_PWRLOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for EMU_PWRLOCK */
#define EMU_PWRLOCK_LOCKKEY_LOCK                     (_EMU_PWRLOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for EMU_PWRLOCK */
#define EMU_PWRLOCK_LOCKKEY_UNLOCKED                 (_EMU_PWRLOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for EMU_PWRLOCK */
#define EMU_PWRLOCK_LOCKKEY_LOCKED                   (_EMU_PWRLOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for EMU_PWRLOCK */
#define EMU_PWRLOCK_LOCKKEY_UNLOCK                   (_EMU_PWRLOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for EMU_PWRLOCK */

/* Bit fields for EMU PWRCFG */
#define _EMU_PWRCFG_RESETVALUE                       0x00000000UL                         /**< Default value for EMU_PWRCFG */
#define _EMU_PWRCFG_MASK                             0x0000000FUL                         /**< Mask for EMU_PWRCFG */
#define _EMU_PWRCFG_PWRCFG_SHIFT                     0                                    /**< Shift value for EMU_PWRCFG */
#define _EMU_PWRCFG_PWRCFG_MASK                      0xFUL                                /**< Bit mask for EMU_PWRCFG */
#define _EMU_PWRCFG_PWRCFG_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for EMU_PWRCFG */
#define _EMU_PWRCFG_PWRCFG_STARTUP                   0x00000000UL                         /**< Mode STARTUP for EMU_PWRCFG */
#define _EMU_PWRCFG_PWRCFG_DCDCTODVDD                0x00000002UL                         /**< Mode DCDCTODVDD for EMU_PWRCFG */
#define EMU_PWRCFG_PWRCFG_DEFAULT                    (_EMU_PWRCFG_PWRCFG_DEFAULT << 0)    /**< Shifted mode DEFAULT for EMU_PWRCFG */
#define EMU_PWRCFG_PWRCFG_STARTUP                    (_EMU_PWRCFG_PWRCFG_STARTUP << 0)    /**< Shifted mode STARTUP for EMU_PWRCFG */
#define EMU_PWRCFG_PWRCFG_DCDCTODVDD                 (_EMU_PWRCFG_PWRCFG_DCDCTODVDD << 0) /**< Shifted mode DCDCTODVDD for EMU_PWRCFG */

/* Bit fields for EMU PWRCTRL */
#define _EMU_PWRCTRL_RESETVALUE                      0x00000000UL                      /**< Default value for EMU_PWRCTRL */
#define _EMU_PWRCTRL_MASK                            0x00000020UL                      /**< Mask for EMU_PWRCTRL */
#define EMU_PWRCTRL_ANASW                            (0x1UL << 5)                      /**< Analog Switch Selection */
#define _EMU_PWRCTRL_ANASW_SHIFT                     5                                 /**< Shift value for EMU_ANASW */
#define _EMU_PWRCTRL_ANASW_MASK                      0x20UL                            /**< Bit mask for EMU_ANASW */
#define _EMU_PWRCTRL_ANASW_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for EMU_PWRCTRL */
#define _EMU_PWRCTRL_ANASW_AVDD                      0x00000000UL                      /**< Mode AVDD for EMU_PWRCTRL */
#define _EMU_PWRCTRL_ANASW_DVDD                      0x00000001UL                      /**< Mode DVDD for EMU_PWRCTRL */
#define EMU_PWRCTRL_ANASW_DEFAULT                    (_EMU_PWRCTRL_ANASW_DEFAULT << 5) /**< Shifted mode DEFAULT for EMU_PWRCTRL */
#define EMU_PWRCTRL_ANASW_AVDD                       (_EMU_PWRCTRL_ANASW_AVDD << 5)    /**< Shifted mode AVDD for EMU_PWRCTRL */
#define EMU_PWRCTRL_ANASW_DVDD                       (_EMU_PWRCTRL_ANASW_DVDD << 5)    /**< Shifted mode DVDD for EMU_PWRCTRL */

/* Bit fields for EMU DCDCCTRL */
#define _EMU_DCDCCTRL_RESETVALUE                     0x00000030UL                                   /**< Default value for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_MASK                           0x00000033UL                                   /**< Mask for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODE_SHIFT                 0                                              /**< Shift value for EMU_DCDCMODE */
#define _EMU_DCDCCTRL_DCDCMODE_MASK                  0x3UL                                          /**< Bit mask for EMU_DCDCMODE */
#define _EMU_DCDCCTRL_DCDCMODE_DEFAULT               0x00000000UL                                   /**< Mode DEFAULT for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODE_BYPASS                0x00000000UL                                   /**< Mode BYPASS for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODE_LOWNOISE              0x00000001UL                                   /**< Mode LOWNOISE for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODE_LOWPOWER              0x00000002UL                                   /**< Mode LOWPOWER for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODE_OFF                   0x00000003UL                                   /**< Mode OFF for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODE_DEFAULT                (_EMU_DCDCCTRL_DCDCMODE_DEFAULT << 0)          /**< Shifted mode DEFAULT for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODE_BYPASS                 (_EMU_DCDCCTRL_DCDCMODE_BYPASS << 0)           /**< Shifted mode BYPASS for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODE_LOWNOISE               (_EMU_DCDCCTRL_DCDCMODE_LOWNOISE << 0)         /**< Shifted mode LOWNOISE for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODE_LOWPOWER               (_EMU_DCDCCTRL_DCDCMODE_LOWPOWER << 0)         /**< Shifted mode LOWPOWER for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODE_OFF                    (_EMU_DCDCCTRL_DCDCMODE_OFF << 0)              /**< Shifted mode OFF for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM23                    (0x1UL << 4)                                   /**< DCDC Mode EM23 */
#define _EMU_DCDCCTRL_DCDCMODEEM23_SHIFT             4                                              /**< Shift value for EMU_DCDCMODEEM23 */
#define _EMU_DCDCCTRL_DCDCMODEEM23_MASK              0x10UL                                         /**< Bit mask for EMU_DCDCMODEEM23 */
#define _EMU_DCDCCTRL_DCDCMODEEM23_EM23SW            0x00000000UL                                   /**< Mode EM23SW for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODEEM23_DEFAULT           0x00000001UL                                   /**< Mode DEFAULT for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODEEM23_EM23LOWPOWER      0x00000001UL                                   /**< Mode EM23LOWPOWER for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM23_EM23SW             (_EMU_DCDCCTRL_DCDCMODEEM23_EM23SW << 4)       /**< Shifted mode EM23SW for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM23_DEFAULT            (_EMU_DCDCCTRL_DCDCMODEEM23_DEFAULT << 4)      /**< Shifted mode DEFAULT for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM23_EM23LOWPOWER       (_EMU_DCDCCTRL_DCDCMODEEM23_EM23LOWPOWER << 4) /**< Shifted mode EM23LOWPOWER for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM4                     (0x1UL << 5)                                   /**< DCDC Mode EM4H */
#define _EMU_DCDCCTRL_DCDCMODEEM4_SHIFT              5                                              /**< Shift value for EMU_DCDCMODEEM4 */
#define _EMU_DCDCCTRL_DCDCMODEEM4_MASK               0x20UL                                         /**< Bit mask for EMU_DCDCMODEEM4 */
#define _EMU_DCDCCTRL_DCDCMODEEM4_EM4SW              0x00000000UL                                   /**< Mode EM4SW for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODEEM4_DEFAULT            0x00000001UL                                   /**< Mode DEFAULT for EMU_DCDCCTRL */
#define _EMU_DCDCCTRL_DCDCMODEEM4_EM4LOWPOWER        0x00000001UL                                   /**< Mode EM4LOWPOWER for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM4_EM4SW               (_EMU_DCDCCTRL_DCDCMODEEM4_EM4SW << 5)         /**< Shifted mode EM4SW for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM4_DEFAULT             (_EMU_DCDCCTRL_DCDCMODEEM4_DEFAULT << 5)       /**< Shifted mode DEFAULT for EMU_DCDCCTRL */
#define EMU_DCDCCTRL_DCDCMODEEM4_EM4LOWPOWER         (_EMU_DCDCCTRL_DCDCMODEEM4_EM4LOWPOWER << 5)   /**< Shifted mode EM4LOWPOWER for EMU_DCDCCTRL */

/* Bit fields for EMU DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_RESETVALUE                 0x33307700UL                                    /**< Default value for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_MASK                       0x377FFF01UL                                    /**< Mask for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LNFORCECCM                  (0x1UL << 0)                                    /**< Force DCDC into CCM mode in low noise operation */
#define _EMU_DCDCMISCCTRL_LNFORCECCM_SHIFT           0                                               /**< Shift value for EMU_LNFORCECCM */
#define _EMU_DCDCMISCCTRL_LNFORCECCM_MASK            0x1UL                                           /**< Bit mask for EMU_LNFORCECCM */
#define _EMU_DCDCMISCCTRL_LNFORCECCM_DEFAULT         0x00000000UL                                    /**< Mode DEFAULT for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LNFORCECCM_DEFAULT          (_EMU_DCDCMISCCTRL_LNFORCECCM_DEFAULT << 0)     /**< Shifted mode DEFAULT for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_PFETCNT_SHIFT              8                                               /**< Shift value for EMU_PFETCNT */
#define _EMU_DCDCMISCCTRL_PFETCNT_MASK               0xF00UL                                         /**< Bit mask for EMU_PFETCNT */
#define _EMU_DCDCMISCCTRL_PFETCNT_DEFAULT            0x00000007UL                                    /**< Mode DEFAULT for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_PFETCNT_DEFAULT             (_EMU_DCDCMISCCTRL_PFETCNT_DEFAULT << 8)        /**< Shifted mode DEFAULT for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_NFETCNT_SHIFT              12                                              /**< Shift value for EMU_NFETCNT */
#define _EMU_DCDCMISCCTRL_NFETCNT_MASK               0xF000UL                                        /**< Bit mask for EMU_NFETCNT */
#define _EMU_DCDCMISCCTRL_NFETCNT_DEFAULT            0x00000007UL                                    /**< Mode DEFAULT for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_NFETCNT_DEFAULT             (_EMU_DCDCMISCCTRL_NFETCNT_DEFAULT << 12)       /**< Shifted mode DEFAULT for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_BYPLIMSEL_SHIFT            16                                              /**< Shift value for EMU_BYPLIMSEL */
#define _EMU_DCDCMISCCTRL_BYPLIMSEL_MASK             0xF0000UL                                       /**< Bit mask for EMU_BYPLIMSEL */
#define _EMU_DCDCMISCCTRL_BYPLIMSEL_DEFAULT          0x00000000UL                                    /**< Mode DEFAULT for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_BYPLIMSEL_DEFAULT           (_EMU_DCDCMISCCTRL_BYPLIMSEL_DEFAULT << 16)     /**< Shifted mode DEFAULT for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_LPCLIMILIMSEL_SHIFT        20                                              /**< Shift value for EMU_LPCLIMILIMSEL */
#define _EMU_DCDCMISCCTRL_LPCLIMILIMSEL_MASK         0x700000UL                                      /**< Bit mask for EMU_LPCLIMILIMSEL */
#define _EMU_DCDCMISCCTRL_LPCLIMILIMSEL_DEFAULT      0x00000003UL                                    /**< Mode DEFAULT for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LPCLIMILIMSEL_DEFAULT       (_EMU_DCDCMISCCTRL_LPCLIMILIMSEL_DEFAULT << 20) /**< Shifted mode DEFAULT for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_LNCLIMILIMSEL_SHIFT        24                                              /**< Shift value for EMU_LNCLIMILIMSEL */
#define _EMU_DCDCMISCCTRL_LNCLIMILIMSEL_MASK         0x7000000UL                                     /**< Bit mask for EMU_LNCLIMILIMSEL */
#define _EMU_DCDCMISCCTRL_LNCLIMILIMSEL_DEFAULT      0x00000003UL                                    /**< Mode DEFAULT for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LNCLIMILIMSEL_DEFAULT       (_EMU_DCDCMISCCTRL_LNCLIMILIMSEL_DEFAULT << 24) /**< Shifted mode DEFAULT for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_LPCMPBIAS_SHIFT            28                                              /**< Shift value for EMU_LPCMPBIAS */
#define _EMU_DCDCMISCCTRL_LPCMPBIAS_MASK             0x30000000UL                                    /**< Bit mask for EMU_LPCMPBIAS */
#define _EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS0            0x00000000UL                                    /**< Mode BIAS0 for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS1            0x00000001UL                                    /**< Mode BIAS1 for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS2            0x00000002UL                                    /**< Mode BIAS2 for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_LPCMPBIAS_DEFAULT          0x00000003UL                                    /**< Mode DEFAULT for EMU_DCDCMISCCTRL */
#define _EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS3            0x00000003UL                                    /**< Mode BIAS3 for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS0             (_EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS0 << 28)       /**< Shifted mode BIAS0 for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS1             (_EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS1 << 28)       /**< Shifted mode BIAS1 for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS2             (_EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS2 << 28)       /**< Shifted mode BIAS2 for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LPCMPBIAS_DEFAULT           (_EMU_DCDCMISCCTRL_LPCMPBIAS_DEFAULT << 28)     /**< Shifted mode DEFAULT for EMU_DCDCMISCCTRL */
#define EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS3             (_EMU_DCDCMISCCTRL_LPCMPBIAS_BIAS3 << 28)       /**< Shifted mode BIAS3 for EMU_DCDCMISCCTRL */

/* Bit fields for EMU DCDCZDETCTRL */
#define _EMU_DCDCZDETCTRL_RESETVALUE                 0x00000130UL                                  /**< Default value for EMU_DCDCZDETCTRL */
#define _EMU_DCDCZDETCTRL_MASK                       0x00000370UL                                  /**< Mask for EMU_DCDCZDETCTRL */
#define _EMU_DCDCZDETCTRL_ZDETILIMSEL_SHIFT          4                                             /**< Shift value for EMU_ZDETILIMSEL */
#define _EMU_DCDCZDETCTRL_ZDETILIMSEL_MASK           0x70UL                                        /**< Bit mask for EMU_ZDETILIMSEL */
#define _EMU_DCDCZDETCTRL_ZDETILIMSEL_DEFAULT        0x00000003UL                                  /**< Mode DEFAULT for EMU_DCDCZDETCTRL */
#define EMU_DCDCZDETCTRL_ZDETILIMSEL_DEFAULT         (_EMU_DCDCZDETCTRL_ZDETILIMSEL_DEFAULT << 4)  /**< Shifted mode DEFAULT for EMU_DCDCZDETCTRL */
#define _EMU_DCDCZDETCTRL_ZDETBLANKDLY_SHIFT         8                                             /**< Shift value for EMU_ZDETBLANKDLY */
#define _EMU_DCDCZDETCTRL_ZDETBLANKDLY_MASK          0x300UL                                       /**< Bit mask for EMU_ZDETBLANKDLY */
#define _EMU_DCDCZDETCTRL_ZDETBLANKDLY_DEFAULT       0x00000001UL                                  /**< Mode DEFAULT for EMU_DCDCZDETCTRL */
#define EMU_DCDCZDETCTRL_ZDETBLANKDLY_DEFAULT        (_EMU_DCDCZDETCTRL_ZDETBLANKDLY_DEFAULT << 8) /**< Shifted mode DEFAULT for EMU_DCDCZDETCTRL */

/* Bit fields for EMU DCDCCLIMCTRL */
#define _EMU_DCDCCLIMCTRL_RESETVALUE                 0x00002100UL                                  /**< Default value for EMU_DCDCCLIMCTRL */
#define _EMU_DCDCCLIMCTRL_MASK                       0x00002300UL                                  /**< Mask for EMU_DCDCCLIMCTRL */
#define _EMU_DCDCCLIMCTRL_CLIMBLANKDLY_SHIFT         8                                             /**< Shift value for EMU_CLIMBLANKDLY */
#define _EMU_DCDCCLIMCTRL_CLIMBLANKDLY_MASK          0x300UL                                       /**< Bit mask for EMU_CLIMBLANKDLY */
#define _EMU_DCDCCLIMCTRL_CLIMBLANKDLY_DEFAULT       0x00000001UL                                  /**< Mode DEFAULT for EMU_DCDCCLIMCTRL */
#define EMU_DCDCCLIMCTRL_CLIMBLANKDLY_DEFAULT        (_EMU_DCDCCLIMCTRL_CLIMBLANKDLY_DEFAULT << 8) /**< Shifted mode DEFAULT for EMU_DCDCCLIMCTRL */
#define EMU_DCDCCLIMCTRL_BYPLIMEN                    (0x1UL << 13)                                 /**< Bypass Current Limit Enable */
#define _EMU_DCDCCLIMCTRL_BYPLIMEN_SHIFT             13                                            /**< Shift value for EMU_BYPLIMEN */
#define _EMU_DCDCCLIMCTRL_BYPLIMEN_MASK              0x2000UL                                      /**< Bit mask for EMU_BYPLIMEN */
#define _EMU_DCDCCLIMCTRL_BYPLIMEN_DEFAULT           0x00000001UL                                  /**< Mode DEFAULT for EMU_DCDCCLIMCTRL */
#define EMU_DCDCCLIMCTRL_BYPLIMEN_DEFAULT            (_EMU_DCDCCLIMCTRL_BYPLIMEN_DEFAULT << 13)    /**< Shifted mode DEFAULT for EMU_DCDCCLIMCTRL */

/* Bit fields for EMU DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_RESETVALUE               0x57204077UL                                 /**< Default value for EMU_DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_MASK                     0xF730F1F7UL                                 /**< Mask for EMU_DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_COMPENR1_SHIFT           0                                            /**< Shift value for EMU_COMPENR1 */
#define _EMU_DCDCLNCOMPCTRL_COMPENR1_MASK            0x7UL                                        /**< Bit mask for EMU_COMPENR1 */
#define _EMU_DCDCLNCOMPCTRL_COMPENR1_DEFAULT         0x00000007UL                                 /**< Mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define EMU_DCDCLNCOMPCTRL_COMPENR1_DEFAULT          (_EMU_DCDCLNCOMPCTRL_COMPENR1_DEFAULT << 0)  /**< Shifted mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_COMPENR2_SHIFT           4                                            /**< Shift value for EMU_COMPENR2 */
#define _EMU_DCDCLNCOMPCTRL_COMPENR2_MASK            0x1F0UL                                      /**< Bit mask for EMU_COMPENR2 */
#define _EMU_DCDCLNCOMPCTRL_COMPENR2_DEFAULT         0x00000007UL                                 /**< Mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define EMU_DCDCLNCOMPCTRL_COMPENR2_DEFAULT          (_EMU_DCDCLNCOMPCTRL_COMPENR2_DEFAULT << 4)  /**< Shifted mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_COMPENR3_SHIFT           12                                           /**< Shift value for EMU_COMPENR3 */
#define _EMU_DCDCLNCOMPCTRL_COMPENR3_MASK            0xF000UL                                     /**< Bit mask for EMU_COMPENR3 */
#define _EMU_DCDCLNCOMPCTRL_COMPENR3_DEFAULT         0x00000004UL                                 /**< Mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define EMU_DCDCLNCOMPCTRL_COMPENR3_DEFAULT          (_EMU_DCDCLNCOMPCTRL_COMPENR3_DEFAULT << 12) /**< Shifted mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_COMPENC1_SHIFT           20                                           /**< Shift value for EMU_COMPENC1 */
#define _EMU_DCDCLNCOMPCTRL_COMPENC1_MASK            0x300000UL                                   /**< Bit mask for EMU_COMPENC1 */
#define _EMU_DCDCLNCOMPCTRL_COMPENC1_DEFAULT         0x00000002UL                                 /**< Mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define EMU_DCDCLNCOMPCTRL_COMPENC1_DEFAULT          (_EMU_DCDCLNCOMPCTRL_COMPENC1_DEFAULT << 20) /**< Shifted mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_COMPENC2_SHIFT           24                                           /**< Shift value for EMU_COMPENC2 */
#define _EMU_DCDCLNCOMPCTRL_COMPENC2_MASK            0x7000000UL                                  /**< Bit mask for EMU_COMPENC2 */
#define _EMU_DCDCLNCOMPCTRL_COMPENC2_DEFAULT         0x00000007UL                                 /**< Mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define EMU_DCDCLNCOMPCTRL_COMPENC2_DEFAULT          (_EMU_DCDCLNCOMPCTRL_COMPENC2_DEFAULT << 24) /**< Shifted mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define _EMU_DCDCLNCOMPCTRL_COMPENC3_SHIFT           28                                           /**< Shift value for EMU_COMPENC3 */
#define _EMU_DCDCLNCOMPCTRL_COMPENC3_MASK            0xF0000000UL                                 /**< Bit mask for EMU_COMPENC3 */
#define _EMU_DCDCLNCOMPCTRL_COMPENC3_DEFAULT         0x00000005UL                                 /**< Mode DEFAULT for EMU_DCDCLNCOMPCTRL */
#define EMU_DCDCLNCOMPCTRL_COMPENC3_DEFAULT          (_EMU_DCDCLNCOMPCTRL_COMPENC3_DEFAULT << 28) /**< Shifted mode DEFAULT for EMU_DCDCLNCOMPCTRL */

/* Bit fields for EMU DCDCLNVCTRL */
#define _EMU_DCDCLNVCTRL_RESETVALUE                  0x00007100UL                           /**< Default value for EMU_DCDCLNVCTRL */
#define _EMU_DCDCLNVCTRL_MASK                        0x00007F02UL                           /**< Mask for EMU_DCDCLNVCTRL */
#define EMU_DCDCLNVCTRL_LNATT                        (0x1UL << 1)                           /**< Low Noise Mode Feedback Attenuation */
#define _EMU_DCDCLNVCTRL_LNATT_SHIFT                 1                                      /**< Shift value for EMU_LNATT */
#define _EMU_DCDCLNVCTRL_LNATT_MASK                  0x2UL                                  /**< Bit mask for EMU_LNATT */
#define _EMU_DCDCLNVCTRL_LNATT_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for EMU_DCDCLNVCTRL */
#define _EMU_DCDCLNVCTRL_LNATT_DIV3                  0x00000000UL                           /**< Mode DIV3 for EMU_DCDCLNVCTRL */
#define _EMU_DCDCLNVCTRL_LNATT_DIV6                  0x00000001UL                           /**< Mode DIV6 for EMU_DCDCLNVCTRL */
#define EMU_DCDCLNVCTRL_LNATT_DEFAULT                (_EMU_DCDCLNVCTRL_LNATT_DEFAULT << 1)  /**< Shifted mode DEFAULT for EMU_DCDCLNVCTRL */
#define EMU_DCDCLNVCTRL_LNATT_DIV3                   (_EMU_DCDCLNVCTRL_LNATT_DIV3 << 1)     /**< Shifted mode DIV3 for EMU_DCDCLNVCTRL */
#define EMU_DCDCLNVCTRL_LNATT_DIV6                   (_EMU_DCDCLNVCTRL_LNATT_DIV6 << 1)     /**< Shifted mode DIV6 for EMU_DCDCLNVCTRL */
#define _EMU_DCDCLNVCTRL_LNVREF_SHIFT                8                                      /**< Shift value for EMU_LNVREF */
#define _EMU_DCDCLNVCTRL_LNVREF_MASK                 0x7F00UL                               /**< Bit mask for EMU_LNVREF */
#define _EMU_DCDCLNVCTRL_LNVREF_DEFAULT              0x00000071UL                           /**< Mode DEFAULT for EMU_DCDCLNVCTRL */
#define EMU_DCDCLNVCTRL_LNVREF_DEFAULT               (_EMU_DCDCLNVCTRL_LNVREF_DEFAULT << 8) /**< Shifted mode DEFAULT for EMU_DCDCLNVCTRL */

/* Bit fields for EMU DCDCTIMING */
#define _EMU_DCDCTIMING_RESETVALUE                   0x0FF1F8FFUL                                  /**< Default value for EMU_DCDCTIMING */
#define _EMU_DCDCTIMING_MASK                         0x6FF1F8FFUL                                  /**< Mask for EMU_DCDCTIMING */
#define _EMU_DCDCTIMING_LPINITWAIT_SHIFT             0                                             /**< Shift value for EMU_LPINITWAIT */
#define _EMU_DCDCTIMING_LPINITWAIT_MASK              0xFFUL                                        /**< Bit mask for EMU_LPINITWAIT */
#define _EMU_DCDCTIMING_LPINITWAIT_DEFAULT           0x000000FFUL                                  /**< Mode DEFAULT for EMU_DCDCTIMING */
#define EMU_DCDCTIMING_LPINITWAIT_DEFAULT            (_EMU_DCDCTIMING_LPINITWAIT_DEFAULT << 0)     /**< Shifted mode DEFAULT for EMU_DCDCTIMING */
#define EMU_DCDCTIMING_COMPENPRCHGEN                 (0x1UL << 11)                                 /**< LN mode precharge enable */
#define _EMU_DCDCTIMING_COMPENPRCHGEN_SHIFT          11                                            /**< Shift value for EMU_COMPENPRCHGEN */
#define _EMU_DCDCTIMING_COMPENPRCHGEN_MASK           0x800UL                                       /**< Bit mask for EMU_COMPENPRCHGEN */
#define _EMU_DCDCTIMING_COMPENPRCHGEN_DEFAULT        0x00000001UL                                  /**< Mode DEFAULT for EMU_DCDCTIMING */
#define EMU_DCDCTIMING_COMPENPRCHGEN_DEFAULT         (_EMU_DCDCTIMING_COMPENPRCHGEN_DEFAULT << 11) /**< Shifted mode DEFAULT for EMU_DCDCTIMING */
#define _EMU_DCDCTIMING_LNWAIT_SHIFT                 12                                            /**< Shift value for EMU_LNWAIT */
#define _EMU_DCDCTIMING_LNWAIT_MASK                  0x1F000UL                                     /**< Bit mask for EMU_LNWAIT */
#define _EMU_DCDCTIMING_LNWAIT_DEFAULT               0x0000001FUL                                  /**< Mode DEFAULT for EMU_DCDCTIMING */
#define EMU_DCDCTIMING_LNWAIT_DEFAULT                (_EMU_DCDCTIMING_LNWAIT_DEFAULT << 12)        /**< Shifted mode DEFAULT for EMU_DCDCTIMING */
#define _EMU_DCDCTIMING_BYPWAIT_SHIFT                20                                            /**< Shift value for EMU_BYPWAIT */
#define _EMU_DCDCTIMING_BYPWAIT_MASK                 0xFF00000UL                                   /**< Bit mask for EMU_BYPWAIT */
#define _EMU_DCDCTIMING_BYPWAIT_DEFAULT              0x000000FFUL                                  /**< Mode DEFAULT for EMU_DCDCTIMING */
#define EMU_DCDCTIMING_BYPWAIT_DEFAULT               (_EMU_DCDCTIMING_BYPWAIT_DEFAULT << 20)       /**< Shifted mode DEFAULT for EMU_DCDCTIMING */
#define _EMU_DCDCTIMING_DUTYSCALE_SHIFT              29                                            /**< Shift value for EMU_DUTYSCALE */
#define _EMU_DCDCTIMING_DUTYSCALE_MASK               0x60000000UL                                  /**< Bit mask for EMU_DUTYSCALE */
#define _EMU_DCDCTIMING_DUTYSCALE_DEFAULT            0x00000000UL                                  /**< Mode DEFAULT for EMU_DCDCTIMING */
#define EMU_DCDCTIMING_DUTYSCALE_DEFAULT             (_EMU_DCDCTIMING_DUTYSCALE_DEFAULT << 29)     /**< Shifted mode DEFAULT for EMU_DCDCTIMING */

/* Bit fields for EMU DCDCLPVCTRL */
#define _EMU_DCDCLPVCTRL_RESETVALUE                  0x00000168UL                           /**< Default value for EMU_DCDCLPVCTRL */
#define _EMU_DCDCLPVCTRL_MASK                        0x000001FFUL                           /**< Mask for EMU_DCDCLPVCTRL */
#define EMU_DCDCLPVCTRL_LPATT                        (0x1UL << 0)                           /**< Low power feedback attenuation */
#define _EMU_DCDCLPVCTRL_LPATT_SHIFT                 0                                      /**< Shift value for EMU_LPATT */
#define _EMU_DCDCLPVCTRL_LPATT_MASK                  0x1UL                                  /**< Bit mask for EMU_LPATT */
#define _EMU_DCDCLPVCTRL_LPATT_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for EMU_DCDCLPVCTRL */
#define _EMU_DCDCLPVCTRL_LPATT_DIV4                  0x00000000UL                           /**< Mode DIV4 for EMU_DCDCLPVCTRL */
#define _EMU_DCDCLPVCTRL_LPATT_DIV8                  0x00000001UL                           /**< Mode DIV8 for EMU_DCDCLPVCTRL */
#define EMU_DCDCLPVCTRL_LPATT_DEFAULT                (_EMU_DCDCLPVCTRL_LPATT_DEFAULT << 0)  /**< Shifted mode DEFAULT for EMU_DCDCLPVCTRL */
#define EMU_DCDCLPVCTRL_LPATT_DIV4                   (_EMU_DCDCLPVCTRL_LPATT_DIV4 << 0)     /**< Shifted mode DIV4 for EMU_DCDCLPVCTRL */
#define EMU_DCDCLPVCTRL_LPATT_DIV8                   (_EMU_DCDCLPVCTRL_LPATT_DIV8 << 0)     /**< Shifted mode DIV8 for EMU_DCDCLPVCTRL */
#define _EMU_DCDCLPVCTRL_LPVREF_SHIFT                1                                      /**< Shift value for EMU_LPVREF */
#define _EMU_DCDCLPVCTRL_LPVREF_MASK                 0x1FEUL                                /**< Bit mask for EMU_LPVREF */
#define _EMU_DCDCLPVCTRL_LPVREF_DEFAULT              0x000000B4UL                           /**< Mode DEFAULT for EMU_DCDCLPVCTRL */
#define EMU_DCDCLPVCTRL_LPVREF_DEFAULT               (_EMU_DCDCLPVCTRL_LPVREF_DEFAULT << 1) /**< Shifted mode DEFAULT for EMU_DCDCLPVCTRL */

/* Bit fields for EMU DCDCLPCTRL */
#define _EMU_DCDCLPCTRL_RESETVALUE                   0x00007000UL                                 /**< Default value for EMU_DCDCLPCTRL */
#define _EMU_DCDCLPCTRL_MASK                         0x0700F000UL                                 /**< Mask for EMU_DCDCLPCTRL */
#define _EMU_DCDCLPCTRL_LPCMPHYSSEL_SHIFT            12                                           /**< Shift value for EMU_LPCMPHYSSEL */
#define _EMU_DCDCLPCTRL_LPCMPHYSSEL_MASK             0xF000UL                                     /**< Bit mask for EMU_LPCMPHYSSEL */
#define _EMU_DCDCLPCTRL_LPCMPHYSSEL_DEFAULT          0x00000007UL                                 /**< Mode DEFAULT for EMU_DCDCLPCTRL */
#define EMU_DCDCLPCTRL_LPCMPHYSSEL_DEFAULT           (_EMU_DCDCLPCTRL_LPCMPHYSSEL_DEFAULT << 12)  /**< Shifted mode DEFAULT for EMU_DCDCLPCTRL */
#define EMU_DCDCLPCTRL_LPVREFDUTYEN                  (0x1UL << 24)                                /**< LP mode duty cycling enable */
#define _EMU_DCDCLPCTRL_LPVREFDUTYEN_SHIFT           24                                           /**< Shift value for EMU_LPVREFDUTYEN */
#define _EMU_DCDCLPCTRL_LPVREFDUTYEN_MASK            0x1000000UL                                  /**< Bit mask for EMU_LPVREFDUTYEN */
#define _EMU_DCDCLPCTRL_LPVREFDUTYEN_DEFAULT         0x00000000UL                                 /**< Mode DEFAULT for EMU_DCDCLPCTRL */
#define EMU_DCDCLPCTRL_LPVREFDUTYEN_DEFAULT          (_EMU_DCDCLPCTRL_LPVREFDUTYEN_DEFAULT << 24) /**< Shifted mode DEFAULT for EMU_DCDCLPCTRL */
#define _EMU_DCDCLPCTRL_LPBLANK_SHIFT                25                                           /**< Shift value for EMU_LPBLANK */
#define _EMU_DCDCLPCTRL_LPBLANK_MASK                 0x6000000UL                                  /**< Bit mask for EMU_LPBLANK */
#define _EMU_DCDCLPCTRL_LPBLANK_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for EMU_DCDCLPCTRL */
#define EMU_DCDCLPCTRL_LPBLANK_DEFAULT               (_EMU_DCDCLPCTRL_LPBLANK_DEFAULT << 25)      /**< Shifted mode DEFAULT for EMU_DCDCLPCTRL */

/* Bit fields for EMU DCDCLNFREQCTRL */
#define _EMU_DCDCLNFREQCTRL_RESETVALUE               0x10000000UL                                /**< Default value for EMU_DCDCLNFREQCTRL */
#define _EMU_DCDCLNFREQCTRL_MASK                     0x1F000007UL                                /**< Mask for EMU_DCDCLNFREQCTRL */
#define _EMU_DCDCLNFREQCTRL_RCOBAND_SHIFT            0                                           /**< Shift value for EMU_RCOBAND */
#define _EMU_DCDCLNFREQCTRL_RCOBAND_MASK             0x7UL                                       /**< Bit mask for EMU_RCOBAND */
#define _EMU_DCDCLNFREQCTRL_RCOBAND_DEFAULT          0x00000000UL                                /**< Mode DEFAULT for EMU_DCDCLNFREQCTRL */
#define EMU_DCDCLNFREQCTRL_RCOBAND_DEFAULT           (_EMU_DCDCLNFREQCTRL_RCOBAND_DEFAULT << 0)  /**< Shifted mode DEFAULT for EMU_DCDCLNFREQCTRL */
#define _EMU_DCDCLNFREQCTRL_RCOTRIM_SHIFT            24                                          /**< Shift value for EMU_RCOTRIM */
#define _EMU_DCDCLNFREQCTRL_RCOTRIM_MASK             0x1F000000UL                                /**< Bit mask for EMU_RCOTRIM */
#define _EMU_DCDCLNFREQCTRL_RCOTRIM_DEFAULT          0x00000010UL                                /**< Mode DEFAULT for EMU_DCDCLNFREQCTRL */
#define EMU_DCDCLNFREQCTRL_RCOTRIM_DEFAULT           (_EMU_DCDCLNFREQCTRL_RCOTRIM_DEFAULT << 24) /**< Shifted mode DEFAULT for EMU_DCDCLNFREQCTRL */

/* Bit fields for EMU DCDCSYNC */
#define _EMU_DCDCSYNC_RESETVALUE                     0x00000000UL                              /**< Default value for EMU_DCDCSYNC */
#define _EMU_DCDCSYNC_MASK                           0x00000001UL                              /**< Mask for EMU_DCDCSYNC */
#define EMU_DCDCSYNC_DCDCCTRLBUSY                    (0x1UL << 0)                              /**< DCDC CTRL Register Transfer Busy. */
#define _EMU_DCDCSYNC_DCDCCTRLBUSY_SHIFT             0                                         /**< Shift value for EMU_DCDCCTRLBUSY */
#define _EMU_DCDCSYNC_DCDCCTRLBUSY_MASK              0x1UL                                     /**< Bit mask for EMU_DCDCCTRLBUSY */
#define _EMU_DCDCSYNC_DCDCCTRLBUSY_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for EMU_DCDCSYNC */
#define EMU_DCDCSYNC_DCDCCTRLBUSY_DEFAULT            (_EMU_DCDCSYNC_DCDCCTRLBUSY_DEFAULT << 0) /**< Shifted mode DEFAULT for EMU_DCDCSYNC */

/* Bit fields for EMU VMONAVDDCTRL */
#define _EMU_VMONAVDDCTRL_RESETVALUE                 0x00000000UL                                      /**< Default value for EMU_VMONAVDDCTRL */
#define _EMU_VMONAVDDCTRL_MASK                       0x00FFFF0DUL                                      /**< Mask for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_EN                          (0x1UL << 0)                                      /**< Enable */
#define _EMU_VMONAVDDCTRL_EN_SHIFT                   0                                                 /**< Shift value for EMU_EN */
#define _EMU_VMONAVDDCTRL_EN_MASK                    0x1UL                                             /**< Bit mask for EMU_EN */
#define _EMU_VMONAVDDCTRL_EN_DEFAULT                 0x00000000UL                                      /**< Mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_EN_DEFAULT                  (_EMU_VMONAVDDCTRL_EN_DEFAULT << 0)               /**< Shifted mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_RISEWU                      (0x1UL << 2)                                      /**< Rise Wakeup */
#define _EMU_VMONAVDDCTRL_RISEWU_SHIFT               2                                                 /**< Shift value for EMU_RISEWU */
#define _EMU_VMONAVDDCTRL_RISEWU_MASK                0x4UL                                             /**< Bit mask for EMU_RISEWU */
#define _EMU_VMONAVDDCTRL_RISEWU_DEFAULT             0x00000000UL                                      /**< Mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_RISEWU_DEFAULT              (_EMU_VMONAVDDCTRL_RISEWU_DEFAULT << 2)           /**< Shifted mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_FALLWU                      (0x1UL << 3)                                      /**< Fall Wakeup */
#define _EMU_VMONAVDDCTRL_FALLWU_SHIFT               3                                                 /**< Shift value for EMU_FALLWU */
#define _EMU_VMONAVDDCTRL_FALLWU_MASK                0x8UL                                             /**< Bit mask for EMU_FALLWU */
#define _EMU_VMONAVDDCTRL_FALLWU_DEFAULT             0x00000000UL                                      /**< Mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_FALLWU_DEFAULT              (_EMU_VMONAVDDCTRL_FALLWU_DEFAULT << 3)           /**< Shifted mode DEFAULT for EMU_VMONAVDDCTRL */
#define _EMU_VMONAVDDCTRL_FALLTHRESFINE_SHIFT        8                                                 /**< Shift value for EMU_FALLTHRESFINE */
#define _EMU_VMONAVDDCTRL_FALLTHRESFINE_MASK         0xF00UL                                           /**< Bit mask for EMU_FALLTHRESFINE */
#define _EMU_VMONAVDDCTRL_FALLTHRESFINE_DEFAULT      0x00000000UL                                      /**< Mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_FALLTHRESFINE_DEFAULT       (_EMU_VMONAVDDCTRL_FALLTHRESFINE_DEFAULT << 8)    /**< Shifted mode DEFAULT for EMU_VMONAVDDCTRL */
#define _EMU_VMONAVDDCTRL_FALLTHRESCOARSE_SHIFT      12                                                /**< Shift value for EMU_FALLTHRESCOARSE */
#define _EMU_VMONAVDDCTRL_FALLTHRESCOARSE_MASK       0xF000UL                                          /**< Bit mask for EMU_FALLTHRESCOARSE */
#define _EMU_VMONAVDDCTRL_FALLTHRESCOARSE_DEFAULT    0x00000000UL                                      /**< Mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_FALLTHRESCOARSE_DEFAULT     (_EMU_VMONAVDDCTRL_FALLTHRESCOARSE_DEFAULT << 12) /**< Shifted mode DEFAULT for EMU_VMONAVDDCTRL */
#define _EMU_VMONAVDDCTRL_RISETHRESFINE_SHIFT        16                                                /**< Shift value for EMU_RISETHRESFINE */
#define _EMU_VMONAVDDCTRL_RISETHRESFINE_MASK         0xF0000UL                                         /**< Bit mask for EMU_RISETHRESFINE */
#define _EMU_VMONAVDDCTRL_RISETHRESFINE_DEFAULT      0x00000000UL                                      /**< Mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_RISETHRESFINE_DEFAULT       (_EMU_VMONAVDDCTRL_RISETHRESFINE_DEFAULT << 16)   /**< Shifted mode DEFAULT for EMU_VMONAVDDCTRL */
#define _EMU_VMONAVDDCTRL_RISETHRESCOARSE_SHIFT      20                                                /**< Shift value for EMU_RISETHRESCOARSE */
#define _EMU_VMONAVDDCTRL_RISETHRESCOARSE_MASK       0xF00000UL                                        /**< Bit mask for EMU_RISETHRESCOARSE */
#define _EMU_VMONAVDDCTRL_RISETHRESCOARSE_DEFAULT    0x00000000UL                                      /**< Mode DEFAULT for EMU_VMONAVDDCTRL */
#define EMU_VMONAVDDCTRL_RISETHRESCOARSE_DEFAULT     (_EMU_VMONAVDDCTRL_RISETHRESCOARSE_DEFAULT << 20) /**< Shifted mode DEFAULT for EMU_VMONAVDDCTRL */

/* Bit fields for EMU VMONALTAVDDCTRL */
#define _EMU_VMONALTAVDDCTRL_RESETVALUE              0x00000000UL                                     /**< Default value for EMU_VMONALTAVDDCTRL */
#define _EMU_VMONALTAVDDCTRL_MASK                    0x0000FF0DUL                                     /**< Mask for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_EN                       (0x1UL << 0)                                     /**< Enable */
#define _EMU_VMONALTAVDDCTRL_EN_SHIFT                0                                                /**< Shift value for EMU_EN */
#define _EMU_VMONALTAVDDCTRL_EN_MASK                 0x1UL                                            /**< Bit mask for EMU_EN */
#define _EMU_VMONALTAVDDCTRL_EN_DEFAULT              0x00000000UL                                     /**< Mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_EN_DEFAULT               (_EMU_VMONALTAVDDCTRL_EN_DEFAULT << 0)           /**< Shifted mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_RISEWU                   (0x1UL << 2)                                     /**< Rise Wakeup */
#define _EMU_VMONALTAVDDCTRL_RISEWU_SHIFT            2                                                /**< Shift value for EMU_RISEWU */
#define _EMU_VMONALTAVDDCTRL_RISEWU_MASK             0x4UL                                            /**< Bit mask for EMU_RISEWU */
#define _EMU_VMONALTAVDDCTRL_RISEWU_DEFAULT          0x00000000UL                                     /**< Mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_RISEWU_DEFAULT           (_EMU_VMONALTAVDDCTRL_RISEWU_DEFAULT << 2)       /**< Shifted mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_FALLWU                   (0x1UL << 3)                                     /**< Fall Wakeup */
#define _EMU_VMONALTAVDDCTRL_FALLWU_SHIFT            3                                                /**< Shift value for EMU_FALLWU */
#define _EMU_VMONALTAVDDCTRL_FALLWU_MASK             0x8UL                                            /**< Bit mask for EMU_FALLWU */
#define _EMU_VMONALTAVDDCTRL_FALLWU_DEFAULT          0x00000000UL                                     /**< Mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_FALLWU_DEFAULT           (_EMU_VMONALTAVDDCTRL_FALLWU_DEFAULT << 3)       /**< Shifted mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define _EMU_VMONALTAVDDCTRL_THRESFINE_SHIFT         8                                                /**< Shift value for EMU_THRESFINE */
#define _EMU_VMONALTAVDDCTRL_THRESFINE_MASK          0xF00UL                                          /**< Bit mask for EMU_THRESFINE */
#define _EMU_VMONALTAVDDCTRL_THRESFINE_DEFAULT       0x00000000UL                                     /**< Mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_THRESFINE_DEFAULT        (_EMU_VMONALTAVDDCTRL_THRESFINE_DEFAULT << 8)    /**< Shifted mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define _EMU_VMONALTAVDDCTRL_THRESCOARSE_SHIFT       12                                               /**< Shift value for EMU_THRESCOARSE */
#define _EMU_VMONALTAVDDCTRL_THRESCOARSE_MASK        0xF000UL                                         /**< Bit mask for EMU_THRESCOARSE */
#define _EMU_VMONALTAVDDCTRL_THRESCOARSE_DEFAULT     0x00000000UL                                     /**< Mode DEFAULT for EMU_VMONALTAVDDCTRL */
#define EMU_VMONALTAVDDCTRL_THRESCOARSE_DEFAULT      (_EMU_VMONALTAVDDCTRL_THRESCOARSE_DEFAULT << 12) /**< Shifted mode DEFAULT for EMU_VMONALTAVDDCTRL */

/* Bit fields for EMU VMONDVDDCTRL */
#define _EMU_VMONDVDDCTRL_RESETVALUE                 0x00000000UL                                  /**< Default value for EMU_VMONDVDDCTRL */
#define _EMU_VMONDVDDCTRL_MASK                       0x0000FF0DUL                                  /**< Mask for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_EN                          (0x1UL << 0)                                  /**< Enable */
#define _EMU_VMONDVDDCTRL_EN_SHIFT                   0                                             /**< Shift value for EMU_EN */
#define _EMU_VMONDVDDCTRL_EN_MASK                    0x1UL                                         /**< Bit mask for EMU_EN */
#define _EMU_VMONDVDDCTRL_EN_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_EN_DEFAULT                  (_EMU_VMONDVDDCTRL_EN_DEFAULT << 0)           /**< Shifted mode DEFAULT for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_RISEWU                      (0x1UL << 2)                                  /**< Rise Wakeup */
#define _EMU_VMONDVDDCTRL_RISEWU_SHIFT               2                                             /**< Shift value for EMU_RISEWU */
#define _EMU_VMONDVDDCTRL_RISEWU_MASK                0x4UL                                         /**< Bit mask for EMU_RISEWU */
#define _EMU_VMONDVDDCTRL_RISEWU_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_RISEWU_DEFAULT              (_EMU_VMONDVDDCTRL_RISEWU_DEFAULT << 2)       /**< Shifted mode DEFAULT for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_FALLWU                      (0x1UL << 3)                                  /**< Fall Wakeup */
#define _EMU_VMONDVDDCTRL_FALLWU_SHIFT               3                                             /**< Shift value for EMU_FALLWU */
#define _EMU_VMONDVDDCTRL_FALLWU_MASK                0x8UL                                         /**< Bit mask for EMU_FALLWU */
#define _EMU_VMONDVDDCTRL_FALLWU_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_FALLWU_DEFAULT              (_EMU_VMONDVDDCTRL_FALLWU_DEFAULT << 3)       /**< Shifted mode DEFAULT for EMU_VMONDVDDCTRL */
#define _EMU_VMONDVDDCTRL_THRESFINE_SHIFT            8                                             /**< Shift value for EMU_THRESFINE */
#define _EMU_VMONDVDDCTRL_THRESFINE_MASK             0xF00UL                                       /**< Bit mask for EMU_THRESFINE */
#define _EMU_VMONDVDDCTRL_THRESFINE_DEFAULT          0x00000000UL                                  /**< Mode DEFAULT for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_THRESFINE_DEFAULT           (_EMU_VMONDVDDCTRL_THRESFINE_DEFAULT << 8)    /**< Shifted mode DEFAULT for EMU_VMONDVDDCTRL */
#define _EMU_VMONDVDDCTRL_THRESCOARSE_SHIFT          12                                            /**< Shift value for EMU_THRESCOARSE */
#define _EMU_VMONDVDDCTRL_THRESCOARSE_MASK           0xF000UL                                      /**< Bit mask for EMU_THRESCOARSE */
#define _EMU_VMONDVDDCTRL_THRESCOARSE_DEFAULT        0x00000000UL                                  /**< Mode DEFAULT for EMU_VMONDVDDCTRL */
#define EMU_VMONDVDDCTRL_THRESCOARSE_DEFAULT         (_EMU_VMONDVDDCTRL_THRESCOARSE_DEFAULT << 12) /**< Shifted mode DEFAULT for EMU_VMONDVDDCTRL */

/* Bit fields for EMU VMONIO0CTRL */
#define _EMU_VMONIO0CTRL_RESETVALUE                  0x00000000UL                                 /**< Default value for EMU_VMONIO0CTRL */
#define _EMU_VMONIO0CTRL_MASK                        0x0000FF1DUL                                 /**< Mask for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_EN                           (0x1UL << 0)                                 /**< Enable */
#define _EMU_VMONIO0CTRL_EN_SHIFT                    0                                            /**< Shift value for EMU_EN */
#define _EMU_VMONIO0CTRL_EN_MASK                     0x1UL                                        /**< Bit mask for EMU_EN */
#define _EMU_VMONIO0CTRL_EN_DEFAULT                  0x00000000UL                                 /**< Mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_EN_DEFAULT                   (_EMU_VMONIO0CTRL_EN_DEFAULT << 0)           /**< Shifted mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_RISEWU                       (0x1UL << 2)                                 /**< Rise Wakeup */
#define _EMU_VMONIO0CTRL_RISEWU_SHIFT                2                                            /**< Shift value for EMU_RISEWU */
#define _EMU_VMONIO0CTRL_RISEWU_MASK                 0x4UL                                        /**< Bit mask for EMU_RISEWU */
#define _EMU_VMONIO0CTRL_RISEWU_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_RISEWU_DEFAULT               (_EMU_VMONIO0CTRL_RISEWU_DEFAULT << 2)       /**< Shifted mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_FALLWU                       (0x1UL << 3)                                 /**< Fall Wakeup */
#define _EMU_VMONIO0CTRL_FALLWU_SHIFT                3                                            /**< Shift value for EMU_FALLWU */
#define _EMU_VMONIO0CTRL_FALLWU_MASK                 0x8UL                                        /**< Bit mask for EMU_FALLWU */
#define _EMU_VMONIO0CTRL_FALLWU_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_FALLWU_DEFAULT               (_EMU_VMONIO0CTRL_FALLWU_DEFAULT << 3)       /**< Shifted mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_RETDIS                       (0x1UL << 4)                                 /**< EM4 IO0 Retention disable */
#define _EMU_VMONIO0CTRL_RETDIS_SHIFT                4                                            /**< Shift value for EMU_RETDIS */
#define _EMU_VMONIO0CTRL_RETDIS_MASK                 0x10UL                                       /**< Bit mask for EMU_RETDIS */
#define _EMU_VMONIO0CTRL_RETDIS_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_RETDIS_DEFAULT               (_EMU_VMONIO0CTRL_RETDIS_DEFAULT << 4)       /**< Shifted mode DEFAULT for EMU_VMONIO0CTRL */
#define _EMU_VMONIO0CTRL_THRESFINE_SHIFT             8                                            /**< Shift value for EMU_THRESFINE */
#define _EMU_VMONIO0CTRL_THRESFINE_MASK              0xF00UL                                      /**< Bit mask for EMU_THRESFINE */
#define _EMU_VMONIO0CTRL_THRESFINE_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_THRESFINE_DEFAULT            (_EMU_VMONIO0CTRL_THRESFINE_DEFAULT << 8)    /**< Shifted mode DEFAULT for EMU_VMONIO0CTRL */
#define _EMU_VMONIO0CTRL_THRESCOARSE_SHIFT           12                                           /**< Shift value for EMU_THRESCOARSE */
#define _EMU_VMONIO0CTRL_THRESCOARSE_MASK            0xF000UL                                     /**< Bit mask for EMU_THRESCOARSE */
#define _EMU_VMONIO0CTRL_THRESCOARSE_DEFAULT         0x00000000UL                                 /**< Mode DEFAULT for EMU_VMONIO0CTRL */
#define EMU_VMONIO0CTRL_THRESCOARSE_DEFAULT          (_EMU_VMONIO0CTRL_THRESCOARSE_DEFAULT << 12) /**< Shifted mode DEFAULT for EMU_VMONIO0CTRL */

/* Bit fields for EMU BIASCONF */
#define _EMU_BIASCONF_RESETVALUE                     0x000000F8UL                            /**< Default value for EMU_BIASCONF */
#define _EMU_BIASCONF_MASK                           0x000000FCUL                            /**< Mask for EMU_BIASCONF */
#define EMU_BIASCONF_NADUTYEM01                      (0x1UL << 2)                            /**< NA DUTY in EM01 */
#define _EMU_BIASCONF_NADUTYEM01_SHIFT               2                                       /**< Shift value for EMU_NADUTYEM01 */
#define _EMU_BIASCONF_NADUTYEM01_MASK                0x4UL                                   /**< Bit mask for EMU_NADUTYEM01 */
#define _EMU_BIASCONF_NADUTYEM01_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_NADUTYEM01_DEFAULT              (_EMU_BIASCONF_NADUTYEM01_DEFAULT << 2) /**< Shifted mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_LPEM01                          (0x1UL << 3)                            /**< LP in EM01 */
#define _EMU_BIASCONF_LPEM01_SHIFT                   3                                       /**< Shift value for EMU_LPEM01 */
#define _EMU_BIASCONF_LPEM01_MASK                    0x8UL                                   /**< Bit mask for EMU_LPEM01 */
#define _EMU_BIASCONF_LPEM01_DEFAULT                 0x00000001UL                            /**< Mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_LPEM01_DEFAULT                  (_EMU_BIASCONF_LPEM01_DEFAULT << 3)     /**< Shifted mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_GMCEM23                         (0x1UL << 4)                            /**< GMC in EM234 */
#define _EMU_BIASCONF_GMCEM23_SHIFT                  4                                       /**< Shift value for EMU_GMCEM23 */
#define _EMU_BIASCONF_GMCEM23_MASK                   0x10UL                                  /**< Bit mask for EMU_GMCEM23 */
#define _EMU_BIASCONF_GMCEM23_DEFAULT                0x00000001UL                            /**< Mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_GMCEM23_DEFAULT                 (_EMU_BIASCONF_GMCEM23_DEFAULT << 4)    /**< Shifted mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_UADUTYEM23                      (0x1UL << 5)                            /**< UADUTY in EM234 */
#define _EMU_BIASCONF_UADUTYEM23_SHIFT               5                                       /**< Shift value for EMU_UADUTYEM23 */
#define _EMU_BIASCONF_UADUTYEM23_MASK                0x20UL                                  /**< Bit mask for EMU_UADUTYEM23 */
#define _EMU_BIASCONF_UADUTYEM23_DEFAULT             0x00000001UL                            /**< Mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_UADUTYEM23_DEFAULT              (_EMU_BIASCONF_UADUTYEM23_DEFAULT << 5) /**< Shifted mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_NADUTYEM23                      (0x1UL << 6)                            /**< NA DUTY in EM234 */
#define _EMU_BIASCONF_NADUTYEM23_SHIFT               6                                       /**< Shift value for EMU_NADUTYEM23 */
#define _EMU_BIASCONF_NADUTYEM23_MASK                0x40UL                                  /**< Bit mask for EMU_NADUTYEM23 */
#define _EMU_BIASCONF_NADUTYEM23_DEFAULT             0x00000001UL                            /**< Mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_NADUTYEM23_DEFAULT              (_EMU_BIASCONF_NADUTYEM23_DEFAULT << 6) /**< Shifted mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_LPEM23                          (0x1UL << 7)                            /**< LP in EM234 */
#define _EMU_BIASCONF_LPEM23_SHIFT                   7                                       /**< Shift value for EMU_LPEM23 */
#define _EMU_BIASCONF_LPEM23_MASK                    0x80UL                                  /**< Bit mask for EMU_LPEM23 */
#define _EMU_BIASCONF_LPEM23_DEFAULT                 0x00000001UL                            /**< Mode DEFAULT for EMU_BIASCONF */
#define EMU_BIASCONF_LPEM23_DEFAULT                  (_EMU_BIASCONF_LPEM23_DEFAULT << 7)     /**< Shifted mode DEFAULT for EMU_BIASCONF */

/* Bit fields for EMU TESTLOCK */
#define _EMU_TESTLOCK_RESETVALUE                     0x00000000UL                          /**< Default value for EMU_TESTLOCK */
#define _EMU_TESTLOCK_MASK                           0x0000FFFFUL                          /**< Mask for EMU_TESTLOCK */
#define _EMU_TESTLOCK_LOCKKEY_SHIFT                  0                                     /**< Shift value for EMU_LOCKKEY */
#define _EMU_TESTLOCK_LOCKKEY_MASK                   0xFFFFUL                              /**< Bit mask for EMU_LOCKKEY */
#define _EMU_TESTLOCK_LOCKKEY_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for EMU_TESTLOCK */
#define _EMU_TESTLOCK_LOCKKEY_LOCK                   0x00000000UL                          /**< Mode LOCK for EMU_TESTLOCK */
#define _EMU_TESTLOCK_LOCKKEY_UNLOCKED               0x00000000UL                          /**< Mode UNLOCKED for EMU_TESTLOCK */
#define _EMU_TESTLOCK_LOCKKEY_LOCKED                 0x00000001UL                          /**< Mode LOCKED for EMU_TESTLOCK */
#define _EMU_TESTLOCK_LOCKKEY_UNLOCK                 0x0000ADE8UL                          /**< Mode UNLOCK for EMU_TESTLOCK */
#define EMU_TESTLOCK_LOCKKEY_DEFAULT                 (_EMU_TESTLOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for EMU_TESTLOCK */
#define EMU_TESTLOCK_LOCKKEY_LOCK                    (_EMU_TESTLOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for EMU_TESTLOCK */
#define EMU_TESTLOCK_LOCKKEY_UNLOCKED                (_EMU_TESTLOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for EMU_TESTLOCK */
#define EMU_TESTLOCK_LOCKKEY_LOCKED                  (_EMU_TESTLOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for EMU_TESTLOCK */
#define EMU_TESTLOCK_LOCKKEY_UNLOCK                  (_EMU_TESTLOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for EMU_TESTLOCK */

/* Bit fields for EMU BIASTESTCTRL */
#define _EMU_BIASTESTCTRL_RESETVALUE                 0x00000000UL                                    /**< Default value for EMU_BIASTESTCTRL */
#define _EMU_BIASTESTCTRL_MASK                       0x00000008UL                                    /**< Mask for EMU_BIASTESTCTRL */
#define EMU_BIASTESTCTRL_BIAS_RIP_RESET              (0x1UL << 3)                                    /**< Reset Bias Ripple Counter */
#define _EMU_BIASTESTCTRL_BIAS_RIP_RESET_SHIFT       3                                               /**< Shift value for EMU_BIAS_RIP_RESET */
#define _EMU_BIASTESTCTRL_BIAS_RIP_RESET_MASK        0x8UL                                           /**< Bit mask for EMU_BIAS_RIP_RESET */
#define _EMU_BIASTESTCTRL_BIAS_RIP_RESET_DEFAULT     0x00000000UL                                    /**< Mode DEFAULT for EMU_BIASTESTCTRL */
#define EMU_BIASTESTCTRL_BIAS_RIP_RESET_DEFAULT      (_EMU_BIASTESTCTRL_BIAS_RIP_RESET_DEFAULT << 3) /**< Shifted mode DEFAULT for EMU_BIASTESTCTRL */

/** @} End of group EFR32FG1P_EMU */
/** @} End of group Parts */

