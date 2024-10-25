/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <ipc/ipc_based_driver.h>

#define DT_DRV_COMPAT       telink_w91_flash_controller

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_w91);

#define FLASH_SIZE          DT_REG_SIZE(DT_INST(0, soc_nv_flash))
#define FLASH_ORIGIN        DT_REG_ADDR(DT_INST(0, soc_nv_flash))
#define FLASH_BLOCK_SIZE    (0x1000u)

enum {
	IPC_DISPATCHER_FLASH_ERASE = IPC_DISPATCHER_FLASH,
	IPC_DISPATCHER_FLASH_WRITE,
	IPC_DISPATCHER_FLASH_READ,
	IPC_DISPATCHER_FLASH_GET_ID,
};

struct flash_w91_config {
	const struct flash_parameters parameters;
	uint8_t instance_id;            /* instance id */
};

/* driver data structure */
struct flash_w91_data {
	struct ipc_based_driver ipc;    /* ipc driver part */
};

struct flash_w91_erase_req {
	uint32_t offset;
	uint32_t len;
};

struct flash_w91_write_req {
	uint32_t offset;
	uint32_t len;
	uint8_t *buffer;
};

#define FLASH_WRITE_MAX_SIZE_IN_PACK                                                            \
((CONFIG_PBUF_RX_READ_BUF_SIZE) - sizeof(uint32_t) - sizeof(uint32_t) - sizeof(uint32_t))

struct flash_w91_read_req {
	uint32_t offset;
	uint32_t len;
};

struct flash_w91_read_resp {
	int err;
	uint32_t len;
	uint8_t *buffer;
};

#define FLASH_READ_MAX_SIZE_IN_PACK                                                        \
((CONFIG_PBUF_RX_READ_BUF_SIZE) - sizeof(uint32_t) - sizeof(int) - sizeof(uint32_t))

#define FLASH_CHIP_MAX_ID_LEN (uint8_t)6
struct flash_w91_get_id_resp {
	int err;
	uint8_t chip_id_len;
	uint8_t chip_id[FLASH_CHIP_MAX_ID_LEN];
};

/* API implementation: driver initialization */
static int flash_w91_init(const struct device *dev)
{
	struct flash_w91_data *data = dev->data;

	ipc_based_driver_init(&data->ipc);

	return 0;
}

/* APIs implementation: flash erase */
static size_t pack_flash_w91_erase(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct flash_w91_erase_req *p_read_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_read_req->offset) + sizeof(p_read_req->len);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_FLASH_ERASE, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_read_req->offset);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_read_req->len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(flash_w91_erase);

static int flash_w91_erase(const struct device *dev, off_t offset, size_t len)
{
	int err;
	struct flash_w91_erase_req erase_req;

	erase_req.offset = offset;
	erase_req.len = len;

	struct ipc_based_driver *ipc_data = &((struct flash_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct flash_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
			flash_w91_erase, &erase_req, &err,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (err < 0) {
		LOG_ERR("Flash erase operation failed");
	}

	return err;
}

/* API implementation: write */
static size_t pack_flash_w91_write(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct flash_w91_write_req *p_write_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) + sizeof(p_write_req->offset) +
			sizeof(p_write_req->len) + p_write_req->len;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_FLASH_WRITE, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_write_req->offset);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_write_req->len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_write_req->buffer, p_write_req->len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(flash_w91_write);

static int flash_w91_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	int err;
	struct flash_w91_write_req write_req = {
		.buffer	= (uint8_t *)data,
	};

	write_req.offset = offset;

	struct ipc_based_driver *ipc_data = &((struct flash_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct flash_w91_config *)dev->config)->instance_id;

	while (len) {
		if (len > FLASH_WRITE_MAX_SIZE_IN_PACK) {
			write_req.len = FLASH_WRITE_MAX_SIZE_IN_PACK;
		} else {
			write_req.len = len;
		}

		IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
				flash_w91_write, &write_req, &err,
				CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

		if (err || (len <= FLASH_WRITE_MAX_SIZE_IN_PACK)) {
			break;
		}

		write_req.offset += write_req.len;
		write_req.buffer += write_req.len;
		len -= write_req.len;
	}

	if (err < 0) {
		LOG_ERR("Flash write operation failed");
	}

	return err;
}

/* APIs implementation: flash read */
static size_t pack_flash_w91_read(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct flash_w91_read_req *p_read_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_read_req->offset) + sizeof(p_read_req->len);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_FLASH_READ, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_read_req->offset);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_read_req->len);
	}

	return pack_data_len;
}

