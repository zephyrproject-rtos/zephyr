/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Intel ADSP HDA DMA (Stream) driver
 *
 * HDA is effectively, from the DSP, a ringbuffer (fifo) where the read
 * and write positions are maintained by the hardware and the software may
 * commit read/writes by writing to another register (DGFPBI) the length of
 * the read or write.
 *
 * It's important that the software knows the position in the ringbuffer to read
 * or write from. It's also important that the buffer be placed in the correct
 * memory region and aligned to 128 bytes. Lastly it's important the host and
 * dsp coordinate the order in which operations takes place. Doing all that
 * HDA streams are a fantastic bit of hardware and do their job well.
 *
 * There are 4 types of streams, with a set of each available to be used to
 * communicate to or from the Host or Link. Each stream set is uni directional.
 */

#include <zephyr/drivers/dma.h>

#include "dma_intel_adsp_hda.h"
#include <intel_adsp_hda.h>

int intel_adsp_hda_dma_host_in_config(const struct device *dev,
				       uint32_t channel,
				       struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA host in supports "
		 "MEMORY_TO_HOST");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->source_address);
	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);

	if (res == 0) {
		*DGMBS(cfg->base, cfg->regblock_size, channel) =
			blk_cfg->block_size & HDA_ALIGN_MASK;

		intel_adsp_hda_set_sample_container_size(cfg->base, cfg->regblock_size, channel,
							 dma_cfg->source_data_size);
	}

	return res;
}


int intel_adsp_hda_dma_host_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA host out supports "
		 "HOST_TO_MEMORY");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->dest_address);

	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);

	if (res == 0) {
		*DGMBS(cfg->base, cfg->regblock_size, channel) =
			blk_cfg->block_size & HDA_ALIGN_MASK;

		intel_adsp_hda_set_sample_container_size(cfg->base, cfg->regblock_size, channel,
							 dma_cfg->dest_data_size);
	}

	return res;
}

int intel_adsp_hda_dma_link_in_config(const struct device *dev,
				       uint32_t channel,
				       struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA link in supports "
		 "PERIPHERAL_TO_MEMORY");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->dest_address);
	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);
	if (res == 0) {
		intel_adsp_hda_set_sample_container_size(cfg->base, cfg->regblock_size, channel,
							 dma_cfg->dest_data_size);
	}

	return res;
}


int intel_adsp_hda_dma_link_out_config(const struct device *dev,
					uint32_t channel,
					struct dma_config *dma_cfg)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	struct dma_block_config *blk_cfg;
	uint8_t *buf;
	int res;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");
	__ASSERT(dma_cfg->block_count == 1,
		 "HDA does not support scatter gather or chained "
		 "block transfers.");
	__ASSERT(dma_cfg->channel_direction == cfg->direction,
		 "Unexpected channel direction, HDA link out supports "
		 "MEMORY_TO_PERIPHERAL");

	blk_cfg = dma_cfg->head_block;
	buf = (uint8_t *)(uintptr_t)(blk_cfg->source_address);

	res = intel_adsp_hda_set_buffer(cfg->base, cfg->regblock_size, channel, buf,
				  blk_cfg->block_size);
	if (res == 0) {
		intel_adsp_hda_set_sample_container_size(cfg->base, cfg->regblock_size, channel,
							 dma_cfg->source_data_size);
	}

	return res;
}


int intel_adsp_hda_dma_link_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	intel_adsp_hda_link_commit(cfg->base, cfg->regblock_size, channel, size);

	return 0;
}

int intel_adsp_hda_dma_host_reload(const struct device *dev, uint32_t channel,
				    uint32_t src, uint32_t dst, size_t size)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

#if CONFIG_DMA_INTEL_ADSP_HDA_TIMING_L1_EXIT
	const size_t buf_size = intel_adsp_hda_get_buffer_size(cfg->base, cfg->regblock_size,
							       channel);

	if (!buf_size) {
		return -EIO;
	}

	intel_adsp_force_dmi_l0_state();
	switch (cfg->direction) {
	case HOST_TO_MEMORY:
		; /* Only statements can be labeled in C, a declaration is not valid */
		const uint32_t rp = *DGBRP(cfg->base, cfg->regblock_size, channel);
		const uint32_t next_rp = (rp + INTEL_HDA_MIN_FPI_INCREMENT_FOR_INTERRUPT) %
			buf_size;

		intel_adsp_hda_set_buffer_segment_ptr(cfg->base, cfg->regblock_size,
						      channel, next_rp);
		intel_adsp_hda_enable_buffer_interrupt(cfg->base, cfg->regblock_size, channel);
		break;
	case MEMORY_TO_HOST:
		;
		const uint32_t wp = *DGBWP(cfg->base, cfg->regblock_size, channel);
		const uint32_t next_wp = (wp + INTEL_HDA_MIN_FPI_INCREMENT_FOR_INTERRUPT) %
			buf_size;

		intel_adsp_hda_set_buffer_segment_ptr(cfg->base, cfg->regblock_size,
						      channel, next_wp);
		intel_adsp_hda_enable_buffer_interrupt(cfg->base, cfg->regblock_size, channel);
		break;
	default:
		break;
	}
