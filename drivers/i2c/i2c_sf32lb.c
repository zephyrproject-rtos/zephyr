/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_i2c

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_sf32lb, CONFIG_I2C_LOG_LEVEL);

#ifdef CONFIG_I2C_SF32LB_DMA
#include <zephyr/drivers/dma/sf32lb.h>
#endif
#include <register.h>

#include "i2c-priv.h"

#define I2C_CR    offsetof(I2C_TypeDef, CR)
#define I2C_TCR   offsetof(I2C_TypeDef, TCR)
#define I2C_IER   offsetof(I2C_TypeDef, IER)
#define I2C_SR    offsetof(I2C_TypeDef, SR)
#define I2C_DBR   offsetof(I2C_TypeDef, DBR)
#define I2C_SAR   offsetof(I2C_TypeDef, SAR)
#define I2C_LCR   offsetof(I2C_TypeDef, LCR)
#define I2C_WCR   offsetof(I2C_TypeDef, WCR)
#define I2C_RCCR  offsetof(I2C_TypeDef, RCCR)
#define I2C_BMR   offsetof(I2C_TypeDef, BMR)
#define I2C_DNR   offsetof(I2C_TypeDef, DNR)
#define I2C_RSVD1 offsetof(I2C_TypeDef, RSVD1)
#define I2C_FIFO  offsetof(I2C_TypeDef, FIFO)

#define I2C_MODE_STD    (0x00U)
#define I2C_MODE_FS     (0x01U)
#define I2C_MODE_HS_STD (0x02U)
#define I2C_MODE_HS_FS  (0x03U)

#define I2C_TIMEOUT_MS (50U)

struct i2c_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	struct sf32lb_clock_dt_spec clock;
	uint32_t bitrate;
#ifdef CONFIG_I2C_SF32LB_INTERRUPT
	void (*irq_cfg_func)(const struct device *dev);
#endif
#ifdef CONFIG_I2C_SF32LB_DMA
	struct sf32lb_dma_dt_spec dma_rx;
	struct sf32lb_dma_dt_spec dma_tx;
#endif
};

struct i2c_sf32lb_data {
	struct k_mutex lock;
	struct k_sem i2c_compl;
};

#if defined(CONFIG_I2C_SF32LB_INTERRUPT) || defined(CONFIG_I2C_SF32LB_DMA)
static void i2c_sf32lb_isr(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
#ifdef CONFIG_I2C_SF32LB_INTERRUPT
#endif

#ifdef CONFIG_I2C_SF32LB_DMA
	if (sys_test_bit(config->base + I2C_SR, I2C_SR_DMADONE_Pos)) {
		sys_set_bit((config->base + I2C_SR), I2C_SR_DMADONE_Pos);
	}
	k_sem_give(&data->i2c_compl);
#endif
}
#endif

static int i2c_sf32lb_send_addr(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;
	uint32_t sr = sys_read32(cfg->base + I2C_SR);

	addr = addr << 1;
	if (i2c_is_read_op(msg)) {
		addr |= 1;
	}

	tcr |= I2C_TCR_START;
	tcr |= I2C_TCR_TB;

	if ((msg->len == 0) && i2c_is_stop_op(msg)) {
		tcr |= I2C_TCR_STOP;
	}

	sys_write32(0, cfg->base + I2C_IER);
	sys_clear_bits(cfg->base + I2C_CR, I2C_CR_DMAEN | I2C_CR_LASTSTOP | I2C_CR_LASTNACK);
	sys_clear_bits(cfg->base + I2C_SR, I2C_SR_TE | I2C_SR_BED);

	sys_write8(addr, cfg->base + I2C_DBR);
	sys_write32(tcr, cfg->base + I2C_TCR);

	while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos)) {
	}

	sys_set_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);

	if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_NACK_Pos)) {
		ret = -EIO;
	}

	return ret;
}

static int i2c_sf32lb_dma_tx_config(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	int err;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	struct dma_status dma_status = {0};

	err = sf32lb_dma_get_status_dt(&config->dma_tx, &dma_status);
	if (err) {
		LOG_ERR("Unable to get Rx status (%d)", err);
		return err;
	}

	if (dma_status.busy) {
		LOG_ERR("Tx DMA Channel is busy");
		return -EBUSY;
	}

	err = sf32lb_dma_reload_dt(&config->dma_tx, (uint32_t)msg->buf,
				   (uint32_t)(config->base + I2C_FIFO), msg->len);
	if (err) {
		LOG_ERR("Error configuring Rx DMA (%d)", err);
	}

	return err;
}

static int i2c_sf32lb_dma_rx_config(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_sf32lb_config *config = dev->config;
	int err;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	struct dma_status dma_status = {0};

	err = sf32lb_dma_get_status_dt(&config->dma_rx, &dma_status);
	if (err) {
		LOG_ERR("Unable to get Rx status (%d)", err);
		return err;
	}

	if (dma_status.busy) {
		LOG_ERR("Rx DMA Channel is busy");
		return -EBUSY;
	}

	err = sf32lb_dma_reload_dt(&config->dma_rx, (uint32_t)(config->base + I2C_FIFO),
				   (uint32_t)msg->buf, msg->len);
	if (err) {
		LOG_ERR("Error configuring Rx DMA (%d)", err);
	}

	return err;
}

