/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include "espi_oob_handler.h"

LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);

struct oob_header {
	uint8_t dest_slave_addr;
	uint8_t oob_cmd_code;
	uint8_t byte_cnt;
	uint8_t src_slave_addr;
};

#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC

#define OOB_THREAD_STACK_SIZE  512ul
#define OOB_THREAD_PRIORITY    K_PRIO_COOP(5)
#define OOB_THREAD_WAIT        -1

/* Thread to process asynchronous callbacks */
void espihub_thread(void *p1, void *p2, void *p3);

void temperature_timer(struct k_timer *timer_id);

K_TIMER_DEFINE(temp_timer, temperature_timer, NULL);
K_THREAD_DEFINE(espihub_thrd_id, OOB_THREAD_STACK_SIZE, espihub_thread,
		NULL, NULL, NULL,
		OOB_THREAD_PRIORITY, K_INHERIT_PERMS, OOB_THREAD_WAIT);

K_MSGQ_DEFINE(from_host, sizeof(uint8_t), 8, 4);

struct thread_context {
	const struct device  *espi_dev;
	int                  cycles;
};

static struct thread_context context;
static bool need_temp;
#endif

static struct espi_oob_packet resp_pckt;
static uint8_t buf[MAX_ESPI_BUF_LEN];

static int request_temp(const struct device *dev)
{
	int ret;
	struct oob_header oob_hdr;
	struct espi_oob_packet req_pckt;

	LOG_WRN("%s", __func__);

	oob_hdr.dest_slave_addr = PCH_DEST_SLV_ADDR;
	oob_hdr.oob_cmd_code = OOB_CMDCODE;
	oob_hdr.byte_cnt = 1;
	oob_hdr.src_slave_addr = SRC_SLV_ADDR;

	/* Packetize OOB request */
	req_pckt.buf = (uint8_t *)&oob_hdr;
	req_pckt.len = sizeof(struct oob_header);

	ret = espi_send_oob(dev, &req_pckt);
	if (ret) {
		LOG_ERR("OOB Tx failed %d", ret);
		return ret;
	}

	return 0;
}

static int retrieve_packet(const struct device *dev, uint8_t *sender)
{
	int ret;

#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	/* Note that no data is in the item */
	uint8_t response_len;

	if (k_msgq_num_used_get(&from_host) == 0U) {
		return -EINVAL;
	}

	k_msgq_get(&from_host, &response_len, K_FOREVER);
#endif

	resp_pckt.buf = (uint8_t *)&buf;
	resp_pckt.len = MAX_ESPI_BUF_LEN;

	ret = espi_receive_oob(dev, &resp_pckt);
	if (ret) {
		LOG_ERR("OOB Rx failed %d", ret);
		return ret;
	}

	LOG_INF("OOB transaction completed rcvd: %d bytes", resp_pckt.len);
	for (int i = 0; i < resp_pckt.len; i++) {
		LOG_INF("%x ", buf[i]);
	}

	if (sender) {
		*sender = buf[OOB_RESPONSE_SENDER_INDEX];
	}

	return 0;
}

int get_pch_temp_sync(const struct device *dev)
{
	int ret;

	for (int i = 0; i < MIN_GET_TEMP_CYCLES; i++) {
		ret = request_temp(dev);
		if (ret) {
			LOG_ERR("OOB req failed %d", ret);
			return ret;
		}

		ret = retrieve_packet(dev, NULL);
		if (ret) {
			LOG_ERR("OOB retrieve failed %d", ret);
			return ret;
		}
	}

	return 0;
}

int get_pch_temp_async(const struct device *dev)
{
#if !defined(CONFIG_ESPI_OOB_CHANNEL) || \
	!defined(CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC)
	return -ENOTSUP;
#else
	context.espi_dev = dev;
	context.cycles = MIN_GET_TEMP_CYCLES;

	k_thread_start(espihub_thrd_id);
	k_thread_join(espihub_thrd_id, K_FOREVER);

	return 0;
#endif
}

#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC

void oob_rx_handler(const struct device *dev, struct espi_callback *cb,
		    struct espi_event event)
{
	uint8_t last_resp_len = event.evt_details;

	LOG_WRN("%s", __func__);
	/* Post for post-processing in a thread
	 * Should not attempt to retrieve in callback context
	 */
	k_msgq_put(&from_host, &last_resp_len, K_NO_WAIT);
}


void temperature_timer(struct k_timer *timer_id)
{
	LOG_WRN("%s", __func__);
	need_temp = true;
}

void espihub_thread(void *p1, void *p2, void *p3)
{
	int ret;
	uint8_t temp;
	uint8_t sender;

	LOG_DBG("%s", __func__);
	k_timer_start(&temp_timer, K_MSEC(100), K_MSEC(100));
	while (context.cycles > 0) {
		k_msleep(50);

		ret = retrieve_packet(context.espi_dev, &sender);
		if (!ret) {
			switch (sender) {
			case PCH_DEST_SLV_ADDR:
				LOG_INF("PCH response");
				/* Any other checks */
				if (resp_pckt.len == OOB_RESPONSE_LEN) {
					temp = buf[OOB_RESPONSE_DATA_INDEX];
					LOG_INF("Temp %d", temp);
				} else {
					LOG_ERR("Incorrect size response");
				}

				break;
			default:
				LOG_INF("Other host sender %x", sender);
			}
		} else {
			LOG_ERR("Failure to retrieve temp %d", ret);
		}

		/* Decrease cycles in both cases failure/success */
		context.cycles--;

		if (need_temp) {
			request_temp(context.espi_dev);
			need_temp = false;
		}
	}

	k_timer_stop(&temp_timer);
}
#endif /* CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC */
