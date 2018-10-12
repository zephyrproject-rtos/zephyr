/***************************************************************************//**
* \file cyip_sflash.h
*
* \brief
* SFLASH IP definitions
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

#ifndef _CYIP_SFLASH_H_
#define _CYIP_SFLASH_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    SFLASH
*******************************************************************************/

#define SFLASH_SECTION_SIZE                     0x00008000UL

/**
  * \brief FLASH Supervisory Region (SFLASH)
  */
typedef struct {
   __IM uint8_t  RESERVED;
  __IOM uint8_t  SI_REVISION_ID;                /*!< 0x00000001 Indicates Silicon Revision ID of the device */
  __IOM uint16_t SILICON_ID;                    /*!< 0x00000002 Indicates Silicon ID of the device */
   __IM uint32_t RESERVED1[2];
  __IOM uint16_t FAMILY_ID;                     /*!< 0x0000000C Indicates Family ID of the device */
   __IM uint16_t RESERVED2[761];
  __IOM uint8_t  DIE_LOT[3];                    /*!< 0x00000600 Lot Number (3 bytes) */
  __IOM uint8_t  DIE_WAFER;                     /*!< 0x00000603 Wafer Number */
  __IOM uint8_t  DIE_X;                         /*!< 0x00000604 X Position on Wafer, CRI Pass/Fail Bin */
  __IOM uint8_t  DIE_Y;                         /*!< 0x00000605 Y Position on Wafer, CHI Pass/Fail Bin */
  __IOM uint8_t  DIE_SORT;                      /*!< 0x00000606 Sort1/2/3 Pass/Fail Bin */
  __IOM uint8_t  DIE_MINOR;                     /*!< 0x00000607 Minor Revision Number */
  __IOM uint8_t  DIE_DAY;                       /*!< 0x00000608 Day number */
  __IOM uint8_t  DIE_MONTH;                     /*!< 0x00000609 Month number */
  __IOM uint8_t  DIE_YEAR;                      /*!< 0x0000060A Year number */
   __IM uint8_t  RESERVED3[61];
  __IOM uint16_t SAR_TEMP_MULTIPLIER;           /*!< 0x00000648 SAR Temperature Sensor Multiplication Factor */
  __IOM uint16_t SAR_TEMP_OFFSET;               /*!< 0x0000064A SAR Temperature Sensor Offset */
   __IM uint32_t RESERVED4[8];
  __IOM uint32_t CSP_PANEL_ID;                  /*!< 0x0000066C CSP Panel Id to record panel ID of CSP die */
   __IM uint32_t RESERVED5[52];
  __IOM uint8_t  LDO_0P9V_TRIM;                 /*!< 0x00000740 LDO_0P9V_TRIM */
  __IOM uint8_t  LDO_1P1V_TRIM;                 /*!< 0x00000741 LDO_1P1V_TRIM */
   __IM uint16_t RESERVED6[95];
  __IOM uint32_t BLE_DEVICE_ADDRESS[128];       /*!< 0x00000800 BLE_DEVICE_ADDRESS */
  __IOM uint32_t USER_FREE_ROW1[128];           /*!< 0x00000A00 USER_FREE_ROW1 */
  __IOM uint32_t USER_FREE_ROW2[128];           /*!< 0x00000C00 USER_FREE_ROW2 */
  __IOM uint32_t USER_FREE_ROW3[128];           /*!< 0x00000E00 USER_FREE_ROW3 */
   __IM uint32_t RESERVED7[302];
  __IOM uint8_t  DEVICE_UID[16];                /*!< 0x000014B8 Unique Identifier Number for each device */
  __IOM uint8_t  MASTER_KEY[16];                /*!< 0x000014C8 Master key to change other keys */
  __IOM uint32_t STANDARD_SMPU_STRUCT_SLAVE_ADDR[16]; /*!< 0x000014D8 Standard SMPU STRUCT Slave Address value */
  __IOM uint32_t STANDARD_SMPU_STRUCT_SLAVE_ATTR[16]; /*!< 0x00001518 Standard SMPU STRUCT Slave Attribute value */
  __IOM uint32_t STANDARD_SMPU_STRUCT_MASTER_ATTR[16]; /*!< 0x00001558 Standard SMPU STRUCT Master Attribute value */
  __IOM uint32_t STANDARD_MPU_STRUCT[16];       /*!< 0x00001598 Standard MPU STRUCT */
  __IOM uint32_t STANDARD_PPU_STRUCT[16];       /*!< 0x000015D8 Standard PPU STRUCT */
   __IM uint32_t RESERVED8[122];
  __IOM uint16_t PILO_FREQ_STEP;                /*!< 0x00001800 Resolution step for PILO at class in BCD format */
   __IM uint16_t RESERVED9;
  __IOM uint32_t CSDV2_CSD0_ADC_VREF0;          /*!< 0x00001804 CSD 1p2 & 1p6 voltage levels for accuracy */
  __IOM uint32_t CSDV2_CSD0_ADC_VREF1;          /*!< 0x00001808 CSD 2p3 & 0p8 voltage levels for accuracy */
  __IOM uint32_t CSDV2_CSD0_ADC_VREF2;          /*!< 0x0000180C CSD calibration spare voltage level for accuracy */
  __IOM uint32_t PWR_TRIM_WAKE_CTL;             /*!< 0x00001810 Wakeup delay */
   __IM uint16_t RESERVED10;
  __IOM uint16_t RADIO_LDO_TRIMS;               /*!< 0x00001816 Radio LDO Trims */
  __IOM uint32_t CPUSS_TRIM_ROM_CTL_ULP;        /*!< 0x00001818 CPUSS TRIM ROM CTL ULP value */
  __IOM uint32_t CPUSS_TRIM_RAM_CTL_ULP;        /*!< 0x0000181C CPUSS TRIM RAM CTL ULP value */
  __IOM uint32_t CPUSS_TRIM_ROM_CTL_LP;         /*!< 0x00001820 CPUSS TRIM ROM CTL LP value */
  __IOM uint32_t CPUSS_TRIM_RAM_CTL_LP;         /*!< 0x00001824 CPUSS TRIM RAM CTL LP value */
   __IM uint32_t RESERVED11[7];
  __IOM uint32_t CPUSS_TRIM_ROM_CTL_HALF_ULP;   /*!< 0x00001844 CPUSS TRIM ROM CTL HALF ULP value */
  __IOM uint32_t CPUSS_TRIM_RAM_CTL_HALF_ULP;   /*!< 0x00001848 CPUSS TRIM RAM CTL HALF ULP value */
  __IOM uint32_t CPUSS_TRIM_ROM_CTL_HALF_LP;    /*!< 0x0000184C CPUSS TRIM ROM CTL HALF LP value */
  __IOM uint32_t CPUSS_TRIM_RAM_CTL_HALF_LP;    /*!< 0x00001850 CPUSS TRIM RAM CTL HALF LP value */
   __IM uint32_t RESERVED12[491];
  __IOM uint32_t FLASH_BOOT_OBJECT_SIZE;        /*!< 0x00002000 Flash Boot - Object Size */
  __IOM uint32_t FLASH_BOOT_APP_ID;             /*!< 0x00002004 Flash Boot - Application ID/Version */
  __IOM uint32_t FLASH_BOOT_ATTRIBUTE;          /*!< 0x00002008 N/A */
  __IOM uint32_t FLASH_BOOT_N_CORES;            /*!< 0x0000200C Flash Boot - Number of Cores(N) */
  __IOM uint32_t FLASH_BOOT_VT_OFFSET;          /*!< 0x00002010 Flash Boot - Core Vector Table offset */
  __IOM uint32_t FLASH_BOOT_CORE_CPUID;         /*!< 0x00002014 Flash Boot - Core CPU ID/Core Index */
   __IM uint32_t RESERVED13[48];
  __IOM uint8_t  FLASH_BOOT_CODE[8488];         /*!< 0x000020D8 Flash Boot - Code and Data */
   __IM uint32_t RESERVED14[1536];
  __IOM uint8_t  PUBLIC_KEY[3072];              /*!< 0x00005A00 Public key for signature verification (max RSA key size 4096) */
  __IOM uint32_t BOOT_PROT_SETTINGS[384];       /*!< 0x00006600 Boot protection settings (not present in PSOC6ABLE2) */
   __IM uint32_t RESERVED15[768];
  __IOM uint32_t TOC1_OBJECT_SIZE;              /*!< 0x00007800 Object size in bytes for CRC calculation starting from offset
                                                                0x00 */
  __IOM uint32_t TOC1_MAGIC_NUMBER;             /*!< 0x00007804 Magic number(0x01211219) */
  __IOM uint32_t TOC1_FHASH_OBJECTS;            /*!< 0x00007808 Number of objects starting from offset 0xC to be verified for
                                                                FACTORY_HASH */
  __IOM uint32_t TOC1_SFLASH_GENERAL_TRIM_ADDR; /*!< 0x0000780C Address of trims stored in SFLASH */
  __IOM uint32_t TOC1_UNIQUE_ID_ADDR;           /*!< 0x00007810 Address of Unique ID stored in SFLASH */
  __IOM uint32_t TOC1_FB_OBJECT_ADDR;           /*!< 0x00007814 Addresss of FLASH Boot(FB) object that include FLASH patch also */
  __IOM uint32_t TOC1_SYSCALL_TABLE_ADDR;       /*!< 0x00007818 Address of SYSCALL_TABLE entry in SFLASH */
  __IOM uint32_t TOC1_BOOT_PROTECTION_ADDR;     /*!< 0x0000781C Address of boot protection object */
   __IM uint32_t RESERVED16[119];
  __IOM uint32_t TOC1_CRC_ADDR;                 /*!< 0x000079FC Upper 2 bytes contain CRC16-CCITT and lower 2 bytes are 0 */
  __IOM uint32_t RTOC1_OBJECT_SIZE;             /*!< 0x00007A00 Redundant Object size in bytes for CRC calculation starting
                                                                from offset 0x00 */
  __IOM uint32_t RTOC1_MAGIC_NUMBER;            /*!< 0x00007A04 Redundant Magic number(0x01211219) */
  __IOM uint32_t RTOC1_FHASH_OBJECTS;           /*!< 0x00007A08 Redundant Number of objects starting from offset 0xC to be
                                                                verified for FACTORY_HASH */
  __IOM uint32_t RTOC1_SFLASH_GENERAL_TRIM_ADDR; /*!< 0x00007A0C Redundant Address of trims stored in SFLASH */
  __IOM uint32_t RTOC1_UNIQUE_ID_ADDR;          /*!< 0x00007A10 Redundant Address of Unique ID stored in SFLASH */
  __IOM uint32_t RTOC1_FB_OBJECT_ADDR;          /*!< 0x00007A14 Redundant Addresss of FLASH Boot(FB) object that include FLASH
                                                                patch also */
  __IOM uint32_t RTOC1_SYSCALL_TABLE_ADDR;      /*!< 0x00007A18 Redundant Address of SYSCALL_TABLE entry in SFLASH */
   __IM uint32_t RESERVED17[120];
  __IOM uint32_t RTOC1_CRC_ADDR;                /*!< 0x00007BFC Redundant CRC,Upper 2 bytes contain CRC16-CCITT and lower 2
                                                                bytes are 0 */
  __IOM uint32_t TOC2_OBJECT_SIZE;              /*!< 0x00007C00 Object size in bytes for CRC calculation starting from offset
                                                                0x00 */
  __IOM uint32_t TOC2_MAGIC_NUMBER;             /*!< 0x00007C04 Magic number(0x01211220) */
  __IOM uint32_t TOC2_KEY_BLOCK_ADDR;           /*!< 0x00007C08 Address of Key Storage FLASH blocks */
  __IOM uint32_t TOC2_SMIF_CFG_STRUCT_ADDR;     /*!< 0x00007C0C Null terminated table of pointers representing the SMIF
                                                                configuration structure */
  __IOM uint32_t TOC2_FIRST_USER_APP_ADDR;      /*!< 0x00007C10 Address of First User Application Object */
  __IOM uint32_t TOC2_FIRST_USER_APP_FORMAT;    /*!< 0x00007C14 Format of First User Application Object. 0 - Basic, 1 - Cypress
                                                                standard & 2 - Simplified */
  __IOM uint32_t TOC2_SECOND_USER_APP_ADDR;     /*!< 0x00007C18 Address of Second User Application Object */
  __IOM uint32_t TOC2_SECOND_USER_APP_FORMAT;   /*!< 0x00007C1C Format of Second User Application Object. 0 - Basic, 1 -
                                                                Cypress standard & 2 - Simplified */
  __IOM uint32_t TOC2_SHASH_OBJECTS;            /*!< 0x00007C20 Number of additional objects(in addition to objects covered by
                                                                FACORY_CAMC) starting from offset 0x24 to be verified for
                                                                SECURE_HASH(SHASH) */
  __IOM uint32_t TOC2_SIGNATURE_VERIF_KEY;      /*!< 0x00007C24 Address of signature verification key (0 if none).The object is
                                                                signature specific key. It is the public key in case of RSA */
   __IM uint32_t RESERVED18[116];
  __IOM uint32_t TOC2_FLAGS;                    /*!< 0x00007DF8 TOC2_FLAGS */
  __IOM uint32_t TOC2_CRC_ADDR;                 /*!< 0x00007DFC CRC,Upper 2 bytes contain CRC16-CCITT and lower 2 bytes are 0 */
  __IOM uint32_t RTOC2_OBJECT_SIZE;             /*!< 0x00007E00 Redundant Object size in bytes for CRC calculation starting
                                                                from offset 0x00 */
  __IOM uint32_t RTOC2_MAGIC_NUMBER;            /*!< 0x00007E04 Redundant Magic number(0x01211220) */
  __IOM uint32_t RTOC2_KEY_BLOCK_ADDR;          /*!< 0x00007E08 Redundant Address of Key Storage FLASH blocks */
  __IOM uint32_t RTOC2_SMIF_CFG_STRUCT_ADDR;    /*!< 0x00007E0C Redundant Null terminated table of pointers representing the
                                                                SMIF configuration structure */
  __IOM uint32_t RTOC2_FIRST_USER_APP_ADDR;     /*!< 0x00007E10 Redundant Address of First User Application Object */
  __IOM uint32_t RTOC2_FIRST_USER_APP_FORMAT;   /*!< 0x00007E14 Redundant Format of First User Application Object. 0 - Basic, 1
                                                                - Cypress standard & 2 - Simplified */
  __IOM uint32_t RTOC2_SECOND_USER_APP_ADDR;    /*!< 0x00007E18 Redundant Address of Second User Application Object */
  __IOM uint32_t RTOC2_SECOND_USER_APP_FORMAT;  /*!< 0x00007E1C Redundant Format of Second User Application Object. 0 - Basic,
                                                                1 - Cypress standard & 2 - Simplified */
  __IOM uint32_t RTOC2_SHASH_OBJECTS;           /*!< 0x00007E20 Redundant Number of additional objects(in addition to objects
                                                                covered by FACORY_CAMC) starting from offset 0x24 to be verified
                                                                for SECURE_HASH(SHASH) */
  __IOM uint32_t RTOC2_SIGNATURE_VERIF_KEY;     /*!< 0x00007E24 Redundant Address of signature verification key (0 if none).The
                                                                object is signature specific key. It is the public key in case
                                                                of RSA */
   __IM uint32_t RESERVED19[116];
  __IOM uint32_t RTOC2_FLAGS;                   /*!< 0x00007FF8 RTOC2_FLAGS */
  __IOM uint32_t RTOC2_CRC_ADDR;                /*!< 0x00007FFC Redundant CRC,Upper 2 bytes contain CRC16-CCITT and lower 2
                                                                bytes are 0 */
} SFLASH_V1_Type;                               /*!< Size = 32768 (0x8000) */


