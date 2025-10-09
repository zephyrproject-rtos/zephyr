/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_canfd_2_0

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/can.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include "can_xilinx_canfd.h"
LOG_MODULE_REGISTER(xcanfd, CONFIG_CAN_LOG_LEVEL);

struct xcanfd_filter {
	can_rx_callback_t callback;
	void *callback_arg;
	struct can_filter filter;
	bool in_use;
};

typedef void (*init_func_t)(const struct device *dev);

struct xcanfd_cfg {
	const struct can_driver_config common;
	uint32_t reg_addr;
	int reg_size;
	init_func_t init_func;
	uint32_t can_core_clock;
};

struct xcanfd_tx_mailbox {
	can_tx_callback_t callback;
	void *callback_arg;
	bool in_use;
};

struct xcanfd_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	uint8_t tx_head;
	uint8_t tx_tail;
	uint8_t tx_sent;
	struct k_mutex rx_mutex;
	enum can_state state;
	struct xcanfd_tx_mailbox tx_mailboxes[XCANFD_MAX_TX_MAILBOXES];
	atomic_t tx_mailbox_mask;
	struct xcanfd_filter filters[XCANFD_MAX_FILTERS];
	uint32_t enabled_filters_mask;
};

/**
 * @brief Read a register value from the Xilinx CANFD controller
 */
static inline uint32_t xcanfd_read32(const struct xcanfd_cfg *config, uint32_t offs)
{
	return sys_read32(config->reg_addr + offs);
}

/**
 * @brief Write a value to a Xilinx CANFD controller register
 */
static inline void xcanfd_write32(const struct xcanfd_cfg *config, uint32_t offs, uint32_t value)
{
	sys_write32(value, config->reg_addr + offs);
}

/**
 * @brief Disable specified acceptance filters
 */
static void xcanfd_filter_disable(const struct xcanfd_cfg *config, uint32_t filter_mask)
{
	uint32_t filters;

	filters = xcanfd_read32(config, XCANFD_AFR_OFFSET);
	filters &= (~filter_mask);
	xcanfd_write32(config, XCANFD_AFR_OFFSET, filters);
}

/**
 * @brief Enable specified acceptance filters
 */
static void xcanfd_filter_enable(const struct xcanfd_cfg *config, uint32_t filter_mask)
{
	uint32_t filters;

	filters = xcanfd_read32(config, XCANFD_AFR_OFFSET);
	filters |= filter_mask;
	xcanfd_write32(config, XCANFD_AFR_OFFSET, filters);
}

/**
 * @brief Set acceptance filter mask and ID values
 */
static int xcanfd_acceptance_filter_set(const struct xcanfd_cfg *config,
					uint32_t filter_index,
					uint32_t mask_value,
					uint32_t id_value)
{
	uint32_t enabled_filters;

	if (filter_index < MIN_FILTER_INDEX || filter_index > MAX_FILTER_INDEX) {
		LOG_ERR("Invalid filter index: %u", filter_index);
		return -EINVAL;
	}

	enabled_filters = xcanfd_read32(config, XCANFD_AFR_OFFSET);
	if ((enabled_filters & BIT(filter_index - 1)) != 0) {
		LOG_ERR("Filter %u is currently enabled", filter_index);
		return -EBUSY;
	}

	filter_index--;

	xcanfd_write32(config, XCANFD_AFMR_ADDR(filter_index), mask_value);
	xcanfd_write32(config, XCANFD_AFIDR_ADDR(filter_index), id_value);

	return 0;
}

/**
 * @brief Convert Zephyr CAN filter to hardware filter format
 */
static void xcanfd_filter_to_hw_format(const struct can_filter *filter,
				       uint32_t *hw_mask, uint32_t *hw_id)
{
	*hw_id = 0;
	*hw_mask = 0;

	if (filter->flags & CAN_FILTER_IDE) {
		*hw_id |= (filter->id << XCANFD_AFIDR_AIID_EXT_SHIFT) &
			  XCANFD_AFIDR_AIID_EXT_MASK;
		*hw_id |= XCANFD_AFIDR_AIIDE_MASK;
		*hw_mask |= (filter->mask << XCANFD_AFMR_AMID_EXT_SHIFT) &
			    XCANFD_AFMR_AMID_EXT_MASK;
		*hw_mask |= XCANFD_AFMR_AMIDE_MASK;
		*hw_mask |= XCANFD_AFMR_AMRTR_MASK;

	} else {
		*hw_id |= (filter->id << XCANFD_AFIDR_AIID_SHIFT) &
			  XCANFD_AFIDR_AIID_MASK;
		*hw_mask |= (filter->mask << XCANFD_AFMR_AMID_SHIFT) &
			    XCANFD_AFMR_AMID_MASK;
		*hw_mask |= XCANFD_AFMR_AMIDE_MASK;
		*hw_mask |= XCANFD_AFMR_AMSRR_MASK;
	}
}

/**
 * @brief Check if a received frame matches any enabled filter
 */
static bool xcanfd_frame_matches_filter(const struct device *dev,
					const struct can_frame *frame,
					struct xcanfd_filter **matched_filter)
{
	uint32_t masked_frame_id, masked_filter_id;
	struct xcanfd_data *data = dev->data;
	bool frame_is_ext, filter_is_ext;
	int i;

