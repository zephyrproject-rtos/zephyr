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

/*
 * Some VC firmware tags (SET_POWER_STATE, clock changes) can take
 * several hundred ms to complete -- the firmware brings up real
 * clocks / PHYs before responding. Matches the 1-second polling
 * window the pre-rpi_fw bcm2835 mailbox driver used in production.
 */
#define RPI_FW_RESPONSE_TIMEOUT 1000U
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

int rpi_fw_fb_setup(const struct device *dev, uint32_t *width, uint32_t *height, uint32_t *depth,
		    uint32_t *pixel_order, uint32_t alignment, uintptr_t *fb_bus, uint32_t *fb_size,
		    uint32_t *pitch)
{
	if (width == NULL || height == NULL || depth == NULL || pixel_order == NULL ||
	    fb_bus == NULL || fb_size == NULL || pitch == NULL) {
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	const struct rpi_fw_config *fw_config = dev->config;
	struct rpi_fw_data *fw_data = dev->data;
	/*
	 * volatile so each word is a single 32-bit store. The shm is mapped
	 * Device-nGnRnE (K_MEM_CACHE_NONE); without volatile the optimiser
	 * merges this contiguous run of stores into 64-bit STUR/STP pairs,
	 * which alignment-fault at the 4-byte-aligned tag offsets.
	 */
	volatile uint32_t *buf = (volatile uint32_t *)fw_data->shm;
	struct mbox_msg msg;
	int ret;

	k_mutex_lock(&fw_data->lock, K_FOREVER);

	/*
	 * Chained property request. Each tag is {id, value-buffer bytes,
	 * request code = 0, value words...}; VC writes responses back into
	 * the same value words. The setup tags must share one request --
	 * VC clamps SET_VIRTUAL_SIZE / SET_DEPTH to minimums when
	 * ALLOCATE_BUFFER lands in a separate message.
	 *
	 *   [0]      total bytes
	 *   [1]      request code
	 *   [2..6]   SET_PHYSICAL_SIZE : id, 8, 0, W, H
	 *   [7..11]  SET_VIRTUAL_SIZE  : id, 8, 0, W, H
	 *   [12..15] SET_DEPTH         : id, 4, 0, bpp
	 *   [16..19] SET_PIXEL_ORDER   : id, 4, 0, order
	 *   [20..24] ALLOCATE_BUFFER   : id, 8, 0, alignment, (size out)
	 *   [25..28] GET_PITCH         : id, 4, 0, (pitch out)
	 *   [29]     end tag
	 */
	buf[1] = RPI_FW_REQUEST_PROCESS;

	buf[2] = RPI_FW_TAG_FB_SET_PHYSICAL_SIZE;
	buf[3] = 8U;
	buf[4] = 0U;
	buf[5] = *width;
	buf[6] = *height;

	buf[7] = RPI_FW_TAG_FB_SET_VIRTUAL_SIZE;
	buf[8] = 8U;
	buf[9] = 0U;
	buf[10] = *width;
	buf[11] = *height;

	buf[12] = RPI_FW_TAG_FB_SET_DEPTH;
	buf[13] = 4U;
	buf[14] = 0U;
	buf[15] = *depth;

	buf[16] = RPI_FW_TAG_FB_SET_PIXEL_ORDER;
	buf[17] = 4U;
	buf[18] = 0U;
	buf[19] = *pixel_order;

	buf[20] = RPI_FW_TAG_FB_ALLOCATE_BUFFER;
	buf[21] = 8U;
	buf[22] = 0U;
	buf[23] = alignment;
	buf[24] = 0U;

	buf[25] = RPI_FW_TAG_FB_GET_PITCH;
	buf[26] = 4U;
	buf[27] = 0U;
	buf[28] = 0U;

	buf[29] = RPI_FW_TAG_END;

	buf[0] = 30U * sizeof(uint32_t);

	/* All shared memory addresses exchanged via the mailbox must be
	 * physical bus addresses; the VideoCore has no view of the ARM
	 * virtual address space.
	 */
	msg.data = (void *)fw_config->shm;
	msg.size = sizeof(uint32_t);

	ret = rpi_fw_transaction(dev, &msg);
	if (ret < 0) {
		goto release_lock;
	}

	if (!rpi_fw_is_response_success((uint32_t *)buf)) {
		LOG_ERR("Firmware rejected fb chain: status=0x%08x", buf[1]);
		ret = -EIO;
		goto release_lock;
	}

	*width = buf[10]; /* virtual size is what the FB actually is */
	*height = buf[11];
	*depth = buf[15];
	*pixel_order = buf[19];
	*fb_bus = buf[23];
	*fb_size = buf[24];
	*pitch = buf[28];

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
