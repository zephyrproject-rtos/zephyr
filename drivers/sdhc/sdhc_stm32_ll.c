/*
 * Copyright (c) 2025 EXALT Technologies.
 * Copyright (c) 2017 STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This file is derived from STM32Cube HAL (stm32XXxx_hal_sdio.c)
 * with refactoring to align with Zephyr coding style and architecture.
 */

#include "sdhc_stm32_ll.h"
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sdhc_stm32_ll, CONFIG_SDHC_LOG_LEVEL);

/* Private validation macros for SDIO parameters */
#define IS_SDIO_RAW_FLAG(ReadAfterWrite) (((ReadAfterWrite) == 0U) || ((ReadAfterWrite) == 1U))
#define IS_SDIO_FUNCTION(FN)             (((FN) >= 0U) && ((FN) <= 7U))

/* SDMMC data error flags */
#define SDMMC_DATA_ERROR_FLAGS                                                                     \
	(SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR)

/* SDMMC wait flags for RX operations */
#define SDMMC_WAIT_RX_FLAGS                                                                        \
	(SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)

/* SDMMC wait flags for TX operations */
#define SDMMC_WAIT_TX_FLAGS                                                                        \
	(SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)

/**
 * @brief Read multiple words from FIFO (FIFO half-full condition)
 *
 * @param Instance Pointer to SDMMC register base
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @param pBuf Pointer to destination buffer pointer (updated after read)
 */
static void sdhc_stm32_ll_read_fifo_block(SDMMC_TypeDef *Instance, uint8_t **pBuf)
{
	uint32_t data;
	uint8_t *buf = *pBuf;

	for (uint32_t count = 0U; count < (SDMMC_FIFO_SIZE / sizeof(uint32_t)); count++) {
		data = SDMMC_ReadFIFO(Instance);
		UNALIGNED_PUT(data, (uint32_t *)buf);
		buf += 4;
	}
	*pBuf = buf;
}

/**
 * @brief Handle data transfer errors and cleanup
 *
 * @param Instance Pointer to SDMMC register base
 * @param errorcode Error code to set
 * @param data Pointer to the STM32 SDHC data structure.
 * @return int 0 on success, negative errno on failure
 */
