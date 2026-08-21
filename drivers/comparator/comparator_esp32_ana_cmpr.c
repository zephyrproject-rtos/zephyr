/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_ana_cmpr

/* Include hal_espressif headers first to avoid redefining BIT() macro */
#include <hal/ana_cmpr_ll.h>
#include <hal/ana_cmpr_periph.h>
#include <esp_private/gpio.h>
#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ana_cmpr_esp32, CONFIG_COMPARATOR_LOG_LEVEL);

struct ana_cmpr_esp32_config {
	analog_cmpr_dev_t *hw;
	uint8_t unit;
	void (*irq_configure)(void);
	ana_cmpr_ref_source_t ref_source;
	ana_cmpr_ref_voltage_t ref_voltage;
	uint32_t debounce_cycles;
};

struct ana_cmpr_esp32_data {
	comparator_callback_t callback;
	void *user_data;
	uint32_t intr_mask;
	uint8_t pending;
};

static int ana_cmpr_esp32_get_output(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * This IP revision (v1: P4/C5/H2) only latches edge-cross events;
	 * there is no register reflecting the instantaneous source-vs-reference
	 * comparison result. The source pad's digital input buffer is also
	 * disabled by ana_cmpr_esp32_init() since the comparator senses it
	 * directly in the analog domain, so a digital readback of the pad is
	 * not a viable substitute either. Live output polling is therefore not
	 * supported on this hardware generation.
	 */
	return -ENOTSUP;
}

static int ana_cmpr_esp32_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct ana_cmpr_esp32_config *config = dev->config;
	struct ana_cmpr_esp32_data *data = dev->data;
	ana_cmpr_cross_type_t cross_type;
	uint32_t mask;
	unsigned int key;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		cross_type = ANA_CMPR_CROSS_DISABLE;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		cross_type = ANA_CMPR_CROSS_POS;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		cross_type = ANA_CMPR_CROSS_NEG;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		cross_type = ANA_CMPR_CROSS_ANY;
		break;
	default:
		return -EINVAL;
	}

	mask = (cross_type == ANA_CMPR_CROSS_DISABLE)
		       ? 0
		       : analog_cmpr_ll_get_intr_mask_by_type(config->unit, 0, cross_type);

	key = irq_lock();

	analog_cmpr_ll_enable_intr(config->hw, data->intr_mask, false);

#if !ANALOG_CMPR_LL_SUPPORT(EDGE_SPECIFIC_INTR_MASK)
	/* Chips without per-edge interrupt masks (e.g. H2) select the cross
	 * type to detect via a single mode field instead.
	 */
	analog_cmpr_ll_set_intr_cross_type(config->hw, cross_type);
#endif

	data->intr_mask = mask;
	data->pending = 0;

	if (mask != 0) {
		/*
		 * Discard whatever this mask's bit(s) may have latched into
		 * int_raw while masked off (int_raw latches independently of
		 * int_ena), so enabling int_ena below can't immediately make
		 * ana_cmpr_esp32_isr() fire on stale state instead of a real
		 * new crossing.
		 */
		analog_cmpr_ll_clear_intr(config->hw, mask);
		analog_cmpr_ll_enable_intr(config->hw, mask, true);
	}

	irq_unlock(key);

	return 0;
}

static int ana_cmpr_esp32_set_trigger_callback(const struct device *dev,
					       comparator_callback_t callback, void *user_data)
{
	struct ana_cmpr_esp32_data *data = dev->data;
	unsigned int key;
	int pending;

	key = irq_lock();
	data->callback = callback;
	data->user_data = user_data;
	pending = data->pending;
	data->pending = 0;
	irq_unlock(key);

	if (callback != NULL && pending) {
		callback(dev, user_data);
	}

	return 0;
}

static int ana_cmpr_esp32_trigger_is_pending(const struct device *dev)
{
	struct ana_cmpr_esp32_data *data = dev->data;
	unsigned int key;
	int pending;

	/*
	 * ana_cmpr_esp32_isr() is the only piece of code that ever touches
	 * the hardware status registers; this just reads back the latest
	 * status it saved into software state.
	 */
	key = irq_lock();
	pending = data->pending;
	data->pending = 0;
	irq_unlock(key);

	return pending;
}

static void IRAM_ATTR ana_cmpr_esp32_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct ana_cmpr_esp32_config *config = dev->config;
	struct ana_cmpr_esp32_data *data = dev->data;
	uint32_t status;

	/*
	 * int_ena only ever has the currently-armed mask's bit(s) enabled
	 * (see ana_cmpr_esp32_set_trigger()), so int_st -- masked by int_ena
	 * in hardware -- already reflects only the crossing(s) the caller
	 * actually asked to be notified about; this is the only place that
	 * ever reads it, since ana_cmpr_esp32_trigger_is_pending() just
	 * returns what got saved here.
	 */
	status = analog_cmpr_ll_get_intr_status(config->hw);
	if (status == 0) {
		return;
	}

	analog_cmpr_ll_clear_intr(config->hw, status);

	if (data->callback != NULL) {
		data->callback(dev, data->user_data);
	} else {
		data->pending = 1;
	}
}

