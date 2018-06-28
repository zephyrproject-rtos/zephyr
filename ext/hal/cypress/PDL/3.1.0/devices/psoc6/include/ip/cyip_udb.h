/***************************************************************************//**
* \file cyip_udb.h
*
* \brief
* UDB IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_UDB_H_
#define _CYIP_UDB_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     UDB
*******************************************************************************/

#define UDB_WRKONE_SECTION_SIZE                 0x00000800UL
#define UDB_WRKMULT_SECTION_SIZE                0x00001000UL
#define UDB_UDBPAIR_UDBSNG_SECTION_SIZE         0x00000080UL
#define UDB_UDBPAIR_ROUTE_SECTION_SIZE          0x00000100UL
#define UDB_UDBPAIR_SECTION_SIZE                0x00000200UL
#define UDB_DSI_SECTION_SIZE                    0x00000080UL
#define UDB_PA_SECTION_SIZE                     0x00000040UL
#define UDB_BCTL_SECTION_SIZE                   0x00000080UL
#define UDB_UDBIF_SECTION_SIZE                  0x00000020UL
#define UDB_SECTION_SIZE                        0x00010000UL

/**
  * \brief UDB Working Registers (2 registers from one UDB at a time) (UDB_WRKONE)
  */
typedef struct {
  __IOM uint32_t A[64];                         /*!< 0x00000000 Accumulator Registers {A1,A0} */
  __IOM uint32_t D[64];                         /*!< 0x00000100 Data Registers {D1,D0} */
  __IOM uint32_t F[64];                         /*!< 0x00000200 FIFOs {F1,F0} */
  __IOM uint32_t CTL_ST[64];                    /*!< 0x00000300 Status and Control Registers {CTL,ST} */
  __IOM uint32_t ACTL_MSK[64];                  /*!< 0x00000400 Mask and Auxiliary Control Registers {ACTL,MSK} */
   __IM uint32_t MC[64];                        /*!< 0x00000500 PLD Macrocell Read Registers {00,MC} */
   __IM uint32_t RESERVED[128];
} UDB_WRKONE_V1_Type;                           /*!< Size = 2048 (0x800) */

/**
  * \brief UDB Working Registers (1 register from multiple UDBs at a time) (UDB_WRKMULT)
  */
typedef struct {
  __IOM uint32_t A0[64];                        /*!< 0x00000000 Accumulator 0 */
  __IOM uint32_t A1[64];                        /*!< 0x00000100 Accumulator 1 */
  __IOM uint32_t D0[64];                        /*!< 0x00000200 Data 0 */
  __IOM uint32_t D1[64];                        /*!< 0x00000300 Data 1 */
  __IOM uint32_t F0[64];                        /*!< 0x00000400 FIFO 0 */
  __IOM uint32_t F1[64];                        /*!< 0x00000500 FIFO 1 */
   __IM uint32_t ST[64];                        /*!< 0x00000600 Status Register */
  __IOM uint32_t CTL[64];                       /*!< 0x00000700 Control Register */
  __IOM uint32_t MSK[64];                       /*!< 0x00000800 Interrupt Mask */
  __IOM uint32_t ACTL[64];                      /*!< 0x00000900 Auxiliary Control */
   __IM uint32_t MC[64];                        /*!< 0x00000A00 PLD Macrocell reading */
   __IM uint32_t RESERVED[320];
} UDB_WRKMULT_V1_Type;                          /*!< Size = 4096 (0x1000) */

/**
  * \brief Single UDB Configuration (UDB_UDBPAIR_UDBSNG)
  */
typedef struct {
  __IOM uint32_t PLD_IT[12];                    /*!< 0x00000000 PLD Input Terms */
  __IOM uint32_t PLD_ORT0;                      /*!< 0x00000030 PLD OR Terms */
  __IOM uint32_t PLD_ORT1;                      /*!< 0x00000034 PLD OR Terms */
  __IOM uint32_t PLD_CFG0;                      /*!< 0x00000038 PLD configuration for Carry Enable, Constant, and XOR feedback */
  __IOM uint32_t PLD_CFG1;                      /*!< 0x0000003C PLD configuration for Set / Reset selection, and Bypass control */
  __IOM uint32_t DPATH_CFG0;                    /*!< 0x00000040 Datapath input selections (RAD0, RAD1, RAD2, F0_LD, F1_LD,
                                                                D0_LD, D1_LD) */
  __IOM uint32_t DPATH_CFG1;                    /*!< 0x00000044 Datapath input and output selections (SCI_MUX, SI_MUX, OUT0
                                                                thru OUT5) */
  __IOM uint32_t DPATH_CFG2;                    /*!< 0x00000048 Datapath output synchronization, ALU mask, compare 0 and 1
                                                                masks */
  __IOM uint32_t DPATH_CFG3;                    /*!< 0x0000004C Datapath mask enables, shift in, carry in, compare, chaining,
                                                                MSB configs; FIFO, shift and parallel input control */
  __IOM uint32_t DPATH_CFG4;                    /*!< 0x00000050 Datapath FIFO and register access configuration control */
  __IOM uint32_t SC_CFG0;                       /*!< 0x00000054 SC Mode 0 and 1 control registers; status register input mode;
                                                                general SC configuration */
  __IOM uint32_t SC_CFG1;                       /*!< 0x00000058 SC counter control */
  __IOM uint32_t RC_CFG0;                       /*!< 0x0000005C PLD0, PLD1, Datatpath, and SC clock and reset control */
  __IOM uint32_t RC_CFG1;                       /*!< 0x00000060 PLD0, PLD1, Datatpath, and SC clock selection, general reset
                                                                control */
  __IOM uint32_t DPATH_OPC[4];                  /*!< 0x00000064 Datapath opcode configuration */
   __IM uint32_t RESERVED[3];
} UDB_UDBPAIR_UDBSNG_V1_Type;                   /*!< Size = 128 (0x80) */

/**
  * \brief Routing Configuration for one UDB Pair (UDB_UDBPAIR_ROUTE)
  */
typedef struct {
  __IOM uint32_t TOP_V_BOT;                     /*!< 0x00000000 Top Vertical Input (TVI) vs Bottom Vertical Input (BVI) muxing */
  __IOM uint32_t LVO1_V_2;                      /*!< 0x00000004 Left Vertical Ouput (LVO) 1 vs 2 muxing for certain horizontals */
  __IOM uint32_t RVO1_V_2;                      /*!< 0x00000008 Right Vertical Ouput (RVO) 1 vs 2 muxing for certain
                                                                horizontals */
  __IOM uint32_t TUI_CFG0;                      /*!< 0x0000000C Top UDB Input (TUI) selection */
  __IOM uint32_t TUI_CFG1;                      /*!< 0x00000010 Top UDB Input (TUI) selection */
  __IOM uint32_t TUI_CFG2;                      /*!< 0x00000014 Top UDB Input (TUI) selection */
  __IOM uint32_t TUI_CFG3;                      /*!< 0x00000018 Top UDB Input (TUI) selection */
  __IOM uint32_t TUI_CFG4;                      /*!< 0x0000001C Top UDB Input (TUI) selection */
  __IOM uint32_t TUI_CFG5;                      /*!< 0x00000020 Top UDB Input (TUI) selection */
  __IOM uint32_t BUI_CFG0;                      /*!< 0x00000024 Bottom UDB Input (BUI) selection */
  __IOM uint32_t BUI_CFG1;                      /*!< 0x00000028 Bottom UDB Input (BUI) selection */
  __IOM uint32_t BUI_CFG2;                      /*!< 0x0000002C Bottom UDB Input (BUI) selection */
  __IOM uint32_t BUI_CFG3;                      /*!< 0x00000030 Bottom UDB Input (BUI) selection */
  __IOM uint32_t BUI_CFG4;                      /*!< 0x00000034 Bottom UDB Input (BUI) selection */
  __IOM uint32_t BUI_CFG5;                      /*!< 0x00000038 Bottom UDB Input (BUI) selection */
  __IOM uint32_t RVO_CFG0;                      /*!< 0x0000003C Right Vertical Ouput (RVO) selection */
  __IOM uint32_t RVO_CFG1;                      /*!< 0x00000040 Right Vertical Ouput (RVO) selection */
  __IOM uint32_t RVO_CFG2;                      /*!< 0x00000044 Right Vertical Ouput (RVO) selection */
  __IOM uint32_t RVO_CFG3;                      /*!< 0x00000048 Right Vertical Ouput (RVO) selection */
  __IOM uint32_t LVO_CFG0;                      /*!< 0x0000004C Left Vertical Ouput (LVO) selection */
  __IOM uint32_t LVO_CFG1;                      /*!< 0x00000050 Left Vertical Ouput (LVO) selection */
  __IOM uint32_t RHO_CFG0;                      /*!< 0x00000054 Right Horizontal Out (RHO) selection */
  __IOM uint32_t RHO_CFG1;                      /*!< 0x00000058 Right Horizontal Out (RHO) selection */
  __IOM uint32_t RHO_CFG2;                      /*!< 0x0000005C Right Horizontal Out (RHO) selection */
  __IOM uint32_t LHO_CFG0;                      /*!< 0x00000060 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG1;                      /*!< 0x00000064 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG2;                      /*!< 0x00000068 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG3;                      /*!< 0x0000006C Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG4;                      /*!< 0x00000070 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG5;                      /*!< 0x00000074 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG6;                      /*!< 0x00000078 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG7;                      /*!< 0x0000007C Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG8;                      /*!< 0x00000080 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG9;                      /*!< 0x00000084 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG10;                     /*!< 0x00000088 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG11;                     /*!< 0x0000008C Left Horizontal Out (LHO) selection */
   __IM uint32_t RESERVED[28];
} UDB_UDBPAIR_ROUTE_V1_Type;                    /*!< Size = 256 (0x100) */

/**
  * \brief UDB Pair Configuration (up to 32 Pairs) (UDB_UDBPAIR)
  */
typedef struct {
        UDB_UDBPAIR_UDBSNG_V1_Type UDBSNG[2];   /*!< 0x00000000 Single UDB Configuration */
        UDB_UDBPAIR_ROUTE_V1_Type ROUTE;        /*!< 0x00000100 Routing Configuration for one UDB Pair */
} UDB_UDBPAIR_V1_Type;                          /*!< Size = 512 (0x200) */

/**
  * \brief DSI Configuration (up to 32 DSI) (UDB_DSI)
  */
typedef struct {
  __IOM uint32_t LVO1_V_2;                      /*!< 0x00000000 Left Vertical Ouput (LVO) 1 vs 2 muxing for certain horizontals */
  __IOM uint32_t RVO1_V_2;                      /*!< 0x00000004 Right Vertical Ouput (RVO) 1 vs 2 muxing for certain
                                                                horizontals */
  __IOM uint32_t DOP_CFG0;                      /*!< 0x00000008 DSI Out Pair (DOP) selection */
  __IOM uint32_t DOP_CFG1;                      /*!< 0x0000000C DSI Out Pair (DOP) selection */
  __IOM uint32_t DOP_CFG2;                      /*!< 0x00000010 DSI Out Pair (DOP) selection */
  __IOM uint32_t DOP_CFG3;                      /*!< 0x00000014 DSI Out Pair (DOP) selection */
  __IOM uint32_t DOT_CFG0;                      /*!< 0x00000018 DSI Out Triplet (DOT) selection */
  __IOM uint32_t DOT_CFG1;                      /*!< 0x0000001C DSI Out Triplet (DOT) selection */
  __IOM uint32_t DOT_CFG2;                      /*!< 0x00000020 DSI Out Triplet (DOT) selection */
  __IOM uint32_t DOT_CFG3;                      /*!< 0x00000024 DSI Out Triplet (DOT) selection */
  __IOM uint32_t RVO_CFG0;                      /*!< 0x00000028 Right Vertical Ouput (RVO) selection */
  __IOM uint32_t RVO_CFG1;                      /*!< 0x0000002C Right Vertical Ouput (RVO) selection */
  __IOM uint32_t RVO_CFG2;                      /*!< 0x00000030 Right Vertical Ouput (RVO) selection */
  __IOM uint32_t RVO_CFG3;                      /*!< 0x00000034 Right Vertical Ouput (RVO) selection */
  __IOM uint32_t LVO_CFG0;                      /*!< 0x00000038 Left Vertical Ouput (LVO) selection */
  __IOM uint32_t LVO_CFG1;                      /*!< 0x0000003C Left Vertical Ouput (LVO) selection */
  __IOM uint32_t RHO_CFG0;                      /*!< 0x00000040 Right Horizontal Out (RHO) selection */
  __IOM uint32_t RHO_CFG1;                      /*!< 0x00000044 Right Horizontal Out (RHO) selection */
  __IOM uint32_t RHO_CFG2;                      /*!< 0x00000048 Right Horizontal Out (RHO) selection */
  __IOM uint32_t LHO_CFG0;                      /*!< 0x0000004C Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG1;                      /*!< 0x00000050 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG2;                      /*!< 0x00000054 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG3;                      /*!< 0x00000058 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG4;                      /*!< 0x0000005C Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG5;                      /*!< 0x00000060 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG6;                      /*!< 0x00000064 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG7;                      /*!< 0x00000068 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG8;                      /*!< 0x0000006C Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG9;                      /*!< 0x00000070 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG10;                     /*!< 0x00000074 Left Horizontal Out (LHO) selection */
  __IOM uint32_t LHO_CFG11;                     /*!< 0x00000078 Left Horizontal Out (LHO) selection */
   __IM uint32_t RESERVED;
} UDB_DSI_V1_Type;                              /*!< Size = 128 (0x80) */

