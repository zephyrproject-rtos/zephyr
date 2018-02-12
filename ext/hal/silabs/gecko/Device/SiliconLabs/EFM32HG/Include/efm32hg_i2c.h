/**************************************************************************//**
 * @file efm32hg_i2c.h
 * @brief EFM32HG_I2C register and bit field definitions
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
 * @defgroup EFM32HG_I2C
 * @{
 * @brief EFM32HG_I2C Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;      /**< Control Register  */
  __IOM uint32_t CMD;       /**< Command Register  */
  __IM uint32_t  STATE;     /**< State Register  */
  __IM uint32_t  STATUS;    /**< Status Register  */
  __IOM uint32_t CLKDIV;    /**< Clock Division Register  */
  __IOM uint32_t SADDR;     /**< Slave Address Register  */
  __IOM uint32_t SADDRMASK; /**< Slave Address Mask Register  */
  __IM uint32_t  RXDATA;    /**< Receive Buffer Data Register  */
  __IM uint32_t  RXDATAP;   /**< Receive Buffer Data Peek Register  */
  __IOM uint32_t TXDATA;    /**< Transmit Buffer Data Register  */
  __IM uint32_t  IF;        /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;       /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;       /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;       /**< Interrupt Enable Register  */
  __IOM uint32_t ROUTE;     /**< I/O Routing Register  */
} I2C_TypeDef;              /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_I2C_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for I2C CTRL */
#define _I2C_CTRL_RESETVALUE              0x00000000UL                     /**< Default value for I2C_CTRL */
#define _I2C_CTRL_MASK                    0x0007B37FUL                     /**< Mask for I2C_CTRL */
#define I2C_CTRL_EN                       (0x1UL << 0)                     /**< I2C Enable */
#define _I2C_CTRL_EN_SHIFT                0                                /**< Shift value for I2C_EN */
#define _I2C_CTRL_EN_MASK                 0x1UL                            /**< Bit mask for I2C_EN */
#define _I2C_CTRL_EN_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_EN_DEFAULT               (_I2C_CTRL_EN_DEFAULT << 0)      /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_SLAVE                    (0x1UL << 1)                     /**< Addressable as Slave */
#define _I2C_CTRL_SLAVE_SHIFT             1                                /**< Shift value for I2C_SLAVE */
#define _I2C_CTRL_SLAVE_MASK              0x2UL                            /**< Bit mask for I2C_SLAVE */
#define _I2C_CTRL_SLAVE_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_SLAVE_DEFAULT            (_I2C_CTRL_SLAVE_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOACK                  (0x1UL << 2)                     /**< Automatic Acknowledge */
#define _I2C_CTRL_AUTOACK_SHIFT           2                                /**< Shift value for I2C_AUTOACK */
#define _I2C_CTRL_AUTOACK_MASK            0x4UL                            /**< Bit mask for I2C_AUTOACK */
#define _I2C_CTRL_AUTOACK_DEFAULT         0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOACK_DEFAULT          (_I2C_CTRL_AUTOACK_DEFAULT << 2) /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSE                   (0x1UL << 3)                     /**< Automatic STOP when Empty */
#define _I2C_CTRL_AUTOSE_SHIFT            3                                /**< Shift value for I2C_AUTOSE */
#define _I2C_CTRL_AUTOSE_MASK             0x8UL                            /**< Bit mask for I2C_AUTOSE */
#define _I2C_CTRL_AUTOSE_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSE_DEFAULT           (_I2C_CTRL_AUTOSE_DEFAULT << 3)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSN                   (0x1UL << 4)                     /**< Automatic STOP on NACK */
#define _I2C_CTRL_AUTOSN_SHIFT            4                                /**< Shift value for I2C_AUTOSN */
#define _I2C_CTRL_AUTOSN_MASK             0x10UL                           /**< Bit mask for I2C_AUTOSN */
#define _I2C_CTRL_AUTOSN_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSN_DEFAULT           (_I2C_CTRL_AUTOSN_DEFAULT << 4)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_ARBDIS                   (0x1UL << 5)                     /**< Arbitration Disable */
#define _I2C_CTRL_ARBDIS_SHIFT            5                                /**< Shift value for I2C_ARBDIS */
#define _I2C_CTRL_ARBDIS_MASK             0x20UL                           /**< Bit mask for I2C_ARBDIS */
#define _I2C_CTRL_ARBDIS_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_ARBDIS_DEFAULT           (_I2C_CTRL_ARBDIS_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_GCAMEN                   (0x1UL << 6)                     /**< General Call Address Match Enable */
#define _I2C_CTRL_GCAMEN_SHIFT            6                                /**< Shift value for I2C_GCAMEN */
#define _I2C_CTRL_GCAMEN_MASK             0x40UL                           /**< Bit mask for I2C_GCAMEN */
#define _I2C_CTRL_GCAMEN_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_GCAMEN_DEFAULT           (_I2C_CTRL_GCAMEN_DEFAULT << 6)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_CLHR_SHIFT              8                                /**< Shift value for I2C_CLHR */
#define _I2C_CTRL_CLHR_MASK               0x300UL                          /**< Bit mask for I2C_CLHR */
#define _I2C_CTRL_CLHR_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_CLHR_STANDARD           0x00000000UL                     /**< Mode STANDARD for I2C_CTRL */
#define _I2C_CTRL_CLHR_ASYMMETRIC         0x00000001UL                     /**< Mode ASYMMETRIC for I2C_CTRL */
#define _I2C_CTRL_CLHR_FAST               0x00000002UL                     /**< Mode FAST for I2C_CTRL */
#define I2C_CTRL_CLHR_DEFAULT             (_I2C_CTRL_CLHR_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_CLHR_STANDARD            (_I2C_CTRL_CLHR_STANDARD << 8)   /**< Shifted mode STANDARD for I2C_CTRL */
#define I2C_CTRL_CLHR_ASYMMETRIC          (_I2C_CTRL_CLHR_ASYMMETRIC << 8) /**< Shifted mode ASYMMETRIC for I2C_CTRL */
#define I2C_CTRL_CLHR_FAST                (_I2C_CTRL_CLHR_FAST << 8)       /**< Shifted mode FAST for I2C_CTRL */
#define _I2C_CTRL_BITO_SHIFT              12                               /**< Shift value for I2C_BITO */
#define _I2C_CTRL_BITO_MASK               0x3000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_CTRL_BITO_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_BITO_OFF                0x00000000UL                     /**< Mode OFF for I2C_CTRL */
#define _I2C_CTRL_BITO_40PCC              0x00000001UL                     /**< Mode 40PCC for I2C_CTRL */
#define _I2C_CTRL_BITO_80PCC              0x00000002UL                     /**< Mode 80PCC for I2C_CTRL */
#define _I2C_CTRL_BITO_160PCC             0x00000003UL                     /**< Mode 160PCC for I2C_CTRL */
#define I2C_CTRL_BITO_DEFAULT             (_I2C_CTRL_BITO_DEFAULT << 12)   /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_BITO_OFF                 (_I2C_CTRL_BITO_OFF << 12)       /**< Shifted mode OFF for I2C_CTRL */
#define I2C_CTRL_BITO_40PCC               (_I2C_CTRL_BITO_40PCC << 12)     /**< Shifted mode 40PCC for I2C_CTRL */
#define I2C_CTRL_BITO_80PCC               (_I2C_CTRL_BITO_80PCC << 12)     /**< Shifted mode 80PCC for I2C_CTRL */
#define I2C_CTRL_BITO_160PCC              (_I2C_CTRL_BITO_160PCC << 12)    /**< Shifted mode 160PCC for I2C_CTRL */
#define I2C_CTRL_GIBITO                   (0x1UL << 15)                    /**< Go Idle on Bus Idle Timeout  */
#define _I2C_CTRL_GIBITO_SHIFT            15                               /**< Shift value for I2C_GIBITO */
#define _I2C_CTRL_GIBITO_MASK             0x8000UL                         /**< Bit mask for I2C_GIBITO */
#define _I2C_CTRL_GIBITO_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_GIBITO_DEFAULT           (_I2C_CTRL_GIBITO_DEFAULT << 15) /**< Shifted mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_CLTO_SHIFT              16                               /**< Shift value for I2C_CLTO */
#define _I2C_CTRL_CLTO_MASK               0x70000UL                        /**< Bit mask for I2C_CLTO */
#define _I2C_CTRL_CLTO_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_CLTO_OFF                0x00000000UL                     /**< Mode OFF for I2C_CTRL */
#define _I2C_CTRL_CLTO_40PCC              0x00000001UL                     /**< Mode 40PCC for I2C_CTRL */
#define _I2C_CTRL_CLTO_80PCC              0x00000002UL                     /**< Mode 80PCC for I2C_CTRL */
#define _I2C_CTRL_CLTO_160PCC             0x00000003UL                     /**< Mode 160PCC for I2C_CTRL */
#define _I2C_CTRL_CLTO_320PPC             0x00000004UL                     /**< Mode 320PPC for I2C_CTRL */
#define _I2C_CTRL_CLTO_1024PPC            0x00000005UL                     /**< Mode 1024PPC for I2C_CTRL */
#define I2C_CTRL_CLTO_DEFAULT             (_I2C_CTRL_CLTO_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_CLTO_OFF                 (_I2C_CTRL_CLTO_OFF << 16)       /**< Shifted mode OFF for I2C_CTRL */
#define I2C_CTRL_CLTO_40PCC               (_I2C_CTRL_CLTO_40PCC << 16)     /**< Shifted mode 40PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_80PCC               (_I2C_CTRL_CLTO_80PCC << 16)     /**< Shifted mode 80PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_160PCC              (_I2C_CTRL_CLTO_160PCC << 16)    /**< Shifted mode 160PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_320PPC              (_I2C_CTRL_CLTO_320PPC << 16)    /**< Shifted mode 320PPC for I2C_CTRL */
#define I2C_CTRL_CLTO_1024PPC             (_I2C_CTRL_CLTO_1024PPC << 16)   /**< Shifted mode 1024PPC for I2C_CTRL */

