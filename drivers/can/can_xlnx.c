/*
 * Copyright (c) 2022 Weidmueller Interface GmbH & Co. KG
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the CAN controllers in the Xilinx Zynq-7000 processing system.
 * Register descriptions: Zynq-7000 SoC Technical Reference Manual (UG585),
 * chapter 18 and appendix B.5.
 *
 * - The hardware acceptance filters are disabled, so every frame on the bus
 *   ends up in the RX FIFO and is matched against the RX filters in software.
 * - At most one frame is written to the TX FIFO at a time. The controller has
 *   no TX FIFO fill level register, so this is what allows each TX OK
 *   interrupt to be attributed to exactly one frame.
 * - Bus-off is left by resetting and restarting the controller, after at
 *   least 128 * 11 bit times (automatic recovery) or on can_recover().
 */

#define DT_DRV_COMPAT xlnx_zynq_can_1_0

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(can_xlnx, CONFIG_CAN_LOG_LEVEL);

/* Register offsets */
#define CAN_XLNX_SRR        0x00 /* Software Reset Register */
#define CAN_XLNX_MSR        0x04 /* Mode Select Register */
#define CAN_XLNX_BRPR       0x08 /* Baud Rate Prescaler Register */
#define CAN_XLNX_BTR        0x0C /* Bit Timing Register */
#define CAN_XLNX_ECR        0x10 /* Error Counter Register */
#define CAN_XLNX_ESR        0x14 /* Error Status Register */
#define CAN_XLNX_SR         0x18 /* Status Register */
#define CAN_XLNX_ISR        0x1C /* Interrupt Status Register */
#define CAN_XLNX_IER        0x20 /* Interrupt Enable Register */
#define CAN_XLNX_ICR        0x24 /* Interrupt Clear Register */
#define CAN_XLNX_TXFIFO_ID  0x30
#define CAN_XLNX_TXFIFO_DLC 0x34
#define CAN_XLNX_TXFIFO_DW1 0x38
#define CAN_XLNX_TXFIFO_DW2 0x3C
#define CAN_XLNX_RXFIFO_ID  0x50
#define CAN_XLNX_RXFIFO_DLC 0x54
#define CAN_XLNX_RXFIFO_DW1 0x58
#define CAN_XLNX_RXFIFO_DW2 0x5C
#define CAN_XLNX_AFR        0x60 /* Acceptance Filter Register */

/* SRR */
#define CAN_XLNX_SRR_CEN  BIT(1)
#define CAN_XLNX_SRR_SRST BIT(0)

/* MSR */
#define CAN_XLNX_MSR_LBACK BIT(1)

/* BTR */
#define CAN_XLNX_BTR_SJW GENMASK(8, 7)
#define CAN_XLNX_BTR_TS2 GENMASK(6, 4)
#define CAN_XLNX_BTR_TS1 GENMASK(3, 0)

/* ECR */
#define CAN_XLNX_ECR_REC GENMASK(15, 8)
#define CAN_XLNX_ECR_TEC GENMASK(7, 0)

/* ESR (write 1 to clear) */
#define CAN_XLNX_ESR_ACKER BIT(4)
#define CAN_XLNX_ESR_BERR  BIT(3)
#define CAN_XLNX_ESR_STER  BIT(2)
#define CAN_XLNX_ESR_FMER  BIT(1)
#define CAN_XLNX_ESR_CRCER BIT(0)
#define CAN_XLNX_ESR_ALL   GENMASK(4, 0)

/* SR */
#define CAN_XLNX_SR_ESTAT          GENMASK(8, 7)
#define CAN_XLNX_SR_ESTAT_BUS_OFF  2U
#define CAN_XLNX_SR_ESTAT_ERR_PASV 3U
#define CAN_XLNX_SR_ERRWRN         BIT(6)
#define CAN_XLNX_SR_NORMAL         BIT(3)
#define CAN_XLNX_SR_LBACK          BIT(1)
#define CAN_XLNX_SR_CONFIG         BIT(0)

/* ISR / IER / ICR */
#define CAN_XLNX_IXR_BSOFF  BIT(9)
#define CAN_XLNX_IXR_ERROR  BIT(8)
#define CAN_XLNX_IXR_RXNEMP BIT(7)
#define CAN_XLNX_IXR_RXOFLW BIT(6)
#define CAN_XLNX_IXR_RXUFLW BIT(5)
#define CAN_XLNX_IXR_RXOK   BIT(4)
#define CAN_XLNX_IXR_TXOK   BIT(1)
#define CAN_XLNX_IXR_ALL    GENMASK(14, 0)

