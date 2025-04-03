/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ot_rcp.h"
#include <stdlib.h>

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_rcp);

#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>

/************************************************************************
 * RCP reception work queue
 ************************************************************************/

static K_KERNEL_STACK_DEFINE(openthread_rcp_work_q_stack,
			     CONFIG_TELINK_W91_OT_RCP_THREAD_STACK_SIZE);

static struct k_work_q openthread_rcp_work_q;

static int openthread_rcp_work_q_init(void)
{
	struct k_work_queue_config cfg = {
		.name = "rcpworkq", .no_yield = false, .essential = false};

	k_work_queue_start(&openthread_rcp_work_q, openthread_rcp_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(openthread_rcp_work_q_stack),
			   CONFIG_TELINK_W91_OT_RCP_THREAD_PRIORITY, &cfg);
	return 0;
}

SYS_INIT(openthread_rcp_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/************************************************************************
 * RCP internal functions
 ************************************************************************/

static void openthread_rcp_reception_isr(const struct device *dev, void *user_data)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)user_data;

	if (!uart_irq_update(dev)) {
		return;
	}
	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	k_work_submit_to_queue(&openthread_rcp_work_q, &ot_rcp->work);
}

static void openthread_rcp_reception_work(struct k_work *item)
{
	struct openthread_rcp_data *ot_rcp = CONTAINER_OF(item, struct openthread_rcp_data, work);

	while (uart_irq_rx_ready(ot_rcp->uart)) {
		uint8_t data[CONFIG_TELINK_W91_OT_RCP_BUFFER_SIZE];
		int rx_result = uart_fifo_read(ot_rcp->uart, data, sizeof(data));

		if (rx_result < 0) {
			LOG_ERR("rcp uart reception error");
			break;
		}
		for (size_t i = 0; i < (size_t)rx_result; i++) {
			hdlc_coder_inp_poll(&ot_rcp->hdlc, data[i]);
		}
	}
}

static void openthread_rcp_spinel_transmission(uint8_t bt, const void *ctx)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

	hdlc_coder_out_poll(&ot_rcp->hdlc, bt);
}

static void openthread_rcp_hdlc_transmission(uint8_t bt, const void *ctx)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

	if (ot_rcp->tx_buffer.data_size == sizeof(ot_rcp->tx_data)) {
		if (uart_fifo_fill(ot_rcp->uart, ot_rcp->tx_buffer.data,
				   ot_rcp->tx_buffer.data_size) != ot_rcp->tx_buffer.data_size) {
			LOG_ERR("rcp uart transmission error");
		}
		ot_rcp->tx_buffer.data_size = 0;
	}
	ot_rcp->tx_buffer.data[ot_rcp->tx_buffer.data_size] = bt;
	ot_rcp->tx_buffer.data_size++;
}

static void openthread_rcp_reception_byte(uint8_t bt, const void *ctx)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

	if (!ot_rcp->spinel_rx_buffer.data) {
		ot_rcp->spinel_rx_buffer.data = malloc(CONFIG_TELINK_W91_OT_SPINEL_RX_BUFFER_SIZE);
		ot_rcp->spinel_rx_buffer.data_size = 0;
	}

	if (ot_rcp->spinel_rx_buffer.data) {
		if (ot_rcp->spinel_rx_buffer.data_size <
		    CONFIG_TELINK_W91_OT_SPINEL_RX_BUFFER_SIZE) {
			ot_rcp->spinel_rx_buffer.data[ot_rcp->spinel_rx_buffer.data_size] = bt;
			ot_rcp->spinel_rx_buffer.data_size++;
		} else {
			LOG_ERR("spinel rx buffer overflows");
		}
	} else {
		LOG_ERR("spinel can't allocate rx buffer");
	}
}

