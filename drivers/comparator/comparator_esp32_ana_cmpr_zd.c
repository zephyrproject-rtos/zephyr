/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_ana_cmpr_zd

#include <hal/ana_cmpr_ll.h>
#include <hal/ana_cmpr_periph.h>
#include <hal/systimer_ll.h>
#include <esp_private/systimer.h>
#include <esp_err.h>
#include <soc/gpio_num.h>
#include <esp_clk_tree.h>
#include <esp_private/esp_clk_tree_common.h>
#include <soc.h>
#include <soc/soc_etm_source.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/clock.h>
#include <zephyr/logging/log.h>

#include <esp_etm_alloc.h>

esp_err_t gpio_config_as_analog(gpio_num_t gpio_num);

LOG_MODULE_REGISTER(ana_cmpr_esp32_zd, CONFIG_COMPARATOR_LOG_LEVEL);

#define ANA_CMPR_ESP32_INTR_ALL ANALOG_CMPR_LL_ALL_INTR_MASK(0)

#define ANA_CMPR_ESP32_CLK_XTAL    1
#define ANA_CMPR_ESP32_CLK_RC_FAST 2

#define ANA_CMPR_ESP32_UNIT(inst) \
	((DT_INST_REG_ADDR(inst) == DR_REG_ZERO_DET_BASE) ? 0 : -1)

struct ana_cmpr_esp32_config {
	analog_cmpr_dev_t *hw;
	uint8_t unit;
	int irq_source;
	uint8_t irq_priority;
	uint32_t irq_flags;
	ana_cmpr_ref_source_t ref_source;
	ana_cmpr_ref_voltage_t ref_voltage;
	ana_cmpr_ref_hys_t hys;
	uint8_t ext_ref_pad;
	uint8_t clk_src;
	uint32_t debounce_cycles;
	uint32_t poll_period_ns;
	uint8_t resample_limit;
	uint32_t scan_period_us;
	uint32_t scan_mask;
	uint8_t n_channels;
	const struct device *channels[ANALOG_CMPR_LL_SRC_CHANNEL_NUM];
};

struct ana_cmpr_esp32_data {
	struct k_mutex cfg_lock;
	struct k_spinlock lock;
	uint8_t armed_mask;
	bool scan_on;
	uint32_t scan_wait_us;
	esp_etm_chan_t etm_chan;
};

struct ana_cmpr_esp32_chan_config {
	const struct device *parent;
	uint8_t channel;
	uint8_t pad;
};

struct ana_cmpr_esp32_chan_data {
	comparator_callback_t callback;
	void *user_data;
	atomic_t pending;
	uint32_t intr_mask;
};

static void ana_cmpr_esp32_isr(void *arg)
{
	const struct device *parent = arg;
	const struct ana_cmpr_esp32_config *cfg = parent->config;
	uint32_t status = analog_cmpr_ll_get_intr_status(cfg->hw);

	if (status == 0U) {
		return;
	}

	analog_cmpr_ll_clear_intr(cfg->hw, status);

	for (uint8_t i = 0U; i < ANALOG_CMPR_LL_SRC_CHANNEL_NUM; i++) {
		const struct device *ch = cfg->channels[i];
		struct ana_cmpr_esp32_chan_data *cdata;

		if (ch == NULL) {
			continue;
		}

		cdata = ch->data;
		if ((status & cdata->intr_mask) == 0U) {
			continue;
		}

		if (cdata->callback != NULL) {
			cdata->callback(ch, cdata->user_data);
		} else {
			atomic_set(&cdata->pending, 1);
		}
	}
}

static uint32_t ana_cmpr_esp32_intr_mask(uint32_t unit, uint32_t channel,
					 enum comparator_trigger trigger)
{
	switch (trigger) {
	case COMPARATOR_TRIGGER_RISING_EDGE:
		return analog_cmpr_ll_get_intr_mask_by_type(unit, channel, ANA_CMPR_CROSS_POS);
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		return analog_cmpr_ll_get_intr_mask_by_type(unit, channel, ANA_CMPR_CROSS_NEG);
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		return analog_cmpr_ll_get_intr_mask_by_type(unit, channel, ANA_CMPR_CROSS_ANY);
	case COMPARATOR_TRIGGER_NONE:
	default:
		return 0U;
	}
}

