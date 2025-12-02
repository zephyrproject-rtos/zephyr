/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_enhance_i2c

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <errno.h>
#include <soc.h>
#include <soc_dt.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ite_enhance, CONFIG_I2C_LOG_LEVEL);
#include "i2c_bitbang.h"
#include "i2c-priv.h"

/* Start smbus session from idle state */
#define I2C_MSG_START    BIT(5)
#define I2C_MSG_W2R_MASK (I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP)

#define I2C_LINE_SCL_HIGH BIT(0)
#define I2C_LINE_SDA_HIGH BIT(1)
#define I2C_LINE_IDLE (I2C_LINE_SCL_HIGH | I2C_LINE_SDA_HIGH)

#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
/* Reserved 5 bytes for ID and CMD_x. */
#define I2C_CQ_MODE_TX_MAX_PAYLOAD_SIZE  (CONFIG_I2C_CQ_MODE_MAX_PAYLOAD_SIZE - 5)

/* Repeat Start. */
#define I2C_CQ_CMD_L_RS BIT(7)
/*
 * R/W (Read/ Write) decides the I2C read or write direction.
 * 1: read, 0: write
 */
#define I2C_CQ_CMD_L_RW BIT(6)
/* P (STOP) is the I2C STOP condition. */
#define I2C_CQ_CMD_L_P  BIT(5)
/* E (End) is this device end flag. */
#define I2C_CQ_CMD_L_E  BIT(4)
/* LA (Last ACK) is Last ACK in master receiver. */
#define I2C_CQ_CMD_L_LA BIT(3)
/* bit[2:0] are number of transfer out or receive data which depends on R/W. */
#define I2C_CQ_CMD_L_NUM_BIT_2_0 GENMASK(2, 0)

struct i2c_cq_packet {
	uint8_t id;
	uint8_t cmd_l;
	uint8_t cmd_h;
	FLEXIBLE_ARRAY_DECLARE(uint8_t, wdata);
};
#endif /* CONFIG_I2C_IT8XXX2_CQ_MODE */

struct i2c_enhance_config {
	void (*irq_config_func)(void);
	uint32_t bitrate;
	uint8_t *base;
	uint8_t i2c_irq_base;
	uint8_t port;
	uint8_t channel_switch_sel;
	/* SCL GPIO cells */
	struct gpio_dt_spec scl_gpios;
	/* SDA GPIO cells */
	struct gpio_dt_spec sda_gpios;
	/* I2C alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	uint8_t prescale_scl_low;
	uint8_t data_hold_time;
	uint32_t clock_gate_offset;
	int transfer_timeout_ms;
	bool target_enable;
	bool target_pio_mode;
	bool push_pull_recovery;
};

enum i2c_pin_fun {
	SCL = 0,
	SDA,
};

enum i2c_ch_status {
	I2C_CH_NORMAL = 0,
	I2C_CH_REPEAT_START,
	I2C_CH_WAIT_READ,
	I2C_CH_WAIT_NEXT_XFER,
};

#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
struct i2c_host_cq_buffer {
	/* Command queue tx payload. */
	uint8_t i2c_cq_mode_tx_dlm[CONFIG_I2C_CQ_MODE_MAX_PAYLOAD_SIZE] __aligned(4);
	/* Command queue rx payload. */
	uint8_t i2c_cq_mode_rx_dlm[CONFIG_I2C_CQ_MODE_MAX_PAYLOAD_SIZE] __aligned(4);
};
#endif
#ifdef CONFIG_I2C_TARGET
/*
 * When accessing data exceeds the maximum buffer, the actual reload address
 * is one byte more than the maximum buffer size. Therefore, it is necessary to
 * have a buffer in place to prevent overwriting other memory.
 */
#define PROTECT_MEM_BUF 4
struct i2c_target_dma_buffer {
	/* Target mode DMA output buffer. */
	uint8_t __aligned(4)
		out_buffer[CONFIG_I2C_TARGET_IT8XXX2_MAX_BUF_SIZE + PROTECT_MEM_BUF];
	/* Target mode DMA input buffer. */
	uint8_t __aligned(4)
		in_buffer[CONFIG_I2C_TARGET_IT8XXX2_MAX_BUF_SIZE + PROTECT_MEM_BUF];
};
#endif

struct i2c_enhance_data {
	enum i2c_ch_status i2ccs;
	struct i2c_msg *active_msg;
	struct k_mutex mutex;
	struct k_sem device_sync_sem;
	struct i2c_bitbang bitbang;
	struct gpio_callback gpio_wui_scl_cb;
	struct gpio_callback gpio_wui_sda_cb;
	/* Index into output data */
	size_t widx;
	/* Index into input data */
	size_t ridx;
	/* operation freq of i2c */
	uint32_t bus_freq;
	/* Error code, if any */
	uint32_t err;
	/* address of device */
	uint16_t addr_16bit;
	/* wait for stop bit interrupt */
	uint8_t stop;
	/* Number of messages. */
	uint8_t num_msgs;
	/* NACK */
	bool nack;
#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
	/* Store command queue mode messages. */
	struct i2c_msg *cq_msgs;
#endif
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_cfg;
	uint32_t buffer_size;
	int target_nack;
	bool target_attached;
#endif
	union {
#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
		struct i2c_host_cq_buffer host_buffer;
#endif
#ifdef CONFIG_I2C_TARGET
		struct i2c_target_dma_buffer target_buffer;
#endif
	};
};

enum enhanced_i2c_transfer_direct {
	TX_DIRECT,
	RX_DIRECT,
};

enum enhanced_i2c_ctl {
	/* Hardware reset */
	E_HW_RST = 0x01,
	/* Stop */
	E_STOP = 0x02,
	/* Start & Repeat start */
	E_START = 0x04,
	/* Acknowledge */
	E_ACK = 0x08,
	/* State reset */
	E_STS_RST = 0x10,
	/* Mode select */
	E_MODE_SEL = 0x20,
	/* I2C interrupt enable */
	E_INT_EN = 0x40,
	/* 0 : Standard mode , 1 : Receive mode */
	E_RX_MODE = 0x80,
	/* State reset and hardware reset */
	E_STS_AND_HW_RST = (E_STS_RST | E_HW_RST),
	/* Generate start condition and transmit slave address */
	E_START_ID = (E_INT_EN | E_MODE_SEL | E_ACK | E_START | E_HW_RST),
	/* Generate stop condition */
	E_FINISH = (E_INT_EN | E_MODE_SEL | E_ACK | E_STOP | E_HW_RST),
	/* Start with command queue mode */
	E_START_CQ = (E_INT_EN | E_MODE_SEL | E_ACK | E_START),
};

enum enhanced_i2c_host_status {
	/* ACK receive */
	E_HOSTA_ACK = 0x01,
	/* Interrupt pending */
	E_HOSTA_INTP = 0x02,
	/* Read/Write */
	E_HOSTA_RW = 0x04,
	/* Time out error */
	E_HOSTA_TMOE = 0x08,
	/* Arbitration lost */
	E_HOSTA_ARB = 0x10,
	/* Bus busy */
	E_HOSTA_BB = 0x20,
	/* Address match */
	E_HOSTA_AM = 0x40,
	/* Byte done status */
	E_HOSTA_BDS = 0x80,
	/* time out or lost arbitration */
	E_HOSTA_ANY_ERROR = (E_HOSTA_TMOE | E_HOSTA_ARB),
	/* Byte transfer done and ACK receive */
	E_HOSTA_BDS_AND_ACK = (E_HOSTA_BDS | E_HOSTA_ACK),
};

