/**************************************************************************//**
 * @file efr32fg1p_i2c.h
 * @brief EFR32FG1P_I2C register and bit field definitions
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
 * @defgroup EFR32FG1P_I2C
 * @{
 * @brief EFR32FG1P_I2C Register Declaration
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
  __IM uint32_t  RXDOUBLE;  /**< Receive Buffer Double Data Register  */
  __IM uint32_t  RXDATAP;   /**< Receive Buffer Data Peek Register  */
  __IM uint32_t  RXDOUBLEP; /**< Receive Buffer Double Data Peek Register  */
  __IOM uint32_t TXDATA;    /**< Transmit Buffer Data Register  */
  __IOM uint32_t TXDOUBLE;  /**< Transmit Buffer Double Data Register  */
  __IM uint32_t  IF;        /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;       /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;       /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;       /**< Interrupt Enable Register  */
  __IOM uint32_t ROUTEPEN;  /**< I/O Routing Pin Enable Register  */
  __IOM uint32_t ROUTELOC0; /**< I/O Routing Location Register  */
} I2C_TypeDef;              /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_I2C_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for I2C CTRL */
#define _I2C_CTRL_RESETVALUE               0x00000000UL                     /**< Default value for I2C_CTRL */
#define _I2C_CTRL_MASK                     0x0007B3FFUL                     /**< Mask for I2C_CTRL */
#define I2C_CTRL_EN                        (0x1UL << 0)                     /**< I2C Enable */
#define _I2C_CTRL_EN_SHIFT                 0                                /**< Shift value for I2C_EN */
#define _I2C_CTRL_EN_MASK                  0x1UL                            /**< Bit mask for I2C_EN */
#define _I2C_CTRL_EN_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_EN_DEFAULT                (_I2C_CTRL_EN_DEFAULT << 0)      /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_SLAVE                     (0x1UL << 1)                     /**< Addressable as Slave */
#define _I2C_CTRL_SLAVE_SHIFT              1                                /**< Shift value for I2C_SLAVE */
#define _I2C_CTRL_SLAVE_MASK               0x2UL                            /**< Bit mask for I2C_SLAVE */
#define _I2C_CTRL_SLAVE_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_SLAVE_DEFAULT             (_I2C_CTRL_SLAVE_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOACK                   (0x1UL << 2)                     /**< Automatic Acknowledge */
#define _I2C_CTRL_AUTOACK_SHIFT            2                                /**< Shift value for I2C_AUTOACK */
#define _I2C_CTRL_AUTOACK_MASK             0x4UL                            /**< Bit mask for I2C_AUTOACK */
#define _I2C_CTRL_AUTOACK_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOACK_DEFAULT           (_I2C_CTRL_AUTOACK_DEFAULT << 2) /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSE                    (0x1UL << 3)                     /**< Automatic STOP when Empty */
#define _I2C_CTRL_AUTOSE_SHIFT             3                                /**< Shift value for I2C_AUTOSE */
#define _I2C_CTRL_AUTOSE_MASK              0x8UL                            /**< Bit mask for I2C_AUTOSE */
#define _I2C_CTRL_AUTOSE_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSE_DEFAULT            (_I2C_CTRL_AUTOSE_DEFAULT << 3)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSN                    (0x1UL << 4)                     /**< Automatic STOP on NACK */
#define _I2C_CTRL_AUTOSN_SHIFT             4                                /**< Shift value for I2C_AUTOSN */
#define _I2C_CTRL_AUTOSN_MASK              0x10UL                           /**< Bit mask for I2C_AUTOSN */
#define _I2C_CTRL_AUTOSN_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_AUTOSN_DEFAULT            (_I2C_CTRL_AUTOSN_DEFAULT << 4)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_ARBDIS                    (0x1UL << 5)                     /**< Arbitration Disable */
#define _I2C_CTRL_ARBDIS_SHIFT             5                                /**< Shift value for I2C_ARBDIS */
#define _I2C_CTRL_ARBDIS_MASK              0x20UL                           /**< Bit mask for I2C_ARBDIS */
#define _I2C_CTRL_ARBDIS_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_ARBDIS_DEFAULT            (_I2C_CTRL_ARBDIS_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_GCAMEN                    (0x1UL << 6)                     /**< General Call Address Match Enable */
#define _I2C_CTRL_GCAMEN_SHIFT             6                                /**< Shift value for I2C_GCAMEN */
#define _I2C_CTRL_GCAMEN_MASK              0x40UL                           /**< Bit mask for I2C_GCAMEN */
#define _I2C_CTRL_GCAMEN_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_GCAMEN_DEFAULT            (_I2C_CTRL_GCAMEN_DEFAULT << 6)  /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_TXBIL                     (0x1UL << 7)                     /**< TX Buffer Interrupt Level */
#define _I2C_CTRL_TXBIL_SHIFT              7                                /**< Shift value for I2C_TXBIL */
#define _I2C_CTRL_TXBIL_MASK               0x80UL                           /**< Bit mask for I2C_TXBIL */
#define _I2C_CTRL_TXBIL_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_TXBIL_EMPTY              0x00000000UL                     /**< Mode EMPTY for I2C_CTRL */
#define _I2C_CTRL_TXBIL_HALFFULL           0x00000001UL                     /**< Mode HALFFULL for I2C_CTRL */
#define I2C_CTRL_TXBIL_DEFAULT             (_I2C_CTRL_TXBIL_DEFAULT << 7)   /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_TXBIL_EMPTY               (_I2C_CTRL_TXBIL_EMPTY << 7)     /**< Shifted mode EMPTY for I2C_CTRL */
#define I2C_CTRL_TXBIL_HALFFULL            (_I2C_CTRL_TXBIL_HALFFULL << 7)  /**< Shifted mode HALFFULL for I2C_CTRL */
#define _I2C_CTRL_CLHR_SHIFT               8                                /**< Shift value for I2C_CLHR */
#define _I2C_CTRL_CLHR_MASK                0x300UL                          /**< Bit mask for I2C_CLHR */
#define _I2C_CTRL_CLHR_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_CLHR_STANDARD            0x00000000UL                     /**< Mode STANDARD for I2C_CTRL */
#define _I2C_CTRL_CLHR_ASYMMETRIC          0x00000001UL                     /**< Mode ASYMMETRIC for I2C_CTRL */
#define _I2C_CTRL_CLHR_FAST                0x00000002UL                     /**< Mode FAST for I2C_CTRL */
#define I2C_CTRL_CLHR_DEFAULT              (_I2C_CTRL_CLHR_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_CLHR_STANDARD             (_I2C_CTRL_CLHR_STANDARD << 8)   /**< Shifted mode STANDARD for I2C_CTRL */
#define I2C_CTRL_CLHR_ASYMMETRIC           (_I2C_CTRL_CLHR_ASYMMETRIC << 8) /**< Shifted mode ASYMMETRIC for I2C_CTRL */
#define I2C_CTRL_CLHR_FAST                 (_I2C_CTRL_CLHR_FAST << 8)       /**< Shifted mode FAST for I2C_CTRL */
#define _I2C_CTRL_BITO_SHIFT               12                               /**< Shift value for I2C_BITO */
#define _I2C_CTRL_BITO_MASK                0x3000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_CTRL_BITO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_BITO_OFF                 0x00000000UL                     /**< Mode OFF for I2C_CTRL */
#define _I2C_CTRL_BITO_40PCC               0x00000001UL                     /**< Mode 40PCC for I2C_CTRL */
#define _I2C_CTRL_BITO_80PCC               0x00000002UL                     /**< Mode 80PCC for I2C_CTRL */
#define _I2C_CTRL_BITO_160PCC              0x00000003UL                     /**< Mode 160PCC for I2C_CTRL */
#define I2C_CTRL_BITO_DEFAULT              (_I2C_CTRL_BITO_DEFAULT << 12)   /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_BITO_OFF                  (_I2C_CTRL_BITO_OFF << 12)       /**< Shifted mode OFF for I2C_CTRL */
#define I2C_CTRL_BITO_40PCC                (_I2C_CTRL_BITO_40PCC << 12)     /**< Shifted mode 40PCC for I2C_CTRL */
#define I2C_CTRL_BITO_80PCC                (_I2C_CTRL_BITO_80PCC << 12)     /**< Shifted mode 80PCC for I2C_CTRL */
#define I2C_CTRL_BITO_160PCC               (_I2C_CTRL_BITO_160PCC << 12)    /**< Shifted mode 160PCC for I2C_CTRL */
#define I2C_CTRL_GIBITO                    (0x1UL << 15)                    /**< Go Idle on Bus Idle Timeout  */
#define _I2C_CTRL_GIBITO_SHIFT             15                               /**< Shift value for I2C_GIBITO */
#define _I2C_CTRL_GIBITO_MASK              0x8000UL                         /**< Bit mask for I2C_GIBITO */
#define _I2C_CTRL_GIBITO_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_GIBITO_DEFAULT            (_I2C_CTRL_GIBITO_DEFAULT << 15) /**< Shifted mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_CLTO_SHIFT               16                               /**< Shift value for I2C_CLTO */
#define _I2C_CTRL_CLTO_MASK                0x70000UL                        /**< Bit mask for I2C_CLTO */
#define _I2C_CTRL_CLTO_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_CTRL */
#define _I2C_CTRL_CLTO_OFF                 0x00000000UL                     /**< Mode OFF for I2C_CTRL */
#define _I2C_CTRL_CLTO_40PCC               0x00000001UL                     /**< Mode 40PCC for I2C_CTRL */
#define _I2C_CTRL_CLTO_80PCC               0x00000002UL                     /**< Mode 80PCC for I2C_CTRL */
#define _I2C_CTRL_CLTO_160PCC              0x00000003UL                     /**< Mode 160PCC for I2C_CTRL */
#define _I2C_CTRL_CLTO_320PCC              0x00000004UL                     /**< Mode 320PCC for I2C_CTRL */
#define _I2C_CTRL_CLTO_1024PCC             0x00000005UL                     /**< Mode 1024PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_DEFAULT              (_I2C_CTRL_CLTO_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_CTRL */
#define I2C_CTRL_CLTO_OFF                  (_I2C_CTRL_CLTO_OFF << 16)       /**< Shifted mode OFF for I2C_CTRL */
#define I2C_CTRL_CLTO_40PCC                (_I2C_CTRL_CLTO_40PCC << 16)     /**< Shifted mode 40PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_80PCC                (_I2C_CTRL_CLTO_80PCC << 16)     /**< Shifted mode 80PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_160PCC               (_I2C_CTRL_CLTO_160PCC << 16)    /**< Shifted mode 160PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_320PCC               (_I2C_CTRL_CLTO_320PCC << 16)    /**< Shifted mode 320PCC for I2C_CTRL */
#define I2C_CTRL_CLTO_1024PCC              (_I2C_CTRL_CLTO_1024PCC << 16)   /**< Shifted mode 1024PCC for I2C_CTRL */

