/*
 * SPDX-FileCopyrightText: <text>Copyright 2021-2022, 2024-2025 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <ethosu_driver.h>

#include "ethos_u_common.h"

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#define DT_DRV_COMPAT arm_ethos_u
LOG_MODULE_REGISTER(arm_ethos_u, CONFIG_ETHOS_U_LOG_LEVEL);

#define TA_MAXR_MASK     0x3Fu
#define TA_MAXW_MASK     0x3Fu
#define TA_MAXRW_MASK    0x3Fu
#define TA_RLATENCY_MASK 0xFFFu
#define TA_WLATENCY_MASK 0xFFFu
#define TA_PULSE_MASK    0xFFFFu
#define TA_BWCAP_MASK    0xFFFFu
#define TA_PERFCTRL_MASK 0x3Fu
#define TA_MODE_MASK     0xFFFu
#define TA_HISTBIN_MASK  0xFu

#define TA_VERSION_UNSPECIFIED UINT32_MAX
#define TA_VERSION_1_1_23      0x1117u

/* Register offsets */
#define TA_MAXR      0x00
#define TA_MAXW      0x04
#define TA_MAXRW     0x08
#define TA_RLATENCY  0x0C
#define TA_WLATENCY  0x10
#define TA_PULSE_ON  0x14
#define TA_PULSE_OFF 0x18
#define TA_BWCAP     0x1C
#define TA_PERFCTRL  0x20
#define TA_PERFCNT   0x24
#define TA_MODE      0x28
#define TA_HISTBIN   0x30
#define TA_HISTCNT   0x34
#define TA_VERSION   0x38

struct ethosu_ta_cfg {
	uintptr_t base;     /* MMIO base for the TA block */
	uint32_t version;   /* Optional: expected TA version (validation/logging) */
	uint32_t maxr;      /* 6-bit: max pending reads (0 = infinite) */
	uint32_t maxw;      /* 6-bit: max pending writes (0 = infinite) */
	uint32_t maxrw;     /* 6-bit: max combined R+W (0 = infinite) */
	uint32_t rlatency;  /* 12-bit: read latency (cycles) */
	uint32_t wlatency;  /* 12-bit: write latency (cycles) */
	uint32_t pulse_on;  /* 16-bit: burst pulse ON cycles */
	uint32_t pulse_off; /* 16-bit: burst pulse OFF cycles */
	uint32_t bwcap;     /* 16-bit: bandwidth cap (bus words / window; 0 = no cap) */
	uint32_t perfctrl;  /* 6-bit: perf control */
	uint32_t perfcnt;   /* 32-bit: perf counter preload/reset */
	uint32_t mode;      /* Mode bits (0..11). Bit 0 enables dynamic clocking. */
	uint32_t histbin;   /* 0..15: histogram bin selector */
	uint32_t histcnt;   /* 32-bit: histogram bin value */
};

#define TA_CFG_FROM_NODE(n)                                                                        \
	{                                                                                          \
		.base = DT_REG_ADDR(n),                                                            \
		.version = DT_PROP_OR(n, version, TA_VERSION_UNSPECIFIED),                         \
		.maxr = DT_PROP_OR(n, maxr, 0u),                                                   \
		.maxw = DT_PROP_OR(n, maxw, 0u),                                                   \
		.maxrw = DT_PROP_OR(n, maxrw, 0u),                                                 \
		.rlatency = DT_PROP_OR(n, rlatency, 0u),                                           \
		.wlatency = DT_PROP_OR(n, wlatency, 0u),                                           \
		.pulse_on = DT_PROP_OR(n, pulse_on, 0u),                                           \
		.pulse_off = DT_PROP_OR(n, pulse_off, 0u),                                         \
		.bwcap = DT_PROP_OR(n, bwcap, 0u),                                                 \
		.perfctrl = DT_PROP_OR(n, perfctrl, 0u),                                           \
		.perfcnt = DT_PROP_OR(n, perfcnt, 0u),                                             \
		.mode = DT_PROP_OR(n, mode, 1u),                                                   \
		.histbin = DT_PROP_OR(n, histbin, 0u),                                             \
		.histcnt = DT_PROP_OR(n, histcnt, 0u),                                             \
	}

#define TA_CFG_IF_COMPAT(n)                                                                        \
	COND_CODE_1(DT_NODE_HAS_COMPAT(n, arm_axi_timing_adapter), (TA_CFG_FROM_NODE(n)), ())

/* clang-format off */
#define ETHOSU_TA_CFGS(inst)                                                                       \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_DRV_INST(inst), TA_CFG_IF_COMPAT, (,))
/* clang-format on */

static const struct ethosu_ta_cfg g_ta_cfgs[] = {ETHOSU_TA_CFGS(0)};

static bool ethosu_ta_version_supported(uint32_t version)
{
	switch (version) {
	case TA_VERSION_1_1_23:
		return true;
	default:
		return false;
	}
}

