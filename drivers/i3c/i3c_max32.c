/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019 NXP
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2024-2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_i3c

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>

#include <i3c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_max32, CONFIG_I3C_MAX32_LOG_LEVEL);

#define I3C_MAX_STOP_RETRIES 5

#define CONT_CTRL1_IBIRESP_ACK  0U
#define CONT_CTRL1_IBIRESP_NACK 1U

struct max32_i3c_config {
	struct i3c_driver_config common;
	mxc_i3c_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
	void (*irq_config_func)(const struct device *dev);
	bool disable_open_drain_high_pp;
};

struct max32_i3c_data {
	/** Common I3C Driver Data */
	struct i3c_driver_data common;
	uint32_t od_clock;

	/** Mutex to serialize access */
	struct k_mutex lock;

	/** Condvar for waiting for bus to be in IDLE state */
	struct k_condvar condvar;

#ifdef CONFIG_I3C_USE_IBI
	struct {
		/** List of addresses used in the MIBIRULES register. */
		uint8_t addr[5];

		/** Number of valid addresses in MIBIRULES. */
		uint8_t num_addr;

		/** True if all addresses have MSB set. */
		bool msb;

		/**
		 * True if all target devices require mandatory byte
		 * for IBI.
		 */
		bool has_mandatory_byte;
	} ibi;
#endif
};

/**
 * @brief Read a register and test for bit matches with timeout.
 *
 * Please be aware that this uses @see k_busy_wait.
 *
 * @param reg Pointer to 32-bit Register.
 * @param mask Mask to the register value.
 * @param match Value to match for masked register value.
 * @param timeout_us Timeout in microsecond before bailing out.
 *
 * @retval 0 If masked register value matches before time out.
 * @retval -ETIMEDOUT Timedout without matching.
 */
static int reg32_poll_timeout(volatile uint32_t *reg, uint32_t mask, uint32_t match,
			      uint32_t timeout_us)
{
	/*
	 * These polling checks are typically satisfied
	 * quickly (some sub-microseconds) so no extra
	 * delay between checks.
	 */
	if (!WAIT_FOR((sys_read32((mm_reg_t)reg) & mask) == match, timeout_us, /*nop*/)) {
		return -ETIMEDOUT;
	}
	return 0;
}

/**
 * @brief Test if masked register value has certain value.
 *
 * @param reg Pointer to 32-bit register.
 * @param mask Mask to test.
 * @param match Value to match.
 *
 * @return True if bits in @p mask mask matches @p match, false otherwise.
 */
static inline bool reg32_test_match(volatile uint32_t *reg, uint32_t mask, uint32_t match)
{
	uint32_t val = sys_read32((mem_addr_t)reg);

	return (val & mask) == match;
}

/**
 * @brief Test if masked register value is the same as the mask.
 *
 * @param reg Pointer to 32-bit register.
 * @param mask Mask to test.
 *
 * @return True if bits in @p mask are all set, false otherwise.
 */
static inline bool reg32_test(volatile uint32_t *reg, uint32_t mask)
{
	return reg32_test_match(reg, mask, mask);
}

/**
 * @brief Disable all interrupts.
 *
 * @param regs Pointer to controller registers.
 *
 * @return Previous enabled interrupts.
 */
static inline uint32_t max32_i3c_interrupt_disable(mxc_i3c_regs_t *regs)
{
	uint32_t intmask = regs->cont_inten;

	regs->cont_intclr = intmask;

	return intmask;
}

/**
 * @brief Enable interrupts according to mask.
 *
 * @param regs Pointer to controller registers.
 * @param mask Interrupts to be enabled.
 *
 */
static inline void max32_i3c_interrupt_enable(mxc_i3c_regs_t *regs, uint32_t mask)
{
	regs->cont_inten |= mask;
}

/**
 * @brief Check if there are any errors.
 *
 * This checks if MSTATUS has ERRWARN bit set.
 *
 * @retval True if there are any errors.
 * @retval False if no errors.
 */
static bool max32_i3c_has_error(mxc_i3c_regs_t *regs)
{
	if (regs->cont_status & MXC_F_I3C_CONT_STATUS_ERRWARN) {
		return true;
	}

	return false;
}

/**
 * @brief Test if certain bits are set in MSTATUS.
 *
 * @param regs Pointer to controller registers.
 * @param mask Bits to be tested.
 *
 * @retval True if @p mask bits are set.
 * @retval False if @p mask bits are not set.
 */
static inline bool max32_i3c_status_is_set(mxc_i3c_regs_t *regs, uint32_t mask)
{
	return reg32_test(&regs->cont_status, mask);
}

/**
 * @brief Spin wait for MSTATUS bit to be set.
 *
 * This spins forever for the bits to be set.
 *
 * @param regs Pointer to controller registers.
 * @param mask Bits to be tested.
 */
static inline void max32_i3c_status_wait(mxc_i3c_regs_t *regs, uint32_t mask)
{
	while (!max32_i3c_status_is_set(regs, mask)) {
		k_busy_wait(1);
	}
}

/**
 * @brief Wait for MSTATUS bits to be set with time out.
 *
 * @param regs Pointer to controller registers.
 * @param mask Bits to be tested.
 * @param timeout_us Timeout in microsecond before bailing out.
 *
 * @retval 0 If bits are set before time out.
 * @retval -ETIMEDOUT
 */
static inline int max32_i3c_status_wait_timeout(mxc_i3c_regs_t *regs, uint32_t mask,
						uint32_t timeout_us)
{
	return reg32_poll_timeout(&regs->cont_status, mask, mask, timeout_us);
}

/**
 * @brief Clear the MSTATUS bits and wait for them to be cleared.
 *
 * This spins forever for the bits to be cleared;
 *
 * @param regs Pointer to controller registers.
 * @param mask Bits to be cleared.
 */
static inline void max32_i3c_status_clear(mxc_i3c_regs_t *regs, uint32_t mask)
{
	/* Try to clear bit until it is cleared */
	while (1) {
		regs->cont_status = mask;

		if (!max32_i3c_status_is_set(regs, mask)) {
			break;
		}

		k_busy_wait(1);
	}
}

/**
 * @brief Clear transfer and IBI related bits in MSTATUS.
 *
 * This spins forever for those bits to be cleared;
 *
 * @see MXC_F_I3C_CONT_STATUS_REQ_DONE
 * @see MXC_F_I3C_CONT_STATUS_DONE
 * @see MXC_F_I3C_CONT_STATUS_IBI_WON
 * @see MXC_F_I3C_CONT_STATUS_ERRWARN
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_status_clear_all(mxc_i3c_regs_t *regs)
{
	uint32_t mask = MXC_F_I3C_CONT_STATUS_REQ_DONE | MXC_F_I3C_CONT_STATUS_DONE |
			MXC_F_I3C_CONT_STATUS_IBI_WON | MXC_F_I3C_CONT_STATUS_ERRWARN;

	max32_i3c_status_clear(regs, mask);
}

/**
 * @brief Clear the MSTATUS bits and wait for them to be cleared with time out.
 *
 * @param regs Pointer to controller registers.
 * @param mask Bits to be cleared.
 * @param timeout_us Timeout in microsecond before bailing out.
 *
 * @retval 0 If bits are cleared before time out.
 * @retval -ETIMEDOUT
 */