/* SFLASH.SI_REVISION_ID */
#define SFLASH_SI_REVISION_ID_SI_REVISION_ID_Pos 0UL
#define SFLASH_SI_REVISION_ID_SI_REVISION_ID_Msk 0xFFUL
/* SFLASH.SILICON_ID */
#define SFLASH_SILICON_ID_ID_Pos                0UL
#define SFLASH_SILICON_ID_ID_Msk                0xFFFFUL
/* SFLASH.FAMILY_ID */
#define SFLASH_FAMILY_ID_FAMILY_ID_Pos          0UL
#define SFLASH_FAMILY_ID_FAMILY_ID_Msk          0xFFFFUL
/* SFLASH.DIE_LOT */
#define SFLASH_DIE_LOT_LOT_Pos                  0UL
#define SFLASH_DIE_LOT_LOT_Msk                  0xFFUL
/* SFLASH.DIE_WAFER */
#define SFLASH_DIE_WAFER_WAFER_Pos              0UL
#define SFLASH_DIE_WAFER_WAFER_Msk              0xFFUL
/* SFLASH.DIE_X */
#define SFLASH_DIE_X_X_Pos                      0UL
#define SFLASH_DIE_X_X_Msk                      0xFFUL
/* SFLASH.DIE_Y */
#define SFLASH_DIE_Y_Y_Pos                      0UL
#define SFLASH_DIE_Y_Y_Msk                      0xFFUL
/* SFLASH.DIE_SORT */
#define SFLASH_DIE_SORT_S1_PASS_Pos             0UL
#define SFLASH_DIE_SORT_S1_PASS_Msk             0x1UL
#define SFLASH_DIE_SORT_S2_PASS_Pos             1UL
#define SFLASH_DIE_SORT_S2_PASS_Msk             0x2UL
#define SFLASH_DIE_SORT_S3_PASS_Pos             2UL
#define SFLASH_DIE_SORT_S3_PASS_Msk             0x4UL
#define SFLASH_DIE_SORT_CRI_PASS_Pos            3UL
#define SFLASH_DIE_SORT_CRI_PASS_Msk            0x8UL
#define SFLASH_DIE_SORT_CHI_PASS_Pos            4UL
#define SFLASH_DIE_SORT_CHI_PASS_Msk            0x10UL
#define SFLASH_DIE_SORT_ENG_PASS_Pos            5UL
#define SFLASH_DIE_SORT_ENG_PASS_Msk            0x20UL
/* SFLASH.DIE_MINOR */
#define SFLASH_DIE_MINOR_MINOR_Pos              0UL
#define SFLASH_DIE_MINOR_MINOR_Msk              0xFFUL
/* SFLASH.DIE_DAY */
#define SFLASH_DIE_DAY_MINOR_Pos                0UL
#define SFLASH_DIE_DAY_MINOR_Msk                0xFFUL
/* SFLASH.DIE_MONTH */
#define SFLASH_DIE_MONTH_MINOR_Pos              0UL
#define SFLASH_DIE_MONTH_MINOR_Msk              0xFFUL
/* SFLASH.DIE_YEAR */
#define SFLASH_DIE_YEAR_MINOR_Pos               0UL
#define SFLASH_DIE_YEAR_MINOR_Msk               0xFFUL
/* SFLASH.SAR_TEMP_MULTIPLIER */
#define SFLASH_SAR_TEMP_MULTIPLIER_TEMP_MULTIPLIER_Pos 0UL
#define SFLASH_SAR_TEMP_MULTIPLIER_TEMP_MULTIPLIER_Msk 0xFFFFUL
/* SFLASH.SAR_TEMP_OFFSET */
#define SFLASH_SAR_TEMP_OFFSET_TEMP_OFFSET_Pos  0UL
#define SFLASH_SAR_TEMP_OFFSET_TEMP_OFFSET_Msk  0xFFFFUL
/* SFLASH.CSP_PANEL_ID */
#define SFLASH_CSP_PANEL_ID_DATA32_Pos          0UL
#define SFLASH_CSP_PANEL_ID_DATA32_Msk          0xFFFFFFFFUL
/* SFLASH.LDO_0P9V_TRIM */
#define SFLASH_LDO_0P9V_TRIM_DATA8_Pos          0UL
#define SFLASH_LDO_0P9V_TRIM_DATA8_Msk          0xFFUL
/* SFLASH.LDO_1P1V_TRIM */
#define SFLASH_LDO_1P1V_TRIM_DATA8_Pos          0UL
#define SFLASH_LDO_1P1V_TRIM_DATA8_Msk          0xFFUL
/* SFLASH.BLE_DEVICE_ADDRESS */
#define SFLASH_BLE_DEVICE_ADDRESS_ADDR_Pos      0UL
#define SFLASH_BLE_DEVICE_ADDRESS_ADDR_Msk      0xFFFFFFFFUL
/* SFLASH.USER_FREE_ROW1 */
#define SFLASH_USER_FREE_ROW1_DATA32_Pos        0UL
#define SFLASH_USER_FREE_ROW1_DATA32_Msk        0xFFFFFFFFUL
/* SFLASH.USER_FREE_ROW2 */
#define SFLASH_USER_FREE_ROW2_DATA32_Pos        0UL
#define SFLASH_USER_FREE_ROW2_DATA32_Msk        0xFFFFFFFFUL
/* SFLASH.USER_FREE_ROW3 */
#define SFLASH_USER_FREE_ROW3_DATA32_Pos        0UL
#define SFLASH_USER_FREE_ROW3_DATA32_Msk        0xFFFFFFFFUL
/* SFLASH.DEVICE_UID */
#define SFLASH_DEVICE_UID_DATA8_Pos             0UL
#define SFLASH_DEVICE_UID_DATA8_Msk             0xFFUL
/* SFLASH.MASTER_KEY */
#define SFLASH_MASTER_KEY_DATA8_Pos             0UL
#define SFLASH_MASTER_KEY_DATA8_Msk             0xFFUL
/* SFLASH.STANDARD_SMPU_STRUCT_SLAVE_ADDR */
#define SFLASH_STANDARD_SMPU_STRUCT_SLAVE_ADDR_DATA32_Pos 0UL
#define SFLASH_STANDARD_SMPU_STRUCT_SLAVE_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.STANDARD_SMPU_STRUCT_SLAVE_ATTR */
#define SFLASH_STANDARD_SMPU_STRUCT_SLAVE_ATTR_DATA32_Pos 0UL
#define SFLASH_STANDARD_SMPU_STRUCT_SLAVE_ATTR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.STANDARD_SMPU_STRUCT_MASTER_ATTR */
#define SFLASH_STANDARD_SMPU_STRUCT_MASTER_ATTR_DATA32_Pos 0UL
#define SFLASH_STANDARD_SMPU_STRUCT_MASTER_ATTR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.STANDARD_MPU_STRUCT */
#define SFLASH_STANDARD_MPU_STRUCT_DATA32_Pos   0UL
#define SFLASH_STANDARD_MPU_STRUCT_DATA32_Msk   0xFFFFFFFFUL
/* SFLASH.STANDARD_PPU_STRUCT */
#define SFLASH_STANDARD_PPU_STRUCT_DATA32_Pos   0UL
#define SFLASH_STANDARD_PPU_STRUCT_DATA32_Msk   0xFFFFFFFFUL
/* SFLASH.PILO_FREQ_STEP */
#define SFLASH_PILO_FREQ_STEP_STEP_Pos          0UL
#define SFLASH_PILO_FREQ_STEP_STEP_Msk          0xFFFFUL
/* SFLASH.CSDV2_CSD0_ADC_VREF0 */
#define SFLASH_CSDV2_CSD0_ADC_VREF0_VREF_HI_LEVELS_1P2_Pos 0UL
#define SFLASH_CSDV2_CSD0_ADC_VREF0_VREF_HI_LEVELS_1P2_Msk 0xFFFFUL
#define SFLASH_CSDV2_CSD0_ADC_VREF0_VREF_HI_LEVELS_1P6_Pos 16UL
#define SFLASH_CSDV2_CSD0_ADC_VREF0_VREF_HI_LEVELS_1P6_Msk 0xFFFF0000UL
/* SFLASH.CSDV2_CSD0_ADC_VREF1 */
#define SFLASH_CSDV2_CSD0_ADC_VREF1_VREF_HI_LEVELS_2P1_Pos 0UL
#define SFLASH_CSDV2_CSD0_ADC_VREF1_VREF_HI_LEVELS_2P1_Msk 0xFFFFUL
#define SFLASH_CSDV2_CSD0_ADC_VREF1_VREF_HI_LEVELS_0P8_Pos 16UL
#define SFLASH_CSDV2_CSD0_ADC_VREF1_VREF_HI_LEVELS_0P8_Msk 0xFFFF0000UL
/* SFLASH.CSDV2_CSD0_ADC_VREF2 */
#define SFLASH_CSDV2_CSD0_ADC_VREF2_VREF_HI_LEVELS_2P6_Pos 0UL
#define SFLASH_CSDV2_CSD0_ADC_VREF2_VREF_HI_LEVELS_2P6_Msk 0xFFFFUL
/* SFLASH.PWR_TRIM_WAKE_CTL */
#define SFLASH_PWR_TRIM_WAKE_CTL_WAKE_DELAY_Pos 0UL
#define SFLASH_PWR_TRIM_WAKE_CTL_WAKE_DELAY_Msk 0xFFUL
/* SFLASH.RADIO_LDO_TRIMS */
#define SFLASH_RADIO_LDO_TRIMS_LDO_ACT_Pos      0UL
#define SFLASH_RADIO_LDO_TRIMS_LDO_ACT_Msk      0xFUL
#define SFLASH_RADIO_LDO_TRIMS_LDO_LNA_Pos      4UL
#define SFLASH_RADIO_LDO_TRIMS_LDO_LNA_Msk      0x30UL
#define SFLASH_RADIO_LDO_TRIMS_LDO_IF_Pos       6UL
#define SFLASH_RADIO_LDO_TRIMS_LDO_IF_Msk       0xC0UL
#define SFLASH_RADIO_LDO_TRIMS_LDO_DIG_Pos      8UL
#define SFLASH_RADIO_LDO_TRIMS_LDO_DIG_Msk      0x300UL
/* SFLASH.CPUSS_TRIM_ROM_CTL_ULP */
#define SFLASH_CPUSS_TRIM_ROM_CTL_ULP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_ROM_CTL_ULP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.CPUSS_TRIM_RAM_CTL_ULP */
#define SFLASH_CPUSS_TRIM_RAM_CTL_ULP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_RAM_CTL_ULP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.CPUSS_TRIM_ROM_CTL_LP */
#define SFLASH_CPUSS_TRIM_ROM_CTL_LP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_ROM_CTL_LP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.CPUSS_TRIM_RAM_CTL_LP */
#define SFLASH_CPUSS_TRIM_RAM_CTL_LP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_RAM_CTL_LP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.CPUSS_TRIM_ROM_CTL_HALF_ULP */
#define SFLASH_CPUSS_TRIM_ROM_CTL_HALF_ULP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_ROM_CTL_HALF_ULP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.CPUSS_TRIM_RAM_CTL_HALF_ULP */
#define SFLASH_CPUSS_TRIM_RAM_CTL_HALF_ULP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_RAM_CTL_HALF_ULP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.CPUSS_TRIM_ROM_CTL_HALF_LP */
#define SFLASH_CPUSS_TRIM_ROM_CTL_HALF_LP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_ROM_CTL_HALF_LP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.CPUSS_TRIM_RAM_CTL_HALF_LP */
#define SFLASH_CPUSS_TRIM_RAM_CTL_HALF_LP_DATA32_Pos 0UL
#define SFLASH_CPUSS_TRIM_RAM_CTL_HALF_LP_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.FLASH_BOOT_OBJECT_SIZE */
#define SFLASH_FLASH_BOOT_OBJECT_SIZE_DATA32_Pos 0UL
#define SFLASH_FLASH_BOOT_OBJECT_SIZE_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.FLASH_BOOT_APP_ID */
#define SFLASH_FLASH_BOOT_APP_ID_APP_ID_Pos     0UL
#define SFLASH_FLASH_BOOT_APP_ID_APP_ID_Msk     0xFFFFUL
#define SFLASH_FLASH_BOOT_APP_ID_MINOR_VERSION_Pos 16UL
#define SFLASH_FLASH_BOOT_APP_ID_MINOR_VERSION_Msk 0xFF0000UL
#define SFLASH_FLASH_BOOT_APP_ID_MAJOR_VERSION_Pos 24UL
#define SFLASH_FLASH_BOOT_APP_ID_MAJOR_VERSION_Msk 0xF000000UL
/* SFLASH.FLASH_BOOT_ATTRIBUTE */
#define SFLASH_FLASH_BOOT_ATTRIBUTE_DATA32_Pos  0UL
#define SFLASH_FLASH_BOOT_ATTRIBUTE_DATA32_Msk  0xFFFFFFFFUL
/* SFLASH.FLASH_BOOT_N_CORES */
#define SFLASH_FLASH_BOOT_N_CORES_DATA32_Pos    0UL
#define SFLASH_FLASH_BOOT_N_CORES_DATA32_Msk    0xFFFFFFFFUL
/* SFLASH.FLASH_BOOT_VT_OFFSET */
#define SFLASH_FLASH_BOOT_VT_OFFSET_DATA32_Pos  0UL
#define SFLASH_FLASH_BOOT_VT_OFFSET_DATA32_Msk  0xFFFFFFFFUL
/* SFLASH.FLASH_BOOT_CORE_CPUID */
#define SFLASH_FLASH_BOOT_CORE_CPUID_DATA32_Pos 0UL
#define SFLASH_FLASH_BOOT_CORE_CPUID_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.FLASH_BOOT_CODE */
#define SFLASH_FLASH_BOOT_CODE_DATA32_Pos       0UL
#define SFLASH_FLASH_BOOT_CODE_DATA32_Msk       0xFFFFFFFFUL
/* SFLASH.PUBLIC_KEY */
#define SFLASH_PUBLIC_KEY_DATA_Pos              0UL
#define SFLASH_PUBLIC_KEY_DATA_Msk              0xFFUL
/* SFLASH.BOOT_PROT_SETTINGS */
#define SFLASH_BOOT_PROT_SETTINGS_DATA32_Pos    0UL
#define SFLASH_BOOT_PROT_SETTINGS_DATA32_Msk    0xFFFFFFFFUL
/* SFLASH.TOC1_OBJECT_SIZE */
#define SFLASH_TOC1_OBJECT_SIZE_DATA32_Pos      0UL
#define SFLASH_TOC1_OBJECT_SIZE_DATA32_Msk      0xFFFFFFFFUL
/* SFLASH.TOC1_MAGIC_NUMBER */
#define SFLASH_TOC1_MAGIC_NUMBER_DATA32_Pos     0UL
#define SFLASH_TOC1_MAGIC_NUMBER_DATA32_Msk     0xFFFFFFFFUL
/* SFLASH.TOC1_FHASH_OBJECTS */
#define SFLASH_TOC1_FHASH_OBJECTS_DATA32_Pos    0UL
#define SFLASH_TOC1_FHASH_OBJECTS_DATA32_Msk    0xFFFFFFFFUL
/* SFLASH.TOC1_SFLASH_GENERAL_TRIM_ADDR */
#define SFLASH_TOC1_SFLASH_GENERAL_TRIM_ADDR_DATA32_Pos 0UL
#define SFLASH_TOC1_SFLASH_GENERAL_TRIM_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC1_UNIQUE_ID_ADDR */
#define SFLASH_TOC1_UNIQUE_ID_ADDR_DATA32_Pos   0UL
#define SFLASH_TOC1_UNIQUE_ID_ADDR_DATA32_Msk   0xFFFFFFFFUL
/* SFLASH.TOC1_FB_OBJECT_ADDR */
#define SFLASH_TOC1_FB_OBJECT_ADDR_DATA32_Pos   0UL
#define SFLASH_TOC1_FB_OBJECT_ADDR_DATA32_Msk   0xFFFFFFFFUL
/* SFLASH.TOC1_SYSCALL_TABLE_ADDR */
#define SFLASH_TOC1_SYSCALL_TABLE_ADDR_DATA32_Pos 0UL
#define SFLASH_TOC1_SYSCALL_TABLE_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC1_BOOT_PROTECTION_ADDR */
#define SFLASH_TOC1_BOOT_PROTECTION_ADDR_DATA32_Pos 0UL
#define SFLASH_TOC1_BOOT_PROTECTION_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC1_CRC_ADDR */
#define SFLASH_TOC1_CRC_ADDR_DATA32_Pos         0UL
#define SFLASH_TOC1_CRC_ADDR_DATA32_Msk         0xFFFFFFFFUL
/* SFLASH.RTOC1_OBJECT_SIZE */
#define SFLASH_RTOC1_OBJECT_SIZE_DATA32_Pos     0UL
#define SFLASH_RTOC1_OBJECT_SIZE_DATA32_Msk     0xFFFFFFFFUL
/* SFLASH.RTOC1_MAGIC_NUMBER */
#define SFLASH_RTOC1_MAGIC_NUMBER_DATA32_Pos    0UL
#define SFLASH_RTOC1_MAGIC_NUMBER_DATA32_Msk    0xFFFFFFFFUL
/* SFLASH.RTOC1_FHASH_OBJECTS */
#define SFLASH_RTOC1_FHASH_OBJECTS_DATA32_Pos   0UL
#define SFLASH_RTOC1_FHASH_OBJECTS_DATA32_Msk   0xFFFFFFFFUL
/* SFLASH.RTOC1_SFLASH_GENERAL_TRIM_ADDR */
#define SFLASH_RTOC1_SFLASH_GENERAL_TRIM_ADDR_DATA32_Pos 0UL
#define SFLASH_RTOC1_SFLASH_GENERAL_TRIM_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC1_UNIQUE_ID_ADDR */
#define SFLASH_RTOC1_UNIQUE_ID_ADDR_DATA32_Pos  0UL
#define SFLASH_RTOC1_UNIQUE_ID_ADDR_DATA32_Msk  0xFFFFFFFFUL
/* SFLASH.RTOC1_FB_OBJECT_ADDR */
#define SFLASH_RTOC1_FB_OBJECT_ADDR_DATA32_Pos  0UL
#define SFLASH_RTOC1_FB_OBJECT_ADDR_DATA32_Msk  0xFFFFFFFFUL
/* SFLASH.RTOC1_SYSCALL_TABLE_ADDR */
#define SFLASH_RTOC1_SYSCALL_TABLE_ADDR_DATA32_Pos 0UL
#define SFLASH_RTOC1_SYSCALL_TABLE_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC1_CRC_ADDR */
#define SFLASH_RTOC1_CRC_ADDR_DATA32_Pos        0UL
#define SFLASH_RTOC1_CRC_ADDR_DATA32_Msk        0xFFFFFFFFUL
/* SFLASH.TOC2_OBJECT_SIZE */
#define SFLASH_TOC2_OBJECT_SIZE_DATA32_Pos      0UL
#define SFLASH_TOC2_OBJECT_SIZE_DATA32_Msk      0xFFFFFFFFUL
/* SFLASH.TOC2_MAGIC_NUMBER */
#define SFLASH_TOC2_MAGIC_NUMBER_DATA32_Pos     0UL
#define SFLASH_TOC2_MAGIC_NUMBER_DATA32_Msk     0xFFFFFFFFUL
/* SFLASH.TOC2_KEY_BLOCK_ADDR */
#define SFLASH_TOC2_KEY_BLOCK_ADDR_DATA32_Pos   0UL
#define SFLASH_TOC2_KEY_BLOCK_ADDR_DATA32_Msk   0xFFFFFFFFUL
/* SFLASH.TOC2_SMIF_CFG_STRUCT_ADDR */
#define SFLASH_TOC2_SMIF_CFG_STRUCT_ADDR_DATA32_Pos 0UL
#define SFLASH_TOC2_SMIF_CFG_STRUCT_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC2_FIRST_USER_APP_ADDR */
#define SFLASH_TOC2_FIRST_USER_APP_ADDR_DATA32_Pos 0UL
#define SFLASH_TOC2_FIRST_USER_APP_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC2_FIRST_USER_APP_FORMAT */
#define SFLASH_TOC2_FIRST_USER_APP_FORMAT_DATA32_Pos 0UL
#define SFLASH_TOC2_FIRST_USER_APP_FORMAT_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC2_SECOND_USER_APP_ADDR */
#define SFLASH_TOC2_SECOND_USER_APP_ADDR_DATA32_Pos 0UL
#define SFLASH_TOC2_SECOND_USER_APP_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC2_SECOND_USER_APP_FORMAT */
#define SFLASH_TOC2_SECOND_USER_APP_FORMAT_DATA32_Pos 0UL
#define SFLASH_TOC2_SECOND_USER_APP_FORMAT_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC2_SHASH_OBJECTS */
#define SFLASH_TOC2_SHASH_OBJECTS_DATA32_Pos    0UL
#define SFLASH_TOC2_SHASH_OBJECTS_DATA32_Msk    0xFFFFFFFFUL
/* SFLASH.TOC2_SIGNATURE_VERIF_KEY */
#define SFLASH_TOC2_SIGNATURE_VERIF_KEY_DATA32_Pos 0UL
#define SFLASH_TOC2_SIGNATURE_VERIF_KEY_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.TOC2_FLAGS */
#define SFLASH_TOC2_FLAGS_DATA32_Pos            0UL
#define SFLASH_TOC2_FLAGS_DATA32_Msk            0xFFFFFFFFUL
/* SFLASH.TOC2_CRC_ADDR */
#define SFLASH_TOC2_CRC_ADDR_DATA32_Pos         0UL
#define SFLASH_TOC2_CRC_ADDR_DATA32_Msk         0xFFFFFFFFUL
/* SFLASH.RTOC2_OBJECT_SIZE */
#define SFLASH_RTOC2_OBJECT_SIZE_DATA32_Pos     0UL
#define SFLASH_RTOC2_OBJECT_SIZE_DATA32_Msk     0xFFFFFFFFUL
/* SFLASH.RTOC2_MAGIC_NUMBER */
#define SFLASH_RTOC2_MAGIC_NUMBER_DATA32_Pos    0UL
#define SFLASH_RTOC2_MAGIC_NUMBER_DATA32_Msk    0xFFFFFFFFUL
/* SFLASH.RTOC2_KEY_BLOCK_ADDR */
#define SFLASH_RTOC2_KEY_BLOCK_ADDR_DATA32_Pos  0UL
#define SFLASH_RTOC2_KEY_BLOCK_ADDR_DATA32_Msk  0xFFFFFFFFUL
/* SFLASH.RTOC2_SMIF_CFG_STRUCT_ADDR */
#define SFLASH_RTOC2_SMIF_CFG_STRUCT_ADDR_DATA32_Pos 0UL
#define SFLASH_RTOC2_SMIF_CFG_STRUCT_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC2_FIRST_USER_APP_ADDR */
#define SFLASH_RTOC2_FIRST_USER_APP_ADDR_DATA32_Pos 0UL
#define SFLASH_RTOC2_FIRST_USER_APP_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC2_FIRST_USER_APP_FORMAT */
#define SFLASH_RTOC2_FIRST_USER_APP_FORMAT_DATA32_Pos 0UL
#define SFLASH_RTOC2_FIRST_USER_APP_FORMAT_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC2_SECOND_USER_APP_ADDR */
#define SFLASH_RTOC2_SECOND_USER_APP_ADDR_DATA32_Pos 0UL
#define SFLASH_RTOC2_SECOND_USER_APP_ADDR_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC2_SECOND_USER_APP_FORMAT */
#define SFLASH_RTOC2_SECOND_USER_APP_FORMAT_DATA32_Pos 0UL
#define SFLASH_RTOC2_SECOND_USER_APP_FORMAT_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC2_SHASH_OBJECTS */
#define SFLASH_RTOC2_SHASH_OBJECTS_DATA32_Pos   0UL
#define SFLASH_RTOC2_SHASH_OBJECTS_DATA32_Msk   0xFFFFFFFFUL
/* SFLASH.RTOC2_SIGNATURE_VERIF_KEY */
#define SFLASH_RTOC2_SIGNATURE_VERIF_KEY_DATA32_Pos 0UL
#define SFLASH_RTOC2_SIGNATURE_VERIF_KEY_DATA32_Msk 0xFFFFFFFFUL
/* SFLASH.RTOC2_FLAGS */
#define SFLASH_RTOC2_FLAGS_DATA32_Pos           0UL
#define SFLASH_RTOC2_FLAGS_DATA32_Msk           0xFFFFFFFFUL
/* SFLASH.RTOC2_CRC_ADDR */
#define SFLASH_RTOC2_CRC_ADDR_DATA32_Pos        0UL
#define SFLASH_RTOC2_CRC_ADDR_DATA32_Msk        0xFFFFFFFFUL


#endif /* _CYIP_SFLASH_H_ */


/* [] END OF FILE */
