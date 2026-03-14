/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32a455_qspi.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_QSPI_H__
#define __N32A455_QSPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"
#include <stdbool.h>
/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup QSPI
 * @brief QSPI driver modules
 * @{
 */
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
    STANDARD_SPI_FORMAT_SEL = 0,
    DUAL_SPI_FORMAT_SEL,
    QUAD_SPI_FORMAT_SEL,
    XIP_SPI_FORMAT_SEL
} QSPI_FORMAT_SEL;

typedef enum
{
    TX_AND_RX = 0,
    TX_ONLY,
    RX_ONLY
} QSPI_DATA_DIR;

typedef enum
{
    QSPI_NSS_PORTA_SEL,
    QSPI_NSS_PORTC_SEL,
    QSPI_NSS_PORTF_SEL
} QSPI_NSS_PORT_SEL;

typedef enum
{
    QSPI_NULL = 0,
    QSPI_SUCCESS,
} QSPI_STATUS;
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    /*QSPI_CTRL0*/
    uint32_t DFS;
    uint32_t FRF;
    uint32_t SCPH;
    uint32_t SCPOL;
    uint32_t TMOD;
    uint32_t SSTE;
    uint32_t CFS;
    uint32_t SPI_FRF;
    
    /*QSPI_CTRL1*/
    uint32_t NDF;

    /*QSPI_MW_CTRL*/
    uint32_t MWMOD;
    uint32_t MC_DIR;
    uint32_t MHS_EN;

    /*QSPI_BAUD*/
    uint32_t CLK_DIV;

    /*QSPI_TXFT*/
    uint32_t TXFT;

    /*QSPI_RXFT*/
    uint32_t RXFT;

    /*QSPI_TXFN*/
    uint32_t TXFN;

    /*QSPI_RXFN*/
    uint32_t RXFN;

    /*QSPI_RS_DELAY*/
    uint32_t SDCN;
    uint32_t SES;

    /*QSPI_ENH_CTRL0*/
    uint32_t ENHANCED_TRANS_TYPE;
    uint32_t ENHANCED_ADDR_LEN;
    uint32_t ENHANCED_MD_BIT_EN;
    uint32_t ENHANCED_INST_L;
    uint32_t ENHANCED_WAIT_CYCLES;
    uint32_t ENHANCED_SPI_DDR_EN;
    uint32_t ENHANCED_INST_DDR_EN;
    uint32_t ENHANCED_XIP_DFS_HC;
    uint32_t ENHANCED_XIP_INST_EN;
    uint32_t ENHANCED_XIP_CT_EN;
    uint32_t ENHANCED_XIP_MBL;
    uint32_t ENHANCED_CLK_STRETCH_EN;
    
    /*QSPI_DDR_TXDE*/
    uint32_t TXDE;

    /*QSPI_XIP_MODE*/
    uint32_t XIP_MD_BITS;

    /*QSPI_XIP_INCR_TOC*/
    uint32_t ITOC;

    /*QSPI_XIP_WRAP_TOC*/
    uint32_t WTOC;
    
    /*QSPI_XIP_CTRL*/
    uint32_t XIP_FRF;
    uint32_t XIP_TRANS_TYPE;
    uint32_t XIP_ADDR_LEN;
    uint32_t XIP_INST_L;
    uint32_t XIP_MD_BITS_EN;
    uint32_t XIP_WAIT_CYCLES;
    uint32_t XIP_DFS_HC;
    uint32_t XIP_DDR_EN;
    uint32_t XIP_INST_DDR_EN;
    uint32_t XIP_INST_EN;
    uint32_t XIP_CT_EN;
    uint32_t XIP_MBL;

    /*QSPI_XIP_TOUT*/
    uint32_t XTOUT;

} QSPI_InitType;
////////////////////////////////////////////////////////////////////////////////////////////////////
#define QSPI_TIME_OUT_CNT           200

#define IS_QSPI_SPI_FRF(SPI_FRF)                                                                                            \
    (((SPI_FRF) == QSPI_CTRL0_SPI_FRF_STANDARD_FORMAT) || ((SPI_FRF) == QSPI_CTRL0_SPI_FRF_DUAL_FORMAT) || ((SPI_FRF) == QSPI_CTRL0_SPI_FRF_QUAD_FORMAT))  

#define IS_QSPI_CFS(CFS) ((((CFS) >= QSPI_CTRL0_CFS_2_BIT) && ((CFS) <= QSPI_CTRL0_CFS_16_BIT)) || ((CFS) == QSPI_CTRL0_CFS_1_BIT))

#define IS_QSPI_SSTE(SSTE) (((SSTE) == QSPI_CTRL0_SSTE_EN) || ((SSTE) == 0))

