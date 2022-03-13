/*
 * Copyright (c) 2022 YuLong Yao <feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <devicetree.h>
#include <drivers/can.h>
#include <drivers/pinctrl.h>
#include <kernel.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(can_gd32_filter, CONFIG_CAN_LOG_LEVEL);

#include "can_gd32_filter.h"

#define DT_DRV_COMPAT gd_gd32_can_filter

int can_gd32_filter_initial(const struct can_gd32_filter *filter)
{
	if (filter->data->initialed) {
		return -EALREADY;
	}

	filter->data->initialed = true;
	k_mutex_init(&filter->data->inst_mutex);

	rcu_periph_clock_enable(filter->cfg->rcu_periph_clock);

	return 0;
}

#define DT_INST_PARENT_REG_ADDR(inst) DT_REG_ADDR_BY_IDX(DT_INST_PARENT(inst), 0)
#define DT_INST_PARENT_PROP(inst, prop) DT_PROP(DT_INST_PARENT(inst), prop)
#define CAN_GD32_FILTER_ADDR_BIAS(addr) ((addr) + 0x200U)

#define CAN_GD32_FILTER_DATA_INIT(inst) struct can_gd32_filter_data can_gd32_filter_data_##inst;
#define CAN_GD32_FILTER_CFG_INIT(inst)                                                             \
	struct can_gd32_filter_cfg can_gd32_filter_cfg_##inst = {                                  \
		.reg = CAN_GD32_FILTER_ADDR_BIAS(DT_INST_PARENT_REG_ADDR(inst)),                   \
		.rcu_periph_clock = DT_INST_PARENT_PROP(inst, rcu_periph_clock),                   \
		.size = DT_INST_PROP(inst, size),                                                  \
	};
#define CAN_GD32_FILTER_DT_INIT(inst)                                                              \
	struct can_gd32_filter DT_DRV_INST(inst) = {                                               \
		.data = &can_gd32_filter_data_##inst,                                              \
		.cfg = &can_gd32_filter_cfg_##inst,                                                \
	};

#define CAN_GD32_FILTER_INIT(inst)                                                                 \
	CAN_GD32_FILTER_DATA_INIT(inst)                                                            \
	CAN_GD32_FILTER_CFG_INIT(inst)                                                             \
	CAN_GD32_FILTER_DT_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_GD32_FILTER_INIT)
