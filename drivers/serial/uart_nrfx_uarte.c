/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
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
#include <helpers/nrfx_gppi.h>
LOG_MODULE_REGISTER(uart_nrfx_uarte, LOG_LEVEL_ERR);

/* Generalize PPI or DPPI channel management */
#if defined(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#define gppi_channel_t nrf_ppi_channel_t
#define gppi_channel_alloc nrfx_ppi_channel_alloc
#define gppi_channel_enable nrfx_ppi_channel_enable
#elif defined(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#define gppi_channel_t uint8_t
#define gppi_channel_alloc nrfx_dppi_channel_alloc
#define gppi_channel_enable nrfx_dppi_channel_enable
#else
#error "No PPI or DPPI"
#endif


#if	(defined(CONFIG_UART_0_NRF_UARTE) &&         \
	 defined(CONFIG_UART_0_INTERRUPT_DRIVEN)) || \
	(defined(CONFIG_UART_1_NRF_UARTE) &&         \
	 defined(CONFIG_UART_1_INTERRUPT_DRIVEN)) || \
	(defined(CONFIG_UART_2_NRF_UARTE) &&         \
	 defined(CONFIG_UART_2_INTERRUPT_DRIVEN)) || \
	(defined(CONFIG_UART_3_NRF_UARTE) &&         \
	 defined(CONFIG_UART_3_INTERRUPT_DRIVEN))
	#define UARTE_INTERRUPT_DRIVEN	1
#endif

#if	(defined(CONFIG_UART_0_NRF_UARTE) && !defined(CONFIG_UART_0_ASYNC)) || \
	(defined(CONFIG_UART_1_NRF_UARTE) && !defined(CONFIG_UART_1_ASYNC)) || \
	(defined(CONFIG_UART_2_NRF_UARTE) && !defined(CONFIG_UART_2_ASYNC)) || \
	(defined(CONFIG_UART_3_NRF_UARTE) && !defined(CONFIG_UART_3_ASYNC))
#define UARTE_ANY_NONE_ASYNC 1
#endif

/*
 * RX timeout is divided into time slabs, this define tells how many divisions
 * should be made. More divisions - higher timeout accuracy and processor usage.
 */
#define RX_TIMEOUT_DIV 5

/* Size of hardware fifo in RX path. */
#define UARTE_HW_RX_FIFO_SIZE 5

#ifdef CONFIG_UART_ASYNC_API
struct uarte_async_cb {
	uart_callback_t user_callback;
	void *user_data;

	const uint8_t *tx_buf;
	volatile size_t tx_size;
	uint8_t *pend_tx_buf;

	struct k_timer tx_timeout_timer;

	uint8_t *rx_buf;
	size_t rx_buf_len;
	size_t rx_offset;
	uint8_t *rx_next_buf;
	size_t rx_next_buf_len;
	uint32_t rx_total_byte_cnt; /* Total number of bytes received */
	uint32_t rx_total_user_byte_cnt; /* Total number of bytes passed to user */
	int32_t rx_timeout; /* Timeout set by user */
	int32_t rx_timeout_slab; /* rx_timeout divided by RX_TIMEOUT_DIV */
	int32_t rx_timeout_left; /* Current time left until user callback */
	struct k_timer rx_timeout_timer;
	union {
		gppi_channel_t ppi;
		uint32_t cnt;
	} rx_cnt;
	volatile int tx_amount;

	atomic_t low_power_mask;
	uint8_t rx_flush_buffer[UARTE_HW_RX_FIFO_SIZE];
	uint8_t rx_flush_cnt;
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
	uint8_t *tx_buffer;
	uint16_t tx_buff_size;
	volatile bool disable_tx_irq;
#ifdef CONFIG_PM_DEVICE
	bool rx_irq_enabled;
#endif
	atomic_t fifo_fill_lock;
};
#endif

/* Device data structure */
struct uarte_nrfx_data {
	const struct device *dev;
	struct uart_config uart_config;
#ifdef UARTE_INTERRUPT_DRIVEN
	struct uarte_nrfx_int_driven *int_driven;
#endif
#ifdef CONFIG_UART_ASYNC_API
	struct uarte_async_cb *async;
#endif
	atomic_val_t poll_out_lock;
	uint8_t char_out;
	uint8_t rx_data;
	gppi_channel_t ppi_ch_endtx;
};

#define UARTE_LOW_POWER_TX BIT(0)
#define UARTE_LOW_POWER_RX BIT(1)

/* Flag indicating that CTS pin is used. */
#define UARTE_CFG_FLAG_CTS_PIN_SET BIT(0)

/* Flag indicating that RTS pin is used. */
#define UARTE_CFG_FLAG_RTS_PIN_SET BIT(1)

/* If enabled, pins are managed when going to low power mode. */
#define UARTE_CFG_FLAG_GPIO_MGMT   BIT(2)

/* If enabled then ENDTX is PPI'ed to TXSTOP */
#define UARTE_CFG_FLAG_PPI_ENDTX   BIT(3)

/* If enabled then UARTE peripheral is disabled when not used. This allows
 * to achieve lowest power consumption in idle.
 */
#define UARTE_CFG_FLAG_LOW_POWER   BIT(4)

#define IS_CTS_PIN_SET(flags) (flags & UARTE_CFG_FLAG_CTS_PIN_SET)
#define IS_RTS_PIN_SET(flags) (flags & UARTE_CFG_FLAG_RTS_PIN_SET)
#define IS_HWFC_PINS_USED(flags) \
	(IS_CTS_PIN_SET(flags) | IS_RTS_PIN_SET(flags))


/**
 * @brief Structure for UARTE configuration.
 */
struct uarte_nrfx_config {
	NRF_UARTE_Type *uarte_regs; /* Instance address */
	uint32_t flags;
	uint32_t pseltxd; /* PSEL.TXD register value */
	uint32_t pselrxd; /* PSEL.RXD register value */
	uint32_t pselcts; /* PSEL.CTS register value */
	uint32_t pselrts; /* PSEL.RTS register value */
	nrf_gpio_pin_pull_t rxd_pull; /* RXD pin pull configuration */
	nrf_gpio_pin_pull_t cts_pull; /* CTS pin pull configuration */
#ifdef CONFIG_UART_ASYNC_API
	nrfx_timer_t timer;
#endif
};

static inline struct uarte_nrfx_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct uarte_nrfx_config *get_dev_config(const struct device *dev)
{
	return dev->config;
}

static inline NRF_UARTE_Type *get_uarte_instance(const struct device *dev)
{
	const struct uarte_nrfx_config *config = get_dev_config(dev);

	return config->uarte_regs;
}

