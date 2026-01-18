/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_i2c

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <errno.h>
#include <r_riic_rx_if.h>
#include <r_riic_rx_private.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <iodefine.h>

#ifdef CONFIG_RENESAS_RX_I2C_DTC
#include <zephyr/drivers/misc/renesas_rx_dtc/renesas_rx_dtc.h>
#endif /* CONFIG_RENESAS_RX_I2C_DTC */

LOG_MODULE_REGISTER(i2c_renesas_rx, CONFIG_I2C_LOG_LEVEL);

struct i2c_rx_config {
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
	void (*riic_eei_sub)(void);
	void (*riic_txi_sub)(void);
	void (*riic_rxi_sub)(void);
	void (*riic_tei_sub)(void);
};

struct i2c_rx_data {
	riic_info_t rdp_info;
	struct k_sem bus_lock;
	struct k_sem bus_sync;
	uint32_t dev_config;
	i2c_callback_t user_callback;
	void *user_data;
	bool skip_callback;
#ifdef CONFIG_RENESAS_RX_I2C_DTC
	volatile struct st_riic *p_regs;
	/* DTC device */
	const struct device *dtc;
	/* RX */
	transfer_info_t rxi_dtc_info;
	uint8_t rxi_count;
	uint8_t rxi_dtc_activation_irq;

	/* TX */
	transfer_info_t txi_dtc_info;
	uint8_t txi_count;
	uint8_t txi_dtc_activation_irq;

	/* Msgs to send and receive */
	struct i2c_msg *msgs;
	uint8_t slv_addr;
	uint8_t num_msgs;
	uint8_t num_processed_msgs;
#endif
};

/** Internal module driver functions */
riic_return_t riic_bps_calc(riic_info_t *p_riic_info, uint16_t kbps);

#ifdef CONFIG_RENESAS_RX_I2C_DTC
static inline void riic_start_cond_generate(struct i2c_rx_data *data)
{
	data->p_regs->ICIER.BIT.STIE = 1;
	data->p_regs->ICSR2.BIT.START = 0;
	data->p_regs->ICCR2.BIT.ST = 1;
}

static inline void riic_restart_cond_generate(struct i2c_rx_data *data)
{
	data->p_regs->ICIER.BIT.STIE = 1;
	data->p_regs->ICSR2.BIT.START = 0;
	data->p_regs->ICCR2.BIT.RS = 1;
}

static inline void riic_stop_cond_generate(struct i2c_rx_data *data)
{
	data->p_regs->ICIER.BIT.SPIE = 1;
	data->p_regs->ICSR2.BIT.STOP = 0;
	data->p_regs->ICCR2.BIT.SP = 1;
}

static inline void riic_txi_trigger(struct i2c_rx_data *data)
{
	/** Enable txi */
	data->p_regs->ICIER.BIT.TIE = 1;

	/** Enable write to TRS */
	data->p_regs->ICMR1.BIT.MTWP = 1;
	/** Clear TRS bit to clear the TDRE flag */
	data->p_regs->ICCR2.BIT.TRS = 0;
	data->p_regs->ICMR1.BIT.MTWP = 0;

	while (data->p_regs->ICCR2.BIT.TRS) {
		/* code */
	}

	/** Set TRS bit to set TDRE flag => trigger txi */
	data->p_regs->ICMR1.BIT.MTWP = 1;
	data->p_regs->ICCR2.BIT.TRS = 1;
	data->p_regs->ICMR1.BIT.MTWP = 0;
}
static uint8_t dummy_dest;
#endif

