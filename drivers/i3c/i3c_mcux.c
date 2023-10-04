/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019 NXP
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_i3c

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i3c.h>

#include <zephyr/drivers/pinctrl.h>

/*
 * This is from NXP HAL which contains register bits macros
 * which are used in this driver.
 */
#include <fsl_i3c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_mcux, CONFIG_I3C_MCUX_LOG_LEVEL);

#define I3C_MCTRL_REQUEST_NONE			I3C_MCTRL_REQUEST(0)
#define I3C_MCTRL_REQUEST_EMIT_START_ADDR	I3C_MCTRL_REQUEST(1)
#define I3C_MCTRL_REQUEST_EMIT_STOP		I3C_MCTRL_REQUEST(2)
#define I3C_MCTRL_REQUEST_IBI_ACK_NACK		I3C_MCTRL_REQUEST(3)
#define I3C_MCTRL_REQUEST_PROCESS_DAA		I3C_MCTRL_REQUEST(4)
#define I3C_MCTRL_REQUEST_FORCE_EXIT		I3C_MCTRL_REQUEST(6)
#define I3C_MCTRL_REQUEST_AUTO_IBI		I3C_MCTRL_REQUEST(7)

#define I3C_MCTRL_IBIRESP_ACK			I3C_MCTRL_IBIRESP(0)
#define I3C_MCTRL_IBIRESP_ACK_AUTO		I3C_MCTRL_IBIRESP(0)
#define I3C_MCTRL_IBIRESP_NACK			I3C_MCTRL_IBIRESP(1)
#define I3C_MCTRL_IBIRESP_ACK_WITH_BYTE		I3C_MCTRL_IBIRESP(2)
#define I3C_MCTRL_IBIRESP_MANUAL		I3C_MCTRL_IBIRESP(3)

#define I3C_MCTRL_TYPE_I3C			I3C_MCTRL_TYPE(0)
#define I3C_MCTRL_TYPE_I2C			I3C_MCTRL_TYPE(1)

#define I3C_MCTRL_DIR_WRITE			I3C_MCTRL_DIR(0)
#define I3C_MCTRL_DIR_READ			I3C_MCTRL_DIR(1)

#define I3C_MSTATUS_STATE_IDLE			I3C_MSTATUS_STATE(0)
#define I3C_MSTATUS_STATE_SLVREQ		I3C_MSTATUS_STATE(1)
#define I3C_MSTATUS_STATE_MSGSDR		I3C_MSTATUS_STATE(2)
#define I3C_MSTATUS_STATE_NORMACT		I3C_MSTATUS_STATE(3)
#define I3C_MSTATUS_STATE_MSGDDR		I3C_MSTATUS_STATE(4)
#define I3C_MSTATUS_STATE_DAA			I3C_MSTATUS_STATE(5)
#define I3C_MSTATUS_STATE_IBIACK		I3C_MSTATUS_STATE(6)
#define I3C_MSTATUS_STATE_IBIRCV		I3C_MSTATUS_STATE(7)

#define I3C_MSTATUS_IBITYPE_NONE		I3C_MSTATUS_IBITYPE(0)
#define I3C_MSTATUS_IBITYPE_IBI			I3C_MSTATUS_IBITYPE(1)
#define I3C_MSTATUS_IBITYPE_MR			I3C_MSTATUS_IBITYPE(2)
#define I3C_MSTATUS_IBITYPE_HJ			I3C_MSTATUS_IBITYPE(3)

struct mcux_i3c_config {
	/** Common I3C Driver Config */
	struct i3c_driver_config common;

	/** Pointer to controller registers. */
	I3C_Type *base;

	/** Pointer to the clock device. */
	const struct device *clock_dev;

	/** Clock control subsys related struct. */
	clock_control_subsys_t clock_subsys;

	/** Pointer to pin control device. */
	const struct pinctrl_dev_config *pincfg;

