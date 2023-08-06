/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>

#include <Linflexd_Uart_Ip.h>
#include <Linflexd_Uart_Ip_Irq.h>
#include "uart_nxp_s32_linflexd.h"

static int uart_nxp_s32_err_check(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	Linflexd_Uart_Ip_StatusType status;
	int err = 0;

	status = Linflexd_Uart_Ip_GetReceiveStatus(config->instance, NULL);

	if (status == LINFLEXD_UART_IP_STATUS_RX_OVERRUN) {
		err |= UART_ERROR_OVERRUN;
	}

	if (status == LINFLEXD_UART_IP_STATUS_PARITY_ERROR) {
		err |= UART_ERROR_PARITY;
	}

	if (status == LINFLEXD_UART_IP_STATUS_FRAMING_ERROR) {
		err |= UART_ERROR_FRAMING;
	}

	return err;
}

static void uart_nxp_s32_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_nxp_s32_config *config = dev->config;
	uint32_t linflexd_ier;
	uint8_t key;

	key = irq_lock();

	/* Save enabled Linflexd's interrupts */
	linflexd_ier = sys_read32(POINTER_TO_UINT(&config->base->LINIER));

	Linflexd_Uart_Ip_SyncSend(config->instance, &c, 1,
				  CONFIG_UART_NXP_S32_POLL_OUT_TIMEOUT);

	/* Restore Linflexd's interrupts */
	sys_write32(linflexd_ier, POINTER_TO_UINT(&config->base->LINIER));

	irq_unlock(key);
}

static int uart_nxp_s32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_nxp_s32_config *config = dev->config;
	Linflexd_Uart_Ip_StatusType status;
	uint32_t linflexd_ier;
	int ret;

	status = LINFLEXD_UART_IP_STATUS_SUCCESS;

	/* Save enabled Linflexd's interrupts */
	linflexd_ier = sys_read32(POINTER_TO_UINT(&config->base->LINIER));

	/* Retrieves data with poll method */
	status = Linflexd_Uart_Ip_SyncReceive(config->instance, c, 1,
					      CONFIG_UART_NXP_S32_POLL_IN_TIMEOUT);

	/* Restore Linflexd's interrupts */
	sys_write32(linflexd_ier, POINTER_TO_UINT(&config->base->LINIER));

	if (status == LINFLEXD_UART_IP_STATUS_SUCCESS) {
		ret = 0;
	} else if (status == LINFLEXD_UART_IP_STATUS_TIMEOUT) {
		ret = -1;
	} else {
		ret = -EBUSY;
	}

	return ret;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_nxp_s32_fifo_fill(const struct device *dev, const uint8_t *tx_data,
				  int size)
{
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);

	if (int_data->tx_fifo_busy) {
		return 0;
	}

	int_data->tx_fifo_busy = true;

	Linflexd_Uart_Ip_AsyncSend(config->instance, tx_data, 1);

	return 1;
}

static int uart_nxp_s32_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);

	if (int_data->rx_fifo_busy) {
		return 0;
	}

	*rx_data = int_data->rx_fifo_data;
	int_data->rx_fifo_busy = true;

	Linflexd_Uart_Ip_SetRxBuffer(config->instance, &(int_data->rx_fifo_data), 1);

	return 1;
}

static void uart_nxp_s32_irq_tx_enable(const struct device *dev)
{

	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);
	uint8_t key;

	int_data->irq_tx_enable = true;

	key = irq_lock();

	/* Callback is called in order to transmit the data */
	if (!int_data->tx_fifo_busy) {

		if (data->callback) {
			data->callback(dev, data->cb_data);
		}
	}

	irq_unlock(key);
}

static void uart_nxp_s32_irq_tx_disable(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);

	int_data->irq_tx_enable = false;
	int_data->tx_fifo_busy  = false;

	Linflexd_Uart_Ip_AbortSendingData(config->instance);
}

static int uart_nxp_s32_irq_tx_ready(const struct device *dev)
{
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);

	return !int_data->tx_fifo_busy && int_data->irq_tx_enable;
}

static void uart_nxp_s32_irq_rx_enable(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);

	int_data->irq_rx_enable = true;

	Linflexd_Uart_Ip_AsyncReceive(config->instance, &(int_data->rx_fifo_data), 1);
}

static void uart_nxp_s32_irq_rx_disable(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);

	int_data->irq_rx_enable = false;
	int_data->rx_fifo_busy = false;

	Linflexd_Uart_Ip_AbortReceivingData(config->instance);
}