#define CAN_XLNX_IER_DEFAULT                                                                       \
	(CAN_XLNX_IXR_BSOFF | CAN_XLNX_IXR_ERROR | CAN_XLNX_IXR_RXNEMP | CAN_XLNX_IXR_RXOFLW |     \
	 CAN_XLNX_IXR_TXOK)

/* FIFO ID word */
#define CAN_XLNX_ID_IDH    GENMASK(31, 21)
#define CAN_XLNX_ID_SRRRTR BIT(20)
#define CAN_XLNX_ID_IDE    BIT(19)
#define CAN_XLNX_ID_IDL    GENMASK(18, 1)
#define CAN_XLNX_ID_RTR    BIT(0)

/* FIFO DLC word */
#define CAN_XLNX_DLC_DLC GENMASK(31, 28)
#define CAN_XLNX_DLC_RXT GENMASK(15, 0)

/* Mode changes complete within a few CAN_REF_CLK cycles */
#define CAN_XLNX_MODE_CHANGE_TIMEOUT_US 1000

/* ISO 11898-1: bus-off is left after 128 occurrences of 11 recessive bits */
#define CAN_XLNX_BUS_OFF_RECOVERY_BITS (128U * 11U)

struct can_xlnx_filter {
	can_rx_callback_t callback;
	void *user_data;
	struct can_filter filter;
};

struct can_xlnx_config {
	const struct can_driver_config common;
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
	uint32_t clock_frequency;
};

struct can_xlnx_data {
	struct can_driver_data common;
	DEVICE_MMIO_NAMED_RAM(reg_base);
	const struct device *dev;
	struct k_mutex lock;
	struct can_timing timing;
	enum can_state state;
	/* Held while a frame is in the TX FIFO */
	struct k_sem tx_idle;
	can_tx_callback_t tx_callback;
	void *tx_user_data;
	ATOMIC_DEFINE(rx_allocs, CONFIG_CAN_XLNX_MAX_FILTERS);
	struct can_xlnx_filter filters[CONFIG_CAN_XLNX_MAX_FILTERS];
	struct k_work_delayable recovery_work;
	/* Set by the bus-off interrupt, cleared when the controller is (re)started or stopped */
	atomic_t recovering;
};

/* The common CAN structs come first, so the MMIO regions use the named variants */
#define DEV_CFG(dev)  ((const struct can_xlnx_config *)(dev)->config)
#define DEV_DATA(dev) ((struct can_xlnx_data *)(dev)->data)

static inline uint32_t can_xlnx_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(dev, reg_base) + reg);
}

static inline void can_xlnx_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_NAMED_GET(dev, reg_base) + reg);
}

static int can_xlnx_wait_sr(const struct device *dev, uint32_t mask)
{
	if (!WAIT_FOR((can_xlnx_read(dev, CAN_XLNX_SR) & mask) != 0U,
		      CAN_XLNX_MODE_CHANGE_TIMEOUT_US, k_busy_wait(1))) {
		return -EIO;
	}

	return 0;
}

/* Reset the controller: clears all registers and both FIFOs, enters configuration mode. */
static int can_xlnx_reset(const struct device *dev)
{
	can_xlnx_write(dev, CAN_XLNX_SRR, CAN_XLNX_SRR_SRST);

	return can_xlnx_wait_sr(dev, CAN_XLNX_SR_CONFIG);
}

/*
 * Program and enable the controller with the stored timing and mode. Interrupts stay disabled
 * in IER, so this does not race with the ISR; the caller enables them with can_xlnx_irq_enable().
 */