	/** Interrupt configuration function. */
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_i3c_data {
	/** Common I3C Driver Data */
	struct i3c_driver_data common;

	/** Configuration parameter to be used with HAL. */
	i3c_master_config_t ctrl_config_hal;

	/** Semaphore to serialize access for applications. */
	struct k_sem lock;

	/** Semaphore to serialize access for IBIs. */
	struct k_sem ibi_lock;

	struct {
		/**
		 * Clock divider for use when generating clock for
		 * I3C Push-pull mode.
		 */
		uint8_t clk_div_pp;

		/**
		 * Clock divider for use when generating clock for
		 * I3C open drain mode.
		 */
		uint8_t clk_div_od;

		/**
		 * Clock divider for the slow time control clock.
		 */
		uint8_t clk_div_tc;

		/** I3C open drain clock frequency in Hz. */
		uint32_t i3c_od_scl_hz;
	} clocks;

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
 * @param init_delay_us Initial delay in microsecond before reading register
 *                      (can be 0).
 * @param step_delay_us Delay in microsecond between each read of register
 *                      (cannot be 0).
 * @param total_delay_us Total delay in microsecond before bailing out.
 *
 * @retval 0 If masked register value matches before time out.
 * @retval -ETIMEDOUT Exhausted all delays without matching.
 */
static int reg32_poll_timeout(volatile uint32_t *reg,
			      uint32_t mask, uint32_t match,
			      uint32_t init_delay_us, uint32_t step_delay_us,
			      uint32_t total_delay_us)
{
	uint32_t delayed = init_delay_us;
	int ret = -ETIMEDOUT;

	if (init_delay_us > 0U) {
		k_busy_wait(init_delay_us);
	}

	while (delayed <= total_delay_us) {
		if ((sys_read32((mm_reg_t)reg) & mask) == match) {
			ret = 0;
			break;
		}

		k_busy_wait(step_delay_us);
		delayed += step_delay_us;
	}

	return ret;
}

/**
 * @brief Update register value.
 *
 * @param reg Pointer to 32-bit Register.
 * @param mask Mask to the register value.
 * @param update Value to be updated in register.
 */
static inline void reg32_update(volatile uint32_t *reg,
				uint32_t mask, uint32_t update)
{
	uint32_t val = sys_read32((mem_addr_t)reg);

	val &= ~mask;
	val |= (update & mask);

	sys_write32(val, (mem_addr_t)reg);
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
static inline bool reg32_test_match(volatile uint32_t *reg,
				    uint32_t mask, uint32_t match)
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
 * @breif Disable all interrupts.
 *
 * @param base Pointer to controller registers.
 *
 * @return Previous enabled interrupts.
 */
static uint32_t mcux_i3c_interrupt_disable(I3C_Type *base)
{
	uint32_t intmask = base->MINTSET;

	base->MINTCLR = intmask;

	return intmask;
}

/**
 * @brief Enable interrupts according to mask.
 *
 * @param base Pointer to controller registers.
 * @param mask Interrupts to be enabled.
 *
 */
static void mcux_i3c_interrupt_enable(I3C_Type *base, uint32_t mask)
{
	base->MINTSET = mask;
}

/**
 * @brief Check if there are any errors.
 *
 * This checks if MSTATUS has ERRWARN bit set.
 *
 * @retval True if there are any errors.
 * @retval False if no errors.
 */
static bool mcux_i3c_has_error(I3C_Type *base)
{
	uint32_t mstatus, merrwarn;

	mstatus = base->MSTATUS;
	if ((mstatus & I3C_MSTATUS_ERRWARN_MASK) == I3C_MSTATUS_ERRWARN_MASK) {
		merrwarn = base->MERRWARN;

		/*
		 * Note that this uses LOG_DBG() for displaying
		 * register values for debugging. In production builds,
		 * printing any error messages should be handled in
		 * callers of this function.
		 */
		LOG_DBG("ERROR: MSTATUS 0x%08x MERRWARN 0x%08x",
			mstatus, merrwarn);

		return true;
	}

	return false;
}

/**
 * @brief Check if there are any errors, and if one of them is time out error.
 *
 * @retval True if controller times out on operation.
 * @retval False if no time out error.
 */
static inline bool mcux_i3c_error_is_timeout(I3C_Type *base)
{
	if (mcux_i3c_has_error(base)) {
		if (reg32_test(&base->MERRWARN, I3C_MERRWARN_TIMEOUT_MASK)) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Check if there are any errors, and if one of them is NACK.
 *
 * NACK is generated when:
 * 1. Target does not ACK the last used address.
 * 2. All targets do not ACK on 0x7E.
 *
 * @retval True if NACK is received.
 * @retval False if no NACK error.
 */
static inline bool mcux_i3c_error_is_nack(I3C_Type *base)
{
	if (mcux_i3c_has_error(base)) {
		if (reg32_test(&base->MERRWARN, I3C_MERRWARN_NACK_MASK)) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Test if certain bits are set in MSTATUS.
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be tested.
 *
 * @retval True if @p mask bits are set.
 * @retval False if @p mask bits are not set.
 */
static inline bool mcux_i3c_status_is_set(I3C_Type *base, uint32_t mask)
{
	return reg32_test(&base->MSTATUS, mask);
}

/**
 * @brief Spin wait for MSTATUS bit to be set.
 *
 * This spins forever for the bits to be set.
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be tested.
 */
static inline void mcux_i3c_status_wait(I3C_Type *base, uint32_t mask)
{
	/* Wait for bits to be set */
	while (!mcux_i3c_status_is_set(base, mask)) {
		k_busy_wait(1);
	};
}

/**
 * @brief Wait for MSTATUS bits to be set with time out.
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be tested.
 * @param init_delay_us Initial delay in microsecond before reading register
 *                      (can be 0).
 * @param step_delay_us Delay in microsecond between each read of register
 *                      (cannot be 0).
 * @param total_delay_us Total delay in microsecond before bailing out.
 *
 * @retval 0 If bits are set before time out.
 * @retval -ETIMEDOUT Exhausted all delays.
 */
static inline int mcux_i3c_status_wait_timeout(I3C_Type *base, uint32_t mask,
					       uint32_t init_delay_us,
					       uint32_t step_delay_us,
					       uint32_t total_delay_us)
{
	return reg32_poll_timeout(&base->MSTATUS, mask, mask,
				  init_delay_us, step_delay_us, total_delay_us);
}

/**
 * @brief Clear the MSTATUS bits and wait for them to be cleared.
 *
 * This spins forever for the bits to be cleared;
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be cleared.
 */
static inline void mcux_i3c_status_clear(I3C_Type *base, uint32_t mask)
{
	/* Try to clear bit until it is cleared */
	while (1) {
		base->MSTATUS = mask;

		if (!mcux_i3c_status_is_set(base, mask)) {
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
 * @see I3C_MSTATUS_SLVSTART_MASK
 * @see I3C_MSTATUS_MCTRLDONE_MASK
 * @see I3C_MSTATUS_COMPLETE_MASK
 * @see I3C_MSTATUS_IBIWON_MASK
 * @see I3C_MSTATUS_ERRWARN_MASK
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_status_clear_all(I3C_Type *base)
{
	uint32_t mask = I3C_MSTATUS_SLVSTART_MASK |
			I3C_MSTATUS_MCTRLDONE_MASK |
			I3C_MSTATUS_COMPLETE_MASK |
			I3C_MSTATUS_IBIWON_MASK |
			I3C_MSTATUS_ERRWARN_MASK;

	mcux_i3c_status_clear(base, mask);
}

/**
 * @brief Clear the MSTATUS bits and wait for them to be cleared with time out.
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be cleared.
 * @param init_delay_us Initial delay in microsecond before reading register
 *                      (can be 0).
 * @param step_delay_us Delay in microsecond between each read of register
 *                      (cannot be 0).
 * @param total_delay_us Total delay in microsecond before bailing out.
 *
 * @retval 0 If bits are cleared before time out.
 * @retval -ETIMEDOUT Exhausted all delays.
 */
static inline int mcux_i3c_status_clear_timeout(I3C_Type *base, uint32_t mask,
						uint32_t init_delay_us,
						uint32_t step_delay_us,
						uint32_t total_delay_us)
{
	uint32_t delayed = init_delay_us;
	int ret = -ETIMEDOUT;

	/* Try to clear bit until it is cleared */
	while (delayed <= total_delay_us) {
		base->MSTATUS = mask;

		if (!mcux_i3c_status_is_set(base, mask)) {
			ret = 0;
			break;
		}

		k_busy_wait(step_delay_us);
		delayed += step_delay_us;
	}

	return ret;
}

/**
 * @brief Spin wait for MSTATUS bit to be set, and clear it afterwards.
 *
 * Note that this spins forever waiting for bits to be set, and
 * to be cleared.
 *
 * @see mcux_i3c_status_wait
 * @see mcux_i3c_status_clear
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be set and to be cleared;
 */
static inline void mcux_i3c_status_wait_clear(I3C_Type *base, uint32_t mask)
{
	mcux_i3c_status_wait(base, mask);
	mcux_i3c_status_clear(base, mask);
}

/**
 * @brief Wait for MSTATUS bit to be set, and clear it afterwards, with time out.
 *
 * @see mcux_i3c_status_wait_timeout
 * @see mcux_i3c_status_clear_timeout
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be set and to be cleared.
 * @param init_delay_us Initial delay in microsecond before reading register
 *                      (can be 0).
 * @param step_delay_us Delay in microsecond between each read of register
 *                      (cannot be 0).
 * @param total_delay_us Total delay in microsecond before bailing out.
 *
 * @retval 0 If masked register value matches before time out.
 * @retval -ETIMEDOUT Exhausted all delays without matching.
 */
static inline int mcux_i3c_status_wait_clear_timeout(I3C_Type *base, uint32_t mask,
						     uint32_t init_delay_us,
						     uint32_t step_delay_us,
						     uint32_t total_delay_us)
{
	int ret;

	ret = mcux_i3c_status_wait_timeout(base, mask, init_delay_us,
					   step_delay_us, total_delay_us);
	if (ret != 0) {
		goto out;
	}

	ret = mcux_i3c_status_clear_timeout(base, mask, init_delay_us,
					    step_delay_us, total_delay_us);

out:
	return ret;
}

/**
 * @brief Clear the MERRWARN register.
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_errwarn_clear_all_nowait(I3C_Type *base)
{
	base->MERRWARN = base->MERRWARN;
}

/**
 * @brief Tell controller to start DAA process.
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_request_daa(I3C_Type *base)
{
	reg32_update(&base->MCTRL,
		     I3C_MCTRL_REQUEST_MASK | I3C_MCTRL_IBIRESP_MASK | I3C_MCTRL_RDTERM_MASK,
		     I3C_MCTRL_REQUEST_PROCESS_DAA | I3C_MCTRL_IBIRESP_NACK);
}

/**
 * @brief Tell controller to start auto IBI.
 *
 * This also waits for the controller to indicate auto IBI
 * has started before returning.
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_request_auto_ibi(I3C_Type *base)
{
	reg32_update(&base->MCTRL,
		     I3C_MCTRL_REQUEST_MASK | I3C_MCTRL_IBIRESP_MASK | I3C_MCTRL_RDTERM_MASK,
		     I3C_MCTRL_REQUEST_AUTO_IBI | I3C_MCTRL_IBIRESP_ACK_AUTO);

	mcux_i3c_status_wait_clear(base, I3C_MSTATUS_MCTRLDONE_MASK);
}

/**
 * @brief Get the controller state.
 *
 * @param base Pointer to controller registers.
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
static inline uint32_t mcux_i3c_state_get(I3C_Type *base)
{
	uint32_t mstatus = base->MSTATUS;
	uint32_t state;

	/* Make sure we are in a state where we can emit STOP */
	state = (mstatus & I3C_MSTATUS_STATE_MASK) >> I3C_MSTATUS_STATE_SHIFT;

	return state;
}

/**
 * @brief Wait for MSTATUS bit to be set, and clear it afterwards with time out.
 *
 * @param base Pointer to controller registers.
 * @param mask Bits to be set.
 * @param init_delay_us Initial delay in microsecond before reading register
 *                      (can be 0).
 * @param step_delay_us Delay in microsecond between each read of register
 *                      (cannot be 0).
 * @param total_delay_us Total delay in microsecond before bailing out.
 *
 * @retval 0 If masked register value matches before time out.
 * @retval -ETIMEDOUT Exhausted all delays without matching.
 */
static inline int mcux_i3c_state_wait_timeout(I3C_Type *base, uint32_t state,
					      uint32_t init_delay_us,
					      uint32_t step_delay_us,
					      uint32_t total_delay_us)
{
	uint32_t delayed = init_delay_us;
	int ret = -ETIMEDOUT;

	/* Try to clear bit until it is cleared */
	while (delayed <= total_delay_us) {
		if (mcux_i3c_state_get(base) == state) {
			ret = 0;
			break;
		}

		k_busy_wait(step_delay_us);
		delayed += step_delay_us;
	}

	return ret;
}

/**
 * @brief Tell controller to emit START.
 *
 * @param base Pointer to controller registers.
 * @param addr Target address.
 * @param is_i2c True if this is I2C transactions, false if I3C.
 * @param is_read True if this is a read transaction, false if write.
 * @param read_sz Number of bytes to read if @p is_read is true.
 *
 * @return 0 if successful, or negative if error.
 */
static int mcux_i3c_request_emit_start(I3C_Type *base, uint8_t addr, bool is_i2c,
				       bool is_read, size_t read_sz)
{
	uint32_t mctrl;
	int ret = 0;

	mctrl = is_i2c ? I3C_MCTRL_TYPE_I2C : I3C_MCTRL_TYPE_I3C;
	mctrl |= I3C_MCTRL_IBIRESP_NACK;

	if (is_read) {
		mctrl |= I3C_MCTRL_DIR_READ;

		/* How many bytes to read */
		mctrl |= I3C_MCTRL_RDTERM(read_sz);
	} else {
		mctrl |= I3C_MCTRL_DIR_WRITE;
	}

	mctrl |= I3C_MCTRL_REQUEST_EMIT_START_ADDR | I3C_MCTRL_ADDR(addr);

	base->MCTRL = mctrl;

	/* Wait for controller to say the operation is done */
	ret = mcux_i3c_status_wait_clear_timeout(base, I3C_MSTATUS_MCTRLDONE_MASK,
						 0, 10, 1000);
	if (ret == 0) {
		/* Check for NACK */
		if (mcux_i3c_error_is_nack(base)) {
			ret = -ENODEV;
		}
	}

	return ret;
}

/**
 * @brief Tell controller to emit STOP.
 *
 * This emits STOP when controller is in NORMACT state as this is
 * the only valid state where STOP can be emitted. This also waits
 * for the controller to get out of NORMACT before returning.
 *
 * @param base Pointer to controller registers.
 * @param wait_stop True if need to wait for controller to be
 *                  no longer in NORMACT.
 */
static inline void mcux_i3c_request_emit_stop(I3C_Type *base, bool wait_stop)
{
	/* Make sure we are in a state where we can emit STOP */
	if (mcux_i3c_state_get(base) != I3C_MSTATUS_STATE_NORMACT) {
		return;
	}

	reg32_update(&base->MCTRL,
		     I3C_MCTRL_REQUEST_MASK | I3C_MCTRL_DIR_MASK | I3C_MCTRL_RDTERM_MASK,
		     I3C_MCTRL_REQUEST_EMIT_STOP);

	/*
	 * EMIT_STOP request doesn't result in MCTRLDONE being cleared
	 * so don't wait for it.
	 */

	if (wait_stop) {
		/*
		 * Note that we don't exactly wait for I3C_MSTATUS_STATE_IDLE.
		 * If there is an incoming IBI, it will get stuck forever
		 * as state would be I3C_MSTATUS_STATE_SLVREQ.
		 */
		while (reg32_test_match(&base->MSTATUS, I3C_MSTATUS_STATE_MASK,
					I3C_MSTATUS_STATE_NORMACT)) {
			if (mcux_i3c_has_error(base)) {
				/*
				 * Bail out if there is any error so
				 * we won't loop forever.
				 */
				return;
			}

			k_busy_wait(10);
		};
	}
}

/**
 * @brief Tell controller to NACK the incoming IBI.
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_ibi_respond_nack(I3C_Type *base)
{
	reg32_update(&base->MCTRL,
		     I3C_MCTRL_REQUEST_MASK | I3C_MCTRL_IBIRESP_MASK,
		     I3C_MCTRL_REQUEST_IBI_ACK_NACK | I3C_MCTRL_IBIRESP_NACK);

	mcux_i3c_status_wait_clear(base, I3C_MSTATUS_MCTRLDONE_MASK);
}

/**
 * @brief Tell controller to ACK the incoming IBI.
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_ibi_respond_ack(I3C_Type *base)
{
	reg32_update(&base->MCTRL,
		     I3C_MCTRL_REQUEST_MASK | I3C_MCTRL_IBIRESP_MASK,
		     I3C_MCTRL_REQUEST_IBI_ACK_NACK | I3C_MCTRL_IBIRESP_ACK_AUTO);

	mcux_i3c_status_wait_clear(base, I3C_MSTATUS_MCTRLDONE_MASK);
}

/**
 * @brief Get the number of bytes in RX FIFO.
 *
 * This returns the number of bytes in RX FIFO which
 * can be read.
 *
 * @param base Pointer to controller registers.
 *
 * @return Number of bytes in RX FIFO.
 */
static inline int mcux_i3c_fifo_rx_count_get(I3C_Type *base)
{
	uint32_t mdatactrl = base->MDATACTRL;

	return (int)((mdatactrl & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT);
}

/**
 * @brief Tell controller to flush both TX and RX FIFOs.
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_fifo_flush(I3C_Type *base)
{
	base->MDATACTRL = I3C_MDATACTRL_FLUSHFB_MASK | I3C_MDATACTRL_FLUSHTB_MASK;
}

/**
 * @brief Prepare the controller for transfers.
 *
 * This is simply a wrapper to clear out status bits,
 * and error bits. Also this tells the controller to
 * flush both TX and RX FIFOs.
 *
 * @param base Pointer to controller registers.
 */
static inline void mcux_i3c_xfer_reset(I3C_Type *base)
{
	mcux_i3c_status_clear_all(base);
	mcux_i3c_errwarn_clear_all_nowait(base);
	mcux_i3c_fifo_flush(base);
}

/**
 * @brief Drain RX FIFO.
 *
 * @param dev Pointer to controller device driver instance.
 */
static void mcux_i3c_fifo_rx_drain(const struct device *dev)
{
	const struct mcux_i3c_config *config = dev->config;
	I3C_Type *base = config->base;
	uint8_t buf;

	/* Read from FIFO as long as RXPEND is set. */
	while (mcux_i3c_status_is_set(base, I3C_MSTATUS_RXPEND_MASK)) {
		buf = base->MRDATAB;
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
static
struct i3c_device_desc *mcux_i3c_device_find(const struct device *dev,
					     const struct i3c_device_id *id)
{
	const struct mcux_i3c_config *config = dev->config;

	return i3c_dev_list_find(&config->common.dev_list, id);
}

/**
 * @brief Perform bus recovery.
 *
 * @param dev Pointer to controller device driver instance.
 */
static int mcux_i3c_recover_bus(const struct device *dev)
{
	const struct mcux_i3c_config *config = dev->config;
	I3C_Type *base = config->base;
	int ret = 0;

	/*
	 * If the controller is in NORMACT state, tells it to emit STOP
	 * so it can return to IDLE, or is ready to clear any pending
	 * target initiated IBIs.
	 */
	if (mcux_i3c_state_get(base) == I3C_MSTATUS_STATE_NORMACT) {
		mcux_i3c_request_emit_stop(base, true);
	};

	/* Exhaust all target initiated IBI */
	while (mcux_i3c_status_is_set(base, I3C_MSTATUS_SLVSTART_MASK)) {
		/* Tell the controller to perform auto IBI. */
		mcux_i3c_request_auto_ibi(base);

		if (mcux_i3c_status_wait_clear_timeout(base, I3C_MSTATUS_COMPLETE_MASK,
						       0, 10, 1000) == -ETIMEDOUT) {
			break;
		}

		/* Once auto IBI is done, discard bytes in FIFO. */
		mcux_i3c_fifo_rx_drain(dev);

		/*
		 * There might be other IBIs waiting.
		 * So pause a bit to let other targets initiates
		 * their IBIs.
		 */
		k_busy_wait(100);
	}

	if (reg32_poll_timeout(&base->MSTATUS, I3C_MSTATUS_STATE_MASK,
			       I3C_MSTATUS_STATE_IDLE, 0, 10, 1000) == -ETIMEDOUT) {
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
 * @param base Pointer to controller registers.
 * @param buf Buffer to store data.
 * @param buf_sz Buffer size in bytes.
 *
 * @return Number of bytes read, or negative if error.
 */
static int mcux_i3c_do_one_xfer_read(I3C_Type *base, uint8_t *buf, uint8_t buf_sz)
{
	int rx_count;
	bool completed = false;
	bool overflow = false;
	int ret = 0;
	int offset = 0;

	while (!completed) {
		/*
		 * Test if the COMPLETE bit is set.
		 */
		if (mcux_i3c_status_is_set(base, I3C_MSTATUS_COMPLETE_MASK)) {
			completed = true;
		}

		/*
		 * If controller says timed out, we abort the transaction.
		 */
		if (mcux_i3c_has_error(base)) {
			if (mcux_i3c_error_is_timeout(base)) {

				ret = -ETIMEDOUT;
			}

			base->MERRWARN = base->MERRWARN;

			goto one_xfer_read_out;
		}

		/*
		 * Transfer data from FIFO into buffer.
		 */
		rx_count = mcux_i3c_fifo_rx_count_get(base);
		while (rx_count > 0) {
			uint8_t data = (uint8_t)base->MRDATAB;

			if (offset < buf_sz) {
				buf[offset] = data;
				offset += 1;
			} else {
				overflow = true;
			}

			rx_count -= 1;
		}
	}

	if (overflow) {
		ret = -EINVAL;
	} else {
		ret = offset;
	}

one_xfer_read_out:
	return ret;
}

/**
 * @brief Perform one write transaction.
 *
 * This writes all data in @p buf to TX FIFO or time out
 * waiting for FIFO spaces.
 *
 * @param base Pointer to controller registers.
 * @param buf Buffer containing data to be sent.
 * @param buf_sz Number of bytes in @p buf to send.
 * @param no_ending True if not to signal end of write message.
 *
 * @return Number of bytes written, or negative if error.
 */
static int mcux_i3c_do_one_xfer_write(I3C_Type *base, uint8_t *buf, uint8_t buf_sz, bool no_ending)
{
	int offset = 0;
	int remaining = buf_sz;
	int ret = 0;

	while (remaining > 0) {
		ret = reg32_poll_timeout(&base->MDATACTRL, I3C_MDATACTRL_TXFULL_MASK, 0,
					 0, 10, 1000);
		if (ret == -ETIMEDOUT) {
			goto one_xfer_write_out;
		}

		if ((remaining > 1) || no_ending) {
			base->MWDATAB = (uint32_t)buf[offset];
		} else {
			base->MWDATABE = (uint32_t)buf[offset];
		}

		offset += 1;
		remaining -= 1;
	}

	ret = offset;

one_xfer_write_out:
	return ret;
}

/**
 * @brief Perform one transfer transaction.
 *
 * @param base Pointer to controller registers.
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
static int mcux_i3c_do_one_xfer(I3C_Type *base, struct mcux_i3c_data *data,
				uint8_t addr, bool is_i2c,
				uint8_t *buf, size_t buf_sz,
				bool is_read, bool emit_start, bool emit_stop,
				bool no_ending)
{
	int ret = 0;

	mcux_i3c_status_clear_all(base);
	mcux_i3c_errwarn_clear_all_nowait(base);

	/* Emit START if so desired */
	if (emit_start) {
		ret = mcux_i3c_request_emit_start(base, addr, is_i2c, is_read, buf_sz);
		if (ret != 0) {
			emit_stop = true;

			goto out_one_xfer;
		}
	}

	if ((buf == NULL) || (buf_sz == 0)) {
		goto out_one_xfer;
	}

	if (is_read) {
		ret = mcux_i3c_do_one_xfer_read(base, buf, buf_sz);
	} else {
		ret = mcux_i3c_do_one_xfer_write(base, buf, buf_sz, no_ending);
	}

	if (is_read || !no_ending) {
		/* Wait for controller to say the operation is done */
		ret = mcux_i3c_status_wait_clear_timeout(base, I3C_MSTATUS_COMPLETE_MASK,
							 0, 10, 1000);
		if (ret != 0) {
			LOG_DBG("%s: timed out addr 0x%02x, buf_sz %u",
				__func__, addr, buf_sz);
			emit_stop = true;

			goto out_one_xfer;
		}
	}

	if (mcux_i3c_has_error(base)) {
		ret = -EIO;
	}

out_one_xfer:
	if (emit_stop) {
		mcux_i3c_request_emit_stop(base, true);
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
static int mcux_i3c_transfer(const struct device *dev,
			     struct i3c_device_desc *target,
			     struct i3c_msg *msgs,
			     uint8_t num_msgs)
{
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *dev_data = dev->data;
	I3C_Type *base = config->base;
	uint32_t intmask;
	int ret;

	if (target->dynamic_addr == 0U) {
		ret = -EINVAL;
		goto out_xfer_i3c;
	}

	k_sem_take(&dev_data->lock, K_FOREVER);

	intmask = mcux_i3c_interrupt_disable(base);

	ret = mcux_i3c_state_wait_timeout(base, I3C_MSTATUS_STATE_IDLE, 0, 100, 100000);
	if (ret == -ETIMEDOUT) {
		goto out_xfer_i3c_unlock;
	}

	mcux_i3c_xfer_reset(base);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		bool is_read = (msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ;
		bool no_ending = false;

		/*
		 * Emit start if this is the first message or that
		 * the RESTART flag is set in message.
		 */
		bool emit_start = (i == 0) ||
				  ((msgs[i].flags & I3C_MSG_RESTART) == I3C_MSG_RESTART);

		bool emit_stop = (msgs[i].flags & I3C_MSG_STOP) == I3C_MSG_STOP;

		/*
		 * The controller requires special treatment of last byte of
		 * a write message. Since the API permits having a bunch of
		 * write messages without RESTART in between, this is just some
		 * logic to determine whether to treat the last byte of this
		 * message to be the last byte of a series of write mssages.
		 * If not, tell the write function not to treat it that way.
		 */
		if (!is_read && !emit_stop && ((i + 1) != num_msgs)) {
			bool next_is_write =
				(msgs[i + 1].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE;
			bool next_is_restart =
				((msgs[i + 1].flags & I3C_MSG_RESTART) == I3C_MSG_RESTART);

			if (next_is_write && !next_is_restart) {
				no_ending = true;
			}
		}

		ret = mcux_i3c_do_one_xfer(base, dev_data, target->dynamic_addr, false,
					   msgs[i].buf, msgs[i].len,
					   is_read, emit_start, emit_stop, no_ending);
		if (ret < 0) {
			goto out_xfer_i3c_stop_unlock;
		}
	}

	ret = 0;

out_xfer_i3c_stop_unlock:
	mcux_i3c_request_emit_stop(base, true);

out_xfer_i3c_unlock:
	mcux_i3c_errwarn_clear_all_nowait(base);
	mcux_i3c_status_clear_all(base);

	mcux_i3c_interrupt_enable(base, intmask);

	k_sem_give(&dev_data->lock);

out_xfer_i3c:
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
static int mcux_i3c_do_daa(const struct device *dev)
{
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *data = dev->data;
	I3C_Type *base = config->base;
	int ret = 0;
	uint8_t rx_buf[8] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
	size_t rx_count;
	uint8_t rx_size = 0;
	uint32_t intmask;

	k_sem_take(&data->lock, K_FOREVER);

	ret = mcux_i3c_state_wait_timeout(base, I3C_MSTATUS_STATE_IDLE, 0, 100, 100000);
	if (ret == -ETIMEDOUT) {
		goto out_daa_unlock;
	}

	LOG_DBG("DAA: ENTDAA");

	/* Disable I3C IRQ sources while we configure stuff. */
	intmask = mcux_i3c_interrupt_disable(base);

	mcux_i3c_xfer_reset(base);

	/* Emit process DAA */
	mcux_i3c_request_daa(base);

	/* Loop until no more responses from devices */
	do {
		/* Loop to grab data from devices (Provisioned ID, BCR and DCR) */
		do {
			if (mcux_i3c_has_error(base)) {
				LOG_ERR("DAA recv error");

				ret = -EIO;

				goto out_daa;
			}

			rx_count = mcux_i3c_fifo_rx_count_get(base);
			while (mcux_i3c_status_is_set(base, I3C_MSTATUS_RXPEND_MASK) &&
			       (rx_count != 0U)) {
				rx_buf[rx_size] = (uint8_t)(base->MRDATAB &
							    I3C_MRDATAB_VALUE_MASK);
				rx_size++;
				rx_count--;
			}
		} while (!mcux_i3c_status_is_set(base, I3C_MSTATUS_MCTRLDONE_MASK));

		mcux_i3c_status_clear(base, I3C_MSTATUS_MCTRLDONE_MASK);

		/* Figure out what address to assign to device */
		if ((mcux_i3c_state_get(base) == I3C_MSTATUS_STATE_DAA) &&
		    (mcux_i3c_status_is_set(base, I3C_MSTATUS_BETWEEN_MASK))) {
			struct i3c_device_desc *target;
			uint16_t vendor_id;
			uint32_t part_no;
			uint64_t pid;
			uint8_t dyn_addr;

			rx_size = 0;

			/* Vendor ID portion of Provisioned ID */
			vendor_id = (((uint16_t)rx_buf[0] << 8U) | (uint16_t)rx_buf[1]) &
				    0xFFFEU;

			/* Part Number portion of Provisioned ID */
			part_no = (uint32_t)rx_buf[2] << 24U | (uint32_t)rx_buf[3] << 16U |
				  (uint32_t)rx_buf[4] << 8U | (uint32_t)rx_buf[5];

			/* ... and combine into one Provisioned ID */
			pid = (uint64_t)vendor_id << 32U | (uint64_t)part_no;

			LOG_DBG("DAA: Rcvd PID 0x%04x%08x", vendor_id, part_no);

			ret = i3c_dev_list_daa_addr_helper(&data->common.attached_dev.addr_slots,
							   &config->common.dev_list, pid,
							   false, false,
							   &target, &dyn_addr);
			if (ret != 0) {
				goto out_daa;
			}

			/* Update target descriptor */
			target->dynamic_addr = dyn_addr;
			target->bcr = rx_buf[6];
			target->dcr = rx_buf[7];

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
			base->MWDATAB = dyn_addr;
			mcux_i3c_request_daa(base);

			LOG_DBG("PID 0x%04x%08x assigned dynamic address 0x%02x",
				vendor_id, part_no, dyn_addr);
		}

	} while (!mcux_i3c_status_is_set(base, I3C_MSTATUS_COMPLETE_MASK));

out_daa:
	/* Clear all flags. */
	mcux_i3c_errwarn_clear_all_nowait(base);
	mcux_i3c_status_clear_all(base);

	/* Re-Enable I3C IRQ sources. */
	mcux_i3c_interrupt_enable(base, intmask);

out_daa_unlock:
	k_sem_give(&data->lock);

	return ret;

	return -EIO;
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
static int mcux_i3c_do_ccc(const struct device *dev,
			   struct i3c_ccc_payload *payload)
{
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *data = dev->data;
	I3C_Type *base = config->base;
	int ret = 0;
	uint32_t intmask;

	if (payload == NULL) {
		return -EINVAL;
	}

	if (config->common.dev_list.num_i3c == 0) {
		/*
		 * No i3c devices in dev tree. Just return so
		 * we don't get errors doing cmds when there
		 * are no devices listening/responding.
		 */
		return 0;
	}

	k_sem_take(&data->lock, K_FOREVER);

	intmask = mcux_i3c_interrupt_disable(base);

	mcux_i3c_xfer_reset(base);

	LOG_DBG("CCC[0x%02x]", payload->ccc.id);

	/* Emit START */
	ret = mcux_i3c_request_emit_start(base, I3C_BROADCAST_ADDR, false, false, 0);
	if (ret < 0) {
		LOG_ERR("CCC[0x%02x] %s START error (%d)",
			payload->ccc.id,
			i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct",
			ret);

		goto out_ccc_stop;
	}

	/* Write the CCC code */
	mcux_i3c_status_clear_all(base);
	mcux_i3c_errwarn_clear_all_nowait(base);
	ret = mcux_i3c_do_one_xfer_write(base, &payload->ccc.id, 1,
					 payload->ccc.data_len > 0);
	if (ret < 0) {
		LOG_ERR("CCC[0x%02x] %s command error (%d)",
			payload->ccc.id,
			i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct",
			ret);

		goto out_ccc_stop;
	}

	/* Write additional data for CCC if needed */
	if (payload->ccc.data_len > 0) {
		mcux_i3c_status_clear_all(base);
		mcux_i3c_errwarn_clear_all_nowait(base);
		ret = mcux_i3c_do_one_xfer_write(base, payload->ccc.data,
						 payload->ccc.data_len, false);
		if (ret < 0) {
			LOG_ERR("CCC[0x%02x] %s command payload error (%d)",
				payload->ccc.id,
				i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct",
				ret);

			goto out_ccc_stop;
		}
	}

	/* Wait for controller to say the operation is done */
	ret = mcux_i3c_status_wait_clear_timeout(base, I3C_MSTATUS_COMPLETE_MASK, 0, 10, 1000);
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

			ret = mcux_i3c_do_one_xfer(base, data,
						   tgt_payload->addr, false,
						   tgt_payload->data,
						   tgt_payload->data_len,
						   is_read, emit_start, false, false);
			if (ret < 0) {
				LOG_ERR("CCC[0x%02x] target payload error (%d)",
					payload->ccc.id, ret);

				goto out_ccc_stop;
			}
		}
	}

out_ccc_stop:
	mcux_i3c_request_emit_stop(base, true);

	if (ret > 0) {
		ret = 0;
	}

	mcux_i3c_interrupt_enable(base, intmask);

	k_sem_give(&data->lock);

	return ret;
}

#ifdef CONFIG_I3C_USE_IBI
/**
 * @brief Callback to service target initiated IBIs.
 *
 * @param work Pointer to k_work item.
 */
static void mcux_i3c_ibi_work(struct k_work *work)
{
	uint8_t payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	size_t payload_sz = 0;

	struct i3c_ibi_work *i3c_ibi_work = CONTAINER_OF(work, struct i3c_ibi_work, work);
	const struct device *dev = i3c_ibi_work->controller;
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *data = dev->data;
	struct i3c_dev_attached_list *dev_list = &data->common.attached_dev;
	I3C_Type *base = config->base;
	struct i3c_device_desc *target = NULL;
	uint32_t mstatus, ibitype, ibiaddr;
	int ret;

	k_sem_take(&data->ibi_lock, K_FOREVER);

	if (mcux_i3c_state_get(base) != I3C_MSTATUS_STATE_SLVREQ) {
		LOG_DBG("IBI work %p running not because of IBI", work);
		LOG_DBG("MSTATUS 0x%08x MERRWARN 0x%08x",
			base->MSTATUS, base->MERRWARN);

		mcux_i3c_request_emit_stop(base, true);

		goto out_ibi_work;
	};

	/* Use auto IBI to service the IBI */
	mcux_i3c_request_auto_ibi(base);

	mstatus = sys_read32((mem_addr_t)&base->MSTATUS);
	ibiaddr = (mstatus & I3C_MSTATUS_IBIADDR_MASK) >> I3C_MSTATUS_IBIADDR_SHIFT;

	/*
	 * Note that the I3C_MSTATUS_IBI_TYPE_* are not shifted right.
	 * So no need to shift here.
	 */
	ibitype = (mstatus & I3C_MSTATUS_IBITYPE_MASK);

	/*
	 * Wait for COMPLETE bit to be set to indicate auto IBI
	 * has finished for hot-join and controller role request.
	 * For target interrupts, the IBI payload may be longer
	 * than the RX FIFO so we won't get the COMPLETE bit set
	 * at the first round of data read. So checking of
	 * COMPLETE bit is deferred to the reading.
	 */
	switch (ibitype) {
	case I3C_MSTATUS_IBITYPE_HJ:
		__fallthrough;

	case I3C_MSTATUS_IBITYPE_MR:
		if (mcux_i3c_status_wait_timeout(base, I3C_MSTATUS_COMPLETE_MASK,
						 0, 10, 1000) == -ETIMEDOUT) {
			LOG_ERR("Timeout waiting for COMPLETE");

			mcux_i3c_request_emit_stop(base, true);

			goto out_ibi_work;
		}
		break;

	default:
		break;
	};

	switch (ibitype) {
	case I3C_MSTATUS_IBITYPE_IBI:
		target = i3c_dev_list_i3c_addr_find(dev_list, (uint8_t)ibiaddr);
		if (target != NULL) {
			ret = mcux_i3c_do_one_xfer_read(base, &payload[0],
							sizeof(payload));
			if (ret >= 0) {
				payload_sz = (size_t)ret;
			} else {
				LOG_ERR("Error reading IBI payload");

				mcux_i3c_request_emit_stop(base, true);

				goto out_ibi_work;
			}
		} else {
			/* NACK IBI coming from unknown device */
			mcux_i3c_ibi_respond_nack(base);
		}
		break;
	case I3C_MSTATUS_IBITYPE_HJ:
		mcux_i3c_ibi_respond_ack(base);
		break;
	case I3C_MSTATUS_IBITYPE_MR:
		LOG_DBG("Controller role handoff not supported");
		mcux_i3c_ibi_respond_nack(base);
		break;
	default:
		break;
	}

	if (mcux_i3c_has_error(base)) {
		/*
		 * If the controller detects any errors, simply
		 * emit a STOP to abort the IBI. The target will
		 * raise IBI again if so desired.
		 */
		mcux_i3c_request_emit_stop(base, true);

		goto out_ibi_work;
	}

	switch (ibitype) {
	case I3C_MSTATUS_IBITYPE_IBI:
		if (target != NULL) {
			if (i3c_ibi_work_enqueue_target_irq(target,
							    &payload[0], payload_sz) != 0) {
				LOG_ERR("Error enqueue IBI IRQ work");
			}
		}

		/* Finishing the IBI transaction */
		mcux_i3c_request_emit_stop(base, true);
		break;
	case I3C_MSTATUS_IBITYPE_HJ:
		if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
			LOG_ERR("Error enqueue IBI HJ work");
		}
		break;
	case I3C_MSTATUS_IBITYPE_MR:
		break;
	default:
		break;
	}

out_ibi_work:
	mcux_i3c_xfer_reset(base);

	k_sem_give(&data->ibi_lock);

	/* Re-enable target initiated IBI interrupt. */
	base->MINTSET = I3C_MINTSET_SLVSTART_MASK;
}

static void mcux_i3c_ibi_rules_setup(struct mcux_i3c_data *data,
				     I3C_Type *base)
{
	uint32_t ibi_rules;
	int idx;

	ibi_rules = 0;

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		uint32_t addr_6bit;

		/* Extract the lower 6-bit of target address */
		addr_6bit = (uint32_t)data->ibi.addr[idx] & I3C_MIBIRULES_ADDR0_MASK;

		/* Shift into correct place */
		addr_6bit <<= idx * I3C_MIBIRULES_ADDR1_SHIFT;

		/* Put into the temporary IBI Rules register */
		ibi_rules |= addr_6bit;
	}

	if (!data->ibi.msb) {
		/* The MSB0 field is 1 if MSB is 0 */
		ibi_rules |= I3C_MIBIRULES_MSB0_MASK;
	}

	if (!data->ibi.has_mandatory_byte) {
		/* The NOBYTE field is 1 if there is no mandatory byte */
		ibi_rules |= I3C_MIBIRULES_NOBYTE_MASK;
	}

	/* Update the register */
	base->MIBIRULES = ibi_rules;

	LOG_DBG("MIBIRULES 0x%08x", ibi_rules);
}

int mcux_i3c_ibi_enable(const struct device *dev,
			struct i3c_device_desc *target)
{
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *data = dev->data;
	I3C_Type *base = config->base;
	struct i3c_ccc_events i3c_events;
	uint8_t idx;
	bool msb, has_mandatory_byte;
	int ret = 0;

	if (!i3c_device_is_ibi_capable(target)) {
		ret = -EINVAL;
		goto out;
	}

	if (data->ibi.num_addr >= ARRAY_SIZE(data->ibi.addr)) {
		/* No more free entries in the IBI Rules table */
		ret = -ENOMEM;
		goto out;
	}

	/* Check for duplicate */
	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (data->ibi.addr[idx] == target->dynamic_addr) {
			ret = -EINVAL;
			goto out;
		}
	}

	/* Disable controller interrupt while we configure IBI rules. */
	base->MINTCLR = I3C_MINTCLR_SLVSTART_MASK;

	LOG_DBG("IBI enabling for 0x%02x (BCR 0x%02x)",
		target->dynamic_addr, target->bcr);

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
		if ((has_mandatory_byte != data->ibi.has_mandatory_byte) ||
		    (msb != data->ibi.msb)) {
			ret = -EINVAL;
			goto out;
		}

		/* Find an empty address slot */
		for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
			if (data->ibi.addr[idx] == 0U) {
				break;
			}
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

	mcux_i3c_ibi_rules_setup(data, base);

	/* Tell target to enable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_ERR("Error sending IBI ENEC for 0x%02x (%d)",
			target->dynamic_addr, ret);
	}

out:
	if (data->ibi.num_addr > 0U) {
		/*
		 * Enable controller to raise interrupt when a target
		 * initiates IBI.
		 */
		base->MINTSET = I3C_MINTSET_SLVSTART_MASK;
	}

	return ret;
}

int mcux_i3c_ibi_disable(const struct device *dev,
			 struct i3c_device_desc *target)
{
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *data = dev->data;
	I3C_Type *base = config->base;
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
	base->MINTCLR = I3C_MINTCLR_SLVSTART_MASK;

	data->ibi.addr[idx] = 0U;
	data->ibi.num_addr -= 1U;

	/* Tell target to disable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_ERR("Error sending IBI DISEC for 0x%02x (%d)",
			target->dynamic_addr, ret);

		goto out;
	}

	mcux_i3c_ibi_rules_setup(data, base);

out:
	if (data->ibi.num_addr > 0U) {
		/*
		 * Enable controller to raise interrupt when a target
		 * initiates IBI.
		 */
		base->MINTSET = I3C_MINTSET_SLVSTART_MASK;
	}

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
static void mcux_i3c_isr(const struct device *dev)
{
#ifdef CONFIG_I3C_USE_IBI
	const struct mcux_i3c_config *config = dev->config;
	I3C_Type *base = config->base;

	/* Target initiated IBIs */
	if (mcux_i3c_status_is_set(base, I3C_MSTATUS_SLVSTART_MASK)) {
		/*
		 * Disable further target initiated IBI interrupt
		 * while we try to service the current one.
		 */
		base->MINTCLR = I3C_MINTCLR_SLVSTART_MASK;

		/*
		 * Handle IBI in workqueue.
		 */
		i3c_ibi_work_enqueue_cb(dev, mcux_i3c_ibi_work);
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
 * @retval -ENOSYS If not implemented.
 */
static int mcux_i3c_configure(const struct device *dev,
			      enum i3c_config_type type, void *config)
{
	const struct mcux_i3c_config *dev_cfg = dev->config;
	struct mcux_i3c_data *dev_data = dev->data;
	I3C_Type *base = dev_cfg->base;
	i3c_master_config_t *ctrl_config_hal = &dev_data->ctrl_config_hal;
	struct i3c_config_controller *ctrl_cfg = config;
	uint32_t clock_freq;
	int ret = 0;

	if (type != I3C_CONFIG_CONTROLLER) {
		ret = -EINVAL;
		goto out_configure;
	}

	/*
	 * Check for valid configuration parameters.
	 *
	 * Currently, must be the primary controller.
	 */
	if ((ctrl_cfg->is_secondary) ||
	    (ctrl_cfg->scl.i2c == 0U) ||
	    (ctrl_cfg->scl.i3c == 0U)) {
		ret = -EINVAL;
		goto out_configure;
	}

	/* Get the clock frequency */
	if (clock_control_get_rate(dev_cfg->clock_dev, dev_cfg->clock_subsys,
				   &clock_freq)) {
		ret = -EINVAL;
		goto out_configure;
	}

	ctrl_config_hal->baudRate_Hz.i2cBaud = ctrl_cfg->scl.i2c;
	ctrl_config_hal->baudRate_Hz.i3cPushPullBaud = ctrl_cfg->scl.i3c;

	/* Initialize hardware */
	I3C_MasterInit(base, ctrl_config_hal, clock_freq);

out_configure:
	return ret;
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
static int mcux_i3c_config_get(const struct device *dev,
			       enum i3c_config_type type, void *config)
{
	struct mcux_i3c_data *data = dev->data;
	int ret = 0;

	if ((type != I3C_CONFIG_CONTROLLER) || (config == NULL)) {
		ret = -EINVAL;
		goto out_configure;
	}

	(void)memcpy(config, &data->common.ctrl_config, sizeof(data->common.ctrl_config));

out_configure:
	return ret;
}

/**
 * @brief Initialize the hardware.
 *
 * @param dev Pointer to controller device driver instance.
 */
static int mcux_i3c_init(const struct device *dev)
{
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *data = dev->data;
	I3C_Type *base = config->base;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	int ret = 0;

	ret = i3c_addr_slots_init(dev);
	if (ret != 0) {
		goto err_out;
	}

	CLOCK_SetClkDiv(kCLOCK_DivI3cClk, data->clocks.clk_div_pp);
	CLOCK_SetClkDiv(kCLOCK_DivI3cSlowClk, data->clocks.clk_div_od);
	CLOCK_SetClkDiv(kCLOCK_DivI3cTcClk, data->clocks.clk_div_tc);

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		goto err_out;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->ibi_lock, 1, 1);

	/*
	 * Default controller configuration to act as the primary
	 * and active controller.
	 */
	I3C_MasterGetDefaultConfig(&data->ctrl_config_hal);

	/* Set default SCL clock rate (in Hz) */
	if (ctrl_config->scl.i2c == 0U) {
		ctrl_config->scl.i2c = data->ctrl_config_hal.baudRate_Hz.i2cBaud;
	}

	if (ctrl_config->scl.i3c == 0U) {
		ctrl_config->scl.i3c = data->ctrl_config_hal.baudRate_Hz.i3cPushPullBaud;
	}

	if (data->clocks.i3c_od_scl_hz != 0U) {
		data->ctrl_config_hal.baudRate_Hz.i3cOpenDrainBaud = data->clocks.i3c_od_scl_hz;
	}

	/* Currently can only act as primary controller. */
	data->common.ctrl_config.is_secondary = false;

	/* HDR mode not supported at the moment. */
	data->common.ctrl_config.supported_hdr = 0U;

	ret = mcux_i3c_configure(dev, I3C_CONFIG_CONTROLLER, ctrl_config);
	if (ret != 0) {
		ret = -EINVAL;
		goto err_out;
	}

	/* Disable all interrupts */
	base->MINTCLR = I3C_MINTCLR_SLVSTART_MASK |
			I3C_MINTCLR_MCTRLDONE_MASK |
			I3C_MINTCLR_COMPLETE_MASK |
			I3C_MINTCLR_RXPEND_MASK |
			I3C_MINTCLR_TXNOTFULL_MASK |
			I3C_MINTCLR_IBIWON_MASK |
			I3C_MINTCLR_ERRWARN_MASK |
			I3C_MINTCLR_NOWMASTER_MASK;

	/* Just in case the bus is not in idle. */
	ret = mcux_i3c_recover_bus(dev);
	if (ret != 0) {
		ret = -EIO;
		goto err_out;
	}

	/* Configure interrupt */
	config->irq_config_func(dev);

	/* Perform bus initialization */
	ret = i3c_bus_init(dev, &config->common.dev_list);

err_out:
	return ret;
}

static int mcux_i3c_i2c_api_configure(const struct device *dev, uint32_t dev_config)
{
	return -ENOSYS;
}

static int mcux_i3c_i2c_api_transfer(const struct device *dev,
				     struct i2c_msg *msgs,
				     uint8_t num_msgs,
				     uint16_t addr)
{
	const struct mcux_i3c_config *config = dev->config;
	struct mcux_i3c_data *dev_data = dev->data;
	I3C_Type *base = config->base;
	uint32_t intmask;
	int ret;

	k_sem_take(&dev_data->lock, K_FOREVER);

	intmask = mcux_i3c_interrupt_disable(base);

	ret = mcux_i3c_state_wait_timeout(base, I3C_MSTATUS_STATE_IDLE, 0, 100, 100000);
	if (ret == -ETIMEDOUT) {
		goto out_xfer_i2c_unlock;
	}

	mcux_i3c_xfer_reset(base);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		bool is_read = (msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool no_ending = false;

		/*
		 * Emit start if this is the first message or that
		 * the RESTART flag is set in message.
		 */
		bool emit_start = (i == 0) ||
				  ((msgs[i].flags & I2C_MSG_RESTART) == I2C_MSG_RESTART);

		bool emit_stop = (msgs[i].flags & I2C_MSG_STOP) == I2C_MSG_STOP;

		/*
		 * The controller requires special treatment of last byte of
		 * a write message. Since the API permits having a bunch of
		 * write messages without RESTART in between, this is just some
		 * logic to determine whether to treat the last byte of this
		 * message to be the last byte of a series of write mssages.
		 * If not, tell the write function not to treat it that way.
		 */
		if (!is_read && !emit_stop && ((i + 1) != num_msgs)) {
			bool next_is_write =
				(msgs[i + 1].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE;
			bool next_is_restart =
				((msgs[i + 1].flags & I2C_MSG_RESTART) == I2C_MSG_RESTART);

			if (next_is_write && !next_is_restart) {
				no_ending = true;
			}
		}

		ret = mcux_i3c_do_one_xfer(base, dev_data, addr, true,
					   msgs[i].buf, msgs[i].len,
					   is_read, emit_start, emit_stop, no_ending);
		if (ret < 0) {
			goto out_xfer_i2c_stop_unlock;
		}
	}

	ret = 0;

out_xfer_i2c_stop_unlock:
	mcux_i3c_request_emit_stop(base, true);

out_xfer_i2c_unlock:
	mcux_i3c_errwarn_clear_all_nowait(base);
	mcux_i3c_status_clear_all(base);

	mcux_i3c_interrupt_enable(base, intmask);

	k_sem_give(&dev_data->lock);

	return ret;
}

static const struct i3c_driver_api mcux_i3c_driver_api = {
	.i2c_api.configure = mcux_i3c_i2c_api_configure,
	.i2c_api.transfer = mcux_i3c_i2c_api_transfer,
	.i2c_api.recover_bus = mcux_i3c_recover_bus,

	.configure = mcux_i3c_configure,
	.config_get = mcux_i3c_config_get,

	.recover_bus = mcux_i3c_recover_bus,

	.do_daa = mcux_i3c_do_daa,
	.do_ccc = mcux_i3c_do_ccc,

	.i3c_device_find = mcux_i3c_device_find,

	.i3c_xfers = mcux_i3c_transfer,

#ifdef CONFIG_I3C_USE_IBI
	.ibi_enable = mcux_i3c_ibi_enable,
	.ibi_disable = mcux_i3c_ibi_disable,
#endif
};

#define I3C_MCUX_DEVICE(id)							\
	PINCTRL_DT_INST_DEFINE(id);						\
	static void mcux_i3c_config_func_##id(const struct device *dev);	\
	static struct i3c_device_desc mcux_i3c_device_array_##id[] =			\
		I3C_DEVICE_ARRAY_DT_INST(id);					\
	static struct i3c_i2c_device_desc mcux_i3c_i2c_device_array_##id[] =		\
		I3C_I2C_DEVICE_ARRAY_DT_INST(id);				\
	static const struct mcux_i3c_config mcux_i3c_config_##id = {		\
		.base = (I3C_Type *) DT_INST_REG_ADDR(id),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),		\
		.clock_subsys =							\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),	\
		.irq_config_func = mcux_i3c_config_func_##id,			\
		.common.dev_list.i3c = mcux_i3c_device_array_##id,			\
		.common.dev_list.num_i3c = ARRAY_SIZE(mcux_i3c_device_array_##id),	\
		.common.dev_list.i2c = mcux_i3c_i2c_device_array_##id,			\
		.common.dev_list.num_i2c = ARRAY_SIZE(mcux_i3c_i2c_device_array_##id),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),			\
	};									\
	static struct mcux_i3c_data mcux_i3c_data_##id = {			\
		.clocks.i3c_od_scl_hz = DT_INST_PROP_OR(id, i3c_od_scl_hz, 0),	\
		.common.ctrl_config.scl.i3c = DT_INST_PROP_OR(id, i3c_scl_hz, 0),	\
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(id, i2c_scl_hz, 0),	\
		.clocks.clk_div_pp = DT_INST_PROP(id, clk_divider),		\
		.clocks.clk_div_od = DT_INST_PROP(id, clk_divider_slow),	\
		.clocks.clk_div_tc = DT_INST_PROP(id, clk_divider_tc),		\
	};									\
	DEVICE_DT_INST_DEFINE(id,						\
			      mcux_i3c_init,					\
			      NULL,						\
			      &mcux_i3c_data_##id,				\
			      &mcux_i3c_config_##id,				\
			      POST_KERNEL,					\
			      CONFIG_I3C_CONTROLLER_INIT_PRIORITY,		\
			      &mcux_i3c_driver_api);				\
	static void mcux_i3c_config_func_##id(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(id),					\
			    DT_INST_IRQ(id, priority),				\
			    mcux_i3c_isr,					\
			    DEVICE_DT_INST_GET(id),				\
			    0);							\
		irq_enable(DT_INST_IRQN(id));					\
	};									\

DT_INST_FOREACH_STATUS_OKAY(I3C_MCUX_DEVICE)