/**
  * \brief Port Adapter Configuration (up to 32 PA) (UDB_PA)
  */
typedef struct {
  __IOM uint32_t CFG0;                          /*!< 0x00000000 PA Data In Clock Control Register */
  __IOM uint32_t CFG1;                          /*!< 0x00000004 PA Data Out Clock Control Register */
  __IOM uint32_t CFG2;                          /*!< 0x00000008 PA Clock Select Register */
  __IOM uint32_t CFG3;                          /*!< 0x0000000C PA Reset Select Register */
  __IOM uint32_t CFG4;                          /*!< 0x00000010 PA Reset Enable Register */
  __IOM uint32_t CFG5;                          /*!< 0x00000014 PA Reset Pin Select Register */
  __IOM uint32_t CFG6;                          /*!< 0x00000018 PA Input Data Sync Control Register - Low */
  __IOM uint32_t CFG7;                          /*!< 0x0000001C PA Input Data Sync Control Register - High */
  __IOM uint32_t CFG8;                          /*!< 0x00000020 PA Output Data Sync Control Register - Low */
  __IOM uint32_t CFG9;                          /*!< 0x00000024 PA Output Data Sync Control Register - High */
  __IOM uint32_t CFG10;                         /*!< 0x00000028 PA Output Data Select Register - Low */
  __IOM uint32_t CFG11;                         /*!< 0x0000002C PA Output Data Select Register - High */
  __IOM uint32_t CFG12;                         /*!< 0x00000030 PA OE Select Register - Low */
  __IOM uint32_t CFG13;                         /*!< 0x00000034 PA OE Select Register - High */
  __IOM uint32_t CFG14;                         /*!< 0x00000038 PA OE Sync Register */
   __IM uint32_t RESERVED;
} UDB_PA_V1_Type;                               /*!< Size = 64 (0x40) */

/**
  * \brief UDB Array Bank Control (UDB_BCTL)
  */
typedef struct {
  __IOM uint32_t MDCLK_EN;                      /*!< 0x00000000 Master Digital Clock Enable Register */
  __IOM uint32_t MBCLK_EN;                      /*!< 0x00000004 Master clk_peri_app Enable Register */
  __IOM uint32_t BOTSEL_L;                      /*!< 0x00000008 Lower Nibble Bottom Digital Clock Select Register */
  __IOM uint32_t BOTSEL_U;                      /*!< 0x0000000C Upper Nibble Bottom Digital Clock Select Register */
  __IOM uint32_t QCLK_EN[16];                   /*!< 0x00000010 Quadrant Digital Clock Enable Registers */
   __IM uint32_t RESERVED[12];
} UDB_BCTL_V1_Type;                             /*!< Size = 128 (0x80) */

/**
  * \brief UDB Subsystem Interface Configuration (UDB_UDBIF)
  */
typedef struct {
  __IOM uint32_t BANK_CTL;                      /*!< 0x00000000 Bank Control */
  __IOM uint32_t INT_CLK_CTL;                   /*!< 0x00000004 Interrupt Clock Control */
  __IOM uint32_t INT_CFG;                       /*!< 0x00000008 Interrupt Configuration */
  __IOM uint32_t TR_CLK_CTL;                    /*!< 0x0000000C Trigger Clock Control */
  __IOM uint32_t TR_CFG;                        /*!< 0x00000010 Trigger Configuration */
  __IOM uint32_t PRIVATE;                       /*!< 0x00000014 Internal use only */
   __IM uint32_t RESERVED[2];
} UDB_UDBIF_V1_Type;                            /*!< Size = 32 (0x20) */

/**
  * \brief Programmable Digital Subsystem (UDB)
  */
typedef struct {
        UDB_WRKONE_V1_Type WRKONE;              /*!< 0x00000000 UDB Working Registers (2 registers from one UDB at a time) */
   __IM uint32_t RESERVED[512];
        UDB_WRKMULT_V1_Type WRKMULT;            /*!< 0x00001000 UDB Working Registers (1 register from multiple UDBs at a time) */
        UDB_UDBPAIR_V1_Type UDBPAIR[32];        /*!< 0x00002000 UDB Pair Configuration (up to 32 Pairs) */
        UDB_DSI_V1_Type DSI[32];                /*!< 0x00006000 DSI Configuration (up to 32 DSI) */
        UDB_PA_V1_Type PA[32];                  /*!< 0x00007000 Port Adapter Configuration (up to 32 PA) */
        UDB_BCTL_V1_Type BCTL;                  /*!< 0x00007800 UDB Array Bank Control */
   __IM uint32_t RESERVED1[32];
        UDB_UDBIF_V1_Type UDBIF;                /*!< 0x00007900 UDB Subsystem Interface Configuration */
} UDB_V1_Type;                                  /*!< Size = 31008 (0x7920) */


/* UDB_WRKONE.A */
#define UDB_WRKONE_A_A0_Pos                     0UL
#define UDB_WRKONE_A_A0_Msk                     0xFFUL
#define UDB_WRKONE_A_A1_Pos                     8UL
#define UDB_WRKONE_A_A1_Msk                     0xFF00UL
/* UDB_WRKONE.D */
#define UDB_WRKONE_D_D0_Pos                     0UL
#define UDB_WRKONE_D_D0_Msk                     0xFFUL
#define UDB_WRKONE_D_D1_Pos                     8UL
#define UDB_WRKONE_D_D1_Msk                     0xFF00UL
/* UDB_WRKONE.F */
#define UDB_WRKONE_F_F0_Pos                     0UL
#define UDB_WRKONE_F_F0_Msk                     0xFFUL
#define UDB_WRKONE_F_F1_Pos                     8UL
#define UDB_WRKONE_F_F1_Msk                     0xFF00UL
/* UDB_WRKONE.CTL_ST */
#define UDB_WRKONE_CTL_ST_ST_Pos                0UL
#define UDB_WRKONE_CTL_ST_ST_Msk                0xFFUL
#define UDB_WRKONE_CTL_ST_CTL_Pos               8UL
#define UDB_WRKONE_CTL_ST_CTL_Msk               0xFF00UL
/* UDB_WRKONE.ACTL_MSK */
#define UDB_WRKONE_ACTL_MSK_MSK_Pos             0UL
#define UDB_WRKONE_ACTL_MSK_MSK_Msk             0x7FUL
#define UDB_WRKONE_ACTL_MSK_FIFO0_CLR_Pos       8UL
#define UDB_WRKONE_ACTL_MSK_FIFO0_CLR_Msk       0x100UL
#define UDB_WRKONE_ACTL_MSK_FIFO1_CLR_Pos       9UL
#define UDB_WRKONE_ACTL_MSK_FIFO1_CLR_Msk       0x200UL
#define UDB_WRKONE_ACTL_MSK_FIFO0_LVL_Pos       10UL
#define UDB_WRKONE_ACTL_MSK_FIFO0_LVL_Msk       0x400UL
#define UDB_WRKONE_ACTL_MSK_FIFO1_LVL_Pos       11UL
#define UDB_WRKONE_ACTL_MSK_FIFO1_LVL_Msk       0x800UL
#define UDB_WRKONE_ACTL_MSK_INT_EN_Pos          12UL
#define UDB_WRKONE_ACTL_MSK_INT_EN_Msk          0x1000UL
#define UDB_WRKONE_ACTL_MSK_CNT_START_Pos       13UL
#define UDB_WRKONE_ACTL_MSK_CNT_START_Msk       0x2000UL
/* UDB_WRKONE.MC */
#define UDB_WRKONE_MC_PLD0_MC_Pos               0UL
#define UDB_WRKONE_MC_PLD0_MC_Msk               0xFUL
#define UDB_WRKONE_MC_PLD1_MC_Pos               4UL
#define UDB_WRKONE_MC_PLD1_MC_Msk               0xF0UL


