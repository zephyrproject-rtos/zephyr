/**
 * \file
 *
 * \brief Component description for UART
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_UART_COMPONENT_H_
#define _SAME70_UART_COMPONENT_H_
#define _SAME70_UART_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_UART Universal Asynchronous Receiver Transmitter
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR UART */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define UART_6418                       /**< (UART) Module ID */
#define REV_UART P                      /**< (UART) Module revision */

/* -------- UART_CR : (UART Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t RSTRX:1;                   /**< bit:      2  Reset Receiver                           */
    uint32_t RSTTX:1;                   /**< bit:      3  Reset Transmitter                        */
    uint32_t RXEN:1;                    /**< bit:      4  Receiver Enable                          */
    uint32_t RXDIS:1;                   /**< bit:      5  Receiver Disable                         */
    uint32_t TXEN:1;                    /**< bit:      6  Transmitter Enable                       */
    uint32_t TXDIS:1;                   /**< bit:      7  Transmitter Disable                      */
    uint32_t RSTSTA:1;                  /**< bit:      8  Reset Status                             */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t REQCLR:1;                  /**< bit:     12  Request Clear                            */
    uint32_t :19;                       /**< bit: 13..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_CR_OFFSET                      (0x00)                                        /**<  (UART_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_CR_RSTRX_Pos                   2                                              /**< (UART_CR) Reset Receiver Position */
#define UART_CR_RSTRX_Msk                   (0x1U << UART_CR_RSTRX_Pos)                    /**< (UART_CR) Reset Receiver Mask */
#define UART_CR_RSTRX                       UART_CR_RSTRX_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_RSTRX_Msk instead */
#define UART_CR_RSTTX_Pos                   3                                              /**< (UART_CR) Reset Transmitter Position */
#define UART_CR_RSTTX_Msk                   (0x1U << UART_CR_RSTTX_Pos)                    /**< (UART_CR) Reset Transmitter Mask */
#define UART_CR_RSTTX                       UART_CR_RSTTX_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_RSTTX_Msk instead */
#define UART_CR_RXEN_Pos                    4                                              /**< (UART_CR) Receiver Enable Position */
#define UART_CR_RXEN_Msk                    (0x1U << UART_CR_RXEN_Pos)                     /**< (UART_CR) Receiver Enable Mask */
#define UART_CR_RXEN                        UART_CR_RXEN_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_RXEN_Msk instead */
#define UART_CR_RXDIS_Pos                   5                                              /**< (UART_CR) Receiver Disable Position */
#define UART_CR_RXDIS_Msk                   (0x1U << UART_CR_RXDIS_Pos)                    /**< (UART_CR) Receiver Disable Mask */
#define UART_CR_RXDIS                       UART_CR_RXDIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_RXDIS_Msk instead */
#define UART_CR_TXEN_Pos                    6                                              /**< (UART_CR) Transmitter Enable Position */
#define UART_CR_TXEN_Msk                    (0x1U << UART_CR_TXEN_Pos)                     /**< (UART_CR) Transmitter Enable Mask */
#define UART_CR_TXEN                        UART_CR_TXEN_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_TXEN_Msk instead */
#define UART_CR_TXDIS_Pos                   7                                              /**< (UART_CR) Transmitter Disable Position */
#define UART_CR_TXDIS_Msk                   (0x1U << UART_CR_TXDIS_Pos)                    /**< (UART_CR) Transmitter Disable Mask */
#define UART_CR_TXDIS                       UART_CR_TXDIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_TXDIS_Msk instead */
#define UART_CR_RSTSTA_Pos                  8                                              /**< (UART_CR) Reset Status Position */
#define UART_CR_RSTSTA_Msk                  (0x1U << UART_CR_RSTSTA_Pos)                   /**< (UART_CR) Reset Status Mask */
#define UART_CR_RSTSTA                      UART_CR_RSTSTA_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_RSTSTA_Msk instead */
#define UART_CR_REQCLR_Pos                  12                                             /**< (UART_CR) Request Clear Position */
#define UART_CR_REQCLR_Msk                  (0x1U << UART_CR_REQCLR_Pos)                   /**< (UART_CR) Request Clear Mask */
#define UART_CR_REQCLR                      UART_CR_REQCLR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CR_REQCLR_Msk instead */
#define UART_CR_MASK                        (0x11FCU)                                      /**< \deprecated (UART_CR) Register MASK  (Use UART_CR_Msk instead)  */
#define UART_CR_Msk                         (0x11FCU)                                      /**< (UART_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_MR : (UART Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :4;                        /**< bit:   0..3  Reserved */
    uint32_t FILTER:1;                  /**< bit:      4  Receiver Digital Filter                  */
    uint32_t :4;                        /**< bit:   5..8  Reserved */
    uint32_t PAR:3;                     /**< bit:  9..11  Parity Type                              */
    uint32_t BRSRCCK:1;                 /**< bit:     12  Baud Rate Source Clock                   */
    uint32_t :1;                        /**< bit:     13  Reserved */
    uint32_t CHMODE:2;                  /**< bit: 14..15  Channel Mode                             */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_MR_OFFSET                      (0x04)                                        /**<  (UART_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_MR_FILTER_Pos                  4                                              /**< (UART_MR) Receiver Digital Filter Position */
