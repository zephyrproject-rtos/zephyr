/*
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/i3c.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>

#include <stdlib.h>

#include "i3c_cdns.h"


#define LOG_MODULE_NAME I3C_CADENCE
LOG_MODULE_REGISTER(I3C_CADENCE, CONFIG_I3C_CADENCE_LOG_LEVEL);

/*******************************************************************************
 * Global Variables Declaration
 ******************************************************************************/

/*******************************************************************************
 * Local Functions Declaration
 ******************************************************************************/

/*******************************************************************************
 * Private Functions Code
 ******************************************************************************/

#ifdef CONFIG_I3C_TARGET
static void cdns_i3c_write_ddr_tx_fifo(const struct cdns_i3c_config *config, const void *buf,
				       uint32_t len)
{
	const uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		val = *ptr++;
		sys_write32(val, config->base + SLV_DDR_TX_FIFO);
	}

	if (remain > 0) {
		val = 0;
		memcpy(&val, ptr, remain);
		sys_write32(val, config->base + SLV_DDR_TX_FIFO);
	}
}
#endif /* CONFIG_I3C_TARGET */

#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_TARGET
static void cdns_i3c_write_ibi_fifo(const struct cdns_i3c_config *config, const void *buf,
				    uint32_t len)
{
	const uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		val = *ptr++;
		sys_write32(val, config->base + IBI_DATA_FIFO);
	}

	if (remain > 0) {
		val = 0;
		memcpy(&val, ptr, remain);
		sys_write32(val, config->base + IBI_DATA_FIFO);
	}
}
#endif /* CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_USE_IBI */
#ifdef CONFIG_I3C_TARGET
#ifndef CONFIG_I3C_TARGET_BUFFER_MODE
static void cdns_i3c_target_read_rx_fifo(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	const struct i3c_target_callbacks *target_cb = data->target_config->callbacks;

	/* Version 1p7 uses the full 32b FIFO width */
	if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
		uint16_t xferred_bytes =
			SLV_STATUS0_XFRD_BYTES(sys_read32(config->base + SLV_STATUS0));

		for (int i = data->fifo_bytes_read; i < xferred_bytes; i += 4) {
			uint32_t rx_data = sys_read32(config->base + RX_FIFO);
			/* Call write received cb for each remaining byte  */
			for (int j = 0; j < MIN(4, xferred_bytes - i); j++) {
				target_cb->write_received_cb(data->target_config,
							     (rx_data >> (8 * j)));
			}
		}
		/*
		 * store the xfer bytes as the thr interrupt may trigger again as xferred_bytes will
		 * count up to the "total" bytes received
		 */
		data->fifo_bytes_read = xferred_bytes;
	} else {
		/*
		 * Target writes only write to the first byte of the 32 bit
		 * width fifo for older version
		 */
		uint8_t rx_data = (uint8_t)sys_read32(config->base + RX_FIFO);

		target_cb->write_received_cb(data->target_config, rx_data);
	}
}
#endif /* !CONFIG_I3C_TARGET_BUFFER_MODE */
#endif /* CONFIG_I3C_TARGET */
#ifdef CONFIG_I3C_CONTROLLER
static void cdns_i3c_set_prescalers(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;

	/* These formulas are from section 6.2.1 of the Cadence I3C Master User Guide. */
	uint32_t prescl_i3c = DIV_ROUND_UP(config->input_frequency,
					   (ctrl_config->scl.i3c * I3C_PRESCL_REG_SCALE)) -
			      1;
	uint32_t prescl_i2c = DIV_ROUND_UP(config->input_frequency,
					   (ctrl_config->scl.i2c * I2C_PRESCL_REG_SCALE)) -
			      1;

	/*
	 * If scl_od_min.high_ns is set, ensure T/2 >= high_ns by increasing prescaler if needed.
	 * OD high period = T/2 = (prescl_i3c + 1) * 2e9 / sys_clk_freq (in ns).
	 * This also affects push-pull speed since both share PRESCL_CTRL0.i3c.
	 */
	if (ctrl_config->scl_od_min.high_ns > 0) {
		uint32_t prescl_i3c_for_high = DIV_ROUND_UP(
			(uint64_t)ctrl_config->scl_od_min.high_ns * config->input_frequency,
			2 * (uint64_t)NSEC_PER_SEC) - 1;
		if (prescl_i3c_for_high > prescl_i3c) {
			LOG_WRN("%s: Increasing I3C prescaler from %u to %u to meet"
				" OD SCL high minimum of %uns,"
				" push-pull speed will also be reduced",
				dev->name, prescl_i3c, prescl_i3c_for_high,
				ctrl_config->scl_od_min.high_ns);
			prescl_i3c = prescl_i3c_for_high;
		}
	}

	/* update with actual value */
	ctrl_config->scl.i3c = config->input_frequency / ((prescl_i3c + 1) * I3C_PRESCL_REG_SCALE);
	ctrl_config->scl.i2c = config->input_frequency / ((prescl_i2c + 1) * I2C_PRESCL_REG_SCALE);

	LOG_DBG("%s: I3C speed = %u, PRESCL_CTRL0.i3c = 0x%x", dev->name, ctrl_config->scl.i3c,
		prescl_i3c);
	LOG_DBG("%s: I2C speed = %u, PRESCL_CTRL0.i2c = 0x%x", dev->name, ctrl_config->scl.i2c,
		prescl_i2c);

	/* Use scl_od_min.low_ns if set, otherwise default to I3C_OD_TLOW_MIN_NS */
	uint32_t od_low_ns = ctrl_config->scl_od_min.low_ns;

	if (od_low_ns == 0) {
		od_low_ns = I3C_OD_TLOW_MIN_NS;
	} else if (od_low_ns < I3C_OD_TLOW_MIN_NS) {
		LOG_WRN("%s: scl_od_min.low_ns (%u) below spec minimum (%u), clamping",
			dev->name, od_low_ns, I3C_OD_TLOW_MIN_NS);
		od_low_ns = I3C_OD_TLOW_MIN_NS;
	}

	/* pres_step is the resolution of PRESCL_CTRL1.od_low in nanoseconds (T/4) */
	uint32_t pres_step = NSEC_PER_SEC / (ctrl_config->scl.i3c * 4);
	int32_t od_low = DIV_ROUND_UP(od_low_ns, pres_step) - 2;

	if (od_low < 0) {
		od_low = 0;
	}
	LOG_DBG("%s: PRESCL_CTRL1.od_low = 0x%x", dev->name, od_low);

	/* disable in order to update timing */
	uint32_t ctrl = sys_read32(config->base + CTRL);

	if (ctrl & CTRL_DEV_EN) {
		sys_write32(~CTRL_DEV_EN & ctrl, config->base + CTRL);
	}

	sys_write32(PRESCL_CTRL0_I3C(prescl_i3c) | PRESCL_CTRL0_I2C(prescl_i2c),
		    config->base + PRESCL_CTRL0);

	/* Sets the open drain low time relative to the push-pull. */
	sys_write32(PRESCL_CTRL1_OD_LOW(od_low & PRESCL_CTRL1_OD_LOW_MASK),
		    config->base + PRESCL_CTRL1);

	/* reenable */
	if (ctrl & CTRL_DEV_EN) {
		sys_write32(CTRL_DEV_EN | ctrl, config->base + CTRL);
	}
}
/**
 * @brief Compute RR0 Value from addr
 *
 * @param addr Address of the target
 *
 * @return RR0 value
 */
static uint32_t prepare_rr0_dev_address(uint16_t addr)
{
	/* RR0[7:1] = addr[6:0] | parity^[0] */
	uint32_t ret = cdns_i3c_even_parity_byte(addr);

	if (addr & GENMASK(9, 7)) {
		/* RR0[15:13] = addr[9:7] */
		ret |= (addr & GENMASK(9, 7)) << 6;
		/* RR0[11] = 10b lvr addr */
		ret |= DEV_ID_RR0_LVR_EXT_ADDR;
	}

	return ret;
}
#endif /* CONFIG_I3C_CONTROLLER */

#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Program Retaining Registers with device lists
 *
 * This will program the retaining register with the controller itself, this should
 * only be called if it is a primary controller.
 *
 * @param dev Pointer to controller device driver instance.
 */
static void cdns_i3c_program_controller_retaining_reg(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	uint8_t controller_da;

	/* Set controller retaining register */
	if (config->common.primary_controller_da) {
		if (!i3c_addr_slots_is_free(&data->common.attached_dev.addr_slots,
					    config->common.primary_controller_da)) {
			controller_da = i3c_addr_slots_next_free_find(
				&data->common.attached_dev.addr_slots, 0);
			LOG_WRN("%s: 0x%02x DA selected for controller as 0x%02x is unavailable",
				dev->name, controller_da, config->common.primary_controller_da);
		} else {
			controller_da = config->common.primary_controller_da;
		}
	} else {
		controller_da =
			i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots, 0);
	}
	sys_write32(prepare_rr0_dev_address(controller_da) | DEV_ID_RR0_IS_I3C,
		    config->base + DEV_ID_RR0(0));
	/* Mark the address as I3C device */
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, controller_da);
}
#endif /* CONFIG_I3C_CONTROLLER */

#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_CONTROLLER
static int cdns_i3c_ibi_hj_response(const struct device *dev, bool ack)
{
	const struct cdns_i3c_config *config = dev->config;

	if (ack) {
		sys_write32(CTRL_HJ_ACK | sys_read32(config->base + CTRL), config->base + CTRL);
	} else {
		sys_write32(~CTRL_HJ_ACK & sys_read32(config->base + CTRL), config->base + CTRL);
	}

	return 0;
}

static int cdns_i3c_controller_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	uint32_t sir_map;
	uint32_t sir_cfg;
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_i2c_dev_data *cdns_i3c_device_data = target->controller_priv;
	struct i3c_ccc_events i3c_events;
	int ret = 0;

	/* Check if the device can issue IBI TIRs or CR */
	if (!i3c_device_is_ibi_capable(target) && !i3c_device_is_controller_capable(target)) {
		ret = -EINVAL;
		return ret;
	}

	/* TODO: check for duplicate in SIR */

	sir_cfg = SIR_MAP_DEV_ROLE(I3C_BCR_DEVICE_ROLE(target->bcr)) |
		  SIR_MAP_DEV_DA(target->dynamic_addr);
	if (i3c_ibi_has_payload(target)) {
		/*
		 * the I3C spec says that a len of 0x00, means no limit, but the cdns i3c doesn't
		 * reconigize stops when loading a new word in the FIFO, so if multiple ibis come in
		 * quick succession, then they may be all in the same fifo word and may not be read
		 * correctly.
		 */
		if (target->data_length.max_ibi == 0x00) {
			sir_cfg |= SIR_MAP_DEV_PL(
				MIN(SIR_MAP_PL_MAX, CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE));
		} else {
			sir_cfg |= SIR_MAP_DEV_PL(target->data_length.max_ibi);
		}
	} else {
		/* Set to 0 for no ibi payload */
		sir_cfg |= SIR_MAP_DEV_PL(0);
	}
	/* ACK if there is an ibi tir cb or if it is controller capable*/
	if ((target->ibi_cb != NULL) || i3c_device_is_controller_capable(target)) {
		sir_cfg |= SIR_MAP_DEV_ACK;
	}
	if (target->bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT) {
		sir_cfg |= SIR_MAP_DEV_SLOW;
	}

	LOG_DBG("%s: IBI enabling for 0x%02x (BCR 0x%02x)", dev->name, target->dynamic_addr,
		target->bcr);

	/* Tell target to enable IBI TIRs and CRs */
	i3c_events.events = I3C_CCC_EVT_INTR | I3C_CCC_EVT_CR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI ENEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	sir_map = sys_read32(config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));
	sir_map &= ~SIR_MAP_DEV_CONF_MASK(cdns_i3c_device_data->id - 1);
	sir_map |= SIR_MAP_DEV_CONF(cdns_i3c_device_data->id - 1, sir_cfg);

	sys_write32(sir_map, config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));

	return ret;
}

