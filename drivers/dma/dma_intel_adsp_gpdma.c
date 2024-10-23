/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adsp_interrupt.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/cache.h>

#define DT_DRV_COMPAT intel_adsp_gpdma

#define GPDMA_CTL_OFFSET 0x0004
#define GPDMA_CTL_FDCGB BIT(0)
#define GPDMA_CTL_DCGD BIT(30)

/* TODO make device tree defined? */
#define GPDMA_CHLLPC_OFFSET(channel) (0x0010 + channel*0x10)
#define GPDMA_CHLLPC_EN BIT(7)
#define GPDMA_CHLLPC_DHRS(x) SET_BITS(6, 0, x)

/* TODO make device tree defined? */
#define GPDMA_CHLLPL(channel) (0x0018 + channel*0x10)
#define GPDMA_CHLLPU(channel) (0x001c + channel*0x10)

#define GPDMA_OSEL(x) SET_BITS(25, 24, x)
#define SHIM_CLKCTL_LPGPDMA_SPA	BIT(0)
#define SHIM_CLKCTL_LPGPDMA_CPA	BIT(8)

# define DSP_INIT_LPGPDMA(x)	  (0x71A60 + (2*x))
# define LPGPDMA_CTLOSEL_FLAG	  BIT(15)
# define LPGPDMA_CHOSEL_FLAG	  0xFF

#include "dma_dw_common.h"
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dma_intel_adsp_gpdma);


/* Device run time data */
struct intel_adsp_gpdma_data {
	struct dw_dma_dev_data dw_data;
};

/* Device constant configuration parameters */
struct intel_adsp_gpdma_cfg {
	struct dw_dma_dev_cfg dw_cfg;
	uint32_t shim;
};

#ifdef DMA_INTEL_ADSP_GPDMA_DEBUG
static void intel_adsp_gpdma_dump_registers(const struct device *dev, uint32_t channel)
{
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	const struct dw_dma_dev_cfg *const dw_cfg = &dev_cfg->dw_cfg;
	uint32_t cap, ctl, ipptr, llpc, llpl, llpu;
	int i;

	/* Shims */
	cap = dw_read(dev_cfg->shim, 0x0);
	ctl = dw_read(dev_cfg->shim, 0x4);
	ipptr = dw_read(dev_cfg->shim, 0x8);
	llpc = dw_read(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel));
	llpl = dw_read(dev_cfg->shim, GPDMA_CHLLPL(channel));
	llpu = dw_read(dev_cfg->shim, GPDMA_CHLLPU(channel));

	LOG_INF("%s: channel: %d cap %x, ctl %x, ipptr %x, llpc %x, llpl %x, llpu %x", dev->name,
		channel, cap, ctl, ipptr, llpc, llpl, llpu);

	/* Channel Register Dump */
	for (i = 0; i <= DW_DMA_CHANNEL_REGISTER_OFFSET_END; i += 0x8) {
		LOG_INF(" channel register offset: %#x value: %#x\n", chan_reg_offs[i],
			dw_read(dw_cfg->base, DW_CHAN_OFFSET(channel) + chan_reg_offs[i]));
	}

	/* IP Register Dump */
	for (i = DW_DMA_CHANNEL_REGISTER_OFFSET_START; i <= DW_DMA_CHANNEL_REGISTER_OFFSET_END;
	     i += 0x8) {
		LOG_INF(" ip register offset: %#x value: %#x\n", ip_reg_offs[i],
			dw_read(dw_cfg->base, ip_reg_offs[i]));
	}
}
#endif

static void intel_adsp_gpdma_llp_config(const struct device *dev,
					uint32_t channel, uint32_t dma_slot)
{
#ifdef CONFIG_DMA_INTEL_ADSP_GPDMA_HAS_LLP
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;

	dw_write(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel),
		 GPDMA_CHLLPC_DHRS(dma_slot));
#endif
}

static inline void intel_adsp_gpdma_llp_enable(const struct device *dev,
					       uint32_t channel)
{
#ifdef CONFIG_DMA_INTEL_ADSP_GPDMA_HAS_LLP
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t val;

	val = dw_read(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel));
	if (!(val & GPDMA_CHLLPC_EN)) {
		dw_write(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel),
			 val | GPDMA_CHLLPC_EN);
	}
#endif
}

