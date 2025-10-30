/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat2_uart

#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#include <cy_scb_uart.h>
#include <cy_gpio.h>
#include <cy_syslib.h>
#include <cy_sysint.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat2.h>
#include <zephyr/drivers/serial/uart_ifx_cat2.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_ifx_cat2, CONFIG_UART_LOG_LEVEL);

cy_rslt_t ifx_cat2_uart_set_baud(const struct device *dev, uint32_t baudrate)
{
	cy_rslt_t status = 0;
	struct ifx_cat2_uart_data *data = dev->data;
	const struct ifx_cat2_uart_config *const config = dev->config;
	uint8_t best_oversample = IFX_CAT2_UART_OVERSAMPLE_MIN;
	uint8_t best_difference = 0xFF;
	uint32_t divider;
	uint32_t peri_frequency;

	Cy_SCB_UART_Disable(config->reg_addr, NULL);
	peri_frequency = Cy_SysClk_ClkHfGetFrequency();
	for (uint8_t i = IFX_CAT2_UART_OVERSAMPLE_MIN; i < IFX_CAT2_UART_OVERSAMPLE_MAX + 1; i++) {
		uint32_t tmp_divider = ((peri_frequency + ((baudrate * i) / 2))) / (baudrate * i);
		uint32_t actual_baud = (peri_frequency / (tmp_divider * i));
		uint8_t difference = IFX_UART_BAUD_DIFF(actual_baud, baudrate);

		if (difference < best_difference) {
			best_difference = difference;
			best_oversample = i;
		}
	}

	if (best_difference > IFX_CAT2_UART_MAX_BAUD_PERCENT_DIFFERENCE) {
		__ASSERT_NO_MSG(0);
	}

	data->scb_config.oversample = best_oversample;
	divider = IFX_UART_DIVIDER(peri_frequency, baudrate, best_oversample);

	en_clk_dst_t clk_idx = ifx_cat2_scb_get_clock_index(data->hw_resource.block_num);

	status = ifx_cat2_utils_peri_pclk_set_divider(clk_idx, &(data->clock), divider - 1);
	if (status < 0) {
		return status;
	}

	SCB_CTRL(config->reg_addr) = IFX_UART_SCB_CTRL_VALUE(data->scb_config, best_oversample);

	Cy_SCB_UART_Enable(config->reg_addr);

	return status;
}

uint32_t ifx_cat2_uart_get_num_in_tx_fifo(const struct device *dev)
{
	const struct ifx_cat2_uart_config *const config = dev->config;

	return Cy_SCB_GetNumInTxFifo(config->reg_addr);
}

bool ifx_cat2_uart_get_tx_active(const struct device *dev)
{
	const struct ifx_cat2_uart_config *const config = dev->config;

	return Cy_SCB_GetTxSrValid(config->reg_addr) ? true : false;
}

static int ifx_cat2_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct ifx_cat2_uart_config *const config = dev->config;
	uint32_t read_value = Cy_SCB_UART_Get(config->reg_addr);

	if (read_value == CY_SCB_UART_RX_NO_DATA) {
		return -1;
	}
	*c = (uint8_t)read_value;

	return 0;
}

static void ifx_cat2_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct ifx_cat2_uart_config *const config = dev->config;

	while (Cy_SCB_UART_Put(config->reg_addr, c) == 0) {
		/* Wait until the character is placed in the FIFO */
	}
}

