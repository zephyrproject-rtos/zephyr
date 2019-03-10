/**
 * \file
 *
 * \brief Component description for PICOP
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAME54_PICOP_COMPONENT_
#define _SAME54_PICOP_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR PICOP */
/* ========================================================================== */
/** \addtogroup SAME54_PICOP PicoProcessor */
/*@{*/

#define PICOP_U2232
#define REV_PICOP                   0x200

/* -------- PICOP_ID : (PICOP Offset: 0x000) (R/W 32) ID n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ID:32;            /*!< bit:  0..31  ID String 0                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_ID_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_ID_OFFSET             0x000        /**< \brief (PICOP_ID offset) ID n */
#define PICOP_ID_RESETVALUE         0x00000000ul /**< \brief (PICOP_ID reset_value) ID n */

#define PICOP_ID_ID_Pos             0            /**< \brief (PICOP_ID) ID String 0 */
#define PICOP_ID_ID_Msk             (0xFFFFFFFFul << PICOP_ID_ID_Pos)
#define PICOP_ID_ID(value)          (PICOP_ID_ID_Msk & ((value) << PICOP_ID_ID_Pos))
#define PICOP_ID_MASK               0xFFFFFFFFul /**< \brief (PICOP_ID) MASK Register */

/* -------- PICOP_CONFIG : (PICOP Offset: 0x020) (R/W 32) Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ISA:2;            /*!< bit:  0.. 1  Instruction Set Architecture       */
    uint32_t ASP:1;            /*!< bit:      2  Aligned Stack Pointer              */
    uint32_t MARRET:1;         /*!< bit:      3  Misaligned implicit long return register (GCC compatibility) */
    uint32_t RRET:4;           /*!< bit:  4.. 7  Implicit return word register      */
    uint32_t PCEXEN:1;         /*!< bit:      8  PC_EX register enabled for reduced interrupt latency */
    uint32_t :23;              /*!< bit:  9..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_CONFIG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_CONFIG_OFFSET         0x020        /**< \brief (PICOP_CONFIG offset) Configuration */
#define PICOP_CONFIG_RESETVALUE     0x00000000ul /**< \brief (PICOP_CONFIG reset_value) Configuration */

#define PICOP_CONFIG_ISA_Pos        0            /**< \brief (PICOP_CONFIG) Instruction Set Architecture */
#define PICOP_CONFIG_ISA_Msk        (0x3ul << PICOP_CONFIG_ISA_Pos)
#define PICOP_CONFIG_ISA(value)     (PICOP_CONFIG_ISA_Msk & ((value) << PICOP_CONFIG_ISA_Pos))
#define   PICOP_CONFIG_ISA_AVR8_Val       0x0ul  /**< \brief (PICOP_CONFIG) AVR8 ISA, AVR8SP=1 */
#define   PICOP_CONFIG_ISA_AVR16C_Val     0x1ul  /**< \brief (PICOP_CONFIG) AVR16 ISA fully compatible with AVR8 ISA, AVR8SP=1 */
#define   PICOP_CONFIG_ISA_AVR16E_Val     0x2ul  /**< \brief (PICOP_CONFIG) AVR16 ISA extended, AVR8SP=1 */
#define   PICOP_CONFIG_ISA_AVR16_Val      0x3ul  /**< \brief (PICOP_CONFIG) AVR16 ISA extended, AVR8SP=0 */
#define PICOP_CONFIG_ISA_AVR8       (PICOP_CONFIG_ISA_AVR8_Val     << PICOP_CONFIG_ISA_Pos)
#define PICOP_CONFIG_ISA_AVR16C     (PICOP_CONFIG_ISA_AVR16C_Val   << PICOP_CONFIG_ISA_Pos)
#define PICOP_CONFIG_ISA_AVR16E     (PICOP_CONFIG_ISA_AVR16E_Val   << PICOP_CONFIG_ISA_Pos)
#define PICOP_CONFIG_ISA_AVR16      (PICOP_CONFIG_ISA_AVR16_Val    << PICOP_CONFIG_ISA_Pos)
#define PICOP_CONFIG_ASP_Pos        2            /**< \brief (PICOP_CONFIG) Aligned Stack Pointer */
#define PICOP_CONFIG_ASP            (0x1ul << PICOP_CONFIG_ASP_Pos)
#define PICOP_CONFIG_MARRET_Pos     3            /**< \brief (PICOP_CONFIG) Misaligned implicit long return register (GCC compatibility) */
#define PICOP_CONFIG_MARRET         (0x1ul << PICOP_CONFIG_MARRET_Pos)
#define PICOP_CONFIG_RRET_Pos       4            /**< \brief (PICOP_CONFIG) Implicit return word register */
#define PICOP_CONFIG_RRET_Msk       (0xFul << PICOP_CONFIG_RRET_Pos)
#define PICOP_CONFIG_RRET(value)    (PICOP_CONFIG_RRET_Msk & ((value) << PICOP_CONFIG_RRET_Pos))
#define PICOP_CONFIG_PCEXEN_Pos     8            /**< \brief (PICOP_CONFIG) PC_EX register enabled for reduced interrupt latency */
#define PICOP_CONFIG_PCEXEN         (0x1ul << PICOP_CONFIG_PCEXEN_Pos)
#define PICOP_CONFIG_MASK           0x000001FFul /**< \brief (PICOP_CONFIG) MASK Register */

/* -------- PICOP_CTRL : (PICOP Offset: 0x024) (R/W 32) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MAPUEXCEPT:1;     /*!< bit:      0  Enable exception for illegal access */
    uint32_t WPICACHE:1;       /*!< bit:      1  Write protect iCache               */
    uint32_t WPVEC:2;          /*!< bit:  2.. 3  Write protect vectors              */
    uint32_t WPCTX:2;          /*!< bit:  4.. 5  Write protect contexts             */
    uint32_t WPCODE:4;         /*!< bit:  6.. 9  Write protect code                 */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_CTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_CTRL_OFFSET           0x024        /**< \brief (PICOP_CTRL offset) Control */
#define PICOP_CTRL_RESETVALUE       0x00000000ul /**< \brief (PICOP_CTRL reset_value) Control */

#define PICOP_CTRL_MAPUEXCEPT_Pos   0            /**< \brief (PICOP_CTRL) Enable exception for illegal access */
#define PICOP_CTRL_MAPUEXCEPT       (0x1ul << PICOP_CTRL_MAPUEXCEPT_Pos)
#define PICOP_CTRL_WPICACHE_Pos     1            /**< \brief (PICOP_CTRL) Write protect iCache */
#define PICOP_CTRL_WPICACHE         (0x1ul << PICOP_CTRL_WPICACHE_Pos)
#define PICOP_CTRL_WPVEC_Pos        2            /**< \brief (PICOP_CTRL) Write protect vectors */
#define PICOP_CTRL_WPVEC_Msk        (0x3ul << PICOP_CTRL_WPVEC_Pos)
#define PICOP_CTRL_WPVEC(value)     (PICOP_CTRL_WPVEC_Msk & ((value) << PICOP_CTRL_WPVEC_Pos))
#define   PICOP_CTRL_WPVEC_NONE_Val       0x0ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPVEC_RSTNMI_Val     0x1ul  /**< \brief (PICOP_CTRL)  */
#define PICOP_CTRL_WPVEC_NONE       (PICOP_CTRL_WPVEC_NONE_Val     << PICOP_CTRL_WPVEC_Pos)
#define PICOP_CTRL_WPVEC_RSTNMI     (PICOP_CTRL_WPVEC_RSTNMI_Val   << PICOP_CTRL_WPVEC_Pos)
#define PICOP_CTRL_WPCTX_Pos        4            /**< \brief (PICOP_CTRL) Write protect contexts */
#define PICOP_CTRL_WPCTX_Msk        (0x3ul << PICOP_CTRL_WPCTX_Pos)
#define PICOP_CTRL_WPCTX(value)     (PICOP_CTRL_WPCTX_Msk & ((value) << PICOP_CTRL_WPCTX_Pos))
#define   PICOP_CTRL_WPCTX_NONE_Val       0x0ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCTX_CTX0_Val       0x1ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCTX_CTX01_Val      0x2ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCTX_CTX012_Val     0x3ul  /**< \brief (PICOP_CTRL)  */
#define PICOP_CTRL_WPCTX_NONE       (PICOP_CTRL_WPCTX_NONE_Val     << PICOP_CTRL_WPCTX_Pos)
#define PICOP_CTRL_WPCTX_CTX0       (PICOP_CTRL_WPCTX_CTX0_Val     << PICOP_CTRL_WPCTX_Pos)
#define PICOP_CTRL_WPCTX_CTX01      (PICOP_CTRL_WPCTX_CTX01_Val    << PICOP_CTRL_WPCTX_Pos)
#define PICOP_CTRL_WPCTX_CTX012     (PICOP_CTRL_WPCTX_CTX012_Val   << PICOP_CTRL_WPCTX_Pos)
#define PICOP_CTRL_WPCODE_Pos       6            /**< \brief (PICOP_CTRL) Write protect code */
#define PICOP_CTRL_WPCODE_Msk       (0xFul << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE(value)    (PICOP_CTRL_WPCODE_Msk & ((value) << PICOP_CTRL_WPCODE_Pos))
#define   PICOP_CTRL_WPCODE_NONE_Val      0x0ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_256B_Val      0x1ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_512B_Val      0x2ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_768B_Val      0x3ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_1024B_Val     0x4ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_1280B_Val     0x5ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_1536B_Val     0x6ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_1792B_Val     0x7ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_2048B_Val     0x8ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_2304B_Val     0x9ul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_2560B_Val     0xAul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_2816B_Val     0xBul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_3072B_Val     0xCul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_3328B_Val     0xDul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_3584B_Val     0xEul  /**< \brief (PICOP_CTRL)  */
#define   PICOP_CTRL_WPCODE_3840B_Val     0xFul  /**< \brief (PICOP_CTRL)  */
#define PICOP_CTRL_WPCODE_NONE      (PICOP_CTRL_WPCODE_NONE_Val    << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_256B      (PICOP_CTRL_WPCODE_256B_Val    << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_512B      (PICOP_CTRL_WPCODE_512B_Val    << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_768B      (PICOP_CTRL_WPCODE_768B_Val    << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_1024B     (PICOP_CTRL_WPCODE_1024B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_1280B     (PICOP_CTRL_WPCODE_1280B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_1536B     (PICOP_CTRL_WPCODE_1536B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_1792B     (PICOP_CTRL_WPCODE_1792B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_2048B     (PICOP_CTRL_WPCODE_2048B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_2304B     (PICOP_CTRL_WPCODE_2304B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_2560B     (PICOP_CTRL_WPCODE_2560B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_2816B     (PICOP_CTRL_WPCODE_2816B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_3072B     (PICOP_CTRL_WPCODE_3072B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_3328B     (PICOP_CTRL_WPCODE_3328B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_3584B     (PICOP_CTRL_WPCODE_3584B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_WPCODE_3840B     (PICOP_CTRL_WPCODE_3840B_Val   << PICOP_CTRL_WPCODE_Pos)
#define PICOP_CTRL_MASK             0x000003FFul /**< \brief (PICOP_CTRL) MASK Register */

/* -------- PICOP_CMD : (PICOP Offset: 0x028) (R/W 32) Command -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // CMD mode
    uint32_t CMD:4;            /*!< bit:  0.. 3  Command                            */
    uint32_t :12;              /*!< bit:  4..15  Reserved                           */
    uint32_t UNLOCK:16;        /*!< bit: 16..31  Unlock                             */
  } CMD;                       /*!< Structure used for CMD                          */
  struct { // STATUS mode
    uint32_t CTTSEX:1;         /*!< bit:      0  Context Task Switch                */
    uint32_t IL0EX:1;          /*!< bit:      1  Interrupt Level 0 Exception        */
    uint32_t IL1EX:1;          /*!< bit:      2  Interrupt Level 1 Exception        */
    uint32_t IL2EX:1;          /*!< bit:      3  Interrupt Level 2 Exception        */
    uint32_t IL3EX:1;          /*!< bit:      4  Interrupt Level 3 Exception        */
    uint32_t IL4EX:1;          /*!< bit:      5  Interrupt Level 4 Exception        */
    uint32_t NMIEX:1;          /*!< bit:      6  NMI Exception                      */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t EXCEPT:1;         /*!< bit:      8  Exception                          */
    uint32_t AVR16:1;          /*!< bit:      9  AVR16 Mode                         */
    uint32_t OCDCOF:1;         /*!< bit:     10  OCD Change of Flow                 */
    uint32_t :5;               /*!< bit: 11..15  Reserved                           */
    uint32_t UPC:8;            /*!< bit: 16..23  Microcode State                    */
    uint32_t :3;               /*!< bit: 24..26  Reserved                           */
    uint32_t STATE:5;          /*!< bit: 27..31  System State                       */
  } STATUS;                    /*!< Structure used for STATUS                       */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_CMD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_CMD_OFFSET            0x028        /**< \brief (PICOP_CMD offset) Command */
