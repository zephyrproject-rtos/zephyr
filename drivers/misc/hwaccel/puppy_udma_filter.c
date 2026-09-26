/*
 * Copyright (c) 2026 Ana Clara Forcelli <ana.forcelli@lsitec.org.br>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puppy_udma_filter

#include <zephyr/drivers/hwaccel.h>
#include <zephyr/drivers/clock_control.h>
#include <soc.h>

#include "puppy_udma_filter.h"

struct puppy_udma_filter_chan {
	uint32_t add;
	uint32_t cfg;
	uint32_t len_x;
	uint32_t len_y;
	uint32_t len_z;
};

struct puppy_udma_filter {
	struct puppy_udma_filter_chan tx0;
	struct puppy_udma_filter_chan tx1;
	struct puppy_udma_filter_chan rx;
	uint32_t au_cfg;
	uint32_t au_regsum;
	uint32_t au_regmult;
	uint32_t bincu_th;
	uint32_t bincu_cnt;
	uint32_t bincu_setup;
	uint32_t bincu_val;
	uint32_t filt_start;
	uint32_t filt_cmd;
	uint32_t status;
};

struct puppy_udma_filter_config {
	struct accel_driver_config hw_config;
	const struct device *clk_dev;
	clock_control_subsys_t clk_bits;
};

struct puppy_udma_filter_data {
	struct accel_driver_data hw_data;
	hwaccel_irq_callback_user_data_t callback;
	void *user_data;
	volatile struct puppy_udma_filter *filt;
};

#define puppy_FILTER(x) (volatile struct puppy_udma_filter *)(DT_INST_REG_ADDR(x))

static int puppy_udma_filter_query_hw_caps(const struct device *dev, accel_hw_caps_t *caps)
{
	const struct puppy_udma_filter_config *config =
		(const struct puppy_udma_filter_config *)dev->config;

	printk("Caps mask: \n\n\tOP: 0x%x\n\n\tFMT: 0x%x\n\n\tCHAN_DIM: %d, N_CHANS: "
	       "%d\n\n",
	       config->hw_config.caps.op_caps, config->hw_config.caps.fmt_caps,
	       config->hw_config.caps.max_chan_dimension, config->hw_config.caps.max_input_buffers);

	*caps = config->hw_config.caps;
	return 0;
}

static int puppy_udma_filter_start(const struct device *dev)
{
	struct puppy_udma_filter_data *data = (struct puppy_udma_filter_data *)dev->data;
	printk("Filter at 0x%x Start\n", (uint32_t)data->filt);
	data->filt->filt_start = 1;
	return 0;
}

static int puppy_udma_filter_abort(const struct device *dev)
{
	struct puppy_udma_filter_data *data = (struct puppy_udma_filter_data *)dev->data;
	printk("Filter Abort\n");
	return 0;
}

void calc_bytes(int *bytes, int *block_size_bytes, accel_buffer_t *buf,
		volatile struct puppy_udma_filter_chan *chan)
{
	*(bytes) = 0;

	switch (buf->fmt) {
	case (FMT_UINT8):
	case (FMT_INT8):
	default:
		*(block_size_bytes) = 1;
		break;
	case (FMT_UINT16):
	case (FMT_INT16):
		*(block_size_bytes) = 2;
		break;
	case (FMT_UINT32):
	case (FMT_INT32):
		*(block_size_bytes) = 4;
		break;
	}

	printk("Block Size Bytes = %d\n", *(block_size_bytes));

	for (int i = 0; i < buf->dim; i++) {
		switch (i) {
		case (0):
			chan->len_x = buf->len[i];
			printk("Chan X len is %d blocks\n", chan->len_x);
			break; // does this break leave the for?
		case (1):
			chan->len_y = buf->len[i];
			printk("Chan Y len is %d blocks\n", chan->len_y);
			break;
		case (2):
			chan->len_z = buf->len[i];
			printk("Chan Z len is %d blocks\n", chan->len_z);
			break;
		default:
			break;
		}
		*(bytes) += buf->len[i] * *(block_size_bytes);
	}

	printk("Transfer size is %d bytes\n\n", *(bytes));
}

static int puppy_udma_filter_set_buffers(const struct device *dev, accel_buffer_t **in_bufs,
					int nbufs, accel_buffer_t *out_buf)
{
	struct puppy_udma_filter_data *data = (struct puppy_udma_filter_data *)dev->data;
	int bytes, block_size_bytes;

	printk("Transferring buffers from addresses: \n\tA: 0x%x\tB: 0x%x, OUT: 0x%x\n\n",in_bufs[0]->buf, in_bufs[1]->buf, out_buf->buf);

	calc_bytes(&bytes, &block_size_bytes, in_bufs[0], &data->filt->tx0);

	switch (in_bufs[0]->fmt) {
	case (FMT_UINT32):
	case (FMT_UINT16):
	case (FMT_UINT8):
		data->filt->au_cfg &= ~(0x1);
		break;
	case (FMT_INT32):
	case (FMT_INT16):
	case (FMT_INT8):
		data->filt->au_cfg |= 0x1;
		break;
	default:
		break;
	}
	// add different dim support
	data->filt->tx0.cfg = TX_CFG(TX_MODE_LINEAR, block_size_bytes >> 1);
	data->filt->tx0.add = (uint32_t)in_bufs[0]->buf;

	if (nbufs == 2) {
		if (in_bufs[1]->fmt != in_bufs[0]->fmt) {
			printk("Invalid B Buffer Format (A Format is %d, B is %d)\n",
			       in_bufs[0]->fmt, in_bufs[1]->fmt);
			return -EINVAL;
		}

		calc_bytes(&bytes, &block_size_bytes, in_bufs[1], &data->filt->tx1);
		data->filt->tx1.cfg = TX_CFG(TX_MODE_LINEAR, block_size_bytes >> 1);
		data->filt->tx1.add = (uint32_t)in_bufs[1]->buf;
	}

	if (out_buf->fmt != in_bufs[0]->fmt) {
		printk("Invalid OUT Buffer Format (A Format is %d, OUT is %d)\n", in_bufs[0]->fmt,
		       out_buf->fmt);
		return -EINVAL;
	}

	calc_bytes(&bytes, &block_size_bytes, out_buf, &data->filt->rx);
	data->filt->rx.cfg = RX_CFG(RX_MODE_LINEAR, block_size_bytes >> 1);
	data->filt->rx.add = (uint32_t)out_buf->buf;

	printk("Registers: \n\tTX0: 0x%x\tTX1: 0x%x, RX: 0x%x\n\n", data->filt->tx0.add, data->filt->tx1.add, data->filt->rx.add);

	return 0;
}

static int puppy_udma_filter_configure_ops(const struct device *dev, accel_hw_ops_t *ops_series,
					  int nops)
{
	struct puppy_udma_filter_data *data = (struct puppy_udma_filter_data *)dev->data;

	// do something to reset
	switch (ops_series[0]) {
	case (HW_OP_SUM):
		data->filt->au_cfg |= AU_CFG(0, AU_MODE_A_PLUS_B, 0, 0);
		break;
	case (HW_OP_MULT):
		data->filt->au_cfg |= AU_CFG(0, AU_MODE_AxB, 0, 0);
		break;
	case (HW_OP_SUB):
		data->filt->au_cfg |= AU_CFG(0, AU_MODE_A_MINUS_B, 0, 0);
		break;
	default:
		return -ENOTSUP;
	}

	printk("Config AU with OP %d\n", ops_series[0]);

	return 0;
}

static int puppy_udma_filter_irq_callback_set(const struct device *dev,
					     hwaccel_irq_callback_user_data_t cb, void *user_data)
{
	struct puppy_udma_filter_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;

	return 0;
}

static void puppy_udma_filter_irq(void *userdata)
{
	const struct device *dev = (const struct device *)userdata;
	struct puppy_udma_filter_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, data->user_data);
	}
}

static int puppy_udma_common_filter_init(const struct device *dev)
{
	const struct puppy_udma_filter_config *config = dev->config;
	int ret = clock_control_on(config->clk_dev, config->clk_bits);
	if (ret != 0) {
		return ret;
	}
	return 0;
}

static struct accel_driver_api puppy_udma_filter_accel_api = {
	.query_hw_caps = puppy_udma_filter_query_hw_caps,
	.configure_ops = puppy_udma_filter_configure_ops,
	.set_buffers = puppy_udma_filter_set_buffers,
	.set_callback = puppy_udma_filter_irq_callback_set,
	.start = puppy_udma_filter_start,
	.abort = puppy_udma_filter_abort,
};

#define puppy_FILTER_INIT(idx)                                                                      \
	static int puppy_udma_filter_##idx##_init(const struct device *dev)                         \
	{                                                                                          \
		int ret = puppy_udma_common_filter_init(dev);                                       \
		if (ret != 0) {                                                                    \
			return ret;                                                                \
		}                                                                                  \
		IRQ_CONNECT(DT_INST_IRQN(idx), 0, puppy_udma_filter_irq, DEVICE_DT_INST_GET(idx),   \
			    0);                                                                    \
		irq_enable(DT_INST_IRQN(idx));                                                     \
		return 0;                                                                          \
	}                                                                                          \
	static struct puppy_udma_filter_data puppy_udma_filter_##idx##_data = {                      \
		.hw_data =                                                                         \
			{                                                                          \
				.current_config = 0,                                               \
			},                                                                         \
		.filt = puppy_FILTER(idx),                                                          \
	};                                                                                         \
	const static struct puppy_udma_filter_config puppy_udma_filter_##idx##_config = {            \
		.hw_config =                                                                       \
			{                                                                          \
				.caps =                                                            \
					{                                                          \
						.op_caps = BIT(HW_OP_SUM) | BIT(HW_OP_SUB) |       \
							   BIT(HW_OP_MULT) |                       \
							   BIT(HW_OP_SUM_SCALAR) |                 \
							   BIT(HW_OP_SUB_SCALAR) |                 \
							   BIT(HW_OP_MULT_SCALAR) |                \
							   BIT(HW_OP_ACCUMULATE),                  \
						.fmt_caps = BIT(FMT_UINT8) | BIT(FMT_UINT16) |     \
							    BIT(FMT_UINT32) | BIT(FMT_INT8) |      \
							    BIT(FMT_INT16) | BIT(FMT_INT32),       \
						.max_input_buffers = 2,                            \
						.max_chan_dimension = 3,                           \
					},                                                         \
			},                                                                         \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                \
		.clk_bits = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, bits),                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, &puppy_udma_filter_##idx##_init, NULL,                           \
			      &puppy_udma_filter_##idx##_data, &puppy_udma_filter_##idx##_config,    \
			      POST_KERNEL, CONFIG_HWACCEL_INIT_PRIORITY,                           \
			      &puppy_udma_filter_accel_api);

DT_INST_FOREACH_STATUS_OKAY(puppy_FILTER_INIT)