static int cdns_i3c_controller_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	uint32_t sir_map;
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_i2c_dev_data *cdns_i3c_device_data = target->controller_priv;
	struct i3c_ccc_events i3c_events;
	int ret = 0;

	if (!i3c_device_is_ibi_capable(target)) {
		ret = -EINVAL;
		return ret;
	}

	/* Tell target to disable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI DISEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	sir_map = sys_read32(config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));
	sir_map &= ~SIR_MAP_DEV_CONF_MASK(cdns_i3c_device_data->id - 1);
	sir_map |=
		SIR_MAP_DEV_CONF(cdns_i3c_device_data->id - 1, SIR_MAP_DEV_DA(I3C_BROADCAST_ADDR));
	sys_write32(sir_map, config->base + SIR_MAP_DEV_REG(cdns_i3c_device_data->id - 1));

	return ret;
}
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
static int cdns_i3c_target_ibi_raise_hj(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;

	/* HJ requests should not be done by primary controllers */
	if (!ctrl_config->is_secondary) {
		LOG_ERR("%s: controller is primary, HJ not available", dev->name);
		return -ENOTSUP;
	}
	/* Check if target already has a DA assigned to it */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_HAS_DA) {
		LOG_ERR("%s: HJ not available, DA already assigned", dev->name);
		return -EACCES;
	}
	/* Check if HJ requests DISEC CCC with DISHJ field set has been received */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_HJ_DIS) {
		LOG_ERR("%s: HJ requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	sys_write32(CTRL_HJ_INIT | sys_read32(config->base + CTRL), config->base + CTRL);
	k_sem_reset(&data->ibi_hj_complete);
	if (k_sem_take(&data->ibi_hj_complete, K_MSEC(500)) != 0) {
		LOG_ERR("%s: timeout waiting for DAA after HJ", dev->name);
		return -ETIMEDOUT;
	}
	return 0;
}
#ifdef CONFIG_I3C_CONTROLLER
static int cdns_i3c_target_ibi_raise_cr(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;

	/* Check if target does not have a DA assigned to it */
	if (!(sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_HAS_DA)) {
		LOG_ERR("%s: CR not available, DA not assigned", dev->name);
		return -EACCES;
	}
	/* Check if CR requests DISEC CCC with DISMR field set has been received */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_MR_DIS) {
		LOG_ERR("%s: CR requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	sys_write32(CTRL_MST_INIT | sys_read32(config->base + CTRL), config->base + CTRL);
	k_sem_reset(&data->ibi_cr_complete);
	if (k_sem_take(&data->ibi_cr_complete, K_MSEC(500)) != 0) {
		LOG_ERR("%s: timeout waiting for GETACCCR after CR", dev->name);
		return -ETIMEDOUT;
	}
	return 0;
}
#endif /* CONFIG_I3C_CONTROLLER */
static int cdns_i3c_target_ibi_raise_intr(const struct device *dev, struct i3c_ibi *request)
{
	const struct cdns_i3c_config *config = dev->config;
	const struct cdns_i3c_data *data = dev->data;
	uint32_t ibi_ctrl_val;

	LOG_DBG("%s: issuing IBI TIR", dev->name);

	/* Check if target does not have a DA assigned to it */
	if (!(sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_HAS_DA)) {
		LOG_ERR("%s: TIR not available, DA not assigned", dev->name);
		return -EACCES;
	}
	/* Check if TIR requests DISEC CCC with DISMR field set has been received */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_IBI_DIS) {
		LOG_ERR("%s: TIR requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	/*
	 * Ensure data will fit within FIFO
	 *
	 * TODO: This limitation prevents burst transfers greater than the
	 *       FIFO sizes and should be replaced with an implementation that
	 *       utilizes the IBI data threshold interrupts.
	 */
	if (request->payload_len > data->hw_cfg.ibi_mem_depth) {
		LOG_ERR("%s: payload too large for IBI TIR", dev->name);
		return -ENOMEM;
	}

	cdns_i3c_write_ibi_fifo(config, request->payload, request->payload_len);

	/* Write Payload Length and Start Condition */
	ibi_ctrl_val = sys_read32(config->base + SLV_IBI_CTRL);
	ibi_ctrl_val |= SLV_IBI_PL(request->payload_len);
	ibi_ctrl_val |= SLV_IBI_REQ;
	sys_write32(ibi_ctrl_val, config->base + SLV_IBI_CTRL);
	return 0;
}

static int cdns_i3c_target_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	int ret;

	__ASSERT_NO_MSG(request != NULL);

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	pm_device_busy_set(dev);

	/* make sure we are not currently the active controller */
	if (sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE) {
		ret = -EACCES;
		goto error;
	}

	switch (request->ibi_type) {
	case I3C_IBI_TARGET_INTR:
		/* Check IP Revision since older versions of CDNS IP do not support IBI interrupt*/
		if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
			ret = cdns_i3c_target_ibi_raise_intr(dev, request);
		} else {
			ret = -ENOTSUP;
		}
		break;
	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
#ifdef CONFIG_I3C_CONTROLLER
		ret = cdns_i3c_target_ibi_raise_cr(dev);
#else
		ret = -ENOTSUP;
#endif
		break;
	case I3C_IBI_HOTJOIN:
		ret = cdns_i3c_target_ibi_raise_hj(dev);
		break;
	default:
		ret = -EINVAL;
		break;
	}
error:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->bus_lock);

	return ret;
}
#endif /* CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_USE_IBI */

#ifdef CONFIG_I3C_CONTROLLER




/**
 * @brief Send Common Command Code (CCC).
 *
 * @see i3c_do_ccc
 *
 * @param dev Pointer to controller device driver instance.
 * @param payload Pointer to CCC payload.
 *
 * @return @see i3c_do_ccc
 */
static int cdns_i3c_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
#ifdef CONFIG_I3C_CADENCE_RTIO
	struct cdns_i3c_data *data = dev->data;

	return i3c_rtio_ccc(data->rtio->i3c_ctx, payload);
#else
	return cdns_i3c_do_ccc_do(dev, payload, false, NULL, NULL);
#endif
}

#ifdef CONFIG_I3C_CALLBACK
/**
 * @brief Send Common Command Code (CCC).
 *
 * @see i3c_do_ccc_cb
 *
 * @param dev Pointer to controller device driver instance.
 * @param payload Pointer to CCC payload.
 * @param cb Callback function
 * @param userdata User data for callback function
 *
 * @return @see i3c_do_ccc_cb
 */
static int cdns_i3c_do_ccc_cb(const struct device *dev, struct i3c_ccc_payload *payload,
			      i3c_callback_t cb, void *userdata)
{
#ifdef CONFIG_I3C_CADENCE_RTIO
	struct cdns_i3c_data *data = dev->data;

	return i3c_rtio_ccc_cb(data->rtio->i3c_ctx, dev, payload, cb, userdata);
#else
	return cdns_i3c_do_ccc_do(dev, payload, true, cb, userdata);
#endif
}
#endif

/**
 * @brief Perform Dynamic Address Assignment.
 *
 * @see i3c_do_daa
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @return @see i3c_do_daa
 */
static int cdns_i3c_do_daa(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	uint8_t last_addr = 0;

	/* read dev active reg */
	uint32_t olddevs = sys_read32(config->base + DEVS_CTRL) & DEVS_CTRL_DEVS_ACTIVE_MASK;
	/* ignore the controller register */
	olddevs |= BIT(0);

	/* Assign dynamic addresses to available RRs */
	/* Loop through each clear bit */
	for (uint8_t i = find_lsb_set(~olddevs); i <= data->max_devs; i++) {
		uint8_t rr_idx = i - 1;

		if (~olddevs & BIT(rr_idx)) {
			/* Read RRx registers */
			last_addr = i3c_addr_slots_next_free_find(
				&data->common.attached_dev.addr_slots, last_addr + 1);
			/* Write RRx registers */
			sys_write32(prepare_rr0_dev_address(last_addr) | DEV_ID_RR0_IS_I3C,
				    config->base + DEV_ID_RR0(rr_idx));
			sys_write32(0, config->base + DEV_ID_RR1(rr_idx));
			sys_write32(0, config->base + DEV_ID_RR2(rr_idx));
		}
	}

	/* the Cadence I3C IP will assign an address for it from the RR */
	struct i3c_ccc_payload entdaa_ccc;

	memset(&entdaa_ccc, 0, sizeof(entdaa_ccc));
	entdaa_ccc.ccc.id = I3C_CCC_ENTDAA;

	int status = cdns_i3c_do_ccc(dev, &entdaa_ccc);

	if (status != 0) {
		return status;
	}

	/* read again dev active reg  */
	uint32_t newdevs = sys_read32(config->base + DEVS_CTRL) & DEVS_CTRL_DEVS_ACTIVE_MASK;
	/* look for new bits that were set */
	newdevs &= ~olddevs;

	if (newdevs) {
		/* loop through each set bit for new devices */
		for (uint8_t i = find_lsb_set(newdevs); i <= find_msb_set(newdevs); i++) {
			uint8_t rr_idx = i - 1;

			if (newdevs & BIT(rr_idx)) {
				/* Read RRx registers */
				uint32_t dev_id_rr0 = sys_read32(config->base + DEV_ID_RR0(rr_idx));
				uint32_t dev_id_rr1 = sys_read32(config->base + DEV_ID_RR1(rr_idx));
				uint32_t dev_id_rr2 = sys_read32(config->base + DEV_ID_RR2(rr_idx));

				uint64_t pid = ((uint64_t)dev_id_rr1 << 16) + (dev_id_rr2 >> 16);
				uint8_t dyn_addr = (dev_id_rr0 & 0xFE) >> 1;
				uint8_t bcr = dev_id_rr2 >> 8;
				uint8_t dcr = dev_id_rr2 & 0xFF;

				const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
				struct i3c_device_desc *target = i3c_device_find(dev, &i3c_id);

				if (!target) {
					/* Target found that is not known, allocate a desc */
					target = i3c_device_desc_alloc();
					if (target) {
						/*
						 * able to allocate a descriptor
						 * write all known values
						 */
						*(const struct device **)&target->bus = dev;
						*(uint64_t *)&target->pid = pid;
						target->dynamic_addr = dyn_addr;
						target->bcr = bcr;
						target->dcr = dcr;
						/* attach it to the slist */
						sys_slist_append(
							&data->common.attached_dev.devices.i3c,
							&target->node);

						data->cdns_i3c_i2c_priv_data[rr_idx].id = rr_idx;
						target->controller_priv =
							&(data->cdns_i3c_i2c_priv_data[rr_idx]);
					}

					LOG_INF("%s: PID 0x%012llx is not in registered device "
						"list, given DA 0x%02x",
						dev->name, pid, dyn_addr);
				} else {
					target->dynamic_addr = dyn_addr;
					target->bcr = bcr;
					target->dcr = dcr;

					data->cdns_i3c_i2c_priv_data[rr_idx].id = rr_idx;
					target->controller_priv =
						&(data->cdns_i3c_i2c_priv_data[rr_idx]);

					LOG_DBG("%s: PID 0x%012llx assigned dynamic address 0x%02x",
						dev->name, pid, dyn_addr);
				}
				i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots,
							dyn_addr);
			}
		}
	} else {
		LOG_DBG("%s: ENTDAA: No devices found", dev->name);
	}

	/* mark slot as not free, may already be set if already attached */
	data->free_rr_slots &= ~newdevs;

	/* Unmask Hot-Join request interrupts. HJ will send DISEC HJ from the CTRL value */
	struct i3c_ccc_events i3c_events;

	i3c_events.events = I3C_CCC_EVT_HJ;
	status = i3c_ccc_do_events_all_set(dev, true, &i3c_events);
	if (status != 0) {
		LOG_DBG("%s: Broadcast ENEC was NACK", dev->name);
	}

	return 0;
}

