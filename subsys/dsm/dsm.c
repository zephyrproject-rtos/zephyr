/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <dsm.h>

void device_status_handler(struct device *dev, int status)
{
	if ((device_check_status(dev) != 0) ||
	    ((device_check_status(dev) == 0) && (status < 0))) {
		/* Would push the dev into each and every registered
		 * status handler's ring buffer.
		 */
	}
}
