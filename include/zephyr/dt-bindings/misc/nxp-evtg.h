/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the NXP EVTG (Event Generator).
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_NXP_EVTG_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_NXP_EVTG_H_

/**
 * @defgroup nxp_evtg_aoi_input NXP EVTG AOI product-term input roles
 * @ingroup devicetree
 * @{
 *
 * How one event input (A, B, C or D) contributes to an AOI product term.
 * A product term is the AND of its four inputs after this role is applied; the
 * AOI output is the OR of its product terms. Values match the fsl_evtg
 * evtg_aoi_input_config_t enum so the driver can use them directly.
 */

/** Force the product term to 0 (disables the term). */
#define EVTG_FORCE0 0
/** Input participates in the term, true value. */
#define EVTG_PASS   1
/** Input participates in the term, inverted. */
#define EVTG_INVERT 2
/** Input excluded from the term (drops out of the AND). */
#define EVTG_IGNORE 3

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MISC_NXP_EVTG_H_ */
