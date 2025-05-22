/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_linflexd

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

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

void uart_nxp_s32_isr(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;

	Linflexd_Uart_Ip_IRQHandler(config->instance);
}

static void uart_nxp_s32_event_handler(const uint8 instance,
				       Linflexd_Uart_Ip_EventType event,
				       const void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	const struct uart_nxp_s32_config *config = dev->config;
	struct uart_nxp_s32_data *data = dev->data;
	struct uart_nxp_s32_int *int_data = &(data->int_data);
	Linflexd_Uart_Ip_StatusType status;

	if (event == LINFLEXD_UART_IP_EVENT_END_TRANSFER) {
		/*
		 * Check the previous UART transmit has	finished
		 * because Rx may also trigger this event
		 */
		status = Linflexd_Uart_Ip_GetTransmitStatus(config->instance, NULL);
		if (status != LINFLEXD_UART_IP_STATUS_BUSY) {
			int_data->tx_fifo_busy = false;
			if (data->callback) {
				data->callback(dev, data->cb_data);
			}
		}
	} else if (event == LINFLEXD_UART_IP_EVENT_RX_FULL) {
		int_data->rx_fifo_busy = false;
		if (data->callback) {
			data->callback(dev, data->cb_data);
		}
	} else if (event == LINFLEXD_UART_IP_EVENT_ERROR) {
		if (data->callback) {
			data->callback(dev, data->cb_data);
		}
	} else {
		/* Other events are not used */
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_nxp_s32_init(const struct device *dev)
{
	const struct uart_nxp_s32_config *config = dev->config;
	int err;
	uint32_t clock_rate;
	Linflexd_Uart_Ip_StatusType status;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		return err;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_rate);
	if (err) {
		return err;
	}

	Linflexd_Uart_Ip_Init(config->instance, &config->hw_cfg);

	status = Linflexd_Uart_Ip_SetBaudrate(config->instance, config->hw_cfg.BaudRate,
					      clock_rate);
	if (status != LINFLEXD_UART_IP_STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(uart, uart_nxp_s32_driver_api) = {
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

#define UART_NXP_S32_HW_INSTANCE_CHECK(i, n) \
	((DT_INST_REG_ADDR(n) == IP_LINFLEX_##i##_BASE) ? i : 0)

#define UART_NXP_S32_HW_INSTANCE(n) \
	LISTIFY(__DEBRACKET LINFLEXD_INSTANCE_COUNT, UART_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define UART_NXP_S32_INTERRUPT_DEFINE(n)					\
	do {									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			uart_nxp_s32_isr, DEVICE_DT_INST_GET(n),		\
			DT_INST_IRQ(n, flags));					\
		irq_enable(DT_INST_IRQN(n));					\
	} while (0)

#define UART_NXP_S32_HW_CONFIG(n)						\
	{									\
		.BaudRate = DT_INST_PROP(n, current_speed),			\
		.BaudRateMantissa = 26U,					\
		.BaudRateDivisor = 16U,						\
		.BaudRateFractionalDivisor = 1U,				\
		.ParityCheck = DT_INST_ENUM_IDX(n, parity) ==			\
				UART_CFG_PARITY_NONE ? false : true,		\
		.ParityType = DT_INST_ENUM_IDX(n, parity) ==			\
					UART_CFG_PARITY_ODD ?			\
				LINFLEXD_UART_IP_PARITY_ODD :			\
				(DT_INST_ENUM_IDX(n, parity) ==			\
						UART_CFG_PARITY_EVEN ?		\
					LINFLEXD_UART_IP_PARITY_ONE :		\
					(DT_INST_ENUM_IDX(n, parity) ==		\
							UART_CFG_PARITY_MARK ?	\
						LINFLEXD_UART_IP_PARITY_EVEN :	\
						LINFLEXD_UART_IP_PARITY_ZERO)),	\
		.StopBitsCount = DT_INST_ENUM_IDX(n, stop_bits) ==		\
						UART_CFG_STOP_BITS_1 ?		\
					LINFLEXD_UART_IP_ONE_STOP_BIT :		\
					LINFLEXD_UART_IP_TWO_STOP_BIT,		\
		.WordLength = DT_INST_ENUM_IDX(n, data_bits) ==			\
						UART_CFG_DATA_BITS_7 ?		\
					LINFLEXD_UART_IP_7_BITS	:		\
					LINFLEXD_UART_IP_8_BITS,		\
		.TransferType = LINFLEXD_UART_IP_USING_INTERRUPTS,		\
		.StateStruct = &Linflexd_Uart_Ip_apStateStructure[n],		\
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (			\
			.Callback = uart_nxp_s32_event_handler,			\
			.CallbackParam = (void *)DEVICE_DT_INST_GET(n),		\
		))								\
	}

#define UART_NXP_S32_INIT_DEVICE(n)						\
	BUILD_ASSERT(DT_INST_ENUM_IDX(n, stop_bits) == UART_CFG_STOP_BITS_1 ||	\
		DT_INST_ENUM_IDX(n, stop_bits) == UART_CFG_STOP_BITS_2,		\
		"Node " DT_NODE_PATH(DT_DRV_INST(n))				\
		" has unsupported stop bits configuration");			\
	BUILD_ASSERT(DT_INST_ENUM_IDX(n, data_bits) == UART_CFG_DATA_BITS_7 ||	\
		DT_INST_ENUM_IDX(n, data_bits) == UART_CFG_DATA_BITS_8,		\
		"Node " DT_NODE_PATH(DT_DRV_INST(n))				\
		" has unsupported data bits configuration");			\
	BUILD_ASSERT(DT_INST_PROP(n, hw_flow_control) == UART_CFG_FLOW_CTRL_NONE,\
		"Node " DT_NODE_PATH(DT_DRV_INST(n))				\
		" has unsupported flow control configuration");			\
	PINCTRL_DT_INST_DEFINE(n);						\
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,				\
		(static struct uart_nxp_s32_data uart_nxp_s32_data_##n;))	\
	static const struct uart_nxp_s32_config uart_nxp_s32_config_##n = {	\
		.instance = UART_NXP_S32_HW_INSTANCE(n),			\
		.base = (LINFLEXD_Type *)DT_INST_REG_ADDR(n),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.hw_cfg = UART_NXP_S32_HW_CONFIG(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
				DT_INST_CLOCKS_CELL(n, name),			\
	};									\
	static int uart_nxp_s32_init_##n(const struct device *dev)		\
	{									\
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,			\
			(UART_NXP_S32_INTERRUPT_DEFINE(n);))			\
										\
		return uart_nxp_s32_init(dev);					\
	}									\
	DEVICE_DT_INST_DEFINE(n,						\
			uart_nxp_s32_init_##n,					\
			NULL,							\
			COND_CODE_1(CONFIG_UART_INTERRUPT_DRIVEN,		\
				   (&uart_nxp_s32_data_##n), (NULL)),		\
			&uart_nxp_s32_config_##n,				\
			PRE_KERNEL_1,						\
			CONFIG_SERIAL_INIT_PRIORITY,				\
			&uart_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_NXP_S32_INIT_DEVICE)
