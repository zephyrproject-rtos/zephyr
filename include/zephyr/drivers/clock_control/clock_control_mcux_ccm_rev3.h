/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCUX_CCM_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCUX_CCM_H_

#include <stdint.h>
#include <zephyr/device.h>

#define IMX_CCM_MAX_SOURCES 4

#define IMX_CCM_FIXED_FREQ(src) ((src).source.fixed.freq)
#define IMX_CCM_PLL_MAX_FREQ(src) ((src).source.pll.max_freq)
#define IMX_CCM_RATE_LIMIT(src) \
	((src).type == IMX_CCM_TYPE_FIXED ? \
	 IMX_CCM_FIXED_FREQ(src) : IMX_CCM_PLL_MAX_FREQ(src))

enum imx_ccm_clock_state {
	IMX_CCM_CLOCK_STATE_INIT = 0, /* initial state - clock may be gated or not */
	IMX_CCM_CLOCK_STATE_GATED, /* clock is gated */
	IMX_CCM_CLOCK_STATE_UNGATED, /* clock is not gated */
};

enum imx_ccm_type {
	IMX_CCM_TYPE_FIXED = 1,
	IMX_CCM_TYPE_PLL,
	IMX_CCM_TYPE_MAX,
};

struct imx_ccm_fixed {
	char *name;
	uint32_t id;
	uint32_t freq;
};

struct imx_ccm_pll {
	char *name;
	uint32_t id;
	uint32_t max_freq; /* maximum allowed frequency */
	uint32_t offset; /* offset from PLL regmap */
};

struct imx_ccm_source {
	enum imx_ccm_type type; /* source type - fixed or PLL */
	union {
		struct imx_ccm_fixed fixed;
		struct imx_ccm_pll pll;
	} source;
};

struct imx_ccm_clock_root {
	char *name;
	uint32_t id;
	struct imx_ccm_source sources[IMX_CCM_MAX_SOURCES];
	uint32_t source_num;
};

struct imx_ccm_clock {
	char *name;
	uint32_t id;
	struct imx_ccm_clock_root root;

	uint32_t lpcg_regmap_phys;
	uint32_t lpcg_regmap_size;
	mm_reg_t lpcg_regmap;

	uint32_t state;
};

/* there are a few possible restrictions regarding the clock array:
 *
 *	1) The index of the clock sources needs to match the
 *	MUX value.
 *		e.g:
 *			The clock root called lpuartx_clock_root has
 *			the following possible MUX values:
 *				00 => PLL0
 *				01 => PLL1
 *				10 => PLL2
 *				11 => PLL3
 *			This means that the clock root structure needs
 *			to be defined as follows:
 *				struct imx_ccm_clock_root root = {
 *					.name = "some_name",
 *					.id = ID_FROM_NXP_HAL,
 *					.sources = {PLL0, PLL1, PLL2, PLL3},
 *					.source_num = 4,
 *				}
 *	* note: this restriction is optional as some developers may choose not to
 *	follow it for their SoC.
 *
 *
 *	2) The macros defined in zephyr/clock/soc_name_ccm.h need to match the indexes
 *	from the clock array.
 *		e.g: lpuart1_clock is found at index 5 in the clock array. This means
 *		that SOC_NAME_LPUART1_CLOCK needs to have value 5.
 *
 *		* note: this restriction is mandatory as it's used in the CCM driver.
 *
 *	3) The ID value from a clock IP/root/source needs to be a value from the NXP HAL.
 *		* note: this restriction is optional as some developers may choose not
 *		to follow it for their SoC.
 */
struct imx_ccm_clock_config {
	uint32_t clock_num;
	struct imx_ccm_clock *clocks;
};

struct imx_ccm_config {
	struct imx_ccm_clock_config *clock_config;

	uint32_t regmap_phys;
	uint32_t regmap_size;

	uint32_t pll_regmap_phys;
	uint32_t pll_regmap_size;
};


struct imx_ccm_data {
	mm_reg_t regmap;
	mm_reg_t pll_regmap;
};

/* disable/enable a given clock
 *
 * it's up to the user of the clock control API to
 * make sure that the sequence of operations is valid.
 */
int imx_ccm_clock_on_off(const struct device *dev, struct imx_ccm_clock *clk, bool on);

/* get the frequency of a given clock */
int imx_ccm_clock_get_rate(const struct device *dev, struct imx_ccm_clock *clk);

/* set the rate of a clock.
 *
 * if successful, the function will return the new rate which
 * may differ from the requested rate.
 */
int imx_ccm_clock_set_rate(const struct device *dev, struct imx_ccm_clock *clk, uint32_t rate);

int imx_ccm_init(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCUX_CCM_H_ */
