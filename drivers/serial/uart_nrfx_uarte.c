/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF UARTE
 */

#include <drivers/uart.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_uarte.h>
#include <nrfx_timer.h>
#include <sys/util.h>
#include <kernel.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(uart_nrfx_uarte, LOG_LEVEL_ERR);

/* Generalize PPI or DPPI channel management */
#if defined(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#define gppi_channel_t nrf_ppi_channel_t
#define gppi_channel_alloc nrfx_ppi_channel_alloc
#define gppi_channel_enable nrfx_ppi_channel_enable
#elif defined(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#define gppi_channel_t u8_t
#define gppi_channel_alloc nrfx_dppi_channel_alloc
#define gppi_channel_enable nrfx_dppi_channel_enable
#else
#error "No PPI or DPPI"
#endif


#if (defined(CONFIG_UART_0_NRF_UARTE) &&         \
     defined(CONFIG_UART_0_INTERRUPT_DRIVEN)) || \
    (defined(CONFIG_UART_1_NRF_UARTE) &&         \
     defined(CONFIG_UART_1_INTERRUPT_DRIVEN)) || \
    (defined(CONFIG_UART_2_NRF_UARTE) &&         \
     defined(CONFIG_UART_2_INTERRUPT_DRIVEN)) || \
    (defined(CONFIG_UART_3_NRF_UARTE) &&         \
     defined(CONFIG_UART_3_INTERRUPT_DRIVEN))
	#define UARTE_INTERRUPT_DRIVEN	1
#endif

/*
 * RX timeout is divided into time slabs, this define tells how many divisions
 * should be made. More divisions - higher timeout accuracy and processor usage.
 */
#define RX_TIMEOUT_DIV 5

#ifdef CONFIG_UART_ASYNC_API
struct uarte_async_cb {
	uart_callback_t user_callback;
	void *user_data;

	/* tx_buf has to be volatile it is used as busy flag in uart_tx and
	 * uart_poll_out. If both tx_buf and tx_size is set then there is
	 * currently ongoing asynchronous transmission. If only tx_size
	 * is bigger than 0 and tx_buf is NULL, then there is ongoing
	 * transmission by uart_poll_out
	 */
	const u8_t *volatile tx_buf;
	size_t tx_size;
	struct k_timer tx_timeout_timer;

	u8_t *rx_buf;
	size_t rx_offset;
	u8_t *rx_next_buf;
	u32_t rx_total_byte_cnt; /* Total number of bytes received */
	u32_t rx_total_user_byte_cnt; /* Total number of bytes passed to user */
	u32_t rx_timeout; /* Timeout set by user */
	s32_t rx_timeout_slab; /* rx_timeout divided by RX_TIMEOUT_DIV */
	s32_t rx_timeout_left; /* Current time left until user callback */
	struct k_timer rx_timeout_timer;
	union {
		gppi_channel_t ppi;
		u32_t cnt;
	} rx_cnt;

	bool rx_enabled;
	bool hw_rx_counting;
	/* Flag to ensure that RX timeout won't be executed during ENDRX ISR */
	volatile bool is_in_irq;
};
#endif

#ifdef UARTE_INTERRUPT_DRIVEN
struct uarte_nrfx_int_driven {
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *cb_data; /**< Callback function arg */
	u8_t *tx_buffer;
	u16_t tx_buff_size;
	volatile bool disable_tx_irq;
};
#endif

/* Device data structure */
struct uarte_nrfx_data {
	struct uart_config uart_config;
#ifdef UARTE_INTERRUPT_DRIVEN
	struct uarte_nrfx_int_driven *int_driven;
#endif
#ifdef CONFIG_UART_ASYNC_API
	struct uarte_async_cb *async;
#endif
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t pm_state;
#endif
	u8_t rx_data;
};

/**
 * @brief Structure for UARTE configuration.
 */
struct uarte_nrfx_config {
	NRF_UARTE_Type *uarte_regs; /* Instance address */
	bool rts_cts_pins_set;
	bool gpio_mgmt;
#ifdef CONFIG_UART_ASYNC_API
	nrfx_timer_t timer;
#endif
};

struct uarte_init_config {
	u32_t  pseltxd; /* TXD pin number */
	u32_t  pselrxd; /* RXD pin number */
	u32_t  pselcts; /* CTS pin number */
	u32_t  pselrts; /* RTS pin number */
};

static inline struct uarte_nrfx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct uarte_nrfx_config *get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static inline NRF_UARTE_Type *get_uarte_instance(struct device *dev)
{
	const struct uarte_nrfx_config *config = get_dev_config(dev);

	return config->uarte_regs;
}

#ifdef UARTE_INTERRUPT_DRIVEN
/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void uarte_nrfx_isr_int(void *arg)
{
	struct device *dev = arg;
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->int_driven->disable_tx_irq &&
	    nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDTX_MASK);

		/* If there is nothing to send, driver will save an energy
		 * when TX is stopped.
		 */
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);

		data->int_driven->disable_tx_irq = false;

		return;
	}

	if (data->int_driven->cb) {
		data->int_driven->cb(data->int_driven->cb_data);
	}
}
#endif /* UARTE_INTERRUPT_DRIVEN */

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UARTE.
 *
 * @param dev UARTE device struct
 * @param baudrate Baud rate
 *
 * @return 0 on success or error code
 */
