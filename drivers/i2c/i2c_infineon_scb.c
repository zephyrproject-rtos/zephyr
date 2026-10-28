/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_scb_i2c

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "i2c-priv.h"
#include "i2c_bitbang.h"

LOG_MODULE_REGISTER(i2c_infineon_scb, CONFIG_I2C_LOG_LEVEL);

#define IFX_SCB_CTRL           0x000U
#define IFX_SCB_I2C_CTRL       0x060U
#define IFX_SCB_I2C_STATUS     0x064U
#define IFX_SCB_I2C_M_CMD      0x068U
#define IFX_SCB_I2C_S_CMD      0x06CU
#define IFX_SCB_I2C_CFG        0x070U
#define IFX_SCB_TX_CTRL        0x200U
#define IFX_SCB_TX_FIFO_CTRL   0x204U
#define IFX_SCB_TX_FIFO_STATUS 0x208U
#define IFX_SCB_TX_FIFO_WR     0x240U
#define IFX_SCB_RX_CTRL        0x300U
#define IFX_SCB_RX_FIFO_CTRL   0x304U
#define IFX_SCB_RX_MATCH       0x310U
#define IFX_SCB_RX_FIFO_RD     0x340U
#define IFX_SCB_INTR_M         0xF00U
#define IFX_SCB_INTR_M_MASK    0xF08U
#define IFX_SCB_INTR_M_MASKED  0xF0CU
#define IFX_SCB_INTR_S         0xF40U
#define IFX_SCB_INTR_S_MASK    0xF48U
#define IFX_SCB_INTR_S_MASKED  0xF4CU
#define IFX_SCB_INTR_TX        0xF80U
#define IFX_SCB_INTR_TX_MASK   0xF88U
#define IFX_SCB_INTR_TX_MASKED 0xF8CU
#define IFX_SCB_INTR_RX        0xFC0U
#define IFX_SCB_INTR_RX_MASK   0xFC8U
#define IFX_SCB_INTR_RX_MASKED 0xFCCU

#define IFX_SCB_CTRL_MEM_WIDTH GENMASK(15, 14)
#define IFX_SCB_CTRL_MODE      GENMASK(25, 24)
#define IFX_SCB_CTRL_ENABLED   BIT(31)
#define IFX_SCB_CTRL_I2C_BYTE                                                                      \
	(FIELD_PREP(IFX_SCB_CTRL_MODE, 0U) | FIELD_PREP(IFX_SCB_CTRL_MEM_WIDTH, 0U))

#define IFX_SCB_I2C_CTRL_HIGH_PHASE       GENMASK(3, 0)
#define IFX_SCB_I2C_CTRL_LOW_PHASE        GENMASK(7, 4)
#define IFX_SCB_I2C_CTRL_S_GENERAL_IGNORE BIT(11)
#define IFX_SCB_I2C_CTRL_SLAVE_MODE       BIT(30)
#define IFX_SCB_I2C_CTRL_MASTER_MODE      BIT(31)

#define IFX_SCB_I2C_STATUS_S_READ BIT(4)

#define IFX_SCB_M_CMD_START         BIT(0)
#define IFX_SCB_M_CMD_START_ON_IDLE BIT(1)
#define IFX_SCB_M_CMD_ACK           BIT(2)
#define IFX_SCB_M_CMD_NACK          BIT(3)
#define IFX_SCB_M_CMD_STOP          BIT(4)

#define IFX_SCB_S_CMD_ACK  BIT(0)
#define IFX_SCB_S_CMD_NACK BIT(1)

#define IFX_SCB_I2C_CFG_SDA_IN_FILT_SEL  BIT(4)
#define IFX_SCB_I2C_CFG_SCL_IN_FILT_SEL  BIT(12)
#define IFX_SCB_I2C_CFG_SDA_OUT_FILT_SEL GENMASK(29, 28)
#define IFX_SCB_I2C_CFG_FILT_SEL                                                                   \
	(IFX_SCB_I2C_CFG_SDA_IN_FILT_SEL | IFX_SCB_I2C_CFG_SCL_IN_FILT_SEL |                       \
	 IFX_SCB_I2C_CFG_SDA_OUT_FILT_SEL)
#define IFX_SCB_I2C_CFG_ANALOG_IN                                                                  \
	(IFX_SCB_I2C_CFG_SDA_IN_FILT_SEL | IFX_SCB_I2C_CFG_SCL_IN_FILT_SEL)

#define IFX_SCB_FRAME_DATA_WIDTH GENMASK(4, 0)
#define IFX_SCB_FRAME_MSB_FIRST  BIT(8)
#define IFX_SCB_FRAME_8BIT     (FIELD_PREP(IFX_SCB_FRAME_DATA_WIDTH, 7U) | IFX_SCB_FRAME_MSB_FIRST)
#define IFX_SCB_RX_CTRL_MEDIAN BIT(9)

#define IFX_SCB_FIFO_CTRL_TRIGGER_LEVEL GENMASK(7, 0)
#define IFX_SCB_FIFO_CTRL_CLEAR         BIT(16)

#define IFX_SCB_TX_FIFO_STATUS_USED     GENMASK(8, 0)
#define IFX_SCB_TX_FIFO_STATUS_SR_VALID BIT(15)

#define IFX_SCB_RX_MATCH_ADDR GENMASK(7, 0)
#define IFX_SCB_RX_MATCH_MASK GENMASK(23, 16)

#define IFX_SCB_INTR_M_ARB_LOST  BIT(0)
#define IFX_SCB_INTR_M_NACK      BIT(1)
#define IFX_SCB_INTR_M_ACK       BIT(2)
#define IFX_SCB_INTR_M_STOP      BIT(4)
#define IFX_SCB_INTR_M_BUS_ERROR BIT(8)
#define IFX_SCB_INTR_M_EVENTS                                                                      \
	(IFX_SCB_INTR_M_ARB_LOST | IFX_SCB_INTR_M_NACK | IFX_SCB_INTR_M_STOP |                     \
	 IFX_SCB_INTR_M_BUS_ERROR)

