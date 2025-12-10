/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for Infineon CAT1 MCU family.
 *
 */

#define DT_DRV_COMPAT infineon_uart

#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#include <infineon_kconfig.h>
#include <cy_device_headers.h>
#include <cy_scb_uart.h>
#include <cy_gpio.h>
#include <cy_syslib.h>
#include <cy_sysint.h>
#include <cy_sysclk.h>

#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_ifx, CONFIG_UART_LOG_LEVEL);

#define IFX_UART_OVERSAMPLE_MIN              8UL
#define IFX_UART_OVERSAMPLE_MAX              16UL
#define IFX_UART_MAX_BAUD_PERCENT_DIFFERENCE 10U

#if defined(CY_IP_MXSCB_INSTANCES)
#define _IFX_CAT1_SCB_ARRAY_SIZE (CY_IP_MXSCB_INSTANCES)
#elif defined(CY_IP_M0S8SCB_INSTANCES)
#define _IFX_CAT1_SCB_ARRAY_SIZE (CY_IP_M0S8SCB_INSTANCES)
#elif defined(CY_IP_MXS22SCB_INSTANCES)
#define _IFX_CAT1_SCB_ARRAY_SIZE (CY_IP_MXS22SCB_INSTANCES)
#endif /* CY_IP_MXSCB_INSTANCES */

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
#define IFX_UART_RX_FIFO_TRIGGER_LEVEL 7
#define IFX_UART_TX_FIFO_TRIGGER_LEVEL 0
#else
#define IFX_UART_RX_FIFO_TRIGGER_LEVEL 63UL
#define IFX_UART_TX_FIFO_TRIGGER_LEVEL 63UL
#endif /* CONFIG_SOC_FAMILY_INFINEON_PSOC4 */

#define IFX_UART_RX_INT_MASK_NONE 0UL
#define IFX_UART_TX_INT_MASK_NONE 0UL

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#include <cy_trigmux.h>

struct ifx_cat1_dma_stream_rx {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	uint8_t *buf;
	size_t buf_len;
	size_t offset;
	size_t counter;
	uint32_t timeout;
	size_t dma_transmitted_bytes;
	struct k_work_delayable timeout_work;
};

struct ifx_cat1_dma_stream_tx {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	const uint8_t *buf;
	size_t buf_len;
	size_t dma_transmitted_bytes;
	struct k_work_delayable timeout_work;
};

struct ifx_cat1_uart_async {
	const struct device *uart_dev;
	uart_callback_t cb;
	void *user_data;
	struct ifx_cat1_dma_stream_rx dma_rx;
	struct ifx_cat1_dma_stream_tx dma_tx;
	uint8_t *rx_next_buf;
	size_t rx_next_buf_len;
};

#define CURRENT_BUFFER 0
#define NEXT_BUFFER    1

#endif /* CONFIG_UART_ASYNC_API */

/* Data structure */
struct ifx_cat1_uart_data {
	struct uart_config cfg;
	struct ifx_cat1_resource_inst hw_resource;
	struct ifx_cat1_clock clock;
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	uint8_t clock_peri_group;
#endif

#if CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb; /* Interrupt Callback */
	void *irq_cb_data;                    /* Interrupt Callback Arg */
#endif
	bool cts_enabled;
	bool rts_enabled;
	cy_stc_scb_uart_context_t context;
	cy_stc_scb_uart_config_t scb_config;
	uint32_t baud_rate;
#ifdef CONFIG_UART_ASYNC_API
	struct ifx_cat1_uart_async async;
#endif
};

/* Device config structure */
struct ifx_cat1_uart_config {
	const struct pinctrl_dev_config *pcfg;
	CySCB_Type *reg_addr;
	struct uart_config dt_cfg;
	uint16_t irq_num;
	uint8_t irq_priority;
};

typedef void (*ifx_cat1_uart_event_callback_t)(void *callback_arg);

const uint8_t data_bits_lut[] = {
	[UART_CFG_DATA_BITS_5] = 5, [UART_CFG_DATA_BITS_6] = 6, [UART_CFG_DATA_BITS_7] = 7,
	[UART_CFG_DATA_BITS_8] = 8, [UART_CFG_DATA_BITS_9] = 9,
};

const uint8_t stop_bits_lut[] = {
	[UART_CFG_STOP_BITS_1] = CY_SCB_UART_STOP_BITS_1,
	[UART_CFG_STOP_BITS_2] = CY_SCB_UART_STOP_BITS_2,
};

const uint8_t parity_lut[] = {
	[UART_CFG_PARITY_NONE] = CY_SCB_UART_PARITY_NONE,
	[UART_CFG_PARITY_ODD] = CY_SCB_UART_PARITY_ODD,
	[UART_CFG_PARITY_EVEN] = CY_SCB_UART_PARITY_EVEN,
};

static inline uint32_t convert_uart_parity_z_to_cy(uint32_t parity)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		return CY_SCB_UART_PARITY_NONE;
	case UART_CFG_PARITY_ODD:
		return CY_SCB_UART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return CY_SCB_UART_PARITY_EVEN;
	default:
		return CY_SCB_UART_PARITY_NONE;
	}
}

static inline uint32_t convert_uart_stop_bits_z_to_cy(uint32_t sb)
{
	if (sb <= UART_CFG_STOP_BITS_2) {
		return stop_bits_lut[sb];
	}

	LOG_WRN("Invalid stop bits (%u), defaulting to 1 stop bit", sb);
	return CY_SCB_UART_STOP_BITS_1;
}

static inline uint32_t convert_uart_data_bits_z_to_cy(uint32_t db)
{
	if (db <= UART_CFG_DATA_BITS_9) {
		return data_bits_lut[db];
	}

	LOG_WRN("Invalid data bits (%u), defaulting to 1 bit", db);
	return 1U;
}

static inline uint32_t ifx_uart_baud_diff(uint32_t actual, uint32_t baud)
{
	return (actual > baud) ? (((actual - baud) * 100U) / baud)
			       : (((baud - actual) * 100U) / baud);
}

static inline uint32_t ifx_uart_divider(uint32_t freq, uint32_t baud, uint32_t oversample)
{
	return (freq + ((baud * oversample) / 2U)) / (baud * oversample);
}