static int uart_nxp_s32_irq_rx_ready(const struct device *dev)
{
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);

	return !int_data->rx_fifo_busy && int_data->irq_rx_enable;
}

static void uart_nxp_s32_irq_err_enable(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	uint32_t linflexd_ier;

	linflexd_ier = sys_read32(POINTER_TO_UINT(&config->base->LINIER));

	/* Enable frame error interrupt and buffer overrun error interrupt */
	linflexd_ier |= (LINFLEXD_LINIER_FEIE_MASK | LINFLEXD_LINIER_BOIE_MASK);

	sys_write32(linflexd_ier, POINTER_TO_UINT(&config->base->LINIER));
}

static void uart_nxp_s32_irq_err_disable(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	uint32_t linflexd_ier;

	linflexd_ier = sys_read32(POINTER_TO_UINT(&config->base->LINIER));

	/* Disable frame error interrupt and buffer overrun error interrupt */
	linflexd_ier &= ~(LINFLEXD_LINIER_FEIE_MASK | LINFLEXD_LINIER_BOIE_MASK);

	sys_write32(linflexd_ier, POINTER_TO_UINT(&config->base->LINIER));
}

static int uart_nxp_s32_irq_is_pending(const struct device *dev)
{
	return (uart_nxp_s32_irq_tx_ready(dev)) || (uart_nxp_s32_irq_rx_ready(dev));
}

static int uart_nxp_s32_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_nxp_s32_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_nxp_s32_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * Note: s32 UART Tx interrupts when ready to send; Rx interrupts when char
 * received.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */

void uart_nxp_s32_isr(const struct device *dev)
{
	struct uart_nxp_s32_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_nxp_s32_init(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	static uint8_t state_idx;
	uint8_t key;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	key = irq_lock();

	/* Initialize UART with default configuration */
	data->hw_cfg.BaudRate = 115200;
	data->hw_cfg.BaudRateMantissa = 26U;
	data->hw_cfg.BaudRateFractionalDivisor = 1U;
	data->hw_cfg.ParityCheck = false;
	data->hw_cfg.ParityType = LINFLEXD_UART_IP_PARITY_EVEN;
	data->hw_cfg.StopBitsCount = LINFLEXD_UART_IP_ONE_STOP_BIT;
	data->hw_cfg.WordLength = LINFLEXD_UART_IP_8_BITS;
	data->hw_cfg.TransferType = LINFLEXD_UART_IP_USING_INTERRUPTS;
	data->hw_cfg.Callback = s32_uart_callback;
	data->hw_cfg.CallbackParam = NULL;
	data->hw_cfg.StateStruct = &Linflexd_Uart_Ip_apStateStructure[state_idx++];

	Linflexd_Uart_Ip_Init(config->instance, &data->hw_cfg);

	irq_unlock(key);

	return 0;
}

static const struct uart_driver_api uart_nxp_s32_driver_api = {
	.poll_in	  = uart_nxp_s32_poll_in,
	.poll_out	  = uart_nxp_s32_poll_out,
	.err_check	  = uart_nxp_s32_err_check,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill	  = uart_nxp_s32_fifo_fill,
	.fifo_read	  = uart_nxp_s32_fifo_read,
	.irq_tx_enable	  = uart_nxp_s32_irq_tx_enable,
	.irq_tx_disable   = uart_nxp_s32_irq_tx_disable,
	.irq_tx_ready	  = uart_nxp_s32_irq_tx_ready,
	.irq_rx_enable	  = uart_nxp_s32_irq_rx_enable,
	.irq_rx_disable   = uart_nxp_s32_irq_rx_disable,
	.irq_rx_ready	  = uart_nxp_s32_irq_rx_ready,
	.irq_err_enable   = uart_nxp_s32_irq_err_enable,
	.irq_err_disable  = uart_nxp_s32_irq_err_disable,
	.irq_is_pending   = uart_nxp_s32_irq_is_pending,
	.irq_update	  = uart_nxp_s32_irq_update,
	.irq_callback_set = uart_nxp_s32_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

};

#define UART_NXP_S32_NODE(n)	DT_NODELABEL(uart##n)

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define UART_NXP_S32_INTERRUPT_DEFINE(n, isr_handler, parameter)	\
	do {								\
		IRQ_CONNECT(DT_IRQN(UART_NXP_S32_NODE(n)),		\
			DT_IRQ(UART_NXP_S32_NODE(n), priority),		\
			isr_handler,					\
			parameter,					\
			DT_IRQ(UART_NXP_S32_NODE(n), flags));		\
									\
		irq_enable(DT_IRQN(UART_NXP_S32_NODE(n)));		\
	} while (0)

#else
#define UART_NXP_S32_INTERRUPT_DEFINE(n, isr_handler, parameter)
#endif

#define UART_NXP_S32_INIT_DEVICE(n)						\
	PINCTRL_DT_DEFINE(UART_NXP_S32_NODE(n));				\
	static struct uart_nxp_s32_data uart_nxp_s32_data_##n;			\
	static const struct uart_nxp_s32_config uart_nxp_s32_config_##n = {	\
		.instance = n,							\
		.base = (LINFLEXD_Type *)DT_REG_ADDR(UART_NXP_S32_NODE(n)),	\
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(UART_NXP_S32_NODE(n)),	\
	};									\
	static int uart_nxp_s32_init_##n(const struct device *dev)		\
	{									\
		UART_NXP_S32_INTERRUPT_DEFINE(n, Linflexd_Uart_Ip_IRQHandler, n);\
										\
		return uart_nxp_s32_init(dev);					\
	}									\
	DEVICE_DT_DEFINE(UART_NXP_S32_NODE(n),					\
			&uart_nxp_s32_init_##n,					\
			NULL,							\
			&uart_nxp_s32_data_##n,					\
			&uart_nxp_s32_config_##n,				\
			PRE_KERNEL_1,						\
			CONFIG_SERIAL_INIT_PRIORITY,				\
			&uart_nxp_s32_driver_api);

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(0), okay)
UART_NXP_S32_INIT_DEVICE(0)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(1), okay)
UART_NXP_S32_INIT_DEVICE(1)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(2), okay)
UART_NXP_S32_INIT_DEVICE(2)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(3), okay)
UART_NXP_S32_INIT_DEVICE(3)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(4), okay)
UART_NXP_S32_INIT_DEVICE(4)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(5), okay)
UART_NXP_S32_INIT_DEVICE(5)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(6), okay)
UART_NXP_S32_INIT_DEVICE(6)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(7), okay)
UART_NXP_S32_INIT_DEVICE(7)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(8), okay)
UART_NXP_S32_INIT_DEVICE(8)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(9), okay)
UART_NXP_S32_INIT_DEVICE(9)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(10), okay)
UART_NXP_S32_INIT_DEVICE(10)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(11), okay)
UART_NXP_S32_INIT_DEVICE(11)
#endif

