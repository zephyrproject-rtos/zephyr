/*
 * Copyright (c) 2026 jeck chen <baidxi404629@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief USB Fastboot sample application
 *
 * Demonstrates:
 * - USB descriptor configuration via USBD_*_DEFINE macros
 * - Partition table registration via fastboot_register_partitions()
 * - Flash operation registration via fastboot_register_ops()
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/usb/class/usbd_fastboot.h>

/*
 * USB Descriptors
 */

USBD_DEVICE_DEFINE(fastboot_sample_config_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_SAMPLE_USBD_VID, CONFIG_SAMPLE_USBD_PID);

USBD_DESC_LANG_DEFINE(fastboot_sample_config_lang);
USBD_DESC_MANUFACTURER_DEFINE(fastboot_sample_config_mfr, CONFIG_SAMPLE_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(fastboot_sample_config_product, CONFIG_SAMPLE_USBD_PRODUCT);
USBD_DESC_SERIAL_NUMBER_DEFINE(fastboot_sample_config_sn);

USBD_DESC_CONFIG_DEFINE(fastboot_sample_config_fs_cfg_str, "Composite FS");
USBD_DESC_CONFIG_DEFINE(fastboot_sample_config_hs_cfg_str, "Composite HS");

USBD_CONFIGURATION_DEFINE(fastboot_sample_config_fs_config,
			  IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &fastboot_sample_config_fs_cfg_str);

USBD_CONFIGURATION_DEFINE(fastboot_sample_config_hs_config,
			  IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &fastboot_sample_config_hs_cfg_str);

const struct usbd_fastboot_config usbd_fastboot_cfg = {
	.ctx = &fastboot_sample_config_ctx,
	.lang = &fastboot_sample_config_lang,
	.mfr = &fastboot_sample_config_mfr,
	.product = &fastboot_sample_config_product,
	.sn = &fastboot_sample_config_sn,
	.fs_cfg_str = &fastboot_sample_config_fs_cfg_str,
	.hs_cfg_str = &fastboot_sample_config_hs_cfg_str,
	.fs_config = &fastboot_sample_config_fs_config,
	.hs_config = &fastboot_sample_config_hs_config,
};

/*
 * Partition Table
 *
 * Each partition is defined as a DTS fixed-partition node with a
 * label matching the first argument to FASTBOOT_PART_DEFINE.
 * The macro automatically extracts name (label property), start
 * address (reg), and size (reg) from the device tree.
 *
 * You must provide a DTS overlay (boards/<board>.overlay) that
 * defines the required partition nodes, e.g.:
 *
 *   &flash0 {
 *       partitions {
 *           boot: partition@0 {
 *               label = "boot";
 *               reg = <0x00000000 DT_SIZE_K(128)>;
 *           };
 *           app: partition@20000 {
 *               label = "app";
 *               reg = <0x00020000 DT_SIZE_K(512)>;
 *           };
 *       };
 *   };
 *
 * The node label (before the colon) MUST match the first argument
 * passed to FASTBOOT_PART_DEFINE.
 */

static const struct fastboot_part sample_parts[] = {
	FASTBOOT_PART_DEFINE(boot, 0x54504C53, true),
	FASTBOOT_PART_DEFINE(app, 0x50504100, true),
};

/*
 * Flash Operations (stub implementations)
 *
 * Replace these with real flash driver calls in your application.
 */

static int sample_flash_erase(uint32_t addr, uint32_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	printk("fastboot: erase addr=0x%08x size=%u\n", addr, size);
	return 0;
}

static int sample_flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	printk("fastboot: write addr=0x%08x size=%u\n", addr, size);
	return 0;
}

static int sample_firmware_verify(const uint8_t *data, uint32_t size, uint32_t expected_magic)
{
	/* Minimal check: firmware must be at least 64 bytes */
	if (size < 64) {
		return -EINVAL;
	}
	ARG_UNUSED(data);
	ARG_UNUSED(expected_magic);
	printk("fastboot: verify size=%u magic=0x%08x\n", size, expected_magic);
	return 0;
}

static struct fastboot_ops sample_ops = {
	.flash_erase = sample_flash_erase,
	.flash_write = sample_flash_write,
	.firmware_verify = sample_firmware_verify,
	/* .get_partition is provided by fastboot_register_partitions() */
	/* .on_flash_done is optional, can be NULL */
};

/*
 * Registration (runs before usbd_fastbootd via SYS_INIT)
 */

static int sample_fastboot_init(void)
{
	int ret;

	ret = fastboot_register_partitions(sample_parts, ARRAY_SIZE(sample_parts));
	if (ret < 0) {
		printk("Failed to register partitions: %d\n", ret);
		return ret;
	}

	ret = fastboot_register_ops(&sample_ops);
	if (ret < 0) {
		printk("Failed to register ops: %d\n", ret);
		return ret;
	}

	printk("Fastboot partitions and ops registered\n");
	return 0;
}

SYS_INIT(sample_fastboot_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int main(void)
{
	printk("Fastboot sample started\n");

	/*
	 * usbd_fastbootd_init() runs automatically via SYS_INIT.
	 * It calls usbd_fastboot_config_get() which returns
	 * &usbd_fastboot_cfg defined above.
	 *
	 * sample_fastboot_init() also runs via SYS_INIT (same priority)
	 * to register partitions and ops before USB is initialized.
	 */
	while (1) {
		k_sleep(K_SECONDS(1));
	}
}
