/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_i2c

#include <errno.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_bee, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#ifdef CONFIG_I2C_BEE_BUS_RECOVERY
#include "i2c_bitbang.h"
#endif

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_i2c.h>
#include <rtl_rcc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_i2c.h>
#include <rtl876x_rcc.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#if defined(CONFIG_SOC_SERIES_RTL8752H)
extern I2C_Status I2C_CheckAbortStatus(I2C_TypeDef *I2Cx);
#endif

struct i2c_bee_context {
	struct i2c_msg *msgs;
	uint8_t num_msgs;
	uint8_t msg_idx;
	uint16_t tx_idx;
	uint16_t rx_idx;
};

struct i2c_bee_config {
	I2C_TypeDef *reg;
	uint32_t bitrate;
	uint16_t clkid;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_cfg_func)(void);
	k_timeout_t transfer_timeout;
#ifdef CONFIG_I2C_BEE_BUS_RECOVERY
	struct gpio_dt_spec scl_gpio;
	struct gpio_dt_spec sda_gpio;
#endif
};

struct i2c_bee_data {
	struct k_sem lock;
	struct k_sem sync_sem;
	struct i2c_bee_context ctx;
	uint32_t dev_config;
	uint8_t errs;
#ifdef CONFIG_I2C_CALLBACK
	i2c_callback_t cb;
	void *userdata;
	struct k_timer timeout_timer;
#endif
};

static inline bool i2c_bee_next_msg_available(struct i2c_bee_context *ctx)
{
	return ctx->msg_idx < ctx->num_msgs;
}

static inline struct i2c_msg *i2c_bee_current_msg(struct i2c_bee_context *ctx)
{
	if (ctx->msg_idx >= ctx->num_msgs) {
		return NULL;
	}
	return &ctx->msgs[ctx->msg_idx];
}

static inline void i2c_bee_advance_msg(struct i2c_bee_context *ctx)
{
	ctx->msg_idx++;
	ctx->tx_idx = 0U;
	ctx->rx_idx = 0U;
}

static int i2c_bee_do_tx(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->reg;
	struct i2c_bee_context *ctx = &data->ctx;
	struct i2c_msg *msg = i2c_bee_current_msg(ctx);
	bool stop_f;
	bool last_byte;
	bool send_stop;

	if (msg == NULL) {
		return 0;
	}

	stop_f = i2c_is_stop_op(msg);

	if (!i2c_is_read_op(msg)) {
		if (ctx->tx_idx >= msg->len) {
			i2c_bee_advance_msg(ctx);
			return 0;
		}

		if (!(i2c->IC_STATUS & I2C_FLAG_TFNF)) {
			return 0;
		}

		last_byte = (ctx->tx_idx >= msg->len - 1U);
		send_stop = stop_f;

		if (last_byte) {
			i2c->IC_DATA_CMD = msg->buf[ctx->tx_idx] | (stop_f ? BIT9 : 0);
			ctx->tx_idx++;
			i2c_bee_advance_msg(ctx);
		} else {
			i2c->IC_DATA_CMD = msg->buf[ctx->tx_idx];
			ctx->tx_idx++;
		}
	} else {
		if (ctx->tx_idx >= msg->len) {
			return 0;
		}

		if (!(i2c->IC_STATUS & I2C_FLAG_TFNF)) {
			return 0;
		}

		last_byte = (ctx->tx_idx >= msg->len - 1U);
		send_stop = last_byte && stop_f;

		i2c->IC_DATA_CMD = BIT8 | (send_stop ? BIT9 : 0);

		ctx->tx_idx++;
	}

	return 0;
}

static int i2c_bee_do_rx(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->reg;
	struct i2c_bee_context *ctx = &data->ctx;
	struct i2c_msg *msg = i2c_bee_current_msg(ctx);

	if (msg == NULL || !(msg->flags & I2C_MSG_READ)) {
		return 0;
	}

	if (!(i2c->IC_STATUS & I2C_FLAG_RFNE)) {
		return 0;
	}

	if (ctx->rx_idx >= msg->len) {
		i2c_bee_advance_msg(ctx);
		return 0;
	}

	msg->buf[ctx->rx_idx] = (uint8_t)i2c->IC_DATA_CMD;
	ctx->rx_idx++;

	if (ctx->rx_idx >= msg->len) {
		i2c_bee_advance_msg(ctx);
	}

	return 0;
}