static int ana_cmpr_esp32_get_output(const struct device *dev)
{
	const struct ana_cmpr_esp32_chan_config *ccfg = dev->config;
	const struct ana_cmpr_esp32_config *cfg = ccfg->parent->config;
	struct ana_cmpr_esp32_data *data = ccfg->parent->data;
	k_spinlock_key_t key;
	bool idle;

	key = k_spin_lock(&data->lock);
	idle = (data->scan_on == false);
	if (idle) {
		analog_cmpr_ll_start_scan(cfg->hw);
	}
	k_spin_unlock(&data->lock, key);

	if (idle) {
		k_busy_wait(data->scan_wait_us);
	}

	if (analog_cmpr_ll_get_compare_result(cfg->hw, ccfg->channel)) {
		return 1;
	}

	return 0;
}

/* Alarm 1 is the HAL's core-1 tick slot, which this port leaves unused. */
static struct k_spinlock ana_cmpr_esp32_alarm_lock;
static bool ana_cmpr_esp32_alarm_on;

static int ana_cmpr_esp32_scan_alarm_enable(uint32_t period_us, uint32_t *event_id)
{
	systimer_dev_t *dev = &SYSTIMER;
	uint64_t ticks;
	k_spinlock_key_t key;

	if (period_us == 0U || event_id == NULL) {
		return -EINVAL;
	}

	ticks = ((uint64_t)period_us * sys_clock_hw_cycles_per_sec()) / 1000000ULL;
	if (ticks == 0ULL || ticks >= (1ULL << 26)) {
		return -EINVAL;
	}

	key = k_spin_lock(&ana_cmpr_esp32_alarm_lock);
	if (ana_cmpr_esp32_alarm_on) {
		k_spin_unlock(&ana_cmpr_esp32_alarm_lock, key);
		return -EBUSY;
	}

	systimer_ll_enable_alarm(dev, SYSTIMER_ALARM_OS_TICK_CORE1, false);
	systimer_ll_connect_alarm_counter(dev, SYSTIMER_ALARM_OS_TICK_CORE1,
					  SYSTIMER_COUNTER_OS_TICK);
	systimer_ll_set_alarm_period(dev, SYSTIMER_ALARM_OS_TICK_CORE1, (uint32_t)ticks);
	systimer_ll_apply_alarm_value(dev, SYSTIMER_ALARM_OS_TICK_CORE1);
	systimer_ll_enable_alarm_oneshot(dev, SYSTIMER_ALARM_OS_TICK_CORE1);
	systimer_ll_enable_alarm_period(dev, SYSTIMER_ALARM_OS_TICK_CORE1);
	systimer_ll_enable_alarm_int(dev, SYSTIMER_ALARM_OS_TICK_CORE1, false);
	systimer_ll_clear_alarm_int(dev, SYSTIMER_ALARM_OS_TICK_CORE1);
	systimer_ll_enable_etm(dev, true);
	systimer_ll_enable_alarm(dev, SYSTIMER_ALARM_OS_TICK_CORE1, true);
	ana_cmpr_esp32_alarm_on = true;
	k_spin_unlock(&ana_cmpr_esp32_alarm_lock, key);

	*event_id = SYSTIMER_EVT_CNT_CMP0 + SYSTIMER_ALARM_OS_TICK_CORE1;
	return 0;
}

static void ana_cmpr_esp32_scan_alarm_disable(void)
{
	systimer_dev_t *dev = &SYSTIMER;
	k_spinlock_key_t key = k_spin_lock(&ana_cmpr_esp32_alarm_lock);

	if (ana_cmpr_esp32_alarm_on == false) {
		k_spin_unlock(&ana_cmpr_esp32_alarm_lock, key);
		return;
	}

	systimer_ll_enable_alarm(dev, SYSTIMER_ALARM_OS_TICK_CORE1, false);
	systimer_ll_enable_alarm_oneshot(dev, SYSTIMER_ALARM_OS_TICK_CORE1);
	systimer_ll_enable_alarm_int(dev, SYSTIMER_ALARM_OS_TICK_CORE1, false);
	systimer_ll_clear_alarm_int(dev, SYSTIMER_ALARM_OS_TICK_CORE1);
	ana_cmpr_esp32_alarm_on = false;
	k_spin_unlock(&ana_cmpr_esp32_alarm_lock, key);
}

