/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_uart_sci

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include "r_sci_rx_if.h"
#include "iodefine_sci.h"

#if CONFIG_SOC_SERIES_RX130
#include "r_sci_rx130_private.h"
#elif CONFIG_SOC_SERIES_RX261
#include "r_sci_rx261_private.h"
#elif CONFIG_SOC_SERIES_RX26T
#include "r_sci_rx26t_private.h"
#else
#error Unknown SOC, not (yet) supported.
#endif

#if defined(CONFIG_UART_ASYNC_API)
#include <zephyr/drivers/misc/renesas_rx_dtc/renesas_rx_dtc.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rx_uart_sci, CONFIG_UART_LOG_LEVEL);

#define DEV_CFG(dev)  ((const struct uart_rx_sci_config *const)(dev)->config)
#define DEV_BASE(dev) (DEV_CFG(dev)->regs)

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static void uart_rx_sci_txi_isr(const struct device *dev);
#endif

/* SCI SCR register */
#define R_SCI_SCR_TIE_Pos  (SCI_BIT7)
#define R_SCI_SCR_RIE_Pos  (SCI_BIT6)
#define R_SCI_SCR_TEIE_Pos (SCI_BIT2)

/* SCI SSR register */
#define R_SCI_SSR_TDRE_Pos    (SCI_BIT7)
#define R_SCI_SSR_RDRF_Pos    (SCI_BIT6)
#define R_SCI_SSR_ORER_Pos    (SCI_BIT5)
#define R_SCI_SSR_FER_Pos     (SCI_BIT4)
#define R_SCI_SSR_PER_Pos     (SCI_BIT3)
#define R_SCI_SSR_TEND_Pos    (SCI_BIT2)
#define r_SCI_SSR_ERRORS_MASK (R_SCI_SSR_ORER_Pos | R_SCI_SSR_FER_Pos | R_SCI_SSR_PER_Pos)

struct uart_rx_sci_config {
	uint32_t regs;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock;
	struct clock_control_rx_subsys_cfg clock_subsys;
};

struct uart_rx_sci_data {
	const struct device *dev;
	uint8_t channel;
	sci_hdl_t hdl;
	struct uart_config uart_config;
	sci_cfg_t sci_config;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uint8_t rxi_irq;
	uint8_t txi_irq;
	uint8_t tei_irq;
	uint8_t eri_irq;
	uart_irq_callback_user_data_t user_cb;
	void *user_cb_data;
#endif
#if defined(CONFIG_UART_ASYNC_API)
	uart_callback_t async_user_cb;
	void *async_user_cb_data;

	struct k_work_delayable rx_timeout_work;
	size_t rx_timeout;
	size_t rx_buf_len;
	size_t rx_buf_offset;
	size_t rx_buf_cap;
	uint8_t *rx_buffer;
	size_t rx_next_buf_cap;
	uint8_t *rx_next_buf;

	struct k_work_delayable tx_timeout;
	size_t tx_buf_cap;
	uint8_t *tx_buffer;
	size_t tx_buf_len;

	const struct device *dtc;
	transfer_info_t rx_transfer_info;
	transfer_info_t tx_transfer_info;

#endif
};

static int uart_rx_sci_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	if (IS_ENABLED(CONFIG_UART_ASYNC_API) && sci->SCR.BIT.RIE) {
		return -EBUSY;
	}

	if (sci->SSR.BIT.RDRF == 0U) {
		/* There are no characters available to read. */
		return -1;
	}

	*c = (unsigned char)sci->RDR;

	return 0;
}

static void uart_rx_sci_poll_out(const struct device *dev, unsigned char c)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	while (sci->SSR.BIT.TEND == 0U) {
	}

	sci->TDR = c;
}

static int uart_rx_err_check(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	const uint32_t status = sci->SSR.BYTE;
	int errors = 0;

	if ((status & BIT(R_SCI_SSR_ORER_Pos)) != 0) {
		errors |= UART_ERROR_OVERRUN;
		sci->SSR.BIT.ORER = 0;
	}
	if ((status & BIT(R_SCI_SSR_PER_Pos)) != 0) {
		errors |= UART_ERROR_PARITY;
		sci->SSR.BIT.PER = 0;
	}
	if ((status & BIT(R_SCI_SSR_FER_Pos)) != 0) {
		errors |= UART_ERROR_FRAMING;
		sci->SSR.BIT.FER = 0;
	}

	return errors;
}