static int sdhc_stm32_ll_handle_data_error(SDMMC_TypeDef *Instance, uint32_t errorcode,
					   struct sdhc_stm32_data *data)
{
	__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
	data->error_code |= errorcode;
	if (errorcode == SDMMC_ERROR_DATA_TIMEOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

/**
 * @brief Disable all data transfer interrupts
 *
 * @param Instance Pointer to SDMMC register base
 */
static void sdhc_stm32_ll_disable_data_interrupts(SDMMC_TypeDef *Instance)
{
	__SDMMC_DISABLE_IT(Instance, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT |
					     SDMMC_IT_TXUNDERR | SDMMC_IT_RXOVERR |
					     SDMMC_IT_TXFIFOHE | SDMMC_IT_RXFIFOHF);
	__SDMMC_DISABLE_IT(Instance, SDMMC_IT_IDMABTC);
}

/**
 * @brief Helper function to convert block size to SDMMC_DataBlockSize
 * @param block_size Block size in bytes (must be power of 2, 1-16384)
 * @return SDMMC DataBlockSize value
 */
static uint32_t sdhc_stm32_ll_convert_block_size(uint32_t block_size)
{
	/* SDMMC_DATABLOCK_SIZE_<n>B is log2(n) shifted to DBLOCKSIZE bit position */
	__ASSERT(IS_POWER_OF_TWO(block_size) && block_size >= 1U && block_size <= 16384U,
		 "Invalid block size: %u", block_size);

	return LOG2(block_size) << SDMMC_DCTRL_DBLOCKSIZE_Pos;
}

/**
 * @brief Configure SDIO data path state machine for extended commands
 *
 * @param Instance Pointer to SDMMC register base
 * @param Argument Extended command argument structure
 * @param Size_byte Number of bytes to transfer
 * @param nbr_of_block Number of blocks (output)
 * @param config Data configuration structure (output)
 * @param is_write true for write operation, false for read
 * @param dev_data Pointer to the STM32 SDHC data structure.
 */
static void sdhc_stm32_ll_config_sdio_dpsm(SDMMC_TypeDef *Instance,
					   sdhc_stm32_sdio_ext_cmd_t *Argument, uint32_t Size_byte,
					   uint32_t *nbr_of_block, SDMMC_DataInitTypeDef *config,
					   bool is_write, struct sdhc_stm32_data *dev_data)
{
	/* Compute number of blocks */
	*nbr_of_block = (Size_byte & ~(dev_data->block_size & 1U)) >>
			(find_lsb_set(dev_data->block_size) - 1);

	/* Initialize data control register */
	if ((Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		Instance->DCTRL = 0U;
	}

	/* Configure SDIO Data Path State Machine (DPSM) */
	config->DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		uint32_t blocks = *nbr_of_block;

		config->DataLength = blocks * dev_data->block_size;
		config->DataBlockSize = sdhc_stm32_ll_convert_block_size(dev_data->block_size);
	} else {
		config->DataLength = (Size_byte > 0U) ? Size_byte : 512U;
		config->DataBlockSize = SDMMC_DATABLOCK_SIZE_1B;
	}

	config->TransferDir = is_write ? SDMMC_TRANSFER_DIR_TO_CARD : SDMMC_TRANSFER_DIR_TO_SDMMC;
	config->TransferMode = (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK)
				       ? SDMMC_TRANSFER_MODE_BLOCK
				       : SDMMC_TRANSFER_MODE_SDIO;
	config->DPSM = SDMMC_DPSM_DISABLE;
}

/**
 * @brief Build CMD53 argument for SDIO extended read/write
 *
 * @param Argument Extended command argument structure
 * @param nbr_of_block Number of blocks
 * @param Size_byte Size in bytes
 * @param is_write true for write operation, false for read
 * @return uint32_t CMD53 argument value
 */
static uint32_t sdhc_stm32_ll_build_cmd53_arg(sdhc_stm32_sdio_ext_cmd_t *Argument,
					      uint32_t nbr_of_block, uint32_t Size_byte,
					      bool is_write)
{
	uint32_t cmd;
	uint32_t count_field;

	cmd = is_write ? (1U << 31U) : (0U << 31U);
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->Block_Mode) << 27U;
	cmd |= ((uint32_t)Argument->OpCode) << 26U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;

	if (nbr_of_block == 0U) {
		count_field = Size_byte;
	} else {
		count_field = nbr_of_block;
	}
	cmd |= count_field & 0x1FFU;

	return cmd;
}

/**
 * @brief Handle CMD53 command error
 *
 * @param Instance Pointer to SDMMC register base
 * @param errorstate Error state from command
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @return true if fatal error (should return), false to continue
 */
static bool sdhc_stm32_ll_handle_cmd53_error(SDMMC_TypeDef *Instance, uint32_t errorstate,
					     struct sdhc_stm32_data *dev_data)
{
	if (errorstate == 0) {
		return false;
	}

	dev_data->error_code |= errorstate;

	if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
			   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
		stm32_reg_modify_bits(&Instance->DCTRL, SDMMC_DCTRL_FIFORST, SDMMC_DCTRL_FIFORST);
		__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
		__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_DATA_FLAGS);
		return true;
	}

	return false;
}

/**
 * @brief Read remaining bytes from FIFO (less than 32 bytes)
 *
 * @param Instance Pointer to SDMMC register base
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @param tempbuff Pointer to buffer pointer (updated after read)
 * @param dataremaining Pointer to remaining data count (updated after read)
 */
static void sdhc_stm32_ll_read_remaining_bytes(SDMMC_TypeDef *Instance, uint8_t **tempbuff,
					       uint32_t *dataremaining)
{
	uint32_t data;
	uint8_t byteCount;

	while ((*dataremaining > 0U) && !(__SDMMC_GET_FLAG(Instance, SDMMC_FLAG_RXFIFOE))) {
		data = SDMMC_ReadFIFO(Instance);
		if (*dataremaining >= 4U) {
			UNALIGNED_PUT(data, (uint32_t *)*tempbuff);
			*tempbuff += 4;
			*dataremaining -= 4;
		} else {
			for (byteCount = 0U; byteCount < *dataremaining; byteCount++) {
				**tempbuff = (uint8_t)((data >> (byteCount * 8U)) & 0xFFU);
				(*tempbuff)++;
			}
			*dataremaining = 0;
		}
	}
}

