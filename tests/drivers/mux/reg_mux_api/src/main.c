/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test suite for the reg-mux (reg_mux) driver.
 *
 * Channel layout (see the per-board overlay):
 *   ch0  word[0] bits[1:0]   mask=0x003   max state=3
 *   ch1  word[0] bits[7:4]   mask=0x0f0   max state=15
 *   ch2  word[1] bits[7:0]   mask=0x0ff   max state=255
 *   ch3  word[1] bits[11:8]  mask=0xf00   max state=15
 *
 * Test consumer nodes:
 *   test_mux_ctrl_node  -- mux-controls: ch0 @ idx 0, ch2 @ idx 1
 *   test_mux_state_node -- mux-states:   ch1 @ idx 0 (default=10),
 *                                        ch3 @ idx 1 (default=7)
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/mux.h>

#ifdef CONFIG_ARCH_POSIX
#include <sys/mman.h>
#endif

#define TEST_MUX_NODE       DT_NODELABEL(test_mux)
#define TEST_CTRL_NODE      DT_NODELABEL(test_mux_ctrl_node)
#define TEST_STATE_NODE     DT_NODELABEL(test_mux_state_node)

/* Device under test */
static const struct device *test_mux_dev = DEVICE_DT_GET(TEST_MUX_NODE);

/*
 * Statically define mux_control descriptors from devicetree.
 *
 * mux-controls consumer (MUX_CONTROLS_SPEC, no embedded default):
 *   idx 0 -> ch0  (cells = {0})
 *   idx 1 -> ch2  (cells = {2})
 *
 * mux-states consumer (MUX_STATES_SPEC, default encoded in DT):
 *   idx 0 -> ch1, default state = 10  (cells = {1, 10})
 *   idx 1 -> ch3, default state = 7   (cells = {3, 7})
 */
MUX_CONTROL_DT_SPEC_DEFINE_ALL(TEST_CTRL_NODE);
MUX_CONTROL_DT_SPEC_DEFINE_ALL(TEST_STATE_NODE);

/* Convenience accessors */
#define CH0_CTRL   MUX_CONTROL_DT_GET_BY_IDX(TEST_CTRL_NODE, 0)
#define CH2_CTRL   MUX_CONTROL_DT_GET_BY_IDX(TEST_CTRL_NODE, 1)
#define CH1_STATE  MUX_CONTROL_DT_GET_BY_IDX(TEST_STATE_NODE, 0)
#define CH3_STATE  MUX_CONTROL_DT_GET_BY_IDX(TEST_STATE_NODE, 1)

/* -------------------------------------------------------------------------
 * Suite-level setup / teardown (platform-specific)
 * -----------------------------------------------------------------------*/

#ifdef CONFIG_ARCH_POSIX
/*
 * On native_sim the DT address (0x40000000) is used directly as a host
 * pointer.  mmap() the region before any test case runs so that the
 * driver's reads and writes land in valid, writable anonymous memory.
 */
static void *reg_mux_setup(void)
{
	void *p = mmap((void *)DT_REG_ADDR(TEST_MUX_NODE),
		       DT_REG_SIZE(TEST_MUX_NODE),
		       PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
		       -1, 0);

	if (p == MAP_FAILED) {
		printk("FATAL: mmap for test MMIO region at 0x%lx failed\n",
		       (unsigned long)DT_REG_ADDR(TEST_MUX_NODE));
		k_panic();
	}

	return NULL;
}

static void reg_mux_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	munmap((void *)DT_REG_ADDR(TEST_MUX_NODE), DT_REG_SIZE(TEST_MUX_NODE));
}
#endif /* CONFIG_ARCH_POSIX */

/*
 * Zero the scratch words before every test so each case starts from a
 * known all-zero state.
 */
static void reg_mux_before(void *fixture)
{
	ARG_UNUSED(fixture);

	volatile uint32_t *scratch = (volatile uint32_t *)DT_REG_ADDR(TEST_MUX_NODE);

	scratch[0] = 0U;
	scratch[1] = 0U;
	scratch[2] = 0U;
	scratch[3] = 0U;
}