#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(12), okay)
UART_NXP_S32_INIT_DEVICE(12)
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void s32_uart_device_event(const struct device *dev, Linflexd_Uart_Ip_EventType event,
				  void *user_data)
{
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);
	Linflexd_Uart_Ip_StatusType status;

	ARG_UNUSED(user_data);

	if (event == LINFLEXD_UART_IP_EVENT_END_TRANSFER) {
		/*
		 * Check the previous UART transmit has	finished
		 * because Rx may also trigger this event
		 */
		status = Linflexd_Uart_Ip_GetTransmitStatus(config->instance, NULL);
		if (status != LINFLEXD_UART_IP_STATUS_BUSY) {
			int_data->tx_fifo_busy = false;
			uart_nxp_s32_isr(dev);
		}
	} else if (event == LINFLEXD_UART_IP_EVENT_RX_FULL) {
		int_data->rx_fifo_busy = false;
		uart_nxp_s32_isr(dev);
	} else if (event == LINFLEXD_UART_IP_EVENT_ERROR) {
		uart_nxp_s32_isr(dev);
	} else {
		/* Other events are not used */
	}
}

void s32_uart_callback(const uint8 instance, Linflexd_Uart_Ip_EventType event, void *user_data)
{
	const struct device *dev;

	switch (instance) {
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(0), okay)
	case 0:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(0));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(1), okay)
	case 1:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(1));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(2), okay)
	case 2:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(2));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(3), okay)
	case 3:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(3));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(4), okay)
	case 4:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(4));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(5), okay)
	case 5:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(5));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(6), okay)
	case 6:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(6));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(7), okay)
	case 7:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(7));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(8), okay)
	case 8:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(8));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(9), okay)
	case 9:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(9));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(10), okay)
	case 10:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(10));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(11), okay)
	case 11:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(11));
		break;
#endif
#if DT_NODE_HAS_STATUS(UART_NXP_S32_NODE(12), okay)
	case 12:
		dev = DEVICE_DT_GET(UART_NXP_S32_NODE(12));
		break;
#endif
	default:
		dev = NULL;
		break;
	}

	if (dev != NULL) {
		s32_uart_device_event(dev, event, user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
