/** @file
 *  @brief Audio Video Remote Control Profile internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define AVCTP_VER_1_4 (0x0104u)
#define AVRCP_VER_1_6 (0x0106u)

#define AVRCP_CAT_1 BIT(0) /* Player/Recorder */
#define AVRCP_CAT_2 BIT(1) /* Monitor/Amplifier */
#define AVRCP_CAT_3 BIT(2) /* Tuner */
#define AVRCP_CAT_4 BIT(3) /* Menu */

int bt_avrcp_init(void);
