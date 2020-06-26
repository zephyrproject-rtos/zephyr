/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */
#define RELATIVE_FILE "zephyr/dai.c"

#include <sof/common.h>
#include <sof/drivers/interrupt.h>
#include <sof/drivers/ssp.h>
#include <sof/lib/dai.h>
#include <sof/lib/dma.h>
#include <sof/lib/memory.h>
#include <sof/sof.h>
#include <ipc/dai.h>
#include <ipc/stream.h>

static SHARED_DATA struct dai ssp[2] = {
{
	.index = 0,
	.plat_data = {
		.base		= SSP0_BASE,
		.irq		= IRQ_NUM_EXT_SSP0,
		.fifo[SOF_IPC_STREAM_PLAYBACK] = {
			.offset		= SSP0_BASE + SSDR,
			.handshake	= DMA_HANDSHAKE_SSP0_TX,
		},
		.fifo[SOF_IPC_STREAM_CAPTURE] = {
			.offset		= SSP0_BASE + SSDR,
			.handshake	= DMA_HANDSHAKE_SSP0_RX,
		}
	},
	.drv		= &ssp_driver,
},
{
	.index = 1,
	.plat_data = {
		.base		= SSP1_BASE,
		.irq		= IRQ_NUM_EXT_SSP1,
		.fifo[SOF_IPC_STREAM_PLAYBACK] = {
			.offset		= SSP1_BASE + SSDR,
			.handshake	= DMA_HANDSHAKE_SSP1_TX,
		},
		.fifo[SOF_IPC_STREAM_CAPTURE] = {
			.offset		= SSP1_BASE + SSDR,
			.handshake	= DMA_HANDSHAKE_SSP1_RX,
		}
	},
	.drv		= &ssp_driver,
},
};

const struct dai_type_info dti[] = {
	{
		.type = SOF_DAI_INTEL_SSP,
		.dai_array = ssp,
		.num_dais = ARRAY_SIZE(ssp)
	}
};

const struct dai_info lib_dai = {
	.dai_type_array = dti,
	.num_dai_types = ARRAY_SIZE(dti)
};

int dai_init(struct sof *sof)
{
	int i;

	/* initialize spin locks early to enable ref counting */
	for (i = 0; i < ARRAY_SIZE(ssp); i++)
		spinlock_init(&ssp[i].lock);

	platform_shared_commit(ssp, sizeof(*ssp));

	sof->dai_info = &lib_dai;

	return 0;
}
