/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qspi_memslot.h"

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

static cy_stc_smif_hybrid_region_info_t *sfdp_slave_slot_0_region_info[16] = {
	&sfdp_slave_slot_0_region_info_storage[0],
	&sfdp_slave_slot_0_region_info_storage[1],
	&sfdp_slave_slot_0_region_info_storage[2],
	&sfdp_slave_slot_0_region_info_storage[3],
	&sfdp_slave_slot_0_region_info_storage[4],
	&sfdp_slave_slot_0_region_info_storage[5],
	&sfdp_slave_slot_0_region_info_storage[6],
	&sfdp_slave_slot_0_region_info_storage[7],
	&sfdp_slave_slot_0_region_info_storage[8],
	&sfdp_slave_slot_0_region_info_storage[9],
	&sfdp_slave_slot_0_region_info_storage[10],
	&sfdp_slave_slot_0_region_info_storage[11],
	&sfdp_slave_slot_0_region_info_storage[12],
	&sfdp_slave_slot_0_region_info_storage[13],
	&sfdp_slave_slot_0_region_info_storage[14],
	&sfdp_slave_slot_0_region_info_storage[15],
};
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