static void endtx_isr(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	int key = irq_lock();

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
	}

	irq_unlock(key);

}

#ifdef UARTE_ANY_NONE_ASYNC
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
	const struct device *dev = arg;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	/* If interrupt driven and asynchronous APIs are disabled then UART
	 * interrupt is still called to stop TX. Unless it is done using PPI.
	 */
	if (nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDTX_MASK) &&
		nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)) {
		endtx_isr(dev);
	}

	if (get_dev_config(dev)->flags & UARTE_CFG_FLAG_LOW_POWER) {
		int key = irq_lock();

		if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
			nrf_uarte_disable(uarte);
		}

#ifdef UARTE_INTERRUPT_DRIVEN
		struct uarte_nrfx_data *data = get_dev_data(dev);

		if (!data->int_driven || data->int_driven->fifo_fill_lock == 0)
#endif
		{
			nrf_uarte_int_disable(uarte,
					      NRF_UARTE_INT_TXSTOPPED_MASK);
		}

		irq_unlock(key);
	}

#ifdef UARTE_INTERRUPT_DRIVEN
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (!data->int_driven) {
		return;
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
		data->int_driven->fifo_fill_lock = 0;
		if (data->int_driven->disable_tx_irq) {
			nrf_uarte_int_disable(uarte,
					      NRF_UARTE_INT_TXSTOPPED_MASK);
			data->int_driven->disable_tx_irq = false;
			return;
		}

	}


	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ERROR)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ERROR);
	}

	if (data->int_driven->cb) {
		data->int_driven->cb(dev, data->int_driven->cb_data);
	}
#endif /* UARTE_INTERRUPT_DRIVEN */
}
#endif /* UARTE_ANY_NONE_ASYNC */

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
static int baudrate_set(const struct device *dev, uint32_t baudrate)
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

static int uarte_nrfx_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	nrf_uarte_config_t uarte_cfg;

#if defined(UARTE_CONFIG_STOP_Msk)
	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uarte_cfg.stop = NRF_UARTE_STOP_ONE;
		break;
	case UART_CFG_STOP_BITS_2:
		uarte_cfg.stop = NRF_UARTE_STOP_TWO;
		break;
	default:
		return -ENOTSUP;
	}
#else
	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOTSUP;
	}
#endif

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uarte_cfg.hwfc = NRF_UARTE_HWFC_DISABLED;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		if (IS_HWFC_PINS_USED(get_dev_config(dev)->flags)) {
			uarte_cfg.hwfc = NRF_UARTE_HWFC_ENABLED;
		} else {
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

#if defined(UARTE_CONFIG_PARITYTYPE_Msk)
	uarte_cfg.paritytype = NRF_UARTE_PARITYTYPE_EVEN;
#endif
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uarte_cfg.parity = NRF_UARTE_PARITY_EXCLUDED;
		break;
	case UART_CFG_PARITY_EVEN:
		uarte_cfg.parity = NRF_UARTE_PARITY_INCLUDED;
		break;
#if defined(UARTE_CONFIG_PARITYTYPE_Msk)
	case UART_CFG_PARITY_ODD:
		uarte_cfg.parity = NRF_UARTE_PARITY_INCLUDED;
		uarte_cfg.paritytype = NRF_UARTE_PARITYTYPE_ODD;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (baudrate_set(dev, cfg->baudrate) != 0) {
		return -ENOTSUP;
	}

	nrf_uarte_configure(get_uarte_instance(dev), &uarte_cfg);

	get_dev_data(dev)->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uarte_nrfx_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	*cfg = get_dev_data(dev)->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */


static int uarte_nrfx_err_check(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	/* register bitfields maps to the defines in uart.h */
	return nrf_uarte_errorsrc_get_and_clear(uarte);
}

/* Function returns true if new transfer can be started. Since TXSTOPPED
 * (and ENDTX) is cleared before triggering new transfer, TX is ready for new
 * transfer if any event is set.
 */
static bool is_tx_ready(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	bool ppi_endtx = get_dev_config(dev)->flags & UARTE_CFG_FLAG_PPI_ENDTX;

	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED) ||
		(!ppi_endtx ?
		       nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX) : 0);
}

/* Wait until the transmitter is in the idle state. When this function returns,
 * IRQ's are locked with the returned key.
 */
static int wait_tx_ready(const struct device *dev)
{
	int key;

	do {
		/* wait arbitrary time before back off. */
		bool res;

		NRFX_WAIT_FOR(is_tx_ready(dev), 100, 1, res);

		if (res) {
			key = irq_lock();
			if (is_tx_ready(dev)) {
				break;
			}

			irq_unlock(key);
		}
		if (IS_ENABLED(CONFIG_MULTITHREADING)) {
			k_msleep(1);
		}
	} while (1);

	return key;
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
#endif /* CONFIG_UART_ASYNC_API */

static void uarte_enable(const struct device *dev, uint32_t mask)
{
#ifdef CONFIG_UART_ASYNC_API
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (data->async) {
		bool disabled = data->async->low_power_mask == 0;

		data->async->low_power_mask |= mask;
		if (hw_rx_counting_enabled(data) && disabled) {
			const nrfx_timer_t *timer = &get_dev_config(dev)->timer;

			nrfx_timer_enable(timer);

			for (int i = 0; i < data->async->rx_flush_cnt; i++) {
				nrfx_timer_increment(timer);
			}
		}
	}
#endif
	nrf_uarte_enable(get_uarte_instance(dev));
}

/* At this point we should have irq locked and any previous transfer completed.
 * Transfer can be started, no need to wait for completion.
 */
static void tx_start(const struct device *dev, const uint8_t *buf, size_t len)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return;
	}
#endif
	nrf_uarte_tx_buffer_set(uarte, buf, len);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTOPPED);

	if (get_dev_config(dev)->flags & UARTE_CFG_FLAG_LOW_POWER) {
		uarte_enable(dev, UARTE_LOW_POWER_TX);
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	}

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);
}

#if defined(CONFIG_UART_ASYNC_API) || defined(CONFIG_PM_DEVICE)
static void uart_disable(const struct device *dev)
{
#ifdef CONFIG_UART_ASYNC_API
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (data->async && hw_rx_counting_enabled(data)) {
		nrfx_timer_disable(&get_dev_config(dev)->timer);
		/* Timer/counter value is reset when disabled. */
		data->async->rx_total_byte_cnt = 0;
		data->async->rx_total_user_byte_cnt = 0;
	}
#endif

	// Workarround (or CPU issue) for UART0, do not do 'NRF_UARTE0->ENABLE = 0'
	if(get_uarte_instance(dev) != NRF_UARTE0 && get_uarte_instance(dev) != NRF_UARTE1)
		nrf_uarte_disable(get_uarte_instance(dev));
}
#endif

