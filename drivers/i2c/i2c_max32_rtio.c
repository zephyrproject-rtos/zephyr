/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/irq.h>

#include <wrap_max32_i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max32_i2c);

#define ADI_MAX32_I2C_INT_FL0_MASK 0x00FFFFFF
#define ADI_MAX32_I2C_INT_FL1_MASK 0x7

#define ADI_MAX32_I2C_STATUS_MASTER_BUSY BIT(5)

#define I2C_RECOVER_MAX_RETRIES 3
#define I2C_STANDAR_BITRATE_CLKHI 0x12b

static int complete_flag;

/* Driver config */
struct max32_i2c_config {
	mxc_i2c_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
	uint32_t bitrate;
#if defined(CONFIG_I2C_MAX32_INTERRUPT)
	uint8_t irqn;
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct max32_i2c_data {
	mxc_i2c_req_t req;
	const struct device *dev;
	uint8_t target_mode;
	uint8_t flags;
	struct i2c_rtio *ctx;
	uint32_t readb;
	uint32_t written;
	uint8_t second_msg_flag;
#if defined(CONFIG_I2C_MAX32_INTERRUPT)
	int err;
#endif
};

static int max32_configure(const struct device *dev,
				uint32_t dev_cfg)
{
	struct i2c_rtio *const ctx = ((struct max32_i2c_data *)
		dev->data)->ctx;

	return i2c_rtio_configure(ctx, dev_cfg);
}

static int max32_do_configure(const struct device *dev, uint32_t dev_cfg)
{
	int ret = 0;
	const struct max32_i2c_config *const cfg = dev->config;
	mxc_i2c_regs_t *i2c = cfg->regs;

	switch (I2C_SPEED_GET(dev_cfg)) {
	case I2C_SPEED_STANDARD: /** I2C Standard Speed: 100 kHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_STD_MODE);
		break;

	case I2C_SPEED_FAST: /** I2C Fast Speed: 400 kHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_FAST_SPEED);
		break;

#if defined(MXC_I2C_FASTPLUS_SPEED)
	case I2C_SPEED_FAST_PLUS: /** I2C Fast Plus Speed: 1 MHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_FASTPLUS_SPEED);
		break;
#endif

#if defined(MXC_I2C_HIGH_SPEED)
	case I2C_SPEED_HIGH: /** I2C High Speed: 3.4 MHz */
		ret = MXC_I2C_SetFrequency(i2c, MXC_I2C_HIGH_SPEED);
		break;
#endif

	default:
		/* Speed not supported */
		return -ENOTSUP;
	}

	return ret;
}

static void max32_complete(const struct device *dev, int status);

static int max32_msg_start(const struct device *dev, uint8_t flags,
				 uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	int ret = 0;
	const struct max32_i2c_config *const cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;
	mxc_i2c_req_t *req = &data->req;
	uint8_t target_rw;

	req->i2c = i2c;
	req->addr = i2c_addr;

	if (data->second_msg_flag == 0) {
		MXC_I2C_ClearRXFIFO(i2c);
		MXC_I2C_ClearTXFIFO(i2c);
		MXC_I2C_SetRXThreshold(i2c, 1);

		/* First message should always begin with a START condition */
		flags |= I2C_MSG_RESTART;
	}

	if (flags & I2C_MSG_READ) {
		req->rx_buf = (unsigned char *)buf;
		req->rx_len = buf_len;
		req->tx_buf = NULL;
		req->tx_len = 0;
		target_rw = (i2c_addr << 1) | 0x1;
	} else {
		req->tx_buf = (unsigned char *)buf;
		req->tx_len = buf_len;
		req->rx_buf = NULL;
		req->rx_len = 0;
		target_rw = (i2c_addr << 1) & ~0x1;
	}
	data->flags = flags;
	data->readb = 0;
	data->written = 0;
	data->err = 0;

	MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);
	MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ERR, 0);
	Wrap_MXC_I2C_SetRxCount(i2c, req->rx_len);
	if ((data->flags & I2C_MSG_RESTART)) {
		MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
		MXC_I2C_Start(i2c);
		Wrap_MXC_I2C_WaitForRestart(i2c);
		MXC_I2C_WriteTXFIFO(i2c, &target_rw, 1);
	} else {
		if (req->tx_len) {
			data->written = MXC_I2C_WriteTXFIFO(i2c, req->tx_buf, 1);
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
		} else {
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_RX_THD, 0);
		}
	}

	if (data->err) {
		MXC_I2C_Stop(i2c);
		ret = data->err;
	}

	return ret;
}

