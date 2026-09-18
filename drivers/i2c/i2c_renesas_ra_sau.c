/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_i2c_sau

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <r_sau_i2c.h>

LOG_MODULE_REGISTER(renesas_ra_i2c_sau, CONFIG_I2C_LOG_LEVEL);

#define SAU_I2C_STCLK_MIN (1)
#define SAU_I2C_STCLK_MAX (127)

void sau_i2c_tei_isr(void);

struct i2c_renesas_ra_sau_config {
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
};

struct i2c_renesas_ra_sau_data {
	sau_i2c_instance_ctrl_t fsp_ctrl;
	i2c_master_cfg_t fsp_cfg;
	sau_i2c_extended_cfg_t fsp_cfg_extend;
	struct k_sem bus_lock;
	struct k_sem complete_sem;
	volatile i2c_master_event_t event;
	uint32_t dev_config;
};

static void i2c_renesas_ra_sau_callback(i2c_master_callback_args_t *p_args)
{
	struct i2c_renesas_ra_sau_data *data = (struct i2c_renesas_ra_sau_data *)p_args->p_context;

	data->event = p_args->event;
	k_sem_give(&data->complete_sem);
}

static int i2c_renesas_ra_sau_calc_clock_settings(const struct device *dev, uint32_t bitrate,
						  uint8_t *stclk)
{
	const struct i2c_renesas_ra_sau_config *cfg = dev->config;
	uint32_t peripheral_clock;
	uint32_t calc_stclk;
	int ret;

	if (bitrate == 0) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys,
				     &peripheral_clock);
	if (ret < 0) {
		LOG_ERR("Failed to get peripheral clock rate: %d", ret);
		return ret;
	}

	/* Hardware formula: STCLK = round(fMCK / (2 * bitrate)) - 1 */
	calc_stclk = DIV_ROUND_CLOSEST(peripheral_clock, (2 * bitrate)) - 1;

	if (calc_stclk < SAU_I2C_STCLK_MIN || calc_stclk > SAU_I2C_STCLK_MAX) {
		LOG_ERR("Bitrate %u not achievable with peripheral clock %u (stclk: %u)", bitrate,
			peripheral_clock, calc_stclk);
		return -EINVAL;
	}

	*stclk = (uint8_t)calc_stclk;
	return 0;
}

static int i2c_renesas_ra_sau_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_renesas_ra_sau_data *data = dev->data;
	i2c_master_rate_t rate;
	uint32_t bitrate;
	uint8_t stclk;
	fsp_err_t fsp_err;
	int ret;

	/* Check controller mode and addressing mode */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Target mode not supported");
		return -ENOTSUP;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bit addressing not supported");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		bitrate = I2C_BITRATE_STANDARD;
		rate = I2C_MASTER_RATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		bitrate = I2C_BITRATE_FAST;
		rate = I2C_MASTER_RATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		bitrate = I2C_BITRATE_FAST_PLUS;
		rate = I2C_MASTER_RATE_FASTPLUS;
		break;
	default:
		LOG_ERR("Unsupported I2C speed: %d", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	ret = i2c_renesas_ra_sau_calc_clock_settings(dev, bitrate, &stclk);
	if (ret < 0) {
		return ret;
	}

	k_sem_take(&data->bus_lock, K_FOREVER);

	/* If already open, close it before applying new configuration */
	if (data->fsp_ctrl.open != 0) {
		fsp_err = R_SAU_I2C_Close(&data->fsp_ctrl);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("R_SAU_I2C_Close failed: %d", fsp_err);
			k_sem_give(&data->bus_lock);
			return -EIO;
		}
	}

	/* Update configuration and reopen the driver */
	data->fsp_cfg.rate = rate;
	data->fsp_cfg_extend.clock_settings.stclk = stclk;

	fsp_err = R_SAU_I2C_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_SAU_I2C_Open failed: %d", fsp_err);
		k_sem_give(&data->bus_lock);
		return -EIO;
	}

	/* Store updated configuration */
	data->dev_config = dev_config;
	k_sem_give(&data->bus_lock);

	return 0;
}

static int i2c_renesas_ra_sau_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_renesas_ra_sau_data *data = dev->data;

	*dev_config = data->dev_config;
	return 0;
}

