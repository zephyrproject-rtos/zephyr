/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_sdhc_slot

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <esp_clk_tree.h>
#include <hal/sdmmc_ll.h>
#include <esp_intr_alloc.h>
#include <esp_timer.h>
#include <hal/gpio_hal.h>
#include <hal/rtc_io_hal.h>
#include <soc/sdmmc_reg.h>
#include <esp_memory_utils.h>

#include "sdhc_esp32.h"

LOG_MODULE_REGISTER(sdhc, CONFIG_SDHC_LOG_LEVEL);

#define SDMMC_SLOT_WIDTH_DEFAULT 1

#define SDMMC_HOST_CLOCK_UPDATE_CMD_TIMEOUT_US 1000 * 1000
#define SDMMC_HOST_RESET_TIMEOUT_US            5000 * 1000
#define SDMMC_HOST_START_CMD_TIMEOUT_US        1000 * 1000
#define SDMMC_HOST_WAIT_EVENT_TIMEOUT_US       1000 * 1000

#define SDMMC_EVENT_QUEUE_LENGTH 32

#define SDMMC_TIMEOUT_MAX 0xFFFFFFFF

/* Number of DMA descriptors used for transfer.
 * Increasing this value above 4 doesn't improve performance for the usual case
 * of SD memory cards (most data transfers are multiples of 512 bytes).
 */
#define SDMMC_DMA_DESC_CNT 4

/* mask for card current state */
#define MMC_R1_CURRENT_STATE(resp) (((resp)[0] >> 9) & 0xf)

struct sdhc_esp32_config {

	int slot;
	const sdmmc_dev_t *sdio_hw;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	const struct gpio_dt_spec pwr_gpio;
	/*
	 * Pins below are only defined for ESP32. For SoC's with GPIO matrix feature
	 * please use pinctrl for pin configuration.
	 */
	const int clk_pin;
	const int cmd_pin;
	const int d0_pin;
	const int d1_pin;
	const int d2_pin;
	const int d3_pin;

	int irq_source;
	int irq_priority;
	int irq_flags;
	uint8_t bus_width_cfg;

	struct sdhc_host_props props;
};

struct sdhc_esp32_data {

	uint8_t bus_width;  /* Bus width used by the slot (can change during execution) */
	uint32_t bus_clock; /* Value in Hz. ESP-IDF functions use kHz instead */

	enum sdhc_power power_mode;
	enum sdhc_timing_mode timing;

	struct host_ctx s_host_ctx;
	struct k_mutex s_request_mutex;
	bool s_is_app_cmd;
	sdmmc_desc_t s_dma_desc[SDMMC_DMA_DESC_CNT];
	struct sdmmc_transfer_state s_cur_transfer;
};

/**********************************************************************
 * ESP32 low level functions
 **********************************************************************/

/* We have two clock divider stages:
 * - one is the clock generator which drives SDMMC peripheral,
 *   it can be configured using sdio_hw->clock register. It can generate
 *   frequencies 160MHz/(N + 1), where 0 < N < 16, I.e. from 10 to 80 MHz.
 * - 4 clock dividers inside SDMMC peripheral, which can divide clock
 *   from the first stage by 2 * M, where 0 < M < 255
 *   (they can also be bypassed).
 *
 * For cards which aren't UHS-1 or UHS-2 cards, which we don't support,
 * maximum bus frequency in high speed (HS) mode is 50 MHz.
 * Note: for non-UHS-1 cards, HS mode is optional.
 * Default speed (DS) mode is mandatory, it works up to 25 MHz.
 * Whether the card supports HS or not can be determined using TRAN_SPEED
 * field of card's CSD register.
 *
 * 50 MHz can not be obtained exactly, closest we can get is 53 MHz.
 *
 * The first stage divider is set to the highest possible value for the given
 * frequency, and the second stage dividers are used if division factor
 * is >16.
 *
 * Of the second stage dividers, div0 is used for card 0, and div1 is used
 * for card 1.
 */
static int sdmmc_host_set_clk_div(sdmmc_dev_t *sdio_hw, int div)
{
	if (!((div > 1) && (div <= 16))) {
		LOG_ERR("Invalid parameter 'div'");
		return ESP_ERR_INVALID_ARG;
	}

	sdmmc_ll_set_clock_div(sdio_hw, div);
	sdmmc_ll_select_clk_source(sdio_hw, SDMMC_CLK_SRC_DEFAULT);
	sdmmc_ll_init_phase_delay(sdio_hw);

	/* Wait for the clock to propagate */
	esp_rom_delay_us(10);

	return 0;
}

static void sdmmc_host_dma_init(sdmmc_dev_t *sdio_hw)
{
	sdio_hw->ctrl.dma_enable = 1;
	sdio_hw->bmod.val = 0;
	sdio_hw->bmod.sw_reset = 1;
	sdio_hw->idinten.ni = 1;
	sdio_hw->idinten.ri = 1;
	sdio_hw->idinten.ti = 1;
}

static void sdmmc_host_dma_stop(sdmmc_dev_t *sdio_hw)
{
	sdio_hw->ctrl.use_internal_dma = 0;
	sdio_hw->ctrl.dma_reset = 1;
	sdio_hw->bmod.fb = 0;
	sdio_hw->bmod.enable = 0;
}

static int sdmmc_host_transaction_handler_init(struct sdhc_esp32_data *data)
{
	k_mutex_init(&data->s_request_mutex);

	data->s_is_app_cmd = false;

	return 0;
}

static int sdmmc_host_wait_for_event(struct sdhc_esp32_data *data, int timeout_ms,
				     struct sdmmc_event *out_event)
{
	if (!out_event) {
		return ESP_ERR_INVALID_ARG;
	}

	if (!data->s_host_ctx.event_queue) {
		return ESP_ERR_INVALID_STATE;
	}

	int ret = k_msgq_get(data->s_host_ctx.event_queue, out_event, K_MSEC(timeout_ms));

	return ret;
}

