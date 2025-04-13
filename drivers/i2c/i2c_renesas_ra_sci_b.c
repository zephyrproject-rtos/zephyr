/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_i2c_sci_b

#define MDDR_DISABLE 256

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "r_sci_b_i2c.h"
#ifdef CONFIG_I2C_RENESAS_RA_SCI_B_DTC
#include "r_dtc.h"
#endif

#include <soc.h>

LOG_MODULE_REGISTER(renesas_ra_i2c_sci_b, CONFIG_I2C_LOG_LEVEL);

#define I2C_MAX_MSG_LEN (1 << (sizeof(uint8_t) * 8))

typedef void (*init_func_t)(const struct device *dev);
struct sci_b_i2c_config {
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
	uint16_t sda_output_delay;
};

struct sci_b_i2c_data {
	sci_b_i2c_instance_ctrl_t ctrl;
	i2c_master_cfg_t i2c_config;
	sci_b_i2c_extended_cfg_t ext_cfg;
	struct k_sem bus_lock;
	struct k_sem complete_sem;
	i2c_master_event_t event;
	uint32_t dev_config;

#ifdef CONFIG_I2C_CALLBACK
	uint16_t addr;
	uint32_t msg_idx;
	struct i2c_msg *msgs;
	uint32_t num_msgs;
	i2c_callback_t cb;
	void *p_context;
#endif /* CONFIG_I2C_CALLBACK */

#ifdef CONFIG_I2C_RENESAS_RA_SCI_B_DTC
	/* RX */
	transfer_instance_t rx_transfer;
	transfer_info_t rx_transfer_info;
	transfer_cfg_t rx_transfer_cfg;
	dtc_instance_ctrl_t rx_transfer_ctrl;
	dtc_extended_cfg_t rx_transfer_cfg_extend;

	/* TX */
	transfer_instance_t tx_transfer;
	transfer_info_t tx_transfer_info;
	transfer_cfg_t tx_transfer_cfg;
	dtc_instance_ctrl_t tx_transfer_ctrl;
	dtc_extended_cfg_t tx_transfer_cfg_extend;
#endif /* CONFIG_I2C_RENESAS_RA_SCI_B_DTC */
};

static void calc_sci_b_iic_clock_setting(const struct device *dev, const uint32_t fsp_i2c_rate,
					 sci_b_i2c_clock_settings_t *clk_cfg);

/* FSP interruption handlers. */
void sci_b_i2c_txi_isr(void);
void sci_b_i2c_tei_isr(void);
void sci_b_i2c_rxi_isr(void);

static int renesas_ra_sci_b_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	struct sci_b_i2c_data *data = (struct sci_b_i2c_data *const)dev->data;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Only I2C Master mode supported.");
		return -EIO;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->i2c_config.rate = I2C_MASTER_RATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		data->i2c_config.rate = I2C_MASTER_RATE_FAST;
		break;
	default:
		LOG_ERR("Invalid I2C speed rate flag: %d", I2C_SPEED_GET(dev_config));
		return -EIO;
	}

	calc_sci_b_iic_clock_setting(dev, data->i2c_config.rate, &data->ext_cfg.clock_settings);

	R_SCI_B_I2C_Close(&data->ctrl);
	R_SCI_B_I2C_Open(&data->ctrl, &data->i2c_config);

	/* save current devconfig. */
	data->dev_config = dev_config;

	return 0;
}

static int renesas_ra_sci_b_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct sci_b_i2c_data *data = (struct sci_b_i2c_data *const)dev->data;
	*dev_config = data->dev_config;
	return 0;
}

#define OPERATION(msg) (((struct i2c_msg *)msg)->flags & I2C_MSG_RW_MASK)

