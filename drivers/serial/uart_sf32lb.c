/*
 * Copyright (c) Core Devices LLC
 * Copyright (c) Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_usart

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma/sf32lb.h>
#include <zephyr/drivers/dma.h>
#endif

#include <register.h>

LOG_MODULE_REGISTER(sf32lb_uart, CONFIG_UART_LOG_LEVEL);

#ifdef CONFIG_UART_ASYNC_API
struct sf32lb_uart_async_tx {
	const uint8_t *buf;
	size_t len;
	struct dma_block_config dma_blk;
	int32_t timeout;
	struct k_work_delayable timeout_work;
};

struct sf32lb_uart_async_rx {
	uint8_t *buf;
	size_t len;
	uint8_t *next_buf;
	size_t next_len;
	size_t offset;
	size_t counter;
	int32_t timeout;
	struct k_work_delayable timeout_work;
};

struct sf32lb_uart_async_data {
	const struct device *uart_dev;
	struct sf32lb_uart_async_tx tx;
	struct sf32lb_uart_async_rx rx;
	uart_callback_t cb;
	void *user_data;
};
#endif /* CONFIG_UART_ASYNC_API */

#define UART_CR1   offsetof(USART_TypeDef, CR1)
#define UART_CR2   offsetof(USART_TypeDef, CR2)
#define UART_CR3   offsetof(USART_TypeDef, CR3)
#define UART_BRR   offsetof(USART_TypeDef, BRR)
#define UART_RQR   offsetof(USART_TypeDef, RQR)
#define UART_ISR   offsetof(USART_TypeDef, ISR)
#define UART_ICR   offsetof(USART_TypeDef, ICR)
#define UART_RDR   offsetof(USART_TypeDef, RDR)
#define UART_TDR   offsetof(USART_TypeDef, TDR)
#define UART_MISCR offsetof(USART_TypeDef, MISCR)

#define UART_CR1_M_6B FIELD_PREP(USART_CR1_M_Msk, 0U)
#define UART_CR1_M_7B FIELD_PREP(USART_CR1_M_Msk, 1U)
#define UART_CR1_M_8B FIELD_PREP(USART_CR1_M_Msk, 2U)
#define UART_CR1_M_9B FIELD_PREP(USART_CR1_M_Msk, 3U)

#define UART_CR2_STOP_1B FIELD_PREP(USART_CR2_STOP_Msk, 0U)
#define UART_CR2_STOP_2B FIELD_PREP(USART_CR2_STOP_Msk, 1U)

/* minimal BRR: INT=1, FRAC=0 (0x10) */
#define UART_BRR_MIN 0x10U

struct uart_sf32lb_data {
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config uart_config;
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_callback;
	void *cb_data;
#endif
#ifdef CONFIG_UART_ASYNC_API
	struct sf32lb_uart_async_data async;
#endif
};

struct uart_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	struct sf32lb_clock_dt_spec clock;
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
#ifdef CONFIG_UART_ASYNC_API
	struct sf32lb_dma_dt_spec tx_dma;
	struct sf32lb_dma_dt_spec rx_dma;
#endif /* CONFIG_UART_ASYNC_API */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sf32lb_isr(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
	struct uart_sf32lb_data *data = dev->data;

	if (data->irq_callback) {
		data->irq_callback(dev, data->cb_data);
	}

	sys_write32(USART_ISR_TXE | USART_ICR_TCCF | USART_ISR_RXNE, config->base + UART_ICR);
}
#endif

