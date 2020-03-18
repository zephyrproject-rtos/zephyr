/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>

#if defined(CONFIG_DEVICE_STATUS_HANDLER)

/**
 * @brief Move the device along with any registered status handler
 *
 */
void device_status_handler(struct device *dev, int status);

#else /* CONFIG_DEVICE_STATUS_HANDLER */

static inline void device_status_handler(struct device *dev, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(status);
}

#endif /* CONFIG_DEVICE_STATUS_HANDLER */
