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
 * Variables
 ******************************************************************************/

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
const clock_ip_name_t g_mpuClock[FSL_FEATURE_SOC_MPU_COUNT] = MPU_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/

void MPU_Init(MPU_Type *base, const mpu_config_t *config)
{
    assert(config);
    uint8_t count;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Un-gate MPU clock */
    CLOCK_EnableClock(g_mpuClock[0]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

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

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the clock. */
    CLOCK_DisableClock(g_mpuClock[0]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
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
    assert(regionConfig->regionNum < FSL_FEATURE_MPU_DESCRIPTOR_COUNT);

    uint32_t wordReg = 0;
    uint8_t msPortNum;
    uint8_t regNumber = regionConfig->regionNum;

    /* The start and end address of the region descriptor. */
    base->WORD[regNumber][0] = regionConfig->startAddress;
    base->WORD[regNumber][1] = regionConfig->endAddress;

    /* Set the privilege rights for master 0 ~ master 3. */
    for (msPortNum = 0; msPortNum <= MPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX; msPortNum++)
    {
        wordReg |= MPU_REGION_RWXRIGHTS_MASTER(
            msPortNum, (((uint32_t)regionConfig->accessRights1[msPortNum].superAccessRights << 3U) |
                        (uint32_t)regionConfig->accessRights1[msPortNum].userAccessRights));

#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
        wordReg |=
            MPU_REGION_RWXRIGHTS_MASTER_PE(msPortNum, regionConfig->accessRights1[msPortNum].processIdentifierEnable);
#endif /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */
    }

    /* Set the normal read write rights for master 4 ~ master 7. */
    for (msPortNum = FSL_FEATURE_MPU_PRIVILEGED_RIGHTS_MASTER_COUNT; msPortNum < FSL_FEATURE_MPU_MASTER_COUNT;
         msPortNum++)
    {
        wordReg |= MPU_REGION_RWRIGHTS_MASTER(msPortNum,
            ((uint32_t)regionConfig->accessRights2[msPortNum - FSL_FEATURE_MPU_PRIVILEGED_RIGHTS_MASTER_COUNT].readEnable << 1U |
            (uint32_t)regionConfig->accessRights2[msPortNum - FSL_FEATURE_MPU_PRIVILEGED_RIGHTS_MASTER_COUNT].writeEnable));
    }

    /* Set region descriptor access rights. */
    base->WORD[regNumber][2] = wordReg;

    wordReg = MPU_WORD_VLD(1);
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    wordReg |= MPU_WORD_PID(regionConfig->processIdentifier) | MPU_WORD_PIDMASK(regionConfig->processIdMask);
#endif /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */

    base->WORD[regNumber][3] = wordReg;
}

void MPU_SetRegionAddr(MPU_Type *base, uint32_t regionNum, uint32_t startAddr, uint32_t endAddr)
{
    assert(regionNum < FSL_FEATURE_MPU_DESCRIPTOR_COUNT);

    base->WORD[regionNum][0] = startAddr;
    base->WORD[regionNum][1] = endAddr;
}

void MPU_SetRegionRwxMasterAccessRights(MPU_Type *base,
                                        uint32_t regionNum,
                                        uint32_t masterNum,
                                        const mpu_rwxrights_master_access_control_t *accessRights)
{
    assert(accessRights);
    assert(regionNum < FSL_FEATURE_MPU_DESCRIPTOR_COUNT);
    assert(masterNum <= MPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX);

    uint32_t mask = MPU_REGION_RWXRIGHTS_MASTER_MASK(masterNum);
    uint32_t right = base->RGDAAC[regionNum];

#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    mask |= MPU_REGION_RWXRIGHTS_MASTER_PE_MASK(masterNum);
#endif

    /* Build rights control value. */
    right &= ~mask;
    right |= MPU_REGION_RWXRIGHTS_MASTER(
        masterNum, ((uint32_t)(accessRights->superAccessRights << 3U) | accessRights->userAccessRights));
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    right |= MPU_REGION_RWXRIGHTS_MASTER_PE(masterNum, accessRights->processIdentifierEnable);
#endif /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */

    /* Set low master region access rights. */
    base->RGDAAC[regionNum] = right;
}

#if FSL_FEATURE_MPU_HAS_MASTER_4_7
void MPU_SetRegionRwMasterAccessRights(MPU_Type *base,
                                       uint32_t regionNum,
                                       uint32_t masterNum,
                                       const mpu_rwrights_master_access_control_t *accessRights)
{
    assert(accessRights);
    assert(regionNum < FSL_FEATURE_MPU_DESCRIPTOR_COUNT);
    assert(masterNum > MPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX);
    assert(masterNum <= FSL_FEATURE_MPU_MASTER_MAX_INDEX);

    uint32_t mask = MPU_REGION_RWRIGHTS_MASTER_MASK(masterNum);
    uint32_t right = base->RGDAAC[regionNum];

    /* Build rights control value. */
    right &= ~mask;
    right |=
        MPU_REGION_RWRIGHTS_MASTER(masterNum, (((uint32_t)accessRights->readEnable << 1U) | accessRights->writeEnable));
    /* Set low master region access rights. */
    base->RGDAAC[regionNum] = right;
}
#endif /* FSL_FEATURE_MPU_HAS_MASTER_4_7 */

bool MPU_GetSlavePortErrorStatus(MPU_Type *base, mpu_slave_t slaveNum)
{
    uint8_t sperr;

    sperr = ((base->CESR & MPU_CESR_SPERR_MASK) >> MPU_CESR_SPERR_SHIFT) & (0x1U << (MPU_SLAVE_PORT_NUM - slaveNum));

    return (sperr != 0) ? true : false;
}

void MPU_GetDetailErrorAccessInfo(MPU_Type *base, mpu_slave_t slaveNum, mpu_access_err_info_t *errInform)
{
    assert(errInform);

    uint16_t value;
    uint32_t cesReg;

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
    errInform->master = (uint32_t)((value & MPU_EDR_EMN_MASK) >> MPU_EDR_EMN_SHIFT);
    errInform->attributes = (mpu_err_attributes_t)((value & MPU_EDR_EATTR_MASK) >> MPU_EDR_EATTR_SHIFT);
    errInform->accessType = (mpu_err_access_type_t)((value & MPU_EDR_ERW_MASK) >> MPU_EDR_ERW_SHIFT);
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    errInform->processorIdentification = (uint8_t)((value & MPU_EDR_EPID_MASK) >> MPU_EDR_EPID_SHIFT);
#endif

    /* Clears error slave port bit. */
    cesReg = (base->CESR & ~MPU_CESR_SPERR_MASK) | ((0x1U << slaveNum) << MPU_CESR_SPERR_SHIFT);
    base->CESR = cesReg;
}
