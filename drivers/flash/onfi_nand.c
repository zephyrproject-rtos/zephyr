/*
 * Copyright 2022 Macronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT   nxp_onfi_nand

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/onfi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "onfi_nand.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bch.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NAND_READY_CHECK_INTERVAL_NORMAL                         (0U)
#define NAND_MAX_FEATURE_ACCESS_TIME_tFEAT_US                    (1000U)
#define NAND_MAX_READ_PARAMETER_PAGE_TIME_tR_US                  (1000U)
#define NAND_FIRST_CMD_CHANGE_COLUMN_SETUP_TIME_NS               (500U)
#define NAND_MAX_RST_TIME0_tRST_US                               (5U)
#define NAND_MAX_RST_TIME1_tRST_US                               (10U)
#define NAND_MAX_RST_TIME2_tRST_US                               (500U)
#define NAND_MAX_RST_TIME3_tRST_US                               (1000U)
#define NAND_ONFI_SIGNATURE_MIN_MATCHED_BYTES                    (0x2U)
#define NAND_FLASH_SR_ONFI_PASSBITMASK                           (0x01U)
#define NAND_FLASH_SR_ONFI_READYBITMASK                          (0x40U)
#define NAND_FLASH_COLUMNBITSNUM_MAX                             (16U)
#define NAND_FLASH_COLUMNBITSNUM                                 (12U)
#define NAND_IPG_START_ADDRESS                                   (0x00000000U)
#define NAND_ADDRESS_CYCLES                                      (0x23)

#define ONFINAND_PAGE_OFFSET                                     0x1000
#define ONFINAND_BLOCK_OFFSET                                    0x40000
#define ONFINAND_PAGE_SIZE                                       0x800
#define ONFINAND_BLOCK_SIZE                                      0x20000

LOG_MODULE_REGISTER(nxp_onfi_nand, CONFIG_FLASH_LOG_LEVEL);

/*! @brief Constructs the four character code for tag */
#if !defined(FOUR_CHAR_CODE)
#define FOUR_CHAR_CODE(a, b, c, d) \
	(((uint32_t)(d) << 24) | ((uint32_t)(c) << 16) | ((uint32_t)(b) << 8) | ((uint32_t)(a)))
#endif

/*! @brief State information for the CRC16 algorithm. */
typedef struct Crc16Data {
	uint16_t currentCrc; //!< Current CRC value.
} crc16_data_t;

/*! @brief Parallel NAND timing config */
typedef struct _nand_ac_timing_parameter {
	uint8_t min_tCS_ns;
	uint8_t min_tCH_ns;
	uint8_t min_tCEITV_ns;
	uint8_t min_tWP_ns;
	uint8_t min_tWH_ns;
	uint8_t min_tRP_ns;
	uint8_t min_tREH_ns;
	uint8_t min_tTA_ns;
	uint8_t min_tWHR_ns;
	uint8_t min_tRHW_ns;
	uint8_t min_tADL_ns;
	uint8_t min_tRR_ns;
	uint8_t max_tWB_ns;
} nand_ac_timing_parameter_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void onfi_nand_crc16_init(crc16_data_t *crc16Info);
static void onfi_nand_crc16_update(crc16_data_t *crc16Info, const uint8_t *src, uint32_t lengthInBytes);
static void onfi_nand_crc16_finalize(crc16_data_t *crc16Info, uint16_t *hash);
static int onfi_nand_issue_reset(const struct device *dev, nand_handle_t *handle);
static int onfi_nand_issue_read_mode(const struct device *dev, nand_handle_t *handle);
static int onfi_nand_issue_access_feature(const struct device *dev, nand_handle_t *handle, nand_onfi_feature_config_t *featureConfig);
static int onfi_nand_issue_read_parameter(const struct device *dev, nand_handle_t *handle, nand_onfi_parameter_config_t *parameterConfig);
static int onfi_nand_issue_read_status(const struct device *dev, nand_handle_t *handle, uint8_t *stat);
static int onfi_nand_issue_read_page(const struct device *dev, nand_handle_t *handle, uint32_t ipgCmdAddr);
/* Read onfi parameter data from device and make use of them*/
static int onfi_nand_get_timing_configure(const struct device *dev, nand_handle_t *handle, onfi_mem_nand_config_t *onfiMemNandConfig);
/* Check the device ready status*/
static int onfi_nand_wait_status_ready(const struct device *dev, nand_handle_t *handle, uint32_t readyCheckIntervalInUs, bool readOpen);
/* Enable the device ECC via feature setting*/
static int onfi_nand_set_device_ecc(const struct device *dev, nand_handle_t *handle);
/* Validates data by using device ECC*/
static int onfi_nand_is_device_ecc_check_passed(const struct device *dev, nand_handle_t *handle, bool *isEccPassed);

#if DT_INST_ON_BUS (0, semc)
void delayUs(uint32_t delay_us)
{
	uint32_t s_tickPerMicrosecond = CLOCK_GetFreq(kCLOCK_CpuClk) / 1000000U;

	// Make sure this value is greater than 0
	if (!s_tickPerMicrosecond) {
		s_tickPerMicrosecond = 1;
	}
	delay_us = delay_us * s_tickPerMicrosecond;
	while (delay_us) {
		__NOP();
		delay_us--;
	}
}
#endif

onfi_nand_config_t onfiNandConfig = {
	.addressCycle     = NAND_ADDRESS_CYCLES,
	.edoModeEnabled   = false,
	.timingConfig     = NULL,
};

onfi_mem_nand_config_t onfiMemConfig = {
	.onfiNandConfig   = &onfiNandConfig,
	.delayUS          = delayUs,
	.onfiVersion      = kNandOnfiVersion_1p0,
	.readyCheckOption = kNandReadyCheckOption_SR,
	.eccCheckType     = kNandEccCheckType_DeviceECC,
};

struct onfi_nand_data {
	const struct device *controller;
	onfi_mem_nand_config_t *nandconfig;
	nand_handle_t *nandhandle;
	uint32_t block_size;
	uint32_t block_offset;
	uint32_t page_size;
	uint32_t page_offset;
	uint8_t ecc_bytes;
	uint8_t ecc_steps;
	uint8_t ecc_layout_pos;
	uint32_t  ecc_size;
	uint8_t *ecc_calc;
	uint8_t *ecc_code;
	uint8_t *page_buf;

	struct nand_bch_control {
		struct bch_code	  *bch;
		unsigned int      *errloc;
		unsigned char     *eccmask;
	} nbc;
};

nand_handle_t nandHandle;

static const struct flash_parameters flash_onfinand_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

int bch_calculate_ecc(const struct device *dev, unsigned char *buf, unsigned char *code)
{
	struct onfi_nand_data *data = dev->data;

	memset(code, 0, data->ecc_bytes);

	bch_encode(data->nbc.bch, buf, (unsigned int *)code);

	return 0;
}

