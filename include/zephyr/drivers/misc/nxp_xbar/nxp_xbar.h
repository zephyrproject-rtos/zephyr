/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_NXP_XBAR_H_
#define ZEPHYR_DRIVERS_MISC_NXP_XBAR_H_

#include <zephyr/devicetree.h>
#include <string.h>

/* Check which XBAR types are available */
#define XBAR_AVAILABLE ( \
	DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar) || \
	DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbara) || \
	DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbarb) || \
	DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar_1))

#if XBAR_AVAILABLE

/* Include all available XBAR headers */
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar)
#include <fsl_xbar.h>
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar_1)
#include <fsl_xbar.h>
#define XBAR_1_VARIANT
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbara)
#include <fsl_xbara.h>
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbarb)
#include <fsl_xbarb.h>
#endif

/**
 * @brief Initialize XBAR peripheral based on compatible string
 *
 * @param compat Compatible string (e.g., "nxp,mcux-xbara")
 * @param base Base address or instance number
 */
static inline void xbar_init_by_compat(const char *compat, uintptr_t base)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar)
	if (strcmp(compat, "nxp,mcux-xbar") == 0) {
		XBAR_Init((XBAR_Type *)base);
		return;
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbara)
	if (strcmp(compat, "nxp,mcux-xbara") == 0) {
		XBARA_Init((XBARA_Type *)base);
		return;
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbarb)
	if (strcmp(compat, "nxp,mcux-xbarb") == 0) {
		XBARB_Init((XBARB_Type *)base);
		return;
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar_1)
	if (strcmp(compat, "nxp,mcux-xbar-1") == 0) {
		XBAR_Init((xbar_instance_t)base);
		return;
	}
#endif
}

/**
 * @brief Connect XBAR signals based on compatible string
 *
 * @param compat Compatible string
 * @param base Base address or instance number
 * @param input Input signal
 * @param output Output signal
 */
static inline void xbar_connect_by_compat(const char *compat, uintptr_t base, uint32_t input,
										uint32_t output)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar)
	if (strcmp(compat, "nxp,mcux-xbar") == 0) {
		XBAR_SetSignalsConnection((XBAR_Type *)base,
								(xbar_input_signal_t)input,
								(xbar_output_signal_t)output);
		return;
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbara)
	if (strcmp(compat, "nxp,mcux-xbara") == 0) {
		XBARA_SetSignalsConnection((XBARA_Type *)base,
								(xbar_input_signal_t)input,
								(xbar_output_signal_t)output);
		return;
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbarb)
	if (strcmp(compat, "nxp,mcux-xbarb") == 0) {
		XBARB_SetSignalsConnection((XBARB_Type *)base,
								(xbar_input_signal_t)input,
								(xbar_output_signal_t)output);
		return;
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_xbar_1)
	if (strcmp(compat, "nxp,mcux-xbar-1") == 0) {
		/* XBAR_1 doesn't take base parameter */
		XBAR_SetSignalsConnection((xbar_input_signal_t)input,
								(xbar_output_signal_t)output);
		return;
	}
#endif
}

/* Convenience macros */
#define XBAR_INIT(compat, base) xbar_init_by_compat(compat, base)
#define XBAR_CONNECT(compat, base, input, output) \
	xbar_connect_by_compat(compat, base, input, output)


/* Get XBAR compatible string from phandle */
#define XBAR_COMPAT_STR(inst, xbar) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, xbar), \
		((const char *)DT_PROP(DT_INST_PHANDLE(inst, xbar), compatible)), \
		(NULL))

/* Get XBAR base address or instance number */
#ifdef XBAR_1_VARIANT
/* XBAR_1 uses instance enum, not base address */
#define XBAR_BASE(inst, xbar) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, xbar), \
		(DT_PROP(DT_INST_PHANDLE(inst, xbar), xbar_instance)), \
		(0))
#else
/* Other XBAR types use register base address */
#define XBAR_BASE(inst, xbar) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, xbar), \
		((uintptr_t)DT_REG_ADDR(DT_INST_PHANDLE(inst, xbar))), \
		(0))
#endif

/* Get XBAR maps array */
#define XBAR_MAPS(inst, xbar) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, xbar), \
		(DT_PROP(DT_INST_PHANDLE(inst, xbar), xbar_maps)), \
		(NULL))

/* Get XBAR maps array length */
#define XBAR_MAPS_LEN(inst, xbar) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, xbar), \
		(DT_PROP_LEN(DT_INST_PHANDLE(inst, xbar), xbar_maps)), \
		(0))

#endif /* XBAR_AVAILABLE */

#endif /* ZEPHYR_DRIVERS_MISC_NXP_XBAR_H_ */