static inline uint32_t ifx_uart_mem_width(uint32_t data_width)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	return (data_width <= CY_SCB_BYTE_WIDTH) ? CY_SCB_CTRL_MEM_WIDTH_BYTE
						 : CY_SCB_CTRL_MEM_WIDTH_HALFWORD;
#else
	return (data_width <= CY_SCB_BYTE_WIDTH) ? CY_SCB_MEM_WIDTH_BYTE
						 : CY_SCB_MEM_WIDTH_HALFWORD;
#endif
}

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define IFX_CAT1_INSTANCE_GROUP(instance, group) (((instance) << 4) | (group))
#endif

#if !defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
static uint8_t ifx_cat1_get_hfclk_for_peri_group(uint8_t peri_group)
{
#if defined(CONFIG_SOC_SERIES_PSE84)
	switch (peri_group) {
	case IFX_CAT1_INSTANCE_GROUP(0, 0):
	case IFX_CAT1_INSTANCE_GROUP(1, 4):
		return 0;
	case IFX_CAT1_INSTANCE_GROUP(0, 7):
	case IFX_CAT1_INSTANCE_GROUP(1, 0):
		return 1;
	case IFX_CAT1_INSTANCE_GROUP(0, 3):
	case IFX_CAT1_INSTANCE_GROUP(1, 2):
		return 5;
	case IFX_CAT1_INSTANCE_GROUP(0, 4):
	case IFX_CAT1_INSTANCE_GROUP(1, 3):
		return 6;
	case IFX_CAT1_INSTANCE_GROUP(1, 1):
		return 7;
	case IFX_CAT1_INSTANCE_GROUP(0, 2):
		return 9;
	case IFX_CAT1_INSTANCE_GROUP(0, 1):
	case IFX_CAT1_INSTANCE_GROUP(0, 5):
		return 10;
	case IFX_CAT1_INSTANCE_GROUP(0, 8):
		return 11;
	case IFX_CAT1_INSTANCE_GROUP(0, 6):
	case IFX_CAT1_INSTANCE_GROUP(0, 9):
		return 13;
	default:
		break;
	}
#elif defined(CONFIG_SOC_SERIES_PSC3)
	switch (peri_group) {
	case 0:
	case 2:
		return 0;
	case 1:
	case 3:
		return 1;
	case 4:
		return 2;
	case 5:
		return 3;
	case 6:
		return 4;
	default:
		break;
	}
#endif
	return -EINVAL;
}
#endif

cy_rslt_t ifx_cat1_uart_set_baud(const struct device *dev, uint32_t baudrate)
{
	cy_rslt_t status;
	struct ifx_cat1_uart_data *data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	uint8_t best_oversample = IFX_UART_OVERSAMPLE_MIN;
	uint8_t best_difference = 0xFF;
	uint32_t divider;

	uint32_t peri_frequency;

	if (data->baud_rate != baudrate) {
		data->baud_rate = baudrate;
	}

	Cy_SCB_UART_Disable(config->reg_addr, NULL);

#if defined(COMPONENT_CAT1A)
	peri_frequency = Cy_SysClk_ClkPeriGetFrequency();
#elif defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) ||                                      \
	defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	uint8_t hfclk = ifx_cat1_get_hfclk_for_peri_group(data->clock_peri_group);

	peri_frequency = Cy_SysClk_ClkHfGetFrequency(hfclk);
#else
	peri_frequency = Cy_SysClk_ClkHfGetFrequency();
#endif
	for (uint8_t i = IFX_UART_OVERSAMPLE_MIN; i < IFX_UART_OVERSAMPLE_MAX + 1; i++) {
		uint32_t tmp_divider = ((peri_frequency + ((baudrate * i) / 2))) / (baudrate * i);

		uint32_t actual_baud = (peri_frequency / (tmp_divider * i));
		uint8_t difference = ifx_uart_baud_diff(actual_baud, baudrate);

		if (difference < best_difference) {
			best_difference = difference;
			best_oversample = i;
		}
	}

	if (best_difference > IFX_UART_MAX_BAUD_PERCENT_DIFFERENCE) {
		status = -EINVAL;
	}

	data->scb_config.oversample = best_oversample;

	divider = ifx_uart_divider(peri_frequency, baudrate, best_oversample);

	en_clk_dst_t clk_idx = ifx_cat1_scb_get_clock_index(data->hw_resource.block_num);

	/* Set baud rate */
	if ((data->clock.block & 0x02) == 0) {
		status = ifx_cat1_utils_peri_pclk_set_divider(clk_idx, &(data->clock), divider - 1);
	} else {
		status = ifx_cat1_utils_peri_pclk_set_frac_divider(clk_idx, &(data->clock),
								   divider - 1, 0);
	}

	if (status < 0) {
		return status;
	}

/* Configure the UART interface */
#if (CY_IP_MXSCB_VERSION >= 2) || (CY_IP_MXS22SCB_VERSION >= 1)
	SCB_CTRL(config->reg_addr) =
		_BOOL2FLD(SCB_CTRL_ADDR_ACCEPT, data->scb_config.acceptAddrInFifo) |
		_BOOL2FLD(SCB_CTRL_MEM_WIDTH, ifx_uart_mem_width(data->scb_config.dataWidth)) |
		_VAL2FLD(SCB_CTRL_OVS, best_oversample - 1) |
		_VAL2FLD(SCB_CTRL_MODE, CY_SCB_CTRL_MODE_UART);
#else /* Older versions of the block */
	SCB_CTRL(config->reg_addr) =
		_BOOL2FLD(SCB_CTRL_ADDR_ACCEPT, data->scb_config.acceptAddrInFifo) |
		_BOOL2FLD(SCB_CTRL_BYTE_MODE, (data->scb_config.dataWidth <= CY_SCB_BYTE_WIDTH)) |
		_VAL2FLD(SCB_CTRL_OVS, best_oversample - 1) |
		_VAL2FLD(SCB_CTRL_MODE, CY_SCB_CTRL_MODE_UART);
#endif

	Cy_SCB_UART_Enable(config->reg_addr);

	return status;
}

uint32_t ifx_cat1_uart_get_num_in_tx_fifo(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;

	return Cy_SCB_GetNumInTxFifo(config->reg_addr);
}