static int can_xlnx_hw_start(const struct device *dev)
{
	struct can_xlnx_data *data = dev->data;
	const struct can_timing *timing = &data->timing;
	uint32_t msr = 0U;
	uint32_t ready;
	int err;

	err = can_xlnx_reset(dev);
	if (err != 0) {
		LOG_ERR("failed to enter configuration mode");
		return err;
	}

	can_xlnx_write(dev, CAN_XLNX_BRPR, timing->prescaler - 1U);
	can_xlnx_write(
		dev, CAN_XLNX_BTR,
		FIELD_PREP(CAN_XLNX_BTR_SJW, timing->sjw - 1U) |
			FIELD_PREP(CAN_XLNX_BTR_TS2, timing->phase_seg2 - 1U) |
			FIELD_PREP(CAN_XLNX_BTR_TS1, timing->prop_seg + timing->phase_seg1 - 1U));

	/* All acceptance filters disabled: every frame is received */
	can_xlnx_write(dev, CAN_XLNX_AFR, 0U);

	if ((data->common.mode & CAN_MODE_LOOPBACK) != 0U) {
		msr |= CAN_XLNX_MSR_LBACK;
		ready = CAN_XLNX_SR_LBACK;
	} else {
		ready = CAN_XLNX_SR_NORMAL;
	}
	can_xlnx_write(dev, CAN_XLNX_MSR, msr);

	can_xlnx_write(dev, CAN_XLNX_ICR, CAN_XLNX_IXR_ALL);
	can_xlnx_write(dev, CAN_XLNX_SRR, CAN_XLNX_SRR_CEN);

	err = can_xlnx_wait_sr(dev, ready);
	if (err != 0) {
		LOG_ERR("failed to leave configuration mode");
		(void)can_xlnx_reset(dev);
		return err;
	}

	return 0;
}

/* Events latched since the controller was enabled are serviced once interrupts are unlocked */
static inline void can_xlnx_irq_enable(const struct device *dev)
{
	can_xlnx_write(dev, CAN_XLNX_IER, CAN_XLNX_IER_DEFAULT);
}

static void can_xlnx_tx_done(const struct device *dev, int status)
{
	struct can_xlnx_data *data = dev->data;
	can_tx_callback_t callback = data->tx_callback;
	void *user_data = data->tx_user_data;

	if (callback == NULL) {
		return;
	}

	data->tx_callback = NULL;
	data->tx_user_data = NULL;
	callback(dev, status, user_data);
	k_sem_give(&data->tx_idle);
}

/* Current bus state from the Status Register; the controller must be started. */
static enum can_state can_xlnx_hw_state(const struct device *dev)
{
	uint32_t sr = can_xlnx_read(dev, CAN_XLNX_SR);

	switch (FIELD_GET(CAN_XLNX_SR_ESTAT, sr)) {
	case CAN_XLNX_SR_ESTAT_BUS_OFF:
		return CAN_STATE_BUS_OFF;
	case CAN_XLNX_SR_ESTAT_ERR_PASV:
		return CAN_STATE_ERROR_PASSIVE;
	default:
		return ((sr & CAN_XLNX_SR_ERRWRN) != 0U) ? CAN_STATE_ERROR_WARNING
							 : CAN_STATE_ERROR_ACTIVE;
	}
}

static void can_xlnx_get_err_cnt(const struct device *dev, struct can_bus_err_cnt *err_cnt)
{
	uint32_t ecr = can_xlnx_read(dev, CAN_XLNX_ECR);

	err_cnt->tx_err_cnt = FIELD_GET(CAN_XLNX_ECR_TEC, ecr);
	err_cnt->rx_err_cnt = FIELD_GET(CAN_XLNX_ECR_REC, ecr);
}

static void can_xlnx_set_state(const struct device *dev, enum can_state state)
{
	struct can_xlnx_data *data = dev->data;
	struct can_bus_err_cnt err_cnt;

	if (state == data->state) {
		return;
	}

	data->state = state;

	can_xlnx_get_err_cnt(dev, &err_cnt);
	can_fire_state_change_callbacks(dev, state, err_cnt);
}

static k_timeout_t can_xlnx_recovery_delay(const struct device *dev)
{
	const struct can_xlnx_config *config = dev->config;
	struct can_xlnx_data *data = dev->data;
	const struct can_timing *timing = &data->timing;
	uint32_t tq_per_bit = 1U + timing->prop_seg + timing->phase_seg1 + timing->phase_seg2;
	uint32_t bitrate = config->clock_frequency / (timing->prescaler * tq_per_bit);
	uint32_t delay_us = DIV_ROUND_UP(CAN_XLNX_BUS_OFF_RECOVERY_BITS * USEC_PER_SEC, bitrate);

	return K_USEC(delay_us);
}

