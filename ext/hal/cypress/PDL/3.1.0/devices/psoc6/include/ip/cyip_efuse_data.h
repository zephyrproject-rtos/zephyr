/***************************************************************************//**
* \file cyip_efuse_data.h
*
* \brief
* EFUSE_DATA IP definitions
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

#ifndef _CYIP_EFUSE_DATA_H_
#define _CYIP_EFUSE_DATA_H_

#include "cyip_headers.h"

/**
  * \brief DEAD access restrictions (DEAD_ACCESS_RESTRICT0)
  */
typedef struct {
    uint8_t AP_CTL_CM0_DISABLE[2];
    uint8_t AP_CTL_CM4_DISABLE[2];
    uint8_t AP_CTL_SYS_DISABLE[2];
    uint8_t SYS_AP_MPU_ENABLE;
    uint8_t DIRECT_EXECUTE_DISABLE;
} cy_stc_dead_access_restrict0_t;

/**
  * \brief DEAD access restrictions (DEAD_ACCESS_RESTRICT1)
  */
typedef struct {
    uint8_t FLASH_ALLOWED[3];
    uint8_t SRAM_ALLOWED[3];
    uint8_t SFLASH_ALLOWED[2];
} cy_stc_dead_access_restrict1_t;

/**
  * \brief SECURE access restrictions (SECURE_ACCESS_RESTRICT0)
  */
typedef struct {
    uint8_t AP_CTL_CM0_DISABLE[2];
    uint8_t AP_CTL_CM4_DISABLE[2];
    uint8_t AP_CTL_SYS_DISABLE[2];
    uint8_t SYS_AP_MPU_ENABLE;
    uint8_t DIRECT_EXECUTE_DISABLE;
} cy_stc_secure_access_restrict0_t;

/**
  * \brief SECURE access restrictions (SECURE_ACCESS_RESTRICT1)
  */
typedef struct {
    uint8_t FLASH_ALLOWED[3];
    uint8_t SRAM_ALLOWED[3];
    uint8_t SFLASH_ALLOWED[2];
} cy_stc_secure_access_restrict1_t;

/**
  * \brief NORMAL, SECURE_WITH_DEBUG, SECURE, and NORMAL_NO_SECURE fuse bits (LIFECYCLE_STAGE)
  */
typedef struct {
    uint8_t NORMAL;
    uint8_t SECURE_WITH_DEBUG;
    uint8_t SECURE;
    uint8_t RMA;
    uint8_t NORMAL_NO_SECURE;
    uint8_t RESERVED;
    uint8_t DEAD_WORK_FLASH_ALLOWED[2];
} cy_stc_lifecycle_stage_t;

/**
  * \brief MMIO_SMIF_ACCESS_RESTRICT (MMIO_SMIF_ACCESS_RESTRICT)
  */
typedef struct {
    uint8_t DEAD_MMIO_ALLOWED[2];
    uint8_t DEAD_SMIF_XIP_ALLOWED;
    uint8_t SECURE_MMIO_ALLOWED[2];
    uint8_t SECURE_SMIF_XIP_ALLOWED;
    uint8_t SECURE_WORK_FLASH_ALLOWED[2];
} cy_stc_mmio_smif_access_restrict_t;

/**
  * \brief Customer data (CUSTOMER_DATA)
  */
typedef struct {
    uint8_t CUSTOMER_USE[8];
} cy_stc_customer_data_t;


/**
  * \brief eFUSE memory (EFUSE_DATA)
  */
typedef struct {
    uint8_t RESERVED[312];
    cy_stc_dead_access_restrict0_t DEAD_ACCESS_RESTRICT0;
    cy_stc_dead_access_restrict1_t DEAD_ACCESS_RESTRICT1;
    cy_stc_secure_access_restrict0_t SECURE_ACCESS_RESTRICT0;
    cy_stc_secure_access_restrict1_t SECURE_ACCESS_RESTRICT1;
    cy_stc_lifecycle_stage_t LIFECYCLE_STAGE;
    uint8_t RESERVED1[152];
    cy_stc_mmio_smif_access_restrict_t MMIO_SMIF_ACCESS_RESTRICT;
    cy_stc_customer_data_t CUSTOMER_DATA[64];
} cy_stc_efuse_data_t;


#endif /* _CYIP_EFUSE_DATA_H_ */


/* [] END OF FILE */
