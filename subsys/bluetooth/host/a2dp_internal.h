/** @file
 * @brief Advance Audio Distribution Profile Internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum bt_a2dp_stream_state {
	A2DP_STREAM_IDLE,
	A2DP_STREAM_STREAMING,
	A2DP_STREAM_SUSPENDED
};

/* To be called when first SEP is being registered */
int bt_a2dp_init(void);
