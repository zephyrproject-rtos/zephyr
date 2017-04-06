/**************************************************************************//**
 * @file efm32wg_uart.h
 * @brief EFM32WG_UART register and bit field definitions
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
 * @defgroup EFM32WG_UART_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for UART CTRL */
#define _UART_CTRL_RESETVALUE                0x00000000UL                            /**< Default value for UART_CTRL */
#define _UART_CTRL_MASK                      0xFFFFFF7FUL                            /**< Mask for UART_CTRL */
#define UART_CTRL_SYNC                       (0x1UL << 0)                            /**< USART Synchronous Mode */
#define _UART_CTRL_SYNC_SHIFT                0                                       /**< Shift value for USART_SYNC */
#define _UART_CTRL_SYNC_MASK                 0x1UL                                   /**< Bit mask for USART_SYNC */
#define _UART_CTRL_SYNC_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_SYNC_DEFAULT               (_UART_CTRL_SYNC_DEFAULT << 0)          /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_LOOPBK                     (0x1UL << 1)                            /**< Loopback Enable */
#define _UART_CTRL_LOOPBK_SHIFT              1                                       /**< Shift value for USART_LOOPBK */
#define _UART_CTRL_LOOPBK_MASK               0x2UL                                   /**< Bit mask for USART_LOOPBK */
#define _UART_CTRL_LOOPBK_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_LOOPBK_DEFAULT             (_UART_CTRL_LOOPBK_DEFAULT << 1)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_CCEN                       (0x1UL << 2)                            /**< Collision Check Enable */
#define _UART_CTRL_CCEN_SHIFT                2                                       /**< Shift value for USART_CCEN */
#define _UART_CTRL_CCEN_MASK                 0x4UL                                   /**< Bit mask for USART_CCEN */
#define _UART_CTRL_CCEN_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_CCEN_DEFAULT               (_UART_CTRL_CCEN_DEFAULT << 2)          /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_MPM                        (0x1UL << 3)                            /**< Multi-Processor Mode */
#define _UART_CTRL_MPM_SHIFT                 3                                       /**< Shift value for USART_MPM */
#define _UART_CTRL_MPM_MASK                  0x8UL                                   /**< Bit mask for USART_MPM */
#define _UART_CTRL_MPM_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_MPM_DEFAULT                (_UART_CTRL_MPM_DEFAULT << 3)           /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_MPAB                       (0x1UL << 4)                            /**< Multi-Processor Address-Bit */
#define _UART_CTRL_MPAB_SHIFT                4                                       /**< Shift value for USART_MPAB */
#define _UART_CTRL_MPAB_MASK                 0x10UL                                  /**< Bit mask for USART_MPAB */
#define _UART_CTRL_MPAB_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_MPAB_DEFAULT               (_UART_CTRL_MPAB_DEFAULT << 4)          /**< Shifted mode DEFAULT for UART_CTRL */
#define _UART_CTRL_OVS_SHIFT                 5                                       /**< Shift value for USART_OVS */
#define _UART_CTRL_OVS_MASK                  0x60UL                                  /**< Bit mask for USART_OVS */
#define _UART_CTRL_OVS_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define _UART_CTRL_OVS_X16                   0x00000000UL                            /**< Mode X16 for UART_CTRL */
#define _UART_CTRL_OVS_X8                    0x00000001UL                            /**< Mode X8 for UART_CTRL */
#define _UART_CTRL_OVS_X6                    0x00000002UL                            /**< Mode X6 for UART_CTRL */
#define _UART_CTRL_OVS_X4                    0x00000003UL                            /**< Mode X4 for UART_CTRL */
#define UART_CTRL_OVS_DEFAULT                (_UART_CTRL_OVS_DEFAULT << 5)           /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_OVS_X16                    (_UART_CTRL_OVS_X16 << 5)               /**< Shifted mode X16 for UART_CTRL */
#define UART_CTRL_OVS_X8                     (_UART_CTRL_OVS_X8 << 5)                /**< Shifted mode X8 for UART_CTRL */
#define UART_CTRL_OVS_X6                     (_UART_CTRL_OVS_X6 << 5)                /**< Shifted mode X6 for UART_CTRL */
#define UART_CTRL_OVS_X4                     (_UART_CTRL_OVS_X4 << 5)                /**< Shifted mode X4 for UART_CTRL */
#define UART_CTRL_CLKPOL                     (0x1UL << 8)                            /**< Clock Polarity */
#define _UART_CTRL_CLKPOL_SHIFT              8                                       /**< Shift value for USART_CLKPOL */
#define _UART_CTRL_CLKPOL_MASK               0x100UL                                 /**< Bit mask for USART_CLKPOL */
#define _UART_CTRL_CLKPOL_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define _UART_CTRL_CLKPOL_IDLELOW            0x00000000UL                            /**< Mode IDLELOW for UART_CTRL */
#define _UART_CTRL_CLKPOL_IDLEHIGH           0x00000001UL                            /**< Mode IDLEHIGH for UART_CTRL */
#define UART_CTRL_CLKPOL_DEFAULT             (_UART_CTRL_CLKPOL_DEFAULT << 8)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_CLKPOL_IDLELOW             (_UART_CTRL_CLKPOL_IDLELOW << 8)        /**< Shifted mode IDLELOW for UART_CTRL */
#define UART_CTRL_CLKPOL_IDLEHIGH            (_UART_CTRL_CLKPOL_IDLEHIGH << 8)       /**< Shifted mode IDLEHIGH for UART_CTRL */
#define UART_CTRL_CLKPHA                     (0x1UL << 9)                            /**< Clock Edge For Setup/Sample */
#define _UART_CTRL_CLKPHA_SHIFT              9                                       /**< Shift value for USART_CLKPHA */
#define _UART_CTRL_CLKPHA_MASK               0x200UL                                 /**< Bit mask for USART_CLKPHA */
#define _UART_CTRL_CLKPHA_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define _UART_CTRL_CLKPHA_SAMPLELEADING      0x00000000UL                            /**< Mode SAMPLELEADING for UART_CTRL */
#define _UART_CTRL_CLKPHA_SAMPLETRAILING     0x00000001UL                            /**< Mode SAMPLETRAILING for UART_CTRL */
#define UART_CTRL_CLKPHA_DEFAULT             (_UART_CTRL_CLKPHA_DEFAULT << 9)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_CLKPHA_SAMPLELEADING       (_UART_CTRL_CLKPHA_SAMPLELEADING << 9)  /**< Shifted mode SAMPLELEADING for UART_CTRL */
#define UART_CTRL_CLKPHA_SAMPLETRAILING      (_UART_CTRL_CLKPHA_SAMPLETRAILING << 9) /**< Shifted mode SAMPLETRAILING for UART_CTRL */
#define UART_CTRL_MSBF                       (0x1UL << 10)                           /**< Most Significant Bit First */
#define _UART_CTRL_MSBF_SHIFT                10                                      /**< Shift value for USART_MSBF */
#define _UART_CTRL_MSBF_MASK                 0x400UL                                 /**< Bit mask for USART_MSBF */
#define _UART_CTRL_MSBF_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_MSBF_DEFAULT               (_UART_CTRL_MSBF_DEFAULT << 10)         /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_CSMA                       (0x1UL << 11)                           /**< Action On Slave-Select In Master Mode */
#define _UART_CTRL_CSMA_SHIFT                11                                      /**< Shift value for USART_CSMA */
#define _UART_CTRL_CSMA_MASK                 0x800UL                                 /**< Bit mask for USART_CSMA */
#define _UART_CTRL_CSMA_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define _UART_CTRL_CSMA_NOACTION             0x00000000UL                            /**< Mode NOACTION for UART_CTRL */
#define _UART_CTRL_CSMA_GOTOSLAVEMODE        0x00000001UL                            /**< Mode GOTOSLAVEMODE for UART_CTRL */
#define UART_CTRL_CSMA_DEFAULT               (_UART_CTRL_CSMA_DEFAULT << 11)         /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_CSMA_NOACTION              (_UART_CTRL_CSMA_NOACTION << 11)        /**< Shifted mode NOACTION for UART_CTRL */
#define UART_CTRL_CSMA_GOTOSLAVEMODE         (_UART_CTRL_CSMA_GOTOSLAVEMODE << 11)   /**< Shifted mode GOTOSLAVEMODE for UART_CTRL */
#define UART_CTRL_TXBIL                      (0x1UL << 12)                           /**< TX Buffer Interrupt Level */
#define _UART_CTRL_TXBIL_SHIFT               12                                      /**< Shift value for USART_TXBIL */
#define _UART_CTRL_TXBIL_MASK                0x1000UL                                /**< Bit mask for USART_TXBIL */
#define _UART_CTRL_TXBIL_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define _UART_CTRL_TXBIL_EMPTY               0x00000000UL                            /**< Mode EMPTY for UART_CTRL */
#define _UART_CTRL_TXBIL_HALFFULL            0x00000001UL                            /**< Mode HALFFULL for UART_CTRL */
#define UART_CTRL_TXBIL_DEFAULT              (_UART_CTRL_TXBIL_DEFAULT << 12)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_TXBIL_EMPTY                (_UART_CTRL_TXBIL_EMPTY << 12)          /**< Shifted mode EMPTY for UART_CTRL */
#define UART_CTRL_TXBIL_HALFFULL             (_UART_CTRL_TXBIL_HALFFULL << 12)       /**< Shifted mode HALFFULL for UART_CTRL */
#define UART_CTRL_RXINV                      (0x1UL << 13)                           /**< Receiver Input Invert */
#define _UART_CTRL_RXINV_SHIFT               13                                      /**< Shift value for USART_RXINV */
#define _UART_CTRL_RXINV_MASK                0x2000UL                                /**< Bit mask for USART_RXINV */
#define _UART_CTRL_RXINV_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_RXINV_DEFAULT              (_UART_CTRL_RXINV_DEFAULT << 13)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_TXINV                      (0x1UL << 14)                           /**< Transmitter output Invert */
#define _UART_CTRL_TXINV_SHIFT               14                                      /**< Shift value for USART_TXINV */
#define _UART_CTRL_TXINV_MASK                0x4000UL                                /**< Bit mask for USART_TXINV */
#define _UART_CTRL_TXINV_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_TXINV_DEFAULT              (_UART_CTRL_TXINV_DEFAULT << 14)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_CSINV                      (0x1UL << 15)                           /**< Chip Select Invert */
#define _UART_CTRL_CSINV_SHIFT               15                                      /**< Shift value for USART_CSINV */
#define _UART_CTRL_CSINV_MASK                0x8000UL                                /**< Bit mask for USART_CSINV */
#define _UART_CTRL_CSINV_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_CSINV_DEFAULT              (_UART_CTRL_CSINV_DEFAULT << 15)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_AUTOCS                     (0x1UL << 16)                           /**< Automatic Chip Select */
#define _UART_CTRL_AUTOCS_SHIFT              16                                      /**< Shift value for USART_AUTOCS */
#define _UART_CTRL_AUTOCS_MASK               0x10000UL                               /**< Bit mask for USART_AUTOCS */
#define _UART_CTRL_AUTOCS_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_AUTOCS_DEFAULT             (_UART_CTRL_AUTOCS_DEFAULT << 16)       /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_AUTOTRI                    (0x1UL << 17)                           /**< Automatic TX Tristate */
#define _UART_CTRL_AUTOTRI_SHIFT             17                                      /**< Shift value for USART_AUTOTRI */
#define _UART_CTRL_AUTOTRI_MASK              0x20000UL                               /**< Bit mask for USART_AUTOTRI */
#define _UART_CTRL_AUTOTRI_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_AUTOTRI_DEFAULT            (_UART_CTRL_AUTOTRI_DEFAULT << 17)      /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_SCMODE                     (0x1UL << 18)                           /**< SmartCard Mode */
#define _UART_CTRL_SCMODE_SHIFT              18                                      /**< Shift value for USART_SCMODE */
#define _UART_CTRL_SCMODE_MASK               0x40000UL                               /**< Bit mask for USART_SCMODE */
#define _UART_CTRL_SCMODE_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_SCMODE_DEFAULT             (_UART_CTRL_SCMODE_DEFAULT << 18)       /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_SCRETRANS                  (0x1UL << 19)                           /**< SmartCard Retransmit */
#define _UART_CTRL_SCRETRANS_SHIFT           19                                      /**< Shift value for USART_SCRETRANS */
#define _UART_CTRL_SCRETRANS_MASK            0x80000UL                               /**< Bit mask for USART_SCRETRANS */
#define _UART_CTRL_SCRETRANS_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_SCRETRANS_DEFAULT          (_UART_CTRL_SCRETRANS_DEFAULT << 19)    /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_SKIPPERRF                  (0x1UL << 20)                           /**< Skip Parity Error Frames */
#define _UART_CTRL_SKIPPERRF_SHIFT           20                                      /**< Shift value for USART_SKIPPERRF */
#define _UART_CTRL_SKIPPERRF_MASK            0x100000UL                              /**< Bit mask for USART_SKIPPERRF */
#define _UART_CTRL_SKIPPERRF_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_SKIPPERRF_DEFAULT          (_UART_CTRL_SKIPPERRF_DEFAULT << 20)    /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_BIT8DV                     (0x1UL << 21)                           /**< Bit 8 Default Value */
#define _UART_CTRL_BIT8DV_SHIFT              21                                      /**< Shift value for USART_BIT8DV */
#define _UART_CTRL_BIT8DV_MASK               0x200000UL                              /**< Bit mask for USART_BIT8DV */
#define _UART_CTRL_BIT8DV_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_BIT8DV_DEFAULT             (_UART_CTRL_BIT8DV_DEFAULT << 21)       /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_ERRSDMA                    (0x1UL << 22)                           /**< Halt DMA On Error */
#define _UART_CTRL_ERRSDMA_SHIFT             22                                      /**< Shift value for USART_ERRSDMA */
#define _UART_CTRL_ERRSDMA_MASK              0x400000UL                              /**< Bit mask for USART_ERRSDMA */
#define _UART_CTRL_ERRSDMA_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_ERRSDMA_DEFAULT            (_UART_CTRL_ERRSDMA_DEFAULT << 22)      /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_ERRSRX                     (0x1UL << 23)                           /**< Disable RX On Error */
#define _UART_CTRL_ERRSRX_SHIFT              23                                      /**< Shift value for USART_ERRSRX */
#define _UART_CTRL_ERRSRX_MASK               0x800000UL                              /**< Bit mask for USART_ERRSRX */
#define _UART_CTRL_ERRSRX_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_ERRSRX_DEFAULT             (_UART_CTRL_ERRSRX_DEFAULT << 23)       /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_ERRSTX                     (0x1UL << 24)                           /**< Disable TX On Error */
#define _UART_CTRL_ERRSTX_SHIFT              24                                      /**< Shift value for USART_ERRSTX */
#define _UART_CTRL_ERRSTX_MASK               0x1000000UL                             /**< Bit mask for USART_ERRSTX */
#define _UART_CTRL_ERRSTX_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_ERRSTX_DEFAULT             (_UART_CTRL_ERRSTX_DEFAULT << 24)       /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_SSSEARLY                   (0x1UL << 25)                           /**< Synchronous Slave Setup Early */
#define _UART_CTRL_SSSEARLY_SHIFT            25                                      /**< Shift value for USART_SSSEARLY */
#define _UART_CTRL_SSSEARLY_MASK             0x2000000UL                             /**< Bit mask for USART_SSSEARLY */
#define _UART_CTRL_SSSEARLY_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_SSSEARLY_DEFAULT           (_UART_CTRL_SSSEARLY_DEFAULT << 25)     /**< Shifted mode DEFAULT for UART_CTRL */
#define _UART_CTRL_TXDELAY_SHIFT             26                                      /**< Shift value for USART_TXDELAY */
#define _UART_CTRL_TXDELAY_MASK              0xC000000UL                             /**< Bit mask for USART_TXDELAY */
#define _UART_CTRL_TXDELAY_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define _UART_CTRL_TXDELAY_NONE              0x00000000UL                            /**< Mode NONE for UART_CTRL */
#define _UART_CTRL_TXDELAY_SINGLE            0x00000001UL                            /**< Mode SINGLE for UART_CTRL */
#define _UART_CTRL_TXDELAY_DOUBLE            0x00000002UL                            /**< Mode DOUBLE for UART_CTRL */
#define _UART_CTRL_TXDELAY_TRIPLE            0x00000003UL                            /**< Mode TRIPLE for UART_CTRL */
#define UART_CTRL_TXDELAY_DEFAULT            (_UART_CTRL_TXDELAY_DEFAULT << 26)      /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_TXDELAY_NONE               (_UART_CTRL_TXDELAY_NONE << 26)         /**< Shifted mode NONE for UART_CTRL */
#define UART_CTRL_TXDELAY_SINGLE             (_UART_CTRL_TXDELAY_SINGLE << 26)       /**< Shifted mode SINGLE for UART_CTRL */
#define UART_CTRL_TXDELAY_DOUBLE             (_UART_CTRL_TXDELAY_DOUBLE << 26)       /**< Shifted mode DOUBLE for UART_CTRL */
#define UART_CTRL_TXDELAY_TRIPLE             (_UART_CTRL_TXDELAY_TRIPLE << 26)       /**< Shifted mode TRIPLE for UART_CTRL */
#define UART_CTRL_BYTESWAP                   (0x1UL << 28)                           /**< Byteswap In Double Accesses */
#define _UART_CTRL_BYTESWAP_SHIFT            28                                      /**< Shift value for USART_BYTESWAP */
#define _UART_CTRL_BYTESWAP_MASK             0x10000000UL                            /**< Bit mask for USART_BYTESWAP */
#define _UART_CTRL_BYTESWAP_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_BYTESWAP_DEFAULT           (_UART_CTRL_BYTESWAP_DEFAULT << 28)     /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_AUTOTX                     (0x1UL << 29)                           /**< Always Transmit When RX Not Full */
#define _UART_CTRL_AUTOTX_SHIFT              29                                      /**< Shift value for USART_AUTOTX */
#define _UART_CTRL_AUTOTX_MASK               0x20000000UL                            /**< Bit mask for USART_AUTOTX */
#define _UART_CTRL_AUTOTX_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_AUTOTX_DEFAULT             (_UART_CTRL_AUTOTX_DEFAULT << 29)       /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_MVDIS                      (0x1UL << 30)                           /**< Majority Vote Disable */
#define _UART_CTRL_MVDIS_SHIFT               30                                      /**< Shift value for USART_MVDIS */
#define _UART_CTRL_MVDIS_MASK                0x40000000UL                            /**< Bit mask for USART_MVDIS */
#define _UART_CTRL_MVDIS_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_MVDIS_DEFAULT              (_UART_CTRL_MVDIS_DEFAULT << 30)        /**< Shifted mode DEFAULT for UART_CTRL */
#define UART_CTRL_SMSDELAY                   (0x1UL << 31)                           /**< Synchronous Master Sample Delay */
#define _UART_CTRL_SMSDELAY_SHIFT            31                                      /**< Shift value for USART_SMSDELAY */
#define _UART_CTRL_SMSDELAY_MASK             0x80000000UL                            /**< Bit mask for USART_SMSDELAY */
#define _UART_CTRL_SMSDELAY_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for UART_CTRL */
#define UART_CTRL_SMSDELAY_DEFAULT           (_UART_CTRL_SMSDELAY_DEFAULT << 31)     /**< Shifted mode DEFAULT for UART_CTRL */