static int uart_rx_sci_apply_config(const struct uart_config *config, sci_cfg_t *uart_config)
{

	switch (config->data_bits) {
	case UART_CFG_DATA_BITS_5:
		return -ENOTSUP;
	case UART_CFG_DATA_BITS_6:
		return -ENOTSUP;
	case UART_CFG_DATA_BITS_7:
		uart_config->async.data_size = SCI_DATA_7BIT;
		break;
	case UART_CFG_DATA_BITS_8:
		uart_config->async.data_size = SCI_DATA_8BIT;
		break;
	case UART_CFG_DATA_BITS_9:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		uart_config->async.parity_en = SCI_PARITY_OFF;
		uart_config->async.parity_type = SCI_EVEN_PARITY;
		break;
	case UART_CFG_PARITY_ODD:
		uart_config->async.parity_en = SCI_PARITY_ON;
		uart_config->async.parity_type = SCI_ODD_PARITY;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_config->async.parity_en = SCI_PARITY_ON;
		uart_config->async.parity_type = SCI_EVEN_PARITY;
		break;
	case UART_CFG_PARITY_MARK:
		return -ENOTSUP;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (config->stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_1:
		uart_config->async.stop_bits = SCI_STOPBITS_1;
		break;
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_2:
		uart_config->async.stop_bits = SCI_STOPBITS_2;
		break;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	uart_config->async.baud_rate = config->baudrate;
	uart_config->async.clk_src = SCI_CLK_INT;
	uart_config->async.int_priority = 4;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_rx_configure(const struct device *dev, const struct uart_config *cfg)
{
	int err;
	sci_err_t sci_err;
	struct uart_rx_sci_data *data = dev->data;

	err = uart_rx_sci_apply_config(cfg, &data->sci_config);
	if (err) {
		return err;
	}

	sci_err = R_SCI_Close(data->hdl);

	if (sci_err) {
		return -EIO;
	}

	sci_err = R_SCI_Open(data->channel, SCI_MODE_ASYNC, &data->sci_config, NULL, &data->hdl);

	if (sci_err) {
		return -EIO;
	}

	memcpy(&data->uart_config, cfg, sizeof(struct uart_config));

	return 0;
}

static int uart_rx_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rx_sci_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(*cfg));

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_rx_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	uint8_t num_tx = 0U;

	if (size > 0 && sci->SSR.BIT.TDRE) {
		/* Send a character (8bit , parity none) */
		sci->TDR = tx_data[num_tx++];
	}

	return num_tx;
}

static int uart_rx_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	uint8_t num_rx = 0U;

	if (size > 0 && sci->SSR.BIT.RDRF) {
		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = sci->RDR;
	}

	return num_rx;
}

static void uart_rx_irq_tx_enable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BYTE |= (BIT(R_SCI_SCR_TIE_Pos) | BIT(R_SCI_SCR_TEIE_Pos));
	irq_enable(data->tei_irq);
#ifdef CONFIG_PM_DEVICE
	pm_device_busy_set(dev);
#endif
}

static void uart_rx_irq_tx_disable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BYTE &= ~(BIT(R_SCI_SCR_TIE_Pos) | BIT(R_SCI_SCR_TEIE_Pos));
	irq_disable(data->tei_irq);
#ifdef CONFIG_PM_DEVICE
	pm_device_busy_clear(dev);
#endif
}

static int uart_rx_irq_tx_ready(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	return (sci->SCR.BIT.TIE == 1U) &&
	       (sci->SSR.BYTE & (BIT(R_SCI_SSR_TDRE_Pos) | BIT(R_SCI_SSR_TEND_Pos)));
}

static int uart_rx_irq_tx_complete(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	return (sci->SCR.BIT.TEIE == 1U) && (sci->SSR.BYTE & BIT(R_SCI_SSR_TEND_Pos));
}

