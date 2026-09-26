/*
 * Copyright (c) 2025-2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_canfd_2_0

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include "can_xilinx_canfd.h"
LOG_MODULE_REGISTER(xilinx_canfd, CONFIG_CAN_LOG_LEVEL);

/* Required by the DEVICE_MMIO_NAMED_* helpers, which expand to DEV_CFG()/DEV_DATA(). */
#define DEV_CFG(_dev)  ((const struct xilinx_canfd_cfg *)(_dev)->config)
#define DEV_DATA(_dev) ((struct xilinx_canfd_data *)(_dev)->data)

struct xilinx_canfd_filter {
	can_rx_callback_t callback;
	void *callback_arg;
	struct can_filter filter;
	bool in_use;
};

typedef void (*init_func_t)(const struct device *dev);

struct xilinx_canfd_cfg {
	const struct can_driver_config common;

	DEVICE_MMIO_NAMED_ROM(mmio);

	init_func_t init_func;
	const struct device *clock_dev;
};

struct xilinx_canfd_tx_mailbox {
	can_tx_callback_t callback;
	void *callback_arg;
	bool in_use;
};

struct xilinx_canfd_data {
	struct can_driver_data common;

	DEVICE_MMIO_NAMED_RAM(mmio);

	struct k_mutex inst_mutex;
	struct k_sem tx_sem;
	enum can_state state;
	struct xilinx_canfd_tx_mailbox tx_mailboxes[XCANFD_MAX_TX_MAILBOXES];
	atomic_t tx_mailbox_mask;
	struct xilinx_canfd_filter filters[XCANFD_MAX_FILTERS];
	uint32_t enabled_filters_mask;
};

static inline uint32_t xilinx_canfd_read32(const struct device *dev, uint32_t offs)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(dev, mmio) + offs);
}

static inline void xilinx_canfd_write32(const struct device *dev, uint32_t offs, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, mmio) + offs);
}

static void xilinx_canfd_filter_disable(const struct device *dev, uint32_t filter_mask)
{
	uint32_t filters;

	filters = xilinx_canfd_read32(dev, XCANFD_AFR_OFFSET);
	filters &= (~filter_mask);
	xilinx_canfd_write32(dev, XCANFD_AFR_OFFSET, filters);
}

static void xilinx_canfd_filter_enable(const struct device *dev, uint32_t filter_mask)
{
	uint32_t filters;

	filters = xilinx_canfd_read32(dev, XCANFD_AFR_OFFSET);
	filters |= filter_mask;
	xilinx_canfd_write32(dev, XCANFD_AFR_OFFSET, filters);
}

static int xilinx_canfd_acceptance_filter_set(const struct device *dev, uint32_t filter_index,
					      uint32_t mask_value, uint32_t id_value)
{
	uint32_t enabled_filters;

	if (filter_index < XCANFD_MIN_FILTER_INDEX || filter_index > XCANFD_MAX_FILTER_INDEX) {
		LOG_ERR("Invalid filter index: %u", filter_index);
		return -EINVAL;
	}

	enabled_filters = xilinx_canfd_read32(dev, XCANFD_AFR_OFFSET);
	if ((enabled_filters & BIT(filter_index - 1)) != 0) {
		LOG_ERR("Filter %u is currently enabled", filter_index);
		return -EBUSY;
	}

	filter_index--;

	xilinx_canfd_write32(dev, XCANFD_AFMR_ADDR(filter_index), mask_value);
	xilinx_canfd_write32(dev, XCANFD_AFIDR_ADDR(filter_index), id_value);

	return 0;
}

static void xilinx_canfd_filter_to_hw_format(const struct can_filter *filter, uint32_t *hw_mask,
					     uint32_t *hw_id)
{
	*hw_id = 0;
	*hw_mask = 0;

	if (filter->flags & CAN_FILTER_IDE) {
		/*
		 * The 29-bit extended identifier is split across two fields in
		 * the acceptance filter registers, mirroring the frame IDR
		 * layout: the 11 MSBs go into the standard AIID/AMID field
		 * ([31:21]) and the 18 LSBs into the AIID_EXT/AMID_EXT field
		 * ([18:1]).
		 */
		uint32_t id_std = (filter->id >> 18) & CAN_STD_ID_MASK;
		uint32_t id_ext = filter->id & GENMASK(17, 0);
		uint32_t mask_std = (filter->mask >> 18) & CAN_STD_ID_MASK;
		uint32_t mask_ext = filter->mask & GENMASK(17, 0);

		*hw_id |= (id_std << XCANFD_AFIDR_AIID_SHIFT) & XCANFD_AFIDR_AIID_MASK;
		*hw_id |= (id_ext << XCANFD_AFIDR_AIID_EXT_SHIFT) & XCANFD_AFIDR_AIID_EXT_MASK;
		*hw_id |= XCANFD_AFIDR_AIIDE_MASK;

		*hw_mask |= (mask_std << XCANFD_AFMR_AMID_SHIFT) & XCANFD_AFMR_AMID_MASK;
		*hw_mask |= (mask_ext << XCANFD_AFMR_AMID_EXT_SHIFT) & XCANFD_AFMR_AMID_EXT_MASK;
		*hw_mask |= XCANFD_AFMR_AMIDE_MASK;
	} else {
		*hw_id |= (filter->id << XCANFD_AFIDR_AIID_SHIFT) & XCANFD_AFIDR_AIID_MASK;
		*hw_mask |= (filter->mask << XCANFD_AFMR_AMID_SHIFT) & XCANFD_AFMR_AMID_MASK;
		*hw_mask |= XCANFD_AFMR_AMIDE_MASK;
	}

	/*
	 * Do not constrain the RTR/SRR bit in the acceptance mask. Zephyr CAN
	 * filters do not discriminate on RTR; whether remote frames are
	 * delivered is decided in software via CONFIG_CAN_ACCEPT_RTR. Setting
	 * the AMSRR/AMRTR mask bits here would reject all remote frames in
	 * hardware and defeat that option.
	 */
}

static bool xilinx_canfd_frame_matches_filter(const struct device *dev,
					      const struct can_frame *frame,
					      struct xilinx_canfd_filter **matched_filter)
{
	struct xilinx_canfd_data *data = dev->data;

	for (int i = 0; i < XCANFD_MAX_FILTERS; i++) {
		if (!data->filters[i].in_use) {
			continue;
		}

		if (!(data->enabled_filters_mask & BIT(i))) {
			continue;
		}

		if (can_frame_matches_filter(frame, &data->filters[i].filter)) {
			*matched_filter = &data->filters[i];
			return true;
		}
	}