/**
 * @brief Write remaining bytes to FIFO (less than 32 bytes)
 *
 * @param Instance Pointer to SDMMC register base
 * @param u8buff Pointer to buffer
 * @param dataremaining Pointer to remaining data count (updated after write)
 */
static void sdhc_stm32_ll_write_remaining_bytes(SDMMC_TypeDef *Instance, uint8_t **u8buff,
						uint32_t *dataremaining)
{
	uint32_t data;
	uint8_t byteCount;

	while (*dataremaining > 0U) {
		if (*dataremaining >= 4U) {
			data = UNALIGNED_GET((uint32_t *)*u8buff);
			*u8buff += 4;
			*dataremaining -= 4;
		} else {
			data = 0U;
			for (byteCount = 0U; byteCount < *dataremaining; byteCount++) {
				data |= ((uint32_t)(**u8buff) << (byteCount << 3U));
				(*u8buff)++;
			}
			*dataremaining = 0;
		}
		Instance->FIFO = data;
	}
}

/**
 * @brief SDIO interrupt handler
 *
 * This function handles SDIO interrupts. It checks for various flags including
 * DATAEND, DCRCFAIL, DTIMEOUT, RXOVERR, and TXUNDERR.
 *
 * For DMA transfers, it disables DMA and clears the data path after completion.
 * This is a simplified version that doesn't handle multi-part transfers or callbacks.
 * @param Instance Pointer to SDMMC register base
 * @param data Pointer to the STM32 SDHC data structure
 */
void sdhc_stm32_ll_irq_handler(SDMMC_TypeDef *Instance, struct sdhc_stm32_data *data);
{
	uint32_t flags;
	uint32_t errorcode;
	const struct sdhc_stm32_config *config = dev->config;

	/* Read interrupt flags */
	flags = READ_REG(Instance->STA);

	/* Check for data transfer completion */
	if (READ_BIT(flags, SDMMC_FLAG_DATAEND) != 0U) {
		__SDMMC_CLEAR_FLAG(Instance, SDMMC_FLAG_DATAEND);

		/* Disable all data transfer interrupts */
		sdhc_stm32_ll_disable_data_interrupts(Instance);
		__SDMMC_CMDTRANS_DISABLE(Instance);

		/* If DMA was used, clean up DMA configuration */
		Instance->DLEN = 0;
		Instance->IDMACTRL = SDMMC_DISABLE_IDMA;

		/* Reset DCTRL register, preserving SDIOEN bit if it was set */
		if ((Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
			Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
		} else {
			Instance->DCTRL = 0U;
		}

		LOG_DBG("SDIO data transfer completed");
	}

	/* Check for errors */
	errorcode = __SDMMC_GET_FLAG(Instance, SDMMC_DATA_ERROR_FLAGS);
	if (errorcode != 0U) {
		dev_data->error_code |= errorcode;

		/* Clear error flags */
		__SDMMC_CLEAR_FLAG(Instance, SDMMC_DATA_ERROR_FLAGS);

		/* Disable interrupts */
		sdhc_stm32_ll_disable_data_interrupts(Instance);

		LOG_ERR("SDIO transfer error: 0x%x", dev_data->error_code);
	}
}

/**
 * @brief Configure SDIO/SDMMC clock frequency
 *
 * This function configures the SDMMC clock divider to achieve the desired
 * clock frequency. It directly manipulates the CLKCR register.
 * @param Instance Pointer to SDMMC register base
 * @param ClockSpeed Desired clock speed in Hz
 * @param data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_config_freq(SDMMC_TypeDef *Instance, uint32_t ClockSpeed,
			      struct sdhc_stm32_data *data)
{
	uint32_t ClockDiv;

	/* Check the parameters */
	assert_param(IS_SDMMC_ALL_INSTANCE(Instance));

	/* Verify clock frequency is set */
	if (data->sdmmc_clk == 0U) {
		LOG_ERR("SDMMC clock frequency not configured");
		return -EIO;
	}

	/* Calculate clock divider
	 * Formula: ClockDiv = PeripheralClock / (2 * DesiredClock)
	 * This is the STM32 SDMMC clock divider calculation
	 */
	ClockDiv = data->sdmmc_clk / (2U * ClockSpeed);

	/* Modify the CLKCR register to set the clock divider
	 * This is direct LL register manipulation
	 */
	stm32_reg_modify_bits(&Instance->CLKCR, SDMMC_CLKCR_CLKDIV, ClockDiv);

	LOG_DBG("Configured SDMMC clock: freq=%u Hz, div=%u", ClockSpeed, ClockDiv);

	return 0;
}

