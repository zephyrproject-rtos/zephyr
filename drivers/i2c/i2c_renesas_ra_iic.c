/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_iic

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>

#include <r_iic_master.h>
#include <r_iic_slave.h>
#include <errno.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_ra_iic, CONFIG_I2C_LOG_LEVEL);

#define OPERATION(msg) (((struct i2c_msg *)msg)->flags & I2C_MSG_RW_MASK)

struct i2c_ra_iic_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	uint32_t ctrl_rise_time_ns;
	uint32_t ctrl_fall_time_ns;
	uint32_t ctrl_noise_filter_stage;
	uint32_t ctrl_duty_cycle_percent;
	uint32_t max_bitrate_supported;
};

struct i2c_ra_iic_data {
	iic_master_instance_ctrl_t control_ctrl;
	i2c_master_cfg_t ctrl_fconfig;
	i2c_master_event_t ctrl_event;
	iic_master_extended_cfg_t iic_ctrl_ext_cfg;
#ifdef CONFIG_I2C_TARGET
	iic_slave_instance_ctrl_t target_ctrl;
	i2c_slave_cfg_t target_fconfig;
	iic_slave_extended_cfg_t iic_target_ext_cfg;
	struct i2c_target_config *target_cfg;
	bool transaction_completed;
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	uint8_t target_buf[CONFIG_I2C_TARGET_RENESAS_RA_IIC_BUFFER_SIZE];
#else
	uint8_t target_buf;
#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */
#endif /* CONFIG_I2C_TARGET */
	struct k_mutex bus_mutex;
	struct k_sem complete_sem;
	uint32_t dev_config;
};

/* IIC clock setting calculation function. */
static int calc_iic_ctrl_clock_setting(const struct device *dev, const uint32_t ctrl_rate,
				       iic_master_clock_settings_t *clk_cfg);
#ifdef CONFIG_I2C_TARGET
static int calc_iic_target_clock_setting(const struct device *dev, const uint32_t slave_rate,
					 iic_slave_clock_settings_t *clk_cfg);
#endif /* CONFIG_I2C_TARGET */

/* FSP interruption handlers. */
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);

#ifdef CONFIG_I2C_TARGET
void iic_slave_rxi_isr(void);
void iic_slave_txi_isr(void);
void iic_slave_tei_isr(void);
void iic_slave_eri_isr(void);
#endif /* CONFIG_I2C_TARGET */

struct ra_iic_ctrl_bitrate {
	uint32_t bitrate;
	uint32_t duty;
	uint32_t divider;
	uint32_t brl;
	uint32_t brh;
	uint32_t duty_error_percent;
};

