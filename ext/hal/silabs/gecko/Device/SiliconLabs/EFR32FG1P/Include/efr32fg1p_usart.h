/**************************************************************************//**
 * @file efr32fg1p_usart.h
 * @brief EFR32FG1P_USART register and bit field definitions
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
 * @defgroup EFR32FG1P_USART
 * @{
 * @brief EFR32FG1P_USART Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< Control Register  */
  __IOM uint32_t FRAME;        /**< USART Frame Format Register  */
  __IOM uint32_t TRIGCTRL;     /**< USART Trigger Control register  */
  __IOM uint32_t CMD;          /**< Command Register  */
  __IM uint32_t  STATUS;       /**< USART Status Register  */
  __IOM uint32_t CLKDIV;       /**< Clock Control Register  */
  __IM uint32_t  RXDATAX;      /**< RX Buffer Data Extended Register  */
  __IM uint32_t  RXDATA;       /**< RX Buffer Data Register  */
  __IM uint32_t  RXDOUBLEX;    /**< RX Buffer Double Data Extended Register  */
  __IM uint32_t  RXDOUBLE;     /**< RX FIFO Double Data Register  */
  __IM uint32_t  RXDATAXP;     /**< RX Buffer Data Extended Peek Register  */
  __IM uint32_t  RXDOUBLEXP;   /**< RX Buffer Double Data Extended Peek Register  */
  __IOM uint32_t TXDATAX;      /**< TX Buffer Data Extended Register  */
  __IOM uint32_t TXDATA;       /**< TX Buffer Data Register  */
  __IOM uint32_t TXDOUBLEX;    /**< TX Buffer Double Data Extended Register  */
  __IOM uint32_t TXDOUBLE;     /**< TX Buffer Double Data Register  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */
  __IOM uint32_t IRCTRL;       /**< IrDA Control Register  */
  uint32_t       RESERVED0[1]; /**< Reserved for future use **/
  __IOM uint32_t INPUT;        /**< USART Input Register  */
  __IOM uint32_t I2SCTRL;      /**< I2S Control Register  */
  __IOM uint32_t TIMING;       /**< Timing Register  */
  __IOM uint32_t CTRLX;        /**< Control Register Extended  */
  __IOM uint32_t TIMECMP0;     /**< Used to generate interrupts and various delays  */
  __IOM uint32_t TIMECMP1;     /**< Used to generate interrupts and various delays  */
  __IOM uint32_t TIMECMP2;     /**< Used to generate interrupts and various delays  */
  __IOM uint32_t ROUTEPEN;     /**< I/O Routing Pin Enable Register  */
  __IOM uint32_t ROUTELOC0;    /**< I/O Routing Location Register  */
  __IOM uint32_t ROUTELOC1;    /**< I/O Routing Location Register  */
} USART_TypeDef;               /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_USART_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for USART CTRL */
#define _USART_CTRL_RESETVALUE                  0x00000000UL                             /**< Default value for USART_CTRL */
#define _USART_CTRL_MASK                        0xF3FFFF7FUL                             /**< Mask for USART_CTRL */
#define USART_CTRL_SYNC                         (0x1UL << 0)                             /**< USART Synchronous Mode */
#define _USART_CTRL_SYNC_SHIFT                  0                                        /**< Shift value for USART_SYNC */
#define _USART_CTRL_SYNC_MASK                   0x1UL                                    /**< Bit mask for USART_SYNC */
#define _USART_CTRL_SYNC_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_SYNC_DEFAULT                 (_USART_CTRL_SYNC_DEFAULT << 0)          /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_LOOPBK                       (0x1UL << 1)                             /**< Loopback Enable */
#define _USART_CTRL_LOOPBK_SHIFT                1                                        /**< Shift value for USART_LOOPBK */
#define _USART_CTRL_LOOPBK_MASK                 0x2UL                                    /**< Bit mask for USART_LOOPBK */
#define _USART_CTRL_LOOPBK_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_LOOPBK_DEFAULT               (_USART_CTRL_LOOPBK_DEFAULT << 1)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_CCEN                         (0x1UL << 2)                             /**< Collision Check Enable */
#define _USART_CTRL_CCEN_SHIFT                  2                                        /**< Shift value for USART_CCEN */
#define _USART_CTRL_CCEN_MASK                   0x4UL                                    /**< Bit mask for USART_CCEN */
#define _USART_CTRL_CCEN_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_CCEN_DEFAULT                 (_USART_CTRL_CCEN_DEFAULT << 2)          /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_MPM                          (0x1UL << 3)                             /**< Multi-Processor Mode */
#define _USART_CTRL_MPM_SHIFT                   3                                        /**< Shift value for USART_MPM */
#define _USART_CTRL_MPM_MASK                    0x8UL                                    /**< Bit mask for USART_MPM */
#define _USART_CTRL_MPM_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_MPM_DEFAULT                  (_USART_CTRL_MPM_DEFAULT << 3)           /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_MPAB                         (0x1UL << 4)                             /**< Multi-Processor Address-Bit */
#define _USART_CTRL_MPAB_SHIFT                  4                                        /**< Shift value for USART_MPAB */
#define _USART_CTRL_MPAB_MASK                   0x10UL                                   /**< Bit mask for USART_MPAB */
#define _USART_CTRL_MPAB_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_MPAB_DEFAULT                 (_USART_CTRL_MPAB_DEFAULT << 4)          /**< Shifted mode DEFAULT for USART_CTRL */
#define _USART_CTRL_OVS_SHIFT                   5                                        /**< Shift value for USART_OVS */
#define _USART_CTRL_OVS_MASK                    0x60UL                                   /**< Bit mask for USART_OVS */
#define _USART_CTRL_OVS_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define _USART_CTRL_OVS_X16                     0x00000000UL                             /**< Mode X16 for USART_CTRL */
#define _USART_CTRL_OVS_X8                      0x00000001UL                             /**< Mode X8 for USART_CTRL */
#define _USART_CTRL_OVS_X6                      0x00000002UL                             /**< Mode X6 for USART_CTRL */
#define _USART_CTRL_OVS_X4                      0x00000003UL                             /**< Mode X4 for USART_CTRL */
#define USART_CTRL_OVS_DEFAULT                  (_USART_CTRL_OVS_DEFAULT << 5)           /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_OVS_X16                      (_USART_CTRL_OVS_X16 << 5)               /**< Shifted mode X16 for USART_CTRL */
#define USART_CTRL_OVS_X8                       (_USART_CTRL_OVS_X8 << 5)                /**< Shifted mode X8 for USART_CTRL */
#define USART_CTRL_OVS_X6                       (_USART_CTRL_OVS_X6 << 5)                /**< Shifted mode X6 for USART_CTRL */
#define USART_CTRL_OVS_X4                       (_USART_CTRL_OVS_X4 << 5)                /**< Shifted mode X4 for USART_CTRL */
#define USART_CTRL_CLKPOL                       (0x1UL << 8)                             /**< Clock Polarity */
#define _USART_CTRL_CLKPOL_SHIFT                8                                        /**< Shift value for USART_CLKPOL */
#define _USART_CTRL_CLKPOL_MASK                 0x100UL                                  /**< Bit mask for USART_CLKPOL */
#define _USART_CTRL_CLKPOL_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define _USART_CTRL_CLKPOL_IDLELOW              0x00000000UL                             /**< Mode IDLELOW for USART_CTRL */
#define _USART_CTRL_CLKPOL_IDLEHIGH             0x00000001UL                             /**< Mode IDLEHIGH for USART_CTRL */
#define USART_CTRL_CLKPOL_DEFAULT               (_USART_CTRL_CLKPOL_DEFAULT << 8)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_CLKPOL_IDLELOW               (_USART_CTRL_CLKPOL_IDLELOW << 8)        /**< Shifted mode IDLELOW for USART_CTRL */
#define USART_CTRL_CLKPOL_IDLEHIGH              (_USART_CTRL_CLKPOL_IDLEHIGH << 8)       /**< Shifted mode IDLEHIGH for USART_CTRL */
#define USART_CTRL_CLKPHA                       (0x1UL << 9)                             /**< Clock Edge For Setup/Sample */
#define _USART_CTRL_CLKPHA_SHIFT                9                                        /**< Shift value for USART_CLKPHA */
#define _USART_CTRL_CLKPHA_MASK                 0x200UL                                  /**< Bit mask for USART_CLKPHA */
#define _USART_CTRL_CLKPHA_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define _USART_CTRL_CLKPHA_SAMPLELEADING        0x00000000UL                             /**< Mode SAMPLELEADING for USART_CTRL */
#define _USART_CTRL_CLKPHA_SAMPLETRAILING       0x00000001UL                             /**< Mode SAMPLETRAILING for USART_CTRL */
#define USART_CTRL_CLKPHA_DEFAULT               (_USART_CTRL_CLKPHA_DEFAULT << 9)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_CLKPHA_SAMPLELEADING         (_USART_CTRL_CLKPHA_SAMPLELEADING << 9)  /**< Shifted mode SAMPLELEADING for USART_CTRL */
#define USART_CTRL_CLKPHA_SAMPLETRAILING        (_USART_CTRL_CLKPHA_SAMPLETRAILING << 9) /**< Shifted mode SAMPLETRAILING for USART_CTRL */
#define USART_CTRL_MSBF                         (0x1UL << 10)                            /**< Most Significant Bit First */
#define _USART_CTRL_MSBF_SHIFT                  10                                       /**< Shift value for USART_MSBF */
#define _USART_CTRL_MSBF_MASK                   0x400UL                                  /**< Bit mask for USART_MSBF */
#define _USART_CTRL_MSBF_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_MSBF_DEFAULT                 (_USART_CTRL_MSBF_DEFAULT << 10)         /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_CSMA                         (0x1UL << 11)                            /**< Action On Slave-Select In Master Mode */
#define _USART_CTRL_CSMA_SHIFT                  11                                       /**< Shift value for USART_CSMA */
#define _USART_CTRL_CSMA_MASK                   0x800UL                                  /**< Bit mask for USART_CSMA */
#define _USART_CTRL_CSMA_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define _USART_CTRL_CSMA_NOACTION               0x00000000UL                             /**< Mode NOACTION for USART_CTRL */
#define _USART_CTRL_CSMA_GOTOSLAVEMODE          0x00000001UL                             /**< Mode GOTOSLAVEMODE for USART_CTRL */
#define USART_CTRL_CSMA_DEFAULT                 (_USART_CTRL_CSMA_DEFAULT << 11)         /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_CSMA_NOACTION                (_USART_CTRL_CSMA_NOACTION << 11)        /**< Shifted mode NOACTION for USART_CTRL */
#define USART_CTRL_CSMA_GOTOSLAVEMODE           (_USART_CTRL_CSMA_GOTOSLAVEMODE << 11)   /**< Shifted mode GOTOSLAVEMODE for USART_CTRL */
#define USART_CTRL_TXBIL                        (0x1UL << 12)                            /**< TX Buffer Interrupt Level */
#define _USART_CTRL_TXBIL_SHIFT                 12                                       /**< Shift value for USART_TXBIL */
#define _USART_CTRL_TXBIL_MASK                  0x1000UL                                 /**< Bit mask for USART_TXBIL */
#define _USART_CTRL_TXBIL_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define _USART_CTRL_TXBIL_EMPTY                 0x00000000UL                             /**< Mode EMPTY for USART_CTRL */
#define _USART_CTRL_TXBIL_HALFFULL              0x00000001UL                             /**< Mode HALFFULL for USART_CTRL */
#define USART_CTRL_TXBIL_DEFAULT                (_USART_CTRL_TXBIL_DEFAULT << 12)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_TXBIL_EMPTY                  (_USART_CTRL_TXBIL_EMPTY << 12)          /**< Shifted mode EMPTY for USART_CTRL */
#define USART_CTRL_TXBIL_HALFFULL               (_USART_CTRL_TXBIL_HALFFULL << 12)       /**< Shifted mode HALFFULL for USART_CTRL */
#define USART_CTRL_RXINV                        (0x1UL << 13)                            /**< Receiver Input Invert */
#define _USART_CTRL_RXINV_SHIFT                 13                                       /**< Shift value for USART_RXINV */
#define _USART_CTRL_RXINV_MASK                  0x2000UL                                 /**< Bit mask for USART_RXINV */
#define _USART_CTRL_RXINV_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_RXINV_DEFAULT                (_USART_CTRL_RXINV_DEFAULT << 13)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_TXINV                        (0x1UL << 14)                            /**< Transmitter output Invert */
#define _USART_CTRL_TXINV_SHIFT                 14                                       /**< Shift value for USART_TXINV */
#define _USART_CTRL_TXINV_MASK                  0x4000UL                                 /**< Bit mask for USART_TXINV */
#define _USART_CTRL_TXINV_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_TXINV_DEFAULT                (_USART_CTRL_TXINV_DEFAULT << 14)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_CSINV                        (0x1UL << 15)                            /**< Chip Select Invert */
#define _USART_CTRL_CSINV_SHIFT                 15                                       /**< Shift value for USART_CSINV */
#define _USART_CTRL_CSINV_MASK                  0x8000UL                                 /**< Bit mask for USART_CSINV */
#define _USART_CTRL_CSINV_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_CSINV_DEFAULT                (_USART_CTRL_CSINV_DEFAULT << 15)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_AUTOCS                       (0x1UL << 16)                            /**< Automatic Chip Select */
#define _USART_CTRL_AUTOCS_SHIFT                16                                       /**< Shift value for USART_AUTOCS */
#define _USART_CTRL_AUTOCS_MASK                 0x10000UL                                /**< Bit mask for USART_AUTOCS */
#define _USART_CTRL_AUTOCS_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_AUTOCS_DEFAULT               (_USART_CTRL_AUTOCS_DEFAULT << 16)       /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_AUTOTRI                      (0x1UL << 17)                            /**< Automatic TX Tristate */
#define _USART_CTRL_AUTOTRI_SHIFT               17                                       /**< Shift value for USART_AUTOTRI */
#define _USART_CTRL_AUTOTRI_MASK                0x20000UL                                /**< Bit mask for USART_AUTOTRI */
#define _USART_CTRL_AUTOTRI_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_AUTOTRI_DEFAULT              (_USART_CTRL_AUTOTRI_DEFAULT << 17)      /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_SCMODE                       (0x1UL << 18)                            /**< SmartCard Mode */
#define _USART_CTRL_SCMODE_SHIFT                18                                       /**< Shift value for USART_SCMODE */
#define _USART_CTRL_SCMODE_MASK                 0x40000UL                                /**< Bit mask for USART_SCMODE */
#define _USART_CTRL_SCMODE_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_SCMODE_DEFAULT               (_USART_CTRL_SCMODE_DEFAULT << 18)       /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_SCRETRANS                    (0x1UL << 19)                            /**< SmartCard Retransmit */
#define _USART_CTRL_SCRETRANS_SHIFT             19                                       /**< Shift value for USART_SCRETRANS */
#define _USART_CTRL_SCRETRANS_MASK              0x80000UL                                /**< Bit mask for USART_SCRETRANS */
#define _USART_CTRL_SCRETRANS_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_SCRETRANS_DEFAULT            (_USART_CTRL_SCRETRANS_DEFAULT << 19)    /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_SKIPPERRF                    (0x1UL << 20)                            /**< Skip Parity Error Frames */
#define _USART_CTRL_SKIPPERRF_SHIFT             20                                       /**< Shift value for USART_SKIPPERRF */
#define _USART_CTRL_SKIPPERRF_MASK              0x100000UL                               /**< Bit mask for USART_SKIPPERRF */
#define _USART_CTRL_SKIPPERRF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_SKIPPERRF_DEFAULT            (_USART_CTRL_SKIPPERRF_DEFAULT << 20)    /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_BIT8DV                       (0x1UL << 21)                            /**< Bit 8 Default Value */
#define _USART_CTRL_BIT8DV_SHIFT                21                                       /**< Shift value for USART_BIT8DV */
#define _USART_CTRL_BIT8DV_MASK                 0x200000UL                               /**< Bit mask for USART_BIT8DV */
#define _USART_CTRL_BIT8DV_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_BIT8DV_DEFAULT               (_USART_CTRL_BIT8DV_DEFAULT << 21)       /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_ERRSDMA                      (0x1UL << 22)                            /**< Halt DMA On Error */
#define _USART_CTRL_ERRSDMA_SHIFT               22                                       /**< Shift value for USART_ERRSDMA */
#define _USART_CTRL_ERRSDMA_MASK                0x400000UL                               /**< Bit mask for USART_ERRSDMA */
#define _USART_CTRL_ERRSDMA_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_ERRSDMA_DEFAULT              (_USART_CTRL_ERRSDMA_DEFAULT << 22)      /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_ERRSRX                       (0x1UL << 23)                            /**< Disable RX On Error */
#define _USART_CTRL_ERRSRX_SHIFT                23                                       /**< Shift value for USART_ERRSRX */
#define _USART_CTRL_ERRSRX_MASK                 0x800000UL                               /**< Bit mask for USART_ERRSRX */
#define _USART_CTRL_ERRSRX_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_ERRSRX_DEFAULT               (_USART_CTRL_ERRSRX_DEFAULT << 23)       /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_ERRSTX                       (0x1UL << 24)                            /**< Disable TX On Error */
#define _USART_CTRL_ERRSTX_SHIFT                24                                       /**< Shift value for USART_ERRSTX */
#define _USART_CTRL_ERRSTX_MASK                 0x1000000UL                              /**< Bit mask for USART_ERRSTX */
#define _USART_CTRL_ERRSTX_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_ERRSTX_DEFAULT               (_USART_CTRL_ERRSTX_DEFAULT << 24)       /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_SSSEARLY                     (0x1UL << 25)                            /**< Synchronous Slave Setup Early */
#define _USART_CTRL_SSSEARLY_SHIFT              25                                       /**< Shift value for USART_SSSEARLY */
#define _USART_CTRL_SSSEARLY_MASK               0x2000000UL                              /**< Bit mask for USART_SSSEARLY */
#define _USART_CTRL_SSSEARLY_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_SSSEARLY_DEFAULT             (_USART_CTRL_SSSEARLY_DEFAULT << 25)     /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_BYTESWAP                     (0x1UL << 28)                            /**< Byteswap In Double Accesses */
#define _USART_CTRL_BYTESWAP_SHIFT              28                                       /**< Shift value for USART_BYTESWAP */
#define _USART_CTRL_BYTESWAP_MASK               0x10000000UL                             /**< Bit mask for USART_BYTESWAP */
#define _USART_CTRL_BYTESWAP_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_BYTESWAP_DEFAULT             (_USART_CTRL_BYTESWAP_DEFAULT << 28)     /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_AUTOTX                       (0x1UL << 29)                            /**< Always Transmit When RX Not Full */
#define _USART_CTRL_AUTOTX_SHIFT                29                                       /**< Shift value for USART_AUTOTX */
#define _USART_CTRL_AUTOTX_MASK                 0x20000000UL                             /**< Bit mask for USART_AUTOTX */
#define _USART_CTRL_AUTOTX_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_AUTOTX_DEFAULT               (_USART_CTRL_AUTOTX_DEFAULT << 29)       /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_MVDIS                        (0x1UL << 30)                            /**< Majority Vote Disable */
#define _USART_CTRL_MVDIS_SHIFT                 30                                       /**< Shift value for USART_MVDIS */
#define _USART_CTRL_MVDIS_MASK                  0x40000000UL                             /**< Bit mask for USART_MVDIS */
#define _USART_CTRL_MVDIS_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_MVDIS_DEFAULT                (_USART_CTRL_MVDIS_DEFAULT << 30)        /**< Shifted mode DEFAULT for USART_CTRL */
#define USART_CTRL_SMSDELAY                     (0x1UL << 31)                            /**< Synchronous Master Sample Delay */
#define _USART_CTRL_SMSDELAY_SHIFT              31                                       /**< Shift value for USART_SMSDELAY */
#define _USART_CTRL_SMSDELAY_MASK               0x80000000UL                             /**< Bit mask for USART_SMSDELAY */
#define _USART_CTRL_SMSDELAY_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USART_CTRL */
#define USART_CTRL_SMSDELAY_DEFAULT             (_USART_CTRL_SMSDELAY_DEFAULT << 31)     /**< Shifted mode DEFAULT for USART_CTRL */

/* Bit fields for USART FRAME */
#define _USART_FRAME_RESETVALUE                 0x00001005UL                              /**< Default value for USART_FRAME */
#define _USART_FRAME_MASK                       0x0000330FUL                              /**< Mask for USART_FRAME */
#define _USART_FRAME_DATABITS_SHIFT             0                                         /**< Shift value for USART_DATABITS */
#define _USART_FRAME_DATABITS_MASK              0xFUL                                     /**< Bit mask for USART_DATABITS */
#define _USART_FRAME_DATABITS_FOUR              0x00000001UL                              /**< Mode FOUR for USART_FRAME */
#define _USART_FRAME_DATABITS_FIVE              0x00000002UL                              /**< Mode FIVE for USART_FRAME */
#define _USART_FRAME_DATABITS_SIX               0x00000003UL                              /**< Mode SIX for USART_FRAME */
#define _USART_FRAME_DATABITS_SEVEN             0x00000004UL                              /**< Mode SEVEN for USART_FRAME */
#define _USART_FRAME_DATABITS_DEFAULT           0x00000005UL                              /**< Mode DEFAULT for USART_FRAME */
#define _USART_FRAME_DATABITS_EIGHT             0x00000005UL                              /**< Mode EIGHT for USART_FRAME */
#define _USART_FRAME_DATABITS_NINE              0x00000006UL                              /**< Mode NINE for USART_FRAME */
#define _USART_FRAME_DATABITS_TEN               0x00000007UL                              /**< Mode TEN for USART_FRAME */
#define _USART_FRAME_DATABITS_ELEVEN            0x00000008UL                              /**< Mode ELEVEN for USART_FRAME */
#define _USART_FRAME_DATABITS_TWELVE            0x00000009UL                              /**< Mode TWELVE for USART_FRAME */
#define _USART_FRAME_DATABITS_THIRTEEN          0x0000000AUL                              /**< Mode THIRTEEN for USART_FRAME */
#define _USART_FRAME_DATABITS_FOURTEEN          0x0000000BUL                              /**< Mode FOURTEEN for USART_FRAME */
#define _USART_FRAME_DATABITS_FIFTEEN           0x0000000CUL                              /**< Mode FIFTEEN for USART_FRAME */
#define _USART_FRAME_DATABITS_SIXTEEN           0x0000000DUL                              /**< Mode SIXTEEN for USART_FRAME */
#define USART_FRAME_DATABITS_FOUR               (_USART_FRAME_DATABITS_FOUR << 0)         /**< Shifted mode FOUR for USART_FRAME */
#define USART_FRAME_DATABITS_FIVE               (_USART_FRAME_DATABITS_FIVE << 0)         /**< Shifted mode FIVE for USART_FRAME */
#define USART_FRAME_DATABITS_SIX                (_USART_FRAME_DATABITS_SIX << 0)          /**< Shifted mode SIX for USART_FRAME */
#define USART_FRAME_DATABITS_SEVEN              (_USART_FRAME_DATABITS_SEVEN << 0)        /**< Shifted mode SEVEN for USART_FRAME */
#define USART_FRAME_DATABITS_DEFAULT            (_USART_FRAME_DATABITS_DEFAULT << 0)      /**< Shifted mode DEFAULT for USART_FRAME */
#define USART_FRAME_DATABITS_EIGHT              (_USART_FRAME_DATABITS_EIGHT << 0)        /**< Shifted mode EIGHT for USART_FRAME */
#define USART_FRAME_DATABITS_NINE               (_USART_FRAME_DATABITS_NINE << 0)         /**< Shifted mode NINE for USART_FRAME */
#define USART_FRAME_DATABITS_TEN                (_USART_FRAME_DATABITS_TEN << 0)          /**< Shifted mode TEN for USART_FRAME */
#define USART_FRAME_DATABITS_ELEVEN             (_USART_FRAME_DATABITS_ELEVEN << 0)       /**< Shifted mode ELEVEN for USART_FRAME */
#define USART_FRAME_DATABITS_TWELVE             (_USART_FRAME_DATABITS_TWELVE << 0)       /**< Shifted mode TWELVE for USART_FRAME */
#define USART_FRAME_DATABITS_THIRTEEN           (_USART_FRAME_DATABITS_THIRTEEN << 0)     /**< Shifted mode THIRTEEN for USART_FRAME */
#define USART_FRAME_DATABITS_FOURTEEN           (_USART_FRAME_DATABITS_FOURTEEN << 0)     /**< Shifted mode FOURTEEN for USART_FRAME */
#define USART_FRAME_DATABITS_FIFTEEN            (_USART_FRAME_DATABITS_FIFTEEN << 0)      /**< Shifted mode FIFTEEN for USART_FRAME */
#define USART_FRAME_DATABITS_SIXTEEN            (_USART_FRAME_DATABITS_SIXTEEN << 0)      /**< Shifted mode SIXTEEN for USART_FRAME */
#define _USART_FRAME_PARITY_SHIFT               8                                         /**< Shift value for USART_PARITY */
#define _USART_FRAME_PARITY_MASK                0x300UL                                   /**< Bit mask for USART_PARITY */
#define _USART_FRAME_PARITY_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for USART_FRAME */
#define _USART_FRAME_PARITY_NONE                0x00000000UL                              /**< Mode NONE for USART_FRAME */
#define _USART_FRAME_PARITY_EVEN                0x00000002UL                              /**< Mode EVEN for USART_FRAME */
#define _USART_FRAME_PARITY_ODD                 0x00000003UL                              /**< Mode ODD for USART_FRAME */
#define USART_FRAME_PARITY_DEFAULT              (_USART_FRAME_PARITY_DEFAULT << 8)        /**< Shifted mode DEFAULT for USART_FRAME */
#define USART_FRAME_PARITY_NONE                 (_USART_FRAME_PARITY_NONE << 8)           /**< Shifted mode NONE for USART_FRAME */
#define USART_FRAME_PARITY_EVEN                 (_USART_FRAME_PARITY_EVEN << 8)           /**< Shifted mode EVEN for USART_FRAME */
#define USART_FRAME_PARITY_ODD                  (_USART_FRAME_PARITY_ODD << 8)            /**< Shifted mode ODD for USART_FRAME */
#define _USART_FRAME_STOPBITS_SHIFT             12                                        /**< Shift value for USART_STOPBITS */
#define _USART_FRAME_STOPBITS_MASK              0x3000UL                                  /**< Bit mask for USART_STOPBITS */
#define _USART_FRAME_STOPBITS_HALF              0x00000000UL                              /**< Mode HALF for USART_FRAME */
#define _USART_FRAME_STOPBITS_DEFAULT           0x00000001UL                              /**< Mode DEFAULT for USART_FRAME */
#define _USART_FRAME_STOPBITS_ONE               0x00000001UL                              /**< Mode ONE for USART_FRAME */
#define _USART_FRAME_STOPBITS_ONEANDAHALF       0x00000002UL                              /**< Mode ONEANDAHALF for USART_FRAME */
#define _USART_FRAME_STOPBITS_TWO               0x00000003UL                              /**< Mode TWO for USART_FRAME */
#define USART_FRAME_STOPBITS_HALF               (_USART_FRAME_STOPBITS_HALF << 12)        /**< Shifted mode HALF for USART_FRAME */
#define USART_FRAME_STOPBITS_DEFAULT            (_USART_FRAME_STOPBITS_DEFAULT << 12)     /**< Shifted mode DEFAULT for USART_FRAME */
#define USART_FRAME_STOPBITS_ONE                (_USART_FRAME_STOPBITS_ONE << 12)         /**< Shifted mode ONE for USART_FRAME */
#define USART_FRAME_STOPBITS_ONEANDAHALF        (_USART_FRAME_STOPBITS_ONEANDAHALF << 12) /**< Shifted mode ONEANDAHALF for USART_FRAME */
#define USART_FRAME_STOPBITS_TWO                (_USART_FRAME_STOPBITS_TWO << 12)         /**< Shifted mode TWO for USART_FRAME */