bool ifx_cat1_uart_get_tx_active(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;

	return Cy_SCB_GetTxSrValid(config->reg_addr) ? true : false;
}

static int ifx_cat1_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct ifx_cat1_uart_config *const config = dev->config;

	uint32_t read_value = Cy_SCB_UART_Get(config->reg_addr);

	while (read_value == CY_SCB_UART_RX_NO_DATA) {
		k_sleep(K_MSEC(1));
		read_value = Cy_SCB_UART_Get(config->reg_addr);
	}
	*c = (uint8_t)read_value;

	return 0;
}

static void ifx_cat1_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct ifx_cat1_uart_config *const config = dev->config;

	while (Cy_SCB_UART_Put(config->reg_addr, c) == 0) {
		/* Wait until the character is placed in the FIFO */
	}
}

static int ifx_cat1_uart_err_check(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;

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

static int ifx_cat1_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	__ASSERT_NO_MSG(cfg != NULL);

	cy_rslt_t result;
	struct ifx_cat1_uart_data *data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	/* Store Uart Zephyr configuration (uart config) into data structure */
	data->cfg = *cfg;

	/* Configure parity, data and stop bits */
	Cy_SCB_UART_Disable(config->reg_addr, NULL);
	data->scb_config.dataWidth = convert_uart_data_bits_z_to_cy(cfg->data_bits);
	data->scb_config.stopBits = convert_uart_stop_bits_z_to_cy(cfg->stop_bits);
	data->scb_config.parity = convert_uart_parity_z_to_cy(cfg->parity);
	data->scb_config.enableCts = data->cts_enabled;

	Cy_SCB_UART_Init(config->reg_addr, &(data->scb_config), NULL);
	Cy_SCB_UART_Enable(config->reg_addr);
	/* Configure the baud rate */
	result = ifx_cat1_uart_set_baud(dev, cfg->baudrate);

	/* Enable RTS/CTS flow control */
	if ((result == CY_RSLT_SUCCESS) && cfg->flow_ctrl) {
		Cy_SCB_UART_EnableCts(config->reg_addr);
	}

	return (result == CY_RSLT_SUCCESS) ? 0 : -ENOTSUP;
};

static int ifx_cat1_uart_config_get(const struct device *dev, struct uart_config *cfg)
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

/* Fill FIFO with data */
static int ifx_cat1_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct ifx_cat1_uart_config *const config = dev->config;
	size_t tx_length;

	tx_length = Cy_SCB_UART_PutArray(config->reg_addr, (void *)tx_data, size);
	return (int)tx_length;
}

/* Read data from FIFO */
static int ifx_cat1_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct ifx_cat1_uart_config *const config = dev->config;
	size_t rx_length;

	rx_length = Cy_SCB_UART_GetArray(config->reg_addr, rx_data, size);

	return (int)rx_length;
}