static int max32_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			uint16_t target_address)
{
	struct i2c_rtio *const ctx = ((struct max32_i2c_data *)
		dev->data)->ctx;
	((struct max32_i2c_data *)dev->data)->second_msg_flag = 0;

	return i2c_rtio_transfer(ctx, msgs, num_msgs, target_address);
}



static void i2c_max32_isr_controller(const struct device *dev, mxc_i2c_regs_t *i2c)
{
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_req_t *req = &data->req;
	uint32_t written, readb;
	uint32_t txfifolevel;
	uint32_t int_fl0, int_fl1;
	uint32_t int_en0, int_en1;

	written = data->written;
	readb = data->readb;

	Wrap_MXC_I2C_GetIntEn(i2c, &int_en0, &int_en1);
	MXC_I2C_GetFlags(i2c, &int_fl0, &int_fl1);
	MXC_I2C_ClearFlags(i2c, ADI_MAX32_I2C_INT_FL0_MASK, ADI_MAX32_I2C_INT_FL1_MASK);
	txfifolevel = Wrap_MXC_I2C_GetTxFIFOLevel(i2c);

	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ERR) {
		data->err = -EIO;
		Wrap_MXC_I2C_SetIntEn(i2c, 0, 0);
		return;
	}

	if (int_fl0 & ADI_MAX32_I2C_INT_FL0_ADDR_ACK) {
		MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
		if (written < req->tx_len) {
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
		} else if (readb < req->rx_len) {
			MXC_I2C_EnableInt(
				i2c, ADI_MAX32_I2C_INT_EN0_RX_THD | ADI_MAX32_I2C_INT_EN0_DONE, 0);
		}
	}

	if (req->tx_len &&
	    (int_fl0 & (ADI_MAX32_I2C_INT_FL0_TX_THD | ADI_MAX32_I2C_INT_FL0_DONE))) {
		if (written < req->tx_len) {
			written += MXC_I2C_WriteTXFIFO(i2c, &req->tx_buf[written],
						       req->tx_len - written);
		} else {
			if (!(int_en0 & ADI_MAX32_I2C_INT_EN0_DONE)) {
				/* We are done, stop sending more data */
				MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_TX_THD, 0);
				if (data->flags & I2C_MSG_STOP) {
					MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
					/* Done flag is not set if stop/restart is not set */
					Wrap_MXC_I2C_Stop(i2c);
				} else {
					complete_flag++;
				}
			}

			if ((int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE)) {
				MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
				complete_flag++;
			}
		}
	} else if ((int_fl0 & (ADI_MAX32_I2C_INT_FL0_RX_THD | ADI_MAX32_I2C_INT_FL0_DONE))) {
		readb += MXC_I2C_ReadRXFIFO(i2c, &req->rx_buf[readb], req->rx_len - readb);
		if (readb == req->rx_len) {
			MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_RX_THD, 0);
			if (data->flags & I2C_MSG_STOP) {
				MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
				Wrap_MXC_I2C_Stop(i2c);
				complete_flag++;
			} else {
				if (int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE) {
					MXC_I2C_DisableInt(i2c, ADI_MAX32_I2C_INT_EN0_DONE, 0);
				}
			}
		} else if ((int_en0 & ADI_MAX32_I2C_INT_EN0_DONE) &&
			   (int_fl0 & ADI_MAX32_I2C_INT_FL0_DONE)) {
			MXC_I2C_DisableInt(
				i2c, (ADI_MAX32_I2C_INT_EN0_RX_THD | ADI_MAX32_I2C_INT_EN0_DONE),
				0);
			Wrap_MXC_I2C_SetRxCount(i2c, req->rx_len - readb);
			MXC_I2C_EnableInt(i2c, ADI_MAX32_I2C_INT_EN0_ADDR_ACK, 0);
			i2c->fifo = (req->addr << 1) | 0x1;
			Wrap_MXC_I2C_Restart(i2c);
		}
	}
	data->written = written;
	data->readb = readb;

	if (complete_flag == 1) {
		max32_complete(dev, 0);
		complete_flag = 0;
	}
}

static bool max32_start(const struct device *dev)
{
	struct max32_i2c_data *data = dev->data;
	struct i2c_rtio *ctx = data->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;
	int res = 0;

	switch (sqe->op) {
	case RTIO_OP_RX:
		return max32_msg_start(dev, I2C_MSG_READ | sqe->iodev_flags,
					    sqe->rx.buf, sqe->rx.buf_len, dt_spec->addr);
	case RTIO_OP_TINY_TX:
		data->second_msg_flag = 0;
		return max32_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
					    (uint8_t *)sqe->tiny_tx.buf, sqe->tiny_tx.buf_len,
					    dt_spec->addr);
	case RTIO_OP_TX:
		return max32_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
					    (uint8_t *)sqe->tx.buf, sqe->tx.buf_len,
					    dt_spec->addr);
	case RTIO_OP_I2C_CONFIGURE:
		res = max32_do_configure(dev, sqe->i2c_config);
		return i2c_rtio_complete(data->ctx, res);
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(data->ctx, -EINVAL);
	}
}