#define UART_MR_FILTER_Msk                  (0x1U << UART_MR_FILTER_Pos)                   /**< (UART_MR) Receiver Digital Filter Mask */
#define UART_MR_FILTER                      UART_MR_FILTER_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_MR_FILTER_Msk instead */
#define   UART_MR_FILTER_DISABLED_Val       (0x0U)                                         /**< (UART_MR) UART does not filter the receive line.  */
#define   UART_MR_FILTER_ENABLED_Val        (0x1U)                                         /**< (UART_MR) UART filters the receive line using a three-sample filter (16x-bit clock) (2 over 3 majority).  */
#define UART_MR_FILTER_DISABLED             (UART_MR_FILTER_DISABLED_Val << UART_MR_FILTER_Pos)  /**< (UART_MR) UART does not filter the receive line. Position  */
#define UART_MR_FILTER_ENABLED              (UART_MR_FILTER_ENABLED_Val << UART_MR_FILTER_Pos)  /**< (UART_MR) UART filters the receive line using a three-sample filter (16x-bit clock) (2 over 3 majority). Position  */
#define UART_MR_PAR_Pos                     9                                              /**< (UART_MR) Parity Type Position */
#define UART_MR_PAR_Msk                     (0x7U << UART_MR_PAR_Pos)                      /**< (UART_MR) Parity Type Mask */
#define UART_MR_PAR(value)                  (UART_MR_PAR_Msk & ((value) << UART_MR_PAR_Pos))
#define   UART_MR_PAR_EVEN_Val              (0x0U)                                         /**< (UART_MR) Even Parity  */
#define   UART_MR_PAR_ODD_Val               (0x1U)                                         /**< (UART_MR) Odd Parity  */
#define   UART_MR_PAR_SPACE_Val             (0x2U)                                         /**< (UART_MR) Space: parity forced to 0  */
#define   UART_MR_PAR_MARK_Val              (0x3U)                                         /**< (UART_MR) Mark: parity forced to 1  */
#define   UART_MR_PAR_NO_Val                (0x4U)                                         /**< (UART_MR) No parity  */
#define UART_MR_PAR_EVEN                    (UART_MR_PAR_EVEN_Val << UART_MR_PAR_Pos)      /**< (UART_MR) Even Parity Position  */
#define UART_MR_PAR_ODD                     (UART_MR_PAR_ODD_Val << UART_MR_PAR_Pos)       /**< (UART_MR) Odd Parity Position  */
#define UART_MR_PAR_SPACE                   (UART_MR_PAR_SPACE_Val << UART_MR_PAR_Pos)     /**< (UART_MR) Space: parity forced to 0 Position  */
#define UART_MR_PAR_MARK                    (UART_MR_PAR_MARK_Val << UART_MR_PAR_Pos)      /**< (UART_MR) Mark: parity forced to 1 Position  */
#define UART_MR_PAR_NO                      (UART_MR_PAR_NO_Val << UART_MR_PAR_Pos)        /**< (UART_MR) No parity Position  */
#define UART_MR_BRSRCCK_Pos                 12                                             /**< (UART_MR) Baud Rate Source Clock Position */
#define UART_MR_BRSRCCK_Msk                 (0x1U << UART_MR_BRSRCCK_Pos)                  /**< (UART_MR) Baud Rate Source Clock Mask */
#define UART_MR_BRSRCCK                     UART_MR_BRSRCCK_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_MR_BRSRCCK_Msk instead */
#define   UART_MR_BRSRCCK_PERIPH_CLK_Val    (0x0U)                                         /**< (UART_MR) The baud rate is driven by the peripheral clock  */
#define   UART_MR_BRSRCCK_PMC_PCK_Val       (0x1U)                                         /**< (UART_MR) The baud rate is driven by a PMC programmable clock PCK (see section Power Management Controller (PMC)).  */
#define UART_MR_BRSRCCK_PERIPH_CLK          (UART_MR_BRSRCCK_PERIPH_CLK_Val << UART_MR_BRSRCCK_Pos)  /**< (UART_MR) The baud rate is driven by the peripheral clock Position  */
#define UART_MR_BRSRCCK_PMC_PCK             (UART_MR_BRSRCCK_PMC_PCK_Val << UART_MR_BRSRCCK_Pos)  /**< (UART_MR) The baud rate is driven by a PMC programmable clock PCK (see section Power Management Controller (PMC)). Position  */
#define UART_MR_CHMODE_Pos                  14                                             /**< (UART_MR) Channel Mode Position */
#define UART_MR_CHMODE_Msk                  (0x3U << UART_MR_CHMODE_Pos)                   /**< (UART_MR) Channel Mode Mask */
#define UART_MR_CHMODE(value)               (UART_MR_CHMODE_Msk & ((value) << UART_MR_CHMODE_Pos))
#define   UART_MR_CHMODE_NORMAL_Val         (0x0U)                                         /**< (UART_MR) Normal mode  */
#define   UART_MR_CHMODE_AUTOMATIC_Val      (0x1U)                                         /**< (UART_MR) Automatic echo  */
#define   UART_MR_CHMODE_LOCAL_LOOPBACK_Val (0x2U)                                         /**< (UART_MR) Local loopback  */
#define   UART_MR_CHMODE_REMOTE_LOOPBACK_Val (0x3U)                                         /**< (UART_MR) Remote loopback  */
#define UART_MR_CHMODE_NORMAL               (UART_MR_CHMODE_NORMAL_Val << UART_MR_CHMODE_Pos)  /**< (UART_MR) Normal mode Position  */
#define UART_MR_CHMODE_AUTOMATIC            (UART_MR_CHMODE_AUTOMATIC_Val << UART_MR_CHMODE_Pos)  /**< (UART_MR) Automatic echo Position  */
#define UART_MR_CHMODE_LOCAL_LOOPBACK       (UART_MR_CHMODE_LOCAL_LOOPBACK_Val << UART_MR_CHMODE_Pos)  /**< (UART_MR) Local loopback Position  */
#define UART_MR_CHMODE_REMOTE_LOOPBACK      (UART_MR_CHMODE_REMOTE_LOOPBACK_Val << UART_MR_CHMODE_Pos)  /**< (UART_MR) Remote loopback Position  */
#define UART_MR_MASK                        (0xDE10U)                                      /**< \deprecated (UART_MR) Register MASK  (Use UART_MR_Msk instead)  */
#define UART_MR_Msk                         (0xDE10U)                                      /**< (UART_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_IER : (UART Offset: 0x08) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXRDY:1;                   /**< bit:      0  Enable RXRDY Interrupt                   */
    uint32_t TXRDY:1;                   /**< bit:      1  Enable TXRDY Interrupt                   */
    uint32_t :3;                        /**< bit:   2..4  Reserved */
    uint32_t OVRE:1;                    /**< bit:      5  Enable Overrun Error Interrupt           */
    uint32_t FRAME:1;                   /**< bit:      6  Enable Framing Error Interrupt           */
    uint32_t PARE:1;                    /**< bit:      7  Enable Parity Error Interrupt            */
    uint32_t :1;                        /**< bit:      8  Reserved */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Enable TXEMPTY Interrupt                 */
    uint32_t :5;                        /**< bit: 10..14  Reserved */
    uint32_t CMP:1;                     /**< bit:     15  Enable Comparison Interrupt              */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_IER_OFFSET                     (0x08)                                        /**<  (UART_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_IER_RXRDY_Pos                  0                                              /**< (UART_IER) Enable RXRDY Interrupt Position */