/**
 * @brief Initialize SDIO peripheral
 *
 * This function initializes the SDMMC peripheral hardware registers.
 * It does NOT perform card initialization or enumeration - that is handled
 * by the Zephyr SD subsystem.
 * @param Instance Pointer to SDMMC register base
 * @param data Pointer to device data struct
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_init(SDMMC_TypeDef *Instance, struct sdhc_stm32_data *data)
{
	SDMMC_InitTypeDef Init;
	uint32_t sdmmc_clk;
	uint32_t init_freq = SDMMC_INIT_FREQ;

	/* Verify clock frequency is set */
	if (data->sdmmc_clk == 0U) {
		LOG_ERR("SDMMC clock frequency not configured");
		return -EINVAL;
	}

	/* Initialize with default values for first-time init */
	Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
	Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	Init.BusWide = SDMMC_BUS_WIDE_1B;
	Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;

	/* Calculate initial clock divider for 400 kHz */
	Init.ClockDiv = data->sdmmc_clk / (2U * init_freq);

	/* Initialize SDMMC peripheral with default configuration */
	if (SDMMC_Init(Instance, Init) != 0) {
		return -EIO;
	}

	/* Set Power State to ON */
	SDMMC_PowerState_ON(Instance);

	/* Wait 74 cycles: required power up time before starting SDIO operations
	 * At 400 kHz, this is ~185 us. Wait 1 ms to be safe.
	 */
	sdmmc_clk = data->sdmmc_clk / (2U * Init.ClockDiv);
	k_msleep(1U + (74U * 1000U / (sdmmc_clk)));

	/* Configure the SDMMC with user parameters from handle */
	Init.ClockEdge = data->Init.ClockEdge;
	Init.ClockPowerSave = data->Init.ClockPowerSave;
	Init.BusWide = data->Init.BusWide;
	Init.HardwareFlowControl = data->Init.HardwareFlowControl;
	Init.ClockDiv = data->Init.ClockDiv;

#if defined(USE_SD_DIRPOL)
	/* Set Transceiver polarity */
	Instance->POWER |= SDMMC_POWER_DIRPOL;
#endif

	/* Apply user configuration to SDMMC peripheral */
	if (SDMMC_Init(Instance, Init) != 0) {
		return -EIO;
	}

	/* Clear error code and set state to ready */
	data->error_code = SDMMC_ERROR_NONE;
	data->Context = SDMMC_CONTEXT_NONE;

	LOG_DBG("SDMMC peripheral initialized successfully");

	return 0;
}

/**
 * @brief Deinitialize SDIO peripheral
 *
 * This function deinitializes the SDMMC peripheral hardware.
 * It powers off the peripheral and resets the state.
 * @param Instance Pointer to SDMMC register base
 * @param data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_deinit(SDMMC_TypeDef *Instance, struct sdhc_stm32_data *data)
{
	/* Check the parameters */
	assert_param(IS_SDMMC_ALL_INSTANCE(Instance));

	/* Set Power State to OFF */
	SDMMC_PowerState_OFF(Instance);

	/* Clear error code and reset state */
	data->error_code = SDMMC_ERROR_NONE;

	LOG_DBG("SDMMC peripheral deinitialized");

	return 0;
}

