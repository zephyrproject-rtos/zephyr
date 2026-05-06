/*
 * Copyright (C) 2025 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The driver is verified with CAST CAN-CTRL IP:7x06N00S00
 * Default settings as per alif board:
 *   a. Maximum Rx buf slots = 16
 *   b. Rx buf Almost full warning limit maximum = 15
 *   c. Maximum Tx buf slots = 16
 */

#define DT_DRV_COMPAT cast_can

#include "can_cast.h"
#include <zephyr/logging/log.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(CAN, CONFIG_CAN_LOG_LEVEL);

/* Define base masks for mode categories */
#define CAN_CAST_BASE_MODE_MASK  (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)

#define DEV_CFG(_dev)  ((const struct can_cast_config *)((_dev)->config))
#define DEV_DATA(_dev) ((struct can_cast_data *)((_dev)->data))

#define QUEUE_HEAD_NEXT(head) (head = (((head) + 1) % (CAN_MAX_STB_SLOTS)))
#define QUEUE_TAIL_NEXT(tail) (tail = (((tail) + 1) % (CAN_MAX_STB_SLOTS)))
/**
 * @fn          static void can_cast_set_err_warn_limit(uint32_t can_base,
 *                                                      const uint8_t ewl)
 * @brief       Configures Warning limits for errors
 * @note        If ewl value is greater than CAN_MAX_ERROR_WARN_LIMIT
 *              the limit will be set to CAN_MAX_ERROR_WARN_LIMIT
 * @param[in]   can_base : Base address of CAN register map
 * @param[in]   ewl      : Limit value for Error warning
 * @return      none
 */
static void can_cast_set_err_warn_limit(uint32_t can_base, const uint8_t ewl)
{
	uint8_t temp;

	temp = sys_read8(can_base + CAN_LIMIT);

	if (ewl <= CAN_MAX_ERROR_WARN_LIMIT) {
		/* Sets the in range error warning value */
		temp |= ((((ewl / 8U) - 1U) << CAN_LIMIT_EWL_Pos) & CAN_LIMIT_EWL_Msk);
		sys_write8(temp, (can_base + CAN_LIMIT));
	} else {
		/* Sets error warning to Max */
		temp |= ((((CAN_MAX_ERROR_WARN_LIMIT / 8U) - 1U) << CAN_LIMIT_EWL_Pos) &
			 CAN_LIMIT_EWL_Msk);
		sys_write8(temp, (can_base + CAN_LIMIT));
	}
}

/**
 * @fn          static void can_cast_set_tx_single_shot_mode(uint32_t can_base,
 *                                                           const bool enable)
 * @brief       Enables/Disables Single shot mode for transmission
 * @param[in]   can_base : Base address of CAN register map
 * @param[in]   enable   : Command to enable/disable single shot mode
 * @return      none
 */
static void can_cast_set_tx_single_shot_mode(uint32_t can_base, const bool enable)
{
	uint8_t temp;

	temp = sys_read8(can_base + CAN_CFG_STAT);
	if (enable) {
		/* Enables STB single shot mode */
		temp |= BIT(CAN_CFG_STAT_TSSS);
		sys_write8(temp, (can_base + CAN_CFG_STAT));
	} else {
		/* Disables STB single shot mode */
		temp &= (~(BIT(CAN_CFG_STAT_TSSS)));
		sys_write8(temp, (can_base + CAN_CFG_STAT));
	}
}

/**
 * @fn          static void can_cast_enable_acpt_fltr(uint32_t can_base,
 *                                                    struct can_acpt_fltr_t filter_config
 * @brief       Configures and enables the particular acceptance filter.
 * @param[in]   can_base       : Base address of CAN register map
 * @param[in]   filter_config  : Filter configuration
 * @return      none
 */
static void can_cast_enable_acpt_fltr(uint32_t can_base, struct can_acpt_fltr_t filter_config)
{
	uint32_t temp;

	/* Select AMASK configuration */
	temp = (filter_config.filter & CAN_ACFCTRL_ACFADR_Msk);
	temp |= BIT(CAN_ACFCTRL_SELMASK);
	sys_write8(temp, (can_base + CAN_ACFCTRL));

	/* Enable filter */
	temp = sys_read16(can_base + CAN_ACF_EN_0);
	temp |= ((1U << filter_config.filter) & CAN_ACF_EN_0_AE_X_MAX_Msk);
	sys_write16(temp, (can_base + CAN_ACF_EN_0));

	/* Converting mask from CMSIS value to controller supporting mask*/
	filter_config.ac_mask = ~(filter_config.ac_mask);

	/* Storing the mask */
	filter_config.ac_mask = CAN_ACF0_3_AMASK_X_Msk(filter_config.ac_mask);

	/* 1. For all frames, the bits 29 and 30 should be zero,
	 * 2. For Extended frames, the bits 29 and 30 should be one,
	 * 3. For Std frames, bit 29 should be zero and bit 30 should be one
	 */
	if (filter_config.frame_type == CAN_ACPT_FILTER_CFG_EXT_FRAMES) {
		filter_config.ac_mask |= (BIT(CAN_ACF_3_MASK_AIDE) | BIT(CAN_ACF_3_MASK_AIDEE));
	} else if (filter_config.frame_type == CAN_ACPT_FILTER_CFG_STD_FRAMES) {
		filter_config.ac_mask |= BIT(CAN_ACF_3_MASK_AIDEE);
	} else {
		; /* Does nothing */
	}

	/* Storing the mask value with the type of frame filtering */
	sys_write32(filter_config.ac_mask, (can_base + CAN_ACF_0_3_MASK));

	/* Select ACODE configuration */
	temp = sys_read8(can_base + CAN_ACFCTRL);
	temp &= ~BIT(CAN_ACFCTRL_SELMASK);
	sys_write8(temp, (can_base + CAN_ACFCTRL));

	sys_write32(CAN_ACF0_3_ACODE_X(filter_config.ac_code), (can_base + CAN_ACF_0_3_CODE));
}

/**
 * @fn          static enum can_acpt_fltr_status can_cast_get_acpt_fltr_status(uint32_t can_base,
 *                                                                             const uint8_t filter)
 * @brief       Retrieves whether the filter is free or occupied.
 * @param[in]   can_base : Base address of CAN register map
 * @param[in]   filter   : Acceptance filter number
 * @return      status of the filter (Free/Occupied)
 */
