/**************************************************************************//**
 * @file efr32fg1p_leuart.h
 * @brief EFR32FG1P_LEUART register and bit field definitions
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
 * @defgroup EFR32FG1P_LEUART
 * @{
 * @brief EFR32FG1P_LEUART Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< Control Register  */
  __IOM uint32_t CMD;          /**< Command Register  */
  __IM uint32_t  STATUS;       /**< Status Register  */
  __IOM uint32_t CLKDIV;       /**< Clock Control Register  */
  __IOM uint32_t STARTFRAME;   /**< Start Frame Register  */
  __IOM uint32_t SIGFRAME;     /**< Signal Frame Register  */
  __IM uint32_t  RXDATAX;      /**< Receive Buffer Data Extended Register  */
  __IM uint32_t  RXDATA;       /**< Receive Buffer Data Register  */
  __IM uint32_t  RXDATAXP;     /**< Receive Buffer Data Extended Peek Register  */
  __IOM uint32_t TXDATAX;      /**< Transmit Buffer Data Extended Register  */
  __IOM uint32_t TXDATA;       /**< Transmit Buffer Data Register  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */
  __IOM uint32_t PULSECTRL;    /**< Pulse Control Register  */

  __IOM uint32_t FREEZE;       /**< Freeze Register  */
  __IM uint32_t  SYNCBUSY;     /**< Synchronization Busy Register  */

  uint32_t       RESERVED0[3]; /**< Reserved for future use **/
  __IOM uint32_t ROUTEPEN;     /**< I/O Routing Pin Enable Register  */
  __IOM uint32_t ROUTELOC0;    /**< I/O Routing Location Register  */
  uint32_t       RESERVED1[2]; /**< Reserved for future use **/
  __IOM uint32_t INPUT;        /**< LEUART Input Register  */
} LEUART_TypeDef;              /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_LEUART_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for LEUART CTRL */
#define _LEUART_CTRL_RESETVALUE                  0x00000000UL                         /**< Default value for LEUART_CTRL */
#define _LEUART_CTRL_MASK                        0x0000FFFFUL                         /**< Mask for LEUART_CTRL */
#define LEUART_CTRL_AUTOTRI                      (0x1UL << 0)                         /**< Automatic Transmitter Tristate */
#define _LEUART_CTRL_AUTOTRI_SHIFT               0                                    /**< Shift value for LEUART_AUTOTRI */
#define _LEUART_CTRL_AUTOTRI_MASK                0x1UL                                /**< Bit mask for LEUART_AUTOTRI */
#define _LEUART_CTRL_AUTOTRI_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_AUTOTRI_DEFAULT              (_LEUART_CTRL_AUTOTRI_DEFAULT << 0)  /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_DATABITS                     (0x1UL << 1)                         /**< Data-Bit Mode */
#define _LEUART_CTRL_DATABITS_SHIFT              1                                    /**< Shift value for LEUART_DATABITS */
#define _LEUART_CTRL_DATABITS_MASK               0x2UL                                /**< Bit mask for LEUART_DATABITS */
#define _LEUART_CTRL_DATABITS_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define _LEUART_CTRL_DATABITS_EIGHT              0x00000000UL                         /**< Mode EIGHT for LEUART_CTRL */
#define _LEUART_CTRL_DATABITS_NINE               0x00000001UL                         /**< Mode NINE for LEUART_CTRL */
#define LEUART_CTRL_DATABITS_DEFAULT             (_LEUART_CTRL_DATABITS_DEFAULT << 1) /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_DATABITS_EIGHT               (_LEUART_CTRL_DATABITS_EIGHT << 1)   /**< Shifted mode EIGHT for LEUART_CTRL */
#define LEUART_CTRL_DATABITS_NINE                (_LEUART_CTRL_DATABITS_NINE << 1)    /**< Shifted mode NINE for LEUART_CTRL */
#define _LEUART_CTRL_PARITY_SHIFT                2                                    /**< Shift value for LEUART_PARITY */
#define _LEUART_CTRL_PARITY_MASK                 0xCUL                                /**< Bit mask for LEUART_PARITY */
#define _LEUART_CTRL_PARITY_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define _LEUART_CTRL_PARITY_NONE                 0x00000000UL                         /**< Mode NONE for LEUART_CTRL */
#define _LEUART_CTRL_PARITY_EVEN                 0x00000002UL                         /**< Mode EVEN for LEUART_CTRL */
#define _LEUART_CTRL_PARITY_ODD                  0x00000003UL                         /**< Mode ODD for LEUART_CTRL */
#define LEUART_CTRL_PARITY_DEFAULT               (_LEUART_CTRL_PARITY_DEFAULT << 2)   /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_PARITY_NONE                  (_LEUART_CTRL_PARITY_NONE << 2)      /**< Shifted mode NONE for LEUART_CTRL */
#define LEUART_CTRL_PARITY_EVEN                  (_LEUART_CTRL_PARITY_EVEN << 2)      /**< Shifted mode EVEN for LEUART_CTRL */
#define LEUART_CTRL_PARITY_ODD                   (_LEUART_CTRL_PARITY_ODD << 2)       /**< Shifted mode ODD for LEUART_CTRL */
#define LEUART_CTRL_STOPBITS                     (0x1UL << 4)                         /**< Stop-Bit Mode */
#define _LEUART_CTRL_STOPBITS_SHIFT              4                                    /**< Shift value for LEUART_STOPBITS */
#define _LEUART_CTRL_STOPBITS_MASK               0x10UL                               /**< Bit mask for LEUART_STOPBITS */
#define _LEUART_CTRL_STOPBITS_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define _LEUART_CTRL_STOPBITS_ONE                0x00000000UL                         /**< Mode ONE for LEUART_CTRL */
#define _LEUART_CTRL_STOPBITS_TWO                0x00000001UL                         /**< Mode TWO for LEUART_CTRL */
#define LEUART_CTRL_STOPBITS_DEFAULT             (_LEUART_CTRL_STOPBITS_DEFAULT << 4) /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_STOPBITS_ONE                 (_LEUART_CTRL_STOPBITS_ONE << 4)     /**< Shifted mode ONE for LEUART_CTRL */
#define LEUART_CTRL_STOPBITS_TWO                 (_LEUART_CTRL_STOPBITS_TWO << 4)     /**< Shifted mode TWO for LEUART_CTRL */
#define LEUART_CTRL_INV                          (0x1UL << 5)                         /**< Invert Input And Output */
#define _LEUART_CTRL_INV_SHIFT                   5                                    /**< Shift value for LEUART_INV */
#define _LEUART_CTRL_INV_MASK                    0x20UL                               /**< Bit mask for LEUART_INV */
#define _LEUART_CTRL_INV_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_INV_DEFAULT                  (_LEUART_CTRL_INV_DEFAULT << 5)      /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_ERRSDMA                      (0x1UL << 6)                         /**< Clear RX DMA On Error */
#define _LEUART_CTRL_ERRSDMA_SHIFT               6                                    /**< Shift value for LEUART_ERRSDMA */
#define _LEUART_CTRL_ERRSDMA_MASK                0x40UL                               /**< Bit mask for LEUART_ERRSDMA */
#define _LEUART_CTRL_ERRSDMA_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_ERRSDMA_DEFAULT              (_LEUART_CTRL_ERRSDMA_DEFAULT << 6)  /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_LOOPBK                       (0x1UL << 7)                         /**< Loopback Enable */
#define _LEUART_CTRL_LOOPBK_SHIFT                7                                    /**< Shift value for LEUART_LOOPBK */
#define _LEUART_CTRL_LOOPBK_MASK                 0x80UL                               /**< Bit mask for LEUART_LOOPBK */
#define _LEUART_CTRL_LOOPBK_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_LOOPBK_DEFAULT               (_LEUART_CTRL_LOOPBK_DEFAULT << 7)   /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_SFUBRX                       (0x1UL << 8)                         /**< Start-Frame UnBlock RX */
#define _LEUART_CTRL_SFUBRX_SHIFT                8                                    /**< Shift value for LEUART_SFUBRX */
#define _LEUART_CTRL_SFUBRX_MASK                 0x100UL                              /**< Bit mask for LEUART_SFUBRX */
#define _LEUART_CTRL_SFUBRX_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_SFUBRX_DEFAULT               (_LEUART_CTRL_SFUBRX_DEFAULT << 8)   /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_MPM                          (0x1UL << 9)                         /**< Multi-Processor Mode */
#define _LEUART_CTRL_MPM_SHIFT                   9                                    /**< Shift value for LEUART_MPM */
#define _LEUART_CTRL_MPM_MASK                    0x200UL                              /**< Bit mask for LEUART_MPM */
#define _LEUART_CTRL_MPM_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_MPM_DEFAULT                  (_LEUART_CTRL_MPM_DEFAULT << 9)      /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_MPAB                         (0x1UL << 10)                        /**< Multi-Processor Address-Bit */
#define _LEUART_CTRL_MPAB_SHIFT                  10                                   /**< Shift value for LEUART_MPAB */
#define _LEUART_CTRL_MPAB_MASK                   0x400UL                              /**< Bit mask for LEUART_MPAB */
#define _LEUART_CTRL_MPAB_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_MPAB_DEFAULT                 (_LEUART_CTRL_MPAB_DEFAULT << 10)    /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_BIT8DV                       (0x1UL << 11)                        /**< Bit 8 Default Value */
#define _LEUART_CTRL_BIT8DV_SHIFT                11                                   /**< Shift value for LEUART_BIT8DV */
#define _LEUART_CTRL_BIT8DV_MASK                 0x800UL                              /**< Bit mask for LEUART_BIT8DV */
#define _LEUART_CTRL_BIT8DV_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_BIT8DV_DEFAULT               (_LEUART_CTRL_BIT8DV_DEFAULT << 11)  /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_RXDMAWU                      (0x1UL << 12)                        /**< RX DMA Wakeup */
#define _LEUART_CTRL_RXDMAWU_SHIFT               12                                   /**< Shift value for LEUART_RXDMAWU */
#define _LEUART_CTRL_RXDMAWU_MASK                0x1000UL                             /**< Bit mask for LEUART_RXDMAWU */
#define _LEUART_CTRL_RXDMAWU_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_RXDMAWU_DEFAULT              (_LEUART_CTRL_RXDMAWU_DEFAULT << 12) /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_TXDMAWU                      (0x1UL << 13)                        /**< TX DMA Wakeup */
#define _LEUART_CTRL_TXDMAWU_SHIFT               13                                   /**< Shift value for LEUART_TXDMAWU */
#define _LEUART_CTRL_TXDMAWU_MASK                0x2000UL                             /**< Bit mask for LEUART_TXDMAWU */
#define _LEUART_CTRL_TXDMAWU_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_TXDMAWU_DEFAULT              (_LEUART_CTRL_TXDMAWU_DEFAULT << 13) /**< Shifted mode DEFAULT for LEUART_CTRL */
#define _LEUART_CTRL_TXDELAY_SHIFT               14                                   /**< Shift value for LEUART_TXDELAY */
#define _LEUART_CTRL_TXDELAY_MASK                0xC000UL                             /**< Bit mask for LEUART_TXDELAY */
#define _LEUART_CTRL_TXDELAY_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for LEUART_CTRL */
#define _LEUART_CTRL_TXDELAY_NONE                0x00000000UL                         /**< Mode NONE for LEUART_CTRL */
#define _LEUART_CTRL_TXDELAY_SINGLE              0x00000001UL                         /**< Mode SINGLE for LEUART_CTRL */
#define _LEUART_CTRL_TXDELAY_DOUBLE              0x00000002UL                         /**< Mode DOUBLE for LEUART_CTRL */
#define _LEUART_CTRL_TXDELAY_TRIPLE              0x00000003UL                         /**< Mode TRIPLE for LEUART_CTRL */
#define LEUART_CTRL_TXDELAY_DEFAULT              (_LEUART_CTRL_TXDELAY_DEFAULT << 14) /**< Shifted mode DEFAULT for LEUART_CTRL */
#define LEUART_CTRL_TXDELAY_NONE                 (_LEUART_CTRL_TXDELAY_NONE << 14)    /**< Shifted mode NONE for LEUART_CTRL */
#define LEUART_CTRL_TXDELAY_SINGLE               (_LEUART_CTRL_TXDELAY_SINGLE << 14)  /**< Shifted mode SINGLE for LEUART_CTRL */
#define LEUART_CTRL_TXDELAY_DOUBLE               (_LEUART_CTRL_TXDELAY_DOUBLE << 14)  /**< Shifted mode DOUBLE for LEUART_CTRL */
#define LEUART_CTRL_TXDELAY_TRIPLE               (_LEUART_CTRL_TXDELAY_TRIPLE << 14)  /**< Shifted mode TRIPLE for LEUART_CTRL */

