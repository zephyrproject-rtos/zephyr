/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Check the SAI clock root rates of the NXP CCM rev2 driver.
 *
 * On i.MX RT11xx and RT118x, clock_control_get_rate() must give the rate of
 * the SAI1 to SAI4 roots also without the I2S driver (audio codec drivers
 * read it as MCLK). IMX_CCM_SAI2_CLK to IMX_CCM_SAI4_CLK share the peripheral
 * part of IMX_CCM_SAI1_CLK, so the test gives each root a different rate:
 * a rate read from another root cannot pass.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>
#include <zephyr/ztest.h>
#include <fsl_clock.h>

#define SAI_ROOT_COUNT 4U

/*
 * Mux input 0 of the SAI roots is a 24 MHz on-chip RC oscillator on RT11xx
 * (OSC_RC_48M_DIV2) and on RT118x (OSC_RC_24M). It always runs, so the test
 * does not depend on the Audio PLL.
 */
#define SAI_ROOT_MUX_RC_OSC 0U

struct sai_root_setting {
	uint32_t mux;
	uint32_t div;
};

static const struct device *const ccm_dev = DEVICE_DT_GET(DT_NODELABEL(ccm));

static const uint32_t sai_clock_names[SAI_ROOT_COUNT] = {
	IMX_CCM_SAI1_CLK,
	IMX_CCM_SAI2_CLK,
	IMX_CCM_SAI3_CLK,
	IMX_CCM_SAI4_CLK,
};

static const clock_root_t sai_roots[SAI_ROOT_COUNT] = {
	kCLOCK_Root_Sai1,
	kCLOCK_Root_Sai2,
	kCLOCK_Root_Sai3,
	kCLOCK_Root_Sai4,
};

static struct sai_root_setting saved_settings[SAI_ROOT_COUNT];

/**
 * @brief Save the mux and the divider of the four SAI roots.
 */
static void save_sai_roots(void)
{
	for (size_t i = 0; i < SAI_ROOT_COUNT; i++) {
		saved_settings[i].mux = CLOCK_GetRootClockMux(sai_roots[i]);
		saved_settings[i].div = CLOCK_GetRootClockDiv(sai_roots[i]);
	}
}

/**
 * @brief Restore the mux and the divider of the four SAI roots.
 *
 * The divider is restored before the mux. Thus a root that goes back to a
 * fast source (for example the Audio PLL) never runs with the small divider
 * of the test.
 */
static void restore_sai_roots(void)
{
	for (size_t i = 0; i < SAI_ROOT_COUNT; i++) {
		CLOCK_SetRootClockDiv(sai_roots[i], saved_settings[i].div);
		CLOCK_SetRootClockMux(sai_roots[i], saved_settings[i].mux);
	}
}

/**
 * @brief Give SAI root n (index n - 1) the RC oscillator and the divider n.
 *
 * The mux is set before the divider, for the same reason as in
 * restore_sai_roots().
 */
static void set_distinct_sai_root_rates(void)
{
	for (size_t i = 0; i < SAI_ROOT_COUNT; i++) {
		CLOCK_SetRootClockMux(sai_roots[i], SAI_ROOT_MUX_RC_OSC);
		CLOCK_SetRootClockDiv(sai_roots[i], i + 1U);
	}
}

ZTEST(nxp_ccm_rev2_sai, test_rate_of_each_sai_root)
{
	uint32_t driver_rates[SAI_ROOT_COUNT];

	set_distinct_sai_root_rates();

	for (size_t i = 0; i < SAI_ROOT_COUNT; i++) {
		uint32_t hal_rate = CLOCK_GetRootClockFreq(sai_roots[i]);
		int ret = clock_control_get_rate(
			ccm_dev, (clock_control_subsys_t)(uintptr_t)sai_clock_names[i],
			&driver_rates[i]);

		TC_PRINT("SAI%u: driver %u Hz, HAL %u Hz\n", (unsigned int)(i + 1U),
			 driver_rates[i], hal_rate);
		zassert_ok(ret, "SAI%u: clock_control_get_rate() returned %d",
			   (unsigned int)(i + 1U), ret);
		zassert_not_equal(driver_rates[i], 0U, "SAI%u: rate is 0", (unsigned int)(i + 1U));
		zassert_equal(driver_rates[i], hal_rate, "SAI%u: driver %u Hz, HAL %u Hz",
			      (unsigned int)(i + 1U), driver_rates[i], hal_rate);
	}

	/* Same source, divider n: rate of SAI1 = n x rate of SAI n. */
	for (size_t i = 1; i < SAI_ROOT_COUNT; i++) {
		zassert_equal(driver_rates[0], (i + 1U) * driver_rates[i],
			      "SAI%u has %u Hz, SAI1 has %u Hz", (unsigned int)(i + 1U),
			      driver_rates[i], driver_rates[0]);
	}
}

static void *nxp_ccm_rev2_sai_setup(void)
{
	zassert_true(device_is_ready(ccm_dev), "CCM device is not ready");
	return NULL;
}

static void nxp_ccm_rev2_sai_before(void *fixture)
{
	ARG_UNUSED(fixture);
	save_sai_roots();
}

static void nxp_ccm_rev2_sai_after(void *fixture)
{
	ARG_UNUSED(fixture);
	restore_sai_roots();
}

ZTEST_SUITE(nxp_ccm_rev2_sai, NULL, nxp_ccm_rev2_sai_setup, nxp_ccm_rev2_sai_before,
	    nxp_ccm_rev2_sai_after, NULL);