static void uart_rx_irq_rx_enable(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BIT.RIE = 1U;
#ifdef CONFIG_PM_DEVICE
	pm_device_busy_set(dev);
#endif
}

static void uart_rx_irq_rx_disable(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BIT.RIE = 0U;
#ifdef CONFIG_PM_DEVICE
	pm_device_busy_clear(dev);
#endif
}

static int uart_rx_irq_rx_ready(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	return (sci->SCR.BIT.RIE == 1U) && ((sci->SSR.BYTE & BIT(R_SCI_SSR_RDRF_Pos)));
}

static void uart_rx_irq_err_enable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	irq_enable(data->eri_irq);
}

static void uart_rx_irq_err_disable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	irq_disable(data->eri_irq);
}

static int uart_rx_irq_is_pending(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	bool tx_pending = false;
	bool rx_pending = false;

	tx_pending = ((sci->SCR.BYTE & BIT(R_SCI_SCR_TIE_Pos)) &&
		      (sci->SSR.BYTE & (BIT(R_SCI_SSR_TEND_Pos) | BIT(R_SCI_SSR_TDRE_Pos))));
	rx_pending = ((sci->SCR.BYTE & BIT(R_SCI_SCR_RIE_Pos)) &&
		      ((sci->SSR.BYTE & (BIT(R_SCI_SSR_RDRF_Pos) | BIT(R_SCI_SSR_PER_Pos) |
					 BIT(R_SCI_SSR_FER_Pos) | BIT(R_SCI_SSR_ORER_Pos)))));

	return tx_pending || rx_pending;
}

static int uart_rx_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_rx_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct uart_rx_sci_data *data = dev->data;

	data->user_cb = cb;
	data->user_cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static inline void disable_tx(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	/* Transmit interrupts must be disabled to start with. */
	sci->SCR.BYTE &= ~(BIT(R_SCI_SCR_TIE_Pos) | BIT(R_SCI_SCR_TEIE_Pos));

	/* Make sure no transmission is in progress. */
	while ((sci->SSR.BYTE & BIT(R_SCI_SSR_TDRE_Pos)) == 0 ||
	       (sci->SSR.BYTE & BIT(R_SCI_SSR_TEND_Pos)) == 0) {
		/* Do nothing */
	}

	sci->SCR.BIT.TE = 0;
}

static inline void enable_tx(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BIT.TIE = 1;
	sci->SCR.BIT.TE = 1;
	irq_enable(data->tei_irq);
}

static int uart_rx_sci_async_callback_set(const struct device *dev, uart_callback_t cb,
					  void *cb_data)

{
	struct uart_rx_sci_data *data = dev->data;

	data->async_user_cb = cb;
	data->async_user_cb_data = cb_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->user_cb = NULL;
	data->user_cb_data = NULL;
#endif

	return 0;
}

static int configure_tx_transfer(const struct device *dev, const uint8_t *buf, size_t len)
{
	struct uart_rx_sci_data *data = dev->data;
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	int err;
	static uint8_t tx_dummy_buf = SCI_CFG_DUMMY_TX_BYTE;

	transfer_info_t *p_info = &data->tx_transfer_info;

	if (buf == NULL) {
		p_info->p_src = &tx_dummy_buf;
	} else {
		p_info->p_src = buf;
	}

	err = dtc_renesas_rx_stop_transfer(data->dtc, data->txi_irq);
	p_info->transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;

	if (p_info->transfer_settings_word_b.mode == TRANSFER_MODE_NORMAL) {
		p_info->length = (uint16_t)len;
	} else if (p_info->transfer_settings_word_b.mode == TRANSFER_MODE_BLOCK) {
		p_info->num_blocks = (uint16_t)len;
	} else {
		/* Do nothing */
	}
	p_info->p_dest = (void *)&sci->TDR;

	err = dtc_renesas_rx_configuration(data->dtc, data->txi_irq, p_info);

	if (err != 0) {
		return err;
	}

	err = dtc_renesas_rx_start_transfer(data->dtc, data->txi_irq);

	if (err != 0) {
		return err;
	}

	return 0;
}