	for (i = 0; i < XCANFD_MAX_FILTERS; i++) {
		if (!data->filters[i].in_use) {
			continue;
		}

		if (!(data->enabled_filters_mask & BIT(i))) {
			continue;
		}

		frame_is_ext = !!(frame->flags & CAN_FRAME_IDE);
		filter_is_ext = !!(data->filters[i].filter.flags & CAN_FILTER_IDE);

		if (frame_is_ext != filter_is_ext) {
			continue;
		}

		masked_frame_id = frame->id & data->filters[i].filter.mask;
		masked_filter_id = data->filters[i].filter.id &
				   data->filters[i].filter.mask;

		if (masked_frame_id == masked_filter_id) {
			*matched_filter = &data->filters[i];
			return true;
		}
	}

	return false;
}

/**
 * @brief Handle sleep and wake-up state change interrupts
 */
static void xcanfd_state_interrupt(const struct device *dev, uint32_t isr)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t msr;

	if (isr & XCANFD_IXR_SLP_MASK) {
		data->state = CAN_STATE_STOPPED;
		msr = xcanfd_read32(config, XCANFD_MSR_OFFSET);

		if (!(msr & XCANFD_MSR_SLEEP_MASK)) {
			xcanfd_write32(config, XCANFD_MSR_OFFSET, msr | XCANFD_MSR_SLEEP_MASK);
		}
		xcanfd_write32(config, XCANFD_SRR_OFFSET, 0);
	}

	if (isr & XCANFD_IXR_WKUP_MASK) {
		data->state = CAN_STATE_ERROR_ACTIVE;
		xcanfd_write32(config, XCANFD_SRR_OFFSET, XCANFD_SRR_CEN_MASK);
	}
}

/**
 * @brief Handle transmission complete interrupts
 */
static void xcanfd_tx_interrupt(const struct device *dev, uint32_t isr)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t trr_reg;
	int mailbox_id;

	xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_TXOK_MASK);

	trr_reg = xcanfd_read32(config, XCANFD_TRR_OFFSET);

	for (mailbox_id = 0; mailbox_id < XCANFD_MAX_TX_MAILBOXES; mailbox_id++) {
		if ((atomic_get(&data->tx_mailbox_mask) & BIT(mailbox_id)) &&
		    !(trr_reg & BIT(mailbox_id))) {
			can_tx_callback_t callback = data->tx_mailboxes[mailbox_id].callback;
			void *callback_arg = data->tx_mailboxes[mailbox_id].callback_arg;

			data->tx_mailboxes[mailbox_id].in_use = false;
			data->tx_mailboxes[mailbox_id].callback = NULL;
			data->tx_mailboxes[mailbox_id].callback_arg = NULL;
			atomic_and(&data->tx_mailbox_mask, ~BIT(mailbox_id));

			data->tx_tail++;
			data->tx_sent++;
			if (callback) {
				callback(dev, 0, callback_arg);
			}
		}
	}
}

/**
 * @brief Handle frame reception interrupts and process received frames
 */
static void xcanfd_rx_interrupt(const struct device *dev, uint32_t isr)
{
	const struct xcanfd_cfg *config = dev->config;
	struct can_frame frame = {0};
	bool dlc_invalid = false;
	uint32_t raw_id_reg;
	uint32_t fsr, mask;
	uint32_t dw_offset;
	uint32_t reg_data;
	uint32_t id1_part;
	uint32_t id2_part;
	uint32_t dlc_reg;
	int copy_len;
	int offset;
	int len;

	fsr = xcanfd_read32(config, XCANFD_FSR_OFFSET);

	mask = XCANFD_2_FSR_FL_MASK;

	if (fsr & mask) {
		offset = XCANFD_RXMSG_2_FRAME_ADDR(fsr & XCANFD_2_FSR_RI_MASK);
		raw_id_reg = xcanfd_read32(config, XCANFD_FRAME_ID_ADDR(offset));
		dlc_reg = xcanfd_read32(config, XCANFD_FRAME_DLC_ADDR(offset));

		if (dlc_reg & XCANFD_DLCR_EDL_MASK) {
			frame.flags |= CAN_FRAME_FDF;
		}

		if (frame.flags & CAN_FRAME_IDE) {
			if (raw_id_reg & XCANFD_IDR_RTR_MASK) {
				frame.flags |= CAN_FRAME_RTR;
			}
		} else {
			if (raw_id_reg & XCANFD_IDR_SRR_MASK) {
				frame.flags |= CAN_FRAME_RTR;
			}
		}

		if (raw_id_reg & XCANFD_IDR_IDE_MASK) {
			frame.flags |= CAN_FRAME_IDE;
		}

		if (frame.flags & CAN_FRAME_IDE) {
			id1_part = (raw_id_reg & XCANFD_IDR_ID1_MASK) >> XCANFD_IDR_ID1_SHIFT;
			id2_part = (raw_id_reg & XCANFD_IDR_ID2_MASK) >> XCANFD_IDR_ID2_SHIFT;

			frame.id = (id1_part << 18) | id2_part;
		} else {
			frame.id = (raw_id_reg & XCANFD_IDR_ID1_MASK) >> XCANFD_IDR_ID1_SHIFT;
		}

		if (dlc_reg & XCANFD_DLCR_BRS_MASK) {
			frame.flags |= CAN_FRAME_BRS;
		}

		if (dlc_reg & XCANFD_DLCR_ESI_MASK) {
			frame.flags |= CAN_FRAME_ESI;
		}

		frame.dlc = (dlc_reg & XCANFD_DLCR_DLC_MASK) >> XCANFD_DLCR_DLC_SHIFT;

		if ((frame.flags & CAN_FRAME_FDF) != 0) {
			if (frame.dlc > CANFD_MAX_DLC) {
				LOG_ERR("Invalid CANFD DLC: %d (max %d)",
					frame.dlc, CANFD_MAX_DLC);
				dlc_invalid = true;
			}
		} else {
			if (frame.dlc > CAN_MAX_DLC) {
				LOG_ERR("Invalid classic CAN DLC: %d (max %d)",
					frame.dlc, CAN_MAX_DLC);
				dlc_invalid = true;
			}
		}

		if (dlc_invalid) {
			xcanfd_write32(config, XCANFD_FSR_OFFSET,
				       xcanfd_read32(config, XCANFD_FSR_OFFSET) |
				       XCANFD_FSR_IRI_MASK);
			return;
		}

		len = can_dlc_to_bytes(frame.dlc);

		if (len > 0 && len <= sizeof(frame.data)) {
			dw_offset = XCANFD_FRAME_DW_ADDR(offset);
			for (int i = 0; i < len; i += 4) {
				reg_data = xcanfd_read32(config, dw_offset);
				reg_data = BSWAP_32(reg_data);
				copy_len = MIN(4, len - i);
				memcpy(&frame.data[i], &reg_data, copy_len);
				dw_offset += XCANFD_DW_BYTES;
			}
		}

		if (frame.flags & CAN_FRAME_RTR) {
			return;
		}

		struct xcanfd_filter *matched_filter = NULL;

		if (xcanfd_frame_matches_filter(dev, &frame, &matched_filter)) {
			matched_filter->callback(dev, &frame, matched_filter->callback_arg);
		} else {
			return;
		}

		xcanfd_write32(config, XCANFD_FSR_OFFSET,
			       xcanfd_read32(config, XCANFD_FSR_OFFSET) | XCANFD_FSR_IRI_MASK);
	}
	xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_RXOK_MASK);
}