static int i2c_ra_iic_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_ra_iic_config *config = dev->config;
	struct i2c_ra_iic_data *data = (struct i2c_ra_iic_data *const)dev->data;
	uint32_t desire_bitrate;
	fsp_err_t fsp_err;
	int err;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Please configure I2C in Controller mode, target should be registered via "
			"i2c_target_register API");
		return -EIO;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		desire_bitrate = I2C_MASTER_RATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		desire_bitrate = I2C_MASTER_RATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		desire_bitrate = I2C_MASTER_RATE_FASTPLUS;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate flag: %d", __func__, I2C_SPEED_GET(dev_config));
		return -EIO;
	}

	if (desire_bitrate <= config->max_bitrate_supported) {
		data->ctrl_fconfig.rate = desire_bitrate;
	} else {
		LOG_ERR("%s: Requested bitrate %u exceeds max supported bitrate %u", __func__,
			desire_bitrate, config->max_bitrate_supported);
		return -EIO;
	}

	/* Recalc clock setting after updating config. */
	err = calc_iic_ctrl_clock_setting(dev, data->ctrl_fconfig.rate,
					  &data->iic_ctrl_ext_cfg.clock_settings);
	if (err != 0) {
		LOG_ERR("Failed to calculate I2C clock settings");
		return err;
	}

	fsp_err = R_IIC_MASTER_Close(&data->control_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to close I2C master instance. FSP_ERR=%d", fsp_err);
		return -EIO;
	}
	fsp_err = R_IIC_MASTER_Open(&data->control_ctrl, &data->ctrl_fconfig);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to open I2C master instance. FSP_ERR=%d", fsp_err);
		return -EIO;
	}

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
						"different directions is required. Current/Total: "
						"[%d/%d]",
						__func__, i, num_msgs);
					ret = -EIO;
					break;
				}
			}

			/* Stop condition is only allowed on last message */
			if (current->flags & I2C_MSG_STOP) {
				LOG_ERR("%s: Invalid stop flag. Stop condition is only allowed on "
					"last message. Current/Total: [%d/%d]",
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

	R_IIC_MASTER_SlaveAddressSet(&data->control_ctrl, addr, addr_mode);

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
				R_IIC_MASTER_Read(&data->control_ctrl, current->buf, current->len,
						  next != NULL && (next->flags & I2C_MSG_RESTART));
		} else {
			fsp_err =
				R_IIC_MASTER_Write(&data->control_ctrl, current->buf, current->len,
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
		switch (data->ctrl_event) {
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

static void i2c_ra_iic_ctrl_callback(i2c_master_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct i2c_ra_iic_data *data = dev->data;

	data->ctrl_event = p_args->event;

	k_sem_give(&data->complete_sem);
}

#ifdef CONFIG_I2C_TARGET

static void i2c_ra_iic_target_callback(i2c_slave_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct i2c_ra_iic_data *data = dev->data;
	const struct i2c_target_callbacks *target_cb = data->target_cfg->callbacks;
	fsp_err_t fsp_err;
	int err;
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	uint8_t *buf = NULL;
	uint32_t len;
#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */

	switch (p_args->event) {

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	case I2C_SLAVE_EVENT_RX_COMPLETE:
		if (p_args->bytes > 0) {
			target_cb->buf_write_received(data->target_cfg, data->target_buf,
						      p_args->bytes);
		}
		__fallthrough;
	case I2C_SLAVE_EVENT_TX_COMPLETE:
		if (data->transaction_completed) {
			target_cb->stop(data->target_cfg);
		}
		break;
	case I2C_SLAVE_EVENT_RX_REQUEST:
		err = target_cb->write_requested(data->target_cfg);
		if (err == 0) {
			fsp_err = R_IIC_SLAVE_Read(&data->target_ctrl, data->target_buf,
						   sizeof(data->target_buf));
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
		} else {
			fsp_err = R_IIC_SLAVE_Read(&data->target_ctrl, data->target_buf, 0);
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
			LOG_DBG("I2C target does not want to receive data."
				"Send a NACK to Controller device to terminate the transaction.");
		}
		data->transaction_completed = false;
		break;
	case I2C_SLAVE_EVENT_TX_REQUEST:
		err = target_cb->buf_read_requested(data->target_cfg, &buf, &len);
		if (err == 0) {
			if (buf != NULL && len != 0) {
				fsp_err = R_IIC_SLAVE_Write(&data->target_ctrl, buf, len);
				__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
			} else {
				LOG_ERR("buf is NULL or len is 0, Controller device will read 0xFF "
					"for the remaining bytes");
			}
		} else {
			LOG_DBG("I2C target doesn't provide new data. The I2C bus will be left "
				"floating, Controller device will read the value 0xFF for the "
				"remaining bytes.");
		}
		data->transaction_completed = false;
		break;
	case I2C_SLAVE_EVENT_RX_MORE_REQUEST:
		fsp_err = R_IIC_SLAVE_Read(&data->target_ctrl, data->target_buf, 0);
		__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
		LOG_ERR("The buffer is full, target device cannot receive more data. Please "
			"increase I2C_TARGET_RENESAS_RA_IIC_BUFFER_SIZE");
		target_cb->stop(data->target_cfg);
		break;
	case I2C_SLAVE_EVENT_TX_MORE_REQUEST:
		LOG_ERR("Out of data to send to the controller device, Controller device will read "
			"0xFF for the remaining bytes");
		break;
#else  /* !CONFIG_I2C_TARGET_BUFFER_MODE */

	case I2C_SLAVE_EVENT_RX_COMPLETE:
		if (p_args->bytes > 0) {
			target_cb->write_received(data->target_cfg, data->target_buf);
		}
		__fallthrough;
	case I2C_SLAVE_EVENT_TX_COMPLETE:
		if (data->transaction_completed) {
			target_cb->stop(data->target_cfg);
		}
		break;
	case I2C_SLAVE_EVENT_RX_REQUEST:
		err = target_cb->write_requested(data->target_cfg);
		if (err == 0) {
			/* Continue to receive data */
			fsp_err = R_IIC_SLAVE_Read(&data->target_ctrl, &data->target_buf, 1);
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
		} else {
			/* NACK the received data */
			fsp_err = R_IIC_SLAVE_Read(&data->target_ctrl, &data->target_buf, 0);
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
			LOG_DBG("I2C target does not want to receive data."
				"Send a NACK to Controller device to terminate the transaction.");
		}
		data->transaction_completed = false;
		break;
	case I2C_SLAVE_EVENT_TX_REQUEST:
		err = target_cb->read_requested(data->target_cfg, &data->target_buf);
		if (err == 0) {
			fsp_err = R_IIC_SLAVE_Write(&data->target_ctrl, &data->target_buf, 1);
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
		} else {
			LOG_DBG("I2C target doesn't provide new data. The I2C bus will be left "
				"floating, Controller device will read the value 0xFF for the "
				"remaining bytes.");
		}
		data->transaction_completed = false;
		break;
	case I2C_SLAVE_EVENT_RX_MORE_REQUEST:
		err = target_cb->write_received(data->target_cfg, data->target_buf);
		if (err == 0) {
			/* Continue to receive data */
			fsp_err = R_IIC_SLAVE_Read(&data->target_ctrl, &data->target_buf, 1);
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
		} else {
			/* NACK the received data */
			fsp_err = R_IIC_SLAVE_Read(&data->target_ctrl, &data->target_buf, 0);
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
			LOG_DBG("I2C target does not want to receive data."
				"Send a NACK to Controller device to terminate the transaction.");
		}
		break;
	case I2C_SLAVE_EVENT_TX_MORE_REQUEST:
		err = target_cb->read_processed(data->target_cfg, &data->target_buf);
		if (err == 0) {
			fsp_err = R_IIC_SLAVE_Write(&data->target_ctrl, &data->target_buf, 1);
			__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
		} else {
			LOG_DBG("I2C target doesn't provide new data. The I2C bus will be left "
				"floating, Controller device will read the value 0xFF for the "
				"remaining bytes.");
		}
		break;
#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */
	case I2C_SLAVE_EVENT_ABORTED:
	case I2C_SLAVE_EVENT_GENERAL_CALL:
	default:
		break;
	}
}
#endif /* CONFIG_I2C_TARGET */

static int i2c_ra_iic_init(const struct device *dev)
{
	const struct i2c_ra_iic_config *config = dev->config;
	struct i2c_ra_iic_data *data = (struct i2c_ra_iic_data *)dev->data;
	fsp_err_t fsp_err;
	int ret = 0;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("%s: pinctrl config failed.", __func__);
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->bus_mutex);
	k_sem_init(&data->complete_sem, 0, 1);

	switch (data->ctrl_fconfig.rate) {
	case I2C_MASTER_RATE_STANDARD:
	case I2C_MASTER_RATE_FAST:
	case I2C_MASTER_RATE_FASTPLUS:
		ret = calc_iic_ctrl_clock_setting(dev, data->ctrl_fconfig.rate,
						  &data->iic_ctrl_ext_cfg.clock_settings);
		if (ret != 0) {
			LOG_ERR("Failed to calculate I2C clock settings");
			break;
		}
		data->iic_ctrl_ext_cfg.timeout_mode = IIC_MASTER_TIMEOUT_MODE_SHORT;
		data->iic_ctrl_ext_cfg.timeout_scl_low = IIC_MASTER_TIMEOUT_SCL_LOW_ENABLED;

		data->ctrl_fconfig.p_extend = &data->iic_ctrl_ext_cfg;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate: %d", __func__, data->ctrl_fconfig.rate);
		return -ENOTSUP;
	}

	if (ret != 0) {
		return ret;
	}

	fsp_err = R_IIC_MASTER_Open(&data->control_ctrl, &data->ctrl_fconfig);
	__ASSERT(fsp_err == FSP_SUCCESS, "%s: Open iic master failed. FSP_ERR=%d", __func__,
		 fsp_err);

#ifdef CONFIG_I2C_TARGET
	data->target_fconfig.p_extend = &data->iic_target_ext_cfg;
#endif /* CONFIG_I2C_TARGET */

	return 0;
}

static void calc_iic_ctrl_bitrate(const struct i2c_ra_iic_config *config, uint32_t total_brl_brh,
				  uint32_t brh, uint32_t divider, uint32_t peripheral_clock,
				  struct ra_iic_ctrl_bitrate *result)
{
	const uint32_t requested_duty = config->ctrl_duty_cycle_percent;
	uint32_t constant_add, divided_pclk, clock_edge, clock_rise_edge;

	/* A constant is added to BRL and BRH in all formulas. This constand is 3 + nf
	 * when CKS == 0, or 2 + nf when CKS != 0.
	 */
	if (divider == 0) {
		constant_add = 3 + config->ctrl_noise_filter_stage;
	} else {
		/* All dividers other than 0 use an addition of 2 + ctrl_noise_filter_stages. */
		constant_add = 2 + config->ctrl_noise_filter_stage;
	}

	divided_pclk = (peripheral_clock >> divider);

	clock_edge = (uint32_t)((uint64_t)((uint64_t)(divided_pclk) * (config->ctrl_rise_time_ns +
								       config->ctrl_fall_time_ns)) /
				1000000000);
	clock_rise_edge =
		(uint32_t)((uint64_t)((uint64_t)(divided_pclk)*config->ctrl_rise_time_ns) /
			   1000000000);

	result->bitrate = divided_pclk / (total_brl_brh + 2 * constant_add + clock_edge);
	result->duty = (clock_rise_edge + brh + constant_add) /
		       (clock_edge + total_brl_brh + 2 * constant_add) * 100;
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

static int calc_iic_ctrl_clock_setting(const struct device *dev, const uint32_t ctrl_rate,
				       iic_master_clock_settings_t *clk_cfg)
{
	const struct i2c_ra_iic_config *config = dev->config;
	const uint32_t ctrl_noise_filter_stage = config->ctrl_noise_filter_stage;
	const uint32_t requested_duty = config->ctrl_duty_cycle_percent;
	struct ra_iic_ctrl_bitrate bitrate = {};
	uint32_t peripheral_clock;
	uint32_t min_brh, min_brl_brh, total_brl_brh;
	uint32_t divided_pclk, constant_add, requested_bitrate;
	int ret;

	switch (ctrl_rate) {
	case I2C_MASTER_RATE_STANDARD:
	case I2C_MASTER_RATE_FAST:
	case I2C_MASTER_RATE_FASTPLUS:
		requested_bitrate = ctrl_rate;
		break;
	default:
		LOG_ERR("%s: Invalid I2C speed rate: %d", __func__, ctrl_rate);
		return -EINVAL;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)&config->clock_subsys,
				     &peripheral_clock);
	if (ret != 0) {
		return ret;
	}

	/* Start with maximum possible bitrate. */
	min_brh = ctrl_noise_filter_stage + 1;
	min_brl_brh = 2 * min_brh;

	calc_iic_ctrl_bitrate(config, min_brl_brh, min_brh, 0, peripheral_clock, &bitrate);

	/* Start with the smallest divider because it gives the most resolution. */
	constant_add = 3 + ctrl_noise_filter_stage;

	for (uint32_t temp_divider = 0; temp_divider <= 7; ++temp_divider) {
		struct ra_iic_ctrl_bitrate temp_bitrate = {};
		uint32_t temp_brh, clock_edge;

		if (1 == temp_divider) {
			/* All dividers other than 0 use an addition of 2 +
			 * ctrl_noise_filter_stages.
			 */
			constant_add = 2 + ctrl_noise_filter_stage;
		}

		/* If the requested bitrate cannot be achieved with this divider, continue.
		 */
		divided_pclk = (peripheral_clock >> temp_divider);

		clock_edge = (uint32_t)((uint64_t)((uint64_t)(divided_pclk) *
						   (config->ctrl_rise_time_ns +
						    config->ctrl_fall_time_ns)) /
					1000000000);

		total_brl_brh = DIV_ROUND_UP(divided_pclk, requested_bitrate) - clock_edge -
				(2 * constant_add);

		if ((total_brl_brh > 62) || (total_brl_brh < min_brl_brh)) {
			continue;
		}

		temp_brh = total_brl_brh * requested_duty / 100;

		if (temp_brh < min_brh) {
			temp_brh = min_brh;
		}

		/* Calculate the actual bitrate and duty cycle. */
		calc_iic_ctrl_bitrate(config, total_brl_brh, temp_brh, temp_divider,
				      peripheral_clock, &temp_bitrate);

		/* Adjust duty cycle down if it helps. */
		while (temp_bitrate.duty > requested_duty) {
			struct ra_iic_ctrl_bitrate new_bitrate = {};

			temp_brh--;

			if ((temp_brh < min_brh) || ((total_brl_brh - temp_brh) > 31)) {
				break;
			}

			calc_iic_ctrl_bitrate(config, total_brl_brh, temp_brh, temp_divider,
					      peripheral_clock, &new_bitrate);

			if (new_bitrate.duty_error_percent < temp_bitrate.duty_error_percent) {
				temp_bitrate = new_bitrate;
			} else {
				break;
			}
		}

		/* Adjust duty cycle up if it helps. */
		while (temp_bitrate.duty < requested_duty) {
			struct ra_iic_ctrl_bitrate new_bitrate = {};

			temp_brh++;

			if ((temp_brh > total_brl_brh) || (temp_brh > 31) ||
			    ((total_brl_brh - temp_brh) < min_brh)) {
				break;
			}

			calc_iic_ctrl_bitrate(config, total_brl_brh, temp_brh, temp_divider,
					      peripheral_clock, &new_bitrate);

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

	LOG_DBG("%s: [input] rate[%u] [output] brl[%u] brh[%u] cks[%u]\n", __func__, ctrl_rate,
		clk_cfg->brl_value, clk_cfg->brh_value, clk_cfg->cks_value);

	return 0;
}

#ifdef CONFIG_I2C_TARGET
static int calc_iic_target_clock_setting(const struct device *dev, const uint32_t slave_rate,
					 iic_slave_clock_settings_t *clk_cfg)
{
	const struct i2c_ra_iic_config *config = dev->config;
	struct i2c_ra_iic_data *data = dev->data;
	uint32_t requested_delay_ns;
	uint32_t peripheral_clock;
	uint32_t min_brl, brl;
	uint64_t brl_ns;
	int ret;

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)&config->clock_subsys,
				     &peripheral_clock);
	if (ret != 0) {
		return ret;
	}

	if (slave_rate == I2C_SLAVE_RATE_FASTPLUS) {
		requested_delay_ns = 50;
	} else if (slave_rate == I2C_SLAVE_RATE_FAST) {
		requested_delay_ns = 100;
	} else {
		requested_delay_ns = 250;
	}

	min_brl = data->iic_target_ext_cfg.clock_settings.digital_filter_stages + 1;
	brl_ns = (uint64_t)(peripheral_clock * requested_delay_ns);
	brl = (uint32_t)DIV_ROUND_UP(brl_ns, 1000000000);

	if (brl < min_brl) {
		brl = min_brl;
	}

	clk_cfg->brl_value = brl;

	return 0;
}

static int i2c_ra_iic_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_ra_iic_data *data = dev->data;
	fsp_err_t fsp_err;
	int ret;

	if (!cfg) {
		return -EINVAL;
	}

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	if (data->control_ctrl.open == 0) {
		LOG_ERR("%s: I2C Controller instance is not opened.", __func__);
		ret = -EIO;
		goto RELEASE_BUS;
	}

	fsp_err = R_IIC_MASTER_Close(&data->control_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("%s: Failed to close I2C Controller instance. FSP_ERR=%d", __func__,
			fsp_err);
		ret = -EIO;
		goto RELEASE_BUS;
	}

	if (I2C_TARGET_FLAGS_ADDR_10_BITS & cfg->flags) {
		data->target_fconfig.addr_mode = I2C_SLAVE_ADDR_MODE_10BIT;
	} else {
		data->target_fconfig.addr_mode = I2C_SLAVE_ADDR_MODE_7BIT;
	}
	data->target_cfg = cfg;
	data->target_fconfig.slave = cfg->address;
	data->target_fconfig.rate = data->ctrl_fconfig.rate;

	ret = calc_iic_target_clock_setting(dev, data->target_fconfig.rate,
					    &data->iic_target_ext_cfg.clock_settings);
	if (ret != 0) {
		LOG_ERR("Failed to calculate I2C Target clock settings");
		ret = -EIO;
		goto RELEASE_BUS;
	}

	data->target_fconfig.p_callback = i2c_ra_iic_target_callback;

	fsp_err = R_IIC_SLAVE_Open(&data->target_ctrl, &data->target_fconfig);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("%s: Failed to enter I2C Target mode. Try to re-open Controller mode",
			__func__);
		fsp_err = R_IIC_MASTER_Open(&data->control_ctrl, &data->ctrl_fconfig);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to re-open I2C Controller instance: %s", dev->name);
		}
		ret = -EIO;
	}

RELEASE_BUS:
	k_mutex_unlock(&data->bus_mutex);

	return ret;
}