void ifx_cat1_uart_enable_event(const struct device *dev, uint32_t event, bool enable)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

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
		/* Omit underflow condition as the interrupt perpetually triggers
		 * Standard mode only uses OVERFLOW irq
		 */
		if (data->scb_config.uartMode == CY_SCB_UART_STANDARD) {
			tx_mask |= (CY_SCB_UART_TX_OVERFLOW | CY_SCB_UART_TRANSMIT_ERR);
		}
		/* SMARTCARD mode uses OVERFLOW, NACK, and ARB_LOST irq's */
		else if (data->scb_config.uartMode == CY_SCB_UART_SMARTCARD) {
			tx_mask |= (CY_SCB_UART_TX_OVERFLOW | CY_SCB_TX_INTR_UART_NACK |
				    CY_SCB_TX_INTR_UART_ARB_LOST | CY_SCB_UART_TRANSMIT_ERR);
		}
		/* LIN Mode only uses OVERFLOW, ARB_LOST irq's */
		else {
			tx_mask |= (CY_SCB_UART_TX_OVERFLOW | CY_SCB_TX_INTR_UART_ARB_LOST |
				    CY_SCB_UART_TRANSMIT_ERR);
		}
	}

	if (event & CY_SCB_UART_RECEIVE_NOT_EMTPY) {
		rx_mask |= CY_SCB_UART_RX_NOT_EMPTY;
	}

	if (event & CY_SCB_UART_RECEIVE_ERR_EVENT) {
		/* Omit underflow condition as the interrupt perpetually triggers. */
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

static void ifx_cat1_uart_irq_tx_enable(const struct device *dev)
{
	ifx_cat1_uart_enable_event(dev, (uint32_t)CY_SCB_UART_TRANSMIT_EMTPY, 1);
}

static void ifx_cat1_uart_irq_tx_disable(const struct device *dev)
{
	ifx_cat1_uart_enable_event(dev, (uint32_t)CY_SCB_UART_TRANSMIT_EMTPY, 0);
}

/* Check if UART TX buffer can accept a new char */
static int ifx_cat1_uart_irq_tx_ready(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;
	uint32_t mask = Cy_SCB_GetTxInterruptStatusMasked(config->reg_addr);

	return (((mask & (CY_SCB_UART_TX_NOT_FULL | SCB_INTR_TX_EMPTY_Msk)) != 0u) ? 1 : 0);
}

/* Check if UART TX block finished transmission */
static int ifx_cat1_uart_irq_tx_complete(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	return Cy_SCB_IsTxComplete(config->reg_addr) ||
	       ((data->context.txStatus & CY_SCB_UART_TRANSMIT_ACTIVE) == 0);
}

static void ifx_cat1_uart_irq_rx_enable(const struct device *dev)
{
	ifx_cat1_uart_enable_event(dev, (uint32_t)CY_SCB_UART_RECEIVE_NOT_EMTPY, 1);
}

static void ifx_cat1_uart_irq_rx_disable(const struct device *dev)
{
	ifx_cat1_uart_enable_event(dev, (uint32_t)CY_SCB_UART_RECEIVE_NOT_EMTPY, 0);
}

/* Check if UART RX buffer has a received char */
static int ifx_cat1_uart_irq_rx_ready(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;

	uint32_t number_available = Cy_SCB_UART_GetNumInRxFifo(config->reg_addr);

	if (data->context.rxRingBuf != NULL) {
		number_available +=
			Cy_SCB_UART_GetNumInRingBuffer(config->reg_addr, &(data->context));
	}

	return (number_available) ? 1 : 0;
}

static void ifx_cat1_uart_irq_err_enable(const struct device *dev)
{
	ifx_cat1_uart_enable_event(dev,
				   ((uint32_t)CY_SCB_UART_TRANSMIT_ERR_EVENT |
				    (uint32_t)CY_SCB_UART_RECEIVE_ERR_EVENT),
				   1);
}

static void ifx_cat1_uart_irq_err_disable(const struct device *dev)
{
	ifx_cat1_uart_enable_event(dev,
				   ((uint32_t)CY_SCB_UART_TRANSMIT_ERR_EVENT |
				    (uint32_t)CY_SCB_UART_RECEIVE_ERR_EVENT),
				   0);
}

static int ifx_cat1_uart_irq_is_pending(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;
	uint32_t intcause = Cy_SCB_GetInterruptCause(config->reg_addr);

	return (int)(intcause & (CY_SCB_TX_INTR | CY_SCB_RX_INTR));
}

/* Start processing interrupts in ISR.
 * This function should be called the first thing in the ISR. Calling
 * uart_irq_rx_ready(), uart_irq_tx_ready(), uart_irq_tx_complete()
 * allowed only after this.
 */
static int ifx_cat1_uart_irq_update(const struct device *dev)
{
	const struct ifx_cat1_uart_config *const config = dev->config;
	uint32_t rx_intr_pending = ((ifx_cat1_uart_irq_is_pending(dev) & CY_SCB_RX_INTR));
	uint32_t num_in_rx_fifo = Cy_SCB_UART_GetNumInRxFifo(config->reg_addr);

	if (rx_intr_pending != 0u && num_in_rx_fifo == 0u) {
		return 0;
	}

	return 1;
}

static void ifx_cat1_uart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct ifx_cat1_uart_data *data = dev->data;

	/* Store user callback info */
	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

static void ifx_cat1_uart_irq_handler(const struct device *dev)
{
	/*
	 * This function clears the interrupt and makes a callback.
	 * It does not handle events.
	 */
	const struct ifx_cat1_uart_config *const config = dev->config;
	struct ifx_cat1_uart_data *const data = dev->data;

	/* Clear all interrupts that could have been configured */
	CySCB_Type *base = config->reg_addr;
	uint32_t locRxErr = (CY_SCB_UART_RECEIVE_ERR & Cy_SCB_GetRxInterruptStatusMasked(base));
	uint32_t locTxErr = (CY_SCB_UART_TRANSMIT_ERR & Cy_SCB_GetTxInterruptStatusMasked(base));
	uint32_t rx_clear = locRxErr | CY_SCB_UART_RX_NOT_EMPTY;
	uint32_t tx_clear = locTxErr | CY_SCB_UART_TX_EMPTY | CY_SCB_UART_TX_OVERFLOW |
			    CY_SCB_TX_INTR_UART_NACK | CY_SCB_TX_INTR_UART_ARB_LOST;

	Cy_SCB_ClearRxInterrupt(base, rx_clear);
	Cy_SCB_ClearTxInterrupt(base, tx_clear);

	/* Call the callback with the callback data.
	 * This does not guarantee a separate callback per event.
	 */
	if (data->irq_cb != NULL) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/* Default Counter configuration structure */
static const cy_stc_scb_uart_config_t _uart_default_config = {
	.uartMode = CY_SCB_UART_STANDARD,
#if defined(CONFIG_SOC_SERIES_PSE84)
	.enableMultiProcessorMode = false,
#else
	.enableMutliProcessorMode = false,
#endif
	.smartCardRetryOnNack = false,
	.irdaInvertRx = false,
	.irdaEnableLowPowerReceiver = false,
#if ((defined(CY_IP_MXSCB_VERSION)) && (CY_IP_MXSCB_VERSION >= 4))
	.halfDuplexMode = false,
#endif
	.oversample = 8,
	.enableMsbFirst = false,
	.dataWidth = 8UL,
	.parity = CY_SCB_UART_PARITY_NONE,
	.stopBits = CY_SCB_UART_STOP_BITS_1,
	.enableInputFilter = false,
	.breakWidth = 11UL,
	.dropOnFrameError = false,
	.dropOnParityError = false,
#if !defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	.breaklevel = false,
#else
	.breakLevel = false,
#endif
	.receiverAddress = 0x0UL,
	.receiverAddressMask = 0x0UL,
	.acceptAddrInFifo = false,
	.enableCts = false,
	.ctsPolarity = CY_SCB_UART_ACTIVE_LOW,
	.rtsRxFifoLevel = 0UL,
	.rtsPolarity = CY_SCB_UART_ACTIVE_LOW,
	.rxFifoTriggerLevel = IFX_UART_RX_FIFO_TRIGGER_LEVEL,
	.rxFifoIntEnableMask = IFX_UART_RX_INT_MASK_NONE,
	.txFifoTriggerLevel = IFX_UART_TX_FIFO_TRIGGER_LEVEL,
	.txFifoIntEnableMask = IFX_UART_TX_INT_MASK_NONE,
};
#ifdef CONFIG_UART_ASYNC_API

static int ifx_cat1_uart_async_callback_set(const struct device *dev, uart_callback_t callback,
					    void *user_data)
{
	struct ifx_cat1_uart_data *const data = dev->data;

	data->async.cb = callback;
	data->async.user_data = user_data;

	return 0;
}

/* Async DMA helper */
static int ifx_cat1_uart_async_dma_config_buffer(const struct device *dev, bool tx)
{
	int ret;
	struct ifx_cat1_uart_data *const data = dev->data;

	if (tx) {
		data->async.dma_tx.blk_cfg.source_address = (uint32_t)data->async.dma_tx.buf;
		data->async.dma_tx.blk_cfg.block_size = data->async.dma_tx.buf_len;

		ret = dma_config(data->async.dma_tx.dma_dev, data->async.dma_tx.dma_channel,
				 &data->async.dma_tx.dma_cfg);

		if (!ret) {
			ret = dma_start(data->async.dma_tx.dma_dev, data->async.dma_tx.dma_channel);
		}
	} else {
		data->async.dma_rx.blk_cfg.dest_address = (uint32_t)data->async.dma_rx.buf;
		data->async.dma_rx.blk_cfg.block_size = data->async.dma_rx.buf_len;

		ret = dma_config(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel,
				 &data->async.dma_rx.dma_cfg);

		if (!ret) {
			ret = dma_start(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel);
		}
	}

	return ret;
}

static int ifx_cat1_uart_async_tx(const struct device *dev, const uint8_t *tx_data,
				  size_t tx_data_size, int32_t timeout)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct device *dma_dev = data->async.dma_tx.dma_dev;
	int err;

	if (dma_dev == NULL) {
		return -ENODEV;
	}

	if (tx_data == NULL || tx_data_size == 0) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	/* Store information about data buffer need to send */
	data->async.dma_tx.buf = tx_data;
	data->async.dma_tx.buf_len = tx_data_size;
	data->async.dma_tx.blk_cfg.block_size = 0;
	data->async.dma_tx.dma_transmitted_bytes = 0;

	/* Configure dma to transfer */
	err = ifx_cat1_uart_async_dma_config_buffer(dev, true);
	if (err) {
		LOG_ERR("Error Tx DMA configure (%d)", err);
		goto exit;
	}

	/* Configure timeout */
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(&data->async.dma_tx.timeout_work, K_USEC(timeout));
	}

exit:
	irq_unlock(key);
	return err;
}

