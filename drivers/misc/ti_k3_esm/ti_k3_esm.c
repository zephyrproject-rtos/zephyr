/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI K3 Error Signaling Module (ESM). Register layout matches the U-Boot
 * k3_esm driver (compatible "ti,j721e-esm"): soft-reset, per-event priority /
 * pin / interrupt enables, global ESM_EN. CPU IRQs are cfg / hi / low.
 */

#define DT_DRV_COMPAT ti_k3_esm

#include <zephyr/device.h>
#include <zephyr/drivers/misc/ti_k3_esm/ti_k3_esm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ti_k3_esm, CONFIG_TI_K3_ESM_LOG_LEVEL);

#define ESM_EN              0x08U
#define ESM_SFT_RST         0x0cU
#define ESM_LOW_PRI         0x20U
#define ESM_HI_PRI          0x24U
#define ESM_EOI             0x30U

#define ESM_KEY             0x0fU

#define ESM_EOI_CFG         0U
#define ESM_EOI_HI          1U
#define ESM_EOI_LOW         2U

#define ESM_GRP_STRIDE      0x20U
#define ESM_GRP_BASE        0x400U
#define ESM_GRP_RAW         0x00U
#define ESM_GRP_STS         0x04U
#define ESM_GRP_INTR_EN_SET 0x08U
#define ESM_GRP_INTR_EN_CLR 0x0cU
#define ESM_GRP_INT_PRIO    0x10U
#define ESM_GRP_PIN_EN_SET  0x14U
#define ESM_GRP_PIN_EN_CLR  0x18U

#define ESM_PRI_NONE        0xffffU

struct ti_k3_esm_config {
	mem_addr_t base;
	const uint16_t *boot_pins;
	uint16_t boot_pins_count;
	uint16_t max_events;
	void (*irq_config_func)(const struct device *dev);
};

struct ti_k3_esm_data {
	ti_k3_esm_callback_t cb;
	void *user_data;
	struct k_spinlock lock;
};

static inline uint32_t esm_read(const struct ti_k3_esm_config *cfg, uint32_t off)
{
	return sys_read32(cfg->base + off);
}

static inline void esm_write(const struct ti_k3_esm_config *cfg, uint32_t off, uint32_t val)
{
	sys_write32(val, cfg->base + off);
}

static inline uint32_t esm_grp_off(uint16_t event, uint32_t reg)
{
	return ESM_GRP_BASE + ((uint32_t)(event / 32U) * ESM_GRP_STRIDE) + reg;
}

static inline uint32_t esm_bit(uint16_t event)
{
	return BIT(event % 32U);
}

static int esm_check_event(const struct ti_k3_esm_config *cfg, uint16_t event)
{
	return (event < cfg->max_events) ? 0 : -EINVAL;
}

static void esm_event_setup(const struct ti_k3_esm_config *cfg, uint16_t event, bool enable)
{
	uint32_t bit = esm_bit(event);

	if (enable) {
		esm_write(cfg, esm_grp_off(event, ESM_GRP_INT_PRIO), bit);
		esm_write(cfg, esm_grp_off(event, ESM_GRP_STS), bit);
		/*
		 * Enable the CPU interrupt path only. Driving the external
		 * ERROR pin (PIN_EN) can assert SoC reset on boards that wire
		 * ESM_ERROR_RST; leave pin routing to explicit board policy.
		 */
		esm_write(cfg, esm_grp_off(event, ESM_GRP_INTR_EN_SET), bit);
	} else {
		esm_write(cfg, esm_grp_off(event, ESM_GRP_INTR_EN_CLR), bit);
		esm_write(cfg, esm_grp_off(event, ESM_GRP_PIN_EN_CLR), bit);
	}
}

static void esm_handle_prio(const struct device *dev, uint32_t pri_reg, uint32_t eoi_key)
{
	const struct ti_k3_esm_config *cfg = dev->config;
	struct ti_k3_esm_data *data = dev->data;
	ti_k3_esm_callback_t cb;
	void *user_data;
	uint32_t pri;
	uint16_t event;

	pri = esm_read(cfg, pri_reg);
	event = (uint16_t)(pri & 0xffffU);

	if (event == ESM_PRI_NONE || esm_check_event(cfg, event) != 0) {
		esm_write(cfg, ESM_EOI, eoi_key);
		return;
	}

	/* Clear raw status so level sources can re-assert. */
	esm_write(cfg, esm_grp_off(event, ESM_GRP_STS), esm_bit(event));

	cb = data->cb;
	user_data = data->user_data;
	esm_write(cfg, ESM_EOI, eoi_key);

	if (cb != NULL) {
		cb(dev, event, user_data);
	}
}

static void ti_k3_esm_isr_hi(const struct device *dev)
{
	esm_handle_prio(dev, ESM_HI_PRI, ESM_EOI_HI);
}

static void ti_k3_esm_isr_low(const struct device *dev)
{
	esm_handle_prio(dev, ESM_LOW_PRI, ESM_EOI_LOW);
}

static void ti_k3_esm_isr_cfg(const struct device *dev)
{
	const struct ti_k3_esm_config *cfg = dev->config;

	/* Clear config-error sticky bits and EOI the cfg channel. */
	esm_write(cfg, 0x14U, 0x7U);
	esm_write(cfg, ESM_EOI, ESM_EOI_CFG);
}