static void openthread_rcp_reception_done(bool data_valid, const void *ctx)
{
	if (data_valid) {
		struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

		if (ot_rcp->spinel_rx_buffer.data) {
			struct spinel_frame_data frame;

			ot_rcp->spinel_rx_buffer.data_size -= HDLC_CODER_LENGTH_CRC;
			if (spinel_drv_check_receive_frame(
				    &ot_rcp->spinel_drv, ot_rcp->spinel_rx_buffer.data,
				    ot_rcp->spinel_rx_buffer.data_size, &frame)) {
				if (ot_rcp->reception) {
					ot_rcp->reception(&frame, ot_rcp->ctx);
				}
				free(ot_rcp->spinel_rx_buffer.data);
			} else {
				k_msgq_put(&ot_rcp->spinel_msgq, &ot_rcp->spinel_rx_buffer,
					   K_FOREVER);
			}
			ot_rcp->spinel_rx_buffer.data = NULL;
		}
	}
}

/************************************************************************
 * RCP interface functions
 ************************************************************************/

int openthread_rcp_init(struct openthread_rcp_data *ot_rcp, const struct device *uart)
{
	int result = 0;

	do {
		if (!device_is_ready(uart)) {
			LOG_ERR("spinel serial not ready");
			result = -EIO;
			break;
		}

		ot_rcp->uart = uart;
		k_work_init(&ot_rcp->work, openthread_rcp_reception_work);
		ot_rcp->tx_buffer.data = ot_rcp->tx_data;
		ot_rcp->tx_buffer.data_size = 0;
		hdlc_coder_init(&ot_rcp->hdlc, ot_rcp);
		hdlc_coder_out_data_set(&ot_rcp->hdlc, openthread_rcp_hdlc_transmission);
		hdlc_coder_inp_data_set(&ot_rcp->hdlc, openthread_rcp_reception_byte);
		hdlc_coder_inp_finish_set(&ot_rcp->hdlc, openthread_rcp_reception_done);
		ot_rcp->spinel_rx_buffer.data = NULL;
		k_msgq_init(&ot_rcp->spinel_msgq, (char *)&ot_rcp->spinel_msgq_buffer,
			    sizeof(struct openthread_rcp_buffer),
			    ARRAY_SIZE(ot_rcp->spinel_msgq_buffer));
		spinel_drv_init(&ot_rcp->spinel_drv, 0);
		ot_rcp->reception = NULL;
		ot_rcp->ctx = NULL;

		if (uart_irq_callback_user_data_set(ot_rcp->uart, openthread_rcp_reception_isr,
						    ot_rcp)) {
			LOG_ERR("can't set serial isr");
			result = -EIO;
			break;
		}
		uart_irq_rx_enable(ot_rcp->uart);
		uart_irq_tx_enable(ot_rcp->uart);
	} while (0);

	return result;
}

void openthread_rcp_reception_set(struct openthread_rcp_data *ot_rcp,
				  openthread_rcp_reception reception, const void *ctx)
{
	ot_rcp->reception = reception;
	ot_rcp->ctx = ctx;
}

int openthread_rcp_deinit(struct openthread_rcp_data *ot_rcp)
{
	int result = 0;

	do {
		uart_irq_tx_disable(ot_rcp->uart);
		uart_irq_rx_disable(ot_rcp->uart);
		if (uart_irq_callback_user_data_set(ot_rcp->uart, NULL, NULL)) {
			LOG_ERR("can't reset serial isr");
			result = -EIO;
			break;
		}

		struct k_work_sync work_sync;

		(void)k_work_cancel_sync(&ot_rcp->work, &work_sync);
		k_msgq_purge(&ot_rcp->spinel_msgq);
	} while (0);

	return result;
}

static int openthread_rcp_process_start(struct openthread_rcp_data *ot_rcp, int encode_result,
					k_timepoint_t *start_time)
{
	if (encode_result < 0) {
		LOG_ERR("spinel encoding error");
	}
	hdlc_coder_out_finish(&ot_rcp->hdlc, encode_result >= 0);
	int result =
		uart_fifo_fill(ot_rcp->uart, ot_rcp->tx_buffer.data, ot_rcp->tx_buffer.data_size);

	if (result != ot_rcp->tx_buffer.data_size) {
		LOG_ERR("rcp uart transmission error");
		result = result < 0 ? result : -EIO;
	}
	ot_rcp->tx_buffer.data_size = 0;
	if (encode_result < 0) {
		result = encode_result;
	}
	if (result >= 0) {
		*start_time =
			sys_timepoint_calc(K_MSEC(CONFIG_TELINK_W91_OT_SPINEL_RESPONSE_TIMEOUT_MS));
	}

	return result;
}