enum i2c_reset_cause {
	I2C_RC_NO_IDLE_FOR_START = 1,
	I2C_RC_TIMEOUT,
};

#ifdef CONFIG_I2C_TARGET
enum enhanced_i2c_target_status {
	/* Time out error */
	E_TARGET_TMOE = 0x08,
	/* Arbitration lost */
	E_TARGET_ARB = 0x10,
	/* Time out or lost arbitration */
	E_TARGET_ANY_ERROR = (E_TARGET_TMOE | E_TARGET_ARB),
};
#endif

static int i2c_parsing_return_value(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;

	if (!data->err) {
		return 0;
	}

	/* Connection timed out */
	if (data->err == ETIMEDOUT) {
		return -ETIMEDOUT;
	}

	/* The device does not respond ACK */
	if (data->err == E_HOSTA_ACK) {
		return -ENXIO;
	} else {
		return -EIO;
	}
}

static int i2c_get_line_levels(const struct device *dev)
{
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;
	int pin_sts = 0;

	if (IT8XXX2_I2C_TOS(base) & IT8XXX2_I2C_SCL_IN) {
		pin_sts |= I2C_LINE_SCL_HIGH;
	}

	if (IT8XXX2_I2C_TOS(base) & IT8XXX2_I2C_SDA_IN) {
		pin_sts |= I2C_LINE_SDA_HIGH;
	}

	return pin_sts;
}

static int i2c_is_busy(const struct device *dev)
{
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;

	return (IT8XXX2_I2C_STR(base) & E_HOSTA_BB);
}

static int i2c_bus_not_available(const struct device *dev)
{
	if (i2c_is_busy(dev) ||
		(i2c_get_line_levels(dev) != I2C_LINE_IDLE)) {
		return -EIO;
	}

	return 0;
}

static void i2c_reset(const struct device *dev)
{
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;

	/* State reset and hardware reset */
	IT8XXX2_I2C_CTR(base) = E_STS_AND_HW_RST;
}

/* Set clock frequency for i2c port D, E , or F */
static void i2c_enhanced_port_set_frequency(const struct device *dev,
					    int freq_hz)
{
	const struct i2c_enhance_config *config = dev->config;
	uint32_t clk_div, psr, pll_clock, psr_h, psr_l;
	uint8_t *base = config->base;
	uint8_t prescale_scl = config->prescale_scl_low;

	pll_clock = chip_get_pll_freq();
	/*
	 * Let psr(Prescale) = IT8XXX2_I2C_PSR(p_ch)
	 * Then, 1 SCL cycle = 2 x (psr + 2) x SMBus clock cycle
	 * SMBus clock = pll_clock / clk_div
	 * SMBus clock cycle = 1 / SMBus clock
	 * 1 SCL cycle = 1 / freq
	 * 1 / freq = 2 x (psr + 2) x (1 / (pll_clock / clk_div))
	 * psr = ((pll_clock / clk_div) x (1 / freq) x (1 / 2)) - 2
	 */
	if (freq_hz) {
		/* Get SMBus clock divide value */
		clk_div = (IT8XXX2_ECPM_SCDCR2 & 0x0F) + 1U;
		/* Calculate PSR value */
		psr = (pll_clock / (clk_div * (2U * freq_hz))) - 2U;
		/* Set psr value under 0xFD */
		if (psr > 0xFD) {
			psr = 0xFD;
		}

		/* Adjust SCL low period prescale */
		psr_l = psr + prescale_scl;
		if (psr_l > 0xFD) {
			psr_l = 0xFD;
			LOG_WRN("(psr + prescale_scl) can not be greater than 0xfd.");
		}

		/*
		 * Adjust SCL high period prescale
		 * The property setting prescale_scl must be less than psr and
		 * the minimum value of psr_h is 2.
		 */
		if (psr > (prescale_scl + 2)) {
			psr_h = psr - prescale_scl;
		} else {
			psr_h = 2;
			LOG_WRN("prescale_scl_low should be less than (psr - 2).");
		}

		/* Set I2C Speed for SCL low period. */
		IT8XXX2_I2C_PSR(base) = psr_l & 0xFF;
		/* Set I2C Speed for SCL high period. */
		IT8XXX2_I2C_HSPR(base) = psr_h & 0xFF;
	}

}

static int i2c_enhance_configure(const struct device *dev,
				 uint32_t dev_config_raw)
{
	const struct i2c_enhance_config *config = dev->config;
	struct i2c_enhance_data *const data = dev->data;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	data->bus_freq = I2C_SPEED_GET(dev_config_raw);

	i2c_enhanced_port_set_frequency(dev, config->bitrate);

	return 0;
}

static int i2c_enhance_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_enhance_data *const data = dev->data;
	uint32_t speed;

	if (!data->bus_freq) {
		LOG_ERR("The bus frequency is not initially configured.");
		return -EIO;
	}

	switch (data->bus_freq) {
	case I2C_SPEED_DT:
	case I2C_SPEED_STANDARD:
	case I2C_SPEED_FAST:
	case I2C_SPEED_FAST_PLUS:
		speed = I2C_SPEED_SET(data->bus_freq);
		break;
	default:
		return -ERANGE;
	}

	*dev_config = (I2C_MODE_CONTROLLER | speed);

	return 0;
}

static int enhanced_i2c_error(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;
	uint32_t i2c_str = IT8XXX2_I2C_STR(base);

	if (i2c_str & E_HOSTA_ANY_ERROR) {
		data->err = i2c_str & E_HOSTA_ANY_ERROR;
	/* device does not respond ACK */
	} else if ((i2c_str & E_HOSTA_BDS_AND_ACK) == E_HOSTA_BDS) {
		if (IT8XXX2_I2C_CTR(base) & E_ACK) {
			data->err = E_HOSTA_ACK;
			data->nack = true;
			/* STOP */
			IT8XXX2_I2C_CTR(base) = E_FINISH;
		}
	}

	return data->err;
}

static void enhanced_i2c_start(const struct device *dev)
{
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;

	/* reset i2c port */
	i2c_reset(dev);
	/* Set i2c frequency */
	i2c_enhanced_port_set_frequency(dev, config->bitrate);
	/*
	 * Set time out register.
	 * I2C D/E/F clock/data low timeout.
	 */
	IT8XXX2_I2C_TOR(base) = I2C_CLK_LOW_TIMEOUT;
	/* bit1: Enable enhanced i2c module */
	IT8XXX2_I2C_CTR1(base) = IT8XXX2_I2C_MDL_EN;
}

static void i2c_pio_trans_data(const struct device *dev,
			       enum enhanced_i2c_transfer_direct direct,
			       uint16_t trans_data, int first_byte)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;
	uint32_t nack = 0;

	if (first_byte) {
		/* First byte must be slave address. */
		IT8XXX2_I2C_DTR(base) = trans_data |
					(direct == RX_DIRECT ? BIT(0) : 0);
		/* start or repeat start signal. */
		IT8XXX2_I2C_CTR(base) = E_START_ID;
	} else {
		if (direct == TX_DIRECT) {
			/* Transmit data */
			IT8XXX2_I2C_DTR(base) = (uint8_t)trans_data;
		} else {
			/*
			 * Receive data.
			 * Last byte should be NACK in the end of read cycle
			 */
			if (((data->ridx + 1) == data->active_msg->len) &&
				(data->active_msg->flags & I2C_MSG_STOP)) {
				nack = 1;
			}
		}
		/* Set hardware reset to start next transmission */
		IT8XXX2_I2C_CTR(base) = E_INT_EN | E_MODE_SEL |
				E_HW_RST | (nack ? 0 : E_ACK);
	}
}