/* Bit fields for LEUART CMD */
#define _LEUART_CMD_RESETVALUE                   0x00000000UL                          /**< Default value for LEUART_CMD */
#define _LEUART_CMD_MASK                         0x000000FFUL                          /**< Mask for LEUART_CMD */
#define LEUART_CMD_RXEN                          (0x1UL << 0)                          /**< Receiver Enable */
#define _LEUART_CMD_RXEN_SHIFT                   0                                     /**< Shift value for LEUART_RXEN */
#define _LEUART_CMD_RXEN_MASK                    0x1UL                                 /**< Bit mask for LEUART_RXEN */
#define _LEUART_CMD_RXEN_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_RXEN_DEFAULT                  (_LEUART_CMD_RXEN_DEFAULT << 0)       /**< Shifted mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_RXDIS                         (0x1UL << 1)                          /**< Receiver Disable */
#define _LEUART_CMD_RXDIS_SHIFT                  1                                     /**< Shift value for LEUART_RXDIS */
#define _LEUART_CMD_RXDIS_MASK                   0x2UL                                 /**< Bit mask for LEUART_RXDIS */
#define _LEUART_CMD_RXDIS_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_RXDIS_DEFAULT                 (_LEUART_CMD_RXDIS_DEFAULT << 1)      /**< Shifted mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_TXEN                          (0x1UL << 2)                          /**< Transmitter Enable */
#define _LEUART_CMD_TXEN_SHIFT                   2                                     /**< Shift value for LEUART_TXEN */
#define _LEUART_CMD_TXEN_MASK                    0x4UL                                 /**< Bit mask for LEUART_TXEN */
#define _LEUART_CMD_TXEN_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_TXEN_DEFAULT                  (_LEUART_CMD_TXEN_DEFAULT << 2)       /**< Shifted mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_TXDIS                         (0x1UL << 3)                          /**< Transmitter Disable */
#define _LEUART_CMD_TXDIS_SHIFT                  3                                     /**< Shift value for LEUART_TXDIS */
#define _LEUART_CMD_TXDIS_MASK                   0x8UL                                 /**< Bit mask for LEUART_TXDIS */
#define _LEUART_CMD_TXDIS_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_TXDIS_DEFAULT                 (_LEUART_CMD_TXDIS_DEFAULT << 3)      /**< Shifted mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_RXBLOCKEN                     (0x1UL << 4)                          /**< Receiver Block Enable */
#define _LEUART_CMD_RXBLOCKEN_SHIFT              4                                     /**< Shift value for LEUART_RXBLOCKEN */
#define _LEUART_CMD_RXBLOCKEN_MASK               0x10UL                                /**< Bit mask for LEUART_RXBLOCKEN */
#define _LEUART_CMD_RXBLOCKEN_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_RXBLOCKEN_DEFAULT             (_LEUART_CMD_RXBLOCKEN_DEFAULT << 4)  /**< Shifted mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_RXBLOCKDIS                    (0x1UL << 5)                          /**< Receiver Block Disable */
#define _LEUART_CMD_RXBLOCKDIS_SHIFT             5                                     /**< Shift value for LEUART_RXBLOCKDIS */
#define _LEUART_CMD_RXBLOCKDIS_MASK              0x20UL                                /**< Bit mask for LEUART_RXBLOCKDIS */
#define _LEUART_CMD_RXBLOCKDIS_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_RXBLOCKDIS_DEFAULT            (_LEUART_CMD_RXBLOCKDIS_DEFAULT << 5) /**< Shifted mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_CLEARTX                       (0x1UL << 6)                          /**< Clear TX */
#define _LEUART_CMD_CLEARTX_SHIFT                6                                     /**< Shift value for LEUART_CLEARTX */
#define _LEUART_CMD_CLEARTX_MASK                 0x40UL                                /**< Bit mask for LEUART_CLEARTX */
#define _LEUART_CMD_CLEARTX_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_CLEARTX_DEFAULT               (_LEUART_CMD_CLEARTX_DEFAULT << 6)    /**< Shifted mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_CLEARRX                       (0x1UL << 7)                          /**< Clear RX */
#define _LEUART_CMD_CLEARRX_SHIFT                7                                     /**< Shift value for LEUART_CLEARRX */
#define _LEUART_CMD_CLEARRX_MASK                 0x80UL                                /**< Bit mask for LEUART_CLEARRX */
#define _LEUART_CMD_CLEARRX_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for LEUART_CMD */
#define LEUART_CMD_CLEARRX_DEFAULT               (_LEUART_CMD_CLEARRX_DEFAULT << 7)    /**< Shifted mode DEFAULT for LEUART_CMD */

/* Bit fields for LEUART STATUS */
#define _LEUART_STATUS_RESETVALUE                0x00000050UL                          /**< Default value for LEUART_STATUS */
#define _LEUART_STATUS_MASK                      0x0000007FUL                          /**< Mask for LEUART_STATUS */
#define LEUART_STATUS_RXENS                      (0x1UL << 0)                          /**< Receiver Enable Status */
#define _LEUART_STATUS_RXENS_SHIFT               0                                     /**< Shift value for LEUART_RXENS */
#define _LEUART_STATUS_RXENS_MASK                0x1UL                                 /**< Bit mask for LEUART_RXENS */
#define _LEUART_STATUS_RXENS_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_RXENS_DEFAULT              (_LEUART_STATUS_RXENS_DEFAULT << 0)   /**< Shifted mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXENS                      (0x1UL << 1)                          /**< Transmitter Enable Status */
#define _LEUART_STATUS_TXENS_SHIFT               1                                     /**< Shift value for LEUART_TXENS */
#define _LEUART_STATUS_TXENS_MASK                0x2UL                                 /**< Bit mask for LEUART_TXENS */
#define _LEUART_STATUS_TXENS_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXENS_DEFAULT              (_LEUART_STATUS_TXENS_DEFAULT << 1)   /**< Shifted mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_RXBLOCK                    (0x1UL << 2)                          /**< Block Incoming Data */
#define _LEUART_STATUS_RXBLOCK_SHIFT             2                                     /**< Shift value for LEUART_RXBLOCK */
#define _LEUART_STATUS_RXBLOCK_MASK              0x4UL                                 /**< Bit mask for LEUART_RXBLOCK */
#define _LEUART_STATUS_RXBLOCK_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_RXBLOCK_DEFAULT            (_LEUART_STATUS_RXBLOCK_DEFAULT << 2) /**< Shifted mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXC                        (0x1UL << 3)                          /**< TX Complete */
#define _LEUART_STATUS_TXC_SHIFT                 3                                     /**< Shift value for LEUART_TXC */
#define _LEUART_STATUS_TXC_MASK                  0x8UL                                 /**< Bit mask for LEUART_TXC */
#define _LEUART_STATUS_TXC_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXC_DEFAULT                (_LEUART_STATUS_TXC_DEFAULT << 3)     /**< Shifted mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXBL                       (0x1UL << 4)                          /**< TX Buffer Level */
#define _LEUART_STATUS_TXBL_SHIFT                4                                     /**< Shift value for LEUART_TXBL */
#define _LEUART_STATUS_TXBL_MASK                 0x10UL                                /**< Bit mask for LEUART_TXBL */
#define _LEUART_STATUS_TXBL_DEFAULT              0x00000001UL                          /**< Mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXBL_DEFAULT               (_LEUART_STATUS_TXBL_DEFAULT << 4)    /**< Shifted mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_RXDATAV                    (0x1UL << 5)                          /**< RX Data Valid */
#define _LEUART_STATUS_RXDATAV_SHIFT             5                                     /**< Shift value for LEUART_RXDATAV */
#define _LEUART_STATUS_RXDATAV_MASK              0x20UL                                /**< Bit mask for LEUART_RXDATAV */
#define _LEUART_STATUS_RXDATAV_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_RXDATAV_DEFAULT            (_LEUART_STATUS_RXDATAV_DEFAULT << 5) /**< Shifted mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXIDLE                     (0x1UL << 6)                          /**< TX Idle */
#define _LEUART_STATUS_TXIDLE_SHIFT              6                                     /**< Shift value for LEUART_TXIDLE */
#define _LEUART_STATUS_TXIDLE_MASK               0x40UL                                /**< Bit mask for LEUART_TXIDLE */
#define _LEUART_STATUS_TXIDLE_DEFAULT            0x00000001UL                          /**< Mode DEFAULT for LEUART_STATUS */
#define LEUART_STATUS_TXIDLE_DEFAULT             (_LEUART_STATUS_TXIDLE_DEFAULT << 6)  /**< Shifted mode DEFAULT for LEUART_STATUS */

