/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for Infineon CAT1 MCU family.
 *
 * Note:
 * - Uart ASYNC functionality is not implemented in current
 *   version of Uart CAT1 driver.
 */

#define DT_DRV_COMPAT infineon_cat1_uart

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <cyhal_uart.h>
#include <cyhal_utils_impl.h>
#include <cyhal_scb_common.h>

/* Data structure */
struct ifx_cat1_uart_data {
	cyhal_uart_t obj;                               /* UART CYHAL object */
	struct uart_config cfg;
	cyhal_resource_inst_t hw_resource;
	cyhal_clock_t clock;

#if CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;           /* Interrupt Callback */
	void *irq_cb_data;                              /* Interrupt Callback Arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* Device config structure */
struct ifx_cat1_uart_config {
	const struct pinctrl_dev_config *pcfg;
	CySCB_Type *reg_addr;
	struct uart_config dt_cfg;
	uint8_t irq_priority;
};

/* Default Counter configuration structure */
static const cy_stc_scb_uart_config_t _cyhal_uart_default_config = {
	.uartMode = CY_SCB_UART_STANDARD,
	.enableMutliProcessorMode = false,
	.smartCardRetryOnNack = false,
	.irdaInvertRx = false,
	.irdaEnableLowPowerReceiver = false,
	.oversample = 12,
	.enableMsbFirst = false,
	.dataWidth = 8UL,
	.parity = CY_SCB_UART_PARITY_NONE,
	.stopBits = CY_SCB_UART_STOP_BITS_1,
	.enableInputFilter = false,
	.breakWidth = 11UL,
	.dropOnFrameError = false,
	.dropOnParityError = false,

	.receiverAddress = 0x0UL,
	.receiverAddressMask = 0x0UL,
	.acceptAddrInFifo = false,

	.enableCts = false,
	.ctsPolarity = CY_SCB_UART_ACTIVE_LOW,
#if defined(COMPONENT_CAT1A) || defined(COMPONENT_CAT1B)
	.rtsRxFifoLevel = 20UL,
#elif defined(COMPONENT_CAT2)
	.rtsRxFifoLevel = 3UL,
#endif
	.rtsPolarity = CY_SCB_UART_ACTIVE_LOW,

	/* Level triggers when at least one element is in FIFO */
	.rxFifoTriggerLevel = 0UL,
	.rxFifoIntEnableMask = 0x0UL,

	/* Level triggers when half-fifo is half empty */
	.txFifoTriggerLevel = (CY_SCB_FIFO_SIZE / 2 - 1),
	.txFifoIntEnableMask = 0x0UL
};

/* Helper API */
static cyhal_uart_parity_t _convert_uart_parity_z_to_cyhal(enum uart_config_parity parity)
{
	cyhal_uart_parity_t cyhal_parity;

	switch (parity) {
	case UART_CFG_PARITY_NONE:
		cyhal_parity = CYHAL_UART_PARITY_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		cyhal_parity = CYHAL_UART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		cyhal_parity = CYHAL_UART_PARITY_EVEN;
		break;
	default:
		cyhal_parity = CYHAL_UART_PARITY_NONE;
	}
	return cyhal_parity;
}

static uint32_t _convert_uart_stop_bits_z_to_cyhal(enum uart_config_stop_bits stop_bits)
{
	uint32_t cyhal_stop_bits;

	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		cyhal_stop_bits = 1u;
		break;

	case UART_CFG_STOP_BITS_2:
		cyhal_stop_bits = 2u;
		break;
	default:
		cyhal_stop_bits = 1u;
	}
	return cyhal_stop_bits;
}

static uint32_t _convert_uart_data_bits_z_to_cyhal(enum uart_config_data_bits data_bits)
{
	uint32_t cyhal_data_bits;

	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		cyhal_data_bits = 1u;
		break;

	case UART_CFG_DATA_BITS_6:
		cyhal_data_bits = 6u;
		break;

	case UART_CFG_DATA_BITS_7:
		cyhal_data_bits = 7u;
		break;

	case UART_CFG_DATA_BITS_8:
		cyhal_data_bits = 8u;
		break;

	case UART_CFG_DATA_BITS_9:
		cyhal_data_bits = 9u;
		break;

	default:
		cyhal_data_bits = 1u;
	}
	return cyhal_data_bits;
}

