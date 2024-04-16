/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Clock control driver for Infineon CAT1 MCU family.
 */

#include <zephyr/drivers/clock_control.h>
#include <cyhal_clock.h>
#include <cyhal_utils.h>
#include <cyhal_clock_impl.h>

#define GET_CLK_SOURCE_ORD(N)  DT_DEP_ORD(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(N), 0))

/* Enumeration of enabled in device tree Clock, uses for indexing clock info table */
enum {
	INFINEON_CAT1_CLOCK_IMO,

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux0), okay)
	INFINEON_CAT1_CLOCK_PATHMUX0,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux1), okay)
	INFINEON_CAT1_CLOCK_PATHMUX1,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux2), okay)
	INFINEON_CAT1_CLOCK_PATHMUX2,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux3), okay)
	INFINEON_CAT1_CLOCK_PATHMUX3,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux4), okay)
	INFINEON_CAT1_CLOCK_PATHMUX4,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf0), okay)
	INFINEON_CAT1_CLOCK_HF0,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf1), okay)
	INFINEON_CAT1_CLOCK_HF1,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf2), okay)
	INFINEON_CAT1_CLOCK_HF2,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf3), okay)
	INFINEON_CAT1_CLOCK_HF3,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf4), okay)
	INFINEON_CAT1_CLOCK_HF4,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_fast), okay)
	INFINEON_CAT1_CLOCK_FAST,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_slow), okay)
	INFINEON_CAT1_CLOCK_SLOW,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_peri), okay)
	INFINEON_CAT1_CLOCK_PERI,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll0), okay)
	INFINEON_CAT1_CLOCK_PLL0,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll1), okay)
	INFINEON_CAT1_CLOCK_PLL1,
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(fll0), okay)
	INFINEON_CAT1_CLOCK_FLL0,
#endif

	/* Count of enabled clock */
	INFINEON_CAT1_ENABLED_CLOCK_COUNT
}; /* infineon_cat1_clock_info_name_t */

/* Clock info structure */
struct infineon_cat1_clock_info_t {
	cyhal_clock_t obj;      /* Hal Clock object */
	uint32_t dt_ord;        /* Device tree node's dependency ordinal */
};

/* Lookup table which presents  clock objects (cyhal_clock_t) correspondence to ordinal
 * number of device tree clock nodes.
 */
static struct infineon_cat1_clock_info_t
	clock_info_table[INFINEON_CAT1_ENABLED_CLOCK_COUNT] = {
	/* We always have IMO */
	[INFINEON_CAT1_CLOCK_IMO] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_imo)) },

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux0), okay)
	[INFINEON_CAT1_CLOCK_PATHMUX0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux0)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux1), okay)
	[INFINEON_CAT1_CLOCK_PATHMUX1] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux1)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux2), okay)
	[INFINEON_CAT1_CLOCK_PATHMUX2] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux2)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux3), okay)
	[INFINEON_CAT1_CLOCK_PATHMUX3] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux3)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux4), okay)
	[INFINEON_CAT1_CLOCK_PATHMUX4] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(path_mux4)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf0), okay)
	[INFINEON_CAT1_CLOCK_HF0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf0)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf1), okay)
	[INFINEON_CAT1_CLOCK_HF1] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf1)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf2), okay)
	[INFINEON_CAT1_CLOCK_HF2] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf2)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf3), okay)
	[INFINEON_CAT1_CLOCK_HF3] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf3)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf4), okay)
	[INFINEON_CAT1_CLOCK_HF4] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_hf4)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_fast), okay)
	[INFINEON_CAT1_CLOCK_FAST] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_fast)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_slow), okay)
	[INFINEON_CAT1_CLOCK_SLOW] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_slow)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_peri), okay)
	[INFINEON_CAT1_CLOCK_PERI] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(clk_peri)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll0), okay)
	[INFINEON_CAT1_CLOCK_PLL0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(pll0)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll1), okay)
	[INFINEON_CAT1_CLOCK_PLL1] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(pll1)) },
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(fll0), okay)
	[INFINEON_CAT1_CLOCK_FLL0] = { .dt_ord = DT_DEP_ORD(DT_NODELABEL(fll0)) },
#endif
};

static cy_rslt_t _configure_path_mux(cyhal_clock_t *clock_obj,
				     cyhal_clock_t *clock_source_obj,
				     const cyhal_clock_t *reserve_obj)
{
	cy_rslt_t rslt;

	ARG_UNUSED(clock_source_obj);

	rslt = cyhal_clock_reserve(clock_obj, reserve_obj);

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_source(clock_obj, clock_source_obj);
	}

	return rslt;
}

static cy_rslt_t _configure_clk_hf(cyhal_clock_t *clock_obj,
				   cyhal_clock_t *clock_source_obj,
				   const cyhal_clock_t *reserve_obj,
				   uint32_t clock_div)
{
	cy_rslt_t rslt;

	rslt = cyhal_clock_reserve(clock_obj, reserve_obj);

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_source(clock_obj, clock_source_obj);
	}

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_enabled(clock_obj, true, true);
	}

	return rslt;
}

static cy_rslt_t _configure_clk_frequency_and_enable(cyhal_clock_t *clock_obj,
						     cyhal_clock_t *clock_source_obj,
						     const cyhal_clock_t *reserve_obj,
						     uint32_t frequency)
{
	ARG_UNUSED(clock_source_obj);
	cy_rslt_t rslt;

	rslt = cyhal_clock_reserve(clock_obj, reserve_obj);

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_frequency(clock_obj, frequency, NULL);
	}

	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_enabled(clock_obj, true, true);
	}

	return rslt;
}

