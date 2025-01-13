/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_i2c

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_npcm, LOG_LEVEL_ERR);

/* Configure TX and RX buffer size for I2C DMA
 * This setting applies to all (12c1a, 12c1b, 12c2a ...)
 */
#define CONFIG_I2C_MAX_TX_SIZE 256
#define CONFIG_I2C_MAX_RX_SIZE 256

/* Default maximum time we allow for an I2C transfer (unit:ms) */
#define I2C_TRANS_TIMEOUT K_MSEC(500)

/* Default max waiting time for i2c ready (unit:ms) */
#define I2C_WAITING_TIME K_MSEC(1000)

/* Hardware Timeout configuration (unit:ms) */
#define CONFIG_CONTROLLER_HW_TIMEOUT_EN 'N'
#define CONFIG_CONTROLLER_HW_TIMEOUT_CLK_LOW_TIME 25
#define CONFIG_CONTROLLER_HW_TIMEOUT_CLK_CYCLE_TIME 50

/* when using Quick command (SMBUS), do not enable target timeout */
#define CONFIG_TARGET_HW_TIMEOUT_EN 'N'
#define CONFIG_TARGET_HW_TIMEOUT_CLK_LOW_TIME 25
#define CONFIG_TARGET_HW_TIMEOUT_CLK_CYCLE_TIME 50

/* Data abort timeout */
#define ABORT_TIMEOUT 10000

/* I2C operation state */
enum i2c_npcm_oper_state {
	I2C_NPCM_OPER_STA_IDLE,
	I2C_NPCM_OPER_STA_START,
	I2C_NPCM_OPER_STA_WRITE,
	I2C_NPCM_OPER_STA_READ,
	I2C_NPCM_OPER_STA_QUICK,
};

/* Device configuration */
struct i2c_npcm_config {
	uintptr_t base;				/* i2c controller base address */
	uint32_t clk_cfg;			/* clock configuration */
	uint32_t default_bitrate;
	uint8_t irq;				/* i2c controller irq */
	const struct pinctrl_dev_config *pcfg;  /* pinmux configuration */
};

/*rx_buf and tx_buf address must 4-align for DMA */
#pragma pack(4)
struct i2c_npcm_data {
	struct k_sem lock_sem;  /* mutex of i2c controller */
	struct k_sem sync_sem;  /* semaphore used for synchronization */
	enum i2c_npcm_oper_state ctrl_oper_state;
	enum i2c_npcm_oper_state target_oper_state;
	uint32_t bitrate;
	uint32_t source_clk;
	uint16_t rx_cnt;
	uint16_t tx_cnt;
	uint8_t dev_addr; /* device address (8 bits) */
	uint8_t rx_buf[CONFIG_I2C_MAX_TX_SIZE];
	uint8_t tx_buf[CONFIG_I2C_MAX_RX_SIZE];
	uint8_t *rx_msg_buf;
	int err_code;
	struct i2c_target_config *target_cfg;
};
#pragma pack()

/* Driver convenience defines */
#define I2C_INSTANCE(dev) ((struct i2c_reg *)((const struct i2c_npcm_config *)(dev)->config)->base)


/* This macro should be set only when in Controller mode or when requesting Controller mode.
 * Set START bit to CTL1 register of I2C module, but exclude STOP bit, ACK bit.
 */
static inline void i2c_npcm_start(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	inst->SMBnCTL1 = (inst->SMBnCTL1 & ~0x13) | BIT(NPCM_SMBnCTL1_START);
}

static inline void i2c_npcm_stop(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	(inst)->SMBnCTL1 = ((inst)->SMBnCTL1 & ~0x13) | BIT(NPCM_SMBnCTL1_STOP);
}

static inline void i2c_npcm_enable_stall(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	inst->SMBnCTL1 = (inst->SMBnCTL1 & ~0x13) | BIT(NPCM_SMBnCTL1_STASTRE);
}

static inline void i2c_npcm_disable_stall(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	inst->SMBnCTL1 = inst->SMBnCTL1 & ~(0x13 | BIT(NPCM_SMBnCTL1_STASTRE));
}

static inline void i2c_npcm_nack(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	inst->SMBnCTL1 = (inst->SMBnCTL1 & ~0x13) | BIT(NPCM_SMBnCTL1_ACK);
}