/**
 * @brief The reg-mux device must be ready after system init.
 */
ZTEST(reg_mux, test_device_ready)
{
	zassert_true(device_is_ready(test_mux_dev),
		     "test_mux device is not ready");
}

/**
 * @brief mux_configure() sets the channel; mux_state_get() reads it back.
 *
 * Exercises the core read-modify-write path and the state read-back
 * verification inside reg_mux_configure().
 */
ZTEST(reg_mux, test_configure_and_state_get)
{
	uint32_t state;
	int ret;

	/* Set ch0 (bits[1:0]) to 2 */
	ret = mux_configure(test_mux_dev, CH0_CTRL, 2U);
	zassert_equal(ret, 0, "mux_configure ch0=2 failed: %d", ret);

	ret = mux_state_get(test_mux_dev, CH0_CTRL, &state);
	zassert_equal(ret, 0, "mux_state_get ch0 failed: %d", ret);
	zassert_equal(state, 2U, "ch0 state: expected 2, got %u", state);

	/* Update ch0 to 1 */
	ret = mux_configure(test_mux_dev, CH0_CTRL, 1U);
	zassert_equal(ret, 0, "mux_configure ch0=1 failed: %d", ret);

	ret = mux_state_get(test_mux_dev, CH0_CTRL, &state);
	zassert_equal(ret, 0, "mux_state_get ch0 (2nd) failed: %d", ret);
	zassert_equal(state, 1U, "ch0 state after update: expected 1, got %u", state);
}

/**
 * @brief Configuring one channel must not corrupt bits of an adjacent
 *        channel that shares the same register word.
 *
 * ch0 (bits[1:0]) and ch1 (bits[7:4]) both live in word[0].  Writing ch1
 * after ch0 must leave ch0 untouched.
 */
ZTEST(reg_mux, test_channel_isolation)
{
	uint32_t state0, state1;
	int ret;

	/* Anchor ch0 to 3 (max value for 2-bit field) */
	ret = mux_configure(test_mux_dev, CH0_CTRL, 3U);
	zassert_equal(ret, 0, "mux_configure ch0=3 failed: %d", ret);

	/* Set ch1 (bits[7:4]) to 0xA; word[0] should become 0xA3 */
	ret = mux_configure(test_mux_dev, CH1_STATE, 0xAU);
	zassert_equal(ret, 0, "mux_configure ch1=0xA failed: %d", ret);

	ret = mux_state_get(test_mux_dev, CH0_CTRL, &state0);
	zassert_equal(ret, 0);
	ret = mux_state_get(test_mux_dev, CH1_STATE, &state1);
	zassert_equal(ret, 0);

	zassert_equal(state0, 3U,
		      "ch0 corrupted after ch1 write: expected 3, got %u", state0);
	zassert_equal(state1, 0xAU,
		      "ch1 state wrong: expected 0xA, got %u", state1);
}

/**
 * @brief All four channels can be configured independently and read back
 *        with the correct values.
 */