static int enhanced_i2c_tran_read(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;
	uint8_t in_data = 0;

	if (data->active_msg->flags & I2C_MSG_START) {
		/* clear start flag */
		data->active_msg->flags &= ~I2C_MSG_START;
		enhanced_i2c_start(dev);
		/* Direct read  */
		data->i2ccs = I2C_CH_WAIT_READ;
		/* Send ID */
		i2c_pio_trans_data(dev, RX_DIRECT, data->addr_16bit << 1, 1);
	} else {
		if (data->i2ccs) {
			if (data->i2ccs == I2C_CH_WAIT_READ) {
				data->i2ccs = I2C_CH_NORMAL;
				/* Receive data */
				i2c_pio_trans_data(dev, RX_DIRECT, in_data, 0);

			/* data->active_msg->flags == I2C_MSG_RESTART */
			} else {
				/* Write to read */
				data->i2ccs = I2C_CH_WAIT_READ;
				/* Send ID */
				i2c_pio_trans_data(dev, RX_DIRECT,
					data->addr_16bit << 1, 1);
			}
		} else {
			if (data->ridx < data->active_msg->len) {
				/* read data */
				*(data->active_msg->buf++) = IT8XXX2_I2C_DRR(base);
				data->ridx++;
				/* done */
				if (data->ridx == data->active_msg->len) {
					data->active_msg->len = 0;
					if (data->active_msg->flags & I2C_MSG_STOP) {
						data->i2ccs = I2C_CH_NORMAL;
						IT8XXX2_I2C_CTR(base) = E_FINISH;
						/* wait for stop bit interrupt */
						data->stop = 1;
						return 1;
					}
					/* End the transaction */
					data->i2ccs = I2C_CH_WAIT_READ;
					return 0;
				}
				/* read next byte */
				i2c_pio_trans_data(dev, RX_DIRECT, in_data, 0);
			} else if (data->active_msg->len == 0) {
				/* Handle data length of 0 */
				data->i2ccs = I2C_CH_NORMAL;
				IT8XXX2_I2C_CTR(base) = E_FINISH;
				/* wait for stop bit interrupt */
				data->stop = 1;
			}
		}
	}
	return 1;
}

static int enhanced_i2c_tran_write(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;
	uint8_t out_data;

	if (data->active_msg->flags & I2C_MSG_START) {
		/* Clear start bit */
		data->active_msg->flags &= ~I2C_MSG_START;
		enhanced_i2c_start(dev);
		/* Send ID */
		i2c_pio_trans_data(dev, TX_DIRECT, data->addr_16bit << 1, 1);
	} else {
		/* Host has completed the transmission of a byte */
		if (data->widx < data->active_msg->len) {
			out_data = *(data->active_msg->buf++);
			data->widx++;

			/* Send Byte */
			i2c_pio_trans_data(dev, TX_DIRECT, out_data, 0);
			if (data->i2ccs == I2C_CH_WAIT_NEXT_XFER) {
				data->i2ccs = I2C_CH_NORMAL;
			}
		} else {
			/* done */
			data->active_msg->len = 0;
			if (data->active_msg->flags & I2C_MSG_STOP) {
				IT8XXX2_I2C_CTR(base) = E_FINISH;
				/* wait for stop bit interrupt */
				data->stop = 1;
			} else {
				/* Direct write with direct read */
				data->i2ccs = I2C_CH_WAIT_NEXT_XFER;
				return 0;
			}
		}
	}
	return 1;
}

static int i2c_transaction(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;

	/* no error */
	if (!(enhanced_i2c_error(dev))) {
		if (!data->stop) {
			/*
			 * The return value indicates if there is more data
			 * to be read or written. If the return value = 1,
			 * it means that the interrupt cannot be disable and
			 * continue to transmit data.
			 */
			if (data->active_msg->flags & I2C_MSG_READ) {
				return enhanced_i2c_tran_read(dev);
			} else {
				return enhanced_i2c_tran_write(dev);
			}
		}
	}

	/*
	 * When a transaction results in NACK, ensure that the IT8XXX2_I2C_CTR
	 * register has been updated E_FINISH before proceeding with the
	 * following i2c_reset.
	 */
	if (data->nack) {
		data->nack = false;
		data->stop = 1;

		return 1;
	}

	/* reset i2c port */
	i2c_reset(dev);
	IT8XXX2_I2C_CTR1(base) = 0;

	data->stop = 0;
	/* done doing work */
	return 0;
}

static int i2c_enhance_pio_transfer(const struct device *dev,
				    struct i2c_msg *msgs)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	int res;

	if (data->i2ccs == I2C_CH_NORMAL) {
		struct i2c_msg *start_msg = &msgs[0];

		start_msg->flags |= I2C_MSG_START;
	}

	for (int i = 0; i < data->num_msgs; i++) {

		data->widx = 0;
		data->ridx = 0;
		data->err = 0;
		data->active_msg = &msgs[i];

		/*
		 * Start transaction.
		 * The return value indicates if the initial configuration
		 * of I2C transaction for read or write has been completed.
		 */
		if (i2c_transaction(dev)) {
			/* Enable I2C interrupt. */
			irq_enable(config->i2c_irq_base);
		}
		/* Wait for the transfer to complete */
		res = k_sem_take(&data->device_sync_sem, K_MSEC(config->transfer_timeout_ms));
		/*
		 * The irq will be enabled at the condition of start or
		 * repeat start of I2C. If timeout occurs without being
		 * wake up during suspend(ex: interrupt is not fired),
		 * the irq should be disabled immediately.
		 */
		irq_disable(config->i2c_irq_base);
		/*
		 * The transaction is dropped on any error(timeout, NACK, fail,
		 * bus error, device error).
		 */
		if (data->err) {
			break;
		}

		if (res != 0) {
			data->err = ETIMEDOUT;
			/* reset i2c port */
			i2c_reset(dev);
			LOG_ERR("I2C ch%d:0x%X reset cause %d",
				config->port, data->addr_16bit, I2C_RC_TIMEOUT);
			/* If this message is sent fail, drop the transaction. */
			break;
		}
	}

	/* reset i2c channel status */
	if (data->err || (data->active_msg->flags & I2C_MSG_STOP)) {
		data->i2ccs = I2C_CH_NORMAL;
	}
	/* Clear the flag */
	data->nack = false;

	return data->err;
}

#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
static void enhanced_i2c_set_cmd_addr_regs(const struct device *dev)
{
	const struct i2c_enhance_config *config = dev->config;
	struct i2c_enhance_data *data = dev->data;
	struct i2c_host_cq_buffer *host_buffer = &data->host_buffer;
	uint32_t dlm_base;
	uint8_t *base = config->base;

	/* Set "Address Register" to store the I2C data. */
	dlm_base = (uint32_t)host_buffer->i2c_cq_mode_rx_dlm & 0xffffff;
	IT8XXX2_I2C_RAMH2A(base) = (dlm_base >> 16) & 0xff;
	IT8XXX2_I2C_RAMHA(base) = (dlm_base >> 8) & 0xff;
	IT8XXX2_I2C_RAMLA(base) = dlm_base & 0xff;

	/* Set "Command Address Register" to get commands. */
	dlm_base = (uint32_t)host_buffer->i2c_cq_mode_tx_dlm & 0xffffff;
	IT8XXX2_I2C_CMD_ADDH2(base) = (dlm_base >> 16) & 0xff;
	IT8XXX2_I2C_CMD_ADDH(base) = (dlm_base >> 8) & 0xff;
	IT8XXX2_I2C_CMD_ADDL(base) = dlm_base & 0xff;
}

