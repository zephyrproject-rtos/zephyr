/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_espi_taf

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <fsl_espi.h>
#include "espi_taf_nxp.h"
#include <zephyr/drivers/espi_saf.h>
#include "espi_utils.h"

LOG_MODULE_REGISTER(espi_taf_nxp, CONFIG_ESPI_LOG_LEVEL);

struct espi_taf_nxp_config {
	ESPI_Type *base;
	const struct device *parent;
	const struct device *flash_dev;
};

struct espi_taf_nxp_data {
	const struct device *dev;
	sys_slist_t callbacks;
	struct espi_callback taf_cb;
	struct k_work work;
	struct espi_nxp_taf_req req;
	bool rd_active;
	uint8_t rd_port;
	uint8_t rd_tag;
	uint32_t rd_addr;
	uint32_t rd_total;
	uint32_t rd_offset;
	uint32_t rd_max_payload;
};

/* Send an unsuccessful flash completion to the host.
 *
 * A failure completion carries no data and is always a single (non-split)
 * completion, so the completion type is fixed to kESPI_SAFReadOnly.
 */
static void espi_taf_nxp_send_fail(ESPI_Type *base, uint8_t port, uint8_t tag)
{
	ESPI_SetFlashOpLen(base, port,
		(uint32_t)kESPI_OMFLEN_SAFCompletionFail, 0U);
	ESPI_SetFlashCompletion(base, port, tag,
		(uint32_t)kESPI_SSTCL_SAFCompletion,
		(uint32_t)kESPI_SAFReadOnly);
}

static void espi_taf_nxp_service_erase(const struct device *dev, uint8_t port)
{
	const struct espi_taf_nxp_config *config = dev->config;
	struct espi_taf_nxp_data *data = dev->data;
	ESPI_Type *base = config->base;
	uint32_t erase_addr = data->req.addr;
	uint32_t erase_len = data->req.len;
	int ret = 0;

	LOG_INF("> TAF erase req addr 0x%x len %u", data->req.addr, data->req.len);

	ESPI_SetFlashCompletion(base, port, data->req.tag,
		(uint32_t)kESPI_SSTCL_SAFReqAccepted,
		(uint32_t)kESPI_SAFReadMiddle);

	if (IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)) {
		struct flash_pages_info page;

		ret = flash_get_page_info_by_offs(config->flash_dev,
						  data->req.addr, &page);
		if (ret == 0) {
			erase_addr = page.start_offset;
			erase_len = page.size;
		} else {
			LOG_ERR("TAF erase page info failed: %d", ret);
		}
	}

	if (ret == 0) {
		ret = flash_erase(config->flash_dev, erase_addr, erase_len);
		if (ret < 0) {
			LOG_ERR("TAF erase failed: %d", ret);
		}
	}

	if (ret < 0) {
		ESPI_SetFlashOpLen(base, port,
			(uint32_t)kESPI_OMFLEN_SAFCompletionFail, 0U);
	} else {
		ESPI_SetFlashOpLen(base, port,
			(uint32_t)kESPI_OMFLEN_SAFCompletionNoData,
			data->req.len);
		LOG_INF("TAF erase done addr 0x%x len %u", erase_addr,
			erase_len);
	}

	ESPI_SetFlashCompletion(base, port, data->req.tag,
		(uint32_t)kESPI_SSTCL_SAFCompletion,
		(uint32_t)kESPI_SAFReadOnly);
}

static void espi_taf_nxp_service_write(const struct device *dev, uint8_t port)
{
	const struct espi_taf_nxp_config *config = dev->config;
	struct espi_taf_nxp_data *data = dev->data;
	ESPI_Type *base = config->base;
	/* buff[0..3] - 4-byte flash address (big-endian)
	 * buff[4..]  - write payload (WriteRequest only)
	 */
	const uint8_t *wr_data = ESPI_GetPortRamBuffer(base, port) + 4;
	int ret;

	LOG_INF("> TAF write req addr 0x%x len %u", data->req.addr,
		data->req.len);

	ESPI_SetFlashCompletion(base, port, data->req.tag,
		(uint32_t)kESPI_SSTCL_SAFReqAccepted,
		(uint32_t)kESPI_SAFReadMiddle);

	ret = flash_write(config->flash_dev, data->req.addr, wr_data,
			  data->req.len);
	if (ret < 0) {
		LOG_ERR("TAF write failed: %d", ret);
		ESPI_SetFlashOpLen(base, port,
			(uint32_t)kESPI_OMFLEN_SAFCompletionFail, 0U);
	} else {
		ESPI_SetFlashOpLen(base, port,
			(uint32_t)kESPI_OMFLEN_SAFCompletionNoData,
			data->req.len);
		LOG_INF("TAF write done");
	}

	ESPI_SetFlashCompletion(base, port, data->req.tag,
		(uint32_t)kESPI_SSTCL_SAFCompletion,
		(uint32_t)kESPI_SAFReadOnly);
}

