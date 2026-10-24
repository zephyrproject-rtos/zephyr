/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Flash driver for the BK7258.
 *
 * Why this driver never touches the flash controller
 * -------------------------------------------------
 * The controller registers are mapped and writable from this core; nothing in
 * the hardware prevents it. Issuing a command is still fatal.
 *
 * The flash device is left in quad / continuous-read mode by the core that
 * owns Wi-Fi and BLE. In that mode an XIP fetch sends only an address: the
 * device remembers the last read command instead of receiving it again. Both
 * cores execute in place from this single flash device, so if this core pushes
 * any command into the controller, the device leaves continuous-read mode
 * while the other core is still fetching address-only. That core then executes
 * garbage and the whole chip resets. Measured: a single RDID, which only reads
 * and neither erases nor programs, resets the chip.
 *
 * The flash command pipeline is therefore shared, stateful, and needs exactly
 * one owner. Here that owner is the other core, which is always running. This
 * driver is a client of its flash service over the inter-core mailbox. The
 * vendor SDK does the same thing in its own flash client.
 *
 * Why reads do not go through the XIP window
 * ------------------------------------------
 * The window at 0x02000000 is where this core executes from, so reading
 * through it looks free. It is not usable as a general read path.
 *
 * The window serves the packed image area: the packaging tool interleaves the
 * image as 32 bytes of data followed by 2 bytes of CRC, and the hardware
 * consumes those CRC bytes as it fetches. Anything that was not written that
 * way -- a partition the service programmed at run time, an erased sector --
 * has no matching CRC, and reading it through the window takes the whole chip
 * down. measured:
 *
 *	physical 0x165000 (inside the packed image) read through the window: ok
 *	physical 0x290000 (a data partition) read through the window:
 *	 whole chip resets, both
 *	 before and after erasing
 *	 that sector
 *	physical 0x7FA000 (EasyFlash ENV) read through the window:
 *	 whole chip resets
 *
 * There is no way to tell from an address whether the bytes behind it carry a
 * CRC, so this driver never reads through the window; every read is a request
 * to the service, which reads the bytes itself and hands them back. The
 * earlier "xip-limit" boundary (see the SoC devicetree) was a misreading of
 * this: 0x7F9000 is not the highest readable offset, it is simply one that
 * happens to sit in a packed area.
 *
 * Offsets, and what a read returns
 * -------------------------------
 * Offsets are physical, matching the partition table and the image slots, and
 * so is what a read returns: the raw byte stream, in which a group is 34
 * bytes (32 of data and 2 of CRC). The service reads physical bytes and so
 * does its own caller in the vendor SDK, and erase is physical as well, so a
 * read, a program and an erase all agree on what offset N means.
 *
 * The consequence is that the bytes this driver returns are not the bytes the
 * core would see at the same range through the XIP window: to get those, drop
 * the two CRC bytes of every 34-byte group. Measured: reading
 * physical 0x165000 returns the vector table in its first 32 bytes and the
 * first CRC byte of that group at byte 32, where the window has the first
 * byte of the next group.
 *
 * An image that has to be executed, as opposed to stored, therefore has to be
 * written already interleaved, which is what the packaging tool does; that is
 * a job for whatever writes images, not for this driver.
 *
 * The proxy
 * ---------
 * Program and erase are one round trip each; a read is two, because the
 * service holds its buffer until the client says it is done with it:
 *
 *	this core sends a command carrying a 16-byte request block that
 *	 lives in shared memory
 *	the service answers twice: once with the response flag set, which
 *	 acknowledges the request and is not waited for,
 *	 and once with a command carrying the result
 *	after a read the client sends READ_DONE, exactly once, and the
 *	 service answers that with a result as well
 *
 * Only a command without the response flag means the operation finished. The
 * acknowledgment carries the same service command number and an empty
 * payload, so matching on the command number alone ends a wait before the
 * flash has been touched at all. Every wait here therefore matches the
 * command number *and* the service command number.
 *
 * Sending READ_DONE twice, or not at all, leaves the service waiting inside
 * its own 3 s timeout with the flash server wedged, so the read path sends it
 * exactly once per read.
 *
 * Every request also has to carry a CRC-8 of its own payload, in the byte of
 * the command block that the mailbox layer reserves for it. The system controller
 * recomputes it over the payload before it hands the request to the service
 * and drops the request when it does not match, so a request without it is
 * never executed at all.
 *
 * A command pushed by the service has to be answered with a response. The SoC
 * layer sends that from the receive interrupt, which is the only place early
 * enough to land inside the sender's window; nothing here sends it.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <string.h>

