/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <stdint.h>
#include <version.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/storage/ffatdisk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifdef BUILD_VERSION
#define BANNER_VERSION STRINGIFY(BUILD_VERSION)
#else
#define BANNER_VERSION KERNEL_VERSION_STRING
#endif

const char txt_info_file[] =
	"Zephyr RTOS\n"
	"Build " BANNER_VERSION "\n"
	"Board " CONFIG_BOARD "\n"
	"Arch "CONFIG_ARCH "\n";

const char html_info_file[] =
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
	"<style>\n"
	"body {\n"
		"background: #AF7FE4;\n"
	"}\n"
	".content {\n"
		"max-width: 480px;\n"
		"background: #F0F2F4;\n"
		"margin: auto;\n"
		"padding: 16px;\n"
	"}\n"
	"</style>\n"
	"</head>\n"
	"<body>\n"
	"<div class=\"content\">\n"
		"<h1>FFAT sample</h1>\n"
		"<p><a href=\"https://www.zephyrproject.org/\">zephyrproject.org</a></p>\n"
	"</div>\n"
	"</body>\n"
	"</html>\n";

struct binfile {
	uint32_t s_tag;
	uint32_t b_num;
	uint8_t reserved[500];
	uint32_t e_tag;
} __packed;

BUILD_ASSERT(sizeof(struct binfile) == 512U);

static int infofile_rd_cb(struct ffat_file *const f, const uint32_t sector,
			  uint8_t *const buf, const uint32_t size)
{
	size_t f_off = size * sector;

	if (f->size > f_off) {
		size_t len = MIN(f->size - f_off, size);

		memcpy(buf, (uint8_t *)f->priv + f_off, len);
		LOG_DBG("Read %u bytes, sector %u file offset %zu, f->size %zu",
			len, sector, f_off, f->size);
	}

	return 0;
}

static int binfile_rd_cb(struct ffat_file *const f, const uint32_t sector,
			 uint8_t *const buf, const uint32_t size)
{
	size_t f_off = size * sector;

	if (f->size > f_off) {
		size_t len = MIN(f->size - f_off, size);
		struct binfile *test = (void *)buf;

		test->s_tag = sys_cpu_to_le32(0xDECAFBAD);
		test->b_num = sys_cpu_to_le32(sector);
		test->e_tag = sys_cpu_to_le32(0xDEADDA7A);

		LOG_DBG("Read %u bytes, sector %u file offset %zu, f->size %zu",
			len, sector, f_off, f->size);
	}

	return 0;
}

static int binfile_wr_cb(struct ffat_file *const f, const uint32_t sector,
			 const uint8_t *const buf, const uint32_t size)
{
	size_t f_off = size * sector;

	if (f->size > f_off) {
		size_t len = MIN(f->size - f_off, size);

		LOG_DBG("Write %u bytes, sector %u file offset %zu, f->size %zu",
			len, sector, f_off, f->size);
	}

	return 0;
}

FFAT_FILE_DEFINE(readme, "FFAT16", "README  TXT", sizeof(txt_info_file),
		 infofile_rd_cb, NULL, txt_info_file);
FFAT_FILE_DEFINE(html, "FFAT16", "INDEX   HTM", sizeof(html_info_file),
		 infofile_rd_cb, NULL, html_info_file);
FFAT_FILE_DEFINE(bin, "FFAT16", "FOOBAZ  BIN", 32768U,
		 binfile_rd_cb, binfile_wr_cb, NULL);

FFAT_FILE_DEFINE(readme32, "FFAT32", "README  TXT", sizeof(txt_info_file),
		 infofile_rd_cb, NULL, txt_info_file);
FFAT_FILE_DEFINE(big, "FFAT32", "FOOBAZ  BIN", 66051072UL,
		 binfile_rd_cb, binfile_wr_cb, NULL);

USBD_DEFINE_MSC_LUN(ffat16, "FFAT16", "Zephyr", "FFAT16", "0.00");
USBD_DEFINE_MSC_LUN(ffat32, "FFAT32", "Zephyr", "FFAT32", "0.00");

int main(void)
{
	struct usbd_context *sample_usbd;
	int ret;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret) {
		LOG_ERR("Failed to enable device support");
		return ret;
	}

	LOG_INF("FFAT sample is ready.");

	return 0;
}
