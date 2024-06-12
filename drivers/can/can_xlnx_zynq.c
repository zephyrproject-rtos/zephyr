/*
 * Xilinx Processor System CAN controller driver
 *
 * Copyright (c) 2024, Immo Birnbaum
 * SPDX-License-Identifier: Apache-2.0
 *
 * Functional description: comp. Zynq-7000 SoC Technical Reference Manual (TRM),
 * Xilinx document ID UG585, rev. 1.13, chapter 18.
 */

#define DT_DRV_COMPAT xlnx_zynq_can_1_0

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include "can_xlnx_zynq.h"

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_xlnx_zynq, CONFIG_CAN_LOG_LEVEL);

#define DEV_CFG(_dev) ((const struct can_xlnx_zynq_dev_cfg *)(_dev)->config)
#define DEV_DATA(_dev) ((struct can_xlnx_zynq_dev_data *const)(_dev)->data)

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int can_xlnx_zynq_config_mode(const struct device *dev)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	uint32_t sr;
	uint16_t retries = 0;

	sr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_SR_OFFSET);
	if (!(sr & CAN_XLNX_ZYNQ_SR_CONFIG_MODE)) {
		/* Disable all interrupts before entering config mode */
		sys_write32(0x0, dev_data->base + CAN_XLNX_ZYNQ_IER_OFFSET);

		/*
		 * Clear SRR[CEN]. This also clears all non-FIFO related
		 * interrupt status bits.
		 */
		sys_write32(0x0, dev_data->base + CAN_XLNX_ZYNQ_SRR_OFFSET);

		/* Wait for SR[CONFIG] = 1 */
		do {
			sr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_SR_OFFSET);
		} while (!(sr & CAN_XLNX_ZYNQ_SR_CONFIG_MODE) &&
			 ++retries < CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES);

		if (retries == CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES) {
			LOG_ERR("%s set configuration mode timed out", dev->name);
			return -EIO;
		}

		LOG_DBG("%s is now in configuration mode", dev->name);
	}

	return 0;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static void can_xlnx_zynq_update_error_counters(const struct device *dev)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	uint32_t ecr;
	uint8_t rx_errors;
	uint8_t tx_errors;

	ecr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_ECR_OFFSET);
	rx_errors = (uint8_t)((ecr >> CAN_XLNX_ZYNQ_ECR_RX_ERRORS_OFFSET) &
		    CAN_XLNX_ZYNQ_ECR_RX_ERRORS_MASK);
	tx_errors = (uint8_t)((ecr >> CAN_XLNX_ZYNQ_ECR_TX_ERRORS_OFFSET) &
		    CAN_XLNX_ZYNQ_ECR_TX_ERRORS_MASK);

	if (rx_errors != dev_data->rx_errors || tx_errors != dev_data->tx_errors) {
		LOG_DBG("%s ECR RX %u TX %u", dev->name, dev_data->rx_errors,
			dev_data->tx_errors);
	}

	dev_data->rx_errors = rx_errors;
	dev_data->tx_errors = tx_errors;
}