/* Enter bus-off: fail the pending frame and reset the controller to flush the TX FIFO. */
static void can_xlnx_handle_bus_off(const struct device *dev)
{
	struct can_xlnx_data *data = dev->data;

	can_xlnx_write(dev, CAN_XLNX_IER, 0U);
	/* Do not wait for configuration mode here; can_xlnx_hw_start() resets again and waits */
	can_xlnx_write(dev, CAN_XLNX_SRR, CAN_XLNX_SRR_SRST);

	can_xlnx_set_state(dev, CAN_STATE_BUS_OFF);
	can_xlnx_tx_done(dev, -ENETUNREACH);

	atomic_set(&data->recovering, 1);
	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		(void)k_work_reschedule(&data->recovery_work, can_xlnx_recovery_delay(dev));
	}
}

/* Leave bus-off by restarting the controller. */
static int can_xlnx_restart(const struct device *dev)
{
	struct can_xlnx_data *data = dev->data;
	unsigned int key;
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->common.started && atomic_get(&data->recovering) != 0) {
		err = can_xlnx_hw_start(dev);
		if (err == 0) {
			key = irq_lock();
			atomic_set(&data->recovering, 0);
			can_xlnx_set_state(dev, CAN_STATE_ERROR_ACTIVE);
			can_xlnx_irq_enable(dev);
			irq_unlock(key);
		} else {
			LOG_ERR("bus-off recovery failed (err %d)", err);
		}
	}

	k_mutex_unlock(&data->lock);

	return err;
}

static void can_xlnx_recovery_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct can_xlnx_data *data = CONTAINER_OF(dwork, struct can_xlnx_data, recovery_work);

	(void)can_xlnx_restart(data->dev);
}

static void can_xlnx_read_frame(const struct device *dev, struct can_frame *frame)
{
	uint32_t id = can_xlnx_read(dev, CAN_XLNX_RXFIFO_ID);
	uint32_t dlc = can_xlnx_read(dev, CAN_XLNX_RXFIFO_DLC);
	uint32_t dw1 = can_xlnx_read(dev, CAN_XLNX_RXFIFO_DW1);
	/* Reading DW2 removes the frame from the RX FIFO */
	uint32_t dw2 = can_xlnx_read(dev, CAN_XLNX_RXFIFO_DW2);

	memset(frame, 0, sizeof(*frame));

	frame->dlc = FIELD_GET(CAN_XLNX_DLC_DLC, dlc);
	if (frame->dlc > CAN_MAX_DLC) {
		frame->dlc = CAN_MAX_DLC;
	}

#ifdef CONFIG_CAN_RX_TIMESTAMP
	frame->timestamp = FIELD_GET(CAN_XLNX_DLC_RXT, dlc);
#endif

	if ((id & CAN_XLNX_ID_IDE) != 0U) {
		frame->flags |= CAN_FRAME_IDE;
		frame->id = (FIELD_GET(CAN_XLNX_ID_IDH, id) << 18) | FIELD_GET(CAN_XLNX_ID_IDL, id);
		if ((id & CAN_XLNX_ID_RTR) != 0U) {
			frame->flags |= CAN_FRAME_RTR;
		}
	} else {
		frame->id = FIELD_GET(CAN_XLNX_ID_IDH, id);
		if ((id & CAN_XLNX_ID_SRRRTR) != 0U) {
			frame->flags |= CAN_FRAME_RTR;
		}
	}

	if ((frame->flags & CAN_FRAME_RTR) == 0U) {
		sys_put_be32(dw1, &frame->data[0]);
		sys_put_be32(dw2, &frame->data[4]);
	}
}

static void can_xlnx_handle_rx(const struct device *dev)
{
	struct can_xlnx_data *data = dev->data;
	struct can_frame frame;
	can_rx_callback_t callback;

	while ((can_xlnx_read(dev, CAN_XLNX_ISR) & CAN_XLNX_IXR_RXNEMP) != 0U) {
		can_xlnx_read_frame(dev, &frame);
		/* RXNEMP is set again if more frames are pending */
		can_xlnx_write(dev, CAN_XLNX_ICR, CAN_XLNX_IXR_RXNEMP | CAN_XLNX_IXR_RXOK);

#ifndef CONFIG_CAN_ACCEPT_RTR
		if ((frame.flags & CAN_FRAME_RTR) != 0U) {
			continue;
		}
#endif

		for (int i = 0; i < ARRAY_SIZE(data->filters); i++) {
			if (!atomic_test_bit(data->rx_allocs, i)) {
				continue;
			}

			if (can_frame_matches_filter(&frame, &data->filters[i].filter)) {
				callback = data->filters[i].callback;
				if (callback != NULL) {
					callback(dev, &frame, data->filters[i].user_data);
				}
			}
		}
	}
}