	return false;
}

static void xilinx_canfd_update_state(const struct device *dev, enum can_state new_state);

static void xilinx_canfd_state_interrupt(const struct device *dev, uint32_t isr)
{
	uint32_t msr;

	if (isr & XCANFD_IXR_SLP_MASK) {
		msr = xilinx_canfd_read32(dev, XCANFD_MSR_OFFSET);

		if (!(msr & XCANFD_MSR_SLEEP_MASK)) {
			xilinx_canfd_write32(dev, XCANFD_MSR_OFFSET, msr | XCANFD_MSR_SLEEP_MASK);
		}
		xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, 0);
		xilinx_canfd_update_state(dev, CAN_STATE_STOPPED);
	}

	if (isr & XCANFD_IXR_WKUP_MASK) {
		xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, XCANFD_SRR_CEN_MASK);
		xilinx_canfd_update_state(dev, CAN_STATE_ERROR_ACTIVE);
	}
}

static void xilinx_canfd_tx_interrupt(const struct device *dev, uint32_t isr)
{
	struct xilinx_canfd_data *data = dev->data;
	uint32_t trr_reg;
	int mailbox_id;

	xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_TXOK_MASK);

	trr_reg = xilinx_canfd_read32(dev, XCANFD_TRR_OFFSET);

	for (mailbox_id = 0; mailbox_id < XCANFD_MAX_TX_MAILBOXES; mailbox_id++) {
		if ((atomic_get(&data->tx_mailbox_mask) & BIT(mailbox_id)) &&
		    !(trr_reg & BIT(mailbox_id))) {
			can_tx_callback_t callback = data->tx_mailboxes[mailbox_id].callback;
			void *callback_arg = data->tx_mailboxes[mailbox_id].callback_arg;

			data->tx_mailboxes[mailbox_id].in_use = false;
			data->tx_mailboxes[mailbox_id].callback = NULL;
			data->tx_mailboxes[mailbox_id].callback_arg = NULL;
			atomic_and(&data->tx_mailbox_mask, ~BIT(mailbox_id));
			k_sem_give(&data->tx_sem);

			if (callback) {
				callback(dev, 0, callback_arg);
			}
		}
	}
}

static void xilinx_canfd_parse_frame_id(struct can_frame *frame, uint32_t raw_id_reg)
{
	if (raw_id_reg & XCANFD_IDR_IDE_MASK) {
		frame->flags |= CAN_FRAME_IDE;
	}

	if (frame->flags & CAN_FRAME_IDE) {
		uint32_t id1_part = (raw_id_reg & XCANFD_IDR_ID1_MASK) >> XCANFD_IDR_ID1_SHIFT;
		uint32_t id2_part = (raw_id_reg & XCANFD_IDR_ID2_MASK) >> XCANFD_IDR_ID2_SHIFT;

		frame->id = (id1_part << 18) | id2_part;
	} else {
		frame->id = (raw_id_reg & XCANFD_IDR_ID1_MASK) >> XCANFD_IDR_ID1_SHIFT;
	}
}

static void xilinx_canfd_parse_frame_flags(struct can_frame *frame, uint32_t raw_id_reg,
					   uint32_t dlc_reg)
{
	if (dlc_reg & XCANFD_DLCR_EDL_MASK) {
		/*
		 * CAN FD frame: decode the FD-specific flags. CAN FD frames
		 * never carry the RTR/SRR bit. The BRS/ESI DLC bits are only
		 * meaningful for FD frames, so they must not be inspected for
		 * classic frames (where they are undefined).
		 */
		frame->flags |= CAN_FRAME_FDF;

		if (dlc_reg & XCANFD_DLCR_BRS_MASK) {
			frame->flags |= CAN_FRAME_BRS;
		}

		if (dlc_reg & XCANFD_DLCR_ESI_MASK) {
			frame->flags |= CAN_FRAME_ESI;
		}
	} else if (raw_id_reg & XCANFD_IDR_IDE_MASK) {
		/* Classic extended frame: RTR is in the dedicated RTR bit. */
		if (raw_id_reg & XCANFD_IDR_RTR_MASK) {
			frame->flags |= CAN_FRAME_RTR;
		}
	} else {
		/* Classic standard frame: RTR is encoded in the SRR bit. */
		if (raw_id_reg & XCANFD_IDR_SRR_MASK) {
			frame->flags |= CAN_FRAME_RTR;
		}
	}
}

static bool xilinx_canfd_validate_dlc(const struct can_frame *frame)
{
	if ((frame->flags & CAN_FRAME_FDF) != 0) {
		if (frame->dlc > CANFD_MAX_DLC) {
			LOG_ERR("Invalid CANFD DLC: %d (max %d)", frame->dlc, CANFD_MAX_DLC);
			return false;
		}
	} else {
		if (frame->dlc > CAN_MAX_DLC) {
			LOG_ERR("Invalid classic CAN DLC: %d (max %d)", frame->dlc, CAN_MAX_DLC);
			return false;
		}
	}
	return true;
}

static void xilinx_canfd_read_frame_data(const struct device *dev, struct can_frame *frame,
					 uint32_t offset)
{
	uint32_t dw_offset = XCANFD_FRAME_DW_ADDR(offset);
	uint32_t reg_data;
	int copy_len;
	int len = can_dlc_to_bytes(frame->dlc);

	if (len > 0 && len <= sizeof(frame->data)) {
		for (int i = 0; i < len; i += 4) {
			reg_data = xilinx_canfd_read32(dev, dw_offset);
			reg_data = BSWAP_32(reg_data);
			copy_len = MIN(4, len - i);
			memcpy(&frame->data[i], &reg_data, copy_len);
			dw_offset += XCANFD_DW_BYTES;
		}
	}
}

