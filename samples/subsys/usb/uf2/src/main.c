/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <cmsis_core.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/storage/ffatdisk.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_msc.h>

#include "uf2.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifdef BUILD_VERSION
#define BANNER_VERSION STRINGIFY(BUILD_VERSION)
#else
#define BANNER_VERSION KERNEL_VERSION_STRING
#endif

#define SLOT0_NODE		DT_NODELABEL(slot0_partition)
#define SLOT0_ID		PARTITION_ID(slot0_partition)
#define SLOT0_SIZE		DT_REG_SIZE(SLOT0_NODE)
#define SLOT0_ADDR		PARTITION_ADDRESS(slot0_partition)

/* CURRENT.UF2 is twice the slot size */
#define CURRENT_UF2_SIZE	((SLOT0_SIZE / UF2_CURRENT_PAYLOAD) * UF2_BLOCK_SIZE)

/* Limits the size of the received blocks bitmap */
#define UF2_MIN_PAYLOAD		64U
#define UF2_MAX_BLOCKS		(SLOT0_SIZE / UF2_MIN_PAYLOAD)
#define UF2_MAX_PAGES		1024U

#define SRAM_NODE		DT_CHOSEN(zephyr_sram)
#define SRAM_START		DT_REG_ADDR(SRAM_NODE)
#define SRAM_END		(SRAM_START + DT_REG_SIZE(SRAM_NODE))

BUILD_ASSERT(IS_ENABLED(CONFIG_CPU_CORTEX_M), "Only Arm Cortex-M is supported");

#define UF2_LED_ERROR_PERIOD	K_MSEC(100)

/* Time for the host to finish file system updates */
#define UF2_RESTART_DELAY	K_MSEC(1000)

/* Forget a transfer after this time without blocks */
#define UF2_IDLE_TIMEOUT	K_SECONDS(5)

static const char info_file[] =
	"UF2 Bootloader Zephyr " BANNER_VERSION "\n"
	"Model: " CONFIG_BOARD "\n"
	"Board-ID: " CONFIG_SOC "-" CONFIG_BOARD "\n"
	"Target: slot0_partition\n";

static const char index_file[] =
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
	"<meta http-equiv=\"refresh\" content=\"0;URL='https://www.zephyrproject.org/'\"/>\n"
	"</head>\n"
	"<body>\n"
	"<a href=\"https://www.zephyrproject.org/\">zephyrproject.org</a>\n"
	"</body>\n"
	"</html>\n";

static struct uf2_ctx {
	const struct flash_area *fa;
	const struct device *dev;
	struct k_work_delayable restart_work;
	struct k_work_delayable led_work;
	bool led_error;
	ATOMIC_DEFINE(rx_map, UF2_MAX_BLOCKS);
	ATOMIC_DEFINE(erased, UF2_MAX_PAGES);
	uint32_t first_page;
	uint32_t num_blocks;
	uint32_t received;
	k_timepoint_t idle;
} uf2_ctx;

static struct usbd_context *sample_usbd;

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* LED is on when idle, toggles on a block, blinks on an error, off when done */
static void uf2_led_work_handler(struct k_work *const work)
{
	if (uf2_ctx.led_error) {
		gpio_pin_toggle_dt(&led);
		k_work_reschedule(&uf2_ctx.led_work, UF2_LED_ERROR_PERIOD);
	}
}

static void uf2_led_error(struct uf2_ctx *const ctx)
{
	if (!ctx->led_error) {
		ctx->led_error = true;
		k_work_reschedule(&ctx->led_work, K_NO_WAIT);
	}
}

static void uf2_led_activity(struct uf2_ctx *const ctx)
{
	ctx->led_error = false;
	gpio_pin_toggle_dt(&led);
}

static void uf2_led_done(struct uf2_ctx *const ctx)
{
	ctx->led_error = false;
	gpio_pin_set_dt(&led, 0);
}

static int uf2_led_init(void)
{
	if (!gpio_is_ready_dt(&led) ||
	    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) != 0) {
		return -ENODEV;
	}

	return 0;
}

static void sample_msg_cb(struct usbd_context *const ctx,
			  const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx) != 0) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(ctx) != 0) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}
}

static void uf2_restart_handler(struct k_work *const work)
{
	int err;

	/* Detach so that the host sees the disk removed */
	err = usbd_disable(sample_usbd);
	if (err != 0) {
		LOG_ERR("Failed to disable device support (%d)", err);
	}

	err = usbd_shutdown(sample_usbd);
	if (err != 0) {
		LOG_ERR("Failed to shut down device support (%d)", err);
	}

	LOG_INF("Restarting");
	sys_reboot(SYS_REBOOT_COLD);
}

