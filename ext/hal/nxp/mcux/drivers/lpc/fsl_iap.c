/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "fsl_iap.h"
#include "fsl_iap_ffr.h"
#include "fsl_device_registers.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.iap1"
#endif

/*!
 * @addtogroup flash_driver_api
 * @{
*/

#define ROM_API_TREE ((uint32_t *)0x130010f0)
#define BOOTLOADER_API_TREE_POINTER ((bootloader_tree_t *)ROM_API_TREE)

static uint32_t S_Version_minor = 0;

typedef status_t (*EraseCommend_t)(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key);
typedef status_t (*ProgramCommend_t)(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes);
typedef status_t (*VerifyProgramCommend_t)(flash_config_t *config,
                                           uint32_t start,
                                           uint32_t lengthInBytes,
                                           const uint8_t *expectedData,
                                           uint32_t *failedAddress,
                                           uint32_t *failedData);
typedef status_t (*FFR_CustomerPagesInit_t)(flash_config_t *config);
typedef status_t (*FFR_InfieldPageWrite_t)(flash_config_t *config, uint8_t *page_data, uint32_t valid_len);
typedef status_t (*FFR_GetManufactureData_t)(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len);
typedef status_t (*FFR_GetRompatchData_t)(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len);

/*
*!@brief Structure of version property.
*
*!@ingroup bl_core
*/
typedef union BootloaderVersion
{
    struct
    {
        uint32_t bugfix : 8; /*!< bugfix version [7:0] */
        uint32_t minor : 8;  /*!< minor version [15:8] */
        uint32_t major : 8;  /*!< major version [23:16] */
        uint32_t name : 8;   /*!< name [31:24] */
    } B;
    uint32_t version; /*!< combined version numbers. */
} standard_version_t;

/*! @brief Interface for the flash driver.*/
typedef struct FlashDriverInterface
{
    standard_version_t version; /*!< flash driver API version number.*/

    /*!< Flash driver.*/
    status_t (*flash_init)(flash_config_t *config);
    status_t (*flash_erase)(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key);
    status_t (*flash_program)(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes);
    status_t (*flash_verify_erase)(flash_config_t *config, uint32_t start, uint32_t lengthInBytes);
    status_t (*flash_verify_program)(flash_config_t *config,
                                     uint32_t start,
                                     uint32_t lengthInBytes,
                                     const uint8_t *expectedData,
                                     uint32_t *failedAddress,
                                     uint32_t *failedData);
    status_t (*flash_get_property)(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value);
    /*!< Flash FFR driver*/
    status_t (*ffr_init)(flash_config_t *config);
    status_t (*ffr_deinit)(flash_config_t *config);
    status_t (*ffr_cust_factory_page_write)(flash_config_t *config, uint8_t *page_data, bool seal_part);
    status_t (*ffr_get_uuid)(flash_config_t *config, uint8_t *uuid);
    status_t (*ffr_get_customer_data)(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len);
    status_t (*ffr_keystore_write)(flash_config_t *config, ffr_key_store_t *pKeyStore);
    status_t (*ffr_keystore_get_ac)(flash_config_t *config, uint8_t *pActivationCode);
    status_t (*ffr_keystore_get_kc)(flash_config_t *config, uint8_t *pKeyCode, ffr_key_type_t keyIndex);
    status_t (*ffr_infield_page_write)(flash_config_t *config, uint8_t *page_data, uint32_t valid_len);
    status_t (*ffr_get_customer_infield_data)(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len);
} flash_driver_interface_t;

/*!
* @brief Root of the bootloader API tree.
*
* An instance of this struct resides in read-only memory in the bootloader. It
* provides a user application access to APIs exported by the bootloader.
*
* @note The order of existing fields must not be changed.
*/
typedef struct BootloaderTree
{
    void (*runBootloader)(void *arg);            /*!< Function to start the bootloader executing. */
    standard_version_t bootloader_version;       /*!< Bootloader version number. */
    const char *copyright;                       /*!< Copyright string. */
    const uint32_t *reserved;                    /*!< Do NOT use. */
    const flash_driver_interface_t *flashDriver; /*!< Flash driver API. */
} bootloader_tree_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Global pointer to the flash driver API table in ROM. */
flash_driver_interface_t *FLASH_API_TREE;
/*! Get pointer to flash driver API table in ROM. */
#define FLASH_API_TREE BOOTLOADER_API_TREE_POINTER->flashDriver
/*******************************************************************************
 * Code
 ******************************************************************************/

/*! See fsl_flash.h for documentation of this function. */
status_t FLASH_Init(flash_config_t *config)
{
    assert(FLASH_API_TREE);
    config->modeConfig.sysFreqInMHz = kSysToFlashFreq_defaultInMHz;
    S_Version_minor = FLASH_API_TREE->version.B.minor;
    return FLASH_API_TREE->flash_init(config);
}