void bch_ecc_init(const struct device *dev, uint8_t ecc_bits)
{
	struct onfi_nand_data *data = dev->data;
	unsigned int m, t, i;
	unsigned char *erased_page;
	unsigned int eccsize = 410;
	unsigned int eccbytes = 0;

	m = fls(1 + 8 * eccsize);
	t = ecc_bits;

	data->ecc_bytes = eccbytes = ((m * t + 31) / 32) * 4;
	data->ecc_size = eccsize;
	data->ecc_steps = data->page_size / eccsize;
	data->ecc_layout_pos = 2; // skip the bad block mark for Macronix spi nand
	data->nbc.bch = bch_init(m, t);
	if (!data->nbc.bch) {
		return;
	}
	/* verify that eccbytes has the expected value */
	if (data->nbc.bch->ecc_words * 4 != eccbytes) {
		LOG_ERR("invalid eccbytes %u, should be %u\n", eccbytes, data->nbc.bch->ecc_words);
		return;
	}

	data->page_buf = (uint8_t *)k_malloc(data->page_size + data->nandhandle->bytesInPageSpareArea);
	if (data->page_buf == NULL) {
		LOG_ERR("Not enough heap \n");
	}
	data->ecc_calc = (uint8_t *)k_malloc(data->ecc_steps * data->ecc_bytes);
	if (data->ecc_calc == NULL) {
		LOG_ERR("Not enough heap \n");
	}
	data->ecc_code = (uint8_t *)k_malloc(data->ecc_steps * data->ecc_bytes);
	if (data->ecc_code == NULL) {
		LOG_ERR("Not enough heap \n");
	}
	data->nbc.eccmask = (unsigned char *)k_malloc(eccbytes);

	data->nbc.errloc = (unsigned int *)k_malloc(t * sizeof(*data->nbc.errloc));
	if (!data->nbc.eccmask || !data->nbc.errloc) {
		LOG_ERR("Not enough heap \n");
		return;
	}
	/*
	* compute and store the inverted ecc of an erased ecc block
	*/
	erased_page = (unsigned char *)k_malloc(eccsize);
	if (!erased_page) {
		return;
	}
	memset(data->page_buf, 0xff, data->page_size + data->nandhandle->bytesInPageSpareArea);
	memset(erased_page, 0xff, eccsize);
	memset(data->nbc.eccmask, 0, eccbytes);
	bch_encode(data->nbc.bch, erased_page, (unsigned int *)data->nbc.eccmask);
	k_free(erased_page);

	for (i = 0; i < eccbytes; i++) {
		data->nbc.eccmask[i] ^= 0xff;
	}
}

void bch_ecc_free(const struct device *dev)
{
	struct onfi_nand_data *data = dev->data;
	bch_free(data->nbc.bch);
	k_free(data->nbc.errloc);
	k_free(data->nbc.eccmask);
	k_free(data->page_buf);
	k_free(data->ecc_calc);
	k_free(data->ecc_code);
}

/*******************************************************************************
 Variables
 ******************************************************************************/
/*! @brief Define the onfi timing mode. */
static const nand_ac_timing_parameter_t s_nandAcTimingParameterTable[] = {
	/* ONFI 1.0, mode 0, 10MHz, 100ns */
	{
		.min_tCS_ns    = 70,
		.min_tCH_ns    = 20,
		.min_tCEITV_ns = 0,
		.min_tWP_ns    = 50,
		.min_tWH_ns    = 30,
		.min_tRP_ns    = 50,
		.min_tREH_ns   = 30,
		.min_tTA_ns    = 0,
		.min_tWHR_ns   = 120,
		.min_tRHW_ns   = 200,
		.min_tADL_ns   = 200,
		.min_tRR_ns    = 40,
		.max_tWB_ns    = 200
	},
	/* ONFI 1.0 mode 1, 20MHz, 50ns */
	{35, 10, 0, 25, 15, 25, 15, 0, 80, 100, 100, 20, 100},
	/* ONFI 1.0 mode 2, 28MHz, 35ns */
	{25, 10, 0, 17, 15, 17, 15, 0, 80, 100, 100, 20, 100},
	/* ONFI 1.0 mode 3, 33MHz, 30ns */
	{25, 5, 0, 15, 10, 15, 10, 0, 60, 100, 100, 20, 100},

	/* Note: From ONFI spec, The host shall use EDO data output cycle timings,
	   when running with a tRC value less than 30 ns. (tRC = tRP + tREH) */
	/* ONFI 1.0 mode 4, 40MHz, 25ns */
	{20, 5, 0, 12, 10, 12, 10, 0, 60, 100, 70, 20, 100},
	/* ONFI 1.0 mode 5, 50MHz, 20ns */
	{15, 5, 0, 10, 7, 10, 7, 0, 60, 100, 70, 20, 100},
	/* Auto-Detection */
	{0}
};

static const char s_nandDeviceManufacturerList[][12] = {{'M', 'I', 'C', 'R', 'O', 'N', ' ', ' ', ' ', ' ', ' ', ' '},
	{'S', 'P', 'A', 'N', 'S', 'I', 'O', 'N', ' ', ' ', ' ', ' '},
	{'S', 'A', 'M', 'S', 'U', 'N', 'G', ' ', ' ', ' ', ' ', ' '},
	{'W', 'I', 'N', 'B', 'O', 'N', 'D', ' ', ' ', ' ', ' ', ' '},
	{'H', 'Y', 'N', 'I', 'X', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
	{'T', 'O', 'S', 'H', 'I', 'B', 'A', ' ', ' ', ' ', ' ', ' '},
	{'M', 'A', 'C', 'R', 'O', 'N', 'I', 'X', ' ', ' ', ' ', ' '},
	{0}
};

static onfi_mem_nand_handle_t onfi_handle;
/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Build IP command for NAND.
 *
 * @param userCommand  NAND device normal command.
 * @param addrMode  NAND address mode. Refer to "nand_addrmode_t".
 * @param cmdMode   NAND command mode. Refer to "nand_cmdmode_t".
 */
static inline uint16_t onfi_build_nand_ipcommand(uint8_t userCommand, nand_addrmode_t addrMode, nand_cmdmode_t cmdMode)
{
	return ((uint16_t)userCommand << 8U) | ((uint16_t)addrMode << 4U) | ((uint16_t)cmdMode & 0x000FU);
}

/* ONFI parameters CRC related functions. */
static const struct flash_parameters *flash_onfinand_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_onfinand_parameters;
}

static void onfi_nand_crc16_init(crc16_data_t *crc16Info)
{
	assert(crc16Info != NULL);

	/* initialize running crc and byte count. */
	crc16Info->currentCrc = 0x4F4EU;
}

