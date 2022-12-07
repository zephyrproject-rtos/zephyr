/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/flash_img.h>

#define SLOT0_PARTITION		slot0_partition
#define SLOT1_PARTITION		slot1_partition

#define SLOT0_PARTITION_ID	FIXED_PARTITION_ID(SLOT0_PARTITION)

#define SLOT1_PARTITION_ID	FIXED_PARTITION_ID(SLOT1_PARTITION)

ZTEST(img_util, test_init_id)
{
	struct flash_img_context ctx_no_id;
	struct flash_img_context ctx_id;
	int ret;

	ret = flash_img_init(&ctx_no_id);
	zassert_true(ret == 0, "Flash img init");

	ret = flash_img_init_id(&ctx_id, SLOT1_PARTITION_ID);
	zassert_true(ret == 0, "Flash img init id");

	/* Verify that the default partition ID is IMAGE_1 */
	zassert_equal(ctx_id.flash_area, ctx_no_id.flash_area,
		      "Default partition ID is incorrect");

	/* Note: IMAGE_0, not IMAGE_1 as above */
	ret = flash_img_init_id(&ctx_id, SLOT0_PARTITION_ID);
	zassert_true(ret == 0, "Flash img init id");

	zassert_equal(ctx_id.flash_area->fa_id, SLOT0_PARTITION_ID,
		      "Partition ID is not set correctly");
}

ZTEST(img_util, test_collecting)
{
	const struct flash_area *fa;
	struct flash_img_context ctx;
	uint32_t i, j;
	uint8_t data[5], temp, k;
	int ret;

	ret = flash_img_init(&ctx);
	zassert_true(ret == 0, "Flash img init");

#ifdef CONFIG_IMG_ERASE_PROGRESSIVELY
	uint8_t erase_buf[8];
	(void)memset(erase_buf, 0xff, sizeof(erase_buf));

	ret = flash_area_open(SLOT1_PARTITION_ID, &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	/* ensure image payload area dirt */
	for (i = 0U; i < 300 * sizeof(data) / sizeof(erase_buf); i++) {
		ret = flash_area_write(fa, i * sizeof(erase_buf), erase_buf,
				       sizeof(erase_buf));
		zassert_true(ret == 0, "Flash write failure (%d)", ret);
	}

	/* ensure that the last page dirt */
	ret = flash_area_write(fa, fa->fa_size - sizeof(erase_buf), erase_buf,
			       sizeof(erase_buf));
	zassert_true(ret == 0, "Flash write failure (%d)", ret);
#else
	ret = flash_area_erase(ctx.flash_area, 0, ctx.flash_area->fa_size);
	zassert_true(ret == 0, "Flash erase failure (%d)", ret);
#endif

	zassert(flash_img_bytes_written(&ctx) == 0, "pass", "fail");

	k = 0U;
	for (i = 0U; i < 300; i++) {
		for (j = 0U; j < ARRAY_SIZE(data); j++) {
			data[j] = k++;
		}
		ret = flash_img_buffered_write(&ctx, data, sizeof(data), false);
		zassert_true(ret == 0, "image collection fail: %d\n", ret);
	}

	zassert(flash_img_buffered_write(&ctx, data, 0, true) == 0, "pass",
					 "fail");


	ret = flash_area_open(SLOT1_PARTITION_ID, &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	k = 0U;
	for (i = 0U; i < 300 * sizeof(data); i++) {
		zassert(flash_area_read(fa, i, &temp, 1) == 0, "pass", "fail");
		zassert(temp == k, "pass", "fail");
		k++;
	}

#ifdef CONFIG_IMG_ERASE_PROGRESSIVELY
	uint8_t buf[sizeof(erase_buf)];

	ret = flash_area_read(fa, fa->fa_size - sizeof(buf), buf, sizeof(buf));
	zassert_true(ret == 0, "Flash read failure (%d)", ret);
	zassert_true(memcmp(erase_buf, buf, sizeof(buf)) == 0,
		     "Image trailer was not cleared");
#endif
}

ZTEST(img_util, test_check_flash)
{
	/* echo $'0123456789abcdef\nfedcba9876543201' > tst.sha
	 * hexdump tst.sha
	 */
	uint8_t tst_vec[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			      0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
			      0x0a, 0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x39,
			      0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31,
			      0x30, 0x0a };
	/* sha256sum tst.sha */
	uint8_t tst_sha[] = { 0xc6, 0xb6, 0x7c, 0x46, 0xe7, 0x2e, 0x14, 0x17,
			      0x49, 0xa4, 0xd2, 0xf1, 0x38, 0x58, 0xb2, 0xa7,
			      0x54, 0xaf, 0x6d, 0x39, 0x50, 0x6b, 0xd5, 0x41,
			      0x90, 0xf6, 0x18, 0x1a, 0xe0, 0xc2, 0x7f, 0x98 };

	struct flash_img_check fic = { NULL, 0 };
	struct flash_img_context ctx;
	int ret;

	ret = flash_img_init_id(&ctx, SLOT1_PARTITION_ID);
	zassert_true(ret == 0, "Flash img init 1");
	ret = flash_area_erase(ctx.flash_area, 0, ctx.flash_area->fa_size);
	zassert_true(ret == 0, "Flash erase failure (%d)\n", ret);
	ret = flash_img_buffered_write(&ctx, tst_vec, sizeof(tst_vec), true);
	zassert_true(ret == 0, "Flash img buffered write\n");

	ret = flash_img_check(NULL, NULL, 0);
	zassert_true(ret == -EINVAL, "Flash img check params 1, 2\n");
	ret = flash_img_check(NULL, &fic, 0);
	zassert_true(ret == -EINVAL, "Flash img check params 2\n");
	ret = flash_img_check(&ctx, NULL, 0);
	zassert_true(ret == -EINVAL, "Flash img check params 1\n");

	ret = flash_img_check(&ctx, &fic, SLOT1_PARTITION_ID);
	zassert_true(ret == -EINVAL, "Flash img check fic match\n");
	fic.match = tst_sha;
	ret = flash_img_check(&ctx, &fic, SLOT1_PARTITION_ID);
	zassert_true(ret == -EINVAL, "Flash img check fic len\n");
	fic.clen = sizeof(tst_vec);

	ret = flash_img_check(&ctx, &fic, SLOT1_PARTITION_ID);
	zassert_true(ret == 0, "Flash img check\n");
	tst_sha[0] = 0x00;
	ret = flash_img_check(&ctx, &fic, SLOT1_PARTITION_ID);
	zassert_false(ret == 0, "Flash img check wrong sha\n");

	flash_area_close(ctx.flash_area);
}

ZTEST_SUITE(img_util, NULL, NULL, NULL, NULL, NULL);