#define PICOP_CMD_RESETVALUE        0x00000000ul /**< \brief (PICOP_CMD reset_value) Command */

// CMD mode
#define PICOP_CMD_CMD_CMD_Pos       0            /**< \brief (PICOP_CMD_CMD) Command */
#define PICOP_CMD_CMD_CMD_Msk       (0xFul << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD(value)    (PICOP_CMD_CMD_CMD_Msk & ((value) << PICOP_CMD_CMD_CMD_Pos))
#define   PICOP_CMD_CMD_CMD_NOACTION_Val  0x0ul  /**< \brief (PICOP_CMD_CMD) No action */
#define   PICOP_CMD_CMD_CMD_STOP_Val      0x1ul  /**< \brief (PICOP_CMD_CMD) Wait for ongoing execution to complete, then stop */
#define   PICOP_CMD_CMD_CMD_RESET_Val     0x2ul  /**< \brief (PICOP_CMD_CMD) Stop, reset and stop */
#define   PICOP_CMD_CMD_CMD_RESTART_Val   0x3ul  /**< \brief (PICOP_CMD_CMD) Stop, reset and run */
#define   PICOP_CMD_CMD_CMD_ABORT_Val     0x4ul  /**< \brief (PICOP_CMD_CMD) Abort, reset and stop */
#define   PICOP_CMD_CMD_CMD_RUN_Val       0x5ul  /**< \brief (PICOP_CMD_CMD) Start execution (from unlocked stopped state) */
#define   PICOP_CMD_CMD_CMD_RUNLOCK_Val   0x6ul  /**< \brief (PICOP_CMD_CMD) Start execution and lock */
#define   PICOP_CMD_CMD_CMD_RUNOCD_Val    0x7ul  /**< \brief (PICOP_CMD_CMD) Start execution and enable host-controlled OCD */
#define   PICOP_CMD_CMD_CMD_UNLOCK_Val    0x8ul  /**< \brief (PICOP_CMD_CMD) Unlock and run */
#define   PICOP_CMD_CMD_CMD_NMI_Val       0x9ul  /**< \brief (PICOP_CMD_CMD) Trigger a NMI */
#define   PICOP_CMD_CMD_CMD_WAKEUP_Val    0xAul  /**< \brief (PICOP_CMD_CMD) Force a wakeup from sleep (if in sleep) */
#define PICOP_CMD_CMD_CMD_NOACTION  (PICOP_CMD_CMD_CMD_NOACTION_Val << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_STOP      (PICOP_CMD_CMD_CMD_STOP_Val    << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_RESET     (PICOP_CMD_CMD_CMD_RESET_Val   << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_RESTART   (PICOP_CMD_CMD_CMD_RESTART_Val << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_ABORT     (PICOP_CMD_CMD_CMD_ABORT_Val   << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_RUN       (PICOP_CMD_CMD_CMD_RUN_Val     << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_RUNLOCK   (PICOP_CMD_CMD_CMD_RUNLOCK_Val << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_RUNOCD    (PICOP_CMD_CMD_CMD_RUNOCD_Val  << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_UNLOCK    (PICOP_CMD_CMD_CMD_UNLOCK_Val  << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_NMI       (PICOP_CMD_CMD_CMD_NMI_Val     << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_CMD_WAKEUP    (PICOP_CMD_CMD_CMD_WAKEUP_Val  << PICOP_CMD_CMD_CMD_Pos)
#define PICOP_CMD_CMD_UNLOCK_Pos    16           /**< \brief (PICOP_CMD_CMD) Unlock */
#define PICOP_CMD_CMD_UNLOCK_Msk    (0xFFFFul << PICOP_CMD_CMD_UNLOCK_Pos)
#define PICOP_CMD_CMD_UNLOCK(value) (PICOP_CMD_CMD_UNLOCK_Msk & ((value) << PICOP_CMD_CMD_UNLOCK_Pos))
#define PICOP_CMD_CMD_MASK          0xFFFF000Ful /**< \brief (PICOP_CMD_CMD) MASK Register */

