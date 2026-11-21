/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define HVX_HANDLE 0x0012
#define INDICATION_PAYLOAD "indication"
#define NOTIFICATION_PAYLOAD "notification"

/* Handle the DUT writes with Prepare Write Requests. The tester has no GATT
 * database: it responds to whatever handle it is given.
 */
#define LONG_WRITE_HANDLE 0x0020U