static void onfi_nand_crc16_update(crc16_data_t *crc16Info, const uint8_t *src, uint32_t lengthInBytes)
{
	assert(crc16Info != NULL);
	assert(src != NULL);

	uint32_t crc = crc16Info->currentCrc;

	for (uint32_t i = 0U; i < lengthInBytes; ++i) {
		uint32_t byte = src[i];
		crc ^= byte << 8U;
		for (uint32_t j = 0U; j < 8U; ++j) {
			uint32_t temp = crc << 1U;
			if ((crc & 0x8000U) != 0x00U) {
				temp ^= 0x8005U;
			}
			crc = temp;
		}
	}

	crc16Info->currentCrc = (uint16_t)crc;
}

static void onfi_nand_crc16_finalize(crc16_data_t *crc16Info, uint16_t *hash)
{
	assert(crc16Info != NULL);
	assert(hash != NULL);
	*hash = crc16Info->currentCrc;
}

static void onfi_get_default_timing_configure(onfi_nand_timing_config_t *onfiNandTimingConfig)
{
	assert(onfiNandTimingConfig != NULL);

	/* Configure Timing mode 0 for timing parameter.*/
	const nand_ac_timing_parameter_t *nandTiming =
		&s_nandAcTimingParameterTable[kNandAcTimingTableIndex_ONFI_1p0_Mode0_10MHz];
	onfiNandTimingConfig->tCeSetup_Ns        = nandTiming->min_tCS_ns;
	onfiNandTimingConfig->tCeHold_Ns         = nandTiming->min_tCH_ns;
	onfiNandTimingConfig->tCeInterval_Ns     = nandTiming->min_tCEITV_ns;
	onfiNandTimingConfig->tWeLow_Ns          = nandTiming->min_tWP_ns;
	onfiNandTimingConfig->tWeHigh_Ns         = nandTiming->min_tWH_ns;
	onfiNandTimingConfig->tReLow_Ns          = nandTiming->min_tRP_ns;
	onfiNandTimingConfig->tReHigh_Ns         = nandTiming->min_tREH_ns;
	onfiNandTimingConfig->tTurnAround_Ns     = nandTiming->min_tTA_ns;
	onfiNandTimingConfig->tWehigh2Relow_Ns   = nandTiming->min_tWHR_ns;
	onfiNandTimingConfig->tRehigh2Welow_Ns   = nandTiming->min_tRHW_ns;
	onfiNandTimingConfig->tAle2WriteStart_Ns = nandTiming->min_tADL_ns;
	onfiNandTimingConfig->tReady2Relow_Ns    = nandTiming->min_tRR_ns;
	onfiNandTimingConfig->tWehigh2Busy_Ns    = nandTiming->max_tWB_ns;
}