// STATUS mode
#define PICOP_CMD_STATUS_CTTSEX_Pos 0            /**< \brief (PICOP_CMD_STATUS) Context Task Switch */
#define PICOP_CMD_STATUS_CTTSEX     (0x1ul << PICOP_CMD_STATUS_CTTSEX_Pos)
#define PICOP_CMD_STATUS_IL0EX_Pos  1            /**< \brief (PICOP_CMD_STATUS) Interrupt Level 0 Exception */
#define PICOP_CMD_STATUS_IL0EX      (0x1ul << PICOP_CMD_STATUS_IL0EX_Pos)
#define PICOP_CMD_STATUS_IL1EX_Pos  2            /**< \brief (PICOP_CMD_STATUS) Interrupt Level 1 Exception */
#define PICOP_CMD_STATUS_IL1EX      (0x1ul << PICOP_CMD_STATUS_IL1EX_Pos)
#define PICOP_CMD_STATUS_IL2EX_Pos  3            /**< \brief (PICOP_CMD_STATUS) Interrupt Level 2 Exception */
#define PICOP_CMD_STATUS_IL2EX      (0x1ul << PICOP_CMD_STATUS_IL2EX_Pos)
#define PICOP_CMD_STATUS_IL3EX_Pos  4            /**< \brief (PICOP_CMD_STATUS) Interrupt Level 3 Exception */
#define PICOP_CMD_STATUS_IL3EX      (0x1ul << PICOP_CMD_STATUS_IL3EX_Pos)
#define PICOP_CMD_STATUS_IL4EX_Pos  5            /**< \brief (PICOP_CMD_STATUS) Interrupt Level 4 Exception */
#define PICOP_CMD_STATUS_IL4EX      (0x1ul << PICOP_CMD_STATUS_IL4EX_Pos)
#define PICOP_CMD_STATUS_NMIEX_Pos  6            /**< \brief (PICOP_CMD_STATUS) NMI Exception */
#define PICOP_CMD_STATUS_NMIEX      (0x1ul << PICOP_CMD_STATUS_NMIEX_Pos)
#define PICOP_CMD_STATUS_EXCEPT_Pos 8            /**< \brief (PICOP_CMD_STATUS) Exception */
#define PICOP_CMD_STATUS_EXCEPT     (0x1ul << PICOP_CMD_STATUS_EXCEPT_Pos)
#define PICOP_CMD_STATUS_AVR16_Pos  9            /**< \brief (PICOP_CMD_STATUS) AVR16 Mode */
#define PICOP_CMD_STATUS_AVR16      (0x1ul << PICOP_CMD_STATUS_AVR16_Pos)
#define PICOP_CMD_STATUS_OCDCOF_Pos 10           /**< \brief (PICOP_CMD_STATUS) OCD Change of Flow */
#define PICOP_CMD_STATUS_OCDCOF     (0x1ul << PICOP_CMD_STATUS_OCDCOF_Pos)
#define PICOP_CMD_STATUS_UPC_Pos    16           /**< \brief (PICOP_CMD_STATUS) Microcode State */
#define PICOP_CMD_STATUS_UPC_Msk    (0xFFul << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC(value) (PICOP_CMD_STATUS_UPC_Msk & ((value) << PICOP_CMD_STATUS_UPC_Pos))
#define   PICOP_CMD_STATUS_UPC_EXEC_Val   0x0ul  /**< \brief (PICOP_CMD_STATUS) Normal execution (no ucode) */
#define   PICOP_CMD_STATUS_UPC_EXEC_NOBRK_Val 0x1ul  /**< \brief (PICOP_CMD_STATUS) Normal execution with break disabled */
#define   PICOP_CMD_STATUS_UPC_EXEC_NOP_Val 0x2ul  /**< \brief (PICOP_CMD_STATUS) OCD NOP override execution (break disabled) */
#define   PICOP_CMD_STATUS_UPC_EXEC_IMM_Val 0x3ul  /**< \brief (PICOP_CMD_STATUS) OCD IMM override execution (break disabled) */
#define   PICOP_CMD_STATUS_UPC_ICACHE_FLUSH_Val 0x4ul  /**< \brief (PICOP_CMD_STATUS) Flush instruction cache */
#define   PICOP_CMD_STATUS_UPC_HALT_Val   0x10ul  /**< \brief (PICOP_CMD_STATUS) HALT execution (shutdown) */
#define   PICOP_CMD_STATUS_UPC_HALTED_Val 0x11ul  /**< \brief (PICOP_CMD_STATUS) Execution halted (shutdown) */
#define   PICOP_CMD_STATUS_UPC_SLEEP_Val  0x17ul  /**< \brief (PICOP_CMD_STATUS) Wait until safe to go to sleeping state */
#define   PICOP_CMD_STATUS_UPC_SLEEPING_Val 0x18ul  /**< \brief (PICOP_CMD_STATUS) Sleeping / reset cycle 0 */
#define   PICOP_CMD_STATUS_UPC_WAKEUP_RST1_Val 0x19ul  /**< \brief (PICOP_CMD_STATUS) Reset cycle 1 */
#define   PICOP_CMD_STATUS_UPC_WAKEUP_CTR_SP_Val 0x1Aul  /**< \brief (PICOP_CMD_STATUS) SLEEP: Context Restore CCR..SP */
#define   PICOP_CMD_STATUS_UPC_WAKEUP_CTR_ZY_Val 0x1Bul  /**< \brief (PICOP_CMD_STATUS) SLEEP: Context Restore Z..Y */
#define   PICOP_CMD_STATUS_UPC_OCD_STATE_Val 0x20ul  /**< \brief (PICOP_CMD_STATUS) OCD state: No break (sr.upc[1:0] == 2'b00) */
#define   PICOP_CMD_STATUS_UPC_OCD_STATE_NOP_Val 0x21ul  /**< \brief (PICOP_CMD_STATUS) OCD state: NOP override (sr.upc[1:0] == 2'b01) */
#define   PICOP_CMD_STATUS_UPC_OCD_STATE_IMM_Val 0x22ul  /**< \brief (PICOP_CMD_STATUS) OCD state: IMM override (sr.upc[1:0] == 2'b10) */
#define   PICOP_CMD_STATUS_UPC_OCD_STATE_SLEEP_Val 0x23ul  /**< \brief (PICOP_CMD_STATUS) OCD state: SLEEP instruction (sr.upc[1:0] == 2'b11) */
#define   PICOP_CMD_STATUS_UPC_OCD_BREAKPOINT_Val 0x28ul  /**< \brief (PICOP_CMD_STATUS) Breakpoint (sr.upc[0] == 1'b0) */
#define   PICOP_CMD_STATUS_UPC_OCD_BREAKI_Val 0x29ul  /**< \brief (PICOP_CMD_STATUS) Breakpoint instruction (sr.upc[0] == 1'b1) */
#define   PICOP_CMD_STATUS_UPC_CANCEL_EX_Val 0x2Eul  /**< \brief (PICOP_CMD_STATUS) Cancel exception */
#define   PICOP_CMD_STATUS_UPC_IRQ_Val    0x2Ful  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save CCR..SP */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_0_Val 0x30ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+0+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_1_Val 0x31ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+1+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_2_Val 0x32ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+2+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_3_Val 0x33ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+3+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_4_Val 0x34ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+4+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_5_Val 0x35ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+5+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_6_Val 0x36ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+6+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_7_Val 0x37ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save R{m+7+1}.l */
#define   PICOP_CMD_STATUS_UPC_IRQ_CTS_PC_Val 0x38ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Context Save (SR):PC */
#define   PICOP_CMD_STATUS_UPC_IRQ_ACK_Val 0x39ul  /**< \brief (PICOP_CMD_STATUS) IRQ: Acknowledge cycle */
#define   PICOP_CMD_STATUS_UPC_EXCEPT_Val 0x3Aul  /**< \brief (PICOP_CMD_STATUS) Internal exceptions */
#define   PICOP_CMD_STATUS_UPC_RETI_SLEEP_Val 0x3Ful  /**< \brief (PICOP_CMD_STATUS) RETI: Clear SLEEPMODE (RETI) */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R0_Val 0x40ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore  R3..R0 (RETIS) */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R4_Val 0x41ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore  R7..R4 */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R8_Val 0x42ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore R11..R8 */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R12_Val 0x43ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore R15..R12 */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R16_Val 0x44ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore R19..R16 */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R20_Val 0x45ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore R23..R20 */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R24_Val 0x46ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore R27..R24 */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_R28_Val 0x47ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore R31..R28 */
#define   PICOP_CMD_STATUS_UPC_RETI_CTR_SP_Val 0x48ul  /**< \brief (PICOP_CMD_STATUS) RETI: Context Restore CCR..SP */
#define   PICOP_CMD_STATUS_UPC_RETI_EXEC_Val 0x49ul  /**< \brief (PICOP_CMD_STATUS) RETI: Return to code execution (PC <- LINK) */
#define PICOP_CMD_STATUS_UPC_EXEC   (PICOP_CMD_STATUS_UPC_EXEC_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_EXEC_NOBRK (PICOP_CMD_STATUS_UPC_EXEC_NOBRK_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_EXEC_NOP (PICOP_CMD_STATUS_UPC_EXEC_NOP_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_EXEC_IMM (PICOP_CMD_STATUS_UPC_EXEC_IMM_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_ICACHE_FLUSH (PICOP_CMD_STATUS_UPC_ICACHE_FLUSH_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_HALT   (PICOP_CMD_STATUS_UPC_HALT_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_HALTED (PICOP_CMD_STATUS_UPC_HALTED_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_SLEEP  (PICOP_CMD_STATUS_UPC_SLEEP_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_SLEEPING (PICOP_CMD_STATUS_UPC_SLEEPING_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_WAKEUP_RST1 (PICOP_CMD_STATUS_UPC_WAKEUP_RST1_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_WAKEUP_CTR_SP (PICOP_CMD_STATUS_UPC_WAKEUP_CTR_SP_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_WAKEUP_CTR_ZY (PICOP_CMD_STATUS_UPC_WAKEUP_CTR_ZY_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_OCD_STATE (PICOP_CMD_STATUS_UPC_OCD_STATE_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_OCD_STATE_NOP (PICOP_CMD_STATUS_UPC_OCD_STATE_NOP_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_OCD_STATE_IMM (PICOP_CMD_STATUS_UPC_OCD_STATE_IMM_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_OCD_STATE_SLEEP (PICOP_CMD_STATUS_UPC_OCD_STATE_SLEEP_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_OCD_BREAKPOINT (PICOP_CMD_STATUS_UPC_OCD_BREAKPOINT_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_OCD_BREAKI (PICOP_CMD_STATUS_UPC_OCD_BREAKI_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_CANCEL_EX (PICOP_CMD_STATUS_UPC_CANCEL_EX_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ    (PICOP_CMD_STATUS_UPC_IRQ_Val  << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_0 (PICOP_CMD_STATUS_UPC_IRQ_CTS_0_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_1 (PICOP_CMD_STATUS_UPC_IRQ_CTS_1_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_2 (PICOP_CMD_STATUS_UPC_IRQ_CTS_2_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_3 (PICOP_CMD_STATUS_UPC_IRQ_CTS_3_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_4 (PICOP_CMD_STATUS_UPC_IRQ_CTS_4_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_5 (PICOP_CMD_STATUS_UPC_IRQ_CTS_5_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_6 (PICOP_CMD_STATUS_UPC_IRQ_CTS_6_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_7 (PICOP_CMD_STATUS_UPC_IRQ_CTS_7_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_CTS_PC (PICOP_CMD_STATUS_UPC_IRQ_CTS_PC_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_IRQ_ACK (PICOP_CMD_STATUS_UPC_IRQ_ACK_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_EXCEPT (PICOP_CMD_STATUS_UPC_EXCEPT_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_SLEEP (PICOP_CMD_STATUS_UPC_RETI_SLEEP_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R0 (PICOP_CMD_STATUS_UPC_RETI_CTR_R0_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R4 (PICOP_CMD_STATUS_UPC_RETI_CTR_R4_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R8 (PICOP_CMD_STATUS_UPC_RETI_CTR_R8_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R12 (PICOP_CMD_STATUS_UPC_RETI_CTR_R12_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R16 (PICOP_CMD_STATUS_UPC_RETI_CTR_R16_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R20 (PICOP_CMD_STATUS_UPC_RETI_CTR_R20_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R24 (PICOP_CMD_STATUS_UPC_RETI_CTR_R24_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_R28 (PICOP_CMD_STATUS_UPC_RETI_CTR_R28_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_CTR_SP (PICOP_CMD_STATUS_UPC_RETI_CTR_SP_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_UPC_RETI_EXEC (PICOP_CMD_STATUS_UPC_RETI_EXEC_Val << PICOP_CMD_STATUS_UPC_Pos)
#define PICOP_CMD_STATUS_STATE_Pos  27           /**< \brief (PICOP_CMD_STATUS) System State */
#define PICOP_CMD_STATUS_STATE_Msk  (0x1Ful << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE(value) (PICOP_CMD_STATUS_STATE_Msk & ((value) << PICOP_CMD_STATUS_STATE_Pos))
#define   PICOP_CMD_STATUS_STATE_RESET_0_Val 0x0ul  /**< \brief (PICOP_CMD_STATUS) Reset step 0 */
#define   PICOP_CMD_STATUS_STATE_RESET_1_Val 0x1ul  /**< \brief (PICOP_CMD_STATUS) Reset step 1 */
#define   PICOP_CMD_STATUS_STATE_RESET_2_Val 0x2ul  /**< \brief (PICOP_CMD_STATUS) Reset step 2 */
#define   PICOP_CMD_STATUS_STATE_RESET_3_Val 0x3ul  /**< \brief (PICOP_CMD_STATUS) Reset step 3 */
#define   PICOP_CMD_STATUS_STATE_FUSE_CHECK_Val 0x4ul  /**< \brief (PICOP_CMD_STATUS) Fuse check */
#define   PICOP_CMD_STATUS_STATE_INITIALIZED_Val 0x5ul  /**< \brief (PICOP_CMD_STATUS) Initialized */
#define   PICOP_CMD_STATUS_STATE_STANDBY_Val 0x6ul  /**< \brief (PICOP_CMD_STATUS) Standby */
#define   PICOP_CMD_STATUS_STATE_RUNNING_LOCKED_Val 0x8ul  /**< \brief (PICOP_CMD_STATUS) Running locked */
#define   PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_1_Val 0x9ul  /**< \brief (PICOP_CMD_STATUS) Running unlock step 1 */
#define   PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_2_Val 0xAul  /**< \brief (PICOP_CMD_STATUS) Running unlock step 2 */
#define   PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_3_Val 0xBul  /**< \brief (PICOP_CMD_STATUS) Running unlock step 3 */
#define   PICOP_CMD_STATUS_STATE_RUNNING_Val 0xCul  /**< \brief (PICOP_CMD_STATUS) Running */
#define   PICOP_CMD_STATUS_STATE_RUNNING_BOOT_Val 0xDul  /**< \brief (PICOP_CMD_STATUS) Running boot */
#define   PICOP_CMD_STATUS_STATE_RUNNING_HOSTOCD_Val 0xEul  /**< \brief (PICOP_CMD_STATUS) Running hostocd */
#define   PICOP_CMD_STATUS_STATE_RESETTING_Val 0x10ul  /**< \brief (PICOP_CMD_STATUS) Resetting */
#define   PICOP_CMD_STATUS_STATE_STOPPING_Val 0x11ul  /**< \brief (PICOP_CMD_STATUS) Stopping */
#define   PICOP_CMD_STATUS_STATE_STOPPED_Val 0x12ul  /**< \brief (PICOP_CMD_STATUS) Stopped */
#define PICOP_CMD_STATUS_STATE_RESET_0 (PICOP_CMD_STATUS_STATE_RESET_0_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RESET_1 (PICOP_CMD_STATUS_STATE_RESET_1_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RESET_2 (PICOP_CMD_STATUS_STATE_RESET_2_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RESET_3 (PICOP_CMD_STATUS_STATE_RESET_3_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_FUSE_CHECK (PICOP_CMD_STATUS_STATE_FUSE_CHECK_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_INITIALIZED (PICOP_CMD_STATUS_STATE_INITIALIZED_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_STANDBY (PICOP_CMD_STATUS_STATE_STANDBY_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RUNNING_LOCKED (PICOP_CMD_STATUS_STATE_RUNNING_LOCKED_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_1 (PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_1_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_2 (PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_2_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_3 (PICOP_CMD_STATUS_STATE_RUNNING_UNLOCK_3_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RUNNING (PICOP_CMD_STATUS_STATE_RUNNING_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RUNNING_BOOT (PICOP_CMD_STATUS_STATE_RUNNING_BOOT_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RUNNING_HOSTOCD (PICOP_CMD_STATUS_STATE_RUNNING_HOSTOCD_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_RESETTING (PICOP_CMD_STATUS_STATE_RESETTING_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_STOPPING (PICOP_CMD_STATUS_STATE_STOPPING_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_STATE_STOPPED (PICOP_CMD_STATUS_STATE_STOPPED_Val << PICOP_CMD_STATUS_STATE_Pos)
#define PICOP_CMD_STATUS_MASK       0xF8FF077Ful /**< \brief (PICOP_CMD_STATUS) MASK Register */

/* -------- PICOP_PC : (PICOP Offset: 0x02C) (R/W 32) Program Counter -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PC:16;            /*!< bit:  0..15  Program Counter                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_PC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_PC_OFFSET             0x02C        /**< \brief (PICOP_PC offset) Program Counter */
#define PICOP_PC_RESETVALUE         0x00000000ul /**< \brief (PICOP_PC reset_value) Program Counter */

#define PICOP_PC_PC_Pos             0            /**< \brief (PICOP_PC) Program Counter */
#define PICOP_PC_PC_Msk             (0xFFFFul << PICOP_PC_PC_Pos)
#define PICOP_PC_PC(value)          (PICOP_PC_PC_Msk & ((value) << PICOP_PC_PC_Pos))
#define PICOP_PC_MASK               0x0000FFFFul /**< \brief (PICOP_PC) MASK Register */

/* -------- PICOP_HF : (PICOP Offset: 0x030) (R/W 32) Host Flags -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t HF:32;            /*!< bit:  0..31  Host Flags                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_HF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_HF_OFFSET             0x030        /**< \brief (PICOP_HF offset) Host Flags */
#define PICOP_HF_RESETVALUE         0x00000000ul /**< \brief (PICOP_HF reset_value) Host Flags */

#define PICOP_HF_HF_Pos             0            /**< \brief (PICOP_HF) Host Flags */
#define PICOP_HF_HF_Msk             (0xFFFFFFFFul << PICOP_HF_HF_Pos)
#define PICOP_HF_HF(value)          (PICOP_HF_HF_Msk & ((value) << PICOP_HF_HF_Pos))
#define PICOP_HF_MASK               0xFFFFFFFFul /**< \brief (PICOP_HF) MASK Register */

/* -------- PICOP_HFCTRL : (PICOP Offset: 0x034) (R/W 32) Host Flag Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :4;               /*!< bit:  0.. 3  Reserved                           */
    uint32_t IRQENCLR:4;       /*!< bit:  4.. 7  Host Flags IRQ Enable Clear        */
    uint32_t :4;               /*!< bit:  8..11  Reserved                           */
    uint32_t IRQENSET:4;       /*!< bit: 12..15  Host Flags IRQ Enable Set          */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_HFCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_HFCTRL_OFFSET         0x034        /**< \brief (PICOP_HFCTRL offset) Host Flag Control */
#define PICOP_HFCTRL_RESETVALUE     0x00000000ul /**< \brief (PICOP_HFCTRL reset_value) Host Flag Control */

#define PICOP_HFCTRL_IRQENCLR_Pos   4            /**< \brief (PICOP_HFCTRL) Host Flags IRQ Enable Clear */
#define PICOP_HFCTRL_IRQENCLR_Msk   (0xFul << PICOP_HFCTRL_IRQENCLR_Pos)
#define PICOP_HFCTRL_IRQENCLR(value) (PICOP_HFCTRL_IRQENCLR_Msk & ((value) << PICOP_HFCTRL_IRQENCLR_Pos))
#define PICOP_HFCTRL_IRQENSET_Pos   12           /**< \brief (PICOP_HFCTRL) Host Flags IRQ Enable Set */
#define PICOP_HFCTRL_IRQENSET_Msk   (0xFul << PICOP_HFCTRL_IRQENSET_Pos)
#define PICOP_HFCTRL_IRQENSET(value) (PICOP_HFCTRL_IRQENSET_Msk & ((value) << PICOP_HFCTRL_IRQENSET_Pos))
#define PICOP_HFCTRL_MASK           0x0000F0F0ul /**< \brief (PICOP_HFCTRL) MASK Register */