/**
 * @brief Main interrupt service routine for Xilinx CANFD controller
 */
static void xcanfd_isr(const struct device *dev)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t isr;

	if (data->state == CAN_STATE_STOPPED && !data->common.started) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, 0xFFFFFFFF);
		return;
	}

	isr = xcanfd_read32(config, XCANFD_ISR_OFFSET);
	if (!isr) {
		return;
	}

	if (isr & (XCANFD_IXR_SLP_MASK | XCANFD_IXR_WKUP_MASK)) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, (XCANFD_IXR_SLP_MASK |
			       XCANFD_IXR_WKUP_MASK));
		xcanfd_state_interrupt(dev, isr);
	}

	if (isr & XCANFD_IXR_ARBLST_MASK) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_ARBLST_MASK);
		unsigned int key = irq_lock();

		for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
			if (data->tx_mailboxes[i].in_use &&
			    data->tx_mailboxes[i].callback) {
				can_tx_callback_t callback = data->tx_mailboxes[i].callback;
				void *callback_arg = data->tx_mailboxes[i].callback_arg;

				data->tx_mailboxes[i].in_use = false;
				data->tx_mailboxes[i].callback = NULL;
				data->tx_mailboxes[i].callback_arg = NULL;
				atomic_and(&data->tx_mailbox_mask, ~BIT(i));

				irq_unlock(key);
				callback(dev, -ENETUNREACH, callback_arg);
				key = irq_lock();
			}
		}
		irq_unlock(key);
	}

	if (isr & XCANFD_IXR_TXOK_MASK) {
		xcanfd_tx_interrupt(dev, isr);
	}

	if (isr & XCANFD_IXR_ERROR_MASK) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_ERROR_MASK);
	}

	if (isr & XCANFD_IXR_BSOFF_MASK) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_BSOFF_MASK);

		data->state = CAN_STATE_BUS_OFF;
		unsigned int key = irq_lock();

		for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
			if (data->tx_mailboxes[i].in_use &&
			    data->tx_mailboxes[i].callback) {
				can_tx_callback_t callback = data->tx_mailboxes[i].callback;
				void *callback_arg = data->tx_mailboxes[i].callback_arg;

				data->tx_mailboxes[i].in_use = false;
				data->tx_mailboxes[i].callback = NULL;
				data->tx_mailboxes[i].callback_arg = NULL;
				atomic_and(&data->tx_mailbox_mask, ~BIT(i));

				irq_unlock(key);
				callback(dev, -ENETUNREACH, callback_arg);
				key = irq_lock();
			}
		}
		irq_unlock(key);
	}

	if (isr & XCANFD_IXR_RXOK_MASK) {
		xcanfd_rx_interrupt(dev, isr);
	}

	if (isr & XCANFD_IXR_RXNEMP_MASK) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_RXNEMP_MASK);
		xcanfd_rx_interrupt(dev, isr);
	}
}

/**
 * @brief Get the supported CAN controller capabilities
 */
static int xcanfd_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);
	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_FD;
	return 0;
}

/**
 * @brief Set the CAN controller operating mode
 */