static void i2c_npcm_start_DMA(const struct device *dev,
				  uint32_t addr, uint16_t len)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	/* address */
	inst->DMA_ADDR1 = (uint8_t)((addr) >> 0);
	inst->DMA_ADDR2 = (uint8_t)((addr) >> 8);
	inst->DMA_ADDR3 = (uint8_t)((addr) >> 16);
	inst->DMA_ADDR4 = (uint8_t)((addr) >> 24);

	/* length */
	inst->DATA_LEN1 = (uint8_t)((len) >> 0);
	inst->DATA_LEN2 = (uint8_t)((len) >> 8);

	/* clear interrupt bit */
	inst->DMA_CTRL = BIT(NPCM_DMA_CTRL_DMA_INT_CLR);

	/* Set DMA enable */
	inst->DMA_CTRL = BIT(NPCM_DMA_CTRL_DMA_EN);
}

/* return Negative Acknowledge when DMA received last byte. */
static void i2c_npcm_DMA_lastbyte(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	inst->DMA_CTRL = (inst->DMA_CTRL & ~BIT(NPCM_DMA_CTRL_DMA_INT_CLR)) |
			 BIT(NPCM_DMA_CTRL_LAST_PEC);
}

static uint16_t i2c_npcm_get_dma_cnt(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	return (uint16_t)(((uint16_t)(inst->DATA_CNT1) << 8) + (uint16_t)(inst->DATA_CNT2));

}

void Set_Cumulative_ClockLow_Timeout(const struct device *dev, uint8_t interval_ms)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	struct i2c_npcm_data *const data = dev->data;
	uint8_t div;

	div = data->source_clk / 1000 / 1000;
	div = div - 1;
	if ((div < 0x03) || (div > 0x3F)) {
		return;
	}
	inst->SMBnCTL2 |= BIT(NPCM_SMBnCTL2_ENABLE);
	inst->TIMEOUT_EN = (div << NPCM_TIMEOUT_EN_TO_CKDIV);
	inst->TIMEOUT_ST |= BIT(NPCM_TIMEOUT_ST_T_OUTST1_EN);
	inst->TIMEOUT_ST = BIT(NPCM_TIMEOUT_ST_T_OUTST1);
	inst->TIMEOUT_CTL2 = interval_ms;
}

void Set_Cumulative_ClockCycle_Timeout(const struct device *dev, uint8_t interval_ms)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	struct i2c_npcm_data *const data = dev->data;
	uint8_t div;

	div = data->source_clk / 1000 / 1000;
	div = div - 1;
	if ((div < 0x03) || (div > 0x3F)) {
		return;
	}
	inst->SMBnCTL2 |= BIT(NPCM_SMBnCTL2_ENABLE);
	inst->TIMEOUT_EN = (div << NPCM_TIMEOUT_EN_TO_CKDIV);
	inst->TIMEOUT_ST |= BIT(NPCM_TIMEOUT_ST_T_OUTST2_EN);
	inst->TIMEOUT_ST = BIT(NPCM_TIMEOUT_ST_T_OUTST2);
	inst->TIMEOUT_CTL1 = interval_ms;
}

static void i2c_npcm_reset_module(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	struct i2c_npcm_data *const data = dev->data;
	uint32_t ctl1_tmp;
	uint32_t timeout_en_tmp;

	ctl1_tmp = inst->SMBnCTL1;
	timeout_en_tmp = inst->TIMEOUT_EN;

	/* Disable and then Enable I2C module */
	inst->SMBnCTL2 &= ~BIT(NPCM_SMBnCTL2_ENABLE);
	inst->SMBnCTL2 |= BIT(NPCM_SMBnCTL2_ENABLE);

	inst->SMBnCTL1 = (ctl1_tmp & (BIT(NPCM_SMBnCTL1_INTEN) | BIT(NPCM_SMBnCTL1_EOBINTE) |
				      BIT(NPCM_SMBnCTL1_GCMEN) | BIT(NPCM_SMBnCTL1_NMINTE)));
	inst->TIMEOUT_EN = timeout_en_tmp;

	data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;
}

static void i2c_npcm_abort_data(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	uint16_t timeout = ABORT_TIMEOUT;

	/* Generate a STOP condition */
	i2c_npcm_stop(dev);

	/* Clear NEGACK, STASTR and BER bits */
	inst->SMBnST = (BIT(NPCM_SMBnST_STASTR) |
			BIT(NPCM_SMBnST_NEGACK) |
			BIT(NPCM_SMBnST_BER));

	/* Wait till STOP condition is generated */
	while (--timeout) {
		if ((inst->SMBnCTL1 & BIT(NPCM_SMBnCTL1_STOP)) == 0x00) {
			break;
		}
	}

	/* Clear BB (BUS BUSY) bit */
	inst->SMBnCST = BIT(NPCM_SMBnCST_BB);
}