static void can_xlnx_zynq_update_state(const struct device *dev)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	struct can_bus_err_cnt error_counters;
	enum can_state state = dev_data->state;
	enum can_state state_left = dev_data->state;
	bool warning = false;
	uint32_t sr = 0x0;
	uint32_t ier;

	can_xlnx_zynq_update_error_counters(dev);
	if (dev_data->common.started) {
		/*
		 * State is not 'stopped': extract the current error state from the
		 * Status Register: read SR[ESTAT]
		 */
		sr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_SR_OFFSET);

		if (sr & CAN_XLNX_ZYNQ_SR_ERROR_WARNING) {
			warning = true;
		}
		sr = (sr >> CAN_XLNX_ZYNQ_SR_ERROR_STATUS_OFFSET) &
		     CAN_XLNX_ZYNQ_SR_ERROR_STATUS_MASK;

		LOG_DBG("%s current status as per SR 0x%08X: %s",
			dev->name, sr,
			(sr == CAN_XLNX_ZYNQ_SR_ESTAT_CONFIG_MODE) ? "config mode" :
			(sr == CAN_XLNX_ZYNQ_SR_ESTAT_ERR_ACTIVE && !warning) ? "error active" :
			(sr == CAN_XLNX_ZYNQ_SR_ESTAT_ERR_ACTIVE && warning) ? "error warning" :
			(sr == CAN_XLNX_ZYNQ_SR_ESTAT_ERR_PASSIVE) ? "error passive" :
			(sr == CAN_XLNX_ZYNQ_SR_ESTAT_BUS_OFF) ? "bus-off" : "invalid");
	} else {
		LOG_DBG("%s current bus status is stopped", dev->name);
	}

	if (!dev_data->common.started) {
		state = CAN_STATE_STOPPED;
	} else if (sr == CAN_XLNX_ZYNQ_SR_ESTAT_ERR_ACTIVE && !warning) {
		state = CAN_STATE_ERROR_ACTIVE;
	} else if (sr == CAN_XLNX_ZYNQ_SR_ESTAT_ERR_ACTIVE && warning) {
		state = CAN_STATE_ERROR_WARNING;
	} else if (sr == CAN_XLNX_ZYNQ_SR_ESTAT_ERR_PASSIVE) {
		state = CAN_STATE_ERROR_PASSIVE;
	} else if (sr == CAN_XLNX_ZYNQ_SR_ESTAT_BUS_OFF) {
		state = CAN_STATE_BUS_OFF;
	}

	if (state != dev_data->state) {
		LOG_DBG("%s transitioning into bus state: %s", dev->name,
			(state == CAN_STATE_STOPPED) ? "stopped" :
			(state == CAN_STATE_ERROR_ACTIVE) ? "error active" :
			(state == CAN_STATE_ERROR_PASSIVE) ? "error passive" :
			(state == CAN_STATE_ERROR_WARNING) ? "error warning" :
			"bus-off");

		dev_data->state = state;

		/* Execute state change callback if registered */
		if (dev_data->common.state_change_cb != NULL) {
			error_counters.rx_err_cnt = dev_data->rx_errors;
			error_counters.tx_err_cnt = dev_data->tx_errors;
			dev_data->common.state_change_cb(dev, state, error_counters,
				dev_data->common.state_change_cb_user_data);
		}

		if (state == CAN_STATE_BUS_OFF) {
			/*
			 * When transitioning INTO bus-off state: disable the bus-off
			 * interrupt -> will be re-enabled once any other state is reached
			 * out of bus-off.
			 */
			ier = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_IER_OFFSET);
			ier &= ~CAN_XLNX_ZYNQ_IRQ_BUS_OFF;
			sys_write32(ier, dev_data->base + CAN_XLNX_ZYNQ_IER_OFFSET);
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
		} else if (state_left == CAN_STATE_BUS_OFF) {
			/*
			 * When transitioning OUT OF bus-off state with manual recovery:
			 * re-enable all interrupts. They were disabled by entering config
			 * mode. With auto-recovery, the interrupt enable bits are not
			 * affected.
			 */
			ier = CAN_XLNX_ZYNQ_IRQ_BUS_OFF | CAN_XLNX_ZYNQ_IRQ_MESSAGE_ERROR |
			CAN_XLNX_ZYNQ_IRQ_MESSAGE_RX | CAN_XLNX_ZYNQ_IRQ_MESSAGE_TX;
			sys_write32(ier, dev_data->base + CAN_XLNX_ZYNQ_IER_OFFSET);
#endif
		} else if (state_left == CAN_STATE_STOPPED) {
			/*
			 * When transitioning OUT OF stopped state: enable all interrupts.
			 * -> all interrupts were disabled by the controller reset in
			 * can_xlnx_zynq_stop().
			 */
			ier = CAN_XLNX_ZYNQ_IRQ_BUS_OFF | CAN_XLNX_ZYNQ_IRQ_MESSAGE_ERROR |
			CAN_XLNX_ZYNQ_IRQ_MESSAGE_RX | CAN_XLNX_ZYNQ_IRQ_MESSAGE_TX;
			sys_write32(ier, dev_data->base + CAN_XLNX_ZYNQ_IER_OFFSET);
		}
	}
}

static int can_xlnx_zynq_apply_mode(const struct device *dev)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	can_mode_t filtered_mode;
	uint32_t msr = 0x0;
	uint32_t sr;
	uint16_t retries = 0;
	bool target_mode_reached = false;

	/* Strip bits like manual recovery mode, not relevant here */
	filtered_mode = dev_data->common.mode & (CAN_MODE_NORMAL |
						 CAN_MODE_LISTENONLY |
						 CAN_MODE_LOOPBACK);

	switch (filtered_mode) {
	case CAN_MODE_NORMAL:
	default:
		/* No explicit indication of normal mode */
		/* Mode value validity checked in can_xlnx_zynq_set_mode() -> use as default */
		break;
	case CAN_MODE_LISTENONLY:
		msr |= CAN_XLNX_ZYNQ_MSR_SNOOP;
		break;
	case CAN_MODE_LOOPBACK:
		msr |= CAN_XLNX_ZYNQ_MSR_LOOPBACK;
		break;
	};

	sys_write32(msr, dev_data->base + CAN_XLNX_ZYNQ_MSR_OFFSET);
	sys_write32(CAN_XLNX_ZYNQ_SRR_CAN_ENABLE, dev_data->base + CAN_XLNX_ZYNQ_SRR_OFFSET);

	do {
		sr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_SR_OFFSET);

		switch (filtered_mode) {
		case CAN_MODE_NORMAL:
		default:
			target_mode_reached = (sr & CAN_XLNX_ZYNQ_SR_NORMAL_MODE)
					      ? true : false;
			break;
		case CAN_MODE_LISTENONLY:
			target_mode_reached = (sr & CAN_XLNX_ZYNQ_SR_SNOOP_MODE)
					      ? true : false;
			break;
		case CAN_MODE_LOOPBACK:
			target_mode_reached = (sr & CAN_XLNX_ZYNQ_SR_LOOPBACK_MODE)
					      ? true : false;
			break;
		};
	} while (!target_mode_reached && ++retries < CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES);

	if (retries == CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES) {
		LOG_ERR("%s transition to mode %s timed out", dev->name,
			(filtered_mode == CAN_MODE_NORMAL) ? "normal" :
			(filtered_mode == CAN_MODE_LISTENONLY) ? "listen only" :
			"loopback");
		return -EIO;
	}

	LOG_DBG("%s is now in %s mode", dev->name,
		(filtered_mode == CAN_MODE_NORMAL) ? "normal" :
		(filtered_mode == CAN_MODE_LISTENONLY) ? "listen only" :
		"loopback");

	return 0;
}