static void riic_eei_isr(const struct device *dev)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
#ifdef CONFIG_RENESAS_RX_I2C_DTC
	if (data->p_regs->ICSR2.BIT.TMOF) {
		LOG_ERR("Timed Out");
		/** Disable interrupt and clear flag */
		data->p_regs->ICIER.BIT.TMOIE = 0;
		data->p_regs->ICSR2.BIT.TMOF = 0;

		riic_stop_cond_generate(data);
		k_sem_give(&data->bus_sync);
		if ((data->user_callback != NULL) && !data->skip_callback) {
			data->user_callback(dev, -ETIME, data->user_data);
		}
		return;
	}
	if (data->p_regs->ICSR2.BIT.STOP) {
		/** Disable interrupt and clear flag */
		data->p_regs->ICIER.BIT.SPIE = 0;
		data->p_regs->ICSR2.BIT.STOP = 0;

		k_sem_give(&data->bus_sync);
		if ((data->user_callback != NULL) && !data->skip_callback) {
			data->user_callback(dev, 0, data->user_data);
		}
		return;
	}
	if (data->p_regs->ICSR2.BIT.NACKF) {
		/** Disable interrupt and clear flag */
		data->p_regs->ICIER.BIT.NAKIE = 0;
		data->p_regs->ICSR2.BIT.NACKF = 0;

		/** Generate stop condition */
		riic_stop_cond_generate(data);
		return;
	}
	if (data->p_regs->ICSR2.BIT.AL) {
		LOG_ERR("Arbitration Lost");
		/** Disable interrupt and clear flag */
		data->p_regs->ICIER.BIT.ALIE = 0;
		data->p_regs->ICSR2.BIT.AL = 0;

		/** Generate stop condition */
		riic_stop_cond_generate(data);
		return;
	}
	if (data->p_regs->ICSR2.BIT.START) {

		data->p_regs->ICIER.BIT.TIE = 1;
		/** Disable interrupt and clear flag */
		data->p_regs->ICIER.BIT.STIE = 0;
		data->p_regs->ICSR2.BIT.START = 0;

		if (data->num_msgs == 0) {
			riic_stop_cond_generate(data);
			return;
		}

		/** In case msg_len = 0, skip this msg */
		while (data->msgs[data->num_processed_msgs].len == 0) {
			data->num_processed_msgs++;
			if (data->num_processed_msgs == data->num_msgs) {
				riic_stop_cond_generate(data);
				return;
			}
		}

		static uint8_t first_byte;

		first_byte = data->slv_addr << 1;
		if ((data->msgs[data->num_processed_msgs].flags & I2C_MSG_RW_MASK) ==
		    I2C_MSG_WRITE) {
			first_byte &= W_CODE;
		} else {
			first_byte |= R_CODE;

			data->rxi_count = 0;
			data->rxi_dtc_info.p_src = (void *)(&data->p_regs->ICDRR);
			/**
			 * The first frame has no data
			 * but have to dummy read the data reg to clear flag
			 */
			data->rxi_dtc_info.length = 1;
			data->rxi_dtc_info.p_dest = &dummy_dest;
			dtc_renesas_rx_configuration(data->dtc, data->rxi_dtc_activation_irq,
						     &data->rxi_dtc_info);
			dtc_renesas_rx_start_transfer(data->dtc, data->rxi_dtc_activation_irq);
			data->p_regs->ICIER.BIT.RIE = 1;
		}

		/** Config DTC */
		data->txi_count = 0;
		data->txi_dtc_info.p_dest = (void *)(&data->p_regs->ICDRT);
		data->txi_dtc_info.p_src = &first_byte;
		data->txi_dtc_info.length = 1;

		dtc_renesas_rx_configuration(data->dtc, data->txi_dtc_activation_irq,
					     &data->txi_dtc_info);
		dtc_renesas_rx_start_transfer(data->dtc, data->txi_dtc_activation_irq);

		riic_txi_trigger(data);

		return;
	}
	return;
#else
	const struct i2c_rx_config *config = dev->config;

	riic_return_t rdp_ret;
	riic_info_t iic_info_m;
	riic_mcu_status_t iic_status;

	iic_info_m.ch_no = data->rdp_info.ch_no;
	rdp_ret = R_RIIC_GetStatus(&iic_info_m, &iic_status);
	if (rdp_ret == RIIC_SUCCESS) {
		if (iic_status.BIT.SP) {
			k_sem_give(&data->bus_sync);
			if ((data->user_callback != NULL) && !data->skip_callback) {
				data->user_callback(dev, 0, data->user_data);
			}
		}
		if (iic_status.BIT.TMO) {
			k_sem_give(&data->bus_sync);
			if ((data->user_callback != NULL) && !data->skip_callback) {
				data->user_callback(dev, -ETIME, data->user_data);
			}
		}
	}

	config->riic_eei_sub();
