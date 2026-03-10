/*
 * Copyright (c) 2024, STRIM, ALC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_flexio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/misc/nxp_flexio/nxp_flexio.h>
#include <fsl_flexio.h>

LOG_MODULE_REGISTER(mcux_flexio, CONFIG_MCUX_FLEXIO_LOG_LEVEL);


struct mcux_flexio_config {
	FLEXIO_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	void (*irq_enable_func)(void);
	void (*irq_disable_func)(void);
};

typedef const struct nxp_flexio_child *nxp_flexio_map_child_t;

struct mcux_flexio_data {
	struct k_mutex lock;
	uint32_t shifter_indexes_used;
	uint32_t timer_indexes_used;
	nxp_flexio_map_child_t *map_shifter_child;
	nxp_flexio_map_child_t *map_timer_child;
	uint32_t map_shifter_child_count;
	uint32_t map_timer_child_count;
};


static int mcux_flexio_child_take_shifter_idx(const struct device *dev)
{
	struct mcux_flexio_data *data = dev->data;

	for (uint32_t i = 0; i < data->map_shifter_child_count; i++) {
		if ((data->shifter_indexes_used & BIT(i)) == 0) {
			WRITE_BIT(data->shifter_indexes_used, i, 1);
			return i;
		}
	}

	return -ENOBUFS;
}

static int mcux_flexio_child_take_timer_idx(const struct device *dev)
{
	struct mcux_flexio_data *data = dev->data;

	for (uint32_t i = 0; i < data->map_timer_child_count; i++) {
		if ((data->timer_indexes_used & BIT(i)) == 0) {
			WRITE_BIT(data->timer_indexes_used, i, 1);
			return i;
		}
	}

	return -ENOBUFS;
}

static void mcux_flexio_isr(const struct device *dev)
{
	const struct mcux_flexio_config *config = dev->config;
	struct mcux_flexio_data *data = dev->data;
	FLEXIO_Type *base = config->base;

	nxp_flexio_map_child_t *map_shifter_child = data->map_shifter_child;
	uint32_t map_shifter_child_count = data->map_shifter_child_count;
	uint32_t shifter_status_flag = FLEXIO_GetShifterStatusFlags(base);
	uint32_t shifter_error_flag = FLEXIO_GetShifterErrorFlags(base);

	/* Not all shifter interrupt is enabled. Only handle those interrupt enabled. */
	shifter_status_flag &= base->SHIFTSIEN;
	shifter_error_flag &= base->SHIFTEIEN;

	if (shifter_status_flag || shifter_error_flag) {
		for (uint32_t idx = 0; idx < map_shifter_child_count; idx++) {
			if (((shifter_status_flag | shifter_error_flag) & BIT(idx)) != 0) {
				const struct nxp_flexio_child *child = map_shifter_child[idx];

				if (child != NULL) {
					nxp_flexio_child_isr_t isr = child->isr;

					if (isr != NULL) {
						isr(child->user_data);
					}
				}
			}
		}
	}

	nxp_flexio_map_child_t *map_timer_child = data->map_timer_child;
	uint32_t map_timer_child_count = data->map_timer_child_count;
	uint32_t timer_status_flag = FLEXIO_GetTimerStatusFlags(base);

	/* Not all timer interrupt is enabled. Only handle those interrupt enabled. */
	timer_status_flag &= base->TIMIEN;

	if (timer_status_flag) {
		for (uint32_t idx = 0; idx < map_timer_child_count; idx++) {
			if ((timer_status_flag & BIT(idx)) != 0) {
				const struct nxp_flexio_child *child = map_timer_child[idx];

				if (child != NULL) {
					nxp_flexio_child_isr_t isr = child->isr;

					if (isr != NULL) {
						isr(child->user_data);
					}
				}
			}
		}
	}

	SDK_ISR_EXIT_BARRIER;
}

static int mcux_flexio_init(const struct device *dev)
{
	const struct mcux_flexio_config *config = dev->config;
	struct mcux_flexio_data *data = dev->data;
	flexio_config_t flexio_config;

	k_mutex_init(&data->lock);

	FLEXIO_GetDefaultConfig(&flexio_config);
#if !(defined(FSL_FEATURE_FLEXIO_HAS_DOZE_MODE_SUPPORT) && \
		(FSL_FEATURE_FLEXIO_HAS_DOZE_MODE_SUPPORT == 0))
	flexio_config.enableInDoze = true;
#endif

	FLEXIO_Init(config->base, &flexio_config);
	config->irq_config_func(dev);

	return 0;
}