/* Bit fields for LEUART CLKDIV */
#define _LEUART_CLKDIV_RESETVALUE                0x00000000UL                      /**< Default value for LEUART_CLKDIV */
#define _LEUART_CLKDIV_MASK                      0x0001FFF8UL                      /**< Mask for LEUART_CLKDIV */
#define _LEUART_CLKDIV_DIV_SHIFT                 3                                 /**< Shift value for LEUART_DIV */
#define _LEUART_CLKDIV_DIV_MASK                  0x1FFF8UL                         /**< Bit mask for LEUART_DIV */
#define _LEUART_CLKDIV_DIV_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LEUART_CLKDIV */
#define LEUART_CLKDIV_DIV_DEFAULT                (_LEUART_CLKDIV_DIV_DEFAULT << 3) /**< Shifted mode DEFAULT for LEUART_CLKDIV */

/* Bit fields for LEUART STARTFRAME */
#define _LEUART_STARTFRAME_RESETVALUE            0x00000000UL                                 /**< Default value for LEUART_STARTFRAME */
#define _LEUART_STARTFRAME_MASK                  0x000001FFUL                                 /**< Mask for LEUART_STARTFRAME */
#define _LEUART_STARTFRAME_STARTFRAME_SHIFT      0                                            /**< Shift value for LEUART_STARTFRAME */
#define _LEUART_STARTFRAME_STARTFRAME_MASK       0x1FFUL                                      /**< Bit mask for LEUART_STARTFRAME */
#define _LEUART_STARTFRAME_STARTFRAME_DEFAULT    0x00000000UL                                 /**< Mode DEFAULT for LEUART_STARTFRAME */
#define LEUART_STARTFRAME_STARTFRAME_DEFAULT     (_LEUART_STARTFRAME_STARTFRAME_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_STARTFRAME */

/* Bit fields for LEUART SIGFRAME */
#define _LEUART_SIGFRAME_RESETVALUE              0x00000000UL                             /**< Default value for LEUART_SIGFRAME */
#define _LEUART_SIGFRAME_MASK                    0x000001FFUL                             /**< Mask for LEUART_SIGFRAME */
#define _LEUART_SIGFRAME_SIGFRAME_SHIFT          0                                        /**< Shift value for LEUART_SIGFRAME */
#define _LEUART_SIGFRAME_SIGFRAME_MASK           0x1FFUL                                  /**< Bit mask for LEUART_SIGFRAME */
#define _LEUART_SIGFRAME_SIGFRAME_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for LEUART_SIGFRAME */
#define LEUART_SIGFRAME_SIGFRAME_DEFAULT         (_LEUART_SIGFRAME_SIGFRAME_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_SIGFRAME */

/* Bit fields for LEUART RXDATAX */
#define _LEUART_RXDATAX_RESETVALUE               0x00000000UL                          /**< Default value for LEUART_RXDATAX */
#define _LEUART_RXDATAX_MASK                     0x0000C1FFUL                          /**< Mask for LEUART_RXDATAX */
#define _LEUART_RXDATAX_RXDATA_SHIFT             0                                     /**< Shift value for LEUART_RXDATA */
#define _LEUART_RXDATAX_RXDATA_MASK              0x1FFUL                               /**< Bit mask for LEUART_RXDATA */
#define _LEUART_RXDATAX_RXDATA_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LEUART_RXDATAX */
#define LEUART_RXDATAX_RXDATA_DEFAULT            (_LEUART_RXDATAX_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_RXDATAX */
#define LEUART_RXDATAX_PERR                      (0x1UL << 14)                         /**< Receive Data Parity Error */
#define _LEUART_RXDATAX_PERR_SHIFT               14                                    /**< Shift value for LEUART_PERR */
#define _LEUART_RXDATAX_PERR_MASK                0x4000UL                              /**< Bit mask for LEUART_PERR */
#define _LEUART_RXDATAX_PERR_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for LEUART_RXDATAX */
#define LEUART_RXDATAX_PERR_DEFAULT              (_LEUART_RXDATAX_PERR_DEFAULT << 14)  /**< Shifted mode DEFAULT for LEUART_RXDATAX */
#define LEUART_RXDATAX_FERR                      (0x1UL << 15)                         /**< Receive Data Framing Error */
#define _LEUART_RXDATAX_FERR_SHIFT               15                                    /**< Shift value for LEUART_FERR */
#define _LEUART_RXDATAX_FERR_MASK                0x8000UL                              /**< Bit mask for LEUART_FERR */
#define _LEUART_RXDATAX_FERR_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for LEUART_RXDATAX */
#define LEUART_RXDATAX_FERR_DEFAULT              (_LEUART_RXDATAX_FERR_DEFAULT << 15)  /**< Shifted mode DEFAULT for LEUART_RXDATAX */

/* Bit fields for LEUART RXDATA */
#define _LEUART_RXDATA_RESETVALUE                0x00000000UL                         /**< Default value for LEUART_RXDATA */
#define _LEUART_RXDATA_MASK                      0x000000FFUL                         /**< Mask for LEUART_RXDATA */
#define _LEUART_RXDATA_RXDATA_SHIFT              0                                    /**< Shift value for LEUART_RXDATA */
#define _LEUART_RXDATA_RXDATA_MASK               0xFFUL                               /**< Bit mask for LEUART_RXDATA */
#define _LEUART_RXDATA_RXDATA_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for LEUART_RXDATA */
#define LEUART_RXDATA_RXDATA_DEFAULT             (_LEUART_RXDATA_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_RXDATA */

/* Bit fields for LEUART RXDATAXP */
#define _LEUART_RXDATAXP_RESETVALUE              0x00000000UL                            /**< Default value for LEUART_RXDATAXP */
#define _LEUART_RXDATAXP_MASK                    0x0000C1FFUL                            /**< Mask for LEUART_RXDATAXP */
#define _LEUART_RXDATAXP_RXDATAP_SHIFT           0                                       /**< Shift value for LEUART_RXDATAP */
#define _LEUART_RXDATAXP_RXDATAP_MASK            0x1FFUL                                 /**< Bit mask for LEUART_RXDATAP */
#define _LEUART_RXDATAXP_RXDATAP_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for LEUART_RXDATAXP */
#define LEUART_RXDATAXP_RXDATAP_DEFAULT          (_LEUART_RXDATAXP_RXDATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_RXDATAXP */
#define LEUART_RXDATAXP_PERRP                    (0x1UL << 14)                           /**< Receive Data Parity Error Peek */
#define _LEUART_RXDATAXP_PERRP_SHIFT             14                                      /**< Shift value for LEUART_PERRP */
#define _LEUART_RXDATAXP_PERRP_MASK              0x4000UL                                /**< Bit mask for LEUART_PERRP */
#define _LEUART_RXDATAXP_PERRP_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for LEUART_RXDATAXP */
#define LEUART_RXDATAXP_PERRP_DEFAULT            (_LEUART_RXDATAXP_PERRP_DEFAULT << 14)  /**< Shifted mode DEFAULT for LEUART_RXDATAXP */
#define LEUART_RXDATAXP_FERRP                    (0x1UL << 15)                           /**< Receive Data Framing Error Peek */
#define _LEUART_RXDATAXP_FERRP_SHIFT             15                                      /**< Shift value for LEUART_FERRP */
#define _LEUART_RXDATAXP_FERRP_MASK              0x8000UL                                /**< Bit mask for LEUART_FERRP */
#define _LEUART_RXDATAXP_FERRP_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for LEUART_RXDATAXP */
#define LEUART_RXDATAXP_FERRP_DEFAULT            (_LEUART_RXDATAXP_FERRP_DEFAULT << 15)  /**< Shifted mode DEFAULT for LEUART_RXDATAXP */

