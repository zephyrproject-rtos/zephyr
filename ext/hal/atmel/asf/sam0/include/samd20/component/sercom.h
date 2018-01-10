/**
 * \file
 *
 * \brief Component description for SERCOM
 *
 * Copyright (c) 2017 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAMD20_SERCOM_COMPONENT_
#define _SAMD20_SERCOM_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR SERCOM */
/* ========================================================================== */
/** \addtogroup SAMD20_SERCOM Serial Communication Interface */
/*@{*/

#define SERCOM_U2201
#define REV_SERCOM                  0x102

/* -------- SERCOM_I2CM_CTRLA : (SERCOM Offset: 0x00) (R/W 32) I2CM I2CM Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t MODE:3;           /*!< bit:  2.. 4  Operating Mode                     */
    uint32_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      7  Run in Standby                     */
    uint32_t :8;               /*!< bit:  8..15  Reserved                           */
    uint32_t PINOUT:1;         /*!< bit:     16  Pin Usage                          */
    uint32_t :3;               /*!< bit: 17..19  Reserved                           */
    uint32_t SDAHOLD:2;        /*!< bit: 20..21  SDA Hold Time                      */
    uint32_t :6;               /*!< bit: 22..27  Reserved                           */
    uint32_t INACTOUT:2;       /*!< bit: 28..29  Inactive Time-out                  */
    uint32_t LOWTOUT:1;        /*!< bit:     30  SCL Low Time-out                   */
    uint32_t :1;               /*!< bit:     31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CM_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_CTRLA_OFFSET    0x00         /**< \brief (SERCOM_I2CM_CTRLA offset) I2CM Control A */
#define SERCOM_I2CM_CTRLA_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_I2CM_CTRLA reset_value) I2CM Control A */

#define SERCOM_I2CM_CTRLA_SWRST_Pos 0            /**< \brief (SERCOM_I2CM_CTRLA) Software Reset */
#define SERCOM_I2CM_CTRLA_SWRST     (_U_(0x1) << SERCOM_I2CM_CTRLA_SWRST_Pos)
#define SERCOM_I2CM_CTRLA_ENABLE_Pos 1            /**< \brief (SERCOM_I2CM_CTRLA) Enable */
#define SERCOM_I2CM_CTRLA_ENABLE    (_U_(0x1) << SERCOM_I2CM_CTRLA_ENABLE_Pos)
#define SERCOM_I2CM_CTRLA_MODE_Pos  2            /**< \brief (SERCOM_I2CM_CTRLA) Operating Mode */
#define SERCOM_I2CM_CTRLA_MODE_Msk  (_U_(0x7) << SERCOM_I2CM_CTRLA_MODE_Pos)
#define SERCOM_I2CM_CTRLA_MODE(value) (SERCOM_I2CM_CTRLA_MODE_Msk & ((value) << SERCOM_I2CM_CTRLA_MODE_Pos))
#define   SERCOM_I2CM_CTRLA_MODE_USART_EXT_CLK_Val _U_(0x0)   /**< \brief (SERCOM_I2CM_CTRLA) USART mode with external clock */
#define   SERCOM_I2CM_CTRLA_MODE_USART_INT_CLK_Val _U_(0x1)   /**< \brief (SERCOM_I2CM_CTRLA) USART mode with internal clock */
#define   SERCOM_I2CM_CTRLA_MODE_SPI_SLAVE_Val _U_(0x2)   /**< \brief (SERCOM_I2CM_CTRLA) SPI mode with external clock */
#define   SERCOM_I2CM_CTRLA_MODE_SPI_MASTER_Val _U_(0x3)   /**< \brief (SERCOM_I2CM_CTRLA) SPI mode with internal clock */
#define   SERCOM_I2CM_CTRLA_MODE_I2C_SLAVE_Val _U_(0x4)   /**< \brief (SERCOM_I2CM_CTRLA) I2C mode with external clock */
#define   SERCOM_I2CM_CTRLA_MODE_I2C_MASTER_Val _U_(0x5)   /**< \brief (SERCOM_I2CM_CTRLA) I2C mode with internal clock */
#define SERCOM_I2CM_CTRLA_MODE_USART_EXT_CLK (SERCOM_I2CM_CTRLA_MODE_USART_EXT_CLK_Val << SERCOM_I2CM_CTRLA_MODE_Pos)
#define SERCOM_I2CM_CTRLA_MODE_USART_INT_CLK (SERCOM_I2CM_CTRLA_MODE_USART_INT_CLK_Val << SERCOM_I2CM_CTRLA_MODE_Pos)
#define SERCOM_I2CM_CTRLA_MODE_SPI_SLAVE (SERCOM_I2CM_CTRLA_MODE_SPI_SLAVE_Val << SERCOM_I2CM_CTRLA_MODE_Pos)
#define SERCOM_I2CM_CTRLA_MODE_SPI_MASTER (SERCOM_I2CM_CTRLA_MODE_SPI_MASTER_Val << SERCOM_I2CM_CTRLA_MODE_Pos)
#define SERCOM_I2CM_CTRLA_MODE_I2C_SLAVE (SERCOM_I2CM_CTRLA_MODE_I2C_SLAVE_Val << SERCOM_I2CM_CTRLA_MODE_Pos)
#define SERCOM_I2CM_CTRLA_MODE_I2C_MASTER (SERCOM_I2CM_CTRLA_MODE_I2C_MASTER_Val << SERCOM_I2CM_CTRLA_MODE_Pos)
#define SERCOM_I2CM_CTRLA_RUNSTDBY_Pos 7            /**< \brief (SERCOM_I2CM_CTRLA) Run in Standby */
#define SERCOM_I2CM_CTRLA_RUNSTDBY  (_U_(0x1) << SERCOM_I2CM_CTRLA_RUNSTDBY_Pos)
#define SERCOM_I2CM_CTRLA_PINOUT_Pos 16           /**< \brief (SERCOM_I2CM_CTRLA) Pin Usage */
#define SERCOM_I2CM_CTRLA_PINOUT    (_U_(0x1) << SERCOM_I2CM_CTRLA_PINOUT_Pos)
#define SERCOM_I2CM_CTRLA_SDAHOLD_Pos 20           /**< \brief (SERCOM_I2CM_CTRLA) SDA Hold Time */
#define SERCOM_I2CM_CTRLA_SDAHOLD_Msk (_U_(0x3) << SERCOM_I2CM_CTRLA_SDAHOLD_Pos)
#define SERCOM_I2CM_CTRLA_SDAHOLD(value) (SERCOM_I2CM_CTRLA_SDAHOLD_Msk & ((value) << SERCOM_I2CM_CTRLA_SDAHOLD_Pos))
#define SERCOM_I2CM_CTRLA_INACTOUT_Pos 28           /**< \brief (SERCOM_I2CM_CTRLA) Inactive Time-out */
#define SERCOM_I2CM_CTRLA_INACTOUT_Msk (_U_(0x3) << SERCOM_I2CM_CTRLA_INACTOUT_Pos)
#define SERCOM_I2CM_CTRLA_INACTOUT(value) (SERCOM_I2CM_CTRLA_INACTOUT_Msk & ((value) << SERCOM_I2CM_CTRLA_INACTOUT_Pos))
#define SERCOM_I2CM_CTRLA_LOWTOUT_Pos 30           /**< \brief (SERCOM_I2CM_CTRLA) SCL Low Time-out */
#define SERCOM_I2CM_CTRLA_LOWTOUT   (_U_(0x1) << SERCOM_I2CM_CTRLA_LOWTOUT_Pos)
#define SERCOM_I2CM_CTRLA_MASK      _U_(0x7031009F) /**< \brief (SERCOM_I2CM_CTRLA) MASK Register */

/* -------- SERCOM_I2CS_CTRLA : (SERCOM Offset: 0x00) (R/W 32) I2CS I2CS Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t MODE:3;           /*!< bit:  2.. 4  Operating Mode                     */
    uint32_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      7  Run in Standby                     */
    uint32_t :8;               /*!< bit:  8..15  Reserved                           */
    uint32_t PINOUT:1;         /*!< bit:     16  Pin Usage                          */
    uint32_t :3;               /*!< bit: 17..19  Reserved                           */
    uint32_t SDAHOLD:2;        /*!< bit: 20..21  SDA Hold Time                      */
    uint32_t :8;               /*!< bit: 22..29  Reserved                           */
    uint32_t LOWTOUT:1;        /*!< bit:     30  SCL Low Time-out                   */
    uint32_t :1;               /*!< bit:     31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CS_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_CTRLA_OFFSET    0x00         /**< \brief (SERCOM_I2CS_CTRLA offset) I2CS Control A */
#define SERCOM_I2CS_CTRLA_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_I2CS_CTRLA reset_value) I2CS Control A */

#define SERCOM_I2CS_CTRLA_SWRST_Pos 0            /**< \brief (SERCOM_I2CS_CTRLA) Software Reset */
#define SERCOM_I2CS_CTRLA_SWRST     (_U_(0x1) << SERCOM_I2CS_CTRLA_SWRST_Pos)
#define SERCOM_I2CS_CTRLA_ENABLE_Pos 1            /**< \brief (SERCOM_I2CS_CTRLA) Enable */
#define SERCOM_I2CS_CTRLA_ENABLE    (_U_(0x1) << SERCOM_I2CS_CTRLA_ENABLE_Pos)
#define SERCOM_I2CS_CTRLA_MODE_Pos  2            /**< \brief (SERCOM_I2CS_CTRLA) Operating Mode */
#define SERCOM_I2CS_CTRLA_MODE_Msk  (_U_(0x7) << SERCOM_I2CS_CTRLA_MODE_Pos)
#define SERCOM_I2CS_CTRLA_MODE(value) (SERCOM_I2CS_CTRLA_MODE_Msk & ((value) << SERCOM_I2CS_CTRLA_MODE_Pos))
#define   SERCOM_I2CS_CTRLA_MODE_USART_EXT_CLK_Val _U_(0x0)   /**< \brief (SERCOM_I2CS_CTRLA) USART mode with external clock */
#define   SERCOM_I2CS_CTRLA_MODE_USART_INT_CLK_Val _U_(0x1)   /**< \brief (SERCOM_I2CS_CTRLA) USART mode with internal clock */
#define   SERCOM_I2CS_CTRLA_MODE_SPI_SLAVE_Val _U_(0x2)   /**< \brief (SERCOM_I2CS_CTRLA) SPI mode with external clock */
#define   SERCOM_I2CS_CTRLA_MODE_SPI_MASTER_Val _U_(0x3)   /**< \brief (SERCOM_I2CS_CTRLA) SPI mode with internal clock */
#define   SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE_Val _U_(0x4)   /**< \brief (SERCOM_I2CS_CTRLA) I2C mode with external clock */
#define   SERCOM_I2CS_CTRLA_MODE_I2C_MASTER_Val _U_(0x5)   /**< \brief (SERCOM_I2CS_CTRLA) I2C mode with internal clock */
#define SERCOM_I2CS_CTRLA_MODE_USART_EXT_CLK (SERCOM_I2CS_CTRLA_MODE_USART_EXT_CLK_Val << SERCOM_I2CS_CTRLA_MODE_Pos)
#define SERCOM_I2CS_CTRLA_MODE_USART_INT_CLK (SERCOM_I2CS_CTRLA_MODE_USART_INT_CLK_Val << SERCOM_I2CS_CTRLA_MODE_Pos)
#define SERCOM_I2CS_CTRLA_MODE_SPI_SLAVE (SERCOM_I2CS_CTRLA_MODE_SPI_SLAVE_Val << SERCOM_I2CS_CTRLA_MODE_Pos)
#define SERCOM_I2CS_CTRLA_MODE_SPI_MASTER (SERCOM_I2CS_CTRLA_MODE_SPI_MASTER_Val << SERCOM_I2CS_CTRLA_MODE_Pos)
#define SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE (SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE_Val << SERCOM_I2CS_CTRLA_MODE_Pos)
#define SERCOM_I2CS_CTRLA_MODE_I2C_MASTER (SERCOM_I2CS_CTRLA_MODE_I2C_MASTER_Val << SERCOM_I2CS_CTRLA_MODE_Pos)
#define SERCOM_I2CS_CTRLA_RUNSTDBY_Pos 7            /**< \brief (SERCOM_I2CS_CTRLA) Run in Standby */
#define SERCOM_I2CS_CTRLA_RUNSTDBY  (_U_(0x1) << SERCOM_I2CS_CTRLA_RUNSTDBY_Pos)
#define SERCOM_I2CS_CTRLA_PINOUT_Pos 16           /**< \brief (SERCOM_I2CS_CTRLA) Pin Usage */
#define SERCOM_I2CS_CTRLA_PINOUT    (_U_(0x1) << SERCOM_I2CS_CTRLA_PINOUT_Pos)
#define SERCOM_I2CS_CTRLA_SDAHOLD_Pos 20           /**< \brief (SERCOM_I2CS_CTRLA) SDA Hold Time */
#define SERCOM_I2CS_CTRLA_SDAHOLD_Msk (_U_(0x3) << SERCOM_I2CS_CTRLA_SDAHOLD_Pos)
#define SERCOM_I2CS_CTRLA_SDAHOLD(value) (SERCOM_I2CS_CTRLA_SDAHOLD_Msk & ((value) << SERCOM_I2CS_CTRLA_SDAHOLD_Pos))
#define   SERCOM_I2CS_CTRLA_SDAHOLD_DIS_Val _U_(0x0)   /**< \brief (SERCOM_I2CS_CTRLA) Disabled */
#define   SERCOM_I2CS_CTRLA_SDAHOLD_75_Val _U_(0x1)   /**< \brief (SERCOM_I2CS_CTRLA) 50-100 ns hold time */
#define   SERCOM_I2CS_CTRLA_SDAHOLD_450_Val _U_(0x2)   /**< \brief (SERCOM_I2CS_CTRLA) 300-600 ns hold time */
#define   SERCOM_I2CS_CTRLA_SDAHOLD_600_Val _U_(0x3)   /**< \brief (SERCOM_I2CS_CTRLA) 400-800 ns hold time */
#define SERCOM_I2CS_CTRLA_SDAHOLD_DIS (SERCOM_I2CS_CTRLA_SDAHOLD_DIS_Val << SERCOM_I2CS_CTRLA_SDAHOLD_Pos)
#define SERCOM_I2CS_CTRLA_SDAHOLD_75 (SERCOM_I2CS_CTRLA_SDAHOLD_75_Val << SERCOM_I2CS_CTRLA_SDAHOLD_Pos)
#define SERCOM_I2CS_CTRLA_SDAHOLD_450 (SERCOM_I2CS_CTRLA_SDAHOLD_450_Val << SERCOM_I2CS_CTRLA_SDAHOLD_Pos)
#define SERCOM_I2CS_CTRLA_SDAHOLD_600 (SERCOM_I2CS_CTRLA_SDAHOLD_600_Val << SERCOM_I2CS_CTRLA_SDAHOLD_Pos)
#define SERCOM_I2CS_CTRLA_LOWTOUT_Pos 30           /**< \brief (SERCOM_I2CS_CTRLA) SCL Low Time-out */
#define SERCOM_I2CS_CTRLA_LOWTOUT   (_U_(0x1) << SERCOM_I2CS_CTRLA_LOWTOUT_Pos)
#define SERCOM_I2CS_CTRLA_MASK      _U_(0x4031009F) /**< \brief (SERCOM_I2CS_CTRLA) MASK Register */

