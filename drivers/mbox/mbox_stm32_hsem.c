/*
 * Copyright (c) 2024 Celina Sophie Kalus <hello@celinakalus.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include "stm32_hsem.h"

LOG_MODULE_REGISTER(mbox_stm32_hsem_ipc, CONFIG_MBOX_LOG_LEVEL);

#define DT_DRV_COMPAT st_mbox_stm32_hsem

#define HSEM_CPU1                   1
#define HSEM_CPU2                   2

#if DT_NODE_EXISTS(DT_NODELABEL(cpu0))
#define HSEM_CPU_ID HSEM_CPU1
#elif DT_NODE_EXISTS(DT_NODELABEL(cpu1))
#define HSEM_CPU_ID HSEM_CPU2
#else
#error "Neither cpu0 nor cpu1 defined!"
#endif

#if HSEM_CPU_ID == HSEM_CPU1
#define MBOX_TX_HSEM_ID CFG_HW_IPM_CPU2_SEMID
#define MBOX_RX_HSEM_ID CFG_HW_IPM_CPU1_SEMID
#else /* HSEM_CPU2 */
#define MBOX_TX_HSEM_ID CFG_HW_IPM_CPU1_SEMID
#define MBOX_RX_HSEM_ID CFG_HW_IPM_CPU2_SEMID
#endif /* HSEM_CPU_ID */

#define MAX_CHANNELS 2

struct mbox_stm32_hsem_data {
	const struct device *dev;
	mbox_callback_t cb;
	void *user_data;
};

static struct mbox_stm32_hsem_data stm32_hsem_mbox_data;

static struct mbox_stm32_hsem_conf {
	struct stm32_pclken pclken;
} stm32_hsem_mbox_conf = {
	.pclken = {
		.bus = DT_INST_CLOCKS_CELL(0, bus),
		.enr = DT_INST_CLOCKS_CELL(0, bits)
	},
};

static inline void stm32_hsem_enable_rx_interrupt(void)
{
	const uint32_t mask_hsem_id = BIT(MBOX_RX_HSEM_ID);

#if HSEM_CPU_ID == HSEM_CPU1
	LL_HSEM_EnableIT_C1IER(HSEM, mask_hsem_id);
#else /* HSEM_CPU2 */
	LL_HSEM_EnableIT_C2IER(HSEM, mask_hsem_id);
#endif /* HSEM_CPU_ID */
}

static inline void stm32_hsem_disable_rx_interrupt(void)
{
	const uint32_t mask_hsem_id = BIT(MBOX_RX_HSEM_ID);

#if HSEM_CPU_ID == HSEM_CPU1
	LL_HSEM_DisableIT_C1IER(HSEM, mask_hsem_id);
#else /* HSEM_CPU2 */
	LL_HSEM_DisableIT_C2IER(HSEM, mask_hsem_id);
#endif /* HSEM_CPU_ID */
}

static inline void stm32_hsem_clear_rx_interrupt(void)
{
	const uint32_t mask_hsem_id = BIT(MBOX_RX_HSEM_ID);

#if HSEM_CPU_ID == HSEM_CPU1
	LL_HSEM_ClearFlag_C1ICR(HSEM, mask_hsem_id);
#else /* HSEM_CPU2 */
	LL_HSEM_ClearFlag_C2ICR(HSEM, mask_hsem_id);
#endif /* HSEM_CPU_ID */
}

static inline uint32_t stm32_hsem_is_rx_interrupt_active(void)
{
	const uint32_t mask_hsem_id = BIT(MBOX_RX_HSEM_ID);

#if HSEM_CPU_ID == HSEM_CPU1
	return LL_HSEM_IsActiveFlag_C1ISR(HSEM, mask_hsem_id);
#else /* HSEM_CPU2 */
	return LL_HSEM_IsActiveFlag_C2ISR(HSEM, mask_hsem_id);
#endif /* HSEM_CPU_ID */
}