static void enhanced_i2c_cq_write(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	struct i2c_host_cq_buffer *host_buffer = &data->host_buffer;
	struct i2c_cq_packet *i2c_cq_pckt;
	uint8_t num_bit_2_0 = (data->cq_msgs[0].len - 1) & I2C_CQ_CMD_L_NUM_BIT_2_0;
	uint8_t num_bit_10_3 = ((data->cq_msgs[0].len - 1) >> 3) & 0xff;

	i2c_cq_pckt = (struct i2c_cq_packet *)host_buffer->i2c_cq_mode_tx_dlm;
	/* Set commands in RAM. */
	i2c_cq_pckt->id = data->addr_16bit << 1;
	i2c_cq_pckt->cmd_l = I2C_CQ_CMD_L_P | I2C_CQ_CMD_L_E | num_bit_2_0;
	i2c_cq_pckt->cmd_h = num_bit_10_3;
	for (int i = 0; i < data->cq_msgs[0].len; i++) {
		i2c_cq_pckt->wdata[i] = data->cq_msgs[0].buf[i];
	}
}

static void enhanced_i2c_cq_read(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	struct i2c_host_cq_buffer *host_buffer = &data->host_buffer;
	struct i2c_cq_packet *i2c_cq_pckt;
	uint8_t num_bit_2_0 = (data->cq_msgs[0].len - 1) & I2C_CQ_CMD_L_NUM_BIT_2_0;
	uint8_t num_bit_10_3 = ((data->cq_msgs[0].len - 1) >> 3) & 0xff;

	i2c_cq_pckt = (struct i2c_cq_packet *)host_buffer->i2c_cq_mode_tx_dlm;
	/* Set commands in RAM. */
	i2c_cq_pckt->id = data->addr_16bit << 1;
	i2c_cq_pckt->cmd_l = I2C_CQ_CMD_L_RW | I2C_CQ_CMD_L_P |
				I2C_CQ_CMD_L_E | num_bit_2_0;
	i2c_cq_pckt->cmd_h = num_bit_10_3;
}

static void enhanced_i2c_cq_write_to_read(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	struct i2c_host_cq_buffer *host_buffer = &data->host_buffer;
	struct i2c_cq_packet *i2c_cq_pckt;
	uint8_t num_bit_2_0 = (data->cq_msgs[0].len - 1) & I2C_CQ_CMD_L_NUM_BIT_2_0;
	uint8_t num_bit_10_3 = ((data->cq_msgs[0].len - 1) >> 3) & 0xff;
	int i;

	i2c_cq_pckt = (struct i2c_cq_packet *)host_buffer->i2c_cq_mode_tx_dlm;
	/* Set commands in RAM. (command byte for write) */
	i2c_cq_pckt->id = data->addr_16bit << 1;
	i2c_cq_pckt->cmd_l = num_bit_2_0;
	i2c_cq_pckt->cmd_h = num_bit_10_3;
	for (i = 0; i < data->cq_msgs[0].len; i++) {
		i2c_cq_pckt->wdata[i] = data->cq_msgs[0].buf[i];
	}

	/* Set commands in RAM. (command byte for read) */
	num_bit_2_0 = (data->cq_msgs[1].len - 1) & I2C_CQ_CMD_L_NUM_BIT_2_0;
	num_bit_10_3 = ((data->cq_msgs[1].len - 1) >> 3) & 0xff;
	i2c_cq_pckt->wdata[i++] = I2C_CQ_CMD_L_RS | I2C_CQ_CMD_L_RW |
				I2C_CQ_CMD_L_P | I2C_CQ_CMD_L_E | num_bit_2_0;
	i2c_cq_pckt->wdata[i] = num_bit_10_3;
}

static int enhanced_i2c_cq_isr(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	struct i2c_host_cq_buffer *host_buffer = &data->host_buffer;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;

	/* Device 1 finish IRQ. */
	if (IT8XXX2_I2C_FST(base) & IT8XXX2_I2C_FST_DEV1_IRQ) {
		uint8_t msgs_idx = data->num_msgs - 1;

		/* Get data if this is a read transaction. */
		if (data->cq_msgs[msgs_idx].flags & I2C_MSG_READ) {
			for (int i = 0; i < data->cq_msgs[msgs_idx].len; i++) {
				data->cq_msgs[msgs_idx].buf[i] =
				host_buffer->i2c_cq_mode_rx_dlm[i];
			}
		}
	} else {
		/* Device 1 error have occurred. eg. nack, timeout... */
		if (IT8XXX2_I2C_NST(base) & IT8XXX2_I2C_NST_ID_NACK) {
			data->err = E_HOSTA_ACK;
		} else {
			data->err = IT8XXX2_I2C_STR(base) &
					E_HOSTA_ANY_ERROR;
		}
	}
	/* Reset bus. */
	IT8XXX2_I2C_CTR(base) = E_STS_AND_HW_RST;
	IT8XXX2_I2C_CTR1(base) = 0;

	return 0;
}

static int enhanced_i2c_cmd_queue_trans(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;

	/* State reset and hardware reset. */
	IT8XXX2_I2C_CTR(base) = E_STS_AND_HW_RST;
	/* Set "PSR" registers to decide the i2c speed. */
	i2c_enhanced_port_set_frequency(dev, config->bitrate);
	/* Set time out register. port D, E, or F clock/data low timeout. */
	IT8XXX2_I2C_TOR(base) = I2C_CLK_LOW_TIMEOUT;

	if (data->num_msgs == 2) {
		/* I2C write to read of command queue mode. */
		enhanced_i2c_cq_write_to_read(dev);
	} else {
		/* I2C read of command queue mode. */
		if (data->cq_msgs[0].flags & I2C_MSG_READ) {
			enhanced_i2c_cq_read(dev);
		/* I2C write of command queue mode. */
		} else {
			enhanced_i2c_cq_write(dev);
		}
	}

	/* Enable i2c module with command queue mode. */
	IT8XXX2_I2C_CTR1(base) = IT8XXX2_I2C_MDL_EN | IT8XXX2_I2C_COMQ_EN;
	/* One shot on device 1. */
	IT8XXX2_I2C_MODE_SEL(base) = 0;
	IT8XXX2_I2C_CTR2(base) = 1;
	/*
	 * The EC processor(CPU) cannot be in the k_cpu_idle() during the
	 * transactions with the CQ mode(DMA mode).
	 * Otherwise, the EC processor would be clock gated.
	 */
	chip_block_idle();
	/* Start */
	IT8XXX2_I2C_CTR(base) = E_START_CQ;

	return 1;
}

