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

typedef void (*init_func_t)(const struct device *dev);

struct xcanfd_cfg {
	const struct can_driver_config common;
	uint32_t reg_addr;
	int reg_size;
	init_func_t init_func;
	uint32_t can_core_clock;
};

struct xcanfd_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	uint8_t tx_head;
	uint8_t tx_tail;
	uint8_t tx_sent;
	struct k_mutex rx_mutex;
	enum can_state state;
	can_tx_callback_t tx_callback;
	void *tx_callback_arg;
	can_rx_callback_t rx_callback;
	void *rx_callback_arg;
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
 * @brief Handle sleep and wake-up state change interrupts
 */
static void xcanfd_state_interrupt(const struct device *dev, uint32_t isr)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t timeout = XCANFD_TIMEOUT;
	uint32_t status_reg;
	uint32_t msr;

	/* Handle sleep state change */
	if (isr & XCANFD_IXR_SLP_MASK) {
		data->state = CAN_STATE_STOPPED;
		msr = xcanfd_read32(config, XCANFD_MSR_OFFSET);

		if (!(msr & XCANFD_MSR_SLEEP_MASK)) {
			xcanfd_write32(config, XCANFD_MSR_OFFSET, msr | XCANFD_MSR_SLEEP_MASK);
		}
		xcanfd_write32(config, XCANFD_SRR_OFFSET, 0);
	}

	/* Handle wake-up state change */
	if (isr & XCANFD_IXR_WKUP_MASK) {
		data->state = CAN_STATE_ERROR_ACTIVE;
		xcanfd_write32(config, XCANFD_SRR_OFFSET, XCANFD_SRR_CEN_MASK);

		while (timeout > 0) {
			status_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
			if (status_reg & XCANFD_SR_NORMAL_MASK) {
				break;
			}
			timeout--;
		}

		if (timeout <= 0) {
			LOG_ERR("Timeout exceeded while waiting for wake-up");
		}
	}
}

/**
 * @brief Handle transmission complete interrupts
 */
static void xcanfd_tx_interrupt(const struct device *dev, uint32_t isr)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	unsigned int frames_in_fifo;
	can_tx_callback_t callback;
	int frames_sent = 1;
	void *callback_arg;

	frames_in_fifo = data->tx_head - data->tx_tail;

	if (frames_in_fifo != 0) {
		while (isr & XCANFD_IXR_TXOK_MASK) {
			xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_TXOK_MASK);
			isr = xcanfd_read32(config, XCANFD_ISR_OFFSET);
		}

		if (isr & XCANFD_IXR_TXFEMP_MASK) {
			frames_sent = frames_in_fifo;
		}
	} else {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_TXOK_MASK);
		return;
	}

	callback = data->tx_callback;
	callback_arg = data->tx_callback_arg;

	while (frames_sent--) {
		data->tx_tail++;
		data->tx_sent++;
	}

	data->tx_callback = NULL;
	data->tx_callback_arg = NULL;

	if (callback != NULL) {
		callback(dev, 0, callback_arg);
	}
}

/**
 * @brief Handle frame reception interrupts and process received frames
 */
static void xcanfd_rx_interrupt(const struct device *dev, uint32_t isr)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t fsr, mask, dwindex = 0;
	struct can_frame frame = {0};
	bool frame_consumed = false;
	bool dlc_invalid = false;
	uint32_t raw_id_reg;
	uint32_t dw_offset;
	uint32_t reg_data;
	uint32_t id1_part;
	uint32_t id2_part;
	uint32_t dlc_reg;
	int copy_len;
	int offset;
	int len;

	fsr = xcanfd_read32(config, XCANFD_FSR_OFFSET);

#ifdef CONFIG_CAN_FD_MODE
	mask = XCANFD_2_FSR_FL_MASK;
#else
	mask = XCANFD_FSR_FL_MASK;