static void can_xlnx_zynq_handle_errors(const struct device *dev)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	uint32_t esr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_ESR_OFFSET);

	LOG_DBG("%s error(s): %s%s%s%s%s\n",
		dev->name,
		(esr & CAN_XLNX_ZYNQ_ESR_ACK_ERROR)   ? "ACK " : "",
		(esr & CAN_XLNX_ZYNQ_ESR_BIT_ERROR)   ? "BIT " : "",
		(esr & CAN_XLNX_ZYNQ_ESR_STUFF_ERROR) ? "STF " : "",
		(esr & CAN_XLNX_ZYNQ_ESR_FORM_ERROR)  ? "FRM " : "",
		(esr & CAN_XLNX_ZYNQ_ESR_CRC_ERROR)   ? "CRC " : "");

	if (esr & CAN_XLNX_ZYNQ_ESR_ACK_ERROR) {
		CAN_STATS_ACK_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ZYNQ_ESR_BIT_ERROR) {
		CAN_STATS_BIT_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ZYNQ_ESR_STUFF_ERROR) {
		CAN_STATS_STUFF_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ZYNQ_ESR_FORM_ERROR) {
		CAN_STATS_FORM_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ZYNQ_ESR_CRC_ERROR) {
		CAN_STATS_CRC_ERROR_INC(dev);
	}

	sys_write32(esr, dev_data->base + CAN_XLNX_ZYNQ_ESR_OFFSET);
	sys_write32(CAN_XLNX_ZYNQ_IRQ_MESSAGE_ERROR, dev_data->base + CAN_XLNX_ZYNQ_ICR_OFFSET);
}

static void can_xlnx_zynq_handle_rx(const struct device *dev)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	struct can_frame rx_frame = {0};
	can_rx_callback_t callback = NULL;
	uint32_t isr;
	uint32_t idr;
	uint32_t dlcr;
	uint32_t dw1;
	uint32_t dw2;
	uint32_t i;

	/*
	 * Extract messages from the RX FIFO until it indicates that there are
	 * no more pending messages left (indicated by ISR[RXNEMP] = 0). If any
	 * read operation causes an underflow error (ISR[RXUFLW]) in the RX FIFO,
	 * break out of the reception handling loop.
	 */
	do {
		idr  = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_RXFIFO_ID_OFFSET);
		dlcr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_RXFIFO_DLC_OFFSET);
		dw1  = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_RXFIFO_DATA1_OFFSET);
		dw2  = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_RXFIFO_DATA2_OFFSET);

		/*
		 * Clear the FIFO not empty flag before processing the current message
		 * -> will be re-asserted if more messages are pending in the RX FIFO.
		 */
		sys_write32(CAN_XLNX_ZYNQ_IRQ_RX_NOT_EMPTY,
			    dev_data->base + CAN_XLNX_ZYNQ_ICR_OFFSET);

		rx_frame.dlc = (dlcr >> CAN_XLNX_ZYNQ_FIFO_DLCR_DLC_OFFSET) &
				CAN_XLNX_ZYNQ_FIFO_DLCR_DLC_MASK;
#ifdef CONFIG_CAN_RX_TIMESTAMP
		rx_frame.timestamp = (dlcr & CAN_XLNX_ZYNQ_FIFO_DLCR_RXT_MASK);