/**
 * @brief Read direct (CMD52)
 *
 * This function performs a direct read operation using CMD52.
 * It constructs the command argument and sends it to the card.
 * @param Instance Pointer to SDMMC register base
 * @param Argument Direct command argument structure
 * @param pData Pointer to receive data
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_read_direct(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_direct_cmd_t *Argument,
				   uint8_t *pData, struct sdhc_stm32_data *dev_data)
{
	uint32_t cmd;
	uint32_t errorstate;

	/* Check parameters */
	if ((Argument == NULL) || (pData == NULL)) {
		return -EINVAL;
	}

	assert_param(IS_SDIO_RAW_FLAG(Argument->ReadAfterWrite));
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	/* Construct CMD52 argument for read operation
	 * Bit 31: R/W flag (0 = read, 1 = write)
	 * Bits 30-28: Function number
	 * Bit 27: RAW flag (read after write)
	 * Bits 25-9: Register address
	 * Bits 7-0: Data (write) or stuff bits (read)
	 */
	cmd = (0U << 31U); /* Read operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->ReadAfterWrite) << 27U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;
	cmd |= 0U; /* Stuff bits for read */

	/* Send CMD52 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteDirect(Instance, cmd, pData);

	if (errorstate != 0) {
		dev_data->error_code |= errorstate;
		/* Check if it's a critical error */
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			/* Clear all static flags */
			__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
			return -EIO;
		}
	}

	/* Disable command transfer path */
	__SDMMC_CMDTRANS_DISABLE(Instance);

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_DATA_FLAGS);

	return 0;
}

/**
 * @brief Write direct (CMD52)
 *
 * This function performs a direct write operation using CMD52.
 * It constructs the command argument and sends it to the card.
 * @param Instance Pointer to SDMMC register base
 * @param Argument Direct command argument structure
 * @param Data Data to write
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_write_direct(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_direct_cmd_t *Argument,
				    uint8_t Data, struct sdhc_stm32_data *dev_data)
{
	uint32_t cmd;
	uint32_t errorstate;
	uint8_t write_data = Data;

	/* Check parameters */
	if (Argument == NULL) {
		return -EINVAL;
	}

	assert_param(IS_SDIO_RAW_FLAG(Argument->ReadAfterWrite));
	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	/* Construct CMD52 argument for write operation
	 * Bit 31: R/W flag (0 = read, 1 = write)
	 * Bits 30-28: Function number
	 * Bit 27: RAW flag (read after write)
	 * Bits 25-9: Register address
	 * Bits 7-0: Data to write
	 */
	cmd = (1U << 31U); /* Write operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->ReadAfterWrite) << 27U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;
	cmd |= Data;

	/* Send CMD52 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteDirect(Instance, cmd, &write_data);

	if (errorstate != SDMMC_ERROR_NONE) {
		dev_data->error_code |= errorstate;
		/* Check if it's a critical error */
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			/* Clear all static flags */
			__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
			return -EIO;
		}
	}

	/* Disable command transfer path */
	__SDMMC_CMDTRANS_DISABLE(Instance);

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_DATA_FLAGS);

	return 0;
}

/**
 * @brief Read extended (CMD53 polling mode)
 *
 * This function performs an extended read operation using CMD53 in polling mode.
 * It configures the data path, sends CMD53, and polls the FIFO for data.
 * @param Instance Pointer to SDMMC register base
 * @param Argument Extended command argument structure
 * @param pData Pointer to receive buffer
 * @param Size_byte Number of bytes to read
 * @param Timeout_Ms Timeout in milliseconds
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_read_extended(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_ext_cmd_t *Argument,
				     uint8_t *pData, uint32_t Size_byte, uint32_t Timeout_Ms,
				     struct sdhc_stm32_data *dev_data)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t dataremaining;
	uint8_t *tempbuff = pData;
	uint32_t nbr_of_block;
	int ret;

	/* Check parameters */
	if ((Argument == NULL) || (pData == NULL)) {
		return -EINVAL;
	}

	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	/* Configure DPSM */
	sdhc_stm32_ll_config_sdio_dpsm(Instance, Argument, Size_byte, &nbr_of_block, &config, false,
				       dev_data);
	(void)SDMMC_ConfigData(Instance, &config);
	__SDMMC_CMDTRANS_ENABLE(Instance);

	/* Build and send CMD53 */
	cmd = sdhc_stm32_ll_build_cmd53_arg(Argument, nbr_of_block, Size_byte, false);
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(Instance, cmd);
	if (sdhc_stm32_ll_handle_cmd53_error(Instance, errorstate, dev_data)) {
		return -EIO;
	}

	/* Poll on SDMMC flags and read data from FIFO */
	dataremaining = config.DataLength;

	ret = WAIT_FOR(__SDMMC_GET_FLAG(Instance, SDMMC_WAIT_RX_FLAGS), Timeout_Ms * USEC_PER_MSEC,
		       ({
				if (__SDMMC_GET_FLAG(Instance, SDMMC_FLAG_RXFIFOHF) &&
				    (dataremaining >= 32U)) {
					sdhc_stm32_ll_read_fifo_block(Instance, &tempbuff);
					dataremaining -= 32U;
				} else if (dataremaining < 32U) {
					sdhc_stm32_ll_read_remaining_bytes(Instance, &tempbuff,
										&dataremaining);
				}
				k_yield();
		       }));

	if (ret == 0) {
		__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
		dev_data->error_code |= SDMMC_ERROR_TIMEOUT;
		return -ETIMEDOUT;
	}

	__SDMMC_CMDTRANS_DISABLE(Instance);

	/* Check for data transfer errors */
	errorstate = __SDMMC_GET_FLAG(Instance, SDMMC_DATA_ERROR_FLAGS);
	if (errorstate != 0U) {
		return sdhc_stm32_ll_handle_data_error(Instance, errorstate, dev_data);
	}

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_DATA_FLAGS);

	return 0;
}