/* -------- SERCOM_SPI_CTRLA : (SERCOM Offset: 0x00) (R/W 32) SPI SPI Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t MODE:3;           /*!< bit:  2.. 4  Operating Mode                     */
    uint32_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      7  Run In Standby                     */
    uint32_t IBON:1;           /*!< bit:      8  Immediate Buffer Overflow Notification */
    uint32_t :7;               /*!< bit:  9..15  Reserved                           */
    uint32_t DOPO:2;           /*!< bit: 16..17  Data Out Pinout                    */
    uint32_t :2;               /*!< bit: 18..19  Reserved                           */
    uint32_t DIPO:2;           /*!< bit: 20..21  Data In Pinout                     */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FORM:4;           /*!< bit: 24..27  Frame Format                       */
    uint32_t CPHA:1;           /*!< bit:     28  Clock Phase                        */
    uint32_t CPOL:1;           /*!< bit:     29  Clock Polarity                     */
    uint32_t DORD:1;           /*!< bit:     30  Data Order                         */
    uint32_t :1;               /*!< bit:     31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_SPI_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_CTRLA_OFFSET     0x00         /**< \brief (SERCOM_SPI_CTRLA offset) SPI Control A */
#define SERCOM_SPI_CTRLA_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_SPI_CTRLA reset_value) SPI Control A */

#define SERCOM_SPI_CTRLA_SWRST_Pos  0            /**< \brief (SERCOM_SPI_CTRLA) Software Reset */
#define SERCOM_SPI_CTRLA_SWRST      (_U_(0x1) << SERCOM_SPI_CTRLA_SWRST_Pos)
#define SERCOM_SPI_CTRLA_ENABLE_Pos 1            /**< \brief (SERCOM_SPI_CTRLA) Enable */
#define SERCOM_SPI_CTRLA_ENABLE     (_U_(0x1) << SERCOM_SPI_CTRLA_ENABLE_Pos)
#define SERCOM_SPI_CTRLA_MODE_Pos   2            /**< \brief (SERCOM_SPI_CTRLA) Operating Mode */
#define SERCOM_SPI_CTRLA_MODE_Msk   (_U_(0x7) << SERCOM_SPI_CTRLA_MODE_Pos)
#define SERCOM_SPI_CTRLA_MODE(value) (SERCOM_SPI_CTRLA_MODE_Msk & ((value) << SERCOM_SPI_CTRLA_MODE_Pos))
#define   SERCOM_SPI_CTRLA_MODE_USART_EXT_CLK_Val _U_(0x0)   /**< \brief (SERCOM_SPI_CTRLA) USART mode with external clock */
#define   SERCOM_SPI_CTRLA_MODE_USART_INT_CLK_Val _U_(0x1)   /**< \brief (SERCOM_SPI_CTRLA) USART mode with internal clock */
#define   SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val _U_(0x2)   /**< \brief (SERCOM_SPI_CTRLA) SPI mode with external clock */
#define   SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val _U_(0x3)   /**< \brief (SERCOM_SPI_CTRLA) SPI mode with internal clock */
#define   SERCOM_SPI_CTRLA_MODE_I2C_SLAVE_Val _U_(0x4)   /**< \brief (SERCOM_SPI_CTRLA) I2C mode with external clock */
#define   SERCOM_SPI_CTRLA_MODE_I2C_MASTER_Val _U_(0x5)   /**< \brief (SERCOM_SPI_CTRLA) I2C mode with internal clock */
#define SERCOM_SPI_CTRLA_MODE_USART_EXT_CLK (SERCOM_SPI_CTRLA_MODE_USART_EXT_CLK_Val << SERCOM_SPI_CTRLA_MODE_Pos)
#define SERCOM_SPI_CTRLA_MODE_USART_INT_CLK (SERCOM_SPI_CTRLA_MODE_USART_INT_CLK_Val << SERCOM_SPI_CTRLA_MODE_Pos)
#define SERCOM_SPI_CTRLA_MODE_SPI_SLAVE (SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val << SERCOM_SPI_CTRLA_MODE_Pos)
#define SERCOM_SPI_CTRLA_MODE_SPI_MASTER (SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val << SERCOM_SPI_CTRLA_MODE_Pos)
#define SERCOM_SPI_CTRLA_MODE_I2C_SLAVE (SERCOM_SPI_CTRLA_MODE_I2C_SLAVE_Val << SERCOM_SPI_CTRLA_MODE_Pos)
#define SERCOM_SPI_CTRLA_MODE_I2C_MASTER (SERCOM_SPI_CTRLA_MODE_I2C_MASTER_Val << SERCOM_SPI_CTRLA_MODE_Pos)
#define SERCOM_SPI_CTRLA_RUNSTDBY_Pos 7            /**< \brief (SERCOM_SPI_CTRLA) Run In Standby */
#define SERCOM_SPI_CTRLA_RUNSTDBY   (_U_(0x1) << SERCOM_SPI_CTRLA_RUNSTDBY_Pos)
#define SERCOM_SPI_CTRLA_IBON_Pos   8            /**< \brief (SERCOM_SPI_CTRLA) Immediate Buffer Overflow Notification */
#define SERCOM_SPI_CTRLA_IBON       (_U_(0x1) << SERCOM_SPI_CTRLA_IBON_Pos)
#define SERCOM_SPI_CTRLA_DOPO_Pos   16           /**< \brief (SERCOM_SPI_CTRLA) Data Out Pinout */
#define SERCOM_SPI_CTRLA_DOPO_Msk   (_U_(0x3) << SERCOM_SPI_CTRLA_DOPO_Pos)
#define SERCOM_SPI_CTRLA_DOPO(value) (SERCOM_SPI_CTRLA_DOPO_Msk & ((value) << SERCOM_SPI_CTRLA_DOPO_Pos))
#define SERCOM_SPI_CTRLA_DIPO_Pos   20           /**< \brief (SERCOM_SPI_CTRLA) Data In Pinout */
#define SERCOM_SPI_CTRLA_DIPO_Msk   (_U_(0x3) << SERCOM_SPI_CTRLA_DIPO_Pos)
#define SERCOM_SPI_CTRLA_DIPO(value) (SERCOM_SPI_CTRLA_DIPO_Msk & ((value) << SERCOM_SPI_CTRLA_DIPO_Pos))
#define SERCOM_SPI_CTRLA_FORM_Pos   24           /**< \brief (SERCOM_SPI_CTRLA) Frame Format */
#define SERCOM_SPI_CTRLA_FORM_Msk   (_U_(0xF) << SERCOM_SPI_CTRLA_FORM_Pos)
#define SERCOM_SPI_CTRLA_FORM(value) (SERCOM_SPI_CTRLA_FORM_Msk & ((value) << SERCOM_SPI_CTRLA_FORM_Pos))
#define   SERCOM_SPI_CTRLA_FORM_SPI_Val   _U_(0x0)   /**< \brief (SERCOM_SPI_CTRLA) SPI frame */
#define   SERCOM_SPI_CTRLA_FORM_SPI_ADDR_Val _U_(0x2)   /**< \brief (SERCOM_SPI_CTRLA) SPI frame with address */
#define SERCOM_SPI_CTRLA_FORM_SPI   (SERCOM_SPI_CTRLA_FORM_SPI_Val << SERCOM_SPI_CTRLA_FORM_Pos)
#define SERCOM_SPI_CTRLA_FORM_SPI_ADDR (SERCOM_SPI_CTRLA_FORM_SPI_ADDR_Val << SERCOM_SPI_CTRLA_FORM_Pos)
#define SERCOM_SPI_CTRLA_CPHA_Pos   28           /**< \brief (SERCOM_SPI_CTRLA) Clock Phase */
#define SERCOM_SPI_CTRLA_CPHA       (_U_(0x1) << SERCOM_SPI_CTRLA_CPHA_Pos)
#define SERCOM_SPI_CTRLA_CPOL_Pos   29           /**< \brief (SERCOM_SPI_CTRLA) Clock Polarity */
#define SERCOM_SPI_CTRLA_CPOL       (_U_(0x1) << SERCOM_SPI_CTRLA_CPOL_Pos)
#define SERCOM_SPI_CTRLA_DORD_Pos   30           /**< \brief (SERCOM_SPI_CTRLA) Data Order */
#define SERCOM_SPI_CTRLA_DORD       (_U_(0x1) << SERCOM_SPI_CTRLA_DORD_Pos)
#define SERCOM_SPI_CTRLA_MASK       _U_(0x7F33019F) /**< \brief (SERCOM_SPI_CTRLA) MASK Register */

/* -------- SERCOM_USART_CTRLA : (SERCOM Offset: 0x00) (R/W 32) USART USART Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t MODE:3;           /*!< bit:  2.. 4  Operating Mode                     */
    uint32_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      7  Run In Standby                     */
    uint32_t IBON:1;           /*!< bit:      8  Immediate Buffer Overflow Notification */
    uint32_t :7;               /*!< bit:  9..15  Reserved                           */
    uint32_t TXPO:1;           /*!< bit:     16  Transmit Data Pinout               */
    uint32_t :3;               /*!< bit: 17..19  Reserved                           */
    uint32_t RXPO:2;           /*!< bit: 20..21  Receive Data Pinout                */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FORM:4;           /*!< bit: 24..27  Frame Format                       */
    uint32_t CMODE:1;          /*!< bit:     28  Communication Mode                 */
    uint32_t CPOL:1;           /*!< bit:     29  Clock Polarity                     */
    uint32_t DORD:1;           /*!< bit:     30  Data Order                         */
    uint32_t :1;               /*!< bit:     31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_USART_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_CTRLA_OFFSET   0x00         /**< \brief (SERCOM_USART_CTRLA offset) USART Control A */
#define SERCOM_USART_CTRLA_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_USART_CTRLA reset_value) USART Control A */