static inline int max32_i3c_status_clear_timeout(mxc_i3c_regs_t *regs, uint32_t mask,
						 uint32_t timeout_us)
{
	bool result;

	regs->cont_status = mask;

	result = WAIT_FOR(!max32_i3c_status_is_set(regs, mask), timeout_us,
			  regs->cont_status = mask);
	if (!result) {
		return -ETIMEDOUT;
	}
	return 0;
}

/**
 * @brief Spin wait for MSTATUS bit to be set, and clear it afterwards.
 *
 * Note that this spins forever waiting for bits to be set, and
 * to be cleared.
 *
 * @see max32_i3c_status_wait
 * @see max32_i3c_status_clear
 *
 * @param regs Pointer to controller registers.
 * @param mask Bits to be set and to be cleared;
 */
static inline void max32_i3c_status_wait_clear(mxc_i3c_regs_t *regs, uint32_t mask)
{
	max32_i3c_status_wait(regs, mask);
	max32_i3c_status_clear(regs, mask);
}

/**
 * @brief Wait for MSTATUS bit to be set, and clear it afterwards, with time out.
 *
 * @see max32_i3c_status_wait_timeout
 * @see max32_i3c_status_clear_timeout
 *
 * @param regs Pointer to controller registers.
 * @param mask Bits to be set and to be cleared.
 * @param timeout_us Timeout in microsecond before bailing out.
 *
 * @retval 0 If masked register value matches before time out.
 * @retval -ETIMEDOUT Timedout without matching.
 */
static inline int max32_i3c_status_wait_clear_timeout(mxc_i3c_regs_t *regs, uint32_t mask,
						      uint32_t timeout_us)
{
	int ret;

	ret = max32_i3c_status_wait_timeout(regs, mask, timeout_us);
	if (ret) {
		return ret;
	}

	ret = max32_i3c_status_clear_timeout(regs, mask, timeout_us);

	return ret;
}

/**
 * @brief Clear the MERRWARN register.
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_errwarn_clear_all_nowait(mxc_i3c_regs_t *regs)
{
	MXC_I3C_Controller_ClearError(regs);
}

/**
 * @brief Tell controller to start DAA process.
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_request_daa(mxc_i3c_regs_t *regs)
{
	regs->cont_ctrl1 &= ~(MXC_F_I3C_CONT_CTRL1_REQ | MXC_F_I3C_CONT_CTRL1_IBIRESP |
			      MXC_F_I3C_CONT_CTRL1_TERM_RD);
	regs->cont_ctrl1 |= MXC_S_I3C_CONT_CTRL1_REQ_PROCESS_DAA |
			    (CONT_CTRL1_IBIRESP_NACK << MXC_F_I3C_CONT_CTRL1_IBIRESP_POS);
}

/**
 * @brief Tell controller to start auto IBI.
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_request_auto_ibi(mxc_i3c_regs_t *regs)
{
	regs->cont_ctrl1 &= ~(MXC_F_I3C_CONT_CTRL1_REQ | MXC_F_I3C_CONT_CTRL1_IBIRESP |
			      MXC_F_I3C_CONT_CTRL1_TERM_RD);
	regs->cont_ctrl1 |=
		MXC_S_I3C_CONT_CTRL1_REQ_AUTO_IBI | (0U << MXC_F_I3C_CONT_CTRL1_IBIRESP_POS);
	max32_i3c_status_wait_clear_timeout(regs, MXC_F_I3C_CONT_STATUS_IBI_WON, 1000);
}

/**
 * @brief Get the controller state.
 *
 * @param regs Pointer to controller registers.
 *
 * @retval I3C_MSTATUS_STATE_IDLE
 * @retval I3C_MSTATUS_STATE_SLVREQ
 * @retval I3C_MSTATUS_STATE_MSGSDR
 * @retval I3C_MSTATUS_STATE_NORMACT
 * @retval I3C_MSTATUS_STATE_MSGDDR
 * @retval I3C_MSTATUS_STATE_DAA
 * @retval I3C_MSTATUS_STATE_IBIACK
 * @retval I3C_MSTATUS_STATE_IBIRCV
 */
static inline uint32_t max32_i3c_state_get(mxc_i3c_regs_t *regs)
{
	return (regs->cont_status & MXC_F_I3C_CONT_STATUS_STATE) >> MXC_F_I3C_CONT_STATUS_STATE_POS;
}

/**
 * @brief Wait for MSTATUS state
 *
 * @param regs Pointer to controller registers.
 * @param state MSTATUS state to wait for.
 * @param step_delay_us Delay in microsecond between each read of register
 *                      (cannot be 0).
 * @param total_delay_us Total delay in microsecond before bailing out.
 *
 * @retval 0 If masked register value matches before time out.
 * @retval -ETIMEDOUT Exhausted all delays without matching.
 */
static inline int max32_i3c_state_wait_timeout(mxc_i3c_regs_t *regs, uint32_t state,
					       uint32_t step_delay_us, uint32_t total_delay_us)
{
	uint32_t delayed = 0;
	int ret = -ETIMEDOUT;

	while (delayed <= total_delay_us) {
		if (max32_i3c_state_get(regs) == state) {
			ret = 0;
			break;
		}

		k_busy_wait(step_delay_us);
		delayed += step_delay_us;
	}

	return ret;
}

/**
 * @brief Wait for MSTATUS to be IDLE
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_wait_idle(struct max32_i3c_data *data, mxc_i3c_regs_t *regs)
{
	while (max32_i3c_state_get(regs) != MXC_V_I3C_CONT_STATUS_STATE_IDLE) {
		k_condvar_wait(&data->condvar, &data->lock, K_FOREVER);
	}
}

/**
 * @brief Tell controller to emit START.
 *
 * @param regs Pointer to controller registers.
 * @param addr Target address.
 * @param is_i2c True if this is I2C transactions, false if I3C.
 * @param is_read True if this is a read transaction, false if write.
 * @param read_sz Number of bytes to read if @p is_read is true.
 *
 * @return 0 if successful, or negative if error.
 */
static int max32_i3c_request_emit_start(mxc_i3c_regs_t *regs, uint8_t addr, bool is_i2c,
					bool is_read, size_t read_sz)
{
	uint32_t cont_ctrl1;
	int ret = 0;

	cont_ctrl1 = (is_i2c ? 1 : 0) << MXC_F_I3C_CONT_CTRL1_TYPE_POS;
	cont_ctrl1 |= CONT_CTRL1_IBIRESP_NACK << MXC_F_I3C_CONT_CTRL1_IBIRESP_POS;

	if (is_read) {
		cont_ctrl1 |= MXC_F_I3C_CONT_CTRL1_RDWR_DIR;

		/* How many bytes to read */
		cont_ctrl1 |= read_sz << MXC_F_I3C_CONT_CTRL1_TERM_RD_POS;
	}

	cont_ctrl1 |= MXC_S_I3C_CONT_CTRL1_REQ_EMIT_START | (addr << MXC_F_I3C_CONT_CTRL1_ADDR_POS);

	regs->cont_ctrl1 = cont_ctrl1;

	/* Wait for controller to say the operation is done */
	ret = max32_i3c_status_wait_clear_timeout(regs, MXC_F_I3C_CONT_STATUS_REQ_DONE, 1000);
	if (ret == 0) {
		/* Check for NACK */
		if (MXC_I3C_Controller_GetError(regs) == E_NO_RESPONSE) {
			ret = -ENODEV;
		}
	}

	return ret;
}