#define IFX_SCB_INTR_S_ARB_LOST   BIT(0)
#define IFX_SCB_INTR_S_NACK       BIT(1)
#define IFX_SCB_INTR_S_STOP       BIT(4)
#define IFX_SCB_INTR_S_ADDR_MATCH BIT(6)
#define IFX_SCB_INTR_S_BUS_ERROR  BIT(8)
#define IFX_SCB_INTR_S_ERRORS     (IFX_SCB_INTR_S_ARB_LOST | IFX_SCB_INTR_S_BUS_ERROR)
#define IFX_SCB_INTR_S_EVENTS                                                                      \
	(IFX_SCB_INTR_S_ERRORS | IFX_SCB_INTR_S_NACK | IFX_SCB_INTR_S_STOP |                       \
	 IFX_SCB_INTR_S_ADDR_MATCH)

#define IFX_SCB_INTR_TX_TRIGGER   BIT(0)
#define IFX_SCB_INTR_TX_EMPTY     BIT(4)
#define IFX_SCB_INTR_TX_UNDERFLOW BIT(6)

#define IFX_SCB_INTR_RX_NOT_EMPTY BIT(2)

#define IFX_SCB_FIFO_DEPTH       128U
#define IFX_SCB_TX_TRIGGER_LEVEL (IFX_SCB_FIFO_DEPTH / 2U)

#define IFX_SCB_I2C_PHASE_MAX 16U
#define IFX_SCB_I2C_ADDR_MAX  0x7FU
#define IFX_SCB_I2C_ADDR_MASK 0xFEU
#define IFX_SCB_I2C_FILL_BYTE 0xFFU

#define IFX_SCB_I2C_DIV_16 1

enum ifx_scb_i2c_state {
	IFX_SCB_I2C_IDLE,
	IFX_SCB_I2C_CTRL_ADDR,
	IFX_SCB_I2C_CTRL_WRITE,
	IFX_SCB_I2C_CTRL_DRAIN,
	IFX_SCB_I2C_CTRL_READ,
	IFX_SCB_I2C_CTRL_STOP,
	IFX_SCB_I2C_TGT_WRITE,
	IFX_SCB_I2C_TGT_READ,
};

struct ifx_scb_i2c_timing {
	uint32_t bitrate;
	uint32_t clk_min;
	uint32_t clk_max;
	uint16_t low_ns;
	uint16_t high_ns;
	uint8_t low_min;
	uint8_t high_min;
	bool median;
};

struct ifx_scb_i2c_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct ifx_clk_peri clk;
	const struct device *src_dev;
	struct ifx_clk src;
	uint32_t div_max;
	uint32_t bitrate;
	k_timeout_t timeout;
	void (*irq_config)(void);
#ifdef CONFIG_I2C_INFINEON_SCB_BUS_RECOVERY
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
#endif
};

struct ifx_scb_i2c_data {
	struct k_mutex bus_lock;
	struct k_sem done;
	struct k_spinlock lock;
	struct i2c_target_config *target;
	struct i2c_msg *msgs;
	uint32_t dev_config;
	uint32_t seg_left;
	uint32_t msg_pos;
	enum ifx_scb_i2c_state state;
	int result;
	uint16_t addr;
	uint8_t num_msgs;
	uint8_t msg_idx;
	uint8_t seg_last;
	bool seg_stop;
	bool owned;
	bool pending;
	bool target_nack;
};

static inline uint32_t ifx_scb_i2c_read(const struct device *dev, mem_addr_t reg)
{
	const struct ifx_scb_i2c_config *config = dev->config;

	return sys_read32(config->base + reg);
}

static inline void ifx_scb_i2c_write(const struct device *dev, mem_addr_t reg, uint32_t val)
{
	const struct ifx_scb_i2c_config *config = dev->config;

	sys_write32(val, config->base + reg);
}

static inline void ifx_scb_i2c_modify(const struct device *dev, mem_addr_t reg, uint32_t clear,
				      uint32_t set)
{
	ifx_scb_i2c_write(dev, reg, (ifx_scb_i2c_read(dev, reg) & ~clear) | set);
}

static inline bool ifx_scb_i2c_is_ctrl(enum ifx_scb_i2c_state state)
{
	return (state >= IFX_SCB_I2C_CTRL_ADDR) && (state <= IFX_SCB_I2C_CTRL_STOP);
}

static inline bool ifx_scb_i2c_is_target(enum ifx_scb_i2c_state state)
{
	return (state == IFX_SCB_I2C_TGT_WRITE) || (state == IFX_SCB_I2C_TGT_READ);
}

static void ifx_scb_i2c_enable(const struct device *dev, bool enable)
{
	ifx_scb_i2c_modify(dev, IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED,
			   enable ? IFX_SCB_CTRL_ENABLED : 0U);
}

static void ifx_scb_i2c_reset(const struct device *dev)
{
	ifx_scb_i2c_enable(dev, false);
	ifx_scb_i2c_enable(dev, true);
}

static void ifx_scb_i2c_fifo_clear(const struct device *dev, mem_addr_t reg)
{
	uint32_t ctrl = ifx_scb_i2c_read(dev, reg);

	ifx_scb_i2c_write(dev, reg, ctrl | IFX_SCB_FIFO_CTRL_CLEAR);
	ifx_scb_i2c_write(dev, reg, ctrl & ~IFX_SCB_FIFO_CTRL_CLEAR);
}

static void ifx_scb_i2c_mask(const struct device *dev, uint32_t m, uint32_t tx, uint32_t rx)
{
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_M_MASK, m);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, tx);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX_MASK, rx);
}

