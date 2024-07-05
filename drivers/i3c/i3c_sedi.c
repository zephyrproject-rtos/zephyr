/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/drivers/i3c.h>
#include "sedi_driver_i3c.h"
#include <zephyr/drivers/i3c/i3c_sedi.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "sedi_soc.h"

LOG_MODULE_REGISTER(i3c_sedi, CONFIG_I3C_LOG_LEVEL);

#define I3C_TIMEOUT_MS (100)
#define I3C_INTERRUPT_TIMEOUT_DEFAULT (k_ms_to_ticks_ceil32(I3C_TIMEOUT_MS))
#define DT_DRV_COMPAT intel_sedi_i3c
#define GPIO_ID_IBI_MASK (0x1F)
#define I3C_DYNAMIC_ADDRESS_DAT_SHIFT 16
#define I3C_DYNAMIC_ADDRESS_MASK 0x7F

static const struct device *gpio0;
static const struct device *i3c;
struct gpio_callback cb;

struct i3c_context {
	struct i3c_driver_data common;
	int sedi_device;
	uint32_t ibi_pins;
	struct k_sem *sem;
	struct k_mutex *mutex;
	uint32_t reason;
	uint32_t base;
	uint8_t slave_cnt;
	uint8_t dat_sts;
	uint8_t ibi_enabled;
	i3c_slave_device slaves[SEDI_I3C_DEVICE_NUM_MAX];
};

struct i3c_config {
	struct i3c_driver_config common;
};

static int i3c_get_free_dat_entry(struct i3c_context *ctx)
{
	int i = 0;

	if (ctx->slave_cnt >= I3C_DEVICE_NUM_MAX)
		return -EIO;

	for (i = 0; i < I3C_DEVICE_NUM_MAX; i++) {
		if ((ctx->dat_sts & BIT(i)) == 0)
			return i;
	}

	return -EIO;
}

static void i3c_log_cb(sedi_i3c_log_priority_t priority, const char *message, va_list vargs)
{
	if (priority < SEDI_I3C_LOG_WARNING) {
		vprintk(message, vargs);
	}
}

static void i3c_evt_cb(uint32_t event, void *prv_data)
{
	const struct device *dev = (const struct device *)prv_data;
	struct i3c_context *ctx = dev->data;

	ctx->reason = event;

	k_sem_give(ctx->sem);
}

static int i3c_get_ibi_index(struct i3c_context *ctx, uint8_t addr)
{
	int i;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (addr == ctx->slaves[i].sensor.dyn_addr ||
		    addr == ctx->slaves[i].sensor.static_addr)
			return i;
	}

	return -EINVAL;
}

static void i3c_ibi_cb(const sedi_ibi_t *ibi, void *prv_data)
{
	const struct device *dev = (const struct device *)prv_data;
	struct i3c_context *ctx = dev->data;
	i3c_slave_device *slave;

	int index;
	index = i3c_get_ibi_index(ctx, ibi->ibi_address);
	if (index >= 0 && index < I3C_DEVICE_NUM_MAX) {
		slave = &ctx->slaves[index];
		slave->ibi_len = ibi->ibi_len;
		__ASSERT(ibi->ibi_len <= SEDI_I3C_MAX_IBI_PAYLOAD_LEN, "");

		memcpy(slave->ibi_payload, ibi->ibi_payload, ibi->ibi_len);

		if (slave->ibi_cb)
			slave->ibi_cb(ctx->sedi_device, index, slave->cb_arg);
	}

	pm_device_busy_clear(dev);
}

static uint8_t i3c_address_assign(struct i3c_context *ctx, bool is_daa, uint8_t dev_index, int num)
{
	int ret = 0;
	int wait_result;

	ctx->reason = 0;

	ret = sedi_i3c_address_assign(ctx->sedi_device, dev_index, num, is_daa);

	if (ret) {
		;
	}

	wait_result = k_sem_take(ctx->sem, Z_TIMEOUT_TICKS(I3C_INTERRUPT_TIMEOUT_DEFAULT));

	if (wait_result != 0 || ctx->reason) {
		LOG_ERR("I3C error, i3c_address_assign failed!\n");
		sedi_i3c_controller_recover(ctx->sedi_device);
		return -EIO;
	}

	return 0;
}

static uint8_t i3c_direct_ccc(struct i3c_context *ctx, uint8_t cmd_code, uint8_t dev_index,
			      int8_t cnt, int op, uint8_t *buf)
{
	int wait_result;
	int retry;

	ctx->reason = 0;
	retry = 0;

xfer_restart:
	sedi_i3c_regular_xfer(ctx->sedi_device, dev_index, cmd_code, SEDI_I3C_XFER_SDR0, buf, cnt,
			      op ? SEDI_I3C_READ : SEDI_I3C_WRITE, SEDI_I3C_POS_SINGLE);

	wait_result = k_sem_take(ctx->sem, Z_TIMEOUT_TICKS(I3C_INTERRUPT_TIMEOUT_DEFAULT));

	if (wait_result != 0 || ctx->reason) {
		LOG_ERR("i3c_direct_ccc response fail, wait_result:%d, reason:0x%x\n", wait_result,
			ctx->reason);
		sedi_i3c_register_dump(ctx->sedi_device);
		sedi_i3c_controller_recover(ctx->sedi_device);
		if (!retry) {
			retry++;
			ctx->reason = 0;
			goto xfer_restart;
		}
		return -EIO;
	}

	return 0;
}

static uint8_t i3c_new_address_assign(struct i3c_context *ctx, uint8_t index, uint8_t new_addr)
{
	int ret = 0;
	uint8_t addr = new_addr << 1;
	uint32_t low = 0;

	ret = i3c_direct_ccc(ctx, I3C_CCC_SETNEWDA, index, 1, SEDI_I3C_WRITE, &addr);

	if (!ret) {
		i3c_addr_slots_mark_i3c(&ctx->common.attached_dev.addr_slots, new_addr);
		sedi_i3c_get_dat_entry(ctx->sedi_device, index, &low, NULL);
		i3c_addr_slots_mark_free(&ctx->common.attached_dev.addr_slots, (low >> 16) & 0x7F);
	}

	return ret;
}

