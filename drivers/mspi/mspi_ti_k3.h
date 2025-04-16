/*
 * Copyright (c) 2025 Siemens Mobility GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MSPI_TI_K3_H_
#define ZEPHYR_DRIVERS_MSPI_TI_K3_H_

#include "zephyr/kernel.h"
#include "zephyr/sys/util_macro.h"

/**
 * Timing configuration for the TI K3 MSPI peripheral.
 *
 * These values are put into the DEV_DELAY register and the names match the
 * register parts.
 */
struct mspi_ti_k3_timing_cfg {
	/* amount of clock cycles the CS pin is deasserted between transactions */
	uint8_t nss;
	/* amount of clock cycles no peripheral is selected during switching */
	uint8_t btwn;
	/* amount of clock cycles chip select is held after the last bit was
	 * transmitted
	 */
	uint8_t after;
	/* amount of clock cycles after CS is asserted and the first bit is
	 * transmitted
	 */
	uint8_t init;
};

/**
 * Enum which timing parameters should be modified
 */
enum mspi_ti_k3_timing_param {
	MSPI_TI_K3_TIMING_PARAM_NSS = BIT(0),
	MSPI_TI_K3_TIMING_PARAM_BTWN = BIT(1),
	MSPI_TI_K3_TIMING_PARAM_AFTER = BIT(2),
	MSPI_TI_K3_TIMING_PARAM_INIT = BIT(3)
};

/* Not implemented mspi_dev_cfg bits */
#define TI_K3_OSPI_NOT_IMPLEMENT_DEV_CONFIG_PARAMS                                                 \
	(MSPI_DEVICE_CONFIG_FREQUENCY | MSPI_DEVICE_CONFIG_MEM_BOUND |                             \
	 MSPI_DEVICE_CONFIG_BREAK_TIME)

/* Ignored dev_cfg_bits */
#define TI_K3_OSPI_IGNORED_DEV_CONFIG_PARAMS                                                       \
	(MSPI_DEVICE_CONFIG_RX_DUMMY | MSPI_DEVICE_CONFIG_TX_DUMMY | MSPI_DEVICE_CONFIG_READ_CMD | \
	 MSPI_DEVICE_CONFIG_WRITE_CMD | MSPI_DEVICE_CONFIG_CMD_LEN | MSPI_DEVICE_CONFIG_ADDR_LEN)

/* Default delay for time between clock enablement and chip select and other */
#define TI_K3_OSPI_DEFAULT_DELAY 10

/* Timeout calculations and default timeout values */
#define TI_K3_OSPI_TIME_BETWEEN_RETRIES_MS     10
#define TI_K3_OSPI_TIME_BETWEEN_RETRIES        K_MSEC(TI_K3_OSPI_TIME_BETWEEN_RETRIES_MS)
#define TI_K3_OSPI_DEFAULT_TIMEOUT_MS          100
#define TI_K3_OSPI_GET_NUM_RETRIES(timeout_ms) (timeout_ms / TI_K3_OSPI_TIME_BETWEEN_RETRIES_MS)

/* General register offsets */
#define TI_K3_OSPI_CONFIG_REG                        0x0u
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_REG           0x4u
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_REG           0x8u
#define TI_K3_OSPI_DEV_DELAY_REG                     0xcu
#define TI_K3_OSPI_RD_DATA_CAPTURE_REG               0x10u
#define TI_K3_OSPI_DEV_SIZE_CONFIG_REG               0x14u
#define TI_K3_OSPI_SRAM_PARTITION_CFG_REG            0x18u
#define TI_K3_OSPI_IND_AHB_ADDR_TRIGGER_REG          0x1cu
#define TI_K3_OSPI_DMA_PERIPH_CONFIG_REG             0x20u
#define TI_K3_OSPI_REMAP_ADDR_REG                    0x24u
#define TI_K3_OSPI_MODE_BIT_CONFIG_REG               0x28u
#define TI_K3_OSPI_SRAM_FILL_REG                     0x2cu
#define TI_K3_OSPI_TX_THRESH_REG                     0x30u
#define TI_K3_OSPI_RX_THRESH_REG                     0x34u
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_REG         0x38u
#define TI_K3_OSPI_NO_OF_POLLS_BEF_EXP_REG           0x3cu
#define TI_K3_OSPI_IRQ_STATUS_REG                    0x40u
#define TI_K3_OSPI_IRQ_MASK_REG                      0x44u
#define TI_K3_OSPI_LOWER_WR_PROT_REG                 0x50u
#define TI_K3_OSPI_UPPER_WR_PROT_REG                 0x54u
#define TI_K3_OSPI_WR_PROT_CTRL_REG                  0x58u
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_REG       0x60u
#define TI_K3_OSPI_INDIRECT_READ_XFER_WATERMARK_REG  0x64u
#define TI_K3_OSPI_INDIRECT_READ_XFER_START_REG      0x68u
#define TI_K3_OSPI_INDIRECT_READ_XFER_NUM_BYTES_REG  0x6cu
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_REG      0x70u
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_WATERMARK_REG 0x74u
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_START_REG     0x78u
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_NUM_BYTES_REG 0x7cu
#define TI_K3_OSPI_INDIRECT_TRIGGER_ADDR_RANGE_REG   0x80u
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_REG        0x8cu
#define TI_K3_OSPI_FLASH_CMD_CTRL_REG                0x90u
#define TI_K3_OSPI_FLASH_CMD_ADDR_REG                0x94u
#define TI_K3_OSPI_FLASH_RD_DATA_LOWER_REG           0xa0u
#define TI_K3_OSPI_FLASH_RD_DATA_UPPER_REG           0xa4u
#define TI_K3_OSPI_FLASH_WR_DATA_LOWER_REG           0xa8u
#define TI_K3_OSPI_FLASH_WR_DATA_UPPER_REG           0xacu
#define TI_K3_OSPI_POLLING_FLASH_STATUS_REG          0xb0u
#define TI_K3_OSPI_PHY_CONFIGURATION_REG             0xb4u
#define TI_K3_OSPI_PHY_MASTER_CONTROL_REG            0xb8u
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_REG          0xbcu
#define TI_K3_OSPI_DLL_OBSERVABLE_UPPER_REG          0xc0u
#define TI_K3_OSPI_OPCODE_EXT_LOWER_REG              0xe0u
#define TI_K3_OSPI_OPCODE_EXT_UPPER_REG              0xe4u
#define TI_K3_OSPI_MODULE_ID_REG                     0xfcu