#define UART_IER_RXRDY_Msk                  (0x1U << UART_IER_RXRDY_Pos)                   /**< (UART_IER) Enable RXRDY Interrupt Mask */
#define UART_IER_RXRDY                      UART_IER_RXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IER_RXRDY_Msk instead */
#define UART_IER_TXRDY_Pos                  1                                              /**< (UART_IER) Enable TXRDY Interrupt Position */
#define UART_IER_TXRDY_Msk                  (0x1U << UART_IER_TXRDY_Pos)                   /**< (UART_IER) Enable TXRDY Interrupt Mask */
#define UART_IER_TXRDY                      UART_IER_TXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IER_TXRDY_Msk instead */
#define UART_IER_OVRE_Pos                   5                                              /**< (UART_IER) Enable Overrun Error Interrupt Position */
#define UART_IER_OVRE_Msk                   (0x1U << UART_IER_OVRE_Pos)                    /**< (UART_IER) Enable Overrun Error Interrupt Mask */
#define UART_IER_OVRE                       UART_IER_OVRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IER_OVRE_Msk instead */
#define UART_IER_FRAME_Pos                  6                                              /**< (UART_IER) Enable Framing Error Interrupt Position */
#define UART_IER_FRAME_Msk                  (0x1U << UART_IER_FRAME_Pos)                   /**< (UART_IER) Enable Framing Error Interrupt Mask */
#define UART_IER_FRAME                      UART_IER_FRAME_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IER_FRAME_Msk instead */
#define UART_IER_PARE_Pos                   7                                              /**< (UART_IER) Enable Parity Error Interrupt Position */
#define UART_IER_PARE_Msk                   (0x1U << UART_IER_PARE_Pos)                    /**< (UART_IER) Enable Parity Error Interrupt Mask */
#define UART_IER_PARE                       UART_IER_PARE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IER_PARE_Msk instead */
#define UART_IER_TXEMPTY_Pos                9                                              /**< (UART_IER) Enable TXEMPTY Interrupt Position */
#define UART_IER_TXEMPTY_Msk                (0x1U << UART_IER_TXEMPTY_Pos)                 /**< (UART_IER) Enable TXEMPTY Interrupt Mask */
#define UART_IER_TXEMPTY                    UART_IER_TXEMPTY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IER_TXEMPTY_Msk instead */
#define UART_IER_CMP_Pos                    15                                             /**< (UART_IER) Enable Comparison Interrupt Position */
#define UART_IER_CMP_Msk                    (0x1U << UART_IER_CMP_Pos)                     /**< (UART_IER) Enable Comparison Interrupt Mask */
#define UART_IER_CMP                        UART_IER_CMP_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IER_CMP_Msk instead */
#define UART_IER_MASK                       (0x82E3U)                                      /**< \deprecated (UART_IER) Register MASK  (Use UART_IER_Msk instead)  */
#define UART_IER_Msk                        (0x82E3U)                                      /**< (UART_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_IDR : (UART Offset: 0x0c) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXRDY:1;                   /**< bit:      0  Disable RXRDY Interrupt                  */
    uint32_t TXRDY:1;                   /**< bit:      1  Disable TXRDY Interrupt                  */
    uint32_t :3;                        /**< bit:   2..4  Reserved */
    uint32_t OVRE:1;                    /**< bit:      5  Disable Overrun Error Interrupt          */
    uint32_t FRAME:1;                   /**< bit:      6  Disable Framing Error Interrupt          */
    uint32_t PARE:1;                    /**< bit:      7  Disable Parity Error Interrupt           */
    uint32_t :1;                        /**< bit:      8  Reserved */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Disable TXEMPTY Interrupt                */
    uint32_t :5;                        /**< bit: 10..14  Reserved */
    uint32_t CMP:1;                     /**< bit:     15  Disable Comparison Interrupt             */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_IDR_OFFSET                     (0x0C)                                        /**<  (UART_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_IDR_RXRDY_Pos                  0                                              /**< (UART_IDR) Disable RXRDY Interrupt Position */