static void i3c_dump_dat_dct(struct i3c_context *ctx)
{
	uint32_t loc1, loc2, loc3, loc4;
	int i = 0;

	for (i = 0; i < SEDI_I3C_DEVICE_NUM_MAX; i++) {
		sedi_i3c_get_dat_entry(ctx->sedi_device, i, &loc1, &loc2);
		LOG_DBG("%s(%d), get dat table, index:%d, data:0x%08X,0x%08X\n", __func__, __LINE__,
			i, loc1, loc2);
	}

	for (i = 0; i < SEDI_I3C_DEVICE_NUM_MAX; i++) {
		sedi_i3c_get_dct_entry(ctx->sedi_device, i, &loc1, &loc2, &loc3, &loc4);
		LOG_DBG("%s(%d), get dct table, index:%d, data:0x%08X,0x%08X,0x%08X,0x%08X\n",
			__func__, __LINE__, i, loc1, loc2, loc3, loc4);
	}
}

static uint8_t check_odd(uint8_t data)
{
	uint8_t result = 0;

	while (data != 0) {
		result ^= (data & 1);
		data = data >> 1;
	}

	return result;
}

static int i3c_config_dat(struct i3c_context *ctx, uint8_t index, i3c_sensor_type_t type,
			  uint8_t static_addr, uint8_t dyn_addr, uint8_t entdaa)
{
	int i = 0;
	int ret = 0;

	i3c_dat_entry_t entry = { 0 };

	i = index;

	if (i >= I3C_DEVICE_NUM_MAX) {
		ret = -EIO;
		return ret;
	}

	sedi_i3c_set_dat_entry(ctx->sedi_device, i, entry.low.as_uint32, entry.high.as_uint32);

	entry.low.as_uint32 = 0;
	entry.high.as_uint32 = 0;
	entry.high.auto_cmd_mode = 1;
	entry.high.auto_cmd_value = 0;
	entry.high.auto_cmd_mask = 0xFF;

	switch (type) {
	case I3C_SENSOR_TYPE_STATIC:
		ctx->slaves[i].sensor.dev_type = I3C_SENSOR_TYPE_STATIC;
		ctx->slaves[i].sensor.static_addr = static_addr;
		ctx->slaves[i].sensor.dyn_addr = dyn_addr;
		i3c_addr_slots_mark_i2c(&ctx->common.attached_dev.addr_slots, static_addr);
		i3c_addr_slots_mark_i3c(&ctx->common.attached_dev.addr_slots, dyn_addr);
		break;
	case I3C_SENSOR_TYPE_DYNAMIC:
		ctx->slaves[i].sensor.dev_type = I3C_SENSOR_TYPE_DYNAMIC;
		ctx->slaves[i].sensor.dyn_addr = dyn_addr;
		i3c_addr_slots_mark_i3c(&ctx->common.attached_dev.addr_slots, dyn_addr);
		break;
	case I3C_SENSOR_TYPE_I2C_LEGACY:
		ctx->slaves[i].sensor.dev_type = I3C_SENSOR_TYPE_I2C_LEGACY;
		ctx->slaves[i].sensor.static_addr = static_addr;
		i3c_addr_slots_mark_i2c(&ctx->common.attached_dev.addr_slots, static_addr);
		break;
	default:
		break;
	}

	if (!entdaa) {
		entry.low.as_uint32 = sedi_i3c_dat_entry_assembler(static_addr, dyn_addr);
	} else {
		if (!check_odd(dyn_addr)) {
			dyn_addr |= 0x80;
		}
		entry.low.as_uint32 = ((dyn_addr & 0xFF) << 16);
	}

	sedi_i3c_set_dat_entry(ctx->sedi_device, i, entry.low.as_uint32, entry.high.as_uint32);
	ctx->dat_sts |= BIT(i);
	ctx->slaves[i].ibi_enabled = FALSE;

	LOG_DBG("%s(%d) set DAT register, index:%d, type:%d, dat_loc1:0x%x, dat_loc2:0x%x\n",
		__func__, __LINE__, i, type, entry.low.as_uint32, entry.high.as_uint32);

	return ret;
}

int i3c_reset_daa(struct i3c_context *ctx)
{
	sedi_i3c_t bus = ctx->sedi_device;

	ctx->reason = 0;

	sedi_i3c_immediate_write(bus, 0, I3C_CCC_BROAD_RSTDAA, SEDI_I3C_XFER_SDR0, NULL, 0,
				 SEDI_I3C_POS_SINGLE);

	int wait_result = k_sem_take(ctx->sem, Z_TIMEOUT_TICKS(I3C_INTERRUPT_TIMEOUT_DEFAULT));

	if (wait_result != 0 || ctx->reason) {
		if (ctx->reason == SEDI_I3C_EVENT_ADDRESS_HEADER) {
			LOG_INF("Pins not for I3C_%d\n", bus);
			return ctx->reason;
		}

		LOG_ERR("I3C Error! i3c_reset_daa wait_result = %d, reason = %d!\n", wait_result,
			ctx->reason);
	}

	return 0;
}