static int handle_idle_state_events(struct sdhc_esp32_data *data)
{
	/* Handle any events which have happened in between transfers.
	 * Under current assumptions (no SDIO support) only card detect events
	 * can happen in the idle state.
	 */
	struct sdmmc_event evt;

	int64_t yield_delay_us = 100 * 1000; /* initially 100ms */
	int64_t t0 = esp_timer_get_time();
	int64_t t1 = 0;

	while (sdmmc_host_wait_for_event(data, 0, &evt) == 0) {

		if (evt.sdmmc_status & SDMMC_INTMASK_CD) {
			LOG_DBG("card detect event");
			evt.sdmmc_status &= ~SDMMC_INTMASK_CD;
		}

		if (evt.sdmmc_status != 0 || evt.dma_status != 0) {
			LOG_DBG("%s unhandled: %08" PRIx32 " %08" PRIx32, __func__,
				evt.sdmmc_status, evt.dma_status);
		}

		/* Loop timeout */
		t1 = esp_timer_get_time();

		if (t1 - t0 > SDMMC_HOST_WAIT_EVENT_TIMEOUT_US) {
			return ESP_ERR_TIMEOUT;
		}

		if (t1 - t0 > yield_delay_us) {
			yield_delay_us *= 2;
			k_sleep(K_MSEC(1));
		}
	}

	return 0;
}

static void fill_dma_descriptors(struct sdhc_esp32_data *data, size_t num_desc)
{
	for (size_t i = 0; i < num_desc; ++i) {
		if (data->s_cur_transfer.size_remaining == 0) {
			return;
		}

		const size_t next = data->s_cur_transfer.next_desc;
		sdmmc_desc_t *desc = &data->s_dma_desc[next];

		if (desc->owned_by_idmac) {
			return;
		}

		size_t size_to_fill = (data->s_cur_transfer.size_remaining < SDMMC_DMA_MAX_BUF_LEN)
					      ? data->s_cur_transfer.size_remaining
					      : SDMMC_DMA_MAX_BUF_LEN;

		bool last = size_to_fill == data->s_cur_transfer.size_remaining;

		desc->last_descriptor = last;
		desc->second_address_chained = 1;
		desc->owned_by_idmac = 1;
		desc->buffer1_ptr = data->s_cur_transfer.ptr;
		desc->next_desc_ptr =
			(last) ? NULL : &data->s_dma_desc[(next + 1) % SDMMC_DMA_DESC_CNT];

		if (!((size_to_fill < 4) || ((size_to_fill % 4) == 0))) {
			return;
		}

		desc->buffer1_size = (size_to_fill + 3) & (~3);

		data->s_cur_transfer.size_remaining -= size_to_fill;
		data->s_cur_transfer.ptr += size_to_fill;
		data->s_cur_transfer.next_desc =
			(data->s_cur_transfer.next_desc + 1) % SDMMC_DMA_DESC_CNT;

		LOG_DBG("fill %d desc=%d rem=%d next=%d last=%d sz=%d", num_desc, next,
			data->s_cur_transfer.size_remaining, data->s_cur_transfer.next_desc,
			desc->last_descriptor, desc->buffer1_size);
	}
}

static void sdmmc_host_dma_resume(sdmmc_dev_t *sdio_hw)
{
	sdmmc_ll_poll_demand(sdio_hw);
}

static void sdmmc_host_dma_prepare(sdmmc_dev_t *sdio_hw, sdmmc_desc_t *desc, size_t block_size,
				   size_t data_size)
{
	/* Set size of data and DMA descriptor pointer */
	sdmmc_ll_set_data_transfer_len(sdio_hw, data_size);
	sdmmc_ll_set_block_size(sdio_hw, block_size);
	sdmmc_ll_set_desc_addr(sdio_hw, (uint32_t)desc);

	/* Enable everything needed to use DMA */
	sdmmc_ll_enable_dma(sdio_hw, true);
	sdmmc_host_dma_resume(sdio_hw);
}

static int sdmmc_host_start_command(sdmmc_dev_t *sdio_hw, int slot, sdmmc_hw_cmd_t cmd,
				    uint32_t arg)
{
	if (!(slot == 0 || slot == 1)) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!sdmmc_ll_is_card_detected(sdio_hw, slot)) {
		return ESP_ERR_NOT_FOUND;
	}
	if (cmd.data_expected && cmd.rw && sdmmc_ll_is_card_write_protected(sdio_hw, slot)) {
		return ESP_ERR_INVALID_STATE;
	}
	/* Outputs should be synchronized to cclk_out */
	cmd.use_hold_reg = 1;

	int64_t yield_delay_us = 100 * 1000; /* initially 100ms */
	int64_t t0 = esp_timer_get_time();
	int64_t t1 = 0;

	while (sdio_hw->cmd.start_command == 1) {
		t1 = esp_timer_get_time();

		if (t1 - t0 > SDMMC_HOST_START_CMD_TIMEOUT_US) {
			return ESP_ERR_TIMEOUT;
		}
		if (t1 - t0 > yield_delay_us) {
			yield_delay_us *= 2;
			k_sleep(K_MSEC(1));
		}
	}

	sdio_hw->cmdarg = arg;
	cmd.card_num = slot;
	cmd.start_command = 1;
	sdio_hw->cmd = cmd;

	return ESP_OK;
}

static void process_command_response(sdmmc_dev_t *sdio_hw, uint32_t status,
				     struct sdmmc_command *cmd)
{
	if (cmd->flags & SCF_RSP_PRESENT) {
		if (cmd->flags & SCF_RSP_136) {
			/* Destination is 4-byte aligned, can memcopy from peripheral registers */
			memcpy(cmd->response, (uint32_t *)sdio_hw->resp, 4 * sizeof(uint32_t));
		} else {
			cmd->response[0] = sdio_hw->resp[0];
			cmd->response[1] = 0;
			cmd->response[2] = 0;
			cmd->response[3] = 0;
		}
	}

	int err = ESP_OK;

	if (status & SDMMC_INTMASK_RTO) {
		/* response timeout is only possible when response is expected */
		if (!(cmd->flags & SCF_RSP_PRESENT)) {
			return;
		}

		err = ESP_ERR_TIMEOUT;
	} else if ((cmd->flags & SCF_RSP_CRC) && (status & SDMMC_INTMASK_RCRC)) {
		err = ESP_ERR_INVALID_CRC;
	} else if (status & SDMMC_INTMASK_RESP_ERR) {
		err = ESP_ERR_INVALID_RESPONSE;
	}

	if (err != ESP_OK) {
		cmd->error = err;
		if (cmd->data) {
			sdmmc_host_dma_stop(sdio_hw);
		}
		LOG_DBG("%s: error 0x%x  (status=%08" PRIx32 ")", __func__, err, status);
	}
}

