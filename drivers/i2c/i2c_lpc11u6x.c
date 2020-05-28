/*
 * Copyright (c) 2020, Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc11u6x_i2c

#include <kernel.h>
#include <drivers/i2c.h>
#include <drivers/pinmux.h>
#include <drivers/clock_control.h>
#include <dt-bindings/pinctrl/lpc11u6x-pinctrl.h>
#include "i2c_lpc11u6x.h"

#define DEV_CFG(dev) ((dev)->config)
#define DEV_BASE(dev) (((struct lpc11u6x_i2c_config *) DEV_CFG((dev)))->base)
#define DEV_DATA(dev) ((dev)->data)

static void lpc11u6x_i2c_set_bus_speed(const struct lpc11u6x_i2c_config *cfg,
				       struct device *clk_dev,
				       uint32_t speed)
{
	uint32_t clk, div;

	clock_control_get_rate(clk_dev, (clock_control_subsys_t) cfg->clkid,
			       &clk);
	div = clk / speed;

	cfg->base->sclh = div / 2;
	cfg->base->scll = div - (div / 2);
}

static int lpc11u6x_i2c_configure(struct device *dev, uint32_t dev_config)
{
	const struct lpc11u6x_i2c_config *cfg = DEV_CFG(dev);
	struct lpc11u6x_i2c_data *data = DEV_DATA(dev);
	struct device *clk_dev, *pinmux_dev;
	uint32_t speed, flags = 0;

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed = 100000;
		break;
	case I2C_SPEED_FAST:
		speed = 400000;
		break;
	case I2C_SPEED_FAST_PLUS:
		flags |= IOCON_FASTI2C_EN;
		speed = 1000000;
		break;
	case I2C_SPEED_HIGH:
	case I2C_SPEED_ULTRA:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	clk_dev = device_get_binding(cfg->clock_drv);
	if (!clk_dev) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	lpc11u6x_i2c_set_bus_speed(cfg, clk_dev, speed);

	if (!flags) {
		goto exit;
	}

	pinmux_dev = device_get_binding(cfg->scl_pinmux_drv);
	if (!pinmux_dev) {
		goto err;
	}
	pinmux_pin_set(pinmux_dev, cfg->scl_pin, cfg->scl_flags | flags);

	pinmux_dev = device_get_binding(cfg->sda_pinmux_drv);
	if (!pinmux_dev) {
		goto err;
	}
	pinmux_pin_set(pinmux_dev, cfg->sda_pin, cfg->sda_flags | flags);

exit:
	k_mutex_unlock(&data->mutex);
	return 0;
err:
	k_mutex_unlock(&data->mutex);
	return -EINVAL;
}

static int lpc11u6x_i2c_transfer(struct device *dev, struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	const struct lpc11u6x_i2c_config *cfg = DEV_CFG(dev);
	struct lpc11u6x_i2c_data *data = DEV_DATA(dev);
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	data->transfer.msgs = msgs;
	data->transfer.curr_buf = msgs->buf;
	data->transfer.curr_len = msgs->len;
	data->transfer.nr_msgs = num_msgs;
	data->transfer.addr = addr;

	/* Reset all control bits */
	cfg->base->con_clr = LPC11U6X_I2C_CONTROL_SI |
		LPC11U6X_I2C_CONTROL_STOP | LPC11U6X_I2C_CONTROL_START;

	/* Send start and wait for completion */
	data->transfer.status = LPC11U6X_I2C_STATUS_BUSY;
	cfg->base->con_set = LPC11U6X_I2C_CONTROL_START;

	k_sem_take(&data->completion, K_FOREVER);

	if (data->transfer.status != LPC11U6X_I2C_STATUS_OK) {
		ret = -EIO;
	}
	data->transfer.status = LPC11U6X_I2C_STATUS_INACTIVE;

	/* If a slave is registered, put the controller in slave mode */
	if (data->slave) {
		cfg->base->con_set = LPC11U6X_I2C_CONTROL_AA;
	}

	k_mutex_unlock(&data->mutex);
	return ret;
}