#endif

		if (idr & CAN_XLNX_ZYNQ_FIFO_IDR_IDE) {
			rx_frame.flags |= CAN_FRAME_IDE;
			rx_frame.id  = (idr >> CAN_XLNX_ZYNQ_FIFO_IDR_IDL_OFFSET) &
					CAN_XLNX_ZYNQ_FIFO_IDR_IDL_MASK;
			rx_frame.id |= ((idr >> CAN_XLNX_ZYNQ_FIFO_IDR_IDH_OFFSET) &
					CAN_XLNX_ZYNQ_FIFO_IDR_IDH_MASK) << 18;

			/* RTR -> acquire from [RTR] */
			if (idr & CAN_XLNX_ZYNQ_FIFO_IDR_RTR) {
				rx_frame.flags |= CAN_FRAME_RTR;
			}
		} else {
			rx_frame.id = (idr >> CAN_XLNX_ZYNQ_FIFO_IDR_IDH_OFFSET) &
				CAN_XLNX_ZYNQ_FIFO_IDR_IDH_MASK;

			/* RTR -> acquire from [SRRRTR] */
			if (idr & CAN_XLNX_ZYNQ_FIFO_IDR_SRRRTR) {
				rx_frame.flags |= CAN_FRAME_RTR;
			}
		}

		rx_frame.data_32[0] = sys_cpu_to_be32(dw1);
		rx_frame.data_32[1] = sys_cpu_to_be32(dw2);

		LOG_DBG("%s RX ID %03X DLC %u %08X|%08X", dev->name, rx_frame.id,
			rx_frame.dlc, rx_frame.data_32[0], rx_frame.data_32[1]);

		/* Perform filter check. Process message if any filter matches. */
#ifndef CONFIG_CAN_ACCEPT_RTR
		if ((rx_frame.flags & CAN_FRAME_RTR) == 0U) {
#endif
			for (i = 0; i < ARRAY_SIZE(dev_data->rx_filters); i++) {
				if (!atomic_test_bit(dev_data->rx_filters_allocated, i)) {
					continue;
				}

				if (can_frame_matches_filter(&rx_frame,
							     &dev_data->rx_filters[i].filter)) {
					callback = dev_data->rx_filters[i].callback;
					if (callback != NULL) {
						callback(dev, &rx_frame,
							 dev_data->rx_filters[i].user_data);
					}
					break;
				}
			}
#ifndef CONFIG_CAN_ACCEPT_RTR
		}
#endif

		isr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_ISR_OFFSET);
		if (isr & CAN_XLNX_ZYNQ_IRQ_RX_UNDERFLOW) {
			LOG_ERR("%s read from RX FIFO caused an underflow error",
				dev->name);
			break;
		}
	} while (isr & CAN_XLNX_ZYNQ_IRQ_RX_NOT_EMPTY);

	sys_write32((CAN_XLNX_ZYNQ_IRQ_MESSAGE_RX | CAN_XLNX_ZYNQ_IRQ_RX_UNDERFLOW),
		    dev_data->base + CAN_XLNX_ZYNQ_ICR_OFFSET);
}

static void can_xlnx_zynq_handle_tx(const struct device *dev, int status)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	can_tx_callback_t tx_callback = dev_data->tx_callback;
	void *tx_user_data = dev_data->tx_user_data;

	if (tx_callback != NULL) {
		dev_data->tx_callback = NULL;
		dev_data->tx_user_data = NULL;
		tx_callback(dev, status, tx_user_data);
	} else {
		k_sem_give(&dev_data->tx_done_sem);
	}

	sys_write32(CAN_XLNX_ZYNQ_IRQ_MESSAGE_TX, dev_data->base + CAN_XLNX_ZYNQ_ICR_OFFSET);

	/* Allow the next call of can_xlnx_zynq_send() */
	k_sem_give(&dev_data->tx_lock_sem);
}

int can_xlnx_zynq_set_timing(const struct device *dev,
			     const struct can_timing *timing)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	uint32_t brpr;
	uint32_t btr;

	if (dev_data->common.started) {
		return -EBUSY;
	}

	/* Requires config mode -> already entered once device is in stopped state */
	brpr = ((timing->prescaler - 1) & CAN_XLNX_ZYNQ_BRPR_PRESCALER_MASK);
	btr  = ((timing->sjw - 1) & CAN_XLNX_ZYNQ_BTR_SJW_MASK) <<
		CAN_XLNX_ZYNQ_BTR_SJW_OFFSET;
	btr |= ((timing->phase_seg2 - 1) & CAN_XLNX_ZYNQ_BTR_TS2_MASK) <<
		CAN_XLNX_ZYNQ_BTR_TS2_OFFSET;
	btr |= ((timing->phase_seg1 + timing->prop_seg - 1) & CAN_XLNX_ZYNQ_BTR_TS1_MASK) <<
		CAN_XLNX_ZYNQ_BTR_TS1_OFFSET;

	LOG_DBG("%s set timing: PS %u SJW %u PS2 %u PS1 %u", dev->name,
		timing->prescaler, timing->sjw, timing->phase_seg2, timing->phase_seg1);

	sys_write32(brpr, dev_data->base + CAN_XLNX_ZYNQ_BRPR_OFFSET);
	sys_write32(btr, dev_data->base + CAN_XLNX_ZYNQ_BTR_OFFSET);

	dev_data->timing = *timing;

	return 0;
}