ZTEST(reg_mux, test_multiple_channels)
{
	/* Values chosen to be within each channel's valid mask range */
	static const uint32_t expected[4] = {
		3U,    /* ch0: bits[1:0],  max=3   */
		5U,    /* ch1: bits[7:4],  max=15  */
		0xABU, /* ch2: bits[7:0],  max=255 */
		0xFU,  /* ch3: bits[11:8], max=15  */
	};
	uint32_t states[4];
	int ret;

	ret = mux_configure(test_mux_dev, CH0_CTRL,  expected[0]);
	zassert_equal(ret, 0, "configure ch0 failed: %d", ret);
	ret = mux_configure(test_mux_dev, CH1_STATE, expected[1]);
	zassert_equal(ret, 0, "configure ch1 failed: %d", ret);
	ret = mux_configure(test_mux_dev, CH2_CTRL,  expected[2]);
	zassert_equal(ret, 0, "configure ch2 failed: %d", ret);
	ret = mux_configure(test_mux_dev, CH3_STATE, expected[3]);
	zassert_equal(ret, 0, "configure ch3 failed: %d", ret);

	ret = mux_state_get(test_mux_dev, CH0_CTRL,  &states[0]);
	zassert_equal(ret, 0);
	ret = mux_state_get(test_mux_dev, CH1_STATE, &states[1]);
	zassert_equal(ret, 0);
	ret = mux_state_get(test_mux_dev, CH2_CTRL,  &states[2]);
	zassert_equal(ret, 0);
	ret = mux_state_get(test_mux_dev, CH3_STATE, &states[3]);
	zassert_equal(ret, 0);

	zassert_equal(states[0], expected[0],
		      "ch0 mismatch: expected %u, got %u", expected[0], states[0]);
	zassert_equal(states[1], expected[1],
		      "ch1 mismatch: expected %u, got %u", expected[1], states[1]);
	zassert_equal(states[2], expected[2],
		      "ch2 mismatch: expected %u, got %u", expected[2], states[2]);
	zassert_equal(states[3], expected[3],
		      "ch3 mismatch: expected %u, got %u", expected[3], states[3]);
}

/**
 * @brief mux_configure_default() applies the state value that is embedded
 *        in the mux-states DT specifier.
 *
 * ch1 has DT default=10, ch3 has DT default=7.
 */
ZTEST(reg_mux, test_configure_default)
{
	uint32_t state;
	int ret;

	/* ch1: cells = {1, 10}, mux_configure_default uses cells[len-1] = 10 */
	ret = mux_configure_default(test_mux_dev, CH1_STATE);
	zassert_equal(ret, 0, "mux_configure_default ch1 failed: %d", ret);

	ret = mux_state_get(test_mux_dev, CH1_STATE, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 10U,
		      "ch1 default state: expected 10, got %u", state);

	/* ch3: cells = {3, 7}, mux_configure_default uses cells[len-1] = 7 */
	ret = mux_configure_default(test_mux_dev, CH3_STATE);
	zassert_equal(ret, 0, "mux_configure_default ch3 failed: %d", ret);

	ret = mux_state_get(test_mux_dev, CH3_STATE, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 7U,
		      "ch3 default state: expected 7, got %u", state);
}

/**
 * @brief mux_configure_default() on a MUX_CONTROLS_SPEC descriptor must
 *        return -ESRCH because mux-controls entries have no embedded state.
 */
ZTEST(reg_mux, test_configure_default_rejects_controls_spec)
{
	int ret = mux_configure_default(test_mux_dev, CH0_CTRL);

	zassert_equal(ret, -ESRCH,
		      "expected -ESRCH for mux-controls spec, got %d", ret);
}

/**
 * @brief mux_configure() with state=0 clears the channel's bitfield without
 *        disturbing the other channel sharing the same register word.
 */
ZTEST(reg_mux, test_configure_zero_clears_field)
{
	uint32_t state;
	int ret;

	/* Set ch2 to 0xFF then clear it; ch3 should remain untouched */
	ret = mux_configure(test_mux_dev, CH2_CTRL,  0xFFU);
	zassert_equal(ret, 0);
	ret = mux_configure(test_mux_dev, CH3_STATE, 0xFU);
	zassert_equal(ret, 0);

	ret = mux_configure(test_mux_dev, CH2_CTRL, 0U);
	zassert_equal(ret, 0, "mux_configure ch2=0 failed: %d", ret);

	ret = mux_state_get(test_mux_dev, CH2_CTRL, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 0U, "ch2 not cleared: got %u", state);

	ret = mux_state_get(test_mux_dev, CH3_STATE, &state);
	zassert_equal(ret, 0);
	zassert_equal(state, 0xFU,
		      "ch3 corrupted after ch2 clear: expected 0xF, got %u", state);
}

#ifdef CONFIG_ARCH_POSIX
ZTEST_SUITE(reg_mux, NULL, reg_mux_setup, reg_mux_before, NULL, reg_mux_teardown);
#else
ZTEST_SUITE(reg_mux, NULL, NULL, reg_mux_before, NULL, NULL);
#endif