static int i2c_sf32lb_master_send(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;
	uint32_t sr = 0;

	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	for (int j = 0; j < msg->len; j++) {
		bool last = (msg->len - j == 1) ? true : false;

		if (last) {
			tcr |= I2C_TCR_STOP;
		}

		sys_write8(msg->buf[j], cfg->base + I2C_DBR);
		tcr |= I2C_TCR_TB;
		sys_write32(tcr, cfg->base + I2C_TCR);

		while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos)) {
		}

		sys_set_bit(cfg->base + I2C_SR, I2C_SR_TE_Pos);

		if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_NACK_Pos)) {
			ret = -EIO;
			break;
		}

		tcr = 0;
	}

	while (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
	}

	return ret;
}

static int i2c_sf32lb_master_send_dma(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;

	if (msg->len > 512U) {
		return -ENOTSUP;
	}

	/* Send address first */
	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	if (i2c_is_stop_op(msg)) {
		sys_set_bit(config->base + I2C_CR, I2C_CR_LASTSTOP_Pos);
	}
	sys_write8(config->base + I2C_DNR, msg->len);
	sys_set_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);

	ret = i2c_sf32lb_dma_tx_config(dev, msg);
	if (ret < 0) {
		return ret;
	}

	ret = sf32lb_dma_start_dt(&config->dma_tx);
	if (ret < 0) {
		return ret;
	}

	k_sem_take(&data->i2c_compl, K_MSEC(I2C_TIMEOUT_MS));

	return ret;
}

static int i2c_sf32lb_master_recv(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret = 0;
	const struct i2c_sf32lb_config *cfg = dev->config;
	uint32_t tcr = 0;

	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	for (int j = 0; j < msg->len; j++) {
		bool last = (j == (msg->len - 1)) ? true : false;

		if (last && i2c_is_stop_op(msg)) {
			tcr |= (I2C_TCR_STOP | I2C_TCR_NACK);
		}

		tcr |= I2C_TCR_TB;
		sys_write32(tcr, cfg->base + I2C_TCR);

		while (!sys_test_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos)) {
		}

		sys_set_bit(cfg->base + I2C_SR, I2C_SR_RF_Pos);

		msg->buf[j] = sys_read8(cfg->base + I2C_DBR);
		tcr = 0;
	}

	while (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
	}

	return ret;
}

static int i2c_sf32lb_master_recv_dma(const struct device *dev, uint16_t addr, struct i2c_msg *msg)
{
	int ret;
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;

	if (msg->len > 512U) {
		return -ENOTSUP;
	}

	/* Send address first */
	ret = i2c_sf32lb_send_addr(dev, addr, msg);
	if (ret < 0) {
		return ret;
	}

	if (i2c_is_stop_op(msg)) {
		sys_set_bit(config->base + I2C_CR, I2C_CR_LASTNACK_Pos);
	}
	sys_write8(config->base + I2C_DNR, msg->len);
	sys_set_bit(config->base + I2C_CR, I2C_CR_DMAEN_Pos);

	ret = i2c_sf32lb_dma_rx_config(dev, msg);
	if (ret < 0) {
		return ret;
	}

	ret = sf32lb_dma_start_dt(&config->dma_rx);
	if (ret < 0) {
		return ret;
	}

	k_sem_take(&data->i2c_compl, K_MSEC(I2C_TIMEOUT_MS));

	if (sys_read8(config->base + I2C_DNR) != msg->len) {
		ret = -EIO;
	}

	return ret;
}