/* Bit fields for I2C CMD */
#define _I2C_CMD_RESETVALUE               0x00000000UL                    /**< Default value for I2C_CMD */
#define _I2C_CMD_MASK                     0x000000FFUL                    /**< Mask for I2C_CMD */
#define I2C_CMD_START                     (0x1UL << 0)                    /**< Send start condition */
#define _I2C_CMD_START_SHIFT              0                               /**< Shift value for I2C_START */
#define _I2C_CMD_START_MASK               0x1UL                           /**< Bit mask for I2C_START */
#define _I2C_CMD_START_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_START_DEFAULT             (_I2C_CMD_START_DEFAULT << 0)   /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_STOP                      (0x1UL << 1)                    /**< Send stop condition */
#define _I2C_CMD_STOP_SHIFT               1                               /**< Shift value for I2C_STOP */
#define _I2C_CMD_STOP_MASK                0x2UL                           /**< Bit mask for I2C_STOP */
#define _I2C_CMD_STOP_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_STOP_DEFAULT              (_I2C_CMD_STOP_DEFAULT << 1)    /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_ACK                       (0x1UL << 2)                    /**< Send ACK */
#define _I2C_CMD_ACK_SHIFT                2                               /**< Shift value for I2C_ACK */
#define _I2C_CMD_ACK_MASK                 0x4UL                           /**< Bit mask for I2C_ACK */
#define _I2C_CMD_ACK_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_ACK_DEFAULT               (_I2C_CMD_ACK_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_NACK                      (0x1UL << 3)                    /**< Send NACK */
#define _I2C_CMD_NACK_SHIFT               3                               /**< Shift value for I2C_NACK */
#define _I2C_CMD_NACK_MASK                0x8UL                           /**< Bit mask for I2C_NACK */
#define _I2C_CMD_NACK_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_NACK_DEFAULT              (_I2C_CMD_NACK_DEFAULT << 3)    /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_CONT                      (0x1UL << 4)                    /**< Continue transmission */
#define _I2C_CMD_CONT_SHIFT               4                               /**< Shift value for I2C_CONT */
#define _I2C_CMD_CONT_MASK                0x10UL                          /**< Bit mask for I2C_CONT */
#define _I2C_CMD_CONT_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_CONT_DEFAULT              (_I2C_CMD_CONT_DEFAULT << 4)    /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_ABORT                     (0x1UL << 5)                    /**< Abort transmission */
#define _I2C_CMD_ABORT_SHIFT              5                               /**< Shift value for I2C_ABORT */
#define _I2C_CMD_ABORT_MASK               0x20UL                          /**< Bit mask for I2C_ABORT */
#define _I2C_CMD_ABORT_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_ABORT_DEFAULT             (_I2C_CMD_ABORT_DEFAULT << 5)   /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARTX                   (0x1UL << 6)                    /**< Clear TX */
#define _I2C_CMD_CLEARTX_SHIFT            6                               /**< Shift value for I2C_CLEARTX */
#define _I2C_CMD_CLEARTX_MASK             0x40UL                          /**< Bit mask for I2C_CLEARTX */
#define _I2C_CMD_CLEARTX_DEFAULT          0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARTX_DEFAULT           (_I2C_CMD_CLEARTX_DEFAULT << 6) /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARPC                   (0x1UL << 7)                    /**< Clear Pending Commands */
#define _I2C_CMD_CLEARPC_SHIFT            7                               /**< Shift value for I2C_CLEARPC */
#define _I2C_CMD_CLEARPC_MASK             0x80UL                          /**< Bit mask for I2C_CLEARPC */
#define _I2C_CMD_CLEARPC_DEFAULT          0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARPC_DEFAULT           (_I2C_CMD_CLEARPC_DEFAULT << 7) /**< Shifted mode DEFAULT for I2C_CMD */