/* -------- PICOP_HFSETCLR0 : (PICOP Offset: 0x038) (R/W 32) Host Flags Set/Clr -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t HFCLR0:8;         /*!< bit:  0.. 7  Host Flags Clear bits 7:0          */
    uint32_t HFSET0:8;         /*!< bit:  8..15  Host Flags Set bits 7:0            */
    uint32_t HFCLR1:8;         /*!< bit: 16..23  Host Flags Clear bits 15:8         */
    uint32_t HFSET1:8;         /*!< bit: 24..31  Host Flags Set bits 15:8           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_HFSETCLR0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_HFSETCLR0_OFFSET      0x038        /**< \brief (PICOP_HFSETCLR0 offset) Host Flags Set/Clr */
#define PICOP_HFSETCLR0_RESETVALUE  0x00000000ul /**< \brief (PICOP_HFSETCLR0 reset_value) Host Flags Set/Clr */

#define PICOP_HFSETCLR0_HFCLR0_Pos  0            /**< \brief (PICOP_HFSETCLR0) Host Flags Clear bits 7:0 */
#define PICOP_HFSETCLR0_HFCLR0_Msk  (0xFFul << PICOP_HFSETCLR0_HFCLR0_Pos)
#define PICOP_HFSETCLR0_HFCLR0(value) (PICOP_HFSETCLR0_HFCLR0_Msk & ((value) << PICOP_HFSETCLR0_HFCLR0_Pos))
#define PICOP_HFSETCLR0_HFSET0_Pos  8            /**< \brief (PICOP_HFSETCLR0) Host Flags Set bits 7:0 */
#define PICOP_HFSETCLR0_HFSET0_Msk  (0xFFul << PICOP_HFSETCLR0_HFSET0_Pos)
#define PICOP_HFSETCLR0_HFSET0(value) (PICOP_HFSETCLR0_HFSET0_Msk & ((value) << PICOP_HFSETCLR0_HFSET0_Pos))
#define PICOP_HFSETCLR0_HFCLR1_Pos  16           /**< \brief (PICOP_HFSETCLR0) Host Flags Clear bits 15:8 */
#define PICOP_HFSETCLR0_HFCLR1_Msk  (0xFFul << PICOP_HFSETCLR0_HFCLR1_Pos)
#define PICOP_HFSETCLR0_HFCLR1(value) (PICOP_HFSETCLR0_HFCLR1_Msk & ((value) << PICOP_HFSETCLR0_HFCLR1_Pos))
#define PICOP_HFSETCLR0_HFSET1_Pos  24           /**< \brief (PICOP_HFSETCLR0) Host Flags Set bits 15:8 */
#define PICOP_HFSETCLR0_HFSET1_Msk  (0xFFul << PICOP_HFSETCLR0_HFSET1_Pos)
#define PICOP_HFSETCLR0_HFSET1(value) (PICOP_HFSETCLR0_HFSET1_Msk & ((value) << PICOP_HFSETCLR0_HFSET1_Pos))
#define PICOP_HFSETCLR0_MASK        0xFFFFFFFFul /**< \brief (PICOP_HFSETCLR0) MASK Register */

/* -------- PICOP_HFSETCLR1 : (PICOP Offset: 0x03C) (R/W 32) Host Flags Set/Clr -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t HFCLR2:8;         /*!< bit:  0.. 7  Host Flags Clear bits 23:16        */
    uint32_t HFSET2:8;         /*!< bit:  8..15  Host Flags Set bits 23:16          */
    uint32_t HFCLR3:8;         /*!< bit: 16..23  Host Flags Clear bits 31:24        */
    uint32_t HFSET3:8;         /*!< bit: 24..31  Host Flags Set bits 31:24          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_HFSETCLR1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_HFSETCLR1_OFFSET      0x03C        /**< \brief (PICOP_HFSETCLR1 offset) Host Flags Set/Clr */
#define PICOP_HFSETCLR1_RESETVALUE  0x00000000ul /**< \brief (PICOP_HFSETCLR1 reset_value) Host Flags Set/Clr */

#define PICOP_HFSETCLR1_HFCLR2_Pos  0            /**< \brief (PICOP_HFSETCLR1) Host Flags Clear bits 23:16 */
#define PICOP_HFSETCLR1_HFCLR2_Msk  (0xFFul << PICOP_HFSETCLR1_HFCLR2_Pos)
#define PICOP_HFSETCLR1_HFCLR2(value) (PICOP_HFSETCLR1_HFCLR2_Msk & ((value) << PICOP_HFSETCLR1_HFCLR2_Pos))
#define PICOP_HFSETCLR1_HFSET2_Pos  8            /**< \brief (PICOP_HFSETCLR1) Host Flags Set bits 23:16 */
#define PICOP_HFSETCLR1_HFSET2_Msk  (0xFFul << PICOP_HFSETCLR1_HFSET2_Pos)
#define PICOP_HFSETCLR1_HFSET2(value) (PICOP_HFSETCLR1_HFSET2_Msk & ((value) << PICOP_HFSETCLR1_HFSET2_Pos))
#define PICOP_HFSETCLR1_HFCLR3_Pos  16           /**< \brief (PICOP_HFSETCLR1) Host Flags Clear bits 31:24 */
#define PICOP_HFSETCLR1_HFCLR3_Msk  (0xFFul << PICOP_HFSETCLR1_HFCLR3_Pos)
#define PICOP_HFSETCLR1_HFCLR3(value) (PICOP_HFSETCLR1_HFCLR3_Msk & ((value) << PICOP_HFSETCLR1_HFCLR3_Pos))
#define PICOP_HFSETCLR1_HFSET3_Pos  24           /**< \brief (PICOP_HFSETCLR1) Host Flags Set bits 31:24 */
#define PICOP_HFSETCLR1_HFSET3_Msk  (0xFFul << PICOP_HFSETCLR1_HFSET3_Pos)
#define PICOP_HFSETCLR1_HFSET3(value) (PICOP_HFSETCLR1_HFSET3_Msk & ((value) << PICOP_HFSETCLR1_HFSET3_Pos))
#define PICOP_HFSETCLR1_MASK        0xFFFFFFFFul /**< \brief (PICOP_HFSETCLR1) MASK Register */

/* -------- PICOP_OCDCONFIG : (PICOP Offset: 0x050) (R/W 32) OCD Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t CCNTEN:1;         /*!< bit:      1  Cycle Counter Enable               */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_OCDCONFIG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_OCDCONFIG_OFFSET      0x050        /**< \brief (PICOP_OCDCONFIG offset) OCD Configuration */
#define PICOP_OCDCONFIG_RESETVALUE  0x00000000ul /**< \brief (PICOP_OCDCONFIG reset_value) OCD Configuration */

#define PICOP_OCDCONFIG_CCNTEN_Pos  1            /**< \brief (PICOP_OCDCONFIG) Cycle Counter Enable */
#define PICOP_OCDCONFIG_CCNTEN      (0x1ul << PICOP_OCDCONFIG_CCNTEN_Pos)
#define PICOP_OCDCONFIG_MASK        0x00000002ul /**< \brief (PICOP_OCDCONFIG) MASK Register */

/* -------- PICOP_OCDCONTROL : (PICOP Offset: 0x054) (R/W 32) OCD Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t OCDEN:1;          /*!< bit:      0  OCD Enable                         */
    uint32_t :1;               /*!< bit:      1  Reserved                           */
    uint32_t BPSSTEP:1;        /*!< bit:      2  Single Step Breakpoint             */
    uint32_t BPCOF:1;          /*!< bit:      3  Change of Flow Breakpoint          */
    uint32_t BPRST:1;          /*!< bit:      4  Reset Breakpoint                   */
    uint32_t BPEXCEPTION:1;    /*!< bit:      5  Exception Breakpoint               */
    uint32_t BPIRQ:1;          /*!< bit:      6  Interrupt Request Breakpoint       */
    uint32_t BPSW:1;           /*!< bit:      7  Software Breakpoint                */
    uint32_t BPSLEEP:1;        /*!< bit:      8  Sleep Breakpoint                   */
    uint32_t BPWDT:1;          /*!< bit:      9  Watchdog Timer Breakpoint          */
    uint32_t BPISA:1;          /*!< bit:     10  ISA Breakpoint                     */
    uint32_t :1;               /*!< bit:     11  Reserved                           */
    uint32_t BPCOMP:4;         /*!< bit: 12..15  Comparator Breakpoint              */
    uint32_t BPGENMODE:4;      /*!< bit: 16..19  Breakpoint Generator n Mode        */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_OCDCONTROL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_OCDCONTROL_OFFSET     0x054        /**< \brief (PICOP_OCDCONTROL offset) OCD Control */
#define PICOP_OCDCONTROL_RESETVALUE 0x00000000ul /**< \brief (PICOP_OCDCONTROL reset_value) OCD Control */

#define PICOP_OCDCONTROL_OCDEN_Pos  0            /**< \brief (PICOP_OCDCONTROL) OCD Enable */
#define PICOP_OCDCONTROL_OCDEN      (0x1ul << PICOP_OCDCONTROL_OCDEN_Pos)
#define PICOP_OCDCONTROL_BPSSTEP_Pos 2            /**< \brief (PICOP_OCDCONTROL) Single Step Breakpoint */
#define PICOP_OCDCONTROL_BPSSTEP    (0x1ul << PICOP_OCDCONTROL_BPSSTEP_Pos)
#define PICOP_OCDCONTROL_BPCOF_Pos  3            /**< \brief (PICOP_OCDCONTROL) Change of Flow Breakpoint */
#define PICOP_OCDCONTROL_BPCOF      (0x1ul << PICOP_OCDCONTROL_BPCOF_Pos)
#define PICOP_OCDCONTROL_BPRST_Pos  4            /**< \brief (PICOP_OCDCONTROL) Reset Breakpoint */
#define PICOP_OCDCONTROL_BPRST      (0x1ul << PICOP_OCDCONTROL_BPRST_Pos)
#define PICOP_OCDCONTROL_BPEXCEPTION_Pos 5            /**< \brief (PICOP_OCDCONTROL) Exception Breakpoint */
#define PICOP_OCDCONTROL_BPEXCEPTION (0x1ul << PICOP_OCDCONTROL_BPEXCEPTION_Pos)
#define PICOP_OCDCONTROL_BPIRQ_Pos  6            /**< \brief (PICOP_OCDCONTROL) Interrupt Request Breakpoint */
#define PICOP_OCDCONTROL_BPIRQ      (0x1ul << PICOP_OCDCONTROL_BPIRQ_Pos)
#define PICOP_OCDCONTROL_BPSW_Pos   7            /**< \brief (PICOP_OCDCONTROL) Software Breakpoint */
#define PICOP_OCDCONTROL_BPSW       (0x1ul << PICOP_OCDCONTROL_BPSW_Pos)
#define PICOP_OCDCONTROL_BPSLEEP_Pos 8            /**< \brief (PICOP_OCDCONTROL) Sleep Breakpoint */
#define PICOP_OCDCONTROL_BPSLEEP    (0x1ul << PICOP_OCDCONTROL_BPSLEEP_Pos)
#define PICOP_OCDCONTROL_BPWDT_Pos  9            /**< \brief (PICOP_OCDCONTROL) Watchdog Timer Breakpoint */
#define PICOP_OCDCONTROL_BPWDT      (0x1ul << PICOP_OCDCONTROL_BPWDT_Pos)
#define PICOP_OCDCONTROL_BPISA_Pos  10           /**< \brief (PICOP_OCDCONTROL) ISA Breakpoint */
#define PICOP_OCDCONTROL_BPISA      (0x1ul << PICOP_OCDCONTROL_BPISA_Pos)
#define PICOP_OCDCONTROL_BPCOMP_Pos 12           /**< \brief (PICOP_OCDCONTROL) Comparator Breakpoint */
#define PICOP_OCDCONTROL_BPCOMP_Msk (0xFul << PICOP_OCDCONTROL_BPCOMP_Pos)
#define PICOP_OCDCONTROL_BPCOMP(value) (PICOP_OCDCONTROL_BPCOMP_Msk & ((value) << PICOP_OCDCONTROL_BPCOMP_Pos))
#define PICOP_OCDCONTROL_BPGENMODE_Pos 16           /**< \brief (PICOP_OCDCONTROL) Breakpoint Generator n Mode */
#define PICOP_OCDCONTROL_BPGENMODE_Msk (0xFul << PICOP_OCDCONTROL_BPGENMODE_Pos)
#define PICOP_OCDCONTROL_BPGENMODE(value) (PICOP_OCDCONTROL_BPGENMODE_Msk & ((value) << PICOP_OCDCONTROL_BPGENMODE_Pos))
#define PICOP_OCDCONTROL_MASK       0x000FF7FDul /**< \brief (PICOP_OCDCONTROL) MASK Register */