/* Bit fields for USART TRIGCTRL */
#define _USART_TRIGCTRL_RESETVALUE              0x00000000UL                             /**< Default value for USART_TRIGCTRL */
#define _USART_TRIGCTRL_MASK                    0x000F1FF0UL                             /**< Mask for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXTEN                    (0x1UL << 4)                             /**< Receive Trigger Enable */
#define _USART_TRIGCTRL_RXTEN_SHIFT             4                                        /**< Shift value for USART_RXTEN */
#define _USART_TRIGCTRL_RXTEN_MASK              0x10UL                                   /**< Bit mask for USART_RXTEN */
#define _USART_TRIGCTRL_RXTEN_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXTEN_DEFAULT            (_USART_TRIGCTRL_RXTEN_DEFAULT << 4)     /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXTEN                    (0x1UL << 5)                             /**< Transmit Trigger Enable */
#define _USART_TRIGCTRL_TXTEN_SHIFT             5                                        /**< Shift value for USART_TXTEN */
#define _USART_TRIGCTRL_TXTEN_MASK              0x20UL                                   /**< Bit mask for USART_TXTEN */
#define _USART_TRIGCTRL_TXTEN_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXTEN_DEFAULT            (_USART_TRIGCTRL_TXTEN_DEFAULT << 5)     /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_AUTOTXTEN                (0x1UL << 6)                             /**< AUTOTX Trigger Enable */
#define _USART_TRIGCTRL_AUTOTXTEN_SHIFT         6                                        /**< Shift value for USART_AUTOTXTEN */
#define _USART_TRIGCTRL_AUTOTXTEN_MASK          0x40UL                                   /**< Bit mask for USART_AUTOTXTEN */
#define _USART_TRIGCTRL_AUTOTXTEN_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_AUTOTXTEN_DEFAULT        (_USART_TRIGCTRL_AUTOTXTEN_DEFAULT << 6) /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXARX0EN                 (0x1UL << 7)                             /**< Enable Transmit Trigger after RX End of Frame plus TCMP0VAL */
#define _USART_TRIGCTRL_TXARX0EN_SHIFT          7                                        /**< Shift value for USART_TXARX0EN */
#define _USART_TRIGCTRL_TXARX0EN_MASK           0x80UL                                   /**< Bit mask for USART_TXARX0EN */
#define _USART_TRIGCTRL_TXARX0EN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXARX0EN_DEFAULT         (_USART_TRIGCTRL_TXARX0EN_DEFAULT << 7)  /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXARX1EN                 (0x1UL << 8)                             /**< Enable Transmit Trigger after RX End of Frame plus TCMP1VAL */
#define _USART_TRIGCTRL_TXARX1EN_SHIFT          8                                        /**< Shift value for USART_TXARX1EN */
#define _USART_TRIGCTRL_TXARX1EN_MASK           0x100UL                                  /**< Bit mask for USART_TXARX1EN */
#define _USART_TRIGCTRL_TXARX1EN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXARX1EN_DEFAULT         (_USART_TRIGCTRL_TXARX1EN_DEFAULT << 8)  /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXARX2EN                 (0x1UL << 9)                             /**< Enable Transmit Trigger after RX End of Frame plus TCMP2VAL */
#define _USART_TRIGCTRL_TXARX2EN_SHIFT          9                                        /**< Shift value for USART_TXARX2EN */
#define _USART_TRIGCTRL_TXARX2EN_MASK           0x200UL                                  /**< Bit mask for USART_TXARX2EN */
#define _USART_TRIGCTRL_TXARX2EN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TXARX2EN_DEFAULT         (_USART_TRIGCTRL_TXARX2EN_DEFAULT << 9)  /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXATX0EN                 (0x1UL << 10)                            /**< Enable Receive Trigger after TX end of frame plus TCMPVAL0 baud-times */
#define _USART_TRIGCTRL_RXATX0EN_SHIFT          10                                       /**< Shift value for USART_RXATX0EN */
#define _USART_TRIGCTRL_RXATX0EN_MASK           0x400UL                                  /**< Bit mask for USART_RXATX0EN */
#define _USART_TRIGCTRL_RXATX0EN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXATX0EN_DEFAULT         (_USART_TRIGCTRL_RXATX0EN_DEFAULT << 10) /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXATX1EN                 (0x1UL << 11)                            /**< Enable Receive Trigger after TX end of frame plus TCMPVAL1 baud-times */
#define _USART_TRIGCTRL_RXATX1EN_SHIFT          11                                       /**< Shift value for USART_RXATX1EN */
#define _USART_TRIGCTRL_RXATX1EN_MASK           0x800UL                                  /**< Bit mask for USART_RXATX1EN */
#define _USART_TRIGCTRL_RXATX1EN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXATX1EN_DEFAULT         (_USART_TRIGCTRL_RXATX1EN_DEFAULT << 11) /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXATX2EN                 (0x1UL << 12)                            /**< Enable Receive Trigger after TX end of frame plus TCMPVAL2 baud-times */
#define _USART_TRIGCTRL_RXATX2EN_SHIFT          12                                       /**< Shift value for USART_RXATX2EN */
#define _USART_TRIGCTRL_RXATX2EN_MASK           0x1000UL                                 /**< Bit mask for USART_RXATX2EN */
#define _USART_TRIGCTRL_RXATX2EN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_RXATX2EN_DEFAULT         (_USART_TRIGCTRL_RXATX2EN_DEFAULT << 12) /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_SHIFT              16                                       /**< Shift value for USART_TSEL */
#define _USART_TRIGCTRL_TSEL_MASK               0xF0000UL                                /**< Bit mask for USART_TSEL */
#define _USART_TRIGCTRL_TSEL_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH0             0x00000000UL                             /**< Mode PRSCH0 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH1             0x00000001UL                             /**< Mode PRSCH1 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH2             0x00000002UL                             /**< Mode PRSCH2 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH3             0x00000003UL                             /**< Mode PRSCH3 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH4             0x00000004UL                             /**< Mode PRSCH4 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH5             0x00000005UL                             /**< Mode PRSCH5 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH6             0x00000006UL                             /**< Mode PRSCH6 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH7             0x00000007UL                             /**< Mode PRSCH7 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH8             0x00000008UL                             /**< Mode PRSCH8 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH9             0x00000009UL                             /**< Mode PRSCH9 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH10            0x0000000AUL                             /**< Mode PRSCH10 for USART_TRIGCTRL */
#define _USART_TRIGCTRL_TSEL_PRSCH11            0x0000000BUL                             /**< Mode PRSCH11 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_DEFAULT             (_USART_TRIGCTRL_TSEL_DEFAULT << 16)     /**< Shifted mode DEFAULT for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH0              (_USART_TRIGCTRL_TSEL_PRSCH0 << 16)      /**< Shifted mode PRSCH0 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH1              (_USART_TRIGCTRL_TSEL_PRSCH1 << 16)      /**< Shifted mode PRSCH1 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH2              (_USART_TRIGCTRL_TSEL_PRSCH2 << 16)      /**< Shifted mode PRSCH2 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH3              (_USART_TRIGCTRL_TSEL_PRSCH3 << 16)      /**< Shifted mode PRSCH3 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH4              (_USART_TRIGCTRL_TSEL_PRSCH4 << 16)      /**< Shifted mode PRSCH4 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH5              (_USART_TRIGCTRL_TSEL_PRSCH5 << 16)      /**< Shifted mode PRSCH5 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH6              (_USART_TRIGCTRL_TSEL_PRSCH6 << 16)      /**< Shifted mode PRSCH6 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH7              (_USART_TRIGCTRL_TSEL_PRSCH7 << 16)      /**< Shifted mode PRSCH7 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH8              (_USART_TRIGCTRL_TSEL_PRSCH8 << 16)      /**< Shifted mode PRSCH8 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH9              (_USART_TRIGCTRL_TSEL_PRSCH9 << 16)      /**< Shifted mode PRSCH9 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH10             (_USART_TRIGCTRL_TSEL_PRSCH10 << 16)     /**< Shifted mode PRSCH10 for USART_TRIGCTRL */
#define USART_TRIGCTRL_TSEL_PRSCH11             (_USART_TRIGCTRL_TSEL_PRSCH11 << 16)     /**< Shifted mode PRSCH11 for USART_TRIGCTRL */

/* Bit fields for USART CMD */
#define _USART_CMD_RESETVALUE                   0x00000000UL                         /**< Default value for USART_CMD */
#define _USART_CMD_MASK                         0x00000FFFUL                         /**< Mask for USART_CMD */
#define USART_CMD_RXEN                          (0x1UL << 0)                         /**< Receiver Enable */
#define _USART_CMD_RXEN_SHIFT                   0                                    /**< Shift value for USART_RXEN */
#define _USART_CMD_RXEN_MASK                    0x1UL                                /**< Bit mask for USART_RXEN */
#define _USART_CMD_RXEN_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_RXEN_DEFAULT                  (_USART_CMD_RXEN_DEFAULT << 0)       /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_RXDIS                         (0x1UL << 1)                         /**< Receiver Disable */
#define _USART_CMD_RXDIS_SHIFT                  1                                    /**< Shift value for USART_RXDIS */
#define _USART_CMD_RXDIS_MASK                   0x2UL                                /**< Bit mask for USART_RXDIS */
#define _USART_CMD_RXDIS_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_RXDIS_DEFAULT                 (_USART_CMD_RXDIS_DEFAULT << 1)      /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_TXEN                          (0x1UL << 2)                         /**< Transmitter Enable */
#define _USART_CMD_TXEN_SHIFT                   2                                    /**< Shift value for USART_TXEN */
#define _USART_CMD_TXEN_MASK                    0x4UL                                /**< Bit mask for USART_TXEN */
#define _USART_CMD_TXEN_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_TXEN_DEFAULT                  (_USART_CMD_TXEN_DEFAULT << 2)       /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_TXDIS                         (0x1UL << 3)                         /**< Transmitter Disable */
#define _USART_CMD_TXDIS_SHIFT                  3                                    /**< Shift value for USART_TXDIS */
#define _USART_CMD_TXDIS_MASK                   0x8UL                                /**< Bit mask for USART_TXDIS */
#define _USART_CMD_TXDIS_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_TXDIS_DEFAULT                 (_USART_CMD_TXDIS_DEFAULT << 3)      /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_MASTEREN                      (0x1UL << 4)                         /**< Master Enable */
#define _USART_CMD_MASTEREN_SHIFT               4                                    /**< Shift value for USART_MASTEREN */
#define _USART_CMD_MASTEREN_MASK                0x10UL                               /**< Bit mask for USART_MASTEREN */
#define _USART_CMD_MASTEREN_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_MASTEREN_DEFAULT              (_USART_CMD_MASTEREN_DEFAULT << 4)   /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_MASTERDIS                     (0x1UL << 5)                         /**< Master Disable */
#define _USART_CMD_MASTERDIS_SHIFT              5                                    /**< Shift value for USART_MASTERDIS */
#define _USART_CMD_MASTERDIS_MASK               0x20UL                               /**< Bit mask for USART_MASTERDIS */
#define _USART_CMD_MASTERDIS_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_MASTERDIS_DEFAULT             (_USART_CMD_MASTERDIS_DEFAULT << 5)  /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_RXBLOCKEN                     (0x1UL << 6)                         /**< Receiver Block Enable */
#define _USART_CMD_RXBLOCKEN_SHIFT              6                                    /**< Shift value for USART_RXBLOCKEN */
#define _USART_CMD_RXBLOCKEN_MASK               0x40UL                               /**< Bit mask for USART_RXBLOCKEN */
#define _USART_CMD_RXBLOCKEN_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_RXBLOCKEN_DEFAULT             (_USART_CMD_RXBLOCKEN_DEFAULT << 6)  /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_RXBLOCKDIS                    (0x1UL << 7)                         /**< Receiver Block Disable */
#define _USART_CMD_RXBLOCKDIS_SHIFT             7                                    /**< Shift value for USART_RXBLOCKDIS */
#define _USART_CMD_RXBLOCKDIS_MASK              0x80UL                               /**< Bit mask for USART_RXBLOCKDIS */
#define _USART_CMD_RXBLOCKDIS_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_RXBLOCKDIS_DEFAULT            (_USART_CMD_RXBLOCKDIS_DEFAULT << 7) /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_TXTRIEN                       (0x1UL << 8)                         /**< Transmitter Tristate Enable */
#define _USART_CMD_TXTRIEN_SHIFT                8                                    /**< Shift value for USART_TXTRIEN */
#define _USART_CMD_TXTRIEN_MASK                 0x100UL                              /**< Bit mask for USART_TXTRIEN */
#define _USART_CMD_TXTRIEN_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_TXTRIEN_DEFAULT               (_USART_CMD_TXTRIEN_DEFAULT << 8)    /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_TXTRIDIS                      (0x1UL << 9)                         /**< Transmitter Tristate Disable */
#define _USART_CMD_TXTRIDIS_SHIFT               9                                    /**< Shift value for USART_TXTRIDIS */
#define _USART_CMD_TXTRIDIS_MASK                0x200UL                              /**< Bit mask for USART_TXTRIDIS */
#define _USART_CMD_TXTRIDIS_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_TXTRIDIS_DEFAULT              (_USART_CMD_TXTRIDIS_DEFAULT << 9)   /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_CLEARTX                       (0x1UL << 10)                        /**< Clear TX */
#define _USART_CMD_CLEARTX_SHIFT                10                                   /**< Shift value for USART_CLEARTX */
#define _USART_CMD_CLEARTX_MASK                 0x400UL                              /**< Bit mask for USART_CLEARTX */
#define _USART_CMD_CLEARTX_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_CLEARTX_DEFAULT               (_USART_CMD_CLEARTX_DEFAULT << 10)   /**< Shifted mode DEFAULT for USART_CMD */
#define USART_CMD_CLEARRX                       (0x1UL << 11)                        /**< Clear RX */
#define _USART_CMD_CLEARRX_SHIFT                11                                   /**< Shift value for USART_CLEARRX */
#define _USART_CMD_CLEARRX_MASK                 0x800UL                              /**< Bit mask for USART_CLEARRX */
#define _USART_CMD_CLEARRX_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for USART_CMD */
#define USART_CMD_CLEARRX_DEFAULT               (_USART_CMD_CLEARRX_DEFAULT << 11)   /**< Shifted mode DEFAULT for USART_CMD */

/* Bit fields for USART STATUS */
#define _USART_STATUS_RESETVALUE                0x00002040UL                                 /**< Default value for USART_STATUS */
#define _USART_STATUS_MASK                      0x00037FFFUL                                 /**< Mask for USART_STATUS */
#define USART_STATUS_RXENS                      (0x1UL << 0)                                 /**< Receiver Enable Status */
#define _USART_STATUS_RXENS_SHIFT               0                                            /**< Shift value for USART_RXENS */
#define _USART_STATUS_RXENS_MASK                0x1UL                                        /**< Bit mask for USART_RXENS */
#define _USART_STATUS_RXENS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXENS_DEFAULT              (_USART_STATUS_RXENS_DEFAULT << 0)           /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXENS                      (0x1UL << 1)                                 /**< Transmitter Enable Status */
#define _USART_STATUS_TXENS_SHIFT               1                                            /**< Shift value for USART_TXENS */
#define _USART_STATUS_TXENS_MASK                0x2UL                                        /**< Bit mask for USART_TXENS */
#define _USART_STATUS_TXENS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXENS_DEFAULT              (_USART_STATUS_TXENS_DEFAULT << 1)           /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_MASTER                     (0x1UL << 2)                                 /**< SPI Master Mode */
#define _USART_STATUS_MASTER_SHIFT              2                                            /**< Shift value for USART_MASTER */
#define _USART_STATUS_MASTER_MASK               0x4UL                                        /**< Bit mask for USART_MASTER */
#define _USART_STATUS_MASTER_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_MASTER_DEFAULT             (_USART_STATUS_MASTER_DEFAULT << 2)          /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXBLOCK                    (0x1UL << 3)                                 /**< Block Incoming Data */
#define _USART_STATUS_RXBLOCK_SHIFT             3                                            /**< Shift value for USART_RXBLOCK */
#define _USART_STATUS_RXBLOCK_MASK              0x8UL                                        /**< Bit mask for USART_RXBLOCK */
#define _USART_STATUS_RXBLOCK_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXBLOCK_DEFAULT            (_USART_STATUS_RXBLOCK_DEFAULT << 3)         /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXTRI                      (0x1UL << 4)                                 /**< Transmitter Tristated */
#define _USART_STATUS_TXTRI_SHIFT               4                                            /**< Shift value for USART_TXTRI */
#define _USART_STATUS_TXTRI_MASK                0x10UL                                       /**< Bit mask for USART_TXTRI */
#define _USART_STATUS_TXTRI_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXTRI_DEFAULT              (_USART_STATUS_TXTRI_DEFAULT << 4)           /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXC                        (0x1UL << 5)                                 /**< TX Complete */
#define _USART_STATUS_TXC_SHIFT                 5                                            /**< Shift value for USART_TXC */
#define _USART_STATUS_TXC_MASK                  0x20UL                                       /**< Bit mask for USART_TXC */
#define _USART_STATUS_TXC_DEFAULT               0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXC_DEFAULT                (_USART_STATUS_TXC_DEFAULT << 5)             /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXBL                       (0x1UL << 6)                                 /**< TX Buffer Level */
#define _USART_STATUS_TXBL_SHIFT                6                                            /**< Shift value for USART_TXBL */
#define _USART_STATUS_TXBL_MASK                 0x40UL                                       /**< Bit mask for USART_TXBL */
#define _USART_STATUS_TXBL_DEFAULT              0x00000001UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXBL_DEFAULT               (_USART_STATUS_TXBL_DEFAULT << 6)            /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXDATAV                    (0x1UL << 7)                                 /**< RX Data Valid */
#define _USART_STATUS_RXDATAV_SHIFT             7                                            /**< Shift value for USART_RXDATAV */
#define _USART_STATUS_RXDATAV_MASK              0x80UL                                       /**< Bit mask for USART_RXDATAV */
#define _USART_STATUS_RXDATAV_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXDATAV_DEFAULT            (_USART_STATUS_RXDATAV_DEFAULT << 7)         /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXFULL                     (0x1UL << 8)                                 /**< RX FIFO Full */
#define _USART_STATUS_RXFULL_SHIFT              8                                            /**< Shift value for USART_RXFULL */
#define _USART_STATUS_RXFULL_MASK               0x100UL                                      /**< Bit mask for USART_RXFULL */
#define _USART_STATUS_RXFULL_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXFULL_DEFAULT             (_USART_STATUS_RXFULL_DEFAULT << 8)          /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXBDRIGHT                  (0x1UL << 9)                                 /**< TX Buffer Expects Double Right Data */
#define _USART_STATUS_TXBDRIGHT_SHIFT           9                                            /**< Shift value for USART_TXBDRIGHT */
#define _USART_STATUS_TXBDRIGHT_MASK            0x200UL                                      /**< Bit mask for USART_TXBDRIGHT */
#define _USART_STATUS_TXBDRIGHT_DEFAULT         0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXBDRIGHT_DEFAULT          (_USART_STATUS_TXBDRIGHT_DEFAULT << 9)       /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXBSRIGHT                  (0x1UL << 10)                                /**< TX Buffer Expects Single Right Data */
#define _USART_STATUS_TXBSRIGHT_SHIFT           10                                           /**< Shift value for USART_TXBSRIGHT */
#define _USART_STATUS_TXBSRIGHT_MASK            0x400UL                                      /**< Bit mask for USART_TXBSRIGHT */
#define _USART_STATUS_TXBSRIGHT_DEFAULT         0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXBSRIGHT_DEFAULT          (_USART_STATUS_TXBSRIGHT_DEFAULT << 10)      /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXDATAVRIGHT               (0x1UL << 11)                                /**< RX Data Right */
#define _USART_STATUS_RXDATAVRIGHT_SHIFT        11                                           /**< Shift value for USART_RXDATAVRIGHT */
#define _USART_STATUS_RXDATAVRIGHT_MASK         0x800UL                                      /**< Bit mask for USART_RXDATAVRIGHT */
#define _USART_STATUS_RXDATAVRIGHT_DEFAULT      0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXDATAVRIGHT_DEFAULT       (_USART_STATUS_RXDATAVRIGHT_DEFAULT << 11)   /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXFULLRIGHT                (0x1UL << 12)                                /**< RX Full of Right Data */
#define _USART_STATUS_RXFULLRIGHT_SHIFT         12                                           /**< Shift value for USART_RXFULLRIGHT */
#define _USART_STATUS_RXFULLRIGHT_MASK          0x1000UL                                     /**< Bit mask for USART_RXFULLRIGHT */
#define _USART_STATUS_RXFULLRIGHT_DEFAULT       0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_RXFULLRIGHT_DEFAULT        (_USART_STATUS_RXFULLRIGHT_DEFAULT << 12)    /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXIDLE                     (0x1UL << 13)                                /**< TX Idle */
#define _USART_STATUS_TXIDLE_SHIFT              13                                           /**< Shift value for USART_TXIDLE */
#define _USART_STATUS_TXIDLE_MASK               0x2000UL                                     /**< Bit mask for USART_TXIDLE */
#define _USART_STATUS_TXIDLE_DEFAULT            0x00000001UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXIDLE_DEFAULT             (_USART_STATUS_TXIDLE_DEFAULT << 13)         /**< Shifted mode DEFAULT for USART_STATUS */
#define USART_STATUS_TIMERRESTARTED             (0x1UL << 14)                                /**< The USART Timer restarted itself */
#define _USART_STATUS_TIMERRESTARTED_SHIFT      14                                           /**< Shift value for USART_TIMERRESTARTED */
#define _USART_STATUS_TIMERRESTARTED_MASK       0x4000UL                                     /**< Bit mask for USART_TIMERRESTARTED */
#define _USART_STATUS_TIMERRESTARTED_DEFAULT    0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TIMERRESTARTED_DEFAULT     (_USART_STATUS_TIMERRESTARTED_DEFAULT << 14) /**< Shifted mode DEFAULT for USART_STATUS */
#define _USART_STATUS_TXBUFCNT_SHIFT            16                                           /**< Shift value for USART_TXBUFCNT */
#define _USART_STATUS_TXBUFCNT_MASK             0x30000UL                                    /**< Bit mask for USART_TXBUFCNT */
#define _USART_STATUS_TXBUFCNT_DEFAULT          0x00000000UL                                 /**< Mode DEFAULT for USART_STATUS */
#define USART_STATUS_TXBUFCNT_DEFAULT           (_USART_STATUS_TXBUFCNT_DEFAULT << 16)       /**< Shifted mode DEFAULT for USART_STATUS */