/**
 * @brief Configure I2C hardware.
 *
 * @param dev Pointer to controller device driver instance.
 * @param config Value of the configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid configure parameters.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int cdns_i3c_i2c_api_configure(const struct device *dev, uint32_t config)
{
	struct cdns_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		ctrl_config->scl.i2c = 100000;
		break;
	case I2C_SPEED_FAST:
		ctrl_config->scl.i2c = 400000;
		break;
	case I2C_SPEED_FAST_PLUS:
		ctrl_config->scl.i2c = 1000000;
		break;
	case I2C_SPEED_HIGH:
		ctrl_config->scl.i2c = 3400000;
		break;
	case I2C_SPEED_ULTRA:
		ctrl_config->scl.i2c = 5000000;
		break;
	default:
		break;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	pm_device_busy_set(dev);

	cdns_i3c_set_prescalers(dev);

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->bus_lock);

	return 0;
}
#endif /* CONFIG_I3C_CONTROLLER */

/**
 * @brief Configure I3C hardware.
 *
 * @param dev Pointer to controller device driver instance.
 * @param type Type of configuration parameters being passed
 *             in @p config.
 * @param config Pointer to the configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid configure parameters.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int cdns_i3c_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	if (type == I3C_CONFIG_CONTROLLER) {
#ifdef CONFIG_I3C_CONTROLLER
		struct cdns_i3c_data *data = dev->data;
		struct i3c_config_controller *ctrl_cfg = config;

		if ((ctrl_cfg->scl.i2c == 0U) || (ctrl_cfg->scl.i3c == 0U)) {
			return -EINVAL;
		}

		data->common.ctrl_config.scl.i3c = ctrl_cfg->scl.i3c;
		data->common.ctrl_config.scl.i2c = ctrl_cfg->scl.i2c;
		data->common.ctrl_config.scl_od_min = ctrl_cfg->scl_od_min;

		k_mutex_lock(&data->bus_lock, K_FOREVER);
		pm_device_busy_set(dev);

		cdns_i3c_set_prescalers(dev);

		pm_device_busy_clear(dev);
		k_mutex_unlock(&data->bus_lock);
#else
		return -ENOTSUP;
#endif
	} else if (type == I3C_CONFIG_TARGET) {
#ifdef CONFIG_I3C_TARGET
		return 0;
#else
		return -ENOTSUP;
#endif
	} else {
		return -EINVAL;
	}

	return 0;
}

#if CONFIG_I3C_CONTROLLER

static int cdns_i3c_master_get_rr_slot(const struct device *dev, uint8_t dyn_addr)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	uint8_t rr_idx, i;
	uint32_t rr, activedevs;

	/* If it does not have a dynamic address, then assign it a free one */
	if (dyn_addr == 0) {
		if (!data->free_rr_slots) {
			return -ENOSPC;
		}

		return find_lsb_set(data->free_rr_slots) - 1;
	}

	/* Device already has a Dynamic Address, so assume it is already in the RRs */
	activedevs = sys_read32(config->base + DEVS_CTRL) & DEVS_CTRL_DEVS_ACTIVE_MASK;
	/* skip itself */
	activedevs &= ~BIT(0);

	/* loop through each set bit for new devices */
	for (i = find_lsb_set(activedevs); i <= find_msb_set(activedevs); i++) {
		rr_idx = i - 1;
		if (activedevs & BIT(rr_idx)) {
			rr = sys_read32(config->base + DEV_ID_RR0(rr_idx));
			if ((rr & DEV_ID_RR0_IS_I3C) && (DEV_ID_RR0_GET_DEV_ADDR(rr) == dyn_addr)) {
				return rr_idx;
			}
		}
	}

	return -EINVAL;
}

static int cdns_i3c_attach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	/*
	 * Mark Devices as active, devices that will be found and marked active during DAA,
	 * it will be given the exact DA programmed in it's RR, otherwise they get set as active
	 * here. If dynamic address is set, then it assumed that it was already initialized by the
	 * primary controller. When assigned through ENTDAA, the dynamic address, bcr, dcr, and pid
	 * are all set in the RR along with setting the device as active. If it has a static addr,
	 * then it is assumed that it will be programmed with SETDASA and will need to be marked
	 * as active before sending out SETDASA.
	 */
	if ((desc->static_addr != 0) || (desc->dynamic_addr != 0)) {
		const struct cdns_i3c_config *config = dev->config;
		struct cdns_i3c_data *data = dev->data;
		int slot;

		k_mutex_lock(&data->bus_lock, K_FOREVER);
		pm_device_busy_set(dev);

		slot = cdns_i3c_master_get_rr_slot(dev, desc->dynamic_addr);

		if (slot < 0) {
			LOG_ERR("%s: no space for i3c device: %s", dev->name, desc->dev->name);
			pm_device_busy_clear(dev);
			k_mutex_unlock(&data->bus_lock);
			return slot;
		}

		sys_write32(sys_read32(config->base + DEVS_CTRL) | DEVS_CTRL_DEV_ACTIVE(slot),
			    config->base + DEVS_CTRL);

		data->cdns_i3c_i2c_priv_data[slot].id = slot;
		desc->controller_priv = &(data->cdns_i3c_i2c_priv_data[slot]);
		data->free_rr_slots &= ~BIT(slot);

		uint32_t dev_id_rr0 =
			DEV_ID_RR0_IS_I3C |
			prepare_rr0_dev_address(desc->dynamic_addr ? desc->dynamic_addr
								   : desc->static_addr);
		uint32_t dev_id_rr1 = DEV_ID_RR1_PID_MSB((desc->pid & 0xFFFFFFFF0000) >> 16);
		uint32_t dev_id_rr2 = DEV_ID_RR2_PID_LSB(desc->pid & 0xFFFF);

		sys_write32(dev_id_rr0, config->base + DEV_ID_RR0(slot));
		sys_write32(dev_id_rr1, config->base + DEV_ID_RR1(slot));
		sys_write32(dev_id_rr2, config->base + DEV_ID_RR2(slot));

		pm_device_busy_clear(dev);
		k_mutex_unlock(&data->bus_lock);
	}

	return 0;
}

static int cdns_i3c_reattach_device(const struct device *dev, struct i3c_device_desc *desc,
				    uint8_t old_dyn_addr)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_i2c_dev_data *cdns_i3c_device_data = desc->controller_priv;

	if (cdns_i3c_device_data == NULL) {
		LOG_ERR("%s: %s: device not attached", dev->name, desc->dev->name);
		return -EINVAL;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	pm_device_busy_set(dev);

	uint32_t dev_id_rr0 = DEV_ID_RR0_IS_I3C | prepare_rr0_dev_address(desc->dynamic_addr);
	uint32_t dev_id_rr1 = DEV_ID_RR1_PID_MSB((desc->pid & 0xFFFFFFFF0000) >> 16);
	uint32_t dev_id_rr2 = DEV_ID_RR2_PID_LSB(desc->pid & 0xFFFF) | DEV_ID_RR2_BCR(desc->bcr) |
			      DEV_ID_RR2_DCR(desc->dcr);

	sys_write32(dev_id_rr0, config->base + DEV_ID_RR0(cdns_i3c_device_data->id));
	sys_write32(dev_id_rr1, config->base + DEV_ID_RR1(cdns_i3c_device_data->id));
	sys_write32(dev_id_rr2, config->base + DEV_ID_RR2(cdns_i3c_device_data->id));

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->bus_lock);

	return 0;
}

static int cdns_i3c_detach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_i2c_dev_data *cdns_i3c_device_data = desc->controller_priv;

	if (cdns_i3c_device_data == NULL) {
		/* device was probably attached, but never went through ENTDAA */
		return 0;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	pm_device_busy_set(dev);

	sys_write32(sys_read32(config->base + DEVS_CTRL) |
			    DEVS_CTRL_DEV_CLR(cdns_i3c_device_data->id),
		    config->base + DEVS_CTRL);
	data->free_rr_slots |= BIT(cdns_i3c_device_data->id);
	desc->controller_priv = NULL;

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->bus_lock);

	return 0;
}

static int cdns_i3c_i2c_attach_device(const struct device *dev, struct i3c_i2c_device_desc *desc)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;

	int slot = cdns_i3c_master_get_rr_slot(dev, 0);

	if (slot < 0) {
		LOG_ERR("%s: no space for i2c device: addr 0x%02x", dev->name, desc->addr);
		return slot;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	pm_device_busy_set(dev);

	uint32_t dev_id_rr0 = prepare_rr0_dev_address(desc->addr);
	uint32_t dev_id_rr2 = DEV_ID_RR2_LVR(desc->lvr);

	sys_write32(dev_id_rr0, config->base + DEV_ID_RR0(slot));
	sys_write32(0, config->base + DEV_ID_RR1(slot));
	sys_write32(dev_id_rr2, config->base + DEV_ID_RR2(slot));

	data->cdns_i3c_i2c_priv_data[slot].id = slot;
	desc->controller_priv = &(data->cdns_i3c_i2c_priv_data[slot]);
	data->free_rr_slots &= ~BIT(slot);

	sys_write32(sys_read32(config->base + DEVS_CTRL) | DEVS_CTRL_DEV_ACTIVE(slot),
		    config->base + DEVS_CTRL);

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->bus_lock);

	return 0;
}

static int cdns_i3c_i2c_detach_device(const struct device *dev, struct i3c_i2c_device_desc *desc)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_i2c_dev_data *cdns_i2c_device_data = desc->controller_priv;

	if (cdns_i2c_device_data == NULL) {
		LOG_ERR("%s: device not attached", dev->name);
		return -EINVAL;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	pm_device_busy_set(dev);

	sys_write32(sys_read32(config->base + DEVS_CTRL) |
			    DEVS_CTRL_DEV_CLR(cdns_i2c_device_data->id),
		    config->base + DEVS_CTRL);
	data->free_rr_slots |= BIT(cdns_i2c_device_data->id);
	desc->controller_priv = NULL;

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->bus_lock);

	return 0;
}


/**
 * @brief Transfer messages in I3C mode.
 *
 * @see i3c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I3C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @return @see i3c_transfer
 */
static int cdns_i3c_transfer(const struct device *dev, struct i3c_device_desc *target,
			     struct i3c_msg *msgs, uint8_t num_msgs)
{
#ifdef CONFIG_I3C_CADENCE_RTIO
	struct cdns_i3c_data *data = dev->data;

	return i3c_rtio_transfer(data->rtio->i3c_ctx, msgs, num_msgs, target);
#else
	return cdns_i3c_transfer_do(dev, target, msgs, num_msgs, false, NULL, NULL);
#endif
}

#ifdef CONFIG_I3C_CALLBACK
/**
 * @brief Transfer messages in I3C mode.
 *
 * @see i3c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I3C messages.
 * @param num_msgs Number of messages to transfers.
 * @param cb Callback function
 * @param userdata User data for callback function
 *
 * @return @see i3c_transfer
 */
static int cdns_i3c_transfer_cb(const struct device *dev, struct i3c_device_desc *target,
				struct i3c_msg *msgs, uint8_t num_msgs, i3c_callback_t cb,
				void *userdata)
{
#ifdef CONFIG_I3C_CADENCE_RTIO
	struct cdns_i3c_data *data = dev->data;

	return i3c_rtio_transfer_cb(data->rtio->i3c_ctx, dev, msgs, num_msgs, target, cb,
				    userdata);
#else
	return cdns_i3c_transfer_do(dev, target, msgs, num_msgs, true, cb, userdata);
#endif
}
#endif
#endif /* CONFIG_I3C_CONTROLLER */

#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_CONTROLLER
static int cdns_i3c_read_ibi_fifo(const struct cdns_i3c_config *config, void *buf, uint32_t len)
{
	uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		if (cdns_i3c_ibi_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + IBI_DATA_FIFO));
		*ptr++ = val;
	}

	if (remain > 0) {
		if (cdns_i3c_ibi_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + IBI_DATA_FIFO));
		memcpy(ptr, &val, remain);
	}

	return 0;
}

static void cdns_i3c_handle_ibi(const struct device *dev, uint32_t ibir)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;

	/* The slave ID returned here is the device ID in the SIR map NOT the device ID
	 * in the RR map.
	 */
	uint8_t slave_id = IBIR_SLVID(ibir);

	if (slave_id == IBIR_SLVID_INV) {
		/* DA does not match any value among SIR map */
		return;
	}

	uint32_t dev_id_rr0 = sys_read32(config->base + DEV_ID_RR0(slave_id + 1));
	uint8_t dyn_addr = DEV_ID_RR0_GET_DEV_ADDR(dev_id_rr0);
	struct i3c_device_desc *desc = i3c_dev_list_i3c_addr_find(dev, dyn_addr);

	/*
	 * Check for NAK or error conditions.
	 *
	 * Note: The logging is for debugging only so will be compiled out in most cases.
	 * However, if the log level for this module is DEBUG and log mode is IMMEDIATE or MINIMAL,
	 * this option is also set this may cause problems due to being inside an ISR.
	 */
	if (!(IBIR_ACKED & ibir)) {
		LOG_DBG("%s: NAK for slave ID %u", dev->name, (unsigned int)slave_id);
		return;
	}
	if (ibir & IBIR_ERROR) {
		/* Controller issued an Abort */
		LOG_ERR("%s: IBI Data overflow", dev->name);
	}

	/* Read out any payload bytes */
	uint8_t ibi_len = IBIR_XFER_BYTES(ibir);

	if (ibi_len > 0) {
		if (ibi_len - data->ibi_buf.ibi_data_cnt > 0) {
			if (cdns_i3c_read_ibi_fifo(
				    config, &data->ibi_buf.ibi_data[data->ibi_buf.ibi_data_cnt],
				    ibi_len - data->ibi_buf.ibi_data_cnt) < 0) {
				LOG_ERR("%s: Failed to get payload", dev->name);
			}
		}
		data->ibi_buf.ibi_data_cnt = 0;
	}

	if (i3c_ibi_work_enqueue_target_irq(desc, data->ibi_buf.ibi_data, ibi_len) != 0) {
		LOG_ERR("%s: Error enqueue IBI IRQ work", dev->name);
	}
}
#ifdef CONFIG_I3C_TARGET
static void cdns_i3c_handle_cr(const struct device *dev, uint32_t ibir)
{
	const struct cdns_i3c_config *config = dev->config;

	/* The slave ID returned here is the device ID in the SIR map NOT the device ID
	 * in the RR map.
	 */
	uint8_t slave_id = IBIR_SLVID(ibir);

	if (slave_id == IBIR_SLVID_INV) {
		/* DA does not match any value among SIR map */
		return;
	}

	uint32_t dev_id_rr0 = sys_read32(config->base + DEV_ID_RR0(slave_id + 1));
	uint8_t dyn_addr = DEV_ID_RR0_GET_DEV_ADDR(dev_id_rr0);
	struct i3c_device_desc *desc = i3c_dev_list_i3c_addr_find(dev, dyn_addr);

	/*
	 * Check for NAK or error conditions.
	 *
	 * Note: The logging is for debugging only so will be compiled out in most cases.
	 * However, if the log level for this module is DEBUG and log mode is IMMEDIATE or MINIMAL,
	 * this option is also set this may cause problems due to being inside an ISR.
	 */
	if (!(IBIR_ACKED & ibir)) {
		LOG_DBG("%s: NAK for slave ID %u", dev->name, (unsigned int)slave_id);
		return;
	}
	if (ibir & IBIR_ERROR) {
		LOG_ERR("%s: Data overflow", dev->name);
		return;
	}

	if (i3c_ibi_work_enqueue_controller_request(desc) != 0) {
		LOG_ERR("%s: Error enqueue IBI IRQ work", dev->name);
	}
}

static void cdns_i3c_target_ibi_cr_complete(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;

	k_sem_give(&data->ibi_cr_complete);
}
#endif /* CONFIG_I3C_TARGET */
static void cdns_i3c_handle_hj(const struct device *dev, uint32_t ibir)
{
	if (!(IBIR_ACKED & ibir)) {
		LOG_DBG("%s: NAK for HJ", dev->name);
		return;
	}

	/* TODO: disable CTRL_HJ_DISEC and process auto-ENTDAA*/
	if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
		LOG_ERR("%s: Error enqueue IBI HJ work", dev->name);
	}
}

static void cnds_i3c_master_demux_ibis(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;

	for (uint32_t status0 = sys_read32(config->base + MST_STATUS0);
	     !(status0 & MST_STATUS0_IBIR_EMP); status0 = sys_read32(config->base + MST_STATUS0)) {
		uint32_t ibir = sys_read32(config->base + IBIR);

		switch (IBIR_TYPE(ibir)) {
		case IBIR_TYPE_IBI:
			cdns_i3c_handle_ibi(dev, ibir);
			break;
		case IBIR_TYPE_HJ:
			cdns_i3c_handle_hj(dev, ibir);
			break;
		case IBIR_TYPE_MR:
#ifdef CONFIG_I3C_TARGET
			cdns_i3c_handle_cr(dev, ibir);
#endif /* CONFIG_I3C_TARGET */
			break;
		default:
			break;
		}
	}
}
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
static void cdns_i3c_target_ibi_hj_complete(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;

	k_sem_give(&data->ibi_hj_complete);
}
#endif /* CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_USE_IBI */

#ifdef CONFIG_I3C_TARGET
static void cdns_i3c_target_sdr_tx_thr_int_handler(const struct device *dev,
						   const struct i3c_target_callbacks *target_cb)
{
	int status = 0;
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;

	if (target_cb != NULL && target_cb->read_processed_cb) {
		/* with REV_ID 1.7, as a target, the fifos are full word, otherwise only the first
		 * byte is used.
		 */
		if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
			/* while tx fifo is not full and there is still data available */
			while ((!(sys_read32(config->base + SLV_STATUS1) &
				  SLV_STATUS1_SDR_TX_FULL)) &&
			       (status == 0)) {
				/* call function pointer for read */
				uint32_t tx_data = 0;
				bool data_valid = false;

				for (int j = 0; j < 4; j++) {
					uint8_t byte;
					/* will return negative if no data left to transmit and 0
					 * if data available
					 */
					status = target_cb->read_processed_cb(data->target_config,
									      &byte);
					if (status == 0) {
						data_valid = true;
						tx_data |= (byte << (j * 8));
					}
				}
				if (data_valid) {
					cdns_i3c_write_tx_fifo(config, &tx_data, sizeof(uint32_t));
				}
			}
		} else {
			/* while tx fifo is not full and there is still data available */
			while ((!(sys_read32(config->base + SLV_STATUS1) &
				  SLV_STATUS1_SDR_TX_FULL)) &&
			       (status == 0)) {
				uint8_t byte;
				/* will return negative if no data left to transmit and 0 if
				 * data available
				 */
				status = target_cb->read_processed_cb(data->target_config, &byte);
				if (status == 0) {
					cdns_i3c_write_tx_fifo(config, &byte, sizeof(uint8_t));
				}
			}
		}
	}
}
#endif /* CONFIG_I3C_TARGET */

static void cdns_i3c_irq_handler(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_USE_IBI) || defined(CONFIG_I3C_TARGET)
	struct cdns_i3c_data *data = dev->data;
#endif
#ifdef CONFIG_I3C_CONTROLLER
	uint32_t int_st = sys_read32(config->base + MST_ISR);

	sys_write32(int_st, config->base + MST_ICR);

	/* Command queue empty */
	if (int_st & MST_INT_HALTED) {
		LOG_WRN("Core Halted, 2 read aborts");
	}

#ifdef CONFIG_I3C_CADENCE_RTIO
	if (int_st & MST_INT_TX_THR) {
		cdns_i3c_rtio_irq_tx_thr(dev);
	}
	if (int_st & MST_INT_RX_THR) {
		cdns_i3c_rtio_irq_rx_thr(dev);
	}
	if (int_st & MST_INT_CMDD_EMP) {
		cdns_i3c_rtio_irq_cmdd_emp(dev);
	}
#else
	/* Command queue empty */
	if (int_st & MST_INT_CMDD_EMP) {
		cdns_i3c_complete_transfer(dev);
	}
#endif

	/* In-band interrupt */
	if (int_st & MST_INT_IBIR_THR) {
#ifdef CONFIG_I3C_USE_IBI
		cnds_i3c_master_demux_ibis(dev);
#else
		LOG_ERR("%s: IBI received - Kconfig for using IBIs is not enabled", dev->name);
#endif
	}

	/* In-band interrupt data threshold */
	if (int_st & MST_INT_IBID_THR) {
#ifdef CONFIG_I3C_USE_IBI
		/* pop data out of the IBI FIFO */
		while (!cdns_i3c_ibi_fifo_empty(config)) {
			uint32_t *ptr =
				(uint32_t *)&data->ibi_buf.ibi_data[data->ibi_buf.ibi_data_cnt];
			*ptr = sys_le32_to_cpu(sys_read32(config->base + IBI_DATA_FIFO));
			data->ibi_buf.ibi_data_cnt += 4;
		}
#else
		LOG_ERR("%s: IBI received - Kconfig for using IBIs is not enabled", dev->name);
#endif
	}

	/* In-band interrupt response overflow */
	if (int_st & MST_INT_IBIR_OVF) {
		LOG_ERR("%s: controller ibir overflow,", dev->name);
	}

	/* In-band interrupt data */
	if (int_st & MST_INT_TX_OVF) {
		LOG_ERR("%s: controller tx buffer overflow,", dev->name);
	}

	/* In-band interrupt data */
	if (int_st & MST_INT_RX_UNF) {
		LOG_ERR("%s: controller rx buffer underflow,", dev->name);
	}
#ifdef CONFIG_I3C_TARGET
	if (int_st & MST_INT_MR_DONE) {
		LOG_DBG("%s: controller CR Handoff done,", dev->name);
		k_sem_give(&data->ch_complete);
	}
