/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_i2c_sci_b

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include "r_sci_b_i2c.h"
#include <soc.h>

#ifdef CONFIG_RENESAS_RX_ICUM_GRP_IRQ
#include <zephyr/drivers/interrupt_controller/intc_renesas_rx_icum_grp_irq.h>
#endif /* CONFIG_RENESAS_RX_ICUM_GRP_IRQ */

LOG_MODULE_REGISTER(renesas_rx_i2c_sci_b, CONFIG_I2C_LOG_LEVEL);

#define I2C_MAX_MSG_LEN (1 << (sizeof(uint8_t) * 8))

#define ICU_EVENT_SCI_TXI(channel) CONCAT(ICU_EVENT_RSCI, channel, _TXI)
#define MDDR_DISABLE               (256U)
#define MDDR_SCALE_FACTOR          (256U)
#define MDDR_MIN_VALID_VALUE       (128U)
#define MDDR_MAX_VALID_VALUE       (256U)
#define SDA_DELAY_MAX_COUNTS       (31U)
#define BRR_MAX_VALUE              (255U)

struct sci_b_i2c_config {
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
	uint16_t sda_output_delay;
#ifdef CONFIG_RENESAS_RX_ICUM_GRP_IRQ
	/*  Group interrupt controller for the transmit end interrupt (TEI) */
	const struct device *tei_ctrl;
	/*  Group interrupt number for the transmit end interrupt (TEI) */
	uint8_t tei_num;
#endif /* CONFIG_RENESAS_RX_ICUM_GRP_IRQ */
};

struct sci_b_i2c_data {
	sci_b_i2c_instance_ctrl_t ctrl;
	i2c_master_cfg_t i2c_config;
	sci_b_i2c_extended_cfg_t ext_cfg;
	struct k_sem bus_lock;
	struct k_sem complete_sem;
	i2c_master_event_t event;
	uint32_t dev_config;
	uint8_t merge_buf[I2C_MAX_MSG_LEN];
#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK
	uint16_t addr;
	uint32_t msg_idx;
	struct i2c_msg *msgs;
	uint32_t num_msgs;
	i2c_callback_t cb;
	void *p_context;
#endif /* CONFIG_I2C_RENESAS_RX_CALLBACK */
};

/* FSP interruption handlers. */
void sci_b_i2c_txi_isr(void);
void sci_b_i2c_tei_isr(void);