static int uart_sf32lb_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_sf32lb_config *config = dev->config;
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_sf32lb_data *data = dev->data;
#endif
	enum uart_config_data_bits data_bits = cfg->data_bits;
	uint32_t cr1, cr2, cr3, brr, miscr;

	/* CR1: disable USART */
	cr1 = sys_read32(config->base + UART_CR1);
	cr1 &= ~USART_CR1_UE;
	sys_write32(cr1, config->base + UART_CR1);

	/* CR1: data bits, parity, oversampling */
	cr1 &= ~(USART_CR1_M_Msk | USART_CR1_PCE_Msk | USART_CR1_PS_Msk | USART_CR1_OVER8_Msk);

	/* data bits include parity bit */
	if (cfg->parity != UART_CFG_PARITY_NONE) {
		data_bits++;
		if (data_bits > UART_CFG_DATA_BITS_9) {
			return -ENOTSUP;
		}
	}

	switch (data_bits) {
	case UART_CFG_DATA_BITS_6:
		cr1 |= UART_CR1_M_6B;
		break;
	case UART_CFG_DATA_BITS_7:
		cr1 |= UART_CR1_M_7B;
		break;
	case UART_CFG_DATA_BITS_8:
		cr1 |= UART_CR1_M_8B;
		break;
	case UART_CFG_DATA_BITS_9:
		cr1 |= UART_CR1_M_9B;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		cr1 |= (USART_CR1_PCE | USART_CR1_PS);
		break;
	case UART_CFG_PARITY_EVEN:
		cr1 |= USART_CR1_PCE;
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(cr1, config->base + UART_CR1);

	/* CR2: stop bits */
	cr2 = sys_read32(config->base + UART_CR2);
	cr2 &= ~USART_CR2_STOP_Msk;

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		cr2 |= UART_CR2_STOP_1B;
		break;
	case UART_CFG_STOP_BITS_2:
		cr2 |= UART_CR2_STOP_2B;
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(cr2, config->base + UART_CR2);

	/* CR3: flow control */
	cr3 = sys_read32(config->base + UART_CR3);
	cr3 &= ~(USART_CR3_RTSE_Msk | USART_CR3_CTSE_Msk);

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		cr3 |= (USART_CR3_RTSE_Msk | USART_CR3_CTSE_Msk);
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(cr3, config->base + UART_CR3);

	/* enable USART */
	cr1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
	sys_write32(cr1, config->base + UART_CR1);

	/* BRR: baudrate */
	miscr = sys_read32(config->base + UART_MISCR);
	miscr &= ~USART_MISCR_SMPLINI_Msk;

	brr = 48000000UL / cfg->baudrate;
	if (brr < UART_BRR_MIN) {
		cr1 |= USART_CR1_OVER8;
		sys_write32(cr1, config->base + UART_CR1);
		/* recalculate brr with reduced oversampling */
		brr = (48000000UL * 2U) / cfg->baudrate;
		miscr |= FIELD_PREP(USART_MISCR_SMPLINI_Msk, 2U);
	} else {
		miscr |= FIELD_PREP(USART_MISCR_SMPLINI_Msk, 6U);
	}

	sys_write32(miscr, config->base + UART_MISCR);
	sys_write32(brr, config->base + UART_BRR);

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	data->uart_config = *cfg;
#endif
	return 0;
}

static int uart_sf32lb_poll_in(const struct device *dev, uint8_t *c)
{
	const struct uart_sf32lb_config *config = dev->config;

	if ((sys_read32(config->base + UART_ISR) & USART_ISR_RXNE) != 0U) {
		*c = sys_read32(config->base + UART_RDR) & 0xFFU;
		return 0;
	}

	return -1;
}

static void uart_sf32lb_poll_out(const struct device *dev, uint8_t c)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_write32(USART_ISR_TC, config->base + UART_ICR);
	sys_write8(c, config->base + UART_TDR);

	while ((sys_read32(config->base + UART_ISR) & USART_ISR_TC) == 0U) {
	}
}