/* Bit fields for I2C CMD */
#define _I2C_CMD_RESETVALUE                0x00000000UL                    /**< Default value for I2C_CMD */
#define _I2C_CMD_MASK                      0x000000FFUL                    /**< Mask for I2C_CMD */
#define I2C_CMD_START                      (0x1UL << 0)                    /**< Send start condition */
#define _I2C_CMD_START_SHIFT               0                               /**< Shift value for I2C_START */
#define _I2C_CMD_START_MASK                0x1UL                           /**< Bit mask for I2C_START */
#define _I2C_CMD_START_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_START_DEFAULT              (_I2C_CMD_START_DEFAULT << 0)   /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_STOP                       (0x1UL << 1)                    /**< Send stop condition */
#define _I2C_CMD_STOP_SHIFT                1                               /**< Shift value for I2C_STOP */
#define _I2C_CMD_STOP_MASK                 0x2UL                           /**< Bit mask for I2C_STOP */
#define _I2C_CMD_STOP_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_STOP_DEFAULT               (_I2C_CMD_STOP_DEFAULT << 1)    /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_ACK                        (0x1UL << 2)                    /**< Send ACK */
#define _I2C_CMD_ACK_SHIFT                 2                               /**< Shift value for I2C_ACK */
#define _I2C_CMD_ACK_MASK                  0x4UL                           /**< Bit mask for I2C_ACK */
#define _I2C_CMD_ACK_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_ACK_DEFAULT                (_I2C_CMD_ACK_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_NACK                       (0x1UL << 3)                    /**< Send NACK */
#define _I2C_CMD_NACK_SHIFT                3                               /**< Shift value for I2C_NACK */
#define _I2C_CMD_NACK_MASK                 0x8UL                           /**< Bit mask for I2C_NACK */
#define _I2C_CMD_NACK_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_NACK_DEFAULT               (_I2C_CMD_NACK_DEFAULT << 3)    /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_CONT                       (0x1UL << 4)                    /**< Continue transmission */
#define _I2C_CMD_CONT_SHIFT                4                               /**< Shift value for I2C_CONT */
#define _I2C_CMD_CONT_MASK                 0x10UL                          /**< Bit mask for I2C_CONT */
#define _I2C_CMD_CONT_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_CONT_DEFAULT               (_I2C_CMD_CONT_DEFAULT << 4)    /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_ABORT                      (0x1UL << 5)                    /**< Abort transmission */
#define _I2C_CMD_ABORT_SHIFT               5                               /**< Shift value for I2C_ABORT */
#define _I2C_CMD_ABORT_MASK                0x20UL                          /**< Bit mask for I2C_ABORT */
#define _I2C_CMD_ABORT_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_ABORT_DEFAULT              (_I2C_CMD_ABORT_DEFAULT << 5)   /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARTX                    (0x1UL << 6)                    /**< Clear TX */
#define _I2C_CMD_CLEARTX_SHIFT             6                               /**< Shift value for I2C_CLEARTX */
#define _I2C_CMD_CLEARTX_MASK              0x40UL                          /**< Bit mask for I2C_CLEARTX */
#define _I2C_CMD_CLEARTX_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARTX_DEFAULT            (_I2C_CMD_CLEARTX_DEFAULT << 6) /**< Shifted mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARPC                    (0x1UL << 7)                    /**< Clear Pending Commands */
#define _I2C_CMD_CLEARPC_SHIFT             7                               /**< Shift value for I2C_CLEARPC */
#define _I2C_CMD_CLEARPC_MASK              0x80UL                          /**< Bit mask for I2C_CLEARPC */
#define _I2C_CMD_CLEARPC_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for I2C_CMD */
#define I2C_CMD_CLEARPC_DEFAULT            (_I2C_CMD_CLEARPC_DEFAULT << 7) /**< Shifted mode DEFAULT for I2C_CMD */

/* Bit fields for I2C STATE */
#define _I2C_STATE_RESETVALUE              0x00000001UL                          /**< Default value for I2C_STATE */
#define _I2C_STATE_MASK                    0x000000FFUL                          /**< Mask for I2C_STATE */
#define I2C_STATE_BUSY                     (0x1UL << 0)                          /**< Bus Busy */
#define _I2C_STATE_BUSY_SHIFT              0                                     /**< Shift value for I2C_BUSY */
#define _I2C_STATE_BUSY_MASK               0x1UL                                 /**< Bit mask for I2C_BUSY */
#define _I2C_STATE_BUSY_DEFAULT            0x00000001UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_BUSY_DEFAULT             (_I2C_STATE_BUSY_DEFAULT << 0)        /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_MASTER                   (0x1UL << 1)                          /**< Master */
#define _I2C_STATE_MASTER_SHIFT            1                                     /**< Shift value for I2C_MASTER */
#define _I2C_STATE_MASTER_MASK             0x2UL                                 /**< Bit mask for I2C_MASTER */
#define _I2C_STATE_MASTER_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_MASTER_DEFAULT           (_I2C_STATE_MASTER_DEFAULT << 1)      /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_TRANSMITTER              (0x1UL << 2)                          /**< Transmitter */
#define _I2C_STATE_TRANSMITTER_SHIFT       2                                     /**< Shift value for I2C_TRANSMITTER */
#define _I2C_STATE_TRANSMITTER_MASK        0x4UL                                 /**< Bit mask for I2C_TRANSMITTER */
#define _I2C_STATE_TRANSMITTER_DEFAULT     0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_TRANSMITTER_DEFAULT      (_I2C_STATE_TRANSMITTER_DEFAULT << 2) /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_NACKED                   (0x1UL << 3)                          /**< Nack Received */
#define _I2C_STATE_NACKED_SHIFT            3                                     /**< Shift value for I2C_NACKED */
#define _I2C_STATE_NACKED_MASK             0x8UL                                 /**< Bit mask for I2C_NACKED */
#define _I2C_STATE_NACKED_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_NACKED_DEFAULT           (_I2C_STATE_NACKED_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_BUSHOLD                  (0x1UL << 4)                          /**< Bus Held */
#define _I2C_STATE_BUSHOLD_SHIFT           4                                     /**< Shift value for I2C_BUSHOLD */
#define _I2C_STATE_BUSHOLD_MASK            0x10UL                                /**< Bit mask for I2C_BUSHOLD */
#define _I2C_STATE_BUSHOLD_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define I2C_STATE_BUSHOLD_DEFAULT          (_I2C_STATE_BUSHOLD_DEFAULT << 4)     /**< Shifted mode DEFAULT for I2C_STATE */
#define _I2C_STATE_STATE_SHIFT             5                                     /**< Shift value for I2C_STATE */
#define _I2C_STATE_STATE_MASK              0xE0UL                                /**< Bit mask for I2C_STATE */
#define _I2C_STATE_STATE_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for I2C_STATE */
#define _I2C_STATE_STATE_IDLE              0x00000000UL                          /**< Mode IDLE for I2C_STATE */
#define _I2C_STATE_STATE_WAIT              0x00000001UL                          /**< Mode WAIT for I2C_STATE */
#define _I2C_STATE_STATE_START             0x00000002UL                          /**< Mode START for I2C_STATE */
#define _I2C_STATE_STATE_ADDR              0x00000003UL                          /**< Mode ADDR for I2C_STATE */
#define _I2C_STATE_STATE_ADDRACK           0x00000004UL                          /**< Mode ADDRACK for I2C_STATE */
#define _I2C_STATE_STATE_DATA              0x00000005UL                          /**< Mode DATA for I2C_STATE */
#define _I2C_STATE_STATE_DATAACK           0x00000006UL                          /**< Mode DATAACK for I2C_STATE */
#define I2C_STATE_STATE_DEFAULT            (_I2C_STATE_STATE_DEFAULT << 5)       /**< Shifted mode DEFAULT for I2C_STATE */
#define I2C_STATE_STATE_IDLE               (_I2C_STATE_STATE_IDLE << 5)          /**< Shifted mode IDLE for I2C_STATE */
#define I2C_STATE_STATE_WAIT               (_I2C_STATE_STATE_WAIT << 5)          /**< Shifted mode WAIT for I2C_STATE */
#define I2C_STATE_STATE_START              (_I2C_STATE_STATE_START << 5)         /**< Shifted mode START for I2C_STATE */
#define I2C_STATE_STATE_ADDR               (_I2C_STATE_STATE_ADDR << 5)          /**< Shifted mode ADDR for I2C_STATE */
#define I2C_STATE_STATE_ADDRACK            (_I2C_STATE_STATE_ADDRACK << 5)       /**< Shifted mode ADDRACK for I2C_STATE */
#define I2C_STATE_STATE_DATA               (_I2C_STATE_STATE_DATA << 5)          /**< Shifted mode DATA for I2C_STATE */
#define I2C_STATE_STATE_DATAACK            (_I2C_STATE_STATE_DATAACK << 5)       /**< Shifted mode DATAACK for I2C_STATE */