static void ana_cmpr_esp32_scan_abort(const struct device *dev, uint32_t mask)
{
	const struct ana_cmpr_esp32_chan_config *ccfg = dev->config;
	struct ana_cmpr_esp32_chan_data *cdata = dev->data;
	const struct ana_cmpr_esp32_config *cfg = ccfg->parent->config;
	struct ana_cmpr_esp32_data *data = ccfg->parent->data;
	k_spinlock_key_t key;
	bool stop;

	key = k_spin_lock(&data->lock);
	analog_cmpr_ll_enable_intr(cfg->hw, mask, false);
	cdata->intr_mask = 0U;
	data->armed_mask &= ~BIT(ccfg->channel);
	stop = (data->armed_mask == 0U);
	if (stop) {
		data->scan_on = false;
	}
	k_spin_unlock(&data->lock, key);

	if (stop) {
		(void)esp_etm_channel_disable(data->etm_chan);
		ana_cmpr_esp32_scan_alarm_disable();
	}
}

static int ana_cmpr_esp32_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct ana_cmpr_esp32_chan_config *ccfg = dev->config;
	struct ana_cmpr_esp32_chan_data *cdata = dev->data;
	const struct ana_cmpr_esp32_config *cfg = ccfg->parent->config;
	struct ana_cmpr_esp32_data *data = ccfg->parent->data;
	k_spinlock_key_t key;
	uint32_t mask;
	bool start;
	bool stop;
	int ret = 0;

	if (trigger > COMPARATOR_TRIGGER_BOTH_EDGES) {
		return -EINVAL;
	}

	/* Serializes arming with the alarm and ETM setup done outside the spinlock. */
	k_mutex_lock(&data->cfg_lock, K_FOREVER);
	mask = ana_cmpr_esp32_intr_mask(cfg->unit, ccfg->channel, trigger);

	if ((mask != 0U) && (data->scan_on == false)) {
		/* Refresh the stored result before enabling the interrupt. */
		analog_cmpr_ll_start_scan(cfg->hw);
		k_busy_wait(data->scan_wait_us);
	}

	key = k_spin_lock(&data->lock);
	analog_cmpr_ll_enable_intr(cfg->hw, cdata->intr_mask, false);
	cdata->intr_mask = 0U;
	atomic_set(&cdata->pending, 0);

	if (mask == 0U) {
		data->armed_mask &= ~BIT(ccfg->channel);
	} else {
		data->armed_mask |= BIT(ccfg->channel);
	}

	stop = (data->armed_mask == 0U) && data->scan_on;
	start = (mask != 0U) && (data->scan_on == false);
	if (start) {
		data->scan_on = true;
	}
	if (stop) {
		data->scan_on = false;
	}

	if (mask != 0U) {
		analog_cmpr_ll_clear_intr(cfg->hw, start ? ANA_CMPR_ESP32_INTR_ALL : mask);
		cdata->intr_mask = mask;
		analog_cmpr_ll_enable_intr(cfg->hw, mask, true);
	}
	k_spin_unlock(&data->lock, key);

	if (stop) {
		(void)esp_etm_channel_disable(data->etm_chan);
		ana_cmpr_esp32_scan_alarm_disable();
		goto out;
	}

	if (start) {
		uint32_t event_id;

		ret = ana_cmpr_esp32_scan_alarm_enable(cfg->scan_period_us, &event_id);
		if (ret != 0) {
			ana_cmpr_esp32_scan_abort(dev, mask);
			goto out;
		}

		ret = esp_etm_channel_connect(data->etm_chan, event_id,
					      ANALOG_CMPR_LL_ETM_TASK(cfg->unit, 0));
		if (ret != 0) {
			ana_cmpr_esp32_scan_abort(dev, mask);
			goto out;
		}

		ret = esp_etm_channel_enable(data->etm_chan);
		if (ret != 0) {
			ana_cmpr_esp32_scan_abort(dev, mask);
			goto out;
		}

		goto out;
	}

