/*
 * Copyright (c) 2025 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT sifli_sf32lb_trng

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

#include <register.h>

LOG_MODULE_REGISTER(entropy_sf32lb, CONFIG_ENTROPY_LOG_LEVEL);

#define TRNG_CTRL offsetof(TRNG_TypeDef, CTRL)
#define TRNG_STAT offsetof(TRNG_TypeDef, STAT)
#define TRNG_IRQ  offsetof(TRNG_TypeDef, IRQ)
#define TRNG_RAND offsetof(TRNG_TypeDef, RAND_NUM0)

#define TRNG_RAND_NUM_MAX (8U)

struct entropy_sf32lb_data {
	struct k_mutex lock;
	struct k_sem sync;
	struct ring_buf pool;
	uint8_t data[CONFIG_ENTROPY_SF32LB_POOL_SIZE];
};

struct entropy_sf32lb_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
	void (*irq_config_func)(void);
};

static void entropy_sf32lb_isr(const struct device *dev)
{
	const struct entropy_sf32lb_config *config = dev->config;
	struct entropy_sf32lb_data *data = dev->data;
	uint32_t buf[TRNG_RAND_NUM_MAX];
	uint32_t cnt;
printk("stat:0x%x\n", sys_read32(config->base + TRNG_IRQ));
}

static int entropy_sf32lb_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct entropy_sf32lb_config *config = dev->config;
	uint32_t buf[TRNG_RAND_NUM_MAX];
	uint16_t bytes;

	while (length) {
		sys_set_bit(config->base + TRNG_CTRL, TRNG_CTRL_GEN_SEED_START_Pos);
		while (!sys_test_bit(config->base + TRNG_STAT, TRNG_STAT_SEED_VALID_Pos)) {
		}

		/* Generate random data */
		sys_set_bit(config->base + TRNG_CTRL, TRNG_CTRL_GEN_RAND_NUM_START_Pos);
		while (!sys_test_bit(config->base + TRNG_STAT, TRNG_STAT_RAND_NUM_VALID_Pos)) {
		}

		for (uint8_t i = 0U; i < TRNG_RAND_NUM_MAX; i++) {
			buf[i] = sys_read32(config->base + TRNG_RAND + (i * 4U));
		}

		bytes = MIN(length, sizeof(buf));

		memcpy(buffer, buf, bytes);

		length -= bytes;
		buffer += bytes;
	}

	return 0;
}

static int entropy_sf32lb_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					  uint16_t length, uint32_t flags)
{
}

static DEVICE_API(entropy, entropy_sf32lb_api) = {
	.get_entropy = entropy_sf32lb_get_entropy,
	.get_entropy_isr = entropy_sf32lb_get_entropy_isr,
};

static int entropy_sf32lb_init(const struct device *dev)
{
	const struct entropy_sf32lb_config *config = dev->config;
	struct entropy_sf32lb_data *data = dev->data;
	int ret;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		return ret;
	}

	ring_buf_init(&data->pool, sizeof(data->data), data->data);
	k_mutex_init(&data->lock);

	sys_clear_bits(config->base + TRNG_IRQ, TRNG_IRQ_PRNG_LOCKUP_MSK |
			       TRNG_IRQ_RAND_NUM_AVAIL_MSK |
			       TRNG_IRQ_SEED_GEN_DONE_MSK);

	config->irq_config_func();

	sys_set_bit(config->base + TRNG_CTRL, TRNG_CTRL_GEN_SEED_START_Pos);

	return ret;
}

#define ENTROPY_SF32LB_DEFINE(n)                                                                   \
	static void entropy_sf32lb_irq_config_##n(void);                                           \
                                                                                                   \
	static struct entropy_sf32lb_data entropy_sf32lb_data_##n = {                              \
		.sync = Z_SEM_INITIALIZER(entropy_sf32lb_data_##n.sync, 0, TRNG_RAND_NUM_MAX),     \
	};                                                                                         \
                                                                                                   \
	static const struct entropy_sf32lb_config entropy_sf32lb_config_##n = {                    \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
		.irq_config_func = entropy_sf32lb_irq_config_##n,                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, entropy_sf32lb_init, NULL, &entropy_sf32lb_data_##n,              \
			      &entropy_sf32lb_config_##n, PRE_KERNEL_1,                            \
			      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_sf32lb_api);                  \
                                                                                                   \
	static void entropy_sf32lb_irq_config_##n(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), entropy_sf32lb_isr,         \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_SF32LB_DEFINE)