static enum can_acpt_fltr_status can_cast_get_acpt_fltr_status(uint32_t can_base,
							       const uint8_t filter)
{
	uint16_t temp = sys_read16(can_base + CAN_ACF_EN_0);

	/* Returns status of the requested filter */
	if (temp & (1U << filter)) {
		return CAN_ACPT_FLTR_STATUS_OCCUPIED;
	}
	return CAN_ACPT_FLTR_STATUS_FREE;
}

/**
 * @fn          static void can_cast_copy_tx_buf(volatile uint32_t* dest,
 * @                                          const uint32_t* src,
 * @                                          const uint8_t len)
 * @brief       Copies the message from source to destination buffer
 * @note        This function is only applicable for CAN Tx buffer copy
 * @param[in]   dest  : pointer to destination buffer
 * @param[in]   src   : pointer to source message buffer
 * @param[in]   len   : Length of message
 * @return      none
 */
static void can_cast_copy_tx_buf(volatile uint32_t *dest, const uint32_t *src, const uint8_t len)
{
	uint8_t iter;
	uint8_t rem;
	uint32_t rem_data = 0U;

	/* Copies the data from src buffer to destination buffer */
	for (iter = 0U; iter < (len / 4U); iter++) {
		*dest++ = src[iter];
	}
	rem = (len % 4);
	iter = 0U;
	while (rem) {
		rem_data |= (((uint8_t *)src)[len - rem] << (8U * iter));
		rem--;
		iter++;
	}
	*dest = rem_data;
}

/**
 * @fn      static void can_cast_prepare_send_msg(uint32_t can_base,
 *                                                const struct can_frame *frame)
 * @brief   Prepare and send tx msg
 * @param[in]   can_base   : Base address of CAN register map
 * @param[in]   frame      : msg frame
 * @return  None
 */
static void can_cast_prepare_send_msg(uint32_t can_base, const struct can_frame *frame)
{
	volatile struct tbuf_regs_t *tx_msg = (volatile struct tbuf_regs_t *)(can_base + CAN_TBUF);
	uint8_t temp = sys_read8(can_base + CAN_TCMD);

	/* Secondary buffer is selected for next msg tx */
	temp |= BIT(CAN_TCMD_TBSEL);
	sys_write8(temp, can_base + CAN_TCMD);

	/* Copies ID and control fields */
#if CONFIG_CAN_RX_TIMESTAMP
	tx_msg->can_id = (frame->id | CAN_MSG_TTSEN);
#else
	tx_msg->can_id = frame->id;
#endif

	tx_msg->control = ((frame->flags & CAN_FRAME_IDE) ? CAN_MSG_IDE(1) : 0);
	tx_msg->control |= ((frame->flags & CAN_FRAME_RTR) ? CAN_MSG_RTR(1) : 0);
	tx_msg->control |= ((frame->flags & CAN_FRAME_FDF) ? CAN_MSG_FDF(1) : 0);
	tx_msg->control |= ((frame->flags & CAN_FRAME_BRS) ? CAN_MSG_BRS(1) : 0);
	tx_msg->control |= CAN_MSG_DLC(frame->dlc);

	/* Copies tx data if it is a data frame*/
	if (!(frame->flags & CAN_FRAME_RTR)) {
		can_cast_copy_tx_buf((volatile uint32_t *)tx_msg->data, (uint32_t *)frame->data,
				     can_dlc_to_bytes(frame->dlc));
	}

	/* Moves the pointer to next buf slot and
	 * enables the tx of all frames in sec buf
	 */
	temp = sys_read8(can_base + CAN_TCTRL);
	temp |= BIT(CAN_TCTRL_TSNEXT);
	sys_write8(temp, (can_base + CAN_TCTRL));

	temp = sys_read8(can_base + CAN_TCMD);
	temp |= BIT(CAN_TCMD_TSALL);
	sys_write8(temp, (can_base + CAN_TCMD));
}

/**
 * @fn      static int can_cast_init(const struct device *dev)
 * @brief   Initialises CAN
 * @param[in_out]   dev : pointer to Runtime device structure
 * @return  None
 */
static int can_cast_init(const struct device *dev)
{
	const struct can_cast_config *config = DEV_CFG(dev);
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	uint32_t cnt_base;
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, can_reg, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, can_cnt_reg, K_MEM_CACHE_NONE);

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);
	cnt_base = DEVICE_MMIO_NAMED_GET(dev, can_cnt_reg);

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("Unable to configure pins err:%d", err);
		return err;
	}

	/* Include the clock controller and set
	 * clock rate
	 */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("clock controller device not ready");
		return -ENODEV;
	}

	err = clock_control_configure(config->clk_dev, config->clk_id, NULL);
	if (err != 0) {
		LOG_ERR("Unable to configure clock: err:%d", err);
		return err;
	}

	err = clock_control_set_rate(config->clk_dev, config->clk_id,
				     (clock_control_subsys_rate_t)config->clk_freq);
	if (err != 0) {
		LOG_ERR("Unable to set clock rate: err:%d", err);
		return err;
	}

	err = clock_control_on(config->clk_dev, config->clk_id);
	if (err != 0) {
		LOG_ERR("Unable to turn on clock: err:%d", err);
		return err;
	}
#endif

	/* Enables Standby mode */
	can_cast_enable_standby_mode(can_base);
	k_usleep(CAN_TRANSCEIVER_STANDBY_DELAY);

	/* Enable IRQ and install ISR handler. */
	config->irq_config_func(dev);

	can_cast_reset_enable(can_base);
	can_cast_set_iso_spec(can_base);
	can_cast_set_tbuf_op_mode_priority(can_base);

	/* Initialise instance mutex */
	k_mutex_init(&data->inst_mutex);

	data->common.mode = 0U;
	data->common.started = false;
	data->err_state = CAN_STATE_STOPPED;

	data->state.initialized = 1;
	return 0;
}

/**
 * @fn        static int can_cast_get_capabilities(const struct device *dev,
 *                                                 can_mode_t *cap)
 * @brief    Get capabilities
 * @param[in]   dev : pointer to Runtime device structure
 * @param[in]   cap : pointer to can capabilities variable
 * @return    None
 */
static int can_cast_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	const struct can_cast_config *config = DEV_CFG(dev);

	if (cap == NULL) {
		return -EINVAL;
	}

	*cap = (CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT);

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		if (config->can_fd) {
			*cap |= CAN_MODE_FD;
		}
	}

	return 0;
}

/**
 * @fn      static int can_cast_start(const struct device *dev)
 * @brief   Start CAN communication
 * @param[in] dev : pointer to Runtime device structure
 * @return  Execution status
 */