static const struct ifx_scb_i2c_timing *ifx_scb_i2c_timing_get(uint32_t dev_config)
{
	static const struct ifx_scb_i2c_timing timings[] = {
		{
			.bitrate = I2C_BITRATE_STANDARD,
			.clk_min = 1550000U,
			.clk_max = 3200000U,
			.low_ns = 5000U,
			.high_ns = 5000U,
			.low_min = 7U,
			.high_min = 5U,
		},
		{
			.bitrate = I2C_BITRATE_FAST,
			.clk_min = 7820000U,
			.clk_max = 10000000U,
			.low_ns = 1600U,
			.high_ns = 900U,
			.low_min = 7U,
			.high_min = 5U,
		},
		{
			.bitrate = I2C_BITRATE_FAST_PLUS,
			.clk_min = 14320000U,
			.clk_max = 25800000U,
			.low_ns = 620U,
			.high_ns = 380U,
			.low_min = 8U,
			.high_min = 6U,
			.median = true,
		},
	};

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		return &timings[0];
	case I2C_SPEED_FAST:
		return &timings[1];
	case I2C_SPEED_FAST_PLUS:
		return &timings[2];
	default:
		return NULL;
	}
}

static uint32_t ifx_scb_i2c_phases(const struct ifx_scb_i2c_timing *timing, uint32_t clk,
				   uint32_t *low, uint32_t *high)
{
	uint32_t lo = MAX(timing->low_min,
			  (uint32_t)DIV_ROUND_UP((uint64_t)timing->low_ns * clk, NSEC_PER_SEC));
	uint32_t hi = MAX(timing->high_min,
			  (uint32_t)DIV_ROUND_UP((uint64_t)timing->high_ns * clk, NSEC_PER_SEC));
	uint32_t total = MAX(lo + hi, DIV_ROUND_UP(clk, timing->bitrate));
	uint32_t extra;

	if ((lo > IFX_SCB_I2C_PHASE_MAX) || (hi > IFX_SCB_I2C_PHASE_MAX) ||
	    (total > (2U * IFX_SCB_I2C_PHASE_MAX))) {
		return 0U;
	}

	extra = total - lo - hi;
	lo += DIV_ROUND_UP(extra, 2U);
	hi += extra / 2U;

	if (lo > IFX_SCB_I2C_PHASE_MAX) {
		hi += lo - IFX_SCB_I2C_PHASE_MAX;
		lo = IFX_SCB_I2C_PHASE_MAX;
	}

	if (hi > IFX_SCB_I2C_PHASE_MAX) {
		lo += hi - IFX_SCB_I2C_PHASE_MAX;
		hi = IFX_SCB_I2C_PHASE_MAX;
	}

	*low = lo;
	*high = hi;

	return clk / total;
}

static uint32_t ifx_scb_i2c_divider(const struct ifx_scb_i2c_timing *timing, uint32_t src,
				    uint32_t div_max)
{
	uint32_t last = MIN(src / timing->clk_min, div_max);
	uint32_t best_rate = 0U;
	uint32_t best_div = 0U;
	uint32_t low;
	uint32_t high;

	for (uint32_t div = MAX(DIV_ROUND_UP(src, timing->clk_max), 1U); div <= last; div++) {
		uint32_t rate = ifx_scb_i2c_phases(timing, src / div, &low, &high);

		if (rate > best_rate) {
			best_rate = rate;
			best_div = div;
		}
	}

	return best_div;
}

static int ifx_scb_i2c_set_speed(const struct device *dev, uint32_t dev_config)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	const struct ifx_scb_i2c_timing *timing = ifx_scb_i2c_timing_get(dev_config);
	uint32_t src_rate;
	uint32_t rate;
	uint32_t div;
	uint32_t low;
	uint32_t high;
	int ret;

	if (timing == NULL) {
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(config->src_dev, (clock_control_subsys_t)&config->src,
				     &src_rate);
	if (ret != 0) {
		return ret;
	}

	div = ifx_scb_i2c_divider(timing, src_rate, config->div_max);
	if (div == 0U) {
		LOG_ERR("No SCB clock for %u Hz from %u Hz", timing->bitrate, src_rate);
		return -ENOTSUP;
	}

	rate = src_rate / div;
	ret = clock_control_set_rate(config->clk_dev, (clock_control_subsys_t)&config->clk,
				     (clock_control_subsys_rate_t)&rate);
	if (ret != 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&config->clk, &rate);
	if (ret != 0) {
		return ret;
	}

	if (ifx_scb_i2c_phases(timing, rate, &low, &high) == 0U) {
		LOG_ERR("SCB clock %u Hz unusable for %u Hz", rate, timing->bitrate);
		return -EIO;
	}

	ifx_scb_i2c_modify(dev, IFX_SCB_I2C_CTRL,
			   IFX_SCB_I2C_CTRL_LOW_PHASE | IFX_SCB_I2C_CTRL_HIGH_PHASE,
			   FIELD_PREP(IFX_SCB_I2C_CTRL_LOW_PHASE, low - 1U) |
				   FIELD_PREP(IFX_SCB_I2C_CTRL_HIGH_PHASE, high - 1U));
	ifx_scb_i2c_modify(dev, IFX_SCB_RX_CTRL, IFX_SCB_RX_CTRL_MEDIAN,
			   timing->median ? IFX_SCB_RX_CTRL_MEDIAN : 0U);
	ifx_scb_i2c_modify(dev, IFX_SCB_I2C_CFG, IFX_SCB_I2C_CFG_FILT_SEL,
			   timing->median ? 0U : IFX_SCB_I2C_CFG_ANALOG_IN);

	return 0;
}

static void ifx_scb_i2c_target_match(const struct device *dev, uint16_t address)
{
	ifx_scb_i2c_write(dev, IFX_SCB_RX_MATCH,
			  FIELD_PREP(IFX_SCB_RX_MATCH_ADDR, (uint32_t)address << 1) |
				  FIELD_PREP(IFX_SCB_RX_MATCH_MASK, IFX_SCB_I2C_ADDR_MASK));
}

static void ifx_scb_i2c_quiesce(const struct device *dev)
{
	ifx_scb_i2c_write(dev, IFX_SCB_CTRL, IFX_SCB_CTRL_I2C_BYTE);
	ifx_scb_i2c_mask(dev, 0U, 0U, 0U);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_S_MASK, 0U);
}

