/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_can

#include <stdint.h>
#include <string.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include "can_mcan.h"
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define CAN_TASKS_START offsetof(NRF_CAN_Type, TASKS_START)
#define CAN_TASKS_STOPREQ offsetof(NRF_CAN_Type, TASKS_STOPREQ)
#define CAN_TASKS_STOP offsetof(NRF_CAN_Type, TASKS_STOP)
#define CAN_EVENTS_CORE_0 offsetof(NRF_CAN_Type, EVENTS_CORE[0])
#define CAN_EVENTS_CORE_1 offsetof(NRF_CAN_Type, EVENTS_CORE[1])
#define CAN_EVENTS_READYFORSTOP offsetof(NRF_CAN_Type, EVENTS_READYFORSTOP)
#define CAN_INTEN offsetof(NRF_CAN_Type, INTEN)

struct can_nrf_filter_cache {
	struct can_filter filter;
	can_rx_callback_t callback;
	void *user_data;
	int filter_id;
};

struct can_nrf_data {
	can_mode_t mode;
	struct can_timing timing;
	struct can_timing timing_data;
	struct can_nrf_filter_cache *caches;
	/* We need to use a mutex as mcan implementation does re-entrant calls */
	struct k_mutex lock;
	bool started;
};

struct can_nrf_config {
	uint32_t wrapper;
	uint32_t mcan;
	uint32_t mrba;
	uint32_t mram;
	const struct device *auxpll;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_configure)(void);
	uint16_t irq;
};

static const struct can_nrf_config *can_nrf_get_config(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;

	return mcan_config->custom;
}

static struct can_nrf_data *can_nrf_get_data(const struct device *dev)
{
	struct can_mcan_data *mcan_data = dev->data;

	return mcan_data->custom;
}

static int can_nrf_cache_get_max_filters(const struct device *dev)
{
	return can_mcan_get_max_filters(dev, false) + can_mcan_get_max_filters(dev, true);
}

static bool can_nrf_timing_is_set(const struct device *dev, const struct can_timing *timing)
{
	return timing->sjw != 0 ||
	       timing->prop_seg != 0 ||
	       timing->phase_seg1 != 0 ||
	       timing->phase_seg2 != 0 ||
	       timing->prescaler != 0;
}

static bool can_nrf_rx_filter_is_cached(const struct device *dev, int cache_filter_id)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);

	return data->caches[cache_filter_id].callback != NULL;
}

static int can_nrf_cache_get_filter_id(const struct device *dev, bool ide)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int num_filters;
	int num_candidate_filters;
	struct can_nrf_filter_cache *cache;

	num_filters = can_nrf_cache_get_max_filters(dev);
	num_candidate_filters = can_mcan_get_max_filters(dev, ide);

	for (int i = 0; i < num_filters; i++) {
		if (!can_nrf_rx_filter_is_cached(dev, i)) {
			continue;
		}

		cache = &data->caches[i];

		if (!!(cache->filter.flags & CAN_FRAME_IDE) != ide) {
			continue;
		}

		num_candidate_filters--;
	}

	if (num_candidate_filters == 0) {
		return -ENOSPC;
	}

	/*
	 * We can store the filter at the first available slot in the rx filter cache,
	 * can_mcan_add_rx_filter() will move it to an appropriate mailbox based on IDE.
	 */

	for (int i = 0; i < num_filters; i++) {
		if (!can_nrf_rx_filter_is_cached(dev, i)) {
			return i;
		}
	}

	return -ENOSPC;
}

static void can_nrf_cache_remove_rx_filter(const struct device *dev, int cache_filter_id)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	struct can_nrf_filter_cache *cache = &data->caches[cache_filter_id];

	cache->callback = NULL;
}

static int can_nrf_enable_rx_filter(const struct device *dev, int cache_filter_id)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	struct can_nrf_filter_cache *cache = &data->caches[cache_filter_id];
	int ret;

	ret = can_mcan_add_rx_filter(dev,
				     cache->callback,
				     cache->user_data,
				     &cache->filter);
	if (ret < 0) {
		return ret;
	}

	cache->filter_id = ret;

	return 0;
}