/* Bit fields for UART FRAME */
#define _UART_FRAME_RESETVALUE               0x00001005UL                             /**< Default value for UART_FRAME */
#define _UART_FRAME_MASK                     0x0000330FUL                             /**< Mask for UART_FRAME */
#define _UART_FRAME_DATABITS_SHIFT           0                                        /**< Shift value for USART_DATABITS */
#define _UART_FRAME_DATABITS_MASK            0xFUL                                    /**< Bit mask for USART_DATABITS */
#define _UART_FRAME_DATABITS_FOUR            0x00000001UL                             /**< Mode FOUR for UART_FRAME */
#define _UART_FRAME_DATABITS_FIVE            0x00000002UL                             /**< Mode FIVE for UART_FRAME */
#define _UART_FRAME_DATABITS_SIX             0x00000003UL                             /**< Mode SIX for UART_FRAME */
#define _UART_FRAME_DATABITS_SEVEN           0x00000004UL                             /**< Mode SEVEN for UART_FRAME */
#define _UART_FRAME_DATABITS_DEFAULT         0x00000005UL                             /**< Mode DEFAULT for UART_FRAME */
#define _UART_FRAME_DATABITS_EIGHT           0x00000005UL                             /**< Mode EIGHT for UART_FRAME */
#define _UART_FRAME_DATABITS_NINE            0x00000006UL                             /**< Mode NINE for UART_FRAME */
#define _UART_FRAME_DATABITS_TEN             0x00000007UL                             /**< Mode TEN for UART_FRAME */
#define _UART_FRAME_DATABITS_ELEVEN          0x00000008UL                             /**< Mode ELEVEN for UART_FRAME */
#define _UART_FRAME_DATABITS_TWELVE          0x00000009UL                             /**< Mode TWELVE for UART_FRAME */
#define _UART_FRAME_DATABITS_THIRTEEN        0x0000000AUL                             /**< Mode THIRTEEN for UART_FRAME */
#define _UART_FRAME_DATABITS_FOURTEEN        0x0000000BUL                             /**< Mode FOURTEEN for UART_FRAME */
#define _UART_FRAME_DATABITS_FIFTEEN         0x0000000CUL                             /**< Mode FIFTEEN for UART_FRAME */
#define _UART_FRAME_DATABITS_SIXTEEN         0x0000000DUL                             /**< Mode SIXTEEN for UART_FRAME */
#define UART_FRAME_DATABITS_FOUR             (_UART_FRAME_DATABITS_FOUR << 0)         /**< Shifted mode FOUR for UART_FRAME */
#define UART_FRAME_DATABITS_FIVE             (_UART_FRAME_DATABITS_FIVE << 0)         /**< Shifted mode FIVE for UART_FRAME */
#define UART_FRAME_DATABITS_SIX              (_UART_FRAME_DATABITS_SIX << 0)          /**< Shifted mode SIX for UART_FRAME */
#define UART_FRAME_DATABITS_SEVEN            (_UART_FRAME_DATABITS_SEVEN << 0)        /**< Shifted mode SEVEN for UART_FRAME */
#define UART_FRAME_DATABITS_DEFAULT          (_UART_FRAME_DATABITS_DEFAULT << 0)      /**< Shifted mode DEFAULT for UART_FRAME */
#define UART_FRAME_DATABITS_EIGHT            (_UART_FRAME_DATABITS_EIGHT << 0)        /**< Shifted mode EIGHT for UART_FRAME */
#define UART_FRAME_DATABITS_NINE             (_UART_FRAME_DATABITS_NINE << 0)         /**< Shifted mode NINE for UART_FRAME */
#define UART_FRAME_DATABITS_TEN              (_UART_FRAME_DATABITS_TEN << 0)          /**< Shifted mode TEN for UART_FRAME */
#define UART_FRAME_DATABITS_ELEVEN           (_UART_FRAME_DATABITS_ELEVEN << 0)       /**< Shifted mode ELEVEN for UART_FRAME */
#define UART_FRAME_DATABITS_TWELVE           (_UART_FRAME_DATABITS_TWELVE << 0)       /**< Shifted mode TWELVE for UART_FRAME */
#define UART_FRAME_DATABITS_THIRTEEN         (_UART_FRAME_DATABITS_THIRTEEN << 0)     /**< Shifted mode THIRTEEN for UART_FRAME */
#define UART_FRAME_DATABITS_FOURTEEN         (_UART_FRAME_DATABITS_FOURTEEN << 0)     /**< Shifted mode FOURTEEN for UART_FRAME */
#define UART_FRAME_DATABITS_FIFTEEN          (_UART_FRAME_DATABITS_FIFTEEN << 0)      /**< Shifted mode FIFTEEN for UART_FRAME */
#define UART_FRAME_DATABITS_SIXTEEN          (_UART_FRAME_DATABITS_SIXTEEN << 0)      /**< Shifted mode SIXTEEN for UART_FRAME */
#define _UART_FRAME_PARITY_SHIFT             8                                        /**< Shift value for USART_PARITY */
#define _UART_FRAME_PARITY_MASK              0x300UL                                  /**< Bit mask for USART_PARITY */
#define _UART_FRAME_PARITY_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for UART_FRAME */
#define _UART_FRAME_PARITY_NONE              0x00000000UL                             /**< Mode NONE for UART_FRAME */
#define _UART_FRAME_PARITY_EVEN              0x00000002UL                             /**< Mode EVEN for UART_FRAME */
#define _UART_FRAME_PARITY_ODD               0x00000003UL                             /**< Mode ODD for UART_FRAME */
#define UART_FRAME_PARITY_DEFAULT            (_UART_FRAME_PARITY_DEFAULT << 8)        /**< Shifted mode DEFAULT for UART_FRAME */
#define UART_FRAME_PARITY_NONE               (_UART_FRAME_PARITY_NONE << 8)           /**< Shifted mode NONE for UART_FRAME */
#define UART_FRAME_PARITY_EVEN               (_UART_FRAME_PARITY_EVEN << 8)           /**< Shifted mode EVEN for UART_FRAME */
#define UART_FRAME_PARITY_ODD                (_UART_FRAME_PARITY_ODD << 8)            /**< Shifted mode ODD for UART_FRAME */
#define _UART_FRAME_STOPBITS_SHIFT           12                                       /**< Shift value for USART_STOPBITS */
#define _UART_FRAME_STOPBITS_MASK            0x3000UL                                 /**< Bit mask for USART_STOPBITS */
#define _UART_FRAME_STOPBITS_HALF            0x00000000UL                             /**< Mode HALF for UART_FRAME */
#define _UART_FRAME_STOPBITS_DEFAULT         0x00000001UL                             /**< Mode DEFAULT for UART_FRAME */
#define _UART_FRAME_STOPBITS_ONE             0x00000001UL                             /**< Mode ONE for UART_FRAME */
#define _UART_FRAME_STOPBITS_ONEANDAHALF     0x00000002UL                             /**< Mode ONEANDAHALF for UART_FRAME */
#define _UART_FRAME_STOPBITS_TWO             0x00000003UL                             /**< Mode TWO for UART_FRAME */
#define UART_FRAME_STOPBITS_HALF             (_UART_FRAME_STOPBITS_HALF << 12)        /**< Shifted mode HALF for UART_FRAME */
#define UART_FRAME_STOPBITS_DEFAULT          (_UART_FRAME_STOPBITS_DEFAULT << 12)     /**< Shifted mode DEFAULT for UART_FRAME */
#define UART_FRAME_STOPBITS_ONE              (_UART_FRAME_STOPBITS_ONE << 12)         /**< Shifted mode ONE for UART_FRAME */
#define UART_FRAME_STOPBITS_ONEANDAHALF      (_UART_FRAME_STOPBITS_ONEANDAHALF << 12) /**< Shifted mode ONEANDAHALF for UART_FRAME */
#define UART_FRAME_STOPBITS_TWO              (_UART_FRAME_STOPBITS_TWO << 12)         /**< Shifted mode TWO for UART_FRAME */