#endif
}
static void riic_rxi_isr(const struct device *dev)
{
#ifdef CONFIG_RENESAS_RX_I2C_DTC
	struct i2c_rx_data *data = dev->data;

	if (data->rxi_count == 0) {
		data->rxi_count++;

		/** Config DTC for rxi */
		data->rxi_dtc_info.p_dest = data->msgs[data->num_processed_msgs].buf;
		data->rxi_dtc_info.length = data->msgs[data->num_processed_msgs].len;

		dtc_renesas_rx_configuration(data->dtc, data->rxi_dtc_activation_irq,
					     &data->rxi_dtc_info);
		dtc_renesas_rx_start_transfer(data->dtc, data->rxi_dtc_activation_irq);
		return;
	} else if (data->rxi_count == 1) {
		data->rxi_count++;
		data->p_regs->ICMR3.BIT.ACKWP = 1;
		data->p_regs->ICMR3.BIT.ACKBT = 1;
		data->p_regs->ICMR3.BIT.ACKWP = 0;
		return;
	}
	data->p_regs->ICIER.BIT.RIE = 0;

	/** Update processed msg count */
	data->num_processed_msgs++;
	/** Stop when there no more msg */
	if (data->num_processed_msgs == data->num_msgs) {
		riic_stop_cond_generate(data);
		return;
	}
	/** Check restart flag of current msg */
	if (data->msgs[data->num_processed_msgs].flags & I2C_MSG_RESTART) {
		riic_restart_cond_generate(data);
	} else {
		/** Stop in other cases */
		riic_stop_cond_generate(data);
	}
	return;

#else
	const struct i2c_rx_config *config = dev->config;

	config->riic_rxi_sub();
#endif
}

static void riic_txi_isr(const struct device *dev)
{
#ifdef CONFIG_RENESAS_RX_I2C_DTC
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;

	if (data->txi_count == 0) {
		data->txi_count++;

		if ((data->msgs[data->num_processed_msgs].flags & I2C_MSG_RW_MASK) ==
		    I2C_MSG_WRITE) {
			/** Reconfig DTC */
			data->txi_dtc_info.p_src = data->msgs[data->num_processed_msgs].buf;
			data->txi_dtc_info.length = data->msgs[data->num_processed_msgs].len;
			if (data->msgs[data->num_processed_msgs].len == 1) {
				/** Enable transmit end interrupt */
				data->p_regs->ICIER.BIT.TEIE = 1;
			}

			dtc_renesas_rx_configuration(data->dtc, data->txi_dtc_activation_irq,
						     &data->txi_dtc_info);
			dtc_renesas_rx_start_transfer(data->dtc, data->txi_dtc_activation_irq);

			riic_txi_trigger(data);
		} else {
			data->p_regs->ICIER.BIT.TIE = 0;
		}
	} else if (data->txi_count == 1) {
		data->txi_count++;

		if (data->msgs[data->num_processed_msgs].len > 1) {
			/** Reconfig DTC */
			data->txi_dtc_info.length = 1;
			data->txi_dtc_info.p_src = data->msgs[data->num_processed_msgs].buf +
						   data->msgs[data->num_processed_msgs].len - 1;

			dtc_renesas_rx_configuration(data->dtc, data->txi_dtc_activation_irq,
						     &data->txi_dtc_info);
			dtc_renesas_rx_start_transfer(data->dtc, data->txi_dtc_activation_irq);

			/** Enable transmit end interrupt */
			data->p_regs->ICIER.BIT.TEIE = 1;

			riic_txi_trigger(data);
		} else {
			data->p_regs->ICIER.BIT.TIE = 0;
		}
	} else {
		data->p_regs->ICIER.BIT.TIE = 0;
	}

#else
	const struct i2c_rx_config *config = dev->config;

	config->riic_txi_sub();
#endif
}

