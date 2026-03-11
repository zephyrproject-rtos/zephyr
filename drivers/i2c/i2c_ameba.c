/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Realtek Ameba I2C interface
 */
#define DT_DRV_COMPAT realtek_ameba_i2c

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ameba, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#if CONFIG_I2C_ASYNC_API
#include <zephyr/drivers/dma/dma_ameba_gdma.h>
#include <zephyr/drivers/dma.h>

struct i2c_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	uint8_t *buffer;
	size_t buffer_length;
};
#endif

struct i2c_ameba_data {
	uint32_t master_mode;
	uint32_t addr_mode;
	uint32_t slave_address;
	struct i2c_msg *current;
	volatile int flag_done;
#if defined(CONFIG_I2C_AMEBA_INTERRUPT)
	I2C_IntModeCtrl i2c_intctrl;
#endif
#ifdef CONFIG_I2C_ASYNC_API
	i2c_callback_t async_cb;
	void *async_user_data;
	struct i2c_dma_stream dma_rx;
	struct i2c_dma_stream dma_tx;
	volatile int flag_dma_done;
#endif
};

typedef void (*irq_connect_cb)(void);

struct i2c_ameba_config {
	I2C_TypeDef *I2Cx;
	uint32_t bitrate;
	const struct pinctrl_dev_config *pcfg;
	const clock_control_subsys_t clock_subsys;
	const struct device *clock_dev;
#if defined(CONFIG_I2C_AMEBA_INTERRUPT)
	void (*irq_cfg_func)(void);
#endif
#if CONFIG_I2C_ASYNC_API
	const struct device *dma_dev;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_channel;
#endif
};

#if defined(CONFIG_I2C_AMEBA_INTERRUPT)
struct k_sem txSemaphore;
struct k_sem rxSemaphore;

static void i2c_give_sema(u32 IsWrite)
{
	if (IsWrite) {
		k_sem_give(&txSemaphore);
	} else {
		k_sem_give(&rxSemaphore);
	}
}

static void i2c_take_sema(u32 IsWrite)
{
	if (IsWrite) {
		k_sem_take(&txSemaphore, K_FOREVER);
	} else {
		k_sem_take(&rxSemaphore, K_FOREVER);
	}
}
#endif

#if defined(CONFIG_I2C_AMEBA_INTERRUPT)
static void i2c_ameba_isr(const struct device *dev)
{
	struct i2c_ameba_data *data = dev->data;

	uint32_t intr_status = I2C_GetINT(data->i2c_intctrl.I2Cx);

	if (intr_status & I2C_BIT_R_STOP_DET) {
		data->flag_done = 1;
		/* Clear I2C interrupt */
		I2C_ClearINT(data->i2c_intctrl.I2Cx, I2C_BIT_R_STOP_DET);
	}

	I2C_ISRHandle(&data->i2c_intctrl);
}
#endif

#ifdef CONFIG_I2C_ASYNC_API
void i2c_ameba_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			 int status)
{
	struct device *dev = user_data;
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->I2Cx;

	I2C_DMAControl(i2c, I2C_BIT_TDMAE, DISABLE);
	data->flag_dma_done = 1;

	while (0 == I2C_CheckFlagState(i2c, I2C_BIT_TFE)) {
	}

	I2C_ClearAllINT(i2c);

	if (dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel) < 0) {
		LOG_ERR("Stop tx ext dma failed !!");
	}

	if (status < 0) {
		LOG_ERR("DMA tx is in error state.");
	}

	I2C_Cmd(i2c, DISABLE);
}

void i2c_ameba_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			 int status)
{
	struct device *dev = user_data;
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->I2Cx;

	I2C_DMAControl(i2c, I2C_BIT_RDMAE, DISABLE);
	data->flag_dma_done = 1;

	while (0 == I2C_CheckFlagState(i2c, I2C_BIT_TFE)) {
	}

	I2C_ClearAllINT(i2c);

	if (dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel) < 0) {
		LOG_ERR("Stop tx ext dma failed !!");
	}

	if (status < 0) {
		LOG_ERR("DMA rx is in error state.");
	}
}