static uint16_t i2c_npcm_get_DMA_cnt(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	return (uint16_t)(((uint16_t)(inst->DATA_CNT1) << 8) + (uint16_t)(inst->DATA_CNT2));
}

static void i2c_npcm_set_baudrate(const struct device *dev, uint32_t bus_freq)
{
	uint32_t reg_tmp;
	const struct i2c_npcm_config *const config = dev->config;
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	struct i2c_npcm_data *const data = dev->data;

	if (clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clk_cfg,
				   &data->source_clk) != 0) {
		LOG_ERR("Get %s clock source.", dev->name);
	}

	LOG_DBG("i2c clock source: %d", data->source_clk);

	reg_tmp = (data->source_clk) / (bus_freq * 4);

	if (bus_freq < 400000) {
		if (reg_tmp < 8) {
			reg_tmp = 8;
		} else if (reg_tmp > 511) {
			reg_tmp = 511;
		}

		/* Disable fast mode and fast mode plus */
		inst->SMBnCTL3 &= ~BIT(NPCM_SMBnCTL3_400K_MODE);
		SET_FIELD(inst->SMBnCTL2, NPCM_SMBnCTL2_SCLFRQ60_FIELD, reg_tmp & 0x7f);
		SET_FIELD(inst->SMBnCTL3, NPCM_SMBnCTL3_SCLFRQ87_FIELD, reg_tmp >> 7);

		/* Set HLDT (48MHz, HLDT = 17, Hold Time = 360ns) */
		if (data->source_clk >= 40000000) {
			inst->SMBnCTL4 = 17;
		} else if (data->source_clk >= 20000000) {
			inst->SMBnCTL4 = 9;
		} else {
			inst->SMBnCTL4 = 7;
		}
	} else {
		if (reg_tmp < 5) {
			reg_tmp = 5;
		} else if (reg_tmp > 255) {
			reg_tmp = 255;
		}

		/* Enable fast mode and fast mode plus */
		inst->SMBnCTL3 |= BIT(NPCM_SMBnCTL3_400K_MODE);
		SET_FIELD(inst->SMBnCTL2, NPCM_SMBnCTL2_SCLFRQ60_FIELD, 0);
		SET_FIELD(inst->SMBnCTL3, NPCM_SMBnCTL3_SCLFRQ87_FIELD, 0);
		inst->SMBnSCLHT = reg_tmp - 3;
		inst->SMBnSCLLT = reg_tmp - 1;

		/* Set HLDT (48MHz, HLDT = 17, Hold Time = 360ns) */
		if (data->source_clk >= 40000000) {
			inst->SMBnCTL4 = 17;
		} else if (data->source_clk >= 20000000) {
			inst->SMBnCTL4 = 9;
		} else {
			inst->SMBnCTL4 = 7;
		}
	}
}

static void i2c_npcm_notify(const struct device *dev, int error)
{
#if (CONFIG_CONTROLLER_HW_TIMEOUT_EN == 'Y')
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
#endif
	struct i2c_npcm_data *const data = dev->data;

#if (CONFIG_CONTROLLER_HW_TIMEOUT_EN == 'Y')
	/* Disable HW Timeout */
	inst->TIMEOUT_EN &= ~BIT(NPCM_TIMEOUT_EN_TIMEOUT_EN);
#endif

	data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;
	data->err_code = error;

	k_sem_give(&data->sync_sem);
}

static int i2c_npcm_wait_completion(const struct device *dev)
{
	struct i2c_npcm_data *const data = dev->data;
	int ret;

	ret = k_sem_take(&data->sync_sem, I2C_TRANS_TIMEOUT);
	if (ret != 0) {
		i2c_npcm_reset_module(dev);
		data->err_code = -ETIMEDOUT;
	}

	return data->err_code;
}

/* NPCX specific I2C controller functions */
static int i2c_npcm_mutex_lock(const struct device *dev, k_timeout_t timeout)
{
	struct i2c_npcm_data *const data = dev->data;

	return k_sem_take(&data->lock_sem, timeout);
}

static void i2c_npcm_mutex_unlock(const struct device *dev)
{
	struct i2c_npcm_data *const data = dev->data;

	k_sem_give(&data->lock_sem);
}