static int i2c_renesas_ra_sau_transfer(const struct device *dev, struct i2c_msg *msgs,
				       uint8_t num_msgs, uint16_t addr)
{
	struct i2c_renesas_ra_sau_data *data = dev->data;
	fsp_err_t fsp_err;
	uint8_t dummy_buf = 0;
	int ret = 0;

	if (num_msgs == 0) {
		return 0;
	}

	/* SAU Simplified I2C only supports 7-bit addressing */
	if (addr > 0x7F) {
		LOG_ERR("10-bit address not supported: 0x%x", addr);
		return -ENOTSUP;
	}

	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			LOG_ERR("10-bit addressing mode not supported");
			return -ENOTSUP;
		}
	}

	/* Acquire bus lock for exclusive access */
	k_sem_take(&data->bus_lock, K_FOREVER);

	/* Set slave address for the transfer */
	fsp_err = R_SAU_I2C_SlaveAddressSet(&data->fsp_ctrl, addr, I2C_MASTER_ADDR_MODE_7BIT);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_SAU_I2C_SlaveAddressSet failed: %d", fsp_err);
		ret = -EIO;
		goto release_bus;
	}

	/* Process each message in the transfer request */
	for (uint8_t i = 0; i < num_msgs; i++) {
		struct i2c_msg *current = &msgs[i];
		struct i2c_msg *next = (i < num_msgs - 1) ? &msgs[i + 1] : NULL;
		bool restart = (next != NULL) && (next->flags & I2C_MSG_RESTART);
		uint8_t *buf = current->buf != NULL ? current->buf : &dummy_buf;

		data->event = 0;

		if (current->flags & I2C_MSG_READ) {
			if (current->len == 0) {
				ret = -EINVAL;
				goto release_bus;
			}
			fsp_err = R_SAU_I2C_Read(&data->fsp_ctrl, buf, current->len, restart);
		} else {
			fsp_err = R_SAU_I2C_Write(&data->fsp_ctrl, buf, current->len, restart);
		}

		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Transfer initiation failed: %d", fsp_err);
			ret = -EIO;
			goto release_bus;
		}

		/* Wait for transfer completion */
		k_sem_take(&data->complete_sem, K_FOREVER);

		/* Check transfer completion event */
		switch (data->event) {
		case I2C_MASTER_EVENT_TX_COMPLETE:
		case I2C_MASTER_EVENT_RX_COMPLETE:
			break;
		case I2C_MASTER_EVENT_ABORTED:
			LOG_DBG("%s NACK received",
				(current->flags & I2C_MSG_READ) ? "Read" : "Write");
			ret = -EIO;
			goto release_bus;
		default:
			LOG_ERR("Unexpected I2C event: %d", data->event);
			ret = -EIO;
			goto release_bus;
		}
	}

release_bus:
	/* Release bus lock */
	k_sem_give(&data->bus_lock);
	return ret;
}

static int i2c_renesas_ra_sau_init(const struct device *dev)
{
	const struct i2c_renesas_ra_sau_config *cfg = dev->config;
	struct i2c_renesas_ra_sau_data *data = dev->data;
	uint8_t stclk;
	fsp_err_t fsp_err;
	int ret;

	/* Verify clock control device is ready */
	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	/* Enable peripheral clock gate */
	ret = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable clock (%d)", ret);
		return ret;
	}

	/* Configure pin mux and properties */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure pins (%d)", ret);
		return ret;
	}

	/* Initialize synchronization semaphores */
	k_sem_init(&data->bus_lock, 1, 1);
	k_sem_init(&data->complete_sem, 0, 1);

	/* Configure interrupt */
	cfg->irq_config_func(dev);

	switch (data->fsp_cfg.rate) {
	case I2C_MASTER_RATE_STANDARD:
		data->dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case I2C_MASTER_RATE_FAST:
		data->dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case I2C_MASTER_RATE_FASTPLUS:
		data->dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
		break;
	default:
		LOG_ERR("Unsupported default I2C speed: %u Hz", data->fsp_cfg.rate);
		return -EINVAL;
	}

	/* Calculate initial clock divider */
	ret = i2c_renesas_ra_sau_calc_clock_settings(dev, data->fsp_cfg.rate, &stclk);
	if (ret < 0) {
		return ret;
	}

	data->fsp_cfg_extend.clock_settings.stclk = stclk;

	/* Open driver instance with initial configuration */
	fsp_err = R_SAU_I2C_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("R_SAU_I2C_Open failed: %d", fsp_err);
		return -EIO;
	}

	return 0;
}

static DEVICE_API(i2c, i2c_renesas_ra_sau_driver_api) = {
	.configure = i2c_renesas_ra_sau_configure,
	.get_config = i2c_renesas_ra_sau_get_config,
	.transfer = i2c_renesas_ra_sau_transfer,
#if defined(CONFIG_I2C_RTIO)
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx)                                                         \
	DT_CLOCKS_CTLR(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0))

