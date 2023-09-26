/*
 * Copyright (c) 2017 Piotr Mienkowski
 * Copyright (c) 2020-2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_i2c_twim

/** @file
 * @brief I2C bus (TWIM) driver for Atmel SAM4L MCU family.
 *
 * I2C Master Mode with 7/10 bit addressing is currently supported.
 * Very long transfers are allowed using NCMDR register. DMA is not
 * yet supported.
 */

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_sam_twim);

#include "i2c-priv.h"

/** I2C bus speed [Hz] in Standard Mode   */
#define BUS_SPEED_STANDARD_HZ         100000U
/** I2C bus speed [Hz] in Fast Mode       */
#define BUS_SPEED_FAST_HZ             400000U
/** I2C bus speed [Hz] in Fast Plus Mode  */
#define BUS_SPEED_PLUS_HZ            1000000U
/** I2C bus speed [Hz] in High Speed Mode */
#define BUS_SPEED_HIGH_HZ            3400000U
/* Maximum value of Clock Divider (CKDIV) */
#define CKDIV_MAX                          7
/* Maximum Frequency prescaled            */
#define F_PRESCALED_MAX                  255

/** Status Clear Register Mask for No Acknowledgements     */
#define TWIM_SCR_NAK_MASK (TWIM_SCR_ANAK | TWIM_SCR_DNAK)
/** Status Register Mask for No Acknowledgements           */
#define TWIM_SR_NAK_MASK  (TWIM_SR_ANAK | TWIM_SR_DNAK)
/** Interrupt Enable Register Mask for No Acknowledgements */
#define TWIM_IER_NAK_MASK (TWIM_IER_ANAK | TWIM_IER_DNAK)
/** Frequently used Interrupt Enable Register Mask         */
#define TWIM_IER_STD_MASK (TWIM_IER_ANAK | TWIM_IER_ARBLST)
/** Frequently used Status Clear Register Mask             */
#define TWIM_SR_STD_MASK  (TWIM_SR_ANAK | TWIM_SR_ARBLST)

/** \internal Max value of NBYTES per transfer by hardware */
#define TWIM_MAX_NBYTES_PER_XFER \
	(TWIM_CMDR_NBYTES_Msk >> TWIM_CMDR_NBYTES_Pos)

#define TWIM_NCMDR_FREE_WAIT            2000

/* Device constant configuration parameters */
struct i2c_sam_twim_dev_cfg {
	Twim *regs;
	void (*irq_config)(void);
	uint32_t bitrate;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	uint8_t irq_id;

	uint8_t std_clk_slew_lim;
	uint8_t std_clk_strength_low;
	uint8_t std_data_slew_lim;
	uint8_t std_data_strength_low;
	uint8_t hs_clk_slew_lim;
	uint8_t hs_clk_strength_high;
	uint8_t hs_clk_strength_low;
	uint8_t hs_data_slew_lim;
	uint8_t hs_data_strength_low;

	uint8_t hs_master_code;
};

/* Device run time data */
struct i2c_sam_twim_dev_data {
	struct k_mutex bus_mutex;
	struct k_sem sem;

	struct i2c_msg *msgs;
	uint32_t msg_cur_idx;
	uint32_t msg_next_idx;
	uint32_t msg_max_idx;

	uint32_t cur_remaining;
	uint32_t cur_idx;
	uint32_t cur_sr;

	uint32_t next_nb_bytes;
	bool next_is_valid;
	bool next_need_rs;
	bool cur_need_rs;
};

