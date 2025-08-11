/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_iic

#include <math.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include "r_iic_master.h"
#include <errno.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_ra_iic);

typedef void (*init_func_t)(const struct device *dev);
static const double RA_IIC_MASTER_DIV_TIME_NS = 1000000000;

struct i2c_ra_iic_config {
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
	uint32_t noise_filter_stage;
	double rise_time_s;
	double fall_time_s;
	uint32_t duty_cycle_percent;
};

struct i2c_ra_iic_data {
	iic_master_instance_ctrl_t ctrl;
	i2c_master_cfg_t fsp_config;
	struct k_mutex bus_mutex;
	struct k_sem complete_sem;
	i2c_master_event_t event;
	iic_master_extended_cfg_t iic_master_ext_cfg;
	uint32_t dev_config;
};

/* IIC master clock setting calculation function. */
static void calc_iic_master_clock_setting(const struct device *dev, const uint32_t fsp_i2c_rate,
					  iic_master_clock_settings_t *clk_cfg);

/* FSP interruption handlers. */
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);

struct ra_iic_master_bitrate {
	uint32_t bitrate;
	uint32_t duty;
	uint32_t divider;
	uint32_t brl;
	uint32_t brh;
	uint32_t duty_error_percent;
};

static int i2c_ra_iic_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_ra_iic_data *data = (struct i2c_ra_iic_data *const)dev->data;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Only I2C Master mode supported.");
		return -EIO;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->fsp_config.rate = I2C_MASTER_RATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		data->fsp_config.rate = I2C_MASTER_RATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->fsp_config.rate = I2C_MASTER_RATE_FASTPLUS;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate flag: %d", __func__, I2C_SPEED_GET(dev_config));
		return -EIO;
	}

	/* Recalc clock setting after updating config. */
	calc_iic_master_clock_setting(dev, data->fsp_config.rate,
				      &data->iic_master_ext_cfg.clock_settings);

	R_IIC_MASTER_Close(&data->ctrl);
	R_IIC_MASTER_Open(&data->ctrl, &data->fsp_config);

	/* save current devconfig. */
	data->dev_config = dev_config;

	return 0;
}

static int i2c_ra_iic_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_ra_iic_data *data = (struct i2c_ra_iic_data *const)dev->data;
	*dev_config = data->dev_config;

	return 0;
}

#define OPERATION(msg) (((struct i2c_msg *)msg)->flags & I2C_MSG_RW_MASK)

static int i2c_ra_iic_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_ra_iic_data *data = (struct i2c_ra_iic_data *const)dev->data;
	struct i2c_msg *current, *next;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	/* Check for validity of all messages before transfer */
	current = msgs;

	/*
	 * Set I2C_MSG_RESTART flag on first message in order to send start
	 * condition
	 */
	current->flags |= I2C_MSG_RESTART;

	for (int i = 1; i <= num_msgs; i++) {
		if (i < num_msgs) {
			next = current + 1;

			/*
			 * Restart condition between messages
			 * of different directions is required
			 */
			if (OPERATION(current) != OPERATION(next)) {
				if (!(next->flags & I2C_MSG_RESTART)) {
					LOG_ERR("%s: Restart condition between messages of "
						"different directions is required."
						"Current/Total: [%d/%d]",
						__func__, i, num_msgs);
					ret = -EIO;
					break;
				}
			}

			/* Stop condition is only allowed on last message */
			if (current->flags & I2C_MSG_STOP) {
				LOG_ERR("%s: Invalid stop flag. Stop condition is only allowed on "
					"last message. "
					"Current/Total: [%d/%d]",
					__func__, i, num_msgs);
				ret = -EIO;
				break;
			}
		} else {
			current->flags |= I2C_MSG_STOP;
		}

		current++;
	}

	if (ret) {
		return ret;
	}

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Set destination address with configured address mode before sending msg. */

	i2c_master_addr_mode_t addr_mode = 0;

	if (I2C_MSG_ADDR_10_BITS & data->dev_config) {
		addr_mode = I2C_MASTER_ADDR_MODE_10BIT;
	} else {
		addr_mode = I2C_MASTER_ADDR_MODE_7BIT;
	}

	R_IIC_MASTER_SlaveAddressSet(&data->ctrl, addr, addr_mode);

	/* Process input `msgs`. */

	current = msgs;

	while (num_msgs > 0) {
		if (num_msgs > 1) {
			next = current + 1;
		} else {
			next = NULL;
		}

		if (current->flags & I2C_MSG_READ) {
			fsp_err =
				R_IIC_MASTER_Read(&data->ctrl, current->buf, current->len,
						  next != NULL && (next->flags & I2C_MSG_RESTART));
		} else {
			fsp_err =
				R_IIC_MASTER_Write(&data->ctrl, current->buf, current->len,
						   next != NULL && (next->flags & I2C_MSG_RESTART));
		}

		if (fsp_err != FSP_SUCCESS) {
			switch (fsp_err) {
			case FSP_ERR_INVALID_SIZE:
				LOG_ERR("%s: Provided number of bytes more than uint16_t size "
					"(65535) while DTC is used for data transfer.",
					__func__);
				break;
			case FSP_ERR_IN_USE:
				LOG_ERR("%s: Bus busy condition. Another transfer was in progress.",
					__func__);
				break;
			default:
				/* Should not reach here. */
				LOG_ERR("%s: Unknown error. FSP_ERR=%d\n", __func__, fsp_err);
				break;
			}

			ret = -EIO;
			goto RELEASE_BUS;
		}

		/* Wait for callback to return. */
		k_sem_take(&data->complete_sem, K_FOREVER);

		/* Handle event msg from callback. */
		switch (data->event) {
		case I2C_MASTER_EVENT_ABORTED:
			LOG_ERR("%s: %s failed.", __func__,
				(current->flags & I2C_MSG_READ) ? "Read" : "Write");
			ret = -EIO;
			goto RELEASE_BUS;
		case I2C_MASTER_EVENT_RX_COMPLETE:
			break;
		case I2C_MASTER_EVENT_TX_COMPLETE:
			break;
		default:
			break;
		}

		current++;
		num_msgs--;
	}

