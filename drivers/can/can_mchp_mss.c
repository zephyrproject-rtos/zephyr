/*
 * Copyright (C) 2025 embedded brains GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is a CAN driver for the Microchip PolarFire SoC.
 *
 * The RTR Auto-Reply feature is not supported by the driver.
 *
 * The driver supports the CAN_MODE_MANUAL_RECOVERY independent of the
 * CONFIG_CAN_MANUAL_RECOVERY_MODE setting.  In automatic recovery mode
 * (default), the CAN controller restarts after a bus-off automatically
 * after 128 groups of 11 recessive bits.  In manual recovery mode, the driver
 * is stopped after a bus-off.  It is not clear what the recover handler should
 * do.
 *
 * The receive buffer linking is enabled for all receive buffers except the
 * last one.
 */

#define DT_DRV_COMPAT microchip_mpfs_can

#include <stdbool.h>
#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

/* Is MSS CAN module 'resets' line property defined */
#define MSS_CAN_RESET_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)

#if MSS_CAN_RESET_ENABLED
#include <zephyr/drivers/reset.h>
#endif

LOG_MODULE_REGISTER(mss_can, CONFIG_CAN_LOG_LEVEL);

#define MSS_CAN_RX_MSG_COUNT 32
#define MSS_CAN_TX_MSG_COUNT 32

#define MSS_CAN_SUPPORTED_MODES                                                                    \
	(CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT |           \
	 CAN_MODE_MANUAL_RECOVERY)

#define MSS_CAN_MAX_BITRATE 1000000

/* MSS CAN register offsets */
#define MSS_CAN_INT_STATUS    0x000U
#define MSS_CAN_INT_ENABLE    0x004U
#define MSS_CAN_RX_BUF_STATUS 0x008U
#define MSS_CAN_TX_BUF_STATUS 0x00cU
#define MSS_CAN_ERROR_STATUS  0x010U
#define MSS_CAN_COMMAND       0x014U
#define MSS_CAN_CONFIG        0x018U
#define MSS_CAN_ECR           0x01cU
#define MSS_CAN_TX_MSG(idx)   (0x020U + 16U * (idx))
#define MSS_CAN_RX_MSG(idx)   (0x220U + 32U * (idx))

/* CAN TX_MSG register offsets relative to MSS_CAN_TX_MSG() */
#define MSS_CAN_TX_MSG_CTRL_CMD  0x0U
#define MSS_CAN_TX_MSG_ID        0x4U
#define MSS_CAN_TX_MSG_DATA_HIGH 0x8U
#define MSS_CAN_TX_MSG_DATA_LOW  0xcU

/* CAN RX_MSG register offsets relative to MSS_CAN_RX_MSG() */
#define MSS_CAN_RX_MSG_CTRL_CMD  0x00U
#define MSS_CAN_RX_MSG_ID        0x04U
#define MSS_CAN_RX_MSG_DATA_HIGH 0x08U
#define MSS_CAN_RX_MSG_DATA_LOW  0x0cU
#define MSS_CAN_RX_MSG_AMR       0x10U
#define MSS_CAN_RX_MSG_ACR       0x14U
#define MSS_CAN_RX_MSG_AMR_DATA  0x18U
#define MSS_CAN_RX_MSG_ACR_DATA  0x1cU

/* CAN INT_STATUS and INT_ENABLE register bits */
#define MSS_CAN_INT_GLOBAL      BIT(0)
#define MSS_CAN_INT_ARB_LOSS    BIT(2)
#define MSS_CAN_INT_OVR_LOAD    BIT(3)
#define MSS_CAN_INT_BIT_ERR     BIT(4)
#define MSS_CAN_INT_STUFF_ERR   BIT(5)
#define MSS_CAN_INT_ACK_ERR     BIT(6)
#define MSS_CAN_INT_FORM_ERR    BIT(7)
#define MSS_CAN_INT_CRC_ERR     BIT(8)
#define MSS_CAN_INT_BUS_OFF     BIT(9)
#define MSS_CAN_INT_RX_MSG_LOSS BIT(10)
#define MSS_CAN_INT_TX_MSG      BIT(11)
#define MSS_CAN_INT_RX_MSG      BIT(12)
#define MSS_CAN_INT_RTR_MSG     BIT(13)
#define MSS_CAN_INT_STUCK_AT_0  BIT(14)
#define MSS_CAN_INT_SST_FAILURE BIT(15)

#ifdef CONFIG_CAN_STATS
#define MSS_CAN_INT_ERROR_MASK                                                                     \
	(MSS_CAN_INT_BUS_OFF | MSS_CAN_INT_BIT_ERR | MSS_CAN_INT_STUFF_ERR | MSS_CAN_INT_ACK_ERR | \
	 MSS_CAN_INT_FORM_ERR | MSS_CAN_INT_CRC_ERR | MSS_CAN_INT_RX_MSG_LOSS)