static int ifx_cat2_uart_err_check(const struct device *dev)
{
	const struct ifx_cat2_uart_config *const config = dev->config;
	uint32_t status = Cy_SCB_UART_GetRxFifoStatus(config->reg_addr);
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

static const uint8_t parity_lut[] = {
    [UART_CFG_PARITY_NONE] = CY_SCB_UART_PARITY_NONE,
    [UART_CFG_PARITY_ODD]  = CY_SCB_UART_PARITY_ODD,
    [UART_CFG_PARITY_EVEN] = CY_SCB_UART_PARITY_EVEN,
};

static const uint8_t stop_bits_lut[] = {
    [UART_CFG_STOP_BITS_1] = CY_SCB_UART_STOP_BITS_1,
    [UART_CFG_STOP_BITS_2] = CY_SCB_UART_STOP_BITS_2,
};

static const uint8_t data_bits_lut[] = {
    [UART_CFG_DATA_BITS_5] = 5,
    [UART_CFG_DATA_BITS_6] = 6,
    [UART_CFG_DATA_BITS_7] = 7,
    [UART_CFG_DATA_BITS_8] = 8,
    [UART_CFG_DATA_BITS_9] = 9,
};

static int ifx_cat2_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	__ASSERT_NO_MSG(cfg != NULL);

	cy_rslt_t result;
	struct ifx_cat2_uart_data *data = dev->data;
	const struct ifx_cat2_uart_config *const config = dev->config;

	data->cfg = *cfg;
	Cy_SCB_UART_Disable(config->reg_addr, NULL);
	data->scb_config.dataWidth = CONVERT_UART_DATA_BITS_Z_TO_CY(cfg->data_bits);
	data->scb_config.stopBits = CONVERT_UART_STOP_BITS_Z_TO_CY(cfg->stop_bits);
	data->scb_config.parity = CONVERT_UART_PARITY_Z_TO_CY(cfg->parity);

	Cy_SCB_UART_Init(config->reg_addr, &(data->scb_config), NULL);
	Cy_SCB_UART_Enable(config->reg_addr);
	result = ifx_cat2_uart_set_baud(dev, cfg->baudrate);

	if ((result == CY_RSLT_SUCCESS) && cfg->flow_ctrl) {
		Cy_SCB_UART_EnableCts(config->reg_addr);
	}

	return (result == CY_RSLT_SUCCESS) ? 0 : -ENOTSUP;
}

static int ifx_cat2_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	__ASSERT_NO_MSG(cfg != NULL);

	struct ifx_cat2_uart_data *const data = dev->data;

	*cfg = data->cfg;

	return 0;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

static int ifx_cat2_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct ifx_cat2_uart_config *const config = dev->config;
	size_t tx_length;

	tx_length = Cy_SCB_UART_PutArray(config->reg_addr, (void *)tx_data, size);

	return (int)tx_length;
}

static int ifx_cat2_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct ifx_cat2_uart_config *const config = dev->config;
	size_t rx_length;

	rx_length = Cy_SCB_UART_GetArray(config->reg_addr, rx_data, size);

	return (int)rx_length;
}

void ifx_cat2_uart_enable_event(const struct device *dev, uint32_t event, bool enable)
{
	struct ifx_cat2_uart_data *const data = dev->data;
	const struct ifx_cat2_uart_config *const config = dev->config;
	uint32_t tx_mask = 0x0;
	uint32_t rx_mask = 0x0;
	uint32_t current_tx_mask = Cy_SCB_GetTxInterruptMask(config->reg_addr);
	uint32_t current_rx_mask = Cy_SCB_GetRxInterruptMask(config->reg_addr);

	irq_disable(config->irq_num);
	NVIC_ClearPendingIRQ(config->irq_num);

	if (event & CY_SCB_UART_TRANSMIT_EMTPY) {
		tx_mask |= CY_SCB_UART_TX_EMPTY;
	}
	if (event & CY_SCB_UART_TRANSMIT_ERR_EVENT) {
		tx_mask |= IFX_UART_TX_MASK(data->scb_config.uartMode);
	}
	if (event & CY_SCB_UART_RECEIVE_NOT_EMTPY) {
		rx_mask |= CY_SCB_UART_RX_NOT_EMPTY;
	}
	if (event & CY_SCB_UART_RECEIVE_ERR_EVENT) {
		rx_mask |= CY_SCB_UART_RECEIVE_ERR;
	}
	if (enable && tx_mask) {
		Cy_SCB_ClearTxInterrupt(config->reg_addr, tx_mask);
	}
	if (enable && rx_mask) {
		Cy_SCB_ClearRxInterrupt(config->reg_addr, rx_mask);
	}

	Cy_SCB_SetTxInterruptMask(config->reg_addr, (enable ? (current_tx_mask | tx_mask)
							    : (current_tx_mask & ~tx_mask)));
	Cy_SCB_SetRxInterruptMask(config->reg_addr, (enable ? (current_rx_mask | rx_mask)
							    : (current_rx_mask & ~rx_mask)));
	irq_enable(config->irq_num);
}