/**
 * @brief Write 32 bytes to FIFO using word-aligned buffer
 *
 * @param Instance Pointer to SDMMC register base
 * @param u32tempbuff Pointer to word-aligned buffer pointer
 */
static void sdhc_stm32_ll_write_fifo_words(SDMMC_TypeDef *Instance, uint32_t **u32tempbuff)
{
	uint32_t *buf = *u32tempbuff;

	for (uint32_t regCount = 0U; regCount < (SDMMC_FIFO_SIZE / sizeof(uint32_t)); regCount++) {
		Instance->FIFO = *buf++;
	}
	*u32tempbuff = buf;
}

/**
 * @brief Write extended (CMD53 polling mode)
 *
 * This function performs an extended write operation using CMD53 in polling mode.
 * It configures the data path, sends CMD53, and polls the FIFO to write data.
 * @param Instance Pointer to SDMMC register base
 * @param Argument Extended command argument structure
 * @param pData Pointer to transmit buffer
 * @param Size_byte Number of bytes to write
 * @param Timeout_Ms Timeout in milliseconds
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_write_extended(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_ext_cmd_t *Argument,
				      uint8_t *pData, uint32_t Size_byte, uint32_t Timeout_Ms,
				      struct sdhc_stm32_data *dev_data)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t dataremaining;
	uint32_t *u32tempbuff = (uint32_t *)pData;
	uint32_t nbr_of_block;
	int ret;

	/* Check parameters */
	if ((Argument == NULL) || (pData == NULL)) {
		return -EINVAL;
	}

	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	/* Configure DPSM */
	sdhc_stm32_ll_config_sdio_dpsm(Instance, Argument, Size_byte, &nbr_of_block, &config, true,
				       dev_data);
	(void)SDMMC_ConfigData(Instance, &config);
	__SDMMC_CMDTRANS_ENABLE(Instance);

	/* Build and send CMD53 */
	cmd = sdhc_stm32_ll_build_cmd53_arg(Argument, nbr_of_block, Size_byte, true);
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(Instance, cmd);
	if (sdhc_stm32_ll_handle_cmd53_error(Instance, errorstate, dev_data)) {
		return -EIO;
	}

	/* Poll on SDMMC flags and write data to FIFO */
	dataremaining = config.DataLength;

	ret = WAIT_FOR(
		__SDMMC_GET_FLAG(Instance, SDMMC_WAIT_TX_FLAGS), Timeout_Ms * USEC_PER_MSEC, ({
			if (__SDMMC_GET_FLAG(Instance, SDMMC_FLAG_TXFIFOHE) &&
			    (dataremaining >= 32U)) {
				sdhc_stm32_ll_write_fifo_words(Instance, &u32tempbuff);
				dataremaining -= 32U;
			} else if ((dataremaining < 32U) &&
				   (__SDMMC_GET_FLAG(Instance,
						     SDMMC_FLAG_TXFIFOHE | SDMMC_FLAG_TXFIFOE))) {
				uint8_t *u8buff = (uint8_t *)u32tempbuff;

				sdhc_stm32_ll_write_remaining_bytes(Instance, &u8buff,
								    &dataremaining);
			}
			k_yield();
		}));

	if (ret == 0) {
		__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
		dev_data->error_code |= SDMMC_ERROR_TIMEOUT;
		return -ETIMEDOUT;
	}

	__SDMMC_CMDTRANS_DISABLE(Instance);

	/* Check for data transfer errors */
	errorstate = __SDMMC_GET_FLAG(Instance, SDMMC_DATA_ERROR_FLAGS);
	if (errorstate != 0U) {
		return sdhc_stm32_ll_handle_data_error(Instance, errorstate, dev_data);
	}

	/* Clear all static data flags */
	__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_DATA_FLAGS);

	return 0;
}