static int openthread_rcp_process_continue(struct openthread_rcp_data *ot_rcp,
					   k_timepoint_t start_time,
					   struct openthread_rcp_buffer *data_income)
{
	int result = -ETIMEDOUT;

	if (!k_msgq_get(&ot_rcp->spinel_msgq, data_income, sys_timepoint_timeout(start_time))) {
		result = data_income->data_size;
	}
	if (result == -ETIMEDOUT) {
		LOG_ERR("spinel response timeout");
	}

	return result;
}

int openthread_rcp_reset(struct openthread_rcp_data *ot_rcp)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_reset(&ot_rcp->spinel_drv, openthread_rcp_spinel_transmission,
					   ot_rcp, SPINEL_RESET_STACK);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_reset(&ot_rcp->spinel_drv, response.data,
						   response.data_size)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_ieee_eui64(struct openthread_rcp_data *ot_rcp, uint8_t ieee_eui64[8])
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_get_ieee_eui64(&ot_rcp->spinel_drv,
						    openthread_rcp_spinel_transmission, ot_rcp);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_get_ieee_eui64(&ot_rcp->spinel_drv, response.data,
							    response.data_size, ieee_eui64)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_capabilities(struct openthread_rcp_data *ot_rcp,
				enum ieee802154_hw_caps *radio_caps)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_get_capabilities(&ot_rcp->spinel_drv,
						      openthread_rcp_spinel_transmission, ot_rcp);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_get_capabilities(&ot_rcp->spinel_drv, response.data,
							      response.data_size, radio_caps)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_enable_src_match(struct openthread_rcp_data *ot_rcp, bool enable)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_enable_src_match(
		&ot_rcp->spinel_drv, openthread_rcp_spinel_transmission, ot_rcp, enable);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_enable_src_match(&ot_rcp->spinel_drv, response.data,
							      response.data_size, enable)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_ack_fpb(struct openthread_rcp_data *ot_rcp, uint16_t addr, bool enable)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_ack_fpb(
		&ot_rcp->spinel_drv, openthread_rcp_spinel_transmission, ot_rcp, addr, enable);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_ack_fpb(&ot_rcp->spinel_drv, response.data,
						     response.data_size, addr, enable)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_ack_fpb_ext(struct openthread_rcp_data *ot_rcp, uint8_t addr[8], bool enable)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_ack_fpb_ext(
		&ot_rcp->spinel_drv, openthread_rcp_spinel_transmission, ot_rcp, addr, enable);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_ack_fpb_ext(&ot_rcp->spinel_drv, response.data,
							 response.data_size, addr, enable)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

static int openthread_rcp_ack_fpb_short_clear(struct openthread_rcp_data *ot_rcp)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_ack_fpb_clear(&ot_rcp->spinel_drv,
						   openthread_rcp_spinel_transmission, ot_rcp);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_ack_fpb_clear(&ot_rcp->spinel_drv, response.data,
							   response.data_size)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

static int openthread_rcp_ack_fpb_ext_clear(struct openthread_rcp_data *ot_rcp)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_ack_fpb_ext_clear(&ot_rcp->spinel_drv,
						       openthread_rcp_spinel_transmission, ot_rcp);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_ack_fpb_ext_clear(&ot_rcp->spinel_drv, response.data,
							       response.data_size)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_ack_fpb_clear(struct openthread_rcp_data *ot_rcp)
{
	int result = openthread_rcp_ack_fpb_short_clear(ot_rcp);

	if (!result) {
		result = openthread_rcp_ack_fpb_ext_clear(ot_rcp);
	}
	return result;
}

