/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_mpu.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Defines the register numbers of the region descriptor configure. */
#define MPU_REGIONDESCRIPTOR_WROD_REGNUM (4U)

/*******************************************************************************
 * Variables
 ******************************************************************************/

const clock_ip_name_t g_mpuClock[FSL_FEATURE_SOC_MPU_COUNT] = MPU_CLOCKS;

/*******************************************************************************
 * Codes
 ******************************************************************************/

void MPU_Init(MPU_Type *base, const mpu_config_t *config)
{
    assert(config);
    uint8_t count;

    /* Un-gate MPU clock */
    CLOCK_EnableClock(g_mpuClock[0]);

    /* Initializes the regions. */
    for (count = 1; count < FSL_FEATURE_MPU_DESCRIPTOR_COUNT; count++)
    {
        base->WORD[count][3] = 0; /* VLD/VID+PID. */
        base->WORD[count][0] = 0; /* Start address. */
        base->WORD[count][1] = 0; /* End address. */
        base->WORD[count][2] = 0; /* Access rights. */
        base->RGDAAC[count] = 0;  /* Alternate access rights. */
    }

    /* MPU configure. */
    while (config)
    {
        MPU_SetRegionConfig(base, &(config->regionConfig));
        config = config->next;
    }
    /* Enable MPU. */
    MPU_Enable(base, true);
}

void MPU_Deinit(MPU_Type *base)
{
    /* Disable MPU. */
    MPU_Enable(base, false);

    /* Gate the clock. */
    CLOCK_DisableClock(g_mpuClock[0]);
}

void MPU_GetHardwareInfo(MPU_Type *base, mpu_hardware_info_t *hardwareInform)
{
    assert(hardwareInform);

    uint32_t cesReg = base->CESR;

    hardwareInform->hardwareRevisionLevel = (cesReg & MPU_CESR_HRL_MASK) >> MPU_CESR_HRL_SHIFT;
    hardwareInform->slavePortsNumbers = (cesReg & MPU_CESR_NSP_MASK) >> MPU_CESR_NSP_SHIFT;
    hardwareInform->regionsNumbers = (mpu_region_total_num_t)((cesReg & MPU_CESR_NRGD_MASK) >> MPU_CESR_NRGD_SHIFT);
}

void MPU_SetRegionConfig(MPU_Type *base, const mpu_region_config_t *regionConfig)
{
    assert(regionConfig);

    uint32_t wordReg = 0;
    uint8_t count;
    uint8_t number = regionConfig->regionNum;

    /* The start and end address of the region descriptor. */
    base->WORD[number][0] = regionConfig->startAddress;
    base->WORD[number][1] = regionConfig->endAddress;

    /* The region descriptor access rights control. */
    for (count = 0; count < MPU_REGIONDESCRIPTOR_WROD_REGNUM; count++)
    {
        wordReg |= MPU_WORD_LOW_MASTER(count, (((uint32_t)regionConfig->accessRights1[count].superAccessRights << 3U) |
                                               (uint8_t)regionConfig->accessRights1[count].userAccessRights)) |
                   MPU_WORD_HIGH_MASTER(count, ((uint32_t)regionConfig->accessRights2[count].readEnable << 1U |
                                                (uint8_t)regionConfig->accessRights2[count].writeEnable));

#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
        wordReg |= MPU_WORD_MASTER_PE(count, regionConfig->accessRights1[count].processIdentifierEnable);
#endif /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */
    }

    /* Set region descriptor access rights. */
    base->WORD[number][2] = wordReg;

    wordReg = MPU_WORD_VLD(1);
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    wordReg |= MPU_WORD_PID(regionConfig->processIdentifier) | MPU_WORD_PIDMASK(regionConfig->processIdMask);
#endif /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */

    base->WORD[number][3] = wordReg;
}

void MPU_SetRegionAddr(MPU_Type *base, mpu_region_num_t regionNum, uint32_t startAddr, uint32_t endAddr)
{
    base->WORD[regionNum][0] = startAddr;
    base->WORD[regionNum][1] = endAddr;
}