static int lpc11u6x_i2c_slave_register(struct device *dev,
				       struct i2c_slave_config *cfg)
{
	const struct lpc11u6x_i2c_config *dev_cfg = DEV_CFG(dev);
	struct lpc11u6x_i2c_data *data = DEV_DATA(dev);
	int ret = 0;

	if (!cfg) {
		return -EINVAL;
	}

	if (cfg->flags & I2C_SLAVE_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	if (data->slave) {
		ret = -EBUSY;
		goto exit;
	}

	data->slave = cfg;
	/* Configure controller to act as slave */
	dev_cfg->base->addr0 = (cfg->address << 1);
	dev_cfg->base->con_clr = LPC11U6X_I2C_CONTROL_START |
		LPC11U6X_I2C_CONTROL_STOP | LPC11U6X_I2C_CONTROL_SI;
	dev_cfg->base->con_set = LPC11U6X_I2C_CONTROL_AA;

exit:
	k_mutex_unlock(&data->mutex);
	return ret;
}


static int lpc11u6x_i2c_slave_unregister(struct device *dev,
					 struct i2c_slave_config *cfg)
{
	const struct lpc11u6x_i2c_config *dev_cfg = DEV_CFG(dev);
	struct lpc11u6x_i2c_data *data = DEV_DATA(dev);

	if (!cfg) {
		return -EINVAL;
	}
	if (data->slave != cfg) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	data->slave = NULL;
	dev_cfg->base->con_clr = LPC11U6X_I2C_CONTROL_AA;
	k_mutex_unlock(&data->mutex);