static int renesas_ra_sci_b_i2c_transfer(const struct device *dev, struct i2c_msg *msgs,
					 uint8_t num_msgs, uint16_t addr)
{
	struct sci_b_i2c_data *data = (struct sci_b_i2c_data *const)dev->data;
	struct i2c_msg *current, *next;
	fsp_err_t fsp_err;
	int ret;

	if (!num_msgs) {
		return 0;
	}

	/* Handle i2c burst write, restructure message to be compatible with HAL*/
	if (num_msgs == 2) {
		if (msgs[0].len == 1U && !(msgs[0].flags & I2C_MSG_READ) &&
		    !(msgs[1].flags & I2C_MSG_READ)) {
			uint16_t tmp_len = msgs[0].len + msgs[1].len;

			if (tmp_len <= I2C_MAX_MSG_LEN) {
				static uint8_t merge_buf[I2C_MAX_MSG_LEN];
				struct i2c_msg tmp_msg;

				memcpy(&merge_buf[0], msgs[0].buf, msgs[0].len);
				memcpy(&merge_buf[msgs[0].len], msgs[1].buf, msgs[1].len);
				tmp_msg.buf = &merge_buf[0];
				tmp_msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;
				tmp_msg.len = (uint8_t)tmp_len;
				/* Merge 2 msgs into 1 msg */
				msgs[0] = tmp_msg;
				num_msgs = 1;
			} else {
				LOG_DBG("messages are too large to merge");
			}
		}
	}

	current = msgs;
	ret = 0;

	/* Check for validity of all messages before transfer */
	for (int i = 1; i <= num_msgs; i++) {
		if (i < num_msgs) {
			next = current + 1;

			/*
			 * Restart condition between messages
			 * of different directions is required
			 */
			if (OPERATION(current) != OPERATION(next)) {
				if (!(next->flags & I2C_MSG_RESTART)) {
					LOG_ERR("Restart condition between messages of "
						"different directions is required."
						"Current/Total: [%d/%d]",
						i, num_msgs);
					ret = -EIO;
					break;
				}
			}

			/* Stop condition is only allowed on last message */
			if (current->flags & I2C_MSG_STOP) {
				LOG_ERR("Invalid stop flag. Stop condition is only allowed on "
					"last message. "
					"Current/Total: [%d/%d]",
					i, num_msgs);
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

	k_sem_take(&data->bus_lock, K_FOREVER);

	/* Set destination address with configured address mode before sending msg. */

	i2c_master_addr_mode_t addr_mode = 0;

	if (I2C_MSG_ADDR_10_BITS & data->dev_config) {
		addr_mode = I2C_MASTER_ADDR_MODE_10BIT;
	} else {
		addr_mode = I2C_MASTER_ADDR_MODE_7BIT;
	}

	R_SCI_B_I2C_SlaveAddressSet(&data->ctrl, addr, addr_mode);

	/* Process input `msgs`. */

	current = msgs;

	while (num_msgs > 0) {
		if (num_msgs > 1) {
			next = current + 1;
		} else {
			next = NULL;
		}

		if (current->flags & I2C_MSG_READ) {
			fsp_err = R_SCI_B_I2C_Read(&data->ctrl, current->buf, current->len,
						   next != NULL && (next->flags & I2C_MSG_RESTART));
		} else {
			fsp_err =
				R_SCI_B_I2C_Write(&data->ctrl, current->buf, current->len,
						  next != NULL && (next->flags & I2C_MSG_RESTART));
		}

		if (fsp_err != FSP_SUCCESS) {
			switch (fsp_err) {
			case FSP_ERR_INVALID_SIZE:
				LOG_ERR("Provided number of bytes more than uint16_t size "
					"(65535) while DTC is used for data transfer.");
				break;
			case FSP_ERR_IN_USE:
				LOG_ERR("Bus busy condition. Another transfer was in progress.");
				break;
			default:
				LOG_ERR("Unknown error.");
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
			LOG_ERR("%s failed.", (current->flags & I2C_MSG_READ) ? "Read" : "Write");
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
	k_sem_give(&data->bus_lock);

	return ret;
}

#ifdef CONFIG_I2C_CALLBACK

static void renesas_ra_sci_b_i2c_async_done(const struct device *dev, struct sci_b_i2c_data *data,
					    int result)
{

	i2c_callback_t cb = data->cb;
	void *p_context = data->p_context;

	data->msg_idx = 0;
	data->msgs = NULL;
	data->num_msgs = 0;
	data->cb = NULL;
	data->p_context = NULL;
	data->addr = 0;

	k_sem_give(&data->bus_lock);

	/* Callback may wish to start another transfer */
	cb(dev, result, p_context);
}

/* Start a transfer asynchronously */
static void renesas_ra_sci_b_i2c_async_iter(const struct device *dev)
{
	struct sci_b_i2c_data *data = dev->data;
	fsp_err_t fsp_err;
	struct i2c_msg *current, *next;

	struct i2c_msg *msg = &data->msgs[data->msg_idx];

	/* Check for validity of all messages before transfer */
	current = msg;
	if (data->msg_idx < (data->num_msgs - 1)) {
		next = current + 1;

		/*
		 * Restart condition between messages
		 * of different directions is required
		 */
		if (OPERATION(current) != OPERATION(next)) {
			if (!(next->flags & I2C_MSG_RESTART)) {
				LOG_ERR("Restart condition between messages of "
					"different directions is required."
					"Current/Total: [%d/%d]",
					data->msg_idx + 1, data->num_msgs);
				renesas_ra_sci_b_i2c_async_done(dev, data, -EIO);
				return;
			}
		}

		if (current->flags & I2C_MSG_STOP) {
			LOG_ERR("Invalid stop flag. Stop condition is only allowed on "
				"last message. "
				"Current/Total: [%d/%d]",
				data->msg_idx + 1, data->num_msgs);
			renesas_ra_sci_b_i2c_async_done(dev, data, -EIO);
			return;
		}
	} else {
		current->flags |= I2C_MSG_STOP;
		next = NULL;
	}

	if (current->flags & I2C_MSG_READ) {
		fsp_err = R_SCI_B_I2C_Read(&data->ctrl, current->buf, current->len,
					   (next != NULL) && (next->flags & I2C_MSG_RESTART));
	} else {
		fsp_err = R_SCI_B_I2C_Write(&data->ctrl, current->buf, current->len,
					    (next != NULL) && (next->flags & I2C_MSG_RESTART));
	}

	/* Return an error if the transfer didn't start successfully
	 * e.g., if the bus was busy
	 */
	if (fsp_err != FSP_SUCCESS) {
		switch (fsp_err) {
		case FSP_ERR_INVALID_SIZE:
			LOG_ERR("Provided number of bytes more than uint16_t size "
				"(65535) while DTC is used for data transfer.");
			break;
		case FSP_ERR_IN_USE:
			LOG_ERR("Bus busy condition. Another transfer was in progress.");
			break;
		default:
			LOG_ERR("Unknown error.");
			break;
		}

		R_SCI_B_I2C_Abort(&data->ctrl);
		return;
	}
}

static int renesas_ra_sci_b_i2c_transfer_cb(const struct device *dev, struct i2c_msg *msgs,
					    uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
					    void *p_context)
{
	struct sci_b_i2c_data *data = dev->data;

	int res = k_sem_take(&data->bus_lock, K_NO_WAIT);

	if (res != 0) {
		return -EWOULDBLOCK;
	}

	data->msg_idx = 0;
	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->addr = addr;
	data->cb = cb;
	data->p_context = p_context;

	renesas_ra_sci_b_i2c_async_iter(dev);

	return 0;
}

#endif /* CONFIG_I2C_CALLBACK */

static void renesas_ra_sci_b_i2c_callback(i2c_master_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct sci_b_i2c_data *data = dev->data;

#ifdef CONFIG_I2C_CALLBACK
	if (data->cb != NULL) {
		/* Async transfer */
		if (p_args->event == I2C_MASTER_EVENT_ABORTED) {
			R_SCI_B_I2C_Abort(&data->ctrl);
			renesas_ra_sci_b_i2c_async_done(dev, data, -EIO);
		} else if (data->msg_idx == data->num_msgs - 1) {
			renesas_ra_sci_b_i2c_async_done(dev, data, 0);
		} else {
			data->msg_idx++;
			renesas_ra_sci_b_i2c_async_iter(dev);
		}
		return;
	}
#endif /* CONFIG_I2C_CALLBACK */

	data->event = p_args->event;

	k_sem_give(&data->complete_sem);
}

static int renesas_ra_sci_b_i2c_init(const struct device *dev)
{
	const struct sci_b_i2c_config *config = dev->config;
	struct sci_b_i2c_data *data = (struct sci_b_i2c_data *)dev->data;
	fsp_err_t fsp_err;
	int ret;

	data->dev_config |= I2C_MODE_CONTROLLER;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("Pinctrl config failed.");
		return ret;
	}

	k_sem_init(&data->bus_lock, 1, 1);
	k_sem_init(&data->complete_sem, 0, 1);

	switch (data->i2c_config.rate) {
	case I2C_MASTER_RATE_STANDARD:
		calc_sci_b_iic_clock_setting(dev, data->i2c_config.rate,
					     &data->ext_cfg.clock_settings);
		data->i2c_config.p_extend = &data->ext_cfg;
		data->dev_config |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case I2C_MASTER_RATE_FAST:
		calc_sci_b_iic_clock_setting(dev, data->i2c_config.rate,
					     &data->ext_cfg.clock_settings);
		data->i2c_config.p_extend = &data->ext_cfg;
		data->dev_config |= I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	default:
		LOG_ERR("Invalid I2C speed rate: %d", data->i2c_config.rate);
		return -ENOTSUP;
	}

#ifdef CONFIG_I2C_RENESAS_RA_SCI_B_DTC
	data->i2c_config.p_transfer_rx = &data->rx_transfer;
	data->i2c_config.p_transfer_tx = &data->tx_transfer;
#endif

	fsp_err = R_SCI_B_I2C_Open(&data->ctrl, &data->i2c_config);

	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("I2C init failed.");
	}

	config->irq_config_func(dev);

	return 0;
};

static void calc_sci_b_iic_clock_setting(const struct device *dev, const uint32_t fsp_i2c_rate,
					 sci_b_i2c_clock_settings_t *clk_cfg)
{
	const struct sci_b_i2c_config *config = dev->config;

	uint32_t bitrate = 0;
	bool use_mddr = clk_cfg->bitrate_modulation;

	uint32_t divisor = 0;
	uint32_t divisor_bitrate_multiple = 0;
	uint32_t brr = 0;
	int32_t cks = 0;
	uint32_t delta_error = 0;

	uint32_t sda_delay_clock = 0;
	uint32_t sda_delay_counts = 0;
	uint32_t temp_mddr;
	uint32_t calculated_bitrate = 0;

	uint32_t mddr = MDDR_DISABLE;

	uint32_t peripheral_clock = R_FSP_SciClockHzGet();

	uint32_t sda_delay_ns = config->sda_output_delay;

	if (I2C_MASTER_RATE_FAST == fsp_i2c_rate) {
		bitrate = 400000;
	} else {
		bitrate = 100000;
	}

	for (uint32_t i = 0; i <= 3; i++) {

		divisor_bitrate_multiple = (1 << (2 * (i + 1))) * 8;
		divisor = divisor_bitrate_multiple * bitrate;

		/* Calculate BRR so that the bit rate is the largest possible value less than or
		 * equal to the desired bitrate.
		 */
		brr = (uint32_t)ceil(((double)peripheral_clock) / divisor - 1);
		if (brr <= 255) {
			break;
		}
		cks++;
	}
	calculated_bitrate = (uint32_t)((double)peripheral_clock) /
			     (divisor_bitrate_multiple * (256 / mddr) * (brr + 1));
	delta_error = bitrate - calculated_bitrate;

	if (use_mddr) {
		for (uint32_t temp_brr = brr; temp_brr > 0; temp_brr--) {

			/** Calculate the MDDR (M) value if bit rate modulation is enabled,
			 * The formula to calculate MBBR (from the M and N relationship given in the
			 * hardware manual) is as follows and it must be between 128 and 256. MDDR =
			 * ((divisor * 256) * (BRR + 1)) / PCLK
			 */
			temp_mddr = (uint32_t)floor(((double)divisor) * 256 * (temp_brr + 1) /
						    peripheral_clock);

			/* The maximum value that could result from the calculation above is 256,
			 * which is a valid MDDR value, so only the lower bound is checked.
			 */
			if (temp_mddr < 128) {
				break;
			}

			/* The maximum for MDDR is 256 (MDDR unused). */
			if (temp_mddr > 256) {
				continue;
			}

			calculated_bitrate =
				(uint32_t)(peripheral_clock /
					   (divisor_bitrate_multiple * (256 / ((double)temp_mddr)) *
					    (temp_brr + 1)));

			/** If the bit rate error is less than the previous lowest bit rate error,
			 * then store these settings as the best value.
			 */
			if ((bitrate - calculated_bitrate) < delta_error) {
				delta_error = bitrate - calculated_bitrate;
				brr = temp_brr;
				mddr = temp_mddr;
			}
		}
	}

	/* If MDDR == 256, disable bitrate modulation and set MDDR to a valid value. */
	if (mddr == 256) {
		mddr = 255;
		use_mddr = false;
	}

	/* Calculate SDA delay. */
	sda_delay_clock = peripheral_clock >> cks;
	sda_delay_counts = (uint32_t)ceil(((double)sda_delay_ns) * sda_delay_clock / 1000000000);
	if (sda_delay_counts > 31) {
		sda_delay_counts = 31;
	}

	clk_cfg->clk_divisor_value = (uint8_t)cks;
	clk_cfg->brr_value = (uint8_t)brr;
	clk_cfg->mddr_value = (uint8_t)mddr;
	clk_cfg->bitrate_modulation = use_mddr;
	clk_cfg->cycles_value = (uint8_t)sda_delay_counts;
}

static const struct i2c_driver_api renesas_ra_sci_b_i2c_driver_api = {
	.configure = renesas_ra_sci_b_i2c_configure,
	.get_config = renesas_ra_sci_b_i2c_get_config,
	.transfer = renesas_ra_sci_b_i2c_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = renesas_ra_sci_b_i2c_transfer_cb,
#endif /* CONFIG_I2C_CALLBACK */
};

#define _ELC_EVENT_SCI_RXI(channel) ELC_EVENT_SCI##channel##_RXI
#define _ELC_EVENT_SCI_TXI(channel) ELC_EVENT_SCI##channel##_TXI
#define _ELC_EVENT_SCI_TEI(channel) ELC_EVENT_SCI##channel##_TEI

#define ELC_EVENT_SCI_RXI(channel) _ELC_EVENT_SCI_RXI(channel)
#define ELC_EVENT_SCI_TXI(channel) _ELC_EVENT_SCI_TXI(channel)
#define ELC_EVENT_SCI_TEI(channel) _ELC_EVENT_SCI_TEI(channel)

#ifndef CONFIG_I2C_RENESAS_RA_SCI_B_DTC
#define SCI_B_I2C_DTC_INIT(index)
#define RXI_TRANSFER(index)
#else
#define SCI_B_I2C_DTC_INIT(index)                                                                  \
	.rx_transfer_info =                                                                        \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED, \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,  \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                          \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,        \
			.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 0,                                                               \
	},                                                                                         \
	.rx_transfer_cfg_extend = {.activation_source =                                            \
					   DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq)},       \
	.rx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &sci_b_i2c_data_##index.rx_transfer_info,                        \
			.p_extend = &sci_b_i2c_data_##index.rx_transfer_cfg_extend,                \
	},                                                                                         \
	.rx_transfer =                                                                             \
		{                                                                                  \
			.p_ctrl = &sci_b_i2c_data_##index.rx_transfer_ctrl,                        \
			.p_cfg = &sci_b_i2c_data_##index.rx_transfer_cfg,                          \
			.p_api = &g_transfer_on_dtc,                                               \
	},                                                                                         \
	.tx_transfer_info =                                                                        \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,       \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,       \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                          \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,  \
			.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 0,                                                               \
	},                                                                                         \
	.tx_transfer_cfg_extend = {.activation_source =                                            \
					   DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq)},       \
	.tx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &sci_b_i2c_data_##index.tx_transfer_info,                        \
			.p_extend = &sci_b_i2c_data_##index.tx_transfer_cfg_extend,                \
	},                                                                                         \
	.tx_transfer = {                                                                           \
		.p_ctrl = &sci_b_i2c_data_##index.tx_transfer_ctrl,                                \
		.p_cfg = &sci_b_i2c_data_##index.tx_transfer_cfg,                                  \
		.p_api = &g_transfer_on_dtc,                                                       \
	},