static void xilinx_canfd_process_rx_frame(const struct device *dev, uint32_t offset)
{
	struct can_frame frame = {0};
	uint32_t raw_id_reg;
	uint32_t dlc_reg;

	raw_id_reg = xilinx_canfd_read32(dev, XCANFD_FRAME_ID_ADDR(offset));
	dlc_reg = xilinx_canfd_read32(dev, XCANFD_FRAME_DLC_ADDR(offset));

	xilinx_canfd_parse_frame_flags(&frame, raw_id_reg, dlc_reg);
	xilinx_canfd_parse_frame_id(&frame, raw_id_reg);

	frame.dlc = (dlc_reg & XCANFD_DLCR_DLC_MASK) >> XCANFD_DLCR_DLC_SHIFT;

	if (!xilinx_canfd_validate_dlc(&frame)) {
		xilinx_canfd_write32(dev, XCANFD_FSR_OFFSET, XCANFD_FSR_IRI_MASK);
		return;
	}

	xilinx_canfd_read_frame_data(dev, &frame, offset);

	/*
	 * Unless CONFIG_CAN_ACCEPT_RTR is enabled, RTR frames are rejected at
	 * the driver level. The RX buffer is still released below either way.
	 */
	if (IS_ENABLED(CONFIG_CAN_ACCEPT_RTR) || (frame.flags & CAN_FRAME_RTR) == 0U) {
		struct xilinx_canfd_filter *matched_filter = NULL;

		if (xilinx_canfd_frame_matches_filter(dev, &frame, &matched_filter)) {
			matched_filter->callback(dev, &frame, matched_filter->callback_arg);
		}
	}

	/* Release the RX buffer by pulsing the increment-read-index bit. */
	xilinx_canfd_write32(dev, XCANFD_FSR_OFFSET, XCANFD_FSR_IRI_MASK);
}

static void xilinx_canfd_rx_interrupt(const struct device *dev, uint32_t isr)
{
	uint32_t fsr;
	int offset;

	ARG_UNUSED(isr);

	/*
	 * Clear RXOK up front, before the is-empty check, so that a frame which
	 * arrives while we are draining the FIFO re-asserts RXOK (and thus
	 * re-triggers the ISR) instead of being lost to a race.
	 */
	xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_RXOK_MASK);

	/*
	 * Drain the RX FIFO: the fill level (FL) field reports the number of
	 * stored frames, and processing each frame advances the read index and
	 * decrements FL, so keep going until the FIFO is empty.
	 */
	fsr = xilinx_canfd_read32(dev, XCANFD_FSR_OFFSET);
	while (fsr & XCANFD_2_FSR_FL_MASK) {
		offset = XCANFD_RXMSG_2_FRAME_ADDR(fsr & XCANFD_2_FSR_RI_MASK);
		xilinx_canfd_process_rx_frame(dev, offset);
		fsr = xilinx_canfd_read32(dev, XCANFD_FSR_OFFSET);
	}
}

static void xilinx_canfd_abort_all_tx(const struct device *dev, struct xilinx_canfd_data *data,
				      int error)
{
	unsigned int key = irq_lock();

	for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
		if (data->tx_mailboxes[i].in_use) {
			can_tx_callback_t callback = data->tx_mailboxes[i].callback;
			void *callback_arg = data->tx_mailboxes[i].callback_arg;

			data->tx_mailboxes[i].in_use = false;
			data->tx_mailboxes[i].callback = NULL;
			data->tx_mailboxes[i].callback_arg = NULL;
			atomic_and(&data->tx_mailbox_mask, ~BIT(i));
			k_sem_give(&data->tx_sem);
			if (callback != NULL) {
				irq_unlock(key);
				callback(dev, error, callback_arg);
				key = irq_lock();
			}
		}
	}
	irq_unlock(key);
}

static enum can_state xilinx_canfd_get_error_state(const struct device *dev)
{
	uint32_t sr = xilinx_canfd_read32(dev, XCANFD_SR_OFFSET);
	uint32_t estat = (sr & XCANFD_SR_ESTAT_MASK) >> XCANFD_SR_ESTAT_SHIFT;

	switch (estat) {
	case XCANFD_SR_ESTAT_BUSOFF:
		return CAN_STATE_BUS_OFF;
	case XCANFD_SR_ESTAT_PASSIVE:
		return CAN_STATE_ERROR_PASSIVE;
	case XCANFD_SR_ESTAT_ACTIVE:
		/* Check ERRWRN bit to distinguish between active and warning */
		if (sr & XCANFD_SR_ERRWRN_MASK) {
			return CAN_STATE_ERROR_WARNING;
		}
		return CAN_STATE_ERROR_ACTIVE;
	default:
		return CAN_STATE_ERROR_ACTIVE;
	}
}

static void xilinx_canfd_update_state(const struct device *dev, enum can_state new_state)
{
	struct xilinx_canfd_data *data = dev->data;
	struct can_bus_err_cnt err_cnt = {0};
	unsigned int key;
	uint32_t ecr;

	/*
	 * update_state() runs from both ISR (error/bus-off) and thread
	 * (recovery) contexts, so the check-and-set of data->state must be
	 * atomic to avoid a lost update or a duplicate state-change callback.
	 */
	key = irq_lock();
	if (new_state == data->state) {
		irq_unlock(key);
		return;
	}
	data->state = new_state;
	irq_unlock(key);

	ecr = xilinx_canfd_read32(dev, XCANFD_ECR_OFFSET);
	err_cnt.tx_err_cnt = ecr & XCANFD_ECR_TEC_MASK;
	err_cnt.rx_err_cnt = (ecr & XCANFD_ECR_REC_MASK) >> XCANFD_ECR_REC_SHIFT;
	can_fire_state_change_callbacks(dev, new_state, err_cnt);
}

static void xilinx_canfd_handle_error(const struct device *dev)
{
	enum can_state current_state;
	uint32_t esr = xilinx_canfd_read32(dev, XCANFD_ESR_OFFSET);

	if ((esr & XCANFD_ESR_CRCER_MASK) || (esr & XCANFD_ESR_F_CRCER_MASK)) {
		CAN_STATS_CRC_ERROR_INC(dev);
	}
	if ((esr & XCANFD_ESR_FMER_MASK) || (esr & XCANFD_ESR_F_FMER_MASK)) {
		CAN_STATS_FORM_ERROR_INC(dev);
	}
	if ((esr & XCANFD_ESR_STER_MASK) || (esr & XCANFD_ESR_F_STER_MASK)) {
		CAN_STATS_STUFF_ERROR_INC(dev);
	}
	if ((esr & XCANFD_ESR_BERR_MASK) || (esr & XCANFD_ESR_F_BERR_MASK)) {
		CAN_STATS_BIT_ERROR_INC(dev);
	}
	if (esr & XCANFD_ESR_ACKER_MASK) {
		CAN_STATS_ACK_ERROR_INC(dev);
	}
	xilinx_canfd_write32(dev, XCANFD_ESR_OFFSET, esr);

	current_state = xilinx_canfd_get_error_state(dev);
	xilinx_canfd_update_state(dev, current_state);
}

