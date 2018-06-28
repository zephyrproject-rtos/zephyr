/***************************************************************************//**
* \file cy_smif_memslot.c
* \version 1.20
*
* \brief
*  This file provides the source code for the memory-level APIs of the SMIF driver.
*
* Note:
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_smif_memslot.h"

#ifdef CY_IP_MXSMIF

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************
*     Function Prototypes
***************************************/
static void Cy_SMIF_Memslot_XipRegInit(SMIF_DEVICE_Type volatile *dev,
                            cy_stc_smif_mem_config_t const * memCfg);


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_Init
****************************************************************************//**
*
* This function initializes the slots of the memory device in the SMIF
* configuration. After this initialization, the memory slave devices are
* automatically mapped into the PSoC memory map. The function needs the SMIF
* to be running in the memory mode to have the memory mapped into the PSoC
* address space. This function is typically called in the System initialization
* phase to initialize all the memory-mapped SMIF devices.
* This function only configures the memory device portion of the SMIF
* initialization and therefore assumes that the SMIF blocks initialization is
* achieved using Cy_SMIF_Init(). The cy_stc_smif_context_t context structure
* returned from Cy_SMIF_Init() is passed as a parameter to this function.
* This function calls the \ref Cy_SMIF_Memslot_SfdpDetect() function for each
* element of the \ref cy_stc_smif_mem_config_t memConfig array and fills 
* the memory parameters if the autoDetectSfdp field is enabled 
* in \ref cy_stc_smif_mem_config_t.
* The filled memConfig is a part of the 
* \ref cy_stc_smif_block_config_t * blockConfig structure. The function expects 
* that all the requirements of \ref Cy_SMIF_Memslot_SfdpDetect() is provided.
*
* \param base
* The address of the slave-slot device register to initialize.
*
* \param context
* The SMIF internal context structure of the block.
*
* \param blockConfig
* The configuration structure array that configures the SMIF memory device to be
* mapped into the PSoC memory map. \ref cy_stc_smif_mem_config_t
*
* \return The memory slot initialization status.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_BAD_PARAM
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_Init(SMIF_Type *base,
                            cy_stc_smif_block_config_t * const blockConfig,
                            cy_stc_smif_context_t *context)
{
    SMIF_DEVICE_Type volatile * device;
    cy_stc_smif_mem_config_t const * memCfg;
    uint32_t result = (uint32_t)CY_SMIF_BAD_PARAM;
    uint32_t sfdpRes =(uint32_t)CY_SMIF_SUCCESS;
    uint32_t idx;

    if ((NULL != base) && (NULL != blockConfig) && (NULL != blockConfig->memConfig) 
        && (NULL != context) && (0U != blockConfig->memCount))
    {
        uint32_t size = blockConfig->memCount;
        cy_stc_smif_mem_config_t** extMemCfg = blockConfig->memConfig;

        result = (uint32_t)CY_SMIF_SUCCESS;
        for(idx = 0UL; idx < size; idx++)
        {
            memCfg = extMemCfg[idx];
            if (NULL != memCfg)
            {
                /* Check smif memory slot configuration*/
                CY_ASSERT_L3(CY_SMIF_SLAVE_SEL_VALID(memCfg->slaveSelect));
                CY_ASSERT_L3(CY_SMIF_DATA_SEL_VALID(memCfg->dataSelect));
                CY_ASSERT_L1(NULL != memCfg->deviceCfg);
                CY_ASSERT_L2(CY_SMIF_MEM_ADDR_SIZE_VALID(memCfg->deviceCfg->numOfAddrBytes));
                
                device = Cy_SMIF_GetDeviceBySlot(base, memCfg->slaveSelect);
                if (NULL != device)
                {
                    /* The slave-slot initialization of the device control register.
                     * Cy_SMIF_Memslot_SfdpDetect() must work */
                    SMIF_DEVICE_CTL(device)  = _CLR_SET_FLD32U(SMIF_DEVICE_CTL(device),
                                                               SMIF_DEVICE_CTL_DATA_SEL,
                                                              (uint32_t)memCfg->dataSelect);
                    uint32_t sfdpRet = (uint32_t)CY_SMIF_SUCCESS;
                    if (0U != (memCfg->flags & CY_SMIF_FLAG_DETECT_SFDP))
                    {
                        sfdpRet = (uint32_t)Cy_SMIF_Memslot_SfdpDetect(base,
                                                memCfg->deviceCfg,
                                                memCfg->slaveSelect,
                                                memCfg->dataSelect,
                                                context);
                        if((uint32_t)CY_SMIF_SUCCESS != sfdpRet)
                        {
                            sfdpRes |=  ((uint32_t)CY_SMIF_SFDP_FAIL << idx);
                        }
                    }
                    if (((uint32_t)CY_SMIF_SUCCESS == sfdpRet) &&
                            (0U != (memCfg->flags & CY_SMIF_FLAG_MEMORY_MAPPED)))
                    {
                        /* Check valid parameters for XIP */
                        CY_ASSERT_L3(CY_SMIF_MEM_ADDR_VALID( memCfg->baseAddress, memCfg->memMappedSize));
                        CY_ASSERT_L3(CY_SMIF_MEM_MAPPED_SIZE_VALID( memCfg->memMappedSize));
                        
                        Cy_SMIF_Memslot_XipRegInit(device, memCfg);

                        /* The device control register initialization */
                        SMIF_DEVICE_CTL(device) = (memCfg->flags & CY_SMIF_FLAG_WR_EN) |
                                      (memCfg->flags & CY_SMIF_FLAG_CRYPTO_EN) |
                                      _VAL2FLD(SMIF_DEVICE_CTL_DATA_SEL,  (uint32_t)memCfg->dataSelect) |
                                      SMIF_DEVICE_CTL_ENABLED_Msk;
                    }
                }
                else
                {
                    result = (uint32_t)CY_SMIF_BAD_PARAM;
                    break;
                }
            }
        }
    }
    if((uint32_t)CY_SMIF_SUCCESS != sfdpRes)
    {
        result = CY_SMIF_ID | CY_PDL_STATUS_ERROR | sfdpRes;
    }
    return (cy_en_smif_status_t) result;
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_XipRegInit
****************************************************************************//**
*
* \internal
* This function initializes the memory device registers used for the XIP mode of
* the specified device.
*
* \param dev
* The SMIF memory device registers structure. \ref SMIF_DEVICE_Type
*
* \param memCfg
* The memory configuration structure that configures the SMIF memory device to
*  map into the PSoC memory map. \ref cy_stc_smif_mem_config_t
*
*******************************************************************************/
static void Cy_SMIF_Memslot_XipRegInit(SMIF_DEVICE_Type volatile *dev, cy_stc_smif_mem_config_t const * memCfg)
{
    cy_stc_smif_mem_device_cfg_t const * devCfg = memCfg->deviceCfg;
    cy_stc_smif_mem_cmd_t const * read = devCfg->readCmd;
    cy_stc_smif_mem_cmd_t const * prog = devCfg->programCmd;

    SMIF_DEVICE_ADDR(dev) = (SMIF_DEVICE_ADDR_ADDR_Msk & memCfg->baseAddress);

    /* Convert the size in the mask*/
    SMIF_DEVICE_MASK(dev)= (SMIF_DEVICE_MASK_MASK_Msk & (~(memCfg->memMappedSize) + 1UL));

    SMIF_DEVICE_ADDR_CTL(dev) = _VAL2FLD(SMIF_DEVICE_ADDR_CTL_SIZE2, (devCfg->numOfAddrBytes - 1UL)) |
                                ((0UL != memCfg->dualQuadSlots)? SMIF_DEVICE_ADDR_CTL_DIV2_Msk: 0UL);

    SMIF_DEVICE_RD_CMD_CTL(dev) = (CY_SMIF_NO_COMMAND_OR_MODE != read->command) ?
                                  (_VAL2FLD(SMIF_DEVICE_RD_CMD_CTL_CODE,  (uint32_t)read->command)  |
                                   _VAL2FLD(SMIF_DEVICE_RD_CMD_CTL_WIDTH, (uint32_t)read->cmdWidth) |
                                   SMIF_DEVICE_RD_CMD_CTL_PRESENT_Msk)
                                  : 0U;

    SMIF_DEVICE_RD_ADDR_CTL(dev) = _VAL2FLD(SMIF_DEVICE_RD_ADDR_CTL_WIDTH, (uint32_t)read->addrWidth);

    SMIF_DEVICE_RD_MODE_CTL(dev) = (CY_SMIF_NO_COMMAND_OR_MODE != read->mode) ?
                                   (_VAL2FLD(SMIF_DEVICE_RD_CMD_CTL_CODE,  (uint32_t)read->mode)     |
                                    _VAL2FLD(SMIF_DEVICE_RD_CMD_CTL_WIDTH, (uint32_t)read->modeWidth)|
                                    SMIF_DEVICE_RD_CMD_CTL_PRESENT_Msk)
                                   : 0U;

    SMIF_DEVICE_RD_DUMMY_CTL(dev) = (0UL != read->dummyCycles)?
                                    (_VAL2FLD(SMIF_DEVICE_RD_DUMMY_CTL_SIZE5, (read->dummyCycles - 1UL)) |
                                     SMIF_DEVICE_RD_DUMMY_CTL_PRESENT_Msk)
                                    : 0U;

    SMIF_DEVICE_RD_DATA_CTL(dev) = _VAL2FLD(SMIF_DEVICE_RD_DATA_CTL_WIDTH, (uint32_t)read->dataWidth);

    SMIF_DEVICE_WR_CMD_CTL(dev) = (CY_SMIF_NO_COMMAND_OR_MODE != prog->command) ?
                                  (_VAL2FLD(SMIF_DEVICE_WR_CMD_CTL_CODE,  (uint32_t)prog->command) |
                                   _VAL2FLD(SMIF_DEVICE_WR_CMD_CTL_WIDTH, (uint32_t)prog->cmdWidth)|
                                   SMIF_DEVICE_WR_CMD_CTL_PRESENT_Msk)
                                  : 0U;

    SMIF_DEVICE_WR_ADDR_CTL(dev) = _VAL2FLD(SMIF_DEVICE_WR_ADDR_CTL_WIDTH, (uint32_t)prog->addrWidth);

    SMIF_DEVICE_WR_MODE_CTL(dev) = (CY_SMIF_NO_COMMAND_OR_MODE != prog->mode) ?
                                    (_VAL2FLD(SMIF_DEVICE_WR_CMD_CTL_CODE,  (uint32_t)prog->mode)     |
                                     _VAL2FLD(SMIF_DEVICE_WR_CMD_CTL_WIDTH, (uint32_t)prog->modeWidth)|
                                     SMIF_DEVICE_WR_CMD_CTL_PRESENT_Msk)
                                     : 0UL;

    SMIF_DEVICE_WR_DUMMY_CTL(dev) = (0UL != prog->dummyCycles) ?
                                    (_VAL2FLD(SMIF_DEVICE_WR_DUMMY_CTL_SIZE5, (prog->dummyCycles - 1UL)) |
                                     SMIF_DEVICE_WR_DUMMY_CTL_PRESENT_Msk)
                                    : 0U;

    SMIF_DEVICE_WR_DATA_CTL(dev) = _VAL2FLD(SMIF_DEVICE_WR_DATA_CTL_WIDTH, (uint32_t)prog->dataWidth);
}


/*******************************************************************************
* Function Name: Cy_SMIF_MemoryDeInit
****************************************************************************//**
*
* This function de-initializes all slave slots of the memory device to their default
* values.
*
* \param base
* Holds the base address of the SMIF block registers.
*
*******************************************************************************/
void Cy_SMIF_Memslot_DeInit(SMIF_Type *base)
{
    /* Configure the SMIF device slots to the default values. The default value is 0 */
    uint32_t deviceIndex;

    for(deviceIndex = 0UL; deviceIndex < (uint32_t)SMIF_DEVICE_NR; deviceIndex++)
    {
        SMIF_DEVICE_IDX_CTL(base, deviceIndex)           = 0U;
        SMIF_DEVICE_IDX_ADDR(base, deviceIndex)          = 0U;
        SMIF_DEVICE_IDX_MASK(base, deviceIndex)          = 0U;
        SMIF_DEVICE_IDX_ADDR_CTL(base, deviceIndex)      = 0U;
        SMIF_DEVICE_IDX_RD_CMD_CTL(base, deviceIndex)    = 0U;
        SMIF_DEVICE_IDX_RD_ADDR_CTL(base, deviceIndex)   = 0U;
        SMIF_DEVICE_IDX_RD_MODE_CTL(base, deviceIndex)   = 0U;
        SMIF_DEVICE_IDX_RD_DUMMY_CTL(base, deviceIndex)  = 0U;
        SMIF_DEVICE_IDX_RD_DATA_CTL(base, deviceIndex)   = 0U;
        SMIF_DEVICE_IDX_WR_CMD_CTL(base, deviceIndex)    = 0U;
        SMIF_DEVICE_IDX_WR_ADDR_CTL(base, deviceIndex)   = 0U;
        SMIF_DEVICE_IDX_WR_MODE_CTL(base, deviceIndex)   = 0U;
        SMIF_DEVICE_IDX_WR_DUMMY_CTL(base, deviceIndex)  = 0U;
        SMIF_DEVICE_IDX_WR_DATA_CTL(base, deviceIndex)   = 0U;
    }
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdWriteEnable
****************************************************************************//**
*
* This function sends the Write Enable command to the memory device.
*
* \note This function uses the low-level Cy_SMIF_TransmitCommand() API.
* The Cy_SMIF_TransmitCommand() API works in a blocking mode. In the dual quad mode,
* this API is called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data. \ref cy_stc_smif_context_t
*
* \param memDevice
* The device to which the command is sent.
*
* \return A status of the command transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdWriteEnable(SMIF_Type *base,
                                        cy_stc_smif_mem_config_t const *memDevice,
                                        cy_stc_smif_context_t const *context)
{
    /* The memory Write Enable */
    cy_stc_smif_mem_cmd_t* writeEn = memDevice->deviceCfg->writeEnCmd;
    
    CY_ASSERT_L1(NULL != writeEn);  

    return Cy_SMIF_TransmitCommand( base, (uint8_t) writeEn->command,
                                    writeEn->cmdWidth,
                                    CY_SMIF_CMD_WITHOUT_PARAM,
                                    CY_SMIF_CMD_WITHOUT_PARAM,
                                    (cy_en_smif_txfr_width_t)CY_SMIF_CMD_WITHOUT_PARAM,
                                    memDevice->slaveSelect,
                                    CY_SMIF_TX_LAST_BYTE,
                                    context);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdWriteDisable
****************************************************************************//**
*
* This function sends a Write Disable command to the memory device.
*
* \note This function uses the low-level Cy_SMIF_TransmitCommand() API.
* Cy_SMIF_TransmitCommand() API works in a blocking mode. In the dual quad mode
* this API should be called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data. \ref cy_stc_smif_context_t
*
* \param memDevice
* The device to which the command is sent.
*
* \return A status of the command transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdWriteDisable(SMIF_Type *base,
                                    cy_stc_smif_mem_config_t const *memDevice,
                                    cy_stc_smif_context_t const *context)
{
    cy_stc_smif_mem_cmd_t* writeDis = memDevice->deviceCfg->writeDisCmd;

    CY_ASSERT_L1(NULL != writeDis);  
     
    /* The memory write disable */
    return Cy_SMIF_TransmitCommand( base, (uint8_t)writeDis->command,
                                writeDis->cmdWidth,
                                CY_SMIF_CMD_WITHOUT_PARAM,
                                CY_SMIF_CMD_WITHOUT_PARAM,
                                (cy_en_smif_txfr_width_t) CY_SMIF_CMD_WITHOUT_PARAM,
                                memDevice->slaveSelect,
                                CY_SMIF_TX_LAST_BYTE,
                                context);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_IsBusy
****************************************************************************//**
*
* This function checks if the status of the memory device is busy.
* This is done by reading the status register and the corresponding bit
* (stsRegBusyMask). This function is a blocking function until the status
* register from the memory is read.
*
* \note In the dual quad mode, this API is called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data.
*
* \param memDevice
*  The device to which the command is sent.
*
* \return A status of the memory device.
*       - True - The device is busy or a timeout occurs.
*       - False - The device is not busy.
*
*******************************************************************************/
bool Cy_SMIF_Memslot_IsBusy(SMIF_Type *base, cy_stc_smif_mem_config_t *memDevice,
                            cy_stc_smif_context_t const *context)
{
    uint8_t  status = 1U;
    cy_en_smif_status_t readStsResult;
    cy_stc_smif_mem_device_cfg_t* device =  memDevice->deviceCfg;

    CY_ASSERT_L1(NULL != device->readStsRegWipCmd);  
    
    readStsResult = Cy_SMIF_Memslot_CmdReadSts(base, memDevice, &status,
                         (uint8_t)device->readStsRegWipCmd->command,
                         context);

    if (CY_SMIF_SUCCESS == readStsResult)
    {
        /* Masked not busy bits in returned status */
        status &= (uint8_t)device->stsRegBusyMask;
    }

    return (0U != status);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_QuadEnable
****************************************************************************//**
*
* This function enables the memory device for the quad mode of operation.
* This command must be executed before sending Quad SPI commands to the
* memory device.
*
* \note In the dual quad mode, this API is called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data.
*
* \param memDevice
* The device to which the command is sent.
*
* \return A status of the command.
*   - \ref CY_SMIF_SUCCESS
*   - \ref CY_SMIF_NO_QE_BIT
*   - \ref CY_SMIF_CMD_FIFO_FULL
*   - \ref CY_SMIF_BAD_PARAM
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_QuadEnable(SMIF_Type *base,
                                    cy_stc_smif_mem_config_t *memDevice,
                                    cy_stc_smif_context_t const *context)
{
    cy_en_smif_status_t result;
    uint8_t statusReg[CY_SMIF_QE_BIT_STS_REG2_T1] = {0U};  
    cy_stc_smif_mem_device_cfg_t* device =  memDevice->deviceCfg;
    
    /* Check that command exists */
    CY_ASSERT_L1(NULL != device->readStsRegQeCmd);
    CY_ASSERT_L1(NULL != device->writeStsRegQeCmd);
    CY_ASSERT_L1(NULL != device->readStsRegWipCmd);
    
    uint8_t readQeCmd  = (uint8_t)device->readStsRegQeCmd->command;
    uint8_t writeQeCmd = (uint8_t)device->writeStsRegQeCmd->command;
    uint8_t readWipCmd = (uint8_t)device->readStsRegWipCmd->command;

    result = Cy_SMIF_Memslot_CmdReadSts(base, memDevice, &statusReg[0U],
                readQeCmd, context);

    if (CY_SMIF_SUCCESS == result)
    {
        uint32_t qeMask = device->stsRegQuadEnableMask;

        switch(qeMask)
        {
            case CY_SMIF_SFDP_QE_BIT_6_OF_SR_1:
                statusReg[0U] |= (uint8_t)qeMask;
                result = Cy_SMIF_Memslot_CmdWriteSts(base, memDevice,
                            &statusReg[0U], writeQeCmd, context);
                break;
            case CY_SMIF_SFDP_QE_BIT_1_OF_SR_2:
                /* Read status register 1 with the assumption that WIP is always in
                 * status register 1 */
                result = Cy_SMIF_Memslot_CmdReadSts(base, memDevice,
                            &statusReg[0U], readWipCmd, context);
                if (CY_SMIF_SUCCESS == result)
                {
                    result = Cy_SMIF_Memslot_CmdReadSts(base, memDevice,
                                &statusReg[1U], readQeCmd, context);
                    if (CY_SMIF_SUCCESS == result)
                    {
                        statusReg[1U] |= (uint8_t)qeMask;
                        result = Cy_SMIF_Memslot_CmdWriteSts(base, memDevice,
                                    statusReg, writeQeCmd, context);
                    }
                }
                break;
            case CY_SMIF_SFDP_QE_BIT_7_OF_SR_2:
                result = Cy_SMIF_Memslot_CmdReadSts(base, memDevice,
                            &statusReg[1U], readQeCmd, context);
                if (CY_SMIF_SUCCESS == result)
                {
                    statusReg[1U] |= (uint8_t)qeMask;
                    result = Cy_SMIF_Memslot_CmdWriteSts(base, memDevice,
                                &statusReg[1U], writeQeCmd, context);
                }
                break;
            default:
                result = CY_SMIF_NO_QE_BIT;
                break;
        }

    }

    return(result);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdReadSts
****************************************************************************//**
*
* This function reads the status register. This function is a blocking function,
* it will block the execution flow until the status register is read.
*
* \note This function uses the low-level Cy_SMIF_TransmitCommand() API.
* the Cy_SMIF_TransmitCommand() API works in a blocking mode. In the dual quad mode,
* this API is called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data.
*
* \param memDevice
* The device to which the command is sent.
*
* \param status
* The status register value returned by the external memory.
*
* \param command
* The command required to read the status/configuration register.
*
* \return A status of the command reception.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_CMD_FIFO_FULL
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdReadSts(SMIF_Type *base,
                                    cy_stc_smif_mem_config_t const *memDevice,
                                    uint8_t *status,
                                    uint8_t command,
                                    cy_stc_smif_context_t const *context)
{
    cy_en_smif_status_t result;

    /* Read the memory status register */
    result = Cy_SMIF_TransmitCommand( base, command, CY_SMIF_WIDTH_SINGLE,
                CY_SMIF_CMD_WITHOUT_PARAM, CY_SMIF_CMD_WITHOUT_PARAM,
                (cy_en_smif_txfr_width_t) CY_SMIF_CMD_WITHOUT_PARAM,
                memDevice->slaveSelect, CY_SMIF_TX_NOT_LAST_BYTE, context);

    if (CY_SMIF_SUCCESS == result)
    {
        result = Cy_SMIF_ReceiveDataBlocking( base, status,
                    CY_SMIF_READ_ONE_BYTE, CY_SMIF_WIDTH_SINGLE, context);
    }

    return(result);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdWriteSts
****************************************************************************//**
*
* This function writes the status register. This is a blocking function, it will
* block the execution flow until the command transmission is completed.
*
* \note This function uses the low-level Cy_SMIF_TransmitCommand() API.
* The Cy_SMIF_TransmitCommand() API works in a blocking mode. In the dual quad mode,
* this API is called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data. \ref cy_stc_smif_context_t
*
* \param memDevice
* The device to which the command is sent.
*
* \param status
* The status to write into the status register.
*
* \param command
* The command to write into the status/configuration register.              
*
* \return A status of the command transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdWriteSts(SMIF_Type *base,
                                    cy_stc_smif_mem_config_t const *memDevice,
                                    void const *status,
                                    uint8_t command,
                                    cy_stc_smif_context_t const *context)
{
    cy_en_smif_status_t result;

    /* The Write Enable */
    result =  Cy_SMIF_Memslot_CmdWriteEnable(base, memDevice, context);

    /* The Write Status */
    if (CY_SMIF_SUCCESS == result)
    {
        uint32_t size;
        uint32_t qeMask = memDevice->deviceCfg->stsRegQuadEnableMask;

        size = ( CY_SMIF_SFDP_QE_BIT_1_OF_SR_2 == qeMask)? CY_SMIF_WRITE_TWO_BYTES:
                                                        CY_SMIF_WRITE_ONE_BYTE;
        result = Cy_SMIF_TransmitCommand( base, command, CY_SMIF_WIDTH_SINGLE,
                    (uint8_t const *)status, size, CY_SMIF_WIDTH_SINGLE,
                    memDevice->slaveSelect, CY_SMIF_TX_LAST_BYTE, context);
    }

    return(result);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdChipErase
****************************************************************************//**
*
* This function performs a chip erase of the external memory. The Write Enable
* command is called before this API.
*
* \note This function uses the low-level Cy_SMIF_TransmitCommand() API.
* Cy_SMIF_TransmitCommand() API works in a blocking mode. In the dual quad mode,
* this API is called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data. \ref cy_stc_smif_context_t
*
* \param memDevice
* The device to which the command is sent
*
* \return A status of the command transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdChipErase(SMIF_Type *base,
                cy_stc_smif_mem_config_t const *memDevice,
                cy_stc_smif_context_t const *context)
{
    cy_en_smif_status_t result;

    cy_stc_smif_mem_cmd_t *cmdErase = memDevice->deviceCfg->chipEraseCmd;
    CY_ASSERT_L1(NULL != cmdErase);

    result = Cy_SMIF_TransmitCommand( base, (uint8_t)cmdErase->command,
            cmdErase->cmdWidth, CY_SMIF_CMD_WITHOUT_PARAM,
            CY_SMIF_CMD_WITHOUT_PARAM,
            (cy_en_smif_txfr_width_t) CY_SMIF_CMD_WITHOUT_PARAM,
            memDevice->slaveSelect, CY_SMIF_TX_LAST_BYTE, context);

    return(result);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdSectorErase
****************************************************************************//**
*
* This function performs a block Erase of the external memory. The Write Enable
* command is called before this API.
*
* \note This function uses the low-level Cy_SMIF_TransmitCommand() API.
* The Cy_SMIF_TransmitCommand() API works in a blocking mode. In the dual quad mode,
* this API is called for each memory.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data. \ref cy_stc_smif_context_t
*
* \param memDevice
* The device to which the command is sent.
*
* \param sectorAddr
* The sector address to erase.
*
* \return A status of the command transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_BAD_PARAM
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdSectorErase(SMIF_Type *base,
                                        cy_stc_smif_mem_config_t *memDevice,
                                        uint8_t const *sectorAddr,
                                        cy_stc_smif_context_t const *context)
{
    cy_en_smif_status_t result = CY_SMIF_BAD_PARAM;

    if (NULL != sectorAddr)
    {

        cy_stc_smif_mem_device_cfg_t *device = memDevice->deviceCfg;
        cy_stc_smif_mem_cmd_t *cmdErase = device->eraseCmd;
        
        CY_ASSERT_L1(NULL != cmdErase);

        result = Cy_SMIF_TransmitCommand( base, (uint8_t)cmdErase->command,
                cmdErase->cmdWidth, sectorAddr, device->numOfAddrBytes,
                cmdErase->cmdWidth, memDevice->slaveSelect,
                CY_SMIF_TX_LAST_BYTE, context);
    }

    return(result);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdProgram
****************************************************************************//**
*
* This function performs the Program operation. 
*
* \note This function uses the  Cy_SMIF_TransmitCommand() API.
* The Cy_SMIF_TransmitCommand() API works in the blocking mode. In the dual quad mode,
* this API works with both types of memory simultaneously.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data.
*
* \param memDevice
* The device to which the command is sent.
*
* \param addr
* The address to program.
*
* \param writeBuff
* The pointer to the data to program. If this pointer is a NULL, then the
* function does not enable the interrupt. This use case is  typically used when
* the FIFO is handled outside the interrupt and is managed in either a 
* polling-based code or a DMA. The user would handle the FIFO management 
* in a DMA or a polling-based code. 
* If the user provides a NULL pointer in this function and does not handle 
* the FIFO transaction, this could either stall or timeout the operation 
* \ref Cy_SMIF_TransmitData().
*
*
* \param size
* The size of data to program. The user must ensure that the data size
* does not exceed the page size.
*
* \param cmdCmpltCb
* The callback function to call after the transfer completion. NULL interpreted
* as no callback.
*
* \return A status of a transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_CMD_FIFO_FULL
*       - \ref CY_SMIF_BAD_PARAM
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdProgram(SMIF_Type *base,
                                    cy_stc_smif_mem_config_t const *memDevice,
                                    uint8_t const *addr,
                                    uint8_t* writeBuff,
                                    uint32_t size,
                                    cy_smif_event_cb_t cmdCmpltCb,
                                    cy_stc_smif_context_t *context)
{
    cy_en_smif_status_t result = CY_SMIF_BAD_PARAM;
    cy_en_smif_slave_select_t slaveSelected;

    cy_stc_smif_mem_device_cfg_t *device = memDevice->deviceCfg;
    cy_stc_smif_mem_cmd_t *cmdProg = device->programCmd;
    
    CY_ASSERT_L1(NULL != cmdProg);

    if ((NULL != addr) && (size <= device->programSize))
    {
        slaveSelected = (0U == memDevice->dualQuadSlots)?  memDevice->slaveSelect :
                                                        (cy_en_smif_slave_select_t)memDevice->dualQuadSlots;
        /* The page program command */
        result = Cy_SMIF_TransmitCommand( base, (uint8_t)cmdProg->command,
                cmdProg->cmdWidth, addr, device->numOfAddrBytes,
                cmdProg->addrWidth, slaveSelected, CY_SMIF_TX_NOT_LAST_BYTE,
                context);

        if((CY_SMIF_SUCCESS == result) && (cmdProg->dummyCycles > 0U))
        {
            result = Cy_SMIF_SendDummyCycles(base, cmdProg->dummyCycles);
        }

        if(CY_SMIF_SUCCESS == result)
        {
            result = Cy_SMIF_TransmitData( base, writeBuff, size,
                    cmdProg->dataWidth, cmdCmpltCb, context);
        }
    }

    return(result);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_CmdRead
****************************************************************************//**
*
* This function performs the Read operation.
*
* \note This function uses the Cy_SMIF_TransmitCommand() API.
* The Cy_SMIF_TransmitCommand() API works in the blocking mode. In the dual quad mode,
* this API works with both types of memory simultaneously.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data.
*
* \param memDevice
* The device to which the command is sent.
*
* \param addr
* The address to read.
*
* \param readBuff
* The pointer to the variable where the read data is stored. If this pointer is 
* a NULL, then the function does not enable the interrupt. This use case is 
* typically used when the FIFO is handled outside the interrupt and is managed
* in either a  polling-based code or a DMA. The user would handle the FIFO
* management in a DMA or a polling-based code. 
* If the user provides a NULL pointer in this function and does not handle 
* the FIFO transaction, this could either stall or timeout the operation 
* \ref Cy_SMIF_TransmitData().
*
* \param size
* The size of data to read.
*
* \param cmdCmpltCb
* The callback function to call after the transfer completion. NULL interpreted
* as no callback.
*
* \return A status of the transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_CMD_FIFO_FULL
*       - \ref CY_SMIF_BAD_PARAM
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_CmdRead(SMIF_Type *base,
                                cy_stc_smif_mem_config_t const *memDevice,
                                uint8_t const *addr,
                                uint8_t* readBuff,
                                uint32_t size,
                                cy_smif_event_cb_t cmdCmpltCb,
                                cy_stc_smif_context_t *context)
{
    cy_en_smif_status_t result = CY_SMIF_BAD_PARAM;
    cy_en_smif_slave_select_t slaveSelected;
    cy_stc_smif_mem_device_cfg_t *device = memDevice->deviceCfg;
    cy_stc_smif_mem_cmd_t *cmdRead = device->readCmd;

    if (NULL != addr)
    {
        slaveSelected = (0U == memDevice->dualQuadSlots)?  memDevice->slaveSelect :
                               (cy_en_smif_slave_select_t)memDevice->dualQuadSlots;

        result = Cy_SMIF_TransmitCommand( base, (uint8_t)cmdRead->command,
                    cmdRead->cmdWidth, addr, device->numOfAddrBytes,
                    cmdRead->addrWidth, slaveSelected, CY_SMIF_TX_NOT_LAST_BYTE,
                    context);

        if((CY_SMIF_SUCCESS == result) && (0U < cmdRead->dummyCycles))
        {
            result = Cy_SMIF_SendDummyCycles(base, cmdRead->dummyCycles);
        }

        if((CY_SMIF_SUCCESS == result) && (CY_SMIF_NO_COMMAND_OR_MODE != cmdRead->mode))
        {
            result = Cy_SMIF_TransmitCommand(base, (uint8_t)cmdRead->mode,
                        cmdRead->dataWidth, CY_SMIF_CMD_WITHOUT_PARAM,
                        CY_SMIF_CMD_WITHOUT_PARAM,
                        (cy_en_smif_txfr_width_t) CY_SMIF_CMD_WITHOUT_PARAM,
                        (cy_en_smif_slave_select_t)slaveSelected,
                        CY_SMIF_TX_NOT_LAST_BYTE, context);
        }

        if(CY_SMIF_SUCCESS == result)
        {
            result = Cy_SMIF_ReceiveData(base, readBuff, size,
                        cmdRead->dataWidth, cmdCmpltCb, context);
        }
    }

    return(result);
}


/*******************************************************************************
* Function Name: Cy_SMIF_Memslot_SfdpDetect
****************************************************************************//**
*
* This function detects the device signature for SFDP devices.
* Refer to the SFDP spec (JESD216B) for details.
* The function asks the device using an SPI and then populates the relevant
* parameters for \ref cy_stc_smif_mem_device_cfg_t.
*
* \note This function is a blocking function and blocks until the structure data
* is read and returned. This function uses \ref Cy_SMIF_TransmitCommand()
* If there is no support for SFDP in the memory device, the API returns an
* error condition. The function requires:
*   - SMIF initialized and enabled to work in the normal mode;
*   - readSfdpCmd field of \ref cy_stc_smif_mem_device_cfg_t is enabled.
*
* \note The SFDP detect takes into account the types of the SPI supported by the
* memory device and also the dataSelect option selected to choose which SPI mode
* (SPI, DSPI, QSPI) to load into the structures. The algorithm prefers
* QSPI>DSPI>SPI, provided there is support for it in the memory device and the
* dataSelect selected by the user.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* The internal SMIF context data.
*
* \param device
* The device structure instance declared by the user. This is where the detected
* parameters are stored and returned.
*
* \param slaveSelect
* The slave select line for the device.
*
* \param dataSelect
* The data line selection options for a slave device.
*
* \return A status of the transmission.
*       - \ref CY_SMIF_SUCCESS
*       - \ref CY_SMIF_CMD_FIFO_FULL
*       - \ref CY_SMIF_NO_SFDP_SUPPORT
*       - \ref CY_SMIF_EXCEED_TIMEOUT
*
*******************************************************************************/
cy_en_smif_status_t Cy_SMIF_Memslot_SfdpDetect(SMIF_Type *base,
                                    cy_stc_smif_mem_device_cfg_t *device,
                                    cy_en_smif_slave_select_t slaveSelect,
                                    cy_en_smif_data_select_t dataSelect,
                                    cy_stc_smif_context_t *context)
{
    /* Check input parameters */
    CY_ASSERT_L1(NULL != device);
    CY_ASSERT_L1(NULL != device->readSfdpCmd);
    
    uint8_t sfdpBuffer[CY_SMIF_SFDP_LENGTH];
    uint8_t sfdpAddress[CY_SMIF_SFDP_ADDRESS_LENGTH] = {0x00U, 0x00U, 0x00U};
    cy_en_smif_status_t result;
    cy_stc_smif_mem_cmd_t *cmdSfdp = device->readSfdpCmd;

    /* Slave slot initialization */
    Cy_SMIF_SetDataSelect(base, slaveSelect, dataSelect);

    result = Cy_SMIF_TransmitCommand( base, (uint8_t)cmdSfdp->command,
                cmdSfdp->cmdWidth, sfdpAddress, CY_SMIF_SFDP_ADDRESS_LENGTH,
                cmdSfdp->addrWidth, slaveSelect, CY_SMIF_TX_NOT_LAST_BYTE,
                context);
    if(CY_SMIF_SUCCESS == result)
    {
        result = Cy_SMIF_SendDummyCycles(base, cmdSfdp->dummyCycles);
    }

    if(CY_SMIF_SUCCESS == result)
    {
        result = Cy_SMIF_ReceiveData( base, sfdpBuffer, CY_SMIF_SFDP_LENGTH,
                    cmdSfdp->dataWidth, NULL, context);
    }

    if (CY_SMIF_SUCCESS == result)
    {
        uint32_t cmdTimeout = context->timeout;
        while (((uint32_t) CY_SMIF_REC_CMPLT != context->transferStatus) &&
                (CY_SMIF_EXCEED_TIMEOUT != result))
        {
            /* Wait until the Read of the SFDP operation is completed. */
            result = Cy_SMIF_TimeoutRun(&cmdTimeout);
        }
    }

    if (CY_SMIF_SUCCESS == result)
    {
        if((sfdpBuffer[CY_SMIF_SFDP_SING_BYTE_00] == (uint8_t)'S') &&
         (sfdpBuffer[CY_SMIF_SFDP_SING_BYTE_01] == (uint8_t)'F') &&
         (sfdpBuffer[CY_SMIF_SFDP_SING_BYTE_02] == (uint8_t)'D') &&
         (sfdpBuffer[CY_SMIF_SFDP_SING_BYTE_03] == (uint8_t)'P') &&
         (sfdpBuffer[CY_SMIF_SFDP_MINOR_REV] >= CY_SMIF_SFDP_JEDEC_REV_B) &&
         (sfdpBuffer[CY_SMIF_SFDP_MAJOR_REV] == CY_SMIF_SFDP_MAJOR_REV_1))
        {
            /* The address of the JEDEC basic flash parameter table */
            uint8_t offset = sfdpBuffer[CY_SMIF_SFDP_PARAM_TABLE_PTR];
            cy_stc_smif_mem_cmd_t *cmdRead = device->readCmd;

            /* The number of address bytes used by the memory slave device */
            uint32_t sfdpDataIndex = CY_SMIF_SFDP_BFPT_BYTE_02 + (uint32_t)offset;
            uint32_t sfdpAddrCode = _FLD2VAL(CY_SMIF_SFDP_ADDRESS_BYTES,
                                                (uint32_t)sfdpBuffer
                                                [sfdpDataIndex]);
            switch(sfdpAddrCode)
            {
                case CY_SMIF_SFDP_THREE_BYTES_ADDR_CODE:
                    device->numOfAddrBytes = CY_SMIF_THREE_BYTES_ADDR;
                    break;
                case CY_SMIF_SFDP_THREE_OR_FOUR_BYTES_ADDR_CODE:
                    device->numOfAddrBytes = CY_SMIF_THREE_BYTES_ADDR;
                    break;
                case CY_SMIF_SFDP_FOUR_BYTES_ADDR_CODE:
                    device->numOfAddrBytes = CY_SMIF_FOUR_BYTES_ADDR;
                    break;
                default:
                    break;
            }

            /* Erase Time Type 1*/
            uint32_t readEraseTime   = ((uint32_t*)sfdpBuffer)[(offset/CY_SMIF_BYTES_IN_WORD) +
                                                               CY_SMIF_JEDEC_BFPT_10TH_DWORD];
            uint32_t eraseUnits = _FLD2VAL(CY_SMIF_SFDP_ERASE_T1_UNITS, readEraseTime);
            uint32_t eraseCount  = _FLD2VAL(CY_SMIF_SFDP_ERASE_T1_COUNT, readEraseTime);
            uint32_t eraseMul = _FLD2VAL(CY_SMIF_SFDP_ERASE_MUL_COUNT, readEraseTime);
            uint32_t eraseMs = 0U;

            switch (eraseUnits)
            {
                case CY_SMIF_SFDP_UNIT_0:
                    eraseMs = CY_SMIF_SFDP_ERASE_TIME_1MS;
                    break;
                case CY_SMIF_SFDP_UNIT_1:
                    eraseMs = CY_SMIF_SFDP_ERASE_TIME_16MS;
                    break;
                case CY_SMIF_SFDP_UNIT_2:
                    eraseMs = CY_SMIF_SFDP_ERASE_TIME_128MS;
                    break;
                case CY_SMIF_SFDP_UNIT_3:
                    eraseMs = CY_SMIF_SFDP_ERASE_TIME_1S;
                    break;
                default:
                    /* An unsupported SFDP value */
                    break;
            }
            /* Convert typical time to max time */
            device->eraseTime = ((eraseCount + 1U) * eraseMs) * (2U * (eraseMul + 1U));


            /* Chip Erase Time*/
            uint32_t chipEraseProgTime   = ((uint32_t*)sfdpBuffer)[(offset/CY_SMIF_BYTES_IN_WORD) +
                                                                   CY_SMIF_JEDEC_BFPT_11TH_DWORD];
            uint32_t chipEraseUnits = _FLD2VAL(CY_SMIF_SFDP_CHIP_ERASE_UNITS, chipEraseProgTime);
            uint32_t chipEraseCount = _FLD2VAL(CY_SMIF_SFDP_CHIP_ERASE_COUNT, chipEraseProgTime);
            uint32_t chipEraseMs = 0U;

            switch (chipEraseUnits)
            {
                case CY_SMIF_SFDP_UNIT_0:
                    chipEraseMs = CY_SMIF_SFDP_CHIP_ERASE_TIME_16MS;
                    break;
                case CY_SMIF_SFDP_UNIT_1:
                    chipEraseMs = CY_SMIF_SFDP_CHIP_ERASE_TIME_256MS;
                    break;
                case CY_SMIF_SFDP_UNIT_2:
                    chipEraseMs = CY_SMIF_SFDP_CHIP_ERASE_TIME_4S;
                    break;
                case CY_SMIF_SFDP_UNIT_3:
                    chipEraseMs = CY_SMIF_SFDP_CHIP_ERASE_TIME_64S;
                    break;
                default:
                    /* An unsupported SFDP value*/
                    break;
            }
            /* Convert typical time to max time */
            device->chipEraseTime = ((chipEraseCount + 1U)*chipEraseMs) * (2U *(eraseMul + 1U));

            /* Page Program Time*/
            uint32_t programTimeUnits = _FLD2VAL(CY_SMIF_SFDP_PAGE_PROG_UNITS, chipEraseProgTime);
            uint32_t programTimeCount  = _FLD2VAL(CY_SMIF_SFDP_PAGE_PROG_COUNT, chipEraseProgTime);
            uint32_t progMul = _FLD2VAL(CY_SMIF_SFDP_PROG_MUL_COUNT, chipEraseProgTime);
            uint32_t progUs;

            if (CY_SMIF_SFDP_UNIT_0 == programTimeUnits)
            {
                progUs = CY_SMIF_SFDP_PROG_TIME_8US;
            }
            else
            {
                progUs = CY_SMIF_SFDP_PROG_TIME_64US;
            }
            /* Convert typical time to max time */
            device->programTime = ((programTimeCount + 1U) * progUs) * (2U * (progMul + 1U));


            /* The size of the external memory */
            uint32_t locSize = Cy_SMIF_PackBytesArray(&sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_04 +
                                                                    offset], true);

            if (0UL == (locSize & CY_SMIF_SFDP_SIZE_ABOVE_4GB_Msk))
            {
                device->memSize = (locSize + 1UL)/CY_SMIF_BITS_IN_BYTE;
            }
            else
            {
                device->memSize = (locSize - CY_SMIF_BITS_IN_BYTE_ABOVE_4GB) |
                        CY_SMIF_SFDP_SIZE_ABOVE_4GB_Msk;
            }
            
            /* The page size */
            device->programSize = 0x01UL << _FLD2VAL(CY_SMIF_SFDP_PAGE_SIZE,
                (uint32_t) sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_28 + offset]);         
            
            /* The size of the Erase sector */
            device->eraseSize = (0x01UL << (uint32_t)sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_1C + offset]);   

            /* This specifies the Read command. The preference order Quad>Dual>SPI */
            if ((_FLD2VAL(CY_SMIF_SFDP_FAST_READ_1_4_4,
                                    ((uint32_t) sfdpBuffer[sfdpDataIndex])) == 1UL) &&
                                                (CY_SMIF_DATA_SEL1 != dataSelect) &&
                                                (CY_SMIF_DATA_SEL3 != dataSelect))
            {
                /* The 8-bit command. 4 x I/O Read command */
                cmdRead->command = sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_09 + offset];

                /* The width of the command transfer */
                cmdRead->cmdWidth = CY_SMIF_WIDTH_SINGLE;

                /* The width of the address transfer */
                cmdRead->addrWidth = CY_SMIF_WIDTH_QUAD;

                /* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present */
                if ((_FLD2VAL(CY_SMIF_SFDP_1_4_4_MODE_CYCLES,
                    (uint32_t) sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_08 + offset])) == 0U)
                {
                    cmdRead->mode = CY_SMIF_NO_COMMAND_OR_MODE;
                }
                else
                {
                    cmdRead->mode = CY_SMIF_READ_MODE_BYTE;
                }

                /* The number of the dummy cycles. A zero value suggests no dummy cycles */
                cmdRead->dummyCycles = _FLD2VAL(CY_SMIF_SFDP_1_4_4_DUMMY_CYCLES, (uint32_t) sfdpBuffer
                                        [CY_SMIF_SFDP_BFPT_BYTE_08 + offset]);

                /* The width of the data transfer*/
                cmdRead->dataWidth = CY_SMIF_WIDTH_QUAD;
            }
            else
            {
                if ((_FLD2VAL(CY_SMIF_SFDP_FAST_READ_1_1_4,
                                    ((uint32_t)sfdpBuffer[sfdpDataIndex])) == 1UL)  &&
                                                (CY_SMIF_DATA_SEL1 != dataSelect) &&
                                                (CY_SMIF_DATA_SEL3 != dataSelect))
                {
                    /* The 8-bit command. 4 x I/O Read command */
                    cmdRead->command  =
                            sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_0B + offset];

                    /* The width of the command transfer */
                    cmdRead->cmdWidth = CY_SMIF_WIDTH_SINGLE;

                    /* The width of the address transfer */
                    cmdRead->addrWidth = CY_SMIF_WIDTH_QUAD;

                    /* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present */
                    if ((_FLD2VAL(CY_SMIF_SFDP_1_1_4_MODE_CYCLES, (uint32_t) sfdpBuffer
                                    [CY_SMIF_SFDP_BFPT_BYTE_0A + offset])) == 0U)
                    {
                        cmdRead->mode = CY_SMIF_NO_COMMAND_OR_MODE;
                    }
                    else
                    {
                        cmdRead->mode = CY_SMIF_READ_MODE_BYTE;
                    }

                    /* The number of the dummy cycles. A zero value suggests no dummy cycles */
                    cmdRead->dummyCycles = _FLD2VAL(CY_SMIF_SFDP_1_1_4_DUMMY_CYCLES,
                        (uint32_t)sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_0A + offset]);

                    /* The width of the data transfer*/
                    cmdRead->dataWidth = CY_SMIF_WIDTH_QUAD;
                }
                else
                {
                    if ((_FLD2VAL(CY_SMIF_SFDP_FAST_READ_1_2_2,
                                        (uint32_t)sfdpBuffer[sfdpDataIndex])) == 1UL)
                    {
                        /* The 8-bit command. 2 x I/O Read command */
                        cmdRead->command  = sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_0F + offset];

                        /* The width of the command transfer */
                        cmdRead->cmdWidth = CY_SMIF_WIDTH_SINGLE;

                        /* The width of the address transfer */
                        cmdRead->addrWidth = CY_SMIF_WIDTH_DUAL;

                        /* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present */
                        if ((_FLD2VAL(CY_SMIF_SFDP_1_2_2_MODE_CYCLES, (uint32_t)
                            sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_0E + offset])) == 0U)
                        {
                            cmdRead->mode = CY_SMIF_NO_COMMAND_OR_MODE;
                        }
                        else
                        {
                            cmdRead->mode = CY_SMIF_READ_MODE_BYTE;
                        }

                        /* The number of the dummy cycles. A zero value suggests no dummy cycles */
                        cmdRead->dummyCycles = _FLD2VAL(CY_SMIF_SFDP_1_2_2_DUMMY_CYCLES,
                                                (uint32_t) sfdpBuffer [CY_SMIF_SFDP_BFPT_BYTE_0E + offset]);

                        /* The width of the data transfer*/
                        cmdRead->dataWidth = CY_SMIF_WIDTH_DUAL;
                    }
                    else
                    {
                        if ((_FLD2VAL(CY_SMIF_SFDP_FAST_READ_1_1_2,
                                            (uint32_t)sfdpBuffer[sfdpDataIndex])) == 1UL)
                        {
                            /* The 8-bit command. 2 x I/O Read command */
                            cmdRead->command  = sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_0D + offset];

                            /* The width of the command transfer */
                            cmdRead->cmdWidth = CY_SMIF_WIDTH_SINGLE;

                            /* The width of the address transfer */
                            cmdRead->addrWidth = CY_SMIF_WIDTH_SINGLE;

                            /* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present */
                            if ((_FLD2VAL(CY_SMIF_SFDP_1_1_2_MODE_CYCLES, (uint32_t)
                                sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_0C + offset])) == 0U)
                            {
                                cmdRead->mode = CY_SMIF_NO_COMMAND_OR_MODE;
                            }
                            else
                            {
                                cmdRead->mode = CY_SMIF_READ_MODE_BYTE;
                            }

                            /* The number of the dummy cycles. A zero value suggests no dummy cycles */
                            cmdRead->dummyCycles = _FLD2VAL(CY_SMIF_SFDP_1_1_2_DUMMY_CYCLES,
                                    (uint32_t)sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_0C + offset]);

                            /* The width of the data transfer*/
                            cmdRead->dataWidth = CY_SMIF_WIDTH_DUAL;
                        }
                        else
                        {
                            /* The 8-bit command. 1 x I/O Read command */
                            cmdRead->command  = CY_SMIF_SINGLE_READ_CMD;

                            /* The width of the command transfer */
                            cmdRead->cmdWidth = CY_SMIF_WIDTH_SINGLE;

                            /* The width of the address transfer */
                            cmdRead->addrWidth = CY_SMIF_WIDTH_SINGLE;

                            /* The 8 bit-mode byte. This value is 0xFFFFFFFF when there is no mode present */
                            cmdRead->mode = CY_SMIF_NO_COMMAND_OR_MODE;

                            /* The number of the dummy cycles. A zero value suggests no dummy cycles */
                            cmdRead->dummyCycles = 0U;

                            /* The width of the data transfer*/
                            cmdRead->dataWidth = CY_SMIF_WIDTH_SINGLE;
                        }
                    }
                }
            }

            /* The Write Enable command */
            /* The 8-bit command. Write Enable */
            device->writeEnCmd->command = CY_SMIF_WR_ENABLE_CMD;
            /* The width of the command transfer */
            device->writeEnCmd->cmdWidth = CY_SMIF_WIDTH_SINGLE;

            /* The Write Disable command */
            /* The 8-bit command. Write Disable */
            device->writeDisCmd->command = CY_SMIF_WR_DISABLE_CMD;
            /* The width of the command transfer */
            device->writeDisCmd->cmdWidth = CY_SMIF_WIDTH_SINGLE;
            
            /* The chip Erase command */
            /* The 8-bit command. Chip Erase */
            device->chipEraseCmd->command  = CY_SMIF_CHIP_ERASE_CMD;
            /* The width of the command transfer */
            device->chipEraseCmd->cmdWidth = CY_SMIF_WIDTH_SINGLE;

            /* The sector Erase command */
            /* The 8-bit command. The sector Erase */
            device->eraseCmd->command  =
                        sfdpBuffer[CY_SMIF_SFDP_BFPT_BYTE_1D + offset];
            /* The width of the command transfer */
            device->eraseCmd->cmdWidth = CY_SMIF_WIDTH_SINGLE;
            /* The width of the address transfer */
            device->eraseCmd->addrWidth = CY_SMIF_WIDTH_SINGLE;

            /* This specifies the program command */
            /* The 8-bit command. 1 x I/O Program command */
            device->programCmd->command = CY_SMIF_SINGLE_PROGRAM_CMD;
            /* The width of the command transfer */
            device->programCmd->cmdWidth = CY_SMIF_WIDTH_SINGLE;
            /* The width of the address transfer */
            device->programCmd->addrWidth = CY_SMIF_WIDTH_SINGLE;
            /* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present */
            device->programCmd->mode = CY_SMIF_NO_COMMAND_OR_MODE;
            /* The number of the dummy cycles. A zero value suggests no dummy cycles */
            device->programCmd->dummyCycles = 0U;
            /* The width of the data transfer*/
            device->programCmd->dataWidth = CY_SMIF_WIDTH_SINGLE;

            /* The busy mask for the status registers */
            device->stsRegBusyMask = CY_SMIF_STS_REG_BUSY_MASK;
                        
            /* The command to read the WIP-containing status register */
            /* The 8-bit command. WIP RDSR */
            device->readStsRegWipCmd->command  = CY_SMIF_RD_STS_REG1_CMD;
            /* The width of the command transfer */
            device->readStsRegWipCmd->cmdWidth = CY_SMIF_WIDTH_SINGLE;

            /* The command to write into the QE-containing status register */
            /* The width of the command transfer */
            device->writeStsRegQeCmd->cmdWidth = CY_SMIF_WIDTH_SINGLE;

            /* The QE mask for the status registers */
            switch (_FLD2VAL(CY_SMIF_SFDP_QE_REQUIREMENTS, (uint32_t)sfdpBuffer
                [CY_SMIF_SFDP_BFPT_BYTE_3A + offset]))
            {
                case CY_SMIF_SFDP_QER_0:
                    device->stsRegQuadEnableMask = CY_SMIF_NO_COMMAND_OR_MODE;
                    device->writeStsRegQeCmd->command  = CY_SMIF_NO_COMMAND_OR_MODE;
                    device->readStsRegQeCmd->command  = CY_SMIF_NO_COMMAND_OR_MODE;
                    break;
                case CY_SMIF_SFDP_QER_1:
                case CY_SMIF_SFDP_QER_4:
                case CY_SMIF_SFDP_QER_5:
                    device->stsRegQuadEnableMask = CY_SMIF_SFDP_QE_BIT_1_OF_SR_2;

                    /* The command to write into the QE-containing status register */
                    /* The 8-bit command. QE WRSR */
                    device->writeStsRegQeCmd->command  = CY_SMIF_WR_STS_REG1_CMD;
                    device->readStsRegQeCmd->command  = CY_SMIF_RD_STS_REG2_T1_CMD;
                    break;
                case CY_SMIF_SFDP_QER_2:
                    device->stsRegQuadEnableMask = CY_SMIF_SFDP_QE_BIT_6_OF_SR_1;

                    /* The command to write into the QE-containing status register */
                    /* The 8-bit command. QE WRSR */
                    device->writeStsRegQeCmd->command  = CY_SMIF_WR_STS_REG1_CMD;
                    device->readStsRegQeCmd->command  = CY_SMIF_RD_STS_REG1_CMD;
                    break;
                case CY_SMIF_SFDP_QER_3:
                    device->stsRegQuadEnableMask = CY_SMIF_SFDP_QE_BIT_7_OF_SR_2;

                    /* The command to write into the QE-containing status register */
                    /* The 8-bit command. QE WRSR */
                    device->writeStsRegQeCmd->command  = CY_SMIF_WR_STS_REG2_CMD;
                    device->readStsRegQeCmd->command  = CY_SMIF_RD_STS_REG2_T2_CMD;
                    break;
                default:
                    break;
            }
        }
        else
        {
            result = CY_SMIF_NO_SFDP_SUPPORT;
        }
    }

    return(result);
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSMIF */

/* [] END OF FILE */