static int xcanfd_set_mode(const struct device *dev, can_mode_t mode)
{
	can_mode_t supported = CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_NORMAL;
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t msr_reg = 0;
	uint32_t brpr = 0;
	int ret = 0;

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		supported |= CAN_MODE_FD;
	}

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

	xcanfd_write32(config, XCANFD_SRR_OFFSET, 0);

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		msr_reg |= XCANFD_MSR_LBACK_MASK;
	} else if ((mode & CAN_MODE_LISTENONLY) != 0) {
		msr_reg |= XCANFD_MSR_SNOOP_MASK;
	}

	if ((mode & CAN_MODE_FD) != 0) {
		if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
			brpr = xcanfd_read32(config, XCANFD_BRPR_OFFSET);
			brpr |= XCANFD_BRPR_TDC_ENABLE_MASK;
			xcanfd_write32(config, XCANFD_BRPR_OFFSET, brpr);
		} else {
			LOG_ERR("CONFIG_CAN_FD_MODE is not enabled");
			ret = -ENOTSUP;
			goto unlock;
		}
	}

	xcanfd_write32(config, XCANFD_MSR_OFFSET, msr_reg);
	xcanfd_write32(config, XCANFD_SRR_OFFSET, XCANFD_SRR_CEN_MASK);
	data->common.mode = mode;

unlock:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

/**
 * @brief Start the CAN controller operation
 */
static int xcanfd_start(const struct device *dev)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t status_reg, ier;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		LOG_ERR("CAN controller already started");
		k_mutex_unlock(&data->inst_mutex);
		return -EALREADY;
	}

	CAN_STATS_RESET(dev);

	data->tx_head = 0;
	data->tx_tail = 0;
	data->tx_sent = 0;

	for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
		data->tx_mailboxes[i].in_use = false;
		data->tx_mailboxes[i].callback = NULL;
		data->tx_mailboxes[i].callback_arg = NULL;
	}
	atomic_set(&data->tx_mailbox_mask, 0);

	status_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
	ret = xcanfd_set_mode(dev, data->common.mode);

	if (ret != 0) {
		LOG_ERR("Failed to enter configured mode %d (err %d)", data->common.mode, ret);
		k_mutex_unlock(&data->inst_mutex);
		return ret;
	}

	status_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
	data->common.started = true;
	data->state = CAN_STATE_ERROR_ACTIVE;

	ier = XCANFD_IXR_TXOK_MASK | XCANFD_IXR_WKUP_MASK | XCANFD_IXR_SLP_MASK |
	      XCANFD_IXR_RXOK_MASK | XCANFD_IXR_RXNEMP_MASK | XCANFD_IXR_ERROR_MASK |
	      XCANFD_IXR_BSOFF_MASK | XCANFD_IXR_ARBLST_MASK;

	xcanfd_write32(config, XCANFD_IER_OFFSET, ier);

	xcanfd_filter_disable(config, XCANFD_AFR_UAF_ALL_MASK);
	if (data->enabled_filters_mask != 0) {
		xcanfd_filter_enable(config, data->enabled_filters_mask);
	}

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

/**
 * @brief Stop the CAN controller operation
 */
static int xcanfd_stop(const struct device *dev)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	int64_t start_time, timeout_ms = 1;
	uint32_t status_reg;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->common.started) {
		k_mutex_unlock(&data->inst_mutex);
		return -EALREADY;
	}

	xcanfd_write32(config, XCANFD_SRR_OFFSET, 0);

	start_time = k_uptime_get();

	while (k_uptime_delta(&start_time) < timeout_ms) {
		status_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
		if (status_reg & XCANFD_SR_CONFIG_MASK) {
			break;
		}
		/* Small delay needed for CAN controller core reset to complete */
		k_busy_wait(1);
	}

	if (k_uptime_delta(&start_time) >= timeout_ms) {
		LOG_ERR("Timeout waiting for configuration mode");
		ret = -ETIMEDOUT;
	}

	xcanfd_write32(config, XCANFD_IER_OFFSET, 0);

	for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
		if (data->tx_mailboxes[i].in_use) {
			if (data->tx_mailboxes[i].callback) {
				data->tx_mailboxes[i].callback(dev, -ENETDOWN,
					data->tx_mailboxes[i].callback_arg);
			}
			data->tx_mailboxes[i].in_use = false;
			data->tx_mailboxes[i].callback = NULL;
			data->tx_mailboxes[i].callback_arg = NULL;
		}
	}
	atomic_set(&data->tx_mailbox_mask, 0);

	data->tx_head = 0;
	data->tx_tail = 0;
	data->tx_sent = 0;
	data->common.started = false;
	data->state = CAN_STATE_STOPPED;

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

/**
 * @brief Set CANFD data phase bit timing parameters
 */