static int onfi_nand_get_timing_configure(const struct device *dev, nand_handle_t *handle, onfi_mem_nand_config_t *onfiMemNandConfig)
{
	assert(handle->deviceSpecific != NULL);

	int ret = 0;
	uint32_t ch;
	uint32_t nandOnfiTag = FOUR_CHAR_CODE('O', 'N', 'F', 'I');
	nand_onfi_parameter_config_t nandOnfiParameterConfig = {0};
	uint8_t acTimingTableIndex = 0;
	onfi_nand_config_t *onfiConfig = onfiMemNandConfig->onfiNandConfig;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	/* Read First ONFI parameter data from device */
	ret = onfi_nand_issue_read_parameter(dev, handle, &nandOnfiParameterConfig);
	if (ret != 0) {
		return ret;
	}

	handle->ecc_bits = nandOnfiParameterConfig.reserved3[4];

	/*
	   Validate ONFI parameter:
	   From Device Spec, To insure data integrity, device contain more than one copies of the parameter page,
	   The Integrity CRC (Cyclic Redundancy Check) field is used to verify that the contents of the parameters page
	   were transferred correctly to the host.
	*/
	if (nandOnfiParameterConfig.signature == nandOnfiTag) {
		/*
		   Validate the interity CRC from ONFI spec:
		   1. The CRC calculation covers all of data between byte 0 and byte 253 of the parameter page inclusive.
		   2. The CRC shall be calculated on byte (8-bit) quantities starting with byte 0 in the parameter page.
			  The bits in the 8-bit quantity are processed from the most significant bit (bit 7) to the least
		   significant bit (bit 0).
		   3. The CRC shall be calculated using the following 16-bit generator polynomial: G(X) = X16 + X15 + X2
		   + 1. This polynomial in hex may be represented as 8005h.
		   4. The CRC value shall be initialized with a value of 4F4Eh before the calculation begins.
		   5. There is no XOR applied to the final CRC value after it is calculated.
		   6. There is no reversal of the data bytes or the CRC calculated value.
		*/
		uint16_t calculatedCrc   = 0;
		uint8_t *calculatedStart = (uint8_t *)&nandOnfiParameterConfig;
		uint32_t calculatedSize  = sizeof(nandOnfiParameterConfig) - 2U;

		crc16_data_t crc16Info;
		onfi_nand_crc16_init(&crc16Info);
		onfi_nand_crc16_update(&crc16Info, calculatedStart, calculatedSize);
		onfi_nand_crc16_finalize(&crc16Info, &calculatedCrc);

		if (calculatedCrc != nandOnfiParameterConfig.integrityCRC) {
			return -1;
		}
	} else {
		return -1;
	}

	/* Get device vendor */
	handle->vendorType = kNandVendorType_Unknown;
	for (uint8_t vendorIndex = kNandVendorType_Micron; vendorIndex < (uint8_t)kNandVendorType_Unknown; vendorIndex++) {
		for (ch = 0; ch < sizeof(s_nandDeviceManufacturerList[vendorIndex]); ch++) {
			if (s_nandDeviceManufacturerList[vendorIndex][ch] != nandOnfiParameterConfig.deviceManufacturer[ch]) {
				break;
			}
		}
		if (ch == sizeof(s_nandDeviceManufacturerList[vendorIndex])) {
			handle->vendorType = vendorIndex;
			break;
		}
	}

	/* Set NAND feature/command info in handler */
	onfiHandle->isFeatureCommandSupport =
		nandOnfiParameterConfig.optionalCommands.setGetfeatures != 0x00U ? true : false;

	if (nandOnfiParameterConfig.optionalCommands.readStatusEnhanced != 0x00U) {
		onfiHandle->statusCommandType = (uint8_t)kNandStatusCommandType_Enhanced;
	} else {
		onfiHandle->statusCommandType = (uint8_t)kNandStatusCommandType_Common;
	}

	if (nandOnfiParameterConfig.optionalCommands.changeReadColumnEnhanced != 0x00U) {
		onfiHandle->changeReadColumnType = (uint8_t)kNandChangeReadColumnCommandType_Enhanced;
	} else {
		onfiHandle->changeReadColumnType = (uint8_t)kNandChangeReadColumnCommandType_Common;
	}

	handle->bytesInPageDataArea	          = nandOnfiParameterConfig.dataBytesPerPage;
	handle->bytesInPageSpareArea          = nandOnfiParameterConfig.spareBytesPerPage;
	handle->pagesInBlock		          = nandOnfiParameterConfig.pagesPerBlock;
	handle->blocksInPlane		          = nandOnfiParameterConfig.blocksPerLUN;
	handle->planesInDevice		          = nandOnfiParameterConfig.LUNsPerDevice;
	onfiHandle->pageReadTimeInUs_tR       = ((uint32_t)nandOnfiParameterConfig.maxPageReadTimeInUs[1] << 8) +
									  nandOnfiParameterConfig.maxPageReadTimeInUs[0];
	onfiHandle->blockEraseTimeInUs_tBERS  = ((uint32_t)nandOnfiParameterConfig.maxBlockEraseTimeInUs[1] << 8) +
										   nandOnfiParameterConfig.maxBlockEraseTimeInUs[0];
	onfiHandle->PageProgramTimeInUs_tPROG = ((uint32_t)nandOnfiParameterConfig.maxPageProgramTimeInUs[1] << 8) +
											nandOnfiParameterConfig.maxPageProgramTimeInUs[0];
	onfiHandle->pageReadTimeInUs_tR >>= 2;
	onfiHandle->blockEraseTimeInUs_tBERS >>= 2;
	onfiHandle->PageProgramTimeInUs_tPROG >>= 2;
	/* Set change cloumn setup time for AXI access */
	onfiHandle->changeColumnSetupTimeInNs_tCCS =
		((uint32_t)nandOnfiParameterConfig.minChangeColunmSetupTimeInNs[1] << 8) +
		nandOnfiParameterConfig.minChangeColunmSetupTimeInNs[0];

	onfiConfig->addressCycle = nandOnfiParameterConfig.addressCycles;

	/* Get onfi timing mode for timing mode setting only when appliation have no timing configuration. */
	if (onfiConfig->timingConfig == NULL) {
		const nand_ac_timing_parameter_t *timingArray;

		if (nandOnfiParameterConfig.timingMode.mode5 != 0x00U) {
			acTimingTableIndex = kNandAcTimingTableIndex_ONFI_1p0_Mode5_50MHz;
		} else if (nandOnfiParameterConfig.timingMode.mode4 != 0x00U) {
			acTimingTableIndex = kNandAcTimingTableIndex_ONFI_1p0_Mode4_40MHz;
		} else if (nandOnfiParameterConfig.timingMode.mode3 != 0x00U) {
			acTimingTableIndex = kNandAcTimingTableIndex_ONFI_1p0_Mode3_33MHz;
		} else if (nandOnfiParameterConfig.timingMode.mode2 != 0x00U) {
			acTimingTableIndex = kNandAcTimingTableIndex_ONFI_1p0_Mode2_28MHz;
		} else if (nandOnfiParameterConfig.timingMode.mode1 != 0x00U) {
			acTimingTableIndex = kNandAcTimingTableIndex_ONFI_1p0_Mode1_20MHz;
		} else {
			acTimingTableIndex = kNandAcTimingTableIndex_ONFI_1p0_Mode0_10MHz;
		}

		/* Set the onfi nand configuration again */
		timingArray				     = &s_nandAcTimingParameterTable[acTimingTableIndex];
		onfiConfig->timingConfig->tCeSetup_Ns	     = timingArray->min_tCS_ns;
		onfiConfig->timingConfig->tCeHold_Ns	     = timingArray->min_tCH_ns;
		onfiConfig->timingConfig->tCeInterval_Ns     = timingArray->min_tCEITV_ns;
		onfiConfig->timingConfig->tWeLow_Ns          = timingArray->min_tWP_ns;
		onfiConfig->timingConfig->tWeHigh_Ns	     = timingArray->min_tWH_ns;
		onfiConfig->timingConfig->tReLow_Ns          = timingArray->min_tRP_ns;
		onfiConfig->timingConfig->tReHigh_Ns         = timingArray->min_tREH_ns;
		onfiConfig->timingConfig->tTurnAround_Ns     = timingArray->min_tTA_ns;
		onfiConfig->timingConfig->tWehigh2Relow_Ns   = timingArray->min_tWHR_ns;
		onfiConfig->timingConfig->tRehigh2Welow_Ns   = timingArray->min_tRHW_ns;
		onfiConfig->timingConfig->tAle2WriteStart_Ns = timingArray->min_tADL_ns;
		onfiConfig->timingConfig->tReady2Relow_Ns    = timingArray->min_tRR_ns;
		onfiConfig->timingConfig->tWehigh2Busy_Ns    = timingArray->max_tWB_ns;

		/* Change the timing mode: on onfi spec, enable EDO mode when using timing mode 4 and 5. */
		if ((acTimingTableIndex == (uint8_t)kNandAcTimingTableIndex_ONFI_1p0_Mode4_40MHz) ||
				(acTimingTableIndex == (uint8_t)kNandAcTimingTableIndex_ONFI_1p0_Mode5_50MHz)) {
			onfiConfig->edoModeEnabled = true;
		}

		if ((acTimingTableIndex > (uint8_t)kNandAcTimingTableIndex_ONFI_1p0_Mode0_10MHz) &&
				(nandOnfiParameterConfig.optionalCommands.setGetfeatures != 0x00U)) {
			/* Switch to specific timing mode */
			nand_onfi_feature_config_t featureConfig = {
				.command   = kNandDeviceCmd_ONFI_SetFeatures,
				.address   = 0x01, /* Feature address for timing mode. */
				.parameter = {acTimingTableIndex, 0, 0, 0},
			};
			ret = onfi_nand_issue_access_feature(dev, handle, &featureConfig);
			if (ret != 0) {
				return ret;
			}

			/* Get current timing mode to double check */
			featureConfig.command	   = kNandDeviceCmd_ONFI_GetFeatures;
			featureConfig.parameter[0] = 0;
			ret			   = onfi_nand_issue_access_feature(dev, handle, &featureConfig);
			if (ret != 0) {
				return ret;
			}

			if (featureConfig.parameter[0] != acTimingTableIndex) {
				return -1;
			}
		}
	}

	return ret;
}

