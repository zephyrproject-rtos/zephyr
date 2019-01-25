/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_sysmpu.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.sysmpu"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
static const clock_ip_name_t g_sysmpuClock[] = SYSMPU_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/

/*!
 * brief Initializes the SYSMPU with the user configuration structure.
 *
 * This function configures the SYSMPU module with the user-defined configuration.
 *
 * param base     SYSMPU peripheral base address.
 * param config   The pointer to the configuration structure.
 */
void SYSMPU_Init(SYSMPU_Type *base, const sysmpu_config_t *config)
{
    assert(config);
    uint8_t count;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Un-gate SYSMPU clock */
    CLOCK_EnableClock(g_sysmpuClock[0]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Initializes the regions. */
    for (count = 1; count < FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT; count++)
    {
        base->WORD[count][3] = 0; /* VLD/VID+PID. */
        base->WORD[count][0] = 0; /* Start address. */
        base->WORD[count][1] = 0; /* End address. */
        base->WORD[count][2] = 0; /* Access rights. */
        base->RGDAAC[count] = 0;  /* Alternate access rights. */
    }

    /* SYSMPU configure. */
    while (config)
    {
        SYSMPU_SetRegionConfig(base, &(config->regionConfig));
        config = config->next;
    }
    /* Enable SYSMPU. */
    SYSMPU_Enable(base, true);
}

/*!
 * brief Deinitializes the SYSMPU regions.
 *
 * param base     SYSMPU peripheral base address.
 */
void SYSMPU_Deinit(SYSMPU_Type *base)
{
    /* Disable SYSMPU. */
    SYSMPU_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the clock. */
    CLOCK_DisableClock(g_sysmpuClock[0]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Gets the SYSMPU basic hardware information.
 *
 * param base           SYSMPU peripheral base address.
 * param hardwareInform The pointer to the SYSMPU hardware information structure. See "sysmpu_hardware_info_t".
 */
void SYSMPU_GetHardwareInfo(SYSMPU_Type *base, sysmpu_hardware_info_t *hardwareInform)
{
    assert(hardwareInform);

    uint32_t cesReg = base->CESR;

    hardwareInform->hardwareRevisionLevel = (cesReg & SYSMPU_CESR_HRL_MASK) >> SYSMPU_CESR_HRL_SHIFT;
    hardwareInform->slavePortsNumbers = (cesReg & SYSMPU_CESR_NSP_MASK) >> SYSMPU_CESR_NSP_SHIFT;
    hardwareInform->regionsNumbers =
        (sysmpu_region_total_num_t)((cesReg & SYSMPU_CESR_NRGD_MASK) >> SYSMPU_CESR_NRGD_SHIFT);
}

/*!
 * brief Sets the SYSMPU region.
 *
 * Note: Due to the SYSMPU protection, the region number 0 does not allow writes from
 * core to affect the start and end address nor the permissions associated with
 * the debugger. It can only write the permission fields associated
 * with the other masters.
 *
 * param base          SYSMPU peripheral base address.
 * param regionConfig  The pointer to the SYSMPU user configuration structure. See "sysmpu_region_config_t".
 */
void SYSMPU_SetRegionConfig(SYSMPU_Type *base, const sysmpu_region_config_t *regionConfig)
{
    assert(regionConfig);
    assert(regionConfig->regionNum < FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT);

    uint32_t wordReg = 0;
    uint8_t msPortNum;
    uint8_t regNumber = regionConfig->regionNum;

    /* The start and end address of the region descriptor. */
    base->WORD[regNumber][0] = regionConfig->startAddress;
    base->WORD[regNumber][1] = regionConfig->endAddress;

    /* Set the privilege rights for master 0 ~ master 3. */
    for (msPortNum = 0; msPortNum < SYSMPU_MASTER_RWATTRIBUTE_START_PORT; msPortNum++)
    {
        wordReg |= SYSMPU_REGION_RWXRIGHTS_MASTER(
            msPortNum, (((uint32_t)regionConfig->accessRights1[msPortNum].superAccessRights << 3U) |
                        (uint32_t)regionConfig->accessRights1[msPortNum].userAccessRights));

#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
        wordReg |= SYSMPU_REGION_RWXRIGHTS_MASTER_PE(msPortNum,
                                                     regionConfig->accessRights1[msPortNum].processIdentifierEnable);
#endif /* FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER */
    }

#if FSL_FEATURE_SYSMPU_MASTER_COUNT > SYSMPU_MASTER_RWATTRIBUTE_START_PORT
    /* Set the normal read write rights for master 4 ~ master 7. */
    for (msPortNum = SYSMPU_MASTER_RWATTRIBUTE_START_PORT; msPortNum < FSL_FEATURE_SYSMPU_MASTER_COUNT; msPortNum++)
    {
        wordReg |= SYSMPU_REGION_RWRIGHTS_MASTER(
            msPortNum,
            ((uint32_t)regionConfig->accessRights2[msPortNum - SYSMPU_MASTER_RWATTRIBUTE_START_PORT].readEnable << 1U |
             (uint32_t)regionConfig->accessRights2[msPortNum - SYSMPU_MASTER_RWATTRIBUTE_START_PORT].writeEnable));
    }
#endif /* FSL_FEATURE_SYSMPU_MASTER_COUNT > SYSMPU_MASTER_RWATTRIBUTE_START_PORT */

    /* Set region descriptor access rights. */
    base->WORD[regNumber][2] = wordReg;

    wordReg = SYSMPU_WORD_VLD(1);
#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
    wordReg |= SYSMPU_WORD_PID(regionConfig->processIdentifier) | SYSMPU_WORD_PIDMASK(regionConfig->processIdMask);
#endif /* FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER */

    base->WORD[regNumber][3] = wordReg;
}

/*!
 * brief Sets the region start and end address.
 *
 * Memory region start address. Note: bit0 ~ bit4 is always marked as 0 by SYSMPU.
 * The actual start address by SYSMPU is 0-modulo-32 byte address.
 * Memory region end address. Note: bit0 ~ bit4 always be marked as 1 by SYSMPU.
 * The end address used by the SYSMPU is 31-modulo-32 byte address.
 * Note: Due to the SYSMPU protection, the startAddr and endAddr can't be
 * changed by the core when regionNum is 0.
 *
 * param base          SYSMPU peripheral base address.
 * param regionNum     SYSMPU region number. The range is from 0 to
 * FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT - 1.
 * param startAddr     Region start address.
 * param endAddr       Region end address.
 */
void SYSMPU_SetRegionAddr(SYSMPU_Type *base, uint32_t regionNum, uint32_t startAddr, uint32_t endAddr)
{
    assert(regionNum < FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT);

    base->WORD[regionNum][0] = startAddr;
    base->WORD[regionNum][1] = endAddr;
}

/*!
 * brief Sets the SYSMPU region access rights for masters with read, write, and execute rights.
 * The SYSMPU access rights depend on two board classifications of bus masters.
 * The privilege rights masters and the normal rights masters.
 * The privilege rights masters have the read, write, and execute access rights.
 * Except the normal read and write rights, the execute rights are also
 * allowed for these masters. The privilege rights masters normally range from
 * bus masters 0 - 3. However, the maximum master number is device-specific.
 * See the "SYSMPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX".
 * The normal rights masters access rights control see
 * "SYSMPU_SetRegionRwMasterAccessRights()".
 *
 * param base          SYSMPU peripheral base address.
 * param regionNum     SYSMPU region number. Should range from 0 to
 * FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT - 1.
 * param masterNum     SYSMPU bus master number. Should range from 0 to
 * SYSMPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX.
 * param accessRights  The pointer to the SYSMPU access rights configuration. See
 * "sysmpu_rwxrights_master_access_control_t".
 */
void SYSMPU_SetRegionRwxMasterAccessRights(SYSMPU_Type *base,
                                           uint32_t regionNum,
                                           uint32_t masterNum,
                                           const sysmpu_rwxrights_master_access_control_t *accessRights)
{
    assert(accessRights);
    assert(regionNum < FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT);
    assert(masterNum < SYSMPU_MASTER_RWATTRIBUTE_START_PORT);

    uint32_t mask = SYSMPU_REGION_RWXRIGHTS_MASTER_MASK(masterNum);
    uint32_t right = base->RGDAAC[regionNum];

#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
    mask |= SYSMPU_REGION_RWXRIGHTS_MASTER_PE_MASK(masterNum);
#endif

    /* Build rights control value. */
    right &= ~mask;
    right |= SYSMPU_REGION_RWXRIGHTS_MASTER(
        masterNum, ((uint32_t)(accessRights->superAccessRights << 3U) | accessRights->userAccessRights));
#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
    right |= SYSMPU_REGION_RWXRIGHTS_MASTER_PE(masterNum, accessRights->processIdentifierEnable);
#endif /* FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER */

    /* Set low master region access rights. */
    base->RGDAAC[regionNum] = right;
}

#if FSL_FEATURE_SYSMPU_MASTER_COUNT > 4
/*!
 * brief Sets the SYSMPU region access rights for masters with read and write rights.
 * The SYSMPU access rights depend on two board classifications of bus masters.
 * The privilege rights masters and the normal rights masters.
 * The normal rights masters only have the read and write access permissions.
 * The privilege rights access control see "SYSMPU_SetRegionRwxMasterAccessRights".
 *
 * param base          SYSMPU peripheral base address.
 * param regionNum     SYSMPU region number. The range is from 0 to
 * FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT - 1.
 * param masterNum     SYSMPU bus master number. Should range from SYSMPU_MASTER_RWATTRIBUTE_START_PORT
 * to ~ FSL_FEATURE_SYSMPU_MASTER_COUNT - 1.
 * param accessRights  The pointer to the SYSMPU access rights configuration. See
 * "sysmpu_rwrights_master_access_control_t".
 */
void SYSMPU_SetRegionRwMasterAccessRights(SYSMPU_Type *base,
                                          uint32_t regionNum,
                                          uint32_t masterNum,
                                          const sysmpu_rwrights_master_access_control_t *accessRights)
{
    assert(accessRights);
    assert(regionNum < FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT);
    assert(masterNum >= SYSMPU_MASTER_RWATTRIBUTE_START_PORT);
    assert(masterNum <= (FSL_FEATURE_SYSMPU_MASTER_COUNT - 1));

    uint32_t mask = SYSMPU_REGION_RWRIGHTS_MASTER_MASK(masterNum);
    uint32_t right = base->RGDAAC[regionNum];

    /* Build rights control value. */
    right &= ~mask;
    right |= SYSMPU_REGION_RWRIGHTS_MASTER(masterNum,
                                           (((uint32_t)accessRights->readEnable << 1U) | accessRights->writeEnable));
    /* Set low master region access rights. */
    base->RGDAAC[regionNum] = right;
}
#endif /* FSL_FEATURE_SYSMPU_MASTER_COUNT > 4 */

/*!
 * brief Gets the numbers of slave ports where errors occur.
 *
 * param base       SYSMPU peripheral base address.
 * param slaveNum   SYSMPU slave port number.
 * return The slave ports error status.
 *         true  - error happens in this slave port.
 *         false - error didn't happen in this slave port.
 */
bool SYSMPU_GetSlavePortErrorStatus(SYSMPU_Type *base, sysmpu_slave_t slaveNum)
{
    uint8_t sperr;

    sperr = ((base->CESR & SYSMPU_CESR_SPERR_MASK) >> SYSMPU_CESR_SPERR_SHIFT) &
            (0x1U << (FSL_FEATURE_SYSMPU_SLAVE_COUNT - slaveNum - 1));

    return (sperr != 0) ? true : false;
}

/*!
 * brief Gets the SYSMPU detailed error access information.
 *
 * param base       SYSMPU peripheral base address.
 * param slaveNum   SYSMPU slave port number.
 * param errInform  The pointer to the SYSMPU access error information. See "sysmpu_access_err_info_t".
 */
void SYSMPU_GetDetailErrorAccessInfo(SYSMPU_Type *base, sysmpu_slave_t slaveNum, sysmpu_access_err_info_t *errInform)
{
    assert(errInform);

    uint16_t value;
    uint32_t cesReg;

    /* Error address. */
    errInform->address = base->SP[slaveNum].EAR;

    /* Error detail information. */
    value = (base->SP[slaveNum].EDR & SYSMPU_EDR_EACD_MASK) >> SYSMPU_EDR_EACD_SHIFT;
    if (!value)
    {
        errInform->accessControl = kSYSMPU_NoRegionHit;
    }
    else if (!(value & (uint16_t)(value - 1)))
    {
        errInform->accessControl = kSYSMPU_NoneOverlappRegion;
    }
    else
    {
        errInform->accessControl = kSYSMPU_OverlappRegion;
    }

    value = base->SP[slaveNum].EDR;
    errInform->master = (uint32_t)((value & SYSMPU_EDR_EMN_MASK) >> SYSMPU_EDR_EMN_SHIFT);
    errInform->attributes = (sysmpu_err_attributes_t)((value & SYSMPU_EDR_EATTR_MASK) >> SYSMPU_EDR_EATTR_SHIFT);
    errInform->accessType = (sysmpu_err_access_type_t)((value & SYSMPU_EDR_ERW_MASK) >> SYSMPU_EDR_ERW_SHIFT);
#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
    errInform->processorIdentification = (uint8_t)((value & SYSMPU_EDR_EPID_MASK) >> SYSMPU_EDR_EPID_SHIFT);
#endif

    /* Clears error slave port bit. */
    cesReg = (base->CESR & ~SYSMPU_CESR_SPERR_MASK) |
             ((0x1U << (FSL_FEATURE_SYSMPU_SLAVE_COUNT - slaveNum - 1)) << SYSMPU_CESR_SPERR_SHIFT);
    base->CESR = cesReg;
}