static void i2c_npcm_ctrl_isr(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	struct i2c_npcm_data *const data = dev->data;

	/* --------------------------------------------- */
	/* Timeout occurred                              */
	/* --------------------------------------------- */
#if (CONFIG_CONTROLLER_HW_TIMEOUT_EN == 'Y')
	if (inst->TIMEOUT_ST & BIT(NPCM_TIMEOUT_ST_T_OUTST1)) {
		/* clear timeout flag */
		inst->TIMEOUT_ST = BIT(NPCM_TIMEOUT_ST_T_OUTST1);
		i2c_npcm_reset_module(dev);
		i2c_npcm_notify(dev, -ETIMEDOUT);
	}
	if (inst->TIMEOUT_ST & BIT(NPCM_TIMEOUT_ST_T_OUTST2)) {
		/* clear timeout flag */
		inst->TIMEOUT_ST = BIT(NPCM_TIMEOUT_ST_T_OUTST2);
		i2c_npcm_reset_module(dev);
		i2c_npcm_notify(dev, -ETIMEDOUT);
	}
#endif

	/* --------------------------------------------- */
	/* NACK occurred                                 */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_NEGACK)) {
		i2c_npcm_abort_data(dev);
		/* Clear DMA flag */
		inst->DMA_CTRL = BIT(NPCM_DMA_CTRL_DMA_INT_CLR);
		data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;
		i2c_npcm_notify(dev, -ENXIO);
	}

	/* --------------------------------------------- */
	/* BUS ERROR occurred                            */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_BER)) {
		i2c_npcm_abort_data(dev);
		data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;
		i2c_npcm_reset_module(dev);
		i2c_npcm_notify(dev, -EAGAIN);
	}

	/* --------------------------------------------- */
	/* SDA status is set - transmit or receive       */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_SDAST)) {
		if (data->ctrl_oper_state == I2C_NPCM_OPER_STA_START) {
			if (data->tx_cnt == 0 && data->rx_cnt == 0) {
				/* Quick command (SMBUS protocol) */
				data->ctrl_oper_state = I2C_NPCM_OPER_STA_QUICK;
				i2c_npcm_enable_stall(dev);
				/* quick read or quick write is determined by address */
				inst->SMBnSDA = data->dev_addr;
			} else if (data->tx_cnt == 0 && data->rx_cnt > 0) {
				/* receive mode */
				data->ctrl_oper_state = I2C_NPCM_OPER_STA_READ;
				i2c_npcm_enable_stall(dev);
				/* send read address */
				inst->SMBnSDA = data->dev_addr | 0x1;
			} else if (data->tx_cnt > 0) {
				/* transmit mode */
				data->ctrl_oper_state = I2C_NPCM_OPER_STA_WRITE;
				/* send write address */
				inst->SMBnSDA = data->dev_addr & 0xFE;
			}
		} else if (data->ctrl_oper_state == I2C_NPCM_OPER_STA_WRITE) {
			/* Set DMA register to send data */
			i2c_npcm_start_DMA(dev, (uint32_t)data->tx_buf, data->tx_cnt);
		} else {
			/* Error */
		}
	}

	/* --------------------------------------------- */
	/* stall occurred                                */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_STASTR)) {
		if (data->ctrl_oper_state == I2C_NPCM_OPER_STA_READ) {
			/* Set DMA register to read data */
			i2c_npcm_DMA_lastbyte(dev);
			i2c_npcm_start_DMA(dev, (uint32_t)data->rx_buf, data->rx_cnt);
		} else if (data->ctrl_oper_state == I2C_NPCM_OPER_STA_QUICK) {
			i2c_npcm_stop(dev);
			data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;
			i2c_npcm_notify(dev, 0);
		} else {
			/* error */
		}

		i2c_npcm_disable_stall(dev);
		/* clear STASTR flag */
		inst->SMBnST = BIT(NPCM_SMBnST_STASTR);
	}

	/* --------------------------------------------- */
	/* DMA IRQ occurred                              */
	/* --------------------------------------------- */
	if (inst->DMA_CTRL & BIT(NPCM_DMA_CTRL_DMA_IRQ)) {
		if (data->ctrl_oper_state == I2C_NPCM_OPER_STA_WRITE) {
			/* Transmit mode */
			if (data->rx_cnt == 0) {
				/* no need to receive data */
				i2c_npcm_stop(dev);
				data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;
				i2c_npcm_notify(dev, 0);
			} else {
				data->ctrl_oper_state = I2C_NPCM_OPER_STA_READ;
				i2c_npcm_enable_stall(dev);
				i2c_npcm_start(dev);
				inst->SMBnSDA = (data->dev_addr | 0x1);
			}
		} else {
			/* received mode */
			i2c_npcm_stop(dev);
			data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;
			data->rx_cnt = i2c_npcm_get_DMA_cnt(dev);
			i2c_npcm_notify(dev, 0);
		}
		/* Clear DMA flag */
		inst->DMA_CTRL = BIT(NPCM_DMA_CTRL_DMA_INT_CLR);
	}
}