static int ifx_scb_i2c_hw_init(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t i2c_ctrl = IFX_SCB_I2C_CTRL_MASTER_MODE | IFX_SCB_I2C_CTRL_S_GENERAL_IGNORE;
	int ret;

	ifx_scb_i2c_quiesce(dev);
	ifx_scb_i2c_write(dev, IFX_SCB_TX_CTRL, IFX_SCB_FRAME_8BIT);
	ifx_scb_i2c_write(dev, IFX_SCB_RX_CTRL, IFX_SCB_FRAME_8BIT);
	ifx_scb_i2c_write(dev, IFX_SCB_TX_FIFO_CTRL,
			  FIELD_PREP(IFX_SCB_FIFO_CTRL_TRIGGER_LEVEL, IFX_SCB_TX_TRIGGER_LEVEL));
	ifx_scb_i2c_write(dev, IFX_SCB_RX_FIFO_CTRL, 0U);

	if (data->target != NULL) {
		i2c_ctrl |= IFX_SCB_I2C_CTRL_SLAVE_MODE;
		ifx_scb_i2c_target_match(dev, data->target->address);
	} else {
		ifx_scb_i2c_write(dev, IFX_SCB_RX_MATCH, 0U);
	}

	ifx_scb_i2c_write(dev, IFX_SCB_I2C_CTRL, i2c_ctrl);

	ret = ifx_scb_i2c_set_speed(dev, data->dev_config);
	if (ret != 0) {
		return ret;
	}

	if (data->target != NULL) {
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_S_MASK, IFX_SCB_INTR_S_EVENTS);
	}

	data->state = IFX_SCB_I2C_IDLE;
	ifx_scb_i2c_enable(dev, true);

	return 0;
}

static uint8_t ifx_scb_i2c_seg_last(const struct i2c_msg *msgs, uint8_t num_msgs, uint8_t first)
{
	uint8_t dir = msgs[first].flags & I2C_MSG_READ;
	uint8_t last = first;

	while (((last + 1U) < num_msgs) && ((msgs[last].flags & I2C_MSG_STOP) == 0U) &&
	       ((msgs[last + 1U].flags & (I2C_MSG_READ | I2C_MSG_RESTART)) == dir)) {
		last++;
	}

	return last;
}

static int ifx_scb_i2c_check_msgs(const struct i2c_msg *msgs, uint8_t num_msgs)
{
	uint8_t first = 0U;

	while (first < num_msgs) {
		uint8_t last = ifx_scb_i2c_seg_last(msgs, num_msgs, first);
		uint32_t len = 0U;

		for (uint8_t i = first; i <= last; i++) {
			if ((msgs[i].flags & I2C_MSG_ADDR_10_BITS) != 0U) {
				return -ENOTSUP;
			}

			if (((msgs[i].len > 0U) && (msgs[i].buf == NULL)) ||
			    (msgs[i].len > (UINT32_MAX - len))) {
				return -EINVAL;
			}

			len += msgs[i].len;
		}

		if (((msgs[first].flags & I2C_MSG_READ) != 0U) && (len == 0U)) {
			return -EINVAL;
		}

		first = last + 1U;
	}

	return 0;
}

static uint8_t *ifx_scb_i2c_cursor(struct ifx_scb_i2c_data *data)
{
	while (data->msg_pos == data->msgs[data->msg_idx].len) {
		data->msg_idx++;
		data->msg_pos = 0U;
	}

	data->seg_left--;

	return &data->msgs[data->msg_idx].buf[data->msg_pos++];
}

static void ifx_scb_i2c_advance(struct ifx_scb_i2c_data *data)
{
	data->msg_idx = data->seg_last + 1U;
	data->msg_pos = 0U;
}

static uint32_t ifx_scb_i2c_segment(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	uint8_t last = ifx_scb_i2c_seg_last(data->msgs, data->num_msgs, data->msg_idx);
	bool read = (data->msgs[data->msg_idx].flags & I2C_MSG_READ) != 0U;

	data->seg_last = last;
	data->seg_stop =
		((data->msgs[last].flags & I2C_MSG_STOP) != 0U) || ((last + 1U) == data->num_msgs);
	data->seg_left = 0U;
	data->msg_pos = 0U;

	for (uint8_t i = data->msg_idx; i <= last; i++) {
		data->seg_left += data->msgs[i].len;
	}

	ifx_scb_i2c_write(dev, IFX_SCB_INTR_M, IFX_SCB_INTR_M_ACK);

	if (read) {
		data->state = IFX_SCB_I2C_CTRL_READ;
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_NOT_EMPTY);
		ifx_scb_i2c_mask(dev, IFX_SCB_INTR_M_EVENTS, 0U, IFX_SCB_INTR_RX_NOT_EMPTY);
	} else {
		data->state = IFX_SCB_I2C_CTRL_ADDR;
		ifx_scb_i2c_mask(dev, IFX_SCB_INTR_M_EVENTS | IFX_SCB_INTR_M_ACK, 0U, 0U);
	}

	return ((uint32_t)data->addr << 1) | (read ? 1U : 0U);
}

static void ifx_scb_i2c_start(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t addr;

	data->owned = false;
	ifx_scb_i2c_fifo_clear(dev, IFX_SCB_TX_FIFO_CTRL);
	ifx_scb_i2c_fifo_clear(dev, IFX_SCB_RX_FIFO_CTRL);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_M, IFX_SCB_INTR_M_EVENTS | IFX_SCB_INTR_M_ACK);

	addr = ifx_scb_i2c_segment(dev);
	ifx_scb_i2c_write(dev, IFX_SCB_TX_FIFO_WR, addr);
	ifx_scb_i2c_write(dev, IFX_SCB_I2C_M_CMD, IFX_SCB_M_CMD_START_ON_IDLE);
}

static void ifx_scb_i2c_restart(const struct device *dev, uint32_t cmd)
{
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t addr;

	ifx_scb_i2c_advance(data);
	addr = ifx_scb_i2c_segment(dev);
	ifx_scb_i2c_write(dev, IFX_SCB_I2C_M_CMD, IFX_SCB_M_CMD_START | cmd);
	ifx_scb_i2c_write(dev, IFX_SCB_TX_FIFO_WR, addr);
}