#endif /* CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	uint32_t int_sl = sys_read32(config->base + SLV_ISR);
	const struct i3c_target_callbacks *target_cb =
		data->target_config ? data->target_config->callbacks : NULL;
	/* Clear interrupts */
	sys_write32(int_sl, config->base + SLV_ICR);

	/* SLV SDR rx fifo threshold */
	if (int_sl & SLV_INT_SDR_RX_THR) {
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
		/* Buffer mode: accumulate into target_rx_buf,
		 * deliver in bulk at WR_COMP.
		 */
		while (!(sys_read32(config->base + SLV_STATUS1) &
			 SLV_STATUS1_SDR_RX_EMPTY)) {
			if (data->target_rx_len < sizeof(data->target_rx_buf)) {
				data->target_rx_buf[data->target_rx_len++] =
					(uint8_t)sys_read32(config->base + RX_FIFO);
			} else {
				(void)sys_read32(config->base + RX_FIFO);
			}
		}
#else
		while (!(sys_read32(config->base + SLV_STATUS1) &
			 SLV_STATUS1_SDR_RX_EMPTY)) {
			if (target_cb != NULL && target_cb->write_received_cb != NULL) {
				cdns_i3c_target_read_rx_fifo(dev);
			}
		}
#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */
	}

	/* SLV SDR tx fifo threshold */
	if (int_sl & SLV_INT_SDR_TX_THR) {
		cdns_i3c_target_sdr_tx_thr_int_handler(dev, target_cb);
	}

	/* SLV SDR rx complete */
	if (int_sl & SLV_INT_SDR_RD_COMP) {
		(void)sys_read32(config->base + SLV_STATUS0);
		if (target_cb != NULL && target_cb->stop_cb) {
			target_cb->stop_cb(data->target_config);
		}
	}

	/* SLV SDR tx complete */
	if (int_sl & SLV_INT_SDR_WR_COMP) {
		(void)sys_read32(config->base + SLV_STATUS0);
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
		/* Drain any tail bytes and deliver the complete buffer */
		while (!(sys_read32(config->base + SLV_STATUS1) &
			 SLV_STATUS1_SDR_RX_EMPTY)) {
			if (data->target_rx_len < sizeof(data->target_rx_buf)) {
				data->target_rx_buf[data->target_rx_len++] =
					(uint8_t)sys_read32(config->base + RX_FIFO);
			} else {
				(void)sys_read32(config->base + RX_FIFO);
			}
		}
		if (target_cb != NULL && target_cb->buf_write_received_cb != NULL) {
			target_cb->buf_write_received_cb(data->target_config,
							 data->target_rx_buf,
							 data->target_rx_len);
		}
		data->target_rx_len = 0;
#else
		/* Drain any remaining bytes from the RX FIFO */
		while (!(sys_read32(config->base + SLV_STATUS1) &
			 SLV_STATUS1_SDR_RX_EMPTY)) {
			if (target_cb != NULL && target_cb->write_received_cb != NULL) {
				cdns_i3c_target_read_rx_fifo(dev);
			} else {
				(void)sys_read32(config->base + RX_FIFO);
			}
		}
#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */
		data->fifo_bytes_read = 0;
		if (target_cb != NULL && target_cb->stop_cb) {
			target_cb->stop_cb(data->target_config);
		}
	}

	/* DA has been updated */
	if (int_sl & SLV_INT_DA_UPD) {
		LOG_INF("%s: DA updated to 0x%02lx", dev->name,
			SLV_STATUS1_DA(sys_read32(config->base + SLV_STATUS1)));
#ifdef CONFIG_I3C_USE_IBI
		cdns_i3c_target_ibi_hj_complete(dev);
#endif
	}

	/* HJ complete and DA has been assigned or HJ NACK'ed or DISEC disabled HJ */
	if (int_sl & SLV_INT_HJ_DONE) {
	}

	/* EISC or DISEC has been received */
	if (int_sl & SLV_INT_EVENT_UP) {
	}

	/* sdr transfer aborted by controller */
	if (int_sl & SLV_INT_M_RD_ABORT) {
		/* TODO: consider flushing tx buffer? */
	}

	/* SLV SDR rx fifo underflow */
	if (int_sl & SLV_INT_SDR_RX_UNF) {
		LOG_ERR("%s: slave sdr rx buffer underflow", dev->name);
	}

	/* SLV SDR tx fifo overflow */
	if (int_sl & SLV_INT_SDR_TX_OVF) {
		LOG_ERR("%s: slave sdr tx buffer overflow,", dev->name);
	}

	if (int_sl & SLV_INT_DDR_RX_THR) {
	}

	/* SLV DDR WR COMPLETE */
	if (int_sl & SLV_INT_DDR_WR_COMP) {
		/* initial value of CRC5 for HDR-DDR is 0x1F */
		uint8_t crc5 = 0x1F;

		while (!(sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_DDR_RX_EMPTY)) {
			uint32_t ddr_rx_data = sys_read32(config->base + SLV_DDR_RX_FIFO);
			uint32_t preamble = (ddr_rx_data & DDR_PREAMBLE_MASK);

			if (preamble == DDR_PREAMBLE_DATA_ABORT ||
			    preamble == DDR_PREAMBLE_DATA_ABORT_ALT) {
				uint16_t ddr_payload = DDR_DATA(ddr_rx_data);

				if (cdns_i3c_ddr_parity(ddr_payload) !=
				    (ddr_rx_data & (DDR_ODD_PARITY | DDR_EVEN_PARITY))) {
					LOG_ERR("%s: Received incorrect DDR Parity", dev->name);
				}
				/* calculate a running a crc */
				crc5 = i3c_cdns_crc5(crc5, ddr_payload);

				if (target_cb != NULL && target_cb->write_received_cb != NULL) {
					/* DDR receives 2B for each payload */
					target_cb->write_received_cb(
						data->target_config,
						(uint8_t)((ddr_payload >> 8) & 0xFF));
					target_cb->write_received_cb(data->target_config,
								     (uint8_t)(ddr_payload));
				}

			} else if ((preamble == DDR_PREAMBLE_CMD_CRC) &&
				   ((ddr_rx_data & DDR_CRC_TOKEN_MASK) == DDR_CRC_TOKEN)) {
				/* should come through here last */
				if (crc5 != DDR_CRC(ddr_rx_data)) {
					LOG_ERR("%s: Received incorrect DDR CRC5", dev->name);
				}
			} else if (preamble == DDR_PREAMBLE_CMD_CRC) {
				/* should come through here first */
				uint16_t ddr_header_payload = DDR_DATA(ddr_rx_data);

				crc5 = i3c_cdns_crc5(crc5, ddr_header_payload);
			}
		}

		if (target_cb != NULL && target_cb->stop_cb != NULL) {
			target_cb->stop_cb(data->target_config);
		}
	}

	/* SLV SDR rx complete */
	if (int_sl & SLV_INT_DDR_RD_COMP) {
		/* a read needs to be done on slv_status 0 else a NACK will happen */
		(void)sys_read32(config->base + SLV_STATUS0);
		/* call stop function pointer */
		if (target_cb != NULL && target_cb->stop_cb) {
			target_cb->stop_cb(data->target_config);
		}
	}

	/* SLV DDR TX THR */
	if (int_sl & SLV_INT_DDR_TX_THR) {
		int status = 0;

		if (target_cb != NULL && target_cb->read_processed_cb) {

			while ((!(sys_read32(config->base + SLV_STATUS1) &
				  SLV_STATUS1_DDR_TX_FULL)) &&
			       (status == 0)) {
				/* call function pointer for read */
				uint8_t byte;
				/* will return negative if no data left to transmit
				 * and 0 if data available
				 */
				status = target_cb->read_processed_cb(data->target_config, &byte);
				if (status == 0) {
					cdns_i3c_write_ddr_tx_fifo(config, &byte, sizeof(byte));
				}
			}
		}
	}
#ifdef CONFIG_I3C_CONTROLLER
	/* Controllership has been been given to us */
	if (int_sl & SLV_INT_MR_DONE) {
#ifdef CONFIG_I3C_USE_IBI
		cdns_i3c_target_ibi_cr_complete(dev);
		i3c_ibi_work_enqueue_cb(dev, i3c_sec_handoffed);
		if (target_cb != NULL && target_cb->controller_handoff_cb) {
			target_cb->controller_handoff_cb(data->target_config);
		}
#endif
	}

	/* DEFTGTS */
	if (int_sl & SLV_INT_DEFSLVS) {
		/* Execute outside of the ISR context */
		k_work_submit(&data->deftgts_work);
	}
#endif /* CONFIG_I3C_CONTROLLER */
#endif /* CONFIG_I3C_TARGET */
}

static void cdns_i3c_read_hw_cfg(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;

	uint32_t devid = sys_read32(config->base + DEV_ID);
	uint32_t revid = sys_read32(config->base + REV_ID);

	LOG_DBG("%s: Device info:\r\n"
		"  vid: 0x%03lX, pid: 0x%03lX\r\n"
		"  revision: major = %lu, minor = %lu\r\n"
		"  device ID: 0x%04X",
		dev->name, REV_ID_VID(revid), REV_ID_PID(revid), REV_ID_REV_MAJOR(revid),
		REV_ID_REV_MINOR(revid), devid);

	/*
	 * Depths are specified as number of words (32bit), convert to bytes
	 */
	uint32_t cfg0 = sys_read32(config->base + CONF_STATUS0);
	uint32_t cfg1 = sys_read32(config->base + CONF_STATUS1);

	data->hw_cfg.rev_id = revid;
	data->hw_cfg.cmdr_mem_depth = CONF_STATUS0_CMDR_DEPTH(cfg0) * 4;
	data->hw_cfg.cmd_mem_depth = CONF_STATUS1_CMD_DEPTH(cfg1) * 4;
	data->hw_cfg.rx_mem_depth = CONF_STATUS1_RX_DEPTH(cfg1) * 4;
	data->hw_cfg.tx_mem_depth = CONF_STATUS1_TX_DEPTH(cfg1) * 4;
	data->hw_cfg.ddr_rx_mem_depth = CONF_STATUS1_SLV_DDR_RX_DEPTH(cfg1) * 4;
	data->hw_cfg.ddr_tx_mem_depth = CONF_STATUS1_SLV_DDR_TX_DEPTH(cfg1) * 4;
	data->hw_cfg.ibir_mem_depth = CONF_STATUS0_IBIR_DEPTH(cfg0) * 4;
	data->hw_cfg.ibi_mem_depth = CONF_STATUS1_IBI_DEPTH(cfg1) * 4;

	LOG_DBG("%s: FIFO info:\r\n"
		"  cmd_mem_depth = %u\r\n"
		"  cmdr_mem_depth = %u\r\n"
		"  rx_mem_depth = %u\r\n"
		"  tx_mem_depth = %u\r\n"
		"  ddr_rx_mem_depth = %u\r\n"
		"  ddr_tx_mem_depth = %u\r\n"
		"  ibi_mem_depth = %u\r\n"
		"  ibir_mem_depth = %u",
		dev->name, data->hw_cfg.cmd_mem_depth, data->hw_cfg.cmdr_mem_depth,
		data->hw_cfg.rx_mem_depth, data->hw_cfg.tx_mem_depth, data->hw_cfg.ddr_rx_mem_depth,
		data->hw_cfg.ddr_tx_mem_depth, data->hw_cfg.ibi_mem_depth,
		data->hw_cfg.ibir_mem_depth);

	/* Regardless of the cmd depth size we are limited by our cmd array length. */
	data->hw_cfg.cmd_mem_depth = MIN(data->hw_cfg.cmd_mem_depth, ARRAY_SIZE(data->xfer.cmds));
}