/**
 * @brief Tell controller to emit STOP.
 *
 * This emits STOP and waits for controller to get out of NORMACT,
 * checking for errors.
 *
 * @param regs Pointer to controller registers.
 * @param wait_stop True if need to wait for controller to be
 *                  no longer in NORMACT.
 */
static inline int max32_i3c_do_request_emit_stop(mxc_i3c_regs_t *regs)
{
	MXC_I3C_EmitStop(regs);

	while (max32_i3c_state_get(regs) == MXC_V_I3C_CONT_STATUS_STATE_SDR_NORM) {
		if (max32_i3c_has_error(regs)) {
			if (MXC_I3C_Controller_GetError(regs) == E_TIME_OUT) {
				MXC_I3C_Controller_ClearError(regs);
				return -ETIMEDOUT;
			}
			return -EIO;
		}
		k_busy_wait(10);
	}

	return 0;
}

/**
 * @brief Tell controller to emit STOP.
 *
 * This emits STOP when controller is in NORMACT state as this is
 * the only valid state where STOP can be emitted. This also waits
 * for the controller to get out of NORMACT before returning and
 * retries if any timeout errors occur during the emit STOP.
 *
 * @param data Pointer to device driver data
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_request_emit_stop(struct max32_i3c_data *data, mxc_i3c_regs_t *regs)
{
	size_t retries;

	if (max32_i3c_has_error(regs)) {
		MXC_I3C_Controller_ClearError(regs);
	}

	if (max32_i3c_state_get(regs) != MXC_V_I3C_CONT_STATUS_STATE_SDR_NORM) {
		return;
	}

	retries = 0;
	while (1) {
		int err = max32_i3c_do_request_emit_stop(regs);

		if (err) {
			retries++;
			if ((err == -ETIMEDOUT) && (retries <= I3C_MAX_STOP_RETRIES)) {
				LOG_WRN("Timeout on emit stop, retrying");
				continue;
			}
			LOG_ERR("Error waiting on stop");
			return;
		}

		if (retries) {
			LOG_WRN("EMIT_STOP succeeded on %u retries", retries);
		}
		break;
	}

	k_condvar_broadcast(&data->condvar);
}

/**
 * @brief Tell controller to NACK the incoming IBI.
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_ibi_respond_nack(mxc_i3c_regs_t *regs)
{
	regs->cont_ctrl1 &= ~(MXC_F_I3C_CONT_CTRL1_REQ | MXC_F_I3C_CONT_CTRL1_IBIRESP);
	regs->cont_ctrl1 |= MXC_S_I3C_CONT_CTRL1_REQ_IBI_ACKNACK |
			    (CONT_CTRL1_IBIRESP_NACK << MXC_F_I3C_CONT_CTRL1_IBIRESP_POS);

	max32_i3c_status_wait_clear(regs, MXC_F_I3C_CONT_STATUS_REQ_DONE);
}

/**
 * @brief Tell controller to ACK the incoming IBI.
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_ibi_respond_ack(mxc_i3c_regs_t *regs)
{
	regs->cont_ctrl1 &= ~(MXC_F_I3C_CONT_CTRL1_REQ | MXC_F_I3C_CONT_CTRL1_IBIRESP);
	regs->cont_ctrl1 |= MXC_S_I3C_CONT_CTRL1_REQ_IBI_ACKNACK |
			    (CONT_CTRL1_IBIRESP_ACK << MXC_F_I3C_CONT_CTRL1_IBIRESP_POS);

	max32_i3c_status_wait_clear(regs, MXC_F_I3C_CONT_STATUS_REQ_DONE);
}

/**
 * @brief Tell controller to flush both TX and RX FIFOs.
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_fifo_flush(mxc_i3c_regs_t *regs)
{
	MXC_I3C_ClearRXFIFO(regs);
	MXC_I3C_ClearTXFIFO(regs);
}

/**
 * @brief Prepare the controller for transfers.
 *
 * This is simply a wrapper to clear out status bits,
 * and error bits. Also this tells the controller to
 * flush both TX and RX FIFOs.
 *
 * @param regs Pointer to controller registers.
 */
static inline void max32_i3c_xfer_reset(mxc_i3c_regs_t *regs)
{
	max32_i3c_status_clear_all(regs);
	max32_i3c_errwarn_clear_all_nowait(regs);
	max32_i3c_fifo_flush(regs);
}

/**
 * @brief Drain RX FIFO.
 *
 * @param dev Pointer to controller device driver instance.
 */
static void max32_i3c_fifo_rx_drain(const struct device *dev)
{
	const struct max32_i3c_config *config = dev->config;
	mxc_i3c_regs_t *regs = config->regs;
	uint8_t buf;

	while (max32_i3c_status_is_set(regs, MXC_F_I3C_CONT_STATUS_RX_RDY)) {
		buf = regs->cont_rxfifo8;
	}
}

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
static struct i3c_device_desc *max32_i3c_device_find(const struct device *dev,
						     const struct i3c_device_id *id)
{
	const struct max32_i3c_config *config = dev->config;

	return i3c_dev_list_find(&config->common.dev_list, id);
}

/**
 * @brief Perform bus recovery.
 *
 * @param dev Pointer to controller device driver instance.
 */
static int max32_i3c_recover_bus(const struct device *dev)
{
	const struct max32_i3c_config *config = dev->config;
	mxc_i3c_regs_t *regs = config->regs;
	int ret = 0;
	uint8_t ibi_type;

	/* Return to IDLE if in SDR message mode */
	if (max32_i3c_state_get(regs) == MXC_V_I3C_CONT_STATUS_STATE_SDR_NORM) {
		max32_i3c_request_emit_stop(dev->data, regs);
	}

	/* Exhaust all target initiated IBIs */
	while (max32_i3c_status_is_set(regs, MXC_F_I3C_CONT_STATUS_TARG_START)) {
		ibi_type = (regs->cont_status & MXC_F_I3C_CONT_STATUS_IBITYPE) >>
			   MXC_F_I3C_CONT_STATUS_IBITYPE_POS;
		max32_i3c_status_wait_clear_timeout(regs, MXC_F_I3C_CONT_STATUS_TARG_START, 1000);
		if (ibi_type == MXC_V_I3C_CONT_STATUS_IBITYPE_HOTJOIN_REQ) {
			max32_i3c_ibi_respond_nack(regs);
		} else {
			max32_i3c_request_auto_ibi(regs);

			if (max32_i3c_status_wait_clear_timeout(regs, MXC_F_I3C_CONT_STATUS_DONE,
								1000) == -ETIMEDOUT) {
				MXC_I3C_ResetTarget(regs);
				break;
			}
		}

		/* Once auto IBI is done, discard bytes in FIFO. */
		max32_i3c_fifo_rx_drain(dev);

		/*
		 * There might be other IBIs waiting.
		 * So pause a bit to let other targets initiates
		 * their IBIs.
		 */
		k_busy_wait(100);
	}

	if (reg32_poll_timeout(&regs->cont_status, MXC_F_I3C_CONT_STATUS_STATE,
			       MXC_V_I3C_CONT_STATUS_STATE_IDLE, 1000) == -ETIMEDOUT) {
		ret = -EBUSY;
	}

	return ret;
}

/**
 * @brief Perform one read transaction.
 *
 * This reads from RX FIFO until COMPLETE bit is set in MSTATUS
 * or time out.
 *
 * @param regs Pointer to controller registers.
 * @param buf Buffer to store data.
 * @param buf_sz Buffer size in bytes.
 *
 * @return Number of bytes read, or negative if error.
 */