static void i2c_bee_log_err(struct i2c_bee_data *data)
{
	switch (data->errs) {
	case I2C_ABRT_7B_ADDR_NOACK:
		LOG_ERR("7 bit address no ack");
		break;
	case I2C_ABRT_10ADDR1_NOACK:
	case I2C_ABRT_10ADDR2_NOACK:
		LOG_ERR("10 bit address no ack");
		break;
	case I2C_ABRT_TXDATA_NOACK:
		LOG_ERR("data no ack");
		break;
	case I2C_ARB_LOST:
		LOG_ERR("arbitration lost");
		break;
	case I2C_ERR_TIMEOUT:
		LOG_ERR("timeout");
		break;
	default:
		LOG_ERR("unknown status: 0x%02x", data->errs);
		break;
	}
}

static void i2c_bee_start(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			  uint16_t addr)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = (I2C_TypeDef *)cfg->reg;

	data->ctx.msgs = msgs;
	data->ctx.num_msgs = num_msgs;
	data->ctx.msg_idx = 0U;
	data->ctx.tx_idx = 0U;
	data->ctx.rx_idx = 0U;
	data->errs = I2C_Success;

	I2C_Cmd(i2c, ENABLE);
	I2C_SetSlaveAddress(i2c, addr);

	I2C_INTConfig(i2c, I2C_INT_TX_ABRT | I2C_INT_RX_FULL | I2C_INT_TX_EMPTY, ENABLE);

	i2c_bee_do_tx(dev);
}

static void i2c_bee_complete(const struct device *dev, int result)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = (I2C_TypeDef *)cfg->reg;

	I2C_INTConfig(i2c, I2C_INT_TX_ABRT | I2C_INT_RX_FULL | I2C_INT_TX_EMPTY, DISABLE);
	I2C_Cmd(i2c, DISABLE);

#ifdef CONFIG_I2C_CALLBACK
	/* Cancel the async watchdog; harmless no-op when it is the one that fired. */
	k_timer_stop(&data->timeout_timer);

	if (data->cb != NULL) {
		i2c_callback_t cb = data->cb;
		void *userdata = data->userdata;

		data->cb = NULL;
		data->userdata = NULL;

		k_sem_give(&data->lock);
		cb(dev, result, userdata);
		return;
	}
#endif /* CONFIG_I2C_CALLBACK */

	ARG_UNUSED(result);
	k_sem_give(&data->sync_sem);
}

static int i2c_bee_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = (I2C_TypeDef *)cfg->reg;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

#ifdef CONFIG_I2C_CALLBACK
	data->cb = NULL;
#endif
	k_sem_reset(&data->sync_sem);

	i2c_bee_start(dev, msgs, num_msgs, addr);

	if (k_sem_take(&data->sync_sem, cfg->transfer_timeout) != 0) {
		I2C_INTConfig(i2c, I2C_INT_TX_ABRT | I2C_INT_RX_FULL | I2C_INT_TX_EMPTY, DISABLE);
		I2C_Cmd(i2c, DISABLE);
		k_sem_give(&data->lock);
		LOG_ERR("transfer timed out");
		return -ETIMEDOUT;
	}

	ret = (data->errs == I2C_Success) ? 0 : -EIO;

	k_sem_give(&data->lock);

	if (ret < 0) {
		i2c_bee_log_err(data);
	}

	return ret;
}

#ifdef CONFIG_I2C_CALLBACK
static void i2c_bee_timeout(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	i2c_bee_complete(dev, -ETIMEDOUT);
}

static int i2c_bee_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr, i2c_callback_t cb, void *userdata)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;

	if (cb == NULL) {
		return -EINVAL;
	}

	/* The bus lock is released from the ISR when the transfer completes. */
	if (k_sem_take(&data->lock, K_NO_WAIT) != 0) {
		return -EWOULDBLOCK;
	}

	data->cb = cb;
	data->userdata = userdata;

	if (!K_TIMEOUT_EQ(cfg->transfer_timeout, K_FOREVER)) {
		k_timer_start(&data->timeout_timer, cfg->transfer_timeout, K_NO_WAIT);
	}

	i2c_bee_start(dev, msgs, num_msgs, addr);

	return 0;
}
#endif /* CONFIG_I2C_CALLBACK */