static inline void ethosu_ta_apply(const struct ethosu_ta_cfg *c)
{
	if ((c == NULL) || (c->base == 0U)) {
		return;
	}

	const uintptr_t base = c->base;
	const uint32_t hw_version = sys_read32(base + TA_VERSION);

	if (!ethosu_ta_version_supported(hw_version)) {
		LOG_ERR("TA@0x%08" PRIxPTR " has unsupported version 0x%08x", base, hw_version);
		return;
	}

	if ((c->version != TA_VERSION_UNSPECIFIED) && (c->version != hw_version)) {
		LOG_WRN("TA@0x%08" PRIxPTR " version mismatch: DT=0x%08x HW=0x%08x", base,
			c->version, hw_version);
	}

	LOG_DBG("TA base=0x%08" PRIxPTR " ver=0x%08x maxr=%u maxw=%u maxrw=%u rlat=%u wlat=%u "
		"pulse_on=%u pulse_off=%u bwcap=%u perfctrl=%u perfcnt=0x%08x mode=%u "
		"histbin=%u histcnt=%u",
		base, hw_version, c->maxr, c->maxw, c->maxrw, c->rlatency, c->wlatency, c->pulse_on,
		c->pulse_off, c->bwcap, c->perfctrl, c->perfcnt, c->mode, c->histbin, c->histcnt);
	sys_write32((c->maxr & TA_MAXR_MASK), base + TA_MAXR);
	sys_write32((c->maxw & TA_MAXW_MASK), base + TA_MAXW);
	sys_write32((c->maxrw & TA_MAXRW_MASK), base + TA_MAXRW);
	sys_write32((c->rlatency & TA_RLATENCY_MASK), base + TA_RLATENCY);
	sys_write32((c->wlatency & TA_WLATENCY_MASK), base + TA_WLATENCY);
	sys_write32((c->pulse_on & TA_PULSE_MASK), base + TA_PULSE_ON);
	sys_write32((c->pulse_off & TA_PULSE_MASK), base + TA_PULSE_OFF);
	sys_write32((c->bwcap & TA_BWCAP_MASK), base + TA_BWCAP);
	sys_write32((c->perfctrl & TA_PERFCTRL_MASK), base + TA_PERFCTRL);
	sys_write32(c->perfcnt, base + TA_PERFCNT);
	sys_write32((c->mode & TA_MODE_MASK), base + TA_MODE);
	sys_write32((c->histbin & TA_HISTBIN_MASK), base + TA_HISTBIN);
	sys_write32(c->histcnt, base + TA_HISTCNT);
}

/* DT helpers: use fast-memory-region if present; else NULL/0. */
#define ETHOSU_FAST_BASE(n)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, fast_memory_region), \
		((void *)DT_REG_ADDR(DT_INST_PHANDLE(n, fast_memory_region))), \
		(NULL))

#define ETHOSU_FAST_SIZE(n)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, fast_memory_region), \
		((size_t)DT_REG_SIZE(DT_INST_PHANDLE(n, fast_memory_region))), \
		(0u))

void ethosu_zephyr_irq_handler(const struct device *dev)
{
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;

	ethosu_irq_handler(drv);
}

static int ethosu_zephyr_init(const struct device *dev)
{
	const struct ethosu_dts_info *config = dev->config;
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;
	struct ethosu_driver_version version;

	LOG_DBG("Ethos-U DTS info. base_address=%p, fast_mem=%p, fast_size=%zu, secure_enable=%u, "
		"privilege_enable=%u",
		config->base_addr, config->fast_mem_base, config->fast_mem_size,
		config->secure_enable, config->privilege_enable);

	ethosu_get_driver_version(&version);

	LOG_DBG("Version. major=%u, minor=%u, patch=%u", version.major, version.minor,
		version.patch);
	for (size_t i = 0; i < ARRAY_SIZE(g_ta_cfgs); ++i) {
		LOG_DBG("TA[%zu] base=0x%08" PRIxPTR, i, g_ta_cfgs[i].base);
		ethosu_ta_apply(&g_ta_cfgs[i]);
	}

	if (ethosu_init(drv, config->base_addr, config->fast_mem_base, config->fast_mem_size,
			config->secure_enable, config->privilege_enable)) {
		LOG_ERR("Failed to initialize NPU with ethosu_init().");
		return -EINVAL;
	}

	config->irq_config();

	return 0;
}

#define ETHOSU_DEVICE_INIT(n)                                                                      \
	static struct ethosu_data ethosu_data_##n;                                                 \
                                                                                                   \
	static void ethosu_zephyr_irq_config_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ethosu_zephyr_irq_handler,  \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ethosu_dts_info ethosu_dts_info_##n = {                                \
		.base_addr = (void *)DT_INST_REG_ADDR(n),                                          \
		.secure_enable = DT_INST_PROP(n, secure_enable),                                   \
		.privilege_enable = DT_INST_PROP(n, privilege_enable),                             \
		.irq_config = &ethosu_zephyr_irq_config_##n,                                       \
		.fast_mem_base = ETHOSU_FAST_BASE(n),                                              \
		.fast_mem_size = ETHOSU_FAST_SIZE(n),                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ethosu_zephyr_init, NULL, &ethosu_data_##n, &ethosu_dts_info_##n, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(ETHOSU_DEVICE_INIT);
