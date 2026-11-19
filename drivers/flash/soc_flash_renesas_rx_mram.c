/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/flash.h>

#include <r_flash_api.h>
#include <r_mram.h>
#include <soc.h>

#define DT_DRV_COMPAT renesas_rx_mram_controller

LOG_MODULE_REGISTER(flash_renesas_rx_mram, CONFIG_FLASH_LOG_LEVEL);

/* MACI node shared by the code MRAM and the data MRAM controllers */
#define MRAM_CMD_INTERFACE_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_rx_maci)

struct mram_renesas_rx_cmd_interface {
	mram_instance_ctrl_t mram_ctrl;
	struct st_flash_cfg mram_fsp_config;
	struct k_mutex interface_lock;
};

enum mram_region {
	CODE_MRAM,
	DATA_MRAM,
};

struct mram_renesas_rx_data {
	const struct device *cmd_interface_dev;
	enum mram_region mram_region;
	uint32_t area_address;
	uint32_t area_size;
};

struct mram_renesas_rx_config {
	struct flash_parameters mram_parameters;
	size_t erase_block_size;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout device_page_layout;
#endif
};

static bool mram_rx_valid_range(struct mram_renesas_rx_data *mram_data, off_t offset, size_t len)
{
	if ((offset < 0) || (offset >= mram_data->area_size) ||
	    (mram_data->area_size - offset < len) || (len > UINT32_MAX - offset)) {
		return false;
	}

	return true;
}

/*
 * Code MRAM cannot be accessed while it is written or erased, and the vector table lives in it,
 * so interrupts are masked during its operations. Both regions share one programming interface,
 * so the interface mutex serializes all of them.
 */
static unsigned int mram_rx_op_lock(struct mram_renesas_rx_data *mram_data,
				    struct mram_renesas_rx_cmd_interface *interface)
{
	unsigned int key = 0U;

	k_mutex_lock(&interface->interface_lock, K_FOREVER);

	if (mram_data->mram_region == CODE_MRAM) {
		key = irq_lock();
	}

	return key;
}

static void mram_rx_op_unlock(struct mram_renesas_rx_data *mram_data,
			      struct mram_renesas_rx_cmd_interface *interface, unsigned int key)
{
	if (mram_data->mram_region == CODE_MRAM) {
		irq_unlock(key);
	}

	k_mutex_unlock(&interface->interface_lock);
}

