/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr EDAC back-end for the TI K3 ECC Aggregator. Tracks SEC (correctable)
 * and DED (uncorrectable) status/enable registers, optionally delivers a
 * notify callback from the aggregator IRQ, and exposes error counts through
 * the standard edac API. Error injection is not implemented in this first cut.
 */

#define DT_DRV_COMPAT ti_k3_ecc_aggr

#include <zephyr/device.h>
#include <zephyr/drivers/edac.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(edac_ti_k3_ecc_aggr, CONFIG_EDAC_LOG_LEVEL);

#define ECC_AGGR_REV                 0x00U
#define ECC_AGGR_SEC_EOI             0x3cU
#define ECC_AGGR_SEC_STATUS0         0x40U
#define ECC_AGGR_SEC_ENABLE_SET0     0x80U
#define ECC_AGGR_SEC_ENABLE_CLR0     0xc0U
#define ECC_AGGR_DED_EOI             0x13cU
#define ECC_AGGR_DED_STATUS0         0x140U
#define ECC_AGGR_DED_ENABLE_SET0     0x180U
#define ECC_AGGR_DED_ENABLE_CLR0     0x1c0U
#define ECC_AGGR_AGGR_ENABLE_SET     0x200U
#define ECC_AGGR_AGGR_ENABLE_CLR     0x204U
#define ECC_AGGR_AGGR_STATUS_SET     0x208U
#define ECC_AGGR_AGGR_STATUS_CLR     0x20cU

#define ECC_AGGR_ENABLE_SEC          BIT(0)
#define ECC_AGGR_ENABLE_DED          BIT(1)

struct ti_k3_ecc_config {
	mem_addr_t base;
	void (*irq_config_func)(const struct device *dev);
	bool has_irq;
};

struct ti_k3_ecc_data {
	edac_notify_callback_f cb;
	uint32_t errors_cor;
	uint32_t errors_uc;
	uint64_t last_sec_log;
	uint64_t last_ded_log;
	bool has_sec_log;
	bool has_ded_log;
};

static inline uint32_t ecc_read(const struct ti_k3_ecc_config *cfg, uint32_t off)
{
	return sys_read32(cfg->base + off);
}

static inline void ecc_write(const struct ti_k3_ecc_config *cfg, uint32_t off, uint32_t val)
{
	sys_write32(val, cfg->base + off);
}

static void ti_k3_ecc_handle(const struct device *dev)
{
	const struct ti_k3_ecc_config *cfg = dev->config;
	struct ti_k3_ecc_data *data = dev->data;
	uint32_t sec = ecc_read(cfg, ECC_AGGR_SEC_STATUS0);
	uint32_t ded = ecc_read(cfg, ECC_AGGR_DED_STATUS0);
	uint32_t aggr = ecc_read(cfg, ECC_AGGR_AGGR_STATUS_SET);
	edac_notify_callback_f cb = data->cb;

	if (sec != 0U) {
		data->errors_cor += POPCOUNT(sec);
		data->last_sec_log = sec;
		data->has_sec_log = true;
		ecc_write(cfg, ECC_AGGR_SEC_STATUS0, sec);
		ecc_write(cfg, ECC_AGGR_SEC_EOI, 0U);
		LOG_WRN("SEC status 0x%08x", sec);
	}

	if (ded != 0U) {
		data->errors_uc += POPCOUNT(ded);
		data->last_ded_log = ded;
		data->has_ded_log = true;
		ecc_write(cfg, ECC_AGGR_DED_STATUS0, ded);
		ecc_write(cfg, ECC_AGGR_DED_EOI, 0U);
		LOG_ERR("DED status 0x%08x", ded);
	}

	if (aggr != 0U) {
		ecc_write(cfg, ECC_AGGR_AGGR_STATUS_CLR, aggr);
	}

	if (cb != NULL && (sec != 0U || ded != 0U)) {
		cb(dev, NULL);
	}
}

static void ti_k3_ecc_isr(const struct device *dev)
{
	ti_k3_ecc_handle(dev);
}

static int ti_k3_ecc_log_get(const struct device *dev, uint64_t *value, bool sec)
{
	struct ti_k3_ecc_data *data = dev->data;

	/* Drain pending bits into the retained log before returning. */
	ti_k3_ecc_handle(dev);

	if (sec) {
		if (!data->has_sec_log) {
			return -ENODATA;
		}
		*value = data->last_sec_log;
	} else {
		if (!data->has_ded_log) {
			return -ENODATA;
		}
		*value = data->last_ded_log;
	}
	return 0;
}

