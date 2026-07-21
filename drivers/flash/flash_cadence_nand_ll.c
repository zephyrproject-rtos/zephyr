/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "flash_cadence_nand_ll.h"

LOG_MODULE_REGISTER(flash_cdns_nand_ll, CONFIG_FLASH_LOG_LEVEL);

/**
 * Wait for the Cadence NAND controller to become idle.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @retval  0 on success or -ETIMEDOUT error value on failure.
 */
static inline int32_t cdns_nand_wait_idle(uintptr_t base_address)
{
	/* Wait status command response ready */
	if (!WAIT_FOR(CNF_GET_CTRL_BUSY(sys_read32(CNF_CMDREG(base_address, CTRL_STATUS))) == 0U,
		      IDLE_TIME_OUT, k_msleep(1))) {
		LOG_ERR("Timed out waiting for wait idle response");
		return -ETIMEDOUT;
	}
	return 0;
}

/**
 * Set the row address for a NAND flash memory device using the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param local_row_address The row address.
 * @param page_set The page set number.
 */
static void row_address_set(struct cadence_nand_params *params, uint32_t *local_row_address,
			    uint32_t page_set)
{
	uint32_t block_number = 0;

	block_number = ((page_set) / (params->npages_per_block));
	*local_row_address = 0;
	*local_row_address |= ROW_VAL_SET((params->page_size_bit) - 1, 0,
					  ((page_set) % (params->npages_per_block)));
	*local_row_address |=
		ROW_VAL_SET((params->block_size_bit) - 1, (params->page_size_bit), block_number);
	*local_row_address |= ROW_VAL_SET((params->lun_size_bit) - 1, (params->block_size_bit),
					  (block_number / params->nblocks_per_lun));
}

/**
 * Retrieve information about the NAND flash device using the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @retval  0 on success or -ENXIO error value on failure.
 */
static int cdns_nand_device_info(struct cadence_nand_params *params)
{
	struct nf_ctrl_version *nf_ver;
	uintptr_t base_address;
	uint32_t reg_value = 0;
	uint8_t type;

	base_address = params->nand_base;

	/* Read flash device version information */
	reg_value = sys_read32(CNF_CTRLPARAM(base_address, VERSION));
	nf_ver = (struct nf_ctrl_version *)&reg_value;

	LOG_INF("NAND Flash Version Information");
	LOG_INF("HPNFC Magic Number 0x%x", nf_ver->hpnfc_magic_number);
	LOG_INF("Fixed number 0x%x", nf_ver->ctrl_fix);
	LOG_INF("Controller Revision Number 0x%x", nf_ver->ctrl_rev);

	/* Interface Type */
	reg_value = sys_read32(CNF_CTRLPARAM(base_address, DEV_PARAMS0));
	type = CNF_GET_DEV_TYPE(reg_value);
	if (type == CNF_DT_UNKNOWN) {
		LOG_ERR("%s: device type unknown", __func__);
		return -ENXIO;
	}

	params->nluns = CNF_GET_NLUNS(reg_value);
	LOG_INF("Number of LUMs %hhx", params->nluns);

	/* Pages per block */
	reg_value = sys_read32(CNF_CTRLCFG(base_address, DEV_LAYOUT));
	params->npages_per_block = GET_PAGES_PER_BLOCK(reg_value);

	/* Page size and spare size */
	reg_value = sys_read32(CNF_CTRLPARAM(base_address, DEV_AREA));
	params->page_size = GET_PAGE_SIZE(reg_value);
	params->spare_size = GET_SPARE_SIZE(reg_value);

	/* Device blocks per LUN */
	params->nblocks_per_lun = sys_read32(CNF_CTRLPARAM(base_address, DEV_BLOCKS_PLUN));

	/* Calculate block size and total device size */
	params->block_size = (params->npages_per_block * params->page_size);
	params->device_size = ((long long)params->block_size *
			       (long long)(params->nblocks_per_lun * params->nluns));
	LOG_INF("block size %x total device size %llx", params->block_size, params->device_size);

	/* Calculate bit size of page, block and lun*/
	params->page_size_bit = find_msb_set((params->npages_per_block) - 1);
	params->block_size_bit = find_msb_set((params->nblocks_per_lun) - 1);
	params->lun_size_bit = find_msb_set((params->nluns) - 1);
	return 0;
}

/**
 * Retrieve the status of a specific thread in the Cadence NAND controller.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @param thread The thread identifier.
 * @retval  The status of the thread.
 */
static uint32_t cdns_nand_get_thrd_status(uintptr_t base_address, uint8_t thread)
{
	uint32_t status;

	sys_write32(THREAD_VAL(thread), (base_address + CMD_STATUS_PTR_ADDR));
	status = sys_read32((base_address + CMD_STAT_CMD_STATUS));
	return status;
}