/* UDB_WRKMULT.A0 */
#define UDB_WRKMULT_A0_A0_0_Pos                 0UL
#define UDB_WRKMULT_A0_A0_0_Msk                 0xFFUL
#define UDB_WRKMULT_A0_A0_1_Pos                 8UL
#define UDB_WRKMULT_A0_A0_1_Msk                 0xFF00UL
#define UDB_WRKMULT_A0_A0_2_Pos                 16UL
#define UDB_WRKMULT_A0_A0_2_Msk                 0xFF0000UL
#define UDB_WRKMULT_A0_A0_3_Pos                 24UL
#define UDB_WRKMULT_A0_A0_3_Msk                 0xFF000000UL
/* UDB_WRKMULT.A1 */
#define UDB_WRKMULT_A1_A1_0_Pos                 0UL
#define UDB_WRKMULT_A1_A1_0_Msk                 0xFFUL
#define UDB_WRKMULT_A1_A1_1_Pos                 8UL
#define UDB_WRKMULT_A1_A1_1_Msk                 0xFF00UL
#define UDB_WRKMULT_A1_A1_2_Pos                 16UL
#define UDB_WRKMULT_A1_A1_2_Msk                 0xFF0000UL
#define UDB_WRKMULT_A1_A1_3_Pos                 24UL
#define UDB_WRKMULT_A1_A1_3_Msk                 0xFF000000UL
/* UDB_WRKMULT.D0 */
#define UDB_WRKMULT_D0_D0_0_Pos                 0UL
#define UDB_WRKMULT_D0_D0_0_Msk                 0xFFUL
#define UDB_WRKMULT_D0_D0_1_Pos                 8UL
#define UDB_WRKMULT_D0_D0_1_Msk                 0xFF00UL
#define UDB_WRKMULT_D0_D0_2_Pos                 16UL
#define UDB_WRKMULT_D0_D0_2_Msk                 0xFF0000UL
#define UDB_WRKMULT_D0_D0_3_Pos                 24UL
#define UDB_WRKMULT_D0_D0_3_Msk                 0xFF000000UL
/* UDB_WRKMULT.D1 */
#define UDB_WRKMULT_D1_D1_0_Pos                 0UL
#define UDB_WRKMULT_D1_D1_0_Msk                 0xFFUL
#define UDB_WRKMULT_D1_D1_1_Pos                 8UL
#define UDB_WRKMULT_D1_D1_1_Msk                 0xFF00UL
#define UDB_WRKMULT_D1_D1_2_Pos                 16UL
#define UDB_WRKMULT_D1_D1_2_Msk                 0xFF0000UL
#define UDB_WRKMULT_D1_D1_3_Pos                 24UL
#define UDB_WRKMULT_D1_D1_3_Msk                 0xFF000000UL
/* UDB_WRKMULT.F0 */
#define UDB_WRKMULT_F0_F0_0_Pos                 0UL
#define UDB_WRKMULT_F0_F0_0_Msk                 0xFFUL
#define UDB_WRKMULT_F0_F0_1_Pos                 8UL
#define UDB_WRKMULT_F0_F0_1_Msk                 0xFF00UL
#define UDB_WRKMULT_F0_F0_2_Pos                 16UL
#define UDB_WRKMULT_F0_F0_2_Msk                 0xFF0000UL
#define UDB_WRKMULT_F0_F0_3_Pos                 24UL
#define UDB_WRKMULT_F0_F0_3_Msk                 0xFF000000UL
/* UDB_WRKMULT.F1 */
#define UDB_WRKMULT_F1_F1_0_Pos                 0UL
#define UDB_WRKMULT_F1_F1_0_Msk                 0xFFUL
#define UDB_WRKMULT_F1_F1_1_Pos                 8UL
#define UDB_WRKMULT_F1_F1_1_Msk                 0xFF00UL
#define UDB_WRKMULT_F1_F1_2_Pos                 16UL
#define UDB_WRKMULT_F1_F1_2_Msk                 0xFF0000UL
#define UDB_WRKMULT_F1_F1_3_Pos                 24UL
#define UDB_WRKMULT_F1_F1_3_Msk                 0xFF000000UL
/* UDB_WRKMULT.ST */
#define UDB_WRKMULT_ST_ST_0_Pos                 0UL
#define UDB_WRKMULT_ST_ST_0_Msk                 0xFFUL
#define UDB_WRKMULT_ST_ST_1_Pos                 8UL
#define UDB_WRKMULT_ST_ST_1_Msk                 0xFF00UL
#define UDB_WRKMULT_ST_ST_2_Pos                 16UL
#define UDB_WRKMULT_ST_ST_2_Msk                 0xFF0000UL
#define UDB_WRKMULT_ST_ST_3_Pos                 24UL
#define UDB_WRKMULT_ST_ST_3_Msk                 0xFF000000UL
/* UDB_WRKMULT.CTL */
#define UDB_WRKMULT_CTL_CTL_0_Pos               0UL
#define UDB_WRKMULT_CTL_CTL_0_Msk               0xFFUL
#define UDB_WRKMULT_CTL_CTL_1_Pos               8UL
#define UDB_WRKMULT_CTL_CTL_1_Msk               0xFF00UL
#define UDB_WRKMULT_CTL_CTL_2_Pos               16UL
#define UDB_WRKMULT_CTL_CTL_2_Msk               0xFF0000UL
#define UDB_WRKMULT_CTL_CTL_3_Pos               24UL
#define UDB_WRKMULT_CTL_CTL_3_Msk               0xFF000000UL
/* UDB_WRKMULT.MSK */
#define UDB_WRKMULT_MSK_MSK_0_Pos               0UL
#define UDB_WRKMULT_MSK_MSK_0_Msk               0x7FUL
#define UDB_WRKMULT_MSK_MSK_1_Pos               8UL
#define UDB_WRKMULT_MSK_MSK_1_Msk               0x7F00UL
#define UDB_WRKMULT_MSK_MSK_2_Pos               16UL
#define UDB_WRKMULT_MSK_MSK_2_Msk               0x7F0000UL
#define UDB_WRKMULT_MSK_MSK_3_Pos               24UL
#define UDB_WRKMULT_MSK_MSK_3_Msk               0x7F000000UL
/* UDB_WRKMULT.ACTL */
#define UDB_WRKMULT_ACTL_FIFO0_CLR_0_Pos        0UL
#define UDB_WRKMULT_ACTL_FIFO0_CLR_0_Msk        0x1UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_0_Pos        1UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_0_Msk        0x2UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_0_Pos        2UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_0_Msk        0x4UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_0_Pos        3UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_0_Msk        0x8UL
#define UDB_WRKMULT_ACTL_INT_EN_0_Pos           4UL
#define UDB_WRKMULT_ACTL_INT_EN_0_Msk           0x10UL
#define UDB_WRKMULT_ACTL_CNT_START_0_Pos        5UL
#define UDB_WRKMULT_ACTL_CNT_START_0_Msk        0x20UL
#define UDB_WRKMULT_ACTL_FIFO0_CLR_1_Pos        8UL
#define UDB_WRKMULT_ACTL_FIFO0_CLR_1_Msk        0x100UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_1_Pos        9UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_1_Msk        0x200UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_1_Pos        10UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_1_Msk        0x400UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_1_Pos        11UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_1_Msk        0x800UL
#define UDB_WRKMULT_ACTL_INT_EN_1_Pos           12UL
#define UDB_WRKMULT_ACTL_INT_EN_1_Msk           0x1000UL
#define UDB_WRKMULT_ACTL_CNT_START_1_Pos        13UL
#define UDB_WRKMULT_ACTL_CNT_START_1_Msk        0x2000UL
#define UDB_WRKMULT_ACTL_FIFO0_CLR_2_Pos        16UL
#define UDB_WRKMULT_ACTL_FIFO0_CLR_2_Msk        0x10000UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_2_Pos        17UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_2_Msk        0x20000UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_2_Pos        18UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_2_Msk        0x40000UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_2_Pos        19UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_2_Msk        0x80000UL
#define UDB_WRKMULT_ACTL_INT_EN_2_Pos           20UL
#define UDB_WRKMULT_ACTL_INT_EN_2_Msk           0x100000UL
#define UDB_WRKMULT_ACTL_CNT_START_2_Pos        21UL
#define UDB_WRKMULT_ACTL_CNT_START_2_Msk        0x200000UL
#define UDB_WRKMULT_ACTL_FIFO0_CLR_3_Pos        24UL
#define UDB_WRKMULT_ACTL_FIFO0_CLR_3_Msk        0x1000000UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_3_Pos        25UL
#define UDB_WRKMULT_ACTL_FIFO1_CLR_3_Msk        0x2000000UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_3_Pos        26UL
#define UDB_WRKMULT_ACTL_FIFO0_LVL_3_Msk        0x4000000UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_3_Pos        27UL
#define UDB_WRKMULT_ACTL_FIFO1_LVL_3_Msk        0x8000000UL
#define UDB_WRKMULT_ACTL_INT_EN_3_Pos           28UL
#define UDB_WRKMULT_ACTL_INT_EN_3_Msk           0x10000000UL
#define UDB_WRKMULT_ACTL_CNT_START_3_Pos        29UL
#define UDB_WRKMULT_ACTL_CNT_START_3_Msk        0x20000000UL
/* UDB_WRKMULT.MC */
#define UDB_WRKMULT_MC_PLD0_MC_0_Pos            0UL
#define UDB_WRKMULT_MC_PLD0_MC_0_Msk            0xFUL
#define UDB_WRKMULT_MC_PLD1_MC_0_Pos            4UL
#define UDB_WRKMULT_MC_PLD1_MC_0_Msk            0xF0UL
#define UDB_WRKMULT_MC_PLD0_MC_1_Pos            8UL
#define UDB_WRKMULT_MC_PLD0_MC_1_Msk            0xF00UL
#define UDB_WRKMULT_MC_PLD1_MC_1_Pos            12UL
#define UDB_WRKMULT_MC_PLD1_MC_1_Msk            0xF000UL
#define UDB_WRKMULT_MC_PLD0_MC_2_Pos            16UL
#define UDB_WRKMULT_MC_PLD0_MC_2_Msk            0xF0000UL
#define UDB_WRKMULT_MC_PLD1_MC_2_Pos            20UL
#define UDB_WRKMULT_MC_PLD1_MC_2_Msk            0xF00000UL
#define UDB_WRKMULT_MC_PLD0_MC_3_Pos            24UL
#define UDB_WRKMULT_MC_PLD0_MC_3_Msk            0xF000000UL
#define UDB_WRKMULT_MC_PLD1_MC_3_Pos            28UL
#define UDB_WRKMULT_MC_PLD1_MC_3_Msk            0xF0000000UL