int can_xlnx_zynq_start(const struct device *dev)
{
	const struct can_xlnx_zynq_dev_cfg *dev_conf = DEV_CFG(dev);
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	int ret;

	if (dev_data->common.started) {
		LOG_DBG("%s already started", dev->name);
		return -EALREADY;
	}

	LOG_DBG("%s starting", dev->name);

	if (dev_conf->common.phy != NULL) {
		ret = can_transceiver_enable(dev_conf->common.phy, dev_data->common.mode);
		if (ret < 0) {
			LOG_ERR("%s failed to enable transceiver (%d)", dev->name, ret);
			return ret;
		}
	}

	ret = can_xlnx_zynq_set_timing(dev, &dev_data->timing);
	(void)ret; /* only faults out if common.started set -> already checked */

	ret = can_xlnx_zynq_apply_mode(dev);
	if (ret < 0) {
		LOG_ERR("%s failed to apply operational mode while starting up", dev->name);
		return ret;
	}

	CAN_STATS_RESET(dev);

	dev_data->common.started = true;
	can_xlnx_zynq_update_state(dev);

	__ASSERT(dev_data->state != CAN_STATE_STOPPED,
		 "unexpected state: still in stopped after state handling "
		 "in %s", __func__);

	LOG_DBG("%s started", dev->name);
	return 0;
}

int can_xlnx_zynq_stop(const struct device *dev)
{
	const struct can_xlnx_zynq_dev_cfg *dev_conf = DEV_CFG(dev);
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	can_tx_callback_t tx_callback = dev_data->tx_callback;
	void *tx_user_data = dev_data->tx_user_data;
	uint32_t sr;
	int ret = 0;
	uint16_t retries = 0;

	if (!dev_data->common.started) {
		LOG_DBG("%s already stopped", dev->name);
		return -EALREADY;
	}

	LOG_DBG("%s stopping", dev->name);

	if (dev_conf->common.phy != NULL) {
		ret = can_transceiver_disable(dev_conf->common.phy);
		if (ret < 0) {
			LOG_ERR("%s failed to disable transceiver (%d)", dev->name, ret);
			return ret;
		}
	}

	/*
	 * Software-reset the controller - aborts pending TX, flushes the FIFOs,
	 * disables all interrupts, clears error counters, enters config mode.
	 * Just entering config mode will not affect the current contents of the
	 * FIFOs!
	 */
	sys_write32(CAN_XLNX_ZYNQ_SRR_SOFTWARE_RESET, dev_data->base + CAN_XLNX_ZYNQ_SRR_OFFSET);
	do {
		sr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_SR_OFFSET);
	} while (!(sr & CAN_XLNX_ZYNQ_SR_CONFIG_MODE) &&
		 ++retries < CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES);
	if (retries == CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES) {
		LOG_ERR("%s controller reset while stopping timed out", dev->name);
		ret = -EIO;
	}

	dev_data->common.started = false;
	can_xlnx_zynq_update_state(dev);

	__ASSERT(dev_data->state == CAN_STATE_STOPPED,
		 "unexpected state: not stopped after state handling "
		 "in %s", __func__);

	/* Unblock any operation using the driver's semaphores or TX callback */
	if (tx_callback != NULL) {
		dev_data->tx_callback = NULL;
		dev_data->tx_user_data = NULL;
		tx_callback(dev, -ENETDOWN, tx_user_data);
	} else {
		k_sem_give(&dev_data->tx_done_sem);
	}

	k_sem_give(&dev_data->tx_lock_sem);

	if (ret == 0) {
		LOG_DBG("%s stopped", dev->name);
	} else {
		LOG_ERR("%s stop command failed, controller reset timed out", dev->name);
	}

	return ret;
}

int can_xlnx_zynq_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = (CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY);
	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return 0;
}