static int baudrate_set(struct device *dev, u32_t baudrate)
{
	nrf_uarte_baudrate_t nrf_baudrate; /* calculated baudrate divisor */
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	switch (baudrate) {
	case 300:
		/* value not supported by Nordic HAL */
		nrf_baudrate = 0x00014000;
		break;
	case 600:
		/* value not supported by Nordic HAL */
		nrf_baudrate = 0x00027000;
		break;
	case 1200:
		nrf_baudrate = NRF_UARTE_BAUDRATE_1200;
		break;
	case 2400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_2400;
		break;
	case 4800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_4800;
		break;
	case 9600:
		nrf_baudrate = NRF_UARTE_BAUDRATE_9600;
		break;
	case 14400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_14400;
		break;
	case 19200:
		nrf_baudrate = NRF_UARTE_BAUDRATE_19200;
		break;
	case 28800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_28800;
		break;
	case 31250:
		nrf_baudrate = NRF_UARTE_BAUDRATE_31250;
		break;
	case 38400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_38400;
		break;
	case 56000:
		nrf_baudrate = NRF_UARTE_BAUDRATE_56000;
		break;
	case 57600:
		nrf_baudrate = NRF_UARTE_BAUDRATE_57600;
		break;
	case 76800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_76800;
		break;
	case 115200:
		nrf_baudrate = NRF_UARTE_BAUDRATE_115200;
		break;
	case 230400:
		nrf_baudrate = NRF_UARTE_BAUDRATE_230400;
		break;
	case 250000:
		nrf_baudrate = NRF_UARTE_BAUDRATE_250000;
		break;
	case 460800:
		nrf_baudrate = NRF_UARTE_BAUDRATE_460800;
		break;
	case 921600:
		nrf_baudrate = NRF_UARTE_BAUDRATE_921600;
		break;
	case 1000000:
		nrf_baudrate = NRF_UARTE_BAUDRATE_1000000;
		break;
	default:
		return -EINVAL;
	}

	nrf_uarte_baudrate_set(uarte, nrf_baudrate);

	return 0;
}

static int uarte_nrfx_configure(struct device *dev,
				const struct uart_config *cfg)
{
	nrf_uarte_parity_t parity;
	nrf_uarte_hwfc_t hwfc;
#ifdef UARTE_CONFIG_STOP_Two
	bool two_stop_bits = false;
#endif

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		break;
#ifdef UARTE_CONFIG_STOP_Two
	case UART_CFG_STOP_BITS_2:
		two_stop_bits = true;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		hwfc = NRF_UARTE_HWFC_DISABLED;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		if (get_dev_config(dev)->rts_cts_pins_set) {
			hwfc = NRF_UARTE_HWFC_ENABLED;
		} else {
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		parity = NRF_UARTE_PARITY_EXCLUDED;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = NRF_UARTE_PARITY_INCLUDED;
		break;
	default:
		return -ENOTSUP;
	}

	if (baudrate_set(dev, cfg->baudrate) != 0) {
		return -ENOTSUP;
	}

	nrf_uarte_configure(get_uarte_instance(dev), parity, hwfc);

#ifdef UARTE_CONFIG_STOP_Two
	if (two_stop_bits) {
		/* TODO Change this to nrfx HAL function when available */
		get_uarte_instance(dev)->CONFIG |=
			UARTE_CONFIG_STOP_Two << UARTE_CONFIG_STOP_Pos;
	}
#endif
	get_dev_data(dev)->uart_config = *cfg;

	return 0;
}

static int uarte_nrfx_config_get(struct device *dev, struct uart_config *cfg)
{
	*cfg = get_dev_data(dev)->uart_config;
	return 0;
}


static int uarte_nrfx_err_check(struct device *dev)
{
	u32_t error = 0U;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ERROR)) {
		/* register bitfields maps to the defines in uart.h */
		error = nrf_uarte_errorsrc_get_and_clear(uarte);
	}

	return error;
}

#ifdef CONFIG_UART_ASYNC_API

static inline bool hw_rx_counting_enabled(struct uarte_nrfx_data *data)
{
	if (IS_ENABLED(CONFIG_UARTE_NRF_HW_ASYNC)) {
		return data->async->hw_rx_counting;
	} else {
		return false;
	}
}

static void timer_handler(nrf_timer_event_t event_type, void *p_context) { }
static void rx_timeout(struct k_timer *timer);
static void tx_timeout(struct k_timer *timer);

