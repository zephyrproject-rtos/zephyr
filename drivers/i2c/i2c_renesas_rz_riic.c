/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_riic

#include <math.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>

#if defined(CONFIG_I2C_RENESAS_RZ_RIIC)
#include "r_riic_master.h"
#else /* CONFIG_I2C_RENESAS_RZ_IIC */
#include "r_iic_master.h"
typedef iic_master_extended_cfg_t riic_master_extended_cfg_t;
#define FSP_PRIV_CLOCK_P0CLK FSP_PRIV_CLOCK_PCLKL
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_rz_riic);

#define RZ_RIIC_MASTER_DIV_TIME_NS (1000000000.0)

struct i2c_rz_riic_config {
	const struct pinctrl_dev_config *pin_config;
	const i2c_master_api_t *fsp_api;
	double rise_time_s;
	double fall_time_s;
	double duty_cycle_percent;
	uint32_t noise_filter_stage;
};

struct i2c_rz_riic_data {
	i2c_master_ctrl_t *fsp_ctrl;
	i2c_master_cfg_t *fsp_cfg;
	riic_master_extended_cfg_t *riic_master_ext_cfg;
	struct k_mutex bus_mutex;
	struct k_sem complete_sem;
	i2c_master_event_t event;
	uint32_t dev_config;
};

/* IIC master clock setting calculation function. */
static void calc_riic_master_clock_setting(const struct device *dev, const uint32_t fsp_i2c_rate,
					   iic_master_clock_settings_t *clk_cfg);

#if defined(CONFIG_I2C_RENESAS_RZ_RIIC)
/* FSP interruption handlers. */
void riic_master_rxi_isr(void *irq);
void riic_master_txi_isr(void *irq);
void riic_master_tei_isr(void *irq);
void riic_master_naki_isr(void *irq);
void riic_master_sti_isr(void *irq);
void riic_master_spi_isr(void *irq);
void riic_master_ali_isr(void *irq);
void riic_master_tmoi_isr(void *irq);

static void i2c_rz_riic_master_rxi_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_rxi_isr((void *)data->fsp_cfg->rxi_irq);
}

static void i2c_rz_riic_master_txi_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_txi_isr((void *)data->fsp_cfg->txi_irq);
}

static void i2c_rz_riic_master_tei_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_tei_isr((void *)data->fsp_cfg->tei_irq);
}

static void i2c_rz_riic_master_naki_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_naki_isr((void *)data->riic_master_ext_cfg->naki_irq);
}

static void i2c_rz_riic_master_sti_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_sti_isr((void *)data->riic_master_ext_cfg->sti_irq);
}

static void i2c_rz_riic_master_spi_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_spi_isr((void *)data->riic_master_ext_cfg->spi_irq);
}

static void i2c_rz_riic_master_ali_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_ali_isr((void *)data->riic_master_ext_cfg->ali_irq);
}

static void i2c_rz_riic_master_tmoi_isr(const struct device *dev)
{
	struct i2c_rz_riic_data *data = dev->data;

	riic_master_tmoi_isr((void *)data->riic_master_ext_cfg->tmoi_irq);
}

#elif defined(CONFIG_I2C_RENESAS_RZ_IIC)
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);
#endif

struct rz_riic_master_bitrate {
	uint32_t bitrate;
	uint32_t duty;
	uint32_t divider;
	uint32_t brl;
	uint32_t brh;
	double duty_error_percent;
};

static int i2c_rz_riic_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_rz_riic_config *config = dev->config;
	struct i2c_rz_riic_data *data = dev->data;
	fsp_err_t err;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Only I2C Master mode supported.");
		return -EIO;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->fsp_cfg->rate = I2C_MASTER_RATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		data->fsp_cfg->rate = I2C_MASTER_RATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->fsp_cfg->rate = I2C_MASTER_RATE_FASTPLUS;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate flag: %d", __func__, I2C_SPEED_GET(dev_config));
		return -EIO;
	}

	/* Recalc clock setting after updating config. */
	calc_riic_master_clock_setting(dev, data->fsp_cfg->rate,
				       &data->riic_master_ext_cfg->clock_settings);

	err = config->fsp_api->close(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to configure I2C device");
		return -EIO;
	}

	err = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to configure I2C device");
		return -EIO;
	}

	/* save current devconfig. */
	data->dev_config = dev_config;

	return 0;
}

static int i2c_rz_riic_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_rz_riic_data *data = (struct i2c_rz_riic_data *const)dev->data;
	*dev_config = data->dev_config;

	return 0;
}

#define OPERATION(msg) (((struct i2c_msg *)msg)->flags & I2C_MSG_RW_MASK)