int can_xlnx_zynq_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);

	if (dev_data->common.started) {
		return -EBUSY;
	}

	if (mode & CAN_MODE_FD) {
		LOG_ERR("%s CAN FD is unsupported by this controller", dev->name);
		return -ENOTSUP;
	}

	if ((mode & (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) ==
	    (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) {
		LOG_ERR("%s cannot combine loopback and listen only mode", dev->name);
		return -ENOTSUP;
	}

	dev_data->common.mode = mode;

	return 0;
}

int can_xlnx_zynq_send(const struct device *dev,
		       const struct can_frame *frame,
		       k_timeout_t timeout,
		       can_tx_callback_t callback,
		       void *user_data)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	uint32_t sr;
	uint32_t isr;
	uint32_t idr;
	uint32_t dlcr;
	uint32_t dw1;
	uint32_t dw2;
	int64_t lock_sem_ticks_pre;
	int64_t lock_sem_ticks_diff;

	__ASSERT(frame != NULL, "%s *frame is NULL for %s", dev->name, __func__);

	if (!dev_data->common.started) {
		LOG_DBG("%s cannot send CAN frame: controller is not started",
			dev->name);
		return -ENETDOWN;
	}

	if (dev_data->state == CAN_STATE_BUS_OFF) {
		LOG_DBG("%s cannot send CAN frame: controller is in bus-off state",
			dev->name);
		return -ENETUNREACH;
	}

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("%s cannot send CAN frame with DLC %u: exceeds "
			"maximum valid value %u", dev->name, frame->dlc,
			CAN_MAX_DLC);
		return -EINVAL;
	}

	if (frame->flags & CAN_FRAME_FDF) {
		LOG_ERR("%s CAN FD is unsupported by this controller", dev->name);
		return -ENOTSUP;
	}

	/* Check if the current operational mode is suitable for transmission */
	sr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_SR_OFFSET);
	if (sr & (CAN_XLNX_ZYNQ_SR_CONFIG_MODE | CAN_XLNX_ZYNQ_SR_SNOOP_MODE)) {
		LOG_ERR("%s cannot send CAN frame: unable to transmit in "
			"%s mode", dev->name, (sr & CAN_XLNX_ZYNQ_SR_CONFIG_MODE) ?
			"config" : "snoop");
		return -EIO;
	}

	/* Cannot send more frames if the TX FIFO is full */
	isr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_ISR_OFFSET);
	if (isr & CAN_XLNX_ZYNQ_IRQ_TX_FULL) {
		LOG_ERR("%s cannot send CAN frame: TX FIFO full", dev->name);
		return -ENOSPC;
	}

	/* Assemble TX FIFO register contents from frame data */

	/* Standard identifier is always present in the IDR register */
	idr = (frame->id & CAN_XLNX_ZYNQ_FIFO_IDR_IDH_MASK) << CAN_XLNX_ZYNQ_FIFO_IDR_IDH_OFFSET;
	/* Extended identifier optional, RTR indication varies depends on std/ext identifier */
	if (frame->flags & CAN_FRAME_IDE) {
		idr  = CAN_XLNX_ZYNQ_FIFO_IDR_IDE;
		idr |= (frame->id & CAN_XLNX_ZYNQ_FIFO_IDR_IDL_MASK) <<
			CAN_XLNX_ZYNQ_FIFO_IDR_IDL_OFFSET;
		idr |= ((frame->id >> 18) & CAN_XLNX_ZYNQ_FIFO_IDR_IDH_MASK) <<
			CAN_XLNX_ZYNQ_FIFO_IDR_IDH_OFFSET;

		/* [SRRRTR]=1 in extended ID frames, actual RTR bit is [RTR] */
		idr |= CAN_XLNX_ZYNQ_FIFO_IDR_SRRRTR;
		if (frame->flags & CAN_FRAME_RTR) {
			idr |= CAN_XLNX_ZYNQ_FIFO_IDR_RTR;
		}
	} else {
		idr = (frame->id & CAN_XLNX_ZYNQ_FIFO_IDR_IDH_MASK) <<
		      CAN_XLNX_ZYNQ_FIFO_IDR_IDH_OFFSET;

		/* [RTR]=0 in standard ID only frames, actual RTR bit is [SRRRTR] */
		if (frame->flags & CAN_FRAME_RTR) {
			idr |= CAN_XLNX_ZYNQ_FIFO_IDR_SRRRTR;
		}
	}

	dlcr = (frame->dlc & CAN_XLNX_ZYNQ_FIFO_DLCR_DLC_MASK) <<
	       CAN_XLNX_ZYNQ_FIFO_DLCR_DLC_OFFSET;

	dw1 = sys_cpu_to_be32(frame->data_32[0]);
	dw2 = sys_cpu_to_be32(frame->data_32[1]);

	/*
	 * Assure that only a single TX operation takes place at any given time,
	 * as the hardware TX FIFO doesn't support priority-based (re-)ordering.
	 * Apply overall TX timeout given by the caller to the TX lock semaphore
	 * take call -> subtract elapsed ticks from the caller's timeout value,
	 * apply the remainder for the tx_done_sem take call if applicable.
	 */
	lock_sem_ticks_pre = k_uptime_ticks();
	if (k_sem_take(&dev_data->tx_lock_sem, timeout) != 0) {
		LOG_ERR("%s cannot send CAN frame: single TX lock semaphore "
			"timed out", dev->name);
		if (!dev_data->common.started) {
			/* TX lock was released by can_xlnx_zynq_stop() */
			return -ENETDOWN;
		}
		/* TX lock acquisition just timed out */
		return -EAGAIN;
	}
	lock_sem_ticks_diff = (k_uptime_ticks() - lock_sem_ticks_pre);

	if (timeout.ticks != K_TICKS_FOREVER && timeout.ticks != 0 &&
	    (timeout.ticks - (k_ticks_t)lock_sem_ticks_diff) <= timeout.ticks) {
		timeout.ticks -= (k_ticks_t)lock_sem_ticks_diff;
	}

	/* Prepare TX done handling - either via callback or the TX done semaphore */
	if (callback != NULL) {
		dev_data->tx_callback = callback;
		dev_data->tx_user_data = user_data;
	} else {
		dev_data->tx_callback = NULL;
		dev_data->tx_user_data = NULL;
		k_sem_reset(&dev_data->tx_done_sem);
	}

	/* Write the 4 data words into the TX FIFO */
	sys_write32(idr, dev_data->base + CAN_XLNX_ZYNQ_TXFIFO_ID_OFFSET);
	sys_write32(dlcr, dev_data->base + CAN_XLNX_ZYNQ_TXFIFO_DLC_OFFSET);
	sys_write32(dw1, dev_data->base + CAN_XLNX_ZYNQ_TXFIFO_DATA1_OFFSET);
	sys_write32(dw2, dev_data->base + CAN_XLNX_ZYNQ_TXFIFO_DATA2_OFFSET);

	/*
	 * Either wait for TX completion if no callback was provided by the caller,
	 * or just exit if no callback was provided. It will be called from within
	 * the ISR once transmission is complete.
	 */
	if (callback == NULL) {
		return k_sem_take(&dev_data->tx_done_sem, timeout) == 0 ? 0 : -EAGAIN;
	}

	return 0;
}