static void xilinx_canfd_isr(const struct device *dev)
{
	struct xilinx_canfd_data *data = dev->data;
	uint32_t isr;

	if (data->state == CAN_STATE_STOPPED && !data->common.started) {
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_ICR_CLEAR_ALL);
		return;
	}

	isr = xilinx_canfd_read32(dev, XCANFD_ISR_OFFSET);
	if (!isr) {
		return;
	}

	if (isr & (XCANFD_IXR_SLP_MASK | XCANFD_IXR_WKUP_MASK)) {
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET,
				     (XCANFD_IXR_SLP_MASK | XCANFD_IXR_WKUP_MASK));
		xilinx_canfd_state_interrupt(dev, isr);
	}

	if (isr & XCANFD_IXR_ARBLST_MASK) {
		/*
		 * Arbitration loss is a normal bus event, not a transmit
		 * failure: the controller automatically retries the frame once
		 * the bus is free, and completion is reported later via TXOK.
		 * Acknowledge the interrupt but leave pending TX mailboxes and
		 * their callbacks untouched.
		 */
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_ARBLST_MASK);
		LOG_DBG("arbitration lost");
	}

	if (isr & XCANFD_IXR_TXOK_MASK) {
		xilinx_canfd_tx_interrupt(dev, isr);
	}

	if (isr & XCANFD_IXR_ERROR_MASK) {
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_ERROR_MASK);
		xilinx_canfd_handle_error(dev);
	}

	if (isr & XCANFD_IXR_RXOFLW_MASK) {
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_RXOFLW_MASK);
		CAN_STATS_RX_OVERRUN_INC(dev);
	}

	if (isr & XCANFD_IXR_RXFOFLW_1_MASK) {
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_RXFOFLW_1_MASK);
		CAN_STATS_RX_OVERRUN_INC(dev);
	}

	if (isr & XCANFD_IXR_BSOFF_MASK) {
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_BSOFF_MASK);

		xilinx_canfd_update_state(dev, CAN_STATE_BUS_OFF);
		xilinx_canfd_abort_all_tx(dev, data, -ENETUNREACH);
	}

	/*
	 * A received frame typically asserts both RXOK and RXNEMP. Drain the
	 * FIFO once for either source to avoid processing the same frames twice.
	 */
	if (isr & (XCANFD_IXR_RXOK_MASK | XCANFD_IXR_RXNEMP_MASK)) {
		xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_IXR_RXNEMP_MASK);
		xilinx_canfd_rx_interrupt(dev, isr);
	}
}

static int xilinx_canfd_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);
	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		*cap |= CAN_MODE_FD;
	}

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return 0;
}

static int xilinx_canfd_set_mode(const struct device *dev, can_mode_t mode)
{
	struct xilinx_canfd_data *data = dev->data;
	can_mode_t supported;
	uint32_t msr_reg = 0;
	int ret = 0;

	(void)xilinx_canfd_get_capabilities(dev, &supported);

	if (data->common.started) {
		LOG_ERR("Cannot change mode while driver is started");
		return -EBUSY;
	}

	if ((mode & ~supported) != 0) {
		LOG_ERR("Unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if ((mode & (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) ==
	    (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) {
		LOG_ERR("Loopback and listen-only modes must not be combined");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, 0);

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		msr_reg |= XCANFD_MSR_LBACK_MASK;
	} else if ((mode & CAN_MODE_LISTENONLY) != 0) {
		msr_reg |= XCANFD_MSR_SNOOP_MASK;
	} else {
		/* Normal mode - no special mode flags set */
	}

	if ((mode & CAN_MODE_FD) != 0) {
		if (!IS_ENABLED(CONFIG_CAN_FD_MODE)) {
			LOG_ERR("CONFIG_CAN_FD_MODE is not enabled");
			ret = -ENOTSUP;
			goto unlock;
		}
		/*
		 * Transmitter Delay Compensation is configured in the data-phase
		 * prescaler register (F_BRPR) by xilinx_canfd_set_timing_data(),
		 * not here in the arbitration-phase register.
		 */
	}

	/*
	 * Only program the mode bits here and leave the controller in
	 * configuration mode (disabled/off-bus). The controller is enabled
	 * (CEN) by xilinx_canfd_start() once interrupts and filters are set up.
	 */
	xilinx_canfd_write32(dev, XCANFD_MSR_OFFSET, msr_reg);
	data->common.mode = mode;

unlock:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static int xilinx_canfd_start(const struct device *dev)
{
	const struct xilinx_canfd_cfg *config = dev->config;
	struct xilinx_canfd_data *data = dev->data;
	uint32_t ier;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		LOG_ERR("CAN controller already started");
		k_mutex_unlock(&data->inst_mutex);
		return -EALREADY;
	}

	if (config->common.phy != NULL) {
		ret = can_transceiver_enable(config->common.phy, data->common.mode);
		if (ret != 0) {
			LOG_ERR("Failed to enable CAN transceiver (err %d)", ret);
			k_mutex_unlock(&data->inst_mutex);
			return ret;
		}
	}

	CAN_STATS_RESET(dev);

	for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
		data->tx_mailboxes[i].in_use = false;
		data->tx_mailboxes[i].callback = NULL;
		data->tx_mailboxes[i].callback_arg = NULL;
	}
	atomic_set(&data->tx_mailbox_mask, 0);
	k_sem_init(&data->tx_sem, XCANFD_MAX_TX_MAILBOXES, XCANFD_MAX_TX_MAILBOXES);

	ret = xilinx_canfd_set_mode(dev, data->common.mode);

	if (ret != 0) {
		LOG_ERR("Failed to enter configured mode %d (err %d)", data->common.mode, ret);
		if (config->common.phy != NULL) {
			(void)can_transceiver_disable(config->common.phy);
		}
		k_mutex_unlock(&data->inst_mutex);
		return ret;
	}

	data->common.started = true;
	data->state = CAN_STATE_ERROR_ACTIVE;

	ier = XCANFD_IXR_TXOK_MASK | XCANFD_IXR_WKUP_MASK | XCANFD_IXR_SLP_MASK |
	      XCANFD_IXR_RXOK_MASK | XCANFD_IXR_RXNEMP_MASK | XCANFD_IXR_ERROR_MASK |
	      XCANFD_IXR_BSOFF_MASK | XCANFD_IXR_ARBLST_MASK | XCANFD_IXR_RXOFLW_MASK |
	      XCANFD_IXR_RXFOFLW_1_MASK;

	xilinx_canfd_write32(dev, XCANFD_IER_OFFSET, ier);

	xilinx_canfd_filter_disable(dev, XCANFD_AFR_UAF_ALL_MASK);
	if (data->enabled_filters_mask != 0) {
		xilinx_canfd_filter_enable(dev, data->enabled_filters_mask);
	}

	/* Enable the controller (go on-bus) now that mode, interrupts and
	 * filters are configured.
	 */
	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, XCANFD_SRR_CEN_MASK);

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int xilinx_canfd_stop(const struct device *dev)
{
	const struct xilinx_canfd_cfg *config = dev->config;
	struct xilinx_canfd_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->common.started) {
		k_mutex_unlock(&data->inst_mutex);
		return -EALREADY;
	}

	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, 0);

	if (!WAIT_FOR((xilinx_canfd_read32(dev, XCANFD_SR_OFFSET) & XCANFD_SR_CONFIG_MASK),
		      XCANFD_TIMEOUT_MS * USEC_PER_MSEC, k_busy_wait(1))) {
		LOG_ERR("Timeout waiting for configuration mode");
		ret = -ETIMEDOUT;
	}

	xilinx_canfd_write32(dev, XCANFD_IER_OFFSET, 0);

	xilinx_canfd_abort_all_tx(dev, data, -ENETDOWN);

	data->common.started = false;
	data->state = CAN_STATE_STOPPED;

	if (config->common.phy != NULL) {
		ret = can_transceiver_disable(config->common.phy);
		if (ret != 0) {
			LOG_ERR("Failed to disable CAN transceiver (err %d)", ret);
		}
	}

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