#ifdef CONFIG_CAN_STATS
static void can_xlnx_update_error_stats(const struct device *dev)
{
	uint32_t esr = can_xlnx_read(dev, CAN_XLNX_ESR);

	can_xlnx_write(dev, CAN_XLNX_ESR, esr & CAN_XLNX_ESR_ALL);

	if ((esr & CAN_XLNX_ESR_ACKER) != 0U) {
		CAN_STATS_ACK_ERROR_INC(dev);
	}
	if ((esr & CAN_XLNX_ESR_BERR) != 0U) {
		CAN_STATS_BIT_ERROR_INC(dev);
	}
	if ((esr & CAN_XLNX_ESR_STER) != 0U) {
		CAN_STATS_STUFF_ERROR_INC(dev);
	}
	if ((esr & CAN_XLNX_ESR_FMER) != 0U) {
		CAN_STATS_FORM_ERROR_INC(dev);
	}
	if ((esr & CAN_XLNX_ESR_CRCER) != 0U) {
		CAN_STATS_CRC_ERROR_INC(dev);
	}
}
#endif /* CONFIG_CAN_STATS */

static void can_xlnx_isr(const struct device *dev)
{
	struct can_xlnx_data *data = dev->data;
	uint32_t isr = can_xlnx_read(dev, CAN_XLNX_ISR) & can_xlnx_read(dev, CAN_XLNX_IER);

	if ((isr & CAN_XLNX_IXR_BSOFF) != 0U) {
		can_xlnx_handle_bus_off(dev);
		return;
	}

	if ((isr & CAN_XLNX_IXR_RXNEMP) != 0U) {
		can_xlnx_handle_rx(dev);
	}

	if ((isr & CAN_XLNX_IXR_RXOFLW) != 0U) {
		CAN_STATS_RX_OVERRUN_INC(dev);
		can_xlnx_write(dev, CAN_XLNX_ICR, CAN_XLNX_IXR_RXOFLW | CAN_XLNX_IXR_RXUFLW);
	}

	if ((isr & CAN_XLNX_IXR_ERROR) != 0U) {
#ifdef CONFIG_CAN_STATS
		can_xlnx_update_error_stats(dev);
#else
		can_xlnx_write(dev, CAN_XLNX_ESR, CAN_XLNX_ESR_ALL);
#endif
		can_xlnx_write(dev, CAN_XLNX_ICR, CAN_XLNX_IXR_ERROR);
	}

	if ((isr & CAN_XLNX_IXR_TXOK) != 0U) {
		can_xlnx_write(dev, CAN_XLNX_ICR, CAN_XLNX_IXR_TXOK);
		can_xlnx_tx_done(dev, 0);
	}

	/* There is no interrupt for error state changes; re-evaluate on every interrupt */
	if (data->common.started) {
		can_xlnx_set_state(dev, can_xlnx_hw_state(dev));
	}
}

static int can_xlnx_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK;

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return 0;
}

static int can_xlnx_start(const struct device *dev)
{
	const struct can_xlnx_config *config = dev->config;
	struct can_xlnx_data *data = dev->data;
	unsigned int key;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->common.started) {
		err = -EALREADY;
		goto unlock;
	}

	if (config->common.phy != NULL) {
		err = can_transceiver_enable(config->common.phy, data->common.mode);
		if (err != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", err);
			goto unlock;
		}
	}

	CAN_STATS_RESET(dev);

	err = can_xlnx_hw_start(dev);
	if (err != 0) {
		if (config->common.phy != NULL) {
			(void)can_transceiver_disable(config->common.phy);
		}
		goto unlock;
	}

	key = irq_lock();
	data->state = CAN_STATE_ERROR_ACTIVE;
	atomic_set(&data->recovering, 0);
	data->common.started = true;
	can_xlnx_irq_enable(dev);
	irq_unlock(key);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int can_xlnx_stop(const struct device *dev)
{
	const struct can_xlnx_config *config = dev->config;
	struct can_xlnx_data *data = dev->data;
	unsigned int key;
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->common.started) {
		err = -EALREADY;
		goto unlock;
	}

	key = irq_lock();
	can_xlnx_write(dev, CAN_XLNX_IER, 0U);
	data->common.started = false;
	irq_unlock(key);

	/* With interrupts disabled in IER, the bus-off handler can no longer reschedule this */
	(void)k_work_cancel_delayable(&data->recovery_work);
	atomic_set(&data->recovering, 0);

	/* The reset aborts the current transmission and flushes both FIFOs */
	(void)can_xlnx_reset(dev);

	if (config->common.phy != NULL) {
		err = can_transceiver_disable(config->common.phy);
		if (err != 0) {
			LOG_ERR("failed to disable CAN transceiver (err %d)", err);
		}
	}

	can_xlnx_tx_done(dev, -ENETDOWN);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int can_xlnx_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_xlnx_data *data = dev->data;
	can_mode_t supported = CAN_MODE_LOOPBACK;

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		supported |= CAN_MODE_MANUAL_RECOVERY;
	}

	if ((mode & ~supported) != 0U) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if (data->common.started) {
		return -EBUSY;
	}

	data->common.mode = mode;

	return 0;
}