/* Bit fields for LEUART TXDATAX */
#define _LEUART_TXDATAX_RESETVALUE               0x00000000UL                            /**< Default value for LEUART_TXDATAX */
#define _LEUART_TXDATAX_MASK                     0x0000E1FFUL                            /**< Mask for LEUART_TXDATAX */
#define _LEUART_TXDATAX_TXDATA_SHIFT             0                                       /**< Shift value for LEUART_TXDATA */
#define _LEUART_TXDATAX_TXDATA_MASK              0x1FFUL                                 /**< Bit mask for LEUART_TXDATA */
#define _LEUART_TXDATAX_TXDATA_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for LEUART_TXDATAX */
#define LEUART_TXDATAX_TXDATA_DEFAULT            (_LEUART_TXDATAX_TXDATA_DEFAULT << 0)   /**< Shifted mode DEFAULT for LEUART_TXDATAX */
#define LEUART_TXDATAX_TXBREAK                   (0x1UL << 13)                           /**< Transmit Data As Break */
#define _LEUART_TXDATAX_TXBREAK_SHIFT            13                                      /**< Shift value for LEUART_TXBREAK */
#define _LEUART_TXDATAX_TXBREAK_MASK             0x2000UL                                /**< Bit mask for LEUART_TXBREAK */
#define _LEUART_TXDATAX_TXBREAK_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for LEUART_TXDATAX */
#define LEUART_TXDATAX_TXBREAK_DEFAULT           (_LEUART_TXDATAX_TXBREAK_DEFAULT << 13) /**< Shifted mode DEFAULT for LEUART_TXDATAX */
#define LEUART_TXDATAX_TXDISAT                   (0x1UL << 14)                           /**< Disable TX After Transmission */
#define _LEUART_TXDATAX_TXDISAT_SHIFT            14                                      /**< Shift value for LEUART_TXDISAT */
#define _LEUART_TXDATAX_TXDISAT_MASK             0x4000UL                                /**< Bit mask for LEUART_TXDISAT */
#define _LEUART_TXDATAX_TXDISAT_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for LEUART_TXDATAX */
#define LEUART_TXDATAX_TXDISAT_DEFAULT           (_LEUART_TXDATAX_TXDISAT_DEFAULT << 14) /**< Shifted mode DEFAULT for LEUART_TXDATAX */
#define LEUART_TXDATAX_RXENAT                    (0x1UL << 15)                           /**< Enable RX After Transmission */
#define _LEUART_TXDATAX_RXENAT_SHIFT             15                                      /**< Shift value for LEUART_RXENAT */
#define _LEUART_TXDATAX_RXENAT_MASK              0x8000UL                                /**< Bit mask for LEUART_RXENAT */
#define _LEUART_TXDATAX_RXENAT_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for LEUART_TXDATAX */
#define LEUART_TXDATAX_RXENAT_DEFAULT            (_LEUART_TXDATAX_RXENAT_DEFAULT << 15)  /**< Shifted mode DEFAULT for LEUART_TXDATAX */

/* Bit fields for LEUART TXDATA */
#define _LEUART_TXDATA_RESETVALUE                0x00000000UL                         /**< Default value for LEUART_TXDATA */
#define _LEUART_TXDATA_MASK                      0x000000FFUL                         /**< Mask for LEUART_TXDATA */
#define _LEUART_TXDATA_TXDATA_SHIFT              0                                    /**< Shift value for LEUART_TXDATA */
#define _LEUART_TXDATA_TXDATA_MASK               0xFFUL                               /**< Bit mask for LEUART_TXDATA */
#define _LEUART_TXDATA_TXDATA_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for LEUART_TXDATA */
#define LEUART_TXDATA_TXDATA_DEFAULT             (_LEUART_TXDATA_TXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_TXDATA */

/* Bit fields for LEUART IF */
#define _LEUART_IF_RESETVALUE                    0x00000002UL                      /**< Default value for LEUART_IF */
#define _LEUART_IF_MASK                          0x000007FFUL                      /**< Mask for LEUART_IF */
#define LEUART_IF_TXC                            (0x1UL << 0)                      /**< TX Complete Interrupt Flag */
#define _LEUART_IF_TXC_SHIFT                     0                                 /**< Shift value for LEUART_TXC */
#define _LEUART_IF_TXC_MASK                      0x1UL                             /**< Bit mask for LEUART_TXC */
#define _LEUART_IF_TXC_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_TXC_DEFAULT                    (_LEUART_IF_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_TXBL                           (0x1UL << 1)                      /**< TX Buffer Level Interrupt Flag */
#define _LEUART_IF_TXBL_SHIFT                    1                                 /**< Shift value for LEUART_TXBL */
#define _LEUART_IF_TXBL_MASK                     0x2UL                             /**< Bit mask for LEUART_TXBL */
#define _LEUART_IF_TXBL_DEFAULT                  0x00000001UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_TXBL_DEFAULT                   (_LEUART_IF_TXBL_DEFAULT << 1)    /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_RXDATAV                        (0x1UL << 2)                      /**< RX Data Valid Interrupt Flag */
#define _LEUART_IF_RXDATAV_SHIFT                 2                                 /**< Shift value for LEUART_RXDATAV */
#define _LEUART_IF_RXDATAV_MASK                  0x4UL                             /**< Bit mask for LEUART_RXDATAV */
#define _LEUART_IF_RXDATAV_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_RXDATAV_DEFAULT                (_LEUART_IF_RXDATAV_DEFAULT << 2) /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_RXOF                           (0x1UL << 3)                      /**< RX Overflow Interrupt Flag */
#define _LEUART_IF_RXOF_SHIFT                    3                                 /**< Shift value for LEUART_RXOF */
#define _LEUART_IF_RXOF_MASK                     0x8UL                             /**< Bit mask for LEUART_RXOF */
#define _LEUART_IF_RXOF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_RXOF_DEFAULT                   (_LEUART_IF_RXOF_DEFAULT << 3)    /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_RXUF                           (0x1UL << 4)                      /**< RX Underflow Interrupt Flag */
#define _LEUART_IF_RXUF_SHIFT                    4                                 /**< Shift value for LEUART_RXUF */
#define _LEUART_IF_RXUF_MASK                     0x10UL                            /**< Bit mask for LEUART_RXUF */
#define _LEUART_IF_RXUF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_RXUF_DEFAULT                   (_LEUART_IF_RXUF_DEFAULT << 4)    /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_TXOF                           (0x1UL << 5)                      /**< TX Overflow Interrupt Flag */
#define _LEUART_IF_TXOF_SHIFT                    5                                 /**< Shift value for LEUART_TXOF */
#define _LEUART_IF_TXOF_MASK                     0x20UL                            /**< Bit mask for LEUART_TXOF */
#define _LEUART_IF_TXOF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_TXOF_DEFAULT                   (_LEUART_IF_TXOF_DEFAULT << 5)    /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_PERR                           (0x1UL << 6)                      /**< Parity Error Interrupt Flag */
#define _LEUART_IF_PERR_SHIFT                    6                                 /**< Shift value for LEUART_PERR */
#define _LEUART_IF_PERR_MASK                     0x40UL                            /**< Bit mask for LEUART_PERR */
#define _LEUART_IF_PERR_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_PERR_DEFAULT                   (_LEUART_IF_PERR_DEFAULT << 6)    /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_FERR                           (0x1UL << 7)                      /**< Framing Error Interrupt Flag */
#define _LEUART_IF_FERR_SHIFT                    7                                 /**< Shift value for LEUART_FERR */
#define _LEUART_IF_FERR_MASK                     0x80UL                            /**< Bit mask for LEUART_FERR */
#define _LEUART_IF_FERR_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_FERR_DEFAULT                   (_LEUART_IF_FERR_DEFAULT << 7)    /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_MPAF                           (0x1UL << 8)                      /**< Multi-Processor Address Frame Interrupt Flag */
#define _LEUART_IF_MPAF_SHIFT                    8                                 /**< Shift value for LEUART_MPAF */
#define _LEUART_IF_MPAF_MASK                     0x100UL                           /**< Bit mask for LEUART_MPAF */
#define _LEUART_IF_MPAF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_MPAF_DEFAULT                   (_LEUART_IF_MPAF_DEFAULT << 8)    /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_STARTF                         (0x1UL << 9)                      /**< Start Frame Interrupt Flag */
#define _LEUART_IF_STARTF_SHIFT                  9                                 /**< Shift value for LEUART_STARTF */
#define _LEUART_IF_STARTF_MASK                   0x200UL                           /**< Bit mask for LEUART_STARTF */
#define _LEUART_IF_STARTF_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_STARTF_DEFAULT                 (_LEUART_IF_STARTF_DEFAULT << 9)  /**< Shifted mode DEFAULT for LEUART_IF */
#define LEUART_IF_SIGF                           (0x1UL << 10)                     /**< Signal Frame Interrupt Flag */
#define _LEUART_IF_SIGF_SHIFT                    10                                /**< Shift value for LEUART_SIGF */
#define _LEUART_IF_SIGF_MASK                     0x400UL                           /**< Bit mask for LEUART_SIGF */
#define _LEUART_IF_SIGF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IF */
#define LEUART_IF_SIGF_DEFAULT                   (_LEUART_IF_SIGF_DEFAULT << 10)   /**< Shifted mode DEFAULT for LEUART_IF */

