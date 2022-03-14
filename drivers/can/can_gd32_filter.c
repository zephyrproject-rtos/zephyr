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

#define CAN_GD32_FILTER_LOCK() k_mutex_lock(&filter->data->mutex, K_FOREVER);
#define CAN_GD32_FILTER_UNLOCK() k_mutex_unlock(&filter->data->mutex);

/* must use in lock */
static inline void can_gd32_filter_setsplit(const struct can_gd32_filter *filter, int location)
{
	__ASSERT(location < filter->cfg->size, "filter location overflow");

	CAN_FCTL(filter->cfg->reg) |= CAN_FCTL_FLD;
	CAN_FCTL(filter->cfg->reg) |= FCTL_HBC1F(location);
	CAN_FCTL(filter->cfg->reg) &= ~CAN_FCTL_FLD;
}

static inline int can_gd32_filter_getsplit(const struct can_gd32_filter *filter)
{
	return GET_BITS(CAN_FCTL(filter->cfg->reg), 8, 13);
}

int can_gd32_filter_initial(const struct can_gd32_filter *filter)
{
	__ASSERT(filter != NULL, "empty ptr");
	if (filter->data->initialed) {
		return -EALREADY;
	}

	filter->data->initialed = true;
	k_mutex_init(&filter->data->mutex);

	/* enable clock of filter if not enabled */
	rcu_periph_clock_enable(filter->cfg->rcu_periph_clock);

	/* set default split location */
	can_gd32_filter_setsplit(filter, CONFIG_CAN_FILTER_SPLIT_DEFAULT);

	return 0;
}

int can_gd32_filter_getmaxsize(const struct can_gd32_filter *filter, enum can_ide id_type,
			       bool is_main_controller)
{
	__ASSERT(filter != NULL, "empty ptr");

	switch (id_type) {
	case CAN_STANDARD_IDENTIFIER:
	case CAN_EXTENDED_IDENTIFIER:
		return is_main_controller ? can_gd32_filter_getsplit(filter) * 2 :
						  filter->cfg->size - can_gd32_filter_getsplit(filter);

	default:
		return -EINVAL;
	}
}

static int can_gd32_filter_getemptyunit(bool is_main_controller, enum can_fifo fifo,
					enum can_filter_type type)
{
	__ASSERT(fifo == CAN_FIFO_0, "FIFO1 Not SUPPORT at this time")

	ARG_UNUSED(type);

	static int filter_main_id = 0;
	static int filter_secondary_id = 0;

	//todo: check unit and split location

	return is_main_controller ? filter_main_id++ : filter_secondary_id++;
}

static int can_gd32_filter_add_mask32(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	__ASSERT(0, "todo");
	return -EINVAL;
}

static int can_gd32_filter_add_mask16(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	__ASSERT(0, "todo");
	return -EINVAL;
}

static int can_gd32_filter_add_list32(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	int filter_unit =
		can_gd32_filter_getemptyunit(is_main_controller, fifo, CAN_FILTER_32BIT_LIST);
	if (filter_unit == -ENOSPC) {
		return ENOSPC;
	}

	__ASSERT(0, "todo");
	return -EINVAL;
}

static int can_gd32_filter_add_list16(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	int filter_unit =
		can_gd32_filter_getemptyunit(is_main_controller, fifo, CAN_FILTER_16BIT_LIST);
	if (filter_unit == -ENOSPC) {
		// if filter full and no list mode filter empty check 16 bit mask mode
		return can_gd32_filter_add_mask16(filter, is_main_controller, fifo, cb, cb_arg,
						  zfilter);
	}

	__ASSERT(0, "todo");
	return -EINVAL;
}

int can_gd32_filter_add(const struct can_gd32_filter *filter, bool is_main_controller,
			enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
			const struct zcan_filter *zfilter)
{
	int ret = -EINVAL;
	CAN_GD32_FILTER_LOCK();
	/* todo */

	if (zfilter->id_type == CAN_STANDARD_IDENTIFIER && zfilter->id_mask == CAN_STD_ID_MASK) {
		// 16 bit list mode or 16 bit mask mode if filter full and no list mode filter empty
		can_gd32_filter_add_list16(filter, is_main_controller, fifo, cb, cb_arg, zfilter);
	} else if (zfilter->id_type == CAN_EXTENDED_IDENTIFIER &&
		   zfilter->id_mask == CAN_EXT_ID_MASK) {
		// 32 bit list mode
		can_gd32_filter_add_list32(filter, is_main_controller, fifo, cb, cb_arg, zfilter);
	} else if (zfilter->id_type == CAN_STANDARD_IDENTIFIER) {
		// 16 bit mask mode
		can_gd32_filter_add_mask16(filter, is_main_controller, fifo, cb, cb_arg, zfilter);
	} else if (zfilter->id_type == CAN_EXTENDED_IDENTIFIER &&
		   (0x7FFF & zfilter->id_mask) == 0) {
		// 16 bit list mode or 16 bit mask mode if filter full and no list mode filter empty
		can_gd32_filter_add_list16(filter, is_main_controller, fifo, cb, cb_arg, zfilter);
	} else if (zfilter->id_type == CAN_EXTENDED_IDENTIFIER) {
		// 32 bit mask mode
		can_gd32_filter_add_mask32(filter, is_main_controller, fifo, cb, cb_arg, zfilter);
	} else {
		__ASSERT(0, "filter mode select failed.");
	}

	CAN_GD32_FILTER_UNLOCK();
	return 0;
}

void can_gd32_filter_remove(const struct can_gd32_filter *filter, int filter_id)
{
	CAN_GD32_FILTER_LOCK();
	/* todo */
	__ASSERT(0, "todo");

	CAN_GD32_FILTER_UNLOCK();
}

can_rx_callback_t can_gd32_filter_getcb(const struct can_gd32_filter *filter, int index)
{
	can_rx_callback_t callback = NULL;

	CAN_GD32_FILTER_LOCK();
	// callback = filter->cfg->unit[index].callback;
	__ASSERT(0, "todo");
	CAN_GD32_FILTER_UNLOCK();

	return callback;
}

void *can_gd32_filter_getcbarg(const struct can_gd32_filter *filter, int index)
{
	void *callback_arg = NULL;

	CAN_GD32_FILTER_LOCK();
	// callback_arg = filter->cfg->unit[index].callback_arg;
	__ASSERT(0, "todo");
	CAN_GD32_FILTER_UNLOCK();

	return callback_arg;
}

#define DT_INST_PARENT_REG_ADDR(inst) DT_REG_ADDR_BY_IDX(DT_INST_PARENT(inst), 0)
#define DT_INST_PARENT_PROP(inst, prop) DT_PROP(DT_INST_PARENT(inst), prop)

#define CAN_GD32_FILTER_DATA_INIT(inst)                                                            \
	struct can_gd32_filter_data can_gd32_filter_data_##inst;                                   \
	struct can_gd32_filter_unit can_gd32_filter_unit_##inst[DT_INST_PROP(inst, size)];
#define CAN_GD32_FILTER_CFG_INIT(inst)                                                             \
	struct can_gd32_filter_cfg can_gd32_filter_cfg_##inst = {                                  \
		.reg = DT_INST_PARENT_REG_ADDR(inst),                                              \
		.rcu_periph_clock = DT_INST_PARENT_PROP(inst, rcu_periph_clock),                   \
		.size = DT_INST_PROP(inst, size),                                                  \
		.unit = (struct can_gd32_filter_unit *)&can_gd32_filter_unit_##inst,               \
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