static int ifx_cat1_uart_async_tx_abort(const struct device *dev)
{
	struct ifx_cat1_uart_data *data = dev->data;
	struct uart_event evt = {0};
	struct dma_status stat;
	int err = 0;

	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.dma_tx.timeout_work);

	err = dma_stop(data->async.dma_tx.dma_dev, data->async.dma_tx.dma_channel);
	if (err) {
		LOG_ERR("Error stopping Tx DMA (%d)", err);
		goto unlock;
	}

	err = dma_get_status(data->async.dma_tx.dma_dev, data->async.dma_tx.dma_channel, &stat);
	if (err) {
		LOG_ERR("Error stopping Tx DMA (%d)", err);
		goto unlock;
	}

	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = data->async.dma_tx.buf;
	evt.data.tx.len = 0;

	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

unlock:
	irq_unlock(key);
	return err;
}

/* This callback is called when the dma's tx is done */
static void dma_callback_tx_done(const struct device *dma_dev, void *arg, uint32_t channel,
				 int status)
{
	const struct device *uart_dev = (void *)arg;
	struct ifx_cat1_uart_data *const data = uart_dev->data;

	unsigned int key = irq_lock();

	if (status == 0) {

		k_work_cancel_delayable(&data->async.dma_tx.timeout_work);
		dma_stop(data->async.dma_tx.dma_dev, data->async.dma_tx.dma_channel);

		struct uart_event evt = {.type = UART_TX_DONE,
					 .data.tx.buf = data->async.dma_tx.buf,
					 .data.tx.len = data->async.dma_tx.buf_len};

		data->async.dma_tx.buf = NULL;
		data->async.dma_tx.buf_len = 0;

		if (data->async.cb) {
			data->async.cb(uart_dev, &evt, data->async.user_data);
		}

	} else {
		/* DMA error */
		dma_stop(data->async.dma_tx.dma_dev, data->async.dma_tx.dma_channel);
	}

	irq_unlock(key);
}

static void ifx_cat1_uart_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ifx_cat1_dma_stream_tx *dma_tx =
		CONTAINER_OF(dwork, struct ifx_cat1_dma_stream_tx, timeout_work);
	struct ifx_cat1_uart_async *async =
		CONTAINER_OF(dma_tx, struct ifx_cat1_uart_async, dma_tx);

	(void)ifx_cat1_uart_async_tx_abort(async->uart_dev);
}

static inline void async_evt_rx_rdy(struct ifx_cat1_uart_data *data)
{
	struct uart_event event = {.type = UART_RX_RDY,
				   .data.rx.buf = data->async.dma_rx.buf,
				   .data.rx.len =
					   data->async.dma_rx.counter - data->async.dma_rx.offset,
				   .data.rx.offset = data->async.dma_rx.offset};

	data->async.dma_rx.offset = data->async.dma_rx.counter;

	if (event.data.rx.len > 0 && data->async.cb) {
		data->async.cb(data->async.uart_dev, &event, data->async.user_data);
	}
}

static inline void async_evt_rx_buf_request(struct ifx_cat1_uart_data *data)
{
	struct uart_event evt = {.type = UART_RX_BUF_REQUEST};

	if (data->async.cb) {
		data->async.cb(data->async.uart_dev, &evt, data->async.user_data);
	}
}

static inline void async_evt_rx_release_buffer(struct ifx_cat1_uart_data *data, int buffer_type)
{
	struct uart_event event = {.type = UART_RX_BUF_RELEASED};

	if (buffer_type == NEXT_BUFFER && !data->async.rx_next_buf) {
		return;
	}

	if (buffer_type == CURRENT_BUFFER && !data->async.dma_rx.buf) {
		return;
	}

	if (buffer_type == NEXT_BUFFER) {
		event.data.rx_buf.buf = data->async.rx_next_buf;
		data->async.rx_next_buf = NULL;
		data->async.rx_next_buf_len = 0;
	} else {
		event.data.rx_buf.buf = data->async.dma_rx.buf;
		data->async.dma_rx.buf = NULL;
		data->async.dma_rx.buf_len = 0;
	}

	if (data->async.cb) {
		data->async.cb(data->async.uart_dev, &event, data->async.user_data);
	}
}

static inline void async_evt_rx_disabled(struct ifx_cat1_uart_data *data)
{
	struct uart_event event = {.type = UART_RX_DISABLED};

	data->async.dma_rx.buf = NULL;
	data->async.dma_rx.buf_len = 0;
	data->async.dma_rx.offset = 0;
	data->async.dma_rx.counter = 0;

	if (data->async.cb) {
		data->async.cb(data->async.uart_dev, &event, data->async.user_data);
	}
}

static inline void async_evt_rx_stopped(struct ifx_cat1_uart_data *data,
					enum uart_rx_stop_reason reason)
{
	struct uart_event event = {.type = UART_RX_STOPPED, .data.rx_stop.reason = reason};
	struct uart_event_rx *rx = &event.data.rx_stop.data;
	struct dma_status stat;