static int i2c_clk_set(const struct device *dev, uint32_t speed)
{
	const struct i2c_sam_twim_dev_cfg *const cfg = dev->config;
	Twim *const twim = cfg->regs;
	uint32_t per_clk = SOC_ATMEL_SAM_MCK_FREQ_HZ;
	uint32_t f_prescaled = (per_clk / speed / 2);
	uint32_t cwgr_reg_val = 0;
	uint8_t cwgr_exp = 0;

	/* f_prescaled must fit in 8 bits, cwgr_exp must fit in 3 bits */
	while ((f_prescaled > F_PRESCALED_MAX) && (cwgr_exp <= CKDIV_MAX)) {
		/* increase clock divider */
		cwgr_exp++;
		/* divide f_prescaled value */
		f_prescaled /= 2;
	}

	if (cwgr_exp > CKDIV_MAX) {
		LOG_ERR("Failed to configure I2C clock");
		return -EIO;
	}

	cwgr_reg_val = TWIM_HSCWGR_LOW(f_prescaled / 2) |
		       TWIM_HSCWGR_HIGH(f_prescaled -
					(f_prescaled / 2)) |
		       TWIM_HSCWGR_EXP(cwgr_exp) |
		       TWIM_HSCWGR_DATA(0) |
		       TWIM_HSCWGR_STASTO(f_prescaled);

	/* This configuration should be applied after a TWIM_CR_SWRST
	 *	Set clock waveform generator register
	 */
	if (speed == BUS_SPEED_HIGH_HZ) {
		twim->HSCWGR = cwgr_reg_val;
	} else {
		twim->CWGR = cwgr_reg_val;
	}

	LOG_DBG("per_clk: %d, f_prescaled: %d, cwgr_exp: 0x%02x,"
		"cwgr_reg_val: 0x%08x", per_clk, f_prescaled,
		cwgr_exp, cwgr_reg_val);

	/* Set clock and data slew rate */
	twim->SRR = ((speed == BUS_SPEED_PLUS_HZ) ? TWIM_SRR_FILTER(2) :
						    TWIM_SRR_FILTER(3)) |
		     TWIM_SRR_CLSLEW(cfg->std_clk_slew_lim) |
		     TWIM_SRR_CLDRIVEL(cfg->std_clk_strength_low) |
		     TWIM_SRR_DASLEW(cfg->std_data_slew_lim) |
		     TWIM_SRR_DADRIVEL(cfg->std_data_strength_low);

	twim->HSSRR = TWIM_HSSRR_FILTER(1) |
		      TWIM_HSSRR_CLSLEW(cfg->hs_clk_slew_lim) |
		      TWIM_HSSRR_CLDRIVEH(cfg->hs_clk_strength_high) |
		      TWIM_HSSRR_CLDRIVEL(cfg->hs_clk_strength_low) |
		      TWIM_HSSRR_DASLEW(cfg->hs_data_slew_lim) |
		      TWIM_HSSRR_DADRIVEL(cfg->hs_data_strength_low);

	return 0;
}

