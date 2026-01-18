/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mbox_xlnx_ipi_mailbox, CONFIG_MBOX_LOG_LEVEL);

/* Register offsets of IPI */
#define IPI_REG_TRIG_OFFSET	0x00U /* Offset for Trigger Register */
#define IPI_REG_OBS_OFFSET	0x04U /* Offset for Observation Register */
#define IPI_REG_ISR_OFFSET	0x10U /* Offset for ISR Register */
#define IPI_REG_IMR_OFFSET	0x14U /* Offset for Interrupt Mask Register */
#define IPI_REG_IER_OFFSET	0x18U /* Offset for Interrupt Enable Register */
#define IPI_REG_IDR_OFFSET	0x1CU /* Offset for Interrupt Disable Register */

/* Mask of all valid IPI bits in above registers */
#define IPI_ALL_MASK	GENMASK(31, 0)

/* IPI mailbox channels */
#define IPI_MB_MAX_CHNLS	2U /* One Tx, One Rx */
#define IPI_MB_CHNL_TX		0U /* IPI mailbox TX channel */
#define IPI_MB_CHNL_RX		1U /* IPI mailbox RX channel */

/* IPI Message Buffer Information */
#define IPI_MAX_MSG_BYTES	(32U)
#define IPI_MAX_MSG_WORDS	(8U)
#define IPI_MEM_STRIDE		(0x200U)
#define IPI_REQ_OFF		(0x00U)
#define IPI_RESP_OFF		(0x20U)
#define IPI_BUF_STRIDE		(0x40U)

/**
 * @brief Configuration for the Xilinx IPI host mailbox
 */
struct mbox_xlnx_ipi_parent_config {
	mem_addr_t reg_base;			/**< Host Control register base address */
	uint8_t *msg_base;			/**< Pointer to Host message buffer */
	uint32_t ipi_id;			/**< Host IPI ID */
	uint32_t ipi_bitmask;			/**< Host IPI Bitmask */
	void (*irq_config_func)(void);		/**< IRQ configuration API pointer */
	const struct device **cdev_list;	/**< Array of pointers to child devices */
	int num_cdev;				/**< Number of child devices */
};

/**
 * @brief Configuration for the Xilinx IPI destination mailbox
 */
struct mbox_xlnx_ipi_child_config {
	mem_addr_t reg_base;		/**< Remote Control register base address */
	uint8_t *msg_base;		/**< Pointer to the Remote message buffer */
	uint32_t remote_ipi_id;		/**< Remote IPI ID */
	uint32_t remote_ipi_bitmask;	/**< Remote IPI Bitmask */
	mem_addr_t parent_ipi_reg;	/**< Host Control register base address */
	uint8_t *parent_ipi_msg;	/**< Pointer to Host message buffer */
};

/**
 * @brief Runtime data for the Xilinx IPI destination mailbox
 */
struct mbox_xlnx_ipi_child_data {
	bool enabled;			/**< Channel enable status */
	mbox_callback_t mb_callback;	/**< Callback function */
	void *user_data;		/**< Application specific data pointer */
};

/**
 * @brief Interrupt Service Routine (ISR) for the Xilinx IPI mailbox
 *
 * Handles interrupts, processes messages and invokes registered callbacks
 */