#define SERCOM_USART_CTRLA_SWRST_Pos 0            /**< \brief (SERCOM_USART_CTRLA) Software Reset */
#define SERCOM_USART_CTRLA_SWRST    (_U_(0x1) << SERCOM_USART_CTRLA_SWRST_Pos)
#define SERCOM_USART_CTRLA_ENABLE_Pos 1            /**< \brief (SERCOM_USART_CTRLA) Enable */
#define SERCOM_USART_CTRLA_ENABLE   (_U_(0x1) << SERCOM_USART_CTRLA_ENABLE_Pos)
#define SERCOM_USART_CTRLA_MODE_Pos 2            /**< \brief (SERCOM_USART_CTRLA) Operating Mode */
#define SERCOM_USART_CTRLA_MODE_Msk (_U_(0x7) << SERCOM_USART_CTRLA_MODE_Pos)
#define SERCOM_USART_CTRLA_MODE(value) (SERCOM_USART_CTRLA_MODE_Msk & ((value) << SERCOM_USART_CTRLA_MODE_Pos))
#define   SERCOM_USART_CTRLA_MODE_USART_EXT_CLK_Val _U_(0x0)   /**< \brief (SERCOM_USART_CTRLA) USART mode with external clock */
#define   SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val _U_(0x1)   /**< \brief (SERCOM_USART_CTRLA) USART mode with internal clock */
#define   SERCOM_USART_CTRLA_MODE_SPI_SLAVE_Val _U_(0x2)   /**< \brief (SERCOM_USART_CTRLA) SPI mode with external clock */
#define   SERCOM_USART_CTRLA_MODE_SPI_MASTER_Val _U_(0x3)   /**< \brief (SERCOM_USART_CTRLA) SPI mode with internal clock */
#define   SERCOM_USART_CTRLA_MODE_I2C_SLAVE_Val _U_(0x4)   /**< \brief (SERCOM_USART_CTRLA) I2C mode with external clock */
#define   SERCOM_USART_CTRLA_MODE_I2C_MASTER_Val _U_(0x5)   /**< \brief (SERCOM_USART_CTRLA) I2C mode with internal clock */
#define SERCOM_USART_CTRLA_MODE_USART_EXT_CLK (SERCOM_USART_CTRLA_MODE_USART_EXT_CLK_Val << SERCOM_USART_CTRLA_MODE_Pos)
#define SERCOM_USART_CTRLA_MODE_USART_INT_CLK (SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val << SERCOM_USART_CTRLA_MODE_Pos)
#define SERCOM_USART_CTRLA_MODE_SPI_SLAVE (SERCOM_USART_CTRLA_MODE_SPI_SLAVE_Val << SERCOM_USART_CTRLA_MODE_Pos)
#define SERCOM_USART_CTRLA_MODE_SPI_MASTER (SERCOM_USART_CTRLA_MODE_SPI_MASTER_Val << SERCOM_USART_CTRLA_MODE_Pos)
#define SERCOM_USART_CTRLA_MODE_I2C_SLAVE (SERCOM_USART_CTRLA_MODE_I2C_SLAVE_Val << SERCOM_USART_CTRLA_MODE_Pos)
#define SERCOM_USART_CTRLA_MODE_I2C_MASTER (SERCOM_USART_CTRLA_MODE_I2C_MASTER_Val << SERCOM_USART_CTRLA_MODE_Pos)
#define SERCOM_USART_CTRLA_RUNSTDBY_Pos 7            /**< \brief (SERCOM_USART_CTRLA) Run In Standby */
#define SERCOM_USART_CTRLA_RUNSTDBY (_U_(0x1) << SERCOM_USART_CTRLA_RUNSTDBY_Pos)
#define SERCOM_USART_CTRLA_IBON_Pos 8            /**< \brief (SERCOM_USART_CTRLA) Immediate Buffer Overflow Notification */
#define SERCOM_USART_CTRLA_IBON     (_U_(0x1) << SERCOM_USART_CTRLA_IBON_Pos)
#define SERCOM_USART_CTRLA_TXPO_Pos 16           /**< \brief (SERCOM_USART_CTRLA) Transmit Data Pinout */
#define SERCOM_USART_CTRLA_TXPO     (_U_(0x1) << SERCOM_USART_CTRLA_TXPO_Pos)
#define   SERCOM_USART_CTRLA_TXPO_PAD0_Val _U_(0x0)   /**< \brief (SERCOM_USART_CTRLA) TXD at PAD0, XCK at PAD1 */
#define   SERCOM_USART_CTRLA_TXPO_PAD2_Val _U_(0x1)   /**< \brief (SERCOM_USART_CTRLA) TXD at PAD2, XCK at PAD3 */
#define SERCOM_USART_CTRLA_TXPO_PAD0 (SERCOM_USART_CTRLA_TXPO_PAD0_Val << SERCOM_USART_CTRLA_TXPO_Pos)
#define SERCOM_USART_CTRLA_TXPO_PAD2 (SERCOM_USART_CTRLA_TXPO_PAD2_Val << SERCOM_USART_CTRLA_TXPO_Pos)
#define SERCOM_USART_CTRLA_RXPO_Pos 20           /**< \brief (SERCOM_USART_CTRLA) Receive Data Pinout */
#define SERCOM_USART_CTRLA_RXPO_Msk (_U_(0x3) << SERCOM_USART_CTRLA_RXPO_Pos)
#define SERCOM_USART_CTRLA_RXPO(value) (SERCOM_USART_CTRLA_RXPO_Msk & ((value) << SERCOM_USART_CTRLA_RXPO_Pos))
#define   SERCOM_USART_CTRLA_RXPO_PAD0_Val _U_(0x0)   /**< \brief (SERCOM_USART_CTRLA) SERCOM_PAD0 */
#define   SERCOM_USART_CTRLA_RXPO_PAD1_Val _U_(0x1)   /**< \brief (SERCOM_USART_CTRLA) SERCOM_PAD1 */
#define   SERCOM_USART_CTRLA_RXPO_PAD2_Val _U_(0x2)   /**< \brief (SERCOM_USART_CTRLA) SERCOM_PAD2 */
#define   SERCOM_USART_CTRLA_RXPO_PAD3_Val _U_(0x3)   /**< \brief (SERCOM_USART_CTRLA) SERCOM_PAD3 */
#define SERCOM_USART_CTRLA_RXPO_PAD0 (SERCOM_USART_CTRLA_RXPO_PAD0_Val << SERCOM_USART_CTRLA_RXPO_Pos)
#define SERCOM_USART_CTRLA_RXPO_PAD1 (SERCOM_USART_CTRLA_RXPO_PAD1_Val << SERCOM_USART_CTRLA_RXPO_Pos)
#define SERCOM_USART_CTRLA_RXPO_PAD2 (SERCOM_USART_CTRLA_RXPO_PAD2_Val << SERCOM_USART_CTRLA_RXPO_Pos)
#define SERCOM_USART_CTRLA_RXPO_PAD3 (SERCOM_USART_CTRLA_RXPO_PAD3_Val << SERCOM_USART_CTRLA_RXPO_Pos)
#define SERCOM_USART_CTRLA_FORM_Pos 24           /**< \brief (SERCOM_USART_CTRLA) Frame Format */
#define SERCOM_USART_CTRLA_FORM_Msk (_U_(0xF) << SERCOM_USART_CTRLA_FORM_Pos)
#define SERCOM_USART_CTRLA_FORM(value) (SERCOM_USART_CTRLA_FORM_Msk & ((value) << SERCOM_USART_CTRLA_FORM_Pos))
#define   SERCOM_USART_CTRLA_FORM_0_Val   _U_(0x0)   /**< \brief (SERCOM_USART_CTRLA) USART frame */
#define   SERCOM_USART_CTRLA_FORM_1_Val   _U_(0x1)   /**< \brief (SERCOM_USART_CTRLA) USART frame with parity */
#define SERCOM_USART_CTRLA_FORM_0   (SERCOM_USART_CTRLA_FORM_0_Val << SERCOM_USART_CTRLA_FORM_Pos)
#define SERCOM_USART_CTRLA_FORM_1   (SERCOM_USART_CTRLA_FORM_1_Val << SERCOM_USART_CTRLA_FORM_Pos)
#define SERCOM_USART_CTRLA_CMODE_Pos 28           /**< \brief (SERCOM_USART_CTRLA) Communication Mode */
#define SERCOM_USART_CTRLA_CMODE    (_U_(0x1) << SERCOM_USART_CTRLA_CMODE_Pos)
#define SERCOM_USART_CTRLA_CPOL_Pos 29           /**< \brief (SERCOM_USART_CTRLA) Clock Polarity */
#define SERCOM_USART_CTRLA_CPOL     (_U_(0x1) << SERCOM_USART_CTRLA_CPOL_Pos)
#define SERCOM_USART_CTRLA_DORD_Pos 30           /**< \brief (SERCOM_USART_CTRLA) Data Order */
#define SERCOM_USART_CTRLA_DORD     (_U_(0x1) << SERCOM_USART_CTRLA_DORD_Pos)
#define SERCOM_USART_CTRLA_MASK     _U_(0x7F31019F) /**< \brief (SERCOM_USART_CTRLA) MASK Register */

/* -------- SERCOM_I2CM_CTRLB : (SERCOM Offset: 0x04) (R/W 32) I2CM I2CM Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :8;               /*!< bit:  0.. 7  Reserved                           */
    uint32_t SMEN:1;           /*!< bit:      8  Smart Mode Enable                  */
    uint32_t QCEN:1;           /*!< bit:      9  Quick Command Enable               */
    uint32_t :6;               /*!< bit: 10..15  Reserved                           */
    uint32_t CMD:2;            /*!< bit: 16..17  Command                            */
    uint32_t ACKACT:1;         /*!< bit:     18  Acknowledge Action                 */
    uint32_t :13;              /*!< bit: 19..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CM_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_CTRLB_OFFSET    0x04         /**< \brief (SERCOM_I2CM_CTRLB offset) I2CM Control B */
#define SERCOM_I2CM_CTRLB_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_I2CM_CTRLB reset_value) I2CM Control B */

#define SERCOM_I2CM_CTRLB_SMEN_Pos  8            /**< \brief (SERCOM_I2CM_CTRLB) Smart Mode Enable */
#define SERCOM_I2CM_CTRLB_SMEN      (_U_(0x1) << SERCOM_I2CM_CTRLB_SMEN_Pos)
#define SERCOM_I2CM_CTRLB_QCEN_Pos  9            /**< \brief (SERCOM_I2CM_CTRLB) Quick Command Enable */
#define SERCOM_I2CM_CTRLB_QCEN      (_U_(0x1) << SERCOM_I2CM_CTRLB_QCEN_Pos)
#define SERCOM_I2CM_CTRLB_CMD_Pos   16           /**< \brief (SERCOM_I2CM_CTRLB) Command */
#define SERCOM_I2CM_CTRLB_CMD_Msk   (_U_(0x3) << SERCOM_I2CM_CTRLB_CMD_Pos)
#define SERCOM_I2CM_CTRLB_CMD(value) (SERCOM_I2CM_CTRLB_CMD_Msk & ((value) << SERCOM_I2CM_CTRLB_CMD_Pos))
#define SERCOM_I2CM_CTRLB_ACKACT_Pos 18           /**< \brief (SERCOM_I2CM_CTRLB) Acknowledge Action */
#define SERCOM_I2CM_CTRLB_ACKACT    (_U_(0x1) << SERCOM_I2CM_CTRLB_ACKACT_Pos)
#define SERCOM_I2CM_CTRLB_MASK      _U_(0x00070300) /**< \brief (SERCOM_I2CM_CTRLB) MASK Register */

/* -------- SERCOM_I2CS_CTRLB : (SERCOM Offset: 0x04) (R/W 32) I2CS I2CS Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :8;               /*!< bit:  0.. 7  Reserved                           */
    uint32_t SMEN:1;           /*!< bit:      8  Smart Mode Enable                  */
    uint32_t :5;               /*!< bit:  9..13  Reserved                           */
    uint32_t AMODE:2;          /*!< bit: 14..15  Address Mode                       */
    uint32_t CMD:2;            /*!< bit: 16..17  Command                            */
    uint32_t ACKACT:1;         /*!< bit:     18  Acknowledge Action                 */
    uint32_t :13;              /*!< bit: 19..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CS_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_CTRLB_OFFSET    0x04         /**< \brief (SERCOM_I2CS_CTRLB offset) I2CS Control B */
#define SERCOM_I2CS_CTRLB_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_I2CS_CTRLB reset_value) I2CS Control B */