/* CONFIG */
#define TI_K3_OSPI_CONFIG_IDLE_FLD_OFFSET                31
#define TI_K3_OSPI_CONFIG_DUAL_BYTE_OPCODE_EN_FLD_OFFSET 30
#define TI_K3_OSPI_CONFIG_CRC_ENABLE_FLD_OFFSET          29
#define TI_K3_OSPI_CONFIG_PIPELINE_PHY_FLD_OFFSET        25
#define TI_K3_OSPI_CONFIG_ENABLE_DTR_PROTOCOL_FLD_OFFSET 24
#define TI_K3_OSPI_CONFIG_ENABLE_AHB_DECODER_FLD_OFFSET  23
#define TI_K3_OSPI_CONFIG_MSTR_BAUD_DIV_FLD_OFFSET       19
#define TI_K3_OSPI_CONFIG_ENTER_XIP_MODE_IMM_FLD_OFFSET  18
#define TI_K3_OSPI_CONFIG_ENTER_XIP_MODE_FLD_OFFSET      17
#define TI_K3_OSPI_CONFIG_ENB_AHB_ADDR_REMAP_FLD_OFFSET  16
#define TI_K3_OSPI_CONFIG_ENB_DMA_IF_FLD_OFFSET          15
#define TI_K3_OSPI_CONFIG_WR_PROT_FLASH_FLD_OFFSET       14
#define TI_K3_OSPI_CONFIG_PERIPH_CS_LINES_FLD_OFFSET     10
#define TI_K3_OSPI_CONFIG_PERIPH_SEL_DEC_FLD_OFFSET      9
#define TI_K3_OSPI_CONFIG_ENB_LEGACY_IP_MODE_FLD_OFFSET  8
#define TI_K3_OSPI_CONFIG_ENB_DIR_ACC_CTRL_FLD_OFFSET    7
#define TI_K3_OSPI_CONFIG_RESET_CFG_FLD_OFFSET           6
#define TI_K3_OSPI_CONFIG_RESET_PIN_FLD_OFFSET           5
#define TI_K3_OSPI_CONFIG_HOLD_PIN_FLD_OFFSET            4
#define TI_K3_OSPI_CONFIG_PHY_MODE_ENABLE_FLD_OFFSET     3
#define TI_K3_OSPI_CONFIG_SEL_CLK_PHASE_FLD_OFFSET       2
#define TI_K3_OSPI_CONFIG_SEL_CLK_POL_FLD_OFFSET         1
#define TI_K3_OSPI_CONFIG_ENABLE_SPI_FLD_OFFSET          0

#define TI_K3_OSPI_CONFIG_IDLE_FLD_SIZE                1
#define TI_K3_OSPI_CONFIG_DUAL_BYTE_OPCODE_EN_FLD_SIZE 1
#define TI_K3_OSPI_CONFIG_CRC_ENABLE_FLD_SIZE          1
#define TI_K3_OSPI_CONFIG_PIPELINE_PHY_FLD_SIZE        1
#define TI_K3_OSPI_CONFIG_ENABLE_DTR_PROTOCOL_FLD_SIZE 1
#define TI_K3_OSPI_CONFIG_ENABLE_AHB_DECODER_FLD_SIZE  1
#define TI_K3_OSPI_CONFIG_MSTR_BAUD_DIV_FLD_SIZE       4
#define TI_K3_OSPI_CONFIG_ENTER_XIP_MODE_IMM_FLD_SIZE  1
#define TI_K3_OSPI_CONFIG_ENTER_XIP_MODE_FLD_SIZE      1
#define TI_K3_OSPI_CONFIG_ENB_AHB_ADDR_REMAP_FLD_SIZE  1
#define TI_K3_OSPI_CONFIG_ENB_DMA_IF_FLD_SIZE          1
#define TI_K3_OSPI_CONFIG_WR_PROT_FLASH_FLD_SIZE       1
#define TI_K3_OSPI_CONFIG_PERIPH_CS_LINES_FLD_SIZE     4
#define TI_K3_OSPI_CONFIG_PERIPH_SEL_DEC_FLD_SIZE      1
#define TI_K3_OSPI_CONFIG_ENB_LEGACY_IP_MODE_FLD_SIZE  1
#define TI_K3_OSPI_CONFIG_ENB_DIR_ACC_CTRL_FLD_SIZE    1
#define TI_K3_OSPI_CONFIG_RESET_CFG_FLD_SIZE           1
#define TI_K3_OSPI_CONFIG_RESET_PIN_FLD_SIZE           1
#define TI_K3_OSPI_CONFIG_HOLD_PIN_FLD_SIZE            1
#define TI_K3_OSPI_CONFIG_PHY_MODE_ENABLE_FLD_SIZE     1
#define TI_K3_OSPI_CONFIG_SEL_CLK_PHASE_FLD_SIZE       1
#define TI_K3_OSPI_CONFIG_SEL_CLK_POL_FLD_SIZE         1
#define TI_K3_OSPI_CONFIG_ENABLE_SPI_FLD_SIZE          1