static int i3c_pre_init(const struct device *dev)
{
	int ret = 0;
	struct i3c_context *ctx = dev->data;
	sedi_i3c_t bus = ctx->sedi_device;
	int i = 0;
	uint8_t free_addr = 0;
	uint32_t loc1 = 0;

	i3c_addr_slots_init(dev);

	ret = sedi_i3c_context_init(bus, (uint32_t *)ctx->base, i3c_log_cb, NULL, i3c_evt_cb,
				    i3c_ibi_cb, (void *)dev);
	if (ret) {
		LOG_ERR("I3C Error! sedi_i3c_context_init returns %d!\n", ret);
		goto err;
	};

	ret = sedi_i3c_controller_init(bus);
	if (ret) {
		LOG_ERR("I3C Error! sedi_i3c_controller_init returns %d!\n", ret);
		goto err;
	};

	sedi_i3c_hci_enable(bus, 1);

	switch (bus) {
	case SEDI_I3C_0:
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(i3c0)), DT_IRQ(DT_NODELABEL(i3c0), priority),
			    sedi_i3c_isr, SEDI_I3C_0, DT_INST_IRQ(0, sense));
		irq_enable(DT_IRQN(DT_NODELABEL(i3c0)));
		break;

	case SEDI_I3C_1:
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(i3c1)), DT_IRQ(DT_NODELABEL(i3c1), priority),
			    sedi_i3c_isr, SEDI_I3C_1, DT_INST_IRQ(1, sense));
		irq_enable(DT_IRQN(DT_NODELABEL(i3c1)));
		break;

	default:
		break;
	}

	ctx->slave_cnt = 0;
	ctx->dat_sts = 0;
	ctx->ibi_enabled = 0;

	pm_device_busy_set(dev);

	ret = i3c_reset_daa(ctx);

	pm_device_busy_clear(dev);

	if (ret != 0) {
		goto err;
	}

	for (i = 0; i < I3C_DEVICE_NUM_MAX; i++) {
		ctx->slaves[i].ibi_len = -1;
	}

	if (!IS_ENABLED(CONFIG_ISH_STATIC_I3C_MODE)) {
		for (i = 0; i < I3C_DEVICE_NUM_MAX; i++) {
			free_addr = i3c_addr_slots_next_free_find(&ctx->common.attached_dev.addr_slots, 0);
			i3c_config_dat(ctx, i, I3C_SENSOR_TYPE_DYNAMIC, 0, free_addr, 1);
			LOG_DBG("Get free addr = 0x%x\n", free_addr);
		}

		ret = i3c_address_assign(ctx, true, 0, I3C_DEVICE_NUM_MAX);
		if (ret != 0) {
			LOG_ERR("I3C Pre Init, i3c_address_assign failed!\n");
			sedi_i3c_controller_recover(ctx->sedi_device);
		}

		for (i = 0; i < I3C_DEVICE_NUM_MAX; i++) {
			sedi_i3c_get_dct_entry(ctx->sedi_device, i, &loc1, NULL, NULL, NULL);
			if (loc1 == 0) {
				break;
			}
			ctx->slave_cnt++;
		}

		for (; i < I3C_DEVICE_NUM_MAX; i++) {
			sedi_i3c_get_dat_entry(ctx->sedi_device, i, &loc1, NULL);
			sedi_i3c_set_dat_entry(ctx->sedi_device, i, 0, 0);
			ctx->dat_sts &= (~(uint8_t)BIT(i));
			i3c_addr_slots_mark_free(&ctx->common.attached_dev.addr_slots,
						(loc1 >> I3C_DYNAMIC_ADDRESS_DAT_SHIFT) &
						I3C_DYNAMIC_ADDRESS_MASK);
		}
	}

	i3c_dump_dat_dct(ctx);
	LOG_INF("I3C Pre Init, DAA slave cnt = %d\n", ctx->slave_cnt);

err:
	return 0;
}

static inline uint8_t i3c_get_static_addr(struct i3c_context *ctx, int index)
{
	uint32_t low;

	sedi_i3c_get_dat_entry(ctx->sedi_device, index, &low, NULL);
	uint8_t dev_addr_tbl = low & 0x7F;

	LOG_DBG("i3c_get_static_addr, dev_addr_tbl:0x%x\n", dev_addr_tbl);

	return dev_addr_tbl;
}

static inline uint8_t i3c_get_dynamic_addr(struct i3c_context *ctx, int index)
{
	uint32_t low;

	sedi_i3c_get_dat_entry(ctx->sedi_device, index, &low, NULL);
	uint8_t dev_addr_tbl = (low >> 16) & 0xFF;

	LOG_DBG("i3c_get_dynamic_addr, dev_addr_tbl:0x%x\n", dev_addr_tbl);

	return dev_addr_tbl;
}

static inline uint64_t get_pid(struct i3c_context *ctx, uint8_t index)
{
	uint64_t pid;
	uint32_t pid_h32, pid_l16;

	sedi_i3c_get_dct_entry(ctx->sedi_device, index, &pid_h32, &pid_l16, NULL, NULL);
	pid_l16 &= 0xFFFF;

	pid = ((uint64_t)(((uint64_t)(pid_h32) << 16) | pid_l16)) & 0xFFFFFFFFFFFF;
	LOG_DBG("%s, index:%d, pid_h32:0x%x, pid_l16:0x%x, pid:0x%x, 0x%x\n", __func__, index,
		pid_h32, pid_l16, (uint32_t)((pid >> 16) & 0xFFFFFFFF), (uint32_t)(pid & 0xFFFF));
	return pid;
}

static int i3c_set_speed(struct i3c_context *ctx, i3c_speed_info *speed_info)
{
	int ret = 0;
	sedi_i3c_speed_t speed = (sedi_i3c_speed_t)speed_info->speed;

	if (speed_info->dev_type == I3C_SENSOR_TYPE_I2C_LEGACY) {
		ret = sedi_i2c_set_speed(ctx->sedi_device, speed);
	} else if (speed_info->dev_type == I3C_SENSOR_TYPE_DYNAMIC ||
		   speed_info->dev_type == I3C_SENSOR_TYPE_STATIC) {
		ret = sedi_i3c_set_speed(ctx->sedi_device, speed);
	} else {
		LOG_ERR("%s(%d), error speed type:%d\n", __func__, __LINE__, speed_info->dev_type);
		ret = -EIO;
	}

	return ret;
}

static int i3c_get_dev_idx(struct i3c_context *ctx, i3c_get_dev_index *info)
{
	int i;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == (struct i3c_device_desc *)info->target)
			break;
	}

	if (i >= ctx->slave_cnt) {
		LOG_ERR("%s, address index %d out of range\n", __func__, i);
		return -EIO;
	}

	info->dev_idx = i;

	return 0;
}

