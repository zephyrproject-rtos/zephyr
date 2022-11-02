/*
 * Copyright 2022 Macronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <sys/byteorder.h>

/*!
 * @addtogroup onfi_nand_flash
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief NAND Flash vendor type, _nand_vendor_type_index */
enum
{
	kNandVendorType_Micron   = 0U,
	kNandVendorType_Spansion = 1U,
	kNandVendorType_Samsung  = 2U,
	kNandVendorType_Winbond  = 3U,
	kNandVendorType_Hynix    = 4U,
	kNandVendorType_Toshiba  = 5U,
	kNandVendorType_Macronix = 6U,
	kNandVendorType_Unknown  = 7U,
};

/*! @brief Parallel NAND Flash AC timing mode, _nand_ac_timing_table_index */
enum
{
	kNandAcTimingTableIndex_ONFI_1p0_Mode0_10MHz = 0U,
	kNandAcTimingTableIndex_ONFI_1p0_Mode1_20MHz = 1U,
	kNandAcTimingTableIndex_ONFI_1p0_Mode2_28MHz = 2U,
	kNandAcTimingTableIndex_ONFI_1p0_Mode3_33MHz = 3U,
	kNandAcTimingTableIndex_ONFI_1p0_Mode4_40MHz = 4U,
	kNandAcTimingTableIndex_ONFI_1p0_Mode5_50MHz = 5U,
	kNandAcTimingTableIndex_ONFI_1p0_FastestMode = 6U,
};

/*! @brief Parallel NAND Flash commands, _nand_onfi_command_set */
enum
{
	/* Must-have command */
	kNandDeviceCmd_ONFI_Reset                         = 0xFFU,
	kNandDeviceCmd_ONFI_ReadMode                      = 0x00U,
	kNandDeviceCmd_ONFI_ReadParameterPage             = 0xECU,
	kNandDeviceCmd_ONFI_ReadStatus                    = 0x70U,
	kNandDeviceCmd_ONFI_ReadPageSetup                 = 0x00U,
	kNandDeviceCmd_ONFI_ReadPageConfirm               = 0x30U,
	kNandDeviceCmd_ONFI_ChangeReadColumnSetup         = 0x05U,
	kNandDeviceCmd_ONFI_ChangeReadColumnEnhancedSetup = 0x06U,
	kNandDeviceCmd_ONFI_ChangeReadColumnConfirm       = 0xE0U,
	kNandDeviceCmd_ONFI_EraseBlockSetup               = 0x60U,
	kNandDeviceCmd_ONFI_EraseBlockConfirm             = 0xD0U,
	kNandDeviceCmd_ONFI_ProgramPageSetup              = 0x80U,
	kNandDeviceCmd_ONFI_ProgramPageConfirm            = 0x10U,
	/* Optional command */
	kNandDeviceCmd_ONFI_ReadStatusEnhanced = 0x78U,
	kNandDeviceCmd_ONFI_SetFeatures        = 0xEFU,
	kNandDeviceCmd_ONFI_GetFeatures        = 0xEEU,
	kNandDeviceCmd_ONFI_GetManufacturerID  = 0x90U,
};

/*! @brief Parallel NAND Flash feature set*/
enum
{
	kNandDeviceFeature_ArrayOperationMode_Address    = 0x90U,
	kNandDeviceFeature_ArrayOperationMode_DisableECC = 0x00U,
	kNandDeviceFeature_ArrayOperationMode_EnableECC  = 0x08U,
};

/*! @brief Parallel NAND Flash ONFI Version */
typedef enum _nand_onfi_version
{
	kNandOnfiVersion_None = 0U,
	kNandOnfiVersion_1p0  = 1U,
	kNandOnfiVersion_2p0  = 2U,
	kNandOnfiVersion_3p0  = 3U,
	kNandOnfiVersion_4p0  = 4U,
} nand_onfi_version_t;

/*! @brief Parallel NAND Flash Status Command Type */
typedef enum _nand_status_command_type
{
	kNandStatusCommandType_Common   = 0U,
	kNandStatusCommandType_Enhanced = 1U,
} nand_status_command_type_t;

/*! @brief Parallel NAND Flash change read column Command Type */
typedef enum _nand_change_readcolumn_command_type
{
	kNandChangeReadColumnCommandType_Common   = 0U,
	kNandChangeReadColumnCommandType_Enhanced = 1U,
} nand_change_readcolumn_command_type_t;