/* Bit fields for LEUART IFS */
#define _LEUART_IFS_RESETVALUE                   0x00000000UL                      /**< Default value for LEUART_IFS */
#define _LEUART_IFS_MASK                         0x000007F9UL                      /**< Mask for LEUART_IFS */
#define LEUART_IFS_TXC                           (0x1UL << 0)                      /**< Set TXC Interrupt Flag */
#define _LEUART_IFS_TXC_SHIFT                    0                                 /**< Shift value for LEUART_TXC */
#define _LEUART_IFS_TXC_MASK                     0x1UL                             /**< Bit mask for LEUART_TXC */
#define _LEUART_IFS_TXC_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_TXC_DEFAULT                   (_LEUART_IFS_TXC_DEFAULT << 0)    /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_RXOF                          (0x1UL << 3)                      /**< Set RXOF Interrupt Flag */
#define _LEUART_IFS_RXOF_SHIFT                   3                                 /**< Shift value for LEUART_RXOF */
#define _LEUART_IFS_RXOF_MASK                    0x8UL                             /**< Bit mask for LEUART_RXOF */
#define _LEUART_IFS_RXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_RXOF_DEFAULT                  (_LEUART_IFS_RXOF_DEFAULT << 3)   /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_RXUF                          (0x1UL << 4)                      /**< Set RXUF Interrupt Flag */
#define _LEUART_IFS_RXUF_SHIFT                   4                                 /**< Shift value for LEUART_RXUF */
#define _LEUART_IFS_RXUF_MASK                    0x10UL                            /**< Bit mask for LEUART_RXUF */
#define _LEUART_IFS_RXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_RXUF_DEFAULT                  (_LEUART_IFS_RXUF_DEFAULT << 4)   /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_TXOF                          (0x1UL << 5)                      /**< Set TXOF Interrupt Flag */
#define _LEUART_IFS_TXOF_SHIFT                   5                                 /**< Shift value for LEUART_TXOF */
#define _LEUART_IFS_TXOF_MASK                    0x20UL                            /**< Bit mask for LEUART_TXOF */
#define _LEUART_IFS_TXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_TXOF_DEFAULT                  (_LEUART_IFS_TXOF_DEFAULT << 5)   /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_PERR                          (0x1UL << 6)                      /**< Set PERR Interrupt Flag */
#define _LEUART_IFS_PERR_SHIFT                   6                                 /**< Shift value for LEUART_PERR */
#define _LEUART_IFS_PERR_MASK                    0x40UL                            /**< Bit mask for LEUART_PERR */
#define _LEUART_IFS_PERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_PERR_DEFAULT                  (_LEUART_IFS_PERR_DEFAULT << 6)   /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_FERR                          (0x1UL << 7)                      /**< Set FERR Interrupt Flag */
#define _LEUART_IFS_FERR_SHIFT                   7                                 /**< Shift value for LEUART_FERR */
#define _LEUART_IFS_FERR_MASK                    0x80UL                            /**< Bit mask for LEUART_FERR */
#define _LEUART_IFS_FERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_FERR_DEFAULT                  (_LEUART_IFS_FERR_DEFAULT << 7)   /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_MPAF                          (0x1UL << 8)                      /**< Set MPAF Interrupt Flag */
#define _LEUART_IFS_MPAF_SHIFT                   8                                 /**< Shift value for LEUART_MPAF */
#define _LEUART_IFS_MPAF_MASK                    0x100UL                           /**< Bit mask for LEUART_MPAF */
#define _LEUART_IFS_MPAF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_MPAF_DEFAULT                  (_LEUART_IFS_MPAF_DEFAULT << 8)   /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_STARTF                        (0x1UL << 9)                      /**< Set STARTF Interrupt Flag */
#define _LEUART_IFS_STARTF_SHIFT                 9                                 /**< Shift value for LEUART_STARTF */
#define _LEUART_IFS_STARTF_MASK                  0x200UL                           /**< Bit mask for LEUART_STARTF */
#define _LEUART_IFS_STARTF_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_STARTF_DEFAULT                (_LEUART_IFS_STARTF_DEFAULT << 9) /**< Shifted mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_SIGF                          (0x1UL << 10)                     /**< Set SIGF Interrupt Flag */
#define _LEUART_IFS_SIGF_SHIFT                   10                                /**< Shift value for LEUART_SIGF */
#define _LEUART_IFS_SIGF_MASK                    0x400UL                           /**< Bit mask for LEUART_SIGF */
#define _LEUART_IFS_SIGF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFS */
#define LEUART_IFS_SIGF_DEFAULT                  (_LEUART_IFS_SIGF_DEFAULT << 10)  /**< Shifted mode DEFAULT for LEUART_IFS */

/* Bit fields for LEUART IFC */
#define _LEUART_IFC_RESETVALUE                   0x00000000UL                      /**< Default value for LEUART_IFC */
#define _LEUART_IFC_MASK                         0x000007F9UL                      /**< Mask for LEUART_IFC */
#define LEUART_IFC_TXC                           (0x1UL << 0)                      /**< Clear TXC Interrupt Flag */
#define _LEUART_IFC_TXC_SHIFT                    0                                 /**< Shift value for LEUART_TXC */
#define _LEUART_IFC_TXC_MASK                     0x1UL                             /**< Bit mask for LEUART_TXC */
#define _LEUART_IFC_TXC_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_TXC_DEFAULT                   (_LEUART_IFC_TXC_DEFAULT << 0)    /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_RXOF                          (0x1UL << 3)                      /**< Clear RXOF Interrupt Flag */
#define _LEUART_IFC_RXOF_SHIFT                   3                                 /**< Shift value for LEUART_RXOF */
#define _LEUART_IFC_RXOF_MASK                    0x8UL                             /**< Bit mask for LEUART_RXOF */
#define _LEUART_IFC_RXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_RXOF_DEFAULT                  (_LEUART_IFC_RXOF_DEFAULT << 3)   /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_RXUF                          (0x1UL << 4)                      /**< Clear RXUF Interrupt Flag */
#define _LEUART_IFC_RXUF_SHIFT                   4                                 /**< Shift value for LEUART_RXUF */
#define _LEUART_IFC_RXUF_MASK                    0x10UL                            /**< Bit mask for LEUART_RXUF */
#define _LEUART_IFC_RXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_RXUF_DEFAULT                  (_LEUART_IFC_RXUF_DEFAULT << 4)   /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_TXOF                          (0x1UL << 5)                      /**< Clear TXOF Interrupt Flag */
#define _LEUART_IFC_TXOF_SHIFT                   5                                 /**< Shift value for LEUART_TXOF */
#define _LEUART_IFC_TXOF_MASK                    0x20UL                            /**< Bit mask for LEUART_TXOF */
#define _LEUART_IFC_TXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_TXOF_DEFAULT                  (_LEUART_IFC_TXOF_DEFAULT << 5)   /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_PERR                          (0x1UL << 6)                      /**< Clear PERR Interrupt Flag */
#define _LEUART_IFC_PERR_SHIFT                   6                                 /**< Shift value for LEUART_PERR */
#define _LEUART_IFC_PERR_MASK                    0x40UL                            /**< Bit mask for LEUART_PERR */
#define _LEUART_IFC_PERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_PERR_DEFAULT                  (_LEUART_IFC_PERR_DEFAULT << 6)   /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_FERR                          (0x1UL << 7)                      /**< Clear FERR Interrupt Flag */
#define _LEUART_IFC_FERR_SHIFT                   7                                 /**< Shift value for LEUART_FERR */
#define _LEUART_IFC_FERR_MASK                    0x80UL                            /**< Bit mask for LEUART_FERR */
#define _LEUART_IFC_FERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_FERR_DEFAULT                  (_LEUART_IFC_FERR_DEFAULT << 7)   /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_MPAF                          (0x1UL << 8)                      /**< Clear MPAF Interrupt Flag */
#define _LEUART_IFC_MPAF_SHIFT                   8                                 /**< Shift value for LEUART_MPAF */
#define _LEUART_IFC_MPAF_MASK                    0x100UL                           /**< Bit mask for LEUART_MPAF */
#define _LEUART_IFC_MPAF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_MPAF_DEFAULT                  (_LEUART_IFC_MPAF_DEFAULT << 8)   /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_STARTF                        (0x1UL << 9)                      /**< Clear STARTF Interrupt Flag */
#define _LEUART_IFC_STARTF_SHIFT                 9                                 /**< Shift value for LEUART_STARTF */
#define _LEUART_IFC_STARTF_MASK                  0x200UL                           /**< Bit mask for LEUART_STARTF */
#define _LEUART_IFC_STARTF_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_STARTF_DEFAULT                (_LEUART_IFC_STARTF_DEFAULT << 9) /**< Shifted mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_SIGF                          (0x1UL << 10)                     /**< Clear SIGF Interrupt Flag */
#define _LEUART_IFC_SIGF_SHIFT                   10                                /**< Shift value for LEUART_SIGF */
#define _LEUART_IFC_SIGF_MASK                    0x400UL                           /**< Bit mask for LEUART_SIGF */
#define _LEUART_IFC_SIGF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LEUART_IFC */
#define LEUART_IFC_SIGF_DEFAULT                  (_LEUART_IFC_SIGF_DEFAULT << 10)  /**< Shifted mode DEFAULT for LEUART_IFC */

