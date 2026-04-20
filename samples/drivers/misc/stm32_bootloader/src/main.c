/*
 * Copyright (c) 2026 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 system bootloader sample: using both the bootloader and
 *        flash-adapter APIs side by side.
 *
 * The bootloader node exposes the AN3155 command set through its own
 * typed API (enter / get_info / go / exit / mass_erase / ...). Its child
 * @c main_flash node exposes the target's internal flash as a Zephyr
 * flash device, so programming flows through stream_flash exactly like
 * on any other flash driver.
 *
 * Pick the API that matches your task:
 *   - bl_dev (parent)        → lifecycle, go, mass_erase, raw memory R/W
 *   - main_flash_dev (child) → flash_read / flash_write / flash_erase and
 *                              anything that composes on top: stream_flash,
 *                              flash_img, mcumgr img_mgmt, UpdateHub.
 *
 * The adapter does not manage sessions — call @c stm32_bootloader_enter
 * on the parent before any @c flash_* call and @c stm32_bootloader_exit
 * when you are done.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/misc/stm32_bootloader/stm32_bootloader.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device *const bl_dev = DEVICE_DT_GET(DT_NODELABEL(stm32_target));
static const struct device *const main_flash_dev = DEVICE_DT_GET(DT_NODELABEL(main_flash));

#define TARGET_FLASH_BASE DT_REG_ADDR(DT_NODELABEL(main_flash))
#define TARGET_FLASH_SIZE DT_REG_SIZE(DT_NODELABEL(main_flash))

static uint8_t stream_buf[256];
static uint8_t verify_scratch[256];

/*
 * Minimal Cortex-M firmware: initial SP, reset vector pointing at the
 * instruction two bytes in, and an endless Thumb loop ("b ."). Word-
 * aligned so the AN3155 write-block-size constraint is satisfied.
 */
static const uint8_t test_firmware[] __aligned(4) = {
	0x00, 0x00, 0x02, 0x20, /* Initial SP = 0x20020000 */
	0x09, 0x00, 0x00, 0x08, /* Reset vector = 0x08000009 (Thumb) */
	0xFE, 0xE7, 0xFE, 0xE7, /* b . ; b . */
};

static int verify_cb(uint8_t *buf, size_t len, size_t offset)
{
	int ret = flash_read(main_flash_dev, offset, verify_scratch, len);

	if (ret < 0) {
		LOG_ERR("Verify read @0x%zx: %d", offset, ret);
		return ret;
	}
	if (memcmp(buf, verify_scratch, len) != 0) {
		LOG_ERR("Verify mismatch @0x%zx", offset);
		return -EIO;
	}
	LOG_INF("Verified %zu bytes @0x%zx", len, offset);
	return 0;
}

int main(void)
{
	struct stm32_bootloader_info info;
	struct stream_flash_ctx ctx;
	int ret;

	if (!device_is_ready(bl_dev) || !device_is_ready(main_flash_dev)) {
		LOG_ERR("Devices not ready");
		return -ENODEV;
	}

	LOG_INF("=== STM32 bootloader sample ===");

	/* Parent API: lifecycle. */
	ret = stm32_bootloader_enter(bl_dev);
	if (ret < 0) {
		LOG_ERR("enter: %d", ret);
		return ret;
	}

	if (stm32_bootloader_get_info(bl_dev, &info) == 0) {
		LOG_INF("Target: PID=0x%04x, BL v%d.%d, %s erase", info.pid, info.bl_version >> 4,
			info.bl_version & 0x0F, info.has_extended_erase ? "extended" : "standard");
	}

	/*
	 * Parent API: a single mass-erase command is much faster than
	 * erasing every page individually via the flash adapter. The
	 * flash adapter deliberately never issues mass-erase on its own.
	 */
	LOG_INF("Mass-erasing target...");
	ret = stm32_bootloader_mass_erase(bl_dev);
	if (ret < 0) {
		LOG_ERR("mass_erase: %d", ret);
		goto leave;
	}

	/*
	 * Flash adapter: stream the firmware through stream_flash with a
	 * post-write verify callback. No bootloader-specific code below
	 * this point — this is the same code you would write for any
	 * Zephyr flash device.
	 */
	ret = stream_flash_init(&ctx, main_flash_dev, stream_buf, sizeof(stream_buf), 0,
				TARGET_FLASH_SIZE, verify_cb);
	if (ret < 0) {
		LOG_ERR("stream_flash_init: %d", ret);
		goto leave;
	}

	LOG_INF("Streaming %zu bytes of firmware...", sizeof(test_firmware));
	ret = stream_flash_buffered_write(&ctx, test_firmware, sizeof(test_firmware), true);
	if (ret < 0) {
		LOG_ERR("stream_flash_buffered_write: %d", ret);
		goto leave;
	}

	/* Parent API: jump into the freshly written firmware. */
	LOG_INF("Jumping to 0x%08x...", (unsigned int)TARGET_FLASH_BASE);
	ret = stm32_bootloader_go(bl_dev, TARGET_FLASH_BASE);
	if (ret < 0) {
		LOG_ERR("go: %d", ret);
	}

leave:
	(void)stm32_bootloader_exit(bl_dev, ret < 0);
	LOG_INF("=== Done ===");
	return ret;
}
