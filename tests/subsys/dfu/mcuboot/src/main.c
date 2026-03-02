/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/storage/flash_map.h>
#include <bootutil/bootutil_public.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/dfu_boot.h>

#define SLOT0_PARTITION		slot0_partition
#define SLOT1_PARTITION		slot1_partition

#define SLOT0_PARTITION_ID	PARTITION_ID(SLOT0_PARTITION)
#define SLOT1_PARTITION_ID	PARTITION_ID(SLOT1_PARTITION)

#define BOOT_MAGIC_VAL_W0 0xf395c277
#define BOOT_MAGIC_VAL_W1 0x7fefd260
#define BOOT_MAGIC_VAL_W2 0x0f505235
#define BOOT_MAGIC_VAL_W3 0x8079b62c
#define BOOT_MAGIC_VALUES {BOOT_MAGIC_VAL_W0, BOOT_MAGIC_VAL_W1,\
			   BOOT_MAGIC_VAL_W2, BOOT_MAGIC_VAL_W3 }

/* MCUboot image header magic */
#define IMAGE_MAGIC 0x96f3b83d

/* MCUboot image header size */
#define IMAGE_HEADER_SIZE 32

/* MCUboot TLV magic values */
#define IMAGE_TLV_INFO_MAGIC      0x6907
#define IMAGE_TLV_PROT_INFO_MAGIC 0x6908

/* MCUboot TLV types */
#define IMAGE_TLV_SHA256 0x10

/* Test image version */
#define TEST_IMG_VER_MAJOR  1
#define TEST_IMG_VER_MINOR  2
#define TEST_IMG_VER_REV    3
#define TEST_IMG_VER_BUILD  0x12345678

/* Minimal fake MCUboot image header structure for testing */
struct test_image_header {
	uint32_t ih_magic;
	uint32_t ih_load_addr;
	uint16_t ih_hdr_size;
	uint16_t ih_protect_tlv_size;
	uint32_t ih_img_size;
	uint32_t ih_flags;
	struct {
		uint8_t iv_major;
		uint8_t iv_minor;
		uint16_t iv_revision;
		uint32_t iv_build_num;
	} ih_ver;
	uint32_t _pad1;
};

/* TLV info header */
struct test_image_tlv_info {
	uint16_t it_magic;
	uint16_t it_tlv_tot;
};

/* Single TLV entry */
struct test_image_tlv {
	uint8_t it_type;
	uint8_t _pad;
	uint16_t it_len;
};

/* Complete fake image with header, minimal "code", and TLV with SHA256 hash */
struct test_fake_image {
	struct test_image_header header;
	uint8_t fake_code[64];
	struct test_image_tlv_info tlv_info;
	struct test_image_tlv hash_tlv;
	uint8_t hash[32];
};

static void create_fake_mcuboot_image(struct test_fake_image *img)
{
	memset(img, 0xFF, sizeof(*img));

	/* Setup header */
	img->header.ih_magic = IMAGE_MAGIC;
	img->header.ih_load_addr = 0x00000000;
	img->header.ih_hdr_size = IMAGE_HEADER_SIZE;
	img->header.ih_protect_tlv_size = 0;
	img->header.ih_img_size = sizeof(img->fake_code);
	img->header.ih_flags = 0;

	/* Setup version */
	img->header.ih_ver.iv_major = TEST_IMG_VER_MAJOR;
	img->header.ih_ver.iv_minor = TEST_IMG_VER_MINOR;
	img->header.ih_ver.iv_revision = TEST_IMG_VER_REV;
	img->header.ih_ver.iv_build_num = TEST_IMG_VER_BUILD;

	/* Fill fake code with pattern */
	memset(img->fake_code, 0xAA, sizeof(img->fake_code));

	/* Setup TLV info */
	img->tlv_info.it_magic = IMAGE_TLV_INFO_MAGIC;
	img->tlv_info.it_tlv_tot = sizeof(img->hash_tlv) + sizeof(img->hash);

	/* Setup hash TLV */
	img->hash_tlv.it_type = IMAGE_TLV_SHA256;
	img->hash_tlv._pad = 0;
	img->hash_tlv.it_len = sizeof(img->hash);

	/* Fill hash with test pattern */
	for (int i = 0; i < sizeof(img->hash); i++) {
		img->hash[i] = (uint8_t)i;
	}
}