int ti_k3_esm_set_callback(const struct device *dev, ti_k3_esm_callback_t cb, void *user_data)
{
	struct ti_k3_esm_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	data->cb = cb;
	data->user_data = user_data;
	k_spin_unlock(&data->lock, key);
	return 0;
}

int ti_k3_esm_enable_event(const struct device *dev, uint16_t event)
{
	const struct ti_k3_esm_config *cfg = dev->config;
	int ret = esm_check_event(cfg, event);

	if (ret != 0) {
		return ret;
	}
	esm_event_setup(cfg, event, true);
	return 0;
}

int ti_k3_esm_disable_event(const struct device *dev, uint16_t event)
{
	const struct ti_k3_esm_config *cfg = dev->config;
	int ret = esm_check_event(cfg, event);

	if (ret != 0) {
		return ret;
	}
	esm_event_setup(cfg, event, false);
	return 0;
}

int ti_k3_esm_clear_event(const struct device *dev, uint16_t event)
{
	const struct ti_k3_esm_config *cfg = dev->config;
	int ret = esm_check_event(cfg, event);

	if (ret != 0) {
		return ret;
	}
	esm_write(cfg, esm_grp_off(event, ESM_GRP_STS), esm_bit(event));
	return 0;
}

int ti_k3_esm_event_pending(const struct device *dev, uint16_t event)
{
	const struct ti_k3_esm_config *cfg = dev->config;
	int ret = esm_check_event(cfg, event);
	uint32_t raw;

	if (ret != 0) {
		return ret;
	}
	raw = esm_read(cfg, esm_grp_off(event, ESM_GRP_RAW));
	return (raw & esm_bit(event)) != 0U ? 1 : 0;
}

static int ti_k3_esm_init(const struct device *dev)
{
	const struct ti_k3_esm_config *cfg = dev->config;

	/*
	 * Do not soft-reset the ESM: on K3 the WKUP instance is shared with
	 * the Device Manager. A SFT_RST here can wedge remoteproc bring-up.
	 * Clear only the events we claim, then ensure the module is enabled.
	 */
	for (uint16_t i = 0U; i < cfg->boot_pins_count; i++) {
		uint16_t event = cfg->boot_pins[i];

		if (esm_check_event(cfg, event) != 0) {
			LOG_WRN("boot pin %u out of range (max %u)", event, cfg->max_events);
			continue;
		}
		esm_event_setup(cfg, event, true);
	}

	/* ESM_EN is typically already set by system firmware; only write if asked. */
	if (cfg->boot_pins_count > 0U) {
		esm_write(cfg, ESM_EN, ESM_KEY);
	}
	cfg->irq_config_func(dev);

	LOG_INF("%s ready (%u events, %u boot pins)", dev->name, cfg->max_events,
		cfg->boot_pins_count);
	return 0;
}

#define TI_K3_ESM_IRQ_CONNECT(inst)                                                                \
	static void ti_k3_esm_##inst##_irq_config(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 0), (                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),                                      \
			    DT_INST_IRQ_BY_IDX(inst, 0, priority), ti_k3_esm_isr_cfg,               \
			    DEVICE_DT_INST_GET(inst),                                              \
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL_AT_IDX(inst, 0, flags),               \
					(DT_INST_IRQ_BY_IDX(inst, 0, flags)), (0)));               \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 0, irq));                                      \
		))                                                                                 \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 1), (                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 1, irq),                                      \
			    DT_INST_IRQ_BY_IDX(inst, 1, priority), ti_k3_esm_isr_hi,                \
			    DEVICE_DT_INST_GET(inst),                                              \
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL_AT_IDX(inst, 1, flags),               \
					(DT_INST_IRQ_BY_IDX(inst, 1, flags)), (0)));               \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 1, irq));                                      \
		))                                                                                 \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 2), (                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 2, irq),                                      \
			    DT_INST_IRQ_BY_IDX(inst, 2, priority), ti_k3_esm_isr_low,               \
			    DEVICE_DT_INST_GET(inst),                                              \
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL_AT_IDX(inst, 2, flags),               \
					(DT_INST_IRQ_BY_IDX(inst, 2, flags)), (0)));               \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 2, irq));                                      \
		))                                                                                 \
	}

#define TI_K3_ESM_BOOT_PINS(inst)                                                                  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, ti_esm_pins),                                      \
		    (static const uint16_t ti_k3_esm_##inst##_pins[] = {                           \
			     DT_INST_PROP(inst, ti_esm_pins)};),                                   \
		    (static const uint16_t ti_k3_esm_##inst##_pins[] = {};))

#define TI_K3_ESM_INIT(inst)                                                                       \
	TI_K3_ESM_BOOT_PINS(inst);                                                                 \
	TI_K3_ESM_IRQ_CONNECT(inst);                                                               \
	static struct ti_k3_esm_data ti_k3_esm_data_##inst;                                         \
	static const struct ti_k3_esm_config ti_k3_esm_config_##inst = {                            \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.boot_pins = ti_k3_esm_##inst##_pins,                                              \
		.boot_pins_count = ARRAY_SIZE(ti_k3_esm_##inst##_pins),                            \
		.max_events = DT_INST_PROP_OR(inst, max_events, 1024),                             \
		.irq_config_func = ti_k3_esm_##inst##_irq_config,                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ti_k3_esm_init, NULL, &ti_k3_esm_data_##inst,                  \
			      &ti_k3_esm_config_##inst, POST_KERNEL,                               \
			      CONFIG_TI_K3_ESM_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TI_K3_ESM_INIT)