#else
#define MSS_CAN_INT_ERROR_MASK MSS_CAN_INT_BUS_OFF
#endif

#define MSS_CAN_INT_ENABLE_MASK                                                                    \
	(MSS_CAN_INT_ERROR_MASK | MSS_CAN_INT_GLOBAL | MSS_CAN_INT_TX_MSG | MSS_CAN_INT_RX_MSG |   \
	 MSS_CAN_INT_RTR_MSG)

/* Error status register bits */
#define MSS_CAN_ESR_TX_ERR_CNT          GENMASK(7, 0)
#define MSS_CAN_ESR_RX_ERR_CNT          GENMASK(15, 8)
#define MSS_CAN_ESR_ERROR_STATE         GENMASK(17, 16)
#define MSS_CAN_ESR_ERROR_STATE_ACTIVE  0x0U
#define MSS_CAN_ESR_ERROR_STATE_PASSIVE 0x1U
#define MSS_CAN_ESR_ERROR_STATE_BUS_OFF 0x2U
#define MSS_CAN_ESR_TXGTE96             BIT(18)
#define MSS_CAN_ESR_RXGTE96             BIT(19)

/* Command register bits */
#define MSS_CAN_CMD_RUN_MODE           BIT(0)
#define MSS_CAN_CMD_LISTEN_ONLY_MODE   BIT(1)
#define MSS_CAN_CMD_LOOPBACK_TEST_MODE BIT(2)
#define MSS_CAN_CMD_SRAM_TEST_MODE     BIT(3)
#define MSS_CAN_CMD_REVISION_CONTROL   GENMASK(31, 16)

/* Config register bits */
#define MSS_CAN_CFG_EDGE_MODE     BIT(0)
#define MSS_CAN_CFG_SAMPLING_MODE BIT(1)
#define MSS_CAN_CFG_SJW           GENMASK(3, 2)
#define MSS_CAN_CFG_AUTO_RESTART  BIT(4)
#define MSS_CAN_CFG_TSEG2         GENMASK(7, 5)
#define MSS_CAN_CFG_TSEG1         GENMASK(11, 8)
#define MSS_CAN_CFG_ARBITER       BIT(12)
#define MSS_CAN_CFG_SWAP_ENDIAN   BIT(13)
#define MSS_CAN_CFG_ECR_MODE      BIT(14)
#define MSS_CAN_CFG_BITRATE       GENMASK(30, 16)

/* ECR register bits */
#define MSS_CAN_ECR_STATUS     BIT(0)
#define MSS_CAN_ECR_ERROR_TYPE GENMASK(3, 1)
#define MSS_CAN_ECR_TX_MODE    BIT(4)
#define MSS_CAN_ECR_RX_MODE    BIT(5)
#define MSS_CAN_ECR_BIT_NUMBER GENMASK(11, 6)
#define MSS_CAN_ECR_FILED      GENMASK(16, 12)

/* TX_MSG_ID/RX_MSG_ID register bits */
#define MSS_CAN_MSG_ID GENMASK(31, 3)

/* TX_MSG_CTRL_CMD CTRL register bits */
#define MSS_CAN_TX_MSG_CTRL_CMD_TXREQ    BIT(0)
#define MSS_CAN_TX_MSG_CTRL_CMD_TXABORT  BIT(1)
#define MSS_CAN_TX_MSG_CTRL_CMD_TXINTEBL BIT(2)
#define MSS_CAN_TX_MSG_CTRL_CMD_WPN_A    BIT(3)
#define MSS_CAN_TX_MSG_CTRL_CMD_DLC      GENMASK(19, 16)
#define MSS_CAN_TX_MSG_CTRL_CMD_IDE      BIT(20)
#define MSS_CAN_TX_MSG_CTRL_CMD_RTR      BIT(21)
#define MSS_CAN_TX_MSG_CTRL_CMD_WPN_B    BIT(23)

/* RX_MSG_CTRL_CMD register bits */
#define MSS_CAN_RX_MSG_CTRL_CMD_MSGAV_RTRS    BIT(0)
#define MSS_CAN_RX_MSG_CTRL_CMD_RTRP          BIT(1)
#define MSS_CAN_RX_MSG_CTRL_CMD_RTRABORT      BIT(2)
#define MSS_CAN_RX_MSG_CTRL_CMD_RXBUFFEREBL   BIT(3)
#define MSS_CAN_RX_MSG_CTRL_CMD_RTR_REPLY     BIT(4)
#define MSS_CAN_RX_MSG_CTRL_CMD_RX_INT_ENABLE BIT(5)
#define MSS_CAN_RX_MSG_CTRL_CMD_LF            BIT(6)
#define MSS_CAN_RX_MSG_CTRL_CMD_WPNL          BIT(7)
#define MSS_CAN_RX_MSG_CTRL_CMD_DLC           GENMASK(19, 16)
#define MSS_CAN_RX_MSG_CTRL_CMD_IDE           BIT(20)
#define MSS_CAN_RX_MSG_CTRL_CMD_RTR           BIT(21)
#define MSS_CAN_RX_MSG_CTRL_CMD_WPNH          BIT(23)