static int uart_sf32lb_err_check(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
	uint32_t isr = sys_read32(config->base + UART_ISR);
	int err = 0;

	if (isr & USART_ISR_ORE) {
		err |= UART_ERROR_OVERRUN;
	}

	if (isr & USART_ISR_PE) {
		err |= UART_ERROR_PARITY;
	}

	if (isr & USART_ISR_FE) {
		err |= UART_ERROR_FRAMING;
	}

	if (isr & USART_ISR_NF) {
		err |= UART_ERROR_NOISE;
	}

	/* clear error flags */
	sys_write32(USART_ICR_ORECF | USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NCF,
		    config->base + UART_ICR);

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_sf32lb_configure_set(const struct device *dev, const struct uart_config *cfg)
{
	return uart_sf32lb_configure(dev, cfg);
}

static int uart_sf32lb_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_sf32lb_data *data = dev->data;

	*cfg = data->uart_config;

	return 0;
}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_sf32lb_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_sf32lb_config *config = dev->config;
	int i;

	for (i = 0; i < len; i++) {
		if (!sys_test_bit(config->base + UART_ISR, USART_ISR_TXE_Pos)) {
			break;
		}
		sys_write8(tx_data[i], config->base + UART_TDR);
	}

	return i;
}

static int uart_sf32lb_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_sf32lb_config *config = dev->config;
	int i;

	for (i = 0; i < size; i++) {
		if (!sys_test_bit(config->base + UART_ISR, USART_ISR_RXNE_Pos)) {
			break;
		}
		rx_data[i] = sys_read8(config->base + UART_RDR);
	}

	return i;
}

static void uart_sf32lb_irq_tx_enable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + UART_CR1, USART_CR1_TXEIE_Pos);
}

static void uart_sf32lb_irq_tx_disable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + UART_CR1, USART_CR1_TXEIE_Pos);
}

static int uart_sf32lb_irq_tx_ready(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_test_bit(config->base + UART_ISR, USART_ISR_TXE_Pos);
}

static int uart_sf32lb_irq_tx_complete(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_test_bit(config->base + UART_ISR, USART_ISR_TC_Pos);
}

static int uart_sf32lb_irq_rx_ready(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_test_bit(config->base + UART_ISR, USART_ISR_RXNE_Pos);
}

static void uart_sf32lb_irq_err_enable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + UART_CR1, USART_CR1_PEIE_Pos);
	sys_set_bit(config->base + UART_CR3, USART_CR3_EIE_Pos);
}

static void uart_sf32lb_irq_err_disable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + UART_CR1, USART_CR1_PEIE_Pos);
	sys_clear_bit(config->base + UART_CR3, USART_CR3_EIE_Pos);
}

static int uart_sf32lb_irq_is_pending(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_read32(config->base + UART_ISR) == 0U ? 0 : 1;
}

static int uart_sf32lb_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_sf32lb_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct uart_sf32lb_data *data = dev->data;

	data->irq_callback = cb;
	data->cb_data = user_data;
}

static void uart_sf32lb_irq_rx_enable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + UART_CR1, USART_CR1_RXNEIE_Pos);
}