#include <soc.h>

LOG_MODULE_REGISTER(flash_bk7258, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT beken_bk7258_flash

/* Mailbox command numbers, and the flag the service sets on a response. */
#define MB_IPC_CONNECT_CMD 0U
#define MB_IPC_SEND_CMD    2U
#define MB_IPC_RSP_FLAG    0x80U

/* Ports of the flash service. */
#define IPC_SERVER_CPU  0U
#define IPC_SERVER_PORT 1U
#define IPC_CLIENT_CPU  1U
#define IPC_CLIENT_PORT 16U

/* Service commands. */
#define FLASH_IPC_ERASE_SECTOR 0U
#define FLASH_IPC_READ         3U
#define FLASH_IPC_READ_DONE    4U
#define FLASH_IPC_WRITE        5U

/*
 * Longest read the service will do: it reads into one buffer of
 * FLASH_IPC_READ_SIZE bytes and refuses a longer request, which also wedges
 * the server for its own 3 s timeout. Writes have no such limit, but the same
 * size is used for both so one buffer serves both directions.
 */
#define FLASH_IPC_CHUNK 512U

/*
 * Memory the system controller can read. The request block, the payload buffer the service
 * reads a write out of, and the result block the service points back at all
 * have to live there.
 */
#define BK_SRAM_BASE  0x28000000U
#define BK_SRAM_END   0x280A0000U
#define BK_PSRAM_BASE 0x60000000U
#define BK_PSRAM_END  0x61000000U

/* 16 bytes, laid out exactly as the service expects it. */
struct flash_ipc_cmd {
	uint32_t part_addr; /* part_id[7:0] and addr[31:8] */
	uint8_t *buff;
	uint16_t len;
	int16_t ret_status;
	uint32_t crc;
};

struct flash_bk7258_config {
	uint32_t size;
	uint32_t sector_size;
	uint32_t timeout_ms;
};

struct flash_bk7258_data {
	struct k_mutex lock;          /* one round trip at a time */
	struct k_sem arrived;         /* something came back from the service */
	struct flash_ipc_cmd req;     /* request block, in shared SRAM */
	struct flash_ipc_cmd result;  /* block the service pushed back */
	uint8_t buf[FLASH_IPC_CHUNK]; /* payload, in shared SRAM */
	uint32_t rx_cmd;
	uint32_t rx_uc;
	uint8_t tag;
	uint8_t seq;
	bool connected;
};

/*
 * The receive callback takes no context, and this driver owns the mailbox
 * channel it filters on, so the single instance is reached through one
 * pointer. Registering a second instance would replace it.
 */
static struct flash_bk7258_data *bk_flash_data;

/*
 * CRC-8 over the payload of a request, polynomial 0x31, seed 0.
 *
 * The mailbox layer reserves one byte of the command block for it and the system controller
 * recomputes it over the payload before the request reaches the flash service.
 * A request carrying the wrong value -- including 0, which is what an
 * uninitialised block looks like -- is dropped with -MB_IPC_RX_DATA_FAILED and
 * the flash is never touched. It also leaves the system controller's socket holding a
 * command it never delivered, so the next request is dropped as well.
 */
static uint8_t bk_crc8(const uint8_t *p, uint16_t len)
{
	uint8_t crc = 0U;

	for (uint16_t i = 0U; i < len; i++) {
		crc ^= p[i];
		for (int b = 0; b < 8; b++) {
			crc = (uint8_t)((crc << 1) ^ ((crc & 0x80U) ? 0x31U : 0U));
		}
	}

	return crc;
}

/*
 * CRC32 the way the flash service computes it: seed 0, and no final inversion.
 * The service refuses a write whose crc does not match, and this is the one it
 * recomputes over the payload before programming it. It also fills this in for
 * a read, which is what makes it worth checking here: a mismatch means the
 * bytes were corrupted on their way through shared memory.
 */
static uint32_t bk_crc32(const uint8_t *p, uint32_t len)
{
	uint32_t crc = 0U;

	for (uint32_t i = 0U; i < len; i++) {
		crc ^= p[i];
		for (int b = 0; b < 8; b++) {
			crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)(-(int32_t)(crc & 1U)));
		}
	}

	return crc;
}