static void ifx_scb_i2c_stop(const struct device *dev, uint32_t cmd)
{
	struct ifx_scb_i2c_data *data = dev->data;

	ifx_scb_i2c_advance(data);
	data->state = IFX_SCB_I2C_CTRL_STOP;
	ifx_scb_i2c_write(dev, IFX_SCB_I2C_M_CMD, IFX_SCB_M_CMD_STOP | cmd);
}

static void ifx_scb_i2c_finish(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;

	ifx_scb_i2c_mask(dev, 0U, 0U, 0U);
	data->state = IFX_SCB_I2C_IDLE;
	k_sem_give(&data->done);
}

static void ifx_scb_i2c_drain(const struct device *dev)
{
	uint32_t status = ifx_scb_i2c_read(dev, IFX_SCB_TX_FIFO_STATUS);

	if ((status & (IFX_SCB_TX_FIFO_STATUS_USED | IFX_SCB_TX_FIFO_STATUS_SR_VALID)) == 0U) {
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, 0U);
		ifx_scb_i2c_restart(dev, 0U);
	}
}

static void ifx_scb_i2c_fill(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t used = FIELD_GET(IFX_SCB_TX_FIFO_STATUS_USED,
				  ifx_scb_i2c_read(dev, IFX_SCB_TX_FIFO_STATUS));
	uint32_t room = IFX_SCB_FIFO_DEPTH - MIN(used, IFX_SCB_FIFO_DEPTH);

	while ((room > 0U) && (data->seg_left > 0U)) {
		ifx_scb_i2c_write(dev, IFX_SCB_TX_FIFO_WR, *ifx_scb_i2c_cursor(data));
		room--;
	}

	if (data->seg_left > 0U) {
		data->state = IFX_SCB_I2C_CTRL_WRITE;
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_TRIGGER);
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, IFX_SCB_INTR_TX_TRIGGER);
		return;
	}

	if (data->seg_stop) {
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, 0U);
		ifx_scb_i2c_stop(dev, 0U);
		return;
	}

	data->state = IFX_SCB_I2C_CTRL_DRAIN;
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_UNDERFLOW);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, IFX_SCB_INTR_TX_UNDERFLOW);
	ifx_scb_i2c_drain(dev);
}

static void ifx_scb_i2c_recv(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;

	*ifx_scb_i2c_cursor(data) = (uint8_t)ifx_scb_i2c_read(dev, IFX_SCB_RX_FIFO_RD);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_NOT_EMPTY);
	data->owned = true;

	if (data->seg_left > 0U) {
		ifx_scb_i2c_write(dev, IFX_SCB_I2C_M_CMD, IFX_SCB_M_CMD_ACK);
		return;
	}

	ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX_MASK, 0U);

	if (data->seg_stop) {
		ifx_scb_i2c_stop(dev, IFX_SCB_M_CMD_NACK);
	} else {
		ifx_scb_i2c_restart(dev, IFX_SCB_M_CMD_NACK);
	}
}

static void ifx_scb_i2c_ctrl_nack(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;

	data->result = -EIO;
	ifx_scb_i2c_mask(dev, IFX_SCB_INTR_M_EVENTS, 0U, 0U);

	if (data->state != IFX_SCB_I2C_CTRL_STOP) {
		data->state = IFX_SCB_I2C_CTRL_STOP;
		ifx_scb_i2c_write(dev, IFX_SCB_I2C_M_CMD, IFX_SCB_M_CMD_NACK | IFX_SCB_M_CMD_STOP);
	}
}

static void ifx_scb_i2c_ctrl_abort(const struct device *dev, uint32_t intr)
{
	struct ifx_scb_i2c_data *data = dev->data;
	bool lost = (intr & IFX_SCB_INTR_M_BUS_ERROR) == 0U;

	data->result = lost ? -EAGAIN : -EIO;

	if (!lost || (data->target == NULL) || data->owned) {
		ifx_scb_i2c_reset(dev);
	}

	ifx_scb_i2c_finish(dev);
}

static void ifx_scb_i2c_stopped(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;

	if (data->state != IFX_SCB_I2C_CTRL_STOP) {
		data->result = -EIO;
	} else if ((data->result == 0) && (data->msg_idx < data->num_msgs)) {
		ifx_scb_i2c_start(dev);
		return;
	}

	ifx_scb_i2c_finish(dev);
}

static void ifx_scb_i2c_ctrl_isr(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t intr = ifx_scb_i2c_read(dev, IFX_SCB_INTR_M_MASKED);

	ifx_scb_i2c_write(dev, IFX_SCB_INTR_M, intr);

	if ((intr & (IFX_SCB_INTR_M_ARB_LOST | IFX_SCB_INTR_M_BUS_ERROR)) != 0U) {
		ifx_scb_i2c_ctrl_abort(dev, intr);
		return;
	}

	if ((intr & IFX_SCB_INTR_M_NACK) != 0U) {
		ifx_scb_i2c_ctrl_nack(dev);
	}

	if (((intr & IFX_SCB_INTR_M_ACK) != 0U) && (data->state == IFX_SCB_I2C_CTRL_ADDR)) {
		data->owned = true;
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_M_MASK, IFX_SCB_INTR_M_EVENTS);
		ifx_scb_i2c_fill(dev);
	}

	if ((data->state == IFX_SCB_I2C_CTRL_READ) &&
	    ((ifx_scb_i2c_read(dev, IFX_SCB_INTR_RX_MASKED) & IFX_SCB_INTR_RX_NOT_EMPTY) != 0U)) {
		ifx_scb_i2c_recv(dev);
	}

	if ((data->state == IFX_SCB_I2C_CTRL_WRITE) &&
	    ((ifx_scb_i2c_read(dev, IFX_SCB_INTR_TX_MASKED) & IFX_SCB_INTR_TX_TRIGGER) != 0U)) {
		ifx_scb_i2c_fill(dev);
	}

	if ((data->state == IFX_SCB_I2C_CTRL_DRAIN) &&
	    ((ifx_scb_i2c_read(dev, IFX_SCB_INTR_TX_MASKED) & IFX_SCB_INTR_TX_UNDERFLOW) != 0U)) {
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_UNDERFLOW);
		ifx_scb_i2c_drain(dev);
	}

	if ((intr & IFX_SCB_INTR_M_STOP) != 0U) {
		ifx_scb_i2c_stopped(dev);
	}
}