static int ti_k3_ecc_log_clear(const struct device *dev, bool sec)
{
	const struct ti_k3_ecc_config *cfg = dev->config;
	struct ti_k3_ecc_data *data = dev->data;
	uint32_t sts;

	if (sec) {
		sts = ecc_read(cfg, ECC_AGGR_SEC_STATUS0);
		if (sts != 0U) {
			ecc_write(cfg, ECC_AGGR_SEC_STATUS0, sts);
			ecc_write(cfg, ECC_AGGR_SEC_EOI, 0U);
		}
		data->has_sec_log = false;
		data->last_sec_log = 0U;
	} else {
		sts = ecc_read(cfg, ECC_AGGR_DED_STATUS0);
		if (sts != 0U) {
			ecc_write(cfg, ECC_AGGR_DED_STATUS0, sts);
			ecc_write(cfg, ECC_AGGR_DED_EOI, 0U);
		}
		data->has_ded_log = false;
		data->last_ded_log = 0U;
	}
	return 0;
}

static int ecc_error_log_get(const struct device *dev, uint64_t *value)
{
	return ti_k3_ecc_log_get(dev, value, true);
}

static int ecc_error_log_clear(const struct device *dev)
{
	return ti_k3_ecc_log_clear(dev, true);
}

static int parity_error_log_get(const struct device *dev, uint64_t *value)
{
	/* Map DED (uncorrectable) onto the parity-log slot. */
	return ti_k3_ecc_log_get(dev, value, false);
}

static int parity_error_log_clear(const struct device *dev)
{
	return ti_k3_ecc_log_clear(dev, false);
}

static int errors_cor_get(const struct device *dev)
{
	struct ti_k3_ecc_data *data = dev->data;

	ti_k3_ecc_handle(dev);
	return (int)data->errors_cor;
}

static int errors_uc_get(const struct device *dev)
{
	struct ti_k3_ecc_data *data = dev->data;

	ti_k3_ecc_handle(dev);
	return (int)data->errors_uc;
}

static int notify_cb_set(const struct device *dev, edac_notify_callback_f cb)
{
	struct ti_k3_ecc_data *data = dev->data;

	data->cb = cb;
	return 0;
}

static int ti_k3_ecc_init(const struct device *dev)
{
	const struct ti_k3_ecc_config *cfg = dev->config;
	uint32_t rev = ecc_read(cfg, ECC_AGGR_REV);

	/* Enable aggregator-level SEC/DED reporting and unmask bank0 bits. */
	ecc_write(cfg, ECC_AGGR_AGGR_ENABLE_SET, ECC_AGGR_ENABLE_SEC | ECC_AGGR_ENABLE_DED);
	ecc_write(cfg, ECC_AGGR_SEC_ENABLE_SET0, 0xffffffffU);
	ecc_write(cfg, ECC_AGGR_DED_ENABLE_SET0, 0xffffffffU);

	/* Clear any sticky status left by firmware. */
	(void)ti_k3_ecc_log_clear(dev, true);
	(void)ti_k3_ecc_log_clear(dev, false);

	if (cfg->has_irq) {
		cfg->irq_config_func(dev);
	}

	LOG_INF("%s rev 0x%08x", dev->name, rev);
	return 0;
}

static DEVICE_API(edac, ti_k3_ecc_api) = {
	.ecc_error_log_get = ecc_error_log_get,
	.ecc_error_log_clear = ecc_error_log_clear,
	.parity_error_log_get = parity_error_log_get,
	.parity_error_log_clear = parity_error_log_clear,
	.errors_cor_get = errors_cor_get,
	.errors_uc_get = errors_uc_get,
	.notify_cb_set = notify_cb_set,
};

#define TI_K3_ECC_IRQ_CONFIG(inst)                                                                 \
	static void ti_k3_ecc_##inst##_irq_config(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), ti_k3_ecc_isr,        \
			    DEVICE_DT_INST_GET(inst),                                              \
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL(inst, flags),                         \
					(DT_INST_IRQ(inst, flags)), (0)));                         \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define TI_K3_ECC_INIT(inst)                                                                       \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(inst, 0), (TI_K3_ECC_IRQ_CONFIG(inst)), ())                \
	static struct ti_k3_ecc_data ti_k3_ecc_data_##inst;                                         \
	static const struct ti_k3_ecc_config ti_k3_ecc_config_##inst = {                            \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.has_irq = DT_INST_IRQ_HAS_IDX(inst, 0),                                           \
		.irq_config_func = COND_CODE_1(DT_INST_IRQ_HAS_IDX(inst, 0),                       \
					       (ti_k3_ecc_##inst##_irq_config), (NULL)),           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ti_k3_ecc_init, NULL, &ti_k3_ecc_data_##inst,                  \
			      &ti_k3_ecc_config_##inst, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ti_k3_ecc_api);

DT_INST_FOREACH_STATUS_OKAY(TI_K3_ECC_INIT)