static int i3c_get_speed_type(struct i3c_context *ctx, i3c_speed_info *speed_info)
{
	int i;
	int ret = 0;
	uint8_t index = 0;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == (struct i3c_device_desc *)speed_info->target)
			break;
	}

	if (i >= ctx->slave_cnt) {
		LOG_ERR("%s, address index %d out of range\n", __func__, i);
		return -EIO;
	}
	i3c_slave_device *slave;

	slave = &ctx->slaves[i];

	if (slave->bcr & BCR_MDS_LIMIT) {
		uint8_t buf[2] = { 0 };
		/* Get Max Data speed, buf[0]-{0:fscl Max, 1:8MHZ, 2:6MHZ, 3:4MHZ, 4:2MHZ} */
		ret = i3c_direct_ccc(ctx, I3C_CCC_GETMXDS, i, sizeof(buf), SEDI_I3C_READ, buf);
		if (ret != 0) {
			LOG_ERR("i3c_direct_ccc error\n");
			return -EIO;
		}
		index = buf[0] & 0x7;
		LOG_DBG("%s, get max data speed:0x%x, 0x%x\n", __func__, buf[0], buf[1]);
	} else
		index = 0;

	__ASSERT(index <= 4, "");
	if (index > 4) {
		LOG_ERR("I3C_CCC_GETMXDS return data error\n");
		return -EIO;
	}

	i3c_speed_type speed_type[] = { I3C_SPEED_10MHZ, I3C_SPEED_8MHZ, I3C_SPEED_6MHZ,
					I3C_SPEED_4MHZ, I3C_SPEED_2MHZ };

	if ((speed_info->speed == I3C_SPEED_AUTO) || (speed_info->speed >= speed_type[index]))
		speed_info->speed = speed_type[index];

	LOG_DBG("%s(%d), index:%d, speed:%d\n", __func__, __LINE__, index, speed_info->speed);

	return 0;
}


static int i3c_read_slave_info(struct i3c_context *ctx, struct i3c_device_desc *target)
{
	int i;
	int ret = 0;
	i3c_slave_device *slave;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == target)
			break;
	}

	if (i >= ctx->slave_cnt) {
		LOG_ERR("%s, address index %d out of range\n", __func__, i);
		return -EIO;
	}

	slave = &ctx->slaves[i];
	if (slave->bcr) {
		LOG_DBG("%s, already set bcr, index:%d, bcr:0x%x\n", __func__, i, slave->bcr);
		return 0;
	}
	/* Get Bus Characteristics */
	ret = i3c_direct_ccc(ctx, I3C_CCC_GETBCR, i, sizeof(uint8_t), SEDI_I3C_READ,
			     &slave->bcr);
	if (ret != 0)
		return -EIO;

	LOG_DBG("%s, get bcr:0x%x\n", __func__, slave->bcr);

	/* Get a slave's maximum possible read length */
	if (slave->bcr & BCR_IBI_PAYLOAD) {
		uint8_t mrl[3];

		ret = i3c_direct_ccc(ctx, I3C_CCC_GETMRL, i, sizeof(mrl), SEDI_I3C_READ, mrl);
		if (ret != 0)
			return ret;
		LOG_DBG("%s, get mrl:0x%x, 0x%x, 0x%x\n", __func__, mrl[0], mrl[1], mrl[2]);
		if (mrl[2] < SEDI_I3C_MAX_IBI_PAYLOAD_LEN) {
			mrl[2] = SEDI_I3C_MAX_IBI_PAYLOAD_LEN;
			ret = i3c_direct_ccc(ctx, I3C_CCC_SETMRL, i, sizeof(mrl),
					     SEDI_I3C_WRITE, mrl);
			if (ret != 0)
				return ret;
		}
	}
	return 0;
}

static int i3c_free_new_addr(struct i3c_context *ctx, uint8_t new_addr)
{
	int i = 0;
	enum i3c_addr_slot_status status;
	uint8_t free_addr;
	uint32_t low = 0, high = 0;
	int ret = 0;

	status = i3c_addr_slots_status(&ctx->common.attached_dev.addr_slots, new_addr);

	if (status == I3C_ADDR_SLOT_STATUS_I3C_DEV) {
		/* First, check if the prefered addr is already assigned to other device */
		for (i = 0; i < ctx->slave_cnt; i++) {
			if (i3c_get_dynamic_addr(ctx, i) == new_addr) {
				break;
			}
		}

		/* Found the device with new_addr assigned */
		if (i < ctx->slave_cnt) {
			/* Get one free address and setnewda on index i,
		       then setnewda with new_addr on index  */
			free_addr = i3c_addr_slots_next_free_find(&ctx->common.attached_dev.addr_slots, 0);
			LOG_DBG("Get free addr = 0x%x\n", free_addr);
			ret = i3c_new_address_assign(ctx, i, free_addr);
			if (ret) {
				LOG_ERR("i3c_new_address_assign failed!\n");
				return -EIO;
			} else {
				sedi_i3c_get_dat_entry(ctx->sedi_device, i, &low, &high);
				SET_BITS(low, 16, 7, free_addr);
				sedi_i3c_set_dat_entry(ctx->sedi_device, i, low, high);
				i3c_addr_slots_mark_i3c(&ctx->common.attached_dev.addr_slots, free_addr);
				i3c_addr_slots_mark_free(&ctx->common.attached_dev.addr_slots, new_addr);
				(ctx->slaves[i].i3c)->dynamic_addr = free_addr;
			}
		}
	}
	return ret;
}

static int i3c_set_new_dynamic_addr(struct i3c_context *ctx, uint8_t index, uint8_t new_addr)
{
	int ret = 0;

	if (!i3c_addr_slots_is_free(&ctx->common.attached_dev.addr_slots, new_addr)) {
		/* No need to assign as it is the same */
		if (i3c_get_dynamic_addr(ctx, index) == new_addr) {
			return index;
		}

		i3c_free_new_addr(ctx, new_addr);
	}

	ret = i3c_new_address_assign(ctx, index, new_addr);
	if (ret) {
		LOG_ERR("i3c_new_address_assign failed!\n");
		return -EIO;
	} else {
		LOG_DBG("New addr = 0x%x is assigned!\n", new_addr);
	}

	return index;
}

