/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/canbus/j1939.h>
#include <zephyr/sys/iterable_sections.h>

#define SLEEP_TIME K_MSEC(1000)

static struct k_work_delayable j1939_work;

static void j1939_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	j1939_task();

	k_work_schedule(&j1939_work,
			K_MSEC(10));
}

const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

J1939_NODE_DEFINE(diag_node, /* Name */
			can_dev, /* CAN device */
			0x11, /* Default Source address */
			1234, /* ID number */
			1, /* Manufacturer code */
			1, /* ECU instance */
			1, /* Function instance */
			1, /* Function */
			1, /* Vehicle system */
			1, /* Vehicle system instance */
			1, /* Industry group */
			false /* Arbitrary address capable */
		);

J1939_NODE_DEFINE(data_node, /* Name */
			can_dev, /* CAN device */
			0x12, /* Default Source address */
			1235, /* ID number */
			1, /* Manufacturer code */
			1, /* ECU instance */
			1, /* Function instance */
			1, /* Function */
			1, /* Vehicle system */
			1, /* Vehicle system instance */
			1, /* Industry group */
			false /* Arbitrary address capable */
		);

/** Serial number of the device */
char hardware_serial_number[] = "12345";

/** Hardware revision of the device */
char hardware_revision[] = "1.0";

/** Part number of the device */
char hardware_part_number[] = "ABC123";

void j1939_app_get_device_info(j1939_device_info_t *device_info)
{
	*device_info = (j1939_device_info_t){
		.hardware_serial_number = hardware_serial_number,
		.hardware_revision = hardware_revision,
		.hardware_part_number = hardware_part_number,
	};
}

char *my_stpcpy(char *dest, const char *src)
{
	while ((*dest = *src)) {
		dest++;
		src++;
	}
	return dest;
}

void j1939_get_software_id(uint8_t **software_id_ptr)
{
	/* SW: Software assembly 0001 */
	*software_id_ptr = my_stpcpy(*software_id_ptr, "SW0001");
	*software_id_ptr = my_stpcpy(*software_id_ptr, ",");
	*software_id_ptr = my_stpcpy(*software_id_ptr, "ABC123");
	*software_id_ptr = my_stpcpy(*software_id_ptr, ",");
	*software_id_ptr = my_stpcpy(*software_id_ptr, "1.0");
	*software_id_ptr = my_stpcpy(*software_id_ptr, "#");
	/* AP: Application 0101 (Application SW) */
	*software_id_ptr = my_stpcpy(*software_id_ptr, "AP0101");
	*software_id_ptr = my_stpcpy(*software_id_ptr, ",");
	/* TODO: Get software part number */
	*software_id_ptr = my_stpcpy(*software_id_ptr, "MyApp");
	*software_id_ptr = my_stpcpy(*software_id_ptr, ",");
	/* TODO: Get software version number */
	*software_id_ptr = my_stpcpy(*software_id_ptr, "1.0.0");
	*software_id_ptr = my_stpcpy(*software_id_ptr, "#");
	/* AC: Application Config 0101 (Application SW Details) */
	*software_id_ptr = my_stpcpy(*software_id_ptr, "AC0101");
	*software_id_ptr = my_stpcpy(*software_id_ptr, ",");
	*software_id_ptr = my_stpcpy(*software_id_ptr, __DATE__);
	*software_id_ptr = my_stpcpy(*software_id_ptr, ",");
	*software_id_ptr = my_stpcpy(*software_id_ptr, "1.0.0");
	*software_id_ptr = my_stpcpy(*software_id_ptr, "#");
	/* '*' at the end of the entire set of s/w assembly */
	*software_id_ptr = my_stpcpy(*software_id_ptr, "*");
}

int main(void)
{
	int ret;
	int j1939_nodes_count;

	STRUCT_SECTION_COUNT(j1939_node_cfg, &j1939_nodes_count);
	printf("Number of nodes: %d\n", j1939_nodes_count);

	if (!device_is_ready(can_dev)) {
		printf("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}

	ret = can_start(can_dev);
	if (ret != 0) {
		printf("Error starting CAN controller [%d]", ret);
		return 0;
	}

	j1939_init();

	k_work_init_delayable(&j1939_work, j1939_work_handler);
	k_work_schedule(&j1939_work,
			K_MSEC(10));

	printf("Finished init.\n");

	uint8_t data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

	while (1) {
		/* node will automatically address claim */


		/* TODO - looks like this is sending before address claim is complete
		 * Need to implement a method to prevent sending from an unclaimed address
		 */
		j1939_transmit_pgn(
			J1939_Priority_6,
			0x1200,
			0x34,
			data,
			8,
			&diag_node);

		k_sleep(SLEEP_TIME);
	}
}
