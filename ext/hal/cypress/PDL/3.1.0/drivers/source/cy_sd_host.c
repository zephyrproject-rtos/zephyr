/*******************************************************************************
* \file cy_sd_host.c
* \version 1.0
*
* \brief
*  This file provides the driver code to the API for the SD Host Controller
*  driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/
#include "cy_sd_host.h"
#include <string.h>
#include <stdlib.h>

#ifdef CY_IP_MXSDHC

#if defined(__cplusplus)
extern "C" {
#endif

__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_eMMC_InitCard(SDHC_Type *base,
                                             cy_stc_sd_host_sd_card_config_t *config,
                                             cy_stc_sd_host_context_t *context);

static cy_en_sd_host_status_t SdcmdRxData(SDHC_Type *base, cy_stc_sd_host_data_config_t *pcmd);

static cy_en_sd_host_status_t Cy_SD_Host_OpsGoIdle(SDHC_Type *base);
static cy_en_sd_host_status_t Cy_SD_Host_OpsVoltageSwitch(SDHC_Type *base,
                                                          cy_stc_sd_host_context_t const *context);
static cy_en_sd_host_status_t Cy_SD_Host_OpsSwitchFunc(SDHC_Type *base,
                                                       uint32_t cmdArgument);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsSetBlockCount(SDHC_Type *base,
                                                                   bool reliableWrite,
                                                                   uint32_t blockNum);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsProgramCsd(SDHC_Type *base,
                                                                uint32_t csd,
                                                                cy_stc_sd_host_context_t *context);
static cy_en_sd_host_status_t Cy_SD_Host_OpsSelectCard(SDHC_Type *base,
                                                      cy_stc_sd_host_context_t const *context);
static cy_en_sd_host_status_t Cy_SD_Host_OpsSendIoRwDirectCmd(SDHC_Type *base,
                                                            uint32_t rwFlag,
                                                            uint32_t functionNumber,
                                                            uint32_t rawFlag,
                                                            uint32_t registerAaddress,
                                                            uint32_t data);
static cy_en_sd_host_status_t Cy_SD_Host_OpsSendAppCmd(SDHC_Type *base,
                                                       cy_stc_sd_host_context_t const *context);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsSendIfCond(SDHC_Type *base,
                                                       uint32_t cmdArgument,
                                                       bool noResponse);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsSetSdBusWidth(SDHC_Type *base,
                                                        uint32_t cmdArgument,
                                                        cy_stc_sd_host_context_t const *context);
static cy_en_sd_host_status_t Cy_SD_Host_OpsSdioSendOpCond(SDHC_Type *base,
                                                           uint32_t *ocr,
                                                           uint32_t cmdArgument);
static cy_en_sd_host_status_t Cy_SD_Host_OpsSdSendOpCond(SDHC_Type *base,
                                                        uint32_t *ocr,
                                                        uint32_t cmdArgument,
                                                        cy_stc_sd_host_context_t const *context);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_MmcOpsSendOpCond(SDHC_Type *base,
                                                        uint32_t *ocr,
                                                        uint32_t cmdArgument,
                                                        cy_stc_sd_host_context_t *context);

__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollBufferReadReady(SDHC_Type *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollBufferWriteReady(SDHC_Type *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollCmdComplete(SDHC_Type *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollTransferComplete(SDHC_Type *base);

static void Cy_SD_Host_ErrorReset(SDHC_Type *base);
static void Cy_SD_Host_NormalReset(SDHC_Type *base);
__STATIC_INLINE bool Cy_SD_Host_VoltageCheck(SDHC_Type *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_IoOcr(SDHC_Type *base,
                                      bool lowVoltageSignaling,
                                      uint32_t *s18A,
                                      uint32_t *sdio,
                                      bool *mp,                                   
                                      uint32_t *ocr);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_SdOcr(SDHC_Type *base,
                                      bool lowVoltageSignaling,
                                      uint32_t *s18A,
                                      bool *mp,
                                      bool f8,
                                      uint32_t *ocr,
                                      cy_stc_sd_host_context_t *context);

__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollCmdLineFree(SDHC_Type const *base);
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollDataLineFree(SDHC_Type const *base);

/* The High level section */