#endif

	if (fsr & mask) {
#ifdef CONFIG_CAN_FD_MODE
		offset = XCANFD_RXMSG_2_FRAME_OFFSET(fsr & XCANFD_2_FSR_RI_MASK);
#else
		offset = XCANFD_RXMSG_FRAME_OFFSET(fsr & XCANFD_FSR_RI_MASK);
#endif

		raw_id_reg = xcanfd_read32(config, XCANFD_FRAME_ID_OFFSET(offset));
		dlc_reg = xcanfd_read32(config, XCANFD_FRAME_DLC_OFFSET(offset));

		if (dlc_reg & XCANFD_DLCR_EDL_MASK) {
			frame.flags |= CAN_FRAME_FDF;
		}

		if (raw_id_reg & XCANFD_IDR_RTR_MASK) {
			frame.flags |= CAN_FRAME_RTR;
		}

		if (raw_id_reg & XCANFD_IDR_IDE_MASK) {
			frame.flags |= CAN_FRAME_IDE;
		}

		if (frame.flags & CAN_FRAME_IDE) {
			id1_part = (raw_id_reg & XCANFD_IDR_ID1_MASK) >> XCANFD_IDR_ID1_SHIFT;
			id2_part = (raw_id_reg & XCANFD_IDR_ID2_MASK) >> XCANFD_IDR_ID2_SHIFT;

			frame.id = (id1_part << 18) | id2_part;
		} else {
			/* Standard frame - decode 11-bit ID from ID1 field */
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
				LOG_ERR("Invalid CANFD DLC: %d (max %d)", frame.dlc, CANFD_MAX_DLC);
				dlc_invalid = true;
			}
		} else {
			if (frame.dlc > CAN_MAX_DLC) {
				LOG_ERR("Invalid classic CAN DLC: %d (max %d)", frame.dlc,
					CAN_MAX_DLC);
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
			for (int i = 0; i < len; i += 4) {
				dw_offset = XCANFD_FRAME_DW_OFFSET(offset) +
					    (dwindex * XCANFD_DW_BYTES);
				reg_data = xcanfd_read32(config, dw_offset);
				reg_data = BSWAP_32(reg_data);
				copy_len = (len - i) > 4 ? 4 : (len - i);
				memcpy(&frame.data[i], &reg_data, copy_len);
				dwindex++;
			}
		}

		if (data->rx_callback != NULL) {
			data->rx_callback(dev, &frame, data->rx_callback_arg);
			frame_consumed = true;
		}

		if (!frame_consumed) {
			LOG_WRN("Frame ID=%08x flags=%02x dlc=%u received but no callback "
				"registered", frame.id, frame.flags, frame.dlc);
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

	if (dev == NULL) {
		LOG_ERR("ISR called with NULL device");
		return;
	}

	if (config == NULL || data == NULL) {
		LOG_ERR("ISR: Invalid config=%p or data=%p", config, data);
		return;
	}

	if (data->state == CAN_STATE_STOPPED && !data->common.started) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, 0xFFFFFFFF);
		return;
	}

	isr = xcanfd_read32(config, XCANFD_ISR_OFFSET);
	if (!isr) {
		return;
	}

	if (isr & (XCANFD_IXR_SLP_MASK | XCANFD_IXR_WKUP_MASK)) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET,
			       (XCANFD_IXR_SLP_MASK | XCANFD_IXR_WKUP_MASK));
		xcanfd_state_interrupt(dev, isr);
	}

	if (isr & XCANFD_IXR_ARBLST_MASK) {
		xcanfd_write32(config, XCANFD_ICR_OFFSET, XCANFD_IXR_ARBLST_MASK);
		k_mutex_lock(&data->inst_mutex, K_FOREVER);

		if (data->tx_callback != NULL) {
			can_tx_callback_t callback = data->tx_callback;
			void *callback_arg = data->tx_callback_arg;

			data->tx_callback = NULL;
			data->tx_callback_arg = NULL;

			k_mutex_unlock(&data->inst_mutex);

			callback(dev, -ENETUNREACH, callback_arg);
		} else {
			k_mutex_unlock(&data->inst_mutex);
		}
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
		k_mutex_lock(&data->inst_mutex, K_FOREVER);

		if (data->tx_callback != NULL) {
			can_tx_callback_t callback = data->tx_callback;
			void *callback_arg = data->tx_callback_arg;

			data->tx_callback = NULL;
			data->tx_callback_arg = NULL;

			k_mutex_unlock(&data->inst_mutex);

			callback(dev, -ENETUNREACH, callback_arg);
		} else {
			k_mutex_unlock(&data->inst_mutex);
		}
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

	/* Wait till configuration mode is active */
	k_busy_wait(1000);

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
	/* Wait for configuration mode to be active */
	k_busy_wait(1000);
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
	xcanfd_filter_enable(config, XCANFD_AFR_UAF_ALL_MASK);

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
	uint32_t timeout = XCANFD_TIMEOUT;
	uint32_t status_reg;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->common.started) {
		LOG_DBG("CAN controller already stopped");
		k_mutex_unlock(&data->inst_mutex);
		return -EALREADY;
	}

	xcanfd_write32(config, XCANFD_SRR_OFFSET, 0);

	while (timeout > 0) {
		status_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
		if (status_reg & XCANFD_SR_CONFIG_MASK) {
			break;
		}
		timeout--;
	}

	if (timeout <= 0) {
		LOG_ERR("Timeout waiting for configuration mode");
		ret = -ETIMEDOUT;
	}

	xcanfd_write32(config, XCANFD_IER_OFFSET, 0);
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
static int xcanfd_set_timing_data(const struct device *dev, const struct can_timing *timing)
{
#ifdef CONFIG_CAN_FD_MODE
	struct xcanfd_data *data = dev->data;
	struct can_timing calc_timing_data;
	uint32_t is_config_mode;
	uint32_t btr0, btr1;
	int ret;

	const struct xcanfd_cfg *config = dev->config;
	uint32_t requested_bitrate = config->common.bitrate_data;


	if (dev == NULL || config == NULL || data == NULL) {
		LOG_ERR("Null pointer: dev=%p, config=%p, data=%p", dev, config, data);
		return -EINVAL;
	}

	if (data->common.started) {
		LOG_ERR("Cannot set bit timing data while CAN controller is started");
		return -EBUSY;
	}

	if (requested_bitrate < config->common.min_bitrate ||
		requested_bitrate > config->common.max_bitrate) {
		LOG_ERR("Requested bitrate (%u) out of range [%u, %u]",
			requested_bitrate, config->common.min_bitrate, config->common.max_bitrate);
		return -EINVAL;
	}

	ret = can_calc_timing_data(dev, &calc_timing_data, config->common.bitrate_data,
				   config->common.sample_point_data);

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
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(timing);
	return -ENOTSUP;
#endif
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

	if (dev == NULL || config == NULL || data == NULL) {
		LOG_ERR("Null pointer: dev=%p, config=%p, data=%p", dev, config, data);
		return -EINVAL;
	}

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
		return -EIO; /* General I/O error */
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

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			*state = data->state;
		}
	}

	if (err_cnt != NULL) {
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
	uint32_t id_reg = 0;

	if (frame == NULL) {
		LOG_ERR("Frame pointer is NULL");
		return -EINVAL;
	}

	if (!data->common.started) {
		LOG_ERR("CANFD controller not started");
		return -ENETDOWN;
	}
#ifdef CONFIG_CAN_FD_MODE
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR |
		CAN_FRAME_FDF | CAN_FRAME_BRS | CAN_FRAME_ESI)) != 0) {
		LOG_ERR("Unsupported CANFD frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if ((frame->flags & CAN_FRAME_FDF) != 0) {
		if ((data->common.mode & CAN_MODE_FD) == 0U) {
			LOG_ERR("CANFD frame not supported in current mode");
			return -ENOTSUP;
		}
	}
#else
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		LOG_ERR("Unsupported CANFD frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}
#endif

	if (frame->dlc > 15) {
		LOG_ERR("DLC %d is invalid (max possible is 15)", frame->dlc);
		return -EINVAL;
	}

#ifdef CONFIG_CAN_FD_MODE
	if ((frame->flags & CAN_FRAME_FDF) != 0) {
		/* CANFD frame - allow up to CANFD_MAX_DLC (15) */
		if (frame->dlc > CANFD_MAX_DLC) {
			LOG_ERR("CANFD DLC of %d exceeds maximum (%d)", frame->dlc, CANFD_MAX_DLC);
			return -EINVAL;
		}
	} else {
		/* Classic CANFD frame - allow up to CAN_MAX_DLC (8) */
		if (frame->dlc > CAN_MAX_DLC) {
			LOG_ERR("Classic CANFD DLC of %d exceeds maximum (%d)",
				frame->dlc, CAN_MAX_DLC);
			return -EINVAL;
		}
	}
#else
	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("Classic CAN DLC of %d exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}
#endif

	if (xcanfd_read32(config, XCANFD_TRR_OFFSET) & BIT(XCANFD_TX_MAILBOX_IDX)) {
		LOG_ERR("TX buffer is busy");
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	frame_offset = XCANFD_TXMSG_FRAME_OFFSET(XCANFD_TX_MAILBOX_IDX);
	data->tx_callback_arg = user_data;
	data->tx_callback = callback;
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

	xcanfd_write32(config, XCANFD_FRAME_ID_OFFSET(frame_offset), id_reg);
	xcanfd_write32(config, XCANFD_FRAME_DLC_OFFSET(frame_offset), dlc_reg);

	nobytes = can_dlc_to_bytes(frame->dlc);

	for (uint32_t len = 0; len < nobytes; len += 4) {
		value = BSWAP_32(frame->data_32[len/4]);
		xcanfd_write32(config, (XCANFD_FRAME_DW_OFFSET(frame_offset) +
				(dwindex * XCANFD_DW_BYTES)), value);
		dwindex++;
	}
	xcanfd_write32(config, XCANFD_TRR_OFFSET, BIT(XCANFD_TX_MAILBOX_IDX));

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

/**
 * @brief Add a receive filter and callback for incoming CAN and CANFD frames
 */
static int xcanfd_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
		void *cb_arg, const struct can_filter *filter)
{
	struct xcanfd_data *data = dev->data;
	int ret = 0;

	if (cb == NULL) {
		LOG_ERR("Receive callback function cannot be NULL");
		return -EINVAL;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/*
	 * Since this driver has no hardware filters, we only support
	 * one global callback that receives all frames.
	 */
	if (data->rx_callback != NULL) {
		LOG_WRN("RX callback already registered, replacing existing callback");
	}
	data->rx_callback = cb;
	data->rx_callback_arg = cb_arg;
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

/**
 * @brief Remove a previously registered receive filter
 */
static void xcanfd_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct xcanfd_data *data = dev->data;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->rx_callback != NULL) {
		data->rx_callback = NULL;
		data->rx_callback_arg = NULL;
	} else {
		LOG_WRN("No previously registered receive filter to remove");
	}

	k_mutex_unlock(&data->inst_mutex);
}

/**
 * @brief Put the CAN controller into reset/configuration mode
 */
static int xcanfd_reset(const struct device *dev)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t timeout = XCANFD_TIMEOUT;
	uint32_t sr_reg;

	xcanfd_write32(config, XCANFD_IER_OFFSET, 0);
	xcanfd_write32(config, XCANFD_ICR_OFFSET, 0xFFFFFFFF);
	xcanfd_write32(config, XCANFD_SRR_OFFSET, XCANFD_SRR_RESET_MASK);

	/* Wait for reset to complete */
	k_busy_wait(1000);

	while (timeout > 0) {
		sr_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
		if (sr_reg & XCANFD_SR_CONFIG_MASK) {
			break;
		}
		timeout--;
	}

	if (timeout <= 0) {
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
	return 1;
}

/**
 * @brief Initialize the Xilinx CANFD controller
 */
static int xcanfd_init(const struct device *dev)
{
	const struct xcanfd_cfg *config = dev->config;
	struct xcanfd_data *data = dev->data;
	uint32_t timeout = XCANFD_TIMEOUT;
	struct can_timing timing = { 0 };
	uint32_t status_reg;
	int ret;

	if (config == NULL || data == NULL) {
		LOG_ERR("Invalid device configuration or data");
		return -EINVAL;
	}

	k_mutex_init(&data->inst_mutex);
	k_mutex_init(&data->rx_mutex);

	data->tx_head = 0;
	data->tx_tail = 0;
	data->tx_sent = 0;

	data->tx_callback = NULL;
	data->tx_callback_arg = NULL;

	data->state = CAN_STATE_STOPPED;

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

	while (timeout > 0) {
		status_reg = xcanfd_read32(config, XCANFD_SR_OFFSET);
		if (status_reg & XCANFD_SR_CONFIG_MASK) {
			break;
		}
		timeout--;
	}

	if (timeout <= 0) {
		LOG_ERR("Failed to enter configuration mode for interrupt setup");
		return -ETIMEDOUT;
	}

	xcanfd_write32(config, XCANFD_IER_OFFSET, 0);
	xcanfd_write32(config, XCANFD_ICR_OFFSET, 0xFFFFFFFF);
	config->init_func(dev);
	data->state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}

static DEVICE_API(can, xcanfd_driver_api) = {
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