int openthread_rcp_mac_frame_counter(struct openthread_rcp_data *ot_rcp, uint32_t frame_counter,
				     bool set_if_larger)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_mac_frame_counter(&ot_rcp->spinel_drv,
						       openthread_rcp_spinel_transmission, ot_rcp,
						       frame_counter, set_if_larger);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_mac_frame_counter(&ot_rcp->spinel_drv, response.data,
							       response.data_size, frame_counter)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_panid(struct openthread_rcp_data *ot_rcp, uint16_t pan_id)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_panid(&ot_rcp->spinel_drv, openthread_rcp_spinel_transmission,
					   ot_rcp, pan_id);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_panid(&ot_rcp->spinel_drv, response.data,
						   response.data_size, pan_id)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_short_addr(struct openthread_rcp_data *ot_rcp, uint16_t addr)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_short_addr(&ot_rcp->spinel_drv,
						openthread_rcp_spinel_transmission, ot_rcp, addr);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_short_addr(&ot_rcp->spinel_drv, response.data,
							response.data_size, addr)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_ext_addr(struct openthread_rcp_data *ot_rcp, uint8_t addr[8])
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_ext_addr(&ot_rcp->spinel_drv,
					      openthread_rcp_spinel_transmission, ot_rcp, addr);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_ext_addr(&ot_rcp->spinel_drv, response.data,
						      response.data_size, addr)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_tx_power(struct openthread_rcp_data *ot_rcp, int8_t pwr_dbm)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_tx_power(&ot_rcp->spinel_drv,
					      openthread_rcp_spinel_transmission, ot_rcp, pwr_dbm);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_tx_power(&ot_rcp->spinel_drv, response.data,
						      response.data_size, pwr_dbm)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_enable(struct openthread_rcp_data *ot_rcp, bool enable)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_rcp_enable(&ot_rcp->spinel_drv,
						openthread_rcp_spinel_transmission, ot_rcp, enable);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_rcp_enable(&ot_rcp->spinel_drv, response.data,
							response.data_size, enable)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_receive_enable(struct openthread_rcp_data *ot_rcp, bool enable)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_receive_enable(
		&ot_rcp->spinel_drv, openthread_rcp_spinel_transmission, ot_rcp, enable);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_receive_enable(&ot_rcp->spinel_drv, response.data,
							    response.data_size, enable)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_channel(struct openthread_rcp_data *ot_rcp, uint8_t channel)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_channel(&ot_rcp->spinel_drv,
					     openthread_rcp_spinel_transmission, ot_rcp, channel);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_channel(&ot_rcp->spinel_drv, response.data,
						     response.data_size, channel)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_transmit(struct openthread_rcp_data *ot_rcp, struct spinel_frame_data *frame)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_transmit_frame(
		&ot_rcp->spinel_drv, openthread_rcp_spinel_transmission, ot_rcp, frame);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_transmit_frame(&ot_rcp->spinel_drv, response.data,
							    response.data_size, frame)) {
				processed = true;
				result = 0;
				if (frame->data && frame->data_length) {
					void *p = malloc(frame->data_length);

					if (p) {
						memcpy(p, frame->data, frame->data_length);
						frame->data = p;
					} else {
						frame->data = NULL;
						frame->data_length = 0;
						LOG_ERR("spinel can't allocate ack frame data");
					}
				}
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}

int openthread_rcp_link_metrics(struct openthread_rcp_data *ot_rcp, uint16_t short_addr,
				const uint8_t ext_addr[8], struct spinel_link_metrics link_metrics)
{
	k_timepoint_t start_tp;
	bool processed = false;
	int result = spinel_drv_send_link_metrics(&ot_rcp->spinel_drv,
						  openthread_rcp_spinel_transmission, ot_rcp,
						  short_addr, ext_addr, link_metrics);

	result = openthread_rcp_process_start(ot_rcp, result, &start_tp);
	while (result >= 0 && !processed) {
		struct openthread_rcp_buffer response;

		result = openthread_rcp_process_continue(ot_rcp, start_tp, &response);
		if (result >= 0) {
			if (spinel_drv_check_link_metrics(&ot_rcp->spinel_drv, response.data,
							  response.data_size)) {
				processed = true;
				result = 0;
			} else {
				LOG_HEXDUMP_WRN(response.data, response.data_size,
						"trash rx @ " STRINGIFY(__LINE__));
			}
			free(response.data);
		}
	}

	return result;
}