static int configure_rx_transfer(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_rx_sci_data *data = dev->data;
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	int err;
	static uint8_t rx_dummy_buf;

	transfer_info_t *p_info = &data->rx_transfer_info;

	if (buf == NULL) {
		p_info->p_dest = &rx_dummy_buf;
	} else {
		p_info->p_dest = buf;
	}

	err = dtc_renesas_rx_stop_transfer(data->dtc, data->rxi_irq);

	p_info->transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
	p_info->transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_DESTINATION;
	p_info->transfer_settings_word_b.irq = TRANSFER_IRQ_EACH;
	p_info->transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
	p_info->transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED;
	p_info->transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
	p_info->transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;

	if (p_info->transfer_settings_word_b.mode == TRANSFER_MODE_NORMAL) {
		p_info->length = (uint16_t)len;
	} else if (p_info->transfer_settings_word_b.mode == TRANSFER_MODE_BLOCK) {
		p_info->num_blocks = (uint16_t)len;
	} else {
		/* Do nothing */
	}
	p_info->p_src = (void *)&sci->RDR;

	err = dtc_renesas_rx_configuration(data->dtc, data->rxi_irq, p_info);

	if (err != 0) {
		return err;
	}

	dtc_renesas_rx_start_transfer(data->dtc, data->rxi_irq);

	return 0;
}

static int uart_rx_sci_async_tx(const struct device *dev, const uint8_t *buf, size_t len,
				int32_t timeout)
{
	struct uart_rx_sci_data *data = dev->data;
	int err;
	unsigned int key = irq_lock();
	/* Transmit interrupts must be disabled to start with. */
	disable_tx(dev);

	err = configure_tx_transfer(dev, buf, len);

	if (err != 0) {
		goto end;
	}

#ifdef CONFIG_PM
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif

	enable_tx(dev);
	data->tx_buffer = (uint8_t *)buf;
	data->tx_buf_cap = len;

	if (timeout != SYS_FOREVER_US && timeout != 0) {
		k_work_reschedule(&data->tx_timeout, Z_TIMEOUT_US(timeout));
	}
end:
	irq_unlock(key);
	return err;
}

static inline void async_user_callback(const struct device *dev, struct uart_event *event)
{
	struct uart_rx_sci_data *data = dev->data;

	if (data->async_user_cb) {
		data->async_user_cb(dev, event, data->async_user_cb_data);
	}
}

static inline void async_rx_release_buf(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx.buf = (uint8_t *)data->rx_buffer,
	};

	async_user_callback(dev, &event);
	data->rx_buffer = NULL;
	data->rx_buf_offset = 0;
	data->rx_buf_len = 0;
	data->rx_buf_cap = 0;
}

static inline void async_rx_release_next_buf(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	if (data->rx_next_buf == NULL) {
		return;
	}

	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx.buf = (uint8_t *)data->rx_next_buf,
	};
	async_user_callback(dev, &event);
	data->rx_next_buf = NULL;
	data->rx_next_buf_cap = 0;
}

static inline void async_rx_req_buf(const struct device *dev)
{
	struct uart_event event = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(dev, &event);
}

static inline void async_rx_disable(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	struct uart_event event = {
		.type = UART_RX_DISABLED,
	};
	async_user_callback(dev, &event);

	/* Disable the RXI request and clear the status flag to be ready for the next reception */
	uart_rx_irq_rx_disable(dev);

	sci->SSR.BIT.RDRF = 0;
}

static inline void async_rx_ready(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	if (!data->rx_buf_len) {
		return;
	}

	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = (uint8_t *)data->rx_buffer,
		.data.rx.offset = data->rx_buf_offset,
		.data.rx.len = data->rx_buf_len,
	};
	async_user_callback(data->dev, &event);
	data->rx_buf_offset += data->rx_buf_len;
	data->rx_buf_len = 0;
}