static int onfi_nand_wait_status_ready(const struct device *dev, nand_handle_t *handle, uint32_t readyCheckIntervalInUs, bool readOpen)
{
	int ret = 0;
	bool isDeviceReady = false;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	if (onfiHandle->delayUS == NULL) {
		return -1;
	}

	while (!isDeviceReady) {
		if (onfiHandle->readyCheckOption == (uint8_t)kNandReadyCheckOption_SR) {
			/* Get SR value from Device by issuing READ STATUS commmand */
			uint8_t stat = 0;

			onfiHandle->delayUS(readyCheckIntervalInUs);
			ret = onfi_nand_issue_read_status(dev, handle, &stat);
			if (ret != 0) {
				return ret;
			}
			/* stat[RDY] = 0, Busy, stat[RDY] = 1, Ready */
			isDeviceReady = (stat & NAND_FLASH_SR_ONFI_READYBITMASK) == 0x00U ? false : true;
		} else if (onfiHandle->readyCheckOption == (uint8_t)kNandReadyCheckOption_RB) {
			/* Monitors the target's R/B# signal to determine the progress. */
			isDeviceReady = onfi_is_nand_ready(dev);
			if (!isDeviceReady) {
				onfiHandle->delayUS(readyCheckIntervalInUs);
				isDeviceReady = onfi_is_nand_ready(dev);
			}
		} else {
			; /* Intentional empty for MISRA c-2012 rule 15.7 */
		}
	}

	/* Note: If the ReadStatus command is used to monitor for command completion, the
	ReadMode command must be used to re-enable data output mode.
	*/
	if ((readOpen) && (onfiHandle->readyCheckOption == (uint8_t)kNandReadyCheckOption_SR)) {
		ret = onfi_nand_issue_read_mode(dev, handle);
		if (ret != 0) {
			return ret;
		}
	}
	return ret;
}

static int onfi_nand_issue_reset(const struct device *dev, nand_handle_t *handle)
{
	int ret = 0;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	/* The RESET command may be executed with the target in any state */
	uint16_t commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_Reset,
						   NANDAM_ColumnRow, // Don't care
						   NANDCM_Command);  // Command Only
	ret = onfi_send_command(dev, NAND_IPG_START_ADDRESS, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}
	/* For ONFI 1.0 Timing mode 0, the max tRST = 1000us
	   For ONFI 1.O other timing modes, the max tRST = 5/10/500us
	  The target is allowed a longer maximum reset time when a program or erase operation is in progress. The maximums
	  correspond to:
		1. The target is not performing an erase or program operation.
		2. The target is performing a program operation.
		3. The target is performing an erase operation.
	*/
	ret = onfi_nand_wait_status_ready(dev, handle, NAND_MAX_RST_TIME3_tRST_US, false);

	return ret;
}

static int onfi_nand_set_device_ecc(const struct device *dev, nand_handle_t *handle)
{
	int ret = 0;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	if (onfiHandle->eccCheckType == kNandEccCheckType_DeviceECC) {
		if (handle->vendorType == (uint8_t)kNandVendorType_Micron) {
			if (onfiHandle->isFeatureCommandSupport) {
				nand_onfi_feature_config_t featureConfig = {
					.command   = kNandDeviceCmd_ONFI_SetFeatures,
					.address   = kNandDeviceFeature_ArrayOperationMode_Address,
					.parameter = {kNandDeviceFeature_ArrayOperationMode_EnableECC, 0, 0, 0},
				};
				ret = onfi_nand_issue_access_feature(dev, handle, &featureConfig);
				if (ret != 0) {
					return ret;
				}
			}
		} else if (handle->vendorType == (uint8_t)kNandVendorType_Macronix) {
			if (onfiHandle->isFeatureCommandSupport) {
				/* The internal ECC is always enabled for MX30LF serials. */
			}
		} else {
			; /* Intentional empty */
		}
	} else {
		if (handle->vendorType == (uint8_t)kNandVendorType_Micron) {
			if (onfiHandle->isFeatureCommandSupport) {
				nand_onfi_feature_config_t featureConfig = {
					.command   = kNandDeviceCmd_ONFI_SetFeatures,
					.address   = kNandDeviceFeature_ArrayOperationMode_Address,
					.parameter = {kNandDeviceFeature_ArrayOperationMode_DisableECC, 0, 0, 0},
				};
				ret = onfi_nand_issue_access_feature(dev, handle, &featureConfig);
				if (ret != 0) {
					return ret;
				}
			}
		}
	}

	return ret;
}

static int onfi_nand_issue_read_mode(const struct device *dev, nand_handle_t *handle)
{
	int ret = 0;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	/* READ MODE command is accepted by device when it is ready (RDY = 1, ARDY = 1). */
	ret = onfi_nand_wait_status_ready(dev, handle, NAND_READY_CHECK_INTERVAL_NORMAL, false);
	if (ret != 0) {
		return ret;
	}

	/* The READ MODE command disables status output and enables data output */
	uint16_t commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ReadMode,
						   NANDAM_ColumnRow, // Don't care
						   NANDCM_Command);  // Command Only
	ret = onfi_send_command(dev, NAND_IPG_START_ADDRESS,
							commandCode, 0, NULL);

	return ret;
}