static inline void intel_adsp_gpdma_llp_disable(const struct device *dev,
						uint32_t channel)
{
#ifdef CONFIG_DMA_INTEL_ADSP_GPDMA_HAS_LLP
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t val;

	val = dw_read(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel));
	dw_write(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel),
		 val | GPDMA_CHLLPC_EN);
#endif
}

static inline void intel_adsp_gpdma_llp_read(const struct device *dev,
					uint32_t channel, uint32_t *llp_l,
					uint32_t *llp_u)
{
#ifdef CONFIG_DMA_INTEL_ADSP_GPDMA_HAS_LLP
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t tmp;

	tmp = dw_read(dev_cfg->shim, GPDMA_CHLLPL(channel));
	*llp_u = dw_read(dev_cfg->shim, GPDMA_CHLLPU(channel));
	*llp_l = dw_read(dev_cfg->shim, GPDMA_CHLLPL(channel));
	if (tmp > *llp_l) {
		/* re-read the LLPU value, as LLPL just wrapped */
		*llp_u = dw_read(dev_cfg->shim, GPDMA_CHLLPU(channel));
	}
#endif
}


static int intel_adsp_gpdma_config(const struct device *dev, uint32_t channel,
				struct dma_config *cfg)
{
	int res = dw_dma_config(dev, channel, cfg);

	if (res != 0) {
		return res;
	}

	/* Assume all scatter/gathers are for the same device? */
	switch (cfg->channel_direction) {
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
		LOG_DBG("%s: channel %d configuring llp for %x", dev->name, channel, cfg->dma_slot);
		intel_adsp_gpdma_llp_config(dev, channel, cfg->dma_slot);
		break;
	default:
		break;
	}

	return res;
}

static int intel_adsp_gpdma_start(const struct device *dev, uint32_t channel)
{
	int ret = 0;
#if CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	bool first_use = false;
	enum pm_device_state state;

	/* We need to power-up device before using it. So in case of a GPDMA, we need to check if
	 * the current instance is already active, and if not, we let the power manager know that
	 * we want to use it.
	 */
	if (pm_device_state_get(dev, &state) != -ENOSYS) {
		first_use = state != PM_DEVICE_STATE_ACTIVE;
		if (first_use) {
			ret = pm_device_runtime_get(dev);
			if (ret < 0) {
				return ret;
			}
		}
	}
#endif

	intel_adsp_gpdma_llp_enable(dev, channel);
	ret = dw_dma_start(dev, channel);
	if (ret != 0) {
		intel_adsp_gpdma_llp_disable(dev, channel);
	}

#if CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	/* Device usage is counted by the calls of dw_dma_start and dw_dma_stop. For the first use,
	 * we need to make sure that the pm_device_runtime_get and pm_device_runtime_put functions
	 * calls are balanced.
	 */
	if (first_use) {
		ret = pm_device_runtime_put(dev);
	}
#endif

	return ret;
}

static int intel_adsp_gpdma_stop(const struct device *dev, uint32_t channel)
{
	int ret = dw_dma_stop(dev, channel);

	if (ret == 0) {
		intel_adsp_gpdma_llp_disable(dev, channel);
	}

	return ret;
}

static int intel_adsp_gpdma_copy(const struct device *dev, uint32_t channel,
		    uint32_t src, uint32_t dst, size_t size)
{
	struct dw_dma_dev_data *const dev_data = dev->data;
	struct dw_dma_chan_data *chan_data;

	if (channel >= DW_MAX_CHAN) {
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];

	/* default action is to clear the DONE bit for all LLI making
	 * sure the cache is coherent between DSP and DMAC.
	 */
	for (int i = 0; i < chan_data->lli_count; i++) {
		chan_data->lli[i].ctrl_hi &= ~DW_CTLH_DONE(1);
	}

	chan_data->ptr_data.current_ptr += size;
	if (chan_data->ptr_data.current_ptr >= chan_data->ptr_data.end_ptr) {
		chan_data->ptr_data.current_ptr = chan_data->ptr_data.start_ptr +
			(chan_data->ptr_data.current_ptr - chan_data->ptr_data.end_ptr);
	}

	return 0;
}

/* Disables automatic clock gating (force disable clock gate) */
static void intel_adsp_gpdma_clock_enable(const struct device *dev)
{
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;
	uint32_t val;

	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_ADSP_ACE)) {
		val = sys_read32(reg) | GPDMA_CTL_DCGD;
	} else {
		val = GPDMA_CTL_FDCGB;
	}

	sys_write32(val, reg);
}