/* Bit fields for UART TRIGCTRL */
#define _UART_TRIGCTRL_RESETVALUE            0x00000000UL                            /**< Default value for UART_TRIGCTRL */
#define _UART_TRIGCTRL_MASK                  0x00000077UL                            /**< Mask for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_SHIFT            0                                       /**< Shift value for USART_TSEL */
#define _UART_TRIGCTRL_TSEL_MASK             0x7UL                                   /**< Bit mask for USART_TSEL */
#define _UART_TRIGCTRL_TSEL_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH0           0x00000000UL                            /**< Mode PRSCH0 for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH1           0x00000001UL                            /**< Mode PRSCH1 for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH2           0x00000002UL                            /**< Mode PRSCH2 for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH3           0x00000003UL                            /**< Mode PRSCH3 for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH4           0x00000004UL                            /**< Mode PRSCH4 for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH5           0x00000005UL                            /**< Mode PRSCH5 for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH6           0x00000006UL                            /**< Mode PRSCH6 for UART_TRIGCTRL */
#define _UART_TRIGCTRL_TSEL_PRSCH7           0x00000007UL                            /**< Mode PRSCH7 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_DEFAULT           (_UART_TRIGCTRL_TSEL_DEFAULT << 0)      /**< Shifted mode DEFAULT for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH0            (_UART_TRIGCTRL_TSEL_PRSCH0 << 0)       /**< Shifted mode PRSCH0 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH1            (_UART_TRIGCTRL_TSEL_PRSCH1 << 0)       /**< Shifted mode PRSCH1 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH2            (_UART_TRIGCTRL_TSEL_PRSCH2 << 0)       /**< Shifted mode PRSCH2 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH3            (_UART_TRIGCTRL_TSEL_PRSCH3 << 0)       /**< Shifted mode PRSCH3 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH4            (_UART_TRIGCTRL_TSEL_PRSCH4 << 0)       /**< Shifted mode PRSCH4 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH5            (_UART_TRIGCTRL_TSEL_PRSCH5 << 0)       /**< Shifted mode PRSCH5 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH6            (_UART_TRIGCTRL_TSEL_PRSCH6 << 0)       /**< Shifted mode PRSCH6 for UART_TRIGCTRL */
#define UART_TRIGCTRL_TSEL_PRSCH7            (_UART_TRIGCTRL_TSEL_PRSCH7 << 0)       /**< Shifted mode PRSCH7 for UART_TRIGCTRL */
#define UART_TRIGCTRL_RXTEN                  (0x1UL << 4)                            /**< Receive Trigger Enable */
#define _UART_TRIGCTRL_RXTEN_SHIFT           4                                       /**< Shift value for USART_RXTEN */
#define _UART_TRIGCTRL_RXTEN_MASK            0x10UL                                  /**< Bit mask for USART_RXTEN */
#define _UART_TRIGCTRL_RXTEN_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for UART_TRIGCTRL */
#define UART_TRIGCTRL_RXTEN_DEFAULT          (_UART_TRIGCTRL_RXTEN_DEFAULT << 4)     /**< Shifted mode DEFAULT for UART_TRIGCTRL */
#define UART_TRIGCTRL_TXTEN                  (0x1UL << 5)                            /**< Transmit Trigger Enable */
#define _UART_TRIGCTRL_TXTEN_SHIFT           5                                       /**< Shift value for USART_TXTEN */
#define _UART_TRIGCTRL_TXTEN_MASK            0x20UL                                  /**< Bit mask for USART_TXTEN */
#define _UART_TRIGCTRL_TXTEN_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for UART_TRIGCTRL */
#define UART_TRIGCTRL_TXTEN_DEFAULT          (_UART_TRIGCTRL_TXTEN_DEFAULT << 5)     /**< Shifted mode DEFAULT for UART_TRIGCTRL */
#define UART_TRIGCTRL_AUTOTXTEN              (0x1UL << 6)                            /**< AUTOTX Trigger Enable */
#define _UART_TRIGCTRL_AUTOTXTEN_SHIFT       6                                       /**< Shift value for USART_AUTOTXTEN */
#define _UART_TRIGCTRL_AUTOTXTEN_MASK        0x40UL                                  /**< Bit mask for USART_AUTOTXTEN */
#define _UART_TRIGCTRL_AUTOTXTEN_DEFAULT     0x00000000UL                            /**< Mode DEFAULT for UART_TRIGCTRL */
#define UART_TRIGCTRL_AUTOTXTEN_DEFAULT      (_UART_TRIGCTRL_AUTOTXTEN_DEFAULT << 6) /**< Shifted mode DEFAULT for UART_TRIGCTRL */

/* Bit fields for UART CMD */
#define _UART_CMD_RESETVALUE                 0x00000000UL                        /**< Default value for UART_CMD */
#define _UART_CMD_MASK                       0x00000FFFUL                        /**< Mask for UART_CMD */
#define UART_CMD_RXEN                        (0x1UL << 0)                        /**< Receiver Enable */
#define _UART_CMD_RXEN_SHIFT                 0                                   /**< Shift value for USART_RXEN */
#define _UART_CMD_RXEN_MASK                  0x1UL                               /**< Bit mask for USART_RXEN */
#define _UART_CMD_RXEN_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_RXEN_DEFAULT                (_UART_CMD_RXEN_DEFAULT << 0)       /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_RXDIS                       (0x1UL << 1)                        /**< Receiver Disable */
#define _UART_CMD_RXDIS_SHIFT                1                                   /**< Shift value for USART_RXDIS */
#define _UART_CMD_RXDIS_MASK                 0x2UL                               /**< Bit mask for USART_RXDIS */
#define _UART_CMD_RXDIS_DEFAULT              0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_RXDIS_DEFAULT               (_UART_CMD_RXDIS_DEFAULT << 1)      /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_TXEN                        (0x1UL << 2)                        /**< Transmitter Enable */
#define _UART_CMD_TXEN_SHIFT                 2                                   /**< Shift value for USART_TXEN */
#define _UART_CMD_TXEN_MASK                  0x4UL                               /**< Bit mask for USART_TXEN */
#define _UART_CMD_TXEN_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_TXEN_DEFAULT                (_UART_CMD_TXEN_DEFAULT << 2)       /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_TXDIS                       (0x1UL << 3)                        /**< Transmitter Disable */
#define _UART_CMD_TXDIS_SHIFT                3                                   /**< Shift value for USART_TXDIS */
#define _UART_CMD_TXDIS_MASK                 0x8UL                               /**< Bit mask for USART_TXDIS */
#define _UART_CMD_TXDIS_DEFAULT              0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_TXDIS_DEFAULT               (_UART_CMD_TXDIS_DEFAULT << 3)      /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_MASTEREN                    (0x1UL << 4)                        /**< Master Enable */
#define _UART_CMD_MASTEREN_SHIFT             4                                   /**< Shift value for USART_MASTEREN */
#define _UART_CMD_MASTEREN_MASK              0x10UL                              /**< Bit mask for USART_MASTEREN */
#define _UART_CMD_MASTEREN_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_MASTEREN_DEFAULT            (_UART_CMD_MASTEREN_DEFAULT << 4)   /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_MASTERDIS                   (0x1UL << 5)                        /**< Master Disable */
#define _UART_CMD_MASTERDIS_SHIFT            5                                   /**< Shift value for USART_MASTERDIS */
#define _UART_CMD_MASTERDIS_MASK             0x20UL                              /**< Bit mask for USART_MASTERDIS */
#define _UART_CMD_MASTERDIS_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_MASTERDIS_DEFAULT           (_UART_CMD_MASTERDIS_DEFAULT << 5)  /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_RXBLOCKEN                   (0x1UL << 6)                        /**< Receiver Block Enable */
#define _UART_CMD_RXBLOCKEN_SHIFT            6                                   /**< Shift value for USART_RXBLOCKEN */
#define _UART_CMD_RXBLOCKEN_MASK             0x40UL                              /**< Bit mask for USART_RXBLOCKEN */
#define _UART_CMD_RXBLOCKEN_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_RXBLOCKEN_DEFAULT           (_UART_CMD_RXBLOCKEN_DEFAULT << 6)  /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_RXBLOCKDIS                  (0x1UL << 7)                        /**< Receiver Block Disable */
#define _UART_CMD_RXBLOCKDIS_SHIFT           7                                   /**< Shift value for USART_RXBLOCKDIS */
#define _UART_CMD_RXBLOCKDIS_MASK            0x80UL                              /**< Bit mask for USART_RXBLOCKDIS */
#define _UART_CMD_RXBLOCKDIS_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_RXBLOCKDIS_DEFAULT          (_UART_CMD_RXBLOCKDIS_DEFAULT << 7) /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_TXTRIEN                     (0x1UL << 8)                        /**< Transmitter Tristate Enable */
#define _UART_CMD_TXTRIEN_SHIFT              8                                   /**< Shift value for USART_TXTRIEN */
#define _UART_CMD_TXTRIEN_MASK               0x100UL                             /**< Bit mask for USART_TXTRIEN */
#define _UART_CMD_TXTRIEN_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_TXTRIEN_DEFAULT             (_UART_CMD_TXTRIEN_DEFAULT << 8)    /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_TXTRIDIS                    (0x1UL << 9)                        /**< Transmitter Tristate Disable */
#define _UART_CMD_TXTRIDIS_SHIFT             9                                   /**< Shift value for USART_TXTRIDIS */
#define _UART_CMD_TXTRIDIS_MASK              0x200UL                             /**< Bit mask for USART_TXTRIDIS */
#define _UART_CMD_TXTRIDIS_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_TXTRIDIS_DEFAULT            (_UART_CMD_TXTRIDIS_DEFAULT << 9)   /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_CLEARTX                     (0x1UL << 10)                       /**< Clear TX */
#define _UART_CMD_CLEARTX_SHIFT              10                                  /**< Shift value for USART_CLEARTX */
#define _UART_CMD_CLEARTX_MASK               0x400UL                             /**< Bit mask for USART_CLEARTX */
#define _UART_CMD_CLEARTX_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_CLEARTX_DEFAULT             (_UART_CMD_CLEARTX_DEFAULT << 10)   /**< Shifted mode DEFAULT for UART_CMD */
#define UART_CMD_CLEARRX                     (0x1UL << 11)                       /**< Clear RX */
#define _UART_CMD_CLEARRX_SHIFT              11                                  /**< Shift value for USART_CLEARRX */
#define _UART_CMD_CLEARRX_MASK               0x800UL                             /**< Bit mask for USART_CLEARRX */
#define _UART_CMD_CLEARRX_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for UART_CMD */
#define UART_CMD_CLEARRX_DEFAULT             (_UART_CMD_CLEARRX_DEFAULT << 11)   /**< Shifted mode DEFAULT for UART_CMD */

/* Bit fields for UART STATUS */
#define _UART_STATUS_RESETVALUE              0x00000040UL                              /**< Default value for UART_STATUS */
#define _UART_STATUS_MASK                    0x00001FFFUL                              /**< Mask for UART_STATUS */
#define UART_STATUS_RXENS                    (0x1UL << 0)                              /**< Receiver Enable Status */
#define _UART_STATUS_RXENS_SHIFT             0                                         /**< Shift value for USART_RXENS */
#define _UART_STATUS_RXENS_MASK              0x1UL                                     /**< Bit mask for USART_RXENS */
#define _UART_STATUS_RXENS_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXENS_DEFAULT            (_UART_STATUS_RXENS_DEFAULT << 0)         /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXENS                    (0x1UL << 1)                              /**< Transmitter Enable Status */
#define _UART_STATUS_TXENS_SHIFT             1                                         /**< Shift value for USART_TXENS */
#define _UART_STATUS_TXENS_MASK              0x2UL                                     /**< Bit mask for USART_TXENS */
#define _UART_STATUS_TXENS_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXENS_DEFAULT            (_UART_STATUS_TXENS_DEFAULT << 1)         /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_MASTER                   (0x1UL << 2)                              /**< SPI Master Mode */
#define _UART_STATUS_MASTER_SHIFT            2                                         /**< Shift value for USART_MASTER */
#define _UART_STATUS_MASTER_MASK             0x4UL                                     /**< Bit mask for USART_MASTER */
#define _UART_STATUS_MASTER_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_MASTER_DEFAULT           (_UART_STATUS_MASTER_DEFAULT << 2)        /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXBLOCK                  (0x1UL << 3)                              /**< Block Incoming Data */
#define _UART_STATUS_RXBLOCK_SHIFT           3                                         /**< Shift value for USART_RXBLOCK */
#define _UART_STATUS_RXBLOCK_MASK            0x8UL                                     /**< Bit mask for USART_RXBLOCK */
#define _UART_STATUS_RXBLOCK_DEFAULT         0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXBLOCK_DEFAULT          (_UART_STATUS_RXBLOCK_DEFAULT << 3)       /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXTRI                    (0x1UL << 4)                              /**< Transmitter Tristated */
#define _UART_STATUS_TXTRI_SHIFT             4                                         /**< Shift value for USART_TXTRI */
#define _UART_STATUS_TXTRI_MASK              0x10UL                                    /**< Bit mask for USART_TXTRI */
#define _UART_STATUS_TXTRI_DEFAULT           0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXTRI_DEFAULT            (_UART_STATUS_TXTRI_DEFAULT << 4)         /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXC                      (0x1UL << 5)                              /**< TX Complete */
#define _UART_STATUS_TXC_SHIFT               5                                         /**< Shift value for USART_TXC */
#define _UART_STATUS_TXC_MASK                0x20UL                                    /**< Bit mask for USART_TXC */
#define _UART_STATUS_TXC_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXC_DEFAULT              (_UART_STATUS_TXC_DEFAULT << 5)           /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXBL                     (0x1UL << 6)                              /**< TX Buffer Level */
#define _UART_STATUS_TXBL_SHIFT              6                                         /**< Shift value for USART_TXBL */
#define _UART_STATUS_TXBL_MASK               0x40UL                                    /**< Bit mask for USART_TXBL */
#define _UART_STATUS_TXBL_DEFAULT            0x00000001UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXBL_DEFAULT             (_UART_STATUS_TXBL_DEFAULT << 6)          /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXDATAV                  (0x1UL << 7)                              /**< RX Data Valid */
#define _UART_STATUS_RXDATAV_SHIFT           7                                         /**< Shift value for USART_RXDATAV */
#define _UART_STATUS_RXDATAV_MASK            0x80UL                                    /**< Bit mask for USART_RXDATAV */
#define _UART_STATUS_RXDATAV_DEFAULT         0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXDATAV_DEFAULT          (_UART_STATUS_RXDATAV_DEFAULT << 7)       /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXFULL                   (0x1UL << 8)                              /**< RX FIFO Full */
#define _UART_STATUS_RXFULL_SHIFT            8                                         /**< Shift value for USART_RXFULL */
#define _UART_STATUS_RXFULL_MASK             0x100UL                                   /**< Bit mask for USART_RXFULL */
#define _UART_STATUS_RXFULL_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXFULL_DEFAULT           (_UART_STATUS_RXFULL_DEFAULT << 8)        /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXBDRIGHT                (0x1UL << 9)                              /**< TX Buffer Expects Double Right Data */
#define _UART_STATUS_TXBDRIGHT_SHIFT         9                                         /**< Shift value for USART_TXBDRIGHT */
#define _UART_STATUS_TXBDRIGHT_MASK          0x200UL                                   /**< Bit mask for USART_TXBDRIGHT */
#define _UART_STATUS_TXBDRIGHT_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXBDRIGHT_DEFAULT        (_UART_STATUS_TXBDRIGHT_DEFAULT << 9)     /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXBSRIGHT                (0x1UL << 10)                             /**< TX Buffer Expects Single Right Data */
#define _UART_STATUS_TXBSRIGHT_SHIFT         10                                        /**< Shift value for USART_TXBSRIGHT */
#define _UART_STATUS_TXBSRIGHT_MASK          0x400UL                                   /**< Bit mask for USART_TXBSRIGHT */
#define _UART_STATUS_TXBSRIGHT_DEFAULT       0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_TXBSRIGHT_DEFAULT        (_UART_STATUS_TXBSRIGHT_DEFAULT << 10)    /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXDATAVRIGHT             (0x1UL << 11)                             /**< RX Data Right */
#define _UART_STATUS_RXDATAVRIGHT_SHIFT      11                                        /**< Shift value for USART_RXDATAVRIGHT */
#define _UART_STATUS_RXDATAVRIGHT_MASK       0x800UL                                   /**< Bit mask for USART_RXDATAVRIGHT */
#define _UART_STATUS_RXDATAVRIGHT_DEFAULT    0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXDATAVRIGHT_DEFAULT     (_UART_STATUS_RXDATAVRIGHT_DEFAULT << 11) /**< Shifted mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXFULLRIGHT              (0x1UL << 12)                             /**< RX Full of Right Data */
#define _UART_STATUS_RXFULLRIGHT_SHIFT       12                                        /**< Shift value for USART_RXFULLRIGHT */
#define _UART_STATUS_RXFULLRIGHT_MASK        0x1000UL                                  /**< Bit mask for USART_RXFULLRIGHT */
#define _UART_STATUS_RXFULLRIGHT_DEFAULT     0x00000000UL                              /**< Mode DEFAULT for UART_STATUS */
#define UART_STATUS_RXFULLRIGHT_DEFAULT      (_UART_STATUS_RXFULLRIGHT_DEFAULT << 12)  /**< Shifted mode DEFAULT for UART_STATUS */