static void riic_tei_isr(const struct device *dev)
{
#ifdef CONFIG_RENESAS_RX_I2C_DTC
	struct i2c_rx_data *data = dev->data;

	data->p_regs->ICSR2.BIT.TEND = 0;
	data->p_regs->ICIER.BIT.TEIE = 0;

	/** Check the stop flag of processed msg */
	if (data->msgs[data->num_processed_msgs].flags & I2C_MSG_STOP) {
		data->num_processed_msgs++;
		riic_stop_cond_generate(data);
		return;
	}

	/** Update processed msg count */
	data->num_processed_msgs++;

	/** When there no more msgs -> stop */
	if (data->num_processed_msgs == data->num_msgs) {
		riic_stop_cond_generate(data);
		return;
	}

	/** Check restart flag of current msg */
	if (data->msgs[data->num_processed_msgs].flags & I2C_MSG_RESTART) {
		riic_restart_cond_generate(data);
		return;
	}

	/**
	 * Check flag read write
	 * if READ => stop because it need restart but no restart flag
	 * if WRITE => update DTC and trigger txi
	 */
	if ((data->msgs[data->num_processed_msgs].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
		/** Update DTC */
		if (data->msgs[data->num_processed_msgs].len > 1) {
			data->txi_dtc_info.length = data->msgs[data->num_processed_msgs].len - 1;
		} else {
			data->txi_dtc_info.length = 1;

			/** Enable transmit end interrupt */
			data->p_regs->ICIER.BIT.TEIE = 1;
		}
		data->txi_count = 1;
		data->txi_dtc_info.p_src = data->msgs[data->num_processed_msgs].buf;
		dtc_renesas_rx_configuration(data->dtc, data->txi_dtc_activation_irq,
					     &data->txi_dtc_info);
		dtc_renesas_rx_start_transfer(data->dtc, data->txi_dtc_activation_irq);

		riic_txi_trigger(data);
	} else {
		riic_stop_cond_generate(data);
	}

#else
	const struct i2c_rx_config *config = dev->config;

	config->riic_tei_sub();
#endif
}

static void rdp_callback(void)
{
	/* Do nothing */
}

#ifndef CONFIG_RENESAS_RX_I2C_DTC
static void setup_rdp_info(struct i2c_rx_data *data, uint8_t *buf1, size_t len1, uint8_t *buf2,
			   size_t len2, uint16_t *addr)
{
	data->rdp_info.cnt1st = len1;
	data->rdp_info.cnt2nd = len2;
	data->rdp_info.p_data1st = buf1 ? buf1 : FIT_NO_PTR;
	data->rdp_info.p_data2nd = buf2 ? buf2 : FIT_NO_PTR;
	data->rdp_info.p_slv_adr = (uint8_t *)addr;
}
#endif /* CONFIG_RENESAS_RX_I2C_DTC */

static int run_rx_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t addr, bool async)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	int ret;
	riic_return_t rdp_ret = 0;

	ret = k_sem_take(&data->bus_lock, async ? K_NO_WAIT : K_FOREVER);
	if (ret != 0) {
		return -EBUSY;
	}

	k_sem_reset(&data->bus_sync);

#ifdef CONFIG_RENESAS_RX_I2C_DTC
	/** Wait for the bus become free */
	while (data->p_regs->ICCR2.BIT.BBSY == 1) {
	}
	/** Store msgs info */
	data->slv_addr = addr;
	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->num_processed_msgs = 0;

	/** Disable all interrupt */
	data->p_regs->ICIER.BYTE = 0x00;

	/** Enable error interrupt */
	data->p_regs->ICIER.BIT.TMOIE = 1;
	data->p_regs->ICIER.BIT.ALIE = 1;
	data->p_regs->ICIER.BIT.NAKIE = 1;

	riic_start_cond_generate(data);
#else
	if (addr == 0x00) {
		/* Enter transmission pattern 4 */
		LOG_DBG("RDP RX I2C master transmit pattern 4\n");
		setup_rdp_info(data, NULL, 0, NULL, 0, NULL);
		rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
		goto transfer_blocking;
	}

	if (num_msgs == 1) {
		if (msgs[0].flags & I2C_MSG_READ) {
			/* Enter master reception pattern 1 */
			LOG_DBG("RDP RX I2C master reception pattern 1\n");
			setup_rdp_info(data, NULL, 0, msgs[0].buf, msgs[0].len, &addr);
			rdp_ret = R_RIIC_MasterReceive(&data->rdp_info);
			goto transfer_blocking;
		} else {
			/* Enter master transmission pattern 2/3 */
			LOG_DBG("RDP RX I2C master transmit pattern 2/3\n");
			setup_rdp_info(data, NULL, 0, msgs[0].len ? msgs[0].buf : NULL, msgs[0].len,
				       &addr);
			rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
			goto transfer_blocking;
		}
	} else if (num_msgs == 2) {
		if (msgs[0].flags & I2C_MSG_READ) {
			goto unsupport_pattern;
		}

		if (msgs[1].flags & I2C_MSG_READ) {
			/* Enter master reception pattern 2 */
			LOG_DBG("RDP RX I2C master reception pattern 2\n");
			setup_rdp_info(data, msgs[0].buf, msgs[0].len, msgs[1].buf, msgs[1].len,
				       &addr);
			rdp_ret = R_RIIC_MasterReceive(&data->rdp_info);
			goto transfer_blocking;
		} else {
			/* Enter master transmission pattern 1 */
			LOG_DBG("RDP RX I2C master transmit pattern 1\n");
			setup_rdp_info(data, msgs[0].buf, msgs[0].len, msgs[1].buf, msgs[1].len,
				       &addr);
			rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
			goto transfer_blocking;
		}
	}