static void ifx_cat2_uart_irq_tx_enable(const struct device *dev)
{
	ifx_cat2_uart_enable_event(dev, (uint32_t)CY_SCB_UART_TRANSMIT_EMTPY, 1);
}

static void ifx_cat2_uart_irq_tx_disable(const struct device *dev)
{
	ifx_cat2_uart_enable_event(dev, (uint32_t)CY_SCB_UART_TRANSMIT_EMTPY, 0);
}

static int ifx_cat2_uart_irq_tx_ready(const struct device *dev)
{
	const struct ifx_cat2_uart_config *const config = dev->config;
	uint32_t mask = Cy_SCB_GetTxInterruptStatusMasked(config->reg_addr);

	return (((mask & (CY_SCB_UART_TX_NOT_FULL | SCB_INTR_TX_EMPTY_Msk)) != 0u) ? 1 : 0);
}

static int ifx_cat2_uart_irq_tx_complete(const struct device *dev)
{
	struct ifx_cat2_uart_data *const data = dev->data;
	const struct ifx_cat2_uart_config *const config = dev->config;

	return Cy_SCB_IsTxComplete(config->reg_addr) ||
	       ((data->context.txStatus & CY_SCB_UART_TRANSMIT_ACTIVE) == 0);
}

static void ifx_cat2_uart_irq_rx_enable(const struct device *dev)
{
	ifx_cat2_uart_enable_event(dev, (uint32_t)CY_SCB_UART_RECEIVE_NOT_EMTPY, 1);
}

static void ifx_cat2_uart_irq_rx_disable(const struct device *dev)
{
	ifx_cat2_uart_enable_event(dev, (uint32_t)CY_SCB_UART_RECEIVE_NOT_EMTPY, 0);
}

static int ifx_cat2_uart_irq_rx_ready(const struct device *dev)
{
	struct ifx_cat2_uart_data *const data = dev->data;
	const struct ifx_cat2_uart_config *const config = dev->config;
	uint32_t number_available = Cy_SCB_UART_GetNumInRxFifo(config->reg_addr);

	if (data->context.rxRingBuf != NULL) {
		number_available +=
			Cy_SCB_UART_GetNumInRingBuffer(config->reg_addr, &(data->context));
	}

	return (number_available) ? 1 : 0;
}

static void ifx_cat2_uart_irq_err_enable(const struct device *dev)
{
	ifx_cat2_uart_enable_event(dev,
			((uint32_t)CY_SCB_UART_TRANSMIT_ERR_EVENT |
			 (uint32_t)CY_SCB_UART_RECEIVE_ERR_EVENT), 1);
}

static void ifx_cat2_uart_irq_err_disable(const struct device *dev)
{
	ifx_cat2_uart_enable_event(dev,
				  ((uint32_t)CY_SCB_UART_TRANSMIT_ERR_EVENT |
				   (uint32_t)CY_SCB_UART_RECEIVE_ERR_EVENT),
				  0);
}

static int ifx_cat2_uart_irq_is_pending(const struct device *dev)
{
	const struct ifx_cat2_uart_config *const config = dev->config;
	uint32_t intcause = Cy_SCB_GetInterruptCause(config->reg_addr);

	return (int)(intcause & (CY_SCB_TX_INTR | CY_SCB_RX_INTR));
}

static int ifx_cat2_uart_irq_update(const struct device *dev)
{
	return ifx_cat2_uart_irq_is_pending(dev) ? 1 : 0;
}

static void ifx_cat2_uart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct ifx_cat2_uart_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

