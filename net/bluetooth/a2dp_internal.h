/** @file
 * @brief Advance Audio Distribution Profile Internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <bluetooth/hci.h>
#include <stddef.h>
#include <sys/types.h>
#include <bluetooth/uuid.h>

enum bt_a2dp_stream_state {
	A2DP_STREAM_IDLE,
	A2DP_STREAM_STREAMING,
	A2DP_STREAM_SUSPENDED
};

/* To be called when first SEP is being registered */
int bt_a2dp_init(void);
