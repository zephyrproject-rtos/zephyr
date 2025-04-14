/*
 * Copyright (c) 2024 Microchip Technology Inc
 *
 * SPDX-License-Identifier: (GPL-2.0 OR MIT)
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "ipm_mchp_ihc.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mchp_ihc_imp, CONFIG_IPM_LOG_LEVEL);

#define MCHP_IHC_RPROC_STOP 	(0xFFFFFF02)

static int mchp_ihc_max_data_size_get(const struct device *dev);
/**
 * @brief Get channel remote hart id
 *
 * @param dev 		IHCC (channel) device instance
 * @return uint32_t 	remote hart id
 */
static uint32_t mchp_ihc_get_channel_remote(const struct device *dev)
{
	struct mchp_ihcc_config *config = (struct mchp_ihcc_config *)dev->config;
	struct mchp_ihcc_reg_map_t * ihcc_regs = (struct mchp_ihcc_reg_map_t *)config->ihcc_regs;
	uint32_t channel_hart_id = 0;

	channel_hart_id = (ihcc_regs->debug_id & MCHP_IHC_REGS_REMOTE_HART_ID_MASK) >>
			  MCHP_IHC_REGS_REMOTE_HART_ID_OFFSET;

	return(channel_hart_id);
}

#if defined(CONFIG_IPM_MCHP_IHC_REMOTEPROC)
/**
 * @brief IRQ routine in case of remoteproc stop message received
 *
 * @param dev 		IHCM device
 */
static void mchp_ihcm_rproc_stop(const struct device *dev)
{
	struct mchp_ihcm_config *ihcm_config = (struct mchp_ihcm_config *)dev->config;
	/**
	 * We enable the interrupt just to clear the pending bit in case it have been disable in
	 * callback
	 */
	irq_enable(ihcm_config->irq_idx);
	riscv_plic_irq_complete(ihcm_config->irq_idx);
	irq_disable(ihcm_config->irq_idx);

	/* use that trick to call the function in the scratchpad with -mo-pie activated */
	__asm__ volatile ("jalr ra, 0(%0)\n\t"
			  :: "r" (CONFIG_IPM_MCHP_IHC_REMOTEPROC_STOP_ADDR)
			  :"ra");
}
#endif

/**
 * @brief IHCM irq handler, treat and dispatch all interrupt for that module
 *
 * @param dev 		IHCM device
 */
static void mchp_ihcm_irq_handler(const struct device *dev)
{
	struct mchp_ihcm_data *ihcm_data = (struct mchp_ihcm_data *)dev->data;
	struct mchp_ihcm_config *ihcm_config = (struct mchp_ihcm_config *)dev->config;
	struct mchp_ihcm_reg_map_t * ihcm_regs =
		(struct mchp_ihcm_reg_map_t *)ihcm_config->ihcm_regs;
	struct mchp_ihcc_config *ihcc_config = NULL;
	struct mchp_ihcc_data *ihcc_data = NULL;
	struct mchp_ihcc_reg_map_t * ihcc_regs = NULL;
	uint32_t i = 0;
#if defined(CONFIG_IPM_MCHP_IHC_REMOTEPROC)
	bool rproc_stop = false;
#endif
	volatile uint32_t irq_status = 0;

	irq_status = ihcm_regs->irq_status;
	/* check irq flag */
	for (i = 0; i < ihcm_data->num_cb; i++) {
		if (irq_status & MCHP_IHC_REGS_IRQ_STATUS_MP(ihcm_data->cb_idx_list[i])) {
			LOG_DBG("mchp_ihcm_irq_handler: MP interrupt received");
			/* message received */
			ihcc_config = (struct mchp_ihcc_config *)ihcm_config->ihcc_list[i]->config;
			ihcc_regs = (struct mchp_ihcc_reg_map_t * )ihcc_config->ihcc_regs;
#if defined(CONFIG_IPM_MCHP_IHC_REMOTEPROC)
			if (ihcc_regs->msg_in[0] == MCHP_IHC_RPROC_STOP) {
				rproc_stop = true;
			}
#endif
			if(ihcm_data->cb_list[i] != NULL) {
				ihcm_data->cb_list[i](ihcm_config->ihcc_list[i],
						      ihcm_data->cb_user_data_list[i],
						      0,
						      ihcc_regs->msg_in);
			}

			/* clear Message Present flag and clear ack */
			sys_clear_bits((uintptr_t)ihcc_regs +
				       MCHP_IHCC_REGS_CTRL_OFFSET,
				       MCHP_IHC_REGS_CH_CTRL_MP_MASK |
				       MCHP_IHC_REGS_CH_CTRL_ACK_MASK);
			/* set ACK flag for remote */
			sys_set_bits((uintptr_t)ihcc_regs + MCHP_IHCC_REGS_CTRL_OFFSET,
				     MCHP_IHC_REGS_CH_CTRL_ACK_MASK);
#if defined(CONFIG_IPM_MCHP_IHC_REMOTEPROC)
			if(rproc_stop) {
				mchp_ihcm_rproc_stop(dev);
			}
#endif
		}
		if (irq_status & MCHP_IHC_REGS_IRQ_STATUS_ACK(ihcm_data->cb_idx_list[i])) {
			ihcc_data = (struct mchp_ihcc_data *)ihcm_config->ihcc_list[i]->data;
			ihcc_config = (struct mchp_ihcc_config *)ihcm_config->ihcc_list[i]->config;
			ihcc_regs = (struct mchp_ihcc_reg_map_t * )ihcc_config->ihcc_regs;
			/* message recive ack from remote */
			LOG_DBG("mchp_ihcm_irq_handler: ACK interrupt received");
			atomic_set(&ihcc_data->ack, true);
			sys_clear_bits((uintptr_t)ihcc_regs + MCHP_IHCC_REGS_CTRL_OFFSET,
				       MCHP_IHC_REGS_CH_CTRL_ACKCLR_MASK);
		}
	}
}

