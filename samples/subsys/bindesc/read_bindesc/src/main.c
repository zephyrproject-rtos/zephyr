/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bindesc.h>
#include <zephyr/drivers/flash.h>

BINDESC_STR_DEFINE(my_string, 1, "Hello world!");
BINDESC_UINT_DEFINE(my_int, 2, 5);
BINDESC_BYTES_DEFINE(my_bytes, 3, ({1, 2, 3, 4}));

const struct device *flash = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

void dump_bytes(const uint8_t *buffer, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++) {
		printk("%02x ", buffer[i]);
	}
	printk("\n");
}

int dump_descriptors_callback(const struct bindesc_entry *entry, void *user_data)
{
	ARG_UNUSED(user_data);

	printk("tag: %hu len: %hu data: ", entry->tag, entry->len);

	switch (BINDESC_GET_TAG_TYPE(entry->tag)) {
	case BINDESC_TYPE_UINT:
		printk("%u\n", *(const uint32_t *)entry->data);
		break;
	case BINDESC_TYPE_STR:
		printk("%s\n", (const char *)entry->data);
		break;
	case BINDESC_TYPE_BYTES:
		dump_bytes((const uint8_t *)entry->data, entry->len);
		break;
	case BINDESC_TYPE_DESCRIPTORS_END:
		printk("Descriptors terminator\n");
		break;
	default:
		printk("\n");
		break;
	}

	return 0;
}

int main(void)
{
	size_t bindesc_offset = UINT16_MAX;
	const uint32_t *version_number;
	struct bindesc_handle handle;
	uint8_t buffer[0x100];
	const uint8_t *bytes;
	const char *version;
	size_t size;
	int retval;
	size_t i;

	/*
	 * In a normal application, the offset of the descriptors should be constant and known,
	 * usually right after the vector table. It can easily be retrieved using
	 * ``west bindesc get_offset path/to/zephyr.bin``.
	 * This sample however is intended for multiple devices, and therefore just searches for
	 * the descriptors in order to remain generic.
	 */
	for (i = 0; i < UINT16_MAX; i += sizeof(void *)) {
		if (*(uint64_t *)(CONFIG_FLASH_BASE_ADDRESS + i) == BINDESC_MAGIC) {
			printk("Found descriptors at 0x%x\n", i);
			bindesc_offset = i;
			break;
		}
	}
	if (i == UINT16_MAX) {
		printk("Descriptors not found\n");
		return 1;
	}

	printk("\n##################################\n");
	printk("Reading using memory mapped flash:\n");
	printk("##################################\n");

	bindesc_open_memory_mapped_flash(&handle, bindesc_offset);
	bindesc_foreach(&handle, dump_descriptors_callback, NULL);

	bindesc_find_str(&handle, BINDESC_ID_KERNEL_VERSION_STRING, &version);
	printk("Zephyr version: %s\n", version);

	bindesc_get_size(&handle, &size);
	printk("Bindesc size: %u\n", size);

	printk("\n##################\n");
	printk("Reading using RAM:\n");
	printk("##################\n");

	flash_read(flash, bindesc_offset, buffer, sizeof(buffer));

	bindesc_open_ram(&handle, buffer, sizeof(buffer));

	/* Search for a non-existent descriptor */
	retval = bindesc_find_str(&handle, 123, &version);
	if (retval) {
		printk("Descriptor not found!\n");
	}

	bindesc_find_uint(&handle, BINDESC_ID_KERNEL_VERSION_MAJOR, &version_number);
	printk("Zephyr version number: %u\n", *version_number);

	printk("\n####################\n");
	printk("Reading using flash:\n");
	printk("####################\n");

	bindesc_open_flash(&handle, bindesc_offset, flash);

	bindesc_find_bytes(&handle, 3, &bytes, &size);
	printk("my_bytes: ");
	dump_bytes(bytes, size);

	return 0;
}