static int can_nrf_enable_rx_filters(const struct device *dev)
{
	int num_filters;
	int ret;

	num_filters = can_nrf_cache_get_max_filters(dev);

	for (int i = 0; i < num_filters; i++) {
		if (!can_nrf_rx_filter_is_cached(dev, i)) {
			continue;
		}

		ret = can_nrf_enable_rx_filter(dev, i);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static void can_nrf_disable_rx_filter(const struct device *dev, int rx_filter_id)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	struct can_nrf_filter_cache *rx_filter = &data->caches[rx_filter_id];

	can_mcan_remove_rx_filter(dev, rx_filter->filter_id);
}

static void can_nrf_disable_rx_filters(const struct device *dev)
{
	int num_filters;

	num_filters = can_nrf_cache_get_max_filters(dev);

	for (int i = 0; i < num_filters; i++) {
		if (can_nrf_rx_filter_is_cached(dev, i)) {
			can_nrf_disable_rx_filter(dev, i);
		}
	}
}

static int can_nrf_start(const struct device *dev)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->started) {
		ret = -EALREADY;
		goto unlock_return;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = can_mcan_set_mode(dev, data->mode);
	if (ret) {
		goto put_unlock_return;
	}

	ret = can_mcan_set_timing(dev, &data->timing);
	if (ret) {
		goto put_unlock_return;
	}

#if CONFIG_CAN_FD_MODE
	ret = can_mcan_set_timing_data(dev, &data->timing_data);
	if (ret) {
		goto put_unlock_return;
	}
#endif /* CONFIG_CAN_FD_MODE */

	ret = can_nrf_enable_rx_filters(dev);
	if (ret) {
		goto disable_put_unlock_return;
	}

	ret = can_mcan_start(dev);
	if (ret < 0) {
		goto disable_put_unlock_return;
	}

	data->started = true;

	goto unlock_return;

disable_put_unlock_return:
	can_nrf_disable_rx_filters(dev);

put_unlock_return:
	(void)pm_device_runtime_put(dev);

unlock_return:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int can_nrf_stop(const struct device *dev)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->started) {
		ret = -EALREADY;
		goto unlock_return;
	}

	can_nrf_disable_rx_filters(dev);

	ret = can_mcan_stop(dev);
	if (ret) {
		goto enable_unlock_return;
	}

	data->started = false;

	ret = pm_device_runtime_put(dev);
	if (ret) {
		goto start_enable_unlock_return;
	}

	goto unlock_return;

start_enable_unlock_return:
	(void)can_mcan_start(dev);

	data->started = true;

enable_unlock_return:
	(void)can_nrf_enable_rx_filters(dev);

unlock_return:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int can_nrf_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->started) {
		ret = -EBUSY;
		goto unlock_return;
	}

	ret = can_mcan_set_mode(dev, mode);
	if (ret < 0) {
		goto unlock_return;
	}

	data->mode = mode;

unlock_return:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int can_nrf_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->started) {
		ret = -EBUSY;
		goto unlock_return;
	}

	data->timing = *timing;
	ret = 0;

unlock_return:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int can_nrf_add_rx_filter(const struct device *dev,
				 can_rx_callback_t callback,
				 void *user_data,
				 const struct can_filter *filter)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;
	int cache_filter_id;
	struct can_nrf_filter_cache *cache;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = can_nrf_cache_get_filter_id(dev, !!(filter->flags & CAN_FRAME_IDE));
	if (ret < 0) {
		goto unlock_return;
	}

	cache_filter_id = ret;

	if (data->started) {
		ret = can_mcan_add_rx_filter(dev, callback, user_data, filter);
	} else {
		/* Will be set when filter is enabled later */
		ret = 0;
	}

	if (ret >= 0) {
		cache = &data->caches[cache_filter_id];
		cache->filter = *filter;
		cache->callback = callback;
		cache->user_data = user_data;
		cache->filter_id = ret;
		ret = cache_filter_id;
	}

unlock_return:
	k_mutex_unlock(&data->lock);

	return ret;
}

static void can_nrf_remove_rx_filter(const struct device *dev, int cache_filter_id)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	struct can_nrf_filter_cache *cache;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (cache_filter_id >= can_nrf_cache_get_max_filters(dev)) {
		goto unlock_return;
	}

	cache = &data->caches[cache_filter_id];

	if (data->started) {
		can_mcan_remove_rx_filter(dev, cache->filter_id);
	}

	can_nrf_cache_remove_rx_filter(dev, cache_filter_id);

unlock_return:
	k_mutex_unlock(&data->lock);
}