static int onfi_nand_issue_access_feature(const struct device *dev, nand_handle_t *handle, nand_onfi_feature_config_t *featureConfig)
{
	int ret = 0;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	/* SET/GET FEATURES command is accepted by the target only when all die (LUNs) on the target are idle. */
	uint16_t commandCode = onfi_build_nand_ipcommand(featureConfig->command,
						   NANDAM_ColumnCA0,	   // CA1
						   NANDCM_CommandAddress); // Commmand Address
	ret = onfi_send_command(dev, featureConfig->address, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	if (featureConfig->command == (uint8_t)kNandDeviceCmd_ONFI_SetFeatures) {
		ret = onfi_write(dev, 0, featureConfig->parameter, sizeof(featureConfig->parameter));
		if (ret != 0) {
			return ret;
		}
		/* Note: From Spec, both R/B and SR can be used to determine the progress,
			but actually only when we choose R/B, then it works well on the EVB and FPGA. */
		if (onfiHandle->readyCheckOption == (uint8_t)kNandReadyCheckOption_RB) {
			ret = onfi_nand_wait_status_ready(dev, handle, NAND_READY_CHECK_INTERVAL_NORMAL, false);
		} else {
			/* Just delay some time to workaround issue */
			onfiHandle->delayUS(NAND_MAX_FEATURE_ACCESS_TIME_tFEAT_US);
		}
	} else if (featureConfig->command == (uint8_t)kNandDeviceCmd_ONFI_GetFeatures) {
		/* Note: From Spec, both R/B and SR can be used to determine the progress,
			but actually only when we choose R/B, then it works well on the EVB and FPGA. */
		if (onfiHandle->readyCheckOption == (uint8_t)kNandReadyCheckOption_RB) {
			ret = onfi_nand_wait_status_ready(dev, handle, NAND_READY_CHECK_INTERVAL_NORMAL, true);
			if (ret != 0) {
				return ret;
			}
		} else {
			/* Just delay some time to workaround issue */
			onfiHandle->delayUS(NAND_MAX_FEATURE_ACCESS_TIME_tFEAT_US);
		}

		ret = onfi_read(dev, 0, featureConfig->parameter, sizeof(featureConfig->parameter));
	} else {
		; /* Intentional empty */
	}

	return ret;
}

static int onfi_nand_issue_read_parameter(const struct device *dev, nand_handle_t *handle, nand_onfi_parameter_config_t *parameterConfig)
{
	int ret = 0;
	uint32_t readyCheckIntervalInUs;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	uint16_t commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ReadParameterPage,
						   NANDAM_ColumnCA0,	   // 1 byte address
						   NANDCM_CommandAddress); // Command Address
	ret = onfi_send_command(dev, 0, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	// Note2: ReadStatus may be used to check the status of Read Parameter Page during execution.
	// Note3: Use of the ReadStatusEnhanced command is prohibited during the power-on
	//  Reset command and when OTP mode is enabled. It is also prohibited following
	//  some of the other reset, identification, and configuration operations.
	readyCheckIntervalInUs = (NAND_MAX_READ_PARAMETER_PAGE_TIME_tR_US > onfiHandle->pageReadTimeInUs_tR) ?
							 NAND_MAX_READ_PARAMETER_PAGE_TIME_tR_US :
							 onfiHandle->pageReadTimeInUs_tR;
	if (onfiHandle->readyCheckOption == (uint8_t)kNandReadyCheckOption_RB) {
		ret = onfi_nand_wait_status_ready(dev, handle, readyCheckIntervalInUs, true);
		if (ret != 0) {
			return ret;
		}
	} else {
		onfiHandle->delayUS(readyCheckIntervalInUs);
	}

	// Only ipg command is supported here
	ret = onfi_read(dev, 0, (uint8_t *)parameterConfig, sizeof(nand_onfi_parameter_config_t));

	return ret;
}

static int onfi_nand_issue_read_status(const struct device *dev, nand_handle_t *handle, uint8_t *stat)
{
	int ret = 0;
	uint32_t readoutData = 0x00U;
	uint16_t commandCode;
	uint32_t slaveAddress;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	/* Note: If there is only one plane per target, the READ STATUS (70h) command can be used
	   to return status following any NAND command.
	   Note: In devices that have more than one plane per target, during and following interleaved
	   die (multi-plane) operations, the READ STATUS ENHANCED command must be used to select the
	   die (LUN) that should report status.
	*/
	if (onfiHandle->statusCommandType == (uint8_t)kNandStatusCommandType_Enhanced) {
		/* READ STATUS ENHANCED command is accepted by all planes in device even when they are busy (RDY = 0).*/
		commandCode  = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ReadStatusEnhanced,
					   NANDAM_RawRA0RA1RA2,		// 3 Byte-RA0/RA1/RA2
					   NANDCM_CommandAddressRead); // Commmand Address Read
		slaveAddress = onfiHandle->rowAddressToGetSR;
	} else {
		/* READ STATUS command is accepted by device even when it is busy (RDY = 0). */
		commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ReadStatus,
					   NANDAM_ColumnRow,	// Don't care
					   NANDCM_CommandRead); // Commmand Read

		/* Note: For those command without address, the address should be valid as well,
		  it shouldn't be out of IPG memory space, or IP will ignore this command.
		*/
		slaveAddress = NAND_IPG_START_ADDRESS;
	}
	ret = onfi_send_command(dev, slaveAddress, commandCode, 0, &readoutData);
	if (ret != 0) {
		return ret;
	}

	/* Set SR value according to readout data from Device */
	*stat = (uint8_t)readoutData;

	return ret;
}

