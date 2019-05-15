/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_prince.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */

#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.prince"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * brief Generate new IV code.
 *
 * This function generates new IV code and stores it into the persistent memory.
 * Ensure about 800 bytes free space on the stack when calling this routine with the store parameter set to true!
 *
 * param region PRINCE region index.
 * param iv_code IV code pointer used for storing the newly generated 52 bytes long IV code.
 * param store flag to allow storing the newly generated IV code into the persistent memory (FFR).
 * param flash_context pointer to the flash driver context structure.
 *
 * return kStatus_Success upon success
 * return kStatus_Fail    otherwise, kStatus_Fail is also returned if the key code for the particular
 *                         PRINCE region is not present in the keystore (though new IV code has been provided)
 */
status_t PRINCE_GenNewIV(prince_region_t region, uint8_t *iv_code, bool store, flash_config_t *flash_context)
{
    status_t retVal = kStatus_Fail;
    uint8_t prince_iv_code[FLASH_FFR_IV_CODE_SIZE] = {0};
    uint8_t tempBuffer[FLASH_FFR_MAX_PAGE_SIZE] = {0};

    if (((SYSCON->PERIPHENCFG & SYSCON_PERIPHENCFG_PRINCEEN_MASK) ? true : false) == false)
    {
        return retVal; /* PRINCE peripheral not enabled, return kStatus_Fail. */
    }

    /* Make sure PUF is started to allow key and IV code decryption and generation */
    if (true != PUF_IsGetKeyAllowed(PUF))
    {
        return retVal;
    }

    /* Generate new IV code for the PRINCE region */
    retVal = PUF_SetIntrinsicKey(PUF, (puf_key_index_register_t)(kPUF_KeyIndex_02 + (puf_key_index_register_t)region),
                                 8, &prince_iv_code[0], FLASH_FFR_IV_CODE_SIZE);
    if ((kStatus_Success == retVal) && (true == store))
    {
        /* Store the new IV code for the PRINCE region into the respective FFRs. */
        /* Create a new version of "Customer Field Programmable" (CFP) page. */
        if (kStatus_FLASH_Success ==
            FFR_GetCustomerInfieldData(flash_context, (uint8_t *)tempBuffer, 0, FLASH_FFR_MAX_PAGE_SIZE))
        {
            /* Set the IV code in the page */
            memcpy(&tempBuffer[offsetof(cfpa_cfg_info_t, ivCodePrinceRegion) + ((region * sizeof(cfpa_cfg_iv_code_t))) +
                               4],
                   &prince_iv_code[0], FLASH_FFR_IV_CODE_SIZE);

            uint32_t *p32 = (uint32_t *)tempBuffer;
            uint32_t version = p32[1];
            if (version == 0xFFFFFFFFu)
            {
                return kStatus_Fail;
            }
            version++;
            p32[1] = version;

            /* Program the page and enable firewall for "Customer field area" */
            if (kStatus_FLASH_Success ==
                FFR_InfieldPageWrite(flash_context, (uint8_t *)tempBuffer, FLASH_FFR_MAX_PAGE_SIZE))
            {
                retVal = kStatus_Success;
            }
            else
            {
                retVal = kStatus_Fail;
            }
        }
    }
    if (retVal == kStatus_Success)
    {
        /* Pass the new IV code */
        memcpy(iv_code, &prince_iv_code[0], FLASH_FFR_IV_CODE_SIZE);
    }
    return retVal;
}

/*!
 * brief Load IV code.
 *
 * This function enables IV code loading into the PRINCE bus encryption engine.
 *
 * param region PRINCE region index.
 * param iv_code IV code pointer used for passing the IV code.
 *
 * return kStatus_Success upon success
 * return kStatus_Fail    otherwise
 */
status_t PRINCE_LoadIV(prince_region_t region, uint8_t *iv_code)
{
    status_t retVal = kStatus_Fail;
    uint32_t keyIndex = 0x0Fu & iv_code[1];
    uint8_t prince_iv[8] = {0};

    if (((SYSCON->PERIPHENCFG & SYSCON_PERIPHENCFG_PRINCEEN_MASK) ? true : false) == false)
    {
        return retVal; /* PRINCE peripheral not enabled, return kStatus_Fail. */
    }

    /* Make sure PUF is started to allow key and IV code decryption and generation */
    if (true != PUF_IsGetKeyAllowed(PUF))
    {
        return retVal;
    }

    /* Check if region number matches the PUF index value */
    if ((kPUF_KeyIndex_02 + (puf_key_index_register_t)region) == (puf_key_index_register_t)keyIndex)
    {
        /* Decrypt the IV */
        if (kStatus_Success == PUF_GetKey(PUF, iv_code, FLASH_FFR_IV_CODE_SIZE, &prince_iv[0], 8))
        {
            /* Store the new IV for the PRINCE region into PRINCE registers. */
            PRINCE_SetRegionIV(PRINCE, (prince_region_t)region, prince_iv);
            retVal = kStatus_Success;
        }
    }
    return retVal;
}

