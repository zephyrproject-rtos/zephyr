/*
 * Xilinx Processor System CAN controller driver
 *
 * Copyright (c) 2022, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * Known limitations / TODOs:
 * - Sleep mode / PM functionality not considered at the time being.
 * - High Priority TX buffer not considered at the time being.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/drivers/can.h>
#include <zephyr/net/net_ip.h>
#include "can_xlnx.h"
#include "can_utils.h"

#define LOG_MODULE_NAME can_xlnx
#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#define DT_DRV_COMPAT xlnx_zynq_can_1_0

/* Functions used internally by the driver */

/**
 * @brief Configuration mode transition function
 * @internal
 *
 * Transition the Xilinx CAN controller to configuration mode.
 * Comp. Zynq-7000 TRM, chap. 18.3.2 for further information
 * which parameters can (exclusively) be configured in this mode
 * and which registers are automatically being cleared along the
 * way.
 *
 * @param dev Pointer to the CAN controller's device.
 */
static void can_xlnx_config_mode(const struct device *dev)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	struct can_xlnx_dev_data *dev_data = dev->data;
	uint32_t sr;

	sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
	if (!(sr & CAN_XLNX_SR_CONFIG_MODE)) {
		/* Disable all interrupts before entering config mode */
		sys_write32(0x0, base_addr + CAN_XLNX_IER_OFFSET);

		/*
		 * Clear SRR[CEN]. This also clears all non-FIFO related
		 * interrupt status bits.
		 */
		sys_write32(0x0, base_addr + CAN_XLNX_SRR_OFFSET);

		/* Wait for SR[CONFIG] */
		do {
			sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
		} while (!(sr & CAN_XLNX_SR_CONFIG_MODE));

		/*
		 * Entering config mode clears the error counters
		 * -> clear the stored error counters.
		 */
		dev_data->rx_errors = 0;
		dev_data->tx_errors = 0;

		LOG_DBG("%s is now in configuration mode", dev->name);
	}
}

/**
 * @brief Bus state transition function
 * @internal
 *
 * Performs a bus state transition, if new state != current state.
 * This function is called from the ISR for handling of the bus off
 * state, can_xlnx_handle_errors for error active / error warning /
 * error passive state handling based on the error counters, and
 * can_xlnx_set_mode whenever an operational mode other than con-
 * figuration mode is established. If a state change callback has
 * been registered with the current driver instance, it will be
 * executed.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param state Potentially new bus state detected in one of the
 *              contexts described above.
 */
static void can_xlnx_state_update(const struct device *dev,
				  enum can_state state)
{
	struct can_xlnx_dev_data *dev_data = dev->data;
	struct can_bus_err_cnt error_counters;

	if (state != dev_data->state) {
		dev_data->state = state;
		LOG_DBG("%s is now in bus state %s", dev->name,
			(state == CAN_STATE_ERROR_ACTIVE) ? "CAN_STATE_ERROR_ACTIVE" :
			(state == CAN_STATE_ERROR_PASSIVE) ? "CAN_STATE_ERROR_PASSIVE" :
			(state == CAN_STATE_ERROR_WARNING) ? "CAN_STATE_ERROR_WARNING" :
			"CAN_STATE_BUS_OFF");

		/*
		 * When transitioning to bus off state:
		 * -> Set separate marker in the driver instance's data as this
		 * state can not be expressed in RX/TX error counter values. We
		 * know that bus off is the new state as this function is only
		 * ever called with this new state based on the evaluation of
		 * the corresponding bit in the Interrupt Status Register.
		 * While the bus off state could also be obtained from SR[ESTAT]
		 * = 0x2 (comp. Zynq-7000 TRM, register SR description, Appendix
		 * B.5, p. 804), a local variable in the device data is probably
		 * preferrable compared to reading a register every time the bus
		 * state is queried.
		 * -> Clear the local error counters, as this happens auto-
		 * matically in hardware when entering the bus off state.
		 */
		if (state != CAN_STATE_BUS_OFF) {
			dev_data->bus_off = 0;
		} else {
			dev_data->bus_off = 1;
			dev_data->rx_errors = 0;
			dev_data->tx_errors = 0;
		}

		/* Execute state change callback if registered */
		if (dev_data->state_chg_cb != NULL) {
			LOG_DBG("%s calling state change callback function",
				dev->name);

			error_counters.rx_err_cnt = dev_data->rx_errors;
			error_counters.tx_err_cnt = dev_data->tx_errors;
			dev_data->state_chg_cb(dev, state, error_counters,
					       dev_data->state_chg_user_data);
		}
	}
}

/**
 * @brief Bus error handling function
 * @internal
 *
 * Called from within the ISR if the error interrupt bit is set in
 * the interrupt status register, this function evaluates the Error
 * Status Register (ESR) and translates the register's contents into
 * detailed error counters. Subsequently, the Error Counter Register
 * (ECR) is read and the RX error or TX error counter (whichever is
 * greater) is used to determine an updated CAN bus state.
 *
 * @param dev Pointer to the CAN controller's device.
 */
