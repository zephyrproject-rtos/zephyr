/*
 * Copyright (c) 2022 The Chromium OS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USBC_SINK_H__
#define __USBC_SINK_H__

/**
 * @brief Initialize the Sink state machine
 *
 * @return 0 on success
 * @return -EIO on failure
 */
int sink_init(void);

/**
 * @brief Run the Sink state machine
 *
 * @return time to wait before calling again
 */
int sink_sm(void);

#endif /* __USBC_SINK_H__ */