/* Bit fields for I2C STATUS */
#define _I2C_STATUS_RESETVALUE             0x00000080UL                       /**< Default value for I2C_STATUS */
#define _I2C_STATUS_MASK                   0x000003FFUL                       /**< Mask for I2C_STATUS */
#define I2C_STATUS_PSTART                  (0x1UL << 0)                       /**< Pending START */
#define _I2C_STATUS_PSTART_SHIFT           0                                  /**< Shift value for I2C_PSTART */
#define _I2C_STATUS_PSTART_MASK            0x1UL                              /**< Bit mask for I2C_PSTART */
#define _I2C_STATUS_PSTART_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PSTART_DEFAULT          (_I2C_STATUS_PSTART_DEFAULT << 0)  /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PSTOP                   (0x1UL << 1)                       /**< Pending STOP */
#define _I2C_STATUS_PSTOP_SHIFT            1                                  /**< Shift value for I2C_PSTOP */
#define _I2C_STATUS_PSTOP_MASK             0x2UL                              /**< Bit mask for I2C_PSTOP */
#define _I2C_STATUS_PSTOP_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PSTOP_DEFAULT           (_I2C_STATUS_PSTOP_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PACK                    (0x1UL << 2)                       /**< Pending ACK */
#define _I2C_STATUS_PACK_SHIFT             2                                  /**< Shift value for I2C_PACK */
#define _I2C_STATUS_PACK_MASK              0x4UL                              /**< Bit mask for I2C_PACK */
#define _I2C_STATUS_PACK_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PACK_DEFAULT            (_I2C_STATUS_PACK_DEFAULT << 2)    /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PNACK                   (0x1UL << 3)                       /**< Pending NACK */
#define _I2C_STATUS_PNACK_SHIFT            3                                  /**< Shift value for I2C_PNACK */
#define _I2C_STATUS_PNACK_MASK             0x8UL                              /**< Bit mask for I2C_PNACK */
#define _I2C_STATUS_PNACK_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PNACK_DEFAULT           (_I2C_STATUS_PNACK_DEFAULT << 3)   /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PCONT                   (0x1UL << 4)                       /**< Pending continue */
#define _I2C_STATUS_PCONT_SHIFT            4                                  /**< Shift value for I2C_PCONT */
#define _I2C_STATUS_PCONT_MASK             0x10UL                             /**< Bit mask for I2C_PCONT */
#define _I2C_STATUS_PCONT_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PCONT_DEFAULT           (_I2C_STATUS_PCONT_DEFAULT << 4)   /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PABORT                  (0x1UL << 5)                       /**< Pending abort */
#define _I2C_STATUS_PABORT_SHIFT           5                                  /**< Shift value for I2C_PABORT */
#define _I2C_STATUS_PABORT_MASK            0x20UL                             /**< Bit mask for I2C_PABORT */
#define _I2C_STATUS_PABORT_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_PABORT_DEFAULT          (_I2C_STATUS_PABORT_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXC                     (0x1UL << 6)                       /**< TX Complete */
#define _I2C_STATUS_TXC_SHIFT              6                                  /**< Shift value for I2C_TXC */
#define _I2C_STATUS_TXC_MASK               0x40UL                             /**< Bit mask for I2C_TXC */
#define _I2C_STATUS_TXC_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXC_DEFAULT             (_I2C_STATUS_TXC_DEFAULT << 6)     /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXBL                    (0x1UL << 7)                       /**< TX Buffer Level */
#define _I2C_STATUS_TXBL_SHIFT             7                                  /**< Shift value for I2C_TXBL */
#define _I2C_STATUS_TXBL_MASK              0x80UL                             /**< Bit mask for I2C_TXBL */
#define _I2C_STATUS_TXBL_DEFAULT           0x00000001UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_TXBL_DEFAULT            (_I2C_STATUS_TXBL_DEFAULT << 7)    /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_RXDATAV                 (0x1UL << 8)                       /**< RX Data Valid */
#define _I2C_STATUS_RXDATAV_SHIFT          8                                  /**< Shift value for I2C_RXDATAV */
#define _I2C_STATUS_RXDATAV_MASK           0x100UL                            /**< Bit mask for I2C_RXDATAV */
#define _I2C_STATUS_RXDATAV_DEFAULT        0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_RXDATAV_DEFAULT         (_I2C_STATUS_RXDATAV_DEFAULT << 8) /**< Shifted mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_RXFULL                  (0x1UL << 9)                       /**< RX FIFO Full */
#define _I2C_STATUS_RXFULL_SHIFT           9                                  /**< Shift value for I2C_RXFULL */
#define _I2C_STATUS_RXFULL_MASK            0x200UL                            /**< Bit mask for I2C_RXFULL */
#define _I2C_STATUS_RXFULL_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for I2C_STATUS */
#define I2C_STATUS_RXFULL_DEFAULT          (_I2C_STATUS_RXFULL_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_STATUS */

/* Bit fields for I2C CLKDIV */
#define _I2C_CLKDIV_RESETVALUE             0x00000000UL                   /**< Default value for I2C_CLKDIV */
#define _I2C_CLKDIV_MASK                   0x000001FFUL                   /**< Mask for I2C_CLKDIV */
#define _I2C_CLKDIV_DIV_SHIFT              0                              /**< Shift value for I2C_DIV */
#define _I2C_CLKDIV_DIV_MASK               0x1FFUL                        /**< Bit mask for I2C_DIV */
#define _I2C_CLKDIV_DIV_DEFAULT            0x00000000UL                   /**< Mode DEFAULT for I2C_CLKDIV */
#define I2C_CLKDIV_DIV_DEFAULT             (_I2C_CLKDIV_DIV_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_CLKDIV */

/* Bit fields for I2C SADDR */
#define _I2C_SADDR_RESETVALUE              0x00000000UL                   /**< Default value for I2C_SADDR */
#define _I2C_SADDR_MASK                    0x000000FEUL                   /**< Mask for I2C_SADDR */
#define _I2C_SADDR_ADDR_SHIFT              1                              /**< Shift value for I2C_ADDR */
#define _I2C_SADDR_ADDR_MASK               0xFEUL                         /**< Bit mask for I2C_ADDR */
#define _I2C_SADDR_ADDR_DEFAULT            0x00000000UL                   /**< Mode DEFAULT for I2C_SADDR */
#define I2C_SADDR_ADDR_DEFAULT             (_I2C_SADDR_ADDR_DEFAULT << 1) /**< Shifted mode DEFAULT for I2C_SADDR */

/* Bit fields for I2C SADDRMASK */
#define _I2C_SADDRMASK_RESETVALUE          0x00000000UL                       /**< Default value for I2C_SADDRMASK */
#define _I2C_SADDRMASK_MASK                0x000000FEUL                       /**< Mask for I2C_SADDRMASK */
#define _I2C_SADDRMASK_MASK_SHIFT          1                                  /**< Shift value for I2C_MASK */
#define _I2C_SADDRMASK_MASK_MASK           0xFEUL                             /**< Bit mask for I2C_MASK */
#define _I2C_SADDRMASK_MASK_DEFAULT        0x00000000UL                       /**< Mode DEFAULT for I2C_SADDRMASK */
#define I2C_SADDRMASK_MASK_DEFAULT         (_I2C_SADDRMASK_MASK_DEFAULT << 1) /**< Shifted mode DEFAULT for I2C_SADDRMASK */

/* Bit fields for I2C RXDATA */
#define _I2C_RXDATA_RESETVALUE             0x00000000UL                      /**< Default value for I2C_RXDATA */
#define _I2C_RXDATA_MASK                   0x000000FFUL                      /**< Mask for I2C_RXDATA */
#define _I2C_RXDATA_RXDATA_SHIFT           0                                 /**< Shift value for I2C_RXDATA */
#define _I2C_RXDATA_RXDATA_MASK            0xFFUL                            /**< Bit mask for I2C_RXDATA */
#define _I2C_RXDATA_RXDATA_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for I2C_RXDATA */
#define I2C_RXDATA_RXDATA_DEFAULT          (_I2C_RXDATA_RXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_RXDATA */

/* Bit fields for I2C RXDOUBLE */
#define _I2C_RXDOUBLE_RESETVALUE           0x00000000UL                         /**< Default value for I2C_RXDOUBLE */
#define _I2C_RXDOUBLE_MASK                 0x0000FFFFUL                         /**< Mask for I2C_RXDOUBLE */
#define _I2C_RXDOUBLE_RXDATA0_SHIFT        0                                    /**< Shift value for I2C_RXDATA0 */
#define _I2C_RXDOUBLE_RXDATA0_MASK         0xFFUL                               /**< Bit mask for I2C_RXDATA0 */
#define _I2C_RXDOUBLE_RXDATA0_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for I2C_RXDOUBLE */
#define I2C_RXDOUBLE_RXDATA0_DEFAULT       (_I2C_RXDOUBLE_RXDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_RXDOUBLE */
#define _I2C_RXDOUBLE_RXDATA1_SHIFT        8                                    /**< Shift value for I2C_RXDATA1 */
#define _I2C_RXDOUBLE_RXDATA1_MASK         0xFF00UL                             /**< Bit mask for I2C_RXDATA1 */
#define _I2C_RXDOUBLE_RXDATA1_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for I2C_RXDOUBLE */
#define I2C_RXDOUBLE_RXDATA1_DEFAULT       (_I2C_RXDOUBLE_RXDATA1_DEFAULT << 8) /**< Shifted mode DEFAULT for I2C_RXDOUBLE */

/* Bit fields for I2C RXDATAP */
#define _I2C_RXDATAP_RESETVALUE            0x00000000UL                        /**< Default value for I2C_RXDATAP */
#define _I2C_RXDATAP_MASK                  0x000000FFUL                        /**< Mask for I2C_RXDATAP */
#define _I2C_RXDATAP_RXDATAP_SHIFT         0                                   /**< Shift value for I2C_RXDATAP */
#define _I2C_RXDATAP_RXDATAP_MASK          0xFFUL                              /**< Bit mask for I2C_RXDATAP */
#define _I2C_RXDATAP_RXDATAP_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for I2C_RXDATAP */
#define I2C_RXDATAP_RXDATAP_DEFAULT        (_I2C_RXDATAP_RXDATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_RXDATAP */