/* Bit fields for I2C STATE */
#define _I2C_STATE_RESETVALUE             0x00000001UL                          /**< Default value for I2C_STATE */
#define _I2C_STATE_MASK                   0x000000FFUL                          /**< Mask for I2C_STATE */
#define I2C_STATE_BUSY                    (0x1UL << 0)                          /**< Bus Busy */
#define _I2C_STATE_BUSY_SHIFT             0                                     /**< Shift value for I2C_BUSY */
#define _I2C_STATE_BUSY_MASK              0x1UL                                 /**< Bit mask for I2C_BUSY */
#define _I2C_STATE_BUSY_DEFAULT           0x00000001UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_BUSY_DEFAULT            (_I2C_STATE_BUSY_DEFAULT << 0)        /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_MASTER                  (0x1UL << 1)                          /**< Master */
#define _I2C_STATE_MASTER_SHIFT           1                                     /**< Shift value for I2C_MASTER */
#define _I2C_STATE_MASTER_MASK            0x2UL                                 /**< Bit mask for I2C_MASTER */
#define _I2C_STATE_MASTER_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_MASTER_DEFAULT          (_I2C_STATE_MASTER_DEFAULT << 1)      /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_TRANSMITTER             (0x1UL << 2)                          /**< Transmitter */
#define _I2C_STATE_TRANSMITTER_SHIFT      2                                     /**< Shift value for I2C_TRANSMITTER */
#define _I2C_STATE_TRANSMITTER_MASK       0x4UL                                 /**< Bit mask for I2C_TRANSMITTER */
#define _I2C_STATE_TRANSMITTER_DEFAULT    0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_TRANSMITTER_DEFAULT     (_I2C_STATE_TRANSMITTER_DEFAULT << 2) /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_NACKED                  (0x1UL << 3)                          /**< Nack Received */
#define _I2C_STATE_NACKED_SHIFT           3                                     /**< Shift value for I2C_NACKED */
#define _I2C_STATE_NACKED_MASK            0x8UL                                 /**< Bit mask for I2C_NACKED */
#define _I2C_STATE_NACKED_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_NACKED_DEFAULT          (_I2C_STATE_NACKED_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_BUSHOLD                 (0x1UL << 4)                          /**< Bus Held */
#define _I2C_STATE_BUSHOLD_SHIFT          4                                     /**< Shift value for I2C_BUSHOLD */
#define _I2C_STATE_BUSHOLD_MASK           0x10UL                                /**< Bit mask for I2C_BUSHOLD */
#define _I2C_STATE_BUSHOLD_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_BUSHOLD_DEFAULT         (_I2C_STATE_BUSHOLD_DEFAULT << 4)     /**< Shifted mode DEFAULT for I2C_STATE */
#define _I2C_STATE_STATE_SHIFT            5                                     /**< Shift value for I2C_STATE */
#define _I2C_STATE_STATE_MASK             0xE0UL                                /**< Bit mask for I2C_STATE */
#define _I2C_STATE_STATE_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define _I2C_STATE_STATE_IDLE             0x00000000UL                          /**< Mode IDLE for I2C_STATE */
#define _I2C_STATE_STATE_WAIT             0x00000001UL                          /**< Mode WAIT for I2C_STATE */
#define _I2C_STATE_STATE_START            0x00000002UL                          /**< Mode START for I2C_STATE */
#define _I2C_STATE_STATE_ADDR             0x00000003UL                          /**< Mode ADDR for I2C_STATE */
#define _I2C_STATE_STATE_ADDRACK          0x00000004UL                          /**< Mode ADDRACK for I2C_STATE */
#define _I2C_STATE_STATE_DATA             0x00000005UL                          /**< Mode DATA for I2C_STATE */
#define _I2C_STATE_STATE_DATAACK          0x00000006UL                          /**< Mode DATAACK for I2C_STATE */
#define I2C_STATE_STATE_DEFAULT           (_I2C_STATE_STATE_DEFAULT << 5)       /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_STATE_IDLE              (_I2C_STATE_STATE_IDLE << 5)          /**< Shifted mode IDLE for I2C_STATE */
#define I2C_STATE_STATE_WAIT              (_I2C_STATE_STATE_WAIT << 5)          /**< Shifted mode WAIT for I2C_STATE */
#define I2C_STATE_STATE_START             (_I2C_STATE_STATE_START << 5)         /**< Shifted mode START for I2C_STATE */
#define I2C_STATE_STATE_ADDR              (_I2C_STATE_STATE_ADDR << 5)          /**< Shifted mode ADDR for I2C_STATE */
#define I2C_STATE_STATE_ADDRACK           (_I2C_STATE_STATE_ADDRACK << 5)       /**< Shifted mode ADDRACK for I2C_STATE */
#define I2C_STATE_STATE_DATA              (_I2C_STATE_STATE_DATA << 5)          /**< Shifted mode DATA for I2C_STATE */
#define I2C_STATE_STATE_DATAACK           (_I2C_STATE_STATE_DATAACK << 5)       /**< Shifted mode DATAACK for I2C_STATE */

/* Bit fields for I2C STATUS */
#define _I2C_STATUS_RESETVALUE            0x00000080UL                       /**< Default value for I2C_STATUS */
#define _I2C_STATUS_MASK                  0x000001FFUL                       /**< Mask for I2C_STATUS */
#define I2C_STATUS_PSTART                 (0x1UL << 0)                       /**< Pending START */
#define _I2C_STATUS_PSTART_SHIFT          0                                  /**< Shift value for I2C_PSTART */
#define _I2C_STATUS_PSTART_MASK           0x1UL                              /**< Bit mask for I2C_PSTART */
#define _I2C_STATUS_PSTART_DEFAULT        0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PSTART_DEFAULT         (_I2C_STATUS_PSTART_DEFAULT << 0)  /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PSTOP                  (0x1UL << 1)                       /**< Pending STOP */
#define _I2C_STATUS_PSTOP_SHIFT           1                                  /**< Shift value for I2C_PSTOP */
#define _I2C_STATUS_PSTOP_MASK            0x2UL                              /**< Bit mask for I2C_PSTOP */
#define _I2C_STATUS_PSTOP_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PSTOP_DEFAULT          (_I2C_STATUS_PSTOP_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PACK                   (0x1UL << 2)                       /**< Pending ACK */
#define _I2C_STATUS_PACK_SHIFT            2                                  /**< Shift value for I2C_PACK */
#define _I2C_STATUS_PACK_MASK             0x4UL                              /**< Bit mask for I2C_PACK */
#define _I2C_STATUS_PACK_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PACK_DEFAULT           (_I2C_STATUS_PACK_DEFAULT << 2)    /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PNACK                  (0x1UL << 3)                       /**< Pending NACK */
#define _I2C_STATUS_PNACK_SHIFT           3                                  /**< Shift value for I2C_PNACK */
#define _I2C_STATUS_PNACK_MASK            0x8UL                              /**< Bit mask for I2C_PNACK */
#define _I2C_STATUS_PNACK_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PNACK_DEFAULT          (_I2C_STATUS_PNACK_DEFAULT << 3)   /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PCONT                  (0x1UL << 4)                       /**< Pending continue */
#define _I2C_STATUS_PCONT_SHIFT           4                                  /**< Shift value for I2C_PCONT */
#define _I2C_STATUS_PCONT_MASK            0x10UL                             /**< Bit mask for I2C_PCONT */
#define _I2C_STATUS_PCONT_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PCONT_DEFAULT          (_I2C_STATUS_PCONT_DEFAULT << 4)   /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PABORT                 (0x1UL << 5)                       /**< Pending abort */
#define _I2C_STATUS_PABORT_SHIFT          5                                  /**< Shift value for I2C_PABORT */
#define _I2C_STATUS_PABORT_MASK           0x20UL                             /**< Bit mask for I2C_PABORT */
#define _I2C_STATUS_PABORT_DEFAULT        0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PABORT_DEFAULT         (_I2C_STATUS_PABORT_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXC                    (0x1UL << 6)                       /**< TX Complete */
#define _I2C_STATUS_TXC_SHIFT             6                                  /**< Shift value for I2C_TXC */
#define _I2C_STATUS_TXC_MASK              0x40UL                             /**< Bit mask for I2C_TXC */
#define _I2C_STATUS_TXC_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXC_DEFAULT            (_I2C_STATUS_TXC_DEFAULT << 6)     /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXBL                   (0x1UL << 7)                       /**< TX Buffer Level */
#define _I2C_STATUS_TXBL_SHIFT            7                                  /**< Shift value for I2C_TXBL */
#define _I2C_STATUS_TXBL_MASK             0x80UL                             /**< Bit mask for I2C_TXBL */
#define _I2C_STATUS_TXBL_DEFAULT          0x00000001UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXBL_DEFAULT           (_I2C_STATUS_TXBL_DEFAULT << 7)    /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_RXDATAV                (0x1UL << 8)                       /**< RX Data Valid */
#define _I2C_STATUS_RXDATAV_SHIFT         8                                  /**< Shift value for I2C_RXDATAV */
#define _I2C_STATUS_RXDATAV_MASK          0x100UL                            /**< Bit mask for I2C_RXDATAV */
#define _I2C_STATUS_RXDATAV_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_RXDATAV_DEFAULT        (_I2C_STATUS_RXDATAV_DEFAULT << 8) /**< Shifted mode DEFAULT for I2C_STATUS */