#ifdef CONFIG_UART_ASYNC_API

static void timer_handler(nrf_timer_event_t event_type, void *p_context) { }
static void rx_timeout(struct k_timer *timer);
static void tx_timeout(struct k_timer *timer);

static int uarte_nrfx_rx_counting_init(const struct device *dev)
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

static int uarte_nrfx_init(const struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	int ret = uarte_nrfx_rx_counting_init(dev);

	if (ret != 0) {
		return ret;
	}

	data->async->low_power_mask = UARTE_LOW_POWER_TX;
	nrf_uarte_int_enable(uarte,
			     NRF_UARTE_INT_ENDRX_MASK |
			     NRF_UARTE_INT_RXSTARTED_MASK |
			     NRF_UARTE_INT_ERROR_MASK |
			     NRF_UARTE_INT_RXTO_MASK);
	nrf_uarte_enable(uarte);

	/**
	 * Stop any currently running RX operations. This can occur when a
	 * bootloader sets up the UART hardware and does not clean it up
	 * before jumping to the next application.
	 */
	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED)) {
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
		while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXTO) &&
		       !nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ERROR)) {
			/* Busy wait for event to register */
		}
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
	}

	k_timer_init(&data->async->rx_timeout_timer, rx_timeout, NULL);
	k_timer_user_data_set(&data->async->rx_timeout_timer, data);
	k_timer_init(&data->async->tx_timeout_timer, tx_timeout, NULL);
	k_timer_user_data_set(&data->async->tx_timeout_timer, data);

	return 0;
}

static int uarte_nrfx_tx(const struct device *dev, const uint8_t *buf,
			 size_t len,
			 int32_t timeout)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (!nrfx_is_in_ram(buf)) {
		return -ENOTSUP;
	}

	int key = irq_lock();

	if (data->async->tx_size) {
		irq_unlock(key);
		return -EBUSY;
	}

	data->async->tx_size = len;
	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);

	if (!is_tx_ready(dev)) {
		/* Active poll out, postpone until it is completed. */
		data->async->pend_tx_buf = (uint8_t *)buf;
	} else {
		data->async->tx_buf = buf;
		data->async->tx_amount = -1;
		tx_start(dev, buf, len);
	}

	irq_unlock(key);

	if (data->uart_config.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS
	    && timeout != SYS_FOREVER_MS) {
		k_timer_start(&data->async->tx_timeout_timer, K_MSEC(timeout),
			      K_NO_WAIT);
	}
	return 0;
}

static int uarte_nrfx_tx_abort(const struct device *dev)
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

static void user_callback(const struct device *dev, struct uart_event *evt)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (data->async->user_callback) {
		data->async->user_callback(dev, evt, data->async->user_data);
	}
}

static void notify_uart_rx_rdy(const struct device *dev, size_t len)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	struct uart_event evt = {
		.type = UART_RX_RDY,
		.data.rx.buf = data->async->rx_buf,
		.data.rx.len = len,
		.data.rx.offset = data->async->rx_offset
	};

	user_callback(dev, &evt);
}

static void notify_rx_buf_release(const struct device *dev,
				  uint8_t **buf, bool clear)
{
	if (*buf) {
		struct uart_event evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = *buf,
		};

		user_callback(dev, &evt);
		if (clear) {
			*buf = NULL;
		}
	}
}

static void notify_rx_disable(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_DISABLED,
	};

	user_callback(dev, (struct uart_event *)&evt);
}

static int uarte_nrfx_rx_enable(const struct device *dev, uint8_t *buf,
				size_t len,
				int32_t timeout)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (nrf_uarte_rx_pin_get(uarte) == NRF_UARTE_PSEL_DISCONNECTED) {
		__ASSERT(false, "TX only UARTE instance");
		return -ENOTSUP;
	}

	data->async->rx_timeout = timeout;
	data->async->rx_timeout_slab =
		MAX(timeout / RX_TIMEOUT_DIV,
		    NRFX_CEIL_DIV(1000, CONFIG_SYS_CLOCK_TICKS_PER_SEC));

	data->async->rx_buf = buf;
	data->async->rx_buf_len = len;
	data->async->rx_offset = 0;
	data->async->rx_next_buf = NULL;
	data->async->rx_next_buf_len = 0;

	if (get_dev_config(dev)->flags & UARTE_CFG_FLAG_LOW_POWER) {
		if (data->async->rx_flush_cnt) {
			int cpy_len = MIN(len, data->async->rx_flush_cnt);

			memcpy(buf, data->async->rx_flush_buffer, cpy_len);
			buf += cpy_len;
			len -= cpy_len;

			/* If flush content filled whole new buffer complete the
			 * request and indicate rx being disabled.
			 */
			if (!len) {
				data->async->rx_flush_cnt -= cpy_len;
				notify_uart_rx_rdy(dev, cpy_len);
				notify_rx_buf_release(dev, &data->async->rx_buf,
						      true);
				notify_rx_disable(dev);
				return 0;
			}
		}
	}

	nrf_uarte_rx_buffer_set(uarte, buf, len);

	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);

	data->async->rx_enabled = true;
	if (get_dev_config(dev)->flags & UARTE_CFG_FLAG_LOW_POWER) {
		int key = irq_lock();

		uarte_enable(dev, UARTE_LOW_POWER_RX);
		irq_unlock(key);
	}

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);

	return 0;
}

static int uarte_nrfx_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				 size_t len)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	int err;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	int key = irq_lock();

	if (data->async->rx_buf == NULL) {
		err = -EACCES;
	} else if (data->async->rx_next_buf == NULL) {
		data->async->rx_next_buf = buf;
		data->async->rx_next_buf_len = len;
		nrf_uarte_rx_buffer_set(uarte, buf, len);
		nrf_uarte_shorts_enable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
		err = 0;
	} else {
		err = -EBUSY;
	}

	irq_unlock(key);

	return err;
}

static int uarte_nrfx_callback_set(const struct device *dev,
				   uart_callback_t callback,
				   void *user_data)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	if (!data->async) {
		return -ENOTSUP;
	}

	data->async->user_callback = callback;
	data->async->user_data = user_data;

	return 0;
}

static int uarte_nrfx_rx_disable(const struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	if (data->async->rx_buf == NULL) {
		return -EFAULT;
	}
	if (data->async->rx_next_buf != NULL) {
		nrf_uarte_shorts_disable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
	}

	k_timer_stop(&data->async->rx_timeout_timer);
	data->async->rx_enabled = false;

	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);

	return 0;
}