static void process_data_status(sdmmc_dev_t *sdio_hw, uint32_t status, struct sdmmc_command *cmd)
{
	if (status & SDMMC_DATA_ERR_MASK) {
		if (status & SDMMC_INTMASK_DTO) {
			cmd->error = ESP_ERR_TIMEOUT;
		} else if (status & SDMMC_INTMASK_DCRC) {
			cmd->error = ESP_ERR_INVALID_CRC;
		} else if ((status & SDMMC_INTMASK_EBE) && (cmd->flags & SCF_CMD_READ) == 0) {
			cmd->error = ESP_ERR_TIMEOUT;
		} else {
			cmd->error = ESP_FAIL;
		}
		sdio_hw->ctrl.fifo_reset = 1;
	}

	if (cmd->error != 0) {
		if (cmd->data) {
			sdmmc_host_dma_stop(sdio_hw);
		}
		LOG_DBG("%s: error 0x%x (status=%08" PRIx32 ")", __func__, cmd->error, status);
	}
}

static inline bool mask_check_and_clear(uint32_t *state, uint32_t mask)
{
	bool ret = ((*state) & mask) != 0;

	*state &= ~mask;

	return ret;
}

static size_t get_free_descriptors_count(struct sdhc_esp32_data *data)
{
	const size_t next = data->s_cur_transfer.next_desc;
	size_t count = 0;

	/* Starting with the current DMA descriptor, count the number of
	 * descriptors which have 'owned_by_idmac' set to 0. These are the
	 * descriptors already processed by the DMA engine.
	 */
	for (size_t i = 0; i < SDMMC_DMA_DESC_CNT; ++i) {
		sdmmc_desc_t *desc = &data->s_dma_desc[(next + i) % SDMMC_DMA_DESC_CNT];

		if (desc->owned_by_idmac) {
			break;
		}
		++count;
		if (desc->next_desc_ptr == NULL) {
			/* final descriptor in the chain */
			break;
		}
	}

	return count;
}

static int process_events(const struct device *dev, struct sdmmc_event evt,
			  struct sdmmc_command *cmd, enum sdmmc_req_state *pstate,
			  struct sdmmc_event *unhandled_events)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	struct sdhc_esp32_data *data = dev->data;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;

	const char *const s_state_names[]
		__attribute__((unused)) = {"IDLE", "SENDING_CMD", "SENDIND_DATA", "BUSY"};
	struct sdmmc_event orig_evt = evt;

	LOG_DBG("%s: state=%s evt=%" PRIx32 " dma=%" PRIx32, __func__, s_state_names[*pstate],
		evt.sdmmc_status, evt.dma_status);

	enum sdmmc_req_state next_state = *pstate;
	enum sdmmc_req_state state = (enum sdmmc_req_state) -1;

	while (next_state != state) {

		state = next_state;

		switch (state) {

		case SDMMC_IDLE:
			break;

		case SDMMC_SENDING_CMD:
			if (mask_check_and_clear(&evt.sdmmc_status, SDMMC_CMD_ERR_MASK)) {
				process_command_response(sdio_hw, orig_evt.sdmmc_status, cmd);
				/*
				 * In addition to the error interrupt, CMD_DONE will also be
				 * reported. It may occur immediately (in the same sdmmc_event_t) or
				 * be delayed until the next interrupt
				 */
			}
			if (mask_check_and_clear(&evt.sdmmc_status, SDMMC_INTMASK_CMD_DONE)) {
				process_command_response(sdio_hw, orig_evt.sdmmc_status, cmd);
				if (cmd->error != ESP_OK) {
					next_state = SDMMC_IDLE;
					break;
				}

				if (cmd->data == NULL) {
					next_state = SDMMC_IDLE;
				} else {
					next_state = SDMMC_SENDING_DATA;
				}
			}
			break;

		case SDMMC_SENDING_DATA:
			if (mask_check_and_clear(&evt.sdmmc_status, SDMMC_DATA_ERR_MASK)) {
				process_data_status(sdio_hw, orig_evt.sdmmc_status, cmd);
				sdmmc_host_dma_stop(sdio_hw);
			}
			if (mask_check_and_clear(&evt.dma_status, SDMMC_DMA_DONE_MASK)) {

				data->s_cur_transfer.desc_remaining--;

				if (data->s_cur_transfer.size_remaining) {

					int desc_to_fill = get_free_descriptors_count(data);

					fill_dma_descriptors(data, desc_to_fill);
					sdmmc_host_dma_resume(sdio_hw);
				}
				if (data->s_cur_transfer.desc_remaining == 0) {
					next_state = SDMMC_BUSY;
				}
			}
			if (orig_evt.sdmmc_status & (SDMMC_INTMASK_SBE | SDMMC_INTMASK_DATA_OVER)) {
				/* On start bit error, DATA_DONE interrupt will not be generated */
				next_state = SDMMC_IDLE;
				break;
			}
			break;

		case SDMMC_BUSY:
			if (!mask_check_and_clear(&evt.sdmmc_status, SDMMC_INTMASK_DATA_OVER)) {
				break;
			}
			process_data_status(sdio_hw, orig_evt.sdmmc_status, cmd);
			next_state = SDMMC_IDLE;
			break;
		}
		LOG_DBG("%s state=%s next_state=%s", __func__, s_state_names[state],
			s_state_names[next_state]);
	}

	*pstate = state;
	*unhandled_events = evt;

	return ESP_OK;
}

static int handle_event(const struct device *dev, struct sdmmc_command *cmd,
			enum sdmmc_req_state *state, struct sdmmc_event *unhandled_events)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	struct sdhc_esp32_data *data = dev->data;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;
	struct sdmmc_event event;

	int err = sdmmc_host_wait_for_event(data, cmd->timeout_ms, &event);

	if (err != 0) {
		LOG_ERR("sdmmc_handle_event: sdmmc_host_wait_for_event returned 0x%x, timeout %d "
			"ms",
			err, cmd->timeout_ms);
		if (err == -EAGAIN) {
			sdmmc_host_dma_stop(sdio_hw);
		}
		return err;
	}

	LOG_DBG("sdmmc_handle_event: event %08" PRIx32 " %08" PRIx32 ", unhandled %08" PRIx32
		" %08" PRIx32,
		event.sdmmc_status, event.dma_status, unhandled_events->sdmmc_status,
		unhandled_events->dma_status);

	event.sdmmc_status |= unhandled_events->sdmmc_status;
	event.dma_status |= unhandled_events->dma_status;

	process_events(dev, event, cmd, state, unhandled_events);
	LOG_DBG("sdmmc_handle_event: events unhandled: %08" PRIx32 " %08" PRIx32,
		unhandled_events->sdmmc_status, unhandled_events->dma_status);

	return ESP_OK;
}