static int uarte_nrfx_rx_counting_init(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	const struct uarte_nrfx_config *cfg = get_dev_config(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	int ret;

	if (hw_rx_counting_enabled(data)) {
		nrfx_timer_config_t tmr_config = NRFX_TIMER_DEFAULT_CONFIG;

		tmr_config.mode = NRF_TIMER_MODE_COUNTER;
		tmr_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
		ret = nrfx_timer_init(&cfg->timer,
				      &tmr_config,
				      timer_handler);
		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Timer already initialized, "
				"switching to software byte counting.");
			data->async->hw_rx_counting = false;
		} else {
			nrfx_timer_enable(&cfg->timer);
			nrfx_timer_clear(&cfg->timer);
		}
	}

	if (hw_rx_counting_enabled(data)) {
		ret = gppi_channel_alloc(&data->async->rx_cnt.ppi);
		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Failed to allocate PPI Channel, "
				"switching to software byte counting.");
			data->async->hw_rx_counting = false;
			nrfx_timer_uninit(&cfg->timer);
		}
	}

	if (hw_rx_counting_enabled(data)) {
#if CONFIG_HAS_HW_NRF_PPI
		ret = nrfx_ppi_channel_assign(
			data->async->rx_cnt.ppi,
			nrf_uarte_event_address_get(uarte,
						    NRF_UARTE_EVENT_RXDRDY),
			nrfx_timer_task_address_get(&cfg->timer,
						    NRF_TIMER_TASK_COUNT));

		if (ret != NRFX_SUCCESS) {
			return -EIO;
		}
#else
		nrf_uarte_publish_set(uarte,
				      NRF_UARTE_EVENT_RXDRDY,
				      data->async->rx_cnt.ppi);
		nrf_timer_subscribe_set(cfg->timer.p_reg,
					NRF_TIMER_TASK_COUNT,
					data->async->rx_cnt.ppi);

#endif
		ret = gppi_channel_enable(data->async->rx_cnt.ppi);
		if (ret != NRFX_SUCCESS) {
			return -EIO;
		}
	} else {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_RXDRDY_MASK);
	}

	return 0;
}

static int uarte_nrfx_init(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	int ret = uarte_nrfx_rx_counting_init(dev);

	if (ret != 0) {
		return ret;
	}
	nrf_uarte_int_enable(uarte,
			     NRF_UARTE_INT_ENDRX_MASK |
			     NRF_UARTE_INT_RXSTARTED_MASK |
			     NRF_UARTE_INT_ERROR_MASK |
			     NRF_UARTE_INT_ENDTX_MASK |
			     NRF_UARTE_INT_TXSTOPPED_MASK |
			     NRF_UARTE_INT_RXTO_MASK);
	nrf_uarte_enable(uarte);

	k_timer_init(&data->async->rx_timeout_timer, rx_timeout, NULL);
	k_timer_user_data_set(&data->async->rx_timeout_timer, dev);
	k_timer_init(&data->async->tx_timeout_timer, tx_timeout, NULL);
	k_timer_user_data_set(&data->async->tx_timeout_timer, dev);

	return 0;
}

static int uarte_nrfx_tx(struct device *dev, const u8_t *buf, size_t len,
			 u32_t timeout)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (!nrfx_is_in_ram(buf)) {
		return -ENOTSUP;
	}

	if (data->async->tx_buf || data->async->tx_size) {
		return -EBUSY;
	}
	data->async->tx_buf = buf;
	data->async->tx_size = len;
	nrf_uarte_tx_buffer_set(uarte, buf, len);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);
	if (data->uart_config.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		k_timer_start(&data->async->tx_timeout_timer, timeout,
			      K_NO_WAIT);
	}
	return 0;
}

static int uarte_nrfx_tx_abort(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->async->tx_buf == NULL) {
		return -EFAULT;
	}
	k_timer_stop(&data->async->tx_timeout_timer);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);

	return 0;
}

static int uarte_nrfx_rx_enable(struct device *dev, u8_t *buf, size_t len,
				u32_t timeout)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	const struct uarte_nrfx_config *cfg = get_dev_config(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (hw_rx_counting_enabled(data)) {
		nrfx_timer_clear(&cfg->timer);
	} else {
		data->async->rx_cnt.cnt = 0;
	}
	data->async->rx_total_byte_cnt = 0;
	data->async->rx_total_user_byte_cnt = 0;

	data->async->rx_timeout = timeout;
	data->async->rx_timeout_slab =
		MAX(timeout / RX_TIMEOUT_DIV,
		    NRFX_CEIL_DIV(1000, CONFIG_SYS_CLOCK_TICKS_PER_SEC));

	data->async->rx_buf = buf;
	data->async->rx_offset = 0;
	nrf_uarte_rx_buffer_set(uarte, buf, len);

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);

	data->async->rx_enabled = true;
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
	return 0;
}

static int uarte_nrfx_rx_buf_rsp(struct device *dev, u8_t *buf, size_t len)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->async->rx_next_buf == NULL) {
		data->async->rx_next_buf = buf;
		nrf_uarte_rx_buffer_set(uarte, buf, len);
	} else {
		return -EBUSY;
	}
	return 0;
}

static int uarte_nrfx_callback_set(struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	data->async->user_callback = callback;
	data->async->user_data = user_data;

	return 0;
}

static int uarte_nrfx_rx_disable(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->async->rx_buf == NULL) {
		return -EFAULT;
	}

	k_timer_stop(&data->async->rx_timeout_timer);
	data->async->rx_enabled = false;

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);

	return 0;
}

static void tx_timeout(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	(void) uarte_nrfx_tx_abort(dev);
}

static void user_callback(struct device *dev, struct uart_event *evt)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (data->async->user_callback) {
		data->async->user_callback(evt, data->async->user_data);
	}
}