/* DEV_INSTR_RD_CONFIG */
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_DUMMY_RD_CLK_CYCLES_FLD_OFFSET     24
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_MODE_BIT_ENABLE_FLD_OFFSET         20
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_DATA_XFER_TYPE_EXT_MODE_FLD_OFFSET 16
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_ADDR_XFER_TYPE_STD_MODE_FLD_OFFSET 12
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_DDR_EN_FLD_OFFSET                  10
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_INSTR_TYPE_FLD_OFFSET              8
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_RD_OPCODE_NON_XIP_FLD_OFFSET       0

#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_DUMMY_RD_CLK_CYCLES_FLD_SIZE     5
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_MODE_BIT_ENABLE_FLD_SIZE         1
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_DATA_XFER_TYPE_EXT_MODE_FLD_SIZE 2
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_ADDR_XFER_TYPE_STD_MODE_FLD_SIZE 2
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_DDR_EN_FLD_SIZE                  1
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_INSTR_TYPE_FLD_SIZE              2
#define TI_K3_OSPI_DEV_INSTR_RD_CONFIG_RD_OPCODE_NON_XIP_FLD_SIZE       8

/* DEV_INSTR_WR_CONFIG */
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_DUMMY_WR_CLK_CYCLES_FLD_OFFSET     24
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_DATA_XFER_TYPE_EXT_MODE_FLD_OFFSET 16
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_ADDR_XFER_TYPE_STD_MODE_FLD_OFFSET 12
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_WEL_DIS_FLD_OFFSET                 8
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_WR_OPCODE_NON_XIP_FLD_OFFSET       0

#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_DUMMY_WR_CLK_CYCLES_FLD_SIZE     5
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_DATA_XFER_TYPE_EXT_MODE_FLD_SIZE 2
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_ADDR_XFER_TYPE_STD_MODE_FLD_SIZE 2
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_WEL_DIS_FLD_SIZE                 1
#define TI_K3_OSPI_DEV_INSTR_WR_CONFIG_WR_OPCODE_NON_XIP_FLD_SIZE       8

/* DEV_DELAY */
#define TI_K3_OSPI_DEV_DELAY_D_NSS_FLD_OFFSET   24
#define TI_K3_OSPI_DEV_DELAY_D_BTWN_FLD_OFFSET  16
#define TI_K3_OSPI_DEV_DELAY_D_AFTER_FLD_OFFSET 8
#define TI_K3_OSPI_DEV_DELAY_D_INIT_FLD_OFFSET  0

#define TI_K3_OSPI_DEV_DELAY_D_NSS_FLD_SIZE   8
#define TI_K3_OSPI_DEV_DELAY_D_BTWN_FLD_SIZE  8
#define TI_K3_OSPI_DEV_DELAY_D_AFTER_FLD_SIZE 8
#define TI_K3_OSPI_DEV_DELAY_D_INIT_FLD_SIZE  8

/* RD_DATA_CAPTURE */
#define TI_K3_OSPI_RD_DATA_CAPTURE_DDR_READ_DELAY_FLD_OFFSET  16
#define TI_K3_OSPI_RD_DATA_CAPTURE_DQS_ENABLE_FLD_OFFSET      8
#define TI_K3_OSPI_RD_DATA_CAPTURE_SAMPLE_EDGE_SEL_FLD_OFFSET 5
#define TI_K3_OSPI_RD_DATA_CAPTURE_DELAY_FLD_OFFSET           1
#define TI_K3_OSPI_RD_DATA_CAPTURE_BYPASS_FLD_OFFSET          0

#define TI_K3_OSPI_RD_DATA_CAPTURE_DDR_READ_DELAY_FLD_SIZE  4
#define TI_K3_OSPI_RD_DATA_CAPTURE_DQS_ENABLE_FLD_SIZE      1
#define TI_K3_OSPI_RD_DATA_CAPTURE_SAMPLE_EDGE_SEL_FLD_SIZE 1
#define TI_K3_OSPI_RD_DATA_CAPTURE_DELAY_FLD_SIZE           4
#define TI_K3_OSPI_RD_DATA_CAPTURE_BYPASS_FLD_SIZE          1

/* DEV_SIZE_CONFIG */
#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS3_FLD_OFFSET       27
#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS2_FLD_OFFSET       25
#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS1_FLD_OFFSET       23
#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS0_FLD_OFFSET       21
#define TI_K3_OSPI_DEV_SIZE_CONFIG_BYTES_PER_SUBSECTOR_FLD_OFFSET   16
#define TI_K3_OSPI_DEV_SIZE_CONFIG_BYTES_PER_DEVICE_PAGE_FLD_OFFSET 4
#define TI_K3_OSPI_DEV_SIZE_CONFIG_NUM_ADDR_BYTES_FLD_OFFSET        0

#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS3_FLD_SIZE       2
#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS2_FLD_SIZE       2
#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS1_FLD_SIZE       2
#define TI_K3_OSPI_DEV_SIZE_CONFIG_MEM_SIZE_ON_CS0_FLD_SIZE       2
#define TI_K3_OSPI_DEV_SIZE_CONFIG_BYTES_PER_SUBSECTOR_FLD_SIZE   5
#define TI_K3_OSPI_DEV_SIZE_CONFIG_BYTES_PER_DEVICE_PAGE_FLD_SIZE 12
#define TI_K3_OSPI_DEV_SIZE_CONFIG_NUM_ADDR_BYTES_FLD_SIZE        4