static int can_cast_start(const struct device *dev)
{
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	uint32_t can_cnt_base;

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);
	can_cnt_base = DEVICE_MMIO_NAMED_GET(dev, can_cnt_reg);

	if (data->common.started) {
		return -EALREADY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Reset statistics and clear error counters */
	CAN_STATS_RESET(dev);

	can_cast_reset_disable(can_base);
	/* Enable interrupts */
	if (!(data->common.mode & CAN_MODE_LISTENONLY)) {
		can_cast_enable_tx_interrupts(can_base);
	}

	can_cast_enable_rx_interrupts(can_base);
	can_cast_enable_error_interrupts(can_base);

#if CONFIG_CAN_RX_TIMESTAMP
	/* Enable Rx message Timestamp */
	can_cast_enable_timestamp(can_base);
	can_cast_counter_set(can_cnt_base, 0);
	can_cast_counter_start(can_cnt_base);
#endif

	/* Disables Standby mode */
	can_cast_disable_standby_mode(can_base);
	k_usleep(CAN_TRANSCEIVER_STANDBY_DELAY);

	/* Stop CAN communication */
	data->common.started = true;
	data->err_state = CAN_STATE_ERROR_ACTIVE;

	k_mutex_unlock(&data->inst_mutex);
	return 0;
}

/**
 * @fn      static int can_cast_stop(const struct device *dev)
 * @brief   Stop CAN communication
 * @param[in] dev : pointer to Runtime device structure
 * @return  Execution status
 */
