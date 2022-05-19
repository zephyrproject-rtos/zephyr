/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/factory_data/factory_data.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

struct factory_data {
	uint8_t uuid[16];
	uint8_t mac_address[6];
	uint8_t value_max_len[CONFIG_FACTORY_DATA_VALUE_LEN_MAX];
};

static int load_callback(const char *name, const uint8_t *value, size_t len, const void *param)
{
	struct factory_data *fd = (struct factory_data *)param;

	if (!strcmp(name, "uuid")) {
		memcpy(fd->uuid, value, MIN(sizeof(fd->uuid), len));
	} else if (!strcmp(name, "mac_address")) {
		memcpy(fd->mac_address, value, MIN(sizeof(fd->mac_address), len));
	} else if (!strcmp(name, "value_max_len")) {
		memcpy(fd->value_max_len, value, MIN(sizeof(fd->value_max_len), len));
	} else {
		/* unknown entry */
		printk("Error: unknown element '%s'\n", name);
	}

	return 0;
}

void main(void)
{
	struct factory_data fd;
	char uuid_hex_str[32 + 1];
	size_t i = 0;
	int ret;

	printk("\n*** Simplistic custom factory data implementation example ***\n");

	ret = factory_data_init();
	if (ret) {
		printk("Failed to initialize factory data: %d", ret);
		return;
	}

	printk("\nLoad all factory data entries\n");
	factory_data_load(load_callback, &fd);

	printk("\nPrint all factory data entries\n");

	bin2hex(fd.uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

	printk("- uuid: %s\n", uuid_hex_str);
	printk("- mac_address: %02x:%02x:%02x:%02x:%02x:%02x\n", fd.mac_address[0],
	       fd.mac_address[1], fd.mac_address[2], fd.mac_address[3], fd.mac_address[4],
	       fd.mac_address[5]);
	printk("- value_max_len:\n");
	for (i = 0; i < sizeof(fd.value_max_len); i++) {
		if (i % 16 == 0) {
			printk("  ");
		}
		if (i % 16 == 8) {
			printk("  ");
		}
		printk("%02x", fd.value_max_len[i]);
		if (i % 16 == 15) {
			printk("\n");
		}
	}

	printk("\nAbove values are booring? Use the factory data shell to manipulate them!\n");
}
