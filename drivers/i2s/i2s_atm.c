/*
 * Copyright (C) Atmosic 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atm_i2s

#include <stdio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_PM
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#endif

#include "arch.h"
#include "at_clkrstgen.h"
#include "at_wrpr.h"
#include "at_pinmux.h"
#include "at_i2s_regs_core_macro.h"
#include "atm_utils_math.h"
LOG_MODULE_REGISTER(i2s_atm, CONFIG_I2S_LOG_LEVEL);

BUILD_ASSERT(IS_ENABLED(CONFIG_ATM_DMA), "I2S requires ATM DMA");
#include "dma.h"

#define PINGPONG_BUF_SIZE 16
#define SRC_CNT_DEFAULT   1
#define SNK_CNT_DEFAULT   1

enum {
	PINGPONG_BUF0,
	PINGPONG_BUF1,
	PINGPONG_BUF2,
	PINGPONG_BUF,
	PINGPONG_BUF_NUM,
};

enum {
	SRC_SNK_PCM,
	SRC_SNK_AHB,
	SRC_SNK_LPBK,
};

#define I2S_TX_BIT  0
#define I2S_RX_BIT  1
#define I2S_TX_MASK (1 << I2S_TX_BIT)
#define I2S_RX_MASK (1 << I2S_RX_BIT)

typedef enum {
	I2S_MODE_PCM,
	I2S_MODE_LEFT_JUSTIFIED,
	I2S_MODE_RIGHT_JUSTIFIED,
} i2s_mode_t;

#define IRQ_SOURCE_UF                                                                              \
	(ATI2S_I2S_IRQM0__PP0_UF__MASK | ATI2S_I2S_IRQM0__PP1_UF__MASK |                           \
	 ATI2S_I2S_IRQM0__PP2_UF__MASK | ATI2S_I2S_IRQM0__PP3_UF__MASK)

#define IRQ_SOURCE_EP_THRSHLD ATI2S_I2S_IRQM0__PP_EP_THRSHLD__MASK

#define IRQ_SOURCE_TX (IRQ_SOURCE_UF | IRQ_SOURCE_EP_THRSHLD)

#define IRQ_STATUS_UF                                                                              \
	(ATI2S_I2S_IRQ0__PP0_UF__MASK | ATI2S_I2S_IRQ0__PP1_UF__MASK |                             \
	 ATI2S_I2S_IRQ0__PP2_UF__MASK | ATI2S_I2S_IRQ0__PP3_UF__MASK)

#define IRQ_STATUS_EP_THRSHLD ATI2S_I2S_IRQ0__PP_EP_THRSHLD__MASK

#define IRQ_STATUS_TX (IRQ_STATUS_UF | IRQ_STATUS_EP_THRSHLD)
typedef struct i2s_cfg_txrx_s {
	uint16_t sck2ws_rt; // serial clock (sck) to word select (ws) ratio
	uint16_t ck2sck_rt; // clock to serial clock ratio
	bool ws_nedge_st;   // tx/rx starts on negative edge of word select
	bool sck_nedge_sd;  // drive/latch tx/rx sd on positive/negative edge of sd
	bool ws_init;       // ws is (de)asserted at starting half of justified mode
	bool mstr_sckws;    // sck and ws (master = 1, slave = 0)
	bool sck_nedge_ws;  // drive/latch tx/rx sd on positive/negative edge of ws
	uint8_t sdw;        // width of valid serial data (1 to 32)
	uint8_t sd_offset;  // cycles (0 to 15) after ws edge to wait before data
} i2s_cfg_txrx_t;

typedef struct i2s_cfg_s {
	i2s_cfg_txrx_t trx;
	i2s_mode_t mode;
	uint8_t sck_init_cnt;
	uint32_t aud_ctrl_i2s;
} i2s_cfg_t;

struct trx_block {
	void *buffer;
	size_t size;
};
struct i2s_atm_stream {
	enum i2s_state state;
	struct k_mem_slab *mem_slab;
	struct k_msgq *queue;
	struct trx_block cur_block;
	bool stop_drain;
};

struct i2s_atm_config {
	void (*fn_cfg_tx_pin)(void);
	uint32_t sys_clk_freq;
};

struct i2s_atm_data {
	enum i2s_dir dir;
	i2s_cfg_t cfg;
	struct i2s_config const *i2s_cfg;
	struct i2s_atm_stream tx;
	struct i2s_atm_stream rx;
	struct device const *dev;
#ifdef CONFIG_PM
	bool pm_tx_constraint_on;
#endif
};

#ifdef CONFIG_PM
static void i2s_atm_pm_tx_constraint_set(const struct device *dev)
{
	struct i2s_atm_data *data = dev->data;

	if (!data->pm_tx_constraint_on) {
		data->pm_tx_constraint_on = true;
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
	}
}

static void i2s_atm_pm_tx_constraint_release(const struct device *dev)
{
	struct i2s_atm_data *data = dev->data;

	if (data->pm_tx_constraint_on) {
		data->pm_tx_constraint_on = false;
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
	}
}
#endif

#define I2S_ATM_MAX_TX_BLOCK 20
#define I2S_ATM_MAX_RX_BLOCK 0
K_MSGQ_DEFINE(tx_queue, sizeof(struct trx_block), I2S_ATM_MAX_TX_BLOCK, sizeof(void *));
K_MSGQ_DEFINE(rx_queue, sizeof(struct trx_block), I2S_ATM_MAX_TX_BLOCK, sizeof(void *));

static int i2s_config_convert(const struct device *dev, const struct i2s_config *cfg,
			      i2s_cfg_t *cfg_atm)
{
	i2s_cfg_txrx_t *trx = &cfg_atm->trx;

	switch (cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S: {
		trx->ws_nedge_st = true;
		trx->sck_nedge_sd = false;
		trx->sck_nedge_ws = false;
		trx->sd_offset = 1;
		cfg_atm->mode = I2S_MODE_LEFT_JUSTIFIED;
	} break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
	case I2S_FMT_DATA_FORMAT_PCM_LONG: {
		trx->ws_nedge_st = false;
		trx->sck_nedge_sd = true;
		trx->sck_nedge_ws = true;
		trx->ws_init = true;
		trx->sd_offset = 1;
		cfg_atm->mode = I2S_MODE_PCM;
	} break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED: {
		trx->ws_nedge_st = false;
		trx->sck_nedge_sd = false;
		trx->sck_nedge_ws = false;
		trx->ws_init = false;
		cfg_atm->mode = I2S_MODE_LEFT_JUSTIFIED;
	} break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED: {
		trx->ws_nedge_st = false;
		trx->sck_nedge_sd = false;
		trx->sck_nedge_ws = false;
		trx->ws_init = false;
		cfg_atm->mode = I2S_MODE_RIGHT_JUSTIFIED;
	} break;
	default:
		LOG_ERR("Unsupported data format");
		return -EINVAL;
	}

	trx->sdw = cfg->word_size;
	if (cfg->word_size != 16) {
		LOG_ERR("Unsupported word size");
		return -EINVAL;
	}
	trx->mstr_sckws = !(cfg->options & I2S_OPT_FRAME_CLK_SLAVE);

#define I2S_16M_CLK 0
#define I2S_32M_CLK 1
	uint32_t i2s_clks[] = {
		[I2S_16M_CLK] = 16000000,
		[I2S_32M_CLK] = 32000000,
	};
	int i2s_clk_cnt = ARRAY_SIZE(i2s_clks);
	if (((struct i2s_atm_config *)dev->config)->sys_clk_freq < 32000000) {
		i2s_clk_cnt = 1;
	}

	uint16_t ws_cnt[] = {
#if CONFIG_I2S_WSCNT
		CONFIG_I2S_WSCNT
#else
		16, 24, 32
#endif
	};

	uint32_t nearest_diff = 0xffffffff;
	for (int i = 0; i < i2s_clk_cnt; i++) {
		for (int j = 0; j < ARRAY_SIZE(ws_cnt); j++) {
			if (ws_cnt[j] < trx->sd_offset + trx->sdw) {
				continue;
			}
			uint16_t ck2sck = DIV_ROUND_CLOSEST(i2s_clks[i] / (ws_cnt[j] * 2),
							    cfg->frame_clk_freq);
			uint32_t real_frame_freq = i2s_clks[i] / ck2sck / (ws_cnt[j] * 2);
			uint32_t diff = ATM_ABS(real_frame_freq - cfg->frame_clk_freq);
			LOG_DBG("i2s_clks[%d] = %d, ws_cnt[%d] = %d, ck2sck = %d, real_frame_freq "
				"= %d, diff = %d\n",
				i, i2s_clks[i], j, ws_cnt[j], ck2sck, real_frame_freq, diff);
			if (diff < nearest_diff) {
				nearest_diff = diff;
				trx->sck2ws_rt = ws_cnt[j];
				trx->ck2sck_rt = ck2sck;
				cfg_atm->aud_ctrl_i2s = i;
			}
		}
	}
	LOG_DBG("ck2ws_rt = %d, ck2sck_rt = %d, aud_ctrl_i2s = %d\n", trx->sck2ws_rt,
		trx->ck2sck_rt, cfg_atm->aud_ctrl_i2s);

	if (nearest_diff == 0xffffffff) {
		LOG_ERR("Unsupported frame clock frequency");
		return -EINVAL;
	}
	return 0;
}

static int i2s_atm_configure(const struct device *dev, enum i2s_dir dir,
			     const struct i2s_config *cfg)
{

	struct i2s_atm_config const *i2s_config = dev->config;
	struct i2s_atm_data *i2s_data = dev->data;
	i2s_cfg_t *cfg_atm = &i2s_data->cfg;

	// Support TX currently
	if ((dir != I2S_DIR_TX) || i2s_data->i2s_cfg || i2s_config_convert(dev, cfg, cfg_atm) < 0) {
		return -EINVAL;
	}

	i2s_data->i2s_cfg = cfg;

	CMSDK_CLKRSTGEN_NONSECURE->CLK_AUD_CTRL =
		CLKRSTGEN_CLK_AUD_CTRL__I2S_SEL__WRITE(cfg_atm->aud_ctrl_i2s) |
		CLKRSTGEN_CLK_AUD_CTRL__I2S_CLK_ENABLE__WRITE(1);

	WRPR_CTRL_SET(CMSDK_I2S, WRPR_CTRL__CLK_ENABLE | WRPR_CTRL__SRESET);
	WRPR_CTRL_SET(CMSDK_I2S, WRPR_CTRL__CLK_ENABLE);

	CMSDK_I2S_NONSECURE->I2S_CTRL0 =
		ATI2S_I2S_CTRL0__SCK_INIT_CNT__WRITE(cfg_atm->sck_init_cnt) |
		ATI2S_I2S_CTRL0__SRC_SNK__WRITE(SRC_SNK_AHB);

	if (dir == I2S_DIR_TX) {

		CMSDK_I2S_NONSECURE->I2S_CTRL0 |=
			(ATI2S_I2S_CTRL0__WS_NEDGE_ST_TX__WRITE(cfg_atm->trx.ws_nedge_st) |
			 ATI2S_I2S_CTRL0__SCK_NEDGE_SD_TX__WRITE(cfg_atm->trx.sck_nedge_sd) |
			 ATI2S_I2S_CTRL0__WS_INIT_TX__WRITE(cfg_atm->trx.ws_init) |
			 ATI2S_I2S_CTRL0__SRC_CNT__WRITE(SRC_CNT_DEFAULT) |
			 ATI2S_I2S_CTRL0__MSTR_SCKWS_TX__WRITE(cfg_atm->trx.mstr_sckws) |
			 ATI2S_I2S_CTRL0__DMA_EN__WRITE(true) |
			 ATI2S_I2S_CTRL0__SCK_NEDGE_WS_TX__WRITE(cfg_atm->trx.sck_nedge_ws));

		CMSDK_I2S_NONSECURE->I2S_CTRL1_TX =
			ATI2S_I2S_CTRL1_TX__SCK2WS_RT__WRITE(cfg_atm->trx.sck2ws_rt) |
			ATI2S_I2S_CTRL1_TX__CK2SCK_RT__WRITE(cfg_atm->trx.ck2sck_rt);

		ASSERT_ERR((cfg_atm->trx.sdw <= cfg_atm->trx.sck2ws_rt) &&
			   (cfg_atm->trx.sdw >= 1) && (cfg_atm->trx.sdw <= 32));
		uint8_t pb_cnt1 = (cfg_atm->mode == I2S_MODE_RIGHT_JUSTIFIED)
					  ? (cfg_atm->trx.sck2ws_rt - cfg_atm->trx.sdw)
					  : 0;
		CMSDK_I2S_NONSECURE->I2S_CTRL2_TX =
			ATI2S_I2S_CTRL2_TX__PB_CNT__WRITE(pb_cnt1) |
			ATI2S_I2S_CTRL2_TX__SDW__WRITE(cfg_atm->trx.sdw) |
			ATI2S_I2S_CTRL2_TX__SD_OFFST__WRITE(cfg_atm->trx.sd_offset) |
			ATI2S_I2S_CTRL2_TX__WSSD_MD__WRITE(cfg_atm->mode);

		ATI2S_I2S_CTRL3__USE_MSB_SMPL__CLR(CMSDK_I2S_NONSECURE->I2S_CTRL3);

		i2s_config->fn_cfg_tx_pin();
		i2s_data->tx.state = I2S_STATE_READY;

		i2s_data->tx.mem_slab = cfg->mem_slab;
		CMSDK_I2S_NONSECURE->I2S_IRQM0 = IRQ_SOURCE_TX;
	} else {
		return -EINVAL;
	}

	i2s_data->dir = dir;

	return 0;
}

static void dma_i2s_tx_free_cur(struct i2s_atm_stream *stream)
{
	if (stream->cur_block.buffer) {
		k_mem_slab_free(stream->mem_slab, stream->cur_block.buffer);
		stream->cur_block.buffer = NULL;
		stream->cur_block.size = 0;
	}
}

static void i2s_queue_drop(struct device const *dev)
{
	struct i2s_atm_data *i2s_data = dev->data;
	struct i2s_atm_stream *stream = &i2s_data->tx;
	struct trx_block block;

	while (!k_msgq_get(stream->queue, &block, K_NO_WAIT)) {
		k_mem_slab_free(stream->mem_slab, block.buffer);
	}
}

static void dma_i2s_tx_callback(void const *ctx)
{
	const struct device *dev = ctx;
	struct i2s_atm_data const *i2s_data = dev->data;
	struct i2s_atm_stream *stream = (struct i2s_atm_stream *)&i2s_data->tx;
	if (!stream->cur_block.buffer) {
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("TX block NULL. state: %u", stream->state);
		}
		return;
	}

	dma_i2s_tx_free_cur(stream);

	if (stream->state == I2S_STATE_STOPPING) {
		if (!stream->stop_drain) {
			i2s_queue_drop(dev);
			return;
		}
	}

	int ret = k_msgq_get(stream->queue, &stream->cur_block, K_NO_WAIT);
	if (ret < 0) {
		if (stream->state == I2S_STATE_STOPPING && stream->stop_drain) {
			return;
		}
		stream->state = I2S_STATE_ERROR;
		return;
	}

	dma_fifo_tx_async(DMA_FIFO_TX_I2S, stream->cur_block.buffer, stream->cur_block.size,
			  dma_i2s_tx_callback, dev);
}

static int i2s_tx_start_transfer(const struct device *dev)
{
	struct i2s_atm_data *i2s_data = dev->data;
	struct i2s_atm_stream *stream = &i2s_data->tx;
	int ret = k_msgq_get(stream->queue, &stream->cur_block, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("TX queue empty");
		return ret;
	}

	dma_fifo_tx_async(DMA_FIFO_TX_I2S, stream->cur_block.buffer, stream->cur_block.size,
			  dma_i2s_tx_callback, dev);

	NVIC_ClearPendingIRQ(I2S_IRQn);
	CMSDK_I2S_NONSECURE->I2S_CTRL0 |= ATI2S_I2S_CTRL0__SRC_SNK_EN__WRITE(I2S_TX_MASK);
	NVIC_EnableIRQ(I2S_IRQn);
#ifdef CONFIG_PM
	i2s_atm_pm_tx_constraint_set(dev);
#endif
	return 0;
}

static int i2s_tx_stop_transfer(const struct device *dev)
{
	NVIC_DisableIRQ(I2S_IRQn);
	CMSDK_I2S_NONSECURE->I2S_CTRL0 &= ~ATI2S_I2S_CTRL0__SRC_SNK_EN__WRITE(I2S_TX_MASK);
#ifdef CONFIG_PM
	i2s_atm_pm_tx_constraint_release(dev);
#endif
	return 0;
}

static int i2s_atm_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_atm_data *i2s_data = dev->data;
	struct i2s_atm_stream *stream;
	switch (dir) {
	case I2S_DIR_TX:
		stream = &i2s_data->tx;
		break;
	default:
		LOG_ERR("Unsupported direction");
		return -EINVAL;
	}
	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START - Invalid state: %u", stream->state);
			return -EIO;
		}
		int ret = i2s_tx_start_transfer(dev);
		if (ret < 0) {
			LOG_ERR("Failed to start TX transfer: %d", ret);
			return ret;
		}
		stream->state = I2S_STATE_RUNNING;
		stream->stop_drain = false;
		break;
	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP - Invalid state: %u", stream->state);
			return -EIO;
		}
		stream->state = I2S_STATE_STOPPING;
		break;
	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN - Invalid state: %u", stream->state);
			return -EIO;
		}
		if (k_msgq_num_used_get(stream->queue) > 0) {
			stream->stop_drain = true;
		}
		stream->state = I2S_STATE_STOPPING;
		break;
	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP - invalid state: %u", stream->state);
			return -EIO;
		}
		stream->state = I2S_STATE_STOPPING;
		break;
	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR && stream->state != I2S_STATE_READY) {
			return -EIO;
		}
		i2s_queue_drop(dev);
		i2s_data->i2s_cfg = NULL;
		i2s_tx_stop_transfer(i2s_data->dev);
		stream->state = I2S_STATE_READY;
	}
	return 0;
}

static int i2s_atm_read(const struct device *dev, void **mem_block, size_t *size)
{
	return -ENOTSUP;
}

static int i2s_atm_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_atm_data *dev_data = dev->data;
	if (dev_data->tx.state != I2S_STATE_RUNNING && dev_data->tx.state != I2S_STATE_READY) {
		LOG_ERR("invalid state %u", dev_data->tx.state);
		ASSERT_ERR(0);
		return -EIO;
	}
	struct trx_block entry = {.buffer = mem_block, .size = size};

	int err = k_msgq_put(dev_data->tx.queue, &entry, K_MSEC(500));
	if (err < 0) {
		LOG_ERR("TX queue full");
		ASSERT_ERR(0);
		return err;
	}

	return 0;
}
static const struct i2s_config *i2s_atm_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_atm_data *i2s_data = dev->data;

	return i2s_data->i2s_cfg;
}

static const struct i2s_driver_api i2s_atm_driver_api = {
	.config_get = i2s_atm_config_get,
	.configure = i2s_atm_configure,
	.trigger = i2s_atm_trigger,
	.read = i2s_atm_read,
	.write = i2s_atm_write,
};

#define I2S_DATA_INIT(n) static struct i2s_atm_data atm_data;

DT_INST_FOREACH_STATUS_OKAY(I2S_DATA_INIT)

static void I2S_Handler(void)
{
	struct i2s_atm_stream *stream = &atm_data.tx;
	uint32_t irq_status_tx = CMSDK_I2S_NONSECURE->I2S_IRQ0;
	if (irq_status_tx & IRQ_STATUS_TX) {
		if (irq_status_tx & ATI2S_I2S_IRQ0__PP_EP_THRSHLD__MASK) {
			ATI2S_I2S_IRQC0__PP_EP_THRSHLD__SET(CMSDK_I2S_NONSECURE->I2S_IRQC0);
			ATI2S_I2S_IRQC0__PP_EP_THRSHLD__CLR(CMSDK_I2S_NONSECURE->I2S_IRQC0);
		} else if (irq_status_tx & IRQ_STATUS_UF) {
			if (stream->state == I2S_STATE_STOPPING) {
				stream->state = I2S_STATE_READY;
				i2s_tx_stop_transfer(atm_data.dev);
				atm_data.i2s_cfg = NULL;
			} else if (stream->state == I2S_STATE_RUNNING) {
				stream->state = I2S_STATE_ERROR;
			}
			if (irq_status_tx & ATI2S_I2S_IRQ0__PP0_UF__MASK) {
				ATI2S_I2S_IRQC0__PP0_UF__SET(CMSDK_I2S_NONSECURE->I2S_IRQC0);
				ATI2S_I2S_IRQC0__PP0_UF__CLR(CMSDK_I2S_NONSECURE->I2S_IRQC0);
			} else if (irq_status_tx & ATI2S_I2S_IRQ0__PP1_UF__MASK) {
				ATI2S_I2S_IRQC0__PP1_UF__SET(CMSDK_I2S_NONSECURE->I2S_IRQC0);
				ATI2S_I2S_IRQC0__PP1_UF__CLR(CMSDK_I2S_NONSECURE->I2S_IRQC0);
			} else if (irq_status_tx & ATI2S_I2S_IRQ0__PP2_UF__MASK) {
				ATI2S_I2S_IRQC0__PP2_UF__SET(CMSDK_I2S_NONSECURE->I2S_IRQC0);
				ATI2S_I2S_IRQC0__PP2_UF__CLR(CMSDK_I2S_NONSECURE->I2S_IRQC0);
			} else if (irq_status_tx & ATI2S_I2S_IRQ0__PP3_UF__MASK) {
				ATI2S_I2S_IRQC0__PP3_UF__SET(CMSDK_I2S_NONSECURE->I2S_IRQC0);
				ATI2S_I2S_IRQC0__PP3_UF__CLR(CMSDK_I2S_NONSECURE->I2S_IRQC0);
			}
		}
	}
}

static int i2s_atm_init(const struct device *dev)
{
	struct i2s_atm_data *i2s_data = dev->data;
	i2s_data->tx.queue = &tx_queue;
	i2s_data->rx.queue = &rx_queue;
	i2s_data->dev = dev;
	Z_ISR_DECLARE(I2S_IRQn, 0, I2S_Handler, NULL);
	LOG_INF("I2S ATM initialized\n");
	return 0;
}

#define I2S_BASE(n) CMSDK_I2S

#define I2S_DEVICE_INIT(n)                                                                         \
	static void i2s_atm_config_tx_pins(void)                                                   \
	{                                                                                          \
		PIN_SELECT(DT_INST_PROP(n, sck_out_pin), I2S0_SCK_OUT);                            \
		PIN_SELECT(DT_INST_PROP(n, ws_out_pin), I2S0_WS_OUT);                              \
		PIN_SELECT(DT_INST_PROP(n, sd_out_pin), I2S0_SD_OUT);                              \
	}                                                                                          \
	static struct i2s_atm_config const config = {                                              \
		.fn_cfg_tx_pin = i2s_atm_config_tx_pins,                                           \
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &i2s_atm_init, NULL, &atm_data, &config, POST_KERNEL,             \
			      CONFIG_I2S_INIT_PRIORITY, &i2s_atm_driver_api);

BUILD_ASSERT(I2S_BASE(n) == (CMSDK_AT_I2S_TypeDef *)DT_REG_ADDR(DT_NODELABEL(i2s)));
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "one instance supported");

DT_INST_FOREACH_STATUS_OKAY(I2S_DEVICE_INIT)