out:
	k_mutex_unlock(&data->cfg_lock);
	return ret;
}

static int ana_cmpr_esp32_set_trigger_callback(const struct device *dev,
					       comparator_callback_t callback, void *user_data)
{
	struct ana_cmpr_esp32_chan_data *cdata = dev->data;
	unsigned int key;
	atomic_val_t pending;

	key = irq_lock();
	cdata->callback = callback;
	cdata->user_data = user_data;
	pending = atomic_clear(&cdata->pending);
	irq_unlock(key);

	if (callback != NULL && pending != 0) {
		callback(dev, user_data);
	}

	return 0;
}

static int ana_cmpr_esp32_trigger_is_pending(const struct device *dev)
{
	struct ana_cmpr_esp32_chan_data *cdata = dev->data;

	return atomic_clear(&cdata->pending) != 0 ? 1 : 0;
}

static DEVICE_API(comparator, ana_cmpr_esp32_api) = {
	.get_output = ana_cmpr_esp32_get_output,
	.set_trigger = ana_cmpr_esp32_set_trigger,
	.set_trigger_callback = ana_cmpr_esp32_set_trigger_callback,
	.trigger_is_pending = ana_cmpr_esp32_trigger_is_pending,
};

static void ana_cmpr_esp32_clear_mux(analog_cmpr_dev_t *hw)
{
	hw->conf.vref_channel_sel = 0;
	hw->conf.comp_channel_1_sel = 0;
	hw->conf.comp_channel_2_sel = 0;
	hw->conf.comp_channel_3_sel = 0;
}

static ana_cmpr_clk_src_t ana_cmpr_esp32_clk(uint8_t src)
{
	switch (src) {
	case ANA_CMPR_ESP32_CLK_XTAL:
		return ANA_CMPR_CLK_SRC_XTAL;
	case ANA_CMPR_ESP32_CLK_RC_FAST:
		return ANA_CMPR_CLK_SRC_RC_FAST;
	default:
		return ANA_CMPR_CLK_SRC_DEFAULT;
	}
}

static int ana_cmpr_esp32_timing(const struct ana_cmpr_esp32_config *cfg, uint32_t freq_hz,
				 uint32_t *poll_cycles, uint32_t *wait_us)
{
	uint64_t cycles;
	uint64_t scan_cycles;
	uint64_t wait;
	uint64_t ticks;

	if (freq_hz == 0U) {
		return -EIO;
	}

	cycles = ((uint64_t)cfg->poll_period_ns * freq_hz + 999999999ULL) / 1000000000ULL;
	if (cycles < 1ULL) {
		cycles = 1ULL;
	}
	if (cycles > ANALOG_CMPR_LL_MAX_POLL_PERIOD_CYCLES) {
		LOG_ERR("unit %u: poll period %u ns exceeds %u cycles", cfg->unit,
			cfg->poll_period_ns, ANALOG_CMPR_LL_MAX_POLL_PERIOD_CYCLES);
		return -EINVAL;
	}

	scan_cycles = (cycles + cfg->resample_limit) * cfg->n_channels;
	wait = (scan_cycles * 1000000ULL + freq_hz - 1ULL) / freq_hz;
	wait += 1ULL;
	if (wait > UINT32_MAX || cfg->scan_period_us <= (uint32_t)wait) {
		LOG_ERR("unit %u: scan period %u us is not longer than one pass (%u us)", cfg->unit,
			cfg->scan_period_us, (uint32_t)wait);
		return -EINVAL;
	}

	ticks = ((uint64_t)cfg->scan_period_us * sys_clock_hw_cycles_per_sec()) / 1000000ULL;
	if (ticks == 0ULL || ticks >= (1ULL << 26)) {
		LOG_ERR("unit %u: scan period %u us is out of the alarm range", cfg->unit,
			cfg->scan_period_us);
		return -EINVAL;
	}

	*poll_cycles = (uint32_t)cycles;
	*wait_us = (uint32_t)wait;
	return 0;
}