/**
 * Wait for a specific thread in the Cadence controller to complete.
 *
 * @param base_address The base address of the Cadence controller.
 * @param thread The thread identifier to wait for.
 * @retval  0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_wait_for_thread(uintptr_t base_address, uint8_t thread)
{

	if (!WAIT_FOR((sys_read32((base_address) + THR_STATUS) & BIT(thread)) == 0U,
		      THREAD_IDLE_TIME_OUT, k_msleep(1))) {
		LOG_ERR("Timed out waiting for thread response");
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 * Set features in the Cadence NAND controller using PIO operations.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @param feat_addr The address of the feature to be set.
 * @param feat_val The value of the feature to be set.
 * @param thread The thread identifier for the PIO operation.
 * @param vol_id The volume identifier for the feature set operation.
 * @param use_intr Flag indicating whether to use interrupts during the operation.
 * @retval  0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_nand_pio_set_features(uintptr_t base_address, uint8_t feat_addr, uint8_t feat_val,
				      uint8_t thread, uint8_t vol_id)
{
	uint32_t status = 0;
	int ret = 0;

	ret = cdns_wait_for_thread(base_address, thread);

	if (ret != 0) {
		return ret;
	}

	sys_write32(SET_FEAT_ADDR(feat_addr), (base_address + CDNS_CMD_REG1));
	sys_write32(feat_val, (base_address + CDNS_CMD_REG2));
	status = CMD_0_THREAD_POS_SET(thread);
	status |= CMD_0_C_MODE_SET(CT_PIO_MODE);
	status |= PIO_CMD0_CT_SET(PIO_SET_FEA_MODE);
	status |= CMD_0_VOL_ID_SET(vol_id);
	sys_write32(status, (base_address + CDNS_CMD_REG0));
	return 0;
}

/**
 * Check whether a transfer complete for PIO operation in the Cadence controller has finished.
 *
 * @param base_address The base address of the Cadence controller.
 * @param thread The thread identifier for the PIO operation.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_pio_transfer_complete(uintptr_t base_address, uint8_t thread)
{
	uint32_t status;

	status = WAIT_FOR(((cdns_nand_get_thrd_status(base_address, thread)) != 0), IDLE_TIME_OUT,
			  k_msleep(1));

	if (status == 0) {
		LOG_ERR("Timed out waiting for thread status response");
		return -ETIMEDOUT;
	}

	if ((status & (BIT(F_CSTAT_COMP)))) {
		if ((status & (BIT(F_CSTAT_FAIL)))) {
			LOG_ERR("Cadence status operation failed %s", __func__);
			return -EIO;
		}
	} else {
		LOG_ERR("Cadence status complete failed %s", __func__);
		return -EIO;
	}
	return 0;
}

/**
 * Set the operational mode for the Cadence NAND controller.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @param opr_mode The operational mode  SDR / NVDDR to set.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_nand_set_opr_mode(uintptr_t base_address, uint8_t opr_mode)
{
	uint8_t device_type;
	uint32_t timing_mode = 0;
	uint32_t status;
	int ret;

	if (opr_mode == CNF_OPR_WORK_MODE_SDR) {

		status = ONFI_TIMING_MODE_SDR(
			sys_read32(CNF_CTRLPARAM(base_address, ONFI_TIMING_0)));
		timing_mode = find_lsb_set(status) - 1;

		/* PHY Register Timing setting*/
		sys_write32(PHY_CTRL_REG_SDR, (base_address + PHY_CTRL_REG_OFFSET));
		sys_write32(PHY_TSEL_REG_SDR, (base_address + PHY_TSEL_REG_OFFSET));
		sys_write32(PHY_DQ_TIMING_REG_SDR, (base_address + PHY_DQ_TIMING_REG_OFFSET));
		sys_write32(PHY_DQS_TIMING_REG_SDR, (base_address + PHY_DQS_TIMING_REG_OFFSET));
		sys_write32(PHY_GATE_LPBK_CTRL_REG_SDR, (base_address + PHY_GATE_LPBK_OFFSET));
		sys_write32(PHY_DLL_MASTER_CTRL_REG_SDR, (base_address + PHY_DLL_MASTER_OFFSET));

		/* Async mode timing settings */
		sys_write32((CNF_ASYNC_TIMINGS_TRH) | (CNF_ASYNC_TIMINGS_TRP) |
				    (CNF_ASYNC_TIMINGS_TWH) | (CNF_ASYNC_TIMINGS_TWP),
			    CNF_MINICTRL(base_address, ASYNC_TOGGLE_TIMINGS));

		/* Set operation work mode in common settings */
		sys_clear_bits(CNF_MINICTRL(base_address, CMN_SETTINGS),
			       CNF_OPR_WORK_MODE_SDR_MASK);

	} else {
		/* NVDDR MODE */
		status = ONFI_TIMING_MODE_NVDDR(
			sys_read32(CNF_CTRLPARAM(base_address, ONFI_TIMING_0)));
		timing_mode = find_lsb_set(status) - 1;
		/* PHY Register Timing setting*/
		sys_write32(PHY_CTRL_REG_DDR, (base_address + PHY_CTRL_REG_OFFSET));
		sys_write32(PHY_TSEL_REG_DDR, (base_address + PHY_TSEL_REG_OFFSET));
		sys_write32(PHY_DQ_TIMING_REG_DDR, (base_address + PHY_DQ_TIMING_REG_OFFSET));
		sys_write32(PHY_DQS_TIMING_REG_DDR, (base_address + PHY_DQS_TIMING_REG_OFFSET));
		sys_write32(PHY_GATE_LPBK_CTRL_REG_DDR, (base_address + PHY_GATE_LPBK_OFFSET));
		sys_write32(PHY_DLL_MASTER_CTRL_REG_DDR, (base_address + PHY_DLL_MASTER_OFFSET));
		/* Set operation work mode in common settings */
		sys_set_bits(CNF_MINICTRL(base_address, CMN_SETTINGS),
			     CNF_OPR_WORK_MODE_NVDDR_MASK);
	}

	/* Wait for controller to be in idle state */
	ret = cdns_nand_wait_idle(base_address);
	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}

	/* Check device type */
	device_type = CNF_GET_DEV_TYPE(sys_read32(CNF_CTRLPARAM(base_address, DEV_PARAMS0)));

	if (device_type != ONFI_INTERFACE) {
		LOG_ERR("Driver does not support this interface");
		return -ENOTSUP;
	}
	/* Reset DLL PHY */
	sys_clear_bit(CNF_MINICTRL(base_address, DLL_PHY_CTRL), CNF_DLL_PHY_RST_N);

	/* Wait for controller to be in idle state */
	ret = cdns_nand_wait_idle(base_address);
	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}

	ret = cdns_nand_pio_set_features(base_address, SET_FEAT_TIMING_MODE_ADDRESS, timing_mode,
					 NF_TDEF_TRD_NUM, VOL_ID);
	if (ret != 0) {
		return ret;
	}

	ret = cdns_pio_transfer_complete(base_address, NF_TDEF_TRD_NUM);
	if (ret != 0) {
		LOG_ERR("cdns pio check failed");
		return ret;
	}

	ret = cdns_nand_wait_idle(base_address);
	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}

	/* set dll_rst_n in dll_phy_ctrl to 1 */
	sys_set_bit(CNF_MINICTRL(base_address, DLL_PHY_CTRL), CNF_DLL_PHY_RST_N);

	ret = cdns_nand_wait_idle(base_address);
	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}

	return 0;
}

/**
 * Configure the transfer settings of the Cadence NAND controller.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @retval  0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_nand_transfer_config(uintptr_t base_address)
{
	int ret = 0;
	/* Wait for controller to be in idle state */
	ret = cdns_nand_wait_idle(base_address);

	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}

	/* Configure data transfer parameters */
	sys_write32(ENABLE, CNF_CTRLCFG(base_address, TRANS_CFG0));

	/* Disable cache and multiplane. */
	sys_write32(DISABLE, CNF_CTRLCFG(base_address, MULTIPLANE_CFG));
	sys_write32(DISABLE, CNF_CTRLCFG(base_address, CACHE_CFG));

	/* Clear all interrupts. */
	sys_write32(CLEAR_ALL_INTERRUPT, (base_address + INTR_STATUS));
	return 0;
}

/**
 * Initialize the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @retval  0 on success or negative error value on failure.
 */