/* SRAM_PARTITION_CFG */
#define TI_K3_OSPI_SRAM_PARTITION_CFG_ADDR_FLD_OFFSET 0

#define TI_K3_OSPI_SRAM_PARTITION_CFG_ADDR_FLD_SIZE 8

/* INDIRECT_TRIGGER_ADDR_RANGE */
#define TI_K3_OSPI_INDIRECT_TRIGGER_ADDR_RANGE_IND_RANGE_WIDTH_FLD_OFFSET 0

#define TI_K3_OSPI_INDIRECT_TRIGGER_ADDR_RANGE_IND_RANGE_WIDTH_FLD_SIZE 4

/* REMAP_ADDR */
#define TI_K3_OSPI_REMAP_ADDR_VALUE_FLD_OFFSET 0

#define TI_K3_OSPI_REMAP_ADDR_VALUE_FLD_SIZE 32

/* SRAM_FILL */
#define TI_K3_OSPI_SRAM_FILL_INDAC_WRITE_FLD_OFFSET 16
#define TI_K3_OSPI_SRAM_FILL_INDAC_READ_FLD_OFFSET  0

#define TI_K3_OSPI_SRAM_FILL_INDAC_WRITE_FLD_SIZE 16
#define TI_K3_OSPI_SRAM_FILL_INDAC_READ_FLD_SIZE  16

/* TX_THRESH */
#define TI_K3_OSPI_TX_THRESH_LEVEL_FLD_OFFSET 0

#define TI_K3_OSPI_TX_THRESH_LEVEL_FLD_SIZE 5

/* RX_THRESH */
#define TI_K3_OSPI_RX_THRESH_LEVEL_FLD_OFFSET 0

#define TI_K3_OSPI_RX_THRESH_LEVEL_FLD_SIZE 5

/* WRITE_COMPLETION_CTRL */
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLL_REP_DELAY_FLD_OFFSET     24
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLL_COUNT_FLD_OFFSET         16
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_ENABLE_POLLING_EXP_FLD_OFFSET 15
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_DISABLE_POLLING_FLD_OFFSET    14
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLLING_POLARITY_FLD_OFFSET   13
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLLING_BIT_INDEX_FLD_OFFSET  8
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_OPCODE_FLD_OFFSET             0

#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLL_REP_DELAY_FLD_SIZE     8
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLL_COUNT_FLD_SIZE         8
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_ENABLE_POLLING_EXP_FLD_SIZE 1
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_DISABLE_POLLING_FLD_SIZE    1
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLLING_POLARITY_FLD_SIZE   1
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_POLLING_BIT_INDEX_FLD_SIZE  3
#define TI_K3_OSPI_WRITE_COMPLETION_CTRL_OPCODE_FLD_SIZE             8

/* NO_OF_POLLS_BEF_EXP */
#define TI_K3_OSPI_NO_OF_POLLS_BEF_EXP_NO_OF_POLLS_BEF_EXP_FLD_OFFSET 0

#define TI_K3_OSPI_NO_OF_POLLS_BEF_EXP_NO_OF_POLLS_BEF_EXP_FLD_SIZE 32

/* IRQ_STATUS */
#define TI_K3_OSPI_IRQ_STATUS_ECC_FAIL_FLD_OFFSET                   19
#define TI_K3_OSPI_IRQ_STATUS_TX_CRC_CHUNK_BRK_FLD_OFFSET           18
#define TI_K3_OSPI_IRQ_STATUS_RX_CRC_DATA_VAL_FLD_OFFSET            17
#define TI_K3_OSPI_IRQ_STATUS_RX_CRC_DATA_ERR_FLD_OFFSET            16
#define TI_K3_OSPI_IRQ_STATUS_STIG_REQ_INT_FLD_OFFSET               14
#define TI_K3_OSPI_IRQ_STATUS_POLL_EXP_INT_FLD_OFFSET               13
#define TI_K3_OSPI_IRQ_STATUS_INDRD_SRAM_FULL_FLD_OFFSET            12
#define TI_K3_OSPI_IRQ_STATUS_RX_FIFO_FULL_FLD_OFFSET               11
#define TI_K3_OSPI_IRQ_STATUS_RX_FIFO_NOT_EMPTY_FLD_OFFSET          10
#define TI_K3_OSPI_IRQ_STATUS_TX_FIFO_FULL_FLD_OFFSET               9
#define TI_K3_OSPI_IRQ_STATUS_TX_FIFO_NOT_FULL_FLD_OFFSET           8
#define TI_K3_OSPI_IRQ_STATUS_RECV_OVERFLOW_FLD_OFFSET              7
#define TI_K3_OSPI_IRQ_STATUS_INDIRECT_XFER_LEVEL_BREACH_FLD_OFFSET 6
#define TI_K3_OSPI_IRQ_STATUS_ILLEGAL_ACCESS_DET_FLD_OFFSET         5
#define TI_K3_OSPI_IRQ_STATUS_PROT_WR_ATTEMPT_FLD_OFFSET            4
#define TI_K3_OSPI_IRQ_STATUS_INDIRECT_READ_REJECT_FLD_OFFSET       3
#define TI_K3_OSPI_IRQ_STATUS_INDIRECT_OP_DONE_FLD_OFFSET           2
#define TI_K3_OSPI_IRQ_STATUS_UNDERFLOW_DET_FLD_OFFSET              1
#define TI_K3_OSPI_IRQ_STATUS_MODE_M_FAIL_FLD_OFFSET                0