/* Bit fields for USART CLKDIV */
#define _USART_CLKDIV_RESETVALUE                0x00000000UL                             /**< Default value for USART_CLKDIV */
#define _USART_CLKDIV_MASK                      0x807FFFF8UL                             /**< Mask for USART_CLKDIV */
#define _USART_CLKDIV_DIV_SHIFT                 3                                        /**< Shift value for USART_DIV */
#define _USART_CLKDIV_DIV_MASK                  0x7FFFF8UL                               /**< Bit mask for USART_DIV */
#define _USART_CLKDIV_DIV_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for USART_CLKDIV */
#define USART_CLKDIV_DIV_DEFAULT                (_USART_CLKDIV_DIV_DEFAULT << 3)         /**< Shifted mode DEFAULT for USART_CLKDIV */
#define USART_CLKDIV_AUTOBAUDEN                 (0x1UL << 31)                            /**< AUTOBAUD detection enable */
#define _USART_CLKDIV_AUTOBAUDEN_SHIFT          31                                       /**< Shift value for USART_AUTOBAUDEN */
#define _USART_CLKDIV_AUTOBAUDEN_MASK           0x80000000UL                             /**< Bit mask for USART_AUTOBAUDEN */
#define _USART_CLKDIV_AUTOBAUDEN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_CLKDIV */
#define USART_CLKDIV_AUTOBAUDEN_DEFAULT         (_USART_CLKDIV_AUTOBAUDEN_DEFAULT << 31) /**< Shifted mode DEFAULT for USART_CLKDIV */

/* Bit fields for USART RXDATAX */
#define _USART_RXDATAX_RESETVALUE               0x00000000UL                         /**< Default value for USART_RXDATAX */
#define _USART_RXDATAX_MASK                     0x0000C1FFUL                         /**< Mask for USART_RXDATAX */
#define _USART_RXDATAX_RXDATA_SHIFT             0                                    /**< Shift value for USART_RXDATA */
#define _USART_RXDATAX_RXDATA_MASK              0x1FFUL                              /**< Bit mask for USART_RXDATA */
#define _USART_RXDATAX_RXDATA_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for USART_RXDATAX */
#define USART_RXDATAX_RXDATA_DEFAULT            (_USART_RXDATAX_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_RXDATAX */
#define USART_RXDATAX_PERR                      (0x1UL << 14)                        /**< Data Parity Error */
#define _USART_RXDATAX_PERR_SHIFT               14                                   /**< Shift value for USART_PERR */
#define _USART_RXDATAX_PERR_MASK                0x4000UL                             /**< Bit mask for USART_PERR */
#define _USART_RXDATAX_PERR_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for USART_RXDATAX */
#define USART_RXDATAX_PERR_DEFAULT              (_USART_RXDATAX_PERR_DEFAULT << 14)  /**< Shifted mode DEFAULT for USART_RXDATAX */
#define USART_RXDATAX_FERR                      (0x1UL << 15)                        /**< Data Framing Error */
#define _USART_RXDATAX_FERR_SHIFT               15                                   /**< Shift value for USART_FERR */
#define _USART_RXDATAX_FERR_MASK                0x8000UL                             /**< Bit mask for USART_FERR */
#define _USART_RXDATAX_FERR_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for USART_RXDATAX */
#define USART_RXDATAX_FERR_DEFAULT              (_USART_RXDATAX_FERR_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_RXDATAX */

/* Bit fields for USART RXDATA */
#define _USART_RXDATA_RESETVALUE                0x00000000UL                        /**< Default value for USART_RXDATA */
#define _USART_RXDATA_MASK                      0x000000FFUL                        /**< Mask for USART_RXDATA */
#define _USART_RXDATA_RXDATA_SHIFT              0                                   /**< Shift value for USART_RXDATA */
#define _USART_RXDATA_RXDATA_MASK               0xFFUL                              /**< Bit mask for USART_RXDATA */
#define _USART_RXDATA_RXDATA_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for USART_RXDATA */
#define USART_RXDATA_RXDATA_DEFAULT             (_USART_RXDATA_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_RXDATA */

/* Bit fields for USART RXDOUBLEX */
#define _USART_RXDOUBLEX_RESETVALUE             0x00000000UL                             /**< Default value for USART_RXDOUBLEX */
#define _USART_RXDOUBLEX_MASK                   0xC1FFC1FFUL                             /**< Mask for USART_RXDOUBLEX */
#define _USART_RXDOUBLEX_RXDATA0_SHIFT          0                                        /**< Shift value for USART_RXDATA0 */
#define _USART_RXDOUBLEX_RXDATA0_MASK           0x1FFUL                                  /**< Bit mask for USART_RXDATA0 */
#define _USART_RXDOUBLEX_RXDATA0_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_RXDATA0_DEFAULT         (_USART_RXDOUBLEX_RXDATA0_DEFAULT << 0)  /**< Shifted mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_PERR0                   (0x1UL << 14)                            /**< Data Parity Error 0 */
#define _USART_RXDOUBLEX_PERR0_SHIFT            14                                       /**< Shift value for USART_PERR0 */
#define _USART_RXDOUBLEX_PERR0_MASK             0x4000UL                                 /**< Bit mask for USART_PERR0 */
#define _USART_RXDOUBLEX_PERR0_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_PERR0_DEFAULT           (_USART_RXDOUBLEX_PERR0_DEFAULT << 14)   /**< Shifted mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_FERR0                   (0x1UL << 15)                            /**< Data Framing Error 0 */
#define _USART_RXDOUBLEX_FERR0_SHIFT            15                                       /**< Shift value for USART_FERR0 */
#define _USART_RXDOUBLEX_FERR0_MASK             0x8000UL                                 /**< Bit mask for USART_FERR0 */
#define _USART_RXDOUBLEX_FERR0_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_FERR0_DEFAULT           (_USART_RXDOUBLEX_FERR0_DEFAULT << 15)   /**< Shifted mode DEFAULT for USART_RXDOUBLEX */
#define _USART_RXDOUBLEX_RXDATA1_SHIFT          16                                       /**< Shift value for USART_RXDATA1 */
#define _USART_RXDOUBLEX_RXDATA1_MASK           0x1FF0000UL                              /**< Bit mask for USART_RXDATA1 */
#define _USART_RXDOUBLEX_RXDATA1_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_RXDATA1_DEFAULT         (_USART_RXDOUBLEX_RXDATA1_DEFAULT << 16) /**< Shifted mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_PERR1                   (0x1UL << 30)                            /**< Data Parity Error 1 */
#define _USART_RXDOUBLEX_PERR1_SHIFT            30                                       /**< Shift value for USART_PERR1 */
#define _USART_RXDOUBLEX_PERR1_MASK             0x40000000UL                             /**< Bit mask for USART_PERR1 */
#define _USART_RXDOUBLEX_PERR1_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_PERR1_DEFAULT           (_USART_RXDOUBLEX_PERR1_DEFAULT << 30)   /**< Shifted mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_FERR1                   (0x1UL << 31)                            /**< Data Framing Error 1 */
#define _USART_RXDOUBLEX_FERR1_SHIFT            31                                       /**< Shift value for USART_FERR1 */
#define _USART_RXDOUBLEX_FERR1_MASK             0x80000000UL                             /**< Bit mask for USART_FERR1 */
#define _USART_RXDOUBLEX_FERR1_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for USART_RXDOUBLEX */
#define USART_RXDOUBLEX_FERR1_DEFAULT           (_USART_RXDOUBLEX_FERR1_DEFAULT << 31)   /**< Shifted mode DEFAULT for USART_RXDOUBLEX */

/* Bit fields for USART RXDOUBLE */
#define _USART_RXDOUBLE_RESETVALUE              0x00000000UL                           /**< Default value for USART_RXDOUBLE */
#define _USART_RXDOUBLE_MASK                    0x0000FFFFUL                           /**< Mask for USART_RXDOUBLE */
#define _USART_RXDOUBLE_RXDATA0_SHIFT           0                                      /**< Shift value for USART_RXDATA0 */
#define _USART_RXDOUBLE_RXDATA0_MASK            0xFFUL                                 /**< Bit mask for USART_RXDATA0 */
#define _USART_RXDOUBLE_RXDATA0_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_RXDOUBLE */
#define USART_RXDOUBLE_RXDATA0_DEFAULT          (_USART_RXDOUBLE_RXDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_RXDOUBLE */
#define _USART_RXDOUBLE_RXDATA1_SHIFT           8                                      /**< Shift value for USART_RXDATA1 */
#define _USART_RXDOUBLE_RXDATA1_MASK            0xFF00UL                               /**< Bit mask for USART_RXDATA1 */
#define _USART_RXDOUBLE_RXDATA1_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_RXDOUBLE */
#define USART_RXDOUBLE_RXDATA1_DEFAULT          (_USART_RXDOUBLE_RXDATA1_DEFAULT << 8) /**< Shifted mode DEFAULT for USART_RXDOUBLE */

/* Bit fields for USART RXDATAXP */
#define _USART_RXDATAXP_RESETVALUE              0x00000000UL                           /**< Default value for USART_RXDATAXP */
#define _USART_RXDATAXP_MASK                    0x0000C1FFUL                           /**< Mask for USART_RXDATAXP */
#define _USART_RXDATAXP_RXDATAP_SHIFT           0                                      /**< Shift value for USART_RXDATAP */
#define _USART_RXDATAXP_RXDATAP_MASK            0x1FFUL                                /**< Bit mask for USART_RXDATAP */
#define _USART_RXDATAXP_RXDATAP_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_RXDATAXP */
#define USART_RXDATAXP_RXDATAP_DEFAULT          (_USART_RXDATAXP_RXDATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_RXDATAXP */
#define USART_RXDATAXP_PERRP                    (0x1UL << 14)                          /**< Data Parity Error Peek */
#define _USART_RXDATAXP_PERRP_SHIFT             14                                     /**< Shift value for USART_PERRP */
#define _USART_RXDATAXP_PERRP_MASK              0x4000UL                               /**< Bit mask for USART_PERRP */
#define _USART_RXDATAXP_PERRP_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for USART_RXDATAXP */
#define USART_RXDATAXP_PERRP_DEFAULT            (_USART_RXDATAXP_PERRP_DEFAULT << 14)  /**< Shifted mode DEFAULT for USART_RXDATAXP */
#define USART_RXDATAXP_FERRP                    (0x1UL << 15)                          /**< Data Framing Error Peek */
#define _USART_RXDATAXP_FERRP_SHIFT             15                                     /**< Shift value for USART_FERRP */
#define _USART_RXDATAXP_FERRP_MASK              0x8000UL                               /**< Bit mask for USART_FERRP */
#define _USART_RXDATAXP_FERRP_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for USART_RXDATAXP */
#define USART_RXDATAXP_FERRP_DEFAULT            (_USART_RXDATAXP_FERRP_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_RXDATAXP */

/* Bit fields for USART RXDOUBLEXP */
#define _USART_RXDOUBLEXP_RESETVALUE            0x00000000UL                               /**< Default value for USART_RXDOUBLEXP */
#define _USART_RXDOUBLEXP_MASK                  0xC1FFC1FFUL                               /**< Mask for USART_RXDOUBLEXP */
#define _USART_RXDOUBLEXP_RXDATAP0_SHIFT        0                                          /**< Shift value for USART_RXDATAP0 */
#define _USART_RXDOUBLEXP_RXDATAP0_MASK         0x1FFUL                                    /**< Bit mask for USART_RXDATAP0 */
#define _USART_RXDOUBLEXP_RXDATAP0_DEFAULT      0x00000000UL                               /**< Mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_RXDATAP0_DEFAULT       (_USART_RXDOUBLEXP_RXDATAP0_DEFAULT << 0)  /**< Shifted mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_PERRP0                 (0x1UL << 14)                              /**< Data Parity Error 0 Peek */
#define _USART_RXDOUBLEXP_PERRP0_SHIFT          14                                         /**< Shift value for USART_PERRP0 */
#define _USART_RXDOUBLEXP_PERRP0_MASK           0x4000UL                                   /**< Bit mask for USART_PERRP0 */
#define _USART_RXDOUBLEXP_PERRP0_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_PERRP0_DEFAULT         (_USART_RXDOUBLEXP_PERRP0_DEFAULT << 14)   /**< Shifted mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_FERRP0                 (0x1UL << 15)                              /**< Data Framing Error 0 Peek */
#define _USART_RXDOUBLEXP_FERRP0_SHIFT          15                                         /**< Shift value for USART_FERRP0 */
#define _USART_RXDOUBLEXP_FERRP0_MASK           0x8000UL                                   /**< Bit mask for USART_FERRP0 */
#define _USART_RXDOUBLEXP_FERRP0_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_FERRP0_DEFAULT         (_USART_RXDOUBLEXP_FERRP0_DEFAULT << 15)   /**< Shifted mode DEFAULT for USART_RXDOUBLEXP */
#define _USART_RXDOUBLEXP_RXDATAP1_SHIFT        16                                         /**< Shift value for USART_RXDATAP1 */
#define _USART_RXDOUBLEXP_RXDATAP1_MASK         0x1FF0000UL                                /**< Bit mask for USART_RXDATAP1 */
#define _USART_RXDOUBLEXP_RXDATAP1_DEFAULT      0x00000000UL                               /**< Mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_RXDATAP1_DEFAULT       (_USART_RXDOUBLEXP_RXDATAP1_DEFAULT << 16) /**< Shifted mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_PERRP1                 (0x1UL << 30)                              /**< Data Parity Error 1 Peek */
#define _USART_RXDOUBLEXP_PERRP1_SHIFT          30                                         /**< Shift value for USART_PERRP1 */
#define _USART_RXDOUBLEXP_PERRP1_MASK           0x40000000UL                               /**< Bit mask for USART_PERRP1 */
#define _USART_RXDOUBLEXP_PERRP1_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_PERRP1_DEFAULT         (_USART_RXDOUBLEXP_PERRP1_DEFAULT << 30)   /**< Shifted mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_FERRP1                 (0x1UL << 31)                              /**< Data Framing Error 1 Peek */
#define _USART_RXDOUBLEXP_FERRP1_SHIFT          31                                         /**< Shift value for USART_FERRP1 */
#define _USART_RXDOUBLEXP_FERRP1_MASK           0x80000000UL                               /**< Bit mask for USART_FERRP1 */
#define _USART_RXDOUBLEXP_FERRP1_DEFAULT        0x00000000UL                               /**< Mode DEFAULT for USART_RXDOUBLEXP */
#define USART_RXDOUBLEXP_FERRP1_DEFAULT         (_USART_RXDOUBLEXP_FERRP1_DEFAULT << 31)   /**< Shifted mode DEFAULT for USART_RXDOUBLEXP */

/* Bit fields for USART TXDATAX */
#define _USART_TXDATAX_RESETVALUE               0x00000000UL                           /**< Default value for USART_TXDATAX */
#define _USART_TXDATAX_MASK                     0x0000F9FFUL                           /**< Mask for USART_TXDATAX */
#define _USART_TXDATAX_TXDATAX_SHIFT            0                                      /**< Shift value for USART_TXDATAX */
#define _USART_TXDATAX_TXDATAX_MASK             0x1FFUL                                /**< Bit mask for USART_TXDATAX */
#define _USART_TXDATAX_TXDATAX_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_TXDATAX_DEFAULT           (_USART_TXDATAX_TXDATAX_DEFAULT << 0)  /**< Shifted mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_UBRXAT                    (0x1UL << 11)                          /**< Unblock RX After Transmission */
#define _USART_TXDATAX_UBRXAT_SHIFT             11                                     /**< Shift value for USART_UBRXAT */
#define _USART_TXDATAX_UBRXAT_MASK              0x800UL                                /**< Bit mask for USART_UBRXAT */
#define _USART_TXDATAX_UBRXAT_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_UBRXAT_DEFAULT            (_USART_TXDATAX_UBRXAT_DEFAULT << 11)  /**< Shifted mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_TXTRIAT                   (0x1UL << 12)                          /**< Set TXTRI After Transmission */
#define _USART_TXDATAX_TXTRIAT_SHIFT            12                                     /**< Shift value for USART_TXTRIAT */
#define _USART_TXDATAX_TXTRIAT_MASK             0x1000UL                               /**< Bit mask for USART_TXTRIAT */
#define _USART_TXDATAX_TXTRIAT_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_TXTRIAT_DEFAULT           (_USART_TXDATAX_TXTRIAT_DEFAULT << 12) /**< Shifted mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_TXBREAK                   (0x1UL << 13)                          /**< Transmit Data As Break */
#define _USART_TXDATAX_TXBREAK_SHIFT            13                                     /**< Shift value for USART_TXBREAK */
#define _USART_TXDATAX_TXBREAK_MASK             0x2000UL                               /**< Bit mask for USART_TXBREAK */
#define _USART_TXDATAX_TXBREAK_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_TXBREAK_DEFAULT           (_USART_TXDATAX_TXBREAK_DEFAULT << 13) /**< Shifted mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_TXDISAT                   (0x1UL << 14)                          /**< Clear TXEN After Transmission */
#define _USART_TXDATAX_TXDISAT_SHIFT            14                                     /**< Shift value for USART_TXDISAT */
#define _USART_TXDATAX_TXDISAT_MASK             0x4000UL                               /**< Bit mask for USART_TXDISAT */
#define _USART_TXDATAX_TXDISAT_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_TXDISAT_DEFAULT           (_USART_TXDATAX_TXDISAT_DEFAULT << 14) /**< Shifted mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_RXENAT                    (0x1UL << 15)                          /**< Enable RX After Transmission */
#define _USART_TXDATAX_RXENAT_SHIFT             15                                     /**< Shift value for USART_RXENAT */
#define _USART_TXDATAX_RXENAT_MASK              0x8000UL                               /**< Bit mask for USART_RXENAT */
#define _USART_TXDATAX_RXENAT_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for USART_TXDATAX */
#define USART_TXDATAX_RXENAT_DEFAULT            (_USART_TXDATAX_RXENAT_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_TXDATAX */

/* Bit fields for USART TXDATA */
#define _USART_TXDATA_RESETVALUE                0x00000000UL                        /**< Default value for USART_TXDATA */
#define _USART_TXDATA_MASK                      0x000000FFUL                        /**< Mask for USART_TXDATA */
#define _USART_TXDATA_TXDATA_SHIFT              0                                   /**< Shift value for USART_TXDATA */
#define _USART_TXDATA_TXDATA_MASK               0xFFUL                              /**< Bit mask for USART_TXDATA */
#define _USART_TXDATA_TXDATA_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for USART_TXDATA */
#define USART_TXDATA_TXDATA_DEFAULT             (_USART_TXDATA_TXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_TXDATA */

/* Bit fields for USART TXDOUBLEX */
#define _USART_TXDOUBLEX_RESETVALUE             0x00000000UL                              /**< Default value for USART_TXDOUBLEX */
#define _USART_TXDOUBLEX_MASK                   0xF9FFF9FFUL                              /**< Mask for USART_TXDOUBLEX */
#define _USART_TXDOUBLEX_TXDATA0_SHIFT          0                                         /**< Shift value for USART_TXDATA0 */
#define _USART_TXDOUBLEX_TXDATA0_MASK           0x1FFUL                                   /**< Bit mask for USART_TXDATA0 */
#define _USART_TXDOUBLEX_TXDATA0_DEFAULT        0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXDATA0_DEFAULT         (_USART_TXDOUBLEX_TXDATA0_DEFAULT << 0)   /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_UBRXAT0                 (0x1UL << 11)                             /**< Unblock RX After Transmission */
#define _USART_TXDOUBLEX_UBRXAT0_SHIFT          11                                        /**< Shift value for USART_UBRXAT0 */
#define _USART_TXDOUBLEX_UBRXAT0_MASK           0x800UL                                   /**< Bit mask for USART_UBRXAT0 */
#define _USART_TXDOUBLEX_UBRXAT0_DEFAULT        0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_UBRXAT0_DEFAULT         (_USART_TXDOUBLEX_UBRXAT0_DEFAULT << 11)  /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXTRIAT0                (0x1UL << 12)                             /**< Set TXTRI After Transmission */
#define _USART_TXDOUBLEX_TXTRIAT0_SHIFT         12                                        /**< Shift value for USART_TXTRIAT0 */
#define _USART_TXDOUBLEX_TXTRIAT0_MASK          0x1000UL                                  /**< Bit mask for USART_TXTRIAT0 */
#define _USART_TXDOUBLEX_TXTRIAT0_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXTRIAT0_DEFAULT        (_USART_TXDOUBLEX_TXTRIAT0_DEFAULT << 12) /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXBREAK0                (0x1UL << 13)                             /**< Transmit Data As Break */
#define _USART_TXDOUBLEX_TXBREAK0_SHIFT         13                                        /**< Shift value for USART_TXBREAK0 */
#define _USART_TXDOUBLEX_TXBREAK0_MASK          0x2000UL                                  /**< Bit mask for USART_TXBREAK0 */
#define _USART_TXDOUBLEX_TXBREAK0_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXBREAK0_DEFAULT        (_USART_TXDOUBLEX_TXBREAK0_DEFAULT << 13) /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXDISAT0                (0x1UL << 14)                             /**< Clear TXEN After Transmission */
#define _USART_TXDOUBLEX_TXDISAT0_SHIFT         14                                        /**< Shift value for USART_TXDISAT0 */
#define _USART_TXDOUBLEX_TXDISAT0_MASK          0x4000UL                                  /**< Bit mask for USART_TXDISAT0 */
#define _USART_TXDOUBLEX_TXDISAT0_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXDISAT0_DEFAULT        (_USART_TXDOUBLEX_TXDISAT0_DEFAULT << 14) /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_RXENAT0                 (0x1UL << 15)                             /**< Enable RX After Transmission */
#define _USART_TXDOUBLEX_RXENAT0_SHIFT          15                                        /**< Shift value for USART_RXENAT0 */
#define _USART_TXDOUBLEX_RXENAT0_MASK           0x8000UL                                  /**< Bit mask for USART_RXENAT0 */
#define _USART_TXDOUBLEX_RXENAT0_DEFAULT        0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_RXENAT0_DEFAULT         (_USART_TXDOUBLEX_RXENAT0_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define _USART_TXDOUBLEX_TXDATA1_SHIFT          16                                        /**< Shift value for USART_TXDATA1 */
#define _USART_TXDOUBLEX_TXDATA1_MASK           0x1FF0000UL                               /**< Bit mask for USART_TXDATA1 */
#define _USART_TXDOUBLEX_TXDATA1_DEFAULT        0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXDATA1_DEFAULT         (_USART_TXDOUBLEX_TXDATA1_DEFAULT << 16)  /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_UBRXAT1                 (0x1UL << 27)                             /**< Unblock RX After Transmission */
#define _USART_TXDOUBLEX_UBRXAT1_SHIFT          27                                        /**< Shift value for USART_UBRXAT1 */
#define _USART_TXDOUBLEX_UBRXAT1_MASK           0x8000000UL                               /**< Bit mask for USART_UBRXAT1 */
#define _USART_TXDOUBLEX_UBRXAT1_DEFAULT        0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_UBRXAT1_DEFAULT         (_USART_TXDOUBLEX_UBRXAT1_DEFAULT << 27)  /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXTRIAT1                (0x1UL << 28)                             /**< Set TXTRI After Transmission */
#define _USART_TXDOUBLEX_TXTRIAT1_SHIFT         28                                        /**< Shift value for USART_TXTRIAT1 */
#define _USART_TXDOUBLEX_TXTRIAT1_MASK          0x10000000UL                              /**< Bit mask for USART_TXTRIAT1 */
#define _USART_TXDOUBLEX_TXTRIAT1_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXTRIAT1_DEFAULT        (_USART_TXDOUBLEX_TXTRIAT1_DEFAULT << 28) /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXBREAK1                (0x1UL << 29)                             /**< Transmit Data As Break */
#define _USART_TXDOUBLEX_TXBREAK1_SHIFT         29                                        /**< Shift value for USART_TXBREAK1 */
#define _USART_TXDOUBLEX_TXBREAK1_MASK          0x20000000UL                              /**< Bit mask for USART_TXBREAK1 */
#define _USART_TXDOUBLEX_TXBREAK1_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXBREAK1_DEFAULT        (_USART_TXDOUBLEX_TXBREAK1_DEFAULT << 29) /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXDISAT1                (0x1UL << 30)                             /**< Clear TXEN After Transmission */
#define _USART_TXDOUBLEX_TXDISAT1_SHIFT         30                                        /**< Shift value for USART_TXDISAT1 */
#define _USART_TXDOUBLEX_TXDISAT1_MASK          0x40000000UL                              /**< Bit mask for USART_TXDISAT1 */
#define _USART_TXDOUBLEX_TXDISAT1_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_TXDISAT1_DEFAULT        (_USART_TXDOUBLEX_TXDISAT1_DEFAULT << 30) /**< Shifted mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_RXENAT1                 (0x1UL << 31)                             /**< Enable RX After Transmission */
#define _USART_TXDOUBLEX_RXENAT1_SHIFT          31                                        /**< Shift value for USART_RXENAT1 */
#define _USART_TXDOUBLEX_RXENAT1_MASK           0x80000000UL                              /**< Bit mask for USART_RXENAT1 */
#define _USART_TXDOUBLEX_RXENAT1_DEFAULT        0x00000000UL                              /**< Mode DEFAULT for USART_TXDOUBLEX */
#define USART_TXDOUBLEX_RXENAT1_DEFAULT         (_USART_TXDOUBLEX_RXENAT1_DEFAULT << 31)  /**< Shifted mode DEFAULT for USART_TXDOUBLEX */

