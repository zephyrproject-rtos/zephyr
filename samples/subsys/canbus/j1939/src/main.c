/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

#include <zephyr/canbus/j1939.h>

#define SLEEP_TIME K_MSEC(1000)

const struct device *const can_dev1 = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

#define J1939_DIAG_NODE DT_NODELABEL(diag_node)

static struct j1939_dt_node_cfg diag_node_s = 	{                                                              \
		.can_dev                 = can_dev1,
		.source_address          = 0x11,
		.id_number               = 1234,
		.manufacturer_code       = 1,
		.ecu_instance            = 1,
		.function_instance       = 1,
		.function                = 1,
		.vehicle_system          = 1,
		.vehicle_system_instance = 1,
		.industry_group          = 1,
		.arbitrary_address_capable = false,
		.node_state              = J1939Ac_State_WaitingStartupInit,
		.ac_timestamp            = 0,
		.recorded_bus_info_count = 0,
		.transmission_enabled    = false,
        .J1939Tp_TransmitBam     = false,
	};

static J1939_Node_T diag_node = &diag_node_s;

struct j1939_dt_node_cfg* j1939_nodes[] = {&diag_node_s};

int main(void)
{
	int ret;

	if (!device_is_ready(can_dev1)) {
		printf("CAN: Device %s not ready.\n", can_dev1->name);
		return 0;
	}

	ret = can_start(can_dev1);
	if (ret != 0) {
		printf("Error starting CAN controller [%d]", ret);
		return 0;
	}

	printf("Finished init.\n");

    uint8_t data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

	while (1) {
        // node will automatically address claim


        // TODO - looks like this is sending before address claim is complete
        // Need to implement a method to prevent sending from an unclaimed address
        J1939_TransmitPgn(
            J1939_Priority_6,      // Priority.
            0x1200, // PGN.
            0x34,           // Source Address to reply to
            data,                  // Data pointer.
            8, // Byte counter for pointer.
            diag_node);                             // CAN Node.

		k_sleep(SLEEP_TIME);
	}
}