static int i2c_ra_iic_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_ra_iic_data *data = dev->data;
	fsp_err_t fsp_err;
	int ret = 0;

	if (data->target_cfg != cfg) {
		return -EINVAL;
	}

	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	if (data->target_ctrl.open == 0) {
		LOG_ERR("%s: I2C Target instance is not opened.", __func__);
		ret = -EINVAL;
		goto RELEASE_BUS;
	}

	fsp_err = R_IIC_SLAVE_Close(&data->target_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("%s: Failed to close I2C Target instance. FSP_ERR=%d", __func__, fsp_err);
		ret = -EIO;
		goto RELEASE_BUS;
	}

	data->target_cfg = NULL;

	fsp_err = R_IIC_MASTER_Open(&data->control_ctrl, &data->ctrl_fconfig);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to re-open I2C Controller instance: %s", dev->name);
		return -EIO;
	}
RELEASE_BUS:
	k_mutex_unlock(&data->bus_mutex);

	return ret;
}

#endif /* CONFIG_I2C_TARGET */

static DEVICE_API(i2c, i2c_ra_iic_driver_api) = {
	.configure = i2c_ra_iic_configure,
	.get_config = i2c_ra_iic_get_config,
	.transfer = i2c_ra_iic_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_ra_iic_target_register,
	.target_unregister = i2c_ra_iic_target_unregister,
#endif /* CONFIG_I2C_TARGET */
};