/* Bit fields for USART TXDOUBLE */
#define _USART_TXDOUBLE_RESETVALUE              0x00000000UL                           /**< Default value for USART_TXDOUBLE */
#define _USART_TXDOUBLE_MASK                    0x0000FFFFUL                           /**< Mask for USART_TXDOUBLE */
#define _USART_TXDOUBLE_TXDATA0_SHIFT           0                                      /**< Shift value for USART_TXDATA0 */
#define _USART_TXDOUBLE_TXDATA0_MASK            0xFFUL                                 /**< Bit mask for USART_TXDATA0 */
#define _USART_TXDOUBLE_TXDATA0_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_TXDOUBLE */
#define USART_TXDOUBLE_TXDATA0_DEFAULT          (_USART_TXDOUBLE_TXDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_TXDOUBLE */
#define _USART_TXDOUBLE_TXDATA1_SHIFT           8                                      /**< Shift value for USART_TXDATA1 */
#define _USART_TXDOUBLE_TXDATA1_MASK            0xFF00UL                               /**< Bit mask for USART_TXDATA1 */
#define _USART_TXDOUBLE_TXDATA1_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_TXDOUBLE */
#define USART_TXDOUBLE_TXDATA1_DEFAULT          (_USART_TXDOUBLE_TXDATA1_DEFAULT << 8) /**< Shifted mode DEFAULT for USART_TXDOUBLE */

/* Bit fields for USART IF */
#define _USART_IF_RESETVALUE                    0x00000002UL                     /**< Default value for USART_IF */
#define _USART_IF_MASK                          0x0001FFFFUL                     /**< Mask for USART_IF */
#define USART_IF_TXC                            (0x1UL << 0)                     /**< TX Complete Interrupt Flag */
#define _USART_IF_TXC_SHIFT                     0                                /**< Shift value for USART_TXC */
#define _USART_IF_TXC_MASK                      0x1UL                            /**< Bit mask for USART_TXC */
#define _USART_IF_TXC_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TXC_DEFAULT                    (_USART_IF_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_TXBL                           (0x1UL << 1)                     /**< TX Buffer Level Interrupt Flag */
#define _USART_IF_TXBL_SHIFT                    1                                /**< Shift value for USART_TXBL */
#define _USART_IF_TXBL_MASK                     0x2UL                            /**< Bit mask for USART_TXBL */
#define _USART_IF_TXBL_DEFAULT                  0x00000001UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TXBL_DEFAULT                   (_USART_IF_TXBL_DEFAULT << 1)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_RXDATAV                        (0x1UL << 2)                     /**< RX Data Valid Interrupt Flag */
#define _USART_IF_RXDATAV_SHIFT                 2                                /**< Shift value for USART_RXDATAV */
#define _USART_IF_RXDATAV_MASK                  0x4UL                            /**< Bit mask for USART_RXDATAV */
#define _USART_IF_RXDATAV_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_RXDATAV_DEFAULT                (_USART_IF_RXDATAV_DEFAULT << 2) /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_RXFULL                         (0x1UL << 3)                     /**< RX Buffer Full Interrupt Flag */
#define _USART_IF_RXFULL_SHIFT                  3                                /**< Shift value for USART_RXFULL */
#define _USART_IF_RXFULL_MASK                   0x8UL                            /**< Bit mask for USART_RXFULL */
#define _USART_IF_RXFULL_DEFAULT                0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_RXFULL_DEFAULT                 (_USART_IF_RXFULL_DEFAULT << 3)  /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_RXOF                           (0x1UL << 4)                     /**< RX Overflow Interrupt Flag */
#define _USART_IF_RXOF_SHIFT                    4                                /**< Shift value for USART_RXOF */
#define _USART_IF_RXOF_MASK                     0x10UL                           /**< Bit mask for USART_RXOF */
#define _USART_IF_RXOF_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_RXOF_DEFAULT                   (_USART_IF_RXOF_DEFAULT << 4)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_RXUF                           (0x1UL << 5)                     /**< RX Underflow Interrupt Flag */
#define _USART_IF_RXUF_SHIFT                    5                                /**< Shift value for USART_RXUF */
#define _USART_IF_RXUF_MASK                     0x20UL                           /**< Bit mask for USART_RXUF */
#define _USART_IF_RXUF_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_RXUF_DEFAULT                   (_USART_IF_RXUF_DEFAULT << 5)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_TXOF                           (0x1UL << 6)                     /**< TX Overflow Interrupt Flag */
#define _USART_IF_TXOF_SHIFT                    6                                /**< Shift value for USART_TXOF */
#define _USART_IF_TXOF_MASK                     0x40UL                           /**< Bit mask for USART_TXOF */
#define _USART_IF_TXOF_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TXOF_DEFAULT                   (_USART_IF_TXOF_DEFAULT << 6)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_TXUF                           (0x1UL << 7)                     /**< TX Underflow Interrupt Flag */
#define _USART_IF_TXUF_SHIFT                    7                                /**< Shift value for USART_TXUF */
#define _USART_IF_TXUF_MASK                     0x80UL                           /**< Bit mask for USART_TXUF */
#define _USART_IF_TXUF_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TXUF_DEFAULT                   (_USART_IF_TXUF_DEFAULT << 7)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_PERR                           (0x1UL << 8)                     /**< Parity Error Interrupt Flag */
#define _USART_IF_PERR_SHIFT                    8                                /**< Shift value for USART_PERR */
#define _USART_IF_PERR_MASK                     0x100UL                          /**< Bit mask for USART_PERR */
#define _USART_IF_PERR_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_PERR_DEFAULT                   (_USART_IF_PERR_DEFAULT << 8)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_FERR                           (0x1UL << 9)                     /**< Framing Error Interrupt Flag */
#define _USART_IF_FERR_SHIFT                    9                                /**< Shift value for USART_FERR */
#define _USART_IF_FERR_MASK                     0x200UL                          /**< Bit mask for USART_FERR */
#define _USART_IF_FERR_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_FERR_DEFAULT                   (_USART_IF_FERR_DEFAULT << 9)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_MPAF                           (0x1UL << 10)                    /**< Multi-Processor Address Frame Interrupt Flag */
#define _USART_IF_MPAF_SHIFT                    10                               /**< Shift value for USART_MPAF */
#define _USART_IF_MPAF_MASK                     0x400UL                          /**< Bit mask for USART_MPAF */
#define _USART_IF_MPAF_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_MPAF_DEFAULT                   (_USART_IF_MPAF_DEFAULT << 10)   /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_SSM                            (0x1UL << 11)                    /**< Slave-Select In Master Mode Interrupt Flag */
#define _USART_IF_SSM_SHIFT                     11                               /**< Shift value for USART_SSM */
#define _USART_IF_SSM_MASK                      0x800UL                          /**< Bit mask for USART_SSM */
#define _USART_IF_SSM_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_SSM_DEFAULT                    (_USART_IF_SSM_DEFAULT << 11)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_CCF                            (0x1UL << 12)                    /**< Collision Check Fail Interrupt Flag */
#define _USART_IF_CCF_SHIFT                     12                               /**< Shift value for USART_CCF */
#define _USART_IF_CCF_MASK                      0x1000UL                         /**< Bit mask for USART_CCF */
#define _USART_IF_CCF_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_CCF_DEFAULT                    (_USART_IF_CCF_DEFAULT << 12)    /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_TXIDLE                         (0x1UL << 13)                    /**< TX Idle Interrupt Flag */
#define _USART_IF_TXIDLE_SHIFT                  13                               /**< Shift value for USART_TXIDLE */
#define _USART_IF_TXIDLE_MASK                   0x2000UL                         /**< Bit mask for USART_TXIDLE */
#define _USART_IF_TXIDLE_DEFAULT                0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TXIDLE_DEFAULT                 (_USART_IF_TXIDLE_DEFAULT << 13) /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_TCMP0                          (0x1UL << 14)                    /**< Timer comparator 0 Interrupt Flag */
#define _USART_IF_TCMP0_SHIFT                   14                               /**< Shift value for USART_TCMP0 */
#define _USART_IF_TCMP0_MASK                    0x4000UL                         /**< Bit mask for USART_TCMP0 */
#define _USART_IF_TCMP0_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TCMP0_DEFAULT                  (_USART_IF_TCMP0_DEFAULT << 14)  /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_TCMP1                          (0x1UL << 15)                    /**< Timer comparator 1 Interrupt Flag */
#define _USART_IF_TCMP1_SHIFT                   15                               /**< Shift value for USART_TCMP1 */
#define _USART_IF_TCMP1_MASK                    0x8000UL                         /**< Bit mask for USART_TCMP1 */
#define _USART_IF_TCMP1_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TCMP1_DEFAULT                  (_USART_IF_TCMP1_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_IF */
#define USART_IF_TCMP2                          (0x1UL << 16)                    /**< Timer comparator 2 Interrupt Flag */
#define _USART_IF_TCMP2_SHIFT                   16                               /**< Shift value for USART_TCMP2 */
#define _USART_IF_TCMP2_MASK                    0x10000UL                        /**< Bit mask for USART_TCMP2 */
#define _USART_IF_TCMP2_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for USART_IF */
#define USART_IF_TCMP2_DEFAULT                  (_USART_IF_TCMP2_DEFAULT << 16)  /**< Shifted mode DEFAULT for USART_IF */

/* Bit fields for USART IFS */
#define _USART_IFS_RESETVALUE                   0x00000000UL                      /**< Default value for USART_IFS */
#define _USART_IFS_MASK                         0x0001FFF9UL                      /**< Mask for USART_IFS */
#define USART_IFS_TXC                           (0x1UL << 0)                      /**< Set TXC Interrupt Flag */
#define _USART_IFS_TXC_SHIFT                    0                                 /**< Shift value for USART_TXC */
#define _USART_IFS_TXC_MASK                     0x1UL                             /**< Bit mask for USART_TXC */
#define _USART_IFS_TXC_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_TXC_DEFAULT                   (_USART_IFS_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_RXFULL                        (0x1UL << 3)                      /**< Set RXFULL Interrupt Flag */
#define _USART_IFS_RXFULL_SHIFT                 3                                 /**< Shift value for USART_RXFULL */
#define _USART_IFS_RXFULL_MASK                  0x8UL                             /**< Bit mask for USART_RXFULL */
#define _USART_IFS_RXFULL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_RXFULL_DEFAULT                (_USART_IFS_RXFULL_DEFAULT << 3)  /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_RXOF                          (0x1UL << 4)                      /**< Set RXOF Interrupt Flag */
#define _USART_IFS_RXOF_SHIFT                   4                                 /**< Shift value for USART_RXOF */
#define _USART_IFS_RXOF_MASK                    0x10UL                            /**< Bit mask for USART_RXOF */
#define _USART_IFS_RXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_RXOF_DEFAULT                  (_USART_IFS_RXOF_DEFAULT << 4)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_RXUF                          (0x1UL << 5)                      /**< Set RXUF Interrupt Flag */
#define _USART_IFS_RXUF_SHIFT                   5                                 /**< Shift value for USART_RXUF */
#define _USART_IFS_RXUF_MASK                    0x20UL                            /**< Bit mask for USART_RXUF */
#define _USART_IFS_RXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_RXUF_DEFAULT                  (_USART_IFS_RXUF_DEFAULT << 5)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_TXOF                          (0x1UL << 6)                      /**< Set TXOF Interrupt Flag */
#define _USART_IFS_TXOF_SHIFT                   6                                 /**< Shift value for USART_TXOF */
#define _USART_IFS_TXOF_MASK                    0x40UL                            /**< Bit mask for USART_TXOF */
#define _USART_IFS_TXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_TXOF_DEFAULT                  (_USART_IFS_TXOF_DEFAULT << 6)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_TXUF                          (0x1UL << 7)                      /**< Set TXUF Interrupt Flag */
#define _USART_IFS_TXUF_SHIFT                   7                                 /**< Shift value for USART_TXUF */
#define _USART_IFS_TXUF_MASK                    0x80UL                            /**< Bit mask for USART_TXUF */
#define _USART_IFS_TXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_TXUF_DEFAULT                  (_USART_IFS_TXUF_DEFAULT << 7)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_PERR                          (0x1UL << 8)                      /**< Set PERR Interrupt Flag */
#define _USART_IFS_PERR_SHIFT                   8                                 /**< Shift value for USART_PERR */
#define _USART_IFS_PERR_MASK                    0x100UL                           /**< Bit mask for USART_PERR */
#define _USART_IFS_PERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_PERR_DEFAULT                  (_USART_IFS_PERR_DEFAULT << 8)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_FERR                          (0x1UL << 9)                      /**< Set FERR Interrupt Flag */
#define _USART_IFS_FERR_SHIFT                   9                                 /**< Shift value for USART_FERR */
#define _USART_IFS_FERR_MASK                    0x200UL                           /**< Bit mask for USART_FERR */
#define _USART_IFS_FERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_FERR_DEFAULT                  (_USART_IFS_FERR_DEFAULT << 9)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_MPAF                          (0x1UL << 10)                     /**< Set MPAF Interrupt Flag */
#define _USART_IFS_MPAF_SHIFT                   10                                /**< Shift value for USART_MPAF */
#define _USART_IFS_MPAF_MASK                    0x400UL                           /**< Bit mask for USART_MPAF */
#define _USART_IFS_MPAF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_MPAF_DEFAULT                  (_USART_IFS_MPAF_DEFAULT << 10)   /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_SSM                           (0x1UL << 11)                     /**< Set SSM Interrupt Flag */
#define _USART_IFS_SSM_SHIFT                    11                                /**< Shift value for USART_SSM */
#define _USART_IFS_SSM_MASK                     0x800UL                           /**< Bit mask for USART_SSM */
#define _USART_IFS_SSM_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_SSM_DEFAULT                   (_USART_IFS_SSM_DEFAULT << 11)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_CCF                           (0x1UL << 12)                     /**< Set CCF Interrupt Flag */
#define _USART_IFS_CCF_SHIFT                    12                                /**< Shift value for USART_CCF */
#define _USART_IFS_CCF_MASK                     0x1000UL                          /**< Bit mask for USART_CCF */
#define _USART_IFS_CCF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_CCF_DEFAULT                   (_USART_IFS_CCF_DEFAULT << 12)    /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_TXIDLE                        (0x1UL << 13)                     /**< Set TXIDLE Interrupt Flag */
#define _USART_IFS_TXIDLE_SHIFT                 13                                /**< Shift value for USART_TXIDLE */
#define _USART_IFS_TXIDLE_MASK                  0x2000UL                          /**< Bit mask for USART_TXIDLE */
#define _USART_IFS_TXIDLE_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_TXIDLE_DEFAULT                (_USART_IFS_TXIDLE_DEFAULT << 13) /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_TCMP0                         (0x1UL << 14)                     /**< Set TCMP0 Interrupt Flag */
#define _USART_IFS_TCMP0_SHIFT                  14                                /**< Shift value for USART_TCMP0 */
#define _USART_IFS_TCMP0_MASK                   0x4000UL                          /**< Bit mask for USART_TCMP0 */
#define _USART_IFS_TCMP0_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_TCMP0_DEFAULT                 (_USART_IFS_TCMP0_DEFAULT << 14)  /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_TCMP1                         (0x1UL << 15)                     /**< Set TCMP1 Interrupt Flag */
#define _USART_IFS_TCMP1_SHIFT                  15                                /**< Shift value for USART_TCMP1 */
#define _USART_IFS_TCMP1_MASK                   0x8000UL                          /**< Bit mask for USART_TCMP1 */
#define _USART_IFS_TCMP1_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_TCMP1_DEFAULT                 (_USART_IFS_TCMP1_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_IFS */
#define USART_IFS_TCMP2                         (0x1UL << 16)                     /**< Set TCMP2 Interrupt Flag */
#define _USART_IFS_TCMP2_SHIFT                  16                                /**< Shift value for USART_TCMP2 */
#define _USART_IFS_TCMP2_MASK                   0x10000UL                         /**< Bit mask for USART_TCMP2 */
#define _USART_IFS_TCMP2_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IFS */
#define USART_IFS_TCMP2_DEFAULT                 (_USART_IFS_TCMP2_DEFAULT << 16)  /**< Shifted mode DEFAULT for USART_IFS */

/* Bit fields for USART IFC */
#define _USART_IFC_RESETVALUE                   0x00000000UL                      /**< Default value for USART_IFC */
#define _USART_IFC_MASK                         0x0001FFF9UL                      /**< Mask for USART_IFC */
#define USART_IFC_TXC                           (0x1UL << 0)                      /**< Clear TXC Interrupt Flag */
#define _USART_IFC_TXC_SHIFT                    0                                 /**< Shift value for USART_TXC */
#define _USART_IFC_TXC_MASK                     0x1UL                             /**< Bit mask for USART_TXC */
#define _USART_IFC_TXC_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_TXC_DEFAULT                   (_USART_IFC_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_RXFULL                        (0x1UL << 3)                      /**< Clear RXFULL Interrupt Flag */
#define _USART_IFC_RXFULL_SHIFT                 3                                 /**< Shift value for USART_RXFULL */
#define _USART_IFC_RXFULL_MASK                  0x8UL                             /**< Bit mask for USART_RXFULL */
#define _USART_IFC_RXFULL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_RXFULL_DEFAULT                (_USART_IFC_RXFULL_DEFAULT << 3)  /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_RXOF                          (0x1UL << 4)                      /**< Clear RXOF Interrupt Flag */
#define _USART_IFC_RXOF_SHIFT                   4                                 /**< Shift value for USART_RXOF */
#define _USART_IFC_RXOF_MASK                    0x10UL                            /**< Bit mask for USART_RXOF */
#define _USART_IFC_RXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_RXOF_DEFAULT                  (_USART_IFC_RXOF_DEFAULT << 4)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_RXUF                          (0x1UL << 5)                      /**< Clear RXUF Interrupt Flag */
#define _USART_IFC_RXUF_SHIFT                   5                                 /**< Shift value for USART_RXUF */
#define _USART_IFC_RXUF_MASK                    0x20UL                            /**< Bit mask for USART_RXUF */
#define _USART_IFC_RXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_RXUF_DEFAULT                  (_USART_IFC_RXUF_DEFAULT << 5)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_TXOF                          (0x1UL << 6)                      /**< Clear TXOF Interrupt Flag */
#define _USART_IFC_TXOF_SHIFT                   6                                 /**< Shift value for USART_TXOF */
#define _USART_IFC_TXOF_MASK                    0x40UL                            /**< Bit mask for USART_TXOF */
#define _USART_IFC_TXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_TXOF_DEFAULT                  (_USART_IFC_TXOF_DEFAULT << 6)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_TXUF                          (0x1UL << 7)                      /**< Clear TXUF Interrupt Flag */
#define _USART_IFC_TXUF_SHIFT                   7                                 /**< Shift value for USART_TXUF */
#define _USART_IFC_TXUF_MASK                    0x80UL                            /**< Bit mask for USART_TXUF */
#define _USART_IFC_TXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_TXUF_DEFAULT                  (_USART_IFC_TXUF_DEFAULT << 7)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_PERR                          (0x1UL << 8)                      /**< Clear PERR Interrupt Flag */
#define _USART_IFC_PERR_SHIFT                   8                                 /**< Shift value for USART_PERR */
#define _USART_IFC_PERR_MASK                    0x100UL                           /**< Bit mask for USART_PERR */
#define _USART_IFC_PERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_PERR_DEFAULT                  (_USART_IFC_PERR_DEFAULT << 8)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_FERR                          (0x1UL << 9)                      /**< Clear FERR Interrupt Flag */
#define _USART_IFC_FERR_SHIFT                   9                                 /**< Shift value for USART_FERR */
#define _USART_IFC_FERR_MASK                    0x200UL                           /**< Bit mask for USART_FERR */
#define _USART_IFC_FERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_FERR_DEFAULT                  (_USART_IFC_FERR_DEFAULT << 9)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_MPAF                          (0x1UL << 10)                     /**< Clear MPAF Interrupt Flag */
#define _USART_IFC_MPAF_SHIFT                   10                                /**< Shift value for USART_MPAF */
#define _USART_IFC_MPAF_MASK                    0x400UL                           /**< Bit mask for USART_MPAF */
#define _USART_IFC_MPAF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_MPAF_DEFAULT                  (_USART_IFC_MPAF_DEFAULT << 10)   /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_SSM                           (0x1UL << 11)                     /**< Clear SSM Interrupt Flag */
#define _USART_IFC_SSM_SHIFT                    11                                /**< Shift value for USART_SSM */
#define _USART_IFC_SSM_MASK                     0x800UL                           /**< Bit mask for USART_SSM */
#define _USART_IFC_SSM_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_SSM_DEFAULT                   (_USART_IFC_SSM_DEFAULT << 11)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_CCF                           (0x1UL << 12)                     /**< Clear CCF Interrupt Flag */
#define _USART_IFC_CCF_SHIFT                    12                                /**< Shift value for USART_CCF */
#define _USART_IFC_CCF_MASK                     0x1000UL                          /**< Bit mask for USART_CCF */
#define _USART_IFC_CCF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_CCF_DEFAULT                   (_USART_IFC_CCF_DEFAULT << 12)    /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_TXIDLE                        (0x1UL << 13)                     /**< Clear TXIDLE Interrupt Flag */
#define _USART_IFC_TXIDLE_SHIFT                 13                                /**< Shift value for USART_TXIDLE */
#define _USART_IFC_TXIDLE_MASK                  0x2000UL                          /**< Bit mask for USART_TXIDLE */
#define _USART_IFC_TXIDLE_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_TXIDLE_DEFAULT                (_USART_IFC_TXIDLE_DEFAULT << 13) /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_TCMP0                         (0x1UL << 14)                     /**< Clear TCMP0 Interrupt Flag */
#define _USART_IFC_TCMP0_SHIFT                  14                                /**< Shift value for USART_TCMP0 */
#define _USART_IFC_TCMP0_MASK                   0x4000UL                          /**< Bit mask for USART_TCMP0 */
#define _USART_IFC_TCMP0_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_TCMP0_DEFAULT                 (_USART_IFC_TCMP0_DEFAULT << 14)  /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_TCMP1                         (0x1UL << 15)                     /**< Clear TCMP1 Interrupt Flag */
#define _USART_IFC_TCMP1_SHIFT                  15                                /**< Shift value for USART_TCMP1 */
#define _USART_IFC_TCMP1_MASK                   0x8000UL                          /**< Bit mask for USART_TCMP1 */
#define _USART_IFC_TCMP1_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_TCMP1_DEFAULT                 (_USART_IFC_TCMP1_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_IFC */
#define USART_IFC_TCMP2                         (0x1UL << 16)                     /**< Clear TCMP2 Interrupt Flag */
#define _USART_IFC_TCMP2_SHIFT                  16                                /**< Shift value for USART_TCMP2 */
#define _USART_IFC_TCMP2_MASK                   0x10000UL                         /**< Bit mask for USART_TCMP2 */
#define _USART_IFC_TCMP2_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IFC */
#define USART_IFC_TCMP2_DEFAULT                 (_USART_IFC_TCMP2_DEFAULT << 16)  /**< Shifted mode DEFAULT for USART_IFC */