/**
 * @brief Get configuration of the I3C hardware.
 *
 * This provides a way to get the current configuration of the I3C hardware.
 *
 * This can return cached config or probed hardware parameters, but it has to
 * be up to date with current configuration.
 *
 * @param[in] dev Pointer to controller device driver instance.
 * @param[in] type Type of configuration parameters being passed
 *                 in @p config.
 * @param[in,out] config Pointer to the configuration parameters.
 *
 * Note that if @p type is @c I3C_CONFIG_CUSTOM, @p config must contain
 * the ID of the parameter to be retrieved.
 *
 * @retval 0 If successful.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int cdns_i3c_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct cdns_i3c_data *data = dev->data;
	__ASSERT_NO_MSG(config != NULL);

	if (type == I3C_CONFIG_CONTROLLER) {
#ifdef CONFIG_I3C_CONTROLLER
		(void)memcpy(config, &data->common.ctrl_config, sizeof(data->common.ctrl_config));
#else
		return -EINVAL;
#endif
	} else if (type == I3C_CONFIG_TARGET) {
#ifdef CONFIG_I3C_TARGET
		const struct cdns_i3c_config *dev_config = dev->config;
		struct i3c_config_target *target_config = (struct i3c_config_target *)config;
		/* Read RR_0 registers for itself */
		uint32_t dev_id_rr0 = sys_read32(dev_config->base + DEV_ID_RR0(0));
		uint32_t dev_id_rr1 = sys_read32(dev_config->base + DEV_ID_RR1(0));
		uint32_t dev_id_rr2 = sys_read32(dev_config->base + DEV_ID_RR2(0));
		uint32_t slv_status1 = sys_read32(dev_config->base + SLV_STATUS1);

		/* if we are currently a target */
		target_config->enabled =
			!!!(sys_read32(dev_config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE);
		if (data->common.ctrl_config.is_secondary) {
			target_config->dynamic_addr = SLV_STATUS1_DA(slv_status1);
		} else {
			target_config->dynamic_addr = (dev_id_rr0 & 0xFE) >> 1;
		}
		target_config->static_addr = 0;
		target_config->pid = ((uint64_t)dev_id_rr1 << 16) + (dev_id_rr2 >> 16);
		target_config->pid_random = !!(slv_status1 & SLV_STATUS1_VEN_TM);
		target_config->bcr = dev_id_rr2 >> 8;
		target_config->dcr = dev_id_rr2 & 0xFF;
		/* Version 1p7 supports reading MRL/MWL */
		if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
			target_config->max_read_len =
				SLV_STATUS2_MRL(sys_read32(dev_config->base + SLV_STATUS2));
			target_config->max_write_len =
				SLV_STATUS3_MWL(sys_read32(dev_config->base + SLV_STATUS3));
		} else {
			target_config->max_read_len = 0;
			target_config->max_write_len = 0;
		}
		target_config->supported_hdr = data->common.ctrl_config.supported_hdr;
#else
		return -EINVAL;
#endif
	} else {
		return -EINVAL;
	}

	return 0;
}
#ifdef CONFIG_I3C_TARGET
static int cdns_i3c_target_tx_ddr_write(const struct device *dev, uint8_t *buf, uint16_t len)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	uint32_t i, preamble;
	uint32_t data_word;
	uint8_t crc5 = 0x1F;

	/* check if there is space available in the tx fifo */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_DDR_TX_FULL) {
		return -ENOSPC;
	}

	/* DDR sends data out in 16b, so len must be a multiple of 2 */
	if (!((len % 2) == 0)) {
		return -EINVAL;
	}

	/* Header shall be known in advanced to calculate crc5 */
	uint8_t slave_da = SLV_STATUS1_DA(sys_read32(config->base + SLV_STATUS1));
	uint16_t ddr_payload_header = HDR_CMD_RD | (slave_da << 1);

	ddr_payload_header = prepare_ddr_cmd_parity_adjustment_bit(ddr_payload_header);
	crc5 = i3c_cdns_crc5(crc5, ddr_payload_header);

	/* write as much as you can to the fifo */
	for (i = 0;
	     i < len && (!(sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_DDR_TX_FULL));
	     i += 2) {
		/* Use ALT with other than first packets */
		preamble = (i > 0) ? DDR_PREAMBLE_DATA_ABORT_ALT : DDR_PREAMBLE_DATA_ABORT;
		data_word = (preamble | prepare_ddr_word(sys_get_be16(&buf[i])));
		crc5 = i3c_cdns_crc5(crc5, sys_get_be16(&buf[i]));
		sys_write32(data_word, config->base + SLV_DDR_TX_FIFO);
	}
	/* end of data buffer, write crc packet (if we are still not full) */
	if ((i == len) && (!(sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_DDR_TX_FULL))) {
		sys_write32(DDR_PREAMBLE_CMD_CRC | DDR_CRC_TOKEN | crc5 << 9,
			    config->base + SLV_DDR_TX_FIFO);
	}

	/* setup THR interrupt */
	uint32_t thr_ctrl = sys_read32(config->base + SLV_DDR_TX_RX_THR_CTRL);

	/*
	 * Interrupt at half of the data or FIFO depth to give it enough time to be
	 * processed. The ISR will then callback to the function pointer
	 * `read_processed_cb` to collect more data to transmit
	 */
	thr_ctrl &= ~TX_THR_MASK;
	thr_ctrl |= TX_THR(MIN((data->hw_cfg.tx_mem_depth / 4) / 2, len / 2));

	sys_write32(thr_ctrl, config->base + SLV_DDR_TX_RX_THR_CTRL);
	/* return total bytes written */
	return i;
}

/**
 * @brief Writes to the Target's TX FIFO
 *
 * The Cadence I3C will then ACK read requests to it's TX FIFO from a
 * Controller
 *
 * @param dev Pointer to the device structure for an I3C controller
 *            driver configured in target mode.
 * @param buf Pointer to the buffer
 * @param len Length of the buffer
 *
 * @retval Total number of bytes written
 * @retval -EACCES Not in Target Mode
 * @retval -ENOSPC No space in Tx FIFO
 */
static int cdns_i3c_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len,
				    uint8_t hdr_mode)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	const uint32_t *buf_32 = (uint32_t *)buf;
	uint32_t i = 0;
	uint32_t val = 0;
	uint16_t remain = len;

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	pm_device_busy_set(dev);

	/* check if we are currently a target */
	if (sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE) {
		i = -EACCES;
		goto error;
	}

	/* check if there is space available in the tx fifo */
	if (sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_SDR_TX_FULL) {
		i = -ENOSPC;
		goto error;
	}

	/* rev 1p7 requires the length be written to the SLV_CTRL reg */
	if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
		sys_write32(len, config->base + SLV_CTRL);
	}
	if (hdr_mode == I3C_MSG_HDR_DDR) {
		if (ctrl_config->supported_hdr & I3C_MSG_HDR_DDR) {
			i = cdns_i3c_target_tx_ddr_write(dev, buf, len);
			/* TODO: DDR THR interrupt support not implemented yet*/
		} else {
			LOG_ERR("%s: HDR-DDR not supported", dev->name);
			i = -ENOTSUP;
		}
	} else if (hdr_mode == 0) {
		/* write as much as you can to the fifo */
		while (i < len &&
		       (!(sys_read32(config->base + SLV_STATUS1) & SLV_STATUS1_SDR_TX_FULL))) {
			/* with rev 1p7, while as a target, the fifos are using the full word,
			 * otherwise only the first byte is used
			 */
			if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
				remain = len - i;
				if (remain >= 4) {
					val = *buf_32++;
				} else if (remain > 0) {
					val = 0;
					memcpy(&val, buf_32, remain);
				}
				sys_write32(val, config->base + TX_FIFO);
				i += 4;
			} else {
				sys_write32((uint32_t)buf[i], config->base + TX_FIFO);
				i++;
			}
		}

		/* setup THR interrupt */
		uint32_t thr_ctrl = sys_read32(config->base + TX_RX_THR_CTRL);

		/*
		 * Interrupt at half of the data or FIFO depth to give it enough time to be
		 * processed. The ISR will then callback to the function pointer
		 * `read_processed_cb` to collect more data to transmit
		 */
		thr_ctrl &= ~TX_THR_MASK;
		thr_ctrl |= TX_THR(MIN((data->hw_cfg.tx_mem_depth / 4) / 2, len / 2));
		sys_write32(thr_ctrl, config->base + TX_RX_THR_CTRL);
	} else {
		LOG_ERR("%s: Unsupported HDR Mode %d", dev->name, hdr_mode);
		i = -ENOTSUP;
	}
error:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->bus_lock);

	/* return total bytes written */
	return i;
}

/**
 * @brief Instructs the I3C Target device to register itself to the I3C Controller
 *
 * This routine instructs the I3C Target device to register itself to the I3C
 * Controller via its parent controller's i3c_target_register() API.
 *
 * @param dev Pointer to target device driver instance.
 * @param cfg Config struct with functions and parameters used by the I3C driver
 * to send bus events
 *
 * @return @see i3c_device_find.
 */
static int cdns_i3c_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	struct cdns_i3c_data *data = dev->data;

	data->target_config = cfg;
	return 0;
}

/**
 * @brief Unregisters the provided config as Target device
 *
 * This routine disables I3C target mode for the 'dev' I3C bus driver using
 * the provided 'config' struct containing the functions and parameters
 * to send bus events.
 *
 * @param dev Pointer to target device driver instance.
 * @param cfg Config struct with functions and parameters used by the I3C driver
 * to send bus events
 *
 * @return @see i3c_device_find.
 */
static int cdns_i3c_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	/* no way to disable? maybe write DA to 0? */
	return 0;
}
#endif /* CONFIG_I3C_TARGET */
#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Find a registered I3C target device.
 *
 * This returns the I3C device descriptor of the I3C device
 * matching the incoming @p id.
 *
 * @param dev Pointer to controller device driver instance.
 * @param id Pointer to I3C device ID.
 *
 * @return @see i3c_device_find.
 */
static struct i3c_device_desc *cdns_i3c_device_find(const struct device *dev,
						    const struct i3c_device_id *id)
{
	const struct cdns_i3c_config *config = dev->config;

	return i3c_dev_list_find(&config->common.dev_list, id);
}

#endif
#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Transfer messages in I2C mode.
 *
 * @see i2c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I2C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @return @see i2c_transfer
 */
static int cdns_i3c_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs,
				     uint8_t num_msgs, uint16_t addr)
{
#ifdef CONFIG_I3C_CADENCE_RTIO
	struct cdns_i3c_data *data = dev->data;

	return i2c_rtio_transfer(data->rtio->i2c_ctx, msgs, num_msgs, addr);
#else
	struct i3c_i2c_device_desc *i2c_dev = cdns_i3c_i2c_device_find(dev, addr);
	int ret;

	if (i2c_dev == NULL) {
		ret = -ENODEV;
	} else {
		ret = cdns_i3c_i2c_transfer(dev, i2c_dev, msgs, num_msgs);
	}

	return ret;
#endif
}