static int32_t _get_hw_block_num(CySCB_Type *reg_addr)
{
	extern const uint8_t _CYHAL_SCB_BASE_ADDRESS_INDEX[_SCB_ARRAY_SIZE];
	extern CySCB_Type *const _CYHAL_SCB_BASE_ADDRESSES[_SCB_ARRAY_SIZE];

	uint32_t i;

	for (i = 0u; i < _SCB_ARRAY_SIZE; i++) {
		if (_CYHAL_SCB_BASE_ADDRESSES[i] == reg_addr) {
			return _CYHAL_SCB_BASE_ADDRESS_INDEX[i];
		}
	}

	return -1;
}

static int ifx_cat1_uart_poll_in(const struct device *dev, unsigned char *c)
{
	cy_rslt_t rec;
	struct ifx_cat1_uart_data *data = dev->data;

	rec = cyhal_uart_getc(&data->obj, c, 0u);

	return ((rec == CY_SCB_UART_RX_NO_DATA) ? -1 : 0);
}

static void ifx_cat1_uart_poll_out(const struct device *dev, unsigned char c)
{
	struct ifx_cat1_uart_data *data = dev->data;

	(void) cyhal_uart_putc(&data->obj, (uint32_t)c);
}

static int ifx_cat1_uart_err_check(const struct device *dev)
{
	struct ifx_cat1_uart_data *data = dev->data;
	uint32_t status = Cy_SCB_UART_GetRxFifoStatus(data->obj.base);
	int errors = 0;

	if (status & CY_SCB_UART_RX_OVERFLOW) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (status & CY_SCB_UART_RX_ERR_PARITY) {
		errors |= UART_ERROR_PARITY;
	}

	if (status & CY_SCB_UART_RX_ERR_FRAME) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int ifx_cat1_uart_configure(const struct device *dev,
				   const struct uart_config *cfg)
{
	__ASSERT_NO_MSG(cfg != NULL);

	cy_rslt_t result;
	struct ifx_cat1_uart_data *data = dev->data;

	cyhal_uart_cfg_t uart_cfg = {
		.data_bits = _convert_uart_data_bits_z_to_cyhal(cfg->data_bits),
		.stop_bits = _convert_uart_stop_bits_z_to_cyhal(cfg->stop_bits),
		.parity = _convert_uart_parity_z_to_cyhal(cfg->parity)
	};

	/* Store Uart Zephyr configuration (uart config) into data structure */
	data->cfg = *cfg;

	/* Configure parity, data and stop bits */
	result = cyhal_uart_configure(&data->obj, &uart_cfg);

	/* Configure the baud rate */
	if (result == CY_RSLT_SUCCESS) {
		result = cyhal_uart_set_baud(&data->obj, cfg->baudrate, NULL);
	}

	/* Set RTS/CTS flow control pins as NC so cyhal will skip initialization */
	data->obj.pin_cts = NC;
	data->obj.pin_rts = NC;

	/* Enable RTS/CTS flow control */
	if ((result == CY_RSLT_SUCCESS) && cfg->flow_ctrl) {
		Cy_SCB_UART_EnableCts(data->obj.base);
	}

	return (result == CY_RSLT_SUCCESS) ? 0 : -ENOTSUP;
};

static int ifx_cat1_uart_config_get(const struct device *dev,
				    struct uart_config *cfg)
{
	ARG_UNUSED(dev);

	struct ifx_cat1_uart_data *const data = dev->data;

	if (cfg == NULL) {
		return -EINVAL;
	}

	*cfg = data->cfg;
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* Uart event callback for Interrupt driven mode */
static void _uart_event_callback_irq_mode(void *arg, cyhal_uart_event_t event)
{
	ARG_UNUSED(event);

	const struct device *dev = (const struct device *) arg;
	struct ifx_cat1_uart_data *const data = dev->data;

	if (data->irq_cb != NULL) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

/* Fill FIFO with data */
static int ifx_cat1_uart_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data, int size)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	size_t _size = (size_t) size;

	(void)cyhal_uart_write(&data->obj, (uint8_t *) tx_data,  &_size);
	return (int) _size;
}

/* Read data from FIFO */
static int ifx_cat1_uart_fifo_read(const struct device *dev,
				   uint8_t *rx_data, const int size)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	size_t _size = (size_t) size;

	(void)cyhal_uart_read(&data->obj, rx_data, &_size);
	return (int) _size;
}

