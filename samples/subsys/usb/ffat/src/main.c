/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
FFAT_FILE_DEFINE(bin, "FFAT16", "FOOBAZ  BIN", 32768,
		 binfile_rd_cb, binfile_wr_cb, NULL);

FFAT_FILE_DEFINE(dummy, "FFAT32", "FOOBAZ  BIN", 262144,
		 binfile_rd_cb, binfile_wr_cb, NULL);

USBD_DEFINE_MSC_LUN(FFAT16, "Zephyr", "FFAT16", "0.00");
USBD_DEFINE_MSC_LUN(FFAT32, "Zephyr", "FFAT32", "0.00");

USBD_CONFIGURATION_DEFINE(config_1,
			  USB_SCD_SELF_POWERED,
			  200);

USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_MANUFACTURER_DEFINE(sample_mfr, "ZEPHYR");
USBD_DESC_PRODUCT_DEFINE(sample_product, "Zephyr FFAT MSC");
USBD_DESC_SERIAL_NUMBER_DEFINE(sample_sn, "0123456789ABCDEF");


USBD_DEVICE_DEFINE(sample_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0x0008);
int main(void)
{
	int ret;

	ret = usbd_add_descriptor(&sample_usbd, &sample_lang);
	if (ret) {
		LOG_ERR("Failed to initialize language descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&sample_usbd, &sample_mfr);
	if (ret) {
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&sample_usbd, &sample_product);
	if (ret) {
		LOG_ERR("Failed to initialize product descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&sample_usbd, &sample_sn);
	if (ret) {
		LOG_ERR("Failed to initialize SN descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_configuration(&sample_usbd, &config_1);
	if (ret) {
		LOG_ERR("Failed to add configuration (%d)", ret);
		return ret;
	}

	ret = usbd_register_class(&sample_usbd, "msc_0", 1);
	if (ret) {
		LOG_ERR("Failed to register MSC class (%d)", ret);
		return ret;
	}

	ret = usbd_init(&sample_usbd);
	if (ret) {
		LOG_ERR("Failed to initialize device support");
		return ret;
	}

	ret = usbd_enable(&sample_usbd);
	if (ret) {
		LOG_ERR("Failed to enable device support");
		return ret;
	}

	LOG_INF("USB mass storage is ready.");

	return 0;
}