#ifdef CONFIG_CAN_FD_MODE
static int xilinx_canfd_set_timing_data(const struct device *dev, const struct can_timing *timing)
{
	struct xilinx_canfd_data *data = dev->data;
	uint32_t btr0, btr1;

	if (data->common.started) {
		LOG_ERR("Cannot set bit timing data while CAN controller is started");
		return -EBUSY;
	}

	btr0 = (timing->prescaler - 1) & XCANFD_BRPR_BRP_MASK;

	/*
	 * Transmitter Delay Compensation is only meaningful (and only supported
	 * by the IP) at the highest data bitrates, i.e. a data phase prescaler
	 * of 1 or 2. Enable it there and program the offset for correct
	 * placement of the Secondary Sample Point.
	 */
	if (timing->prescaler <= 2U) {
		uint32_t tdco = CAN_CALC_TDCO(timing, 0U, XCANFD_TDCO_MAX);

		btr0 |= XCANFD_BRPR_TDC_ENABLE_MASK;
		btr0 |= (tdco << XCANFD_BRPR_TDCO_SHIFT) & XCANFD_BRPR_TDCO_MASK;
		LOG_DBG("TDC enabled, using TDCO %u", tdco);
	}

	btr1 = ((timing->prop_seg + timing->phase_seg1 - 1)) & XCANFD_F_BTR_TS1_MASK_CANFD;
	btr1 |= (((timing->phase_seg2 - 1) << XCANFD_BTR_TS2_SHIFT_CANFD) &
		 XCANFD_F_BTR_TS2_MASK_CANFD);
	btr1 |= (((timing->sjw - 1) << XCANFD_BTR_SJW_SHIFT_CANFD) & XCANFD_F_BTR_SJW_MASK_CANFD);

	xilinx_canfd_write32(dev, XCANFD_F_BRPR_OFFSET, btr0);
	xilinx_canfd_write32(dev, XCANFD_F_BTR_OFFSET, btr1);

	return 0;
}
#endif /* CONFIG_CAN_FD_MODE */

static int xilinx_canfd_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct xilinx_canfd_data *data = dev->data;
	uint32_t btr0, btr1;

	if (data->common.started) {
		LOG_ERR("Cannot set bit timing while CANFD controller is started");
		return -EBUSY;
	}

	btr0 = (timing->prescaler - 1) & XCANFD_BRPR_BRP_MASK;
	btr1 = ((timing->prop_seg + timing->phase_seg1 - 1)) & XCANFD_BTR_TS1_MASK_CANFD;
	btr1 |= (((timing->phase_seg2 - 1) << XCANFD_BTR_TS2_SHIFT_CANFD) &
		 XCANFD_BTR_TS2_MASK_CANFD);
	btr1 |= (((timing->sjw - 1) << XCANFD_BTR_SJW_SHIFT_CANFD) & XCANFD_BTR_SJW_MASK_CANFD);

	xilinx_canfd_write32(dev, XCANFD_BRPR_OFFSET, btr0);
	xilinx_canfd_write32(dev, XCANFD_BTR_OFFSET, btr1);

	return 0;
}

static int xilinx_canfd_get_state(const struct device *dev, enum can_state *state,
				  struct can_bus_err_cnt *err_cnt)
{
	struct xilinx_canfd_data *data = dev->data;

	if (state) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			*state = data->state;
		}
	}

	if (err_cnt) {
		err_cnt->tx_err_cnt =
			xilinx_canfd_read32(dev, XCANFD_ECR_OFFSET) & XCANFD_ECR_TEC_MASK;
		err_cnt->rx_err_cnt =
			((xilinx_canfd_read32(dev, XCANFD_ECR_OFFSET) & XCANFD_ECR_REC_MASK) >>
			 XCANFD_ECR_REC_SHIFT);
	}
	return 0;
}

static int xilinx_canfd_validate_frame(const struct xilinx_canfd_data *data,
				       const struct can_frame *frame)
{
	if (!frame) {
		LOG_ERR("Frame pointer is NULL");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		if ((frame->flags &
		     ~(CAN_FRAME_IDE | CAN_FRAME_RTR | CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
			LOG_ERR("Unsupported CANFD frame flags 0x%02x", frame->flags);
			return -ENOTSUP;
		}

		if ((frame->flags & CAN_FRAME_FDF) != 0 &&
		    (data->common.mode & CAN_MODE_FD) == 0U) {
			LOG_ERR("CANFD frame not supported in current mode");
			return -ENOTSUP;
		}

		if ((frame->flags & CAN_FRAME_FDF) != 0 && (frame->flags & CAN_FRAME_RTR) != 0) {
			LOG_ERR("RTR not supported for CANFD frames");
			return -ENOTSUP;
		}
	} else {
		if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
			LOG_ERR("Unsupported CANFD frame flags 0x%02x", frame->flags);
			return -ENOTSUP;
		}
	}

	return 0;
}

static int xilinx_canfd_validate_frame_dlc(const struct can_frame *frame)
{
	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		if ((frame->flags & CAN_FRAME_FDF) != 0) {
			if (frame->dlc > CANFD_MAX_DLC) {
				LOG_ERR("CANFD DLC of %d exceeds maximum (%d)", frame->dlc,
					CANFD_MAX_DLC);
				return -EINVAL;
			}
		} else {
			if (frame->dlc > CAN_MAX_DLC) {
				LOG_ERR("Classic CANFD DLC of %d exceeds maximum (%d)", frame->dlc,
					CAN_MAX_DLC);
				return -EINVAL;
			}
		}
	} else {
		if (frame->dlc > CAN_MAX_DLC) {
			LOG_ERR("Classic CAN DLC of %d exceeds maximum (%d)", frame->dlc,
				CAN_MAX_DLC);
			return -EINVAL;
		}
	}

	return 0;
}