/* Bit fields for I2C RXDOUBLEP */
#define _I2C_RXDOUBLEP_RESETVALUE          0x00000000UL                           /**< Default value for I2C_RXDOUBLEP */
#define _I2C_RXDOUBLEP_MASK                0x0000FFFFUL                           /**< Mask for I2C_RXDOUBLEP */
#define _I2C_RXDOUBLEP_RXDATAP0_SHIFT      0                                      /**< Shift value for I2C_RXDATAP0 */
#define _I2C_RXDOUBLEP_RXDATAP0_MASK       0xFFUL                                 /**< Bit mask for I2C_RXDATAP0 */
#define _I2C_RXDOUBLEP_RXDATAP0_DEFAULT    0x00000000UL                           /**< Mode DEFAULT for I2C_RXDOUBLEP */
#define I2C_RXDOUBLEP_RXDATAP0_DEFAULT     (_I2C_RXDOUBLEP_RXDATAP0_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_RXDOUBLEP */
#define _I2C_RXDOUBLEP_RXDATAP1_SHIFT      8                                      /**< Shift value for I2C_RXDATAP1 */
#define _I2C_RXDOUBLEP_RXDATAP1_MASK       0xFF00UL                               /**< Bit mask for I2C_RXDATAP1 */
#define _I2C_RXDOUBLEP_RXDATAP1_DEFAULT    0x00000000UL                           /**< Mode DEFAULT for I2C_RXDOUBLEP */
#define I2C_RXDOUBLEP_RXDATAP1_DEFAULT     (_I2C_RXDOUBLEP_RXDATAP1_DEFAULT << 8) /**< Shifted mode DEFAULT for I2C_RXDOUBLEP */

/* Bit fields for I2C TXDATA */
#define _I2C_TXDATA_RESETVALUE             0x00000000UL                      /**< Default value for I2C_TXDATA */
#define _I2C_TXDATA_MASK                   0x000000FFUL                      /**< Mask for I2C_TXDATA */
#define _I2C_TXDATA_TXDATA_SHIFT           0                                 /**< Shift value for I2C_TXDATA */
#define _I2C_TXDATA_TXDATA_MASK            0xFFUL                            /**< Bit mask for I2C_TXDATA */
#define _I2C_TXDATA_TXDATA_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for I2C_TXDATA */
#define I2C_TXDATA_TXDATA_DEFAULT          (_I2C_TXDATA_TXDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_TXDATA */

/* Bit fields for I2C TXDOUBLE */
#define _I2C_TXDOUBLE_RESETVALUE           0x00000000UL                         /**< Default value for I2C_TXDOUBLE */
#define _I2C_TXDOUBLE_MASK                 0x0000FFFFUL                         /**< Mask for I2C_TXDOUBLE */
#define _I2C_TXDOUBLE_TXDATA0_SHIFT        0                                    /**< Shift value for I2C_TXDATA0 */
#define _I2C_TXDOUBLE_TXDATA0_MASK         0xFFUL                               /**< Bit mask for I2C_TXDATA0 */
#define _I2C_TXDOUBLE_TXDATA0_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for I2C_TXDOUBLE */
#define I2C_TXDOUBLE_TXDATA0_DEFAULT       (_I2C_TXDOUBLE_TXDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_TXDOUBLE */
#define _I2C_TXDOUBLE_TXDATA1_SHIFT        8                                    /**< Shift value for I2C_TXDATA1 */
#define _I2C_TXDOUBLE_TXDATA1_MASK         0xFF00UL                             /**< Bit mask for I2C_TXDATA1 */
#define _I2C_TXDOUBLE_TXDATA1_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for I2C_TXDOUBLE */
#define I2C_TXDOUBLE_TXDATA1_DEFAULT       (_I2C_TXDOUBLE_TXDATA1_DEFAULT << 8) /**< Shifted mode DEFAULT for I2C_TXDOUBLE */

/* Bit fields for I2C IF */
#define _I2C_IF_RESETVALUE                 0x00000010UL                    /**< Default value for I2C_IF */
#define _I2C_IF_MASK                       0x0007FFFFUL                    /**< Mask for I2C_IF */
#define I2C_IF_START                       (0x1UL << 0)                    /**< START condition Interrupt Flag */
#define _I2C_IF_START_SHIFT                0                               /**< Shift value for I2C_START */
#define _I2C_IF_START_MASK                 0x1UL                           /**< Bit mask for I2C_START */
#define _I2C_IF_START_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_START_DEFAULT               (_I2C_IF_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_RSTART                      (0x1UL << 1)                    /**< Repeated START condition Interrupt Flag */
#define _I2C_IF_RSTART_SHIFT               1                               /**< Shift value for I2C_RSTART */
#define _I2C_IF_RSTART_MASK                0x2UL                           /**< Bit mask for I2C_RSTART */
#define _I2C_IF_RSTART_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_RSTART_DEFAULT              (_I2C_IF_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_ADDR                        (0x1UL << 2)                    /**< Address Interrupt Flag */
#define _I2C_IF_ADDR_SHIFT                 2                               /**< Shift value for I2C_ADDR */
#define _I2C_IF_ADDR_MASK                  0x4UL                           /**< Bit mask for I2C_ADDR */
#define _I2C_IF_ADDR_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_ADDR_DEFAULT                (_I2C_IF_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_TXC                         (0x1UL << 3)                    /**< Transfer Completed Interrupt Flag */
#define _I2C_IF_TXC_SHIFT                  3                               /**< Shift value for I2C_TXC */
#define _I2C_IF_TXC_MASK                   0x8UL                           /**< Bit mask for I2C_TXC */
#define _I2C_IF_TXC_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_TXC_DEFAULT                 (_I2C_IF_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_TXBL                        (0x1UL << 4)                    /**< Transmit Buffer Level Interrupt Flag */
#define _I2C_IF_TXBL_SHIFT                 4                               /**< Shift value for I2C_TXBL */
#define _I2C_IF_TXBL_MASK                  0x10UL                          /**< Bit mask for I2C_TXBL */
#define _I2C_IF_TXBL_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_TXBL_DEFAULT                (_I2C_IF_TXBL_DEFAULT << 4)     /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_RXDATAV                     (0x1UL << 5)                    /**< Receive Data Valid Interrupt Flag */
#define _I2C_IF_RXDATAV_SHIFT              5                               /**< Shift value for I2C_RXDATAV */
#define _I2C_IF_RXDATAV_MASK               0x20UL                          /**< Bit mask for I2C_RXDATAV */
#define _I2C_IF_RXDATAV_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_RXDATAV_DEFAULT             (_I2C_IF_RXDATAV_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_ACK                         (0x1UL << 6)                    /**< Acknowledge Received Interrupt Flag */
#define _I2C_IF_ACK_SHIFT                  6                               /**< Shift value for I2C_ACK */
#define _I2C_IF_ACK_MASK                   0x40UL                          /**< Bit mask for I2C_ACK */
#define _I2C_IF_ACK_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_ACK_DEFAULT                 (_I2C_IF_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_NACK                        (0x1UL << 7)                    /**< Not Acknowledge Received Interrupt Flag */
#define _I2C_IF_NACK_SHIFT                 7                               /**< Shift value for I2C_NACK */
#define _I2C_IF_NACK_MASK                  0x80UL                          /**< Bit mask for I2C_NACK */
#define _I2C_IF_NACK_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_NACK_DEFAULT                (_I2C_IF_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_MSTOP                       (0x1UL << 8)                    /**< Master STOP Condition Interrupt Flag */
#define _I2C_IF_MSTOP_SHIFT                8                               /**< Shift value for I2C_MSTOP */
#define _I2C_IF_MSTOP_MASK                 0x100UL                         /**< Bit mask for I2C_MSTOP */
#define _I2C_IF_MSTOP_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_MSTOP_DEFAULT               (_I2C_IF_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_ARBLOST                     (0x1UL << 9)                    /**< Arbitration Lost Interrupt Flag */
#define _I2C_IF_ARBLOST_SHIFT              9                               /**< Shift value for I2C_ARBLOST */
#define _I2C_IF_ARBLOST_MASK               0x200UL                         /**< Bit mask for I2C_ARBLOST */
#define _I2C_IF_ARBLOST_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_ARBLOST_DEFAULT             (_I2C_IF_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_BUSERR                      (0x1UL << 10)                   /**< Bus Error Interrupt Flag */
#define _I2C_IF_BUSERR_SHIFT               10                              /**< Shift value for I2C_BUSERR */
#define _I2C_IF_BUSERR_MASK                0x400UL                         /**< Bit mask for I2C_BUSERR */
#define _I2C_IF_BUSERR_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_BUSERR_DEFAULT              (_I2C_IF_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_BUSHOLD                     (0x1UL << 11)                   /**< Bus Held Interrupt Flag */
#define _I2C_IF_BUSHOLD_SHIFT              11                              /**< Shift value for I2C_BUSHOLD */
#define _I2C_IF_BUSHOLD_MASK               0x800UL                         /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IF_BUSHOLD_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_BUSHOLD_DEFAULT             (_I2C_IF_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_TXOF                        (0x1UL << 12)                   /**< Transmit Buffer Overflow Interrupt Flag */
#define _I2C_IF_TXOF_SHIFT                 12                              /**< Shift value for I2C_TXOF */
#define _I2C_IF_TXOF_MASK                  0x1000UL                        /**< Bit mask for I2C_TXOF */
#define _I2C_IF_TXOF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_TXOF_DEFAULT                (_I2C_IF_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_RXUF                        (0x1UL << 13)                   /**< Receive Buffer Underflow Interrupt Flag */
#define _I2C_IF_RXUF_SHIFT                 13                              /**< Shift value for I2C_RXUF */
#define _I2C_IF_RXUF_MASK                  0x2000UL                        /**< Bit mask for I2C_RXUF */
#define _I2C_IF_RXUF_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_RXUF_DEFAULT                (_I2C_IF_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_BITO                        (0x1UL << 14)                   /**< Bus Idle Timeout Interrupt Flag */
#define _I2C_IF_BITO_SHIFT                 14                              /**< Shift value for I2C_BITO */
#define _I2C_IF_BITO_MASK                  0x4000UL                        /**< Bit mask for I2C_BITO */
#define _I2C_IF_BITO_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_BITO_DEFAULT                (_I2C_IF_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_CLTO                        (0x1UL << 15)                   /**< Clock Low Timeout Interrupt Flag */
#define _I2C_IF_CLTO_SHIFT                 15                              /**< Shift value for I2C_CLTO */
#define _I2C_IF_CLTO_MASK                  0x8000UL                        /**< Bit mask for I2C_CLTO */
#define _I2C_IF_CLTO_DEFAULT               0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_CLTO_DEFAULT                (_I2C_IF_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_SSTOP                       (0x1UL << 16)                   /**< Slave STOP condition Interrupt Flag */
#define _I2C_IF_SSTOP_SHIFT                16                              /**< Shift value for I2C_SSTOP */
#define _I2C_IF_SSTOP_MASK                 0x10000UL                       /**< Bit mask for I2C_SSTOP */
#define _I2C_IF_SSTOP_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_SSTOP_DEFAULT               (_I2C_IF_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_RXFULL                      (0x1UL << 17)                   /**< Receive Buffer Full Interrupt Flag */
#define _I2C_IF_RXFULL_SHIFT               17                              /**< Shift value for I2C_RXFULL */
#define _I2C_IF_RXFULL_MASK                0x20000UL                       /**< Bit mask for I2C_RXFULL */
#define _I2C_IF_RXFULL_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_RXFULL_DEFAULT              (_I2C_IF_RXFULL_DEFAULT << 17)  /**< Shifted mode DEFAULT for I2C_IF */
#define I2C_IF_CLERR                       (0x1UL << 18)                   /**< Clock Low Error Interrupt Flag */
#define _I2C_IF_CLERR_SHIFT                18                              /**< Shift value for I2C_CLERR */
#define _I2C_IF_CLERR_MASK                 0x40000UL                       /**< Bit mask for I2C_CLERR */
#define _I2C_IF_CLERR_DEFAULT              0x00000000UL                    /**< Mode DEFAULT for I2C_IF */
#define I2C_IF_CLERR_DEFAULT               (_I2C_IF_CLERR_DEFAULT << 18)   /**< Shifted mode DEFAULT for I2C_IF */