static void mbox_xlnx_ipi_isr(const struct device *pdev)
{
	const struct mbox_xlnx_ipi_parent_config *pcfg = pdev->config;
	const struct mbox_xlnx_ipi_child_config *cdev_conf;
	const struct mbox_xlnx_ipi_child_data *cdev_data;
	uint32_t ipi_msg_buf[IPI_MAX_MSG_WORDS];
	const struct device **cdev_list;
	struct mbox_msg *msgptr = NULL;
	const struct device *cdev;
	uint32_t ipi_src_mask;
	struct mbox_msg msg;
	uint8_t *buf_ptr;
	mem_addr_t off;
	int num_cdev;
	int cdev_idx;
	int buf_idx;

	cdev_list = pcfg->cdev_list;
	num_cdev = pcfg->num_cdev;

	/* Read interrupt status */
	ipi_src_mask = sys_read32(pcfg->reg_base + IPI_REG_ISR_OFFSET);

	for (cdev_idx = 0; cdev_idx < num_cdev; cdev_idx++) {
		cdev = cdev_list[cdev_idx];
		cdev_conf = cdev->config;
		cdev_data = cdev->data;

		if (cdev_data->enabled == false) {
			continue; /* channel is disabled */
		}

		if ((ipi_src_mask & (cdev_conf->remote_ipi_bitmask)) == 0U) {
			continue; /* interrupt is not for this channel */
		}

		if (cdev_data->mb_callback == NULL) {
			continue; /* callback is not registered */
		}

		/* Read the message if buffered IPI */
		if ((pcfg->msg_base != NULL) && (cdev_conf->msg_base != NULL)) {
			off = (mem_addr_t)pcfg->msg_base + IPI_REQ_OFF;
			off += (cdev_conf->remote_ipi_id) * IPI_BUF_STRIDE;
			buf_ptr = (uint8_t *)ipi_msg_buf;
			for (buf_idx = 0; buf_idx < IPI_MAX_MSG_BYTES; buf_idx += 4) {
				*(uint32_t *)(buf_ptr + buf_idx) = sys_read32(off + buf_idx);
			}
			msg.size = IPI_MAX_MSG_BYTES;
			msg.data = (void *)buf_ptr;
			msgptr = &msg;
		}
		/* Call registered callback */
		cdev_data->mb_callback(cdev, IPI_MB_CHNL_RX, cdev_data->user_data, msgptr);
	}

	/* Clear the interrupt status */
	sys_write32(ipi_src_mask, pcfg->reg_base + IPI_REG_ISR_OFFSET);
}

/**
 * @brief Send a message/signal over the MBOX device.
 */
static int mbox_xlnx_ipi_send(const struct device *cdev, uint32_t channel,
			      const struct mbox_msg *msg)
{
	uint32_t __aligned(4) data32;
	const struct mbox_xlnx_ipi_child_config *cfg = cdev->config;
	uint32_t *data;
	mem_addr_t off;
	int obs_bit;
	size_t len;

	/* Validate outbound channel */
	if (channel != IPI_MB_CHNL_TX) {
		LOG_ERR("Invalid MBOX Tx channel number: %u", channel);
		return -EINVAL;
	}

	/* Check if Remote has read the previous message */
	obs_bit = sys_test_bit(cfg->parent_ipi_reg + IPI_REG_OBS_OFFSET, cfg->remote_ipi_id);
	if (obs_bit != 0) {
		LOG_DBG("Remote IPI-ID:%u is busy", cfg->remote_ipi_id);
		return -EBUSY;
	}

	/* Signalling mode: trigger interrupt without message */
	if ((msg == NULL) || (cfg->parent_ipi_msg == NULL) || (cfg->msg_base == NULL)) {
		sys_set_bit(cfg->parent_ipi_reg + IPI_REG_TRIG_OFFSET, cfg->remote_ipi_id);
		return 0;
	}

	/* Data transfer mode: write message and then trigger interrupt */
	if (msg->size > IPI_MAX_MSG_BYTES) {
		/* we can only send max this many bytes at a time */
		LOG_ERR("size: %u is invalid, Max size is %u bytes", msg->size, IPI_MAX_MSG_BYTES);
		return -EMSGSIZE;
	}

	data = (uint32_t *)msg->data;
	len = msg->size;
	off = (mem_addr_t)(cfg->parent_ipi_msg) + IPI_REQ_OFF;
	off += (cfg->remote_ipi_id * IPI_BUF_STRIDE);

	/* send msg data in 4-byte chunks */
	while (len >= 4U) {
		/* memcpy to avoid issues when msg->data is not word-aligned */
		(void)memcpy(&data32, data, sizeof(data32));
		sys_write32(data32, off);

		data++;
		len -= 4U;
		off += 4U;
	}

	/* Handle any leftover bytes */
	if (len > 0U) {
		data32 = 0U;
		(void)memcpy(&data32, data, len);
		sys_write32(data32, off);
	}

	/* Trigger IPI to the target */
	sys_set_bit(cfg->parent_ipi_reg + IPI_REG_TRIG_OFFSET, cfg->remote_ipi_id);

	return 0;
}

