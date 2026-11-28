/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Build-time verification that the "nordic,nrf-ain" devicetree node matches
 * the external analog input tables provided by nrfx, which are the source of
 * truth. Any difference fails the build.
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <nrfx.h>

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ain)

#define AIN_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_ain)

#define AIN_DT_PIN(idx)                                                                            \
	NRF_PIN_PORT_TO_PIN_NUMBER(DT_GPIO_PIN_BY_IDX(AIN_NODE, ain_gpios, idx),                   \
				   DT_PROP(DT_GPIO_CTLR_BY_IDX(AIN_NODE, ain_gpios, idx), port))

/* Input selection that nrfx is expected to hold for AIN<idx>. */
#define AIN_DT_PSEL(idx, has_ain_as_pin, legacy_prefix, not_present)                               \
	COND_CODE_1(DT_PROP_HAS_IDX(AIN_NODE, ain_gpios, idx),                                     \
		    (COND_CODE_1(has_ain_as_pin, (AIN_DT_PIN(idx)),                                \
				 (UTIL_CAT(legacy_prefix, idx)))),                                 \
		    (not_present))

/* nrfx tables may end with a trailing comma, which yields an empty element. */
#define AIN_COUNT_ONE(psel) IF_DISABLED(IS_EMPTY(psel), (+1))
#define AIN_NRFX_COUNT(...) (0 FOR_EACH(AIN_COUNT_ONE, (), __VA_ARGS__))
#define AIN_DT_COUNT        DT_PROP_LEN(AIN_NODE, ain_gpios)

#if defined(SAADC_PRESENT)
#include <hal/nrf_saadc.h>
#include <drivers/src/nrfx_saadc_common.h>

#define SAADC_AIN_DT(idx)                                                                          \
	AIN_DT_PSEL(idx, NRF_SAADC_HAS_AIN_AS_PIN, NRF_SAADC_INPUT_AIN,                            \
		    NRFX_SAADC_INPUT_NOT_PRESENT)

/* SAADC defines the full set of external inputs, so it must match exactly. */
#define SAADC_AIN_CHECK(idx, psel)                                                                 \
	IF_DISABLED(IS_EMPTY(psel),                                                                \
		    (BUILD_ASSERT((uint32_t)(psel) == (uint32_t)SAADC_AIN_DT(idx),                 \
				  "SAADC AIN" STRINGIFY(idx)                                       \
				  ": nordic,nrf-ain devicetree node differs from nrfx");))

FOR_EACH_IDX(SAADC_AIN_CHECK, (), SAADC_EXTERNAL_AIN_PSELS)

BUILD_ASSERT(AIN_NRFX_COUNT(SAADC_EXTERNAL_AIN_PSELS) == AIN_DT_COUNT,
	     "SAADC: number of AIN inputs differs between nordic,nrf-ain and nrfx");
#endif /* SAADC_PRESENT */

#if defined(COMP_PRESENT)
#include <hal/nrf_comp.h>
#include <drivers/src/nrfx_comp_common.h>

#define COMP_AIN_NOT_PRESENT (nrf_comp_input_t)NRFX_COMP_INPUT_NOT_PRESENT
#define COMP_AIN_DT(idx)                                                                           \
	AIN_DT_PSEL(idx, NRF_COMP_HAS_AIN_AS_PIN, NRF_COMP_INPUT_, COMP_AIN_NOT_PRESENT)

/* COMP may support only a subset of AIN inputs, but the pins must match. */
#define COMP_AIN_CHECK(idx, psel)                                                                  \
	IF_DISABLED(IS_EMPTY(psel),                                                                \
		    (BUILD_ASSERT(((uint32_t)(psel) == (uint32_t)COMP_AIN_NOT_PRESENT) ||          \
				  ((uint32_t)(psel) == (uint32_t)COMP_AIN_DT(idx)),                \
				  "COMP AIN" STRINGIFY(idx)                                        \
				  ": nordic,nrf-ain devicetree node differs from nrfx");))

FOR_EACH_IDX(COMP_AIN_CHECK, (), COMP_EXTERNAL_AIN_PSELS)

BUILD_ASSERT(AIN_NRFX_COUNT(COMP_EXTERNAL_AIN_PSELS) <= AIN_DT_COUNT,
	     "COMP: nrfx has more AIN inputs than nordic,nrf-ain");
#endif /* COMP_PRESENT */

#if defined(LPCOMP_PRESENT)
#include <hal/nrf_lpcomp.h>
#include <drivers/src/nrfx_lpcomp_common.h>

#define LPCOMP_AIN_NOT_PRESENT (nrf_lpcomp_input_t)NRFX_LPCOMP_INPUT_NOT_PRESENT
#define LPCOMP_AIN_DT(idx)                                                                         \
	AIN_DT_PSEL(idx, NRF_LPCOMP_HAS_AIN_AS_PIN, NRF_LPCOMP_INPUT_, LPCOMP_AIN_NOT_PRESENT)

/* LPCOMP may support only a subset of AIN inputs, but the pins must match. */
#define LPCOMP_AIN_CHECK(idx, psel)                                                                \
	IF_DISABLED(IS_EMPTY(psel),                                                                \
		    (BUILD_ASSERT(((uint32_t)(psel) == (uint32_t)LPCOMP_AIN_NOT_PRESENT) ||        \
				  ((uint32_t)(psel) == (uint32_t)LPCOMP_AIN_DT(idx)),              \
				  "LPCOMP AIN" STRINGIFY(idx)                                      \
				  ": nordic,nrf-ain devicetree node differs from nrfx");))

FOR_EACH_IDX(LPCOMP_AIN_CHECK, (), LPCOMP_EXTERNAL_AIN_PSELS)

BUILD_ASSERT(AIN_NRFX_COUNT(LPCOMP_EXTERNAL_AIN_PSELS) <= AIN_DT_COUNT,
	     "LPCOMP: nrfx has more AIN inputs than nordic,nrf-ain");
#endif /* LPCOMP_PRESENT */

#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ain) */