static int i2c_bee_do_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = (I2C_TypeDef *)cfg->reg;
	I2C_InitTypeDef i2c_init_struct;

	/* Only support Controller mode for now, since Target API is not implemented */
	if ((dev_config & I2C_MODE_CONTROLLER) == 0) {
		return -ENOTSUP;
	}

	I2C_Cmd(i2c, DISABLE);

	I2C_StructInit(&i2c_init_struct);

	i2c_init_struct.I2C_DeviveMode = I2C_DeviveMode_Master;

	if (dev_config & I2C_ADDR_10_BITS) {
		i2c_init_struct.I2C_AddressMode = I2C_AddressMode_10BIT;
	} else {
		i2c_init_struct.I2C_AddressMode = I2C_AddressMode_7BIT;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_init_struct.I2C_ClockSpeed = I2C_BITRATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		i2c_init_struct.I2C_ClockSpeed = I2C_BITRATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		i2c_init_struct.I2C_ClockSpeed = I2C_BITRATE_FAST_PLUS;
		break;
	default:
		return -EINVAL;
	}

	I2C_Init(i2c, &i2c_init_struct);

	I2C_Cmd(i2c, ENABLE);

	data->dev_config = dev_config;

	return 0;
}

static int i2c_bee_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_bee_data *data = dev->data;
	int err;

	k_sem_take(&data->lock, K_FOREVER);
	err = i2c_bee_do_configure(dev, dev_config);
	k_sem_give(&data->lock);

	return err;
}

static int i2c_bee_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_bee_data *data = dev->data;

	*dev_config = data->dev_config;

	return 0;
}

#ifdef CONFIG_I2C_BEE_BUS_RECOVERY
#if defined(CONFIG_SOC_SERIES_RTL8752H)
/*
 * RTL8752H has no hardware open-drain output mode, so open-drain is emulated by
 * switching the pin direction on every edge: released == input with pull-up,
 * driven low == output low.
 */
static void i2c_bee_bitbang_set_line(const struct gpio_dt_spec *gpio, int state)
{
	(void)gpio_pin_configure_dt(gpio, state ? (GPIO_INPUT | GPIO_PULL_UP) : GPIO_OUTPUT_LOW);
}

static void i2c_bee_bitbang_set_scl(void *io_context, int state)
{
	const struct i2c_bee_config *cfg = io_context;

	i2c_bee_bitbang_set_line(&cfg->scl_gpio, state);
}

static void i2c_bee_bitbang_set_sda(void *io_context, int state)
{
	const struct i2c_bee_config *cfg = io_context;

	i2c_bee_bitbang_set_line(&cfg->sda_gpio, state);
}
#else
/*
 * Other Bee SoCs support a hardware open-drain output mode, so the lines are
 * configured once (see i2c_bee_recover_bus) and each edge only writes the data
 * register - fast enough to meet the SCL timing and free of the transient that
 * re-running the full pad configuration on every edge would cause.
 */
static void i2c_bee_bitbang_set_scl(void *io_context, int state)
{
	const struct i2c_bee_config *cfg = io_context;

	gpio_pin_set_dt(&cfg->scl_gpio, state);
}

static void i2c_bee_bitbang_set_sda(void *io_context, int state)
{
	const struct i2c_bee_config *cfg = io_context;

	gpio_pin_set_dt(&cfg->sda_gpio, state);
}
#endif

static int i2c_bee_bitbang_get_sda(void *io_context)
{
	const struct i2c_bee_config *cfg = io_context;

	return gpio_pin_get_dt(&cfg->sda_gpio) != 0 ? 1 : 0;
}

static int i2c_bee_recover_bus(const struct device *dev)
{
	const struct i2c_bee_config *cfg = dev->config;
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bitbang_io bitbang_io = {
		.set_scl = i2c_bee_bitbang_set_scl,
		.set_sda = i2c_bee_bitbang_set_sda,
		.get_sda = i2c_bee_bitbang_get_sda,
	};
	struct i2c_bitbang bitbang;
	uint32_t dev_config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->scl_gpio) || !gpio_is_ready_dt(&cfg->sda_gpio)) {
		LOG_ERR("SCL/SDA recovery GPIO not available");
		return -ENOSYS;
	}

	k_sem_take(&data->lock, K_FOREVER);

	ret = 0;
