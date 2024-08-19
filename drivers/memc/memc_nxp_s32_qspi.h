/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/types.h>

#define NXP_QSPI_LUT_OPRND_MASK GENMASK(7, 0)
#define NXP_QSPI_LUT_OPRND(v)   FIELD_PREP(NXP_QSPI_LUT_OPRND_MASK, (v))
#define NXP_QSPI_LUT_PAD_MASK   GENMASK(9, 8)
#define NXP_QSPI_LUT_PAD(v)     FIELD_PREP(NXP_QSPI_LUT_PAD_MASK, (v))
#define NXP_QSPI_LUT_INSTR_MASK GENMASK(15, 10)
#define NXP_QSPI_LUT_INSTR(v)   FIELD_PREP(NXP_QSPI_LUT_INSTR_MASK, (v))

/** Number of 16-bit LUT commands that make up a sequence */
#define NXP_QSPI_LUT_MAX_CMD 10U

/**
 * @brief Build a Look-up Table (LUT) entry for the programmable sequence engine
 *
 * @param inst Instruction
 * @param pads Pad information
 * @param op   Operand
 */
#define NXP_QSPI_LUT_CMD(inst, pad, op)                                                            \
	((uint16_t)((uint16_t)NXP_QSPI_LUT_INSTR(inst) | (uint16_t)NXP_QSPI_LUT_PAD(pad) |         \
		    (uint16_t)NXP_QSPI_LUT_OPRND(op)))

/** LUT sequence of commands */
typedef uint16_t nxp_qspi_lut_seq_t[NXP_QSPI_LUT_MAX_CMD];

/** Instruction for the programmable sequence engine */
enum memc_nxp_qspi_lut_instr {
	/** End of sequence */
	NXP_QSPI_LUT_INSTR_STOP = 0U,
	/** Command */
	NXP_QSPI_LUT_INSTR_CMD = 1U,
	/** Address */
	NXP_QSPI_LUT_INSTR_ADDR = 2U,
	/** Dummy cycles */
	NXP_QSPI_LUT_INSTR_DUMMY = 3U,
	/** 8-bit mode */
	NXP_QSPI_LUT_INSTR_MODE = 4U,
	/** 2-bit mode */
	NXP_QSPI_LUT_INSTR_MODE2 = 5U,
	/** 4-bit mode */
	NXP_QSPI_LUT_INSTR_MODE4 = 6U,
	/** Read data */
	NXP_QSPI_LUT_INSTR_READ = 7U,
	/** Write data */
	NXP_QSPI_LUT_INSTR_WRITE = 8U,
	/** Jump on chip select deassert and stop */
	NXP_QSPI_LUT_INSTR_JMP_ON_CS = 9U,
	/** Address - DDR mode */
	NXP_QSPI_LUT_INSTR_ADDR_DDR = 10U,
	/** 8-bit mode - DDR mode */
	NXP_QSPI_LUT_INSTR_MODE_DDR = 11U,
	/** 2-bit mode - DDR mode */
	NXP_QSPI_LUT_INSTR_MODE2_DDR = 12U,
	/** 4-bit mode - DDR mode */
	NXP_QSPI_LUT_INSTR_MODE4_DDR = 13U,
	/** Read data - DDR mode */
	NXP_QSPI_LUT_INSTR_READ_DDR = 14U,
	/** Write data - DDR mode */
	NXP_QSPI_LUT_INSTR_WRITE_DDR = 15U,
	/** Data learning pattern */
	NXP_QSPI_LUT_INSTR_DATA_LEARN = 16U,
	/** DDR command */
	NXP_QSPI_LUT_INSTR_CMD_DDR = 17U,
	/** Column address */
	NXP_QSPI_LUT_INSTR_CADDR = 18U,
	/** Column address - DDR mode */
	NXP_QSPI_LUT_INSTR_CADDR_DDR = 19U,
	/** Jump on chip select deassert and continue */
	NXP_QSPI_LUT_INSTR_JMP_TO_SEQ = 20U,
};

/** Pad information for the programmable sequence engine */
enum memc_nxp_qspi_lut_pad {
	/** 1 Pad */
	NXP_QSPI_LUT_PADS_1 = 0U,
	/** 2 Pads */
	NXP_QSPI_LUT_PADS_2 = 1U,
	/** 4 Pads */
	NXP_QSPI_LUT_PADS_4 = 2U,
	/** 8 Pads */
	NXP_QSPI_LUT_PADS_8 = 3U,
};

/** Port corresponding to the chip-select line connected to the external memory */
enum memc_nxp_qspi_port {
	/** Access memory on port A1 */
	NXP_QSPI_PORT_A1 = 0U,
	/** Access memory on port A2 */
	NXP_QSPI_PORT_A2 = 1U,
	/** Access memory on port B1 */
	NXP_QSPI_PORT_B1 = 2U,
	/** Access memory on port B2 */
	NXP_QSPI_PORT_B2 = 3U,
};

/** Transfer command type */
enum memc_nxp_qspi_cmd {
	/* Command, no data is exchanged with the memory device */
	NXP_QSPI_COMMAND,
	/* Read, receive data from the memory device */
	NXP_QSPI_READ,
	/* Write, transmit data to the memory device */
	NXP_QSPI_WRITE,
};

/** Transfer configuration */
struct memc_nxp_qspi_config {
	/** Pointer to a virtual LUT containing the sequence to be executed */
	nxp_qspi_lut_seq_t *lut_seq;
	/** Port where the memory is connected */
	enum memc_nxp_qspi_port port;
	/** Type of operation to be performed */
	enum memc_nxp_qspi_cmd cmd;
	/** Memory address (offset) */
	uint32_t addr;
	/** Pointer to buffer containing the data to be transmitted or received */
	uint8_t *data;
	/** Size of the data to be transmitted or received */
	size_t size;
	/** Memory alignment of the device */
	uint8_t alignment;
};

/**
 * @brief Perform a transfer from or to the memory device
 *
 * @param[in]     dev    Pointer to the device structure for the controller
 * @param[in,out] config Transfer configuration
 */
int memc_nxp_qspi_transfer(const struct device *dev, struct memc_nxp_qspi_config *config);

/**
 * @brief Get status of the controller
 *
 * @param[in]     dev      Pointer to the device structure for the controller
 *
 * @retval 0 if the controller is idle and no errors are reported
 * @retval -EIO if the controller is idle and error flags are reported
 * @retval -EBUSY if a transfer is in progress
 */
int memc_nxp_qspi_get_status(const struct device *dev);