static void ifx_scb_i2c_target_end(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;

	ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, 0U);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX_MASK, 0U);
	ifx_scb_i2c_fifo_clear(dev, IFX_SCB_TX_FIFO_CTRL);
	ifx_scb_i2c_fifo_clear(dev, IFX_SCB_RX_FIFO_CTRL);
	data->state = IFX_SCB_I2C_IDLE;
}

static void ifx_scb_i2c_target_error(const struct device *dev, uint32_t intr)
{
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;

	if (ifx_scb_i2c_is_ctrl(data->state)) {
		return;
	}

	if (ifx_scb_i2c_is_target(data->state)) {
		ifx_scb_i2c_target_end(dev);

		if (cb->error != NULL) {
			cb->error(data->target, ((intr & IFX_SCB_INTR_S_ARB_LOST) != 0U)
							? I2C_ERROR_ARBITRATION
							: I2C_ERROR_GENERIC);
		}
	}

	ifx_scb_i2c_reset(dev);
}

static void ifx_scb_i2c_target_recv(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint8_t val = (uint8_t)ifx_scb_i2c_read(dev, IFX_SCB_RX_FIFO_RD);

	ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_NOT_EMPTY);

	if (!data->target_nack && (cb->write_received != NULL)) {
		data->target_nack = cb->write_received(data->target, val) != 0;
	}

	ifx_scb_i2c_write(dev, IFX_SCB_I2C_S_CMD,
			  data->target_nack ? IFX_SCB_S_CMD_NACK : IFX_SCB_S_CMD_ACK);
}

static void ifx_scb_i2c_target_send(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint8_t val = IFX_SCB_I2C_FILL_BYTE;

	if (!data->target_nack) {
		data->target_nack = (cb->read_processed == NULL) ||
				    (cb->read_processed(data->target, &val) != 0);
	}

	ifx_scb_i2c_write(dev, IFX_SCB_TX_FIFO_WR, data->target_nack ? IFX_SCB_I2C_FILL_BYTE : val);
	ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_EMPTY);
}

static void ifx_scb_i2c_target_addr(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint8_t val = IFX_SCB_I2C_FILL_BYTE;
	int ret;

	if (ifx_scb_i2c_is_ctrl(data->state)) {
		ifx_scb_i2c_write(dev, IFX_SCB_I2C_M_CMD, 0U);
		ifx_scb_i2c_mask(dev, 0U, 0U, 0U);
		data->pending = true;
	}

	ifx_scb_i2c_target_end(dev);
	data->target_nack = false;

	if ((ifx_scb_i2c_read(dev, IFX_SCB_I2C_STATUS) & IFX_SCB_I2C_STATUS_S_READ) != 0U) {
		ret = (cb->read_requested != NULL) ? cb->read_requested(data->target, &val)
						   : -ENOTSUP;
		if (ret != 0) {
			ifx_scb_i2c_write(dev, IFX_SCB_I2C_S_CMD, IFX_SCB_S_CMD_NACK);
			return;
		}

		data->state = IFX_SCB_I2C_TGT_READ;
		ifx_scb_i2c_write(dev, IFX_SCB_TX_FIFO_WR, val);
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_EMPTY);
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, IFX_SCB_INTR_TX_EMPTY);
	} else {
		ret = (cb->write_requested != NULL) ? cb->write_requested(data->target) : 0;
		if (ret != 0) {
			ifx_scb_i2c_write(dev, IFX_SCB_I2C_S_CMD, IFX_SCB_S_CMD_NACK);
			return;
		}

		data->state = IFX_SCB_I2C_TGT_WRITE;
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_NOT_EMPTY);
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_RX_MASK, IFX_SCB_INTR_RX_NOT_EMPTY);
	}

	ifx_scb_i2c_write(dev, IFX_SCB_I2C_S_CMD, IFX_SCB_S_CMD_ACK);
}

static void ifx_scb_i2c_target_isr(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint32_t intr = ifx_scb_i2c_read(dev, IFX_SCB_INTR_S_MASKED);

	ifx_scb_i2c_write(dev, IFX_SCB_INTR_S, intr);

	if ((intr & IFX_SCB_INTR_S_ERRORS) != 0U) {
		ifx_scb_i2c_target_error(dev, intr);
		return;
	}

	if ((data->state == IFX_SCB_I2C_TGT_WRITE) &&
	    ((ifx_scb_i2c_read(dev, IFX_SCB_INTR_RX_MASKED) & IFX_SCB_INTR_RX_NOT_EMPTY) != 0U)) {
		ifx_scb_i2c_target_recv(dev);
	}

	if ((intr & IFX_SCB_INTR_S_NACK) != 0U) {
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_TX_MASK, 0U);
	} else if ((data->state == IFX_SCB_I2C_TGT_READ) &&
		   ((ifx_scb_i2c_read(dev, IFX_SCB_INTR_TX_MASKED) & IFX_SCB_INTR_TX_EMPTY) !=
		    0U)) {
		ifx_scb_i2c_target_send(dev);
	}

	if (((intr & IFX_SCB_INTR_S_STOP) != 0U) && ifx_scb_i2c_is_target(data->state)) {
		ifx_scb_i2c_target_end(dev);

		if (cb->stop != NULL) {
			(void)cb->stop(data->target);
		}
	}

	if ((intr & IFX_SCB_INTR_S_ADDR_MATCH) != 0U) {
		ifx_scb_i2c_target_addr(dev);
	}
}

static void ifx_scb_i2c_isr(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (ifx_scb_i2c_is_ctrl(data->state)) {
		ifx_scb_i2c_ctrl_isr(dev);
	}

	if (IS_ENABLED(CONFIG_I2C_TARGET) && (data->target != NULL)) {
		ifx_scb_i2c_target_isr(dev);
	}

	if ((data->state == IFX_SCB_I2C_IDLE) && data->pending) {
		data->pending = false;
		ifx_scb_i2c_start(dev);
	}

	k_spin_unlock(&data->lock, key);
}