#define RENESAS_RA_SAU_I2C_CHECK_OPERATION_CLOCK(idx)                                              \
	BUILD_ASSERT(                                                                              \
		(IS_EQ(DT_PROP(DT_PARENT(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0)), unit), 0)  \
			 ? (DT_SAME_NODE(I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx),                       \
					 DT_NODELABEL(sau_ck00)) ||                                \
			    DT_SAME_NODE(I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx),                       \
					 DT_NODELABEL(sau_ck01)))                                  \
			 : (IS_EQ(DT_PROP(DT_PARENT(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0)), \
					  unit),                                                   \
				  1)                                                               \
				    ? (DT_SAME_NODE(I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx),            \
						    DT_NODELABEL(sau_ck10)) ||                     \
				       DT_SAME_NODE(I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx),            \
						    DT_NODELABEL(sau_ck11)))                       \
				    : 0)),                                                         \
		"operation_clock not supported")

#define RENESAS_RA_SAU_I2C_OPERATION_CLOCK(idx)                                                    \
	((DT_SAME_NODE(I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx), DT_NODELABEL(sau_ck00)) ||              \
	  DT_SAME_NODE(I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx), DT_NODELABEL(sau_ck10)))                \
		 ? SAU_I2C_MASTER_OPERATION_CLOCK_CK0                                              \
		 : SAU_I2C_MASTER_OPERATION_CLOCK_CK1)

#define I2C_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_DEFINE(idx)                                             \
	static void i2c_renesas_ra_sau_irq_configure_##idx(const struct device *dev)               \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_IRQ(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), irq),             \
			    DT_IRQ(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), priority),        \
			    sau_i2c_tei_isr, NULL, 0);                                             \
	}

#define I2C_RENESAS_RA_SAU_INIT(idx)                                                               \
	RENESAS_RA_SAU_I2C_CHECK_OPERATION_CLOCK(idx);                                             \
                                                                                                   \
	I2C_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_DEFINE(idx)                                             \
	PINCTRL_DT_DEFINE(DT_DRV_INST(idx));                                                       \
                                                                                                   \
	static const struct i2c_renesas_ra_sau_config i2c_renesas_ra_sau_config_##idx = {          \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(idx)),                             \
		.clock_dev = DEVICE_DT_GET(I2C_RENESAS_RA_SAU_CLOCK_CTRL(idx)),                    \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_CLOCKS_CELL_BY_IDX(                           \
					DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), 0, mstp),    \
				.stop_bit = DT_CLOCKS_CELL_BY_IDX(                                 \
					DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), 0,           \
					stop_bit),                                                 \
			},                                                                         \
		.irq_config_func = i2c_renesas_ra_sau_irq_configure_##idx,                         \
	};                                                                                         \
                                                                                                   \
	static struct i2c_renesas_ra_sau_data i2c_renesas_ra_sau_data_##idx = {                    \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.channel =                                                         \
					DT_REG_ADDR(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0)), \
				.rate = DT_INST_PROP_OR(idx, clock_frequency,                      \
							I2C_BITRATE_STANDARD),                     \
				.slave = 0,                                                        \
				.addr_mode = I2C_MASTER_ADDR_MODE_7BIT,                            \
				.p_callback = i2c_renesas_ra_sau_callback,                         \
				.p_context = &i2c_renesas_ra_sau_data_##idx,                       \
				.p_extend = &i2c_renesas_ra_sau_data_##idx.fsp_cfg_extend,         \
				.ipl = DT_IRQ(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0),        \
					      priority),                                           \
				.tei_irq =                                                         \
					DT_IRQ(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), irq), \
				.p_transfer_tx = NULL,                                             \
				.p_transfer_rx = NULL,                                             \
			},                                                                         \
		.fsp_cfg_extend =                                                                  \
			{                                                                          \
				.delay_time = DT_INST_PROP_OR(idx, sda_delay_us, 5),               \
				.i2c_unit = DT_PROP(                                               \
					DT_PARENT(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0)),   \
					unit),                                                     \
				.clock_settings =                                                  \
					{                                                          \
						.operation_clock =                                 \
							RENESAS_RA_SAU_I2C_OPERATION_CLOCK(idx),   \
					},                                                         \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, i2c_renesas_ra_sau_init, NULL, &i2c_renesas_ra_sau_data_##idx,  \
			      &i2c_renesas_ra_sau_config_##idx, POST_KERNEL,                       \
			      CONFIG_I2C_INIT_PRIORITY, &i2c_renesas_ra_sau_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RENESAS_RA_SAU_INIT)