/*! @brief NAND Flash ecc check type */
typedef enum _nand_ecc_check_type
{
	kNandEccCheckType_SoftwareECC = 0U,
	kNandEccCheckType_DeviceECC   = 1U,
} nand_ecc_check_type_t;

/*! @brief Parallel NAND Flash Ready check option */
typedef enum _nand_ready_check_option
{
	kNandReadyCheckOption_SR = 0U, /*!< Via Status Register */
	kNandReadyCheckOption_RB = 1U, /*!< Via R/B# signal */
} nand_ready_check_option_t;

/*! @brief IP command for NAND: address mode. */
typedef enum _nand_addrmode
{
	NANDAM_ColumnRow = 0x0U, /*!< Address mode: column and row address(5Byte-CA0/CA1/RA0/RA1/RA2). */
	NANDAM_ColumnCA0,        /*!< Address mode: column address only(1 Byte-CA0).  */
	NANDAM_ColumnCA0CA1,     /*!< Address mode: column address only(2 Byte-CA0/CA1). */
	NANDAM_RawRA0,           /*!< Address mode: row address only(1 Byte-RA0). */
	NANDAM_RawRA0RA1,        /*!< Address mode: row address only(2 Byte-RA0/RA1). */
	NANDAM_RawRA0RA1RA2      /*!< Address mode: row address only(3 Byte-RA0).  */
} nand_addrmode_t;

/*! @brief IP command for NAND： command mode. */
typedef enum _nand_cmdmode
{
	NANDCM_Command = 0x2U,      /*!< command. */
	NANDCM_CommandHold,         /*!< Command hold. */
	NANDCM_CommandAddress,      /*!< Command address. */
	NANDCM_CommandAddressHold,  /*!< Command address hold.  */
	NANDCM_CommandAddressRead,  /*!< Command address read.  */
	NANDCM_CommandAddressWrite, /*!< Command address write.  */
	NANDCM_CommandRead,         /*!< Command read.  */
	NANDCM_CommandWrite,        /*!< Command write.  */
	NANDCM_Read,                /*!< Read.  */
	NANDCM_Write                /*!< Write.  */
} nand_cmdmode_t;

/*!@brief Parallel NAND ONFI parameter config */
typedef struct __nand_onfi_parameter_config
{
	/* Revision information and features block */
	uint32_t signature;      /*!< [0x000-0x003] */
	uint16_t revisionNumber; /*!< [0x004-0x005] */
	struct
	{
		uint16_t x16bitDataBusWidth : 1;
		uint16_t multipleLUNoperations : 1;
		uint16_t reserved : 14;
	} supportedFeatures; /*!< [0x006-0x007] */
	struct
	{
		uint16_t reserved0 : 2;
		uint16_t setGetfeatures : 1;
		uint16_t readStatusEnhanced : 1;
		uint16_t reserved1 : 2;
		uint16_t changeReadColumnEnhanced : 1;
		uint16_t reserved2 : 9;
	} optionalCommands;    /*!< [0x008-0x009] */
	uint8_t reserved0[22]; /*!< [0x00a-0x01f] */
	/* Manufacturer information block */
	char deviceManufacturer[12]; /*!< [0x020-0x02b] */
	uint8_t deviceModel[20];     /*!< [0x02c-0x03f] */
	uint8_t JEDECid;             /*!< [0x040-0x040] */
	uint8_t dataCode[2];         /*!< [0x041-0x042] */
	uint8_t reserved1[13];       /*!< [0x043-0x04f] */
	/* Memory organization block */
	uint32_t dataBytesPerPage;  /*!< [0x050-0x053] */
	uint16_t spareBytesPerPage; /*!< [0x054-0x055] */
	uint8_t reserved2[6];       /*!< [0x056-0x05b] */
	uint32_t pagesPerBlock;     /*!< [0x05c-0x05f] */
	uint32_t blocksPerLUN;      /*!< [0x060-0x063] */
	uint8_t LUNsPerDevice;      /*!< [0x064-0x064] */
	union
	{
		uint8_t addressCycles;
		struct
		{
			uint8_t rowAddressCycles : 4;
			uint8_t columnAddressCycles : 4;
		} addressCyclesStr;
	};                     /*!< [0x065-0x065] */
	uint8_t reserved3[26]; /*!< [0x066-0x07f] */
	/* Electrical parameters block */
	uint8_t reserved4; /*!< [0x080-0x080] */
	struct
	{
		uint8_t mode0 : 1;
		uint8_t mode1 : 1;
		uint8_t mode2 : 1;
		uint8_t mode3 : 1;
		uint8_t mode4 : 1;
		uint8_t mode5 : 1;
		uint8_t reserved : 2;
	} timingMode;                            /*!< [0x081-0x081] */
	uint8_t reserved5[3];                    /*!< [0x082-0x084] */
	uint8_t maxPageProgramTimeInUs[2];       /*!< [0x085-0x086] */
	uint8_t maxBlockEraseTimeInUs[2];        /*!< [0x087-0x088] */
	uint8_t maxPageReadTimeInUs[2];          /*!< [0x089-0x08a] */
	uint8_t minChangeColunmSetupTimeInNs[2]; /*!< [0x08b-0x08c] */
	uint8_t reserved6[23];                   /*!< [0x08d-0x0a3] */
	/* Vendor block */
	uint16_t vendorSpecificRevisionNumber; /*!< [0x0a4-0x0a5] */
	uint8_t reserved7[88];                 /*!< [0x0a6-0x0fd] */
	uint16_t integrityCRC;                 /*!< [0x0fe-0x0ff] */
} nand_onfi_parameter_config_t;