#define IS_QSPI_TMOD(TMOD)                                                                                            \
    (((TMOD) == QSPI_CTRL0_TMOD_TX_AND_RX) || ((TMOD) == QSPI_CTRL0_TMOD_TX_ONLY) || ((TMOD) == QSPI_CTRL0_TMOD_RX_ONLY) || ((TMOD) == QSPI_CTRL0_TMOD_EEPROM_READ))  

#define IS_QSPI_SCPOL(SCPOL) (((SCPOL) == QSPI_CTRL0_SCPOL_LOW) || ((SCPOL) == QSPI_CTRL0_SCPOL_HIGH))

#define IS_QSPI_SCPH(SCPH) (((SCPH) == QSPI_CTRL0_SCPH_FIRST_EDGE) || ((SCPH) == QSPI_CTRL0_SCPH_SECOND_EDGE))

#define IS_QSPI_FRF(FRF) (((FRF) == QSPI_CTRL0_FRF_MOTOROLA) || ((FRF) == QSPI_CTRL0_FRF_TI) || ((FRF) == QSPI_CTRL0_FRF_MICROWIRE))

#define IS_QSPI_DFS(DFS) (((DFS) >= QSPI_CTRL0_DFS_4_BIT) && ((DFS) <= QSPI_CTRL0_DFS_32_BIT))


#define IS_QSPI_NDF(NDF) (((NDF) <= 0xFFFF))

#define IS_QSPI_MWMOD(MWMOD) (((MWMOD) == QSPI_MW_CTRL_MWMOD_UNSEQUENTIAL) || ((MWMOD) == QSPI_MW_CTRL_MWMOD_SEQUENTIAL))

#define IS_QSPI_MC_DIR(MC_DIR) (((MC_DIR) == QSPI_MW_CTRL_MC_DIR_RX) || ((MC_DIR) == QSPI_MW_CTRL_MC_DIR_TX))

#define IS_QSPI_MHS_EN(MHS_EN) (((MHS_EN) == QSPI_MW_CTRL_MHS_EN) || ((MHS_EN) == 0))

#define IS_QSPI_CLK_DIV(CLK_DIV) (((CLK_DIV) <= 0xFFFF))

#define IS_QSPI_TXFT(TXFT) (((TXFT) <= 0x1FFFFF))

#define IS_QSPI_RXFT(RXFT) (((RXFT) <= 0x1F))

#define IS_QSPI_TXFN(TXFN) (((TXFN) <= 0x3F))

#define IS_QSPI_RXFN(RXFN) (((RXFN) <= 0x3F))

#define IS_QSPI_DMA_CTRL(DMA_CTRL) (((DMA_CTRL) == QSPI_DMA_CTRL_TX_DMA_EN) || ((DMA_CTRL) == QSPI_DMA_CTRL_RX_DMA_EN))

#define IS_QSPI_DMATDL_CTRL(DMATDL_CTRL) (((DMATDL_CTRL) <= 0x3F))

#define IS_QSPI_DMARDL_CTRL(DMARDL_CTRL) (((DMARDL_CTRL) <= 0x3F))

#define IS_QSPI_SES(SES) (((SES) == QSPI_RS_DELAY_SES_RISING_EDGE) || ((SES) == QSPI_RS_DELAY_SES_FALLING_EDGE))

#define IS_QSPI_SDCN(SDCN) (((SDCN) <= 0xFF))

#define IS_QSPI_ENH_CLK_STRETCH_EN(ENH_CLK_STRETCH_EN) (((ENH_CLK_STRETCH_EN) == QSPI_ENH_CTRL0_CLK_STRETCH_EN) || ((ENH_CLK_STRETCH_EN) == 0))

#define IS_QSPI_ENH_XIP_MBL(ENH_XIP_MBL)                                                                                            \
    (((ENH_XIP_MBL) == QSPI_ENH_CTRL0_XIP_MBL_2_BIT) || ((ENH_XIP_MBL) == QSPI_ENH_CTRL0_XIP_MBL_4_BIT) ||      \
    ((ENH_XIP_MBL) == QSPI_ENH_CTRL0_XIP_MBL_8_BIT)  || ((ENH_XIP_MBL) == QSPI_ENH_CTRL0_XIP_MBL_16_BIT))

#define IS_QSPI_ENH_XIP_CT_EN(ENH_XIP_CT_EN) (((ENH_XIP_CT_EN) == QSPI_ENH_CTRL0_XIP_CT_EN) || ((ENH_XIP_CT_EN) == 0))

#define IS_QSPI_ENH_XIP_INST_EN(ENH_XIP_INST_EN) (((ENH_XIP_INST_EN) == QSPI_ENH_CTRL0_XIP_INST_EN) || ((ENH_XIP_INST_EN) == 0))

#define IS_QSPI_ENH_XIP_DFS_HC(ENH_XIP_DFS_HC) (((ENH_XIP_DFS_HC) == QSPI_ENH_CTRL0_XIP_DFS_HC) || ((ENH_XIP_DFS_HC) == 0))

#define IS_QSPI_ENH_INST_DDR_EN(ENH_INST_DDR_EN) (((ENH_INST_DDR_EN) == QSPI_ENH_CTRL0_INST_DDR_EN) || ((ENH_INST_DDR_EN) == 0))

#define IS_QSPI_ENH_SPI_DDR_EN(ENH_SPI_DDR_EN) (((ENH_SPI_DDR_EN) == QSPI_ENH_CTRL0_SPI_DDR_EN) || ((ENH_SPI_DDR_EN) == 0))

#define IS_QSPI_ENH_WAIT_CYCLES(ENH_WAIT_CYCLES) ((((ENH_WAIT_CYCLES) >= QSPI_ENH_CTRL0_WAIT_1CYCLES) && ((ENH_WAIT_CYCLES) <= QSPI_ENH_CTRL0_WAIT_31CYCLES)) || \
                                                    ((ENH_WAIT_CYCLES) == 0))

#define IS_QSPI_ENH_INST_L(ENH_INST_L)                                                                                            \
    (((ENH_INST_L) == QSPI_ENH_CTRL0_INST_L_0_LINE) || ((ENH_INST_L) == QSPI_ENH_CTRL0_INST_L_4_LINE) ||                                                    \
    ((ENH_INST_L) == QSPI_ENH_CTRL0_INST_L_8_LINE)  || ((ENH_INST_L) == QSPI_ENH_CTRL0_INST_L_16_LINE))

#define IS_QSPI_ENH_MD_BIT_EN(ENH_MD_BIT_EN) (((ENH_MD_BIT_EN) == QSPI_ENH_CTRL0_MD_BIT_EN) || ((ENH_MD_BIT_EN) == 0))

#define IS_QSPI_ENH_ADDR_LEN(ENH_ADDR_LEN) ((((ENH_ADDR_LEN) >= QSPI_ENH_CTRL0_ADDR_LEN_4_BIT) && ((ENH_ADDR_LEN) <= QSPI_ENH_CTRL0_ADDR_LEN_60_BIT)) || \
                                            ((ENH_ADDR_LEN) == 0))

#define IS_QSPI_ENH_TRANS_TYPE(ENH_TRANS_TYPE) (((ENH_TRANS_TYPE) == QSPI_ENH_CTRL0_TRANS_TYPE_STANDARD) ||   \
                                        ((ENH_TRANS_TYPE) == QSPI_ENH_CTRL0_TRANS_TYPE_ADDRESS_BY_FRF)   ||   \
                                        ((ENH_TRANS_TYPE) == QSPI_ENH_CTRL0_TRANS_TYPE_ALL_BY_FRF))
    

#define IS_QSPI_DDR_TXDE(DDR_TXDE) (((DDR_TXDE) <= 0xFF))

#define IS_QSPI_XIP_MODE(XIP_MODE) (((XIP_MODE) <= 0xFFFF))

#define IS_QSPI_XIP_INCR_TOC(XIP_INCR_TOC) (((XIP_INCR_TOC) <= 0xFFFF))

#define IS_QSPI_XIP_WRAP_TOC(XIP_WRAP_TOC) (((XIP_WRAP_TOC) <= 0xFFFF))

#define IS_QSPI_XIP_TOUT(XIP_TOUT) (((XIP_TOUT) <= 0xFF))

#define IS_QSPI_XIP_MBL(XIP_MBL)                                                                                            \
    (((XIP_MBL) == QSPI_XIP_CTRL_XIP_MBL_LEN_2_BIT) || ((XIP_MBL) == QSPI_XIP_CTRL_XIP_MBL_LEN_4_BIT) ||        \
    ((XIP_MBL) == QSPI_XIP_CTRL_XIP_MBL_LEN_8_BIT)  || ((XIP_MBL) == QSPI_XIP_CTRL_XIP_MBL_LEN_16_BIT))
    
#define IS_QSPI_XIP_CT_EN(XIP_CT_EN) (((XIP_CT_EN) == QSPI_XIP_CTRL_XIP_CT_EN) || ((XIP_CT_EN) == 0))

#define IS_QSPI_XIP_INST_EN(XIP_INST_EN) (((XIP_INST_EN) == QSPI_XIP_CTRL_XIP_INST_EN) || ((XIP_INST_EN) == 0))