static int max32_i3c_do_one_xfer_read(mxc_i3c_regs_t *regs, uint8_t *buf, uint8_t buf_sz, bool ibi)
{
	int ret = 0;
	uint16_t offset = 0;
	uint16_t readb = 0;

	while (offset < buf_sz) {
		readb = MXC_I3C_ReadRXFIFO(
			regs, buf + offset, buf_sz - offset,
			1000); /*
				* If controller says timed out, we abort the transaction.
				*/
		if (max32_i3c_has_error(regs) || readb == 0) {
			if (MXC_I3C_Controller_GetError(regs) == E_TIME_OUT || readb == 0) {
				ret = -ETIMEDOUT;
			}
			/* clear error  */
			MXC_I3C_Controller_ClearError(regs);

			/* for ibi, ignore timeout err if any bytes were
			 * read, since the code doesn't know how many
			 * bytes will be sent by device. for regular
			 * application read request, return err always.
			 */
			if ((ret == -ETIMEDOUT) && ibi && offset) {
				break;
			}

			if (ret == -ETIMEDOUT) {
				LOG_ERR("Timeout error");
			}

			goto one_xfer_read_out;
		} else {
			offset += readb;
		}
	}
	ret = offset;

one_xfer_read_out:
	return ret;
}

/**
 * @brief Perform one write transaction.
 *
 * This writes all data in @p buf to TX FIFO or time out
 * waiting for FIFO spaces.
 *
 * @param regs Pointer to controller registers.
 * @param buf Buffer containing data to be sent.
 * @param buf_sz Number of bytes in @p buf to send.
 * @param no_ending True if not to signal end of write message.
 *
 * @return Number of bytes written, or negative if error.
 */
static int max32_i3c_do_one_xfer_write(mxc_i3c_regs_t *regs, uint8_t *buf, uint16_t buf_sz,
				       bool no_ending)
{
	unsigned int offset = 0;
	int remaining = buf_sz;

	while (remaining > 0) {
		if (reg32_poll_timeout(&regs->cont_fifoctrl, MXC_F_I3C_CONT_FIFOCTRL_TX_FULL, 0,
				       1000)) {
			return -ETIMEDOUT;
		}

		offset += MXC_I3C_WriteTXFIFO(regs, buf + offset, ((unsigned int)buf_sz) - offset,
					      !no_ending, 100);
		remaining = buf_sz - offset;
	}

	return (int)offset;
}

/**
 * @brief Perform one transfer transaction.
 *
 * @param regs Pointer to controller registers.
 * @param data Pointer to controller device instance data.
 * @param addr Target address.
 * @param is_i2c True if this is I2C transactions, false if I3C.
 * @param buf Buffer for data to be sent or received.
 * @param buf_sz Buffer size in bytes.
 * @param is_read True if this is a read transaction, false if write.
 * @param emit_start True if START is needed before read/write.
 * @param emit_stop True if STOP is needed after read/write.
 * @param no_ending True if not to signal end of write message.
 *
 * @return Number of bytes read/written, or negative if error.
 */
