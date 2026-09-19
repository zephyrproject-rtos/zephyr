/*
 * Copyright (c) 2026, Muhammed Asif P
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/misc/mchp_evsys/mchp_evsys_g1.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_evsys_g1

LOG_MODULE_REGISTER(mchp_evsys_g1, CONFIG_MCHP_EVSYS_LOG_LEVEL);

struct evsys_mchp_clock {
	const struct device *clk_dev;
	clock_control_subsys_t clk_mclk;
	clock_control_subsys_t clk_gclk[EVSYS_SYNCH_NUM];
};

struct evsys_mchp_route {
	uint8_t channel;
	uint32_t evgen;
	const uint32_t *users;
	uint8_t user_count;
};

struct evsys_mchp_config {
	evsys_registers_t *regs;
	struct evsys_mchp_clock evsys_clock;
	void (*irq_config_func)(const struct device *dev);
	const struct evsys_mchp_route *routes;
	size_t route_count;
	uint8_t max_channels;
};

struct evsys_mchp_data {
	uint32_t channel_usage_status;
};

static void evsys_mchp_isr(const struct device *dev)
{
	const struct evsys_mchp_config *config = dev->config;
	struct evsys_mchp_data *data = dev->data;
	(void)config;
	(void)data;
}

int evsys_mchp_connect_channel_to_evgen(const struct device *dev, uint8_t channel_num, int ev_gen)
{
	const struct evsys_mchp_config *config = dev->config;
	struct evsys_mchp_data *data = dev->data;
	evsys_registers_t *regs = config->regs;

	if (channel_num > (config->max_channels)) {
		LOG_ERR("Invalid channel number %d", channel_num);
		return -EINVAL;
	}

	if (data->channel_usage_status & BIT(channel_num)) {
		LOG_ERR("Channel %d is already in use", channel_num);
		return -EBUSY;
	}

	regs->CHANNEL[channel_num].EVSYS_CHANNEL =
		EVSYS_CHANNEL_EVGEN(ev_gen) |
		EVSYS_CHANNEL_PATH(EVSYS_CHANNEL_PATH_ASYNCHRONOUS_Val) |
		EVSYS_CHANNEL_EDGSEL(EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT_Val);

	data->channel_usage_status |= BIT(channel_num);

	return 0;
}

int evsys_mchp_connect_user_to_channel(const struct device *dev, uint8_t channel_num,
				       int user_offset)
{
	const struct evsys_mchp_config *config = dev->config;
	evsys_registers_t *regs = config->regs;
	int user_idx;

	if (channel_num > (config->max_channels)) {
		LOG_ERR("Invalid channel number %d", channel_num);
		return -EINVAL;
	}

	user_idx = (user_offset - EVSYS_USER_REG_OFST) / 4;
	LOG_INF("called %s user_idx = %d", __func__, user_idx);

	if (user_idx >= EVSYS_MCHP_EVUSER_MAX) {
		LOG_ERR("Invalid User ID %d", user_idx);
		return -EINVAL;
	}
	/* one is added with channel here as per the calculation mentioned in the datasheet */
	regs->EVSYS_USER[user_idx] = EVSYS_USER_CHANNEL(channel_num + 1);

	return 0;
}

static int evsys_mchp_init(const struct device *dev)
{
	const struct evsys_mchp_config *config = dev->config;
	evsys_registers_t *regs = config->regs;
	int ret_val;
	size_t i;

	ret_val = clock_control_on(config->evsys_clock.clk_dev, config->evsys_clock.clk_mclk);
	LOG_INF("mclock on retval = %d", ret_val);

	for (i = 0; i < EVSYS_SYNCH_NUM; i++) {
		ret_val = clock_control_on(config->evsys_clock.clk_dev,
					   config->evsys_clock.clk_gclk[i]);
		LOG_INF("gclock[%zu] on retval = %d", i, ret_val);
	}

	regs->EVSYS_CTRLA = EVSYS_CTRLA_SWRST_Msk;

	for (i = 0; i < (config->route_count); i++) {
		const struct evsys_mchp_route *route = &config->routes[i];
		size_t j;

		ret_val = evsys_mchp_connect_channel_to_evgen(dev, route->channel, route->evgen);
		if (ret_val < 0) {
			LOG_ERR("Channel to evgen connect failed %d", ret_val);
			return ret_val;
		}

		for (j = 0; j < (route->user_count); j++) {
			ret_val = evsys_mchp_connect_user_to_channel(dev, route->channel,
								     route->users[j]);
			if (ret_val < 0) {
				LOG_ERR("User to channel connect failed %d", ret_val);
				return ret_val;
			}
		}
	}

	config->irq_config_func(dev);

	return 0;
}