static void ifx_cat2_uart_irq_handler(const struct device *dev)
{
	const struct ifx_cat2_uart_config *const config = dev->config;
	struct ifx_cat2_uart_data *const data = dev->data;
	CySCB_Type *base = config->reg_addr;
	uint32_t rx_status = Cy_SCB_GetRxInterruptStatusMasked(base);
	uint32_t tx_status = Cy_SCB_GetTxInterruptStatusMasked(base);

	Cy_SCB_ClearRxInterrupt(base, rx_status);
	Cy_SCB_ClearTxInterrupt(base, tx_status);

	if (data->irq_cb != NULL) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif

CySCB_Type *const _IFX_CAT2_SCB_BASE_ADDRESSES[_IFX_CAT2_SCB_ARRAY_SIZE] = {
#ifdef SCB0
	SCB0,
#endif
#ifdef SCB1
	SCB1,
#endif
#ifdef SCB2
	SCB2,
#endif
#ifdef SCB3
	SCB3,
#endif
#ifdef SCB4
	SCB4,
#endif
};

const uint8_t _IFX_CAT2_SCB_BASE_ADDRESS_INDEX[_IFX_CAT2_SCB_ARRAY_SIZE] = {
#ifdef SCB0
	0u,
#endif
#ifdef SCB1
	1u,
#endif
#ifdef SCB2
	2u,
#endif
#ifdef SCB3
	3u,
#endif
#ifdef SCB4
	4u,
#endif
};

int32_t ifx_cat2_uart_get_hw_block_num(CySCB_Type *reg_addr)
{
	extern const uint8_t _IFX_CAT2_SCB_BASE_ADDRESS_INDEX[_IFX_CAT2_SCB_ARRAY_SIZE];
	uint32_t i;

	for (i = 0u; i < _IFX_CAT2_SCB_ARRAY_SIZE; i++) {
		if (_IFX_CAT2_SCB_BASE_ADDRESSES[i] == reg_addr) {
			return _IFX_CAT2_SCB_BASE_ADDRESS_INDEX[i];
		}
	}

	return -1;
}

static int ifx_cat2_uart_init(const struct device *dev)
{
	struct ifx_cat2_uart_data *const data = dev->data;
	const struct ifx_cat2_uart_config *const config = dev->config;
	cy_rslt_t result;
	int ret;

	data->hw_resource.type = IFX_CAT2_RSC_SCB;
	data->hw_resource.block_num = ifx_cat2_uart_get_hw_block_num(config->reg_addr);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	data->scb_config = *config->uart_def_conf;
	result = (cy_rslt_t)Cy_SCB_UART_Init(config->reg_addr, &(data->scb_config),
					     &(data->context));
	if (result != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	irq_enable(config->irq_num);
	ret = ifx_cat2_uart_configure(dev, &config->dt_cfg);

	return ret;
}

static DEVICE_API(uart, ifx_cat2_uart_driver_api) = {
	.poll_in = ifx_cat2_uart_poll_in,
	.poll_out = ifx_cat2_uart_poll_out,
	.err_check = ifx_cat2_uart_err_check,

#if CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = ifx_cat2_uart_configure,
	.config_get = ifx_cat2_uart_config_get,
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = ifx_cat2_uart_fifo_fill,
	.fifo_read = ifx_cat2_uart_fifo_read,
	.irq_tx_enable = ifx_cat2_uart_irq_tx_enable,
	.irq_tx_disable = ifx_cat2_uart_irq_tx_disable,
	.irq_tx_ready = ifx_cat2_uart_irq_tx_ready,
	.irq_rx_enable = ifx_cat2_uart_irq_rx_enable,
	.irq_rx_disable = ifx_cat2_uart_irq_rx_disable,
	.irq_tx_complete = ifx_cat2_uart_irq_tx_complete,
	.irq_rx_ready = ifx_cat2_uart_irq_rx_ready,
	.irq_err_enable = ifx_cat2_uart_irq_err_enable,
	.irq_err_disable = ifx_cat2_uart_irq_err_disable,
	.irq_is_pending = ifx_cat2_uart_irq_is_pending,
	.irq_update = ifx_cat2_uart_irq_update,
	.irq_callback_set = ifx_cat2_uart_irq_callback_set,
#endif
};

#define IRQ_INFO(n) .irq_num = DT_INST_IRQN(n), .irq_priority = DT_INST_IRQ(n, priority)

#if (CONFIG_UART_INTERRUPT_DRIVEN)
#define INTERRUPT_DRIVEN_UART_INIT(n)                                                              \
	void uart_handle_events_func_##n(void)							   \
	{											   \
		ifx_cat2_uart_irq_handler(DEVICE_DT_INST_GET(n));				   \
	}											   \
	static void ifx_cat2_uart_irq_config_func_##n(void)                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                             \
			    uart_handle_events_func_##n, DEVICE_DT_INST_GET(n), 0);		   \
	}
#define CALL_UART_IRQ_CONFIG(n) ifx_cat2_uart_irq_config_func_##n();
#else
#define INTERRUPT_DRIVEN_UART_INIT(n)
#define CALL_UART_IRQ_CONFIG(n)
#endif

#define UART_PERI_CLOCK_INIT(n)									   \
	.clock =										   \
		{										   \
			.block = IFX_CAT2_PERIPHERAL_GROUP_ADJUST(				   \
				 DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), clk_dst, 1),	   \
				 DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),			   \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, div_num),			   \
		},