RELEASE_BUS:
	k_mutex_unlock(&data->bus_mutex);

	return ret;
}

static void i2c_ra_iic_callback(i2c_master_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct i2c_ra_iic_data *data = dev->data;

	data->event = p_args->event;

	k_sem_give(&data->complete_sem);
}

static int i2c_ra_iic_init(const struct device *dev)
{
	const struct i2c_ra_iic_config *config = dev->config;
	struct i2c_ra_iic_data *data = (struct i2c_ra_iic_data *)dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("%s: pinctrl config failed.", __func__);
		return ret;
	}

	k_mutex_init(&data->bus_mutex);
	k_sem_init(&data->complete_sem, 0, 1);

	switch (data->fsp_config.rate) {
	case I2C_MASTER_RATE_STANDARD:
	case I2C_MASTER_RATE_FAST:
	case I2C_MASTER_RATE_FASTPLUS:
		calc_iic_master_clock_setting(dev, data->fsp_config.rate,
					      &data->iic_master_ext_cfg.clock_settings);
		data->iic_master_ext_cfg.timeout_mode = IIC_MASTER_TIMEOUT_MODE_SHORT;
		data->iic_master_ext_cfg.timeout_scl_low = IIC_MASTER_TIMEOUT_SCL_LOW_ENABLED;

		data->fsp_config.p_extend = &data->iic_master_ext_cfg;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate: %d", __func__, data->fsp_config.rate);
		return -ENOTSUP;
	}

	fsp_err = R_IIC_MASTER_Open(&data->ctrl, &data->fsp_config);
	__ASSERT(fsp_err == FSP_SUCCESS, "%s: Open iic master failed. FSP_ERR=%d", __func__,
		 fsp_err);

	config->irq_config_func(dev);

	return 0;
}