static int max32_i3c_do_one_xfer(mxc_i3c_regs_t *regs, struct max32_i3c_data *data, uint8_t addr,
				 bool is_i2c, uint8_t *buf, size_t buf_sz, bool is_read,
				 bool emit_start, bool emit_stop, bool no_ending)
{
	int ret = 0;

	max32_i3c_status_clear_all(regs);
	max32_i3c_errwarn_clear_all_nowait(regs);

	/* Emit START if so desired */
	if (emit_start) {
		ret = max32_i3c_request_emit_start(regs, addr, is_i2c, is_read, buf_sz);
		if (ret != 0) {
			emit_stop = true;

			goto out_one_xfer;
		}
	}

	if ((buf == NULL) || (buf_sz == 0)) {
		goto out_one_xfer;
	}

	if (is_read) {
		ret = max32_i3c_do_one_xfer_read(regs, buf, buf_sz, false);
	} else {
		ret = max32_i3c_do_one_xfer_write(regs, buf, buf_sz, no_ending);
	}

	if (ret < 0) {
		goto out_one_xfer;
	}

	if (is_read || !no_ending) {
		/*
		 * Wait for controller to say the operation is done.
		 * Save time by not clearing the bit.
		 */
		ret = max32_i3c_status_wait_timeout(regs, MXC_F_I3C_CONT_STATUS_DONE, 10000);
		if (ret != 0) {
			LOG_ERR("%s: timed out addr 0x%02x, buf_sz %u", __func__, addr, buf_sz);
			emit_stop = true;

			goto out_one_xfer;
		}
	}

	if (max32_i3c_has_error(regs)) {
		LOG_ERR("HAS ERR\n");
		ret = -EIO;
	}

out_one_xfer:
	if (emit_stop) {
		max32_i3c_request_emit_stop(data, regs);
	}

	return ret;
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
static int max32_i3c_transfer(const struct device *dev, struct i3c_device_desc *target,
			      struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct max32_i3c_config *config = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = config->regs;
	int ret;
	bool send_broadcast = true;

	if (target->dynamic_addr == 0U) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	max32_i3c_wait_idle(data, regs);
	max32_i3c_xfer_reset(regs);

	for (int i = 0; i < num_msgs; i++) {
		bool is_read = (msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ;
		bool no_ending = false;

		/* Check start/stop conditions */
		bool emit_start =
			(i == 0) || ((msgs[i].flags & I3C_MSG_RESTART) == I3C_MSG_RESTART);
		bool emit_stop = (msgs[i].flags & I3C_MSG_STOP) == I3C_MSG_STOP;

		/* Check if last byte needs to be handled */
		if (!is_read && !emit_stop && ((i + 1) != num_msgs)) {
			bool next_is_write = (msgs[i + 1].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE;
			bool next_is_restart =
				((msgs[i + 1].flags & I3C_MSG_RESTART) == I3C_MSG_RESTART);

			if (next_is_write && !next_is_restart) {
				no_ending = true;
			}
		}

		/* Send broadcast if required */
		if (!(msgs[i].flags & I3C_MSG_NBCH) && (send_broadcast)) {
			while (1) {
				ret = max32_i3c_request_emit_start(regs, I3C_BROADCAST_ADDR, false,
								   false, 0);
				if (ret == -ENODEV) {
					LOG_WRN("emit start of broadcast addr got NACK, maybe IBI");
					if (max32_i3c_state_get(regs) ==
					    MXC_V_I3C_CONT_STATUS_STATE_TARG_REQ) {
						/* If IBI, then wait for idle */
						max32_i3c_wait_idle(data, regs);
						continue;
					}
					goto out_xfer_i3c_stop_unlock;
				}
				if (ret < 0) {
					LOG_ERR("emit start of broadcast addr failed, error (%d)",
						ret);
					goto out_xfer_i3c_stop_unlock;
				}
				break;
			}
			send_broadcast = false;
		}

		ret = max32_i3c_do_one_xfer(regs, data, target->dynamic_addr, false, msgs[i].buf,
					    msgs[i].len, is_read, emit_start, emit_stop, no_ending);
		if (ret < 0) {
			LOG_ERR("Xfer failed %d\n", ret);
			goto out_xfer_i3c_stop_unlock;
		}

		/* write back the total number of bytes transferred */
		msgs[i].num_xfer = ret;

		if (emit_stop) {
			/* After a STOP, send broadcast header before next msg */
			send_broadcast = true;
		}
	}

	ret = 0;

out_xfer_i3c_stop_unlock:
	max32_i3c_request_emit_stop(data, regs);
	max32_i3c_errwarn_clear_all_nowait(regs);
	max32_i3c_status_clear_all(regs);
	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief Perform Dynamic Address Assignment.
 *
 * @see i3c_do_daa
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @return @see i3c_do_daa
 */
static int max32_i3c_do_daa(const struct device *dev)
{
	const struct max32_i3c_config *config = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = config->regs;
	int ret = 0;
	uint8_t rx_buf[8] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
	size_t rx_count;
	uint8_t rx_size = 0;
	uint32_t intmask;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max32_i3c_state_wait_timeout(regs, MXC_V_I3C_CONT_STATUS_STATE_IDLE, 100, 100000);
	if (ret == -ETIMEDOUT) {
		goto out_daa_unlock;
	}

	LOG_DBG("DAA: ENTDAA");

	/* Disable I3C IRQ sources while we configure stuff. */
	intmask = max32_i3c_interrupt_disable(regs);

	max32_i3c_xfer_reset(regs);

	/* Emit process DAA */
	max32_i3c_request_daa(regs);

	/* Loop until no more responses from devices */
	do {
		/* Loop to grab data from devices (Provisioned ID, BCR and DCR) */
		do {
			if (max32_i3c_has_error(regs)) {
				LOG_ERR("DAA recv error");

				ret = -EIO;

				goto out_daa;
			}

			rx_count = MXC_I3C_Controller_GetRXCount(regs);
			while (max32_i3c_status_is_set(regs, MXC_F_I3C_CONT_STATUS_RX_RDY) &&
			       (rx_count != 0U)) {
				rx_buf[rx_size] = (uint8_t)(regs->cont_rxfifo8 & 0xFF);
				rx_size++;
				rx_count--;
			}
		} while (!max32_i3c_status_is_set(regs, MXC_F_I3C_CONT_STATUS_REQ_DONE));

		max32_i3c_status_clear(regs, MXC_F_I3C_CONT_STATUS_REQ_DONE);

		/* Figure out what address to assign to device */
		if ((max32_i3c_state_get(regs) == MXC_V_I3C_CONT_STATUS_STATE_DAA) &&
		    (max32_i3c_status_is_set(regs, MXC_F_I3C_CONT_STATUS_WAIT))) {
			struct i3c_device_desc *target;
			uint16_t vendor_id =
				(((uint16_t)rx_buf[0] << 8U) | (uint16_t)rx_buf[1]) & 0xFFFEU;
			uint32_t part_no = (uint32_t)rx_buf[2] << 24U | (uint32_t)rx_buf[3] << 16U |
					   (uint32_t)rx_buf[4] << 8U | (uint32_t)rx_buf[5];
			uint64_t pid = (uint64_t)vendor_id << 32U | (uint64_t)part_no;
			const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
			uint8_t dyn_addr;

			rx_size = 0;

			LOG_DBG("DAA: Rcvd PID 0x%04x%08x", vendor_id, part_no);

			dyn_addr = i3c_addr_slots_next_free_find(
				&data->common.attached_dev.addr_slots, 0);
			if (dyn_addr == 0U) {
				/* No free addresses available */
				LOG_DBG("No more free addresses available.");
				ret = -ENOSPC;
				goto out_daa;
			}

			target = i3c_device_find(dev, &i3c_id);
			if (!target) {
				/* Target not known, allocate a descriptor */
				target = i3c_device_desc_alloc();
				if (target) {
					*(const struct device **)&target->bus = dev;
					*(uint64_t *)&target->pid = pid;
					target->dynamic_addr = dyn_addr;
					target->bcr = rx_buf[6];
					target->dcr = rx_buf[7];
					/* attach it to the list */
					sys_slist_append(&data->common.attached_dev.devices.i3c,
							 &target->node);
				} else {
					/* No more free device descriptors */
					LOG_DBG("No more free device descriptors.");
					ret = -ENOMEM;
					goto out_daa;
				}

				LOG_INF("%s: PID 0x%012llx is not in registered device "
					"list, given DA 0x%02x",
					dev->name, pid, dyn_addr);
			} else {
				target->dynamic_addr = dyn_addr;
				target->bcr = rx_buf[6];
				target->dcr = rx_buf[7];
			}
			/* Mark the address as I3C device */
			i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dyn_addr);

			/*
			 * If the device has static address, after address assignment,
			 * the device will not respond to the static address anymore.
			 * So free the static one from address slots if different from
			 * newly assigned one.
			 */
			if ((target->static_addr != 0U) && (dyn_addr != target->static_addr)) {
				i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots,
							 dyn_addr);
			}

			/* Emit process DAA again to send the address to the device */
			MXC_I3C_WriteTXFIFO(regs, &dyn_addr, 1, false, 10);
			max32_i3c_request_daa(regs);

			LOG_DBG("PID 0x%04x%08x assigned dynamic address 0x%02x", vendor_id,
				part_no, dyn_addr);
		}

	} while (!max32_i3c_status_is_set(regs, MXC_F_I3C_CONT_STATUS_DONE));

out_daa:
	/* Clear all flags. */
	max32_i3c_errwarn_clear_all_nowait(regs);
	max32_i3c_status_clear_all(regs);

	/* Re-Enable I3C IRQ sources. */
	max32_i3c_interrupt_enable(regs, intmask);

out_daa_unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

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
static int max32_i3c_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct max32_i3c_config *config = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = config->regs;
	int ret = 0;

	if (payload == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	max32_i3c_xfer_reset(regs);

	LOG_DBG("CCC[0x%02x]", payload->ccc.id);

	/* Emit START */
	ret = max32_i3c_request_emit_start(regs, I3C_BROADCAST_ADDR, false, false, 0);
	if (ret < 0) {
		LOG_ERR("CCC[0x%02x] %s START error (%d)", payload->ccc.id,
			i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct", ret);

		goto out_ccc_stop;
	}

	/* Write the CCC code */
	max32_i3c_status_clear_all(regs);
	max32_i3c_errwarn_clear_all_nowait(regs);
	ret = max32_i3c_do_one_xfer_write(regs, &payload->ccc.id, 1, payload->ccc.data_len > 0);
	if (ret < 0) {
		LOG_ERR("CCC[0x%02x] %s command error (%d)", payload->ccc.id,
			i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct", ret);

		goto out_ccc_stop;
	}

	/* Write additional data for CCC if needed */
	if (payload->ccc.data_len > 0) {
		max32_i3c_status_clear_all(regs);
		max32_i3c_errwarn_clear_all_nowait(regs);
		ret = max32_i3c_do_one_xfer_write(regs, payload->ccc.data, payload->ccc.data_len,
						  false);
		if (ret < 0) {
			LOG_ERR("CCC[0x%02x] %s command payload error (%d)", payload->ccc.id,
				i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct",
				ret);

			goto out_ccc_stop;
		}

		/* write back the total number of bytes transferred */
		payload->ccc.num_xfer = ret;
	}

	/* Wait for controller to say the operation is done */
	ret = max32_i3c_status_wait_clear_timeout(regs, MXC_F_I3C_CONT_STATUS_DONE, 1000);
	if (ret != 0) {
		goto out_ccc_stop;
	}

	if (!i3c_ccc_is_payload_broadcast(payload)) {
		/*
		 * If there are payload(s) for each target,
		 * RESTART and then send payload for each target.
		 */
		for (int idx = 0; idx < payload->targets.num_targets; idx++) {
			struct i3c_ccc_target_payload *tgt_payload =
				&payload->targets.payloads[idx];

			bool is_read = tgt_payload->rnw == 1U;
			bool emit_start = idx == 0;

			ret = max32_i3c_do_one_xfer(regs, data, tgt_payload->addr, false,
						    tgt_payload->data, tgt_payload->data_len,
						    is_read, emit_start, false, false);
			if (ret < 0) {
				LOG_ERR("CCC[0x%02x] target payload error (%d)", payload->ccc.id,
					ret);

				goto out_ccc_stop;
			}

			/* write back the total number of bytes transferred */
			tgt_payload->num_xfer = ret;
		}
	}

out_ccc_stop:
	max32_i3c_request_emit_stop(data, regs);

	if (ret > 0) {
		ret = 0;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

#ifdef CONFIG_I3C_USE_IBI
/**
 * @brief Callback to service target initiated IBIs.
 *
 * @param work Pointer to k_work item.
 */
static void max32_i3c_ibi_work(struct k_work *work)
{
	uint8_t payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	size_t payload_sz = 0;

	struct i3c_ibi_work *i3c_ibi_work = CONTAINER_OF(work, struct i3c_ibi_work, work);
	const struct device *dev = i3c_ibi_work->controller;
	const struct max32_i3c_config *config = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = config->regs;
	struct i3c_device_desc *target = NULL;
	uint32_t cont_status, ibitype, ibiaddr;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (max32_i3c_state_get(regs) != MXC_V_I3C_CONT_STATUS_STATE_TARG_REQ) {
		LOG_DBG("IBI work %p running not because of IBI", work);
		LOG_DBG("CONT_STATUS 0x%08x CONT_ERRWARN 0x%08x", regs->cont_status,
			regs->cont_errwarn);
		max32_i3c_request_emit_stop(data, regs);

		goto out_ibi_work;
	};

	/* Use auto IBI to service the IBI */
	max32_i3c_request_auto_ibi(regs);

	cont_status = sys_read32((mem_addr_t)&regs->cont_status);
	ibiaddr = (cont_status & MXC_F_I3C_CONT_STATUS_IBI_ADDR) >>
		  MXC_F_I3C_CONT_STATUS_IBI_ADDR_POS;

	ibitype =
		(cont_status & MXC_F_I3C_CONT_STATUS_IBITYPE) >> MXC_F_I3C_CONT_STATUS_IBITYPE_POS;

	/*
	 * Wait for COMPLETE bit to be set to indicate auto IBI
	 * has finished for hot-join and controller role request.
	 * For target interrupts, the IBI payload may be longer
	 * than the RX FIFO so we won't get the COMPLETE bit set
	 * at the first round of data read. So checking of
	 * COMPLETE bit is deferred to the reading.
	 */
	switch (ibitype) {
	case MXC_V_I3C_CONT_STATUS_IBITYPE_HOTJOIN_REQ:
		__fallthrough;

	case MXC_V_I3C_CONT_STATUS_IBITYPE_CONT_REQ:
		if (max32_i3c_status_wait_timeout(regs, MXC_F_I3C_CONT_STATUS_DONE, 1000) ==
		    -ETIMEDOUT) {
			LOG_ERR("Timeout waiting for COMPLETE");

			max32_i3c_request_emit_stop(data, regs);

			goto out_ibi_work;
		}
		break;

	default:
		break;
	};

	switch (ibitype) {
	case MXC_V_I3C_CONT_STATUS_IBITYPE_IBI:
		target = i3c_dev_list_i3c_addr_find(dev, (uint8_t)ibiaddr);
		if (target != NULL) {
			ret = max32_i3c_do_one_xfer_read(regs, &payload[0], sizeof(payload), true);
			if (ret >= 0) {
				payload_sz = (size_t)ret;
			} else {
				LOG_ERR("Error reading IBI payload");

				max32_i3c_request_emit_stop(data, regs);

				goto out_ibi_work;
			}
		} else {
			LOG_ERR("IBI from unknown device addr 0x%x", ibiaddr);
			/* NACK IBI coming from unknown device */
			max32_i3c_ibi_respond_nack(regs);
		}
		break;
	case MXC_V_I3C_CONT_STATUS_IBITYPE_HOTJOIN_REQ:
		max32_i3c_ibi_respond_ack(regs);
		break;
	case MXC_V_I3C_CONT_STATUS_IBITYPE_CONT_REQ:
		LOG_DBG("Controller role handoff not supported");
		max32_i3c_ibi_respond_nack(regs);
		break;
	default:
		break;
	}

	if (max32_i3c_has_error(regs)) {
		/*
		 * If the controller detects any errors, simply
		 * emit a STOP to abort the IBI. The target will
		 * raise IBI again if so desired.
		 */
		max32_i3c_request_emit_stop(data, regs);

		goto out_ibi_work;
	}

	switch (ibitype) {
	case MXC_V_I3C_CONT_STATUS_IBITYPE_IBI:
		if ((target != NULL) &&
		    (i3c_ibi_work_enqueue_target_irq(target, &payload[0], payload_sz) != 0)) {
			LOG_ERR("Error enqueue IBI IRQ work");
		}

		/* Finishing the IBI transaction */
		max32_i3c_request_emit_stop(data, regs);
		break;
	case MXC_V_I3C_CONT_STATUS_IBITYPE_HOTJOIN_REQ:
		if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
			LOG_ERR("Error enqueue IBI HJ work");
		}
		break;
	case MXC_V_I3C_CONT_STATUS_IBITYPE_CONT_REQ:
		break;
	default:
		break;
	}

out_ibi_work:
	k_mutex_unlock(&data->lock);

	/* Re-enable target initiated IBI interrupt. */
	regs->cont_inten = MXC_F_I3C_CONT_INTEN_TARG_START;
}

static void max32_i3c_ibi_rules_setup(struct max32_i3c_data *data, mxc_i3c_regs_t *regs)
{
	uint32_t ibi_rules;
	int idx;

	ibi_rules = 0;

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		uint32_t addr_6bit;

		/* Extract the lower 6-bit of target address */
		addr_6bit = (uint32_t)data->ibi.addr[idx] & MXC_F_I3C_CONT_IBIRULES_ADDR0;

		/* Shift into correct place */
		addr_6bit <<= idx * MXC_F_I3C_CONT_IBIRULES_ADDR0_POS;

		/* Put into the temporary IBI Rules register */
		ibi_rules |= addr_6bit;
	}

	if (!data->ibi.msb) {
		/* The MSB0 field is 1 if MSB is 0 */
		ibi_rules |= MXC_F_I3C_CONT_IBIRULES_MSB0;
	}

	if (!data->ibi.has_mandatory_byte) {
		/* The NOBYTE field is 1 if there is no mandatory byte */
		ibi_rules |= MXC_F_I3C_CONT_IBIRULES_NOBYTE;
	}

	/* Update the register */
	regs->cont_ibirules = ibi_rules;

	LOG_DBG("CONT_IBIRULES 0x%08x", ibi_rules);
}

int max32_i3c_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	const struct max32_i3c_config *config = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = config->regs;
	struct i3c_ccc_events i3c_events;
	uint8_t idx;
	bool msb, has_mandatory_byte;
	int ret = 0;

	if (!i3c_device_is_ibi_capable(target)) {
		return -EINVAL;
	}

	if (data->ibi.num_addr >= ARRAY_SIZE(data->ibi.addr)) {
		/* No more free entries in the IBI Rules table */
		return -ENOMEM;
	}

	/* Check for duplicate */
	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (data->ibi.addr[idx] == target->dynamic_addr) {
			return -EINVAL;
		}
	}

	/* Disable controller interrupt while we configure IBI rules. */
	MXC_I3C_Controller_DisableInt(regs, MXC_F_I3C_CONT_INTCLR_TARG_START);

	LOG_DBG("IBI enabling for 0x%02x (BCR 0x%02x)", target->dynamic_addr, target->bcr);

	msb = (target->dynamic_addr & BIT(6)) == BIT(6);
	has_mandatory_byte = i3c_ibi_has_payload(target);

	/*
	 * If there are already addresses in the table, we must
	 * check if the incoming entry is compatible with
	 * the existing ones.
	 */
	if (data->ibi.num_addr > 0) {
		/*
		 * 1. All devices in the table must all use mandatory
		 *    bytes, or do not.
		 *
		 * 2. Each address in entry only captures the lowest 6-bit.
		 *    The MSB (7th bit) is captured separated in another bit
		 *    in the register. So all addresses must have the same MSB.
		 */
		if (has_mandatory_byte != data->ibi.has_mandatory_byte) {
			LOG_ERR("New IBI does not have same mandatory byte requirement"
				" as previous IBI");
			ret = -EINVAL;
			goto out;
		}
		if (msb != data->ibi.msb) {
			LOG_ERR("New IBI does not have same msb as previous IBI");
			ret = -EINVAL;
			goto out;
		}

		/* Find an empty address slot */
		for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
			if (data->ibi.addr[idx] == 0U) {
				break;
			}
		}
		if (idx >= ARRAY_SIZE(data->ibi.addr)) {
			LOG_ERR("Cannot support more IBIs");
			ret = -ENOTSUP;
			goto out;
		}
	} else {
		/*
		 * If the incoming address is the first in the table,
		 * it dictates future compatibilities.
		 */
		data->ibi.has_mandatory_byte = has_mandatory_byte;
		data->ibi.msb = msb;

		idx = 0;
	}

	data->ibi.addr[idx] = target->dynamic_addr;
	data->ibi.num_addr += 1U;

	max32_i3c_ibi_rules_setup(data, regs);

	/* Tell target to enable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_ERR("Error sending IBI ENEC for 0x%02x (%d)", target->dynamic_addr, ret);
	}

out:
	if (data->ibi.num_addr > 0U) {
		/*
		 * Enable controller to raise interrupt when a target
		 * initiates IBI.
		 */
		MXC_I3C_Controller_EnableInt(regs, MXC_F_I3C_CONT_INTCLR_TARG_START);
	}

	return ret;
}

int max32_i3c_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	const struct max32_i3c_config *config = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = config->regs;
	struct i3c_ccc_events i3c_events;
	int ret = 0;
	int idx;

	if (!i3c_device_is_ibi_capable(target)) {
		ret = -EINVAL;
		goto out;
	}

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (target->dynamic_addr == data->ibi.addr[idx]) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(data->ibi.addr)) {
		/* Target is not in list of registered addresses. */
		ret = -ENODEV;
		goto out;
	}

	/* Disable controller interrupt while we configure IBI rules. */
	MXC_I3C_Controller_DisableInt(regs, MXC_F_I3C_CONT_INTCLR_TARG_START);

	data->ibi.addr[idx] = 0U;
	data->ibi.num_addr -= 1U;

	/* Tell target to disable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_ERR("Error sending IBI DISEC for 0x%02x (%d)", target->dynamic_addr, ret);
	}

	max32_i3c_ibi_rules_setup(data, regs);

	if (data->ibi.num_addr > 0U) {
		/*
		 * Enable controller to raise interrupt when a target
		 * initiates IBI.
		 */
		MXC_I3C_Controller_EnableInt(regs, MXC_F_I3C_CONT_INTCLR_TARG_START);
	}