static void can_xlnx_handle_errors(const struct device *dev)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	struct can_xlnx_dev_data *dev_data = dev->data;
	uint32_t esr;
	uint32_t ecr;
	uint8_t higher_errors;

	/* Decode the type of error -> update error statistics */
	esr = sys_read32(base_addr + CAN_XLNX_ESR_OFFSET);
	LOG_ERR("%s error(s): %s%s%s%s%s\n",
		dev->name,
		(esr & CAN_XLNX_ESR_ACK_ERROR)   ? "ACK " : "",
		(esr & CAN_XLNX_ESR_BIT_ERROR)   ? "BIT " : "",
		(esr & CAN_XLNX_ESR_STUFF_ERROR) ? "STF " : "",
		(esr & CAN_XLNX_ESR_FORM_ERROR)  ? "FRM " : "",
		(esr & CAN_XLNX_ESR_CRC_ERROR)   ? "CRC " : "");

	if (esr & CAN_XLNX_ESR_ACK_ERROR) {
		CAN_STATS_ACK_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ESR_BIT_ERROR) {
		CAN_STATS_BIT0_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ESR_STUFF_ERROR) {
		CAN_STATS_STUFF_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ESR_FORM_ERROR) {
		CAN_STATS_FORM_ERROR_INC(dev);
	}
	if (esr & CAN_XLNX_ESR_CRC_ERROR) {
		CAN_STATS_CRC_ERROR_INC(dev);
	}

	/* Clear the error status register */
	sys_write32(CAN_XLNX_ESR_CLEAR_ALL_MASK, base_addr + CAN_XLNX_ESR_OFFSET);

	/* Update the error counters */
	ecr = sys_read32(base_addr + CAN_XLNX_ECR_OFFSET);
	dev_data->rx_errors = (uint8_t)((ecr >> CAN_XLNX_ECR_RX_ERRORS_OFFSET) &
		CAN_XLNX_ECR_RX_ERRORS_MASK);
	dev_data->tx_errors = (uint8_t)((ecr >> CAN_XLNX_ECR_TX_ERRORS_OFFSET) &
		CAN_XLNX_ECR_TX_ERRORS_MASK);

	/*
	 * Check if a state change is required due to the new error counter values.
	 * Bus off state can not be detected via the error counters, entering this
	 * error state is indicated by a separate interrupt flag.
	 *
	 * NOTICE: this could alternatively be determined by evaluating the ESTAT
	 * bits in the Status Register (SR). Comp. Zynq-7000 TRM, register SR de-
	 * scription, Appendix B.5, p. 804), but we need the counters updated any-
	 * ways as we need up-to-date values whenever the counters are queried
	 * via can_xlnx_get_state.
	 */
	higher_errors = (dev_data->rx_errors > dev_data->tx_errors) ?
			dev_data->rx_errors : dev_data->tx_errors;
	if (higher_errors < 96) {
		can_xlnx_state_update(dev, CAN_STATE_ERROR_ACTIVE);
	} else if (higher_errors >= 96 && higher_errors < 128) {
		can_xlnx_state_update(dev, CAN_STATE_ERROR_WARNING);
	} else if (higher_errors >= 128 && higher_errors <= 255) {
		can_xlnx_state_update(dev, CAN_STATE_ERROR_PASSIVE);
	}

	/* Clear the error interrupt status bit */
	sys_write32(CAN_XLNX_IRQ_MESSAGE_ERROR, base_addr + CAN_XLNX_ICR_OFFSET);
}

/**
 * @brief RX FIFO message acquisition function
 * @internal
 *
 * Called from within the ISR if the RX OK interrupt bit is set in
 * the Interrupt Status Register, this function reads CAN messages
 * from the RX FIFO until it is indicated via the Interrupt Status
 * Register (ISR) that there are no further messages stored in the
 * RX FIFO. Subsequent processing of each received message requires
 * that a compatible RX filter has been configured, otherwise, the
 * respective message is discarded.
 *
 * @param dev Pointer to the CAN controller's device.
 */
static void can_xlnx_handle_rx(const struct device *dev)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	struct can_xlnx_dev_data *dev_data = dev->data;
	struct can_frame rx_frame = {0};
	uint32_t isr;
	uint32_t idr;
	uint32_t dlcr;
	uint32_t dw1;
	uint32_t dw2;
	uint32_t i;

	/*
	 * Clear the RX FIFO underflow interrupt status bit. The RX FIFO
	 * should not be empty as the RX OK interrupt led to the execution
	 * of this function. If the subsequent read operation causes an
	 * underflow, that's actually an error as it contradicts the RX OK
	 * indication. Also, clear the RX OK bit so that any further incoming
	 * messages can raise this flag again.
	 */
	sys_write32((CAN_XLNX_IRQ_RX_UNDERFLOW | CAN_XLNX_IRQ_MESSAGE_RX),
		    base_addr + CAN_XLNX_ICR_OFFSET);

	/*
	 * Extract messages from the RX FIFO until it indicates that there are
	 * no more pending messages left (indicated by ISR[RXNEMP] = 0). If any
	 * read operation causes an underflow error (ISR[RXUFLW]) in the RX FIFO,
	 * break out of the reception handling loop.
	 */
	do {
		/* Read the raw data from the RX FIFO */
		idr  = sys_read32(base_addr + CAN_XLNX_RXFIFO_ID_OFFSET);
		dlcr = sys_read32(base_addr + CAN_XLNX_RXFIFO_DLC_OFFSET);
		dw1  = sys_read32(base_addr + CAN_XLNX_RXFIFO_DATA1_OFFSET);
		dw2  = sys_read32(base_addr + CAN_XLNX_RXFIFO_DATA2_OFFSET);

		/*
		 * Clear the FIFO not empty flag before reading the next message
		 * -> will be re-asserted if more messages are pending in the RX FIFO.
		 */
		sys_write32(CAN_XLNX_IRQ_RX_NOT_EMPTY, base_addr + CAN_XLNX_ICR_OFFSET);

		/* Start with the simplest item: DLC */
		rx_frame.dlc = (dlcr >> CAN_XLNX_FIFO_DLCR_DLC_OFFSET) &
				CAN_XLNX_FIFO_DLCR_DLC_MASK;
		/* RX timestamp if feature is enabled */
#if defined(CONFIG_CAN_RX_TIMESTAMP)
		rx_frame.timestamp = (dlcr & CAN_XLNX_FIFO_DLCR_RXT_MASK);
#endif

		/*
		 * Extract ID based on standard / extended ID indication.
		 * RTR indication depends on standard / extended format.
		 */
		if (idr & CAN_XLNX_FIFO_IDR_IDE) {
			rx_frame.id_type = CAN_EXTENDED_IDENTIFIER;

			/* Extended ID in IDH and IDL */
			rx_frame.id  = (idr >> CAN_XLNX_FIFO_IDR_IDL_OFFSET) &
				CAN_XLNX_FIFO_IDR_IDL_MASK;
			rx_frame.id |= ((idr >> CAN_XLNX_FIFO_IDR_IDH_OFFSET) &
					CAN_XLNX_FIFO_IDR_IDH_MASK) << 18;

			/* RTR -> acquire from [RTR] */
			if (idr & CAN_XLNX_FIFO_IDR_RTR) {
				rx_frame.rtr = CAN_REMOTEREQUEST;
			} else {
				rx_frame.rtr = CAN_DATAFRAME;
			}
		} else {
			rx_frame.id_type = CAN_STANDARD_IDENTIFIER;

			/* Standard ID in IDH only */
			rx_frame.id  = (idr >> CAN_XLNX_FIFO_IDR_IDH_OFFSET) &
				CAN_XLNX_FIFO_IDR_IDH_MASK;

			/* RTR -> acquire from [SRRRTR] */
			if (idr & CAN_XLNX_FIFO_IDR_SRRRTR) {
				rx_frame.rtr = CAN_REMOTEREQUEST;
			} else {
				rx_frame.rtr = CAN_DATAFRAME;
			}
		}

		/* Frame payload */
		rx_frame.data_32[0] = ntohl(dw1);
		rx_frame.data_32[1] = ntohl(dw2);

		/*
		 * The frame has been read from the RX FIFO unconditionally,
		 * now check if it matches and registered RX filter.
		 */
		if (dev_data->filter_usage_mask) {
			for (i = 0; i < CAN_XLNX_NUM_ACCEPTANCE_FILTERS; i++) {
				/*
				 * Skip the current filter entry if:
				 * - it is unused (corresp. bit not set in filter_usage_mask)
				 * - the filter's ID/mask don't match the RX frame's ID.
				 */
				if (!(dev_data->filter_usage_mask & BIT(i))) {
					continue;
				}

				if (!can_utils_filter_match(&rx_frame,
							    &dev_data->rx_filters[i].filter)) {
					continue;
				}

				dev_data->rx_filters[i].cb(dev, &rx_frame,
							   dev_data->rx_filters[i].user_data);
			}
		} else {
			LOG_DBG("%s discarded received frame with ID 0x%X as no "
				"RX filters are registered", dev->name, rx_frame.id);
		}

		/* Get updated interrupt status register */
		isr = sys_read32(base_addr + CAN_XLNX_ISR_OFFSET);

		/* Check for RX FIFO undeflow error */
		if (isr & CAN_XLNX_IRQ_RX_UNDERFLOW) {
			sys_write32(CAN_XLNX_IRQ_RX_UNDERFLOW,
				    base_addr + CAN_XLNX_ICR_OFFSET);
			LOG_ERR("%s read from RX FIFO caused an undeflow error",
				dev->name);
			break;
		}
	} while (isr & CAN_XLNX_IRQ_RX_NOT_EMPTY);
}