static int mchp_ihc_send(const struct device *dev, int wait, uint32_t id,
			 const void *data, int size)
{
	struct mchp_ihcc_config *ihcc_config = (struct mchp_ihcc_config *)dev->config;
	struct mchp_ihcc_data *ihcc_data = (struct mchp_ihcc_data *)dev->data;
	uintptr_t ihcc_regs = (uintptr_t)ihcc_config->ihcc_regs;
	uint32_t i;
	uint32_t reg_val;
	int ret = 0;

	k_mutex_lock(ihcc_data->channel_lock, K_FOREVER);

	reg_val = sys_read32(ihcc_regs + MCHP_IHCC_REGS_CTRL_OFFSET);
	if (reg_val & (MCHP_IHC_REGS_CH_CTRL_RMP_MASK | MCHP_IHC_REGS_CH_CTRL_ACK_MASK)) {
		ret = -EBUSY;
		goto cleanup;
	}

	if (size > mchp_ihc_max_data_size_get(dev)) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (size > 0) {
		/* if 32 bit aligned words */
		if (((uintptr_t)data % sizeof(uint32_t) == 0) && (size % sizeof(uint32_t) == 0)) {
			for (i = 0; i < size/4; i++) {
				sys_write32(((uint32_t *)data)[i],
					    ihcc_regs +
					    MCHP_IHCC_REGS_MSG_OUT_OFFSET +
					    i*sizeof(uint32_t));
			}
		} else {
			uint32_t data_words[size/sizeof(uint32_t)];
			memset(data_words, 0, size);
			memcpy(data_words, data, size);
			/* if not 32 bit aligned, copy byte by byte */
			for (i = 0; i < size; i++) {
				sys_write32(data_words[i],
					    ihcc_regs +
					    MCHP_IHCC_REGS_MSG_OUT_OFFSET +
					    i*sizeof(uint32_t));
			}
		}

		/* set MP flag for remote hart */
		sys_set_bits(ihcc_regs + MCHP_IHCC_REGS_CTRL_OFFSET,
			     MCHP_IHC_REGS_CH_CTRL_RMP_MASK);
		LOG_DBG("mchp_ihc_send message sent");
		/**
		 * Wait for ack, this is intentionally a busy waiting loop as
		 * requested by IPM API
		 */
		while (wait) {
			if (atomic_get(&ihcc_data->ack) == true) {
				atomic_set(&ihcc_data->ack, false);
				break;
			} else {
				/* delay in usec */
				k_busy_wait(200);
			}
		}
	} else {
		ret = -EINVAL;
		goto cleanup;
	}
	ret = 0;
cleanup:
	k_mutex_unlock(ihcc_data->channel_lock);
	return ret;
}