#if !defined(CONFIG_SOC_SERIES_RTL8752H)
	/*
	 * Configure both lines once as open-drain outputs. The bit-bang helper
	 * then only writes the data register on each edge (a single register
	 * access), which is fast enough to meet the SCL timing and never
	 * re-runs the full pad configuration, so the line does not glitch while
	 * toggling between the driven-low and released states.
	 */
	ret = gpio_pin_configure_dt(&cfg->scl_gpio,
				    GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	if (ret == 0) {
		ret = gpio_pin_configure_dt(&cfg->sda_gpio,
					    GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	}
#endif

	if (ret == 0) {
		i2c_bitbang_init(&bitbang, &bitbang_io, (void *)cfg);

		ret = i2c_bitbang_recover_bus(&bitbang);
		if (ret < 0) {
			LOG_ERR("failed to recover bus (%d)", ret);
		}
	} else {
		LOG_ERR("failed to configure recovery GPIO (%d)", ret);
	}

	(void)i2c_bee_get_config(dev, &dev_config);
	(void)pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);
	(void)i2c_bee_do_configure(dev, dev_config);

	k_sem_give(&data->lock);

	return ret;
}
#endif /* CONFIG_I2C_BEE_BUS_RECOVERY */

static void i2c_bee_isr(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->reg;

	if (I2C_GetINTStatus(i2c, I2C_INT_TX_ABRT)) {
		data->errs = I2C_CheckAbortStatus(i2c);
		I2C_ClearINTPendingBit(i2c, I2C_INT_TX_ABRT);
		I2C_ClearINTPendingBit(i2c, I2C_INT_RX_FULL);
		I2C_ClearINTPendingBit(i2c, I2C_INT_TX_EMPTY);
		i2c_bee_complete(dev, -EIO);
		return;
	}

	if (I2C_GetINTStatus(i2c, I2C_INT_RX_FULL)) {
		i2c_bee_do_rx(dev);
		I2C_ClearINTPendingBit(i2c, I2C_INT_RX_FULL);
	}

	if (I2C_GetINTStatus(i2c, I2C_INT_TX_EMPTY)) {
		i2c_bee_do_tx(dev);
		I2C_ClearINTPendingBit(i2c, I2C_INT_TX_EMPTY);
	}

	if (!i2c_bee_next_msg_available(&data->ctx) && !I2C_GetFlagState(i2c, I2C_FLAG_ACTIVITY)) {
		i2c_bee_complete(dev, 0);
	}
}

static int i2c_bee_init(const struct device *dev)
{
	struct i2c_bee_data *data = dev->data;
	const struct i2c_bee_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int err;

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

	k_sem_init(&data->lock, 1, 1);

	k_sem_init(&data->sync_sem, 0, K_SEM_MAX_LIMIT);

#ifdef CONFIG_I2C_CALLBACK
	k_timer_init(&data->timeout_timer, i2c_bee_timeout, NULL);
	k_timer_user_data_set(&data->timeout_timer, (void *)dev);
#endif

	cfg->irq_cfg_func();

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);
	i2c_bee_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);

	return 0;
}

static DEVICE_API(i2c, i2c_bee_driver_api) = {
	.configure = i2c_bee_configure,
	.get_config = i2c_bee_get_config,
	.transfer = i2c_bee_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i2c_bee_transfer_cb,
#endif
#ifdef CONFIG_I2C_BEE_BUS_RECOVERY
	.recover_bus = i2c_bee_recover_bus,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_IRQ_FUNC_DEFINE(index)                                                                 \
	static void i2c_bee_irq_cfg_func_##index(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2c_bee_isr,        \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#ifdef CONFIG_I2C_BEE_BUS_RECOVERY
#define I2C_BEE_RECOVERY_INIT(index)                                                               \
	.scl_gpio = GPIO_DT_SPEC_INST_GET_OR(index, scl_gpios, {0}),                               \
	.sda_gpio = GPIO_DT_SPEC_INST_GET_OR(index, sda_gpios, {0}),
#else
#define I2C_BEE_RECOVERY_INIT(index)
#endif

#define I2C_BEE_INIT(index)                                                                        \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	I2C_IRQ_FUNC_DEFINE(index);                                                                \
	static struct i2c_bee_data i2c_bee_data_##index;                                           \
	const static struct i2c_bee_config i2c_bee_cfg_##index = {                                 \
		.reg = (I2C_TypeDef *)DT_INST_REG_ADDR(index),                                     \
		.bitrate = DT_INST_PROP(index, clock_frequency),                                   \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_cfg_func = i2c_bee_irq_cfg_func_##index,                                      \
		.transfer_timeout = I2C_DT_INST_TRANSFER_TIMEOUT(index),                           \
		I2C_BEE_RECOVERY_INIT(index)                                                       \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_bee_init, NULL, &i2c_bee_data_##index,                \
				  &i2c_bee_cfg_##index, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,     \
				  &i2c_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_BEE_INIT)