	return 0;
}

static void lpc11u6x_i2c_isr(void *arg)
{
	struct lpc11u6x_i2c_data *data = DEV_DATA((struct device *)arg);
	struct lpc11u6x_i2c_regs *i2c = DEV_BASE((struct device *) arg);
	struct lpc11u6x_i2c_current_transfer *transfer = &data->transfer;
	uint32_t clear = LPC11U6X_I2C_CONTROL_SI;
	uint32_t set = 0;
	uint8_t val;

	switch (i2c->stat) {
		/* Master TX states */
	case LPC11U6X_I2C_MASTER_TX_START:
	case LPC11U6X_I2C_MASTER_TX_RESTART:
		i2c->dat = (transfer->addr << 1) |
			(transfer->msgs->flags & I2C_MSG_READ);
		clear |= LPC11U6X_I2C_CONTROL_START;
		transfer->curr_buf = transfer->msgs->buf;
		transfer->curr_len = transfer->msgs->len;
		break;

	case LPC11U6X_I2C_MASTER_TX_ADR_ACK:
	case LPC11U6X_I2C_MASTER_TX_DAT_ACK:
		if (!transfer->curr_len) {
			transfer->msgs++;
			transfer->nr_msgs--;
			if (!transfer->nr_msgs) {
				transfer->status = LPC11U6X_I2C_STATUS_OK;
				set |= LPC11U6X_I2C_CONTROL_STOP;
			} else {
				set |= LPC11U6X_I2C_CONTROL_START;
			}
		} else {
			i2c->dat = transfer->curr_buf[0];
			transfer->curr_buf++;
			transfer->curr_len--;
		}
		break;

		/* Master RX states */
	case LPC11U6X_I2C_MASTER_RX_DAT_NACK:
		transfer->msgs++;
		transfer->nr_msgs--;
		set |= (transfer->nr_msgs ? LPC11U6X_I2C_CONTROL_START :
				LPC11U6X_I2C_CONTROL_STOP);
		if (!transfer->nr_msgs) {
			transfer->status = LPC11U6X_I2C_STATUS_OK;
		}
		/* fallthrough */
	case LPC11U6X_I2C_MASTER_RX_DAT_ACK:
		transfer->curr_buf[0] = i2c->dat;
		transfer->curr_buf++;
		transfer->curr_len--;
		/* fallthrough */
	case LPC11U6X_I2C_MASTER_RX_ADR_ACK:
		if (transfer->curr_len <= 1) {
			clear |= LPC11U6X_I2C_CONTROL_AA;
		} else {
			set |= LPC11U6X_I2C_CONTROL_AA;
		}
		break;

		/* Slave States */
	case LPC11U6X_I2C_SLAVE_RX_ADR_ACK:
	case LPC11U6X_I2C_SLAVE_RX_ARB_LOST_ADR_ACK:
	case LPC11U6X_I2C_SLAVE_RX_GC_ACK:
	case LPC11U6X_I2C_SLAVE_RX_ARB_LOST_GC_ACK:
		if (data->slave->callbacks->write_requested(data->slave)) {
			clear |= LPC11U6X_I2C_CONTROL_AA;
		}
		break;

	case LPC11U6X_I2C_SLAVE_RX_DAT_ACK:
	case LPC11U6X_I2C_SLAVE_RX_GC_DAT_ACK:
		val = i2c->dat;
		if (data->slave->callbacks->write_received(data->slave, val)) {
			clear |= LPC11U6X_I2C_CONTROL_AA;
		}
		break;

	case LPC11U6X_I2C_SLAVE_RX_DAT_NACK:
	case LPC11U6X_I2C_SLAVE_RX_GC_DAT_NACK:
		val = i2c->dat;
		data->slave->callbacks->write_received(data->slave, val);
		data->slave->callbacks->stop(data->slave);
		set |= LPC11U6X_I2C_CONTROL_AA;
		break;

	case LPC11U6X_I2C_SLAVE_RX_STOP:
		data->slave->callbacks->stop(data->slave);
		set |= LPC11U6X_I2C_CONTROL_AA;
		break;

	case LPC11U6X_I2C_SLAVE_TX_ADR_ACK:
	case LPC11U6X_I2C_SLAVE_TX_ARB_LOST_ADR_ACK:
		if (data->slave->callbacks->read_requested(data->slave, &val)) {
			clear |= LPC11U6X_I2C_CONTROL_AA;
		}
		i2c->dat = val;
		break;
	case LPC11U6X_I2C_SLAVE_TX_DAT_ACK:
		if (data->slave->callbacks->read_processed(data->slave, &val)) {
			clear |= LPC11U6X_I2C_CONTROL_AA;
		}
		i2c->dat = val;
		break;
	case LPC11U6X_I2C_SLAVE_TX_DAT_NACK:
	case LPC11U6X_I2C_SLAVE_TX_LAST_BYTE:
		data->slave->callbacks->stop(data->slave);
		set |= LPC11U6X_I2C_CONTROL_AA;
		break;

		/* Error cases */
	case LPC11U6X_I2C_MASTER_TX_ADR_NACK:
	case LPC11U6X_I2C_MASTER_RX_ADR_NACK:
	case LPC11U6X_I2C_MASTER_TX_DAT_NACK:
	case LPC11U6X_I2C_MASTER_TX_ARB_LOST:
		transfer->status = LPC11U6X_I2C_STATUS_FAIL;
		set = LPC11U6X_I2C_CONTROL_STOP;
		break;

	default:
		set = LPC11U6X_I2C_CONTROL_STOP;
		break;
	}

	i2c->con_clr = clear;
	i2c->con_set = set;
	if ((transfer->status != LPC11U6X_I2C_STATUS_BUSY) &&
	    (transfer->status != LPC11U6X_I2C_STATUS_INACTIVE)) {
		k_sem_give(&data->completion);
	}
}


static int lpc11u6x_i2c_init(struct device *dev)
{
	const struct lpc11u6x_i2c_config *cfg = DEV_CFG(dev);
	struct lpc11u6x_i2c_data *data = DEV_DATA(dev);
	struct device *pinmux_dev, *clk_dev;

	/* Configure SCL and SDA pins */
	pinmux_dev = device_get_binding(cfg->scl_pinmux_drv);
	if (!pinmux_dev) {
		return -EINVAL;
	}
	pinmux_pin_set(pinmux_dev, cfg->scl_pin, cfg->scl_flags);

	pinmux_dev = device_get_binding(cfg->sda_pinmux_drv);
	if (!pinmux_dev) {
		return -EINVAL;
	}
	pinmux_pin_set(pinmux_dev, cfg->sda_pin, cfg->sda_flags);

	/* Configure clock and de-assert reset for I2Cx */
	clk_dev = device_get_binding(cfg->clock_drv);
	if (!clk_dev) {
		return -EINVAL;
	}
	clock_control_on(clk_dev, (clock_control_subsys_t) cfg->clkid);

	/* Configure bus speed. Default is 100KHz */
	lpc11u6x_i2c_set_bus_speed(cfg, clk_dev, 100000);

	/* Clear all control bytes and enable I2C interface */
	cfg->base->con_clr = LPC11U6X_I2C_CONTROL_AA | LPC11U6X_I2C_CONTROL_SI |
		LPC11U6X_I2C_CONTROL_START | LPC11U6X_I2C_CONTROL_I2C_EN;
	cfg->base->con_set = LPC11U6X_I2C_CONTROL_I2C_EN;

	/* Initialize mutex and semaphore */
	k_mutex_init(&data->mutex);
	k_sem_init(&data->completion, 0, 1);

	data->transfer.status = LPC11U6X_I2C_STATUS_INACTIVE;
	/* Configure IRQ */
	cfg->irq_config_func(dev);
	return 0;
}

static const struct i2c_driver_api i2c_api = {
	.configure = lpc11u6x_i2c_configure,
	.transfer = lpc11u6x_i2c_transfer,
	.slave_register = lpc11u6x_i2c_slave_register,
	.slave_unregister = lpc11u6x_i2c_slave_unregister,
};

#define LPC11U6X_I2C_INIT(idx)						      \
									      \
static void lpc11u6x_i2c_isr_config_##idx(struct device *dev);	              \
									      \
static const struct lpc11u6x_i2c_config i2c_cfg_##idx = {		      \
	.base =								      \
	(struct lpc11u6x_i2c_regs *) DT_INST_REG_ADDR(idx),		      \
	.clock_drv = DT_LABEL(DT_INST_PHANDLE(idx, clocks)),		      \
	.scl_pinmux_drv =						      \
	DT_LABEL(DT_INST_PHANDLE_BY_NAME(idx, pinmuxs, scl)),		      \
	.sda_pinmux_drv =						      \
	DT_LABEL(DT_INST_PHANDLE_BY_NAME(idx, pinmuxs, sda)),		      \
	.irq_config_func = lpc11u6x_i2c_isr_config_##idx,		      \
	.scl_flags =							      \
	DT_INST_PHA_BY_NAME(idx, pinmuxs, scl, function),		      \
	.sda_flags =							      \
	DT_INST_PHA_BY_NAME(idx, pinmuxs, sda, function),		      \
	.scl_pin = DT_INST_PHA_BY_NAME(idx, pinmuxs, scl, pin),		      \
	.sda_pin = DT_INST_PHA_BY_NAME(idx, pinmuxs, sda, pin),		      \
	.clkid = DT_INST_PHA_BY_IDX(idx, clocks, 0, clkid),		      \
};									      \
									      \
static struct lpc11u6x_i2c_data i2c_data_##idx;			              \
									      \
DEVICE_AND_API_INIT(lpc11u6x_i2c_##idx, DT_INST_LABEL(idx),		      \
		    &lpc11u6x_i2c_init,					      \
		    &i2c_data_##idx, &i2c_cfg_##idx,			      \
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,	      \
		    &i2c_api);						      \
									      \
static void lpc11u6x_i2c_isr_config_##idx(struct device *dev)		      \
{									      \
	IRQ_CONNECT(DT_INST_IRQN(idx),					      \
		    DT_INST_IRQ(idx, priority),				      \
		    lpc11u6x_i2c_isr, DEVICE_GET(lpc11u6x_i2c_##idx), 0);     \
									      \
	irq_enable(DT_INST_IRQN(idx));					      \
}

DT_INST_FOREACH_STATUS_OKAY(LPC11U6X_I2C_INIT);