/* Bit fields for UART CLKDIV */
#define _UART_CLKDIV_RESETVALUE              0x00000000UL                    /**< Default value for UART_CLKDIV */
#define _UART_CLKDIV_MASK                    0x001FFFC0UL                    /**< Mask for UART_CLKDIV */
#define _UART_CLKDIV_DIV_SHIFT               6                               /**< Shift value for USART_DIV */
#define _UART_CLKDIV_DIV_MASK                0x1FFFC0UL                      /**< Bit mask for USART_DIV */
#define _UART_CLKDIV_DIV_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for UART_CLKDIV */
#define UART_CLKDIV_DIV_DEFAULT              (_UART_CLKDIV_DIV_DEFAULT << 6) /**< Shifted mode DEFAULT for UART_CLKDIV */

/* Bit fields for UART RXDATAX */
#define _UART_RXDATAX_RESETVALUE             0x00000000UL                        /**< Default value for UART_RXDATAX */
#define _UART_RXDATAX_MASK                   0x0000C1FFUL                        /**< Mask for UART_RXDATAX */
#define _UART_RXDATAX_RXDATA_SHIFT           0                                   /**< Shift value for USART_RXDATA */
#define _UART_RXDATAX_RXDATA_MASK            0x1FFUL                             /**< Bit mask for USART_RXDATA */
#define _UART_RXDATAX_RXDATA_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for UART_RXDATAX */
#define UART_RXDATAX_RXDATA_DEFAULT          (_UART_RXDATAX_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for UART_RXDATAX */
#define UART_RXDATAX_PERR                    (0x1UL << 14)                       /**< Data Parity Error */
#define _UART_RXDATAX_PERR_SHIFT             14                                  /**< Shift value for USART_PERR */
#define _UART_RXDATAX_PERR_MASK              0x4000UL                            /**< Bit mask for USART_PERR */
#define _UART_RXDATAX_PERR_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for UART_RXDATAX */
#define UART_RXDATAX_PERR_DEFAULT            (_UART_RXDATAX_PERR_DEFAULT << 14)  /**< Shifted mode DEFAULT for UART_RXDATAX */
#define UART_RXDATAX_FERR                    (0x1UL << 15)                       /**< Data Framing Error */
#define _UART_RXDATAX_FERR_SHIFT             15                                  /**< Shift value for USART_FERR */
#define _UART_RXDATAX_FERR_MASK              0x8000UL                            /**< Bit mask for USART_FERR */
#define _UART_RXDATAX_FERR_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for UART_RXDATAX */
#define UART_RXDATAX_FERR_DEFAULT            (_UART_RXDATAX_FERR_DEFAULT << 15)  /**< Shifted mode DEFAULT for UART_RXDATAX */

/* Bit fields for UART RXDATA */
#define _UART_RXDATA_RESETVALUE              0x00000000UL                       /**< Default value for UART_RXDATA */
#define _UART_RXDATA_MASK                    0x000000FFUL                       /**< Mask for UART_RXDATA */
#define _UART_RXDATA_RXDATA_SHIFT            0                                  /**< Shift value for USART_RXDATA */
#define _UART_RXDATA_RXDATA_MASK             0xFFUL                             /**< Bit mask for USART_RXDATA */
#define _UART_RXDATA_RXDATA_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for UART_RXDATA */
#define UART_RXDATA_RXDATA_DEFAULT           (_UART_RXDATA_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for UART_RXDATA */

/* Bit fields for UART RXDOUBLEX */
#define _UART_RXDOUBLEX_RESETVALUE           0x00000000UL                            /**< Default value for UART_RXDOUBLEX */
#define _UART_RXDOUBLEX_MASK                 0xC1FFC1FFUL                            /**< Mask for UART_RXDOUBLEX */
#define _UART_RXDOUBLEX_RXDATA0_SHIFT        0                                       /**< Shift value for USART_RXDATA0 */
#define _UART_RXDOUBLEX_RXDATA0_MASK         0x1FFUL                                 /**< Bit mask for USART_RXDATA0 */
#define _UART_RXDOUBLEX_RXDATA0_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_RXDATA0_DEFAULT       (_UART_RXDOUBLEX_RXDATA0_DEFAULT << 0)  /**< Shifted mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_PERR0                 (0x1UL << 14)                           /**< Data Parity Error 0 */
#define _UART_RXDOUBLEX_PERR0_SHIFT          14                                      /**< Shift value for USART_PERR0 */
#define _UART_RXDOUBLEX_PERR0_MASK           0x4000UL                                /**< Bit mask for USART_PERR0 */
#define _UART_RXDOUBLEX_PERR0_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_PERR0_DEFAULT         (_UART_RXDOUBLEX_PERR0_DEFAULT << 14)   /**< Shifted mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_FERR0                 (0x1UL << 15)                           /**< Data Framing Error 0 */
#define _UART_RXDOUBLEX_FERR0_SHIFT          15                                      /**< Shift value for USART_FERR0 */
#define _UART_RXDOUBLEX_FERR0_MASK           0x8000UL                                /**< Bit mask for USART_FERR0 */
#define _UART_RXDOUBLEX_FERR0_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_FERR0_DEFAULT         (_UART_RXDOUBLEX_FERR0_DEFAULT << 15)   /**< Shifted mode DEFAULT for UART_RXDOUBLEX */
#define _UART_RXDOUBLEX_RXDATA1_SHIFT        16                                      /**< Shift value for USART_RXDATA1 */
#define _UART_RXDOUBLEX_RXDATA1_MASK         0x1FF0000UL                             /**< Bit mask for USART_RXDATA1 */
#define _UART_RXDOUBLEX_RXDATA1_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_RXDATA1_DEFAULT       (_UART_RXDOUBLEX_RXDATA1_DEFAULT << 16) /**< Shifted mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_PERR1                 (0x1UL << 30)                           /**< Data Parity Error 1 */
#define _UART_RXDOUBLEX_PERR1_SHIFT          30                                      /**< Shift value for USART_PERR1 */
#define _UART_RXDOUBLEX_PERR1_MASK           0x40000000UL                            /**< Bit mask for USART_PERR1 */
#define _UART_RXDOUBLEX_PERR1_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_PERR1_DEFAULT         (_UART_RXDOUBLEX_PERR1_DEFAULT << 30)   /**< Shifted mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_FERR1                 (0x1UL << 31)                           /**< Data Framing Error 1 */
#define _UART_RXDOUBLEX_FERR1_SHIFT          31                                      /**< Shift value for USART_FERR1 */
#define _UART_RXDOUBLEX_FERR1_MASK           0x80000000UL                            /**< Bit mask for USART_FERR1 */
#define _UART_RXDOUBLEX_FERR1_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for UART_RXDOUBLEX */
#define UART_RXDOUBLEX_FERR1_DEFAULT         (_UART_RXDOUBLEX_FERR1_DEFAULT << 31)   /**< Shifted mode DEFAULT for UART_RXDOUBLEX */

/* Bit fields for UART RXDOUBLE */
#define _UART_RXDOUBLE_RESETVALUE            0x00000000UL                          /**< Default value for UART_RXDOUBLE */
#define _UART_RXDOUBLE_MASK                  0x0000FFFFUL                          /**< Mask for UART_RXDOUBLE */
#define _UART_RXDOUBLE_RXDATA0_SHIFT         0                                     /**< Shift value for USART_RXDATA0 */
#define _UART_RXDOUBLE_RXDATA0_MASK          0xFFUL                                /**< Bit mask for USART_RXDATA0 */
#define _UART_RXDOUBLE_RXDATA0_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for UART_RXDOUBLE */
#define UART_RXDOUBLE_RXDATA0_DEFAULT        (_UART_RXDOUBLE_RXDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for UART_RXDOUBLE */
#define _UART_RXDOUBLE_RXDATA1_SHIFT         8                                     /**< Shift value for USART_RXDATA1 */
#define _UART_RXDOUBLE_RXDATA1_MASK          0xFF00UL                              /**< Bit mask for USART_RXDATA1 */
#define _UART_RXDOUBLE_RXDATA1_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for UART_RXDOUBLE */
#define UART_RXDOUBLE_RXDATA1_DEFAULT        (_UART_RXDOUBLE_RXDATA1_DEFAULT << 8) /**< Shifted mode DEFAULT for UART_RXDOUBLE */

/* Bit fields for UART RXDATAXP */
#define _UART_RXDATAXP_RESETVALUE            0x00000000UL                          /**< Default value for UART_RXDATAXP */
#define _UART_RXDATAXP_MASK                  0x0000C1FFUL                          /**< Mask for UART_RXDATAXP */
#define _UART_RXDATAXP_RXDATAP_SHIFT         0                                     /**< Shift value for USART_RXDATAP */
#define _UART_RXDATAXP_RXDATAP_MASK          0x1FFUL                               /**< Bit mask for USART_RXDATAP */
#define _UART_RXDATAXP_RXDATAP_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for UART_RXDATAXP */
#define UART_RXDATAXP_RXDATAP_DEFAULT        (_UART_RXDATAXP_RXDATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for UART_RXDATAXP */
#define UART_RXDATAXP_PERRP                  (0x1UL << 14)                         /**< Data Parity Error Peek */
#define _UART_RXDATAXP_PERRP_SHIFT           14                                    /**< Shift value for USART_PERRP */
#define _UART_RXDATAXP_PERRP_MASK            0x4000UL                              /**< Bit mask for USART_PERRP */
#define _UART_RXDATAXP_PERRP_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for UART_RXDATAXP */
#define UART_RXDATAXP_PERRP_DEFAULT          (_UART_RXDATAXP_PERRP_DEFAULT << 14)  /**< Shifted mode DEFAULT for UART_RXDATAXP */
#define UART_RXDATAXP_FERRP                  (0x1UL << 15)                         /**< Data Framing Error Peek */
#define _UART_RXDATAXP_FERRP_SHIFT           15                                    /**< Shift value for USART_FERRP */
#define _UART_RXDATAXP_FERRP_MASK            0x8000UL                              /**< Bit mask for USART_FERRP */
#define _UART_RXDATAXP_FERRP_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for UART_RXDATAXP */
#define UART_RXDATAXP_FERRP_DEFAULT          (_UART_RXDATAXP_FERRP_DEFAULT << 15)  /**< Shifted mode DEFAULT for UART_RXDATAXP */

/* Bit fields for UART RXDOUBLEXP */
#define _UART_RXDOUBLEXP_RESETVALUE          0x00000000UL                              /**< Default value for UART_RXDOUBLEXP */
#define _UART_RXDOUBLEXP_MASK                0xC1FFC1FFUL                              /**< Mask for UART_RXDOUBLEXP */
#define _UART_RXDOUBLEXP_RXDATAP0_SHIFT      0                                         /**< Shift value for USART_RXDATAP0 */
#define _UART_RXDOUBLEXP_RXDATAP0_MASK       0x1FFUL                                   /**< Bit mask for USART_RXDATAP0 */
#define _UART_RXDOUBLEXP_RXDATAP0_DEFAULT    0x00000000UL                              /**< Mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_RXDATAP0_DEFAULT     (_UART_RXDOUBLEXP_RXDATAP0_DEFAULT << 0)  /**< Shifted mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_PERRP0               (0x1UL << 14)                             /**< Data Parity Error 0 Peek */
#define _UART_RXDOUBLEXP_PERRP0_SHIFT        14                                        /**< Shift value for USART_PERRP0 */
#define _UART_RXDOUBLEXP_PERRP0_MASK         0x4000UL                                  /**< Bit mask for USART_PERRP0 */
#define _UART_RXDOUBLEXP_PERRP0_DEFAULT      0x00000000UL                              /**< Mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_PERRP0_DEFAULT       (_UART_RXDOUBLEXP_PERRP0_DEFAULT << 14)   /**< Shifted mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_FERRP0               (0x1UL << 15)                             /**< Data Framing Error 0 Peek */
#define _UART_RXDOUBLEXP_FERRP0_SHIFT        15                                        /**< Shift value for USART_FERRP0 */
#define _UART_RXDOUBLEXP_FERRP0_MASK         0x8000UL                                  /**< Bit mask for USART_FERRP0 */
#define _UART_RXDOUBLEXP_FERRP0_DEFAULT      0x00000000UL                              /**< Mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_FERRP0_DEFAULT       (_UART_RXDOUBLEXP_FERRP0_DEFAULT << 15)   /**< Shifted mode DEFAULT for UART_RXDOUBLEXP */
#define _UART_RXDOUBLEXP_RXDATAP1_SHIFT      16                                        /**< Shift value for USART_RXDATAP1 */
#define _UART_RXDOUBLEXP_RXDATAP1_MASK       0x1FF0000UL                               /**< Bit mask for USART_RXDATAP1 */
#define _UART_RXDOUBLEXP_RXDATAP1_DEFAULT    0x00000000UL                              /**< Mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_RXDATAP1_DEFAULT     (_UART_RXDOUBLEXP_RXDATAP1_DEFAULT << 16) /**< Shifted mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_PERRP1               (0x1UL << 30)                             /**< Data Parity Error 1 Peek */
#define _UART_RXDOUBLEXP_PERRP1_SHIFT        30                                        /**< Shift value for USART_PERRP1 */
#define _UART_RXDOUBLEXP_PERRP1_MASK         0x40000000UL                              /**< Bit mask for USART_PERRP1 */
#define _UART_RXDOUBLEXP_PERRP1_DEFAULT      0x00000000UL                              /**< Mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_PERRP1_DEFAULT       (_UART_RXDOUBLEXP_PERRP1_DEFAULT << 30)   /**< Shifted mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_FERRP1               (0x1UL << 31)                             /**< Data Framing Error 1 Peek */
#define _UART_RXDOUBLEXP_FERRP1_SHIFT        31                                        /**< Shift value for USART_FERRP1 */
#define _UART_RXDOUBLEXP_FERRP1_MASK         0x80000000UL                              /**< Bit mask for USART_FERRP1 */
#define _UART_RXDOUBLEXP_FERRP1_DEFAULT      0x00000000UL                              /**< Mode DEFAULT for UART_RXDOUBLEXP */
#define UART_RXDOUBLEXP_FERRP1_DEFAULT       (_UART_RXDOUBLEXP_FERRP1_DEFAULT << 31)   /**< Shifted mode DEFAULT for UART_RXDOUBLEXP */