/* RX_MSG_AMR and RX_MSG_ACR register bits */
#define MSS_CAN_RX_MSG_ACC_RTR BIT(1)
#define MSS_CAN_RX_MSG_ACC_IDE BIT(2)
#define MSS_CAN_RX_MSG_ACC_ID  GENMASK(31, 3)

struct mss_can_config {
	const struct can_driver_config common;
	uintptr_t reg_base;
	uint32_t irq;
	uint32_t clock_freq;
#if MSS_CAN_RESET_ENABLED
	struct reset_dt_spec reset_spec;
#endif
};

struct mss_can_tx_callback {
	can_tx_callback_t cb;
	void *user_data;
};

struct mss_can_rx_callback {
	can_rx_callback_t cb;
	void *user_data;
};

struct mss_can_data {
	struct can_driver_data common;
	struct k_mutex mtx;
	struct k_spinlock rx_lock;
	struct k_spinlock tx_lock;
	struct k_sem tx_sem;

	/*
	 * This bit field is used to account for transmit buffers which are in
	 * use.  Used buffers contain a CAN frame to transmit and the driver
	 * did issue a transmit request.  The mss_can_irq_tx_done() function
	 * processes used buffers to make them available for reuse.
	 */
	uint32_t tx_used;

	struct mss_can_tx_callback tx_callbacks[MSS_CAN_TX_MSG_COUNT];
	struct mss_can_rx_callback rx_callbacks[MSS_CAN_RX_MSG_COUNT];
};

static void mss_can_stop_and_disable_interrupts(uintptr_t reg_base)
{
	uint32_t can_cmd = sys_read32(reg_base + MSS_CAN_COMMAND);

	can_cmd &= ~(MSS_CAN_CMD_RUN_MODE | MSS_CAN_CMD_LISTEN_ONLY_MODE |
		     MSS_CAN_CMD_LOOPBACK_TEST_MODE | MSS_CAN_CMD_SRAM_TEST_MODE);
	sys_write32(can_cmd, reg_base + MSS_CAN_COMMAND);
	sys_write32(0, reg_base + MSS_CAN_INT_ENABLE);
}

static void mss_can_write_command(uintptr_t reg_base, can_mode_t mode)
{
	uint32_t can_cmd = sys_read32(reg_base + MSS_CAN_COMMAND);

	can_cmd &= ~(MSS_CAN_CMD_LISTEN_ONLY_MODE | MSS_CAN_CMD_LOOPBACK_TEST_MODE |
		     MSS_CAN_CMD_SRAM_TEST_MODE);
	can_cmd |= MSS_CAN_CMD_RUN_MODE;

	if ((mode & (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) != 0) {
		can_cmd |= MSS_CAN_CMD_LISTEN_ONLY_MODE;
	}

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		can_cmd |= MSS_CAN_CMD_LOOPBACK_TEST_MODE;
	}

	sys_write32(can_cmd, reg_base + MSS_CAN_COMMAND);
}

static void mss_can_write_config(uintptr_t reg_base, can_mode_t mode)
{
	uint32_t can_cfg = sys_read32(reg_base + MSS_CAN_CONFIG);

	if ((mode & CAN_MODE_MANUAL_RECOVERY) != 0) {
		can_cfg &= ~MSS_CAN_CFG_AUTO_RESTART;
	} else {
		can_cfg |= MSS_CAN_CFG_AUTO_RESTART;
	}

	sys_write32(can_cfg, reg_base + MSS_CAN_CONFIG);
}