static int ana_cmpr_esp32_configure_pads(const struct device *dev)
{
	const struct ana_cmpr_esp32_config *cfg = dev->config;
	int ret;

	ana_cmpr_esp32_clear_mux(cfg->hw);

	for (uint8_t i = 0U; i < ANALOG_CMPR_LL_SRC_CHANNEL_NUM; i++) {
		const struct device *ch = cfg->channels[i];
		const struct ana_cmpr_esp32_chan_config *ccfg;
		int gpio;

		if (ch == NULL) {
			continue;
		}

		ccfg = ch->config;
		gpio = ana_cmpr_periph[cfg->unit].pad_gpios[ccfg->pad];
		ret = gpio_config_as_analog(gpio);
		if (ret != 0) {
			LOG_ERR("unit %u: gpio %d analog config failed (%d)", cfg->unit, gpio, ret);
			return -EIO;
		}

		analog_cmpr_ll_set_src_pad(cfg->hw, ccfg->channel, ccfg->pad);
	}

	if (cfg->ref_source == ANA_CMPR_REF_SRC_EXTERNAL) {
		int gpio = ana_cmpr_periph[cfg->unit].pad_gpios[cfg->ext_ref_pad];

		ret = gpio_config_as_analog(gpio);
		if (ret != 0) {
			LOG_ERR("unit %u: ref gpio %d analog config failed (%d)", cfg->unit, gpio,
				ret);
			return -EIO;
		}

		analog_cmpr_ll_set_ext_ref_pad(cfg->hw, cfg->ext_ref_pad);
	}

	return 0;
}

static int ana_cmpr_esp32_init(const struct device *dev)
{
	const struct ana_cmpr_esp32_config *cfg = dev->config;
	struct ana_cmpr_esp32_data *data = dev->data;
	ana_cmpr_clk_src_t clk = ana_cmpr_esp32_clk(cfg->clk_src);
	uint32_t freq_hz = 0U;
	uint32_t poll_cycles;
	int ret;

	data->etm_chan = ESP_ETM_CHAN_NONE;
	k_mutex_init(&data->cfg_lock);

	analog_cmpr_ll_enable_bus_clock(cfg->unit, true);
	analog_cmpr_ll_reset_register(cfg->unit);

	ret = esp_clk_tree_enable_src((soc_module_clk_t)clk, true);
	if (ret != 0) {
		LOG_ERR("unit %u: clock source enable failed (%d)", cfg->unit, ret);
		return -EIO;
	}

	ret = esp_clk_tree_src_get_freq_hz((soc_module_clk_t)clk,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &freq_hz);
	if (ret != 0 || freq_hz == 0U) {
		LOG_ERR("unit %u: clock frequency unavailable (%d)", cfg->unit, ret);
		return -EIO;
	}

	analog_cmpr_ll_set_clk_src(cfg->unit, clk);
	analog_cmpr_ll_set_clk_div(cfg->unit, 1U);
	analog_cmpr_ll_enable_function_clock(cfg->unit, true);
	analog_cmpr_ll_reset_core(cfg->unit);

	ret = ana_cmpr_esp32_timing(cfg, freq_hz, &poll_cycles, &data->scan_wait_us);
	if (ret != 0) {
		return ret;
	}

	analog_cmpr_ll_enable(cfg->hw, true);
	ret = ana_cmpr_esp32_configure_pads(dev);
	if (ret != 0) {
		return ret;
	}

	analog_cmpr_ll_set_ref_source(cfg->hw, cfg->ref_source);
	if (cfg->ref_source == ANA_CMPR_REF_SRC_INTERNAL) {
		analog_cmpr_ll_set_internal_ref_voltage(cfg->hw, cfg->ref_voltage);
		analog_cmpr_ll_set_ref_hys_level(cfg->hw, cfg->hys);
	}

	analog_cmpr_ll_set_poll_period(cfg->hw, poll_cycles);
	analog_cmpr_ll_set_resample_limit(cfg->hw, cfg->resample_limit);
	analog_cmpr_ll_set_cross_debounce_cycle(cfg->hw, cfg->debounce_cycles);
	analog_cmpr_ll_set_scan_mode(cfg->hw, ANA_CMPR_SCAN_MODE_FULL);
	analog_cmpr_ll_set_scan_mask(cfg->hw, cfg->scan_mask);

	analog_cmpr_ll_enable_intr(cfg->hw, ANA_CMPR_ESP32_INTR_ALL, false);
	analog_cmpr_ll_clear_intr(cfg->hw, ANA_CMPR_ESP32_INTR_ALL);

	analog_cmpr_ll_start_scan(cfg->hw);
	k_busy_wait(data->scan_wait_us);
	analog_cmpr_ll_clear_intr(cfg->hw, ANA_CMPR_ESP32_INTR_ALL);

	ret = esp_etm_channel_alloc(&data->etm_chan);
	if (ret != 0) {
		LOG_ERR("unit %u: ETM channel alloc failed (%d)", cfg->unit, ret);
		return ret;
	}

	ret = esp_intr_alloc_intrstatus(
		cfg->irq_source,
		ESP_PRIO_TO_FLAGS(cfg->irq_priority) | ESP_INT_FLAGS_CHECK(cfg->irq_flags) |
			ESP_INTR_FLAG_SHARED,
		(uint32_t)(uintptr_t)analog_cmpr_ll_get_intr_status_reg(cfg->hw),
		ANALOG_CMPR_LL_ALL_INTR_MASK(cfg->unit), ana_cmpr_esp32_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("unit %u: IRQ alloc failed (%d)", cfg->unit, ret);
		(void)esp_etm_channel_free(data->etm_chan);
		data->etm_chan = ESP_ETM_CHAN_NONE;
		return ret;
	}

	return 0;
}