#define SERCOM_I2CS_CTRLB_SMEN_Pos  8            /**< \brief (SERCOM_I2CS_CTRLB) Smart Mode Enable */
#define SERCOM_I2CS_CTRLB_SMEN      (_U_(0x1) << SERCOM_I2CS_CTRLB_SMEN_Pos)
#define SERCOM_I2CS_CTRLB_AMODE_Pos 14           /**< \brief (SERCOM_I2CS_CTRLB) Address Mode */
#define SERCOM_I2CS_CTRLB_AMODE_Msk (_U_(0x3) << SERCOM_I2CS_CTRLB_AMODE_Pos)
#define SERCOM_I2CS_CTRLB_AMODE(value) (SERCOM_I2CS_CTRLB_AMODE_Msk & ((value) << SERCOM_I2CS_CTRLB_AMODE_Pos))
#define SERCOM_I2CS_CTRLB_CMD_Pos   16           /**< \brief (SERCOM_I2CS_CTRLB) Command */
#define SERCOM_I2CS_CTRLB_CMD_Msk   (_U_(0x3) << SERCOM_I2CS_CTRLB_CMD_Pos)
#define SERCOM_I2CS_CTRLB_CMD(value) (SERCOM_I2CS_CTRLB_CMD_Msk & ((value) << SERCOM_I2CS_CTRLB_CMD_Pos))
#define SERCOM_I2CS_CTRLB_ACKACT_Pos 18           /**< \brief (SERCOM_I2CS_CTRLB) Acknowledge Action */
#define SERCOM_I2CS_CTRLB_ACKACT    (_U_(0x1) << SERCOM_I2CS_CTRLB_ACKACT_Pos)
#define SERCOM_I2CS_CTRLB_MASK      _U_(0x0007C100) /**< \brief (SERCOM_I2CS_CTRLB) MASK Register */

/* -------- SERCOM_SPI_CTRLB : (SERCOM Offset: 0x04) (R/W 32) SPI SPI Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CHSIZE:3;         /*!< bit:  0.. 2  Character Size                     */
    uint32_t :3;               /*!< bit:  3.. 5  Reserved                           */
    uint32_t PLOADEN:1;        /*!< bit:      6  Slave Data Preload Enable          */
    uint32_t :7;               /*!< bit:  7..13  Reserved                           */
    uint32_t AMODE:2;          /*!< bit: 14..15  Address Mode                       */
    uint32_t :1;               /*!< bit:     16  Reserved                           */
    uint32_t RXEN:1;           /*!< bit:     17  Receiver Enable                    */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_SPI_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_CTRLB_OFFSET     0x04         /**< \brief (SERCOM_SPI_CTRLB offset) SPI Control B */
#define SERCOM_SPI_CTRLB_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_SPI_CTRLB reset_value) SPI Control B */

#define SERCOM_SPI_CTRLB_CHSIZE_Pos 0            /**< \brief (SERCOM_SPI_CTRLB) Character Size */
#define SERCOM_SPI_CTRLB_CHSIZE_Msk (_U_(0x7) << SERCOM_SPI_CTRLB_CHSIZE_Pos)
#define SERCOM_SPI_CTRLB_CHSIZE(value) (SERCOM_SPI_CTRLB_CHSIZE_Msk & ((value) << SERCOM_SPI_CTRLB_CHSIZE_Pos))
#define SERCOM_SPI_CTRLB_PLOADEN_Pos 6            /**< \brief (SERCOM_SPI_CTRLB) Slave Data Preload Enable */
#define SERCOM_SPI_CTRLB_PLOADEN    (_U_(0x1) << SERCOM_SPI_CTRLB_PLOADEN_Pos)
#define SERCOM_SPI_CTRLB_AMODE_Pos  14           /**< \brief (SERCOM_SPI_CTRLB) Address Mode */
#define SERCOM_SPI_CTRLB_AMODE_Msk  (_U_(0x3) << SERCOM_SPI_CTRLB_AMODE_Pos)
#define SERCOM_SPI_CTRLB_AMODE(value) (SERCOM_SPI_CTRLB_AMODE_Msk & ((value) << SERCOM_SPI_CTRLB_AMODE_Pos))
#define   SERCOM_SPI_CTRLB_AMODE_MASK_Val _U_(0x0)   /**< \brief (SERCOM_SPI_CTRLB) ADDRMASK is used as a mask to the ADDR register. */
#define   SERCOM_SPI_CTRLB_AMODE_2ADDR_Val _U_(0x1)   /**< \brief (SERCOM_SPI_CTRLB) The slave responds to the 2 unique addresses in ADDR and ADDRMASK. */
#define   SERCOM_SPI_CTRLB_AMODE_RANGE_Val _U_(0x2)   /**< \brief (SERCOM_SPI_CTRLB) The slave responds to the range of addresses between and including ADDR and ADDRMASK. ADDR is the upper limit. */
#define SERCOM_SPI_CTRLB_AMODE_MASK (SERCOM_SPI_CTRLB_AMODE_MASK_Val << SERCOM_SPI_CTRLB_AMODE_Pos)
#define SERCOM_SPI_CTRLB_AMODE_2ADDR (SERCOM_SPI_CTRLB_AMODE_2ADDR_Val << SERCOM_SPI_CTRLB_AMODE_Pos)
#define SERCOM_SPI_CTRLB_AMODE_RANGE (SERCOM_SPI_CTRLB_AMODE_RANGE_Val << SERCOM_SPI_CTRLB_AMODE_Pos)
#define SERCOM_SPI_CTRLB_RXEN_Pos   17           /**< \brief (SERCOM_SPI_CTRLB) Receiver Enable */
#define SERCOM_SPI_CTRLB_RXEN       (_U_(0x1) << SERCOM_SPI_CTRLB_RXEN_Pos)
#define SERCOM_SPI_CTRLB_MASK       _U_(0x0002C047) /**< \brief (SERCOM_SPI_CTRLB) MASK Register */

/* -------- SERCOM_USART_CTRLB : (SERCOM Offset: 0x04) (R/W 32) USART USART Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CHSIZE:3;         /*!< bit:  0.. 2  Character Size                     */
    uint32_t :3;               /*!< bit:  3.. 5  Reserved                           */
    uint32_t SBMODE:1;         /*!< bit:      6  Stop Bit Mode                      */
    uint32_t :2;               /*!< bit:  7.. 8  Reserved                           */
    uint32_t SFDE:1;           /*!< bit:      9  Start of Frame Detection Enable    */
    uint32_t :3;               /*!< bit: 10..12  Reserved                           */
    uint32_t PMODE:1;          /*!< bit:     13  Parity Mode                        */
    uint32_t :2;               /*!< bit: 14..15  Reserved                           */
    uint32_t TXEN:1;           /*!< bit:     16  Transmitter Enable                 */
    uint32_t RXEN:1;           /*!< bit:     17  Receiver Enable                    */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_USART_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_CTRLB_OFFSET   0x04         /**< \brief (SERCOM_USART_CTRLB offset) USART Control B */
#define SERCOM_USART_CTRLB_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_USART_CTRLB reset_value) USART Control B */

#define SERCOM_USART_CTRLB_CHSIZE_Pos 0            /**< \brief (SERCOM_USART_CTRLB) Character Size */
#define SERCOM_USART_CTRLB_CHSIZE_Msk (_U_(0x7) << SERCOM_USART_CTRLB_CHSIZE_Pos)
#define SERCOM_USART_CTRLB_CHSIZE(value) (SERCOM_USART_CTRLB_CHSIZE_Msk & ((value) << SERCOM_USART_CTRLB_CHSIZE_Pos))
#define SERCOM_USART_CTRLB_SBMODE_Pos 6            /**< \brief (SERCOM_USART_CTRLB) Stop Bit Mode */
#define SERCOM_USART_CTRLB_SBMODE   (_U_(0x1) << SERCOM_USART_CTRLB_SBMODE_Pos)
#define SERCOM_USART_CTRLB_SFDE_Pos 9            /**< \brief (SERCOM_USART_CTRLB) Start of Frame Detection Enable */
#define SERCOM_USART_CTRLB_SFDE     (_U_(0x1) << SERCOM_USART_CTRLB_SFDE_Pos)
#define SERCOM_USART_CTRLB_PMODE_Pos 13           /**< \brief (SERCOM_USART_CTRLB) Parity Mode */
#define SERCOM_USART_CTRLB_PMODE    (_U_(0x1) << SERCOM_USART_CTRLB_PMODE_Pos)
#define SERCOM_USART_CTRLB_TXEN_Pos 16           /**< \brief (SERCOM_USART_CTRLB) Transmitter Enable */
#define SERCOM_USART_CTRLB_TXEN     (_U_(0x1) << SERCOM_USART_CTRLB_TXEN_Pos)
#define SERCOM_USART_CTRLB_RXEN_Pos 17           /**< \brief (SERCOM_USART_CTRLB) Receiver Enable */
#define SERCOM_USART_CTRLB_RXEN     (_U_(0x1) << SERCOM_USART_CTRLB_RXEN_Pos)
#define SERCOM_USART_CTRLB_MASK     _U_(0x00032247) /**< \brief (SERCOM_USART_CTRLB) MASK Register */

/* -------- SERCOM_I2CM_DBGCTRL : (SERCOM Offset: 0x08) (R/W  8) I2CM I2CM Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGSTOP:1;        /*!< bit:      0  Debug Stop Mode                    */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CM_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_DBGCTRL_OFFSET  0x08         /**< \brief (SERCOM_I2CM_DBGCTRL offset) I2CM Debug Control */
#define SERCOM_I2CM_DBGCTRL_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CM_DBGCTRL reset_value) I2CM Debug Control */

#define SERCOM_I2CM_DBGCTRL_DBGSTOP_Pos 0            /**< \brief (SERCOM_I2CM_DBGCTRL) Debug Stop Mode */
#define SERCOM_I2CM_DBGCTRL_DBGSTOP (_U_(0x1) << SERCOM_I2CM_DBGCTRL_DBGSTOP_Pos)
#define SERCOM_I2CM_DBGCTRL_MASK    _U_(0x01)    /**< \brief (SERCOM_I2CM_DBGCTRL) MASK Register */

/* -------- SERCOM_SPI_DBGCTRL : (SERCOM Offset: 0x08) (R/W  8) SPI SPI Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGSTOP:1;        /*!< bit:      0  Debug Stop Mode                    */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_SPI_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_DBGCTRL_OFFSET   0x08         /**< \brief (SERCOM_SPI_DBGCTRL offset) SPI Debug Control */
#define SERCOM_SPI_DBGCTRL_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_SPI_DBGCTRL reset_value) SPI Debug Control */

#define SERCOM_SPI_DBGCTRL_DBGSTOP_Pos 0            /**< \brief (SERCOM_SPI_DBGCTRL) Debug Stop Mode */
#define SERCOM_SPI_DBGCTRL_DBGSTOP  (_U_(0x1) << SERCOM_SPI_DBGCTRL_DBGSTOP_Pos)
#define SERCOM_SPI_DBGCTRL_MASK     _U_(0x01)    /**< \brief (SERCOM_SPI_DBGCTRL) MASK Register */

/* -------- SERCOM_USART_DBGCTRL : (SERCOM Offset: 0x08) (R/W  8) USART USART Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGSTOP:1;        /*!< bit:      0  Debug Stop Mode                    */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_USART_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_DBGCTRL_OFFSET 0x08         /**< \brief (SERCOM_USART_DBGCTRL offset) USART Debug Control */
#define SERCOM_USART_DBGCTRL_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_USART_DBGCTRL reset_value) USART Debug Control */

#define SERCOM_USART_DBGCTRL_DBGSTOP_Pos 0            /**< \brief (SERCOM_USART_DBGCTRL) Debug Stop Mode */
#define SERCOM_USART_DBGCTRL_DBGSTOP (_U_(0x1) << SERCOM_USART_DBGCTRL_DBGSTOP_Pos)
#define SERCOM_USART_DBGCTRL_MASK   _U_(0x01)    /**< \brief (SERCOM_USART_DBGCTRL) MASK Register */

/* -------- SERCOM_I2CM_BAUD : (SERCOM Offset: 0x0A) (R/W 16) I2CM I2CM Baud Rate -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BAUD:8;           /*!< bit:  0.. 7  Master Baud Rate                   */
    uint16_t BAUDLOW:8;        /*!< bit:  8..15  Master Baud Rate Low               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CM_BAUD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_BAUD_OFFSET     0x0A         /**< \brief (SERCOM_I2CM_BAUD offset) I2CM Baud Rate */
#define SERCOM_I2CM_BAUD_RESETVALUE _U_(0x0000)  /**< \brief (SERCOM_I2CM_BAUD reset_value) I2CM Baud Rate */

#define SERCOM_I2CM_BAUD_BAUD_Pos   0            /**< \brief (SERCOM_I2CM_BAUD) Master Baud Rate */
#define SERCOM_I2CM_BAUD_BAUD_Msk   (_U_(0xFF) << SERCOM_I2CM_BAUD_BAUD_Pos)
#define SERCOM_I2CM_BAUD_BAUD(value) (SERCOM_I2CM_BAUD_BAUD_Msk & ((value) << SERCOM_I2CM_BAUD_BAUD_Pos))
#define SERCOM_I2CM_BAUD_BAUDLOW_Pos 8            /**< \brief (SERCOM_I2CM_BAUD) Master Baud Rate Low */
#define SERCOM_I2CM_BAUD_BAUDLOW_Msk (_U_(0xFF) << SERCOM_I2CM_BAUD_BAUDLOW_Pos)
#define SERCOM_I2CM_BAUD_BAUDLOW(value) (SERCOM_I2CM_BAUD_BAUDLOW_Msk & ((value) << SERCOM_I2CM_BAUD_BAUDLOW_Pos))
#define SERCOM_I2CM_BAUD_MASK       _U_(0xFFFF)  /**< \brief (SERCOM_I2CM_BAUD) MASK Register */