/*! See fsl_flash.h for documentation of this function. */
status_t FLASH_Erase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key)
{
    if (S_Version_minor == 0)
    {
        EraseCommend_t EraseCommand =
            (EraseCommend_t)(0x1300413b); /*!< get the flash erase api location adress int rom */
        return EraseCommand(config, start, lengthInBytes, key);
    }
    else
    {
        assert(FLASH_API_TREE);
        return FLASH_API_TREE->flash_erase(config, start, lengthInBytes, key);
    }
}

/*! See fsl_flash.h for documentation of this function. */
status_t FLASH_Program(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes)
{
    if (S_Version_minor == 0)
    {
        ProgramCommend_t ProgramCommend =
            (ProgramCommend_t)(0x1300419d); /*!< get the flash program api location adress in rom*/
        return ProgramCommend(config, start, src, lengthInBytes);
    }
    else
    {
        assert(FLASH_API_TREE);
        return FLASH_API_TREE->flash_program(config, start, src, lengthInBytes);
    }
}

/*! See fsl_flash.h for documentation of this function. */
status_t FLASH_VerifyErase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->flash_verify_erase(config, start, lengthInBytes);
}

/*! See fsl_flash.h for documentation of this function. */
status_t FLASH_VerifyProgram(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             const uint8_t *expectedData,
                             uint32_t *failedAddress,
                             uint32_t *failedData)
{
    if (S_Version_minor == 0)
    {
        VerifyProgramCommend_t VerifyProgramCommend =
            (VerifyProgramCommend_t)(0x1300427d); /*!< get the flash verify program api location adress in rom*/
        return VerifyProgramCommend(config, start, lengthInBytes, expectedData, failedAddress, failedData);
    }
    else
    {
        assert(FLASH_API_TREE);
        return FLASH_API_TREE->flash_verify_program(config, start, lengthInBytes, expectedData, failedAddress,
                                                    failedData);
    }
}

/*! See fsl_flash.h for documentation of this function.*/
status_t FLASH_GetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->flash_get_property(config, whichProperty, value);
}
/********************************************************************************
 * fsl_flash_ffr CODE
 *******************************************************************************/

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_Init(flash_config_t *config)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_init(config);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_Deinit(flash_config_t *config)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_deinit(config);
}

status_t FFR_CustomerPagesInit(flash_config_t *config)
{
    assert(FLASH_API_TREE);
    FFR_CustomerPagesInit_t FFR_CustomerPagesInit_cmd = (FFR_CustomerPagesInit_t)(0x13004951);
    return FFR_CustomerPagesInit_cmd(config);
}

status_t FFR_InfieldPageWrite(flash_config_t *config, uint8_t *page_data, uint32_t valid_len)
{
    FFR_InfieldPageWrite_t FFR_InfieldPageWrite_cmd = (FFR_InfieldPageWrite_t)(0x13004a0b);
    return FFR_InfieldPageWrite_cmd(config, page_data, valid_len);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_CustFactoryPageWrite(flash_config_t *config, uint8_t *page_data, bool seal_part)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_cust_factory_page_write(config, page_data, seal_part);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_GetCustomerData(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_get_customer_data(config, pData, offset, len);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_KeystoreWrite(flash_config_t *config, ffr_key_store_t *pKeyStore)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_keystore_write(config, pKeyStore);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_KeystoreGetAC(flash_config_t *config, uint8_t *pActivationCode)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_keystore_get_ac(config, pActivationCode);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_KeystoreGetKC(flash_config_t *config, uint8_t *pKeyCode, ffr_key_type_t keyIndex)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_keystore_get_kc(config, pKeyCode, keyIndex);
}

status_t FFR_GetRompatchData(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len)
{
    FFR_GetRompatchData_t FFR_GetRompatchData_cmd = (FFR_GetRompatchData_t)(0x13004db3);
    return FFR_GetRompatchData_cmd(config, pData, offset, len);
}

/* APIs to access NMPA pages */
status_t FFR_GetManufactureData(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len)
{
    FFR_GetManufactureData_t FFR_GetManufactureData_cmd = (FFR_GetManufactureData_t)(0x13004e15);
    return FFR_GetManufactureData_cmd(config, pData, offset, len);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_GetUUID(flash_config_t *config, uint8_t *uuid)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_get_uuid(config, uuid);
}

/*! See fsl_flash_ffr.h for documentation of this function. */
status_t FFR_GetCustomerInfieldData(flash_config_t *config, uint8_t *pData, uint32_t offset, uint32_t len)
{
    assert(FLASH_API_TREE);
    return FLASH_API_TREE->ffr_get_customer_infield_data(config, pData, offset, len);
}
/*! @}*/

/********************************************************************************
 * EOF
 *******************************************************************************/