#ifdef CONFIG_CAN_FD_MODE
static int can_nrf_set_timing_data(const struct device *dev, const struct can_timing *timing_data)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->started) {
		ret = -EBUSY;
		goto unlock_return;
	}

	data->timing_data = *timing_data;
	ret = 0;

unlock_return:
	k_mutex_unlock(&data->lock);

	return ret;
}
#endif /* CONFIG_CAN_FD_MODE */

static int can_nrf_send(const struct device *dev,
			const struct can_frame *frame,
			k_timeout_t timeout,
			can_tx_callback_t callback,
			void *user_data)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = can_mcan_send(dev, frame, timeout, callback, user_data);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int can_nrf_get_state(const struct device *dev,
			     enum can_state *state,
			     struct can_bus_err_cnt *err_cnt)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = can_mcan_get_state(dev, state, err_cnt);
	k_mutex_unlock(&data->lock);

	return ret;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int can_nrf_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = can_mcan_recover(dev, timeout);
	k_mutex_unlock(&data->lock);

	return ret;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static void can_nrf_set_state_change_callback(const struct device *dev,
					      can_state_change_callback_t callback,
					      void *user_data)
{
	struct can_nrf_data *data = can_nrf_get_data(dev);

	k_mutex_lock(&data->lock, K_FOREVER);
	can_mcan_set_state_change_callback(dev, callback, user_data);
	k_mutex_unlock(&data->lock);
}

static void can_nrf_irq_handler(const struct device *dev)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);

	if (sys_read32(config->wrapper + CAN_EVENTS_CORE_0) == 1U) {
		sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_0);
		can_mcan_line_0_isr(dev);
	}

	if (sys_read32(config->wrapper + CAN_EVENTS_CORE_1) == 1U) {
		sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_1);
		can_mcan_line_1_isr(dev);
	}
}

static int can_nrf_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);

	return clock_control_get_rate(config->auxpll, NULL, rate);
}

static DEVICE_API(can, can_nrf_api) = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_nrf_start,
	.stop = can_nrf_stop,
	.set_mode = can_nrf_set_mode,
	.set_timing = can_nrf_set_timing,
	.send = can_nrf_send,
	.add_rx_filter = can_nrf_add_rx_filter,
	.remove_rx_filter = can_nrf_remove_rx_filter,
	.get_state = can_nrf_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_nrf_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_core_clock = can_nrf_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_nrf_set_state_change_callback,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_nrf_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static int can_nrf_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);

	return can_mcan_sys_read_reg(config->mcan, reg, val);
}

static int can_nrf_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);

	return can_mcan_sys_write_reg(config->mcan, reg, val);
}

static int can_nrf_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);

	return can_mcan_sys_read_mram(config->mram, offset, dst, len);
}

static int can_nrf_write_mram(const struct device *dev,
			      uint16_t offset,
			      const void *src,
			      size_t len)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);

	return can_mcan_sys_write_mram(config->mram, offset, src, len);
}

static int can_nrf_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);

	return can_mcan_sys_clear_mram(config->mram, offset, len);
}

static const struct can_mcan_ops can_mcan_nrf_ops = {
	.read_reg = can_nrf_read_reg,
	.write_reg = can_nrf_write_reg,
	.read_mram = can_nrf_read_mram,
	.write_mram = can_nrf_write_mram,
	.clear_mram = can_nrf_clear_mram,
};

static int can_nrf_resume(const struct device *dev)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);
	struct can_nrf_data *data = can_nrf_get_data(dev);
	int ret;
	struct can_timing timing;
#if CONFIG_CAN_FD_MODE
	struct can_timing timing_data;
#endif

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = nrf_clock_control_request_sync(config->auxpll, NULL, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_0);
	sys_write32(0U, config->wrapper + CAN_EVENTS_CORE_1);
	sys_write32(CAN_INTEN_CORE0_Msk | CAN_INTEN_CORE1_Msk, config->wrapper + CAN_INTEN);
	sys_write32(1U, config->wrapper + CAN_TASKS_START);

	ret = can_mcan_configure_mram(dev, config->mrba, config->mram);
	if (ret < 0) {
		return ret;
	}

	/*
	 * can_mcan_init() calls can_mcan_set_timing() which calls can_set_timing(), overwriting
	 * the user provided timings with default timings. Store timings before and restore them
	 * after call to can_mcan_init() if provided.
	 */

	timing = data->timing;
#if CONFIG_CAN_FD_MODE
	timing_data = data->timing_data;
