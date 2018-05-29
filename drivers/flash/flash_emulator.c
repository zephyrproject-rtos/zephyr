/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <flash.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <string.h>

#define EXPAND(addr) ((addr)-CONFIG_FLASH_EMULATOR_BASE_ADDR)

#define FLASH_SIZE                                                             \
	(CONFIG_FLASH_EMULATOR_FLASH_SIZE * CONFIG_FLASH_EMULATOR_ERASE_UNIT)

static u8_t mock_flash[FLASH_SIZE];
static bool write_protection;

static int toss(int failure_rate)
{
	return (sys_rand32_get() % 100) < failure_rate ? -EIO : 0;
}

static int flash_range_is_valid(struct device *dev, off_t offset, size_t len)
{
	ARG_UNUSED(dev);
	if (offset + len > FLASH_SIZE + CONFIG_FLASH_EMULATOR_BASE_ADDR) {
		return -EINVAL;
	}
	return 1;
}

static int flash_wp_set(struct device *dev, bool enable)
{
	ARG_UNUSED(dev);
	write_protection = enable;
	return 0;
}

static bool flash_wp_is_set(void) { return write_protection; }

static int flash_eread(struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	int rc;
#if !CONFIG_FLASH_EMULATOR_EMULATE_FAILURES
	rc = 0;
#else
	rc = toss(CONFIG_FLASH_EMULATOR_READ_API_FAILURE_RATE);
#endif

	if (!rc) {
		memcpy(data, mock_flash + EXPAND(offset), len);
#if CONFIG_FLASH_EMULATOR_EMULATE_FAILURES
		/* randomly toggle a byte in the output buffer */
		int hw_error = toss(CONFIG_FLASH_EMULATOR_READ_HW_FAILURE_RATE);
		if (hw_error) {
			*((u8_t *)data + (sys_rand32_get() % len)) ^= 1;
		}
#endif
	}

#if CONFIG_FLASH_EMULATOR_EMULATE_TIMING
	k_busy_wait(CONFIG_FLASH_EMULATOR_MIN_READ_TIME_US);
#endif

	return rc;
}

static int flash_eerase(struct device *dev, off_t offset, size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if (flash_wp_is_set()) {
		return -EACCES;
	}

	/* erase operation must be aligned to the erase unit boundary */
	if ((offset % CONFIG_FLASH_EMULATOR_ERASE_UNIT)
	    || (len % CONFIG_FLASH_EMULATOR_ERASE_UNIT)) {
		return -EINVAL;
	}

	int rc;
#if !CONFIG_FLASH_EMULATOR_EMULATE_FAILURES
	rc = 0;
#else
	rc = toss(CONFIG_FLASH_EMULATOR_ERASE_API_FAILURE_RATE);
#endif

	if (!rc) {
		memset(mock_flash + EXPAND(offset), 0xFF, len);
#if CONFIG_FLASH_EMULATOR_EMULATE_FAILURES
		int hw_error =
		    toss(CONFIG_FLASH_EMULATOR_ERASE_HW_FAILURE_RATE);
		if (hw_error) {
			*((u8_t *)mock_flash + EXPAND(offset)
			  + (sys_rand32_get() % len)) = 0;
		}
#endif
	}

#if CONFIG_FLASH_EMULATOR_EMULATE_TIMING
	k_busy_wait(CONFIG_FLASH_EMULATOR_MIN_ERASE_TIME_US);
#endif

	return rc;
}

static int flash_ewrite(struct device *dev, off_t offset, const void *data,
			size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if (flash_wp_is_set()) {
		return -EACCES;
	}

	int rc;
#if !CONFIG_FLASH_EMULATOR_EMULATE_FAILURES
	rc = 0;
#else
	rc = toss(CONFIG_FLASH_EMULATOR_WRITE_API_FAILURE_RATE);
#endif

	if (!rc) {
		for (u32_t i = 0; i < len; i++) {
			*(mock_flash + EXPAND(offset) + i) &=
			    *((u8_t *)data + i);
		}
#if CONFIG_FLASH_EMULATOR_EMULATE_FAILURES
		int hw_error =
		    toss(CONFIG_FLASH_EMULATOR_WRITE_HW_FAILURE_RATE);
		if (hw_error) {
			*((u8_t *)mock_flash + EXPAND(offset)
			  + (sys_rand32_get() % len)) ^= 1;
		}
#endif
	}

#if CONFIG_FLASH_EMULATOR_EMULATE_TIMING
	k_busy_wait(CONFIG_FLASH_EMULATOR_MIN_WRITE_TIME_US);
#endif

	return rc;
}

#if 0
static struct flash_emu_priv flash_data = {
	char dummy;
};
#endif

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_emu_pages_layout = {
    .pages_count = CONFIG_FLASH_EMULATOR_FLASH_SIZE,
    .pages_size = CONFIG_FLASH_EMULATOR_ERASE_UNIT,
};

void flash_emu_page_layout(struct device *dev,
			   const struct flash_pages_layout **layout,
			   size_t *layout_size)
{
	*layout = &flash_emu_pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api flash_emulator_api = {
    .read = flash_eread,
    .write = flash_ewrite,
    .erase = flash_eerase,
    .write_protection = flash_wp_set,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
    .page_layout = flash_emu_page_layout,
#endif
    .write_block_size = CONFIG_FLASH_EMULATOR_WRITE_UNIT,
};

static int flash_init(struct device *dev)
{
	memset(mock_flash, 0x00, ARRAY_SIZE(mock_flash));
	return 0;
}

DEVICE_AND_API_INIT(flash_emulator, "FLASH_EMULATOR", flash_init,
		    NULL, //&flash_data,
		    NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &flash_emulator_api);