/* Bit fields for USART IEN */
#define _USART_IEN_RESETVALUE                   0x00000000UL                      /**< Default value for USART_IEN */
#define _USART_IEN_MASK                         0x0001FFFFUL                      /**< Mask for USART_IEN */
#define USART_IEN_TXC                           (0x1UL << 0)                      /**< TXC Interrupt Enable */
#define _USART_IEN_TXC_SHIFT                    0                                 /**< Shift value for USART_TXC */
#define _USART_IEN_TXC_MASK                     0x1UL                             /**< Bit mask for USART_TXC */
#define _USART_IEN_TXC_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TXC_DEFAULT                   (_USART_IEN_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_TXBL                          (0x1UL << 1)                      /**< TXBL Interrupt Enable */
#define _USART_IEN_TXBL_SHIFT                   1                                 /**< Shift value for USART_TXBL */
#define _USART_IEN_TXBL_MASK                    0x2UL                             /**< Bit mask for USART_TXBL */
#define _USART_IEN_TXBL_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TXBL_DEFAULT                  (_USART_IEN_TXBL_DEFAULT << 1)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_RXDATAV                       (0x1UL << 2)                      /**< RXDATAV Interrupt Enable */
#define _USART_IEN_RXDATAV_SHIFT                2                                 /**< Shift value for USART_RXDATAV */
#define _USART_IEN_RXDATAV_MASK                 0x4UL                             /**< Bit mask for USART_RXDATAV */
#define _USART_IEN_RXDATAV_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_RXDATAV_DEFAULT               (_USART_IEN_RXDATAV_DEFAULT << 2) /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_RXFULL                        (0x1UL << 3)                      /**< RXFULL Interrupt Enable */
#define _USART_IEN_RXFULL_SHIFT                 3                                 /**< Shift value for USART_RXFULL */
#define _USART_IEN_RXFULL_MASK                  0x8UL                             /**< Bit mask for USART_RXFULL */
#define _USART_IEN_RXFULL_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_RXFULL_DEFAULT                (_USART_IEN_RXFULL_DEFAULT << 3)  /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_RXOF                          (0x1UL << 4)                      /**< RXOF Interrupt Enable */
#define _USART_IEN_RXOF_SHIFT                   4                                 /**< Shift value for USART_RXOF */
#define _USART_IEN_RXOF_MASK                    0x10UL                            /**< Bit mask for USART_RXOF */
#define _USART_IEN_RXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_RXOF_DEFAULT                  (_USART_IEN_RXOF_DEFAULT << 4)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_RXUF                          (0x1UL << 5)                      /**< RXUF Interrupt Enable */
#define _USART_IEN_RXUF_SHIFT                   5                                 /**< Shift value for USART_RXUF */
#define _USART_IEN_RXUF_MASK                    0x20UL                            /**< Bit mask for USART_RXUF */
#define _USART_IEN_RXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_RXUF_DEFAULT                  (_USART_IEN_RXUF_DEFAULT << 5)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_TXOF                          (0x1UL << 6)                      /**< TXOF Interrupt Enable */
#define _USART_IEN_TXOF_SHIFT                   6                                 /**< Shift value for USART_TXOF */
#define _USART_IEN_TXOF_MASK                    0x40UL                            /**< Bit mask for USART_TXOF */
#define _USART_IEN_TXOF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TXOF_DEFAULT                  (_USART_IEN_TXOF_DEFAULT << 6)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_TXUF                          (0x1UL << 7)                      /**< TXUF Interrupt Enable */
#define _USART_IEN_TXUF_SHIFT                   7                                 /**< Shift value for USART_TXUF */
#define _USART_IEN_TXUF_MASK                    0x80UL                            /**< Bit mask for USART_TXUF */
#define _USART_IEN_TXUF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TXUF_DEFAULT                  (_USART_IEN_TXUF_DEFAULT << 7)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_PERR                          (0x1UL << 8)                      /**< PERR Interrupt Enable */
#define _USART_IEN_PERR_SHIFT                   8                                 /**< Shift value for USART_PERR */
#define _USART_IEN_PERR_MASK                    0x100UL                           /**< Bit mask for USART_PERR */
#define _USART_IEN_PERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_PERR_DEFAULT                  (_USART_IEN_PERR_DEFAULT << 8)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_FERR                          (0x1UL << 9)                      /**< FERR Interrupt Enable */
#define _USART_IEN_FERR_SHIFT                   9                                 /**< Shift value for USART_FERR */
#define _USART_IEN_FERR_MASK                    0x200UL                           /**< Bit mask for USART_FERR */
#define _USART_IEN_FERR_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_FERR_DEFAULT                  (_USART_IEN_FERR_DEFAULT << 9)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_MPAF                          (0x1UL << 10)                     /**< MPAF Interrupt Enable */
#define _USART_IEN_MPAF_SHIFT                   10                                /**< Shift value for USART_MPAF */
#define _USART_IEN_MPAF_MASK                    0x400UL                           /**< Bit mask for USART_MPAF */
#define _USART_IEN_MPAF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_MPAF_DEFAULT                  (_USART_IEN_MPAF_DEFAULT << 10)   /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_SSM                           (0x1UL << 11)                     /**< SSM Interrupt Enable */
#define _USART_IEN_SSM_SHIFT                    11                                /**< Shift value for USART_SSM */
#define _USART_IEN_SSM_MASK                     0x800UL                           /**< Bit mask for USART_SSM */
#define _USART_IEN_SSM_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_SSM_DEFAULT                   (_USART_IEN_SSM_DEFAULT << 11)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_CCF                           (0x1UL << 12)                     /**< CCF Interrupt Enable */
#define _USART_IEN_CCF_SHIFT                    12                                /**< Shift value for USART_CCF */
#define _USART_IEN_CCF_MASK                     0x1000UL                          /**< Bit mask for USART_CCF */
#define _USART_IEN_CCF_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_CCF_DEFAULT                   (_USART_IEN_CCF_DEFAULT << 12)    /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_TXIDLE                        (0x1UL << 13)                     /**< TXIDLE Interrupt Enable */
#define _USART_IEN_TXIDLE_SHIFT                 13                                /**< Shift value for USART_TXIDLE */
#define _USART_IEN_TXIDLE_MASK                  0x2000UL                          /**< Bit mask for USART_TXIDLE */
#define _USART_IEN_TXIDLE_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TXIDLE_DEFAULT                (_USART_IEN_TXIDLE_DEFAULT << 13) /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_TCMP0                         (0x1UL << 14)                     /**< TCMP0 Interrupt Enable */
#define _USART_IEN_TCMP0_SHIFT                  14                                /**< Shift value for USART_TCMP0 */
#define _USART_IEN_TCMP0_MASK                   0x4000UL                          /**< Bit mask for USART_TCMP0 */
#define _USART_IEN_TCMP0_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TCMP0_DEFAULT                 (_USART_IEN_TCMP0_DEFAULT << 14)  /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_TCMP1                         (0x1UL << 15)                     /**< TCMP1 Interrupt Enable */
#define _USART_IEN_TCMP1_SHIFT                  15                                /**< Shift value for USART_TCMP1 */
#define _USART_IEN_TCMP1_MASK                   0x8000UL                          /**< Bit mask for USART_TCMP1 */
#define _USART_IEN_TCMP1_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TCMP1_DEFAULT                 (_USART_IEN_TCMP1_DEFAULT << 15)  /**< Shifted mode DEFAULT for USART_IEN */
#define USART_IEN_TCMP2                         (0x1UL << 16)                     /**< TCMP2 Interrupt Enable */
#define _USART_IEN_TCMP2_SHIFT                  16                                /**< Shift value for USART_TCMP2 */
#define _USART_IEN_TCMP2_MASK                   0x10000UL                         /**< Bit mask for USART_TCMP2 */
#define _USART_IEN_TCMP2_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for USART_IEN */
#define USART_IEN_TCMP2_DEFAULT                 (_USART_IEN_TCMP2_DEFAULT << 16)  /**< Shifted mode DEFAULT for USART_IEN */

/* Bit fields for USART IRCTRL */
#define _USART_IRCTRL_RESETVALUE                0x00000000UL                          /**< Default value for USART_IRCTRL */
#define _USART_IRCTRL_MASK                      0x00000F8FUL                          /**< Mask for USART_IRCTRL */
#define USART_IRCTRL_IREN                       (0x1UL << 0)                          /**< Enable IrDA Module */
#define _USART_IRCTRL_IREN_SHIFT                0                                     /**< Shift value for USART_IREN */
#define _USART_IRCTRL_IREN_MASK                 0x1UL                                 /**< Bit mask for USART_IREN */
#define _USART_IRCTRL_IREN_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for USART_IRCTRL */
#define USART_IRCTRL_IREN_DEFAULT               (_USART_IRCTRL_IREN_DEFAULT << 0)     /**< Shifted mode DEFAULT for USART_IRCTRL */
#define _USART_IRCTRL_IRPW_SHIFT                1                                     /**< Shift value for USART_IRPW */
#define _USART_IRCTRL_IRPW_MASK                 0x6UL                                 /**< Bit mask for USART_IRPW */
#define _USART_IRCTRL_IRPW_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for USART_IRCTRL */
#define _USART_IRCTRL_IRPW_ONE                  0x00000000UL                          /**< Mode ONE for USART_IRCTRL */
#define _USART_IRCTRL_IRPW_TWO                  0x00000001UL                          /**< Mode TWO for USART_IRCTRL */
#define _USART_IRCTRL_IRPW_THREE                0x00000002UL                          /**< Mode THREE for USART_IRCTRL */
#define _USART_IRCTRL_IRPW_FOUR                 0x00000003UL                          /**< Mode FOUR for USART_IRCTRL */
#define USART_IRCTRL_IRPW_DEFAULT               (_USART_IRCTRL_IRPW_DEFAULT << 1)     /**< Shifted mode DEFAULT for USART_IRCTRL */
#define USART_IRCTRL_IRPW_ONE                   (_USART_IRCTRL_IRPW_ONE << 1)         /**< Shifted mode ONE for USART_IRCTRL */
#define USART_IRCTRL_IRPW_TWO                   (_USART_IRCTRL_IRPW_TWO << 1)         /**< Shifted mode TWO for USART_IRCTRL */
#define USART_IRCTRL_IRPW_THREE                 (_USART_IRCTRL_IRPW_THREE << 1)       /**< Shifted mode THREE for USART_IRCTRL */
#define USART_IRCTRL_IRPW_FOUR                  (_USART_IRCTRL_IRPW_FOUR << 1)        /**< Shifted mode FOUR for USART_IRCTRL */
#define USART_IRCTRL_IRFILT                     (0x1UL << 3)                          /**< IrDA RX Filter */
#define _USART_IRCTRL_IRFILT_SHIFT              3                                     /**< Shift value for USART_IRFILT */
#define _USART_IRCTRL_IRFILT_MASK               0x8UL                                 /**< Bit mask for USART_IRFILT */
#define _USART_IRCTRL_IRFILT_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for USART_IRCTRL */
#define USART_IRCTRL_IRFILT_DEFAULT             (_USART_IRCTRL_IRFILT_DEFAULT << 3)   /**< Shifted mode DEFAULT for USART_IRCTRL */
#define USART_IRCTRL_IRPRSEN                    (0x1UL << 7)                          /**< IrDA PRS Channel Enable */
#define _USART_IRCTRL_IRPRSEN_SHIFT             7                                     /**< Shift value for USART_IRPRSEN */
#define _USART_IRCTRL_IRPRSEN_MASK              0x80UL                                /**< Bit mask for USART_IRPRSEN */
#define _USART_IRCTRL_IRPRSEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for USART_IRCTRL */
#define USART_IRCTRL_IRPRSEN_DEFAULT            (_USART_IRCTRL_IRPRSEN_DEFAULT << 7)  /**< Shifted mode DEFAULT for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_SHIFT            8                                     /**< Shift value for USART_IRPRSSEL */
#define _USART_IRCTRL_IRPRSSEL_MASK             0xF00UL                               /**< Bit mask for USART_IRPRSSEL */
#define _USART_IRCTRL_IRPRSSEL_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH0           0x00000000UL                          /**< Mode PRSCH0 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH1           0x00000001UL                          /**< Mode PRSCH1 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH2           0x00000002UL                          /**< Mode PRSCH2 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH3           0x00000003UL                          /**< Mode PRSCH3 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH4           0x00000004UL                          /**< Mode PRSCH4 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH5           0x00000005UL                          /**< Mode PRSCH5 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH6           0x00000006UL                          /**< Mode PRSCH6 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH7           0x00000007UL                          /**< Mode PRSCH7 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH8           0x00000008UL                          /**< Mode PRSCH8 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH9           0x00000009UL                          /**< Mode PRSCH9 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH10          0x0000000AUL                          /**< Mode PRSCH10 for USART_IRCTRL */
#define _USART_IRCTRL_IRPRSSEL_PRSCH11          0x0000000BUL                          /**< Mode PRSCH11 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_DEFAULT           (_USART_IRCTRL_IRPRSSEL_DEFAULT << 8) /**< Shifted mode DEFAULT for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH0            (_USART_IRCTRL_IRPRSSEL_PRSCH0 << 8)  /**< Shifted mode PRSCH0 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH1            (_USART_IRCTRL_IRPRSSEL_PRSCH1 << 8)  /**< Shifted mode PRSCH1 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH2            (_USART_IRCTRL_IRPRSSEL_PRSCH2 << 8)  /**< Shifted mode PRSCH2 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH3            (_USART_IRCTRL_IRPRSSEL_PRSCH3 << 8)  /**< Shifted mode PRSCH3 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH4            (_USART_IRCTRL_IRPRSSEL_PRSCH4 << 8)  /**< Shifted mode PRSCH4 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH5            (_USART_IRCTRL_IRPRSSEL_PRSCH5 << 8)  /**< Shifted mode PRSCH5 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH6            (_USART_IRCTRL_IRPRSSEL_PRSCH6 << 8)  /**< Shifted mode PRSCH6 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH7            (_USART_IRCTRL_IRPRSSEL_PRSCH7 << 8)  /**< Shifted mode PRSCH7 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH8            (_USART_IRCTRL_IRPRSSEL_PRSCH8 << 8)  /**< Shifted mode PRSCH8 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH9            (_USART_IRCTRL_IRPRSSEL_PRSCH9 << 8)  /**< Shifted mode PRSCH9 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH10           (_USART_IRCTRL_IRPRSSEL_PRSCH10 << 8) /**< Shifted mode PRSCH10 for USART_IRCTRL */
#define USART_IRCTRL_IRPRSSEL_PRSCH11           (_USART_IRCTRL_IRPRSSEL_PRSCH11 << 8) /**< Shifted mode PRSCH11 for USART_IRCTRL */

/* Bit fields for USART INPUT */
#define _USART_INPUT_RESETVALUE                 0x00000000UL                          /**< Default value for USART_INPUT */
#define _USART_INPUT_MASK                       0x00008F8FUL                          /**< Mask for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_SHIFT             0                                     /**< Shift value for USART_RXPRSSEL */
#define _USART_INPUT_RXPRSSEL_MASK              0xFUL                                 /**< Bit mask for USART_RXPRSSEL */
#define _USART_INPUT_RXPRSSEL_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH0            0x00000000UL                          /**< Mode PRSCH0 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH1            0x00000001UL                          /**< Mode PRSCH1 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH2            0x00000002UL                          /**< Mode PRSCH2 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH3            0x00000003UL                          /**< Mode PRSCH3 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH4            0x00000004UL                          /**< Mode PRSCH4 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH5            0x00000005UL                          /**< Mode PRSCH5 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH6            0x00000006UL                          /**< Mode PRSCH6 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH7            0x00000007UL                          /**< Mode PRSCH7 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH8            0x00000008UL                          /**< Mode PRSCH8 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH9            0x00000009UL                          /**< Mode PRSCH9 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH10           0x0000000AUL                          /**< Mode PRSCH10 for USART_INPUT */
#define _USART_INPUT_RXPRSSEL_PRSCH11           0x0000000BUL                          /**< Mode PRSCH11 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_DEFAULT            (_USART_INPUT_RXPRSSEL_DEFAULT << 0)  /**< Shifted mode DEFAULT for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH0             (_USART_INPUT_RXPRSSEL_PRSCH0 << 0)   /**< Shifted mode PRSCH0 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH1             (_USART_INPUT_RXPRSSEL_PRSCH1 << 0)   /**< Shifted mode PRSCH1 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH2             (_USART_INPUT_RXPRSSEL_PRSCH2 << 0)   /**< Shifted mode PRSCH2 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH3             (_USART_INPUT_RXPRSSEL_PRSCH3 << 0)   /**< Shifted mode PRSCH3 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH4             (_USART_INPUT_RXPRSSEL_PRSCH4 << 0)   /**< Shifted mode PRSCH4 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH5             (_USART_INPUT_RXPRSSEL_PRSCH5 << 0)   /**< Shifted mode PRSCH5 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH6             (_USART_INPUT_RXPRSSEL_PRSCH6 << 0)   /**< Shifted mode PRSCH6 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH7             (_USART_INPUT_RXPRSSEL_PRSCH7 << 0)   /**< Shifted mode PRSCH7 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH8             (_USART_INPUT_RXPRSSEL_PRSCH8 << 0)   /**< Shifted mode PRSCH8 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH9             (_USART_INPUT_RXPRSSEL_PRSCH9 << 0)   /**< Shifted mode PRSCH9 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH10            (_USART_INPUT_RXPRSSEL_PRSCH10 << 0)  /**< Shifted mode PRSCH10 for USART_INPUT */
#define USART_INPUT_RXPRSSEL_PRSCH11            (_USART_INPUT_RXPRSSEL_PRSCH11 << 0)  /**< Shifted mode PRSCH11 for USART_INPUT */
#define USART_INPUT_RXPRS                       (0x1UL << 7)                          /**< PRS RX Enable */
#define _USART_INPUT_RXPRS_SHIFT                7                                     /**< Shift value for USART_RXPRS */
#define _USART_INPUT_RXPRS_MASK                 0x80UL                                /**< Bit mask for USART_RXPRS */
#define _USART_INPUT_RXPRS_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for USART_INPUT */
#define USART_INPUT_RXPRS_DEFAULT               (_USART_INPUT_RXPRS_DEFAULT << 7)     /**< Shifted mode DEFAULT for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_SHIFT            8                                     /**< Shift value for USART_CLKPRSSEL */
#define _USART_INPUT_CLKPRSSEL_MASK             0xF00UL                               /**< Bit mask for USART_CLKPRSSEL */
#define _USART_INPUT_CLKPRSSEL_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH0           0x00000000UL                          /**< Mode PRSCH0 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH1           0x00000001UL                          /**< Mode PRSCH1 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH2           0x00000002UL                          /**< Mode PRSCH2 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH3           0x00000003UL                          /**< Mode PRSCH3 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH4           0x00000004UL                          /**< Mode PRSCH4 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH5           0x00000005UL                          /**< Mode PRSCH5 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH6           0x00000006UL                          /**< Mode PRSCH6 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH7           0x00000007UL                          /**< Mode PRSCH7 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH8           0x00000008UL                          /**< Mode PRSCH8 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH9           0x00000009UL                          /**< Mode PRSCH9 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH10          0x0000000AUL                          /**< Mode PRSCH10 for USART_INPUT */
#define _USART_INPUT_CLKPRSSEL_PRSCH11          0x0000000BUL                          /**< Mode PRSCH11 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_DEFAULT           (_USART_INPUT_CLKPRSSEL_DEFAULT << 8) /**< Shifted mode DEFAULT for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH0            (_USART_INPUT_CLKPRSSEL_PRSCH0 << 8)  /**< Shifted mode PRSCH0 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH1            (_USART_INPUT_CLKPRSSEL_PRSCH1 << 8)  /**< Shifted mode PRSCH1 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH2            (_USART_INPUT_CLKPRSSEL_PRSCH2 << 8)  /**< Shifted mode PRSCH2 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH3            (_USART_INPUT_CLKPRSSEL_PRSCH3 << 8)  /**< Shifted mode PRSCH3 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH4            (_USART_INPUT_CLKPRSSEL_PRSCH4 << 8)  /**< Shifted mode PRSCH4 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH5            (_USART_INPUT_CLKPRSSEL_PRSCH5 << 8)  /**< Shifted mode PRSCH5 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH6            (_USART_INPUT_CLKPRSSEL_PRSCH6 << 8)  /**< Shifted mode PRSCH6 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH7            (_USART_INPUT_CLKPRSSEL_PRSCH7 << 8)  /**< Shifted mode PRSCH7 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH8            (_USART_INPUT_CLKPRSSEL_PRSCH8 << 8)  /**< Shifted mode PRSCH8 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH9            (_USART_INPUT_CLKPRSSEL_PRSCH9 << 8)  /**< Shifted mode PRSCH9 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH10           (_USART_INPUT_CLKPRSSEL_PRSCH10 << 8) /**< Shifted mode PRSCH10 for USART_INPUT */
#define USART_INPUT_CLKPRSSEL_PRSCH11           (_USART_INPUT_CLKPRSSEL_PRSCH11 << 8) /**< Shifted mode PRSCH11 for USART_INPUT */
#define USART_INPUT_CLKPRS                      (0x1UL << 15)                         /**< PRS CLK Enable */
#define _USART_INPUT_CLKPRS_SHIFT               15                                    /**< Shift value for USART_CLKPRS */
#define _USART_INPUT_CLKPRS_MASK                0x8000UL                              /**< Bit mask for USART_CLKPRS */
#define _USART_INPUT_CLKPRS_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for USART_INPUT */
#define USART_INPUT_CLKPRS_DEFAULT              (_USART_INPUT_CLKPRS_DEFAULT << 15)   /**< Shifted mode DEFAULT for USART_INPUT */