/**
 * @brief TX FIFO transmission done handler function
 * @internal
 *
 * Called from within the ISR if the TX OK interrupt bit is set in
 * the Interrupt Status Register, this function executes each sent
 * message's transmission done handler. This can either be the com-
 * bination of a callback function pointer plus its parameter pointer,
 * or a pointer to a semaphore on whose acquisition the can_xlnx_send
 * function is currently blocking.
 *
 * @param dev Pointer to the CAN controller's device.
 */
static void can_xlnx_handle_tx(const struct device *dev)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	struct can_xlnx_dev_data *dev_data = dev->data;
	struct can_xlnx_tx_done_handler *handler;

	/* Clear the TX OK interrupt bit */
	sys_write32(CAN_XLNX_IRQ_MESSAGE_TX, base_addr + CAN_XLNX_ICR_OFFSET);

	/* Process all pending TX done handlers */
	while (dev_data->tx_done_rd_idx != dev_data->tx_done_wr_idx) {
		handler = &dev_data->tx_done_handlers[dev_data->tx_done_rd_idx];
		dev_data->tx_done_rd_idx = (dev_data->tx_done_rd_idx + 1) %
					   CAN_XLNX_RX_TX_FIFO_DEPTH;

		if (handler->cb != NULL) {
			handler->cb(dev, 0, handler->user_data);
		} else if (handler->sem != NULL) {
			k_sem_give(handler->sem);
		}

		handler->cb = NULL;
		handler->user_data = NULL;
		handler->sem = NULL;
	}
}

/* Functions accessible via the driver's API */

/**
 * @brief Sets the current operational mode of the Xilinx CAN controller
 *
 * Sets the current controller instance's operational mode based on the
 * mode parameter provided by the caller. The parameter's content is
 * translated to the matching values to be written into the controller's
 * Mode Select Register (MSR). Before the CAN bus state is being reset
 * and interrupts are being re-enabled, it is assured that the specified
 * operational mode has actually been reached.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param mode Supported modes: CAN_MODE_NORMAL, CAN_MODE_LISTENONLY,
 *             CAN_MODE_LOOPBACK. Unsupported: silent loopback mode
 *             (CAN_MODE_LISTENONLY | CAN_MODE_LOOPBACK) and CAN_MODE_FD.
 *
 * @retval -ENOTSUP in case of invalid mode value, 0 otherwise.
 */
int can_xlnx_set_mode(const struct device *dev, can_mode_t mode)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	struct can_xlnx_dev_data *dev_data = dev->data;
#endif
	uint32_t msr = 0x0;
	uint32_t sr;
	uint32_t ier;
	uint8_t target_mode_reached = 0;

	/*
	 * Documentation of operational modes and transitions between
	 * them: comp. Zynq-7000 TRM, chap. 18.2.1.
	 */

#ifdef CONFIG_CAN_FD_MODE
	if (mode & CAN_MODE_FD) {
		LOG_ERR("%s CAN FD is unsupported", dev->name);
		return -ENOTSUP;
	}