#ifdef CONFIG_PM_DEVICE
static void intel_adsp_gpdma_clock_disable(const struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;
	uint32_t val = sys_read32(reg) & ~GPDMA_CTL_DCGD;

	sys_write32(val, reg);
#endif
}
#endif

static void intel_adsp_gpdma_claim_ownership(const struct device *dev)
{
#ifdef CONFIG_DMA_INTEL_ADSP_GPDMA_NEED_CONTROLLER_OWNERSHIP
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;
	uint32_t val = sys_read32(reg) | GPDMA_OSEL(0x3);

	sys_write32(val, reg);
#else
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG, DSP_INIT_LPGPDMA(0));
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG, DSP_INIT_LPGPDMA(1));
	ARG_UNUSED(dev);
#endif /* CONFIG_SOC_SERIES_INTEL_ADSP_ACE */
#endif /* CONFIG_DMA_INTEL_ADSP_GPDMA_NEED_CONTROLLER_OWNERSHIP */
}

#ifdef CONFIG_PM_DEVICE
static void intel_adsp_gpdma_release_ownership(const struct device *dev)
{
#ifdef CONFIG_DMA_INTEL_ADSP_GPDMA_NEED_CONTROLLER_OWNERSHIP
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;
	uint32_t val = sys_read32(reg) & ~GPDMA_OSEL(0x3);

	sys_write32(val, reg);
	/* CHECKME: Do CAVS platforms set ownership over DMA,
	 * if yes, add support for it releasing.
	 */
#endif /* CONFIG_SOC_SERIES_INTEL_ADSP_ACE */
#endif /* CONFIG_DMA_INTEL_ADSP_GPDMA_NEED_CONTROLLER_OWNERSHIP */
}
#endif

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
static int intel_adsp_gpdma_enable(const struct device *dev)
{
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;

	sys_write32(SHIM_CLKCTL_LPGPDMA_SPA, reg);

	if (!WAIT_FOR((sys_read32(reg) & SHIM_CLKCTL_LPGPDMA_CPA), 10000,
		      k_busy_wait(1))) {
		return -1;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int intel_adsp_gpdma_disable(const struct device *dev)
{
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;

	sys_write32(sys_read32(reg) & ~SHIM_CLKCTL_LPGPDMA_SPA, reg);
	return 0;
}
#endif /* CONFIG_PM_DEVICE */
#endif /* CONFIG_SOC_SERIES_INTEL_ADSP_ACE */

static int intel_adsp_gpdma_power_on(const struct device *dev)
{
	const struct intel_adsp_gpdma_cfg *const dev_cfg = dev->config;
	int ret;

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	/* Power up */
	ret = intel_adsp_gpdma_enable(dev);

	if (ret != 0) {
		LOG_ERR("%s: failed to initialize", dev->name);
		goto out;
	}
#endif

	/* DW DMA Owner Select to DSP */
	intel_adsp_gpdma_claim_ownership(dev);

	/* Disable dynamic clock gating appropriately before initializing */
	intel_adsp_gpdma_clock_enable(dev);

	/* Disable all channels and Channel interrupts */
	ret = dw_dma_setup(dev);
	if (ret != 0) {
		LOG_ERR("%s: failed to initialize", dev->name);
		goto out;
	}

	/* Configure interrupts */
	dev_cfg->dw_cfg.irq_config();

	LOG_INF("%s: initialized", dev->name);

out:
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int intel_adsp_gpdma_power_off(const struct device *dev)
{
	LOG_INF("%s: power off", dev->name);
	/* Enabling dynamic clock gating */
	intel_adsp_gpdma_clock_disable(dev);

	/* Relesing DMA ownership*/
	intel_adsp_gpdma_release_ownership(dev);
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	/* Power down */
	return intel_adsp_gpdma_disable(dev);
#else
	return 0;
#endif /* CONFIG_SOC_SERIES_INTEL_ADSP_ACE */
}
#endif /* CONFIG_PM_DEVICE */

int intel_adsp_gpdma_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	uint32_t llp_l = 0;
	uint32_t llp_u = 0;

	if (channel >= DW_MAX_CHAN) {
		return -EINVAL;
	}

	intel_adsp_gpdma_llp_read(dev, channel, &llp_l, &llp_u);
	stat->total_copied = ((uint64_t)llp_u << 32) | llp_l;

	return dw_dma_get_status(dev, channel, stat);
}

int intel_adsp_gpdma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = sys_cache_data_line_size_get();
		break;
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = DMA_BUF_SIZE_ALIGNMENT(DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_gpdma));
		break;
	case DMA_ATTR_COPY_ALIGNMENT:
		*value = DMA_COPY_ALIGNMENT(DT_COMPAT_GET_ANY_STATUS_OKAY(intel_adsp_gpdma));
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = CONFIG_DMA_DW_LLI_POOL_SIZE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
static inline void ace_gpdma_intc_unmask(void)
{
	ACE_DINT[0].ie[ACE_INTL_GPDMA] = BIT(0);
}
#else
static inline void ace_gpdma_intc_unmask(void) {}
#endif


int intel_adsp_gpdma_init(const struct device *dev)
{
	struct dw_dma_dev_data *const dev_data = dev->data;

	/* Setup context and atomics for channels */
	dev_data->dma_ctx.magic = DMA_MAGIC;
	dev_data->dma_ctx.dma_channels = DW_MAX_CHAN;
	dev_data->dma_ctx.atomic = dev_data->channels_atomic;

	ace_gpdma_intc_unmask();

#if CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	if (pm_device_on_power_domain(dev)) {
		pm_device_init_off(dev);
	} else {
		pm_device_init_suspended(dev);
	}

	return 0;
#else
	return intel_adsp_gpdma_power_on(dev);
#endif
}
#ifdef CONFIG_PM_DEVICE
static int gpdma_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return intel_adsp_gpdma_power_on(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return intel_adsp_gpdma_power_off(dev);
	/* ON and OFF actions are used only by the power domain to change internal power status of
	 * the device. OFF state mean that device and its power domain are disabled, SUSPEND mean
	 * that device is power off but domain is already power on.
	 */
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static const struct dma_driver_api intel_adsp_gpdma_driver_api = {
	.config = intel_adsp_gpdma_config,
	.reload = intel_adsp_gpdma_copy,
	.start = intel_adsp_gpdma_start,
	.stop = intel_adsp_gpdma_stop,
	.suspend = dw_dma_suspend,
	.resume = dw_dma_resume,
	.get_status = intel_adsp_gpdma_get_status,
	.get_attribute = intel_adsp_gpdma_get_attribute,
};

#define INTEL_ADSP_GPDMA_CHAN_ARB_DATA(inst)				\
	static struct dw_drv_plat_data dmac##inst = {			\
		.chan[0] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[1] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[2] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[3] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[4] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[5] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[6] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[7] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
	}

