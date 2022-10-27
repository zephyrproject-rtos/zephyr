/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _VIEW_H_
#define _VIEW_H_

#include "mask.h"

/**
 * @brief Set the cc line that the snooper is monitoring
 *
 * @param vs bitmask that describes which cc lines should be monitored
 *
 * @return 0 on success
 * @return EIO on failure
 */
int view_set_snoop(snooper_mask_t vs);

/**
 * @brief Records the current cc line connection the Twinkie is detecting
 *
 * @param vc bitmask that describes the cc line that is currently connected
 *
 * @return 0 on success
 * @return EIO on failure
 */
int view_set_connection(snooper_mask_t vc);

/**
 * @brief Returns the value of the cc line currently being snooped. 
 *
 * @return bitmask for the current view status
 */
snooper_mask_t get_view_snoop();

#endif