/**
 * Whole timeout is divided by RX_TIMEOUT_DIV into smaller units, rx_timeout
 * is executed periodically every rx_timeout_slab ms. If between executions
 * data was received, then we start counting down time from start, if not, then
 * we subtract rx_timeout_slab from rx_timeout_left.
 * If rx_timeout_left is less than rx_timeout_slab it means that receiving has
 * timed out and we should tell user about that.
 */
static void rx_timeout(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	struct uarte_nrfx_data *data = get_dev_data(dev);
	const struct uarte_nrfx_config *cfg = get_dev_config(dev);
	u32_t read;

	if (data->async->is_in_irq) {
		return;
	}

	/* Disable ENDRX ISR, in case ENDRX event is generated, it will be
	 * handled after rx_timeout routine is complete.
	 */
	nrf_uarte_int_disable(get_uarte_instance(dev),
			      NRF_UARTE_INT_ENDRX_MASK);

	if (hw_rx_counting_enabled(data)) {
		read = nrfx_timer_capture(&cfg->timer, 0);
	} else {
		read = data->async->rx_cnt.cnt;
	}

	/* Check if data was received since last function call */
	if (read != data->async->rx_total_byte_cnt) {
		data->async->rx_total_byte_cnt = read;
		data->async->rx_timeout_left = data->async->rx_timeout;
	}

	/* Check if there is data that was not sent to user yet */
	if (data->async->rx_total_byte_cnt
	    != data->async->rx_total_user_byte_cnt) {
		if (data->async->rx_timeout_left
		    < data->async->rx_timeout_slab) {
			/* rx_timeout ms elapsed since last receiving */
			u32_t len = data->async->rx_total_byte_cnt
				    - data->async->rx_total_user_byte_cnt;
			struct uart_event evt = {
				.type = UART_RX_RDY,
				.data.rx.buf = data->async->rx_buf,
				.data.rx.len = len,
				.data.rx.offset = data->async->rx_offset
			};
			data->async->rx_offset += len;
			data->async->rx_total_user_byte_cnt =
				data->async->rx_total_byte_cnt;
			user_callback(dev, &evt);
		} else {
			data->async->rx_timeout_left -=
				data->async->rx_timeout_slab;
		}
	}

	nrf_uarte_int_enable(get_uarte_instance(dev),
			     NRF_UARTE_INT_ENDRX_MASK);
}

#define UARTE_ERROR_FROM_MASK(mask)					\
	((mask) & NRF_UARTE_ERROR_OVERRUN_MASK ? UART_ERROR_OVERRUN	\
	 : (mask) & NRF_UARTE_ERROR_PARITY_MASK ? UART_ERROR_PARITY	\
	 : (mask) & NRF_UARTE_ERROR_FRAMING_MASK ? UART_ERROR_FRAMING	\
	 : (mask) & NRF_UARTE_ERROR_BREAK_MASK ? UART_BREAK		\
	 : 0)

static void error_isr(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	u32_t err = nrf_uarte_errorsrc_get_and_clear(uarte);
	struct uart_event evt = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = UARTE_ERROR_FROM_MASK(err),
	};
	user_callback(dev, &evt);
	(void) uarte_nrfx_rx_disable(dev);
}

static void rxstarted_isr(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};
	user_callback(dev, &evt);
	if (data->async->rx_timeout) {
		data->async->rx_timeout_left = data->async->rx_timeout;
		k_timer_start(&data->async->rx_timeout_timer,
			      data->async->rx_timeout_slab,
			      data->async->rx_timeout_slab);
	}
}

static void endrx_isr(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (!data->async->rx_enabled) {
		return;
	}

	data->async->is_in_irq = true;

	if (data->async->rx_next_buf) {
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
	}
	k_timer_stop(&data->async->rx_timeout_timer);

	size_t rx_len = nrf_uarte_rx_amount_get(uarte)
			- data->async->rx_offset;

	data->async->rx_total_user_byte_cnt += rx_len;

	if (!hw_rx_counting_enabled(data)) {
		/* Prevent too low value of rx_cnt.cnt which may occur due to
		 * latencies in handling of the RXRDY interrupt. Because whole
		 * buffer was filled we can be sure that rx_total_user_byte_cnt
		 * is current total number of received bytes.
		 */
		data->async->rx_cnt.cnt = data->async->rx_total_user_byte_cnt;
	}

	struct uart_event evt = {
		.type = UART_RX_RDY,
		.data.rx.buf = data->async->rx_buf,
		.data.rx.len = rx_len,
		.data.rx.offset = data->async->rx_offset,
	};
	user_callback(dev, &evt);

	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = data->async->rx_buf;
	user_callback(dev, &evt);

	if (data->async->rx_next_buf) {
		data->async->rx_buf = data->async->rx_next_buf;
		data->async->rx_next_buf = NULL;

		data->async->rx_offset = 0;
	} else {
		data->async->rx_buf = NULL;
		evt.type = UART_RX_DISABLED;
		user_callback(dev, &evt);
	}

	data->async->is_in_irq = false;
}

/* This handler is called when the reception is interrupted, in contrary to
 * finishing the reception after filling all provided buffers, in which case
 * the events UART_RX_BUF_RELEASED and UART_RX_DISABLED are reported
 * from endrx_isr.
 */