static void uf2_reset_transfer(struct uf2_ctx *const ctx, const uint32_t num_blocks)
{
	k_work_cancel_delayable(&ctx->restart_work);
	memset(ctx->rx_map, 0, sizeof(ctx->rx_map));
	memset(ctx->erased, 0, sizeof(ctx->erased));
	ctx->num_blocks = num_blocks;
	ctx->received = 0U;
}

static bool button_pressed(void)
{
	if (!gpio_is_ready_dt(&button) ||
	    gpio_pin_configure_dt(&button, GPIO_INPUT) != 0) {
		return false;
	}

	return gpio_pin_get_dt(&button) > 0;
}

static bool app_valid(const struct uf2_ctx *const ctx, uint32_t *const vt)
{
	if (flash_area_read(ctx->fa, 0, vt, 2 * sizeof(vt[0])) != 0) {
		return false;
	}

	/* Initial stack pointer must be in RAM */
	if (vt[0] <= SRAM_START || vt[0] > SRAM_END ||
	    (vt[0] % sizeof(uint32_t)) != 0U) {
		return false;
	}

	/* Reset vector must have the Thumb bit and point into the slot */
	return (vt[1] & 1U) != 0U && (vt[1] & ~1U) >= SLOT0_ADDR &&
	       (vt[1] & ~1U) < SLOT0_ADDR + SLOT0_SIZE;
}

/* Unfortunately there is no platform agnostic way to turn the MPU off */
#if defined(CONFIG_CPU_HAS_ARM_MPU)
static void app_mpu_clear(void)
{
	const uint32_t regions = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;

	ARM_MPU_Disable();

	for (uint32_t i = 0U; i < regions; i++) {
		ARM_MPU_ClrRegion(i);
	}
}
#elif defined(CONFIG_CPU_HAS_NXP_SYSMPU)
#include <fsl_sysmpu.h>

static void app_mpu_clear(void)
{
	SYSMPU_Enable(SYSMPU, false);

	/* Region 0 is reserved for the debugger */
	for (uint32_t i = 1U; i < FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT; i++) {
		SYSMPU_RegionEnable(SYSMPU, i, false);
	}
}
#else
static void app_mpu_clear(void)
{
}
#endif

static FUNC_NORETURN void app_boot(const uint32_t *const vt)
{
	unsigned int i;

	if (IS_ENABLED(CONFIG_SYSTEM_TIMER_HAS_DISABLE_SUPPORT)) {
		sys_clock_disable();
	}

	/* Let pending exceptions run */
	__ISB();
	__disable_irq();

	for (i = 0; i < ARRAY_SIZE(NVIC->ICER); i++) {
		NVIC->ICER[i] = 0xFFFFFFFFU;
		NVIC->ICPR[i] = 0xFFFFFFFFU;
	}

	/* Application sets up its own regions and limits */
	app_mpu_clear();

#if defined(CONFIG_BUILTIN_STACK_GUARD) && defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM(0);
	__set_MSPLIM(0);
#endif

	SCB->VTOR = SLOT0_ADDR;

	/* Run on the main stack, as after reset */
	__asm__ volatile("msr control, %[ctl]\n\t"
			 "isb\n\t"
			 "msr msp, %[sp]\n\t"
			 "bx %[rst]\n\t"
			 :: [ctl] "r" (0), [sp] "r" (vt[0]), [rst] "r" (vt[1]));

	CODE_UNREACHABLE;
}

static int uf2_init_flash(struct uf2_ctx *const ctx)
{
	struct flash_pages_info info;
	int err;

	err = flash_area_open(SLOT0_ID, &ctx->fa);
	if (err != 0) {
		return err;
	}

	ctx->dev = flash_area_get_device(ctx->fa);

	err = flash_get_page_info_by_offs(ctx->dev, ctx->fa->fa_off, &info);
	if (err != 0) {
		return err;
	}

	ctx->first_page = info.index;

	err = flash_get_page_info_by_offs(ctx->dev,
					  ctx->fa->fa_off + ctx->fa->fa_size - 1,
					  &info);
	if (err != 0) {
		return err;
	}

	if (info.index - ctx->first_page >= UF2_MAX_PAGES) {
		return -ENOSPC;
	}

	return 0;
}

/* Blocks arrive in any order, erase pages on first use */
static int uf2_erase(struct uf2_ctx *const ctx, const uint32_t off, const size_t len)
{
	struct flash_pages_info info;
	uint32_t pos = off;
	size_t remaining = len;
	uint32_t page_off;
	uint32_t idx;
	int err;

	while (remaining > 0U) {
		size_t step;

		err = flash_get_page_info_by_offs(ctx->dev,
						  ctx->fa->fa_off + pos, &info);
		if (err != 0) {
			return err;
		}

		idx = info.index - ctx->first_page;
		page_off = info.start_offset - ctx->fa->fa_off;
		if (!atomic_test_and_set_bit(ctx->erased, idx)) {
			err = flash_area_flatten(ctx->fa, page_off, info.size);
			if (err != 0) {
				atomic_clear_bit(ctx->erased, idx);
				return err;
			}
		}

		step = page_off + info.size - pos;
		step = MIN(step, remaining);
		pos += step;
		remaining -= step;
	}

	return 0;
}