static void i2c_npcm_target_isr(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	struct i2c_npcm_data *const data = dev->data;
	const struct i2c_target_callbacks *target_cb =
		data->target_cfg->callbacks;
	uint16_t len;
	uint8_t overflow_data;


#if (CONFIG_TARGET_HW_TIMEOUT_EN == 'Y')
	/* --------------------------------------------- */
	/* Timeout occurred                              */
	/* --------------------------------------------- */
	if (inst->TIMEOUT_ST & BIT(NPCM_TIMEOUT_ST_T_OUTST1)) {
		LOG_ERR("target: HW timeout");
		data->target_oper_state = I2C_NPCM_OPER_STA_START;
		inst->TIMEOUT_ST = BIT(NPCM_TIMEOUT_ST_T_OUTST1);
		i2c_npcm_reset_module(dev);
	}
	if (inst->TIMEOUT_ST & BIT(NPCM_TIMEOUT_ST_T_OUTST2)) {
		LOG_ERR("target: HW timeout");
		data->target_oper_state = I2C_NPCM_OPER_STA_START;
		inst->TIMEOUT_ST = BIT(NPCM_TIMEOUT_ST_T_OUTST2);
		i2c_npcm_reset_module(dev);
	}
#endif

	/* --------------------------------------------- */
	/* NACK occurred                                 */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_NEGACK)) {
		inst->SMBnST = BIT(NPCM_SMBnST_NEGACK);
	}

	/* --------------------------------------------- */
	/* BUS ERROR occurred                            */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_BER)) {
		if (data->target_oper_state != I2C_NPCM_OPER_STA_QUICK) {
			LOG_ERR("target: bus error");
		}
		data->target_oper_state = I2C_NPCM_OPER_STA_START;
		/* clear BER */
		inst->SMBnST = BIT(NPCM_SMBnST_BER);
		i2c_npcm_reset_module(dev);
	}

	/* --------------------------------------------- */
	/* DMA IRQ occurred                              */
	/* --------------------------------------------- */
	if (inst->DMA_CTRL & BIT(NPCM_DMA_CTRL_DMA_IRQ)) {
		if (data->target_oper_state == I2C_NPCM_OPER_STA_READ) {
			/* if DMA overflow, send NACK to Controller, and next IRQ is SDAST alert */
			i2c_npcm_nack(dev);
		}
		/* clear DMA flag */
		inst->DMA_CTRL = BIT(NPCM_DMA_CTRL_DMA_INT_CLR);
	}

	/* --------------------------------------------- */
	/* Address match occurred                        */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_NMATCH)) {
		if (inst->SMBnST & BIT(NPCM_SMBnST_XMIT)) {
			/* target received Read-Address */
			if (data->target_oper_state != I2C_NPCM_OPER_STA_START) {
				/* target received data before */
				len = 0;
				while (len < i2c_npcm_get_dma_cnt(dev)) {
					target_cb->write_received(data->target_cfg,
								 data->rx_buf[len]);
					len++;
				}
			}

			/* prepare tx data */
			if (target_cb->read_requested(data->target_cfg, data->tx_buf) == 0) {
				len = 0;
				while (++len < sizeof(data->tx_buf)) {
					if (target_cb->read_processed(data->target_cfg,
							data->tx_buf + len) != 0) {
						break;
					}
				}
				data->tx_cnt = len;
			} else {
				/* target has no data to send */
				data->tx_cnt = 0;
			}

			data->target_oper_state = I2C_NPCM_OPER_STA_WRITE;

			if (data->tx_cnt != 0) {
				/* Set DMA register to send data */
				i2c_npcm_start_DMA(dev, (uint32_t)data->tx_buf, data->tx_cnt);
			} else {
				data->target_oper_state = I2C_NPCM_OPER_STA_QUICK;
			}
		} else {
			/* target received Write-Address */
			data->target_oper_state = I2C_NPCM_OPER_STA_READ;
			/* Set DMA register to get data */
			i2c_npcm_start_DMA(dev, (uint32_t)data->rx_buf, sizeof(data->rx_buf));
			target_cb->write_requested(data->target_cfg);
		}
		/* Clear address match bit & SDA pull high */
		inst->SMBnST = BIT(NPCM_SMBnST_NMATCH);
	}

	/* --------------------------------------------- */
	/* SDA status is set - transmit or receive       */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_SDAST)) {
		if (data->target_oper_state == I2C_NPCM_OPER_STA_READ) {
			/* Over Flow */
			overflow_data = inst->SMBnSDA;

			len = 0;
			while (len < i2c_npcm_get_dma_cnt(dev)) {
				if (target_cb->write_received(data->target_cfg, data->rx_buf[len]))
					break;

				len++;
			}
			target_cb->write_received(data->target_cfg, overflow_data);
			data->target_oper_state = I2C_NPCM_OPER_STA_START;
		} else {
			/* No Enough DMA data to send */
			inst->SMBnSDA = 0xFF;
		}
	}

	/* --------------------------------------------- */
	/* Target STOP occurred                           */
	/* --------------------------------------------- */
	if (inst->SMBnST & BIT(NPCM_SMBnST_SLVSTP)) {
		if (data->target_oper_state == I2C_NPCM_OPER_STA_READ) {
			len = 0;
			while (len < i2c_npcm_get_dma_cnt(dev)) {
				target_cb->write_received(data->target_cfg, data->rx_buf[len]);
				len++;
			}
		}
		if (data->target_oper_state != I2C_NPCM_OPER_STA_IDLE) {
			target_cb->stop(data->target_cfg);
		}
		data->target_oper_state = I2C_NPCM_OPER_STA_START;
		/* clear STOP flag */
		inst->SMBnST = BIT(NPCM_SMBnST_SLVSTP);
	}
}