/* Bit fields for UART TXDATAX */
#define _UART_TXDATAX_RESETVALUE             0x00000000UL                          /**< Default value for UART_TXDATAX */
#define _UART_TXDATAX_MASK                   0x0000F9FFUL                          /**< Mask for UART_TXDATAX */
#define _UART_TXDATAX_TXDATAX_SHIFT          0                                     /**< Shift value for USART_TXDATAX */
#define _UART_TXDATAX_TXDATAX_MASK           0x1FFUL                               /**< Bit mask for USART_TXDATAX */
#define _UART_TXDATAX_TXDATAX_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_TXDATAX_DEFAULT         (_UART_TXDATAX_TXDATAX_DEFAULT << 0)  /**< Shifted mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_UBRXAT                  (0x1UL << 11)                         /**< Unblock RX After Transmission */
#define _UART_TXDATAX_UBRXAT_SHIFT           11                                    /**< Shift value for USART_UBRXAT */
#define _UART_TXDATAX_UBRXAT_MASK            0x800UL                               /**< Bit mask for USART_UBRXAT */
#define _UART_TXDATAX_UBRXAT_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_UBRXAT_DEFAULT          (_UART_TXDATAX_UBRXAT_DEFAULT << 11)  /**< Shifted mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_TXTRIAT                 (0x1UL << 12)                         /**< Set TXTRI After Transmission */
#define _UART_TXDATAX_TXTRIAT_SHIFT          12                                    /**< Shift value for USART_TXTRIAT */
#define _UART_TXDATAX_TXTRIAT_MASK           0x1000UL                              /**< Bit mask for USART_TXTRIAT */
#define _UART_TXDATAX_TXTRIAT_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_TXTRIAT_DEFAULT         (_UART_TXDATAX_TXTRIAT_DEFAULT << 12) /**< Shifted mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_TXBREAK                 (0x1UL << 13)                         /**< Transmit Data As Break */
#define _UART_TXDATAX_TXBREAK_SHIFT          13                                    /**< Shift value for USART_TXBREAK */
#define _UART_TXDATAX_TXBREAK_MASK           0x2000UL                              /**< Bit mask for USART_TXBREAK */
#define _UART_TXDATAX_TXBREAK_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_TXBREAK_DEFAULT         (_UART_TXDATAX_TXBREAK_DEFAULT << 13) /**< Shifted mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_TXDISAT                 (0x1UL << 14)                         /**< Clear TXEN After Transmission */
#define _UART_TXDATAX_TXDISAT_SHIFT          14                                    /**< Shift value for USART_TXDISAT */
#define _UART_TXDATAX_TXDISAT_MASK           0x4000UL                              /**< Bit mask for USART_TXDISAT */
#define _UART_TXDATAX_TXDISAT_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_TXDISAT_DEFAULT         (_UART_TXDATAX_TXDISAT_DEFAULT << 14) /**< Shifted mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_RXENAT                  (0x1UL << 15)                         /**< Enable RX After Transmission */
#define _UART_TXDATAX_RXENAT_SHIFT           15                                    /**< Shift value for USART_RXENAT */
#define _UART_TXDATAX_RXENAT_MASK            0x8000UL                              /**< Bit mask for USART_RXENAT */
#define _UART_TXDATAX_RXENAT_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for UART_TXDATAX */
#define UART_TXDATAX_RXENAT_DEFAULT          (_UART_TXDATAX_RXENAT_DEFAULT << 15)  /**< Shifted mode DEFAULT for UART_TXDATAX */

/* Bit fields for UART TXDATA */
#define _UART_TXDATA_RESETVALUE              0x00000000UL                       /**< Default value for UART_TXDATA */
#define _UART_TXDATA_MASK                    0x000000FFUL                       /**< Mask for UART_TXDATA */
#define _UART_TXDATA_TXDATA_SHIFT            0                                  /**< Shift value for USART_TXDATA */
#define _UART_TXDATA_TXDATA_MASK             0xFFUL                             /**< Bit mask for USART_TXDATA */
#define _UART_TXDATA_TXDATA_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for UART_TXDATA */
#define UART_TXDATA_TXDATA_DEFAULT           (_UART_TXDATA_TXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for UART_TXDATA */

/* Bit fields for UART TXDOUBLEX */
#define _UART_TXDOUBLEX_RESETVALUE           0x00000000UL                             /**< Default value for UART_TXDOUBLEX */
#define _UART_TXDOUBLEX_MASK                 0xF9FFF9FFUL                             /**< Mask for UART_TXDOUBLEX */
#define _UART_TXDOUBLEX_TXDATA0_SHIFT        0                                        /**< Shift value for USART_TXDATA0 */
#define _UART_TXDOUBLEX_TXDATA0_MASK         0x1FFUL                                  /**< Bit mask for USART_TXDATA0 */
#define _UART_TXDOUBLEX_TXDATA0_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXDATA0_DEFAULT       (_UART_TXDOUBLEX_TXDATA0_DEFAULT << 0)   /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_UBRXAT0               (0x1UL << 11)                            /**< Unblock RX After Transmission */
#define _UART_TXDOUBLEX_UBRXAT0_SHIFT        11                                       /**< Shift value for USART_UBRXAT0 */
#define _UART_TXDOUBLEX_UBRXAT0_MASK         0x800UL                                  /**< Bit mask for USART_UBRXAT0 */
#define _UART_TXDOUBLEX_UBRXAT0_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_UBRXAT0_DEFAULT       (_UART_TXDOUBLEX_UBRXAT0_DEFAULT << 11)  /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXTRIAT0              (0x1UL << 12)                            /**< Set TXTRI After Transmission */
#define _UART_TXDOUBLEX_TXTRIAT0_SHIFT       12                                       /**< Shift value for USART_TXTRIAT0 */
#define _UART_TXDOUBLEX_TXTRIAT0_MASK        0x1000UL                                 /**< Bit mask for USART_TXTRIAT0 */
#define _UART_TXDOUBLEX_TXTRIAT0_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXTRIAT0_DEFAULT      (_UART_TXDOUBLEX_TXTRIAT0_DEFAULT << 12) /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXBREAK0              (0x1UL << 13)                            /**< Transmit Data As Break */
#define _UART_TXDOUBLEX_TXBREAK0_SHIFT       13                                       /**< Shift value for USART_TXBREAK0 */
#define _UART_TXDOUBLEX_TXBREAK0_MASK        0x2000UL                                 /**< Bit mask for USART_TXBREAK0 */
#define _UART_TXDOUBLEX_TXBREAK0_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXBREAK0_DEFAULT      (_UART_TXDOUBLEX_TXBREAK0_DEFAULT << 13) /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXDISAT0              (0x1UL << 14)                            /**< Clear TXEN After Transmission */
#define _UART_TXDOUBLEX_TXDISAT0_SHIFT       14                                       /**< Shift value for USART_TXDISAT0 */
#define _UART_TXDOUBLEX_TXDISAT0_MASK        0x4000UL                                 /**< Bit mask for USART_TXDISAT0 */
#define _UART_TXDOUBLEX_TXDISAT0_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXDISAT0_DEFAULT      (_UART_TXDOUBLEX_TXDISAT0_DEFAULT << 14) /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_RXENAT0               (0x1UL << 15)                            /**< Enable RX After Transmission */
#define _UART_TXDOUBLEX_RXENAT0_SHIFT        15                                       /**< Shift value for USART_RXENAT0 */
#define _UART_TXDOUBLEX_RXENAT0_MASK         0x8000UL                                 /**< Bit mask for USART_RXENAT0 */
#define _UART_TXDOUBLEX_RXENAT0_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_RXENAT0_DEFAULT       (_UART_TXDOUBLEX_RXENAT0_DEFAULT << 15)  /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define _UART_TXDOUBLEX_TXDATA1_SHIFT        16                                       /**< Shift value for USART_TXDATA1 */
#define _UART_TXDOUBLEX_TXDATA1_MASK         0x1FF0000UL                              /**< Bit mask for USART_TXDATA1 */
#define _UART_TXDOUBLEX_TXDATA1_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXDATA1_DEFAULT       (_UART_TXDOUBLEX_TXDATA1_DEFAULT << 16)  /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_UBRXAT1               (0x1UL << 27)                            /**< Unblock RX After Transmission */
#define _UART_TXDOUBLEX_UBRXAT1_SHIFT        27                                       /**< Shift value for USART_UBRXAT1 */
#define _UART_TXDOUBLEX_UBRXAT1_MASK         0x8000000UL                              /**< Bit mask for USART_UBRXAT1 */
#define _UART_TXDOUBLEX_UBRXAT1_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_UBRXAT1_DEFAULT       (_UART_TXDOUBLEX_UBRXAT1_DEFAULT << 27)  /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXTRIAT1              (0x1UL << 28)                            /**< Set TXTRI After Transmission */
#define _UART_TXDOUBLEX_TXTRIAT1_SHIFT       28                                       /**< Shift value for USART_TXTRIAT1 */
#define _UART_TXDOUBLEX_TXTRIAT1_MASK        0x10000000UL                             /**< Bit mask for USART_TXTRIAT1 */
#define _UART_TXDOUBLEX_TXTRIAT1_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXTRIAT1_DEFAULT      (_UART_TXDOUBLEX_TXTRIAT1_DEFAULT << 28) /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXBREAK1              (0x1UL << 29)                            /**< Transmit Data As Break */
#define _UART_TXDOUBLEX_TXBREAK1_SHIFT       29                                       /**< Shift value for USART_TXBREAK1 */
#define _UART_TXDOUBLEX_TXBREAK1_MASK        0x20000000UL                             /**< Bit mask for USART_TXBREAK1 */
#define _UART_TXDOUBLEX_TXBREAK1_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXBREAK1_DEFAULT      (_UART_TXDOUBLEX_TXBREAK1_DEFAULT << 29) /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXDISAT1              (0x1UL << 30)                            /**< Clear TXEN After Transmission */
#define _UART_TXDOUBLEX_TXDISAT1_SHIFT       30                                       /**< Shift value for USART_TXDISAT1 */
#define _UART_TXDOUBLEX_TXDISAT1_MASK        0x40000000UL                             /**< Bit mask for USART_TXDISAT1 */
#define _UART_TXDOUBLEX_TXDISAT1_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_TXDISAT1_DEFAULT      (_UART_TXDOUBLEX_TXDISAT1_DEFAULT << 30) /**< Shifted mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_RXENAT1               (0x1UL << 31)                            /**< Enable RX After Transmission */
#define _UART_TXDOUBLEX_RXENAT1_SHIFT        31                                       /**< Shift value for USART_RXENAT1 */
#define _UART_TXDOUBLEX_RXENAT1_MASK         0x80000000UL                             /**< Bit mask for USART_RXENAT1 */
#define _UART_TXDOUBLEX_RXENAT1_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for UART_TXDOUBLEX */
#define UART_TXDOUBLEX_RXENAT1_DEFAULT       (_UART_TXDOUBLEX_RXENAT1_DEFAULT << 31)  /**< Shifted mode DEFAULT for UART_TXDOUBLEX */

/* Bit fields for UART TXDOUBLE */
#define _UART_TXDOUBLE_RESETVALUE            0x00000000UL                          /**< Default value for UART_TXDOUBLE */
#define _UART_TXDOUBLE_MASK                  0x0000FFFFUL                          /**< Mask for UART_TXDOUBLE */
#define _UART_TXDOUBLE_TXDATA0_SHIFT         0                                     /**< Shift value for USART_TXDATA0 */
#define _UART_TXDOUBLE_TXDATA0_MASK          0xFFUL                                /**< Bit mask for USART_TXDATA0 */
#define _UART_TXDOUBLE_TXDATA0_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for UART_TXDOUBLE */
#define UART_TXDOUBLE_TXDATA0_DEFAULT        (_UART_TXDOUBLE_TXDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for UART_TXDOUBLE */
#define _UART_TXDOUBLE_TXDATA1_SHIFT         8                                     /**< Shift value for USART_TXDATA1 */
#define _UART_TXDOUBLE_TXDATA1_MASK          0xFF00UL                              /**< Bit mask for USART_TXDATA1 */
#define _UART_TXDOUBLE_TXDATA1_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for UART_TXDOUBLE */
#define UART_TXDOUBLE_TXDATA1_DEFAULT        (_UART_TXDOUBLE_TXDATA1_DEFAULT << 8) /**< Shifted mode DEFAULT for UART_TXDOUBLE */