#define UART_IDR_RXRDY_Msk                  (0x1U << UART_IDR_RXRDY_Pos)                   /**< (UART_IDR) Disable RXRDY Interrupt Mask */
#define UART_IDR_RXRDY                      UART_IDR_RXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IDR_RXRDY_Msk instead */
#define UART_IDR_TXRDY_Pos                  1                                              /**< (UART_IDR) Disable TXRDY Interrupt Position */
#define UART_IDR_TXRDY_Msk                  (0x1U << UART_IDR_TXRDY_Pos)                   /**< (UART_IDR) Disable TXRDY Interrupt Mask */
#define UART_IDR_TXRDY                      UART_IDR_TXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IDR_TXRDY_Msk instead */
#define UART_IDR_OVRE_Pos                   5                                              /**< (UART_IDR) Disable Overrun Error Interrupt Position */
#define UART_IDR_OVRE_Msk                   (0x1U << UART_IDR_OVRE_Pos)                    /**< (UART_IDR) Disable Overrun Error Interrupt Mask */
#define UART_IDR_OVRE                       UART_IDR_OVRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IDR_OVRE_Msk instead */
#define UART_IDR_FRAME_Pos                  6                                              /**< (UART_IDR) Disable Framing Error Interrupt Position */
#define UART_IDR_FRAME_Msk                  (0x1U << UART_IDR_FRAME_Pos)                   /**< (UART_IDR) Disable Framing Error Interrupt Mask */
#define UART_IDR_FRAME                      UART_IDR_FRAME_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IDR_FRAME_Msk instead */
#define UART_IDR_PARE_Pos                   7                                              /**< (UART_IDR) Disable Parity Error Interrupt Position */
#define UART_IDR_PARE_Msk                   (0x1U << UART_IDR_PARE_Pos)                    /**< (UART_IDR) Disable Parity Error Interrupt Mask */
#define UART_IDR_PARE                       UART_IDR_PARE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IDR_PARE_Msk instead */
#define UART_IDR_TXEMPTY_Pos                9                                              /**< (UART_IDR) Disable TXEMPTY Interrupt Position */
#define UART_IDR_TXEMPTY_Msk                (0x1U << UART_IDR_TXEMPTY_Pos)                 /**< (UART_IDR) Disable TXEMPTY Interrupt Mask */
#define UART_IDR_TXEMPTY                    UART_IDR_TXEMPTY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IDR_TXEMPTY_Msk instead */
#define UART_IDR_CMP_Pos                    15                                             /**< (UART_IDR) Disable Comparison Interrupt Position */
#define UART_IDR_CMP_Msk                    (0x1U << UART_IDR_CMP_Pos)                     /**< (UART_IDR) Disable Comparison Interrupt Mask */
#define UART_IDR_CMP                        UART_IDR_CMP_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IDR_CMP_Msk instead */
#define UART_IDR_MASK                       (0x82E3U)                                      /**< \deprecated (UART_IDR) Register MASK  (Use UART_IDR_Msk instead)  */
#define UART_IDR_Msk                        (0x82E3U)                                      /**< (UART_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_IMR : (UART Offset: 0x10) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXRDY:1;                   /**< bit:      0  Mask RXRDY Interrupt                     */
    uint32_t TXRDY:1;                   /**< bit:      1  Disable TXRDY Interrupt                  */
    uint32_t :3;                        /**< bit:   2..4  Reserved */
    uint32_t OVRE:1;                    /**< bit:      5  Mask Overrun Error Interrupt             */
    uint32_t FRAME:1;                   /**< bit:      6  Mask Framing Error Interrupt             */
    uint32_t PARE:1;                    /**< bit:      7  Mask Parity Error Interrupt              */
    uint32_t :1;                        /**< bit:      8  Reserved */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Mask TXEMPTY Interrupt                   */
    uint32_t :5;                        /**< bit: 10..14  Reserved */
    uint32_t CMP:1;                     /**< bit:     15  Mask Comparison Interrupt                */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_IMR_OFFSET                     (0x10)                                        /**<  (UART_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_IMR_RXRDY_Pos                  0                                              /**< (UART_IMR) Mask RXRDY Interrupt Position */
