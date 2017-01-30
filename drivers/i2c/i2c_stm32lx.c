/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>

#include <board.h>
#include <i2c.h>
#include <clock_control.h>

#include <misc/util.h>

#include <clock_control/stm32_clock_control.h>
#include "i2c_stm32lx.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <logging/sys_log.h>

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct i2c_stm32lx_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct i2c_stm32lx_data * const)(dev)->driver_data)
#define I2C_STRUCT(dev)							\
	((volatile struct i2c_stm32lx *)(DEV_CFG(dev))->base)

static int i2c_stm32lx_runtime_configure(struct device *dev, uint32_t config)
{
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
	struct i2c_stm32lx_data *data = DEV_DATA(dev);
	const struct i2c_stm32lx_config *cfg = DEV_CFG(dev);
	uint32_t clock;
	uint32_t i2c_h_min_time, i2c_l_min_time;
	uint32_t i2c_hold_time_min, i2c_setup_time_min;
	uint32_t presc = 1;

	data->dev_config.raw = config;

	clock_control_get_rate(data->clock,
			(clock_control_subsys_t *)&cfg->pclken, &clock);

	if (data->dev_config.bits.is_slave_read)
		return -EINVAL;

	/* Disable peripheral */
	i2c->cr1.bit.pe = 0;
	while (i2c->cr1.bit.pe)
		;

	switch (data->dev_config.bits.speed) {
	case I2C_SPEED_STANDARD:
		i2c_h_min_time = 4000;
		i2c_l_min_time = 4700;
		i2c_hold_time_min = 500;
		i2c_setup_time_min = 1250;
		break;
	case I2C_SPEED_FAST:
		i2c_h_min_time = 600;
		i2c_l_min_time = 1300;
		i2c_hold_time_min = 375;
		i2c_setup_time_min = 500;
		break;
	default:
		return -EINVAL;
	}

	/* Calculate period until prescaler matches */
	do {
		uint32_t t_presc = clock / presc;
		uint32_t ns_presc = NSEC_PER_SEC / t_presc;
		uint32_t sclh = i2c_h_min_time / ns_presc;
		uint32_t scll = i2c_l_min_time / ns_presc;
		uint32_t sdadel = i2c_hold_time_min / ns_presc;
		uint32_t scldel = i2c_setup_time_min / ns_presc;

		if ((sclh - 1) > 255 ||
		    (scll - 1) > 255 ||
		    sdadel > 15 ||
		    (scldel - 1) > 15) {
			++presc;
			continue;
		}

		i2c->timingr.bit.presc = presc-1;
		i2c->timingr.bit.scldel = scldel-1;
		i2c->timingr.bit.sdadel = sdadel;
		i2c->timingr.bit.sclh = sclh-1;
		i2c->timingr.bit.scll = scll-1;

		break;
	} while (presc < 16);

	if (presc >= 16) {
		SYS_LOG_DBG("I2C:failed to find prescaler value");
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
static void i2c_stm32lx_ev_isr(void *arg)
{
	struct device * const dev = (struct device *)arg;
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
	struct i2c_stm32lx_data *data = DEV_DATA(dev);

	if (data->current.is_write) {
		if (data->current.len && i2c->isr.bit.txis) {
			i2c->txdr = *data->current.buf;
			data->current.buf++;
			data->current.len--;

			if (!data->current.len)
				k_sem_give(&data->device_sync_sem);
		} else {
			/* Impossible situation */
			data->current.is_err = 1;
			i2c->cr1.bit.txie = 0;

			k_sem_give(&data->device_sync_sem);
		}
	} else {
		if (data->current.len && i2c->isr.bit.rxne) {
			*data->current.buf = i2c->rxdr.bit.data;
			data->current.buf++;
			data->current.len--;

			if (!data->current.len)
				k_sem_give(&data->device_sync_sem);
		} else {
			/* Impossible situation */
			data->current.is_err = 1;
			i2c->cr1.bit.rxie = 0;

			k_sem_give(&data->device_sync_sem);
		}
	}
}

static void i2c_stm32lx_er_isr(void *arg)
{
	struct device * const dev = (struct device *)arg;
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
	struct i2c_stm32lx_data *data = DEV_DATA(dev);

	if (i2c->isr.bit.nackf) {
		i2c->icr.bit.nack = 1;

		data->current.is_nack = 1;

		k_sem_give(&data->device_sync_sem);
	} else {
		/* Unknown Error */
		data->current.is_err = 1;

		k_sem_give(&data->device_sync_sem);
	}
}
#endif

static inline void transfer_setup(struct device *dev, uint16_t slave_address,
				  unsigned int read_transfer)
{
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
	struct i2c_stm32lx_data *data = DEV_DATA(dev);

	if (data->dev_config.bits.use_10_bit_addr) {
		i2c->cr2.bit.add10 = 1;
		i2c->cr2.bit.sadd = slave_address;
	} else
		i2c->cr2.bit.sadd = slave_address << 1;

	i2c->cr2.bit.rd_wrn = !!read_transfer;
}

static inline int msg_write(struct device *dev, struct i2c_msg *msg,
			    unsigned int flags)
{
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	struct i2c_stm32lx_data *data = DEV_DATA(dev);
#endif
	unsigned int len = msg->len;
	uint8_t *buf = msg->buf;

	if (len > 255)
		return -EINVAL;

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	data->current.len = len;
	data->current.buf = buf;
	data->current.is_nack = 0;
	data->current.is_err = 0;
	data->current.is_write = 1;
#endif

	i2c->cr2.bit.nbytes = len;

	if ((flags & I2C_MSG_RESTART) == I2C_MSG_RESTART)
		i2c->cr2.bit.autoend = 0;
	else
		i2c->cr2.bit.autoend = 1;

	i2c->cr2.bit.reload = 0;
	i2c->cr2.bit.start = 1;

	while (i2c->cr2.bit.start)
		;

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	i2c->cr1.bit.txie = 1;
	i2c->cr1.bit.nackie = 1;

	k_sem_take(&data->device_sync_sem, K_FOREVER);

	if (data->current.is_nack || data->current.is_err) {
		i2c->cr1.bit.txie = 0;
		i2c->cr1.bit.nackie = 0;
		if (data->current.is_nack)
			SYS_LOG_DBG("%s: NACK", __func__);
		if (data->current.is_err)
			SYS_LOG_DBG("%s: ERR %d", __func__,
				    data->current.is_err);
		data->current.is_nack = 0;
		data->current.is_err = 0;
		return -EIO;
	}
#else
	while (len) {
		do {
			if (i2c->isr.bit.txis)
				break;

			if (i2c->isr.bit.nackf) {
				i2c->icr.bit.nack = 1;
				SYS_LOG_DBG("%s: NACK", __func__);
				return -EIO;
			}
		} while (1);

		i2c->txdr = *buf;

		buf++;
		len--;
	}
#endif

	if ((flags & I2C_MSG_RESTART) == 0) {
		while (!i2c->isr.bit.stopf)
			;
		i2c->icr.bit.stop = 1;
	}

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	i2c->cr1.bit.txie = 0;
	i2c->cr1.bit.nackie = 0;
#endif

	return 0;
}

static inline int msg_read(struct device *dev, struct i2c_msg *msg,
			   unsigned int flags)
{
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	struct i2c_stm32lx_data *data = DEV_DATA(dev);
#endif
	unsigned int len = msg->len;
	uint8_t *buf = msg->buf;

	if (len > 255)
		return -EINVAL;

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	data->current.len = len;
	data->current.buf = buf;
	data->current.is_err = 0;
	data->current.is_write = 0;
#endif

	i2c->cr2.bit.nbytes = len;

	if ((flags & I2C_MSG_RESTART) == I2C_MSG_RESTART)
		i2c->cr2.bit.autoend = 0;
	else
		i2c->cr2.bit.autoend = 1;

	i2c->cr2.bit.reload = 0;
	i2c->cr2.bit.start = 1;

	while (i2c->cr2.bit.start)
		;

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	i2c->cr1.bit.rxie = 1;

	k_sem_take(&data->device_sync_sem, K_FOREVER);

	if (data->current.is_err) {
		i2c->cr1.bit.rxie = 0;
		if (data->current.is_err)
			SYS_LOG_DBG("%s: ERR %d", __func__,
				    data->current.is_err);
		data->current.is_err = 0;
		return -EIO;
	}
#else
	while (len) {
		while (!i2c->isr.bit.rxne)
			;

		*buf = i2c->rxdr.bit.data;

		buf++;
		len--;
	}
#endif

	if ((flags & I2C_MSG_RESTART) == 0) {
		while (!i2c->isr.bit.stopf)
			;
		i2c->icr.bit.stop = 1;
	}

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	i2c->cr1.bit.rxie = 0;
#endif

	return 0;
}

static int i2c_stm32lx_transfer(struct device *dev,
				struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t slave_address)
{
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
	struct i2c_msg *cur_msg = msgs;
	uint8_t msg_left = num_msgs;
	int ret = 0;

	/* Enable Peripheral */
	i2c->cr1.bit.pe = 1;

	/* Process all messages one-by-one */
	while (msg_left > 0) {
		unsigned int flags = 0;

		if (cur_msg->len > 255)
			return -EINVAL;

		if (msg_left > 1 &&
		    (cur_msg[0].flags & I2C_MSG_RW_MASK) !=
		    (cur_msg[1].flags & I2C_MSG_RW_MASK))
			flags |= I2C_MSG_RESTART;

		if ((cur_msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			transfer_setup(dev, slave_address, 0);
			ret = msg_write(dev, cur_msg, flags);
		} else {
			transfer_setup(dev, slave_address, 1);
			ret = msg_read(dev, cur_msg, flags);
		}

		if (ret < 0) {
			ret = -EIO;
			break;
		}

		cur_msg++;
		msg_left--;
	};

	/* Disable Peripheral */
	i2c->cr1.bit.pe = 0;

	return ret;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_stm32lx_runtime_configure,
	.transfer = i2c_stm32lx_transfer,
};

static inline void __i2c_stm32lx_get_clock(struct device *dev)
{
	struct i2c_stm32lx_data *data = DEV_DATA(dev);
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}

static int i2c_stm32lx_init(struct device *dev)
{
	volatile struct i2c_stm32lx *i2c = I2C_STRUCT(dev);
	struct i2c_stm32lx_data *data = DEV_DATA(dev);
	const struct i2c_stm32lx_config *cfg = DEV_CFG(dev);

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);

	__i2c_stm32lx_get_clock(dev);

	/* enable clock */
	clock_control_on(data->clock,
		(clock_control_subsys_t *)&cfg->pclken);

	/* Reset config */
	i2c->cr1.val = 0;
	i2c->cr2.val = 0;
	i2c->oar1.val = 0;
	i2c->oar2.val = 0;
	i2c->timingr.val = 0;
	i2c->timeoutr.val = 0;
	i2c->pecr.val = 0;
	i2c->icr.val = 0xFFFFFFFF;

	/* Try to Setup HW */
	i2c_stm32lx_runtime_configure(dev, data->dev_config.raw);

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	cfg->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_I2C_0

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
static void i2c_stm32lx_irq_config_func_0(struct device *port);
#endif

static const struct i2c_stm32lx_config i2c_stm32lx_cfg_0 = {
	.base = (uint8_t *)I2C1_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB1,
		    .enr = LL_APB1_GRP1_PERIPH_I2C1 },
#ifdef CONFIG_I2C_STM32LX_INTERRUPT
	.irq_config_func = i2c_stm32lx_irq_config_func_0,
#endif
};

static struct i2c_stm32lx_data i2c_stm32lx_dev_data_0 = {
	.dev_config.raw = CONFIG_I2C_0_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c_stm32lx_0, CONFIG_I2C_0_NAME, &i2c_stm32lx_init,
		    &i2c_stm32lx_dev_data_0, &i2c_stm32lx_cfg_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_I2C_STM32LX_INTERRUPT
static void i2c_stm32lx_irq_config_func_0(struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_STM32L4X
#define PORT_0_EV_IRQ STM32L4_IRQ_I2C1_EV
#define PORT_0_ER_IRQ STM32L4_IRQ_I2C1_ER
#endif

	IRQ_CONNECT(PORT_0_EV_IRQ, CONFIG_I2C_0_IRQ_PRI,
		i2c_stm32lx_ev_isr, DEVICE_GET(i2c_stm32lx_0), 0);
	irq_enable(PORT_0_EV_IRQ);

	IRQ_CONNECT(PORT_0_ER_IRQ, CONFIG_I2C_0_IRQ_PRI,
		i2c_stm32lx_er_isr, DEVICE_GET(i2c_stm32lx_0), 0);
	irq_enable(PORT_0_ER_IRQ);
}
#endif

#endif /* CONFIG_I2C_0 */