static int i3c_add_new_i3c_device(struct i3c_context *ctx, struct i3c_device_desc *desc,
				  uint8_t dyn_addr)
{
	int ret = -EIO;
	int i = 0;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (desc->pid != get_pid(ctx, i)) {
			continue;
		}
	}

	if (i < ctx->slave_cnt) {
		uint32_t low = 0, high = 0;

		sedi_i3c_get_dat_entry(ctx->sedi_device, i, &low, &high);
		if (desc->static_addr) {
			i3c_addr_slots_mark_i2c(&ctx->common.attached_dev.addr_slots, desc->static_addr);
			SET_BITS(low, 0, 7, (desc->static_addr & 0x7F));
		}

		if (!dyn_addr) {
			ret = i;
		} else {
			/* setnewda */
			ret = i3c_set_new_dynamic_addr(ctx, i, dyn_addr);
			if (ret == -EIO) {
				LOG_ERR("i3c_set_new_dynamic_addr failed!\n");
			} else {
				SET_BITS(low, 16, 7, (dyn_addr & 0x7F));
			}
		}
		sedi_i3c_set_dat_entry(ctx->sedi_device, i, low, high);
	} else {
		if (!i3c_addr_slots_is_free(&ctx->common.attached_dev.addr_slots, dyn_addr)) {
			i3c_free_new_addr(ctx, dyn_addr);
		}

		if (ctx->slave_cnt >= I3C_DEVICE_NUM_MAX) {
			ret = -EIO;
			return ret;
		}
		if (desc->static_addr) {
			/* setdasa */
			i3c_config_dat(ctx, i3c_get_free_dat_entry(ctx), I3C_SENSOR_TYPE_STATIC,
				       desc->static_addr, dyn_addr, 0);
			if (i3c_address_assign(ctx, false, ctx->slave_cnt, 1) != 0) {
				ctx->dat_sts &= ~BIT(ctx->slave_cnt);
				return -EIO;
			}
		} else {
			/* entdaa */
			if (!dyn_addr) {
				dyn_addr = i3c_addr_slots_next_free_find(&ctx->common.attached_dev.addr_slots, 0);
			}

			i3c_config_dat(ctx, i3c_get_free_dat_entry(ctx), I3C_SENSOR_TYPE_DYNAMIC, 0,
				       dyn_addr, 1);
			if (i3c_address_assign(ctx, true, ctx->slave_cnt, 1) != 0) {
				ctx->dat_sts &= ~BIT(ctx->slave_cnt);
				return -EIO;
			}
		}
		ret = ctx->slave_cnt;
		desc->dynamic_addr = dyn_addr;
		ctx->slaves[ctx->slave_cnt].i3c = desc;
		ctx->slave_cnt++;
	}

	return ret;
}

static int i3c_add_static_device_without_pid(struct i3c_context *ctx, struct i3c_device_desc *desc,
					     uint8_t dyn_addr)
{
	int ret = -EIO;

	if (ctx->slave_cnt == I3C_DEVICE_NUM_MAX) {
		return ret;
	}

	if (desc->static_addr <= 0) {
		return ret;
	}

	if (!dyn_addr) {
		/* Get one free addr for dyn_addr */
		dyn_addr = i3c_addr_slots_next_free_find(&ctx->common.attached_dev.addr_slots, 0);
		LOG_DBG("Get free addr = 0x%x, static addr = 0x%x\n", dyn_addr, desc->static_addr);
	}

	i3c_config_dat(ctx, i3c_get_free_dat_entry(ctx), I3C_SENSOR_TYPE_STATIC, desc->static_addr,
		       dyn_addr, 0);
	if (i3c_address_assign(ctx, false, ctx->slave_cnt, 1) != 0) {
		ctx->dat_sts &= ~BIT(ctx->slave_cnt);
		return -EIO;
	} else {
		ret = ctx->slave_cnt;
		desc->dynamic_addr = dyn_addr;
		ctx->slaves[ctx->slave_cnt].i3c = desc;
		ctx->slave_cnt++;
		LOG_DBG("Slave cnt = %d\n", ctx->slave_cnt);
	}

	return ret;
}


static void i3c_wake_notify(const struct device *port, struct gpio_callback *c,
			    gpio_port_pins_t pins)
{
	struct i3c_context *ctx = (struct i3c_context *)i3c->data;

	gpio_pin_interrupt_configure(gpio0, ctx->ibi_pins, GPIO_INT_DISABLE);
	if (i3c != NULL) {
		pm_device_busy_set(i3c);
	}
	sedi_i3c_on_power_ungate(ctx->sedi_device);
}


static int i3c_register_slave_ibi(const struct device *dev, void *params)
{
	int i;
	int ret = 0;
	struct i3c_context *ctx;

	ctx = (struct i3c_context *)dev->data;
	struct i3c_ibi_param *param;

	param = (struct i3c_ibi_param *)params;

	i3c_slave_device *slave;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == param->target)
			break;
	}

	slave = &ctx->slaves[i];

	if (slave->cookie != 0) {
		LOG_DBG("%s(%d), i3c ibi address %d is already registered by handle %d\n", __func__,
			__LINE__, i, slave->cookie);
		return 0;
	}

	slave->cookie = param->cookie;
	slave->ibi_cb = param->ibi_cb;
	slave->cb_arg = param->cb_arg;

	return ret;
}

static int i3c_request_slave_ibi(const struct device *dev, void *params)
{
	int i;
	int ret = 0;
	struct i3c_context *ctx;
	struct i3c_ibi_req *param;
	i3c_slave_device *slave;

	ctx = (struct i3c_context *)dev->data;
	param = (struct i3c_ibi_req *)params;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == param->target)
			break;
	}

	slave = &ctx->slaves[i];

	if (slave->ibi_len < 0) {
		return -EIO;
	} else {
		param->cookie = slave->cookie;
		memcpy(param->payload, slave->ibi_payload, slave->ibi_len);
		param->len = slave->ibi_len;
		slave->ibi_len = -1;
		return ret;
	}
}