static bool wait_for_busy_cleared(const sdmmc_dev_t *sdio_hw, uint32_t timeout_ms)
{
	if (timeout_ms == 0) {
		return !(sdio_hw->status.data_busy == 1);
	}

	/* It would have been nice to do this without polling, however the peripheral
	 * can only generate Busy Clear Interrupt for data write commands, and waiting
	 * for busy clear is mostly needed for other commands such as MMC_SWITCH.
	 */
	uint32_t timeout_ticks = k_ms_to_ticks_ceil32(timeout_ms);

	while (timeout_ticks-- > 0) {
		if (!(sdio_hw->status.data_busy == 1)) {
			return true;
		}
		k_sleep(K_MSEC(1));
	}

	return false;
}

static bool cmd_needs_auto_stop(const struct sdmmc_command *cmd)
{
	/* SDMMC host needs an "auto stop" flag for the following commands: */
	return cmd->datalen > 0 &&
	       (cmd->opcode == SD_WRITE_MULTIPLE_BLOCK || cmd->opcode == SD_READ_MULTIPLE_BLOCK);
}

static sdmmc_hw_cmd_t make_hw_cmd(struct sdmmc_command *cmd)
{
	sdmmc_hw_cmd_t res = {0};

	res.cmd_index = cmd->opcode;
	if (cmd->opcode == SD_STOP_TRANSMISSION) {
		res.stop_abort_cmd = 1;
	} else if (cmd->opcode == SD_GO_IDLE_STATE) {
		res.send_init = 1;
	} else {
		res.wait_complete = 1;
	}
	if (cmd->opcode == SD_GO_IDLE_STATE) {
		res.send_init = 1;
	}
	if (cmd->flags & SCF_RSP_PRESENT) {
		res.response_expect = 1;
		if (cmd->flags & SCF_RSP_136) {
			res.response_long = 1;
		}
	}
	if (cmd->flags & SCF_RSP_CRC) {
		res.check_response_crc = 1;
	}
	if (cmd->data) {
		res.data_expected = 1;

		if ((cmd->flags & SCF_CMD_READ) == 0) {
			res.rw = 1;
		}

		if ((cmd->datalen % cmd->blklen) != 0) {
			return res; /* Error situation, data will be invalid */
		}

		res.send_auto_stop = cmd_needs_auto_stop(cmd) ? 1 : 0;
	}
	LOG_DBG("%s: opcode=%d, rexp=%d, crc=%d, auto_stop=%d", __func__, res.cmd_index,
		res.response_expect, res.check_response_crc, res.send_auto_stop);

	return res;
}

static int sdmmc_host_do_transaction(const struct device *dev, int slot,
				     struct sdmmc_command *cmdinfo)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	struct sdhc_esp32_data *data = dev->data;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;
	int ret;

	if (k_mutex_lock(&data->s_request_mutex, K_FOREVER) != 0) {
		return ESP_ERR_NO_MEM;
	}

	/* dispose of any events which happened asynchronously */
	handle_idle_state_events(data);

	/* convert cmdinfo to hardware register value */
	sdmmc_hw_cmd_t hw_cmd = make_hw_cmd(cmdinfo);

	if (cmdinfo->data) {
		/* Length should be either <4 or >=4 and =0 (mod 4) */
		if ((cmdinfo->datalen >= 4) && (cmdinfo->datalen % 4) != 0) {
			LOG_DBG("%s: invalid size: total=%d", __func__, cmdinfo->datalen);
			ret = ESP_ERR_INVALID_SIZE;
			goto out;
		}

		if ((((intptr_t)cmdinfo->data % 4) != 0) || !esp_ptr_dma_capable(cmdinfo->data)) {
			LOG_DBG("%s: buffer %p can not be used for DMA", __func__, cmdinfo->data);
			ret = ESP_ERR_INVALID_ARG;
			goto out;
		}

		/* this clears "owned by IDMAC" bits */
		memset(data->s_dma_desc, 0, sizeof(data->s_dma_desc));

		/* initialize first descriptor */
		data->s_dma_desc[0].first_descriptor = 1;

		/* save transfer info */
		data->s_cur_transfer.ptr = (uint8_t *)cmdinfo->data;
		data->s_cur_transfer.size_remaining = cmdinfo->datalen;
		data->s_cur_transfer.next_desc = 0;
		data->s_cur_transfer.desc_remaining =
			(cmdinfo->datalen + SDMMC_DMA_MAX_BUF_LEN - 1) / SDMMC_DMA_MAX_BUF_LEN;

		/* prepare descriptors */
		fill_dma_descriptors(data, SDMMC_DMA_DESC_CNT);

		/* write transfer info into hardware */
		sdmmc_host_dma_prepare(sdio_hw, &data->s_dma_desc[0], cmdinfo->blklen,
				       cmdinfo->datalen);
	}

	/* write command into hardware, this also sends the command to the card */
	ret = sdmmc_host_start_command(sdio_hw, slot, hw_cmd, cmdinfo->arg);

	if (ret != ESP_OK) {
		goto out;
	}

	/* process events until transfer is complete */
	cmdinfo->error = ESP_OK;

	enum sdmmc_req_state state = SDMMC_SENDING_CMD;
	struct sdmmc_event unhandled_events = {0};

	while (state != SDMMC_IDLE) {
		ret = handle_event(dev, cmdinfo, &state, &unhandled_events);
		if (ret != 0) {
			break;
		}
	}

	if (ret == 0 && (cmdinfo->flags & SCF_WAIT_BUSY)) {
		if (!wait_for_busy_cleared(sdio_hw, cmdinfo->timeout_ms)) {
			ret = ESP_ERR_TIMEOUT;
		}
	}

	data->s_is_app_cmd = (ret == ESP_OK && cmdinfo->opcode == SD_APP_CMD);

out:

	k_mutex_unlock(&data->s_request_mutex);

	return ret;
}

static int sdmmc_host_clock_update_command(sdmmc_dev_t *sdio_hw, int slot)
{
	int ret;
	bool repeat = true;

	/* Clock update command (not a real command; just updates CIU registers) */
	sdmmc_hw_cmd_t cmd_val = {.card_num = slot, .update_clk_reg = 1, .wait_complete = 1};

	while (repeat) {

		ret = sdmmc_host_start_command(sdio_hw, slot, cmd_val, 0);

		if (ret != 0) {
			return ret;
		}

		int64_t yield_delay_us = 100 * 1000; /* initially 100ms */
		int64_t t0 = esp_timer_get_time();
		int64_t t1 = 0;

		while (true) {
			t1 = esp_timer_get_time();

			if (t1 - t0 > SDMMC_HOST_CLOCK_UPDATE_CMD_TIMEOUT_US) {
				return ESP_ERR_TIMEOUT;
			}
			/* Sending clock update command to the CIU can generate HLE error */
			/* According to the manual, this is okay and we must retry the command */
			if (sdio_hw->rintsts.hle) {
				sdio_hw->rintsts.hle = 1;
				repeat = true;
				break;
			}
			/* When the command is accepted by CIU, start_command bit will be */
			/* cleared in sdio_hw->cmd register */
			if (sdio_hw->cmd.start_command == 0) {
				repeat = false;
				break;
			}
			if (t1 - t0 > yield_delay_us) {
				yield_delay_us *= 2;
				k_sleep(K_MSEC(1));
			}
		}
	}

	return 0;
}

