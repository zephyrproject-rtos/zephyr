/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief This file is the mailbox configuration for AM261x SOC series
 * This file contains a constant array of mailbox configurations
 */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/mbox/ti_am26x_mbox.h>
// #include "ti_am26x_mbox.h"

/*
 * MSS controller mailbox register definitions
 * see ipc_notify_v1_cfg.c in TI SDK
 */
/* R5FSS0_0 mailbox registers */
#define R5FSS0_0_MBOX_WRITE_DONE	0x00004000U
#define R5FSS0_0_MBOX_READ_REQ		0x00004004U

/* R5FSS0_1 mailbox registers */
#define R5FSS0_1_MBOX_WRITE_DONE	0x00008000U
#define R5FSS0_1_MBOX_READ_REQ		0x00008004U

/* Core bit positions within mailbox registers */
#define R5FSS0_0_MBOX_PROC_BIT_POS	0U
#define R5FSS0_1_MBOX_PROC_BIT_POS	4U

#define R5FSS0_0_TO_R5FSS0_1_SW_ADDR AM26X_MBOX_GET_SW_Q_ADDR(AM26X_MBOX_GET_CH_NUM(0, 1))
#define R5FSS0_1_TO_R5FSS0_0_SW_ADDR AM26X_MBOX_GET_SW_Q_ADDR(AM26X_MBOX_GET_CH_NUM(1, 0))

#define AM26X_MBOX_MEM_SIZE 16 * 1024U /* 16KB of Mailbox SRAM */
#define AM26X_MBOX_RAM_BASE 0x72000000U

#define AM26X_MBOX_MAX_QUEUE_SIZE	32

#define AM26X_MBOX_GET_SW_Q_ADDR(ch_number)                                                        \
	(AM26X_MBOX_RAM_BASE + AM26X_MBOX_MEM_SIZE -                                               \
	 (AM26X_MBOX_GET_SW_Q_IDX(ch_number) * AM26X_MBOX_MAX_QUEUE_SIZE))

am26x_mbox_cfg am26x_mbox_static_cfg[CONFIG_MAX_CPUS_NUM][CONFIG_MAX_CPUS_NUM] = {
	{
		/* Channel 0: R5FSS0_0 to R5FSS0_0 */
		{
			.write_done_offset = R5FSS0_0_MBOX_WRITE_DONE,
			.read_req_offset = R5FSS0_0_MBOX_READ_REQ,
			.intr_bit_pos = R5FSS0_0_MBOX_PROC_BIT_POS,
			.swQ_addr = 0,
		},
		/* Channel 1: R5FSS0_0 to R5FSS0_1 */
		{
			.write_done_offset = R5FSS0_0_MBOX_WRITE_DONE,
			.read_req_offset = R5FSS0_0_MBOX_READ_REQ,
			.intr_bit_pos = R5FSS0_1_MBOX_PROC_BIT_POS,
			.swQ_addr = R5FSS0_0_TO_R5FSS0_1_SW_ADDR,
		},
	},
	{
		/* Channel 0: R5FSS0_1 to R5FSS0_0 */
		{
			.write_done_offset = R5FSS0_1_MBOX_WRITE_DONE,
			.read_req_offset = R5FSS0_1_MBOX_READ_REQ,
			.intr_bit_pos = R5FSS0_0_MBOX_PROC_BIT_POS,
			.swQ_addr = R5FSS0_1_TO_R5FSS0_0_SW_ADDR,
		},
		/* Channel 1: R5FSS0_1 to R5FSS0_1 */
		{
			.write_done_offset = R5FSS0_1_MBOX_WRITE_DONE,
			.read_req_offset = R5FSS0_1_MBOX_READ_REQ,
			.intr_bit_pos = R5FSS0_1_MBOX_PROC_BIT_POS,
			.swQ_addr = 0,
		},
	},
};