static void renesas_rx_sci_b_i2c_calc_clock_setting(const struct device *dev,
						    const uint32_t fsp_i2c_rate,
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
		if (brr <= BRR_MAX_VALUE) {
			break;
		}
		cks++;
	}
	calculated_bitrate = (uint32_t)((double)peripheral_clock) /
			     (divisor_bitrate_multiple * (MDDR_SCALE_FACTOR / mddr) * (brr + 1));
	delta_error = bitrate - calculated_bitrate;

	if (use_mddr) {
		for (uint32_t temp_brr = brr; temp_brr > 0; temp_brr--) {

			/** Calculate the MDDR (M) value if bit rate modulation is enabled,
			 * The formula to calculate MBBR (from the M and N relationship given in the
			 * hardware manual) is as follows and it must be between 128 and 256. MDDR =
			 * ((divisor * 256) * (BRR + 1)) / PCLK
			 */
			temp_mddr = (uint32_t)floor(((double)divisor) * MDDR_SCALE_FACTOR *
						    (temp_brr + 1) / peripheral_clock);

			/* The maximum value that could result from the calculation above is 256,
			 * which is a valid MDDR value, so only the lower bound is checked.
			 */
			if (temp_mddr < MDDR_MIN_VALID_VALUE) {
				break;
			}

			/* The maximum for MDDR is 256 (MDDR unused). */
			if (temp_mddr > MDDR_MAX_VALID_VALUE) {
				continue;
			}

			calculated_bitrate = (uint32_t)(peripheral_clock /
							(divisor_bitrate_multiple *
							 (MDDR_SCALE_FACTOR / ((double)temp_mddr)) *
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
	if (mddr == MDDR_MAX_VALID_VALUE) {
		mddr = MDDR_MAX_VALID_VALUE - 1;
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

static int renesas_rx_sci_b_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	struct sci_b_i2c_data *data = (struct sci_b_i2c_data *const)dev->data;
	int ret = 0;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Only I2C Master mode supported.");
		return -EINVAL;
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
		return -EINVAL;
	}

	renesas_rx_sci_b_i2c_calc_clock_setting(dev, data->i2c_config.rate,
						&data->ext_cfg.clock_settings);

	ret = R_SCI_B_I2C_Close(&data->ctrl);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("Failed to close I2C driver. FSP error code: %d", ret);
		return -EIO;
	}

	ret = R_SCI_B_I2C_Open(&data->ctrl, &data->i2c_config);

	if (ret != FSP_SUCCESS) {
		LOG_ERR("I2C init failed. FSP error code: %d", ret);
		return -EIO;
	}

	/* save current devconfig. */
	data->dev_config = dev_config;

	return 0;
}

static int renesas_rx_sci_b_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct sci_b_i2c_data *data = (struct sci_b_i2c_data *const)dev->data;
	*dev_config = data->dev_config;
	return 0;
}

#define OPERATION(msg) (((struct i2c_msg *)msg)->flags & I2C_MSG_RW_MASK)

static int renesas_rx_sci_b_i2c_transfer(const struct device *dev, struct i2c_msg *msgs,
					 uint8_t num_msgs, uint16_t addr)
{
	struct sci_b_i2c_data *data = (struct sci_b_i2c_data *const)dev->data;
	struct i2c_msg *current, *next;
	fsp_err_t fsp_err;
	int ret;
	uint8_t *merge_buf = data->merge_buf;
	struct i2c_msg tmp_msg;

	if (!num_msgs) {
		return 0;
	}

	/* Handle i2c burst write, restructure message to be compatible with HAL*/
	if (num_msgs == 2) {
		if (msgs[0].len == 1U && !(msgs[0].flags & I2C_MSG_READ) &&
		    !(msgs[1].flags & I2C_MSG_READ)) {
			uint16_t tmp_len = msgs[0].len + msgs[1].len;

			if (tmp_len <= I2C_MAX_MSG_LEN) {
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

#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK

static void renesas_rx_sci_b_i2c_async_done(const struct device *dev, struct sci_b_i2c_data *data,
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
static void renesas_rx_sci_b_i2c_async_iter(const struct device *dev)
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
				renesas_rx_sci_b_i2c_async_done(dev, data, -EIO);
				return;
			}
		}

		if (current->flags & I2C_MSG_STOP) {
			LOG_ERR("Invalid stop flag. Stop condition is only allowed on "
				"last message. "
				"Current/Total: [%d/%d]",
				data->msg_idx + 1, data->num_msgs);
			renesas_rx_sci_b_i2c_async_done(dev, data, -EIO);
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

		fsp_err = R_SCI_B_I2C_Abort(&data->ctrl);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to abort I2C driver. FSP error code: %d", fsp_err);
			renesas_rx_sci_b_i2c_async_done(dev, data, -EIO);
		}

		return;
	}
}

static int renesas_rx_sci_b_i2c_transfer_cb(const struct device *dev, struct i2c_msg *msgs,
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

	renesas_rx_sci_b_i2c_async_iter(dev);

	return 0;
}

#endif /* CONFIG_I2C_RENESAS_RX_CALLBACK */

static void renesas_rx_sci_b_i2c_callback(i2c_master_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct sci_b_i2c_data *data = dev->data;
#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK
	if (data->cb != NULL) {
		/* Async transfer */
		if (p_args->event == I2C_MASTER_EVENT_ABORTED) {
			R_SCI_B_I2C_Abort(&data->ctrl);
			renesas_rx_sci_b_i2c_async_done(dev, data, -EIO);
		} else if (data->msg_idx == data->num_msgs - 1) {
			renesas_rx_sci_b_i2c_async_done(dev, data, 0);
		} else {
			data->msg_idx++;
			renesas_rx_sci_b_i2c_async_iter(dev);
		}
		return;
	}
#endif /* CONFIG_I2C_RENESAS_RX_CALLBACK */

	data->event = p_args->event;

	k_sem_give(&data->complete_sem);
}

static int renesas_rx_sci_b_i2c_init(const struct device *dev)
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
		renesas_rx_sci_b_i2c_calc_clock_setting(dev, data->i2c_config.rate,
							&data->ext_cfg.clock_settings);
		data->i2c_config.p_extend = &data->ext_cfg;
		data->dev_config |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case I2C_MASTER_RATE_FAST:
		renesas_rx_sci_b_i2c_calc_clock_setting(dev, data->i2c_config.rate,
							&data->ext_cfg.clock_settings);
		data->i2c_config.p_extend = &data->ext_cfg;
		data->dev_config |= I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	default:
		LOG_ERR("Invalid I2C speed rate: %d", data->i2c_config.rate);
		return -ENOTSUP;
	}

	fsp_err = R_SCI_B_I2C_Open(&data->ctrl, &data->i2c_config);

	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("I2C init failed.");
		return -EIO;
	}

	config->irq_config_func(dev);
	return 0;
}

static DEVICE_API(i2c, renesas_rx_sci_b_i2c_driver_api) = {
	.configure = renesas_rx_sci_b_i2c_configure,
	.get_config = renesas_rx_sci_b_i2c_get_config,
	.transfer = renesas_rx_sci_b_i2c_transfer,
#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK
	.transfer_cb = renesas_rx_sci_b_i2c_transfer_cb,
#endif /* CONFIG_I2C_RENESAS_RX_CALLBACK */
};

