/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Microchip MEC5 ns16550 compatible UART Serial Driver
 */

#define DT_DRV_COMPAT microchip_mec5_uart

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(uart_mec5, CONFIG_UART_LOG_LEVEL);

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_ecia_api.h>
#include <mec_uart_api.h>

#define UART_MEC_DFLT_CLK_FREQ                1843200u
#define UART_MEC_DEVCFG_FLAG_RX_FIFO_TRIG_POS 0
#define UART_MEC_DEVCFG_FLAG_RX_FIFO_TRIG_MSK 0x3u
#define UART_MEC_DEVCFG_FLAG_FIFO_DIS_POS     4
#define UART_MEC_DEVCFG_FLAG_USE_EXTCLK_POS   5

struct uart_mec5_devcfg {
	struct mec_uart_regs *regs;
	const struct pinctrl_dev_config *pcfg;
	uint32_t clock_freq;
	uint32_t flags;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_mec5_dev_data {
	const struct device *dev;
	struct uart_config current_config;
	struct uart_config ucfg;
	struct k_spinlock lock;
	enum mec_uart_ipend ipend;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /* Callback function pointer */
	void *cb_data;                    /* Callback function arg */
	uint32_t flags;
	uint8_t rx_enabled;
	uint8_t tx_enabled;
#endif
};

static const uint8_t mec5_xlat_word_len[4] = {
	MEC_UART_WORD_LEN_5,
	MEC_UART_WORD_LEN_6,
	MEC_UART_WORD_LEN_7,
	MEC_UART_WORD_LEN_8,
};

static const uint8_t mec5_xlat_stop_bits[4] = {
	MEC_UART_STOP_BITS_1,
	MEC_UART_STOP_BITS_1,
	MEC_UART_STOP_BITS_2,
	MEC_UART_STOP_BITS_2,
};

static const uint8_t mec5_xlat_parity[5] = {
	(uint8_t)(MEC5_UART_CFG_PARITY_NONE >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_ODD >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_EVEN >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_MARK >> MEC5_UART_CFG_PARITY_POS),
	(uint8_t)(MEC5_UART_CFG_PARITY_SPACE >> MEC5_UART_CFG_PARITY_POS),
};

static int uart_mec5_xlat_cfg(const struct uart_config *cfg, uint32_t *cfg_word)
{
	uint32_t temp;

	if (!cfg || !cfg_word) {
		return -EINVAL;
	}

	*cfg_word = 0u;

	if (cfg->data_bits > UART_CFG_DATA_BITS_8) {
		return -EINVAL;
	}
	temp = mec5_xlat_word_len[cfg->data_bits];
	*cfg_word |= ((temp << MEC5_UART_CFG_WORD_LEN_POS) & MEC5_UART_CFG_WORD_LEN_MSK);

	if (cfg->stop_bits > UART_CFG_STOP_BITS_2) {
		return -EINVAL;
	}
	temp = mec5_xlat_stop_bits[cfg->stop_bits];
	*cfg_word |= ((temp << MEC5_UART_CFG_STOP_BITS_POS) & MEC5_UART_CFG_STOP_BITS_MSK);

	if (cfg->parity > UART_CFG_PARITY_SPACE) {
		return -EINVAL;
	}
	temp = mec5_xlat_parity[cfg->parity];
	*cfg_word |= ((temp << MEC5_UART_CFG_PARITY_POS) & MEC5_UART_CFG_PARITY_MSK);

	return 0;
}

/* Configure UART TX and RX FIFOs based on device tree.
 * Both FIFOs are fixed 16-byte.
 * RX FIFO has a configurable interrupt trigger level of 1, 4, 8, or 14 bytes.
 */
static uint32_t uart_mec5_fifo_config(uint32_t mcfg, uint32_t cfg_flags)
{
	uint32_t new_mcfg = mcfg;
	uint32_t temp = 0;

	if (!(cfg_flags & BIT(UART_MEC_DEVCFG_FLAG_FIFO_DIS_POS))) {
		new_mcfg |= BIT(MEC5_UART_CFG_FIFO_EN_POS);
		temp = (cfg_flags & UART_MEC_DEVCFG_FLAG_RX_FIFO_TRIG_MSK) >>
		       UART_MEC_DEVCFG_FLAG_RX_FIFO_TRIG_POS;
		switch (temp) {
		case 0:
			new_mcfg |= MEC5_UART_CFG_RX_FIFO_TRIG_LVL_1;
			break;
		case 1:
			new_mcfg |= MEC5_UART_CFG_RX_FIFO_TRIG_LVL_4;
			break;
		case 2:
			new_mcfg |= MEC5_UART_CFG_RX_FIFO_TRIG_LVL_8;
			break;
		default:
			new_mcfg |= MEC5_UART_CFG_RX_FIFO_TRIG_LVL_14;
			break;
		}
	}

	return new_mcfg;
}

static int config_mec5_uart(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_mec5_devcfg *devcfg = dev->config;
	struct mec_uart_regs *const regs = devcfg->regs;
	struct uart_mec5_dev_data *const data = dev->data;
	int ret = 0;
	uint32_t mcfg = 0, extclk = 0;

	data->ipend = MEC_UART_IPEND_NONE;

	ret = uart_mec5_xlat_cfg(cfg, &mcfg);
	if (ret) {
		return ret;
	}

	mcfg = uart_mec5_fifo_config(mcfg, devcfg->flags);
	mcfg |= BIT(MEC5_UART_CFG_GIRQ_EN_POS);

	if (devcfg->flags & BIT(UART_MEC_DEVCFG_FLAG_USE_EXTCLK_POS)) {
		extclk = devcfg->clock_freq;
	}

	ret = mec_hal_uart_init(regs, cfg->baudrate, mcfg, extclk);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	memcpy(&data->ucfg, cfg, sizeof(struct uart_config));

	return ret;
};

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/* run-time driver configuration API */
static int uart_mec5_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_mec5_devcfg *devcfg = dev->config;
	struct uart_mec5_dev_data *const data = dev->data;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("MEC5 UART pinctrl error (%d)", ret);
	}

	ret = config_mec5_uart(dev, cfg);
	if (ret) {
		LOG_ERR("MEC5 UART config error (%d)", ret);
		return ret;
	}

	data->current_config = *cfg;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int uart_mec5_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_mec5_dev_data *const data = dev->data;

	if (!cfg) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	*cfg = data->current_config;

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