/* Bit fields for I2C CLKDIV */
#define _I2C_CLKDIV_RESETVALUE            0x00000000UL                   /**< Default value for I2C_CLKDIV */
#define _I2C_CLKDIV_MASK                  0x000001FFUL                   /**< Mask for I2C_CLKDIV */
#define _I2C_CLKDIV_DIV_SHIFT             0                              /**< Shift value for I2C_DIV */
#define _I2C_CLKDIV_DIV_MASK              0x1FFUL                        /**< Bit mask for I2C_DIV */
#define _I2C_CLKDIV_DIV_DEFAULT           0x00000000UL                   /**< Mode DEFAULT for I2C_CLKDIV */
#define I2C_CLKDIV_DIV_DEFAULT            (_I2C_CLKDIV_DIV_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_CLKDIV */

/* Bit fields for I2C SADDR */
#define _I2C_SADDR_RESETVALUE             0x00000000UL                   /**< Default value for I2C_SADDR */
#define _I2C_SADDR_MASK                   0x000000FEUL                   /**< Mask for I2C_SADDR */
#define _I2C_SADDR_ADDR_SHIFT             1                              /**< Shift value for I2C_ADDR */
#define _I2C_SADDR_ADDR_MASK              0xFEUL                         /**< Bit mask for I2C_ADDR */
#define _I2C_SADDR_ADDR_DEFAULT           0x00000000UL                   /**< Mode DEFAULT for I2C_SADDR */
#define I2C_SADDR_ADDR_DEFAULT            (_I2C_SADDR_ADDR_DEFAULT << 1) /**< Shifted mode DEFAULT for I2C_SADDR */

/* Bit fields for I2C SADDRMASK */
#define _I2C_SADDRMASK_RESETVALUE         0x00000000UL                       /**< Default value for I2C_SADDRMASK */
#define _I2C_SADDRMASK_MASK               0x000000FEUL                       /**< Mask for I2C_SADDRMASK */
#define _I2C_SADDRMASK_MASK_SHIFT         1                                  /**< Shift value for I2C_MASK */
#define _I2C_SADDRMASK_MASK_MASK          0xFEUL                             /**< Bit mask for I2C_MASK */
#define _I2C_SADDRMASK_MASK_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for I2C_SADDRMASK */
#define I2C_SADDRMASK_MASK_DEFAULT        (_I2C_SADDRMASK_MASK_DEFAULT << 1) /**< Shifted mode DEFAULT for I2C_SADDRMASK */

/* Bit fields for I2C RXDATA */
#define _I2C_RXDATA_RESETVALUE            0x00000000UL                      /**< Default value for I2C_RXDATA */
#define _I2C_RXDATA_MASK                  0x000000FFUL                      /**< Mask for I2C_RXDATA */
#define _I2C_RXDATA_RXDATA_SHIFT          0                                 /**< Shift value for I2C_RXDATA */
#define _I2C_RXDATA_RXDATA_MASK           0xFFUL                            /**< Bit mask for I2C_RXDATA */
#define _I2C_RXDATA_RXDATA_DEFAULT        0x00000000UL                      /**< Mode DEFAULT for I2C_RXDATA */
#define I2C_RXDATA_RXDATA_DEFAULT         (_I2C_RXDATA_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_RXDATA */

/* Bit fields for I2C RXDATAP */
#define _I2C_RXDATAP_RESETVALUE           0x00000000UL                        /**< Default value for I2C_RXDATAP */
#define _I2C_RXDATAP_MASK                 0x000000FFUL                        /**< Mask for I2C_RXDATAP */
#define _I2C_RXDATAP_RXDATAP_SHIFT        0                                   /**< Shift value for I2C_RXDATAP */
#define _I2C_RXDATAP_RXDATAP_MASK         0xFFUL                              /**< Bit mask for I2C_RXDATAP */
#define _I2C_RXDATAP_RXDATAP_DEFAULT      0x00000000UL                        /**< Mode DEFAULT for I2C_RXDATAP */
#define I2C_RXDATAP_RXDATAP_DEFAULT       (_I2C_RXDATAP_RXDATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_RXDATAP */

/* Bit fields for I2C TXDATA */
#define _I2C_TXDATA_RESETVALUE            0x00000000UL                      /**< Default value for I2C_TXDATA */
#define _I2C_TXDATA_MASK                  0x000000FFUL                      /**< Mask for I2C_TXDATA */
#define _I2C_TXDATA_TXDATA_SHIFT          0                                 /**< Shift value for I2C_TXDATA */
#define _I2C_TXDATA_TXDATA_MASK           0xFFUL                            /**< Bit mask for I2C_TXDATA */
#define _I2C_TXDATA_TXDATA_DEFAULT        0x00000000UL                      /**< Mode DEFAULT for I2C_TXDATA */
#define I2C_TXDATA_TXDATA_DEFAULT         (_I2C_TXDATA_TXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_TXDATA */