static int i2c_sam_twim_configure(const struct device *dev, uint32_t config)
{
	uint32_t bitrate;
	int ret;

	if (!(config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Master Mode is not enabled");
		return -EIO;
	}

	if (config & I2C_ADDR_10_BITS) {
		LOG_ERR("I2C 10-bit addressing is currently not supported");
		LOG_ERR("Please submit a patch");
		return -EIO;
	}

	/* Configure clock */
	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		bitrate = BUS_SPEED_STANDARD_HZ;
		break;
	case I2C_SPEED_FAST:
		bitrate = BUS_SPEED_FAST_HZ;
		break;
	case I2C_SPEED_FAST_PLUS:
		bitrate = BUS_SPEED_PLUS_HZ;
		break;
	case I2C_SPEED_HIGH:
		bitrate = BUS_SPEED_HIGH_HZ;
		break;
	default:
		LOG_ERR("Unsupported I2C speed value");
		return -EIO;
	}

	/* Setup clock waveform */
	ret = i2c_clk_set(dev, bitrate);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void i2c_prepare_xfer_data(struct i2c_sam_twim_dev_data *data)
{
	struct i2c_msg *next_msg = NULL;

	if (data->next_nb_bytes > TWIM_MAX_NBYTES_PER_XFER) {
		data->cur_remaining = TWIM_MAX_NBYTES_PER_XFER;

		data->next_nb_bytes -= TWIM_MAX_NBYTES_PER_XFER;
		data->next_is_valid = true;
		data->next_need_rs = false;
	} else {
		data->cur_remaining = data->next_nb_bytes;

		if ((data->msg_next_idx + 1) < data->msg_max_idx) {
			next_msg = &data->msgs[++data->msg_next_idx];

			data->next_nb_bytes = next_msg->len;
			data->next_is_valid = true;
			data->next_need_rs = true;
		} else {
			data->next_nb_bytes = 0;
			data->next_is_valid = false;
			data->next_need_rs = false;
		}
	}
}

static uint32_t i2c_prepare_xfer_cmd(struct i2c_sam_twim_dev_data *data,
				     uint32_t *cmdr_reg,
				     uint32_t next_msg_idx)
{
	struct i2c_msg *next_msg = &data->msgs[next_msg_idx];
	bool next_msg_is_read;
	uint32_t next_nb_remaining;

	*cmdr_reg &= ~(TWIM_CMDR_NBYTES_Msk |
		       TWIM_CMDR_ACKLAST |
		       TWIM_CMDR_START |
		       TWIM_CMDR_READ);

	next_msg_is_read = ((next_msg->flags & I2C_MSG_RW_MASK) ==
			    I2C_MSG_READ);

	if (next_msg_is_read) {
		*cmdr_reg |= TWIM_CMDR_READ;
	}

	if (data->next_need_rs) {
		/* TODO: evaluate 10 bits repeat start read
		 * because of blank cmd
		 */
		*cmdr_reg |= TWIM_CMDR_START;
	}

	if (data->next_nb_bytes > TWIM_MAX_NBYTES_PER_XFER) {
		next_nb_remaining = TWIM_MAX_NBYTES_PER_XFER;

		if (next_msg_is_read) {
			*cmdr_reg |= TWIM_CMDR_ACKLAST;
		}
	} else {
		next_nb_remaining = data->next_nb_bytes;

		/* Is there any more messages ? */
		if ((next_msg_idx + 1) >= data->msg_max_idx) {
			*cmdr_reg |= TWIM_CMDR_STOP;
		}
	}

	return next_nb_remaining;
}

static void i2c_start_xfer(const struct device *dev, uint16_t daddr)
{
	const struct i2c_sam_twim_dev_cfg *const cfg = dev->config;
	struct i2c_sam_twim_dev_data *data = dev->data;
	struct i2c_msg *msg = &data->msgs[0];
	Twim *const twim = cfg->regs;
	uint32_t cmdr_reg;
	uint32_t data_size;
	uint32_t cur_is_read;

	/* Reset the TWIM module */
	twim->CR = TWIM_CR_MEN;
	twim->CR = TWIM_CR_SWRST;
	twim->CR = TWIM_CR_MDIS;
	twim->IDR = ~0UL;		/* Clear the interrupt flags */
	twim->SCR = ~0UL;		/* Clear the status flags */

	/* Reset indexes */
	data->msg_cur_idx = 0;
	data->msg_next_idx = 0;

	/* pre-load current message to infer next */
	data->next_nb_bytes = data->msgs[data->msg_next_idx].len;
	data->next_is_valid = false;
	data->next_need_rs = false;
	data->cur_remaining = 0;
	data->cur_idx = 0;

	LOG_DBG("Config first/next Transfer: msgs: %d", data->msg_max_idx);

	cmdr_reg = TWIM_CMDR_SADR(daddr) |
		   TWIM_CMDR_VALID;

	if (I2C_SPEED_GET(msg->flags) >= I2C_SPEED_HIGH) {
		cmdr_reg |= TWIM_CMDR_HS |
			    TWIM_CMDR_HSMCODE(cfg->hs_master_code);
	}

	if (msg->flags & I2C_MSG_ADDR_10_BITS) {
		cmdr_reg |= TWIM_CMDR_TENBIT;
	}

	if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ &&
	    (msg->flags & I2C_MSG_ADDR_10_BITS)) {

		/* Fill transfer command (empty)
		 * It must be a write xfer with NBYTES = 0
		 */
		twim->CMDR = cmdr_reg | TWIM_CMDR_START;

		/* Fill next transfer command. REPSAME performs a repeated
		 * start to the same slave address as addressed in the
		 * previous transfer in order to enter master receiver mode.
		 */
		cmdr_reg |= TWIM_CMDR_REPSAME;

		i2c_prepare_xfer_data(data);

		/* Special condition: reset msg_next_idx */
		data->msg_next_idx = 0;

		data_size = i2c_prepare_xfer_cmd(data, &cmdr_reg, 0);
		cmdr_reg |= TWIM_CMDR_NBYTES(data->cur_remaining);
		twim->NCMDR = cmdr_reg | TWIM_CMDR_START;
	} else {
		/* Fill transfer command */
		i2c_prepare_xfer_data(data);

		data_size = i2c_prepare_xfer_cmd(data, &cmdr_reg, 0);
		cmdr_reg |= TWIM_CMDR_NBYTES(data->cur_remaining);
		twim->CMDR = cmdr_reg | TWIM_CMDR_START;

		/* Fill next transfer command */
		if (data->next_is_valid) {
			data_size = i2c_prepare_xfer_cmd(data, &cmdr_reg,
							 data->msg_next_idx);
			cmdr_reg |= TWIM_CMDR_NBYTES(data_size);
			twim->NCMDR = cmdr_reg;
		}
	}

	LOG_DBG("Start Transfer: CMDR: 0x%08x, NCMDR: 0x%08x",
		twim->CMDR, twim->NCMDR);

	/* Extract Read/Write start operation */
	cmdr_reg = twim->CMDR;
	cur_is_read = (cmdr_reg & TWIM_CMDR_READ);

	/* Enable master transfer */
	twim->CR = TWIM_CR_MEN;

	twim->IER = TWIM_IER_STD_MASK |
		    (cur_is_read ? TWIM_IER_RXRDY : TWIM_IER_TXRDY) |
		    TWIM_IER_IDLE;
}