/* UDB_UDBPAIR_UDBSNG.PLD_IT */
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT0_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT0_Msk 0x1UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT1_Pos 1UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT1_Msk 0x2UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT2_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT2_Msk 0x4UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT3_Pos 3UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT3_Msk 0x8UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT4_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT4_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT5_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT5_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT6_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT6_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT7_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_C_FOR_PT7_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT0_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT0_Msk 0x100UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT1_Pos 9UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT1_Msk 0x200UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT2_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT2_Msk 0x400UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT3_Pos 11UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT3_Msk 0x800UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT4_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT4_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT5_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT5_Msk 0x2000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT6_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT6_Msk 0x4000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT7_Pos 15UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_C_FOR_PT7_Msk 0x8000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT0_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT0_Msk 0x10000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT1_Pos 17UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT1_Msk 0x20000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT2_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT2_Msk 0x40000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT3_Pos 19UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT3_Msk 0x80000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT4_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT4_Msk 0x100000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT5_Pos 21UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT5_Msk 0x200000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT6_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT6_Msk 0x400000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT7_Pos 23UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD0_INX_T_FOR_PT7_Msk 0x800000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT0_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT0_Msk 0x1000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT1_Pos 25UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT1_Msk 0x2000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT2_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT2_Msk 0x4000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT3_Pos 27UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT3_Msk 0x8000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT4_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT4_Msk 0x10000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT5_Pos 29UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT5_Msk 0x20000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT6_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT6_Msk 0x40000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT7_Pos 31UL
#define UDB_UDBPAIR_UDBSNG_PLD_IT_PLD1_INX_T_FOR_PT7_Msk 0x80000000UL
/* UDB_UDBPAIR_UDBSNG.PLD_ORT0 */
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT0_T_FOR_OUT0_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT0_T_FOR_OUT0_Msk 0x1UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT1_T_FOR_OUT0_Pos 1UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT1_T_FOR_OUT0_Msk 0x2UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT2_T_FOR_OUT0_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT2_T_FOR_OUT0_Msk 0x4UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT3_T_FOR_OUT0_Pos 3UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT3_T_FOR_OUT0_Msk 0x8UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT4_T_FOR_OUT0_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT4_T_FOR_OUT0_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT5_T_FOR_OUT0_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT5_T_FOR_OUT0_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT6_T_FOR_OUT0_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT6_T_FOR_OUT0_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT7_T_FOR_OUT0_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT7_T_FOR_OUT0_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT0_T_FOR_OUT0_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT0_T_FOR_OUT0_Msk 0x100UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT1_T_FOR_OUT0_Pos 9UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT1_T_FOR_OUT0_Msk 0x200UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT2_T_FOR_OUT0_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT2_T_FOR_OUT0_Msk 0x400UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT3_T_FOR_OUT0_Pos 11UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT3_T_FOR_OUT0_Msk 0x800UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT4_T_FOR_OUT0_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT4_T_FOR_OUT0_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT5_T_FOR_OUT0_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT5_T_FOR_OUT0_Msk 0x2000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT6_T_FOR_OUT0_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT6_T_FOR_OUT0_Msk 0x4000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT7_T_FOR_OUT0_Pos 15UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT7_T_FOR_OUT0_Msk 0x8000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT0_T_FOR_OUT1_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT0_T_FOR_OUT1_Msk 0x10000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT1_T_FOR_OUT1_Pos 17UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT1_T_FOR_OUT1_Msk 0x20000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT2_T_FOR_OUT1_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT2_T_FOR_OUT1_Msk 0x40000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT3_T_FOR_OUT1_Pos 19UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT3_T_FOR_OUT1_Msk 0x80000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT4_T_FOR_OUT1_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT4_T_FOR_OUT1_Msk 0x100000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT5_T_FOR_OUT1_Pos 21UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT5_T_FOR_OUT1_Msk 0x200000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT6_T_FOR_OUT1_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT6_T_FOR_OUT1_Msk 0x400000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT7_T_FOR_OUT1_Pos 23UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD0_PT7_T_FOR_OUT1_Msk 0x800000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT0_T_FOR_OUT1_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT0_T_FOR_OUT1_Msk 0x1000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT1_T_FOR_OUT1_Pos 25UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT1_T_FOR_OUT1_Msk 0x2000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT2_T_FOR_OUT1_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT2_T_FOR_OUT1_Msk 0x4000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT3_T_FOR_OUT1_Pos 27UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT3_T_FOR_OUT1_Msk 0x8000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT4_T_FOR_OUT1_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT4_T_FOR_OUT1_Msk 0x10000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT5_T_FOR_OUT1_Pos 29UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT5_T_FOR_OUT1_Msk 0x20000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT6_T_FOR_OUT1_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT6_T_FOR_OUT1_Msk 0x40000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT7_T_FOR_OUT1_Pos 31UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT0_PLD1_PT7_T_FOR_OUT1_Msk 0x80000000UL
/* UDB_UDBPAIR_UDBSNG.PLD_ORT1 */
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT0_T_FOR_OUT2_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT0_T_FOR_OUT2_Msk 0x1UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT1_T_FOR_OUT2_Pos 1UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT1_T_FOR_OUT2_Msk 0x2UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT2_T_FOR_OUT2_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT2_T_FOR_OUT2_Msk 0x4UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT3_T_FOR_OUT2_Pos 3UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT3_T_FOR_OUT2_Msk 0x8UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT4_T_FOR_OUT2_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT4_T_FOR_OUT2_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT5_T_FOR_OUT2_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT5_T_FOR_OUT2_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT6_T_FOR_OUT2_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT6_T_FOR_OUT2_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT7_T_FOR_OUT2_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT7_T_FOR_OUT2_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT0_T_FOR_OUT2_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT0_T_FOR_OUT2_Msk 0x100UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT1_T_FOR_OUT2_Pos 9UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT1_T_FOR_OUT2_Msk 0x200UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT2_T_FOR_OUT2_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT2_T_FOR_OUT2_Msk 0x400UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT3_T_FOR_OUT2_Pos 11UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT3_T_FOR_OUT2_Msk 0x800UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT4_T_FOR_OUT2_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT4_T_FOR_OUT2_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT5_T_FOR_OUT2_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT5_T_FOR_OUT2_Msk 0x2000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT6_T_FOR_OUT2_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT6_T_FOR_OUT2_Msk 0x4000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT7_T_FOR_OUT2_Pos 15UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT7_T_FOR_OUT2_Msk 0x8000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT0_T_FOR_OUT3_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT0_T_FOR_OUT3_Msk 0x10000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT1_T_FOR_OUT3_Pos 17UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT1_T_FOR_OUT3_Msk 0x20000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT2_T_FOR_OUT3_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT2_T_FOR_OUT3_Msk 0x40000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT3_T_FOR_OUT3_Pos 19UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT3_T_FOR_OUT3_Msk 0x80000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT4_T_FOR_OUT3_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT4_T_FOR_OUT3_Msk 0x100000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT5_T_FOR_OUT3_Pos 21UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT5_T_FOR_OUT3_Msk 0x200000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT6_T_FOR_OUT3_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT6_T_FOR_OUT3_Msk 0x400000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT7_T_FOR_OUT3_Pos 23UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD0_PT7_T_FOR_OUT3_Msk 0x800000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT0_T_FOR_OUT3_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT0_T_FOR_OUT3_Msk 0x1000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT1_T_FOR_OUT3_Pos 25UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT1_T_FOR_OUT3_Msk 0x2000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT2_T_FOR_OUT3_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT2_T_FOR_OUT3_Msk 0x4000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT3_T_FOR_OUT3_Pos 27UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT3_T_FOR_OUT3_Msk 0x8000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT4_T_FOR_OUT3_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT4_T_FOR_OUT3_Msk 0x10000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT5_T_FOR_OUT3_Pos 29UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT5_T_FOR_OUT3_Msk 0x20000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT6_T_FOR_OUT3_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT6_T_FOR_OUT3_Msk 0x40000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT7_T_FOR_OUT3_Pos 31UL
#define UDB_UDBPAIR_UDBSNG_PLD_ORT1_PLD1_PT7_T_FOR_OUT3_Msk 0x80000000UL
/* UDB_UDBPAIR_UDBSNG.PLD_CFG0 */
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC0_CEN_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC0_CEN_Msk 0x1UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC0_DFF_C_Pos 1UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC0_DFF_C_Msk 0x2UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC1_CEN_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC1_CEN_Msk 0x4UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC1_DFF_C_Pos 3UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC1_DFF_C_Msk 0x8UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC2_CEN_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC2_CEN_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC2_DFF_C_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC2_DFF_C_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC3_CEN_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC3_CEN_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC3_DFF_C_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC3_DFF_C_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC0_CEN_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC0_CEN_Msk 0x100UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC0_DFF_C_Pos 9UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC0_DFF_C_Msk 0x200UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC1_CEN_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC1_CEN_Msk 0x400UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC1_DFF_C_Pos 11UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC1_DFF_C_Msk 0x800UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC2_CEN_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC2_CEN_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC2_DFF_C_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC2_DFF_C_Msk 0x2000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC3_CEN_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC3_CEN_Msk 0x4000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC3_DFF_C_Pos 15UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC3_DFF_C_Msk 0x8000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC0_XORFB_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC0_XORFB_Msk 0x30000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC1_XORFB_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC1_XORFB_Msk 0xC0000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC2_XORFB_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC2_XORFB_Msk 0x300000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC3_XORFB_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD0_MC3_XORFB_Msk 0xC00000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC0_XORFB_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC0_XORFB_Msk 0x3000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC1_XORFB_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC1_XORFB_Msk 0xC000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC2_XORFB_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC2_XORFB_Msk 0x30000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC3_XORFB_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG0_PLD1_MC3_XORFB_Msk 0xC0000000UL
/* UDB_UDBPAIR_UDBSNG.PLD_CFG1 */
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC0_SET_SEL_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC0_SET_SEL_Msk 0x1UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC0_RESET_SEL_Pos 1UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC0_RESET_SEL_Msk 0x2UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC1_SET_SEL_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC1_SET_SEL_Msk 0x4UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC1_RESET_SEL_Pos 3UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC1_RESET_SEL_Msk 0x8UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC2_SET_SEL_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC2_SET_SEL_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC2_RESET_SEL_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC2_RESET_SEL_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC3_SET_SEL_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC3_SET_SEL_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC3_RESET_SEL_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC3_RESET_SEL_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC0_SET_SEL_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC0_SET_SEL_Msk 0x100UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC0_RESET_SEL_Pos 9UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC0_RESET_SEL_Msk 0x200UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC1_SET_SEL_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC1_SET_SEL_Msk 0x400UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC1_RESET_SEL_Pos 11UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC1_RESET_SEL_Msk 0x800UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC2_SET_SEL_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC2_SET_SEL_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC2_RESET_SEL_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC2_RESET_SEL_Msk 0x2000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC3_SET_SEL_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC3_SET_SEL_Msk 0x4000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC3_RESET_SEL_Pos 15UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC3_RESET_SEL_Msk 0x8000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC0_BYPASS_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC0_BYPASS_Msk 0x10000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC1_BYPASS_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC1_BYPASS_Msk 0x40000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC2_BYPASS_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC2_BYPASS_Msk 0x100000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC3_BYPASS_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD0_MC3_BYPASS_Msk 0x400000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC0_BYPASS_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC0_BYPASS_Msk 0x1000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC1_BYPASS_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC1_BYPASS_Msk 0x4000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC2_BYPASS_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC2_BYPASS_Msk 0x10000000UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC3_BYPASS_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_PLD_CFG1_PLD1_MC3_BYPASS_Msk 0x40000000UL
/* UDB_UDBPAIR_UDBSNG.DPATH_CFG0 */
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_RAD0_Pos  0UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_RAD0_Msk  0x7UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_RAD1_Pos  4UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_RAD1_Msk  0x70UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_RAD2_Pos  8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_RAD2_Msk  0x700UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS0_Pos 11UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS0_Msk 0x800UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS1_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS1_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS2_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS2_Msk 0x2000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS3_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS3_Msk 0x4000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS4_Pos 15UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS4_Msk 0x8000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_F0_LD_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_F0_LD_Msk 0x70000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS5_Pos 19UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_DP_RTE_BYPASS5_Msk 0x80000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_F1_LD_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_F1_LD_Msk 0x700000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_D0_LD_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_D0_LD_Msk 0x7000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_D1_LD_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG0_D1_LD_Msk 0x70000000UL
/* UDB_UDBPAIR_UDBSNG.DPATH_CFG1 */
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_SI_MUX_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_SI_MUX_Msk 0x7UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_CI_MUX_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_CI_MUX_Msk 0x70UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT0_Pos  8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT0_Msk  0xF00UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT1_Pos  12UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT1_Msk  0xF000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT2_Pos  16UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT2_Msk  0xF0000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT3_Pos  20UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT3_Msk  0xF00000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT4_Pos  24UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT4_Msk  0xF000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT5_Pos  28UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG1_OUT5_Msk  0xF0000000UL
/* UDB_UDBPAIR_UDBSNG.DPATH_CFG2 */
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_OUT_SYNC_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_OUT_SYNC_Msk 0x3FUL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_AMASK_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_AMASK_Msk 0xFF00UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_CMASK0_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_CMASK0_Msk 0xFF0000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_CMASK1_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG2_CMASK1_Msk 0xFF000000UL
/* UDB_UDBPAIR_UDBSNG.DPATH_CFG3 */
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_SI_SELA_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_SI_SELA_Msk 0x3UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_SI_SELB_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_SI_SELB_Msk 0xCUL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_DEF_SI_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_DEF_SI_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_AMASK_EN_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_AMASK_EN_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMASK0_EN_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMASK0_EN_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMASK1_EN_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMASK1_EN_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CI_SELA_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CI_SELA_Msk 0x300UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CI_SELB_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CI_SELB_Msk 0xC00UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMP_SELA_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMP_SELA_Msk 0x3000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMP_SELB_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CMP_SELB_Msk 0xC000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN0_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN0_Msk 0x10000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN1_Pos 17UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN1_Msk 0x20000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN_FB_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN_FB_Msk 0x40000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN_CMSB_Pos 19UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_CHAIN_CMSB_Msk 0x80000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_MSB_SEL_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_MSB_SEL_Msk 0x700000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_MSB_EN_Pos 23UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_MSB_EN_Msk 0x800000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_F0_INSEL_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_F0_INSEL_Msk 0x3000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_F1_INSEL_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_F1_INSEL_Msk 0xC000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_MSB_SI_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_MSB_SI_Msk 0x10000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_PI_DYN_Pos 29UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_PI_DYN_Msk 0x20000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_SHIFT_SEL_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_SHIFT_SEL_Msk 0x40000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_PI_SEL_Pos 31UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG3_PI_SEL_Msk 0x80000000UL
/* UDB_UDBPAIR_UDBSNG.DPATH_CFG4 */
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_EXT_CRCPRS_Pos 1UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_EXT_CRCPRS_Msk 0x2UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_ASYNC_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_ASYNC_Msk 0x4UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_EDGE_Pos 3UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_EDGE_Msk 0x8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_CAP_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_CAP_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_FAST_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_FAST_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F0_CK_INV_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F0_CK_INV_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F1_CK_INV_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F1_CK_INV_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F0_DYN_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F0_DYN_Msk 0x100UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F1_DYN_Pos 9UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_F1_DYN_Msk 0x200UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_ADD_SYNC_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_DPATH_CFG4_FIFO_ADD_SYNC_Msk 0x1000UL
/* UDB_UDBPAIR_UDBSNG.SC_CFG0 */
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_CTL_MD0_Pos  0UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_CTL_MD0_Msk  0xFFUL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_CTL_MD1_Pos  8UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_CTL_MD1_Msk  0xFF00UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_STAT_MD_Pos  16UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_STAT_MD_Msk  0xFF0000UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_OUT_CTL_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_OUT_CTL_Msk 0x3000000UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_INT_MD_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_INT_MD_Msk 0x4000000UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_SYNC_MD_Pos 27UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_SYNC_MD_Msk 0x8000000UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_EXT_RES_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG0_SC_EXT_RES_Msk 0x10000000UL
/* UDB_UDBPAIR_UDBSNG.SC_CFG1 */
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_CNT_LD_SEL_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_CNT_LD_SEL_Msk 0x3UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_CNT_EN_SEL_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_CNT_EN_SEL_Msk 0xCUL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_ROUTE_LD_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_ROUTE_LD_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_ROUTE_EN_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_ROUTE_EN_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_ALT_CNT_Pos  6UL
#define UDB_UDBPAIR_UDBSNG_SC_CFG1_ALT_CNT_Msk  0x40UL
/* UDB_UDBPAIR_UDBSNG.RC_CFG0 */
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_EN_SEL_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_EN_SEL_Msk 0x3UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_EN_MODE_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_EN_MODE_Msk 0xCUL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_EN_INV_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_EN_INV_Msk 0x10UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_INV_Pos 5UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_INV_Msk 0x20UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_RES_SEL0_OR_FRES_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_RES_SEL0_OR_FRES_Msk 0x40UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_RES_SEL1_Pos 7UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD0_RC_RES_SEL1_Msk 0x80UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_EN_SEL_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_EN_SEL_Msk 0x300UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_EN_MODE_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_EN_MODE_Msk 0xC00UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_EN_INV_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_EN_INV_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_INV_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_INV_Msk 0x2000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_RES_SEL0_OR_FRES_Pos 14UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_PLD1_RC_RES_SEL0_OR_FRES_Msk 0x4000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_EN_SEL_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_EN_SEL_Msk 0x30000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_EN_MODE_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_EN_MODE_Msk 0xC0000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_EN_INV_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_EN_INV_Msk 0x100000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_INV_Pos 21UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_INV_Msk 0x200000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_RES_SEL0_OR_FRES_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_RES_SEL0_OR_FRES_Msk 0x400000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_RES_SEL1_Pos 23UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_DP_RC_RES_SEL1_Msk 0x800000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_EN_SEL_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_EN_SEL_Msk 0x3000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_EN_MODE_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_EN_MODE_Msk 0xC000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_EN_INV_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_EN_INV_Msk 0x10000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_INV_Pos 29UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_INV_Msk 0x20000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_RES_SEL0_OR_FRES_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_RES_SEL0_OR_FRES_Msk 0x40000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_RES_SEL1_Pos 31UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG0_SC_RC_RES_SEL1_Msk 0x80000000UL
/* UDB_UDBPAIR_UDBSNG.RC_CFG1 */
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_PLD0_CK_SEL_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_PLD0_CK_SEL_Msk 0xFUL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_PLD1_CK_SEL_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_PLD1_CK_SEL_Msk 0xF0UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_DP_CK_SEL_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_DP_CK_SEL_Msk 0xF00UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_SC_CK_SEL_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_SC_CK_SEL_Msk 0xF000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_RES_SEL_Pos  16UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_RES_SEL_Msk  0x30000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_RES_POL_Pos  18UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_RES_POL_Msk  0x40000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EN_RES_CNTCTL_Pos 19UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EN_RES_CNTCTL_Msk 0x80000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_DP_RES_POL_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_DP_RES_POL_Msk 0x400000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_SC_RES_POL_Pos 23UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_SC_RES_POL_Msk 0x800000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_ALT_RES_Pos  24UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_ALT_RES_Msk  0x1000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EXT_SYNC_Pos 25UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EXT_SYNC_Msk 0x2000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EN_RES_STAT_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EN_RES_STAT_Msk 0x4000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EN_RES_DP_Pos 27UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EN_RES_DP_Msk 0x8000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EXT_CK_SEL_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_EXT_CK_SEL_Msk 0x30000000UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_PLD0_RES_POL_Pos 30UL
#define UDB_UDBPAIR_UDBSNG_RC_CFG1_PLD0_RES_POL_Msk 0x40000000UL
/* UDB_UDBPAIR_UDBSNG.DPATH_OPC */
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_CMP_SEL_Pos 0UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_CMP_SEL_Msk 0x1UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SI_SEL_Pos 1UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SI_SEL_Msk 0x2UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_CI_SEL_Pos 2UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_CI_SEL_Msk 0x4UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_CFB_EN_Pos 3UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_CFB_EN_Msk 0x8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_A1_WR_SRC_Pos 4UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_A1_WR_SRC_Msk 0x30UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_A0_WR_SRC_Pos 6UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_A0_WR_SRC_Msk 0xC0UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SHIFT_Pos 8UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SHIFT_Msk 0x300UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SRC_B_Pos 10UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SRC_B_Msk 0xC00UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SRC_A_Pos 12UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_SRC_A_Msk 0x1000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_FUNC_Pos 13UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC0_FUNC_Msk 0xE000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_CMP_SEL_Pos 16UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_CMP_SEL_Msk 0x10000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SI_SEL_Pos 17UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SI_SEL_Msk 0x20000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_CI_SEL_Pos 18UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_CI_SEL_Msk 0x40000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_CFB_EN_Pos 19UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_CFB_EN_Msk 0x80000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_A1_WR_SRC_Pos 20UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_A1_WR_SRC_Msk 0x300000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_A0_WR_SRC_Pos 22UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_A0_WR_SRC_Msk 0xC00000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SHIFT_Pos 24UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SHIFT_Msk 0x3000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SRC_B_Pos 26UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SRC_B_Msk 0xC000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SRC_A_Pos 28UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_SRC_A_Msk 0x10000000UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_FUNC_Pos 29UL
#define UDB_UDBPAIR_UDBSNG_DPATH_OPC_OPC1_FUNC_Msk 0xE0000000UL