void sdmmc_host_get_clk_dividers(uint32_t freq_khz, int *host_div, int *card_div)
{
	uint32_t clk_src_freq_hz = 0;

	esp_clk_tree_src_get_freq_hz(SDMMC_CLK_SRC_DEFAULT, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
				     &clk_src_freq_hz);
	assert(clk_src_freq_hz == (160 * 1000 * 1000));

	/* Calculate new dividers */
	if (freq_khz >= SDMMC_FREQ_HIGHSPEED) {
		*host_div = 4; /* 160 MHz / 4 = 40 MHz */
		*card_div = 0;
	} else if (freq_khz == SDMMC_FREQ_DEFAULT) {
		*host_div = 8; /* 160 MHz / 8 = 20 MHz */
		*card_div = 0;
	} else if (freq_khz == SDMMC_FREQ_PROBING) {
		*host_div = 10; /* 160 MHz / 10 / (20 * 2) = 400 kHz */
		*card_div = 20;
	} else {
		/*
		 * for custom frequencies use maximum range of host divider (1-16), find the closest
		 * <= div. combination if exceeded, combine with the card divider to keep reasonable
		 * precision (applies mainly to low frequencies) effective frequency range: 400 kHz
		 * - 32 MHz (32.1 - 39.9 MHz cannot be covered with given divider scheme)
		 */
		*host_div = (clk_src_freq_hz) / (freq_khz * 1000);
		if (*host_div > 15) {
			*host_div = 2;
			*card_div = (clk_src_freq_hz / 2) / (2 * freq_khz * 1000);
			if (((clk_src_freq_hz / 2) % (2 * freq_khz * 1000)) > 0) {
				(*card_div)++;
			}
		} else if ((clk_src_freq_hz % (freq_khz * 1000)) > 0) {
			(*host_div)++;
		}
	}
}

static int sdmmc_host_calc_freq(const int host_div, const int card_div)
{
	uint32_t clk_src_freq_hz = 0;

	esp_clk_tree_src_get_freq_hz(SDMMC_CLK_SRC_DEFAULT, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
				     &clk_src_freq_hz);
	assert(clk_src_freq_hz == (160 * 1000 * 1000));

	return clk_src_freq_hz / host_div / ((card_div == 0) ? 1 : card_div * 2) / 1000;
}

int sdmmc_host_set_card_clk(sdmmc_dev_t *sdio_hw, int slot, uint32_t freq_khz)
{
	if (!(slot == 0 || slot == 1)) {
		return ESP_ERR_INVALID_ARG;
	}

	/* Disable clock first */
	sdmmc_ll_enable_card_clock(sdio_hw, slot, false);
	int err = sdmmc_host_clock_update_command(sdio_hw, slot);

	if (err != 0) {
		LOG_ERR("disabling clk failed");
		LOG_ERR("%s: sdmmc_host_clock_update_command returned 0x%x", __func__, err);
		return err;
	}

	int host_div = 0; /* clock divider of the host (sdio_hw->clock) */
	int card_div = 0; /* 1/2 of card clock divider (sdio_hw->clkdiv) */

	sdmmc_host_get_clk_dividers(freq_khz, &host_div, &card_div);

	int real_freq = sdmmc_host_calc_freq(host_div, card_div);

	LOG_DBG("slot=%d host_div=%d card_div=%d freq=%dkHz (max %" PRIu32 "kHz)", slot, host_div,
		card_div, real_freq, freq_khz);

	/* Program card clock settings, send them to the CIU */
	sdmmc_ll_set_card_clock_div(sdio_hw, slot, card_div);
	err = sdmmc_host_set_clk_div(sdio_hw, host_div);

	if (err != 0) {
		return err;
	}

	err = sdmmc_host_clock_update_command(sdio_hw, slot);

	if (err != 0) {
		LOG_ERR("setting clk div failed");
		LOG_ERR("%s: sdmmc_host_clock_update_command returned 0x%x", __func__, err);
		return err;
	}

	/* Re-enable clocks */
	sdmmc_ll_enable_card_clock(sdio_hw, slot, true);
	sdmmc_ll_enable_card_clock_low_power(sdio_hw, slot, true);

	err = sdmmc_host_clock_update_command(sdio_hw, slot);

	if (err != 0) {
		LOG_ERR("re-enabling clk failed");
		LOG_ERR("%s: sdmmc_host_clock_update_command returned 0x%x", __func__, err);
		return err;
	}

	/* set data timeout */
	const uint32_t data_timeout_ms = 100;
	uint32_t data_timeout_cycles = data_timeout_ms * freq_khz;

	sdmmc_ll_set_data_timeout(sdio_hw, data_timeout_cycles);
	/* always set response timeout to highest value, it's small enough anyway */
	sdmmc_ll_set_response_timeout(sdio_hw, 255);

	return 0;
}

int sdmmc_host_set_bus_width(sdmmc_dev_t *sdio_hw, int slot, size_t width)
{
	if (!(slot == 0 || slot == 1)) {
		return ESP_ERR_INVALID_ARG;
	}

	const uint16_t mask = BIT(slot);

	if (width == 1) {
		sdio_hw->ctype.card_width_8 &= ~mask;
		sdio_hw->ctype.card_width &= ~mask;
	} else if (width == 4) {
		sdio_hw->ctype.card_width_8 &= ~mask;
		sdio_hw->ctype.card_width |= mask;
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	LOG_DBG("slot=%d width=%d", slot, width);
	return ESP_OK;
}

static void configure_pin_iomux(int gpio_num)
{
	const int sdmmc_func = SDMMC_LL_IOMUX_FUNC;
	const int drive_strength = 3;

	if (gpio_num == GPIO_NUM_NC) {
		return; /* parameter check*/
	}

	int rtc_num = rtc_io_num_map[gpio_num];

	rtcio_hal_pulldown_disable(rtc_num);
	rtcio_hal_pullup_enable(rtc_num);

	uint32_t reg = GPIO_PIN_MUX_REG[gpio_num];

	PIN_INPUT_ENABLE(reg);
	gpio_hal_iomux_func_sel(reg, sdmmc_func);
	PIN_SET_DRV(reg, drive_strength);
}

/**********************************************************************
 * Zephyr API
 **********************************************************************/

/*
 * Reset USDHC controller
 */
static int sdhc_esp32_reset(const struct device *dev)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;

	/* Set reset bits */
	sdio_hw->ctrl.controller_reset = 1;
	sdio_hw->ctrl.dma_reset = 1;
	sdio_hw->ctrl.fifo_reset = 1;

	/* Wait for the reset bits to be cleared by hardware */
	int64_t yield_delay_us = 100 * 1000; /* initially 100ms */
	int64_t t0 = esp_timer_get_time();
	int64_t t1 = 0;

	while (sdio_hw->ctrl.controller_reset || sdio_hw->ctrl.fifo_reset ||
	       sdio_hw->ctrl.dma_reset) {
		t1 = esp_timer_get_time();

		if (t1 - t0 > SDMMC_HOST_RESET_TIMEOUT_US) {
			return -ETIMEDOUT;
		}

		if (t1 - t0 > yield_delay_us) {
			yield_delay_us *= 2;
			k_busy_wait(1);
		}
	}

	/* Reset carried out successfully */
	return 0;
}