/* Bit fields for LEUART IEN */
#define _LEUART_IEN_RESETVALUE                   0x00000000UL                       /**< Default value for LEUART_IEN */
#define _LEUART_IEN_MASK                         0x000007FFUL                       /**< Mask for LEUART_IEN */
#define LEUART_IEN_TXC                           (0x1UL << 0)                       /**< TXC Interrupt Enable */
#define _LEUART_IEN_TXC_SHIFT                    0                                  /**< Shift value for LEUART_TXC */
#define _LEUART_IEN_TXC_MASK                     0x1UL                              /**< Bit mask for LEUART_TXC */
#define _LEUART_IEN_TXC_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_TXC_DEFAULT                   (_LEUART_IEN_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_TXBL                          (0x1UL << 1)                       /**< TXBL Interrupt Enable */
#define _LEUART_IEN_TXBL_SHIFT                   1                                  /**< Shift value for LEUART_TXBL */
#define _LEUART_IEN_TXBL_MASK                    0x2UL                              /**< Bit mask for LEUART_TXBL */
#define _LEUART_IEN_TXBL_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_TXBL_DEFAULT                  (_LEUART_IEN_TXBL_DEFAULT << 1)    /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_RXDATAV                       (0x1UL << 2)                       /**< RXDATAV Interrupt Enable */
#define _LEUART_IEN_RXDATAV_SHIFT                2                                  /**< Shift value for LEUART_RXDATAV */
#define _LEUART_IEN_RXDATAV_MASK                 0x4UL                              /**< Bit mask for LEUART_RXDATAV */
#define _LEUART_IEN_RXDATAV_DEFAULT              0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_RXDATAV_DEFAULT               (_LEUART_IEN_RXDATAV_DEFAULT << 2) /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_RXOF                          (0x1UL << 3)                       /**< RXOF Interrupt Enable */
#define _LEUART_IEN_RXOF_SHIFT                   3                                  /**< Shift value for LEUART_RXOF */
#define _LEUART_IEN_RXOF_MASK                    0x8UL                              /**< Bit mask for LEUART_RXOF */
#define _LEUART_IEN_RXOF_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_RXOF_DEFAULT                  (_LEUART_IEN_RXOF_DEFAULT << 3)    /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_RXUF                          (0x1UL << 4)                       /**< RXUF Interrupt Enable */
#define _LEUART_IEN_RXUF_SHIFT                   4                                  /**< Shift value for LEUART_RXUF */
#define _LEUART_IEN_RXUF_MASK                    0x10UL                             /**< Bit mask for LEUART_RXUF */
#define _LEUART_IEN_RXUF_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_RXUF_DEFAULT                  (_LEUART_IEN_RXUF_DEFAULT << 4)    /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_TXOF                          (0x1UL << 5)                       /**< TXOF Interrupt Enable */
#define _LEUART_IEN_TXOF_SHIFT                   5                                  /**< Shift value for LEUART_TXOF */
#define _LEUART_IEN_TXOF_MASK                    0x20UL                             /**< Bit mask for LEUART_TXOF */
#define _LEUART_IEN_TXOF_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_TXOF_DEFAULT                  (_LEUART_IEN_TXOF_DEFAULT << 5)    /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_PERR                          (0x1UL << 6)                       /**< PERR Interrupt Enable */
#define _LEUART_IEN_PERR_SHIFT                   6                                  /**< Shift value for LEUART_PERR */
#define _LEUART_IEN_PERR_MASK                    0x40UL                             /**< Bit mask for LEUART_PERR */
#define _LEUART_IEN_PERR_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_PERR_DEFAULT                  (_LEUART_IEN_PERR_DEFAULT << 6)    /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_FERR                          (0x1UL << 7)                       /**< FERR Interrupt Enable */
#define _LEUART_IEN_FERR_SHIFT                   7                                  /**< Shift value for LEUART_FERR */
#define _LEUART_IEN_FERR_MASK                    0x80UL                             /**< Bit mask for LEUART_FERR */
#define _LEUART_IEN_FERR_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_FERR_DEFAULT                  (_LEUART_IEN_FERR_DEFAULT << 7)    /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_MPAF                          (0x1UL << 8)                       /**< MPAF Interrupt Enable */
#define _LEUART_IEN_MPAF_SHIFT                   8                                  /**< Shift value for LEUART_MPAF */
#define _LEUART_IEN_MPAF_MASK                    0x100UL                            /**< Bit mask for LEUART_MPAF */
#define _LEUART_IEN_MPAF_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_MPAF_DEFAULT                  (_LEUART_IEN_MPAF_DEFAULT << 8)    /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_STARTF                        (0x1UL << 9)                       /**< STARTF Interrupt Enable */
#define _LEUART_IEN_STARTF_SHIFT                 9                                  /**< Shift value for LEUART_STARTF */
#define _LEUART_IEN_STARTF_MASK                  0x200UL                            /**< Bit mask for LEUART_STARTF */
#define _LEUART_IEN_STARTF_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_STARTF_DEFAULT                (_LEUART_IEN_STARTF_DEFAULT << 9)  /**< Shifted mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_SIGF                          (0x1UL << 10)                      /**< SIGF Interrupt Enable */
#define _LEUART_IEN_SIGF_SHIFT                   10                                 /**< Shift value for LEUART_SIGF */
#define _LEUART_IEN_SIGF_MASK                    0x400UL                            /**< Bit mask for LEUART_SIGF */
#define _LEUART_IEN_SIGF_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for LEUART_IEN */
#define LEUART_IEN_SIGF_DEFAULT                  (_LEUART_IEN_SIGF_DEFAULT << 10)   /**< Shifted mode DEFAULT for LEUART_IEN */

/* Bit fields for LEUART PULSECTRL */
#define _LEUART_PULSECTRL_RESETVALUE             0x00000000UL                               /**< Default value for LEUART_PULSECTRL */
#define _LEUART_PULSECTRL_MASK                   0x0000003FUL                               /**< Mask for LEUART_PULSECTRL */
#define _LEUART_PULSECTRL_PULSEW_SHIFT           0                                          /**< Shift value for LEUART_PULSEW */
#define _LEUART_PULSECTRL_PULSEW_MASK            0xFUL                                      /**< Bit mask for LEUART_PULSEW */
#define _LEUART_PULSECTRL_PULSEW_DEFAULT         0x00000000UL                               /**< Mode DEFAULT for LEUART_PULSECTRL */
#define LEUART_PULSECTRL_PULSEW_DEFAULT          (_LEUART_PULSECTRL_PULSEW_DEFAULT << 0)    /**< Shifted mode DEFAULT for LEUART_PULSECTRL */
#define LEUART_PULSECTRL_PULSEEN                 (0x1UL << 4)                               /**< Pulse Generator/Extender Enable */
#define _LEUART_PULSECTRL_PULSEEN_SHIFT          4                                          /**< Shift value for LEUART_PULSEEN */
#define _LEUART_PULSECTRL_PULSEEN_MASK           0x10UL                                     /**< Bit mask for LEUART_PULSEEN */
#define _LEUART_PULSECTRL_PULSEEN_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for LEUART_PULSECTRL */
#define LEUART_PULSECTRL_PULSEEN_DEFAULT         (_LEUART_PULSECTRL_PULSEEN_DEFAULT << 4)   /**< Shifted mode DEFAULT for LEUART_PULSECTRL */
#define LEUART_PULSECTRL_PULSEFILT               (0x1UL << 5)                               /**< Pulse Filter */
#define _LEUART_PULSECTRL_PULSEFILT_SHIFT        5                                          /**< Shift value for LEUART_PULSEFILT */
#define _LEUART_PULSECTRL_PULSEFILT_MASK         0x20UL                                     /**< Bit mask for LEUART_PULSEFILT */
#define _LEUART_PULSECTRL_PULSEFILT_DEFAULT      0x00000000UL                               /**< Mode DEFAULT for LEUART_PULSECTRL */
#define LEUART_PULSECTRL_PULSEFILT_DEFAULT       (_LEUART_PULSECTRL_PULSEFILT_DEFAULT << 5) /**< Shifted mode DEFAULT for LEUART_PULSECTRL */

/* Bit fields for LEUART FREEZE */
#define _LEUART_FREEZE_RESETVALUE                0x00000000UL                            /**< Default value for LEUART_FREEZE */
#define _LEUART_FREEZE_MASK                      0x00000001UL                            /**< Mask for LEUART_FREEZE */
#define LEUART_FREEZE_REGFREEZE                  (0x1UL << 0)                            /**< Register Update Freeze */
#define _LEUART_FREEZE_REGFREEZE_SHIFT           0                                       /**< Shift value for LEUART_REGFREEZE */
#define _LEUART_FREEZE_REGFREEZE_MASK            0x1UL                                   /**< Bit mask for LEUART_REGFREEZE */
#define _LEUART_FREEZE_REGFREEZE_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for LEUART_FREEZE */
#define _LEUART_FREEZE_REGFREEZE_UPDATE          0x00000000UL                            /**< Mode UPDATE for LEUART_FREEZE */
#define _LEUART_FREEZE_REGFREEZE_FREEZE          0x00000001UL                            /**< Mode FREEZE for LEUART_FREEZE */
#define LEUART_FREEZE_REGFREEZE_DEFAULT          (_LEUART_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_FREEZE */
#define LEUART_FREEZE_REGFREEZE_UPDATE           (_LEUART_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for LEUART_FREEZE */
#define LEUART_FREEZE_REGFREEZE_FREEZE           (_LEUART_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for LEUART_FREEZE */

/* Bit fields for LEUART SYNCBUSY */
#define _LEUART_SYNCBUSY_RESETVALUE              0x00000000UL                               /**< Default value for LEUART_SYNCBUSY */
#define _LEUART_SYNCBUSY_MASK                    0x000000FFUL                               /**< Mask for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_CTRL                     (0x1UL << 0)                               /**< CTRL Register Busy */
#define _LEUART_SYNCBUSY_CTRL_SHIFT              0                                          /**< Shift value for LEUART_CTRL */
#define _LEUART_SYNCBUSY_CTRL_MASK               0x1UL                                      /**< Bit mask for LEUART_CTRL */
#define _LEUART_SYNCBUSY_CTRL_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_CTRL_DEFAULT             (_LEUART_SYNCBUSY_CTRL_DEFAULT << 0)       /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_CMD                      (0x1UL << 1)                               /**< CMD Register Busy */
#define _LEUART_SYNCBUSY_CMD_SHIFT               1                                          /**< Shift value for LEUART_CMD */
#define _LEUART_SYNCBUSY_CMD_MASK                0x2UL                                      /**< Bit mask for LEUART_CMD */
#define _LEUART_SYNCBUSY_CMD_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_CMD_DEFAULT              (_LEUART_SYNCBUSY_CMD_DEFAULT << 1)        /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_CLKDIV                   (0x1UL << 2)                               /**< CLKDIV Register Busy */
#define _LEUART_SYNCBUSY_CLKDIV_SHIFT            2                                          /**< Shift value for LEUART_CLKDIV */
#define _LEUART_SYNCBUSY_CLKDIV_MASK             0x4UL                                      /**< Bit mask for LEUART_CLKDIV */
#define _LEUART_SYNCBUSY_CLKDIV_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_CLKDIV_DEFAULT           (_LEUART_SYNCBUSY_CLKDIV_DEFAULT << 2)     /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_STARTFRAME               (0x1UL << 3)                               /**< STARTFRAME Register Busy */
#define _LEUART_SYNCBUSY_STARTFRAME_SHIFT        3                                          /**< Shift value for LEUART_STARTFRAME */
#define _LEUART_SYNCBUSY_STARTFRAME_MASK         0x8UL                                      /**< Bit mask for LEUART_STARTFRAME */
#define _LEUART_SYNCBUSY_STARTFRAME_DEFAULT      0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_STARTFRAME_DEFAULT       (_LEUART_SYNCBUSY_STARTFRAME_DEFAULT << 3) /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_SIGFRAME                 (0x1UL << 4)                               /**< SIGFRAME Register Busy */
#define _LEUART_SYNCBUSY_SIGFRAME_SHIFT          4                                          /**< Shift value for LEUART_SIGFRAME */
#define _LEUART_SYNCBUSY_SIGFRAME_MASK           0x10UL                                     /**< Bit mask for LEUART_SIGFRAME */
#define _LEUART_SYNCBUSY_SIGFRAME_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_SIGFRAME_DEFAULT         (_LEUART_SYNCBUSY_SIGFRAME_DEFAULT << 4)   /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_TXDATAX                  (0x1UL << 5)                               /**< TXDATAX Register Busy */
#define _LEUART_SYNCBUSY_TXDATAX_SHIFT           5                                          /**< Shift value for LEUART_TXDATAX */
#define _LEUART_SYNCBUSY_TXDATAX_MASK            0x20UL                                     /**< Bit mask for LEUART_TXDATAX */
#define _LEUART_SYNCBUSY_TXDATAX_DEFAULT         0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_TXDATAX_DEFAULT          (_LEUART_SYNCBUSY_TXDATAX_DEFAULT << 5)    /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_TXDATA                   (0x1UL << 6)                               /**< TXDATA Register Busy */
#define _LEUART_SYNCBUSY_TXDATA_SHIFT            6                                          /**< Shift value for LEUART_TXDATA */
#define _LEUART_SYNCBUSY_TXDATA_MASK             0x40UL                                     /**< Bit mask for LEUART_TXDATA */
#define _LEUART_SYNCBUSY_TXDATA_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_TXDATA_DEFAULT           (_LEUART_SYNCBUSY_TXDATA_DEFAULT << 6)     /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_PULSECTRL                (0x1UL << 7)                               /**< PULSECTRL Register Busy */
#define _LEUART_SYNCBUSY_PULSECTRL_SHIFT         7                                          /**< Shift value for LEUART_PULSECTRL */
#define _LEUART_SYNCBUSY_PULSECTRL_MASK          0x80UL                                     /**< Bit mask for LEUART_PULSECTRL */
#define _LEUART_SYNCBUSY_PULSECTRL_DEFAULT       0x00000000UL                               /**< Mode DEFAULT for LEUART_SYNCBUSY */
#define LEUART_SYNCBUSY_PULSECTRL_DEFAULT        (_LEUART_SYNCBUSY_PULSECTRL_DEFAULT << 7)  /**< Shifted mode DEFAULT for LEUART_SYNCBUSY */