/* UDB_UDBPAIR_ROUTE.TOP_V_BOT */
#define UDB_UDBPAIR_ROUTE_TOP_V_BOT_TOP_V_BOT_Pos 0UL
#define UDB_UDBPAIR_ROUTE_TOP_V_BOT_TOP_V_BOT_Msk 0xFFFFFFFFUL
/* UDB_UDBPAIR_ROUTE.LVO1_V_2 */
#define UDB_UDBPAIR_ROUTE_LVO1_V_2_LVO1_V_2_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LVO1_V_2_LVO1_V_2_Msk 0xFFFFFFFFUL
/* UDB_UDBPAIR_ROUTE.RVO1_V_2 */
#define UDB_UDBPAIR_ROUTE_RVO1_V_2_RVO1_V_2_Pos 0UL
#define UDB_UDBPAIR_ROUTE_RVO1_V_2_RVO1_V_2_Msk 0xFFFFFFFFUL
/* UDB_UDBPAIR_ROUTE.TUI_CFG0 */
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI0SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI0SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI1SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI1SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI2SEL_Pos  8UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI2SEL_Msk  0xF00UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI3SEL_Pos  12UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI3SEL_Msk  0xF000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI4SEL_Pos  16UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI4SEL_Msk  0xF0000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI5SEL_Pos  20UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI5SEL_Msk  0xF00000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI6SEL_Pos  24UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI6SEL_Msk  0xF000000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI7SEL_Pos  28UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG0_TUI7SEL_Msk  0xF0000000UL
/* UDB_UDBPAIR_ROUTE.TUI_CFG1 */
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI8SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI8SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI9SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI9SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI10SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI10SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI11SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI11SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI12SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI12SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI13SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI13SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI14SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI14SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI15SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG1_TUI15SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.TUI_CFG2 */
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI16SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI16SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI17SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI17SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI18SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI18SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI19SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI19SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI20SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI20SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI21SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI21SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI22SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI22SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI23SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG2_TUI23SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.TUI_CFG3 */
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI24SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI24SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI25SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI25SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI26SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI26SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI27SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI27SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI28SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI28SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI29SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI29SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI30SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI30SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI31SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG3_TUI31SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.TUI_CFG4 */
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI32SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI32SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI33SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI33SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI34SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI34SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI35SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI35SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI36SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI36SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI37SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI37SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI38SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI38SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI39SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG4_TUI39SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.TUI_CFG5 */
#define UDB_UDBPAIR_ROUTE_TUI_CFG5_TUI40SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG5_TUI40SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_TUI_CFG5_TUI41SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_TUI_CFG5_TUI41SEL_Msk 0xF0UL
/* UDB_UDBPAIR_ROUTE.BUI_CFG0 */
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI0SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI0SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI1SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI1SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI2SEL_Pos  8UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI2SEL_Msk  0xF00UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI3SEL_Pos  12UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI3SEL_Msk  0xF000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI4SEL_Pos  16UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI4SEL_Msk  0xF0000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI5SEL_Pos  20UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI5SEL_Msk  0xF00000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI6SEL_Pos  24UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI6SEL_Msk  0xF000000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI7SEL_Pos  28UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG0_BUI7SEL_Msk  0xF0000000UL
/* UDB_UDBPAIR_ROUTE.BUI_CFG1 */
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI8SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI8SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI9SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI9SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI10SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI10SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI11SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI11SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI12SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI12SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI13SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI13SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI14SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI14SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI15SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG1_BUI15SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.BUI_CFG2 */
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI16SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI16SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI17SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI17SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI18SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI18SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI19SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI19SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI20SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI20SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI21SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI21SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI22SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI22SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI23SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG2_BUI23SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.BUI_CFG3 */
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI24SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI24SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI25SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI25SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI26SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI26SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI27SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI27SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI28SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI28SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI29SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI29SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI30SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI30SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI31SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG3_BUI31SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.BUI_CFG4 */
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI32SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI32SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI33SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI33SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI34SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI34SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI35SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI35SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI36SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI36SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI37SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI37SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI38SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI38SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI39SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG4_BUI39SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.BUI_CFG5 */
#define UDB_UDBPAIR_ROUTE_BUI_CFG5_BUI40SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG5_BUI40SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_BUI_CFG5_BUI41SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_BUI_CFG5_BUI41SEL_Msk 0xF0UL
/* UDB_UDBPAIR_ROUTE.RVO_CFG0 */
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO0SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO0SEL_Msk  0x1FUL
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO1SEL_Pos  8UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO1SEL_Msk  0x1F00UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO2SEL_Pos  16UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO2SEL_Msk  0x1F0000UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO3SEL_Pos  24UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG0_RVO3SEL_Msk  0x1F000000UL
/* UDB_UDBPAIR_ROUTE.RVO_CFG1 */
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO4SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO4SEL_Msk  0x1FUL
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO5SEL_Pos  8UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO5SEL_Msk  0x1F00UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO6SEL_Pos  16UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO6SEL_Msk  0x1F0000UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO7SEL_Pos  24UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG1_RVO7SEL_Msk  0x1F000000UL
/* UDB_UDBPAIR_ROUTE.RVO_CFG2 */
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO8SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO8SEL_Msk  0x1FUL
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO9SEL_Pos  8UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO9SEL_Msk  0x1F00UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO10SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO10SEL_Msk 0x1F0000UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO11SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG2_RVO11SEL_Msk 0x1F000000UL
/* UDB_UDBPAIR_ROUTE.RVO_CFG3 */
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO12SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO12SEL_Msk 0x1FUL
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO13SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO13SEL_Msk 0x1F00UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO14SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO14SEL_Msk 0x1F0000UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO15SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_RVO_CFG3_RVO15SEL_Msk 0x1F000000UL
/* UDB_UDBPAIR_ROUTE.LVO_CFG0 */
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO0SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO0SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO1SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO1SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO2SEL_Pos  8UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO2SEL_Msk  0xF00UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO3SEL_Pos  12UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO3SEL_Msk  0xF000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO4SEL_Pos  16UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO4SEL_Msk  0xF0000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO5SEL_Pos  20UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO5SEL_Msk  0xF00000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO6SEL_Pos  24UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO6SEL_Msk  0xF000000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO7SEL_Pos  28UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG0_LVO7SEL_Msk  0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LVO_CFG1 */
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO8SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO8SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO9SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO9SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO10SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO10SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO11SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO11SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO12SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO12SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO13SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO13SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO14SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO14SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO15SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LVO_CFG1_LVO15SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.RHO_CFG0 */
#define UDB_UDBPAIR_ROUTE_RHO_CFG0_RHOSEL_Pos   0UL
#define UDB_UDBPAIR_ROUTE_RHO_CFG0_RHOSEL_Msk   0xFFFFFFFFUL
/* UDB_UDBPAIR_ROUTE.RHO_CFG1 */
#define UDB_UDBPAIR_ROUTE_RHO_CFG1_RHOSEL_Pos   0UL
#define UDB_UDBPAIR_ROUTE_RHO_CFG1_RHOSEL_Msk   0xFFFFFFFFUL
/* UDB_UDBPAIR_ROUTE.RHO_CFG2 */
#define UDB_UDBPAIR_ROUTE_RHO_CFG2_RHOSEL_Pos   0UL
#define UDB_UDBPAIR_ROUTE_RHO_CFG2_RHOSEL_Msk   0xFFFFFFFFUL
/* UDB_UDBPAIR_ROUTE.LHO_CFG0 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO0SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO0SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO1SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO1SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO2SEL_Pos  8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO2SEL_Msk  0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO3SEL_Pos  12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO3SEL_Msk  0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO4SEL_Pos  16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO4SEL_Msk  0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO5SEL_Pos  20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO5SEL_Msk  0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO6SEL_Pos  24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO6SEL_Msk  0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO7SEL_Pos  28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG0_LHO7SEL_Msk  0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG1 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO8SEL_Pos  0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO8SEL_Msk  0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO9SEL_Pos  4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO9SEL_Msk  0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO10SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO10SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO11SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO11SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO12SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO12SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO13SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO13SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO14SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO14SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO15SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG1_LHO15SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG2 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO16SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO16SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO17SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO17SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO18SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO18SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO19SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO19SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO20SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO20SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO21SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO21SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO22SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO22SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO23SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG2_LHO23SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG3 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO24SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO24SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO25SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO25SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO26SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO26SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO27SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO27SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO28SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO28SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO29SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO29SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO30SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO30SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO31SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG3_LHO31SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG4 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO32SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO32SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO33SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO33SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO34SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO34SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO35SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO35SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO36SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO36SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO37SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO37SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO38SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO38SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO39SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG4_LHO39SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG5 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO40SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO40SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO41SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO41SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO42SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO42SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO43SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO43SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO44SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO44SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO45SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO45SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO46SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO46SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO47SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG5_LHO47SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG6 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO48SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO48SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO49SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO49SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO50SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO50SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO51SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO51SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO52SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO52SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO53SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO53SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO54SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO54SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO55SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG6_LHO55SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG7 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO56SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO56SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO57SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO57SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO58SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO58SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO59SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO59SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO60SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO60SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO61SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO61SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO62SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO62SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO63SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG7_LHO63SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG8 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO64SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO64SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO65SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO65SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO66SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO66SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO67SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO67SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO68SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO68SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO69SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO69SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO70SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO70SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO71SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG8_LHO71SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG9 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO72SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO72SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO73SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO73SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO74SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO74SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO75SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO75SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO76SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO76SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO77SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO77SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO78SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO78SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO79SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG9_LHO79SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG10 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO80SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO80SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO81SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO81SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO82SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO82SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO83SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO83SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO84SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO84SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO85SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO85SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO86SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO86SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO87SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG10_LHO87SEL_Msk 0xF0000000UL
/* UDB_UDBPAIR_ROUTE.LHO_CFG11 */
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO88SEL_Pos 0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO88SEL_Msk 0xFUL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO89SEL_Pos 4UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO89SEL_Msk 0xF0UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO90SEL_Pos 8UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO90SEL_Msk 0xF00UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO91SEL_Pos 12UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO91SEL_Msk 0xF000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO92SEL_Pos 16UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO92SEL_Msk 0xF0000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO93SEL_Pos 20UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO93SEL_Msk 0xF00000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO94SEL_Pos 24UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO94SEL_Msk 0xF000000UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO95SEL_Pos 28UL
#define UDB_UDBPAIR_ROUTE_LHO_CFG11_LHO95SEL_Msk 0xF0000000UL