static void mchp_ihc_register_callback(const struct device *dev,
				       ipm_callback_t cb,
				       void *user_data)
{
	struct mchp_ihcc_config *config = (struct mchp_ihcc_config *)dev->config;
	struct mchp_ihcm_data *ihcm_data = (struct mchp_ihcm_data *)config->parent_node->data;
	uint32_t channel_hart_id = 0;
	uint32_t i = 0;
	int key;

	channel_hart_id = mchp_ihc_get_channel_remote(dev);

	key = irq_lock();

	for (i = 0; i < ihcm_data->num_cb; i++) {
		if (ihcm_data->cb_idx_list[i] == channel_hart_id) {
			ihcm_data->cb_list[i] = cb;
			ihcm_data->cb_user_data_list[i] = user_data;
		}
	}

	irq_unlock(key);
}

static int mchp_ihc_max_data_size_get(const struct device *dev)
{
	struct mchp_ihcc_config *ihcc_config = (struct mchp_ihcc_config *)dev->config;
	uint32_t reg_val;

	reg_val = sys_read32(ihcc_config->ihcc_regs + MCHP_IHCC_REGS_MSG_DEPTH_OFFSET);
	reg_val &= MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_OUT_MASK;
	return((int)reg_val);
}

static uint32_t mchp_ihc_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return UINT32_MAX;
}

static int mchp_ihcm_set_enabled(const struct device *dev, int enable, uint32_t remote_hart_id)
{
	struct mchp_ihcm_data *ihcm_data = (struct mchp_ihcm_data *)dev->data;
	struct mchp_ihcm_config *ihcm_config = (struct mchp_ihcm_config *)dev->config;
	uintptr_t ihcm_regs = ihcm_config->ihcm_regs;

	k_mutex_lock(ihcm_data->module_lock, K_FOREVER);
	if (enable) {
		if (ihcm_data->isr_counter > 0) {
			ihcm_data->isr_counter++;
		} else {
			irq_enable(ihcm_config->irq_idx);
			LOG_DBG("enable irq : %d\r\n", ihcm_config->irq_idx);
			ihcm_data->isr_counter++;
		}
		sys_set_bits(ihcm_regs + MCHP_IHCM_REGS_IRQ_MASK_OFFSET,
			     MCHP_IHC_REGS_IRQ_MASK_MP(remote_hart_id) |
			     MCHP_IHC_REGS_IRQ_MASK_ACK(remote_hart_id));
	} else {
		sys_clear_bits(ihcm_regs + MCHP_IHCM_REGS_IRQ_MASK_OFFSET,
			       MCHP_IHC_REGS_IRQ_MASK_MP(remote_hart_id) |
			       MCHP_IHC_REGS_IRQ_MASK_ACK(remote_hart_id));
		if (ihcm_data->isr_counter == 1) {
			irq_disable(ihcm_config->irq_idx);
			LOG_DBG("disable irq : %d\r\n", ihcm_config->irq_idx);
			ihcm_data->isr_counter--;
		} else if (ihcm_data->isr_counter > 1) {
			ihcm_data->isr_counter--;
		} else {
			LOG_ERR("Error: ISR counter is zero but still trying to disable the IRQ");
		}
	}
	k_mutex_unlock(ihcm_data->module_lock);
	return(0);
}