/* -------- PICOP_OCDSTATUS : (PICOP Offset: 0x058) (R/W 32) OCD Status and Command -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // CMD mode
    uint32_t INST:16;          /*!< bit:  0..15  Instruction Override               */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } CMD;                       /*!< Structure used for CMD                          */
  struct { // STATUS mode
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t BPEXT:1;          /*!< bit:      1  External Breakpoint                */
    uint32_t BPSSTEP:1;        /*!< bit:      2  Single Step Breakpoint             */
    uint32_t BPCOF:1;          /*!< bit:      3  Change of Flow Breakpoint          */
    uint32_t BPRST:1;          /*!< bit:      4  Reset Breakpoint                   */
    uint32_t BPEXCEPTION:1;    /*!< bit:      5  Exception Breakpoint               */
    uint32_t BPIRQ:1;          /*!< bit:      6  Interrupt Request Breakpoint       */
    uint32_t BPSW:1;           /*!< bit:      7  Software Breakpoint                */
    uint32_t BPSLEEP:1;        /*!< bit:      8  Sleep Breakpoint                   */
    uint32_t BPWDT:1;          /*!< bit:      9  Watchdog Timer Breakpoint          */
    uint32_t BPISA:1;          /*!< bit:     10  ISA Breakpoint                     */
    uint32_t :1;               /*!< bit:     11  Reserved                           */
    uint32_t BPCOMP:4;         /*!< bit: 12..15  Comparator Breakpoint              */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } STATUS;                    /*!< Structure used for STATUS                       */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_OCDSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_OCDSTATUS_OFFSET      0x058        /**< \brief (PICOP_OCDSTATUS offset) OCD Status and Command */
#define PICOP_OCDSTATUS_RESETVALUE  0x00000000ul /**< \brief (PICOP_OCDSTATUS reset_value) OCD Status and Command */

// CMD mode
#define PICOP_OCDSTATUS_CMD_INST_Pos 0            /**< \brief (PICOP_OCDSTATUS_CMD) Instruction Override */
#define PICOP_OCDSTATUS_CMD_INST_Msk (0xFFFFul << PICOP_OCDSTATUS_CMD_INST_Pos)
#define PICOP_OCDSTATUS_CMD_INST(value) (PICOP_OCDSTATUS_CMD_INST_Msk & ((value) << PICOP_OCDSTATUS_CMD_INST_Pos))
#define PICOP_OCDSTATUS_CMD_MASK    0x0000FFFFul /**< \brief (PICOP_OCDSTATUS_CMD) MASK Register */

// STATUS mode
#define PICOP_OCDSTATUS_STATUS_BPEXT_Pos 1            /**< \brief (PICOP_OCDSTATUS_STATUS) External Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPEXT (0x1ul << PICOP_OCDSTATUS_STATUS_BPEXT_Pos)
#define PICOP_OCDSTATUS_STATUS_BPSSTEP_Pos 2            /**< \brief (PICOP_OCDSTATUS_STATUS) Single Step Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPSSTEP (0x1ul << PICOP_OCDSTATUS_STATUS_BPSSTEP_Pos)
#define PICOP_OCDSTATUS_STATUS_BPCOF_Pos 3            /**< \brief (PICOP_OCDSTATUS_STATUS) Change of Flow Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPCOF (0x1ul << PICOP_OCDSTATUS_STATUS_BPCOF_Pos)
#define PICOP_OCDSTATUS_STATUS_BPRST_Pos 4            /**< \brief (PICOP_OCDSTATUS_STATUS) Reset Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPRST (0x1ul << PICOP_OCDSTATUS_STATUS_BPRST_Pos)
#define PICOP_OCDSTATUS_STATUS_BPEXCEPTION_Pos 5            /**< \brief (PICOP_OCDSTATUS_STATUS) Exception Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPEXCEPTION (0x1ul << PICOP_OCDSTATUS_STATUS_BPEXCEPTION_Pos)
#define PICOP_OCDSTATUS_STATUS_BPIRQ_Pos 6            /**< \brief (PICOP_OCDSTATUS_STATUS) Interrupt Request Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPIRQ (0x1ul << PICOP_OCDSTATUS_STATUS_BPIRQ_Pos)
#define PICOP_OCDSTATUS_STATUS_BPSW_Pos 7            /**< \brief (PICOP_OCDSTATUS_STATUS) Software Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPSW (0x1ul << PICOP_OCDSTATUS_STATUS_BPSW_Pos)
#define PICOP_OCDSTATUS_STATUS_BPSLEEP_Pos 8            /**< \brief (PICOP_OCDSTATUS_STATUS) Sleep Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPSLEEP (0x1ul << PICOP_OCDSTATUS_STATUS_BPSLEEP_Pos)
#define PICOP_OCDSTATUS_STATUS_BPWDT_Pos 9            /**< \brief (PICOP_OCDSTATUS_STATUS) Watchdog Timer Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPWDT (0x1ul << PICOP_OCDSTATUS_STATUS_BPWDT_Pos)
#define PICOP_OCDSTATUS_STATUS_BPISA_Pos 10           /**< \brief (PICOP_OCDSTATUS_STATUS) ISA Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPISA (0x1ul << PICOP_OCDSTATUS_STATUS_BPISA_Pos)
#define PICOP_OCDSTATUS_STATUS_BPCOMP_Pos 12           /**< \brief (PICOP_OCDSTATUS_STATUS) Comparator Breakpoint */
#define PICOP_OCDSTATUS_STATUS_BPCOMP_Msk (0xFul << PICOP_OCDSTATUS_STATUS_BPCOMP_Pos)
#define PICOP_OCDSTATUS_STATUS_BPCOMP(value) (PICOP_OCDSTATUS_STATUS_BPCOMP_Msk & ((value) << PICOP_OCDSTATUS_STATUS_BPCOMP_Pos))
#define PICOP_OCDSTATUS_STATUS_MASK 0x0000F7FEul /**< \brief (PICOP_OCDSTATUS_STATUS) MASK Register */

/* -------- PICOP_OCDPC : (PICOP Offset: 0x05C) (R/W 32) ODC Program Counter -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PC:16;            /*!< bit:  0..15  Program Counter                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_OCDPC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_OCDPC_OFFSET          0x05C        /**< \brief (PICOP_OCDPC offset) ODC Program Counter */

#define PICOP_OCDPC_PC_Pos          0            /**< \brief (PICOP_OCDPC) Program Counter */
#define PICOP_OCDPC_PC_Msk          (0xFFFFul << PICOP_OCDPC_PC_Pos)
#define PICOP_OCDPC_PC(value)       (PICOP_OCDPC_PC_Msk & ((value) << PICOP_OCDPC_PC_Pos))
#define PICOP_OCDPC_MASK            0x0000FFFFul /**< \brief (PICOP_OCDPC) MASK Register */

/* -------- PICOP_OCDFEAT : (PICOP Offset: 0x060) (R/W 32) OCD Features -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CCNT:2;           /*!< bit:  0.. 1  Cycle Counter                      */
    uint32_t BPGEN:2;          /*!< bit:  2.. 3  Breakpoint Generators              */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_OCDFEAT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_OCDFEAT_OFFSET        0x060        /**< \brief (PICOP_OCDFEAT offset) OCD Features */
#define PICOP_OCDFEAT_RESETVALUE    0x00000000ul /**< \brief (PICOP_OCDFEAT reset_value) OCD Features */

#define PICOP_OCDFEAT_CCNT_Pos      0            /**< \brief (PICOP_OCDFEAT) Cycle Counter */
#define PICOP_OCDFEAT_CCNT_Msk      (0x3ul << PICOP_OCDFEAT_CCNT_Pos)
#define PICOP_OCDFEAT_CCNT(value)   (PICOP_OCDFEAT_CCNT_Msk & ((value) << PICOP_OCDFEAT_CCNT_Pos))
#define PICOP_OCDFEAT_BPGEN_Pos     2            /**< \brief (PICOP_OCDFEAT) Breakpoint Generators */
#define PICOP_OCDFEAT_BPGEN_Msk     (0x3ul << PICOP_OCDFEAT_BPGEN_Pos)
#define PICOP_OCDFEAT_BPGEN(value)  (PICOP_OCDFEAT_BPGEN_Msk & ((value) << PICOP_OCDFEAT_BPGEN_Pos))
#define PICOP_OCDFEAT_MASK          0x0000000Ful /**< \brief (PICOP_OCDFEAT) MASK Register */

/* -------- PICOP_OCDCCNT : (PICOP Offset: 0x068) (R/W 32) OCD Cycle Counter -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CCNT:32;          /*!< bit:  0..31  Cycle Count                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_OCDCCNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_OCDCCNT_OFFSET        0x068        /**< \brief (PICOP_OCDCCNT offset) OCD Cycle Counter */
#define PICOP_OCDCCNT_RESETVALUE    0x00000000ul /**< \brief (PICOP_OCDCCNT reset_value) OCD Cycle Counter */

#define PICOP_OCDCCNT_CCNT_Pos      0            /**< \brief (PICOP_OCDCCNT) Cycle Count */
#define PICOP_OCDCCNT_CCNT_Msk      (0xFFFFFFFFul << PICOP_OCDCCNT_CCNT_Pos)
#define PICOP_OCDCCNT_CCNT(value)   (PICOP_OCDCCNT_CCNT_Msk & ((value) << PICOP_OCDCCNT_CCNT_Pos))
#define PICOP_OCDCCNT_MASK          0xFFFFFFFFul /**< \brief (PICOP_OCDCCNT) MASK Register */

/* -------- PICOP_OCDBPGEN : (PICOP Offset: 0x070) (R/W 32) OCD Breakpoint Generator n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BPGEN:16;         /*!< bit:  0..15  Breakpoint Generator               */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_OCDBPGEN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_OCDBPGEN_OFFSET       0x070        /**< \brief (PICOP_OCDBPGEN offset) OCD Breakpoint Generator n */
#define PICOP_OCDBPGEN_RESETVALUE   0x00000000ul /**< \brief (PICOP_OCDBPGEN reset_value) OCD Breakpoint Generator n */

#define PICOP_OCDBPGEN_BPGEN_Pos    0            /**< \brief (PICOP_OCDBPGEN) Breakpoint Generator */
#define PICOP_OCDBPGEN_BPGEN_Msk    (0xFFFFul << PICOP_OCDBPGEN_BPGEN_Pos)
#define PICOP_OCDBPGEN_BPGEN(value) (PICOP_OCDBPGEN_BPGEN_Msk & ((value) << PICOP_OCDBPGEN_BPGEN_Pos))
#define PICOP_OCDBPGEN_MASK         0x0000FFFFul /**< \brief (PICOP_OCDBPGEN) MASK Register */

/* -------- PICOP_R3R0 : (PICOP Offset: 0x080) (R/W 32) R3 to 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R3R0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R3R0_OFFSET           0x080        /**< \brief (PICOP_R3R0 offset) R3 to 0 */

#define PICOP_R3R0_R0_Pos           0            /**< \brief (PICOP_R3R0) Register 0 */
#define PICOP_R3R0_R0_Msk           (0xFFul << PICOP_R3R0_R0_Pos)
#define PICOP_R3R0_R0(value)        (PICOP_R3R0_R0_Msk & ((value) << PICOP_R3R0_R0_Pos))
#define PICOP_R3R0_R1_Pos           8            /**< \brief (PICOP_R3R0) Register 1 */
#define PICOP_R3R0_R1_Msk           (0xFFul << PICOP_R3R0_R1_Pos)
#define PICOP_R3R0_R1(value)        (PICOP_R3R0_R1_Msk & ((value) << PICOP_R3R0_R1_Pos))
#define PICOP_R3R0_R2_Pos           16           /**< \brief (PICOP_R3R0) Register 2 */
#define PICOP_R3R0_R2_Msk           (0xFFul << PICOP_R3R0_R2_Pos)
#define PICOP_R3R0_R2(value)        (PICOP_R3R0_R2_Msk & ((value) << PICOP_R3R0_R2_Pos))
#define PICOP_R3R0_R3_Pos           24           /**< \brief (PICOP_R3R0) Register 3 */
#define PICOP_R3R0_R3_Msk           (0xFFul << PICOP_R3R0_R3_Pos)
#define PICOP_R3R0_R3(value)        (PICOP_R3R0_R3_Msk & ((value) << PICOP_R3R0_R3_Pos))
#define PICOP_R3R0_MASK             0xFFFFFFFFul /**< \brief (PICOP_R3R0) MASK Register */