	if (data->async.dma_rx.buf_len == 0 || data->async.cb == NULL) {
		return;
	}

	rx->buf = data->async.dma_rx.buf;

	if (dma_get_status(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel, &stat) ==
	    0) {
		data->async.dma_rx.counter = data->async.dma_rx.buf_len - stat.pending_length;
	}
	rx->len = data->async.dma_rx.counter - data->async.dma_rx.offset;
	rx->offset = data->async.dma_rx.counter;

	data->async.cb(data->async.uart_dev, &event, data->async.user_data);
}

static int ifx_cat1_uart_async_rx_enable(const struct device *dev, uint8_t *rx_data,
					 size_t rx_data_size, int32_t timeout)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	struct dma_status dma_status = {0};
	int err = 0;

	if (data->async.dma_rx.dma_dev == NULL) {
		return -ENODEV;
	}

	err = dma_get_status(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel,
			     &dma_status);
	if (err) {
		return err;
	}

	if (dma_status.busy) {
		return -EBUSY;
	}

	unsigned int key = irq_lock();

	if (data->async.dma_rx.buf_len != 0) {
		err = -ENODEV;
		goto unlock;
	}

	/* Store information about data buffer need to send */
	data->async.dma_rx.buf = rx_data;
	data->async.dma_rx.buf_len = rx_data_size;
	data->async.dma_rx.blk_cfg.block_size = 0;
	data->async.dma_rx.dma_transmitted_bytes = 0;
	data->async.dma_rx.timeout = timeout;

	/* Request buffers before enabling rx */
	async_evt_rx_buf_request(data);

	/* Configure dma to transfer */
	err = ifx_cat1_uart_async_dma_config_buffer(dev, false);
	if (err) {
		LOG_ERR("Error Rx DMA configure (%d)", err);
		goto unlock;
	}

	/* Configure timeout */
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(&data->async.dma_rx.timeout_work, K_USEC(timeout));
	}

unlock:
	irq_unlock(key);
	return err;
}

/* This callback is called when the dma's rx is ready */
static void dma_callback_rx_rdy(const struct device *dma_dev, void *arg, uint32_t channel,
				int status)
{
	const struct device *uart_dev = (void *)arg;
	struct ifx_cat1_uart_data *const data = uart_dev->data;

	unsigned int key = irq_lock();

	if (status == 0) {
		/* All data are sent, call user callback */
		k_work_cancel_delayable(&data->async.dma_rx.timeout_work);
		data->async.dma_rx.counter = data->async.dma_rx.buf_len;

		async_evt_rx_rdy(data);
		async_evt_rx_release_buffer(data, CURRENT_BUFFER);

		data->async.dma_rx.buf = NULL;
		data->async.dma_rx.buf_len = 0;
		data->async.dma_rx.blk_cfg.block_size = 0;
		data->async.dma_rx.dma_transmitted_bytes = 0;

		if (!data->async.rx_next_buf) {
			dma_stop(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel);
			async_evt_rx_disabled(data);
			goto unlock;
		}

		data->async.dma_rx.buf = data->async.rx_next_buf;
		data->async.dma_rx.buf_len = data->async.rx_next_buf_len;
		data->async.dma_rx.offset = 0;
		data->async.dma_rx.counter = 0;
		data->async.rx_next_buf = NULL;
		data->async.rx_next_buf_len = 0;

		ifx_cat1_uart_async_dma_config_buffer(uart_dev, false);

		async_evt_rx_buf_request(data);

		if ((data->async.dma_rx.timeout != SYS_FOREVER_US) &&
		    (data->async.dma_rx.timeout != 0)) {
			k_work_reschedule(&data->async.dma_rx.timeout_work,
					  K_USEC(data->async.dma_rx.timeout));
		}

	} else {
		/* DMA error */
		dma_stop(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel);

		async_evt_rx_stopped(data, UART_ERROR_OVERRUN);
		async_evt_rx_release_buffer(data, CURRENT_BUFFER);
		async_evt_rx_release_buffer(data, NEXT_BUFFER);
		async_evt_rx_disabled(data);
		goto unlock;
	}

unlock:
	irq_unlock(key);
}

static void ifx_cat1_uart_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ifx_cat1_dma_stream_rx *dma_rx =
		CONTAINER_OF(dwork, struct ifx_cat1_dma_stream_rx, timeout_work);
	struct ifx_cat1_uart_async *async =
		CONTAINER_OF(dma_rx, struct ifx_cat1_uart_async, dma_rx);
	struct ifx_cat1_uart_data *data = CONTAINER_OF(async, struct ifx_cat1_uart_data, async);
	struct dma_status stat;

	unsigned int key = irq_lock();

	if (dma_rx->buf_len == 0) {
		irq_unlock(key);
		return;
	}

	if (dma_get_status(dma_rx->dma_dev, dma_rx->dma_channel, &stat) == 0) {
		size_t rx_rcv_len = dma_rx->buf_len - stat.pending_length;

		if ((rx_rcv_len > 0) && (rx_rcv_len == dma_rx->counter)) {
			dma_rx->counter = rx_rcv_len;
			async_evt_rx_rdy(data);
		} else {
			dma_rx->counter = rx_rcv_len;
		}
	}

	if ((dma_rx->timeout != SYS_FOREVER_US) && (dma_rx->timeout != 0)) {
		k_work_reschedule(&dma_rx->timeout_work, K_USEC(dma_rx->timeout));
	}

	irq_unlock(key);
}

static int ifx_cat1_uart_async_rx_disable(const struct device *dev)
{
	struct ifx_cat1_uart_data *data = dev->data;
	struct dma_status stat = {0};
	unsigned int key;

	key = irq_lock();

	k_work_cancel_delayable(&data->async.dma_rx.timeout_work);

	if (data->async.dma_rx.buf_len == 0) {
		__ASSERT_NO_MSG(data->async.dma_rx.buf == NULL);
		irq_unlock(key);
		return -EINVAL;
	}

	dma_stop(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel);

	if (dma_get_status(data->async.dma_rx.dma_dev, data->async.dma_rx.dma_channel, &stat) ==
	    0) {
		size_t rx_rcv_len = data->async.dma_rx.buf_len - stat.pending_length;

		if (rx_rcv_len > data->async.dma_rx.offset) {
			data->async.dma_rx.counter = rx_rcv_len;
			async_evt_rx_rdy(data);
		}
	}

	async_evt_rx_release_buffer(data, CURRENT_BUFFER);
	async_evt_rx_release_buffer(data, NEXT_BUFFER);
	async_evt_rx_disabled(data);

	irq_unlock(key);

	return 0;
}

