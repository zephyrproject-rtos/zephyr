/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DT_BINDINGS_VIDEO_ADI_GMSL_H_
#define DT_BINDINGS_VIDEO_ADI_GMSL_H_

/**
 * @file
 * @brief Common devicetree constants for ADI GMSL deserializers and
 *        serializers (MAX96724, MAX96717, and family).
 */

/**
 * @defgroup adi_gmsl_dt_bindings ADI GMSL Devicetree Bindings
 * @{
 */

/**
 * @name GMSL link indices
 * Identify the GMSL link (A-D) connected to a deserializer input port.
 * @{
 */
#define ADI_GMSL_LINK_A 0 /**< GMSL link A */
#define ADI_GMSL_LINK_B 1 /**< GMSL link B */
#define ADI_GMSL_LINK_C 2 /**< GMSL link C */
#define ADI_GMSL_LINK_D 3 /**< GMSL link D */
/** @} */

/**
 * @name CSI-2 D-PHY indices
 * Identify the CSI-2 D-PHY (0-3) used by a deserializer CSI output port.
 * @{
 */
#define ADI_GMSL_PHY_0 0 /**< CSI-2 D-PHY 0 */
#define ADI_GMSL_PHY_1 1 /**< CSI-2 D-PHY 1 */
#define ADI_GMSL_PHY_2 2 /**< CSI-2 D-PHY 2 */
#define ADI_GMSL_PHY_3 3 /**< CSI-2 D-PHY 3 */
/** @} */

/** @} */

#endif /* DT_BINDINGS_VIDEO_ADI_GMSL_H_ */