static int can_xlnx_zynq_add_rx_filter(const struct device *dev,
				       can_rx_callback_t callback,
				       void *user_data,
				       const struct can_filter *filter)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	int filter_id = -ENOSPC;
	int i;

	__ASSERT(callback != NULL, "%s NULL callback function pointer provided for RX filter",
		 dev->name);
	__ASSERT(filter != NULL, "%s NULL callback data pointer provided for RX filter",
		 dev->name);

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("%s unsupported bits in RX filter flags (%02X)", dev->name,
			filter->flags);
		return -ENOTSUP;
	}

	for (i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		if (!atomic_test_and_set_bit(dev_data->rx_filters_allocated, i)) {
			filter_id = i;
			break;
		}
	}

	if (filter_id >= 0) {
		dev_data->rx_filters[filter_id].filter = *filter;
		dev_data->rx_filters[filter_id].callback = callback;
		dev_data->rx_filters[filter_id].user_data = user_data;
	} else {
		LOG_ERR("%s cannot add any more RX filters - all %u filters in use",
			dev->name, CONFIG_CAN_MAX_FILTER);
	}

	return filter_id;
}

static void can_xlnx_zynq_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);

	if (filter_id < 0 || filter_id >= CONFIG_CAN_MAX_FILTER) {
		LOG_ERR("%s cannot remove RX filter at index [%u] - out of bounds, "
			"highest valid index is [%u]", dev->name, filter_id,
			(CONFIG_CAN_MAX_FILTER - 1));
		return;
	}

	if (atomic_test_and_clear_bit(dev_data->rx_filters_allocated, filter_id)) {
		dev_data->rx_filters[filter_id].filter = (struct can_filter){0};
		dev_data->rx_filters[filter_id].callback = NULL;
		dev_data->rx_filters[filter_id].user_data = NULL;
	}
}

int can_xlnx_zynq_get_state(const struct device *dev,
			    enum can_state *state,
			    struct can_bus_err_cnt *err_cnt)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);

	if (state != NULL) {
		*state = dev_data->state;
	}

	if (err_cnt != NULL) {
		err_cnt->rx_err_cnt = dev_data->rx_errors;
		err_cnt->tx_err_cnt = dev_data->tx_errors;
	}

	return 0;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
int can_xlnx_zynq_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	int64_t recovery_start_ticks = k_uptime_ticks();
	int ret = 0;

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	if ((dev_data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0) {
		return -ENOTSUP;
	}

	if (dev_data->state != CAN_STATE_BUS_OFF) {
		return 0;
	}

	/* Restore the regular operational mode we expect the controller to be in */
	do {
		ret = can_xlnx_zynq_apply_mode(dev);
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    (k_uptime_ticks() - recovery_start_ticks > timeout.ticks)) {
			LOG_ERR("%s recovery from bus-off state timed out "
				"(target mode not reached)", dev->name);
			return -EAGAIN;
		}
	} while (ret != 0);

	/*
	 * Poll until the state changes from bus-off to anything else,
	 * preferrably error active. If that times out, revert to config
	 * mode.
	 */
	while (dev_data->state == CAN_STATE_BUS_OFF) {
		can_xlnx_zynq_update_state(dev);
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    (k_uptime_ticks() - recovery_start_ticks > timeout.ticks)) {
			ret = can_xlnx_zynq_config_mode(dev);
			LOG_ERR("%s recovery from bus-off state timed out "
				"(target state not reached)", dev->name);
			return -EAGAIN;
		}
	}

	return 0;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

void can_xlnx_zynq_set_state_change_callback(const struct device *dev,
					     can_state_change_callback_t callback,
					     void *user_data)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);

	dev_data->common.state_change_cb = callback;
	dev_data->common.state_change_cb_user_data = user_data;
}

int can_xlnx_zynq_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_xlnx_zynq_dev_cfg *dev_conf = DEV_CFG(dev);

	*rate = dev_conf->clock_frequency;
	return 0;
}

int can_xlnx_zynq_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
}