static void tx_timeout(struct k_timer *timer)
{
	struct uarte_nrfx_data *data = k_timer_user_data_get(timer);
	(void) uarte_nrfx_tx_abort(data->dev);
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
	struct uarte_nrfx_data *data = k_timer_user_data_get(timer);
	const struct device *dev = data->dev;
	const struct uarte_nrfx_config *cfg = get_dev_config(dev);
	uint32_t read;

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

	/* Check if there is data that was not sent to user yet
	 * Note though that 'len' is a count of data bytes received, but not
	 * necessarily the amount available in the current buffer
	 */
	int32_t len = data->async->rx_total_byte_cnt
		    - data->async->rx_total_user_byte_cnt;

	if (!hw_rx_counting_enabled(data) &&
	    (len < 0)) {
		/* Prevent too low value of rx_cnt.cnt which may occur due to
		 * latencies in handling of the RXRDY interrupt.
		 * At this point, the number of received bytes is at least
		 * equal to what was reported to the user.
		 */
		data->async->rx_cnt.cnt = data->async->rx_total_user_byte_cnt;
		len = 0;
	}

	/* Check for current buffer being full.
	 * if the UART receives characters before the the ENDRX is handled
	 * and the 'next' buffer is set up, then the SHORT between ENDRX and
	 * STARTRX will mean that data will be going into to the 'next' buffer
	 * until the ENDRX event gets a chance to be handled.
	 */
	bool clipped = false;

	if (len + data->async->rx_offset > data->async->rx_buf_len) {
		len = data->async->rx_buf_len - data->async->rx_offset;
		clipped = true;
	}

	if (len > 0) {
		if (clipped ||
			(data->async->rx_timeout_left
				< data->async->rx_timeout_slab)) {
			/* rx_timeout ms elapsed since last receiving */
			notify_uart_rx_rdy(dev, len);
			data->async->rx_offset += len;
			data->async->rx_total_user_byte_cnt += len;
		} else {
			data->async->rx_timeout_left -=
				data->async->rx_timeout_slab;
		}

		/* If theres nothing left to report until the buffers are
		 * switched then the timer can be stopped
		 */
		if (clipped) {
			k_timer_stop(&data->async->rx_timeout_timer);
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

static void error_isr(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	uint32_t err = nrf_uarte_errorsrc_get_and_clear(uarte);
	struct uart_event evt = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = UARTE_ERROR_FROM_MASK(err),
	};
	user_callback(dev, &evt);
	(void) uarte_nrfx_rx_disable(dev);
}

static void rxstarted_isr(const struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};
	user_callback(dev, &evt);
	if (data->async->rx_timeout != SYS_FOREVER_MS) {
		data->async->rx_timeout_left = data->async->rx_timeout;
		k_timer_start(&data->async->rx_timeout_timer,
			      K_MSEC(data->async->rx_timeout_slab),
			      K_MSEC(data->async->rx_timeout_slab));
	}
}

static void endrx_isr(const struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	data->async->is_in_irq = true;

	/* ensure rx timer is stopped - it will be restarted in RXSTARTED
	 * handler if needed
	 */
	k_timer_stop(&data->async->rx_timeout_timer);

	/* this is the amount that the EasyDMA controller has copied into the
	 * buffer
	 */
	const int rx_amount = nrf_uarte_rx_amount_get(uarte) +
				data->async->rx_flush_cnt;

	data->async->rx_flush_cnt = 0;

	/* The 'rx_offset' can be bigger than 'rx_amount', so it the length
	 * of data we report back the the user may need to be clipped.
	 * This can happen because the 'rx_offset' count derives from RXRDY
	 * events, which can occur already for the next buffer before we are
	 * here to handle this buffer. (The next buffer is now already active
	 * because of the ENDRX_STARTRX shortcut)
	 */
	int rx_len = rx_amount - data->async->rx_offset;

	if (rx_len < 0) {
		rx_len = 0;
	}

	data->async->rx_total_user_byte_cnt += rx_len;

	/* Only send the RX_RDY event if there is something to send */
	if (rx_len > 0) {
		notify_uart_rx_rdy(dev, rx_len);
	}

	if (!data->async->rx_enabled) {
		data->async->is_in_irq = false;
		return;
	}

	notify_rx_buf_release(dev, &data->async->rx_buf, false);

	/* If there is a next buffer, then STARTRX will have already been
	 * invoked by the short (the next buffer will be filling up already)
	 * and here we just do the swap of which buffer the driver is following,
	 * the next rx_timeout() will update the rx_offset.
	 */
	int key = irq_lock();

	if (data->async->rx_next_buf) {
		data->async->rx_buf = data->async->rx_next_buf;
		data->async->rx_buf_len = data->async->rx_next_buf_len;
		data->async->rx_next_buf = NULL;
		data->async->rx_next_buf_len = 0;

		data->async->rx_offset = 0;
		/* Check is based on assumption that ISR handler handles
		 * ENDRX before RXSTARTED so if short was set on time, RXSTARTED
		 * event will be set.
		 */
		if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED)) {
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
		}
		/* Remove the short until the subsequent next buffer is setup */
		nrf_uarte_shorts_disable(uarte, NRF_UARTE_SHORT_ENDRX_STARTRX);
	} else {
		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
	}

	irq_unlock(key);

	data->async->is_in_irq = false;
}

/* Function for flushing internal RX fifo. Function can be called in case
 * flushed data is discarded or when data is valid and needs to be retrieved.
 *
 * However, UARTE does not update RXAMOUNT register if fifo is empty. Old value
 * remains. In certain cases it makes it impossible to distinguish between
 * case when fifo was empty and not. Function is trying to minimize chances of
 * error with following measures:
 * - RXAMOUNT is read before flushing and compared against value after flushing
 *   if they differ it indicates that data was flushed
 * - user buffer is dirtied and if RXAMOUNT did not changed it is checked if
 *   it is still dirty. If not then it indicates that data was flushed
 *
 * In other cases function indicates that fifo was empty. It means that if
 * number of bytes in the fifo equal last rx transfer length and data is equal
 * to dirty marker it will be discarded.
 *
 * @param dev Device.
 * @param buf Buffer for flushed data, null indicates that flushed data can be
 *	      dropped.
 * @param len Buffer size, not used if @p buf is null.
 *
 * @return number of bytes flushed from the fifo.
 */