/* Bit fields for I2C IF */
#define _I2C_IF_RESETVALUE                0x00000010UL                    /**< Default value for I2C_IF */
#define _I2C_IF_MASK                      0x0001FFFFUL                    /**< Mask for I2C_IF */
#define I2C_IF_START                      (0x1UL << 0)                    /**< START condition Interrupt Flag */
#define _I2C_IF_START_SHIFT               0                               /**< Shift value for I2C_START */
#define _I2C_IF_START_MASK                0x1UL                           /**< Bit mask for I2C_START */
#define _I2C_IF_START_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_START_DEFAULT              (_I2C_IF_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_RSTART                     (0x1UL << 1)                    /**< Repeated START condition Interrupt Flag */
#define _I2C_IF_RSTART_SHIFT              1                               /**< Shift value for I2C_RSTART */
#define _I2C_IF_RSTART_MASK               0x2UL                           /**< Bit mask for I2C_RSTART */
#define _I2C_IF_RSTART_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_RSTART_DEFAULT             (_I2C_IF_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_ADDR                       (0x1UL << 2)                    /**< Address Interrupt Flag */
#define _I2C_IF_ADDR_SHIFT                2                               /**< Shift value for I2C_ADDR */
#define _I2C_IF_ADDR_MASK                 0x4UL                           /**< Bit mask for I2C_ADDR */
#define _I2C_IF_ADDR_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_ADDR_DEFAULT               (_I2C_IF_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_TXC                        (0x1UL << 3)                    /**< Transfer Completed Interrupt Flag */
#define _I2C_IF_TXC_SHIFT                 3                               /**< Shift value for I2C_TXC */
#define _I2C_IF_TXC_MASK                  0x8UL                           /**< Bit mask for I2C_TXC */
#define _I2C_IF_TXC_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_TXC_DEFAULT                (_I2C_IF_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_TXBL                       (0x1UL << 4)                    /**< Transmit Buffer Level Interrupt Flag */
#define _I2C_IF_TXBL_SHIFT                4                               /**< Shift value for I2C_TXBL */
#define _I2C_IF_TXBL_MASK                 0x10UL                          /**< Bit mask for I2C_TXBL */
#define _I2C_IF_TXBL_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_TXBL_DEFAULT               (_I2C_IF_TXBL_DEFAULT << 4)     /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_RXDATAV                    (0x1UL << 5)                    /**< Receive Data Valid Interrupt Flag */
#define _I2C_IF_RXDATAV_SHIFT             5                               /**< Shift value for I2C_RXDATAV */
#define _I2C_IF_RXDATAV_MASK              0x20UL                          /**< Bit mask for I2C_RXDATAV */
#define _I2C_IF_RXDATAV_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_RXDATAV_DEFAULT            (_I2C_IF_RXDATAV_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_ACK                        (0x1UL << 6)                    /**< Acknowledge Received Interrupt Flag */
#define _I2C_IF_ACK_SHIFT                 6                               /**< Shift value for I2C_ACK */
#define _I2C_IF_ACK_MASK                  0x40UL                          /**< Bit mask for I2C_ACK */
#define _I2C_IF_ACK_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_ACK_DEFAULT                (_I2C_IF_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_NACK                       (0x1UL << 7)                    /**< Not Acknowledge Received Interrupt Flag */
#define _I2C_IF_NACK_SHIFT                7                               /**< Shift value for I2C_NACK */
#define _I2C_IF_NACK_MASK                 0x80UL                          /**< Bit mask for I2C_NACK */
#define _I2C_IF_NACK_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_NACK_DEFAULT               (_I2C_IF_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_MSTOP                      (0x1UL << 8)                    /**< Master STOP Condition Interrupt Flag */
#define _I2C_IF_MSTOP_SHIFT               8                               /**< Shift value for I2C_MSTOP */
#define _I2C_IF_MSTOP_MASK                0x100UL                         /**< Bit mask for I2C_MSTOP */
#define _I2C_IF_MSTOP_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_MSTOP_DEFAULT              (_I2C_IF_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_ARBLOST                    (0x1UL << 9)                    /**< Arbitration Lost Interrupt Flag */
#define _I2C_IF_ARBLOST_SHIFT             9                               /**< Shift value for I2C_ARBLOST */
#define _I2C_IF_ARBLOST_MASK              0x200UL                         /**< Bit mask for I2C_ARBLOST */
#define _I2C_IF_ARBLOST_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_ARBLOST_DEFAULT            (_I2C_IF_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_BUSERR                     (0x1UL << 10)                   /**< Bus Error Interrupt Flag */
#define _I2C_IF_BUSERR_SHIFT              10                              /**< Shift value for I2C_BUSERR */
#define _I2C_IF_BUSERR_MASK               0x400UL                         /**< Bit mask for I2C_BUSERR */
#define _I2C_IF_BUSERR_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_BUSERR_DEFAULT             (_I2C_IF_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_BUSHOLD                    (0x1UL << 11)                   /**< Bus Held Interrupt Flag */
#define _I2C_IF_BUSHOLD_SHIFT             11                              /**< Shift value for I2C_BUSHOLD */
#define _I2C_IF_BUSHOLD_MASK              0x800UL                         /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IF_BUSHOLD_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_BUSHOLD_DEFAULT            (_I2C_IF_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_TXOF                       (0x1UL << 12)                   /**< Transmit Buffer Overflow Interrupt Flag */
#define _I2C_IF_TXOF_SHIFT                12                              /**< Shift value for I2C_TXOF */
#define _I2C_IF_TXOF_MASK                 0x1000UL                        /**< Bit mask for I2C_TXOF */
#define _I2C_IF_TXOF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_TXOF_DEFAULT               (_I2C_IF_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_RXUF                       (0x1UL << 13)                   /**< Receive Buffer Underflow Interrupt Flag */
#define _I2C_IF_RXUF_SHIFT                13                              /**< Shift value for I2C_RXUF */
#define _I2C_IF_RXUF_MASK                 0x2000UL                        /**< Bit mask for I2C_RXUF */
#define _I2C_IF_RXUF_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_RXUF_DEFAULT               (_I2C_IF_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_BITO                       (0x1UL << 14)                   /**< Bus Idle Timeout Interrupt Flag */
#define _I2C_IF_BITO_SHIFT                14                              /**< Shift value for I2C_BITO */
#define _I2C_IF_BITO_MASK                 0x4000UL                        /**< Bit mask for I2C_BITO */
#define _I2C_IF_BITO_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_BITO_DEFAULT               (_I2C_IF_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_CLTO                       (0x1UL << 15)                   /**< Clock Low Timeout Interrupt Flag */
#define _I2C_IF_CLTO_SHIFT                15                              /**< Shift value for I2C_CLTO */
#define _I2C_IF_CLTO_MASK                 0x8000UL                        /**< Bit mask for I2C_CLTO */
#define _I2C_IF_CLTO_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_CLTO_DEFAULT               (_I2C_IF_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_SSTOP                      (0x1UL << 16)                   /**< Slave STOP condition Interrupt Flag */
#define _I2C_IF_SSTOP_SHIFT               16                              /**< Shift value for I2C_SSTOP */
#define _I2C_IF_SSTOP_MASK                0x10000UL                       /**< Bit mask for I2C_SSTOP */
#define _I2C_IF_SSTOP_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_SSTOP_DEFAULT              (_I2C_IF_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IF */