/*!
 * brief Allow encryption/decryption for specified address range.
 *
 * This function sets the encryption/decryption for specified address range.
 * Ensure about 800 bytes free space on the stack when calling this routine!
 *
 * param region PRINCE region index.
 * param start_address start address of the area to be encrypted/decrypted.
 * param length length of the area to be encrypted/decrypted.
 * param flash_context pointer to the flash driver context structure.
 *
 * return kStatus_Success upon success
 * return kStatus_Fail    otherwise
 */
status_t PRINCE_SetEncryptForAddressRange(prince_region_t region,
                                          uint32_t start_address,
                                          uint32_t length,
                                          flash_config_t *flash_context)
{
    status_t retVal = kStatus_Fail;
    uint32_t srEnableRegister = 0;
    uint32_t alignedStartAddress;
    uint32_t end_address = start_address + length;
    uint32_t prince_region_base_address = 0;
    uint8_t my_prince_iv_code[52] = {0};
    uint8_t tempBuffer[512] = {0};
    uint32_t prince_base_addr_ffr_word = 0;

    /* Check the address range, regions overlaping. */
    if ((start_address > 0xA0000) || ((start_address < 0x40000) && (end_address > 0x40000)) ||
        ((start_address < 0x80000) && (end_address > 0x80000)) ||
        ((start_address < 0xA0000) && (end_address > 0xA0000)))
    {
        return kStatus_Fail;
    }

    /* Generate new IV code for the PRINCE region and store the new IV into the respective FFRs */
    retVal = PRINCE_GenNewIV((prince_region_t)region, &my_prince_iv_code[0], true, flash_context);
    if (kStatus_Success != retVal)
    {
        return kStatus_Fail;
    }

    /* Store the new IV for the PRINCE region into PRINCE registers. */
    retVal = PRINCE_LoadIV((prince_region_t)region, &my_prince_iv_code[0]);
    if (kStatus_Success != retVal)
    {
        return kStatus_Fail;
    }

    alignedStartAddress = ALIGN_DOWN(start_address, FSL_PRINCE_DRIVER_SUBREGION_SIZE_IN_KB * 1024);

    uint32_t subregion = alignedStartAddress / (FSL_PRINCE_DRIVER_SUBREGION_SIZE_IN_KB * 1024);
    if (subregion < (32))
    {
        /* PRINCE_Region0 */
        prince_region_base_address = 0;
    }
    else if (subregion < (64))
    {
        /* PRINCE_Region1 */
        subregion = subregion - 32;
        prince_region_base_address = 0x40000;
    }
    else
    {
        /* PRINCE_Region2 */
        subregion = subregion - 64;
        prince_region_base_address = 0x80000;
    }

    srEnableRegister = (1 << subregion);
    alignedStartAddress += (FSL_PRINCE_DRIVER_SUBREGION_SIZE_IN_KB * 1024);

    while (alignedStartAddress < (start_address + length))
    {
        subregion++;
        srEnableRegister |= (1 << subregion);
        alignedStartAddress += (FSL_PRINCE_DRIVER_SUBREGION_SIZE_IN_KB * 1024);
    }

    /* Store BASE_ADDR into PRINCE register before storing the SR to avoid en/decryption triggering
       from addresses being defined by current BASE_ADDR register content (could be 0 and the decryption
       of actually executed code can be started causing the hardfault then). */
    retVal = PRINCE_SetRegionBaseAddress(PRINCE, (prince_region_t)region, prince_region_base_address);
    if (kStatus_Success != retVal)
    {
        return retVal;
    }

    /* Store SR into PRINCE register */
    retVal = PRINCE_SetRegionSREnable(PRINCE, (prince_region_t)region, srEnableRegister);
    if (kStatus_Success != retVal)
    {
        return retVal;
    }

    /* Store SR and BASE_ADDR into CMPA FFR */
    if (kStatus_Success == FFR_GetCustomerData(flash_context, (uint8_t *)&tempBuffer, 0, FLASH_FFR_MAX_PAGE_SIZE))
    {
        /* Set the PRINCE_SR_X in the page */
        memcpy(&tempBuffer[offsetof(cmpa_cfg_info_t, princeSr) + (region * sizeof(uint32_t))], &srEnableRegister,
               sizeof(uint32_t));

        /* Set the ADDRX_PRG in the page */
        memcpy(&prince_base_addr_ffr_word, &tempBuffer[offsetof(cmpa_cfg_info_t, princeBaseAddr)], sizeof(uint32_t));
        prince_base_addr_ffr_word &= ~((FLASH_CMPA_PRINCE_BASE_ADDR_ADDR0_PRG_MASK) << (region * 4));
        prince_base_addr_ffr_word |=
            (((prince_region_base_address >> 18) & FLASH_CMPA_PRINCE_BASE_ADDR_ADDR0_PRG_MASK) << (region * 4));
        memcpy(&tempBuffer[offsetof(cmpa_cfg_info_t, princeBaseAddr)], &prince_base_addr_ffr_word, sizeof(uint32_t));

        /* Program the CMPA page, set seal_part parameter to false (used during development to avoid sealing the part)
         */
        retVal = FFR_CustFactoryPageWrite(flash_context, (uint8_t *)tempBuffer, false);
    }

    return retVal;
}