#ifdef CONFIG_I2C_CALLBACK
static int cdns_i3c_i2c_api_transfer_cb(const struct device *dev, struct i2c_msg *msgs,
					uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
					void *userdata)
{
#ifdef CONFIG_I3C_CADENCE_RTIO
	struct cdns_i3c_data *data = dev->data;

	return i2c_rtio_transfer_cb(data->rtio->i2c_ctx, dev, msgs, num_msgs, addr, cb, userdata);
#else
	struct i3c_i2c_device_desc *i2c_dev = cdns_i3c_i2c_device_find(dev, addr);

	if (i2c_dev == NULL) {
		return -ENODEV;
	}

	return cdns_i3c_i2c_transfer_cb(dev, i2c_dev, msgs, num_msgs, cb, userdata);
#endif
}
#endif /* CONFIG_I2C_CALLBACK */
#endif /* CONFIG_I3C_CONTROLLER */
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
/**
 * ACK or NACK Controller Handoffs
 *
 * Reads the LVR of all I2C devices and returns the I3C bus
 * Mode
 *
 * @param dev Pointer to device driver instance.
 * @param accept True to accept controller handoffs, False to decline
 *
 * @return @see i3c_target_controller_handoff
 */
static int cdns_i3c_target_controller_handoff(const struct device *dev, bool accept)
{
	const struct cdns_i3c_config *config = dev->config;
	uint32_t ctrl = sys_read32(config->base + CTRL);

	if (accept) {
		sys_write32(ctrl | CTRL_MST_ACK, config->base + CTRL);
	} else {
		sys_write32(ctrl & ~CTRL_MST_ACK, config->base + CTRL);
	}

	return 0;
}
#endif
#ifdef CONFIG_I3C_CONTROLLER
/**
 * Determine THD_DEL value for CTRL register
 *
 * Should be MIN(t_cf, t_cr) + 3ns
 *
 * @param dev Pointer to device driver instance.
 *
 * @return Value to be written to THD_DEL
 */
static uint8_t cdns_i3c_sda_data_hold(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	uint32_t input_clock_frequency = config->input_frequency;
	uint8_t thd_delay =
		DIV_ROUND_UP(I3C_HD_PP_DEFAULT_NS, (NSEC_PER_SEC / input_clock_frequency));

	if (thd_delay > THD_DELAY_MAX) {
		thd_delay = THD_DELAY_MAX;
	}

	return (THD_DELAY_MAX - thd_delay);
}
#endif /* CONFIG_I3C_CONTROLLER */
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
static void i3c_cdns_deftgts_work_fn(struct k_work *work)
{
	const struct cdns_i3c_config *config;
	struct cdns_i3c_data *data;
	const struct device *dev;
	uint32_t devs;
	uint8_t count;
	uint8_t n = 0;

	data = CONTAINER_OF(work, struct cdns_i3c_data, deftgts_work);
	dev = data->dev;
	config = dev->config;
	data = dev->data;

	devs = sys_read32(config->base + DEVS_CTRL) & DEVS_CTRL_DEVS_ACTIVE_MASK;
	data->free_rr_slots = GENMASK(data->max_devs, 1) & ~devs;

	/*
	 * count the number of ones in devs, The IP will 'skip' writing it self to the RR if
	 * it was in DEFTGTS. Also, if the IP never had a DA, then the deftgts interrupt will
	 * never fire. Subtract 1 as the active controller is not included in `count`.
	 */
	count = POPCOUNT(devs) - 1;

	/* Free memory if it was previously allocated */
	if (data->common.deftgts) {
		free(data->common.deftgts);
		data->common.deftgts = NULL;
	}

	/* Allocate memory for deftgts */
	data->common.deftgts =
		malloc(sizeof(uint8_t) + sizeof(struct i3c_ccc_deftgts_active_controller) +
		       (count * sizeof(struct i3c_ccc_deftgts_target)));
	if (!data->common.deftgts) {
		LOG_ERR("%s: Failed to allocate memory for DEFTGTS", dev->name);
		return;
	}

	data->common.deftgts->count = count;

	for (uint8_t i = find_lsb_set(devs); i <= find_msb_set(devs); i++) {
		uint8_t rr_idx = i - 1;

		if (devs & BIT(rr_idx)) {
			/* Read RRx registers */
			uint32_t dev_id_rr0 = sys_read32(config->base + DEV_ID_RR0(rr_idx));
			uint32_t dev_id_rr2 = sys_read32(config->base + DEV_ID_RR2(rr_idx));

			uint8_t addr = (dev_id_rr0 & 0xFE) >> 1;
			uint8_t bcr = dev_id_rr2 >> 8;
			uint8_t dcr_lvr = dev_id_rr2 & 0xFF;
			bool is_i3c = !!(dev_id_rr0 & DEV_ID_RR0_IS_I3C);

			/* RR IDX 1 should always be expected to be the AC */
			if (rr_idx == 1) {
				data->common.deftgts->active_controller.addr = addr;
				data->common.deftgts->active_controller.dcr = dcr_lvr;
				data->common.deftgts->active_controller.bcr = bcr;
				data->common.deftgts->active_controller.static_addr = 0;
			} else if (is_i3c) {
				data->common.deftgts->targets[n].addr = addr;
				data->common.deftgts->targets[n].dcr = dcr_lvr;
				data->common.deftgts->targets[n].bcr = bcr;
				data->common.deftgts->targets[n].static_addr = 0;
				n++;
			} else {
				data->common.deftgts->targets[n].addr = 0;
				data->common.deftgts->targets[n].lvr = dcr_lvr;
				data->common.deftgts->targets[n].bcr = 0;
				data->common.deftgts->targets[n].static_addr = addr;
				n++;
			}
		}
	}
	data->common.deftgts_refreshed = true;
	LOG_HEXDUMP_DBG(
		(uint8_t *)data->common.deftgts,
		sizeof(uint8_t) + sizeof(struct i3c_ccc_deftgts_active_controller) +
			(data->common.deftgts->count * sizeof(struct i3c_ccc_deftgts_target)),
		"DEFTGTS Received");
}
#endif
/**
 * @brief Initialize the hardware.
 *
 * @param dev Pointer to controller device driver instance.
 */
static int cdns_i3c_bus_init(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;

	cdns_i3c_read_hw_cfg(dev);

	/* Clear all retaining regs */
	sys_write32(DEVS_CTRL_DEV_CLR_ALL, config->base + DEVS_CTRL);

	uint32_t conf0 = sys_read32(config->base + CONF_STATUS0);
	uint32_t conf1 = sys_read32(config->base + CONF_STATUS1);
	data->max_devs = CONF_STATUS0_DEVS_NUM(conf0);
	data->free_rr_slots = GENMASK(data->max_devs, 1);

	/* DDR supported bit moved in 1p7 revision along with dev role added */
	if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
		ctrl_config->supported_hdr =
			(conf1 & CONF_STATUS1_SUPPORTS_DDR) ? I3C_MSG_HDR_DDR : 0;
		ctrl_config->is_secondary =
			(CONF_STATUS0_DEV_ROLE(conf0) == CONF_STATUS0_DEV_ROLE_SEC_MASTER) ? true
											   : false;
	} else {
		ctrl_config->supported_hdr =
			(conf0 & CONF_STATUS0_SUPPORTS_DDR) ? I3C_MSG_HDR_DDR : 0;
		ctrl_config->is_secondary = (conf0 & CONF_STATUS0_SEC_MASTER) ? true : false;
	}
	/*
	 * Ensure that is_secondary is only set when CONFIG_I3C_TARGET is enabled,
	 * or ensure that it is false when CONFIG_I3C_CONTROLLER is enabled.
	 */
	__ASSERT_NO_MSG((IS_ENABLED(CONFIG_I3C_TARGET) && ctrl_config->is_secondary) ||
			(IS_ENABLED(CONFIG_I3C_CONTROLLER) && !ctrl_config->is_secondary));

	k_mutex_init(&data->bus_lock);
	k_sem_init(&data->xfer.complete, 0, 1);
#if (defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)) || defined(CONFIG_I3C_CALLBACK)
	data->dev = dev;
#endif
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
	k_sem_init(&data->ch_complete, 0, 1);
	k_work_init(&data->deftgts_work, i3c_cdns_deftgts_work_fn);
#endif /* defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET) */
#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_TARGET
	k_sem_init(&data->ibi_hj_complete, 0, 1);
#ifdef CONFIG_I3C_CONTROLLER
	k_sem_init(&data->ibi_cr_complete, 0, 1);
#endif /* CONFIG_I3C_CONTROLLER */
#endif /* CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_USE_IBI */
#if defined(CONFIG_I3C_CONTROLLER) && (defined(CONFIG_I3C_CALLBACK) || defined(CONFIG_I2C_CALLBACK))
	k_sem_init(&data->async_sem, 1, 1);
#ifndef CONFIG_I3C_CADENCE_RTIO
	/*
	 * The legacy hw-driving path uses k_timer to bound the async transfer.
	 * The native RTIO path doesn't have an equivalent watchdog (yet) and
	 * doesn't reference cdns_i3c_cb_timeout — which lives in i3c_cdns_legacy.c
	 * and isn't compiled when CADENCE_RTIO=y.
	 */
	k_timer_init(&data->timeout, cdns_i3c_cb_timeout, NULL);
#endif
#endif /* CONFIG_I3C_CONTROLLER && (CONFIG_I3C_CALLBACK || CONFIG_I2C_CALLBACK) */

	cdns_i3c_interrupts_disable(config);
	cdns_i3c_interrupts_clear(config);

	config->irq_config_func(dev);

	/* Ensure the bus is disabled. */
	sys_write32(~CTRL_DEV_EN & sys_read32(config->base + CTRL), config->base + CTRL);
#if defined(CONFIG_I3C_CONTROLLER)
	/* determine prescaler timings for i3c and i2c scl */
	cdns_i3c_set_prescalers(dev);

	enum i3c_bus_mode mode = i3c_bus_mode(&config->common.dev_list);

	LOG_DBG("%s: i3c bus mode %d", dev->name, mode);
	int cdns_mode;

	switch (mode) {
	case I3C_BUS_MODE_PURE:
		cdns_mode = CTRL_PURE_BUS_MODE;
		break;
	case I3C_BUS_MODE_MIXED_FAST:
		cdns_mode = CTRL_MIXED_FAST_BUS_MODE;
		break;
	case I3C_BUS_MODE_MIXED_LIMITED:
	case I3C_BUS_MODE_MIXED_SLOW:
		cdns_mode = CTRL_MIXED_SLOW_BUS_MODE;
		break;
	default:
		return -EINVAL;
	}
#endif
	/*
	 * When a Hot-Join request happens, disable all events coming from this device.
	 * We will issue ENTDAA afterwards from the threaded IRQ handler.
	 * Set HJ ACK later after bus init to prevent targets from indirect DAA enforcement.
	 *
	 * Set the I3C Bus Mode based on the LVR of the I2C devices
	 */
	uint32_t ctrl = CTRL_HJ_DISEC | CTRL_MCS_EN;
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
	ctrl |= CTRL_MST_ACK;
#endif

#ifdef CONFIG_I3C_CONTROLLER
	ctrl |= (CTRL_BUS_MODE_MASK & cdns_mode);
	/*
	 * Cadence I3C release r104v1p0 and above support configuration of the sda data hold time
	 */
	if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 4)) {
		ctrl |= CTRL_THD_DELAY(cdns_i3c_sda_data_hold(dev));
	}