static void max32_complete(const struct device *dev, int status)
{
	struct max32_i2c_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;
	const struct max32_i2c_config *const cfg = dev->config;
	int ret = 0;

	if (cfg->regs->clkhi == I2C_STANDAR_BITRATE_CLKHI) {
		/* When I2C is configured in Standard Bitrate 100KHz
		 * Hardware completes first read sample transaction
		 * and gets stuck in idle instead of starting the
		 * next transaction, if given k_busy_wait for
		 * 20 us ~= 2 additional I2C cycles sample read
		 * won't have any issues but all other transactions
		 * (like setup of sensor) will have this unnecessary
		 * delay. This doesn't happen when using Fast
		 * Bitrate 400Hz.
		 */
		LOG_ERR("For Standard speed HW needs more time to run");
		return;
	}
	if (i2c_rtio_complete(ctx, ret)) {
		data->second_msg_flag = 1;
		max32_start(dev);
	}
}

static void max32_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct max32_i2c_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_submit(ctx, iodev_sqe)) {
		max32_start(dev);
	}
}


static void i2c_max32_isr(const struct device *dev)
{
	const struct max32_i2c_config *cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;

	if (data->target_mode == 0) {
		i2c_max32_isr_controller(dev, i2c);
		return;
	}
}

static int i2c_max32_init(const struct device *dev)
{
	const struct max32_i2c_config *const cfg = dev->config;
	struct max32_i2c_data *data = dev->data;
	mxc_i2c_regs_t *i2c = cfg->regs;
	int ret = 0;

	if (!device_is_ready(cfg->clock)) {
		return -ENODEV;
	}

	MXC_I2C_Shutdown(i2c); /* Clear everything out */

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = MXC_I2C_Init(i2c, 1, 0); /* Configure as master */
	if (ret) {
		return ret;
	}

	MXC_I2C_SetFrequency(i2c, cfg->bitrate);

#if defined(CONFIG_I2C_MAX32_INTERRUPT)
	cfg->irq_config_func(dev);
#endif

#ifdef CONFIG_I2C_MAX32_INTERRUPT
	irq_enable(cfg->irqn);

#endif

	data->dev = dev;

	i2c_rtio_init(data->ctx, dev);
	return ret;
}

static const struct i2c_driver_api max32_driver_api = {
	.configure = max32_configure,
	.transfer = max32_transfer,
	.iodev_submit = max32_submit,
};

#if defined(CONFIG_I2C_TARGET) || defined(CONFIG_I2C_MAX32_INTERRUPT)
#define I2C_MAX32_CONFIG_IRQ_FUNC(n)                                                             \
	.irq_config_func = i2c_max32_irq_config_func_##n, .irqn = DT_INST_IRQN(n),

#define I2C_MAX32_IRQ_CONFIG_FUNC(n)                                                              \
	static void i2c_max32_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_max32_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}
#else
#define I2C_MAX32_CONFIG_IRQ_FUNC(n)
#define I2C_MAX32_IRQ_CONFIG_FUNC(n)
#endif


#define DEFINE_I2C_MAX32(_num)                                                                   \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	I2C_MAX32_IRQ_CONFIG_FUNC(_num)                                                            \
	static const struct max32_i2c_config max32_i2c_dev_cfg_##_num = {                          \
		.regs = (mxc_i2c_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		.bitrate = DT_INST_PROP(_num, clock_frequency),                                    \
		I2C_MAX32_CONFIG_IRQ_FUNC(_num)};                                              \
		I2C_RTIO_DEFINE(_i2c##n##_max32_rtio,				\
		DT_INST_PROP_OR(n, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),	\
		DT_INST_PROP_OR(n, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));	\
	static struct max32_i2c_data max32_i2c_data_##_num = {                                     \
		.ctx = &CONCAT(_i2c, n, _max32_rtio),			\
	};								\
	I2C_DEVICE_DT_INST_DEFINE(_num, i2c_max32_init, NULL, &max32_i2c_data_##_num,              \
				  &max32_i2c_dev_cfg_##_num, PRE_KERNEL_2,                         \
				  CONFIG_I2C_INIT_PRIORITY, &max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_MAX32)