static int mram_rx_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct mram_renesas_rx_data *mram_data = dev->data;
	struct mram_renesas_rx_cmd_interface *interface = mram_data->cmd_interface_dev->data;

	if (len == 0) {
		return 0;
	}

	if (!mram_rx_valid_range(mram_data, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("mram: read 0x%lx, len: %zu", (long)(offset + mram_data->area_address), len);

	k_mutex_lock(&interface->interface_lock, K_FOREVER);

	memcpy(data, (uint8_t *)((uintptr_t)offset + mram_data->area_address), len);

	k_mutex_unlock(&interface->interface_lock);

	return 0;
}

static int mram_rx_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct mram_renesas_rx_data *mram_data = dev->data;
	struct mram_renesas_rx_cmd_interface *interface = mram_data->cmd_interface_dev->data;
	unsigned int key;
	fsp_err_t err;

	if (len == 0) {
		return 0;
	}

	if (!mram_rx_valid_range(mram_data, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("mram: write 0x%lx, len: %zu", (long)(offset + mram_data->area_address), len);

	key = mram_rx_op_lock(mram_data, interface);

	err = R_MRAM_Write(&interface->mram_ctrl, (uint32_t)(uintptr_t)data,
			   (uint32_t)(offset + mram_data->area_address), len);

	mram_rx_op_unlock(mram_data, interface, key);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int mram_rx_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct mram_renesas_rx_config *mram_config = dev->config;
	struct mram_renesas_rx_data *mram_data = dev->data;
	struct mram_renesas_rx_cmd_interface *interface = mram_data->cmd_interface_dev->data;
	unsigned int key;
	fsp_err_t err;
	uint32_t block_num;

	if (size == 0) {
		return 0;
	}

	if (!mram_rx_valid_range(mram_data, offset, size)) {
		return -EINVAL;
	}

	/*
	 * R_MRAM_Erase() operates on whole erase blocks; offset and size must both
	 * be aligned to the block size.
	 */
	if (((offset % mram_config->erase_block_size) != 0) ||
	    ((size % mram_config->erase_block_size) != 0)) {
		return -EINVAL;
	}

	block_num = DIV_ROUND_UP(size, mram_config->erase_block_size);

	LOG_DBG("mram: erase 0x%lx, len: %zu", (long)(offset + mram_data->area_address), size);

	key = mram_rx_op_lock(mram_data, interface);

	err = R_MRAM_Erase(&interface->mram_ctrl, (uint32_t)(offset + mram_data->area_address),
			   block_num);

	mram_rx_op_unlock(mram_data, interface, key);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static const struct flash_parameters *mram_rx_get_parameters(const struct device *dev)
{
	const struct mram_renesas_rx_config *mram_config = dev->config;

	return &mram_config->mram_parameters;
}

static int mram_rx_get_size(const struct device *dev, uint64_t *size)
{
	struct mram_renesas_rx_data *mram_data = dev->data;

	*size = (uint64_t)mram_data->area_size;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void mram_rx_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	const struct mram_renesas_rx_config *mram_config = dev->config;

	*layout = &mram_config->device_page_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static struct mram_renesas_rx_cmd_interface mram_renesas_rx_cmd_interface = {
	.mram_fsp_config = {
		.data_flash_bgo = false,
		.irq = FSP_INVALID_VECTOR,
		.err_irq = FSP_INVALID_VECTOR,
		.ipl = BSP_IRQ_DISABLED,
		.err_ipl = BSP_IRQ_DISABLED,
		.p_callback = NULL,
	}};

static int mram_rx_cmd_interface_init(const struct device *dev)
{
	struct mram_renesas_rx_cmd_interface *data = dev->data;
	fsp_err_t err;

	k_mutex_init(&data->interface_lock);

	err = R_MRAM_Open(&data->mram_ctrl, &data->mram_fsp_config);
	if (err != FSP_SUCCESS) {
		LOG_DBG("mram: open error=%d", (int)err);
		return -EIO;
	}

	return 0;
}

DEVICE_DT_DEFINE(MRAM_CMD_INTERFACE_NODE, mram_rx_cmd_interface_init, NULL,
		 &mram_renesas_rx_cmd_interface, NULL, PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY,
		 NULL);

static int mram_rx_init(const struct device *dev)
{
	struct mram_renesas_rx_data *mram_data = dev->data;

	if (!device_is_ready(mram_data->cmd_interface_dev)) {
		return -ENODEV;
	}

	mram_data->mram_region = (mram_data->area_address >= BSP_FLASH_CODE_FLASH_START_ADDRESS)
					 ? CODE_MRAM
					 : DATA_MRAM;

	return 0;
}

static DEVICE_API(flash, mram_rx_api) = {
	.erase = mram_rx_erase,
	.write = mram_rx_write,
	.read = mram_rx_read,
	.get_parameters = mram_rx_get_parameters,
	.get_size = mram_rx_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = mram_rx_page_layout,
#endif
};

#ifdef CONFIG_FLASH_PAGE_LAYOUT
#define MRAM_RX_PAGE_LAYOUT(nv_node)                                                               \
	.device_page_layout = {                                                                    \
		.pages_count = (DT_RANGES_LENGTH_BY_IDX(nv_node, 0) /                              \
				DT_PROP(nv_node, erase_block_size)),                               \
		.pages_size = DT_PROP(nv_node, erase_block_size),                                  \
	},
#else
#define MRAM_RX_PAGE_LAYOUT(nv_node)
#endif

#define MRAM_RX_INIT_NV(nv_node, index)                                                            \
	static struct mram_renesas_rx_data mram_renesas_rx_data_##index = {                        \
		.cmd_interface_dev = DEVICE_DT_GET(MRAM_CMD_INTERFACE_NODE),                       \
		.area_address = DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(nv_node, 0),                   \
		.area_size = DT_RANGES_LENGTH_BY_IDX(nv_node, 0),                                  \
	};                                                                                         \
	static const struct mram_renesas_rx_config mram_renesas_rx_config_##index = {              \
		.mram_parameters =                                                                 \
			{                                                                          \
				.write_block_size = DT_PROP(nv_node, write_block_size),            \
				.erase_value = 0xff,                                               \
				.caps =                                                            \
					{                                                          \
						.no_explicit_erase = true,                         \
					},                                                         \
			},                                                                         \
		.erase_block_size = DT_PROP(nv_node, erase_block_size),                            \
		MRAM_RX_PAGE_LAYOUT(nv_node)};                                                     \
	DEVICE_DT_DEFINE(DT_DRV_INST(index), mram_rx_init, NULL, &mram_renesas_rx_data_##index,    \
			 &mram_renesas_rx_config_##index, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, \
			 &mram_rx_api);

#define MRAM_RX_INIT(index) DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(index, MRAM_RX_INIT_NV, index)

DT_INST_FOREACH_STATUS_OKAY(MRAM_RX_INIT);
