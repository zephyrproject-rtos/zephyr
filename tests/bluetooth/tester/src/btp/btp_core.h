/* bttester.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

/* Core Service */
#define CORE_READ_SUPPORTED_COMMANDS	0x01
struct core_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define CORE_READ_SUPPORTED_SERVICES	0x02
struct core_read_supported_services_rp {
	uint8_t data[0];
} __packed;

#define CORE_REGISTER_SERVICE		0x03
struct core_register_service_cmd {
	uint8_t id;
} __packed;

#define CORE_UNREGISTER_SERVICE		0x04
struct core_unregister_service_cmd {
	uint8_t id;
} __packed;

/* events */
#define CORE_EV_IUT_READY		0x80