static bool uf2_block_valid(const struct uf2_ctx *const ctx,
			    const struct uf2_block *const blk)
{
	const uint32_t payload = sys_le32_to_cpu(blk->payload_size);
	const uint32_t addr = sys_le32_to_cpu(blk->target_addr);
	const uint32_t num_blocks = sys_le32_to_cpu(blk->num_blocks);
	const uint32_t block_no = sys_le32_to_cpu(blk->block_no);

	if (payload < UF2_MIN_PAYLOAD || payload > UF2_DATA_SIZE ||
	    (payload % flash_area_align(ctx->fa)) != 0U) {
		return false;
	}

	if (num_blocks == 0U || num_blocks > UF2_MAX_BLOCKS ||
	    block_no >= num_blocks) {
		return false;
	}

	if (addr % flash_area_align(ctx->fa) != 0U) {
		return false;
	}

	return true;
}

static bool uf2_block_in_slot(const struct uf2_block *const blk)
{
	const uint32_t payload = sys_le32_to_cpu(blk->payload_size);
	const uint32_t addr = sys_le32_to_cpu(blk->target_addr);

	return addr >= SLOT0_ADDR && addr - SLOT0_ADDR <= SLOT0_SIZE - payload;
}

static int uf2_write_block(struct uf2_ctx *const ctx,
			   const struct uf2_block *const blk)
{
	const uint32_t flags = sys_le32_to_cpu(blk->flags);
	const uint32_t payload = sys_le32_to_cpu(blk->payload_size);
	const uint32_t num_blocks = sys_le32_to_cpu(blk->num_blocks);
	const uint32_t block_no = sys_le32_to_cpu(blk->block_no);
	const uint32_t off = sys_le32_to_cpu(blk->target_addr) - SLOT0_ADDR;
	int err;

	if (blk->magic_start0 != sys_cpu_to_le32(UF2_MAGIC_START0) ||
	    blk->magic_start1 != sys_cpu_to_le32(UF2_MAGIC_START1) ||
	    blk->magic_end != sys_cpu_to_le32(UF2_MAGIC_END)) {
		/* Not a UF2 block */
		return -ENODATA;
	}

	/* Disregard other families as required by UF2 */
	if ((flags & UF2_FLAG_FAMILY_ID) == 0U ||
	    blk->family_id != sys_cpu_to_le32(UF2_FAMILY_ID)) {
		return -ENODATA;
	}

	if ((flags & (UF2_FLAG_NOFLASH | UF2_FLAG_FILECONTAINER)) != 0U) {
		return 0;
	}

	if (!uf2_block_valid(ctx, blk)) {
		LOG_WRN("Invalid block %u of %u", block_no, num_blocks);
		uf2_led_error(ctx);
		return -EINVAL;
	}

	if (!uf2_block_in_slot(blk)) {
		/* Image linked for another address */
		LOG_WRN("Ignoring block %u of %u at 0x%08x outside of the slot",
			block_no, num_blocks, sys_le32_to_cpu(blk->target_addr));
		uf2_led_error(ctx);
		return 0;
	}

	if (num_blocks != ctx->num_blocks ||
	    sys_timepoint_expired(ctx->idle)) {
		LOG_INF("New image, %u blocks", num_blocks);
		uf2_reset_transfer(ctx, num_blocks);
	}

	ctx->idle = sys_timepoint_calc(UF2_IDLE_TIMEOUT);

	if (atomic_test_and_set_bit(ctx->rx_map, block_no)) {
		/* Host may write a block more than once */
		return 0;
	}

	err = uf2_erase(ctx, off, payload);
	if (err == 0) {
		err = flash_area_write(ctx->fa, off, blk->data, payload);
	}

	if (err != 0) {
		LOG_ERR("Block %u at offset 0x%08x failed (%d)",
			block_no, off, err);
		atomic_clear_bit(ctx->rx_map, block_no);
		uf2_led_error(ctx);
		return err;
	}

	uf2_led_activity(ctx);

	if (++ctx->received == ctx->num_blocks) {
		LOG_INF("Image received");
		uf2_led_done(ctx);

		if (IS_ENABLED(CONFIG_APP_UF2_AUTO_REBOOT)) {
			k_work_reschedule(&ctx->restart_work, UF2_RESTART_DELAY);
		}
	}

	return 0;
}