static void sci_b_i2c_txi_handler(const struct device *dev)
{
	struct sci_b_i2c_data *data = dev->data;
	uint32_t irq = data->i2c_config.txi_irq;

	BSP_ICU_IRQ_HANDLER(irq, sci_b_i2c_txi_isr);
}

#ifdef CONFIG_RENESAS_RX_ICUM_GRP_IRQ
static void sci_b_i2c_tei_callback_setup(const struct device *dev)
{
	const struct sci_b_i2c_config *cfg = dev->config;
	struct sci_b_i2c_data *data = dev->data;
	int err;

	err = rx_icum_grp_irq_callback_set(cfg->tei_ctrl, cfg->tei_num, sci_b_i2c_tei_isr,
					   (void *)&data->ctrl);
	if (err != 0) {
		LOG_ERR("Failed to set callback for group interrupt TEI: %d", err);
		return;
	}

	err = rx_icum_grp_irq_enable(cfg->tei_ctrl, cfg->tei_num);
	if (err != 0) {
		LOG_ERR("Failed to allow interrupt request for TEI: %d", err);
	}
}
#else
static inline void sci_b_i2c_tei_callback_setup(const struct device *dev)
{
	ARG_UNUSED(dev);
}
#endif /* CONFIG_RENESAS_RX_ICUM_GRP_IRQ */

#ifdef CONFIG_RENESAS_RX_ICUM_GRP_IRQ
#define SCI_B_I2C_RX_TEI_CTRL_INIT(index)                                                          \
	.tei_ctrl = DEVICE_DT_GET(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(index), tei)),                \
	.tei_num = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),
#else
#define SCI_B_I2C_RX_TEI_CTRL_INIT(index)
#endif /* CONFIG_RENESAS_RX_ICUM_GRP_IRQ */

#define SCI_B_I2C_RX_INIT(index)                                                                   \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
	static void renesas_rx_sci_b_i2c_irq_config_func##index(const struct device *dev)          \
	{                                                                                          \
		int txi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq);                     \
		if (128 <= txi_irq && txi_irq < 144) {                                             \
			R_ICU->SLIXR[txi_irq - 128].SLIXR =                                        \
				ICU_EVENT_SCI_TXI(DT_PROP(DT_INST_PARENT(index), channel));        \
		} else if (144 <= txi_irq && txi_irq <= 255) {                                     \
			R_ICU->SLIR[txi_irq - 144].SLIR =                                          \
				ICU_EVENT_SCI_TXI(DT_PROP(DT_INST_PARENT(index), channel));        \
		} else {                                                                           \
			LOG_ERR("TXI IRQ num %d is INVALID", txi_irq);                             \
		}                                                                                  \
                                                                                                   \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),                  \
			    sci_b_i2c_txi_handler, DEVICE_DT_INST_GET(index), 0);                  \
                                                                                                   \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq));                       \
		sci_b_i2c_tei_callback_setup(dev);                                                 \
	};                                                                                         \
                                                                                                   \
	static const struct sci_b_i2c_config sci_b_i2c_config_##index = {                          \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.irq_config_func = renesas_rx_sci_b_i2c_irq_config_func##index,                    \
		.sda_output_delay = DT_INST_PROP(index, sda_output_delay),                         \
		SCI_B_I2C_RX_TEI_CTRL_INIT(index)};                                                \
                                                                                                   \
	static struct sci_b_i2c_data sci_b_i2c_data_##index = {                                    \
		.i2c_config =                                                                      \
			{                                                                          \
				.channel = DT_PROP(DT_INST_PARENT(index), channel),                \
				.slave = 0,                                                        \
				.rate = I2C_MASTER_RATE_STANDARD,                                  \
				.addr_mode = I2C_MASTER_ADDR_MODE_7BIT,                            \
				.ipl = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),       \
				.txi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),        \
				.p_callback = renesas_rx_sci_b_i2c_callback,                       \
				.p_context = (void *)DEVICE_DT_GET(DT_DRV_INST(index)),            \
			},                                                                         \
		.ext_cfg =                                                                         \
			{                                                                          \
				.clock_settings.snfr_value =                                       \
					DT_INST_PROP(index, noise_filter_clock_select),            \
				.clock_settings.bitrate_modulation = DT_INST_NODE_HAS_PROP(        \
					DT_DRV_INST(index), bit_rate_modulation),                  \
				.clock_settings.clock_source = SCI_B_I2C_CLOCK_SOURCE_SCISPICLK,   \
			},                                                                         \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(index, renesas_rx_sci_b_i2c_init, NULL, &sci_b_i2c_data_##index, \
				  &sci_b_i2c_config_##index, POST_KERNEL,                          \
				  CONFIG_I2C_INIT_PRIORITY, &renesas_rx_sci_b_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCI_B_I2C_RX_INIT)