/* UDB_DSI.LVO1_V_2 */
#define UDB_DSI_LVO1_V_2_LVO1_V_2_Pos           0UL
#define UDB_DSI_LVO1_V_2_LVO1_V_2_Msk           0xFFFFFFFFUL
/* UDB_DSI.RVO1_V_2 */
#define UDB_DSI_RVO1_V_2_RVO1_V_2_Pos           0UL
#define UDB_DSI_RVO1_V_2_RVO1_V_2_Msk           0xFFFFFFFFUL
/* UDB_DSI.DOP_CFG0 */
#define UDB_DSI_DOP_CFG0_DOP0SEL_Pos            0UL
#define UDB_DSI_DOP_CFG0_DOP0SEL_Msk            0x1FUL
#define UDB_DSI_DOP_CFG0_DOP1SEL_Pos            8UL
#define UDB_DSI_DOP_CFG0_DOP1SEL_Msk            0x1F00UL
#define UDB_DSI_DOP_CFG0_DOP2SEL_Pos            16UL
#define UDB_DSI_DOP_CFG0_DOP2SEL_Msk            0x1F0000UL
#define UDB_DSI_DOP_CFG0_DOP3SEL_Pos            24UL
#define UDB_DSI_DOP_CFG0_DOP3SEL_Msk            0x1F000000UL
/* UDB_DSI.DOP_CFG1 */
#define UDB_DSI_DOP_CFG1_DOP4SEL_Pos            0UL
#define UDB_DSI_DOP_CFG1_DOP4SEL_Msk            0x1FUL
#define UDB_DSI_DOP_CFG1_DOP5SEL_Pos            8UL
#define UDB_DSI_DOP_CFG1_DOP5SEL_Msk            0x1F00UL
#define UDB_DSI_DOP_CFG1_DOP6SEL_Pos            16UL
#define UDB_DSI_DOP_CFG1_DOP6SEL_Msk            0x1F0000UL
#define UDB_DSI_DOP_CFG1_DOP7SEL_Pos            24UL
#define UDB_DSI_DOP_CFG1_DOP7SEL_Msk            0x1F000000UL
/* UDB_DSI.DOP_CFG2 */
#define UDB_DSI_DOP_CFG2_DOP8SEL_Pos            0UL
#define UDB_DSI_DOP_CFG2_DOP8SEL_Msk            0x1FUL
#define UDB_DSI_DOP_CFG2_DOP9SEL_Pos            8UL
#define UDB_DSI_DOP_CFG2_DOP9SEL_Msk            0x1F00UL
#define UDB_DSI_DOP_CFG2_DOP10SEL_Pos           16UL
#define UDB_DSI_DOP_CFG2_DOP10SEL_Msk           0x1F0000UL
#define UDB_DSI_DOP_CFG2_DOP11SEL_Pos           24UL
#define UDB_DSI_DOP_CFG2_DOP11SEL_Msk           0x1F000000UL
/* UDB_DSI.DOP_CFG3 */
#define UDB_DSI_DOP_CFG3_DOP12SEL_Pos           0UL
#define UDB_DSI_DOP_CFG3_DOP12SEL_Msk           0x1FUL
#define UDB_DSI_DOP_CFG3_DOP13SEL_Pos           8UL
#define UDB_DSI_DOP_CFG3_DOP13SEL_Msk           0x1F00UL
#define UDB_DSI_DOP_CFG3_DOP14SEL_Pos           16UL
#define UDB_DSI_DOP_CFG3_DOP14SEL_Msk           0x1F0000UL
#define UDB_DSI_DOP_CFG3_DOP15SEL_Pos           24UL
#define UDB_DSI_DOP_CFG3_DOP15SEL_Msk           0x1F000000UL
/* UDB_DSI.DOT_CFG0 */
#define UDB_DSI_DOT_CFG0_DOT0SEL_Pos            0UL
#define UDB_DSI_DOT_CFG0_DOT0SEL_Msk            0x1FUL
#define UDB_DSI_DOT_CFG0_DOT1SEL_Pos            8UL
#define UDB_DSI_DOT_CFG0_DOT1SEL_Msk            0x1F00UL
#define UDB_DSI_DOT_CFG0_DOT2SEL_Pos            16UL
#define UDB_DSI_DOT_CFG0_DOT2SEL_Msk            0x1F0000UL
#define UDB_DSI_DOT_CFG0_DOT3SEL_Pos            24UL
#define UDB_DSI_DOT_CFG0_DOT3SEL_Msk            0x1F000000UL
/* UDB_DSI.DOT_CFG1 */
#define UDB_DSI_DOT_CFG1_DOT4SEL_Pos            0UL
#define UDB_DSI_DOT_CFG1_DOT4SEL_Msk            0x1FUL
#define UDB_DSI_DOT_CFG1_DOT5SEL_Pos            8UL
#define UDB_DSI_DOT_CFG1_DOT5SEL_Msk            0x1F00UL
#define UDB_DSI_DOT_CFG1_DOT6SEL_Pos            16UL
#define UDB_DSI_DOT_CFG1_DOT6SEL_Msk            0x1F0000UL
#define UDB_DSI_DOT_CFG1_DOT7SEL_Pos            24UL
#define UDB_DSI_DOT_CFG1_DOT7SEL_Msk            0x1F000000UL
/* UDB_DSI.DOT_CFG2 */
#define UDB_DSI_DOT_CFG2_DOT8SEL_Pos            0UL
#define UDB_DSI_DOT_CFG2_DOT8SEL_Msk            0x1FUL
#define UDB_DSI_DOT_CFG2_DOT9SEL_Pos            8UL
#define UDB_DSI_DOT_CFG2_DOT9SEL_Msk            0x1F00UL
#define UDB_DSI_DOT_CFG2_DOT10SEL_Pos           16UL
#define UDB_DSI_DOT_CFG2_DOT10SEL_Msk           0x1F0000UL
#define UDB_DSI_DOT_CFG2_DOT11SEL_Pos           24UL
#define UDB_DSI_DOT_CFG2_DOT11SEL_Msk           0x1F000000UL
/* UDB_DSI.DOT_CFG3 */
#define UDB_DSI_DOT_CFG3_DOT12SEL_Pos           0UL
#define UDB_DSI_DOT_CFG3_DOT12SEL_Msk           0x1FUL
#define UDB_DSI_DOT_CFG3_DOT13SEL_Pos           8UL
#define UDB_DSI_DOT_CFG3_DOT13SEL_Msk           0x1F00UL
#define UDB_DSI_DOT_CFG3_DOT14SEL_Pos           16UL
#define UDB_DSI_DOT_CFG3_DOT14SEL_Msk           0x1F0000UL
#define UDB_DSI_DOT_CFG3_DOT15SEL_Pos           24UL
#define UDB_DSI_DOT_CFG3_DOT15SEL_Msk           0x1F000000UL
/* UDB_DSI.RVO_CFG0 */
#define UDB_DSI_RVO_CFG0_RVO0SEL_Pos            0UL
#define UDB_DSI_RVO_CFG0_RVO0SEL_Msk            0x1FUL
#define UDB_DSI_RVO_CFG0_RVO1SEL_Pos            8UL
#define UDB_DSI_RVO_CFG0_RVO1SEL_Msk            0x1F00UL
#define UDB_DSI_RVO_CFG0_RVO2SEL_Pos            16UL
#define UDB_DSI_RVO_CFG0_RVO2SEL_Msk            0x1F0000UL
#define UDB_DSI_RVO_CFG0_RVO3SEL_Pos            24UL
#define UDB_DSI_RVO_CFG0_RVO3SEL_Msk            0x1F000000UL
/* UDB_DSI.RVO_CFG1 */
#define UDB_DSI_RVO_CFG1_RVO4SEL_Pos            0UL
#define UDB_DSI_RVO_CFG1_RVO4SEL_Msk            0x1FUL
#define UDB_DSI_RVO_CFG1_RVO5SEL_Pos            8UL
#define UDB_DSI_RVO_CFG1_RVO5SEL_Msk            0x1F00UL
#define UDB_DSI_RVO_CFG1_RVO6SEL_Pos            16UL
#define UDB_DSI_RVO_CFG1_RVO6SEL_Msk            0x1F0000UL
#define UDB_DSI_RVO_CFG1_RVO7SEL_Pos            24UL
#define UDB_DSI_RVO_CFG1_RVO7SEL_Msk            0x1F000000UL
/* UDB_DSI.RVO_CFG2 */
#define UDB_DSI_RVO_CFG2_RVO8SEL_Pos            0UL
#define UDB_DSI_RVO_CFG2_RVO8SEL_Msk            0x1FUL
#define UDB_DSI_RVO_CFG2_RVO9SEL_Pos            8UL
#define UDB_DSI_RVO_CFG2_RVO9SEL_Msk            0x1F00UL
#define UDB_DSI_RVO_CFG2_RVO10SEL_Pos           16UL
#define UDB_DSI_RVO_CFG2_RVO10SEL_Msk           0x1F0000UL
#define UDB_DSI_RVO_CFG2_RVO11SEL_Pos           24UL
#define UDB_DSI_RVO_CFG2_RVO11SEL_Msk           0x1F000000UL
/* UDB_DSI.RVO_CFG3 */
#define UDB_DSI_RVO_CFG3_RVO12SEL_Pos           0UL
#define UDB_DSI_RVO_CFG3_RVO12SEL_Msk           0x1FUL
#define UDB_DSI_RVO_CFG3_RVO13SEL_Pos           8UL
#define UDB_DSI_RVO_CFG3_RVO13SEL_Msk           0x1F00UL
#define UDB_DSI_RVO_CFG3_RVO14SEL_Pos           16UL
#define UDB_DSI_RVO_CFG3_RVO14SEL_Msk           0x1F0000UL
#define UDB_DSI_RVO_CFG3_RVO15SEL_Pos           24UL
#define UDB_DSI_RVO_CFG3_RVO15SEL_Msk           0x1F000000UL
/* UDB_DSI.LVO_CFG0 */
#define UDB_DSI_LVO_CFG0_LVO0SEL_Pos            0UL
#define UDB_DSI_LVO_CFG0_LVO0SEL_Msk            0xFUL
#define UDB_DSI_LVO_CFG0_LVO1SEL_Pos            4UL
#define UDB_DSI_LVO_CFG0_LVO1SEL_Msk            0xF0UL
#define UDB_DSI_LVO_CFG0_LVO2SEL_Pos            8UL
#define UDB_DSI_LVO_CFG0_LVO2SEL_Msk            0xF00UL
#define UDB_DSI_LVO_CFG0_LVO3SEL_Pos            12UL
#define UDB_DSI_LVO_CFG0_LVO3SEL_Msk            0xF000UL
#define UDB_DSI_LVO_CFG0_LVO4SEL_Pos            16UL
#define UDB_DSI_LVO_CFG0_LVO4SEL_Msk            0xF0000UL
#define UDB_DSI_LVO_CFG0_LVO5SEL_Pos            20UL
#define UDB_DSI_LVO_CFG0_LVO5SEL_Msk            0xF00000UL
#define UDB_DSI_LVO_CFG0_LVO6SEL_Pos            24UL
#define UDB_DSI_LVO_CFG0_LVO6SEL_Msk            0xF000000UL
#define UDB_DSI_LVO_CFG0_LVO7SEL_Pos            28UL
#define UDB_DSI_LVO_CFG0_LVO7SEL_Msk            0xF0000000UL
/* UDB_DSI.LVO_CFG1 */
#define UDB_DSI_LVO_CFG1_LVO8SEL_Pos            0UL
#define UDB_DSI_LVO_CFG1_LVO8SEL_Msk            0xFUL
#define UDB_DSI_LVO_CFG1_LVO9SEL_Pos            4UL
#define UDB_DSI_LVO_CFG1_LVO9SEL_Msk            0xF0UL
#define UDB_DSI_LVO_CFG1_LVO10SEL_Pos           8UL
#define UDB_DSI_LVO_CFG1_LVO10SEL_Msk           0xF00UL
#define UDB_DSI_LVO_CFG1_LVO11SEL_Pos           12UL
#define UDB_DSI_LVO_CFG1_LVO11SEL_Msk           0xF000UL
#define UDB_DSI_LVO_CFG1_LVO12SEL_Pos           16UL
#define UDB_DSI_LVO_CFG1_LVO12SEL_Msk           0xF0000UL
#define UDB_DSI_LVO_CFG1_LVO13SEL_Pos           20UL
#define UDB_DSI_LVO_CFG1_LVO13SEL_Msk           0xF00000UL
#define UDB_DSI_LVO_CFG1_LVO14SEL_Pos           24UL
#define UDB_DSI_LVO_CFG1_LVO14SEL_Msk           0xF000000UL
#define UDB_DSI_LVO_CFG1_LVO15SEL_Pos           28UL
#define UDB_DSI_LVO_CFG1_LVO15SEL_Msk           0xF0000000UL
/* UDB_DSI.RHO_CFG0 */
#define UDB_DSI_RHO_CFG0_RHOSEL_Pos             0UL
#define UDB_DSI_RHO_CFG0_RHOSEL_Msk             0xFFFFFFFFUL
/* UDB_DSI.RHO_CFG1 */
#define UDB_DSI_RHO_CFG1_RHOSEL_Pos             0UL
#define UDB_DSI_RHO_CFG1_RHOSEL_Msk             0xFFFFFFFFUL
/* UDB_DSI.RHO_CFG2 */
#define UDB_DSI_RHO_CFG2_RHOSEL_Pos             0UL
#define UDB_DSI_RHO_CFG2_RHOSEL_Msk             0xFFFFFFFFUL
/* UDB_DSI.LHO_CFG0 */
#define UDB_DSI_LHO_CFG0_LHO0SEL_Pos            0UL
#define UDB_DSI_LHO_CFG0_LHO0SEL_Msk            0xFUL
#define UDB_DSI_LHO_CFG0_LHO1SEL_Pos            4UL
#define UDB_DSI_LHO_CFG0_LHO1SEL_Msk            0xF0UL
#define UDB_DSI_LHO_CFG0_LHO2SEL_Pos            8UL
#define UDB_DSI_LHO_CFG0_LHO2SEL_Msk            0xF00UL
#define UDB_DSI_LHO_CFG0_LHO3SEL_Pos            12UL
#define UDB_DSI_LHO_CFG0_LHO3SEL_Msk            0xF000UL
#define UDB_DSI_LHO_CFG0_LHO4SEL_Pos            16UL
#define UDB_DSI_LHO_CFG0_LHO4SEL_Msk            0xF0000UL
#define UDB_DSI_LHO_CFG0_LHO5SEL_Pos            20UL
#define UDB_DSI_LHO_CFG0_LHO5SEL_Msk            0xF00000UL
#define UDB_DSI_LHO_CFG0_LHO6SEL_Pos            24UL
#define UDB_DSI_LHO_CFG0_LHO6SEL_Msk            0xF000000UL
#define UDB_DSI_LHO_CFG0_LHO7SEL_Pos            28UL
#define UDB_DSI_LHO_CFG0_LHO7SEL_Msk            0xF0000000UL
/* UDB_DSI.LHO_CFG1 */
#define UDB_DSI_LHO_CFG1_LHO8SEL_Pos            0UL
#define UDB_DSI_LHO_CFG1_LHO8SEL_Msk            0xFUL
#define UDB_DSI_LHO_CFG1_LHO9SEL_Pos            4UL
#define UDB_DSI_LHO_CFG1_LHO9SEL_Msk            0xF0UL
#define UDB_DSI_LHO_CFG1_LHO10SEL_Pos           8UL
#define UDB_DSI_LHO_CFG1_LHO10SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG1_LHO11SEL_Pos           12UL
#define UDB_DSI_LHO_CFG1_LHO11SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG1_LHO12SEL_Pos           16UL
#define UDB_DSI_LHO_CFG1_LHO12SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG1_LHO13SEL_Pos           20UL
#define UDB_DSI_LHO_CFG1_LHO13SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG1_LHO14SEL_Pos           24UL
#define UDB_DSI_LHO_CFG1_LHO14SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG1_LHO15SEL_Pos           28UL
#define UDB_DSI_LHO_CFG1_LHO15SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG2 */
#define UDB_DSI_LHO_CFG2_LHO16SEL_Pos           0UL
#define UDB_DSI_LHO_CFG2_LHO16SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG2_LHO17SEL_Pos           4UL
#define UDB_DSI_LHO_CFG2_LHO17SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG2_LHO18SEL_Pos           8UL
#define UDB_DSI_LHO_CFG2_LHO18SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG2_LHO19SEL_Pos           12UL
#define UDB_DSI_LHO_CFG2_LHO19SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG2_LHO20SEL_Pos           16UL
#define UDB_DSI_LHO_CFG2_LHO20SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG2_LHO21SEL_Pos           20UL
#define UDB_DSI_LHO_CFG2_LHO21SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG2_LHO22SEL_Pos           24UL
#define UDB_DSI_LHO_CFG2_LHO22SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG2_LHO23SEL_Pos           28UL
#define UDB_DSI_LHO_CFG2_LHO23SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG3 */
#define UDB_DSI_LHO_CFG3_LHO24SEL_Pos           0UL
#define UDB_DSI_LHO_CFG3_LHO24SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG3_LHO25SEL_Pos           4UL
#define UDB_DSI_LHO_CFG3_LHO25SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG3_LHO26SEL_Pos           8UL
#define UDB_DSI_LHO_CFG3_LHO26SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG3_LHO27SEL_Pos           12UL
#define UDB_DSI_LHO_CFG3_LHO27SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG3_LHO28SEL_Pos           16UL
#define UDB_DSI_LHO_CFG3_LHO28SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG3_LHO29SEL_Pos           20UL
#define UDB_DSI_LHO_CFG3_LHO29SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG3_LHO30SEL_Pos           24UL
#define UDB_DSI_LHO_CFG3_LHO30SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG3_LHO31SEL_Pos           28UL
#define UDB_DSI_LHO_CFG3_LHO31SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG4 */
#define UDB_DSI_LHO_CFG4_LHO32SEL_Pos           0UL
#define UDB_DSI_LHO_CFG4_LHO32SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG4_LHO33SEL_Pos           4UL
#define UDB_DSI_LHO_CFG4_LHO33SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG4_LHO34SEL_Pos           8UL
#define UDB_DSI_LHO_CFG4_LHO34SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG4_LHO35SEL_Pos           12UL
#define UDB_DSI_LHO_CFG4_LHO35SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG4_LHO36SEL_Pos           16UL
#define UDB_DSI_LHO_CFG4_LHO36SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG4_LHO37SEL_Pos           20UL
#define UDB_DSI_LHO_CFG4_LHO37SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG4_LHO38SEL_Pos           24UL
#define UDB_DSI_LHO_CFG4_LHO38SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG4_LHO39SEL_Pos           28UL
#define UDB_DSI_LHO_CFG4_LHO39SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG5 */
#define UDB_DSI_LHO_CFG5_LHO40SEL_Pos           0UL
#define UDB_DSI_LHO_CFG5_LHO40SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG5_LHO41SEL_Pos           4UL
#define UDB_DSI_LHO_CFG5_LHO41SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG5_LHO42SEL_Pos           8UL
#define UDB_DSI_LHO_CFG5_LHO42SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG5_LHO43SEL_Pos           12UL
#define UDB_DSI_LHO_CFG5_LHO43SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG5_LHO44SEL_Pos           16UL
#define UDB_DSI_LHO_CFG5_LHO44SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG5_LHO45SEL_Pos           20UL
#define UDB_DSI_LHO_CFG5_LHO45SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG5_LHO46SEL_Pos           24UL
#define UDB_DSI_LHO_CFG5_LHO46SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG5_LHO47SEL_Pos           28UL
#define UDB_DSI_LHO_CFG5_LHO47SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG6 */
#define UDB_DSI_LHO_CFG6_LHO48SEL_Pos           0UL
#define UDB_DSI_LHO_CFG6_LHO48SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG6_LHO49SEL_Pos           4UL
#define UDB_DSI_LHO_CFG6_LHO49SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG6_LHO50SEL_Pos           8UL
#define UDB_DSI_LHO_CFG6_LHO50SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG6_LHO51SEL_Pos           12UL
#define UDB_DSI_LHO_CFG6_LHO51SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG6_LHO52SEL_Pos           16UL
#define UDB_DSI_LHO_CFG6_LHO52SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG6_LHO53SEL_Pos           20UL
#define UDB_DSI_LHO_CFG6_LHO53SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG6_LHO54SEL_Pos           24UL
#define UDB_DSI_LHO_CFG6_LHO54SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG6_LHO55SEL_Pos           28UL
#define UDB_DSI_LHO_CFG6_LHO55SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG7 */
#define UDB_DSI_LHO_CFG7_LHO56SEL_Pos           0UL
#define UDB_DSI_LHO_CFG7_LHO56SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG7_LHO57SEL_Pos           4UL
#define UDB_DSI_LHO_CFG7_LHO57SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG7_LHO58SEL_Pos           8UL
#define UDB_DSI_LHO_CFG7_LHO58SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG7_LHO59SEL_Pos           12UL
#define UDB_DSI_LHO_CFG7_LHO59SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG7_LHO60SEL_Pos           16UL
#define UDB_DSI_LHO_CFG7_LHO60SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG7_LHO61SEL_Pos           20UL
#define UDB_DSI_LHO_CFG7_LHO61SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG7_LHO62SEL_Pos           24UL
#define UDB_DSI_LHO_CFG7_LHO62SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG7_LHO63SEL_Pos           28UL
#define UDB_DSI_LHO_CFG7_LHO63SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG8 */
#define UDB_DSI_LHO_CFG8_LHO64SEL_Pos           0UL
#define UDB_DSI_LHO_CFG8_LHO64SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG8_LHO65SEL_Pos           4UL
#define UDB_DSI_LHO_CFG8_LHO65SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG8_LHO66SEL_Pos           8UL
#define UDB_DSI_LHO_CFG8_LHO66SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG8_LHO67SEL_Pos           12UL
#define UDB_DSI_LHO_CFG8_LHO67SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG8_LHO68SEL_Pos           16UL
#define UDB_DSI_LHO_CFG8_LHO68SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG8_LHO69SEL_Pos           20UL
#define UDB_DSI_LHO_CFG8_LHO69SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG8_LHO70SEL_Pos           24UL
#define UDB_DSI_LHO_CFG8_LHO70SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG8_LHO71SEL_Pos           28UL
#define UDB_DSI_LHO_CFG8_LHO71SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG9 */
#define UDB_DSI_LHO_CFG9_LHO72SEL_Pos           0UL
#define UDB_DSI_LHO_CFG9_LHO72SEL_Msk           0xFUL
#define UDB_DSI_LHO_CFG9_LHO73SEL_Pos           4UL
#define UDB_DSI_LHO_CFG9_LHO73SEL_Msk           0xF0UL
#define UDB_DSI_LHO_CFG9_LHO74SEL_Pos           8UL
#define UDB_DSI_LHO_CFG9_LHO74SEL_Msk           0xF00UL
#define UDB_DSI_LHO_CFG9_LHO75SEL_Pos           12UL
#define UDB_DSI_LHO_CFG9_LHO75SEL_Msk           0xF000UL
#define UDB_DSI_LHO_CFG9_LHO76SEL_Pos           16UL
#define UDB_DSI_LHO_CFG9_LHO76SEL_Msk           0xF0000UL
#define UDB_DSI_LHO_CFG9_LHO77SEL_Pos           20UL
#define UDB_DSI_LHO_CFG9_LHO77SEL_Msk           0xF00000UL
#define UDB_DSI_LHO_CFG9_LHO78SEL_Pos           24UL
#define UDB_DSI_LHO_CFG9_LHO78SEL_Msk           0xF000000UL
#define UDB_DSI_LHO_CFG9_LHO79SEL_Pos           28UL
#define UDB_DSI_LHO_CFG9_LHO79SEL_Msk           0xF0000000UL
/* UDB_DSI.LHO_CFG10 */
#define UDB_DSI_LHO_CFG10_LHO80SEL_Pos          0UL
#define UDB_DSI_LHO_CFG10_LHO80SEL_Msk          0xFUL
#define UDB_DSI_LHO_CFG10_LHO81SEL_Pos          4UL
#define UDB_DSI_LHO_CFG10_LHO81SEL_Msk          0xF0UL
#define UDB_DSI_LHO_CFG10_LHO82SEL_Pos          8UL
#define UDB_DSI_LHO_CFG10_LHO82SEL_Msk          0xF00UL
#define UDB_DSI_LHO_CFG10_LHO83SEL_Pos          12UL
#define UDB_DSI_LHO_CFG10_LHO83SEL_Msk          0xF000UL
#define UDB_DSI_LHO_CFG10_LHO84SEL_Pos          16UL
#define UDB_DSI_LHO_CFG10_LHO84SEL_Msk          0xF0000UL
#define UDB_DSI_LHO_CFG10_LHO85SEL_Pos          20UL
#define UDB_DSI_LHO_CFG10_LHO85SEL_Msk          0xF00000UL
#define UDB_DSI_LHO_CFG10_LHO86SEL_Pos          24UL
#define UDB_DSI_LHO_CFG10_LHO86SEL_Msk          0xF000000UL
#define UDB_DSI_LHO_CFG10_LHO87SEL_Pos          28UL
#define UDB_DSI_LHO_CFG10_LHO87SEL_Msk          0xF0000000UL
/* UDB_DSI.LHO_CFG11 */
#define UDB_DSI_LHO_CFG11_LHO88SEL_Pos          0UL
#define UDB_DSI_LHO_CFG11_LHO88SEL_Msk          0xFUL
#define UDB_DSI_LHO_CFG11_LHO89SEL_Pos          4UL
#define UDB_DSI_LHO_CFG11_LHO89SEL_Msk          0xF0UL
#define UDB_DSI_LHO_CFG11_LHO90SEL_Pos          8UL
#define UDB_DSI_LHO_CFG11_LHO90SEL_Msk          0xF00UL
#define UDB_DSI_LHO_CFG11_LHO91SEL_Pos          12UL
#define UDB_DSI_LHO_CFG11_LHO91SEL_Msk          0xF000UL
#define UDB_DSI_LHO_CFG11_LHO92SEL_Pos          16UL
#define UDB_DSI_LHO_CFG11_LHO92SEL_Msk          0xF0000UL
#define UDB_DSI_LHO_CFG11_LHO93SEL_Pos          20UL
#define UDB_DSI_LHO_CFG11_LHO93SEL_Msk          0xF00000UL
#define UDB_DSI_LHO_CFG11_LHO94SEL_Pos          24UL
#define UDB_DSI_LHO_CFG11_LHO94SEL_Msk          0xF000000UL
#define UDB_DSI_LHO_CFG11_LHO95SEL_Pos          28UL
#define UDB_DSI_LHO_CFG11_LHO95SEL_Msk          0xF0000000UL