static uint8_t rx_flush(const struct device *dev, uint8_t *buf, uint32_t len)
{
	/* Flushing RX fifo requires buffer bigger than 4 bytes to empty fifo*/
	static const uint8_t dirty;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	uint32_t prev_rx_amount = nrf_uarte_rx_amount_get(uarte);
	uint8_t tmp_buf[UARTE_HW_RX_FIFO_SIZE];
	uint8_t *flush_buf = buf ? buf : tmp_buf;
	size_t flush_len = buf ? len : sizeof(tmp_buf);

	if (buf) {
		memset(buf, dirty, len);
		flush_buf = buf;
		flush_len = len;
	} else {
		flush_buf = tmp_buf;
		flush_len = sizeof(tmp_buf);
	}

	nrf_uarte_rx_buffer_set(uarte, flush_buf, flush_len);
	/* Final part of handling RXTO event is in ENDRX interrupt
	 * handler. ENDRX is generated as a result of FLUSHRX task.
	 */
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_FLUSHRX);
	while (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* empty */
	}
	nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

	if (!buf) {
		return nrf_uarte_rx_amount_get(uarte);
	}

	uint32_t rx_amount = nrf_uarte_rx_amount_get(uarte);

	if (rx_amount != prev_rx_amount) {
		return rx_amount;
	}

	for (int i = 0; i < flush_len; i++) {
		if (buf[i] != dirty) {
			return rx_amount;
		}
	}

	return 0;
}

static void async_uart_release(const struct device *dev, uint32_t dir_mask)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	int key = irq_lock();

	data->async->low_power_mask &= ~dir_mask;
	if (!data->async->low_power_mask) {
		if (dir_mask == UARTE_LOW_POWER_RX) {
			data->async->rx_flush_cnt =
				rx_flush(dev, data->async->rx_flush_buffer,
					 sizeof(data->async->rx_flush_buffer));
		}

		uart_disable(dev);
	}

	irq_unlock(key);
}

/* This handler is called when the receiver is stopped. If rx was aborted
 * data from fifo is flushed.
 */
static void rxto_isr(const struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	notify_rx_buf_release(dev, &data->async->rx_buf, true);
	notify_rx_buf_release(dev, &data->async->rx_next_buf, true);

	if (!data->async->rx_enabled) {
		(void)rx_flush(dev, NULL, 0);
	}

	if (get_dev_config(dev)->flags & UARTE_CFG_FLAG_LOW_POWER) {
		async_uart_release(dev, UARTE_LOW_POWER_RX);
	}

	notify_rx_disable(dev);
}

static void txstopped_isr(const struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	int key;

	if (get_dev_config(dev)->flags & UARTE_CFG_FLAG_LOW_POWER) {
		nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
		async_uart_release(dev, UARTE_LOW_POWER_TX);

		if (!data->async->tx_size) {
			return;
		}
	}

	if (!data->async->tx_buf) {
		/* If there is a pending tx request, it means that uart_tx()
		 * was called when there was ongoing uart_poll_out. Handling
		 * TXSTOPPED interrupt means that uart_poll_out has completed.
		 */
		if (data->async->pend_tx_buf) {
			key = irq_lock();

			if (nrf_uarte_event_check(uarte,
						NRF_UARTE_EVENT_TXSTOPPED)) {
				data->async->tx_buf = data->async->pend_tx_buf;
				data->async->pend_tx_buf = NULL;
				data->async->tx_amount = -1;
				tx_start(dev, data->async->tx_buf,
					 data->async->tx_size);
			}

			irq_unlock(key);
		}
		return;
	}

	k_timer_stop(&data->async->tx_timeout_timer);

	key = irq_lock();
	size_t amount = (data->async->tx_amount >= 0) ?
			data->async->tx_amount : nrf_uarte_tx_amount_get(uarte);

	irq_unlock(key);

	struct uart_event evt = {
		.data.tx.buf = data->async->tx_buf,
		.data.tx.len = amount,
	};
	if (amount == data->async->tx_size) {
		evt.type = UART_TX_DONE;
	} else {
		evt.type = UART_TX_ABORTED;
	}

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	data->async->tx_buf = NULL;
	data->async->tx_size = 0;

	user_callback(dev, &evt);
}

static void uarte_nrfx_isr_async(const struct device *dev)
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

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)
	    && nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDRX_MASK)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
		endrx_isr(dev);
	}

	/* RXSTARTED must be handled after ENDRX because it starts the RX timeout
	 * and if order is swapped then ENDRX will stop this timeout.
	 * Skip if ENDRX is set when RXSTARTED is set. It means that
	 * ENDRX occurred after check for ENDRX in isr which may happen when
	 * UARTE interrupt got preempted. Events are not cleared
	 * and isr will be called again. ENDRX will be handled first.
	 */
	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED) &&
	    !nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
		rxstarted_isr(dev);
	}

	/* RXTO must be handled after ENDRX which should notify the buffer.
	 * Skip if ENDRX is set when RXTO is set. It means that
	 * ENDRX occurred after check for ENDRX in isr which may happen when
	 * UARTE interrupt got preempted. Events are not cleared
	 * and isr will be called again. ENDRX will be handled first.
	 */
	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXTO) &&
	    !nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
		rxto_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDTX)
	    && nrf_uarte_int_enable_check(uarte, NRF_UARTE_INT_ENDTX_MASK)) {
		endtx_isr(dev);
	}

	if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)
	    && nrf_uarte_int_enable_check(uarte,
					  NRF_UARTE_INT_TXSTOPPED_MASK)) {
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
static int uarte_nrfx_poll_in(const struct device *dev, unsigned char *c)
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
static void uarte_nrfx_poll_out(const struct device *dev, unsigned char c)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	bool isr_mode = k_is_in_isr() || k_is_pre_kernel();
	int key;

	if (isr_mode) {
		while (1) {
			key = irq_lock();
			if (is_tx_ready(dev)) {
#if CONFIG_UART_ASYNC_API
				if (data->async && data->async->tx_size &&
					data->async->tx_amount < 0) {
					data->async->tx_amount =
						nrf_uarte_tx_amount_get(
						      get_uarte_instance(dev));
				}
#endif
				break;
			}

			irq_unlock(key);
		}
	} else {
		key = wait_tx_ready(dev);
	}

	data->char_out = c;
	tx_start(dev, &data->char_out, 1);

	irq_unlock(key);
}