static int ana_cmpr_esp32_chan_init(const struct device *dev)
{
	const struct ana_cmpr_esp32_chan_config *ccfg = dev->config;

	if (device_is_ready(ccfg->parent) == false) {
		return -ENODEV;
	}

	return 0;
}

#define ANA_CMPR_ESP32_CHAN_PTR(node) [DT_REG_ADDR(node)] = DEVICE_DT_GET(node),

#define ANA_CMPR_ESP32_PAD_BIT(node)  | BIT(DT_PROP(node, source_pad))
#define ANA_CMPR_ESP32_PAD_ADD(node)  + BIT(DT_PROP(node, source_pad))
#define ANA_CMPR_ESP32_CH_BIT(node)   | BIT(DT_REG_ADDR(node))
#define ANA_CMPR_ESP32_CH_ADD(node)   + BIT(DT_REG_ADDR(node))
#define ANA_CMPR_ESP32_SCAN_BIT(node) | BIT(DT_REG_ADDR(node))

#define ANA_CMPR_ESP32_CHAN_DEVICE(node)                                                           \
	BUILD_ASSERT(DT_REG_ADDR(node) < ANALOG_CMPR_LL_SRC_CHANNEL_NUM,                           \
		     "channel reg must be 0, 1 or 2");                                             \
	BUILD_ASSERT(DT_PROP(node, source_pad) < ANALOG_CMPR_LL_PAD_NUM,                           \
		     "source-pad must be 0..3");                                                   \
	static struct ana_cmpr_esp32_chan_data ana_cmpr_esp32_chan_data_##node;                    \
	static const struct ana_cmpr_esp32_chan_config ana_cmpr_esp32_chan_cfg_##node = {          \
		.parent = DEVICE_DT_GET(DT_PARENT(node)),                                          \
		.channel = DT_REG_ADDR(node),                                                      \
		.pad = DT_PROP(node, source_pad),                                                  \
	};                                                                                         \
	DEVICE_DT_DEFINE(node, ana_cmpr_esp32_chan_init, NULL, &ana_cmpr_esp32_chan_data_##node,   \
			 &ana_cmpr_esp32_chan_cfg_##node, POST_KERNEL,                             \
			 CONFIG_COMPARATOR_INIT_PRIORITY, &ana_cmpr_esp32_api);

#define ANA_CMPR_ESP32_HYS(inst)                                                                   \
	((DT_INST_PROP(inst, hysteresis_mv) == 120)  ? ANA_CMPR_REF_HYS_LEVEL3                     \
	 : (DT_INST_PROP(inst, hysteresis_mv) == 80) ? ANA_CMPR_REF_HYS_LEVEL2                     \
	 : (DT_INST_PROP(inst, hysteresis_mv) == 40) ? ANA_CMPR_REF_HYS_LEVEL1                     \
						     : ANA_CMPR_REF_HYS_LEVEL0)