static void calc_iic_master_bitrate(const struct i2c_ra_iic_config *config, uint32_t total_brl_brh,
				    uint32_t brh, uint32_t divider,
				    struct ra_iic_master_bitrate *result)
{
	const uint32_t noise_filter_stage = config->noise_filter_stage;
	const double rise_time_s = config->rise_time_s;
	const double fall_time_s = config->fall_time_s;
	const uint32_t requested_duty = config->duty_cycle_percent;
	const uint32_t peripheral_clock = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB);
	uint32_t constant_add = 0;

	/* A constant is added to BRL and BRH in all formulas. This constand is 3 + nf
	 * when CKS == 0, or 2 + nf when CKS != 0.
	 */
	if (divider == 0) {
		constant_add = 3 + noise_filter_stage;
	} else {
		/* All dividers other than 0 use an addition of 2 + noise_filter_stages. */
		constant_add = 2 + noise_filter_stage;
	}

	/* Converts all divided numbers to double to avoid data loss. */
	uint32_t divided_pclk = (peripheral_clock >> divider);

	result->bitrate =
		1 / ((total_brl_brh + 2 * constant_add) / divided_pclk + rise_time_s + fall_time_s);
	result->duty =
		100 *
		((rise_time_s + ((brh + constant_add) / divided_pclk)) /
		 (rise_time_s + fall_time_s + ((total_brl_brh + 2 * constant_add)) / divided_pclk));
	result->divider = divider;
	result->brh = brh;
	result->brl = total_brl_brh - brh;
	result->duty_error_percent =
		(result->duty > requested_duty ? result->duty - requested_duty
					       : requested_duty - result->duty) /
		requested_duty;

	LOG_DBG("%s: [input] total_brl_brh[%d] brh[%d] divider[%d]"
		" [output] bitrate[%u] duty[%u] divider[%u] brh[%u] brl[%u] "
		"duty_error_percent[%u]\n",
		__func__, total_brl_brh, brh, divider, result->bitrate, result->duty,
		result->divider, result->brh, result->brl, result->duty_error_percent);
}

static void calc_iic_master_clock_setting(const struct device *dev, const uint32_t fsp_i2c_rate,
					  iic_master_clock_settings_t *clk_cfg)
{
	const struct i2c_ra_iic_config *config = dev->config;
	const uint32_t noise_filter_stage = config->noise_filter_stage;
	const double rise_time_s = config->rise_time_s;
	const double fall_time_s = config->fall_time_s;
	const uint32_t requested_duty = config->duty_cycle_percent;
	const uint32_t peripheral_clock = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB);

	uint32_t requested_bitrate = 0;

	switch (fsp_i2c_rate) {
	case I2C_MASTER_RATE_STANDARD:
	case I2C_MASTER_RATE_FAST:
	case I2C_MASTER_RATE_FASTPLUS:
		requested_bitrate = fsp_i2c_rate;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate: %d", __func__, fsp_i2c_rate);
		return;
	}

	/* Start with maximum possible bitrate. */
	uint32_t min_brh = noise_filter_stage + 1;
	uint32_t min_brl_brh = 2 * min_brh;
	struct ra_iic_master_bitrate bitrate = {};

	calc_iic_master_bitrate(config, min_brl_brh, min_brh, 0, &bitrate);

	/* Start with the smallest divider because it gives the most resolution. */
	uint32_t constant_add = 3 + noise_filter_stage;

	for (int temp_divider = 0; temp_divider <= 7; ++temp_divider) {
		if (1 == temp_divider) {
			/* All dividers other than 0 use an addition of 2 + noise_filter_stages.
			 */
			constant_add = 2 + noise_filter_stage;
		}

		/* If the requested bitrate cannot be achieved with this divider, continue.
		 */
		uint32_t divided_pclk = (peripheral_clock >> temp_divider);
		uint32_t total_brl_brh =
			ceil(((1 / (double)requested_bitrate) - (rise_time_s + fall_time_s)) *
				     divided_pclk -
			     (2 * constant_add));

		if ((total_brl_brh > 62) || (total_brl_brh < min_brl_brh)) {
			continue;
		}

		uint32_t temp_brh = total_brl_brh * requested_duty / 100;

		if (temp_brh < min_brh) {
			temp_brh = min_brh;
		}

		/* Calculate the actual bitrate and duty cycle. */
		struct ra_iic_master_bitrate temp_bitrate = {};

		calc_iic_master_bitrate(config, total_brl_brh, temp_brh, temp_divider,
					&temp_bitrate);

		/* Adjust duty cycle down if it helps. */
		struct ra_iic_master_bitrate test_bitrate = temp_bitrate;

		while (test_bitrate.duty > requested_duty) {
			temp_brh -= 1;

			if ((temp_brh < min_brh) || ((total_brl_brh - temp_brh) > 31)) {
				break;
			}

			struct ra_iic_master_bitrate new_bitrate = {};

			calc_iic_master_bitrate(config, total_brl_brh, temp_brh, temp_divider,
						&new_bitrate);

			if (new_bitrate.duty_error_percent < temp_bitrate.duty_error_percent) {
				temp_bitrate = new_bitrate;
			} else {
				break;
			}
		}

		/* Adjust duty cycle up if it helps. */
		while (test_bitrate.duty < requested_duty) {
			++temp_brh;

			if ((temp_brh > total_brl_brh) || (temp_brh > 31) ||
			    ((total_brl_brh - temp_brh) < min_brh)) {
				break;
			}

			struct ra_iic_master_bitrate new_bitrate = {};

			calc_iic_master_bitrate(config, total_brl_brh, temp_brh, temp_divider,
						&new_bitrate);

			if (new_bitrate.duty_error_percent < temp_bitrate.duty_error_percent) {
				temp_bitrate = new_bitrate;
			} else {
				break;
			}
		}

		if ((temp_bitrate.brh < 32) && (temp_bitrate.brl < 32)) {
			/* Valid setting found. */
			bitrate = temp_bitrate;
			break;
		}
	}

	clk_cfg->brl_value = bitrate.brl;
	clk_cfg->brh_value = bitrate.brh;
	clk_cfg->cks_value = bitrate.divider;

	LOG_DBG("%s: [input] rate[%u] [output] brl[%u] brh[%u] cks[%u]\n", __func__, fsp_i2c_rate,
		clk_cfg->brl_value, clk_cfg->brh_value, clk_cfg->cks_value);
}