static int xilinx_canfd_find_free_mailbox(const struct device *dev, struct xilinx_canfd_data *data)
{
	uint32_t trr_reg = xilinx_canfd_read32(dev, XCANFD_TRR_OFFSET);

	for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
		if (!data->tx_mailboxes[i].in_use && !(trr_reg & BIT(i))) {
			return i;
		}
	}

	return -1;
}

static uint32_t xilinx_canfd_build_id_reg(const struct can_frame *frame)
{
	uint32_t id_reg = 0;

	if (frame->flags & CAN_FRAME_IDE) {
		uint32_t upper_11_bits = (frame->id >> 18) & 0x7FF;
		uint32_t lower_18_bits = frame->id & 0x3FFFF;

		/* Extended frames must always assert SRR (recessive). */
		id_reg = (upper_11_bits << XCANFD_IDR_ID1_SHIFT) |
			 (lower_18_bits << XCANFD_IDR_ID2_SHIFT) | XCANFD_IDR_IDE_MASK |
			 XCANFD_IDR_SRR_MASK;

		/* For extended frames RTR is encoded in the dedicated RTR bit. */
		if (frame->flags & CAN_FRAME_RTR) {
			id_reg |= XCANFD_IDR_RTR_MASK;
		}
	} else {
		id_reg = (frame->id << XCANFD_IDR_ID1_SHIFT) & XCANFD_IDR_ID1_MASK;

		/* For standard frames RTR is encoded in the SRR bit position. */
		if (frame->flags & CAN_FRAME_RTR) {
			id_reg |= XCANFD_IDR_SRR_MASK;
		}
	}

	return id_reg;
}

static uint32_t xilinx_canfd_build_dlc_reg(const struct can_frame *frame)
{
	uint32_t dlc_reg = (frame->dlc << XCANFD_DLCR_DLC_SHIFT) & XCANFD_DLCR_DLC_MASK;

	if (frame->flags & CAN_FRAME_FDF) {
		dlc_reg |= XCANFD_DLCR_EDL_MASK;
	}
	if (frame->flags & CAN_FRAME_BRS) {
		dlc_reg |= XCANFD_DLCR_BRS_MASK;
	}
	if (frame->flags & CAN_FRAME_ESI) {
		dlc_reg |= XCANFD_DLCR_ESI_MASK;
	}

	return dlc_reg;
}

static void xilinx_canfd_write_frame_data(const struct device *dev, const struct can_frame *frame,
					  uint32_t frame_offset)
{
	uint32_t nobytes = can_dlc_to_bytes(frame->dlc);
	uint32_t dwindex = 0;

	for (uint32_t len = 0; len < nobytes; len += 4) {
		uint32_t value = BSWAP_32(frame->data_32[len / 4]);

		xilinx_canfd_write32(
			dev, (XCANFD_FRAME_DW_ADDR(frame_offset) + (dwindex * XCANFD_DW_BYTES)),
			value);
		dwindex++;
	}
}