static int write_fake_image_to_slot(int slot, struct test_fake_image *img)
{
	const struct flash_area *fa;
	int area_id;
	int rc;

	area_id = dfu_boot_get_flash_area_id(slot);
	if (area_id < 0) {
		return area_id;
	}

	rc = flash_area_open(area_id, &fa);
	if (rc != 0) {
		return rc;
	}

	/* Erase first */
	rc = flash_area_erase(fa, 0, fa->fa_size);
	if (rc != 0) {
		flash_area_close(fa);
		return rc;
	}

	/* Write the fake image at the image start offset */
	size_t offset = boot_get_image_start_offset(area_id);

	rc = flash_area_write(fa, offset, img, sizeof(*img));
	flash_area_close(fa);

	return rc;
}

ZTEST(mcuboot_interface, test_bank_erase)
{
	const struct flash_area *fa;
	uint32_t temp;
	uint32_t temp2 = 0x5a5a5a5a;
	off_t offs;
	int ret;

	ret = flash_area_open(SLOT1_PARTITION_ID, &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	for (offs = 0; offs < fa->fa_size; offs += sizeof(temp)) {
		ret = flash_area_read(fa, offs, &temp, sizeof(temp));
		zassert_true(ret == 0, "Reading from flash");
		if (temp == 0xFFFFFFFF) {
			ret = flash_area_write(fa, offs, &temp2, sizeof(temp));
			zassert_true(ret == 0, "Writing to flash");
		}
	}

	zassert(boot_erase_img_bank(SLOT1_PARTITION_ID) == 0,
		"pass", "fail");

	for (offs = 0; offs < fa->fa_size; offs += sizeof(temp)) {
		ret = flash_area_read(fa, offs, &temp, sizeof(temp));
		zassert_true(ret == 0, "Reading from flash");
		zassert(temp == 0xFFFFFFFF, "pass", "fail");
	}
}

ZTEST(mcuboot_interface, test_request_upgrade)
{
	const struct flash_area *fa;
	const uint32_t expectation[6] = {
		0xffffffff,
		0xffffffff,
		BOOT_MAGIC_VAL_W0,
		BOOT_MAGIC_VAL_W1,
		BOOT_MAGIC_VAL_W2,
		BOOT_MAGIC_VAL_W3
	};
	uint32_t readout[ARRAY_SIZE(expectation)];
	int ret;

	ret = flash_area_open(SLOT1_PARTITION_ID, &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	zassert(boot_request_upgrade(false) == 0, "pass", "fail");

	ret = flash_area_read(fa, fa->fa_size - sizeof(expectation),
			      &readout, sizeof(readout));
	zassert_true(ret == 0, "Read from flash");

	zassert(memcmp(expectation, readout, sizeof(expectation)) == 0,
		"pass", "fail");

	zassert(boot_erase_img_bank(SLOT1_PARTITION_ID) == 0,
				    "pass", "fail");

	zassert(boot_request_upgrade(true) == 0, "pass", "fail");

	ret = flash_area_read(fa, fa->fa_size - sizeof(expectation),
			      &readout, sizeof(readout));
	zassert_true(ret == 0, "Read from flash");

	zassert(memcmp(&expectation[2], &readout[2], sizeof(expectation) -
		       2 * sizeof(expectation[0])) == 0, "pass", "fail");

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

ZTEST(mcuboot_interface, test_write_confirm)
{
	const uint32_t img_magic[4] = BOOT_MAGIC_VALUES;
	uint32_t readout[ARRAY_SIZE(img_magic)];
	uint8_t flag[BOOT_MAX_ALIGN];
	const struct flash_area *fa;
	int ret;

	flag[0] = 0x01;
	memset(&flag[1], 0xff, sizeof(flag) - 1);

	ret = flash_area_open(SLOT0_PARTITION_ID, &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	zassert(boot_erase_img_bank(SLOT0_PARTITION_ID) == 0,
		"pass", "fail");

	ret = flash_area_read(fa, fa->fa_size - sizeof(img_magic),
			      &readout, sizeof(img_magic));
	zassert_true(ret == 0, "Read from flash");

	if (memcmp(img_magic, readout, sizeof(img_magic)) != 0) {
		ret = flash_area_write(fa, fa->fa_size - 16,
				       img_magic, 16);
		zassert_true(ret == 0, "Write to flash");
	}

	/* set copy-done flag */
	ret = flash_area_write(fa, fa->fa_size - 32, &flag, sizeof(flag));
	zassert_true(ret == 0, "Write to flash");

	ret = boot_write_img_confirmed();
	zassert(ret == 0, "pass", "fail (%d)", ret);

	ret = flash_area_read(fa, fa->fa_size - 24, readout,
			      sizeof(readout[0]));
	zassert_true(ret == 0, "Read from flash");

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

/*
 * DFU Boot API tests using MCUboot-specific fake image data
 */

ZTEST(mcuboot_interface, test_dfu_boot_validate_header_mcuboot)
{
	struct test_fake_image fake_img;
	struct dfu_boot_img_info info;
	int rc;

	create_fake_mcuboot_image(&fake_img);

	/* Test valid MCUboot header */
	rc = dfu_boot_validate_header(&fake_img.header, sizeof(fake_img.header), &info);
	zassert_equal(rc, 0, "Validating valid MCUboot header should succeed");
	zassert_true(info.valid, "Image info should be marked valid");
	zassert_equal(info.version.major, TEST_IMG_VER_MAJOR, "Major version mismatch");
	zassert_equal(info.version.minor, TEST_IMG_VER_MINOR, "Minor version mismatch");
	zassert_equal(info.version.revision, TEST_IMG_VER_REV, "Revision mismatch");
	zassert_equal(info.version.build_num, TEST_IMG_VER_BUILD, "Build number mismatch");
	zassert_equal(info.img_size, sizeof(fake_img.fake_code), "Image size mismatch");
	zassert_equal(info.hdr_size, IMAGE_HEADER_SIZE, "Header size mismatch");

	/* Test with corrupted magic */
	fake_img.header.ih_magic = 0xDEADBEEFU;
	rc = dfu_boot_validate_header(&fake_img.header, sizeof(fake_img.header), &info);
	zassert_true(rc != 0, "Validating corrupted magic should fail");
}

ZTEST(mcuboot_interface, test_dfu_boot_validate_header_flags)
{
	struct test_fake_image fake_img;
	struct dfu_boot_img_info info;
	int rc;

	create_fake_mcuboot_image(&fake_img);

	/* Test with ROM_FIXED flag (0x100 in MCUboot) */
	fake_img.header.ih_flags = 0x100;
	rc = dfu_boot_validate_header(&fake_img.header, sizeof(fake_img.header), &info);
	zassert_equal(rc, 0, "Validating header with ROM_FIXED flag should succeed");
	zassert_true(info.flags & DFU_BOOT_IMG_F_ROM_FIXED, "ROM_FIXED flag should be set");

	/* Test with NON_BOOTABLE flag (0x10 in MCUboot) */
	create_fake_mcuboot_image(&fake_img);
	fake_img.header.ih_flags = 0x10;
	rc = dfu_boot_validate_header(&fake_img.header, sizeof(fake_img.header), &info);
	zassert_equal(rc, 0, "Validating header with NON_BOOTABLE flag should succeed");
	zassert_true(info.flags & DFU_BOOT_IMG_F_NON_BOOTABLE, "NON_BOOTABLE flag should be set");
}

ZTEST(mcuboot_interface, test_dfu_boot_read_img_info_mcuboot)
{
	struct test_fake_image fake_img;
	struct dfu_boot_img_info info;
	int rc;

	create_fake_mcuboot_image(&fake_img);

	/* Write fake image to slot 1 */
	rc = write_fake_image_to_slot(1, &fake_img);
	zassert_equal(rc, 0, "Writing fake image to slot 1 should succeed");

	/* Read image info using dfu_boot API */
	rc = dfu_boot_read_img_info(1, &info);
	zassert_equal(rc, 0, "Reading image info should succeed");
	zassert_true(info.valid, "Image info should be valid");

	/* Verify version */
	zassert_equal(info.version.major, TEST_IMG_VER_MAJOR, "Major version mismatch");
	zassert_equal(info.version.minor, TEST_IMG_VER_MINOR, "Minor version mismatch");
	zassert_equal(info.version.revision, TEST_IMG_VER_REV, "Revision mismatch");
	zassert_equal(info.version.build_num, TEST_IMG_VER_BUILD, "Build number mismatch");

	/* Verify hash was read */
	zassert_equal(info.hash_len, 32, "Hash length should be 32 bytes for SHA256");
	for (int i = 0; i < 32; i++) {
		zassert_equal(info.hash[i], (uint8_t)i, "Hash byte %d mismatch", i);
	}

	/* Cleanup */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");
}

ZTEST(mcuboot_interface, test_dfu_boot_set_pending_mcuboot)
{
	const struct flash_area *fa;
	const uint32_t boot_magic[4] = BOOT_MAGIC_VALUES;
	uint32_t readout[4];
	int rc;

	/* Erase slot 1 first */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");

	/* Set slot 1 as pending (non-permanent) using dfu_boot API */
	rc = dfu_boot_set_pending(1, false);
	zassert_equal(rc, 0, "Setting slot 1 as pending should succeed");

	/* Verify boot magic was written to trailer */
	rc = flash_area_open(SLOT1_PARTITION_ID, &fa);
	zassert_equal(rc, 0, "Opening flash area should succeed");

	rc = flash_area_read(fa, fa->fa_size - sizeof(boot_magic), readout, sizeof(readout));
	zassert_equal(rc, 0, "Reading from flash should succeed");

	zassert_equal(readout[0], boot_magic[0], "Boot magic word 0 mismatch");
	zassert_equal(readout[1], boot_magic[1], "Boot magic word 1 mismatch");
	zassert_equal(readout[2], boot_magic[2], "Boot magic word 2 mismatch");
	zassert_equal(readout[3], boot_magic[3], "Boot magic word 3 mismatch");

	flash_area_close(fa);

	/* Cleanup */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");
}

ZTEST(mcuboot_interface, test_dfu_boot_confirm_mcuboot)
{
	const uint32_t img_magic[4] = BOOT_MAGIC_VALUES;
	uint32_t readout[4];
	uint8_t flag[BOOT_MAX_ALIGN];
	const struct flash_area *fa;
	int rc;

	flag[0] = 0x01;
	memset(&flag[1], 0xff, sizeof(flag) - 1);

	rc = flash_area_open(SLOT0_PARTITION_ID, &fa);
	zassert_equal(rc, 0, "Opening flash area should succeed");

	/* Erase and prepare slot 0 with magic and copy-done flag */
	rc = boot_erase_img_bank(SLOT0_PARTITION_ID);
	zassert_equal(rc, 0, "Erasing slot 0 should succeed");

	rc = flash_area_write(fa, fa->fa_size - 16, img_magic, 16);
	zassert_equal(rc, 0, "Writing boot magic should succeed");

	rc = flash_area_write(fa, fa->fa_size - 32, &flag, sizeof(flag));
	zassert_equal(rc, 0, "Writing copy-done flag should succeed");

	/* Confirm using dfu_boot API */
	rc = dfu_boot_confirm();
	zassert_equal(rc, 0, "dfu_boot_confirm should succeed");

	/* Verify image_ok flag was written */
	rc = flash_area_read(fa, fa->fa_size - 24, readout, sizeof(readout[0]));
	zassert_equal(rc, 0, "Reading from flash should succeed");
	zassert_equal(readout[0] & 0xff, 1, "image_ok flag should be set");

	flash_area_close(fa);
}

ZTEST(mcuboot_interface, test_dfu_boot_swap_type_mcuboot)
{
	int swap_type;
	int rc;

	/* Erase slot 1 to ensure clean state */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");

	/* With no pending image, swap type should be NONE */
	swap_type = dfu_boot_get_swap_type(0);
	zassert_equal(swap_type, DFU_BOOT_SWAP_TYPE_NONE,
		      "Swap type should be NONE when no image is pending");
}

ZTEST(mcuboot_interface, test_dfu_boot_offsets_mcuboot)
{
	const struct flash_area *fa;
	size_t img_offset;
	size_t trailer_offset;
	int rc;

	rc = flash_area_open(SLOT1_PARTITION_ID, &fa);
	zassert_equal(rc, 0, "Opening flash area should succeed");

	/* Test image start offset */
	img_offset = dfu_boot_get_image_start_offset(1);
	/* For most configurations, this should be 0 */
	zassert_true(img_offset < fa->fa_size, "Image offset should be within flash area");

	/* Test trailer status offset */
	trailer_offset = dfu_boot_get_trailer_status_offset(fa->fa_size);
	zassert_true(trailer_offset > 0, "Trailer offset should be positive");
	zassert_true(trailer_offset < fa->fa_size, "Trailer offset should be within flash area");

	/* Trailer should be near the end of the flash area */
	zassert_true(trailer_offset > fa->fa_size / 2,
		     "Trailer should be in the second half of flash area");

	flash_area_close(fa);
}

ZTEST_SUITE(mcuboot_interface, NULL, NULL, NULL, NULL, NULL);