/* -------- PICOP_R7R4 : (PICOP Offset: 0x084) (R/W 32) R7 to 4 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R7R4_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R7R4_OFFSET           0x084        /**< \brief (PICOP_R7R4 offset) R7 to 4 */

#define PICOP_R7R4_R0_Pos           0            /**< \brief (PICOP_R7R4) Register 0 */
#define PICOP_R7R4_R0_Msk           (0xFFul << PICOP_R7R4_R0_Pos)
#define PICOP_R7R4_R0(value)        (PICOP_R7R4_R0_Msk & ((value) << PICOP_R7R4_R0_Pos))
#define PICOP_R7R4_R1_Pos           8            /**< \brief (PICOP_R7R4) Register 1 */
#define PICOP_R7R4_R1_Msk           (0xFFul << PICOP_R7R4_R1_Pos)
#define PICOP_R7R4_R1(value)        (PICOP_R7R4_R1_Msk & ((value) << PICOP_R7R4_R1_Pos))
#define PICOP_R7R4_R2_Pos           16           /**< \brief (PICOP_R7R4) Register 2 */
#define PICOP_R7R4_R2_Msk           (0xFFul << PICOP_R7R4_R2_Pos)
#define PICOP_R7R4_R2(value)        (PICOP_R7R4_R2_Msk & ((value) << PICOP_R7R4_R2_Pos))
#define PICOP_R7R4_R3_Pos           24           /**< \brief (PICOP_R7R4) Register 3 */
#define PICOP_R7R4_R3_Msk           (0xFFul << PICOP_R7R4_R3_Pos)
#define PICOP_R7R4_R3(value)        (PICOP_R7R4_R3_Msk & ((value) << PICOP_R7R4_R3_Pos))
#define PICOP_R7R4_MASK             0xFFFFFFFFul /**< \brief (PICOP_R7R4) MASK Register */

/* -------- PICOP_R11R8 : (PICOP Offset: 0x088) (R/W 32) R11 to 8 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R11R8_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R11R8_OFFSET          0x088        /**< \brief (PICOP_R11R8 offset) R11 to 8 */

#define PICOP_R11R8_R0_Pos          0            /**< \brief (PICOP_R11R8) Register 0 */
#define PICOP_R11R8_R0_Msk          (0xFFul << PICOP_R11R8_R0_Pos)
#define PICOP_R11R8_R0(value)       (PICOP_R11R8_R0_Msk & ((value) << PICOP_R11R8_R0_Pos))
#define PICOP_R11R8_R1_Pos          8            /**< \brief (PICOP_R11R8) Register 1 */
#define PICOP_R11R8_R1_Msk          (0xFFul << PICOP_R11R8_R1_Pos)
#define PICOP_R11R8_R1(value)       (PICOP_R11R8_R1_Msk & ((value) << PICOP_R11R8_R1_Pos))
#define PICOP_R11R8_R2_Pos          16           /**< \brief (PICOP_R11R8) Register 2 */
#define PICOP_R11R8_R2_Msk          (0xFFul << PICOP_R11R8_R2_Pos)
#define PICOP_R11R8_R2(value)       (PICOP_R11R8_R2_Msk & ((value) << PICOP_R11R8_R2_Pos))
#define PICOP_R11R8_R3_Pos          24           /**< \brief (PICOP_R11R8) Register 3 */
#define PICOP_R11R8_R3_Msk          (0xFFul << PICOP_R11R8_R3_Pos)
#define PICOP_R11R8_R3(value)       (PICOP_R11R8_R3_Msk & ((value) << PICOP_R11R8_R3_Pos))
#define PICOP_R11R8_MASK            0xFFFFFFFFul /**< \brief (PICOP_R11R8) MASK Register */

/* -------- PICOP_R15R12 : (PICOP Offset: 0x08C) (R/W 32) R15 to 12 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R15R12_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R15R12_OFFSET         0x08C        /**< \brief (PICOP_R15R12 offset) R15 to 12 */

#define PICOP_R15R12_R0_Pos         0            /**< \brief (PICOP_R15R12) Register 0 */
#define PICOP_R15R12_R0_Msk         (0xFFul << PICOP_R15R12_R0_Pos)
#define PICOP_R15R12_R0(value)      (PICOP_R15R12_R0_Msk & ((value) << PICOP_R15R12_R0_Pos))
#define PICOP_R15R12_R1_Pos         8            /**< \brief (PICOP_R15R12) Register 1 */
#define PICOP_R15R12_R1_Msk         (0xFFul << PICOP_R15R12_R1_Pos)
#define PICOP_R15R12_R1(value)      (PICOP_R15R12_R1_Msk & ((value) << PICOP_R15R12_R1_Pos))
#define PICOP_R15R12_R2_Pos         16           /**< \brief (PICOP_R15R12) Register 2 */
#define PICOP_R15R12_R2_Msk         (0xFFul << PICOP_R15R12_R2_Pos)
#define PICOP_R15R12_R2(value)      (PICOP_R15R12_R2_Msk & ((value) << PICOP_R15R12_R2_Pos))
#define PICOP_R15R12_R3_Pos         24           /**< \brief (PICOP_R15R12) Register 3 */
#define PICOP_R15R12_R3_Msk         (0xFFul << PICOP_R15R12_R3_Pos)
#define PICOP_R15R12_R3(value)      (PICOP_R15R12_R3_Msk & ((value) << PICOP_R15R12_R3_Pos))
#define PICOP_R15R12_MASK           0xFFFFFFFFul /**< \brief (PICOP_R15R12) MASK Register */

/* -------- PICOP_R19R16 : (PICOP Offset: 0x090) (R/W 32) R19 to 16 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R19R16_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R19R16_OFFSET         0x090        /**< \brief (PICOP_R19R16 offset) R19 to 16 */

#define PICOP_R19R16_R0_Pos         0            /**< \brief (PICOP_R19R16) Register 0 */
#define PICOP_R19R16_R0_Msk         (0xFFul << PICOP_R19R16_R0_Pos)
#define PICOP_R19R16_R0(value)      (PICOP_R19R16_R0_Msk & ((value) << PICOP_R19R16_R0_Pos))
#define PICOP_R19R16_R1_Pos         8            /**< \brief (PICOP_R19R16) Register 1 */
#define PICOP_R19R16_R1_Msk         (0xFFul << PICOP_R19R16_R1_Pos)
#define PICOP_R19R16_R1(value)      (PICOP_R19R16_R1_Msk & ((value) << PICOP_R19R16_R1_Pos))
#define PICOP_R19R16_R2_Pos         16           /**< \brief (PICOP_R19R16) Register 2 */
#define PICOP_R19R16_R2_Msk         (0xFFul << PICOP_R19R16_R2_Pos)
#define PICOP_R19R16_R2(value)      (PICOP_R19R16_R2_Msk & ((value) << PICOP_R19R16_R2_Pos))
#define PICOP_R19R16_R3_Pos         24           /**< \brief (PICOP_R19R16) Register 3 */
#define PICOP_R19R16_R3_Msk         (0xFFul << PICOP_R19R16_R3_Pos)
#define PICOP_R19R16_R3(value)      (PICOP_R19R16_R3_Msk & ((value) << PICOP_R19R16_R3_Pos))
#define PICOP_R19R16_MASK           0xFFFFFFFFul /**< \brief (PICOP_R19R16) MASK Register */

/* -------- PICOP_R23R20 : (PICOP Offset: 0x094) (R/W 32) R23 to 20 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R23R20_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R23R20_OFFSET         0x094        /**< \brief (PICOP_R23R20 offset) R23 to 20 */

#define PICOP_R23R20_R0_Pos         0            /**< \brief (PICOP_R23R20) Register 0 */
#define PICOP_R23R20_R0_Msk         (0xFFul << PICOP_R23R20_R0_Pos)
#define PICOP_R23R20_R0(value)      (PICOP_R23R20_R0_Msk & ((value) << PICOP_R23R20_R0_Pos))
#define PICOP_R23R20_R1_Pos         8            /**< \brief (PICOP_R23R20) Register 1 */
#define PICOP_R23R20_R1_Msk         (0xFFul << PICOP_R23R20_R1_Pos)
#define PICOP_R23R20_R1(value)      (PICOP_R23R20_R1_Msk & ((value) << PICOP_R23R20_R1_Pos))
#define PICOP_R23R20_R2_Pos         16           /**< \brief (PICOP_R23R20) Register 2 */
#define PICOP_R23R20_R2_Msk         (0xFFul << PICOP_R23R20_R2_Pos)
#define PICOP_R23R20_R2(value)      (PICOP_R23R20_R2_Msk & ((value) << PICOP_R23R20_R2_Pos))
#define PICOP_R23R20_R3_Pos         24           /**< \brief (PICOP_R23R20) Register 3 */
#define PICOP_R23R20_R3_Msk         (0xFFul << PICOP_R23R20_R3_Pos)
#define PICOP_R23R20_R3(value)      (PICOP_R23R20_R3_Msk & ((value) << PICOP_R23R20_R3_Pos))
#define PICOP_R23R20_MASK           0xFFFFFFFFul /**< \brief (PICOP_R23R20) MASK Register */

/* -------- PICOP_R27R24 : (PICOP Offset: 0x098) (R/W 32) R27 to 24: XH, XL, R25, R24 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R27R24_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R27R24_OFFSET         0x098        /**< \brief (PICOP_R27R24 offset) R27 to 24: XH, XL, R25, R24 */

#define PICOP_R27R24_R0_Pos         0            /**< \brief (PICOP_R27R24) Register 0 */
#define PICOP_R27R24_R0_Msk         (0xFFul << PICOP_R27R24_R0_Pos)
#define PICOP_R27R24_R0(value)      (PICOP_R27R24_R0_Msk & ((value) << PICOP_R27R24_R0_Pos))
#define PICOP_R27R24_R1_Pos         8            /**< \brief (PICOP_R27R24) Register 1 */
#define PICOP_R27R24_R1_Msk         (0xFFul << PICOP_R27R24_R1_Pos)
#define PICOP_R27R24_R1(value)      (PICOP_R27R24_R1_Msk & ((value) << PICOP_R27R24_R1_Pos))
#define PICOP_R27R24_R2_Pos         16           /**< \brief (PICOP_R27R24) Register 2 */
#define PICOP_R27R24_R2_Msk         (0xFFul << PICOP_R27R24_R2_Pos)
#define PICOP_R27R24_R2(value)      (PICOP_R27R24_R2_Msk & ((value) << PICOP_R27R24_R2_Pos))
#define PICOP_R27R24_R3_Pos         24           /**< \brief (PICOP_R27R24) Register 3 */
#define PICOP_R27R24_R3_Msk         (0xFFul << PICOP_R27R24_R3_Pos)
#define PICOP_R27R24_R3(value)      (PICOP_R27R24_R3_Msk & ((value) << PICOP_R27R24_R3_Pos))
#define PICOP_R27R24_MASK           0xFFFFFFFFul /**< \brief (PICOP_R27R24) MASK Register */

/* -------- PICOP_R31R28 : (PICOP Offset: 0x09C) (R/W 32) R31 to 28: ZH, ZL, YH, YL -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_R31R28_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_R31R28_OFFSET         0x09C        /**< \brief (PICOP_R31R28 offset) R31 to 28: ZH, ZL, YH, YL */

#define PICOP_R31R28_R0_Pos         0            /**< \brief (PICOP_R31R28) Register 0 */
#define PICOP_R31R28_R0_Msk         (0xFFul << PICOP_R31R28_R0_Pos)
#define PICOP_R31R28_R0(value)      (PICOP_R31R28_R0_Msk & ((value) << PICOP_R31R28_R0_Pos))
#define PICOP_R31R28_R1_Pos         8            /**< \brief (PICOP_R31R28) Register 1 */
#define PICOP_R31R28_R1_Msk         (0xFFul << PICOP_R31R28_R1_Pos)
#define PICOP_R31R28_R1(value)      (PICOP_R31R28_R1_Msk & ((value) << PICOP_R31R28_R1_Pos))
#define PICOP_R31R28_R2_Pos         16           /**< \brief (PICOP_R31R28) Register 2 */
#define PICOP_R31R28_R2_Msk         (0xFFul << PICOP_R31R28_R2_Pos)
#define PICOP_R31R28_R2(value)      (PICOP_R31R28_R2_Msk & ((value) << PICOP_R31R28_R2_Pos))
#define PICOP_R31R28_R3_Pos         24           /**< \brief (PICOP_R31R28) Register 3 */
#define PICOP_R31R28_R3_Msk         (0xFFul << PICOP_R31R28_R3_Pos)
#define PICOP_R31R28_R3(value)      (PICOP_R31R28_R3_Msk & ((value) << PICOP_R31R28_R3_Pos))
#define PICOP_R31R28_MASK           0xFFFFFFFFul /**< \brief (PICOP_R31R28) MASK Register */

