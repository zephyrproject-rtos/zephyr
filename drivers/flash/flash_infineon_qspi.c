/*
 * Copyright 2024 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT     infineon_cat1_qspi_flash
#define SOC_NV_FLASH_NODE DT_PARENT(DT_INST(0, fixed_partitions))

#define PAGE_LEN DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include "cy_serial_flash_qspi.h"
#include "cy_smif_memslot.h"

LOG_MODULE_REGISTER(flash_infineon_cat1, CONFIG_FLASH_LOG_LEVEL);

/* Device config structure */
struct ifx_cat1_flash_config {
	uint32_t base_addr;
	uint32_t max_addr;
};

/* Data structure */
struct ifx_cat1_flash_data {
	cyhal_flash_t flash_obj;
	struct k_sem sem;
};

static struct flash_parameters ifx_cat1_flash_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0xFF,
};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_read_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_write_en_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_write_dis_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_erase_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_chip_erase_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_program_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_read_sts_reg_qe_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_read_sts_reg_wip_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_write_sts_reg_qe_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_read_sts_reg_oe_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_write_sts_reg_oe_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_read_latency_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_write_latency_cmd = {0};

cy_stc_smif_mem_cmd_t sfdp_slave_slot_0_read_sfdp_cmd = {
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x5AU,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 8U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE,
};

cy_stc_smif_octal_ddr_en_seq_t oe_sequence_SFDP_SlaveSlot_0 = {
	.cmdSeq1Len = CY_SMIF_SFDP_ODDR_CMD_SEQ_MAX_LEN,
	.cmdSeq2Len = CY_SMIF_SFDP_ODDR_CMD_SEQ_MAX_LEN,
	.cmdSeq1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	.cmdSeq2 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

/* Support for memories with hybrid regions is added in the version 1.50
 * Please refer to the changelog in
 * https://iot-webserver.aus.cypress.com/projects/iot_release/
 * ASSETS/repo/mtb-pdl-cat1/develop/Latest/deploy/docs/
 * pdl_api_reference_manual/html/group__group__smif.html
 * for more details
 */
#if (CY_SMIF_DRV_VERSION_MAJOR > 1) && (CY_SMIF_DRV_VERSION_MINOR >= 50)
static cy_stc_smif_hybrid_region_info_t sfdp_slave_slot_0_region_info_storage[16];

#define GENERATE_REGION_INFO_PTR(index, _) &sfdp_slave_slot_0_region_info_storage[index],

static cy_stc_smif_hybrid_region_info_t *sfdp_slave_slot_0_region_info[16] = {
	LISTIFY(16, GENERATE_REGION_INFO_PTR, ())};
#endif

cy_stc_smif_mem_device_cfg_t deviceCfg_SFDP_SlaveSlot_0 = {
	/* Specifies the number of address bytes used by the memory slave device. */
	.numOfAddrBytes = 0x03U,
	/* The size of the memory. */
	.memSize = 0x0000100U,
	/* Specifies the Read command. */
	.readCmd = &sfdp_slave_slot_0_read_cmd,
	/* Specifies the Write Enable command. */
	.writeEnCmd = &sfdp_slave_slot_0_write_en_cmd,
	/* Specifies the Write Disable command. */
	.writeDisCmd = &sfdp_slave_slot_0_write_dis_cmd,
	/* Specifies the Erase command. */
	.eraseCmd = &sfdp_slave_slot_0_erase_cmd,
	/* Specifies the sector size of each erase. */
	.eraseSize = 0x0001000U,
	/* Specifies the Chip Erase command. */
	.chipEraseCmd = &sfdp_slave_slot_0_chip_erase_cmd,
	/* Specifies the Program command. */
	.programCmd = &sfdp_slave_slot_0_program_cmd,
	/* Specifies the page size for programming. */
	.programSize = 0x0000100U,
	/* Specifies the command to read the QE-containing status register. */
	.readStsRegQeCmd = &sfdp_slave_slot_0_read_sts_reg_qe_cmd,
	/* Specifies the command to read the WIP-containing status register. */
	.readStsRegWipCmd = &sfdp_slave_slot_0_read_sts_reg_wip_cmd,
	/* Specifies the read SFDP command */
	.readSfdpCmd = &sfdp_slave_slot_0_read_sfdp_cmd,
	/* Specifies the command to write into the QE-containing status register. */
	.writeStsRegQeCmd = &sfdp_slave_slot_0_write_sts_reg_qe_cmd,
	/* The mask for the status register. */
	.stsRegBusyMask = 0x00U,
	/* The mask for the status register. */
	.stsRegQuadEnableMask = 0x00U,
	/* The max time for the erase type-1 cycle-time in ms. */
	.eraseTime = 1U,
	/* The max time for the chip-erase cycle-time in ms. */
	.chipEraseTime = 16U,
	/* The max time for the page-program cycle-time in us. */
	.programTime = 8U,
#if (CY_SMIF_DRV_VERSION_MAJOR > 1) && (CY_SMIF_DRV_VERSION_MINOR >= 50)
	/* Points to NULL or to structure with info about sectors for hybrid memory. */
	.hybridRegionCount = 0U,
	.hybridRegionInfo = sfdp_slave_slot_0_region_info,
#endif
	/* Specifies the command to read variable latency cycles configuration register */
	.readLatencyCmd = &sfdp_slave_slot_0_read_latency_cmd,
	/* Specifies the command to write variable latency cycles configuration register */
	.writeLatencyCmd = &sfdp_slave_slot_0_write_latency_cmd,
	/* Specifies the address for variable latency cycle address */
	.latencyCyclesRegAddr = 0x00U,
	/* Specifies variable latency cycles Mask */
	.latencyCyclesMask = 0x00U,
	/* Specifies data for memory with hybrid sectors */
	.octalDDREnableSeq = &oe_sequence_SFDP_SlaveSlot_0,
	/* Specifies the command to read the OE-containing status register. */
	.readStsRegOeCmd = &sfdp_slave_slot_0_read_sts_reg_oe_cmd,
	/* Specifies the command to write the OE-containing status register. */
	.writeStsRegOeCmd = &sfdp_slave_slot_0_write_sts_reg_oe_cmd,
	/* QE mask for the status registers */
	.stsRegOctalEnableMask = 0x00U,
	/* Octal enable register address */
	.octalEnableRegAddr = 0x00U,
	/* Frequency of operation used in Octal mode */
	.freq_of_operation = CY_SMIF_100MHZ_OPERATION,
};

cy_stc_smif_mem_config_t sfdp_slave_slot_0 = {
	/* Determines the slot number where the memory device is placed. */
	.slaveSelect = CY_SMIF_SLAVE_SELECT_0,
	/* Flags. */
	.flags = CY_SMIF_FLAG_SMIF_REV_3 | CY_SMIF_FLAG_MEMORY_MAPPED | CY_SMIF_FLAG_WR_EN |
		 CY_SMIF_FLAG_DETECT_SFDP | CY_SMIF_FLAG_MERGE_ENABLE,
	/* The data-line selection options for a slave device. */
	.dataSelect = CY_SMIF_DATA_SEL0,
	/* The base address the memory slave
	 * Valid when the memory-mapped mode is enabled.
	 */
	.baseAddress = 0x60000000U,
	/* The size allocated in the memory map, for the memory slave device.
	 * The size is allocated from the base address. Valid when the memory mapped mode is
	 * enabled.
	 */
	.memMappedSize = 0x100000U,
	/* If this memory device is one of the devices in the dual quad SPI configuration.
	 * Valid when the memory mapped mode is enabled.
	 */
	.dualQuadSlots = 0,
	/* The configuration of the device. */
	.deviceCfg = &deviceCfg_SFDP_SlaveSlot_0,
	/** Continuous transfer merge timeout.
	 * After this period the memory device is deselected. A later transfer, even from a
	 * continuous address, starts with the overhead phases (command, address, mode, dummy
	 * cycles). This configuration parameter is available for CAT1B devices.
	 */
	.mergeTimeout = CY_SMIF_MERGE_TIMEOUT_1_CYCLE,
};

static inline void flash_ifx_sem_take(const struct device *dev)
{
	struct ifx_cat1_flash_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static inline void flash_ifx_sem_give(const struct device *dev)
{
	struct ifx_cat1_flash_data *data = dev->data;

	k_sem_give(&data->sem);
}

static int ifx_cat1_flash_read(const struct device *dev, off_t offset, void *data, size_t data_len)
{
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret = 0;

	if (!data_len) {
		return 0;
	}

	flash_ifx_sem_take(dev);

	rslt = cy_serial_flash_qspi_read(offset, data_len, data);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Error reading @ %lu (Err:0x%x)", offset, rslt);
		ret = -EIO;
	}

	flash_ifx_sem_give(dev);
	return ret;
}

static int ifx_cat1_flash_write(const struct device *dev, off_t offset, const void *data,
				size_t data_len)
{
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret = 0;

	if (data_len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	flash_ifx_sem_take(dev);

	rslt = cy_serial_flash_qspi_write(offset, data_len, data);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Error in writing @ %lu (Err:0x%x)", offset, rslt);
		ret = -EIO;
	}

	flash_ifx_sem_give(dev);
	return ret;
}

static int ifx_cat1_flash_erase(const struct device *dev, off_t offset, size_t size)
{
	cy_rslt_t rslt;
	int ret = 0;

	if (offset < 0) {
		return -EINVAL;
	}

	flash_ifx_sem_take(dev);

	rslt = cy_serial_flash_qspi_erase(offset, size);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Error in erasing : 0x%x", rslt);
		ret = -EIO;
	}

	flash_ifx_sem_give(dev);
	return ret;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout ifx_cat1_flash_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / PAGE_LEN,
	.pages_size = PAGE_LEN,
};