/*
 * Set SDHC io properties
 */
static int sdhc_esp32_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;
	struct sdhc_esp32_data *data = dev->data;
	uint8_t bus_width;
	int ret = 0;

	LOG_INF("SDHC I/O: slot: %d, bus width %d, clock %dHz, card power %s, voltage %s",
		cfg->slot, ios->bus_width, ios->clock,
		ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	if (ios->clock) {
		/* Check for frequency boundaries supported by host */
		if (ios->clock > cfg->props.f_max || ios->clock < cfg->props.f_min) {
			LOG_ERR("Proposed clock outside supported host range");
			return -EINVAL;
		}

		if (data->bus_clock != (uint32_t)ios->clock) {
			/* Try setting new clock */
			ret = sdmmc_host_set_card_clk(sdio_hw, cfg->slot, (ios->clock / 1000));

			if (ret == 0) {
				LOG_INF("Bus clock successfully set to %d kHz", ios->clock / 1000);
			} else {
				LOG_ERR("Error configuring card clock");
				return err_esp2zep(ret);
			}

			data->bus_clock = (uint32_t)ios->clock;
		}
	}

	if (ios->bus_width > 0) {
		/* Set bus width */
		switch (ios->bus_width) {
		case SDHC_BUS_WIDTH1BIT:
			bus_width = 1;
			break;
		case SDHC_BUS_WIDTH4BIT:
			bus_width = 4;
			break;
		default:
			return -ENOTSUP;
		}

		if (data->bus_width != bus_width) {
			ret = sdmmc_host_set_bus_width(sdio_hw, cfg->slot, bus_width);

			if (ret == 0) {
				LOG_INF("Bus width set successfully to %d bit", bus_width);
			} else {
				LOG_ERR("Error configuring bus width");
				return err_esp2zep(ret);
			}

			data->bus_width = bus_width;
		}
	}

	/* Toggle card power supply */
	if ((data->power_mode != ios->power_mode) && (cfg->pwr_gpio.port)) {
		if (ios->power_mode == SDHC_POWER_OFF) {
			gpio_pin_set_dt(&cfg->pwr_gpio, 0);
		} else if (ios->power_mode == SDHC_POWER_ON) {
			gpio_pin_set_dt(&cfg->pwr_gpio, 1);
		}
		data->power_mode = ios->power_mode;
	}

	if (ios->timing > 0) {
		/* Set I/O timing */
		if (data->timing != ios->timing) {
			switch (ios->timing) {
			case SDHC_TIMING_LEGACY:
			case SDHC_TIMING_HS:
				sdmmc_ll_enable_ddr_mode(sdio_hw, cfg->slot, false);
				break;
			case SDHC_TIMING_DDR50:
			case SDHC_TIMING_DDR52:
				/* Enable DDR mode */
				sdmmc_ll_enable_ddr_mode(sdio_hw, cfg->slot, true);
				LOG_INF("DDR mode enabled");
				break;
			case SDHC_TIMING_SDR12:
			case SDHC_TIMING_SDR25:
				sdmmc_ll_enable_ddr_mode(sdio_hw, cfg->slot, false);
				break;
			case SDHC_TIMING_SDR50:
			case SDHC_TIMING_HS400:
			case SDHC_TIMING_SDR104:
			case SDHC_TIMING_HS200:
			default:
				LOG_ERR("Timing mode not supported for this device");
				ret = -ENOTSUP;
				break;
			}

			LOG_INF("Bus timing successfully changed to %s", timingStr[ios->timing]);
			data->timing = ios->timing;
		}
	}

	return ret;
}

/*
 * Return 0 if card is not busy, 1 if it is
 */
static int sdhc_esp32_card_busy(const struct device *dev)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	const sdmmc_dev_t *sdio_hw = cfg->sdio_hw;

	return (sdio_hw->status.data_busy == 1);
}

/*
 * Send CMD or CMD/DATA via SDHC
 */