int cdns_nand_init(struct cadence_nand_params *params)
{
	uint32_t reg_value_read = 0;
	uintptr_t base_address = params->nand_base;
	uint8_t datarate_mode = params->datarate_mode;
	int ret;

	if (!WAIT_FOR(CNF_GET_INIT_COMP(sys_read32(CNF_CMDREG(base_address, CTRL_STATUS))) != 0U,
		      IDLE_TIME_OUT, k_msleep(1))) {
		LOG_ERR("Timed out waiting for NAND Controller Init complete status response");
		return -ETIMEDOUT;
	}

	if (CNF_GET_INIT_FAIL(sys_read32(CNF_CMDREG(base_address, CTRL_STATUS))) != 0) {
		LOG_ERR("NAND Controller Init complete Failed!!!");
		return -ENODEV;
	}

	ret = cdns_nand_device_info(params);
	if (ret != 0) {
		return ret;
	}

	/* Hardware Support Features */
	reg_value_read = sys_read32(CNF_CTRLPARAM(base_address, FEATURE));
	/* Enable data integrity parity check if the data integrity parity mechanism is */
	/* supported by the device */
	if (CNF_HW_DI_PR_SUPPORT(reg_value_read) != 0) {
		sys_set_bit(CNF_DI(base_address, CONTROL), CNF_DI_PAR_EN);
	}

	/* Enable data integrity CRC check if the data integrity CRC mechanism is */
	/* supported by the device */
	if (CNF_HW_DI_CRC_SUPPORT(reg_value_read) != 0) {
		sys_set_bit(CNF_DI(base_address, CONTROL), CNF_DI_CRC_EN);
	}
	/* Status polling mode, device control and status register */
	ret = cdns_nand_wait_idle(base_address);

	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}
	sys_write32(DEV_STAT_DEF_VALUE, CNF_CTRLCFG(base_address, DEV_STAT));

	/* Set operation work mode */
	ret = cdns_nand_set_opr_mode(base_address, datarate_mode);
	if (ret != 0) {
		return ret;
	}

	/* Set data transfer configuration parameters */
	ret = cdns_nand_transfer_config(base_address);
	if (ret != 0) {
		return ret;
	}

	/* Wait for controller to be in idle state */
	ret = cdns_nand_wait_idle(base_address);
	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}

	/* DMA Setting */
	sys_write32((F_BURST_SEL_SET(NF_TDEF_BURST_SEL)) | (BIT(F_OTE)),
		    (base_address + NF_DMA_SETTING));

	/* Pre fetch */
	sys_write32(((NF_FIFO_TRIGG_LVL_SET(PRE_FETCH_VALUE)) |
		     (NF_DMA_PACKAGE_SIZE_SET(PRE_FETCH_VALUE))),
		    (base_address + NF_PRE_FETCH));
	/* Total bits in row addressing*/
	params->total_bit_row = find_msb_set(((params->npages_per_block) - 1)) +
				find_msb_set((params->nblocks_per_lun) - 1);

	if (ret != 0) {
		LOG_ERR("Failed to establish device access width!");
		return -EINVAL;
	}
	/* Enable Global Interrupt for NAND*/
#ifdef CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
	sys_set_bit((base_address + INTERRUPT_STATUS_REG), GINTR_ENABLE);
#endif
	return 0;
}

#if CONFIG_CDNS_NAND_CDMA_MODE
/**
 *
 * This function performs Command descriptor structure prepareation.
 *
 * @param nf_mem determine which NF memory bank will be selected
 * @param flash_ptr start ROW address in NF memory
 * @param mem_ptr system memory pointer
 * @param ctype Command type (read/write/erase)
 * @param cmd_cnt counter for commands
 * @param dma_sel select DMA engine (0 - slave DMA, 1 - master DMA)
 * @param vol_id specify target volume ID
 *
 */
void cdns_nand_cdma_prepare(char nf_mem, uint32_t flash_ptr, char *mem_ptr, uint16_t ctype,
			    int32_t cmd_cnt, uint8_t dma_sel, uint8_t vol_id,
			    struct cdns_cdma_command_descriptor *desc)
{
	struct cdns_cdma_command_descriptor *cdma_desc;

	cdma_desc = desc;
	/* set fields for one descriptor */
	cdma_desc->flash_pointer = flash_ptr;
	cdma_desc->bank_number = nf_mem;
	cdma_desc->command_flags |= CDMA_CF_DMA_MASTER_SET(dma_sel) | F_CFLAGS_VOL_ID_SET(vol_id);
	cdma_desc->memory_pointer = (uintptr_t)mem_ptr;
	cdma_desc->status = 0;
	cdma_desc->sync_flag_pointer = 0;
	cdma_desc->sync_arguments = 0;
	cdma_desc->ctrl_data_ptr = 0x40;
	cdma_desc->command_type = ctype;
	if (cmd_cnt > 1) {
		cdma_desc->next_pointer = (uintptr_t)(desc + 1);
		cdma_desc->command_flags |= CFLAGS_MPTRPC_SET | CFLAGS_MPTRPC_SET;
		cdma_desc->command_flags |= CFLAGS_CONT_SET;
	} else {
		cdma_desc->next_pointer = 0;
#ifdef CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
		cdma_desc->command_flags |= CDMA_CF_INT_SET;
#endif
	}
}

/**
 * Check a command descriptor transfer complete status in the Cadence NAND controller.
 *
 * @param desc_ptr The pointer to the command descriptor structure.
 * @param params The Cadence NAND parameters structure.
 * @retval 0 on success or negative error value on failure.
 */
static int cdns_transfer_complete(struct cdns_cdma_command_descriptor *desc_ptr,
				  struct cadence_nand_params *params)
{
#ifdef CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
	uint32_t status = 0;

	NAND_INT_SEM_TAKE(params);
	sys_write32(NF_TDEF_TRD_NUM, (params->nand_base + CMD_STATUS_PTR_ADDR));
	status = sys_read32((params->nand_base + CMD_STAT_CMD_STATUS));
	if ((status & (BIT(F_CSTAT_COMP)))) {
		if ((status & (BIT(F_CSTAT_FAIL)))) {
			LOG_ERR("Cadence status operation failed %s", __func__);
			return -EIO;
		}
	} else {
		LOG_ERR("Cadence status complete failed %s", __func__);
		return -EIO;
	}
#else
	ARG_UNUSED(params);

	if (!WAIT_FOR(((desc_ptr->status & (BIT(F_CSTAT_COMP))) != 0), IDLE_TIME_OUT,
		      k_msleep(1))) {
		LOG_ERR("Timed out waiting for thread status response");
		return -ETIMEDOUT;
	}
	if ((desc_ptr->status & (BIT(F_CSTAT_FAIL))) != 0) {
		LOG_ERR("Cadence status operation failed %s", __func__);
		return -EIO;
	}
#endif
	return 0;
}

