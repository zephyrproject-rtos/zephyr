/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileComment: IEC-61508-T3
 */

/* Set initial alignment to the 32 byte minimum for all MPUs */
_app_data_align = 32;
. = ALIGN(32);