static int xilinx_canfd_send(const struct device *dev, const struct can_frame *frame,
			     k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct xilinx_canfd_data *data = dev->data;
	uint32_t frame_offset;
	uint32_t dlc_reg;
	uint32_t id_reg;
	unsigned int key;
	int mailbox_id;
	int ret;

	if (!data->common.started) {
		LOG_ERR("CANFD controller not started");
		return -ENETDOWN;
	}

	ret = xilinx_canfd_validate_frame(data, frame);
	if (ret != 0) {
		return ret;
	}

	ret = xilinx_canfd_validate_frame_dlc(frame);
	if (ret != 0) {
		return ret;
	}

	if (k_sem_take(&data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	mailbox_id = xilinx_canfd_find_free_mailbox(dev, data);
	if (mailbox_id == -1) {
		k_mutex_unlock(&data->inst_mutex);
		k_sem_give(&data->tx_sem);
		return -EBUSY;
	}

	frame_offset = XCANFD_TXMSG_FRAME_ADDR(mailbox_id);

	id_reg = xilinx_canfd_build_id_reg(frame);
	dlc_reg = xilinx_canfd_build_dlc_reg(frame);

	/*
	 * Populate the TX mailbox contents before arming it. The controller
	 * does not act on the buffer until the corresponding TRR bit is set.
	 */
	xilinx_canfd_write32(dev, XCANFD_FRAME_ID_ADDR(frame_offset), id_reg);
	xilinx_canfd_write32(dev, XCANFD_FRAME_DLC_ADDR(frame_offset), dlc_reg);
	xilinx_canfd_write_frame_data(dev, frame, frame_offset);

	/*
	 * Arming the mailbox (marking it in-use, setting the completion mask
	 * bit and requesting transmission) must be atomic with respect to the
	 * TX-done ISR. Otherwise tx_interrupt() could observe the mask bit set
	 * while the TRR bit is not yet set and wrongly complete the frame
	 * before it has been transmitted.
	 */
	key = irq_lock();
	data->tx_mailboxes[mailbox_id].in_use = true;
	data->tx_mailboxes[mailbox_id].callback = callback;
	data->tx_mailboxes[mailbox_id].callback_arg = user_data;
	atomic_or(&data->tx_mailbox_mask, BIT(mailbox_id));
	xilinx_canfd_write32(dev, XCANFD_TRR_OFFSET, BIT(mailbox_id));
	irq_unlock(key);

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int xilinx_canfd_add_rx_filter(const struct device *dev, can_rx_callback_t cb, void *cb_arg,
				      const struct can_filter *filter)
{
	struct xilinx_canfd_data *data = dev->data;
	uint32_t hw_mask, hw_id;
	int filter_index = -1;
	unsigned int key;
	int ret = 0;

	/* NULL callback/filter and CAN ID/mask range are validated in can_common.c. */

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	for (int i = 0; i < XCANFD_MAX_FILTERS; i++) {
		if (!data->filters[i].in_use) {
			filter_index = i;
			break;
		}
	}

	if (filter_index == -1) {
		LOG_ERR("No available hardware filters");
		ret = -ENOSPC;
		goto unlock;
	}

	xilinx_canfd_filter_to_hw_format(filter, &hw_mask, &hw_id);

	ret = xilinx_canfd_acceptance_filter_set(dev, filter_index + 1, hw_mask, hw_id);
	if (ret != 0) {
		LOG_ERR("Failed to set hardware filter %d", filter_index);
		goto unlock;
	}

	/*
	 * Populate the software filter slot and mark it discoverable with
	 * interrupts locked so the RX ISR never observes a half-initialised
	 * slot. This is done before enabling hardware acceptance so a frame
	 * arriving the instant filtering is enabled is not dropped for lack of
	 * a software match.
	 */
	key = irq_lock();
	data->filters[filter_index].callback = cb;
	data->filters[filter_index].callback_arg = cb_arg;
	data->filters[filter_index].filter = *filter;
	data->filters[filter_index].in_use = true;
	data->enabled_filters_mask |= BIT(filter_index);
	irq_unlock(key);

	xilinx_canfd_filter_enable(dev, BIT(filter_index));

	ret = filter_index;

unlock:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static void xilinx_canfd_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct xilinx_canfd_data *data = dev->data;
	unsigned int key;

	if (filter_id < 0 || filter_id >= XCANFD_MAX_FILTERS) {
		LOG_ERR("Invalid filter ID: %d", filter_id);
		return;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->filters[filter_id].in_use) {
		LOG_WRN("Filter %d is not in use", filter_id);
		goto unlock;
	}

	/* Stop hardware acceptance for this slot before tearing it down. */
	xilinx_canfd_filter_disable(dev, BIT(filter_id));

	/*
	 * Clear the software slot with interrupts locked so the RX ISR cannot
	 * dispatch to a filter that is being removed.
	 */
	key = irq_lock();
	data->enabled_filters_mask &= ~BIT(filter_id);
	memset(&data->filters[filter_id], 0, sizeof(data->filters[filter_id]));
	data->filters[filter_id].in_use = false;
	irq_unlock(key);

unlock:
	k_mutex_unlock(&data->inst_mutex);
}

static int xilinx_canfd_reset(const struct device *dev)
{
	xilinx_canfd_write32(dev, XCANFD_IER_OFFSET, 0);
	xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_ICR_CLEAR_ALL);
	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, XCANFD_SRR_RESET_MASK);

	if (!WAIT_FOR((xilinx_canfd_read32(dev, XCANFD_SR_OFFSET) & XCANFD_SR_CONFIG_MASK),
		      XCANFD_TIMEOUT_MS * USEC_PER_MSEC, k_busy_wait(1))) {
		LOG_ERR("Timeout exceeded while waiting for configuration mode (SR=0x%08x)",
			xilinx_canfd_read32(dev, XCANFD_SR_OFFSET));
		return -ETIMEDOUT;
	}
	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, 0);
	return 0;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int xilinx_canfd_recover(const struct device *dev, k_timeout_t timeout)
{
	struct xilinx_canfd_data *data = dev->data;
	struct can_bus_err_cnt err_cnt;
	enum can_state current_state;
	k_timepoint_t end;
	int ret;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->inst_mutex, K_FOREVER);
	if (ret != 0) {
		return -EAGAIN;
	}

	xilinx_canfd_get_state(dev, &current_state, &err_cnt);
	if (current_state != CAN_STATE_BUS_OFF) {
		k_mutex_unlock(&data->inst_mutex);
		return 0;
	}

	/*
	 * Recover by taking the controller off-bus and back on-bus. Clearing
	 * CEN enters configuration mode and resets the error counters (thus
	 * clearing the bus-off condition), while preserving the programmed bit
	 * timing, operating mode and acceptance filters. A full soft reset is
	 * intentionally avoided here as it would wipe that configuration.
	 */
	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, 0);

	if (!WAIT_FOR((xilinx_canfd_read32(dev, XCANFD_SR_OFFSET) & XCANFD_SR_CONFIG_MASK),
		      XCANFD_TIMEOUT_MS * USEC_PER_MSEC, k_busy_wait(1))) {
		LOG_ERR("Timeout waiting for configuration mode during recovery");
		ret = -EIO;
		goto unlock;
	}

	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, XCANFD_SRR_CEN_MASK);

	end = sys_timepoint_calc(timeout);
	while (!sys_timepoint_expired(end)) {
		k_busy_wait(100);

		current_state = xilinx_canfd_get_error_state(dev);
		if (current_state != CAN_STATE_BUS_OFF) {
			xilinx_canfd_update_state(dev, current_state);
			ret = 0;
			goto unlock;
		}
	}

	/* Timeout occurred */
	ret = -EAGAIN;

unlock:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static int xilinx_canfd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct xilinx_canfd_cfg *config = dev->config;
	int ret;

	ret = clock_control_get_rate(config->clock_dev, NULL, rate);
	if (ret != 0) {
		return ret;
	}

	/*
	 * The reported rate can carry small inaccuracies from devicetree or
	 * clock-tree rounding (e.g. 159,998,398 Hz where the real clock is
	 * 160 MHz). can_calc_timing() only accepts a prescaler when
	 * core_clock % (prescaler * bitrate) == 0, so such a value yields no
	 * valid timing. CAN reference clocks are integer-MHz by design, so
	 * round to the nearest MHz. The residual on-wire bitrate error is far
	 * below the CAN bit-timing tolerance.
	 */
	*rate = DIV_ROUND_CLOSEST(*rate, 1000000U) * 1000000U;

	/*
	 * The CANFD IP has a fixed internal prescaler that divides the input
	 * clock by 2 before it reaches the baud rate generator. Report the
	 * effective clock so the timing calculation derives correct prescaler
	 * and segment values (otherwise the on-wire bitrate is halved).
	 */
	*rate /= 2U;

	return 0;
}

static int xilinx_canfd_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);
	return XCANFD_MAX_FILTERS;
}