unsupport_pattern:
	/* Unsupport pattern, emit each fragment as a distinct transaction  */
	LOG_DBG("%s: \"Not a generic pattern ...\" !\n", __func__);
	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			/* Enter master reception pattern 1 */
			LOG_DBG("RDP RX I2C master reception pattern 1\n");
			setup_rdp_info(data, NULL, 0, msgs[i].buf, msgs[i].len, &addr);
			rdp_ret = R_RIIC_MasterReceive(&data->rdp_info);
		} else {
			/* Enter master transmission pattern 2 */
			LOG_DBG("RDP RX I2C master transmit pattern 2/3\n");
			setup_rdp_info(data, NULL, 0, (msgs[i].len) ? msgs[i].buf : NULL,
				       msgs[i].len, &addr);
			rdp_ret = R_RIIC_MasterSend(&data->rdp_info);
		}
		if (i == num_msgs - 1) {
			data->skip_callback = false; /* Invoke callback in the last message */
		}
		if (!rdp_ret) {
			k_sem_take(&data->bus_sync, K_FOREVER);
			k_sem_reset(&data->bus_sync);
		} else {
			break;
		}
	}
	goto transfer_exit;

transfer_blocking:
#endif /* CONFIG_RENESAS_RX_I2C_DTC */
	if (!rdp_ret && !async) {
		data->skip_callback = false; /* Invoke callback */
		k_sem_take(&data->bus_sync, K_FOREVER);
	}
#ifndef CONFIG_RENESAS_RX_I2C_DTC
transfer_exit:
#endif /* CONFIG_RENESAS_RX_I2C_DTC */
	k_sem_give(&data->bus_lock);

	if (!rdp_ret) {
		return 0;
	}
	return -EIO;
}

static int i2c_rx_init(const struct device *dev)
{
	const struct i2c_rx_config *config = dev->config;
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	int ret = 0;

	/* Setup pin control */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Pin control config failed.");
		return ret;
	}

	/* Init kernal object */
	k_sem_init(&data->bus_lock, 1, 1);
	k_sem_init(&data->bus_sync, 0, 1);

	/* Open Device */
	ret = R_RIIC_Open(&data->rdp_info);

	if (ret) {
		LOG_ERR("Open i2c master failed.");
		return -EIO;
	}

	/* Connect and enable Interrupts in zephyr */
	config->irq_config_func(dev);
	return 0;
}

static int i2c_rx_configure(const struct device *dev, uint32_t dev_config)
{
	/* Setup bitrate */
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	uint16_t speed; /* bitrate in kbps */
	int ret;

	/* Validate input */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Only I2C Master mode supported.");
		return -ENOTSUP;
	}
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("Only address 7 bits supported.");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed = 100;
		break;
	case I2C_SPEED_FAST:
		speed = 400;
		break;
	default:
		LOG_ERR("Unsupported speed: %d", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = riic_bps_calc(&data->rdp_info, speed);

	k_sem_give(&data->bus_lock);

	/* Validate result */
	if (ret) {
		return -EINVAL;
	}

	data->dev_config = dev_config;
	return 0;
}

static int i2c_rx_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;

	*dev_config = data->dev_config;
	return 0;
}

static int i2c_rx_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t addr)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;

	data->user_callback = NULL;
	data->user_data = NULL;
	data->skip_callback = true;

	/* Transfer */
	return run_rx_transfer(dev, msgs, num_msgs, addr, false);
}