static void uart_sf32lb_irq_rx_disable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + UART_CR1, USART_CR1_RXNEIE_Pos);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static inline void uart_sf32lb_async_timer_start(struct k_work_delayable *work, size_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		LOG_DBG("Async timer started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void uart_sf32lb_dma_tx_done(const struct device *dma_dev, void *user_data, uint32_t channel,
				    int status)
{
	struct uart_sf32lb_data *data = user_data;
	const struct device *uart_dev = data->async.uart_dev;
	const struct uart_sf32lb_config *config = uart_dev->config;
	struct uart_event evt = {0};
	unsigned int key;

	k_work_cancel_delayable(&data->async.tx.timeout_work);
	sf32lb_dma_stop_dt(&config->tx_dma);
	key = irq_lock();

	/* Disable DMA for TX */
	sys_clear_bit(config->base + UART_CR3, USART_CR3_DMAT_Pos);

	evt.type = UART_TX_DONE;
	evt.data.tx.buf = data->async.tx.buf;
	evt.data.tx.len = data->async.tx.len;
	if (data->async.cb) {
		data->async.cb(uart_dev, &evt, data->async.user_data);
	}

	/* Reset TX Buffer */
	data->async.tx.buf = NULL;
	data->async.tx.len = 0U;

	irq_unlock(key);
}

static void uart_sf32lb_dma_rx_done(const struct device *dma_dev, void *user_data, uint32_t channel,
				    int status)
{
	struct uart_sf32lb_data *data = user_data;
	const struct device *uart_dev = data->async.uart_dev;
	const struct uart_sf32lb_config *config = uart_dev->config;
	struct uart_event evt = {0};
	unsigned int key;

	key = irq_lock();

	/* Disable DMA for RX */
	sys_clear_bit(config->base + UART_CR3, USART_CR3_DMAR_Pos);
	sf32lb_dma_stop_dt(&config->rx_dma);

	/* Notify RX_RDY */
	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async.rx.buf;
	evt.data.rx.len = data->async.rx.len;
	evt.data.rx.offset = 0;
	if (data->async.cb && evt.data.rx.len) {
		data->async.cb(uart_dev, &evt, data->async.user_data);
	}

	/* Release current buffer */
	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = data->async.rx.buf;
	if (data->async.cb) {
		data->async.cb(uart_dev, &evt, data->async.user_data);
	}

	/* Load next buffer and request another */
	data->async.rx.buf = data->async.rx.next_buf;
	data->async.rx.len = data->async.rx.next_len;
	data->async.rx.next_buf = NULL;
	data->async.rx.next_len = 0U;

	/* If there's a next buffer, restart DMA */
	if (data->async.rx.buf) {
		sf32lb_dma_reload_dt(&config->rx_dma, (uint32_t)(config->base + UART_RDR),
				     (uint32_t)data->async.rx.buf, data->async.rx.len);

		(void)sf32lb_dma_start_dt(&config->rx_dma);

		sys_set_bit(config->base + UART_CR3, USART_CR3_DMAR_Pos);
		sys_set_bit(config->base + UART_RQR, USART_RQR_RXFRQ_Pos);
	}

	evt.type = UART_RX_BUF_REQUEST;
	if (data->async.cb) {
		data->async.cb(uart_dev, &evt, data->async.user_data);
	}

	/* Notify RX_DISABLED when there is no buffer */
	if (!data->async.rx.buf) {
		evt.type = UART_RX_DISABLED;
		if (data->async.cb) {
			data->async.cb(uart_dev, &evt, data->async.user_data);
		}
	}

	irq_unlock(key);
}

static int uart_async_sf32lb_callback_set(const struct device *dev, uart_callback_t callback,
					  void *user_data)
{
	struct uart_sf32lb_data *data = dev->data;

	data->async.cb = callback;
	data->async.user_data = user_data;

	return 0;
}

static int uart_async_sf32lb_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
				       int32_t timeout)
{
	const struct uart_sf32lb_config *config = dev->config;
	struct uart_sf32lb_data *data = dev->data;
	unsigned int key;
	struct dma_status dma_stat;
	struct uart_event evt = {0};

	sf32lb_dma_get_status_dt(&config->rx_dma, &dma_stat);
	if (dma_stat.busy) {
		return -EBUSY;
	}

	key = irq_lock();

	data->async.rx.buf = buf;
	data->async.rx.len = len;
	data->async.rx.timeout = timeout;

	sf32lb_dma_reload_dt(&config->rx_dma, (uintptr_t)(config->base + UART_RDR),
			     (uint32_t)data->async.rx.buf, data->async.rx.len);

	(void)sf32lb_dma_start_dt(&config->rx_dma);

	sys_set_bit(config->base + UART_CR3, USART_CR3_DMAR_Pos);
	sys_set_bit(config->base + UART_RQR, USART_RQR_RXFRQ_Pos);

	/* Request next buffer */
	evt.type = UART_RX_BUF_REQUEST;
	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}
	uart_sf32lb_async_timer_start(&data->async.rx.timeout_work, timeout);

	irq_unlock(key);

	return 0;
}

static int uart_async_sf32lb_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_sf32lb_data *data = dev->data;
	unsigned int key;
	int ret = 0;

	key = irq_lock();
	if (data->async.rx.next_buf != NULL || data->async.rx.next_len != 0) {
		ret = -EBUSY;
	} else {
		data->async.rx.next_buf = buf;
		data->async.rx.next_len = len;
	}
	irq_unlock(key);

	return ret;
}