static void rxto_isr(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->async->rx_buf,
	};
	user_callback(dev, &evt);

	data->async->rx_buf = NULL;
	if (data->async->rx_next_buf) {
		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = data->async->rx_next_buf;
		user_callback(dev, &evt);
		data->async->rx_next_buf = NULL;
	}
	evt.type = UART_RX_DISABLED;

	user_callback(dev, &evt);
}

static void txstopped_isr(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (!data->async->tx_buf) {
		return;
	}

	size_t amount = nrf_uarte_tx_amount_get(get_uarte_instance(dev));

	struct uart_event evt = {
		.data.tx.buf = data->async->tx_buf,
		.data.tx.len = amount,
	};
	if (amount == data->async->tx_size) {
		evt.type = UART_TX_DONE;
	} else {
		evt.type = UART_TX_ABORTED;
	}
	data->async->tx_buf = NULL;
	data->async->tx_size = 0;
	user_callback(dev, &evt);
}

static void endtx_isr(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
	k_timer_stop(&data->async->tx_timeout_timer);
}

static void uarte_nrfx_isr_async(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (!hw_rx_counting_enabled(data)
	    && nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXDRDY)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXDRDY);
		data->async->rx_cnt.cnt++;
		return;
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ERROR)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ERROR);
		error_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
		rxstarted_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
		endrx_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXTO)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
		rxto_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
		endtx_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTOPPED);
		txstopped_isr(dev);
	}
}

#endif /* CONFIG_UART_ASYNC_API */

/**
 * @brief Poll the device for input.
 *
 * @param dev UARTE device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer is empty.
 */
static int uarte_nrfx_poll_in(struct device *dev, unsigned char *c)
{

	const struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#ifdef CONFIG_UART_ASYNC_API
	if (data->async) {
		return -ENOTSUP;
	}
#endif

	if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		return -1;
	}

	*c = data->rx_data;

	/* clear the interrupt */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UARTE device struct
 * @param c Character to send
 */
static void uarte_nrfx_poll_out(struct device *dev, unsigned char c)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#ifdef CONFIG_UART_ASYNC_API
	const struct uarte_nrfx_data *data = get_dev_data(dev);

	if (data->async) {
		while (data->async->tx_buf) {
			/* If there is ongoing transmission, and we are in
			 * isr, then call uarte interrupt routine, otherwise
			 * busy wait until transmission is finished.
			 */
			if (k_is_in_isr()) {
				uarte_nrfx_isr_async(dev);
			}
		}
		/* Set tx_size but don't set tx_buf to differentiate this
		 * transmission from the one started with uarte_nrfx_tx,
		 * this way uarte_nrfx_tx will return -EBUSY and poll out will
		 * work when interrupted.
		 */
		data->async->tx_size = 1;
		nrf_uarte_int_disable(uarte,
				      NRF_UARTE_INT_ENDTX_MASK |
				      NRF_UARTE_INT_TXSTOPPED_MASK);
	}
#endif
	/* The UART API dictates that poll_out should wait for the transmitter
	 * to be empty before sending a character. However, the only way of
	 * telling if the transmitter became empty in the UARTE peripheral is
	 * to check if the ENDTX event for the previous transmission was set.
	 * Since this event is not cleared automatically when a new transmission
	 * is started, it must be cleared in software, and this leads to a rare
	 * yet possible race condition if the thread is preempted right after
	 * clearing the event but before sending a new character. The preempting
	 * thread, if it also called poll_out, would then wait for the ENDTX
	 * event that had no chance to become set.

	 * Because of this race condition, the while loop has to be placed
	 * after the write to TXD, and we can't wait for an empty transmitter
	 * before writing. This is a trade-off between losing a byte once in a
	 * blue moon against hanging up the whole thread permanently
	 */

	/* reset transmitter ready state */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);

	/* send a character */
	nrf_uarte_tx_buffer_set(uarte, &c, 1);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

	/* Wait for transmitter to be ready */
	while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
	}

	/* If there is nothing to send, driver will save an energy
	 * when TX is stopped.
	 */
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);

#ifdef CONFIG_UART_ASYNC_API
	if (data->async) {
		data->async->tx_size = 0;
		nrf_uarte_int_enable(uarte,
				     NRF_UARTE_INT_ENDTX_MASK |
				     NRF_UARTE_INT_TXSTOPPED_MASK);

	}
#endif
}
#ifdef UARTE_INTERRUPT_DRIVEN
/** Interrupt driven FIFO fill function */
static int uarte_nrfx_fifo_fill(struct device *dev,
				const u8_t *tx_data,
				int len)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		return 0;
	}

	if (len > data->int_driven->tx_buff_size) {
		len = data->int_driven->tx_buff_size;
	}

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);

	/* Copy data to RAM buffer for EasyDMA transfer */
	for (int i = 0; i < len; i++) {
		data->int_driven->tx_buffer[i] = tx_data[i];
	}

	nrf_uarte_tx_buffer_set(uarte, data->int_driven->tx_buffer, len);

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

	return len;
}

