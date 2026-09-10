/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMM_WIDGET_PMC_IPC_H
#define COMM_WIDGET_PMC_IPC_H


#define CW_PMC_MAILBOX3_DATA0_ADDRESS		0x68
#define CW_PMC_MAILBOX3_DATA1_ADDRESS		0x6c
#define CW_PMC_MAILBOX3_DATA2_ADDRESS		0x70
#define CW_PMC_MAILBOX3_DATA3_ADDRESS		0x74
#define CW_PMC_MAILBOX3_INTERFACE_ADDRESS	0x78
#define CW_PMC_DESTID_VALUE			0xc8


/*
 * The requesting agent will write the PMC command op-code into this field.
 */
#define CW_PMC_IPC_OP_CODE			GENMASK(7, 0)

/*
 * When the PMC has completed the API command, the PMC will write a completion code (CC) back into
 * this field.
 */
#define CW_PMC_IPC_CC				GENMASK(7, 0)

/*
 * Some commands require additional information which is passed into this 8 bit field.
 */
#define CW_PMC_IPC_PARAM1			GENMASK(15, 8)

/*
 * Some commands require additional information which is passed into this 8 bit field.
 */
#define CW_PMC_IPC_PARAM2			GENMASK(23, 16)

/*
 * Some commands require additional information which is passed into this 4 bit field.
 */
#define CW_PMC_IPC_PARAM3			GENMASK(27, 24)

/*
 * Reserved
 */
#define CW_PMC_IPC_RSVD				GENMASK(30, 28)

/*
 * busy - The run/busy bit can only be set by the requesting agent and can only be cleared by the
 * responding agent. When this bit is set it will prompt the PMC to execute the command placed in
 * the mailbox. The PMC will clear this flag after the command has been processed and a completion
 * code has been written back into the COMMAND field and any data requested has been written into
 * the DATA field.
 */
#define CW_PMC_IPC_BUSY				BIT(31)


/* PMC operation codes */

/*
 * No operation - PMC FW will clear the run / busy bit and return a success response
 */
#define CW_PMC_OPC_NOP				0x0

/*
 * Version - The IPC command to obtain information about current HAL version.
 *	param_1 - Pass back the PMC ACE protocol version - the initial version is 0.
 */
#define CW_PMC_OPC_VERSION			0x1

/*
 * SRAM config - Any FW allocating HP-SRAM is expected to report allocated number of banks.
 * HP-SRAM reporting is defined as 10-bit field span across Parameter 1 and 2, unit is 32 KB.
 */
#define CW_PMC_OPC_SRAM_CONFIG			0x2

/*
 * Number of allocated HP-SRAM of banks, unit is 32 KB.
 */
#define CW_PMC_IPC_SRAM_USED_BANKS		GENMASK(17, 8)

/*
 * Some commands require additional information which is passed into this 8 bit field.
 */
#define CW_PMC_IPC_SRAM_RESERVED		GENMASK(30, 18)

#endif