#define UART_IMR_RXRDY_Msk                  (0x1U << UART_IMR_RXRDY_Pos)                   /**< (UART_IMR) Mask RXRDY Interrupt Mask */
#define UART_IMR_RXRDY                      UART_IMR_RXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IMR_RXRDY_Msk instead */
#define UART_IMR_TXRDY_Pos                  1                                              /**< (UART_IMR) Disable TXRDY Interrupt Position */
#define UART_IMR_TXRDY_Msk                  (0x1U << UART_IMR_TXRDY_Pos)                   /**< (UART_IMR) Disable TXRDY Interrupt Mask */
#define UART_IMR_TXRDY                      UART_IMR_TXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IMR_TXRDY_Msk instead */
#define UART_IMR_OVRE_Pos                   5                                              /**< (UART_IMR) Mask Overrun Error Interrupt Position */
#define UART_IMR_OVRE_Msk                   (0x1U << UART_IMR_OVRE_Pos)                    /**< (UART_IMR) Mask Overrun Error Interrupt Mask */
#define UART_IMR_OVRE                       UART_IMR_OVRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IMR_OVRE_Msk instead */
#define UART_IMR_FRAME_Pos                  6                                              /**< (UART_IMR) Mask Framing Error Interrupt Position */
#define UART_IMR_FRAME_Msk                  (0x1U << UART_IMR_FRAME_Pos)                   /**< (UART_IMR) Mask Framing Error Interrupt Mask */
#define UART_IMR_FRAME                      UART_IMR_FRAME_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IMR_FRAME_Msk instead */
#define UART_IMR_PARE_Pos                   7                                              /**< (UART_IMR) Mask Parity Error Interrupt Position */
#define UART_IMR_PARE_Msk                   (0x1U << UART_IMR_PARE_Pos)                    /**< (UART_IMR) Mask Parity Error Interrupt Mask */
#define UART_IMR_PARE                       UART_IMR_PARE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IMR_PARE_Msk instead */
#define UART_IMR_TXEMPTY_Pos                9                                              /**< (UART_IMR) Mask TXEMPTY Interrupt Position */
#define UART_IMR_TXEMPTY_Msk                (0x1U << UART_IMR_TXEMPTY_Pos)                 /**< (UART_IMR) Mask TXEMPTY Interrupt Mask */
#define UART_IMR_TXEMPTY                    UART_IMR_TXEMPTY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IMR_TXEMPTY_Msk instead */
#define UART_IMR_CMP_Pos                    15                                             /**< (UART_IMR) Mask Comparison Interrupt Position */
#define UART_IMR_CMP_Msk                    (0x1U << UART_IMR_CMP_Pos)                     /**< (UART_IMR) Mask Comparison Interrupt Mask */
#define UART_IMR_CMP                        UART_IMR_CMP_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_IMR_CMP_Msk instead */
#define UART_IMR_MASK                       (0x82E3U)                                      /**< \deprecated (UART_IMR) Register MASK  (Use UART_IMR_Msk instead)  */
#define UART_IMR_Msk                        (0x82E3U)                                      /**< (UART_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_SR : (UART Offset: 0x14) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXRDY:1;                   /**< bit:      0  Receiver Ready                           */
    uint32_t TXRDY:1;                   /**< bit:      1  Transmitter Ready                        */
    uint32_t :3;                        /**< bit:   2..4  Reserved */
    uint32_t OVRE:1;                    /**< bit:      5  Overrun Error                            */
    uint32_t FRAME:1;                   /**< bit:      6  Framing Error                            */
    uint32_t PARE:1;                    /**< bit:      7  Parity Error                             */
    uint32_t :1;                        /**< bit:      8  Reserved */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Transmitter Empty                        */
    uint32_t :5;                        /**< bit: 10..14  Reserved */
    uint32_t CMP:1;                     /**< bit:     15  Comparison Match                         */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_SR_OFFSET                      (0x14)                                        /**<  (UART_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_SR_RXRDY_Pos                   0                                              /**< (UART_SR) Receiver Ready Position */