static int ifx_scb_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	if (((dev_config & I2C_MODE_CONTROLLER) == 0U) || ((dev_config & I2C_ADDR_10_BITS) != 0U) ||
	    (ifx_scb_i2c_timing_get(dev_config) == NULL)) {
		return -ENOTSUP;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	key = k_spin_lock(&data->lock);
	if (data->state != IFX_SCB_I2C_IDLE) {
		ret = -EBUSY;
	} else {
		ifx_scb_i2c_enable(dev, false);
	}
	k_spin_unlock(&data->lock, key);

	if (ret == 0) {
		ret = ifx_scb_i2c_set_speed(dev, dev_config);
		if (ret == 0) {
			data->dev_config = dev_config;
		} else {
			(void)ifx_scb_i2c_set_speed(dev, data->dev_config);
		}

		ifx_scb_i2c_enable(dev, true);
	}

	k_mutex_unlock(&data->bus_lock);
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int ifx_scb_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct ifx_scb_i2c_data *data = dev->data;

	*dev_config = data->dev_config;

	return 0;
}

static int ifx_scb_i2c_xfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	k_sem_reset(&data->done);

	key = k_spin_lock(&data->lock);
	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->addr = addr;
	data->msg_idx = 0U;
	data->result = 0;

	if (data->state == IFX_SCB_I2C_IDLE) {
		ifx_scb_i2c_start(dev);
	} else {
		data->pending = true;
	}
	k_spin_unlock(&data->lock, key);

	ret = k_sem_take(&data->done, config->timeout);

	key = k_spin_lock(&data->lock);
	if (data->pending || ifx_scb_i2c_is_ctrl(data->state)) {
		if (ifx_scb_i2c_is_ctrl(data->state)) {
			ifx_scb_i2c_mask(dev, 0U, 0U, 0U);
			ifx_scb_i2c_reset(dev);
			data->state = IFX_SCB_I2C_IDLE;
		}

		data->pending = false;
		ret = -ETIMEDOUT;
	} else {
		ret = data->result;
	}
	k_spin_unlock(&data->lock, key);

	if (ret == -ETIMEDOUT) {
		LOG_ERR("Transfer to 0x%02x timed out", addr);
	}

	return ret;
}

static int ifx_scb_i2c_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr)
{
	struct ifx_scb_i2c_data *data = dev->data;
	int ret;

	if (num_msgs == 0U) {
		return 0;
	}

	if (addr > IFX_SCB_I2C_ADDR_MAX) {
		return -EINVAL;
	}

	ret = ifx_scb_i2c_check_msgs(msgs, num_msgs);
	if (ret != 0) {
		return ret;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	if ((data->target != NULL) && (data->target->address == addr)) {
		ret = -EINVAL;
	} else {
		ret = ifx_scb_i2c_xfer(dev, msgs, num_msgs, addr);
	}

	k_mutex_unlock(&data->bus_lock);
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int ifx_scb_i2c_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (!IS_ENABLED(CONFIG_I2C_TARGET) || (cfg == NULL) || (cfg->callbacks == NULL)) {
		return IS_ENABLED(CONFIG_I2C_TARGET) ? -EINVAL : -ENOTSUP;
	}

	if ((cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) != 0U) {
		return -ENOTSUP;
	}

	if (cfg->address > IFX_SCB_I2C_ADDR_MAX) {
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	key = k_spin_lock(&data->lock);

	if (data->target != NULL) {
		ret = -EBUSY;
	} else {
		ifx_scb_i2c_enable(dev, false);
		ifx_scb_i2c_target_match(dev, cfg->address);
		ifx_scb_i2c_modify(dev, IFX_SCB_I2C_CTRL, 0U, IFX_SCB_I2C_CTRL_SLAVE_MODE);
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_S_MASK, IFX_SCB_INTR_S_EVENTS);
		data->target = cfg;
		ifx_scb_i2c_enable(dev, true);
	}

	k_spin_unlock(&data->lock, key);
	k_mutex_unlock(&data->bus_lock);

	if (ret != 0) {
		(void)pm_device_runtime_put(dev);
	}

	return ret;
}

static int ifx_scb_i2c_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (!IS_ENABLED(CONFIG_I2C_TARGET)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	key = k_spin_lock(&data->lock);

	if ((cfg == NULL) || (data->target != cfg)) {
		ret = -EINVAL;
	} else if (ifx_scb_i2c_is_target(data->state)) {
		ret = -EBUSY;
	} else {
		ifx_scb_i2c_enable(dev, false);
		ifx_scb_i2c_write(dev, IFX_SCB_INTR_S_MASK, 0U);
		ifx_scb_i2c_modify(dev, IFX_SCB_I2C_CTRL, IFX_SCB_I2C_CTRL_SLAVE_MODE, 0U);
		ifx_scb_i2c_write(dev, IFX_SCB_RX_MATCH, 0U);
		data->target = NULL;
		ifx_scb_i2c_enable(dev, true);
	}

	k_spin_unlock(&data->lock, key);
	k_mutex_unlock(&data->bus_lock);

	if (ret == 0) {
		(void)pm_device_runtime_put(dev);
	}

	return ret;
}

#ifdef CONFIG_I2C_INFINEON_SCB_BUS_RECOVERY
static void ifx_scb_i2c_set_scl(void *io_context, int state)
{
	const struct ifx_scb_i2c_config *config = io_context;

	(void)gpio_pin_set_dt(&config->scl, state);
}

static void ifx_scb_i2c_set_sda(void *io_context, int state)
{
	const struct ifx_scb_i2c_config *config = io_context;

	(void)gpio_pin_set_dt(&config->sda, state);
}

static int ifx_scb_i2c_get_sda(void *io_context)
{
	const struct ifx_scb_i2c_config *config = io_context;

	return (gpio_pin_get_dt(&config->sda) > 0) ? 1 : 0;
}

static int ifx_scb_i2c_bitbang_recover(const struct ifx_scb_i2c_config *config)
{
	static const struct i2c_bitbang_io io = {
		.set_scl = ifx_scb_i2c_set_scl,
		.set_sda = ifx_scb_i2c_set_sda,
		.get_sda = ifx_scb_i2c_get_sda,
	};
	gpio_flags_t flags = GPIO_INPUT | GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN;
	struct i2c_bitbang bitbang;
	int ret;

	ret = gpio_pin_configure_dt(&config->scl, flags);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->sda, flags);
	if (ret != 0) {
		return ret;
	}

	i2c_bitbang_init(&bitbang, &io, (void *)config);

	ret = i2c_bitbang_configure(&bitbang,
				    I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD));
	if (ret != 0) {
		return ret;
	}

	return i2c_bitbang_recover_bus(&bitbang);
}