static int mss_can_reset(const struct device *dev)
{
	const struct mss_can_config *cfg = dev->config;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t can_config;

#if MSS_CAN_RESET_ENABLED
	if (cfg->reset_spec.dev != NULL) {
		(void)reset_line_toggle_dt(&cfg->reset_spec);
	}
#endif

	mss_can_stop_and_disable_interrupts(reg_base);
	BUILD_ASSERT(MSS_CAN_TX_MSG_COUNT == MSS_CAN_RX_MSG_COUNT);

	can_config = sys_read32(reg_base + MSS_CAN_CONFIG);
#if CONFIG_CAN_MCHP_MSS_SWAP_ENDIANNESS
	can_config |= MSS_CAN_CFG_SWAP_ENDIAN;
#else
	can_config &= ~MSS_CAN_CFG_SWAP_ENDIAN;
#endif
	sys_write32(can_config, reg_base + MSS_CAN_CONFIG);

	for (uintptr_t i = 0; i < MSS_CAN_TX_MSG_COUNT; ++i) {
		/* Clear transmit buffer */
		uintptr_t tx_msg_base = reg_base + MSS_CAN_TX_MSG(i);
		uint32_t tx_msg_ctrl_cmd =
			MSS_CAN_TX_MSG_CTRL_CMD_WPN_A | MSS_CAN_TX_MSG_CTRL_CMD_WPN_B;

		sys_write32(tx_msg_ctrl_cmd, tx_msg_base + MSS_CAN_TX_MSG_CTRL_CMD);
		sys_write32(0, tx_msg_base + MSS_CAN_TX_MSG_ID);
		sys_write32(0, tx_msg_base + MSS_CAN_TX_MSG_DATA_HIGH);
		sys_write32(0, tx_msg_base + MSS_CAN_TX_MSG_DATA_LOW);

		/* Clear receive buffer */
		uintptr_t rx_msg_base = reg_base + MSS_CAN_RX_MSG(i);
		uint32_t rx_msg_ctrl_cmd =
			MSS_CAN_RX_MSG_CTRL_CMD_WPNL | MSS_CAN_RX_MSG_CTRL_CMD_WPNH;

		sys_write32(rx_msg_ctrl_cmd, rx_msg_base + MSS_CAN_RX_MSG_CTRL_CMD);
		sys_write32(0, rx_msg_base + MSS_CAN_RX_MSG_ID);
		sys_write32(0, rx_msg_base + MSS_CAN_RX_MSG_DATA_HIGH);
		sys_write32(0, rx_msg_base + MSS_CAN_RX_MSG_DATA_LOW);
		sys_write32(0, rx_msg_base + MSS_CAN_RX_MSG_AMR);
		sys_write32(0, rx_msg_base + MSS_CAN_RX_MSG_ACR);
		sys_write32(0xffffffffU, rx_msg_base + MSS_CAN_RX_MSG_AMR_DATA);
		sys_write32(0, rx_msg_base + MSS_CAN_RX_MSG_ACR_DATA);
	}

	struct can_timing timing = {0};
	int err = can_calc_timing(dev, &timing, cfg->common.bitrate, cfg->common.sample_point);

	if (err != 0) {
		return err;
	}

	err = can_set_timing(dev, &timing);

	if (err != 0) {
		return err;
	}

	return 0;
}

static int mss_can_send(const struct device *dev, const struct can_frame *frame,
			k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		return -ENOTSUP;
	}

	if (frame->dlc > CAN_MAX_DLC) {
		return -EINVAL;
	}

	struct mss_can_data *data = dev->data;
	int err = k_sem_take(&data->tx_sem, timeout);

	if (unlikely(err != 0)) {
		return err;
	}

	const struct mss_can_config *cfg = dev->config;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t tx_msg_ctrl_cmd = FIELD_PREP(MSS_CAN_TX_MSG_CTRL_CMD_DLC, frame->dlc) |
				   MSS_CAN_TX_MSG_CTRL_CMD_TXREQ |
				   MSS_CAN_TX_MSG_CTRL_CMD_TXINTEBL |
				   MSS_CAN_TX_MSG_CTRL_CMD_WPN_A | MSS_CAN_TX_MSG_CTRL_CMD_WPN_B;
	int id_shift;

	if ((frame->flags & CAN_FRAME_IDE) == 0) {
		id_shift = 29 - 11 + 3;
	} else {
		id_shift = 3;
		tx_msg_ctrl_cmd |= MSS_CAN_TX_MSG_CTRL_CMD_IDE;
	}

	if (unlikely((frame->flags & CAN_FRAME_RTR) != 0)) {
		tx_msg_ctrl_cmd |= MSS_CAN_TX_MSG_CTRL_CMD_RTR;
	}

	if (unlikely((data->common.mode & CAN_MODE_ONE_SHOT) != 0)) {
		tx_msg_ctrl_cmd |= MSS_CAN_TX_MSG_CTRL_CMD_TXABORT;
	}

	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

	if (likely(data->common.started)) {
		/*
		 * Since we were able to take the transmit semaphore, at least
		 * one buffer is not used.
		 */
		uint32_t tx_used = data->tx_used;
		uint32_t index = find_msb_set(~tx_used) - 1;
		uintptr_t tx_msg_base = reg_base + MSS_CAN_TX_MSG(index);

		/* Request transmission of the frame */
		sys_write32(frame->id << id_shift, tx_msg_base + MSS_CAN_TX_MSG_ID);
		sys_write32(frame->data_32[0], tx_msg_base + MSS_CAN_TX_MSG_DATA_HIGH);
		sys_write32(frame->data_32[1], tx_msg_base + MSS_CAN_TX_MSG_DATA_LOW);
		sys_write32(tx_msg_ctrl_cmd, tx_msg_base + MSS_CAN_TX_MSG_CTRL_CMD);

		data->tx_callbacks[index].cb = callback;
		data->tx_callbacks[index].user_data = user_data;

		/* The buffer is now in use */
		data->tx_used = tx_used | (UINT32_C(1) << index);

		k_spin_unlock(&data->tx_lock, key);
		err = 0;
	} else {
		k_spin_unlock(&data->tx_lock, key);
		k_sem_give(&data->tx_sem);
		err = -ENETDOWN;
	}

	return err;
}