out:

	return ret;
}
#endif /* CONFIG_I3C_USE_IBI */

/**
 * @brief Interrupt Service Routine
 *
 * Currently only services interrupts when any target initiates IBIs.
 *
 * @param dev Pointer to controller device driver instance.
 */
static void max32_i3c_isr(const struct device *dev)
{
#ifdef CONFIG_I3C_USE_IBI
	const struct max32_i3c_config *config = dev->config;
	mxc_i3c_regs_t *regs = config->regs;

	/* Target initiated IBIs */
	if (max32_i3c_status_is_set(regs, MXC_F_I3C_CONT_STATUS_TARG_START)) {
		int err;

		/* Clear SLVSTART interrupt */
		regs->cont_status = MXC_F_I3C_CONT_STATUS_TARG_START;

		/*
		 * Disable further target initiated IBI interrupt
		 * while we try to service the current one.
		 */
		regs->cont_intclr = MXC_F_I3C_CONT_INTCLR_TARG_START;

		/*
		 * Handle IBI in workqueue.
		 */
		err = i3c_ibi_work_enqueue_cb(dev, max32_i3c_ibi_work);
		if (err) {
			LOG_ERR("Error enqueuing ibi work, err %d", err);
			regs->cont_inten = MXC_F_I3C_CONT_INTEN_TARG_START;
		}
	}
#else
	ARG_UNUSED(dev);
#endif
}

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
 */