static int uart_rx_sci_async_tx_abort(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;
	int err;

	int key = irq_lock();

	disable_tx(dev);
	k_work_cancel_delayable(&data->tx_timeout);

	transfer_properties_t tx_properties = {0};

	err = dtc_renesas_rx_info_get(dev, data->txi_irq, &tx_properties);

	if (err != 0) {
		err = -EIO;
		goto end;
	}

	data->tx_buf_len = data->tx_buf_cap - tx_properties.transfer_length_remaining;

	dtc_renesas_rx_disable_transfer(data->txi_irq);

	if (data->tx_buf_len < data->tx_buf_cap) {
		struct uart_event event = {
			.type = UART_TX_ABORTED,
			.data.tx.buf = (uint8_t *)data->tx_transfer_info.p_src,
			.data.tx.len = data->tx_buf_cap - tx_properties.transfer_length_remaining,
		};
		async_user_callback(dev, &event);
	}

	data->tx_buffer = NULL;
	data->tx_buf_cap = 0;

end:
	irq_unlock(key);
	enable_tx(dev);
	return err;
}

static inline void uart_rx_sci_async_timer_start(struct k_work_delayable *work, size_t timeout)
{
	if (timeout != SYS_FOREVER_US && timeout != 0) {
		LOG_DBG("Async timer started for %lu us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static int uart_rx_sci_async_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
				       int32_t timeout)
{
	struct uart_rx_sci_data *data = dev->data;
	struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	int err;
	unsigned int key = irq_lock();

	if (data->rx_buffer) {
		err = -EAGAIN;
		goto unlock;
	}

	sci->SSR.BYTE &= ~(uint8_t)(r_SCI_SSR_ERRORS_MASK);

	/* config DTC */
	err = configure_rx_transfer(dev, buf, len);

	if (err != 0) {
		goto unlock;
	}

	data->rx_timeout = timeout;
	data->rx_buffer = buf;
	data->rx_buf_cap = len;
	data->rx_buf_len = 0;
	data->rx_buf_offset = 0;

	/* call buffer request user callback */
	uart_rx_irq_rx_enable(dev);
	async_rx_req_buf(dev);

unlock:
	irq_unlock(key);
	return err;
}

static int uart_rx_sci_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_rx_sci_data *data = dev->data;

	data->rx_next_buf = buf;
	data->rx_next_buf_cap = len;

	return 0;
}
static int uart_rx_sci_async_rx_disable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;
	int err = 0;
	unsigned int key = irq_lock();

	if (!data->rx_buffer) {
		err = -EAGAIN;
		goto unlock;
	}
	transfer_properties_t rx_properties = {0};

	err = dtc_renesas_rx_info_get(dev, data->rxi_irq, &rx_properties);
	if (err != 0) {
		err = -EIO;
		goto unlock;
	}
	data->rx_buf_len =
		data->rx_buf_cap - rx_properties.transfer_length_remaining - data->rx_buf_offset;

	/* stop transfer dtc */
	err = dtc_renesas_rx_disable_transfer(data->rxi_irq);
	if (err != 0) {
		goto unlock;
	}
	k_work_cancel_delayable(&data->rx_timeout_work);
	async_rx_ready(dev);
	async_rx_release_buf(dev);
	async_rx_release_next_buf(dev);
	async_rx_disable(dev);

unlock:
	irq_unlock(key);
	return err;
}

static void uart_rx_sci_rx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_rx_sci_data *data =
		CONTAINER_OF(dwork, struct uart_rx_sci_data, rx_timeout_work);
	unsigned int key = irq_lock();

	async_rx_ready(data->dev);
	irq_unlock(key);
}

static void uart_rx_sci_tx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_rx_sci_data *data = CONTAINER_OF(dwork, struct uart_rx_sci_data, tx_timeout);

	uart_rx_sci_async_tx_abort(data->dev);
}

#endif /* CONFIG_UART_ASYNC_API */

