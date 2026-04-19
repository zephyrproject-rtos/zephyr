/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT raspberrypi_bcm283x_firmware

#include "rpi_fw.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mbox.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rpi_fw, CONFIG_LOG_DEFAULT_LEVEL);

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
	mem_addr_t shm;
	size_t shm_size;
};

struct rpi_fw_data {
	mem_addr_t shm;
	size_t shm_size;
	struct k_mutex lock;
	struct k_sem reply;
};

static inline bool rpi_fw_is_response_success(uint32_t *buf)
{
	return buf[1] == RPI_FW_REQUEST_SUCCESS;
}

static void rpi_fw_mbox_cb(const struct device *mbox_dev, mbox_channel_id_t channel,
			   void *user_data, struct mbox_msg *msg)
{
	ARG_UNUSED(mbox_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(msg);

	struct rpi_fw_data *data = (struct rpi_fw_data *)user_data;

	k_sem_give(&data->reply);
}

static int rpi_fw_transaction(const struct device *dev, struct mbox_msg *msg)
{
	const struct rpi_fw_config *config = dev->config;
	struct rpi_fw_data *data = dev->data;
	int ret;

	ret = mbox_send_dt(&config->mbox, msg);
	if (ret < 0) {
		LOG_ERR("Failed to send mailbox message (%d)", ret);
		return ret;
	}

	ret = k_sem_take(&data->reply, K_MSEC(RPI_FW_RESPONSE_TIMEOUT));
	if (ret < 0) {
		LOG_ERR("Firmware transaction timed out (%d)", ret);
		return -ETIMEDOUT;
	}

	return 0;
}

int rpi_fw_transfer(const struct device *dev, uint32_t tag, void *data, uint32_t data_size)
{
	if (!device_is_ready(dev)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	const struct rpi_fw_config *fw_config = dev->config;
	struct rpi_fw_data *fw_data = dev->data;
	uint32_t hdr_size = sizeof(struct rpi_fw_tag_header);
	uint32_t buf_size = hdr_size + data_size + RPI_FW_BUF_OVERHEAD;
	struct rpi_fw_tag_header *hdr;
	uint32_t *buf = (uint32_t *)fw_data->shm;
	struct mbox_msg msg;
	int ret;

	if (data == NULL || data_size == 0) {
		LOG_ERR("Invalid transfer data");
		return -EINVAL;
	}

	if (buf_size & 3U) {
		LOG_ERR("buffer size is not a multiple of 4");
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

	/* All shared memory addresses exchanged via the mailbox must be
	 * physical bus addresses. The VideoCore has no knowledge of the
	 * virtual address space.
	 */
	msg.data = (void *)fw_config->shm;
	msg.size = sizeof(uint32_t);

	ret = rpi_fw_transaction(dev, &msg);
	if (ret < 0) {
		goto release_lock;
	}

	if (!rpi_fw_is_response_success(buf)) {
		LOG_ERR("Firmware transaction failed: status=0x%08x",
			buf[RPI_FW_RESPONSE_STATUS]);
		ret = -EINVAL;
		goto release_lock;
	}

	memcpy(data, (uint8_t *)&buf[RPI_FW_RESPONSE_TAG] + hdr_size, data_size);

release_lock:
	k_mutex_unlock(&fw_data->lock);
	return ret;
}

static int rpi_fw_init(const struct device *dev)
{
	const struct rpi_fw_config *config = dev->config;
	struct rpi_fw_data *data = dev->data;
	const struct device *mbox_dev = config->mbox.dev;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->reply, 0, 1);

	device_map(&data->shm, config->shm, config->shm_size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);

	if (!device_is_ready(mbox_dev)) {
		LOG_ERR_DEVICE_NOT_READY(mbox_dev);
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
		.shm_size = DT_REG_SIZE(DT_PHANDLE(DT_DRV_INST(n), shared_memory)),                \
		.shm = DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(n), shared_memory)),                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, rpi_fw_init, NULL, &rpi_fw_data_##n, &rpi_fw_cfg_##n,             \
			      POST_KERNEL, CONFIG_RASPBERRYPI_FIRMWARE_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(RPI_FW_DEFINE)