/* Bit fields for I2C IFS */
#define _I2C_IFS_RESETVALUE                0x00000000UL                     /**< Default value for I2C_IFS */
#define _I2C_IFS_MASK                      0x0007FFCFUL                     /**< Mask for I2C_IFS */
#define I2C_IFS_START                      (0x1UL << 0)                     /**< Set START Interrupt Flag */
#define _I2C_IFS_START_SHIFT               0                                /**< Shift value for I2C_START */
#define _I2C_IFS_START_MASK                0x1UL                            /**< Bit mask for I2C_START */
#define _I2C_IFS_START_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_START_DEFAULT              (_I2C_IFS_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_RSTART                     (0x1UL << 1)                     /**< Set RSTART Interrupt Flag */
#define _I2C_IFS_RSTART_SHIFT              1                                /**< Shift value for I2C_RSTART */
#define _I2C_IFS_RSTART_MASK               0x2UL                            /**< Bit mask for I2C_RSTART */
#define _I2C_IFS_RSTART_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_RSTART_DEFAULT             (_I2C_IFS_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_ADDR                       (0x1UL << 2)                     /**< Set ADDR Interrupt Flag */
#define _I2C_IFS_ADDR_SHIFT                2                                /**< Shift value for I2C_ADDR */
#define _I2C_IFS_ADDR_MASK                 0x4UL                            /**< Bit mask for I2C_ADDR */
#define _I2C_IFS_ADDR_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_ADDR_DEFAULT               (_I2C_IFS_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXC                        (0x1UL << 3)                     /**< Set TXC Interrupt Flag */
#define _I2C_IFS_TXC_SHIFT                 3                                /**< Shift value for I2C_TXC */
#define _I2C_IFS_TXC_MASK                  0x8UL                            /**< Bit mask for I2C_TXC */
#define _I2C_IFS_TXC_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXC_DEFAULT                (_I2C_IFS_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_ACK                        (0x1UL << 6)                     /**< Set ACK Interrupt Flag */
#define _I2C_IFS_ACK_SHIFT                 6                                /**< Shift value for I2C_ACK */
#define _I2C_IFS_ACK_MASK                  0x40UL                           /**< Bit mask for I2C_ACK */
#define _I2C_IFS_ACK_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_ACK_DEFAULT                (_I2C_IFS_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_NACK                       (0x1UL << 7)                     /**< Set NACK Interrupt Flag */
#define _I2C_IFS_NACK_SHIFT                7                                /**< Shift value for I2C_NACK */
#define _I2C_IFS_NACK_MASK                 0x80UL                           /**< Bit mask for I2C_NACK */
#define _I2C_IFS_NACK_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_NACK_DEFAULT               (_I2C_IFS_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_MSTOP                      (0x1UL << 8)                     /**< Set MSTOP Interrupt Flag */
#define _I2C_IFS_MSTOP_SHIFT               8                                /**< Shift value for I2C_MSTOP */
#define _I2C_IFS_MSTOP_MASK                0x100UL                          /**< Bit mask for I2C_MSTOP */
#define _I2C_IFS_MSTOP_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_MSTOP_DEFAULT              (_I2C_IFS_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_ARBLOST                    (0x1UL << 9)                     /**< Set ARBLOST Interrupt Flag */
#define _I2C_IFS_ARBLOST_SHIFT             9                                /**< Shift value for I2C_ARBLOST */
#define _I2C_IFS_ARBLOST_MASK              0x200UL                          /**< Bit mask for I2C_ARBLOST */
#define _I2C_IFS_ARBLOST_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_ARBLOST_DEFAULT            (_I2C_IFS_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSERR                     (0x1UL << 10)                    /**< Set BUSERR Interrupt Flag */
#define _I2C_IFS_BUSERR_SHIFT              10                               /**< Shift value for I2C_BUSERR */
#define _I2C_IFS_BUSERR_MASK               0x400UL                          /**< Bit mask for I2C_BUSERR */
#define _I2C_IFS_BUSERR_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSERR_DEFAULT             (_I2C_IFS_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSHOLD                    (0x1UL << 11)                    /**< Set BUSHOLD Interrupt Flag */
#define _I2C_IFS_BUSHOLD_SHIFT             11                               /**< Shift value for I2C_BUSHOLD */
#define _I2C_IFS_BUSHOLD_MASK              0x800UL                          /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IFS_BUSHOLD_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_BUSHOLD_DEFAULT            (_I2C_IFS_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXOF                       (0x1UL << 12)                    /**< Set TXOF Interrupt Flag */
#define _I2C_IFS_TXOF_SHIFT                12                               /**< Shift value for I2C_TXOF */
#define _I2C_IFS_TXOF_MASK                 0x1000UL                         /**< Bit mask for I2C_TXOF */
#define _I2C_IFS_TXOF_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_TXOF_DEFAULT               (_I2C_IFS_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_RXUF                       (0x1UL << 13)                    /**< Set RXUF Interrupt Flag */
#define _I2C_IFS_RXUF_SHIFT                13                               /**< Shift value for I2C_RXUF */
#define _I2C_IFS_RXUF_MASK                 0x2000UL                         /**< Bit mask for I2C_RXUF */
#define _I2C_IFS_RXUF_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_RXUF_DEFAULT               (_I2C_IFS_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_BITO                       (0x1UL << 14)                    /**< Set BITO Interrupt Flag */
#define _I2C_IFS_BITO_SHIFT                14                               /**< Shift value for I2C_BITO */
#define _I2C_IFS_BITO_MASK                 0x4000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_IFS_BITO_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_BITO_DEFAULT               (_I2C_IFS_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_CLTO                       (0x1UL << 15)                    /**< Set CLTO Interrupt Flag */
#define _I2C_IFS_CLTO_SHIFT                15                               /**< Shift value for I2C_CLTO */
#define _I2C_IFS_CLTO_MASK                 0x8000UL                         /**< Bit mask for I2C_CLTO */
#define _I2C_IFS_CLTO_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_CLTO_DEFAULT               (_I2C_IFS_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_SSTOP                      (0x1UL << 16)                    /**< Set SSTOP Interrupt Flag */
#define _I2C_IFS_SSTOP_SHIFT               16                               /**< Shift value for I2C_SSTOP */
#define _I2C_IFS_SSTOP_MASK                0x10000UL                        /**< Bit mask for I2C_SSTOP */
#define _I2C_IFS_SSTOP_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_SSTOP_DEFAULT              (_I2C_IFS_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_RXFULL                     (0x1UL << 17)                    /**< Set RXFULL Interrupt Flag */
#define _I2C_IFS_RXFULL_SHIFT              17                               /**< Shift value for I2C_RXFULL */
#define _I2C_IFS_RXFULL_MASK               0x20000UL                        /**< Bit mask for I2C_RXFULL */
#define _I2C_IFS_RXFULL_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_RXFULL_DEFAULT             (_I2C_IFS_RXFULL_DEFAULT << 17)  /**< Shifted mode DEFAULT for I2C_IFS */
#define I2C_IFS_CLERR                      (0x1UL << 18)                    /**< Set CLERR Interrupt Flag */
#define _I2C_IFS_CLERR_SHIFT               18                               /**< Shift value for I2C_CLERR */
#define _I2C_IFS_CLERR_MASK                0x40000UL                        /**< Bit mask for I2C_CLERR */
#define _I2C_IFS_CLERR_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFS */
#define I2C_IFS_CLERR_DEFAULT              (_I2C_IFS_CLERR_DEFAULT << 18)   /**< Shifted mode DEFAULT for I2C_IFS */

