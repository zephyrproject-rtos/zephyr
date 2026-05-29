/*
 * Copyright (c) 2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT neorv32_gptmr

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(neorv32_gptmr, CONFIG_COUNTER_LOG_LEVEL);

/* Registers */
#define NEORV32_GPTMR_CTRL         0x00
#define NEORV32_GPTMR_CTRL_EN      BIT(0)
#define NEORV32_GPTMR_CTRL_PRSC    GENMASK(3, 1)
#define NEORV32_GPTMR_CTRL_IRQ_CLR BIT(30)
#define NEORV32_GPTMR_CTRL_IRQ_PND BIT(31)

#define NEORV32_GPTMR_THRES 0x04
#define NEORV32_GPTMR_COUNT 0x08

struct neorv32_gptmr_config {
	struct counter_config_info info;
	const struct device *syscon;
	mm_reg_t base;
	uint8_t prescaler;
	void (*irq_config_func)(void);
};

struct neorv32_gptmr_data {
	struct k_spinlock lock;
	counter_top_callback_t top_callback;
	void *top_user_data;
};

static inline uint32_t neorv32_gptmr_read(const struct device *dev, uint16_t reg)
{
	const struct neorv32_gptmr_config *config = dev->config;

	return sys_read32(config->base + reg);
}

static inline void neorv32_gptmr_write(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct neorv32_gptmr_config *config = dev->config;

	sys_write32(val, config->base + reg);
}