/* Enable TX interrupt */
static void ifx_cat1_uart_irq_tx_enable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj,
				(cyhal_uart_event_t) CYHAL_UART_IRQ_TX_EMPTY,
				config->irq_priority, 1);
}

/* Disable TX interrupt */
static void ifx_cat1_uart_irq_tx_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj,
				(cyhal_uart_event_t) CYHAL_UART_IRQ_TX_EMPTY,
				config->irq_priority, 0);
}

/* Check if UART TX buffer can accept a new char */
static int ifx_cat1_uart_irq_tx_ready(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	uint32_t mask = Cy_SCB_GetTxInterruptStatusMasked(data->obj.base);

	return (((mask & (CY_SCB_UART_TX_NOT_FULL | SCB_INTR_TX_EMPTY_Msk)) != 0u) ? 1 : 0);
}

/* Check if UART TX block finished transmission */
static int ifx_cat1_uart_irq_tx_complete(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;

	return (int) !(cyhal_uart_is_tx_active(&data->obj));
}

/* Enable RX interrupt */
static void ifx_cat1_uart_irq_rx_enable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj,
				(cyhal_uart_event_t) CYHAL_UART_IRQ_RX_NOT_EMPTY,
				config->irq_priority, 1);
}

/* Disable TX interrupt */
static void ifx_cat1_uart_irq_rx_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj,
				(cyhal_uart_event_t) CYHAL_UART_IRQ_RX_NOT_EMPTY,
				config->irq_priority, 0);
}

/* Check if UART RX buffer has a received char */
static int ifx_cat1_uart_irq_rx_ready(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;

	return cyhal_uart_readable(&data->obj) ? 1 : 0;
}

/* Enable Error interrupts */
static void ifx_cat1_uart_irq_err_enable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj, (cyhal_uart_event_t)
				(CYHAL_UART_IRQ_TX_ERROR | CYHAL_UART_IRQ_RX_ERROR),
				config->irq_priority, 1);
}

/* Disable Error interrupts */
static void ifx_cat1_uart_irq_err_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	cyhal_uart_enable_event(&data->obj, (cyhal_uart_event_t)
				(CYHAL_UART_IRQ_TX_ERROR | CYHAL_UART_IRQ_RX_ERROR),
				config->irq_priority, 0);
}

/* Check if any IRQs is pending */
static int ifx_cat1_uart_irq_is_pending(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	uint32_t intcause = Cy_SCB_GetInterruptCause(data->obj.base);

	return (int) (intcause & (CY_SCB_TX_INTR | CY_SCB_RX_INTR));
}

/* Start processing interrupts in ISR.
 * This function should be called the first thing in the ISR. Calling
 * uart_irq_rx_ready(), uart_irq_tx_ready(), uart_irq_tx_complete()
 * allowed only after this.
 */
static int ifx_cat1_uart_irq_update(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	int status = 1;

	if (((ifx_cat1_uart_irq_is_pending(dev) & CY_SCB_RX_INTR) != 0u) &&
	    (Cy_SCB_UART_GetNumInRxFifo(data->obj.base) == 0u)) {
		status = 0;
	}

	return status;
}