/* -------- SERCOM_SPI_BAUD : (SERCOM Offset: 0x0A) (R/W  8) SPI SPI Baud Rate -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  BAUD:8;           /*!< bit:  0.. 7  Baud Register                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_SPI_BAUD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_BAUD_OFFSET      0x0A         /**< \brief (SERCOM_SPI_BAUD offset) SPI Baud Rate */
#define SERCOM_SPI_BAUD_RESETVALUE  _U_(0x00)    /**< \brief (SERCOM_SPI_BAUD reset_value) SPI Baud Rate */

#define SERCOM_SPI_BAUD_BAUD_Pos    0            /**< \brief (SERCOM_SPI_BAUD) Baud Register */
#define SERCOM_SPI_BAUD_BAUD_Msk    (_U_(0xFF) << SERCOM_SPI_BAUD_BAUD_Pos)
#define SERCOM_SPI_BAUD_BAUD(value) (SERCOM_SPI_BAUD_BAUD_Msk & ((value) << SERCOM_SPI_BAUD_BAUD_Pos))
#define SERCOM_SPI_BAUD_MASK        _U_(0xFF)    /**< \brief (SERCOM_SPI_BAUD) MASK Register */

/* -------- SERCOM_USART_BAUD : (SERCOM Offset: 0x0A) (R/W 16) USART USART Baud -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BAUD:16;          /*!< bit:  0..15  Baud Value                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_USART_BAUD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_BAUD_OFFSET    0x0A         /**< \brief (SERCOM_USART_BAUD offset) USART Baud */
#define SERCOM_USART_BAUD_RESETVALUE _U_(0x0000)  /**< \brief (SERCOM_USART_BAUD reset_value) USART Baud */

#define SERCOM_USART_BAUD_BAUD_Pos  0            /**< \brief (SERCOM_USART_BAUD) Baud Value */
#define SERCOM_USART_BAUD_BAUD_Msk  (_U_(0xFFFF) << SERCOM_USART_BAUD_BAUD_Pos)
#define SERCOM_USART_BAUD_BAUD(value) (SERCOM_USART_BAUD_BAUD_Msk & ((value) << SERCOM_USART_BAUD_BAUD_Pos))
#define SERCOM_USART_BAUD_MASK      _U_(0xFFFF)  /**< \brief (SERCOM_USART_BAUD) MASK Register */

/* -------- SERCOM_I2CM_INTENCLR : (SERCOM Offset: 0x0C) (R/W  8) I2CM I2CM Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  MB:1;             /*!< bit:      0  Master on Bus Interrupt Enable     */
    uint8_t  SB:1;             /*!< bit:      1  Slave on Bus Interrupt Enable      */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CM_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_INTENCLR_OFFSET 0x0C         /**< \brief (SERCOM_I2CM_INTENCLR offset) I2CM Interrupt Enable Clear */
#define SERCOM_I2CM_INTENCLR_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CM_INTENCLR reset_value) I2CM Interrupt Enable Clear */

#define SERCOM_I2CM_INTENCLR_MB_Pos 0            /**< \brief (SERCOM_I2CM_INTENCLR) Master on Bus Interrupt Enable */
#define SERCOM_I2CM_INTENCLR_MB     (_U_(0x1) << SERCOM_I2CM_INTENCLR_MB_Pos)
#define SERCOM_I2CM_INTENCLR_SB_Pos 1            /**< \brief (SERCOM_I2CM_INTENCLR) Slave on Bus Interrupt Enable */
#define SERCOM_I2CM_INTENCLR_SB     (_U_(0x1) << SERCOM_I2CM_INTENCLR_SB_Pos)
#define SERCOM_I2CM_INTENCLR_MASK   _U_(0x03)    /**< \brief (SERCOM_I2CM_INTENCLR) MASK Register */

/* -------- SERCOM_I2CS_INTENCLR : (SERCOM Offset: 0x0C) (R/W  8) I2CS I2CS Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PREC:1;           /*!< bit:      0  Stop Received Interrupt Enable     */
    uint8_t  AMATCH:1;         /*!< bit:      1  Address Match Interrupt Enable     */
    uint8_t  DRDY:1;           /*!< bit:      2  Data Ready Interrupt Enable        */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CS_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_INTENCLR_OFFSET 0x0C         /**< \brief (SERCOM_I2CS_INTENCLR offset) I2CS Interrupt Enable Clear */
#define SERCOM_I2CS_INTENCLR_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CS_INTENCLR reset_value) I2CS Interrupt Enable Clear */

#define SERCOM_I2CS_INTENCLR_PREC_Pos 0            /**< \brief (SERCOM_I2CS_INTENCLR) Stop Received Interrupt Enable */
#define SERCOM_I2CS_INTENCLR_PREC   (_U_(0x1) << SERCOM_I2CS_INTENCLR_PREC_Pos)
#define SERCOM_I2CS_INTENCLR_AMATCH_Pos 1            /**< \brief (SERCOM_I2CS_INTENCLR) Address Match Interrupt Enable */
#define SERCOM_I2CS_INTENCLR_AMATCH (_U_(0x1) << SERCOM_I2CS_INTENCLR_AMATCH_Pos)
#define SERCOM_I2CS_INTENCLR_DRDY_Pos 2            /**< \brief (SERCOM_I2CS_INTENCLR) Data Ready Interrupt Enable */
#define SERCOM_I2CS_INTENCLR_DRDY   (_U_(0x1) << SERCOM_I2CS_INTENCLR_DRDY_Pos)
#define SERCOM_I2CS_INTENCLR_MASK   _U_(0x07)    /**< \brief (SERCOM_I2CS_INTENCLR) MASK Register */

/* -------- SERCOM_SPI_INTENCLR : (SERCOM Offset: 0x0C) (R/W  8) SPI SPI Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DRE:1;            /*!< bit:      0  Data Register Empty Interrupt Enable */
    uint8_t  TXC:1;            /*!< bit:      1  Transmit Complete Interrupt Enable */
    uint8_t  RXC:1;            /*!< bit:      2  Receive Complete Interrupt Enable  */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_SPI_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_INTENCLR_OFFSET  0x0C         /**< \brief (SERCOM_SPI_INTENCLR offset) SPI Interrupt Enable Clear */
#define SERCOM_SPI_INTENCLR_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_SPI_INTENCLR reset_value) SPI Interrupt Enable Clear */

#define SERCOM_SPI_INTENCLR_DRE_Pos 0            /**< \brief (SERCOM_SPI_INTENCLR) Data Register Empty Interrupt Enable */
#define SERCOM_SPI_INTENCLR_DRE     (_U_(0x1) << SERCOM_SPI_INTENCLR_DRE_Pos)
#define SERCOM_SPI_INTENCLR_TXC_Pos 1            /**< \brief (SERCOM_SPI_INTENCLR) Transmit Complete Interrupt Enable */
#define SERCOM_SPI_INTENCLR_TXC     (_U_(0x1) << SERCOM_SPI_INTENCLR_TXC_Pos)
#define SERCOM_SPI_INTENCLR_RXC_Pos 2            /**< \brief (SERCOM_SPI_INTENCLR) Receive Complete Interrupt Enable */
#define SERCOM_SPI_INTENCLR_RXC     (_U_(0x1) << SERCOM_SPI_INTENCLR_RXC_Pos)
#define SERCOM_SPI_INTENCLR_MASK    _U_(0x07)    /**< \brief (SERCOM_SPI_INTENCLR) MASK Register */

/* -------- SERCOM_USART_INTENCLR : (SERCOM Offset: 0x0C) (R/W  8) USART USART Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DRE:1;            /*!< bit:      0  Data Register Empty Interrupt Enable */
    uint8_t  TXC:1;            /*!< bit:      1  Transmit Complete Interrupt Enable */
    uint8_t  RXC:1;            /*!< bit:      2  Receive Complete Interrupt Enable  */
    uint8_t  RXS:1;            /*!< bit:      3  Receive Start Interrupt Disable    */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_USART_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_INTENCLR_OFFSET 0x0C         /**< \brief (SERCOM_USART_INTENCLR offset) USART Interrupt Enable Clear */
#define SERCOM_USART_INTENCLR_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_USART_INTENCLR reset_value) USART Interrupt Enable Clear */

#define SERCOM_USART_INTENCLR_DRE_Pos 0            /**< \brief (SERCOM_USART_INTENCLR) Data Register Empty Interrupt Enable */
#define SERCOM_USART_INTENCLR_DRE   (_U_(0x1) << SERCOM_USART_INTENCLR_DRE_Pos)
#define SERCOM_USART_INTENCLR_TXC_Pos 1            /**< \brief (SERCOM_USART_INTENCLR) Transmit Complete Interrupt Enable */
#define SERCOM_USART_INTENCLR_TXC   (_U_(0x1) << SERCOM_USART_INTENCLR_TXC_Pos)
#define SERCOM_USART_INTENCLR_RXC_Pos 2            /**< \brief (SERCOM_USART_INTENCLR) Receive Complete Interrupt Enable */
#define SERCOM_USART_INTENCLR_RXC   (_U_(0x1) << SERCOM_USART_INTENCLR_RXC_Pos)
#define SERCOM_USART_INTENCLR_RXS_Pos 3            /**< \brief (SERCOM_USART_INTENCLR) Receive Start Interrupt Disable */
#define SERCOM_USART_INTENCLR_RXS   (_U_(0x1) << SERCOM_USART_INTENCLR_RXS_Pos)
#define SERCOM_USART_INTENCLR_MASK  _U_(0x0F)    /**< \brief (SERCOM_USART_INTENCLR) MASK Register */

/* -------- SERCOM_I2CM_INTENSET : (SERCOM Offset: 0x0D) (R/W  8) I2CM I2CM Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  MB:1;             /*!< bit:      0  Master on Bus Interrupt Enable     */
    uint8_t  SB:1;             /*!< bit:      1  Slave on Bus Interrupt Enable      */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CM_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_INTENSET_OFFSET 0x0D         /**< \brief (SERCOM_I2CM_INTENSET offset) I2CM Interrupt Enable Set */
#define SERCOM_I2CM_INTENSET_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CM_INTENSET reset_value) I2CM Interrupt Enable Set */

#define SERCOM_I2CM_INTENSET_MB_Pos 0            /**< \brief (SERCOM_I2CM_INTENSET) Master on Bus Interrupt Enable */
#define SERCOM_I2CM_INTENSET_MB     (_U_(0x1) << SERCOM_I2CM_INTENSET_MB_Pos)
#define SERCOM_I2CM_INTENSET_SB_Pos 1            /**< \brief (SERCOM_I2CM_INTENSET) Slave on Bus Interrupt Enable */
#define SERCOM_I2CM_INTENSET_SB     (_U_(0x1) << SERCOM_I2CM_INTENSET_SB_Pos)
#define SERCOM_I2CM_INTENSET_MASK   _U_(0x03)    /**< \brief (SERCOM_I2CM_INTENSET) MASK Register */

/* -------- SERCOM_I2CS_INTENSET : (SERCOM Offset: 0x0D) (R/W  8) I2CS I2CS Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PREC:1;           /*!< bit:      0  Stop Received Interrupt Enable     */
    uint8_t  AMATCH:1;         /*!< bit:      1  Address Match Interrupt Enable     */
    uint8_t  DRDY:1;           /*!< bit:      2  Data Ready Interrupt Enable        */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CS_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_INTENSET_OFFSET 0x0D         /**< \brief (SERCOM_I2CS_INTENSET offset) I2CS Interrupt Enable Set */
#define SERCOM_I2CS_INTENSET_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CS_INTENSET reset_value) I2CS Interrupt Enable Set */

#define SERCOM_I2CS_INTENSET_PREC_Pos 0            /**< \brief (SERCOM_I2CS_INTENSET) Stop Received Interrupt Enable */
#define SERCOM_I2CS_INTENSET_PREC   (_U_(0x1) << SERCOM_I2CS_INTENSET_PREC_Pos)
#define SERCOM_I2CS_INTENSET_AMATCH_Pos 1            /**< \brief (SERCOM_I2CS_INTENSET) Address Match Interrupt Enable */
#define SERCOM_I2CS_INTENSET_AMATCH (_U_(0x1) << SERCOM_I2CS_INTENSET_AMATCH_Pos)
#define SERCOM_I2CS_INTENSET_DRDY_Pos 2            /**< \brief (SERCOM_I2CS_INTENSET) Data Ready Interrupt Enable */
#define SERCOM_I2CS_INTENSET_DRDY   (_U_(0x1) << SERCOM_I2CS_INTENSET_DRDY_Pos)
#define SERCOM_I2CS_INTENSET_MASK   _U_(0x07)    /**< \brief (SERCOM_I2CS_INTENSET) MASK Register */