#endif /* CONFIG_I3C_CONTROLLER */

	/*
	 * Cadence I3C release r105v1p0 and above support I3C v1.1 timing change
	 * for tCASHr_min = tCAS_min / 2, otherwise tCASr_min = tCAS_min (as
	 * per MIPI spec v1.0)
	 */
	if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 5)) {
		ctrl |= CTRL_I3C_11_SUPP;
	}

	/* write ctrl register value */
	sys_write32(ctrl, config->base + CTRL);

	/* enable Core */
	sys_write32(CTRL_DEV_EN | ctrl, config->base + CTRL);

	/* Set fifo thresholds. */
	sys_write32(CMD_THR(I3C_CMDD_THR) | IBI_THR(config->ibid_thr) | CMDR_THR(I3C_CMDR_THR) |
			    IBIR_THR(I3C_IBIR_THR),
		    config->base + CMD_IBI_THR_CTRL);

#ifdef CONFIG_I3C_TARGET
	/* enable target interrupts */
	sys_write32(SLV_INT_DA_UPD | SLV_INT_SDR_RD_COMP | SLV_INT_SDR_WR_COMP |
			    SLV_INT_SDR_RX_THR | SLV_INT_SDR_TX_THR | SLV_INT_SDR_RX_UNF |
			    SLV_INT_SDR_TX_OVF | SLV_INT_HJ_DONE | SLV_INT_MR_DONE |
			    SLV_INT_DEFSLVS | SLV_INT_DDR_WR_COMP | SLV_INT_DDR_RD_COMP |
			    SLV_INT_DDR_RX_THR | SLV_INT_DDR_TX_THR,
		    config->base + SLV_IER);
#endif /* CONFIG_I3C_TARGET */
#ifdef CONFIG_I3C_CONTROLLER
	/* Enable controller interrupts. */
	sys_write32(MST_INT_IBIR_THR | MST_INT_RX_UNF | MST_INT_HALTED | MST_INT_MR_DONE |
			    MST_INT_TX_OVF | MST_INT_IBIR_OVF | MST_INT_IBID_THR,
		    config->base + MST_IER);

	int ret = i3c_addr_slots_init(dev);

	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_I3C_CADENCE_RTIO
	ret = cdns_i3c_rtio_init(dev);
	if (ret != 0) {
		return ret;
	}
	/* Override master-mode TX/RX thresholds for RTIO streaming.
	 * Half-FIFO thresholds let each TX_THR/RX_THR IRQ move a useful
	 * chunk of bytes. Target-mode thresholds are left as-is.
	 */
	if (sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE) {
		uint32_t tx_thr = MAX(data->hw_cfg.tx_mem_depth / sizeof(uint32_t) / 2, 1U);
		uint32_t rx_thr = MAX(data->hw_cfg.rx_mem_depth / sizeof(uint32_t) / 2, 1U);

		sys_write32(TX_THR(tx_thr) | RX_THR(rx_thr),
			    config->base + TX_RX_THR_CTRL);
	} else {
		sys_write32(TX_THR(1) | RX_THR(1), config->base + TX_RX_THR_CTRL);
		sys_write32(SLV_DDR_TX_THR(0) | SLV_DDR_RX_THR(1),
			    config->base + SLV_DDR_TX_RX_THR_CTRL);
	}
#else
	/* Set TX/RX interrupt thresholds. */
	if (sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE) {
		sys_write32(TX_THR(I3C_TX_THR) | RX_THR(data->hw_cfg.rx_mem_depth),
			    config->base + TX_RX_THR_CTRL);
	} else {
		sys_write32(TX_THR(1) | RX_THR(1), config->base + TX_RX_THR_CTRL);
		sys_write32(SLV_DDR_TX_THR(0) | SLV_DDR_RX_THR(1),
			    config->base + SLV_DDR_TX_RX_THR_CTRL);
	}
#endif

	/* only primary controllers are responsible for initializing the bus */
	if (!ctrl_config->is_secondary) {
		/* Program retaining regs. */
		cdns_i3c_program_controller_retaining_reg(dev);
		/* Sleep to wait for bus idle. */
		k_busy_wait(201);
		/* Perform bus initialization */
		if (config->common.dev_list.num_i3c > 0 &&
		    !(config->common.flags & I3C_CONTROLLER_FLAG_DISABLE_BUS_INIT)) {
			ret = i3c_bus_init(dev, &config->common.dev_list);
		}
#ifdef CONFIG_I3C_USE_IBI
		/* Bus Initialization Complete, allow HJ ACKs if not disabled */
		if (!(config->common.flags & I3C_CONTROLLER_FLAG_DISABLE_HJ_AT_INIT)) {
			sys_write32(CTRL_HJ_ACK | sys_read32(config->base + CTRL),
				    config->base + CTRL);
		}
#endif
	}
#endif /* CONFIG_I3C_CONTROLLER */
	return 0;
}

static DEVICE_API(i3c, api) = {
#ifdef CONFIG_I3C_CONTROLLER
	.i2c_api.configure = cdns_i3c_i2c_api_configure,
	.i2c_api.transfer = cdns_i3c_i2c_api_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.i2c_api.transfer_cb = cdns_i3c_i2c_api_transfer_cb,
#endif
#ifdef CONFIG_I2C_RTIO
#ifdef CONFIG_I3C_CADENCE_RTIO
	.i2c_api.iodev_submit = cdns_i3c_i2c_iodev_submit,
#else
	.i2c_api.iodev_submit = i2c_iodev_submit_fallback,
#endif
#endif
#endif /* CONFIG_I3C_CONTROLLER */

	.configure = cdns_i3c_configure,
	.config_get = cdns_i3c_config_get,
#ifdef CONFIG_I3C_CONTROLLER
	.attach_i3c_device = cdns_i3c_attach_device,
	.reattach_i3c_device = cdns_i3c_reattach_device,
	.detach_i3c_device = cdns_i3c_detach_device,
	.attach_i2c_device = cdns_i3c_i2c_attach_device,
	.detach_i2c_device = cdns_i3c_i2c_detach_device,

	.do_daa = cdns_i3c_do_daa,
	.do_ccc = cdns_i3c_do_ccc,
#ifdef CONFIG_I3C_CALLBACK
	.do_ccc_cb = cdns_i3c_do_ccc_cb,
#endif

	.i3c_device_find = cdns_i3c_device_find,

	.i3c_xfers = cdns_i3c_transfer,
#ifdef CONFIG_I3C_CALLBACK
	.i3c_xfers_cb = cdns_i3c_transfer_cb,
#endif
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	.target_tx_write = cdns_i3c_target_tx_write,
	.target_register = cdns_i3c_target_register,
	.target_unregister = cdns_i3c_target_unregister,
#ifdef CONFIG_I3C_CONTROLLER
	.target_controller_handoff = cdns_i3c_target_controller_handoff,
#endif /* CONFIG_I3C_CONTROLLER */
#endif /* CONFIG_I3C_TARGET */
#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_CONTROLLER
	.ibi_hj_response = cdns_i3c_ibi_hj_response,
	.ibi_enable = cdns_i3c_controller_ibi_enable,
	.ibi_disable = cdns_i3c_controller_ibi_disable,
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	.ibi_raise = cdns_i3c_target_ibi_raise,
#endif /* CONFIG_I3C_TARGET */
#endif

#ifdef CONFIG_I3C_RTIO
#ifdef CONFIG_I3C_CADENCE_RTIO
	.iodev_submit = cdns_i3c_iodev_submit,
#else
	.iodev_submit = i3c_iodev_submit_fallback,
#endif
#endif
};

#ifdef CONFIG_I3C_CADENCE_RTIO
#define CADENCE_I3C_RTIO_DEFINE(n)                                                                 \
	I3C_RTIO_DEFINE(_cdns_i3c##n##_rtio_ctx, CONFIG_I3C_RTIO_SQ_SIZE,                          \
			CONFIG_I3C_RTIO_CQ_SIZE);                                                  \
	I2C_RTIO_DEFINE(_cdns_i3c##n##_i2c_rtio_ctx, CONFIG_I2C_RTIO_SQ_SIZE,                      \
			CONFIG_I2C_RTIO_CQ_SIZE);                                                  \
	static struct cdns_i3c_rtio_state _cdns_i3c##n##_rtio_state = {                            \
		.i3c_ctx = &_cdns_i3c##n##_rtio_ctx,                                               \
		.i2c_ctx = &_cdns_i3c##n##_i2c_rtio_ctx,                                           \
	};
#define CADENCE_I3C_RTIO_DATA_INIT(n) .rtio = &_cdns_i3c##n##_rtio_state,
#else
#define CADENCE_I3C_RTIO_DEFINE(n)
#define CADENCE_I3C_RTIO_DATA_INIT(n)
#endif

#define CADENCE_I3C_INSTANTIATE(n)                                                                 \
	static void cdns_i3c_config_func_##n(const struct device *dev);                            \
	IF_ENABLED(CONFIG_I3C_CONTROLLER,                                                          \
	(static struct i3c_device_desc cdns_i3c_device_array_##n[] = I3C_DEVICE_ARRAY_DT_INST(n);  \
	static struct i3c_i2c_device_desc cdns_i3c_i2c_device_array_##n[] =                        \
		I3C_I2C_DEVICE_ARRAY_DT_INST(n);))                                                 \
	CADENCE_I3C_RTIO_DEFINE(n)                                                                 \
	static const struct cdns_i3c_config i3c_config_##n = {                                     \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.input_frequency = DT_INST_PROP(n, input_clock_frequency),                         \
		.irq_config_func = cdns_i3c_config_func_##n,                                       \
		.ibid_thr = DT_INST_PROP(n, ibid_thr),                                             \
		IF_ENABLED(CONFIG_I3C_CONTROLLER,                                                  \
			(.common.dev_list.i3c = cdns_i3c_device_array_##n,                         \
			.common.dev_list.num_i3c = ARRAY_SIZE(cdns_i3c_device_array_##n),          \
			.common.dev_list.i2c = cdns_i3c_i2c_device_array_##n,                      \
			.common.dev_list.num_i2c = ARRAY_SIZE(cdns_i3c_i2c_device_array_##n),      \
			.common.primary_controller_da =                                            \
				DT_INST_PROP_OR(n, primary_controller_da, 0x00),                   \
			.common.flags = I3C_CONTROLLER_CONFIG_FLAGS_DT_INST(n),))                  \
	};                                                                                         \
	static struct cdns_i3c_data i3c_data_##n = {                                               \
		IF_ENABLED(CONFIG_I3C_CONTROLLER,                                                  \
			(.common.ctrl_config.scl.i3c = DT_INST_PROP(n, i3c_scl_hz),                \
			.common.ctrl_config.scl.i2c = DT_INST_PROP(n, i2c_scl_hz),                 \
			.common.ctrl_config.scl_od_min.high_ns =                                   \
				DT_INST_PROP(n, od_thigh_min_ns),                                  \
			.common.ctrl_config.scl_od_min.low_ns =                                    \
				DT_INST_PROP(n, od_tlow_min_ns),))                                 \
		CADENCE_I3C_RTIO_DATA_INIT(n)                                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, cdns_i3c_bus_init, NULL, &i3c_data_##n, &i3c_config_##n,          \
			      POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY, &api);             \
	static void cdns_i3c_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns_i3c_irq_handler,       \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};

#define DT_DRV_COMPAT cdns_i3c
DT_INST_FOREACH_STATUS_OKAY(CADENCE_I3C_INSTANTIATE)