/* UDB_PA.CFG0 */
#define UDB_PA_CFG0_CLKIN_EN_SEL_Pos            0UL
#define UDB_PA_CFG0_CLKIN_EN_SEL_Msk            0x3UL
#define UDB_PA_CFG0_CLKIN_EN_MODE_Pos           2UL
#define UDB_PA_CFG0_CLKIN_EN_MODE_Msk           0xCUL
#define UDB_PA_CFG0_CLKIN_EN_INV_Pos            4UL
#define UDB_PA_CFG0_CLKIN_EN_INV_Msk            0x10UL
#define UDB_PA_CFG0_CLKIN_INV_Pos               5UL
#define UDB_PA_CFG0_CLKIN_INV_Msk               0x20UL
/* UDB_PA.CFG1 */
#define UDB_PA_CFG1_CLKOUT_EN_SEL_Pos           0UL
#define UDB_PA_CFG1_CLKOUT_EN_SEL_Msk           0x3UL
#define UDB_PA_CFG1_CLKOUT_EN_MODE_Pos          2UL
#define UDB_PA_CFG1_CLKOUT_EN_MODE_Msk          0xCUL
#define UDB_PA_CFG1_CLKOUT_EN_INV_Pos           4UL
#define UDB_PA_CFG1_CLKOUT_EN_INV_Msk           0x10UL
#define UDB_PA_CFG1_CLKOUT_INV_Pos              5UL
#define UDB_PA_CFG1_CLKOUT_INV_Msk              0x20UL
/* UDB_PA.CFG2 */
#define UDB_PA_CFG2_CLKIN_SEL_Pos               0UL
#define UDB_PA_CFG2_CLKIN_SEL_Msk               0xFUL
#define UDB_PA_CFG2_CLKOUT_SEL_Pos              4UL
#define UDB_PA_CFG2_CLKOUT_SEL_Msk              0xF0UL
/* UDB_PA.CFG3 */
#define UDB_PA_CFG3_RES_IN_SEL_Pos              0UL
#define UDB_PA_CFG3_RES_IN_SEL_Msk              0x3UL
#define UDB_PA_CFG3_RES_IN_INV_Pos              2UL
#define UDB_PA_CFG3_RES_IN_INV_Msk              0x4UL
#define UDB_PA_CFG3_RES_OUT_SEL_Pos             4UL
#define UDB_PA_CFG3_RES_OUT_SEL_Msk             0x30UL
#define UDB_PA_CFG3_RES_OUT_INV_Pos             6UL
#define UDB_PA_CFG3_RES_OUT_INV_Msk             0x40UL
/* UDB_PA.CFG4 */
#define UDB_PA_CFG4_RES_IN_EN_Pos               0UL
#define UDB_PA_CFG4_RES_IN_EN_Msk               0x1UL
#define UDB_PA_CFG4_RES_OUT_EN_Pos              1UL
#define UDB_PA_CFG4_RES_OUT_EN_Msk              0x2UL
#define UDB_PA_CFG4_RES_OE_EN_Pos               2UL
#define UDB_PA_CFG4_RES_OE_EN_Msk               0x4UL
/* UDB_PA.CFG5 */
#define UDB_PA_CFG5_PIN_SEL_Pos                 0UL
#define UDB_PA_CFG5_PIN_SEL_Msk                 0x7UL
/* UDB_PA.CFG6 */
#define UDB_PA_CFG6_IN_SYNC0_Pos                0UL
#define UDB_PA_CFG6_IN_SYNC0_Msk                0x3UL
#define UDB_PA_CFG6_IN_SYNC1_Pos                2UL
#define UDB_PA_CFG6_IN_SYNC1_Msk                0xCUL
#define UDB_PA_CFG6_IN_SYNC2_Pos                4UL
#define UDB_PA_CFG6_IN_SYNC2_Msk                0x30UL
#define UDB_PA_CFG6_IN_SYNC3_Pos                6UL
#define UDB_PA_CFG6_IN_SYNC3_Msk                0xC0UL
/* UDB_PA.CFG7 */
#define UDB_PA_CFG7_IN_SYNC4_Pos                0UL
#define UDB_PA_CFG7_IN_SYNC4_Msk                0x3UL
#define UDB_PA_CFG7_IN_SYNC5_Pos                2UL
#define UDB_PA_CFG7_IN_SYNC5_Msk                0xCUL
#define UDB_PA_CFG7_IN_SYNC6_Pos                4UL
#define UDB_PA_CFG7_IN_SYNC6_Msk                0x30UL
#define UDB_PA_CFG7_IN_SYNC7_Pos                6UL
#define UDB_PA_CFG7_IN_SYNC7_Msk                0xC0UL
/* UDB_PA.CFG8 */
#define UDB_PA_CFG8_OUT_SYNC0_Pos               0UL
#define UDB_PA_CFG8_OUT_SYNC0_Msk               0x3UL
#define UDB_PA_CFG8_OUT_SYNC1_Pos               2UL
#define UDB_PA_CFG8_OUT_SYNC1_Msk               0xCUL
#define UDB_PA_CFG8_OUT_SYNC2_Pos               4UL
#define UDB_PA_CFG8_OUT_SYNC2_Msk               0x30UL
#define UDB_PA_CFG8_OUT_SYNC3_Pos               6UL
#define UDB_PA_CFG8_OUT_SYNC3_Msk               0xC0UL
/* UDB_PA.CFG9 */
#define UDB_PA_CFG9_OUT_SYNC4_Pos               0UL
#define UDB_PA_CFG9_OUT_SYNC4_Msk               0x3UL
#define UDB_PA_CFG9_OUT_SYNC5_Pos               2UL
#define UDB_PA_CFG9_OUT_SYNC5_Msk               0xCUL
#define UDB_PA_CFG9_OUT_SYNC6_Pos               4UL
#define UDB_PA_CFG9_OUT_SYNC6_Msk               0x30UL
#define UDB_PA_CFG9_OUT_SYNC7_Pos               6UL
#define UDB_PA_CFG9_OUT_SYNC7_Msk               0xC0UL
/* UDB_PA.CFG10 */
#define UDB_PA_CFG10_DATA_SEL0_Pos              0UL
#define UDB_PA_CFG10_DATA_SEL0_Msk              0x3UL
#define UDB_PA_CFG10_DATA_SEL1_Pos              2UL
#define UDB_PA_CFG10_DATA_SEL1_Msk              0xCUL
#define UDB_PA_CFG10_DATA_SEL2_Pos              4UL
#define UDB_PA_CFG10_DATA_SEL2_Msk              0x30UL
#define UDB_PA_CFG10_DATA_SEL3_Pos              6UL
#define UDB_PA_CFG10_DATA_SEL3_Msk              0xC0UL
/* UDB_PA.CFG11 */
#define UDB_PA_CFG11_DATA_SEL4_Pos              0UL
#define UDB_PA_CFG11_DATA_SEL4_Msk              0x3UL
#define UDB_PA_CFG11_DATA_SEL5_Pos              2UL
#define UDB_PA_CFG11_DATA_SEL5_Msk              0xCUL
#define UDB_PA_CFG11_DATA_SEL6_Pos              4UL
#define UDB_PA_CFG11_DATA_SEL6_Msk              0x30UL
#define UDB_PA_CFG11_DATA_SEL7_Pos              6UL
#define UDB_PA_CFG11_DATA_SEL7_Msk              0xC0UL
/* UDB_PA.CFG12 */
#define UDB_PA_CFG12_OE_SEL0_Pos                0UL
#define UDB_PA_CFG12_OE_SEL0_Msk                0x3UL
#define UDB_PA_CFG12_OE_SEL1_Pos                2UL
#define UDB_PA_CFG12_OE_SEL1_Msk                0xCUL
#define UDB_PA_CFG12_OE_SEL2_Pos                4UL
#define UDB_PA_CFG12_OE_SEL2_Msk                0x30UL
#define UDB_PA_CFG12_OE_SEL3_Pos                6UL
#define UDB_PA_CFG12_OE_SEL3_Msk                0xC0UL
/* UDB_PA.CFG13 */
#define UDB_PA_CFG13_OE_SEL4_Pos                0UL
#define UDB_PA_CFG13_OE_SEL4_Msk                0x3UL
#define UDB_PA_CFG13_OE_SEL5_Pos                2UL
#define UDB_PA_CFG13_OE_SEL5_Msk                0xCUL
#define UDB_PA_CFG13_OE_SEL6_Pos                4UL
#define UDB_PA_CFG13_OE_SEL6_Msk                0x30UL
#define UDB_PA_CFG13_OE_SEL7_Pos                6UL
#define UDB_PA_CFG13_OE_SEL7_Msk                0xC0UL
/* UDB_PA.CFG14 */
#define UDB_PA_CFG14_OE_SYNC0_Pos               0UL
#define UDB_PA_CFG14_OE_SYNC0_Msk               0x3UL
#define UDB_PA_CFG14_OE_SYNC1_Pos               2UL
#define UDB_PA_CFG14_OE_SYNC1_Msk               0xCUL
#define UDB_PA_CFG14_OE_SYNC2_Pos               4UL
#define UDB_PA_CFG14_OE_SYNC2_Msk               0x30UL
#define UDB_PA_CFG14_OE_SYNC3_Pos               6UL
#define UDB_PA_CFG14_OE_SYNC3_Msk               0xC0UL