static int ana_cmpr_esp32_init(const struct device *dev)
{
	const struct ana_cmpr_esp32_config *config = dev->config;
	int ret;

	/*
	 * The source pad (and, for an external reference, the ext-ref pad) is
	 * sensed directly by the comparator's analog front-end. Float it by
	 * disabling the digital input/output buffers and pull resistors,
	 * exactly like ESP-IDF's own ana_cmpr driver does.
	 */
	ret = gpio_config_as_analog(ana_cmpr_periph[config->unit].src_gpio);
	if (ret != 0) {
		LOG_ERR("unit %d: failed to configure source GPIO as analog (%d)", config->unit,
			ret);
		return -EIO;
	}

	if (config->ref_source == ANA_CMPR_REF_SRC_EXTERNAL) {
		ret = gpio_config_as_analog(ana_cmpr_periph[config->unit].ext_ref_gpio);
		if (ret != 0) {
			LOG_ERR("unit %d: failed to configure external reference GPIO as analog "
				"(%d)",
				config->unit, ret);
			return -EIO;
		}
	}

	analog_cmpr_ll_set_ref_source(config->hw, config->ref_source);
	if (config->ref_source == ANA_CMPR_REF_SRC_INTERNAL) {
		analog_cmpr_ll_set_internal_ref_voltage(config->hw, config->ref_voltage);
	}
	analog_cmpr_ll_set_cross_debounce_cycle(config->hw, config->debounce_cycles);

	analog_cmpr_ll_enable_intr(config->hw, ANALOG_CMPR_LL_ALL_INTR_MASK(config->unit), false);
	analog_cmpr_ll_clear_intr(config->hw, ANALOG_CMPR_LL_ALL_INTR_MASK(config->unit));

	analog_cmpr_ll_enable(config->hw, true);

	/*
	 * Where a unit shares its INTMUX source with another unit (P4), the
	 * devicetree gives it a level-3 aggregator and one IRQ per cross-type
	 * bit, so z_soc_3rd_lvl_isr does the demux that ESP_INTR_FLAG_SHARED
	 * used to. Pre-multilevel path, kept for reference:
	 *
	 * ret = esp_intr_alloc_intrstatus(
	 *         config->irq_source,
	 *         ESP_PRIO_TO_FLAGS(config->irq_priority) |
	 *                 ESP_INT_FLAGS_CHECK(config->irq_flags) |
	 *                 ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM,
	 *         (uint32_t)(uintptr_t)analog_cmpr_ll_get_intr_status_reg(config->hw),
	 *         ANALOG_CMPR_LL_ALL_INTR_MASK(config->unit), ana_cmpr_esp32_isr, (void *)dev,
	 *         &irq_handle);
	 * if (ret != 0) {
	 *         LOG_ERR("unit %d: failed to allocate IRQ (%d)", config->unit, ret);
	 *         return ret;
	 * }
	 */
	config->irq_configure();

	return 0;
}

static DEVICE_API(comparator, ana_cmpr_esp32_api) = {
	.get_output = ana_cmpr_esp32_get_output,
	.set_trigger = ana_cmpr_esp32_set_trigger,
	.set_trigger_callback = ana_cmpr_esp32_set_trigger_callback,
	.trigger_is_pending = ana_cmpr_esp32_trigger_is_pending,
};

/* clang-format off */

/*
 * One IRQ per devicetree entry: a single INTMUX source where the unit owns it
 * outright (C5), or one level-3 flag per cross type where two units share it
 * (P4). Either way every entry lands on the same handler.
 */
#define ANA_CMPR_ESP32_IRQ_CONNECT(idx, inst)                                                    \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(inst, idx), IRQ_DEFAULT_PRIORITY,                          \
		    ana_cmpr_esp32_isr, DEVICE_DT_INST_GET(inst), ESP_INTR_FLAG_IRAM);             \
	irq_enable(DT_INST_IRQN_BY_IDX(inst, idx));

#define ANA_CMPR_ESP32_DEVICE(inst)                                                              \
	BUILD_ASSERT(DT_INST_PROP(inst, unit) < ANALOG_CMPR_LL_INST_NUM,                          \
		     "invalid ana_cmpr unit index");                                              \
                                                                                                   \
	static struct ana_cmpr_esp32_data ana_cmpr_esp32_data_##inst;                             \
                                                                                                   \
	static void ana_cmpr_esp32_irq_configure_##inst(void)                                     \
	{                                                                                          \
		LISTIFY(DT_INST_NUM_IRQS(inst), ANA_CMPR_ESP32_IRQ_CONNECT, (), inst)              \
	}                                                                                          \
                                                                                                   \
	static const struct ana_cmpr_esp32_config ana_cmpr_esp32_config_##inst = {                \
		.hw = ANALOG_CMPR_LL_GET_HW(DT_INST_PROP(inst, unit)),                             \
		.unit = DT_INST_PROP(inst, unit),                                                  \
		.irq_configure = ana_cmpr_esp32_irq_configure_##inst,                              \
		.ref_source = (ana_cmpr_ref_source_t)DT_INST_ENUM_IDX(inst, ref_source),           \
		.ref_voltage = (ana_cmpr_ref_voltage_t)(DT_INST_PROP(inst, ref_voltage) / 10),     \
		.debounce_cycles = DT_INST_PROP(inst, debounce_cycles),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ana_cmpr_esp32_init, NULL, &ana_cmpr_esp32_data_##inst,       \
			      &ana_cmpr_esp32_config_##inst, POST_KERNEL,                         \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &ana_cmpr_esp32_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(ANA_CMPR_ESP32_DEVICE)