static int i2c_enhance_cq_transfer(const struct device *dev,
				   struct i2c_msg *msgs)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	int res = 0;

	data->err = 0;
	data->cq_msgs = msgs;

	/* Start transaction */
	if (enhanced_i2c_cmd_queue_trans(dev)) {
		/* Enable i2c interrupt */
		irq_enable(config->i2c_irq_base);
	}
	/* Wait for the transfer to complete */
	res = k_sem_take(&data->device_sync_sem, K_MSEC(config->transfer_timeout_ms));

	irq_disable(config->i2c_irq_base);

	if (res != 0) {
		data->err = ETIMEDOUT;
		/* Reset i2c port. */
		i2c_reset(dev);
		LOG_ERR("I2C ch%d:0x%X reset cause %d",
			config->port, data->addr_16bit, I2C_RC_TIMEOUT);
	}

	/* Permit to enter idle mode. */
	chip_permit_idle();

	return data->err;
}

static bool cq_mode_allowed(const struct device *dev, struct i2c_msg *msgs)
{
	struct i2c_enhance_data *data = dev->data;

	/*
	 * If the transaction of write or read is divided into two
	 * transfers(not two messages), the command queue mode does
	 * not support.
	 */
	if (data->i2ccs != I2C_CH_NORMAL) {
		return false;
	}
	/*
	 * When there is only one message, use the command queue transfer
	 * directly.
	 */
	if (data->num_msgs == 1 && (msgs[0].flags & I2C_MSG_STOP)) {
		/* Read transfer payload too long, use PIO mode */
		if (((msgs[0].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) &&
		    (msgs[0].len > CONFIG_I2C_CQ_MODE_MAX_PAYLOAD_SIZE)) {
			return false;
		}
		/* Write transfer payload too long, use PIO mode */
		if (((msgs[0].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) &&
		    (msgs[0].len > I2C_CQ_MODE_TX_MAX_PAYLOAD_SIZE)) {
			return false;
		}
		/*
		 * Use PIO mode when no data is written and read, such as in the
		 * case of cmd_i2c_scan.
		 */
		if (msgs[0].len == 0) {
			return false;
		}
		return true;
	}
	/*
	 * When there are two messages, we need to judge whether or not there
	 * is I2C_MSG_RESTART flag from the second message, and then decide to
	 * do the command queue or PIO mode transfer.
	 */
	if (data->num_msgs == 2) {
		/*
		 * The first of two messages must be write.
		 * If the length of write to read transfer is greater than
		 * command queue payload size, there will execute PIO mode.
		 */
		if (((msgs[0].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) &&
		    (msgs[0].len <= I2C_CQ_MODE_TX_MAX_PAYLOAD_SIZE)) {
			/*
			 * The transfer is i2c_burst_read().
			 *
			 * e.g. msg[0].flags = I2C_MSG_WRITE;
			 *      msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ |
			 *                     I2C_MSG_STOP;
			 */
			if (((msgs[1].flags & I2C_MSG_W2R_MASK) == I2C_MSG_W2R_MASK) &&
			    (msgs[1].len <= CONFIG_I2C_CQ_MODE_MAX_PAYLOAD_SIZE) &&
			    (msgs[1].len != 0)) {
				return true;
			}
		}
	}

	return false;
}
#endif /* CONFIG_I2C_IT8XXX2_CQ_MODE */

static int i2c_enhance_transfer(const struct device *dev,
				struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t addr)
{
	struct i2c_enhance_data *data = dev->data;
	int ret;

#ifdef CONFIG_I2C_TARGET
	if (data->target_attached) {
		LOG_ERR("Device is registered as target");
		return -EBUSY;
	}
#endif
	/* Lock mutex of i2c controller */
	k_mutex_lock(&data->mutex, K_FOREVER);
	/* Block to enter power policy. */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	data->num_msgs = num_msgs;
	data->addr_16bit = addr;
	/*
	 * If the transaction of write to read is divided into two
	 * transfers, the repeat start transfer uses this flag to
	 * exclude checking bus busy.
	 */
	if (data->i2ccs == I2C_CH_NORMAL) {
		/* Make sure we're in a good state to start */
		if (i2c_bus_not_available(dev)) {
			/* Recovery I2C bus */
			i2c_recover_bus(dev);
			/*
			 * After resetting I2C bus, if I2C bus is not available
			 * (No external pull-up), drop the transaction.
			 */
			if (i2c_bus_not_available(dev)) {
				ret = -EIO;
				goto done;
			}
		}
	}

#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
	if (cq_mode_allowed(dev, msgs)) {
		data->err = i2c_enhance_cq_transfer(dev, msgs);
	} else
#endif
	{
		data->err = i2c_enhance_pio_transfer(dev, msgs);
	}
	/* Save return value. */
	ret = i2c_parsing_return_value(dev);

done:
	/* Permit to enter power policy */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	/* Unlock mutex of i2c controller */
	k_mutex_unlock(&data->mutex);

	return ret;
}

#ifdef CONFIG_I2C_TARGET
static void target_i2c_isr_dma(const struct device *dev,
			       uint8_t interrupt_status)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	const struct i2c_target_callbacks *target_cb = data->target_cfg->callbacks;
	struct i2c_target_dma_buffer *target_buffer = &data->target_buffer;
	uint8_t *base = config->base;

	/* Byte counter enable */
	if (interrupt_status & IT8XXX2_I2C_IDW_CLR) {
		IT8XXX2_I2C_BYTE_CNT_L(base) |=
			(IT8XXX2_I2C_DMA_ADDR_RELOAD |
			 IT8XXX2_I2C_BYTE_CNT_ENABLE);
	}
	/* The number of received data exceeds the byte counter setting */
	if (interrupt_status & IT8XXX2_I2C_CNT_HOLD) {
		LOG_ERR("The excess data written starts "
			"from the memory address:%p",
			target_buffer->in_buffer +
			CONFIG_I2C_TARGET_IT8XXX2_MAX_BUF_SIZE);
	}
	/* Controller to write data */
	if (interrupt_status & IT8XXX2_I2C_SLVDATAFLG) {
		/* Number of receive data in target mode */
		data->buffer_size =
			((IT8XXX2_I2C_SLV_NUM_H(base) << 8) |
			 IT8XXX2_I2C_SLV_NUM_L(base)) + 1;

		/* Write data done callback function */
		target_cb->buf_write_received(data->target_cfg,
			target_buffer->in_buffer, data->buffer_size);
	}
	/* Peripheral finish */
	if (interrupt_status & IT8XXX2_I2C_P_CLR) {
		/* Transfer done callback function */
		target_cb->stop(data->target_cfg);
	}
	/* Controller to read data */
	if (interrupt_status & IT8XXX2_I2C_IDR_CLR) {
		uint32_t len;
		uint8_t *rdata = NULL;

		/* Clear byte counter setting */
		IT8XXX2_I2C_BYTE_CNT_L(base) &=
			~(IT8XXX2_I2C_DMA_ADDR_RELOAD |
			  IT8XXX2_I2C_BYTE_CNT_ENABLE);
		/* Read data callback function */
		target_cb->buf_read_requested(data->target_cfg,
			&rdata, &len);

		if (len > CONFIG_I2C_TARGET_IT8XXX2_MAX_BUF_SIZE) {
			LOG_ERR("The buffer size exceeds "
				"I2C_TARGET_IT8XXX2_MAX_BUF_SIZE: len=%d",
				len);
		} else {
			memcpy(target_buffer->out_buffer, rdata, len);
		}
	}

	/* Write clear the peripheral status */
	IT8XXX2_I2C_IRQ_ST(base) = interrupt_status;
}

static int target_i2c_isr_pio(const struct device *dev,
			      uint8_t interrupt_status,
			      uint8_t target_status)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	const struct i2c_target_callbacks *target_cb = data->target_cfg->callbacks;
	int ret = 0;
	uint8_t *base = config->base;
	uint8_t val;

	/* Target ID write flag */
	if (interrupt_status & IT8XXX2_I2C_IDW_CLR) {
		ret = target_cb->write_requested(data->target_cfg);
	}
	/* Target ID read flag */
	else if (interrupt_status & IT8XXX2_I2C_IDR_CLR) {
		if (!target_cb->read_requested(data->target_cfg, &val)) {
			IT8XXX2_I2C_DTR(base) = val;
		}
	}
	/* Byte transfer done */
	else if (target_status & IT8XXX2_I2C_BYTE_DONE) {
		/* Read of write */
		if (target_status & IT8XXX2_I2C_RW) {
			/* Host receiving, target transmitting */
			if (!target_cb->read_processed(data->target_cfg, &val)) {
				IT8XXX2_I2C_DTR(base) = val;
			}
		} else {
			/* Host transmitting, target receiving */
			val = IT8XXX2_I2C_DRR(base);
			ret = target_cb->write_received(data->target_cfg, val);
		}
	}

	return ret;
}

static void target_i2c_isr(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	const struct i2c_target_callbacks *target_cb = data->target_cfg->callbacks;
	uint8_t *base = config->base;
	uint8_t target_status = IT8XXX2_I2C_STR(base);

	/* Any error */
	if (target_status & E_TARGET_ANY_ERROR) {
		/* Hardware reset */
		IT8XXX2_I2C_CTR(base) |= IT8XXX2_I2C_HALT;
		return;
	}

	/* Interrupt pending */
	if (target_status & IT8XXX2_I2C_INT_PEND) {
		uint8_t interrupt_status = IT8XXX2_I2C_IRQ_ST(base);

		/* Determine whether the transaction uses PIO or DMA mode */
		if (config->target_pio_mode) {
			if (target_i2c_isr_pio(dev, interrupt_status, target_status) < 0) {
				/* NACK */
				IT8XXX2_I2C_CTR(base) &= ~IT8XXX2_I2C_ACK;
				IT8XXX2_I2C_CTR(base) |= IT8XXX2_I2C_HALT;
				data->target_nack = 1;
			}
			/* Peripheral finish */
			if (interrupt_status & IT8XXX2_I2C_P_CLR) {
				/* Transfer done callback function */
				target_cb->stop(data->target_cfg);

				if (data->target_nack) {
					/* Set acknowledge */
					IT8XXX2_I2C_CTR(base) |=
						IT8XXX2_I2C_ACK;
					data->target_nack = 0;
				}
			}
			/* Write clear the peripheral status */
			IT8XXX2_I2C_IRQ_ST(base) = interrupt_status;
			/* Hardware reset */
			IT8XXX2_I2C_CTR(base) |= IT8XXX2_I2C_HALT;
		} else {
			target_i2c_isr_dma(dev, interrupt_status);
		}
	}
}
#endif

static void i2c_enhance_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;

#ifdef CONFIG_I2C_TARGET
	if (data->target_attached) {
		target_i2c_isr(dev);
	} else {
#endif
#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
		uint8_t *base = config->base;

		/* If done doing work, wake up the task waiting for the transfer */
		if (IT8XXX2_I2C_CTR1(base) & IT8XXX2_I2C_COMQ_EN) {
			if (enhanced_i2c_cq_isr(dev)) {
				return;
			}
		} else
#endif
		{
			if (i2c_transaction(dev)) {
				return;
			}
		}
		irq_disable(config->i2c_irq_base);
		k_sem_give(&data->device_sync_sem);
#ifdef CONFIG_I2C_TARGET
	}
#endif
}