static int neorv32_gptmr_start(const struct device *dev)
{
	struct neorv32_gptmr_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_gptmr_read(dev, NEORV32_GPTMR_CTRL);
	ctrl |= NEORV32_GPTMR_CTRL_EN;
	neorv32_gptmr_write(dev, NEORV32_GPTMR_CTRL, ctrl);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int neorv32_gptmr_stop(const struct device *dev)
{
	struct neorv32_gptmr_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_gptmr_read(dev, NEORV32_GPTMR_CTRL);
	ctrl &= ~NEORV32_GPTMR_CTRL_EN;
	neorv32_gptmr_write(dev, NEORV32_GPTMR_CTRL, ctrl);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int neorv32_gptmr_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = neorv32_gptmr_read(dev, NEORV32_GPTMR_COUNT);

	return 0;
}

static int neorv32_gptmr_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	struct neorv32_gptmr_data *data = dev->data;
	k_spinlock_key_t key;
	bool restart = false;
	uint32_t count;
	uint32_t ctrl;
	int err = 0;

	__ASSERT_NO_MSG(cfg != NULL);

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	if ((cfg->flags & ~(COUNTER_TOP_CFG_DONT_RESET | COUNTER_TOP_CFG_RESET_WHEN_LATE)) != 0U) {
		LOG_ERR("unsupported flags 0x%08x", cfg->flags);
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	ctrl = neorv32_gptmr_read(dev, NEORV32_GPTMR_CTRL);
	count = neorv32_gptmr_read(dev, NEORV32_GPTMR_COUNT);

	if ((ctrl & NEORV32_GPTMR_CTRL_EN) != 0U) {
		if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0U) {
			neorv32_gptmr_write(dev, NEORV32_GPTMR_CTRL, ctrl & ~NEORV32_GPTMR_CTRL_EN);
			restart = true;
		} else if (count >= cfg->ticks) {
			if ((cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0U) {
				neorv32_gptmr_write(dev, NEORV32_GPTMR_CTRL,
						    ctrl & ~NEORV32_GPTMR_CTRL_EN);
				restart = true;
			}

			err = -ETIME;
		}
	}

	neorv32_gptmr_write(dev, NEORV32_GPTMR_THRES, cfg->ticks);

	if (restart) {
		neorv32_gptmr_write(dev, NEORV32_GPTMR_CTRL, ctrl);
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static uint32_t neorv32_gptmr_get_pending_int(const struct device *dev)
{
	uint32_t ctrl;

	ctrl = neorv32_gptmr_read(dev, NEORV32_GPTMR_CTRL);

	return (ctrl & NEORV32_GPTMR_CTRL_IRQ_PND) != 0 ? 1 : 0;
}

static uint32_t neorv32_gptmr_get_top_value(const struct device *dev)
{
	return neorv32_gptmr_read(dev, NEORV32_GPTMR_THRES);
}

static uint32_t neorv32_gptmr_get_freq(const struct device *dev)
{
	static const uint16_t prescalers[] = { 2U, 4U, 8U, 64U, 128U, 1024U, 2048U, 4096U };
	const struct neorv32_gptmr_config *config = dev->config;
	uint32_t clk;
	int err;

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_CLK, &clk);
	if (err < 0) {
		LOG_ERR("failed to determine clock rate (err %d)", err);
		return 0U;
	}

	return clk / prescalers[config->prescaler];
}

static void neorv32_gptmr_isr(const struct device *dev)
{
	struct neorv32_gptmr_data *data = dev->data;
	counter_top_callback_t top_callback;
	void *top_user_data;
	k_spinlock_key_t key;
	uint32_t ctrl;

	key = k_spin_lock(&data->lock);

	ctrl = neorv32_gptmr_read(dev, NEORV32_GPTMR_CTRL);
	ctrl |= NEORV32_GPTMR_CTRL_IRQ_CLR;
	neorv32_gptmr_write(dev, NEORV32_GPTMR_CTRL, ctrl);

	top_callback = data->top_callback;
	top_user_data = data->top_user_data;

	k_spin_unlock(&data->lock, key);

	if (top_callback != NULL) {
		top_callback(dev, top_user_data);
	}
}

static int neorv32_gptmr_init(const struct device *dev)
{
	const struct neorv32_gptmr_config *config = dev->config;
	uint32_t features;
	uint32_t ctrl;
	int err;

	if (!device_is_ready(config->syscon)) {
		LOG_ERR("syscon device not ready");
		return -EINVAL;
	}

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_SOC, &features);
	if (err < 0) {
		LOG_ERR("failed to determine implemented features (err %d)", err);
		return -EIO;
	}

	if ((features & NEORV32_SYSINFO_SOC_IO_GPTMR) == 0) {
		LOG_ERR("neorv32 gptmr not supported");
		return -ENODEV;
	}

	/* Stop timer, set prescaler, clear any pending interrupt */
	ctrl = FIELD_PREP(NEORV32_GPTMR_CTRL_PRSC, config->prescaler) | NEORV32_GPTMR_CTRL_IRQ_CLR;
	neorv32_gptmr_write(dev, NEORV32_GPTMR_CTRL, ctrl);

	config->irq_config_func();

	return 0;
}

static DEVICE_API(counter, neorv32_gptmr_driver_api) = {
	.start = neorv32_gptmr_start,
	.stop = neorv32_gptmr_stop,
	.get_value = neorv32_gptmr_get_value,
	.set_top_value = neorv32_gptmr_set_top_value,
	.get_pending_int = neorv32_gptmr_get_pending_int,
	.get_top_value = neorv32_gptmr_get_top_value,
	.get_freq = neorv32_gptmr_get_freq,
};

#define COUNTER_NEORV32_GPTMR_INIT(n)                                                              \
	static void neorv32_gptmr_config_func_##n(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), neorv32_gptmr_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static struct neorv32_gptmr_data neorv32_gptmr_data_##n;                                   \
                                                                                                   \
	static struct neorv32_gptmr_config neorv32_gptmr_config_##n = {                            \
		.info = {                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = 0U,                                                        \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 0,                                                     \
		},                                                                                 \
		.syscon = DEVICE_DT_GET(DT_INST_PHANDLE(n, syscon)),                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.prescaler = DT_INST_ENUM_IDX(n, prescaler),                                       \
		.irq_config_func = neorv32_gptmr_config_func_##n,                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &neorv32_gptmr_init, NULL, &neorv32_gptmr_data_##n,               \
			      &neorv32_gptmr_config_##n, POST_KERNEL,                              \
			      CONFIG_COUNTER_INIT_PRIORITY, &neorv32_gptmr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_NEORV32_GPTMR_INIT)
