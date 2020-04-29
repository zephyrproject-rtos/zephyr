/*
 * Copyright (c) 2019 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */

/* SoC level DTS fixup file */

#define DT_ADC_0_NAME	DT_LABEL(DT_NODELABEL(afec0))
#define DT_ADC_1_NAME	DT_LABEL(DT_NODELABEL(afec1))

/* End of SoC Level DTS fixup file */