/* UDB_BCTL.MDCLK_EN */
#define UDB_BCTL_MDCLK_EN_DCEN_Pos              0UL
#define UDB_BCTL_MDCLK_EN_DCEN_Msk              0xFFUL
/* UDB_BCTL.MBCLK_EN */
#define UDB_BCTL_MBCLK_EN_BCEN_Pos              0UL
#define UDB_BCTL_MBCLK_EN_BCEN_Msk              0x1UL
/* UDB_BCTL.BOTSEL_L */
#define UDB_BCTL_BOTSEL_L_CLK_SEL0_Pos          0UL
#define UDB_BCTL_BOTSEL_L_CLK_SEL0_Msk          0x3UL
#define UDB_BCTL_BOTSEL_L_CLK_SEL1_Pos          2UL
#define UDB_BCTL_BOTSEL_L_CLK_SEL1_Msk          0xCUL
#define UDB_BCTL_BOTSEL_L_CLK_SEL2_Pos          4UL
#define UDB_BCTL_BOTSEL_L_CLK_SEL2_Msk          0x30UL
#define UDB_BCTL_BOTSEL_L_CLK_SEL3_Pos          6UL
#define UDB_BCTL_BOTSEL_L_CLK_SEL3_Msk          0xC0UL
/* UDB_BCTL.BOTSEL_U */
#define UDB_BCTL_BOTSEL_U_CLK_SEL4_Pos          0UL
#define UDB_BCTL_BOTSEL_U_CLK_SEL4_Msk          0x3UL
#define UDB_BCTL_BOTSEL_U_CLK_SEL5_Pos          2UL
#define UDB_BCTL_BOTSEL_U_CLK_SEL5_Msk          0xCUL
#define UDB_BCTL_BOTSEL_U_CLK_SEL6_Pos          4UL
#define UDB_BCTL_BOTSEL_U_CLK_SEL6_Msk          0x30UL
#define UDB_BCTL_BOTSEL_U_CLK_SEL7_Pos          6UL
#define UDB_BCTL_BOTSEL_U_CLK_SEL7_Msk          0xC0UL
/* UDB_BCTL.QCLK_EN */
#define UDB_BCTL_QCLK_EN_DCEN_Q_Pos             0UL
#define UDB_BCTL_QCLK_EN_DCEN_Q_Msk             0xFFUL
#define UDB_BCTL_QCLK_EN_BCEN_Q_Pos             8UL
#define UDB_BCTL_QCLK_EN_BCEN_Q_Msk             0x100UL
#define UDB_BCTL_QCLK_EN_DISABLE_ROUTE_Pos      11UL
#define UDB_BCTL_QCLK_EN_DISABLE_ROUTE_Msk      0x800UL


/* UDB_UDBIF.BANK_CTL */
#define UDB_UDBIF_BANK_CTL_DIS_COR_Pos          0UL
#define UDB_UDBIF_BANK_CTL_DIS_COR_Msk          0x1UL
#define UDB_UDBIF_BANK_CTL_ROUTE_EN_Pos         1UL
#define UDB_UDBIF_BANK_CTL_ROUTE_EN_Msk         0x2UL
#define UDB_UDBIF_BANK_CTL_BANK_EN_Pos          2UL
#define UDB_UDBIF_BANK_CTL_BANK_EN_Msk          0x4UL
#define UDB_UDBIF_BANK_CTL_READ_WAIT_Pos        8UL
#define UDB_UDBIF_BANK_CTL_READ_WAIT_Msk        0x100UL
/* UDB_UDBIF.INT_CLK_CTL */
#define UDB_UDBIF_INT_CLK_CTL_INT_CLK_ENABLE_Pos 0UL
#define UDB_UDBIF_INT_CLK_CTL_INT_CLK_ENABLE_Msk 0x1UL
/* UDB_UDBIF.INT_CFG */
#define UDB_UDBIF_INT_CFG_INT_MODE_CFG_Pos      0UL
#define UDB_UDBIF_INT_CFG_INT_MODE_CFG_Msk      0xFFFFFFFFUL
/* UDB_UDBIF.TR_CLK_CTL */
#define UDB_UDBIF_TR_CLK_CTL_TR_CLOCK_ENABLE_Pos 0UL
#define UDB_UDBIF_TR_CLK_CTL_TR_CLOCK_ENABLE_Msk 0x1UL
/* UDB_UDBIF.TR_CFG */
#define UDB_UDBIF_TR_CFG_TR_MODE_CFG_Pos        0UL
#define UDB_UDBIF_TR_CFG_TR_MODE_CFG_Msk        0xFFFFFFFFUL
/* UDB_UDBIF.PRIVATE */
#define UDB_UDBIF_PRIVATE_PIPELINE_MD_Pos       0UL
#define UDB_UDBIF_PRIVATE_PIPELINE_MD_Msk       0x1UL


#endif /* _CYIP_UDB_H_ */


/* [] END OF FILE */
