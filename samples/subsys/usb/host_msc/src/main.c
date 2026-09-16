/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>

LOG_MODULE_REGISTER(usb_host_msc_sample, LOG_LEVEL_INF);

#if !IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_QUIET)
#define SAMPLE_TRACE(...) LOG_INF(__VA_ARGS__)
#else
#define SAMPLE_TRACE(...)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(snps_dwc3)
#include <errno.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh_hcd.h>
#include <zephyr/usb/usbh_msc.h>
#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
#include <zephyr/usb/usb_msc_disk.h>
#endif
#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FS_SHELL)
#include "msc_multi_mount.h"
#endif
#include "usb_host_dump.h"

static K_SEM_DEFINE(usbh_cfg_done_sem, 0, CONFIG_USBH_USB_DEVICE_MAX);
static uint32_t usbh_connect_gen;

void usbh_device_configured_notify(struct usb_device *udev)
{
	ARG_UNUSED(udev);

	usbh_connect_gen++;
	k_sem_give(&usbh_cfg_done_sem);
}

void usbh_device_removed_notify(struct usb_device *udev)
{
#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FS_SHELL)
	msc_fs_unmount_udev(udev);
#endif

#if IS_ENABLED(CONFIG_USBH_MSC_DISK) && !IS_ENABLED(CONFIG_USBH_MSC_AUTO_BRINGUP)
	usb_msc_disk_device_removed(udev);
#endif

	usbh_connect_gen++;

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FS_SHELL)
	LOG_INF("USB device removed — replug MSC device(s), then msc mount USB (or USB1 …)");
#elif IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_FILE_DEMO)
	LOG_INF("USB device removed — replug MSC device(s), then fs mount fat /%s:",
		CONFIG_USBH_MSC_DISK_NAME);
	LOG_INF("  (or /USB1: …)");
#else
	LOG_INF("USB device removed — replug MSC device(s) to enumerate again");
#endif
}

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static int usb_host_bringup(void)
{
	int err;

	err = usbh_init(&uhs_ctx);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("usbh_init failed: %d", err);
		return err;
	}

	err = usbh_enable(&uhs_ctx);
	if (err != 0) {
		LOG_ERR("usbh_enable failed: %d", err);
		return err;
	}

	err = uhc_sof_enable(uhs_ctx.dev);
	if (err != 0) {
		LOG_ERR("uhc_sof_enable failed: %d", err);
		return err;
	}

	return 0;
}

static void usb_host_msc_log_ready_volumes(struct usb_device *udev)
{
#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
	char disk_name[16];

	for (uint8_t lun = 0U; lun < CONFIG_USBH_MSC_LUN_SLOTS; lun++) {
		if (usb_msc_disk_get_volume_name(udev, lun, disk_name, sizeof(disk_name)) >= 0) {
#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FS_SHELL)
			LOG_INF("USB MSC ready - msc mount %s  (then fs ls /%s:)", disk_name,
				disk_name);
#else
			LOG_INF("USB MSC ready - fs mount fat /%s:", disk_name);
#endif
		}
	}
#else
	ARG_UNUSED(udev);
#endif
}

static bool usb_host_msc_needs_bringup(struct usb_device *udev)
{
#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
	char disk_name[16];
#endif

	if (udev == NULL || udev->addr == 0U || udev->state != USB_STATE_CONFIGURED) {
		return false;
	}

	if (!usbh_device_still_connected(udev)) {
		return false;
	}

	if (!usbh_msc_device_has_storage(udev)) {
		return false;
	}

#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
	if (usb_msc_disk_get_volume_name(udev, 0, disk_name, sizeof(disk_name)) >= 0) {
		return false;
	}
#endif

	return true;
}

static void usb_host_msc_try_bringup(struct usb_device *udev)
{
	int err;
	uint32_t cfg_gen = usbh_connect_gen;

	SAMPLE_TRACE("main: CONFIGURED addr=%u", udev->addr);

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FS_SHELL)
	msc_fs_unmount_udev(udev);
#endif