static int mss_can_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				 void *user_data, const struct can_filter *filter)
{
	if ((filter->flags & ~CAN_FILTER_IDE) != 0) {
		return -ENOTSUP;
	}

	uint32_t amr = (~filter->mask) << 3;
	uint32_t acr = filter->id << 3;

	if ((filter->flags & CAN_FILTER_IDE) != 0) {
		acr |= MSS_CAN_RX_MSG_ACC_IDE;
	} else {
		amr <<= 29 - 11;
		amr |= 0x1ffff8U;
		acr <<= 29 - 11;
	}

	amr |= MSS_CAN_RX_MSG_ACC_RTR;

	struct mss_can_data *data = dev->data;
	const struct mss_can_config *cfg = dev->config;
	uintptr_t reg_base = cfg->reg_base;

	k_spinlock_key_t key = k_spin_lock(&data->rx_lock);

	uintptr_t index = 0;

	/* Search for the first available receive message buffer */
	do {
		uintptr_t rx_msg_base = reg_base + MSS_CAN_RX_MSG(index);
		uint32_t rx_msg_ctrl_cmd = sys_read32(rx_msg_base + MSS_CAN_RX_MSG_CTRL_CMD);

		if ((rx_msg_ctrl_cmd & MSS_CAN_RX_MSG_CTRL_CMD_RXBUFFEREBL) == 0) {
			break;
		}

		++index;
	} while (index < MSS_CAN_RX_MSG_COUNT);

	int filter_id;

	if (index != MSS_CAN_RX_MSG_COUNT) {
		uintptr_t rx_msg_base = reg_base + MSS_CAN_RX_MSG(index);
		uint32_t rx_msg_ctrl_cmd = MSS_CAN_RX_MSG_CTRL_CMD_RXBUFFEREBL |
					   MSS_CAN_RX_MSG_CTRL_CMD_RX_INT_ENABLE |
					   MSS_CAN_RX_MSG_CTRL_CMD_WPNL |
					   MSS_CAN_RX_MSG_CTRL_CMD_WPNH;

		if (index < MSS_CAN_RX_MSG_COUNT - 1) {
			rx_msg_ctrl_cmd |= MSS_CAN_RX_MSG_CTRL_CMD_LF;
		}

		sys_write32(amr, rx_msg_base + MSS_CAN_RX_MSG_AMR);
		sys_write32(acr, rx_msg_base + MSS_CAN_RX_MSG_ACR);
		sys_write32(rx_msg_ctrl_cmd, rx_msg_base + MSS_CAN_RX_MSG_CTRL_CMD);

		data->rx_callbacks[index].cb = callback;
		data->rx_callbacks[index].user_data = user_data;
		filter_id = (int)index;
	} else {
		filter_id = -ENOSPC;
	}

	k_spin_unlock(&data->rx_lock, key);
	return filter_id;
}

static void mss_can_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct mss_can_data *data = dev->data;
	const struct mss_can_config *cfg = dev->config;
	uintptr_t index = (uintptr_t)filter_id;

	if (index >= ARRAY_SIZE(data->rx_callbacks)) {
		return;
	}

	uintptr_t reg_base = cfg->reg_base;
	uintptr_t rx_msg_base = reg_base + MSS_CAN_RX_MSG(index);

	k_spinlock_key_t key = k_spin_lock(&data->rx_lock);

	uint32_t rx_msg_ctrl_cmd = MSS_CAN_RX_MSG_CTRL_CMD_WPNL | MSS_CAN_RX_MSG_CTRL_CMD_WPNH;

	sys_write32(rx_msg_ctrl_cmd, rx_msg_base + MSS_CAN_RX_MSG_CTRL_CMD);
	data->rx_callbacks[filter_id].cb = NULL;

	k_spin_unlock(&data->rx_lock, key);
}

static int mss_can_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);
	*cap = MSS_CAN_SUPPORTED_MODES;
	return 0;
}

static int mss_can_start(const struct device *dev)
{
	struct mss_can_data *data = dev->data;
	const struct mss_can_config *cfg = dev->config;
	uintptr_t reg_base = cfg->reg_base;
	int err;

	(void)k_mutex_lock(&data->mtx, K_FOREVER);
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

	if (data->common.started) {
		err = -EALREADY;
	} else {
		CAN_STATS_RESET(dev);
		mss_can_write_config(reg_base, data->common.mode);
		mss_can_write_command(reg_base, data->common.mode);
		sys_write32(MSS_CAN_INT_ENABLE_MASK, reg_base + MSS_CAN_INT_ENABLE);
		irq_enable(cfg->irq);
		data->common.started = true;
		err = 0;
	}

	k_spin_unlock(&data->tx_lock, key);
	k_mutex_unlock(&data->mtx);
	return err;
}