static int xilinx_canfd_init(const struct device *dev)
{
	const struct xilinx_canfd_cfg *config = dev->config;
	struct xilinx_canfd_data *data = dev->data;
	struct can_timing timing;
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, mmio, K_MEM_CACHE_NONE);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock controller device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, NULL);
	if (ret != 0 && ret != -EALREADY && ret != -ENOSYS) {
		LOG_ERR("Failed to enable CAN core clock: %d", ret);
		return ret;
	}

	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_sem, XCANFD_MAX_TX_MAILBOXES, XCANFD_MAX_TX_MAILBOXES);
	data->state = CAN_STATE_STOPPED;

	memset(data->tx_mailboxes, 0, sizeof(data->tx_mailboxes));
	atomic_set(&data->tx_mailbox_mask, 0);
	for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
		data->tx_mailboxes[i].in_use = false;
		data->tx_mailboxes[i].callback = NULL;
		data->tx_mailboxes[i].callback_arg = NULL;
	}

	memset(data->filters, 0, sizeof(data->filters));
	data->enabled_filters_mask = 0;

	xilinx_canfd_write32(dev, XCANFD_AFR_OFFSET, 0);

	for (int i = 0; i < XCANFD_MAX_FILTERS; i++) {
		xilinx_canfd_write32(dev, XCANFD_AFMR_ADDR(i), 0);
		xilinx_canfd_write32(dev, XCANFD_AFIDR_ADDR(i), 0);
	}

	data->common.started = false;
	data->common.mode = CAN_MODE_NORMAL;
	sys_slist_init(&data->common.state_change_callbacks);

	ret = xilinx_canfd_reset(dev);
	if (ret) {
		LOG_ERR("Failed to enter reset mode: %d", ret);
		return ret;
	}

	ret = can_calc_timing(dev, &timing, config->common.bitrate, config->common.sample_point);
	if (ret < 0) {
		LOG_ERR("Failed to calculate arbitration timing: %d", ret);
		return ret;
	}

	ret = xilinx_canfd_set_timing(dev, &timing);
	if (ret) {
		LOG_ERR("Error setting arbitration timing: %d", ret);
		return ret;
	}

#ifdef CONFIG_CAN_FD_MODE
	struct can_timing timing_data;

	ret = can_calc_timing_data(dev, &timing_data, config->common.bitrate_data,
				   config->common.sample_point_data);
	if (ret < 0) {
		LOG_ERR("Failed to calculate data phase timing: %d", ret);
		return ret;
	}

	ret = xilinx_canfd_set_timing_data(dev, &timing_data);
	if (ret) {
		LOG_ERR("Error setting data phase timing: %d", ret);
		return ret;
	}
#endif /* CONFIG_CAN_FD_MODE */

	xilinx_canfd_write32(dev, XCANFD_SRR_OFFSET, 0);

	if (!WAIT_FOR((xilinx_canfd_read32(dev, XCANFD_SR_OFFSET) & XCANFD_SR_CONFIG_MASK),
		      XCANFD_TIMEOUT_MS * USEC_PER_MSEC, k_busy_wait(1))) {
		LOG_ERR("Failed to enter configuration mode for interrupt setup");
		return -ETIMEDOUT;
	}

	xilinx_canfd_write32(dev, XCANFD_IER_OFFSET, 0);
	xilinx_canfd_write32(dev, XCANFD_ICR_OFFSET, XCANFD_ICR_CLEAR_ALL);
	config->init_func(dev);
	data->state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}

static DEVICE_API(can, xilinx_canfd_driver_api) = {
	.get_capabilities = xilinx_canfd_get_capabilities,
	.start = xilinx_canfd_start,
	.stop = xilinx_canfd_stop,
	.set_mode = xilinx_canfd_set_mode,
	.set_timing = xilinx_canfd_set_timing,
	.send = xilinx_canfd_send,
	.add_rx_filter = xilinx_canfd_add_rx_filter,
	.remove_rx_filter = xilinx_canfd_remove_rx_filter,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = xilinx_canfd_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_state = xilinx_canfd_get_state,
	.get_core_clock = xilinx_canfd_get_core_clock,
	.get_max_filters = xilinx_canfd_get_max_filters,
	.timing_min = {.sjw = XCANFD_TIMING_SJW_MIN,
		       .prop_seg = XCANFD_TIMING_PROP_SEG_MIN,
		       .phase_seg1 = XCANFD_TIMING_PHASE_SEG1_MIN,
		       .phase_seg2 = XCANFD_TIMING_PHASE_SEG2_MIN,
		       .prescaler = XCANFD_TIMING_PRESCALER_MIN},
	.timing_max = {.sjw = XCANFD_TIMING_SJW_MAX,
		       .prop_seg = XCANFD_TIMING_PROP_SEG_MAX,
		       .phase_seg1 = XCANFD_TIMING_PHASE_SEG1_MAX,
		       .phase_seg2 = XCANFD_TIMING_PHASE_SEG2_MAX,
		       .prescaler = XCANFD_TIMING_PRESCALER_MAX},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = xilinx_canfd_set_timing_data,
	.timing_data_min = {.sjw = XCANFD_TIMING_DATA_SJW_MIN,
			    .prop_seg = XCANFD_TIMING_DATA_PROP_SEG_MIN,
			    .phase_seg1 = XCANFD_TIMING_DATA_PHASE_SEG1_MIN,
			    .phase_seg2 = XCANFD_TIMING_DATA_PHASE_SEG2_MIN,
			    .prescaler = XCANFD_TIMING_DATA_PRESCALER_MIN},
	.timing_data_max = {.sjw = XCANFD_TIMING_DATA_SJW_MAX,
			    .prop_seg = XCANFD_TIMING_DATA_PROP_SEG_MAX,
			    .phase_seg1 = XCANFD_TIMING_DATA_PHASE_SEG1_MAX,
			    .phase_seg2 = XCANFD_TIMING_DATA_PHASE_SEG2_MAX,
			    .prescaler = XCANFD_TIMING_DATA_PRESCALER_MAX},
#endif
};

#define XCANFD_IRQ_CONFIG(n)                                                                       \
	static void xilinx_canfd_config_intr##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ_BY_IDX(n, 0, priority), xilinx_canfd_isr, \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* Device Instantiation */
#define xilinx_canfd_INST(n)                                                                       \
	XCANFD_IRQ_CONFIG(n)                                                                       \
	static const struct xilinx_canfd_cfg xilinx_canfd_cfg_##n = {                              \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(n, XCANFD_BUS_SPEED_MIN,                   \
							XCANFD_BUS_SPEED_MAX),                     \
		DEVICE_MMIO_NAMED_ROM_INIT(mmio, DT_DRV_INST(n)),                                  \
		.init_func = xilinx_canfd_config_intr##n,                                          \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
	};                                                                                         \
	static struct xilinx_canfd_data xilinx_canfd_data_##n;                                     \
	CAN_DEVICE_DT_INST_DEFINE(n, xilinx_canfd_init, NULL, &xilinx_canfd_data_##n,              \
				  &xilinx_canfd_cfg_##n, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,    \
				  &xilinx_canfd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(xilinx_canfd_INST)