/* Bit fields for USART I2SCTRL */
#define _USART_I2SCTRL_RESETVALUE               0x00000000UL                           /**< Default value for USART_I2SCTRL */
#define _USART_I2SCTRL_MASK                     0x0000071FUL                           /**< Mask for USART_I2SCTRL */
#define USART_I2SCTRL_EN                        (0x1UL << 0)                           /**< Enable I2S Mode */
#define _USART_I2SCTRL_EN_SHIFT                 0                                      /**< Shift value for USART_EN */
#define _USART_I2SCTRL_EN_MASK                  0x1UL                                  /**< Bit mask for USART_EN */
#define _USART_I2SCTRL_EN_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_EN_DEFAULT                (_USART_I2SCTRL_EN_DEFAULT << 0)       /**< Shifted mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_MONO                      (0x1UL << 1)                           /**< Stero or Mono */
#define _USART_I2SCTRL_MONO_SHIFT               1                                      /**< Shift value for USART_MONO */
#define _USART_I2SCTRL_MONO_MASK                0x2UL                                  /**< Bit mask for USART_MONO */
#define _USART_I2SCTRL_MONO_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_MONO_DEFAULT              (_USART_I2SCTRL_MONO_DEFAULT << 1)     /**< Shifted mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_JUSTIFY                   (0x1UL << 2)                           /**< Justification of I2S Data */
#define _USART_I2SCTRL_JUSTIFY_SHIFT            2                                      /**< Shift value for USART_JUSTIFY */
#define _USART_I2SCTRL_JUSTIFY_MASK             0x4UL                                  /**< Bit mask for USART_JUSTIFY */
#define _USART_I2SCTRL_JUSTIFY_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for USART_I2SCTRL */
#define _USART_I2SCTRL_JUSTIFY_LEFT             0x00000000UL                           /**< Mode LEFT for USART_I2SCTRL */
#define _USART_I2SCTRL_JUSTIFY_RIGHT            0x00000001UL                           /**< Mode RIGHT for USART_I2SCTRL */
#define USART_I2SCTRL_JUSTIFY_DEFAULT           (_USART_I2SCTRL_JUSTIFY_DEFAULT << 2)  /**< Shifted mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_JUSTIFY_LEFT              (_USART_I2SCTRL_JUSTIFY_LEFT << 2)     /**< Shifted mode LEFT for USART_I2SCTRL */
#define USART_I2SCTRL_JUSTIFY_RIGHT             (_USART_I2SCTRL_JUSTIFY_RIGHT << 2)    /**< Shifted mode RIGHT for USART_I2SCTRL */
#define USART_I2SCTRL_DMASPLIT                  (0x1UL << 3)                           /**< Separate DMA Request For Left/Right Data */
#define _USART_I2SCTRL_DMASPLIT_SHIFT           3                                      /**< Shift value for USART_DMASPLIT */
#define _USART_I2SCTRL_DMASPLIT_MASK            0x8UL                                  /**< Bit mask for USART_DMASPLIT */
#define _USART_I2SCTRL_DMASPLIT_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_DMASPLIT_DEFAULT          (_USART_I2SCTRL_DMASPLIT_DEFAULT << 3) /**< Shifted mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_DELAY                     (0x1UL << 4)                           /**< Delay on I2S data */
#define _USART_I2SCTRL_DELAY_SHIFT              4                                      /**< Shift value for USART_DELAY */
#define _USART_I2SCTRL_DELAY_MASK               0x10UL                                 /**< Bit mask for USART_DELAY */
#define _USART_I2SCTRL_DELAY_DEFAULT            0x00000000UL                           /**< Mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_DELAY_DEFAULT             (_USART_I2SCTRL_DELAY_DEFAULT << 4)    /**< Shifted mode DEFAULT for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_SHIFT             8                                      /**< Shift value for USART_FORMAT */
#define _USART_I2SCTRL_FORMAT_MASK              0x700UL                                /**< Bit mask for USART_FORMAT */
#define _USART_I2SCTRL_FORMAT_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W32D32            0x00000000UL                           /**< Mode W32D32 for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W32D24M           0x00000001UL                           /**< Mode W32D24M for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W32D24            0x00000002UL                           /**< Mode W32D24 for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W32D16            0x00000003UL                           /**< Mode W32D16 for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W32D8             0x00000004UL                           /**< Mode W32D8 for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W16D16            0x00000005UL                           /**< Mode W16D16 for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W16D8             0x00000006UL                           /**< Mode W16D8 for USART_I2SCTRL */
#define _USART_I2SCTRL_FORMAT_W8D8              0x00000007UL                           /**< Mode W8D8 for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_DEFAULT            (_USART_I2SCTRL_FORMAT_DEFAULT << 8)   /**< Shifted mode DEFAULT for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W32D32             (_USART_I2SCTRL_FORMAT_W32D32 << 8)    /**< Shifted mode W32D32 for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W32D24M            (_USART_I2SCTRL_FORMAT_W32D24M << 8)   /**< Shifted mode W32D24M for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W32D24             (_USART_I2SCTRL_FORMAT_W32D24 << 8)    /**< Shifted mode W32D24 for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W32D16             (_USART_I2SCTRL_FORMAT_W32D16 << 8)    /**< Shifted mode W32D16 for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W32D8              (_USART_I2SCTRL_FORMAT_W32D8 << 8)     /**< Shifted mode W32D8 for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W16D16             (_USART_I2SCTRL_FORMAT_W16D16 << 8)    /**< Shifted mode W16D16 for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W16D8              (_USART_I2SCTRL_FORMAT_W16D8 << 8)     /**< Shifted mode W16D8 for USART_I2SCTRL */
#define USART_I2SCTRL_FORMAT_W8D8               (_USART_I2SCTRL_FORMAT_W8D8 << 8)      /**< Shifted mode W8D8 for USART_I2SCTRL */

/* Bit fields for USART TIMING */
#define _USART_TIMING_RESETVALUE                0x00000000UL                          /**< Default value for USART_TIMING */
#define _USART_TIMING_MASK                      0x77770000UL                          /**< Mask for USART_TIMING */
#define _USART_TIMING_TXDELAY_SHIFT             16                                    /**< Shift value for USART_TXDELAY */
#define _USART_TIMING_TXDELAY_MASK              0x70000UL                             /**< Bit mask for USART_TXDELAY */
#define _USART_TIMING_TXDELAY_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for USART_TIMING */
#define _USART_TIMING_TXDELAY_DISABLE           0x00000000UL                          /**< Mode DISABLE for USART_TIMING */
#define _USART_TIMING_TXDELAY_ONE               0x00000001UL                          /**< Mode ONE for USART_TIMING */
#define _USART_TIMING_TXDELAY_TWO               0x00000002UL                          /**< Mode TWO for USART_TIMING */
#define _USART_TIMING_TXDELAY_THREE             0x00000003UL                          /**< Mode THREE for USART_TIMING */
#define _USART_TIMING_TXDELAY_SEVEN             0x00000004UL                          /**< Mode SEVEN for USART_TIMING */
#define _USART_TIMING_TXDELAY_TCMP0             0x00000005UL                          /**< Mode TCMP0 for USART_TIMING */
#define _USART_TIMING_TXDELAY_TCMP1             0x00000006UL                          /**< Mode TCMP1 for USART_TIMING */
#define _USART_TIMING_TXDELAY_TCMP2             0x00000007UL                          /**< Mode TCMP2 for USART_TIMING */
#define USART_TIMING_TXDELAY_DEFAULT            (_USART_TIMING_TXDELAY_DEFAULT << 16) /**< Shifted mode DEFAULT for USART_TIMING */
#define USART_TIMING_TXDELAY_DISABLE            (_USART_TIMING_TXDELAY_DISABLE << 16) /**< Shifted mode DISABLE for USART_TIMING */
#define USART_TIMING_TXDELAY_ONE                (_USART_TIMING_TXDELAY_ONE << 16)     /**< Shifted mode ONE for USART_TIMING */
#define USART_TIMING_TXDELAY_TWO                (_USART_TIMING_TXDELAY_TWO << 16)     /**< Shifted mode TWO for USART_TIMING */
#define USART_TIMING_TXDELAY_THREE              (_USART_TIMING_TXDELAY_THREE << 16)   /**< Shifted mode THREE for USART_TIMING */
#define USART_TIMING_TXDELAY_SEVEN              (_USART_TIMING_TXDELAY_SEVEN << 16)   /**< Shifted mode SEVEN for USART_TIMING */
#define USART_TIMING_TXDELAY_TCMP0              (_USART_TIMING_TXDELAY_TCMP0 << 16)   /**< Shifted mode TCMP0 for USART_TIMING */
#define USART_TIMING_TXDELAY_TCMP1              (_USART_TIMING_TXDELAY_TCMP1 << 16)   /**< Shifted mode TCMP1 for USART_TIMING */
#define USART_TIMING_TXDELAY_TCMP2              (_USART_TIMING_TXDELAY_TCMP2 << 16)   /**< Shifted mode TCMP2 for USART_TIMING */
#define _USART_TIMING_CSSETUP_SHIFT             20                                    /**< Shift value for USART_CSSETUP */
#define _USART_TIMING_CSSETUP_MASK              0x700000UL                            /**< Bit mask for USART_CSSETUP */
#define _USART_TIMING_CSSETUP_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for USART_TIMING */
#define _USART_TIMING_CSSETUP_ZERO              0x00000000UL                          /**< Mode ZERO for USART_TIMING */
#define _USART_TIMING_CSSETUP_ONE               0x00000001UL                          /**< Mode ONE for USART_TIMING */
#define _USART_TIMING_CSSETUP_TWO               0x00000002UL                          /**< Mode TWO for USART_TIMING */
#define _USART_TIMING_CSSETUP_THREE             0x00000003UL                          /**< Mode THREE for USART_TIMING */
#define _USART_TIMING_CSSETUP_SEVEN             0x00000004UL                          /**< Mode SEVEN for USART_TIMING */
#define _USART_TIMING_CSSETUP_TCMP0             0x00000005UL                          /**< Mode TCMP0 for USART_TIMING */
#define _USART_TIMING_CSSETUP_TCMP1             0x00000006UL                          /**< Mode TCMP1 for USART_TIMING */
#define _USART_TIMING_CSSETUP_TCMP2             0x00000007UL                          /**< Mode TCMP2 for USART_TIMING */
#define USART_TIMING_CSSETUP_DEFAULT            (_USART_TIMING_CSSETUP_DEFAULT << 20) /**< Shifted mode DEFAULT for USART_TIMING */
#define USART_TIMING_CSSETUP_ZERO               (_USART_TIMING_CSSETUP_ZERO << 20)    /**< Shifted mode ZERO for USART_TIMING */
#define USART_TIMING_CSSETUP_ONE                (_USART_TIMING_CSSETUP_ONE << 20)     /**< Shifted mode ONE for USART_TIMING */
#define USART_TIMING_CSSETUP_TWO                (_USART_TIMING_CSSETUP_TWO << 20)     /**< Shifted mode TWO for USART_TIMING */
#define USART_TIMING_CSSETUP_THREE              (_USART_TIMING_CSSETUP_THREE << 20)   /**< Shifted mode THREE for USART_TIMING */
#define USART_TIMING_CSSETUP_SEVEN              (_USART_TIMING_CSSETUP_SEVEN << 20)   /**< Shifted mode SEVEN for USART_TIMING */
#define USART_TIMING_CSSETUP_TCMP0              (_USART_TIMING_CSSETUP_TCMP0 << 20)   /**< Shifted mode TCMP0 for USART_TIMING */
#define USART_TIMING_CSSETUP_TCMP1              (_USART_TIMING_CSSETUP_TCMP1 << 20)   /**< Shifted mode TCMP1 for USART_TIMING */
#define USART_TIMING_CSSETUP_TCMP2              (_USART_TIMING_CSSETUP_TCMP2 << 20)   /**< Shifted mode TCMP2 for USART_TIMING */
#define _USART_TIMING_ICS_SHIFT                 24                                    /**< Shift value for USART_ICS */
#define _USART_TIMING_ICS_MASK                  0x7000000UL                           /**< Bit mask for USART_ICS */
#define _USART_TIMING_ICS_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for USART_TIMING */
#define _USART_TIMING_ICS_ZERO                  0x00000000UL                          /**< Mode ZERO for USART_TIMING */
#define _USART_TIMING_ICS_ONE                   0x00000001UL                          /**< Mode ONE for USART_TIMING */
#define _USART_TIMING_ICS_TWO                   0x00000002UL                          /**< Mode TWO for USART_TIMING */
#define _USART_TIMING_ICS_THREE                 0x00000003UL                          /**< Mode THREE for USART_TIMING */
#define _USART_TIMING_ICS_SEVEN                 0x00000004UL                          /**< Mode SEVEN for USART_TIMING */
#define _USART_TIMING_ICS_TCMP0                 0x00000005UL                          /**< Mode TCMP0 for USART_TIMING */
#define _USART_TIMING_ICS_TCMP1                 0x00000006UL                          /**< Mode TCMP1 for USART_TIMING */
#define _USART_TIMING_ICS_TCMP2                 0x00000007UL                          /**< Mode TCMP2 for USART_TIMING */
#define USART_TIMING_ICS_DEFAULT                (_USART_TIMING_ICS_DEFAULT << 24)     /**< Shifted mode DEFAULT for USART_TIMING */
#define USART_TIMING_ICS_ZERO                   (_USART_TIMING_ICS_ZERO << 24)        /**< Shifted mode ZERO for USART_TIMING */
#define USART_TIMING_ICS_ONE                    (_USART_TIMING_ICS_ONE << 24)         /**< Shifted mode ONE for USART_TIMING */
#define USART_TIMING_ICS_TWO                    (_USART_TIMING_ICS_TWO << 24)         /**< Shifted mode TWO for USART_TIMING */
#define USART_TIMING_ICS_THREE                  (_USART_TIMING_ICS_THREE << 24)       /**< Shifted mode THREE for USART_TIMING */
#define USART_TIMING_ICS_SEVEN                  (_USART_TIMING_ICS_SEVEN << 24)       /**< Shifted mode SEVEN for USART_TIMING */
#define USART_TIMING_ICS_TCMP0                  (_USART_TIMING_ICS_TCMP0 << 24)       /**< Shifted mode TCMP0 for USART_TIMING */
#define USART_TIMING_ICS_TCMP1                  (_USART_TIMING_ICS_TCMP1 << 24)       /**< Shifted mode TCMP1 for USART_TIMING */
#define USART_TIMING_ICS_TCMP2                  (_USART_TIMING_ICS_TCMP2 << 24)       /**< Shifted mode TCMP2 for USART_TIMING */
#define _USART_TIMING_CSHOLD_SHIFT              28                                    /**< Shift value for USART_CSHOLD */
#define _USART_TIMING_CSHOLD_MASK               0x70000000UL                          /**< Bit mask for USART_CSHOLD */
#define _USART_TIMING_CSHOLD_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for USART_TIMING */
#define _USART_TIMING_CSHOLD_ZERO               0x00000000UL                          /**< Mode ZERO for USART_TIMING */
#define _USART_TIMING_CSHOLD_ONE                0x00000001UL                          /**< Mode ONE for USART_TIMING */
#define _USART_TIMING_CSHOLD_TWO                0x00000002UL                          /**< Mode TWO for USART_TIMING */
#define _USART_TIMING_CSHOLD_THREE              0x00000003UL                          /**< Mode THREE for USART_TIMING */
#define _USART_TIMING_CSHOLD_SEVEN              0x00000004UL                          /**< Mode SEVEN for USART_TIMING */
#define _USART_TIMING_CSHOLD_TCMP0              0x00000005UL                          /**< Mode TCMP0 for USART_TIMING */
#define _USART_TIMING_CSHOLD_TCMP1              0x00000006UL                          /**< Mode TCMP1 for USART_TIMING */
#define _USART_TIMING_CSHOLD_TCMP2              0x00000007UL                          /**< Mode TCMP2 for USART_TIMING */
#define USART_TIMING_CSHOLD_DEFAULT             (_USART_TIMING_CSHOLD_DEFAULT << 28)  /**< Shifted mode DEFAULT for USART_TIMING */
#define USART_TIMING_CSHOLD_ZERO                (_USART_TIMING_CSHOLD_ZERO << 28)     /**< Shifted mode ZERO for USART_TIMING */
#define USART_TIMING_CSHOLD_ONE                 (_USART_TIMING_CSHOLD_ONE << 28)      /**< Shifted mode ONE for USART_TIMING */
#define USART_TIMING_CSHOLD_TWO                 (_USART_TIMING_CSHOLD_TWO << 28)      /**< Shifted mode TWO for USART_TIMING */
#define USART_TIMING_CSHOLD_THREE               (_USART_TIMING_CSHOLD_THREE << 28)    /**< Shifted mode THREE for USART_TIMING */
#define USART_TIMING_CSHOLD_SEVEN               (_USART_TIMING_CSHOLD_SEVEN << 28)    /**< Shifted mode SEVEN for USART_TIMING */
#define USART_TIMING_CSHOLD_TCMP0               (_USART_TIMING_CSHOLD_TCMP0 << 28)    /**< Shifted mode TCMP0 for USART_TIMING */
#define USART_TIMING_CSHOLD_TCMP1               (_USART_TIMING_CSHOLD_TCMP1 << 28)    /**< Shifted mode TCMP1 for USART_TIMING */
#define USART_TIMING_CSHOLD_TCMP2               (_USART_TIMING_CSHOLD_TCMP2 << 28)    /**< Shifted mode TCMP2 for USART_TIMING */

/* Bit fields for USART CTRLX */
#define _USART_CTRLX_RESETVALUE                 0x00000000UL                        /**< Default value for USART_CTRLX */
#define _USART_CTRLX_MASK                       0x0000000FUL                        /**< Mask for USART_CTRLX */
#define USART_CTRLX_DBGHALT                     (0x1UL << 0)                        /**< Debug halt */
#define _USART_CTRLX_DBGHALT_SHIFT              0                                   /**< Shift value for USART_DBGHALT */
#define _USART_CTRLX_DBGHALT_MASK               0x1UL                               /**< Bit mask for USART_DBGHALT */
#define _USART_CTRLX_DBGHALT_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for USART_CTRLX */
#define USART_CTRLX_DBGHALT_DEFAULT             (_USART_CTRLX_DBGHALT_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_CTRLX */
#define USART_CTRLX_CTSINV                      (0x1UL << 1)                        /**< CTS Pin Inversion */
#define _USART_CTRLX_CTSINV_SHIFT               1                                   /**< Shift value for USART_CTSINV */
#define _USART_CTRLX_CTSINV_MASK                0x2UL                               /**< Bit mask for USART_CTSINV */
#define _USART_CTRLX_CTSINV_DEFAULT             0x00000000UL                        /**< Mode DEFAULT for USART_CTRLX */
#define USART_CTRLX_CTSINV_DEFAULT              (_USART_CTRLX_CTSINV_DEFAULT << 1)  /**< Shifted mode DEFAULT for USART_CTRLX */
#define USART_CTRLX_CTSEN                       (0x1UL << 2)                        /**< CTS Function enabled */
#define _USART_CTRLX_CTSEN_SHIFT                2                                   /**< Shift value for USART_CTSEN */
#define _USART_CTRLX_CTSEN_MASK                 0x4UL                               /**< Bit mask for USART_CTSEN */
#define _USART_CTRLX_CTSEN_DEFAULT              0x00000000UL                        /**< Mode DEFAULT for USART_CTRLX */
#define USART_CTRLX_CTSEN_DEFAULT               (_USART_CTRLX_CTSEN_DEFAULT << 2)   /**< Shifted mode DEFAULT for USART_CTRLX */
#define USART_CTRLX_RTSINV                      (0x1UL << 3)                        /**< RTS Pin Inversion */
#define _USART_CTRLX_RTSINV_SHIFT               3                                   /**< Shift value for USART_RTSINV */
#define _USART_CTRLX_RTSINV_MASK                0x8UL                               /**< Bit mask for USART_RTSINV */
#define _USART_CTRLX_RTSINV_DEFAULT             0x00000000UL                        /**< Mode DEFAULT for USART_CTRLX */
#define USART_CTRLX_RTSINV_DEFAULT              (_USART_CTRLX_RTSINV_DEFAULT << 3)  /**< Shifted mode DEFAULT for USART_CTRLX */

/* Bit fields for USART TIMECMP0 */
#define _USART_TIMECMP0_RESETVALUE              0x00000000UL                              /**< Default value for USART_TIMECMP0 */
#define _USART_TIMECMP0_MASK                    0x017700FFUL                              /**< Mask for USART_TIMECMP0 */
#define _USART_TIMECMP0_TCMPVAL_SHIFT           0                                         /**< Shift value for USART_TCMPVAL */
#define _USART_TIMECMP0_TCMPVAL_MASK            0xFFUL                                    /**< Bit mask for USART_TCMPVAL */
#define _USART_TIMECMP0_TCMPVAL_DEFAULT         0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP0 */
#define USART_TIMECMP0_TCMPVAL_DEFAULT          (_USART_TIMECMP0_TCMPVAL_DEFAULT << 0)    /**< Shifted mode DEFAULT for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTART_SHIFT            16                                        /**< Shift value for USART_TSTART */
#define _USART_TIMECMP0_TSTART_MASK             0x70000UL                                 /**< Bit mask for USART_TSTART */
#define _USART_TIMECMP0_TSTART_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTART_DISABLE          0x00000000UL                              /**< Mode DISABLE for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTART_TXEOF            0x00000001UL                              /**< Mode TXEOF for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTART_TXC              0x00000002UL                              /**< Mode TXC for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTART_RXACT            0x00000003UL                              /**< Mode RXACT for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTART_RXEOF            0x00000004UL                              /**< Mode RXEOF for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTART_DEFAULT           (_USART_TIMECMP0_TSTART_DEFAULT << 16)    /**< Shifted mode DEFAULT for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTART_DISABLE           (_USART_TIMECMP0_TSTART_DISABLE << 16)    /**< Shifted mode DISABLE for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTART_TXEOF             (_USART_TIMECMP0_TSTART_TXEOF << 16)      /**< Shifted mode TXEOF for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTART_TXC               (_USART_TIMECMP0_TSTART_TXC << 16)        /**< Shifted mode TXC for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTART_RXACT             (_USART_TIMECMP0_TSTART_RXACT << 16)      /**< Shifted mode RXACT for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTART_RXEOF             (_USART_TIMECMP0_TSTART_RXEOF << 16)      /**< Shifted mode RXEOF for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTOP_SHIFT             20                                        /**< Shift value for USART_TSTOP */
#define _USART_TIMECMP0_TSTOP_MASK              0x700000UL                                /**< Bit mask for USART_TSTOP */
#define _USART_TIMECMP0_TSTOP_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTOP_TCMP0             0x00000000UL                              /**< Mode TCMP0 for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTOP_TXST              0x00000001UL                              /**< Mode TXST for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTOP_RXACT             0x00000002UL                              /**< Mode RXACT for USART_TIMECMP0 */
#define _USART_TIMECMP0_TSTOP_RXACTN            0x00000003UL                              /**< Mode RXACTN for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTOP_DEFAULT            (_USART_TIMECMP0_TSTOP_DEFAULT << 20)     /**< Shifted mode DEFAULT for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTOP_TCMP0              (_USART_TIMECMP0_TSTOP_TCMP0 << 20)       /**< Shifted mode TCMP0 for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTOP_TXST               (_USART_TIMECMP0_TSTOP_TXST << 20)        /**< Shifted mode TXST for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTOP_RXACT              (_USART_TIMECMP0_TSTOP_RXACT << 20)       /**< Shifted mode RXACT for USART_TIMECMP0 */
#define USART_TIMECMP0_TSTOP_RXACTN             (_USART_TIMECMP0_TSTOP_RXACTN << 20)      /**< Shifted mode RXACTN for USART_TIMECMP0 */
#define USART_TIMECMP0_RESTARTEN                (0x1UL << 24)                             /**< Restart Timer on TCMP0 */
#define _USART_TIMECMP0_RESTARTEN_SHIFT         24                                        /**< Shift value for USART_RESTARTEN */
#define _USART_TIMECMP0_RESTARTEN_MASK          0x1000000UL                               /**< Bit mask for USART_RESTARTEN */
#define _USART_TIMECMP0_RESTARTEN_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP0 */
#define USART_TIMECMP0_RESTARTEN_DEFAULT        (_USART_TIMECMP0_RESTARTEN_DEFAULT << 24) /**< Shifted mode DEFAULT for USART_TIMECMP0 */