/* Bit fields for I2C IFC */
#define _I2C_IFC_RESETVALUE                0x00000000UL                     /**< Default value for I2C_IFC */
#define _I2C_IFC_MASK                      0x0007FFCFUL                     /**< Mask for I2C_IFC */
#define I2C_IFC_START                      (0x1UL << 0)                     /**< Clear START Interrupt Flag */
#define _I2C_IFC_START_SHIFT               0                                /**< Shift value for I2C_START */
#define _I2C_IFC_START_MASK                0x1UL                            /**< Bit mask for I2C_START */
#define _I2C_IFC_START_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_START_DEFAULT              (_I2C_IFC_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_RSTART                     (0x1UL << 1)                     /**< Clear RSTART Interrupt Flag */
#define _I2C_IFC_RSTART_SHIFT              1                                /**< Shift value for I2C_RSTART */
#define _I2C_IFC_RSTART_MASK               0x2UL                            /**< Bit mask for I2C_RSTART */
#define _I2C_IFC_RSTART_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_RSTART_DEFAULT             (_I2C_IFC_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_ADDR                       (0x1UL << 2)                     /**< Clear ADDR Interrupt Flag */
#define _I2C_IFC_ADDR_SHIFT                2                                /**< Shift value for I2C_ADDR */
#define _I2C_IFC_ADDR_MASK                 0x4UL                            /**< Bit mask for I2C_ADDR */
#define _I2C_IFC_ADDR_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_ADDR_DEFAULT               (_I2C_IFC_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXC                        (0x1UL << 3)                     /**< Clear TXC Interrupt Flag */
#define _I2C_IFC_TXC_SHIFT                 3                                /**< Shift value for I2C_TXC */
#define _I2C_IFC_TXC_MASK                  0x8UL                            /**< Bit mask for I2C_TXC */
#define _I2C_IFC_TXC_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXC_DEFAULT                (_I2C_IFC_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_ACK                        (0x1UL << 6)                     /**< Clear ACK Interrupt Flag */
#define _I2C_IFC_ACK_SHIFT                 6                                /**< Shift value for I2C_ACK */
#define _I2C_IFC_ACK_MASK                  0x40UL                           /**< Bit mask for I2C_ACK */
#define _I2C_IFC_ACK_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_ACK_DEFAULT                (_I2C_IFC_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_NACK                       (0x1UL << 7)                     /**< Clear NACK Interrupt Flag */
#define _I2C_IFC_NACK_SHIFT                7                                /**< Shift value for I2C_NACK */
#define _I2C_IFC_NACK_MASK                 0x80UL                           /**< Bit mask for I2C_NACK */
#define _I2C_IFC_NACK_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_NACK_DEFAULT               (_I2C_IFC_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_MSTOP                      (0x1UL << 8)                     /**< Clear MSTOP Interrupt Flag */
#define _I2C_IFC_MSTOP_SHIFT               8                                /**< Shift value for I2C_MSTOP */
#define _I2C_IFC_MSTOP_MASK                0x100UL                          /**< Bit mask for I2C_MSTOP */
#define _I2C_IFC_MSTOP_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_MSTOP_DEFAULT              (_I2C_IFC_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_ARBLOST                    (0x1UL << 9)                     /**< Clear ARBLOST Interrupt Flag */
#define _I2C_IFC_ARBLOST_SHIFT             9                                /**< Shift value for I2C_ARBLOST */
#define _I2C_IFC_ARBLOST_MASK              0x200UL                          /**< Bit mask for I2C_ARBLOST */
#define _I2C_IFC_ARBLOST_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_ARBLOST_DEFAULT            (_I2C_IFC_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSERR                     (0x1UL << 10)                    /**< Clear BUSERR Interrupt Flag */
#define _I2C_IFC_BUSERR_SHIFT              10                               /**< Shift value for I2C_BUSERR */
#define _I2C_IFC_BUSERR_MASK               0x400UL                          /**< Bit mask for I2C_BUSERR */
#define _I2C_IFC_BUSERR_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSERR_DEFAULT             (_I2C_IFC_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSHOLD                    (0x1UL << 11)                    /**< Clear BUSHOLD Interrupt Flag */
#define _I2C_IFC_BUSHOLD_SHIFT             11                               /**< Shift value for I2C_BUSHOLD */
#define _I2C_IFC_BUSHOLD_MASK              0x800UL                          /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IFC_BUSHOLD_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_BUSHOLD_DEFAULT            (_I2C_IFC_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXOF                       (0x1UL << 12)                    /**< Clear TXOF Interrupt Flag */
#define _I2C_IFC_TXOF_SHIFT                12                               /**< Shift value for I2C_TXOF */
#define _I2C_IFC_TXOF_MASK                 0x1000UL                         /**< Bit mask for I2C_TXOF */
#define _I2C_IFC_TXOF_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_TXOF_DEFAULT               (_I2C_IFC_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_RXUF                       (0x1UL << 13)                    /**< Clear RXUF Interrupt Flag */
#define _I2C_IFC_RXUF_SHIFT                13                               /**< Shift value for I2C_RXUF */
#define _I2C_IFC_RXUF_MASK                 0x2000UL                         /**< Bit mask for I2C_RXUF */
#define _I2C_IFC_RXUF_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_RXUF_DEFAULT               (_I2C_IFC_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_BITO                       (0x1UL << 14)                    /**< Clear BITO Interrupt Flag */
#define _I2C_IFC_BITO_SHIFT                14                               /**< Shift value for I2C_BITO */
#define _I2C_IFC_BITO_MASK                 0x4000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_IFC_BITO_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_BITO_DEFAULT               (_I2C_IFC_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_CLTO                       (0x1UL << 15)                    /**< Clear CLTO Interrupt Flag */
#define _I2C_IFC_CLTO_SHIFT                15                               /**< Shift value for I2C_CLTO */
#define _I2C_IFC_CLTO_MASK                 0x8000UL                         /**< Bit mask for I2C_CLTO */
#define _I2C_IFC_CLTO_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_CLTO_DEFAULT               (_I2C_IFC_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_SSTOP                      (0x1UL << 16)                    /**< Clear SSTOP Interrupt Flag */
#define _I2C_IFC_SSTOP_SHIFT               16                               /**< Shift value for I2C_SSTOP */
#define _I2C_IFC_SSTOP_MASK                0x10000UL                        /**< Bit mask for I2C_SSTOP */
#define _I2C_IFC_SSTOP_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_SSTOP_DEFAULT              (_I2C_IFC_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_RXFULL                     (0x1UL << 17)                    /**< Clear RXFULL Interrupt Flag */
#define _I2C_IFC_RXFULL_SHIFT              17                               /**< Shift value for I2C_RXFULL */
#define _I2C_IFC_RXFULL_MASK               0x20000UL                        /**< Bit mask for I2C_RXFULL */
#define _I2C_IFC_RXFULL_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_RXFULL_DEFAULT             (_I2C_IFC_RXFULL_DEFAULT << 17)  /**< Shifted mode DEFAULT for I2C_IFC */
#define I2C_IFC_CLERR                      (0x1UL << 18)                    /**< Clear CLERR Interrupt Flag */
#define _I2C_IFC_CLERR_SHIFT               18                               /**< Shift value for I2C_CLERR */
#define _I2C_IFC_CLERR_MASK                0x40000UL                        /**< Bit mask for I2C_CLERR */
#define _I2C_IFC_CLERR_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IFC */
#define I2C_IFC_CLERR_DEFAULT              (_I2C_IFC_CLERR_DEFAULT << 18)   /**< Shifted mode DEFAULT for I2C_IFC */