#ifdef CONFIG_I2C_TARGET_ALLOW_POWER_SAVING
void wui_scl_isr(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	/* Disable interrupts on SCL pin to avoid repeated interrupts. */
	(void)gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1), GPIO_INT_MODE_DISABLED);
}

void wui_sda_isr(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	/* Disable interrupts on SDA pin to avoid repeated interrupts. */
	(void)gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1), GPIO_INT_MODE_DISABLED);
}
#endif

static int i2c_enhance_init(const struct device *dev)
{
	struct i2c_enhance_data *data = dev->data;
	const struct i2c_enhance_config *config = dev->config;
	uint8_t *base = config->base;
	uint8_t data_hold_time = config->data_hold_time;
	uint32_t bitrate_cfg;
	int error, status;

#ifdef CONFIG_I2C_TARGET
	if (!config->target_enable) {
#endif
		/* Initialize mutex and semaphore */
		k_mutex_init(&data->mutex);
		k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

		/* Enable clock to specified peripheral */
		volatile uint8_t *reg = (volatile uint8_t *)
			(IT8XXX2_ECPM_BASE + (config->clock_gate_offset >> 8));
		uint8_t reg_mask = config->clock_gate_offset & 0xff;
		*reg &= ~reg_mask;

		/* Enable I2C function */
		/* Software reset */
		IT8XXX2_I2C_DHTR(base) |= IT8XXX2_I2C_SOFT_RST;
		IT8XXX2_I2C_DHTR(base) &= ~IT8XXX2_I2C_SOFT_RST;
		/* reset i2c port */
		i2c_reset(dev);
		/* bit1, Module enable */
		IT8XXX2_I2C_CTR1(base) = 0;

#ifdef CONFIG_I2C_IT8XXX2_CQ_MODE
		/* Set command address registers. */
		enhanced_i2c_set_cmd_addr_regs(dev);
#endif

		/* ChannelA-F switch selection of I2C pin */
		if (config->port == SMB_CHANNEL_A) {
			IT8XXX2_SMB_SMB01CHS = (IT8XXX2_SMB_SMB01CHS &= ~GENMASK(2, 0)) |
				config->channel_switch_sel;
		} else if (config->port == SMB_CHANNEL_B) {
			IT8XXX2_SMB_SMB01CHS = (config->channel_switch_sel << 4) |
				(IT8XXX2_SMB_SMB01CHS &= ~GENMASK(6, 4));
		} else if (config->port == SMB_CHANNEL_C) {
			IT8XXX2_SMB_SMB23CHS = (IT8XXX2_SMB_SMB23CHS &= ~GENMASK(2, 0)) |
				config->channel_switch_sel;
		} else if (config->port == I2C_CHANNEL_D) {
			IT8XXX2_SMB_SMB23CHS = (config->channel_switch_sel << 4) |
				(IT8XXX2_SMB_SMB23CHS &= ~GENMASK(6, 4));
		} else if (config->port == I2C_CHANNEL_E) {
			IT8XXX2_SMB_SMB45CHS = (IT8XXX2_SMB_SMB45CHS &= ~GENMASK(2, 0)) |
				config->channel_switch_sel;
		} else if (config->port == I2C_CHANNEL_F) {
			IT8XXX2_SMB_SMB45CHS = (config->channel_switch_sel << 4) |
				(IT8XXX2_SMB_SMB45CHS &= ~GENMASK(6, 4));
		}

		/* Set I2C data hold time. */
		IT8XXX2_I2C_DHTR(base) = (IT8XXX2_I2C_DHTR(base) & ~GENMASK(2, 0)) |
			(data_hold_time - 3);

		/* Set clock frequency for I2C ports */
		if (config->bitrate == I2C_BITRATE_STANDARD ||
			config->bitrate == I2C_BITRATE_FAST ||
			config->bitrate == I2C_BITRATE_FAST_PLUS) {
			bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
		} else {
			/* Device tree specified speed */
			bitrate_cfg = I2C_SPEED_DT << I2C_SPEED_SHIFT;
		}

		error = i2c_enhance_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
		data->i2ccs = I2C_CH_NORMAL;

		if (error) {
			LOG_ERR("i2c: failure initializing");
			return error;
		}
#ifdef CONFIG_I2C_TARGET
	}
#endif

	/* Set the pin to I2C alternate function. */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure I2C pins");
		return status;
	}