#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK
static int i2c_rx_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr, i2c_callback_t cb, void *userdata)
{
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;

	data->user_callback = cb;
	data->user_data = userdata;
	data->skip_callback = true;

	/* Transfer */
	return run_rx_transfer(dev, msgs, num_msgs, addr, true);
}
#endif

static int i2c_rx_recover_bus(const struct device *dev)
{
	LOG_DBG("Recover I2C Bus\n");
	struct i2c_rx_data *data = (struct i2c_rx_data *const)dev->data;
	riic_return_t rdp_ret = RIIC_SUCCESS;

	k_sem_take(&data->bus_lock, K_FOREVER);
	rdp_ret |= R_RIIC_Control(&data->rdp_info, RIIC_GEN_START_CON);
	rdp_ret |= R_RIIC_Control(&data->rdp_info, RIIC_GEN_STOP_CON);
	k_sem_give(&data->bus_lock);

	if (rdp_ret != 0) {
		return -EIO;
	}
	return 0;
}

static DEVICE_API(i2c, i2c_rx_driver_api) = {
	.configure = (i2c_api_configure_t)i2c_rx_configure,
	.get_config = (i2c_api_get_config_t)i2c_rx_get_config,
	.transfer = (i2c_api_full_io_t)i2c_rx_transfer,
#ifdef CONFIG_I2C_RENESAS_RX_CALLBACK
	.transfer_cb = (i2c_api_transfer_cb_t)i2c_rx_transfer_cb,
#endif
	.recover_bus = (i2c_api_recover_bus_t)i2c_rx_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* clang-format off */
#ifdef CONFIG_RENESAS_RX_I2C_DTC
#define DTC_DATA_STRUCT_INIT(n)                                                                    \
	.dtc = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), dtc)),                                     \
	.rxi_count = 0,                                                                            \
	.rxi_dtc_activation_irq = DT_INST_IRQ_BY_NAME(n, rxi, irq),                                \
	.rxi_dtc_info =                                                                            \
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
	.txi_count = 0,                                                                            \
	.txi_dtc_activation_irq = DT_INST_IRQ_BY_NAME(n, txi, irq),                                \
	.txi_dtc_info =                                                                            \
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
	.slv_addr = 0x00,                                                                          \
	.msgs = NULL,                                                                              \
	.num_msgs = 0,                                                                             \
	.num_processed_msgs = 0,
#else
#define DTC_DATA_STRUCT_INIT(n)
#endif /* CONFIG_RENESAS_RX_I2C_DTC */
/* clang-format on */

#define I2C_RX_RIIC_INIT(index)                                                                    \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static void i2c_rx_irq_config_func##index(const struct device *dev)                        \
	{                                                                                          \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, eei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, eei, priority), riic_eei_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rxi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, rxi, priority), riic_rxi_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, txi, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, txi, priority), riic_txi_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tei, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, tei, priority), riic_tei_isr,               \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, eei, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, rxi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, txi, irq));                                  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, tei, irq));                                  \
	};                                                                                         \
                                                                                                   \
	static const struct i2c_rx_config i2c_rx_config_##index = {                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_config_func = i2c_rx_irq_config_func##index,                                  \
		.riic_eei_sub = riic##index##_eei_sub,                                             \
		.riic_rxi_sub = riic##index##_rxi_sub,                                             \
		.riic_txi_sub = riic##index##_txi_sub,                                             \
		.riic_tei_sub = riic##index##_tei_sub,                                             \
	};                                                                                         \
                                                                                                   \
	static struct i2c_rx_data i2c_rx_data_##index = {                                          \
		.p_regs = (struct st_riic *)DT_INST_REG_ADDR(index),                               \
		.rdp_info =                                                                        \
			{                                                                          \
				.dev_sts = RIIC_NO_INIT,                                           \
				.ch_no = DT_INST_PROP(index, channel),                             \
				.callbackfunc = rdp_callback,                                      \
			},                                                                         \
		DTC_DATA_STRUCT_INIT(index)};                                                      \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_rx_init, NULL, &i2c_rx_data_##index,                  \
				  &i2c_rx_config_##index, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
				  &i2c_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RX_RIIC_INIT)