#define TI_K3_OSPI_IRQ_STATUS_ECC_FAIL_FLD_SIZE                   1
#define TI_K3_OSPI_IRQ_STATUS_TX_CRC_CHUNK_BRK_FLD_SIZE           1
#define TI_K3_OSPI_IRQ_STATUS_RX_CRC_DATA_VAL_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_STATUS_RX_CRC_DATA_ERR_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_STATUS_STIG_REQ_INT_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_STATUS_POLL_EXP_INT_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_STATUS_INDRD_SRAM_FULL_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_STATUS_RX_FIFO_FULL_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_STATUS_RX_FIFO_NOT_EMPTY_FLD_SIZE          1
#define TI_K3_OSPI_IRQ_STATUS_TX_FIFO_FULL_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_STATUS_TX_FIFO_NOT_FULL_FLD_SIZE           1
#define TI_K3_OSPI_IRQ_STATUS_RECV_OVERFLOW_FLD_SIZE              1
#define TI_K3_OSPI_IRQ_STATUS_INDIRECT_XFER_LEVEL_BREACH_FLD_SIZE 1
#define TI_K3_OSPI_IRQ_STATUS_ILLEGAL_ACCESS_DET_FLD_SIZE         1
#define TI_K3_OSPI_IRQ_STATUS_PROT_WR_ATTEMPT_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_STATUS_INDIRECT_READ_REJECT_FLD_SIZE       1
#define TI_K3_OSPI_IRQ_STATUS_INDIRECT_OP_DONE_FLD_SIZE           1
#define TI_K3_OSPI_IRQ_STATUS_UNDERFLOW_DET_FLD_SIZE              1
#define TI_K3_OSPI_IRQ_STATUS_MODE_M_FAIL_FLD_SIZE                1

#define TI_K3_OSPI_IRQ_STATUS_ALL (BIT_MASK(19) & ~BIT(15))

/* IRQ_MASK */
#define TI_K3_OSPI_IRQ_MASK_ECC_FAIL_MASK_FLD_OFFSET                   19
#define TI_K3_OSPI_IRQ_MASK_TX_CRC_CHUNK_BRK_MASK_FLD_OFFSET           18
#define TI_K3_OSPI_IRQ_MASK_RX_CRC_DATA_VAL_MASK_FLD_OFFSET            17
#define TI_K3_OSPI_IRQ_MASK_RX_CRC_DATA_ERR_MASK_FLD_OFFSET            16
#define TI_K3_OSPI_IRQ_MASK_STIG_REQ_INT_MASK_FLD_OFFSET               14
#define TI_K3_OSPI_IRQ_MASK_POLL_EXP_INT_MASK_FLD_OFFSET               13
#define TI_K3_OSPI_IRQ_MASK_INDRD_SRAM_FULL_MASK_FLD_OFFSET            12
#define TI_K3_OSPI_IRQ_MASK_RX_FIFO_FULL_MASK_FLD_OFFSET               11
#define TI_K3_OSPI_IRQ_MASK_RX_FIFO_NOT_EMPTY_MASK_FLD_OFFSET          10
#define TI_K3_OSPI_IRQ_MASK_TX_FIFO_FULL_MASK_FLD_OFFSET               9
#define TI_K3_OSPI_IRQ_MASK_TX_FIFO_NOT_FULL_MASK_FLD_OFFSET           8
#define TI_K3_OSPI_IRQ_MASK_RECV_OVERFLOW_MASK_FLD_OFFSET              7
#define TI_K3_OSPI_IRQ_MASK_INDIRECT_XFER_LEVEL_BREACH_MASK_FLD_OFFSET 6
#define TI_K3_OSPI_IRQ_MASK_ILLEGAL_ACCESS_DET_MASK_FLD_OFFSET         5
#define TI_K3_OSPI_IRQ_MASK_PROT_WR_ATTEMPT_MASK_FLD_OFFSET            4
#define TI_K3_OSPI_IRQ_MASK_INDIRECT_READ_REJECT_MASK_FLD_OFFSET       3
#define TI_K3_OSPI_IRQ_MASK_INDIRECT_OP_DONE_MASK_FLD_OFFSET           2
#define TI_K3_OSPI_IRQ_MASK_UNDERFLOW_DET_MASK_FLD_OFFSET              1
#define TI_K3_OSPI_IRQ_MASK_MODE_M_FAIL_MASK_FLD_OFFSET                0