/* -------- SERCOM_SPI_INTENSET : (SERCOM Offset: 0x0D) (R/W  8) SPI SPI Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DRE:1;            /*!< bit:      0  Data Register Empty Interrupt Enable */
    uint8_t  TXC:1;            /*!< bit:      1  Transmit Complete Interrupt Enable */
    uint8_t  RXC:1;            /*!< bit:      2  Receive Complete Interrupt Enable  */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_SPI_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_INTENSET_OFFSET  0x0D         /**< \brief (SERCOM_SPI_INTENSET offset) SPI Interrupt Enable Set */
#define SERCOM_SPI_INTENSET_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_SPI_INTENSET reset_value) SPI Interrupt Enable Set */

#define SERCOM_SPI_INTENSET_DRE_Pos 0            /**< \brief (SERCOM_SPI_INTENSET) Data Register Empty Interrupt Enable */
#define SERCOM_SPI_INTENSET_DRE     (_U_(0x1) << SERCOM_SPI_INTENSET_DRE_Pos)
#define SERCOM_SPI_INTENSET_TXC_Pos 1            /**< \brief (SERCOM_SPI_INTENSET) Transmit Complete Interrupt Enable */
#define SERCOM_SPI_INTENSET_TXC     (_U_(0x1) << SERCOM_SPI_INTENSET_TXC_Pos)
#define SERCOM_SPI_INTENSET_RXC_Pos 2            /**< \brief (SERCOM_SPI_INTENSET) Receive Complete Interrupt Enable */
#define SERCOM_SPI_INTENSET_RXC     (_U_(0x1) << SERCOM_SPI_INTENSET_RXC_Pos)
#define SERCOM_SPI_INTENSET_MASK    _U_(0x07)    /**< \brief (SERCOM_SPI_INTENSET) MASK Register */

/* -------- SERCOM_USART_INTENSET : (SERCOM Offset: 0x0D) (R/W  8) USART USART Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DRE:1;            /*!< bit:      0  Data Register Empty Interrupt Enable */
    uint8_t  TXC:1;            /*!< bit:      1  Transmit Complete Interrupt Enable */
    uint8_t  RXC:1;            /*!< bit:      2  Receive Complete Interrupt Enable  */
    uint8_t  RXS:1;            /*!< bit:      3  Receive Start Interrupt Enable     */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_USART_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_INTENSET_OFFSET 0x0D         /**< \brief (SERCOM_USART_INTENSET offset) USART Interrupt Enable Set */
#define SERCOM_USART_INTENSET_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_USART_INTENSET reset_value) USART Interrupt Enable Set */

#define SERCOM_USART_INTENSET_DRE_Pos 0            /**< \brief (SERCOM_USART_INTENSET) Data Register Empty Interrupt Enable */
#define SERCOM_USART_INTENSET_DRE   (_U_(0x1) << SERCOM_USART_INTENSET_DRE_Pos)
#define SERCOM_USART_INTENSET_TXC_Pos 1            /**< \brief (SERCOM_USART_INTENSET) Transmit Complete Interrupt Enable */
#define SERCOM_USART_INTENSET_TXC   (_U_(0x1) << SERCOM_USART_INTENSET_TXC_Pos)
#define SERCOM_USART_INTENSET_RXC_Pos 2            /**< \brief (SERCOM_USART_INTENSET) Receive Complete Interrupt Enable */
#define SERCOM_USART_INTENSET_RXC   (_U_(0x1) << SERCOM_USART_INTENSET_RXC_Pos)
#define SERCOM_USART_INTENSET_RXS_Pos 3            /**< \brief (SERCOM_USART_INTENSET) Receive Start Interrupt Enable */
#define SERCOM_USART_INTENSET_RXS   (_U_(0x1) << SERCOM_USART_INTENSET_RXS_Pos)
#define SERCOM_USART_INTENSET_MASK  _U_(0x0F)    /**< \brief (SERCOM_USART_INTENSET) MASK Register */

/* -------- SERCOM_I2CM_INTFLAG : (SERCOM Offset: 0x0E) (R/W  8) I2CM I2CM Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  MB:1;             /*!< bit:      0  Master on Bus                      */
    __I uint8_t  SB:1;             /*!< bit:      1  Slave on Bus                       */
    __I uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CM_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_INTFLAG_OFFSET  0x0E         /**< \brief (SERCOM_I2CM_INTFLAG offset) I2CM Interrupt Flag Status and Clear */
#define SERCOM_I2CM_INTFLAG_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CM_INTFLAG reset_value) I2CM Interrupt Flag Status and Clear */

#define SERCOM_I2CM_INTFLAG_MB_Pos  0            /**< \brief (SERCOM_I2CM_INTFLAG) Master on Bus */
#define SERCOM_I2CM_INTFLAG_MB      (_U_(0x1) << SERCOM_I2CM_INTFLAG_MB_Pos)
#define SERCOM_I2CM_INTFLAG_SB_Pos  1            /**< \brief (SERCOM_I2CM_INTFLAG) Slave on Bus */
#define SERCOM_I2CM_INTFLAG_SB      (_U_(0x1) << SERCOM_I2CM_INTFLAG_SB_Pos)
#define SERCOM_I2CM_INTFLAG_MASK    _U_(0x03)    /**< \brief (SERCOM_I2CM_INTFLAG) MASK Register */

/* -------- SERCOM_I2CS_INTFLAG : (SERCOM Offset: 0x0E) (R/W  8) I2CS I2CS Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  PREC:1;           /*!< bit:      0  Stop Received                      */
    __I uint8_t  AMATCH:1;         /*!< bit:      1  Address Match                      */
    __I uint8_t  DRDY:1;           /*!< bit:      2  Data Ready                         */
    __I uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CS_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_INTFLAG_OFFSET  0x0E         /**< \brief (SERCOM_I2CS_INTFLAG offset) I2CS Interrupt Flag Status and Clear */
#define SERCOM_I2CS_INTFLAG_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CS_INTFLAG reset_value) I2CS Interrupt Flag Status and Clear */

#define SERCOM_I2CS_INTFLAG_PREC_Pos 0            /**< \brief (SERCOM_I2CS_INTFLAG) Stop Received */
#define SERCOM_I2CS_INTFLAG_PREC    (_U_(0x1) << SERCOM_I2CS_INTFLAG_PREC_Pos)
#define SERCOM_I2CS_INTFLAG_AMATCH_Pos 1            /**< \brief (SERCOM_I2CS_INTFLAG) Address Match */
#define SERCOM_I2CS_INTFLAG_AMATCH  (_U_(0x1) << SERCOM_I2CS_INTFLAG_AMATCH_Pos)
#define SERCOM_I2CS_INTFLAG_DRDY_Pos 2            /**< \brief (SERCOM_I2CS_INTFLAG) Data Ready */
#define SERCOM_I2CS_INTFLAG_DRDY    (_U_(0x1) << SERCOM_I2CS_INTFLAG_DRDY_Pos)
#define SERCOM_I2CS_INTFLAG_MASK    _U_(0x07)    /**< \brief (SERCOM_I2CS_INTFLAG) MASK Register */

/* -------- SERCOM_SPI_INTFLAG : (SERCOM Offset: 0x0E) (R/W  8) SPI SPI Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  DRE:1;            /*!< bit:      0  Data Register Empty                */
    __I uint8_t  TXC:1;            /*!< bit:      1  Transmit Complete                  */
    __I uint8_t  RXC:1;            /*!< bit:      2  Receive Complete                   */
    __I uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_SPI_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_INTFLAG_OFFSET   0x0E         /**< \brief (SERCOM_SPI_INTFLAG offset) SPI Interrupt Flag Status and Clear */
#define SERCOM_SPI_INTFLAG_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_SPI_INTFLAG reset_value) SPI Interrupt Flag Status and Clear */

#define SERCOM_SPI_INTFLAG_DRE_Pos  0            /**< \brief (SERCOM_SPI_INTFLAG) Data Register Empty */
#define SERCOM_SPI_INTFLAG_DRE      (_U_(0x1) << SERCOM_SPI_INTFLAG_DRE_Pos)
#define SERCOM_SPI_INTFLAG_TXC_Pos  1            /**< \brief (SERCOM_SPI_INTFLAG) Transmit Complete */
#define SERCOM_SPI_INTFLAG_TXC      (_U_(0x1) << SERCOM_SPI_INTFLAG_TXC_Pos)
#define SERCOM_SPI_INTFLAG_RXC_Pos  2            /**< \brief (SERCOM_SPI_INTFLAG) Receive Complete */
#define SERCOM_SPI_INTFLAG_RXC      (_U_(0x1) << SERCOM_SPI_INTFLAG_RXC_Pos)
#define SERCOM_SPI_INTFLAG_MASK     _U_(0x07)    /**< \brief (SERCOM_SPI_INTFLAG) MASK Register */

/* -------- SERCOM_USART_INTFLAG : (SERCOM Offset: 0x0E) (R/W  8) USART USART Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  DRE:1;            /*!< bit:      0  Data Register Empty                */
    __I uint8_t  TXC:1;            /*!< bit:      1  Transmit Complete                  */
    __I uint8_t  RXC:1;            /*!< bit:      2  Receive Complete                   */
    __I uint8_t  RXS:1;            /*!< bit:      3  Receive Start Interrupt            */
    __I uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_USART_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_INTFLAG_OFFSET 0x0E         /**< \brief (SERCOM_USART_INTFLAG offset) USART Interrupt Flag Status and Clear */
#define SERCOM_USART_INTFLAG_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_USART_INTFLAG reset_value) USART Interrupt Flag Status and Clear */

#define SERCOM_USART_INTFLAG_DRE_Pos 0            /**< \brief (SERCOM_USART_INTFLAG) Data Register Empty */
#define SERCOM_USART_INTFLAG_DRE    (_U_(0x1) << SERCOM_USART_INTFLAG_DRE_Pos)
#define SERCOM_USART_INTFLAG_TXC_Pos 1            /**< \brief (SERCOM_USART_INTFLAG) Transmit Complete */
#define SERCOM_USART_INTFLAG_TXC    (_U_(0x1) << SERCOM_USART_INTFLAG_TXC_Pos)
#define SERCOM_USART_INTFLAG_RXC_Pos 2            /**< \brief (SERCOM_USART_INTFLAG) Receive Complete */
#define SERCOM_USART_INTFLAG_RXC    (_U_(0x1) << SERCOM_USART_INTFLAG_RXC_Pos)
#define SERCOM_USART_INTFLAG_RXS_Pos 3            /**< \brief (SERCOM_USART_INTFLAG) Receive Start Interrupt */
#define SERCOM_USART_INTFLAG_RXS    (_U_(0x1) << SERCOM_USART_INTFLAG_RXS_Pos)
#define SERCOM_USART_INTFLAG_MASK   _U_(0x0F)    /**< \brief (SERCOM_USART_INTFLAG) MASK Register */

/* -------- SERCOM_I2CM_STATUS : (SERCOM Offset: 0x10) (R/W 16) I2CM I2CM Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BUSERR:1;         /*!< bit:      0  Bus Error                          */
    uint16_t ARBLOST:1;        /*!< bit:      1  Arbitration Lost                   */
    uint16_t RXNACK:1;         /*!< bit:      2  Received Not Acknowledge           */
    uint16_t :1;               /*!< bit:      3  Reserved                           */
    uint16_t BUSSTATE:2;       /*!< bit:  4.. 5  Bus State                          */
    uint16_t LOWTOUT:1;        /*!< bit:      6  SCL Low Time-out                   */
    uint16_t CLKHOLD:1;        /*!< bit:      7  Clock Hold                         */
    uint16_t :7;               /*!< bit:  8..14  Reserved                           */
    uint16_t SYNCBUSY:1;       /*!< bit:     15  Synchronization Busy               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CM_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_STATUS_OFFSET   0x10         /**< \brief (SERCOM_I2CM_STATUS offset) I2CM Status */
#define SERCOM_I2CM_STATUS_RESETVALUE _U_(0x0000)  /**< \brief (SERCOM_I2CM_STATUS reset_value) I2CM Status */