static int xcanfd_set_timing_data(const struct device *dev,
				  const struct can_timing *timing)
{
	if (!IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		ARG_UNUSED(dev);
		ARG_UNUSED(timing);
		return -ENOTSUP;
	}

	struct xcanfd_data *data = dev->data;
	struct can_timing calc_timing_data;
	uint32_t is_config_mode;
	uint32_t btr0, btr1;
	int ret;

	const struct xcanfd_cfg *config = dev->config;

	if (data->common.started) {
		LOG_ERR("Cannot set bit timing data while CAN controller is started");
		return -EBUSY;
	}

#ifdef CONFIG_CAN_FD_MODE
	uint32_t requested_bitrate = config->common.bitrate_data;

	if (requested_bitrate < config->common.min_bitrate ||
	    requested_bitrate > config->common.max_bitrate) {
		LOG_ERR("Requested bitrate (%u) out of range [%u, %u]",
			requested_bitrate, config->common.min_bitrate,
			config->common.max_bitrate);
		return -EINVAL;
	}

	ret = can_calc_timing_data(dev, &calc_timing_data, config->common.bitrate_data,
				   config->common.sample_point_data);
#else
	if (timing) {
		calc_timing_data = *timing;
		ret = 0;
	} else {
		LOG_ERR("Timing parameter required for non-FD mode");
		return -EINVAL;
	}
#endif

	if (ret == -EINVAL) {
		LOG_ERR("Failed to calculate valid timing parameters");
		return -EIO;
	}

	is_config_mode = xcanfd_read32(config, XCANFD_SR_OFFSET) & XCANFD_SR_CONFIG_MASK;
	if (!is_config_mode) {
		LOG_ERR("Cannot set bit timing - CANFD is not in config mode");
		return -EIO;
	}

	if (ret == 0) {
		btr0 = (calc_timing_data.prescaler - 1) & XCANFD_BRPR_BRP_MASK;
		btr1 = ((calc_timing_data.prop_seg + calc_timing_data.phase_seg1 - 1)) &
			XCANFD_BTR_TS1_MASK_CANFD;
		btr1 |= (((calc_timing_data.phase_seg2 - 1) << XCANFD_BTR_TS2_SHIFT_CANFD) &
			 XCANFD_BTR_TS2_MASK_CANFD);
		btr1 |= (((calc_timing_data.sjw - 1) << XCANFD_BTR_SJW_SHIFT_CANFD) &
			 XCANFD_BTR_SJW_MASK_CANFD);

		xcanfd_write32(config, XCANFD_F_BRPR_OFFSET, btr0);
		xcanfd_write32(config, XCANFD_F_BTR_OFFSET, btr1);
	}
	return 0;
}

/**
 * @brief Set CAN arbitration phase bit timing parameters
 */
static int xcanfd_set_timing(const struct device *dev,
			     const struct can_timing *timing)
{
	struct xcanfd_data *data = dev->data;
	struct can_timing calc_timing;
	uint32_t is_config_mode;
	uint32_t btr0, btr1;
	int ret;

	const struct xcanfd_cfg *config = dev->config;
	uint32_t requested_bitrate = config->common.bitrate;

	if (data->common.started) {
		LOG_ERR("Cannot set bit timing while CANFD controller is started");
		return -EBUSY;
	}

	if (requested_bitrate < config->common.min_bitrate ||
	    requested_bitrate > config->common.max_bitrate) {
		LOG_ERR("Requested bitrate (%u) out of range [%u, %u]", requested_bitrate,
			config->common.min_bitrate, config->common.max_bitrate);
		return -EINVAL;
	}

	is_config_mode = xcanfd_read32(config, XCANFD_SR_OFFSET) & XCANFD_SR_CONFIG_MASK;
	if (!is_config_mode) {
		LOG_ERR("Cannot set bit timing - CANFD is not in config mode");
		return -EIO;  /* General I/O error */
	}

	ret = can_calc_timing(dev, &calc_timing, config->common.bitrate,
			      config->common.sample_point);

	if (ret == -EINVAL) {
		LOG_ERR("Failed to calculate valid timing parameters");
		return -EIO;
	}

	if (ret == 0) {
		btr0 = (calc_timing.prescaler - 1) & XCANFD_BRPR_BRP_MASK;
		btr1 = ((calc_timing.prop_seg + calc_timing.phase_seg1 - 1)) &
			XCANFD_BTR_TS1_MASK;
		btr1 |= (((calc_timing.phase_seg2 - 1) << XCANFD_BTR_TS2_SHIFT) &
			 XCANFD_BTR_TS2_MASK);
		btr1 |= (((calc_timing.sjw - 1) << XCANFD_BTR_SJW_SHIFT) &
			 XCANFD_BTR_SJW_MASK);
		xcanfd_write32(config, XCANFD_BRPR_OFFSET, btr0);
		xcanfd_write32(config, XCANFD_BTR_OFFSET, btr1);
	}
	return 0;
}

/**
 * @brief Set callback function for CANFD controller state changes
 */
static void xcanfd_set_state_change_callback(const struct device *dev,
					     can_state_change_callback_t cb,
					     void *user_data)
{
	struct xcanfd_data *data = dev->data;

	data->common.state_change_cb = cb;
	data->common.state_change_cb_user_data = user_data;
}

/**
 * @brief Get current CANFD controller state and error counters
 */
static int xcanfd_get_state(const struct device *dev, enum can_state *state,
			    struct can_bus_err_cnt *err_cnt)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;

	if (state) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			*state = data->state;
		}
	}

	if (err_cnt) {
		err_cnt->tx_err_cnt = xcanfd_read32(config, XCANFD_ECR_OFFSET) &
					XCANFD_ECR_TEC_MASK;
		err_cnt->rx_err_cnt = ((xcanfd_read32(config, XCANFD_ECR_OFFSET) &
					XCANFD_ECR_REC_MASK) >> XCANFD_ECR_REC_SHIFT);
	}
	return 0;
}

/**
 * @brief Send a CANFD frame
 */
