/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Shared state and helpers for the I2C fault injection suite.
 *
 * Each test file covers one fault class and needs the controller under test,
 * an address that nothing on the bus answers, and a way to assert that a
 * faulted transfer failed rather than hung.  The test setup is done in
 * main.c, and the test files reach its results through i2c_fault_ctx() and
 * the helpers declared here.
 */

#ifndef I2C_FAULT_H_
#define I2C_FAULT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/ztest.h>

/* Defines struct i2c_fault_outcome, which the transfer helpers below return. */
#include "i2c_timing.h"

/* Controller under test.  Every board supporting the suite provides this. */
#define I2C_FAULT_DUT_NODE DT_ALIAS(i2c_dut)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(I2C_FAULT_DUT_NODE),
	     "the i2c-dut alias must name an enabled I2C controller");

/*
 * A bounded return is documented only for a driver that selects
 * I2C_TRANSFER_TIMEOUT_SUPPORTED with a non-zero timeout, resolved from
 * zephyr,transfer-timeout-ms or CONFIG_I2C_TRANSFER_TIMEOUT_MS
 * (doc/hardware/peripherals/i2c.rst, "Transfer Timeout").  The suite can hold
 * a driver to it only when that timeout is shorter than the test wait limit
 * (CONFIG_I2C_FAULT_TEST_TIMEOUT_MS).
 */
#if defined(CONFIG_I2C_TRANSFER_TIMEOUT_SUPPORTED)
#define I2C_FAULT_DUT_TIMEOUT_MS                                                                   \
	DT_PROP_OR(I2C_FAULT_DUT_NODE, zephyr_transfer_timeout_ms, CONFIG_I2C_TRANSFER_TIMEOUT_MS)
#define I2C_FAULT_BOUND_DOCUMENTED                                                                 \
	((I2C_FAULT_DUT_TIMEOUT_MS > 0) &&                                                         \
	 (I2C_FAULT_DUT_TIMEOUT_MS < CONFIG_I2C_FAULT_TEST_TIMEOUT_MS))
#else
#define I2C_FAULT_BOUND_DOCUMENTED 0
#endif

/* Payload length the conditions transfer. */
#define I2C_FAULT_PAYLOAD_LEN CONFIG_I2C_FAULT_TEST_PAYLOAD_LEN

/** The controller under test, the addresses setup settled on, and setup results. */
struct i2c_fault_ctx {
	/** Controller under test. */
	const struct device *dut;
	/** Address on the bus under test that nothing answers. */
	uint16_t absent_addr;
	/** Address that answers, used for the known-good transfers. */
	uint16_t present_addr;
	/** Set when the known-good address answered its setup probe. */
	bool have_present;
	/**
	 * Set when a setup probe did not return within the wait limit.  No
	 * address could be verified, so every test skips.
	 */
	bool bus_unusable;
	/**
	 * Set when the absent address did not refuse its setup probe.  Every
	 * test relies on that refusal, and the API does not require a driver
	 * to support a zero-length write, so the run skips rather than fails.
	 */
	bool probe_unusable;
	/** Set once a transfer has failed to return within the wait limit. */
	bool hang_detected;
	/** Set when CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED declares the forcing GPIOs wired. */
	bool bus_force_ready;
};

const struct i2c_fault_ctx *i2c_fault_ctx(void);

void *i2c_fault_setup(void);
void i2c_fault_before(void *fixture);

/**
 * Skip the running test after a transfer did not return within the wait limit
 * on a controller for which the API documents no bounded return.
 */
void i2c_fault_skip_unbounded(void);

/**
 * Assert that a transfer run on the worker thread returned within the wait
 * limit.
 *
 * Where I2C_FAULT_BOUND_DOCUMENTED holds, a transfer that did not return
 * within the wait limit fails the test with the given message.  Elsewhere the
 * message is printed and the test skips, because the API does not promise the
 * driver returns.
 */
#define I2C_FAULT_ASSERT_RETURNED(outcome, ...)                                                    \
	do {                                                                                       \
		if ((outcome)->timed_out && !I2C_FAULT_BOUND_DOCUMENTED) {                         \
			TC_PRINT("  " __VA_ARGS__);                                                \
			i2c_fault_skip_unbounded();                                                \
		} else {                                                                           \
			zassert_false((outcome)->timed_out, __VA_ARGS__);                          \
		}                                                                                  \
	} while (false)

/** Skip the running test unless a known-good address is available. */
void i2c_fault_require_present(void);

/** Skip the running test unless the forcing GPIOs are wired and ready. */
void i2c_fault_require_bus_force(void);

/**
 * Assert that @p ret reports a failed transfer.
 *
 * The I2C API documents -EIO for a failed transfer and -ETIMEDOUT for one
 * that reached the transfer timeout, so those are what this requires by
 * default.  CONFIG_I2C_FAULT_TEST_RELAXED_ERRNO weakens it to any negative
 * value for bringing up a driver that is not yet conformant.  Either way the
 * value is recorded, because collecting what drivers return is one of the
 * outputs of this suite.
 */
void i2c_fault_assert_error(int ret, const char *condition);

/** Record the value a driver returned without asserting anything about it. */
void i2c_fault_record(const char *condition, int ret);

/**
 * Assert that the bus still works after a fault: the known-good address
 * still answers or, where none was found, the absent address still refuses.
 */
void i2c_fault_assert_healthy(const char *condition);

/**
 * Byte-mode callback table for a target registered only to check that
 * registration works.
 *
 * The I2C API does not mark any target callback optional, so a controller
 * driver may call these without checking for NULL when the target is
 * addressed.  They accept every write and return 0xFF for every read.
 */
extern const struct i2c_target_callbacks i2c_fault_noop_callbacks;

/**
 * Largest number of messages in one transfer run on the worker thread.  The
 * message array is copied into static storage of this size.
 */
#define I2C_FAULT_MAX_MSGS 4

/**
 * The arguments of one i2c_transfer() call, gathered into one struct so they
 * can be passed to i2c_fault_xfer_job(), which takes a single argument.
 */
struct i2c_fault_xfer {
	const struct device *bus;
	uint16_t addr;
	struct i2c_msg *msgs;
	uint8_t num_msgs;
};

/**
 * Job entry point that performs an @ref i2c_fault_xfer.
 *
 * Faulted transfers run on the worker thread rather than in the test thread,
 * so that a driver which never returns is reported as a hang instead of
 * stalling the whole run.
 */
int i2c_fault_xfer_job(void *arg);

/**
 * Run a transfer on the worker thread and report it through
 * I2C_FAULT_ASSERT_RETURNED() if it does not return within the wait limit.
 *
 * The message array is copied into static storage, because a job still
 * running at the wait limit is left running and the controller may still be
 * reading the descriptors after the test thread has unwound its stack.  The
 * data buffers the messages point at are not copied and must therefore
 * outlive the run; the suite gives them static storage for that reason.
 *
 * @param[in]  xfer      transfer to perform, at most @ref I2C_FAULT_MAX_MSGS
 *                       messages.
 * @param[in]  condition condition identifier used in the failure message.
 * @param[out] out       result and elapsed time.
 */
void i2c_fault_transfer_bounded(const struct i2c_fault_xfer *xfer, const char *condition,
				struct i2c_fault_outcome *out);

#endif /* I2C_FAULT_H_ */
