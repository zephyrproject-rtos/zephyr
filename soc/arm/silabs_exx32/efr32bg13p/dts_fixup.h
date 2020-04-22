/*
 * Copyright (c) 2020 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */

/* SoC level DTS fixup file */

#define DT_FLASH_DEV_BASE_ADDRESS	DT_SILABS_GECKO_FLASH_CONTROLLER_400E0000_BASE_ADDRESS
#define DT_FLASH_DEV_NAME		DT_SILABS_GECKO_FLASH_CONTROLLER_400E0000_LABEL

/* End of SoC Level DTS fixup file */