/* -------- PICOP_S1S0 : (PICOP Offset: 0x0A0) (R/W 32) System Regs 1 to 0: SR -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_S1S0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_S1S0_OFFSET           0x0A0        /**< \brief (PICOP_S1S0 offset) System Regs 1 to 0: SR */

#define PICOP_S1S0_R0_Pos           0            /**< \brief (PICOP_S1S0) Register 0 */
#define PICOP_S1S0_R0_Msk           (0xFFul << PICOP_S1S0_R0_Pos)
#define PICOP_S1S0_R0(value)        (PICOP_S1S0_R0_Msk & ((value) << PICOP_S1S0_R0_Pos))
#define PICOP_S1S0_R1_Pos           8            /**< \brief (PICOP_S1S0) Register 1 */
#define PICOP_S1S0_R1_Msk           (0xFFul << PICOP_S1S0_R1_Pos)
#define PICOP_S1S0_R1(value)        (PICOP_S1S0_R1_Msk & ((value) << PICOP_S1S0_R1_Pos))
#define PICOP_S1S0_R2_Pos           16           /**< \brief (PICOP_S1S0) Register 2 */
#define PICOP_S1S0_R2_Msk           (0xFFul << PICOP_S1S0_R2_Pos)
#define PICOP_S1S0_R2(value)        (PICOP_S1S0_R2_Msk & ((value) << PICOP_S1S0_R2_Pos))
#define PICOP_S1S0_R3_Pos           24           /**< \brief (PICOP_S1S0) Register 3 */
#define PICOP_S1S0_R3_Msk           (0xFFul << PICOP_S1S0_R3_Pos)
#define PICOP_S1S0_R3(value)        (PICOP_S1S0_R3_Msk & ((value) << PICOP_S1S0_R3_Pos))
#define PICOP_S1S0_MASK             0xFFFFFFFFul /**< \brief (PICOP_S1S0) MASK Register */

/* -------- PICOP_S3S2 : (PICOP Offset: 0x0A4) (R/W 32) System Regs 3 to 2: CTRL -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_S3S2_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_S3S2_OFFSET           0x0A4        /**< \brief (PICOP_S3S2 offset) System Regs 3 to 2: CTRL */

#define PICOP_S3S2_R0_Pos           0            /**< \brief (PICOP_S3S2) Register 0 */
#define PICOP_S3S2_R0_Msk           (0xFFul << PICOP_S3S2_R0_Pos)
#define PICOP_S3S2_R0(value)        (PICOP_S3S2_R0_Msk & ((value) << PICOP_S3S2_R0_Pos))
#define PICOP_S3S2_R1_Pos           8            /**< \brief (PICOP_S3S2) Register 1 */
#define PICOP_S3S2_R1_Msk           (0xFFul << PICOP_S3S2_R1_Pos)
#define PICOP_S3S2_R1(value)        (PICOP_S3S2_R1_Msk & ((value) << PICOP_S3S2_R1_Pos))
#define PICOP_S3S2_R2_Pos           16           /**< \brief (PICOP_S3S2) Register 2 */
#define PICOP_S3S2_R2_Msk           (0xFFul << PICOP_S3S2_R2_Pos)
#define PICOP_S3S2_R2(value)        (PICOP_S3S2_R2_Msk & ((value) << PICOP_S3S2_R2_Pos))
#define PICOP_S3S2_R3_Pos           24           /**< \brief (PICOP_S3S2) Register 3 */
#define PICOP_S3S2_R3_Msk           (0xFFul << PICOP_S3S2_R3_Pos)
#define PICOP_S3S2_R3(value)        (PICOP_S3S2_R3_Msk & ((value) << PICOP_S3S2_R3_Pos))
#define PICOP_S3S2_MASK             0xFFFFFFFFul /**< \brief (PICOP_S3S2) MASK Register */

/* -------- PICOP_S5S4 : (PICOP Offset: 0x0A8) (R/W 32) System Regs 5 to 4: SREG, CCR -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_S5S4_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_S5S4_OFFSET           0x0A8        /**< \brief (PICOP_S5S4 offset) System Regs 5 to 4: SREG, CCR */

#define PICOP_S5S4_R0_Pos           0            /**< \brief (PICOP_S5S4) Register 0 */
#define PICOP_S5S4_R0_Msk           (0xFFul << PICOP_S5S4_R0_Pos)
#define PICOP_S5S4_R0(value)        (PICOP_S5S4_R0_Msk & ((value) << PICOP_S5S4_R0_Pos))
#define PICOP_S5S4_R1_Pos           8            /**< \brief (PICOP_S5S4) Register 1 */
#define PICOP_S5S4_R1_Msk           (0xFFul << PICOP_S5S4_R1_Pos)
#define PICOP_S5S4_R1(value)        (PICOP_S5S4_R1_Msk & ((value) << PICOP_S5S4_R1_Pos))
#define PICOP_S5S4_R2_Pos           16           /**< \brief (PICOP_S5S4) Register 2 */
#define PICOP_S5S4_R2_Msk           (0xFFul << PICOP_S5S4_R2_Pos)
#define PICOP_S5S4_R2(value)        (PICOP_S5S4_R2_Msk & ((value) << PICOP_S5S4_R2_Pos))
#define PICOP_S5S4_R3_Pos           24           /**< \brief (PICOP_S5S4) Register 3 */
#define PICOP_S5S4_R3_Msk           (0xFFul << PICOP_S5S4_R3_Pos)
#define PICOP_S5S4_R3(value)        (PICOP_S5S4_R3_Msk & ((value) << PICOP_S5S4_R3_Pos))
#define PICOP_S5S4_MASK             0xFFFFFFFFul /**< \brief (PICOP_S5S4) MASK Register */

/* -------- PICOP_S11S10 : (PICOP Offset: 0x0B4) (R/W 32) System Regs 11 to 10: Immediate -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_S11S10_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_S11S10_OFFSET         0x0B4        /**< \brief (PICOP_S11S10 offset) System Regs 11 to 10: Immediate */

#define PICOP_S11S10_R0_Pos         0            /**< \brief (PICOP_S11S10) Register 0 */
#define PICOP_S11S10_R0_Msk         (0xFFul << PICOP_S11S10_R0_Pos)
#define PICOP_S11S10_R0(value)      (PICOP_S11S10_R0_Msk & ((value) << PICOP_S11S10_R0_Pos))
#define PICOP_S11S10_R1_Pos         8            /**< \brief (PICOP_S11S10) Register 1 */
#define PICOP_S11S10_R1_Msk         (0xFFul << PICOP_S11S10_R1_Pos)
#define PICOP_S11S10_R1(value)      (PICOP_S11S10_R1_Msk & ((value) << PICOP_S11S10_R1_Pos))
#define PICOP_S11S10_R2_Pos         16           /**< \brief (PICOP_S11S10) Register 2 */
#define PICOP_S11S10_R2_Msk         (0xFFul << PICOP_S11S10_R2_Pos)
#define PICOP_S11S10_R2(value)      (PICOP_S11S10_R2_Msk & ((value) << PICOP_S11S10_R2_Pos))
#define PICOP_S11S10_R3_Pos         24           /**< \brief (PICOP_S11S10) Register 3 */
#define PICOP_S11S10_R3_Msk         (0xFFul << PICOP_S11S10_R3_Pos)
#define PICOP_S11S10_R3(value)      (PICOP_S11S10_R3_Msk & ((value) << PICOP_S11S10_R3_Pos))
#define PICOP_S11S10_MASK           0xFFFFFFFFul /**< \brief (PICOP_S11S10) MASK Register */

/* -------- PICOP_LINK : (PICOP Offset: 0x0B8) (R/W 32) Link -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_LINK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_LINK_OFFSET           0x0B8        /**< \brief (PICOP_LINK offset) Link */
#define PICOP_LINK_MASK             0xFFFFFFFFul /**< \brief (PICOP_LINK) MASK Register */

/* -------- PICOP_SP : (PICOP Offset: 0x0BC) (R/W 32) Stack Pointer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t R0:8;             /*!< bit:  0.. 7  Register 0                         */
    uint32_t R1:8;             /*!< bit:  8..15  Register 1                         */
    uint32_t R2:8;             /*!< bit: 16..23  Register 2                         */
    uint32_t R3:8;             /*!< bit: 24..31  Register 3                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_SP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_SP_OFFSET             0x0BC        /**< \brief (PICOP_SP offset) Stack Pointer */

#define PICOP_SP_R0_Pos             0            /**< \brief (PICOP_SP) Register 0 */
#define PICOP_SP_R0_Msk             (0xFFul << PICOP_SP_R0_Pos)
#define PICOP_SP_R0(value)          (PICOP_SP_R0_Msk & ((value) << PICOP_SP_R0_Pos))
#define PICOP_SP_R1_Pos             8            /**< \brief (PICOP_SP) Register 1 */
#define PICOP_SP_R1_Msk             (0xFFul << PICOP_SP_R1_Pos)
#define PICOP_SP_R1(value)          (PICOP_SP_R1_Msk & ((value) << PICOP_SP_R1_Pos))
#define PICOP_SP_R2_Pos             16           /**< \brief (PICOP_SP) Register 2 */
#define PICOP_SP_R2_Msk             (0xFFul << PICOP_SP_R2_Pos)
#define PICOP_SP_R2(value)          (PICOP_SP_R2_Msk & ((value) << PICOP_SP_R2_Pos))
#define PICOP_SP_R3_Pos             24           /**< \brief (PICOP_SP) Register 3 */
#define PICOP_SP_R3_Msk             (0xFFul << PICOP_SP_R3_Pos)
#define PICOP_SP_R3(value)          (PICOP_SP_R3_Msk & ((value) << PICOP_SP_R3_Pos))
#define PICOP_SP_MASK               0xFFFFFFFFul /**< \brief (PICOP_SP) MASK Register */

/* -------- PICOP_MMUFLASH : (PICOP Offset: 0x100) (R/W 32) MMU mapping for flash -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDRESS:4;        /*!< bit:  0.. 3  MMU Flash Address                  */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_MMUFLASH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_MMUFLASH_OFFSET       0x100        /**< \brief (PICOP_MMUFLASH offset) MMU mapping for flash */
#define PICOP_MMUFLASH_RESETVALUE   0x00000000ul /**< \brief (PICOP_MMUFLASH reset_value) MMU mapping for flash */

#define PICOP_MMUFLASH_ADDRESS_Pos  0            /**< \brief (PICOP_MMUFLASH) MMU Flash Address */
#define PICOP_MMUFLASH_ADDRESS_Msk  (0xFul << PICOP_MMUFLASH_ADDRESS_Pos)
#define PICOP_MMUFLASH_ADDRESS(value) (PICOP_MMUFLASH_ADDRESS_Msk & ((value) << PICOP_MMUFLASH_ADDRESS_Pos))
#define PICOP_MMUFLASH_MASK         0x0000000Ful /**< \brief (PICOP_MMUFLASH) MASK Register */

/* -------- PICOP_MMU0 : (PICOP Offset: 0x118) (R/W 32) MMU mapping user 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDRESS:32;       /*!< bit:  0..31  MMU User 0 Address                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_MMU0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_MMU0_OFFSET           0x118        /**< \brief (PICOP_MMU0 offset) MMU mapping user 0 */
#define PICOP_MMU0_RESETVALUE       0x00000000ul /**< \brief (PICOP_MMU0 reset_value) MMU mapping user 0 */

#define PICOP_MMU0_ADDRESS_Pos      0            /**< \brief (PICOP_MMU0) MMU User 0 Address */
#define PICOP_MMU0_ADDRESS_Msk      (0xFFFFFFFFul << PICOP_MMU0_ADDRESS_Pos)
#define PICOP_MMU0_ADDRESS(value)   (PICOP_MMU0_ADDRESS_Msk & ((value) << PICOP_MMU0_ADDRESS_Pos))
#define PICOP_MMU0_MASK             0xFFFFFFFFul /**< \brief (PICOP_MMU0) MASK Register */