static bool sc_readable(const void *p)
{
	uint32_t a = (uint32_t)p;

	return ((a >= BK_SRAM_BASE) && (a < BK_SRAM_END)) ||
	       ((a >= BK_PSRAM_BASE) && (a < BK_PSRAM_END));
}

static void mbox_rx_cb(uint32_t hdr, uint32_t w1, uint32_t w2, uint32_t w3, uint32_t nbytes)
{
	struct flash_bk7258_data *data = bk_flash_data;
	uint32_t len = w2 & 0xFFFFU;

	ARG_UNUSED(w1);
	ARG_UNUSED(nbytes);

	if (data == NULL) {
		return;
	}

	/*
	 * Only what the service pushes on its own channel, and only commands:
	 * an ACK is the system controller confirming something this core sent.
	 */
	if (((hdr >> 24) != SOC_MB_CHNL_CP_IPC) || SOC_MB_HDR_IS_ACK(hdr)) {
		return;
	}

	data->rx_cmd = hdr & 0xFFU;
	data->rx_uc = (w2 >> 24) & 0xFFU;

	if ((w3 != 0U) && (len != 0U) && sc_readable((const void *)w3)) {
		memcpy(&data->result, (const void *)w3, MIN(len, (uint32_t)sizeof(data->result)));
	}

	k_sem_give(&data->arrived);
}

static int ipc_post(struct flash_bk7258_data *data, uint32_t cmd, uint32_t user_cmd,
		    const void *payload, uint32_t len)
{
	uint32_t block[4];

	/* Tags must not repeat: the system controller drops a reply whose tag it has seen. */
	if (data->tag == 0U) {
		data->tag = 1U;
	}

	block[0] =
		(cmd & 0xFFU) | (((uint32_t)data->seq++ & 0xFFU) << 16) | (SOC_MB_CHNL_IPC << 24);
	block[1] = (IPC_SERVER_PORT & 0x3FU) | ((IPC_SERVER_CPU & 0x3U) << 6) |
		   ((IPC_CLIENT_PORT & 0x3FU) << 8) | ((IPC_CLIENT_CPU & 0x3U) << 14) |
		   (((uint32_t)data->tag++ & 0xFFU) << 16);
	block[2] = (len & 0xFFFFU) |
		   (((uint32_t)bk_crc8((const uint8_t *)payload, (uint16_t)len)) << 16) |
		   ((user_cmd & 0xFFU) << 24);
	block[3] = (uint32_t)payload;

	return soc_bk7258_mbox_post(block, SOC_MB_IPC_MSG_LEN);
}