#endif

	intel_adsp_hda_host_commit(cfg->base, cfg->regblock_size, channel, size);

	return 0;
}

int intel_adsp_hda_dma_status(const struct device *dev, uint32_t channel,
	struct dma_status *stat)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	uint32_t llp_l = 0;
	uint32_t llp_u = 0;
	bool xrun_det;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	uint32_t unused = intel_adsp_hda_unused(cfg->base, cfg->regblock_size, channel);
	uint32_t used = *DGBS(cfg->base, cfg->regblock_size, channel) - unused;

	stat->dir = cfg->direction;
	stat->busy = *DGCS(cfg->base, cfg->regblock_size, channel) & DGCS_GBUSY;
	stat->write_position = *DGBWP(cfg->base, cfg->regblock_size, channel);
	stat->read_position = *DGBRP(cfg->base, cfg->regblock_size, channel);
	stat->pending_length = used;
	stat->free = unused;

#if CONFIG_SOC_INTEL_ACE20_LNL || CONFIG_SOC_INTEL_ACE30
	/* Linear Link Position via HDA-DMA is only supported on ACE2 or newer */
	if (cfg->direction == MEMORY_TO_PERIPHERAL || cfg->direction == PERIPHERAL_TO_MEMORY) {
		uint32_t tmp;

		tmp = *DGLLLPL(cfg->base, cfg->regblock_size, channel);
		llp_u = *DGLLLPU(cfg->base, cfg->regblock_size, channel);
		llp_l = *DGLLLPL(cfg->base, cfg->regblock_size, channel);
		if (tmp > llp_l) {
			/* re-read the LLPU value, as LLPL just wrapped */
			llp_u = *DGLLLPU(cfg->base, cfg->regblock_size, channel);
		}
	}
#endif
	stat->total_copied = ((uint64_t)llp_u << 32) | llp_l;

	switch (cfg->direction) {
	case MEMORY_TO_PERIPHERAL:
		xrun_det = intel_adsp_hda_is_buffer_underrun(cfg->base, cfg->regblock_size,
							     channel);
		if (xrun_det) {
			intel_adsp_hda_underrun_clear(cfg->base, cfg->regblock_size, channel);
			return -EPIPE;
		}
		break;
	case PERIPHERAL_TO_MEMORY:
		xrun_det = intel_adsp_hda_is_buffer_overrun(cfg->base, cfg->regblock_size,
							    channel);
		if (xrun_det) {
			intel_adsp_hda_overrun_clear(cfg->base, cfg->regblock_size, channel);
			return -EPIPE;
		}
		break;
	default:
		break;
	}

	return 0;
}

bool intel_adsp_hda_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t requested_channel;

	if (!filter_param) {
		return true;
	}

	requested_channel = *(uint32_t *)filter_param;

	if (channel == requested_channel) {
		return true;
	}

	return false;
}

int intel_adsp_hda_dma_start(const struct device *dev, uint32_t channel)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;
	uint32_t size;
	bool set_fifordy;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

#if CONFIG_PM_DEVICE_RUNTIME
	bool first_use = false;
	enum pm_device_state state;

	/* If the device is used for the first time, we need to let the power domain know that
	 * we want to use it.
	 */
	if (pm_device_state_get(dev, &state) == 0) {
		first_use = state != PM_DEVICE_STATE_ACTIVE;
		if (first_use) {
			int ret = pm_device_runtime_get(dev);

			if (ret < 0) {
				return ret;
			}
		}
	}
#endif

	if (intel_adsp_hda_is_enabled(cfg->base, cfg->regblock_size, channel)) {
		return 0;
	}

	set_fifordy = (cfg->direction == HOST_TO_MEMORY || cfg->direction == MEMORY_TO_HOST);
	intel_adsp_hda_enable(cfg->base, cfg->regblock_size, channel, set_fifordy);

	if (cfg->direction == MEMORY_TO_PERIPHERAL) {
		size = intel_adsp_hda_get_buffer_size(cfg->base, cfg->regblock_size, channel);
		intel_adsp_hda_link_commit(cfg->base, cfg->regblock_size, channel, size);
	}

#if CONFIG_PM_DEVICE_RUNTIME
	if (!first_use) {
		return pm_device_runtime_get(dev);
	}
#endif
	return 0;
}