/** Interrupt driven FIFO read function */
static int uarte_nrfx_fifo_read(struct device *dev,
				u8_t *rx_data,
				const int size)
{
	int num_rx = 0;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_data *data = get_dev_data(dev);

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* Clear the interrupt */
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

		/* Receive a character */
		rx_data[num_rx++] = (u8_t)data->rx_data;

		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uarte_nrfx_irq_tx_enable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);

	data->int_driven->disable_tx_irq = false;
	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
}

/** Interrupt driven transfer disabling function */
static void uarte_nrfx_irq_tx_disable(struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	/* TX IRQ will be disabled after current transmission is finished */
	data->int_driven->disable_tx_irq = true;
}

/** Interrupt driven transfer ready function */
static int uarte_nrfx_irq_tx_ready_complete(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	/* ENDTX flag is always on so that ISR is called when we enable TX IRQ.
	 * Because of that we have to explicitly check if ENDTX interrupt is
	 * enabled, otherwise this function would always return true no matter
	 * what would be the source of interrupt.
	 */
	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX) &&
	       nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDTX_MASK);
}

static int uarte_nrfx_irq_rx_ready(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX);
}

/** Interrupt driven receiver enabling function */
static void uarte_nrfx_irq_rx_enable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven receiver disabling function */
static void uarte_nrfx_irq_rx_disable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven error enabling function */
static void uarte_nrfx_irq_err_enable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven error disabling function */
static void uarte_nrfx_irq_err_disable(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven pending status function */
static int uarte_nrfx_irq_is_pending(struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return ((nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_ENDTX_MASK) &&
		 uarte_nrfx_irq_tx_ready_complete(dev))
		||
		(nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_ENDRX_MASK) &&
		 uarte_nrfx_irq_rx_ready(dev)));
}

/** Interrupt driven interrupt update function */
static int uarte_nrfx_irq_update(struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uarte_nrfx_irq_callback_set(struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	data->int_driven->cb = cb;
	data->int_driven->cb_data = cb_data;
}
#endif /* UARTE_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_nrfx_uarte_driver_api = {
	.poll_in		= uarte_nrfx_poll_in,
	.poll_out		= uarte_nrfx_poll_out,
	.err_check		= uarte_nrfx_err_check,
	.configure              = uarte_nrfx_configure,
	.config_get             = uarte_nrfx_config_get,
#ifdef CONFIG_UART_ASYNC_API
	.callback_set		= uarte_nrfx_callback_set,
	.tx			= uarte_nrfx_tx,
	.tx_abort		= uarte_nrfx_tx_abort,
	.rx_enable		= uarte_nrfx_rx_enable,
	.rx_buf_rsp		= uarte_nrfx_rx_buf_rsp,
	.rx_disable		= uarte_nrfx_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
#ifdef UARTE_INTERRUPT_DRIVEN
	.fifo_fill		= uarte_nrfx_fifo_fill,
	.fifo_read		= uarte_nrfx_fifo_read,
	.irq_tx_enable		= uarte_nrfx_irq_tx_enable,
	.irq_tx_disable		= uarte_nrfx_irq_tx_disable,
	.irq_tx_ready		= uarte_nrfx_irq_tx_ready_complete,
	.irq_rx_enable		= uarte_nrfx_irq_rx_enable,
	.irq_rx_disable		= uarte_nrfx_irq_rx_disable,
	.irq_tx_complete	= uarte_nrfx_irq_tx_ready_complete,
	.irq_rx_ready		= uarte_nrfx_irq_rx_ready,
	.irq_err_enable		= uarte_nrfx_irq_err_enable,
	.irq_err_disable	= uarte_nrfx_irq_err_disable,
	.irq_is_pending		= uarte_nrfx_irq_is_pending,
	.irq_update		= uarte_nrfx_irq_update,
	.irq_callback_set	= uarte_nrfx_irq_callback_set,
#endif /* UARTE_INTERRUPT_DRIVEN */
};

static int uarte_instance_init(struct device *dev,
			       const struct uarte_init_config *config,
			       u8_t interrupts_active)
{
	int err;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);

	nrf_gpio_pin_write(config->pseltxd, 1);
	nrf_gpio_cfg_output(config->pseltxd);

	nrf_gpio_cfg_input(config->pselrxd, NRF_GPIO_PIN_NOPULL);

	nrf_uarte_txrx_pins_set(uarte,
				config->pseltxd,
				config->pselrxd);

	if (config->pselcts != NRF_UARTE_PSEL_DISCONNECTED &&
	    config->pselrts != NRF_UARTE_PSEL_DISCONNECTED) {
		nrf_gpio_pin_write(config->pselrts, 1);
		nrf_gpio_cfg_output(config->pselrts);

		nrf_gpio_cfg_input(config->pselcts, NRF_GPIO_PIN_NOPULL);

		nrf_uarte_hwfc_pins_set(uarte,
					config->pselrts,
					config->pselcts);
	}

	err = uarte_nrfx_configure(dev, &get_dev_data(dev)->uart_config);
	if (err) {
		return err;
	}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	data->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif


#ifdef CONFIG_UART_ASYNC_API
	if (data->async) {
		return uarte_nrfx_init(dev);
	}
#endif
	/* Enable receiver and transmitter */
	nrf_uarte_enable(uarte);

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

	nrf_uarte_rx_buffer_set(uarte, &data->rx_data, 1);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

