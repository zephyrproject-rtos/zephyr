/*
 * Copyright 2024 NXP
 *
 * Based on flexspi_nor_config.h, which is:
 * Copyright (c) 2019, MADMACHINE LIMITED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZEPHYR_DRIVERS_MSPI_NXP_FLEXSPI_H__
#define __ZEPHYR_DRIVERS_MSPI_NXP_FLEXSPI_H__

#define FLEXSPI_OP_CMD_SDR        0x01
#define FLEXSPI_OP_CMD_DDR        0x21
#define FLEXSPI_OP_RADDR_SDR      0x02
#define FLEXSPI_OP_RADDR_DDR      0x22
#define FLEXSPI_OP_CADDR_SDR      0x03
#define FLEXSPI_OP_CADDR_DDR      0x23
#define FLEXSPI_OP_MODE1_SDR      0x04
#define FLEXSPI_OP_MODE1_DDR      0x24
#define FLEXSPI_OP_MODE2_SDR      0x05
#define FLEXSPI_OP_MODE2_DDR      0x25
#define FLEXSPI_OP_MODE4_SDR      0x06
#define FLEXSPI_OP_MODE4_DDR      0x26
#define FLEXSPI_OP_MODE8_SDR      0x07
#define FLEXSPI_OP_MODE8_DDR      0x27
#define FLEXSPI_OP_WRITE_SDR      0x08
#define FLEXSPI_OP_WRITE_DDR      0x28
#define FLEXSPI_OP_READ_SDR       0x09
#define FLEXSPI_OP_READ_DDR       0x29
#define FLEXSPI_OP_LEARN_SDR      0x0A
#define FLEXSPI_OP_LEARN_DDR      0x2A
#define FLEXSPI_OP_DATSZ_SDR      0x0B
#define FLEXSPI_OP_DATSZ_DDR      0x2B
#define FLEXSPI_OP_DUMMY_SDR      0x0C
#define FLEXSPI_OP_DUMMY_DDR      0x2C
#define FLEXSPI_OP_DUMMY_RWDS_SDR 0x0D
#define FLEXSPI_OP_DUMMY_RWDS_DDR 0x2D
#define FLEXSPI_OP_JMP_ON_CS      0x1F
#define FLEXSPI_OP_STOP           0

#define FLEXSPI_1PAD 0
#define FLEXSPI_2PAD 1
#define FLEXSPI_4PAD 2
#define FLEXSPI_8PAD 3

#define FLEXSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)                                          \
	(FLEXSPI_LUT_OPERAND0(op0) | FLEXSPI_LUT_NUM_PADS0(pad0) | FLEXSPI_LUT_OPCODE0(cmd0) |     \
	 FLEXSPI_LUT_OPERAND1(op1) | FLEXSPI_LUT_NUM_PADS1(pad1) | FLEXSPI_LUT_OPCODE1(cmd1))

#endif /* __ZEPHYR_DRIVERS_MSPI_NXP_FLEXSPI_H__ */