#endif

	if ((mode & (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) ==
	    (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) {
		LOG_ERR("%s cannot combine loopback and listen only mode",
			dev->name);
		return -ENOTSUP;
	}

	/* Configure operational mode in the Mode Select Register */
	switch (mode) {
	case CAN_MODE_NORMAL:
		/* No explicit indication of normal mode */
		break;
	case CAN_MODE_LISTENONLY:
		msr |= CAN_XLNX_MSR_SNOOP;
		break;
	case CAN_MODE_LOOPBACK:
		msr |= CAN_XLNX_MSR_LOOPBACK;
		break;
	default:
		/* CAN_SILENT_LOOPBACK_MODE: unsupported by the controller */
		return -EINVAL;
	};

	/* Config mode is required in order to change operational mode */
	can_xlnx_config_mode(dev);

	/* Write MSR and set SRR[CEN] */
	sys_write32(msr, base_addr + CAN_XLNX_MSR_OFFSET);
	sys_write32(CAN_XLNX_SRR_CAN_ENABLE, base_addr + CAN_XLNX_SRR_OFFSET);

	/* Make sure that we actually reach the requested mode */
	do {
		sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);

		switch (mode) {
		case CAN_MODE_NORMAL:
			target_mode_reached = (sr & CAN_XLNX_SR_NORMAL_MODE) ? 1 : 0;
			break;
		case CAN_MODE_LISTENONLY:
			target_mode_reached = (sr & CAN_XLNX_SR_SNOOP_MODE) ? 1 : 0;
			break;
		case CAN_MODE_LOOPBACK:
			target_mode_reached = (sr & CAN_XLNX_SR_LOOPBACK_MODE) ? 1 : 0;
			break;
		default:
			break;
		};
	} while (!target_mode_reached);

	LOG_DBG("%s is now in %s mode", dev->name,
		(mode == CAN_MODE_NORMAL) ? "normal" :
		(mode == CAN_MODE_LISTENONLY) ? "listen only" :
		"loopback");

	/*
	 * As we have just come out of config mode, all error counters have been cleared,
	 * (updated locally within can_xlnx_config_mode) as has been any pending error-
	 * warning / error-passive / bus off state.
	 * -> Set the current state to CAN_STATE_ERROR_ACTIVE, which also executes the state
	 * change callback if one is registered.
	 */
	can_xlnx_state_update(dev, CAN_STATE_ERROR_ACTIVE);

	/*
	 * Enable interrupts -> IER was cleared when entering config mode.
	 * Enable error and bus off interrupts, as well as per-message RX OK
	 * and TX OK interrupts.
	 */
	ier = CAN_XLNX_IRQ_BUS_OFF | CAN_XLNX_IRQ_MESSAGE_ERROR |
	      CAN_XLNX_IRQ_MESSAGE_RX | CAN_XLNX_IRQ_MESSAGE_TX;
	sys_write32(ier, base_addr + CAN_XLNX_IER_OFFSET);

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	/*
	 * Unblock the recovery timeout semaphore once the target mode has been reached
	 * and auto bus off recovery is disabled.
	 */
	k_sem_give(&dev_data->recovery_sem);
#endif

	return 0;
}

/**
 * @brief Sets the timing of the Xilinx CAN controller
 *
 * Converts the timing parameters provided by the caller into matching
 * content for the Bit Timing Register (BTR) and the Baud Rate Prescaler
 * Register (BRPR).
 *
 * @param dev Pointer to the CAN controller's device.
 * @param timing Timing data to be used for the configuration of the
 *               current controller's bit timing behaviour and baud rate.
 * @param timing_data unused.
 *
 * @retval always 0.
 */
int can_xlnx_set_timing(const struct device *dev,
			const struct can_timing *timing)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	can_mode_t curr_mode = CAN_MODE_NORMAL;
	uint32_t brpr;
	uint32_t btr;
	uint32_t sr;

	sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
	if (!(sr & CAN_XLNX_SR_CONFIG_MODE)) {
		curr_mode = (sr & CAN_XLNX_SR_NORMAL_MODE) ? CAN_MODE_NORMAL :
			    (sr & CAN_XLNX_SR_SNOOP_MODE) ? CAN_MODE_LISTENONLY :
			    CAN_MODE_LOOPBACK;
		can_xlnx_config_mode(dev);
	}

	brpr = ((timing->prescaler - 1) & CAN_XLNX_BRPR_PRESCALER_MASK);
	btr  = ((timing->sjw - 1) & CAN_XLNX_BTR_SJW_MASK) <<
		CAN_XLNX_BTR_SJW_OFFSET;
	btr |= ((timing->phase_seg2 - 1) & CAN_XLNX_BTR_TS2_MASK) <<
		CAN_XLNX_BTR_TS2_OFFSET;
	btr |= ((timing->phase_seg1 + timing->prop_seg - 1) & CAN_XLNX_BTR_TS1_MASK) <<
		CAN_XLNX_BTR_TS1_OFFSET;

	sys_write32(brpr, base_addr + CAN_XLNX_BRPR_OFFSET);
	sys_write32(btr, base_addr + CAN_XLNX_BTR_OFFSET);

	if (!(sr & CAN_XLNX_SR_CONFIG_MODE)) {
		can_xlnx_set_mode(dev, curr_mode);
	}

	return 0;
}

/**
 * @brief Message send function
 *
 * Sends the CAN message provided by the caller. Notification of TX
 * completion can be performed via a callback function plus parameters
 * if such pointers are supplied by the caller. If this is the case,
 * this function will immediately return after the transmission has
 * been triggered, and the specified callback will be called from with-
 * in the interrupt context once the corresponding message has been
 * transmitted successfully. If the callback parameter is NULL, this
 * function will block until the transmission has been confirmed, which
 * is achieved by the interrupt context giving to a semaphore this
 * function is trying to take from without timeout.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param frame Pointer to the CAN message to be transmitted.
 * @param timeout Timeout for TX completion notification íf no callback
 *                function pointer is provided by the caller.
 * @param callback Callback function pointer, to be called once the
 *                 current message has been transmitted.
 * @param user_data Parameter data pointer to be supplied to the call-
 *                  back function.
 *
 * @retval -EINVAL in case of invalid parameters. -ENETUNREACH in case the
 *         controller's current bus state is bus off. -EIO if the con-
 *         troller's operational mode doesn't allow message transmission.
 *         -ENOSPC if the TX FIFO is currently full. -EAGAIN in case of
 *         TX confirmation timeout.
 */