static cyhal_clock_t *_get_hal_obj_from_ord(uint32_t dt_ord)
{
	cyhal_clock_t *ret_obj = NULL;

	for (uint32_t i = 0u; i < INFINEON_CAT1_ENABLED_CLOCK_COUNT; i++) {
		if (clock_info_table[i].dt_ord == dt_ord) {
			ret_obj = &clock_info_table[i].obj;
			return ret_obj;
		}
	}
	return ret_obj;
}

static int clock_control_infineon_cat1_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	cy_rslt_t rslt;
	cyhal_clock_t *clock_obj = NULL;
	cyhal_clock_t *clock_source_obj = NULL;
	uint32 frequency;
	uint32 clock_div;

	/* Configure IMO */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_imo), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_IMO].obj;
	if (cyhal_clock_get(clock_obj, &CYHAL_CLOCK_RSC_IMO)) {
		return -EIO;
	}
#else
	#error "IMO clock must be enabled"
#endif

	/* Configure the PathMux[0] to source defined in tree device 'path_mux0' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux0), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX0].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux0));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[0])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[1] to source defined in tree device 'path_mux1' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux1), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX1].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux1));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[1])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[2] to source defined in tree device 'path_mux2' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux2), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX2].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux2));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[2])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[3] to source defined in tree device 'path_mux3' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux3), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX3].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux3));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[3])) {
		return -EIO;
	}
#endif

	/* Configure the PathMux[4] to source defined in tree device 'path_mux4' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(path_mux4), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PATHMUX4].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(path_mux4));

	if (_configure_path_mux(clock_obj, clock_source_obj, &CYHAL_CLOCK_PATHMUX[4])) {
		return -EIO;
	}
#endif

	/* Configure FLL0 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(fll0), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_FLL0].obj;
	frequency = DT_PROP(DT_NODELABEL(fll0), clock_frequency);

	rslt = _configure_clk_frequency_and_enable(clock_obj, clock_source_obj,
						   &CYHAL_CLOCK_FLL, frequency);
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure PLL0 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll0), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PLL0].obj;
	frequency = DT_PROP(DT_NODELABEL(pll0), clock_frequency);

	rslt = _configure_clk_frequency_and_enable(clock_obj, clock_source_obj,
						   &CYHAL_CLOCK_PLL[0], frequency);

	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure PLL1 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll1), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PLL1].obj;
	frequency = DT_PROP(DT_NODELABEL(pll1), clock_frequency);

	rslt = _configure_clk_frequency_and_enable(clock_obj, clock_source_obj,
						   &CYHAL_CLOCK_PLL[1], frequency);
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure the HF[0] to source defined in tree device 'clk_hf0' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf0), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF0].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf0));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf0), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[0], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[1] to source defined in tree device 'clk_hf1' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf1), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF1].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf1));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf1), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[1], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[2] to source defined in tree device 'clk_hf2' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf2), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF2].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf2));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf2), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[2], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[3] to source defined in tree device 'clk_hf3' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf3), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF3].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf3));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf3), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[3], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the HF[4] to source defined in tree device 'clk_hf4' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hf4), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_HF4].obj;
	clock_source_obj = _get_hal_obj_from_ord(GET_CLK_SOURCE_ORD(clk_hf4));
	clock_div = DT_PROP(DT_NODELABEL(clk_hf4), clock_div);

	if (_configure_clk_hf(clock_obj, clock_source_obj, &CYHAL_CLOCK_HF[4], clock_div)) {
		return -EIO;
	}
#endif

	/* Configure the clock fast to source defined in tree device 'clk_fast' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_fast), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_FAST].obj;
	clock_div = DT_PROP(DT_NODELABEL(clk_fast), clock_div);

	rslt = cyhal_clock_reserve(clock_obj, &CYHAL_CLOCK_FAST);
	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure the clock peri to source defined in tree device 'clk_peri' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_peri), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_PERI].obj;
	clock_div = DT_PROP(DT_NODELABEL(clk_peri), clock_div);

	rslt = cyhal_clock_reserve(clock_obj, &CYHAL_CLOCK_PERI);
	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}
	if (rslt) {
		return -EIO;
	}
#endif

	/* Configure the clock slow to source defined in tree device 'clk_slow' node */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_slow), okay)
	clock_obj = &clock_info_table[INFINEON_CAT1_CLOCK_SLOW].obj;
	clock_div = DT_PROP(DT_NODELABEL(clk_slow), clock_div);

	rslt = cyhal_clock_reserve(clock_obj, &CYHAL_CLOCK_SLOW);
	if (rslt == CY_RSLT_SUCCESS) {
		rslt = cyhal_clock_set_divider(clock_obj, clock_div);
	}
	if (rslt) {
		return -EIO;
	}
#endif

	return (int) rslt;
}

static int clock_control_infineon_cat_on_off(const struct device *dev,
					     clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	/* On/off functionality are not supported */
	return -ENOSYS;
}

static const struct clock_control_driver_api clock_control_infineon_cat1_api = {
	.on = clock_control_infineon_cat_on_off,
	.off = clock_control_infineon_cat_on_off
};

DEVICE_DT_DEFINE(DT_NODELABEL(clk_imo),
		 &clock_control_infineon_cat1_init,
		 NULL,
		 NULL,
		 NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_infineon_cat1_api);