static void espi_taf_nxp_service_read(const struct device *dev, uint8_t port,
				       bool read_start)
{
	const struct espi_taf_nxp_config *config = dev->config;
	struct espi_taf_nxp_data *data = dev->data;
	ESPI_Type *base = config->base;
	espi_saf_rx_completion_type_t rx_type;
	uint32_t chunk, remaining;
	uint8_t *buf;
	int ret;

	if (read_start) {
		if (data->rd_active) {
			LOG_ERR("TAF read overrun: new request while previous active");
			espi_taf_nxp_send_fail(base, port, data->req.tag);
			return;
		}

		/* Initialize the read cursor from the incoming request. */
		data->rd_active      = true;
		data->rd_port        = port;
		data->rd_tag         = data->req.tag;
		data->rd_addr        = data->req.addr;
		data->rd_total       = data->req.len;
		data->rd_offset      = 0U;
		data->rd_max_payload = ESPI_GetFlashMaxPayload(base);

		LOG_INF("> TAF read req addr 0x%x len %u", data->rd_addr,
			data->rd_total);

		/* Acknowledge the request before sending the first chunk. */
		ESPI_SetFlashCompletion(base, port, data->rd_tag,
			(uint32_t)kESPI_SSTCL_SAFReqAccepted,
			(uint32_t)kESPI_SAFReadMiddle);
	}

	if (!data->rd_active) {
		/* Stale completion notification after the read already finished. */
		return;
	}

	/* For subsequent chunks, use cursor-stored port so the correct port
	 * RAM buffer is addressed even if the notification carries a stale port.
	 */
	port = data->rd_port;

	remaining = data->rd_total - data->rd_offset;
	chunk     = MIN(remaining, data->rd_max_payload);

	/* Determine the completion type for this chunk. */
	if (data->rd_total <= data->rd_max_payload) {
		rx_type = kESPI_SAFReadOnly;
	} else if (data->rd_offset == 0U) {
		rx_type = kESPI_SAFReadFirst;
	} else if (remaining == chunk) {
		rx_type = kESPI_SAFReadLast;
	} else {
		rx_type = kESPI_SAFReadMiddle;
	}

	/* Read flash directly into the TAF port RAM buffer so that
	 * ESPI_SetFlashCompletion sends it to the host without an extra copy.
	 */
	buf = ESPI_GetPortRamBuffer(base, port);
	ret = flash_read(config->flash_dev, data->rd_addr + data->rd_offset,
			 buf, chunk);
	if (ret < 0) {
		LOG_ERR("TAF read failed at offset %u: %d", data->rd_offset,
			ret);
		espi_taf_nxp_send_fail(base, port, data->rd_tag);
		data->rd_active = false;
		return;
	}

	ESPI_SetFlashOpLen(base, port,
		(uint32_t)kESPI_OMFLEN_SAFCompletionWithData, chunk);
	ESPI_SetFlashCompletion(base, port, data->rd_tag,
		(uint32_t)kESPI_SSTCL_SAFCompletion,
		(uint32_t)rx_type);

	LOG_INF("TAF read chunk addr 0x%x len %u",
		data->rd_addr + data->rd_offset, chunk);

	data->rd_offset += chunk;
	if (data->rd_offset >= data->rd_total) {
		LOG_INF("TAF read done");
		data->rd_active = false;
	}
}

static void espi_taf_nxp_work(struct k_work *work)
{
	struct espi_taf_nxp_data *data = CONTAINER_OF(work,
						      struct espi_taf_nxp_data,
						      work);
	const struct device *dev = data->dev;

	if (data->req.type == (uint8_t)kESPI_WRSTAT_EraseRequest) {
		espi_taf_nxp_service_erase(dev, data->req.port);
	} else if (data->req.type == (uint8_t)kESPI_WRSTAT_WriteRequest) {
		espi_taf_nxp_service_write(dev, data->req.port);
	} else if (data->req.type == (uint8_t)kESPI_WRSTAT_ReadRequest) {
		espi_taf_nxp_service_read(dev, data->req.port, data->req.read_start);
	} else {
		/* No action needed. */
	}
}