#define IS_QSPI_INST_DDR_EN(INST_DDR_EN) (((INST_DDR_EN) == QSPI_XIP_CTRL_XIP_INST_EN) || ((INST_DDR_EN) == 0))

#define IS_QSPI_DDR_EN(DDR_EN) (((DDR_EN) == QSPI_XIP_CTRL_DDR_EN) || ((DDR_EN) == 0))

#define IS_QSPI_XIP_DFS_HC(XIP_DFS_HC) (((XIP_DFS_HC) == QSPI_XIP_CTRL_DFS_HC) || ((XIP_DFS_HC) == 0))

#define IS_QSPI_XIP_WAIT_CYCLES(XIP_WAIT_CYCLES) ((((XIP_WAIT_CYCLES) >= QSPI_XIP_CTRL_WAIT_1CYCLES) && ((XIP_WAIT_CYCLES) <= QSPI_XIP_CTRL_WAIT_31CYCLES)) || \
                                                    ((XIP_WAIT_CYCLES) == 0))

#define IS_QSPI_XIP_MD_BIT_EN(XIP_MD_BIT_EN) (((XIP_MD_BIT_EN) == QSPI_XIP_CTRL_MD_BIT_EN) || ((XIP_MD_BIT_EN) == 0))

#define IS_QSPI_XIP_INST_L(XIP_INST_L)                                                                                            \
    (((XIP_INST_L) == QSPI_XIP_CTRL_INST_L_0_LINE) || ((XIP_INST_L) == QSPI_XIP_CTRL_INST_L_4_LINE) ||      \
    ((XIP_INST_L) == QSPI_XIP_CTRL_INST_L_8_LINE)  || ((XIP_INST_L) == QSPI_XIP_CTRL_INST_L_16_LINE))

#define IS_QSPI_XIP_ADDR_LEN(XIP_ADDR_LEN) ((((XIP_ADDR_LEN) >= QSPI_XIP_CTRL_ADDR_4BIT) && ((XIP_ADDR_LEN) <= QSPI_XIP_CTRL_ADDR_60BIT)) || \
                                            ((XIP_ADDR_LEN) == 0))

#define IS_QSPI_XIP_TRANS_TYPE(XIP_TRANS_TYPE) (((XIP_TRANS_TYPE) == QSPI_XIP_CTRL_TRANS_TYPE_STANDARD_SPI) ||   \
                                        ((XIP_TRANS_TYPE) == QSPI_XIP_CTRL_TRANS_TYPE_ADDRESS_BY_XIP_FRF)  ||   \
                                        ((XIP_TRANS_TYPE) == QSPI_XIP_CTRL_TRANS_TYPE_INSTRUCT_BY_XIP_FRF))

#define IS_QSPI_XIP_FRF(XIP_FRF) (((XIP_FRF) == QSPI_XIP_CTRL_FRF_2_LINE) || ((XIP_FRF) == QSPI_XIP_CTRL_FRF_4_LINE) || ((XIP_FRF) == 0))
    





////////////////////////////////////////////////////////////////////////////////////////////////////
void QSPI_Cmd(bool cmd);
void QSPI_XIP_Cmd(bool cmd);
void QSPI_DeInit(void);
void QspiInitConfig(QSPI_InitType* QSPI_InitStruct);
void QSPI_GPIO(QSPI_NSS_PORT_SEL qspi_nss_port_sel, bool IO1_Input, bool IO3_Output);
void QSPI_DMA_CTRL_Config(uint8_t TxRx,uint8_t TxDataLevel,uint8_t RxDataLevel);
uint16_t QSPI_GetITStatus(uint16_t FLAG);
void QSPI_ClearITFLAG(uint16_t FLAG);
void QSPI_XIP_ClearITFLAG(uint16_t FLAG);
bool GetQspiBusyStatus(void);
bool GetQspiTxDataBusyStatus(void);
bool GetQspiTxDataEmptyStatus(void);
bool GetQspiRxHaveDataStatus(void);
bool GetQspiRxDataFullStatus(void);
bool GetQspiTransmitErrorStatus(void);
bool GetQspiDataConflictErrorStatus(void);
void QspiSendWord(uint32_t SendData);
uint32_t QspiReadWord(void);
uint32_t QspiGetDataPointer(void);
uint32_t QspiReadRxFifoNum(void);
void ClrFifo(void);
uint32_t GetFifoData(uint32_t* pData, uint32_t Len);
void QspiSendAndGetWords(uint32_t* pSrcData, uint32_t* pDstData, uint32_t cnt);
uint32_t QspiSendWordAndGetWords(uint32_t WrData, uint32_t* pRdData, uint8_t LastRd);


#ifdef __cplusplus
}
#endif

#endif /*__N32A455_QSPI_H__ */

/**
 * @}
 */

/**
 * @}
 */