/**
 * Send a command descriptor to the Cadence NAND controller for execution.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @param desc_ptr The pointer to the command descriptor.
 * @param thread The thread number for the execution.
 * @retval 0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_nand_send(uintptr_t base_address, char *desc_ptr, uint8_t thread)
{
	uint64_t desc_address;
	uint32_t status;
	int ret;

	desc_address = (uint64_t)desc_ptr;

	ret = cdns_wait_for_thread(base_address, thread);

	if (ret != 0) {
		return ret;
	}
	/* desc_ptr address passing */
	sys_write32(desc_address & U32_MASK_VAL, (base_address + CDNS_CMD_REG2));
	sys_write32((desc_address >> 32) & U32_MASK_VAL, (base_address + CDNS_CMD_REG3));
	/* Thread selection */
	status = CMD_0_THREAD_POS_SET(thread);
	/* CDMA Mode selection */
	status |= CMD_0_C_MODE_SET(CT_CDMA_MODE);
	/* CMD 0 Reg write*/
	sys_write32(status, (base_address + CDNS_CMD_REG0));
	return 0;
}

static int cdns_cdma_desc_transfer_finish(struct cadence_nand_params *params, uint32_t page_count,
					  uint32_t max_page_desc, uint32_t ctype,
					  uint32_t cond_start, char *buffer)
{
	uint32_t page_count_pass = 0;
	uint32_t row_address = 0;
	uint32_t base_address;
	uint32_t page_buffer_size;
	struct cdns_cdma_command_descriptor *cdma_desc;
	int ret;

	page_buffer_size = (page_count > max_page_desc) ? max_page_desc : page_count;

	cdma_desc = k_malloc(sizeof(struct cdns_cdma_command_descriptor) * page_buffer_size);

	if (cdma_desc == NULL) {
		LOG_ERR("Memory allocation error occurred %s", __func__);
		return -ENOSR;
	}

	base_address = params->nand_base;

	while (page_count > 0) {
		row_address_set(params, &row_address, cond_start);

		if (page_count > max_page_desc) {
			page_count_pass = max_page_desc;
			page_count = page_count - max_page_desc;
			cond_start = cond_start + page_count_pass;
		} else {
			page_count_pass = page_count;
			page_count = page_count - page_count_pass;
		}
		for (int index = 0; index < page_count_pass; index++) {
			cdns_nand_cdma_prepare(NF_TDEF_DEV_NUM, row_address, buffer,
					       (ctype + index), (page_count_pass - index),
					       DMA_MS_SEL, VOL_ID, (cdma_desc + index));
		}
		ret = cdns_nand_send(base_address, (char *)cdma_desc, NF_TDEF_TRD_NUM);

		if (ret != 0) {
			k_free(cdma_desc);
			return ret;
		}

		if (ctype != CNF_CMD_ERASE) {
			buffer = buffer + (max_page_desc * params->page_size);
		}

		ret = cdns_transfer_complete(cdma_desc, params);

		if (ret != 0) {
			k_free(cdma_desc);
			return ret;
		}
	}

	k_free(cdma_desc);

	return 0;
}
/**
 * Perform a CDMA write operation for the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param start_page_number The starting page number for the write operation.
 * @param buffer The buffer containing the data to be written.
 * @param page_count The number of pages to be written.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_nand_cdma_write(struct cadence_nand_params *params, uint32_t start_page_number,
				char *buffer, uint32_t page_count)
{
	int ret;

	ret = cdns_cdma_desc_transfer_finish(params, page_count, CONFIG_FLASH_CDNS_CDMA_PAGE_COUNT,
					     CNF_CMD_WR, start_page_number, buffer);

	return ret;
}

/**
 * Perform a CDMA read operation for the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param start_page_number The starting page number for the read operation.
 * @param buffer The buffer to store the read data.
 * @param page_count The number of pages to be read.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_nand_cdma_read(struct cadence_nand_params *params, uint32_t start_page_number,
			       char *buffer, uint32_t page_count)
{
	int ret;

	ret = cdns_cdma_desc_transfer_finish(params, page_count, CONFIG_FLASH_CDNS_CDMA_PAGE_COUNT,
					     CNF_CMD_RD, start_page_number, buffer);

	return ret;
}

/**
 * Perform a CDMA erase operation for the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param start_block_number The starting block number for the erase operation.
 * @param block_count The number of blocks to be erased.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_nand_cdma_erase(struct cadence_nand_params *params, uint32_t start_block_number,
				uint32_t block_count)
{
	int ret;

	ret = cdns_cdma_desc_transfer_finish(params, block_count,
					     CONFIG_FLASH_CDNS_CDMA_BLOCK_COUNT, CNF_CMD_ERASE,
					     start_block_number, NULL);

	return ret;
}
#endif

#if CONFIG_CDNS_NAND_PIO_MODE

/**
 * Perform an erase operation on the Cadence NAND controller using PIO.
 *
 * @param params The Cadence NAND parameters structure.
 * @param thread The thread identifier for the PIO operation.
 * @param bank The bank identifier for the erase operation.
 * @param start_block The starting block number for the erase operation.
 * @param ctype The command type for the erase operation.
 * @param block_count The number of blocks to be erased.
 * @retval   0 on success or negative error value on failure.
 */