void can_xlnx_zynq_isr(const struct device *dev)
{
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);
	uint32_t isr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_ISR_OFFSET);

	if (isr & CAN_XLNX_ZYNQ_IRQ_MESSAGE_TX) {
		can_xlnx_zynq_handle_tx(dev, 0);
	}
	if (isr & CAN_XLNX_ZYNQ_IRQ_MESSAGE_RX) {
		can_xlnx_zynq_handle_rx(dev);
	}
	if (isr & CAN_XLNX_ZYNQ_IRQ_MESSAGE_ERROR) {
		can_xlnx_zynq_handle_errors(dev);
	}
	if (isr & CAN_XLNX_ZYNQ_IRQ_BUS_OFF) {
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
		LOG_ERR("%s bus-off, manual recovery required", dev->name);

		/*
		 * Automatic bus-off auto-recovery is disabled:
		 * -> update state now, as entering config mode clears
		 * the error status bits. State will transition to 'bus-off'.
		 * -> enter configuration mode, disable further interrupts
		 * (config mode preserves RX/TX FIFO contents)
		 * -> restoring an operational state requires explicit
		 * transition to normal/snoop/loopback mode via the
		 * recovery function (or stopping the device, which resets
		 * the controller).
		 * -> 'bus-off' to 'error active' state transition will
		 * take place once normal/snoop/loopback mode is entered
		 */
		can_xlnx_zynq_update_state(dev);

		if (can_xlnx_zynq_config_mode(dev) < 0) {
			LOG_ERR("%s set config state for manual bus-off recovery "
				"failed", dev->name);
		}
#endif
		sys_write32(CAN_XLNX_ZYNQ_IRQ_BUS_OFF,
			    dev_data->base + CAN_XLNX_ZYNQ_ICR_OFFSET);
	}

	can_xlnx_zynq_update_state(dev);
}

int can_xlnx_zynq_init(const struct device *dev)
{
	const struct can_xlnx_zynq_dev_cfg *dev_conf = DEV_CFG(dev);
	struct can_xlnx_zynq_dev_data *dev_data = DEV_DATA(dev);

	uint32_t sr;
	int ret;
	uint16_t retries = 0;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	dev_data->base = DEVICE_MMIO_NAMED_GET(dev, reg_base);
	__ASSERT(dev_data->base != 0, "%s map register space failed", dev->name);
	if (dev_data->base == 0) {
		LOG_ERR("%s map device memory failed", dev->name);
		return -EIO;
	}

#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(dev_conf->pin_config, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
#endif

	k_sem_init(&dev_data->tx_done_sem, 0, 1);
	k_sem_init(&dev_data->tx_lock_sem, 1, 1);

	/* Calculate initial timing config */
	ret = can_calc_timing(dev, &dev_data->timing, dev_conf->common.bus_speed,
			      dev_conf->common.sample_point);
	if (ret < 0) {
		LOG_ERR("%s calculate timing failed (%d)", dev->name, ret);
		return ret;
	}

	/* Software-reset the controller - clears all registers */
	sys_write32(CAN_XLNX_ZYNQ_SRR_SOFTWARE_RESET, dev_data->base + CAN_XLNX_ZYNQ_SRR_OFFSET);
	/* Filtering is all software-based -> AFR was set to 0 during reset */

	/* Wait for configuration mode: can.SR[CONFIG] must be 1 */
	do {
		sr = sys_read32(dev_data->base + CAN_XLNX_ZYNQ_SR_OFFSET);
	} while (!(sr & CAN_XLNX_ZYNQ_SR_CONFIG_MODE) &&
		 ++retries < CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES);

	if (retries == CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES) {
		LOG_ERR("%s config mode after controller reset timed out", dev->name);
		return -EIO;
	}

	dev_conf->irq_config_func(dev);

	return 0;
}

/* Driver API */
static const struct can_driver_api can_xlnx_zynq_driver_api = {
	.start = can_xlnx_zynq_start,
	.stop = can_xlnx_zynq_stop,
	.get_capabilities = can_xlnx_zynq_get_capabilities,
	.set_mode = can_xlnx_zynq_set_mode,
	.set_timing = can_xlnx_zynq_set_timing,
	.send = can_xlnx_zynq_send,
	.add_rx_filter = can_xlnx_zynq_add_rx_filter,
	.remove_rx_filter = can_xlnx_zynq_remove_rx_filter,
	.get_state = can_xlnx_zynq_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_xlnx_zynq_recover,
#endif
	.set_state_change_callback = can_xlnx_zynq_set_state_change_callback,
	.get_core_clock = can_xlnx_zynq_get_core_clock,
	.get_max_filters = can_xlnx_zynq_get_max_filters,
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = CAN_XLNX_ZYNQ_BRPR_MIN_PRESCALER
	},
	.timing_max = {
		.sjw = 4,
		.prop_seg = 1,
		.phase_seg1 = 15,
		.phase_seg2 = 8,
		.prescaler = CAN_XLNX_ZYNQ_BRPR_MAX_PRESCALER
	}
};

/* Register & initialize all CAN controllers specified in the device tree. */
DT_INST_FOREACH_STATUS_OKAY(CAN_XLNX_ZYNQ_DEV_INITITALIZE);