static int i3c_set_get_config(const struct device *dev, uint32_t id, void *param)
{
	int ret = 0;
	struct i3c_context *ctx;

	ctx = (struct i3c_context *)dev->data;

	ret = k_mutex_lock(ctx->mutex, K_FOREVER);
	if (ret != 0) {
		goto err_before_lock;
	}

	pm_device_busy_set(dev);

	switch (id) {
	case I3C_GET_DEV_IDX:
		ret = i3c_get_dev_idx(ctx,  (i3c_get_dev_index *)param);
		break;
	case I3C_REGISTER_IBI:
		ret = i3c_register_slave_ibi(dev,  (struct i3c_ibi_param *)param);
		break;
	case I3C_SET_SPEED:
		ret = i3c_set_speed(ctx, (i3c_speed_info *)param);
		break;
	case I3C_GET_SPEED_TYPE:
		ret = i3c_get_speed_type(ctx, (i3c_speed_info *)param);
		break;
	case I3C_READ_SLAVE_INFO:
		ret = i3c_read_slave_info(ctx, (struct i3c_device_desc *)param);
		break;
	default:
		break;
	}

	k_mutex_unlock(ctx->mutex);

	pm_device_busy_clear(dev);
	return ret;

err_before_lock:
	return -EIO;
}

static int i3c_sedi_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	if (type == I3C_CONFIG_CUSTOM) {
		return i3c_set_get_config(dev, ((struct i3c_config_custom *)config)->id, ((struct i3c_config_custom *)config)->ptr);
	}

	return -EINVAL;
}

static int i3c_sedi_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	if (type == I3C_CONFIG_CUSTOM) {
		return i3c_set_get_config(dev, ((struct i3c_config_custom *)config)->id, ((struct i3c_config_custom *)config)->ptr);
	}

	return -EINVAL;
}

static int i3c_sedi_attach_device(const struct device *dev, struct i3c_device_desc *desc,
				  uint8_t addr)
{
	int ret = 0;
	struct i3c_context *ctx = dev->data;

	ret = k_mutex_lock(ctx->mutex, K_FOREVER);

	pm_device_busy_set(dev);

	i3c_sensor_type_t type = (i3c_sensor_type_t)(desc->controller_priv);

	switch (type) {
	case I3C_SENSOR_TYPE_STATIC:
		if (!desc->static_addr) {
			ret = -EIO;
			goto err;
		} else {
			if (i3c_addr_slots_status(&ctx->common.attached_dev.addr_slots,
			   desc->static_addr) == I3C_ADDR_SLOT_STATUS_I2C_DEV) {
				LOG_ERR("static address is already used!\n");
				ret = -EIO;
				goto err;
			}
		}

		if (!desc->pid) {
			ret = i3c_add_static_device_without_pid(ctx, desc, addr);
		} else {
			ret = i3c_add_new_i3c_device(ctx, desc, addr);
		}
		break;

	case I3C_SENSOR_TYPE_I2C_LEGACY:
		if (!desc->static_addr) {
			ret = -EIO;
			goto err;
		}

		if (!i3c_addr_slots_is_free(&ctx->common.attached_dev.addr_slots, desc->static_addr)) {
			if (i3c_free_new_addr(ctx, desc->static_addr)) {
				goto err;
			}
		}

		ret = i3c_config_dat(ctx, i3c_get_free_dat_entry(ctx),
				     I3C_SENSOR_TYPE_I2C_LEGACY, desc->static_addr, 0, 0);

		if (ret) {
			goto err;
		}

		ret = ctx->slave_cnt;
		ctx->slaves[ctx->slave_cnt].i3c = desc;
		ctx->slave_cnt++;
		break;

	case I3C_SENSOR_TYPE_DYNAMIC:
		if (!desc->pid) {
			ret = -EIO;
			goto err;
		}
		ret = i3c_add_new_i3c_device(ctx, desc, addr);
		break;

	default:
		break;
	}

	gpio0 = device_get_binding(CONFIG_GPIO_DEV);
err:
	pm_device_busy_clear(dev);
	k_mutex_unlock(ctx->mutex);
	return ret;
}

static int i3c_sedi_detach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	int i;
	int ret = 0;
	struct i3c_context *ctx = dev->data;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == desc)
			break;
	}

	if (i >= ctx->slave_cnt)
		return -EIO;

	pm_device_busy_set(dev);

	if (ctx->dat_sts & BIT(i)) {
		sedi_i3c_set_dat_entry(ctx->sedi_device, i, 0, 0);
		ctx->dat_sts &= (~(uint8_t)BIT(i));
		ctx->slave_cnt--;
	}

	pm_device_busy_clear(dev);

	return ret;
}

static int i3c_sedi_recover_bus(const struct device *dev)
{
	struct i3c_context *ctx = dev->data;

	return sedi_i3c_controller_recover(ctx->sedi_device);
}

static int i3c_sedi_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	int i;
	int ret = 0;
	struct i3c_context *ctx = dev->data;
	struct i3c_ccc_target_payload *tgt;

	if (payload == NULL) {
		return -EINVAL;
	}

	if (payload->targets.payloads != NULL) {
		if (payload->targets.num_targets != 1) {
			return -EINVAL;
		}

		tgt = &payload->targets.payloads[0];

		for (i = 0; i < ctx->slave_cnt; i++) {
			if (i3c_get_dynamic_addr(ctx, i) == tgt->addr) {
				break;
			}
		}

		if (i >=  ctx->slave_cnt) {
			return -EINVAL;
		}

		ret = i3c_direct_ccc(ctx, payload->ccc.id, i, tgt->data_len, tgt->rnw, tgt->data);
	} else {
		ret = i3c_direct_ccc(ctx, payload->ccc.id, 0, payload->ccc.data_len, 0, payload->ccc.data);
	}

	return ret;
}

static struct i3c_device_desc *i3c_sedi_device_find(const struct device *dev,
						    const struct i3c_device_id *id)
{
	int i;
	struct i3c_context *ctx = dev->data;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if ((ctx->slaves[i].i3c) && ((ctx->slaves[i].i3c)->pid == id->pid)) {
			return ctx->slaves[i].i3c;
		}
	}

	return NULL;
}