static inline bool is_rx_channel_valid(const struct device *dev, uint32_t ch)
{
	/* Only support one RX channel */
	return (ch == MBOX_RX_HSEM_ID);
}

static inline bool is_tx_channel_valid(const struct device *dev, uint32_t ch)
{
	/* Only support one TX channel */
	return (ch == MBOX_TX_HSEM_ID);
}

static void mbox_dispatcher(const struct device *dev)
{
	struct mbox_stm32_hsem_data *data = dev->data;

	/* Check semaphore rx_semid interrupt status */
	if (!stm32_hsem_is_rx_interrupt_active()) {
		return;
	}

	if (data->cb != NULL) {
		data->cb(dev, MBOX_RX_HSEM_ID, data->user_data, NULL);
	}

	/* Clear semaphore rx_semid interrupt status and masked status */
	stm32_hsem_clear_rx_interrupt();
}

static int mbox_stm32_hsem_send(const struct device *dev, uint32_t channel,
			 const struct mbox_msg *msg)
{
	if (msg) {
		LOG_ERR("Sending data not supported.");
		return -EINVAL;
	}

	if (!is_tx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	/*
	 * Locking and unlocking the hardware semaphore
	 * causes an interrupt on the receiving side.
	 */
	z_stm32_hsem_lock(MBOX_TX_HSEM_ID, HSEM_LOCK_DEFAULT_RETRY);
	z_stm32_hsem_unlock(MBOX_TX_HSEM_ID);

	return 0;
}

static int mbox_stm32_hsem_register_callback(const struct device *dev, uint32_t channel,
				      mbox_callback_t cb, void *user_data)
{
	struct mbox_stm32_hsem_data *data = dev->data;

	if (!(is_rx_channel_valid(dev, channel))) {
		return -EINVAL;
	}

	data->cb = cb;
	data->user_data = user_data;

	return 0;
}

static int mbox_stm32_hsem_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* We only support signalling */
	return 0;
}

static uint32_t mbox_stm32_hsem_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Only two channels supported, one RX and one TX */
	return MAX_CHANNELS;
}

static int mbox_stm32_hsem_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	if (!is_rx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	if (enable) {
		stm32_hsem_clear_rx_interrupt();
		stm32_hsem_enable_rx_interrupt();
	} else {
		stm32_hsem_disable_rx_interrupt();
	}

	return 0;
}

#if HSEM_CPU_ID == HSEM_CPU1
static int mbox_stm32_clock_init(const struct device *dev)
{
	const struct mbox_stm32_hsem_conf *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("Clock control device not ready.");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken) != 0) {
		LOG_WRN("Failed to enable clock.");
		return -EIO;
	}

	return 0;
}
#endif /* HSEM_CPU_ID */

static int mbox_stm32_hsem_init(const struct device *dev)
{
	struct mbox_stm32_hsem_data *data = dev->data;
	int ret = 0;

	data->dev = dev;

#if HSEM_CPU_ID == HSEM_CPU1
	ret = mbox_stm32_clock_init(dev);

	if (ret != 0) {
		return ret;
	}
#endif /* HSEM_CPU_ID */

	/* Configure interrupt service routine */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    mbox_dispatcher, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	return ret;
}

static DEVICE_API(mbox, mbox_stm32_hsem_driver_api) = {
	.send = mbox_stm32_hsem_send,
	.register_callback = mbox_stm32_hsem_register_callback,
	.mtu_get = mbox_stm32_hsem_mtu_get,
	.max_channels_get = mbox_stm32_hsem_max_channels_get,
	.set_enabled = mbox_stm32_hsem_set_enabled,
};

DEVICE_DT_INST_DEFINE(
	0,
	mbox_stm32_hsem_init,
	NULL,
	&stm32_hsem_mbox_data,
	&stm32_hsem_mbox_conf,
	POST_KERNEL,
	CONFIG_MBOX_INIT_PRIORITY,
	&mbox_stm32_hsem_driver_api);