/* Bit fields for I2C IEN */
#define _I2C_IEN_RESETVALUE                0x00000000UL                     /**< Default value for I2C_IEN */
#define _I2C_IEN_MASK                      0x0007FFFFUL                     /**< Mask for I2C_IEN */
#define I2C_IEN_START                      (0x1UL << 0)                     /**< START Interrupt Enable */
#define _I2C_IEN_START_SHIFT               0                                /**< Shift value for I2C_START */
#define _I2C_IEN_START_MASK                0x1UL                            /**< Bit mask for I2C_START */
#define _I2C_IEN_START_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_START_DEFAULT              (_I2C_IEN_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_RSTART                     (0x1UL << 1)                     /**< RSTART Interrupt Enable */
#define _I2C_IEN_RSTART_SHIFT              1                                /**< Shift value for I2C_RSTART */
#define _I2C_IEN_RSTART_MASK               0x2UL                            /**< Bit mask for I2C_RSTART */
#define _I2C_IEN_RSTART_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_RSTART_DEFAULT             (_I2C_IEN_RSTART_DEFAULT << 1)   /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_ADDR                       (0x1UL << 2)                     /**< ADDR Interrupt Enable */
#define _I2C_IEN_ADDR_SHIFT                2                                /**< Shift value for I2C_ADDR */
#define _I2C_IEN_ADDR_MASK                 0x4UL                            /**< Bit mask for I2C_ADDR */
#define _I2C_IEN_ADDR_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_ADDR_DEFAULT               (_I2C_IEN_ADDR_DEFAULT << 2)     /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXC                        (0x1UL << 3)                     /**< TXC Interrupt Enable */
#define _I2C_IEN_TXC_SHIFT                 3                                /**< Shift value for I2C_TXC */
#define _I2C_IEN_TXC_MASK                  0x8UL                            /**< Bit mask for I2C_TXC */
#define _I2C_IEN_TXC_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXC_DEFAULT                (_I2C_IEN_TXC_DEFAULT << 3)      /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXBL                       (0x1UL << 4)                     /**< TXBL Interrupt Enable */
#define _I2C_IEN_TXBL_SHIFT                4                                /**< Shift value for I2C_TXBL */
#define _I2C_IEN_TXBL_MASK                 0x10UL                           /**< Bit mask for I2C_TXBL */
#define _I2C_IEN_TXBL_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXBL_DEFAULT               (_I2C_IEN_TXBL_DEFAULT << 4)     /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXDATAV                    (0x1UL << 5)                     /**< RXDATAV Interrupt Enable */
#define _I2C_IEN_RXDATAV_SHIFT             5                                /**< Shift value for I2C_RXDATAV */
#define _I2C_IEN_RXDATAV_MASK              0x20UL                           /**< Bit mask for I2C_RXDATAV */
#define _I2C_IEN_RXDATAV_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXDATAV_DEFAULT            (_I2C_IEN_RXDATAV_DEFAULT << 5)  /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_ACK                        (0x1UL << 6)                     /**< ACK Interrupt Enable */
#define _I2C_IEN_ACK_SHIFT                 6                                /**< Shift value for I2C_ACK */
#define _I2C_IEN_ACK_MASK                  0x40UL                           /**< Bit mask for I2C_ACK */
#define _I2C_IEN_ACK_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_ACK_DEFAULT                (_I2C_IEN_ACK_DEFAULT << 6)      /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_NACK                       (0x1UL << 7)                     /**< NACK Interrupt Enable */
#define _I2C_IEN_NACK_SHIFT                7                                /**< Shift value for I2C_NACK */
#define _I2C_IEN_NACK_MASK                 0x80UL                           /**< Bit mask for I2C_NACK */
#define _I2C_IEN_NACK_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_NACK_DEFAULT               (_I2C_IEN_NACK_DEFAULT << 7)     /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_MSTOP                      (0x1UL << 8)                     /**< MSTOP Interrupt Enable */
#define _I2C_IEN_MSTOP_SHIFT               8                                /**< Shift value for I2C_MSTOP */
#define _I2C_IEN_MSTOP_MASK                0x100UL                          /**< Bit mask for I2C_MSTOP */
#define _I2C_IEN_MSTOP_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_MSTOP_DEFAULT              (_I2C_IEN_MSTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_ARBLOST                    (0x1UL << 9)                     /**< ARBLOST Interrupt Enable */
#define _I2C_IEN_ARBLOST_SHIFT             9                                /**< Shift value for I2C_ARBLOST */
#define _I2C_IEN_ARBLOST_MASK              0x200UL                          /**< Bit mask for I2C_ARBLOST */
#define _I2C_IEN_ARBLOST_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_ARBLOST_DEFAULT            (_I2C_IEN_ARBLOST_DEFAULT << 9)  /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSERR                     (0x1UL << 10)                    /**< BUSERR Interrupt Enable */
#define _I2C_IEN_BUSERR_SHIFT              10                               /**< Shift value for I2C_BUSERR */
#define _I2C_IEN_BUSERR_MASK               0x400UL                          /**< Bit mask for I2C_BUSERR */
#define _I2C_IEN_BUSERR_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSERR_DEFAULT             (_I2C_IEN_BUSERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSHOLD                    (0x1UL << 11)                    /**< BUSHOLD Interrupt Enable */
#define _I2C_IEN_BUSHOLD_SHIFT             11                               /**< Shift value for I2C_BUSHOLD */
#define _I2C_IEN_BUSHOLD_MASK              0x800UL                          /**< Bit mask for I2C_BUSHOLD */
#define _I2C_IEN_BUSHOLD_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_BUSHOLD_DEFAULT            (_I2C_IEN_BUSHOLD_DEFAULT << 11) /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXOF                       (0x1UL << 12)                    /**< TXOF Interrupt Enable */
#define _I2C_IEN_TXOF_SHIFT                12                               /**< Shift value for I2C_TXOF */
#define _I2C_IEN_TXOF_MASK                 0x1000UL                         /**< Bit mask for I2C_TXOF */
#define _I2C_IEN_TXOF_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_TXOF_DEFAULT               (_I2C_IEN_TXOF_DEFAULT << 12)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXUF                       (0x1UL << 13)                    /**< RXUF Interrupt Enable */
#define _I2C_IEN_RXUF_SHIFT                13                               /**< Shift value for I2C_RXUF */
#define _I2C_IEN_RXUF_MASK                 0x2000UL                         /**< Bit mask for I2C_RXUF */
#define _I2C_IEN_RXUF_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXUF_DEFAULT               (_I2C_IEN_RXUF_DEFAULT << 13)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_BITO                       (0x1UL << 14)                    /**< BITO Interrupt Enable */
#define _I2C_IEN_BITO_SHIFT                14                               /**< Shift value for I2C_BITO */
#define _I2C_IEN_BITO_MASK                 0x4000UL                         /**< Bit mask for I2C_BITO */
#define _I2C_IEN_BITO_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_BITO_DEFAULT               (_I2C_IEN_BITO_DEFAULT << 14)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_CLTO                       (0x1UL << 15)                    /**< CLTO Interrupt Enable */
#define _I2C_IEN_CLTO_SHIFT                15                               /**< Shift value for I2C_CLTO */
#define _I2C_IEN_CLTO_MASK                 0x8000UL                         /**< Bit mask for I2C_CLTO */
#define _I2C_IEN_CLTO_DEFAULT              0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_CLTO_DEFAULT               (_I2C_IEN_CLTO_DEFAULT << 15)    /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_SSTOP                      (0x1UL << 16)                    /**< SSTOP Interrupt Enable */
#define _I2C_IEN_SSTOP_SHIFT               16                               /**< Shift value for I2C_SSTOP */
#define _I2C_IEN_SSTOP_MASK                0x10000UL                        /**< Bit mask for I2C_SSTOP */
#define _I2C_IEN_SSTOP_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_SSTOP_DEFAULT              (_I2C_IEN_SSTOP_DEFAULT << 16)   /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXFULL                     (0x1UL << 17)                    /**< RXFULL Interrupt Enable */
#define _I2C_IEN_RXFULL_SHIFT              17                               /**< Shift value for I2C_RXFULL */
#define _I2C_IEN_RXFULL_MASK               0x20000UL                        /**< Bit mask for I2C_RXFULL */
#define _I2C_IEN_RXFULL_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_RXFULL_DEFAULT             (_I2C_IEN_RXFULL_DEFAULT << 17)  /**< Shifted mode DEFAULT for I2C_IEN */
#define I2C_IEN_CLERR                      (0x1UL << 18)                    /**< CLERR Interrupt Enable */
#define _I2C_IEN_CLERR_SHIFT               18                               /**< Shift value for I2C_CLERR */
#define _I2C_IEN_CLERR_MASK                0x40000UL                        /**< Bit mask for I2C_CLERR */
#define _I2C_IEN_CLERR_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for I2C_IEN */
#define I2C_IEN_CLERR_DEFAULT              (_I2C_IEN_CLERR_DEFAULT << 18)   /**< Shifted mode DEFAULT for I2C_IEN */

/* Bit fields for I2C ROUTEPEN */
#define _I2C_ROUTEPEN_RESETVALUE           0x00000000UL                        /**< Default value for I2C_ROUTEPEN */
#define _I2C_ROUTEPEN_MASK                 0x00000003UL                        /**< Mask for I2C_ROUTEPEN */
#define I2C_ROUTEPEN_SDAPEN                (0x1UL << 0)                        /**< SDA Pin Enable */
#define _I2C_ROUTEPEN_SDAPEN_SHIFT         0                                   /**< Shift value for I2C_SDAPEN */
#define _I2C_ROUTEPEN_SDAPEN_MASK          0x1UL                               /**< Bit mask for I2C_SDAPEN */
#define _I2C_ROUTEPEN_SDAPEN_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for I2C_ROUTEPEN */
#define I2C_ROUTEPEN_SDAPEN_DEFAULT        (_I2C_ROUTEPEN_SDAPEN_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_ROUTEPEN */
#define I2C_ROUTEPEN_SCLPEN                (0x1UL << 1)                        /**< SCL Pin Enable */
#define _I2C_ROUTEPEN_SCLPEN_SHIFT         1                                   /**< Shift value for I2C_SCLPEN */
#define _I2C_ROUTEPEN_SCLPEN_MASK          0x2UL                               /**< Bit mask for I2C_SCLPEN */
#define _I2C_ROUTEPEN_SCLPEN_DEFAULT       0x00000000UL                        /**< Mode DEFAULT for I2C_ROUTEPEN */
#define I2C_ROUTEPEN_SCLPEN_DEFAULT        (_I2C_ROUTEPEN_SCLPEN_DEFAULT << 1) /**< Shifted mode DEFAULT for I2C_ROUTEPEN */