static int i3c_sedi_transfer(const struct device *dev, struct i3c_device_desc *target,
			     struct i3c_msg *msgs, uint8_t num_msgs)
{
	int addr;
	int ret = 0;
	struct i3c_context *ctx = dev->data;
	sedi_i3c_position_t position = SEDI_I3C_POS_INVALID;
	uint8_t prev_type = 0xFF;
	uint8_t speed_mode = SEDI_I3C_XFER_SDR0;
	uint8_t tran_type = 0;
	uint8_t msg_type = 0;
	uint8_t cmd = 0;
	uint8_t cmd_type = I3C_CMD_TRANSFER;
	int i = 0;

	for (addr = 0; addr < ctx->slave_cnt; addr++) {
		if (ctx->slaves[addr].i3c == target)
			break;
	}

	if (addr >= ctx->slave_cnt) {
		return -EINVAL;
	}

	ret = k_mutex_lock(ctx->mutex, K_FOREVER);
	if (ret != 0) {
		goto err_before_lock;
	}

	if (ctx->ibi_enabled) {
		gpio_pin_interrupt_configure(gpio0, ctx->ibi_pins, GPIO_INT_DISABLE);
	}

	pm_device_busy_set(dev);
	sedi_i3c_on_power_ungate(ctx->sedi_device);
	if (ctx->ibi_enabled) {
		sedi_i3c_ibi_enable(ctx->sedi_device, false);
	}

	for (i = 0; i < num_msgs; i++) {
		tran_type = (msgs[i].flags & I3C_MSG_I2C_TRAN);

		if (tran_type) {
			sedi_i3c_enable_i2c_xfer(ctx->sedi_device, true);

			if (msgs[i].flags & I3C_MSG_HDR)
				speed_mode = SEDI_I2C_XFER_FMP;
			else
				speed_mode = SEDI_I2C_XFER_FM;
		} else {
			sedi_i3c_enable_i2c_xfer(ctx->sedi_device, false);

			if (msgs[i].flags & I3C_MSG_HDR) {
				speed_mode = SEDI_I3C_XFER_HDR_DDR;
				cmd_type = I3C_CMD_CCC_HDR;
			}
		}

		msg_type = msgs[i].flags & (I3C_MSG_READ | I3C_MSG_IMM_COMBO);

		if (msg_type == (I3C_MSG_READ | I3C_MSG_IMM_COMBO)) {
			if (prev_type != 0xFF || i != num_msgs - 1) {
				/* combo must be single transfer */
				goto err;
			} else {
				position = SEDI_I3C_POS_SINGLE;
			}
		} else {
			if (i != num_msgs - 1) {
				if (prev_type == 0xFF) {
					position = SEDI_I3C_POS_FIRST;
				} else {
					position = SEDI_I3C_POS_CONTINUE;
				}
			} else if (prev_type == 0xFF) {
				position = SEDI_I3C_POS_SINGLE;
			} else {
				position = SEDI_I3C_POS_LAST;
			}
		}

		switch (msg_type) {
		case I3C_MSG_IMM_COMBO: /* immediate write */
			__ASSERT(msgs[i].len <= 4, "");
			if (msgs[i].len > 4)
				goto err;

			cmd = (cmd_type == I3C_CMD_CCC_HDR ? SEDI_I3C_HDR_CMD_WRITE : 0);
			sedi_i3c_immediate_write(ctx->sedi_device, addr, cmd, speed_mode,
						 msgs[i].buf, msgs[i].len, position);
			break;

		case 0: /* regular write */
			cmd = (cmd_type == I3C_CMD_CCC_HDR ? SEDI_I3C_HDR_CMD_WRITE : 0);

			if (msgs[i].len > 0xffff) {
				msgs[i].len &= 0xffff;
			}

			sedi_i3c_regular_xfer(ctx->sedi_device, addr, cmd, speed_mode,
					      msgs[i].buf, msgs[i].len, SEDI_I3C_WRITE, position);
			break;
		case I3C_MSG_READ: /* regular read */
			cmd = (cmd_type == I3C_CMD_CCC_HDR ? SEDI_I3C_HDR_CMD_READ : 0);

			if (msgs[i].len > 0xffff) {
				msgs[i].len &= 0xffff;
			}

			sedi_i3c_regular_xfer(ctx->sedi_device, addr, cmd, speed_mode,
					      msgs[i].buf, msgs[i].len, SEDI_I3C_READ, position);
			break;
		case (I3C_MSG_READ | I3C_MSG_IMM_COMBO): /* combo read */
			__ASSERT(msgs[i].len >= 2, "");
			if (msgs[i].len < 2)
				goto err;
			cmd = (cmd_type == I3C_CMD_CCC_HDR ? SEDI_I3C_HDR_CMD_READ : 0);

			if (msgs[i].len > 0xffff) {
				msgs[i].len &= 0xffff;
			}
			position = SEDI_I3C_POS_SINGLE;
			sedi_i3c_combo_xfer(ctx->sedi_device, addr, cmd, speed_mode,
					    msgs[i].buf, msgs[i].len, SEDI_I3C_READ, position);
			break;

		default:
			break;
		}

		prev_type = msg_type;

		ret = k_sem_take(ctx->sem, Z_TIMEOUT_TICKS(I3C_INTERRUPT_TIMEOUT_DEFAULT));

		if (ret != 0 || ctx->reason) {
			LOG_ERR("%s(%d), reason:0x%x, err_type:%d\n", __func__, __LINE__,
				ctx->reason, ret);
			goto err;
		}
	}

	sedi_i3c_on_power_gate(ctx->sedi_device);
	if (ctx->ibi_enabled) {
		sedi_i3c_ibi_enable(ctx->sedi_device, true);
	}
	pm_device_busy_clear(dev);
	if (ctx->ibi_enabled) {
		gpio_pin_interrupt_configure(gpio0, ctx->ibi_pins, GPIO_INT_EDGE_FALLING);
	}