/*! @brief Parallel NAND ONFI feature config */
typedef struct __nand_onfi_feature_config
{
	uint8_t command;
	uint8_t address;
	uint8_t parameter[4];
	uint8_t reserved[2];
} nand_onfi_feature_config_t;

/*! @brief us delay function pointer */
typedef void (*delay_us)(uint32_t us);

/*! @brief NAND Flash Config block structure */
typedef struct _onfi_mem_nand_config
{
	onfi_nand_config_t *onfiNandConfig; /*!< memory controller configuration, shoule bd configured with controller configure structure. */
	delay_us delayUS;                   /*!< delay function pointer, application should prepare a delay function for it */
	nand_onfi_version_t onfiVersion;    /*!< only onfi nand flash will be supported currently. */
	uint8_t readyCheckOption;           /*!< Set with enum type defined in "nand_ready_check_option_t" */
	nand_ecc_check_type_t eccCheckType; /*!< Soft/device ECC check. */
} onfi_mem_nand_config_t;

/*!@brief NAND Flash handle info*/
typedef struct _onfi_mem_nand_handle
{
	delay_us delayUS;                        /*!< delay function pointer, application should prepare a delay function for it */
	uint32_t ctlAccessMemAddr1;              /*!< Nand memory address for memory controller access. */
	uint32_t ctlAccessMemAddr2;              /*!< Nand memory address for memory controller access. */
	uint8_t readyCheckOption;                /*!< Set with enum type defined in "nand_ready_check_option_t" */
	nand_ecc_check_type_t eccCheckType;      /*!< Soft/device ECC check. */
	uint8_t statusCommandType;               /*!< the command enhanced mode or normal command mode */
	uint8_t changeReadColumnType;            /*!< the change read column type. */
	uint8_t columnWidth;                     /*!< the Colum width setting in the controller. */
	bool isFeatureCommandSupport;            /*!< feature command support .*/
	uint32_t rowAddressToGetSR;              /*!< Row address for read status enhanced command */
	uint32_t pageReadTimeInUs_tR;            /*!< Page read time delay */
	uint32_t PageProgramTimeInUs_tPROG;      /*!< Page program time delay */
	uint32_t blockEraseTimeInUs_tBERS;       /*!< block erase time delay */
	uint32_t changeColumnSetupTimeInNs_tCCS; /*!< Change column setup time delay */
} onfi_mem_nand_handle_t;
/*! @} */

/*!@brief NAND Flash handle info*/
typedef struct _nand_handle
{
	/*------------Common parameters used for normal nand flash controller operation ----------*/
	uint8_t vendorType;            /*!< vendor type */
	uint8_t ecc_bits;              /*!< ecc bits correction*/
	uint32_t bytesInPageDataArea;  /*!< Bytes in page data area .*/
	uint32_t bytesInPageSpareArea; /*!< Bytes in page spare area .*/
	uint32_t pagesInBlock;         /*!< Pages in each block. */
	uint32_t blocksInPlane;        /*!< blocks in each plane. */
	uint32_t planesInDevice;       /*!< planes in each device .*/
	void *deviceSpecific;          /*!< Device specific control parameter */
} nand_handle_t;