static int infofile_rd_cb(struct ffat_file *const f, const uint32_t block,
			  uint8_t *const buf, const uint32_t size)
{
	const uint32_t f_off = size * block;

	if (f->size > f_off) {
		memcpy(buf, (const uint8_t *)f->priv + f_off,
		       MIN(f->size - f_off, size));
	}

	return 0;
}

/* Present the slot as UF2 blocks */
static int current_rd_cb(struct ffat_file *const f, const uint32_t block,
			 uint8_t *const buf, const uint32_t size)
{
	struct uf2_block *const blk = (void *)buf;
	const uint32_t off = block * UF2_CURRENT_PAYLOAD;
	int err;

	if (size != UF2_BLOCK_SIZE) {
		return -EINVAL;
	}

	if (off >= SLOT0_SIZE) {
		return 0;
	}

	err = flash_area_read(uf2_ctx.fa, off, blk->data, UF2_CURRENT_PAYLOAD);
	if (err != 0) {
		return err;
	}

	blk->magic_start0 = sys_cpu_to_le32(UF2_MAGIC_START0);
	blk->magic_start1 = sys_cpu_to_le32(UF2_MAGIC_START1);
	blk->flags = sys_cpu_to_le32(UF2_FLAG_FAMILY_ID);
	blk->target_addr = sys_cpu_to_le32(SLOT0_ADDR + off);
	blk->payload_size = sys_cpu_to_le32(UF2_CURRENT_PAYLOAD);
	blk->block_no = sys_cpu_to_le32(block);
	blk->num_blocks = sys_cpu_to_le32(SLOT0_SIZE / UF2_CURRENT_PAYLOAD);
	blk->family_id = sys_cpu_to_le32(UF2_FAMILY_ID);
	blk->magic_end = sys_cpu_to_le32(UF2_MAGIC_END);

	return 0;
}

/* Blocks carry their target address, position in the file is irrelevant */
static int current_wr_cb(struct ffat_file *const f, const uint32_t block,
			 const uint8_t *const buf, const uint32_t size)
{
	if (buf == NULL && block == 0U && size == 0U) {
		/*
		 * The host marked the volume clean, the payload it placed
		 * is complete. This can happen more than once per payload,
		 * so restart only if the image is complete. The delay is
		 * the fallback for a host that never marks it clean.
		 */
		if (IS_ENABLED(CONFIG_APP_UF2_AUTO_REBOOT) &&
		    uf2_ctx.num_blocks != 0U &&
		    uf2_ctx.received == uf2_ctx.num_blocks) {
			k_work_reschedule(&uf2_ctx.restart_work, K_NO_WAIT);
		}

		LOG_INF("Host flushed the volume");
		return 0;
	}

	if (size != UF2_BLOCK_SIZE) {
		return -ENODATA;
	}

	return uf2_write_block(&uf2_ctx, (const void *)buf);
}

FFAT_FILE_DEFINE(info, "UF2", "INFO_UF2TXT", sizeof(info_file),
		 infofile_rd_cb, NULL, info_file);
FFAT_FILE_DEFINE(index_htm, "UF2", "INDEX   HTM", sizeof(index_file),
		 infofile_rd_cb, NULL, index_file);
FFAT_FILE_DEFINE(current, "UF2", "CURRENT UF2", CURRENT_UF2_SIZE,
		 current_rd_cb, current_wr_cb, NULL);

USBD_DEFINE_MSC_LUN(uf2, "UF2", "Zephyr", "UF2 Boot", "0.01");

int main(void)
{
	uint32_t vt[2];
	int err;

	k_work_init_delayable(&uf2_ctx.restart_work, uf2_restart_handler);
	k_work_init_delayable(&uf2_ctx.led_work, uf2_led_work_handler);

	err = uf2_init_flash(&uf2_ctx);
	if (err != 0) {
		LOG_ERR("Failed to initialize the flash slot (%d)", err);
		if (uf2_led_init() == 0) {
			uf2_led_error(&uf2_ctx);
		}

		return err;
	}

	/* Boot the application unless button is held or slot is empty */
	if (!button_pressed() && app_valid(&uf2_ctx, vt)) {
		app_boot(vt);
	}

	if (uf2_led_init() != 0) {
		LOG_ERR("Failed to configure the LED");
		return -ENODEV;
	}

	sample_usbd = sample_usbd_init_device(sample_msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		uf2_led_error(&uf2_ctx);
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		err = usbd_enable(sample_usbd);
		if (err != 0) {
			LOG_ERR("Failed to enable device support");
			uf2_led_error(&uf2_ctx);
			return err;
		}
	}

	LOG_INF("USB UF2 sample is initialized, target 0x%08x size %u",
		(uint32_t)SLOT0_ADDR, (uint32_t)SLOT0_SIZE);

	return 0;
}
