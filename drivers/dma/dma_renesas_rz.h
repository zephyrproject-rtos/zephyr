/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

/* Used to store interrupt and priority for channels */
#define RZ_DMA_CHANNEL_DECLARE(n, inst)                                                            \
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
#define RZ_DMA_CHANNEL_DATA_ARRAY(inst)                                                            \
	{LISTIFY(DT_INST_PROP(inst, dma_channels), RZ_DMA_CHANNEL_DECLARE, (,), inst)}

#ifdef CONFIG_CPU_CORTEX_A
#define RZ_DMA_EXTERN_INFO_DECLARATION(size) static dma_extended_info_t g_dma_extended_info[size];

#define RZ_DMA_EXTEND_INFO_DECLARE(n, inst)                                                        \
	{                                                                                          \
		.p_extend_info = &g_dma_extended_info[n],                                          \
	}

#define RZ_DMA_TRANSFER_INFO_ARRAY(inst)                                                           \
	{LISTIFY(DT_INST_PROP(inst, dma_channels), RZ_DMA_EXTEND_INFO_DECLARE, (,), inst)}

#else /* CONFIG_CPU_CORTEX_M || CONFIG_CPU_AARCH32_CORTEX_R */
#define RZ_DMA_EXTERN_INFO_DECLARATION(size)

#define RZ_DMA_TRANSFER_INFO_ARRAY(inst)                                                           \
	{                                                                                          \
	}
#endif /* CONFIG_CPU_CORTEX_A */

#if defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A)
#define RZ_DMA_DATA_STRUCT_GET_ERR_IRQ(inst, err_name)                                             \
	.err_irq = DT_INST_IRQ_BY_NAME(inst, err_name, irq),
#else /* CONFIG_CPU_AARCH32_CORTEX_R */
#define RZ_DMA_DATA_STRUCT_GET_ERR_IRQ(inst, err_name)
#endif /* defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_CPU_CORTEX_A) */

#define RZ_DMA_PRV_CHANNEL(channel) (channel % 8)
#define RZ_DMA_PRV_GROUP(channel)   (channel / 8)