int can_xlnx_send(const struct device *dev,
		  const struct can_frame *frame,
		  k_timeout_t timeout,
		  can_tx_callback_t callback,
		  void *user_data)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	struct can_xlnx_dev_data *dev_data = dev->data;
	struct can_xlnx_tx_done_handler done_handler;
	uint32_t sr;
	uint32_t isr;
	uint32_t ier;
	uint32_t idr;
	uint32_t dlcr;
	uint32_t dw1;
	uint32_t dw2;
	int sem_rc;

	__ASSERT(frame != NULL, "%s %s: frame pointer is NULL",
		 __func__, dev->name);

	/* CAN FD is unsupported */
	if (frame->fd != 0) {
		LOG_ERR("CAN FD is unsuppoted by the Xilinx CAN controller");
		return -EINVAL;
	}

	/* Maximum DLC value is 8. */
	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("Cannot send CAN frame via interface %s: DLC %u exceeds "
			"maximum valid value 8", dev->name, frame->dlc);
		return -EINVAL;
	}

	/* Cannot send if the controller is in bus off state */
	if (dev_data->bus_off == 1) {
		return -ENETUNREACH;
	}

	/* Check if the current operational mode is suitable for transmission */
	sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
	if (sr & (CAN_XLNX_SR_CONFIG_MODE | CAN_XLNX_SR_SNOOP_MODE)) {
		LOG_ERR("Cannot send CAN frame via interface %s: unable to transmit "
			"in %s mode", dev->name, (sr & CAN_XLNX_SR_CONFIG_MODE) ?
			"config" : "snoop");
		return -EIO;
	}

	/* Cannot send more frames if the TX FIFO is full */
	isr = sys_read32(base_addr + CAN_XLNX_ISR_OFFSET);
	if (isr & CAN_XLNX_IRQ_TX_FULL) {
		LOG_ERR("Cannot send CAN frame via interface %s: TX FIFO full", dev->name);
		return -ENOSPC;
	}

	/* Prepare TX done handling - either via callback or the TX done semaphore */
	if (callback != NULL) {
		done_handler.cb = callback;
		done_handler.user_data = user_data;
		done_handler.sem = NULL;
	} else {
		k_sem_reset(&dev_data->tx_done_sem);
		done_handler.cb = NULL;
		done_handler.user_data = NULL;
		done_handler.sem = &dev_data->tx_done_sem;
	}

	/* Prepare the contents of the registers required to write into the TX FIFO */

	/* Standard identifier is always present in the IDR register */
	idr = (frame->id & CAN_XLNX_FIFO_IDR_IDH_MASK) << CAN_XLNX_FIFO_IDR_IDH_OFFSET;
	/*
	 * Extended identifier is optional, RTR indication varies based on whether the
	 * extended identifier is present or not.
	 */
	if (frame->id_type == CAN_EXTENDED_IDENTIFIER) {
		idr  = CAN_XLNX_FIFO_IDR_IDE;
		idr |= (frame->id & CAN_XLNX_FIFO_IDR_IDL_MASK) <<
			CAN_XLNX_FIFO_IDR_IDL_OFFSET;
		idr |= ((frame->id >> 18) & CAN_XLNX_FIFO_IDR_IDH_MASK) <<
			CAN_XLNX_FIFO_IDR_IDH_OFFSET;

		/* [SRRRTR] bit =1 in extended ID frames, actual RTR bit is [RTR] */
		idr |= CAN_XLNX_FIFO_IDR_SRRRTR;
		if (frame->rtr) {
			idr |= CAN_XLNX_FIFO_IDR_RTR;
		}
	} else {
		idr = (frame->id & CAN_XLNX_FIFO_IDR_IDH_MASK) << CAN_XLNX_FIFO_IDR_IDH_OFFSET;

		/* [RTR] bit =0 in standard ID only frames, actual RTR bit is [SRRRTR] */
		if (frame->rtr) {
			idr |= CAN_XLNX_FIFO_IDR_SRRRTR;
		}
	}

	dlcr = (frame->dlc & CAN_XLNX_FIFO_DLCR_DLC_MASK) << CAN_XLNX_FIFO_DLCR_DLC_OFFSET;

	dw1 = htonl(frame->data_32[0]);
	dw2 = htonl(frame->data_32[1]);

	/*
	 * Disable the TX OK interrupt while the TX done handler for the current message
	 * is being set up and its data is being written to the TX FIFO. This assures that
	 * the done handler access semaphore is unlocked before the TX done interrupt is
	 * handled. Otherwise, there might be problems if the semaphore is still locked
	 * by can_xlnx_send which might be called recursively out of a callback function
	 * called from within the ISR context.
	 */
	ier = sys_read32(base_addr + CAN_XLNX_IER_OFFSET);
	sys_write32((ier & ~CAN_XLNX_IRQ_MESSAGE_TX), base_addr + CAN_XLNX_IER_OFFSET);

	/* Get the TX done handler semaphore -> non-blocking if in ISR context */
	sem_rc = k_sem_take(&dev_data->tx_done_prot_sem,
		 (k_is_in_isr()) ? K_NO_WAIT : K_FOREVER);
	if (sem_rc < 0) {
		return -EAGAIN;
	}

	/* Store the TX done handler & move the write pointer forwards */
	dev_data->tx_done_handlers[dev_data->tx_done_wr_idx] = done_handler;
	dev_data->tx_done_wr_idx = (dev_data->tx_done_wr_idx + 1) %
				   CAN_XLNX_RX_TX_FIFO_DEPTH;

	/* Write the 4 data words into the TX FIFO */
	sys_write32(idr,  base_addr + CAN_XLNX_TXFIFO_ID_OFFSET);
	sys_write32(dlcr, base_addr + CAN_XLNX_TXFIFO_DLC_OFFSET);
	sys_write32(dw1,  base_addr + CAN_XLNX_TXFIFO_DATA1_OFFSET);
	sys_write32(dw2,  base_addr + CAN_XLNX_TXFIFO_DATA2_OFFSET);

	/* Release the TX done handler semaphore */
	k_sem_give(&dev_data->tx_done_prot_sem);
	/* Re-enable the applicable TX done interrupt */
	sys_write32(ier, base_addr + CAN_XLNX_IER_OFFSET);

	/*
	 * Either wait for TX completion if no callback was provided by the caller,
	 * or just exit if no callback was provided. It will be called from within
	 * the ISR once transmission is complete.
	 */
	if (callback == NULL) {
		sem_rc = k_sem_take(&dev_data->tx_done_sem, timeout);
		return (sem_rc == 0) ? 0 : -EAGAIN;
	} else {
		return 0;
	}
}

/**
 * @brief Adds a message receive filter
 *
 * Adds a filter each received message is checked against, if the re-
 * ceived message matches the criteria specified in the filter, the
 * callback function associated with the filter will be called and the
 * received message will be handed over to it.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param cb Callback function pointer associated with the filter.
 * @param user_data Parameter pointer for the callback function.
 * @param filter Message acceptance criteria for the filter to be added.
 *
 * @retval -ENOSPC if all 4 filter slots supported by the Xilinx CAN
 *         controller are currently in use, the index of the slot the
 *         filter configuration was stored in ([0 .. 3]) otherwise.
 */