static void mss_can_do_stop(const struct device *dev, struct mss_can_data *data)
{
	const struct mss_can_config *cfg = dev->config;

	data->common.started = false;
	irq_disable(cfg->irq);
	mss_can_stop_and_disable_interrupts(cfg->reg_base);
}

static k_spinlock_key_t mss_can_call_tx_callbacks(const struct device *dev,
						  struct mss_can_data *data, int err,
						  k_spinlock_key_t key)
{
	const struct mss_can_config *cfg = dev->config;
	uintptr_t reg_base = cfg->reg_base;

	while (true) {
		uint32_t tx_used = data->tx_used;

		if (tx_used == 0) {
			break;
		}

		uint32_t index = find_msb_set(tx_used) - 1;
		uint32_t mask = ~(UINT32_C(1) << index);

		/* Abort pending transmit */
		uintptr_t tx_msg_base = reg_base + MSS_CAN_TX_MSG(index);
		uint32_t tx_msg_ctrl_cmd = MSS_CAN_TX_MSG_CTRL_CMD_TXABORT |
					   MSS_CAN_TX_MSG_CTRL_CMD_WPN_A |
					   MSS_CAN_TX_MSG_CTRL_CMD_WPN_B;

		sys_write32(tx_msg_ctrl_cmd, tx_msg_base + MSS_CAN_TX_MSG_CTRL_CMD);

		/* Get a snapshot of the callback with user data */
		can_tx_callback_t cb = data->tx_callbacks[index].cb;
		void *user_data = data->tx_callbacks[index].user_data;

		data->tx_used = tx_used & mask;
		k_spin_unlock(&data->tx_lock, key);
		k_sem_give(&data->tx_sem);

		if (cb != NULL) {
			cb(dev, err, user_data);
		}

		key = k_spin_lock(&data->tx_lock);
	}

	return key;
}

static int mss_can_stop(const struct device *dev)
{
	struct mss_can_data *data = dev->data;
	int err;

	(void)k_mutex_lock(&data->mtx, K_FOREVER);
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

	if (data->common.started) {
		mss_can_do_stop(dev, data);
		key = mss_can_call_tx_callbacks(dev, data, -ENETDOWN, key);
		err = 0;
	} else {
		err = -EALREADY;
	}

	k_spin_unlock(&data->tx_lock, key);
	k_mutex_unlock(&data->mtx);
	return err;
}

static int mss_can_set_mode(const struct device *dev, can_mode_t mode)
{
	struct mss_can_data *data = dev->data;
	int err;

	(void)k_mutex_lock(&data->mtx, K_FOREVER);
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

	if (data->common.started) {
		err = -EBUSY;
	} else if ((mode & ~MSS_CAN_SUPPORTED_MODES) != 0) {
		err = -ENOTSUP;
	} else {
		data->common.mode = mode;
		err = 0;
	}

	k_spin_unlock(&data->tx_lock, key);
	k_mutex_unlock(&data->mtx);
	return err;
}

static void mss_can_write_timing(uintptr_t reg_base, const struct can_timing *timing)
{
	uint32_t config = sys_read32(reg_base + MSS_CAN_CONFIG);

	config &= ~(MSS_CAN_CFG_BITRATE | MSS_CAN_CFG_TSEG1 | MSS_CAN_CFG_TSEG2 | MSS_CAN_CFG_SJW);
	config |= FIELD_PREP(MSS_CAN_CFG_BITRATE, timing->prescaler - 1U) |
		  FIELD_PREP(MSS_CAN_CFG_TSEG1, timing->prop_seg + timing->phase_seg1 - 1U) |
		  FIELD_PREP(MSS_CAN_CFG_TSEG2, timing->phase_seg2 - 1U) |
		  FIELD_PREP(MSS_CAN_CFG_SJW, timing->sjw - 1U);

	sys_write32(config, reg_base + MSS_CAN_CONFIG);
}

static int mss_can_set_timing(const struct device *dev, const struct can_timing *timing)
{
	const struct mss_can_config *cfg = dev->config;
	struct mss_can_data *data = dev->data;
	int err;

	(void)k_mutex_lock(&data->mtx, K_FOREVER);

	if (data->common.started) {
		err = -EBUSY;
	} else {
		mss_can_write_timing(cfg->reg_base, timing);
		err = 0;
	}

	(void)k_mutex_unlock(&data->mtx);
	return err;
}

static int mss_can_get_state(const struct device *dev, enum can_state *state,
			     struct can_bus_err_cnt *err_cnt)
{
	struct mss_can_data *data = dev->data;
	const struct mss_can_config *cfg = dev->config;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t esr = sys_read32(reg_base + MSS_CAN_ERROR_STATUS);