#define SERCOM_I2CM_STATUS_BUSERR_Pos 0            /**< \brief (SERCOM_I2CM_STATUS) Bus Error */
#define SERCOM_I2CM_STATUS_BUSERR   (_U_(0x1) << SERCOM_I2CM_STATUS_BUSERR_Pos)
#define SERCOM_I2CM_STATUS_ARBLOST_Pos 1            /**< \brief (SERCOM_I2CM_STATUS) Arbitration Lost */
#define SERCOM_I2CM_STATUS_ARBLOST  (_U_(0x1) << SERCOM_I2CM_STATUS_ARBLOST_Pos)
#define SERCOM_I2CM_STATUS_RXNACK_Pos 2            /**< \brief (SERCOM_I2CM_STATUS) Received Not Acknowledge */
#define SERCOM_I2CM_STATUS_RXNACK   (_U_(0x1) << SERCOM_I2CM_STATUS_RXNACK_Pos)
#define SERCOM_I2CM_STATUS_BUSSTATE_Pos 4            /**< \brief (SERCOM_I2CM_STATUS) Bus State */
#define SERCOM_I2CM_STATUS_BUSSTATE_Msk (_U_(0x3) << SERCOM_I2CM_STATUS_BUSSTATE_Pos)
#define SERCOM_I2CM_STATUS_BUSSTATE(value) (SERCOM_I2CM_STATUS_BUSSTATE_Msk & ((value) << SERCOM_I2CM_STATUS_BUSSTATE_Pos))
#define SERCOM_I2CM_STATUS_LOWTOUT_Pos 6            /**< \brief (SERCOM_I2CM_STATUS) SCL Low Time-out */
#define SERCOM_I2CM_STATUS_LOWTOUT  (_U_(0x1) << SERCOM_I2CM_STATUS_LOWTOUT_Pos)
#define SERCOM_I2CM_STATUS_CLKHOLD_Pos 7            /**< \brief (SERCOM_I2CM_STATUS) Clock Hold */
#define SERCOM_I2CM_STATUS_CLKHOLD  (_U_(0x1) << SERCOM_I2CM_STATUS_CLKHOLD_Pos)
#define SERCOM_I2CM_STATUS_SYNCBUSY_Pos 15           /**< \brief (SERCOM_I2CM_STATUS) Synchronization Busy */
#define SERCOM_I2CM_STATUS_SYNCBUSY (_U_(0x1) << SERCOM_I2CM_STATUS_SYNCBUSY_Pos)
#define SERCOM_I2CM_STATUS_MASK     _U_(0x80F7)  /**< \brief (SERCOM_I2CM_STATUS) MASK Register */

/* -------- SERCOM_I2CS_STATUS : (SERCOM Offset: 0x10) (R/W 16) I2CS I2CS Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BUSERR:1;         /*!< bit:      0  Bus Error                          */
    uint16_t COLL:1;           /*!< bit:      1  Transmit Collision                 */
    uint16_t RXNACK:1;         /*!< bit:      2  Received Not Acknowledge           */
    uint16_t DIR:1;            /*!< bit:      3  Read / Write Direction             */
    uint16_t SR:1;             /*!< bit:      4  Repeated Start                     */
    uint16_t :1;               /*!< bit:      5  Reserved                           */
    uint16_t LOWTOUT:1;        /*!< bit:      6  SCL Low Time-out                   */
    uint16_t CLKHOLD:1;        /*!< bit:      7  Clock Hold                         */
    uint16_t :7;               /*!< bit:  8..14  Reserved                           */
    uint16_t SYNCBUSY:1;       /*!< bit:     15  Synchronization Busy               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CS_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_STATUS_OFFSET   0x10         /**< \brief (SERCOM_I2CS_STATUS offset) I2CS Status */
#define SERCOM_I2CS_STATUS_RESETVALUE _U_(0x0000)  /**< \brief (SERCOM_I2CS_STATUS reset_value) I2CS Status */

#define SERCOM_I2CS_STATUS_BUSERR_Pos 0            /**< \brief (SERCOM_I2CS_STATUS) Bus Error */
#define SERCOM_I2CS_STATUS_BUSERR   (_U_(0x1) << SERCOM_I2CS_STATUS_BUSERR_Pos)
#define SERCOM_I2CS_STATUS_COLL_Pos 1            /**< \brief (SERCOM_I2CS_STATUS) Transmit Collision */
#define SERCOM_I2CS_STATUS_COLL     (_U_(0x1) << SERCOM_I2CS_STATUS_COLL_Pos)
#define SERCOM_I2CS_STATUS_RXNACK_Pos 2            /**< \brief (SERCOM_I2CS_STATUS) Received Not Acknowledge */
#define SERCOM_I2CS_STATUS_RXNACK   (_U_(0x1) << SERCOM_I2CS_STATUS_RXNACK_Pos)
#define SERCOM_I2CS_STATUS_DIR_Pos  3            /**< \brief (SERCOM_I2CS_STATUS) Read / Write Direction */
#define SERCOM_I2CS_STATUS_DIR      (_U_(0x1) << SERCOM_I2CS_STATUS_DIR_Pos)
#define SERCOM_I2CS_STATUS_SR_Pos   4            /**< \brief (SERCOM_I2CS_STATUS) Repeated Start */
#define SERCOM_I2CS_STATUS_SR       (_U_(0x1) << SERCOM_I2CS_STATUS_SR_Pos)
#define SERCOM_I2CS_STATUS_LOWTOUT_Pos 6            /**< \brief (SERCOM_I2CS_STATUS) SCL Low Time-out */
#define SERCOM_I2CS_STATUS_LOWTOUT  (_U_(0x1) << SERCOM_I2CS_STATUS_LOWTOUT_Pos)
#define SERCOM_I2CS_STATUS_CLKHOLD_Pos 7            /**< \brief (SERCOM_I2CS_STATUS) Clock Hold */
#define SERCOM_I2CS_STATUS_CLKHOLD  (_U_(0x1) << SERCOM_I2CS_STATUS_CLKHOLD_Pos)
#define SERCOM_I2CS_STATUS_SYNCBUSY_Pos 15           /**< \brief (SERCOM_I2CS_STATUS) Synchronization Busy */
#define SERCOM_I2CS_STATUS_SYNCBUSY (_U_(0x1) << SERCOM_I2CS_STATUS_SYNCBUSY_Pos)
#define SERCOM_I2CS_STATUS_MASK     _U_(0x80DF)  /**< \brief (SERCOM_I2CS_STATUS) MASK Register */

/* -------- SERCOM_SPI_STATUS : (SERCOM Offset: 0x10) (R/W 16) SPI SPI Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint16_t BUFOVF:1;         /*!< bit:      2  Buffer Overflow                    */
    uint16_t :12;              /*!< bit:  3..14  Reserved                           */
    uint16_t SYNCBUSY:1;       /*!< bit:     15  Synchronization Busy               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_SPI_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_STATUS_OFFSET    0x10         /**< \brief (SERCOM_SPI_STATUS offset) SPI Status */
#define SERCOM_SPI_STATUS_RESETVALUE _U_(0x0000)  /**< \brief (SERCOM_SPI_STATUS reset_value) SPI Status */

#define SERCOM_SPI_STATUS_BUFOVF_Pos 2            /**< \brief (SERCOM_SPI_STATUS) Buffer Overflow */
#define SERCOM_SPI_STATUS_BUFOVF    (_U_(0x1) << SERCOM_SPI_STATUS_BUFOVF_Pos)
#define SERCOM_SPI_STATUS_SYNCBUSY_Pos 15           /**< \brief (SERCOM_SPI_STATUS) Synchronization Busy */
#define SERCOM_SPI_STATUS_SYNCBUSY  (_U_(0x1) << SERCOM_SPI_STATUS_SYNCBUSY_Pos)
#define SERCOM_SPI_STATUS_MASK      _U_(0x8004)  /**< \brief (SERCOM_SPI_STATUS) MASK Register */

/* -------- SERCOM_USART_STATUS : (SERCOM Offset: 0x10) (R/W 16) USART USART Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PERR:1;           /*!< bit:      0  Parity Error                       */
    uint16_t FERR:1;           /*!< bit:      1  Frame Error                        */
    uint16_t BUFOVF:1;         /*!< bit:      2  Buffer Overflow                    */
    uint16_t :12;              /*!< bit:  3..14  Reserved                           */
    uint16_t SYNCBUSY:1;       /*!< bit:     15  Synchronization Busy               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_USART_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_STATUS_OFFSET  0x10         /**< \brief (SERCOM_USART_STATUS offset) USART Status */
#define SERCOM_USART_STATUS_RESETVALUE _U_(0x0000)  /**< \brief (SERCOM_USART_STATUS reset_value) USART Status */

#define SERCOM_USART_STATUS_PERR_Pos 0            /**< \brief (SERCOM_USART_STATUS) Parity Error */
#define SERCOM_USART_STATUS_PERR    (_U_(0x1) << SERCOM_USART_STATUS_PERR_Pos)
#define SERCOM_USART_STATUS_FERR_Pos 1            /**< \brief (SERCOM_USART_STATUS) Frame Error */
#define SERCOM_USART_STATUS_FERR    (_U_(0x1) << SERCOM_USART_STATUS_FERR_Pos)
#define SERCOM_USART_STATUS_BUFOVF_Pos 2            /**< \brief (SERCOM_USART_STATUS) Buffer Overflow */
#define SERCOM_USART_STATUS_BUFOVF  (_U_(0x1) << SERCOM_USART_STATUS_BUFOVF_Pos)
#define SERCOM_USART_STATUS_SYNCBUSY_Pos 15           /**< \brief (SERCOM_USART_STATUS) Synchronization Busy */
#define SERCOM_USART_STATUS_SYNCBUSY (_U_(0x1) << SERCOM_USART_STATUS_SYNCBUSY_Pos)
#define SERCOM_USART_STATUS_MASK    _U_(0x8007)  /**< \brief (SERCOM_USART_STATUS) MASK Register */

/* -------- SERCOM_I2CM_ADDR : (SERCOM Offset: 0x14) (R/W  8) I2CM I2CM Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ADDR:8;           /*!< bit:  0.. 7  Address                            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CM_ADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_ADDR_OFFSET     0x14         /**< \brief (SERCOM_I2CM_ADDR offset) I2CM Address */
#define SERCOM_I2CM_ADDR_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CM_ADDR reset_value) I2CM Address */

#define SERCOM_I2CM_ADDR_ADDR_Pos   0            /**< \brief (SERCOM_I2CM_ADDR) Address */
#define SERCOM_I2CM_ADDR_ADDR_Msk   (_U_(0xFF) << SERCOM_I2CM_ADDR_ADDR_Pos)
#define SERCOM_I2CM_ADDR_ADDR(value) (SERCOM_I2CM_ADDR_ADDR_Msk & ((value) << SERCOM_I2CM_ADDR_ADDR_Pos))
#define SERCOM_I2CM_ADDR_MASK       _U_(0xFF)    /**< \brief (SERCOM_I2CM_ADDR) MASK Register */

/* -------- SERCOM_I2CS_ADDR : (SERCOM Offset: 0x14) (R/W 32) I2CS I2CS Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t GENCEN:1;         /*!< bit:      0  General Call Address Enable        */
    uint32_t ADDR:7;           /*!< bit:  1.. 7  Address                            */
    uint32_t :9;               /*!< bit:  8..16  Reserved                           */
    uint32_t ADDRMASK:7;       /*!< bit: 17..23  Address Mask                       */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_I2CS_ADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_ADDR_OFFSET     0x14         /**< \brief (SERCOM_I2CS_ADDR offset) I2CS Address */
#define SERCOM_I2CS_ADDR_RESETVALUE _U_(0x00000000) /**< \brief (SERCOM_I2CS_ADDR reset_value) I2CS Address */

#define SERCOM_I2CS_ADDR_GENCEN_Pos 0            /**< \brief (SERCOM_I2CS_ADDR) General Call Address Enable */
#define SERCOM_I2CS_ADDR_GENCEN     (_U_(0x1) << SERCOM_I2CS_ADDR_GENCEN_Pos)
#define SERCOM_I2CS_ADDR_ADDR_Pos   1            /**< \brief (SERCOM_I2CS_ADDR) Address */
#define SERCOM_I2CS_ADDR_ADDR_Msk   (_U_(0x7F) << SERCOM_I2CS_ADDR_ADDR_Pos)
#define SERCOM_I2CS_ADDR_ADDR(value) (SERCOM_I2CS_ADDR_ADDR_Msk & ((value) << SERCOM_I2CS_ADDR_ADDR_Pos))
#define SERCOM_I2CS_ADDR_ADDRMASK_Pos 17           /**< \brief (SERCOM_I2CS_ADDR) Address Mask */
#define SERCOM_I2CS_ADDR_ADDRMASK_Msk (_U_(0x7F) << SERCOM_I2CS_ADDR_ADDRMASK_Pos)
#define SERCOM_I2CS_ADDR_ADDRMASK(value) (SERCOM_I2CS_ADDR_ADDRMASK_Msk & ((value) << SERCOM_I2CS_ADDR_ADDRMASK_Pos))
#define SERCOM_I2CS_ADDR_MASK       _U_(0x00FE00FF) /**< \brief (SERCOM_I2CS_ADDR) MASK Register */

/* -------- SERCOM_SPI_ADDR : (SERCOM Offset: 0x14) (R/W 32) SPI SPI Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:8;           /*!< bit:  0.. 7  Address                            */
    uint32_t :8;               /*!< bit:  8..15  Reserved                           */
    uint32_t ADDRMASK:8;       /*!< bit: 16..23  Address Mask                       */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SERCOM_SPI_ADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_ADDR_OFFSET      0x14         /**< \brief (SERCOM_SPI_ADDR offset) SPI Address */