static void ifx_cat1_uart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *cb_data)
{
	struct ifx_cat1_uart_data *data = dev->data;
	cyhal_uart_t *uart_obj = &data->obj;

	/* Store user callback info */
	data->irq_cb = cb;
	data->irq_cb_data = cb_data;

	/* Register a uart general callback handler  */
	cyhal_uart_register_callback(uart_obj, _uart_event_callback_irq_mode, (void *) dev);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static int ifx_cat1_uart_init(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;
	cy_rslt_t result;
	int ret;

	cyhal_uart_configurator_t uart_init_cfg = {
		.resource = &data->hw_resource,
		.config = &_cyhal_uart_default_config,
		.clock = &data->clock,
		.gpios = {
			.pin_tx  = NC,
			.pin_rts = NC,
			.pin_cts = NC,
		},
	};

	/* Dedicate SCB HW resource */
	data->hw_resource.type = CYHAL_RSC_SCB;
	data->hw_resource.block_num = _get_hw_block_num(config->reg_addr);

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Allocates clock for selected IP block */
	result = _cyhal_utils_allocate_clock(&data->clock, &data->hw_resource,
					     CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT, true);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Assigns a programmable divider to a selected IP block */
	en_clk_dst_t clk_idx = _cyhal_scb_get_clock_index(uart_init_cfg.resource->block_num);

	result = _cyhal_utils_peri_pclk_assign_divider(clk_idx, uart_init_cfg.clock);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Initialize the UART peripheral */
	result = cyhal_uart_init_cfg(&data->obj, &uart_init_cfg);
	if (result != CY_RSLT_SUCCESS) {
		return -ENOTSUP;
	}

	/* Perform initial Uart configuration */
	data->obj.is_clock_owned = true;
	ret = ifx_cat1_uart_configure(dev, &config->dt_cfg);

	return ret;
}

static DEVICE_API(uart, ifx_cat1_uart_driver_api) = {
	.poll_in = ifx_cat1_uart_poll_in,
	.poll_out = ifx_cat1_uart_poll_out,
	.err_check = ifx_cat1_uart_err_check,

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = ifx_cat1_uart_configure,
	.config_get = ifx_cat1_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = ifx_cat1_uart_fifo_fill,
	.fifo_read = ifx_cat1_uart_fifo_read,
	.irq_tx_enable = ifx_cat1_uart_irq_tx_enable,
	.irq_tx_disable = ifx_cat1_uart_irq_tx_disable,
	.irq_tx_ready = ifx_cat1_uart_irq_tx_ready,
	.irq_rx_enable = ifx_cat1_uart_irq_rx_enable,
	.irq_rx_disable = ifx_cat1_uart_irq_rx_disable,
	.irq_tx_complete = ifx_cat1_uart_irq_tx_complete,
	.irq_rx_ready = ifx_cat1_uart_irq_rx_ready,
	.irq_err_enable = ifx_cat1_uart_irq_err_enable,
	.irq_err_disable = ifx_cat1_uart_irq_err_disable,
	.irq_is_pending = ifx_cat1_uart_irq_is_pending,
	.irq_update = ifx_cat1_uart_irq_update,
	.irq_callback_set = ifx_cat1_uart_irq_callback_set,
#endif    /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define INFINEON_CAT1_UART_INIT(n)							     \
	PINCTRL_DT_INST_DEFINE(n);							     \
	static struct ifx_cat1_uart_data ifx_cat1_uart##n##_data;			     \
											     \
	static struct ifx_cat1_uart_config ifx_cat1_uart##n##_cfg = {			     \
		.dt_cfg.baudrate = DT_INST_PROP(n, current_speed),			     \
		.dt_cfg.parity = DT_INST_ENUM_IDX(n, parity),				     \
		.dt_cfg.stop_bits = DT_INST_ENUM_IDX(n, stop_bits),			     \
		.dt_cfg.data_bits = DT_INST_ENUM_IDX(n, data_bits),			     \
		.dt_cfg.flow_ctrl = DT_INST_PROP(n, hw_flow_control),			     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				     \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),				     \
		.irq_priority = DT_INST_IRQ(n, priority)				     \
	};										     \
											     \
	DEVICE_DT_INST_DEFINE(n,							     \
			      ifx_cat1_uart_init, NULL,					     \
			      &ifx_cat1_uart##n##_data,					     \
			      &ifx_cat1_uart##n##_cfg, PRE_KERNEL_1,			     \
			      CONFIG_SERIAL_INIT_PRIORITY,				     \
			      &ifx_cat1_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_UART_INIT)