static DEVICE_API(i2c, i2c_ra_iic_driver_api) = {
	.configure = i2c_ra_iic_configure,
	.get_config = i2c_ra_iic_get_config,
	.transfer = i2c_ra_iic_transfer,
};

#define EVENT_IIC_RXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _RXI))
#define EVENT_IIC_TXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _TXI))
#define EVENT_IIC_TEI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _TEI))
#define EVENT_IIC_ERI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _ERI))

#define I2C_RA_IIC_INIT(index)                                                                     \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static void i2c_ra_iic_irq_config_func##index(const struct device *dev)                    \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, rxi, irq)] =                               \
			EVENT_IIC_RXI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, txi, irq)] =                               \
			EVENT_IIC_TXI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, tei, irq)] =                               \
			EVENT_IIC_TEI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, eri, irq)] =                               \
			EVENT_IIC_ERI(DT_INST_PROP(index, channel));                               \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rxi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, rxi, priority), iic_master_rxi_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, txi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, txi, priority), iic_master_txi_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, tei, priority), iic_master_tei_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, eri, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, eri, priority), iic_master_eri_isr,         \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, rxi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, txi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, tei, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, eri, irq));                                  \
	}                                                                                          \
                                                                                                   \
	static const struct i2c_ra_iic_config i2c_ra_iic_config_##index = {                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_config_func = i2c_ra_iic_irq_config_func##index,                              \
		.noise_filter_stage = 1, /* Cannot be configured. */                               \
		.rise_time_s = DT_INST_PROP(index, rise_time_ns) / RA_IIC_MASTER_DIV_TIME_NS,      \
		.fall_time_s = DT_INST_PROP(index, fall_time_ns) / RA_IIC_MASTER_DIV_TIME_NS,      \
		.duty_cycle_percent = DT_INST_PROP(index, duty_cycle_percent),                     \
	};                                                                                         \
                                                                                                   \
	static struct i2c_ra_iic_data i2c_ra_iic_data_##index = {                                  \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.slave = 0,                                                        \
				.rate = DT_INST_PROP(index, clock_frequency),                      \
				.addr_mode = I2C_MASTER_ADDR_MODE_7BIT,                            \
				.ipl = DT_INST_PROP(index, interrupt_priority_level),              \
				.rxi_irq = DT_INST_IRQ_BY_NAME(index, rxi, irq),                   \
				.txi_irq = DT_INST_IRQ_BY_NAME(index, txi, irq),                   \
				.tei_irq = DT_INST_IRQ_BY_NAME(index, tei, irq),                   \
				.eri_irq = DT_INST_IRQ_BY_NAME(index, eri, irq),                   \
				.p_callback = i2c_ra_iic_callback,                                 \
				.p_context = (void *)DEVICE_DT_GET(DT_DRV_INST(index)),            \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_ra_iic_init, NULL, &i2c_ra_iic_data_##index,          \
				  &i2c_ra_iic_config_##index, POST_KERNEL,                         \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_ra_iic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RA_IIC_INIT)
