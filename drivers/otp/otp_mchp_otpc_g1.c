/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#define DT_DRV_COMPAT microchip_otpc_g1_otp

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mchp_otpc_g1, CONFIG_OTP_LOG_LEVEL);

#define OTPC_TIMEOUT_MS	10

#define HEADER(header, field) \
	FIELD_GET(OTPC_HR_##field##_Msk, (header))
#define HEADER_SIZE(header) \
	(HEADER(header, SIZE) + 1)

enum packet_type {
	PACKET_TYPE_REGULAR = 1,
	PACKET_TYPE_KEY = 2,
	PACKET_TYPE_BOOT_CONFIG = 3,
	PACKET_TYPE_SECURE_BOOT_CONFIG = 4,
	PACKET_TYPE_HARDWARE_CONFIG = 5,
	PACKET_TYPE_CUSTOM = 6,
	PACKET_TYPE_MAX = PACKET_TYPE_CUSTOM,
};

struct otpc_packet {
	uint32_t header; /* Packet header */
	uint32_t addr;   /* Packet address, in 32-bit-word units */
	uint32_t tag;    /* Payload tag of regular packet */

	sys_snode_t node;
};

struct mchp_otpc_config {
	otpc_registers_t *regs;
	uint32_t otp_size;
	uint32_t emul_sram_size;
	void (*irq_config_func)(void);
};

struct mchp_otpc_data {
	uint32_t memory_size;
	sys_slist_t packet_list;
	struct k_mutex lock;
	struct k_sem completion;
};

static void otpc_isr(const struct device *dev)
{
	const struct mchp_otpc_config *cfg = dev->config;
	struct mchp_otpc_data *data = dev->data;
	otpc_registers_t *otpc = cfg->regs;
	uint32_t isr;

	isr = otpc->OTPC_ISR;
	LOG_DBG("OTPC: isr:0x%08x imr:0x%08x", isr, otpc->OTPC_IMR);
	isr &= otpc->OTPC_IMR;

	if ((isr & OTPC_ISR_EOR_Msk) != 0U) {
		k_sem_give(&data->completion);
	}
}

static int otpc_prepare_read(const struct device *dev, uint32_t addr)
{
	const struct mchp_otpc_config *cfg = dev->config;
	struct mchp_otpc_data *data = dev->data;
	otpc_registers_t *otpc = cfg->regs;
	int ret;

	(void)otpc->OTPC_SR;
	otpc->OTPC_MR &= ~OTPC_MR_ADDR_Msk;
	otpc->OTPC_MR |= OTPC_MR_ADDR(addr);
	otpc->OTPC_IER |= OTPC_IER_EOR_Msk;
	otpc->OTPC_CR |= OTPC_CR_READ_Msk;

	ret = k_sem_take(&data->completion, K_MSEC(OTPC_TIMEOUT_MS));

	otpc->OTPC_IDR |= OTPC_IDR_EOR_Msk;

	if (ret != 0) {
		LOG_ERR("OTPC: Failed to fetch packet from OTP!");
		return -EIO;
	}

	return (otpc->OTPC_SR & OTPC_SR_ONEF_Msk) != 0U;
}

static uint32_t otpc_read_packet_header(const struct device *dev)
{
	const struct mchp_otpc_config *cfg = dev->config;
	otpc_registers_t *otpc = cfg->regs;

	return otpc->OTPC_HR;
}

static int otpc_read_packet_data(const struct device *dev,
				 uint32_t addr, uint32_t *buf, uint32_t size)
{
	const struct mchp_otpc_config *cfg = dev->config;
	otpc_registers_t *otpc = cfg->regs;

	otpc->OTPC_AR = OTPC_AR_INCRT_AFTER_READ |
			OTPC_AR_DADDR(0);

	while (size--) {
		*buf++ = otpc->OTPC_DR;
	}

	return 0;
}

static int otpc_read_packet(const struct device *dev,
			    struct otpc_packet *packet,
			    uint32_t *buf, uint32_t size)
{
	if ((otpc_prepare_read(dev, packet->addr) != 1) ||
	    (otpc_read_packet_header(dev) != packet->header)) {
		return -EIO;
	}

	return otpc_read_packet_data(dev, packet->addr, buf, size);
}

static int otpc_init_packets_list(const struct device *dev)
{
	struct mchp_otpc_data *data = dev->data;
	struct otpc_packet *packet;
	uint32_t word_size = data->memory_size / sizeof(uint32_t);
	uint32_t offset = 0, count = 0;
	uint32_t header;
	int ret;

	while (offset < word_size) {
		ret = otpc_prepare_read(dev, offset);
		if (ret < 0) { /* Prepare read packet error */
			return ret;
		}
		if (ret == 0) { /* No valid packet found */
			break;
		}

		header = otpc_read_packet_header(dev);
		if ((HEADER(header, PACKET) == 0U) ||
		    (HEADER(header, PACKET) > PACKET_TYPE_MAX) ||
		    (HEADER(header, ONE) != 1U)) {
			LOG_ERR("OTPC: Invalid packet header: 0x%08x!", header);
			break;
		}

		packet = k_calloc(1, sizeof(*packet));
		if (packet == NULL) {
			LOG_ERR("OTPC: Failed to alloc packet!");
			return -ENOMEM;
		}

		packet->header = header;
		packet->addr = offset;
		sys_slist_append(&data->packet_list, &packet->node);

		if (HEADER(packet->header, PACKET) == PACKET_TYPE_REGULAR) {
			ret = otpc_read_packet_data(dev, packet->addr, &packet->tag, 1);
			if (ret != 0) {
				return ret;
			}
		}

		offset += HEADER_SIZE(packet->header) + 1;
		count++;
	}

	return count;
}

static int otpc_init(const struct device *dev)
{
	const struct mchp_otpc_config *cfg = dev->config;
	struct mchp_otpc_data *data = dev->data;
	otpc_registers_t *otpc = cfg->regs;
	uint32_t isr = otpc->OTPC_ISR;

	if ((isr & OTPC_ISR_COERR_Msk) != 0U) {
		LOG_ERR("OTPC: Init failed: corrupted header (ISR=0x%08x)!", isr);
		return -EIO;
	}

	if ((otpc->OTPC_MR & OTPC_MR_EMUL_Msk) != 0U) {
		LOG_INF("OTPC: Emulation mode enabled");
		data->memory_size = cfg->emul_sram_size;
	} else {
		data->memory_size = cfg->otp_size;
	}

	return otpc_init_packets_list(dev);
}

static int mchp_otpc_read(const struct device *dev,
			  off_t offset, void *buf, size_t len)
{
	struct mchp_otpc_data *data = dev->data;
	struct otpc_packet *packet = NULL;
	sys_snode_t *node;
	int ret;

	if (buf == NULL) {
		LOG_ERR("OTPC: %s invalid argument, buf = NULL!", __func__);
		return -EINVAL;
	}

	if ((!IS_ALIGNED((uintptr_t)buf, sizeof(uint32_t))) ||
	    (!IS_ALIGNED(len, sizeof(uint32_t)))) {
		LOG_ERR("OTPC: %s buf or len is not word aligned!", __func__);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE(&data->packet_list, node) {
		struct otpc_packet *pkt =
			CONTAINER_OF(node, struct otpc_packet, node);

		if (pkt->tag == offset) {
			packet = pkt;
			break;
		}
	}

	if (packet == NULL) {
		LOG_ERR("OTPC: Packet tag 0x%08x not found!", (uint32_t)offset);
		ret = -EINVAL;
	} else {
		uint32_t size = len / sizeof(uint32_t);

		if (size > HEADER_SIZE(packet->header)) {
			LOG_ERR("OTPC: Request word size is out of range %d:%d!",
				size, HEADER_SIZE(packet->header));
			ret = -EINVAL;
		} else {
			ret = otpc_read_packet(dev, packet, buf, size);
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(otp, mchp_otpc_api) = {
	.read = mchp_otpc_read,
};

static int mchp_otpc_init(const struct device *dev)
{
	const struct mchp_otpc_config *cfg = dev->config;
	struct mchp_otpc_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->completion, 0, 1);
	sys_slist_init(&data->packet_list);

	cfg->irq_config_func();

	ret = otpc_init(dev);
	if (ret >= 0) {
		sys_snode_t *node;

		LOG_DBG("OTPC: %d packets found", ret);
		SYS_SLIST_FOR_EACH_NODE(&data->packet_list, node) {
			struct otpc_packet *packet =
				CONTAINER_OF(node, struct otpc_packet, node);
			LOG_DBG("OTPC: header:0x%08x type:%d addr:%d size:%d tag:0x%08x",
				packet->header, HEADER(packet->header, PACKET),
				packet->addr, HEADER_SIZE(packet->header),
				packet->tag);
		}

		return 0;
	}

	return ret;
}

#define OTP_MCHP_OTPC_DEVICE_DEFINE(n)						\
	static void mchp_otpc_irq_config_func_##n(void);			\
										\
	static struct mchp_otpc_data mchp_otpc_data_##n = {			\
	};									\
										\
	static struct mchp_otpc_config mchp_otpc_config_##n = {			\
		.regs = (otpc_registers_t *)DT_INST_REG_ADDR(n),		\
		.otp_size = DT_INST_PROP(n, otp_memory_size),			\
		.emul_sram_size = DT_INST_PROP(n, emul_sram_size),		\
		.irq_config_func = mchp_otpc_irq_config_func_##n,		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, mchp_otpc_init, NULL,				\
			      &mchp_otpc_data_##n, &mchp_otpc_config_##n,	\
			      POST_KERNEL, CONFIG_OTP_INIT_PRIORITY,		\
			      &mchp_otpc_api);					\
										\
	static void mchp_otpc_irq_config_func_##n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    otpc_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(OTP_MCHP_OTPC_DEVICE_DEFINE)