static int cdns_nand_pio_erase(struct cadence_nand_params *params, uint8_t thread, uint8_t bank,
			       uint32_t start_block, uint16_t ctype, uint32_t block_count)
{
	uint32_t status;
	uintptr_t base_address;
	uint32_t row_address = 0;
	uint32_t index = 0;
	int ret;

	base_address = params->nand_base;
	for (index = 0; index < block_count; index++) {

		ret = cdns_wait_for_thread(base_address, thread);

		if (ret != 0) {
			return ret;
		}
		row_address_set(params, &row_address, (start_block * params->npages_per_block));
		sys_write32(row_address, (base_address + CDNS_CMD_REG1));
		start_block++;
		sys_write32((NF_CMD4_BANK_SET(bank)), (base_address + CDNS_CMD_REG4));
		status = CMD_0_THREAD_POS_SET(thread);
#ifdef CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
		status |= BIT(PIO_CF_INT);
#endif
		status |= CMD_0_C_MODE_SET(CT_PIO_MODE);
		status |= PIO_CMD0_CT_SET(ctype);
		sys_write32(status, (base_address + CDNS_CMD_REG0));
		NAND_INT_SEM_TAKE(params);
		ret = cdns_pio_transfer_complete(base_address, thread);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/**
 * Prepare for a PIO operation in the Cadence NAND controller.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @param thread The thread ID associated with the operation.
 * @param bank The bank ID for the operation.
 * @param row_address The row address for the operation.
 * @param buf The buffer containing the data for the operation.
 * @param ctype The command type for the operation.
 * @param dma_sel The DMA selection flag for the operation.
 * @param vol_id The volume ID for the operation.
 * @retval 0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_nand_pio_prepare(uintptr_t base_address, uint8_t thread, uint8_t bank,
				 uint32_t row_address, char *buf, uint16_t ctype, uint8_t dma_sel,
				 uint8_t vol_id)
{
	uint64_t buf_addr = (uintptr_t)buf;
	uint32_t status;
	int ret;

	ret = cdns_wait_for_thread(base_address, thread);

	if (ret != 0) {
		return ret;
	}

	sys_write32(row_address, (base_address + CDNS_CMD_REG1));
	sys_write32(NF_CMD4_BANK_SET(bank), (base_address + CDNS_CMD_REG4));
	sys_write32(buf_addr & U32_MASK_VAL, (base_address + CDNS_CMD_REG2));
	sys_write32((buf_addr >> 32) & U32_MASK_VAL, (base_address + CDNS_CMD_REG3));
	status = CMD_0_THREAD_POS_SET(thread);
#ifdef CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
	status |= PIO_CF_INT_SET;
#endif
	status |= PIO_CF_DMA_MASTER_SET(dma_sel);
	status |= CMD_0_C_MODE_SET(CT_PIO_MODE);
	status |= PIO_CMD0_CT_SET(ctype);
	status |= CMD_0_VOL_ID_SET(vol_id);
	sys_write32(status, (base_address + CDNS_CMD_REG0));
	return 0;
}

/**
 * Perform a PIO write operation for the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param row_address The row address for the write operation.
 * @param buffer The buffer containing the data to be written.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_nand_pio_write(struct cadence_nand_params *params, uint32_t row_address,
			       char *buffer)
{
	uintptr_t base_address;
	int ret;

	base_address = params->nand_base;

	ret = cdns_nand_pio_prepare(base_address, NF_TDEF_TRD_NUM, NF_TDEF_DEV_NUM, row_address,
				    buffer, CNF_CMD_WR, DMA_MS_SEL, VOL_ID);

	if (ret != 0) {
		return ret;
	}
	NAND_INT_SEM_TAKE(params);
	ret = cdns_pio_transfer_complete(base_address, NF_TDEF_TRD_NUM);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

/**
 * Perform a PIO read operation for the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param row_address The row address for the read operation.
 * @param buffer The buffer to store the read data.
 * @retval   0 on success or negative error value on failure.
 */
static int cdns_nand_pio_read(struct cadence_nand_params *params, uint32_t row_address,
			      char *buffer)
{
	uintptr_t base_address;
	int ret;

	base_address = params->nand_base;

	ret = cdns_nand_pio_prepare(base_address, NF_TDEF_TRD_NUM, NF_TDEF_DEV_NUM, row_address,
				    buffer, CNF_CMD_RD, DMA_MS_SEL, VOL_ID);

	if (ret != 0) {
		return ret;
	}

	NAND_INT_SEM_TAKE(params);
	ret = cdns_pio_transfer_complete(base_address, NF_TDEF_TRD_NUM);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

/**
 * Perform a combined PIO read and write operation for the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param start_page_number The starting page number for the read/write operation.
 * @param buffer The buffer containing the data to be written or to store the read data.
 * @param page_count The number of pages to be read or written.
 * @param mode The mode of operation (read, write).
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_nand_pio_read_write(struct cadence_nand_params *params, uint32_t start_page_number,
				    char *buffer, uint32_t page_count, uint8_t mode)
{
	uint32_t index;
	uint32_t pio_row_address = 0;
	int ret = 0;

	for (index = 0; index < page_count; index++) {
		row_address_set(params, &pio_row_address, start_page_number++);
		if (mode == CDNS_READ) {
			ret = cdns_nand_pio_read(params, pio_row_address,
						 buffer + (index * (params->page_size)));
		} else {
			ret = cdns_nand_pio_write(params, pio_row_address,
						  buffer + (index * (params->page_size)));
		}
	}
	return ret;
}
#endif

#if CONFIG_CDNS_NAND_GENERIC_MODE
/**
 * Send a generic command to the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param mini_ctrl_cmd The command to be sent.
 * @retval  0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_generic_send_cmd(struct cadence_nand_params *params, uint64_t mini_ctrl_cmd)
{

	uint32_t mini_ctrl_cmd_l, mini_ctrl_cmd_h, status;
	uintptr_t base_address;
	int ret = 0;

	base_address = params->nand_base;
	mini_ctrl_cmd_l = mini_ctrl_cmd & U32_MASK_VAL;
	mini_ctrl_cmd_h = mini_ctrl_cmd >> 32;
	ret = cdns_nand_wait_idle(base_address);

	if (ret != 0) {
		LOG_ERR("Wait for controller to be in idle state Failed");
		return ret;
	}
	sys_write32(mini_ctrl_cmd_l, (base_address + CDNS_CMD_REG2));
	sys_write32(mini_ctrl_cmd_h, (base_address + CDNS_CMD_REG3));
	/* Select generic command. */
	status = CMD_0_THREAD_POS_SET(NF_TDEF_TRD_NUM);
#ifdef CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
	status |= GEN_CF_INT_SET(GEN_CF_INT_ENABLE);
#endif
	status |= CMD_0_C_MODE_SET(CT_GENERIC_MODE);
	sys_write32(status, (base_address + CDNS_CMD_REG0));
	return 0;
}

/**
 * Send a generic command data to the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param mode The mode of operation (read, write).
 * @param data_length The length of the associated data.
 * @retval   0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_generic_cmd_data(struct cadence_nand_params *params, uint8_t mode,
				 uint32_t data_length)
{
	uint64_t mini_ctrl_cmd = 0;
	int ret = 0;

	mini_ctrl_cmd |= GCMD_TWB_VALUE;
	mini_ctrl_cmd |= GCMCD_DATA_SEQ;
	mini_ctrl_cmd |= GEN_SECTOR_COUNT_SET;
	mini_ctrl_cmd |= GEN_LAST_SECTOR_SIZE_SET((uint64_t)data_length);
	mini_ctrl_cmd |= GEN_DIR_SET((uint64_t)mode);
	mini_ctrl_cmd |= GEN_SECTOR_SET((uint64_t)data_length);
	ret = cdns_generic_send_cmd(params, mini_ctrl_cmd);
	return ret;
}

/**
 * Wait for the completion of an SDMA operation in the Cadence NAND controller.
 *
 * @param base_address The base address of the Cadence NAND controller.
 * @retval  0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_wait_sdma(uintptr_t base_address)
{

	if (!WAIT_FOR(((sys_read32(base_address + INTR_STATUS) & BIT(SDMA_TRIGG)) != 0),
		      IDLE_TIME_OUT, k_msleep(1))) {
		LOG_ERR("Timed out waiting for sdma response");
		return -ETIMEDOUT;
	}
	sys_set_bit((base_address + INTR_STATUS), SDMA_TRIGG);
	return 0;
}

/**
 * Perform buffer copying to SDMA regs in the Cadence NAND controller.
 *
 * @param sdma_base_address The base address of the SDMA in the Cadence NAND controller.
 * @param buffer The source or destination buffer for the copy operation.
 * @param data_length The length of the data to be copied.
 */
static void sdma_buffer_copy_in(uint32_t sdma_base_address, uint8_t *buffer, uint32_t data_length)
{
	uint32_t index;

	for (index = 0; index < data_length; index++) {
		sys_write8(*(buffer + index), sdma_base_address + index);
	}
}

/**
 * Perform buffer copying from SDMA regs in the Cadence NAND controller.
 *
 * @param sdma_base_address The base address of the SDMA in the Cadence NAND controller.
 * @param buffer The source or destination buffer for the copy operation.
 * @param data_length The length of the data to be copied.
 */
static void sdma_buffer_copy_out(uint32_t sdma_base_address, uint8_t *buffer, uint32_t data_length)
{
	uint32_t index;

	for (index = 0; index < data_length; index++) {
		*(buffer + index) = sys_read8(sdma_base_address + index);
	}
}

/**
 * Perform a generic page read operation in the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param read_address The address from which to read the page.
 * @param data_buffer The buffer to store the read data.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_generic_page_read(struct cadence_nand_params *params, uint64_t read_address,
				  void *data_buffer)
{
	uint64_t mini_ctrl_cmd = 0;
	uintptr_t base_address = params->nand_base;
	int ret;

	mini_ctrl_cmd = PAGE_READ_CMD;
	mini_ctrl_cmd |= GCMD_TWB_VALUE;
	if ((params->nluns > 1) || (params->total_bit_row > 16)) {
		mini_ctrl_cmd |= PAGE_MAX_BYTES(PAGE_MAX_SIZE);
	} else {
		mini_ctrl_cmd |= PAGE_MAX_BYTES(PAGE_MAX_SIZE - 1);
	}
	mini_ctrl_cmd |= read_address << 32;
	ret = cdns_generic_send_cmd(params, mini_ctrl_cmd);
	if (ret != 0) {
		return ret;
	}
	NAND_INT_SEM_TAKE(params);
	ret = cdns_generic_cmd_data(params, CDNS_READ, params->page_size);
	if (ret != 0) {
		return ret;
	}
	NAND_INT_SEM_TAKE(params);
	ret = cdns_wait_sdma(base_address);
	if (ret != 0) {
		return ret;
	}
	sdma_buffer_copy_out(params->sdma_base, data_buffer, params->page_size);
	return 0;
}

/**
 * Perform a generic page write operation in the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param write_address The address to which the page will be written.
 * @param data_buffer The buffer containing the data to be written.
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_generic_page_write(struct cadence_nand_params *params, uint64_t write_address,
				   void *data_buffer)
{
	uint64_t mini_ctrl_cmd = 0;
	int ret;

	uintptr_t base_address = params->nand_base;

	mini_ctrl_cmd |= GCMD_TWB_VALUE;
	mini_ctrl_cmd |= GEN_ADDR_WRITE_DATA((uint32_t)write_address);
	if ((params->nluns > 1) || (params->total_bit_row > BIT16_CHECK)) {
		mini_ctrl_cmd |= PAGE_MAX_BYTES(PAGE_MAX_SIZE);
	} else {
		mini_ctrl_cmd |= PAGE_MAX_BYTES(PAGE_MAX_SIZE - 1);
	}
	mini_ctrl_cmd |= PAGE_WRITE_CMD;
	ret = cdns_generic_send_cmd(params, mini_ctrl_cmd);
	if (ret != 0) {
		return ret;
	}
	NAND_INT_SEM_TAKE(params);
	ret = cdns_generic_cmd_data(params, CDNS_WRITE, params->page_size);
	if (ret != 0) {
		return ret;
	}
	sdma_buffer_copy_in(params->sdma_base, data_buffer, params->page_size);
	NAND_INT_SEM_TAKE(params);
	mini_ctrl_cmd = 0;
	mini_ctrl_cmd |= PAGE_WRITE_10H_CMD;
	mini_ctrl_cmd |= GCMD_TWB_VALUE;
	mini_ctrl_cmd |= PAGE_CMOD_CMD;
	ret = cdns_generic_send_cmd(params, mini_ctrl_cmd);
	if (ret != 0) {
		return ret;
	}
	NAND_INT_SEM_TAKE(params);
	ret = cdns_wait_sdma(base_address);
	return ret;
}

/**
 * Perform a generic read or write operation for a range of pages in the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param start_page_number The starting page number for the read or write operation.
 * @param buffer The buffer containing the data to be written or to store the read data.
 * @param page_count The number of pages to be read or written.
 * @param mode The mode of operation (read, write).
 * @retval  0 on success or negative error value on failure.
 */
static int cdns_nand_gen_read_write(struct cadence_nand_params *params, uint32_t start_page_number,
				    char *buffer, uint32_t page_count, uint8_t mode)
{
	uint64_t address = 0;
	uint32_t index = 0;
	uint32_t gen_row_address = 0;
	int ret = 0;

	for (index = 0; index < page_count; index++) {
		row_address_set(params, &gen_row_address, start_page_number++);
		address = ((uint64_t)gen_row_address);
		if (mode == CDNS_READ) {
			ret = cdns_generic_page_read(params, address,
						     buffer + (index * (params->page_size)));
			if (ret != 0) {
				LOG_ERR("Cadence NAND Generic Page Read Error!!");
				return ret;
			}
		} else {
			ret = cdns_generic_page_write(params, address,
						      buffer + (index * (params->page_size)));
			if (ret != 0) {
				LOG_ERR("Cadence NAND Generic Page write Error!!");
				return ret;
			}
		}
	}
	return 0;
}

/**
 * Perform a generic erase operation for a range of blocks in the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param start_block The starting block number for the erase operation.
 * @param block_count The number of blocks to be erased.
 * @retval  0 on success or -ETIMEDOUT error value on failure.
 */
static int cdns_nand_gen_erase(struct cadence_nand_params *params, uint32_t start_block,
			       uint32_t block_count)
{
	uint64_t mini_ctrl_cmd = 0;
	uintptr_t base_address = 0;
	uint32_t gen_row_address = 0;
	uint32_t index = 0;
	int ret = 0;

	for (index = 0; index < block_count; index++) {
		row_address_set(params, &gen_row_address, (start_block * params->npages_per_block));
		start_block++;
		base_address = params->nand_base;
		mini_ctrl_cmd |= GCMD_TWB_VALUE;
		mini_ctrl_cmd |= ERASE_ADDR_SIZE;
		mini_ctrl_cmd |= ((gen_row_address) & (U32_MASK_VAL));
		mini_ctrl_cmd |= PAGE_ERASE_CMD;
		ret = cdns_generic_send_cmd(params, mini_ctrl_cmd);
		if (ret != 0) {
			return ret;
		}
		NAND_INT_SEM_TAKE(params);
	}
	return 0;
}
#endif

/**
 * Read data from the Cadence NAND controller into a buffer.
 */
static inline int cdns_read_data(struct cadence_nand_params *params, uint32_t start_page_number,
				 const void *buffer, uint32_t page_count)
{
	int ret;

#if CONFIG_CDNS_NAND_CDMA_MODE
	ret = cdns_nand_cdma_read(params, start_page_number, (char *)buffer, page_count);
#elif CONFIG_CDNS_NAND_PIO_MODE
	ret = cdns_nand_pio_read_write(params, start_page_number, (char *)buffer, page_count,
				       CDNS_READ);
#elif CONFIG_CDNS_NAND_GENERIC_MODE
	ret = cdns_nand_gen_read_write(params, start_page_number, (char *)buffer, page_count,
				       CDNS_READ);
#endif
	return ret;
}

/**
 * Read data from the Cadence NAND controller into a buffer.
 *
 * @param params The Cadence NAND parameters structure.
 * @param buffer The buffer to store the read data.
 * @param offset The offset within the NAND to start reading from.
 * @param size The size of the data to read.
 * @retval  0 on success or negative error value on failure.
 */
int cdns_nand_read(struct cadence_nand_params *params, const void *buffer, uint32_t offset,
		   uint32_t size)
{
	uint32_t start_page_number;
	uint32_t end_page_number;
	uint32_t page_count;
	int ret = 0;
	uint16_t r_bytes;
	uint16_t bytes_dif;
	uint16_t lp_bytes_dif;
	uint8_t check_page_first = 0;
	uint8_t check_page_last = 0;
	uint8_t *first_end_page;
	uint8_t *last_end_page;

	if (params == NULL) {
		LOG_ERR("Wrong parameter passed!!");
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	if ((offset >= params->device_size) || (size > (params->device_size - offset))) {
		LOG_ERR("Wrong offset or size value passed!!");
		return -EINVAL;
	}

	start_page_number = offset / (params->page_size);
	end_page_number = ((offset + size) - 1) / ((params->page_size));

	if ((offset % params->page_size) == 0) {
		check_page_first = 1;
	}
	if (((offset + size) % params->page_size) == 0) {
		check_page_last = 1;
	}
	page_count = end_page_number - start_page_number;
	page_count++;
	if ((check_page_last == 1) && (check_page_first == 1)) {
		ret = cdns_read_data(params, start_page_number, (char *)buffer, page_count);
		if (ret != 0) {
			return ret;
		}

	} else if (((check_page_last == 0) && (check_page_first == 1) && (page_count == 1)) ||
		   ((check_page_last == 0) && (check_page_first == 0) && (page_count == 1)) ||
		   ((check_page_last == 1) && (check_page_first == 0) && (page_count == 1))) {
		first_end_page = (char *)k_malloc(sizeof(char) * (params->page_size));
		if (first_end_page != NULL) {
			memset(first_end_page, 0xFF, sizeof(char) * (params->page_size));
		} else {
			LOG_ERR("Memory allocation error occurred %s", __func__);
			return -ENOSR;
		}
		ret = cdns_read_data(params, start_page_number, first_end_page, page_count);
		if (ret != 0) {
			k_free(first_end_page);
			return ret;
		}
		memcpy((char *)buffer, first_end_page + (offset % (params->page_size)), size);
		k_free(first_end_page);
	} else if (((check_page_last == 0) && (check_page_first == 1) && (page_count == 2)) ||
		   ((check_page_last == 0) && (check_page_first == 0) && (page_count == 2)) ||
		   ((check_page_last == 1) && (check_page_first == 0) && (page_count == 2))) {
		first_end_page = (char *)k_malloc(sizeof(char) * (params->page_size * 2));
		if (first_end_page != NULL) {
			memset(first_end_page, 0xFF, sizeof(char) * (params->page_size * 2));
		} else {
			LOG_ERR("Memory allocation error occurred %s", __func__);
			return -ENOSR;
		}
		ret = cdns_read_data(params, start_page_number, first_end_page, page_count);
		if (ret < 0) {
			k_free(first_end_page);
			return ret;
		}
		memcpy((char *)buffer, first_end_page + (offset % (params->page_size)), size);
		k_free(first_end_page);

	} else if ((check_page_last == 0) && (check_page_first == 1) && (page_count > 2)) {
		first_end_page = (char *)k_malloc(sizeof(char) * (params->page_size));
		if (first_end_page != NULL) {
			memset(first_end_page, 0xFF, sizeof(char) * (params->page_size));
		} else {
			LOG_ERR("Memory allocation error occurred %s", __func__);
			return -ENOSR;
		}
		ret = cdns_read_data(params, end_page_number, first_end_page, 1);
		if (ret < 0) {
			k_free(first_end_page);
			return ret;
		}
		r_bytes = (offset + size) % (params->page_size);
		ret = cdns_read_data(params, start_page_number, (char *)buffer, (--page_count));
		if (ret != 0) {
			k_free(first_end_page);
			return ret;
		}

		memcpy((char *)buffer + ((page_count - 1) * params->page_size), first_end_page,
		       r_bytes);
		k_free(first_end_page);

	} else if ((check_page_last == 1) && (check_page_first == 0) && (page_count > 2)) {
		first_end_page = (char *)k_malloc(sizeof(char) * (params->page_size));
		if (first_end_page != NULL) {
			memset(first_end_page, 0xFF, sizeof(char) * (params->page_size));
		} else {
			LOG_ERR("Memory allocation error occurred %s", __func__);
			return -ENOSR;
		}
		ret = cdns_read_data(params, start_page_number, first_end_page, 1);
		if (ret < 0) {
			k_free(first_end_page);
			return ret;
		}
		r_bytes = (offset) % (params->page_size);
		bytes_dif = (((start_page_number + 1) * params->page_size) - r_bytes);
		r_bytes = (offset + size) % (params->page_size);
		ret = cdns_read_data(params, (++start_page_number), ((char *)buffer + bytes_dif),
				     (--page_count));
		if (ret != 0) {
			k_free(first_end_page);
			return ret;
		}
		memcpy((char *)buffer, first_end_page + r_bytes, bytes_dif);
		k_free(first_end_page);
	} else if ((check_page_last == 0) && (check_page_first == 0) && (page_count > 2)) {
		first_end_page = (char *)k_malloc(sizeof(char) * (params->page_size));
		last_end_page = (char *)k_malloc(sizeof(char) * (params->page_size));
		if ((first_end_page != NULL) && (last_end_page != NULL)) {
			memset(first_end_page, 0xFF, sizeof(char) * (params->page_size));
			memset(last_end_page, 0xFF, sizeof(char) * (params->page_size));
		} else {
			LOG_ERR("Memory allocation error occurred %s", __func__);
			return -ENOSR;
		}
		ret = cdns_read_data(params, start_page_number, first_end_page, 1);
		if (ret != 0) {
			k_free(first_end_page);
			k_free(last_end_page);
			return ret;
		}
		r_bytes = (offset) % (params->page_size);
		bytes_dif = (((start_page_number + 1) * params->page_size) - r_bytes);
		lp_bytes_dif = (offset + size) % (params->page_size);
		ret = cdns_read_data(params, end_page_number, last_end_page, 1);
		if (ret != 0) {
			k_free(last_end_page);
			k_free(first_end_page);
			return ret;
		}
		r_bytes = (offset + size) % (params->page_size);
		ret = cdns_read_data(params, (++start_page_number), ((char *)buffer + bytes_dif),
				     (page_count - 2));
		if (ret != 0) {
			k_free(last_end_page);
			k_free(first_end_page);
			return ret;
		}
		memcpy((char *)buffer, first_end_page + r_bytes, bytes_dif);
		memcpy(((char *)buffer + bytes_dif +
			((page_count - 2) * (params->npages_per_block))),
		       last_end_page, lp_bytes_dif);
	}

	return 0;
}

/**
 * Write data from a buffer to the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param buffer The buffer containing the data to be written.
 * @param offset The offset within the NAND to start writing to.
 * @param len The length of the data to write.
 * @retval  0 on success or negative error value on failure.
 */
int cdns_nand_write(struct cadence_nand_params *params, const void *buffer, uint32_t offset,
		    uint32_t len)
{
	uint32_t start_page_number;
	uint32_t end_page_number;
	uint32_t page_count;
	int ret = 0;

	if (params == NULL) {
		LOG_ERR("Wrong parameter passed!!");
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if ((offset >= params->device_size) || (len > (params->device_size - offset))) {
		LOG_ERR("Wrong offset or len value passed!!");
		return -EINVAL;
	}

	if ((offset % params->page_size) != 0) {
		LOG_ERR("offset not page aligned!!! Page size = 0x%x", params->page_size);
		return -EINVAL;
	}

	if ((len % params->page_size) != 0) {
		LOG_ERR("length not page aligned!!! Page size = 0x%x", params->page_size);
		return -EINVAL;
	}

	start_page_number = offset / (params->page_size);
	end_page_number = ((offset + len) - 1) / ((params->page_size));
	page_count = end_page_number - start_page_number;

#if CONFIG_CDNS_NAND_CDMA_MODE
	ret = cdns_nand_cdma_write(params, start_page_number, (char *)buffer, ++page_count);
#elif CONFIG_CDNS_NAND_PIO_MODE
	ret = cdns_nand_pio_read_write(params, start_page_number, (char *)buffer, ++page_count,
				       CDNS_WRITE);
#elif CONFIG_CDNS_NAND_GENERIC_MODE
	ret = cdns_nand_gen_read_write(params, start_page_number, (char *)buffer, ++page_count,
				       CDNS_WRITE);
#endif
	if (ret != 0) {
		LOG_ERR("Cadence driver write Failed!!!");
	}

	return ret;
}

/**
 * Perform an erase operation on the Cadence NAND controller.
 *
 * @param params The Cadence NAND parameters structure.
 * @param offset The offset within the NAND to start erasing.
 * @param size The size of the data to erase.
 * @retval  0 on success or negative error value on failure.
 */
int cdns_nand_erase(struct cadence_nand_params *params, uint32_t offset, uint32_t size)
{
	uint32_t start_block_number;
	uint32_t end_block_number;
	uint32_t block_count;
	int ret;

	if (params == NULL) {
		LOG_ERR("Wrong parameter passed!!");
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	if ((offset >= params->device_size) || (size > (params->device_size - offset))) {
		LOG_ERR("Wrong offset or size value passed!!");
		return -EINVAL;
	}
	if ((offset % (params->block_size)) != 0) {
		LOG_ERR("Offset value not aligned with block size!! Erase block size = %x",
			params->block_size);
		return -EINVAL;
	}
	if ((size % (params->block_size)) != 0) {
		LOG_ERR("Length value not aligned with block size!! Erase block size = %x",
			params->block_size);
		return -EINVAL;
	}

	start_block_number = (offset / ((params->page_size))) / (params->npages_per_block);
	end_block_number =
		(((offset + size) - 1) / ((params->page_size))) / (params->npages_per_block);
	block_count = end_block_number - start_block_number;
#if CONFIG_CDNS_NAND_CDMA_MODE
	ret = cdns_nand_cdma_erase(params, start_block_number, ++block_count);
#elif CONFIG_CDNS_NAND_PIO_MODE
	ret = cdns_nand_pio_erase(params, NF_TDEF_TRD_NUM, NF_TDEF_DEV_NUM, start_block_number,
				  CNF_CMD_ERASE, ++block_count);
#elif CONFIG_CDNS_NAND_GENERIC_MODE
	ret = cdns_nand_gen_erase(params, start_block_number, ++block_count);
#endif
	if (ret != 0) {
		LOG_ERR("Cadence driver Erase Failed!!!");
	}

	return ret;
}

#if CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
void cdns_nand_irq_handler_ll(struct cadence_nand_params *params)
{
	uint32_t status = 0;
	uint8_t thread_num = 0;

	status = sys_read32(params->nand_base + THREAD_INTERRUPT_STATUS);
	thread_num = find_lsb_set(status);

	if (GET_INIT_SET_CHECK(status, (thread_num - 1)) != 0) {
		/* Clear the interrupt*/
		sys_write32(BIT((thread_num - 1)), params->nand_base + THREAD_INTERRUPT_STATUS);
	}
}
#endif