/*!
 * brief Gets the PRINCE Sub-Region Enable register.
 *
 * This function gets PRINCE SR_ENABLE register.
 *
 * param base PRINCE peripheral address.
 * param region PRINCE region index.
 * param sr_enable Sub-Region Enable register pointer.
 *
 * return kStatus_Success upon success
 * return kStatus_InvalidArgument
 */
status_t PRINCE_GetRegionSREnable(PRINCE_Type *base, prince_region_t region, uint32_t *sr_enable)
{
    status_t status = kStatus_Success;

    switch (region)
    {
        case kPRINCE_Region0:
            *sr_enable = base->SR_ENABLE0;
            break;

        case kPRINCE_Region1:
            *sr_enable = base->SR_ENABLE1;
            break;

        case kPRINCE_Region2:
            *sr_enable = base->SR_ENABLE2;
            break;

        default:
            status = kStatus_InvalidArgument;
            break;
    }

    return status;
}

/*!
 * brief Gets the PRINCE region base address register.
 *
 * This function gets PRINCE BASE_ADDR register.
 *
 * param base PRINCE peripheral address.
 * param region PRINCE region index.
 * param region_base_addr Region base address pointer.
 *
 * return kStatus_Success upon success
 * return kStatus_InvalidArgument
 */
status_t PRINCE_GetRegionBaseAddress(PRINCE_Type *base, prince_region_t region, uint32_t *region_base_addr)
{
    status_t status = kStatus_Success;

    switch (region)
    {
        case kPRINCE_Region0:
            *region_base_addr = base->BASE_ADDR0;
            break;

        case kPRINCE_Region1:
            *region_base_addr = base->BASE_ADDR1;
            break;

        case kPRINCE_Region2:
            *region_base_addr = base->BASE_ADDR2;
            break;

        default:
            status = kStatus_InvalidArgument;
            break;
    }

    return status;
}

/*!
 * @brief Sets the PRINCE region IV.
 *
 * This function sets specified AES IV for the given region.
 *
 * @param base PRINCE peripheral address.
 * @param region Selection of the PRINCE region to be configured.
 * @param iv 64-bit AES IV in little-endian byte order.
 */
status_t PRINCE_SetRegionIV(PRINCE_Type *base, prince_region_t region, const uint8_t iv[8])
{
    status_t status = kStatus_Fail;
    volatile uint32_t *IVMsb_reg = NULL;
    volatile uint32_t *IVLsb_reg = NULL;

    switch (region)
    {
        case kPRINCE_Region0:
            IVLsb_reg = &base->IV_LSB0;
            IVMsb_reg = &base->IV_MSB0;
            break;

        case kPRINCE_Region1:
            IVLsb_reg = &base->IV_LSB1;
            IVMsb_reg = &base->IV_MSB1;
            break;

        case kPRINCE_Region2:
            IVLsb_reg = &base->IV_LSB2;
            IVMsb_reg = &base->IV_MSB2;
            break;

        default:
            status = kStatus_InvalidArgument;
            break;
    }

    if (status != kStatus_InvalidArgument)
    {
        *IVLsb_reg = ((uint32_t *)(uintptr_t)iv)[0];
        *IVMsb_reg = ((uint32_t *)(uintptr_t)iv)[1];
        status = kStatus_Success;
    }

    return status;
}

/*!
 * @brief Sets the PRINCE region base address.
 *
 * This function configures PRINCE region base address.
 *
 * @param base PRINCE peripheral address.
 * @param region Selection of the PRINCE region to be configured.
 * @param region_base_addr Base Address for region.
 */
status_t PRINCE_SetRegionBaseAddress(PRINCE_Type *base, prince_region_t region, uint32_t region_base_addr)
{
    status_t status = kStatus_Success;

    switch (region)
    {
        case kPRINCE_Region0:
            base->BASE_ADDR0 = region_base_addr;
            break;

        case kPRINCE_Region1:
            base->BASE_ADDR1 = region_base_addr;
            break;

        case kPRINCE_Region2:
            base->BASE_ADDR2 = region_base_addr;
            break;

        default:
            status = kStatus_InvalidArgument;
            break;
    }

    return status;
}

/*!
 * @brief Sets the PRINCE Sub-Region Enable register.
 *
 * This function configures PRINCE SR_ENABLE register.
 *
 * @param base PRINCE peripheral address.
 * @param region Selection of the PRINCE region to be configured.
 * @param sr_enable Sub-Region Enable register value.
 */
status_t PRINCE_SetRegionSREnable(PRINCE_Type *base, prince_region_t region, uint32_t sr_enable)
{
    status_t status = kStatus_Success;

    switch (region)
    {
        case kPRINCE_Region0:
            base->SR_ENABLE0 = sr_enable;
            break;

        case kPRINCE_Region1:
            base->SR_ENABLE1 = sr_enable;
            break;

        case kPRINCE_Region2:
            base->SR_ENABLE2 = sr_enable;
            break;

        default:
            status = kStatus_InvalidArgument;
            break;
    }

    return status;
}