#ifdef CONFIG_I2C_TARGET_ALLOW_POWER_SAVING
	if (config->target_enable) {
		/*
		 * Configure GPIO callbacks for SDA/SCL pins as wake-up sources.
		 * When the device enters PM_DEVICE_ACTION_SUSPEND, the pins are
		 * set to trigger interrupts on both edges.
		 * Any bus activity will wake the system from Deep Doze. Enabling
		 * lower power consumption while maintaining reliable communication.
		 */
		gpio_init_callback(&data->gpio_wui_scl_cb, wui_scl_isr, BIT(config->scl_gpios.pin));
		status = gpio_add_callback(config->scl_gpios.port, &data->gpio_wui_scl_cb);
		if (status < 0) {
			LOG_ERR("Failed to add SCL %d wui pin callback (err %d)", config->port,
				status);
			return status;
		}
		gpio_init_callback(&data->gpio_wui_sda_cb, wui_sda_isr, BIT(config->sda_gpios.pin));
		status = gpio_add_callback(config->sda_gpios.port, &data->gpio_wui_sda_cb);
		if (status < 0) {
			LOG_ERR("Failed to add SDA %d wui pin callback (err %d)", config->port,
				status);
			return status;
		}
	}
#endif

	return 0;
}

static void i2c_enhance_set_scl(void *io_context, int state)
{
	const struct i2c_enhance_config *config = io_context;

	gpio_pin_set_dt(&config->scl_gpios, state);
}

static void i2c_enhance_set_sda(void *io_context, int state)
{
	const struct i2c_enhance_config *config = io_context;

	gpio_pin_set_dt(&config->sda_gpios, state);
}

static int i2c_enhance_get_sda(void *io_context)
{
	const struct i2c_enhance_config *config = io_context;
	int ret = gpio_pin_get_dt(&config->sda_gpios);

	/* Default high as that would be a NACK */
	return ret != 0;
}

static const struct i2c_bitbang_io i2c_enhance_bitbang_io = {
	.set_scl = i2c_enhance_set_scl,
	.set_sda = i2c_enhance_set_sda,
	.get_sda = i2c_enhance_get_sda,
};

static int i2c_enhance_recover_bus(const struct device *dev)
{
	const struct i2c_enhance_config *config = dev->config;
	struct i2c_enhance_data *data = dev->data;
	int status, ret;

	/* Output type selection */
	gpio_flags_t flags = GPIO_OUTPUT | (config->push_pull_recovery ? 0 : GPIO_OPEN_DRAIN);
	/* Set SCL of I2C as GPIO pin */
	gpio_pin_configure_dt(&config->scl_gpios, flags);
	/* Set SDA of I2C as GPIO pin */
	gpio_pin_configure_dt(&config->sda_gpios, flags);

	i2c_bitbang_init(&data->bitbang, &i2c_enhance_bitbang_io, (void *)config);

	ret = i2c_bitbang_recover_bus(&data->bitbang);
	if (ret != 0) {
		LOG_ERR("%s: Failed to recover bus (err %d)", dev->name, ret);
	}

	/* Set GPIO back to I2C alternate function */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure I2C pins");
		return status;
	}

	/* reset i2c port */
	i2c_reset(dev);
	LOG_ERR("I2C ch%d reset cause %d", config->port,
		I2C_RC_NO_IDLE_FOR_START);

	return 0;
}

#ifdef CONFIG_I2C_TARGET
static int i2c_enhance_target_register(const struct device *dev,
				       struct i2c_target_config *target_cfg)
{
	const struct i2c_enhance_config *config = dev->config;
	struct i2c_enhance_data *data = dev->data;
	uint8_t *base = config->base;

	if (!target_cfg) {
		return -EINVAL;
	}

	if (target_cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	if (data->target_attached) {
		return -EBUSY;
	}

	data->target_cfg = target_cfg;
	data->target_attached = true;

	/* Software reset */
	IT8XXX2_I2C_DHTR(base) |= IT8XXX2_I2C_SOFT_RST;
	IT8XXX2_I2C_DHTR(base) &= ~IT8XXX2_I2C_SOFT_RST;
	/* Disable the timeout setting when clock/data are in a low state */
	IT8XXX2_I2C_TO_ARB_ST(base) &= ~(IT8XXX2_I2C_SCL_TIMEOUT_EN |
					 IT8XXX2_I2C_SDA_TIMEOUT_EN);
	/* Bit stretching */
	IT8XXX2_I2C_TOS(base) |= IT8XXX2_I2C_CLK_STRETCH;
	/* Peripheral address(8-bit) */
	IT8XXX2_I2C_IDR(base) = target_cfg->address << 1;
	/* I2C interrupt enable and set acknowledge */
	IT8XXX2_I2C_CTR(base) = IT8XXX2_I2C_INT_EN | IT8XXX2_I2C_HALT |
				     IT8XXX2_I2C_ACK;
	/* Interrupt status write clear */
	IT8XXX2_I2C_IRQ_ST(base) = 0xff;

	/* I2C target initial configuration of PIO mode */
	if (config->target_pio_mode) {
#ifndef CONFIG_I2C_TARGET_ALLOW_POWER_SAVING
		/* Block to enter power policy. */
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif

		/* I2C module enable */
		IT8XXX2_I2C_CTR1(base) = IT8XXX2_I2C_MDL_EN;
	/* I2C target initial configuration of DMA mode */
	} else {
		struct i2c_target_dma_buffer *target_buffer = &data->target_buffer;
		uint32_t in_data_addr, out_data_addr;
		int buf_size = CONFIG_I2C_TARGET_IT8XXX2_MAX_BUF_SIZE;

		/* Clear read and write data buffer of DMA */
		memset(target_buffer->in_buffer, 0, buf_size);
		memset(target_buffer->out_buffer, 0, buf_size);

		in_data_addr = (uint32_t)target_buffer->in_buffer & 0xffffff;
		out_data_addr = (uint32_t)target_buffer->out_buffer & 0xffffff;
		/*
		 * DMA write target address register
		 * for high order byte
		 */
		IT8XXX2_I2C_RAMH2A(base) = in_data_addr >> 16;
		IT8XXX2_I2C_RAMHA(base) = in_data_addr >> 8;
		IT8XXX2_I2C_RAMLA(base) = in_data_addr;
		/*
		 * DMA read target address register
		 * for high order byte
		 */
		IT8XXX2_I2C_CMD_ADDH2(base) = out_data_addr >> 16;
		IT8XXX2_I2C_RAMHA2(base) = out_data_addr >> 8;
		IT8XXX2_I2C_RAMLA2(base) = out_data_addr;

		/* Byte counter setting */
		/* This register indicates byte count[10:3]. */
		IT8XXX2_I2C_BYTE_CNT_H(base) =
			CONFIG_I2C_TARGET_IT8XXX2_MAX_BUF_SIZE >> 3;
		/* This register indicates byte count[2:0]. */
		IT8XXX2_I2C_BYTE_CNT_L(base) =
			CONFIG_I2C_TARGET_IT8XXX2_MAX_BUF_SIZE & GENMASK(2, 0);

#ifndef CONFIG_I2C_TARGET_ALLOW_POWER_SAVING
		/*
		 * The EC processor(CPU) cannot be in the k_cpu_idle() and power
		 * policy during the transactions with the CQ mode(DMA mode).
		 * Otherwise, the EC processor would be clock gated.
		 */
		chip_block_idle();
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif

		/* I2C module enable and command queue mode */
		IT8XXX2_I2C_CTR1(base) = IT8XXX2_I2C_COMQ_EN | IT8XXX2_I2C_MDL_EN;
	}

	ite_intc_isr_clear(config->i2c_irq_base);
	irq_enable(config->i2c_irq_base);

	return 0;
}

static int i2c_enhance_target_unregister(const struct device *dev,
					 struct i2c_target_config *cfg)
{
	const struct i2c_enhance_config *config = dev->config;
	struct i2c_enhance_data *data = dev->data;