/**
 * @brief Register a callback function on a channel for incoming messages.
 */
static int mbox_xlnx_ipi_register_callback(const struct device *cdev, uint32_t channel,
					   mbox_callback_t cb, void *user_data)
{
	struct mbox_xlnx_ipi_child_data *cdev_data = cdev->data;

	/* Validate inbound channel */
	if (channel != IPI_MB_CHNL_RX) {
		LOG_ERR("Invalid MBOX Rx channel number: %u", channel);
		return -EINVAL;
	}

	/* Set the callback and user data */
	cdev_data->mb_callback = cb;
	cdev_data->user_data = user_data;

	return 0;
}

/**
 * @brief Return the maximum number of bytes possible in an outbound message.
 */
static int mbox_xlnx_ipi_mtu_get(const struct device *cdev)
{
	const struct mbox_xlnx_ipi_child_config *cfg = cdev->config;

	/* Signalling mode: buffer-less IPI */
	if ((cfg->parent_ipi_msg == NULL) || (cfg->msg_base == NULL)) {
		return 0;
	}

	return IPI_MAX_MSG_BYTES;
}

/**
 * @brief Return the maximum number of channels supported by MBOX device instance.
 */
static uint32_t mbox_xlnx_ipi_max_channels_get(const struct device *cdev)
{
	ARG_UNUSED(cdev);

	return IPI_MB_MAX_CHNLS;
}

/**
 * @brief Enable (disable) interrupts and callbacks for inbound channels.
 */
static int mbox_xlnx_ipi_set_enabled(const struct device *cdev, uint32_t channel, bool enable)
{
	struct mbox_xlnx_ipi_child_data *cdev_data = cdev->data;
	const struct mbox_xlnx_ipi_child_config *cfg = cdev->config;

	/* Validate inbound channel */
	if (channel != IPI_MB_CHNL_RX) {
		LOG_ERR("Invalid MBOX Rx channel number: %u", channel);
		return -EINVAL;
	}

	/* check if already */
	if (cdev_data->enabled == enable) {
		return -EALREADY;
	}

	if (enable) {
		if (cdev_data->mb_callback == NULL) {
			LOG_WRN("Enabling channel:%u, without a registered callback", channel);
		}

		/* Enable the interrupt for the specified channel */
		sys_set_bit(cfg->parent_ipi_reg + IPI_REG_IER_OFFSET, cfg->remote_ipi_id);
	} else {
		/* Disable the interrupt for the specified channel */
		sys_set_bit(cfg->parent_ipi_reg + IPI_REG_IDR_OFFSET, cfg->remote_ipi_id);
	}

	cdev_data->enabled = enable;

	return 0;
}

/**
 * @brief Initialize the IPI mailbox module.
 */
static int mbox_xlnx_ipi_init(const struct device *pdev)
{
	const struct mbox_xlnx_ipi_parent_config *pcfg = pdev->config;

	/* Disable all the interrupts */
	sys_write32(IPI_ALL_MASK, pcfg->reg_base + IPI_REG_IDR_OFFSET);

	/* Clear status of any previous interrupts */
	sys_write32(IPI_ALL_MASK, pcfg->reg_base + IPI_REG_ISR_OFFSET);

	/* Configure IRQ */
	pcfg->irq_config_func();

	return 0;
}