static int i2c_tx_dma_config(const struct device *dev, u8 *pdata, u32 length)
{
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	struct i2c_dma_stream *i2c_dma = NULL;

	i2c_dma = &data->dma_tx;
	uint32_t handshake_index = i2c_dma->dma_cfg.dma_slot;

	memset(&i2c_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&i2c_dma->blk_cfg, 0, sizeof(struct dma_block_config));
	i2c_dma->dma_cfg.head_block = &i2c_dma->blk_cfg;
	i2c_dma->blk_cfg.source_address = (u32)pdata;
	i2c_dma->blk_cfg.dest_address = (u32)&cfg->I2Cx->IC_DATA_CMD;
	i2c_dma->dma_cfg.dma_slot = handshake_index;
	i2c_dma->dma_cfg.dma_callback = i2c_ameba_dma_tx_cb;

	i2c_dma->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	i2c_dma->dma_cfg.source_handshake = 1;
	i2c_dma->dma_cfg.dest_handshake = 0;
	i2c_dma->dma_cfg.channel_priority = 1; /* ? */

	/* Configure GDMA transfer */
	/* 24bits or 16bits mode */
	if (((length & 0x03) == 0) && (((u32)(pdata) & 0x03) == 0)) {
		LOG_INF(" 4-bytes aligned");
		/* 4-bytes aligned, move 4 bytes each transfer */
		i2c_dma->dma_cfg.source_burst_length = 1;
		i2c_dma->dma_cfg.source_data_size = 4;
		i2c_dma->blk_cfg.block_size = length;
	} else {
		LOG_INF(" not 4-bytes aligned");
		/* 2-bytes aligned, move 2 bytes each transfer */
		i2c_dma->dma_cfg.source_burst_length = 4;
		i2c_dma->dma_cfg.source_data_size = 1;
		i2c_dma->blk_cfg.block_size = length;
	}

	i2c_dma->dma_cfg.block_count = 1;
	i2c_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	i2c_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	i2c_dma->blk_cfg.source_reload_en = 0;
	i2c_dma->blk_cfg.dest_reload_en = 0;
	i2c_dma->blk_cfg.flow_control_mode = 0;

	i2c_dma->dma_cfg.dest_data_size = 1;
	i2c_dma->dma_cfg.dest_burst_length = 4;
	i2c_dma->dma_cfg.user_data = (void *)dev;

	i2c_dma->buffer = pdata;
	i2c_dma->buffer_length = length;

	dma_config(i2c_dma->dma_dev, i2c_dma->dma_channel, &(i2c_dma->dma_cfg));
	return 0;
}

static int i2c_rx_dma_config(const struct device *dev, u8 *pdata, u32 length)
{
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	struct i2c_dma_stream *i2c_dma = NULL;

	i2c_dma = &data->dma_rx;
	uint32_t handshake_index = i2c_dma->dma_cfg.dma_slot;

	memset(&i2c_dma->dma_cfg, 0, sizeof(struct dma_config));
	memset(&i2c_dma->blk_cfg, 0, sizeof(struct dma_block_config));
	i2c_dma->dma_cfg.head_block = &i2c_dma->blk_cfg;

	i2c_dma->blk_cfg.source_address = (u32)&cfg->I2Cx->IC_DATA_CMD;
	i2c_dma->dma_cfg.dma_slot = handshake_index;
	i2c_dma->dma_cfg.dma_callback = i2c_ameba_dma_rx_cb;

	i2c_dma->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	i2c_dma->dma_cfg.error_callback_dis = 1;
	i2c_dma->dma_cfg.source_handshake = 0;
	i2c_dma->dma_cfg.dest_handshake = 1;
	i2c_dma->dma_cfg.channel_priority = 1;

	if (((u32)(pdata) & 0x03) == 0) {
		/* 4-bytes aligned, move 4 bytes each transfer */
		i2c_dma->dma_cfg.dest_burst_length = 1;
		i2c_dma->dma_cfg.dest_data_size = 4;
	} else {
		/* 2-bytes aligned, move 2 bytes each transfer */
		i2c_dma->dma_cfg.dest_burst_length = 4;
		i2c_dma->dma_cfg.dest_data_size = 1;
	}

	i2c_dma->blk_cfg.block_size = length;

	i2c_dma->dma_cfg.block_count = 1;
	i2c_dma->blk_cfg.dest_address = (uint32_t)pdata;
	i2c_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	i2c_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	i2c_dma->blk_cfg.source_reload_en = 0;
	i2c_dma->blk_cfg.dest_reload_en = 0;
	i2c_dma->blk_cfg.flow_control_mode = 0;

	i2c_dma->dma_cfg.source_data_size = 1;
	i2c_dma->dma_cfg.source_burst_length = 4;
	i2c_dma->dma_cfg.user_data = (void *)dev;

	i2c_dma->buffer = pdata;
	i2c_dma->buffer_length = length;

	return dma_config(i2c_dma->dma_dev, i2c_dma->dma_channel, &i2c_dma->dma_cfg);
}