static int ifx_cat1_uart_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct ifx_cat1_uart_data *data = dev->data;
	unsigned int key;
	int ret = 0;

	key = irq_lock();

	if (data->async.dma_rx.buf_len == 0U) {
		ret = -EACCES;
		goto unlock;
	}

	if (data->async.rx_next_buf_len != 0U) {
		ret = -EBUSY;
		goto unlock;
	}

	data->async.rx_next_buf = buf;
	data->async.rx_next_buf_len = len;

unlock:
	irq_unlock(key);
	return ret;
}

#endif /*CONFIG_UART_ASYNC_API */

CySCB_Type *const _IFX_CAT1_SCB_BASE_ADDRESSES[_IFX_CAT1_SCB_ARRAY_SIZE] = {
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
#ifdef SCB5
	SCB5,
#endif
#ifdef SCB6
	SCB6,
#endif
#ifdef SCB7
	SCB7,
#endif
#ifdef SCB8
	SCB8,
#endif
#ifdef SCB9
	SCB9,
#endif
#ifdef SCB10
	SCB10,
#endif
#ifdef SCB11
	SCB11,
#endif
#ifdef SCB12
	SCB12,
#endif
#ifdef SCB13
	SCB13,
#endif
#ifdef SCB14
	SCB14,
#endif
#ifdef SCB15
	SCB15,
#endif
};

const uint8_t _IFX_CAT1_SCB_BASE_ADDRESS_INDEX[_IFX_CAT1_SCB_ARRAY_SIZE] = {
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
#ifdef SCB5
	5u,
#endif
#ifdef SCB6
	6u,
#endif
#ifdef SCB7
	7u,
#endif
#ifdef SCB8
	8u,
#endif
#ifdef SCB9
	9u,
#endif
#ifdef SCB10
	10u,
#endif
#ifdef SCB11
	11u,
#endif
#ifdef SCB12
	12u,
#endif
#ifdef SCB13
	13u,
#endif
#ifdef SCB14
	14u,
#endif
#ifdef SCB15
	15u,
#endif
};

int32_t ifx_cat1_uart_get_hw_block_num(CySCB_Type *reg_addr)
{
	extern const uint8_t _IFX_CAT1_SCB_BASE_ADDRESS_INDEX[_IFX_CAT1_SCB_ARRAY_SIZE];
	uint32_t i;

	for (i = 0u; i < _IFX_CAT1_SCB_ARRAY_SIZE; i++) {
		if (_IFX_CAT1_SCB_BASE_ADDRESSES[i] == reg_addr) {
			return _IFX_CAT1_SCB_BASE_ADDRESS_INDEX[i];
		}
	}

	return -1;
}

static int ifx_cat1_uart_init(const struct device *dev)
{
	struct ifx_cat1_uart_data *const data = dev->data;
	const struct ifx_cat1_uart_config *const config = dev->config;
	cy_rslt_t result;
	int ret;

	/* Dedicate SCB HW resource */
	data->hw_resource.type = IFX_RSC_SCB;
	data->hw_resource.block_num = ifx_cat1_uart_get_hw_block_num(config->reg_addr);

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	data->scb_config = _uart_default_config;
#ifdef CONFIG_UART_ASYNC_API
	data->scb_config.rxFifoTriggerLevel = 0;
	data->scb_config.txFifoTriggerLevel = 1;
#endif

	result = (cy_rslt_t)Cy_SCB_UART_Init(config->reg_addr, &(data->scb_config),
					     &(data->context));

	if (result == CY_RSLT_SUCCESS) {
		irq_enable(config->irq_num);

		Cy_SCB_UART_Enable(config->reg_addr);
	} else {
		return -ENOTSUP;
	}

#if (CONFIG_SOC_FAMILY_INFINEON_CAT1C && CONFIG_UART_INTERRUPT_DRIVEN)
	/* Enable the UART interrupt */
	enable_sys_int(config->irq_num, config->irq_priority,
		       (void (*)(const void *))(void *)ifx_cat1_uart_irq_handler, &data->obj);
#endif

	/* Perform initial Uart configuration */
	ret = ifx_cat1_uart_configure(dev, &config->dt_cfg);

#ifdef CONFIG_UART_ASYNC_API
	data->async.uart_dev = dev;
	if (data->async.dma_rx.dma_dev != NULL) {
		if (!device_is_ready(data->async.dma_rx.dma_dev)) {
			return -ENODEV;
		}
		data->async.dma_rx.blk_cfg.source_address =
			(uint32_t)(&(config->reg_addr->RX_FIFO_RD));
		data->async.dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->async.dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->async.dma_rx.dma_cfg.head_block = &data->async.dma_rx.blk_cfg;
		data->async.dma_rx.dma_cfg.user_data = (void *)dev;
		data->async.dma_rx.dma_cfg.dma_callback = dma_callback_rx_rdy;

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
		Cy_TrigMux_Connect(
			PERI_0_TRIG_IN_MUX_0_SCB_RX_TR_OUT0 + data->hw_resource.block_num,
			PERI_0_TRIG_OUT_MUX_0_PDMA0_TR_IN0 + data->async.dma_rx.dma_channel, false,
			TRIGGER_TYPE_LEVEL);
#elif defined(COMPONENT_CAT1B)
		Cy_TrigMux_Connect(TRIG_IN_MUX_0_SCB_RX0 + (3 * data->hw_resource.block_num),
				   TRIG_OUT_MUX_0_PDMA0_TR_IN0 + data->async.dma_rx.dma_channel,
				   false, TRIGGER_TYPE_LEVEL);
#endif
	}

	if (data->async.dma_tx.dma_dev != NULL) {
		if (!device_is_ready(data->async.dma_tx.dma_dev)) {
			return -ENODEV;
		}

		data->async.dma_tx.blk_cfg.dest_address =
			(uint32_t)(&(config->reg_addr->TX_FIFO_WR));
		data->async.dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->async.dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->async.dma_tx.dma_cfg.head_block = &data->async.dma_tx.blk_cfg;
		data->async.dma_tx.dma_cfg.user_data = (void *)dev;
		data->async.dma_tx.dma_cfg.dma_callback = dma_callback_tx_done;

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
		Cy_TrigMux_Connect(
			PERI_0_TRIG_IN_MUX_0_SCB_TX_TR_OUT0 + data->hw_resource.block_num,
			PERI_0_TRIG_OUT_MUX_0_PDMA0_TR_IN0 + data->async.dma_tx.dma_channel, false,
			TRIGGER_TYPE_EDGE);
#elif defined(COMPONENT_CAT1B)
		Cy_TrigMux_Connect(TRIG_IN_MUX_0_SCB_TX0 + (3 * data->hw_resource.block_num),
				   TRIG_OUT_MUX_0_PDMA0_TR_IN0 + data->async.dma_tx.dma_channel,
				   false, TRIGGER_TYPE_EDGE);
#endif
	}

	k_work_init_delayable(&data->async.dma_tx.timeout_work, ifx_cat1_uart_async_tx_timeout);
	k_work_init_delayable(&data->async.dma_rx.timeout_work, ifx_cat1_uart_async_rx_timeout);
#endif /* CONFIG_UART_ASYNC_API */

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
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	.callback_set = ifx_cat1_uart_async_callback_set,
	.tx = ifx_cat1_uart_async_tx,
	.tx_abort = ifx_cat1_uart_async_tx_abort,
	.rx_enable = ifx_cat1_uart_async_rx_enable,
	.rx_buf_rsp = ifx_cat1_uart_async_rx_buf_rsp,
	.rx_disable = ifx_cat1_uart_async_rx_disable,
#endif /*CONFIG_UART_ASYNC_API*/

};