static int can_xlnx_add_rx_filter(const struct device *dev,
				  can_rx_callback_t cb,
				  void *user_data,
				  const struct can_filter *filter)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	struct can_xlnx_dev_data *dev_data = dev->data;
	struct can_xlnx_filter_data *rx_filter = NULL;
	uint32_t filter_id;
	uint32_t sr;
	uint32_t ier;
	uint32_t afmr;
	uint32_t afmr_offset;
	uint32_t afir;
	uint32_t afir_offset;

	__ASSERT(cb != NULL, "%s NULL callback function pointer provided to %s",
		 dev->name, __func__);
	__ASSERT(filter != NULL, "%s NULL filter data pointer provided to %s",
		 dev->name, __func__);

	/*
	 * Filters are modified only once a semaphore has been acquired.
	 * -> Simultaneous calls of this function in SMP might otherwise
	 * result in the same filter slot being identified as unused and
	 * being modified twice.
	 */
	k_sem_take(&dev_data->filter_prot_sem, K_FOREVER);

	/*
	 * Check if a filter entry is available, store data provided by
	 * the caller if a free filter entry can be allocated.
	 */

	for (filter_id = 0; filter_id < CAN_XLNX_NUM_ACCEPTANCE_FILTERS; filter_id++) {
		if (!(dev_data->filter_usage_mask & BIT(filter_id))) {
			rx_filter = &dev_data->rx_filters[filter_id];
			dev_data->filter_usage_mask |= BIT(filter_id);

			rx_filter->cb = cb;
			rx_filter->user_data = user_data;
			rx_filter->filter = *filter;

			afmr_offset = CAN_XLNX_AFMR1_OFFSET + (filter_id * 0x8);
			afir_offset = CAN_XLNX_AFIR1_OFFSET + (filter_id * 0x8);

			break;
		}
	}

	if (rx_filter == NULL) {
		k_sem_give(&dev_data->filter_prot_sem);
		LOG_WRN("%s cannot add RX filter: no free filter slot available",
			dev->name);
		return -ENOSPC;
	}

	/* Assemble AFMR, AFIR register contents */
	if ((enum can_ide)filter->id_type == CAN_EXTENDED_IDENTIFIER) {
		afmr  = CAN_XLNX_AFR_IDE;
		afmr |= (filter->id_mask & CAN_XLNX_AFR_EXT_ID_MASK) <<
			CAN_XLNX_AFR_EXT_ID_OFFSET;
		afmr |= ((filter->id_mask >> 18) & CAN_XLNX_AFR_STD_ID_MASK) <<
			CAN_XLNX_AFR_STD_ID_OFFSET;
		afmr |= (filter->rtr_mask) ? CAN_XLNX_AFR_RTR : 0;
	} else {
		afmr = (filter->id_mask & CAN_XLNX_AFR_STD_ID_MASK) <<
			CAN_XLNX_AFR_STD_ID_OFFSET;
		afmr |= (filter->rtr_mask) ? CAN_XLNX_AFR_SRR_RTR : 0;
	}

	if ((enum can_ide)filter->id_type == CAN_EXTENDED_IDENTIFIER) {
		afir  = CAN_XLNX_AFR_IDE;
		afir |= (filter->id & CAN_XLNX_AFR_EXT_ID_MASK) <<
			CAN_XLNX_AFR_EXT_ID_OFFSET;
		afir |= ((filter->id >> 18) & CAN_XLNX_AFR_STD_ID_MASK) <<
			CAN_XLNX_AFR_STD_ID_OFFSET;
		afir |= (filter->rtr) ? CAN_XLNX_AFR_RTR : 0;
	} else {
		afir = (filter->id & CAN_XLNX_AFR_STD_ID_MASK)
			<< CAN_XLNX_AFR_STD_ID_OFFSET;
		afir |= (filter->rtr) ? CAN_XLNX_AFR_SRR_RTR : 0;
	}

	/*
	 * Filter configuration is applicable, program the registers of the
	 * assigned hardware filter unit. Programming sequence: comp. Zynq-7000
	 * TRM, chap. 18.2.5.
	 */

	/* Disable interrupts while updating the filters */
	ier = sys_read32(base_addr + CAN_XLNX_IER_OFFSET);
	sys_write32(0x0, base_addr + CAN_XLNX_IER_OFFSET);

	/* Disable acceptance filters */
	sys_write32(0x0, base_addr + CAN_XLNX_AFR_OFFSET);

	/* Wait for filter to be idle */
	do {
		sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
	} while (sr & CAN_XLNX_SR_ACC_FLTR_BUSY);

	/* Write AFMR, AFIR registers */
	sys_write32(afmr, base_addr + afmr_offset);
	sys_write32(afir, base_addr + afir_offset);

	/* (Re-)enable filters, write bit mask to AFR */
	sys_write32(dev_data->filter_usage_mask, base_addr + CAN_XLNX_AFR_OFFSET);

	/* Filter state is now clean & consistent, release the semaphore */
	k_sem_give(&dev_data->filter_prot_sem);

	/* Re-enable interrupts */
	sys_write32(ier, base_addr + CAN_XLNX_IER_OFFSET);

	return filter_id;
}

/**
 * @brief Removes a message receive filter
 *
 * Remove a filter that was added by calling #can_xlnx_add_rx_filter().
 * The filter to be removed is addressed by the filter index that was
 * returned at the time the filter was added. This function doesn't return
 * anything, attempts to remove a filter from an unused slot are ignored
 * without error.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param filter_id Filter index ([0 .. 3]) to be removed.
 */