static DEVICE_API(mbox, mbox_xlnx_ipi_driver_api) = {
	.send = mbox_xlnx_ipi_send,
	.register_callback = mbox_xlnx_ipi_register_callback,
	.mtu_get = mbox_xlnx_ipi_mtu_get,
	.max_channels_get = mbox_xlnx_ipi_max_channels_get,
	.set_enabled = mbox_xlnx_ipi_set_enabled,
};

/*
 * ************************* DRIVER REGISTER SECTION ***************************
 */

/* Child node is used for MBOX driver */
#define MBOX_XLNX_VERSAL_IPI_CHILD(ch_node)\
	struct mbox_xlnx_ipi_child_data mbox_xlnx_ipi_child_data##ch_node = {\
		.enabled = false,\
		.mb_callback = NULL,\
		.user_data = NULL,\
	};\
	struct mbox_xlnx_ipi_child_config mbox_xlnx_ipi_child_config##ch_node = {\
		.reg_base = DT_REG_ADDR_BY_NAME(ch_node, ctrl),\
		.msg_base = (uint8_t *)DT_REG_ADDR_BY_NAME_OR(ch_node, msg, NULL),\
		.remote_ipi_id = DT_PROP(ch_node, xlnx_ipi_id),\
		.remote_ipi_bitmask = BIT(DT_PROP(ch_node, xlnx_ipi_id)),\
		.parent_ipi_reg = DT_REG_ADDR_BY_NAME(DT_PARENT(ch_node), ctrl),\
		.parent_ipi_msg = (uint8_t *)DT_REG_ADDR_BY_NAME_OR(DT_PARENT(ch_node), msg, NULL),\
	};\
	DEVICE_DT_DEFINE(ch_node, NULL, NULL, &mbox_xlnx_ipi_child_data##ch_node,\
			&mbox_xlnx_ipi_child_config##ch_node, POST_KERNEL,\
			CONFIG_MBOX_INIT_PRIORITY, &mbox_xlnx_ipi_driver_api);

/* Parent node for ISR and initialization */
#define MBOX_XLNX_VERSAL_IPI_INSTANCE_DEFINE(idx)\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, MBOX_XLNX_VERSAL_IPI_CHILD);\
	static const struct device *cdev##idx[] = {\
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(idx, DEVICE_DT_GET, (,))\
	};\
	static void mbox_xlnx_ipi_##idx##_irq_config_func(void);\
	const static struct mbox_xlnx_ipi_parent_config mbox_xlnx_ipi_##idx##_pconfig = {\
		.reg_base = DT_REG_ADDR_BY_NAME(DT_DRV_INST(idx), ctrl),\
		.msg_base = (uint8_t *)DT_REG_ADDR_BY_NAME_OR(DT_DRV_INST(idx), msg, NULL),\
		.ipi_id = DT_INST_PROP(idx, xlnx_ipi_id),\
		.ipi_bitmask = BIT(DT_INST_PROP(idx, xlnx_ipi_id)),\
		.irq_config_func = mbox_xlnx_ipi_##idx##_irq_config_func,\
		.cdev_list = cdev##idx,\
		.num_cdev = ARRAY_SIZE(cdev##idx),\
		};\
	static void mbox_xlnx_ipi_##idx##_irq_config_func(void)\
	{\
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority),\
			mbox_xlnx_ipi_isr, DEVICE_DT_INST_GET(idx), 0);\
		irq_enable(DT_INST_IRQN(idx));\
	} \
	DEVICE_DT_INST_DEFINE(idx, mbox_xlnx_ipi_init, NULL, NULL,\
				&mbox_xlnx_ipi_##idx##_pconfig, POST_KERNEL,\
				CONFIG_MBOX_INIT_PRIORITY, NULL);

#define DT_DRV_COMPAT xlnx_mbox_versal_ipi_mailbox
DT_INST_FOREACH_STATUS_OKAY(MBOX_XLNX_VERSAL_IPI_INSTANCE_DEFINE)