static void i2c_prepare_next(struct i2c_sam_twim_dev_data *data,
			     Twim *const twim)
{
	struct i2c_msg *msg = &data->msgs[data->msg_cur_idx];
	volatile uint32_t ncmdr_wait;
	uint32_t cmdr_reg;
	uint32_t data_size;
	uint32_t cur_is_read;

	if (data->cur_idx == msg->len) {
		data->cur_idx = 0;
		data->msg_cur_idx++;
	}

	i2c_prepare_xfer_data(data);

	/* Sync CMDR with NCMDR before apply changes */
	ncmdr_wait = TWIM_NCMDR_FREE_WAIT;
	while ((twim->NCMDR & TWIM_NCMDR_VALID) && (ncmdr_wait--)) {
		;
	}

	cmdr_reg = twim->CMDR;
	cur_is_read = (cmdr_reg & TWIM_CMDR_READ);
	twim->IER |= (cur_is_read ? TWIM_IER_RXRDY : TWIM_IER_TXRDY);

	/* Is there any more transfer? */
	if (data->next_nb_bytes == 0) {
		return;
	}

	data_size = i2c_prepare_xfer_cmd(data, &cmdr_reg, data->msg_next_idx);
	cmdr_reg |= TWIM_CMDR_NBYTES(data_size);
	twim->NCMDR = cmdr_reg;

	LOG_DBG("ld xfer: NCMDR: 0x%08x", twim->NCMDR);
}

static void i2c_sam_twim_isr(const struct device *dev)
{
	const struct i2c_sam_twim_dev_cfg *const cfg = dev->config;
	struct i2c_sam_twim_dev_data *const data = dev->data;
	Twim *const twim = cfg->regs;
	struct i2c_msg *msg = &data->msgs[data->msg_cur_idx];
	uint32_t isr_status;

	/* Retrieve interrupt status */
	isr_status = twim->SR & twim->IMR;

	LOG_DBG("ISR: IMR: 0x%08x", isr_status);

	/* Not Acknowledged */
	if (isr_status & TWIM_SR_STD_MASK) {
		/*
		 * If we get a NACK, clear the valid bit in CMDR,
		 * otherwise the command will be re-sent.
		 */
		twim->NCMDR &= ~TWIM_NCMDR_VALID;
		twim->CMDR &= ~TWIM_CMDR_VALID;

		data->cur_sr = isr_status;
		goto xfer_comp;
	}

	data->cur_sr = 0;

	/* Byte received */
	if (isr_status & TWIM_SR_RXRDY) {
		msg->buf[data->cur_idx++] = twim->RHR;
		data->cur_remaining--;

		if (data->cur_remaining > 0) {
			goto check_xfer;
		}

		twim->IDR = TWIM_IDR_RXRDY;

		/* Check for next transfer */
		if (data->next_is_valid && data->next_nb_bytes > 0) {
			i2c_prepare_next(data, twim);
		} else {
			data->next_nb_bytes = 0;
		}
	}

	/* Byte sent */
	if (isr_status & TWIM_SR_TXRDY) {

		if (data->cur_idx < msg->len) {
			twim->THR = msg->buf[data->cur_idx++];
			data->cur_remaining--;

			goto check_xfer;
		}

		twim->IDR = TWIM_IDR_TXRDY;

		/* Check for next transfer */
		if (data->next_is_valid && data->next_nb_bytes > 0) {
			i2c_prepare_next(data, twim);
		}
	}

check_xfer:

	/* Is transaction finished ? */
	if (!(isr_status & TWIM_SR_IDLE)) {
		return;
	}

	LOG_DBG("ISR: TWIM_SR_IDLE");

xfer_comp:
	/* Disable all enabled interrupts */
	twim->IDR = ~0UL;

	/* Clear all status */
	twim->SCR = ~0UL;

	/* We are done */
	k_sem_give(&data->sem);
}