static int xcanfd_send(const struct device *dev, const struct can_frame *frame,
		       k_timeout_t timeout, can_tx_callback_t callback,
		       void *user_data)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t value = 0, dwindex = 0;
	uint32_t upper_11_bits;
	uint32_t lower_18_bits;
	uint32_t frame_offset;
	uint32_t nobytes = 0;
	uint32_t dlc_reg = 0;
	uint32_t trr_reg = 0;
	int mailbox_id = -1;
	uint32_t id_reg = 0;

	if (!frame) {
		LOG_ERR("Frame pointer is NULL");
		return -EINVAL;
	}

	if (!data->common.started) {
		LOG_ERR("CANFD controller not started");
		return -ENETDOWN;
	}
	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		if ((frame->flags & ~(CAN_FRAME_IDE |
			CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
			LOG_ERR("Unsupported CANFD frame flags 0x%02x", frame->flags);
			return -ENOTSUP;
		}

		if ((frame->flags & CAN_FRAME_FDF) != 0) {
			if ((data->common.mode & CAN_MODE_FD) == 0U) {
				LOG_ERR("CANFD frame not supported in current mode");
				return -ENOTSUP;
			}
		}
	} else {
		if ((frame->flags & ~(CAN_FRAME_IDE)) != 0) {
			LOG_ERR("Unsupported CANFD frame flags 0x%02x", frame->flags);
			return -ENOTSUP;
		}
	}

	if (frame->dlc > 15) {
		LOG_ERR("DLC %d is invalid (max possible is 15)", frame->dlc);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		if ((frame->flags & CAN_FRAME_FDF) != 0) {
			if (frame->dlc > CANFD_MAX_DLC) {
				LOG_ERR("CANFD DLC of %d exceeds maximum (%d)",
					frame->dlc, CANFD_MAX_DLC);
				return -EINVAL;
			}
		} else {
			if (frame->dlc > CAN_MAX_DLC) {
				LOG_ERR("Classic CANFD DLC of %d exceeds maximum (%d)",
					frame->dlc, CAN_MAX_DLC);
				return -EINVAL;
			}
		}
	} else {
		if (frame->dlc > CAN_MAX_DLC) {
			LOG_ERR("Classic CAN DLC of %d exceeds maximum (%d)",
				frame->dlc, CAN_MAX_DLC);
			return -EINVAL;
		}
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	trr_reg = xcanfd_read32(config, XCANFD_TRR_OFFSET);

	for (int i = 0; i < XCANFD_MAX_TX_MAILBOXES; i++) {
		if (!data->tx_mailboxes[i].in_use && !(trr_reg & BIT(i))) {
			mailbox_id = i;
			break;
		}
	}

	if (mailbox_id == -1) {
		k_mutex_unlock(&data->inst_mutex);
		return -EBUSY;
	}

	data->tx_mailboxes[mailbox_id].in_use = true;
	data->tx_mailboxes[mailbox_id].callback = callback;
	data->tx_mailboxes[mailbox_id].callback_arg = user_data;
	atomic_or(&data->tx_mailbox_mask, BIT(mailbox_id));

	frame_offset = XCANFD_TXMSG_FRAME_ADDR(mailbox_id);
	data->tx_head++;

	if (frame->flags & CAN_FRAME_IDE) {
		if (frame->id > CAN_EXT_ID_MASK) {
			LOG_ERR("TX: Extended ID 0x%08x exceeds maximum 0x%08x",
				frame->id, CAN_EXT_ID_MASK);
		}

		upper_11_bits = (frame->id >> 18) & 0x7FF;    /* Extract bits 28:18 */
		lower_18_bits = frame->id & 0x3FFFF;          /* Extract bits 17:0 */

		id_reg = (upper_11_bits << XCANFD_IDR_ID1_SHIFT) |
			(lower_18_bits << XCANFD_IDR_ID2_SHIFT) |
			XCANFD_IDR_IDE_MASK;
	} else {
		if (frame->id > CAN_STD_ID_MASK) {
			LOG_ERR("TX: Standard ID 0x%08x exceeds maximum 0x%08x",
				frame->id, CAN_STD_ID_MASK);
		}

		id_reg = (frame->id << XCANFD_IDR_ID1_SHIFT) & XCANFD_IDR_ID1_MASK;
	}

	if (frame->flags & CAN_FRAME_RTR) {
		id_reg |= XCANFD_IDR_RTR_MASK;
	}
	dlc_reg = (frame->dlc << XCANFD_DLCR_DLC_SHIFT) & XCANFD_DLCR_DLC_MASK;

	if (frame->flags & CAN_FRAME_FDF) {
		dlc_reg |= XCANFD_DLCR_EDL_MASK;
	}
	if (frame->flags & CAN_FRAME_BRS) {
		dlc_reg |= XCANFD_DLCR_BRS_MASK;
	}
	if (frame->flags & CAN_FRAME_ESI) {
		dlc_reg |= XCANFD_DLCR_ESI_MASK;
	}

	xcanfd_write32(config, XCANFD_FRAME_ID_ADDR(frame_offset), id_reg);
	xcanfd_write32(config, XCANFD_FRAME_DLC_ADDR(frame_offset), dlc_reg);

	nobytes = can_dlc_to_bytes(frame->dlc);

	for (uint32_t len = 0; len < nobytes; len += 4) {
		value = BSWAP_32(frame->data_32[len / 4]);
		xcanfd_write32(config, (XCANFD_FRAME_DW_ADDR(frame_offset) +
				(dwindex * XCANFD_DW_BYTES)), value);
		dwindex++;
	}
	xcanfd_write32(config, XCANFD_TRR_OFFSET, BIT(mailbox_id));

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

/**
 * @brief Add a receive filter and callback for incoming CAN and CANFD frames
 */
static int xcanfd_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
				void *cb_arg, const struct can_filter *filter)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t hw_mask, hw_id;
	int filter_index = -1;
	int ret = 0;
	int i;

	if (!cb) {
		LOG_ERR("Receive callback function cannot be NULL");
		return -EINVAL;
	}

	if (!filter) {
		LOG_ERR("Filter cannot be NULL");
		return -EINVAL;
	}

	if (filter->flags & CAN_FILTER_IDE) {
		if (filter->id > CAN_EXT_ID_MASK) {
			LOG_ERR("Extended ID 0x%08x exceeds maximum 0x%08x",
				filter->id, CAN_EXT_ID_MASK);
			return -EINVAL;
		}
		if (filter->mask > CAN_EXT_ID_MASK) {
			LOG_ERR("Extended mask 0x%08x exceeds maximum 0x%08x",
				filter->mask, CAN_EXT_ID_MASK);
			return -EINVAL;
		}
	} else {
		if (filter->id > CAN_STD_ID_MASK) {
			LOG_ERR("Standard ID 0x%03x exceeds maximum 0x%03x",
				filter->id, CAN_STD_ID_MASK);
			return -EINVAL;
		}
		if (filter->mask > CAN_STD_ID_MASK) {
			LOG_ERR("Standard mask 0x%03x exceeds maximum 0x%03x",
				filter->mask, CAN_STD_ID_MASK);
			return -EINVAL;
		}
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	for (i = 0; i < XCANFD_MAX_FILTERS; i++) {
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

	xcanfd_filter_to_hw_format(filter, &hw_mask, &hw_id);

	ret = xcanfd_acceptance_filter_set(config, filter_index + 1, hw_mask, hw_id);
	if (ret != 0) {
		LOG_ERR("Failed to set hardware filter %d", filter_index);
		goto unlock;
	}

	data->filters[filter_index].callback = cb;
	data->filters[filter_index].callback_arg = cb_arg;
	data->filters[filter_index].filter = *filter;
	data->filters[filter_index].in_use = true;

	xcanfd_filter_enable(config, BIT(filter_index));
	data->enabled_filters_mask |= BIT(filter_index);

	ret = filter_index;

unlock:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

/**
 * @brief Remove a previously registered receive filter
 */
static void xcanfd_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;

	if (filter_id < 0 || filter_id >= XCANFD_MAX_FILTERS) {
		LOG_ERR("Invalid filter ID: %d", filter_id);
		return;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->filters[filter_id].in_use) {
		LOG_WRN("Filter %d is not in use", filter_id);
		goto unlock;
	}

	xcanfd_filter_disable(config, BIT(filter_id));
	data->enabled_filters_mask &= ~BIT(filter_id);

	memset(&data->filters[filter_id], 0, sizeof(data->filters[filter_id]));
	data->filters[filter_id].in_use = false;

unlock:
	k_mutex_unlock(&data->inst_mutex);
}

/**
 * @brief Put the CAN controller into reset/configuration mode
 */
static int xcanfd_reset(const struct device *dev)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	int64_t start_time, timeout_ms = 1;
	uint32_t sr_reg;

	xcanfd_write32(config, XCANFD_IER_OFFSET, 0);
	xcanfd_write32(config, XCANFD_ICR_OFFSET, 0xFFFFFFFF);
	xcanfd_write32(config, XCANFD_SRR_OFFSET, XCANFD_SRR_RESET_MASK);

	start_time = k_uptime_get();

	while (k_uptime_delta(&start_time) < timeout_ms) {
		sr_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
		if (sr_reg & XCANFD_SR_CONFIG_MASK) {
			break;
		}
		/* Small delay needed for CAN controller core reset to complete */
		k_busy_wait(1);
	}

	if (k_uptime_delta(&start_time) >= timeout_ms) {
		LOG_ERR("Timeout exceeded while waiting for configuration mode (SR=0x%08x)",
			xcanfd_read32(config, XCANFD_SR_OFFSET));
		return -ETIMEDOUT;
	}
	xcanfd_write32(config, XCANFD_SRR_OFFSET, 0);
	data->tx_head = 0;
	data->tx_tail = 0;
	return 0;
}

/**
 * @brief Get the CAN controller core clock frequency
 */
static int xcanfd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct xcanfd_cfg *config = dev->config;
	*rate = DIV_ROUND_CLOSEST(config->can_core_clock, 1000000U) * 1000000U;
	return 0;
}