/**
 * @brief Read extended DMA (CMD53 DMA mode)

 * This function performs an extended read operation using CMD53 in DMA mode.
 * It configures the data path and DMA, sends CMD53, and enables interrupts.
 * The actual data transfer completion is handled by the interrupt handler.
 * @param Instance Pointer to SDMMC register base
 * @param Argument Extended command argument structure
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_read_extended_dma(SDMMC_TypeDef *Instance,
					 sdhc_stm32_sdio_ext_cmd_t *Argument,
					 struct sdhc_stm32_data *dev_data)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t cmd;
	uint32_t nbr_of_block;

	/* Check parameters */
	if ((Argument == NULL) || (dev_data->sdio_dma_buf == NULL)) {
		return -EINVAL;
	}

	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	dev_data->is_sdio_transfer = true;

	/* Initialize data control register */
	if ((Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		Instance->DCTRL = 0U;
	}

	/* Compute number of blocks to receive */
	nbr_of_block = (dev_data->total_transfer_bytes & ~(dev_data->block_size & 1U)) >>
		       __CLZ(__RBIT(dev_data->block_size));

	/* Configure DMA (use single buffer mode) */
	Instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;
	Instance->IDMABASE0 = (uint32_t)dev_data->sdio_dma_buf;

	/* Configure SDIO Data Path State Machine (DPSM) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		config.DataLength = (uint32_t)(nbr_of_block * dev_data->block_size);
		config.DataBlockSize = sdhc_stm32_ll_convert_block_size(dev_data->block_size);
	} else {
		config.DataLength = (dev_data->total_transfer_bytes > 0U)
					    ? dev_data->total_transfer_bytes
					    : 512U;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_1B;
	}

	config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode = (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK)
				      ? SDMMC_TRANSFER_MODE_BLOCK
				      : SDMMC_TRANSFER_MODE_SDIO;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(Instance, &config);

	__SDMMC_CMDTRANS_ENABLE(Instance);

	/* Construct CMD53 argument for read operation */
	cmd = (0U << 31U); /* Read operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->Block_Mode) << 27U;
	cmd |= ((uint32_t)Argument->OpCode) << 26U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;
	cmd |= ((nbr_of_block == 0U) ? dev_data->total_transfer_bytes : nbr_of_block) & 0x1FFU;

	/* Send CMD53 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(Instance, cmd);
	if (errorstate != SDMMC_ERROR_NONE) {
		dev_data->error_code |= errorstate;
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			stm32_reg_modify_bits(&Instance->DCTRL, SDMMC_DCTRL_FIFORST,
					      SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_DATA_FLAGS);
			return -EIO;
		}
	}

	/* Enable interrupts for DMA transfer */
	__SDMMC_ENABLE_IT(Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_RXOVERR |
				     SDMMC_IT_DATAEND));

	return 0;
}

/**
 * @brief Write extended DMA (CMD53 DMA mode)
 *
 * This function performs an extended write operation using CMD53 in DMA mode.
 * It configures the data path and DMA, sends CMD53, and enables interrupts.
 * The actual data transfer completion is handled by the interrupt handler.
 * @param Instance Pointer to SDMMC register base
 * @param Argument Extended command argument structure
 * @param dev_data Pointer to the STM32 SDHC data structure
 * @return int 0 on success, negative errno on failure
 */