	k_mutex_unlock(ctx->mutex);

	return ret;
err:
	sedi_i3c_on_power_gate(ctx->sedi_device);
	if (ctx->ibi_enabled) {
		sedi_i3c_ibi_enable(ctx->sedi_device, true);
	}
	if (ctx->ibi_enabled) {
		gpio_pin_interrupt_configure(gpio0, ctx->ibi_pins, GPIO_INT_EDGE_FALLING);
	}
	k_mutex_unlock(ctx->mutex);
	pm_device_busy_clear(dev);
err_before_lock:
	return -EIO;
}

static int i3c_sedi_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	int i;
	int ret = 0;
	uint8_t data = 1;
	struct i3c_context *ctx = dev->data;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == target)
			break;
	}

	if (i >= I3C_DEVICE_NUM_MAX)
		return -EIO;

	i3c = dev;

	pm_device_busy_set(dev);

	/* Config GPIO pin for IBI wake */

	gpio_pin_configure(gpio0, ctx->ibi_pins, GPIO_INPUT);

	gpio_init_callback(&cb, i3c_wake_notify, BIT(ctx->ibi_pins & GPIO_ID_IBI_MASK));

	gpio_add_callback(gpio0, &cb);

	ret = i3c_direct_ccc(ctx, I3C_CCC_ENEC, i, sizeof(uint8_t), SEDI_I3C_WRITE, &data);

	if (!ret) {
		ctx->slaves[i].ibi_enabled = TRUE;
		sedi_i3c_ibi_enable(ctx->sedi_device, true);
		ctx->ibi_enabled |= BIT(i);
	}

	pm_device_busy_clear(dev);
	return ret;
}

static int i3c_sedi_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	int i;
	int ret = 0;
	uint8_t data = 1;
	struct i3c_context *ctx = dev->data;

	for (i = 0; i < ctx->slave_cnt; i++) {
		if (ctx->slaves[i].i3c == target)
			break;
	}

	if (i >= I3C_DEVICE_NUM_MAX)
		return -EIO;

	gpio_pin_interrupt_configure(gpio0, ctx->ibi_pins, GPIO_INT_DISABLE);
	pm_device_busy_set(dev);

	ret = i3c_direct_ccc(ctx, I3C_CCC_DISEC, i, sizeof(uint8_t), SEDI_I3C_WRITE, &data);

	if (!ret) {
		ctx->slaves[i].ibi_enabled = FALSE;
		sedi_i3c_ibi_enable(ctx->sedi_device, false);
		ctx->ibi_enabled &= (~(uint8_t)BIT(i));
	}

	pm_device_busy_clear(dev);
	return ret;
}

static int i3c_sedi_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	if (request->ibi_type == I3C_IBI_TARGET_INTR) {
		return i3c_request_slave_ibi(dev, (void *)(request->payload));
	}

	return -EINVAL;
}

static struct i3c_driver_api i3c_apis = {
	.configure = i3c_sedi_configure,
	.config_get = i3c_sedi_config_get,
	.attach_i3c_device = i3c_sedi_attach_device,
	.detach_i3c_device = i3c_sedi_detach_device,
	.recover_bus = i3c_sedi_recover_bus,
	.do_ccc = i3c_sedi_do_ccc,
	.i3c_device_find = i3c_sedi_device_find,
	.i3c_xfers = i3c_sedi_transfer,
#ifdef CONFIG_I3C_USE_IBI
	.ibi_enable = i3c_sedi_ibi_enable,
	.ibi_disable = i3c_sedi_ibi_disable,
	.ibi_raise = i3c_sedi_ibi_raise,
#endif
};

void i3c_disable_all_ibi(sedi_i3c_t bus)
{
	(void)bus;
}

extern void sedi_i3c_isr(IN sedi_i3c_t bus);

#define CREATE_I3C_INSTANCE(num)                                                                   \
	static K_SEM_DEFINE(i3c_##num##_sem, 0, 1);                                                \
	static K_MUTEX_DEFINE(i3c_##num##_mutex);                                                  \
	static struct i3c_context i3c_##num##_context = {                                          \
		.sedi_device = num,                                                                \
		.sem = &i3c_##num##_sem,                                                           \
		.mutex = &i3c_##num##_mutex,                                                       \
		.base = SEDI_I3C_##num##_REG_BASE,                                                 \
		.ibi_pins = DT_PROP(DT_NODELABEL(i3c##num), ibi_pins) & GPIO_ID_IBI_MASK           \
	};                                                                                         \
	static struct i3c_device_desc i3c_sedi_device_array_##n[SEDI_I3C_DEVICE_NUM_MAX];          \
	static struct i3c_config i3c_##num##_config = {                                            \
		.common.dev_list.i3c = i3c_sedi_device_array_##n,                                  \
		.common.dev_list.num_i3c = 0,                                                      \
	};                                                                                         \
	PM_DEVICE_DEFINE(i3c_sedi_##num, i3c_sedi_device_ctrl);                                    \
	DEVICE_DEFINE(i3c_sedi_##num, DEVICE_DT_NAME(DT_NODELABEL(i3c##num)), &i3c_pre_init,       \
		      PM_DEVICE_GET(i3c_sedi_##num),                                               \
		      &i3c_##num##_context, &i3c_##num##_config, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
		      &i3c_apis)

#ifdef CONFIG_PM_DEVICE

static int i3c_suspend_device(const struct device *dev)
{
	if (pm_device_is_busy(dev)) {
		return -EBUSY;
	}

	return 0;
}

static int i3c_resume_device_from_suspend(const struct device *dev)
{
	(void)dev;
	return 0;
}

static int i3c_sedi_device_ctrl(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = i3c_suspend_device(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = i3c_resume_device_from_suspend(dev);
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}

#endif /* CONFIG_PM_DEVICE */

#if defined(CONFIG_I3C_SEDI)

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i3c0), okay)
CREATE_I3C_INSTANCE(0);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i3c1), okay)
CREATE_I3C_INSTANCE(1);
#endif

#endif