static int uart_async_sf32lb_rx_disable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
	struct uart_sf32lb_data *data = dev->data;
	unsigned int key;
	struct dma_status dma_stat;
	int err;
	struct uart_event evt = {0};

	k_work_cancel_delayable(&data->async.rx.timeout_work);

	sf32lb_dma_get_status_dt(&config->rx_dma, &dma_stat);
	if (dma_stat.busy) {
		err = -EBUSY;
		goto unlock;
	}

	key = irq_lock();

	if (data->async.rx.len == 0U) {
		err = -EINVAL;
		goto unlock;
	}

	sys_clear_bit(config->base + UART_CR3, USART_CR3_DMAR_Pos);

	err = sf32lb_dma_stop_dt(&config->rx_dma);
	if (err) {
		LOG_ERR("Error stopping Rx DMA (%d)", err);
		goto unlock;
	}

	/* If any bytes have been received notify RX_RDY */
	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async.rx.buf;
	evt.data.rx.len = data->async.rx.counter - data->async.rx.offset;
	evt.data.rx.offset = data->async.rx.offset;

	if (data->async.cb && evt.data.rx.len) {
		data->async.cb(data->async.uart_dev, &evt, data->async.user_data);
	}

	data->async.rx.offset = 0;
	data->async.rx.counter = 0;

	/* Release current buffer */
	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = data->async.rx.buf;

	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

	data->async.rx.len = 0U;
	data->async.rx.buf = NULL;

	/* Release next buffer */
	if (data->async.rx.next_len) {
		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = data->async.rx.next_buf;
		if (data->async.cb) {
			data->async.cb(dev, &evt, data->async.user_data);
		}

		data->async.rx.next_len = 0U;
		data->async.rx.next_buf = NULL;
	}

	/* Notify UART_RX_DISABLED */
	evt.type = UART_RX_DISABLED;
	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

unlock:
	irq_unlock(key);
	return err;
}

static int uart_async_sf32lb_tx(const struct device *dev, const uint8_t *buf, size_t len,
				int32_t timeout)
{
	const struct uart_sf32lb_config *config = dev->config;
	struct uart_sf32lb_data *data = dev->data;
	unsigned int key;
	struct dma_status dma_stat;

	if (!buf || (len == 0U)) {
		return -EINVAL;
	}

	sf32lb_dma_get_status_dt(&config->tx_dma, &dma_stat);
	if (dma_stat.busy) {
		LOG_WRN("Tx busy");
		return -EBUSY;
	}

	key = irq_lock();

	data->async.tx.buf = buf;
	data->async.tx.len = len;

	sf32lb_dma_reload_dt(&config->tx_dma, (uint32_t)data->async.tx.buf,
			     (uint32_t)(config->base + UART_TDR), data->async.tx.len);

	(void)sf32lb_dma_start_dt(&config->tx_dma);

	uart_sf32lb_async_timer_start(&data->async.tx.timeout_work, timeout);

	/* Clear the TC flag in the ICR register */
	sys_write32(USART_ICR_TCCF, config->base + UART_ICR);

	sys_set_bit(config->base + UART_CR3, USART_CR3_DMAT_Pos);

	irq_unlock(key);

	return 0;
}

static int uart_async_sf32lb_tx_abort(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
	struct uart_sf32lb_data *data = dev->data;
	struct uart_event evt = {0};
	struct dma_status dma_stat;
	int err;
	unsigned int key;

	sf32lb_dma_get_status_dt(&config->tx_dma, &dma_stat);
	if (dma_stat.busy) {
		err = -EBUSY;
		goto unlock;
	}

	k_work_cancel_delayable(&data->async.tx.timeout_work);

	key = irq_lock();

	sys_clear_bit(config->base + UART_CR3, USART_CR3_DMAT_Pos);

	err = sf32lb_dma_stop_dt(&config->tx_dma);
	if (err) {
		LOG_ERR("Error stopping Tx DMA (%d)", err);
		goto unlock;
	}

	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = data->async.tx.buf;
	evt.data.tx.len = data->async.tx.len;

	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

unlock:
	irq_unlock(key);
	return err;
}

