/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define IFX_CAT1_CLKPATH_IN_IMO    0 /**< Select the IMO as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_EXT    1 /**< Select the EXT as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_ECO    2 /**< Select the ECO as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_ALTHF  3 /**< Select the ALTHF as the output of the path mux */
/* Select the DSI MUX output as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_DSIMUX 4
#define IFX_CAT1_CLKPATH_IN_LPECO  5 /**< Select the LPECO as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_IHO    6 /**< Select the IHO as the output of the path mux */
/* Select a DSI signal (0 - 15) as the output of the DSI mux and path mux.         \
 *   Make sure the DSI clock sources are available on used device.                   \
 */
#define IFX_CAT1_CLKPATH_IN_DSI    0x100
/**< Select the ILO (16) as the output of the DSI mux and path mux */
#define IFX_CAT1_CLKPATH_IN_ILO    0x110
/**< Select the WCO (17) as the output of the DSI mux and path mux */
#define IFX_CAT1_CLKPATH_IN_WCO    0x111
/**< Select the ALTLF (18) as the output of the DSI mux and path mux.                \
 *   Make sure the ALTLF clock sources in available on used device.                  \
 */
#define IFX_CAT1_CLKPATH_IN_ALTLF  0x112
/**< Select the PILO (19) as the output of the DSI mux and path mux.                 \
 *   Make sure the PILO clock sources in available on used device.                   \
 */
#define IFX_CAT1_CLKPATH_IN_PILO   0x113
/**< Select the ILO1 (20) as the output of the DSI mux and path mux */
#define IFX_CAT1_CLKPATH_IN_ILO1   0x114
