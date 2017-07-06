/**
 * \file
 *
 * \brief Header file for RDA5981A
 *
 * Copyright (c) 2017 RDA Microelectronics.
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
#ifndef _SDMMC_REG_H_
#define _SDMMC_REG_H_

#ifdef CT_ASM
#error "You are trying to use in an assembly code the normal H description of 'sdmmc'."
#endif

#include "stdint.h"

/* =============================================================================
 *  MACROS
 * =============================================================================

 * =============================================================================
 *  TYPES
 * =============================================================================

 * =============================================================================
 * SDMMC_T
 * -----------------------------------------------------------------------------
 *
 * =============================================================================
 */
#define REG_SDMMC1_BASE            (0x40006000)

struct hwp_sdmmc
{
        uint32_t        apbi_ctrl_sdmmc;              /* 0x00000000 */
        uint32_t        reserved_00000004;            /* 0x00000004 */
        uint32_t        apbi_fifo_txrx;               /* 0x00000008 */
        uint32_t        reserved_0000000c[509];       /* 0x0000000c */
        uint32_t        sdmmc_config;                 /* 0x00000800 */
        uint32_t        sdmmc_status;                 /* 0x00000804 */
        uint32_t        sdmmc_cmd_index;              /* 0x00000808 */
        uint32_t        sdmmc_cmd_arg;                /* 0x0000080c */
        uint32_t        sdmmc_resp_index;             /* 0x00000810 */
        uint32_t        sdmmc_resp_arg3;              /* 0x00000814 */
        uint32_t        sdmmc_resp_arg2;              /* 0x00000818 */
        uint32_t        sdmmc_resp_arg1;              /* 0x0000081c */
        uint32_t        sdmmc_resp_arg0;              /* 0x00000820 */
        uint32_t        sdmmc_data_width;             /* 0x00000824 */
        uint32_t        sdmmc_block_size;             /* 0x00000828 */
        uint32_t        sdmmc_block_cnt;              /* 0x0000082c */
        uint32_t        sdmmc_int_status;             /* 0x00000830 */
        uint32_t        sdmmc_int_mask;               /* 0x00000834 */
        uint32_t        sdmmc_int_clear;              /* 0x00000838 */
        uint32_t        sdmmc_trans_speed;            /* 0x0000083c */
        uint32_t        sdmmc_mclk_adjust;            /* 0x00000840 */
        uint32_t        sdmmc_status2;                /* 0x00000844 */
        uint32_t        sdmmc_rsp_waitcnt;            /* 0x00000848 */
        uint32_t        sdmmc_clk_data_sample;        /* 0x0000084c */
        uint32_t        sdmmc_anaphy_config;          /* 0x00000850 */
};

#define hwp_sdmmc1                 ((struct hwp_sdmmc*) KSEG1(REG_SDMMC1_BASE))

/* apbi_ctrl_sdmmc */
#define SDMMC_L_ENDIAN(n)          (((n)&7)<<0)
#define SDMMC_SOFT_RST_L           (1<<3)

/* APBI_FIFO_TxRx */
#define SDMMC_FIFO_TXRX(n)         (((n)&0xFFFFFFFF)<<0)

/* SDDMA_CONTROLER */
#define SDMMC_SDDMA_START          (1<<0)
#define SDMMC_SDDMA_DISALBE        (1<<1)
#define SDMMC_SDDMA_AUTODISABLE    (1<<4)
#define SDMMC_SDDMA_DIRECTION      (1<<8)
#define SDMMC_FLUSH                (1<<16)
#define SDMMC_MODE_SEL             (1<<17)
#define SDMMC_ADDR_CNT(n)          (((n)&7)<<18)

/* SDDMA_STATUS */
#define SDMMC_SDDMA_ENABLE         (1<<0)
#define SDMMC_SDDMA_FIFO_EMPTY     (1<<1)
#define SDMMC_BURST_FIFO_RCOUNT(n) (((n)&0x3F)<<2)
#define SDMMC_SG_ERROR             (1<<8)
#define SDMMC_SG_NUM(n)            (((n)&0xFF)<<12)
#define SDMMC_AHB_RQ_STATE(n)      (((n)&3)<<20)
#define SDMMC_A9B_RQ_STATE(n)      (((n)&3)<<22)