int sdhc_stm32_ll_sdio_write_extended_dma(SDMMC_TypeDef *Instance,
					  sdhc_stm32_sdio_ext_cmd_t *Argument,
					  struct sdhc_stm32_data *dev_data)
{
	uint32_t cmd;
	SDMMC_DataInitTypeDef config;
	uint32_t count;
	uint32_t errorstate;
	uint32_t nbr_of_block;

	/* Check parameters */
	if ((Argument == NULL) || (dev_data->sdio_dma_buf == NULL)) {
		return -EINVAL;
	}

	assert_param(IS_SDIO_FUNCTION(Argument->IOFunctionNbr));

	/* Set state to busy */
	dev_data->error_code = SDMMC_ERROR_NONE;

	dev_data->is_sdio_transfer = true;

	/* Initialize data control register */
	if ((Instance->DCTRL & SDMMC_DCTRL_SDIOEN) != 0U) {
		Instance->DCTRL = SDMMC_DCTRL_SDIOEN;
	} else {
		Instance->DCTRL = 0U;
	}

	/* Compute number of blocks to send */
	nbr_of_block = (dev_data->total_transfer_bytes & ~(dev_data->block_size & 1U)) >>
		       __CLZ(__RBIT(dev_data->block_size));

	/* Configure DMA (use single buffer mode) */
	Instance->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;
	Instance->IDMABASE0 = (uint32_t)dev_data->sdio_dma_buf;

	/* Configure SDIO Data Path State Machine (DPSM) */
	config.DataTimeOut = SDMMC_DATATIMEOUT;

	if (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK) {
		config.DataLength = (uint32_t)(nbr_of_block * dev_data->block_size);
		config.DataBlockSize = sdhc_stm32_ll_convert_block_size(dev_data->block_size);
	} else {
		config.DataLength = (dev_data->total_transfer_bytes > 512U)
					    ? 512U
					    : dev_data->total_transfer_bytes;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_1B;
	}

	config.TransferDir = SDMMC_TRANSFER_DIR_TO_CARD;
	config.TransferMode = (Argument->Block_Mode == SDMMC_SDIO_MODE_BLOCK)
				      ? SDMMC_TRANSFER_MODE_BLOCK
				      : SDMMC_TRANSFER_MODE_SDIO;
	config.DPSM = SDMMC_DPSM_DISABLE;
	(void)SDMMC_ConfigData(Instance, &config);

	__SDMMC_CMDTRANS_ENABLE(Instance);

	/* Construct CMD53 argument for write operation */
	cmd = (1U << 31U); /* Write operation */
	cmd |= ((uint32_t)Argument->IOFunctionNbr) << 28U;
	cmd |= ((uint32_t)Argument->Block_Mode) << 27U;
	cmd |= ((uint32_t)Argument->OpCode) << 26U;
	cmd |= (Argument->Reg_Addr & 0x1FFFFU) << 9U;

	if (nbr_of_block == 0U) {
		count = (dev_data->total_transfer_bytes > 512U) ? 512U
								: dev_data->total_transfer_bytes;
	} else {
		count = nbr_of_block;
	}

	cmd |= (count & 0x1FFU);

	/* Send CMD53 using LL function */
	errorstate = SDMMC_SDIO_CmdReadWriteExtended(Instance, cmd);
	if (errorstate != SDMMC_ERROR_NONE) {
		dev_data->error_code |= errorstate;
		if (errorstate != (SDMMC_ERROR_ADDR_OUT_OF_RANGE | SDMMC_ERROR_ILLEGAL_CMD |
				   SDMMC_ERROR_COM_CRC_FAILED | SDMMC_ERROR_GENERAL_UNKNOWN_ERR)) {
			stm32_reg_modify_bits(&Instance->DCTRL, SDMMC_DCTRL_FIFORST,
					      SDMMC_DCTRL_FIFORST);
			__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_FLAGS);
			__SDMMC_CLEAR_FLAG(Instance, SDMMC_STATIC_DATA_FLAGS);
			return -EIO;
		}
	}

	/* Enable interrupts for DMA transfer */
	__SDMMC_ENABLE_IT(Instance, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
				     SDMMC_IT_DATAEND));

	return 0;
}