static int can_xlnx_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_xlnx_data *data = dev->data;

	if (data->common.started) {
		return -EBUSY;
	}

	data->timing = *timing;

	return 0;
}

static int can_xlnx_send(const struct device *dev, const struct can_frame *frame,
			 k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_xlnx_data *data = dev->data;
	uint32_t id;
	unsigned int key;

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("TX frame DLC %u exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0U) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if (data->state == CAN_STATE_BUS_OFF) {
		return -ENETUNREACH;
	}

	if (k_sem_take(&data->tx_idle, timeout) != 0) {
		return -EAGAIN;
	}

	key = irq_lock();

	/* The controller may have been stopped or gone bus-off while waiting */
	if (!data->common.started || data->state == CAN_STATE_BUS_OFF) {
		irq_unlock(key);
		k_sem_give(&data->tx_idle);
		return data->common.started ? -ENETUNREACH : -ENETDOWN;
	}

	if ((frame->flags & CAN_FRAME_IDE) != 0U) {
		id = FIELD_PREP(CAN_XLNX_ID_IDH, frame->id >> 18) | CAN_XLNX_ID_SRRRTR |
		     CAN_XLNX_ID_IDE | FIELD_PREP(CAN_XLNX_ID_IDL, frame->id);
		if ((frame->flags & CAN_FRAME_RTR) != 0U) {
			id |= CAN_XLNX_ID_RTR;
		}
	} else {
		id = FIELD_PREP(CAN_XLNX_ID_IDH, frame->id);
		if ((frame->flags & CAN_FRAME_RTR) != 0U) {
			id |= CAN_XLNX_ID_SRRRTR;
		}
	}

	data->tx_callback = callback;
	data->tx_user_data = user_data;

	can_xlnx_write(dev, CAN_XLNX_TXFIFO_ID, id);
	can_xlnx_write(dev, CAN_XLNX_TXFIFO_DLC, FIELD_PREP(CAN_XLNX_DLC_DLC, frame->dlc));
	can_xlnx_write(dev, CAN_XLNX_TXFIFO_DW1, sys_get_be32(&frame->data[0]));
	/* Writing DW2 commits the frame to the TX FIFO */
	can_xlnx_write(dev, CAN_XLNX_TXFIFO_DW2, sys_get_be32(&frame->data[4]));

	irq_unlock(key);

	return 0;
}

static int can_xlnx_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				  void *user_data, const struct can_filter *filter)
{
	struct can_xlnx_data *data = dev->data;
	unsigned int key;
	int filter_id = -ENOSPC;

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0U) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	for (int i = 0; i < ARRAY_SIZE(data->filters); i++) {
		if (!atomic_test_and_set_bit(data->rx_allocs, i)) {
			filter_id = i;
			break;
		}
	}

	if (filter_id >= 0) {
		/* The RX interrupt checks the allocation bit before the filter contents */
		key = irq_lock();
		data->filters[filter_id].filter = *filter;
		data->filters[filter_id].user_data = user_data;
		data->filters[filter_id].callback = callback;
		irq_unlock(key);
	}

	return filter_id;
}