/* START_ADDR1 */
#define SDMMC_START_ADDR1(n)       (((n)&0xFFFFFFFF)<<0)

/* TC1 */
#define SDMMC_TC1(n)               (((n)&0xFFFFF)<<0)

/* LIST_ADDR */
#define SDMMC_LIST_ADDR(n)         (((n)&0xFFFFFFFF)<<0)

/* SYSTEM_PARAMETER_LOW */
#define SDMMC_SYSTEM_PARAMETER_LOW(n) (((n)&0xFFFFFFFF)<<0)

/* SYSTEM_PARAMETER_HIGH */
#define SDMMC_SYSTEM_PARAMETER_HIGH(n) (((n)&0xFFFFFFFF)<<0)

/* SYSTEM_PARAMETER_LOW_PRE */
#define SDMMC_SYSTEM_PARAMETER_LOW_PRE(n) (((n)&0xFFFFFFFF)<<0)

/* SYSTEM_PARAMETER_HIGH_PRE */
#define SDMMC_SYSTEM_PARAMETER_HIGH_PRE(n) (((n)&0xFFFFFFFF)<<0)

/* START_ADDR2 */
#define SDMMC_START_ADDR2(n)       (((n)&0xFFFFFFFF)<<0)

/* TC2 */
#define SDMMC_TC2(n)               (((n)&0xFFFFF)<<0)

/* START_ADDR3 */
#define SDMMC_START_ADDR3(n)       (((n)&0xFFFFFFFF)<<0)

/* TC3 */
#define SDMMC_TC3(n)               (((n)&0xFFFFF)<<0)

/* START_ADDR4 */
#define SDMMC_START_ADDR4(n)       (((n)&0xFFFFFFFF)<<0)

/* TC4 */
#define SDMMC_TC4(n)               (((n)&0xFFFFF)<<0)

/* START_ADDR5 */
#define SDMMC_START_ADDR5(n)       (((n)&0xFFFFFFFF)<<0)

/* TC5 */
#define SDMMC_TC6(n)               (((n)&0xFFFFF)<<0)

/* START_ADDR6 */
#define SDMMC_START_ADDR6(n)       (((n)&0xFFFFFFFF)<<0)

/* TC6 */
/* #define SDMMC_TC6(n)             (((n)&0xFFFFF)<<0) */

/* START_ADDR7 */
#define SDMMC_START_ADDR7(n)       (((n)&0xFFFFFFFF)<<0)

/* TC7 */
#define SDMMC_TC7(n)               (((n)&0xFFFFF)<<0)

/* START_ADDR8 */
#define SDMMC_START_ADDR8(n)       (((n)&0xFFFFFFFF)<<0)

/* TC8 */
#define SDMMC_TC8(n)               (((n)&0xFFFFF)<<0)

/* SDMMC_CONFIG */
#define SDMMC_SDMMC_SENDCMD        (1<<0)
#define SDMMC_SDMMC_SUSPEND        (1<<1)
#define SDMMC_RSP_EN               (1<<4)
#define SDMMC_RSP_SEL(n)           (((n)&3)<<5)
#define SDMMC_RSP_SEL_R2           (2<<5)
#define SDMMC_RSP_SEL_R3           (1<<5)
#define SDMMC_RSP_SEL_OTHER        (0<<5)
#define SDMMC_RD_WT_EN             (1<<8)
#define SDMMC_RD_WT_SEL            (1<<9)
#define SDMMC_RD_WT_SEL_READ       (0<<9)
#define SDMMC_RD_WT_SEL_WRITE      (1<<9)
#define SDMMC_S_M_SEL              (1<<10)
#define SDMMC_S_M_SEL_SIMPLE       (0<<10)
#define SDMMC_S_M_SEL_MULTIPLE     (1<<10)
#define SDMMC_NCRC(n)              (((n)&15)<<12)
#define SDMMC_AUTO_CMD_EN          (1<<16)
#define SDMMC_DDR_EN               (1<<17)
#define SDMMC_SAMPLE_SDGE_SEL      (1<<19)
#define SDMMC_FORCE_RESET          (1<<20)
#define SDMMC_FIFO_1BLOCK_MARGIN   (1<<24)