/* Bit fields for UART IF */
#define _UART_IF_RESETVALUE                  0x00000002UL                    /**< Default value for UART_IF */
#define _UART_IF_MASK                        0x00001FFFUL                    /**< Mask for UART_IF */
#define UART_IF_TXC                          (0x1UL << 0)                    /**< TX Complete Interrupt Flag */
#define _UART_IF_TXC_SHIFT                   0                               /**< Shift value for USART_TXC */
#define _UART_IF_TXC_MASK                    0x1UL                           /**< Bit mask for USART_TXC */
#define _UART_IF_TXC_DEFAULT                 0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_TXC_DEFAULT                  (_UART_IF_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_TXBL                         (0x1UL << 1)                    /**< TX Buffer Level Interrupt Flag */
#define _UART_IF_TXBL_SHIFT                  1                               /**< Shift value for USART_TXBL */
#define _UART_IF_TXBL_MASK                   0x2UL                           /**< Bit mask for USART_TXBL */
#define _UART_IF_TXBL_DEFAULT                0x00000001UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_TXBL_DEFAULT                 (_UART_IF_TXBL_DEFAULT << 1)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_RXDATAV                      (0x1UL << 2)                    /**< RX Data Valid Interrupt Flag */
#define _UART_IF_RXDATAV_SHIFT               2                               /**< Shift value for USART_RXDATAV */
#define _UART_IF_RXDATAV_MASK                0x4UL                           /**< Bit mask for USART_RXDATAV */
#define _UART_IF_RXDATAV_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_RXDATAV_DEFAULT              (_UART_IF_RXDATAV_DEFAULT << 2) /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_RXFULL                       (0x1UL << 3)                    /**< RX Buffer Full Interrupt Flag */
#define _UART_IF_RXFULL_SHIFT                3                               /**< Shift value for USART_RXFULL */
#define _UART_IF_RXFULL_MASK                 0x8UL                           /**< Bit mask for USART_RXFULL */
#define _UART_IF_RXFULL_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_RXFULL_DEFAULT               (_UART_IF_RXFULL_DEFAULT << 3)  /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_RXOF                         (0x1UL << 4)                    /**< RX Overflow Interrupt Flag */
#define _UART_IF_RXOF_SHIFT                  4                               /**< Shift value for USART_RXOF */
#define _UART_IF_RXOF_MASK                   0x10UL                          /**< Bit mask for USART_RXOF */
#define _UART_IF_RXOF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_RXOF_DEFAULT                 (_UART_IF_RXOF_DEFAULT << 4)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_RXUF                         (0x1UL << 5)                    /**< RX Underflow Interrupt Flag */
#define _UART_IF_RXUF_SHIFT                  5                               /**< Shift value for USART_RXUF */
#define _UART_IF_RXUF_MASK                   0x20UL                          /**< Bit mask for USART_RXUF */
#define _UART_IF_RXUF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_RXUF_DEFAULT                 (_UART_IF_RXUF_DEFAULT << 5)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_TXOF                         (0x1UL << 6)                    /**< TX Overflow Interrupt Flag */
#define _UART_IF_TXOF_SHIFT                  6                               /**< Shift value for USART_TXOF */
#define _UART_IF_TXOF_MASK                   0x40UL                          /**< Bit mask for USART_TXOF */
#define _UART_IF_TXOF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_TXOF_DEFAULT                 (_UART_IF_TXOF_DEFAULT << 6)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_TXUF                         (0x1UL << 7)                    /**< TX Underflow Interrupt Flag */
#define _UART_IF_TXUF_SHIFT                  7                               /**< Shift value for USART_TXUF */
#define _UART_IF_TXUF_MASK                   0x80UL                          /**< Bit mask for USART_TXUF */
#define _UART_IF_TXUF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_TXUF_DEFAULT                 (_UART_IF_TXUF_DEFAULT << 7)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_PERR                         (0x1UL << 8)                    /**< Parity Error Interrupt Flag */
#define _UART_IF_PERR_SHIFT                  8                               /**< Shift value for USART_PERR */
#define _UART_IF_PERR_MASK                   0x100UL                         /**< Bit mask for USART_PERR */
#define _UART_IF_PERR_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_PERR_DEFAULT                 (_UART_IF_PERR_DEFAULT << 8)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_FERR                         (0x1UL << 9)                    /**< Framing Error Interrupt Flag */
#define _UART_IF_FERR_SHIFT                  9                               /**< Shift value for USART_FERR */
#define _UART_IF_FERR_MASK                   0x200UL                         /**< Bit mask for USART_FERR */
#define _UART_IF_FERR_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_FERR_DEFAULT                 (_UART_IF_FERR_DEFAULT << 9)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_MPAF                         (0x1UL << 10)                   /**< Multi-Processor Address Frame Interrupt Flag */
#define _UART_IF_MPAF_SHIFT                  10                              /**< Shift value for USART_MPAF */
#define _UART_IF_MPAF_MASK                   0x400UL                         /**< Bit mask for USART_MPAF */
#define _UART_IF_MPAF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_MPAF_DEFAULT                 (_UART_IF_MPAF_DEFAULT << 10)   /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_SSM                          (0x1UL << 11)                   /**< Slave-Select In Master Mode Interrupt Flag */
#define _UART_IF_SSM_SHIFT                   11                              /**< Shift value for USART_SSM */
#define _UART_IF_SSM_MASK                    0x800UL                         /**< Bit mask for USART_SSM */
#define _UART_IF_SSM_DEFAULT                 0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_SSM_DEFAULT                  (_UART_IF_SSM_DEFAULT << 11)    /**< Shifted mode DEFAULT for UART_IF */
#define UART_IF_CCF                          (0x1UL << 12)                   /**< Collision Check Fail Interrupt Flag */
#define _UART_IF_CCF_SHIFT                   12                              /**< Shift value for USART_CCF */
#define _UART_IF_CCF_MASK                    0x1000UL                        /**< Bit mask for USART_CCF */
#define _UART_IF_CCF_DEFAULT                 0x00000000UL                    /**< Mode DEFAULT for UART_IF */
#define UART_IF_CCF_DEFAULT                  (_UART_IF_CCF_DEFAULT << 12)    /**< Shifted mode DEFAULT for UART_IF */

/* Bit fields for UART IFS */
#define _UART_IFS_RESETVALUE                 0x00000000UL                    /**< Default value for UART_IFS */
#define _UART_IFS_MASK                       0x00001FF9UL                    /**< Mask for UART_IFS */
#define UART_IFS_TXC                         (0x1UL << 0)                    /**< Set TX Complete Interrupt Flag */
#define _UART_IFS_TXC_SHIFT                  0                               /**< Shift value for USART_TXC */
#define _UART_IFS_TXC_MASK                   0x1UL                           /**< Bit mask for USART_TXC */
#define _UART_IFS_TXC_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_TXC_DEFAULT                 (_UART_IFS_TXC_DEFAULT << 0)    /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_RXFULL                      (0x1UL << 3)                    /**< Set RX Buffer Full Interrupt Flag */
#define _UART_IFS_RXFULL_SHIFT               3                               /**< Shift value for USART_RXFULL */
#define _UART_IFS_RXFULL_MASK                0x8UL                           /**< Bit mask for USART_RXFULL */
#define _UART_IFS_RXFULL_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_RXFULL_DEFAULT              (_UART_IFS_RXFULL_DEFAULT << 3) /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_RXOF                        (0x1UL << 4)                    /**< Set RX Overflow Interrupt Flag */
#define _UART_IFS_RXOF_SHIFT                 4                               /**< Shift value for USART_RXOF */
#define _UART_IFS_RXOF_MASK                  0x10UL                          /**< Bit mask for USART_RXOF */
#define _UART_IFS_RXOF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_RXOF_DEFAULT                (_UART_IFS_RXOF_DEFAULT << 4)   /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_RXUF                        (0x1UL << 5)                    /**< Set RX Underflow Interrupt Flag */
#define _UART_IFS_RXUF_SHIFT                 5                               /**< Shift value for USART_RXUF */
#define _UART_IFS_RXUF_MASK                  0x20UL                          /**< Bit mask for USART_RXUF */
#define _UART_IFS_RXUF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_RXUF_DEFAULT                (_UART_IFS_RXUF_DEFAULT << 5)   /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_TXOF                        (0x1UL << 6)                    /**< Set TX Overflow Interrupt Flag */
#define _UART_IFS_TXOF_SHIFT                 6                               /**< Shift value for USART_TXOF */
#define _UART_IFS_TXOF_MASK                  0x40UL                          /**< Bit mask for USART_TXOF */
#define _UART_IFS_TXOF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_TXOF_DEFAULT                (_UART_IFS_TXOF_DEFAULT << 6)   /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_TXUF                        (0x1UL << 7)                    /**< Set TX Underflow Interrupt Flag */
#define _UART_IFS_TXUF_SHIFT                 7                               /**< Shift value for USART_TXUF */
#define _UART_IFS_TXUF_MASK                  0x80UL                          /**< Bit mask for USART_TXUF */
#define _UART_IFS_TXUF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_TXUF_DEFAULT                (_UART_IFS_TXUF_DEFAULT << 7)   /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_PERR                        (0x1UL << 8)                    /**< Set Parity Error Interrupt Flag */
#define _UART_IFS_PERR_SHIFT                 8                               /**< Shift value for USART_PERR */
#define _UART_IFS_PERR_MASK                  0x100UL                         /**< Bit mask for USART_PERR */
#define _UART_IFS_PERR_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_PERR_DEFAULT                (_UART_IFS_PERR_DEFAULT << 8)   /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_FERR                        (0x1UL << 9)                    /**< Set Framing Error Interrupt Flag */
#define _UART_IFS_FERR_SHIFT                 9                               /**< Shift value for USART_FERR */
#define _UART_IFS_FERR_MASK                  0x200UL                         /**< Bit mask for USART_FERR */
#define _UART_IFS_FERR_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_FERR_DEFAULT                (_UART_IFS_FERR_DEFAULT << 9)   /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_MPAF                        (0x1UL << 10)                   /**< Set Multi-Processor Address Frame Interrupt Flag */
#define _UART_IFS_MPAF_SHIFT                 10                              /**< Shift value for USART_MPAF */
#define _UART_IFS_MPAF_MASK                  0x400UL                         /**< Bit mask for USART_MPAF */
#define _UART_IFS_MPAF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_MPAF_DEFAULT                (_UART_IFS_MPAF_DEFAULT << 10)  /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_SSM                         (0x1UL << 11)                   /**< Set Slave-Select in Master mode Interrupt Flag */
#define _UART_IFS_SSM_SHIFT                  11                              /**< Shift value for USART_SSM */
#define _UART_IFS_SSM_MASK                   0x800UL                         /**< Bit mask for USART_SSM */
#define _UART_IFS_SSM_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_SSM_DEFAULT                 (_UART_IFS_SSM_DEFAULT << 11)   /**< Shifted mode DEFAULT for UART_IFS */
#define UART_IFS_CCF                         (0x1UL << 12)                   /**< Set Collision Check Fail Interrupt Flag */
#define _UART_IFS_CCF_SHIFT                  12                              /**< Shift value for USART_CCF */
#define _UART_IFS_CCF_MASK                   0x1000UL                        /**< Bit mask for USART_CCF */
#define _UART_IFS_CCF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IFS */
#define UART_IFS_CCF_DEFAULT                 (_UART_IFS_CCF_DEFAULT << 12)   /**< Shifted mode DEFAULT for UART_IFS */

/* Bit fields for UART IFC */
#define _UART_IFC_RESETVALUE                 0x00000000UL                    /**< Default value for UART_IFC */
#define _UART_IFC_MASK                       0x00001FF9UL                    /**< Mask for UART_IFC */
#define UART_IFC_TXC                         (0x1UL << 0)                    /**< Clear TX Complete Interrupt Flag */
#define _UART_IFC_TXC_SHIFT                  0                               /**< Shift value for USART_TXC */
#define _UART_IFC_TXC_MASK                   0x1UL                           /**< Bit mask for USART_TXC */
#define _UART_IFC_TXC_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_TXC_DEFAULT                 (_UART_IFC_TXC_DEFAULT << 0)    /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_RXFULL                      (0x1UL << 3)                    /**< Clear RX Buffer Full Interrupt Flag */
#define _UART_IFC_RXFULL_SHIFT               3                               /**< Shift value for USART_RXFULL */
#define _UART_IFC_RXFULL_MASK                0x8UL                           /**< Bit mask for USART_RXFULL */
#define _UART_IFC_RXFULL_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_RXFULL_DEFAULT              (_UART_IFC_RXFULL_DEFAULT << 3) /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_RXOF                        (0x1UL << 4)                    /**< Clear RX Overflow Interrupt Flag */
#define _UART_IFC_RXOF_SHIFT                 4                               /**< Shift value for USART_RXOF */
#define _UART_IFC_RXOF_MASK                  0x10UL                          /**< Bit mask for USART_RXOF */
#define _UART_IFC_RXOF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_RXOF_DEFAULT                (_UART_IFC_RXOF_DEFAULT << 4)   /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_RXUF                        (0x1UL << 5)                    /**< Clear RX Underflow Interrupt Flag */
#define _UART_IFC_RXUF_SHIFT                 5                               /**< Shift value for USART_RXUF */
#define _UART_IFC_RXUF_MASK                  0x20UL                          /**< Bit mask for USART_RXUF */
#define _UART_IFC_RXUF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_RXUF_DEFAULT                (_UART_IFC_RXUF_DEFAULT << 5)   /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_TXOF                        (0x1UL << 6)                    /**< Clear TX Overflow Interrupt Flag */
#define _UART_IFC_TXOF_SHIFT                 6                               /**< Shift value for USART_TXOF */
#define _UART_IFC_TXOF_MASK                  0x40UL                          /**< Bit mask for USART_TXOF */
#define _UART_IFC_TXOF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_TXOF_DEFAULT                (_UART_IFC_TXOF_DEFAULT << 6)   /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_TXUF                        (0x1UL << 7)                    /**< Clear TX Underflow Interrupt Flag */
#define _UART_IFC_TXUF_SHIFT                 7                               /**< Shift value for USART_TXUF */
#define _UART_IFC_TXUF_MASK                  0x80UL                          /**< Bit mask for USART_TXUF */
#define _UART_IFC_TXUF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_TXUF_DEFAULT                (_UART_IFC_TXUF_DEFAULT << 7)   /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_PERR                        (0x1UL << 8)                    /**< Clear Parity Error Interrupt Flag */
#define _UART_IFC_PERR_SHIFT                 8                               /**< Shift value for USART_PERR */
#define _UART_IFC_PERR_MASK                  0x100UL                         /**< Bit mask for USART_PERR */
#define _UART_IFC_PERR_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_PERR_DEFAULT                (_UART_IFC_PERR_DEFAULT << 8)   /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_FERR                        (0x1UL << 9)                    /**< Clear Framing Error Interrupt Flag */
#define _UART_IFC_FERR_SHIFT                 9                               /**< Shift value for USART_FERR */
#define _UART_IFC_FERR_MASK                  0x200UL                         /**< Bit mask for USART_FERR */
#define _UART_IFC_FERR_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_FERR_DEFAULT                (_UART_IFC_FERR_DEFAULT << 9)   /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_MPAF                        (0x1UL << 10)                   /**< Clear Multi-Processor Address Frame Interrupt Flag */
#define _UART_IFC_MPAF_SHIFT                 10                              /**< Shift value for USART_MPAF */
#define _UART_IFC_MPAF_MASK                  0x400UL                         /**< Bit mask for USART_MPAF */
#define _UART_IFC_MPAF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_MPAF_DEFAULT                (_UART_IFC_MPAF_DEFAULT << 10)  /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_SSM                         (0x1UL << 11)                   /**< Clear Slave-Select In Master Mode Interrupt Flag */
#define _UART_IFC_SSM_SHIFT                  11                              /**< Shift value for USART_SSM */
#define _UART_IFC_SSM_MASK                   0x800UL                         /**< Bit mask for USART_SSM */
#define _UART_IFC_SSM_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_SSM_DEFAULT                 (_UART_IFC_SSM_DEFAULT << 11)   /**< Shifted mode DEFAULT for UART_IFC */
#define UART_IFC_CCF                         (0x1UL << 12)                   /**< Clear Collision Check Fail Interrupt Flag */
#define _UART_IFC_CCF_SHIFT                  12                              /**< Shift value for USART_CCF */
#define _UART_IFC_CCF_MASK                   0x1000UL                        /**< Bit mask for USART_CCF */
#define _UART_IFC_CCF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for UART_IFC */
#define UART_IFC_CCF_DEFAULT                 (_UART_IFC_CCF_DEFAULT << 12)   /**< Shifted mode DEFAULT for UART_IFC */

