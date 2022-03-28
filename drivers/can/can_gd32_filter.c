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

struct can_gd32_filter_cb {
	can_rx_callback_t cb;
	void *cb_arg;

	uint8_t filter_number;

	struct can_gd32_filter_cb *next;
};
/*can main/secondary, fifo 0/1, items, too large, use linked-list*/
static struct can_gd32_filter_cb *can_gd32_filter_cb_lut[2][2] = { NULL, NULL, NULL, NULL };
K_HEAP_DEFINE(can_gd32_filter_cb_list,
	      sizeof(struct can_gd32_filter_cb) * 10 /* todo add to kconfig*/);

static int inline can_gd32_filter_cb_lut_append(bool is_main_controller, enum can_fifo fifo,
						can_rx_callback_t cb, void *cb_arg,
						uint8_t filter_number)
{
	struct can_gd32_filter_cb *ptr =
		can_gd32_filter_cb_lut[(uint8_t)is_main_controller][(uint8_t)fifo];
	struct can_gd32_filter_cb *ptr_tmp; /* for swap val */
	/* malloc area for save data */
	struct can_gd32_filter_cb *val = k_heap_alloc(&can_gd32_filter_cb_list,
						      sizeof(struct can_gd32_filter_cb), K_NO_WAIT);
	if (val == NULL)
		return -ENOSPC;

	/* copy data */
	val->cb = cb;
	val->cb_arg = cb_arg;
	val->filter_number = filter_number;
	val->next = NULL;

	/* get the end of list */
	if (ptr == NULL) {
		/* save head to list */
		can_gd32_filter_cb_lut[(uint8_t)is_main_controller][(uint8_t)fifo] = val;
	} else {
		/* save child by order of filter_number */
		while (ptr->next != NULL && ptr->filter_number < val->filter_number)
			ptr = ptr->next;

		ptr_tmp = ptr->next;
		ptr->next = val;
		val->next = ptr_tmp;
	}

	return 0;
}

static inline struct can_gd32_filter_cb *
can_gd32_filter_cb_lut_get(bool is_main_controller, enum can_fifo fifo, uint8_t filter_number)
{
	struct can_gd32_filter_cb *ptr =
		can_gd32_filter_cb_lut[(uint8_t)is_main_controller][(uint8_t)fifo];

	while (ptr != NULL) {
		if (ptr->filter_number == filter_number) {
			return ptr;
		}

		ptr = ptr->next;
	}

	return -ENODATA;
}

static inline int can_gd32_filter_get_unitsize(const struct can_gd32_filter *filter,
					       uint8_t filter_unit)
{
#define CAN_GD32_FILTER_SIZE_MASK32 1
#define CAN_GD32_FILTER_SIZE_MASK16 2
#define CAN_GD32_FILTER_SIZE_LIST32 2
#define CAN_GD32_FILTER_SIZE_LIST16 4
	__ASSERT(filter_unit < CONFIG_CAN_MAX_FILTER, "filter unit index error");

	if (CAN_FMCFG(filter->cfg->reg) & (1 << filter_unit)) { /* mask */
		if (CAN_FMCFG(filter->cfg->reg) & (1 << filter_unit)) { /* 32-bit */
			return CAN_GD32_FILTER_SIZE_MASK32;
		} else { /* 16-bit */
			return CAN_GD32_FILTER_SIZE_MASK16;
		}
	} else { /* list */
		if (CAN_FMCFG(filter->cfg->reg) & (1 << filter_unit)) { /* 32-bit */
			return CAN_GD32_FILTER_SIZE_LIST32;
		} else { /* 16-bit */
			return CAN_GD32_FILTER_SIZE_LIST16;
		}
	}
}

static inline uint8_t can_gd32_filter_get_number(const struct can_gd32_filter *filter,
						 bool is_main_controller, enum can_fifo fifo,
						 uint8_t filter_unit)
{
	__ASSERT(filter_unit < CONFIG_CAN_MAX_FILTER, "filter unit index error");
	uint8_t result = 0;
	size_t i = is_main_controller ? 0 : CONFIG_CAN_FILTER_SPLIT_DEFAULT;
	int size = 0;

	for (; i < filter_unit; i++) {
		/* ignore dismatch fifo */
		if (CAN_FAFIFO(filter->cfg->reg) & (1 << i))
			continue;

		size = can_gd32_filter_get_unitsize(filter, i - 1);
		if (size <= 0)
			return size; /* error */

		result += size;
	}

	return result;
}

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
	uint8_t filter_number = can_gd32_filter_get_number(
		filter, is_main_controller, fifo, filter_unit) /* need add sub filter index here*/;
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
	CAN_FDATA1(filter->cfg->reg, filter_unit) = 0; /* clear unused rule */

	/* save cb */
	can_gd32_filter_cb_lut_append(is_main_controller, fifo, cb, cb_arg, filter_number);

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
	uint8_t filter_number = can_gd32_filter_get_number(
		filter, is_main_controller, fifo, filter_unit) /* need add sub filter index here*/;
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
	CAN_FDATA1(filter->cfg->reg, filter_unit) = 0; /* clear unused rule */

	/* save cb */
	can_gd32_filter_cb_lut_append(is_main_controller, fifo, cb, cb_arg, filter_number);

	/* active */
	can_gd32_filter_set_active(filter, filter_unit, true);
	can_gd32_filter_leave_init_mode(filter);
	return filter_number;
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

can_rx_callback_t can_gd32_filter_getcb(const struct can_gd32_filter *filter,
					bool is_main_controller, enum can_fifo fifo,
					uint8_t filter_number)
{
	can_rx_callback_t callback = NULL;

	CAN_GD32_FILTER_LOCK();

	struct can_gd32_filter_cb *cb =
		can_gd32_filter_cb_lut_get(is_main_controller, fifo, filter_number);
	if (cb != -ENODATA) {
		callback = cb->cb;
	}

	CAN_GD32_FILTER_UNLOCK();

	return callback;
}

void *can_gd32_filter_getcbarg(const struct can_gd32_filter *filter, bool is_main_controller,
			       enum can_fifo fifo, int filter_number)
{
	void *callback_arg = NULL;

	CAN_GD32_FILTER_LOCK();
	struct can_gd32_filter_cb *cb =
		can_gd32_filter_cb_lut_get(is_main_controller, fifo, filter_number);
	if (cb != -ENODATA) {
		callback_arg = cb->cb_arg;
	}
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