/* Bit fields for I2C IFS */
#define _I2C_IFS_RESETVALUE               0x00000000UL                     /**< Default value for I2C_IFS */
#define _I2C_IFS_MASK                     0x0001FFCFUL                     /**< Mask for I2C_IFS */
#define I2C_IFS_START                     (0x1UL << 0)                     /**< Set START Interrupt Flag */
#define _I2C_IFS_START_SHIFT              0                                /**< Shift value for I2C_START */
#define _I2C_IFS_START_MASK               0x1UL                            /**< Bit mask for I2C_START */
#define _I2C_IFS_START_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_START_DEFAULT             (_I2C_IFS_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_RSTART                    (0x1UL << 1)                     /**< Set Repeated START Interrupt Flag */
#define _I2C_IFS_RSTART_SHIFT             1                                /**< Shift value for I2C_RSTART */
#define _I2C_IFS_RSTART_MASK              0x2UL                            /**< Bit mask for I2C_RSTART */
#define _I2C_IFS_RSTART_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_RSTART_DEFAULT            (_I2C_IFS_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_ADDR                      (0x1UL << 2)                     /**< Set Address Interrupt Flag */
#define _I2C_IFS_ADDR_SHIFT               2                                /**< Shift value for I2C_ADDR */
#define _I2C_IFS_ADDR_MASK                0x4UL                            /**< Bit mask for I2C_ADDR */
#define _I2C_IFS_ADDR_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_ADDR_DEFAULT              (_I2C_IFS_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXC                       (0x1UL << 3)                     /**< Set Transfer Completed Interrupt Flag */
#define _I2C_IFS_TXC_SHIFT                3                                /**< Shift value for I2C_TXC */
#define _I2C_IFS_TXC_MASK                 0x8UL                            /**< Bit mask for I2C_TXC */
#define _I2C_IFS_TXC_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXC_DEFAULT               (_I2C_IFS_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_ACK                       (0x1UL << 6)                     /**< Set Acknowledge Received Interrupt Flag */
#define _I2C_IFS_ACK_SHIFT                6                                /**< Shift value for I2C_ACK */
#define _I2C_IFS_ACK_MASK                 0x40UL                           /**< Bit mask for I2C_ACK */
#define _I2C_IFS_ACK_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_ACK_DEFAULT               (_I2C_IFS_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_NACK                      (0x1UL << 7)                     /**< Set Not Acknowledge Received Interrupt Flag */
#define _I2C_IFS_NACK_SHIFT               7                                /**< Shift value for I2C_NACK */
#define _I2C_IFS_NACK_MASK                0x80UL                           /**< Bit mask for I2C_NACK */
#define _I2C_IFS_NACK_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_NACK_DEFAULT              (_I2C_IFS_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_MSTOP                     (0x1UL << 8)                     /**< Set MSTOP Interrupt Flag */
#define _I2C_IFS_MSTOP_SHIFT              8                                /**< Shift value for I2C_MSTOP */
#define _I2C_IFS_MSTOP_MASK               0x100UL                          /**< Bit mask for I2C_MSTOP */
#define _I2C_IFS_MSTOP_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_MSTOP_DEFAULT             (_I2C_IFS_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_ARBLOST                   (0x1UL << 9)                     /**< Set Arbitration Lost Interrupt Flag */
#define _I2C_IFS_ARBLOST_SHIFT            9                                /**< Shift value for I2C_ARBLOST */
#define _I2C_IFS_ARBLOST_MASK             0x200UL                          /**< Bit mask for I2C_ARBLOST */
#define _I2C_IFS_ARBLOST_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_ARBLOST_DEFAULT           (_I2C_IFS_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSERR                    (0x1UL << 10)                    /**< Set Bus Error Interrupt Flag */
#define _I2C_IFS_BUSERR_SHIFT             10                               /**< Shift value for I2C_BUSERR */
#define _I2C_IFS_BUSERR_MASK              0x400UL                          /**< Bit mask for I2C_BUSERR */
#define _I2C_IFS_BUSERR_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSERR_DEFAULT            (_I2C_IFS_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSHOLD                   (0x1UL << 11)                    /**< Set Bus Held Interrupt Flag */
#define _I2C_IFS_BUSHOLD_SHIFT            11                               /**< Shift value for I2C_BUSHOLD */
#define _I2C_IFS_BUSHOLD_MASK             0x800UL                          /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IFS_BUSHOLD_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSHOLD_DEFAULT           (_I2C_IFS_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXOF                      (0x1UL << 12)                    /**< Set Transmit Buffer Overflow Interrupt Flag */
#define _I2C_IFS_TXOF_SHIFT               12                               /**< Shift value for I2C_TXOF */
#define _I2C_IFS_TXOF_MASK                0x1000UL                         /**< Bit mask for I2C_TXOF */
#define _I2C_IFS_TXOF_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXOF_DEFAULT              (_I2C_IFS_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_RXUF                      (0x1UL << 13)                    /**< Set Receive Buffer Underflow Interrupt Flag */
#define _I2C_IFS_RXUF_SHIFT               13                               /**< Shift value for I2C_RXUF */
#define _I2C_IFS_RXUF_MASK                0x2000UL                         /**< Bit mask for I2C_RXUF */
#define _I2C_IFS_RXUF_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_RXUF_DEFAULT              (_I2C_IFS_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_BITO                      (0x1UL << 14)                    /**< Set Bus Idle Timeout Interrupt Flag */
#define _I2C_IFS_BITO_SHIFT               14                               /**< Shift value for I2C_BITO */
#define _I2C_IFS_BITO_MASK                0x4000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_IFS_BITO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_BITO_DEFAULT              (_I2C_IFS_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_CLTO                      (0x1UL << 15)                    /**< Set Clock Low Interrupt Flag */
#define _I2C_IFS_CLTO_SHIFT               15                               /**< Shift value for I2C_CLTO */
#define _I2C_IFS_CLTO_MASK                0x8000UL                         /**< Bit mask for I2C_CLTO */
#define _I2C_IFS_CLTO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_CLTO_DEFAULT              (_I2C_IFS_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_SSTOP                     (0x1UL << 16)                    /**< Set SSTOP Interrupt Flag */
#define _I2C_IFS_SSTOP_SHIFT              16                               /**< Shift value for I2C_SSTOP */
#define _I2C_IFS_SSTOP_MASK               0x10000UL                        /**< Bit mask for I2C_SSTOP */
#define _I2C_IFS_SSTOP_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_SSTOP_DEFAULT             (_I2C_IFS_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IFS */

