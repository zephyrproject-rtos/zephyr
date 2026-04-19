/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Mailbox property interface:
 *   https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 *
 */

#define DT_DRV_COMPAT raspberrypi_bcm2711_videocore

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mbox.h>

#include "rpi_fw.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rpi_firmware, CONFIG_LOG_DEFAULT_LEVEL);

#define RPI_FW_RESPONSE_TIMEOUT 100U
#define RPI_FW_RESPONSE_STATUS  1U
#define RPI_FW_RESPONSE_TAG     2U
#define RPI_FW_BUF_OVERHEAD     12U

struct rpi_fw_tag_header {
	uint32_t tag;
	uint32_t data_size;
	uint32_t reserved;
};

struct rpi_fw_config {
	struct mbox_dt_spec mbox;
};

struct rpi_fw_data {
	const uint32_t *shm;
	size_t shm_size;
	struct k_mutex lock;
	struct k_sem reply;
};

static inline bool rpi_fw_is_response_success(const uint32_t *shm)
{
	return shm[1] == RPI_FW_REQUEST_SUCCESS;
}

static void rpi_fw_mbox_cb(const struct device *mbox_dev, mbox_channel_id_t channel,
			   void *user_data, struct mbox_msg *msg)
{
	ARG_UNUSED(mbox_dev);
	ARG_UNUSED(channel);

	struct rpi_fw_data *data = (struct rpi_fw_data *)user_data;

	data->shm = msg->data;
	data->shm_size = msg->size;

	k_sem_give(&data->reply);
}

static int rpi_fw_transaction(const struct device *dev, struct mbox_msg *msg)
{
	const struct rpi_fw_config *config = dev->config;
	struct rpi_fw_data *data = dev->data;
	int ret;

	k_sem_reset(&data->reply);

	ret = mbox_send_dt(&config->mbox, msg);
	if (ret < 0) {
		LOG_ERR("Failed to send mailbox message (%d)", ret);
		return ret;
	}

	ret = k_sem_take(&data->reply, K_MSEC(RPI_FW_RESPONSE_TIMEOUT));
	if (ret < 0) {
		LOG_ERR("Firmware transaction timed out (%d)", ret);
		k_sem_give(&data->reply);
		return -ETIMEDOUT;
	}

	return 0;
}

int rpi_fw_transfer(const struct device *dev, uint32_t tag, void *data, uint32_t data_size)
{
	struct rpi_fw_data *fw_data = dev->data;
	uint32_t hdr_size = sizeof(struct rpi_fw_tag_header);
	uint32_t buf_size = hdr_size + data_size + RPI_FW_BUF_OVERHEAD;
	struct rpi_fw_tag_header *hdr;
	uint32_t buf[buf_size / 4U];
	struct mbox_msg msg;
	int ret;

	if (data == NULL || data_size == 0) {
		LOG_ERR("Invalid buffer size %d", data_size);
		return -EINVAL;
	}

	if (buf_size & 3U) {
		LOG_ERR("VideCore buffer %d is not a multiple of 4", buf_size);
		return -EINVAL;
	}

	k_mutex_lock(&fw_data->lock, K_FOREVER);

	hdr = (struct rpi_fw_tag_header *)&buf[2];
	hdr->tag = tag;
	hdr->data_size = data_size;
	hdr->reserved = 0;

	memcpy((uint8_t *)hdr + hdr_size, data, data_size);

	buf[0] = buf_size;
	buf[1] = RPI_FW_REQUEST_PROCESS;
	buf[buf_size / 4U - 1U] = RPI_FW_TAG_END;

	msg.data = buf;
	msg.size = buf_size;

	ret = rpi_fw_transaction(dev, &msg);
	if (ret < 0) {
		k_mutex_unlock(&fw_data->lock);
		return ret;
	}

	if (rpi_fw_is_response_success(fw_data->shm) == false) {
		LOG_ERR("Firmware transaction failed: status=0x%08x",
			fw_data->shm[RPI_FW_RESPONSE_STATUS]);
		k_mutex_unlock(&fw_data->lock);
		return ret;
	}

	memcpy(data, (uint8_t *)&fw_data->shm[RPI_FW_RESPONSE_TAG] + hdr_size, data_size);

	k_mutex_unlock(&fw_data->lock);
	return 0;
}

static int rpi_fw_init(const struct device *dev)
{
	const struct rpi_fw_config *config = dev->config;
	struct rpi_fw_data *data = dev->data;
	const struct device *mbox_dev = config->mbox.dev;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->reply, 0, 1);

	if (!device_is_ready(mbox_dev)) {
		LOG_ERR("mbox device %s is not ready", mbox_dev->name);
		return -ENODEV;
	}

	ret = mbox_register_callback_dt(&config->mbox, rpi_fw_mbox_cb, data);
	if (ret) {
		LOG_ERR("Failed to register mailbox callback (%d)", ret);
		return ret;
	}

	ret = mbox_set_enabled_dt(&config->mbox, true);
	if (ret) {
		LOG_ERR("Failed to enable mailbox (%d)", ret);
		return ret;
	}

	LOG_DBG("Raspberry Pi VideoCore firmware is ready");

	return 0;
}

#define RPI_FW_DEFINE(n)                                                                           \
	static struct rpi_fw_data rpi_fw_data_##n;                                                 \
                                                                                                   \
	static const struct rpi_fw_config rpi_fw_cfg_##n = {                                       \
		.mbox = MBOX_DT_SPEC_INST_GET(n, vc),                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, rpi_fw_init, NULL, &rpi_fw_data_##n, &rpi_fw_cfg_##n,             \
			      POST_KERNEL, CONFIG_RPI_VIDEOCORE_FIRMWARE_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(RPI_FW_DEFINE)