static int onfi_nand_issue_read_page(const struct device *dev, nand_handle_t *handle, uint32_t ipgCmdAddr)
{
	int ret = 0;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	/* READ PAGE command is accepted by the device when it is ready (RDY = 1, ARDY = 1). */
	ret = onfi_nand_wait_status_ready(dev, handle, NAND_READY_CHECK_INTERVAL_NORMAL, false);
	if (ret != 0) {
		return ret;
	}

	uint16_t commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ReadPageSetup,
						   NANDAM_ColumnRow,	   // Address value
						   NANDCM_CommandAddress); // Command Address
	ret = onfi_send_command(dev, ipgCmdAddr, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ReadPageConfirm,
						   NANDAM_ColumnRow,	// Don't care
						   NANDCM_CommandHold); // Commmand Hold
	ret = onfi_send_command(dev, ipgCmdAddr, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	/* Monitors the target's R/B# signal or Reads the status register
	  to determine the progress of the page data transfer. */
	ret = onfi_nand_wait_status_ready(dev, handle, onfiHandle->pageReadTimeInUs_tR, true);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

static int onfi_nand_is_device_ecc_check_passed(const struct device *dev, nand_handle_t *handle, bool *isEccPassed)
{
	int ret = 0;
	uint8_t SR = 0;

	/* During READ operations the device executes the internal ECC engine (n-bit detection
		and (n-1)-bit error correction). When the READ operaton is complete, read status bit 0 must
		be checked to determine whether errors larger than n bits have occurred. */

	/* Note1: For MT29 series device: We just need to check SR[PASS] to see the ECC result for
		all types of operation(PROGRAM/ERASE/READ)
	   Note2: For S34 series device: Error Detection Code check is a feature that can be used during the
	  copy back program operation. For common program/erase, the Status Bit SR[PASS] may be checked. The
	  internal write/erase verify detects only errors for 1’s/0's that are not successfully programmed to 0’s/1's.
	  */
	ret = onfi_nand_issue_read_status(dev, handle, &SR);
	if (ret == 0) {
		/* SR[PASS] = 0, Successful PROGRAM/ERASE/READ; SR[PASS] = 1, Error in PROGRAM/ERASE/READ */
		*isEccPassed = (SR & NAND_FLASH_SR_ONFI_PASSBITMASK) == 0x00U ? true : false;
	} else {
		*isEccPassed = false;
	}

	/* READ MODE command should be issued in case read cycle is following. */
	ret = onfi_nand_issue_read_mode(dev, handle);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

/* Initialize Parallel NAND Flash device */
int Nand_Flash_Init(const struct device *dev, onfi_mem_nand_config_t *onfiConfig, nand_handle_t *handle)
{
	assert(config != NULL);
	assert(handle != NULL);

	onfi_nand_timing_config_t onfiNandTimingConfig;
	onfi_mem_nand_handle_t *onfiHandle;
	bool setFlag = false;
	int ret = 0;

	(void)memset(handle, 0, sizeof(nand_handle_t));

	/* Store all needs for NAND operations. */
	handle->deviceSpecific       = (void *)&onfi_handle;
	onfiHandle                   = (onfi_mem_nand_handle_t *)handle->deviceSpecific;
	onfiHandle->delayUS          = onfiConfig->delayUS;
	onfiHandle->eccCheckType     = onfiConfig->eccCheckType;
	onfiHandle->readyCheckOption = onfiConfig->readyCheckOption;
	onfiHandle->columnWidth      = NAND_FLASH_COLUMNBITSNUM;
	/* Currently we only support ONFI device. */
	if (onfiConfig->onfiVersion == kNandOnfiVersion_None) {
		return -1;
	}

	if (onfiConfig->onfiNandConfig->timingConfig == NULL) {
		/* Prepare the NAND configuration part one: get timing parameter in NAND configure structure. */
		onfi_get_default_timing_configure(&onfiNandTimingConfig);
		onfiConfig->onfiNandConfig->timingConfig = &onfiNandTimingConfig;
		setFlag = true;
	}

	/* Configure NAND flash. */
	ret = onfi_configure_nand(dev, onfiConfig->onfiNandConfig);
	if (ret != 0) {
		if (setFlag) {
			/* Clear the given timing configure variable. */
			onfiConfig->onfiNandConfig->timingConfig = NULL;
		}
		return ret;
	}

	/* Issues the RESET command to device, make sure that we have clean NAND device status */
	ret = onfi_nand_issue_reset(dev, handle);
	if (ret != 0) {
		if (setFlag) {
			/* Clear the given timing configure variable. */
			onfiConfig->onfiNandConfig->timingConfig = NULL;
		}
		return ret;
	}

	/* Try to read ONFI parameter and reset the configure parameters. */
	ret = onfi_nand_get_timing_configure(dev, handle, onfiConfig);
	if (ret != 0) {
		if (setFlag) {
			/* Clear the given timing configure variable. */
			onfiConfig->onfiNandConfig->timingConfig = NULL;
		}
		return ret;
	}

	/* Re-Init NAND module using new parameter */
	ret = onfi_configure_nand(dev, onfiConfig->onfiNandConfig);
	if (ret != 0) {
		if (setFlag) {
			/* Clear the given timing configure variable. */
			onfiConfig->onfiNandConfig->timingConfig = NULL;
		}
		return ret;
	}

	/* Enable/Disable device ECC if necessary */
	ret = onfi_nand_set_device_ecc(dev, handle);
	if (ret != 0) {
		if (setFlag) {
			/* Clear the given timing configure variable. */
			onfiConfig->onfiNandConfig->timingConfig = NULL;
		}
		return ret;
	}
	/* Clear the given timing configure variable. */
	if (setFlag) {
		onfiConfig->onfiNandConfig->timingConfig = NULL;
	}
	return ret;
}

int Nand_Flash_Read_Page(const struct device *dev, nand_handle_t *handle, uint32_t pageIndex, uint8_t *buffer, uint32_t length)
{
	int ret = 0;
	uint32_t ipgCmdAddr;
	bool eccCheckPassed                = false;
	uint32_t pageSize                  = handle->bytesInPageDataArea + handle->bytesInPageSpareArea;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;
	uint32_t pageNum                   = handle->pagesInBlock * handle->blocksInPlane * handle->planesInDevice;

	ipgCmdAddr = pageIndex * (1UL << onfiHandle->columnWidth);
	/* Validate given length */
	if ((length > pageSize) || (pageIndex >= pageNum)) {
		return -1;
	}
	onfiHandle->rowAddressToGetSR = ipgCmdAddr;

	/* Issues the page read command to device */
	ret = onfi_nand_issue_read_page(dev, handle, ipgCmdAddr);
	if (ret != 0) {
		return ret;
	}

	if (onfiHandle->eccCheckType == kNandEccCheckType_DeviceECC) {
		/* Check NAND Device ECC status if applicable */
		ret = onfi_nand_is_device_ecc_check_passed(dev, handle, &eccCheckPassed);
		if ((ret != 0) || (!eccCheckPassed)) {
			return -1;
		}
	}

	ret = onfi_read(dev, 0, buffer, length);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

int Nand_Flash_Page_Program(const struct device *dev, nand_handle_t *handle, uint32_t pageIndex, const uint8_t *src, uint32_t length)
{
	int ret = 0;
	uint32_t pageNum = handle->pagesInBlock * handle->blocksInPlane * handle->planesInDevice;
	uint32_t ipgCmdAddr;
	bool eccCheckPassed = false;
	uint16_t commandCode;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;

	if ((length > (handle->bytesInPageDataArea + handle->bytesInPageSpareArea)) || (pageIndex >= pageNum)) {
		return -1;
	}

	ipgCmdAddr                    = pageIndex * (1UL << onfiHandle->columnWidth);
	onfiHandle->rowAddressToGetSR = ipgCmdAddr;

	/* PROGRAM PAGE command is accepted by the device when it is ready (RDY = 1, ARDY = 1). */
	ret = onfi_nand_wait_status_ready(dev, handle, NAND_READY_CHECK_INTERVAL_NORMAL, false);
	if (ret != 0) {
		return ret;
	}
	commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ProgramPageSetup,
						   NANDAM_ColumnRow,	   // Address value
						   NANDCM_CommandAddress); // Command Address
	ret = onfi_send_command(dev, ipgCmdAddr, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	ret = onfi_write(dev, 0, src, length);
	if (ret != 0) {
		return ret;
	}

	/* Issues the page program command to device */
	commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_ProgramPageConfirm,
						   NANDAM_ColumnRow, // Don't care
						   NANDCM_Command);  // Commmand Only
	ret = onfi_send_command(dev, ipgCmdAddr, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}
	/* Monitors the target's R/B# signal or Reads the status register
	  to determine the progress of the page data transfer. */
	ret = onfi_nand_wait_status_ready(dev, handle, onfiHandle->PageProgramTimeInUs_tPROG, true);
	if (ret != 0) {
		return ret;
	}

	if (onfiHandle->eccCheckType == kNandEccCheckType_DeviceECC) {
		/* Check NAND Device ECC status if applicable */
		ret = onfi_nand_is_device_ecc_check_passed(dev, handle, &eccCheckPassed);
		if ((ret != 0) || (!eccCheckPassed)) {
			return -1;
		}
	}

	return ret;
}

int Nand_Flash_Erase_Block(const struct device *dev, nand_handle_t *handle, uint32_t blockIndex)
{
	int ret = 0;
	uint32_t ipgCmdAddr;
	bool eccCheckPassed                = false;
	onfi_mem_nand_handle_t *onfiHandle = (onfi_mem_nand_handle_t *)handle->deviceSpecific;
	ipgCmdAddr                         = blockIndex * handle->pagesInBlock * (1UL << onfiHandle->columnWidth);

	if (blockIndex >= handle->blocksInPlane * handle->planesInDevice) {
		return -1;
	}

	onfiHandle->rowAddressToGetSR = ipgCmdAddr;

	/* Issues the block erase command to device
	  ERASE BLOCK command is accepted by the device when it is ready (RDY = 1, ARDY = 1).*/
	ret = onfi_nand_wait_status_ready(dev, handle, NAND_READY_CHECK_INTERVAL_NORMAL, false);
	if (ret != 0) {
		return ret;
	}
	/* SA = blockIndex * pagesInBlock * pageDataSize */
	uint16_t commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_EraseBlockSetup,
						   NANDAM_RawRA0RA1RA2,	// Address value
						   NANDCM_CommandAddress); // Command Address
	ret = onfi_send_command(dev, ipgCmdAddr, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	commandCode = onfi_build_nand_ipcommand(kNandDeviceCmd_ONFI_EraseBlockConfirm,
						   NANDAM_RawRA0RA1RA2, // Don't care
						   NANDCM_CommandHold); // Commmand Hold
	ret = onfi_send_command(dev, ipgCmdAddr, commandCode, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	ret = onfi_nand_wait_status_ready(dev, handle, onfiHandle->blockEraseTimeInUs_tBERS, false);
	if (ret != 0) {
		return ret;
	}

	if (onfiHandle->eccCheckType == kNandEccCheckType_DeviceECC) {
		/* Check NAND Device ECC status if applicable */
		ret = onfi_nand_is_device_ecc_check_passed(dev, handle, &eccCheckPassed);
		if ((ret != 0) || (!eccCheckPassed)) {
			return -1;
		}
	}

	return ret;
}

static int onfi_nand_erase(const struct device *dev, off_t addr, size_t size)
{
	struct onfi_nand_data *data = dev->data;
	int ret = 0;

	if (addr % ONFINAND_BLOCK_SIZE) {
		LOG_ERR("Invalid address");
		return -EINVAL;
	}

	if (size > ONFINAND_BLOCK_OFFSET) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	uint32_t blockIndex = addr / ((1UL << ((onfi_mem_nand_handle_t *)data->nandhandle->deviceSpecific)->columnWidth) * data->nandhandle->pagesInBlock);

	ret = Nand_Flash_Erase_Block(data->controller, data->nandhandle, blockIndex);

	if (ret != 0) {
		LOG_ERR("NAND Flash erase fail!");
		return ret;
	}

	return 0;
}

static int onfi_nand_init(const struct device *dev)
{
	struct onfi_nand_data *data = dev->data;
	int ret = 0;

	ret = Nand_Flash_Init(data->controller, data->nandconfig, data->nandhandle);

	if (ret != 0) {
		LOG_ERR("NAND Flash initialize fail!");
		return ret;
	}

	if (data->nandhandle->ecc_bits > 0) {
		bch_ecc_init(dev, data->nandhandle->ecc_bits);
	}

	return  0;
}

static int onfi_nand_write(const struct device *dev, off_t addr, const uint8_t *buffer, size_t size)
{
	struct onfi_nand_data *data = dev->data;
	int ret = 0;
	uint8_t *p = (uint8_t *)data->page_buf;
	uint8_t ecc_steps = data->ecc_steps;
	uint32_t written_bytes = data->page_size + data->nandhandle->bytesInPageSpareArea;
	if (addr % data->page_size) {
		LOG_ERR("Invalid address");
		return -EINVAL;
	}
	if (size > data->page_size) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}
	if (data->nandhandle->ecc_bits > 0) {
		// prepare data
		memset(data->page_buf, 0xff, data->page_size + data->nandhandle->bytesInPageSpareArea);
		memcpy(data->page_buf, buffer, data->page_size);
		// calculate the software ECC
		for (uint8_t i = 0; ecc_steps; ecc_steps--, i += data->ecc_bytes, p += data->ecc_size) {
			memset(data->nbc.bch->input_data, 0x0, (1 << data->nbc.bch->m) / 8);
			memcpy(data->nbc.bch->input_data + data->ecc_bytes, p, data->ecc_size);
			bch_calculate_ecc(dev, data->nbc.bch->input_data, data->ecc_calc + i);
		}

		// prepare ECC code
		memcpy(data->page_buf + data->page_size + data->ecc_layout_pos,
			   data->ecc_calc, data->ecc_bytes * data->ecc_steps);
	}

	uint32_t pageIndex = addr / (1UL << ((onfi_mem_nand_handle_t *)data->nandhandle->deviceSpecific)->columnWidth);

	ret = Nand_Flash_Page_Program(data->controller, data->nandhandle, pageIndex, (uint8_t *)data->page_buf, written_bytes);

	if (ret != 0) {
		LOG_ERR("NAND Flash write fail!");
		return ret;
	}

	return 0;
}

static int onfi_nand_read(const struct device *dev, off_t addr, void *buffer, size_t size)
{
	struct onfi_nand_data *data = dev->data;
	int ret = 0;
	uint32_t read_bytes = data->page_size + data->nandhandle->bytesInPageSpareArea;
	uint8_t ecc_steps = data->ecc_steps;
	uint8_t *p = (uint8_t *)data->page_buf;

	if (addr % data->page_size) {
		LOG_ERR("Invalid address");
		return -EINVAL;
	}
	if (size > data->page_size) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	uint32_t pageIndex = addr / (1UL << ((onfi_mem_nand_handle_t *)data->nandhandle->deviceSpecific)->columnWidth);

	ret = Nand_Flash_Read_Page(data->controller, data->nandhandle, pageIndex, data->page_buf, read_bytes);

	if (ret != 0) {
		LOG_ERR("NAND Flash read fail!");
		return ret;
	}
	if (data->nandhandle->ecc_bits > 0) {

		memcpy(data->ecc_code, data->page_buf + data->page_size + data->ecc_layout_pos,
			   data->ecc_bytes * data->ecc_steps);

		ecc_steps = data->ecc_steps;
		for (uint8_t i = 0; ecc_steps; ecc_steps--, i += data->ecc_bytes, p += data->ecc_size) {
			memset(data->nbc.bch->input_data, 0x0, (1 << data->nbc.bch->m) / 8);
			memcpy(data->nbc.bch->input_data + data->ecc_bytes, p, data->ecc_size);

			int ret = bch_decode(data->nbc.bch, data->nbc.bch->input_data, (unsigned int *)(data->ecc_code + i));

			if (ret < 0) {
				LOG_ERR("Reading data failed");
				return -ENODEV;
			}
			memcpy(p, data->nbc.bch->input_data + data->ecc_bytes, data->ecc_size);
		}
	}
	memcpy(buffer, data->page_buf, data->page_size);

	return 0;
}

static const struct flash_driver_api onfi_nand_api = {
	.erase = onfi_nand_erase,
	.write = onfi_nand_write,
	.read = onfi_nand_read,
	.get_parameters = flash_onfinand_get_parameters,
};

static struct onfi_nand_data onfi_nand_data_0 = {
	.controller = DEVICE_DT_GET(DT_INST_BUS(0)),
	.nandconfig = &onfiMemConfig,
	.nandhandle = &nandHandle,
	.block_size = DT_INST_PROP(0, block_size),
	.block_offset = DT_INST_PROP(0, block_offset),
	.page_size = DT_INST_PROP(0, page_size),
	.page_offset = DT_INST_PROP(0, page_offset),
};

DEVICE_DT_INST_DEFINE(0, &onfi_nand_init, NULL, &onfi_nand_data_0,
					  NULL,
					  POST_KERNEL, 85,
					  &onfi_nand_api);