/* Bit fields for I2C IFC */
#define _I2C_IFC_RESETVALUE               0x00000000UL                     /**< Default value for I2C_IFC */
#define _I2C_IFC_MASK                     0x0001FFCFUL                     /**< Mask for I2C_IFC */
#define I2C_IFC_START                     (0x1UL << 0)                     /**< Clear START Interrupt Flag */
#define _I2C_IFC_START_SHIFT              0                                /**< Shift value for I2C_START */
#define _I2C_IFC_START_MASK               0x1UL                            /**< Bit mask for I2C_START */
#define _I2C_IFC_START_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_START_DEFAULT             (_I2C_IFC_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_RSTART                    (0x1UL << 1)                     /**< Clear Repeated START Interrupt Flag */
#define _I2C_IFC_RSTART_SHIFT             1                                /**< Shift value for I2C_RSTART */
#define _I2C_IFC_RSTART_MASK              0x2UL                            /**< Bit mask for I2C_RSTART */
#define _I2C_IFC_RSTART_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_RSTART_DEFAULT            (_I2C_IFC_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_ADDR                      (0x1UL << 2)                     /**< Clear Address Interrupt Flag */
#define _I2C_IFC_ADDR_SHIFT               2                                /**< Shift value for I2C_ADDR */
#define _I2C_IFC_ADDR_MASK                0x4UL                            /**< Bit mask for I2C_ADDR */
#define _I2C_IFC_ADDR_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_ADDR_DEFAULT              (_I2C_IFC_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXC                       (0x1UL << 3)                     /**< Clear Transfer Completed Interrupt Flag */
#define _I2C_IFC_TXC_SHIFT                3                                /**< Shift value for I2C_TXC */
#define _I2C_IFC_TXC_MASK                 0x8UL                            /**< Bit mask for I2C_TXC */
#define _I2C_IFC_TXC_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXC_DEFAULT               (_I2C_IFC_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_ACK                       (0x1UL << 6)                     /**< Clear Acknowledge Received Interrupt Flag */
#define _I2C_IFC_ACK_SHIFT                6                                /**< Shift value for I2C_ACK */
#define _I2C_IFC_ACK_MASK                 0x40UL                           /**< Bit mask for I2C_ACK */
#define _I2C_IFC_ACK_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_ACK_DEFAULT               (_I2C_IFC_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_NACK                      (0x1UL << 7)                     /**< Clear Not Acknowledge Received Interrupt Flag */
#define _I2C_IFC_NACK_SHIFT               7                                /**< Shift value for I2C_NACK */
#define _I2C_IFC_NACK_MASK                0x80UL                           /**< Bit mask for I2C_NACK */
#define _I2C_IFC_NACK_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_NACK_DEFAULT              (_I2C_IFC_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_MSTOP                     (0x1UL << 8)                     /**< Clear MSTOP Interrupt Flag */
#define _I2C_IFC_MSTOP_SHIFT              8                                /**< Shift value for I2C_MSTOP */
#define _I2C_IFC_MSTOP_MASK               0x100UL                          /**< Bit mask for I2C_MSTOP */
#define _I2C_IFC_MSTOP_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_MSTOP_DEFAULT             (_I2C_IFC_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_ARBLOST                   (0x1UL << 9)                     /**< Clear Arbitration Lost Interrupt Flag */
#define _I2C_IFC_ARBLOST_SHIFT            9                                /**< Shift value for I2C_ARBLOST */
#define _I2C_IFC_ARBLOST_MASK             0x200UL                          /**< Bit mask for I2C_ARBLOST */
#define _I2C_IFC_ARBLOST_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_ARBLOST_DEFAULT           (_I2C_IFC_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSERR                    (0x1UL << 10)                    /**< Clear Bus Error Interrupt Flag */
#define _I2C_IFC_BUSERR_SHIFT             10                               /**< Shift value for I2C_BUSERR */
#define _I2C_IFC_BUSERR_MASK              0x400UL                          /**< Bit mask for I2C_BUSERR */
#define _I2C_IFC_BUSERR_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSERR_DEFAULT            (_I2C_IFC_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSHOLD                   (0x1UL << 11)                    /**< Clear Bus Held Interrupt Flag */
#define _I2C_IFC_BUSHOLD_SHIFT            11                               /**< Shift value for I2C_BUSHOLD */
#define _I2C_IFC_BUSHOLD_MASK             0x800UL                          /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IFC_BUSHOLD_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSHOLD_DEFAULT           (_I2C_IFC_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXOF                      (0x1UL << 12)                    /**< Clear Transmit Buffer Overflow Interrupt Flag */
#define _I2C_IFC_TXOF_SHIFT               12                               /**< Shift value for I2C_TXOF */
#define _I2C_IFC_TXOF_MASK                0x1000UL                         /**< Bit mask for I2C_TXOF */
#define _I2C_IFC_TXOF_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXOF_DEFAULT              (_I2C_IFC_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_RXUF                      (0x1UL << 13)                    /**< Clear Receive Buffer Underflow Interrupt Flag */
#define _I2C_IFC_RXUF_SHIFT               13                               /**< Shift value for I2C_RXUF */
#define _I2C_IFC_RXUF_MASK                0x2000UL                         /**< Bit mask for I2C_RXUF */
#define _I2C_IFC_RXUF_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_RXUF_DEFAULT              (_I2C_IFC_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_BITO                      (0x1UL << 14)                    /**< Clear Bus Idle Timeout Interrupt Flag */
#define _I2C_IFC_BITO_SHIFT               14                               /**< Shift value for I2C_BITO */
#define _I2C_IFC_BITO_MASK                0x4000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_IFC_BITO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_BITO_DEFAULT              (_I2C_IFC_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_CLTO                      (0x1UL << 15)                    /**< Clear Clock Low Interrupt Flag */
#define _I2C_IFC_CLTO_SHIFT               15                               /**< Shift value for I2C_CLTO */
#define _I2C_IFC_CLTO_MASK                0x8000UL                         /**< Bit mask for I2C_CLTO */
#define _I2C_IFC_CLTO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_CLTO_DEFAULT              (_I2C_IFC_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_SSTOP                     (0x1UL << 16)                    /**< Clear SSTOP Interrupt Flag */
#define _I2C_IFC_SSTOP_SHIFT              16                               /**< Shift value for I2C_SSTOP */
#define _I2C_IFC_SSTOP_MASK               0x10000UL                        /**< Bit mask for I2C_SSTOP */
#define _I2C_IFC_SSTOP_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_SSTOP_DEFAULT             (_I2C_IFC_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IFC */