static void uart_sf32lb_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sf32lb_uart_async_tx *tx =
		CONTAINER_OF(dwork, struct sf32lb_uart_async_tx, timeout_work);
	struct sf32lb_uart_async_data *async = CONTAINER_OF(tx, struct sf32lb_uart_async_data, tx);
	struct uart_sf32lb_data *data = CONTAINER_OF(async, struct uart_sf32lb_data, async);

	uart_async_sf32lb_tx_abort(data->async.uart_dev);
}

static void uart_sf32lb_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sf32lb_uart_async_rx *rx =
		CONTAINER_OF(dwork, struct sf32lb_uart_async_rx, timeout_work);
	struct sf32lb_uart_async_data *async = CONTAINER_OF(rx, struct sf32lb_uart_async_data, rx);
	struct uart_sf32lb_data *data = CONTAINER_OF(async, struct uart_sf32lb_data, async);
	const struct uart_sf32lb_config *config = data->async.uart_dev->config;
	struct dma_status dma_stat;
	uint32_t total_rx;
	unsigned int key;

	key = irq_lock();

	sf32lb_dma_get_status_dt(&config->rx_dma, &dma_stat);

	k_work_cancel_delayable(&data->async.rx.timeout_work);

	irq_unlock(key);

	total_rx = async->rx.len - dma_stat.pending_length;

	if (total_rx > async->rx.offset) {
		async->rx.counter = total_rx - async->rx.offset;
		struct uart_event rdy_event = {
			.type = UART_RX_RDY,
			.data.rx.buf = async->rx.buf,
			.data.rx.len = async->rx.counter,
			.data.rx.offset = async->rx.offset,
		};
		if (data->async.cb) {
			data->async.cb(async->uart_dev, &rdy_event, data->async.user_data);
		}
	}
	async->rx.offset += async->rx.counter;
	async->rx.counter = 0;
}
#endif /* CONFIG_UART_ASYNC_API */

static DEVICE_API(uart, uart_sf32lb_api) = {
	.poll_in = uart_sf32lb_poll_in,
	.poll_out = uart_sf32lb_poll_out,
	.err_check = uart_sf32lb_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_sf32lb_configure_set,
	.config_get = uart_sf32lb_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sf32lb_fifo_fill,
	.fifo_read = uart_sf32lb_fifo_read,
	.irq_tx_enable = uart_sf32lb_irq_tx_enable,
	.irq_tx_disable = uart_sf32lb_irq_tx_disable,
	.irq_tx_complete = uart_sf32lb_irq_tx_complete,
	.irq_tx_ready = uart_sf32lb_irq_tx_ready,
	.irq_rx_enable = uart_sf32lb_irq_rx_enable,
	.irq_rx_disable = uart_sf32lb_irq_rx_disable,
	.irq_rx_ready = uart_sf32lb_irq_rx_ready,
	.irq_err_enable = uart_sf32lb_irq_err_enable,
	.irq_err_disable = uart_sf32lb_irq_err_disable,
	.irq_is_pending = uart_sf32lb_irq_is_pending,
	.irq_update = uart_sf32lb_irq_update,
	.irq_callback_set = uart_sf32lb_irq_callback_set,
#endif
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_async_sf32lb_callback_set,
	.rx_enable = uart_async_sf32lb_rx_enable,
	.rx_buf_rsp = uart_async_sf32lb_rx_buf_rsp,
	.rx_disable = uart_async_sf32lb_rx_disable,
	.tx = uart_async_sf32lb_tx,
	.tx_abort = uart_async_sf32lb_tx_abort,
#endif
};