static void i2c_set_target_addr(const struct device *dev, uint8_t target_addr)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

	/* set target addr 1 */
	inst->SMBnADDR1 = (target_addr | BIT(NPCM_SMBnADDR_SAEN));

	/* Enable I2C address match interrupt */
	inst->SMBnCTL1 |= BIT(NPCM_SMBnCTL1_NMINTE);
}

static int i2c_npcm_target_register(const struct device *dev,
				      struct i2c_target_config *cfg)
{
	struct i2c_npcm_data *data = dev->data;
	int ret = 0;

	if (!cfg) {
		return -EINVAL;
	}
	if (cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}
	if (i2c_npcm_mutex_lock(dev, I2C_WAITING_TIME) != 0) {
		return -EBUSY;
	}

	if (data->target_cfg) {
		/* target is already registered */
		ret = -EBUSY;
		goto exit;
	}

	data->target_cfg = cfg;
	data->target_oper_state = I2C_NPCM_OPER_STA_START;
	/* set target addr, cfg->address is 7 bit address */
	i2c_set_target_addr(dev, cfg->address);

exit:
	i2c_npcm_mutex_unlock(dev);
	return ret;
}

static int i2c_npcm_target_unregister(const struct device *dev,
					struct i2c_target_config *config)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	struct i2c_npcm_data *data = dev->data;


	if (!data->target_cfg) {
		return -EINVAL;
	}

	if (data->target_oper_state != I2C_NPCM_OPER_STA_START &&
	    data->ctrl_oper_state != I2C_NPCM_OPER_STA_IDLE) {
		return -EBUSY;
	}

	if (i2c_npcm_mutex_lock(dev, I2C_WAITING_TIME) != 0) {
		return -EBUSY;
	}

	/* clear target addr 1 */
	inst->SMBnADDR1 = 0;

	/* Disable I2C address match interrupt */
	inst->SMBnCTL1 &= ~BIT(NPCM_SMBnCTL1_NMINTE);

	/* clear all interrupt status */
	inst->SMBnST = 0xFF;

	data->target_oper_state = I2C_NPCM_OPER_STA_IDLE;
	data->target_cfg = NULL;

	i2c_npcm_mutex_unlock(dev);

	return 0;
}

/* I2C controller isr function */
static void i2c_npcm_isr(const struct device *dev)
{
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	struct i2c_npcm_data *data = dev->data;

	if (data->ctrl_oper_state != I2C_NPCM_OPER_STA_IDLE) {
		i2c_npcm_ctrl_isr(dev);
	} else {
		if (data->target_oper_state == I2C_NPCM_OPER_STA_IDLE) {
			/* clear all interrupt status */
			inst->SMBnST = 0xFF;
		} else {
			i2c_npcm_target_isr(dev);
		}
	}
}