/* Bit fields for USART TIMECMP1 */
#define _USART_TIMECMP1_RESETVALUE              0x00000000UL                              /**< Default value for USART_TIMECMP1 */
#define _USART_TIMECMP1_MASK                    0x017700FFUL                              /**< Mask for USART_TIMECMP1 */
#define _USART_TIMECMP1_TCMPVAL_SHIFT           0                                         /**< Shift value for USART_TCMPVAL */
#define _USART_TIMECMP1_TCMPVAL_MASK            0xFFUL                                    /**< Bit mask for USART_TCMPVAL */
#define _USART_TIMECMP1_TCMPVAL_DEFAULT         0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP1 */
#define USART_TIMECMP1_TCMPVAL_DEFAULT          (_USART_TIMECMP1_TCMPVAL_DEFAULT << 0)    /**< Shifted mode DEFAULT for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTART_SHIFT            16                                        /**< Shift value for USART_TSTART */
#define _USART_TIMECMP1_TSTART_MASK             0x70000UL                                 /**< Bit mask for USART_TSTART */
#define _USART_TIMECMP1_TSTART_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTART_DISABLE          0x00000000UL                              /**< Mode DISABLE for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTART_TXEOF            0x00000001UL                              /**< Mode TXEOF for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTART_TXC              0x00000002UL                              /**< Mode TXC for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTART_RXACT            0x00000003UL                              /**< Mode RXACT for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTART_RXEOF            0x00000004UL                              /**< Mode RXEOF for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTART_DEFAULT           (_USART_TIMECMP1_TSTART_DEFAULT << 16)    /**< Shifted mode DEFAULT for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTART_DISABLE           (_USART_TIMECMP1_TSTART_DISABLE << 16)    /**< Shifted mode DISABLE for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTART_TXEOF             (_USART_TIMECMP1_TSTART_TXEOF << 16)      /**< Shifted mode TXEOF for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTART_TXC               (_USART_TIMECMP1_TSTART_TXC << 16)        /**< Shifted mode TXC for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTART_RXACT             (_USART_TIMECMP1_TSTART_RXACT << 16)      /**< Shifted mode RXACT for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTART_RXEOF             (_USART_TIMECMP1_TSTART_RXEOF << 16)      /**< Shifted mode RXEOF for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTOP_SHIFT             20                                        /**< Shift value for USART_TSTOP */
#define _USART_TIMECMP1_TSTOP_MASK              0x700000UL                                /**< Bit mask for USART_TSTOP */
#define _USART_TIMECMP1_TSTOP_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTOP_TCMP1             0x00000000UL                              /**< Mode TCMP1 for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTOP_TXST              0x00000001UL                              /**< Mode TXST for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTOP_RXACT             0x00000002UL                              /**< Mode RXACT for USART_TIMECMP1 */
#define _USART_TIMECMP1_TSTOP_RXACTN            0x00000003UL                              /**< Mode RXACTN for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTOP_DEFAULT            (_USART_TIMECMP1_TSTOP_DEFAULT << 20)     /**< Shifted mode DEFAULT for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTOP_TCMP1              (_USART_TIMECMP1_TSTOP_TCMP1 << 20)       /**< Shifted mode TCMP1 for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTOP_TXST               (_USART_TIMECMP1_TSTOP_TXST << 20)        /**< Shifted mode TXST for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTOP_RXACT              (_USART_TIMECMP1_TSTOP_RXACT << 20)       /**< Shifted mode RXACT for USART_TIMECMP1 */
#define USART_TIMECMP1_TSTOP_RXACTN             (_USART_TIMECMP1_TSTOP_RXACTN << 20)      /**< Shifted mode RXACTN for USART_TIMECMP1 */
#define USART_TIMECMP1_RESTARTEN                (0x1UL << 24)                             /**< Restart Timer on TCMP1 */
#define _USART_TIMECMP1_RESTARTEN_SHIFT         24                                        /**< Shift value for USART_RESTARTEN */
#define _USART_TIMECMP1_RESTARTEN_MASK          0x1000000UL                               /**< Bit mask for USART_RESTARTEN */
#define _USART_TIMECMP1_RESTARTEN_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP1 */
#define USART_TIMECMP1_RESTARTEN_DEFAULT        (_USART_TIMECMP1_RESTARTEN_DEFAULT << 24) /**< Shifted mode DEFAULT for USART_TIMECMP1 */

/* Bit fields for USART TIMECMP2 */
#define _USART_TIMECMP2_RESETVALUE              0x00000000UL                              /**< Default value for USART_TIMECMP2 */
#define _USART_TIMECMP2_MASK                    0x017700FFUL                              /**< Mask for USART_TIMECMP2 */
#define _USART_TIMECMP2_TCMPVAL_SHIFT           0                                         /**< Shift value for USART_TCMPVAL */
#define _USART_TIMECMP2_TCMPVAL_MASK            0xFFUL                                    /**< Bit mask for USART_TCMPVAL */
#define _USART_TIMECMP2_TCMPVAL_DEFAULT         0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP2 */
#define USART_TIMECMP2_TCMPVAL_DEFAULT          (_USART_TIMECMP2_TCMPVAL_DEFAULT << 0)    /**< Shifted mode DEFAULT for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTART_SHIFT            16                                        /**< Shift value for USART_TSTART */
#define _USART_TIMECMP2_TSTART_MASK             0x70000UL                                 /**< Bit mask for USART_TSTART */
#define _USART_TIMECMP2_TSTART_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTART_DISABLE          0x00000000UL                              /**< Mode DISABLE for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTART_TXEOF            0x00000001UL                              /**< Mode TXEOF for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTART_TXC              0x00000002UL                              /**< Mode TXC for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTART_RXACT            0x00000003UL                              /**< Mode RXACT for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTART_RXEOF            0x00000004UL                              /**< Mode RXEOF for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTART_DEFAULT           (_USART_TIMECMP2_TSTART_DEFAULT << 16)    /**< Shifted mode DEFAULT for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTART_DISABLE           (_USART_TIMECMP2_TSTART_DISABLE << 16)    /**< Shifted mode DISABLE for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTART_TXEOF             (_USART_TIMECMP2_TSTART_TXEOF << 16)      /**< Shifted mode TXEOF for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTART_TXC               (_USART_TIMECMP2_TSTART_TXC << 16)        /**< Shifted mode TXC for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTART_RXACT             (_USART_TIMECMP2_TSTART_RXACT << 16)      /**< Shifted mode RXACT for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTART_RXEOF             (_USART_TIMECMP2_TSTART_RXEOF << 16)      /**< Shifted mode RXEOF for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTOP_SHIFT             20                                        /**< Shift value for USART_TSTOP */
#define _USART_TIMECMP2_TSTOP_MASK              0x700000UL                                /**< Bit mask for USART_TSTOP */
#define _USART_TIMECMP2_TSTOP_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTOP_TCMP2             0x00000000UL                              /**< Mode TCMP2 for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTOP_TXST              0x00000001UL                              /**< Mode TXST for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTOP_RXACT             0x00000002UL                              /**< Mode RXACT for USART_TIMECMP2 */
#define _USART_TIMECMP2_TSTOP_RXACTN            0x00000003UL                              /**< Mode RXACTN for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTOP_DEFAULT            (_USART_TIMECMP2_TSTOP_DEFAULT << 20)     /**< Shifted mode DEFAULT for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTOP_TCMP2              (_USART_TIMECMP2_TSTOP_TCMP2 << 20)       /**< Shifted mode TCMP2 for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTOP_TXST               (_USART_TIMECMP2_TSTOP_TXST << 20)        /**< Shifted mode TXST for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTOP_RXACT              (_USART_TIMECMP2_TSTOP_RXACT << 20)       /**< Shifted mode RXACT for USART_TIMECMP2 */
#define USART_TIMECMP2_TSTOP_RXACTN             (_USART_TIMECMP2_TSTOP_RXACTN << 20)      /**< Shifted mode RXACTN for USART_TIMECMP2 */
#define USART_TIMECMP2_RESTARTEN                (0x1UL << 24)                             /**< Restart Timer on TCMP2 */
#define _USART_TIMECMP2_RESTARTEN_SHIFT         24                                        /**< Shift value for USART_RESTARTEN */
#define _USART_TIMECMP2_RESTARTEN_MASK          0x1000000UL                               /**< Bit mask for USART_RESTARTEN */
#define _USART_TIMECMP2_RESTARTEN_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for USART_TIMECMP2 */
#define USART_TIMECMP2_RESTARTEN_DEFAULT        (_USART_TIMECMP2_RESTARTEN_DEFAULT << 24) /**< Shifted mode DEFAULT for USART_TIMECMP2 */

/* Bit fields for USART ROUTEPEN */
#define _USART_ROUTEPEN_RESETVALUE              0x00000000UL                          /**< Default value for USART_ROUTEPEN */
#define _USART_ROUTEPEN_MASK                    0x0000003FUL                          /**< Mask for USART_ROUTEPEN */
#define USART_ROUTEPEN_RXPEN                    (0x1UL << 0)                          /**< RX Pin Enable */
#define _USART_ROUTEPEN_RXPEN_SHIFT             0                                     /**< Shift value for USART_RXPEN */
#define _USART_ROUTEPEN_RXPEN_MASK              0x1UL                                 /**< Bit mask for USART_RXPEN */
#define _USART_ROUTEPEN_RXPEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_RXPEN_DEFAULT            (_USART_ROUTEPEN_RXPEN_DEFAULT << 0)  /**< Shifted mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_TXPEN                    (0x1UL << 1)                          /**< TX Pin Enable */
#define _USART_ROUTEPEN_TXPEN_SHIFT             1                                     /**< Shift value for USART_TXPEN */
#define _USART_ROUTEPEN_TXPEN_MASK              0x2UL                                 /**< Bit mask for USART_TXPEN */
#define _USART_ROUTEPEN_TXPEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_TXPEN_DEFAULT            (_USART_ROUTEPEN_TXPEN_DEFAULT << 1)  /**< Shifted mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_CSPEN                    (0x1UL << 2)                          /**< CS Pin Enable */
#define _USART_ROUTEPEN_CSPEN_SHIFT             2                                     /**< Shift value for USART_CSPEN */
#define _USART_ROUTEPEN_CSPEN_MASK              0x4UL                                 /**< Bit mask for USART_CSPEN */
#define _USART_ROUTEPEN_CSPEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_CSPEN_DEFAULT            (_USART_ROUTEPEN_CSPEN_DEFAULT << 2)  /**< Shifted mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_CLKPEN                   (0x1UL << 3)                          /**< CLK Pin Enable */
#define _USART_ROUTEPEN_CLKPEN_SHIFT            3                                     /**< Shift value for USART_CLKPEN */
#define _USART_ROUTEPEN_CLKPEN_MASK             0x8UL                                 /**< Bit mask for USART_CLKPEN */
#define _USART_ROUTEPEN_CLKPEN_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_CLKPEN_DEFAULT           (_USART_ROUTEPEN_CLKPEN_DEFAULT << 3) /**< Shifted mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_CTSPEN                   (0x1UL << 4)                          /**< CTS Pin Enable */
#define _USART_ROUTEPEN_CTSPEN_SHIFT            4                                     /**< Shift value for USART_CTSPEN */
#define _USART_ROUTEPEN_CTSPEN_MASK             0x10UL                                /**< Bit mask for USART_CTSPEN */
#define _USART_ROUTEPEN_CTSPEN_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_CTSPEN_DEFAULT           (_USART_ROUTEPEN_CTSPEN_DEFAULT << 4) /**< Shifted mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_RTSPEN                   (0x1UL << 5)                          /**< RTS Pin Enable */
#define _USART_ROUTEPEN_RTSPEN_SHIFT            5                                     /**< Shift value for USART_RTSPEN */
#define _USART_ROUTEPEN_RTSPEN_MASK             0x20UL                                /**< Bit mask for USART_RTSPEN */
#define _USART_ROUTEPEN_RTSPEN_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for USART_ROUTEPEN */
#define USART_ROUTEPEN_RTSPEN_DEFAULT           (_USART_ROUTEPEN_RTSPEN_DEFAULT << 5) /**< Shifted mode DEFAULT for USART_ROUTEPEN */

/* Bit fields for USART ROUTELOC0 */
#define _USART_ROUTELOC0_RESETVALUE             0x00000000UL                            /**< Default value for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_MASK                   0x1F1F1F1FUL                            /**< Mask for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_SHIFT            0                                       /**< Shift value for USART_RXLOC */
#define _USART_ROUTELOC0_RXLOC_MASK             0x1FUL                                  /**< Bit mask for USART_RXLOC */
#define _USART_ROUTELOC0_RXLOC_LOC0             0x00000000UL                            /**< Mode LOC0 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC1             0x00000001UL                            /**< Mode LOC1 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC2             0x00000002UL                            /**< Mode LOC2 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC3             0x00000003UL                            /**< Mode LOC3 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC4             0x00000004UL                            /**< Mode LOC4 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC5             0x00000005UL                            /**< Mode LOC5 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC6             0x00000006UL                            /**< Mode LOC6 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC7             0x00000007UL                            /**< Mode LOC7 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC8             0x00000008UL                            /**< Mode LOC8 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC9             0x00000009UL                            /**< Mode LOC9 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC10            0x0000000AUL                            /**< Mode LOC10 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC11            0x0000000BUL                            /**< Mode LOC11 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC12            0x0000000CUL                            /**< Mode LOC12 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC13            0x0000000DUL                            /**< Mode LOC13 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC14            0x0000000EUL                            /**< Mode LOC14 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC15            0x0000000FUL                            /**< Mode LOC15 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC16            0x00000010UL                            /**< Mode LOC16 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC17            0x00000011UL                            /**< Mode LOC17 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC18            0x00000012UL                            /**< Mode LOC18 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC19            0x00000013UL                            /**< Mode LOC19 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC20            0x00000014UL                            /**< Mode LOC20 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC21            0x00000015UL                            /**< Mode LOC21 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC22            0x00000016UL                            /**< Mode LOC22 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC23            0x00000017UL                            /**< Mode LOC23 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC24            0x00000018UL                            /**< Mode LOC24 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC25            0x00000019UL                            /**< Mode LOC25 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC26            0x0000001AUL                            /**< Mode LOC26 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC27            0x0000001BUL                            /**< Mode LOC27 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC28            0x0000001CUL                            /**< Mode LOC28 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC29            0x0000001DUL                            /**< Mode LOC29 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC30            0x0000001EUL                            /**< Mode LOC30 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_RXLOC_LOC31            0x0000001FUL                            /**< Mode LOC31 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC0              (_USART_ROUTELOC0_RXLOC_LOC0 << 0)      /**< Shifted mode LOC0 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_DEFAULT           (_USART_ROUTELOC0_RXLOC_DEFAULT << 0)   /**< Shifted mode DEFAULT for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC1              (_USART_ROUTELOC0_RXLOC_LOC1 << 0)      /**< Shifted mode LOC1 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC2              (_USART_ROUTELOC0_RXLOC_LOC2 << 0)      /**< Shifted mode LOC2 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC3              (_USART_ROUTELOC0_RXLOC_LOC3 << 0)      /**< Shifted mode LOC3 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC4              (_USART_ROUTELOC0_RXLOC_LOC4 << 0)      /**< Shifted mode LOC4 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC5              (_USART_ROUTELOC0_RXLOC_LOC5 << 0)      /**< Shifted mode LOC5 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC6              (_USART_ROUTELOC0_RXLOC_LOC6 << 0)      /**< Shifted mode LOC6 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC7              (_USART_ROUTELOC0_RXLOC_LOC7 << 0)      /**< Shifted mode LOC7 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC8              (_USART_ROUTELOC0_RXLOC_LOC8 << 0)      /**< Shifted mode LOC8 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC9              (_USART_ROUTELOC0_RXLOC_LOC9 << 0)      /**< Shifted mode LOC9 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC10             (_USART_ROUTELOC0_RXLOC_LOC10 << 0)     /**< Shifted mode LOC10 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC11             (_USART_ROUTELOC0_RXLOC_LOC11 << 0)     /**< Shifted mode LOC11 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC12             (_USART_ROUTELOC0_RXLOC_LOC12 << 0)     /**< Shifted mode LOC12 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC13             (_USART_ROUTELOC0_RXLOC_LOC13 << 0)     /**< Shifted mode LOC13 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC14             (_USART_ROUTELOC0_RXLOC_LOC14 << 0)     /**< Shifted mode LOC14 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC15             (_USART_ROUTELOC0_RXLOC_LOC15 << 0)     /**< Shifted mode LOC15 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC16             (_USART_ROUTELOC0_RXLOC_LOC16 << 0)     /**< Shifted mode LOC16 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC17             (_USART_ROUTELOC0_RXLOC_LOC17 << 0)     /**< Shifted mode LOC17 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC18             (_USART_ROUTELOC0_RXLOC_LOC18 << 0)     /**< Shifted mode LOC18 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC19             (_USART_ROUTELOC0_RXLOC_LOC19 << 0)     /**< Shifted mode LOC19 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC20             (_USART_ROUTELOC0_RXLOC_LOC20 << 0)     /**< Shifted mode LOC20 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC21             (_USART_ROUTELOC0_RXLOC_LOC21 << 0)     /**< Shifted mode LOC21 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC22             (_USART_ROUTELOC0_RXLOC_LOC22 << 0)     /**< Shifted mode LOC22 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC23             (_USART_ROUTELOC0_RXLOC_LOC23 << 0)     /**< Shifted mode LOC23 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC24             (_USART_ROUTELOC0_RXLOC_LOC24 << 0)     /**< Shifted mode LOC24 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC25             (_USART_ROUTELOC0_RXLOC_LOC25 << 0)     /**< Shifted mode LOC25 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC26             (_USART_ROUTELOC0_RXLOC_LOC26 << 0)     /**< Shifted mode LOC26 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC27             (_USART_ROUTELOC0_RXLOC_LOC27 << 0)     /**< Shifted mode LOC27 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC28             (_USART_ROUTELOC0_RXLOC_LOC28 << 0)     /**< Shifted mode LOC28 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC29             (_USART_ROUTELOC0_RXLOC_LOC29 << 0)     /**< Shifted mode LOC29 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC30             (_USART_ROUTELOC0_RXLOC_LOC30 << 0)     /**< Shifted mode LOC30 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_RXLOC_LOC31             (_USART_ROUTELOC0_RXLOC_LOC31 << 0)     /**< Shifted mode LOC31 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_SHIFT            8                                       /**< Shift value for USART_TXLOC */
#define _USART_ROUTELOC0_TXLOC_MASK             0x1F00UL                                /**< Bit mask for USART_TXLOC */
#define _USART_ROUTELOC0_TXLOC_LOC0             0x00000000UL                            /**< Mode LOC0 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC1             0x00000001UL                            /**< Mode LOC1 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC2             0x00000002UL                            /**< Mode LOC2 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC3             0x00000003UL                            /**< Mode LOC3 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC4             0x00000004UL                            /**< Mode LOC4 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC5             0x00000005UL                            /**< Mode LOC5 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC6             0x00000006UL                            /**< Mode LOC6 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC7             0x00000007UL                            /**< Mode LOC7 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC8             0x00000008UL                            /**< Mode LOC8 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC9             0x00000009UL                            /**< Mode LOC9 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC10            0x0000000AUL                            /**< Mode LOC10 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC11            0x0000000BUL                            /**< Mode LOC11 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC12            0x0000000CUL                            /**< Mode LOC12 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC13            0x0000000DUL                            /**< Mode LOC13 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC14            0x0000000EUL                            /**< Mode LOC14 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC15            0x0000000FUL                            /**< Mode LOC15 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC16            0x00000010UL                            /**< Mode LOC16 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC17            0x00000011UL                            /**< Mode LOC17 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC18            0x00000012UL                            /**< Mode LOC18 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC19            0x00000013UL                            /**< Mode LOC19 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC20            0x00000014UL                            /**< Mode LOC20 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC21            0x00000015UL                            /**< Mode LOC21 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC22            0x00000016UL                            /**< Mode LOC22 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC23            0x00000017UL                            /**< Mode LOC23 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC24            0x00000018UL                            /**< Mode LOC24 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC25            0x00000019UL                            /**< Mode LOC25 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC26            0x0000001AUL                            /**< Mode LOC26 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC27            0x0000001BUL                            /**< Mode LOC27 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC28            0x0000001CUL                            /**< Mode LOC28 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC29            0x0000001DUL                            /**< Mode LOC29 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC30            0x0000001EUL                            /**< Mode LOC30 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_TXLOC_LOC31            0x0000001FUL                            /**< Mode LOC31 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC0              (_USART_ROUTELOC0_TXLOC_LOC0 << 8)      /**< Shifted mode LOC0 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_DEFAULT           (_USART_ROUTELOC0_TXLOC_DEFAULT << 8)   /**< Shifted mode DEFAULT for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC1              (_USART_ROUTELOC0_TXLOC_LOC1 << 8)      /**< Shifted mode LOC1 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC2              (_USART_ROUTELOC0_TXLOC_LOC2 << 8)      /**< Shifted mode LOC2 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC3              (_USART_ROUTELOC0_TXLOC_LOC3 << 8)      /**< Shifted mode LOC3 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC4              (_USART_ROUTELOC0_TXLOC_LOC4 << 8)      /**< Shifted mode LOC4 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC5              (_USART_ROUTELOC0_TXLOC_LOC5 << 8)      /**< Shifted mode LOC5 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC6              (_USART_ROUTELOC0_TXLOC_LOC6 << 8)      /**< Shifted mode LOC6 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC7              (_USART_ROUTELOC0_TXLOC_LOC7 << 8)      /**< Shifted mode LOC7 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC8              (_USART_ROUTELOC0_TXLOC_LOC8 << 8)      /**< Shifted mode LOC8 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC9              (_USART_ROUTELOC0_TXLOC_LOC9 << 8)      /**< Shifted mode LOC9 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC10             (_USART_ROUTELOC0_TXLOC_LOC10 << 8)     /**< Shifted mode LOC10 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC11             (_USART_ROUTELOC0_TXLOC_LOC11 << 8)     /**< Shifted mode LOC11 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC12             (_USART_ROUTELOC0_TXLOC_LOC12 << 8)     /**< Shifted mode LOC12 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC13             (_USART_ROUTELOC0_TXLOC_LOC13 << 8)     /**< Shifted mode LOC13 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC14             (_USART_ROUTELOC0_TXLOC_LOC14 << 8)     /**< Shifted mode LOC14 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC15             (_USART_ROUTELOC0_TXLOC_LOC15 << 8)     /**< Shifted mode LOC15 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC16             (_USART_ROUTELOC0_TXLOC_LOC16 << 8)     /**< Shifted mode LOC16 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC17             (_USART_ROUTELOC0_TXLOC_LOC17 << 8)     /**< Shifted mode LOC17 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC18             (_USART_ROUTELOC0_TXLOC_LOC18 << 8)     /**< Shifted mode LOC18 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC19             (_USART_ROUTELOC0_TXLOC_LOC19 << 8)     /**< Shifted mode LOC19 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC20             (_USART_ROUTELOC0_TXLOC_LOC20 << 8)     /**< Shifted mode LOC20 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC21             (_USART_ROUTELOC0_TXLOC_LOC21 << 8)     /**< Shifted mode LOC21 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC22             (_USART_ROUTELOC0_TXLOC_LOC22 << 8)     /**< Shifted mode LOC22 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC23             (_USART_ROUTELOC0_TXLOC_LOC23 << 8)     /**< Shifted mode LOC23 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC24             (_USART_ROUTELOC0_TXLOC_LOC24 << 8)     /**< Shifted mode LOC24 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC25             (_USART_ROUTELOC0_TXLOC_LOC25 << 8)     /**< Shifted mode LOC25 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC26             (_USART_ROUTELOC0_TXLOC_LOC26 << 8)     /**< Shifted mode LOC26 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC27             (_USART_ROUTELOC0_TXLOC_LOC27 << 8)     /**< Shifted mode LOC27 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC28             (_USART_ROUTELOC0_TXLOC_LOC28 << 8)     /**< Shifted mode LOC28 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC29             (_USART_ROUTELOC0_TXLOC_LOC29 << 8)     /**< Shifted mode LOC29 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC30             (_USART_ROUTELOC0_TXLOC_LOC30 << 8)     /**< Shifted mode LOC30 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_TXLOC_LOC31             (_USART_ROUTELOC0_TXLOC_LOC31 << 8)     /**< Shifted mode LOC31 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_SHIFT            16                                      /**< Shift value for USART_CSLOC */
#define _USART_ROUTELOC0_CSLOC_MASK             0x1F0000UL                              /**< Bit mask for USART_CSLOC */
#define _USART_ROUTELOC0_CSLOC_LOC0             0x00000000UL                            /**< Mode LOC0 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC1             0x00000001UL                            /**< Mode LOC1 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC2             0x00000002UL                            /**< Mode LOC2 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC3             0x00000003UL                            /**< Mode LOC3 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC4             0x00000004UL                            /**< Mode LOC4 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC5             0x00000005UL                            /**< Mode LOC5 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC6             0x00000006UL                            /**< Mode LOC6 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC7             0x00000007UL                            /**< Mode LOC7 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC8             0x00000008UL                            /**< Mode LOC8 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC9             0x00000009UL                            /**< Mode LOC9 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC10            0x0000000AUL                            /**< Mode LOC10 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC11            0x0000000BUL                            /**< Mode LOC11 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC12            0x0000000CUL                            /**< Mode LOC12 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC13            0x0000000DUL                            /**< Mode LOC13 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC14            0x0000000EUL                            /**< Mode LOC14 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC15            0x0000000FUL                            /**< Mode LOC15 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC16            0x00000010UL                            /**< Mode LOC16 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC17            0x00000011UL                            /**< Mode LOC17 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC18            0x00000012UL                            /**< Mode LOC18 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC19            0x00000013UL                            /**< Mode LOC19 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC20            0x00000014UL                            /**< Mode LOC20 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC21            0x00000015UL                            /**< Mode LOC21 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC22            0x00000016UL                            /**< Mode LOC22 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC23            0x00000017UL                            /**< Mode LOC23 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC24            0x00000018UL                            /**< Mode LOC24 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC25            0x00000019UL                            /**< Mode LOC25 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC26            0x0000001AUL                            /**< Mode LOC26 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC27            0x0000001BUL                            /**< Mode LOC27 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC28            0x0000001CUL                            /**< Mode LOC28 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC29            0x0000001DUL                            /**< Mode LOC29 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC30            0x0000001EUL                            /**< Mode LOC30 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CSLOC_LOC31            0x0000001FUL                            /**< Mode LOC31 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC0              (_USART_ROUTELOC0_CSLOC_LOC0 << 16)     /**< Shifted mode LOC0 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_DEFAULT           (_USART_ROUTELOC0_CSLOC_DEFAULT << 16)  /**< Shifted mode DEFAULT for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC1              (_USART_ROUTELOC0_CSLOC_LOC1 << 16)     /**< Shifted mode LOC1 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC2              (_USART_ROUTELOC0_CSLOC_LOC2 << 16)     /**< Shifted mode LOC2 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC3              (_USART_ROUTELOC0_CSLOC_LOC3 << 16)     /**< Shifted mode LOC3 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC4              (_USART_ROUTELOC0_CSLOC_LOC4 << 16)     /**< Shifted mode LOC4 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC5              (_USART_ROUTELOC0_CSLOC_LOC5 << 16)     /**< Shifted mode LOC5 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC6              (_USART_ROUTELOC0_CSLOC_LOC6 << 16)     /**< Shifted mode LOC6 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC7              (_USART_ROUTELOC0_CSLOC_LOC7 << 16)     /**< Shifted mode LOC7 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC8              (_USART_ROUTELOC0_CSLOC_LOC8 << 16)     /**< Shifted mode LOC8 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC9              (_USART_ROUTELOC0_CSLOC_LOC9 << 16)     /**< Shifted mode LOC9 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC10             (_USART_ROUTELOC0_CSLOC_LOC10 << 16)    /**< Shifted mode LOC10 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC11             (_USART_ROUTELOC0_CSLOC_LOC11 << 16)    /**< Shifted mode LOC11 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC12             (_USART_ROUTELOC0_CSLOC_LOC12 << 16)    /**< Shifted mode LOC12 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC13             (_USART_ROUTELOC0_CSLOC_LOC13 << 16)    /**< Shifted mode LOC13 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC14             (_USART_ROUTELOC0_CSLOC_LOC14 << 16)    /**< Shifted mode LOC14 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC15             (_USART_ROUTELOC0_CSLOC_LOC15 << 16)    /**< Shifted mode LOC15 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC16             (_USART_ROUTELOC0_CSLOC_LOC16 << 16)    /**< Shifted mode LOC16 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC17             (_USART_ROUTELOC0_CSLOC_LOC17 << 16)    /**< Shifted mode LOC17 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC18             (_USART_ROUTELOC0_CSLOC_LOC18 << 16)    /**< Shifted mode LOC18 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC19             (_USART_ROUTELOC0_CSLOC_LOC19 << 16)    /**< Shifted mode LOC19 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC20             (_USART_ROUTELOC0_CSLOC_LOC20 << 16)    /**< Shifted mode LOC20 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC21             (_USART_ROUTELOC0_CSLOC_LOC21 << 16)    /**< Shifted mode LOC21 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC22             (_USART_ROUTELOC0_CSLOC_LOC22 << 16)    /**< Shifted mode LOC22 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC23             (_USART_ROUTELOC0_CSLOC_LOC23 << 16)    /**< Shifted mode LOC23 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC24             (_USART_ROUTELOC0_CSLOC_LOC24 << 16)    /**< Shifted mode LOC24 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC25             (_USART_ROUTELOC0_CSLOC_LOC25 << 16)    /**< Shifted mode LOC25 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC26             (_USART_ROUTELOC0_CSLOC_LOC26 << 16)    /**< Shifted mode LOC26 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC27             (_USART_ROUTELOC0_CSLOC_LOC27 << 16)    /**< Shifted mode LOC27 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC28             (_USART_ROUTELOC0_CSLOC_LOC28 << 16)    /**< Shifted mode LOC28 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC29             (_USART_ROUTELOC0_CSLOC_LOC29 << 16)    /**< Shifted mode LOC29 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC30             (_USART_ROUTELOC0_CSLOC_LOC30 << 16)    /**< Shifted mode LOC30 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CSLOC_LOC31             (_USART_ROUTELOC0_CSLOC_LOC31 << 16)    /**< Shifted mode LOC31 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_SHIFT           24                                      /**< Shift value for USART_CLKLOC */
#define _USART_ROUTELOC0_CLKLOC_MASK            0x1F000000UL                            /**< Bit mask for USART_CLKLOC */
#define _USART_ROUTELOC0_CLKLOC_LOC0            0x00000000UL                            /**< Mode LOC0 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC1            0x00000001UL                            /**< Mode LOC1 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC2            0x00000002UL                            /**< Mode LOC2 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC3            0x00000003UL                            /**< Mode LOC3 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC4            0x00000004UL                            /**< Mode LOC4 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC5            0x00000005UL                            /**< Mode LOC5 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC6            0x00000006UL                            /**< Mode LOC6 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC7            0x00000007UL                            /**< Mode LOC7 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC8            0x00000008UL                            /**< Mode LOC8 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC9            0x00000009UL                            /**< Mode LOC9 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC10           0x0000000AUL                            /**< Mode LOC10 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC11           0x0000000BUL                            /**< Mode LOC11 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC12           0x0000000CUL                            /**< Mode LOC12 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC13           0x0000000DUL                            /**< Mode LOC13 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC14           0x0000000EUL                            /**< Mode LOC14 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC15           0x0000000FUL                            /**< Mode LOC15 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC16           0x00000010UL                            /**< Mode LOC16 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC17           0x00000011UL                            /**< Mode LOC17 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC18           0x00000012UL                            /**< Mode LOC18 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC19           0x00000013UL                            /**< Mode LOC19 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC20           0x00000014UL                            /**< Mode LOC20 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC21           0x00000015UL                            /**< Mode LOC21 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC22           0x00000016UL                            /**< Mode LOC22 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC23           0x00000017UL                            /**< Mode LOC23 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC24           0x00000018UL                            /**< Mode LOC24 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC25           0x00000019UL                            /**< Mode LOC25 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC26           0x0000001AUL                            /**< Mode LOC26 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC27           0x0000001BUL                            /**< Mode LOC27 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC28           0x0000001CUL                            /**< Mode LOC28 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC29           0x0000001DUL                            /**< Mode LOC29 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC30           0x0000001EUL                            /**< Mode LOC30 for USART_ROUTELOC0 */
#define _USART_ROUTELOC0_CLKLOC_LOC31           0x0000001FUL                            /**< Mode LOC31 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC0             (_USART_ROUTELOC0_CLKLOC_LOC0 << 24)    /**< Shifted mode LOC0 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_DEFAULT          (_USART_ROUTELOC0_CLKLOC_DEFAULT << 24) /**< Shifted mode DEFAULT for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC1             (_USART_ROUTELOC0_CLKLOC_LOC1 << 24)    /**< Shifted mode LOC1 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC2             (_USART_ROUTELOC0_CLKLOC_LOC2 << 24)    /**< Shifted mode LOC2 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC3             (_USART_ROUTELOC0_CLKLOC_LOC3 << 24)    /**< Shifted mode LOC3 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC4             (_USART_ROUTELOC0_CLKLOC_LOC4 << 24)    /**< Shifted mode LOC4 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC5             (_USART_ROUTELOC0_CLKLOC_LOC5 << 24)    /**< Shifted mode LOC5 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC6             (_USART_ROUTELOC0_CLKLOC_LOC6 << 24)    /**< Shifted mode LOC6 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC7             (_USART_ROUTELOC0_CLKLOC_LOC7 << 24)    /**< Shifted mode LOC7 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC8             (_USART_ROUTELOC0_CLKLOC_LOC8 << 24)    /**< Shifted mode LOC8 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC9             (_USART_ROUTELOC0_CLKLOC_LOC9 << 24)    /**< Shifted mode LOC9 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC10            (_USART_ROUTELOC0_CLKLOC_LOC10 << 24)   /**< Shifted mode LOC10 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC11            (_USART_ROUTELOC0_CLKLOC_LOC11 << 24)   /**< Shifted mode LOC11 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC12            (_USART_ROUTELOC0_CLKLOC_LOC12 << 24)   /**< Shifted mode LOC12 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC13            (_USART_ROUTELOC0_CLKLOC_LOC13 << 24)   /**< Shifted mode LOC13 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC14            (_USART_ROUTELOC0_CLKLOC_LOC14 << 24)   /**< Shifted mode LOC14 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC15            (_USART_ROUTELOC0_CLKLOC_LOC15 << 24)   /**< Shifted mode LOC15 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC16            (_USART_ROUTELOC0_CLKLOC_LOC16 << 24)   /**< Shifted mode LOC16 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC17            (_USART_ROUTELOC0_CLKLOC_LOC17 << 24)   /**< Shifted mode LOC17 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC18            (_USART_ROUTELOC0_CLKLOC_LOC18 << 24)   /**< Shifted mode LOC18 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC19            (_USART_ROUTELOC0_CLKLOC_LOC19 << 24)   /**< Shifted mode LOC19 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC20            (_USART_ROUTELOC0_CLKLOC_LOC20 << 24)   /**< Shifted mode LOC20 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC21            (_USART_ROUTELOC0_CLKLOC_LOC21 << 24)   /**< Shifted mode LOC21 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC22            (_USART_ROUTELOC0_CLKLOC_LOC22 << 24)   /**< Shifted mode LOC22 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC23            (_USART_ROUTELOC0_CLKLOC_LOC23 << 24)   /**< Shifted mode LOC23 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC24            (_USART_ROUTELOC0_CLKLOC_LOC24 << 24)   /**< Shifted mode LOC24 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC25            (_USART_ROUTELOC0_CLKLOC_LOC25 << 24)   /**< Shifted mode LOC25 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC26            (_USART_ROUTELOC0_CLKLOC_LOC26 << 24)   /**< Shifted mode LOC26 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC27            (_USART_ROUTELOC0_CLKLOC_LOC27 << 24)   /**< Shifted mode LOC27 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC28            (_USART_ROUTELOC0_CLKLOC_LOC28 << 24)   /**< Shifted mode LOC28 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC29            (_USART_ROUTELOC0_CLKLOC_LOC29 << 24)   /**< Shifted mode LOC29 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC30            (_USART_ROUTELOC0_CLKLOC_LOC30 << 24)   /**< Shifted mode LOC30 for USART_ROUTELOC0 */
#define USART_ROUTELOC0_CLKLOC_LOC31            (_USART_ROUTELOC0_CLKLOC_LOC31 << 24)   /**< Shifted mode LOC31 for USART_ROUTELOC0 */