static int uart_rx_init(const struct device *dev)
{
	const struct uart_rx_sci_config *config = dev->config;
	struct uart_rx_sci_data *data = dev->data;
	int ret;
	sci_err_t sci_err;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = uart_rx_sci_apply_config(&data->uart_config, &data->sci_config);
	if (ret) {
		return ret;
	}

#if defined(CONFIG_UART_ASYNC_API)
	k_work_init_delayable(&data->rx_timeout_work, uart_rx_sci_rx_timeout_handler);
	k_work_init_delayable(&data->tx_timeout, uart_rx_sci_tx_timeout_handler);
#endif

	sci_err = R_SCI_Open(data->channel, SCI_MODE_ASYNC, &data->sci_config, NULL, &data->hdl);
	if (sci_err) {
		return -EIO;
	}

	/* Set the Asynchronous Start Bit Edge Detection Select to falling edge on the RXDn pin */
	sci_err = R_SCI_Control(data->hdl, SCI_CMD_START_BIT_EDGE, FIT_NO_PTR);
	if (sci_err) {
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int uart_rx_sci_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_rx_sci_config *config = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = clock_control_on(config->clock,
				       (clock_control_subsys_t)&config->clock_subsys);
		if (ret < 0) {
			return ret;
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = clock_control_off(config->clock,
					(clock_control_subsys_t)&config->clock_subsys);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

static DEVICE_API(uart, uart_rx_driver_api) = {
	.poll_in = uart_rx_sci_poll_in,
	.poll_out = uart_rx_sci_poll_out,
	.err_check = uart_rx_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rx_configure,
	.config_get = uart_rx_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rx_fifo_fill,
	.fifo_read = uart_rx_fifo_read,
	.irq_tx_enable = uart_rx_irq_tx_enable,
	.irq_tx_disable = uart_rx_irq_tx_disable,
	.irq_tx_ready = uart_rx_irq_tx_ready,
	.irq_rx_enable = uart_rx_irq_rx_enable,
	.irq_rx_disable = uart_rx_irq_rx_disable,
	.irq_tx_complete = uart_rx_irq_tx_complete,
	.irq_rx_ready = uart_rx_irq_rx_ready,
	.irq_err_enable = uart_rx_irq_err_enable,
	.irq_err_disable = uart_rx_irq_err_disable,
	.irq_is_pending = uart_rx_irq_is_pending,
	.irq_update = uart_rx_irq_update,
	.irq_callback_set = uart_rx_irq_callback_set,
#endif
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_rx_sci_async_callback_set,
	.tx = uart_rx_sci_async_tx,
	.tx_abort = uart_rx_sci_async_tx_abort,
	.rx_enable = uart_rx_sci_async_rx_enable,
	.rx_buf_rsp = uart_rx_sci_async_rx_buf_rsp,
	.rx_disable = uart_rx_sci_async_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
static void uart_rx_sci_rxi_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif
#if defined(CONFIG_UART_ASYNC_API)
	if (data->rx_timeout != SYS_FOREVER_US && data->rx_timeout != 0) {
		k_work_reschedule(&data->rx_timeout_work, Z_TIMEOUT_US(data->rx_timeout));
	}
	data->rx_buf_len++;
	if (data->rx_buf_len + data->rx_buf_offset == data->rx_buf_cap) {
		async_rx_ready(dev);
		async_rx_release_buf(dev);
		if (data->rx_next_buf) {
			data->rx_buffer = data->rx_next_buf;
			data->rx_buf_offset = 0;
			data->rx_buf_cap = data->rx_next_buf_cap;
			data->rx_next_buf = NULL;
			configure_rx_transfer(dev, data->rx_buffer, data->rx_buf_cap);
			async_rx_req_buf(dev);
			uart_rx_irq_rx_enable(dev);
		} else {
			async_rx_disable(dev);
		}
	}
#endif
}

static void uart_rx_sci_txi_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif
#if defined(CONFIG_UART_ASYNC_API)
	struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BIT.TEIE = 1;
	sci->SCR.BIT.TIE = 0;
#endif
}

static void uart_rx_sci_tei_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif
#if defined(CONFIG_UART_ASYNC_API)
	/* TODO */
	struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BIT.TEIE = 0;
	k_work_cancel_delayable(&data->tx_timeout);
	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = (uint8_t *)data->tx_transfer_info.p_src,
		.data.tx.len = data->tx_buf_cap,
	};
	async_user_callback(dev, &event);
#ifdef CONFIG_PM
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
#endif
}

