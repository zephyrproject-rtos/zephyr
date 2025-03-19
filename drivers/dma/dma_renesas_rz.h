/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

/* used to store interrupt and priority for channels */
#define DMA_CHANNEL_DECLARE(n, inst)                                                               \
	{                                                                                          \
		.fsp_ctrl = (transfer_ctrl_t *)&g_transfer_ctrl[n],                                \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.p_info = &g_transfer_info[n],                                     \
				.p_extend = &g_transfer_extend[n],                                 \
			},                                                                         \
		.irq = DT_INST_IRQ_BY_IDX(inst, n, irq),                                           \
		.irq_ipl = DT_INST_IRQ_BY_IDX(inst, n, priority),                                  \
	}

/* Generate an array of DMA channel data structures */
#define DMA_CHANNEL_ARRAY(inst)                                                                    \
	{LISTIFY(DT_INST_PROP(inst, dma_channels), DMA_CHANNEL_DECLARE, (,), inst) }

#define DMA_PRV_CHANNEL(channel) (channel % 8)
#define DMA_PRV_GROUP(channel)   (channel / 8)