#ifdef CONFIG_UART_ASYNC_API

#define UART_DMA_CHANNEL_INIT(index, dir, ch_dir, src_data_size, dst_data_size)                    \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                           \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                             \
	.dma_cfg = {                                                                               \
		.channel_direction = ch_dir,                                                       \
		.source_data_size = src_data_size,                                                 \
		.dest_data_size = dst_data_size,                                                   \
		.source_burst_length = 0,                                                          \
		.dest_burst_length = 0,                                                            \
		.block_count = 1,                                                                  \
		.complete_callback_en = 0,                                                         \
	},

#define UART_DMA_CHANNEL(index, dir, ch_dir, src_data_size, dst_data_size)                         \
	.async.dma_##dir = {COND_CODE_1(                                                           \
		DT_INST_DMAS_HAS_NAME(index, dir),                                                 \
		(UART_DMA_CHANNEL_INIT(index, dir, ch_dir, src_data_size, dst_data_size)),         \
		(NULL))},

#else
#define UART_DMA_CHANNEL(index, dir, ch_dir, src_data_size, dst_data_size)
#endif /* CONFIG_UART_ASYNC_API */

#if (CONFIG_SOC_FAMILY_INFINEON_CAT1C)
#define IRQ_INFO(n)                                                                                \
	.irq_num = DT_INST_PROP_BY_IDX(n, system_interrupts, SYS_INT_NUM),                         \
	.irq_priority = DT_INST_PROP_BY_IDX(n, system_interrupts, SYS_INT_PRI)
#else
#define IRQ_INFO(n) .irq_num = DT_INST_IRQN(n), .irq_priority = DT_INST_IRQ(n, priority)
#endif

#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define PERI_INFO(n) .clock_peri_group = DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),
#else
#define PERI_INFO(n)
#endif

#if (CONFIG_UART_INTERRUPT_DRIVEN)
#define INTERRUPT_DRIVEN_UART_INIT(n)                                                              \
	void uart_handle_events_func_##n(void)                                                     \
	{                                                                                          \
		ifx_cat1_uart_irq_handler(DEVICE_DT_INST_GET(n));                                  \
	}                                                                                          \
	static void ifx_cat1_uart_irq_config_func_##n(void)                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                             \
			    uart_handle_events_func_##n, DEVICE_DT_INST_GET(n), 0);                \
	}
#define CALL_UART_IRQ_CONFIG(n) ifx_cat1_uart_irq_config_func_##n();
#else
#define INTERRUPT_DRIVEN_UART_INIT(n)
#define CALL_UART_IRQ_CONFIG(n)
#endif

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define UART_PERI_CLOCK_INIT(n)                                                                    \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),         \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
	},                                                                                         \
	PERI_INFO(n)
#else
#define UART_PERI_CLOCK_INIT(n)                                                                    \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
	},                                                                                         \
	PERI_INFO(n)
#endif

#define INFINEON_CAT1_UART_INIT(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	INTERRUPT_DRIVEN_UART_INIT(n)                                                              \
	static struct ifx_cat1_uart_data ifx_cat1_uart##n##_data = {                               \
		UART_PERI_CLOCK_INIT(n) UART_DMA_CHANNEL(n, tx, MEMORY_TO_PERIPHERAL, 1, 1)        \
			UART_DMA_CHANNEL(n, rx, PERIPHERAL_TO_MEMORY, 1, 1)};                      \
                                                                                                   \
	static int ifx_cat1_uart_init##n(const struct device *dev)                                 \
	{                                                                                          \
		CALL_UART_IRQ_CONFIG(n);                                                           \
		return ifx_cat1_uart_init(dev);                                                    \
	}                                                                                          \
                                                                                                   \
	static struct ifx_cat1_uart_config ifx_cat1_uart##n##_cfg = {                              \
		.dt_cfg.baudrate = DT_INST_PROP(n, current_speed),                                 \
		.dt_cfg.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),             \
		.dt_cfg.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),       \
		.dt_cfg.data_bits = DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),       \
		.dt_cfg.flow_ctrl = DT_INST_PROP(n, hw_flow_control),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.reg_addr = (CySCB_Type *)DT_INST_REG_ADDR(n),                                     \
		IRQ_INFO(n)};                                                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_uart_init##n, NULL, &ifx_cat1_uart##n##_data,           \
			      &ifx_cat1_uart##n##_cfg, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &ifx_cat1_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_UART_INIT)