static int sdhc_esp32_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	int retries = (int)(cmd->retries + 1); /* first try plus retries */
	uint32_t timeout_cfg = 0;
	int ret_esp = 0;
	int ret = 0;

	/* convert command structures Zephyr vs ESP */
	struct sdmmc_command esp_cmd = {
		.opcode = cmd->opcode,
		.arg = cmd->arg,
	};

	if (data) {
		esp_cmd.data = data->data;
		esp_cmd.blklen = data->block_size;
		esp_cmd.datalen = (data->blocks * data->block_size);
		esp_cmd.buflen = esp_cmd.datalen;
		timeout_cfg = data->timeout_ms;
	} else {
		timeout_cfg = cmd->timeout_ms;
	}

	/* setting timeout according to command type */
	if (cmd->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		esp_cmd.timeout_ms = SDMMC_TIMEOUT_MAX;
	} else {
		esp_cmd.timeout_ms = timeout_cfg;
	}

	/*
	 * Handle flags and arguments with ESP32 specifics
	 */
	switch (cmd->opcode) {
	case SD_GO_IDLE_STATE:
		esp_cmd.flags = SCF_CMD_BC | SCF_RSP_R0;
		break;

	case SD_APP_CMD:
	case SD_SEND_STATUS:
	case SD_SET_BLOCK_SIZE:
		esp_cmd.flags = SCF_CMD_AC | SCF_RSP_R1;
		break;

	case SD_SEND_IF_COND:
		esp_cmd.flags = SCF_CMD_BCR | SCF_RSP_R7;
		break;

	case SD_APP_SEND_OP_COND:
		esp_cmd.flags = SCF_CMD_BCR | SCF_RSP_R3;
		esp_cmd.arg = SD_OCR_SDHC_CAP | SD_OCR_VOL_MASK;
		break;

	case SDIO_RW_DIRECT:
		esp_cmd.flags = SCF_CMD_AC | SCF_RSP_R5;
		break;

	case SDIO_SEND_OP_COND:
		esp_cmd.flags = SCF_CMD_BCR | SCF_RSP_R4;
		break;

	case SD_ALL_SEND_CID:
		esp_cmd.flags = SCF_CMD_BCR | SCF_RSP_R2;
		break;

	case SD_SEND_RELATIVE_ADDR:
		esp_cmd.flags = SCF_CMD_BCR | SCF_RSP_R6;
		break;

	case SD_SEND_CSD:
		esp_cmd.flags = SCF_CMD_AC | SCF_RSP_R2;
		esp_cmd.datalen = 0;
		break;

	case SD_SELECT_CARD:
		/* Don't expect to see a response when de-selecting a card */
		esp_cmd.flags = SCF_CMD_AC | (cmd->arg > 0 ? SCF_RSP_R1 : 0);
		break;

	case SD_APP_SEND_SCR:
	case SD_SWITCH:
	case SD_READ_SINGLE_BLOCK:
	case SD_READ_MULTIPLE_BLOCK:
	case SD_APP_SEND_NUM_WRITTEN_BLK:
		esp_cmd.flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1;
		break;

	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		esp_cmd.flags = SCF_CMD_ADTC | SCF_RSP_R1;
		break;

	default:
		LOG_INF("SDHC driver: command %u not supported", cmd->opcode);
		return -ENOTSUP;
	}

	while (retries > 0) {

		ret_esp = sdmmc_host_do_transaction(dev, cfg->slot, &esp_cmd);

		if (ret_esp) {
			retries--; /* error, retry */
		} else {
			break;
		}
	}

	if ((ret_esp != 0) || esp_cmd.error) {
		LOG_DBG("Error command: %u arg %08x ret_esp = 0x%x error = 0x%x\n",
			cmd->opcode, cmd->arg, ret_esp, esp_cmd.error);

		ret_esp = (ret_esp > 0) ? ret_esp : esp_cmd.error;

		ret = err_esp2zep(ret_esp);
	} else {
		/* fill response buffer */
		memcpy(cmd->response, esp_cmd.response, sizeof(cmd->response));

		int state = MMC_R1_CURRENT_STATE(esp_cmd.response);

		LOG_DBG("cmd %u arg %08x response %08x %08x %08x %08x err=0x%x state=%d",
			esp_cmd.opcode, esp_cmd.arg, esp_cmd.response[0], esp_cmd.response[1],
			esp_cmd.response[2], esp_cmd.response[3], esp_cmd.error, state);

		if (data) {
			/* Record number of bytes xfered */
			data->bytes_xfered = esp_cmd.datalen;
		}
	}

	return ret;
}

/*
 * Get card presence
 */
static int sdhc_esp32_get_card_present(const struct device *dev)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;

	return sdmmc_ll_is_card_detected(sdio_hw, cfg->slot);
}

/*
 * Get host properties
 */
static int sdhc_esp32_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct sdhc_esp32_config *cfg = dev->config;

	memcpy(props, &cfg->props, sizeof(struct sdhc_host_props));
	return 0;
}

/**
 * @brief SDMMC interrupt handler
 *
 * All communication in SD protocol is driven by the master, and the hardware
 * handles things like stop commands automatically.
 * So the interrupt handler doesn't need to do much, we just push interrupt
 * status into a queue, clear interrupt flags, and let the task currently
 * doing communication figure out what to do next.
 *
 * Card detect interrupts pose a small issue though, because if a card is
 * plugged in and out a few times, while there is no task to process
 * the events, event queue can become full and some card detect events
 * may be dropped. We ignore this problem for now, since the there are no other
 * interesting events which can get lost due to this.
 */
static void IRAM_ATTR sdio_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct sdhc_esp32_config *cfg = dev->config;
	struct sdhc_esp32_data *data = dev->data;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;

	struct sdmmc_event event;
	struct k_msgq *queue = data->s_host_ctx.event_queue;
	uint32_t pending = sdmmc_ll_get_intr_status(sdio_hw) & 0xFFFF;

	sdio_hw->rintsts.val = pending;
	event.sdmmc_status = pending;

	uint32_t dma_pending = sdio_hw->idsts.val;

	sdio_hw->idsts.val = dma_pending;
	event.dma_status = dma_pending & 0x1f;

	if ((pending != 0) || (dma_pending != 0)) {
		k_msgq_put(queue, &event, K_NO_WAIT);
	}
}

/*
 * Perform early system init for SDHC
 */