/* clang-format off */
#define EVSYS_MCHP_CLOCK_ASSIGN(n)                                                                 \
	.evsys_clock.clk_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                 \
	.evsys_clock.clk_mclk = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_evsys, subsystem)),   \
	.evsys_clock.clk_gclk[0] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys0, subsystem), \
	.evsys_clock.clk_gclk[1] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys1, subsystem), \
	.evsys_clock.clk_gclk[2] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys2, subsystem), \
	.evsys_clock.clk_gclk[3] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys3, subsystem), \
	.evsys_clock.clk_gclk[4] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys4, subsystem), \
	.evsys_clock.clk_gclk[5] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys5, subsystem), \
	.evsys_clock.clk_gclk[6] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys6, subsystem), \
	.evsys_clock.clk_gclk[7] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys7, subsystem), \
	.evsys_clock.clk_gclk[8] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys8, subsystem), \
	.evsys_clock.clk_gclk[9] = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys9, subsystem), \
	.evsys_clock.clk_gclk[10] =                                                                \
		(void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys10, subsystem),                   \
	.evsys_clock.clk_gclk[11] =                                                                \
		(void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_evsys11, subsystem),

#define EVSYS_MCHP_USER_ARRAY(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),

#define EVSYS_MCHP_ROUTE_INIT(node_id)                                                             \
	{                                                                                          \
		.channel = DT_PROP(node_id, channel),                                              \
		.evgen = DT_PROP(node_id, event_generator),                                        \
		.user_count = DT_PROP_LEN(node_id, event_users),                                   \
		.users = (const uint32_t[]){DT_FOREACH_PROP_ELEM(node_id, event_users,             \
								 EVSYS_MCHP_USER_ARRAY)},          \
	},

#define EVSYS_MCHP_INIT(n)                                                                         \
	static const struct evsys_mchp_route evsys_mchp_routes_##n[] = {                           \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), EVSYS_MCHP_ROUTE_INIT)                \
	};                                                                                         \
	static void evsys_mchp_irq_config_func_##n(const struct device *dev);                      \
	static struct evsys_mchp_data evsys_mchp_data_##n = {};                                    \
                                                                                                   \
	static const struct evsys_mchp_config evsys_mchp_config_##n = {                            \
		.regs = (evsys_registers_t *)DT_INST_REG_ADDR(n),                                  \
		EVSYS_MCHP_CLOCK_ASSIGN(n)                                                         \
		.irq_config_func = evsys_mchp_irq_config_func_##n,                                 \
		.routes = evsys_mchp_routes_##n,                                                   \
		.route_count = ARRAY_SIZE(evsys_mchp_routes_##n),                                  \
		.max_channels = DT_INST_PROP(n, max_channels),					   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &evsys_mchp_init, NULL, &evsys_mchp_data_##n,                     \
			      &evsys_mchp_config_##n, POST_KERNEL,                                 \
			      CONFIG_MCHP_EVSYS_INIT_PRIORITY, NULL);                              \
                                                                                                   \
	static void evsys_mchp_irq_config_func_##n(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), evsys_mchp_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* clang-format on */
DT_INST_FOREACH_STATUS_OKAY(EVSYS_MCHP_INIT)