#define TI_K3_OSPI_IRQ_MASK_ECC_FAIL_MASK_FLD_SIZE                   1
#define TI_K3_OSPI_IRQ_MASK_TX_CRC_CHUNK_BRK_MASK_FLD_SIZE           1
#define TI_K3_OSPI_IRQ_MASK_RX_CRC_DATA_VAL_MASK_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_MASK_RX_CRC_DATA_ERR_MASK_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_MASK_STIG_REQ_INT_MASK_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_MASK_POLL_EXP_INT_MASK_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_MASK_INDRD_SRAM_FULL_MASK_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_MASK_RX_FIFO_FULL_MASK_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_MASK_RX_FIFO_NOT_EMPTY_MASK_FLD_SIZE          1
#define TI_K3_OSPI_IRQ_MASK_TX_FIFO_FULL_MASK_FLD_SIZE               1
#define TI_K3_OSPI_IRQ_MASK_TX_FIFO_NOT_FULL_MASK_FLD_SIZE           1
#define TI_K3_OSPI_IRQ_MASK_RECV_OVERFLOW_MASK_FLD_SIZE              1
#define TI_K3_OSPI_IRQ_MASK_INDIRECT_XFER_LEVEL_BREACH_MASK_FLD_SIZE 1
#define TI_K3_OSPI_IRQ_MASK_ILLEGAL_ACCESS_DET_MASK_FLD_SIZE         1
#define TI_K3_OSPI_IRQ_MASK_PROT_WR_ATTEMPT_MASK_FLD_SIZE            1
#define TI_K3_OSPI_IRQ_MASK_INDIRECT_READ_REJECT_MASK_FLD_SIZE       1
#define TI_K3_OSPI_IRQ_MASK_INDIRECT_OP_DONE_MASK_FLD_SIZE           1
#define TI_K3_OSPI_IRQ_MASK_UNDERFLOW_DET_MASK_FLD_SIZE              1
#define TI_K3_OSPI_IRQ_MASK_MODE_M_FAIL_MASK_FLD_SIZE                1

#define TI_K3_OSPI_IRQ_MASK_ALL (BIT_MASK(19) & ~BIT(15))

/* LOWER_WR_PROT */
#define TI_K3_OSPI_LOWER_WR_PROT_SUBSECTOR_FLD_OFFSET 0

#define TI_K3_OSPI_LOWER_WR_PROT_SUBSECTOR_FLD_SIZE 32

/* UPPER_WR_PROT */
#define TI_K3_OSPI_UPPER_WR_PROT_SUBSECTOR_FLD_OFFSET 0

#define TI_K3_OSPI_UPPER_WR_PROT_SUBSECTOR_FLD_SIZE 32

/* WR_PROT_CTRL */
#define TI_K3_OSPI_WR_PROT_CTRL_ENB_FLD_OFFSET 1
#define TI_K3_OSPI_WR_PROT_CTRL_INV_FLD_OFFSET 0

#define TI_K3_OSPI_WR_PROT_CTRL_ENB_FLD_SIZE 1
#define TI_K3_OSPI_WR_PROT_CTRL_INV_FLD_SIZE 1

/* INDIRECT_READ_XFER_CTRL */
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_NUM_IND_OPS_DONE_FLD_OFFSET    6
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_IND_OPS_DONE_STATUS_FLD_OFFSET 5
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_RD_QUEUED_FLD_OFFSET           4
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_SRAM_FULL_FLD_OFFSET           3
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_RD_STATUS_FLD_OFFSET           2
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_CANCEL_FLD_OFFSET              1
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_START_FLD_OFFSET               0

#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_NUM_IND_OPS_DONE_FLD_SIZE    2
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_IND_OPS_DONE_STATUS_FLD_SIZE 1
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_RD_QUEUED_FLD_SIZE           1
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_SRAM_FULL_FLD_SIZE           1
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_RD_STATUS_FLD_SIZE           1
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_CANCEL_FLD_SIZE              1
#define TI_K3_OSPI_INDIRECT_READ_XFER_CTRL_START_FLD_SIZE               1

/* INDIRECT_READ_XFER_WATERMARK */
#define TI_K3_OSPI_INDIRECT_READ_XFER_WATERMARK_LEVEL_FLD_OFFSET 0

#define TI_K3_OSPI_INDIRECT_READ_XFER_WATERMARK_LEVEL_FLD_SIZE 32

/* INDIRECT_READ_XFER_START */
#define TI_K3_OSPI_INDIRECT_READ_XFER_START_ADDR_FLD_OFFSET 0

#define TI_K3_OSPI_INDIRECT_READ_XFER_START_ADDR_FLD_SIZE 32

/* INDIRECT_READ_XFER_NUM_BYTES */
#define TI_K3_OSPI_INDIRECT_READ_XFER_NUM_BYTES_VALUE_FLD_OFFSET 0

#define TI_K3_OSPI_INDIRECT_READ_XFER_NUM_BYTES_VALUE_FLD_SIZE 32

/* INDIRECT_WRITE_XFER_CTRL_REG */
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_NUM_IND_OPS_DONE_FLD_OFFSET    6
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_IND_OPS_DONE_STATUS_FLD_OFFSET 5
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_WR_QUEUED_FLD_OFFSET           4
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_WR_STATUS_FLD_OFFSET           2
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_CANCEL_FLD_OFFSET              1
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_START_FLD_OFFSET               0

#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_NUM_IND_OPS_DONE_FLD_SIZE    2
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_IND_OPS_DONE_STATUS_FLD_SIZE 1
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_WR_QUEUED_FLD_SIZE           1
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_WR_STATUS_FLD_SIZE           1
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_CANCEL_FLD_SIZE              1
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_CTRL_START_FLD_SIZE               1

/* INDIRECT_WRITE_XFER_WATERMARK */
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_WATERMARK_LEVEL_FLD_OFFSET 0

#define TI_K3_OSPI_INDIRECT_WRITE_XFER_WATERMARK_LEVEL_FLD_SIZE 32

/* INDIRECT_WRITE_XFER_START */
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_START_ADDR_FLD_OFFSET 0

#define TI_K3_OSPI_INDIRECT_WRITE_XFER_START_ADDR_FLD_SIZE 32

/* INDIRECT_WRITE_XFER_NUM_BYTES */
#define TI_K3_OSPI_INDIRECT_WRITE_XFER_NUM_BYTES_VALUE_FLD_OFFSET 0