static void can_xlnx_remove_rx_filter(const struct device *dev, int filter_id)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	struct can_xlnx_dev_data *dev_data = dev->data;
	uint32_t afmr_offset;
	uint32_t afir_offset;
	uint32_t sr;
	uint32_t ier;

	/* As above in add_rx_filter: prevent concurrent modification */
	k_sem_take(&dev_data->filter_prot_sem, K_FOREVER);

	if (!(dev_data->filter_usage_mask & BIT(filter_id))) {
		/* This filter is not currently in use */
		k_sem_give(&dev_data->filter_prot_sem);
		return;
	}

	afmr_offset = CAN_XLNX_AFMR1_OFFSET + (filter_id * 0x8);
	afir_offset = CAN_XLNX_AFIR1_OFFSET + (filter_id * 0x8);

	/* Disable interrupts while updating the filters */
	ier = sys_read32(base_addr + CAN_XLNX_IER_OFFSET);
	sys_write32(0x0, base_addr + CAN_XLNX_IER_OFFSET);

	/* Disable acceptance filters */
	sys_write32(0x0, base_addr + CAN_XLNX_AFR_OFFSET);

	/* Wait for filter to be idle */
	do {
		sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
	} while (sr & CAN_XLNX_SR_ACC_FLTR_BUSY);

	/* Write AFMR, AFIR registers */
	sys_write32(0x0, base_addr + afmr_offset);
	sys_write32(0x0, base_addr + afir_offset);

	/* (Re-)enable filters, write bit mask to AFR */
	dev_data->filter_usage_mask &= ~BIT(filter_id);
	sys_write32(dev_data->filter_usage_mask, base_addr + CAN_XLNX_AFR_OFFSET);

	/* Clear data of the now disabled filter */
	dev_data->rx_filters[filter_id].cb = NULL;
	dev_data->rx_filters[filter_id].user_data = NULL;
	memset((void *)&dev_data->rx_filters[filter_id].filter,
	       0x00, sizeof(struct can_filter));

	/* Filter state is now clean & consistent, release the semaphore */
	k_sem_give(&dev_data->filter_prot_sem);

	/* Re-enable interrupts */
	sys_write32(ier, base_addr + CAN_XLNX_IER_OFFSET);
}

/**
 * @brief Returns the current CAN bus state
 *
 * Returns the current CAN bus state if the corresponding pointer
 * parameter is not NULL. Returns the current RX and TX error counter
 * values if the corresponding pointer parameter is not NULL.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param state If this parameter is not NULL, the current bus state
 *              will be written to the location pointed to.
 * @param err_cnt If this parameter is not NULL, the current RX and
 *                TX error counter tuple will be written to the lo-
 *                cation pointed to.
 *
 * @retval always 0.
 */
static int can_xlnx_get_state(const struct device *dev,
			      enum can_state *state,
			      struct can_bus_err_cnt *err_cnt)
{
	struct can_xlnx_dev_data *dev_data = dev->data;

	/* Return bus state if requested */
	if (state != NULL) {
		if (dev_data->bus_off == 0) {
			/* Return the state variable derived from the error counters */
			*state = dev_data->state;
		} else {
			/*
			 * Controller has indicated 'bus off' state.
			 * This state cannot be indicated via the error counters as
			 * this would require a range > 8 bits. 'Bus off' is indi-
			 * cated via an interrupt status bit instead and is picked
			 * up within the ISR.
			 */
			*state = CAN_STATE_BUS_OFF;
		}
	}

	/* Return current error counter values if requested */
	if (err_cnt != NULL) {
		err_cnt->rx_err_cnt = dev_data->rx_errors;
		err_cnt->tx_err_cnt = dev_data->tx_errors;
	}

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
/**
 * @brief Invokes manual recovery from bus off state
 *
 * Manually transitions the current CAN controller to the 'normal'
 * operational mode. By the time a bus off condition is detected and
 * auto-recovery is disabled, the controller will be switched over
 * to configuration mode, in which any persisting errors are dis-
 * carded. The explicit transition to 'normal' mode should make the
 * controller fully functional once again, assuming that the circum-
 * stances which led to the controller entering bus off state no longer
 * persist.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param timeout Maximum amount of time the transition back to
 *                normal mode may take in order to be considered
 *                successful.
 *
 * @retval -EAGAIN in case the transition is not completed successfully
 *         within the amount of time specified by the timeout parameter,
 *         0 if the transition to normal mode was successful.
 */
int can_xlnx_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_xlnx_dev_data *dev_data = dev->data;
	int sem_status;

	/* Just return if recovery is not required */
	if (!dev_data->bus_off) {
		return 0;
	}

	k_sem_reset(&dev_data->recovery_sem);
	can_xlnx_set_mode(dev, CAN_MODE_NORMAL);
	sem_status = k_sem_take(&dev_data->recovery_sem, timeout);

	return (sem_status == 0) ? 0 : -EAGAIN;
}
#endif /* !CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

/**
 * @brief Installs a state change callback for the current controller
 *
 * Installs a state change callback function for the current controller
 * instance which is called whenever a CAN bus state change is detected.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param cb State change callback function pointer.
 * @param user_data Parameter data for the callback function.
 */
static void can_xlnx_set_state_change_callback(const struct device *dev,
					       can_state_change_callback_t cb,
					       void *user_data)
{
	struct can_xlnx_dev_data *dev_data = dev->data;

	dev_data->state_chg_user_data = user_data;
	dev_data->state_chg_cb = cb;
}

/**
 * @brief Returns the core clock frequency driving the current controller.
 *
 * Returns the core clock frequency driving the current controller in-
 * stance. This value is identical for all instances of the Xilinx CAN
 * controller within the same Xilinx Processor System instance. While
 * clock source PLL and the prescalers applied to it are configurable
 * in the Processor System configuration within the Vivado project of
 * the current target, this is not a per-controller setting, but a set-
 * ting equally applied to all PS CAN controllers, unlike, for example,
 * the Xilinx GEM Ethernet Controller or the Xilinx PS UART, which each
 * allow per-istance reference clock configuration.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param rate If this pointer is not NULL, the core clock frequency
 *             will be written to the location pointed to.
 *
 * @retval always 0.
 */
int can_xlnx_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_xlnx_dev_cfg *dev_conf = dev->config;

	*rate = dev_conf->clock_frequency;
	return 0;
}

/**
 * @brief Returns the number of RX message filters supported.
 *
 * Returns the number of RX message filters that can be registered
 * for each instance of the CAN controller. For the Xilinx CAN con-
 * troller, this value is constant =4. As the Xilinx CAN controller
 * doesn't provide separate sets of filter slots for messages with
 * regular and extended CAN IDs, and as CAN FD is unsupported, the
 * id_type parameter is ignored.
 *
 * @param dev ignored.
 * @param id_type ignored.
 *
 * @retval always 4.
 */
int can_xlnx_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	ARG_UNUSED(dev);     /* Filter count is not device-specific */
	ARG_UNUSED(id_type); /* Not relevant for this controller's filters */

	/*
	 * Each instance of this controller always has 4 filters with
	 * corresponding configuration registers AFMR[4..1], AFIR[4..1].
	 */
	return CAN_XLNX_NUM_ACCEPTANCE_FILTERS;
}