/* Called by kernel during driver initialization phase */
static int uart_mec5_init(const struct device *dev)
{
	const struct uart_mec5_devcfg *devcfg = dev->config;
	struct uart_mec5_dev_data *data = dev->data;
	int ret = 0;

	ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("MEC5 UART init pinctrl error (%d)", ret);
		return ret;
	}

	ret = config_mec5_uart(dev, &data->ucfg);
	if (ret != 0) {
		return -EIO;
	}

	return ret;
}

/*
 * Poll the UART for input.
 * return 0 is a byte arrived else -1 if no data.
 */
static int uart_mec5_poll_in(const struct device *dev, unsigned char *cptr)
{
	const struct uart_mec5_devcfg *devcfg = dev->config;
	struct mec_uart_regs *const regs = devcfg->regs;
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	ret = mec_hal_uart_rx_byte(regs, (uint8_t *)cptr);
	if (ret == MEC_RET_ERR_NO_DATA) {
		k_spin_unlock(&data->lock, key);
		return -1;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

/* Block until UART can accept data byte. */
static void uart_mec5_poll_out(const struct device *dev, unsigned char out_data)
{
	const struct uart_mec5_devcfg *devcfg = dev->config;
	struct mec_uart_regs *const regs = devcfg->regs;
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	do {
		ret = mec_hal_uart_tx_byte(regs, (uint8_t)out_data);
	} while (ret == MEC_RET_ERR_BUSY);

	k_spin_unlock(&data->lock, key);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static inline void irq_tx_enable(const struct device *dev)
{
	const struct uart_mec5_devcfg *devcfg = dev->config;
	struct mec_uart_regs *const regs = devcfg->regs;
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	mec_hal_uart_intr_mask(regs, MEC_UART_IEN_FLAG_ETHREI, MEC_UART_IEN_FLAG_ETHREI);

	k_spin_unlock(&data->lock, key);
}

static inline void irq_tx_disable(const struct device *dev)
{
	const struct uart_mec5_devcfg *devcfg = dev->config;
	struct uart_mec5_dev_data *data = dev->data;
	struct mec_uart_regs *const regs = devcfg->regs;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	mec_hal_uart_intr_mask(regs, MEC_UART_IEN_FLAG_ETHREI, 0);

	k_spin_unlock(&data->lock, key);
}

static inline void irq_rx_enable(const struct device *dev)
{
	const struct uart_mec5_devcfg *const devcfg = dev->config;
	struct uart_mec5_dev_data *data = dev->data;
	struct mec_uart_regs *const regs = devcfg->regs;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	mec_hal_uart_intr_mask(regs, MEC_UART_IEN_FLAG_ERDAI, MEC_UART_IEN_FLAG_ERDAI);

	k_spin_unlock(&data->lock, key);
}

static inline void irq_rx_disable(const struct device *dev)
{
	const struct uart_mec5_devcfg *const devcfg = dev->config;
	struct uart_mec5_dev_data *data = dev->data;
	struct mec_uart_regs *const regs = devcfg->regs;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	mec_hal_uart_intr_mask(regs, MEC_UART_IEN_FLAG_ERDAI, 0);

	k_spin_unlock(&data->lock, key);
}

static int uart_mec5_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_mec5_devcfg *const devcfg = dev->config;
	struct uart_mec5_dev_data *data = dev->data;
	struct mec_uart_regs *const regs = devcfg->regs;
	int num_tx = 0, ret = 0;

	if (len < 0) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while (num_tx < len) {
		ret = mec_hal_uart_tx_byte(regs, tx_data[num_tx]);
		if (ret == MEC_RET_ERR_BUSY) {
			break;
		}
		num_tx++;
	}

	if (data->tx_enabled) {
		irq_tx_enable(dev);
	}

	k_spin_unlock(&data->lock, key);

	return num_tx;
}

static int uart_mec5_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_mec5_devcfg *const devcfg = dev->config;
	struct uart_mec5_dev_data *data = dev->data;
	struct mec_uart_regs *const regs = devcfg->regs;
	int num_rx = 0, ret = 0;

	if (size < 0) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint8_t *pdata = rx_data;

	while (num_rx < size) {
		ret = mec_hal_uart_rx_byte(regs, pdata);
		if (ret != MEC_RET_OK) {
			break;
		}
		pdata++;
		num_rx++;
	}

	if (data->rx_enabled) {
		irq_rx_enable(dev);
	}

	k_spin_unlock(&data->lock, key);

	return num_rx;
}

static void uart_mec5_irq_tx_enable(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->tx_enabled = 1;
	irq_tx_enable(dev);

	k_spin_unlock(&data->lock, key);
}

static void uart_mec5_irq_tx_disable(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_tx_disable(dev);
	data->tx_enabled = 0;

	k_spin_unlock(&data->lock, key);
}

static int uart_mec5_irq_tx_ready(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	if (data->ipend == MEC_UART_IPEND_TX) {
		ret = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void uart_mec5_irq_rx_enable(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->rx_enabled = 1;
	irq_rx_enable(dev);

	k_spin_unlock(&data->lock, key);
}

static void uart_mec5_irq_rx_disable(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_rx_disable(dev);
	data->rx_enabled = 0;

	k_spin_unlock(&data->lock, key);
}

/* check if UART TX shift register is empty. Empty TX shift register indicates
 * the UART does not need clocks and can be put into a low power state.
 * return 1 nothing remains to be transmitted, 0 otherwise.
 */
static int uart_mec5_irq_tx_complete(const struct device *dev)
{
	const struct uart_mec5_devcfg *const devcfg = dev->config;
	struct mec_uart_regs *const regs = devcfg->regs;
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = mec_hal_uart_is_tx_empty(regs);

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int uart_mec5_irq_rx_ready(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->ipend == MEC_UART_IPEND_RX_DATA) {
		ret = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void irq_error_enable(const struct device *dev, uint8_t enable)
{
	const struct uart_mec5_devcfg *const devcfg = dev->config;
	struct mec_uart_regs *const regs = devcfg->regs;
	uint8_t msk = 0;

	if (enable) {
		msk = MEC_UART_IEN_FLAG_ELSI;
	}

	mec_hal_uart_intr_mask(regs, MEC_UART_IEN_FLAG_ELSI, msk);
}

/*
 * Enable received line status interrupt active when one or more of the following errors
 * occur: overrun, parity, framing, or break.
 */
static void uart_mec5_irq_err_enable(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_error_enable(dev, 1u);

	k_spin_unlock(&data->lock, key);
}

static void uart_mec5_irq_err_disable(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_error_enable(dev, 0);

	k_spin_unlock(&data->lock, key);
}

static int uart_mec5_irq_is_pending(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	if (data->ipend != MEC_UART_IPEND_NONE) {
		ret = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int uart_mec5_irq_update(const struct device *dev)
{
	const struct uart_mec5_devcfg *const devcfg = dev->config;
	struct mec_uart_regs *const regs = devcfg->regs;
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->ipend = MEC_UART_IPEND_NONE;
	mec_hal_uart_pending_status(regs, &data->ipend);

	switch (data->ipend) {
	case MEC_UART_IPEND_NONE:
		break;
	case MEC_UART_IPEND_TX:
		irq_tx_disable(dev);
		break;
	case MEC_UART_IPEND_RX_DATA:
		irq_rx_disable(dev);
		break;
	case MEC_UART_IPEND_RX_ERR:
		irq_error_enable(dev, 0);
		break;
	case MEC_UART_IPEND_MODEM:
		__fallthrough; /* fall through */
	default:
		k_panic();
		break;
	}

	k_spin_unlock(&data->lock, key);

	return 1;
}

static void uart_mec5_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_mec5_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->cb = cb;
	data->cb_data = cb_data;

	k_spin_unlock(&data->lock, key);
}

static void uart_mec5_isr(const struct device *dev)
{
	struct uart_mec5_dev_data *data = dev->data;

	if (data->cb) {
		data->cb(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_mec5_driver_api) = {
	.poll_in = uart_mec5_poll_in,
	.poll_out = uart_mec5_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_mec5_configure,
	.config_get = uart_mec5_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mec5_fifo_fill,
	.fifo_read = uart_mec5_fifo_read,
	.irq_tx_enable = uart_mec5_irq_tx_enable,
	.irq_tx_disable = uart_mec5_irq_tx_disable,
	.irq_tx_ready = uart_mec5_irq_tx_ready,
	.irq_rx_enable = uart_mec5_irq_rx_enable,
	.irq_rx_disable = uart_mec5_irq_rx_disable,
	.irq_tx_complete = uart_mec5_irq_tx_complete,
	.irq_rx_ready = uart_mec5_irq_rx_ready,
	.irq_err_enable = uart_mec5_irq_err_enable,
	.irq_err_disable = uart_mec5_irq_err_disable,
	.irq_is_pending = uart_mec5_irq_is_pending,
	.irq_update = uart_mec5_irq_update,
	.irq_callback_set = uart_mec5_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_MEC5_CONFIGURE(n)                                                                     \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_mec5_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	} while (0)
#else
#define UART_MEC5_CONFIGURE(n)
#endif

#define UART_MEC5_DCFG_FLAGS(i)                                                                    \
	((DT_INST_ENUM_IDX_OR(i, rx_fifo_trig, 2) & 0x3u) |                                        \
	 ((DT_INST_PROP_OR(i, fifo_mode_disable, 0) & 0x1u) << 4) |                                \
	 ((DT_INST_PROP_OR(i, use_extclk, 0) & 0x1u) << 5))

#define DEV_DATA_FLOW_CTRL(n) DT_INST_PROP_OR(n, hw_flow_control, UART_CFG_FLOW_CTRL_NONE)

#define UART_MEC5_DEVICE(i)                                                                        \
	PINCTRL_DT_INST_DEFINE(i);                                                                 \
	static const struct uart_mec5_devcfg uart_mec5_##i##_devcfg = {                            \
		.regs = (struct mec_uart_regs *)DT_INST_REG_ADDR(i),                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),                                         \
		.clock_freq = DT_INST_PROP_OR(i, clock_frequency, UART_MEC_DFLT_CLK_FREQ),         \
		.flags = UART_MEC5_DCFG_FLAGS(i),                                                  \
	};                                                                                         \
	static struct uart_mec5_dev_data uart_mec5_##i##_dev_data = {                              \
		.ucfg.baudrate = DT_INST_PROP_OR(i, current_speed, 0),                             \
		.ucfg.parity = UART_CFG_PARITY_NONE,                                               \
		.ucfg.stop_bits = UART_CFG_STOP_BITS_1,                                            \
		.ucfg.data_bits = UART_CFG_DATA_BITS_8,                                            \
		.ucfg.flow_ctrl = DEV_DATA_FLOW_CTRL(i),                                           \
	};                                                                                         \
	static int uart_mec5_##i##_init(const struct device *dev)                                  \
	{                                                                                          \
		UART_MEC5_CONFIGURE(i);                                                            \
		return uart_mec5_init(dev);                                                        \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(i, uart_mec5_##i##_init, NULL, &uart_mec5_##i##_dev_data,            \
			      &uart_mec5_##i##_devcfg, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &uart_mec5_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_MEC5_DEVICE)