#define INFINEON_CAT2_UART_INIT(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	INTERRUPT_DRIVEN_UART_INIT(n)                                                              \
	static struct ifx_cat2_uart_data ifx_cat2_uart##n##_data = {UART_PERI_CLOCK_INIT(n)};      \
	                                                                                           \
	static int ifx_cat2_uart_init##n(const struct device *dev)                                 \
	{                                                                                          \
		CALL_UART_IRQ_CONFIG(n);                                                           \
		return ifx_cat2_uart_init(dev);							   \
	}											   \
												   \
	static const cy_stc_scb_uart_config_t _uart_default_config_##n = {                         \
		.uartMode = DT_INST_PROP(n, uart_mode),                                            \
		.enableMutliProcessorMode = DT_INST_PROP(n, multi_proc_mode),                      \
		.smartCardRetryOnNack =  DT_INST_PROP(n, smart_card_ron),                          \
		.irdaInvertRx = DT_INST_PROP(n, irda_invert_rx),                                   \
		.irdaEnableLowPowerReceiver = DT_INST_PROP(n, irda_en_lowpowrx),                   \
		.oversample = DT_INST_PROP(n, oversample),                                         \
		.enableMsbFirst =  DT_INST_PROP(n, en_msb_first),                                  \
		.dataWidth = DT_INST_PROP(n, data_width),                                          \
		.parity = DT_INST_PROP(n, uart_parity),                                            \
		.stopBits =  DT_INST_PROP(n, uart_stopbits),	                 		   \
		.enableInputFilter =  DT_INST_PROP(n, en_input_filter),				   \
		.breakWidth = DT_INST_PROP(n, breakwidth),					   \
		.dropOnFrameError = DT_INST_PROP(n, drop_on_frame_err),			           \
		.dropOnParityError =  DT_INST_PROP(n, drop_on_parity_err),			   \
		.breakLevel = DT_INST_PROP(n, break_lvl),				           \
		.receiverAddress = DT_INST_PROP(n, rx_addrs),					   \
		.receiverAddressMask = DT_INST_PROP(n, rx_addrs_mask), 				   \
		.acceptAddrInFifo = DT_INST_PROP(n, accept_addr_in_fifo),			   \
		.enableCts = DT_INST_PROP(n, enable_cts),					   \
		.ctsPolarity = DT_INST_PROP(n, cts_polarity),	       				   \
		.rtsRxFifoLevel = DT_INST_PROP(n, rts_rx_fifo_lvl),				   \
		.rtsPolarity = DT_INST_PROP(n, rts_polarity),					   \
		.rxFifoTriggerLevel =  DT_INST_PROP(n, rx_fifo_trig_lvl),			   \
		.rxFifoIntEnableMask = DT_INST_PROP(n, rx_fifo_int_en_mask),			   \
		.txFifoTriggerLevel = DT_INST_PROP(n, tx_fifo_trig_lvl),			   \
		.txFifoIntEnableMask = DT_INST_PROP(n, tx_fifo_int_en_mask),			   \
	};											   \
												   \
	static struct ifx_cat2_uart_config ifx_cat2_uart##n##_cfg = {				   \
		.dt_cfg.baudrate = DT_INST_PROP(n, current_speed),				   \
		.dt_cfg.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),		   \
		.dt_cfg.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),	   \
		.dt_cfg.data_bits = DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),	   \
		.dt_cfg.flow_ctrl = DT_INST_PROP(n, hw_flow_control),				   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					   \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),					   \
		.uart_def_conf = &_uart_default_config_##n,					   \
		IRQ_INFO(n)};									   \
												   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat2_uart_init##n, NULL, &ifx_cat2_uart##n##_data,	   \
			&ifx_cat2_uart##n##_cfg, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,	   \
			&ifx_cat2_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT2_UART_INIT)
