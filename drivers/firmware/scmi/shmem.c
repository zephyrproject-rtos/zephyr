/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(arm_scmi_shmem);

#define DT_DRV_COMPAT arm_scmi_shmem

#ifndef DEVICE_MMIO_IS_IN_RAM
#define device_map(virt, phys, size, flags) *(virt) = (phys)
#endif /* DEVICE_MMIO_IS_IN_RAM */

struct scmi_shmem_config {
	uintptr_t phys_addr;
	uint32_t size;
};

struct scmi_shmem_data {
	mm_reg_t regmap;
};

struct scmi_shmem_layout {
	volatile uint32_t res0;
	volatile uint32_t chan_status;
	volatile uint32_t res1[2];
	volatile uint32_t chan_flags;
	volatile uint32_t len;
	volatile uint32_t msg_hdr;
};

int scmi_shmem_get_channel_status(const struct device *dev, uint32_t *status)
{
	struct scmi_shmem_data *data;
	struct scmi_shmem_layout *layout;

	data = dev->data;
	layout = (struct scmi_shmem_layout *)data->regmap;

	*status = layout->chan_status;

	return 0;
}

static void scmi_shmem_memcpy(mm_reg_t dst, mm_reg_t src, uint32_t bytes)
{
	int i;

	for (i = 0; i < bytes; i++) {
		sys_write8(*(uint8_t *)(src + i), dst + i);
	}
}

int scmi_shmem_read_message(const struct device *shmem, struct scmi_message *msg)
{
	struct scmi_shmem_layout *layout;
	struct scmi_shmem_data *data;
	const struct scmi_shmem_config *cfg;

	data = shmem->data;
	cfg = shmem->config;
	layout = (struct scmi_shmem_layout *)data->regmap;

	/* some sanity checks first */
	if (!msg) {
		return -EINVAL;
	}

	if (!msg->content && msg->len) {
		return -EINVAL;
	}

	if (cfg->size < (sizeof(*layout) + msg->len)) {
		LOG_ERR("message doesn't fit in shmem area");
		return -EINVAL;
	}

	/* mismatch between expected reply size and actual size? */
	if (msg->len != (layout->len - sizeof(layout->msg_hdr))) {
		LOG_ERR("bad message len. Expected 0x%x, got 0x%x",
			msg->len,
			(uint32_t)(layout->len - sizeof(layout->msg_hdr)));
		return -EINVAL;
	}

	/* header match? */
	if (layout->msg_hdr != msg->hdr) {
		LOG_ERR("bad message header. Expected 0x%x, got 0x%x",
			msg->hdr, layout->msg_hdr);
		return -EINVAL;
	}

	if (msg->content) {
		scmi_shmem_memcpy(POINTER_TO_UINT(msg->content),
				  data->regmap + sizeof(*layout), msg->len);
	}

	return 0;
}

int scmi_shmem_write_message(const struct device *shmem, struct scmi_message *msg)
{
	struct scmi_shmem_layout *layout;
	struct scmi_shmem_data *data;
	const struct scmi_shmem_config *cfg;

	data = shmem->data;
	cfg = shmem->config;
	layout = (struct scmi_shmem_layout *)data->regmap;

	/* some sanity checks first */
	if (!msg) {
		return -EINVAL;
	}

	if (!msg->content && msg->len) {
		return -EINVAL;
	}

	if (cfg->size < (sizeof(*layout) + msg->len)) {
		return -EINVAL;
	}

	if (!(layout->chan_status & SCMI_SHMEM_CHAN_STATUS_BUSY_BIT)) {
		return -EBUSY;
	}

	layout->len = sizeof(layout->msg_hdr) + msg->len;
	layout->msg_hdr = msg->hdr;

	if (msg->content) {
		scmi_shmem_memcpy(data->regmap + sizeof(*layout),
				  POINTER_TO_UINT(msg->content), msg->len);
	}

	/* done, mark channel as busy and proceed */
	layout->chan_status &= ~SCMI_SHMEM_CHAN_STATUS_BUSY_BIT;

	return 0;
}

uint32_t scmi_shmem_channel_status(const struct device *shmem)
{
	struct scmi_shmem_layout *layout;
	struct scmi_shmem_data *data;

	data = shmem->data;
	layout = (struct scmi_shmem_layout *)data->regmap;

	return layout->chan_status;
}

void scmi_shmem_update_flags(const struct device *shmem, uint32_t mask, uint32_t val)
{
	struct scmi_shmem_layout *layout;
	struct scmi_shmem_data *data;

	data = shmem->data;
	layout = (struct scmi_shmem_layout *)data->regmap;

	layout->chan_flags = (layout->chan_flags & ~mask) | (val & mask);
}

static int scmi_shmem_init(const struct device *dev)
{
	const struct scmi_shmem_config *cfg;
	struct scmi_shmem_data *data;

	cfg = dev->config;
	data = dev->data;

	if (cfg->size < sizeof(struct scmi_shmem_layout)) {
		return -EINVAL;
	}

	device_map(&data->regmap, cfg->phys_addr, cfg->size, K_MEM_CACHE_NONE);

	return 0;
}

#define SCMI_SHMEM_INIT(inst)					\
static const struct scmi_shmem_config config_##inst = {		\
	.phys_addr = DT_INST_REG_ADDR(inst),			\
	.size = DT_INST_REG_SIZE(inst),				\
};								\
								\
static struct scmi_shmem_data data_##inst;			\
								\
DEVICE_DT_INST_DEFINE(inst, &scmi_shmem_init, NULL,		\
		      &data_##inst, &config_##inst,		\
		      PRE_KERNEL_1,				\
		      CONFIG_ARM_SCMI_SHMEM_INIT_PRIORITY,	\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(SCMI_SHMEM_INIT);