/*******************************************************************************
* Function Name: Cy_SD_Host_eMMC_InitCard
****************************************************************************//**
*
*  Initializes eMMC card.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *config
*     The pointer to the SD card configuration structure.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_eMMC_InitCard(SDHC_Type *base,
                                                       cy_stc_sd_host_sd_card_config_t *config,
                                                       cy_stc_sd_host_context_t *context)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_SUCCESS;
    uint32_t retry;
    uint32_t ocr; /* The Operation Condition register. */
    uint32_t cid[CY_SD_HOST_CID_SIZE];  /* The Device Identification register. */
    uint32_t extCsd[CY_SD_HOST_EXTCSD_SIZE] = { 0u };
    cy_en_sd_host_bus_speed_mode_t speedMode = CY_SD_HOST_BUS_SPEED_EMMC_LEGACY;
    uint32_t genericCmd6Time;
    uint32_t csdReg[CY_SD_HOST_CSD_SIZE]; /* The Card-Specific Data register. */

    /* Reset Card (CMD0). */
    ret = Cy_SD_Host_OpsGoIdle(base); /* The Idle state. */
    
    /* Software reset for the CMD line */
    Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_DATALINE);
    Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_CMD_LINE);

    retry = CY_SD_HOST_RETRY_TIME;
    while (retry > 0u)
    {
        /* Get OCR (CMD1). */
        ret = Cy_SD_Host_MmcOpsSendOpCond(base,
                                          &ocr,
                                          CY_SD_HOST_EMMC_VOLTAGE_WINDOW,
                                          context);

        context->cardCapacity = CY_SD_HOST_EMMC_LESS_2G;

        /* Check if the eMMC capacity is greater than 2GB */
        if (CY_SD_HOST_OCR_CAPACITY_MASK == (ocr & CY_SD_HOST_OCR_CAPACITY_MASK))
        {
            context->cardCapacity = CY_SD_HOST_EMMC_GREATER_2G;
        }

        if (Cy_SD_Host_SUCCESS == ret)
        {
            break;
        }
        else
        {
            Cy_SD_Host_ErrorReset(base);

            Cy_SysLib_DelayUs(CY_SD_HOST_CMD1_TIMEOUT_MS); /* The CMD1 timeout. */
        }
        retry--;
    }

    /* The low voltage operation check. */
    if ((true == config->lowVoltageSignaling) &&
        (CY_SD_HOST_EMMC_DUAL_VOLTAGE == (ocr & CY_SD_HOST_EMMC_DUAL_VOLTAGE)))
    {
        /* Switch the bus to 1.8 V */
        Cy_SD_Host_ChangeIoVoltage(base, CY_SD_HOST_IO_VOLT_1_8V);

        /* Wait 10 ms */
        Cy_SysLib_Delay(CY_SD_HOST_1_8_REG_STABLE_TIME_MS);

        retry = CY_SD_HOST_RETRY_TIME;
        while (retry > 0u)
        {
            /* Get OCR (CMD1). */
            ret = Cy_SD_Host_MmcOpsSendOpCond(base,
                                              &ocr,
                                              CY_SD_HOST_EMMC_DUAL_VOLTAGE,
                                              context);

            Cy_SD_Host_ErrorReset(base);

            if (Cy_SD_Host_SUCCESS == ret)
            {
                break;
            }

            Cy_SysLib_DelayUs(CY_SD_HOST_CMD1_TIMEOUT_MS); /* The CMD1 timeout. */
            retry--;
        }
    }

    if (Cy_SD_Host_SUCCESS == ret) /* The ready state. */
    {
        /* Get CID (CMD2). */
        ret = Cy_SD_Host_GetCid(base, cid);
        
        Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

        if (Cy_SD_Host_SUCCESS == ret) /* The Identification state. */
        {
            /* Get RCA (CMD3) */
            context->RCA = Cy_SD_Host_GetRca(base);

            /* The stand-by state. */

            /* Get CSD (CMD9)  */
            ret = Cy_SD_Host_GetCsd(base, csdReg, context);
            
            if (Cy_SD_Host_SUCCESS == ret)
            {
                /* Select the card (CMD7). */
                ret = Cy_SD_Host_OpsSelectCard(base, context);

                if (Cy_SD_Host_SUCCESS == ret) /* The transfer state. */
                {
                    /* Get Ext CSD (CMD8).
                    * Capacity Type and Max Sector Num are in context
                    */
                    ret = Cy_SD_Host_GetExtCsd(base, extCsd, context);

                    /* Get GENERIC_CMD6_TIME [248] of the EXTCSD register */
                    genericCmd6Time = CY_SD_HOST_EMMC_CMD6_TIMEOUT_MULT * 
                                      (extCsd[CY_SD_HOST_EXTCSD_GENERIC_CMD6_TIME] & 0xFFuL);

                    if ((Cy_SD_Host_SUCCESS == ret) &&
                        (CY_SD_HOST_BUS_WIDTH_1_BIT != config->busWidth))
                    {
                            /* Set Bus Width (CMD6) */
                            ret = Cy_SD_Host_SetBusWidth(base,
                                                         config->busWidth,
                                                         context);

                            Cy_SysLib_Delay(genericCmd6Time); /* The CMD6 timeout. */
                    }

                    if (Cy_SD_Host_SUCCESS == ret)
                    {
                        /* Set Bus Speed Mode (CMD6). */
                        ret = Cy_SD_Host_SetBusSpeedMode(base,
                                                         speedMode,
                                                         context);
                        if (Cy_SD_Host_SUCCESS == ret)
                        {
                            (void)Cy_SD_Host_SdCardChangeClock(base, CY_SD_HOST_CLK_20M);
                        }
                        Cy_SysLib_Delay(CY_SD_HOST_CLK_RAMP_UP_TIME_MS);
                    }
                }
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_InitCard
****************************************************************************//**
*
*  Initializes a card if it is connected.
*  After this function has been called the card is in the transfer state.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *config
*     The pointer to the SD card configuration structure.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_InitCard(SDHC_Type *base,
                                           cy_stc_sd_host_sd_card_config_t *config,
                                           cy_stc_sd_host_context_t *context)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    bool f8 = false; /* The CMD8 flag. */
    uint32_t sdio = 0u; /* The IO flag. */
    bool mp = false; /* The MEM flag. */
    uint32_t s18A = 0u; /* The S18A flag. */
    uint32_t ocr; /* The Operation Condition register. */
    uint32_t cid[CY_SD_HOST_CID_SIZE]; /* The Device Identification register. */
    uint32_t csdReg[CY_SD_HOST_CSD_SIZE]; /* The Card-Specific Data register. */
    cy_en_sd_host_bus_speed_mode_t speedMode = CY_SD_HOST_BUS_SPEED_DEFAULT;

    if ((NULL != base) && (NULL != context) && (NULL != config))
    {
        CY_ASSERT_L3(CY_SD_HOST_IS_BUS_WIDTH_VALID(config->busWidth, context->cardType));   
        
        /* Wait until the card is stable and check if it is connected. */
        if ((true == Cy_SD_Host_IsCardConnected(base)) || 
            (CY_SD_HOST_EMMC == context->cardType))
        {
            Cy_SD_Host_EnableCardVoltage(base);

            Cy_SD_Host_ChangeIoVoltage(base, CY_SD_HOST_IO_VOLT_3_3V);
           
            /* Set the host bus width to 1 bit */
            (void)Cy_SD_Host_SetHostBusWidth(base, CY_SD_HOST_BUS_WIDTH_1_BIT);
      
            /* Change the host SD clock to 400 kHz */
            (void)Cy_SD_Host_SdCardChangeClock(base, CY_SD_HOST_CLK_400K);

            /* Wait to the stable voltage and the stable clock */
            Cy_SysLib_Delay(CY_SD_HOST_SUPPLY_RAMP_UP_TIME_MS);
            
            context->RCA = 0uL;
        
            if (CY_SD_HOST_EMMC != context->cardType)
            {
                /* Send CMD0 and CMD8 commands */
                f8 = Cy_SD_Host_VoltageCheck(base);

                /* Set SDHC can be supported. */
                if (f8)
                {
                    context->cardCapacity = CY_SD_HOST_SDHC;
                }
                
                /* Clear the insert event */
                Cy_SD_Host_NormalReset(base);

                /* Send CMD5 to get IO OCR */  
                ret = Cy_SD_Host_IoOcr(base,
                                       config->lowVoltageSignaling,
                                       &s18A,
                                       &sdio,
                                       &mp,                                
                                       &ocr);

                /* Check if CMD5 has no response or MP = 1.
                *  This means we have SD memory card or the Combo card.
                */
                if (mp || (0u == sdio))
                {
                    /* Send ACMD41 */
                    ret = Cy_SD_Host_SdOcr(base,
                                           config->lowVoltageSignaling,
                                           &s18A,
                                           &mp,
                                           f8,
                                           &ocr,
                                           context);
                }
                   
                /* Set the card type */
                if (Cy_SD_Host_ERROR_UNUSABLE_CARD != ret) /* The Ready state. */
                {
                    if (true == mp)
                    {
                        if (0u != sdio)
                        { 
                            context->cardType = CY_SD_HOST_COMBO;
                        }
                        else
                        {
                            context->cardType = CY_SD_HOST_SD; 
                        }
                    } 
                    else if (0u != sdio)
                    {
                        context->cardType = CY_SD_HOST_SDIO;
                    }
                    else
                    {
                        /* The unusable card */
                        ret = Cy_SD_Host_ERROR_UNUSABLE_CARD;

                        context->cardType = CY_SD_HOST_UNUSABLE;
                    }

                    if (Cy_SD_Host_ERROR_UNUSABLE_CARD != ret)
                    {
                        if ((1u == s18A) && (config->lowVoltageSignaling))
                        {
                            /* Voltage switch (CMD11). */
                            ret =  Cy_SD_Host_OpsVoltageSwitch(base, context);
                            
                            if (Cy_SD_Host_SUCCESS != ret)  /* Init again at 3.3 V */
                            {                           
                                Cy_SD_Host_ChangeIoVoltage(base, CY_SD_HOST_IO_VOLT_3_3V);

                                /* Wait to the stable voltage and the stable clock */
                                Cy_SysLib_Delay(CY_SD_HOST_SUPPLY_RAMP_UP_TIME_MS);
                                
                                /* Send CMD0 and CMD8 commands */
                                f8 = Cy_SD_Host_VoltageCheck(base); 
                                
                                if (0u < sdio)
                                {
                                    /* Send CMD5 to get IO OCR */
                                    ret = Cy_SD_Host_IoOcr(base,
                                                           config->lowVoltageSignaling,
                                                           &s18A,
                                                           &sdio,
                                                           &mp,                                
                                                           &ocr);
                                }

                                if (mp)
                                {
                                    /* Send ACMD41 */
                                    ret = Cy_SD_Host_SdOcr(base,
                                                           config->lowVoltageSignaling,
                                                           &s18A,
                                                           &mp,
                                                           f8,
                                                           &ocr,
                                                           context);
                                }
                                
                                s18A = 0u;
                            }
                        }
                        
                        if (CY_SD_HOST_SDIO != context->cardType)
                        {
                            /* Get CID (CMD2).  */
                            ret = Cy_SD_Host_GetCid(base, cid);
                            
                            Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);
                        }                                       /* The Identification state. */

                        /* Get RCA (CMD3). */
                        context->RCA = Cy_SD_Host_GetRca(base);
                        
                        Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);
                        
                        if (CY_SD_HOST_SDIO != context->cardType) /* The Standby state. */
                        {
                            /* Get CSD (CMD9) to save
                             * Max Sector Number in the context.
                             */
                            ret = Cy_SD_Host_GetCsd(base, csdReg, context);
                            
                            Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);
                        }

                        if (Cy_SD_Host_SUCCESS == ret) /* The Standby state. */
                        {
                            /* Select card (CMD7). */
                            ret = Cy_SD_Host_OpsSelectCard(base, context);
                        }

                        if ((Cy_SD_Host_SUCCESS == ret) &&
                            (CY_SD_HOST_SDIO != context->cardType)) /* The Transition state. */
                        {
                            /* Set Bus Width (ACMD6). */
                            if (CY_SD_HOST_BUS_WIDTH_1_BIT != config->busWidth)
                            {
                                ret = Cy_SD_Host_SetBusWidth(base,
                                                             CY_SD_HOST_BUS_WIDTH_4_BIT,
                                                             context);
                                Cy_SysLib_Delay(CY_SD_HOST_READ_TIMEOUT_MS); 
                            }

                            if (Cy_SD_Host_SUCCESS == ret)
                            {
                                if ((1u == s18A) && (config->lowVoltageSignaling))
                                {
                                    speedMode = CY_SD_HOST_BUS_SPEED_SDR12_5;
                                }

                                /* Set Bus Speed Mode (CMD6). */
                                ret = Cy_SD_Host_SetBusSpeedMode(base,
                                                                 speedMode,
                                                                 context);
                                Cy_SysLib_Delay(CY_SD_HOST_READ_TIMEOUT_MS);                      

                                if (Cy_SD_Host_SUCCESS == ret)
                                {
                                    (void)Cy_SD_Host_SdCardChangeClock(base,
                                                                       CY_SD_HOST_CLK_20M);
                                }
                                Cy_SysLib_Delay(CY_SD_HOST_CLK_RAMP_UP_TIME_MS);
                            }
                        }
                    }
                }
            }
            else
            {
                ret = Cy_SD_Host_eMMC_InitCard(base, config, context);
            }
            
            *config->rca = context->RCA;
            *config->cardType = context->cardType;
            *config->cardCapacity = context->cardCapacity;
        }
        else
        {
            /* The SD card is disconnected or embedded card. */
            ret = Cy_SD_Host_ERROR_DISCONNECTED;
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_Read
****************************************************************************//**
*
*  Reads a single or multiple block data from the SD card in the DMA mode.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *config
*     The pointer to the SD card read-write structure.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_Read(SDHC_Type *base,
                                       cy_stc_sd_host_write_read_config_t *config,
                                       cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t       ret  = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    cy_stc_sd_host_cmd_config_t  cmd;
    uint32_t                     dataAddress = config->address;
    cy_stc_sd_host_data_config_t dataConfig;
    uint32_t aDmaDescriptTbl[CY_SD_HOST_ADMA2_DESCR_SIZE]; /* The ADMA2 descriptor table. */
    uint32_t length; /* The length of data to transfer for a descriptor. */

    if ((NULL != context) && (NULL != config) && (NULL != config->data))
    {
        CY_ASSERT_L3(CY_SD_HOST_IS_AUTO_CMD_VALID(config->autoCommand));
        CY_ASSERT_L2(CY_SD_HOST_IS_TIMEOUT_VALID(config->dataTimeout));
        CY_ASSERT_L3(CY_SD_HOST_IS_DMA_WR_RD_VALID(context->dmaType));

        /* 0 < maxSectorNum check is needed for legacy EMMC cards. */
        if (!((0u < context->maxSectorNum) &&
             (((context->maxSectorNum - dataAddress) < config->numberOfBlocks) ||
             (context->maxSectorNum < (dataAddress + config->numberOfBlocks)))))
        {
            if (CY_SD_HOST_SDSC == context->cardCapacity)
            {
                dataAddress = dataAddress << CY_SD_HOST_SDSC_ADDR_SHIFT;
            }

            if (1u < config->numberOfBlocks)
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD18;
                dataConfig.enableIntAtBlockGap = true;
            }
            else
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD17;
                dataConfig.enableIntAtBlockGap = false;
            }

            dataConfig.blockSize = CY_SD_HOST_BLOCK_SIZE;
            dataConfig.numberOfBlock = config->numberOfBlocks;
            dataConfig.enableDma = true;
            dataConfig.autoCommand = config->autoCommand;
            dataConfig.read = true;
            dataConfig.dataTimeout = config->dataTimeout;
            dataConfig.enReliableWrite = config->enReliableWrite;

            if (CY_SD_HOST_DMA_ADMA2 == context->dmaType)
            {
                length = CY_SD_HOST_BLOCK_SIZE * config->numberOfBlocks;

                aDmaDescriptTbl[0] = (1uL << CY_SD_HOST_ADMA_ATTR_VALID_POS) | /* Attr Valid */
                                     (1uL << CY_SD_HOST_ADMA_ATTR_END_POS) |   /* Attr End */
                                     (0uL << CY_SD_HOST_ADMA_ATTR_INT_POS) |   /* Attr Int */
                                     (CY_SD_HOST_ADMA_TRAN << CY_SD_HOST_ADMA_ACT_POS) |
                                     (length << CY_SD_HOST_ADMA_LEN_POS);     /* Len */

                aDmaDescriptTbl[1] = (uint32_t)config->data;

                /* The address of ADMA2 descriptor table. */
                dataConfig.data = (uint32_t*)&aDmaDescriptTbl[0];
            }
            else
            {
                /* The SDMA mode. */
                dataConfig.data = (uint32_t*)config->data;
            }

            ret = Cy_SD_Host_InitDataTransfer(base, &dataConfig, context);

            if (Cy_SD_Host_SUCCESS == ret)
            {
                cmd.commandArgument = dataAddress;
                cmd.dataPresent     = true;
                cmd.enableAutoResponseErrorCheck = false;
                cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
                cmd.enableCrcCheck  = true;
                cmd.enableIdxCheck  = true;
                cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

                ret = Cy_SD_Host_SendCommand(base, &cmd);
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_Write
****************************************************************************//**
*
*  Writes a single or multiple block data to the SD card in the DMA mode.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *config
*     The pointer to the SD card read-write structure.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_Write(SDHC_Type *base,
                                        cy_stc_sd_host_write_read_config_t *config,
                                        cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t       ret  = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    cy_stc_sd_host_cmd_config_t  cmd;
    uint32_t                     dataAddress = config->address;
    cy_stc_sd_host_data_config_t dataConfig;
    uint32_t aDmaDescriptTbl[CY_SD_HOST_ADMA2_DESCR_SIZE]; /* The ADMA2 descriptor table. */
    uint32_t length;  /* The length of data to transfer for a descriptor. */

    if ((NULL != context) && (NULL != config) && (NULL != config->data))
    {
        CY_ASSERT_L3(CY_SD_HOST_IS_AUTO_CMD_VALID(config->autoCommand));
        CY_ASSERT_L2(CY_SD_HOST_IS_TIMEOUT_VALID(config->dataTimeout));
        CY_ASSERT_L3(CY_SD_HOST_IS_DMA_WR_RD_VALID(context->dmaType));

        /* 0 < maxSectorNum check is needed for legacy EMMC cards */
        if (!((0u < context->maxSectorNum) &&
                 (((context->maxSectorNum - dataAddress) < config->numberOfBlocks) ||
                  (context->maxSectorNum < (dataAddress + config->numberOfBlocks)))))
        {
            if (CY_SD_HOST_SDSC == context->cardCapacity)
            {
                /* SDSC card uses byte unit address, multiply by 512 */
                dataAddress = dataAddress << CY_SD_HOST_SDSC_ADDR_SHIFT;
            }

            if (1u < config->numberOfBlocks)
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD25;
                dataConfig.enableIntAtBlockGap = true;
            }
            else
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD24;
                dataConfig.enableIntAtBlockGap = false;
            }

            dataConfig.blockSize = CY_SD_HOST_BLOCK_SIZE;
            dataConfig.numberOfBlock = config->numberOfBlocks;
            dataConfig.enableDma = true;
            dataConfig.autoCommand = config->autoCommand;
            dataConfig.read = false;
            dataConfig.data = (uint32_t*)config->data;
            dataConfig.dataTimeout = config->dataTimeout;
            dataConfig.enReliableWrite = config->enReliableWrite;

            if (CY_SD_HOST_DMA_ADMA2 == context->dmaType)
            {
                length = CY_SD_HOST_BLOCK_SIZE * config->numberOfBlocks;

                aDmaDescriptTbl[0] = (1uL << CY_SD_HOST_ADMA_ATTR_VALID_POS) | /* Attr Valid */
                                     (1uL << CY_SD_HOST_ADMA_ATTR_END_POS) |   /* Attr End */
                                     (0uL << CY_SD_HOST_ADMA_ATTR_INT_POS) |   /* Attr Int */
                                     (CY_SD_HOST_ADMA_TRAN << CY_SD_HOST_ADMA_ACT_POS) |
                                     (length << CY_SD_HOST_ADMA_LEN_POS);     /* Len */

                aDmaDescriptTbl[1] = (uint32_t)config->data;

                /* The address of ADMA descriptor table. */
                dataConfig.data = (uint32_t*)&aDmaDescriptTbl[0];
            }
            else
            {
                dataConfig.data = (uint32_t*)config->data;
            }

            ret = Cy_SD_Host_InitDataTransfer(base, &dataConfig, context);

            if (Cy_SD_Host_SUCCESS == ret)
            {
                cmd.commandArgument = dataAddress;
                cmd.dataPresent     = true;
                cmd.enableAutoResponseErrorCheck = false;
                cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
                cmd.enableCrcCheck  = true;
                cmd.enableIdxCheck  = true;
                cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

                ret = Cy_SD_Host_SendCommand(base, &cmd);
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_Erase
****************************************************************************//**
*
*  Erases a number block data of SD card.
*
* \note This function starts the erase operation end exits.
* It is the user responsibility to check when the erase operation completes.
* The erase operation completes when ref\ Cy_SD_Host_GetCardStatus returns
* status value where both the ready for data (CY_SD_HOST_CMD13_READY_FOR_DATA)
* and the card transition (CY_SD_HOST_CARD_TRAN) bits are set.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param startAddr
*     The address to start erasing from.
*
* \param endAddr
*     The address to stop erasing.
*
* \param eraseType
*     Specifies the erase type (FULE, DISCARD).
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_Erase(SDHC_Type *base,
                              uint32_t startAddr,
                              uint32_t endAddr,
                              cy_en_sd_host_erase_type_t eraseType,
                              cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t      ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    cy_stc_sd_host_cmd_config_t cmd;

    if ((NULL != context) && (NULL != base))
    { 
        /* 0<maxSectorNum check is needed for legacy EMMC cards */
        if (!((0u < context->maxSectorNum) &&
              ((context->maxSectorNum < (startAddr)) ||
               (context->maxSectorNum < (endAddr)))))
        {
            /* Init the command parameters */
            cmd.dataPresent    = false;
            cmd.enableAutoResponseErrorCheck = false;
            cmd.respType       = CY_SD_HOST_RESPONSE_LEN_48;
            cmd.enableCrcCheck = true;
            cmd.enableIdxCheck = true;
            cmd.cmdType        = CY_SD_HOST_CMD_NORMAL;

            /* EraseStartAddr (CMD32) */
            if (CY_SD_HOST_SDSC == context->cardCapacity)
            {
                /* The SDSC card uses byte unit address, multiply by 512 */
                startAddr = startAddr << CY_SD_HOST_SDSC_ADDR_SHIFT;
            }

            if (CY_SD_HOST_EMMC == context->cardType)
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD35;
            }
            else
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD32;
            }
            cmd.commandArgument = startAddr;

            ret = Cy_SD_Host_SendCommand(base, &cmd);

            /* EraseEndAddr (CMD33) */
            if (CY_SD_HOST_SDSC == context->cardCapacity)
            {
                /* The SDSC card uses byte unit address, multiply by 512 */
                endAddr = endAddr << CY_SD_HOST_SDSC_ADDR_SHIFT;
            }

            if (CY_SD_HOST_EMMC == context->cardType)
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD36;
            }
            else
            {
                cmd.commandIndex = CY_SD_HOST_SD_CMD33;
            }
            cmd.commandArgument = endAddr;

            ret = Cy_SD_Host_SendCommand(base, &cmd);

            /* Erase (CMD38) */
            cmd.commandIndex = CY_SD_HOST_SD_CMD38;

            switch (eraseType)
            {
                case CY_SD_HOST_ERASE_ERASE:
                    cmd.commandArgument = CY_SD_HOST_ERASE;
                    break;
                case CY_SD_HOST_ERASE_DISCARD:
                    if (CY_SD_HOST_EMMC == context->cardType)
                    {
                        cmd.commandArgument = CY_SD_HOST_EMMC_DISCARD;
                    }
                    else
                    {
                        cmd.commandArgument = CY_SD_HOST_SD_DISCARD;
                    }
                    break;
                case CY_SD_HOST_ERASE_FUJE:
                    cmd.commandArgument = CY_SD_HOST_SD_FUJE;
                    break;
                case CY_SD_HOST_ERASE_SECURE:
                    cmd.commandArgument = CY_SD_HOST_EMMC_SECURE_ERASE;
                    break;
                case CY_SD_HOST_ERASE_SECURE_TRIM_STEP_2:
                    cmd.commandArgument = CY_SD_HOST_EMMC_SECURE_TRIM_STEP_2;
                    break;
                case CY_SD_HOST_ERASE_SECURE_TRIM_STEP_1:
                    cmd.commandArgument = CY_SD_HOST_EMMC_SECURE_TRIM_STEP_1;
                    break;
                case CY_SD_HOST_ERASE_TRIM:
                    cmd.commandArgument = CY_SD_HOST_EMMC_TRIM;
                    break;
                default:
                    ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
                    break;
            }

            ret = Cy_SD_Host_SendCommand(base, &cmd);
        }
    }

    return ret;
}

/* The commands low level section */

/*******************************************************************************
* Function Name: Cy_SD_Host_PollCmdLineFree
****************************************************************************//**
*
*  Waits for command line free.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollCmdLineFree(SDHC_Type const *base)
{
    cy_en_sd_host_status_t ret   = Cy_SD_Host_ERROR_TIMEOUT;
    uint32_t               retry = CY_SD_HOST_RETRY_TIME;
    bool                   commandInhibit;

    while (retry > 0u)
    {
        commandInhibit = _FLD2BOOL(SDHC_CORE_PSTATE_REG_CMD_INHIBIT,
                               SDHC_CORE_PSTATE_REG(base));

        if (false == commandInhibit)
        {
            ret = Cy_SD_Host_SUCCESS;
            break;
        }

        Cy_SysLib_DelayUs(CY_SD_HOST_CMD_TIMEOUT_MS);
        retry--;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_PollDataLineFree
****************************************************************************//**
*
*  Waits or data line free.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollDataLineFree(SDHC_Type const *base)
{
    cy_en_sd_host_status_t ret   = Cy_SD_Host_ERROR_TIMEOUT;
    uint32_t               retry = CY_SD_HOST_RETRY_TIME;
    bool                   dataInhibit;

    while (retry > 0u)
    {
        dataInhibit = _FLD2BOOL(SDHC_CORE_PSTATE_REG_CMD_INHIBIT_DAT,
                                   SDHC_CORE_PSTATE_REG(base));

        if (false == dataInhibit)
        {
            ret = Cy_SD_Host_SUCCESS;
            break;
        }

        Cy_SysLib_DelayUs(CY_SD_HOST_CMD_TIMEOUT_MS);
        retry--;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_PollBufferReadReady
****************************************************************************//**
*
*  Waits for the Buffer read ready event.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollBufferReadReady(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_TIMEOUT;
    uint32_t               retry = CY_SD_HOST_RETRY_TIME;

    while (retry > 0u)
    {
        /* Buffer read ready */
        if (true == _FLD2BOOL(SDHC_CORE_NORMAL_INT_STAT_R_BUF_RD_READY,
                             SDHC_CORE_NORMAL_INT_STAT_R(base)))
        {
            /* Clear interrupt flag */
             SDHC_CORE_NORMAL_INT_STAT_R(base) = CY_SD_HOST_BUF_RD_READY;

            ret = Cy_SD_Host_SUCCESS;
            break;
        }

        Cy_SysLib_DelayUs(CY_SD_HOST_BUFFER_RDY_TIMEOUT_MS);
        retry--;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_PollBufferWriteReady
****************************************************************************//**
*
*  Waits for the Buffer read ready event.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollBufferWriteReady(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_TIMEOUT;
    uint32_t               retry = CY_SD_HOST_RETRY_TIME;

    while (retry > 0u)
    {
        /* Buffer read ready */
        if (true == _FLD2BOOL(SDHC_CORE_NORMAL_INT_STAT_R_BUF_WR_READY,
                             SDHC_CORE_NORMAL_INT_STAT_R(base)))
        {
            /* Clear interrupt flag */
             SDHC_CORE_NORMAL_INT_STAT_R(base) = CY_SD_HOST_BUF_WR_READY;

            ret = Cy_SD_Host_SUCCESS;
            break;
        }

        Cy_SysLib_DelayUs(CY_SD_HOST_BUFFER_RDY_TIMEOUT_MS);
        retry--;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_PollCmdComplete
****************************************************************************//**
*
*  Waits for the command complete event.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollCmdComplete(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_TIMEOUT;
    uint32_t               retry = CY_SD_HOST_RETRY_TIME;

    while (retry > 0u)
    {
        /* Command complete */
        if (true == _FLD2BOOL(SDHC_CORE_NORMAL_INT_STAT_R_CMD_COMPLETE,
                             SDHC_CORE_NORMAL_INT_STAT_R(base)))
        {
            /* Clear interrupt flag */
             SDHC_CORE_NORMAL_INT_STAT_R(base) = CY_SD_HOST_CMD_COMPLETE;

            ret = Cy_SD_Host_SUCCESS;
            break;
        }

        Cy_SysLib_DelayUs(CY_SD_HOST_CMD_TIMEOUT_MS);
        retry--;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_PollTransferComplete
****************************************************************************//**
*
*  Waits for the command complete event.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_PollTransferComplete(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_TIMEOUT;
    uint32_t               retry = CY_SD_HOST_RETRY_TIME;

    while (retry > 0u)
    {
        /* Transfer complete */
        if (true == _FLD2BOOL(SDHC_CORE_NORMAL_INT_STAT_R_XFER_COMPLETE,
                             SDHC_CORE_NORMAL_INT_STAT_R(base)))
        {
            /* Clear interrupt flag */
             SDHC_CORE_NORMAL_INT_STAT_R(base) = CY_SD_HOST_XFER_COMPLETE;

            ret = Cy_SD_Host_SUCCESS;
            break;
        }

        Cy_SysLib_DelayUs(CY_SD_HOST_WRITE_TIMEOUT_MS);
        retry--;
    }

    return ret;
}


/*******************************************************************************
* Function Name: SdcmdRxData
****************************************************************************//**
*
*  Reads the command data.
*
* \param *base
*     The SDIO registers structure pointer.
*
* \param *pcmd
*     The pointer to the current command data structure.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t SdcmdRxData(SDHC_Type *base,
                                   cy_stc_sd_host_data_config_t *pcmd)
{
    cy_en_sd_host_status_t ret;
    uint32_t               blkSize = 0u;
    uint32_t               blkCnt  = 0u;
    uint32_t               i       = 0u;
    uint32_t               retry;

    blkCnt = pcmd->numberOfBlock;
    blkSize = pcmd->blockSize;

    if (pcmd->enableDma)
    {
        ret = Cy_SD_Host_PollTransferComplete(base);
    }
    else
    {
        ret = Cy_SD_Host_PollBufferReadReady(base);

        if (Cy_SD_Host_SUCCESS == ret)
        {
            while (blkCnt > 0u)
            {
                retry = CY_SD_HOST_RETRY_TIME;
                while ((false == _FLD2BOOL(SDHC_CORE_PSTATE_REG_BUF_RD_ENABLE,
                                          SDHC_CORE_PSTATE_REG(base))) &&
                       (retry > 0u))
                {
                    Cy_SysLib_DelayUs(CY_SD_HOST_READ_TIMEOUT_MS);
                    retry--;
                }

                if (false == _FLD2BOOL(SDHC_CORE_PSTATE_REG_BUF_RD_ENABLE,
                                       SDHC_CORE_PSTATE_REG(base)))
                {
                    break;
                }

                for ( i = blkSize >> 2u; i != 0u; i-- )
                {
                    *pcmd->data = Cy_SD_Host_BufferRead(base);
                    pcmd->data++;
                }
                blkCnt--;
            }
        }

        ret = Cy_SD_Host_PollTransferComplete(base);
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SendCommand
****************************************************************************//**
*
*  Starts sending a command on the SD bus. If the command uses the data lines
*  Cy_SD_Host_InitDataTransfer()must be call first.
*  This function returns before the command finishes.
*  To determine if the command is done, read the Normal Interrupt Status register
*  and check the CMD_COMPLETE flag. To determine if the entire transfer is done
*  check the XFER_COMPLETE flag. Also the interrupt is used and flags are set
*  on these events in an ISR.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *config
*  The configuration structure for the command.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SendCommand(SDHC_Type *base,
                                              cy_stc_sd_host_cmd_config_t const *config)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_SUCCESS;

    if ((NULL != base) && (NULL != config))
    {
        CY_ASSERT_L2(CY_SD_HOST_IS_CMD_IDX_VALID(config->commandIndex));
        CY_ASSERT_L3(CY_SD_HOST_IS_CMD_TYPE_VALID(config->cmdType));

        if (Cy_SD_Host_SUCCESS == ret)
        {
            ret = Cy_SD_Host_PollCmdLineFree(base);
            if (Cy_SD_Host_SUCCESS == ret)
            {
                ret = Cy_SD_Host_PollDataLineFree(base);
                if (Cy_SD_Host_SUCCESS == ret)
                {
                    /* Set the commandArgument directly to the hardware register */
                    SDHC_CORE_ARGUMENT_R(base) = config->commandArgument;

                    /* Update the command hardware register (the command is sent)
                    * according to the spec, this register should be only write once
                    */
                    SDHC_CORE_CMD_R(base) = (uint16_t)(_VAL2FLD(SDHC_CORE_CMD_R_CMD_TYPE, 0u) |
                                       _VAL2FLD(SDHC_CORE_CMD_R_RESP_TYPE_SELECT, (uint32_t)config->respType) |
                                       _VAL2FLD(SDHC_CORE_CMD_R_CMD_TYPE, (uint32_t)config->cmdType) |
                                       _VAL2FLD(SDHC_CORE_CMD_R_DATA_PRESENT_SEL, ((true == config->dataPresent) ? 1u : 0u)) |
                                       _VAL2FLD(SDHC_CORE_CMD_R_CMD_IDX_CHK_ENABLE, ((true == config->enableIdxCheck) ? 1u : 0u)) |
                                       _VAL2FLD(SDHC_CORE_CMD_R_CMD_CRC_CHK_ENABLE, ((true == config->enableCrcCheck) ? 1u : 0u)) |
                                       _VAL2FLD(SDHC_CORE_CMD_R_CMD_INDEX, config->commandIndex & CY_SD_HOST_ACMD_OFFSET_MASK));

                    /* Wait for the Command Complete event. */
                    ret = Cy_SD_Host_PollCmdComplete(base);
                }
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetBusWidth
****************************************************************************//**
*
*  Sends out SD bus width changing command.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param width
*     The width of the data bus.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SetBusWidth(SDHC_Type *base,
                                              cy_en_sd_host_bus_width_t width,
                                              cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t    ret = Cy_SD_Host_SUCCESS;
    uint32_t                  cmdArgument = 0u;
    cy_en_sd_host_bus_width_t bitMode = CY_SD_HOST_BUS_WIDTH_1_BIT;

    if ((NULL != base) && (NULL != context))
    {
        if (CY_SD_HOST_SD == context->cardType)
        {
            switch (width)
            {
                case CY_SD_HOST_BUS_WIDTH_1_BIT:
                    cmdArgument = 0u;  /* 0 = 1bit */
                    bitMode = CY_SD_HOST_BUS_WIDTH_1_BIT;
                    break;
                case CY_SD_HOST_BUS_WIDTH_4_BIT:
                    cmdArgument = 2u;  /* 2 = 4bit */
                    bitMode = CY_SD_HOST_BUS_WIDTH_4_BIT;
                    break;
                default:
                    ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
                    break;
            }

            if (Cy_SD_Host_SUCCESS == ret)
            {
                ret = Cy_SD_Host_OpsSetSdBusWidth(base, cmdArgument, context);
            }
        }
        else if (CY_SD_HOST_EMMC == context->cardType)
        {
            switch (width)
            {
                /* The CMD6 Argument data structure:
                 * [ACCESS:2]<<24 | [EXT_CSD IDX:8]<<16
                 * |[Value:8]<<8 | [CMD set:2]<<0
                 */
                case CY_SD_HOST_BUS_WIDTH_1_BIT:
                    cmdArgument = (CY_SD_HOST_EMMC_ACCESS_WRITE_BYTE << CY_SD_HOST_EMMC_CMD6_ACCESS_OFFSET) |
                                      (CY_SD_HOST_EMMC_BUS_WIDTH_ADDR << CY_SD_HOST_EMMC_CMD6_IDX_OFFSET) |
                                      (0x0uL << CY_SD_HOST_EMMC_CMD6_VALUE_OFFSET) |
                                      (0x0uL << CY_SD_HOST_EMMC_CMD6_CMD_SET_OFFSET);
                    bitMode = CY_SD_HOST_BUS_WIDTH_1_BIT;
                    break;
                case CY_SD_HOST_BUS_WIDTH_4_BIT:
                    cmdArgument = (CY_SD_HOST_EMMC_ACCESS_WRITE_BYTE << CY_SD_HOST_EMMC_CMD6_ACCESS_OFFSET) |
                                      (CY_SD_HOST_EMMC_BUS_WIDTH_ADDR << CY_SD_HOST_EMMC_CMD6_IDX_OFFSET) |
                                      (0x1uL << CY_SD_HOST_EMMC_CMD6_VALUE_OFFSET) |
                                      (0x0uL << CY_SD_HOST_EMMC_CMD6_CMD_SET_OFFSET);
                    bitMode = CY_SD_HOST_BUS_WIDTH_4_BIT;
                    break;
                case CY_SD_HOST_BUS_WIDTH_8_BIT:
                    cmdArgument = (CY_SD_HOST_EMMC_ACCESS_WRITE_BYTE << CY_SD_HOST_EMMC_CMD6_ACCESS_OFFSET) |
                                      (CY_SD_HOST_EMMC_BUS_WIDTH_ADDR << CY_SD_HOST_EMMC_CMD6_IDX_OFFSET) |
                                      (0x2uL << CY_SD_HOST_EMMC_CMD6_VALUE_OFFSET) |
                                      (0x0uL << CY_SD_HOST_EMMC_CMD6_CMD_SET_OFFSET);
                    bitMode = CY_SD_HOST_BUS_WIDTH_8_BIT;
                    break;
                default:
                    ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
                    break;
            }

            if (Cy_SD_Host_SUCCESS == ret)
            {
                /* Send CMD6 */
                ret = Cy_SD_Host_OpsSwitchFunc(base, cmdArgument);
            }
        }
        else if (CY_SD_HOST_SDIO == context->cardType)
        {
            switch (width)
            {
                case CY_SD_HOST_BUS_WIDTH_1_BIT:
                    cmdArgument = 0u;
                    bitMode = CY_SD_HOST_BUS_WIDTH_1_BIT;
                    break;
                case CY_SD_HOST_BUS_WIDTH_4_BIT:
                    cmdArgument = CY_SD_HOST_CCCR_BUS_WIDTH_1;
                    bitMode = CY_SD_HOST_BUS_WIDTH_4_BIT;
                    break;
                case CY_SD_HOST_BUS_WIDTH_8_BIT:
                    cmdArgument = CY_SD_HOST_CCCR_BUS_WIDTH_0 |
                                      CY_SD_HOST_CCCR_BUS_WIDTH_1 |
                                      CY_SD_HOST_CCCR_S8B;
                    bitMode = CY_SD_HOST_BUS_WIDTH_8_BIT;
                    break;
                default:
                    ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
                    break;
            }

            ret = Cy_SD_Host_OpsSendIoRwDirectCmd(base,
                                                  1u,
                                                  0u,
                                                  0u,
                                                  CY_SD_HOST_CCCR_BUS_INTERFACE_CTR,
                                                  cmdArgument);
        }
        else
        {
           ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
        }

        if (Cy_SD_Host_SUCCESS == ret)
        {
            /* Update the host side setting. */
            ret = Cy_SD_Host_SetHostBusWidth(base, bitMode);
        }
    }
    else
    {
       ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsGoIdle
****************************************************************************//**
*
*  Send CMD0 (Go idle).
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t Cy_SD_Host_OpsGoIdle(SDHC_Type *base)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD0;
    cmd.commandArgument = 0u;
    cmd.enableCrcCheck  = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_NONE;
    cmd.enableIdxCheck  = false;
    cmd.dataPresent     = false;
    cmd.cmdType         = CY_SD_HOST_CMD_ABORT;

    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsVoltageSwitch
****************************************************************************//**
*
*  Send CMD11 (Signal Voltage Switch to 1.8 V).
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t Cy_SD_Host_OpsVoltageSwitch(SDHC_Type *base,
                                                          cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t         ret;
    cy_stc_sd_host_cmd_config_t    cmd;
    uint32_t pState;

    /* Voltage switch (CMD11). */
    cmd.commandIndex = CY_SD_HOST_SD_CMD11;
    cmd.commandArgument = 0u;
    cmd.enableCrcCheck = true;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableIdxCheck = true;
    cmd.dataPresent = false;
    cmd.cmdType     = CY_SD_HOST_CMD_NORMAL;
    
    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    if ((Cy_SD_Host_SUCCESS == ret) && (CY_SD_HOST_SDIO != context->cardType))
    {
        /* Disable providing the SD Clock. */
        Cy_SD_Host_DisableSdClk(base);

        pState = Cy_SD_Host_GetPresentState(base) & SDHC_CORE_PSTATE_REG_DAT_3_0_Msk;

        /* Check DAT[3:0]. */
        if (0uL == pState)
        {
            /* Switch the bus to 1.8 V (Set the IO_VOLT_SEL pin to low)*/
            Cy_SD_Host_ChangeIoVoltage(base, CY_SD_HOST_IO_VOLT_1_8V);

            /* Wait 10 ms to 1.8 voltage regulator to be stable. */
            Cy_SysLib_Delay(CY_SD_HOST_1_8_REG_STABLE_TIME_MS);

            /* Check the 1.8V signaling enable. */
            if (true == _FLD2BOOL(SDHC_CORE_HOST_CTRL2_R_SIGNALING_EN,
                                  SDHC_CORE_HOST_CTRL2_R(base)))
            {
                /* Enable providing the SD Clock. */
                Cy_SD_Host_EnableSdClk(base);

                /* Wait for stable CLK */
                Cy_SysLib_Delay(CY_SD_HOST_CLK_RAMP_UP_TIME_MS);

                pState = Cy_SD_Host_GetPresentState(base) & SDHC_CORE_PSTATE_REG_DAT_3_0_Msk;

                if (SDHC_CORE_PSTATE_REG_DAT_3_0_Msk != pState)
                {
                    ret = Cy_SD_Host_ERROR_UNUSABLE_CARD;
                }
            }
            else
            {
                ret = Cy_SD_Host_ERROR_UNUSABLE_CARD;
            }
        }
        else
        {
            ret = Cy_SD_Host_ERROR_UNUSABLE_CARD;
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSendIoRwDirectCmd
****************************************************************************//**
*
*  Sends CMD52 (Reads/writes 1 byte to the SDIO register).
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t Cy_SD_Host_OpsSendIoRwDirectCmd(SDHC_Type *base,
                                                uint32_t rwFlag,
                                                uint32_t functionNumber,
                                                uint32_t rawFlag,
                                                uint32_t registerAaddress,
                                                uint32_t data)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD52;
    cmd.commandArgument = ((rwFlag & 0x01uL) << CY_SD_HOST_CMD52_RWFLAG_POS) |
                          ((functionNumber & CY_SD_HOST_CMD52_FUNCT_NUM_MSK) << CY_SD_HOST_CMD52_FUNCT_NUM_POS) |
                          ((rawFlag & 0x01uL) << CY_SD_HOST_CMD52_RAWFLAG_POS) |
                          ((registerAaddress & CY_SD_HOST_CMD52_REG_ADDR_MSK) << CY_SD_HOST_CMD52_REG_ADDR_POS) |
                          (data & CY_SD_HOST_CMD52_DATA_MSK);

    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48B;
    cmd.enableCrcCheck  = true;
    cmd.enableIdxCheck  = true;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;
    
    ret = Cy_SD_Host_SendCommand(base, &cmd);

    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSendAppCmd
****************************************************************************//**
*
*  Sends CMD55 (Sends the application command).
* If no response of CMD55 recieved the card is not the SD card.
* The CMD55 response may have error because some SD card do not support the CMD8
* command.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t Cy_SD_Host_OpsSendAppCmd(SDHC_Type *base,
                                                cy_stc_sd_host_context_t const *context)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD55;
    cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = true;
    cmd.enableIdxCheck  = true;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSendIfCond
****************************************************************************//**
*
*  Send CMD8 (Send application command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \param cmdArgument
*     The command argument.
*
* \param noResponse
*     No response if true, otherwise 48 bit response with CRC and IDX check.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsSendIfCond(SDHC_Type *base,
                                                uint32_t cmdArgument,
                                                bool noResponse)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD8;
    cmd.commandArgument = cmdArgument;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;
    if (noResponse)
    {
        cmd.respType        = CY_SD_HOST_RESPONSE_NONE;
        cmd.enableCrcCheck  = false;
        cmd.enableIdxCheck  = false;
    }
    else
    {
        cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
        cmd.enableCrcCheck  = true;
        cmd.enableIdxCheck  = true;
    }

    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSelectCard
****************************************************************************//**
*
*  Send CMD7 (Send select card command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
 cy_en_sd_host_status_t Cy_SD_Host_OpsSelectCard(SDHC_Type *base,
                                                cy_stc_sd_host_context_t const *context)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD7;
    cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false; 
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48B;
    cmd.enableCrcCheck  = false;
    cmd.enableIdxCheck  = false;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    (void)Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    /* R1b response requires sending an optional busy
    * signal on the DAT line. The transfer complete event
    *  should be checked and reset.
    */
    ret = Cy_SD_Host_PollTransferComplete(base);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSetSdBusWidth
****************************************************************************//**
*
*  Sends ACMD6 (Send set bus width command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsSetSdBusWidth(SDHC_Type *base,
                                                 uint32_t cmdArgument,
                                                 cy_stc_sd_host_context_t const *context)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;

    cmd.commandIndex    = CY_SD_HOST_SD_ACMD6;
    cmd.commandArgument = cmdArgument;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = false;
    cmd.enableIdxCheck  = false;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    ret = Cy_SD_Host_OpsSendAppCmd(base, context);

    if (Cy_SD_Host_SUCCESS == ret)
    {
        ret = Cy_SD_Host_SendCommand(base, &cmd);
        
        Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSwitchFunc
****************************************************************************//**
*
*  Sends CMD6 (Sends switch function command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t Cy_SD_Host_OpsSwitchFunc(SDHC_Type *base, uint32_t cmdArgument)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD6;
    cmd.commandArgument = cmdArgument;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = true;
    cmd.enableIdxCheck  = true;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSetBlockCount
****************************************************************************//**
*
*  Sends CMD23 (Sends the Set Block Count command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsSetBlockCount(SDHC_Type *base,
                                                                   bool reliableWrite,
                                                                   uint32_t blockNum)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;
    uint32_t cmdArgument;

    cmdArgument = (blockNum & CY_SD_HOST_CMD23_BLOCKS_NUM_MASK) |
                  (reliableWrite ? (1uL << CY_SD_HOST_CMD23_RELIABLE_WRITE_POS) : 0uL);

    cmd.commandIndex    = CY_SD_HOST_SD_CMD23;
    cmd.commandArgument = cmdArgument;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = true;
    cmd.enableIdxCheck  = true;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsProgramCsd
****************************************************************************//**
*
*  Sends CMD27 (Sends the Program CSD command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \param csd
*     The Card-Specific Data register value.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_OpsProgramCsd(SDHC_Type *base,
                                                                uint32_t csd,
                                                                cy_stc_sd_host_context_t *context)
{
    cy_en_sd_host_status_t       ret;
    cy_stc_sd_host_cmd_config_t  cmd;
    cy_stc_sd_host_data_config_t dataConfig;
    uint32_t retry;
    uint32_t i;
    uint32_t blkSize = CY_SD_HOST_CSD_BLOCKS;
    uint32_t csdTepm;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD27;
    cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
    cmd.dataPresent     = true;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = true;
    cmd.enableIdxCheck  = true;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    dataConfig.blockSize            = CY_SD_HOST_CSD_BLOCKS;
    dataConfig.numberOfBlock        = 1u;
    dataConfig.enableDma            = false;
    dataConfig.autoCommand          = CY_SD_HOST_AUTO_CMD_NONE;
    dataConfig.read                 = false;
    dataConfig.data                 = context->csd;
    dataConfig.dataTimeout          = CY_SD_HOST_MAX_TIMEOUT;
    dataConfig.enableIntAtBlockGap  = false;
    dataConfig.enReliableWrite      = false;
    
    /* The CSD register is sent using DAT lines. Init data transfer in the non-DMA mode. */
    (void)Cy_SD_Host_InitDataTransfer(base, &dataConfig, context);

    /* Send the program CSD command (CMD27) */
    ret = Cy_SD_Host_SendCommand(base, &cmd);

    if (Cy_SD_Host_SUCCESS == ret)
    {
        /* Wait when buffer is read ready. */
        ret = Cy_SD_Host_PollBufferWriteReady(base);

        for (i = (blkSize >> 2u); i != 0u; i--)
        {
            /* The CSD register is sent using usual data (8-bit width) type of Data packet format.
             * The usual data (8-bit width) are sent in LSB (Least Significant Byte) first,
             * MSB (Most Significant Byte) last. The bytes in each the context->csd[] element 
             * should be reordered and shifted right to one byte.
             */
            csdTepm = ((context->csd[i-1u] & CY_SD_HOST_CSD_ISBL_MASK) >> CY_SD_HOST_CSD_ISB_SHIFT) |
                      (context->csd[i-1u] & CY_SD_HOST_CSD_ISBR_MASK) |
                      ((context->csd[i-1u] & CY_SD_HOST_CSD_LSB_MASK) << CY_SD_HOST_CSD_ISB_SHIFT);
                  
            if (i > 1u)
            {
                csdTepm |= (context->csd[i-2u] & CY_SD_HOST_CSD_MSB_MASK); 
            }
            else
            {
                csdTepm &= ~CY_SD_HOST_CSD_MSB_MASK; /* Clear writable bits of the CSD register. */
                csdTepm |= csd; /* Set writable bits of the CSD register. */
            }

            (void)Cy_SD_Host_BufferWrite(base, csdTepm);
        }
    }

    /* Wait for the transfer complete */
    ret = Cy_SD_Host_PollTransferComplete(base);

    retry = CY_SD_HOST_RETRY_TIME;

    /* Check if the data line is free. */
    while ((true == _FLD2BOOL(SDHC_CORE_PSTATE_REG_DAT_LINE_ACTIVE, SDHC_CORE_PSTATE_REG(base))) && (retry > 0u) )
    {
        Cy_SysLib_DelayUs(CY_SD_HOST_WRITE_TIMEOUT_MS);
        retry--;
    }

    if ( true == _FLD2BOOL(SDHC_CORE_PSTATE_REG_DAT_LINE_ACTIVE, SDHC_CORE_PSTATE_REG(base)) )
    {
        ret = Cy_SD_Host_ERROR_TIMEOUT;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSdioSendOpCond
****************************************************************************//**
*
*  Send CMD5 (Send SDIO operation condition command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \param cmdArgument
*     The command argument.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t Cy_SD_Host_OpsSdioSendOpCond(SDHC_Type *base,
                                                           uint32_t *ocr,
                                                           uint32_t cmdArgument)
{
    cy_en_sd_host_status_t      ret;
    cy_stc_sd_host_cmd_config_t cmd;
    uint32_t                    response;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD5;
    cmd.commandArgument = cmdArgument;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = false;
    cmd.enableIdxCheck  = false;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    /* Send the SDIO operation condition command (CMD5) */
    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);

    *ocr = response;

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_OpsSdSendOpCond
****************************************************************************//**
*
*  Send ACMD41 (Send SD operation condition command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \param cmdArgument
*     The command argument.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
static cy_en_sd_host_status_t Cy_SD_Host_OpsSdSendOpCond(SDHC_Type *base,
                                                  uint32_t *ocr,
                                                  uint32_t cmdArgument,
                                                  cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t      ret;
    cy_stc_sd_host_cmd_config_t cmd;
    uint32_t                    response = 0u;

    cmd.commandIndex    = CY_SD_HOST_SD_ACMD41;
    cmd.commandArgument = cmdArgument;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = false;
    cmd.enableIdxCheck  = false;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    /* Send the application command (CMD55) */
    ret = Cy_SD_Host_OpsSendAppCmd(base, context);

    if (Cy_SD_Host_SUCCESS == ret)
    {
        ret = Cy_SD_Host_SendCommand(base, &cmd);

        Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);
            
        (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);

        *ocr = response;

        if (0x0u == cmdArgument)
        {
            /* Voltage window = 0 */
        }
        else if (0u == (response & CY_SD_HOST_ARG_ACMD41_BUSY))
        {
            /* Set error */
            ret = Cy_SD_Host_OPERATION_INPROGRESS;
        }
        else
        {
            /* Success */
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_MmcOpsSendOpCond
****************************************************************************//**
*
*  Send CMD1 (Send MMC operation condition command).
*
* \param *base
*     The SD host registers structure pointer.
*
* \param cmdArgument
*     The command argument.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_MmcOpsSendOpCond(SDHC_Type *base,
                                                   uint32_t *ocr,
                                                   uint32_t cmdArgument,
                                                   cy_stc_sd_host_context_t *context)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret;
    uint32_t                    response = 0u;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD1;
    cmd.commandArgument = cmdArgument;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48; /* R3 response */
    cmd.enableCrcCheck  = false;
    cmd.enableIdxCheck  = false;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

    ret = Cy_SD_Host_SendCommand(base, &cmd);
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    /* Get the OCR register */
    (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);

    *ocr = response;

    if (CY_SD_HOST_OCR_BUSY_BIT != (CY_SD_HOST_OCR_BUSY_BIT & response))
    {
        ret = Cy_SD_Host_OPERATION_INPROGRESS;
    }

    return ret;
}

/* The SD driver low level section */

/*******************************************************************************
* Function Name: Cy_SD_Host_SdCardChangeClock
****************************************************************************//**
*
*  Changes host controller sd clock.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param frequency
*     The frequency in Hz.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SdCardChangeClock(SDHC_Type *base, uint32_t frequency)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t               clkDiv;
    uint32_t               clockInput = CY_SD_HOST_PERI_FREQUENCY;

    CY_ASSERT_L2(CY_SD_HOST_IS_FREQ_VALID(frequency));

    if (NULL != base)
    {
        clkDiv = clockInput / 2u / frequency;
        Cy_SD_Host_DisableSdClk(base);
        ret = Cy_SD_Host_SetSdClkDiv(base, (uint16_t)clkDiv);
        Cy_SD_Host_EnableSdClk(base);
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_Init
****************************************************************************//**
*
*  Initializes an SD host module.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param config
*     The SD host module configuration.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_Init(SDHC_Type *base,
                      const cy_stc_sd_host_init_config_t *config,
                      cy_stc_sd_host_context_t *context)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_SUCCESS;

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != config) && (NULL != context))
    {
        CY_ASSERT_L3(CY_SD_HOST_IS_DMA_VALID(config->dmaType));
        
        SDHC_CORE_GP_OUT_R(base) = _VAL2FLD(SDHC_CORE_GP_OUT_R_IO_VOLT_SEL_OE, 1u) | /* The IO voltage selection signal. */
                              _VAL2FLD(SDHC_CORE_GP_OUT_R_CARD_MECH_WRITE_PROT_EN, 1u) | /* The mechanical write protection. */
                              _VAL2FLD(SDHC_CORE_GP_OUT_R_LED_CTRL_OE, config->enableLedControl ? 1u : 0u) | /* The LED Control. */
                              _VAL2FLD(SDHC_CORE_GP_OUT_R_CARD_CLOCK_OE, 1u) | /* The Sd Clk. */
                              _VAL2FLD(SDHC_CORE_GP_OUT_R_CARD_IF_PWR_EN_OE, 1u) | /* Enable the card_if_pwr_en. */
                              _VAL2FLD(SDHC_CORE_GP_OUT_R_CARD_DETECT_EN, 1u); /* Enable the card detection. */

        context->dmaType = config->dmaType;

        if (config->emmc)
        {
            /* Save the card type */
            context->cardType = CY_SD_HOST_EMMC;

            /* Set the eMMC Card present */
            SDHC_CORE_EMMC_CTRL_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_EMMC_CTRL_R(base),
                                          SDHC_CORE_EMMC_CTRL_R_CARD_IS_EMMC,
                                          1u);

        }
        
        if (config->enableLedControl)
        {                          
            /* LED Control */
            SDHC_CORE_HOST_CTRL1_R(base) = (uint8_t)_CLR_SET_FLD8U(SDHC_CORE_HOST_CTRL1_R(base),
                                          SDHC_CORE_HOST_CTRL1_R_LED_CTRL,
                                          1u);
        }

        /* Select ADMA or not */
        SDHC_CORE_HOST_CTRL1_R(base) = (uint8_t)_CLR_SET_FLD8U(SDHC_CORE_HOST_CTRL1_R(base),
                                      SDHC_CORE_HOST_CTRL1_R_DMA_SEL,
                                      config->dmaType);
                                      
            /* Set data timeout time to max */
        SDHC_CORE_TOUT_CTRL_R(base) = _CLR_SET_FLD8U(SDHC_CORE_TOUT_CTRL_R(base),
                                                SDHC_CORE_TOUT_CTRL_R_TOUT_CNT,
                                                CY_SD_HOST_MAX_TIMEOUT);

        /* Enable all statuses */
        Cy_SD_Host_SetNormalInterruptEnable(base, CY_SD_HOST_NORMAL_INT_MSK);
        Cy_SD_Host_SetErrorInterruptEnable(base, CY_SD_HOST_ERROR_INT_MSK);

        Cy_SD_Host_SetNormalInterruptMask(base, CY_SD_HOST_NORMAL_INT_MSK);
        Cy_SD_Host_SetErrorInterruptMask(base, CY_SD_HOST_ERROR_INT_MSK);

        /* Enable Host Version 4 */
        SDHC_CORE_HOST_CTRL2_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_HOST_CTRL2_R(base),
                                      SDHC_CORE_HOST_CTRL2_R_HOST_VER4_ENABLE,
                                      1u);

        /* Wait to the host stable voltage */
        Cy_SysLib_Delay(CY_SD_HOST_SUPPLY_RAMP_UP_TIME_MS);
    }
    else
    {
        ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_DeInit
****************************************************************************//**
*
* Restores the SD Host block registers back to default.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t  Cy_SD_Host_DeInit(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_SUCCESS;

    /* Check for NULL pointer */
    if (NULL != base)
    {
        if (Cy_SD_Host_SUCCESS ==  Cy_SD_Host_PollCmdLineFree(base))
        {
            if (Cy_SD_Host_SUCCESS ==  Cy_SD_Host_PollDataLineFree(base))
            {
                Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_ALL);
                
                /* Disable the SDHC block. */
                SDHC_WRAP_CTL(base) = _CLR_SET_FLD32U(SDHC_WRAP_CTL(base),
                                                SDHC_WRAP_CTL_ENABLE,
                                                0u);
            }
            else
            {
                ret = Cy_SD_Host_OPERATION_INPROGRESS;
            }
        }
        else
        {
            ret = Cy_SD_Host_OPERATION_INPROGRESS;
        }
    }
    else
    {
        ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_AbortTransfer
****************************************************************************//**
*
*  Calling this function causes the currently executing command with
*  data to be aborted. It doesn't issue a reset, that is the users responsibility.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t  Cy_SD_Host_AbortTransfer(SDHC_Type *base,
                                                 cy_stc_sd_host_context_t const *context)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t                    response = 0u;

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != context))
    {
        /* Check the card - Memory or SDIO? */
        if (CY_SD_HOST_SDIO == context->cardType)
        {
            ret = Cy_SD_Host_OpsSendIoRwDirectCmd(base,
                                                  1u,
                                                  0u,
                                                  0u,
                                                  CY_SD_HOST_CCCR_IO_ABORT,
                                                  1u);
        }
        else
        {
            cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
            cmd.dataPresent = false;
            cmd.enableAutoResponseErrorCheck = false;
            cmd.respType = CY_SD_HOST_RESPONSE_LEN_48;
            cmd.enableCrcCheck = true;
            cmd.enableIdxCheck = true;
            cmd.cmdType        = CY_SD_HOST_CMD_ABORT;

            /* Issue CMD12 */
            cmd.commandIndex = CY_SD_HOST_SD_CMD12;
            ret = Cy_SD_Host_SendCommand(base, &cmd);

            /* Issue CMD13 */
            cmd.commandIndex = CY_SD_HOST_SD_CMD13;
            ret = Cy_SD_Host_SendCommand(base, &cmd);

            /* Get R1 */
            (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);

            /* Is card in tran */
            if ((CY_SD_HOST_CARD_TRAN << CY_SD_HOST_CMD13_CURRENT_STATE) != 
                (response & CY_SD_HOST_CMD13_CURRENT_STATE_MSK))
            {
                /* Issue CMD12 */
                cmd.commandIndex = CY_SD_HOST_SD_CMD12;
                ret = Cy_SD_Host_SendCommand(base, &cmd);

                /* Issue CMD13 */
                cmd.commandIndex = CY_SD_HOST_SD_CMD13;
                ret = Cy_SD_Host_SendCommand(base, &cmd);

                /* Get R1 */
                (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);

                /* Is card in tran */
                if ((CY_SD_HOST_CARD_TRAN << CY_SD_HOST_CMD13_CURRENT_STATE) != 
                    (response & CY_SD_HOST_CMD13_CURRENT_STATE_MSK))
                {
                   ret = Cy_SD_Host_ERROR;
                }
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_WriteProtect
****************************************************************************//**
*
*  Write protects blocks of data from the SD card.
*  This function should only be called after Cy_SD_Host_SDCard_Init()/eMMC_Init().
*
* \param *base
*     The SD host registers structure pointer.
*
* \param permenantWriteProtect
*     true - the permenant write protect, false - the temporary write protect.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_WriteProtect(SDHC_Type *base,
                                               bool permenantWriteProtect,
                                               cy_stc_sd_host_context_t *context)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t csdReg;

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != context))
    {
        if (permenantWriteProtect)
        {
            csdReg = 1uL << CY_SD_HOST_CSD_PERM_WRITE_PROTECT;
        }
        else
        {
            csdReg = 1uL << CY_SD_HOST_CSD_TEMP_WRITE_PROTECT;   
        }

        ret = Cy_SD_Host_OpsProgramCsd(base, csdReg, context);
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetCardStatus
****************************************************************************//**
*
*  Returns the card status.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return uint32_t
*     The card status (the result of the CMD13 command).
*
*******************************************************************************/
uint32_t Cy_SD_Host_GetCardStatus(SDHC_Type *base,
                                  cy_stc_sd_host_context_t const *context)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t                    response = 0u;
    uint32_t                    status = (1uL << CY_SD_HOST_CMD13_ERROR);

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != context))
    {
        cmd.commandIndex    = CY_SD_HOST_SD_CMD13;
        cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
        cmd.dataPresent     = false;
        cmd.enableAutoResponseErrorCheck = false;
        cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
        cmd.enableCrcCheck  = true;
        cmd.enableIdxCheck  = true;
        cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

        ret = Cy_SD_Host_SendCommand(base, &cmd);

        if (Cy_SD_Host_SUCCESS == ret)
        {
            (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);
            status = response;
        }
        else
        {
            status = (1uL << CY_SD_HOST_CMD13_ERROR);
        }
    }

    return status;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetSdStatus
****************************************************************************//**
*
*  Returns the SD status from the card.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *sdStatus
*     The pointer to where to store the SD status array.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_GetSdStatus(SDHC_Type *base,
                                              uint32_t *sdStatus,
                                              cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t       ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    cy_stc_sd_host_cmd_config_t  cmd;
    cy_stc_sd_host_data_config_t dataConfig;

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != sdStatus) && (NULL != context))
    {
        ret = Cy_SD_Host_OpsSendAppCmd(base, context);
        if (Cy_SD_Host_SUCCESS == ret)
        {
            cmd.commandIndex    = CY_SD_HOST_SD_CMD13;
            cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
            cmd.dataPresent     = true;
            cmd.enableAutoResponseErrorCheck = false;
            cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
            cmd.enableCrcCheck  = true;
            cmd.enableIdxCheck  = true;
            cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

            dataConfig.blockSize            = CY_SD_HOST_SD_STATUS_BLOCKS;
            dataConfig.numberOfBlock        = 1u;
            dataConfig.enableDma            = false;
            dataConfig.autoCommand          = CY_SD_HOST_AUTO_CMD_NONE;
            dataConfig.read                 = true;
            dataConfig.data                 = sdStatus;
            dataConfig.dataTimeout          = CY_SD_HOST_MAX_TIMEOUT;
            dataConfig.enableIntAtBlockGap  = false;
            dataConfig.enReliableWrite      = false;

            (void)Cy_SD_Host_InitDataTransfer(base, &dataConfig, context);

            ret = Cy_SD_Host_SendCommand(base, &cmd);

            if (Cy_SD_Host_SUCCESS == ret)
            {
                ret = SdcmdRxData(base, &dataConfig);
            }
            (void)Cy_SD_Host_PollCmdLineFree(base);
            (void)Cy_SD_Host_PollDataLineFree(base);
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetOcr
****************************************************************************//**
*
*  Reads the Operating Condition Register (OCR) register from the card.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return uint32_t
*     The OCR regsiter.
*
* \note For combo cards the function returns the OCR regsiter for IO portion only.
*
*******************************************************************************/
uint32_t Cy_SD_Host_GetOcr(SDHC_Type *base, cy_stc_sd_host_context_t *context)
{
    uint32_t ocr = 0uL;

    if (CY_SD_HOST_SD == context->cardType)
    {
        (void)Cy_SD_Host_OpsSdSendOpCond(base, &ocr, 0u, context);
    }
    else if (CY_SD_HOST_EMMC == context->cardType)
    {
        (void)Cy_SD_Host_MmcOpsSendOpCond(base, &ocr, 0u, context);
    }
    else if ((CY_SD_HOST_SDIO == context->cardType) ||
             (CY_SD_HOST_COMBO == context->cardType))
    {
        (void)Cy_SD_Host_OpsSdioSendOpCond(base, &ocr, 0u);
    }
    else
    {
        /* Invalid card. */
    }

    return ocr;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetCid
****************************************************************************//**
*
*  Returns Card IDentification (CID) Register contents.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param cid
*     The pointer to where to store CID register.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_GetCid(SDHC_Type *base,
                                         uint32_t *cid)
{
    cy_stc_sd_host_cmd_config_t cmd;
    cy_en_sd_host_status_t      ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t                    i;
    uint32_t                    response[CY_SD_HOST_RESPONSE_SIZE] = { 0u };

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != cid))
    {
        cmd.commandIndex    = CY_SD_HOST_SD_CMD2;
        cmd.commandArgument = 0u;
        cmd.dataPresent     = false;
        cmd.enableAutoResponseErrorCheck = false;
        cmd.respType        = CY_SD_HOST_RESPONSE_LEN_136;
        cmd.enableCrcCheck  = true;
        cmd.enableIdxCheck  = false;
        cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

        ret = Cy_SD_Host_SendCommand(base, &cmd);

        if (Cy_SD_Host_SUCCESS == ret)
        {
            (void)Cy_SD_Host_GetResponse(base, (uint32_t *)response, true);

            /* Get CID from the response. */
            for ( i = 0u; i < CY_SD_HOST_CID_SIZE; i++ )
            {
                cid[i] = *((uint32_t *)(response + i));
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetCsd
****************************************************************************//**
*
*  Returns the Card Specific Data (CSD) register contents.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *csd
*     The pointer to where to store the CSD register.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_GetCsd(SDHC_Type *base,
                                         uint32_t *csd,
                                         cy_stc_sd_host_context_t *context)
{
    cy_stc_sd_host_cmd_config_t   cmd;
    cy_en_sd_host_status_t        ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t                      response[CY_SD_HOST_RESPONSE_SIZE] = { 0u };
    uint32_t                      numSector;
    uint32_t                      cSize;
    uint32_t                      cSizeMult;
    uint32_t                      readBlLen;

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != csd) && (NULL != context))
    {
        cmd.commandIndex    = CY_SD_HOST_SD_CMD9;
        cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
        cmd.dataPresent     = false;
        cmd.enableAutoResponseErrorCheck = false;
        cmd.respType        = CY_SD_HOST_RESPONSE_LEN_136;
        cmd.enableCrcCheck  = true;
        cmd.enableIdxCheck  = false;
        cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

        ret = Cy_SD_Host_SendCommand(base, &cmd);

        if (ret == Cy_SD_Host_SUCCESS)
        {
            (void)Cy_SD_Host_GetResponse(base, (uint32_t *)response, true);

            (void)memcpy(csd, response, sizeof(response));
            (void)memcpy(context->csd, response, sizeof(response));

            if (CY_SD_HOST_SDHC == context->cardCapacity) /* High Capacity */
            {
                cSize = (response[1] & CY_SD_HOST_CSD_V2_C_SIZE_MASK) >>
                         CY_SD_HOST_CSD_V2_C_SIZE_POS;

                numSector = (cSize + 1u) << CY_SD_HOST_CSD_V2_SECTOR_MULT;
            }
            else /* Standard Capacity */
            {
                cSize =  (response[2] & CY_SD_HOST_CSD_V1_C_SIZE_MSB_MASK) <<
                         CY_SD_HOST_CSD_V1_C_SIZE_MSB_MULT;    /* C_SIZE3 */
                cSize +=((response[1] & CY_SD_HOST_CSD_V1_C_SIZE_ISB_MASK) >>
                         CY_SD_HOST_CSD_V1_C_SIZE_ISB_POS) <<
                         CY_SD_HOST_CSD_V1_C_SIZE_ISB_MULT;    /* C_SIZE2 */
                cSize += (response[1] & CY_SD_HOST_CSD_V1_C_SIZE_LSB_MASK) >>
                          CY_SD_HOST_CSD_V1_C_SIZE_LSB_POS;    /* C_SIZE1 */

                cSizeMult = ((response[1] & CY_SD_HOST_CSD_V1_C_SIZE_MULT_MASK) >>
                             CY_SD_HOST_CSD_V1_C_SIZE_MULT_POS);

                readBlLen = (response[2] & CY_SD_HOST_CSD_V1_READ_BL_LEN_MASK) >>
                             CY_SD_HOST_CSD_V1_READ_BL_LEN_POS; /* READ_BL_LEN */

                numSector = (cSize + 1u) << (cSizeMult + 2u);

                if (CY_SD_HOST_CSD_V1_BL_LEN_1024 == readBlLen)
                {
                    numSector *= CY_SD_HOST_CSD_V1_1024_SECT_FACTOR;
                }
                else if (CY_SD_HOST_CSD_V1_BL_LEN_2048 == readBlLen)
                {
                    numSector *= CY_SD_HOST_CSD_V1_2048_SECT_FACTOR;
                }
                else
                {
                    /* Block length = 512 bytes (readBlLen = 9u) */
                }
            }

            context->maxSectorNum = numSector;
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetExtCsd
****************************************************************************//**
*
*  Returns EXTCSD Register contents. This is only for EMMC cards.
*  There are 512 bytes of data that are read.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *extCsd
*     The pointer to where to store the EXTCSD register.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_GetExtCsd(SDHC_Type *base, uint32_t *extCsd,
                                                   cy_stc_sd_host_context_t *context)
{
    cy_stc_sd_host_cmd_config_t  cmd;
    cy_en_sd_host_status_t       ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    cy_stc_sd_host_data_config_t dataConfig;

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != extCsd) && (NULL != context))
    {
        cmd.commandIndex    = CY_SD_HOST_MMC_CMD8;
        cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
        cmd.dataPresent     = true;
        cmd.enableAutoResponseErrorCheck = false;
        cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
        cmd.enableCrcCheck  = true;
        cmd.enableIdxCheck  = true;
        cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;

        dataConfig.blockSize            = CY_SD_HOST_BLOCK_SIZE;
        dataConfig.numberOfBlock        = 1u;
        dataConfig.enableDma            = false;
        dataConfig.autoCommand          = CY_SD_HOST_AUTO_CMD_NONE;
        dataConfig.read                 = true;
        dataConfig.data                 = extCsd;
        dataConfig.dataTimeout          = CY_SD_HOST_MAX_TIMEOUT;
        dataConfig.enableIntAtBlockGap  = false;
        dataConfig.enReliableWrite      = false;

        (void)Cy_SD_Host_InitDataTransfer(base, &dataConfig, context);

        ret = Cy_SD_Host_SendCommand(base, &cmd);

        if (Cy_SD_Host_SUCCESS == ret)
        {
            ret = SdcmdRxData(base, &dataConfig);
        }
        (void)Cy_SD_Host_PollCmdLineFree(base);
        (void)Cy_SD_Host_PollDataLineFree(base);

        if (Cy_SD_Host_SUCCESS == ret)
        {
            context->maxSectorNum = extCsd[CY_SD_HOST_EXTCSD_SEC_COUNT];

            context->cardCapacity = CY_SD_HOST_EMMC_LESS_2G;

            /* Check if the eMMC capacity is greater than 2GB */
            if ((CY_SD_HOST_MMC_LEGACY_SIZE_BYTES/CY_SD_HOST_BLOCK_SIZE) <
                 context->maxSectorNum)
            {
                context->cardCapacity = CY_SD_HOST_EMMC_GREATER_2G;
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetRca
****************************************************************************//**
*
*  Reads the Relative Card Address (RCA) register from the card.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return uint32_t
*     The RCA register.
*
*******************************************************************************/
uint32_t Cy_SD_Host_GetRca(SDHC_Type *base)
{
    cy_stc_sd_host_cmd_config_t cmd;
    uint32_t                    response = 0u;

    cmd.commandIndex    = CY_SD_HOST_SD_CMD3;
    cmd.commandArgument = 0u;
    cmd.dataPresent     = false;
    cmd.enableAutoResponseErrorCheck = false;
    cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48;
    cmd.enableCrcCheck  = true;
    cmd.enableIdxCheck  = true;
    cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;
        
    (void)Cy_SD_Host_SendCommand(base, &cmd);

    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);
        
    (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);

    return (response >> CY_SD_HOST_RCA_SHIFT);
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetScr
****************************************************************************//**
*
*  Returns the SD Card Configuration Register (SCR) Register contents.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *scr
*     The pointer to where to store the SCR register.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_GetScr(SDHC_Type *base,
                                         uint32_t *scr,
                                         cy_stc_sd_host_context_t const *context)
{
    cy_stc_sd_host_cmd_config_t  cmd;
    cy_en_sd_host_status_t       ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    cy_stc_sd_host_data_config_t dataConfig;

    /* Check for NULL pointer */
    if ((NULL != base) && (NULL != scr) && (NULL != context))
    {
        ret = Cy_SD_Host_OpsSendAppCmd(base, context);
        if (Cy_SD_Host_SUCCESS == ret)
        {
            cmd.commandIndex    = CY_SD_HOST_SD_ACMD51;
            cmd.commandArgument = context->RCA << CY_SD_HOST_RCA_SHIFT;
            cmd.dataPresent     = true;
            cmd.enableAutoResponseErrorCheck = false;
            cmd.respType        = CY_SD_HOST_RESPONSE_LEN_48B;
            cmd.enableCrcCheck  = true;
            cmd.enableIdxCheck  = true;
            cmd.cmdType         = CY_SD_HOST_CMD_NORMAL;
            
            dataConfig.blockSize           = CY_SD_HOST_SCR_BLOCKS;
            dataConfig.numberOfBlock       = 1u;
            dataConfig.enableDma           = false;
            dataConfig.autoCommand         = CY_SD_HOST_AUTO_CMD_NONE;
            dataConfig.read                = true;
            dataConfig.data                = (uint32_t *)scr;
            dataConfig.dataTimeout         = CY_SD_HOST_MAX_TIMEOUT;
            dataConfig.enableIntAtBlockGap = false;
            dataConfig.enReliableWrite     = false;

            (void)Cy_SD_Host_InitDataTransfer(base, &dataConfig, context);

            ret = Cy_SD_Host_SendCommand(base, &cmd);

            if (Cy_SD_Host_SUCCESS == ret)
            {
                ret = SdcmdRxData(base, &dataConfig);
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_ErrorReset
****************************************************************************//**
*
*  Checks for error event and resets it. Then resets the CMD line.
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
static void Cy_SD_Host_ErrorReset(SDHC_Type *base)
{
    uint32_t intError; /* The error events mask. */

    intError = Cy_SD_Host_GetErrorInterruptStatus(base);

    /* Check the error event */
    if (0u < intError)
    {
        /* Clear the error event */
        Cy_SD_Host_ClearErrorInterruptStatus(base, intError);

        Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_CMD_LINE);
    }
}


/*******************************************************************************
* Function Name: Cy_SD_Host_NormalReset
****************************************************************************//**
*
*  Checks for normal event and resets it.
*
* \param *base
*     The SD host registers structure pointer.
*
*******************************************************************************/
static void Cy_SD_Host_NormalReset(SDHC_Type *base)
{
    uint32_t intNormal; /* The normal events mask. */

    intNormal = Cy_SD_Host_GetNormalInterruptStatus(base);

    /* Check the normal event */
    if (0u < intNormal)
    {
        /* Clear the normal event */
        Cy_SD_Host_ClearNormalInterruptStatus(base, intNormal);
    }
}


/*******************************************************************************
* Function Name: Cy_SD_Host_VoltageCheck
****************************************************************************//**
*
*  resets the card (CMD0) and checks the voltage (CMD8).
*
* \param *base
*     The SD host registers structure pointer.
*
* \return The CMD8 valid flag (f8).
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SD_Host_VoltageCheck(SDHC_Type *base)
{
    bool f8 = false; /* The CMD8 valid flag. */
    cy_en_sd_host_status_t ret;
    uint32_t response = 0u;  /* The CMD response. */
    uint32_t retry = CY_SD_HOST_VOLTAGE_CHECK_RETRY;

    while (retry > 0u)
    {
        /* Reset Card (CMD0). */
        ret = Cy_SD_Host_OpsGoIdle(base);  /* The Idle state. */
        
        /* Software reset for the CMD line */
        Cy_SD_Host_SoftwareReset(base, CY_SD_HOST_RESET_CMD_LINE);

        /* Voltage check (CMD8). */
        ret = Cy_SD_Host_OpsSendIfCond(base,
                                       CY_SD_HOST_CMD8_VHS_27_36 |
                                       CY_SD_HOST_CMD8_CHECK_PATTERN,
                                       false);

        /* Check the response. */
        (void)Cy_SD_Host_GetResponse(base,
                                     (uint32_t *)&response,
                                     false);

        /* Check the pattern. */
        if (CY_SD_HOST_CMD8_CHECK_PATTERN == (response &
                                              CY_SD_HOST_CMD8_PATTERN_MASK))
        {
            /* The pattern is valid. */
            f8 = true;
        }
        else if (CY_SD_HOST_VOLTAGE_CHECK_RETRY == retry)
        {
           /* CMD8 fails. Retry one more time */
        }
        else
        {
            /* The unusable card or the SDIO card. */
            ret = Cy_SD_Host_ERROR_UNUSABLE_CARD;
        }
        
        if ((Cy_SD_Host_ERROR_TIMEOUT == ret) || (f8))
        {
            /* The pattern is valid or voltage mismatch (No response). */
            break;  
        }
        
        retry--;
    }

    if (Cy_SD_Host_SUCCESS != ret)   /* The Idle state. */
    {
        /* Reset the error and the CMD line for the case of the SDIO card. */
        Cy_SD_Host_ErrorReset(base);
        Cy_SD_Host_NormalReset(base);
    }

    return f8;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_IoOcr
****************************************************************************//**
*
*  Sends CMD5 to get IO OCR. Checks if IO is present (sdio),
*   if the memory is present (mp) and if 1.8 signaling can be supported (s18A).
*
* \param *base
*     The SD host registers structure pointer.
*
* \param lowVoltageSignaling
*     The lowVoltageSignaling flag.
*
* \param *s18A
*     The S18A flag (1.8 signaling support).
*
* \param *sdio
*     The IO flag (the number of IO functions).
*
* \param *mp
*     The MEM flag (memory support).
*
* \param *ocr
*     The Operation Condition register (OCR).
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_IoOcr(SDHC_Type *base,
                                      bool lowVoltageSignaling,
                                      uint32_t *s18A,
                                      uint32_t *sdio,
                                      bool *mp,                                   
                                      uint32_t *ocr)
{
    cy_en_sd_host_status_t ret;
    uint32_t retry;

    /* Get IO OCR (CMD5) */
    ret = Cy_SD_Host_OpsSdioSendOpCond(base, ocr, 0u);

    /* Get the number of IO functions. */
    *sdio = *ocr & CY_SD_HOST_CMD5_IO_NUM_MASK;

    if (0u < *sdio)
    {
        if (true == lowVoltageSignaling)
        {
            /* Set the OCR to change the signal
            * voltage to 1.8 V for the UHS-I mode.
            */
            *ocr = (*ocr & CY_SD_HOST_IO_OCR_MASK) |
                           CY_SD_HOST_ACMD41_S18R;
        }

        retry = CY_SD_HOST_RETRY_TIME;
        while (retry > 0u)
        {
            /* Set S18R and the voltage window in IO OCR (CMD5) */
            ret = Cy_SD_Host_OpsSdioSendOpCond(base,
                                              ocr,
                                              *ocr);

            /* Check the IO power up status */
            if (Cy_SD_Host_SUCCESS == ret)
            {
                if (CY_SD_HOST_IO_OCR_C == (*ocr & CY_SD_HOST_IO_OCR_C))
                {
                    /* The SDIO card supports 1.8 signaling */
                    *s18A = 1u;
                }

                if(CY_SD_HOST_CMD5_MP_MASK == (*ocr & CY_SD_HOST_CMD5_MP_MASK))
                {
                    /* MP = 1. (The memory is supported.) */
                    *mp = true;
                }

                /* IO > 0. */
                break;
            }
            
            Cy_SysLib_DelayUs(CY_SD_HOST_SDIO_CMD5_TIMEOUT_MS); /* 1 sec timeout. */
            retry--;
        }

        if (Cy_SD_Host_SUCCESS != ret)
        {
            /* IO = 0. */
            *sdio = 0u;
        }
    }
    else
    {
        /* IO = 0. We have the SD memory card. Reset errors. */
        Cy_SD_Host_ErrorReset(base);
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SdOcr
****************************************************************************//**
*
*  Sends ACMD41 to get SD OCR.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param lowVoltageSignaling
*     The lowVoltageSignaling flag.
*
* \param *s18A
*     The S18A flag (1.8 signaling support).
*
* \param *mp
*     The MEM flag (memory support).
*
* \param f8
*     The CMD8 flag.
*
* \param *ocr
*     The Operation Condition register (OCR).
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
__STATIC_INLINE cy_en_sd_host_status_t Cy_SD_Host_SdOcr(SDHC_Type *base,
                                      bool lowVoltageSignaling,
                                      uint32_t *s18A,
                                      bool *mp,
                                      bool f8,
                                      uint32_t *ocr,
                                      cy_stc_sd_host_context_t *context)
{
    cy_en_sd_host_status_t ret;
    uint32_t response = 0u;  /* The CMD response. */
    uint32_t retry;
    uint32_t cmdArgument;

    /* Get OCR (ACMD41). Voltage window = 0. */
    ret = Cy_SD_Host_OpsSdSendOpCond(base,
                                     ocr,
                                     0x00000000u,
                                     context);

    if (Cy_SD_Host_SUCCESS == ret)
    {
        /* Set voltage window from 2.7 to 3.6 V.  */
        cmdArgument = CY_SD_HOST_ACMD41_VOLTAGE;

        if (f8)
        {
            /* Set the SDHC supported bit.*/
            cmdArgument |= CY_SD_HOST_ACMD41_HCS;

            if (true == lowVoltageSignaling)
            {
                /* Set the 1.8 V request bit.*/
                cmdArgument |= CY_SD_HOST_ACMD41_S18R;
            }
        }

        /* Set OCR (ACMD41). */
        retry = CY_SD_HOST_RETRY_TIME;
        while (retry > 0u)
        {
            ret = Cy_SD_Host_OpsSdSendOpCond(base,
                                             ocr,
                                             cmdArgument,
                                             context);
            if (Cy_SD_Host_SUCCESS == ret)
            {
                break;
            }
            Cy_SysLib_DelayUs(CY_SD_HOST_ACMD41_TIMEOUT_MS);
            retry--;
        }

        /* Check the response. */
        (void)Cy_SD_Host_GetResponse(base,
                                    (uint32_t *)&response,
                                    false);

        if (0u == (response & CY_SD_HOST_OCR_CAPACITY_MASK))
        {
            /* The SDSC card. */
            context->cardCapacity = CY_SD_HOST_SDSC;
        }

        /* Check S18A. */
        if (CY_SD_HOST_OCR_S18A == (response &
                                    CY_SD_HOST_OCR_S18A))
        {
            /* The SD card supports 1.8 signaling */
            *s18A |= 1u;
        }

        if (Cy_SD_Host_SUCCESS == ret)
        {
            /* MP = 1. (The memory is supported.) */
            *mp = true;
        }
        else
        {
            /* MP = 0. (The memory is not supported.) */
            *mp = false;
        }
    }
    else if (false == f8)
    {
        /* Not SD card */
        ret = Cy_SD_Host_ERROR_UNUSABLE_CARD;
        context->cardType = CY_SD_HOST_UNUSABLE;
    }
    else
    {
        /* The card is not present or busy */
    }
    
    Cy_SysLib_DelayUs(CY_SD_HOST_NCC_MIN_US);

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_Enable
****************************************************************************//**
*
*  Enables the SD host block.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t  Cy_SD_Host_Enable(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t retry;

    /* Check for NULL pointer */
    if (NULL != base)
    {
        /* Enable the SDHC block. */
        SDHC_WRAP_CTL(base) = _CLR_SET_FLD32U(SDHC_WRAP_CTL(base),
                                        SDHC_WRAP_CTL_ENABLE,
                                        1u);

        retry = CY_SD_HOST_RETRY_TIME;

        /* Enable the Internal clock. */
        SDHC_CORE_CLK_CTRL_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base),
                                        SDHC_CORE_CLK_CTRL_R_INTERNAL_CLK_EN,
                                        1u);

        while((true != _FLD2BOOL(SDHC_CORE_CLK_CTRL_R_INTERNAL_CLK_STABLE, SDHC_CORE_CLK_CTRL_R(base)))
              && (retry > 0u))
        {
            /* Wait for stable Internal Clock . */
            Cy_SysLib_DelayUs(CY_SD_HOST_INT_CLK_STABLE_TIMEOUT_MS);
        }

        ret = Cy_SD_Host_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_Disable
****************************************************************************//**
*
*  Disables the SD host block.
*
* \param *base
*     The SD host registers structure pointer.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_Disable(SDHC_Type *base)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;

    /* Check for NULL pointer */
    if (NULL != base)
    {
        Cy_SD_Host_DisableSdClk(base);

        /* Disable the Internal clock. */
        SDHC_CORE_CLK_CTRL_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base),
                                        SDHC_CORE_CLK_CTRL_R_INTERNAL_CLK_EN,
                                        0u);

        /* Disable the SDHC block. */
        SDHC_WRAP_CTL(base) = _CLR_SET_FLD32U(SDHC_WRAP_CTL(base),
                                        SDHC_WRAP_CTL_ENABLE,
                                        0u);

        ret = Cy_SD_Host_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetSdClkDiv
****************************************************************************//**
*
*  Changes the speed of the SD bus. This function should be called along
*  with Cy_SD_Host_SetHostSpeedMode to configure the bus correctly.
*
* \note
*  The divider is clocked from clock peri. In order to determine
*  the SD bus speed divide the clock peri by the divider value passed
*  in this function. The divider value is 2^clkDiv.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param clkDiv
*     The clock divider for the SD clock.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SetSdClkDiv(SDHC_Type *base, uint16_t clkDiv)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;

    /* Check for NULL pointer */
    if (NULL != base)
    {
        /* Set clock division the first LSB 8 bits */
        SDHC_CORE_CLK_CTRL_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base),
                                      SDHC_CORE_CLK_CTRL_R_FREQ_SEL,
                                      ((uint32_t)clkDiv & CY_SD_HOST_FREQ_SEL_MSK));

        /* Set clock division the upper 2 bits */
        SDHC_CORE_CLK_CTRL_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base),
                                      SDHC_CORE_CLK_CTRL_R_UPPER_FREQ_SEL,
                                      (((uint32_t)clkDiv >> CY_SD_HOST_UPPER_FREQ_SEL_POS) &
                                        CY_SD_HOST_UPPER_FREQ_SEL_MSK));

        ret = Cy_SD_Host_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetHostBusWidth
****************************************************************************//**
*
*  Only changes the bus width on the host side.
*  It doesn't change the bus width on the card side.
*  To change bus width on the card call Cy_SD_Host_SetBusWidth().
*
* \param *base
*     The SD host registers structure pointer.
*
* \param width
*     The width of the data bus.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SetHostBusWidth(SDHC_Type *base,
                                                  cy_en_sd_host_bus_width_t width)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;

    /* Check for NULL pointer */
    if (NULL != base)
    {                        
        SDHC_CORE_HOST_CTRL1_R(base) = (SDHC_CORE_HOST_CTRL1_R(base) &
                                       (uint8_t)~(SDHC_CORE_HOST_CTRL1_R_EXT_DAT_XFER_Msk |
                                                 SDHC_CORE_HOST_CTRL1_R_DAT_XFER_WIDTH_Msk)) |
                                        (_BOOL2FLD(SDHC_CORE_HOST_CTRL1_R_EXT_DAT_XFER, (CY_SD_HOST_BUS_WIDTH_8_BIT == width)) |
                                         _BOOL2FLD(SDHC_CORE_HOST_CTRL1_R_DAT_XFER_WIDTH, (CY_SD_HOST_BUS_WIDTH_4_BIT == width)));

        ret = Cy_SD_Host_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetHostSpeedMode
****************************************************************************//**
*
*  Only updates the host register to indicate bus speed mode.
*  This function doesn't change the speed on the bus, or change
*  anything in the card.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param speedMode
*     The bus speed mode.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SetHostSpeedMode(SDHC_Type *base,
                                                  cy_en_sd_host_bus_speed_mode_t speedMode)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_SUCCESS;
    uint32_t               ultraHighSpeed;

    /* Check for NULL pointer */
    if (NULL != base)
    {
        /* UHS Mode/eMMC Speed Mode Select */
        switch (speedMode)
        {
            case CY_SD_HOST_BUS_SPEED_EMMC_LEGACY:
            case CY_SD_HOST_BUS_SPEED_DEFAULT:
            case CY_SD_HOST_BUS_SPEED_SDR12_5:
                ultraHighSpeed = CY_SD_HOST_SDR12_SPEED; /* Max clock = 25 MHz */
                break;
            case CY_SD_HOST_BUS_SPEED_EMMC_HIGHSPEED_SDR:
            case CY_SD_HOST_BUS_SPEED_SDR25:
            case CY_SD_HOST_BUS_SPEED_HIGHSPEED:
                ultraHighSpeed = CY_SD_HOST_SDR25_SPEED; /* Max clock = 50 MHz */
                break;
            case CY_SD_HOST_BUS_SPEED_SDR50:
                ultraHighSpeed = CY_SD_HOST_SDR50_SPEED; /* Max clock = 100 MHz */
                break;
            default:
                ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
                break;
        }

        if (Cy_SD_Host_ERROR_INVALID_PARAMETER != ret)
        {
            SDHC_CORE_HOST_CTRL1_R(base) = (uint8_t)_CLR_SET_FLD8U(SDHC_CORE_HOST_CTRL1_R(base),
                                          SDHC_CORE_HOST_CTRL1_R_HIGH_SPEED_EN,
                                          ((CY_SD_HOST_BUS_SPEED_HIGHSPEED == speedMode) ? 1u : 0u));

            SDHC_CORE_HOST_CTRL2_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_HOST_CTRL2_R(base),
                                          SDHC_CORE_HOST_CTRL2_R_UHS_MODE_SEL,
                                          ultraHighSpeed);
        }
    }
    else
    {
        ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;  
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SetBusSpeedMode
****************************************************************************//**
*
*  Negotiates with the card to change the bus speed mode of the card
*  and the host. It doesn't change the SD clock frequency that must be done
*  separately.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param speedMode
*     The bus speed mode.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SetBusSpeedMode(SDHC_Type *base,
                                                  cy_en_sd_host_bus_speed_mode_t speedMode,
                                                  cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t  ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    uint32_t                highSpeedValue = CY_SD_HOST_DEFAULT_SPEED;
    uint32_t                cmdArgument = 0u;
    uint32_t                response[CY_SD_HOST_RESPONSE_SIZE] = { 0u };

    /* Check for NULL pointer */
    if (NULL != base)
    {
        CY_ASSERT_L3(CY_SD_HOST_IS_SPEED_MODE_VALID(speedMode));

        /* 1. Does the card support memory? */
        if ((CY_SD_HOST_SD == context->cardType) || 
            (CY_SD_HOST_EMMC == context->cardType) || 
            (CY_SD_HOST_COMBO == context->cardType))
        {
            /* 2. Change Bus Speed Mode: Issue CMD6 with mode 1 */
            if ((CY_SD_HOST_BUS_SPEED_DEFAULT != speedMode) &&
                (CY_SD_HOST_BUS_SPEED_EMMC_LEGACY != speedMode))
            {
                highSpeedValue = CY_SD_HOST_HIGH_SPEED;
            }

            if (CY_SD_HOST_SD == context->cardType)
            {
                /* Set the mode bit to 1 and select the default function */
                cmdArgument = (1uL << CY_SD_HOST_SWITCH_FUNCTION_BIT);

                /* Set the High Speed/SDR25 bit */
                cmdArgument |= highSpeedValue & 0xFuL;
            }
            else
            {
                cmdArgument = (CY_SD_HOST_EMMC_ACCESS_WRITE_BYTE << CY_SD_HOST_EMMC_CMD6_ACCESS_OFFSET) |
                                  (CY_SD_HOST_EMMC_HS_TIMING_ADDR << CY_SD_HOST_EMMC_CMD6_IDX_OFFSET) |
                                  (highSpeedValue << CY_SD_HOST_EMMC_CMD6_VALUE_OFFSET) |
                                  (0x0uL << CY_SD_HOST_EMMC_CMD6_CMD_SET_OFFSET);
            }

            ret = Cy_SD_Host_OpsSwitchFunc(base, cmdArgument);
        }

        /* 5. Is SDIO Supported? */
        if ((CY_SD_HOST_SDIO == context->cardType) || 
            (CY_SD_HOST_COMBO == context->cardType))
        {
            /* 6. Change Bus Speed Mode: Set EHS or BSS[2:0] in CCCR */
            ret = Cy_SD_Host_OpsSendIoRwDirectCmd(base,
                                                  0u,
                                                  0u,
                                                  0u,
                                                  CY_SD_HOST_CCCR_SPEED_CONTROL,
                                                  0u);

            (void)Cy_SD_Host_GetResponse(base, (uint32_t *)response, false);

            /* Check the SHS bit - the High Speed mode operation ability  */
            if (0u != (response[0] & CY_SD_HOST_CCCR_SPEED_SHS_MASK))
            {
                switch (speedMode)
                {
                    case CY_SD_HOST_BUS_SPEED_DEFAULT:
                    case CY_SD_HOST_BUS_SPEED_SDR12_5:
                        /* BSS0 = 0, BSS1 = 0  */
                        break;
                    case CY_SD_HOST_BUS_SPEED_SDR25:
                    case CY_SD_HOST_BUS_SPEED_HIGHSPEED:
                        response[0] |= CY_SD_HOST_CCCR_SPEED_BSS0_MASK;
                        break;
                    case CY_SD_HOST_BUS_SPEED_SDR50:
                        response[0] |= CY_SD_HOST_CCCR_SPEED_BSS1_MASK;
                        break;
                    default:
                        ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
                        break;
                }

                /* Set the EHS bit to enable the High Speed mode */
                if (Cy_SD_Host_SUCCESS == ret)
                {
                    highSpeedValue = response[0] & (CY_SD_HOST_CCCR_SPEED_BSS0_MASK |
                                             CY_SD_HOST_CCCR_SPEED_BSS1_MASK |
                                             CY_SD_HOST_CCCR_SPEED_SHS_MASK);

                    ret = Cy_SD_Host_OpsSendIoRwDirectCmd(base,
                                                          1u,
                                                          0u,
                                                          0u,
                                                          CY_SD_HOST_CCCR_SPEED_CONTROL,
                                                          response[0]);

                    (void)Cy_SD_Host_GetResponse(base, (uint32_t *)response, false);

                    response[0] = response[0] & (CY_SD_HOST_CCCR_SPEED_BSS0_MASK |
                                   CY_SD_HOST_CCCR_SPEED_BSS1_MASK |
                                   CY_SD_HOST_CCCR_SPEED_SHS_MASK);

                    if(highSpeedValue != response[0])
                    {
                        ret = Cy_SD_Host_ERROR_UNINITIALIZED;
                    }
                }
            }
            else
            {
                /* The card can operate in the High Speed mode only. */
            }
        }

        /* 3 and 7. Changed Bus Speed Mode Successfully  */
        if (Cy_SD_Host_SUCCESS == ret)
        {
            /* 4 and 8. Set the same bus speed mode in the host controller */
            ret = Cy_SD_Host_SetHostSpeedMode(base, speedMode);
        }

    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SelBusVoltage
****************************************************************************//**
*
*  Negotiates with the card to change the bus signaling level to 1.8V.
*
* \note The host needs to change the regulator supplying voltage to
*       the VDDIO of the SD block in order to operate at 1.8V.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param enable18VSignal
*     If true use 1.8V signaling otherwise use 3.3V signaling.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
* \note The SD card power supply sholud be disabled when this function
*  returns Cy_SD_Host_ERROR_UNUSABLE_CARD.
*
*******************************************************************************/
cy_en_sd_host_status_t Cy_SD_Host_SelBusVoltage(SDHC_Type *base,
                                                bool enable18VSignal,
                                                cy_stc_sd_host_context_t *context)
{
    cy_en_sd_host_status_t ret;
    uint32_t               retry;
    uint32_t               cmdArgument;
    uint32_t               ocr; /* The Operation Condition register. */
    uint32_t               response = 0u;

    /* Get OCR (ACMD41). Voltage window = 0. */
    ret = Cy_SD_Host_OpsSdSendOpCond(base, &ocr, 0x0u, context);

    if (Cy_SD_Host_SUCCESS == ret)
    {
        /* Set voltage window from 2.7 to 3.6 V.  */
        cmdArgument = CY_SD_HOST_ACMD41_VOLTAGE;

        if (CY_SD_HOST_SDHC == context->cardCapacity)
        {
            /* Set the SDHC supported bit.*/
            cmdArgument |= CY_SD_HOST_ACMD41_HCS;

            if (true == enable18VSignal)
            {
                /* Set the 1.8 V request bit.*/
                cmdArgument |= CY_SD_HOST_ACMD41_S18R;
            }
        }

        /* Set OCR (ACMD41). */
        retry = CY_SD_HOST_RETRY_TIME;
        while (retry > 0u)
        {
            ret = Cy_SD_Host_OpsSdSendOpCond(base, &ocr, cmdArgument, context);
            if (Cy_SD_Host_SUCCESS == ret)
            {
                break;
            }
            Cy_SysLib_DelayUs(CY_SD_HOST_ACMD41_TIMEOUT_MS);
            retry--;
        }

        /* Check the response. */
        (void)Cy_SD_Host_GetResponse(base, (uint32_t *)&response, false);

        if (0u == (response & CY_SD_HOST_OCR_CAPACITY_MASK))
        {
            context->cardCapacity = CY_SD_HOST_SDSC;
        }

        if ((Cy_SD_Host_SUCCESS == ret) && (enable18VSignal))
        {
            /* Check S18A. */
            if (CY_SD_HOST_OCR_S18A == (response & CY_SD_HOST_OCR_S18A))
            {
                /* Voltage switch (CMD11). */
                ret = Cy_SD_Host_OpsVoltageSwitch(base, context);
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetResponse
****************************************************************************//**
*
*  Receives response on the CMD line.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param *responsePtr
*     The pointer to response data.
*
* \param largeResponse
*     If true response is 136 bits, otherwise it is 32 bits.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t  Cy_SD_Host_GetResponse(SDHC_Type const *base,
                                               uint32_t *responsePtr,
                                               bool largeResponse)
{
    cy_en_sd_host_status_t  ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
    volatile uint32_t const *responseBaseAddr;
    uint8_t                 i;
    uint8_t                 responseLength = (true == largeResponse) ? CY_SD_HOST_RESPONSE_SIZE : 1u;

    if ((NULL != base) &&
        (NULL != responsePtr))
    {
        /* Get the Response Register 0 address */
        responseBaseAddr = &SDHC_CORE_RESP01_R(base);

        /* Read the largeResponse Response registers values */
        for (i = 0u; i < responseLength; i++)
        {
            *responsePtr = *responseBaseAddr;
            responsePtr++;
            responseBaseAddr++;
        }

        ret = Cy_SD_Host_SUCCESS;
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_InitDataTransfer
****************************************************************************//**
*
*  Initializes the SD block for a data transfer. It does not start a transfer.
*  To start a transfer call Cy_SD_Host_SendCommand() after calling this function.
*  If DMA is not used for Data transfer then the buffer needs to be filled
*  with data first if this is a write.
*
* \param *base
*     The SD host registers structure pointer.
*
* \param dataConfig
*     The pointer to the data transfer configuration structure.
*
* \param context
* The pointer to the context structure \ref cy_stc_sd_host_context_t allocated
* by the user. The structure is used during the SD host operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only SD host functions which do not require context will be used pass NULL
* as pointer to context.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
cy_en_sd_host_status_t  Cy_SD_Host_InitDataTransfer(SDHC_Type *base,
                                                    cy_stc_sd_host_data_config_t const *dataConfig,
                                                    cy_stc_sd_host_context_t const *context)
{
    cy_en_sd_host_status_t ret = Cy_SD_Host_SUCCESS;

    if ((NULL != base) && (NULL != dataConfig) && (NULL != dataConfig->data))
    {
        CY_ASSERT_L3(CY_SD_HOST_IS_AUTO_CMD_VALID(dataConfig->autoCommand));
        CY_ASSERT_L2(CY_SD_HOST_IS_TIMEOUT_VALID(dataConfig->dataTimeout));
        CY_ASSERT_L2(CY_SD_HOST_IS_BLK_SIZE_VALID(dataConfig->blockSize));

        if ((CY_SD_HOST_DMA_ADMA2_ADMA3 == context->dmaType) && (dataConfig->enableDma))
        {
            /* ADMA3 Integrated Descriptor Address */
            SDHC_CORE_ADMA_ID_LOW_R(base) = (uint32_t)dataConfig->data;
        }
        else
        {
            if (dataConfig->enableDma)
            {
                /* Set descriptor table of ADMA */
                if (CY_SD_HOST_DMA_SDMA == context->dmaType)
                {
                    /* Set 512K bytes SDMA Buffer Boundary */
                    SDHC_CORE_BLOCKSIZE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_BLOCKSIZE_R(base),
                                                  SDHC_CORE_BLOCKSIZE_R_SDMA_BUF_BDARY,
                                                  CY_SD_HOST_SDMA_BUF_BYTES_512K);

                    if (true == _FLD2BOOL(SDHC_CORE_HOST_CTRL2_R_HOST_VER4_ENABLE, SDHC_CORE_HOST_CTRL2_R(base)))
                    {
                        /* Data address */
                        SDHC_CORE_ADMA_SA_LOW_R(base) = (uint32_t)dataConfig->data;

                        /* Set block count */
                        SDHC_CORE_SDMASA_R(base) = dataConfig->numberOfBlock;
                    }
                    else
                    {
                        /* Data address */
                        SDHC_CORE_SDMASA_R(base) = (uint32_t)dataConfig->data;
                    }
                }
                else
                {
                    /* Data address */
                    SDHC_CORE_ADMA_SA_LOW_R(base) = (uint32_t)dataConfig->data;
                }
            }
            else
            {
                /* Set block count */
                SDHC_CORE_SDMASA_R(base) = dataConfig->numberOfBlock;
            }

            /* Set multi-block or single block transfer */
            SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                          SDHC_CORE_XFER_MODE_R_MULTI_BLK_SEL,
                                          ((1u < dataConfig->numberOfBlock) ? 1u : 0u));

            /* Set data transfer direction */
            SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                          SDHC_CORE_XFER_MODE_R_DATA_XFER_DIR,
                                          ((true == dataConfig->read) ? 1u : 0u));

            /* Set block size */
            SDHC_CORE_BLOCKSIZE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_BLOCKSIZE_R(base),
                                          SDHC_CORE_BLOCKSIZE_R_XFER_BLOCK_SIZE,
                                          dataConfig->blockSize);

            /* Set block count */
            SDHC_CORE_BLOCKCOUNT_R(base) = (uint16_t)dataConfig->numberOfBlock;


            /* Set block count enable */
            SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                              SDHC_CORE_XFER_MODE_R_BLOCK_COUNT_ENABLE,
                                              1u);

            /* Enable DMA or not */
            SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                          SDHC_CORE_XFER_MODE_R_DMA_ENABLE,
                                          ((true == dataConfig->enableDma) ? 1u : 0u));

            /* Set an interrupt at the block gap */
            SDHC_CORE_BGAP_CTRL_R(base) = (uint8_t)_CLR_SET_FLD8U(SDHC_CORE_BGAP_CTRL_R(base),
                                          SDHC_CORE_BGAP_CTRL_R_INT_AT_BGAP,
                                          ((true == dataConfig->enableIntAtBlockGap) ? 1u : 0u));

            /* Set data timeout time (Base clock*2^27) */
            SDHC_CORE_TOUT_CTRL_R(base) = _CLR_SET_FLD8U(SDHC_CORE_TOUT_CTRL_R(base),
                                                    SDHC_CORE_TOUT_CTRL_R_TOUT_CNT,
                                                    dataConfig->dataTimeout);

            if (true == _FLD2BOOL(SDHC_CORE_XFER_MODE_R_RESP_ERR_CHK_ENABLE, SDHC_CORE_XFER_MODE_R(base)))
            {
                SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                              SDHC_CORE_XFER_MODE_R_RESP_INT_DISABLE,
                                              1u);
                SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                              SDHC_CORE_XFER_MODE_R_RESP_TYPE,
                                              1u);
            }

            /* Reliable write setting */
            if ((CY_SD_HOST_EMMC == context->cardType) && (dataConfig->enReliableWrite))
            {
                ret = Cy_SD_Host_OpsSetBlockCount(base,
                                                  dataConfig->enReliableWrite,
                                                  dataConfig->numberOfBlock);
            }

            /* Auto command setting */
            switch (dataConfig->autoCommand)
            {
                case CY_SD_HOST_AUTO_CMD_NONE:
                    SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                                    SDHC_CORE_XFER_MODE_R_AUTO_CMD_ENABLE,
                                                    0u);
                    break;
                case CY_SD_HOST_AUTO_CMD_12:
                    SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                                    SDHC_CORE_XFER_MODE_R_AUTO_CMD_ENABLE,
                                                    1u);
                    break;
                case CY_SD_HOST_AUTO_CMD_23:
                    SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                                    SDHC_CORE_XFER_MODE_R_AUTO_CMD_ENABLE,
                                                    2u);
                    break;
                case CY_SD_HOST_AUTO_CMD_AUTO:
                    SDHC_CORE_XFER_MODE_R(base) = _CLR_SET_FLD16U(SDHC_CORE_XFER_MODE_R(base),
                                                    SDHC_CORE_XFER_MODE_R_AUTO_CMD_ENABLE,
                                                    3u);
                    break;
                default:
                    ret = Cy_SD_Host_ERROR_INVALID_PARAMETER;
                    break;
            }
        }
    }

    return ret;
}


/*******************************************************************************
* Function Name: Cy_SD_Host_SoftwareReset
****************************************************************************//**
*
*  Issues the software reset command to the SD card.
*
* \param *base
*     SD host registers structure pointer.
*
* \param reset
*     The reset type.
*
* \return \ref cy_en_sd_host_status_t
*
*******************************************************************************/
void Cy_SD_Host_SoftwareReset(SDHC_Type *base,
                              cy_en_sd_host_reset_t reset)
{
    switch (reset)
    {
        case CY_SD_HOST_RESET_DATALINE:
            SDHC_CORE_SW_RST_R(base) = (uint8_t)_VAL2FLD(SDHC_CORE_SW_RST_R_SW_RST_DAT, 1u);

            while(false != _FLD2BOOL(SDHC_CORE_SW_RST_R_SW_RST_DAT, SDHC_CORE_SW_RST_R(base)))
            {
                /* Wait until reset finishes */
            }

            break;
        case CY_SD_HOST_RESET_CMD_LINE:
            SDHC_CORE_SW_RST_R(base) = (uint8_t)_VAL2FLD(SDHC_CORE_SW_RST_R_SW_RST_CMD, 1u);

            while(false != _FLD2BOOL(SDHC_CORE_SW_RST_R_SW_RST_CMD, SDHC_CORE_SW_RST_R(base)))
            {
                /* Wait until reset finishes */                  
            }

            break;
        case CY_SD_HOST_RESET_ALL:
            SDHC_CORE_SW_RST_R(base) = (uint8_t)_VAL2FLD(SDHC_CORE_SW_RST_R_SW_RST_ALL, 1u);

            while(false != _FLD2BOOL(SDHC_CORE_SW_RST_R_SW_RST_ALL, SDHC_CORE_SW_RST_R(base)))
            {
                /* Wait until reset finishes */  
            }
            
            /* Enable the Internal clock. */
            SDHC_CORE_CLK_CTRL_R(base) = (uint16_t)_CLR_SET_FLD16U(SDHC_CORE_CLK_CTRL_R(base),
                                            SDHC_CORE_CLK_CTRL_R_INTERNAL_CLK_EN,
                                            1u);

            while(true != _FLD2BOOL(SDHC_CORE_CLK_CTRL_R_INTERNAL_CLK_STABLE, SDHC_CORE_CLK_CTRL_R(base)))
            {
                /* Wait for stable Internal Clock . */
            }

            break;
        default:

            break;
    }
}


/*******************************************************************************
* Function Name: Cy_SD_Host_GetPresentState
****************************************************************************//**
*
*  Returns the values of the present state register.
*
* \param *base
*     SD host registers structure pointer.
*
* \return The value of the present state register.
*
*******************************************************************************/
uint32_t Cy_SD_Host_GetPresentState(SDHC_Type const *base)
{
    uint32_t ret;

    ret = SDHC_CORE_PSTATE_REG(base);

    return ret;
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSDHC */

/* [] END OF FILE */