static int max32_i3c_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	const struct max32_i3c_config *cfg = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = cfg->regs;
	mxc_i3c_config_t hal_cfg;
	struct i3c_config_controller *ctrl_cfg = config;
	uint32_t clock_freq;
	int ret = 0;

	if (type != I3C_CONFIG_CONTROLLER) {
		return -EINVAL;
	}

	if ((ctrl_cfg->is_secondary) || (ctrl_cfg->scl.i2c == 0U) || (ctrl_cfg->scl.i3c == 0U)) {
		return -EINVAL;
	}

	if (clock_control_get_rate(cfg->clock, (clock_control_subsys_t)&cfg->perclk, &clock_freq)) {
		return -EINVAL;
	}

	memcpy(&data->common.ctrl_config, ctrl_cfg, sizeof(*ctrl_cfg));

	hal_cfg.target_mode = false;
	hal_cfg.pp_hz = ctrl_cfg->scl.i3c;
	hal_cfg.od_hz = data->od_clock;
	hal_cfg.i2c_hz = ctrl_cfg->scl.i2c;

	ret = MXC_I3C_Init(regs, &hal_cfg);
	if (ret < 0) {
		return -EIO;
	}

	MXC_I3C_SetODFrequency(regs, hal_cfg.od_hz, !cfg->disable_open_drain_high_pp);
	MXC_I3C_SetI2CFrequency(regs, hal_cfg.i2c_hz);

	return 0;
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
 * @retval -EINVAL Invalid configuration parameters.
 */
static int max32_i3c_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct max32_i3c_data *data = dev->data;

	if ((type != I3C_CONFIG_CONTROLLER) || config == NULL) {
		return -EINVAL;
	}

	memcpy(config, &data->common.ctrl_config, sizeof(data->common.ctrl_config));

	return 0;
}

/**
 * @brief Initialize the hardware.
 *
 * @param dev Pointer to controller device driver instance.
 */