static int mchp_ihc_set_enabled(const struct device *dev, int enable)
{
	struct mchp_ihcc_data *data = (struct mchp_ihcc_data *)dev->data;
	struct mchp_ihcc_config *config = (struct mchp_ihcc_config *)dev->config;
	uintptr_t ihcc_regs = config->ihcc_regs;
	uint32_t channel_hart_id = 0;

	channel_hart_id = mchp_ihc_get_channel_remote(dev);

	k_mutex_lock(data->channel_lock, K_FOREVER);
	if (enable && (data->enabled == false)) {
		(void)mchp_ihcm_set_enabled(config->parent_node, enable, channel_hart_id);
		data->enabled = true;
		sys_set_bits(ihcc_regs + MCHP_IHCC_REGS_CTRL_OFFSET,
			     MCHP_IHC_REGS_CH_CTRL_MPIE_MASK |
			     MCHP_IHC_REGS_CH_CTRL_ACKIE_MASK);
	} else if (!enable && (data->enabled == true)) {
		data->enabled = false;
		sys_clear_bits(ihcc_regs + MCHP_IHCC_REGS_CTRL_OFFSET,
			       MCHP_IHC_REGS_CH_CTRL_MPIE_MASK |
			       MCHP_IHC_REGS_CH_CTRL_ACKIE_MASK);
		(void)mchp_ihcm_set_enabled(config->parent_node, enable, channel_hart_id);
	}
	k_mutex_unlock(data->channel_lock);

	return(0);
}

/**
 * @brief Initialize the IHC Channels
 *
 * @param dev IHCC device instance
 */
static void mchp_ihcc_init(const struct device *dev)
{
	struct mchp_ihcc_config *ihcc_cfg = (struct mchp_ihcc_config *)dev->config;
	uintptr_t ihcc_regs = ihcc_cfg->ihcc_regs;

	sys_write32(0, ihcc_regs + MCHP_IHCC_REGS_CTRL_OFFSET);
}

/**
 * @brief Initialize the IHC Modules
 *
 * @param dev IHCM device instance
 */
static void mchp_ihcm_init(const struct device *dev)
{
	uint32_t channel_remote_id = 0;
	uint32_t i;
	struct mchp_ihcm_config *ihcm_cfg = (struct mchp_ihcm_config *)dev->config;
	struct mchp_ihcm_data *ihcm_data = (struct mchp_ihcm_data *)dev->data;
	struct mchp_ihcm_reg_map_t * ihcm_regs = (struct mchp_ihcm_reg_map_t *)ihcm_cfg->ihcm_regs;

	ihcm_cfg->config_func();
	ihcm_regs->irq_mask = MCHP_IHC_REGS_IRQ_DISABLE_MASK;

	/* Make sure any unclaimed interrupts are cleared */
	irq_enable(ihcm_cfg->irq_idx);
	riscv_plic_irq_complete(ihcm_cfg->irq_idx);
	irq_disable(ihcm_cfg->irq_idx);

	for (i = 0; i < ihcm_cfg->num_ihcc; i++) {
		/*
		 * Associate each channel device to a remote hart in
		 * order to map irq to callback later.
		 * The callback itself is register in the appropriate
		 * function.
		*/
		channel_remote_id = mchp_ihc_get_channel_remote(ihcm_cfg->ihcc_list[i]);
		ihcm_data->cb_idx_list[i] = channel_remote_id;
		mchp_ihcc_init(ihcm_cfg->ihcc_list[i]);
	}
}


/**
 * @brief Initialize the IHC driver
 *
 * @note This function initializes the IHC driver and connects the IRQs for the
 * enabled IHCM child nodes
 * @param dev 	instance of microchip,miv-ihc-rtl-v2
 * @return int 	0 = success, <0 error
 */
static int mchp_ihc_init(const struct device *dev)
{
	struct mchp_ihc_config *ihc_cfg = (struct mchp_ihc_config *)dev->config;
	uint32_t i;

	/* connect IRQs for enabled IHCC nodes */
	for (i = 0; i < ihc_cfg->num_ihcm; i++) {
		mchp_ihcm_init(ihc_cfg->ihcm_list[i]);
	}
	return(0);
}

static const struct ipm_driver_api mchp_ihc_driver_api = {
	.send = mchp_ihc_send,
	.register_callback = mchp_ihc_register_callback,
	.max_data_size_get = mchp_ihc_max_data_size_get,
	.max_id_val_get = mchp_ihc_max_id_val_get,
	.set_enabled = mchp_ihc_set_enabled
};

#define DT_DRV_COMPAT microchip_miv_ihc_rtl_v2