/**
 * @brief Get the maximum number of receive filters supported
 */
static int xcanfd_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);
	return XCANFD_MAX_FILTERS;
}

/**
 * @brief Initialize the Xilinx CANFD controller
 */
static int xcanfd_init(const struct device *dev)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	int64_t start_time, timeout_ms = 1;
	struct can_timing timing = { 0 };
	uint32_t status_reg;
	int ret;

	k_mutex_init(&data->inst_mutex);
	k_mutex_init(&data->rx_mutex);

	data->tx_head = 0;
	data->tx_tail = 0;
	data->tx_sent = 0;

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

	xcanfd_write32(config, XCANFD_AFR_OFFSET, 0);

	for (int i = 0; i < XCANFD_MAX_FILTERS; i++) {
		xcanfd_write32(config, XCANFD_AFMR_ADDR(i), 0);
		xcanfd_write32(config, XCANFD_AFIDR_ADDR(i), 0);
	}

	data->common.started = false;
	data->common.mode = CAN_MODE_NORMAL;
	data->common.state_change_cb = NULL;
	data->common.state_change_cb_user_data = NULL;

	ret = xcanfd_reset(dev);
	if (ret) {
		LOG_ERR("Failed to enter reset mode: %d", ret);
		return ret;
	}

	ret = xcanfd_set_timing(dev, &timing);
	if (ret) {
		LOG_ERR("Error setting arbitration timing: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		ret = xcanfd_set_timing_data(dev, &timing);
		if (ret) {
			LOG_ERR("Error setting data phase timing: %d", ret);
			return ret;
		}
	}

	xcanfd_write32(config, XCANFD_SRR_OFFSET, 0);

	start_time = k_uptime_get();

	while (k_uptime_delta(&start_time) < timeout_ms) {
		status_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
		if (status_reg & XCANFD_SR_CONFIG_MASK) {
			break;
		}
		/* Small delay needed for CAN controller core reset to complete */
		k_busy_wait(1);
	}

	if (k_uptime_delta(&start_time) >= timeout_ms) {
		LOG_ERR("Failed to enter configuration mode for interrupt setup");
		return -ETIMEDOUT;
	}

	xcanfd_write32(config, XCANFD_IER_OFFSET, 0);
	xcanfd_write32(config, XCANFD_ICR_OFFSET, 0xFFFFFFFF);
	config->init_func(dev);
	data->state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}