#ifdef CONFIG_I2C_TARGET
#define IIC_RXI_ISR iic_rxi_isr
#define IIC_TXI_ISR iic_txi_isr
#define IIC_TEI_ISR iic_tei_isr
#define IIC_ERI_ISR iic_eri_isr
#define TARGET_FCONFIG_DEFAULT(index)                                                              \
	.target_fconfig =                                                                          \
		{                                                                                  \
			.channel = DT_INST_PROP(index, channel),                                   \
			.addr_mode = I2C_SLAVE_ADDR_MODE_7BIT,                                     \
			.rate = DT_INST_PROP(index, clock_frequency),                              \
			.rxi_irq = DT_INST_IRQ_BY_NAME(index, rxi, irq),                           \
			.txi_irq = DT_INST_IRQ_BY_NAME(index, txi, irq),                           \
			.tei_irq = DT_INST_IRQ_BY_NAME(index, tei, irq),                           \
			.eri_irq = DT_INST_IRQ_BY_NAME(index, eri, irq),                           \
			.eri_ipl = DT_INST_IRQ_BY_NAME(index, eri, priority),                      \
			.ipl = DT_INST_IRQ_BY_NAME(index, rxi, priority),                          \
			.p_callback = i2c_ra_iic_target_callback,                                  \
			.p_context = (void *)DEVICE_DT_INST_GET(index),                            \
			.clock_stretching_enable = false,                                          \
			.general_call_enable = false,                                              \
	},                                                                                         \
	.iic_target_ext_cfg = {                                                                    \
		.clock_settings =                                                                  \
			{                                                                          \
				.cks_value = 0,                                                    \
				.digital_filter_stages =                                           \
					DT_INST_PROP(index, target_digital_noise_filter),          \
			},                                                                         \
	},

