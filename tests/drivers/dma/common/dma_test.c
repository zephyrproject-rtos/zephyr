/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/dma.h>

#include "dma_test.h"

#define DMA_TEST_CHANNELS_DEFINE(inst)                                                             \
	static const uint32_t dma_test_channels_##inst[] =                                         \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, channels),                                 \
			    (DT_INST_PROP(inst, channels)), ({0, 1}));

#define DMA_TEST_DEV_INIT(inst)                                                                    \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, dma)),                                  \
		.channels = dma_test_channels_##inst,                                              \
		.channel_count = ARRAY_SIZE(dma_test_channels_##inst),                             \
		.max_burst_length = DT_INST_PROP_OR(inst, max_burst_length, 0),                    \
	},

DT_INST_FOREACH_STATUS_OKAY(DMA_TEST_CHANNELS_DEFINE)

const struct dma_test_dev dma_test_devs[DMA_TEST_DEV_COUNT] = {
	DT_INST_FOREACH_STATUS_OKAY(DMA_TEST_DEV_INIT)
};

int dma_test_channels_get(const struct dma_test_dev *td, size_t count,
			  struct dma_test_channels *chans)
{
	if (count == 0 || count > DMA_TEST_MAX_CHANNELS) {
		return -EINVAL;
	}

	chans->count = 0;
	chans->allocated = false;

	for (size_t i = 0; i < count; i++) {
		int ch = dma_request_channel(td->dev, NULL);

		if (ch < 0) {
			break;
		}

		chans->id[chans->count++] = (uint32_t)ch;
		chans->allocated = true;
	}

	if (chans->allocated) {
		/* The driver allocates channels; fewer than asked is what it has. */
		return 0;
	}

	for (size_t i = 0; i < count && i < td->channel_count; i++) {
		chans->id[chans->count++] = td->channels[i];
	}

	return 0;
}

void dma_test_channels_put(const struct dma_test_dev *td, struct dma_test_channels *chans)
{
	if (chans->allocated) {
		for (size_t i = 0; i < chans->count; i++) {
			dma_release_channel(td->dev, chans->id[i]);
		}
	}

	chans->count = 0;
	chans->allocated = false;
}
