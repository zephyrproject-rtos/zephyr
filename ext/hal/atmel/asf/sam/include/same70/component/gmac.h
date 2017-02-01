/**
 * \file
 *
 * \brief Component description for GMAC
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

#ifndef _SAME70_GMAC_COMPONENT_H_
#define _SAME70_GMAC_COMPONENT_H_
#define _SAME70_GMAC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_GMAC Gigabit Ethernet MAC
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR GMAC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define GMAC_11046                      /**< (GMAC) Module ID */
#define REV_GMAC N                      /**< (GMAC) Module revision */

/* -------- GMAC_SAB : (GMAC Offset: 0x00) (R/W 32) Specific Address 1 Bottom Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ADDR:32;                   /**< bit:  0..31  Specific Address 1                       */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SAB_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAB_OFFSET                     (0x00)                                        /**<  (GMAC_SAB) Specific Address 1 Bottom Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SAB_ADDR_Pos                   0                                              /**< (GMAC_SAB) Specific Address 1 Position */
#define GMAC_SAB_ADDR_Msk                   (0xFFFFFFFFU << GMAC_SAB_ADDR_Pos)             /**< (GMAC_SAB) Specific Address 1 Mask */
#define GMAC_SAB_ADDR(value)                (GMAC_SAB_ADDR_Msk & ((value) << GMAC_SAB_ADDR_Pos))
#define GMAC_SAB_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_SAB) Register MASK  (Use GMAC_SAB_Msk instead)  */
#define GMAC_SAB_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_SAB) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_SAT : (GMAC Offset: 0x04) (R/W 32) Specific Address 1 Top Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ADDR:16;                   /**< bit:  0..15  Specific Address 1                       */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SAT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAT_OFFSET                     (0x04)                                        /**<  (GMAC_SAT) Specific Address 1 Top Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SAT_ADDR_Pos                   0                                              /**< (GMAC_SAT) Specific Address 1 Position */
#define GMAC_SAT_ADDR_Msk                   (0xFFFFU << GMAC_SAT_ADDR_Pos)                 /**< (GMAC_SAT) Specific Address 1 Mask */
#define GMAC_SAT_ADDR(value)                (GMAC_SAT_ADDR_Msk & ((value) << GMAC_SAT_ADDR_Pos))
#define GMAC_SAT_MASK                       (0xFFFFU)                                      /**< \deprecated (GMAC_SAT) Register MASK  (Use GMAC_SAT_Msk instead)  */
#define GMAC_SAT_Msk                        (0xFFFFU)                                      /**< (GMAC_SAT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_NCR : (GMAC Offset: 0x00) (R/W 32) Network Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t LBL:1;                     /**< bit:      1  Loop Back Local                          */
    uint32_t RXEN:1;                    /**< bit:      2  Receive Enable                           */
    uint32_t TXEN:1;                    /**< bit:      3  Transmit Enable                          */
    uint32_t MPE:1;                     /**< bit:      4  Management Port Enable                   */
    uint32_t CLRSTAT:1;                 /**< bit:      5  Clear Statistics Registers               */
    uint32_t INCSTAT:1;                 /**< bit:      6  Increment Statistics Registers           */
    uint32_t WESTAT:1;                  /**< bit:      7  Write Enable for Statistics Registers    */
    uint32_t BP:1;                      /**< bit:      8  Back pressure                            */
    uint32_t TSTART:1;                  /**< bit:      9  Start Transmission                       */
    uint32_t THALT:1;                   /**< bit:     10  Transmit Halt                            */
    uint32_t TXPF:1;                    /**< bit:     11  Transmit Pause Frame                     */
    uint32_t TXZQPF:1;                  /**< bit:     12  Transmit Zero Quantum Pause Frame        */
    uint32_t :2;                        /**< bit: 13..14  Reserved */
    uint32_t SRTSM:1;                   /**< bit:     15  Store Receive Time Stamp to Memory       */
    uint32_t ENPBPR:1;                  /**< bit:     16  Enable PFC Priority-based Pause Reception */
    uint32_t TXPBPF:1;                  /**< bit:     17  Transmit PFC Priority-based Pause Frame  */
    uint32_t FNP:1;                     /**< bit:     18  Flush Next Packet                        */
    uint32_t :13;                       /**< bit: 19..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_NCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NCR_OFFSET                     (0x00)                                        /**<  (GMAC_NCR) Network Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_NCR_LBL_Pos                    1                                              /**< (GMAC_NCR) Loop Back Local Position */
#define GMAC_NCR_LBL_Msk                    (0x1U << GMAC_NCR_LBL_Pos)                     /**< (GMAC_NCR) Loop Back Local Mask */
#define GMAC_NCR_LBL                        GMAC_NCR_LBL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_LBL_Msk instead */
#define GMAC_NCR_RXEN_Pos                   2                                              /**< (GMAC_NCR) Receive Enable Position */
#define GMAC_NCR_RXEN_Msk                   (0x1U << GMAC_NCR_RXEN_Pos)                    /**< (GMAC_NCR) Receive Enable Mask */
#define GMAC_NCR_RXEN                       GMAC_NCR_RXEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_RXEN_Msk instead */
#define GMAC_NCR_TXEN_Pos                   3                                              /**< (GMAC_NCR) Transmit Enable Position */
#define GMAC_NCR_TXEN_Msk                   (0x1U << GMAC_NCR_TXEN_Pos)                    /**< (GMAC_NCR) Transmit Enable Mask */
#define GMAC_NCR_TXEN                       GMAC_NCR_TXEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_TXEN_Msk instead */
#define GMAC_NCR_MPE_Pos                    4                                              /**< (GMAC_NCR) Management Port Enable Position */
#define GMAC_NCR_MPE_Msk                    (0x1U << GMAC_NCR_MPE_Pos)                     /**< (GMAC_NCR) Management Port Enable Mask */
#define GMAC_NCR_MPE                        GMAC_NCR_MPE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_MPE_Msk instead */
#define GMAC_NCR_CLRSTAT_Pos                5                                              /**< (GMAC_NCR) Clear Statistics Registers Position */
#define GMAC_NCR_CLRSTAT_Msk                (0x1U << GMAC_NCR_CLRSTAT_Pos)                 /**< (GMAC_NCR) Clear Statistics Registers Mask */
#define GMAC_NCR_CLRSTAT                    GMAC_NCR_CLRSTAT_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_CLRSTAT_Msk instead */
#define GMAC_NCR_INCSTAT_Pos                6                                              /**< (GMAC_NCR) Increment Statistics Registers Position */
#define GMAC_NCR_INCSTAT_Msk                (0x1U << GMAC_NCR_INCSTAT_Pos)                 /**< (GMAC_NCR) Increment Statistics Registers Mask */
#define GMAC_NCR_INCSTAT                    GMAC_NCR_INCSTAT_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_INCSTAT_Msk instead */
#define GMAC_NCR_WESTAT_Pos                 7                                              /**< (GMAC_NCR) Write Enable for Statistics Registers Position */
#define GMAC_NCR_WESTAT_Msk                 (0x1U << GMAC_NCR_WESTAT_Pos)                  /**< (GMAC_NCR) Write Enable for Statistics Registers Mask */
#define GMAC_NCR_WESTAT                     GMAC_NCR_WESTAT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_WESTAT_Msk instead */
#define GMAC_NCR_BP_Pos                     8                                              /**< (GMAC_NCR) Back pressure Position */
#define GMAC_NCR_BP_Msk                     (0x1U << GMAC_NCR_BP_Pos)                      /**< (GMAC_NCR) Back pressure Mask */
#define GMAC_NCR_BP                         GMAC_NCR_BP_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_BP_Msk instead */
#define GMAC_NCR_TSTART_Pos                 9                                              /**< (GMAC_NCR) Start Transmission Position */
#define GMAC_NCR_TSTART_Msk                 (0x1U << GMAC_NCR_TSTART_Pos)                  /**< (GMAC_NCR) Start Transmission Mask */
#define GMAC_NCR_TSTART                     GMAC_NCR_TSTART_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_TSTART_Msk instead */
#define GMAC_NCR_THALT_Pos                  10                                             /**< (GMAC_NCR) Transmit Halt Position */
#define GMAC_NCR_THALT_Msk                  (0x1U << GMAC_NCR_THALT_Pos)                   /**< (GMAC_NCR) Transmit Halt Mask */
#define GMAC_NCR_THALT                      GMAC_NCR_THALT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_THALT_Msk instead */
#define GMAC_NCR_TXPF_Pos                   11                                             /**< (GMAC_NCR) Transmit Pause Frame Position */
#define GMAC_NCR_TXPF_Msk                   (0x1U << GMAC_NCR_TXPF_Pos)                    /**< (GMAC_NCR) Transmit Pause Frame Mask */
#define GMAC_NCR_TXPF                       GMAC_NCR_TXPF_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_TXPF_Msk instead */
#define GMAC_NCR_TXZQPF_Pos                 12                                             /**< (GMAC_NCR) Transmit Zero Quantum Pause Frame Position */
#define GMAC_NCR_TXZQPF_Msk                 (0x1U << GMAC_NCR_TXZQPF_Pos)                  /**< (GMAC_NCR) Transmit Zero Quantum Pause Frame Mask */
#define GMAC_NCR_TXZQPF                     GMAC_NCR_TXZQPF_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_TXZQPF_Msk instead */
#define GMAC_NCR_SRTSM_Pos                  15                                             /**< (GMAC_NCR) Store Receive Time Stamp to Memory Position */
#define GMAC_NCR_SRTSM_Msk                  (0x1U << GMAC_NCR_SRTSM_Pos)                   /**< (GMAC_NCR) Store Receive Time Stamp to Memory Mask */
#define GMAC_NCR_SRTSM                      GMAC_NCR_SRTSM_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_SRTSM_Msk instead */
#define GMAC_NCR_ENPBPR_Pos                 16                                             /**< (GMAC_NCR) Enable PFC Priority-based Pause Reception Position */
#define GMAC_NCR_ENPBPR_Msk                 (0x1U << GMAC_NCR_ENPBPR_Pos)                  /**< (GMAC_NCR) Enable PFC Priority-based Pause Reception Mask */
#define GMAC_NCR_ENPBPR                     GMAC_NCR_ENPBPR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_ENPBPR_Msk instead */
#define GMAC_NCR_TXPBPF_Pos                 17                                             /**< (GMAC_NCR) Transmit PFC Priority-based Pause Frame Position */
#define GMAC_NCR_TXPBPF_Msk                 (0x1U << GMAC_NCR_TXPBPF_Pos)                  /**< (GMAC_NCR) Transmit PFC Priority-based Pause Frame Mask */
#define GMAC_NCR_TXPBPF                     GMAC_NCR_TXPBPF_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_TXPBPF_Msk instead */
#define GMAC_NCR_FNP_Pos                    18                                             /**< (GMAC_NCR) Flush Next Packet Position */
#define GMAC_NCR_FNP_Msk                    (0x1U << GMAC_NCR_FNP_Pos)                     /**< (GMAC_NCR) Flush Next Packet Mask */
#define GMAC_NCR_FNP                        GMAC_NCR_FNP_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCR_FNP_Msk instead */
#define GMAC_NCR_MASK                       (0x79FFEU)                                     /**< \deprecated (GMAC_NCR) Register MASK  (Use GMAC_NCR_Msk instead)  */
#define GMAC_NCR_Msk                        (0x79FFEU)                                     /**< (GMAC_NCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_NCFGR : (GMAC Offset: 0x04) (R/W 32) Network Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SPD:1;                     /**< bit:      0  Speed                                    */
    uint32_t FD:1;                      /**< bit:      1  Full Duplex                              */
    uint32_t DNVLAN:1;                  /**< bit:      2  Discard Non-VLAN FRAMES                  */
    uint32_t JFRAME:1;                  /**< bit:      3  Jumbo Frame Size                         */
    uint32_t CAF:1;                     /**< bit:      4  Copy All Frames                          */
    uint32_t NBC:1;                     /**< bit:      5  No Broadcast                             */
    uint32_t MTIHEN:1;                  /**< bit:      6  Multicast Hash Enable                    */
    uint32_t UNIHEN:1;                  /**< bit:      7  Unicast Hash Enable                      */
    uint32_t MAXFS:1;                   /**< bit:      8  1536 Maximum Frame Size                  */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t RTY:1;                     /**< bit:     12  Retry Test                               */
    uint32_t PEN:1;                     /**< bit:     13  Pause Enable                             */
    uint32_t RXBUFO:2;                  /**< bit: 14..15  Receive Buffer Offset                    */
    uint32_t LFERD:1;                   /**< bit:     16  Length Field Error Frame Discard         */
    uint32_t RFCS:1;                    /**< bit:     17  Remove FCS                               */
    uint32_t CLK:3;                     /**< bit: 18..20  MDC CLock Division                       */
    uint32_t DBW:2;                     /**< bit: 21..22  Data Bus Width                           */
    uint32_t DCPF:1;                    /**< bit:     23  Disable Copy of Pause Frames             */
    uint32_t RXCOEN:1;                  /**< bit:     24  Receive Checksum Offload Enable          */
    uint32_t EFRHD:1;                   /**< bit:     25  Enable Frames Received in Half Duplex    */
    uint32_t IRXFCS:1;                  /**< bit:     26  Ignore RX FCS                            */
    uint32_t :1;                        /**< bit:     27  Reserved */
    uint32_t IPGSEN:1;                  /**< bit:     28  IP Stretch Enable                        */
    uint32_t RXBP:1;                    /**< bit:     29  Receive Bad Preamble                     */
    uint32_t IRXER:1;                   /**< bit:     30  Ignore IPG GRXER                         */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_NCFGR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NCFGR_OFFSET                   (0x04)                                        /**<  (GMAC_NCFGR) Network Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_NCFGR_SPD_Pos                  0                                              /**< (GMAC_NCFGR) Speed Position */
#define GMAC_NCFGR_SPD_Msk                  (0x1U << GMAC_NCFGR_SPD_Pos)                   /**< (GMAC_NCFGR) Speed Mask */
#define GMAC_NCFGR_SPD                      GMAC_NCFGR_SPD_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_SPD_Msk instead */
#define GMAC_NCFGR_FD_Pos                   1                                              /**< (GMAC_NCFGR) Full Duplex Position */
#define GMAC_NCFGR_FD_Msk                   (0x1U << GMAC_NCFGR_FD_Pos)                    /**< (GMAC_NCFGR) Full Duplex Mask */
#define GMAC_NCFGR_FD                       GMAC_NCFGR_FD_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_FD_Msk instead */
#define GMAC_NCFGR_DNVLAN_Pos               2                                              /**< (GMAC_NCFGR) Discard Non-VLAN FRAMES Position */
#define GMAC_NCFGR_DNVLAN_Msk               (0x1U << GMAC_NCFGR_DNVLAN_Pos)                /**< (GMAC_NCFGR) Discard Non-VLAN FRAMES Mask */
#define GMAC_NCFGR_DNVLAN                   GMAC_NCFGR_DNVLAN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_DNVLAN_Msk instead */
#define GMAC_NCFGR_JFRAME_Pos               3                                              /**< (GMAC_NCFGR) Jumbo Frame Size Position */
#define GMAC_NCFGR_JFRAME_Msk               (0x1U << GMAC_NCFGR_JFRAME_Pos)                /**< (GMAC_NCFGR) Jumbo Frame Size Mask */
#define GMAC_NCFGR_JFRAME                   GMAC_NCFGR_JFRAME_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_JFRAME_Msk instead */
#define GMAC_NCFGR_CAF_Pos                  4                                              /**< (GMAC_NCFGR) Copy All Frames Position */
#define GMAC_NCFGR_CAF_Msk                  (0x1U << GMAC_NCFGR_CAF_Pos)                   /**< (GMAC_NCFGR) Copy All Frames Mask */
#define GMAC_NCFGR_CAF                      GMAC_NCFGR_CAF_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_CAF_Msk instead */
#define GMAC_NCFGR_NBC_Pos                  5                                              /**< (GMAC_NCFGR) No Broadcast Position */
#define GMAC_NCFGR_NBC_Msk                  (0x1U << GMAC_NCFGR_NBC_Pos)                   /**< (GMAC_NCFGR) No Broadcast Mask */
#define GMAC_NCFGR_NBC                      GMAC_NCFGR_NBC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_NBC_Msk instead */
#define GMAC_NCFGR_MTIHEN_Pos               6                                              /**< (GMAC_NCFGR) Multicast Hash Enable Position */
#define GMAC_NCFGR_MTIHEN_Msk               (0x1U << GMAC_NCFGR_MTIHEN_Pos)                /**< (GMAC_NCFGR) Multicast Hash Enable Mask */
#define GMAC_NCFGR_MTIHEN                   GMAC_NCFGR_MTIHEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_MTIHEN_Msk instead */
#define GMAC_NCFGR_UNIHEN_Pos               7                                              /**< (GMAC_NCFGR) Unicast Hash Enable Position */
#define GMAC_NCFGR_UNIHEN_Msk               (0x1U << GMAC_NCFGR_UNIHEN_Pos)                /**< (GMAC_NCFGR) Unicast Hash Enable Mask */
#define GMAC_NCFGR_UNIHEN                   GMAC_NCFGR_UNIHEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_UNIHEN_Msk instead */
#define GMAC_NCFGR_MAXFS_Pos                8                                              /**< (GMAC_NCFGR) 1536 Maximum Frame Size Position */
#define GMAC_NCFGR_MAXFS_Msk                (0x1U << GMAC_NCFGR_MAXFS_Pos)                 /**< (GMAC_NCFGR) 1536 Maximum Frame Size Mask */
#define GMAC_NCFGR_MAXFS                    GMAC_NCFGR_MAXFS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_MAXFS_Msk instead */
#define GMAC_NCFGR_RTY_Pos                  12                                             /**< (GMAC_NCFGR) Retry Test Position */
#define GMAC_NCFGR_RTY_Msk                  (0x1U << GMAC_NCFGR_RTY_Pos)                   /**< (GMAC_NCFGR) Retry Test Mask */
#define GMAC_NCFGR_RTY                      GMAC_NCFGR_RTY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_RTY_Msk instead */
#define GMAC_NCFGR_PEN_Pos                  13                                             /**< (GMAC_NCFGR) Pause Enable Position */
#define GMAC_NCFGR_PEN_Msk                  (0x1U << GMAC_NCFGR_PEN_Pos)                   /**< (GMAC_NCFGR) Pause Enable Mask */
#define GMAC_NCFGR_PEN                      GMAC_NCFGR_PEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_PEN_Msk instead */
#define GMAC_NCFGR_RXBUFO_Pos               14                                             /**< (GMAC_NCFGR) Receive Buffer Offset Position */
#define GMAC_NCFGR_RXBUFO_Msk               (0x3U << GMAC_NCFGR_RXBUFO_Pos)                /**< (GMAC_NCFGR) Receive Buffer Offset Mask */
#define GMAC_NCFGR_RXBUFO(value)            (GMAC_NCFGR_RXBUFO_Msk & ((value) << GMAC_NCFGR_RXBUFO_Pos))
#define GMAC_NCFGR_LFERD_Pos                16                                             /**< (GMAC_NCFGR) Length Field Error Frame Discard Position */
#define GMAC_NCFGR_LFERD_Msk                (0x1U << GMAC_NCFGR_LFERD_Pos)                 /**< (GMAC_NCFGR) Length Field Error Frame Discard Mask */
#define GMAC_NCFGR_LFERD                    GMAC_NCFGR_LFERD_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_LFERD_Msk instead */
#define GMAC_NCFGR_RFCS_Pos                 17                                             /**< (GMAC_NCFGR) Remove FCS Position */
#define GMAC_NCFGR_RFCS_Msk                 (0x1U << GMAC_NCFGR_RFCS_Pos)                  /**< (GMAC_NCFGR) Remove FCS Mask */
#define GMAC_NCFGR_RFCS                     GMAC_NCFGR_RFCS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_RFCS_Msk instead */
#define GMAC_NCFGR_CLK_Pos                  18                                             /**< (GMAC_NCFGR) MDC CLock Division Position */
#define GMAC_NCFGR_CLK_Msk                  (0x7U << GMAC_NCFGR_CLK_Pos)                   /**< (GMAC_NCFGR) MDC CLock Division Mask */
#define GMAC_NCFGR_CLK(value)               (GMAC_NCFGR_CLK_Msk & ((value) << GMAC_NCFGR_CLK_Pos))
#define   GMAC_NCFGR_CLK_MCK_8_Val          (0x0U)                                         /**< (GMAC_NCFGR) MCK divided by 8 (MCK up to 20 MHz)  */
#define   GMAC_NCFGR_CLK_MCK_16_Val         (0x1U)                                         /**< (GMAC_NCFGR) MCK divided by 16 (MCK up to 40 MHz)  */
#define   GMAC_NCFGR_CLK_MCK_32_Val         (0x2U)                                         /**< (GMAC_NCFGR) MCK divided by 32 (MCK up to 80 MHz)  */
#define   GMAC_NCFGR_CLK_MCK_48_Val         (0x3U)                                         /**< (GMAC_NCFGR) MCK divided by 48 (MCK up to 120 MHz)  */
#define   GMAC_NCFGR_CLK_MCK_64_Val         (0x4U)                                         /**< (GMAC_NCFGR) MCK divided by 64 (MCK up to 160 MHz)  */
#define   GMAC_NCFGR_CLK_MCK_96_Val         (0x5U)                                         /**< (GMAC_NCFGR) MCK divided by 96 (MCK up to 240 MHz)  */
#define GMAC_NCFGR_CLK_MCK_8                (GMAC_NCFGR_CLK_MCK_8_Val << GMAC_NCFGR_CLK_Pos)  /**< (GMAC_NCFGR) MCK divided by 8 (MCK up to 20 MHz) Position  */
#define GMAC_NCFGR_CLK_MCK_16               (GMAC_NCFGR_CLK_MCK_16_Val << GMAC_NCFGR_CLK_Pos)  /**< (GMAC_NCFGR) MCK divided by 16 (MCK up to 40 MHz) Position  */
#define GMAC_NCFGR_CLK_MCK_32               (GMAC_NCFGR_CLK_MCK_32_Val << GMAC_NCFGR_CLK_Pos)  /**< (GMAC_NCFGR) MCK divided by 32 (MCK up to 80 MHz) Position  */
#define GMAC_NCFGR_CLK_MCK_48               (GMAC_NCFGR_CLK_MCK_48_Val << GMAC_NCFGR_CLK_Pos)  /**< (GMAC_NCFGR) MCK divided by 48 (MCK up to 120 MHz) Position  */
#define GMAC_NCFGR_CLK_MCK_64               (GMAC_NCFGR_CLK_MCK_64_Val << GMAC_NCFGR_CLK_Pos)  /**< (GMAC_NCFGR) MCK divided by 64 (MCK up to 160 MHz) Position  */
#define GMAC_NCFGR_CLK_MCK_96               (GMAC_NCFGR_CLK_MCK_96_Val << GMAC_NCFGR_CLK_Pos)  /**< (GMAC_NCFGR) MCK divided by 96 (MCK up to 240 MHz) Position  */
#define GMAC_NCFGR_DBW_Pos                  21                                             /**< (GMAC_NCFGR) Data Bus Width Position */
#define GMAC_NCFGR_DBW_Msk                  (0x3U << GMAC_NCFGR_DBW_Pos)                   /**< (GMAC_NCFGR) Data Bus Width Mask */
#define GMAC_NCFGR_DBW(value)               (GMAC_NCFGR_DBW_Msk & ((value) << GMAC_NCFGR_DBW_Pos))
#define GMAC_NCFGR_DCPF_Pos                 23                                             /**< (GMAC_NCFGR) Disable Copy of Pause Frames Position */
#define GMAC_NCFGR_DCPF_Msk                 (0x1U << GMAC_NCFGR_DCPF_Pos)                  /**< (GMAC_NCFGR) Disable Copy of Pause Frames Mask */
#define GMAC_NCFGR_DCPF                     GMAC_NCFGR_DCPF_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_DCPF_Msk instead */
#define GMAC_NCFGR_RXCOEN_Pos               24                                             /**< (GMAC_NCFGR) Receive Checksum Offload Enable Position */
#define GMAC_NCFGR_RXCOEN_Msk               (0x1U << GMAC_NCFGR_RXCOEN_Pos)                /**< (GMAC_NCFGR) Receive Checksum Offload Enable Mask */
#define GMAC_NCFGR_RXCOEN                   GMAC_NCFGR_RXCOEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_RXCOEN_Msk instead */
#define GMAC_NCFGR_EFRHD_Pos                25                                             /**< (GMAC_NCFGR) Enable Frames Received in Half Duplex Position */
#define GMAC_NCFGR_EFRHD_Msk                (0x1U << GMAC_NCFGR_EFRHD_Pos)                 /**< (GMAC_NCFGR) Enable Frames Received in Half Duplex Mask */
#define GMAC_NCFGR_EFRHD                    GMAC_NCFGR_EFRHD_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_EFRHD_Msk instead */
#define GMAC_NCFGR_IRXFCS_Pos               26                                             /**< (GMAC_NCFGR) Ignore RX FCS Position */
#define GMAC_NCFGR_IRXFCS_Msk               (0x1U << GMAC_NCFGR_IRXFCS_Pos)                /**< (GMAC_NCFGR) Ignore RX FCS Mask */
#define GMAC_NCFGR_IRXFCS                   GMAC_NCFGR_IRXFCS_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_IRXFCS_Msk instead */
#define GMAC_NCFGR_IPGSEN_Pos               28                                             /**< (GMAC_NCFGR) IP Stretch Enable Position */
#define GMAC_NCFGR_IPGSEN_Msk               (0x1U << GMAC_NCFGR_IPGSEN_Pos)                /**< (GMAC_NCFGR) IP Stretch Enable Mask */
#define GMAC_NCFGR_IPGSEN                   GMAC_NCFGR_IPGSEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_IPGSEN_Msk instead */
#define GMAC_NCFGR_RXBP_Pos                 29                                             /**< (GMAC_NCFGR) Receive Bad Preamble Position */
#define GMAC_NCFGR_RXBP_Msk                 (0x1U << GMAC_NCFGR_RXBP_Pos)                  /**< (GMAC_NCFGR) Receive Bad Preamble Mask */
#define GMAC_NCFGR_RXBP                     GMAC_NCFGR_RXBP_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_RXBP_Msk instead */
#define GMAC_NCFGR_IRXER_Pos                30                                             /**< (GMAC_NCFGR) Ignore IPG GRXER Position */
#define GMAC_NCFGR_IRXER_Msk                (0x1U << GMAC_NCFGR_IRXER_Pos)                 /**< (GMAC_NCFGR) Ignore IPG GRXER Mask */
#define GMAC_NCFGR_IRXER                    GMAC_NCFGR_IRXER_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NCFGR_IRXER_Msk instead */
#define GMAC_NCFGR_MASK                     (0x77FFF1FFU)                                  /**< \deprecated (GMAC_NCFGR) Register MASK  (Use GMAC_NCFGR_Msk instead)  */
#define GMAC_NCFGR_Msk                      (0x77FFF1FFU)                                  /**< (GMAC_NCFGR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_NSR : (GMAC Offset: 0x08) (R/ 32) Network Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t MDIO:1;                    /**< bit:      1  MDIO Input Status                        */
    uint32_t IDLE:1;                    /**< bit:      2  PHY Management Logic Idle                */
    uint32_t :29;                       /**< bit:  3..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_NSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NSR_OFFSET                     (0x08)                                        /**<  (GMAC_NSR) Network Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_NSR_MDIO_Pos                   1                                              /**< (GMAC_NSR) MDIO Input Status Position */
#define GMAC_NSR_MDIO_Msk                   (0x1U << GMAC_NSR_MDIO_Pos)                    /**< (GMAC_NSR) MDIO Input Status Mask */
#define GMAC_NSR_MDIO                       GMAC_NSR_MDIO_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NSR_MDIO_Msk instead */
#define GMAC_NSR_IDLE_Pos                   2                                              /**< (GMAC_NSR) PHY Management Logic Idle Position */
#define GMAC_NSR_IDLE_Msk                   (0x1U << GMAC_NSR_IDLE_Pos)                    /**< (GMAC_NSR) PHY Management Logic Idle Mask */
#define GMAC_NSR_IDLE                       GMAC_NSR_IDLE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_NSR_IDLE_Msk instead */
#define GMAC_NSR_MASK                       (0x06U)                                        /**< \deprecated (GMAC_NSR) Register MASK  (Use GMAC_NSR_Msk instead)  */
#define GMAC_NSR_Msk                        (0x06U)                                        /**< (GMAC_NSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_UR : (GMAC Offset: 0x0c) (R/W 32) User Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RMII:1;                    /**< bit:      0  Reduced MII Mode                         */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_UR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_UR_OFFSET                      (0x0C)                                        /**<  (GMAC_UR) User Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_UR_RMII_Pos                    0                                              /**< (GMAC_UR) Reduced MII Mode Position */
#define GMAC_UR_RMII_Msk                    (0x1U << GMAC_UR_RMII_Pos)                     /**< (GMAC_UR) Reduced MII Mode Mask */
#define GMAC_UR_RMII                        GMAC_UR_RMII_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_UR_RMII_Msk instead */
#define GMAC_UR_MASK                        (0x01U)                                        /**< \deprecated (GMAC_UR) Register MASK  (Use GMAC_UR_Msk instead)  */
#define GMAC_UR_Msk                         (0x01U)                                        /**< (GMAC_UR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_DCFGR : (GMAC Offset: 0x10) (R/W 32) DMA Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FBLDO:5;                   /**< bit:   0..4  Fixed Burst Length for DMA Data Operations: */
    uint32_t :1;                        /**< bit:      5  Reserved */
    uint32_t ESMA:1;                    /**< bit:      6  Endian Swap Mode Enable for Management Descriptor Accesses */
    uint32_t ESPA:1;                    /**< bit:      7  Endian Swap Mode Enable for Packet Data Accesses */
    uint32_t RXBMS:2;                   /**< bit:   8..9  Receiver Packet Buffer Memory Size Select */
    uint32_t TXPBMS:1;                  /**< bit:     10  Transmitter Packet Buffer Memory Size Select */
    uint32_t TXCOEN:1;                  /**< bit:     11  Transmitter Checksum Generation Offload Enable */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t DRBS:8;                    /**< bit: 16..23  DMA Receive Buffer Size                  */
    uint32_t DDRP:1;                    /**< bit:     24  DMA Discard Receive Packets              */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_DCFGR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_DCFGR_OFFSET                   (0x10)                                        /**<  (GMAC_DCFGR) DMA Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_DCFGR_FBLDO_Pos                0                                              /**< (GMAC_DCFGR) Fixed Burst Length for DMA Data Operations: Position */
#define GMAC_DCFGR_FBLDO_Msk                (0x1FU << GMAC_DCFGR_FBLDO_Pos)                /**< (GMAC_DCFGR) Fixed Burst Length for DMA Data Operations: Mask */
#define GMAC_DCFGR_FBLDO(value)             (GMAC_DCFGR_FBLDO_Msk & ((value) << GMAC_DCFGR_FBLDO_Pos))
#define   GMAC_DCFGR_FBLDO_SINGLE_Val       (0x1U)                                         /**< (GMAC_DCFGR) 00001: Always use SINGLE AHB bursts  */
#define   GMAC_DCFGR_FBLDO_INCR4_Val        (0x4U)                                         /**< (GMAC_DCFGR) 001xx: Attempt to use INCR4 AHB bursts (Default)  */
#define   GMAC_DCFGR_FBLDO_INCR8_Val        (0x8U)                                         /**< (GMAC_DCFGR) 01xxx: Attempt to use INCR8 AHB bursts  */
#define   GMAC_DCFGR_FBLDO_INCR16_Val       (0x10U)                                        /**< (GMAC_DCFGR) 1xxxx: Attempt to use INCR16 AHB bursts  */
#define GMAC_DCFGR_FBLDO_SINGLE             (GMAC_DCFGR_FBLDO_SINGLE_Val << GMAC_DCFGR_FBLDO_Pos)  /**< (GMAC_DCFGR) 00001: Always use SINGLE AHB bursts Position  */
#define GMAC_DCFGR_FBLDO_INCR4              (GMAC_DCFGR_FBLDO_INCR4_Val << GMAC_DCFGR_FBLDO_Pos)  /**< (GMAC_DCFGR) 001xx: Attempt to use INCR4 AHB bursts (Default) Position  */
#define GMAC_DCFGR_FBLDO_INCR8              (GMAC_DCFGR_FBLDO_INCR8_Val << GMAC_DCFGR_FBLDO_Pos)  /**< (GMAC_DCFGR) 01xxx: Attempt to use INCR8 AHB bursts Position  */
#define GMAC_DCFGR_FBLDO_INCR16             (GMAC_DCFGR_FBLDO_INCR16_Val << GMAC_DCFGR_FBLDO_Pos)  /**< (GMAC_DCFGR) 1xxxx: Attempt to use INCR16 AHB bursts Position  */
#define GMAC_DCFGR_ESMA_Pos                 6                                              /**< (GMAC_DCFGR) Endian Swap Mode Enable for Management Descriptor Accesses Position */
#define GMAC_DCFGR_ESMA_Msk                 (0x1U << GMAC_DCFGR_ESMA_Pos)                  /**< (GMAC_DCFGR) Endian Swap Mode Enable for Management Descriptor Accesses Mask */
#define GMAC_DCFGR_ESMA                     GMAC_DCFGR_ESMA_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_DCFGR_ESMA_Msk instead */
#define GMAC_DCFGR_ESPA_Pos                 7                                              /**< (GMAC_DCFGR) Endian Swap Mode Enable for Packet Data Accesses Position */
#define GMAC_DCFGR_ESPA_Msk                 (0x1U << GMAC_DCFGR_ESPA_Pos)                  /**< (GMAC_DCFGR) Endian Swap Mode Enable for Packet Data Accesses Mask */
#define GMAC_DCFGR_ESPA                     GMAC_DCFGR_ESPA_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_DCFGR_ESPA_Msk instead */
#define GMAC_DCFGR_RXBMS_Pos                8                                              /**< (GMAC_DCFGR) Receiver Packet Buffer Memory Size Select Position */
#define GMAC_DCFGR_RXBMS_Msk                (0x3U << GMAC_DCFGR_RXBMS_Pos)                 /**< (GMAC_DCFGR) Receiver Packet Buffer Memory Size Select Mask */
#define GMAC_DCFGR_RXBMS(value)             (GMAC_DCFGR_RXBMS_Msk & ((value) << GMAC_DCFGR_RXBMS_Pos))
#define   GMAC_DCFGR_RXBMS_EIGHTH_Val       (0x0U)                                         /**< (GMAC_DCFGR) 4/8 Kbyte Memory Size  */
#define   GMAC_DCFGR_RXBMS_QUARTER_Val      (0x1U)                                         /**< (GMAC_DCFGR) 4/4 Kbytes Memory Size  */
#define   GMAC_DCFGR_RXBMS_HALF_Val         (0x2U)                                         /**< (GMAC_DCFGR) 4/2 Kbytes Memory Size  */
#define   GMAC_DCFGR_RXBMS_FULL_Val         (0x3U)                                         /**< (GMAC_DCFGR) 4 Kbytes Memory Size  */
#define GMAC_DCFGR_RXBMS_EIGHTH             (GMAC_DCFGR_RXBMS_EIGHTH_Val << GMAC_DCFGR_RXBMS_Pos)  /**< (GMAC_DCFGR) 4/8 Kbyte Memory Size Position  */
#define GMAC_DCFGR_RXBMS_QUARTER            (GMAC_DCFGR_RXBMS_QUARTER_Val << GMAC_DCFGR_RXBMS_Pos)  /**< (GMAC_DCFGR) 4/4 Kbytes Memory Size Position  */
#define GMAC_DCFGR_RXBMS_HALF               (GMAC_DCFGR_RXBMS_HALF_Val << GMAC_DCFGR_RXBMS_Pos)  /**< (GMAC_DCFGR) 4/2 Kbytes Memory Size Position  */
#define GMAC_DCFGR_RXBMS_FULL               (GMAC_DCFGR_RXBMS_FULL_Val << GMAC_DCFGR_RXBMS_Pos)  /**< (GMAC_DCFGR) 4 Kbytes Memory Size Position  */
#define GMAC_DCFGR_TXPBMS_Pos               10                                             /**< (GMAC_DCFGR) Transmitter Packet Buffer Memory Size Select Position */
#define GMAC_DCFGR_TXPBMS_Msk               (0x1U << GMAC_DCFGR_TXPBMS_Pos)                /**< (GMAC_DCFGR) Transmitter Packet Buffer Memory Size Select Mask */
#define GMAC_DCFGR_TXPBMS                   GMAC_DCFGR_TXPBMS_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_DCFGR_TXPBMS_Msk instead */
#define GMAC_DCFGR_TXCOEN_Pos               11                                             /**< (GMAC_DCFGR) Transmitter Checksum Generation Offload Enable Position */
#define GMAC_DCFGR_TXCOEN_Msk               (0x1U << GMAC_DCFGR_TXCOEN_Pos)                /**< (GMAC_DCFGR) Transmitter Checksum Generation Offload Enable Mask */
#define GMAC_DCFGR_TXCOEN                   GMAC_DCFGR_TXCOEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_DCFGR_TXCOEN_Msk instead */
#define GMAC_DCFGR_DRBS_Pos                 16                                             /**< (GMAC_DCFGR) DMA Receive Buffer Size Position */
#define GMAC_DCFGR_DRBS_Msk                 (0xFFU << GMAC_DCFGR_DRBS_Pos)                 /**< (GMAC_DCFGR) DMA Receive Buffer Size Mask */
#define GMAC_DCFGR_DRBS(value)              (GMAC_DCFGR_DRBS_Msk & ((value) << GMAC_DCFGR_DRBS_Pos))
#define GMAC_DCFGR_DDRP_Pos                 24                                             /**< (GMAC_DCFGR) DMA Discard Receive Packets Position */
#define GMAC_DCFGR_DDRP_Msk                 (0x1U << GMAC_DCFGR_DDRP_Pos)                  /**< (GMAC_DCFGR) DMA Discard Receive Packets Mask */
#define GMAC_DCFGR_DDRP                     GMAC_DCFGR_DDRP_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_DCFGR_DDRP_Msk instead */
#define GMAC_DCFGR_MASK                     (0x1FF0FDFU)                                   /**< \deprecated (GMAC_DCFGR) Register MASK  (Use GMAC_DCFGR_Msk instead)  */
#define GMAC_DCFGR_Msk                      (0x1FF0FDFU)                                   /**< (GMAC_DCFGR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TSR : (GMAC Offset: 0x14) (R/W 32) Transmit Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UBR:1;                     /**< bit:      0  Used Bit Read                            */
    uint32_t COL:1;                     /**< bit:      1  Collision Occurred                       */
    uint32_t RLE:1;                     /**< bit:      2  Retry Limit Exceeded                     */
    uint32_t TXGO:1;                    /**< bit:      3  Transmit Go                              */
    uint32_t TFC:1;                     /**< bit:      4  Transmit Frame Corruption Due to AHB Error */
    uint32_t TXCOMP:1;                  /**< bit:      5  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t HRESP:1;                   /**< bit:      8  HRESP Not OK                             */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSR_OFFSET                     (0x14)                                        /**<  (GMAC_TSR) Transmit Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TSR_UBR_Pos                    0                                              /**< (GMAC_TSR) Used Bit Read Position */
#define GMAC_TSR_UBR_Msk                    (0x1U << GMAC_TSR_UBR_Pos)                     /**< (GMAC_TSR) Used Bit Read Mask */
#define GMAC_TSR_UBR                        GMAC_TSR_UBR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TSR_UBR_Msk instead */
#define GMAC_TSR_COL_Pos                    1                                              /**< (GMAC_TSR) Collision Occurred Position */
#define GMAC_TSR_COL_Msk                    (0x1U << GMAC_TSR_COL_Pos)                     /**< (GMAC_TSR) Collision Occurred Mask */
#define GMAC_TSR_COL                        GMAC_TSR_COL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TSR_COL_Msk instead */
#define GMAC_TSR_RLE_Pos                    2                                              /**< (GMAC_TSR) Retry Limit Exceeded Position */
#define GMAC_TSR_RLE_Msk                    (0x1U << GMAC_TSR_RLE_Pos)                     /**< (GMAC_TSR) Retry Limit Exceeded Mask */
#define GMAC_TSR_RLE                        GMAC_TSR_RLE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TSR_RLE_Msk instead */
#define GMAC_TSR_TXGO_Pos                   3                                              /**< (GMAC_TSR) Transmit Go Position */
#define GMAC_TSR_TXGO_Msk                   (0x1U << GMAC_TSR_TXGO_Pos)                    /**< (GMAC_TSR) Transmit Go Mask */
#define GMAC_TSR_TXGO                       GMAC_TSR_TXGO_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TSR_TXGO_Msk instead */
#define GMAC_TSR_TFC_Pos                    4                                              /**< (GMAC_TSR) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_TSR_TFC_Msk                    (0x1U << GMAC_TSR_TFC_Pos)                     /**< (GMAC_TSR) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_TSR_TFC                        GMAC_TSR_TFC_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TSR_TFC_Msk instead */
#define GMAC_TSR_TXCOMP_Pos                 5                                              /**< (GMAC_TSR) Transmit Complete Position */
#define GMAC_TSR_TXCOMP_Msk                 (0x1U << GMAC_TSR_TXCOMP_Pos)                  /**< (GMAC_TSR) Transmit Complete Mask */
#define GMAC_TSR_TXCOMP                     GMAC_TSR_TXCOMP_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TSR_TXCOMP_Msk instead */
#define GMAC_TSR_HRESP_Pos                  8                                              /**< (GMAC_TSR) HRESP Not OK Position */
#define GMAC_TSR_HRESP_Msk                  (0x1U << GMAC_TSR_HRESP_Pos)                   /**< (GMAC_TSR) HRESP Not OK Mask */
#define GMAC_TSR_HRESP                      GMAC_TSR_HRESP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TSR_HRESP_Msk instead */
#define GMAC_TSR_MASK                       (0x13FU)                                       /**< \deprecated (GMAC_TSR) Register MASK  (Use GMAC_TSR_Msk instead)  */
#define GMAC_TSR_Msk                        (0x13FU)                                       /**< (GMAC_TSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RBQB : (GMAC Offset: 0x18) (R/W 32) Receive Buffer Queue Base Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t ADDR:30;                   /**< bit:  2..31  Receive Buffer Queue Base Address        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RBQB_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RBQB_OFFSET                    (0x18)                                        /**<  (GMAC_RBQB) Receive Buffer Queue Base Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RBQB_ADDR_Pos                  2                                              /**< (GMAC_RBQB) Receive Buffer Queue Base Address Position */
#define GMAC_RBQB_ADDR_Msk                  (0x3FFFFFFFU << GMAC_RBQB_ADDR_Pos)            /**< (GMAC_RBQB) Receive Buffer Queue Base Address Mask */
#define GMAC_RBQB_ADDR(value)               (GMAC_RBQB_ADDR_Msk & ((value) << GMAC_RBQB_ADDR_Pos))
#define GMAC_RBQB_MASK                      (0xFFFFFFFCU)                                  /**< \deprecated (GMAC_RBQB) Register MASK  (Use GMAC_RBQB_Msk instead)  */
#define GMAC_RBQB_Msk                       (0xFFFFFFFCU)                                  /**< (GMAC_RBQB) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBQB : (GMAC Offset: 0x1c) (R/W 32) Transmit Buffer Queue Base Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t ADDR:30;                   /**< bit:  2..31  Transmit Buffer Queue Base Address       */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBQB_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBQB_OFFSET                    (0x1C)                                        /**<  (GMAC_TBQB) Transmit Buffer Queue Base Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBQB_ADDR_Pos                  2                                              /**< (GMAC_TBQB) Transmit Buffer Queue Base Address Position */
#define GMAC_TBQB_ADDR_Msk                  (0x3FFFFFFFU << GMAC_TBQB_ADDR_Pos)            /**< (GMAC_TBQB) Transmit Buffer Queue Base Address Mask */
#define GMAC_TBQB_ADDR(value)               (GMAC_TBQB_ADDR_Msk & ((value) << GMAC_TBQB_ADDR_Pos))
#define GMAC_TBQB_MASK                      (0xFFFFFFFCU)                                  /**< \deprecated (GMAC_TBQB) Register MASK  (Use GMAC_TBQB_Msk instead)  */
#define GMAC_TBQB_Msk                       (0xFFFFFFFCU)                                  /**< (GMAC_TBQB) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RSR : (GMAC Offset: 0x20) (R/W 32) Receive Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BNA:1;                     /**< bit:      0  Buffer Not Available                     */
    uint32_t REC:1;                     /**< bit:      1  Frame Received                           */
    uint32_t RXOVR:1;                   /**< bit:      2  Receive Overrun                          */
    uint32_t HNO:1;                     /**< bit:      3  HRESP Not OK                             */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RSR_OFFSET                     (0x20)                                        /**<  (GMAC_RSR) Receive Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RSR_BNA_Pos                    0                                              /**< (GMAC_RSR) Buffer Not Available Position */
#define GMAC_RSR_BNA_Msk                    (0x1U << GMAC_RSR_BNA_Pos)                     /**< (GMAC_RSR) Buffer Not Available Mask */
#define GMAC_RSR_BNA                        GMAC_RSR_BNA_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_RSR_BNA_Msk instead */
#define GMAC_RSR_REC_Pos                    1                                              /**< (GMAC_RSR) Frame Received Position */
#define GMAC_RSR_REC_Msk                    (0x1U << GMAC_RSR_REC_Pos)                     /**< (GMAC_RSR) Frame Received Mask */
#define GMAC_RSR_REC                        GMAC_RSR_REC_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_RSR_REC_Msk instead */
#define GMAC_RSR_RXOVR_Pos                  2                                              /**< (GMAC_RSR) Receive Overrun Position */
#define GMAC_RSR_RXOVR_Msk                  (0x1U << GMAC_RSR_RXOVR_Pos)                   /**< (GMAC_RSR) Receive Overrun Mask */
#define GMAC_RSR_RXOVR                      GMAC_RSR_RXOVR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_RSR_RXOVR_Msk instead */
#define GMAC_RSR_HNO_Pos                    3                                              /**< (GMAC_RSR) HRESP Not OK Position */
#define GMAC_RSR_HNO_Msk                    (0x1U << GMAC_RSR_HNO_Pos)                     /**< (GMAC_RSR) HRESP Not OK Mask */
#define GMAC_RSR_HNO                        GMAC_RSR_HNO_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_RSR_HNO_Msk instead */
#define GMAC_RSR_MASK                       (0x0FU)                                        /**< \deprecated (GMAC_RSR) Register MASK  (Use GMAC_RSR_Msk instead)  */
#define GMAC_RSR_Msk                        (0x0FU)                                        /**< (GMAC_RSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ISR : (GMAC Offset: 0x24) (R/ 32) Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MFS:1;                     /**< bit:      0  Management Frame Sent                    */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t TXUBR:1;                   /**< bit:      3  TX Used Bit Read                         */
    uint32_t TUR:1;                     /**< bit:      4  Transmit Underrun                        */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded                     */
    uint32_t TFC:1;                     /**< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t PFNZ:1;                    /**< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;                     /**< bit:     13  Pause Time Zero                          */
    uint32_t PFTR:1;                    /**< bit:     14  Pause Frame Transmitted                  */
    uint32_t :3;                        /**< bit: 15..17  Reserved */
    uint32_t DRQFR:1;                   /**< bit:     18  PTP Delay Request Frame Received         */
    uint32_t SFR:1;                     /**< bit:     19  PTP Sync Frame Received                  */
    uint32_t DRQFT:1;                   /**< bit:     20  PTP Delay Request Frame Transmitted      */
    uint32_t SFT:1;                     /**< bit:     21  PTP Sync Frame Transmitted               */
    uint32_t PDRQFR:1;                  /**< bit:     22  PDelay Request Frame Received            */
    uint32_t PDRSFR:1;                  /**< bit:     23  PDelay Response Frame Received           */
    uint32_t PDRQFT:1;                  /**< bit:     24  PDelay Request Frame Transmitted         */
    uint32_t PDRSFT:1;                  /**< bit:     25  PDelay Response Frame Transmitted        */
    uint32_t SRI:1;                     /**< bit:     26  TSU Seconds Register Increment           */
    uint32_t :1;                        /**< bit:     27  Reserved */
    uint32_t WOL:1;                     /**< bit:     28  Wake On LAN                              */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ISR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ISR_OFFSET                     (0x24)                                        /**<  (GMAC_ISR) Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ISR_MFS_Pos                    0                                              /**< (GMAC_ISR) Management Frame Sent Position */
#define GMAC_ISR_MFS_Msk                    (0x1U << GMAC_ISR_MFS_Pos)                     /**< (GMAC_ISR) Management Frame Sent Mask */
#define GMAC_ISR_MFS                        GMAC_ISR_MFS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_MFS_Msk instead */
#define GMAC_ISR_RCOMP_Pos                  1                                              /**< (GMAC_ISR) Receive Complete Position */
#define GMAC_ISR_RCOMP_Msk                  (0x1U << GMAC_ISR_RCOMP_Pos)                   /**< (GMAC_ISR) Receive Complete Mask */
#define GMAC_ISR_RCOMP                      GMAC_ISR_RCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_RCOMP_Msk instead */
#define GMAC_ISR_RXUBR_Pos                  2                                              /**< (GMAC_ISR) RX Used Bit Read Position */
#define GMAC_ISR_RXUBR_Msk                  (0x1U << GMAC_ISR_RXUBR_Pos)                   /**< (GMAC_ISR) RX Used Bit Read Mask */
#define GMAC_ISR_RXUBR                      GMAC_ISR_RXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_RXUBR_Msk instead */
#define GMAC_ISR_TXUBR_Pos                  3                                              /**< (GMAC_ISR) TX Used Bit Read Position */
#define GMAC_ISR_TXUBR_Msk                  (0x1U << GMAC_ISR_TXUBR_Pos)                   /**< (GMAC_ISR) TX Used Bit Read Mask */
#define GMAC_ISR_TXUBR                      GMAC_ISR_TXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_TXUBR_Msk instead */
#define GMAC_ISR_TUR_Pos                    4                                              /**< (GMAC_ISR) Transmit Underrun Position */
#define GMAC_ISR_TUR_Msk                    (0x1U << GMAC_ISR_TUR_Pos)                     /**< (GMAC_ISR) Transmit Underrun Mask */
#define GMAC_ISR_TUR                        GMAC_ISR_TUR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_TUR_Msk instead */
#define GMAC_ISR_RLEX_Pos                   5                                              /**< (GMAC_ISR) Retry Limit Exceeded Position */
#define GMAC_ISR_RLEX_Msk                   (0x1U << GMAC_ISR_RLEX_Pos)                    /**< (GMAC_ISR) Retry Limit Exceeded Mask */
#define GMAC_ISR_RLEX                       GMAC_ISR_RLEX_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_RLEX_Msk instead */
#define GMAC_ISR_TFC_Pos                    6                                              /**< (GMAC_ISR) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_ISR_TFC_Msk                    (0x1U << GMAC_ISR_TFC_Pos)                     /**< (GMAC_ISR) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_ISR_TFC                        GMAC_ISR_TFC_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_TFC_Msk instead */
#define GMAC_ISR_TCOMP_Pos                  7                                              /**< (GMAC_ISR) Transmit Complete Position */
#define GMAC_ISR_TCOMP_Msk                  (0x1U << GMAC_ISR_TCOMP_Pos)                   /**< (GMAC_ISR) Transmit Complete Mask */
#define GMAC_ISR_TCOMP                      GMAC_ISR_TCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_TCOMP_Msk instead */
#define GMAC_ISR_ROVR_Pos                   10                                             /**< (GMAC_ISR) Receive Overrun Position */
#define GMAC_ISR_ROVR_Msk                   (0x1U << GMAC_ISR_ROVR_Pos)                    /**< (GMAC_ISR) Receive Overrun Mask */
#define GMAC_ISR_ROVR                       GMAC_ISR_ROVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_ROVR_Msk instead */
#define GMAC_ISR_HRESP_Pos                  11                                             /**< (GMAC_ISR) HRESP Not OK Position */
#define GMAC_ISR_HRESP_Msk                  (0x1U << GMAC_ISR_HRESP_Pos)                   /**< (GMAC_ISR) HRESP Not OK Mask */
#define GMAC_ISR_HRESP                      GMAC_ISR_HRESP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_HRESP_Msk instead */
#define GMAC_ISR_PFNZ_Pos                   12                                             /**< (GMAC_ISR) Pause Frame with Non-zero Pause Quantum Received Position */
#define GMAC_ISR_PFNZ_Msk                   (0x1U << GMAC_ISR_PFNZ_Pos)                    /**< (GMAC_ISR) Pause Frame with Non-zero Pause Quantum Received Mask */
#define GMAC_ISR_PFNZ                       GMAC_ISR_PFNZ_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_PFNZ_Msk instead */
#define GMAC_ISR_PTZ_Pos                    13                                             /**< (GMAC_ISR) Pause Time Zero Position */
#define GMAC_ISR_PTZ_Msk                    (0x1U << GMAC_ISR_PTZ_Pos)                     /**< (GMAC_ISR) Pause Time Zero Mask */
#define GMAC_ISR_PTZ                        GMAC_ISR_PTZ_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_PTZ_Msk instead */
#define GMAC_ISR_PFTR_Pos                   14                                             /**< (GMAC_ISR) Pause Frame Transmitted Position */
#define GMAC_ISR_PFTR_Msk                   (0x1U << GMAC_ISR_PFTR_Pos)                    /**< (GMAC_ISR) Pause Frame Transmitted Mask */
#define GMAC_ISR_PFTR                       GMAC_ISR_PFTR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_PFTR_Msk instead */
#define GMAC_ISR_DRQFR_Pos                  18                                             /**< (GMAC_ISR) PTP Delay Request Frame Received Position */
#define GMAC_ISR_DRQFR_Msk                  (0x1U << GMAC_ISR_DRQFR_Pos)                   /**< (GMAC_ISR) PTP Delay Request Frame Received Mask */
#define GMAC_ISR_DRQFR                      GMAC_ISR_DRQFR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_DRQFR_Msk instead */
#define GMAC_ISR_SFR_Pos                    19                                             /**< (GMAC_ISR) PTP Sync Frame Received Position */
#define GMAC_ISR_SFR_Msk                    (0x1U << GMAC_ISR_SFR_Pos)                     /**< (GMAC_ISR) PTP Sync Frame Received Mask */
#define GMAC_ISR_SFR                        GMAC_ISR_SFR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_SFR_Msk instead */
#define GMAC_ISR_DRQFT_Pos                  20                                             /**< (GMAC_ISR) PTP Delay Request Frame Transmitted Position */
#define GMAC_ISR_DRQFT_Msk                  (0x1U << GMAC_ISR_DRQFT_Pos)                   /**< (GMAC_ISR) PTP Delay Request Frame Transmitted Mask */
#define GMAC_ISR_DRQFT                      GMAC_ISR_DRQFT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_DRQFT_Msk instead */
#define GMAC_ISR_SFT_Pos                    21                                             /**< (GMAC_ISR) PTP Sync Frame Transmitted Position */
#define GMAC_ISR_SFT_Msk                    (0x1U << GMAC_ISR_SFT_Pos)                     /**< (GMAC_ISR) PTP Sync Frame Transmitted Mask */
#define GMAC_ISR_SFT                        GMAC_ISR_SFT_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_SFT_Msk instead */
#define GMAC_ISR_PDRQFR_Pos                 22                                             /**< (GMAC_ISR) PDelay Request Frame Received Position */
#define GMAC_ISR_PDRQFR_Msk                 (0x1U << GMAC_ISR_PDRQFR_Pos)                  /**< (GMAC_ISR) PDelay Request Frame Received Mask */
#define GMAC_ISR_PDRQFR                     GMAC_ISR_PDRQFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_PDRQFR_Msk instead */
#define GMAC_ISR_PDRSFR_Pos                 23                                             /**< (GMAC_ISR) PDelay Response Frame Received Position */
#define GMAC_ISR_PDRSFR_Msk                 (0x1U << GMAC_ISR_PDRSFR_Pos)                  /**< (GMAC_ISR) PDelay Response Frame Received Mask */
#define GMAC_ISR_PDRSFR                     GMAC_ISR_PDRSFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_PDRSFR_Msk instead */
#define GMAC_ISR_PDRQFT_Pos                 24                                             /**< (GMAC_ISR) PDelay Request Frame Transmitted Position */
#define GMAC_ISR_PDRQFT_Msk                 (0x1U << GMAC_ISR_PDRQFT_Pos)                  /**< (GMAC_ISR) PDelay Request Frame Transmitted Mask */
#define GMAC_ISR_PDRQFT                     GMAC_ISR_PDRQFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_PDRQFT_Msk instead */
#define GMAC_ISR_PDRSFT_Pos                 25                                             /**< (GMAC_ISR) PDelay Response Frame Transmitted Position */
#define GMAC_ISR_PDRSFT_Msk                 (0x1U << GMAC_ISR_PDRSFT_Pos)                  /**< (GMAC_ISR) PDelay Response Frame Transmitted Mask */
#define GMAC_ISR_PDRSFT                     GMAC_ISR_PDRSFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_PDRSFT_Msk instead */
#define GMAC_ISR_SRI_Pos                    26                                             /**< (GMAC_ISR) TSU Seconds Register Increment Position */
#define GMAC_ISR_SRI_Msk                    (0x1U << GMAC_ISR_SRI_Pos)                     /**< (GMAC_ISR) TSU Seconds Register Increment Mask */
#define GMAC_ISR_SRI                        GMAC_ISR_SRI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_SRI_Msk instead */
#define GMAC_ISR_WOL_Pos                    28                                             /**< (GMAC_ISR) Wake On LAN Position */
#define GMAC_ISR_WOL_Msk                    (0x1U << GMAC_ISR_WOL_Pos)                     /**< (GMAC_ISR) Wake On LAN Mask */
#define GMAC_ISR_WOL                        GMAC_ISR_WOL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISR_WOL_Msk instead */
#define GMAC_ISR_MASK                       (0x17FC7CFFU)                                  /**< \deprecated (GMAC_ISR) Register MASK  (Use GMAC_ISR_Msk instead)  */
#define GMAC_ISR_Msk                        (0x17FC7CFFU)                                  /**< (GMAC_ISR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IER : (GMAC Offset: 0x28) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MFS:1;                     /**< bit:      0  Management Frame Sent                    */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t TXUBR:1;                   /**< bit:      3  TX Used Bit Read                         */
    uint32_t TUR:1;                     /**< bit:      4  Transmit Underrun                        */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded or Late Collision   */
    uint32_t TFC:1;                     /**< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t PFNZ:1;                    /**< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;                     /**< bit:     13  Pause Time Zero                          */
    uint32_t PFTR:1;                    /**< bit:     14  Pause Frame Transmitted                  */
    uint32_t EXINT:1;                   /**< bit:     15  External Interrupt                       */
    uint32_t :2;                        /**< bit: 16..17  Reserved */
    uint32_t DRQFR:1;                   /**< bit:     18  PTP Delay Request Frame Received         */
    uint32_t SFR:1;                     /**< bit:     19  PTP Sync Frame Received                  */
    uint32_t DRQFT:1;                   /**< bit:     20  PTP Delay Request Frame Transmitted      */
    uint32_t SFT:1;                     /**< bit:     21  PTP Sync Frame Transmitted               */
    uint32_t PDRQFR:1;                  /**< bit:     22  PDelay Request Frame Received            */
    uint32_t PDRSFR:1;                  /**< bit:     23  PDelay Response Frame Received           */
    uint32_t PDRQFT:1;                  /**< bit:     24  PDelay Request Frame Transmitted         */
    uint32_t PDRSFT:1;                  /**< bit:     25  PDelay Response Frame Transmitted        */
    uint32_t SRI:1;                     /**< bit:     26  TSU Seconds Register Increment           */
    uint32_t :1;                        /**< bit:     27  Reserved */
    uint32_t WOL:1;                     /**< bit:     28  Wake On LAN                              */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IER_OFFSET                     (0x28)                                        /**<  (GMAC_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IER_MFS_Pos                    0                                              /**< (GMAC_IER) Management Frame Sent Position */
#define GMAC_IER_MFS_Msk                    (0x1U << GMAC_IER_MFS_Pos)                     /**< (GMAC_IER) Management Frame Sent Mask */
#define GMAC_IER_MFS                        GMAC_IER_MFS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_MFS_Msk instead */
#define GMAC_IER_RCOMP_Pos                  1                                              /**< (GMAC_IER) Receive Complete Position */
#define GMAC_IER_RCOMP_Msk                  (0x1U << GMAC_IER_RCOMP_Pos)                   /**< (GMAC_IER) Receive Complete Mask */
#define GMAC_IER_RCOMP                      GMAC_IER_RCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_RCOMP_Msk instead */
#define GMAC_IER_RXUBR_Pos                  2                                              /**< (GMAC_IER) RX Used Bit Read Position */
#define GMAC_IER_RXUBR_Msk                  (0x1U << GMAC_IER_RXUBR_Pos)                   /**< (GMAC_IER) RX Used Bit Read Mask */
#define GMAC_IER_RXUBR                      GMAC_IER_RXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_RXUBR_Msk instead */
#define GMAC_IER_TXUBR_Pos                  3                                              /**< (GMAC_IER) TX Used Bit Read Position */
#define GMAC_IER_TXUBR_Msk                  (0x1U << GMAC_IER_TXUBR_Pos)                   /**< (GMAC_IER) TX Used Bit Read Mask */
#define GMAC_IER_TXUBR                      GMAC_IER_TXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_TXUBR_Msk instead */
#define GMAC_IER_TUR_Pos                    4                                              /**< (GMAC_IER) Transmit Underrun Position */
#define GMAC_IER_TUR_Msk                    (0x1U << GMAC_IER_TUR_Pos)                     /**< (GMAC_IER) Transmit Underrun Mask */
#define GMAC_IER_TUR                        GMAC_IER_TUR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_TUR_Msk instead */
#define GMAC_IER_RLEX_Pos                   5                                              /**< (GMAC_IER) Retry Limit Exceeded or Late Collision Position */
#define GMAC_IER_RLEX_Msk                   (0x1U << GMAC_IER_RLEX_Pos)                    /**< (GMAC_IER) Retry Limit Exceeded or Late Collision Mask */
#define GMAC_IER_RLEX                       GMAC_IER_RLEX_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_RLEX_Msk instead */
#define GMAC_IER_TFC_Pos                    6                                              /**< (GMAC_IER) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_IER_TFC_Msk                    (0x1U << GMAC_IER_TFC_Pos)                     /**< (GMAC_IER) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_IER_TFC                        GMAC_IER_TFC_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_TFC_Msk instead */
#define GMAC_IER_TCOMP_Pos                  7                                              /**< (GMAC_IER) Transmit Complete Position */
#define GMAC_IER_TCOMP_Msk                  (0x1U << GMAC_IER_TCOMP_Pos)                   /**< (GMAC_IER) Transmit Complete Mask */
#define GMAC_IER_TCOMP                      GMAC_IER_TCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_TCOMP_Msk instead */
#define GMAC_IER_ROVR_Pos                   10                                             /**< (GMAC_IER) Receive Overrun Position */
#define GMAC_IER_ROVR_Msk                   (0x1U << GMAC_IER_ROVR_Pos)                    /**< (GMAC_IER) Receive Overrun Mask */
#define GMAC_IER_ROVR                       GMAC_IER_ROVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_ROVR_Msk instead */
#define GMAC_IER_HRESP_Pos                  11                                             /**< (GMAC_IER) HRESP Not OK Position */
#define GMAC_IER_HRESP_Msk                  (0x1U << GMAC_IER_HRESP_Pos)                   /**< (GMAC_IER) HRESP Not OK Mask */
#define GMAC_IER_HRESP                      GMAC_IER_HRESP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_HRESP_Msk instead */
#define GMAC_IER_PFNZ_Pos                   12                                             /**< (GMAC_IER) Pause Frame with Non-zero Pause Quantum Received Position */
#define GMAC_IER_PFNZ_Msk                   (0x1U << GMAC_IER_PFNZ_Pos)                    /**< (GMAC_IER) Pause Frame with Non-zero Pause Quantum Received Mask */
#define GMAC_IER_PFNZ                       GMAC_IER_PFNZ_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_PFNZ_Msk instead */
#define GMAC_IER_PTZ_Pos                    13                                             /**< (GMAC_IER) Pause Time Zero Position */
#define GMAC_IER_PTZ_Msk                    (0x1U << GMAC_IER_PTZ_Pos)                     /**< (GMAC_IER) Pause Time Zero Mask */
#define GMAC_IER_PTZ                        GMAC_IER_PTZ_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_PTZ_Msk instead */
#define GMAC_IER_PFTR_Pos                   14                                             /**< (GMAC_IER) Pause Frame Transmitted Position */
#define GMAC_IER_PFTR_Msk                   (0x1U << GMAC_IER_PFTR_Pos)                    /**< (GMAC_IER) Pause Frame Transmitted Mask */
#define GMAC_IER_PFTR                       GMAC_IER_PFTR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_PFTR_Msk instead */
#define GMAC_IER_EXINT_Pos                  15                                             /**< (GMAC_IER) External Interrupt Position */
#define GMAC_IER_EXINT_Msk                  (0x1U << GMAC_IER_EXINT_Pos)                   /**< (GMAC_IER) External Interrupt Mask */
#define GMAC_IER_EXINT                      GMAC_IER_EXINT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_EXINT_Msk instead */
#define GMAC_IER_DRQFR_Pos                  18                                             /**< (GMAC_IER) PTP Delay Request Frame Received Position */
#define GMAC_IER_DRQFR_Msk                  (0x1U << GMAC_IER_DRQFR_Pos)                   /**< (GMAC_IER) PTP Delay Request Frame Received Mask */
#define GMAC_IER_DRQFR                      GMAC_IER_DRQFR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_DRQFR_Msk instead */
#define GMAC_IER_SFR_Pos                    19                                             /**< (GMAC_IER) PTP Sync Frame Received Position */
#define GMAC_IER_SFR_Msk                    (0x1U << GMAC_IER_SFR_Pos)                     /**< (GMAC_IER) PTP Sync Frame Received Mask */
#define GMAC_IER_SFR                        GMAC_IER_SFR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_SFR_Msk instead */
#define GMAC_IER_DRQFT_Pos                  20                                             /**< (GMAC_IER) PTP Delay Request Frame Transmitted Position */
#define GMAC_IER_DRQFT_Msk                  (0x1U << GMAC_IER_DRQFT_Pos)                   /**< (GMAC_IER) PTP Delay Request Frame Transmitted Mask */
#define GMAC_IER_DRQFT                      GMAC_IER_DRQFT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_DRQFT_Msk instead */
#define GMAC_IER_SFT_Pos                    21                                             /**< (GMAC_IER) PTP Sync Frame Transmitted Position */
#define GMAC_IER_SFT_Msk                    (0x1U << GMAC_IER_SFT_Pos)                     /**< (GMAC_IER) PTP Sync Frame Transmitted Mask */
#define GMAC_IER_SFT                        GMAC_IER_SFT_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_SFT_Msk instead */
#define GMAC_IER_PDRQFR_Pos                 22                                             /**< (GMAC_IER) PDelay Request Frame Received Position */
#define GMAC_IER_PDRQFR_Msk                 (0x1U << GMAC_IER_PDRQFR_Pos)                  /**< (GMAC_IER) PDelay Request Frame Received Mask */
#define GMAC_IER_PDRQFR                     GMAC_IER_PDRQFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_PDRQFR_Msk instead */
#define GMAC_IER_PDRSFR_Pos                 23                                             /**< (GMAC_IER) PDelay Response Frame Received Position */
#define GMAC_IER_PDRSFR_Msk                 (0x1U << GMAC_IER_PDRSFR_Pos)                  /**< (GMAC_IER) PDelay Response Frame Received Mask */
#define GMAC_IER_PDRSFR                     GMAC_IER_PDRSFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_PDRSFR_Msk instead */
#define GMAC_IER_PDRQFT_Pos                 24                                             /**< (GMAC_IER) PDelay Request Frame Transmitted Position */
#define GMAC_IER_PDRQFT_Msk                 (0x1U << GMAC_IER_PDRQFT_Pos)                  /**< (GMAC_IER) PDelay Request Frame Transmitted Mask */
#define GMAC_IER_PDRQFT                     GMAC_IER_PDRQFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_PDRQFT_Msk instead */
#define GMAC_IER_PDRSFT_Pos                 25                                             /**< (GMAC_IER) PDelay Response Frame Transmitted Position */
#define GMAC_IER_PDRSFT_Msk                 (0x1U << GMAC_IER_PDRSFT_Pos)                  /**< (GMAC_IER) PDelay Response Frame Transmitted Mask */
#define GMAC_IER_PDRSFT                     GMAC_IER_PDRSFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_PDRSFT_Msk instead */
#define GMAC_IER_SRI_Pos                    26                                             /**< (GMAC_IER) TSU Seconds Register Increment Position */
#define GMAC_IER_SRI_Msk                    (0x1U << GMAC_IER_SRI_Pos)                     /**< (GMAC_IER) TSU Seconds Register Increment Mask */
#define GMAC_IER_SRI                        GMAC_IER_SRI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_SRI_Msk instead */
#define GMAC_IER_WOL_Pos                    28                                             /**< (GMAC_IER) Wake On LAN Position */
#define GMAC_IER_WOL_Msk                    (0x1U << GMAC_IER_WOL_Pos)                     /**< (GMAC_IER) Wake On LAN Mask */
#define GMAC_IER_WOL                        GMAC_IER_WOL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IER_WOL_Msk instead */
#define GMAC_IER_MASK                       (0x17FCFCFFU)                                  /**< \deprecated (GMAC_IER) Register MASK  (Use GMAC_IER_Msk instead)  */
#define GMAC_IER_Msk                        (0x17FCFCFFU)                                  /**< (GMAC_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IDR : (GMAC Offset: 0x2c) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MFS:1;                     /**< bit:      0  Management Frame Sent                    */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t TXUBR:1;                   /**< bit:      3  TX Used Bit Read                         */
    uint32_t TUR:1;                     /**< bit:      4  Transmit Underrun                        */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded or Late Collision   */
    uint32_t TFC:1;                     /**< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t PFNZ:1;                    /**< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;                     /**< bit:     13  Pause Time Zero                          */
    uint32_t PFTR:1;                    /**< bit:     14  Pause Frame Transmitted                  */
    uint32_t EXINT:1;                   /**< bit:     15  External Interrupt                       */
    uint32_t :2;                        /**< bit: 16..17  Reserved */
    uint32_t DRQFR:1;                   /**< bit:     18  PTP Delay Request Frame Received         */
    uint32_t SFR:1;                     /**< bit:     19  PTP Sync Frame Received                  */
    uint32_t DRQFT:1;                   /**< bit:     20  PTP Delay Request Frame Transmitted      */
    uint32_t SFT:1;                     /**< bit:     21  PTP Sync Frame Transmitted               */
    uint32_t PDRQFR:1;                  /**< bit:     22  PDelay Request Frame Received            */
    uint32_t PDRSFR:1;                  /**< bit:     23  PDelay Response Frame Received           */
    uint32_t PDRQFT:1;                  /**< bit:     24  PDelay Request Frame Transmitted         */
    uint32_t PDRSFT:1;                  /**< bit:     25  PDelay Response Frame Transmitted        */
    uint32_t SRI:1;                     /**< bit:     26  TSU Seconds Register Increment           */
    uint32_t :1;                        /**< bit:     27  Reserved */
    uint32_t WOL:1;                     /**< bit:     28  Wake On LAN                              */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IDR_OFFSET                     (0x2C)                                        /**<  (GMAC_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IDR_MFS_Pos                    0                                              /**< (GMAC_IDR) Management Frame Sent Position */
#define GMAC_IDR_MFS_Msk                    (0x1U << GMAC_IDR_MFS_Pos)                     /**< (GMAC_IDR) Management Frame Sent Mask */
#define GMAC_IDR_MFS                        GMAC_IDR_MFS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_MFS_Msk instead */
#define GMAC_IDR_RCOMP_Pos                  1                                              /**< (GMAC_IDR) Receive Complete Position */
#define GMAC_IDR_RCOMP_Msk                  (0x1U << GMAC_IDR_RCOMP_Pos)                   /**< (GMAC_IDR) Receive Complete Mask */
#define GMAC_IDR_RCOMP                      GMAC_IDR_RCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_RCOMP_Msk instead */
#define GMAC_IDR_RXUBR_Pos                  2                                              /**< (GMAC_IDR) RX Used Bit Read Position */
#define GMAC_IDR_RXUBR_Msk                  (0x1U << GMAC_IDR_RXUBR_Pos)                   /**< (GMAC_IDR) RX Used Bit Read Mask */
#define GMAC_IDR_RXUBR                      GMAC_IDR_RXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_RXUBR_Msk instead */
#define GMAC_IDR_TXUBR_Pos                  3                                              /**< (GMAC_IDR) TX Used Bit Read Position */
#define GMAC_IDR_TXUBR_Msk                  (0x1U << GMAC_IDR_TXUBR_Pos)                   /**< (GMAC_IDR) TX Used Bit Read Mask */
#define GMAC_IDR_TXUBR                      GMAC_IDR_TXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_TXUBR_Msk instead */
#define GMAC_IDR_TUR_Pos                    4                                              /**< (GMAC_IDR) Transmit Underrun Position */
#define GMAC_IDR_TUR_Msk                    (0x1U << GMAC_IDR_TUR_Pos)                     /**< (GMAC_IDR) Transmit Underrun Mask */
#define GMAC_IDR_TUR                        GMAC_IDR_TUR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_TUR_Msk instead */
#define GMAC_IDR_RLEX_Pos                   5                                              /**< (GMAC_IDR) Retry Limit Exceeded or Late Collision Position */
#define GMAC_IDR_RLEX_Msk                   (0x1U << GMAC_IDR_RLEX_Pos)                    /**< (GMAC_IDR) Retry Limit Exceeded or Late Collision Mask */
#define GMAC_IDR_RLEX                       GMAC_IDR_RLEX_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_RLEX_Msk instead */
#define GMAC_IDR_TFC_Pos                    6                                              /**< (GMAC_IDR) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_IDR_TFC_Msk                    (0x1U << GMAC_IDR_TFC_Pos)                     /**< (GMAC_IDR) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_IDR_TFC                        GMAC_IDR_TFC_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_TFC_Msk instead */
#define GMAC_IDR_TCOMP_Pos                  7                                              /**< (GMAC_IDR) Transmit Complete Position */
#define GMAC_IDR_TCOMP_Msk                  (0x1U << GMAC_IDR_TCOMP_Pos)                   /**< (GMAC_IDR) Transmit Complete Mask */
#define GMAC_IDR_TCOMP                      GMAC_IDR_TCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_TCOMP_Msk instead */
#define GMAC_IDR_ROVR_Pos                   10                                             /**< (GMAC_IDR) Receive Overrun Position */
#define GMAC_IDR_ROVR_Msk                   (0x1U << GMAC_IDR_ROVR_Pos)                    /**< (GMAC_IDR) Receive Overrun Mask */
#define GMAC_IDR_ROVR                       GMAC_IDR_ROVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_ROVR_Msk instead */
#define GMAC_IDR_HRESP_Pos                  11                                             /**< (GMAC_IDR) HRESP Not OK Position */
#define GMAC_IDR_HRESP_Msk                  (0x1U << GMAC_IDR_HRESP_Pos)                   /**< (GMAC_IDR) HRESP Not OK Mask */
#define GMAC_IDR_HRESP                      GMAC_IDR_HRESP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_HRESP_Msk instead */
#define GMAC_IDR_PFNZ_Pos                   12                                             /**< (GMAC_IDR) Pause Frame with Non-zero Pause Quantum Received Position */
#define GMAC_IDR_PFNZ_Msk                   (0x1U << GMAC_IDR_PFNZ_Pos)                    /**< (GMAC_IDR) Pause Frame with Non-zero Pause Quantum Received Mask */
#define GMAC_IDR_PFNZ                       GMAC_IDR_PFNZ_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_PFNZ_Msk instead */
#define GMAC_IDR_PTZ_Pos                    13                                             /**< (GMAC_IDR) Pause Time Zero Position */
#define GMAC_IDR_PTZ_Msk                    (0x1U << GMAC_IDR_PTZ_Pos)                     /**< (GMAC_IDR) Pause Time Zero Mask */
#define GMAC_IDR_PTZ                        GMAC_IDR_PTZ_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_PTZ_Msk instead */
#define GMAC_IDR_PFTR_Pos                   14                                             /**< (GMAC_IDR) Pause Frame Transmitted Position */
#define GMAC_IDR_PFTR_Msk                   (0x1U << GMAC_IDR_PFTR_Pos)                    /**< (GMAC_IDR) Pause Frame Transmitted Mask */
#define GMAC_IDR_PFTR                       GMAC_IDR_PFTR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_PFTR_Msk instead */
#define GMAC_IDR_EXINT_Pos                  15                                             /**< (GMAC_IDR) External Interrupt Position */
#define GMAC_IDR_EXINT_Msk                  (0x1U << GMAC_IDR_EXINT_Pos)                   /**< (GMAC_IDR) External Interrupt Mask */
#define GMAC_IDR_EXINT                      GMAC_IDR_EXINT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_EXINT_Msk instead */
#define GMAC_IDR_DRQFR_Pos                  18                                             /**< (GMAC_IDR) PTP Delay Request Frame Received Position */
#define GMAC_IDR_DRQFR_Msk                  (0x1U << GMAC_IDR_DRQFR_Pos)                   /**< (GMAC_IDR) PTP Delay Request Frame Received Mask */
#define GMAC_IDR_DRQFR                      GMAC_IDR_DRQFR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_DRQFR_Msk instead */
#define GMAC_IDR_SFR_Pos                    19                                             /**< (GMAC_IDR) PTP Sync Frame Received Position */
#define GMAC_IDR_SFR_Msk                    (0x1U << GMAC_IDR_SFR_Pos)                     /**< (GMAC_IDR) PTP Sync Frame Received Mask */
#define GMAC_IDR_SFR                        GMAC_IDR_SFR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_SFR_Msk instead */
#define GMAC_IDR_DRQFT_Pos                  20                                             /**< (GMAC_IDR) PTP Delay Request Frame Transmitted Position */
#define GMAC_IDR_DRQFT_Msk                  (0x1U << GMAC_IDR_DRQFT_Pos)                   /**< (GMAC_IDR) PTP Delay Request Frame Transmitted Mask */
#define GMAC_IDR_DRQFT                      GMAC_IDR_DRQFT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_DRQFT_Msk instead */
#define GMAC_IDR_SFT_Pos                    21                                             /**< (GMAC_IDR) PTP Sync Frame Transmitted Position */
#define GMAC_IDR_SFT_Msk                    (0x1U << GMAC_IDR_SFT_Pos)                     /**< (GMAC_IDR) PTP Sync Frame Transmitted Mask */
#define GMAC_IDR_SFT                        GMAC_IDR_SFT_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_SFT_Msk instead */
#define GMAC_IDR_PDRQFR_Pos                 22                                             /**< (GMAC_IDR) PDelay Request Frame Received Position */
#define GMAC_IDR_PDRQFR_Msk                 (0x1U << GMAC_IDR_PDRQFR_Pos)                  /**< (GMAC_IDR) PDelay Request Frame Received Mask */
#define GMAC_IDR_PDRQFR                     GMAC_IDR_PDRQFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_PDRQFR_Msk instead */
#define GMAC_IDR_PDRSFR_Pos                 23                                             /**< (GMAC_IDR) PDelay Response Frame Received Position */
#define GMAC_IDR_PDRSFR_Msk                 (0x1U << GMAC_IDR_PDRSFR_Pos)                  /**< (GMAC_IDR) PDelay Response Frame Received Mask */
#define GMAC_IDR_PDRSFR                     GMAC_IDR_PDRSFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_PDRSFR_Msk instead */
#define GMAC_IDR_PDRQFT_Pos                 24                                             /**< (GMAC_IDR) PDelay Request Frame Transmitted Position */
#define GMAC_IDR_PDRQFT_Msk                 (0x1U << GMAC_IDR_PDRQFT_Pos)                  /**< (GMAC_IDR) PDelay Request Frame Transmitted Mask */
#define GMAC_IDR_PDRQFT                     GMAC_IDR_PDRQFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_PDRQFT_Msk instead */
#define GMAC_IDR_PDRSFT_Pos                 25                                             /**< (GMAC_IDR) PDelay Response Frame Transmitted Position */
#define GMAC_IDR_PDRSFT_Msk                 (0x1U << GMAC_IDR_PDRSFT_Pos)                  /**< (GMAC_IDR) PDelay Response Frame Transmitted Mask */
#define GMAC_IDR_PDRSFT                     GMAC_IDR_PDRSFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_PDRSFT_Msk instead */
#define GMAC_IDR_SRI_Pos                    26                                             /**< (GMAC_IDR) TSU Seconds Register Increment Position */
#define GMAC_IDR_SRI_Msk                    (0x1U << GMAC_IDR_SRI_Pos)                     /**< (GMAC_IDR) TSU Seconds Register Increment Mask */
#define GMAC_IDR_SRI                        GMAC_IDR_SRI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_SRI_Msk instead */
#define GMAC_IDR_WOL_Pos                    28                                             /**< (GMAC_IDR) Wake On LAN Position */
#define GMAC_IDR_WOL_Msk                    (0x1U << GMAC_IDR_WOL_Pos)                     /**< (GMAC_IDR) Wake On LAN Mask */
#define GMAC_IDR_WOL                        GMAC_IDR_WOL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDR_WOL_Msk instead */
#define GMAC_IDR_MASK                       (0x17FCFCFFU)                                  /**< \deprecated (GMAC_IDR) Register MASK  (Use GMAC_IDR_Msk instead)  */
#define GMAC_IDR_Msk                        (0x17FCFCFFU)                                  /**< (GMAC_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IMR : (GMAC Offset: 0x30) (R/W 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MFS:1;                     /**< bit:      0  Management Frame Sent                    */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t TXUBR:1;                   /**< bit:      3  TX Used Bit Read                         */
    uint32_t TUR:1;                     /**< bit:      4  Transmit Underrun                        */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded                     */
    uint32_t TFC:1;                     /**< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t PFNZ:1;                    /**< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;                     /**< bit:     13  Pause Time Zero                          */
    uint32_t PFTR:1;                    /**< bit:     14  Pause Frame Transmitted                  */
    uint32_t EXINT:1;                   /**< bit:     15  External Interrupt                       */
    uint32_t :2;                        /**< bit: 16..17  Reserved */
    uint32_t DRQFR:1;                   /**< bit:     18  PTP Delay Request Frame Received         */
    uint32_t SFR:1;                     /**< bit:     19  PTP Sync Frame Received                  */
    uint32_t DRQFT:1;                   /**< bit:     20  PTP Delay Request Frame Transmitted      */
    uint32_t SFT:1;                     /**< bit:     21  PTP Sync Frame Transmitted               */
    uint32_t PDRQFR:1;                  /**< bit:     22  PDelay Request Frame Received            */
    uint32_t PDRSFR:1;                  /**< bit:     23  PDelay Response Frame Received           */
    uint32_t PDRQFT:1;                  /**< bit:     24  PDelay Request Frame Transmitted         */
    uint32_t PDRSFT:1;                  /**< bit:     25  PDelay Response Frame Transmitted        */
    uint32_t :6;                        /**< bit: 26..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IMR_OFFSET                     (0x30)                                        /**<  (GMAC_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IMR_MFS_Pos                    0                                              /**< (GMAC_IMR) Management Frame Sent Position */
#define GMAC_IMR_MFS_Msk                    (0x1U << GMAC_IMR_MFS_Pos)                     /**< (GMAC_IMR) Management Frame Sent Mask */
#define GMAC_IMR_MFS                        GMAC_IMR_MFS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_MFS_Msk instead */
#define GMAC_IMR_RCOMP_Pos                  1                                              /**< (GMAC_IMR) Receive Complete Position */
#define GMAC_IMR_RCOMP_Msk                  (0x1U << GMAC_IMR_RCOMP_Pos)                   /**< (GMAC_IMR) Receive Complete Mask */
#define GMAC_IMR_RCOMP                      GMAC_IMR_RCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_RCOMP_Msk instead */
#define GMAC_IMR_RXUBR_Pos                  2                                              /**< (GMAC_IMR) RX Used Bit Read Position */
#define GMAC_IMR_RXUBR_Msk                  (0x1U << GMAC_IMR_RXUBR_Pos)                   /**< (GMAC_IMR) RX Used Bit Read Mask */
#define GMAC_IMR_RXUBR                      GMAC_IMR_RXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_RXUBR_Msk instead */
#define GMAC_IMR_TXUBR_Pos                  3                                              /**< (GMAC_IMR) TX Used Bit Read Position */
#define GMAC_IMR_TXUBR_Msk                  (0x1U << GMAC_IMR_TXUBR_Pos)                   /**< (GMAC_IMR) TX Used Bit Read Mask */
#define GMAC_IMR_TXUBR                      GMAC_IMR_TXUBR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_TXUBR_Msk instead */
#define GMAC_IMR_TUR_Pos                    4                                              /**< (GMAC_IMR) Transmit Underrun Position */
#define GMAC_IMR_TUR_Msk                    (0x1U << GMAC_IMR_TUR_Pos)                     /**< (GMAC_IMR) Transmit Underrun Mask */
#define GMAC_IMR_TUR                        GMAC_IMR_TUR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_TUR_Msk instead */
#define GMAC_IMR_RLEX_Pos                   5                                              /**< (GMAC_IMR) Retry Limit Exceeded Position */
#define GMAC_IMR_RLEX_Msk                   (0x1U << GMAC_IMR_RLEX_Pos)                    /**< (GMAC_IMR) Retry Limit Exceeded Mask */
#define GMAC_IMR_RLEX                       GMAC_IMR_RLEX_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_RLEX_Msk instead */
#define GMAC_IMR_TFC_Pos                    6                                              /**< (GMAC_IMR) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_IMR_TFC_Msk                    (0x1U << GMAC_IMR_TFC_Pos)                     /**< (GMAC_IMR) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_IMR_TFC                        GMAC_IMR_TFC_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_TFC_Msk instead */
#define GMAC_IMR_TCOMP_Pos                  7                                              /**< (GMAC_IMR) Transmit Complete Position */
#define GMAC_IMR_TCOMP_Msk                  (0x1U << GMAC_IMR_TCOMP_Pos)                   /**< (GMAC_IMR) Transmit Complete Mask */
#define GMAC_IMR_TCOMP                      GMAC_IMR_TCOMP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_TCOMP_Msk instead */
#define GMAC_IMR_ROVR_Pos                   10                                             /**< (GMAC_IMR) Receive Overrun Position */
#define GMAC_IMR_ROVR_Msk                   (0x1U << GMAC_IMR_ROVR_Pos)                    /**< (GMAC_IMR) Receive Overrun Mask */
#define GMAC_IMR_ROVR                       GMAC_IMR_ROVR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_ROVR_Msk instead */
#define GMAC_IMR_HRESP_Pos                  11                                             /**< (GMAC_IMR) HRESP Not OK Position */
#define GMAC_IMR_HRESP_Msk                  (0x1U << GMAC_IMR_HRESP_Pos)                   /**< (GMAC_IMR) HRESP Not OK Mask */
#define GMAC_IMR_HRESP                      GMAC_IMR_HRESP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_HRESP_Msk instead */
#define GMAC_IMR_PFNZ_Pos                   12                                             /**< (GMAC_IMR) Pause Frame with Non-zero Pause Quantum Received Position */
#define GMAC_IMR_PFNZ_Msk                   (0x1U << GMAC_IMR_PFNZ_Pos)                    /**< (GMAC_IMR) Pause Frame with Non-zero Pause Quantum Received Mask */
#define GMAC_IMR_PFNZ                       GMAC_IMR_PFNZ_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_PFNZ_Msk instead */
#define GMAC_IMR_PTZ_Pos                    13                                             /**< (GMAC_IMR) Pause Time Zero Position */
#define GMAC_IMR_PTZ_Msk                    (0x1U << GMAC_IMR_PTZ_Pos)                     /**< (GMAC_IMR) Pause Time Zero Mask */
#define GMAC_IMR_PTZ                        GMAC_IMR_PTZ_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_PTZ_Msk instead */
#define GMAC_IMR_PFTR_Pos                   14                                             /**< (GMAC_IMR) Pause Frame Transmitted Position */
#define GMAC_IMR_PFTR_Msk                   (0x1U << GMAC_IMR_PFTR_Pos)                    /**< (GMAC_IMR) Pause Frame Transmitted Mask */
#define GMAC_IMR_PFTR                       GMAC_IMR_PFTR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_PFTR_Msk instead */
#define GMAC_IMR_EXINT_Pos                  15                                             /**< (GMAC_IMR) External Interrupt Position */
#define GMAC_IMR_EXINT_Msk                  (0x1U << GMAC_IMR_EXINT_Pos)                   /**< (GMAC_IMR) External Interrupt Mask */
#define GMAC_IMR_EXINT                      GMAC_IMR_EXINT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_EXINT_Msk instead */
#define GMAC_IMR_DRQFR_Pos                  18                                             /**< (GMAC_IMR) PTP Delay Request Frame Received Position */
#define GMAC_IMR_DRQFR_Msk                  (0x1U << GMAC_IMR_DRQFR_Pos)                   /**< (GMAC_IMR) PTP Delay Request Frame Received Mask */
#define GMAC_IMR_DRQFR                      GMAC_IMR_DRQFR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_DRQFR_Msk instead */
#define GMAC_IMR_SFR_Pos                    19                                             /**< (GMAC_IMR) PTP Sync Frame Received Position */
#define GMAC_IMR_SFR_Msk                    (0x1U << GMAC_IMR_SFR_Pos)                     /**< (GMAC_IMR) PTP Sync Frame Received Mask */
#define GMAC_IMR_SFR                        GMAC_IMR_SFR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_SFR_Msk instead */
#define GMAC_IMR_DRQFT_Pos                  20                                             /**< (GMAC_IMR) PTP Delay Request Frame Transmitted Position */
#define GMAC_IMR_DRQFT_Msk                  (0x1U << GMAC_IMR_DRQFT_Pos)                   /**< (GMAC_IMR) PTP Delay Request Frame Transmitted Mask */
#define GMAC_IMR_DRQFT                      GMAC_IMR_DRQFT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_DRQFT_Msk instead */
#define GMAC_IMR_SFT_Pos                    21                                             /**< (GMAC_IMR) PTP Sync Frame Transmitted Position */
#define GMAC_IMR_SFT_Msk                    (0x1U << GMAC_IMR_SFT_Pos)                     /**< (GMAC_IMR) PTP Sync Frame Transmitted Mask */
#define GMAC_IMR_SFT                        GMAC_IMR_SFT_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_SFT_Msk instead */
#define GMAC_IMR_PDRQFR_Pos                 22                                             /**< (GMAC_IMR) PDelay Request Frame Received Position */
#define GMAC_IMR_PDRQFR_Msk                 (0x1U << GMAC_IMR_PDRQFR_Pos)                  /**< (GMAC_IMR) PDelay Request Frame Received Mask */
#define GMAC_IMR_PDRQFR                     GMAC_IMR_PDRQFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_PDRQFR_Msk instead */
#define GMAC_IMR_PDRSFR_Pos                 23                                             /**< (GMAC_IMR) PDelay Response Frame Received Position */
#define GMAC_IMR_PDRSFR_Msk                 (0x1U << GMAC_IMR_PDRSFR_Pos)                  /**< (GMAC_IMR) PDelay Response Frame Received Mask */
#define GMAC_IMR_PDRSFR                     GMAC_IMR_PDRSFR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_PDRSFR_Msk instead */
#define GMAC_IMR_PDRQFT_Pos                 24                                             /**< (GMAC_IMR) PDelay Request Frame Transmitted Position */
#define GMAC_IMR_PDRQFT_Msk                 (0x1U << GMAC_IMR_PDRQFT_Pos)                  /**< (GMAC_IMR) PDelay Request Frame Transmitted Mask */
#define GMAC_IMR_PDRQFT                     GMAC_IMR_PDRQFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_PDRQFT_Msk instead */
#define GMAC_IMR_PDRSFT_Pos                 25                                             /**< (GMAC_IMR) PDelay Response Frame Transmitted Position */
#define GMAC_IMR_PDRSFT_Msk                 (0x1U << GMAC_IMR_PDRSFT_Pos)                  /**< (GMAC_IMR) PDelay Response Frame Transmitted Mask */
#define GMAC_IMR_PDRSFT                     GMAC_IMR_PDRSFT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMR_PDRSFT_Msk instead */
#define GMAC_IMR_MASK                       (0x3FCFCFFU)                                   /**< \deprecated (GMAC_IMR) Register MASK  (Use GMAC_IMR_Msk instead)  */
#define GMAC_IMR_Msk                        (0x3FCFCFFU)                                   /**< (GMAC_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_MAN : (GMAC Offset: 0x34) (R/W 32) PHY Maintenance Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATA:16;                   /**< bit:  0..15  PHY Data                                 */
    uint32_t WTN:2;                     /**< bit: 16..17  Write Ten                                */
    uint32_t REGA:5;                    /**< bit: 18..22  Register Address                         */
    uint32_t PHYA:5;                    /**< bit: 23..27  PHY Address                              */
    uint32_t OP:2;                      /**< bit: 28..29  Operation                                */
    uint32_t CLTTO:1;                   /**< bit:     30  Clause 22 Operation                      */
    uint32_t WZO:1;                     /**< bit:     31  Write ZERO                               */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_MAN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MAN_OFFSET                     (0x34)                                        /**<  (GMAC_MAN) PHY Maintenance Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_MAN_DATA_Pos                   0                                              /**< (GMAC_MAN) PHY Data Position */
#define GMAC_MAN_DATA_Msk                   (0xFFFFU << GMAC_MAN_DATA_Pos)                 /**< (GMAC_MAN) PHY Data Mask */
#define GMAC_MAN_DATA(value)                (GMAC_MAN_DATA_Msk & ((value) << GMAC_MAN_DATA_Pos))
#define GMAC_MAN_WTN_Pos                    16                                             /**< (GMAC_MAN) Write Ten Position */
#define GMAC_MAN_WTN_Msk                    (0x3U << GMAC_MAN_WTN_Pos)                     /**< (GMAC_MAN) Write Ten Mask */
#define GMAC_MAN_WTN(value)                 (GMAC_MAN_WTN_Msk & ((value) << GMAC_MAN_WTN_Pos))
#define GMAC_MAN_REGA_Pos                   18                                             /**< (GMAC_MAN) Register Address Position */
#define GMAC_MAN_REGA_Msk                   (0x1FU << GMAC_MAN_REGA_Pos)                   /**< (GMAC_MAN) Register Address Mask */
#define GMAC_MAN_REGA(value)                (GMAC_MAN_REGA_Msk & ((value) << GMAC_MAN_REGA_Pos))
#define GMAC_MAN_PHYA_Pos                   23                                             /**< (GMAC_MAN) PHY Address Position */
#define GMAC_MAN_PHYA_Msk                   (0x1FU << GMAC_MAN_PHYA_Pos)                   /**< (GMAC_MAN) PHY Address Mask */
#define GMAC_MAN_PHYA(value)                (GMAC_MAN_PHYA_Msk & ((value) << GMAC_MAN_PHYA_Pos))
#define GMAC_MAN_OP_Pos                     28                                             /**< (GMAC_MAN) Operation Position */
#define GMAC_MAN_OP_Msk                     (0x3U << GMAC_MAN_OP_Pos)                      /**< (GMAC_MAN) Operation Mask */
#define GMAC_MAN_OP(value)                  (GMAC_MAN_OP_Msk & ((value) << GMAC_MAN_OP_Pos))
#define GMAC_MAN_CLTTO_Pos                  30                                             /**< (GMAC_MAN) Clause 22 Operation Position */
#define GMAC_MAN_CLTTO_Msk                  (0x1U << GMAC_MAN_CLTTO_Pos)                   /**< (GMAC_MAN) Clause 22 Operation Mask */
#define GMAC_MAN_CLTTO                      GMAC_MAN_CLTTO_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_MAN_CLTTO_Msk instead */
#define GMAC_MAN_WZO_Pos                    31                                             /**< (GMAC_MAN) Write ZERO Position */
#define GMAC_MAN_WZO_Msk                    (0x1U << GMAC_MAN_WZO_Pos)                     /**< (GMAC_MAN) Write ZERO Mask */
#define GMAC_MAN_WZO                        GMAC_MAN_WZO_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_MAN_WZO_Msk instead */
#define GMAC_MAN_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_MAN) Register MASK  (Use GMAC_MAN_Msk instead)  */
#define GMAC_MAN_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_MAN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RPQ : (GMAC Offset: 0x38) (R/ 32) Received Pause Quantum Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RPQ:16;                    /**< bit:  0..15  Received Pause Quantum                   */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RPQ_OFFSET                     (0x38)                                        /**<  (GMAC_RPQ) Received Pause Quantum Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RPQ_RPQ_Pos                    0                                              /**< (GMAC_RPQ) Received Pause Quantum Position */
#define GMAC_RPQ_RPQ_Msk                    (0xFFFFU << GMAC_RPQ_RPQ_Pos)                  /**< (GMAC_RPQ) Received Pause Quantum Mask */
#define GMAC_RPQ_RPQ(value)                 (GMAC_RPQ_RPQ_Msk & ((value) << GMAC_RPQ_RPQ_Pos))
#define GMAC_RPQ_MASK                       (0xFFFFU)                                      /**< \deprecated (GMAC_RPQ) Register MASK  (Use GMAC_RPQ_Msk instead)  */
#define GMAC_RPQ_Msk                        (0xFFFFU)                                      /**< (GMAC_RPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TPQ : (GMAC Offset: 0x3c) (R/W 32) Transmit Pause Quantum Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TPQ:16;                    /**< bit:  0..15  Transmit Pause Quantum                   */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TPQ_OFFSET                     (0x3C)                                        /**<  (GMAC_TPQ) Transmit Pause Quantum Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TPQ_TPQ_Pos                    0                                              /**< (GMAC_TPQ) Transmit Pause Quantum Position */
#define GMAC_TPQ_TPQ_Msk                    (0xFFFFU << GMAC_TPQ_TPQ_Pos)                  /**< (GMAC_TPQ) Transmit Pause Quantum Mask */
#define GMAC_TPQ_TPQ(value)                 (GMAC_TPQ_TPQ_Msk & ((value) << GMAC_TPQ_TPQ_Pos))
#define GMAC_TPQ_MASK                       (0xFFFFU)                                      /**< \deprecated (GMAC_TPQ) Register MASK  (Use GMAC_TPQ_Msk instead)  */
#define GMAC_TPQ_Msk                        (0xFFFFU)                                      /**< (GMAC_TPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TPSF : (GMAC Offset: 0x40) (R/W 32) TX Partial Store and Forward Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TPB1ADR:12;                /**< bit:  0..11  Transmit Partial Store and Forward Address */
    uint32_t :19;                       /**< bit: 12..30  Reserved */
    uint32_t ENTXP:1;                   /**< bit:     31  Enable TX Partial Store and Forward Operation */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TPSF_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TPSF_OFFSET                    (0x40)                                        /**<  (GMAC_TPSF) TX Partial Store and Forward Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TPSF_TPB1ADR_Pos               0                                              /**< (GMAC_TPSF) Transmit Partial Store and Forward Address Position */
#define GMAC_TPSF_TPB1ADR_Msk               (0xFFFU << GMAC_TPSF_TPB1ADR_Pos)              /**< (GMAC_TPSF) Transmit Partial Store and Forward Address Mask */
#define GMAC_TPSF_TPB1ADR(value)            (GMAC_TPSF_TPB1ADR_Msk & ((value) << GMAC_TPSF_TPB1ADR_Pos))
#define GMAC_TPSF_ENTXP_Pos                 31                                             /**< (GMAC_TPSF) Enable TX Partial Store and Forward Operation Position */
#define GMAC_TPSF_ENTXP_Msk                 (0x1U << GMAC_TPSF_ENTXP_Pos)                  /**< (GMAC_TPSF) Enable TX Partial Store and Forward Operation Mask */
#define GMAC_TPSF_ENTXP                     GMAC_TPSF_ENTXP_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TPSF_ENTXP_Msk instead */
#define GMAC_TPSF_MASK                      (0x80000FFFU)                                  /**< \deprecated (GMAC_TPSF) Register MASK  (Use GMAC_TPSF_Msk instead)  */
#define GMAC_TPSF_Msk                       (0x80000FFFU)                                  /**< (GMAC_TPSF) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RPSF : (GMAC Offset: 0x44) (R/W 32) RX Partial Store and Forward Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RPB1ADR:12;                /**< bit:  0..11  Receive Partial Store and Forward Address */
    uint32_t :19;                       /**< bit: 12..30  Reserved */
    uint32_t ENRXP:1;                   /**< bit:     31  Enable RX Partial Store and Forward Operation */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RPSF_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RPSF_OFFSET                    (0x44)                                        /**<  (GMAC_RPSF) RX Partial Store and Forward Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RPSF_RPB1ADR_Pos               0                                              /**< (GMAC_RPSF) Receive Partial Store and Forward Address Position */
#define GMAC_RPSF_RPB1ADR_Msk               (0xFFFU << GMAC_RPSF_RPB1ADR_Pos)              /**< (GMAC_RPSF) Receive Partial Store and Forward Address Mask */
#define GMAC_RPSF_RPB1ADR(value)            (GMAC_RPSF_RPB1ADR_Msk & ((value) << GMAC_RPSF_RPB1ADR_Pos))
#define GMAC_RPSF_ENRXP_Pos                 31                                             /**< (GMAC_RPSF) Enable RX Partial Store and Forward Operation Position */
#define GMAC_RPSF_ENRXP_Msk                 (0x1U << GMAC_RPSF_ENRXP_Pos)                  /**< (GMAC_RPSF) Enable RX Partial Store and Forward Operation Mask */
#define GMAC_RPSF_ENRXP                     GMAC_RPSF_ENRXP_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_RPSF_ENRXP_Msk instead */
#define GMAC_RPSF_MASK                      (0x80000FFFU)                                  /**< \deprecated (GMAC_RPSF) Register MASK  (Use GMAC_RPSF_Msk instead)  */
#define GMAC_RPSF_Msk                       (0x80000FFFU)                                  /**< (GMAC_RPSF) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RJFML : (GMAC Offset: 0x48) (R/W 32) RX Jumbo Frame Max Length Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FML:14;                    /**< bit:  0..13  Frame Max Length                         */
    uint32_t :18;                       /**< bit: 14..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RJFML_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RJFML_OFFSET                   (0x48)                                        /**<  (GMAC_RJFML) RX Jumbo Frame Max Length Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RJFML_FML_Pos                  0                                              /**< (GMAC_RJFML) Frame Max Length Position */
#define GMAC_RJFML_FML_Msk                  (0x3FFFU << GMAC_RJFML_FML_Pos)                /**< (GMAC_RJFML) Frame Max Length Mask */
#define GMAC_RJFML_FML(value)               (GMAC_RJFML_FML_Msk & ((value) << GMAC_RJFML_FML_Pos))
#define GMAC_RJFML_MASK                     (0x3FFFU)                                      /**< \deprecated (GMAC_RJFML) Register MASK  (Use GMAC_RJFML_Msk instead)  */
#define GMAC_RJFML_Msk                      (0x3FFFU)                                      /**< (GMAC_RJFML) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_HRB : (GMAC Offset: 0x80) (R/W 32) Hash Register Bottom -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ADDR:32;                   /**< bit:  0..31  Hash Address                             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_HRB_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_HRB_OFFSET                     (0x80)                                        /**<  (GMAC_HRB) Hash Register Bottom  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_HRB_ADDR_Pos                   0                                              /**< (GMAC_HRB) Hash Address Position */
#define GMAC_HRB_ADDR_Msk                   (0xFFFFFFFFU << GMAC_HRB_ADDR_Pos)             /**< (GMAC_HRB) Hash Address Mask */
#define GMAC_HRB_ADDR(value)                (GMAC_HRB_ADDR_Msk & ((value) << GMAC_HRB_ADDR_Pos))
#define GMAC_HRB_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_HRB) Register MASK  (Use GMAC_HRB_Msk instead)  */
#define GMAC_HRB_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_HRB) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_HRT : (GMAC Offset: 0x84) (R/W 32) Hash Register Top -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ADDR:32;                   /**< bit:  0..31  Hash Address                             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_HRT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_HRT_OFFSET                     (0x84)                                        /**<  (GMAC_HRT) Hash Register Top  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_HRT_ADDR_Pos                   0                                              /**< (GMAC_HRT) Hash Address Position */
#define GMAC_HRT_ADDR_Msk                   (0xFFFFFFFFU << GMAC_HRT_ADDR_Pos)             /**< (GMAC_HRT) Hash Address Mask */
#define GMAC_HRT_ADDR(value)                (GMAC_HRT_ADDR_Msk & ((value) << GMAC_HRT_ADDR_Pos))
#define GMAC_HRT_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_HRT) Register MASK  (Use GMAC_HRT_Msk instead)  */
#define GMAC_HRT_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_HRT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TIDM1 : (GMAC Offset: 0xa8) (R/W 32) Type ID Match 1 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TID:16;                    /**< bit:  0..15  Type ID Match 1                          */
    uint32_t :15;                       /**< bit: 16..30  Reserved */
    uint32_t ENID1:1;                   /**< bit:     31  Enable Copying of TID Matched Frames     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TIDM1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TIDM1_OFFSET                   (0xA8)                                        /**<  (GMAC_TIDM1) Type ID Match 1 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TIDM1_TID_Pos                  0                                              /**< (GMAC_TIDM1) Type ID Match 1 Position */
#define GMAC_TIDM1_TID_Msk                  (0xFFFFU << GMAC_TIDM1_TID_Pos)                /**< (GMAC_TIDM1) Type ID Match 1 Mask */
#define GMAC_TIDM1_TID(value)               (GMAC_TIDM1_TID_Msk & ((value) << GMAC_TIDM1_TID_Pos))
#define GMAC_TIDM1_ENID1_Pos                31                                             /**< (GMAC_TIDM1) Enable Copying of TID Matched Frames Position */
#define GMAC_TIDM1_ENID1_Msk                (0x1U << GMAC_TIDM1_ENID1_Pos)                 /**< (GMAC_TIDM1) Enable Copying of TID Matched Frames Mask */
#define GMAC_TIDM1_ENID1                    GMAC_TIDM1_ENID1_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TIDM1_ENID1_Msk instead */
#define GMAC_TIDM1_MASK                     (0x8000FFFFU)                                  /**< \deprecated (GMAC_TIDM1) Register MASK  (Use GMAC_TIDM1_Msk instead)  */
#define GMAC_TIDM1_Msk                      (0x8000FFFFU)                                  /**< (GMAC_TIDM1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TIDM2 : (GMAC Offset: 0xac) (R/W 32) Type ID Match 2 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TID:16;                    /**< bit:  0..15  Type ID Match 2                          */
    uint32_t :15;                       /**< bit: 16..30  Reserved */
    uint32_t ENID2:1;                   /**< bit:     31  Enable Copying of TID Matched Frames     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TIDM2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TIDM2_OFFSET                   (0xAC)                                        /**<  (GMAC_TIDM2) Type ID Match 2 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TIDM2_TID_Pos                  0                                              /**< (GMAC_TIDM2) Type ID Match 2 Position */
#define GMAC_TIDM2_TID_Msk                  (0xFFFFU << GMAC_TIDM2_TID_Pos)                /**< (GMAC_TIDM2) Type ID Match 2 Mask */
#define GMAC_TIDM2_TID(value)               (GMAC_TIDM2_TID_Msk & ((value) << GMAC_TIDM2_TID_Pos))
#define GMAC_TIDM2_ENID2_Pos                31                                             /**< (GMAC_TIDM2) Enable Copying of TID Matched Frames Position */
#define GMAC_TIDM2_ENID2_Msk                (0x1U << GMAC_TIDM2_ENID2_Pos)                 /**< (GMAC_TIDM2) Enable Copying of TID Matched Frames Mask */
#define GMAC_TIDM2_ENID2                    GMAC_TIDM2_ENID2_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TIDM2_ENID2_Msk instead */
#define GMAC_TIDM2_MASK                     (0x8000FFFFU)                                  /**< \deprecated (GMAC_TIDM2) Register MASK  (Use GMAC_TIDM2_Msk instead)  */
#define GMAC_TIDM2_Msk                      (0x8000FFFFU)                                  /**< (GMAC_TIDM2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TIDM3 : (GMAC Offset: 0xb0) (R/W 32) Type ID Match 3 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TID:16;                    /**< bit:  0..15  Type ID Match 3                          */
    uint32_t :15;                       /**< bit: 16..30  Reserved */
    uint32_t ENID3:1;                   /**< bit:     31  Enable Copying of TID Matched Frames     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TIDM3_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TIDM3_OFFSET                   (0xB0)                                        /**<  (GMAC_TIDM3) Type ID Match 3 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TIDM3_TID_Pos                  0                                              /**< (GMAC_TIDM3) Type ID Match 3 Position */
#define GMAC_TIDM3_TID_Msk                  (0xFFFFU << GMAC_TIDM3_TID_Pos)                /**< (GMAC_TIDM3) Type ID Match 3 Mask */
#define GMAC_TIDM3_TID(value)               (GMAC_TIDM3_TID_Msk & ((value) << GMAC_TIDM3_TID_Pos))
#define GMAC_TIDM3_ENID3_Pos                31                                             /**< (GMAC_TIDM3) Enable Copying of TID Matched Frames Position */
#define GMAC_TIDM3_ENID3_Msk                (0x1U << GMAC_TIDM3_ENID3_Pos)                 /**< (GMAC_TIDM3) Enable Copying of TID Matched Frames Mask */
#define GMAC_TIDM3_ENID3                    GMAC_TIDM3_ENID3_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TIDM3_ENID3_Msk instead */
#define GMAC_TIDM3_MASK                     (0x8000FFFFU)                                  /**< \deprecated (GMAC_TIDM3) Register MASK  (Use GMAC_TIDM3_Msk instead)  */
#define GMAC_TIDM3_Msk                      (0x8000FFFFU)                                  /**< (GMAC_TIDM3) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TIDM4 : (GMAC Offset: 0xb4) (R/W 32) Type ID Match 4 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TID:16;                    /**< bit:  0..15  Type ID Match 4                          */
    uint32_t :15;                       /**< bit: 16..30  Reserved */
    uint32_t ENID4:1;                   /**< bit:     31  Enable Copying of TID Matched Frames     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TIDM4_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TIDM4_OFFSET                   (0xB4)                                        /**<  (GMAC_TIDM4) Type ID Match 4 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TIDM4_TID_Pos                  0                                              /**< (GMAC_TIDM4) Type ID Match 4 Position */
#define GMAC_TIDM4_TID_Msk                  (0xFFFFU << GMAC_TIDM4_TID_Pos)                /**< (GMAC_TIDM4) Type ID Match 4 Mask */
#define GMAC_TIDM4_TID(value)               (GMAC_TIDM4_TID_Msk & ((value) << GMAC_TIDM4_TID_Pos))
#define GMAC_TIDM4_ENID4_Pos                31                                             /**< (GMAC_TIDM4) Enable Copying of TID Matched Frames Position */
#define GMAC_TIDM4_ENID4_Msk                (0x1U << GMAC_TIDM4_ENID4_Pos)                 /**< (GMAC_TIDM4) Enable Copying of TID Matched Frames Mask */
#define GMAC_TIDM4_ENID4                    GMAC_TIDM4_ENID4_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TIDM4_ENID4_Msk instead */
#define GMAC_TIDM4_MASK                     (0x8000FFFFU)                                  /**< \deprecated (GMAC_TIDM4) Register MASK  (Use GMAC_TIDM4_Msk instead)  */
#define GMAC_TIDM4_Msk                      (0x8000FFFFU)                                  /**< (GMAC_TIDM4) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_WOL : (GMAC Offset: 0xb8) (R/W 32) Wake on LAN Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IP:16;                     /**< bit:  0..15  ARP Request IP Address                   */
    uint32_t MAG:1;                     /**< bit:     16  Magic Packet Event Enable                */
    uint32_t ARP:1;                     /**< bit:     17  ARP Request IP Address                   */
    uint32_t SA1:1;                     /**< bit:     18  Specific Address Register 1 Event Enable */
    uint32_t MTI:1;                     /**< bit:     19  Multicast Hash Event Enable              */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_WOL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_WOL_OFFSET                     (0xB8)                                        /**<  (GMAC_WOL) Wake on LAN Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_WOL_IP_Pos                     0                                              /**< (GMAC_WOL) ARP Request IP Address Position */
#define GMAC_WOL_IP_Msk                     (0xFFFFU << GMAC_WOL_IP_Pos)                   /**< (GMAC_WOL) ARP Request IP Address Mask */
#define GMAC_WOL_IP(value)                  (GMAC_WOL_IP_Msk & ((value) << GMAC_WOL_IP_Pos))
#define GMAC_WOL_MAG_Pos                    16                                             /**< (GMAC_WOL) Magic Packet Event Enable Position */
#define GMAC_WOL_MAG_Msk                    (0x1U << GMAC_WOL_MAG_Pos)                     /**< (GMAC_WOL) Magic Packet Event Enable Mask */
#define GMAC_WOL_MAG                        GMAC_WOL_MAG_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_WOL_MAG_Msk instead */
#define GMAC_WOL_ARP_Pos                    17                                             /**< (GMAC_WOL) ARP Request IP Address Position */
#define GMAC_WOL_ARP_Msk                    (0x1U << GMAC_WOL_ARP_Pos)                     /**< (GMAC_WOL) ARP Request IP Address Mask */
#define GMAC_WOL_ARP                        GMAC_WOL_ARP_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_WOL_ARP_Msk instead */
#define GMAC_WOL_SA1_Pos                    18                                             /**< (GMAC_WOL) Specific Address Register 1 Event Enable Position */
#define GMAC_WOL_SA1_Msk                    (0x1U << GMAC_WOL_SA1_Pos)                     /**< (GMAC_WOL) Specific Address Register 1 Event Enable Mask */
#define GMAC_WOL_SA1                        GMAC_WOL_SA1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_WOL_SA1_Msk instead */
#define GMAC_WOL_MTI_Pos                    19                                             /**< (GMAC_WOL) Multicast Hash Event Enable Position */
#define GMAC_WOL_MTI_Msk                    (0x1U << GMAC_WOL_MTI_Pos)                     /**< (GMAC_WOL) Multicast Hash Event Enable Mask */
#define GMAC_WOL_MTI                        GMAC_WOL_MTI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_WOL_MTI_Msk instead */
#define GMAC_WOL_MASK                       (0xFFFFFU)                                     /**< \deprecated (GMAC_WOL) Register MASK  (Use GMAC_WOL_Msk instead)  */
#define GMAC_WOL_Msk                        (0xFFFFFU)                                     /**< (GMAC_WOL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IPGS : (GMAC Offset: 0xbc) (R/W 32) IPG Stretch Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FL:16;                     /**< bit:  0..15  Frame Length                             */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IPGS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IPGS_OFFSET                    (0xBC)                                        /**<  (GMAC_IPGS) IPG Stretch Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IPGS_FL_Pos                    0                                              /**< (GMAC_IPGS) Frame Length Position */
#define GMAC_IPGS_FL_Msk                    (0xFFFFU << GMAC_IPGS_FL_Pos)                  /**< (GMAC_IPGS) Frame Length Mask */
#define GMAC_IPGS_FL(value)                 (GMAC_IPGS_FL_Msk & ((value) << GMAC_IPGS_FL_Pos))
#define GMAC_IPGS_MASK                      (0xFFFFU)                                      /**< \deprecated (GMAC_IPGS) Register MASK  (Use GMAC_IPGS_Msk instead)  */
#define GMAC_IPGS_Msk                       (0xFFFFU)                                      /**< (GMAC_IPGS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_SVLAN : (GMAC Offset: 0xc0) (R/W 32) Stacked VLAN Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t VLAN_TYPE:16;              /**< bit:  0..15  User Defined VLAN_TYPE Field             */
    uint32_t :15;                       /**< bit: 16..30  Reserved */
    uint32_t ESVLAN:1;                  /**< bit:     31  Enable Stacked VLAN Processing Mode      */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SVLAN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SVLAN_OFFSET                   (0xC0)                                        /**<  (GMAC_SVLAN) Stacked VLAN Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SVLAN_VLAN_TYPE_Pos            0                                              /**< (GMAC_SVLAN) User Defined VLAN_TYPE Field Position */
#define GMAC_SVLAN_VLAN_TYPE_Msk            (0xFFFFU << GMAC_SVLAN_VLAN_TYPE_Pos)          /**< (GMAC_SVLAN) User Defined VLAN_TYPE Field Mask */
#define GMAC_SVLAN_VLAN_TYPE(value)         (GMAC_SVLAN_VLAN_TYPE_Msk & ((value) << GMAC_SVLAN_VLAN_TYPE_Pos))
#define GMAC_SVLAN_ESVLAN_Pos               31                                             /**< (GMAC_SVLAN) Enable Stacked VLAN Processing Mode Position */
#define GMAC_SVLAN_ESVLAN_Msk               (0x1U << GMAC_SVLAN_ESVLAN_Pos)                /**< (GMAC_SVLAN) Enable Stacked VLAN Processing Mode Mask */
#define GMAC_SVLAN_ESVLAN                   GMAC_SVLAN_ESVLAN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_SVLAN_ESVLAN_Msk instead */
#define GMAC_SVLAN_MASK                     (0x8000FFFFU)                                  /**< \deprecated (GMAC_SVLAN) Register MASK  (Use GMAC_SVLAN_Msk instead)  */
#define GMAC_SVLAN_Msk                      (0x8000FFFFU)                                  /**< (GMAC_SVLAN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TPFCP : (GMAC Offset: 0xc4) (R/W 32) Transmit PFC Pause Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PEV:8;                     /**< bit:   0..7  Priority Enable Vector                   */
    uint32_t PQ:8;                      /**< bit:  8..15  Pause Quantum                            */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TPFCP_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TPFCP_OFFSET                   (0xC4)                                        /**<  (GMAC_TPFCP) Transmit PFC Pause Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TPFCP_PEV_Pos                  0                                              /**< (GMAC_TPFCP) Priority Enable Vector Position */
#define GMAC_TPFCP_PEV_Msk                  (0xFFU << GMAC_TPFCP_PEV_Pos)                  /**< (GMAC_TPFCP) Priority Enable Vector Mask */
#define GMAC_TPFCP_PEV(value)               (GMAC_TPFCP_PEV_Msk & ((value) << GMAC_TPFCP_PEV_Pos))
#define GMAC_TPFCP_PQ_Pos                   8                                              /**< (GMAC_TPFCP) Pause Quantum Position */
#define GMAC_TPFCP_PQ_Msk                   (0xFFU << GMAC_TPFCP_PQ_Pos)                   /**< (GMAC_TPFCP) Pause Quantum Mask */
#define GMAC_TPFCP_PQ(value)                (GMAC_TPFCP_PQ_Msk & ((value) << GMAC_TPFCP_PQ_Pos))
#define GMAC_TPFCP_MASK                     (0xFFFFU)                                      /**< \deprecated (GMAC_TPFCP) Register MASK  (Use GMAC_TPFCP_Msk instead)  */
#define GMAC_TPFCP_Msk                      (0xFFFFU)                                      /**< (GMAC_TPFCP) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_SAMB1 : (GMAC Offset: 0xc8) (R/W 32) Specific Address 1 Mask Bottom Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ADDR:32;                   /**< bit:  0..31  Specific Address 1 Mask                  */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SAMB1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAMB1_OFFSET                   (0xC8)                                        /**<  (GMAC_SAMB1) Specific Address 1 Mask Bottom Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SAMB1_ADDR_Pos                 0                                              /**< (GMAC_SAMB1) Specific Address 1 Mask Position */
#define GMAC_SAMB1_ADDR_Msk                 (0xFFFFFFFFU << GMAC_SAMB1_ADDR_Pos)           /**< (GMAC_SAMB1) Specific Address 1 Mask Mask */
#define GMAC_SAMB1_ADDR(value)              (GMAC_SAMB1_ADDR_Msk & ((value) << GMAC_SAMB1_ADDR_Pos))
#define GMAC_SAMB1_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_SAMB1) Register MASK  (Use GMAC_SAMB1_Msk instead)  */
#define GMAC_SAMB1_Msk                      (0xFFFFFFFFU)                                  /**< (GMAC_SAMB1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_SAMT1 : (GMAC Offset: 0xcc) (R/W 32) Specific Address 1 Mask Top Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ADDR:16;                   /**< bit:  0..15  Specific Address 1 Mask                  */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SAMT1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAMT1_OFFSET                   (0xCC)                                        /**<  (GMAC_SAMT1) Specific Address 1 Mask Top Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SAMT1_ADDR_Pos                 0                                              /**< (GMAC_SAMT1) Specific Address 1 Mask Position */
#define GMAC_SAMT1_ADDR_Msk                 (0xFFFFU << GMAC_SAMT1_ADDR_Pos)               /**< (GMAC_SAMT1) Specific Address 1 Mask Mask */
#define GMAC_SAMT1_ADDR(value)              (GMAC_SAMT1_ADDR_Msk & ((value) << GMAC_SAMT1_ADDR_Pos))
#define GMAC_SAMT1_MASK                     (0xFFFFU)                                      /**< \deprecated (GMAC_SAMT1) Register MASK  (Use GMAC_SAMT1_Msk instead)  */
#define GMAC_SAMT1_Msk                      (0xFFFFU)                                      /**< (GMAC_SAMT1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_NSC : (GMAC Offset: 0xdc) (R/W 32) 1588 Timer Nanosecond Comparison Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NANOSEC:22;                /**< bit:  0..21  1588 Timer Nanosecond Comparison Value   */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_NSC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NSC_OFFSET                     (0xDC)                                        /**<  (GMAC_NSC) 1588 Timer Nanosecond Comparison Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_NSC_NANOSEC_Pos                0                                              /**< (GMAC_NSC) 1588 Timer Nanosecond Comparison Value Position */
#define GMAC_NSC_NANOSEC_Msk                (0x3FFFFFU << GMAC_NSC_NANOSEC_Pos)            /**< (GMAC_NSC) 1588 Timer Nanosecond Comparison Value Mask */
#define GMAC_NSC_NANOSEC(value)             (GMAC_NSC_NANOSEC_Msk & ((value) << GMAC_NSC_NANOSEC_Pos))
#define GMAC_NSC_MASK                       (0x3FFFFFU)                                    /**< \deprecated (GMAC_NSC) Register MASK  (Use GMAC_NSC_Msk instead)  */
#define GMAC_NSC_Msk                        (0x3FFFFFU)                                    /**< (GMAC_NSC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_SCL : (GMAC Offset: 0xe0) (R/W 32) 1588 Timer Second Comparison Low Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SEC:32;                    /**< bit:  0..31  1588 Timer Second Comparison Value       */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SCL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SCL_OFFSET                     (0xE0)                                        /**<  (GMAC_SCL) 1588 Timer Second Comparison Low Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SCL_SEC_Pos                    0                                              /**< (GMAC_SCL) 1588 Timer Second Comparison Value Position */
#define GMAC_SCL_SEC_Msk                    (0xFFFFFFFFU << GMAC_SCL_SEC_Pos)              /**< (GMAC_SCL) 1588 Timer Second Comparison Value Mask */
#define GMAC_SCL_SEC(value)                 (GMAC_SCL_SEC_Msk & ((value) << GMAC_SCL_SEC_Pos))
#define GMAC_SCL_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_SCL) Register MASK  (Use GMAC_SCL_Msk instead)  */
#define GMAC_SCL_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_SCL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_SCH : (GMAC Offset: 0xe4) (R/W 32) 1588 Timer Second Comparison High Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SEC:16;                    /**< bit:  0..15  1588 Timer Second Comparison Value       */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SCH_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SCH_OFFSET                     (0xE4)                                        /**<  (GMAC_SCH) 1588 Timer Second Comparison High Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SCH_SEC_Pos                    0                                              /**< (GMAC_SCH) 1588 Timer Second Comparison Value Position */
#define GMAC_SCH_SEC_Msk                    (0xFFFFU << GMAC_SCH_SEC_Pos)                  /**< (GMAC_SCH) 1588 Timer Second Comparison Value Mask */
#define GMAC_SCH_SEC(value)                 (GMAC_SCH_SEC_Msk & ((value) << GMAC_SCH_SEC_Pos))
#define GMAC_SCH_MASK                       (0xFFFFU)                                      /**< \deprecated (GMAC_SCH) Register MASK  (Use GMAC_SCH_Msk instead)  */
#define GMAC_SCH_Msk                        (0xFFFFU)                                      /**< (GMAC_SCH) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_EFTSH : (GMAC Offset: 0xe8) (R/ 32) PTP Event Frame Transmitted Seconds High Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:16;                    /**< bit:  0..15  Register Update                          */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_EFTSH_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFTSH_OFFSET                   (0xE8)                                        /**<  (GMAC_EFTSH) PTP Event Frame Transmitted Seconds High Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_EFTSH_RUD_Pos                  0                                              /**< (GMAC_EFTSH) Register Update Position */
#define GMAC_EFTSH_RUD_Msk                  (0xFFFFU << GMAC_EFTSH_RUD_Pos)                /**< (GMAC_EFTSH) Register Update Mask */
#define GMAC_EFTSH_RUD(value)               (GMAC_EFTSH_RUD_Msk & ((value) << GMAC_EFTSH_RUD_Pos))
#define GMAC_EFTSH_MASK                     (0xFFFFU)                                      /**< \deprecated (GMAC_EFTSH) Register MASK  (Use GMAC_EFTSH_Msk instead)  */
#define GMAC_EFTSH_Msk                      (0xFFFFU)                                      /**< (GMAC_EFTSH) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_EFRSH : (GMAC Offset: 0xec) (R/ 32) PTP Event Frame Received Seconds High Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:16;                    /**< bit:  0..15  Register Update                          */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_EFRSH_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFRSH_OFFSET                   (0xEC)                                        /**<  (GMAC_EFRSH) PTP Event Frame Received Seconds High Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_EFRSH_RUD_Pos                  0                                              /**< (GMAC_EFRSH) Register Update Position */
#define GMAC_EFRSH_RUD_Msk                  (0xFFFFU << GMAC_EFRSH_RUD_Pos)                /**< (GMAC_EFRSH) Register Update Mask */
#define GMAC_EFRSH_RUD(value)               (GMAC_EFRSH_RUD_Msk & ((value) << GMAC_EFRSH_RUD_Pos))
#define GMAC_EFRSH_MASK                     (0xFFFFU)                                      /**< \deprecated (GMAC_EFRSH) Register MASK  (Use GMAC_EFRSH_Msk instead)  */
#define GMAC_EFRSH_Msk                      (0xFFFFU)                                      /**< (GMAC_EFRSH) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PEFTSH : (GMAC Offset: 0xf0) (R/ 32) PTP Peer Event Frame Transmitted Seconds High Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:16;                    /**< bit:  0..15  Register Update                          */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PEFTSH_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFTSH_OFFSET                  (0xF0)                                        /**<  (GMAC_PEFTSH) PTP Peer Event Frame Transmitted Seconds High Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PEFTSH_RUD_Pos                 0                                              /**< (GMAC_PEFTSH) Register Update Position */
#define GMAC_PEFTSH_RUD_Msk                 (0xFFFFU << GMAC_PEFTSH_RUD_Pos)               /**< (GMAC_PEFTSH) Register Update Mask */
#define GMAC_PEFTSH_RUD(value)              (GMAC_PEFTSH_RUD_Msk & ((value) << GMAC_PEFTSH_RUD_Pos))
#define GMAC_PEFTSH_MASK                    (0xFFFFU)                                      /**< \deprecated (GMAC_PEFTSH) Register MASK  (Use GMAC_PEFTSH_Msk instead)  */
#define GMAC_PEFTSH_Msk                     (0xFFFFU)                                      /**< (GMAC_PEFTSH) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PEFRSH : (GMAC Offset: 0xf4) (R/ 32) PTP Peer Event Frame Received Seconds High Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:16;                    /**< bit:  0..15  Register Update                          */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PEFRSH_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFRSH_OFFSET                  (0xF4)                                        /**<  (GMAC_PEFRSH) PTP Peer Event Frame Received Seconds High Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PEFRSH_RUD_Pos                 0                                              /**< (GMAC_PEFRSH) Register Update Position */
#define GMAC_PEFRSH_RUD_Msk                 (0xFFFFU << GMAC_PEFRSH_RUD_Pos)               /**< (GMAC_PEFRSH) Register Update Mask */
#define GMAC_PEFRSH_RUD(value)              (GMAC_PEFRSH_RUD_Msk & ((value) << GMAC_PEFRSH_RUD_Pos))
#define GMAC_PEFRSH_MASK                    (0xFFFFU)                                      /**< \deprecated (GMAC_PEFRSH) Register MASK  (Use GMAC_PEFRSH_Msk instead)  */
#define GMAC_PEFRSH_Msk                     (0xFFFFU)                                      /**< (GMAC_PEFRSH) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_OTLO : (GMAC Offset: 0x100) (R/ 32) Octets Transmitted Low Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXO:32;                    /**< bit:  0..31  Transmitted Octets                       */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_OTLO_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_OTLO_OFFSET                    (0x100)                                       /**<  (GMAC_OTLO) Octets Transmitted Low Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_OTLO_TXO_Pos                   0                                              /**< (GMAC_OTLO) Transmitted Octets Position */
#define GMAC_OTLO_TXO_Msk                   (0xFFFFFFFFU << GMAC_OTLO_TXO_Pos)             /**< (GMAC_OTLO) Transmitted Octets Mask */
#define GMAC_OTLO_TXO(value)                (GMAC_OTLO_TXO_Msk & ((value) << GMAC_OTLO_TXO_Pos))
#define GMAC_OTLO_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_OTLO) Register MASK  (Use GMAC_OTLO_Msk instead)  */
#define GMAC_OTLO_Msk                       (0xFFFFFFFFU)                                  /**< (GMAC_OTLO) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_OTHI : (GMAC Offset: 0x104) (R/ 32) Octets Transmitted High Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXO:16;                    /**< bit:  0..15  Transmitted Octets                       */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_OTHI_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_OTHI_OFFSET                    (0x104)                                       /**<  (GMAC_OTHI) Octets Transmitted High Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_OTHI_TXO_Pos                   0                                              /**< (GMAC_OTHI) Transmitted Octets Position */
#define GMAC_OTHI_TXO_Msk                   (0xFFFFU << GMAC_OTHI_TXO_Pos)                 /**< (GMAC_OTHI) Transmitted Octets Mask */
#define GMAC_OTHI_TXO(value)                (GMAC_OTHI_TXO_Msk & ((value) << GMAC_OTHI_TXO_Pos))
#define GMAC_OTHI_MASK                      (0xFFFFU)                                      /**< \deprecated (GMAC_OTHI) Register MASK  (Use GMAC_OTHI_Msk instead)  */
#define GMAC_OTHI_Msk                       (0xFFFFU)                                      /**< (GMAC_OTHI) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_FT : (GMAC Offset: 0x108) (R/ 32) Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FTX:32;                    /**< bit:  0..31  Frames Transmitted without Error         */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_FT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_FT_OFFSET                      (0x108)                                       /**<  (GMAC_FT) Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_FT_FTX_Pos                     0                                              /**< (GMAC_FT) Frames Transmitted without Error Position */
#define GMAC_FT_FTX_Msk                     (0xFFFFFFFFU << GMAC_FT_FTX_Pos)               /**< (GMAC_FT) Frames Transmitted without Error Mask */
#define GMAC_FT_FTX(value)                  (GMAC_FT_FTX_Msk & ((value) << GMAC_FT_FTX_Pos))
#define GMAC_FT_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_FT) Register MASK  (Use GMAC_FT_Msk instead)  */
#define GMAC_FT_Msk                         (0xFFFFFFFFU)                                  /**< (GMAC_FT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_BCFT : (GMAC Offset: 0x10c) (R/ 32) Broadcast Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BFTX:32;                   /**< bit:  0..31  Broadcast Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_BCFT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BCFT_OFFSET                    (0x10C)                                       /**<  (GMAC_BCFT) Broadcast Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_BCFT_BFTX_Pos                  0                                              /**< (GMAC_BCFT) Broadcast Frames Transmitted without Error Position */
#define GMAC_BCFT_BFTX_Msk                  (0xFFFFFFFFU << GMAC_BCFT_BFTX_Pos)            /**< (GMAC_BCFT) Broadcast Frames Transmitted without Error Mask */
#define GMAC_BCFT_BFTX(value)               (GMAC_BCFT_BFTX_Msk & ((value) << GMAC_BCFT_BFTX_Pos))
#define GMAC_BCFT_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_BCFT) Register MASK  (Use GMAC_BCFT_Msk instead)  */
#define GMAC_BCFT_Msk                       (0xFFFFFFFFU)                                  /**< (GMAC_BCFT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_MFT : (GMAC Offset: 0x110) (R/ 32) Multicast Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MFTX:32;                   /**< bit:  0..31  Multicast Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_MFT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MFT_OFFSET                     (0x110)                                       /**<  (GMAC_MFT) Multicast Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_MFT_MFTX_Pos                   0                                              /**< (GMAC_MFT) Multicast Frames Transmitted without Error Position */
#define GMAC_MFT_MFTX_Msk                   (0xFFFFFFFFU << GMAC_MFT_MFTX_Pos)             /**< (GMAC_MFT) Multicast Frames Transmitted without Error Mask */
#define GMAC_MFT_MFTX(value)                (GMAC_MFT_MFTX_Msk & ((value) << GMAC_MFT_MFTX_Pos))
#define GMAC_MFT_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_MFT) Register MASK  (Use GMAC_MFT_Msk instead)  */
#define GMAC_MFT_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_MFT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PFT : (GMAC Offset: 0x114) (R/ 32) Pause Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PFTX:16;                   /**< bit:  0..15  Pause Frames Transmitted Register        */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PFT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PFT_OFFSET                     (0x114)                                       /**<  (GMAC_PFT) Pause Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PFT_PFTX_Pos                   0                                              /**< (GMAC_PFT) Pause Frames Transmitted Register Position */
#define GMAC_PFT_PFTX_Msk                   (0xFFFFU << GMAC_PFT_PFTX_Pos)                 /**< (GMAC_PFT) Pause Frames Transmitted Register Mask */
#define GMAC_PFT_PFTX(value)                (GMAC_PFT_PFTX_Msk & ((value) << GMAC_PFT_PFTX_Pos))
#define GMAC_PFT_MASK                       (0xFFFFU)                                      /**< \deprecated (GMAC_PFT) Register MASK  (Use GMAC_PFT_Msk instead)  */
#define GMAC_PFT_Msk                        (0xFFFFU)                                      /**< (GMAC_PFT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_BFT64 : (GMAC Offset: 0x118) (R/ 32) 64 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFTX:32;                   /**< bit:  0..31  64 Byte Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_BFT64_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BFT64_OFFSET                   (0x118)                                       /**<  (GMAC_BFT64) 64 Byte Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_BFT64_NFTX_Pos                 0                                              /**< (GMAC_BFT64) 64 Byte Frames Transmitted without Error Position */
#define GMAC_BFT64_NFTX_Msk                 (0xFFFFFFFFU << GMAC_BFT64_NFTX_Pos)           /**< (GMAC_BFT64) 64 Byte Frames Transmitted without Error Mask */
#define GMAC_BFT64_NFTX(value)              (GMAC_BFT64_NFTX_Msk & ((value) << GMAC_BFT64_NFTX_Pos))
#define GMAC_BFT64_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_BFT64) Register MASK  (Use GMAC_BFT64_Msk instead)  */
#define GMAC_BFT64_Msk                      (0xFFFFFFFFU)                                  /**< (GMAC_BFT64) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFT127 : (GMAC Offset: 0x11c) (R/ 32) 65 to 127 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFTX:32;                   /**< bit:  0..31  65 to 127 Byte Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFT127_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT127_OFFSET                 (0x11C)                                       /**<  (GMAC_TBFT127) 65 to 127 Byte Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFT127_NFTX_Pos               0                                              /**< (GMAC_TBFT127) 65 to 127 Byte Frames Transmitted without Error Position */
#define GMAC_TBFT127_NFTX_Msk               (0xFFFFFFFFU << GMAC_TBFT127_NFTX_Pos)         /**< (GMAC_TBFT127) 65 to 127 Byte Frames Transmitted without Error Mask */
#define GMAC_TBFT127_NFTX(value)            (GMAC_TBFT127_NFTX_Msk & ((value) << GMAC_TBFT127_NFTX_Pos))
#define GMAC_TBFT127_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFT127) Register MASK  (Use GMAC_TBFT127_Msk instead)  */
#define GMAC_TBFT127_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_TBFT127) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFT255 : (GMAC Offset: 0x120) (R/ 32) 128 to 255 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFTX:32;                   /**< bit:  0..31  128 to 255 Byte Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFT255_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT255_OFFSET                 (0x120)                                       /**<  (GMAC_TBFT255) 128 to 255 Byte Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFT255_NFTX_Pos               0                                              /**< (GMAC_TBFT255) 128 to 255 Byte Frames Transmitted without Error Position */
#define GMAC_TBFT255_NFTX_Msk               (0xFFFFFFFFU << GMAC_TBFT255_NFTX_Pos)         /**< (GMAC_TBFT255) 128 to 255 Byte Frames Transmitted without Error Mask */
#define GMAC_TBFT255_NFTX(value)            (GMAC_TBFT255_NFTX_Msk & ((value) << GMAC_TBFT255_NFTX_Pos))
#define GMAC_TBFT255_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFT255) Register MASK  (Use GMAC_TBFT255_Msk instead)  */
#define GMAC_TBFT255_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_TBFT255) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFT511 : (GMAC Offset: 0x124) (R/ 32) 256 to 511 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFTX:32;                   /**< bit:  0..31  256 to 511 Byte Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFT511_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT511_OFFSET                 (0x124)                                       /**<  (GMAC_TBFT511) 256 to 511 Byte Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFT511_NFTX_Pos               0                                              /**< (GMAC_TBFT511) 256 to 511 Byte Frames Transmitted without Error Position */
#define GMAC_TBFT511_NFTX_Msk               (0xFFFFFFFFU << GMAC_TBFT511_NFTX_Pos)         /**< (GMAC_TBFT511) 256 to 511 Byte Frames Transmitted without Error Mask */
#define GMAC_TBFT511_NFTX(value)            (GMAC_TBFT511_NFTX_Msk & ((value) << GMAC_TBFT511_NFTX_Pos))
#define GMAC_TBFT511_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFT511) Register MASK  (Use GMAC_TBFT511_Msk instead)  */
#define GMAC_TBFT511_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_TBFT511) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFT1023 : (GMAC Offset: 0x128) (R/ 32) 512 to 1023 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFTX:32;                   /**< bit:  0..31  512 to 1023 Byte Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFT1023_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT1023_OFFSET                (0x128)                                       /**<  (GMAC_TBFT1023) 512 to 1023 Byte Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFT1023_NFTX_Pos              0                                              /**< (GMAC_TBFT1023) 512 to 1023 Byte Frames Transmitted without Error Position */
#define GMAC_TBFT1023_NFTX_Msk              (0xFFFFFFFFU << GMAC_TBFT1023_NFTX_Pos)        /**< (GMAC_TBFT1023) 512 to 1023 Byte Frames Transmitted without Error Mask */
#define GMAC_TBFT1023_NFTX(value)           (GMAC_TBFT1023_NFTX_Msk & ((value) << GMAC_TBFT1023_NFTX_Pos))
#define GMAC_TBFT1023_MASK                  (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFT1023) Register MASK  (Use GMAC_TBFT1023_Msk instead)  */
#define GMAC_TBFT1023_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_TBFT1023) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFT1518 : (GMAC Offset: 0x12c) (R/ 32) 1024 to 1518 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFTX:32;                   /**< bit:  0..31  1024 to 1518 Byte Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFT1518_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT1518_OFFSET                (0x12C)                                       /**<  (GMAC_TBFT1518) 1024 to 1518 Byte Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFT1518_NFTX_Pos              0                                              /**< (GMAC_TBFT1518) 1024 to 1518 Byte Frames Transmitted without Error Position */
#define GMAC_TBFT1518_NFTX_Msk              (0xFFFFFFFFU << GMAC_TBFT1518_NFTX_Pos)        /**< (GMAC_TBFT1518) 1024 to 1518 Byte Frames Transmitted without Error Mask */
#define GMAC_TBFT1518_NFTX(value)           (GMAC_TBFT1518_NFTX_Msk & ((value) << GMAC_TBFT1518_NFTX_Pos))
#define GMAC_TBFT1518_MASK                  (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFT1518) Register MASK  (Use GMAC_TBFT1518_Msk instead)  */
#define GMAC_TBFT1518_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_TBFT1518) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_GTBFT1518 : (GMAC Offset: 0x130) (R/ 32) Greater Than 1518 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFTX:32;                   /**< bit:  0..31  Greater than 1518 Byte Frames Transmitted without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_GTBFT1518_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_GTBFT1518_OFFSET               (0x130)                                       /**<  (GMAC_GTBFT1518) Greater Than 1518 Byte Frames Transmitted Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_GTBFT1518_NFTX_Pos             0                                              /**< (GMAC_GTBFT1518) Greater than 1518 Byte Frames Transmitted without Error Position */
#define GMAC_GTBFT1518_NFTX_Msk             (0xFFFFFFFFU << GMAC_GTBFT1518_NFTX_Pos)       /**< (GMAC_GTBFT1518) Greater than 1518 Byte Frames Transmitted without Error Mask */
#define GMAC_GTBFT1518_NFTX(value)          (GMAC_GTBFT1518_NFTX_Msk & ((value) << GMAC_GTBFT1518_NFTX_Pos))
#define GMAC_GTBFT1518_MASK                 (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_GTBFT1518) Register MASK  (Use GMAC_GTBFT1518_Msk instead)  */
#define GMAC_GTBFT1518_Msk                  (0xFFFFFFFFU)                                  /**< (GMAC_GTBFT1518) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TUR : (GMAC Offset: 0x134) (R/ 32) Transmit Underruns Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXUNR:10;                  /**< bit:   0..9  Transmit Underruns                       */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TUR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TUR_OFFSET                     (0x134)                                       /**<  (GMAC_TUR) Transmit Underruns Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TUR_TXUNR_Pos                  0                                              /**< (GMAC_TUR) Transmit Underruns Position */
#define GMAC_TUR_TXUNR_Msk                  (0x3FFU << GMAC_TUR_TXUNR_Pos)                 /**< (GMAC_TUR) Transmit Underruns Mask */
#define GMAC_TUR_TXUNR(value)               (GMAC_TUR_TXUNR_Msk & ((value) << GMAC_TUR_TXUNR_Pos))
#define GMAC_TUR_MASK                       (0x3FFU)                                       /**< \deprecated (GMAC_TUR) Register MASK  (Use GMAC_TUR_Msk instead)  */
#define GMAC_TUR_Msk                        (0x3FFU)                                       /**< (GMAC_TUR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_SCF : (GMAC Offset: 0x138) (R/ 32) Single Collision Frames Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SCOL:18;                   /**< bit:  0..17  Single Collision                         */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_SCF_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SCF_OFFSET                     (0x138)                                       /**<  (GMAC_SCF) Single Collision Frames Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_SCF_SCOL_Pos                   0                                              /**< (GMAC_SCF) Single Collision Position */
#define GMAC_SCF_SCOL_Msk                   (0x3FFFFU << GMAC_SCF_SCOL_Pos)                /**< (GMAC_SCF) Single Collision Mask */
#define GMAC_SCF_SCOL(value)                (GMAC_SCF_SCOL_Msk & ((value) << GMAC_SCF_SCOL_Pos))
#define GMAC_SCF_MASK                       (0x3FFFFU)                                     /**< \deprecated (GMAC_SCF) Register MASK  (Use GMAC_SCF_Msk instead)  */
#define GMAC_SCF_Msk                        (0x3FFFFU)                                     /**< (GMAC_SCF) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_MCF : (GMAC Offset: 0x13c) (R/ 32) Multiple Collision Frames Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MCOL:18;                   /**< bit:  0..17  Multiple Collision                       */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_MCF_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MCF_OFFSET                     (0x13C)                                       /**<  (GMAC_MCF) Multiple Collision Frames Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_MCF_MCOL_Pos                   0                                              /**< (GMAC_MCF) Multiple Collision Position */
#define GMAC_MCF_MCOL_Msk                   (0x3FFFFU << GMAC_MCF_MCOL_Pos)                /**< (GMAC_MCF) Multiple Collision Mask */
#define GMAC_MCF_MCOL(value)                (GMAC_MCF_MCOL_Msk & ((value) << GMAC_MCF_MCOL_Pos))
#define GMAC_MCF_MASK                       (0x3FFFFU)                                     /**< \deprecated (GMAC_MCF) Register MASK  (Use GMAC_MCF_Msk instead)  */
#define GMAC_MCF_Msk                        (0x3FFFFU)                                     /**< (GMAC_MCF) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_EC : (GMAC Offset: 0x140) (R/ 32) Excessive Collisions Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t XCOL:10;                   /**< bit:   0..9  Excessive Collisions                     */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_EC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EC_OFFSET                      (0x140)                                       /**<  (GMAC_EC) Excessive Collisions Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_EC_XCOL_Pos                    0                                              /**< (GMAC_EC) Excessive Collisions Position */
#define GMAC_EC_XCOL_Msk                    (0x3FFU << GMAC_EC_XCOL_Pos)                   /**< (GMAC_EC) Excessive Collisions Mask */
#define GMAC_EC_XCOL(value)                 (GMAC_EC_XCOL_Msk & ((value) << GMAC_EC_XCOL_Pos))
#define GMAC_EC_MASK                        (0x3FFU)                                       /**< \deprecated (GMAC_EC) Register MASK  (Use GMAC_EC_Msk instead)  */
#define GMAC_EC_Msk                         (0x3FFU)                                       /**< (GMAC_EC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_LC : (GMAC Offset: 0x144) (R/ 32) Late Collisions Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LCOL:10;                   /**< bit:   0..9  Late Collisions                          */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_LC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_LC_OFFSET                      (0x144)                                       /**<  (GMAC_LC) Late Collisions Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_LC_LCOL_Pos                    0                                              /**< (GMAC_LC) Late Collisions Position */
#define GMAC_LC_LCOL_Msk                    (0x3FFU << GMAC_LC_LCOL_Pos)                   /**< (GMAC_LC) Late Collisions Mask */
#define GMAC_LC_LCOL(value)                 (GMAC_LC_LCOL_Msk & ((value) << GMAC_LC_LCOL_Pos))
#define GMAC_LC_MASK                        (0x3FFU)                                       /**< \deprecated (GMAC_LC) Register MASK  (Use GMAC_LC_Msk instead)  */
#define GMAC_LC_Msk                         (0x3FFU)                                       /**< (GMAC_LC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_DTF : (GMAC Offset: 0x148) (R/ 32) Deferred Transmission Frames Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DEFT:18;                   /**< bit:  0..17  Deferred Transmission                    */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_DTF_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_DTF_OFFSET                     (0x148)                                       /**<  (GMAC_DTF) Deferred Transmission Frames Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_DTF_DEFT_Pos                   0                                              /**< (GMAC_DTF) Deferred Transmission Position */
#define GMAC_DTF_DEFT_Msk                   (0x3FFFFU << GMAC_DTF_DEFT_Pos)                /**< (GMAC_DTF) Deferred Transmission Mask */
#define GMAC_DTF_DEFT(value)                (GMAC_DTF_DEFT_Msk & ((value) << GMAC_DTF_DEFT_Pos))
#define GMAC_DTF_MASK                       (0x3FFFFU)                                     /**< \deprecated (GMAC_DTF) Register MASK  (Use GMAC_DTF_Msk instead)  */
#define GMAC_DTF_Msk                        (0x3FFFFU)                                     /**< (GMAC_DTF) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_CSE : (GMAC Offset: 0x14c) (R/ 32) Carrier Sense Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CSR:10;                    /**< bit:   0..9  Carrier Sense Error                      */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_CSE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_CSE_OFFSET                     (0x14C)                                       /**<  (GMAC_CSE) Carrier Sense Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_CSE_CSR_Pos                    0                                              /**< (GMAC_CSE) Carrier Sense Error Position */
#define GMAC_CSE_CSR_Msk                    (0x3FFU << GMAC_CSE_CSR_Pos)                   /**< (GMAC_CSE) Carrier Sense Error Mask */
#define GMAC_CSE_CSR(value)                 (GMAC_CSE_CSR_Msk & ((value) << GMAC_CSE_CSR_Pos))
#define GMAC_CSE_MASK                       (0x3FFU)                                       /**< \deprecated (GMAC_CSE) Register MASK  (Use GMAC_CSE_Msk instead)  */
#define GMAC_CSE_Msk                        (0x3FFU)                                       /**< (GMAC_CSE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ORLO : (GMAC Offset: 0x150) (R/ 32) Octets Received Low Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXO:32;                    /**< bit:  0..31  Received Octets                          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ORLO_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ORLO_OFFSET                    (0x150)                                       /**<  (GMAC_ORLO) Octets Received Low Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ORLO_RXO_Pos                   0                                              /**< (GMAC_ORLO) Received Octets Position */
#define GMAC_ORLO_RXO_Msk                   (0xFFFFFFFFU << GMAC_ORLO_RXO_Pos)             /**< (GMAC_ORLO) Received Octets Mask */
#define GMAC_ORLO_RXO(value)                (GMAC_ORLO_RXO_Msk & ((value) << GMAC_ORLO_RXO_Pos))
#define GMAC_ORLO_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_ORLO) Register MASK  (Use GMAC_ORLO_Msk instead)  */
#define GMAC_ORLO_Msk                       (0xFFFFFFFFU)                                  /**< (GMAC_ORLO) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ORHI : (GMAC Offset: 0x154) (R/ 32) Octets Received High Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXO:16;                    /**< bit:  0..15  Received Octets                          */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ORHI_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ORHI_OFFSET                    (0x154)                                       /**<  (GMAC_ORHI) Octets Received High Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ORHI_RXO_Pos                   0                                              /**< (GMAC_ORHI) Received Octets Position */
#define GMAC_ORHI_RXO_Msk                   (0xFFFFU << GMAC_ORHI_RXO_Pos)                 /**< (GMAC_ORHI) Received Octets Mask */
#define GMAC_ORHI_RXO(value)                (GMAC_ORHI_RXO_Msk & ((value) << GMAC_ORHI_RXO_Pos))
#define GMAC_ORHI_MASK                      (0xFFFFU)                                      /**< \deprecated (GMAC_ORHI) Register MASK  (Use GMAC_ORHI_Msk instead)  */
#define GMAC_ORHI_Msk                       (0xFFFFU)                                      /**< (GMAC_ORHI) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_FR : (GMAC Offset: 0x158) (R/ 32) Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FRX:32;                    /**< bit:  0..31  Frames Received without Error            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_FR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_FR_OFFSET                      (0x158)                                       /**<  (GMAC_FR) Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_FR_FRX_Pos                     0                                              /**< (GMAC_FR) Frames Received without Error Position */
#define GMAC_FR_FRX_Msk                     (0xFFFFFFFFU << GMAC_FR_FRX_Pos)               /**< (GMAC_FR) Frames Received without Error Mask */
#define GMAC_FR_FRX(value)                  (GMAC_FR_FRX_Msk & ((value) << GMAC_FR_FRX_Pos))
#define GMAC_FR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_FR) Register MASK  (Use GMAC_FR_Msk instead)  */
#define GMAC_FR_Msk                         (0xFFFFFFFFU)                                  /**< (GMAC_FR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_BCFR : (GMAC Offset: 0x15c) (R/ 32) Broadcast Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BFRX:32;                   /**< bit:  0..31  Broadcast Frames Received without Error  */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_BCFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BCFR_OFFSET                    (0x15C)                                       /**<  (GMAC_BCFR) Broadcast Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_BCFR_BFRX_Pos                  0                                              /**< (GMAC_BCFR) Broadcast Frames Received without Error Position */
#define GMAC_BCFR_BFRX_Msk                  (0xFFFFFFFFU << GMAC_BCFR_BFRX_Pos)            /**< (GMAC_BCFR) Broadcast Frames Received without Error Mask */
#define GMAC_BCFR_BFRX(value)               (GMAC_BCFR_BFRX_Msk & ((value) << GMAC_BCFR_BFRX_Pos))
#define GMAC_BCFR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_BCFR) Register MASK  (Use GMAC_BCFR_Msk instead)  */
#define GMAC_BCFR_Msk                       (0xFFFFFFFFU)                                  /**< (GMAC_BCFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_MFR : (GMAC Offset: 0x160) (R/ 32) Multicast Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MFRX:32;                   /**< bit:  0..31  Multicast Frames Received without Error  */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_MFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MFR_OFFSET                     (0x160)                                       /**<  (GMAC_MFR) Multicast Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_MFR_MFRX_Pos                   0                                              /**< (GMAC_MFR) Multicast Frames Received without Error Position */
#define GMAC_MFR_MFRX_Msk                   (0xFFFFFFFFU << GMAC_MFR_MFRX_Pos)             /**< (GMAC_MFR) Multicast Frames Received without Error Mask */
#define GMAC_MFR_MFRX(value)                (GMAC_MFR_MFRX_Msk & ((value) << GMAC_MFR_MFRX_Pos))
#define GMAC_MFR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_MFR) Register MASK  (Use GMAC_MFR_Msk instead)  */
#define GMAC_MFR_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_MFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PFR : (GMAC Offset: 0x164) (R/ 32) Pause Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PFRX:16;                   /**< bit:  0..15  Pause Frames Received Register           */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PFR_OFFSET                     (0x164)                                       /**<  (GMAC_PFR) Pause Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PFR_PFRX_Pos                   0                                              /**< (GMAC_PFR) Pause Frames Received Register Position */
#define GMAC_PFR_PFRX_Msk                   (0xFFFFU << GMAC_PFR_PFRX_Pos)                 /**< (GMAC_PFR) Pause Frames Received Register Mask */
#define GMAC_PFR_PFRX(value)                (GMAC_PFR_PFRX_Msk & ((value) << GMAC_PFR_PFRX_Pos))
#define GMAC_PFR_MASK                       (0xFFFFU)                                      /**< \deprecated (GMAC_PFR) Register MASK  (Use GMAC_PFR_Msk instead)  */
#define GMAC_PFR_Msk                        (0xFFFFU)                                      /**< (GMAC_PFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_BFR64 : (GMAC Offset: 0x168) (R/ 32) 64 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFRX:32;                   /**< bit:  0..31  64 Byte Frames Received without Error    */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_BFR64_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BFR64_OFFSET                   (0x168)                                       /**<  (GMAC_BFR64) 64 Byte Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_BFR64_NFRX_Pos                 0                                              /**< (GMAC_BFR64) 64 Byte Frames Received without Error Position */
#define GMAC_BFR64_NFRX_Msk                 (0xFFFFFFFFU << GMAC_BFR64_NFRX_Pos)           /**< (GMAC_BFR64) 64 Byte Frames Received without Error Mask */
#define GMAC_BFR64_NFRX(value)              (GMAC_BFR64_NFRX_Msk & ((value) << GMAC_BFR64_NFRX_Pos))
#define GMAC_BFR64_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_BFR64) Register MASK  (Use GMAC_BFR64_Msk instead)  */
#define GMAC_BFR64_Msk                      (0xFFFFFFFFU)                                  /**< (GMAC_BFR64) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFR127 : (GMAC Offset: 0x16c) (R/ 32) 65 to 127 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFRX:32;                   /**< bit:  0..31  65 to 127 Byte Frames Received without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFR127_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR127_OFFSET                 (0x16C)                                       /**<  (GMAC_TBFR127) 65 to 127 Byte Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFR127_NFRX_Pos               0                                              /**< (GMAC_TBFR127) 65 to 127 Byte Frames Received without Error Position */
#define GMAC_TBFR127_NFRX_Msk               (0xFFFFFFFFU << GMAC_TBFR127_NFRX_Pos)         /**< (GMAC_TBFR127) 65 to 127 Byte Frames Received without Error Mask */
#define GMAC_TBFR127_NFRX(value)            (GMAC_TBFR127_NFRX_Msk & ((value) << GMAC_TBFR127_NFRX_Pos))
#define GMAC_TBFR127_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFR127) Register MASK  (Use GMAC_TBFR127_Msk instead)  */
#define GMAC_TBFR127_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_TBFR127) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFR255 : (GMAC Offset: 0x170) (R/ 32) 128 to 255 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFRX:32;                   /**< bit:  0..31  128 to 255 Byte Frames Received without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFR255_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR255_OFFSET                 (0x170)                                       /**<  (GMAC_TBFR255) 128 to 255 Byte Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFR255_NFRX_Pos               0                                              /**< (GMAC_TBFR255) 128 to 255 Byte Frames Received without Error Position */
#define GMAC_TBFR255_NFRX_Msk               (0xFFFFFFFFU << GMAC_TBFR255_NFRX_Pos)         /**< (GMAC_TBFR255) 128 to 255 Byte Frames Received without Error Mask */
#define GMAC_TBFR255_NFRX(value)            (GMAC_TBFR255_NFRX_Msk & ((value) << GMAC_TBFR255_NFRX_Pos))
#define GMAC_TBFR255_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFR255) Register MASK  (Use GMAC_TBFR255_Msk instead)  */
#define GMAC_TBFR255_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_TBFR255) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFR511 : (GMAC Offset: 0x174) (R/ 32) 256 to 511 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFRX:32;                   /**< bit:  0..31  256 to 511 Byte Frames Received without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFR511_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR511_OFFSET                 (0x174)                                       /**<  (GMAC_TBFR511) 256 to 511 Byte Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFR511_NFRX_Pos               0                                              /**< (GMAC_TBFR511) 256 to 511 Byte Frames Received without Error Position */
#define GMAC_TBFR511_NFRX_Msk               (0xFFFFFFFFU << GMAC_TBFR511_NFRX_Pos)         /**< (GMAC_TBFR511) 256 to 511 Byte Frames Received without Error Mask */
#define GMAC_TBFR511_NFRX(value)            (GMAC_TBFR511_NFRX_Msk & ((value) << GMAC_TBFR511_NFRX_Pos))
#define GMAC_TBFR511_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFR511) Register MASK  (Use GMAC_TBFR511_Msk instead)  */
#define GMAC_TBFR511_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_TBFR511) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFR1023 : (GMAC Offset: 0x178) (R/ 32) 512 to 1023 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFRX:32;                   /**< bit:  0..31  512 to 1023 Byte Frames Received without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFR1023_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR1023_OFFSET                (0x178)                                       /**<  (GMAC_TBFR1023) 512 to 1023 Byte Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFR1023_NFRX_Pos              0                                              /**< (GMAC_TBFR1023) 512 to 1023 Byte Frames Received without Error Position */
#define GMAC_TBFR1023_NFRX_Msk              (0xFFFFFFFFU << GMAC_TBFR1023_NFRX_Pos)        /**< (GMAC_TBFR1023) 512 to 1023 Byte Frames Received without Error Mask */
#define GMAC_TBFR1023_NFRX(value)           (GMAC_TBFR1023_NFRX_Msk & ((value) << GMAC_TBFR1023_NFRX_Pos))
#define GMAC_TBFR1023_MASK                  (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFR1023) Register MASK  (Use GMAC_TBFR1023_Msk instead)  */
#define GMAC_TBFR1023_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_TBFR1023) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBFR1518 : (GMAC Offset: 0x17c) (R/ 32) 1024 to 1518 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFRX:32;                   /**< bit:  0..31  1024 to 1518 Byte Frames Received without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBFR1518_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR1518_OFFSET                (0x17C)                                       /**<  (GMAC_TBFR1518) 1024 to 1518 Byte Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBFR1518_NFRX_Pos              0                                              /**< (GMAC_TBFR1518) 1024 to 1518 Byte Frames Received without Error Position */
#define GMAC_TBFR1518_NFRX_Msk              (0xFFFFFFFFU << GMAC_TBFR1518_NFRX_Pos)        /**< (GMAC_TBFR1518) 1024 to 1518 Byte Frames Received without Error Mask */
#define GMAC_TBFR1518_NFRX(value)           (GMAC_TBFR1518_NFRX_Msk & ((value) << GMAC_TBFR1518_NFRX_Pos))
#define GMAC_TBFR1518_MASK                  (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TBFR1518) Register MASK  (Use GMAC_TBFR1518_Msk instead)  */
#define GMAC_TBFR1518_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_TBFR1518) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TMXBFR : (GMAC Offset: 0x180) (R/ 32) 1519 to Maximum Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NFRX:32;                   /**< bit:  0..31  1519 to Maximum Byte Frames Received without Error */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TMXBFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TMXBFR_OFFSET                  (0x180)                                       /**<  (GMAC_TMXBFR) 1519 to Maximum Byte Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TMXBFR_NFRX_Pos                0                                              /**< (GMAC_TMXBFR) 1519 to Maximum Byte Frames Received without Error Position */
#define GMAC_TMXBFR_NFRX_Msk                (0xFFFFFFFFU << GMAC_TMXBFR_NFRX_Pos)          /**< (GMAC_TMXBFR) 1519 to Maximum Byte Frames Received without Error Mask */
#define GMAC_TMXBFR_NFRX(value)             (GMAC_TMXBFR_NFRX_Msk & ((value) << GMAC_TMXBFR_NFRX_Pos))
#define GMAC_TMXBFR_MASK                    (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TMXBFR) Register MASK  (Use GMAC_TMXBFR_Msk instead)  */
#define GMAC_TMXBFR_Msk                     (0xFFFFFFFFU)                                  /**< (GMAC_TMXBFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_UFR : (GMAC Offset: 0x184) (R/ 32) Undersize Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UFRX:10;                   /**< bit:   0..9  Undersize Frames Received                */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_UFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_UFR_OFFSET                     (0x184)                                       /**<  (GMAC_UFR) Undersize Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_UFR_UFRX_Pos                   0                                              /**< (GMAC_UFR) Undersize Frames Received Position */
#define GMAC_UFR_UFRX_Msk                   (0x3FFU << GMAC_UFR_UFRX_Pos)                  /**< (GMAC_UFR) Undersize Frames Received Mask */
#define GMAC_UFR_UFRX(value)                (GMAC_UFR_UFRX_Msk & ((value) << GMAC_UFR_UFRX_Pos))
#define GMAC_UFR_MASK                       (0x3FFU)                                       /**< \deprecated (GMAC_UFR) Register MASK  (Use GMAC_UFR_Msk instead)  */
#define GMAC_UFR_Msk                        (0x3FFU)                                       /**< (GMAC_UFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_OFR : (GMAC Offset: 0x188) (R/ 32) Oversize Frames Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFRX:10;                   /**< bit:   0..9  Oversized Frames Received                */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_OFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_OFR_OFFSET                     (0x188)                                       /**<  (GMAC_OFR) Oversize Frames Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_OFR_OFRX_Pos                   0                                              /**< (GMAC_OFR) Oversized Frames Received Position */
#define GMAC_OFR_OFRX_Msk                   (0x3FFU << GMAC_OFR_OFRX_Pos)                  /**< (GMAC_OFR) Oversized Frames Received Mask */
#define GMAC_OFR_OFRX(value)                (GMAC_OFR_OFRX_Msk & ((value) << GMAC_OFR_OFRX_Pos))
#define GMAC_OFR_MASK                       (0x3FFU)                                       /**< \deprecated (GMAC_OFR) Register MASK  (Use GMAC_OFR_Msk instead)  */
#define GMAC_OFR_Msk                        (0x3FFU)                                       /**< (GMAC_OFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_JR : (GMAC Offset: 0x18c) (R/ 32) Jabbers Received Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t JRX:10;                    /**< bit:   0..9  Jabbers Received                         */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_JR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_JR_OFFSET                      (0x18C)                                       /**<  (GMAC_JR) Jabbers Received Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_JR_JRX_Pos                     0                                              /**< (GMAC_JR) Jabbers Received Position */
#define GMAC_JR_JRX_Msk                     (0x3FFU << GMAC_JR_JRX_Pos)                    /**< (GMAC_JR) Jabbers Received Mask */
#define GMAC_JR_JRX(value)                  (GMAC_JR_JRX_Msk & ((value) << GMAC_JR_JRX_Pos))
#define GMAC_JR_MASK                        (0x3FFU)                                       /**< \deprecated (GMAC_JR) Register MASK  (Use GMAC_JR_Msk instead)  */
#define GMAC_JR_Msk                         (0x3FFU)                                       /**< (GMAC_JR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_FCSE : (GMAC Offset: 0x190) (R/ 32) Frame Check Sequence Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FCKR:10;                   /**< bit:   0..9  Frame Check Sequence Errors              */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_FCSE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_FCSE_OFFSET                    (0x190)                                       /**<  (GMAC_FCSE) Frame Check Sequence Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_FCSE_FCKR_Pos                  0                                              /**< (GMAC_FCSE) Frame Check Sequence Errors Position */
#define GMAC_FCSE_FCKR_Msk                  (0x3FFU << GMAC_FCSE_FCKR_Pos)                 /**< (GMAC_FCSE) Frame Check Sequence Errors Mask */
#define GMAC_FCSE_FCKR(value)               (GMAC_FCSE_FCKR_Msk & ((value) << GMAC_FCSE_FCKR_Pos))
#define GMAC_FCSE_MASK                      (0x3FFU)                                       /**< \deprecated (GMAC_FCSE) Register MASK  (Use GMAC_FCSE_Msk instead)  */
#define GMAC_FCSE_Msk                       (0x3FFU)                                       /**< (GMAC_FCSE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_LFFE : (GMAC Offset: 0x194) (R/ 32) Length Field Frame Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LFER:10;                   /**< bit:   0..9  Length Field Frame Errors                */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_LFFE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_LFFE_OFFSET                    (0x194)                                       /**<  (GMAC_LFFE) Length Field Frame Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_LFFE_LFER_Pos                  0                                              /**< (GMAC_LFFE) Length Field Frame Errors Position */
#define GMAC_LFFE_LFER_Msk                  (0x3FFU << GMAC_LFFE_LFER_Pos)                 /**< (GMAC_LFFE) Length Field Frame Errors Mask */
#define GMAC_LFFE_LFER(value)               (GMAC_LFFE_LFER_Msk & ((value) << GMAC_LFFE_LFER_Pos))
#define GMAC_LFFE_MASK                      (0x3FFU)                                       /**< \deprecated (GMAC_LFFE) Register MASK  (Use GMAC_LFFE_Msk instead)  */
#define GMAC_LFFE_Msk                       (0x3FFU)                                       /**< (GMAC_LFFE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RSE : (GMAC Offset: 0x198) (R/ 32) Receive Symbol Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXSE:10;                   /**< bit:   0..9  Receive Symbol Errors                    */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RSE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RSE_OFFSET                     (0x198)                                       /**<  (GMAC_RSE) Receive Symbol Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RSE_RXSE_Pos                   0                                              /**< (GMAC_RSE) Receive Symbol Errors Position */
#define GMAC_RSE_RXSE_Msk                   (0x3FFU << GMAC_RSE_RXSE_Pos)                  /**< (GMAC_RSE) Receive Symbol Errors Mask */
#define GMAC_RSE_RXSE(value)                (GMAC_RSE_RXSE_Msk & ((value) << GMAC_RSE_RXSE_Pos))
#define GMAC_RSE_MASK                       (0x3FFU)                                       /**< \deprecated (GMAC_RSE) Register MASK  (Use GMAC_RSE_Msk instead)  */
#define GMAC_RSE_Msk                        (0x3FFU)                                       /**< (GMAC_RSE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_AE : (GMAC Offset: 0x19c) (R/ 32) Alignment Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t AER:10;                    /**< bit:   0..9  Alignment Errors                         */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_AE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_AE_OFFSET                      (0x19C)                                       /**<  (GMAC_AE) Alignment Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_AE_AER_Pos                     0                                              /**< (GMAC_AE) Alignment Errors Position */
#define GMAC_AE_AER_Msk                     (0x3FFU << GMAC_AE_AER_Pos)                    /**< (GMAC_AE) Alignment Errors Mask */
#define GMAC_AE_AER(value)                  (GMAC_AE_AER_Msk & ((value) << GMAC_AE_AER_Pos))
#define GMAC_AE_MASK                        (0x3FFU)                                       /**< \deprecated (GMAC_AE) Register MASK  (Use GMAC_AE_Msk instead)  */
#define GMAC_AE_Msk                         (0x3FFU)                                       /**< (GMAC_AE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RRE : (GMAC Offset: 0x1a0) (R/ 32) Receive Resource Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXRER:18;                  /**< bit:  0..17  Receive Resource Errors                  */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RRE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RRE_OFFSET                     (0x1A0)                                       /**<  (GMAC_RRE) Receive Resource Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RRE_RXRER_Pos                  0                                              /**< (GMAC_RRE) Receive Resource Errors Position */
#define GMAC_RRE_RXRER_Msk                  (0x3FFFFU << GMAC_RRE_RXRER_Pos)               /**< (GMAC_RRE) Receive Resource Errors Mask */
#define GMAC_RRE_RXRER(value)               (GMAC_RRE_RXRER_Msk & ((value) << GMAC_RRE_RXRER_Pos))
#define GMAC_RRE_MASK                       (0x3FFFFU)                                     /**< \deprecated (GMAC_RRE) Register MASK  (Use GMAC_RRE_Msk instead)  */
#define GMAC_RRE_Msk                        (0x3FFFFU)                                     /**< (GMAC_RRE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ROE : (GMAC Offset: 0x1a4) (R/ 32) Receive Overrun Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXOVR:10;                  /**< bit:   0..9  Receive Overruns                         */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ROE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ROE_OFFSET                     (0x1A4)                                       /**<  (GMAC_ROE) Receive Overrun Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ROE_RXOVR_Pos                  0                                              /**< (GMAC_ROE) Receive Overruns Position */
#define GMAC_ROE_RXOVR_Msk                  (0x3FFU << GMAC_ROE_RXOVR_Pos)                 /**< (GMAC_ROE) Receive Overruns Mask */
#define GMAC_ROE_RXOVR(value)               (GMAC_ROE_RXOVR_Msk & ((value) << GMAC_ROE_RXOVR_Pos))
#define GMAC_ROE_MASK                       (0x3FFU)                                       /**< \deprecated (GMAC_ROE) Register MASK  (Use GMAC_ROE_Msk instead)  */
#define GMAC_ROE_Msk                        (0x3FFU)                                       /**< (GMAC_ROE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IHCE : (GMAC Offset: 0x1a8) (R/ 32) IP Header Checksum Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t HCKER:8;                   /**< bit:   0..7  IP Header Checksum Errors                */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IHCE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IHCE_OFFSET                    (0x1A8)                                       /**<  (GMAC_IHCE) IP Header Checksum Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IHCE_HCKER_Pos                 0                                              /**< (GMAC_IHCE) IP Header Checksum Errors Position */
#define GMAC_IHCE_HCKER_Msk                 (0xFFU << GMAC_IHCE_HCKER_Pos)                 /**< (GMAC_IHCE) IP Header Checksum Errors Mask */
#define GMAC_IHCE_HCKER(value)              (GMAC_IHCE_HCKER_Msk & ((value) << GMAC_IHCE_HCKER_Pos))
#define GMAC_IHCE_MASK                      (0xFFU)                                        /**< \deprecated (GMAC_IHCE) Register MASK  (Use GMAC_IHCE_Msk instead)  */
#define GMAC_IHCE_Msk                       (0xFFU)                                        /**< (GMAC_IHCE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TCE : (GMAC Offset: 0x1ac) (R/ 32) TCP Checksum Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TCKER:8;                   /**< bit:   0..7  TCP Checksum Errors                      */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TCE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TCE_OFFSET                     (0x1AC)                                       /**<  (GMAC_TCE) TCP Checksum Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TCE_TCKER_Pos                  0                                              /**< (GMAC_TCE) TCP Checksum Errors Position */
#define GMAC_TCE_TCKER_Msk                  (0xFFU << GMAC_TCE_TCKER_Pos)                  /**< (GMAC_TCE) TCP Checksum Errors Mask */
#define GMAC_TCE_TCKER(value)               (GMAC_TCE_TCKER_Msk & ((value) << GMAC_TCE_TCKER_Pos))
#define GMAC_TCE_MASK                       (0xFFU)                                        /**< \deprecated (GMAC_TCE) Register MASK  (Use GMAC_TCE_Msk instead)  */
#define GMAC_TCE_Msk                        (0xFFU)                                        /**< (GMAC_TCE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_UCE : (GMAC Offset: 0x1b0) (R/ 32) UDP Checksum Errors Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UCKER:8;                   /**< bit:   0..7  UDP Checksum Errors                      */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_UCE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_UCE_OFFSET                     (0x1B0)                                       /**<  (GMAC_UCE) UDP Checksum Errors Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_UCE_UCKER_Pos                  0                                              /**< (GMAC_UCE) UDP Checksum Errors Position */
#define GMAC_UCE_UCKER_Msk                  (0xFFU << GMAC_UCE_UCKER_Pos)                  /**< (GMAC_UCE) UDP Checksum Errors Mask */
#define GMAC_UCE_UCKER(value)               (GMAC_UCE_UCKER_Msk & ((value) << GMAC_UCE_UCKER_Pos))
#define GMAC_UCE_MASK                       (0xFFU)                                        /**< \deprecated (GMAC_UCE) Register MASK  (Use GMAC_UCE_Msk instead)  */
#define GMAC_UCE_Msk                        (0xFFU)                                        /**< (GMAC_UCE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TISUBN : (GMAC Offset: 0x1bc) (R/W 32) 1588 Timer Increment Sub-nanoseconds Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LSBTIR:16;                 /**< bit:  0..15  Lower Significant Bits of Timer Increment Register */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TISUBN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TISUBN_OFFSET                  (0x1BC)                                       /**<  (GMAC_TISUBN) 1588 Timer Increment Sub-nanoseconds Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TISUBN_LSBTIR_Pos              0                                              /**< (GMAC_TISUBN) Lower Significant Bits of Timer Increment Register Position */
#define GMAC_TISUBN_LSBTIR_Msk              (0xFFFFU << GMAC_TISUBN_LSBTIR_Pos)            /**< (GMAC_TISUBN) Lower Significant Bits of Timer Increment Register Mask */
#define GMAC_TISUBN_LSBTIR(value)           (GMAC_TISUBN_LSBTIR_Msk & ((value) << GMAC_TISUBN_LSBTIR_Pos))
#define GMAC_TISUBN_MASK                    (0xFFFFU)                                      /**< \deprecated (GMAC_TISUBN) Register MASK  (Use GMAC_TISUBN_Msk instead)  */
#define GMAC_TISUBN_Msk                     (0xFFFFU)                                      /**< (GMAC_TISUBN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TSH : (GMAC Offset: 0x1c0) (R/W 32) 1588 Timer Seconds High Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TCS:16;                    /**< bit:  0..15  Timer Count in Seconds                   */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TSH_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSH_OFFSET                     (0x1C0)                                       /**<  (GMAC_TSH) 1588 Timer Seconds High Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TSH_TCS_Pos                    0                                              /**< (GMAC_TSH) Timer Count in Seconds Position */
#define GMAC_TSH_TCS_Msk                    (0xFFFFU << GMAC_TSH_TCS_Pos)                  /**< (GMAC_TSH) Timer Count in Seconds Mask */
#define GMAC_TSH_TCS(value)                 (GMAC_TSH_TCS_Msk & ((value) << GMAC_TSH_TCS_Pos))
#define GMAC_TSH_MASK                       (0xFFFFU)                                      /**< \deprecated (GMAC_TSH) Register MASK  (Use GMAC_TSH_Msk instead)  */
#define GMAC_TSH_Msk                        (0xFFFFU)                                      /**< (GMAC_TSH) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TSL : (GMAC Offset: 0x1d0) (R/W 32) 1588 Timer Seconds Low Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TCS:32;                    /**< bit:  0..31  Timer Count in Seconds                   */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TSL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSL_OFFSET                     (0x1D0)                                       /**<  (GMAC_TSL) 1588 Timer Seconds Low Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TSL_TCS_Pos                    0                                              /**< (GMAC_TSL) Timer Count in Seconds Position */
#define GMAC_TSL_TCS_Msk                    (0xFFFFFFFFU << GMAC_TSL_TCS_Pos)              /**< (GMAC_TSL) Timer Count in Seconds Mask */
#define GMAC_TSL_TCS(value)                 (GMAC_TSL_TCS_Msk & ((value) << GMAC_TSL_TCS_Pos))
#define GMAC_TSL_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_TSL) Register MASK  (Use GMAC_TSL_Msk instead)  */
#define GMAC_TSL_Msk                        (0xFFFFFFFFU)                                  /**< (GMAC_TSL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TN : (GMAC Offset: 0x1d4) (R/W 32) 1588 Timer Nanoseconds Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TNS:30;                    /**< bit:  0..29  Timer Count in Nanoseconds               */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TN_OFFSET                      (0x1D4)                                       /**<  (GMAC_TN) 1588 Timer Nanoseconds Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TN_TNS_Pos                     0                                              /**< (GMAC_TN) Timer Count in Nanoseconds Position */
#define GMAC_TN_TNS_Msk                     (0x3FFFFFFFU << GMAC_TN_TNS_Pos)               /**< (GMAC_TN) Timer Count in Nanoseconds Mask */
#define GMAC_TN_TNS(value)                  (GMAC_TN_TNS_Msk & ((value) << GMAC_TN_TNS_Pos))
#define GMAC_TN_MASK                        (0x3FFFFFFFU)                                  /**< \deprecated (GMAC_TN) Register MASK  (Use GMAC_TN_Msk instead)  */
#define GMAC_TN_Msk                         (0x3FFFFFFFU)                                  /**< (GMAC_TN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TA : (GMAC Offset: 0x1d8) (/W 32) 1588 Timer Adjust Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ITDT:30;                   /**< bit:  0..29  Increment/Decrement                      */
    uint32_t :1;                        /**< bit:     30  Reserved */
    uint32_t ADJ:1;                     /**< bit:     31  Adjust 1588 Timer                        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TA_OFFSET                      (0x1D8)                                       /**<  (GMAC_TA) 1588 Timer Adjust Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TA_ITDT_Pos                    0                                              /**< (GMAC_TA) Increment/Decrement Position */
#define GMAC_TA_ITDT_Msk                    (0x3FFFFFFFU << GMAC_TA_ITDT_Pos)              /**< (GMAC_TA) Increment/Decrement Mask */
#define GMAC_TA_ITDT(value)                 (GMAC_TA_ITDT_Msk & ((value) << GMAC_TA_ITDT_Pos))
#define GMAC_TA_ADJ_Pos                     31                                             /**< (GMAC_TA) Adjust 1588 Timer Position */
#define GMAC_TA_ADJ_Msk                     (0x1U << GMAC_TA_ADJ_Pos)                      /**< (GMAC_TA) Adjust 1588 Timer Mask */
#define GMAC_TA_ADJ                         GMAC_TA_ADJ_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_TA_ADJ_Msk instead */
#define GMAC_TA_MASK                        (0xBFFFFFFFU)                                  /**< \deprecated (GMAC_TA) Register MASK  (Use GMAC_TA_Msk instead)  */
#define GMAC_TA_Msk                         (0xBFFFFFFFU)                                  /**< (GMAC_TA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TI : (GMAC Offset: 0x1dc) (R/W 32) 1588 Timer Increment Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CNS:8;                     /**< bit:   0..7  Count Nanoseconds                        */
    uint32_t ACNS:8;                    /**< bit:  8..15  Alternative Count Nanoseconds            */
    uint32_t NIT:8;                     /**< bit: 16..23  Number of Increments                     */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TI_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TI_OFFSET                      (0x1DC)                                       /**<  (GMAC_TI) 1588 Timer Increment Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TI_CNS_Pos                     0                                              /**< (GMAC_TI) Count Nanoseconds Position */
#define GMAC_TI_CNS_Msk                     (0xFFU << GMAC_TI_CNS_Pos)                     /**< (GMAC_TI) Count Nanoseconds Mask */
#define GMAC_TI_CNS(value)                  (GMAC_TI_CNS_Msk & ((value) << GMAC_TI_CNS_Pos))
#define GMAC_TI_ACNS_Pos                    8                                              /**< (GMAC_TI) Alternative Count Nanoseconds Position */
#define GMAC_TI_ACNS_Msk                    (0xFFU << GMAC_TI_ACNS_Pos)                    /**< (GMAC_TI) Alternative Count Nanoseconds Mask */
#define GMAC_TI_ACNS(value)                 (GMAC_TI_ACNS_Msk & ((value) << GMAC_TI_ACNS_Pos))
#define GMAC_TI_NIT_Pos                     16                                             /**< (GMAC_TI) Number of Increments Position */
#define GMAC_TI_NIT_Msk                     (0xFFU << GMAC_TI_NIT_Pos)                     /**< (GMAC_TI) Number of Increments Mask */
#define GMAC_TI_NIT(value)                  (GMAC_TI_NIT_Msk & ((value) << GMAC_TI_NIT_Pos))
#define GMAC_TI_MASK                        (0xFFFFFFU)                                    /**< \deprecated (GMAC_TI) Register MASK  (Use GMAC_TI_Msk instead)  */
#define GMAC_TI_Msk                         (0xFFFFFFU)                                    /**< (GMAC_TI) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_EFTSL : (GMAC Offset: 0x1e0) (R/ 32) PTP Event Frame Transmitted Seconds Low Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:32;                    /**< bit:  0..31  Register Update                          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_EFTSL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFTSL_OFFSET                   (0x1E0)                                       /**<  (GMAC_EFTSL) PTP Event Frame Transmitted Seconds Low Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_EFTSL_RUD_Pos                  0                                              /**< (GMAC_EFTSL) Register Update Position */
#define GMAC_EFTSL_RUD_Msk                  (0xFFFFFFFFU << GMAC_EFTSL_RUD_Pos)            /**< (GMAC_EFTSL) Register Update Mask */
#define GMAC_EFTSL_RUD(value)               (GMAC_EFTSL_RUD_Msk & ((value) << GMAC_EFTSL_RUD_Pos))
#define GMAC_EFTSL_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_EFTSL) Register MASK  (Use GMAC_EFTSL_Msk instead)  */
#define GMAC_EFTSL_Msk                      (0xFFFFFFFFU)                                  /**< (GMAC_EFTSL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_EFTN : (GMAC Offset: 0x1e4) (R/ 32) PTP Event Frame Transmitted Nanoseconds Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:30;                    /**< bit:  0..29  Register Update                          */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_EFTN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFTN_OFFSET                    (0x1E4)                                       /**<  (GMAC_EFTN) PTP Event Frame Transmitted Nanoseconds Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_EFTN_RUD_Pos                   0                                              /**< (GMAC_EFTN) Register Update Position */
#define GMAC_EFTN_RUD_Msk                   (0x3FFFFFFFU << GMAC_EFTN_RUD_Pos)             /**< (GMAC_EFTN) Register Update Mask */
#define GMAC_EFTN_RUD(value)                (GMAC_EFTN_RUD_Msk & ((value) << GMAC_EFTN_RUD_Pos))
#define GMAC_EFTN_MASK                      (0x3FFFFFFFU)                                  /**< \deprecated (GMAC_EFTN) Register MASK  (Use GMAC_EFTN_Msk instead)  */
#define GMAC_EFTN_Msk                       (0x3FFFFFFFU)                                  /**< (GMAC_EFTN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_EFRSL : (GMAC Offset: 0x1e8) (R/ 32) PTP Event Frame Received Seconds Low Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:32;                    /**< bit:  0..31  Register Update                          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_EFRSL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFRSL_OFFSET                   (0x1E8)                                       /**<  (GMAC_EFRSL) PTP Event Frame Received Seconds Low Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_EFRSL_RUD_Pos                  0                                              /**< (GMAC_EFRSL) Register Update Position */
#define GMAC_EFRSL_RUD_Msk                  (0xFFFFFFFFU << GMAC_EFRSL_RUD_Pos)            /**< (GMAC_EFRSL) Register Update Mask */
#define GMAC_EFRSL_RUD(value)               (GMAC_EFRSL_RUD_Msk & ((value) << GMAC_EFRSL_RUD_Pos))
#define GMAC_EFRSL_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_EFRSL) Register MASK  (Use GMAC_EFRSL_Msk instead)  */
#define GMAC_EFRSL_Msk                      (0xFFFFFFFFU)                                  /**< (GMAC_EFRSL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_EFRN : (GMAC Offset: 0x1ec) (R/ 32) PTP Event Frame Received Nanoseconds Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:30;                    /**< bit:  0..29  Register Update                          */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_EFRN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFRN_OFFSET                    (0x1EC)                                       /**<  (GMAC_EFRN) PTP Event Frame Received Nanoseconds Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_EFRN_RUD_Pos                   0                                              /**< (GMAC_EFRN) Register Update Position */
#define GMAC_EFRN_RUD_Msk                   (0x3FFFFFFFU << GMAC_EFRN_RUD_Pos)             /**< (GMAC_EFRN) Register Update Mask */
#define GMAC_EFRN_RUD(value)                (GMAC_EFRN_RUD_Msk & ((value) << GMAC_EFRN_RUD_Pos))
#define GMAC_EFRN_MASK                      (0x3FFFFFFFU)                                  /**< \deprecated (GMAC_EFRN) Register MASK  (Use GMAC_EFRN_Msk instead)  */
#define GMAC_EFRN_Msk                       (0x3FFFFFFFU)                                  /**< (GMAC_EFRN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PEFTSL : (GMAC Offset: 0x1f0) (R/ 32) PTP Peer Event Frame Transmitted Seconds Low Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:32;                    /**< bit:  0..31  Register Update                          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PEFTSL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFTSL_OFFSET                  (0x1F0)                                       /**<  (GMAC_PEFTSL) PTP Peer Event Frame Transmitted Seconds Low Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PEFTSL_RUD_Pos                 0                                              /**< (GMAC_PEFTSL) Register Update Position */
#define GMAC_PEFTSL_RUD_Msk                 (0xFFFFFFFFU << GMAC_PEFTSL_RUD_Pos)           /**< (GMAC_PEFTSL) Register Update Mask */
#define GMAC_PEFTSL_RUD(value)              (GMAC_PEFTSL_RUD_Msk & ((value) << GMAC_PEFTSL_RUD_Pos))
#define GMAC_PEFTSL_MASK                    (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_PEFTSL) Register MASK  (Use GMAC_PEFTSL_Msk instead)  */
#define GMAC_PEFTSL_Msk                     (0xFFFFFFFFU)                                  /**< (GMAC_PEFTSL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PEFTN : (GMAC Offset: 0x1f4) (R/ 32) PTP Peer Event Frame Transmitted Nanoseconds Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:30;                    /**< bit:  0..29  Register Update                          */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PEFTN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFTN_OFFSET                   (0x1F4)                                       /**<  (GMAC_PEFTN) PTP Peer Event Frame Transmitted Nanoseconds Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PEFTN_RUD_Pos                  0                                              /**< (GMAC_PEFTN) Register Update Position */
#define GMAC_PEFTN_RUD_Msk                  (0x3FFFFFFFU << GMAC_PEFTN_RUD_Pos)            /**< (GMAC_PEFTN) Register Update Mask */
#define GMAC_PEFTN_RUD(value)               (GMAC_PEFTN_RUD_Msk & ((value) << GMAC_PEFTN_RUD_Pos))
#define GMAC_PEFTN_MASK                     (0x3FFFFFFFU)                                  /**< \deprecated (GMAC_PEFTN) Register MASK  (Use GMAC_PEFTN_Msk instead)  */
#define GMAC_PEFTN_Msk                      (0x3FFFFFFFU)                                  /**< (GMAC_PEFTN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PEFRSL : (GMAC Offset: 0x1f8) (R/ 32) PTP Peer Event Frame Received Seconds Low Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:32;                    /**< bit:  0..31  Register Update                          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PEFRSL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFRSL_OFFSET                  (0x1F8)                                       /**<  (GMAC_PEFRSL) PTP Peer Event Frame Received Seconds Low Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PEFRSL_RUD_Pos                 0                                              /**< (GMAC_PEFRSL) Register Update Position */
#define GMAC_PEFRSL_RUD_Msk                 (0xFFFFFFFFU << GMAC_PEFRSL_RUD_Pos)           /**< (GMAC_PEFRSL) Register Update Mask */
#define GMAC_PEFRSL_RUD(value)              (GMAC_PEFRSL_RUD_Msk & ((value) << GMAC_PEFRSL_RUD_Pos))
#define GMAC_PEFRSL_MASK                    (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_PEFRSL) Register MASK  (Use GMAC_PEFRSL_Msk instead)  */
#define GMAC_PEFRSL_Msk                     (0xFFFFFFFFU)                                  /**< (GMAC_PEFRSL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_PEFRN : (GMAC Offset: 0x1fc) (R/ 32) PTP Peer Event Frame Received Nanoseconds Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RUD:30;                    /**< bit:  0..29  Register Update                          */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_PEFRN_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFRN_OFFSET                   (0x1FC)                                       /**<  (GMAC_PEFRN) PTP Peer Event Frame Received Nanoseconds Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_PEFRN_RUD_Pos                  0                                              /**< (GMAC_PEFRN) Register Update Position */
#define GMAC_PEFRN_RUD_Msk                  (0x3FFFFFFFU << GMAC_PEFRN_RUD_Pos)            /**< (GMAC_PEFRN) Register Update Mask */
#define GMAC_PEFRN_RUD(value)               (GMAC_PEFRN_RUD_Msk & ((value) << GMAC_PEFRN_RUD_Pos))
#define GMAC_PEFRN_MASK                     (0x3FFFFFFFU)                                  /**< \deprecated (GMAC_PEFRN) Register MASK  (Use GMAC_PEFRN_Msk instead)  */
#define GMAC_PEFRN_Msk                      (0x3FFFFFFFU)                                  /**< (GMAC_PEFRN) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ISRPQ : (GMAC Offset: 0x400) (R/ 32) Interrupt Status Register Priority Queue  (index = 1) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t :2;                        /**< bit:   3..4  Reserved */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded or Late Collision   */
    uint32_t TFC:1;                     /**< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ISRPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ISRPQ_OFFSET                   (0x400)                                       /**<  (GMAC_ISRPQ) Interrupt Status Register Priority Queue  (index = 1) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ISRPQ_RCOMP_Pos                1                                              /**< (GMAC_ISRPQ) Receive Complete Position */
#define GMAC_ISRPQ_RCOMP_Msk                (0x1U << GMAC_ISRPQ_RCOMP_Pos)                 /**< (GMAC_ISRPQ) Receive Complete Mask */
#define GMAC_ISRPQ_RCOMP                    GMAC_ISRPQ_RCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISRPQ_RCOMP_Msk instead */
#define GMAC_ISRPQ_RXUBR_Pos                2                                              /**< (GMAC_ISRPQ) RX Used Bit Read Position */
#define GMAC_ISRPQ_RXUBR_Msk                (0x1U << GMAC_ISRPQ_RXUBR_Pos)                 /**< (GMAC_ISRPQ) RX Used Bit Read Mask */
#define GMAC_ISRPQ_RXUBR                    GMAC_ISRPQ_RXUBR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISRPQ_RXUBR_Msk instead */
#define GMAC_ISRPQ_RLEX_Pos                 5                                              /**< (GMAC_ISRPQ) Retry Limit Exceeded or Late Collision Position */
#define GMAC_ISRPQ_RLEX_Msk                 (0x1U << GMAC_ISRPQ_RLEX_Pos)                  /**< (GMAC_ISRPQ) Retry Limit Exceeded or Late Collision Mask */
#define GMAC_ISRPQ_RLEX                     GMAC_ISRPQ_RLEX_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISRPQ_RLEX_Msk instead */
#define GMAC_ISRPQ_TFC_Pos                  6                                              /**< (GMAC_ISRPQ) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_ISRPQ_TFC_Msk                  (0x1U << GMAC_ISRPQ_TFC_Pos)                   /**< (GMAC_ISRPQ) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_ISRPQ_TFC                      GMAC_ISRPQ_TFC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISRPQ_TFC_Msk instead */
#define GMAC_ISRPQ_TCOMP_Pos                7                                              /**< (GMAC_ISRPQ) Transmit Complete Position */
#define GMAC_ISRPQ_TCOMP_Msk                (0x1U << GMAC_ISRPQ_TCOMP_Pos)                 /**< (GMAC_ISRPQ) Transmit Complete Mask */
#define GMAC_ISRPQ_TCOMP                    GMAC_ISRPQ_TCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISRPQ_TCOMP_Msk instead */
#define GMAC_ISRPQ_ROVR_Pos                 10                                             /**< (GMAC_ISRPQ) Receive Overrun Position */
#define GMAC_ISRPQ_ROVR_Msk                 (0x1U << GMAC_ISRPQ_ROVR_Pos)                  /**< (GMAC_ISRPQ) Receive Overrun Mask */
#define GMAC_ISRPQ_ROVR                     GMAC_ISRPQ_ROVR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISRPQ_ROVR_Msk instead */
#define GMAC_ISRPQ_HRESP_Pos                11                                             /**< (GMAC_ISRPQ) HRESP Not OK Position */
#define GMAC_ISRPQ_HRESP_Msk                (0x1U << GMAC_ISRPQ_HRESP_Pos)                 /**< (GMAC_ISRPQ) HRESP Not OK Mask */
#define GMAC_ISRPQ_HRESP                    GMAC_ISRPQ_HRESP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ISRPQ_HRESP_Msk instead */
#define GMAC_ISRPQ_MASK                     (0xCE6U)                                       /**< \deprecated (GMAC_ISRPQ) Register MASK  (Use GMAC_ISRPQ_Msk instead)  */
#define GMAC_ISRPQ_Msk                      (0xCE6U)                                       /**< (GMAC_ISRPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_TBQBAPQ : (GMAC Offset: 0x440) (R/W 32) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t TXBQBA:30;                 /**< bit:  2..31  Transmit Buffer Queue Base Address       */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_TBQBAPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBQBAPQ_OFFSET                 (0x440)                                       /**<  (GMAC_TBQBAPQ) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_TBQBAPQ_TXBQBA_Pos             2                                              /**< (GMAC_TBQBAPQ) Transmit Buffer Queue Base Address Position */
#define GMAC_TBQBAPQ_TXBQBA_Msk             (0x3FFFFFFFU << GMAC_TBQBAPQ_TXBQBA_Pos)       /**< (GMAC_TBQBAPQ) Transmit Buffer Queue Base Address Mask */
#define GMAC_TBQBAPQ_TXBQBA(value)          (GMAC_TBQBAPQ_TXBQBA_Msk & ((value) << GMAC_TBQBAPQ_TXBQBA_Pos))
#define GMAC_TBQBAPQ_MASK                   (0xFFFFFFFCU)                                  /**< \deprecated (GMAC_TBQBAPQ) Register MASK  (Use GMAC_TBQBAPQ_Msk instead)  */
#define GMAC_TBQBAPQ_Msk                    (0xFFFFFFFCU)                                  /**< (GMAC_TBQBAPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RBQBAPQ : (GMAC Offset: 0x480) (R/W 32) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t RXBQBA:30;                 /**< bit:  2..31  Receive Buffer Queue Base Address        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RBQBAPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RBQBAPQ_OFFSET                 (0x480)                                       /**<  (GMAC_RBQBAPQ) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RBQBAPQ_RXBQBA_Pos             2                                              /**< (GMAC_RBQBAPQ) Receive Buffer Queue Base Address Position */
#define GMAC_RBQBAPQ_RXBQBA_Msk             (0x3FFFFFFFU << GMAC_RBQBAPQ_RXBQBA_Pos)       /**< (GMAC_RBQBAPQ) Receive Buffer Queue Base Address Mask */
#define GMAC_RBQBAPQ_RXBQBA(value)          (GMAC_RBQBAPQ_RXBQBA_Msk & ((value) << GMAC_RBQBAPQ_RXBQBA_Pos))
#define GMAC_RBQBAPQ_MASK                   (0xFFFFFFFCU)                                  /**< \deprecated (GMAC_RBQBAPQ) Register MASK  (Use GMAC_RBQBAPQ_Msk instead)  */
#define GMAC_RBQBAPQ_Msk                    (0xFFFFFFFCU)                                  /**< (GMAC_RBQBAPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_RBSRPQ : (GMAC Offset: 0x4a0) (R/W 32) Receive Buffer Size Register Priority Queue  (index = 1) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RBS:16;                    /**< bit:  0..15  Receive Buffer Size                      */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_RBSRPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RBSRPQ_OFFSET                  (0x4A0)                                       /**<  (GMAC_RBSRPQ) Receive Buffer Size Register Priority Queue  (index = 1) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_RBSRPQ_RBS_Pos                 0                                              /**< (GMAC_RBSRPQ) Receive Buffer Size Position */
#define GMAC_RBSRPQ_RBS_Msk                 (0xFFFFU << GMAC_RBSRPQ_RBS_Pos)               /**< (GMAC_RBSRPQ) Receive Buffer Size Mask */
#define GMAC_RBSRPQ_RBS(value)              (GMAC_RBSRPQ_RBS_Msk & ((value) << GMAC_RBSRPQ_RBS_Pos))
#define GMAC_RBSRPQ_MASK                    (0xFFFFU)                                      /**< \deprecated (GMAC_RBSRPQ) Register MASK  (Use GMAC_RBSRPQ_Msk instead)  */
#define GMAC_RBSRPQ_Msk                     (0xFFFFU)                                      /**< (GMAC_RBSRPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_CBSCR : (GMAC Offset: 0x4bc) (R/W 32) Credit-Based Shaping Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t QBE:1;                     /**< bit:      0  Queue B CBS Enable                       */
    uint32_t QAE:1;                     /**< bit:      1  Queue A CBS Enable                       */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_CBSCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_CBSCR_OFFSET                   (0x4BC)                                       /**<  (GMAC_CBSCR) Credit-Based Shaping Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_CBSCR_QBE_Pos                  0                                              /**< (GMAC_CBSCR) Queue B CBS Enable Position */
#define GMAC_CBSCR_QBE_Msk                  (0x1U << GMAC_CBSCR_QBE_Pos)                   /**< (GMAC_CBSCR) Queue B CBS Enable Mask */
#define GMAC_CBSCR_QBE                      GMAC_CBSCR_QBE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_CBSCR_QBE_Msk instead */
#define GMAC_CBSCR_QAE_Pos                  1                                              /**< (GMAC_CBSCR) Queue A CBS Enable Position */
#define GMAC_CBSCR_QAE_Msk                  (0x1U << GMAC_CBSCR_QAE_Pos)                   /**< (GMAC_CBSCR) Queue A CBS Enable Mask */
#define GMAC_CBSCR_QAE                      GMAC_CBSCR_QAE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_CBSCR_QAE_Msk instead */
#define GMAC_CBSCR_MASK                     (0x03U)                                        /**< \deprecated (GMAC_CBSCR) Register MASK  (Use GMAC_CBSCR_Msk instead)  */
#define GMAC_CBSCR_Msk                      (0x03U)                                        /**< (GMAC_CBSCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_CBSISQA : (GMAC Offset: 0x4c0) (R/W 32) Credit-Based Shaping IdleSlope Register for Queue A -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IS:32;                     /**< bit:  0..31  IdleSlope                                */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_CBSISQA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_CBSISQA_OFFSET                 (0x4C0)                                       /**<  (GMAC_CBSISQA) Credit-Based Shaping IdleSlope Register for Queue A  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_CBSISQA_IS_Pos                 0                                              /**< (GMAC_CBSISQA) IdleSlope Position */
#define GMAC_CBSISQA_IS_Msk                 (0xFFFFFFFFU << GMAC_CBSISQA_IS_Pos)           /**< (GMAC_CBSISQA) IdleSlope Mask */
#define GMAC_CBSISQA_IS(value)              (GMAC_CBSISQA_IS_Msk & ((value) << GMAC_CBSISQA_IS_Pos))
#define GMAC_CBSISQA_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_CBSISQA) Register MASK  (Use GMAC_CBSISQA_Msk instead)  */
#define GMAC_CBSISQA_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_CBSISQA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_CBSISQB : (GMAC Offset: 0x4c4) (R/W 32) Credit-Based Shaping IdleSlope Register for Queue B -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IS:32;                     /**< bit:  0..31  IdleSlope                                */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_CBSISQB_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_CBSISQB_OFFSET                 (0x4C4)                                       /**<  (GMAC_CBSISQB) Credit-Based Shaping IdleSlope Register for Queue B  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_CBSISQB_IS_Pos                 0                                              /**< (GMAC_CBSISQB) IdleSlope Position */
#define GMAC_CBSISQB_IS_Msk                 (0xFFFFFFFFU << GMAC_CBSISQB_IS_Pos)           /**< (GMAC_CBSISQB) IdleSlope Mask */
#define GMAC_CBSISQB_IS(value)              (GMAC_CBSISQB_IS_Msk & ((value) << GMAC_CBSISQB_IS_Pos))
#define GMAC_CBSISQB_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (GMAC_CBSISQB) Register MASK  (Use GMAC_CBSISQB_Msk instead)  */
#define GMAC_CBSISQB_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_CBSISQB) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST1RPQ : (GMAC Offset: 0x500) (R/W 32) Screening Type 1 Register Priority Queue  (index = 0) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t QNB:3;                     /**< bit:   0..2  Queue Number (0-2)                       */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t DSTCM:8;                   /**< bit:  4..11  Differentiated Services or Traffic Class Match */
    uint32_t UDPM:16;                   /**< bit: 12..27  UDP Port Match                           */
    uint32_t DSTCE:1;                   /**< bit:     28  Differentiated Services or Traffic Class Match Enable */
    uint32_t UDPE:1;                    /**< bit:     29  UDP Port Match Enable                    */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST1RPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST1RPQ_OFFSET                  (0x500)                                       /**<  (GMAC_ST1RPQ) Screening Type 1 Register Priority Queue  (index = 0) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST1RPQ_QNB_Pos                 0                                              /**< (GMAC_ST1RPQ) Queue Number (0-2) Position */
#define GMAC_ST1RPQ_QNB_Msk                 (0x7U << GMAC_ST1RPQ_QNB_Pos)                  /**< (GMAC_ST1RPQ) Queue Number (0-2) Mask */
#define GMAC_ST1RPQ_QNB(value)              (GMAC_ST1RPQ_QNB_Msk & ((value) << GMAC_ST1RPQ_QNB_Pos))
#define GMAC_ST1RPQ_DSTCM_Pos               4                                              /**< (GMAC_ST1RPQ) Differentiated Services or Traffic Class Match Position */
#define GMAC_ST1RPQ_DSTCM_Msk               (0xFFU << GMAC_ST1RPQ_DSTCM_Pos)               /**< (GMAC_ST1RPQ) Differentiated Services or Traffic Class Match Mask */
#define GMAC_ST1RPQ_DSTCM(value)            (GMAC_ST1RPQ_DSTCM_Msk & ((value) << GMAC_ST1RPQ_DSTCM_Pos))
#define GMAC_ST1RPQ_UDPM_Pos                12                                             /**< (GMAC_ST1RPQ) UDP Port Match Position */
#define GMAC_ST1RPQ_UDPM_Msk                (0xFFFFU << GMAC_ST1RPQ_UDPM_Pos)              /**< (GMAC_ST1RPQ) UDP Port Match Mask */
#define GMAC_ST1RPQ_UDPM(value)             (GMAC_ST1RPQ_UDPM_Msk & ((value) << GMAC_ST1RPQ_UDPM_Pos))
#define GMAC_ST1RPQ_DSTCE_Pos               28                                             /**< (GMAC_ST1RPQ) Differentiated Services or Traffic Class Match Enable Position */
#define GMAC_ST1RPQ_DSTCE_Msk               (0x1U << GMAC_ST1RPQ_DSTCE_Pos)                /**< (GMAC_ST1RPQ) Differentiated Services or Traffic Class Match Enable Mask */
#define GMAC_ST1RPQ_DSTCE                   GMAC_ST1RPQ_DSTCE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ST1RPQ_DSTCE_Msk instead */
#define GMAC_ST1RPQ_UDPE_Pos                29                                             /**< (GMAC_ST1RPQ) UDP Port Match Enable Position */
#define GMAC_ST1RPQ_UDPE_Msk                (0x1U << GMAC_ST1RPQ_UDPE_Pos)                 /**< (GMAC_ST1RPQ) UDP Port Match Enable Mask */
#define GMAC_ST1RPQ_UDPE                    GMAC_ST1RPQ_UDPE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ST1RPQ_UDPE_Msk instead */
#define GMAC_ST1RPQ_MASK                    (0x3FFFFFF7U)                                  /**< \deprecated (GMAC_ST1RPQ) Register MASK  (Use GMAC_ST1RPQ_Msk instead)  */
#define GMAC_ST1RPQ_Msk                     (0x3FFFFFF7U)                                  /**< (GMAC_ST1RPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2RPQ : (GMAC Offset: 0x540) (R/W 32) Screening Type 2 Register Priority Queue  (index = 0) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t QNB:3;                     /**< bit:   0..2  Queue Number (0-2)                       */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t VLANP:3;                   /**< bit:   4..6  VLAN Priority                            */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t VLANE:1;                   /**< bit:      8  VLAN Enable                              */
    uint32_t I2ETH:3;                   /**< bit:  9..11  Index of Screening Type 2 EtherType register x */
    uint32_t ETHE:1;                    /**< bit:     12  EtherType Enable                         */
    uint32_t COMPA:5;                   /**< bit: 13..17  Index of Screening Type 2 Compare Word 0/Word 1 register x */
    uint32_t COMPAE:1;                  /**< bit:     18  Compare A Enable                         */
    uint32_t COMPB:5;                   /**< bit: 19..23  Index of Screening Type 2 Compare Word 0/Word 1 register x */
    uint32_t COMPBE:1;                  /**< bit:     24  Compare B Enable                         */
    uint32_t COMPC:5;                   /**< bit: 25..29  Index of Screening Type 2 Compare Word 0/Word 1 register x */
    uint32_t COMPCE:1;                  /**< bit:     30  Compare C Enable                         */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2RPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2RPQ_OFFSET                  (0x540)                                       /**<  (GMAC_ST2RPQ) Screening Type 2 Register Priority Queue  (index = 0) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2RPQ_QNB_Pos                 0                                              /**< (GMAC_ST2RPQ) Queue Number (0-2) Position */
#define GMAC_ST2RPQ_QNB_Msk                 (0x7U << GMAC_ST2RPQ_QNB_Pos)                  /**< (GMAC_ST2RPQ) Queue Number (0-2) Mask */
#define GMAC_ST2RPQ_QNB(value)              (GMAC_ST2RPQ_QNB_Msk & ((value) << GMAC_ST2RPQ_QNB_Pos))
#define GMAC_ST2RPQ_VLANP_Pos               4                                              /**< (GMAC_ST2RPQ) VLAN Priority Position */
#define GMAC_ST2RPQ_VLANP_Msk               (0x7U << GMAC_ST2RPQ_VLANP_Pos)                /**< (GMAC_ST2RPQ) VLAN Priority Mask */
#define GMAC_ST2RPQ_VLANP(value)            (GMAC_ST2RPQ_VLANP_Msk & ((value) << GMAC_ST2RPQ_VLANP_Pos))
#define GMAC_ST2RPQ_VLANE_Pos               8                                              /**< (GMAC_ST2RPQ) VLAN Enable Position */
#define GMAC_ST2RPQ_VLANE_Msk               (0x1U << GMAC_ST2RPQ_VLANE_Pos)                /**< (GMAC_ST2RPQ) VLAN Enable Mask */
#define GMAC_ST2RPQ_VLANE                   GMAC_ST2RPQ_VLANE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ST2RPQ_VLANE_Msk instead */
#define GMAC_ST2RPQ_I2ETH_Pos               9                                              /**< (GMAC_ST2RPQ) Index of Screening Type 2 EtherType register x Position */
#define GMAC_ST2RPQ_I2ETH_Msk               (0x7U << GMAC_ST2RPQ_I2ETH_Pos)                /**< (GMAC_ST2RPQ) Index of Screening Type 2 EtherType register x Mask */
#define GMAC_ST2RPQ_I2ETH(value)            (GMAC_ST2RPQ_I2ETH_Msk & ((value) << GMAC_ST2RPQ_I2ETH_Pos))
#define GMAC_ST2RPQ_ETHE_Pos                12                                             /**< (GMAC_ST2RPQ) EtherType Enable Position */
#define GMAC_ST2RPQ_ETHE_Msk                (0x1U << GMAC_ST2RPQ_ETHE_Pos)                 /**< (GMAC_ST2RPQ) EtherType Enable Mask */
#define GMAC_ST2RPQ_ETHE                    GMAC_ST2RPQ_ETHE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ST2RPQ_ETHE_Msk instead */
#define GMAC_ST2RPQ_COMPA_Pos               13                                             /**< (GMAC_ST2RPQ) Index of Screening Type 2 Compare Word 0/Word 1 register x Position */
#define GMAC_ST2RPQ_COMPA_Msk               (0x1FU << GMAC_ST2RPQ_COMPA_Pos)               /**< (GMAC_ST2RPQ) Index of Screening Type 2 Compare Word 0/Word 1 register x Mask */
#define GMAC_ST2RPQ_COMPA(value)            (GMAC_ST2RPQ_COMPA_Msk & ((value) << GMAC_ST2RPQ_COMPA_Pos))
#define GMAC_ST2RPQ_COMPAE_Pos              18                                             /**< (GMAC_ST2RPQ) Compare A Enable Position */
#define GMAC_ST2RPQ_COMPAE_Msk              (0x1U << GMAC_ST2RPQ_COMPAE_Pos)               /**< (GMAC_ST2RPQ) Compare A Enable Mask */
#define GMAC_ST2RPQ_COMPAE                  GMAC_ST2RPQ_COMPAE_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ST2RPQ_COMPAE_Msk instead */
#define GMAC_ST2RPQ_COMPB_Pos               19                                             /**< (GMAC_ST2RPQ) Index of Screening Type 2 Compare Word 0/Word 1 register x Position */
#define GMAC_ST2RPQ_COMPB_Msk               (0x1FU << GMAC_ST2RPQ_COMPB_Pos)               /**< (GMAC_ST2RPQ) Index of Screening Type 2 Compare Word 0/Word 1 register x Mask */
#define GMAC_ST2RPQ_COMPB(value)            (GMAC_ST2RPQ_COMPB_Msk & ((value) << GMAC_ST2RPQ_COMPB_Pos))
#define GMAC_ST2RPQ_COMPBE_Pos              24                                             /**< (GMAC_ST2RPQ) Compare B Enable Position */
#define GMAC_ST2RPQ_COMPBE_Msk              (0x1U << GMAC_ST2RPQ_COMPBE_Pos)               /**< (GMAC_ST2RPQ) Compare B Enable Mask */
#define GMAC_ST2RPQ_COMPBE                  GMAC_ST2RPQ_COMPBE_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ST2RPQ_COMPBE_Msk instead */
#define GMAC_ST2RPQ_COMPC_Pos               25                                             /**< (GMAC_ST2RPQ) Index of Screening Type 2 Compare Word 0/Word 1 register x Position */
#define GMAC_ST2RPQ_COMPC_Msk               (0x1FU << GMAC_ST2RPQ_COMPC_Pos)               /**< (GMAC_ST2RPQ) Index of Screening Type 2 Compare Word 0/Word 1 register x Mask */
#define GMAC_ST2RPQ_COMPC(value)            (GMAC_ST2RPQ_COMPC_Msk & ((value) << GMAC_ST2RPQ_COMPC_Pos))
#define GMAC_ST2RPQ_COMPCE_Pos              30                                             /**< (GMAC_ST2RPQ) Compare C Enable Position */
#define GMAC_ST2RPQ_COMPCE_Msk              (0x1U << GMAC_ST2RPQ_COMPCE_Pos)               /**< (GMAC_ST2RPQ) Compare C Enable Mask */
#define GMAC_ST2RPQ_COMPCE                  GMAC_ST2RPQ_COMPCE_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_ST2RPQ_COMPCE_Msk instead */
#define GMAC_ST2RPQ_MASK                    (0x7FFFFF77U)                                  /**< \deprecated (GMAC_ST2RPQ) Register MASK  (Use GMAC_ST2RPQ_Msk instead)  */
#define GMAC_ST2RPQ_Msk                     (0x7FFFFF77U)                                  /**< (GMAC_ST2RPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IERPQ : (GMAC Offset: 0x600) (/W 32) Interrupt Enable Register Priority Queue  (index = 1) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t :2;                        /**< bit:   3..4  Reserved */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded or Late Collision   */
    uint32_t TFC:1;                     /**< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IERPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IERPQ_OFFSET                   (0x600)                                       /**<  (GMAC_IERPQ) Interrupt Enable Register Priority Queue  (index = 1) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IERPQ_RCOMP_Pos                1                                              /**< (GMAC_IERPQ) Receive Complete Position */
#define GMAC_IERPQ_RCOMP_Msk                (0x1U << GMAC_IERPQ_RCOMP_Pos)                 /**< (GMAC_IERPQ) Receive Complete Mask */
#define GMAC_IERPQ_RCOMP                    GMAC_IERPQ_RCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IERPQ_RCOMP_Msk instead */
#define GMAC_IERPQ_RXUBR_Pos                2                                              /**< (GMAC_IERPQ) RX Used Bit Read Position */
#define GMAC_IERPQ_RXUBR_Msk                (0x1U << GMAC_IERPQ_RXUBR_Pos)                 /**< (GMAC_IERPQ) RX Used Bit Read Mask */
#define GMAC_IERPQ_RXUBR                    GMAC_IERPQ_RXUBR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IERPQ_RXUBR_Msk instead */
#define GMAC_IERPQ_RLEX_Pos                 5                                              /**< (GMAC_IERPQ) Retry Limit Exceeded or Late Collision Position */
#define GMAC_IERPQ_RLEX_Msk                 (0x1U << GMAC_IERPQ_RLEX_Pos)                  /**< (GMAC_IERPQ) Retry Limit Exceeded or Late Collision Mask */
#define GMAC_IERPQ_RLEX                     GMAC_IERPQ_RLEX_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IERPQ_RLEX_Msk instead */
#define GMAC_IERPQ_TFC_Pos                  6                                              /**< (GMAC_IERPQ) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_IERPQ_TFC_Msk                  (0x1U << GMAC_IERPQ_TFC_Pos)                   /**< (GMAC_IERPQ) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_IERPQ_TFC                      GMAC_IERPQ_TFC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IERPQ_TFC_Msk instead */
#define GMAC_IERPQ_TCOMP_Pos                7                                              /**< (GMAC_IERPQ) Transmit Complete Position */
#define GMAC_IERPQ_TCOMP_Msk                (0x1U << GMAC_IERPQ_TCOMP_Pos)                 /**< (GMAC_IERPQ) Transmit Complete Mask */
#define GMAC_IERPQ_TCOMP                    GMAC_IERPQ_TCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IERPQ_TCOMP_Msk instead */
#define GMAC_IERPQ_ROVR_Pos                 10                                             /**< (GMAC_IERPQ) Receive Overrun Position */
#define GMAC_IERPQ_ROVR_Msk                 (0x1U << GMAC_IERPQ_ROVR_Pos)                  /**< (GMAC_IERPQ) Receive Overrun Mask */
#define GMAC_IERPQ_ROVR                     GMAC_IERPQ_ROVR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IERPQ_ROVR_Msk instead */
#define GMAC_IERPQ_HRESP_Pos                11                                             /**< (GMAC_IERPQ) HRESP Not OK Position */
#define GMAC_IERPQ_HRESP_Msk                (0x1U << GMAC_IERPQ_HRESP_Pos)                 /**< (GMAC_IERPQ) HRESP Not OK Mask */
#define GMAC_IERPQ_HRESP                    GMAC_IERPQ_HRESP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IERPQ_HRESP_Msk instead */
#define GMAC_IERPQ_MASK                     (0xCE6U)                                       /**< \deprecated (GMAC_IERPQ) Register MASK  (Use GMAC_IERPQ_Msk instead)  */
#define GMAC_IERPQ_Msk                      (0xCE6U)                                       /**< (GMAC_IERPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IDRPQ : (GMAC Offset: 0x620) (/W 32) Interrupt Disable Register Priority Queue  (index = 1) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t :2;                        /**< bit:   3..4  Reserved */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded or Late Collision   */
    uint32_t TFC:1;                     /**< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IDRPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IDRPQ_OFFSET                   (0x620)                                       /**<  (GMAC_IDRPQ) Interrupt Disable Register Priority Queue  (index = 1) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IDRPQ_RCOMP_Pos                1                                              /**< (GMAC_IDRPQ) Receive Complete Position */
#define GMAC_IDRPQ_RCOMP_Msk                (0x1U << GMAC_IDRPQ_RCOMP_Pos)                 /**< (GMAC_IDRPQ) Receive Complete Mask */
#define GMAC_IDRPQ_RCOMP                    GMAC_IDRPQ_RCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDRPQ_RCOMP_Msk instead */
#define GMAC_IDRPQ_RXUBR_Pos                2                                              /**< (GMAC_IDRPQ) RX Used Bit Read Position */
#define GMAC_IDRPQ_RXUBR_Msk                (0x1U << GMAC_IDRPQ_RXUBR_Pos)                 /**< (GMAC_IDRPQ) RX Used Bit Read Mask */
#define GMAC_IDRPQ_RXUBR                    GMAC_IDRPQ_RXUBR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDRPQ_RXUBR_Msk instead */
#define GMAC_IDRPQ_RLEX_Pos                 5                                              /**< (GMAC_IDRPQ) Retry Limit Exceeded or Late Collision Position */
#define GMAC_IDRPQ_RLEX_Msk                 (0x1U << GMAC_IDRPQ_RLEX_Pos)                  /**< (GMAC_IDRPQ) Retry Limit Exceeded or Late Collision Mask */
#define GMAC_IDRPQ_RLEX                     GMAC_IDRPQ_RLEX_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDRPQ_RLEX_Msk instead */
#define GMAC_IDRPQ_TFC_Pos                  6                                              /**< (GMAC_IDRPQ) Transmit Frame Corruption Due to AHB Error Position */
#define GMAC_IDRPQ_TFC_Msk                  (0x1U << GMAC_IDRPQ_TFC_Pos)                   /**< (GMAC_IDRPQ) Transmit Frame Corruption Due to AHB Error Mask */
#define GMAC_IDRPQ_TFC                      GMAC_IDRPQ_TFC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDRPQ_TFC_Msk instead */
#define GMAC_IDRPQ_TCOMP_Pos                7                                              /**< (GMAC_IDRPQ) Transmit Complete Position */
#define GMAC_IDRPQ_TCOMP_Msk                (0x1U << GMAC_IDRPQ_TCOMP_Pos)                 /**< (GMAC_IDRPQ) Transmit Complete Mask */
#define GMAC_IDRPQ_TCOMP                    GMAC_IDRPQ_TCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDRPQ_TCOMP_Msk instead */
#define GMAC_IDRPQ_ROVR_Pos                 10                                             /**< (GMAC_IDRPQ) Receive Overrun Position */
#define GMAC_IDRPQ_ROVR_Msk                 (0x1U << GMAC_IDRPQ_ROVR_Pos)                  /**< (GMAC_IDRPQ) Receive Overrun Mask */
#define GMAC_IDRPQ_ROVR                     GMAC_IDRPQ_ROVR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDRPQ_ROVR_Msk instead */
#define GMAC_IDRPQ_HRESP_Pos                11                                             /**< (GMAC_IDRPQ) HRESP Not OK Position */
#define GMAC_IDRPQ_HRESP_Msk                (0x1U << GMAC_IDRPQ_HRESP_Pos)                 /**< (GMAC_IDRPQ) HRESP Not OK Mask */
#define GMAC_IDRPQ_HRESP                    GMAC_IDRPQ_HRESP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IDRPQ_HRESP_Msk instead */
#define GMAC_IDRPQ_MASK                     (0xCE6U)                                       /**< \deprecated (GMAC_IDRPQ) Register MASK  (Use GMAC_IDRPQ_Msk instead)  */
#define GMAC_IDRPQ_Msk                      (0xCE6U)                                       /**< (GMAC_IDRPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_IMRPQ : (GMAC Offset: 0x640) (R/W 32) Interrupt Mask Register Priority Queue  (index = 1) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :1;                        /**< bit:      0  Reserved */
    uint32_t RCOMP:1;                   /**< bit:      1  Receive Complete                         */
    uint32_t RXUBR:1;                   /**< bit:      2  RX Used Bit Read                         */
    uint32_t :2;                        /**< bit:   3..4  Reserved */
    uint32_t RLEX:1;                    /**< bit:      5  Retry Limit Exceeded or Late Collision   */
    uint32_t AHB:1;                     /**< bit:      6  AHB Error                                */
    uint32_t TCOMP:1;                   /**< bit:      7  Transmit Complete                        */
    uint32_t :2;                        /**< bit:   8..9  Reserved */
    uint32_t ROVR:1;                    /**< bit:     10  Receive Overrun                          */
    uint32_t HRESP:1;                   /**< bit:     11  HRESP Not OK                             */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_IMRPQ_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IMRPQ_OFFSET                   (0x640)                                       /**<  (GMAC_IMRPQ) Interrupt Mask Register Priority Queue  (index = 1) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_IMRPQ_RCOMP_Pos                1                                              /**< (GMAC_IMRPQ) Receive Complete Position */
#define GMAC_IMRPQ_RCOMP_Msk                (0x1U << GMAC_IMRPQ_RCOMP_Pos)                 /**< (GMAC_IMRPQ) Receive Complete Mask */
#define GMAC_IMRPQ_RCOMP                    GMAC_IMRPQ_RCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMRPQ_RCOMP_Msk instead */
#define GMAC_IMRPQ_RXUBR_Pos                2                                              /**< (GMAC_IMRPQ) RX Used Bit Read Position */
#define GMAC_IMRPQ_RXUBR_Msk                (0x1U << GMAC_IMRPQ_RXUBR_Pos)                 /**< (GMAC_IMRPQ) RX Used Bit Read Mask */
#define GMAC_IMRPQ_RXUBR                    GMAC_IMRPQ_RXUBR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMRPQ_RXUBR_Msk instead */
#define GMAC_IMRPQ_RLEX_Pos                 5                                              /**< (GMAC_IMRPQ) Retry Limit Exceeded or Late Collision Position */
#define GMAC_IMRPQ_RLEX_Msk                 (0x1U << GMAC_IMRPQ_RLEX_Pos)                  /**< (GMAC_IMRPQ) Retry Limit Exceeded or Late Collision Mask */
#define GMAC_IMRPQ_RLEX                     GMAC_IMRPQ_RLEX_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMRPQ_RLEX_Msk instead */
#define GMAC_IMRPQ_AHB_Pos                  6                                              /**< (GMAC_IMRPQ) AHB Error Position */
#define GMAC_IMRPQ_AHB_Msk                  (0x1U << GMAC_IMRPQ_AHB_Pos)                   /**< (GMAC_IMRPQ) AHB Error Mask */
#define GMAC_IMRPQ_AHB                      GMAC_IMRPQ_AHB_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMRPQ_AHB_Msk instead */
#define GMAC_IMRPQ_TCOMP_Pos                7                                              /**< (GMAC_IMRPQ) Transmit Complete Position */
#define GMAC_IMRPQ_TCOMP_Msk                (0x1U << GMAC_IMRPQ_TCOMP_Pos)                 /**< (GMAC_IMRPQ) Transmit Complete Mask */
#define GMAC_IMRPQ_TCOMP                    GMAC_IMRPQ_TCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMRPQ_TCOMP_Msk instead */
#define GMAC_IMRPQ_ROVR_Pos                 10                                             /**< (GMAC_IMRPQ) Receive Overrun Position */
#define GMAC_IMRPQ_ROVR_Msk                 (0x1U << GMAC_IMRPQ_ROVR_Pos)                  /**< (GMAC_IMRPQ) Receive Overrun Mask */
#define GMAC_IMRPQ_ROVR                     GMAC_IMRPQ_ROVR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMRPQ_ROVR_Msk instead */
#define GMAC_IMRPQ_HRESP_Pos                11                                             /**< (GMAC_IMRPQ) HRESP Not OK Position */
#define GMAC_IMRPQ_HRESP_Msk                (0x1U << GMAC_IMRPQ_HRESP_Pos)                 /**< (GMAC_IMRPQ) HRESP Not OK Mask */
#define GMAC_IMRPQ_HRESP                    GMAC_IMRPQ_HRESP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use GMAC_IMRPQ_HRESP_Msk instead */
#define GMAC_IMRPQ_MASK                     (0xCE6U)                                       /**< \deprecated (GMAC_IMRPQ) Register MASK  (Use GMAC_IMRPQ_Msk instead)  */
#define GMAC_IMRPQ_Msk                      (0xCE6U)                                       /**< (GMAC_IMRPQ) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2ER : (GMAC Offset: 0x6e0) (R/W 32) Screening Type 2 Ethertype Register  (index = 0) 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t COMPVAL:16;                /**< bit:  0..15  Ethertype Compare Value                  */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2ER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2ER_OFFSET                   (0x6E0)                                       /**<  (GMAC_ST2ER) Screening Type 2 Ethertype Register  (index = 0) 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2ER_COMPVAL_Pos              0                                              /**< (GMAC_ST2ER) Ethertype Compare Value Position */
#define GMAC_ST2ER_COMPVAL_Msk              (0xFFFFU << GMAC_ST2ER_COMPVAL_Pos)            /**< (GMAC_ST2ER) Ethertype Compare Value Mask */
#define GMAC_ST2ER_COMPVAL(value)           (GMAC_ST2ER_COMPVAL_Msk & ((value) << GMAC_ST2ER_COMPVAL_Pos))
#define GMAC_ST2ER_MASK                     (0xFFFFU)                                      /**< \deprecated (GMAC_ST2ER) Register MASK  (Use GMAC_ST2ER_Msk instead)  */
#define GMAC_ST2ER_Msk                      (0xFFFFU)                                      /**< (GMAC_ST2ER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW00 : (GMAC Offset: 0x700) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW00_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW00_OFFSET                 (0x700)                                       /**<  (GMAC_ST2CW00) Screening Type 2 Compare Word 0 Register  (index = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW00_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW00) Mask Value Position */
#define GMAC_ST2CW00_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW00_MASKVAL_Pos)          /**< (GMAC_ST2CW00) Mask Value Mask */
#define GMAC_ST2CW00_MASKVAL(value)         (GMAC_ST2CW00_MASKVAL_Msk & ((value) << GMAC_ST2CW00_MASKVAL_Pos))
#define GMAC_ST2CW00_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW00) Compare Value Position */
#define GMAC_ST2CW00_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW00_COMPVAL_Pos)          /**< (GMAC_ST2CW00) Compare Value Mask */
#define GMAC_ST2CW00_COMPVAL(value)         (GMAC_ST2CW00_COMPVAL_Msk & ((value) << GMAC_ST2CW00_COMPVAL_Pos))
#define GMAC_ST2CW00_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW00) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW10 : (GMAC Offset: 0x704) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW10_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW10_OFFSET                 (0x704)                                       /**<  (GMAC_ST2CW10) Screening Type 2 Compare Word 1 Register  (index = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW10_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW10) Offset Value in Bytes Position */
#define GMAC_ST2CW10_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW10_OFFSVAL_Pos)            /**< (GMAC_ST2CW10) Offset Value in Bytes Mask */
#define GMAC_ST2CW10_OFFSVAL(value)         (GMAC_ST2CW10_OFFSVAL_Msk & ((value) << GMAC_ST2CW10_OFFSVAL_Pos))
#define GMAC_ST2CW10_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW10) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW10_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW10_OFFSSTRT_Pos)            /**< (GMAC_ST2CW10) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW10_OFFSSTRT(value)        (GMAC_ST2CW10_OFFSSTRT_Msk & ((value) << GMAC_ST2CW10_OFFSSTRT_Pos))
#define   GMAC_ST2CW10_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW10) Offset from the start of the frame  */
#define   GMAC_ST2CW10_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW10) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW10_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW10) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW10_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW10) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW10_OFFSSTRT_FRAMESTART    (GMAC_ST2CW10_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW10_OFFSSTRT_Pos)  /**< (GMAC_ST2CW10) Offset from the start of the frame Position  */
#define GMAC_ST2CW10_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW10_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW10_OFFSSTRT_Pos)  /**< (GMAC_ST2CW10) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW10_OFFSSTRT_IP            (GMAC_ST2CW10_OFFSSTRT_IP_Val << GMAC_ST2CW10_OFFSSTRT_Pos)  /**< (GMAC_ST2CW10) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW10_OFFSSTRT_TCP_UDP       (GMAC_ST2CW10_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW10_OFFSSTRT_Pos)  /**< (GMAC_ST2CW10) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW10_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW10) Register MASK  (Use GMAC_ST2CW10_Msk instead)  */
#define GMAC_ST2CW10_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW10) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW01 : (GMAC Offset: 0x708) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 1) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW01_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW01_OFFSET                 (0x708)                                       /**<  (GMAC_ST2CW01) Screening Type 2 Compare Word 0 Register  (index = 1)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW01_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW01) Mask Value Position */
#define GMAC_ST2CW01_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW01_MASKVAL_Pos)          /**< (GMAC_ST2CW01) Mask Value Mask */
#define GMAC_ST2CW01_MASKVAL(value)         (GMAC_ST2CW01_MASKVAL_Msk & ((value) << GMAC_ST2CW01_MASKVAL_Pos))
#define GMAC_ST2CW01_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW01) Compare Value Position */
#define GMAC_ST2CW01_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW01_COMPVAL_Pos)          /**< (GMAC_ST2CW01) Compare Value Mask */
#define GMAC_ST2CW01_COMPVAL(value)         (GMAC_ST2CW01_COMPVAL_Msk & ((value) << GMAC_ST2CW01_COMPVAL_Pos))
#define GMAC_ST2CW01_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW01) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW11 : (GMAC Offset: 0x70c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 1) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW11_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW11_OFFSET                 (0x70C)                                       /**<  (GMAC_ST2CW11) Screening Type 2 Compare Word 1 Register  (index = 1)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW11_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW11) Offset Value in Bytes Position */
#define GMAC_ST2CW11_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW11_OFFSVAL_Pos)            /**< (GMAC_ST2CW11) Offset Value in Bytes Mask */
#define GMAC_ST2CW11_OFFSVAL(value)         (GMAC_ST2CW11_OFFSVAL_Msk & ((value) << GMAC_ST2CW11_OFFSVAL_Pos))
#define GMAC_ST2CW11_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW11) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW11_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW11_OFFSSTRT_Pos)            /**< (GMAC_ST2CW11) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW11_OFFSSTRT(value)        (GMAC_ST2CW11_OFFSSTRT_Msk & ((value) << GMAC_ST2CW11_OFFSSTRT_Pos))
#define   GMAC_ST2CW11_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW11) Offset from the start of the frame  */
#define   GMAC_ST2CW11_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW11) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW11_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW11) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW11_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW11) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW11_OFFSSTRT_FRAMESTART    (GMAC_ST2CW11_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW11_OFFSSTRT_Pos)  /**< (GMAC_ST2CW11) Offset from the start of the frame Position  */
#define GMAC_ST2CW11_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW11_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW11_OFFSSTRT_Pos)  /**< (GMAC_ST2CW11) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW11_OFFSSTRT_IP            (GMAC_ST2CW11_OFFSSTRT_IP_Val << GMAC_ST2CW11_OFFSSTRT_Pos)  /**< (GMAC_ST2CW11) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW11_OFFSSTRT_TCP_UDP       (GMAC_ST2CW11_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW11_OFFSSTRT_Pos)  /**< (GMAC_ST2CW11) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW11_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW11) Register MASK  (Use GMAC_ST2CW11_Msk instead)  */
#define GMAC_ST2CW11_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW11) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW02 : (GMAC Offset: 0x710) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 2) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW02_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW02_OFFSET                 (0x710)                                       /**<  (GMAC_ST2CW02) Screening Type 2 Compare Word 0 Register  (index = 2)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW02_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW02) Mask Value Position */
#define GMAC_ST2CW02_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW02_MASKVAL_Pos)          /**< (GMAC_ST2CW02) Mask Value Mask */
#define GMAC_ST2CW02_MASKVAL(value)         (GMAC_ST2CW02_MASKVAL_Msk & ((value) << GMAC_ST2CW02_MASKVAL_Pos))
#define GMAC_ST2CW02_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW02) Compare Value Position */
#define GMAC_ST2CW02_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW02_COMPVAL_Pos)          /**< (GMAC_ST2CW02) Compare Value Mask */
#define GMAC_ST2CW02_COMPVAL(value)         (GMAC_ST2CW02_COMPVAL_Msk & ((value) << GMAC_ST2CW02_COMPVAL_Pos))
#define GMAC_ST2CW02_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW02) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW12 : (GMAC Offset: 0x714) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 2) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW12_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW12_OFFSET                 (0x714)                                       /**<  (GMAC_ST2CW12) Screening Type 2 Compare Word 1 Register  (index = 2)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW12_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW12) Offset Value in Bytes Position */
#define GMAC_ST2CW12_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW12_OFFSVAL_Pos)            /**< (GMAC_ST2CW12) Offset Value in Bytes Mask */
#define GMAC_ST2CW12_OFFSVAL(value)         (GMAC_ST2CW12_OFFSVAL_Msk & ((value) << GMAC_ST2CW12_OFFSVAL_Pos))
#define GMAC_ST2CW12_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW12) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW12_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW12_OFFSSTRT_Pos)            /**< (GMAC_ST2CW12) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW12_OFFSSTRT(value)        (GMAC_ST2CW12_OFFSSTRT_Msk & ((value) << GMAC_ST2CW12_OFFSSTRT_Pos))
#define   GMAC_ST2CW12_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW12) Offset from the start of the frame  */
#define   GMAC_ST2CW12_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW12) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW12_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW12) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW12_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW12) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW12_OFFSSTRT_FRAMESTART    (GMAC_ST2CW12_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW12_OFFSSTRT_Pos)  /**< (GMAC_ST2CW12) Offset from the start of the frame Position  */
#define GMAC_ST2CW12_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW12_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW12_OFFSSTRT_Pos)  /**< (GMAC_ST2CW12) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW12_OFFSSTRT_IP            (GMAC_ST2CW12_OFFSSTRT_IP_Val << GMAC_ST2CW12_OFFSSTRT_Pos)  /**< (GMAC_ST2CW12) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW12_OFFSSTRT_TCP_UDP       (GMAC_ST2CW12_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW12_OFFSSTRT_Pos)  /**< (GMAC_ST2CW12) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW12_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW12) Register MASK  (Use GMAC_ST2CW12_Msk instead)  */
#define GMAC_ST2CW12_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW12) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW03 : (GMAC Offset: 0x718) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 3) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW03_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW03_OFFSET                 (0x718)                                       /**<  (GMAC_ST2CW03) Screening Type 2 Compare Word 0 Register  (index = 3)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW03_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW03) Mask Value Position */
#define GMAC_ST2CW03_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW03_MASKVAL_Pos)          /**< (GMAC_ST2CW03) Mask Value Mask */
#define GMAC_ST2CW03_MASKVAL(value)         (GMAC_ST2CW03_MASKVAL_Msk & ((value) << GMAC_ST2CW03_MASKVAL_Pos))
#define GMAC_ST2CW03_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW03) Compare Value Position */
#define GMAC_ST2CW03_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW03_COMPVAL_Pos)          /**< (GMAC_ST2CW03) Compare Value Mask */
#define GMAC_ST2CW03_COMPVAL(value)         (GMAC_ST2CW03_COMPVAL_Msk & ((value) << GMAC_ST2CW03_COMPVAL_Pos))
#define GMAC_ST2CW03_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW03) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW13 : (GMAC Offset: 0x71c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 3) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW13_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW13_OFFSET                 (0x71C)                                       /**<  (GMAC_ST2CW13) Screening Type 2 Compare Word 1 Register  (index = 3)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW13_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW13) Offset Value in Bytes Position */
#define GMAC_ST2CW13_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW13_OFFSVAL_Pos)            /**< (GMAC_ST2CW13) Offset Value in Bytes Mask */
#define GMAC_ST2CW13_OFFSVAL(value)         (GMAC_ST2CW13_OFFSVAL_Msk & ((value) << GMAC_ST2CW13_OFFSVAL_Pos))
#define GMAC_ST2CW13_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW13) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW13_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW13_OFFSSTRT_Pos)            /**< (GMAC_ST2CW13) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW13_OFFSSTRT(value)        (GMAC_ST2CW13_OFFSSTRT_Msk & ((value) << GMAC_ST2CW13_OFFSSTRT_Pos))
#define   GMAC_ST2CW13_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW13) Offset from the start of the frame  */
#define   GMAC_ST2CW13_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW13) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW13_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW13) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW13_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW13) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW13_OFFSSTRT_FRAMESTART    (GMAC_ST2CW13_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW13_OFFSSTRT_Pos)  /**< (GMAC_ST2CW13) Offset from the start of the frame Position  */
#define GMAC_ST2CW13_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW13_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW13_OFFSSTRT_Pos)  /**< (GMAC_ST2CW13) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW13_OFFSSTRT_IP            (GMAC_ST2CW13_OFFSSTRT_IP_Val << GMAC_ST2CW13_OFFSSTRT_Pos)  /**< (GMAC_ST2CW13) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW13_OFFSSTRT_TCP_UDP       (GMAC_ST2CW13_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW13_OFFSSTRT_Pos)  /**< (GMAC_ST2CW13) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW13_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW13) Register MASK  (Use GMAC_ST2CW13_Msk instead)  */
#define GMAC_ST2CW13_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW13) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW04 : (GMAC Offset: 0x720) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 4) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW04_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW04_OFFSET                 (0x720)                                       /**<  (GMAC_ST2CW04) Screening Type 2 Compare Word 0 Register  (index = 4)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW04_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW04) Mask Value Position */
#define GMAC_ST2CW04_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW04_MASKVAL_Pos)          /**< (GMAC_ST2CW04) Mask Value Mask */
#define GMAC_ST2CW04_MASKVAL(value)         (GMAC_ST2CW04_MASKVAL_Msk & ((value) << GMAC_ST2CW04_MASKVAL_Pos))
#define GMAC_ST2CW04_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW04) Compare Value Position */
#define GMAC_ST2CW04_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW04_COMPVAL_Pos)          /**< (GMAC_ST2CW04) Compare Value Mask */
#define GMAC_ST2CW04_COMPVAL(value)         (GMAC_ST2CW04_COMPVAL_Msk & ((value) << GMAC_ST2CW04_COMPVAL_Pos))
#define GMAC_ST2CW04_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW04) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW14 : (GMAC Offset: 0x724) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 4) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW14_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW14_OFFSET                 (0x724)                                       /**<  (GMAC_ST2CW14) Screening Type 2 Compare Word 1 Register  (index = 4)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW14_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW14) Offset Value in Bytes Position */
#define GMAC_ST2CW14_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW14_OFFSVAL_Pos)            /**< (GMAC_ST2CW14) Offset Value in Bytes Mask */
#define GMAC_ST2CW14_OFFSVAL(value)         (GMAC_ST2CW14_OFFSVAL_Msk & ((value) << GMAC_ST2CW14_OFFSVAL_Pos))
#define GMAC_ST2CW14_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW14) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW14_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW14_OFFSSTRT_Pos)            /**< (GMAC_ST2CW14) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW14_OFFSSTRT(value)        (GMAC_ST2CW14_OFFSSTRT_Msk & ((value) << GMAC_ST2CW14_OFFSSTRT_Pos))
#define   GMAC_ST2CW14_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW14) Offset from the start of the frame  */
#define   GMAC_ST2CW14_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW14) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW14_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW14) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW14_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW14) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW14_OFFSSTRT_FRAMESTART    (GMAC_ST2CW14_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW14_OFFSSTRT_Pos)  /**< (GMAC_ST2CW14) Offset from the start of the frame Position  */
#define GMAC_ST2CW14_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW14_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW14_OFFSSTRT_Pos)  /**< (GMAC_ST2CW14) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW14_OFFSSTRT_IP            (GMAC_ST2CW14_OFFSSTRT_IP_Val << GMAC_ST2CW14_OFFSSTRT_Pos)  /**< (GMAC_ST2CW14) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW14_OFFSSTRT_TCP_UDP       (GMAC_ST2CW14_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW14_OFFSSTRT_Pos)  /**< (GMAC_ST2CW14) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW14_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW14) Register MASK  (Use GMAC_ST2CW14_Msk instead)  */
#define GMAC_ST2CW14_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW14) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW05 : (GMAC Offset: 0x728) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 5) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW05_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW05_OFFSET                 (0x728)                                       /**<  (GMAC_ST2CW05) Screening Type 2 Compare Word 0 Register  (index = 5)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW05_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW05) Mask Value Position */
#define GMAC_ST2CW05_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW05_MASKVAL_Pos)          /**< (GMAC_ST2CW05) Mask Value Mask */
#define GMAC_ST2CW05_MASKVAL(value)         (GMAC_ST2CW05_MASKVAL_Msk & ((value) << GMAC_ST2CW05_MASKVAL_Pos))
#define GMAC_ST2CW05_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW05) Compare Value Position */
#define GMAC_ST2CW05_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW05_COMPVAL_Pos)          /**< (GMAC_ST2CW05) Compare Value Mask */
#define GMAC_ST2CW05_COMPVAL(value)         (GMAC_ST2CW05_COMPVAL_Msk & ((value) << GMAC_ST2CW05_COMPVAL_Pos))
#define GMAC_ST2CW05_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW05) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW15 : (GMAC Offset: 0x72c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 5) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW15_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW15_OFFSET                 (0x72C)                                       /**<  (GMAC_ST2CW15) Screening Type 2 Compare Word 1 Register  (index = 5)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW15_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW15) Offset Value in Bytes Position */
#define GMAC_ST2CW15_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW15_OFFSVAL_Pos)            /**< (GMAC_ST2CW15) Offset Value in Bytes Mask */
#define GMAC_ST2CW15_OFFSVAL(value)         (GMAC_ST2CW15_OFFSVAL_Msk & ((value) << GMAC_ST2CW15_OFFSVAL_Pos))
#define GMAC_ST2CW15_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW15) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW15_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW15_OFFSSTRT_Pos)            /**< (GMAC_ST2CW15) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW15_OFFSSTRT(value)        (GMAC_ST2CW15_OFFSSTRT_Msk & ((value) << GMAC_ST2CW15_OFFSSTRT_Pos))
#define   GMAC_ST2CW15_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW15) Offset from the start of the frame  */
#define   GMAC_ST2CW15_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW15) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW15_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW15) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW15_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW15) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW15_OFFSSTRT_FRAMESTART    (GMAC_ST2CW15_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW15_OFFSSTRT_Pos)  /**< (GMAC_ST2CW15) Offset from the start of the frame Position  */
#define GMAC_ST2CW15_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW15_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW15_OFFSSTRT_Pos)  /**< (GMAC_ST2CW15) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW15_OFFSSTRT_IP            (GMAC_ST2CW15_OFFSSTRT_IP_Val << GMAC_ST2CW15_OFFSSTRT_Pos)  /**< (GMAC_ST2CW15) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW15_OFFSSTRT_TCP_UDP       (GMAC_ST2CW15_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW15_OFFSSTRT_Pos)  /**< (GMAC_ST2CW15) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW15_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW15) Register MASK  (Use GMAC_ST2CW15_Msk instead)  */
#define GMAC_ST2CW15_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW15) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW06 : (GMAC Offset: 0x730) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 6) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW06_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW06_OFFSET                 (0x730)                                       /**<  (GMAC_ST2CW06) Screening Type 2 Compare Word 0 Register  (index = 6)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW06_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW06) Mask Value Position */
#define GMAC_ST2CW06_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW06_MASKVAL_Pos)          /**< (GMAC_ST2CW06) Mask Value Mask */
#define GMAC_ST2CW06_MASKVAL(value)         (GMAC_ST2CW06_MASKVAL_Msk & ((value) << GMAC_ST2CW06_MASKVAL_Pos))
#define GMAC_ST2CW06_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW06) Compare Value Position */
#define GMAC_ST2CW06_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW06_COMPVAL_Pos)          /**< (GMAC_ST2CW06) Compare Value Mask */
#define GMAC_ST2CW06_COMPVAL(value)         (GMAC_ST2CW06_COMPVAL_Msk & ((value) << GMAC_ST2CW06_COMPVAL_Pos))
#define GMAC_ST2CW06_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW06) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW16 : (GMAC Offset: 0x734) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 6) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW16_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW16_OFFSET                 (0x734)                                       /**<  (GMAC_ST2CW16) Screening Type 2 Compare Word 1 Register  (index = 6)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW16_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW16) Offset Value in Bytes Position */
#define GMAC_ST2CW16_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW16_OFFSVAL_Pos)            /**< (GMAC_ST2CW16) Offset Value in Bytes Mask */
#define GMAC_ST2CW16_OFFSVAL(value)         (GMAC_ST2CW16_OFFSVAL_Msk & ((value) << GMAC_ST2CW16_OFFSVAL_Pos))
#define GMAC_ST2CW16_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW16) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW16_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW16_OFFSSTRT_Pos)            /**< (GMAC_ST2CW16) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW16_OFFSSTRT(value)        (GMAC_ST2CW16_OFFSSTRT_Msk & ((value) << GMAC_ST2CW16_OFFSSTRT_Pos))
#define   GMAC_ST2CW16_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW16) Offset from the start of the frame  */
#define   GMAC_ST2CW16_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW16) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW16_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW16) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW16_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW16) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW16_OFFSSTRT_FRAMESTART    (GMAC_ST2CW16_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW16_OFFSSTRT_Pos)  /**< (GMAC_ST2CW16) Offset from the start of the frame Position  */
#define GMAC_ST2CW16_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW16_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW16_OFFSSTRT_Pos)  /**< (GMAC_ST2CW16) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW16_OFFSSTRT_IP            (GMAC_ST2CW16_OFFSSTRT_IP_Val << GMAC_ST2CW16_OFFSSTRT_Pos)  /**< (GMAC_ST2CW16) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW16_OFFSSTRT_TCP_UDP       (GMAC_ST2CW16_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW16_OFFSSTRT_Pos)  /**< (GMAC_ST2CW16) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW16_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW16) Register MASK  (Use GMAC_ST2CW16_Msk instead)  */
#define GMAC_ST2CW16_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW16) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW07 : (GMAC Offset: 0x738) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 7) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW07_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW07_OFFSET                 (0x738)                                       /**<  (GMAC_ST2CW07) Screening Type 2 Compare Word 0 Register  (index = 7)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW07_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW07) Mask Value Position */
#define GMAC_ST2CW07_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW07_MASKVAL_Pos)          /**< (GMAC_ST2CW07) Mask Value Mask */
#define GMAC_ST2CW07_MASKVAL(value)         (GMAC_ST2CW07_MASKVAL_Msk & ((value) << GMAC_ST2CW07_MASKVAL_Pos))
#define GMAC_ST2CW07_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW07) Compare Value Position */
#define GMAC_ST2CW07_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW07_COMPVAL_Pos)          /**< (GMAC_ST2CW07) Compare Value Mask */
#define GMAC_ST2CW07_COMPVAL(value)         (GMAC_ST2CW07_COMPVAL_Msk & ((value) << GMAC_ST2CW07_COMPVAL_Pos))
#define GMAC_ST2CW07_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW07) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW17 : (GMAC Offset: 0x73c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 7) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW17_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW17_OFFSET                 (0x73C)                                       /**<  (GMAC_ST2CW17) Screening Type 2 Compare Word 1 Register  (index = 7)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW17_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW17) Offset Value in Bytes Position */
#define GMAC_ST2CW17_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW17_OFFSVAL_Pos)            /**< (GMAC_ST2CW17) Offset Value in Bytes Mask */
#define GMAC_ST2CW17_OFFSVAL(value)         (GMAC_ST2CW17_OFFSVAL_Msk & ((value) << GMAC_ST2CW17_OFFSVAL_Pos))
#define GMAC_ST2CW17_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW17) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW17_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW17_OFFSSTRT_Pos)            /**< (GMAC_ST2CW17) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW17_OFFSSTRT(value)        (GMAC_ST2CW17_OFFSSTRT_Msk & ((value) << GMAC_ST2CW17_OFFSSTRT_Pos))
#define   GMAC_ST2CW17_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW17) Offset from the start of the frame  */
#define   GMAC_ST2CW17_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW17) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW17_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW17) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW17_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW17) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW17_OFFSSTRT_FRAMESTART    (GMAC_ST2CW17_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW17_OFFSSTRT_Pos)  /**< (GMAC_ST2CW17) Offset from the start of the frame Position  */
#define GMAC_ST2CW17_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW17_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW17_OFFSSTRT_Pos)  /**< (GMAC_ST2CW17) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW17_OFFSSTRT_IP            (GMAC_ST2CW17_OFFSSTRT_IP_Val << GMAC_ST2CW17_OFFSSTRT_Pos)  /**< (GMAC_ST2CW17) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW17_OFFSSTRT_TCP_UDP       (GMAC_ST2CW17_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW17_OFFSSTRT_Pos)  /**< (GMAC_ST2CW17) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW17_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW17) Register MASK  (Use GMAC_ST2CW17_Msk instead)  */
#define GMAC_ST2CW17_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW17) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW08 : (GMAC Offset: 0x740) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 8) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW08_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW08_OFFSET                 (0x740)                                       /**<  (GMAC_ST2CW08) Screening Type 2 Compare Word 0 Register  (index = 8)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW08_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW08) Mask Value Position */
#define GMAC_ST2CW08_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW08_MASKVAL_Pos)          /**< (GMAC_ST2CW08) Mask Value Mask */
#define GMAC_ST2CW08_MASKVAL(value)         (GMAC_ST2CW08_MASKVAL_Msk & ((value) << GMAC_ST2CW08_MASKVAL_Pos))
#define GMAC_ST2CW08_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW08) Compare Value Position */
#define GMAC_ST2CW08_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW08_COMPVAL_Pos)          /**< (GMAC_ST2CW08) Compare Value Mask */
#define GMAC_ST2CW08_COMPVAL(value)         (GMAC_ST2CW08_COMPVAL_Msk & ((value) << GMAC_ST2CW08_COMPVAL_Pos))
#define GMAC_ST2CW08_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW08) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW18 : (GMAC Offset: 0x744) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 8) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW18_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW18_OFFSET                 (0x744)                                       /**<  (GMAC_ST2CW18) Screening Type 2 Compare Word 1 Register  (index = 8)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW18_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW18) Offset Value in Bytes Position */
#define GMAC_ST2CW18_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW18_OFFSVAL_Pos)            /**< (GMAC_ST2CW18) Offset Value in Bytes Mask */
#define GMAC_ST2CW18_OFFSVAL(value)         (GMAC_ST2CW18_OFFSVAL_Msk & ((value) << GMAC_ST2CW18_OFFSVAL_Pos))
#define GMAC_ST2CW18_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW18) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW18_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW18_OFFSSTRT_Pos)            /**< (GMAC_ST2CW18) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW18_OFFSSTRT(value)        (GMAC_ST2CW18_OFFSSTRT_Msk & ((value) << GMAC_ST2CW18_OFFSSTRT_Pos))
#define   GMAC_ST2CW18_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW18) Offset from the start of the frame  */
#define   GMAC_ST2CW18_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW18) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW18_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW18) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW18_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW18) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW18_OFFSSTRT_FRAMESTART    (GMAC_ST2CW18_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW18_OFFSSTRT_Pos)  /**< (GMAC_ST2CW18) Offset from the start of the frame Position  */
#define GMAC_ST2CW18_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW18_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW18_OFFSSTRT_Pos)  /**< (GMAC_ST2CW18) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW18_OFFSSTRT_IP            (GMAC_ST2CW18_OFFSSTRT_IP_Val << GMAC_ST2CW18_OFFSSTRT_Pos)  /**< (GMAC_ST2CW18) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW18_OFFSSTRT_TCP_UDP       (GMAC_ST2CW18_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW18_OFFSSTRT_Pos)  /**< (GMAC_ST2CW18) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW18_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW18) Register MASK  (Use GMAC_ST2CW18_Msk instead)  */
#define GMAC_ST2CW18_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW18) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW09 : (GMAC Offset: 0x748) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 9) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW09_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW09_OFFSET                 (0x748)                                       /**<  (GMAC_ST2CW09) Screening Type 2 Compare Word 0 Register  (index = 9)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW09_MASKVAL_Pos            0                                              /**< (GMAC_ST2CW09) Mask Value Position */
#define GMAC_ST2CW09_MASKVAL_Msk            (0xFFFFU << GMAC_ST2CW09_MASKVAL_Pos)          /**< (GMAC_ST2CW09) Mask Value Mask */
#define GMAC_ST2CW09_MASKVAL(value)         (GMAC_ST2CW09_MASKVAL_Msk & ((value) << GMAC_ST2CW09_MASKVAL_Pos))
#define GMAC_ST2CW09_COMPVAL_Pos            16                                             /**< (GMAC_ST2CW09) Compare Value Position */
#define GMAC_ST2CW09_COMPVAL_Msk            (0xFFFFU << GMAC_ST2CW09_COMPVAL_Pos)          /**< (GMAC_ST2CW09) Compare Value Mask */
#define GMAC_ST2CW09_COMPVAL(value)         (GMAC_ST2CW09_COMPVAL_Msk & ((value) << GMAC_ST2CW09_COMPVAL_Pos))
#define GMAC_ST2CW09_Msk                    (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW09) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW19 : (GMAC Offset: 0x74c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 9) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW19_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW19_OFFSET                 (0x74C)                                       /**<  (GMAC_ST2CW19) Screening Type 2 Compare Word 1 Register  (index = 9)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW19_OFFSVAL_Pos            0                                              /**< (GMAC_ST2CW19) Offset Value in Bytes Position */
#define GMAC_ST2CW19_OFFSVAL_Msk            (0x7FU << GMAC_ST2CW19_OFFSVAL_Pos)            /**< (GMAC_ST2CW19) Offset Value in Bytes Mask */
#define GMAC_ST2CW19_OFFSVAL(value)         (GMAC_ST2CW19_OFFSVAL_Msk & ((value) << GMAC_ST2CW19_OFFSVAL_Pos))
#define GMAC_ST2CW19_OFFSSTRT_Pos           7                                              /**< (GMAC_ST2CW19) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW19_OFFSSTRT_Msk           (0x3U << GMAC_ST2CW19_OFFSSTRT_Pos)            /**< (GMAC_ST2CW19) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW19_OFFSSTRT(value)        (GMAC_ST2CW19_OFFSSTRT_Msk & ((value) << GMAC_ST2CW19_OFFSSTRT_Pos))
#define   GMAC_ST2CW19_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW19) Offset from the start of the frame  */
#define   GMAC_ST2CW19_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW19) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW19_OFFSSTRT_IP_Val      (0x2U)                                         /**< (GMAC_ST2CW19) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW19_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW19) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW19_OFFSSTRT_FRAMESTART    (GMAC_ST2CW19_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW19_OFFSSTRT_Pos)  /**< (GMAC_ST2CW19) Offset from the start of the frame Position  */
#define GMAC_ST2CW19_OFFSSTRT_ETHERTYPE     (GMAC_ST2CW19_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW19_OFFSSTRT_Pos)  /**< (GMAC_ST2CW19) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW19_OFFSSTRT_IP            (GMAC_ST2CW19_OFFSSTRT_IP_Val << GMAC_ST2CW19_OFFSSTRT_Pos)  /**< (GMAC_ST2CW19) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW19_OFFSSTRT_TCP_UDP       (GMAC_ST2CW19_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW19_OFFSSTRT_Pos)  /**< (GMAC_ST2CW19) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW19_MASK                   (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW19) Register MASK  (Use GMAC_ST2CW19_Msk instead)  */
#define GMAC_ST2CW19_Msk                    (0x1FFU)                                       /**< (GMAC_ST2CW19) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW010 : (GMAC Offset: 0x750) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 10) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW010_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW010_OFFSET                (0x750)                                       /**<  (GMAC_ST2CW010) Screening Type 2 Compare Word 0 Register  (index = 10)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW010_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW010) Mask Value Position */
#define GMAC_ST2CW010_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW010_MASKVAL_Pos)         /**< (GMAC_ST2CW010) Mask Value Mask */
#define GMAC_ST2CW010_MASKVAL(value)        (GMAC_ST2CW010_MASKVAL_Msk & ((value) << GMAC_ST2CW010_MASKVAL_Pos))
#define GMAC_ST2CW010_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW010) Compare Value Position */
#define GMAC_ST2CW010_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW010_COMPVAL_Pos)         /**< (GMAC_ST2CW010) Compare Value Mask */
#define GMAC_ST2CW010_COMPVAL(value)        (GMAC_ST2CW010_COMPVAL_Msk & ((value) << GMAC_ST2CW010_COMPVAL_Pos))
#define GMAC_ST2CW010_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW010) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW110 : (GMAC Offset: 0x754) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 10) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW110_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW110_OFFSET                (0x754)                                       /**<  (GMAC_ST2CW110) Screening Type 2 Compare Word 1 Register  (index = 10)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW110_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW110) Offset Value in Bytes Position */
#define GMAC_ST2CW110_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW110_OFFSVAL_Pos)           /**< (GMAC_ST2CW110) Offset Value in Bytes Mask */
#define GMAC_ST2CW110_OFFSVAL(value)        (GMAC_ST2CW110_OFFSVAL_Msk & ((value) << GMAC_ST2CW110_OFFSVAL_Pos))
#define GMAC_ST2CW110_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW110) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW110_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW110_OFFSSTRT_Pos)           /**< (GMAC_ST2CW110) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW110_OFFSSTRT(value)       (GMAC_ST2CW110_OFFSSTRT_Msk & ((value) << GMAC_ST2CW110_OFFSSTRT_Pos))
#define   GMAC_ST2CW110_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW110) Offset from the start of the frame  */
#define   GMAC_ST2CW110_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW110) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW110_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW110) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW110_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW110) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW110_OFFSSTRT_FRAMESTART   (GMAC_ST2CW110_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW110_OFFSSTRT_Pos)  /**< (GMAC_ST2CW110) Offset from the start of the frame Position  */
#define GMAC_ST2CW110_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW110_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW110_OFFSSTRT_Pos)  /**< (GMAC_ST2CW110) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW110_OFFSSTRT_IP           (GMAC_ST2CW110_OFFSSTRT_IP_Val << GMAC_ST2CW110_OFFSSTRT_Pos)  /**< (GMAC_ST2CW110) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW110_OFFSSTRT_TCP_UDP      (GMAC_ST2CW110_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW110_OFFSSTRT_Pos)  /**< (GMAC_ST2CW110) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW110_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW110) Register MASK  (Use GMAC_ST2CW110_Msk instead)  */
#define GMAC_ST2CW110_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW110) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW011 : (GMAC Offset: 0x758) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 11) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW011_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW011_OFFSET                (0x758)                                       /**<  (GMAC_ST2CW011) Screening Type 2 Compare Word 0 Register  (index = 11)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW011_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW011) Mask Value Position */
#define GMAC_ST2CW011_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW011_MASKVAL_Pos)         /**< (GMAC_ST2CW011) Mask Value Mask */
#define GMAC_ST2CW011_MASKVAL(value)        (GMAC_ST2CW011_MASKVAL_Msk & ((value) << GMAC_ST2CW011_MASKVAL_Pos))
#define GMAC_ST2CW011_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW011) Compare Value Position */
#define GMAC_ST2CW011_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW011_COMPVAL_Pos)         /**< (GMAC_ST2CW011) Compare Value Mask */
#define GMAC_ST2CW011_COMPVAL(value)        (GMAC_ST2CW011_COMPVAL_Msk & ((value) << GMAC_ST2CW011_COMPVAL_Pos))
#define GMAC_ST2CW011_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW011) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW111 : (GMAC Offset: 0x75c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 11) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW111_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW111_OFFSET                (0x75C)                                       /**<  (GMAC_ST2CW111) Screening Type 2 Compare Word 1 Register  (index = 11)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW111_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW111) Offset Value in Bytes Position */
#define GMAC_ST2CW111_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW111_OFFSVAL_Pos)           /**< (GMAC_ST2CW111) Offset Value in Bytes Mask */
#define GMAC_ST2CW111_OFFSVAL(value)        (GMAC_ST2CW111_OFFSVAL_Msk & ((value) << GMAC_ST2CW111_OFFSVAL_Pos))
#define GMAC_ST2CW111_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW111) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW111_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW111_OFFSSTRT_Pos)           /**< (GMAC_ST2CW111) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW111_OFFSSTRT(value)       (GMAC_ST2CW111_OFFSSTRT_Msk & ((value) << GMAC_ST2CW111_OFFSSTRT_Pos))
#define   GMAC_ST2CW111_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW111) Offset from the start of the frame  */
#define   GMAC_ST2CW111_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW111) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW111_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW111) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW111_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW111) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW111_OFFSSTRT_FRAMESTART   (GMAC_ST2CW111_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW111_OFFSSTRT_Pos)  /**< (GMAC_ST2CW111) Offset from the start of the frame Position  */
#define GMAC_ST2CW111_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW111_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW111_OFFSSTRT_Pos)  /**< (GMAC_ST2CW111) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW111_OFFSSTRT_IP           (GMAC_ST2CW111_OFFSSTRT_IP_Val << GMAC_ST2CW111_OFFSSTRT_Pos)  /**< (GMAC_ST2CW111) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW111_OFFSSTRT_TCP_UDP      (GMAC_ST2CW111_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW111_OFFSSTRT_Pos)  /**< (GMAC_ST2CW111) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW111_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW111) Register MASK  (Use GMAC_ST2CW111_Msk instead)  */
#define GMAC_ST2CW111_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW111) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW012 : (GMAC Offset: 0x760) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 12) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW012_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW012_OFFSET                (0x760)                                       /**<  (GMAC_ST2CW012) Screening Type 2 Compare Word 0 Register  (index = 12)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW012_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW012) Mask Value Position */
#define GMAC_ST2CW012_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW012_MASKVAL_Pos)         /**< (GMAC_ST2CW012) Mask Value Mask */
#define GMAC_ST2CW012_MASKVAL(value)        (GMAC_ST2CW012_MASKVAL_Msk & ((value) << GMAC_ST2CW012_MASKVAL_Pos))
#define GMAC_ST2CW012_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW012) Compare Value Position */
#define GMAC_ST2CW012_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW012_COMPVAL_Pos)         /**< (GMAC_ST2CW012) Compare Value Mask */
#define GMAC_ST2CW012_COMPVAL(value)        (GMAC_ST2CW012_COMPVAL_Msk & ((value) << GMAC_ST2CW012_COMPVAL_Pos))
#define GMAC_ST2CW012_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW012) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW112 : (GMAC Offset: 0x764) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 12) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW112_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW112_OFFSET                (0x764)                                       /**<  (GMAC_ST2CW112) Screening Type 2 Compare Word 1 Register  (index = 12)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW112_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW112) Offset Value in Bytes Position */
#define GMAC_ST2CW112_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW112_OFFSVAL_Pos)           /**< (GMAC_ST2CW112) Offset Value in Bytes Mask */
#define GMAC_ST2CW112_OFFSVAL(value)        (GMAC_ST2CW112_OFFSVAL_Msk & ((value) << GMAC_ST2CW112_OFFSVAL_Pos))
#define GMAC_ST2CW112_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW112) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW112_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW112_OFFSSTRT_Pos)           /**< (GMAC_ST2CW112) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW112_OFFSSTRT(value)       (GMAC_ST2CW112_OFFSSTRT_Msk & ((value) << GMAC_ST2CW112_OFFSSTRT_Pos))
#define   GMAC_ST2CW112_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW112) Offset from the start of the frame  */
#define   GMAC_ST2CW112_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW112) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW112_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW112) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW112_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW112) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW112_OFFSSTRT_FRAMESTART   (GMAC_ST2CW112_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW112_OFFSSTRT_Pos)  /**< (GMAC_ST2CW112) Offset from the start of the frame Position  */
#define GMAC_ST2CW112_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW112_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW112_OFFSSTRT_Pos)  /**< (GMAC_ST2CW112) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW112_OFFSSTRT_IP           (GMAC_ST2CW112_OFFSSTRT_IP_Val << GMAC_ST2CW112_OFFSSTRT_Pos)  /**< (GMAC_ST2CW112) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW112_OFFSSTRT_TCP_UDP      (GMAC_ST2CW112_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW112_OFFSSTRT_Pos)  /**< (GMAC_ST2CW112) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW112_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW112) Register MASK  (Use GMAC_ST2CW112_Msk instead)  */
#define GMAC_ST2CW112_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW112) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW013 : (GMAC Offset: 0x768) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 13) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW013_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW013_OFFSET                (0x768)                                       /**<  (GMAC_ST2CW013) Screening Type 2 Compare Word 0 Register  (index = 13)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW013_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW013) Mask Value Position */
#define GMAC_ST2CW013_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW013_MASKVAL_Pos)         /**< (GMAC_ST2CW013) Mask Value Mask */
#define GMAC_ST2CW013_MASKVAL(value)        (GMAC_ST2CW013_MASKVAL_Msk & ((value) << GMAC_ST2CW013_MASKVAL_Pos))
#define GMAC_ST2CW013_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW013) Compare Value Position */
#define GMAC_ST2CW013_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW013_COMPVAL_Pos)         /**< (GMAC_ST2CW013) Compare Value Mask */
#define GMAC_ST2CW013_COMPVAL(value)        (GMAC_ST2CW013_COMPVAL_Msk & ((value) << GMAC_ST2CW013_COMPVAL_Pos))
#define GMAC_ST2CW013_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW013) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW113 : (GMAC Offset: 0x76c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 13) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW113_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW113_OFFSET                (0x76C)                                       /**<  (GMAC_ST2CW113) Screening Type 2 Compare Word 1 Register  (index = 13)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW113_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW113) Offset Value in Bytes Position */
#define GMAC_ST2CW113_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW113_OFFSVAL_Pos)           /**< (GMAC_ST2CW113) Offset Value in Bytes Mask */
#define GMAC_ST2CW113_OFFSVAL(value)        (GMAC_ST2CW113_OFFSVAL_Msk & ((value) << GMAC_ST2CW113_OFFSVAL_Pos))
#define GMAC_ST2CW113_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW113) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW113_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW113_OFFSSTRT_Pos)           /**< (GMAC_ST2CW113) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW113_OFFSSTRT(value)       (GMAC_ST2CW113_OFFSSTRT_Msk & ((value) << GMAC_ST2CW113_OFFSSTRT_Pos))
#define   GMAC_ST2CW113_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW113) Offset from the start of the frame  */
#define   GMAC_ST2CW113_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW113) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW113_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW113) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW113_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW113) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW113_OFFSSTRT_FRAMESTART   (GMAC_ST2CW113_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW113_OFFSSTRT_Pos)  /**< (GMAC_ST2CW113) Offset from the start of the frame Position  */
#define GMAC_ST2CW113_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW113_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW113_OFFSSTRT_Pos)  /**< (GMAC_ST2CW113) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW113_OFFSSTRT_IP           (GMAC_ST2CW113_OFFSSTRT_IP_Val << GMAC_ST2CW113_OFFSSTRT_Pos)  /**< (GMAC_ST2CW113) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW113_OFFSSTRT_TCP_UDP      (GMAC_ST2CW113_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW113_OFFSSTRT_Pos)  /**< (GMAC_ST2CW113) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW113_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW113) Register MASK  (Use GMAC_ST2CW113_Msk instead)  */
#define GMAC_ST2CW113_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW113) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW014 : (GMAC Offset: 0x770) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 14) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW014_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW014_OFFSET                (0x770)                                       /**<  (GMAC_ST2CW014) Screening Type 2 Compare Word 0 Register  (index = 14)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW014_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW014) Mask Value Position */
#define GMAC_ST2CW014_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW014_MASKVAL_Pos)         /**< (GMAC_ST2CW014) Mask Value Mask */
#define GMAC_ST2CW014_MASKVAL(value)        (GMAC_ST2CW014_MASKVAL_Msk & ((value) << GMAC_ST2CW014_MASKVAL_Pos))
#define GMAC_ST2CW014_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW014) Compare Value Position */
#define GMAC_ST2CW014_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW014_COMPVAL_Pos)         /**< (GMAC_ST2CW014) Compare Value Mask */
#define GMAC_ST2CW014_COMPVAL(value)        (GMAC_ST2CW014_COMPVAL_Msk & ((value) << GMAC_ST2CW014_COMPVAL_Pos))
#define GMAC_ST2CW014_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW014) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW114 : (GMAC Offset: 0x774) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 14) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW114_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW114_OFFSET                (0x774)                                       /**<  (GMAC_ST2CW114) Screening Type 2 Compare Word 1 Register  (index = 14)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW114_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW114) Offset Value in Bytes Position */
#define GMAC_ST2CW114_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW114_OFFSVAL_Pos)           /**< (GMAC_ST2CW114) Offset Value in Bytes Mask */
#define GMAC_ST2CW114_OFFSVAL(value)        (GMAC_ST2CW114_OFFSVAL_Msk & ((value) << GMAC_ST2CW114_OFFSVAL_Pos))
#define GMAC_ST2CW114_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW114) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW114_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW114_OFFSSTRT_Pos)           /**< (GMAC_ST2CW114) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW114_OFFSSTRT(value)       (GMAC_ST2CW114_OFFSSTRT_Msk & ((value) << GMAC_ST2CW114_OFFSSTRT_Pos))
#define   GMAC_ST2CW114_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW114) Offset from the start of the frame  */
#define   GMAC_ST2CW114_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW114) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW114_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW114) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW114_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW114) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW114_OFFSSTRT_FRAMESTART   (GMAC_ST2CW114_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW114_OFFSSTRT_Pos)  /**< (GMAC_ST2CW114) Offset from the start of the frame Position  */
#define GMAC_ST2CW114_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW114_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW114_OFFSSTRT_Pos)  /**< (GMAC_ST2CW114) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW114_OFFSSTRT_IP           (GMAC_ST2CW114_OFFSSTRT_IP_Val << GMAC_ST2CW114_OFFSSTRT_Pos)  /**< (GMAC_ST2CW114) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW114_OFFSSTRT_TCP_UDP      (GMAC_ST2CW114_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW114_OFFSSTRT_Pos)  /**< (GMAC_ST2CW114) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW114_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW114) Register MASK  (Use GMAC_ST2CW114_Msk instead)  */
#define GMAC_ST2CW114_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW114) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW015 : (GMAC Offset: 0x778) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 15) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW015_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW015_OFFSET                (0x778)                                       /**<  (GMAC_ST2CW015) Screening Type 2 Compare Word 0 Register  (index = 15)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW015_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW015) Mask Value Position */
#define GMAC_ST2CW015_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW015_MASKVAL_Pos)         /**< (GMAC_ST2CW015) Mask Value Mask */
#define GMAC_ST2CW015_MASKVAL(value)        (GMAC_ST2CW015_MASKVAL_Msk & ((value) << GMAC_ST2CW015_MASKVAL_Pos))
#define GMAC_ST2CW015_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW015) Compare Value Position */
#define GMAC_ST2CW015_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW015_COMPVAL_Pos)         /**< (GMAC_ST2CW015) Compare Value Mask */
#define GMAC_ST2CW015_COMPVAL(value)        (GMAC_ST2CW015_COMPVAL_Msk & ((value) << GMAC_ST2CW015_COMPVAL_Pos))
#define GMAC_ST2CW015_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW015) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW115 : (GMAC Offset: 0x77c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 15) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW115_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW115_OFFSET                (0x77C)                                       /**<  (GMAC_ST2CW115) Screening Type 2 Compare Word 1 Register  (index = 15)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW115_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW115) Offset Value in Bytes Position */
#define GMAC_ST2CW115_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW115_OFFSVAL_Pos)           /**< (GMAC_ST2CW115) Offset Value in Bytes Mask */
#define GMAC_ST2CW115_OFFSVAL(value)        (GMAC_ST2CW115_OFFSVAL_Msk & ((value) << GMAC_ST2CW115_OFFSVAL_Pos))
#define GMAC_ST2CW115_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW115) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW115_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW115_OFFSSTRT_Pos)           /**< (GMAC_ST2CW115) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW115_OFFSSTRT(value)       (GMAC_ST2CW115_OFFSSTRT_Msk & ((value) << GMAC_ST2CW115_OFFSSTRT_Pos))
#define   GMAC_ST2CW115_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW115) Offset from the start of the frame  */
#define   GMAC_ST2CW115_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW115) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW115_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW115) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW115_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW115) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW115_OFFSSTRT_FRAMESTART   (GMAC_ST2CW115_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW115_OFFSSTRT_Pos)  /**< (GMAC_ST2CW115) Offset from the start of the frame Position  */
#define GMAC_ST2CW115_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW115_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW115_OFFSSTRT_Pos)  /**< (GMAC_ST2CW115) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW115_OFFSSTRT_IP           (GMAC_ST2CW115_OFFSSTRT_IP_Val << GMAC_ST2CW115_OFFSSTRT_Pos)  /**< (GMAC_ST2CW115) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW115_OFFSSTRT_TCP_UDP      (GMAC_ST2CW115_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW115_OFFSSTRT_Pos)  /**< (GMAC_ST2CW115) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW115_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW115) Register MASK  (Use GMAC_ST2CW115_Msk instead)  */
#define GMAC_ST2CW115_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW115) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW016 : (GMAC Offset: 0x780) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 16) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW016_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW016_OFFSET                (0x780)                                       /**<  (GMAC_ST2CW016) Screening Type 2 Compare Word 0 Register  (index = 16)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW016_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW016) Mask Value Position */
#define GMAC_ST2CW016_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW016_MASKVAL_Pos)         /**< (GMAC_ST2CW016) Mask Value Mask */
#define GMAC_ST2CW016_MASKVAL(value)        (GMAC_ST2CW016_MASKVAL_Msk & ((value) << GMAC_ST2CW016_MASKVAL_Pos))
#define GMAC_ST2CW016_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW016) Compare Value Position */
#define GMAC_ST2CW016_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW016_COMPVAL_Pos)         /**< (GMAC_ST2CW016) Compare Value Mask */
#define GMAC_ST2CW016_COMPVAL(value)        (GMAC_ST2CW016_COMPVAL_Msk & ((value) << GMAC_ST2CW016_COMPVAL_Pos))
#define GMAC_ST2CW016_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW016) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW116 : (GMAC Offset: 0x784) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 16) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW116_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW116_OFFSET                (0x784)                                       /**<  (GMAC_ST2CW116) Screening Type 2 Compare Word 1 Register  (index = 16)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW116_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW116) Offset Value in Bytes Position */
#define GMAC_ST2CW116_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW116_OFFSVAL_Pos)           /**< (GMAC_ST2CW116) Offset Value in Bytes Mask */
#define GMAC_ST2CW116_OFFSVAL(value)        (GMAC_ST2CW116_OFFSVAL_Msk & ((value) << GMAC_ST2CW116_OFFSVAL_Pos))
#define GMAC_ST2CW116_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW116) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW116_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW116_OFFSSTRT_Pos)           /**< (GMAC_ST2CW116) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW116_OFFSSTRT(value)       (GMAC_ST2CW116_OFFSSTRT_Msk & ((value) << GMAC_ST2CW116_OFFSSTRT_Pos))
#define   GMAC_ST2CW116_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW116) Offset from the start of the frame  */
#define   GMAC_ST2CW116_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW116) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW116_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW116) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW116_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW116) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW116_OFFSSTRT_FRAMESTART   (GMAC_ST2CW116_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW116_OFFSSTRT_Pos)  /**< (GMAC_ST2CW116) Offset from the start of the frame Position  */
#define GMAC_ST2CW116_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW116_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW116_OFFSSTRT_Pos)  /**< (GMAC_ST2CW116) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW116_OFFSSTRT_IP           (GMAC_ST2CW116_OFFSSTRT_IP_Val << GMAC_ST2CW116_OFFSSTRT_Pos)  /**< (GMAC_ST2CW116) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW116_OFFSSTRT_TCP_UDP      (GMAC_ST2CW116_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW116_OFFSSTRT_Pos)  /**< (GMAC_ST2CW116) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW116_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW116) Register MASK  (Use GMAC_ST2CW116_Msk instead)  */
#define GMAC_ST2CW116_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW116) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW017 : (GMAC Offset: 0x788) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 17) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW017_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW017_OFFSET                (0x788)                                       /**<  (GMAC_ST2CW017) Screening Type 2 Compare Word 0 Register  (index = 17)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW017_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW017) Mask Value Position */
#define GMAC_ST2CW017_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW017_MASKVAL_Pos)         /**< (GMAC_ST2CW017) Mask Value Mask */
#define GMAC_ST2CW017_MASKVAL(value)        (GMAC_ST2CW017_MASKVAL_Msk & ((value) << GMAC_ST2CW017_MASKVAL_Pos))
#define GMAC_ST2CW017_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW017) Compare Value Position */
#define GMAC_ST2CW017_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW017_COMPVAL_Pos)         /**< (GMAC_ST2CW017) Compare Value Mask */
#define GMAC_ST2CW017_COMPVAL(value)        (GMAC_ST2CW017_COMPVAL_Msk & ((value) << GMAC_ST2CW017_COMPVAL_Pos))
#define GMAC_ST2CW017_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW017) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW117 : (GMAC Offset: 0x78c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 17) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW117_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW117_OFFSET                (0x78C)                                       /**<  (GMAC_ST2CW117) Screening Type 2 Compare Word 1 Register  (index = 17)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW117_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW117) Offset Value in Bytes Position */
#define GMAC_ST2CW117_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW117_OFFSVAL_Pos)           /**< (GMAC_ST2CW117) Offset Value in Bytes Mask */
#define GMAC_ST2CW117_OFFSVAL(value)        (GMAC_ST2CW117_OFFSVAL_Msk & ((value) << GMAC_ST2CW117_OFFSVAL_Pos))
#define GMAC_ST2CW117_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW117) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW117_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW117_OFFSSTRT_Pos)           /**< (GMAC_ST2CW117) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW117_OFFSSTRT(value)       (GMAC_ST2CW117_OFFSSTRT_Msk & ((value) << GMAC_ST2CW117_OFFSSTRT_Pos))
#define   GMAC_ST2CW117_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW117) Offset from the start of the frame  */
#define   GMAC_ST2CW117_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW117) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW117_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW117) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW117_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW117) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW117_OFFSSTRT_FRAMESTART   (GMAC_ST2CW117_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW117_OFFSSTRT_Pos)  /**< (GMAC_ST2CW117) Offset from the start of the frame Position  */
#define GMAC_ST2CW117_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW117_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW117_OFFSSTRT_Pos)  /**< (GMAC_ST2CW117) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW117_OFFSSTRT_IP           (GMAC_ST2CW117_OFFSSTRT_IP_Val << GMAC_ST2CW117_OFFSSTRT_Pos)  /**< (GMAC_ST2CW117) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW117_OFFSSTRT_TCP_UDP      (GMAC_ST2CW117_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW117_OFFSSTRT_Pos)  /**< (GMAC_ST2CW117) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW117_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW117) Register MASK  (Use GMAC_ST2CW117_Msk instead)  */
#define GMAC_ST2CW117_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW117) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW018 : (GMAC Offset: 0x790) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 18) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW018_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW018_OFFSET                (0x790)                                       /**<  (GMAC_ST2CW018) Screening Type 2 Compare Word 0 Register  (index = 18)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW018_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW018) Mask Value Position */
#define GMAC_ST2CW018_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW018_MASKVAL_Pos)         /**< (GMAC_ST2CW018) Mask Value Mask */
#define GMAC_ST2CW018_MASKVAL(value)        (GMAC_ST2CW018_MASKVAL_Msk & ((value) << GMAC_ST2CW018_MASKVAL_Pos))
#define GMAC_ST2CW018_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW018) Compare Value Position */
#define GMAC_ST2CW018_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW018_COMPVAL_Pos)         /**< (GMAC_ST2CW018) Compare Value Mask */
#define GMAC_ST2CW018_COMPVAL(value)        (GMAC_ST2CW018_COMPVAL_Msk & ((value) << GMAC_ST2CW018_COMPVAL_Pos))
#define GMAC_ST2CW018_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW018) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW118 : (GMAC Offset: 0x794) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 18) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW118_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW118_OFFSET                (0x794)                                       /**<  (GMAC_ST2CW118) Screening Type 2 Compare Word 1 Register  (index = 18)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW118_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW118) Offset Value in Bytes Position */
#define GMAC_ST2CW118_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW118_OFFSVAL_Pos)           /**< (GMAC_ST2CW118) Offset Value in Bytes Mask */
#define GMAC_ST2CW118_OFFSVAL(value)        (GMAC_ST2CW118_OFFSVAL_Msk & ((value) << GMAC_ST2CW118_OFFSVAL_Pos))
#define GMAC_ST2CW118_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW118) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW118_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW118_OFFSSTRT_Pos)           /**< (GMAC_ST2CW118) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW118_OFFSSTRT(value)       (GMAC_ST2CW118_OFFSSTRT_Msk & ((value) << GMAC_ST2CW118_OFFSSTRT_Pos))
#define   GMAC_ST2CW118_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW118) Offset from the start of the frame  */
#define   GMAC_ST2CW118_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW118) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW118_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW118) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW118_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW118) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW118_OFFSSTRT_FRAMESTART   (GMAC_ST2CW118_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW118_OFFSSTRT_Pos)  /**< (GMAC_ST2CW118) Offset from the start of the frame Position  */
#define GMAC_ST2CW118_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW118_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW118_OFFSSTRT_Pos)  /**< (GMAC_ST2CW118) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW118_OFFSSTRT_IP           (GMAC_ST2CW118_OFFSSTRT_IP_Val << GMAC_ST2CW118_OFFSSTRT_Pos)  /**< (GMAC_ST2CW118) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW118_OFFSSTRT_TCP_UDP      (GMAC_ST2CW118_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW118_OFFSSTRT_Pos)  /**< (GMAC_ST2CW118) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW118_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW118) Register MASK  (Use GMAC_ST2CW118_Msk instead)  */
#define GMAC_ST2CW118_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW118) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW019 : (GMAC Offset: 0x798) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 19) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW019_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW019_OFFSET                (0x798)                                       /**<  (GMAC_ST2CW019) Screening Type 2 Compare Word 0 Register  (index = 19)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW019_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW019) Mask Value Position */
#define GMAC_ST2CW019_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW019_MASKVAL_Pos)         /**< (GMAC_ST2CW019) Mask Value Mask */
#define GMAC_ST2CW019_MASKVAL(value)        (GMAC_ST2CW019_MASKVAL_Msk & ((value) << GMAC_ST2CW019_MASKVAL_Pos))
#define GMAC_ST2CW019_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW019) Compare Value Position */
#define GMAC_ST2CW019_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW019_COMPVAL_Pos)         /**< (GMAC_ST2CW019) Compare Value Mask */
#define GMAC_ST2CW019_COMPVAL(value)        (GMAC_ST2CW019_COMPVAL_Msk & ((value) << GMAC_ST2CW019_COMPVAL_Pos))
#define GMAC_ST2CW019_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW019) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW119 : (GMAC Offset: 0x79c) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 19) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW119_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW119_OFFSET                (0x79C)                                       /**<  (GMAC_ST2CW119) Screening Type 2 Compare Word 1 Register  (index = 19)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW119_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW119) Offset Value in Bytes Position */
#define GMAC_ST2CW119_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW119_OFFSVAL_Pos)           /**< (GMAC_ST2CW119) Offset Value in Bytes Mask */
#define GMAC_ST2CW119_OFFSVAL(value)        (GMAC_ST2CW119_OFFSVAL_Msk & ((value) << GMAC_ST2CW119_OFFSVAL_Pos))
#define GMAC_ST2CW119_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW119) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW119_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW119_OFFSSTRT_Pos)           /**< (GMAC_ST2CW119) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW119_OFFSSTRT(value)       (GMAC_ST2CW119_OFFSSTRT_Msk & ((value) << GMAC_ST2CW119_OFFSSTRT_Pos))
#define   GMAC_ST2CW119_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW119) Offset from the start of the frame  */
#define   GMAC_ST2CW119_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW119) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW119_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW119) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW119_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW119) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW119_OFFSSTRT_FRAMESTART   (GMAC_ST2CW119_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW119_OFFSSTRT_Pos)  /**< (GMAC_ST2CW119) Offset from the start of the frame Position  */
#define GMAC_ST2CW119_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW119_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW119_OFFSSTRT_Pos)  /**< (GMAC_ST2CW119) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW119_OFFSSTRT_IP           (GMAC_ST2CW119_OFFSSTRT_IP_Val << GMAC_ST2CW119_OFFSSTRT_Pos)  /**< (GMAC_ST2CW119) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW119_OFFSSTRT_TCP_UDP      (GMAC_ST2CW119_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW119_OFFSSTRT_Pos)  /**< (GMAC_ST2CW119) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW119_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW119) Register MASK  (Use GMAC_ST2CW119_Msk instead)  */
#define GMAC_ST2CW119_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW119) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW020 : (GMAC Offset: 0x7a0) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 20) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW020_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW020_OFFSET                (0x7A0)                                       /**<  (GMAC_ST2CW020) Screening Type 2 Compare Word 0 Register  (index = 20)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW020_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW020) Mask Value Position */
#define GMAC_ST2CW020_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW020_MASKVAL_Pos)         /**< (GMAC_ST2CW020) Mask Value Mask */
#define GMAC_ST2CW020_MASKVAL(value)        (GMAC_ST2CW020_MASKVAL_Msk & ((value) << GMAC_ST2CW020_MASKVAL_Pos))
#define GMAC_ST2CW020_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW020) Compare Value Position */
#define GMAC_ST2CW020_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW020_COMPVAL_Pos)         /**< (GMAC_ST2CW020) Compare Value Mask */
#define GMAC_ST2CW020_COMPVAL(value)        (GMAC_ST2CW020_COMPVAL_Msk & ((value) << GMAC_ST2CW020_COMPVAL_Pos))
#define GMAC_ST2CW020_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW020) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW120 : (GMAC Offset: 0x7a4) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 20) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW120_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW120_OFFSET                (0x7A4)                                       /**<  (GMAC_ST2CW120) Screening Type 2 Compare Word 1 Register  (index = 20)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW120_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW120) Offset Value in Bytes Position */
#define GMAC_ST2CW120_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW120_OFFSVAL_Pos)           /**< (GMAC_ST2CW120) Offset Value in Bytes Mask */
#define GMAC_ST2CW120_OFFSVAL(value)        (GMAC_ST2CW120_OFFSVAL_Msk & ((value) << GMAC_ST2CW120_OFFSVAL_Pos))
#define GMAC_ST2CW120_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW120) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW120_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW120_OFFSSTRT_Pos)           /**< (GMAC_ST2CW120) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW120_OFFSSTRT(value)       (GMAC_ST2CW120_OFFSSTRT_Msk & ((value) << GMAC_ST2CW120_OFFSSTRT_Pos))
#define   GMAC_ST2CW120_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW120) Offset from the start of the frame  */
#define   GMAC_ST2CW120_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW120) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW120_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW120) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW120_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW120) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW120_OFFSSTRT_FRAMESTART   (GMAC_ST2CW120_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW120_OFFSSTRT_Pos)  /**< (GMAC_ST2CW120) Offset from the start of the frame Position  */
#define GMAC_ST2CW120_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW120_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW120_OFFSSTRT_Pos)  /**< (GMAC_ST2CW120) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW120_OFFSSTRT_IP           (GMAC_ST2CW120_OFFSSTRT_IP_Val << GMAC_ST2CW120_OFFSSTRT_Pos)  /**< (GMAC_ST2CW120) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW120_OFFSSTRT_TCP_UDP      (GMAC_ST2CW120_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW120_OFFSSTRT_Pos)  /**< (GMAC_ST2CW120) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW120_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW120) Register MASK  (Use GMAC_ST2CW120_Msk instead)  */
#define GMAC_ST2CW120_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW120) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW021 : (GMAC Offset: 0x7a8) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 21) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW021_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW021_OFFSET                (0x7A8)                                       /**<  (GMAC_ST2CW021) Screening Type 2 Compare Word 0 Register  (index = 21)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW021_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW021) Mask Value Position */
#define GMAC_ST2CW021_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW021_MASKVAL_Pos)         /**< (GMAC_ST2CW021) Mask Value Mask */
#define GMAC_ST2CW021_MASKVAL(value)        (GMAC_ST2CW021_MASKVAL_Msk & ((value) << GMAC_ST2CW021_MASKVAL_Pos))
#define GMAC_ST2CW021_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW021) Compare Value Position */
#define GMAC_ST2CW021_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW021_COMPVAL_Pos)         /**< (GMAC_ST2CW021) Compare Value Mask */
#define GMAC_ST2CW021_COMPVAL(value)        (GMAC_ST2CW021_COMPVAL_Msk & ((value) << GMAC_ST2CW021_COMPVAL_Pos))
#define GMAC_ST2CW021_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW021) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW121 : (GMAC Offset: 0x7ac) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 21) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW121_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW121_OFFSET                (0x7AC)                                       /**<  (GMAC_ST2CW121) Screening Type 2 Compare Word 1 Register  (index = 21)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW121_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW121) Offset Value in Bytes Position */
#define GMAC_ST2CW121_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW121_OFFSVAL_Pos)           /**< (GMAC_ST2CW121) Offset Value in Bytes Mask */
#define GMAC_ST2CW121_OFFSVAL(value)        (GMAC_ST2CW121_OFFSVAL_Msk & ((value) << GMAC_ST2CW121_OFFSVAL_Pos))
#define GMAC_ST2CW121_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW121) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW121_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW121_OFFSSTRT_Pos)           /**< (GMAC_ST2CW121) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW121_OFFSSTRT(value)       (GMAC_ST2CW121_OFFSSTRT_Msk & ((value) << GMAC_ST2CW121_OFFSSTRT_Pos))
#define   GMAC_ST2CW121_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW121) Offset from the start of the frame  */
#define   GMAC_ST2CW121_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW121) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW121_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW121) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW121_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW121) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW121_OFFSSTRT_FRAMESTART   (GMAC_ST2CW121_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW121_OFFSSTRT_Pos)  /**< (GMAC_ST2CW121) Offset from the start of the frame Position  */
#define GMAC_ST2CW121_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW121_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW121_OFFSSTRT_Pos)  /**< (GMAC_ST2CW121) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW121_OFFSSTRT_IP           (GMAC_ST2CW121_OFFSSTRT_IP_Val << GMAC_ST2CW121_OFFSSTRT_Pos)  /**< (GMAC_ST2CW121) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW121_OFFSSTRT_TCP_UDP      (GMAC_ST2CW121_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW121_OFFSSTRT_Pos)  /**< (GMAC_ST2CW121) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW121_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW121) Register MASK  (Use GMAC_ST2CW121_Msk instead)  */
#define GMAC_ST2CW121_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW121) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW022 : (GMAC Offset: 0x7b0) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 22) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW022_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW022_OFFSET                (0x7B0)                                       /**<  (GMAC_ST2CW022) Screening Type 2 Compare Word 0 Register  (index = 22)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW022_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW022) Mask Value Position */
#define GMAC_ST2CW022_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW022_MASKVAL_Pos)         /**< (GMAC_ST2CW022) Mask Value Mask */
#define GMAC_ST2CW022_MASKVAL(value)        (GMAC_ST2CW022_MASKVAL_Msk & ((value) << GMAC_ST2CW022_MASKVAL_Pos))
#define GMAC_ST2CW022_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW022) Compare Value Position */
#define GMAC_ST2CW022_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW022_COMPVAL_Pos)         /**< (GMAC_ST2CW022) Compare Value Mask */
#define GMAC_ST2CW022_COMPVAL(value)        (GMAC_ST2CW022_COMPVAL_Msk & ((value) << GMAC_ST2CW022_COMPVAL_Pos))
#define GMAC_ST2CW022_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW022) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW122 : (GMAC Offset: 0x7b4) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 22) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW122_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW122_OFFSET                (0x7B4)                                       /**<  (GMAC_ST2CW122) Screening Type 2 Compare Word 1 Register  (index = 22)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW122_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW122) Offset Value in Bytes Position */
#define GMAC_ST2CW122_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW122_OFFSVAL_Pos)           /**< (GMAC_ST2CW122) Offset Value in Bytes Mask */
#define GMAC_ST2CW122_OFFSVAL(value)        (GMAC_ST2CW122_OFFSVAL_Msk & ((value) << GMAC_ST2CW122_OFFSVAL_Pos))
#define GMAC_ST2CW122_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW122) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW122_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW122_OFFSSTRT_Pos)           /**< (GMAC_ST2CW122) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW122_OFFSSTRT(value)       (GMAC_ST2CW122_OFFSSTRT_Msk & ((value) << GMAC_ST2CW122_OFFSSTRT_Pos))
#define   GMAC_ST2CW122_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW122) Offset from the start of the frame  */
#define   GMAC_ST2CW122_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW122) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW122_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW122) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW122_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW122) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW122_OFFSSTRT_FRAMESTART   (GMAC_ST2CW122_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW122_OFFSSTRT_Pos)  /**< (GMAC_ST2CW122) Offset from the start of the frame Position  */
#define GMAC_ST2CW122_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW122_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW122_OFFSSTRT_Pos)  /**< (GMAC_ST2CW122) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW122_OFFSSTRT_IP           (GMAC_ST2CW122_OFFSSTRT_IP_Val << GMAC_ST2CW122_OFFSSTRT_Pos)  /**< (GMAC_ST2CW122) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW122_OFFSSTRT_TCP_UDP      (GMAC_ST2CW122_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW122_OFFSSTRT_Pos)  /**< (GMAC_ST2CW122) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW122_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW122) Register MASK  (Use GMAC_ST2CW122_Msk instead)  */
#define GMAC_ST2CW122_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW122) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW023 : (GMAC Offset: 0x7b8) (R/W 32) Screening Type 2 Compare Word 0 Register  (index = 23) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MASKVAL:16;                /**< bit:  0..15  Mask Value                               */
    uint32_t COMPVAL:16;                /**< bit: 16..31  Compare Value                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW023_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW023_OFFSET                (0x7B8)                                       /**<  (GMAC_ST2CW023) Screening Type 2 Compare Word 0 Register  (index = 23)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW023_MASKVAL_Pos           0                                              /**< (GMAC_ST2CW023) Mask Value Position */
#define GMAC_ST2CW023_MASKVAL_Msk           (0xFFFFU << GMAC_ST2CW023_MASKVAL_Pos)         /**< (GMAC_ST2CW023) Mask Value Mask */
#define GMAC_ST2CW023_MASKVAL(value)        (GMAC_ST2CW023_MASKVAL_Msk & ((value) << GMAC_ST2CW023_MASKVAL_Pos))
#define GMAC_ST2CW023_COMPVAL_Pos           16                                             /**< (GMAC_ST2CW023) Compare Value Position */
#define GMAC_ST2CW023_COMPVAL_Msk           (0xFFFFU << GMAC_ST2CW023_COMPVAL_Pos)         /**< (GMAC_ST2CW023) Compare Value Mask */
#define GMAC_ST2CW023_COMPVAL(value)        (GMAC_ST2CW023_COMPVAL_Msk & ((value) << GMAC_ST2CW023_COMPVAL_Pos))
#define GMAC_ST2CW023_Msk                   (0xFFFFFFFFU)                                  /**< (GMAC_ST2CW023) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- GMAC_ST2CW123 : (GMAC Offset: 0x7bc) (R/W 32) Screening Type 2 Compare Word 1 Register  (index = 23) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSVAL:7;                 /**< bit:   0..6  Offset Value in Bytes                    */
    uint32_t OFFSSTRT:2;                /**< bit:   7..8  Ethernet Frame Offset Start              */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} GMAC_ST2CW123_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ST2CW123_OFFSET                (0x7BC)                                       /**<  (GMAC_ST2CW123) Screening Type 2 Compare Word 1 Register  (index = 23)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define GMAC_ST2CW123_OFFSVAL_Pos           0                                              /**< (GMAC_ST2CW123) Offset Value in Bytes Position */
#define GMAC_ST2CW123_OFFSVAL_Msk           (0x7FU << GMAC_ST2CW123_OFFSVAL_Pos)           /**< (GMAC_ST2CW123) Offset Value in Bytes Mask */
#define GMAC_ST2CW123_OFFSVAL(value)        (GMAC_ST2CW123_OFFSVAL_Msk & ((value) << GMAC_ST2CW123_OFFSVAL_Pos))
#define GMAC_ST2CW123_OFFSSTRT_Pos          7                                              /**< (GMAC_ST2CW123) Ethernet Frame Offset Start Position */
#define GMAC_ST2CW123_OFFSSTRT_Msk          (0x3U << GMAC_ST2CW123_OFFSSTRT_Pos)           /**< (GMAC_ST2CW123) Ethernet Frame Offset Start Mask */
#define GMAC_ST2CW123_OFFSSTRT(value)       (GMAC_ST2CW123_OFFSSTRT_Msk & ((value) << GMAC_ST2CW123_OFFSSTRT_Pos))
#define   GMAC_ST2CW123_OFFSSTRT_FRAMESTART_Val (0x0U)                                         /**< (GMAC_ST2CW123) Offset from the start of the frame  */
#define   GMAC_ST2CW123_OFFSSTRT_ETHERTYPE_Val (0x1U)                                         /**< (GMAC_ST2CW123) Offset from the byte after the EtherType field  */
#define   GMAC_ST2CW123_OFFSSTRT_IP_Val     (0x2U)                                         /**< (GMAC_ST2CW123) Offset from the byte after the IP header field  */
#define   GMAC_ST2CW123_OFFSSTRT_TCP_UDP_Val (0x3U)                                         /**< (GMAC_ST2CW123) Offset from the byte after the TCP/UDP header field  */
#define GMAC_ST2CW123_OFFSSTRT_FRAMESTART   (GMAC_ST2CW123_OFFSSTRT_FRAMESTART_Val << GMAC_ST2CW123_OFFSSTRT_Pos)  /**< (GMAC_ST2CW123) Offset from the start of the frame Position  */
#define GMAC_ST2CW123_OFFSSTRT_ETHERTYPE    (GMAC_ST2CW123_OFFSSTRT_ETHERTYPE_Val << GMAC_ST2CW123_OFFSSTRT_Pos)  /**< (GMAC_ST2CW123) Offset from the byte after the EtherType field Position  */
#define GMAC_ST2CW123_OFFSSTRT_IP           (GMAC_ST2CW123_OFFSSTRT_IP_Val << GMAC_ST2CW123_OFFSSTRT_Pos)  /**< (GMAC_ST2CW123) Offset from the byte after the IP header field Position  */
#define GMAC_ST2CW123_OFFSSTRT_TCP_UDP      (GMAC_ST2CW123_OFFSSTRT_TCP_UDP_Val << GMAC_ST2CW123_OFFSSTRT_Pos)  /**< (GMAC_ST2CW123) Offset from the byte after the TCP/UDP header field Position  */
#define GMAC_ST2CW123_MASK                  (0x1FFU)                                       /**< \deprecated (GMAC_ST2CW123) Register MASK  (Use GMAC_ST2CW123_Msk instead)  */
#define GMAC_ST2CW123_Msk                   (0x1FFU)                                       /**< (GMAC_ST2CW123) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
#define GMACSA_NUMBER 4
/** \brief GMAC_SA hardware registers */
typedef struct {  
  __IO uint32_t GMAC_SAB;       /**< (GMAC_SA Offset: 0x00) Specific Address 1 Bottom Register */
  __IO uint32_t GMAC_SAT;       /**< (GMAC_SA Offset: 0x04) Specific Address 1 Top Register */
} GmacSa;

/** \brief GMAC hardware registers */
typedef struct {  
  __IO uint32_t GMAC_NCR;       /**< (GMAC Offset: 0x00) Network Control Register */
  __IO uint32_t GMAC_NCFGR;     /**< (GMAC Offset: 0x04) Network Configuration Register */
  __I  uint32_t GMAC_NSR;       /**< (GMAC Offset: 0x08) Network Status Register */
  __IO uint32_t GMAC_UR;        /**< (GMAC Offset: 0x0C) User Register */
  __IO uint32_t GMAC_DCFGR;     /**< (GMAC Offset: 0x10) DMA Configuration Register */
  __IO uint32_t GMAC_TSR;       /**< (GMAC Offset: 0x14) Transmit Status Register */
  __IO uint32_t GMAC_RBQB;      /**< (GMAC Offset: 0x18) Receive Buffer Queue Base Address Register */
  __IO uint32_t GMAC_TBQB;      /**< (GMAC Offset: 0x1C) Transmit Buffer Queue Base Address Register */
  __IO uint32_t GMAC_RSR;       /**< (GMAC Offset: 0x20) Receive Status Register */
  __I  uint32_t GMAC_ISR;       /**< (GMAC Offset: 0x24) Interrupt Status Register */
  __O  uint32_t GMAC_IER;       /**< (GMAC Offset: 0x28) Interrupt Enable Register */
  __O  uint32_t GMAC_IDR;       /**< (GMAC Offset: 0x2C) Interrupt Disable Register */
  __IO uint32_t GMAC_IMR;       /**< (GMAC Offset: 0x30) Interrupt Mask Register */
  __IO uint32_t GMAC_MAN;       /**< (GMAC Offset: 0x34) PHY Maintenance Register */
  __I  uint32_t GMAC_RPQ;       /**< (GMAC Offset: 0x38) Received Pause Quantum Register */
  __IO uint32_t GMAC_TPQ;       /**< (GMAC Offset: 0x3C) Transmit Pause Quantum Register */
  __IO uint32_t GMAC_TPSF;      /**< (GMAC Offset: 0x40) TX Partial Store and Forward Register */
  __IO uint32_t GMAC_RPSF;      /**< (GMAC Offset: 0x44) RX Partial Store and Forward Register */
  __IO uint32_t GMAC_RJFML;     /**< (GMAC Offset: 0x48) RX Jumbo Frame Max Length Register */
  __I  uint32_t Reserved1[13];
  __IO uint32_t GMAC_HRB;       /**< (GMAC Offset: 0x80) Hash Register Bottom */
  __IO uint32_t GMAC_HRT;       /**< (GMAC Offset: 0x84) Hash Register Top */
       GmacSa   GMAC_SA[GMACSA_NUMBER]; /**< Offset: 0x88 Specific Address 1 Bottom Register */
  __IO uint32_t GMAC_TIDM1;     /**< (GMAC Offset: 0xA8) Type ID Match 1 Register */
  __IO uint32_t GMAC_TIDM2;     /**< (GMAC Offset: 0xAC) Type ID Match 2 Register */
  __IO uint32_t GMAC_TIDM3;     /**< (GMAC Offset: 0xB0) Type ID Match 3 Register */
  __IO uint32_t GMAC_TIDM4;     /**< (GMAC Offset: 0xB4) Type ID Match 4 Register */
  __IO uint32_t GMAC_WOL;       /**< (GMAC Offset: 0xB8) Wake on LAN Register */
  __IO uint32_t GMAC_IPGS;      /**< (GMAC Offset: 0xBC) IPG Stretch Register */
  __IO uint32_t GMAC_SVLAN;     /**< (GMAC Offset: 0xC0) Stacked VLAN Register */
  __IO uint32_t GMAC_TPFCP;     /**< (GMAC Offset: 0xC4) Transmit PFC Pause Register */
  __IO uint32_t GMAC_SAMB1;     /**< (GMAC Offset: 0xC8) Specific Address 1 Mask Bottom Register */
  __IO uint32_t GMAC_SAMT1;     /**< (GMAC Offset: 0xCC) Specific Address 1 Mask Top Register */
  __I  uint32_t Reserved2[3];
  __IO uint32_t GMAC_NSC;       /**< (GMAC Offset: 0xDC) 1588 Timer Nanosecond Comparison Register */
  __IO uint32_t GMAC_SCL;       /**< (GMAC Offset: 0xE0) 1588 Timer Second Comparison Low Register */
  __IO uint32_t GMAC_SCH;       /**< (GMAC Offset: 0xE4) 1588 Timer Second Comparison High Register */
  __I  uint32_t GMAC_EFTSH;     /**< (GMAC Offset: 0xE8) PTP Event Frame Transmitted Seconds High Register */
  __I  uint32_t GMAC_EFRSH;     /**< (GMAC Offset: 0xEC) PTP Event Frame Received Seconds High Register */
  __I  uint32_t GMAC_PEFTSH;    /**< (GMAC Offset: 0xF0) PTP Peer Event Frame Transmitted Seconds High Register */
  __I  uint32_t GMAC_PEFRSH;    /**< (GMAC Offset: 0xF4) PTP Peer Event Frame Received Seconds High Register */
  __I  uint32_t Reserved3[2];
  __I  uint32_t GMAC_OTLO;      /**< (GMAC Offset: 0x100) Octets Transmitted Low Register */
  __I  uint32_t GMAC_OTHI;      /**< (GMAC Offset: 0x104) Octets Transmitted High Register */
  __I  uint32_t GMAC_FT;        /**< (GMAC Offset: 0x108) Frames Transmitted Register */
  __I  uint32_t GMAC_BCFT;      /**< (GMAC Offset: 0x10C) Broadcast Frames Transmitted Register */
  __I  uint32_t GMAC_MFT;       /**< (GMAC Offset: 0x110) Multicast Frames Transmitted Register */
  __I  uint32_t GMAC_PFT;       /**< (GMAC Offset: 0x114) Pause Frames Transmitted Register */
  __I  uint32_t GMAC_BFT64;     /**< (GMAC Offset: 0x118) 64 Byte Frames Transmitted Register */
  __I  uint32_t GMAC_TBFT127;   /**< (GMAC Offset: 0x11C) 65 to 127 Byte Frames Transmitted Register */
  __I  uint32_t GMAC_TBFT255;   /**< (GMAC Offset: 0x120) 128 to 255 Byte Frames Transmitted Register */
  __I  uint32_t GMAC_TBFT511;   /**< (GMAC Offset: 0x124) 256 to 511 Byte Frames Transmitted Register */
  __I  uint32_t GMAC_TBFT1023;  /**< (GMAC Offset: 0x128) 512 to 1023 Byte Frames Transmitted Register */
  __I  uint32_t GMAC_TBFT1518;  /**< (GMAC Offset: 0x12C) 1024 to 1518 Byte Frames Transmitted Register */
  __I  uint32_t GMAC_GTBFT1518; /**< (GMAC Offset: 0x130) Greater Than 1518 Byte Frames Transmitted Register */
  __I  uint32_t GMAC_TUR;       /**< (GMAC Offset: 0x134) Transmit Underruns Register */
  __I  uint32_t GMAC_SCF;       /**< (GMAC Offset: 0x138) Single Collision Frames Register */
  __I  uint32_t GMAC_MCF;       /**< (GMAC Offset: 0x13C) Multiple Collision Frames Register */
  __I  uint32_t GMAC_EC;        /**< (GMAC Offset: 0x140) Excessive Collisions Register */
  __I  uint32_t GMAC_LC;        /**< (GMAC Offset: 0x144) Late Collisions Register */
  __I  uint32_t GMAC_DTF;       /**< (GMAC Offset: 0x148) Deferred Transmission Frames Register */
  __I  uint32_t GMAC_CSE;       /**< (GMAC Offset: 0x14C) Carrier Sense Errors Register */
  __I  uint32_t GMAC_ORLO;      /**< (GMAC Offset: 0x150) Octets Received Low Received Register */
  __I  uint32_t GMAC_ORHI;      /**< (GMAC Offset: 0x154) Octets Received High Received Register */
  __I  uint32_t GMAC_FR;        /**< (GMAC Offset: 0x158) Frames Received Register */
  __I  uint32_t GMAC_BCFR;      /**< (GMAC Offset: 0x15C) Broadcast Frames Received Register */
  __I  uint32_t GMAC_MFR;       /**< (GMAC Offset: 0x160) Multicast Frames Received Register */
  __I  uint32_t GMAC_PFR;       /**< (GMAC Offset: 0x164) Pause Frames Received Register */
  __I  uint32_t GMAC_BFR64;     /**< (GMAC Offset: 0x168) 64 Byte Frames Received Register */
  __I  uint32_t GMAC_TBFR127;   /**< (GMAC Offset: 0x16C) 65 to 127 Byte Frames Received Register */
  __I  uint32_t GMAC_TBFR255;   /**< (GMAC Offset: 0x170) 128 to 255 Byte Frames Received Register */
  __I  uint32_t GMAC_TBFR511;   /**< (GMAC Offset: 0x174) 256 to 511 Byte Frames Received Register */
  __I  uint32_t GMAC_TBFR1023;  /**< (GMAC Offset: 0x178) 512 to 1023 Byte Frames Received Register */
  __I  uint32_t GMAC_TBFR1518;  /**< (GMAC Offset: 0x17C) 1024 to 1518 Byte Frames Received Register */
  __I  uint32_t GMAC_TMXBFR;    /**< (GMAC Offset: 0x180) 1519 to Maximum Byte Frames Received Register */
  __I  uint32_t GMAC_UFR;       /**< (GMAC Offset: 0x184) Undersize Frames Received Register */
  __I  uint32_t GMAC_OFR;       /**< (GMAC Offset: 0x188) Oversize Frames Received Register */
  __I  uint32_t GMAC_JR;        /**< (GMAC Offset: 0x18C) Jabbers Received Register */
  __I  uint32_t GMAC_FCSE;      /**< (GMAC Offset: 0x190) Frame Check Sequence Errors Register */
  __I  uint32_t GMAC_LFFE;      /**< (GMAC Offset: 0x194) Length Field Frame Errors Register */
  __I  uint32_t GMAC_RSE;       /**< (GMAC Offset: 0x198) Receive Symbol Errors Register */
  __I  uint32_t GMAC_AE;        /**< (GMAC Offset: 0x19C) Alignment Errors Register */
  __I  uint32_t GMAC_RRE;       /**< (GMAC Offset: 0x1A0) Receive Resource Errors Register */
  __I  uint32_t GMAC_ROE;       /**< (GMAC Offset: 0x1A4) Receive Overrun Register */
  __I  uint32_t GMAC_IHCE;      /**< (GMAC Offset: 0x1A8) IP Header Checksum Errors Register */
  __I  uint32_t GMAC_TCE;       /**< (GMAC Offset: 0x1AC) TCP Checksum Errors Register */
  __I  uint32_t GMAC_UCE;       /**< (GMAC Offset: 0x1B0) UDP Checksum Errors Register */
  __I  uint32_t Reserved4[2];
  __IO uint32_t GMAC_TISUBN;    /**< (GMAC Offset: 0x1BC) 1588 Timer Increment Sub-nanoseconds Register */
  __IO uint32_t GMAC_TSH;       /**< (GMAC Offset: 0x1C0) 1588 Timer Seconds High Register */
  __I  uint32_t Reserved5[3];
  __IO uint32_t GMAC_TSL;       /**< (GMAC Offset: 0x1D0) 1588 Timer Seconds Low Register */
  __IO uint32_t GMAC_TN;        /**< (GMAC Offset: 0x1D4) 1588 Timer Nanoseconds Register */
  __O  uint32_t GMAC_TA;        /**< (GMAC Offset: 0x1D8) 1588 Timer Adjust Register */
  __IO uint32_t GMAC_TI;        /**< (GMAC Offset: 0x1DC) 1588 Timer Increment Register */
  __I  uint32_t GMAC_EFTSL;     /**< (GMAC Offset: 0x1E0) PTP Event Frame Transmitted Seconds Low Register */
  __I  uint32_t GMAC_EFTN;      /**< (GMAC Offset: 0x1E4) PTP Event Frame Transmitted Nanoseconds Register */
  __I  uint32_t GMAC_EFRSL;     /**< (GMAC Offset: 0x1E8) PTP Event Frame Received Seconds Low Register */
  __I  uint32_t GMAC_EFRN;      /**< (GMAC Offset: 0x1EC) PTP Event Frame Received Nanoseconds Register */
  __I  uint32_t GMAC_PEFTSL;    /**< (GMAC Offset: 0x1F0) PTP Peer Event Frame Transmitted Seconds Low Register */
  __I  uint32_t GMAC_PEFTN;     /**< (GMAC Offset: 0x1F4) PTP Peer Event Frame Transmitted Nanoseconds Register */
  __I  uint32_t GMAC_PEFRSL;    /**< (GMAC Offset: 0x1F8) PTP Peer Event Frame Received Seconds Low Register */
  __I  uint32_t GMAC_PEFRN;     /**< (GMAC Offset: 0x1FC) PTP Peer Event Frame Received Nanoseconds Register */
  __I  uint32_t Reserved6[128];
  __I  uint32_t GMAC_ISRPQ[2];  /**< (GMAC Offset: 0x400) Interrupt Status Register Priority Queue  (index = 1) 0 */
  __I  uint32_t Reserved7[14];
  __IO uint32_t GMAC_TBQBAPQ[2]; /**< (GMAC Offset: 0x440) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
  __I  uint32_t Reserved8[14];
  __IO uint32_t GMAC_RBQBAPQ[2]; /**< (GMAC Offset: 0x480) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
  __I  uint32_t Reserved9[6];
  __IO uint32_t GMAC_RBSRPQ[2]; /**< (GMAC Offset: 0x4A0) Receive Buffer Size Register Priority Queue  (index = 1) 0 */
  __I  uint32_t Reserved10[5];
  __IO uint32_t GMAC_CBSCR;     /**< (GMAC Offset: 0x4BC) Credit-Based Shaping Control Register */
  __IO uint32_t GMAC_CBSISQA;   /**< (GMAC Offset: 0x4C0) Credit-Based Shaping IdleSlope Register for Queue A */
  __IO uint32_t GMAC_CBSISQB;   /**< (GMAC Offset: 0x4C4) Credit-Based Shaping IdleSlope Register for Queue B */
  __I  uint32_t Reserved11[14];
  __IO uint32_t GMAC_ST1RPQ[4]; /**< (GMAC Offset: 0x500) Screening Type 1 Register Priority Queue  (index = 0) 0 */
  __I  uint32_t Reserved12[12];
  __IO uint32_t GMAC_ST2RPQ[8]; /**< (GMAC Offset: 0x540) Screening Type 2 Register Priority Queue  (index = 0) 0 */
  __I  uint32_t Reserved13[40];
  __O  uint32_t GMAC_IERPQ[2];  /**< (GMAC Offset: 0x600) Interrupt Enable Register Priority Queue  (index = 1) 0 */
  __I  uint32_t Reserved14[6];
  __O  uint32_t GMAC_IDRPQ[2];  /**< (GMAC Offset: 0x620) Interrupt Disable Register Priority Queue  (index = 1) 0 */
  __I  uint32_t Reserved15[6];
  __IO uint32_t GMAC_IMRPQ[2];  /**< (GMAC Offset: 0x640) Interrupt Mask Register Priority Queue  (index = 1) 0 */
  __I  uint32_t Reserved16[38];
  __IO uint32_t GMAC_ST2ER[4];  /**< (GMAC Offset: 0x6E0) Screening Type 2 Ethertype Register  (index = 0) 0 */
  __I  uint32_t Reserved17[4];
  __IO uint32_t GMAC_ST2CW00;   /**< (GMAC Offset: 0x700) Screening Type 2 Compare Word 0 Register  (index = 0) */
  __IO uint32_t GMAC_ST2CW10;   /**< (GMAC Offset: 0x704) Screening Type 2 Compare Word 1 Register  (index = 0) */
  __IO uint32_t GMAC_ST2CW01;   /**< (GMAC Offset: 0x708) Screening Type 2 Compare Word 0 Register  (index = 1) */
  __IO uint32_t GMAC_ST2CW11;   /**< (GMAC Offset: 0x70C) Screening Type 2 Compare Word 1 Register  (index = 1) */
  __IO uint32_t GMAC_ST2CW02;   /**< (GMAC Offset: 0x710) Screening Type 2 Compare Word 0 Register  (index = 2) */
  __IO uint32_t GMAC_ST2CW12;   /**< (GMAC Offset: 0x714) Screening Type 2 Compare Word 1 Register  (index = 2) */
  __IO uint32_t GMAC_ST2CW03;   /**< (GMAC Offset: 0x718) Screening Type 2 Compare Word 0 Register  (index = 3) */
  __IO uint32_t GMAC_ST2CW13;   /**< (GMAC Offset: 0x71C) Screening Type 2 Compare Word 1 Register  (index = 3) */
  __IO uint32_t GMAC_ST2CW04;   /**< (GMAC Offset: 0x720) Screening Type 2 Compare Word 0 Register  (index = 4) */
  __IO uint32_t GMAC_ST2CW14;   /**< (GMAC Offset: 0x724) Screening Type 2 Compare Word 1 Register  (index = 4) */
  __IO uint32_t GMAC_ST2CW05;   /**< (GMAC Offset: 0x728) Screening Type 2 Compare Word 0 Register  (index = 5) */
  __IO uint32_t GMAC_ST2CW15;   /**< (GMAC Offset: 0x72C) Screening Type 2 Compare Word 1 Register  (index = 5) */
  __IO uint32_t GMAC_ST2CW06;   /**< (GMAC Offset: 0x730) Screening Type 2 Compare Word 0 Register  (index = 6) */
  __IO uint32_t GMAC_ST2CW16;   /**< (GMAC Offset: 0x734) Screening Type 2 Compare Word 1 Register  (index = 6) */
  __IO uint32_t GMAC_ST2CW07;   /**< (GMAC Offset: 0x738) Screening Type 2 Compare Word 0 Register  (index = 7) */
  __IO uint32_t GMAC_ST2CW17;   /**< (GMAC Offset: 0x73C) Screening Type 2 Compare Word 1 Register  (index = 7) */
  __IO uint32_t GMAC_ST2CW08;   /**< (GMAC Offset: 0x740) Screening Type 2 Compare Word 0 Register  (index = 8) */
  __IO uint32_t GMAC_ST2CW18;   /**< (GMAC Offset: 0x744) Screening Type 2 Compare Word 1 Register  (index = 8) */
  __IO uint32_t GMAC_ST2CW09;   /**< (GMAC Offset: 0x748) Screening Type 2 Compare Word 0 Register  (index = 9) */
  __IO uint32_t GMAC_ST2CW19;   /**< (GMAC Offset: 0x74C) Screening Type 2 Compare Word 1 Register  (index = 9) */
  __IO uint32_t GMAC_ST2CW010;  /**< (GMAC Offset: 0x750) Screening Type 2 Compare Word 0 Register  (index = 10) */
  __IO uint32_t GMAC_ST2CW110;  /**< (GMAC Offset: 0x754) Screening Type 2 Compare Word 1 Register  (index = 10) */
  __IO uint32_t GMAC_ST2CW011;  /**< (GMAC Offset: 0x758) Screening Type 2 Compare Word 0 Register  (index = 11) */
  __IO uint32_t GMAC_ST2CW111;  /**< (GMAC Offset: 0x75C) Screening Type 2 Compare Word 1 Register  (index = 11) */
  __IO uint32_t GMAC_ST2CW012;  /**< (GMAC Offset: 0x760) Screening Type 2 Compare Word 0 Register  (index = 12) */
  __IO uint32_t GMAC_ST2CW112;  /**< (GMAC Offset: 0x764) Screening Type 2 Compare Word 1 Register  (index = 12) */
  __IO uint32_t GMAC_ST2CW013;  /**< (GMAC Offset: 0x768) Screening Type 2 Compare Word 0 Register  (index = 13) */
  __IO uint32_t GMAC_ST2CW113;  /**< (GMAC Offset: 0x76C) Screening Type 2 Compare Word 1 Register  (index = 13) */
  __IO uint32_t GMAC_ST2CW014;  /**< (GMAC Offset: 0x770) Screening Type 2 Compare Word 0 Register  (index = 14) */
  __IO uint32_t GMAC_ST2CW114;  /**< (GMAC Offset: 0x774) Screening Type 2 Compare Word 1 Register  (index = 14) */
  __IO uint32_t GMAC_ST2CW015;  /**< (GMAC Offset: 0x778) Screening Type 2 Compare Word 0 Register  (index = 15) */
  __IO uint32_t GMAC_ST2CW115;  /**< (GMAC Offset: 0x77C) Screening Type 2 Compare Word 1 Register  (index = 15) */
  __IO uint32_t GMAC_ST2CW016;  /**< (GMAC Offset: 0x780) Screening Type 2 Compare Word 0 Register  (index = 16) */
  __IO uint32_t GMAC_ST2CW116;  /**< (GMAC Offset: 0x784) Screening Type 2 Compare Word 1 Register  (index = 16) */
  __IO uint32_t GMAC_ST2CW017;  /**< (GMAC Offset: 0x788) Screening Type 2 Compare Word 0 Register  (index = 17) */
  __IO uint32_t GMAC_ST2CW117;  /**< (GMAC Offset: 0x78C) Screening Type 2 Compare Word 1 Register  (index = 17) */
  __IO uint32_t GMAC_ST2CW018;  /**< (GMAC Offset: 0x790) Screening Type 2 Compare Word 0 Register  (index = 18) */
  __IO uint32_t GMAC_ST2CW118;  /**< (GMAC Offset: 0x794) Screening Type 2 Compare Word 1 Register  (index = 18) */
  __IO uint32_t GMAC_ST2CW019;  /**< (GMAC Offset: 0x798) Screening Type 2 Compare Word 0 Register  (index = 19) */
  __IO uint32_t GMAC_ST2CW119;  /**< (GMAC Offset: 0x79C) Screening Type 2 Compare Word 1 Register  (index = 19) */
  __IO uint32_t GMAC_ST2CW020;  /**< (GMAC Offset: 0x7A0) Screening Type 2 Compare Word 0 Register  (index = 20) */
  __IO uint32_t GMAC_ST2CW120;  /**< (GMAC Offset: 0x7A4) Screening Type 2 Compare Word 1 Register  (index = 20) */
  __IO uint32_t GMAC_ST2CW021;  /**< (GMAC Offset: 0x7A8) Screening Type 2 Compare Word 0 Register  (index = 21) */
  __IO uint32_t GMAC_ST2CW121;  /**< (GMAC Offset: 0x7AC) Screening Type 2 Compare Word 1 Register  (index = 21) */
  __IO uint32_t GMAC_ST2CW022;  /**< (GMAC Offset: 0x7B0) Screening Type 2 Compare Word 0 Register  (index = 22) */
  __IO uint32_t GMAC_ST2CW122;  /**< (GMAC Offset: 0x7B4) Screening Type 2 Compare Word 1 Register  (index = 22) */
  __IO uint32_t GMAC_ST2CW023;  /**< (GMAC Offset: 0x7B8) Screening Type 2 Compare Word 0 Register  (index = 23) */
  __IO uint32_t GMAC_ST2CW123;  /**< (GMAC Offset: 0x7BC) Screening Type 2 Compare Word 1 Register  (index = 23) */
} Gmac;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief GMAC_SA hardware registers */
typedef struct {  
  __IO GMAC_SAB_Type                  GMAC_SAB;       /**< Offset: 0x00 (R/W  32) Specific Address 1 Bottom Register */
  __IO GMAC_SAT_Type                  GMAC_SAT;       /**< Offset: 0x04 (R/W  32) Specific Address 1 Top Register */
} GmacSa;

/** \brief GMAC hardware registers */
typedef struct {  
  __IO GMAC_NCR_Type                  GMAC_NCR;       /**< Offset: 0x00 (R/W  32) Network Control Register */
  __IO GMAC_NCFGR_Type                GMAC_NCFGR;     /**< Offset: 0x04 (R/W  32) Network Configuration Register */
  __I  GMAC_NSR_Type                  GMAC_NSR;       /**< Offset: 0x08 (R/   32) Network Status Register */
  __IO GMAC_UR_Type                   GMAC_UR;        /**< Offset: 0x0C (R/W  32) User Register */
  __IO GMAC_DCFGR_Type                GMAC_DCFGR;     /**< Offset: 0x10 (R/W  32) DMA Configuration Register */
  __IO GMAC_TSR_Type                  GMAC_TSR;       /**< Offset: 0x14 (R/W  32) Transmit Status Register */
  __IO GMAC_RBQB_Type                 GMAC_RBQB;      /**< Offset: 0x18 (R/W  32) Receive Buffer Queue Base Address Register */
  __IO GMAC_TBQB_Type                 GMAC_TBQB;      /**< Offset: 0x1C (R/W  32) Transmit Buffer Queue Base Address Register */
  __IO GMAC_RSR_Type                  GMAC_RSR;       /**< Offset: 0x20 (R/W  32) Receive Status Register */
  __I  GMAC_ISR_Type                  GMAC_ISR;       /**< Offset: 0x24 (R/   32) Interrupt Status Register */
  __O  GMAC_IER_Type                  GMAC_IER;       /**< Offset: 0x28 ( /W  32) Interrupt Enable Register */
  __O  GMAC_IDR_Type                  GMAC_IDR;       /**< Offset: 0x2C ( /W  32) Interrupt Disable Register */
  __IO GMAC_IMR_Type                  GMAC_IMR;       /**< Offset: 0x30 (R/W  32) Interrupt Mask Register */
  __IO GMAC_MAN_Type                  GMAC_MAN;       /**< Offset: 0x34 (R/W  32) PHY Maintenance Register */
  __I  GMAC_RPQ_Type                  GMAC_RPQ;       /**< Offset: 0x38 (R/   32) Received Pause Quantum Register */
  __IO GMAC_TPQ_Type                  GMAC_TPQ;       /**< Offset: 0x3C (R/W  32) Transmit Pause Quantum Register */
  __IO GMAC_TPSF_Type                 GMAC_TPSF;      /**< Offset: 0x40 (R/W  32) TX Partial Store and Forward Register */
  __IO GMAC_RPSF_Type                 GMAC_RPSF;      /**< Offset: 0x44 (R/W  32) RX Partial Store and Forward Register */
  __IO GMAC_RJFML_Type                GMAC_RJFML;     /**< Offset: 0x48 (R/W  32) RX Jumbo Frame Max Length Register */
       RoReg8                         Reserved1[0x34];
  __IO GMAC_HRB_Type                  GMAC_HRB;       /**< Offset: 0x80 (R/W  32) Hash Register Bottom */
  __IO GMAC_HRT_Type                  GMAC_HRT;       /**< Offset: 0x84 (R/W  32) Hash Register Top */
       GmacSa                         GMAC_SA[4];     /**< Offset: 0x88 Specific Address 1 Bottom Register */
  __IO GMAC_TIDM1_Type                GMAC_TIDM1;     /**< Offset: 0xA8 (R/W  32) Type ID Match 1 Register */
  __IO GMAC_TIDM2_Type                GMAC_TIDM2;     /**< Offset: 0xAC (R/W  32) Type ID Match 2 Register */
  __IO GMAC_TIDM3_Type                GMAC_TIDM3;     /**< Offset: 0xB0 (R/W  32) Type ID Match 3 Register */
  __IO GMAC_TIDM4_Type                GMAC_TIDM4;     /**< Offset: 0xB4 (R/W  32) Type ID Match 4 Register */
  __IO GMAC_WOL_Type                  GMAC_WOL;       /**< Offset: 0xB8 (R/W  32) Wake on LAN Register */
  __IO GMAC_IPGS_Type                 GMAC_IPGS;      /**< Offset: 0xBC (R/W  32) IPG Stretch Register */
  __IO GMAC_SVLAN_Type                GMAC_SVLAN;     /**< Offset: 0xC0 (R/W  32) Stacked VLAN Register */
  __IO GMAC_TPFCP_Type                GMAC_TPFCP;     /**< Offset: 0xC4 (R/W  32) Transmit PFC Pause Register */
  __IO GMAC_SAMB1_Type                GMAC_SAMB1;     /**< Offset: 0xC8 (R/W  32) Specific Address 1 Mask Bottom Register */
  __IO GMAC_SAMT1_Type                GMAC_SAMT1;     /**< Offset: 0xCC (R/W  32) Specific Address 1 Mask Top Register */
       RoReg8                         Reserved2[0xC];
  __IO GMAC_NSC_Type                  GMAC_NSC;       /**< Offset: 0xDC (R/W  32) 1588 Timer Nanosecond Comparison Register */
  __IO GMAC_SCL_Type                  GMAC_SCL;       /**< Offset: 0xE0 (R/W  32) 1588 Timer Second Comparison Low Register */
  __IO GMAC_SCH_Type                  GMAC_SCH;       /**< Offset: 0xE4 (R/W  32) 1588 Timer Second Comparison High Register */
  __I  GMAC_EFTSH_Type                GMAC_EFTSH;     /**< Offset: 0xE8 (R/   32) PTP Event Frame Transmitted Seconds High Register */
  __I  GMAC_EFRSH_Type                GMAC_EFRSH;     /**< Offset: 0xEC (R/   32) PTP Event Frame Received Seconds High Register */
  __I  GMAC_PEFTSH_Type               GMAC_PEFTSH;    /**< Offset: 0xF0 (R/   32) PTP Peer Event Frame Transmitted Seconds High Register */
  __I  GMAC_PEFRSH_Type               GMAC_PEFRSH;    /**< Offset: 0xF4 (R/   32) PTP Peer Event Frame Received Seconds High Register */
       RoReg8                         Reserved3[0x8];
  __I  GMAC_OTLO_Type                 GMAC_OTLO;      /**< Offset: 0x100 (R/   32) Octets Transmitted Low Register */
  __I  GMAC_OTHI_Type                 GMAC_OTHI;      /**< Offset: 0x104 (R/   32) Octets Transmitted High Register */
  __I  GMAC_FT_Type                   GMAC_FT;        /**< Offset: 0x108 (R/   32) Frames Transmitted Register */
  __I  GMAC_BCFT_Type                 GMAC_BCFT;      /**< Offset: 0x10C (R/   32) Broadcast Frames Transmitted Register */
  __I  GMAC_MFT_Type                  GMAC_MFT;       /**< Offset: 0x110 (R/   32) Multicast Frames Transmitted Register */
  __I  GMAC_PFT_Type                  GMAC_PFT;       /**< Offset: 0x114 (R/   32) Pause Frames Transmitted Register */
  __I  GMAC_BFT64_Type                GMAC_BFT64;     /**< Offset: 0x118 (R/   32) 64 Byte Frames Transmitted Register */
  __I  GMAC_TBFT127_Type              GMAC_TBFT127;   /**< Offset: 0x11C (R/   32) 65 to 127 Byte Frames Transmitted Register */
  __I  GMAC_TBFT255_Type              GMAC_TBFT255;   /**< Offset: 0x120 (R/   32) 128 to 255 Byte Frames Transmitted Register */
  __I  GMAC_TBFT511_Type              GMAC_TBFT511;   /**< Offset: 0x124 (R/   32) 256 to 511 Byte Frames Transmitted Register */
  __I  GMAC_TBFT1023_Type             GMAC_TBFT1023;  /**< Offset: 0x128 (R/   32) 512 to 1023 Byte Frames Transmitted Register */
  __I  GMAC_TBFT1518_Type             GMAC_TBFT1518;  /**< Offset: 0x12C (R/   32) 1024 to 1518 Byte Frames Transmitted Register */
  __I  GMAC_GTBFT1518_Type            GMAC_GTBFT1518; /**< Offset: 0x130 (R/   32) Greater Than 1518 Byte Frames Transmitted Register */
  __I  GMAC_TUR_Type                  GMAC_TUR;       /**< Offset: 0x134 (R/   32) Transmit Underruns Register */
  __I  GMAC_SCF_Type                  GMAC_SCF;       /**< Offset: 0x138 (R/   32) Single Collision Frames Register */
  __I  GMAC_MCF_Type                  GMAC_MCF;       /**< Offset: 0x13C (R/   32) Multiple Collision Frames Register */
  __I  GMAC_EC_Type                   GMAC_EC;        /**< Offset: 0x140 (R/   32) Excessive Collisions Register */
  __I  GMAC_LC_Type                   GMAC_LC;        /**< Offset: 0x144 (R/   32) Late Collisions Register */
  __I  GMAC_DTF_Type                  GMAC_DTF;       /**< Offset: 0x148 (R/   32) Deferred Transmission Frames Register */
  __I  GMAC_CSE_Type                  GMAC_CSE;       /**< Offset: 0x14C (R/   32) Carrier Sense Errors Register */
  __I  GMAC_ORLO_Type                 GMAC_ORLO;      /**< Offset: 0x150 (R/   32) Octets Received Low Received Register */
  __I  GMAC_ORHI_Type                 GMAC_ORHI;      /**< Offset: 0x154 (R/   32) Octets Received High Received Register */
  __I  GMAC_FR_Type                   GMAC_FR;        /**< Offset: 0x158 (R/   32) Frames Received Register */
  __I  GMAC_BCFR_Type                 GMAC_BCFR;      /**< Offset: 0x15C (R/   32) Broadcast Frames Received Register */
  __I  GMAC_MFR_Type                  GMAC_MFR;       /**< Offset: 0x160 (R/   32) Multicast Frames Received Register */
  __I  GMAC_PFR_Type                  GMAC_PFR;       /**< Offset: 0x164 (R/   32) Pause Frames Received Register */
  __I  GMAC_BFR64_Type                GMAC_BFR64;     /**< Offset: 0x168 (R/   32) 64 Byte Frames Received Register */
  __I  GMAC_TBFR127_Type              GMAC_TBFR127;   /**< Offset: 0x16C (R/   32) 65 to 127 Byte Frames Received Register */
  __I  GMAC_TBFR255_Type              GMAC_TBFR255;   /**< Offset: 0x170 (R/   32) 128 to 255 Byte Frames Received Register */
  __I  GMAC_TBFR511_Type              GMAC_TBFR511;   /**< Offset: 0x174 (R/   32) 256 to 511 Byte Frames Received Register */
  __I  GMAC_TBFR1023_Type             GMAC_TBFR1023;  /**< Offset: 0x178 (R/   32) 512 to 1023 Byte Frames Received Register */
  __I  GMAC_TBFR1518_Type             GMAC_TBFR1518;  /**< Offset: 0x17C (R/   32) 1024 to 1518 Byte Frames Received Register */
  __I  GMAC_TMXBFR_Type               GMAC_TMXBFR;    /**< Offset: 0x180 (R/   32) 1519 to Maximum Byte Frames Received Register */
  __I  GMAC_UFR_Type                  GMAC_UFR;       /**< Offset: 0x184 (R/   32) Undersize Frames Received Register */
  __I  GMAC_OFR_Type                  GMAC_OFR;       /**< Offset: 0x188 (R/   32) Oversize Frames Received Register */
  __I  GMAC_JR_Type                   GMAC_JR;        /**< Offset: 0x18C (R/   32) Jabbers Received Register */
  __I  GMAC_FCSE_Type                 GMAC_FCSE;      /**< Offset: 0x190 (R/   32) Frame Check Sequence Errors Register */
  __I  GMAC_LFFE_Type                 GMAC_LFFE;      /**< Offset: 0x194 (R/   32) Length Field Frame Errors Register */
  __I  GMAC_RSE_Type                  GMAC_RSE;       /**< Offset: 0x198 (R/   32) Receive Symbol Errors Register */
  __I  GMAC_AE_Type                   GMAC_AE;        /**< Offset: 0x19C (R/   32) Alignment Errors Register */
  __I  GMAC_RRE_Type                  GMAC_RRE;       /**< Offset: 0x1A0 (R/   32) Receive Resource Errors Register */
  __I  GMAC_ROE_Type                  GMAC_ROE;       /**< Offset: 0x1A4 (R/   32) Receive Overrun Register */
  __I  GMAC_IHCE_Type                 GMAC_IHCE;      /**< Offset: 0x1A8 (R/   32) IP Header Checksum Errors Register */
  __I  GMAC_TCE_Type                  GMAC_TCE;       /**< Offset: 0x1AC (R/   32) TCP Checksum Errors Register */
  __I  GMAC_UCE_Type                  GMAC_UCE;       /**< Offset: 0x1B0 (R/   32) UDP Checksum Errors Register */
       RoReg8                         Reserved4[0x8];
  __IO GMAC_TISUBN_Type               GMAC_TISUBN;    /**< Offset: 0x1BC (R/W  32) 1588 Timer Increment Sub-nanoseconds Register */
  __IO GMAC_TSH_Type                  GMAC_TSH;       /**< Offset: 0x1C0 (R/W  32) 1588 Timer Seconds High Register */
       RoReg8                         Reserved5[0xC];
  __IO GMAC_TSL_Type                  GMAC_TSL;       /**< Offset: 0x1D0 (R/W  32) 1588 Timer Seconds Low Register */
  __IO GMAC_TN_Type                   GMAC_TN;        /**< Offset: 0x1D4 (R/W  32) 1588 Timer Nanoseconds Register */
  __O  GMAC_TA_Type                   GMAC_TA;        /**< Offset: 0x1D8 ( /W  32) 1588 Timer Adjust Register */
  __IO GMAC_TI_Type                   GMAC_TI;        /**< Offset: 0x1DC (R/W  32) 1588 Timer Increment Register */
  __I  GMAC_EFTSL_Type                GMAC_EFTSL;     /**< Offset: 0x1E0 (R/   32) PTP Event Frame Transmitted Seconds Low Register */
  __I  GMAC_EFTN_Type                 GMAC_EFTN;      /**< Offset: 0x1E4 (R/   32) PTP Event Frame Transmitted Nanoseconds Register */
  __I  GMAC_EFRSL_Type                GMAC_EFRSL;     /**< Offset: 0x1E8 (R/   32) PTP Event Frame Received Seconds Low Register */
  __I  GMAC_EFRN_Type                 GMAC_EFRN;      /**< Offset: 0x1EC (R/   32) PTP Event Frame Received Nanoseconds Register */
  __I  GMAC_PEFTSL_Type               GMAC_PEFTSL;    /**< Offset: 0x1F0 (R/   32) PTP Peer Event Frame Transmitted Seconds Low Register */
  __I  GMAC_PEFTN_Type                GMAC_PEFTN;     /**< Offset: 0x1F4 (R/   32) PTP Peer Event Frame Transmitted Nanoseconds Register */
  __I  GMAC_PEFRSL_Type               GMAC_PEFRSL;    /**< Offset: 0x1F8 (R/   32) PTP Peer Event Frame Received Seconds Low Register */
  __I  GMAC_PEFRN_Type                GMAC_PEFRN;     /**< Offset: 0x1FC (R/   32) PTP Peer Event Frame Received Nanoseconds Register */
       RoReg8                         Reserved6[0x200];
  __I  GMAC_ISRPQ_Type                GMAC_ISRPQ[2];  /**< Offset: 0x400 (R/   32) Interrupt Status Register Priority Queue  (index = 1) 0 */
       RoReg8                         Reserved7[0x38];
  __IO GMAC_TBQBAPQ_Type              GMAC_TBQBAPQ[2]; /**< Offset: 0x440 (R/W  32) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
       RoReg8                         Reserved8[0x38];
  __IO GMAC_RBQBAPQ_Type              GMAC_RBQBAPQ[2]; /**< Offset: 0x480 (R/W  32) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
       RoReg8                         Reserved9[0x18];
  __IO GMAC_RBSRPQ_Type               GMAC_RBSRPQ[2]; /**< Offset: 0x4A0 (R/W  32) Receive Buffer Size Register Priority Queue  (index = 1) 0 */
       RoReg8                         Reserved10[0x14];
  __IO GMAC_CBSCR_Type                GMAC_CBSCR;     /**< Offset: 0x4BC (R/W  32) Credit-Based Shaping Control Register */
  __IO GMAC_CBSISQA_Type              GMAC_CBSISQA;   /**< Offset: 0x4C0 (R/W  32) Credit-Based Shaping IdleSlope Register for Queue A */
  __IO GMAC_CBSISQB_Type              GMAC_CBSISQB;   /**< Offset: 0x4C4 (R/W  32) Credit-Based Shaping IdleSlope Register for Queue B */
       RoReg8                         Reserved11[0x38];
  __IO GMAC_ST1RPQ_Type               GMAC_ST1RPQ[4]; /**< Offset: 0x500 (R/W  32) Screening Type 1 Register Priority Queue  (index = 0) 0 */
       RoReg8                         Reserved12[0x30];
  __IO GMAC_ST2RPQ_Type               GMAC_ST2RPQ[8]; /**< Offset: 0x540 (R/W  32) Screening Type 2 Register Priority Queue  (index = 0) 0 */
       RoReg8                         Reserved13[0xA0];
  __O  GMAC_IERPQ_Type                GMAC_IERPQ[2];  /**< Offset: 0x600 ( /W  32) Interrupt Enable Register Priority Queue  (index = 1) 0 */
       RoReg8                         Reserved14[0x18];
  __O  GMAC_IDRPQ_Type                GMAC_IDRPQ[2];  /**< Offset: 0x620 ( /W  32) Interrupt Disable Register Priority Queue  (index = 1) 0 */
       RoReg8                         Reserved15[0x18];
  __IO GMAC_IMRPQ_Type                GMAC_IMRPQ[2];  /**< Offset: 0x640 (R/W  32) Interrupt Mask Register Priority Queue  (index = 1) 0 */
       RoReg8                         Reserved16[0x98];
  __IO GMAC_ST2ER_Type                GMAC_ST2ER[4];  /**< Offset: 0x6E0 (R/W  32) Screening Type 2 Ethertype Register  (index = 0) 0 */
       RoReg8                         Reserved17[0x10];
  __IO GMAC_ST2CW00_Type              GMAC_ST2CW00;   /**< Offset: 0x700 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 0) */
  __IO GMAC_ST2CW10_Type              GMAC_ST2CW10;   /**< Offset: 0x704 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 0) */
  __IO GMAC_ST2CW01_Type              GMAC_ST2CW01;   /**< Offset: 0x708 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 1) */
  __IO GMAC_ST2CW11_Type              GMAC_ST2CW11;   /**< Offset: 0x70C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 1) */
  __IO GMAC_ST2CW02_Type              GMAC_ST2CW02;   /**< Offset: 0x710 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 2) */
  __IO GMAC_ST2CW12_Type              GMAC_ST2CW12;   /**< Offset: 0x714 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 2) */
  __IO GMAC_ST2CW03_Type              GMAC_ST2CW03;   /**< Offset: 0x718 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 3) */
  __IO GMAC_ST2CW13_Type              GMAC_ST2CW13;   /**< Offset: 0x71C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 3) */
  __IO GMAC_ST2CW04_Type              GMAC_ST2CW04;   /**< Offset: 0x720 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 4) */
  __IO GMAC_ST2CW14_Type              GMAC_ST2CW14;   /**< Offset: 0x724 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 4) */
  __IO GMAC_ST2CW05_Type              GMAC_ST2CW05;   /**< Offset: 0x728 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 5) */
  __IO GMAC_ST2CW15_Type              GMAC_ST2CW15;   /**< Offset: 0x72C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 5) */
  __IO GMAC_ST2CW06_Type              GMAC_ST2CW06;   /**< Offset: 0x730 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 6) */
  __IO GMAC_ST2CW16_Type              GMAC_ST2CW16;   /**< Offset: 0x734 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 6) */
  __IO GMAC_ST2CW07_Type              GMAC_ST2CW07;   /**< Offset: 0x738 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 7) */
  __IO GMAC_ST2CW17_Type              GMAC_ST2CW17;   /**< Offset: 0x73C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 7) */
  __IO GMAC_ST2CW08_Type              GMAC_ST2CW08;   /**< Offset: 0x740 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 8) */
  __IO GMAC_ST2CW18_Type              GMAC_ST2CW18;   /**< Offset: 0x744 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 8) */
  __IO GMAC_ST2CW09_Type              GMAC_ST2CW09;   /**< Offset: 0x748 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 9) */
  __IO GMAC_ST2CW19_Type              GMAC_ST2CW19;   /**< Offset: 0x74C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 9) */
  __IO GMAC_ST2CW010_Type             GMAC_ST2CW010;  /**< Offset: 0x750 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 10) */
  __IO GMAC_ST2CW110_Type             GMAC_ST2CW110;  /**< Offset: 0x754 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 10) */
  __IO GMAC_ST2CW011_Type             GMAC_ST2CW011;  /**< Offset: 0x758 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 11) */
  __IO GMAC_ST2CW111_Type             GMAC_ST2CW111;  /**< Offset: 0x75C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 11) */
  __IO GMAC_ST2CW012_Type             GMAC_ST2CW012;  /**< Offset: 0x760 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 12) */
  __IO GMAC_ST2CW112_Type             GMAC_ST2CW112;  /**< Offset: 0x764 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 12) */
  __IO GMAC_ST2CW013_Type             GMAC_ST2CW013;  /**< Offset: 0x768 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 13) */
  __IO GMAC_ST2CW113_Type             GMAC_ST2CW113;  /**< Offset: 0x76C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 13) */
  __IO GMAC_ST2CW014_Type             GMAC_ST2CW014;  /**< Offset: 0x770 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 14) */
  __IO GMAC_ST2CW114_Type             GMAC_ST2CW114;  /**< Offset: 0x774 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 14) */
  __IO GMAC_ST2CW015_Type             GMAC_ST2CW015;  /**< Offset: 0x778 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 15) */
  __IO GMAC_ST2CW115_Type             GMAC_ST2CW115;  /**< Offset: 0x77C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 15) */
  __IO GMAC_ST2CW016_Type             GMAC_ST2CW016;  /**< Offset: 0x780 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 16) */
  __IO GMAC_ST2CW116_Type             GMAC_ST2CW116;  /**< Offset: 0x784 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 16) */
  __IO GMAC_ST2CW017_Type             GMAC_ST2CW017;  /**< Offset: 0x788 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 17) */
  __IO GMAC_ST2CW117_Type             GMAC_ST2CW117;  /**< Offset: 0x78C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 17) */
  __IO GMAC_ST2CW018_Type             GMAC_ST2CW018;  /**< Offset: 0x790 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 18) */
  __IO GMAC_ST2CW118_Type             GMAC_ST2CW118;  /**< Offset: 0x794 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 18) */
  __IO GMAC_ST2CW019_Type             GMAC_ST2CW019;  /**< Offset: 0x798 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 19) */
  __IO GMAC_ST2CW119_Type             GMAC_ST2CW119;  /**< Offset: 0x79C (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 19) */
  __IO GMAC_ST2CW020_Type             GMAC_ST2CW020;  /**< Offset: 0x7A0 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 20) */
  __IO GMAC_ST2CW120_Type             GMAC_ST2CW120;  /**< Offset: 0x7A4 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 20) */
  __IO GMAC_ST2CW021_Type             GMAC_ST2CW021;  /**< Offset: 0x7A8 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 21) */
  __IO GMAC_ST2CW121_Type             GMAC_ST2CW121;  /**< Offset: 0x7AC (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 21) */
  __IO GMAC_ST2CW022_Type             GMAC_ST2CW022;  /**< Offset: 0x7B0 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 22) */
  __IO GMAC_ST2CW122_Type             GMAC_ST2CW122;  /**< Offset: 0x7B4 (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 22) */
  __IO GMAC_ST2CW023_Type             GMAC_ST2CW023;  /**< Offset: 0x7B8 (R/W  32) Screening Type 2 Compare Word 0 Register  (index = 23) */
  __IO GMAC_ST2CW123_Type             GMAC_ST2CW123;  /**< Offset: 0x7BC (R/W  32) Screening Type 2 Compare Word 1 Register  (index = 23) */
} Gmac;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Gigabit Ethernet MAC */

#endif /* _SAME70_GMAC_COMPONENT_H_ */