static int uart_sf32lb_init(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
#ifdef CONFIG_UART_ASYNC_API
	struct uart_sf32lb_data *data = dev->data;
#endif
#ifdef CONFIG_UART_ASYNC_API
	struct dma_config rx_dma_cfg = {0};
	struct dma_config tx_dma_cfg = {0};
	struct dma_block_config rx_dma_blk = {0};
	struct dma_block_config tx_dma_blk = {0};
#endif
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (config->clock.dev != NULL) {
		if (!sf32lb_clock_is_ready_dt(&config->clock)) {
			return -ENODEV;
		}

		ret = sf32lb_clock_control_on_dt(&config->clock);
		if (ret < 0) {
			return ret;
		}
	}

	ret = uart_sf32lb_configure(dev, &config->uart_cfg);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	data->async.uart_dev = dev;
	k_work_init_delayable(&data->async.tx.timeout_work, uart_sf32lb_async_tx_timeout);
	k_work_init_delayable(&data->async.rx.timeout_work, uart_sf32lb_async_rx_timeout);

	sf32lb_dma_config_init_dt(&config->rx_dma, &rx_dma_cfg);

	rx_dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	rx_dma_cfg.source_data_size = 1U;
	rx_dma_cfg.dest_data_size = 1U;
	rx_dma_cfg.complete_callback_en = 1U;
	rx_dma_cfg.dma_callback = uart_sf32lb_dma_rx_done;
	rx_dma_cfg.user_data = (void *)data;
	rx_dma_cfg.block_count = 1U;

	rx_dma_cfg.head_block = &rx_dma_blk;
	rx_dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	rx_dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	ret = sf32lb_dma_config_dt(&config->rx_dma, &rx_dma_cfg);
	if (ret < 0) {
		LOG_ERR("Error configuring Rx DMA (%d)", ret);
		return ret;
	}

	sf32lb_dma_config_init_dt(&config->tx_dma, &tx_dma_cfg);

	tx_dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	tx_dma_cfg.source_data_size = 1U;
	tx_dma_cfg.dest_data_size = 1U;
	tx_dma_cfg.complete_callback_en = 1U;
	tx_dma_cfg.dma_callback = uart_sf32lb_dma_tx_done;
	tx_dma_cfg.user_data = (void *)data;
	tx_dma_cfg.block_count = 1U;

	tx_dma_cfg.head_block = &tx_dma_blk;
	tx_dma_blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	tx_dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	ret = sf32lb_dma_config_dt(&config->tx_dma, &tx_dma_cfg);
	if (ret) {
		LOG_ERR("Error configuring Tx DMA (%d)", ret);
		return ret;
	}
#endif

	return 0;
}

#define SF32LB_UART_DEFINE(index)                                                                  \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                                   \
	(static void uart_sf32lb_irq_config_func_##index(const struct device *dev)                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), uart_sf32lb_isr,    \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}));                                                                                       \
                                                                                                   \
	static const struct uart_sf32lb_config uart_sf32lb_cfg_##index = {                         \
		.base = DT_INST_REG_ADDR(index),                                                   \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET_OR(index, {}),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.uart_cfg =                                                                        \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity =                                                          \
					DT_INST_ENUM_IDX_OR(index, parity, UART_CFG_PARITY_NONE),  \
				.stop_bits = DT_INST_ENUM_IDX_OR(index, stop_bits,                 \
								 UART_CFG_STOP_BITS_1),            \
				.data_bits = DT_INST_ENUM_IDX_OR(index, data_bits,                 \
								 UART_CFG_DATA_BITS_8),            \
				.flow_ctrl = DT_INST_PROP(index, hw_flow_control)                  \
						     ? UART_CFG_FLOW_CTRL_RTS_CTS                  \
						     : UART_CFG_FLOW_CTRL_NONE,                    \
			},                                                                         \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                           \
			(.irq_config_func = uart_sf32lb_irq_config_func_##index,))                 \
		IF_ENABLED(CONFIG_UART_ASYNC_API,                                                  \
			(.tx_dma = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(index, tx),                 \
			 .rx_dma = SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(index, rx),))               \
	};                                                                                         \
                                                                                                   \
	static struct uart_sf32lb_data uart_sf32lb_data_##index;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, uart_sf32lb_init, NULL, &uart_sf32lb_data_##index,            \
			      &uart_sf32lb_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_sf32lb_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_UART_DEFINE)