static int can_cast_stop(const struct device *dev)
{
#if CONFIG_CAN_FD_MODE
	const struct can_cast_config *config = DEV_CFG(dev);
	uint32_t temp;
#endif
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	uint32_t can_cnt_base;

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);
	can_cnt_base = DEVICE_MMIO_NAMED_GET(dev, can_cnt_reg);

	if (!data->common.started) {
		return -EALREADY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

#if CONFIG_CAN_RX_TIMESTAMP
	/* Stop Timer counter */
	can_cast_counter_stop(can_cnt_base);
#endif

	/* Enables Standby mode */
	can_cast_enable_standby_mode(can_base);
	k_usleep(CAN_TRANSCEIVER_STANDBY_DELAY);

	/* Disable and clear all the interrupts */
	can_cast_disable_tx_interrupts(can_base);
	can_cast_disable_rx_interrupts(can_base);
	can_cast_disable_error_interrupts(can_base);

	/* Abort if any tx is pending */
	if (can_cast_tx_active(can_base)) {
		can_cast_abort_tx(can_base);
	}

	can_cast_clear_interrupts(can_base);

	/* Reset Tx queue */
	data->tx_queue.head = 0U;
	data->tx_queue.tail = 0U;

	/* Stop CAN communication */
	can_cast_reset_enable(can_base);

#if CONFIG_CAN_FD_MODE
	if (data->common.mode & CAN_MODE_FD) {
		if (config->can_fd) {
			temp = sys_read32(config->can_fd_ctrl_reg);
			temp &= ~BIT(config->can_fd_bit);
			sys_write32(temp, config->can_fd_ctrl_reg);
		}

	}
#endif
	data->common.mode = 0U;
	data->common.started = false;
	data->err_state = CAN_STATE_STOPPED;

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

/**
 * @fn      static int can_cast_set_mode(const struct device *dev,
 *                                       can_mode_t mode)
 * @brief   Set CAN Mode
 * @param[in]   dev  : pointer to Runtime device structure
 * @param[in]   mode : CAN mode
 * @return  None
 */
static int can_cast_set_mode(const struct device *dev, can_mode_t mode)
{
#if CONFIG_CAN_FD_MODE
	const struct can_cast_config *config = DEV_CFG(dev);
	uint32_t temp;
#endif
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	uint32_t base_modes = (mode & CAN_CAST_BASE_MODE_MASK);

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	if (data->common.started) {
		return -EBUSY;
	}

	if (mode & CAN_MODE_3_SAMPLES) {
		LOG_ERR("Triple Sampling mode Not Supported");
		return -ENOTSUP;
	}

	/* Check if more than one base mode bit is set */
	if ((base_modes != 0) && ((base_modes & (base_modes - 1)) != 0)) {
		LOG_ERR("Only one base mode allowed, got: 0x%x", mode);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Releases CAN from Reset state */
	can_cast_reset_disable(can_base);

	/* Disables and clears all interrupts */
	can_cast_disable_tx_interrupts(can_base);
	can_cast_disable_rx_interrupts(can_base);
	can_cast_disable_error_interrupts(can_base);
	can_cast_clear_interrupts(can_base);

	if (mode & CAN_MODE_LOOPBACK) {
		/* If msg transmission is happening then return Busy */
		if (can_cast_comm_active(can_base)) {
			LOG_ERR("Device is busy in communication");
			k_mutex_unlock(&data->inst_mutex);
			return -EBUSY;
		}

		/* Enables Internal Loopback Mode */
		can_cast_enable_internal_loop_back_mode(can_base);
	} else if (mode & CAN_MODE_LISTENONLY) {
		/* Enables Listen Only Mode */
		can_cast_enable_listen_only_mode(can_base);
	} else {
		/* Enables Normal mode */
		can_cast_enable_normal_mode(can_base);
	}

#if CONFIG_CAN_FD_MODE
	if (mode & CAN_MODE_FD) {
		if (config->can_fd) {
			temp = sys_read32(config->can_fd_ctrl_reg);
			temp |= BIT(config->can_fd_bit);
			sys_write32(temp, config->can_fd_ctrl_reg);
		} else {
			LOG_ERR("FD mode is disabled in device tree");
			k_mutex_unlock(&data->inst_mutex);
			return -ENOTSUP;
		}

	}
#endif

	if (mode & CAN_MODE_ONE_SHOT) {
		can_cast_set_tx_single_shot_mode(can_base, true);
	}

	/* Sets the CAN Error and Rx buf almost full warning limits */
	can_cast_set_err_warn_limit(can_base, CAN_ERROR_WARN_LIMIT);
	can_cast_set_rbuf_almost_full_warn_limit(can_base, CONFIG_CAN_RBUF_AFWL);

	data->common.mode = mode;

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

/**
 * @fn      static int can_cast_set_timing(const struct device *dev,
 *                                         const struct can_timing *timing)
 * @brief   Set CAN timing (bitrate settings)
 * @param[in] dev    : pointer to Runtime device structure
 * @param[in] timing : pointer to can timing variable
 * @return  None
 */
static int can_cast_set_timing(const struct device *dev, const struct can_timing *timing)
{
	const struct can_cast_config *config = DEV_CFG(dev);
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	struct can_timing loc_timing;

	if (data->common.started) {
		return -EBUSY;
	}

	if (!timing) {
		return -EINVAL;
	}

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	/* If max_bitrate is less than 1b or greater than 10Mb returns an error */
	if ((config->common.max_bitrate < 0x1U) ||
		(config->common.max_bitrate > CAN_MAX_BITRATE)) {
		LOG_ERR("Maximum bitrate is invalid");
		return -ENOTSUP;
	}

	loc_timing.sjw = timing->sjw;
	/* Checks for the validity of the Signal Jump Width*/
	if ((loc_timing.sjw < 0x1U) || (loc_timing.sjw > 0x10U) ||
	    (loc_timing.sjw > timing->phase_seg2)) {
		LOG_ERR("Invalid Signal Jump width");
		return -ENOTSUP;
	}

	/* Combines Sync seg, Prop Seg and Phase seg1 */
	loc_timing.phase_seg1 = timing->prop_seg + timing->phase_seg1 + CAN_SYNC_SEG_TQ;

	/* Checks for the validity of the Segment 1*/
	if ((loc_timing.phase_seg1 < 0x2U) || (loc_timing.phase_seg1 > 0x41U)) {
		LOG_ERR("Invalid Prop or Phase segment 1");
		return -ENOTSUP;
	}

	loc_timing.phase_seg2 = timing->phase_seg2;
	/* Checks for the validity of the Segment 2*/
	if ((loc_timing.phase_seg2 < 0x1U) || (loc_timing.phase_seg2 > 0x20U)) {
		LOG_ERR("Invalid Phase segment 2");
		return -ENOTSUP;
	}

	loc_timing.prescaler = timing->prescaler;
	if ((loc_timing.prescaler < 0x1U) || (loc_timing.prescaler > 0x4U)) {
		LOG_ERR("Invalid Prescaler");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Invokes below function to set the nominal bitrate */
	can_cast_set_nominal_bit_time(can_base, loc_timing);

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

#if CONFIG_CAN_FD_MODE
/**
 * @fn      static int can_cast_set_timing_data(const struct device *dev,
 *                                              const struct can_timing *timing_data)
 * @brief   Set CAN timing for FD mode (bitrate settings)
 * @param[in] dev         : pointer to Runtime device structure
 * @param[in] timing_data : pointer to can timing data variable
 * @return  None
 */
static int can_cast_set_timing_data(const struct device *dev, const struct can_timing *timing_data)
{
	const struct can_cast_config *config = DEV_CFG(dev);
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	struct can_timing loc_timing;
	uint8_t tdc;

	if (data->common.started) {
		return -EBUSY;
	}

	if (!timing_data) {
		return -EINVAL;
	}

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	/* If max_bitrate is less than 1b or greater than 10Mb returns an error */
	if ((config->common.max_bitrate < 0x1U) || (config->common.max_bitrate > CAN_MAX_BITRATE)) {
		LOG_ERR("Maximum bitrate is invalid");
		return -ENOTSUP;
	}

	loc_timing.sjw = timing_data->sjw;
	/* Checks for the validity of the Signal Jump Width*/
	if ((loc_timing.sjw < 0x1U) || (loc_timing.sjw > 0x10U) ||
	    (loc_timing.sjw > timing_data->phase_seg2)) {
		LOG_ERR("Invalid Signal Jump Width data");
		return -ENOTSUP;
	}

	/* Combines Sync seg, Prop Seg and Phase seg1 */
	loc_timing.phase_seg1 = timing_data->prop_seg + timing_data->phase_seg1 + CAN_SYNC_SEG_TQ;

	/* Checks for the validity of the Segment 1*/
	if ((loc_timing.phase_seg1 < 0x2U) || (loc_timing.phase_seg1 > 0x11U)) {
		LOG_ERR("Invalid Prop or Phase segment 1 data");
		return -ENOTSUP;
	}

	loc_timing.phase_seg2 = timing_data->phase_seg2;
	/* Checks for the validity of the Segment 2*/
	if ((loc_timing.phase_seg2 < 0x1U) || (loc_timing.phase_seg2 > 0x8U)) {
		LOG_ERR("Invalid Phase segment 2 data");
		return -ENOTSUP;
	}

	loc_timing.prescaler = timing_data->prescaler;
	if ((loc_timing.prescaler < 0x1U) || (loc_timing.prescaler > 0x2U)) {
		LOG_ERR("Invalid Prescaler data");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Calculates Transmitter delay compensation */
	tdc = (loc_timing.prescaler * loc_timing.phase_seg1);

	/* Invokes below function to set the Fast-data bitrate */
	can_cast_set_fd_bit_time(can_base, loc_timing, tdc);

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}
#endif

/**
 * @fn      static int can_cast_send(const struct device *dev,
 *                                   const struct can_frame *frame,
 *                                   k_timeout_t timeout,
 *                                   can_tx_callback_t callback,
 *                                   void *user_data)
 * @brief   Send CAN messages
 * @param[in]   dev      : pointer to Runtime device structure
 * @param[in]   frame    : pointer to Tx message frame
 * @param[in]   timeout  : Timeout value
 * @param[in]   callback : pointer to callback function
 * @param[in]   userdata : pointer to userdata
 * @return  None
 */

static int can_cast_send(const struct device *dev, const struct can_frame *frame,
			 k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_cast_data *data = DEV_DATA(dev);
	bool fifo_free = true;
	uint32_t can_base;
	uint64_t usec_timeout;

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if (data->err_state == CAN_STATE_BUS_OFF) {
		return -ENETUNREACH;
	}

	/* Returns error if in Listen Only mode */
	if (data->common.mode & CAN_MODE_LISTENONLY) {
		LOG_ERR("Mode: ListenOnly");
		return -ENOTSUP;
	}

	/* Returns error if RTR is requested to send in FD mode */
	if ((frame->flags & CAN_FRAME_RTR) && (frame->flags & (CAN_FRAME_FDF | CAN_FRAME_BRS))) {
		LOG_ERR("RTR in FD Mode");
		return -EINVAL;
	}

#if CONFIG_CAN_FD_MODE
	if (frame->dlc > CANFD_MAX_DLC) {
		LOG_ERR("DLC is greater than max");
		return -EINVAL;
	}
#else
	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC is greater than max");
		return -EINVAL;
	}
#endif

	/* Returns error if FD len frame is requested
	 * to send in normal mode
	 */
	if ((frame->dlc > CAN_MAX_DLC) && ((!(frame->flags & (CAN_FRAME_FDF | CAN_FRAME_BRS))) ||
					   (!(data->common.mode & CAN_MODE_FD)))) {
		LOG_ERR("FD mode OFF");
		return -EINVAL;
	}

	/* If the error warning, then returns an error */
	if (can_cast_err_warn_limit_reached(can_base)) {
		LOG_ERR("Error Warning limit crossed");
		return -EIO;
	}

	/* Checks for Tx queue free status */
	if (!can_cast_stb_free(can_base)) {
		fifo_free = false;

		usec_timeout = k_ticks_to_us_near64(timeout.ticks);

		/* Wait for Tx buffer to get free */
		while (usec_timeout) {
			if (can_cast_stb_free(can_base)) {
				fifo_free = true;
				break;
			}
			k_usleep(100);
			usec_timeout -= 100;
		}
	}

	if (!fifo_free) {
		LOG_ERR("Send Timed out");
		return -EAGAIN;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Store the callback info */
	data->tx_queue.cb_list[data->tx_queue.head].cb = callback;
	data->tx_queue.cb_list[data->tx_queue.head].cb_arg = user_data;

	can_cast_prepare_send_msg(can_base, frame);

	if (callback) {
		QUEUE_HEAD_NEXT(data->tx_queue.head);
	} else {
		usec_timeout = k_ticks_to_us_near64(K_TICKS_FOREVER);

		/* Wait for Tx buffer to get empty*/
		while (usec_timeout) {
			if (can_cast_stb_empty(can_base)) {
				break;
			}
			k_usleep(100);
			usec_timeout -= 100;
		}
	}

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

/**
 * @fn      static int can_cast_add_rx_filter(const struct device *dev,
 *                                            can_rx_callback_t callback,
 *                                            void *user_data,
 *                                            const struct can_filter *filter)
 * @brief   Add rx message filter
 * @param[in]   dev       : pointer to Runtime device structure
 * @param[in]   callback  : pointer to rx callback function
 * @param[in]   user_data : pointer to rx data buffer
 * @param[in]   filter    : pointer to Rx message filter
 * @return  None
 */

static int can_cast_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				  void *user_data, const struct can_filter *filter)
{
	struct can_cast_data *data = DEV_DATA(dev);
	struct can_acpt_fltr_t filter_cfg = {0x0U};
	struct can_cast_filter_t *msg_filter = NULL;
	uint32_t can_base = 0U;
	uint8_t filter_num = 0x0U;
	bool filter_avail = false;

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* If none of the filters configured then resets the first filter*/
	if (data->state.filter_configured == 0x0U) {
		can_cast_reset_acpt_fltrs(can_base);
	}

	for (filter_num = 0x0U; filter_num < CONFIG_CAN_MAX_FILTER; filter_num++) {
		if (can_cast_get_acpt_fltr_status(can_base, filter_num) ==
		    CAN_ACPT_FLTR_STATUS_FREE) {
			if (filter->flags & CAN_FILTER_IDE) {
				filter_cfg.frame_type = CAN_ACPT_FILTER_CFG_EXT_FRAMES;
			} else {
				filter_cfg.frame_type = CAN_ACPT_FILTER_CFG_STD_FRAMES;
			}

			filter_cfg.ac_code = filter->id;
			filter_cfg.ac_mask = filter->mask;
			filter_cfg.filter = filter_num;

			/* If the filter is available, then configures the values*/
			can_cast_enable_acpt_fltr(can_base, filter_cfg);
			filter_avail = true;

			msg_filter = &data->filter[filter_num];

			msg_filter->rx_cb = callback;
			msg_filter->cb_arg = user_data;
			memcpy(&(msg_filter->rx_filter), filter, sizeof(struct can_filter));

			break;
		}
	}

	/* If at least one filter is configured then it sets the flag*/
	if (can_cast_acpt_fltr_configured(can_base)) {
		data->state.filter_configured = 0x1U;
	} else {
		data->state.filter_configured = 0x0U;
	}

	k_mutex_unlock(&data->inst_mutex);

	/* If either the filter is not available or
	 * the requested configuration is unavailable
	 * then returns Specific error
	 */
	if (!(filter_avail)) {
		LOG_ERR("Rx filter unavailable");
		return -ENOSPC;
	}

	return filter_num;
}

/**
 * @fn      static void can_cast_remove_rx_filter(const struct device *dev,
 *                                                int filter_id)
 * @brief   Removes requested Rx filter
 * @param[in]   dev       : pointer to Runtime device structure
 * @param[in]   filter_id : filter number
 * @return  None
 */

static void can_cast_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_cast_data *data = DEV_DATA(dev);
	struct can_cast_filter_t *msg_filter = NULL;
	uint32_t can_base;

	/* Invalid Filter ID */
	if (filter_id >= CAN_MAX_ACCEPTANCE_FILTERS) {
		return;
	}

	/* If none of the filters configured then return */
	if (data->state.filter_configured == 0x0U) {
		return;
	}

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	msg_filter = &data->filter[filter_id];

	/* Resets and disables the filter */
	can_cast_disable_acpt_fltr(can_base, filter_id);

	msg_filter->rx_cb = NULL;
	msg_filter->rx_filter.id = 0U;
	msg_filter->rx_filter.mask = 0U;

	/* If at least one filter is configured then it sets the flag*/
	if (can_cast_acpt_fltr_configured(can_base)) {
		data->state.filter_configured = 0x1U;
	} else {
		data->state.filter_configured = 0x0U;
	}

	k_mutex_unlock(&data->inst_mutex);
}

#if defined(CONFIG_CAN_MANUAL_RECOVERY_MODE)
/**
 * @fn      static int can_cast_recover(const struct device *dev,
 *                                       k_timeout_t timeout)
 * @brief   Removes requested Rx filter
 * @param[in]   dev      : pointer to Runtime device structure
 * @param[in]   timeout  : Timeout for recovery
 * @return  Execution status
 */
static int can_cast_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	uint64_t usec_timeout;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if (data->err_state != CAN_STATE_BUS_OFF) {
		return 0;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
	}

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	can_cast_turn_on_bus(can_base);

	usec_timeout = k_ticks_to_us_near64(timeout.ticks);

	while (usec_timeout) {
		if (can_cast_get_bus_status(can_base) == CAN_BUS_STATUS_ON) {
			data->err_state = CAN_STATE_ERROR_ACTIVE;
			return 0;
		}
		k_usleep(100);
		usec_timeout -= 100;
	}

	LOG_ERR("Recover Timed out");
	return -EAGAIN;
}
#endif

/**
 * @fn      static int can_cast_get_state(const struct device *dev,
 *                                        enum can_state *state,
 *                                        struct can_bus_err_cnt * err_cnt)
 * @brief   Get current state
 * @param[in]   dev     : pointer to Runtime device structure
 * @param[in]   state   : pointer to device state
 * @param[in]   err_cnt : pointer to error count
 * @return  Execution status
 */

static int can_cast_get_state(const struct device *dev, enum can_state *state,
			      struct can_bus_err_cnt *err_cnt)
{
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;

	if ((!state) || (!err_cnt)) {
		return -EINVAL;
	}

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	*state = data->err_state;
	err_cnt->tx_err_cnt = can_cast_get_tx_error_count(can_base);
	err_cnt->rx_err_cnt = can_cast_get_rx_error_count(can_base);

	return 0;
}

/**
 * @fn      static int can_cast_set_state_change_cb(const struct device *dev,
 *                                               can_state_change_callback_t callback,
 *                                               void *user_data)
 * @brief   Store state change callback address
 * @param[in]   dev       : pointer to Runtime device structure
 * @param[in]   callback  : Callback function address
 * @param[in]   user_data : Callback user data pointer
 * @return  None
 */

static void can_cast_set_state_change_cb(const struct device *dev,
					 can_state_change_callback_t callback, void *user_data)
{
	struct can_cast_data *data = DEV_DATA(dev);

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;
}

/**
 * @fn      static int can_cast_get_core_clk(const struct device *dev,
 *                                           uint32_t *rate)
 * @brief   Get input clock rate
 * @param[in]   dev  : pointer to Runtime device structure
 * @param[in]   rate : pointer to clock-speed
 * @return  Execution status
 */

static int can_cast_get_core_clk(const struct device *dev, uint32_t *rate)
{
	const struct can_cast_config *config = dev->config;

	if (!rate) {
		return -EINVAL;
	}

	/* Get CAN clock rate */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	int ret;

	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("clock controller device not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clk_dev, config->clk_id, rate);
	if (ret != 0) {
		LOG_ERR("Unable to get clock rate: err:%d", ret);
		return ret;
	}
#else
	*rate = config->clk_freq;
#endif

	return 0;
}

/**
 * @fn      static int can_cast_get_max_filters(const struct device *dev,
 *                                              bool ide)
 * @brief   Get max Rx-filters
 * @param[in]   dev : pointer to Runtime device structure
 * @param[in]   ide : Identifier
 * @return  Maximum CAN Rx filters
 */

static int can_cast_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CAN_MAX_ACCEPTANCE_FILTERS;
}

/**
 *@fn          static int can_cast_receive(uint32_t can_base,
 *                                         struct can_frame *dest_frame)
 * @brief       Fetches the data from Rx buffer
 * @param[in]   can_base   : Base address of CAN register map
 * @param[in]   dest_frame : Destination Data frame pointer
 * @return      none
 */
static int can_cast_receive(uint32_t can_base, struct can_frame *dest_frame)
{
	uint8_t iter;
	uint8_t loc_var;

	volatile const struct rbuf_regs_t *rx_msg =
		(volatile const struct rbuf_regs_t *)(can_base + CAN_RBUF);

	dest_frame->id = (rx_msg->can_id & (~CAN_MSG_ESI_Msk));

	dest_frame->flags |= (((rx_msg->can_id >> CAN_MSG_ESI_Pos) & 1U) ? CAN_FRAME_ESI : 0);
	dest_frame->flags |= (((rx_msg->control >> CAN_MSG_IDE_Pos) & 1U) ? CAN_FRAME_IDE : 0);
	dest_frame->flags |= (((rx_msg->control >> CAN_MSG_RTR_Pos) & 1U) ? CAN_FRAME_RTR : 0);
	dest_frame->flags |= (((rx_msg->control >> CAN_MSG_FDF_Pos) & 1U) ? CAN_FRAME_FDF : 0);
	dest_frame->flags |= (((rx_msg->control >> CAN_MSG_BRS_Pos) & 1U) ? CAN_FRAME_BRS : 0);

#if !CONFIG_CAN_ACCEPT_RTR
	if (dest_frame->flags & CAN_FRAME_RTR) {
		return -ENOTSUP;
	}
#endif

	dest_frame->dlc = ((rx_msg->control >> CAN_MSG_DLC_Pos) & 0xFU);

	loc_var = can_dlc_to_bytes(dest_frame->dlc);
	if (loc_var > CAN_MAX_DLEN) {
		return -ENOTSUP;
	}

	/* Copy the data*/
	for (iter = 0U; iter < loc_var; iter++) {
		dest_frame->data[iter] = rx_msg->data[iter];
	}

#if CONFIG_CAN_RX_TIMESTAMP
	dest_frame->timestamp = rx_msg->rx_timestamp[0U];
#endif

	/* Release the buffer */
	loc_var = sys_read8(can_base + CAN_RCTRL);
	loc_var |= BIT(CAN_RCTRL_RREL);
	sys_write8(loc_var, (can_base + CAN_RCTRL));

	return 0;
}

/**
 *@fn          static void can_cast_handle_rx_frame(const struct device *dev)
 *@brief       Checks and shares Rx message using app callback function
 *@param[in]   dev      : pointer to Runtime device structure
 *@return      none
 */
static void can_cast_handle_rx_frame(const struct device *dev)
{
	uint32_t can_base;
	struct can_cast_data *data = DEV_DATA(dev);
	struct can_frame rx_frame = {0};
	struct can_cast_filter_t *filter = NULL;
	uint8_t iter;

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	if (can_cast_receive(can_base, &rx_frame) != 0) {
		return;
	}

	for (iter = 0; iter < CONFIG_CAN_MAX_FILTER; iter++) {
		filter = &data->filter[iter];
		if ((filter->rx_cb != NULL) &&
		    (can_frame_matches_filter(&rx_frame, &filter->rx_filter))) {
			filter->rx_cb(dev, &rx_frame, filter->cb_arg);
			break;
		}
	}
}

/**
 *@fn          void can_cast_chk_last_err_code(const struct device *dev,
 *                                             uint32_t can_base)
 *@brief       Fetches the latest error occurred
 *@param[in]   can_base :  Base address of CAN register map
 *@return      none
 */
static void can_cast_chk_last_err_code(const struct device *dev, uint32_t can_base)
{
	struct can_cast_data *data = DEV_DATA(dev);
	uint8_t error;

	error = ((sys_read8(can_base + CAN_EALCAP) & CAN_EALCAP_KOER_Msk) >> CAN_EALCAP_KOER_Pos);
	switch (error) {
	case CAN_EALCAP_KOER_BIT:
		CAN_STATS_BIT_ERROR_INC(dev);
		break;

	case CAN_EALCAP_KOER_FORM:
		CAN_STATS_FORM_ERROR_INC(dev);
		break;

	case CAN_EALCAP_KOER_STUFF:
		CAN_STATS_STUFF_ERROR_INC(dev);
		break;

	case CAN_EALCAP_KOER_ACK:
		/* Checks if the mode is Listen Only and process data
		 * if true
		 */
		if (data->common.mode & CAN_MODE_LISTENONLY) {
			can_cast_handle_rx_frame(dev);
		}
		CAN_STATS_ACK_ERROR_INC(dev);
		break;

	case CAN_EALCAP_KOER_CRC:
		CAN_STATS_CRC_ERROR_INC(dev);
		break;

	case CAN_EALCAP_KOER_OTHER:
	case CAN_EALCAP_KOER_NONE:
		break;
	}
}

/**
 *@fn          static void can_cast_handle_error(const struct device *dev,
 *                                               uint32_t event)
 *@brief       Performs error handling
 *@param[in]   dev   : pointer to Runtime device structure
 *@param[in]   event : Error Event
 *@return      none
 */
static void can_cast_handle_error(const struct device *dev, uint32_t event)
{
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	enum can_state state = data->err_state;
	struct can_bus_err_cnt err_cnt;

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	err_cnt.tx_err_cnt = can_cast_get_tx_error_count(can_base);
	err_cnt.rx_err_cnt = can_cast_get_rx_error_count(can_base);

	if (event & CAN_ARBTR_LOST_EVENT) {
		if (data->tx_queue.tail != data->tx_queue.head) {
			data->tx_queue.cb_list[data->tx_queue.tail].cb(
				dev, 1, data->tx_queue.cb_list[data->tx_queue.tail].cb_arg);
			if (can_cast_stb_single_shot_mode(can_base)) {
				QUEUE_TAIL_NEXT(data->tx_queue.tail);
			}
		}
		CAN_STATS_BIT1_ERROR_INC(dev);
	} else if (event & CAN_ERROR_PASSIVE_EVENT) {
		if (can_cast_error_passive_mode(can_base)) {
			/* Sets the state Error Passive */
			state = CAN_STATE_ERROR_PASSIVE;
		} else {
			/* Sets the state Error Active */
			state = CAN_STATE_ERROR_ACTIVE;
		}
	} else if (event & CAN_ERROR_EVENT) {
		if (can_cast_get_bus_status(can_base) == CAN_BUS_STATUS_OFF) {
			/* Abort all pending transmissions */
			can_cast_abort_tx(can_base);

			/* Reset Tx queue */
			data->tx_queue.head = 0U;
			data->tx_queue.tail = 0U;

			/* Sets the state to Bus off */
			state = CAN_STATE_BUS_OFF;
		} else if (can_cast_err_warn_limit_reached(can_base) == true) {
			/* Sets the state to Error Warning */
			state = CAN_STATE_ERROR_WARNING;
		}
	}

	if (event & CAN_BUS_ERROR_EVENT) {
		can_cast_chk_last_err_code(dev, can_base);
	}

	if (data->err_state != state) {
		data->err_state = state;
		if (data->common.state_change_cb) {
			data->common.state_change_cb(dev, data->err_state, err_cnt,
						     data->common.state_change_cb_user_data);
		}
	}
}

/**
 * @fn          static void can_cast_clear_interrupt(uint32_t can_base,
 *                                                   const uint32_t event)
 * @brief       Clears the interrupt
 * @param[in]   can_base : Base address of CAN register map
 * @param[in]   event : Interrupt event
 * @return      none
 */
static void can_cast_clear_interrupt(uint32_t can_base, const uint32_t event)
{
	uint8_t temp = (uint8_t)event;

	if (event & CAN_RTIF_REG_Msk) {
		if (event & BIT(CAN_RTIF_RIF)) {
			if (can_cast_rx_msg_available(can_base)) {
				/* If Rx data is still available
				 * then this interrupt won't be cleared
				 */
				temp &= ~(BIT(CAN_RTIF_RIF));
			}
		}
		/* Clears Data interrupt */
		sys_write8((temp & CAN_RTIF_REG_Msk), (can_base + CAN_RTIF));
		(void)sys_read8(can_base + CAN_RTIF);
	} else if ((event >> 8U) & CAN_ERRINT_REG_Msk) {
		/* Clears Error interrupt */
		temp = (sys_read8(can_base + CAN_ERRINT) & CAN_ERRINT_EN_Msk);
		temp |= ((event >> 8U) & CAN_ERRINT_REG_Msk);
		sys_write8(temp, (can_base + CAN_ERRINT));
		(void)sys_read8(can_base + CAN_ERRINT);
	}
}

/**
 * @fn          static uint32_t can_cast_get_irq_event(uint32_t can_base)
 * @brief       Returns the interrupt event
 * @param[in]   can_base : Base address of CAN register map
 * @return      CAN interrupt event
 */
static uint32_t can_cast_get_irq_event(uint32_t can_base)
{
	uint32_t event = 0U;

	event = (sys_read8(can_base + CAN_RTIF) & CAN_RTIF_REG_Msk);
	if (!event) {
		event = ((sys_read8(can_base + CAN_ERRINT) & CAN_ERRINT_REG_Msk) << 8U);
	}
	return event;
}

/**
 * @fn      static int can_cast_irq_handler(const struct device *dev)
 * @brief   Handle CAN Interrupt
 * @param[in]   dev : pointer to Runtime device structure
 * @return  None
 */
void can_cast_irq_handler(const struct device *dev)
{
	struct can_cast_data *data = DEV_DATA(dev);
	uint32_t can_base;
	uint32_t irq_event;

	can_base = DEVICE_MMIO_NAMED_GET(dev, can_reg);

	irq_event = can_cast_get_irq_event(can_base);

	if (irq_event & CAN_RBUF_OVERRUN_EVENT) {
		/* If the Rbuf is overrun then performs below operation */
		can_cast_handle_rx_frame(dev);
		irq_event = (CAN_RBUF_OVERRUN_EVENT | CAN_RBUF_FULL_EVENT |
			     CAN_RBUF_ALMOST_FULL_EVENT | CAN_RBUF_AVAILABLE_EVENT);
		CAN_STATS_RX_OVERRUN_INC(dev);
	} else if (irq_event & CAN_RBUF_ALMOST_FULL_EVENT) {
		/* If the Rx buffer is almost full then performs below operation */
		can_cast_handle_rx_frame(dev);
		irq_event = (CAN_RBUF_AVAILABLE_EVENT | CAN_RBUF_FULL_EVENT |
			     CAN_RBUF_ALMOST_FULL_EVENT);
	} else if (irq_event & CAN_RBUF_AVAILABLE_EVENT) {
		/* If the Rx msg available then performs below operation */
		can_cast_handle_rx_frame(dev);
		irq_event = (CAN_RBUF_AVAILABLE_EVENT | CAN_RBUF_FULL_EVENT |
			     CAN_RBUF_ALMOST_FULL_EVENT);
	} else if (irq_event & CAN_SECONDARY_BUF_TX_COMPLETE_EVENT) {
		/* If the Secondary buf Tx interrupt is occurred
		 * then performs below operation
		 */
		irq_event = CAN_SECONDARY_BUF_TX_COMPLETE_EVENT;
		if (data->tx_queue.tail != data->tx_queue.head) {
			data->tx_queue.cb_list[data->tx_queue.tail].cb(
				dev, 0, data->tx_queue.cb_list[data->tx_queue.tail].cb_arg);
			QUEUE_TAIL_NEXT(data->tx_queue.tail);
		}
	} else {
		can_cast_handle_error(dev, irq_event);
	}

	/* Invokes the low level api to clear the interrupt */
	can_cast_clear_interrupt(can_base, irq_event);
}

/* CAN Driver API structure */
struct can_driver_api can_cast_driver_api = {
	.get_capabilities = can_cast_get_capabilities,
	.start = can_cast_start,
	.stop = can_cast_stop,
	.set_mode = can_cast_set_mode,
	.set_timing = can_cast_set_timing,
	.send = can_cast_send,
	.add_rx_filter = can_cast_add_rx_filter,
	.remove_rx_filter = can_cast_remove_rx_filter,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_cast_recover,
#endif
	.get_state = can_cast_get_state,
	.set_state_change_callback = can_cast_set_state_change_cb,
	.get_core_clock = can_cast_get_core_clk,
	.get_max_filters = can_cast_get_max_filters,
	.timing_min = CAN_MIN_BIT_TIME,
	.timing_max = CAN_MAX_BIT_TIME,
#if CONFIG_CAN_FD_MODE
	.set_timing_data = can_cast_set_timing_data,
	.timing_data_min = CAN_MIN_BIT_TIME_DATA,
	.timing_data_max = CAN_MAX_BIT_TIME_DATA,
#endif
};

#define CAN_CAST_MAX_BITRATE(inst)                                                                 \
	COND_CODE_1(DT_INST_HAS_PROP(inst, max_bitrate), DT_INST_PROP(inst, max_bitrate),          \
				(CAN_MAX_BITRATE))

#define CAN_CAST_CLOCK_INIT(inst)                                                                  \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, clocks),                                            \
		(.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                              \
		 .clk_id  = COND_CODE_1(DT_INST_PHA_HAS_CELL(inst, clocks, clkid),                 \
					((clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid)),\
					((clock_control_subsys_t)0)),))                            \
		 .clk_freq = DT_INST_PROP(inst, clock_frequency)

#define CAN_CAST_FD_CONFIG_INIT(inst)                                                              \
	COND_CODE_1(DT_INST_PROP(inst, can_fd),                                                    \
		(                                                                                  \
		.can_fd = true,                                                                    \
		.can_fd_ctrl_reg = DT_INST_PROP(inst, can_fd_ctrl_reg),                            \
		.can_fd_bit = DT_INST_PROP(inst, can_fd_bit),                                      \
		),                                                                                 \
		(                                                                                  \
		.can_fd = false,                                                                   \
		)                                                                                  \
	)

/* CAN Initialisation */
#define CAN_CAST_INIT(inst)                                                                        \
	BUILD_ASSERT((CAN_MAX_ACCEPTANCE_FILTERS > CONFIG_CAN_MAX_FILTER),                         \
				"Invalid number of acceptance filters chosen");                    \
	BUILD_ASSERT((CONFIG_CAN_RBUF_AFWL <= CAN_RBUF_AFWL_MAX),                                  \
				"Invalid Rx buf Almost warning limit");                            \
	static void can_cast_config_func_##inst(const struct device *dev);                         \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, pinctrl_0), (PINCTRL_DT_INST_DEFINE(inst)));        \
	static const struct can_cast_config config_##inst = {                                      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(can_reg, DT_DRV_INST(inst)),                    \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(can_cnt_reg, DT_DRV_INST(inst)),                \
		.common = {                                                                        \
			.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, phys)),                 \
			.max_bitrate  = CAN_CAST_MAX_BITRATE(inst),                                \
			.bitrate = DT_INST_PROP_OR(inst, bitrate, CONFIG_CAN_DEFAULT_BITRATE),     \
			.sample_point = DT_INST_PROP_OR(inst, sample_point, 0),                    \
			IF_ENABLED(CONFIG_CAN_FD_MODE,                                             \
			(.bitrate_data = DT_INST_PROP_OR(inst, bitrate_data,                       \
						CONFIG_CAN_DEFAULT_BITRATE_DATA),                  \
			.sample_point_data = DT_INST_PROP_OR(inst, sample_point_data, 0)))},       \
		CAN_CAST_CLOCK_INIT(inst),                                                         \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, pinctrl_0),                                 \
		(.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(inst)))),                           \
		CAN_CAST_FD_CONFIG_INIT(inst)                                                      \
		.irq_config_func = can_cast_config_func_##inst};                                   \
	static struct can_cast_data data_##inst;                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_cast_init, NULL, &data_##inst, &config_##inst,         \
	POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, &can_cast_driver_api);                              \
                                                                                                   \
	static void can_cast_config_func_##inst(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority),                   \
					can_cast_irq_handler, DEVICE_DT_INST_GET(inst), 0);        \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
	}
DT_INST_FOREACH_STATUS_OKAY(CAN_CAST_INIT)