/* I2C controller driver registration */
static int i2c_npcm_init(const struct device *dev)
{
	const struct i2c_npcm_config *const config = dev->config;
	struct i2c_npcm_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
	int ret;

	LOG_DBG("Device name: %s", dev->name);

	/* Configure pin-mux for I2C device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2C pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* Turn on device clock first and get source clock freq. */
	if (clock_control_on(clk_dev, (clock_control_subsys_t)config->clk_cfg) != 0) {
		LOG_ERR("Turn on %s clock fail.", dev->name);
		return -EIO;
	}

	/* reset data */
	gdma_memset_u8((uint8_t *)data, 0, sizeof(struct i2c_npcm_data));

	/* Set default baud rate for i2c*/
	data->bitrate = config->default_bitrate;
	LOG_DBG("bitrate: %d", data->bitrate);
	i2c_npcm_set_baudrate(dev, data->bitrate);

	/* Enable I2C module */
	inst->SMBnCTL2 |= BIT(NPCM_SMBnCTL2_ENABLE);
	/* Enable I2C interrupt */
	inst->SMBnCTL1 |= BIT(NPCM_SMBnCTL1_INTEN);

	/* initialize mutux and semaphore for i2c controller */
	ret = k_sem_init(&data->lock_sem, 1, 1);
	ret = k_sem_init(&data->sync_sem, 0, 1);

	/* Initialize driver status machine */
	data->ctrl_oper_state = I2C_NPCM_OPER_STA_IDLE;

	data->target_cfg = NULL;

	return 0;
}

static int i2c_npcm_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_npcm_data *const data = dev->data;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		/* 100 Kbit/s */
		data->bitrate = I2C_BITRATE_STANDARD;
		break;

	case I2C_SPEED_FAST:
		/* 400 Kbit/s */
		data->bitrate = I2C_BITRATE_FAST;
		break;

	case I2C_SPEED_FAST_PLUS:
		/* 1 Mbit/s */
		data->bitrate = I2C_BITRATE_FAST_PLUS;
		break;

	default:
		/* Not supported */
		return -ERANGE;
	}

	i2c_npcm_set_baudrate(dev, data->bitrate);

	return 0;
}

static int i2c_npcm_combine_msg(const struct device *dev,
				   struct i2c_msg *msgs, uint8_t num_msgs)
{
	struct i2c_npcm_data *const data = dev->data;
	uint8_t step = 0;
	uint8_t i;

	for (i = 0U; i < num_msgs; i++) {
		struct i2c_msg *msg = msgs + i;

		if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			/* support more than one write msg in a transfer */
			if (step == 0) {
				gdma_memcpy_u8(data->tx_buf + data->tx_cnt, msg->buf, msg->len);
				data->tx_cnt += msg->len;
			} else {
				return -1;
			}
		} else {
			/* just support one read msg in a transfer */
			if (step == 1) {
				return -1;
			}
			step = 1;
			data->rx_cnt = msg->len;
			data->rx_msg_buf = msg->buf;
		}
	}
	return 0;
}

static int i2c_npcm_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t addr)
{
	uint8_t value;
	struct i2c_reg *const inst = I2C_INSTANCE(dev);

#if (CONFIG_CONTROLLER_HW_TIMEOUT_EN == 'Y')
	struct i2c_reg *const inst = I2C_INSTANCE(dev);
#endif
	struct i2c_npcm_data *const data = dev->data;
	int ret;

	if (i2c_npcm_mutex_lock(dev, I2C_WAITING_TIME) != 0) {
		return -EBUSY;
	}

	/* Disable target addr 1 */
	value = inst->SMBnADDR1;
	value &= ~BIT(NPCM_SMBnADDR_SAEN);
	inst->SMBnADDR1 = value;

	/* prepare data to transfer */
	data->rx_cnt = 0;
	data->tx_cnt = 0;
	data->dev_addr = addr << 1;
	data->ctrl_oper_state = I2C_NPCM_OPER_STA_START;
	data->err_code = 0;
	if (i2c_npcm_combine_msg(dev, msgs, num_msgs) < 0) {
		i2c_npcm_mutex_unlock(dev);
		/* Enable target addr 1 */
		value = inst->SMBnADDR1;
		value |= BIT(NPCM_SMBnADDR_SAEN);
		inst->SMBnADDR1 = value;
		return -EPROTONOSUPPORT;
	}

	if (data->rx_cnt == 0 && data->tx_cnt == 0) {
		/* Quick command */
		if (num_msgs != 1) {
			/* Quick command must have one msg */
			i2c_npcm_mutex_unlock(dev);
			/* Enable target addr 1 */
			value = inst->SMBnADDR1;
			value |= BIT(NPCM_SMBnADDR_SAEN);
			inst->SMBnADDR1 = value;
			return -EPROTONOSUPPORT;
		}
		if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			/* set address to write address */
			data->dev_addr = data->dev_addr & 0xFE;
		} else {
			/* set address to read address */
			data->dev_addr = data->dev_addr | 0x1;
		}
	}