static int i2c_rz_riic_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr)
{
	const struct i2c_rz_riic_config *config = dev->config;
	struct i2c_rz_riic_data *data = (struct i2c_rz_riic_data *const)dev->data;
	struct i2c_msg *current, *next;
	fsp_err_t err;
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

	err = config->fsp_api->slaveAddressSet(data->fsp_ctrl, addr, addr_mode);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to set slave address");
		return -EIO;
	}

	/* Process input `msgs`. */

	current = msgs;

	while (num_msgs > 0) {
		if (num_msgs > 1) {
			next = current + 1;
		} else {
			next = NULL;
		}

		if (current->flags & I2C_MSG_READ) {
			err = config->fsp_api->read(data->fsp_ctrl, current->buf, current->len,
						    next != NULL &&
							    (next->flags & I2C_MSG_RESTART));
		} else {
			err = config->fsp_api->write(data->fsp_ctrl, current->buf, current->len,
						     next != NULL &&
							     (next->flags & I2C_MSG_RESTART));
		}

		if (err != FSP_SUCCESS) {
			switch (err) {
			case FSP_ERR_IN_USE:
				LOG_ERR("%s: Bus busy condition. Another transfer was in progress.",
					__func__);
				break;
			default:
				/* Should not reach here. */
				LOG_ERR("%s: Unknown error. FSP_ERR=%d\n", __func__, err);
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

static void i2c_rz_riic_callback(i2c_master_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct i2c_rz_riic_data *data = dev->data;

	data->event = p_args->event;

	k_sem_give(&data->complete_sem);
}

static int i2c_rz_riic_init(const struct device *dev)
{
	const struct i2c_rz_riic_config *config = dev->config;
	struct i2c_rz_riic_data *data = dev->data;
	fsp_err_t err;
	int ret = 0;

	/* Configure dt provided device signals when available */
	if (config->pin_config->state_cnt > 0) {
		ret = pinctrl_apply_state(config->pin_config, PINCTRL_STATE_DEFAULT);

		if (ret < 0) {
			LOG_ERR("%s: pinctrl config failed.", __func__);
			return ret;
		}
	}

	k_mutex_init(&data->bus_mutex);
	k_sem_init(&data->complete_sem, 0, 1);

	switch (data->fsp_cfg->rate) {
	case I2C_MASTER_RATE_STANDARD:
	case I2C_MASTER_RATE_FAST:
	case I2C_MASTER_RATE_FASTPLUS:
		calc_riic_master_clock_setting(dev, data->fsp_cfg->rate,
					       &data->riic_master_ext_cfg->clock_settings);
		data->riic_master_ext_cfg->timeout_mode = IIC_MASTER_TIMEOUT_MODE_SHORT;
		data->riic_master_ext_cfg->timeout_scl_low = IIC_MASTER_TIMEOUT_SCL_LOW_ENABLED;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate: %d", __func__, data->fsp_cfg->rate);
		return -ENOTSUP;
	}

	err = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	if (err != FSP_SUCCESS) {
		LOG_ERR("I2C initialization failed");
		return -EIO;
	}

	return 0;
}

static void calc_riic_master_bitrate(const struct i2c_rz_riic_config *config,
				     uint32_t total_brl_brh, uint32_t brh, uint32_t divider,
				     struct rz_riic_master_bitrate *result)
{
	const uint32_t noise_filter_stages = config->noise_filter_stage;
	const double rise_time_s = config->rise_time_s;
	const double fall_time_s = config->fall_time_s;
	const double requested_duty = config->duty_cycle_percent;
	const uint32_t peripheral_clock = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_P0CLK);
	uint32_t constant_add = 0;

	/* A constant is added to BRL and BRH in all formulas. This constand is 3 + nf when CKS ==
	 * 0, or 2 + nf when CKS != 0.
	 */
	if (divider == 0) {
		constant_add = 3 + noise_filter_stages;
	} else {
		/* All dividers other than 0 use an addition of 2 + noise_filter_stages. */
		constant_add = 2 + noise_filter_stages;
	}

	/* Converts all divided numbers to double to avoid data loss. */
	double divided_p0 = (peripheral_clock >> divider);

	result->bitrate =
		1 / ((total_brl_brh + 2 * constant_add) / divided_p0 + rise_time_s + fall_time_s);
	result->duty =
		100 *
		((rise_time_s + ((brh + constant_add) / divided_p0)) /
		 (rise_time_s + fall_time_s + ((total_brl_brh + 2 * constant_add)) / divided_p0));
	result->divider = divider;
	result->brh = brh;
	result->brl = total_brl_brh - brh;
	result->duty_error_percent =
		(result->duty > requested_duty ? (result->duty - requested_duty)
					       : (requested_duty - result->duty)) /
		requested_duty;
}

static void calc_riic_master_clock_setting(const struct device *dev, const uint32_t fsp_i2c_rate,
					   iic_master_clock_settings_t *clk_cfg)
{
	const struct i2c_rz_riic_config *config = dev->config;
	const uint32_t noise_filter_stages = config->noise_filter_stage;
	const double rise_time_s = config->rise_time_s;
	const double fall_time_s = config->fall_time_s;
	const uint32_t requested_duty = config->duty_cycle_percent;
	const uint32_t peripheral_clock = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_P0CLK);

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
	uint32_t min_brh = noise_filter_stages + 1;
	uint32_t min_brl_brh = 2 * min_brh;
	struct rz_riic_master_bitrate bitrate = {};

	calc_riic_master_bitrate(config, min_brl_brh, min_brh, 0, &bitrate);

	/* Start with the smallest divider because it gives the most resolution. */
	uint32_t constant_add = 3 + noise_filter_stages;

	for (int temp_divider = 0; temp_divider <= 7; ++temp_divider) {
		if (1 == temp_divider) {
			/* All dividers other than 0 use an addition of 2 + noise_filter_stages. */
			constant_add = 2 + noise_filter_stages;
		}

		/* If the requested bitrate cannot be achieved with this divider, continue. */
		double divided_p0 = (peripheral_clock >> temp_divider);
		uint32_t total_brl_brh =
			ceil(((1 / (double)requested_bitrate) - (rise_time_s + fall_time_s)) *
				     divided_p0 -
			     (2 * constant_add));

		if ((total_brl_brh > 62) || (total_brl_brh < min_brl_brh)) {
			continue;
		}

		uint32_t temp_brh = total_brl_brh * requested_duty / 100;

		if (temp_brh < min_brh) {
			temp_brh = min_brh;
		}

		/* Calculate the actual bitrate and duty cycle. */
		struct rz_riic_master_bitrate temp_bitrate = {};

		calc_riic_master_bitrate(config, total_brl_brh, temp_brh, temp_divider,
					 &temp_bitrate);

		/* Adjust duty cycle down if it helps. */
		struct rz_riic_master_bitrate test_bitrate = temp_bitrate;

		while (test_bitrate.duty > requested_duty) {
			temp_brh -= 1;

			if ((temp_brh < min_brh) || ((total_brl_brh - temp_brh) > 31)) {
				break;
			}

			struct rz_riic_master_bitrate new_bitrate = {};

			calc_riic_master_bitrate(config, total_brl_brh, temp_brh, temp_divider,
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

			struct rz_riic_master_bitrate new_bitrate = {};

			calc_riic_master_bitrate(config, total_brl_brh, temp_brh, temp_divider,
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
}

static DEVICE_API(i2c, i2c_rz_riic_driver_api) = {
	.configure = i2c_rz_riic_configure,
	.get_config = i2c_rz_riic_get_config,
	.transfer = i2c_rz_riic_transfer,
};

#ifdef CONFIG_CPU_CORTEX_M
#define GET_IRQ_FLAGS(index, irq_name) 0
#else /* Cortex-A/R */
#define GET_IRQ_FLAGS(index, irq_name) DT_INST_IRQ_BY_NAME(index, irq_name, flags)
#endif

#define I2C_RZ_IRQ_CONNECT(index, irq_name, isr)                                                   \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, irq_name, irq),                             \
			    DT_INST_IRQ_BY_NAME(index, irq_name, priority), isr,                   \
			    DEVICE_DT_INST_GET(index), GET_IRQ_FLAGS(index, irq_name));            \
		irq_enable(DT_INST_IRQ_BY_NAME(index, irq_name, irq));                             \
	} while (0)

#if defined(CONFIG_I2C_RENESAS_RZ_RIIC)
#define I2C_RZ_CONFIG_FUNC(index)                                                                  \
	I2C_RZ_IRQ_CONNECT(index, rxi, i2c_rz_riic_master_rxi_isr);                                \
	I2C_RZ_IRQ_CONNECT(index, txi, i2c_rz_riic_master_txi_isr);                                \
	I2C_RZ_IRQ_CONNECT(index, tei, i2c_rz_riic_master_tei_isr);                                \
	I2C_RZ_IRQ_CONNECT(index, naki, i2c_rz_riic_master_naki_isr);                              \
	I2C_RZ_IRQ_CONNECT(index, sti, i2c_rz_riic_master_sti_isr);                                \
	I2C_RZ_IRQ_CONNECT(index, spi, i2c_rz_riic_master_spi_isr);                                \
	I2C_RZ_IRQ_CONNECT(index, ali, i2c_rz_riic_master_ali_isr);                                \
	I2C_RZ_IRQ_CONNECT(index, tmoi, i2c_rz_riic_master_tmoi_isr);

#define I2C_RZ_EXTENDED_CFG(index)                                                                 \
	static riic_master_extended_cfg_t g_i2c_master##index##_extend = {                         \
		.noise_filter_stage = DT_INST_PROP(index, noise_filter_stages),                    \
		.naki_irq = DT_INST_IRQ_BY_NAME(index, naki, irq),                                 \
		.sti_irq = DT_INST_IRQ_BY_NAME(index, sti, irq),                                   \
		.spi_irq = DT_INST_IRQ_BY_NAME(index, spi, irq),                                   \
		.ali_irq = DT_INST_IRQ_BY_NAME(index, ali, irq),                                   \
		.tmoi_irq = DT_INST_IRQ_BY_NAME(index, tmoi, irq),                                 \
	};
#endif /* CONFIG_I2C_RENESAS_RZ_RIIC */

#if defined(CONFIG_I2C_RENESAS_RZ_IIC)
#define I2C_RZ_CONFIG_FUNC(index)                                                                  \
	I2C_RZ_IRQ_CONNECT(index, eri, iic_master_eri_isr);                                        \
	I2C_RZ_IRQ_CONNECT(index, rxi, iic_master_rxi_isr);                                        \
	I2C_RZ_IRQ_CONNECT(index, txi, iic_master_txi_isr);                                        \
	I2C_RZ_IRQ_CONNECT(index, tei, iic_master_tei_isr);

#define I2C_RZ_EXTENDED_CFG(index)                                                                 \
	static riic_master_extended_cfg_t g_i2c_master##index##_extend = {};
#endif /* CONFIG_I2C_RENESAS_RZ_IIC */

#define I2C_RZ_RIIC_INIT(index)                                                                    \
                                                                                                   \
	I2C_RZ_EXTENDED_CFG(index)                                                                 \
                                                                                                   \
	static i2c_master_cfg_t g_i2c_master##index##_cfg = {                                      \
		.channel = DT_INST_PROP(index, channel),                                           \
		.rate = DT_INST_PROP(index, clock_frequency),                                      \
		.slave = 0x00,                                                                     \
		.addr_mode = I2C_MASTER_ADDR_MODE_7BIT,                                            \
		.p_transfer_tx = NULL,                                                             \
		.p_transfer_rx = NULL,                                                             \
		.p_callback = i2c_rz_riic_callback,                                                \
		.p_context = DEVICE_DT_GET(DT_DRV_INST(index)),                                    \
		.rxi_irq = DT_INST_IRQ_BY_NAME(index, rxi, irq),                                   \
		.txi_irq = DT_INST_IRQ_BY_NAME(index, txi, irq),                                   \
		.tei_irq = DT_INST_IRQ_BY_NAME(index, tei, irq),                                   \
		.ipl = DT_INST_IRQ_BY_NAME(index, rxi, priority),                                  \
		.p_extend = &g_i2c_master##index##_extend,                                         \
		IF_ENABLED(CONFIG_I2C_RENESAS_RZ_IIC,                                              \
				(.eri_irq = DT_INST_IRQ_BY_NAME(index, eri, irq),))};       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct i2c_rz_riic_config i2c_rz_riic_config_##index = {                      \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                               \
		.fsp_api = &g_i2c_master_on_iic,                                                   \
		.rise_time_s = DT_INST_PROP(index, rise_time_ns) / RZ_RIIC_MASTER_DIV_TIME_NS,     \
		.fall_time_s = DT_INST_PROP(index, fall_time_ns) / RZ_RIIC_MASTER_DIV_TIME_NS,     \
		.duty_cycle_percent = DT_INST_PROP(index, duty_cycle_percent),                     \
		.noise_filter_stage = DT_INST_PROP(index, noise_filter_stages),                    \
	};                                                                                         \
                                                                                                   \
	static iic_master_instance_ctrl_t g_i2c_master##index##_ctrl;                              \
                                                                                                   \
	static struct i2c_rz_riic_data i2c_rz_riic_data_##index = {                                \
		.fsp_ctrl = (i2c_master_ctrl_t *)&g_i2c_master##index##_ctrl,                      \
		.fsp_cfg = &g_i2c_master##index##_cfg,                                             \
		.riic_master_ext_cfg = &g_i2c_master##index##_extend,                              \
	};                                                                                         \
                                                                                                   \
	static int i2c_rz_riic_init_##index(const struct device *dev)                              \
	{                                                                                          \
		I2C_RZ_CONFIG_FUNC(index)                                                          \
		return i2c_rz_riic_init(dev);                                                      \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_rz_riic_init_##index, NULL,                           \
				  &i2c_rz_riic_data_##index, &i2c_rz_riic_config_##index,          \
				  PRE_KERNEL_2, CONFIG_I2C_INIT_PRIORITY,                          \
				  &i2c_rz_riic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RZ_RIIC_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_rz_iic

DT_INST_FOREACH_STATUS_OKAY(I2C_RZ_RIIC_INIT)