static const struct can_driver_api xcanfd_driver_api = {
	.get_capabilities = xcanfd_get_capabilities,
	.start = xcanfd_start,
	.stop = xcanfd_stop,
	.set_mode = xcanfd_set_mode,
	.set_timing = xcanfd_set_timing,
	.send = xcanfd_send,
	.add_rx_filter = xcanfd_add_rx_filter,
	.remove_rx_filter = xcanfd_remove_rx_filter,
	.get_state = xcanfd_get_state,
	.set_state_change_callback = xcanfd_set_state_change_callback,
	.get_core_clock = xcanfd_get_core_clock,
	.get_max_filters = xcanfd_get_max_filters,
	.timing_min = {
		.sjw = XCANFD_TIMING_SJW_MIN,
		.prop_seg = XCANFD_TIMING_PROP_SEG_MIN,
		.phase_seg1 = XCANFD_TIMING_PHASE_SEG1_MIN,
		.phase_seg2 = XCANFD_TIMING_PHASE_SEG2_MIN,
		.prescaler = XCANFD_TIMING_PRESCALER_MIN
	},
	.timing_max = {
		.sjw = XCANFD_TIMING_SJW_MAX,
		.prop_seg = XCANFD_TIMING_PROP_SEG_MAX,
		.phase_seg1 = XCANFD_TIMING_PHASE_SEG1_MAX,
		.phase_seg2 = XCANFD_TIMING_PHASE_SEG2_MAX,
		.prescaler = XCANFD_TIMING_PRESCALER_MAX,
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = xcanfd_set_timing_data,
	.timing_data_min = {
		.sjw = XCANFD_TIMING_SJW_MIN,
		.prop_seg = XCANFD_TIMING_PROP_SEG_MIN,
		.phase_seg1 = XCANFD_TIMING_PHASE_SEG1_MIN,
		.phase_seg2 = XCANFD_TIMING_PHASE_SEG2_MIN,
		.prescaler = XCANFD_TIMING_PRESCALER_MIN
	},
	.timing_data_max = {
		.sjw = XCANFD_TIMING_SJW_MAX,
		.prop_seg = XCANFD_TIMING_PROP_SEG_MAX,
		.phase_seg1 = XCANFD_TIMING_PHASE_SEG1_MAX,
		.phase_seg2 = XCANFD_TIMING_PHASE_SEG2_MAX,
		.prescaler = XCANFD_TIMING_PRESCALER_MAX,
	}
#endif
};

#define XCANFD_IRQ_CONFIG(n)						\
	static void xcanfd_config_intr##n(const struct device *dev)	\
{									\
	IRQ_CONNECT(DT_INST_IRQN(n),					\
		    DT_INST_IRQ_BY_IDX(n, 0, priority),			\
		    xcanfd_isr,						\
		    DEVICE_DT_INST_GET(n), 0);				\
	irq_enable(DT_INST_IRQN(n));					\
}

/* Device Instantiation */
#define xcanfd_INST(n)							\
	BUILD_ASSERT(DT_INST_REG_ADDR(n) != 0,				\
			"Invalid register base address");		\
	BUILD_ASSERT(DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency)\
		!= 0, "Invalid CAN core clock frequency");		\
	XCANFD_IRQ_CONFIG(n)						\
	static const struct xcanfd_cfg xcanfd_cfg_##n = {		\
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(n,		\
				XLNX_CANFD_BUS_SPEED_MIN,		\
				XLNX_CANFD_BUS_SPEED_MAX),		\
		.reg_addr = DT_INST_REG_ADDR(n),			\
		.reg_size = DT_INST_REG_SIZE(n),			\
		.init_func = xcanfd_config_intr##n,			\
		.can_core_clock =					\
		DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),	\
	};								\
	static struct xcanfd_data xcanfd_data_##n;			\
	DEVICE_DT_INST_DEFINE(n, xcanfd_init,				\
			NULL,						\
			&xcanfd_data_##n,				\
			&xcanfd_cfg_##n,				\
			POST_KERNEL,					\
			CONFIG_CAN_INIT_PRIORITY,			\
			&xcanfd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(xcanfd_INST)