/* SDMMC_STATUS */
#define SDMMC_NOT_SDMMC_OVER       (1<<0)
#define SDMMC_BUSY                 (1<<1)
#define SDMMC_DL_BUSY              (1<<2)
#define SDMMC_SUSPEND              (1<<3)
#define SDMMC_RSP_ERROR            (1<<8)
#define SDMMC_NO_RSP_ERROR         (1<<9)
#define SDMMC_CRC_STATUS(n)        (((n)&7)<<12)
#define SDMMC_DATA_ERROR(n)        (((n)&0xFF)<<16)
#define SDMMC_DAT3_VAL             (1<<24)
#define SDMMC_DAT1_VAL             (1<<25)
#define SDMMC_DAT0_VAL             (1<<26)
#define SDMMC_FIFO_NOT_EMPTY       (1<<30)
/* #define SDMMC_FIFO_NOT_EMPTY     (1<<31) */

/* SDMMC_CMD_INDEX */
#define SDMMC_COMMAND(n)           (((n)&0x3F)<<0)

/* SDMMC_CMD_ARG */
#define SDMMC_ARGUMENT(n)          (((n)&0xFFFFFFFF)<<0)

/* SDMMC_RESP_INDEX */
#define SDMMC_RESPONSE(n)          (((n)&0x3F)<<0)

/* SDMMC_RESP_ARG3 */
#define SDMMC_ARGUMENT3(n)         (((n)&0xFFFFFFFF)<<0)

/* SDMMC_RESP_ARG2 */
#define SDMMC_ARGUMENT2(n)         (((n)&0xFFFFFFFF)<<0)

/* SDMMC_RESP_ARG1 */
#define SDMMC_ARGUMENT1(n)         (((n)&0xFFFFFFFF)<<0)

/* SDMMC_RESP_ARG0 */
#define SDMMC_ARGUMENT0(n)         (((n)&0xFFFFFFFF)<<0)
#define SDMMC_RESERVE              (1<<31)

/* SDMMC_DATA_WIDTH */
#define SDMMC_SDMMC_DATA_WIDTH(n)  (((n)&15)<<0)

/* SDMMC_BLOCK_SIZE */
#define SDMMC_SDMMC_BLOCK_SIZE(n)  (((n)&15)<<0)

/* SDMMC_BLOCK_CNT */
#define SDMMC_SDMMC_BLOCK_CNT(n)   (((n)&0xFFFF)<<0)

/* SDMMC_INT_STATUS */
#define SDMMC_NO_RSP_INT           (1<<0)
#define SDMMC_RSP_ERR_INT          (1<<1)
#define SDMMC_RD_ERR_INT           (1<<2)
#define SDMMC_WR_ERR_INT           (1<<3)
#define SDMMC_DAT_OVER_INT         (1<<4)
#define SDMMC_TXDMA_DONE_INT       (1<<5)
#define SDMMC_RXDMA_DONE_INT       (1<<6)
#define SDMMC_DAT_0__EER_INT       (1<<7)
#define SDMMC_DAT_8__EER_INT       (1<<8)
#define SDMMC_RSP_NORMAL_INT       (1<<9)
#define SDMMC_SG_ERROR_INT         (1<<10)
#define SDMMC_NO_RSP_SC            (1<<11)
#define SDMMC_RSP_ERR_SC           (1<<12)
#define SDMMC_RD_ERR_SC            (1<<13)
#define SDMMC_WR_ERR_SC            (1<<14)
#define SDMMC_DAT_OVER_SC          (1<<15)
#define SDMMC_TXDMA_DONE_SC        (1<<16)
#define SDMMC_RXDMA_DONE_SC        (1<<17)
#define SDMMC_DAT_0__EER_SC        (1<<18)
#define SDMMC_DAT_1__EER_SC        (1<<19)
#define SDMMC_RSP_NORMAL_SC        (1<<20)
#define SDMMC_SG_ERROR_SC          (1<<21)