static int i2c_send_dma_master(const struct device *dev, u8 *pdata, u32 length)
{
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->I2Cx;

	if (i2c_tx_dma_config(dev, pdata, length) < 0) {
		LOG_ERR("i2s tx dma config failed.");
		return -EIO;
	}
	dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	I2C_DmaMode1Config(i2c, I2C_BIT_DMODE_ENABLE | I2C_BIT_DMODE_STOP, length);
	I2C_DMAControl(i2c, I2C_BIT_TDMAE, ENABLE);

	return 0;
}

static int i2c_send_dma_slave(const struct device *dev, u8 *pdata, u32 length)
{
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->I2Cx;

	if (i2c_tx_dma_config(dev, pdata, length) < 0) {
		LOG_ERR("i2s tx dma config failed.");
		return -EIO;
	}
	dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	I2C_DmaMode1Config(i2c, I2C_BIT_DMODE_ENABLE, length);
	while ((i2c->IC_RAW_INTR_STAT & I2C_BIT_RD_REQ) == 0) {
	}
	I2C_DMAControl(i2c, I2C_BIT_TDMAE, ENABLE);

	return 0;
}

static int i2c_receive_dma_master(const struct device *dev, u8 *pdata, u32 length)
{
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->I2Cx;

	if (i2c_rx_dma_config(dev, (u8 *)pdata, length) < 0) {
		LOG_ERR("i2s rx dma config failed.");
		return -EIO;
	}
	dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
	I2C_DmaMode1Config(i2c, I2C_BIT_DMODE_ENABLE | I2C_BIT_DMODE_STOP | I2C_BIT_DMODE_CMD,
			   length);
	I2C_DMAControl(i2c, I2C_BIT_RDMAE, ENABLE);

	return 0;
}

static int i2c_receive_dma_slave(const struct device *dev, u8 *pdata, u32 length)
{
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *cfg = dev->config;
	I2C_TypeDef *i2c = cfg->I2Cx;

	if (i2c_rx_dma_config(dev, (u8 *)pdata, length) < 0) {
		LOG_ERR("i2s rx dma config failed.");
		return -EIO;
	}
	dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
	I2C_DmaMode1Config(i2c, I2C_BIT_DMODE_ENABLE, length);
	I2C_DMAControl(i2c, I2C_BIT_RDMAE, ENABLE);

	return 0;
}
#endif

static int i2c_ameba_configure(const struct device *dev, uint32_t dev_config)
{
	I2C_InitTypeDef I2C_InitStruct;

	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *config = dev->config;

	I2C_TypeDef *i2c = config->I2Cx;
	int err = 0;

	/* Disable I2C device */
	I2C_Cmd(i2c, DISABLE);

	I2C_StructInit(&I2C_InitStruct);

	if (dev_config & I2C_MODE_CONTROLLER) {
		I2C_InitStruct.I2CMaster = I2C_MASTER_MODE;
	} else {
		I2C_InitStruct.I2CMaster = I2C_SLAVE_MODE;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		I2C_InitStruct.I2CAddrMod = I2C_ADDR_10BIT;
	} else {
		I2C_InitStruct.I2CAddrMod = I2C_ADDR_7BIT;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		I2C_InitStruct.I2CSpdMod = I2C_SS_MODE;
		I2C_InitStruct.I2CClk = (I2C_BITRATE_STANDARD / 1000);
		break;
	case I2C_SPEED_FAST:
		I2C_InitStruct.I2CSpdMod = I2C_FS_MODE;
		I2C_InitStruct.I2CClk = (I2C_BITRATE_FAST / 1000);
		break;
	case I2C_SPEED_HIGH:
		I2C_InitStruct.I2CSpdMod = I2C_HS_MODE;
		I2C_InitStruct.I2CClk = (I2C_BITRATE_HIGH / 1000);
		break;
	default:
		err = -EINVAL;
		goto error;
	}

	data->master_mode = I2C_InitStruct.I2CMaster;
	data->addr_mode = I2C_InitStruct.I2CAddrMod;

	I2C_Init(i2c, &I2C_InitStruct);

error:
	return err;
}