#define ANA_CMPR_ESP32_INIT(inst)                                                                  \
	BUILD_ASSERT(ANA_CMPR_ESP32_UNIT(inst) >= 0,                                               \
		     "reg does not match a zero-detector base address");                           \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(inst) > 0, "enable at least one channel");      \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(inst) <= ANALOG_CMPR_LL_SRC_CHANNEL_NUM,        \
		     "at most three channels");                                                    \
	BUILD_ASSERT((0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ANA_CMPR_ESP32_PAD_BIT)) ==        \
			     (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ANA_CMPR_ESP32_PAD_ADD)),  \
		     "two channels share a source pad");                                           \
	BUILD_ASSERT((0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ANA_CMPR_ESP32_CH_BIT)) ==         \
			     (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ANA_CMPR_ESP32_CH_ADD)),   \
		     "two channels share a reg value");                                            \
	IF_ENABLED(DT_INST_ENUM_HAS_VALUE(inst, ref_source, external),                             \
		   (BUILD_ASSERT(DT_INST_PROP(inst, hysteresis_mv) == 0,                           \
				 "external reference has no hysteresis");                          \
		    BUILD_ASSERT(((0 DT_INST_FOREACH_CHILD_STATUS_OKAY( \
					   inst, ANA_CMPR_ESP32_PAD_BIT)) & \
				  BIT(DT_INST_PROP(inst, ext_ref_pad))) == 0,                      \
				 "a channel uses the external reference pad");)) \
	static struct ana_cmpr_esp32_data ana_cmpr_esp32_data_##inst = {                           \
		.etm_chan = ESP_ETM_CHAN_NONE,                                                     \
	};                                                                                         \
	static const struct ana_cmpr_esp32_config ana_cmpr_esp32_cfg_##inst = {                    \
		.hw = ANALOG_CMPR_LL_GET_HW(ANA_CMPR_ESP32_UNIT(inst)),                            \
		.unit = ANA_CMPR_ESP32_UNIT(inst),                                                 \
		.irq_source = DT_INST_IRQ_BY_IDX(inst, 0, irq),                                    \
		.irq_priority = DT_INST_IRQ_BY_IDX(inst, 0, priority),                             \
		.irq_flags = DT_INST_IRQ_BY_IDX(inst, 0, flags),                                   \
		.ref_source = (ana_cmpr_ref_source_t)DT_INST_ENUM_IDX(inst, ref_source),           \
		.ref_voltage = (ana_cmpr_ref_voltage_t)(DT_INST_PROP(inst, ref_voltage) / 10),     \
		.hys = ANA_CMPR_ESP32_HYS(inst),                                                   \
		.ext_ref_pad = DT_INST_PROP(inst, ext_ref_pad),                                    \
		.clk_src = DT_INST_ENUM_IDX(inst, clock_source),                                   \
		.debounce_cycles = DT_INST_PROP(inst, debounce_cycles),                            \
		.poll_period_ns = DT_INST_PROP(inst, poll_period_ns),                              \
		.resample_limit = DT_INST_PROP(inst, resample_limit),                              \
		.scan_period_us = DT_INST_PROP(inst, scan_period_us),                              \
		.scan_mask = (0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ANA_CMPR_ESP32_SCAN_BIT)), \
		.n_channels = DT_INST_CHILD_NUM_STATUS_OKAY(inst),                                 \
		.channels = {DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ANA_CMPR_ESP32_CHAN_PTR)},    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ana_cmpr_esp32_init, NULL, &ana_cmpr_esp32_data_##inst,        \
			      &ana_cmpr_esp32_cfg_##inst, POST_KERNEL,                             \
			      CONFIG_COMPARATOR_INIT_PRIORITY, NULL);                              \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ANA_CMPR_ESP32_CHAN_DEVICE)

DT_INST_FOREACH_STATUS_OKAY(ANA_CMPR_ESP32_INIT)
