/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>

void main(void)
{
}

#if DT_NODE_EXISTS(DT_INST(0, vnd_gpio))
/* Fake GPIO device, needed for building drivers that use DEVICE_DT_GET()
 * to access GPIO controllers.
 */
DEVICE_DT_DEFINE(DT_INST(0, vnd_gpio), NULL, NULL, NULL, NULL,
		 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);
#endif