static int i2c_sam_twim_transfer(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	struct i2c_sam_twim_dev_data *data = dev->data;
	int ret = 0;

	/* Send out messages */
	k_mutex_lock(&data->bus_mutex, K_FOREVER);

	/* Load messages */
	data->msgs = msgs;
	data->msg_max_idx = num_msgs;

	i2c_start_xfer(dev, addr);

	/* Wait for the message transfer to complete */
	k_sem_take(&data->sem, K_FOREVER);

	if (data->cur_sr & TWIM_SR_STD_MASK) {
		ret = -EIO;

		LOG_INF("MSG: %d, ANAK: %d, ARBLST: %d",
			data->msg_cur_idx,
			(data->cur_sr & TWIM_SR_ANAK) > 0,
			(data->cur_sr & TWIM_SR_ARBLST) > 0);
	}

	k_mutex_unlock(&data->bus_mutex);

	return ret;
}

static int i2c_sam_twim_initialize(const struct device *dev)
{
	const struct i2c_sam_twim_dev_cfg *const cfg = dev->config;
	struct i2c_sam_twim_dev_data *data = dev->data;
	Twim *const twim = cfg->regs;
	uint32_t bitrate_cfg;
	int ret;

	/* Configure interrupts */
	cfg->irq_config();

	/*
	 * initialize mutex. it is used when multiple transfers are taking
	 * place to guarantee that each one is atomic and has exclusive access
	 * to the I2C bus.
	 */
	k_mutex_init(&data->bus_mutex);

	/* Initialize semaphore */
	k_sem_init(&data->sem, 0, 1);

	/* Connect pins to the peripheral */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Enable TWIM clock in PM */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	/* Enable the module*/
	twim->CR = TWIM_CR_MEN;

	/* Reset the module */
	twim->CR |= TWIM_CR_SWRST;

	/* Clear SR */
	twim->SCR = ~0UL;

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_sam_twim_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to initialize %s device", dev->name);
		return ret;
	}

	/* Enable module's IRQ */
	irq_enable(cfg->irq_id);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct i2c_driver_api i2c_sam_twim_driver_api = {
	.configure = i2c_sam_twim_configure,
	.transfer = i2c_sam_twim_transfer,
};

#define I2C_TWIM_SAM_SLEW_REGS(n)					\
	.std_clk_slew_lim = DT_INST_ENUM_IDX(n, std_clk_slew_lim),	\
	.std_clk_strength_low = DT_INST_ENUM_IDX(n, std_clk_strength_low),\
	.std_data_slew_lim = DT_INST_ENUM_IDX(n, std_data_slew_lim),	\
	.std_data_strength_low = DT_INST_ENUM_IDX(n, std_data_strength_low),\
	.hs_clk_slew_lim = DT_INST_ENUM_IDX(n, hs_clk_slew_lim),	\
	.hs_clk_strength_high = DT_INST_ENUM_IDX(n, hs_clk_strength_high),\
	.hs_clk_strength_low = DT_INST_ENUM_IDX(n, hs_clk_strength_low),\
	.hs_data_slew_lim = DT_INST_ENUM_IDX(n, hs_data_slew_lim),	\
	.hs_data_strength_low = DT_INST_ENUM_IDX(n, hs_data_strength_low)

#define I2C_TWIM_SAM_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void i2c##n##_sam_irq_config(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    i2c_sam_twim_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
	}								\
									\
	static const struct i2c_sam_twim_dev_cfg i2c##n##_sam_config = {\
		.regs = (Twim *)DT_INST_REG_ADDR(n),			\
		.irq_config = i2c##n##_sam_irq_config,			\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		.irq_id = DT_INST_IRQN(n),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		.hs_master_code = DT_INST_ENUM_IDX(n, hs_master_code),	\
		I2C_TWIM_SAM_SLEW_REGS(n),				\
	};								\
									\
	static struct i2c_sam_twim_dev_data i2c##n##_sam_data;		\
									\
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_sam_twim_initialize,		\
			    NULL,					\
			    &i2c##n##_sam_data, &i2c##n##_sam_config,	\
			    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,	\
			    &i2c_sam_twim_driver_api)

DT_INST_FOREACH_STATUS_OKAY(I2C_TWIM_SAM_INIT);