/* Bit fields for LEUART ROUTEPEN */
#define _LEUART_ROUTEPEN_RESETVALUE              0x00000000UL                          /**< Default value for LEUART_ROUTEPEN */
#define _LEUART_ROUTEPEN_MASK                    0x00000003UL                          /**< Mask for LEUART_ROUTEPEN */
#define LEUART_ROUTEPEN_RXPEN                    (0x1UL << 0)                          /**< RX Pin Enable */
#define _LEUART_ROUTEPEN_RXPEN_SHIFT             0                                     /**< Shift value for LEUART_RXPEN */
#define _LEUART_ROUTEPEN_RXPEN_MASK              0x1UL                                 /**< Bit mask for LEUART_RXPEN */
#define _LEUART_ROUTEPEN_RXPEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LEUART_ROUTEPEN */
#define LEUART_ROUTEPEN_RXPEN_DEFAULT            (_LEUART_ROUTEPEN_RXPEN_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_ROUTEPEN */
#define LEUART_ROUTEPEN_TXPEN                    (0x1UL << 1)                          /**< TX Pin Enable */
#define _LEUART_ROUTEPEN_TXPEN_SHIFT             1                                     /**< Shift value for LEUART_TXPEN */
#define _LEUART_ROUTEPEN_TXPEN_MASK              0x2UL                                 /**< Bit mask for LEUART_TXPEN */
#define _LEUART_ROUTEPEN_TXPEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LEUART_ROUTEPEN */
#define LEUART_ROUTEPEN_TXPEN_DEFAULT            (_LEUART_ROUTEPEN_TXPEN_DEFAULT << 1) /**< Shifted mode DEFAULT for LEUART_ROUTEPEN */

/* Bit fields for LEUART ROUTELOC0 */
#define _LEUART_ROUTELOC0_RESETVALUE             0x00000000UL                           /**< Default value for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_MASK                   0x00001F1FUL                           /**< Mask for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_SHIFT            0                                      /**< Shift value for LEUART_RXLOC */
#define _LEUART_ROUTELOC0_RXLOC_MASK             0x1FUL                                 /**< Bit mask for LEUART_RXLOC */
#define _LEUART_ROUTELOC0_RXLOC_LOC0             0x00000000UL                           /**< Mode LOC0 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC1             0x00000001UL                           /**< Mode LOC1 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC2             0x00000002UL                           /**< Mode LOC2 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC3             0x00000003UL                           /**< Mode LOC3 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC4             0x00000004UL                           /**< Mode LOC4 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC5             0x00000005UL                           /**< Mode LOC5 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC6             0x00000006UL                           /**< Mode LOC6 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC7             0x00000007UL                           /**< Mode LOC7 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC8             0x00000008UL                           /**< Mode LOC8 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC9             0x00000009UL                           /**< Mode LOC9 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC10            0x0000000AUL                           /**< Mode LOC10 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC11            0x0000000BUL                           /**< Mode LOC11 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC12            0x0000000CUL                           /**< Mode LOC12 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC13            0x0000000DUL                           /**< Mode LOC13 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC14            0x0000000EUL                           /**< Mode LOC14 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC15            0x0000000FUL                           /**< Mode LOC15 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC16            0x00000010UL                           /**< Mode LOC16 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC17            0x00000011UL                           /**< Mode LOC17 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC18            0x00000012UL                           /**< Mode LOC18 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC19            0x00000013UL                           /**< Mode LOC19 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC20            0x00000014UL                           /**< Mode LOC20 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC21            0x00000015UL                           /**< Mode LOC21 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC22            0x00000016UL                           /**< Mode LOC22 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC23            0x00000017UL                           /**< Mode LOC23 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC24            0x00000018UL                           /**< Mode LOC24 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC25            0x00000019UL                           /**< Mode LOC25 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC26            0x0000001AUL                           /**< Mode LOC26 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC27            0x0000001BUL                           /**< Mode LOC27 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC28            0x0000001CUL                           /**< Mode LOC28 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC29            0x0000001DUL                           /**< Mode LOC29 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC30            0x0000001EUL                           /**< Mode LOC30 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_RXLOC_LOC31            0x0000001FUL                           /**< Mode LOC31 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC0              (_LEUART_ROUTELOC0_RXLOC_LOC0 << 0)    /**< Shifted mode LOC0 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_DEFAULT           (_LEUART_ROUTELOC0_RXLOC_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC1              (_LEUART_ROUTELOC0_RXLOC_LOC1 << 0)    /**< Shifted mode LOC1 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC2              (_LEUART_ROUTELOC0_RXLOC_LOC2 << 0)    /**< Shifted mode LOC2 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC3              (_LEUART_ROUTELOC0_RXLOC_LOC3 << 0)    /**< Shifted mode LOC3 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC4              (_LEUART_ROUTELOC0_RXLOC_LOC4 << 0)    /**< Shifted mode LOC4 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC5              (_LEUART_ROUTELOC0_RXLOC_LOC5 << 0)    /**< Shifted mode LOC5 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC6              (_LEUART_ROUTELOC0_RXLOC_LOC6 << 0)    /**< Shifted mode LOC6 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC7              (_LEUART_ROUTELOC0_RXLOC_LOC7 << 0)    /**< Shifted mode LOC7 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC8              (_LEUART_ROUTELOC0_RXLOC_LOC8 << 0)    /**< Shifted mode LOC8 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC9              (_LEUART_ROUTELOC0_RXLOC_LOC9 << 0)    /**< Shifted mode LOC9 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC10             (_LEUART_ROUTELOC0_RXLOC_LOC10 << 0)   /**< Shifted mode LOC10 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC11             (_LEUART_ROUTELOC0_RXLOC_LOC11 << 0)   /**< Shifted mode LOC11 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC12             (_LEUART_ROUTELOC0_RXLOC_LOC12 << 0)   /**< Shifted mode LOC12 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC13             (_LEUART_ROUTELOC0_RXLOC_LOC13 << 0)   /**< Shifted mode LOC13 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC14             (_LEUART_ROUTELOC0_RXLOC_LOC14 << 0)   /**< Shifted mode LOC14 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC15             (_LEUART_ROUTELOC0_RXLOC_LOC15 << 0)   /**< Shifted mode LOC15 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC16             (_LEUART_ROUTELOC0_RXLOC_LOC16 << 0)   /**< Shifted mode LOC16 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC17             (_LEUART_ROUTELOC0_RXLOC_LOC17 << 0)   /**< Shifted mode LOC17 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC18             (_LEUART_ROUTELOC0_RXLOC_LOC18 << 0)   /**< Shifted mode LOC18 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC19             (_LEUART_ROUTELOC0_RXLOC_LOC19 << 0)   /**< Shifted mode LOC19 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC20             (_LEUART_ROUTELOC0_RXLOC_LOC20 << 0)   /**< Shifted mode LOC20 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC21             (_LEUART_ROUTELOC0_RXLOC_LOC21 << 0)   /**< Shifted mode LOC21 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC22             (_LEUART_ROUTELOC0_RXLOC_LOC22 << 0)   /**< Shifted mode LOC22 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC23             (_LEUART_ROUTELOC0_RXLOC_LOC23 << 0)   /**< Shifted mode LOC23 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC24             (_LEUART_ROUTELOC0_RXLOC_LOC24 << 0)   /**< Shifted mode LOC24 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC25             (_LEUART_ROUTELOC0_RXLOC_LOC25 << 0)   /**< Shifted mode LOC25 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC26             (_LEUART_ROUTELOC0_RXLOC_LOC26 << 0)   /**< Shifted mode LOC26 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC27             (_LEUART_ROUTELOC0_RXLOC_LOC27 << 0)   /**< Shifted mode LOC27 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC28             (_LEUART_ROUTELOC0_RXLOC_LOC28 << 0)   /**< Shifted mode LOC28 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC29             (_LEUART_ROUTELOC0_RXLOC_LOC29 << 0)   /**< Shifted mode LOC29 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC30             (_LEUART_ROUTELOC0_RXLOC_LOC30 << 0)   /**< Shifted mode LOC30 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_RXLOC_LOC31             (_LEUART_ROUTELOC0_RXLOC_LOC31 << 0)   /**< Shifted mode LOC31 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_SHIFT            8                                      /**< Shift value for LEUART_TXLOC */
#define _LEUART_ROUTELOC0_TXLOC_MASK             0x1F00UL                               /**< Bit mask for LEUART_TXLOC */
#define _LEUART_ROUTELOC0_TXLOC_LOC0             0x00000000UL                           /**< Mode LOC0 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC1             0x00000001UL                           /**< Mode LOC1 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC2             0x00000002UL                           /**< Mode LOC2 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC3             0x00000003UL                           /**< Mode LOC3 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC4             0x00000004UL                           /**< Mode LOC4 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC5             0x00000005UL                           /**< Mode LOC5 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC6             0x00000006UL                           /**< Mode LOC6 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC7             0x00000007UL                           /**< Mode LOC7 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC8             0x00000008UL                           /**< Mode LOC8 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC9             0x00000009UL                           /**< Mode LOC9 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC10            0x0000000AUL                           /**< Mode LOC10 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC11            0x0000000BUL                           /**< Mode LOC11 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC12            0x0000000CUL                           /**< Mode LOC12 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC13            0x0000000DUL                           /**< Mode LOC13 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC14            0x0000000EUL                           /**< Mode LOC14 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC15            0x0000000FUL                           /**< Mode LOC15 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC16            0x00000010UL                           /**< Mode LOC16 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC17            0x00000011UL                           /**< Mode LOC17 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC18            0x00000012UL                           /**< Mode LOC18 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC19            0x00000013UL                           /**< Mode LOC19 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC20            0x00000014UL                           /**< Mode LOC20 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC21            0x00000015UL                           /**< Mode LOC21 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC22            0x00000016UL                           /**< Mode LOC22 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC23            0x00000017UL                           /**< Mode LOC23 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC24            0x00000018UL                           /**< Mode LOC24 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC25            0x00000019UL                           /**< Mode LOC25 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC26            0x0000001AUL                           /**< Mode LOC26 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC27            0x0000001BUL                           /**< Mode LOC27 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC28            0x0000001CUL                           /**< Mode LOC28 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC29            0x0000001DUL                           /**< Mode LOC29 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC30            0x0000001EUL                           /**< Mode LOC30 for LEUART_ROUTELOC0 */
#define _LEUART_ROUTELOC0_TXLOC_LOC31            0x0000001FUL                           /**< Mode LOC31 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC0              (_LEUART_ROUTELOC0_TXLOC_LOC0 << 8)    /**< Shifted mode LOC0 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_DEFAULT           (_LEUART_ROUTELOC0_TXLOC_DEFAULT << 8) /**< Shifted mode DEFAULT for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC1              (_LEUART_ROUTELOC0_TXLOC_LOC1 << 8)    /**< Shifted mode LOC1 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC2              (_LEUART_ROUTELOC0_TXLOC_LOC2 << 8)    /**< Shifted mode LOC2 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC3              (_LEUART_ROUTELOC0_TXLOC_LOC3 << 8)    /**< Shifted mode LOC3 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC4              (_LEUART_ROUTELOC0_TXLOC_LOC4 << 8)    /**< Shifted mode LOC4 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC5              (_LEUART_ROUTELOC0_TXLOC_LOC5 << 8)    /**< Shifted mode LOC5 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC6              (_LEUART_ROUTELOC0_TXLOC_LOC6 << 8)    /**< Shifted mode LOC6 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC7              (_LEUART_ROUTELOC0_TXLOC_LOC7 << 8)    /**< Shifted mode LOC7 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC8              (_LEUART_ROUTELOC0_TXLOC_LOC8 << 8)    /**< Shifted mode LOC8 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC9              (_LEUART_ROUTELOC0_TXLOC_LOC9 << 8)    /**< Shifted mode LOC9 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC10             (_LEUART_ROUTELOC0_TXLOC_LOC10 << 8)   /**< Shifted mode LOC10 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC11             (_LEUART_ROUTELOC0_TXLOC_LOC11 << 8)   /**< Shifted mode LOC11 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC12             (_LEUART_ROUTELOC0_TXLOC_LOC12 << 8)   /**< Shifted mode LOC12 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC13             (_LEUART_ROUTELOC0_TXLOC_LOC13 << 8)   /**< Shifted mode LOC13 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC14             (_LEUART_ROUTELOC0_TXLOC_LOC14 << 8)   /**< Shifted mode LOC14 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC15             (_LEUART_ROUTELOC0_TXLOC_LOC15 << 8)   /**< Shifted mode LOC15 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC16             (_LEUART_ROUTELOC0_TXLOC_LOC16 << 8)   /**< Shifted mode LOC16 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC17             (_LEUART_ROUTELOC0_TXLOC_LOC17 << 8)   /**< Shifted mode LOC17 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC18             (_LEUART_ROUTELOC0_TXLOC_LOC18 << 8)   /**< Shifted mode LOC18 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC19             (_LEUART_ROUTELOC0_TXLOC_LOC19 << 8)   /**< Shifted mode LOC19 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC20             (_LEUART_ROUTELOC0_TXLOC_LOC20 << 8)   /**< Shifted mode LOC20 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC21             (_LEUART_ROUTELOC0_TXLOC_LOC21 << 8)   /**< Shifted mode LOC21 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC22             (_LEUART_ROUTELOC0_TXLOC_LOC22 << 8)   /**< Shifted mode LOC22 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC23             (_LEUART_ROUTELOC0_TXLOC_LOC23 << 8)   /**< Shifted mode LOC23 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC24             (_LEUART_ROUTELOC0_TXLOC_LOC24 << 8)   /**< Shifted mode LOC24 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC25             (_LEUART_ROUTELOC0_TXLOC_LOC25 << 8)   /**< Shifted mode LOC25 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC26             (_LEUART_ROUTELOC0_TXLOC_LOC26 << 8)   /**< Shifted mode LOC26 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC27             (_LEUART_ROUTELOC0_TXLOC_LOC27 << 8)   /**< Shifted mode LOC27 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC28             (_LEUART_ROUTELOC0_TXLOC_LOC28 << 8)   /**< Shifted mode LOC28 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC29             (_LEUART_ROUTELOC0_TXLOC_LOC29 << 8)   /**< Shifted mode LOC29 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC30             (_LEUART_ROUTELOC0_TXLOC_LOC30 << 8)   /**< Shifted mode LOC30 for LEUART_ROUTELOC0 */
#define LEUART_ROUTELOC0_TXLOC_LOC31             (_LEUART_ROUTELOC0_TXLOC_LOC31 << 8)   /**< Shifted mode LOC31 for LEUART_ROUTELOC0 */