/* SDMMC_INT_MASK */
#define SDMMC_NO_RSP_MK            (1<<0)
#define SDMMC_RSP_ERR_MK           (1<<1)
#define SDMMC_RD_ERR_MK            (1<<2)
#define SDMMC_WR_ERR_MK            (1<<3)
#define SDMMC_DAT_OVER_MK          (1<<4)
#define SDMMC_TXDMA_DONE_MK        (1<<5)
#define SDMMC_RXDMA_DONE_MK        (1<<6)
#define SDMMC_DATA_0__ERR_MK       (1<<7)
#define SDMMC_DATA_1__ERR_MK       (1<<8)
#define SDMMC_RSP_NORMAL_MK        (1<<9)
#define SDMMC_SG_ERROR_MK          (1<<10)

/* SDMMC_INT_CLEAR */
#define SDMMC_NO_RSP_CL            (1<<0)
#define SDMMC_RSP_ERR_CL           (1<<1)
#define SDMMC_RD_ERR_CL            (1<<2)
#define SDMMC_WR_ERR_CL            (1<<3)
#define SDMMC_DAT_OVER_CL          (1<<4)
#define SDMMC_TXDMA_DONE_CL        (1<<5)
#define SDMMC_RXDMA_DONE_CL        (1<<6)
#define SDMMC_DATA_0__ERR_CL       (1<<7)
#define SDMMC_DATA_1__ERR_CL       (1<<8)
#define SDMMC_RSP_NORMAL_CL        (1<<9)
#define SDMMC_SG_ERROR_CL          (1<<10)

/* SDMMC_TRANS_SPEED */
#define SDMMC_SDMMC_TRANS_SPEED(n) (((n)&0x00FF)<<0)

/* SDMMC_MCLK_ADJUST */
#define SDMMC_SDMMC_MCLK_ADJUST(n) (((n)&15)<<0)
#define SDMMC_CLK_INV              (1<<4)
#define SDMMC_MCLK_INV             (1<<5)
#define SDMMC_CLK_BYPASS_EN        (1<<6)

/* SDMMC_STATUS2 */
#define SDMMC_DATA_STATUS_MON(n)   (((n)&31)<<0)
#define SDMMC_RX_FIFO_LEVEL_MON(n) (((n)&0x1FF)<<6)
#define SDMMC_TX_FIFO_LEVEL_MON(n) (((n)&0x1FF)<<15)
#define SDMMC_CMD_STATUS_MON(n)    (((n)&7)<<24)
#define SDMMC_RESP_STATUS_MON(n)   (((n)&31)<<27)

/* SDMMC_RSP_WAITCNT */
#define SDMMC_SDMMC_RSP_WAITCNT(n) (((n)&0xFFFF)<<0)

/* SDMMC_CLK_DATA_SAMPLE */
#define SDMMC_SDMMC_CLK_OUT_SEL(n) (((n)&0x3F)<<0)
#define SDMMC_SDMMC_CLK_OUT_SEL_EN (1<<8)
#define SDMMC_SDMMC_SAMPLE_CLK_SEL(n) (((n)&0x3F)<<16)
#define SDMMC_SDMMC_SAMPLE_CLK_SEL_EN (1<<24)
#define SDMMC_SDMMC_CLOSE_CLK_EN   (1<<28)

/* SDMMC_ANAPHY_CONFIG */
#define SDMMC_SDMMC_CLK_WRITE_ENABLE (1<<0)
#define SDMMC_SDMMC_IO3V3_ENABLE   (1<<11)

#endif