void nxp_flexio_irq_enable(const struct device *dev)
{
	const struct mcux_flexio_config *config = dev->config;

	config->irq_enable_func();
}

void nxp_flexio_irq_disable(const struct device *dev)
{
	const struct mcux_flexio_config *config = dev->config;

	config->irq_disable_func();
}

void nxp_flexio_lock(const struct device *dev)
{
	struct mcux_flexio_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
}

void nxp_flexio_unlock(const struct device *dev)
{
	struct mcux_flexio_data *data = dev->data;

	k_mutex_unlock(&data->lock);
}

int nxp_flexio_get_rate(const struct device *dev, uint32_t *rate)
{
	const struct mcux_flexio_config *config = dev->config;

	return clock_control_get_rate(config->clock_dev, config->clock_subsys, rate);
}

int nxp_flexio_child_attach(const struct device *dev,
	const struct nxp_flexio_child *child)
{
	struct mcux_flexio_data *data = dev->data;
	const struct nxp_flexio_child_res *child_res = &child->res;

	for (uint32_t i = 0; i < child_res->shifter_count; i++) {
		int shifter_idx = mcux_flexio_child_take_shifter_idx(dev);

		if (shifter_idx < 0) {
			LOG_ERR("Failed to take shifter index: %d", shifter_idx);
			return shifter_idx;
		}
		child_res->shifter_index[i] = shifter_idx;
		data->map_shifter_child[shifter_idx] = child;
		LOG_DBG("child %p: shifter_idx[%d] is %d", child, i, shifter_idx);
	}

	for (uint32_t i = 0; i < child_res->timer_count; i++) {
		int timer_idx = mcux_flexio_child_take_timer_idx(dev);

		if (timer_idx < 0) {
			LOG_ERR("Failed to take timer index: %d", timer_idx);
			return timer_idx;
		}
		child_res->timer_index[i] = timer_idx;
		data->map_timer_child[timer_idx] = child;
		LOG_DBG("child %p: timer_idx[%d] is %d", child, i, timer_idx);
	}

	return 0;
}

#define MCUX_FLEXIO_SHIFTER_COUNT_MAX(n) \
	ARRAY_SIZE(((FLEXIO_Type *)DT_INST_REG_ADDR(n))->SHIFTCTL)

#define MCUX_FLEXIO_TIMER_COUNT_MAX(n) \
	ARRAY_SIZE(((FLEXIO_Type *)DT_INST_REG_ADDR(n))->TIMCTL)

#define MCUX_FLEXIO_INIT(n)						\
	static void mcux_flexio_irq_config_func_##n(const struct device *dev); \
	static void mcux_flexio_irq_enable_func_##n(void);		\
	static void mcux_flexio_irq_disable_func_##n(void);		\
									\
	static nxp_flexio_map_child_t					\
		nxp_flexio_map_shifter_child_##n[MCUX_FLEXIO_SHIFTER_COUNT_MAX(n)] = {0}; \
	static nxp_flexio_map_child_t					\
		nxp_flexio_map_timer_child_##n[MCUX_FLEXIO_TIMER_COUNT_MAX(n)] = {0}; \
									\
	static struct mcux_flexio_data mcux_flexio_data_##n = {		\
		.map_shifter_child = nxp_flexio_map_shifter_child_##n, \
		.map_shifter_child_count = ARRAY_SIZE(nxp_flexio_map_shifter_child_##n), \
		.map_timer_child = nxp_flexio_map_timer_child_##n, \
		.map_timer_child_count = ARRAY_SIZE(nxp_flexio_map_timer_child_##n), \
	};								\
									\
	static const struct mcux_flexio_config mcux_flexio_config_##n = { \
		.base = (FLEXIO_Type *)DT_INST_REG_ADDR(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys =						\
		(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.irq_config_func = mcux_flexio_irq_config_func_##n,	\
		.irq_enable_func = mcux_flexio_irq_enable_func_##n,	\
		.irq_disable_func = mcux_flexio_irq_disable_func_##n,	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &mcux_flexio_init,			\
				NULL,					\
				&mcux_flexio_data_##n,			\
				&mcux_flexio_config_##n,		\
				POST_KERNEL,				\
				CONFIG_MCUX_FLEXIO_INIT_PRIORITY,	\
				NULL);					\
									\
	static void mcux_flexio_irq_config_func_##n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			mcux_flexio_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
									\
	static void mcux_flexio_irq_enable_func_##n(void)		\
	{								\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
									\
	static void mcux_flexio_irq_disable_func_##n(void)		\
	{								\
		irq_disable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_FLEXIO_INIT)