/* Bit fields for UART IEN */
#define _UART_IEN_RESETVALUE                 0x00000000UL                     /**< Default value for UART_IEN */
#define _UART_IEN_MASK                       0x00001FFFUL                     /**< Mask for UART_IEN */
#define UART_IEN_TXC                         (0x1UL << 0)                     /**< TX Complete Interrupt Enable */
#define _UART_IEN_TXC_SHIFT                  0                                /**< Shift value for USART_TXC */
#define _UART_IEN_TXC_MASK                   0x1UL                            /**< Bit mask for USART_TXC */
#define _UART_IEN_TXC_DEFAULT                0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_TXC_DEFAULT                 (_UART_IEN_TXC_DEFAULT << 0)     /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_TXBL                        (0x1UL << 1)                     /**< TX Buffer Level Interrupt Enable */
#define _UART_IEN_TXBL_SHIFT                 1                                /**< Shift value for USART_TXBL */
#define _UART_IEN_TXBL_MASK                  0x2UL                            /**< Bit mask for USART_TXBL */
#define _UART_IEN_TXBL_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_TXBL_DEFAULT                (_UART_IEN_TXBL_DEFAULT << 1)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_RXDATAV                     (0x1UL << 2)                     /**< RX Data Valid Interrupt Enable */
#define _UART_IEN_RXDATAV_SHIFT              2                                /**< Shift value for USART_RXDATAV */
#define _UART_IEN_RXDATAV_MASK               0x4UL                            /**< Bit mask for USART_RXDATAV */
#define _UART_IEN_RXDATAV_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_RXDATAV_DEFAULT             (_UART_IEN_RXDATAV_DEFAULT << 2) /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_RXFULL                      (0x1UL << 3)                     /**< RX Buffer Full Interrupt Enable */
#define _UART_IEN_RXFULL_SHIFT               3                                /**< Shift value for USART_RXFULL */
#define _UART_IEN_RXFULL_MASK                0x8UL                            /**< Bit mask for USART_RXFULL */
#define _UART_IEN_RXFULL_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_RXFULL_DEFAULT              (_UART_IEN_RXFULL_DEFAULT << 3)  /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_RXOF                        (0x1UL << 4)                     /**< RX Overflow Interrupt Enable */
#define _UART_IEN_RXOF_SHIFT                 4                                /**< Shift value for USART_RXOF */
#define _UART_IEN_RXOF_MASK                  0x10UL                           /**< Bit mask for USART_RXOF */
#define _UART_IEN_RXOF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_RXOF_DEFAULT                (_UART_IEN_RXOF_DEFAULT << 4)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_RXUF                        (0x1UL << 5)                     /**< RX Underflow Interrupt Enable */
#define _UART_IEN_RXUF_SHIFT                 5                                /**< Shift value for USART_RXUF */
#define _UART_IEN_RXUF_MASK                  0x20UL                           /**< Bit mask for USART_RXUF */
#define _UART_IEN_RXUF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_RXUF_DEFAULT                (_UART_IEN_RXUF_DEFAULT << 5)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_TXOF                        (0x1UL << 6)                     /**< TX Overflow Interrupt Enable */
#define _UART_IEN_TXOF_SHIFT                 6                                /**< Shift value for USART_TXOF */
#define _UART_IEN_TXOF_MASK                  0x40UL                           /**< Bit mask for USART_TXOF */
#define _UART_IEN_TXOF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_TXOF_DEFAULT                (_UART_IEN_TXOF_DEFAULT << 6)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_TXUF                        (0x1UL << 7)                     /**< TX Underflow Interrupt Enable */
#define _UART_IEN_TXUF_SHIFT                 7                                /**< Shift value for USART_TXUF */
#define _UART_IEN_TXUF_MASK                  0x80UL                           /**< Bit mask for USART_TXUF */
#define _UART_IEN_TXUF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_TXUF_DEFAULT                (_UART_IEN_TXUF_DEFAULT << 7)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_PERR                        (0x1UL << 8)                     /**< Parity Error Interrupt Enable */
#define _UART_IEN_PERR_SHIFT                 8                                /**< Shift value for USART_PERR */
#define _UART_IEN_PERR_MASK                  0x100UL                          /**< Bit mask for USART_PERR */
#define _UART_IEN_PERR_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_PERR_DEFAULT                (_UART_IEN_PERR_DEFAULT << 8)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_FERR                        (0x1UL << 9)                     /**< Framing Error Interrupt Enable */
#define _UART_IEN_FERR_SHIFT                 9                                /**< Shift value for USART_FERR */
#define _UART_IEN_FERR_MASK                  0x200UL                          /**< Bit mask for USART_FERR */
#define _UART_IEN_FERR_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_FERR_DEFAULT                (_UART_IEN_FERR_DEFAULT << 9)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_MPAF                        (0x1UL << 10)                    /**< Multi-Processor Address Frame Interrupt Enable */
#define _UART_IEN_MPAF_SHIFT                 10                               /**< Shift value for USART_MPAF */
#define _UART_IEN_MPAF_MASK                  0x400UL                          /**< Bit mask for USART_MPAF */
#define _UART_IEN_MPAF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_MPAF_DEFAULT                (_UART_IEN_MPAF_DEFAULT << 10)   /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_SSM                         (0x1UL << 11)                    /**< Slave-Select In Master Mode Interrupt Enable */
#define _UART_IEN_SSM_SHIFT                  11                               /**< Shift value for USART_SSM */
#define _UART_IEN_SSM_MASK                   0x800UL                          /**< Bit mask for USART_SSM */
#define _UART_IEN_SSM_DEFAULT                0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_SSM_DEFAULT                 (_UART_IEN_SSM_DEFAULT << 11)    /**< Shifted mode DEFAULT for UART_IEN */
#define UART_IEN_CCF                         (0x1UL << 12)                    /**< Collision Check Fail Interrupt Enable */
#define _UART_IEN_CCF_SHIFT                  12                               /**< Shift value for USART_CCF */
#define _UART_IEN_CCF_MASK                   0x1000UL                         /**< Bit mask for USART_CCF */
#define _UART_IEN_CCF_DEFAULT                0x00000000UL                     /**< Mode DEFAULT for UART_IEN */
#define UART_IEN_CCF_DEFAULT                 (_UART_IEN_CCF_DEFAULT << 12)    /**< Shifted mode DEFAULT for UART_IEN */

/* Bit fields for UART IRCTRL */
#define _UART_IRCTRL_RESETVALUE              0x00000000UL                         /**< Default value for UART_IRCTRL */
#define _UART_IRCTRL_MASK                    0x000000FFUL                         /**< Mask for UART_IRCTRL */
#define UART_IRCTRL_IREN                     (0x1UL << 0)                         /**< Enable IrDA Module */
#define _UART_IRCTRL_IREN_SHIFT              0                                    /**< Shift value for USART_IREN */
#define _UART_IRCTRL_IREN_MASK               0x1UL                                /**< Bit mask for USART_IREN */
#define _UART_IRCTRL_IREN_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for UART_IRCTRL */
#define UART_IRCTRL_IREN_DEFAULT             (_UART_IRCTRL_IREN_DEFAULT << 0)     /**< Shifted mode DEFAULT for UART_IRCTRL */
#define _UART_IRCTRL_IRPW_SHIFT              1                                    /**< Shift value for USART_IRPW */
#define _UART_IRCTRL_IRPW_MASK               0x6UL                                /**< Bit mask for USART_IRPW */
#define _UART_IRCTRL_IRPW_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for UART_IRCTRL */
#define _UART_IRCTRL_IRPW_ONE                0x00000000UL                         /**< Mode ONE for UART_IRCTRL */
#define _UART_IRCTRL_IRPW_TWO                0x00000001UL                         /**< Mode TWO for UART_IRCTRL */
#define _UART_IRCTRL_IRPW_THREE              0x00000002UL                         /**< Mode THREE for UART_IRCTRL */
#define _UART_IRCTRL_IRPW_FOUR               0x00000003UL                         /**< Mode FOUR for UART_IRCTRL */
#define UART_IRCTRL_IRPW_DEFAULT             (_UART_IRCTRL_IRPW_DEFAULT << 1)     /**< Shifted mode DEFAULT for UART_IRCTRL */
#define UART_IRCTRL_IRPW_ONE                 (_UART_IRCTRL_IRPW_ONE << 1)         /**< Shifted mode ONE for UART_IRCTRL */
#define UART_IRCTRL_IRPW_TWO                 (_UART_IRCTRL_IRPW_TWO << 1)         /**< Shifted mode TWO for UART_IRCTRL */
#define UART_IRCTRL_IRPW_THREE               (_UART_IRCTRL_IRPW_THREE << 1)       /**< Shifted mode THREE for UART_IRCTRL */
#define UART_IRCTRL_IRPW_FOUR                (_UART_IRCTRL_IRPW_FOUR << 1)        /**< Shifted mode FOUR for UART_IRCTRL */
#define UART_IRCTRL_IRFILT                   (0x1UL << 3)                         /**< IrDA RX Filter */
#define _UART_IRCTRL_IRFILT_SHIFT            3                                    /**< Shift value for USART_IRFILT */
#define _UART_IRCTRL_IRFILT_MASK             0x8UL                                /**< Bit mask for USART_IRFILT */
#define _UART_IRCTRL_IRFILT_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for UART_IRCTRL */
#define UART_IRCTRL_IRFILT_DEFAULT           (_UART_IRCTRL_IRFILT_DEFAULT << 3)   /**< Shifted mode DEFAULT for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_SHIFT          4                                    /**< Shift value for USART_IRPRSSEL */
#define _UART_IRCTRL_IRPRSSEL_MASK           0x70UL                               /**< Bit mask for USART_IRPRSSEL */
#define _UART_IRCTRL_IRPRSSEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH0         0x00000000UL                         /**< Mode PRSCH0 for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH1         0x00000001UL                         /**< Mode PRSCH1 for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH2         0x00000002UL                         /**< Mode PRSCH2 for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH3         0x00000003UL                         /**< Mode PRSCH3 for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH4         0x00000004UL                         /**< Mode PRSCH4 for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH5         0x00000005UL                         /**< Mode PRSCH5 for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH6         0x00000006UL                         /**< Mode PRSCH6 for UART_IRCTRL */
#define _UART_IRCTRL_IRPRSSEL_PRSCH7         0x00000007UL                         /**< Mode PRSCH7 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_DEFAULT         (_UART_IRCTRL_IRPRSSEL_DEFAULT << 4) /**< Shifted mode DEFAULT for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH0          (_UART_IRCTRL_IRPRSSEL_PRSCH0 << 4)  /**< Shifted mode PRSCH0 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH1          (_UART_IRCTRL_IRPRSSEL_PRSCH1 << 4)  /**< Shifted mode PRSCH1 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH2          (_UART_IRCTRL_IRPRSSEL_PRSCH2 << 4)  /**< Shifted mode PRSCH2 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH3          (_UART_IRCTRL_IRPRSSEL_PRSCH3 << 4)  /**< Shifted mode PRSCH3 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH4          (_UART_IRCTRL_IRPRSSEL_PRSCH4 << 4)  /**< Shifted mode PRSCH4 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH5          (_UART_IRCTRL_IRPRSSEL_PRSCH5 << 4)  /**< Shifted mode PRSCH5 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH6          (_UART_IRCTRL_IRPRSSEL_PRSCH6 << 4)  /**< Shifted mode PRSCH6 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSSEL_PRSCH7          (_UART_IRCTRL_IRPRSSEL_PRSCH7 << 4)  /**< Shifted mode PRSCH7 for UART_IRCTRL */
#define UART_IRCTRL_IRPRSEN                  (0x1UL << 7)                         /**< IrDA PRS Channel Enable */
#define _UART_IRCTRL_IRPRSEN_SHIFT           7                                    /**< Shift value for USART_IRPRSEN */
#define _UART_IRCTRL_IRPRSEN_MASK            0x80UL                               /**< Bit mask for USART_IRPRSEN */
#define _UART_IRCTRL_IRPRSEN_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for UART_IRCTRL */
#define UART_IRCTRL_IRPRSEN_DEFAULT          (_UART_IRCTRL_IRPRSEN_DEFAULT << 7)  /**< Shifted mode DEFAULT for UART_IRCTRL */