static int ipc_wait(struct flash_bk7258_data *data, uint32_t cmd, uint32_t user_cmd,
		    uint32_t timeout_ms)
{
	for (uint32_t waited = 0U; waited < timeout_ms; waited++) {
		soc_bk7258_mbox_pump();

		if (k_sem_take(&data->arrived, K_MSEC(1)) != 0) {
			continue;
		}

		if ((data->rx_cmd == cmd) && (data->rx_uc == user_cmd)) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int ipc_xfer(const struct device *dev, uint32_t user_cmd)
{
	const struct flash_bk7258_config *cfg = dev->config;
	struct flash_bk7258_data *data = dev->data;
	int rc;

	k_sem_reset(&data->arrived);

	rc = ipc_post(data, MB_IPC_SEND_CMD, user_cmd, &data->req, sizeof(data->req));
	if (rc != 0) {
		LOG_ERR("cannot post service command %u: %d", user_cmd, rc);
		return -EIO;
	}

	rc = ipc_wait(data, MB_IPC_SEND_CMD, user_cmd, cfg->timeout_ms);
	if (rc != 0) {
		LOG_ERR("no result for service command %u in %u ms", user_cmd, cfg->timeout_ms);
		return -EIO;
	}

	return 0;
}

/*
 * The service only accepts commands on an established connection, and it is
 * not ready the moment this core boots, so connecting is retried on demand
 * rather than once at init.
 */
static int ipc_connect(const struct device *dev)
{
	struct flash_bk7258_data *data = dev->data;
	const struct flash_bk7258_config *cfg = dev->config;
	int rc;

	if (data->connected) {
		return 0;
	}

	k_sem_reset(&data->arrived);

	rc = ipc_post(data, MB_IPC_CONNECT_CMD, 0U, NULL, 0U);
	if (rc != 0) {
		LOG_ERR("cannot post connect: %d", rc);
		return -EIO;
	}

	rc = ipc_wait(data, MB_IPC_CONNECT_CMD | MB_IPC_RSP_FLAG, 0U, cfg->timeout_ms);
	if (rc != 0) {
		LOG_ERR("flash service did not answer connect in %u ms", cfg->timeout_ms);
		return -EIO;
	}

	data->connected = true;

	return 0;
}

static int ipc_read_chunk(const struct device *dev, off_t off, uint8_t *dst, uint32_t len)
{
	struct flash_bk7258_data *data = dev->data;
	const uint8_t *src;
	int rc;

	memset(&data->req, 0, sizeof(data->req));
	data->req.part_addr = (uint32_t)off << 8U;
	data->req.len = (uint16_t)len;

	memset(&data->result, 0, sizeof(data->result));

	rc = ipc_xfer(dev, FLASH_IPC_READ);
	if (rc != 0) {
		return rc;
	}

	if (data->result.ret_status != 0) {
		LOG_ERR("read at 0x%lx rejected by the service: %d", (long)off,
			data->result.ret_status);
		return -EIO;
	}

	/*
	 * The service reads into a buffer of its own and hands the address
	 * back, rather than filling the one this core offered.
	 */
	src = data->result.buff;

	if (!sc_readable(src)) {
		LOG_ERR("service returned no readable buffer (0x%p)", src);
		return -EIO;
	}

	if (data->result.crc != bk_crc32(src, len)) {
		LOG_ERR("read at 0x%lx came back with a bad crc", (long)off);
		rc = -EIO;
	} else {
		memcpy(dst, src, len);
	}

	/*
	 * Release the service buffer. Exactly once: a second READ_DONE is
	 * answered by nothing and leaves the server inside its own timeout,
	 * and none at all leaves it waiting for this one. It has to be sent
	 * even when the crc above failed, because by then the service is
	 * already waiting for it.
	 */
	memset(&data->req, 0, sizeof(data->req));
	data->req.part_addr = (uint32_t)off << 8U;
	data->req.len = (uint16_t)len;

	(void)ipc_xfer(dev, FLASH_IPC_READ_DONE);

	return rc;
}

static int flash_bk7258_read(const struct device *dev, off_t off, void *data, size_t len)
{
	const struct flash_bk7258_config *cfg = dev->config;
	struct flash_bk7258_data *drv_data = dev->data;
	uint8_t *dst = data;
	int rc;

	if (len == 0U) {
		return 0;
	}

	if ((off < 0) || ((size_t)off >= cfg->size) || (((size_t)off + len) > cfg->size)) {
		return -EINVAL;
	}

	if (k_mutex_lock(&drv_data->lock, K_FOREVER) != 0) {
		return -EIO;
	}

	rc = ipc_connect(dev);
	if (rc != 0) {
		goto out;
	}

	while (len != 0U) {
		uint32_t n = MIN(len, (size_t)FLASH_IPC_CHUNK);

		rc = ipc_read_chunk(dev, off, dst, n);
		if (rc != 0) {
			goto out;
		}

		dst += n;
		off += (off_t)n;
		len -= n;
	}

out:
	k_mutex_unlock(&drv_data->lock);

	return rc;
}

static int flash_bk7258_write(const struct device *dev, off_t off, const void *data, size_t len)
{
	const struct flash_bk7258_config *cfg = dev->config;
	struct flash_bk7258_data *drv_data = dev->data;
	const uint8_t *src = data;
	int rc;

	if (len == 0U) {
		return 0;
	}

	if ((off < 0) || ((size_t)off >= cfg->size) || (((size_t)off + len) > cfg->size)) {
		return -EINVAL;
	}

	if (k_mutex_lock(&drv_data->lock, K_FOREVER) != 0) {
		return -EIO;
	}

	rc = ipc_connect(dev);
	if (rc != 0) {
		goto out;
	}

	while (len != 0U) {
		uint32_t n = MIN(len, (size_t)FLASH_IPC_CHUNK);

		/*
		 * The service reads the payload out of memory this core can
		 * see, so it is staged here: the caller's buffer may be
		 * anywhere, including somewhere the system controller cannot reach.
		 */
		memcpy(drv_data->buf, src, n);

		memset(&drv_data->req, 0, sizeof(drv_data->req));
		drv_data->req.part_addr = (uint32_t)off << 8U;
		drv_data->req.buff = drv_data->buf;
		drv_data->req.len = (uint16_t)n;
		drv_data->req.crc = bk_crc32(drv_data->buf, n);

		memset(&drv_data->result, 0, sizeof(drv_data->result));

		rc = ipc_xfer(dev, FLASH_IPC_WRITE);
		if (rc != 0) {
			goto out;
		}

		if (drv_data->result.ret_status != 0) {
			LOG_ERR("write at 0x%lx rejected by the service: %d", (long)off,
				drv_data->result.ret_status);
			rc = -EIO;
			goto out;
		}

		src += n;
		off += (off_t)n;
		len -= n;
	}

out:
	k_mutex_unlock(&drv_data->lock);

	return rc;
}

static int flash_bk7258_erase(const struct device *dev, off_t off, size_t len)
{
	const struct flash_bk7258_config *cfg = dev->config;
	struct flash_bk7258_data *drv_data = dev->data;
	int rc;

	if (len == 0U) {
		return 0;
	}

	if ((off < 0) || (((size_t)off % cfg->sector_size) != 0U) ||
	    ((len % cfg->sector_size) != 0U) || (((size_t)off + len) > cfg->size)) {
		return -EINVAL;
	}

	if (k_mutex_lock(&drv_data->lock, K_FOREVER) != 0) {
		return -EIO;
	}

	rc = ipc_connect(dev);
	if (rc != 0) {
		goto out;
	}

	/* The service erases one sector per request. */
	for (size_t done = 0U; done < len; done += cfg->sector_size) {
		memset(&drv_data->req, 0, sizeof(drv_data->req));
		drv_data->req.part_addr = (uint32_t)(off + (off_t)done) << 8U;

		memset(&drv_data->result, 0, sizeof(drv_data->result));

		rc = ipc_xfer(dev, FLASH_IPC_ERASE_SECTOR);
		if (rc != 0) {
			goto out;
		}

		if (drv_data->result.ret_status != 0) {
			LOG_ERR("erase at 0x%lx rejected by the service: %d",
				(long)(off + (off_t)done), drv_data->result.ret_status);
			rc = -EIO;
			goto out;
		}
	}

out:
	k_mutex_unlock(&drv_data->lock);

	return rc;
}

static const struct flash_parameters *flash_bk7258_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters params = {
		.write_block_size = 1U,
		.erase_value = 0xFFU,
	};

	return &params;
}

static int flash_bk7258_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_bk7258_config *cfg = dev->config;

	if (size == NULL) {
		return -EINVAL;
	}

	*size = (uint64_t)cfg->size;

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_bk7258_page_layout(const struct device *dev,
				     const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct flash_bk7258_config *cfg = dev->config;
	static struct flash_pages_layout bk_layout;

	ARG_UNUSED(dev);

	bk_layout.pages_count = cfg->size / cfg->sector_size;
	bk_layout.pages_size = cfg->sector_size;

	*layout = &bk_layout;
	*layout_size = 1U;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_bk7258_init(const struct device *dev)
{
	struct flash_bk7258_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_sem_init(&data->arrived, 0U, 1U);
	data->connected = false;

	bk_flash_data = data;
	soc_bk7258_mbox_set_rx_cb(mbox_rx_cb);

	return 0;
}

static DEVICE_API(flash, flash_bk7258_api) = {
	.read = flash_bk7258_read,
	.write = flash_bk7258_write,
	.erase = flash_bk7258_erase,
	.get_parameters = flash_bk7258_get_parameters,
	.get_size = flash_bk7258_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_bk7258_page_layout,
#endif
};

#define BK_FLASH_INST(inst)                                                                        \
	static struct flash_bk7258_data flash_bk7258_data_##inst;                                  \
	static const struct flash_bk7258_config flash_bk7258_cfg_##inst = {                        \
		.size = DT_INST_PROP(inst, size),                                                  \
		.sector_size = CONFIG_FLASH_BK7258_ERASE_BLOCK_SIZE,                               \
		.timeout_ms = CONFIG_FLASH_BK7258_IPC_TIMEOUT_MS,                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, flash_bk7258_init, NULL, &flash_bk7258_data_##inst,            \
			      &flash_bk7258_cfg_##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,   \
			      &flash_bk7258_api);

DT_INST_FOREACH_STATUS_OKAY(BK_FLASH_INST)