#ifdef UARTE_INTERRUPT_DRIVEN
	if (interrupts_active) {
		/* Set ENDTX event by requesting fake (zero-length) transfer.
		 * Pointer to RAM variable (data->tx_buffer) is set because
		 * otherwise such operation may result in HardFault or RAM
		 * corruption.
		 */
		nrf_uarte_tx_buffer_set(uarte, data->int_driven->tx_buffer, 0);
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

		/* switch off transmitter to save an energy */
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
	}
#endif
	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void uarte_nrfx_set_power_state(struct device *dev, u32_t new_state)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	u32_t tx_pin = nrf_uarte_tx_pin_get(uarte);
	u32_t rx_pin = nrf_uarte_rx_pin_get(uarte);

	if (new_state == DEVICE_PM_ACTIVE_STATE) {
		if (get_dev_config(dev)->gpio_mgmt) {
			nrf_gpio_pin_write(tx_pin, 1);
			nrf_gpio_cfg_output(tx_pin);
			nrf_gpio_cfg_input(rx_pin, NRF_GPIO_PIN_NOPULL);
		}

		nrf_uarte_enable(uarte);
#ifdef CONFIG_UART_ASYNC_API
		if (get_dev_data(dev)->async) {
			return;
		}
#endif
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
	} else {
		assert(new_state == DEVICE_PM_LOW_POWER_STATE ||
		       new_state == DEVICE_PM_SUSPEND_STATE ||
		       new_state == DEVICE_PM_OFF_STATE);

		/* Disabling UART requires stopping RX, but stop RX event is
		 * only sent after each RX if async UART API is used.
		 */
#ifdef CONFIG_UART_ASYNC_API
		if (get_dev_data(dev)->async) {
			nrf_uarte_disable(uarte);
			if (get_dev_config(dev)->gpio_mgmt) {
				nrf_gpio_cfg_default(tx_pin);
				nrf_gpio_cfg_default(rx_pin);
			}
			return;
		}
#endif
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
		while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXTO)) {
			/* Busy wait for event to register */
		}
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
		nrf_uarte_disable(uarte);
		if (get_dev_config(dev)->gpio_mgmt) {
			nrf_gpio_cfg_default(tx_pin);
			nrf_gpio_cfg_default(rx_pin);
		}
	}
}