static void uart_rx_sci_eri_isr(const struct device *dev)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
#endif
#if defined(CONFIG_UART_ASYNC_API)
	/* TODO */
	struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	uint8_t reason;

	if (sci->SSR.BIT.ORER) {
		reason = UART_ERROR_OVERRUN;
	} else if (sci->SSR.BIT.PER) {
		reason = UART_ERROR_PARITY;
	} else if (sci->SSR.BIT.FER) {
		reason = UART_ERROR_FRAMING;
	} else {
		reason = UART_ERROR_OVERRUN;
	}

	/* Clear error flags */
	sci->SSR.BYTE &=
		~(BIT(R_SCI_SSR_ORER_Pos) | BIT(R_SCI_SSR_PER_Pos) | BIT(R_SCI_SSR_FER_Pos));

	k_work_cancel_delayable(&data->rx_timeout_work);
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = reason,
		.data.rx_stop.data.buf = (uint8_t *)data->rx_buffer,
		.data.rx_stop.data.offset = 0,
		.data.rx_stop.data.len = data->rx_buf_cap - data->rx_buf_len - data->rx_buf_offset,
	};
	async_user_callback(dev, &event);
#endif
}
#endif

#if defined(CONFIG_SOC_SERIES_RX26T)
#define UART_RX_SCI_CONFIG_INIT(index)
#else
#define UART_RX_SCI_CONFIG_INIT(index)                                                             \
	.rxi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),                                \
	.txi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                                \
	.tei_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                                \
	.eri_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),
#endif

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
#ifndef CONFIG_SOC_SERIES_RX26T
#define UART_RX_SCI_IRQ_INIT(index)                                                                \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, priority),                  \
			    uart_rx_sci_rxi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),                  \
			    uart_rx_sci_txi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, priority),                  \
			    uart_rx_sci_tei_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, priority),                  \
			    uart_rx_sci_eri_isr, DEVICE_DT_INST_GET(index), 0);                    \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq));                       \
	} while (0)
#endif
#else
#define UART_RX_SCI_IRQ_INIT(index)
#endif

#if defined(CONFIG_UART_ASYNC_API)
#define UART_RX_SCI_ASYNC_INIT(index)                                                              \
	.dtc = DEVICE_DT_GET(DT_PHANDLE(DT_INST_PARENT(index), dtc)),                              \
	.rx_transfer_info =                                                                        \
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
	.tx_transfer_info = {                                                                      \
		.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,               \
		.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,               \
		.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                                  \
		.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,               \
		.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,          \
		.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                             \
		.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                             \
		.p_dest = (void *)NULL,                                                            \
		.p_src = (void const *)NULL,                                                       \
		.num_blocks = 0,                                                                   \
		.length = 0,                                                                       \
	},
#else
#define UART_RX_SCI_ASYNC_INIT(index)
#endif

#define UART_RX_INIT(index)                                                                        \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
                                                                                                   \
	static const struct uart_rx_sci_config uart_rx_sci_config_##index = {                      \
		.regs = DT_REG_ADDR(DT_INST_PARENT(index)),                                        \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.clock = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(index))),                     \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = DT_CLOCKS_CELL(DT_INST_PARENT(index), mstp),               \
				.stop_bit = DT_CLOCKS_CELL(DT_INST_PARENT(index), stop_bit),       \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct uart_rx_sci_data uart_rx_sci_data_##index = {                                \
		.dev = DEVICE_DT_GET(DT_DRV_INST(index)),                                          \
		.channel = DT_PROP(DT_INST_PARENT(index), channel),                                \
		.sci_config = {},                                                                  \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
		UART_RX_SCI_CONFIG_INIT(index) UART_RX_SCI_ASYNC_INIT(index)};                     \
                                                                                                   \
	static int uart_rx_init_##index(const struct device *dev)                                  \
	{                                                                                          \
		UART_RX_SCI_IRQ_INIT(index);                                                       \
		int err = uart_rx_init(dev);                                                       \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(index, uart_rx_sci_pm_action);                                    \
	DEVICE_DT_INST_DEFINE(index, uart_rx_init_##index, PM_DEVICE_DT_INST_GET(index),           \
			      &uart_rx_sci_data_##index, &uart_rx_sci_config_##index,              \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RX_INIT)
