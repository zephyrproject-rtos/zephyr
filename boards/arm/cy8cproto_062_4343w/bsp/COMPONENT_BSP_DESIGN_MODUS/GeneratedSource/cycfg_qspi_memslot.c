/*******************************************************************************
 * File Name: cycfg_qspi_memslot.c
 *
 * Description:
 * Provides definitions of the SMIF-driver memory configuration.
 * This file was automatically generated and should not be modified.
 * QSPI Configurator 2.20.0.2857
 *
 ********************************************************************************
 * Copyright 2020 Cypress Semiconductor Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ********************************************************************************/

#include "cycfg_qspi_memslot.h"

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_readCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0xECU,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_QUAD,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0x01U,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_QUAD,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 4U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_QUAD
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_writeEnCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x06U,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_writeDisCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x04U,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_eraseCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0xDCU,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_chipEraseCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x60U,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_programCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x34U,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_QUAD,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_QUAD
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_readStsRegQeCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x35U,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_readStsRegWipCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x05U,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE
};

const cy_stc_smif_mem_cmd_t S25FL512S_SlaveSlot_0_writeStsRegQeCmd =
{
	/* The 8-bit command. 1 x I/O read command. */
	.command = 0x01U,
	/* The width of the command transfer. */
	.cmdWidth = CY_SMIF_WIDTH_SINGLE,
	/* The width of the address transfer. */
	.addrWidth = CY_SMIF_WIDTH_SINGLE,
	/* The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present. */
	.mode = 0xFFFFFFFFU,
	/* The width of the mode command transfer. */
	.modeWidth = CY_SMIF_WIDTH_SINGLE,
	/* The number of dummy cycles. A zero value suggests no dummy cycles. */
	.dummyCycles = 0U,
	/* The width of the data transfer. */
	.dataWidth = CY_SMIF_WIDTH_SINGLE
};

const cy_stc_smif_mem_device_cfg_t deviceCfg_S25FL512S_SlaveSlot_0 =
{
	/* Specifies the number of address bytes used by the memory slave device. */
	.numOfAddrBytes = 0x04U,
	/* The size of the memory. */
	.memSize = 0x04000000U,
	/* Specifies the Read command. */
	.readCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_readCmd,
	/* Specifies the Write Enable command. */
	.writeEnCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_writeEnCmd,
	/* Specifies the Write Disable command. */
	.writeDisCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_writeDisCmd,
	/* Specifies the Erase command. */
	.eraseCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_eraseCmd,
	/* Specifies the sector size of each erase. */
	.eraseSize = 0x00040000U,
	/* Specifies the Chip Erase command. */
	.chipEraseCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_chipEraseCmd,
	/* Specifies the Program command. */
	.programCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_programCmd,
	/* Specifies the page size for programming. */
	.programSize = 0x00000200U,
	/* Specifies the command to read the QE-containing status register. */
	.readStsRegQeCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_readStsRegQeCmd,
	/* Specifies the command to read the WIP-containing status register. */
	.readStsRegWipCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_readStsRegWipCmd,
	/* Specifies the command to write into the QE-containing status register. */
	.writeStsRegQeCmd = (cy_stc_smif_mem_cmd_t *)&S25FL512S_SlaveSlot_0_writeStsRegQeCmd,
	/* The mask for the status register. */
	.stsRegBusyMask = 0x01U,
	/* The mask for the status register. */
	.stsRegQuadEnableMask = 0x02U,
	/* The max time for the erase type-1 cycle-time in ms. */
	.eraseTime = 2600U,
	/* The max time for the chip-erase cycle-time in ms. */
	.chipEraseTime = 460000U,
	/* The max time for the page-program cycle-time in us. */
	.programTime = 1300U,
#if (CY_SMIF_DRV_VERSION_MAJOR > 1) || (CY_SMIF_DRV_VERSION_MINOR >= 50)
	/* Points to NULL or to structure with info about sectors for hybrid memory. */
	.hybridRegionCount = 0U,
	.hybridRegionInfo = NULL
#endif
};

const cy_stc_smif_mem_config_t S25FL512S_SlaveSlot_0 =
{
	/* Determines the slot number where the memory device is placed. */
	.slaveSelect = CY_SMIF_SLAVE_SELECT_0,
	/* Flags. */
	.flags = CY_SMIF_FLAG_MEMORY_MAPPED | CY_SMIF_FLAG_WR_EN,
	/* The data-line selection options for a slave device. */
	.dataSelect = CY_SMIF_DATA_SEL0,
	/* The base address the memory slave is mapped to in the PSoC memory map.
	   Valid when the memory-mapped mode is enabled. */
	.baseAddress = 0x18000000U,
	/* The size allocated in the PSoC memory map, for the memory slave device.
	   The size is allocated from the base address. Valid when the memory mapped mode is enabled. */
	.memMappedSize = 0x4000000U,
	/* If this memory device is one of the devices in the dual quad SPI configuration.
	   Valid when the memory mapped mode is enabled. */
	.dualQuadSlots = 0,
	/* The configuration of the device. */
	.deviceCfg = (cy_stc_smif_mem_device_cfg_t *)&deviceCfg_S25FL512S_SlaveSlot_0
};

const cy_stc_smif_mem_config_t *const smifMemConfigs[] = {
	&S25FL512S_SlaveSlot_0
};

const cy_stc_smif_block_config_t smifBlockConfig =
{
	/* The number of SMIF memories defined. */
	.memCount = CY_SMIF_DEVICE_NUM,
	/* The pointer to the array of memory config structures of size memCount. */
	.memConfig = (cy_stc_smif_mem_config_t **)smifMemConfigs,
	/* The version of the SMIF driver. */
	.majorVersion = CY_SMIF_DRV_VERSION_MAJOR,
	/* The version of the SMIF driver. */
	.minorVersion = CY_SMIF_DRV_VERSION_MINOR
};