#if !IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_QUIET)
	LOG_INF("USB device: VID=0x%04x PID=0x%04x configs=%u", udev->dev_desc.idVendor,
		udev->dev_desc.idProduct, udev->dev_desc.bNumConfigurations);

	usb_host_dump_cfg_interfaces(udev);
	usb_host_try_print_msc_capacity(udev);
#endif

#if !IS_ENABLED(CONFIG_USBH_MSC_AUTO_BRINGUP)
	for (int steady = 0; steady < 50 && !usbh_post_configure_steady(udev); steady++) {
		k_msleep(10);
	}

	err = usb_host_msc_storage_bringup(udev, uhs_ctx.dev);
	if ((err == -ETIMEDOUT || err == -EALREADY) && usbh_device_still_connected(udev)) {
		LOG_WRN("main: MSC bringup err=%d — retry after detach/settle", err);
		usb_msc_disk_device_removed(udev);
		k_msleep(200);
		err = usb_host_msc_storage_bringup(udev, uhs_ctx.dev);
	}
#else
	err = usbh_device_still_connected(udev) ? 0 : -ENODEV;
#endif
	if (err == 0 && (cfg_gen != usbh_connect_gen || !usbh_device_still_connected(udev))) {
		LOG_WRN("main: device disconnected during MSC bringup");
		err = -ENODEV;
	}
	if (err == 0) {
#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_FILE_DEMO) &&                                        \
	IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_AUTO_DEMO)
		int fat_err = usb_host_msc_sample_fat_file_demo();

		if (fat_err == 0) {
			LOG_INF("main: simple FS ok (FS_VALIDATE passed)");
		} else {
			LOG_WRN("main: simple FS failed (err=%d)", fat_err);
		}
#elif IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_FILE_DEMO) && IS_ENABLED(CONFIG_FILE_SYSTEM)
		usb_host_msc_log_ready_volumes(udev);
#elif IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_WRITE)
		int large_err = usb_host_msc_sample_write_large_text_file();

		if (large_err == 0) {
			LOG_INF("main: large file write ok");
		} else {
			LOG_WRN("main: large file write failed (err=%d)", large_err);
		}
		usb_host_msc_log_ready_volumes(udev);
#elif IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FS_SHELL) && IS_ENABLED(CONFIG_USBH_MSC_DISK)
		usb_host_msc_log_ready_volumes(udev);
#elif IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_FILE_DEMO)
		LOG_INF("main: FatFs disk bound (auto demo disabled)");
#endif
	} else {
		LOG_WRN("main: MSC storage bringup failed (err=%d)", err);
	}
}

static void usb_host_msc_scan_all(void)
{
	struct usb_device *udev;

	SYS_DLIST_FOR_EACH_CONTAINER(&uhs_ctx.udevs, udev, node) {
		if (!usb_host_msc_needs_bringup(udev)) {
			if (udev->state == USB_STATE_CONFIGURED && udev->addr != 0U &&
			    !usbh_msc_device_has_storage(udev)) {
				SAMPLE_TRACE("main: addr=%u has no MSC interface — skip",
					     udev->addr);
			}
			continue;
		}

		usb_host_msc_try_bringup(udev);
	}
}

static void run_usb_host_enumeration(void)
{
	if (usb_host_bringup() != 0) {
		return;
	}

	SAMPLE_TRACE("USB host ready — plug a device; MSC runs after CONFIGURED");

	for (;;) {
		(void)k_sem_take(&usbh_cfg_done_sem, K_FOREVER);

		while (k_sem_take(&usbh_cfg_done_sem, K_NO_WAIT) == 0) {
			/* coalesce concurrent configure notifications */
		}

		usb_host_msc_scan_all();
	}
}
#endif /* snps_dwc3 */

int main(void)
{
	printf("USB host MSC sample: %s\n", CONFIG_BOARD_TARGET);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(k_current_get(), "main");
	}

#if DT_HAS_COMPAT_STATUS_OKAY(snps_dwc3)
	run_usb_host_enumeration();
#else
	LOG_INF("No snps,dwc3 in devicetree — add a usb_host.overlay. USB skipped");
#endif

	return 0;
}