/**
 * @brief Returns the maximum bit rate supported.
 *
 * Returns the maximum bit rate supported by the Xilinx CAN controller.
 *
 * @param dev Pointer to the CAN controller's device.
 * @param max_bitrate Pointer to the location to which the maxmimum
 *                    supported bit rate shall be written to.
 *
 * @retval always 0.
 */
int can_xlnx_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_xlnx_dev_cfg *dev_conf = dev->config;

	*max_bitrate = dev_conf->max_bitrate;

	return 0;
}

/**
 * @brief Xilinx CAN controller ISR
 *
 * Interrupt service routine for the Xilinx CAN controller's
 * IRQ. Handles RX OK, TX OK, bus off and error interrupts.
 *
 * @param dev Pointer to the CAN controller's device.
 */
static void can_xlnx_isr(const struct device *dev)
{
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	uint32_t isr;
#ifdef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	uint32_t sr;
#endif

	/* Get the Interrupt Status Register*/
	isr = sys_read32(base_addr + CAN_XLNX_ISR_OFFSET);

	/* Process Interrupt Status flags */

	/* TX-related interrupt handling */
	if (isr & CAN_XLNX_IRQ_MESSAGE_TX) {
		can_xlnx_handle_tx(dev);
	}

	/* RX-related interrupt handling */
	if (isr & CAN_XLNX_IRQ_MESSAGE_RX) {
		can_xlnx_handle_rx(dev);
	}

	/* Error-related interrupt handling */
	if (isr & CAN_XLNX_IRQ_MESSAGE_ERROR) {
		can_xlnx_handle_errors(dev);
	}

	if (isr & CAN_XLNX_IRQ_BUS_OFF) {
		/*
		 * CAN bus off state indication is always required in order to
		 * clear the driver instance's local error counters. This happens
		 * automatically in hardware whenever bus off state is entered.
		 */
		can_xlnx_state_update(dev, CAN_STATE_BUS_OFF);
#ifdef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
		/*
		 * The controller supports auto-recovery. In order to determine
		 * when auto-recovery has completed, poll SR[ESTAT] (comp. Zynq-
		 * 7000 TRM, Appendix B.5, p. 804) for any value other than 0x2
		 * (indicates bus off).
		 */
		do {
			sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
		} while (((sr >> CAN_XLNX_SR_ERROR_STATUS_OFFSET) &
			CAN_XLNX_SR_ERROR_STATUS_MASK) == CAN_XLNX_SR_ESTAT_BUS_OFF);

		can_xlnx_state_update(dev, CAN_STATE_ERROR_ACTIVE);
#else
		/*
		 * Bus off auto-recovery is disabled:
		 * -> leave the current state at 'bus off'
		 * -> enter configuration mode which disables further interrupts
		 * -> restoring an operational state requires explicit transition
		 * to normal/snoop/loopback mode
		 * -> 'bus off' to 'error active' state transition will take place
		 * once normal/snoop/loopback mode is entered
		 */
		can_xlnx_config_mode(dev);
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

		/* Clear the bus off interrupt bit */
		sys_write32(CAN_XLNX_IRQ_BUS_OFF, base_addr + CAN_XLNX_ICR_OFFSET);
	}
}

/**
 * @brief Initialize a Xilinx CAN controller device
 *
 * Initialize a Xilinx CAN controller device.
 *
 * @param dev Pointer to the CAN controller's device.
 *
 * @retval Always 0.
 */
static int can_xlnx_init(const struct device *dev)
{
	const struct can_xlnx_dev_cfg *dev_conf = dev->config;
	struct can_xlnx_dev_data *dev_data = dev->data;
	struct can_timing timing = {0};
	uint32_t sr;
	int ret;

	k_sem_init(&dev_data->filter_prot_sem, 1, 1);
	k_sem_init(&dev_data->tx_done_sem, 0, 1);
	k_sem_init(&dev_data->tx_done_prot_sem, 1, 1);
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	k_sem_init(&dev_data->recovery_sem, 0, 1);
#endif

	/* Apply MIO I/O pin mapping, if this is to be handled by the OS */
#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(dev_conf->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
#endif

	/* Map the respective device's memory, if applicable */
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);

	/* Reset the controller */
	sys_write32(CAN_XLNX_SRR_SOFTWARE_RESET, base_addr + CAN_XLNX_SRR_OFFSET);

	/* Wait for configuration mode: can.SR[CONFIG] must be 1 */
	do {
		sr = sys_read32(base_addr + CAN_XLNX_SR_OFFSET);
	} while (!(sr & CAN_XLNX_SR_CONFIG_MODE));

	/* Set up timing */
	ret = can_calc_timing(dev, &timing, dev_conf->bus_speed,
			      dev_conf->sample_point);

	timing.sjw = dev_conf->sjw;
	can_xlnx_set_timing(dev, &timing);

	/* Attach to the current controller's IRQ */
	dev_conf->config_func(dev);

	/* Start off in normal mode */
	can_xlnx_set_mode(dev, CAN_MODE_NORMAL);

	return 0;
}

/* Xilinx CAN controller driver API */
static const struct can_driver_api can_xlnx_apis = {
	.set_mode = can_xlnx_set_mode,
	.set_timing = can_xlnx_set_timing,
	.send = can_xlnx_send,
	.add_rx_filter = can_xlnx_add_rx_filter,
	.remove_rx_filter = can_xlnx_remove_rx_filter,
	.get_state = can_xlnx_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_xlnx_recover,
#endif
	.set_state_change_callback = can_xlnx_set_state_change_callback,
	.get_core_clock = can_xlnx_get_core_clock,
	.get_max_filters = can_xlnx_get_max_filters,
	.get_max_bitrate = can_xlnx_get_max_bitrate,
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = CAN_XLNX_BRPR_MIN_PRESCALER
	},
	.timing_max = {
		.sjw = 4,
		.prop_seg = 1,
		.phase_seg1 = 15,
		.phase_seg2 = 8,
		.prescaler = CAN_XLNX_BRPR_MAX_PRESCALER
	}
};

/* Register & initialize all CAN controllers specified in the device tree. */
DT_INST_FOREACH_STATUS_OKAY(CAN_XLNX_DEV_INITITALIZE);
