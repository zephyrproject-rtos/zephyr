/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/lock.h>
#include "rmt_private.h"
#include "clk_ctrl_os.h"
#include "esp_private/esp_clk_tree_common.h"
#include "esp_private/periph_ctrl.h"
#if RMT_SLEEP_RETENTION_ENABLED
#include "esp_private/sleep_retention.h"
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(espressif_rmt);

typedef struct rmt_platform_t {
	/**< Array of RMT group instances */
	rmt_group_t *groups[RMT_LL_GET(INST_NUM)];
	/**< Reference count used to protect group install/uninstall */
	int group_ref_counts[RMT_LL_GET(INST_NUM)];
} rmt_platform_t;

static rmt_platform_t s_platform; /* singleton platform */

#if RMT_SLEEP_RETENTION_ENABLED
static int rmt_create_sleep_retention_link_cb(void *arg);
#endif

rmt_group_t *rmt_acquire_group_handle(int group_id)
{
	unsigned int key;
	bool new_group = false;
	rmt_group_t *group = NULL;

	/* prevent install rmt group concurrently */
	key = irq_lock();
	if (!s_platform.groups[group_id]) {
		group = k_calloc(1, sizeof(rmt_group_t));
		if (group) {
			new_group = true;
			s_platform.groups[group_id] = group;
			group->group_id = group_id;
			/* Initial occupy_mask: 1111...100...0 */
			group->occupy_mask = UINT32_MAX & ~((1 << RMT_LL_GET(CHANS_PER_INST)) - 1);
			/*
			 * Group clock won't be configured at this stage,
			 * it will be set when allocate the first channel.
			 */
			group->clk_src = 0;
			/**
			 * Group interrupt priority is shared between all channels,
			 * it will be set when allocate the first channel.
			 */
			group->intr_priority = RMT_GROUP_INTR_PRIORITY_UNINITIALIZED;
			/* enable the bus clock for the RMT peripheral */
			PERIPH_RCC_ATOMIC() {
				rmt_ll_enable_bus_clock(group_id, true);
				rmt_ll_reset_register(group_id);
			}
#if RMT_SLEEP_RETENTION_ENABLED
			sleep_retention_module_t module = rmt_reg_retention_info[group_id].module;
			sleep_retention_module_init_param_t init_param = {
				.cbs = {
					.create = {
						.handle = rmt_create_sleep_retention_link_cb,
						.arg = group,
					},
				},
				.depends = RETENTION_MODULE_BITMAP_INIT(CLOCK_SYSTEM)
			};
			if (sleep_retention_module_init(module, &init_param) != ESP_OK) {
				/**
				 * Even though the sleep retention module init failed,
				 * RMT driver should still work, so just warning here
				 */
				LOG_WRN("Init sleep retention failed %d, power domain may be "
					"turned off during sleep", group_id);
			}
#endif
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
	irq_unlock(key);

	if (new_group) {
		LOG_DBG("new group(%d) at %p, occupy=%"PRIx32, group_id, group, group->occupy_mask);
	}
	return group;
}

void rmt_release_group_handle(rmt_group_t *group)
{
	unsigned int key;
	rmt_clock_source_t clk_src = group->clk_src;
	bool do_deinitialize = false;
	rmt_hal_context_t *hal = &group->hal;

	key = irq_lock();
	s_platform.group_ref_counts[group->group_id]--;
	if (s_platform.group_ref_counts[group->group_id] == 0) {
		do_deinitialize = true;
		s_platform.groups[group->group_id] = NULL;
		/* disable core clock */
		PERIPH_RCC_ATOMIC() {
			rmt_ll_enable_group_clock(hal->regs, false);
		}
		/* hal layer deinitialize */
		rmt_hal_deinit(hal);
		/* disable bus clock */
		PERIPH_RCC_ATOMIC() {
			rmt_ll_enable_bus_clock(group->group_id, false);
		}
	}
	irq_unlock(key);

	switch (clk_src) {
#if RMT_LL_SUPPORT(RC_FAST)
	case RMT_CLK_SRC_RC_FAST:
		periph_rtc_dig_clk8m_disable();
		break;
#endif
	default:
		break;
	}

	if (do_deinitialize) {
#if RMT_SLEEP_RETENTION_ENABLED
		sleep_retention_module_t module = rmt_reg_retention_info[group->group_id].module;

		if (sleep_retention_is_module_created(module)) {
			sleep_retention_module_free(module);
		}
		if (sleep_retention_is_module_inited(module)) {
			sleep_retention_module_deinit(module);
		}
#endif
		LOG_DBG("del group(%d)", group->group_id);
		k_free(group);
	}
}

#if !RMT_LL_GET(CHANNEL_CLK_INDEPENDENT)
static int rmt_set_group_prescale(rmt_channel_t *chan, uint32_t expect_resolution_hz,
	uint32_t *ret_channel_prescale)
{
	esp_err_t ret;
	uint32_t periph_src_clk_hz = 0;
	rmt_group_t *group = chan->group;
	uint32_t group_resolution_hz = 0;
	uint32_t group_prescale = 0;
	uint32_t channel_prescale = 0;
	bool prescale_conflict = false;
	k_spinlock_key_t key;

	ret = esp_clk_tree_src_get_freq_hz(group->clk_src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
		&periph_src_clk_hz);
	if (ret) {
		LOG_ERR("get clock source freq failed");
		return -EINVAL;
	}

	if (group->resolution_hz == 0) {
		while (++group_prescale <= RMT_LL_GROUP_CLOCK_MAX_INTEGER_PRESCALE) {
			group_resolution_hz = periph_src_clk_hz / group_prescale;
			channel_prescale = (group_resolution_hz + expect_resolution_hz / 2)
				/ expect_resolution_hz;
			/**
			 * Use the first value found during the search that satisfies the division
			 * requirement (highest frequency)
			 */
			if (channel_prescale > 0 &&
				channel_prescale <= RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE) {
				break;
			}
		}
	} else {
		group_prescale = periph_src_clk_hz / group->resolution_hz;
		channel_prescale = (group->resolution_hz + expect_resolution_hz / 2)
			/ expect_resolution_hz;
	}

	if (!(group_prescale > 0 && group_prescale <= RMT_LL_GROUP_CLOCK_MAX_INTEGER_PRESCALE)) {
		LOG_ERR("group prescale out of the range");
		return -EINVAL;
	}
	if (!(channel_prescale > 0 && channel_prescale <= RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE)) {
		LOG_ERR("channel prescale out of the range");
		return -EINVAL;
	}

	/**
	 * Group prescale is shared by all rmt_channel, only set once.
	 * Use critical section to avoid race condition.
	 */
	group_resolution_hz = periph_src_clk_hz / group_prescale;
	key = k_spin_lock(&group->spinlock);
	if (group->resolution_hz == 0) {
		group->resolution_hz = group_resolution_hz;
		PERIPH_RCC_ATOMIC() {
			rmt_ll_set_group_clock_src(group->hal.regs, chan->channel_id,
				group->clk_src, group_prescale, 1, 0);
			rmt_ll_enable_group_clock(group->hal.regs, true);
		}
	} else {
		prescale_conflict = (group->resolution_hz != group_resolution_hz);
	}
	k_spin_unlock(&group->spinlock, key);
	if (prescale_conflict) {
		LOG_ERR("group resolution conflict, already is %"PRIu32" but attempt to %"PRIu32"",
			group->resolution_hz, group_resolution_hz);
		return -EINVAL;
	}
	LOG_DBG("group (%d) clock resolution:%"PRIu32"Hz", group->group_id, group->resolution_hz);
	*ret_channel_prescale = channel_prescale;
	return 0;
}
#endif

int rmt_select_periph_clock(rmt_channel_handle_t chan, rmt_clock_source_t clk_src,
	uint32_t expect_channel_resolution)
{
	esp_err_t ret;
	rmt_group_t *group = chan->group;
	bool clock_selection_conflict = false;
	k_spinlock_key_t key;
	uint32_t real_div = 0;

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
#if RMT_LL_SUPPORT(RC_FAST)
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

	ret = esp_clk_tree_enable_src((soc_module_clk_t)clk_src, true);
	if (ret) {
		LOG_ERR("clock source enable failed");
		return -EINVAL;
	}
#if RMT_LL_GET(CHANNEL_CLK_INDEPENDENT)
	uint32_t periph_src_clk_hz = 0;
	/* get clock source frequency */
	if (esp_clk_tree_src_get_freq_hz((soc_module_clk_t)clk_src,
		ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &periph_src_clk_hz) != ESP_OK) {
		LOG_ERR("get clock source frequency failed");
		return -EINVAL;
	}
	PERIPH_RCC_ATOMIC() {
		rmt_ll_set_group_clock_src(group->hal.regs, chan->channel_id, clk_src, 1, 1, 0);
		rmt_ll_enable_group_clock(group->hal.regs, true);
	}
	group->resolution_hz = periph_src_clk_hz;
	LOG_DBG("group clock resolution:%"PRIu32, group->resolution_hz);
	real_div = (group->resolution_hz + expect_channel_resolution / 2)
		/ expect_channel_resolution;
#else
	/**
	 * Set division for group clock source,
	 * to achieve highest resolution while guaranteeing the channel resolution.
	 */
	if (rmt_set_group_prescale(chan, expect_channel_resolution, &real_div)) {
		LOG_ERR("set rmt group prescale failed");
		return -EINVAL;
	}
#endif

	if (chan->direction == RMT_CHANNEL_DIRECTION_TX) {
		rmt_ll_tx_set_channel_clock_div(group->hal.regs, chan->channel_id, real_div);
	} else {
		rmt_ll_rx_set_channel_clock_div(group->hal.regs, chan->channel_id, real_div);
	}
	/* resolution lost due to division, calculate the real resolution */
	chan->resolution_hz = group->resolution_hz / real_div;
	if (chan->resolution_hz != expect_channel_resolution) {
		LOG_WRN("channel resolution loss, real=%"PRIu32, chan->resolution_hz);
	}

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
	if (group->intr_priority == RMT_GROUP_INTR_PRIORITY_UNINITIALIZED) {
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

int rmt_isr_priority_to_flags(rmt_group_t *group)
{
	int isr_flags = 0;

	if (group->intr_priority) {
		/* Use user-specified priority bit */
		isr_flags |= (1 << (group->intr_priority));
	} else {
		/* Allow all LOWMED priority bits */
		isr_flags |= RMT_ALLOW_INTR_PRIORITY_MASK;
	}

	return isr_flags;
}

#if RMT_SLEEP_RETENTION_ENABLED
static int rmt_create_sleep_retention_link_cb(void *arg)
{
	rmt_group_t *group = (rmt_group_t *)arg;

	if (!sleep_retention_entries_create(
		rmt_reg_retention_info[group->group_id].regdma_entry_array,
		rmt_reg_retention_info[group->group_id].array_size, REGDMA_LINK_PRI_RMT,
		rmt_reg_retention_info[group->group_id].module)) {
		return -EINVAL;
	}

	return 0;
}

void rmt_create_retention_module(rmt_group_t *group)
{
	unsigned int key;
	sleep_retention_module_t module = rmt_reg_retention_info[group->group_id].module;

	key = irq_lock();
	if (sleep_retention_is_module_inited(module) &&
		!sleep_retention_is_module_created(module)) {
		if (sleep_retention_module_allocate(module) != ESP_OK) {
			/**
			 * Even though the sleep retention module create failed,
			 * RMT driver should still work, so just warning here
			 */
			LOG_WRN("Create retention link failed, "
				"power domain will not be turned off during sleep");
		}
	}
	irq_unlock(key);
}
#endif