/* -------- PICOP_MMU1 : (PICOP Offset: 0x11C) (R/W 32) MMU mapping user 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDRESS:32;       /*!< bit:  0..31  MMU User 1 Address                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_MMU1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_MMU1_OFFSET           0x11C        /**< \brief (PICOP_MMU1 offset) MMU mapping user 1 */
#define PICOP_MMU1_RESETVALUE       0x00000000ul /**< \brief (PICOP_MMU1 reset_value) MMU mapping user 1 */

#define PICOP_MMU1_ADDRESS_Pos      0            /**< \brief (PICOP_MMU1) MMU User 1 Address */
#define PICOP_MMU1_ADDRESS_Msk      (0xFFFFFFFFul << PICOP_MMU1_ADDRESS_Pos)
#define PICOP_MMU1_ADDRESS(value)   (PICOP_MMU1_ADDRESS_Msk & ((value) << PICOP_MMU1_ADDRESS_Pos))
#define PICOP_MMU1_MASK             0xFFFFFFFFul /**< \brief (PICOP_MMU1) MASK Register */

/* -------- PICOP_MMUCTRL : (PICOP Offset: 0x120) (R/W 32) MMU Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t IODIS:1;          /*!< bit:      0  Peripheral MMU Disable             */
    uint32_t MEMDIS:1;         /*!< bit:      1  Memory MMU Disable                 */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_MMUCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_MMUCTRL_OFFSET        0x120        /**< \brief (PICOP_MMUCTRL offset) MMU Control */
#define PICOP_MMUCTRL_RESETVALUE    0x00000000ul /**< \brief (PICOP_MMUCTRL reset_value) MMU Control */

#define PICOP_MMUCTRL_IODIS_Pos     0            /**< \brief (PICOP_MMUCTRL) Peripheral MMU Disable */
#define PICOP_MMUCTRL_IODIS         (0x1ul << PICOP_MMUCTRL_IODIS_Pos)
#define PICOP_MMUCTRL_MEMDIS_Pos    1            /**< \brief (PICOP_MMUCTRL) Memory MMU Disable */
#define PICOP_MMUCTRL_MEMDIS        (0x1ul << PICOP_MMUCTRL_MEMDIS_Pos)
#define PICOP_MMUCTRL_MASK          0x00000003ul /**< \brief (PICOP_MMUCTRL) MASK Register */

/* -------- PICOP_ICACHE : (PICOP Offset: 0x180) (R/W 32) Instruction Cache Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CTRL:2;           /*!< bit:  0.. 1  Instruction Cache Control          */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_ICACHE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_ICACHE_OFFSET         0x180        /**< \brief (PICOP_ICACHE offset) Instruction Cache Control */
#define PICOP_ICACHE_RESETVALUE     0x00000000ul /**< \brief (PICOP_ICACHE reset_value) Instruction Cache Control */

#define PICOP_ICACHE_CTRL_Pos       0            /**< \brief (PICOP_ICACHE) Instruction Cache Control */
#define PICOP_ICACHE_CTRL_Msk       (0x3ul << PICOP_ICACHE_CTRL_Pos)
#define PICOP_ICACHE_CTRL(value)    (PICOP_ICACHE_CTRL_Msk & ((value) << PICOP_ICACHE_CTRL_Pos))
#define PICOP_ICACHE_MASK           0x00000003ul /**< \brief (PICOP_ICACHE) MASK Register */

/* -------- PICOP_ICACHELRU : (PICOP Offset: 0x184) (R/W 32) Instruction Cache LRU -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LRU0:2;           /*!< bit:  0.. 1  Instruction Cache LRU 0            */
    uint32_t LRU1:2;           /*!< bit:  2.. 3  Instruction Cache LRU 1            */
    uint32_t LRU2:2;           /*!< bit:  4.. 5  Instruction Cache LRU 2            */
    uint32_t LRU3:2;           /*!< bit:  6.. 7  Instruction Cache LRU 3            */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_ICACHELRU_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_ICACHELRU_OFFSET      0x184        /**< \brief (PICOP_ICACHELRU offset) Instruction Cache LRU */
#define PICOP_ICACHELRU_RESETVALUE  0x00000000ul /**< \brief (PICOP_ICACHELRU reset_value) Instruction Cache LRU */

#define PICOP_ICACHELRU_LRU0_Pos    0            /**< \brief (PICOP_ICACHELRU) Instruction Cache LRU 0 */
#define PICOP_ICACHELRU_LRU0_Msk    (0x3ul << PICOP_ICACHELRU_LRU0_Pos)
#define PICOP_ICACHELRU_LRU0(value) (PICOP_ICACHELRU_LRU0_Msk & ((value) << PICOP_ICACHELRU_LRU0_Pos))
#define PICOP_ICACHELRU_LRU1_Pos    2            /**< \brief (PICOP_ICACHELRU) Instruction Cache LRU 1 */
#define PICOP_ICACHELRU_LRU1_Msk    (0x3ul << PICOP_ICACHELRU_LRU1_Pos)
#define PICOP_ICACHELRU_LRU1(value) (PICOP_ICACHELRU_LRU1_Msk & ((value) << PICOP_ICACHELRU_LRU1_Pos))
#define PICOP_ICACHELRU_LRU2_Pos    4            /**< \brief (PICOP_ICACHELRU) Instruction Cache LRU 2 */
#define PICOP_ICACHELRU_LRU2_Msk    (0x3ul << PICOP_ICACHELRU_LRU2_Pos)
#define PICOP_ICACHELRU_LRU2(value) (PICOP_ICACHELRU_LRU2_Msk & ((value) << PICOP_ICACHELRU_LRU2_Pos))
#define PICOP_ICACHELRU_LRU3_Pos    6            /**< \brief (PICOP_ICACHELRU) Instruction Cache LRU 3 */
#define PICOP_ICACHELRU_LRU3_Msk    (0x3ul << PICOP_ICACHELRU_LRU3_Pos)
#define PICOP_ICACHELRU_LRU3(value) (PICOP_ICACHELRU_LRU3_Msk & ((value) << PICOP_ICACHELRU_LRU3_Pos))
#define PICOP_ICACHELRU_MASK        0x000000FFul /**< \brief (PICOP_ICACHELRU) MASK Register */

/* -------- PICOP_QOSCTRL : (PICOP Offset: 0x200) (R/W 32) QOS Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t QOS:2;            /*!< bit:  0.. 1  Quality of Service                 */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PICOP_QOSCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PICOP_QOSCTRL_OFFSET        0x200        /**< \brief (PICOP_QOSCTRL offset) QOS Control */

#define PICOP_QOSCTRL_QOS_Pos       0            /**< \brief (PICOP_QOSCTRL) Quality of Service */
#define PICOP_QOSCTRL_QOS_Msk       (0x3ul << PICOP_QOSCTRL_QOS_Pos)
#define PICOP_QOSCTRL_QOS(value)    (PICOP_QOSCTRL_QOS_Msk & ((value) << PICOP_QOSCTRL_QOS_Pos))
#define PICOP_QOSCTRL_MASK          0x00000003ul /**< \brief (PICOP_QOSCTRL) MASK Register */

/** \brief PICOP hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO PICOP_ID_Type             ID[8];       /**< \brief Offset: 0x000 (R/W 32) ID n */
  __IO PICOP_CONFIG_Type         CONFIG;      /**< \brief Offset: 0x020 (R/W 32) Configuration */
  __IO PICOP_CTRL_Type           CTRL;        /**< \brief Offset: 0x024 (R/W 32) Control */
  __IO PICOP_CMD_Type            CMD;         /**< \brief Offset: 0x028 (R/W 32) Command */
  __IO PICOP_PC_Type             PC;          /**< \brief Offset: 0x02C (R/W 32) Program Counter */
  __IO PICOP_HF_Type             HF;          /**< \brief Offset: 0x030 (R/W 32) Host Flags */
  __IO PICOP_HFCTRL_Type         HFCTRL;      /**< \brief Offset: 0x034 (R/W 32) Host Flag Control */
  __IO PICOP_HFSETCLR0_Type      HFSETCLR0;   /**< \brief Offset: 0x038 (R/W 32) Host Flags Set/Clr */
  __IO PICOP_HFSETCLR1_Type      HFSETCLR1;   /**< \brief Offset: 0x03C (R/W 32) Host Flags Set/Clr */
       RoReg8                    Reserved1[0x10];
  __IO PICOP_OCDCONFIG_Type      OCDCONFIG;   /**< \brief Offset: 0x050 (R/W 32) OCD Configuration */
  __IO PICOP_OCDCONTROL_Type     OCDCONTROL;  /**< \brief Offset: 0x054 (R/W 32) OCD Control */
  __IO PICOP_OCDSTATUS_Type      OCDSTATUS;   /**< \brief Offset: 0x058 (R/W 32) OCD Status and Command */
  __IO PICOP_OCDPC_Type          OCDPC;       /**< \brief Offset: 0x05C (R/W 32) ODC Program Counter */
  __IO PICOP_OCDFEAT_Type        OCDFEAT;     /**< \brief Offset: 0x060 (R/W 32) OCD Features */
       RoReg8                    Reserved2[0x4];
  __IO PICOP_OCDCCNT_Type        OCDCCNT;     /**< \brief Offset: 0x068 (R/W 32) OCD Cycle Counter */
       RoReg8                    Reserved3[0x4];
  __IO PICOP_OCDBPGEN_Type       OCDBPGEN[4]; /**< \brief Offset: 0x070 (R/W 32) OCD Breakpoint Generator n */
  __IO PICOP_R3R0_Type           R3R0;        /**< \brief Offset: 0x080 (R/W 32) R3 to 0 */
  __IO PICOP_R7R4_Type           R7R4;        /**< \brief Offset: 0x084 (R/W 32) R7 to 4 */
  __IO PICOP_R11R8_Type          R11R8;       /**< \brief Offset: 0x088 (R/W 32) R11 to 8 */
  __IO PICOP_R15R12_Type         R15R12;      /**< \brief Offset: 0x08C (R/W 32) R15 to 12 */
  __IO PICOP_R19R16_Type         R19R16;      /**< \brief Offset: 0x090 (R/W 32) R19 to 16 */
  __IO PICOP_R23R20_Type         R23R20;      /**< \brief Offset: 0x094 (R/W 32) R23 to 20 */
  __IO PICOP_R27R24_Type         R27R24;      /**< \brief Offset: 0x098 (R/W 32) R27 to 24: XH, XL, R25, R24 */
  __IO PICOP_R31R28_Type         R31R28;      /**< \brief Offset: 0x09C (R/W 32) R31 to 28: ZH, ZL, YH, YL */
  __IO PICOP_S1S0_Type           S1S0;        /**< \brief Offset: 0x0A0 (R/W 32) System Regs 1 to 0: SR */
  __IO PICOP_S3S2_Type           S3S2;        /**< \brief Offset: 0x0A4 (R/W 32) System Regs 3 to 2: CTRL */
  __IO PICOP_S5S4_Type           S5S4;        /**< \brief Offset: 0x0A8 (R/W 32) System Regs 5 to 4: SREG, CCR */
       RoReg8                    Reserved4[0x8];
  __IO PICOP_S11S10_Type         S11S10;      /**< \brief Offset: 0x0B4 (R/W 32) System Regs 11 to 10: Immediate */
  __IO PICOP_LINK_Type           LINK;        /**< \brief Offset: 0x0B8 (R/W 32) Link */
  __IO PICOP_SP_Type             SP;          /**< \brief Offset: 0x0BC (R/W 32) Stack Pointer */
       RoReg8                    Reserved5[0x40];
  __IO PICOP_MMUFLASH_Type       MMUFLASH;    /**< \brief Offset: 0x100 (R/W 32) MMU mapping for flash */
       RoReg8                    Reserved6[0x14];
  __IO PICOP_MMU0_Type           MMU0;        /**< \brief Offset: 0x118 (R/W 32) MMU mapping user 0 */
  __IO PICOP_MMU1_Type           MMU1;        /**< \brief Offset: 0x11C (R/W 32) MMU mapping user 1 */
  __IO PICOP_MMUCTRL_Type        MMUCTRL;     /**< \brief Offset: 0x120 (R/W 32) MMU Control */
       RoReg8                    Reserved7[0x5C];
  __IO PICOP_ICACHE_Type         ICACHE;      /**< \brief Offset: 0x180 (R/W 32) Instruction Cache Control */
  __IO PICOP_ICACHELRU_Type      ICACHELRU;   /**< \brief Offset: 0x184 (R/W 32) Instruction Cache LRU */
       RoReg8                    Reserved8[0x78];
  __IO PICOP_QOSCTRL_Type        QOSCTRL;     /**< \brief Offset: 0x200 (R/W 32) QOS Control */
} Picop;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/*@}*/

#endif /* _SAME54_PICOP_COMPONENT_ */