static int ifx_scb_i2c_recover_bus(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int restore;
	int ret;

	if ((config->scl.port == NULL) || (config->sda.port == NULL)) {
		return -ENOTSUP;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	key = k_spin_lock(&data->lock);
	if (data->state != IFX_SCB_I2C_IDLE) {
		ret = -EBUSY;
	} else {
		ifx_scb_i2c_enable(dev, false);
	}
	k_spin_unlock(&data->lock, key);

	if (ret == 0) {
		ret = ifx_scb_i2c_bitbang_recover(config);

		restore = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret == 0) {
			ret = restore;
		}

		ifx_scb_i2c_enable(dev, true);
	}

	k_mutex_unlock(&data->bus_lock);
	(void)pm_device_runtime_put(dev);

	return ret;
}
#endif

static int ifx_scb_i2c_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			return ret;
		}

		return ifx_scb_i2c_hw_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		if (k_mutex_lock(&data->bus_lock, K_NO_WAIT) != 0) {
			return -EBUSY;
		}

		key = k_spin_lock(&data->lock);
		if ((data->state != IFX_SCB_I2C_IDLE) || (data->target != NULL)) {
			ret = -EBUSY;
		} else {
			ifx_scb_i2c_enable(dev, false);
			ret = 0;
		}
		k_spin_unlock(&data->lock, key);
		k_mutex_unlock(&data->bus_lock);

		if (ret != 0) {
			return ret;
		}

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);

		return (ret == -ENOENT) ? 0 : ret;
	default:
		return -ENOTSUP;
	}
}

static int ifx_scb_i2c_init(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;

	if (!device_is_ready(config->clk_dev) || !device_is_ready(config->src_dev)) {
		return -ENODEV;
	}

	k_mutex_init(&data->bus_lock);
	k_sem_init(&data->done, 0, 1);
	data->dev_config = I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate);

	ifx_scb_i2c_quiesce(dev);
	config->irq_config();

	return pm_device_driver_init(dev, ifx_scb_i2c_pm_action);
}

static DEVICE_API(i2c, ifx_scb_i2c_api) = {
	.configure = ifx_scb_i2c_configure,
	.get_config = ifx_scb_i2c_get_config,
	.transfer = ifx_scb_i2c_transfer,
	.target_register = ifx_scb_i2c_target_register,
	.target_unregister = ifx_scb_i2c_target_unregister,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
#ifdef CONFIG_I2C_INFINEON_SCB_BUS_RECOVERY
	.recover_bus = ifx_scb_i2c_recover_bus,
#endif
};

#define IFX_SCB_I2C_DIV(n)   DT_INST_CLOCKS_CTLR(n)
#define IFX_SCB_I2C_HF_ID(n) DT_REG_ADDR(DT_CLOCKS_CTLR(DT_PARENT(IFX_SCB_I2C_DIV(n))))
#define IFX_SCB_I2C_DIV_MAX(n)                                                                     \
	((DT_PROP(IFX_SCB_I2C_DIV(n), div_type) == IFX_SCB_I2C_DIV_16) ? BIT(16) : BIT(8))

#ifdef CONFIG_I2C_INFINEON_SCB_BUS_RECOVERY
#define IFX_SCB_I2C_RECOVERY_INIT(n)                                                               \
	.scl = GPIO_DT_SPEC_INST_GET_OR(n, scl_gpios, {0}),                                        \
	.sda = GPIO_DT_SPEC_INST_GET_OR(n, sda_gpios, {0}),
#else
#define IFX_SCB_I2C_RECOVERY_INIT(n)
#endif

#define IFX_SCB_I2C_INIT(n)                                                                        \
	BUILD_ASSERT(DT_PROP(IFX_SCB_I2C_DIV(n), div_type) <= IFX_SCB_I2C_DIV_16,                  \
		     "SCB I2C requires an 8-bit or 16-bit integer clock divider");                 \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(IFX_SCB_I2C_DIV(n), clock_div),                             \
		     "SCB I2C clock divider must not set clock-div");                              \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void ifx_scb_i2c_irq_config_##n(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ifx_scb_i2c_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ifx_scb_i2c_config ifx_scb_i2c_config_##n = {                          \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(IFX_SCB_I2C_DIV(n))),                           \
		.clk = IFX_CLK_PERI_DT_INST_SPEC_GET(n),                                           \
		.src_dev = DEVICE_DT_GET(DT_NODELABEL(clocks)),                                    \
		.src = {.clk = IFX_CLK_HF, .clk_id = IFX_SCB_I2C_HF_ID(n)},                        \
		.div_max = IFX_SCB_I2C_DIV_MAX(n),                                                 \
		.bitrate = DT_INST_PROP_OR(n, clock_frequency, I2C_BITRATE_STANDARD),              \
		.timeout = I2C_DT_INST_TRANSFER_TIMEOUT(n),                                        \
		.irq_config = ifx_scb_i2c_irq_config_##n,                                          \
		IFX_SCB_I2C_RECOVERY_INIT(n)                                                       \
	};                                                                                         \
                                                                                                   \
	static struct ifx_scb_i2c_data ifx_scb_i2c_data_##n;                                       \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, ifx_scb_i2c_pm_action);                                        \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, ifx_scb_i2c_init, PM_DEVICE_DT_INST_GET(n),                   \
				  &ifx_scb_i2c_data_##n, &ifx_scb_i2c_config_##n, POST_KERNEL,     \
				  CONFIG_I2C_INIT_PRIORITY, &ifx_scb_i2c_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SCB_I2C_INIT)