#endif

	ret = can_mcan_init(dev);
	if (ret < 0) {
		return ret;
	}

	/*
	 * If the user has not provided timings, we keep the default timings just provided by
	 * can_mcan_init().
	 */

	if (can_nrf_timing_is_set(dev, &timing)) {
		data->timing = timing;
	}

#if CONFIG_CAN_FD_MODE
	if (can_nrf_timing_is_set(dev, &timing_data)) {
		data->timing_data = timing_data;
	}
#endif

	return 0;
}

static int can_nrf_suspend(const struct device *dev)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);
	int ret;

	sys_write32(0U, config->wrapper + CAN_EVENTS_READYFORSTOP);
	sys_write32(1U, config->wrapper + CAN_TASKS_STOPREQ);
	WAIT_FOR(sys_read32(config->wrapper + CAN_EVENTS_READYFORSTOP), 200000, k_msleep(1));
	sys_write32(1U, config->wrapper + CAN_TASKS_STOP);

	ret = nrf_clock_control_release(config->auxpll, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int nrf_can_pm_callback(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = can_nrf_resume(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = can_nrf_suspend(dev);
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int can_nrf_init(const struct device *dev)
{
	const struct can_nrf_config *config = can_nrf_get_config(dev);
	struct can_nrf_data *data = can_nrf_get_data(dev);

	if (!device_is_ready(config->auxpll)) {
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	config->irq_configure();

	return pm_device_driver_init(dev, nrf_can_pm_callback);
}

#define CAN_NRF_CACHES_GET(inst) \
	CONCAT(caches, inst)

#define CAN_NRF_CACHE_COUNT(inst)								\
	(											\
		CAN_MCAN_DT_INST_MRAM_STD_FILTER_ELEMENTS(inst) +				\
		CAN_MCAN_DT_INST_MRAM_EXT_FILTER_ELEMENTS(inst)					\
	)

#define CAN_NRF_CACHES_DEFINE(inst)								\
	static struct can_nrf_filter_cache CAN_NRF_CACHES_GET(inst)				\
		[CAN_NRF_CACHE_COUNT(inst)]

#define CAN_NRF_DEFINE(inst)									\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static void CONCAT(can_nrf_irq_configure, inst)(void)					\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst),							\
			    DT_INST_IRQ(inst, priority),					\
			    can_nrf_irq_handler,						\
			    DEVICE_DT_INST_GET(inst),						\
			    0);									\
												\
		irq_enable(DT_INST_IRQN(inst));							\
	}											\
												\
	static const struct can_nrf_config CONCAT(can_nrf_config, inst) = {			\
		.wrapper = DT_INST_REG_ADDR_BY_NAME(inst, wrapper),				\
		.mcan = CAN_MCAN_DT_INST_MCAN_ADDR(inst),					\
		.mrba = CAN_MCAN_DT_INST_MRBA(inst),						\
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(inst),					\
		.auxpll = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, auxpll)),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
		.irq = DT_INST_IRQN(inst),							\
		.irq_configure = CONCAT(can_nrf_irq_configure, inst),				\
	};											\
												\
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(inst, CONCAT(can_mcan_nrf_cbs, inst));		\
												\
	static const struct can_mcan_config CONCAT(can_mcan_nrf_config, inst) =			\
		CAN_MCAN_DT_CONFIG_INST_GET(inst,						\
					    &CONCAT(can_nrf_config, inst),			\
					    &can_mcan_nrf_ops,					\
					    &CONCAT(can_mcan_nrf_cbs, inst));			\
												\
	CAN_NRF_CACHES_DEFINE(inst);								\
												\
	static struct can_nrf_data CONCAT(can_nrf_data, inst) = {				\
		.caches = CAN_NRF_CACHES_GET(inst),						\
	};											\
												\
	CAN_MCAN_DATA_DEFINE(CONCAT(can_mcan_nrf_data, inst), &CONCAT(can_nrf_data, inst));	\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, nrf_can_pm_callback);					\
												\
	CAN_DEVICE_DT_INST_DEFINE(inst,								\
				  can_nrf_init,							\
				  PM_DEVICE_DT_INST_GET(inst),					\
				  &CONCAT(can_mcan_nrf_data, inst),				\
				  &CONCAT(can_mcan_nrf_config, inst),				\
				  POST_KERNEL,							\
				  CONFIG_CAN_INIT_PRIORITY,					\
				  &can_nrf_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_NRF_DEFINE)