static int max32_i3c_init(const struct device *dev)
{
	const struct max32_i3c_config *cfg = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = cfg->regs;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	int ret = 0;

	ret = i3c_addr_slots_init(dev);
	if (ret) {
		return ret;
	}

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	k_mutex_init(&data->lock);
	k_condvar_init(&data->condvar);

	/* Currently can only act as primary controller. */
	ctrl_config->is_secondary = false;

	/* HDR mode is not supported. */
	ctrl_config->supported_hdr = 0U;

	ret = max32_i3c_configure(dev, I3C_CONFIG_CONTROLLER, ctrl_config);
	if (ret) {
		return -EINVAL;
	}

	MXC_I3C_Controller_DisableInt(
		regs, MXC_F_I3C_CONT_INTCLR_TARG_START | MXC_F_I3C_CONT_INTCLR_REQ_DONE |
			      MXC_F_I3C_CONT_INTCLR_DONE | MXC_F_I3C_CONT_INTCLR_RX_RDY |
			      MXC_F_I3C_CONT_INTCLR_TX_NFULL | MXC_F_I3C_CONT_INTCLR_IBI_WON |
			      MXC_F_I3C_CONT_INTCLR_ERRWARN | MXC_F_I3C_CONT_INTCLR_NOW_CONT);

	ret = max32_i3c_recover_bus(dev);
	if (ret) {
		return -EIO;
	}

	cfg->irq_config_func(dev);

	ret = i3c_bus_init(dev, &cfg->common.dev_list);

	return ret;
}

static int max32_i3c_i2c_api_configure(const struct device *dev, uint32_t dev_config)
{
	return -ENOSYS;
}

static int max32_i3c_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs,
				      uint8_t num_msgs, uint16_t addr)
{
	const struct max32_i3c_config *config = dev->config;
	struct max32_i3c_data *data = dev->data;
	mxc_i3c_regs_t *regs = config->regs;
	int ret = 0;
	int readb = 0;
	uint32_t max_rd = MXC_F_I3C_CONT_CTRL1_TERM_RD >> MXC_F_I3C_CONT_CTRL1_TERM_RD_POS;
	size_t chunk_size;

	k_mutex_lock(&data->lock, K_FOREVER);

	max32_i3c_wait_idle(data, regs);

	max32_i3c_xfer_reset(regs);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		bool is_read = (msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool no_ending = false;

		/*
		 * Emit start if this is the first message or that
		 * the RESTART flag is set in message.
		 */
		bool emit_start =
			(i == 0) || ((msgs[i].flags & I2C_MSG_RESTART) == I2C_MSG_RESTART);

		bool emit_stop = (msgs[i].flags & I2C_MSG_STOP) == I2C_MSG_STOP;

		/*
		 * The controller requires special treatment of last byte of
		 * a write message. Since the API permits having a bunch of
		 * write messages without RESTART in between, this is just some
		 * logic to determine whether to treat the last byte of this
		 * message to be the last byte of a series of write messages.
		 * If not, tell the write function not to treat it that way.
		 */
		if (!is_read && !emit_stop && ((i + 1) != num_msgs)) {
			bool next_is_write = (msgs[i + 1].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE;
			bool next_is_restart =
				((msgs[i + 1].flags & I2C_MSG_RESTART) == I2C_MSG_RESTART);

			if (next_is_write && !next_is_restart) {
				no_ending = true;
			}
		}

		if (is_read) {
			bool emit_stop_between = false;

			while (readb < msgs[i].len) {
				if (msgs[i].len - readb > max_rd) {
					chunk_size = max_rd;
				} else {
					chunk_size = msgs[i].len - readb;
					emit_stop_between = emit_stop;
				}
				ret = max32_i3c_do_one_xfer(
					regs, data, addr, true, msgs[i].buf + readb, chunk_size,
					is_read, emit_start, emit_stop_between, no_ending);
				if (ret < 0) {
					goto out_xfer_i2c_stop_unlock;
				}
				readb += chunk_size;
			}
		} else {
			ret = max32_i3c_do_one_xfer(regs, data, addr, true, msgs[i].buf,
						    msgs[i].len, is_read, emit_start, emit_stop,
						    no_ending);
		}
		if (ret < 0) {
			goto out_xfer_i2c_stop_unlock;
		}
	}

	ret = 0;

out_xfer_i2c_stop_unlock:
	max32_i3c_request_emit_stop(data, regs);
	max32_i3c_errwarn_clear_all_nowait(regs);
	max32_i3c_status_clear_all(regs);
	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(i3c, max32_i3c_driver_api) = {
	.i2c_api.configure = max32_i3c_i2c_api_configure,
	.i2c_api.transfer = max32_i3c_i2c_api_transfer,
	.i2c_api.recover_bus = max32_i3c_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.i2c_api.iodev_submit = i2c_iodev_submit_fallback,
#endif

	.configure = max32_i3c_configure,
	.config_get = max32_i3c_config_get,

	.recover_bus = max32_i3c_recover_bus,

	.do_daa = max32_i3c_do_daa,
	.do_ccc = max32_i3c_do_ccc,

	.i3c_device_find = max32_i3c_device_find,

	.i3c_xfers = max32_i3c_transfer,

#ifdef CONFIG_I3C_USE_IBI
	.ibi_enable = max32_i3c_ibi_enable,
	.ibi_disable = max32_i3c_ibi_disable,
#endif

#ifdef CONFIG_I3C_RTIO
	.iodev_submit = i3c_iodev_submit_fallback,
#endif
};

#define I3C_MAX32_DEVICE(id)                                                                       \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static void max32_i3c_config_func_##id(const struct device *dev);                          \
	static struct i3c_device_desc max32_i3c_device_array_##id[] =                              \
		I3C_DEVICE_ARRAY_DT_INST(id);                                                      \
	static struct i3c_i2c_device_desc max32_i3c_i2c_device_array_##id[] =                      \
		I3C_I2C_DEVICE_ARRAY_DT_INST(id);                                                  \
	static const struct max32_i3c_config max32_i3c_config_##id = {                             \
		.regs = (mxc_i3c_regs_t *)DT_INST_REG_ADDR(id),                                    \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                                   \
		.perclk.bus = DT_INST_CLOCKS_CELL(id, offset),                                     \
		.perclk.bit = DT_INST_CLOCKS_CELL(id, bit),                                        \
		.irq_config_func = max32_i3c_config_func_##id,                                     \
		.common.dev_list.i3c = max32_i3c_device_array_##id,                                \
		.common.dev_list.num_i3c = ARRAY_SIZE(max32_i3c_device_array_##id),                \
		.common.dev_list.i2c = max32_i3c_i2c_device_array_##id,                            \
		.common.dev_list.num_i2c = ARRAY_SIZE(max32_i3c_i2c_device_array_##id),            \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                       \
		.disable_open_drain_high_pp = DT_INST_PROP(id, disable_open_drain_high_pp),        \
	};                                                                                         \
	static struct max32_i3c_data max32_i3c_data_##id = {                                       \
		.od_clock = DT_INST_PROP_OR(id, i3c_od_scl_hz, 0),                                 \
		.common.ctrl_config.scl.i3c = DT_INST_PROP_OR(id, i3c_scl_hz, 0),                  \
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(id, i2c_scl_hz, 0),                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, max32_i3c_init, NULL, &max32_i3c_data_##id,                      \
			      &max32_i3c_config_##id, POST_KERNEL,                                 \
			      CONFIG_I3C_CONTROLLER_INIT_PRIORITY, &max32_i3c_driver_api);         \
	static void max32_i3c_config_func_##id(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), max32_i3c_isr,            \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	};

DT_INST_FOREACH_STATUS_OKAY(I3C_MAX32_DEVICE)