/* Bit fields for I2C ROUTELOC0 */
#define _I2C_ROUTELOC0_RESETVALUE          0x00000000UL                         /**< Default value for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_MASK                0x00001F1FUL                         /**< Mask for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_SHIFT        0                                    /**< Shift value for I2C_SDALOC */
#define _I2C_ROUTELOC0_SDALOC_MASK         0x1FUL                               /**< Bit mask for I2C_SDALOC */
#define _I2C_ROUTELOC0_SDALOC_LOC0         0x00000000UL                         /**< Mode LOC0 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC1         0x00000001UL                         /**< Mode LOC1 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC2         0x00000002UL                         /**< Mode LOC2 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC3         0x00000003UL                         /**< Mode LOC3 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC4         0x00000004UL                         /**< Mode LOC4 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC5         0x00000005UL                         /**< Mode LOC5 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC6         0x00000006UL                         /**< Mode LOC6 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC7         0x00000007UL                         /**< Mode LOC7 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC8         0x00000008UL                         /**< Mode LOC8 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC9         0x00000009UL                         /**< Mode LOC9 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC10        0x0000000AUL                         /**< Mode LOC10 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC11        0x0000000BUL                         /**< Mode LOC11 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC12        0x0000000CUL                         /**< Mode LOC12 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC13        0x0000000DUL                         /**< Mode LOC13 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC14        0x0000000EUL                         /**< Mode LOC14 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC15        0x0000000FUL                         /**< Mode LOC15 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC16        0x00000010UL                         /**< Mode LOC16 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC17        0x00000011UL                         /**< Mode LOC17 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC18        0x00000012UL                         /**< Mode LOC18 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC19        0x00000013UL                         /**< Mode LOC19 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC20        0x00000014UL                         /**< Mode LOC20 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC21        0x00000015UL                         /**< Mode LOC21 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC22        0x00000016UL                         /**< Mode LOC22 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC23        0x00000017UL                         /**< Mode LOC23 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC24        0x00000018UL                         /**< Mode LOC24 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC25        0x00000019UL                         /**< Mode LOC25 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC26        0x0000001AUL                         /**< Mode LOC26 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC27        0x0000001BUL                         /**< Mode LOC27 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC28        0x0000001CUL                         /**< Mode LOC28 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC29        0x0000001DUL                         /**< Mode LOC29 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC30        0x0000001EUL                         /**< Mode LOC30 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SDALOC_LOC31        0x0000001FUL                         /**< Mode LOC31 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC0          (_I2C_ROUTELOC0_SDALOC_LOC0 << 0)    /**< Shifted mode LOC0 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_DEFAULT       (_I2C_ROUTELOC0_SDALOC_DEFAULT << 0) /**< Shifted mode DEFAULT for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC1          (_I2C_ROUTELOC0_SDALOC_LOC1 << 0)    /**< Shifted mode LOC1 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC2          (_I2C_ROUTELOC0_SDALOC_LOC2 << 0)    /**< Shifted mode LOC2 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC3          (_I2C_ROUTELOC0_SDALOC_LOC3 << 0)    /**< Shifted mode LOC3 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC4          (_I2C_ROUTELOC0_SDALOC_LOC4 << 0)    /**< Shifted mode LOC4 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC5          (_I2C_ROUTELOC0_SDALOC_LOC5 << 0)    /**< Shifted mode LOC5 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC6          (_I2C_ROUTELOC0_SDALOC_LOC6 << 0)    /**< Shifted mode LOC6 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC7          (_I2C_ROUTELOC0_SDALOC_LOC7 << 0)    /**< Shifted mode LOC7 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC8          (_I2C_ROUTELOC0_SDALOC_LOC8 << 0)    /**< Shifted mode LOC8 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC9          (_I2C_ROUTELOC0_SDALOC_LOC9 << 0)    /**< Shifted mode LOC9 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC10         (_I2C_ROUTELOC0_SDALOC_LOC10 << 0)   /**< Shifted mode LOC10 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC11         (_I2C_ROUTELOC0_SDALOC_LOC11 << 0)   /**< Shifted mode LOC11 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC12         (_I2C_ROUTELOC0_SDALOC_LOC12 << 0)   /**< Shifted mode LOC12 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC13         (_I2C_ROUTELOC0_SDALOC_LOC13 << 0)   /**< Shifted mode LOC13 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC14         (_I2C_ROUTELOC0_SDALOC_LOC14 << 0)   /**< Shifted mode LOC14 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC15         (_I2C_ROUTELOC0_SDALOC_LOC15 << 0)   /**< Shifted mode LOC15 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC16         (_I2C_ROUTELOC0_SDALOC_LOC16 << 0)   /**< Shifted mode LOC16 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC17         (_I2C_ROUTELOC0_SDALOC_LOC17 << 0)   /**< Shifted mode LOC17 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC18         (_I2C_ROUTELOC0_SDALOC_LOC18 << 0)   /**< Shifted mode LOC18 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC19         (_I2C_ROUTELOC0_SDALOC_LOC19 << 0)   /**< Shifted mode LOC19 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC20         (_I2C_ROUTELOC0_SDALOC_LOC20 << 0)   /**< Shifted mode LOC20 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC21         (_I2C_ROUTELOC0_SDALOC_LOC21 << 0)   /**< Shifted mode LOC21 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC22         (_I2C_ROUTELOC0_SDALOC_LOC22 << 0)   /**< Shifted mode LOC22 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC23         (_I2C_ROUTELOC0_SDALOC_LOC23 << 0)   /**< Shifted mode LOC23 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC24         (_I2C_ROUTELOC0_SDALOC_LOC24 << 0)   /**< Shifted mode LOC24 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC25         (_I2C_ROUTELOC0_SDALOC_LOC25 << 0)   /**< Shifted mode LOC25 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC26         (_I2C_ROUTELOC0_SDALOC_LOC26 << 0)   /**< Shifted mode LOC26 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC27         (_I2C_ROUTELOC0_SDALOC_LOC27 << 0)   /**< Shifted mode LOC27 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC28         (_I2C_ROUTELOC0_SDALOC_LOC28 << 0)   /**< Shifted mode LOC28 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC29         (_I2C_ROUTELOC0_SDALOC_LOC29 << 0)   /**< Shifted mode LOC29 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC30         (_I2C_ROUTELOC0_SDALOC_LOC30 << 0)   /**< Shifted mode LOC30 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SDALOC_LOC31         (_I2C_ROUTELOC0_SDALOC_LOC31 << 0)   /**< Shifted mode LOC31 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_SHIFT        8                                    /**< Shift value for I2C_SCLLOC */
#define _I2C_ROUTELOC0_SCLLOC_MASK         0x1F00UL                             /**< Bit mask for I2C_SCLLOC */
#define _I2C_ROUTELOC0_SCLLOC_LOC0         0x00000000UL                         /**< Mode LOC0 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC1         0x00000001UL                         /**< Mode LOC1 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC2         0x00000002UL                         /**< Mode LOC2 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC3         0x00000003UL                         /**< Mode LOC3 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC4         0x00000004UL                         /**< Mode LOC4 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC5         0x00000005UL                         /**< Mode LOC5 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC6         0x00000006UL                         /**< Mode LOC6 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC7         0x00000007UL                         /**< Mode LOC7 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC8         0x00000008UL                         /**< Mode LOC8 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC9         0x00000009UL                         /**< Mode LOC9 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC10        0x0000000AUL                         /**< Mode LOC10 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC11        0x0000000BUL                         /**< Mode LOC11 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC12        0x0000000CUL                         /**< Mode LOC12 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC13        0x0000000DUL                         /**< Mode LOC13 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC14        0x0000000EUL                         /**< Mode LOC14 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC15        0x0000000FUL                         /**< Mode LOC15 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC16        0x00000010UL                         /**< Mode LOC16 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC17        0x00000011UL                         /**< Mode LOC17 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC18        0x00000012UL                         /**< Mode LOC18 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC19        0x00000013UL                         /**< Mode LOC19 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC20        0x00000014UL                         /**< Mode LOC20 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC21        0x00000015UL                         /**< Mode LOC21 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC22        0x00000016UL                         /**< Mode LOC22 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC23        0x00000017UL                         /**< Mode LOC23 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC24        0x00000018UL                         /**< Mode LOC24 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC25        0x00000019UL                         /**< Mode LOC25 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC26        0x0000001AUL                         /**< Mode LOC26 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC27        0x0000001BUL                         /**< Mode LOC27 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC28        0x0000001CUL                         /**< Mode LOC28 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC29        0x0000001DUL                         /**< Mode LOC29 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC30        0x0000001EUL                         /**< Mode LOC30 for I2C_ROUTELOC0 */
#define _I2C_ROUTELOC0_SCLLOC_LOC31        0x0000001FUL                         /**< Mode LOC31 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC0          (_I2C_ROUTELOC0_SCLLOC_LOC0 << 8)    /**< Shifted mode LOC0 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_DEFAULT       (_I2C_ROUTELOC0_SCLLOC_DEFAULT << 8) /**< Shifted mode DEFAULT for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC1          (_I2C_ROUTELOC0_SCLLOC_LOC1 << 8)    /**< Shifted mode LOC1 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC2          (_I2C_ROUTELOC0_SCLLOC_LOC2 << 8)    /**< Shifted mode LOC2 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC3          (_I2C_ROUTELOC0_SCLLOC_LOC3 << 8)    /**< Shifted mode LOC3 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC4          (_I2C_ROUTELOC0_SCLLOC_LOC4 << 8)    /**< Shifted mode LOC4 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC5          (_I2C_ROUTELOC0_SCLLOC_LOC5 << 8)    /**< Shifted mode LOC5 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC6          (_I2C_ROUTELOC0_SCLLOC_LOC6 << 8)    /**< Shifted mode LOC6 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC7          (_I2C_ROUTELOC0_SCLLOC_LOC7 << 8)    /**< Shifted mode LOC7 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC8          (_I2C_ROUTELOC0_SCLLOC_LOC8 << 8)    /**< Shifted mode LOC8 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC9          (_I2C_ROUTELOC0_SCLLOC_LOC9 << 8)    /**< Shifted mode LOC9 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC10         (_I2C_ROUTELOC0_SCLLOC_LOC10 << 8)   /**< Shifted mode LOC10 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC11         (_I2C_ROUTELOC0_SCLLOC_LOC11 << 8)   /**< Shifted mode LOC11 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC12         (_I2C_ROUTELOC0_SCLLOC_LOC12 << 8)   /**< Shifted mode LOC12 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC13         (_I2C_ROUTELOC0_SCLLOC_LOC13 << 8)   /**< Shifted mode LOC13 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC14         (_I2C_ROUTELOC0_SCLLOC_LOC14 << 8)   /**< Shifted mode LOC14 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC15         (_I2C_ROUTELOC0_SCLLOC_LOC15 << 8)   /**< Shifted mode LOC15 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC16         (_I2C_ROUTELOC0_SCLLOC_LOC16 << 8)   /**< Shifted mode LOC16 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC17         (_I2C_ROUTELOC0_SCLLOC_LOC17 << 8)   /**< Shifted mode LOC17 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC18         (_I2C_ROUTELOC0_SCLLOC_LOC18 << 8)   /**< Shifted mode LOC18 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC19         (_I2C_ROUTELOC0_SCLLOC_LOC19 << 8)   /**< Shifted mode LOC19 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC20         (_I2C_ROUTELOC0_SCLLOC_LOC20 << 8)   /**< Shifted mode LOC20 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC21         (_I2C_ROUTELOC0_SCLLOC_LOC21 << 8)   /**< Shifted mode LOC21 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC22         (_I2C_ROUTELOC0_SCLLOC_LOC22 << 8)   /**< Shifted mode LOC22 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC23         (_I2C_ROUTELOC0_SCLLOC_LOC23 << 8)   /**< Shifted mode LOC23 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC24         (_I2C_ROUTELOC0_SCLLOC_LOC24 << 8)   /**< Shifted mode LOC24 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC25         (_I2C_ROUTELOC0_SCLLOC_LOC25 << 8)   /**< Shifted mode LOC25 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC26         (_I2C_ROUTELOC0_SCLLOC_LOC26 << 8)   /**< Shifted mode LOC26 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC27         (_I2C_ROUTELOC0_SCLLOC_LOC27 << 8)   /**< Shifted mode LOC27 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC28         (_I2C_ROUTELOC0_SCLLOC_LOC28 << 8)   /**< Shifted mode LOC28 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC29         (_I2C_ROUTELOC0_SCLLOC_LOC29 << 8)   /**< Shifted mode LOC29 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC30         (_I2C_ROUTELOC0_SCLLOC_LOC30 << 8)   /**< Shifted mode LOC30 for I2C_ROUTELOC0 */
#define I2C_ROUTELOC0_SCLLOC_LOC31         (_I2C_ROUTELOC0_SCLLOC_LOC31 << 8)   /**< Shifted mode LOC31 for I2C_ROUTELOC0 */

/** @} End of group EFR32FG1P_I2C */
/** @} End of group Parts */