#define UART_SR_RXRDY_Msk                   (0x1U << UART_SR_RXRDY_Pos)                    /**< (UART_SR) Receiver Ready Mask */
#define UART_SR_RXRDY                       UART_SR_RXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_SR_RXRDY_Msk instead */
#define UART_SR_TXRDY_Pos                   1                                              /**< (UART_SR) Transmitter Ready Position */
#define UART_SR_TXRDY_Msk                   (0x1U << UART_SR_TXRDY_Pos)                    /**< (UART_SR) Transmitter Ready Mask */
#define UART_SR_TXRDY                       UART_SR_TXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_SR_TXRDY_Msk instead */
#define UART_SR_OVRE_Pos                    5                                              /**< (UART_SR) Overrun Error Position */
#define UART_SR_OVRE_Msk                    (0x1U << UART_SR_OVRE_Pos)                     /**< (UART_SR) Overrun Error Mask */
#define UART_SR_OVRE                        UART_SR_OVRE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_SR_OVRE_Msk instead */
#define UART_SR_FRAME_Pos                   6                                              /**< (UART_SR) Framing Error Position */
#define UART_SR_FRAME_Msk                   (0x1U << UART_SR_FRAME_Pos)                    /**< (UART_SR) Framing Error Mask */
#define UART_SR_FRAME                       UART_SR_FRAME_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_SR_FRAME_Msk instead */
#define UART_SR_PARE_Pos                    7                                              /**< (UART_SR) Parity Error Position */
#define UART_SR_PARE_Msk                    (0x1U << UART_SR_PARE_Pos)                     /**< (UART_SR) Parity Error Mask */
#define UART_SR_PARE                        UART_SR_PARE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_SR_PARE_Msk instead */
#define UART_SR_TXEMPTY_Pos                 9                                              /**< (UART_SR) Transmitter Empty Position */
#define UART_SR_TXEMPTY_Msk                 (0x1U << UART_SR_TXEMPTY_Pos)                  /**< (UART_SR) Transmitter Empty Mask */
#define UART_SR_TXEMPTY                     UART_SR_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_SR_TXEMPTY_Msk instead */
#define UART_SR_CMP_Pos                     15                                             /**< (UART_SR) Comparison Match Position */
#define UART_SR_CMP_Msk                     (0x1U << UART_SR_CMP_Pos)                      /**< (UART_SR) Comparison Match Mask */
#define UART_SR_CMP                         UART_SR_CMP_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_SR_CMP_Msk instead */
#define UART_SR_MASK                        (0x82E3U)                                      /**< \deprecated (UART_SR) Register MASK  (Use UART_SR_Msk instead)  */
#define UART_SR_Msk                         (0x82E3U)                                      /**< (UART_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_RHR : (UART Offset: 0x18) (R/ 32) Receive Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXCHR:8;                   /**< bit:   0..7  Received Character                       */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_RHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_RHR_OFFSET                     (0x18)                                        /**<  (UART_RHR) Receive Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_RHR_RXCHR_Pos                  0                                              /**< (UART_RHR) Received Character Position */
#define UART_RHR_RXCHR_Msk                  (0xFFU << UART_RHR_RXCHR_Pos)                  /**< (UART_RHR) Received Character Mask */
#define UART_RHR_RXCHR(value)               (UART_RHR_RXCHR_Msk & ((value) << UART_RHR_RXCHR_Pos))
#define UART_RHR_MASK                       (0xFFU)                                        /**< \deprecated (UART_RHR) Register MASK  (Use UART_RHR_Msk instead)  */
#define UART_RHR_Msk                        (0xFFU)                                        /**< (UART_RHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_THR : (UART Offset: 0x1c) (/W 32) Transmit Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXCHR:8;                   /**< bit:   0..7  Character to be Transmitted              */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_THR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_THR_OFFSET                     (0x1C)                                        /**<  (UART_THR) Transmit Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_THR_TXCHR_Pos                  0                                              /**< (UART_THR) Character to be Transmitted Position */
#define UART_THR_TXCHR_Msk                  (0xFFU << UART_THR_TXCHR_Pos)                  /**< (UART_THR) Character to be Transmitted Mask */
#define UART_THR_TXCHR(value)               (UART_THR_TXCHR_Msk & ((value) << UART_THR_TXCHR_Pos))
#define UART_THR_MASK                       (0xFFU)                                        /**< \deprecated (UART_THR) Register MASK  (Use UART_THR_Msk instead)  */
#define UART_THR_Msk                        (0xFFU)                                        /**< (UART_THR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_BRGR : (UART Offset: 0x20) (R/W 32) Baud Rate Generator Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CD:16;                     /**< bit:  0..15  Clock Divisor                            */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_BRGR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_BRGR_OFFSET                    (0x20)                                        /**<  (UART_BRGR) Baud Rate Generator Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_BRGR_CD_Pos                    0                                              /**< (UART_BRGR) Clock Divisor Position */
#define UART_BRGR_CD_Msk                    (0xFFFFU << UART_BRGR_CD_Pos)                  /**< (UART_BRGR) Clock Divisor Mask */
#define UART_BRGR_CD(value)                 (UART_BRGR_CD_Msk & ((value) << UART_BRGR_CD_Pos))
#define UART_BRGR_MASK                      (0xFFFFU)                                      /**< \deprecated (UART_BRGR) Register MASK  (Use UART_BRGR_Msk instead)  */
#define UART_BRGR_Msk                       (0xFFFFU)                                      /**< (UART_BRGR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_CMPR : (UART Offset: 0x24) (R/W 32) Comparison Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t VAL1:8;                    /**< bit:   0..7  First Comparison Value for Received Character */
    uint32_t :4;                        /**< bit:  8..11  Reserved */
    uint32_t CMPMODE:1;                 /**< bit:     12  Comparison Mode                          */
    uint32_t :1;                        /**< bit:     13  Reserved */
    uint32_t CMPPAR:1;                  /**< bit:     14  Compare Parity                           */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t VAL2:8;                    /**< bit: 16..23  Second Comparison Value for Received Character */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_CMPR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_CMPR_OFFSET                    (0x24)                                        /**<  (UART_CMPR) Comparison Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_CMPR_VAL1_Pos                  0                                              /**< (UART_CMPR) First Comparison Value for Received Character Position */