#define MCHP_IHCC_INIT(node)                                                                       \
	K_MUTEX_DEFINE(mchp_ihcc_lock##node);                                                      \
	struct mchp_ihcc_data mchp_ihcc_data##node = {                                             \
		.enabled = false,                                                                  \
		.channel_lock = &mchp_ihcc_lock##node,                                             \
		.ack = ATOMIC_INIT(false),                                                         \
	};                                                                                         \
	struct mchp_ihcc_config mchp_ihcc_config##node = {                                         \
		.parent_node = DEVICE_DT_GET(DT_PARENT(node)),                                     \
		.gparent_node = DEVICE_DT_GET(DT_GPARENT(node)),                                   \
		.ihc_regs = DT_REG_ADDR(DT_GPARENT(node)),                                         \
		.ihcm_regs = DT_REG_ADDR(DT_PARENT(node)),                                         \
		.ihcc_regs = DT_REG_ADDR(node),                                                    \
	};                                                                                         \
	DEVICE_DT_DEFINE(node, NULL, NULL, &mchp_ihcc_data##node,                                  \
			 &mchp_ihcc_config##node, POST_KERNEL,                                     \
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &mchp_ihc_driver_api);

#define MCHP_IHCM_INIT(node)                                                                       \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(node, MCHP_IHCC_INIT, (;));                               \
	K_MUTEX_DEFINE(mchp_ihcm_lock##node);                                                      \
	static int ihcm_cfg_func_##node(void);                                                     \
	static const struct device *mchp_ihcc##node[] = {                                          \
		DT_FOREACH_CHILD_STATUS_OKAY_SEP(node, DEVICE_DT_GET, (,))};                       \
	static ipm_callback_t mchp_ihcm_cb_list##node[ARRAY_SIZE(mchp_ihcc##node)] = {             \
		NULL};                                                                             \
	static void *mchp_ihcm_user_data##node[ARRAY_SIZE(mchp_ihcc##node)] = {                    \
		NULL};                                                                             \
	static uint32_t mchp_ihcm_cb_idx##node[ARRAY_SIZE(mchp_ihcc##node)] = {                    \
		0};                                                                                \
	struct mchp_ihcm_data mchp_ihcm_data##node = {                                             \
		.isr_counter = 0,                                                                  \
		.module_lock = &mchp_ihcm_lock##node,                                              \
		.cb_list = mchp_ihcm_cb_list##node,                                                \
		.cb_user_data_list = mchp_ihcm_user_data##node,                                    \
		.cb_idx_list = mchp_ihcm_cb_idx##node,                                             \
		.num_cb = ARRAY_SIZE(mchp_ihcm_cb_list##node),                                     \
	};                                                                                         \
	struct mchp_ihcm_config mchp_ihcm_config##node = {                                         \
		.ihcm_regs = DT_REG_ADDR(node),                                                    \
		.ihcc_list = mchp_ihcc##node,	                                                   \
		.num_ihcc = ARRAY_SIZE(mchp_ihcc##node),                                           \
		.irq_idx = DT_IRQN(node),                                                          \
		.config_func =  ihcm_cfg_func_##node,                                              \
	};                                                                                         \
	DEVICE_DT_DEFINE(node, NULL, NULL, &mchp_ihcm_data##node,                                  \
			 &mchp_ihcm_config##node, POST_KERNEL,                                     \
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);                               \
	static int ihcm_cfg_func_##node(void)                                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(node),                                                         \
			    DT_IRQ(node, priority),                                                \
			    mchp_ihcm_irq_handler,                                                 \
			    DEVICE_DT_GET(node),                                                   \
			    0);                                                                    \
		return 0;                                                                          \
	};

#define MCHP_IHC_INIT(inst)                                                                        \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, MCHP_IHCM_INIT);                                   \
	static const struct device *mchp_ihcm##inst[] = {                                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst, DEVICE_DT_GET, (,))};                  \
	static struct mchp_ihc_config mchp_ihc_device_cfg_##inst = {	                           \
		.ihc_regs = DT_INST_REG_ADDR(inst),                                                \
		.ihcm_list = mchp_ihcm##inst,                                                      \
		.num_ihcm = ARRAY_SIZE(mchp_ihcm##inst),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &mchp_ihc_init, NULL,                                          \
			NULL /*data*/, &mchp_ihc_device_cfg_##inst,                                \
			PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                         \
			NULL);

DT_INST_FOREACH_STATUS_OKAY(MCHP_IHC_INIT);