static int uarte_nrfx_pm_control(struct device *dev, u32_t ctrl_command,
				 void *context, device_pm_cb cb, void *arg)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		u32_t new_state = *((const u32_t *)context);

		if (new_state != data->pm_state) {
			uarte_nrfx_set_power_state(dev, new_state);
			data->pm_state = new_state;
		}
	} else {
		assert(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((u32_t *)context) = data->pm_state;
	}

	if (cb) {
		cb(dev, 0, context, arg);
	}

	return 0;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

#define UART_NRF_UARTE_DEVICE(idx)					       \
	DEVICE_DECLARE(uart_nrfx_uarte##idx);				       \
	UARTE_INT_DRIVEN(idx);						       \
	UARTE_ASYNC(idx);						       \
	static struct uarte_nrfx_data uarte_##idx##_data = {		       \
		UARTE_CONFIG(idx),					       \
		COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_ASYNC),	       \
			    (.async = &uarte##idx##_async,),		       \
			    ())						       \
		COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN),  \
			    (.int_driven = &uarte##idx##_int_driven,),	       \
			    ())						       \
	};								       \
	static const struct uarte_nrfx_config uarte_##idx##z_config = {	       \
		.uarte_regs = (NRF_UARTE_Type *)			       \
			DT_NORDIC_NRF_UARTE_UART_##idx##_BASE_ADDRESS,	       \
		.rts_cts_pins_set = IS_ENABLED(UARTE_##idx##_CONFIG_RTS_CTS),  \
		.gpio_mgmt = IS_ENABLED(CONFIG_UART_##idx##_GPIO_MANAGEMENT),  \
		COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC),      \
			(.timer = NRFX_TIMER_INSTANCE(			       \
				CONFIG_UART_##idx##_NRF_HW_ASYNC_TIMER),),     \
			())						       \
	};								       \
	static int uarte_##idx##_init(struct device *dev)		       \
	{								       \
		const struct uarte_init_config init_config = {		       \
			.pseltxd = DT_NORDIC_NRF_UARTE_UART_##idx##_TX_PIN,    \
			.pselrxd = DT_NORDIC_NRF_UARTE_UART_##idx##_RX_PIN,    \
			UARTE_NRF_RTS_CTS_PINS(idx),			       \
		};							       \
		COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN),  \
			(IRQ_CONNECT(					       \
				NRFX_IRQ_NUMBER_GET(NRF_UARTE##idx),	       \
				DT_NORDIC_NRF_UARTE_UART_##idx##_IRQ_0_PRIORITY, \
				uarte_nrfx_isr_int,			       \
				DEVICE_GET(uart_nrfx_uarte##idx),	       \
				0);					       \
			irq_enable(DT_NORDIC_NRF_UARTE_UART_##idx##_IRQ_0);), ())\
		COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_ASYNC),	       \
			(IRQ_CONNECT(					       \
				NRFX_IRQ_NUMBER_GET(NRF_UARTE##idx),	       \
				DT_NORDIC_NRF_UARTE_UART_##idx##_IRQ_0_PRIORITY, \
				uarte_nrfx_isr_async,			       \
				DEVICE_GET(uart_nrfx_uarte##idx),	       \
				0);					       \
			irq_enable(DT_NORDIC_NRF_UARTE_UART_##idx##_IRQ_0);), ())\
		return uarte_instance_init(				       \
			dev,						       \
			&init_config,					       \
			IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN));     \
	}								       \
	DEVICE_DEFINE(uart_nrfx_uarte##idx,				       \
		      DT_NORDIC_NRF_UARTE_UART_##idx##_LABEL,		       \
		      uarte_##idx##_init,				       \
		      uarte_nrfx_pm_control,				       \
		      &uarte_##idx##_data,				       \
		      &uarte_##idx##z_config,				       \
		      PRE_KERNEL_1,					       \
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		       \
		      &uart_nrfx_uarte_driver_api)

#define UARTE_CONFIG(idx)						       \
	.uart_config = {						       \
		.baudrate = DT_NORDIC_NRF_UARTE_UART_##idx##_CURRENT_SPEED,    \
		.data_bits = UART_CFG_DATA_BITS_8,			       \
		.stop_bits = UART_CFG_STOP_BITS_1,			       \
		.parity = IS_ENABLED(CONFIG_UART_##idx##_NRF_PARITY_BIT)       \
			  ? UART_CFG_PARITY_EVEN			       \
			  : UART_CFG_PARITY_NONE,			       \
		.flow_ctrl = IS_ENABLED(CONFIG_UART_##idx##_NRF_FLOW_CONTROL)  \
			     ? UART_CFG_FLOW_CTRL_RTS_CTS		       \
			     : UART_CFG_FLOW_CTRL_NONE,			       \
	}

#define UARTE_NRF_RTS_CTS_PINS(idx)					       \
	.pselcts = COND_CODE_1(IS_ENABLED(UARTE_##idx##_CONFIG_RTS_CTS),       \
			       (DT_NORDIC_NRF_UARTE_UART_##idx##_CTS_PIN),     \
			       (NRF_UARTE_PSEL_DISCONNECTED)),		       \
	.pselrts = COND_CODE_1(IS_ENABLED(UARTE_##idx##_CONFIG_RTS_CTS),       \
			       (DT_NORDIC_NRF_UARTE_UART_##idx##_RTS_PIN),     \
			       (NRF_UARTE_PSEL_DISCONNECTED))

#define UARTE_ASYNC(idx)						       \
	COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_ASYNC),		       \
		(struct uarte_async_cb uarte##idx##_async = {		       \
		COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC),      \
			(.hw_rx_counting = true),			       \
			(.hw_rx_counting = false)),			       \
		}), ())

#define UARTE_INT_DRIVEN(idx)						       \
	COND_CODE_1(IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN),	       \
		(static u8_t uarte##idx##_tx_buffer[\
			MIN(CONFIG_UART_##idx##_NRF_TX_BUFFER_SIZE,	       \
			    BIT_MASK(UARTE##idx##_EASYDMA_MAXCNT_SIZE))];      \
		 static struct uarte_nrfx_int_driven			       \
			uarte##idx##_int_driven = {			       \
				.tx_buffer = uarte##idx##_tx_buffer,	       \
				.tx_buff_size = sizeof(uarte##idx##_tx_buffer),\
			};),						       \
		())

#ifdef CONFIG_UART_0_NRF_UARTE
	#if defined(DT_NORDIC_NRF_UARTE_UART_0_RTS_PIN) && \
	    defined(DT_NORDIC_NRF_UARTE_UART_0_CTS_PIN)
		#define UARTE_0_CONFIG_RTS_CTS 1
	#endif

	UART_NRF_UARTE_DEVICE(0);
#endif /* CONFIG_UART_0_NRF_UARTE */

#ifdef CONFIG_UART_1_NRF_UARTE
	#if defined(DT_NORDIC_NRF_UARTE_UART_1_RTS_PIN) && \
	    defined(DT_NORDIC_NRF_UARTE_UART_1_CTS_PIN)
		#define UARTE_1_CONFIG_RTS_CTS 1
	#endif

	UART_NRF_UARTE_DEVICE(1);
#endif /* CONFIG_UART_1_NRF_UARTE */

#ifdef CONFIG_UART_2_NRF_UARTE
	#if defined(DT_NORDIC_NRF_UARTE_UART_2_RTS_PIN) && \
	    defined(DT_NORDIC_NRF_UARTE_UART_2_CTS_PIN)
		#define UARTE_2_CONFIG_RTS_CTS 1
	#endif

	UART_NRF_UARTE_DEVICE(2);
#endif /* CONFIG_UART_2_NRF_UARTE */

#ifdef CONFIG_UART_3_NRF_UARTE
	#if defined(DT_NORDIC_NRF_UARTE_UART_3_RTS_PIN) && \
	    defined(DT_NORDIC_NRF_UARTE_UART_3_CTS_PIN)
		#define UARTE_3_CONFIG_RTS_CTS 1
	#endif

	UART_NRF_UARTE_DEVICE(3);
#endif /* CONFIG_UART_3_NRF_UARTE */