/* Bit fields for LEUART INPUT */
#define _LEUART_INPUT_RESETVALUE                 0x00000000UL                          /**< Default value for LEUART_INPUT */
#define _LEUART_INPUT_MASK                       0x0000002FUL                          /**< Mask for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_SHIFT             0                                     /**< Shift value for LEUART_RXPRSSEL */
#define _LEUART_INPUT_RXPRSSEL_MASK              0xFUL                                 /**< Bit mask for LEUART_RXPRSSEL */
#define _LEUART_INPUT_RXPRSSEL_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH0            0x00000000UL                          /**< Mode PRSCH0 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH1            0x00000001UL                          /**< Mode PRSCH1 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH2            0x00000002UL                          /**< Mode PRSCH2 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH3            0x00000003UL                          /**< Mode PRSCH3 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH4            0x00000004UL                          /**< Mode PRSCH4 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH5            0x00000005UL                          /**< Mode PRSCH5 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH6            0x00000006UL                          /**< Mode PRSCH6 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH7            0x00000007UL                          /**< Mode PRSCH7 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH8            0x00000008UL                          /**< Mode PRSCH8 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH9            0x00000009UL                          /**< Mode PRSCH9 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH10           0x0000000AUL                          /**< Mode PRSCH10 for LEUART_INPUT */
#define _LEUART_INPUT_RXPRSSEL_PRSCH11           0x0000000BUL                          /**< Mode PRSCH11 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_DEFAULT            (_LEUART_INPUT_RXPRSSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH0             (_LEUART_INPUT_RXPRSSEL_PRSCH0 << 0)  /**< Shifted mode PRSCH0 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH1             (_LEUART_INPUT_RXPRSSEL_PRSCH1 << 0)  /**< Shifted mode PRSCH1 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH2             (_LEUART_INPUT_RXPRSSEL_PRSCH2 << 0)  /**< Shifted mode PRSCH2 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH3             (_LEUART_INPUT_RXPRSSEL_PRSCH3 << 0)  /**< Shifted mode PRSCH3 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH4             (_LEUART_INPUT_RXPRSSEL_PRSCH4 << 0)  /**< Shifted mode PRSCH4 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH5             (_LEUART_INPUT_RXPRSSEL_PRSCH5 << 0)  /**< Shifted mode PRSCH5 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH6             (_LEUART_INPUT_RXPRSSEL_PRSCH6 << 0)  /**< Shifted mode PRSCH6 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH7             (_LEUART_INPUT_RXPRSSEL_PRSCH7 << 0)  /**< Shifted mode PRSCH7 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH8             (_LEUART_INPUT_RXPRSSEL_PRSCH8 << 0)  /**< Shifted mode PRSCH8 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH9             (_LEUART_INPUT_RXPRSSEL_PRSCH9 << 0)  /**< Shifted mode PRSCH9 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH10            (_LEUART_INPUT_RXPRSSEL_PRSCH10 << 0) /**< Shifted mode PRSCH10 for LEUART_INPUT */
#define LEUART_INPUT_RXPRSSEL_PRSCH11            (_LEUART_INPUT_RXPRSSEL_PRSCH11 << 0) /**< Shifted mode PRSCH11 for LEUART_INPUT */
#define LEUART_INPUT_RXPRS                       (0x1UL << 5)                          /**< PRS RX Enable */
#define _LEUART_INPUT_RXPRS_SHIFT                5                                     /**< Shift value for LEUART_RXPRS */
#define _LEUART_INPUT_RXPRS_MASK                 0x20UL                                /**< Bit mask for LEUART_RXPRS */
#define _LEUART_INPUT_RXPRS_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for LEUART_INPUT */
#define LEUART_INPUT_RXPRS_DEFAULT               (_LEUART_INPUT_RXPRS_DEFAULT << 5)    /**< Shifted mode DEFAULT for LEUART_INPUT */

/** @} End of group EFR32FG1P_LEUART */
/** @} End of group Parts */

