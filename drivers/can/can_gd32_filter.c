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

static inline void can_gd32_filter_enter_init_mode(const struct can_gd32_filter *filter)
{
	CAN_FCTL(filter->cfg->reg) |= CAN_FCTL_FLD;
}

static inline void can_gd32_filter_leave_init_mode(const struct can_gd32_filter *filter)
{
	CAN_FCTL(filter->cfg->reg) &= ~CAN_FCTL_FLD;
}

/* must use in lock */
static inline void can_gd32_filter_setsplit(const struct can_gd32_filter *filter, int location)
{
	__ASSERT(location < filter->cfg->size, "filter location overflow");

	can_gd32_filter_enter_init_mode(filter);
	CAN_FCTL(filter->cfg->reg) |= FCTL_HBC1F(location);
	can_gd32_filter_leave_init_mode(filter);
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

	// todo: use 32bit only
	static int filter_main_id = 0;
	static int filter_secondary_id = CONFIG_CAN_MAX_FILTER;

	// todo: check unit and split location

	// fixme: overflow and ENOSPC
	int filter = is_main_controller ? filter_main_id++ : filter_secondary_id--;
	LOG_DBG("[CAN FILTER][%s][%s]filter number: %d\n",
		is_main_controller ? "can_main" : "can_secondary",
		fifo == CAN_FIFO0 ? "FIFO0" : "FIFO1", filter);
	return filter;
}

static inline void can_gd32_filter_set_mode(const struct can_gd32_filter *filter, int filter_unit,
					    enum can_filter_type type)
{
	const int filter_unit_bit = 1 << filter_unit;
	CAN_FMCFG(filter->cfg->reg) &= ~filter_unit_bit;
	CAN_FSCFG(filter->cfg->reg) &= ~filter_unit_bit;

	switch (type) {
	case CAN_FILTER_16BIT_LIST:
		CAN_FMCFG(filter->cfg->reg) |= filter_unit_bit;
		break;

	case CAN_FILTER_32BIT_LIST:
		CAN_FSCFG(filter->cfg->reg) |= filter_unit_bit;
		CAN_FMCFG(filter->cfg->reg) |= filter_unit_bit;
		break;

	case CAN_FILTER_16BIT_MASK:
		/* default is 16bit mask */
		break;

	case CAN_FILTER_32BIT_MASK:
		CAN_FSCFG(filter->cfg->reg) |= filter_unit_bit;
		break;

	default:
		__ASSERT(0, "filter mode not exist.");
		break;
	}
}

static inline void can_gd32_filter_set_active(const struct can_gd32_filter *filter, int filter_unit,
					      bool active)
{
	switch (active) {
	case true:
		CAN_FW(filter->cfg->reg) |= 1 << filter_unit;
		break;

	default: /*false*/
		CAN_FW(filter->cfg->reg) &= ~(1 << filter_unit);
		break;
	}
}

static inline void can_gd32_filter_set_fifo(const struct can_gd32_filter *filter, int filter_unit,
					    enum can_fifo fifo)
{
	__ASSERT(fifo == CAN_FIFO_0, "FIFO1 not support yet");
}

static int can_gd32_filter_add_mask32(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	/* need CAN_GD32_FILTER_LOCK outside*/
	__ASSERT(0, "todo");
	LOG_DBG("[CAN FILTER]%s\n", __func__);
	return -EINVAL;
}

static int can_gd32_filter_add_mask16(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	/* need CAN_GD32_FILTER_LOCK outside*/
	__ASSERT(0, "todo");
	LOG_DBG("[CAN FILTER]%s\n", __func__);
	return -EINVAL;
}

static int can_gd32_filter_add_list32(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	/* need CAN_GD32_FILTER_LOCK outside*/
	LOG_DBG("[CAN FILTER]%s\n", __func__);
	int filter_unit =
		can_gd32_filter_getemptyunit(is_main_controller, fifo, CAN_FILTER_32BIT_LIST);
	if (filter_unit == -ENOSPC) {
		return ENOSPC;
	}

	/* config filter*/
	can_gd32_filter_enter_init_mode(filter);
	can_gd32_filter_set_active(filter, filter_unit, false);
	can_gd32_filter_set_mode(filter, filter_unit, CAN_FILTER_32BIT_LIST);
	can_gd32_filter_set_fifo(filter, filter_unit, fifo);

	/* set data */
	/* todo: set other in same unit */
	CAN_FDATA0(filter->cfg->reg, filter_unit) = ((zfilter->id_type == CAN_STANDARD_IDENTIFIER) ?
							     zfilter->id << 21 :
								   (zfilter->id << 3 | CAN_FF_EXTENDED));
	CAN_FDATA0(filter->cfg->reg, filter_unit) |=
		(zfilter->rtr_mask == 0 && zfilter->rtr) ? CAN_FT_REMOTE : CAN_FT_DATA;
	CAN_FDATA1(filter->cfg->reg, filter_unit) = 0; /* clear unuse rule */

	/* active */
	can_gd32_filter_set_active(filter, filter_unit, true);
	can_gd32_filter_leave_init_mode(filter);
	return filter_unit;
}

static int can_gd32_filter_add_list16(const struct can_gd32_filter *filter, bool is_main_controller,
				      enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
				      const struct zcan_filter *zfilter)
{
	/* need CAN_GD32_FILTER_LOCK outside*/
	LOG_DBG("[CAN FILTER]%s\n", __func__);
	int filter_unit =
		can_gd32_filter_getemptyunit(is_main_controller, fifo, CAN_FILTER_16BIT_LIST);
	if (filter_unit == -ENOSPC) {
		// if filter full and no list mode filter empty try 16 bit mask mode
		return can_gd32_filter_add_mask16(filter, is_main_controller, fifo, cb, cb_arg,
						  zfilter);
	}

	/* config filter*/
	can_gd32_filter_enter_init_mode(filter);
	can_gd32_filter_set_active(filter, filter_unit, false);
	can_gd32_filter_set_mode(filter, filter_unit, CAN_FILTER_16BIT_LIST);
	can_gd32_filter_set_fifo(filter, filter_unit, fifo);

	/* set data */
	/* todo: set other in same unit */
	CAN_FDATA0(filter->cfg->reg, filter_unit) =
		FDATA_MASK_HIGH(0) /* must clear unuse rule */ |
		FDATA_MASK_LOW((zfilter->id << 5) & CAN_FILTER_MASK_16BITS);
	CAN_FDATA1(filter->cfg->reg, filter_unit) = 0; /* must clear unuse rule */

	/* active */
	can_gd32_filter_set_active(filter, filter_unit, true);
	can_gd32_filter_leave_init_mode(filter);
	return filter_unit;
}

int can_gd32_filter_add(const struct can_gd32_filter *filter, bool is_main_controller,
			enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
			const struct zcan_filter *zfilter)
{
	int ret = -EINVAL;
	ARG_UNUSED(ret);
	CAN_GD32_FILTER_LOCK();

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