#ifdef UARTE_INTERRUPT_DRIVEN
/** Interrupt driven FIFO fill function */
static int uarte_nrfx_fifo_fill(const struct device *dev,
				const uint8_t *tx_data,
				int len)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);

	len = MIN(len, data->int_driven->tx_buff_size);
	if (!atomic_cas(&data->int_driven->fifo_fill_lock, 0, 1)) {
		return 0;
	}

	/* Copy data to RAM buffer for EasyDMA transfer */
	for (int i = 0; i < len; i++) {
		data->int_driven->tx_buffer[i] = tx_data[i];
	}

	int key = irq_lock();

	if (!is_tx_ready(dev)) {
		data->int_driven->fifo_fill_lock = 0;
		len = 0;
	} else {
		tx_start(dev, data->int_driven->tx_buffer, len);
	}

	irq_unlock(key);

	return len;
}

/** Interrupt driven FIFO read function */
static int uarte_nrfx_fifo_read(const struct device *dev,
				uint8_t *rx_data,
				const int size)
{
	int num_rx = 0;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	const struct uarte_nrfx_data *data = get_dev_data(dev);

	if (size > 0 && nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX)) {
		/* Clear the interrupt */
		nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

		/* Receive a character */
		rx_data[num_rx++] = (uint8_t)data->rx_data;

		nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
	}

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uarte_nrfx_irq_tx_enable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);
	int key = irq_lock();

	data->int_driven->disable_tx_irq = false;
	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);

	irq_unlock(key);
}

/** Interrupt driven transfer disabling function */
static void uarte_nrfx_irq_tx_disable(const struct device *dev)
{
	struct uarte_nrfx_data *data = get_dev_data(dev);
	/* TX IRQ will be disabled after current transmission is finished */
	data->int_driven->disable_tx_irq = true;
}

/** Interrupt driven transfer ready function */
static int uarte_nrfx_irq_tx_ready_complete(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);

	/* ENDTX flag is always on so that ISR is called when we enable TX IRQ.
	 * Because of that we have to explicitly check if ENDTX interrupt is
	 * enabled, otherwise this function would always return true no matter
	 * what would be the source of interrupt.
	 */
	bool ready = !data->int_driven->disable_tx_irq &&
		     nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED) &&
		     nrf_uarte_int_enable_check(uarte,
						NRF_UARTE_INT_TXSTOPPED_MASK);

	if (ready) {
		data->int_driven->fifo_fill_lock = 0;
	}

	return ready;
}

static int uarte_nrfx_irq_rx_ready(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX);
}

/** Interrupt driven receiver enabling function */
static void uarte_nrfx_irq_rx_enable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven receiver disabling function */
static void uarte_nrfx_irq_rx_disable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDRX_MASK);
}

/** Interrupt driven error enabling function */
static void uarte_nrfx_irq_err_enable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven error disabling function */
static void uarte_nrfx_irq_err_disable(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ERROR_MASK);
}

/** Interrupt driven pending status function */
static int uarte_nrfx_irq_is_pending(const struct device *dev)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);

	return ((nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_TXSTOPPED_MASK) &&
		 uarte_nrfx_irq_tx_ready_complete(dev))
		||
		(nrf_uarte_int_enable_check(uarte,
					    NRF_UARTE_INT_ENDRX_MASK) &&
		 uarte_nrfx_irq_rx_ready(dev)));
}

/** Interrupt driven interrupt update function */
static int uarte_nrfx_irq_update(const struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uarte_nrfx_irq_callback_set(const struct device *dev,
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
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure              = uarte_nrfx_configure,
	.config_get             = uarte_nrfx_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
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

static int endtx_stoptx_ppi_init(NRF_UARTE_Type *uarte,
				 struct uarte_nrfx_data *data)
{
	nrfx_err_t ret;

	ret = gppi_channel_alloc(&data->ppi_ch_endtx);
	if (ret != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI Channel");
		return -EIO;
	}

	nrfx_gppi_channel_endpoints_setup(data->ppi_ch_endtx,
		nrf_uarte_event_address_get(uarte, NRF_UARTE_EVENT_ENDTX),
		nrf_uarte_task_address_get(uarte, NRF_UARTE_TASK_STOPTX));
	nrfx_gppi_channels_enable(BIT(data->ppi_ch_endtx));

	return 0;
}

static int uarte_instance_init(const struct device *dev,
			       uint8_t interrupts_active)
{
	int err;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	struct uarte_nrfx_data *data = get_dev_data(dev);
	const struct uarte_nrfx_config *cfg = get_dev_config(dev);

	nrf_uarte_disable(uarte);

	data->dev = dev;

	nrf_gpio_pin_write(cfg->pseltxd, 1);
	nrf_gpio_cfg_output(cfg->pseltxd);

	if (cfg->pselrxd !=  NRF_UARTE_PSEL_DISCONNECTED) {
		nrf_gpio_cfg_input(cfg->pselrxd, cfg->rxd_pull);
	}

	nrf_uarte_txrx_pins_set(uarte, cfg->pseltxd, cfg->pselrxd);

	if (cfg->pselcts != NRF_UARTE_PSEL_DISCONNECTED) {
		nrf_gpio_cfg_input(cfg->pselcts, cfg->cts_pull);
	}

	if (cfg->pselrts != NRF_UARTE_PSEL_DISCONNECTED) {
		nrf_gpio_pin_write(cfg->pselrts, 1);
		nrf_gpio_cfg_output(cfg->pselrts);
	}

	nrf_uarte_hwfc_pins_set(uarte, cfg->pselrts, cfg->pselcts);

	err = uarte_nrfx_configure(dev, &get_dev_data(dev)->uart_config);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_UART_ENHANCED_POLL_OUT) &&
	    cfg->flags & UARTE_CFG_FLAG_PPI_ENDTX) {
		err = endtx_stoptx_ppi_init(uarte, data);
		if (err < 0) {
			return err;
		}
	}


#ifdef CONFIG_UART_ASYNC_API
	if (data->async) {
		err = uarte_nrfx_init(dev);
		if (err < 0) {
			return err;
		}
	} else
#endif
	{
		/* Enable receiver and transmitter */
		nrf_uarte_enable(uarte);

		if (cfg->pselrxd != NRF_UARTE_PSEL_DISCONNECTED) {
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);

			nrf_uarte_rx_buffer_set(uarte, &data->rx_data, 1);
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
		}
	}

	if (!(cfg->flags & UARTE_CFG_FLAG_PPI_ENDTX)) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
	}

	if (cfg->flags & UARTE_CFG_FLAG_LOW_POWER) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_TXSTOPPED_MASK);
	}

	/* Set TXSTOPPED event by requesting fake (zero-length) transfer.
	 * Pointer to RAM variable (data->tx_buffer) is set because otherwise
	 * such operation may result in HardFault or RAM corruption.
	 */
	nrf_uarte_tx_buffer_set(uarte, &data->char_out, 0);
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTTX);

	/* switch off transmitter to save an energy */
	nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static void uarte_nrfx_pins_enable(const struct device *dev, bool enable)
{
	const struct uarte_nrfx_config *cfg = get_dev_config(dev);

	if (!(cfg->flags & UARTE_CFG_FLAG_GPIO_MGMT)) {
		return;
	}

	if (enable) {
		nrf_gpio_pin_write(cfg->pseltxd, 1);
		nrf_gpio_cfg_output(cfg->pseltxd);
		if (cfg->pselrxd != NRF_UARTE_PSEL_DISCONNECTED) {
			nrf_gpio_cfg_input(cfg->pselrxd, cfg->rxd_pull);
		}

		if (IS_RTS_PIN_SET(cfg->flags)) {
			nrf_gpio_pin_write(cfg->pselrts, 1);
			nrf_gpio_cfg_output(cfg->pselrts);
		}

		if (IS_CTS_PIN_SET(cfg->flags)) {
			nrf_gpio_cfg_input(cfg->pselcts, cfg->cts_pull);
		}
	} else {
		nrf_gpio_cfg_default(cfg->pseltxd);
		if (cfg->pselrxd != NRF_UARTE_PSEL_DISCONNECTED) {
			nrf_gpio_cfg_default(cfg->pselrxd);
		}

		if (IS_RTS_PIN_SET(cfg->flags)) {
			nrf_gpio_cfg_default(cfg->pselrts);
		}

		if (IS_CTS_PIN_SET(cfg->flags)) {
			nrf_gpio_cfg_default(cfg->pselcts);
		}
	}
}