int intel_adsp_hda_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	__ASSERT(channel < cfg->dma_channels, "Channel does not exist");

	if (!intel_adsp_hda_is_enabled(cfg->base, cfg->regblock_size, channel)) {
		return 0;
	}

	intel_adsp_hda_disable(cfg->base, cfg->regblock_size, channel);

	if (!WAIT_FOR(!intel_adsp_hda_is_enabled(cfg->base, cfg->regblock_size, channel), 1000,
			k_busy_wait(1))) {
		return -EBUSY;
	}

	return pm_device_runtime_put(dev);
}

static void intel_adsp_hda_channels_init(const struct device *dev)
{
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	for (uint32_t i = 0; i < cfg->dma_channels; i++) {
		intel_adsp_hda_init(cfg->base, cfg->regblock_size, i);

		if (intel_adsp_hda_is_enabled(cfg->base, cfg->regblock_size, i)) {
			uint32_t size;

			size = intel_adsp_hda_get_buffer_size(cfg->base, cfg->regblock_size, i);
			intel_adsp_hda_disable(cfg->base, cfg->regblock_size, i);
			intel_adsp_hda_link_commit(cfg->base, cfg->regblock_size, i, size);
		}
	}

#if CONFIG_DMA_INTEL_ADSP_HDA_TIMING_L1_EXIT
	/* Configure interrupts */
	if (cfg->irq_config) {
		cfg->irq_config();
	}
#endif
}

int intel_adsp_hda_dma_init(const struct device *dev)
{
	struct intel_adsp_hda_dma_data *data = dev->data;
	const struct intel_adsp_hda_dma_cfg *const cfg = dev->config;

	data->ctx.dma_channels = cfg->dma_channels;
	data->ctx.atomic = data->channels_atomic;
	data->ctx.magic = DMA_MAGIC;
#ifdef CONFIG_PM_DEVICE_RUNTIME
	if (pm_device_on_power_domain(dev)) {
		pm_device_init_off(dev);
	} else {
		intel_adsp_hda_channels_init(dev);
		pm_device_init_suspended(dev);
	}

	return pm_device_runtime_enable(dev);
#else
	intel_adsp_hda_channels_init(dev);
	return 0;
#endif
}

int intel_adsp_hda_dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = DMA_BUF_ADDR_ALIGNMENT(
				DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_hda_link_out));
		break;
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = DMA_BUF_SIZE_ALIGNMENT(
				DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_hda_link_out));
		break;
	case DMA_ATTR_COPY_ALIGNMENT:
		*value = DMA_COPY_ALIGNMENT(DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_hda_link_out));
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = 1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
int intel_adsp_hda_dma_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		intel_adsp_hda_channels_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),

void intel_adsp_hda_dma_isr(void)
{
#if CONFIG_DMA_INTEL_ADSP_HDA_TIMING_L1_EXIT
	struct dma_context *dma_ctx;
	const struct intel_adsp_hda_dma_cfg *cfg;
	bool triggered_interrupts = false;
	int i, j;
	int expected_interrupts = 0;
	const struct device *host_dev[] = {
#if CONFIG_DMA_INTEL_ADSP_HDA_HOST_OUT
		DT_FOREACH_STATUS_OKAY(intel_adsp_hda_host_out, DEVICE_DT_GET_AND_COMMA)
#endif
#if CONFIG_DMA_INTEL_ADSP_HDA_HOST_IN
		DT_FOREACH_STATUS_OKAY(intel_adsp_hda_host_in, DEVICE_DT_GET_AND_COMMA)
#endif
	};

	/*
	 * To initiate transfer, DSP must be in L0 state. Once the transfer is started, DSP can go
	 * to the low power L1 state, and the transfer will be able to continue and finish in L1
	 * state. Interrupts are configured to trigger after the first 32 bytes of data arrive.
	 * Once such an interrupt arrives, the transfer has already started. If all expected
	 * transfers have started, it is safe to allow the low power L1 state.
	 */

	for (i = 0; i < ARRAY_SIZE(host_dev); i++) {
		dma_ctx = (struct dma_context *)host_dev[i]->data;
		cfg = host_dev[i]->config;

		for (j = 0; j < dma_ctx->dma_channels; j++) {
			if (!atomic_test_bit(dma_ctx->atomic, j))
				continue;

			if (!intel_adsp_hda_is_buffer_interrupt_enabled(cfg->base,
									cfg->regblock_size, j))
				continue;

			if (intel_adsp_hda_check_buffer_interrupt(cfg->base,
								  cfg->regblock_size, j)) {
				triggered_interrupts = true;
				intel_adsp_hda_disable_buffer_interrupt(cfg->base,
									cfg->regblock_size, j);
				intel_adsp_hda_clear_buffer_interrupt(cfg->base,
								      cfg->regblock_size, j);
			} else {
				expected_interrupts++;
			}
		}
	}

	/*
	 * Allow entering low power L1 state only after all enabled interrupts arrived, i.e.,
	 * transfers started on all channels.
	 */
	if (triggered_interrupts && expected_interrupts == 0) {
		intel_adsp_allow_dmi_l1_state();
	}
#endif
}