void iic_rxi_isr(const struct device *dev)
{
	struct i2c_ra_iic_data *data = dev->data;

	if (data->target_ctrl.open) {
		iic_slave_rxi_isr();
	} else {
		iic_master_rxi_isr();
	}
}

void iic_txi_isr(const struct device *dev)
{
	struct i2c_ra_iic_data *data = dev->data;

	if (data->target_ctrl.open) {
		iic_slave_txi_isr();
	} else {
		iic_master_txi_isr();
	}
}

void iic_tei_isr(const struct device *dev)
{
	struct i2c_ra_iic_data *data = dev->data;

	if (data->target_ctrl.open) {
		iic_slave_tei_isr();
	} else {
		iic_master_tei_isr();
	}
}

void iic_eri_isr(const struct device *dev)
{
	struct i2c_ra_iic_data *data = dev->data;

	if (data->target_ctrl.open) {
		if (data->target_ctrl.p_reg->ICSR2_b.STOP == 1) {
			data->transaction_completed = true;
		}
		iic_slave_eri_isr();
	} else {
		iic_master_eri_isr();
	}
}

#else /* !CONFIG_I2C_TARGET */
#define IIC_RXI_ISR iic_master_rxi_isr
#define IIC_TXI_ISR iic_master_txi_isr
#define IIC_TEI_ISR iic_master_tei_isr
#define IIC_ERI_ISR iic_master_eri_isr
#define TARGET_FCONFIG_DEFAULT(index)
#endif /* CONFIG_I2C_TARGET */