#define UART_CMPR_VAL1_Msk                  (0xFFU << UART_CMPR_VAL1_Pos)                  /**< (UART_CMPR) First Comparison Value for Received Character Mask */
#define UART_CMPR_VAL1(value)               (UART_CMPR_VAL1_Msk & ((value) << UART_CMPR_VAL1_Pos))
#define UART_CMPR_CMPMODE_Pos               12                                             /**< (UART_CMPR) Comparison Mode Position */
#define UART_CMPR_CMPMODE_Msk               (0x1U << UART_CMPR_CMPMODE_Pos)                /**< (UART_CMPR) Comparison Mode Mask */
#define UART_CMPR_CMPMODE                   UART_CMPR_CMPMODE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CMPR_CMPMODE_Msk instead */
#define   UART_CMPR_CMPMODE_FLAG_ONLY_Val   (0x0U)                                         /**< (UART_CMPR) Any character is received and comparison function drives CMP flag.  */
#define   UART_CMPR_CMPMODE_START_CONDITION_Val (0x1U)                                         /**< (UART_CMPR) Comparison condition must be met to start reception.  */
#define UART_CMPR_CMPMODE_FLAG_ONLY         (UART_CMPR_CMPMODE_FLAG_ONLY_Val << UART_CMPR_CMPMODE_Pos)  /**< (UART_CMPR) Any character is received and comparison function drives CMP flag. Position  */
#define UART_CMPR_CMPMODE_START_CONDITION   (UART_CMPR_CMPMODE_START_CONDITION_Val << UART_CMPR_CMPMODE_Pos)  /**< (UART_CMPR) Comparison condition must be met to start reception. Position  */
#define UART_CMPR_CMPPAR_Pos                14                                             /**< (UART_CMPR) Compare Parity Position */
#define UART_CMPR_CMPPAR_Msk                (0x1U << UART_CMPR_CMPPAR_Pos)                 /**< (UART_CMPR) Compare Parity Mask */
#define UART_CMPR_CMPPAR                    UART_CMPR_CMPPAR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_CMPR_CMPPAR_Msk instead */
#define UART_CMPR_VAL2_Pos                  16                                             /**< (UART_CMPR) Second Comparison Value for Received Character Position */
#define UART_CMPR_VAL2_Msk                  (0xFFU << UART_CMPR_VAL2_Pos)                  /**< (UART_CMPR) Second Comparison Value for Received Character Mask */
#define UART_CMPR_VAL2(value)               (UART_CMPR_VAL2_Msk & ((value) << UART_CMPR_VAL2_Pos))
#define UART_CMPR_MASK                      (0xFF50FFU)                                    /**< \deprecated (UART_CMPR) Register MASK  (Use UART_CMPR_Msk instead)  */
#define UART_CMPR_Msk                       (0xFF50FFU)                                    /**< (UART_CMPR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UART_WPMR : (UART Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UART_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UART_WPMR_OFFSET                    (0xE4)                                        /**<  (UART_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UART_WPMR_WPEN_Pos                  0                                              /**< (UART_WPMR) Write Protection Enable Position */
#define UART_WPMR_WPEN_Msk                  (0x1U << UART_WPMR_WPEN_Pos)                   /**< (UART_WPMR) Write Protection Enable Mask */
#define UART_WPMR_WPEN                      UART_WPMR_WPEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use UART_WPMR_WPEN_Msk instead */
#define UART_WPMR_WPKEY_Pos                 8                                              /**< (UART_WPMR) Write Protection Key Position */
#define UART_WPMR_WPKEY_Msk                 (0xFFFFFFU << UART_WPMR_WPKEY_Pos)             /**< (UART_WPMR) Write Protection Key Mask */
#define UART_WPMR_WPKEY(value)              (UART_WPMR_WPKEY_Msk & ((value) << UART_WPMR_WPKEY_Pos))
#define   UART_WPMR_WPKEY_PASSWD_Val        (0x554152U)                                    /**< (UART_WPMR) Writing any other value in this field aborts the write operation.Always reads as 0.  */
#define UART_WPMR_WPKEY_PASSWD              (UART_WPMR_WPKEY_PASSWD_Val << UART_WPMR_WPKEY_Pos)  /**< (UART_WPMR) Writing any other value in this field aborts the write operation.Always reads as 0. Position  */
#define UART_WPMR_MASK                      (0xFFFFFF01U)                                  /**< \deprecated (UART_WPMR) Register MASK  (Use UART_WPMR_Msk instead)  */
#define UART_WPMR_Msk                       (0xFFFFFF01U)                                  /**< (UART_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief UART hardware registers */
typedef struct {  
  __O  uint32_t UART_CR;        /**< (UART Offset: 0x00) Control Register */
  __IO uint32_t UART_MR;        /**< (UART Offset: 0x04) Mode Register */
  __O  uint32_t UART_IER;       /**< (UART Offset: 0x08) Interrupt Enable Register */
  __O  uint32_t UART_IDR;       /**< (UART Offset: 0x0C) Interrupt Disable Register */
  __I  uint32_t UART_IMR;       /**< (UART Offset: 0x10) Interrupt Mask Register */
  __I  uint32_t UART_SR;        /**< (UART Offset: 0x14) Status Register */
  __I  uint32_t UART_RHR;       /**< (UART Offset: 0x18) Receive Holding Register */
  __O  uint32_t UART_THR;       /**< (UART Offset: 0x1C) Transmit Holding Register */
  __IO uint32_t UART_BRGR;      /**< (UART Offset: 0x20) Baud Rate Generator Register */
  __IO uint32_t UART_CMPR;      /**< (UART Offset: 0x24) Comparison Register */
  __I  uint32_t Reserved1[47];
  __IO uint32_t UART_WPMR;      /**< (UART Offset: 0xE4) Write Protection Mode Register */
} Uart;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief UART hardware registers */
typedef struct {  
  __O  UART_CR_Type                   UART_CR;        /**< Offset: 0x00 ( /W  32) Control Register */
  __IO UART_MR_Type                   UART_MR;        /**< Offset: 0x04 (R/W  32) Mode Register */
  __O  UART_IER_Type                  UART_IER;       /**< Offset: 0x08 ( /W  32) Interrupt Enable Register */
  __O  UART_IDR_Type                  UART_IDR;       /**< Offset: 0x0C ( /W  32) Interrupt Disable Register */
  __I  UART_IMR_Type                  UART_IMR;       /**< Offset: 0x10 (R/   32) Interrupt Mask Register */
  __I  UART_SR_Type                   UART_SR;        /**< Offset: 0x14 (R/   32) Status Register */
  __I  UART_RHR_Type                  UART_RHR;       /**< Offset: 0x18 (R/   32) Receive Holding Register */
  __O  UART_THR_Type                  UART_THR;       /**< Offset: 0x1C ( /W  32) Transmit Holding Register */
  __IO UART_BRGR_Type                 UART_BRGR;      /**< Offset: 0x20 (R/W  32) Baud Rate Generator Register */
  __IO UART_CMPR_Type                 UART_CMPR;      /**< Offset: 0x24 (R/W  32) Comparison Register */
       RoReg8                         Reserved1[0xBC];
  __IO UART_WPMR_Type                 UART_WPMR;      /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
} Uart;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Universal Asynchronous Receiver Transmitter */

#endif /* _SAME70_UART_COMPONENT_H_ */