#define TI_K3_OSPI_INDIRECT_WRITE_XFER_NUM_BYTES_VALUE_FLD_SIZE 32

/* IND_AHB_ADDR_TRIGGER */
#define TI_K3_OSPI_IND_AHB_ADDR_TRIGGER_ADDR_FLD_OFFSET 0

#define TI_K3_OSPI_IND_AHB_ADDR_TRIGGER_ADDR_FLD_SIZE 32

/* FLASH_COMMAND_CTRL_MEM */
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_MEM_BANK_ADDR_FLD_OFFSET            20
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_NB_OF_STIG_READ_BYTES_FLD_OFFSET    16
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_MEM_BANK_READ_DATA_FLD_OFFSET       8
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_MEM_BANK_REQ_IN_PROGRESS_FLD_OFFSET 1
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_TRIGGER_MEM_BANK_REQ_FLD_OFFSET     0

#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_MEM_BANK_ADDR_FLD_SIZE            9
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_NB_OF_STIG_READ_BYTES_FLD_SIZE    3
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_MEM_BANK_READ_DATA_FLD_SIZE       8
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_MEM_BANK_REQ_IN_PROGRESS_FLD_SIZE 1
#define TI_K3_OSPI_FLASH_COMMAND_CTRL_MEM_TRIGGER_MEM_BANK_REQ_FLD_SIZE     1

/* FLASH_CMD_CTRL */
#define TI_K3_OSPI_FLASH_CMD_CTRL_CMD_OPCODE_FLD_OFFSET        24
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_READ_DATA_FLD_OFFSET     23
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_RD_DATA_BYTES_FLD_OFFSET 20
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_COMD_ADDR_FLD_OFFSET     19
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_MODE_BIT_FLD_OFFSET      18
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_ADDR_BYTES_FLD_OFFSET    16
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_WRITE_DATA_FLD_OFFSET    15
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_WR_DATA_BYTES_FLD_OFFSET 12
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_DUMMY_CYCLES_FLD_OFFSET  7
#define TI_K3_OSPI_FLASH_CMD_CTRL_STIG_MEM_BANK_EN_FLD_OFFSET  2
#define TI_K3_OSPI_FLASH_CMD_CTRL_CMD_EXEC_STATUS_FLD_OFFSET   1
#define TI_K3_OSPI_FLASH_CMD_CTRL_CMD_EXEC_FLD_OFFSET          0

#define TI_K3_OSPI_FLASH_CMD_CTRL_CMD_OPCODE_FLD_SIZE        8
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_READ_DATA_FLD_SIZE     1
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_RD_DATA_BYTES_FLD_SIZE 3
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_COMD_ADDR_FLD_SIZE     1
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_MODE_BIT_FLD_SIZE      1
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_ADDR_BYTES_FLD_SIZE    2
#define TI_K3_OSPI_FLASH_CMD_CTRL_ENB_WRITE_DATA_FLD_SIZE    1
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_WR_DATA_BYTES_FLD_SIZE 3
#define TI_K3_OSPI_FLASH_CMD_CTRL_NUM_DUMMY_CYCLES_FLD_SIZE  5
#define TI_K3_OSPI_FLASH_CMD_CTRL_STIG_MEM_BANK_EN_FLD_SIZE  1
#define TI_K3_OSPI_FLASH_CMD_CTRL_CMD_EXEC_STATUS_FLD_SIZE   1
#define TI_K3_OSPI_FLASH_CMD_CTRL_CMD_EXEC_FLD_SIZE          1

/* FLASH_CMD_ADDR */
#define TI_K3_OSPI_FLASH_CMD_ADDR_ADDR_FLD_OFFSET 0

#define TI_K3_OSPI_FLASH_CMD_ADDR_ADDR_FLD_SIZE 32

/* FLASH_RD_DATA_LOWER */
#define TI_K3_OSPI_FLASH_RD_DATA_LOWER_DATA_FLD_OFFSET 0

#define TI_K3_OSPI_FLASH_RD_DATA_LOWER_DATA_FLD_SIZE 32

/* FLASH_RD_DATA_UPPER */
#define TI_K3_OSPI_FLASH_RD_DATA_UPPER_DATA_FLD_OFFSET 0

#define TI_K3_OSPI_FLASH_RD_DATA_UPPER_DATA_FLD_SIZE 32

/* FLASH_WR_DATA_LOWER */
#define TI_K3_OSPI_FLASH_WR_DATA_LOWER_DATA_FLD_OFFSET 0

#define TI_K3_OSPI_FLASH_WR_DATA_LOWER_DATA_FLD_SIZE 32

/* FLASH_WR_DATA_UPPER */
#define TI_K3_OSPI_FLASH_WR_DATA_UPPER_DATA_FLD_OFFSET 0

#define TI_K3_OSPI_FLASH_WR_DATA_UPPER_DATA_FLD_SIZE 32

/* POLLING_FLASH_STATUS */
#define TI_K3_OSPI_POLLING_FLASH_STATUS_DEVICE_STATUS_NB_DUMMY_FLD_OFFSET 16
#define TI_K3_OSPI_POLLING_FLASH_STATUS_DEVICE_STATUS_VALID_FLD_OFFSET    8
#define TI_K3_OSPI_POLLING_FLASH_STATUS_DEVICE_STATUS_FLD_OFFSET          0

#define TI_K3_OSPI_POLLING_FLASH_STATUS_DEVICE_STATUS_NB_DUMMY_FLD_SIZE 4
#define TI_K3_OSPI_POLLING_FLASH_STATUS_DEVICE_STATUS_VALID_FLD_SIZE    1
#define TI_K3_OSPI_POLLING_FLASH_STATUS_DEVICE_STATUS_FLD_SIZE          8