static void ifx_cat1_flash_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	*layout = &ifx_cat1_flash_pages_layout;

	/*
	 * For flash memories which have uniform page sizes, this routine
	 * returns an array of length 1, which specifies the page size and
	 * number of pages in the memory.
	 */
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *ifx_cat1_flash_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &ifx_cat1_flash_parameters;
}

static int ifx_cat1_flash_init(const struct device *dev)
{
	struct ifx_cat1_flash_data *data = dev->data;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;

	rslt = cy_serial_flash_qspi_init(&sfdp_slave_slot_0, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC,
					 50000000lu);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Serial Flash initialization failed [rslt: 0x%x]", rslt);
	}

	k_sem_init(&data->sem, 1, 1);

	return 0;
}

static DEVICE_API(flash, ifx_cat1_flash_driver_api) = {
	.read = ifx_cat1_flash_read,
	.write = ifx_cat1_flash_write,
	.erase = ifx_cat1_flash_erase,
	.get_parameters = ifx_cat1_flash_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = ifx_cat1_flash_page_layout,
#endif
};

static struct ifx_cat1_flash_data flash_data;

static const struct ifx_cat1_flash_config flash_config = {
	.base_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE),
	.max_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE) + DT_REG_SIZE(SOC_NV_FLASH_NODE)};

DEVICE_DT_INST_DEFINE(0, ifx_cat1_flash_init, NULL, &flash_data, &flash_config, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &ifx_cat1_flash_driver_api);
