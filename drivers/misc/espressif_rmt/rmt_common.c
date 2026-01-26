/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/lock.h>
#include "rmt_private.h"
#include "clk_ctrl_os.h"
#include "soc/rtc.h"
#include "soc/rmt_periph.h"
#include "hal/rmt_ll.h"
#include "esp_private/periph_ctrl.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espressif_rmt_common, CONFIG_ESPRESSIF_RMT_LOG_LEVEL);

typedef struct rmt_platform_t {
	/* !< Platform level mutex lock */
	unsigned int key;
	/* !< Array of RMT group instances */
	rmt_group_t *groups[SOC_RMT_GROUPS];
	/* !< Reference count used to protect group install/uninstall */
	int group_ref_counts[SOC_RMT_GROUPS];
} rmt_platform_t;

static rmt_platform_t s_platform; /* singleton platform */

rmt_group_t *rmt_acquire_group_handle(int group_id)
{
	bool new_group = false;
	rmt_group_t *group = NULL;

	/* prevent install rmt group concurrently */
	s_platform.key = irq_lock();
	if (!s_platform.groups[group_id]) {
		group = heap_caps_calloc(1, sizeof(rmt_group_t), RMT_MEM_ALLOC_CAPS);
		if (group) {
			new_group = true;
			s_platform.groups[group_id] = group;
			group->group_id = group_id;
			/* initial occupy_mask: 1111...100...0 */
			group->occupy_mask = UINT32_MAX & ~((1 << SOC_RMT_CHANNELS_PER_GROUP) - 1);
			/*
			 * group clock won't be configured at this stage,
			 * it will be set when allocate the first channel
			 */
			group->clk_src = 0;
			/* enable APB access RMT registers */
			periph_module_enable(rmt_periph_signals.groups[group_id].module);
			periph_module_reset(rmt_periph_signals.groups[group_id].module);
			/*
			 * "uninitialize" group intr_priority
			 * read comments in `rmt_new_tx_channel()` for detail
			 */
			group->intr_priority = RMT_GROUP_INTR_PRIORITY_UNINITALIZED;
			/* hal layer initialize */
			rmt_hal_init(&group->hal);
		}
	} else {
		/* group already install */
		group = s_platform.groups[group_id];
	}
	if (group) {
		/*
		 * someone acquired the group handle means we have a new object that refer to
		 * this group
		 */
		s_platform.group_ref_counts[group_id]++;
	}
	irq_unlock(s_platform.key);

	if (new_group) {
		LOG_DBG("new group(%d) at %p, occupy=%"PRIx32, group_id, group, group->occupy_mask);
	}
	return group;
}

void rmt_release_group_handle(rmt_group_t *group)
{
	rmt_clock_source_t clk_src = group->clk_src;
	bool do_deinitialize = false;

	s_platform.key = irq_lock();
	s_platform.group_ref_counts[group->group_id]--;
	if (s_platform.group_ref_counts[group->group_id] == 0) {
		do_deinitialize = true;
		s_platform.groups[group->group_id] = NULL;
		/* hal layer deinitialize */
		rmt_hal_deinit(&group->hal);
		periph_module_disable(rmt_periph_signals.groups[group->group_id].module);
		free(group);
	}
	irq_unlock(s_platform.key);

	switch (clk_src) {
#if SOC_RMT_SUPPORT_RC_FAST
	case RMT_CLK_SRC_RC_FAST:
		periph_rtc_dig_clk8m_disable();
		break;
#endif
	default:
		break;
	}

	if (do_deinitialize) {
		LOG_DBG("del group(%d)", group->group_id);
	}
}