static int i2c_ameba_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	struct i2c_ameba_data *data = dev->data;
	const struct i2c_ameba_config *config = dev->config;

	I2C_TypeDef *i2c = config->I2Cx;
	struct i2c_msg *current, *next;
	int err = 0;

	current = msgs;

	/* First message flags implicitly contain I2C_MSG_RESTART flag. */
	current->flags |= I2C_MSG_RESTART;
	/*restart check for write or read dirction*/
	for (uint8_t i = 1; i <= num_msgs; i++) {
		if (i < num_msgs) {
			next = current + 1;

			/*
			 * If there have a R/W transfer state change between messages,
			 * An explicit I2C_MSG_RESTART flag is needed for the second message.
			 */
			if ((current->flags & I2C_MSG_RW_MASK) != (next->flags & I2C_MSG_RW_MASK)) {
				if ((next->flags & I2C_MSG_RESTART) == 0U) {
					return -EINVAL;
				}
			}

			/* Only the last message need I2C_MSG_STOP flag to free the Bus. */
			if (current->flags & I2C_MSG_STOP) {
				return -EINVAL;
			}
		} else {
			/* Last message flags implicitly contain I2C_MSG_STOP flag. */
			current->flags |= I2C_MSG_STOP;
		}

		if ((current->buf == NULL) || (current->len == 0U)) {
			return -EINVAL;
		}

		current++;
	}

	I2C_SetSlaveAddress(i2c, addr);
	/* Enable i2c device */
	I2C_Cmd(i2c, ENABLE);
	data->slave_address = addr;