/* PHY_CONFIGURATION */
#define TI_K3_OSPI_PHY_CONFIGURATION_RESYNC_FLD_OFFSET        31
#define TI_K3_OSPI_PHY_CONFIGURATION_RESET_FLD_OFFSET         30
#define TI_K3_OSPI_PHY_CONFIGURATION_RX_DLL_BYPASS_FLD_OFFSET 29
#define TI_K3_OSPI_PHY_CONFIGURATION_TX_DLL_DELAY_FLD_OFFSET  16
#define TI_K3_OSPI_PHY_CONFIGURATION_RX_DLL_DELAY_FLD_OFFSET  0

#define TI_K3_OSPI_PHY_CONFIGURATION_RESYNC_FLD_SIZE        1
#define TI_K3_OSPI_PHY_CONFIGURATION_RESET_FLD_SIZE         1
#define TI_K3_OSPI_PHY_CONFIGURATION_RX_DLL_BYPASS_FLD_SIZE 1
#define TI_K3_OSPI_PHY_CONFIGURATION_TX_DLL_DELAY_FLD_SIZE  7
#define TI_K3_OSPI_PHY_CONFIGURATION_RX_DLL_DELAY_FLD_SIZE  7

/* PHY_MASTER_CONTROL */
#define TI_K3_OSPI_PHY_MASTER_CONTROL_LOCK_MODE_FLD_OFFSET             24
#define TI_K3_OSPI_PHY_MASTER_CONTROL_BYPASS_MODE_FLD_OFFSET           23
#define TI_K3_OSPI_PHY_MASTER_CONTROL_PHASE_DETECT_SELECTOR_FLD_OFFSET 20
#define TI_K3_OSPI_PHY_MASTER_CONTROL_NB_INDICATIONS_FLD_OFFSET        16
#define TI_K3_OSPI_PHY_MASTER_CONTROL_INITIAL_DELAY_FLD_OFFSET         0

#define TI_K3_OSPI_PHY_MASTER_CONTROL_LOCK_MODE_FLD_SIZE             1
#define TI_K3_OSPI_PHY_MASTER_CONTROL_BYPASS_MODE_FLD_SIZE           1
#define TI_K3_OSPI_PHY_MASTER_CONTROL_PHASE_DETECT_SELECTOR_FLD_SIZE 3
#define TI_K3_OSPI_PHY_MASTER_CONTROL_NB_INDICATIONS_FLD_SIZE        3
#define TI_K3_OSPI_PHY_MASTER_CONTROL_INITIAL_DELAY_FLD_SIZE         7

/* DLL_OBSERVABLE_LOWER */
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_DLL_LOCK_INC_FLD_OFFSET   24
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_DLL_LOCK_DEC_FLD_OFFSET   16
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_LOOPBACK_LOCK_FLD_OFFSET  15
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_LOCK_VALUE_FLD_OFFSET     8
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_UNLOCK_COUNTER_FLD_OFFSET 3
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_LOCK_MODE_FLD_OFFSET      1
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_DLL_LOCK_FLD_OFFSET       0

#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_DLL_LOCK_INC_FLD_SIZE   8
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_DLL_LOCK_DEC_FLD_SIZE   8
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_LOOPBACK_LOCK_FLD_SIZE  1
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_LOCK_VALUE_FLD_SIZE     7
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_UNLOCK_COUNTER_FLD_SIZE 5
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_LOCK_MODE_FLD_SIZE      2
#define TI_K3_OSPI_DLL_OBSERVABLE_LOWER_DLL_LOCK_FLD_SIZE       1

/* DLL_OBSERVABLE_UPPER */
#define TI_K3_OSPI_DLL_OBSERVABLE_UPPER_TX_DECODER_OUTPUT_FLD_OFFSET 16
#define TI_K3_OSPI_DLL_OBSERVABLE_UPPER_RX_DECODER_OUTPUT_FLD_OFFSET 0

#define TI_K3_OSPI_DLL_OBSERVABLE_UPPER_TX_DECODER_OUTPUT_FLD_SIZE 7
#define TI_K3_OSPI_DLL_OBSERVABLE_UPPER_RX_DECODER_OUTPUT_FLD_SIZE 7

/* OPCODE_EXT_LOWER */
#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_READ_OPCODE_FLD_OFFSET  24
#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_WRITE_OPCODE_FLD_OFFSET 16
#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_POLL_OPCODE_FLD_OFFSET  8
#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_STIG_OPCODE_FLD_OFFSET  0

#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_READ_OPCODE_FLD_SIZE  8
#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_WRITE_OPCODE_FLD_SIZE 8
#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_POLL_OPCODE_FLD_SIZE  8
#define TI_K3_OSPI_OPCODE_EXT_LOWER_EXT_STIG_OPCODE_FLD_SIZE  8

/* OPCODE_EXT_UPPER */
#define TI_K3_OSPI_OPCODE_EXT_UPPER_WEL_OPCODE_FLD_OFFSET     24
#define TI_K3_OSPI_OPCODE_EXT_UPPER_EXT_WEL_OPCODE_FLD_OFFSET 16

#define TI_K3_OSPI_OPCODE_EXT_UPPER_WEL_OPCODE_FLD_SIZE     8
#define TI_K3_OSPI_OPCODE_EXT_UPPER_EXT_WEL_OPCODE_FLD_SIZE 8

#endif /* ZEPHYR_DRIVERS_MSPI_TI_K3_H_*/