int rmt_select_periph_clock(rmt_channel_handle_t channel, rmt_clock_source_t clk_src)
{
	esp_err_t ret;
	rmt_group_t *group = channel->group;
	uint32_t periph_src_clk_hz = 0;
	bool clock_selection_conflict = false;
	k_spinlock_key_t key;

	/*
	 * check if we need to update the group clock source,
	 * group clock source is shared by all channels
	 */
	key = k_spin_lock(&group->spinlock);
	if (group->clk_src == 0) {
		group->clk_src = clk_src;
	} else {
		clock_selection_conflict = (group->clk_src != clk_src);
	}
	k_spin_unlock(&group->spinlock, key);
	if (clock_selection_conflict) {
		LOG_ERR("Group clock conflict, already is %d but attempt to %d", group->clk_src,
			clk_src);
		return -EINVAL;
	}
	/*
	 * TODO: [clk_tree] to use a generic clock enable/disable
	 * or acquire/release function for all clock source
	 */
#if SOC_RMT_SUPPORT_RC_FAST
	if (clk_src == RMT_CLK_SRC_RC_FAST) {
		/*
		 * RC_FAST clock is not enabled automatically on start up,
		 * we enable it here manually.
		 * Note there's a ref count in the enable/disable function,
		 * we must call them in pair in the driver.
		 */
		periph_rtc_dig_clk8m_enable();
	}
#endif

	/* get clock source frequency */
	ret = esp_clk_tree_src_get_freq_hz((soc_module_clk_t)clk_src,
		ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &periph_src_clk_hz);
	if (ret) {
		LOG_ERR("Reading clock source frequency failed");
		return -ENODEV;
	}

#if CONFIG_ESPRESSIF_RMT_PM
	/* if DMA is not used, we're using CPU to push the data to the RMT FIFO.
	 * if the CPU frequency goes down, the transfer+encoding scheme could be unstable because
	 * CPU can't fill the data in time so, choose ESP_PM_CPU_FREQ_MAX lock for non-dma mode
	 * otherwise, chose lock type based on the clock source
	 */
#if SOC_RMT_SUPPORT_DMA
	esp_pm_lock_type_t pm_lock_type =
		channel->dma_dev ? ESP_PM_NO_LIGHT_SLEEP : ESP_PM_CPU_FREQ_MAX;
#else
	esp_pm_lock_type_t pm_lock_type = ESP_PM_CPU_FREQ_MAX;
#endif

#if SOC_RMT_SUPPORT_APB
	if (clk_src == RMT_CLK_SRC_APB) {
		/* APB clock frequency can be changed during DFS */
		pm_lock_type = ESP_PM_APB_FREQ_MAX;
	}
#endif
	snprintf(channel->pm_lock_name, RMT_PM_LOCK_NAME_LEN_MAX, "rmt_%d_%d", group->group_id,
		channel->channel_id);
	ret = esp_pm_lock_create(pm_lock_type, 0, channel->pm_lock_name, &channel->pm_lock);
	if (ret) {
		LOG_ERR("Create PM lock failed");
		return -ENODEV;
	}
#endif

	/* no division for group clock source, to achieve highest resolution */
	rmt_ll_set_group_clock_src(group->hal.regs, channel->channel_id, clk_src, 1, 1, 0);
	group->resolution_hz = periph_src_clk_hz;
	LOG_DBG("group clock resolution:%"PRIu32, group->resolution_hz);

	return 0;
}

int rmt_apply_carrier(rmt_channel_handle_t channel, const rmt_carrier_config_t *config)
{
	/* specially, we allow config to be NULL, means to disable the carrier submodule */
	if (!channel) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	return channel->set_carrier_action(channel, config);
}

int rmt_del_channel(rmt_channel_handle_t channel)
{
	if (!channel) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	return channel->del(channel);
}

int rmt_enable(rmt_channel_handle_t channel)
{
	if (!channel) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	return channel->enable(channel);
}

int rmt_disable(rmt_channel_handle_t channel)
{
	if (!channel) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	return channel->disable(channel);
}

bool rmt_set_intr_priority_to_group(rmt_group_t *group, int intr_priority)
{
	bool priority_conflict = false;
	k_spinlock_key_t key;

	key = k_spin_lock(&group->spinlock);
	if (group->intr_priority == RMT_GROUP_INTR_PRIORITY_UNINITALIZED) {
		/* intr_priority never allocated, accept user's value unconditionally */
		/* intr_priority could only be set once here */
		group->intr_priority = intr_priority;
	} else {
		/*
		 * group intr_priority already specified
		 * If interrupt priority specified before,
		 * it CANNOT BE CHANGED until `rmt_release_group_handle()` called
		 * So we have to check if the new priority specified conflicts with the old one
		 */
		if (intr_priority) {
			/*
			 * User specified intr_priority, check if conflict or not
			 * Even though the `group->intr_priority` is 0, an intr_priority must have
			 * been specified automatically too, although we do not know it exactly now,
			 * so specifying the intr_priority again might also cause conflict.
			 * So no matter if `group->intr_priority` is 0 or not, we have to check.
			 * Value `0` of `group->intr_priority` means "unknown", NOT "unspecified"!
			 */
			if (intr_priority != (group->intr_priority)) {
				/* intr_priority conflicts! */
				priority_conflict = true;
			}
		}
		/*
		 * user did not specify intr_priority, then keep the old priority
		 * We'll use the `RMT_INTR_ALLOC_FLAG | RMT_ALLOW_INTR_PRIORITY_MASK`,
		 * which should always success
		 */
	}

	/*
	 * The `group->intr_priority` will not change any longer, even though another task tries to
	 * modify it. So we could exit critical here safely.
	 */
	k_spin_unlock(&group->spinlock, key);

	return priority_conflict;
}

int rmt_get_isr_flags(rmt_group_t *group)
{
	int isr_flags = RMT_INTR_ALLOC_FLAG;

	if (group->intr_priority) {
		/* Use user-specified priority bit */
		isr_flags |= (1 << (group->intr_priority));
	} else {
		/* Allow all LOWMED priority bits */
		isr_flags |= RMT_ALLOW_INTR_PRIORITY_MASK;
	}

	return isr_flags;
}