	if (state != NULL) {
		if (data->common.started) {
			uint32_t error_state = FIELD_GET(MSS_CAN_ESR_ERROR_STATE, esr);

			if (error_state == MSS_CAN_ESR_ERROR_STATE_ACTIVE) {
				*state = CAN_STATE_ERROR_ACTIVE;
			} else if (error_state == MSS_CAN_ESR_ERROR_STATE_PASSIVE) {
				*state = CAN_STATE_ERROR_PASSIVE;
			} else {
				*state = CAN_STATE_BUS_OFF;
			}
		} else {
			*state = CAN_STATE_STOPPED;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->rx_err_cnt = FIELD_GET(MSS_CAN_ESR_RX_ERR_CNT, esr);
		err_cnt->tx_err_cnt = FIELD_GET(MSS_CAN_ESR_TX_ERR_CNT, esr);
	}

	return 0;
}

static void mss_can_set_state_change_callback(const struct device *dev,
					      can_state_change_callback_t callback, void *user_data)
{
	struct mss_can_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;
	k_spin_unlock(&data->tx_lock, key);
}

static int mss_can_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mss_can_config *cfg = dev->config;

	*rate = cfg->clock_freq;
	return 0;
}

static int mss_can_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);
	return MSS_CAN_RX_MSG_COUNT;
}

static void mss_can_irq_tx_done(const struct device *dev, struct mss_can_data *data,
				uintptr_t reg_base)
{
	/*
	 * We have to negate the status since we are interested in buffers
	 * where no transmit request is pending.
	 */
	uint32_t tx_buf_status = ~sys_read32(reg_base + MSS_CAN_TX_BUF_STATUS);

	while (true) {
		k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

		/* We are only interested in used buffers */
		uint32_t tx_used = data->tx_used;

		tx_buf_status &= tx_used;

		if (tx_buf_status == 0) {
			k_spin_unlock(&data->tx_lock, key);
			break;
		}

		/*
		 * This buffer is used and there is no transmit request
		 * pending, thus it was transmitted or aborted.  There is no
		 * way to figure out what happened exactly.
		 */
		uint32_t index = find_msb_set(tx_buf_status) - 1;
		uint32_t mask = ~(UINT32_C(1) << index);

		/* Get a snapshot of the callback with user data */
		can_tx_callback_t cb = data->tx_callbacks[index].cb;
		void *user_data = data->tx_callbacks[index].user_data;

		/* The buffer is no longer used */
		data->tx_used = tx_used & mask;

		k_spin_unlock(&data->tx_lock, key);
		k_sem_give(&data->tx_sem);

		if (cb != NULL) {
			cb(dev, 0, user_data);
		}

		/* We are done with this buffer for now */
		tx_buf_status &= mask;
	}
}

static void mss_can_irq_rx_available(const struct device *dev, struct mss_can_data *data,
				     uintptr_t reg_base)
{
	uint32_t rx_buf_status = sys_read32(reg_base + MSS_CAN_RX_BUF_STATUS);

	while (rx_buf_status != 0) {
		uint32_t index = find_msb_set(rx_buf_status) - 1;
		uintptr_t rx_msg_base = reg_base + MSS_CAN_RX_MSG(index);

		k_spinlock_key_t key = k_spin_lock(&data->rx_lock);

		uint32_t rx_msg_ctrl_cmd = sys_read32(rx_msg_base + MSS_CAN_RX_MSG_CTRL_CMD);
		uint32_t id = sys_read32(rx_msg_base + MSS_CAN_RX_MSG_ID);
		uint8_t flags = 0;

		if ((rx_msg_ctrl_cmd & MSS_CAN_RX_MSG_CTRL_CMD_IDE) != 0) {
			flags |= CAN_FRAME_IDE;
			id >>= 3;
		} else {
			id >>= 3 + 29 - 11;
		}

		if ((rx_msg_ctrl_cmd & MSS_CAN_RX_MSG_CTRL_CMD_RTR) != 0) {
			flags |= CAN_FRAME_RTR;
		}

		struct can_frame frame = {
			.id = id,
			.dlc = FIELD_GET(MSS_CAN_RX_MSG_CTRL_CMD_DLC, rx_msg_ctrl_cmd),
			.flags = flags,
			.data_32 = {sys_read32(rx_msg_base + MSS_CAN_RX_MSG_DATA_HIGH),
				    sys_read32(rx_msg_base + MSS_CAN_RX_MSG_DATA_LOW)}};

		/* Make buffer available for new messages */
		rx_msg_ctrl_cmd |= MSS_CAN_RX_MSG_CTRL_CMD_MSGAV_RTRS;
		sys_write32(rx_msg_ctrl_cmd, rx_msg_base + MSS_CAN_RX_MSG_CTRL_CMD);

		/* Get a snapshot of the callback with user data */
		can_rx_callback_t cb = data->rx_callbacks[index].cb;
		void *user_data = data->rx_callbacks[index].user_data;

		k_spin_unlock(&data->rx_lock, key);

		if (cb != NULL) {
			cb(dev, &frame, user_data);
		}

		/* We are done with this buffer for now */
		rx_buf_status &= ~(UINT32_C(1) << index);
	}
}