#define RXI_TRANSFER(index)                                                                        \
	/* rxi */                                                                                  \
	R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq)] =                            \
		ELC_EVENT_SCI_RXI(DT_INST_PROP(index, channel));                                   \
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),                               \
		    DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, priority), sci_b_i2c_rxi_isr,       \
		    DEVICE_DT_INST_GET(index), 0);                                                 \
	irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq));
#endif

#define SCI_B_I2C_RA_INIT(index)                                                                   \
	static void renesas_ra_sci_b_i2c_irq_config_func##index(const struct device *dev)          \
	{                                                                                          \
		RXI_TRANSFER(index)                                                                \
                                                                                                   \
		/* txi */                                                                          \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq)] =                    \
			ELC_EVENT_SCI_TXI(DT_INST_PROP(index, channel));                           \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),                  \
			    sci_b_i2c_txi_isr, DEVICE_DT_INST_GET(index), 0);                      \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq));                       \
                                                                                                   \
		/* tei */                                                                          \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq)] =                    \
			ELC_EVENT_SCI_TEI(DT_INST_PROP(index, channel));                           \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, priority),                  \
			    sci_b_i2c_tei_isr, DEVICE_DT_INST_GET(index), 0);                      \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq));                       \
	}                                                                                          \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
                                                                                                   \
	static const struct sci_b_i2c_config sci_b_i2c_config_##index = {                          \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.irq_config_func = renesas_ra_sci_b_i2c_irq_config_func##index,                    \
		.sda_output_delay = DT_INST_PROP(index, sda_output_delay),                         \
	};                                                                                         \
	static struct sci_b_i2c_data sci_b_i2c_data_##index = {                                    \
		.i2c_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.slave = 0,                                                        \
				.rate = I2C_MASTER_RATE_STANDARD,                                  \
				.addr_mode = I2C_MASTER_ADDR_MODE_7BIT,                            \
				.ipl = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),       \
				.rxi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),        \
				.txi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),        \
				.tei_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),        \
				.p_callback = renesas_ra_sci_b_i2c_callback,                       \
				.p_context = DEVICE_DT_GET(DT_DRV_INST(index)),                    \
			},                                                                         \
		.ext_cfg =                                                                         \
			{                                                                          \
				.clock_settings.snfr_value =                                       \
					DT_INST_PROP(index, noise_filter_clock_select),            \
				.clock_settings.bitrate_modulation = DT_INST_NODE_HAS_PROP(        \
					DT_DRV_INST(index), bit_rate_modulation),                  \
				.clock_settings.clock_source = SCI_B_I2C_CLOCK_SOURCE_SCISPICLK,   \
			},                                                                         \
		SCI_B_I2C_DTC_INIT(index)};                                                        \
	I2C_DEVICE_DT_INST_DEFINE(index, renesas_ra_sci_b_i2c_init, NULL, &sci_b_i2c_data_##index, \
				  &sci_b_i2c_config_##index, POST_KERNEL,                          \
				  CONFIG_I2C_INIT_PRIORITY, &renesas_ra_sci_b_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCI_B_I2C_RA_INIT)