static void unpack_flash_w91_read(void *unpack_data, const uint8_t *pack_data, size_t pack_data_len)
{
	struct flash_w91_read_resp *p_read_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_read_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_read_resp->len);

	size_t expect_len = sizeof(uint32_t) + sizeof(p_read_resp->err) +
			sizeof(p_read_resp->len) + p_read_resp->len;

	if (expect_len != pack_data_len) {
		p_read_resp->err = -EINVAL;
		return;
	}

	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_read_resp->buffer, p_read_resp->len);
}

static int flash_w91_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	/* return SUCCESS if len equals 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	struct flash_w91_read_req read_req;
	struct flash_w91_read_resp read_resp = {
		.buffer	= data,
	};

	read_req.offset = offset;

	struct ipc_based_driver *ipc_data = &((struct flash_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct flash_w91_config *)dev->config)->instance_id;

	while (len) {
		if (len > FLASH_READ_MAX_SIZE_IN_PACK) {
			read_req.len = FLASH_READ_MAX_SIZE_IN_PACK;
		} else {
			read_req.len = len;
		}

		IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
				flash_w91_read, &read_req, &read_resp,
				CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

		if (read_resp.err || (len <= FLASH_READ_MAX_SIZE_IN_PACK)) {
			break;
		}

		read_req.offset += read_req.len;
		read_resp.buffer += read_req.len;
		len -= read_req.len;
	}

	if (read_resp.err < 0) {
		LOG_ERR("Flash read operation failed");
	}

	return read_resp.err;
}

/* APIs implementation: flash get ID */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(flash_w91_get_id, IPC_DISPATCHER_FLASH_GET_ID);

static void unpack_flash_w91_get_id(
	void *unpack_data, const uint8_t *pack_data, size_t pack_data_len)
{
	struct flash_w91_get_id_resp *p_get_id_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_get_id_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_get_id_resp->chip_id_len);

	size_t expect_len = sizeof(uint32_t) + sizeof(p_get_id_resp->err) +
			sizeof(p_get_id_resp->chip_id_len) + p_get_id_resp->chip_id_len;

	if (expect_len == pack_data_len) {
		IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_get_id_resp->chip_id,
									p_get_id_resp->chip_id_len);
	} else {
		p_get_id_resp->err = -EINVAL;
	}
}

int flash_w91_get_id(const struct device *dev, uint8_t *hw_id)
{
	struct ipc_based_driver *ipc_data = &((struct flash_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct flash_w91_config *)dev->config)->instance_id;
	struct flash_w91_get_id_resp read_resp;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
			flash_w91_get_id, NULL, &read_resp,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (read_resp.err != 0) {
		LOG_ERR("Flash get ID operation failed");
	} else if (read_resp.chip_id_len == FLASH_CHIP_MAX_ID_LEN) {
		memcpy(hw_id, read_resp.chip_id, read_resp.chip_id_len);
	}

	return read_resp.err;
}

/* API implementation: get_parameters */
static const struct flash_parameters *flash_w91_get_parameters(const struct device *dev)
{
	const struct flash_w91_config *cfg = dev->config;

	return &cfg->parameters;
}

/* API implementation: page_layout */
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = FLASH_SIZE / FLASH_BLOCK_SIZE,
	.pages_size = FLASH_BLOCK_SIZE,
};

static void flash_w91_pages_layout(const struct device *dev,
				   const struct flash_pages_layout **layout,
				   size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_w91_api = {
	.erase = flash_w91_erase,
	.write = flash_w91_write,
	.read = flash_w91_read,
	.get_parameters = flash_w91_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_w91_pages_layout,
#endif
};

/* Driver registration */
#define FLASH_W91_INIT(n)                                                              \
	static const struct flash_w91_config flash_w91_config_##n = {                      \
		.parameters = {                                                                \
			.write_block_size = DT_PROP(DT_INST(0, soc_nv_flash), write_block_size),   \
			.erase_value = 0xff,                                                       \
		},                                                                             \
		.instance_id = n,                                                              \
	};                                                                                 \
	                                                                                   \
	static struct flash_w91_data flash_data_##n;                                       \
	                                                                                   \
	DEVICE_DT_INST_DEFINE(n, flash_w91_init,                                           \
			NULL,                                                                      \
			&flash_data_##n,                                                           \
			&flash_w91_config_##n,                                                     \
			POST_KERNEL,                                                               \
			CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,                               \
			&flash_w91_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_W91_INIT)