void MPU_SetRegionLowMasterAccessRights(MPU_Type *base,
                                        mpu_region_num_t regionNum,
                                        mpu_master_t masterNum,
                                        const mpu_low_masters_access_rights_t *accessRights)
{
    assert(accessRights);
#if FSL_FEATURE_MPU_HAS_MASTER4
    assert(masterNum < kMPU_Master4);
#endif
    uint32_t mask = MPU_WORD_LOW_MASTER_MASK(masterNum);
    uint32_t right = base->RGDAAC[regionNum];

#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    mask |= MPU_LOW_MASTER_PE_MASK(masterNum);
#endif

    /* Build rights control value. */
    right &= ~mask;
    right |= MPU_WORD_LOW_MASTER(masterNum,
                                 ((uint32_t)(accessRights->superAccessRights << 3U) | accessRights->userAccessRights));
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    right |= MPU_WORD_MASTER_PE(masterNum, accessRights->processIdentifierEnable);
#endif /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */

    /* Set low master region access rights. */
    base->RGDAAC[regionNum] = right;
}

void MPU_SetRegionHighMasterAccessRights(MPU_Type *base,
                                         mpu_region_num_t regionNum,
                                         mpu_master_t masterNum,
                                         const mpu_high_masters_access_rights_t *accessRights)
{
    assert(accessRights);
#if FSL_FEATURE_MPU_HAS_MASTER3
    assert(masterNum > kMPU_Master3);
#endif
    uint32_t mask = MPU_WORD_HIGH_MASTER_MASK(masterNum);
    uint32_t right = base->RGDAAC[regionNum];

    /* Build rights control value. */
    right &= ~mask;
    right |= MPU_WORD_HIGH_MASTER((masterNum - (uint8_t)kMPU_RegionNum04),
                                  (((uint32_t)accessRights->readEnable << 1U) | accessRights->writeEnable));
    /* Set low master region access rights. */
    base->RGDAAC[regionNum] = right;
}

bool MPU_GetSlavePortErrorStatus(MPU_Type *base, mpu_slave_t slaveNum)
{
    uint8_t sperr;

    sperr = ((base->CESR & MPU_CESR_SPERR_MASK) >> MPU_CESR_SPERR_SHIFT) & (0x1U << slaveNum);

    return (sperr != 0) ? true : false;
}

void MPU_GetDetailErrorAccessInfo(MPU_Type *base, mpu_slave_t slaveNum, mpu_access_err_info_t *errInform)
{
    assert(errInform);

    uint16_t value;

    /* Error address. */
    errInform->address = base->SP[slaveNum].EAR;

    /* Error detail information. */
    value = (base->SP[slaveNum].EDR & MPU_EDR_EACD_MASK) >> MPU_EDR_EACD_SHIFT;
    if (!value)
    {
        errInform->accessControl = kMPU_NoRegionHit;
    }
    else if (!(value & (uint16_t)(value - 1)))
    {
        errInform->accessControl = kMPU_NoneOverlappRegion;
    }
    else
    {
        errInform->accessControl = kMPU_OverlappRegion;
    }

    value = base->SP[slaveNum].EDR;
    errInform->master = (mpu_master_t)((value & MPU_EDR_EMN_MASK) >> MPU_EDR_EMN_SHIFT);
    errInform->attributes = (mpu_err_attributes_t)((value & MPU_EDR_EATTR_MASK) >> MPU_EDR_EATTR_SHIFT);
    errInform->accessType = (mpu_err_access_type_t)((value & MPU_EDR_ERW_MASK) >> MPU_EDR_ERW_SHIFT);
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    errInform->processorIdentification = (uint8_t)((value & MPU_EDR_EPID_MASK) >> MPU_EDR_EPID_SHIFT);
#endif

    /*!< Clears error slave port bit. */
    value = (base->CESR & ~MPU_CESR_SPERR_MASK) | (0x1U << slaveNum);
    base->CESR = value;
}