/* Bit fields for USART ROUTELOC1 */
#define _USART_ROUTELOC1_RESETVALUE             0x00000000UL                           /**< Default value for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_MASK                   0x00001F1FUL                           /**< Mask for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_SHIFT           0                                      /**< Shift value for USART_CTSLOC */
#define _USART_ROUTELOC1_CTSLOC_MASK            0x1FUL                                 /**< Bit mask for USART_CTSLOC */
#define _USART_ROUTELOC1_CTSLOC_LOC0            0x00000000UL                           /**< Mode LOC0 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC1            0x00000001UL                           /**< Mode LOC1 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC2            0x00000002UL                           /**< Mode LOC2 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC3            0x00000003UL                           /**< Mode LOC3 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC4            0x00000004UL                           /**< Mode LOC4 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC5            0x00000005UL                           /**< Mode LOC5 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC6            0x00000006UL                           /**< Mode LOC6 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC7            0x00000007UL                           /**< Mode LOC7 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC8            0x00000008UL                           /**< Mode LOC8 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC9            0x00000009UL                           /**< Mode LOC9 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC10           0x0000000AUL                           /**< Mode LOC10 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC11           0x0000000BUL                           /**< Mode LOC11 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC12           0x0000000CUL                           /**< Mode LOC12 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC13           0x0000000DUL                           /**< Mode LOC13 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC14           0x0000000EUL                           /**< Mode LOC14 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC15           0x0000000FUL                           /**< Mode LOC15 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC16           0x00000010UL                           /**< Mode LOC16 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC17           0x00000011UL                           /**< Mode LOC17 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC18           0x00000012UL                           /**< Mode LOC18 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC19           0x00000013UL                           /**< Mode LOC19 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC20           0x00000014UL                           /**< Mode LOC20 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC21           0x00000015UL                           /**< Mode LOC21 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC22           0x00000016UL                           /**< Mode LOC22 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC23           0x00000017UL                           /**< Mode LOC23 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC24           0x00000018UL                           /**< Mode LOC24 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC25           0x00000019UL                           /**< Mode LOC25 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC26           0x0000001AUL                           /**< Mode LOC26 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC27           0x0000001BUL                           /**< Mode LOC27 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC28           0x0000001CUL                           /**< Mode LOC28 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC29           0x0000001DUL                           /**< Mode LOC29 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC30           0x0000001EUL                           /**< Mode LOC30 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_CTSLOC_LOC31           0x0000001FUL                           /**< Mode LOC31 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC0             (_USART_ROUTELOC1_CTSLOC_LOC0 << 0)    /**< Shifted mode LOC0 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_DEFAULT          (_USART_ROUTELOC1_CTSLOC_DEFAULT << 0) /**< Shifted mode DEFAULT for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC1             (_USART_ROUTELOC1_CTSLOC_LOC1 << 0)    /**< Shifted mode LOC1 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC2             (_USART_ROUTELOC1_CTSLOC_LOC2 << 0)    /**< Shifted mode LOC2 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC3             (_USART_ROUTELOC1_CTSLOC_LOC3 << 0)    /**< Shifted mode LOC3 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC4             (_USART_ROUTELOC1_CTSLOC_LOC4 << 0)    /**< Shifted mode LOC4 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC5             (_USART_ROUTELOC1_CTSLOC_LOC5 << 0)    /**< Shifted mode LOC5 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC6             (_USART_ROUTELOC1_CTSLOC_LOC6 << 0)    /**< Shifted mode LOC6 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC7             (_USART_ROUTELOC1_CTSLOC_LOC7 << 0)    /**< Shifted mode LOC7 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC8             (_USART_ROUTELOC1_CTSLOC_LOC8 << 0)    /**< Shifted mode LOC8 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC9             (_USART_ROUTELOC1_CTSLOC_LOC9 << 0)    /**< Shifted mode LOC9 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC10            (_USART_ROUTELOC1_CTSLOC_LOC10 << 0)   /**< Shifted mode LOC10 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC11            (_USART_ROUTELOC1_CTSLOC_LOC11 << 0)   /**< Shifted mode LOC11 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC12            (_USART_ROUTELOC1_CTSLOC_LOC12 << 0)   /**< Shifted mode LOC12 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC13            (_USART_ROUTELOC1_CTSLOC_LOC13 << 0)   /**< Shifted mode LOC13 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC14            (_USART_ROUTELOC1_CTSLOC_LOC14 << 0)   /**< Shifted mode LOC14 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC15            (_USART_ROUTELOC1_CTSLOC_LOC15 << 0)   /**< Shifted mode LOC15 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC16            (_USART_ROUTELOC1_CTSLOC_LOC16 << 0)   /**< Shifted mode LOC16 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC17            (_USART_ROUTELOC1_CTSLOC_LOC17 << 0)   /**< Shifted mode LOC17 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC18            (_USART_ROUTELOC1_CTSLOC_LOC18 << 0)   /**< Shifted mode LOC18 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC19            (_USART_ROUTELOC1_CTSLOC_LOC19 << 0)   /**< Shifted mode LOC19 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC20            (_USART_ROUTELOC1_CTSLOC_LOC20 << 0)   /**< Shifted mode LOC20 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC21            (_USART_ROUTELOC1_CTSLOC_LOC21 << 0)   /**< Shifted mode LOC21 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC22            (_USART_ROUTELOC1_CTSLOC_LOC22 << 0)   /**< Shifted mode LOC22 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC23            (_USART_ROUTELOC1_CTSLOC_LOC23 << 0)   /**< Shifted mode LOC23 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC24            (_USART_ROUTELOC1_CTSLOC_LOC24 << 0)   /**< Shifted mode LOC24 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC25            (_USART_ROUTELOC1_CTSLOC_LOC25 << 0)   /**< Shifted mode LOC25 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC26            (_USART_ROUTELOC1_CTSLOC_LOC26 << 0)   /**< Shifted mode LOC26 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC27            (_USART_ROUTELOC1_CTSLOC_LOC27 << 0)   /**< Shifted mode LOC27 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC28            (_USART_ROUTELOC1_CTSLOC_LOC28 << 0)   /**< Shifted mode LOC28 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC29            (_USART_ROUTELOC1_CTSLOC_LOC29 << 0)   /**< Shifted mode LOC29 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC30            (_USART_ROUTELOC1_CTSLOC_LOC30 << 0)   /**< Shifted mode LOC30 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_CTSLOC_LOC31            (_USART_ROUTELOC1_CTSLOC_LOC31 << 0)   /**< Shifted mode LOC31 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_SHIFT           8                                      /**< Shift value for USART_RTSLOC */
#define _USART_ROUTELOC1_RTSLOC_MASK            0x1F00UL                               /**< Bit mask for USART_RTSLOC */
#define _USART_ROUTELOC1_RTSLOC_LOC0            0x00000000UL                           /**< Mode LOC0 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC1            0x00000001UL                           /**< Mode LOC1 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC2            0x00000002UL                           /**< Mode LOC2 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC3            0x00000003UL                           /**< Mode LOC3 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC4            0x00000004UL                           /**< Mode LOC4 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC5            0x00000005UL                           /**< Mode LOC5 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC6            0x00000006UL                           /**< Mode LOC6 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC7            0x00000007UL                           /**< Mode LOC7 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC8            0x00000008UL                           /**< Mode LOC8 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC9            0x00000009UL                           /**< Mode LOC9 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC10           0x0000000AUL                           /**< Mode LOC10 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC11           0x0000000BUL                           /**< Mode LOC11 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC12           0x0000000CUL                           /**< Mode LOC12 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC13           0x0000000DUL                           /**< Mode LOC13 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC14           0x0000000EUL                           /**< Mode LOC14 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC15           0x0000000FUL                           /**< Mode LOC15 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC16           0x00000010UL                           /**< Mode LOC16 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC17           0x00000011UL                           /**< Mode LOC17 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC18           0x00000012UL                           /**< Mode LOC18 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC19           0x00000013UL                           /**< Mode LOC19 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC20           0x00000014UL                           /**< Mode LOC20 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC21           0x00000015UL                           /**< Mode LOC21 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC22           0x00000016UL                           /**< Mode LOC22 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC23           0x00000017UL                           /**< Mode LOC23 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC24           0x00000018UL                           /**< Mode LOC24 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC25           0x00000019UL                           /**< Mode LOC25 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC26           0x0000001AUL                           /**< Mode LOC26 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC27           0x0000001BUL                           /**< Mode LOC27 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC28           0x0000001CUL                           /**< Mode LOC28 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC29           0x0000001DUL                           /**< Mode LOC29 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC30           0x0000001EUL                           /**< Mode LOC30 for USART_ROUTELOC1 */
#define _USART_ROUTELOC1_RTSLOC_LOC31           0x0000001FUL                           /**< Mode LOC31 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC0             (_USART_ROUTELOC1_RTSLOC_LOC0 << 8)    /**< Shifted mode LOC0 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_DEFAULT          (_USART_ROUTELOC1_RTSLOC_DEFAULT << 8) /**< Shifted mode DEFAULT for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC1             (_USART_ROUTELOC1_RTSLOC_LOC1 << 8)    /**< Shifted mode LOC1 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC2             (_USART_ROUTELOC1_RTSLOC_LOC2 << 8)    /**< Shifted mode LOC2 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC3             (_USART_ROUTELOC1_RTSLOC_LOC3 << 8)    /**< Shifted mode LOC3 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC4             (_USART_ROUTELOC1_RTSLOC_LOC4 << 8)    /**< Shifted mode LOC4 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC5             (_USART_ROUTELOC1_RTSLOC_LOC5 << 8)    /**< Shifted mode LOC5 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC6             (_USART_ROUTELOC1_RTSLOC_LOC6 << 8)    /**< Shifted mode LOC6 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC7             (_USART_ROUTELOC1_RTSLOC_LOC7 << 8)    /**< Shifted mode LOC7 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC8             (_USART_ROUTELOC1_RTSLOC_LOC8 << 8)    /**< Shifted mode LOC8 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC9             (_USART_ROUTELOC1_RTSLOC_LOC9 << 8)    /**< Shifted mode LOC9 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC10            (_USART_ROUTELOC1_RTSLOC_LOC10 << 8)   /**< Shifted mode LOC10 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC11            (_USART_ROUTELOC1_RTSLOC_LOC11 << 8)   /**< Shifted mode LOC11 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC12            (_USART_ROUTELOC1_RTSLOC_LOC12 << 8)   /**< Shifted mode LOC12 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC13            (_USART_ROUTELOC1_RTSLOC_LOC13 << 8)   /**< Shifted mode LOC13 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC14            (_USART_ROUTELOC1_RTSLOC_LOC14 << 8)   /**< Shifted mode LOC14 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC15            (_USART_ROUTELOC1_RTSLOC_LOC15 << 8)   /**< Shifted mode LOC15 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC16            (_USART_ROUTELOC1_RTSLOC_LOC16 << 8)   /**< Shifted mode LOC16 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC17            (_USART_ROUTELOC1_RTSLOC_LOC17 << 8)   /**< Shifted mode LOC17 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC18            (_USART_ROUTELOC1_RTSLOC_LOC18 << 8)   /**< Shifted mode LOC18 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC19            (_USART_ROUTELOC1_RTSLOC_LOC19 << 8)   /**< Shifted mode LOC19 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC20            (_USART_ROUTELOC1_RTSLOC_LOC20 << 8)   /**< Shifted mode LOC20 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC21            (_USART_ROUTELOC1_RTSLOC_LOC21 << 8)   /**< Shifted mode LOC21 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC22            (_USART_ROUTELOC1_RTSLOC_LOC22 << 8)   /**< Shifted mode LOC22 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC23            (_USART_ROUTELOC1_RTSLOC_LOC23 << 8)   /**< Shifted mode LOC23 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC24            (_USART_ROUTELOC1_RTSLOC_LOC24 << 8)   /**< Shifted mode LOC24 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC25            (_USART_ROUTELOC1_RTSLOC_LOC25 << 8)   /**< Shifted mode LOC25 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC26            (_USART_ROUTELOC1_RTSLOC_LOC26 << 8)   /**< Shifted mode LOC26 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC27            (_USART_ROUTELOC1_RTSLOC_LOC27 << 8)   /**< Shifted mode LOC27 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC28            (_USART_ROUTELOC1_RTSLOC_LOC28 << 8)   /**< Shifted mode LOC28 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC29            (_USART_ROUTELOC1_RTSLOC_LOC29 << 8)   /**< Shifted mode LOC29 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC30            (_USART_ROUTELOC1_RTSLOC_LOC30 << 8)   /**< Shifted mode LOC30 for USART_ROUTELOC1 */
#define USART_ROUTELOC1_RTSLOC_LOC31            (_USART_ROUTELOC1_RTSLOC_LOC31 << 8)   /**< Shifted mode LOC31 for USART_ROUTELOC1 */

/** @} End of group EFR32FG1P_USART */
/** @} End of group Parts */