#define SERCOM_SPI_ADDR_RESETVALUE  _U_(0x00000000) /**< \brief (SERCOM_SPI_ADDR reset_value) SPI Address */

#define SERCOM_SPI_ADDR_ADDR_Pos    0            /**< \brief (SERCOM_SPI_ADDR) Address */
#define SERCOM_SPI_ADDR_ADDR_Msk    (_U_(0xFF) << SERCOM_SPI_ADDR_ADDR_Pos)
#define SERCOM_SPI_ADDR_ADDR(value) (SERCOM_SPI_ADDR_ADDR_Msk & ((value) << SERCOM_SPI_ADDR_ADDR_Pos))
#define SERCOM_SPI_ADDR_ADDRMASK_Pos 16           /**< \brief (SERCOM_SPI_ADDR) Address Mask */
#define SERCOM_SPI_ADDR_ADDRMASK_Msk (_U_(0xFF) << SERCOM_SPI_ADDR_ADDRMASK_Pos)
#define SERCOM_SPI_ADDR_ADDRMASK(value) (SERCOM_SPI_ADDR_ADDRMASK_Msk & ((value) << SERCOM_SPI_ADDR_ADDRMASK_Pos))
#define SERCOM_SPI_ADDR_MASK        _U_(0x00FF00FF) /**< \brief (SERCOM_SPI_ADDR) MASK Register */

/* -------- SERCOM_I2CM_DATA : (SERCOM Offset: 0x18) (R/W  8) I2CM I2CM Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DATA:8;           /*!< bit:  0.. 7  Data                               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CM_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CM_DATA_OFFSET     0x18         /**< \brief (SERCOM_I2CM_DATA offset) I2CM Data */
#define SERCOM_I2CM_DATA_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CM_DATA reset_value) I2CM Data */

#define SERCOM_I2CM_DATA_DATA_Pos   0            /**< \brief (SERCOM_I2CM_DATA) Data */
#define SERCOM_I2CM_DATA_DATA_Msk   (_U_(0xFF) << SERCOM_I2CM_DATA_DATA_Pos)
#define SERCOM_I2CM_DATA_DATA(value) (SERCOM_I2CM_DATA_DATA_Msk & ((value) << SERCOM_I2CM_DATA_DATA_Pos))
#define SERCOM_I2CM_DATA_MASK       _U_(0xFF)    /**< \brief (SERCOM_I2CM_DATA) MASK Register */

/* -------- SERCOM_I2CS_DATA : (SERCOM Offset: 0x18) (R/W  8) I2CS I2CS Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DATA:8;           /*!< bit:  0.. 7  Data                               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SERCOM_I2CS_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_I2CS_DATA_OFFSET     0x18         /**< \brief (SERCOM_I2CS_DATA offset) I2CS Data */
#define SERCOM_I2CS_DATA_RESETVALUE _U_(0x00)    /**< \brief (SERCOM_I2CS_DATA reset_value) I2CS Data */

#define SERCOM_I2CS_DATA_DATA_Pos   0            /**< \brief (SERCOM_I2CS_DATA) Data */
#define SERCOM_I2CS_DATA_DATA_Msk   (_U_(0xFF) << SERCOM_I2CS_DATA_DATA_Pos)
#define SERCOM_I2CS_DATA_DATA(value) (SERCOM_I2CS_DATA_DATA_Msk & ((value) << SERCOM_I2CS_DATA_DATA_Pos))
#define SERCOM_I2CS_DATA_MASK       _U_(0xFF)    /**< \brief (SERCOM_I2CS_DATA) MASK Register */

/* -------- SERCOM_SPI_DATA : (SERCOM Offset: 0x18) (R/W 16) SPI SPI Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DATA:9;           /*!< bit:  0.. 8  Data                               */
    uint16_t :7;               /*!< bit:  9..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_SPI_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_SPI_DATA_OFFSET      0x18         /**< \brief (SERCOM_SPI_DATA offset) SPI Data */
#define SERCOM_SPI_DATA_RESETVALUE  _U_(0x0000)  /**< \brief (SERCOM_SPI_DATA reset_value) SPI Data */

#define SERCOM_SPI_DATA_DATA_Pos    0            /**< \brief (SERCOM_SPI_DATA) Data */
#define SERCOM_SPI_DATA_DATA_Msk    (_U_(0x1FF) << SERCOM_SPI_DATA_DATA_Pos)
#define SERCOM_SPI_DATA_DATA(value) (SERCOM_SPI_DATA_DATA_Msk & ((value) << SERCOM_SPI_DATA_DATA_Pos))
#define SERCOM_SPI_DATA_MASK        _U_(0x01FF)  /**< \brief (SERCOM_SPI_DATA) MASK Register */

/* -------- SERCOM_USART_DATA : (SERCOM Offset: 0x18) (R/W 16) USART USART Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DATA:9;           /*!< bit:  0.. 8  Data                               */
    uint16_t :7;               /*!< bit:  9..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SERCOM_USART_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SERCOM_USART_DATA_OFFSET    0x18         /**< \brief (SERCOM_USART_DATA offset) USART Data */
#define SERCOM_USART_DATA_RESETVALUE _U_(0x0000)  /**< \brief (SERCOM_USART_DATA reset_value) USART Data */

#define SERCOM_USART_DATA_DATA_Pos  0            /**< \brief (SERCOM_USART_DATA) Data */
#define SERCOM_USART_DATA_DATA_Msk  (_U_(0x1FF) << SERCOM_USART_DATA_DATA_Pos)
#define SERCOM_USART_DATA_DATA(value) (SERCOM_USART_DATA_DATA_Msk & ((value) << SERCOM_USART_DATA_DATA_Pos))
#define SERCOM_USART_DATA_MASK      _U_(0x01FF)  /**< \brief (SERCOM_USART_DATA) MASK Register */

/** \brief SERCOM_I2CM hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* I2C Master Mode */
  __IO SERCOM_I2CM_CTRLA_Type    CTRLA;       /**< \brief Offset: 0x00 (R/W 32) I2CM Control A */
  __IO SERCOM_I2CM_CTRLB_Type    CTRLB;       /**< \brief Offset: 0x04 (R/W 32) I2CM Control B */
  __IO SERCOM_I2CM_DBGCTRL_Type  DBGCTRL;     /**< \brief Offset: 0x08 (R/W  8) I2CM Debug Control */
       RoReg8                    Reserved1[0x1];
  __IO SERCOM_I2CM_BAUD_Type     BAUD;        /**< \brief Offset: 0x0A (R/W 16) I2CM Baud Rate */
  __IO SERCOM_I2CM_INTENCLR_Type INTENCLR;    /**< \brief Offset: 0x0C (R/W  8) I2CM Interrupt Enable Clear */
  __IO SERCOM_I2CM_INTENSET_Type INTENSET;    /**< \brief Offset: 0x0D (R/W  8) I2CM Interrupt Enable Set */
  __IO SERCOM_I2CM_INTFLAG_Type  INTFLAG;     /**< \brief Offset: 0x0E (R/W  8) I2CM Interrupt Flag Status and Clear */
       RoReg8                    Reserved2[0x1];
  __IO SERCOM_I2CM_STATUS_Type   STATUS;      /**< \brief Offset: 0x10 (R/W 16) I2CM Status */
       RoReg8                    Reserved3[0x2];
  __IO SERCOM_I2CM_ADDR_Type     ADDR;        /**< \brief Offset: 0x14 (R/W  8) I2CM Address */
       RoReg8                    Reserved4[0x3];
  __IO SERCOM_I2CM_DATA_Type     DATA;        /**< \brief Offset: 0x18 (R/W  8) I2CM Data */
} SercomI2cm;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief SERCOM_I2CS hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* I2C Slave Mode */
  __IO SERCOM_I2CS_CTRLA_Type    CTRLA;       /**< \brief Offset: 0x00 (R/W 32) I2CS Control A */
  __IO SERCOM_I2CS_CTRLB_Type    CTRLB;       /**< \brief Offset: 0x04 (R/W 32) I2CS Control B */
       RoReg8                    Reserved1[0x4];
  __IO SERCOM_I2CS_INTENCLR_Type INTENCLR;    /**< \brief Offset: 0x0C (R/W  8) I2CS Interrupt Enable Clear */
  __IO SERCOM_I2CS_INTENSET_Type INTENSET;    /**< \brief Offset: 0x0D (R/W  8) I2CS Interrupt Enable Set */
  __IO SERCOM_I2CS_INTFLAG_Type  INTFLAG;     /**< \brief Offset: 0x0E (R/W  8) I2CS Interrupt Flag Status and Clear */
       RoReg8                    Reserved2[0x1];
  __IO SERCOM_I2CS_STATUS_Type   STATUS;      /**< \brief Offset: 0x10 (R/W 16) I2CS Status */
       RoReg8                    Reserved3[0x2];
  __IO SERCOM_I2CS_ADDR_Type     ADDR;        /**< \brief Offset: 0x14 (R/W 32) I2CS Address */
  __IO SERCOM_I2CS_DATA_Type     DATA;        /**< \brief Offset: 0x18 (R/W  8) I2CS Data */
} SercomI2cs;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief SERCOM_SPI hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* SPI Mode */
  __IO SERCOM_SPI_CTRLA_Type     CTRLA;       /**< \brief Offset: 0x00 (R/W 32) SPI Control A */
  __IO SERCOM_SPI_CTRLB_Type     CTRLB;       /**< \brief Offset: 0x04 (R/W 32) SPI Control B */
  __IO SERCOM_SPI_DBGCTRL_Type   DBGCTRL;     /**< \brief Offset: 0x08 (R/W  8) SPI Debug Control */
       RoReg8                    Reserved1[0x1];
  __IO SERCOM_SPI_BAUD_Type      BAUD;        /**< \brief Offset: 0x0A (R/W  8) SPI Baud Rate */
       RoReg8                    Reserved2[0x1];
  __IO SERCOM_SPI_INTENCLR_Type  INTENCLR;    /**< \brief Offset: 0x0C (R/W  8) SPI Interrupt Enable Clear */
  __IO SERCOM_SPI_INTENSET_Type  INTENSET;    /**< \brief Offset: 0x0D (R/W  8) SPI Interrupt Enable Set */
  __IO SERCOM_SPI_INTFLAG_Type   INTFLAG;     /**< \brief Offset: 0x0E (R/W  8) SPI Interrupt Flag Status and Clear */
       RoReg8                    Reserved3[0x1];
  __IO SERCOM_SPI_STATUS_Type    STATUS;      /**< \brief Offset: 0x10 (R/W 16) SPI Status */
       RoReg8                    Reserved4[0x2];
  __IO SERCOM_SPI_ADDR_Type      ADDR;        /**< \brief Offset: 0x14 (R/W 32) SPI Address */
  __IO SERCOM_SPI_DATA_Type      DATA;        /**< \brief Offset: 0x18 (R/W 16) SPI Data */
} SercomSpi;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief SERCOM_USART hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* USART Mode */
  __IO SERCOM_USART_CTRLA_Type   CTRLA;       /**< \brief Offset: 0x00 (R/W 32) USART Control A */
  __IO SERCOM_USART_CTRLB_Type   CTRLB;       /**< \brief Offset: 0x04 (R/W 32) USART Control B */
  __IO SERCOM_USART_DBGCTRL_Type DBGCTRL;     /**< \brief Offset: 0x08 (R/W  8) USART Debug Control */
       RoReg8                    Reserved1[0x1];
  __IO SERCOM_USART_BAUD_Type    BAUD;        /**< \brief Offset: 0x0A (R/W 16) USART Baud */
  __IO SERCOM_USART_INTENCLR_Type INTENCLR;    /**< \brief Offset: 0x0C (R/W  8) USART Interrupt Enable Clear */
  __IO SERCOM_USART_INTENSET_Type INTENSET;    /**< \brief Offset: 0x0D (R/W  8) USART Interrupt Enable Set */
  __IO SERCOM_USART_INTFLAG_Type INTFLAG;     /**< \brief Offset: 0x0E (R/W  8) USART Interrupt Flag Status and Clear */
       RoReg8                    Reserved2[0x1];
  __IO SERCOM_USART_STATUS_Type  STATUS;      /**< \brief Offset: 0x10 (R/W 16) USART Status */
       RoReg8                    Reserved3[0x6];
  __IO SERCOM_USART_DATA_Type    DATA;        /**< \brief Offset: 0x18 (R/W 16) USART Data */
} SercomUsart;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
       SercomI2cm                I2CM;        /**< \brief Offset: 0x00 I2C Master Mode */
       SercomI2cs                I2CS;        /**< \brief Offset: 0x00 I2C Slave Mode */
       SercomSpi                 SPI;         /**< \brief Offset: 0x00 SPI Mode */
       SercomUsart               USART;       /**< \brief Offset: 0x00 USART Mode */
} Sercom;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/*@}*/

#endif /* _SAMD20_SERCOM_COMPONENT_ */