static int sdhc_esp32_init(const struct device *dev)
{
	const struct sdhc_esp32_config *cfg = dev->config;
	struct sdhc_esp32_data *data = dev->data;
	sdmmc_dev_t *sdio_hw = (sdmmc_dev_t *)cfg->sdio_hw;
	int ret;

	/* Pin configuration */

	/* Set power GPIO high, so card starts powered */
	if (cfg->pwr_gpio.port) {
		ret = gpio_pin_configure_dt(&cfg->pwr_gpio, GPIO_OUTPUT_ACTIVE);

		if (ret) {
			return -EIO;
		}
	}

	/*
	 * Pins below are only defined for ESP32. For SoC's with GPIO matrix feature
	 * please use pinctrl for pin configuration.
	 */
	configure_pin_iomux(cfg->clk_pin);
	configure_pin_iomux(cfg->cmd_pin);
	configure_pin_iomux(cfg->d0_pin);
	configure_pin_iomux(cfg->d1_pin);
	configure_pin_iomux(cfg->d2_pin);
	configure_pin_iomux(cfg->d3_pin);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("Failed to configure SDHC pins");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);

	if (ret != 0) {
		LOG_ERR("Error enabling SDHC clock");
		return ret;
	}

	/* Enable clock to peripheral. Use smallest divider first */
	ret = sdmmc_host_set_clk_div(sdio_hw, 2);

	if (ret != 0) {
		return err_esp2zep(ret);
	}

	/* Reset controller */
	sdhc_esp32_reset(dev);

	/* Clear interrupt status and set interrupt mask to known state */
	sdio_hw->rintsts.val = 0xffffffff;
	sdio_hw->intmask.val = 0;
	sdio_hw->ctrl.int_enable = 0;

	/* Attach interrupt handler */
	ret = esp_intr_alloc(cfg->irq_source,
				ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				ESP_INT_FLAGS_CHECK(cfg->irq_flags) | ESP_INTR_FLAG_IRAM,
				&sdio_esp32_isr, (void *)dev,
				&data->s_host_ctx.intr_handle);

	if (ret != 0) {
		k_msgq_purge(data->s_host_ctx.event_queue);
		return -EFAULT;
	}

	/* Enable interrupts */
	sdio_hw->intmask.val = SDMMC_INTMASK_CD | SDMMC_INTMASK_CMD_DONE | SDMMC_INTMASK_DATA_OVER |
			       SDMMC_INTMASK_RCRC | SDMMC_INTMASK_DCRC | SDMMC_INTMASK_RTO |
			       SDMMC_INTMASK_DTO | SDMMC_INTMASK_HTO | SDMMC_INTMASK_SBE |
			       SDMMC_INTMASK_EBE | SDMMC_INTMASK_RESP_ERR |
			       SDMMC_INTMASK_HLE; /* sdio is enabled only when use */

	sdio_hw->ctrl.int_enable = 1;

	/* Disable generation of Busy Clear Interrupt */
	sdio_hw->cardthrctl.busy_clr_int_en = 0;

	/* Enable DMA */
	sdmmc_host_dma_init(sdio_hw);

	/* Initialize transaction handler */
	ret = sdmmc_host_transaction_handler_init(data);

	if (ret != 0) {
		k_msgq_purge(data->s_host_ctx.event_queue);
		esp_intr_free(data->s_host_ctx.intr_handle);
		data->s_host_ctx.intr_handle = NULL;

		return ret;
	}

	/* post init settings */
	ret = sdmmc_host_set_card_clk(sdio_hw, cfg->slot, data->bus_clock / 1000);

	if (ret != 0) {
		LOG_ERR("Error configuring card clock");
		return err_esp2zep(ret);
	}

	ret = sdmmc_host_set_bus_width(sdio_hw, cfg->slot, data->bus_width);

	if (ret != 0) {
		LOG_ERR("Error configuring bus width");
		return err_esp2zep(ret);
	}

	return 0;
}

static const struct sdhc_driver_api sdhc_api = {.reset = sdhc_esp32_reset,
						.request = sdhc_esp32_request,
						.set_io = sdhc_esp32_set_io,
						.get_card_present = sdhc_esp32_get_card_present,
						.card_busy = sdhc_esp32_card_busy,
						.get_host_props = sdhc_esp32_get_host_props};

#define SDHC_ESP32_INIT(n)                                                                         \
                                                                                                   \
	PINCTRL_DT_DEFINE(DT_DRV_INST(n));                                                         \
	K_MSGQ_DEFINE(sdhc##n##_queue, sizeof(struct sdmmc_event), SDMMC_EVENT_QUEUE_LENGTH, 1);   \
                                                                                                   \
	static const struct sdhc_esp32_config sdhc_esp32_##n##_config = {                          \
		.sdio_hw = (const sdmmc_dev_t *)DT_REG_ADDR(DT_INST_PARENT(n)),                    \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(n))),                     \
		.clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(DT_INST_PARENT(n), offset), \
		.irq_source = DT_IRQ_BY_IDX(DT_INST_PARENT(n), 0, irq),                            \
		.irq_priority = DT_IRQ_BY_IDX(DT_INST_PARENT(n), 0, priority),                     \
		.irq_flags = DT_IRQ_BY_IDX(DT_INST_PARENT(n), 0, flags),                           \
		.slot = DT_REG_ADDR(DT_DRV_INST(n)),                                               \
		.bus_width_cfg = DT_INST_PROP(n, bus_width),                                       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(n)),                                 \
		.pwr_gpio = GPIO_DT_SPEC_INST_GET_OR(n, pwr_gpios, {0}),                           \
		.clk_pin = DT_INST_PROP_OR(n, clk_pin, GPIO_NUM_NC),                               \
		.cmd_pin = DT_INST_PROP_OR(n, cmd_pin, GPIO_NUM_NC),                               \
		.d0_pin = DT_INST_PROP_OR(n, d0_pin, GPIO_NUM_NC),                                 \
		.d1_pin = DT_INST_PROP_OR(n, d1_pin, GPIO_NUM_NC),                                 \
		.d2_pin = DT_INST_PROP_OR(n, d2_pin, GPIO_NUM_NC),                                 \
		.d3_pin = DT_INST_PROP_OR(n, d3_pin, GPIO_NUM_NC),                                 \
		.props = {.is_spi = false,                                                         \
			  .f_max = DT_INST_PROP(n, max_bus_freq),                                  \
			  .f_min = DT_INST_PROP(n, min_bus_freq),                                  \
			  .max_current_330 = DT_INST_PROP(n, max_current_330),                     \
			  .max_current_180 = DT_INST_PROP(n, max_current_180),                     \
			  .power_delay = DT_INST_PROP_OR(n, power_delay_ms, 0),                    \
			  .host_caps = {.vol_180_support = false,                                  \
					.vol_300_support = false,                                  \
					.vol_330_support = true,                                   \
					.suspend_res_support = false,                              \
					.sdma_support = true,                                      \
					.high_spd_support =                                        \
						(DT_INST_PROP(n, bus_width) == 4) ? true : false,  \
					.adma_2_support = false,                                   \
					.max_blk_len = 0,                                          \
					.ddr50_support = false,                                    \
					.sdr104_support = false,                                   \
					.sdr50_support = false,                                    \
					.bus_8_bit_support = false,                                \
					.bus_4_bit_support =                                       \
						(DT_INST_PROP(n, bus_width) == 4) ? true : false,  \
					.hs200_support = false,                                    \
					.hs400_support = false}}};                                 \
                                                                                                   \
	static struct sdhc_esp32_data sdhc_esp32_##n##_data = {                                    \
		.bus_width = SDMMC_SLOT_WIDTH_DEFAULT,                                             \
		.bus_clock = (SDMMC_FREQ_PROBING * 1000),                                          \
		.power_mode = SDHC_POWER_ON,                                                       \
		.timing = SDHC_TIMING_LEGACY,                                                      \
		.s_host_ctx = {.event_queue = &sdhc##n##_queue}};                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sdhc_esp32_init, NULL, &sdhc_esp32_##n##_data,                   \
			      &sdhc_esp32_##n##_config, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,    \
			      &sdhc_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_ESP32_INIT)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Currently, only one espressif,esp32-sdhc-slot compatible node is supported");