static void espi_taf_nxp_event_handler(const struct device *parent,
					struct espi_callback *cb,
					struct espi_event event)
{
	struct espi_taf_nxp_data *data = CONTAINER_OF(cb,
						      struct espi_taf_nxp_data,
						      taf_cb);
	const struct espi_taf_nxp_config *config = data->dev->config;
	struct espi_nxp_taf_req *req;
	uint64_t flash_size;

	ARG_UNUSED(parent);

	if (event.evt_type != ESPI_BUS_TAF_NOTIFICATION ||
	    event.evt_details != ESPI_CHANNEL_FLASH) {
		return;
	}

	req = (struct espi_nxp_taf_req *)(uintptr_t)event.evt_data;
	if (req == NULL) {
		return;
	}

	/* Reject requests whose flash access range exceeds the device size.
	 * The RAM buffer bound is not checked here. The over-length payload is
	 * rejected by the eSPI hardware at reception.
	 */
	if ((flash_get_size(config->flash_dev, &flash_size) < 0) ||
	    ((uint64_t)req->addr + req->len > flash_size)) {
		LOG_ERR("TAF request out of range: addr 0x%x len %u",
			req->addr, req->len);
		espi_taf_nxp_send_fail(config->base, req->port, req->tag);
		return;
	}

	/* Reject new requests when the work item is still busy. */
	if (k_work_busy_get(&data->work) != 0) {
		LOG_ERR("TAF request overrun, rejecting with fail completion");
		espi_taf_nxp_send_fail(config->base, req->port, req->tag);
		return;
	}

	data->req = *req;
	k_work_submit(&data->work);
}

static int espi_taf_nxp_configure(const struct device *dev,
				   const struct espi_saf_cfg *cfg)
{
	ARG_UNUSED(dev);

	if (cfg == NULL || cfg->nflash_devices == 0U) {
		return -EINVAL;
	}

	return 0;
}

static int espi_taf_nxp_set_pr(const struct device *dev,
				const struct espi_saf_protection *pr)
{
	ARG_UNUSED(dev);

	if (pr == NULL) {
		return -EINVAL;
	}

	/* Flash protection regions are not implemented on this TAF path. */
	return -EOPNOTSUPP;
}

static int espi_taf_nxp_activate(const struct device *dev)
{
	return 0;
}

static bool espi_taf_nxp_channel_ready(const struct device *dev)
{
	const struct espi_taf_nxp_config *config = dev->config;
	uint32_t reg;

	if (!device_is_ready(config->flash_dev)) {
		return false;
	}

	reg = config->base->ESPICFG;

	return ((reg & ESPI_ESPICFG_FLSHOK_MASK) != 0U) &&
			((reg & ESPI_ESPICFG_SAF_MASK) != 0U);
}

static int espi_taf_nxp_manage_callback(const struct device *dev,
					 struct espi_callback *callback, bool set)
{
	struct espi_taf_nxp_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_taf_nxp_init(const struct device *dev)
{
	const struct espi_taf_nxp_config *config = dev->config;
	struct espi_taf_nxp_data *data = dev->data;

	data->dev       = dev;
	data->rd_active = false;
	k_work_init(&data->work, espi_taf_nxp_work);
	espi_init_callback(&data->taf_cb, espi_taf_nxp_event_handler,
			   ESPI_BUS_TAF_NOTIFICATION);
	espi_add_callback(config->parent, &data->taf_cb);

	return 0;
}

static DEVICE_API(espi_saf, espi_taf_nxp_driver_api) = {
	.config = espi_taf_nxp_configure,
	.set_protection_regions = espi_taf_nxp_set_pr,
	.activate = espi_taf_nxp_activate,
	.get_channel_status = espi_taf_nxp_channel_ready,
	.manage_callback = espi_taf_nxp_manage_callback,
};

#define ESPI_TAF_NXP_INIT(n)							\
	static struct espi_taf_nxp_data espi_taf_nxp_data_##n;			\
										\
	static const struct espi_taf_nxp_config espi_taf_nxp_config_##n = {	\
		.base      = (ESPI_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),	\
		.parent    = DEVICE_DT_GET(DT_INST_PARENT(n)),			\
		.flash_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, flash_dev)),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &espi_taf_nxp_init, NULL,			\
			      &espi_taf_nxp_data_##n,				\
			      &espi_taf_nxp_config_##n,				\
			      POST_KERNEL,					\
			      UTIL_INC(CONFIG_ESPI_INIT_PRIORITY),		\
			      &espi_taf_nxp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESPI_TAF_NXP_INIT)