/* Bit fields for UART ROUTE */
#define _UART_ROUTE_RESETVALUE               0x00000000UL                        /**< Default value for UART_ROUTE */
#define _UART_ROUTE_MASK                     0x0000070FUL                        /**< Mask for UART_ROUTE */
#define UART_ROUTE_RXPEN                     (0x1UL << 0)                        /**< RX Pin Enable */
#define _UART_ROUTE_RXPEN_SHIFT              0                                   /**< Shift value for USART_RXPEN */
#define _UART_ROUTE_RXPEN_MASK               0x1UL                               /**< Bit mask for USART_RXPEN */
#define _UART_ROUTE_RXPEN_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_RXPEN_DEFAULT             (_UART_ROUTE_RXPEN_DEFAULT << 0)    /**< Shifted mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_TXPEN                     (0x1UL << 1)                        /**< TX Pin Enable */
#define _UART_ROUTE_TXPEN_SHIFT              1                                   /**< Shift value for USART_TXPEN */
#define _UART_ROUTE_TXPEN_MASK               0x2UL                               /**< Bit mask for USART_TXPEN */
#define _UART_ROUTE_TXPEN_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_TXPEN_DEFAULT             (_UART_ROUTE_TXPEN_DEFAULT << 1)    /**< Shifted mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_CSPEN                     (0x1UL << 2)                        /**< CS Pin Enable */
#define _UART_ROUTE_CSPEN_SHIFT              2                                   /**< Shift value for USART_CSPEN */
#define _UART_ROUTE_CSPEN_MASK               0x4UL                               /**< Bit mask for USART_CSPEN */
#define _UART_ROUTE_CSPEN_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_CSPEN_DEFAULT             (_UART_ROUTE_CSPEN_DEFAULT << 2)    /**< Shifted mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_CLKPEN                    (0x1UL << 3)                        /**< CLK Pin Enable */
#define _UART_ROUTE_CLKPEN_SHIFT             3                                   /**< Shift value for USART_CLKPEN */
#define _UART_ROUTE_CLKPEN_MASK              0x8UL                               /**< Bit mask for USART_CLKPEN */
#define _UART_ROUTE_CLKPEN_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_CLKPEN_DEFAULT            (_UART_ROUTE_CLKPEN_DEFAULT << 3)   /**< Shifted mode DEFAULT for UART_ROUTE */
#define _UART_ROUTE_LOCATION_SHIFT           8                                   /**< Shift value for USART_LOCATION */
#define _UART_ROUTE_LOCATION_MASK            0x700UL                             /**< Bit mask for USART_LOCATION */
#define _UART_ROUTE_LOCATION_LOC0            0x00000000UL                        /**< Mode LOC0 for UART_ROUTE */
#define _UART_ROUTE_LOCATION_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for UART_ROUTE */
#define _UART_ROUTE_LOCATION_LOC1            0x00000001UL                        /**< Mode LOC1 for UART_ROUTE */
#define _UART_ROUTE_LOCATION_LOC2            0x00000002UL                        /**< Mode LOC2 for UART_ROUTE */
#define _UART_ROUTE_LOCATION_LOC3            0x00000003UL                        /**< Mode LOC3 for UART_ROUTE */
#define _UART_ROUTE_LOCATION_LOC4            0x00000004UL                        /**< Mode LOC4 for UART_ROUTE */
#define _UART_ROUTE_LOCATION_LOC5            0x00000005UL                        /**< Mode LOC5 for UART_ROUTE */
#define UART_ROUTE_LOCATION_LOC0             (_UART_ROUTE_LOCATION_LOC0 << 8)    /**< Shifted mode LOC0 for UART_ROUTE */
#define UART_ROUTE_LOCATION_DEFAULT          (_UART_ROUTE_LOCATION_DEFAULT << 8) /**< Shifted mode DEFAULT for UART_ROUTE */
#define UART_ROUTE_LOCATION_LOC1             (_UART_ROUTE_LOCATION_LOC1 << 8)    /**< Shifted mode LOC1 for UART_ROUTE */
#define UART_ROUTE_LOCATION_LOC2             (_UART_ROUTE_LOCATION_LOC2 << 8)    /**< Shifted mode LOC2 for UART_ROUTE */
#define UART_ROUTE_LOCATION_LOC3             (_UART_ROUTE_LOCATION_LOC3 << 8)    /**< Shifted mode LOC3 for UART_ROUTE */
#define UART_ROUTE_LOCATION_LOC4             (_UART_ROUTE_LOCATION_LOC4 << 8)    /**< Shifted mode LOC4 for UART_ROUTE */
#define UART_ROUTE_LOCATION_LOC5             (_UART_ROUTE_LOCATION_LOC5 << 8)    /**< Shifted mode LOC5 for UART_ROUTE */

/* Bit fields for UART INPUT */
#define _UART_INPUT_RESETVALUE               0x00000000UL                        /**< Default value for UART_INPUT */
#define _UART_INPUT_MASK                     0x0000001FUL                        /**< Mask for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_SHIFT           0                                   /**< Shift value for USART_RXPRSSEL */
#define _UART_INPUT_RXPRSSEL_MASK            0xFUL                               /**< Bit mask for USART_RXPRSSEL */
#define _UART_INPUT_RXPRSSEL_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH0          0x00000000UL                        /**< Mode PRSCH0 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH1          0x00000001UL                        /**< Mode PRSCH1 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH2          0x00000002UL                        /**< Mode PRSCH2 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH3          0x00000003UL                        /**< Mode PRSCH3 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH4          0x00000004UL                        /**< Mode PRSCH4 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH5          0x00000005UL                        /**< Mode PRSCH5 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH6          0x00000006UL                        /**< Mode PRSCH6 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH7          0x00000007UL                        /**< Mode PRSCH7 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH8          0x00000008UL                        /**< Mode PRSCH8 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH9          0x00000009UL                        /**< Mode PRSCH9 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH10         0x0000000AUL                        /**< Mode PRSCH10 for UART_INPUT */
#define _UART_INPUT_RXPRSSEL_PRSCH11         0x0000000BUL                        /**< Mode PRSCH11 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_DEFAULT          (_UART_INPUT_RXPRSSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH0           (_UART_INPUT_RXPRSSEL_PRSCH0 << 0)  /**< Shifted mode PRSCH0 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH1           (_UART_INPUT_RXPRSSEL_PRSCH1 << 0)  /**< Shifted mode PRSCH1 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH2           (_UART_INPUT_RXPRSSEL_PRSCH2 << 0)  /**< Shifted mode PRSCH2 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH3           (_UART_INPUT_RXPRSSEL_PRSCH3 << 0)  /**< Shifted mode PRSCH3 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH4           (_UART_INPUT_RXPRSSEL_PRSCH4 << 0)  /**< Shifted mode PRSCH4 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH5           (_UART_INPUT_RXPRSSEL_PRSCH5 << 0)  /**< Shifted mode PRSCH5 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH6           (_UART_INPUT_RXPRSSEL_PRSCH6 << 0)  /**< Shifted mode PRSCH6 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH7           (_UART_INPUT_RXPRSSEL_PRSCH7 << 0)  /**< Shifted mode PRSCH7 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH8           (_UART_INPUT_RXPRSSEL_PRSCH8 << 0)  /**< Shifted mode PRSCH8 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH9           (_UART_INPUT_RXPRSSEL_PRSCH9 << 0)  /**< Shifted mode PRSCH9 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH10          (_UART_INPUT_RXPRSSEL_PRSCH10 << 0) /**< Shifted mode PRSCH10 for UART_INPUT */
#define UART_INPUT_RXPRSSEL_PRSCH11          (_UART_INPUT_RXPRSSEL_PRSCH11 << 0) /**< Shifted mode PRSCH11 for UART_INPUT */
#define UART_INPUT_RXPRS                     (0x1UL << 4)                        /**< PRS RX Enable */
#define _UART_INPUT_RXPRS_SHIFT              4                                   /**< Shift value for USART_RXPRS */
#define _UART_INPUT_RXPRS_MASK               0x10UL                              /**< Bit mask for USART_RXPRS */
#define _UART_INPUT_RXPRS_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for UART_INPUT */
#define UART_INPUT_RXPRS_DEFAULT             (_UART_INPUT_RXPRS_DEFAULT << 4)    /**< Shifted mode DEFAULT for UART_INPUT */

/* Bit fields for UART I2SCTRL */
#define _UART_I2SCTRL_RESETVALUE             0x00000000UL                          /**< Default value for UART_I2SCTRL */
#define _UART_I2SCTRL_MASK                   0x0000071FUL                          /**< Mask for UART_I2SCTRL */
#define UART_I2SCTRL_EN                      (0x1UL << 0)                          /**< Enable I2S Mode */
#define _UART_I2SCTRL_EN_SHIFT               0                                     /**< Shift value for USART_EN */
#define _UART_I2SCTRL_EN_MASK                0x1UL                                 /**< Bit mask for USART_EN */
#define _UART_I2SCTRL_EN_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_EN_DEFAULT              (_UART_I2SCTRL_EN_DEFAULT << 0)       /**< Shifted mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_MONO                    (0x1UL << 1)                          /**< Stero or Mono */
#define _UART_I2SCTRL_MONO_SHIFT             1                                     /**< Shift value for USART_MONO */
#define _UART_I2SCTRL_MONO_MASK              0x2UL                                 /**< Bit mask for USART_MONO */
#define _UART_I2SCTRL_MONO_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_MONO_DEFAULT            (_UART_I2SCTRL_MONO_DEFAULT << 1)     /**< Shifted mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_JUSTIFY                 (0x1UL << 2)                          /**< Justification of I2S Data */
#define _UART_I2SCTRL_JUSTIFY_SHIFT          2                                     /**< Shift value for USART_JUSTIFY */
#define _UART_I2SCTRL_JUSTIFY_MASK           0x4UL                                 /**< Bit mask for USART_JUSTIFY */
#define _UART_I2SCTRL_JUSTIFY_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for UART_I2SCTRL */
#define _UART_I2SCTRL_JUSTIFY_LEFT           0x00000000UL                          /**< Mode LEFT for UART_I2SCTRL */
#define _UART_I2SCTRL_JUSTIFY_RIGHT          0x00000001UL                          /**< Mode RIGHT for UART_I2SCTRL */
#define UART_I2SCTRL_JUSTIFY_DEFAULT         (_UART_I2SCTRL_JUSTIFY_DEFAULT << 2)  /**< Shifted mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_JUSTIFY_LEFT            (_UART_I2SCTRL_JUSTIFY_LEFT << 2)     /**< Shifted mode LEFT for UART_I2SCTRL */
#define UART_I2SCTRL_JUSTIFY_RIGHT           (_UART_I2SCTRL_JUSTIFY_RIGHT << 2)    /**< Shifted mode RIGHT for UART_I2SCTRL */
#define UART_I2SCTRL_DMASPLIT                (0x1UL << 3)                          /**< Separate DMA Request For Left/Right Data */
#define _UART_I2SCTRL_DMASPLIT_SHIFT         3                                     /**< Shift value for USART_DMASPLIT */
#define _UART_I2SCTRL_DMASPLIT_MASK          0x8UL                                 /**< Bit mask for USART_DMASPLIT */
#define _UART_I2SCTRL_DMASPLIT_DEFAULT       0x00000000UL                          /**< Mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_DMASPLIT_DEFAULT        (_UART_I2SCTRL_DMASPLIT_DEFAULT << 3) /**< Shifted mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_DELAY                   (0x1UL << 4)                          /**< Delay on I2S data */
#define _UART_I2SCTRL_DELAY_SHIFT            4                                     /**< Shift value for USART_DELAY */
#define _UART_I2SCTRL_DELAY_MASK             0x10UL                                /**< Bit mask for USART_DELAY */
#define _UART_I2SCTRL_DELAY_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_DELAY_DEFAULT           (_UART_I2SCTRL_DELAY_DEFAULT << 4)    /**< Shifted mode DEFAULT for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_SHIFT           8                                     /**< Shift value for USART_FORMAT */
#define _UART_I2SCTRL_FORMAT_MASK            0x700UL                               /**< Bit mask for USART_FORMAT */
#define _UART_I2SCTRL_FORMAT_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W32D32          0x00000000UL                          /**< Mode W32D32 for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W32D24M         0x00000001UL                          /**< Mode W32D24M for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W32D24          0x00000002UL                          /**< Mode W32D24 for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W32D16          0x00000003UL                          /**< Mode W32D16 for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W32D8           0x00000004UL                          /**< Mode W32D8 for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W16D16          0x00000005UL                          /**< Mode W16D16 for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W16D8           0x00000006UL                          /**< Mode W16D8 for UART_I2SCTRL */
#define _UART_I2SCTRL_FORMAT_W8D8            0x00000007UL                          /**< Mode W8D8 for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_DEFAULT          (_UART_I2SCTRL_FORMAT_DEFAULT << 8)   /**< Shifted mode DEFAULT for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W32D32           (_UART_I2SCTRL_FORMAT_W32D32 << 8)    /**< Shifted mode W32D32 for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W32D24M          (_UART_I2SCTRL_FORMAT_W32D24M << 8)   /**< Shifted mode W32D24M for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W32D24           (_UART_I2SCTRL_FORMAT_W32D24 << 8)    /**< Shifted mode W32D24 for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W32D16           (_UART_I2SCTRL_FORMAT_W32D16 << 8)    /**< Shifted mode W32D16 for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W32D8            (_UART_I2SCTRL_FORMAT_W32D8 << 8)     /**< Shifted mode W32D8 for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W16D16           (_UART_I2SCTRL_FORMAT_W16D16 << 8)    /**< Shifted mode W16D16 for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W16D8            (_UART_I2SCTRL_FORMAT_W16D8 << 8)     /**< Shifted mode W16D8 for UART_I2SCTRL */
#define UART_I2SCTRL_FORMAT_W8D8             (_UART_I2SCTRL_FORMAT_W8D8 << 8)      /**< Shifted mode W8D8 for UART_I2SCTRL */

/** @} End of group EFM32WG_UART */
/** @} End of group Parts */