static void can_xlnx_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_xlnx_data *data = dev->data;
	unsigned int key;

	if (filter_id < 0 || filter_id >= ARRAY_SIZE(data->filters)) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	key = irq_lock();
	if (atomic_test_and_clear_bit(data->rx_allocs, filter_id)) {
		data->filters[filter_id].callback = NULL;
		data->filters[filter_id].user_data = NULL;
		data->filters[filter_id].filter = (struct can_filter){0};
	}
	irq_unlock(key);
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int can_xlnx_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_xlnx_data *data = dev->data;
	k_timeout_t delay;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
	}

	if (atomic_get(&data->recovering) == 0) {
		return 0;
	}

	/* Keep the transmitter off for at least the bus-off recovery sequence */
	delay = can_xlnx_recovery_delay(dev);
	if (!K_TIMEOUT_EQ(timeout, K_FOREVER) && timeout.ticks < delay.ticks) {
		return -EAGAIN;
	}

	k_sleep(delay);

	return can_xlnx_restart(dev);
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static int can_xlnx_get_state(const struct device *dev, enum can_state *state,
			      struct can_bus_err_cnt *err_cnt)
{
	struct can_xlnx_data *data = dev->data;

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else if (atomic_get(&data->recovering) != 0) {
			*state = CAN_STATE_BUS_OFF;
		} else {
			*state = can_xlnx_hw_state(dev);
		}
	}

	if (err_cnt != NULL) {
		can_xlnx_get_err_cnt(dev, err_cnt);
	}

	return 0;
}

static int can_xlnx_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_xlnx_config *config = dev->config;

	*rate = config->clock_frequency;

	return 0;
}

static int can_xlnx_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_XLNX_MAX_FILTERS;
}

static int can_xlnx_init(const struct device *dev)
{
	const struct can_xlnx_config *config = dev->config;
	struct can_xlnx_data *data = dev->data;
	struct can_timing timing = {0};
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);

	data->dev = dev;
	sys_slist_init(&data->common.state_change_callbacks);
	k_mutex_init(&data->lock);
	k_sem_init(&data->tx_idle, 1, 1);
	k_work_init_delayable(&data->recovery_work, can_xlnx_recovery_work_handler);
	data->state = CAN_STATE_ERROR_ACTIVE;
	data->common.mode = CAN_MODE_NORMAL;

	if (config->common.phy != NULL && !device_is_ready(config->common.phy)) {
		LOG_ERR("CAN transceiver not ready");
		return -ENODEV;
	}

	/* No pinctrl state when the pins are muxed by the FSBL or routed through EMIO */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0 && err != -ENOENT) {
		return err;
	}

	err = can_xlnx_reset(dev);
	if (err != 0) {
		LOG_ERR("controller does not respond: is its clock enabled?");
		return err;
	}

	err = can_calc_timing(dev, &timing, config->common.bitrate, config->common.sample_point);
	if (err == -EINVAL) {
		LOG_ERR("bitrate/sample point cannot be met (err %d)", err);
		return err;
	}

	LOG_DBG("initial sample point error: %d", err);

	err = can_set_timing(dev, &timing);
	if (err != 0) {
		return err;
	}

	config->irq_config_func();

	return 0;
}

static DEVICE_API(can, can_xlnx_driver_api) = {
	.get_capabilities = can_xlnx_get_capabilities,
	.start = can_xlnx_start,
	.stop = can_xlnx_stop,
	.set_mode = can_xlnx_set_mode,
	.set_timing = can_xlnx_set_timing,
	.send = can_xlnx_send,
	.add_rx_filter = can_xlnx_add_rx_filter,
	.remove_rx_filter = can_xlnx_remove_rx_filter,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_xlnx_recover,
#endif
	.get_state = can_xlnx_get_state,
	.get_core_clock = can_xlnx_get_core_clock,
	.get_max_filters = can_xlnx_get_max_filters,
	/* clang-format off */
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_max = {
		.sjw = 4,
		.prop_seg = 0,
		.phase_seg1 = 16,
		.phase_seg2 = 8,
		.prescaler = 256,
	},
	/* clang-format on */
};

#define CAN_XLNX_INIT(inst)                                                                        \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static void can_xlnx_irq_config_##inst(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), can_xlnx_isr,         \
			    DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, flags));                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static const struct can_xlnx_config can_xlnx_config_##inst = {                             \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 1000000),                         \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(inst)),                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.irq_config_func = can_xlnx_irq_config_##inst,                                     \
		.clock_frequency = DT_INST_PROP(inst, clock_frequency),                            \
	};                                                                                         \
                                                                                                   \
	static struct can_xlnx_data can_xlnx_data_##inst;                                          \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_xlnx_init, NULL, &can_xlnx_data_##inst,                \
				  &can_xlnx_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,  \
				  &can_xlnx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_XLNX_INIT)