/** @brief Pend until TX is stopped.
 *
 * There are 2 configurations that must be handled:
 * - ENDTX->TXSTOPPED PPI enabled - just pend until TXSTOPPED event is set
 * - disable ENDTX interrupt and manually trigger STOPTX, pend for TXSTOPPED
 */
static void wait_for_tx_stopped(const struct device *dev)
{
	bool ppi_endtx = get_dev_config(dev)->flags & UARTE_CFG_FLAG_PPI_ENDTX;
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
	bool res;

	if (!ppi_endtx) {
		/* We assume here that it can be called from any context,
		 * including the one that uarte interrupt will not preempt.
		 * Disable endtx interrupt to ensure that it will not be triggered
		 * (if in lower priority context) and stop TX if necessary.
		 */
		nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDTX_MASK);
		NRFX_WAIT_FOR(is_tx_ready(dev), 1000, 1, res);
		if (!nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED)) {
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPTX);
		}
	}

	NRFX_WAIT_FOR(nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_TXSTOPPED),
		      1000, 1, res);

	if (!ppi_endtx) {
		nrf_uarte_int_enable(uarte, NRF_UARTE_INT_ENDTX_MASK);
	}
}


static int uarte_nrfx_pm_control(const struct device *dev,
				 enum pm_device_action action)
{
	NRF_UARTE_Type *uarte = get_uarte_instance(dev);
#if defined(CONFIG_UART_ASYNC_API) || defined(UARTE_INTERRUPT_DRIVEN)
	struct uarte_nrfx_data *data = get_dev_data(dev);
#endif

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		uarte_nrfx_pins_enable(dev, true);
		nrf_uarte_enable(uarte);

#ifdef CONFIG_UART_ASYNC_API
		if (hw_rx_counting_enabled(get_dev_data(dev))) {
			nrfx_timer_enable(&get_dev_config(dev)->timer);
		}
		if (get_dev_data(dev)->async) {
			return 0;
		}
#endif
		if (nrf_uarte_rx_pin_get(uarte) != NRF_UARTE_PSEL_DISCONNECTED) {

			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STARTRX);
#ifdef UARTE_INTERRUPT_DRIVEN
			if (data->int_driven &&
			    data->int_driven->rx_irq_enabled) {
				nrf_uarte_int_enable(uarte,
						     NRF_UARTE_INT_ENDRX_MASK);
			}
#endif
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Disabling UART requires stopping RX, but stop RX event is
		 * only sent after each RX if async UART API is used.
		 */
#ifdef CONFIG_UART_ASYNC_API
		if (get_dev_data(dev)->async) {
			/* Entering inactive state requires device to be no
			 * active asynchronous calls.
			 */
			__ASSERT_NO_MSG(!data->async->rx_enabled);
			__ASSERT_NO_MSG(!data->async->tx_size);

		}
#endif
		if (nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED)) {
#ifdef UARTE_INTERRUPT_DRIVEN
			if (data->int_driven) {
				data->int_driven->rx_irq_enabled =
					nrf_uarte_int_enable_check(uarte,
						NRF_UARTE_INT_ENDRX_MASK);
				if (data->int_driven->rx_irq_enabled) {
					nrf_uarte_int_disable(uarte,
						NRF_UARTE_INT_ENDRX_MASK);
				}
			}
#endif
			nrf_uarte_task_trigger(uarte, NRF_UARTE_TASK_STOPRX);
			while (!nrf_uarte_event_check(uarte,
						      NRF_UARTE_EVENT_RXTO) &&
			       !nrf_uarte_event_check(uarte,
						      NRF_UARTE_EVENT_ERROR)) {
				/* Busy wait for event to register */
			}
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
			nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
		}

		wait_for_tx_stopped(dev);
		uart_disable(dev);
		uarte_nrfx_pins_enable(dev, false);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#define UARTE(idx)			DT_NODELABEL(uart##idx)
#define UARTE_HAS_PROP(idx, prop)	DT_NODE_HAS_PROP(UARTE(idx), prop)
#define UARTE_PROP(idx, prop)		DT_PROP(UARTE(idx), prop)

#define UARTE_PSEL(idx, pin_prop)					       \
	COND_CODE_1(UARTE_HAS_PROP(idx, pin_prop),			       \
		    (UARTE_PROP(idx, pin_prop)),			       \
		    (NRF_UARTE_PSEL_DISCONNECTED))

#define UARTE_PULL(idx, pull_up_prop) \
	COND_CODE_1(UARTE_PROP(idx, pull_up_prop), \
		    (NRF_GPIO_PIN_PULLUP), \
		    (NRF_GPIO_PIN_NOPULL))

#define HWFC_AVAILABLE(idx)					       \
	(UARTE_HAS_PROP(idx, rts_pin) || UARTE_HAS_PROP(idx, cts_pin))