static void mss_can_irq_error(const struct device *dev, struct mss_can_data *data,
			      uint32_t int_status)
{
#ifdef CONFIG_CAN_STATS
	if ((int_status & MSS_CAN_INT_BIT_ERR) != 0) {
		CAN_STATS_BIT_ERROR_INC(dev);
	}

	if ((int_status & MSS_CAN_INT_STUFF_ERR) != 0) {
		CAN_STATS_STUFF_ERROR_INC(dev);
	}

	if ((int_status & MSS_CAN_INT_ACK_ERR) != 0) {
		CAN_STATS_ACK_ERROR_INC(dev);
	}

	if ((int_status & MSS_CAN_INT_FORM_ERR) != 0) {
		CAN_STATS_FORM_ERROR_INC(dev);
	}

	if ((int_status & MSS_CAN_INT_CRC_ERR) != 0) {
		CAN_STATS_CRC_ERROR_INC(dev);
	}

	if ((int_status & MSS_CAN_INT_RX_MSG_LOSS) != 0) {
		CAN_STATS_RX_OVERRUN_INC(dev);
	}
#endif

	if ((int_status & MSS_CAN_INT_BUS_OFF) != 0) {
		k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

		if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) != 0) {
			mss_can_do_stop(dev, data);
		}

		key = mss_can_call_tx_callbacks(dev, data, -ENETUNREACH, key);

		enum can_state state;
		struct can_bus_err_cnt err_cnt;
		(void)mss_can_get_state(dev, &state, &err_cnt);

		const can_state_change_callback_t cb = data->common.state_change_cb;
		void *user_data = data->common.state_change_cb_user_data;

		k_spin_unlock(&data->tx_lock, key);

		if (cb != NULL) {
			cb(dev, state, err_cnt, user_data);
		}
	}
}

static void mss_can_irq_handler(const struct device *dev)
{
	const struct mss_can_config *cfg = dev->config;
	struct mss_can_data *data = dev->data;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t int_status = sys_read32(reg_base + MSS_CAN_INT_STATUS);

	/* Clear interrupts */
	sys_write32(int_status, reg_base + MSS_CAN_INT_STATUS);

	if ((int_status & MSS_CAN_INT_TX_MSG) != 0) {
		mss_can_irq_tx_done(dev, data, reg_base);
	}

	if ((int_status & MSS_CAN_INT_RX_MSG) != 0) {
		mss_can_irq_rx_available(dev, data, reg_base);
	}

	if (unlikely(int_status & MSS_CAN_INT_ERROR_MASK) != 0) {
		mss_can_irq_error(dev, data, int_status);
	}
}

static int mss_can_init(const struct device *dev)
{
	struct mss_can_data *data = dev->data;

	(void)k_mutex_init(&data->mtx);
	(void)k_sem_init(&data->tx_sem, MSS_CAN_TX_MSG_COUNT, MSS_CAN_TX_MSG_COUNT);
	return mss_can_reset(dev);
}

static DEVICE_API(can, mss_can_driver_api) = {
	.get_capabilities = mss_can_get_capabilities,
	.start = mss_can_start,
	.stop = mss_can_stop,
	.set_mode = mss_can_set_mode,
	.set_timing = mss_can_set_timing,
	.send = mss_can_send,
	.add_rx_filter = mss_can_add_rx_filter,
	.remove_rx_filter = mss_can_remove_rx_filter,
	.get_state = mss_can_get_state,
	.set_state_change_callback = mss_can_set_state_change_callback,
	.get_core_clock = mss_can_get_core_clock,
	.get_max_filters = mss_can_get_max_filters,
	/* Recommended configuration ranges from CiA 601-2 */
	.timing_min = {.sjw = 1, .prop_seg = 0, .phase_seg1 = 2, .phase_seg2 = 2, .prescaler = 1},
	.timing_max = {
		.sjw = 4, .prop_seg = 0, .phase_seg1 = 16, .phase_seg2 = 8, .prescaler = 32768}};

#define MSS_CAN_INIT(n)                                                                            \
	static int mss_can_init_##n(const struct device *dev)                                      \
	{                                                                                          \
		mss_can_init(dev);                                                                 \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mss_can_irq_handler,        \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct mss_can_config mss_can_config_##n = {                                  \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(n, 0, MSS_CAN_MAX_BITRATE),                \
		.reg_base = DT_INST_REG_ADDR(n),                                                   \
		.irq = DT_INST_IRQN(n),                                                            \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, resets),                                       \
			(.reset_spec = RESET_DT_SPEC_INST_GET(n),)) };    \
                                                                                                   \
	static struct mss_can_data mss_can_data_##n;                                               \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(n, mss_can_init_##n, NULL, &mss_can_data_##n,                    \
				  &mss_can_config_##n, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,      \
				  &mss_can_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSS_CAN_INIT)