#define INTEL_ADSP_GPDMA_INIT(inst)					\
	INTEL_ADSP_GPDMA_CHAN_ARB_DATA(inst);				\
	static void intel_adsp_gpdma##inst##_irq_config(void);		\
									\
	static const struct intel_adsp_gpdma_cfg intel_adsp_gpdma##inst##_config = {\
		.dw_cfg = {						\
			.base = DT_INST_REG_ADDR(inst),		\
			.irq_config = intel_adsp_gpdma##inst##_irq_config,\
		},							\
		.shim = DT_INST_PROP_BY_IDX(inst, shim, 0),		\
	};								\
									\
	static struct intel_adsp_gpdma_data intel_adsp_gpdma##inst##_data = {\
		.dw_data = {						\
			.channel_data = &dmac##inst,			\
		},							\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(inst, gpdma_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			      &intel_adsp_gpdma_init,			\
			      PM_DEVICE_DT_INST_GET(inst),		\
			      &intel_adsp_gpdma##inst##_data,		\
			      &intel_adsp_gpdma##inst##_config, POST_KERNEL,\
			      CONFIG_DMA_INIT_PRIORITY,		\
			      &intel_adsp_gpdma_driver_api);		\
									\
	static void intel_adsp_gpdma##inst##_irq_config(void)		\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),			\
			    DT_INST_IRQ(inst, priority), dw_dma_isr,	\
			    DEVICE_DT_INST_GET(inst),			\
			    DT_INST_IRQ(inst, sense));			\
		irq_enable(DT_INST_IRQN(inst));			\
	}

DT_INST_FOREACH_STATUS_OKAY(INTEL_ADSP_GPDMA_INIT)