/* Bit fields for I2C IEN */
#define _I2C_IEN_RESETVALUE               0x00000000UL                     /**< Default value for I2C_IEN */
#define _I2C_IEN_MASK                     0x0001FFFFUL                     /**< Mask for I2C_IEN */
#define I2C_IEN_START                     (0x1UL << 0)                     /**< START Condition Interrupt Enable */
#define _I2C_IEN_START_SHIFT              0                                /**< Shift value for I2C_START */
#define _I2C_IEN_START_MASK               0x1UL                            /**< Bit mask for I2C_START */
#define _I2C_IEN_START_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_START_DEFAULT             (_I2C_IEN_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_RSTART                    (0x1UL << 1)                     /**< Repeated START condition Interrupt Enable */
#define _I2C_IEN_RSTART_SHIFT             1                                /**< Shift value for I2C_RSTART */
#define _I2C_IEN_RSTART_MASK              0x2UL                            /**< Bit mask for I2C_RSTART */
#define _I2C_IEN_RSTART_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_RSTART_DEFAULT            (_I2C_IEN_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_ADDR                      (0x1UL << 2)                     /**< Address Interrupt Enable */
#define _I2C_IEN_ADDR_SHIFT               2                                /**< Shift value for I2C_ADDR */
#define _I2C_IEN_ADDR_MASK                0x4UL                            /**< Bit mask for I2C_ADDR */
#define _I2C_IEN_ADDR_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_ADDR_DEFAULT              (_I2C_IEN_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXC                       (0x1UL << 3)                     /**< Transfer Completed Interrupt Enable */
#define _I2C_IEN_TXC_SHIFT                3                                /**< Shift value for I2C_TXC */
#define _I2C_IEN_TXC_MASK                 0x8UL                            /**< Bit mask for I2C_TXC */
#define _I2C_IEN_TXC_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXC_DEFAULT               (_I2C_IEN_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXBL                      (0x1UL << 4)                     /**< Transmit Buffer level Interrupt Enable */
#define _I2C_IEN_TXBL_SHIFT               4                                /**< Shift value for I2C_TXBL */
#define _I2C_IEN_TXBL_MASK                0x10UL                           /**< Bit mask for I2C_TXBL */
#define _I2C_IEN_TXBL_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXBL_DEFAULT              (_I2C_IEN_TXBL_DEFAULT << 4)     /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXDATAV                   (0x1UL << 5)                     /**< Receive Data Valid Interrupt Enable */
#define _I2C_IEN_RXDATAV_SHIFT            5                                /**< Shift value for I2C_RXDATAV */
#define _I2C_IEN_RXDATAV_MASK             0x20UL                           /**< Bit mask for I2C_RXDATAV */
#define _I2C_IEN_RXDATAV_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXDATAV_DEFAULT           (_I2C_IEN_RXDATAV_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_ACK                       (0x1UL << 6)                     /**< Acknowledge Received Interrupt Enable */
#define _I2C_IEN_ACK_SHIFT                6                                /**< Shift value for I2C_ACK */
#define _I2C_IEN_ACK_MASK                 0x40UL                           /**< Bit mask for I2C_ACK */
#define _I2C_IEN_ACK_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_ACK_DEFAULT               (_I2C_IEN_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_NACK                      (0x1UL << 7)                     /**< Not Acknowledge Received Interrupt Enable */
#define _I2C_IEN_NACK_SHIFT               7                                /**< Shift value for I2C_NACK */
#define _I2C_IEN_NACK_MASK                0x80UL                           /**< Bit mask for I2C_NACK */
#define _I2C_IEN_NACK_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_NACK_DEFAULT              (_I2C_IEN_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_MSTOP                     (0x1UL << 8)                     /**< MSTOP Interrupt Enable */
#define _I2C_IEN_MSTOP_SHIFT              8                                /**< Shift value for I2C_MSTOP */
#define _I2C_IEN_MSTOP_MASK               0x100UL                          /**< Bit mask for I2C_MSTOP */
#define _I2C_IEN_MSTOP_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_MSTOP_DEFAULT             (_I2C_IEN_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_ARBLOST                   (0x1UL << 9)                     /**< Arbitration Lost Interrupt Enable */
#define _I2C_IEN_ARBLOST_SHIFT            9                                /**< Shift value for I2C_ARBLOST */
#define _I2C_IEN_ARBLOST_MASK             0x200UL                          /**< Bit mask for I2C_ARBLOST */
#define _I2C_IEN_ARBLOST_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_ARBLOST_DEFAULT           (_I2C_IEN_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSERR                    (0x1UL << 10)                    /**< Bus Error Interrupt Enable */
#define _I2C_IEN_BUSERR_SHIFT             10                               /**< Shift value for I2C_BUSERR */
#define _I2C_IEN_BUSERR_MASK              0x400UL                          /**< Bit mask for I2C_BUSERR */
#define _I2C_IEN_BUSERR_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSERR_DEFAULT            (_I2C_IEN_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSHOLD                   (0x1UL << 11)                    /**< Bus Held Interrupt Enable */
#define _I2C_IEN_BUSHOLD_SHIFT            11                               /**< Shift value for I2C_BUSHOLD */
#define _I2C_IEN_BUSHOLD_MASK             0x800UL                          /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IEN_BUSHOLD_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSHOLD_DEFAULT           (_I2C_IEN_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXOF                      (0x1UL << 12)                    /**< Transmit Buffer Overflow Interrupt Enable */
#define _I2C_IEN_TXOF_SHIFT               12                               /**< Shift value for I2C_TXOF */
#define _I2C_IEN_TXOF_MASK                0x1000UL                         /**< Bit mask for I2C_TXOF */
#define _I2C_IEN_TXOF_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXOF_DEFAULT              (_I2C_IEN_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXUF                      (0x1UL << 13)                    /**< Receive Buffer Underflow Interrupt Enable */
#define _I2C_IEN_RXUF_SHIFT               13                               /**< Shift value for I2C_RXUF */
#define _I2C_IEN_RXUF_MASK                0x2000UL                         /**< Bit mask for I2C_RXUF */
#define _I2C_IEN_RXUF_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXUF_DEFAULT              (_I2C_IEN_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_BITO                      (0x1UL << 14)                    /**< Bus Idle Timeout Interrupt Enable */
#define _I2C_IEN_BITO_SHIFT               14                               /**< Shift value for I2C_BITO */
#define _I2C_IEN_BITO_MASK                0x4000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_IEN_BITO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_BITO_DEFAULT              (_I2C_IEN_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_CLTO                      (0x1UL << 15)                    /**< Clock Low Interrupt Enable */
#define _I2C_IEN_CLTO_SHIFT               15                               /**< Shift value for I2C_CLTO */
#define _I2C_IEN_CLTO_MASK                0x8000UL                         /**< Bit mask for I2C_CLTO */
#define _I2C_IEN_CLTO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_CLTO_DEFAULT              (_I2C_IEN_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_SSTOP                     (0x1UL << 16)                    /**< SSTOP Interrupt Enable */
#define _I2C_IEN_SSTOP_SHIFT              16                               /**< Shift value for I2C_SSTOP */
#define _I2C_IEN_SSTOP_MASK               0x10000UL                        /**< Bit mask for I2C_SSTOP */
#define _I2C_IEN_SSTOP_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_SSTOP_DEFAULT             (_I2C_IEN_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IEN */

/* Bit fields for I2C ROUTE */
#define _I2C_ROUTE_RESETVALUE             0x00000000UL                       /**< Default value for I2C_ROUTE */
#define _I2C_ROUTE_MASK                   0x00000703UL                       /**< Mask for I2C_ROUTE */
#define I2C_ROUTE_SDAPEN                  (0x1UL << 0)                       /**< SDA Pin Enable */
#define _I2C_ROUTE_SDAPEN_SHIFT           0                                  /**< Shift value for I2C_SDAPEN */
#define _I2C_ROUTE_SDAPEN_MASK            0x1UL                              /**< Bit mask for I2C_SDAPEN */
#define _I2C_ROUTE_SDAPEN_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_ROUTE */
#define I2C_ROUTE_SDAPEN_DEFAULT          (_I2C_ROUTE_SDAPEN_DEFAULT << 0)   /**< Shifted mode DEFAULT for I2C_ROUTE */
#define I2C_ROUTE_SCLPEN                  (0x1UL << 1)                       /**< SCL Pin Enable */
#define _I2C_ROUTE_SCLPEN_SHIFT           1                                  /**< Shift value for I2C_SCLPEN */
#define _I2C_ROUTE_SCLPEN_MASK            0x2UL                              /**< Bit mask for I2C_SCLPEN */
#define _I2C_ROUTE_SCLPEN_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_ROUTE */
#define I2C_ROUTE_SCLPEN_DEFAULT          (_I2C_ROUTE_SCLPEN_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_SHIFT         8                                  /**< Shift value for I2C_LOCATION */
#define _I2C_ROUTE_LOCATION_MASK          0x700UL                            /**< Bit mask for I2C_LOCATION */
#define _I2C_ROUTE_LOCATION_LOC0          0x00000000UL                       /**< Mode LOC0 for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_LOC1          0x00000001UL                       /**< Mode LOC1 for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_LOC2          0x00000002UL                       /**< Mode LOC2 for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_LOC3          0x00000003UL                       /**< Mode LOC3 for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_LOC4          0x00000004UL                       /**< Mode LOC4 for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_LOC5          0x00000005UL                       /**< Mode LOC5 for I2C_ROUTE */
#define _I2C_ROUTE_LOCATION_LOC6          0x00000006UL                       /**< Mode LOC6 for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_LOC0           (_I2C_ROUTE_LOCATION_LOC0 << 8)    /**< Shifted mode LOC0 for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_DEFAULT        (_I2C_ROUTE_LOCATION_DEFAULT << 8) /**< Shifted mode DEFAULT for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_LOC1           (_I2C_ROUTE_LOCATION_LOC1 << 8)    /**< Shifted mode LOC1 for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_LOC2           (_I2C_ROUTE_LOCATION_LOC2 << 8)    /**< Shifted mode LOC2 for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_LOC3           (_I2C_ROUTE_LOCATION_LOC3 << 8)    /**< Shifted mode LOC3 for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_LOC4           (_I2C_ROUTE_LOCATION_LOC4 << 8)    /**< Shifted mode LOC4 for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_LOC5           (_I2C_ROUTE_LOCATION_LOC5 << 8)    /**< Shifted mode LOC5 for I2C_ROUTE */
#define I2C_ROUTE_LOCATION_LOC6           (_I2C_ROUTE_LOCATION_LOC6 << 8)    /**< Shifted mode LOC6 for I2C_ROUTE */

/** @} End of group EFM32HG_I2C */
/** @} End of group Parts */