#define EVENT_IIC_RXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _RXI))
#define EVENT_IIC_TXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _TXI))
#define EVENT_IIC_TEI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _TEI))
#define EVENT_IIC_ERI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_IIC, channel, _ERI))

#define IIC_RENESAS_RA_IRQ_INIT(index)                                                             \
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
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_IIC_RXI(DT_INST_PROP(index, channel)));     \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_IIC_TXI(DT_INST_PROP(index, channel)));     \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_IIC_TEI(DT_INST_PROP(index, channel)));     \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(EVENT_IIC_ERI(DT_INST_PROP(index, channel)));     \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rxi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, rxi, priority), IIC_RXI_ISR,                \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, txi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, txi, priority), IIC_TXI_ISR,                \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, tei, priority), IIC_TEI_ISR,                \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, eri, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, eri, priority), IIC_ERI_ISR,                \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, rxi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, txi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, tei, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, eri, irq));                                  \
	}

#define I2C_RA_IIC_INIT(index)                                                                     \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(index, clock_frequency) <=                                       \
			     DT_INST_PROP(index, max_bitrate_supported),                           \
		     "The desire clock-frequency in devicetree exceeds max-bitrate-supported");    \
                                                                                                   \
	static const struct i2c_ra_iic_config i2c_ra_iic_config_##index = {                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(index, 0, mstp),      \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(index, 0, stop_bit),        \
			},                                                                         \
		.ctrl_noise_filter_stage = 1, /* Cannot be configured. */                          \
		.ctrl_rise_time_ns = DT_INST_PROP(index, rise_time_ns),                            \
		.ctrl_fall_time_ns = DT_INST_PROP(index, fall_time_ns),                            \
		.ctrl_duty_cycle_percent = DT_INST_PROP(index, duty_cycle_percent),                \
		.max_bitrate_supported = DT_INST_PROP(index, max_bitrate_supported),               \
	};                                                                                         \
                                                                                                   \
	static struct i2c_ra_iic_data i2c_ra_iic_data_##index = {                                  \
		.ctrl_fconfig =                                                                    \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.slave = 0,                                                        \
				.rate = DT_INST_PROP(index, clock_frequency),                      \
				.addr_mode = I2C_MASTER_ADDR_MODE_7BIT,                            \
				.rxi_irq = DT_INST_IRQ_BY_NAME(index, rxi, irq),                   \
				.txi_irq = DT_INST_IRQ_BY_NAME(index, txi, irq),                   \
				.tei_irq = DT_INST_IRQ_BY_NAME(index, tei, irq),                   \
				.eri_irq = DT_INST_IRQ_BY_NAME(index, eri, irq),                   \
				.ipl = DT_INST_IRQ_BY_NAME(index, eri, priority),                  \
				.p_callback = i2c_ra_iic_ctrl_callback,                            \
				.p_context = (void *)DEVICE_DT_GET(DT_DRV_INST(index)),            \
			},                                                                         \
		TARGET_FCONFIG_DEFAULT(index)};                                                    \
                                                                                                   \
	static int i2c_renesas_ra_init##index(const struct device *dev)                            \
	{                                                                                          \
		IIC_RENESAS_RA_IRQ_INIT(index)                                                     \
		return i2c_ra_iic_init(dev);                                                       \
	}                                                                                          \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_renesas_ra_init##index, NULL,                         \
				  &i2c_ra_iic_data_##index, &i2c_ra_iic_config_##index,            \
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &i2c_ra_iic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RA_IIC_INIT)