#if defined(CONFIG_I2C_AMEBA_INTERRUPT)
	for (uint8_t i = 0; i < num_msgs; ++i) {
		data->current = &msgs[i];
		if (data->master_mode == 1) {
			data->flag_done = 0;
			I2C_INTConfig(i2c, I2C_BIT_R_STOP_DET, ENABLE);

			if ((data->current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				I2C_MasterWriteInt(i2c, &data->i2c_intctrl, data->current->buf,
						   data->current->len);
				while (data->flag_done == 0) {
				}
			} else {
				I2C_MasterReadInt(i2c, &data->i2c_intctrl, data->current->buf,
						  data->current->len);
				while (data->flag_done == 0) {
				}
			}
		}
	}
#elif defined(CONFIG_I2C_ASYNC_API)
	for (uint8_t i = 0; i < num_msgs; ++i) {
		k_sleep(K_MSEC(5));
		data->current = &msgs[i];
		data->flag_dma_done = 0;
		if (data->master_mode == 1) {
			if ((data->current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				i2c_send_dma_master(dev, data->current->buf, data->current->len);
				while (data->flag_dma_done == 0) {
				}
			} else {
				i2c_receive_dma_master(dev, data->current->buf, data->current->len);
				while (data->flag_dma_done == 0) {
				}
			}
		} else {
			if ((data->current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				i2c_send_dma_slave(dev, data->current->buf, data->current->len);
				while (data->flag_dma_done == 0) {
				}
			} else {
				i2c_receive_dma_slave(dev, data->current->buf, data->current->len);
				while (data->flag_dma_done == 0) {
				}
			}
		}
	}
#else
	for (uint8_t i = 0; i < num_msgs; ++i) {
		k_sleep(K_MSEC(5));
		data->current = &msgs[i];
		if (data->master_mode == 1) {
			if ((data->current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				I2C_MasterWrite(i2c, data->current->buf, data->current->len);
			} else {
				I2C_MasterRead(i2c, data->current->buf, data->current->len);
			}
		} else {
			if ((data->current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				I2C_SlaveWrite(i2c, data->current->buf, data->current->len);
			} else {
				I2C_SlaveRead(i2c, data->current->buf, data->current->len);
			}
		}
	}
#endif
	return err;
}

static DEVICE_API(i2c, i2c_ameba_driver_api) = {
	.configure = i2c_ameba_configure,
	.transfer = i2c_ameba_transfer,
};

static int i2c_ameba_init(const struct device *dev)
{
	const struct i2c_ameba_config *config = dev->config;

	uint32_t bitrate_cfg;
	int err = 0;

	/*clk enable*/
	clock_control_on(config->clock_dev, (clock_control_subsys_t)config->clock_subsys);
	/* Configure pinmux  */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

#if defined(CONFIG_I2C_AMEBA_INTERRUPT)
	struct i2c_ameba_data *data = dev->data;

	k_sem_init(&txSemaphore, 1, 1);
	k_sem_init(&rxSemaphore, 1, 1);

	k_sem_take(&txSemaphore, K_FOREVER);
	k_sem_take(&rxSemaphore, K_FOREVER);

	data->i2c_intctrl.I2Cx = config->I2Cx;
	data->i2c_intctrl.I2CSendSem = i2c_give_sema;
	data->i2c_intctrl.I2CWaitSem = i2c_take_sema;
	config->irq_cfg_func();
#endif

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	i2c_ameba_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);

	return 0;
}

#ifdef CONFIG_I2C_ASYNC_API

#define I2C_DMA_CHANNEL_INIT(index, dir)                                                           \
	.dma_dev = AMEBA_DT_INST_DMA_CTLR(index, dir),                                             \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = AMEBA_DMA_CONFIG(index, dir, 1, i2c_ameba_dma_##dir##_cb),

#endif
/* define macro for dma_rx or dma_tx */
#ifdef CONFIG_I2C_ASYNC_API
#define I2C_DMA_CHANNEL(index, dir)                                                                \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),        \
					(I2C_DMA_CHANNEL_INIT(index, dir)),           \
					(NULL)) },
#else
#define I2C_DMA_CHANNEL(index, dir)
#endif

#if defined(CONFIG_I2C_AMEBA_INTERRUPT)
#define AMEBA_I2C_IRQ_HANDLER_DECL(n) static void i2c_ameba_irq_config_func_##n(void);
#define AMEBA_I2C_IRQ_HANDLER(n)                                                                   \
	static void i2c_ameba_irq_config_func_##n(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_ameba_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define AMEBA_I2C_IRQ_HANDLER_FUNC(n) .irq_cfg_func = i2c_ameba_irq_config_func_##n,
#else
#define AMEBA_I2C_IRQ_HANDLER_DECL(n) /* Not used */
#define AMEBA_I2C_IRQ_HANDLER(n)      /* Not used */
#define AMEBA_I2C_IRQ_HANDLER_FUNC(n) /* Not used */
#endif

#define AMEBA_I2C_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	AMEBA_I2C_IRQ_HANDLER_DECL(n)                                                              \
	static struct i2c_ameba_data i2c_ameba_data_##n = {I2C_DMA_CHANNEL(n, rx)                  \
								   I2C_DMA_CHANNEL(n, tx)};        \
                                                                                                   \
	static const struct i2c_ameba_config i2c_ameba_config_##n = {                              \
		.I2Cx = (I2C_TypeDef *)DT_INST_REG_ADDR(n),                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, idx),               \
		AMEBA_I2C_IRQ_HANDLER_FUNC(n)};                                                    \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_ameba_init, NULL, &i2c_ameba_data_##n,                    \
				  &i2c_ameba_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,    \
				  &i2c_ameba_driver_api);                                          \
	AMEBA_I2C_IRQ_HANDLER(n)
DT_INST_FOREACH_STATUS_OKAY(AMEBA_I2C_INIT)