	if (!data->target_attached) {
		return -EINVAL;
	}

	irq_disable(config->i2c_irq_base);

#ifndef CONFIG_I2C_TARGET_ALLOW_POWER_SAVING
	/* Permit to enter power policy and idle mode. */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	if (!config->target_pio_mode) {
		chip_permit_idle();
	}
#endif

	data->target_cfg = NULL;
	data->target_attached = false;
	data->target_nack = 0;

	return 0;
}
#endif

#ifdef CONFIG_I2C_TARGET_ALLOW_POWER_SAVING
static inline int i2c_enhance_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_enhance_config *const config = dev->config;
	int ret;

	if (config->target_enable) {
		switch (action) {
		/* Next device power state is in active. */
		case PM_DEVICE_ACTION_RESUME:
			/* Disable interrupts on SCL/SDA pin to avoid repeated interrupts. */
			ret = gpio_pin_interrupt_configure_dt(&config->scl_gpios,
							      GPIO_INT_MODE_DISABLED);
			if (ret < 0) {
				LOG_ERR("Failed to configure I2C%d WUI (ret %d)", config->port,
					ret);
				return ret;
			}
			ret = gpio_pin_interrupt_configure_dt(&config->sda_gpios,
							      GPIO_INT_MODE_DISABLED);
			if (ret < 0) {
				LOG_ERR("Failed to configure I2C%d WUI (ret %d)", config->port,
					ret);
				return ret;
			}
			break;
		/* Next device power state is deep doze mode */
		case PM_DEVICE_ACTION_SUSPEND:
			/* Configure wakeup pins as both edge tribbers */
			ret = gpio_pin_interrupt_configure_dt(
				&config->scl_gpios, GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_BOTH);
			if (ret < 0) {
				LOG_ERR("Failed to configure I2C%d WUI (ret %d)", config->port,
					ret);
				return ret;
			}
			ret = gpio_pin_interrupt_configure_dt(
				&config->sda_gpios, GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_BOTH);
			if (ret < 0) {
				LOG_ERR("Failed to configure I2C%d WUI (ret %d)", config->port,
					ret);
				return ret;
			}
			break;
		default:
			return -ENOTSUP;
		}
	}

	return 0;
}
#endif /* CONFIG_I2C_TARGET_ALLOW_POWER_SAVING */

static DEVICE_API(i2c, i2c_enhance_driver_api) = {
	.configure = i2c_enhance_configure,
	.get_config = i2c_enhance_get_config,
	.transfer = i2c_enhance_transfer,
	.recover_bus = i2c_enhance_recover_bus,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_enhance_target_register,
	.target_unregister = i2c_enhance_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#ifdef CONFIG_I2C_TARGET
BUILD_ASSERT(IS_ENABLED(CONFIG_I2C_TARGET_BUFFER_MODE),
	     "When I2C target config is enabled, the buffer mode must be used.");
#endif

#define I2C_ITE_ENHANCE_INIT(inst)                                              \
	PINCTRL_DT_INST_DEFINE(inst);                                           \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) ==                    \
		     50000) ||                                                  \
		     (DT_INST_PROP(inst, clock_frequency) ==                    \
		     I2C_BITRATE_STANDARD) ||                                   \
		     (DT_INST_PROP(inst, clock_frequency) ==                    \
		     I2C_BITRATE_FAST) ||                                       \
		     (DT_INST_PROP(inst, clock_frequency) ==                    \
		     I2C_BITRATE_FAST_PLUS), "Not support I2C bit rate value"); \
	static void i2c_enhance_config_func_##inst(void);                       \
										\
	static const struct i2c_enhance_config i2c_enhance_cfg_##inst = {       \
		.base = (uint8_t *)(DT_INST_REG_ADDR(inst)),                    \
		.irq_config_func = i2c_enhance_config_func_##inst,              \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                 \
		.i2c_irq_base = DT_INST_IRQN(inst),                             \
		.port = DT_INST_PROP(inst, port_num),                           \
		.channel_switch_sel = DT_INST_PROP(inst, channel_switch_sel),   \
		.scl_gpios = GPIO_DT_SPEC_INST_GET(inst, scl_gpios),            \
		.sda_gpios = GPIO_DT_SPEC_INST_GET(inst, sda_gpios),            \
		.prescale_scl_low = DT_INST_PROP_OR(inst, prescale_scl_low, 0), \
		.data_hold_time = DT_INST_PROP_OR(inst, data_hold_time, 0),     \
		.clock_gate_offset = DT_INST_PROP(inst, clock_gate_offset),     \
		.transfer_timeout_ms = DT_INST_PROP(inst, transfer_timeout_ms), \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                   \
		.target_enable = DT_INST_PROP(inst, target_enable),             \
		.target_pio_mode = DT_INST_PROP(inst, target_pio_mode),         \
		.push_pull_recovery = DT_INST_PROP(inst, push_pull_recovery),   \
	};                                                                      \
										\
	static struct i2c_enhance_data i2c_enhance_data_##inst;                 \
		IF_ENABLED(CONFIG_I2C_TARGET_ALLOW_POWER_SAVING,                \
		(PM_DEVICE_DT_INST_DEFINE(inst, i2c_enhance_pm_action);))       \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_enhance_init,                       \
		COND_CODE_1(CONFIG_I2C_TARGET_ALLOW_POWER_SAVING,               \
			    (PM_DEVICE_DT_INST_GET(inst)), (NULL)),             \
				  &i2c_enhance_data_##inst,                     \
				  &i2c_enhance_cfg_##inst,                      \
				  POST_KERNEL,                                  \
				  CONFIG_I2C_INIT_PRIORITY,                     \
				  &i2c_enhance_driver_api);                     \
										\
	static void i2c_enhance_config_func_##inst(void)                        \
	{                                                                       \
		IRQ_CONNECT(DT_INST_IRQN(inst),                                 \
			0,                                                      \
			i2c_enhance_isr,                                        \
			DEVICE_DT_INST_GET(inst), 0);                           \
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_ITE_ENHANCE_INIT)