#define UARTE_IRQ_CONFIGURE(idx, isr_handler)				       \
	do {								       \
		IRQ_CONNECT(DT_IRQN(UARTE(idx)), DT_IRQ(UARTE(idx), priority), \
			    isr_handler, DEVICE_DT_GET(UARTE(idx)), 0); \
		irq_enable(DT_IRQN(UARTE(idx)));			       \
	} while (0)

#define HWFC_CONFIG_CHECK(idx) \
	BUILD_ASSERT( \
		(UARTE_PROP(idx, hw_flow_control) && HWFC_AVAILABLE(idx)) \
		|| \
		!UARTE_PROP(idx, hw_flow_control) \
	)

/* Low power mode is used when  rx pin is not defined or in async mode if
 * kconfig option is enabled.
 */
#define USE_LOW_POWER(idx) \
	((UARTE_HAS_PROP(idx, rx_pin) && \
	COND_CODE_1(CONFIG_UART_##idx##_ASYNC,	       \
		(!IS_ENABLED(CONFIG_UART_##idx##_NRF_ASYNC_LOW_POWER)), \
		(1))) ? 0 : UARTE_CFG_FLAG_LOW_POWER)

#define UART_NRF_UARTE_DEVICE(idx)					       \
	HWFC_CONFIG_CHECK(idx);						       \
	UARTE_INT_DRIVEN(idx);						       \
	UARTE_ASYNC(idx);						       \
	static struct uarte_nrfx_data uarte_##idx##_data = {		       \
		UARTE_CONFIG(idx),					       \
		IF_ENABLED(CONFIG_UART_##idx##_ASYNC,			       \
			    (.async = &uarte##idx##_async,))		       \
		IF_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN,	       \
			    (.int_driven = &uarte##idx##_int_driven,))	       \
	};								       \
	static const struct uarte_nrfx_config uarte_##idx##z_config = {	       \
		.uarte_regs = (NRF_UARTE_Type *)DT_REG_ADDR(UARTE(idx)),       \
		.flags =						       \
			(UARTE_HAS_PROP(idx, rts_pin) ?			       \
				UARTE_CFG_FLAG_RTS_PIN_SET : 0) |	       \
			(UARTE_HAS_PROP(idx, cts_pin) ?			       \
				UARTE_CFG_FLAG_CTS_PIN_SET : 0) |	       \
			(IS_ENABLED(CONFIG_UART_##idx##_GPIO_MANAGEMENT) ?     \
				UARTE_CFG_FLAG_GPIO_MGMT : 0) |		       \
			(IS_ENABLED(CONFIG_UART_##idx##_ENHANCED_POLL_OUT) ?   \
				UARTE_CFG_FLAG_PPI_ENDTX : 0) |		       \
			USE_LOW_POWER(idx),				       \
		.pseltxd  = UARTE_PROP(idx, tx_pin),  /* must be set */	       \
		.pselrxd  = UARTE_PSEL(idx, rx_pin),  /* optional */	       \
		.pselcts  = UARTE_PSEL(idx, cts_pin), /* optional */	       \
		.pselrts  = UARTE_PSEL(idx, rts_pin), /* optional */	       \
		.rxd_pull = UARTE_PULL(idx, rx_pull_up),		       \
		.cts_pull = UARTE_PULL(idx, cts_pull_up),		       \
		IF_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC,		       \
			(.timer = NRFX_TIMER_INSTANCE(			       \
				CONFIG_UART_##idx##_NRF_HW_ASYNC_TIMER),))     \
	};								       \
	static int uarte_##idx##_init(const struct device *dev)		       \
	{								       \
		COND_CODE_1(CONFIG_UART_##idx##_ASYNC,			       \
			   (UARTE_IRQ_CONFIGURE(idx, uarte_nrfx_isr_async);),  \
			   (UARTE_IRQ_CONFIGURE(idx, uarte_nrfx_isr_int);))    \
		return uarte_instance_init(				       \
			dev,						       \
			IS_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN));     \
	}								       \
	DEVICE_DT_DEFINE(UARTE(idx),					       \
		      uarte_##idx##_init,				       \
		      uarte_nrfx_pm_control,				       \
		      &uarte_##idx##_data,				       \
		      &uarte_##idx##z_config,				       \
		      PRE_KERNEL_1,					       \
		      CONFIG_SERIAL_INIT_PRIORITY,			       \
		      &uart_nrfx_uarte_driver_api)

#define UARTE_CONFIG(idx)						       \
	.uart_config = {						       \
		.baudrate = UARTE_PROP(idx, current_speed),		       \
		.data_bits = UART_CFG_DATA_BITS_8,			       \
		.stop_bits = UART_CFG_STOP_BITS_1,			       \
		.parity = IS_ENABLED(CONFIG_UART_##idx##_NRF_PARITY_BIT)       \
			  ? UART_CFG_PARITY_EVEN			       \
			  : UART_CFG_PARITY_NONE,			       \
		.flow_ctrl = UARTE_PROP(idx, hw_flow_control)  \
			     ? UART_CFG_FLOW_CTRL_RTS_CTS		       \
			     : UART_CFG_FLOW_CTRL_NONE,			       \
	}

#define UARTE_ASYNC(idx)						       \
	IF_ENABLED(CONFIG_UART_##idx##_ASYNC,				       \
		(struct uarte_async_cb uarte##idx##_async = {		       \
			.hw_rx_counting =				       \
				IS_ENABLED(CONFIG_UART_##idx##_NRF_HW_ASYNC),  \
		}))

#define UARTE_INT_DRIVEN(idx)						       \
	IF_ENABLED(CONFIG_UART_##idx##_INTERRUPT_DRIVEN,	       \
		(static uint8_t uarte##idx##_tx_buffer[\
			MIN(CONFIG_UART_##idx##_NRF_TX_BUFFER_SIZE,	       \
			    BIT_MASK(UARTE##idx##_EASYDMA_MAXCNT_SIZE))];      \
		 static struct uarte_nrfx_int_driven			       \
			uarte##idx##_int_driven = {			       \
				.tx_buffer = uarte##idx##_tx_buffer,	       \
				.tx_buff_size = sizeof(uarte##idx##_tx_buffer),\
			};))

#ifdef CONFIG_UART_0_NRF_UARTE
UART_NRF_UARTE_DEVICE(0);
#endif

#ifdef CONFIG_UART_1_NRF_UARTE
UART_NRF_UARTE_DEVICE(1);
#endif

#ifdef CONFIG_UART_2_NRF_UARTE
UART_NRF_UARTE_DEVICE(2);
#endif

#ifdef CONFIG_UART_3_NRF_UARTE
UART_NRF_UARTE_DEVICE(3);
#endif