static int i2c_sf32lb_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	uint32_t cr = sys_read32(cfg->base + I2C_CR);
	uint32_t dnf = (cr >> I2C_CR_DNF_Pos) & I2C_CR_DNF_Msk;
	uint32_t lv;
	uint32_t slv;
	uint32_t flv;

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		cr |= I2C_MODE_STD;
		lv = (48000000U / cfg->bitrate - dnf - 7 + 1) / 2;
		slv = (lv << I2C_LCR_SLV_Pos) & I2C_LCR_SLV_Msk;
		sys_write32(slv, cfg->base + I2C_LCR);
		break;

	case I2C_SPEED_FAST:
		cr |= I2C_MODE_FS;
		lv = (48000000U / cfg->bitrate - dnf - 7 + 1) / 2;
		flv = (lv << I2C_LCR_FLV_Pos) & I2C_LCR_FLV_Msk;
		sys_write32(flv, cfg->base + I2C_LCR);
		break;

	case I2C_SPEED_FAST_PLUS:
		cr |= I2C_MODE_HS_STD;
		break;

	case I2C_SPEED_HIGH:
		cr |= I2C_MODE_HS_FS;
		break;

	case I2C_SPEED_ULTRA:
		return -ENOTSUP;

	default:
		LOG_ERR("Unsupported I2C speed requested:%d", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	sys_write32(cr, cfg->base + I2C_CR);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int i2c_sf32lb_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	const struct i2c_sf32lb_config *cfg = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	if (sys_test_bit(cfg->base + I2C_SR, I2C_SR_UB_Pos)) {
		return -EBUSY;
	};

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			ret = -ENOTSUP;
			break;
		}

		if (msgs[i].flags & I2C_MSG_READ) {
#ifdef CONFIG_I2C_SF32LB_DMA
			ret = i2c_sf32lb_master_recv_dma(dev, addr, &msgs[i]);
#else
			ret = i2c_sf32lb_master_recv(dev, addr, &msgs[i]);
#endif
			if (ret < 0) {
				break;
			}
		} else {
#ifdef CONFIG_I2C_SF32LB_DMA
			ret = i2c_sf32lb_master_send_dma(dev, addr, &msgs[i]);
#else
			ret = i2c_sf32lb_master_send(dev, addr, &msgs[i]);
#endif

			if (ret < 0) {
				break;
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(i2c, i2c_sf32lb_driver_api) = {
	.configure = i2c_sf32lb_configure,
	.transfer = i2c_sf32lb_transfer,
};

static int i2c_sf32lb_init(const struct device *dev)
{
	const struct i2c_sf32lb_config *config = dev->config;
	struct i2c_sf32lb_data *data = dev->data;
	int ret;
#ifdef CONFIG_I2C_SF32LB_DMA
	struct dma_config tx_dma_cfg = {0};
	struct dma_config rx_dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
#endif

	ret = k_mutex_init(&data->lock);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	if (config->clock.dev != NULL) {
		if (!sf3232lb_clock_is_ready_dt(&config->clock)) {
			return -ENODEV;
		}

		ret = sf32lb_clock_control_on_dt(&config->clock);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_sf32lb_configure(dev, I2C_MODE_CONTROLLER | (config->bitrate));
#ifdef CONFIG_I2C_SF32LB_DMA
	if (!sf32lb_dma_is_ready_dt(&config->dma_tx)) {
		return -ENODEV;
	}

	if (!sf32lb_dma_is_ready_dt(&config->dma_rx)) {
		return -ENODEV;
	}

	sf32lb_dma_config_init_dt(&config->dma_tx, &tx_dma_cfg);
	sf32lb_dma_config_init_dt(&config->dma_rx, &rx_dma_cfg);

	tx_dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	tx_dma_cfg.block_count = 1U;
	tx_dma_cfg.source_data_size = 1U;
	tx_dma_cfg.dest_data_size = 1U;

	tx_dma_cfg.head_block = &dma_blk;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	sf32lb_dma_config_dt(&config->dma_tx, &tx_dma_cfg);

	rx_dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	rx_dma_cfg.block_count = 1U;
	rx_dma_cfg.source_data_size = 1U;
	rx_dma_cfg.dest_data_size = 1U;

	rx_dma_cfg.head_block = &dma_blk;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	sf32lb_dma_config_dt(&config->dma_rx, &rx_dma_cfg);

	sys_set_bit(config->base + I2C_IER, I2C_IER_DMADONEIE_Pos);
#endif

	sys_set_bits(config->base + I2C_CR, I2C_CR_IUE | I2C_CR_SCLE);

#ifdef CONFIG_I2C_SF32LB_INTERRUPT
	if (config->irq_cfg_func) {
		config->irq_cfg_func();
	}
#endif
	return ret;
}

#define I2C_SF32LB_INIT(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                               \
	IF_ENABLED(CONFIG_I2C_SF32LB_DMA,                                                        \
	(static void i2c_sf32lb_irq_config_func_##n(void)                                        \
	{                                                                                        \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_sf32lb_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                           \
		irq_enable(DT_INST_IRQN(n));                                                     \
	}));                                                                                     \
	static struct i2c_sf32lb_data i2c_sf32lb_data_##n;                                       \
	static const struct i2c_sf32lb_config i2c_sf32lb_config_##n = {                          \
		.base = DT_INST_REG_ADDR(n),                                                     \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                     \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET_OR(n, {}),                                \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                     \
		IF_ENABLED(CONFIG_I2C_SF32LB_DMA, (                                              \
			.dma_tx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, tx),                    \
			.dma_rx = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(n, rx),                    \
		))                                                                               \
		IF_ENABLED(CONFIG_I2C_SF32LB_INTERRUPT, (                                        \
			.irq_cfg_func = i2c_sf32lb_irq_config_func_##n,                          \
		)) };                                                                            \
	DEVICE_DT_INST_DEFINE(n, i2c_sf32lb_init, NULL, &i2c_sf32lb_data_##n,                    \
			      &i2c_sf32lb_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,     \
			      &i2c_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SF32LB_INIT)