#if (CONFIG_CONTROLLER_HW_TIMEOUT_EN == 'Y')
	/* Set I2C HW timeout value */
	Set_Cumulative_ClockCycle_Timeout(dev, CONFIG_CONTROLLER_HW_TIMEOUT_CLK_CYCLE_TIME);
	Set_Cumulative_ClockLow_Timeout(dev, CONFIG_CONTROLLER_HW_TIMEOUT_CLK_LOW_TIME);
	/* Enable HW Timeout */
	inst->TIMEOUT_EN |= BIT(NPCM_TIMEOUT_EN_TIMEOUT_EN);
#endif

	k_sem_reset(&data->sync_sem);

	i2c_npcm_start(dev);

	ret = i2c_npcm_wait_completion(dev);

	if (data->rx_cnt != 0) {
		gdma_memcpy_u8(data->rx_msg_buf, data->rx_buf, data->rx_cnt);
	}

	i2c_npcm_mutex_unlock(dev);

	/* Enable target addr 1 */
	value = inst->SMBnADDR1;
	value |= BIT(NPCM_SMBnADDR_SAEN);
	inst->SMBnADDR1 = value;

	return ret;
}

static const struct i2c_driver_api i2c_npcm_driver_api = {
	.configure = i2c_npcm_configure,
	.transfer = i2c_npcm_transfer,
	.target_register = i2c_npcm_target_register,
	.target_unregister = i2c_npcm_target_unregister,
};


/* I2C controller init macro functions */
#define I2C_NPCM_CTRL_INIT_FUNC(inst) _CONCAT(i2c_npcm_init_, inst)
#define I2C_NPCM_CTRL_INIT_FUNC_DECL(inst)					 \
	static int i2c_npcm_init_##inst(const struct device *dev)
#define I2C_NPCM_CTRL_INIT_FUNC_IMPL(inst)					 \
	static int i2c_npcm_init_##inst(const struct device *dev)		 \
	{									 \
		int ret;							 \
										 \
		ret = i2c_npcm_init(dev);					 \
		IRQ_CONNECT(DT_INST_IRQN(inst),					 \
			    DT_INST_IRQ(inst, priority),			 \
			    i2c_npcm_isr,					 \
			    DEVICE_DT_INST_GET(inst),				 \
			    0);							 \
		irq_enable(DT_INST_IRQN(inst));					 \
										 \
		return ret;							 \
	}


#define I2C_NPCM_CTRL_INIT(inst)						 \
	PINCTRL_DT_INST_DEFINE(inst);						 \
										 \
	I2C_NPCM_CTRL_INIT_FUNC_DECL(inst);					 \
										 \
	static const struct i2c_npcm_config i2c_npcm_cfg_##inst = {		 \
		.base = DT_INST_REG_ADDR(inst),					 \
		.clk_cfg = DT_INST_PHA(inst, clocks, clk_cfg),			 \
		.default_bitrate = DT_INST_PROP(inst, clock_frequency),		 \
		.irq = DT_INST_IRQN(inst),					 \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			 \
	};									 \
										 \
	static struct i2c_npcm_data i2c_npcm_data_##inst;			 \
										 \
	DEVICE_DT_INST_DEFINE(inst,						 \
			      I2C_NPCM_CTRL_INIT_FUNC(inst),			 \
			      NULL,						 \
			      &i2c_npcm_data_##inst, &i2c_npcm_cfg_##inst,	 \
			      PRE_KERNEL_1, CONFIG_I2C_INIT_PRIORITY,		 \
			      &i2c_npcm_driver_api);				 \
										 \
	I2C_NPCM_CTRL_INIT_FUNC_IMPL(inst)

DT_INST_FOREACH_STATUS_OKAY(I2C_NPCM_CTRL_INIT)
