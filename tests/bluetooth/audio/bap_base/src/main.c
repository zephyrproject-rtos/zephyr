/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

struct bap_base_test_suite_fixture {
	struct bt_data valid_base_ad;
	uint8_t *valid_base_data;
	struct bt_data invalid_base_ad;
	uint8_t *invalid_base_data;
};

static void bap_base_test_suite_fixture_init(struct bap_base_test_suite_fixture *fixture)
{
	uint8_t base_data[] = {
		0x51, 0x18,                   /* uuid */
		0x40, 0x9C, 0x00,             /* pd */
		0x02,                         /* subgroup count */
		0x01,                         /* Subgroup 1: bis count */
		0x06, 0x00, 0x00, 0x00, 0x00, /* LC3 codec_id*/
		0x10,                         /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x01,                                           /* bis index */
		0x03,                                           /* bis cc length */
		0x02, 0x03, 0x03,                               /* bis cc length */
		0x01,                                           /* Subgroup 1: bis count */
		0x06, 0x00, 0x00, 0x00, 0x00,                   /* LC3 codec_id*/
		0x10,                                           /* cc length */
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00, /* cc */
		0x04,                                           /* meta length */
		0x03, 0x02, 0x01, 0x00,                         /* meta */
		0x02,                                           /* bis index */
		0x03,                                           /* bis cc length */
		0x02, 0x03, 0x03                                /* bis cc length */
	};

	fixture->valid_base_data = malloc(sizeof(base_data));
	zassert_not_null(fixture->valid_base_data);
	memcpy(fixture->valid_base_data, base_data, sizeof(base_data));

	fixture->valid_base_ad.type = 0x16; /* service data */
	fixture->valid_base_ad.data_len = sizeof(base_data);
	fixture->valid_base_ad.data = fixture->valid_base_data;

	/* Modify the CC length to generate an invalid BASE for invalid BASE tests */
	base_data[12] = 0xaa; /* Set invalid CC length*/
	fixture->invalid_base_data = malloc(sizeof(base_data));
	zassert_not_null(fixture->invalid_base_data);
	memcpy(fixture->invalid_base_data, base_data, sizeof(base_data));

	fixture->invalid_base_ad.type = 0x16; /* service data */
	fixture->invalid_base_ad.data_len = sizeof(base_data);
	fixture->invalid_base_ad.data = fixture->invalid_base_data;
}

static void *bap_base_test_suite_setup(void)
{
	struct bap_base_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void bap_base_test_suite_before(void *f)
{
	memset(f, 0, sizeof(struct bap_base_test_suite_fixture));
	bap_base_test_suite_fixture_init(f);
}

static void bap_base_test_suite_after(void *f)
{
	struct bap_base_test_suite_fixture *fixture = f;

	free(fixture->valid_base_data);
}

static void bap_base_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(bap_base_test_suite, NULL, bap_base_test_suite_setup, bap_base_test_suite_before,
	    bap_base_test_suite_after, bap_base_test_suite_teardown);

ZTEST_F(bap_base_test_suite, test_base_get_base_from_ad)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);

	zassert_not_null(base);
}

ZTEST_F(bap_base_test_suite, test_base_get_base_from_ad_inval_base)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->invalid_base_ad);

	zassert_is_null(base);
}

ZTEST_F(bap_base_test_suite, test_base_get_base_from_ad_inval_param_null)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(NULL);

	zassert_is_null(base);
}

ZTEST_F(bap_base_test_suite, test_base_get_base_from_ad_inval_param_type)
{
	const struct bt_bap_base *base;

	fixture->valid_base_ad.type = 0x03; /* BT_DATA_UUID16_ALL */

	base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);

	zassert_is_null(base);
}

ZTEST_F(bap_base_test_suite, test_base_get_base_from_ad_inval_param_len)
{
	const struct bt_bap_base *base;

	fixture->valid_base_ad.data_len = 0x03; /* Minimum len is BASE_MIN_SIZE (16) */

	base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);

	zassert_is_null(base);
}

ZTEST_F(bap_base_test_suite, test_base_get_base_from_ad_inval_param_uuid)
{
	const struct bt_bap_base *base;

	/* Modify the BASE data to have invalid UUID */
	fixture->valid_base_data[0] = 0x01;
	fixture->valid_base_data[1] = 0x02;

	base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);

	zassert_is_null(base);
}

ZTEST_F(bap_base_test_suite, test_base_get_pres_delay)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_get_pres_delay(base);
	zassert_equal(ret, 40000, "Unexpected presentation delay: %d", ret);
}

ZTEST_F(bap_base_test_suite, test_base_get_pres_delay_inval_param_null)
{
	int ret;

	ret = bt_bap_base_get_pres_delay(NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_count)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_get_subgroup_count(base);
	zassert_equal(ret, 2, "Unexpected presentation delay: %d", ret);
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_count_inval_param_null)
{
	int ret;

	ret = bt_bap_base_get_subgroup_count(NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);
}

ZTEST_F(bap_base_test_suite, test_base_get_bis_indexes)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	uint32_t bis_indexes;
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_get_bis_indexes(base, &bis_indexes);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_equal(bis_indexes, 0x00000006 /* Bit 1 and 2 */,
		      "Unexpected BIS index value: 0x%08X", bis_indexes);
}

ZTEST_F(bap_base_test_suite, test_base_get_bis_indexes_inval_param_null_base)
{
	uint32_t bis_indexes;
	int ret;

	ret = bt_bap_base_get_bis_indexes(NULL, &bis_indexes);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);
}

ZTEST_F(bap_base_test_suite, test_base_get_bis_indexes_inval_param_null_index)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_get_bis_indexes(base, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);
}

static bool test_base_foreach_subgroup_cb(const struct bt_bap_base_subgroup *subgroup,
					  void *user_data)
{
	size_t *count = user_data;

	(*count)++;

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_foreach_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	size_t count = 0U;
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_foreach_subgroup_cb, &count);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_equal(count, 0x02, "Unexpected subgroup count value: %u", count);
}

ZTEST_F(bap_base_test_suite, test_base_foreach_subgroup_inval_param_null_base)
{
	size_t count;
	int ret;

	ret = bt_bap_base_foreach_subgroup(NULL, test_base_foreach_subgroup_cb, &count);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);
}

ZTEST_F(bap_base_test_suite, test_base_foreach_subgroup_inval_param_null_cb)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, NULL, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_codec_id_cb(const struct bt_bap_base_subgroup *subgroup,
					       void *user_data)
{
	struct bt_bap_base_codec_id codec_id;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, &codec_id);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_equal(codec_id.id, 0x06, "Unexpected codec.id value: %u", codec_id.id);
	zassert_equal(codec_id.cid, 0x0000, "Unexpected codec.cid value: %u", codec_id.cid);
	zassert_equal(codec_id.vid, 0x0000, "Unexpected codec.vid value: %u", codec_id.vid);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_id)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_get_subgroup_codec_id_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_codec_id_inval_param_null_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	struct bt_bap_base_codec_id codec_id;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_id(NULL, &codec_id);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_id_inval_param_null_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_get_subgroup_codec_id_inval_param_null_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool
test_base_get_subgroup_codec_id_inval_param_null_cb(const struct bt_bap_base_subgroup *subgroup,
						    void *user_data)
{
	int ret;

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_id_inval_param_null)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_get_subgroup_codec_id_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_codec_data_cb(const struct bt_bap_base_subgroup *subgroup,
						 void *user_data)
{
	const uint8_t expected_data[] = {
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00,
	};
	uint8_t *data;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_data(subgroup, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value: %d", ret);
	zassert_mem_equal(data, expected_data, sizeof(expected_data));

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_data)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_get_subgroup_codec_data_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_codec_data_inval_param_null_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	uint8_t *data;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_data(NULL, &data);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_data_inval_param_null_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_get_subgroup_codec_data_inval_param_null_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool
test_base_get_subgroup_codec_data_inval_param_null_cb(const struct bt_bap_base_subgroup *subgroup,
						      void *user_data)
{
	int ret;

	ret = bt_bap_base_get_subgroup_codec_data(subgroup, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_data_inval_param_null)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_get_subgroup_codec_data_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_codec_meta_cb(const struct bt_bap_base_subgroup *subgroup,
						 void *user_data)
{
	const uint8_t expected_data[] = {0x03, 0x02, 0x01, 0x00};
	uint8_t *data;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value: %d", ret);
	zassert_mem_equal(data, expected_data, sizeof(expected_data));

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_meta)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_get_subgroup_codec_meta_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_codec_meta_inval_param_null_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	uint8_t *data;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_meta(NULL, &data);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_meta_inval_param_null_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_get_subgroup_codec_meta_inval_param_null_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool
test_base_get_subgroup_codec_meta_inval_param_null_cb(const struct bt_bap_base_subgroup *subgroup,
						      void *user_data)
{
	int ret;

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_codec_meta_inval_param_null)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_get_subgroup_codec_meta_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_subgroup_codec_to_codec_cfg_cb(const struct bt_bap_base_subgroup *subgroup,
						     void *user_data)
{
	const uint8_t expected_meta[] = {0x03, 0x02, 0x01, 0x00};
	const uint8_t expected_data[] = {
		0x02, 0x01, 0x03, 0x02, 0x02, 0x01, 0x05, 0x03,
		0x01, 0x00, 0x00, 0x00, 0x03, 0x04, 0x28, 0x00,
	};
	struct bt_audio_codec_cfg codec_cfg;
	int ret;

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &codec_cfg);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_equal(codec_cfg.data_len, sizeof(expected_data), "Unexpected data length: %d", ret);
	zassert_equal(codec_cfg.meta_len, sizeof(expected_meta), "Unexpected meta length: %d", ret);
	zassert_mem_equal(codec_cfg.data, expected_data, sizeof(expected_data));
	zassert_mem_equal(codec_cfg.meta, expected_meta, sizeof(expected_meta));

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_codec_to_codec_cfg)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_subgroup_codec_to_codec_cfg_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_subgroup_codec_to_codec_cfg_inval_param_null_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	struct bt_audio_codec_cfg codec_cfg;
	int ret;

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(NULL, &codec_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_codec_to_codec_cfg_inval_param_null_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_subgroup_codec_to_codec_cfg_inval_param_null_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_subgroup_codec_to_codec_cfg_inval_param_null_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	int ret;

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_codec_to_codec_cfg_inval_param_null)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_subgroup_codec_to_codec_cfg_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_bis_count_cb(const struct bt_bap_base_subgroup *subgroup,
						void *user_data)
{
	int ret;

	ret = bt_bap_base_get_subgroup_bis_count(subgroup);
	zassert_equal(ret, 0x01, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_bis_count)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_get_subgroup_bis_count_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_get_subgroup_bis_count_inval_param_null_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	int ret;

	ret = bt_bap_base_get_subgroup_bis_count(NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_get_subgroup_bis_count_inval_param_null_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_get_subgroup_bis_count_inval_param_null_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool
test_bt_bap_base_subgroup_get_bis_indexes_cb(const struct bt_bap_base_subgroup *subgroup,
					     void *user_data)
{
	uint32_t bis_indexes;
	int ret;

	ret = bt_bap_base_subgroup_get_bis_indexes(subgroup, &bis_indexes);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_not_equal(bis_indexes, 0 /* May be Bit 1 or 2 */,
			  "Unexpected BIS index value: 0x%08X", bis_indexes);

	return true;
}

ZTEST_F(bap_base_test_suite, test_bt_bap_base_subgroup_get_bis_indexes)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	uint32_t bis_indexes;
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_bt_bap_base_subgroup_get_bis_indexes_cb,
					   NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_bt_bap_base_subgroup_get_bis_indexes_inval_param_null_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	uint32_t bis_indexes;
	int ret;

	ret = bt_bap_base_subgroup_get_bis_indexes(NULL, &bis_indexes);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_bt_bap_base_subgroup_get_bis_indexes_inval_param_null_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_bt_bap_base_subgroup_get_bis_indexes_inval_param_null_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_bt_bap_base_subgroup_get_bis_indexes_inval_param_null_index_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	int ret;

	ret = bt_bap_base_subgroup_get_bis_indexes(subgroup, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_bt_bap_base_subgroup_get_bis_indexes_inval_param_null_index)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_bt_bap_base_subgroup_get_bis_indexes_inval_param_null_index_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool
test_base_subgroup_foreach_bis_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis,
					       void *user_data)
{
	size_t *count = user_data;

	(*count)++;

	return true;
}

static bool test_base_subgroup_foreach_bis_subgroup_cb(const struct bt_bap_base_subgroup *subgroup,
						       void *user_data)
{
	size_t *total_count = user_data;
	size_t count = 0U;
	int ret;

	ret = bt_bap_base_subgroup_foreach_bis(
		subgroup, test_base_subgroup_foreach_bis_subgroup_bis_cb, &count);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_equal(count, 0x01, "Unexpected subgroup count value: %u", count);

	*total_count += count;

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_foreach_bis)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	size_t count = 0U;
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(base, test_base_subgroup_foreach_bis_subgroup_cb,
					   &count);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_equal(count, 0x02, "Unexpected subgroup count value: %u", count);
}

static bool test_base_subgroup_foreach_bis_inval_param_null_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	size_t count;
	int ret;

	ret = bt_bap_base_subgroup_foreach_bis(NULL, test_base_subgroup_foreach_bis_subgroup_bis_cb,
					       &count);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_foreach_bis_inval_param_null_subgroup)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_subgroup_foreach_bis_inval_param_null_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool
test_base_subgroup_foreach_bis_inval_param_null_cb_cb(const struct bt_bap_base_subgroup *subgroup,
						      void *user_data)
{
	int ret;

	ret = bt_bap_base_subgroup_foreach_bis(subgroup, NULL, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_foreach_bis_inval_param_null_cb)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_subgroup_foreach_bis_inval_param_null_cb_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool
test_base_subgroup_bis_codec_to_codec_cfg_bis_cb(const struct bt_bap_base_subgroup_bis *bis,
						 void *user_data)
{
	const uint8_t expected_data[] = {0x02, 0x03, 0x03};
	struct bt_audio_codec_cfg codec_cfg;
	int ret;

	ret = bt_bap_base_subgroup_bis_codec_to_codec_cfg(bis, &codec_cfg);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
	zassert_equal(codec_cfg.data_len, sizeof(expected_data), "Unexpected data length: %d", ret);
	zassert_mem_equal(codec_cfg.data, expected_data, sizeof(expected_data));

	return true;
}

static bool
test_base_subgroup_bis_codec_to_codec_cfg_subgroup_cb(const struct bt_bap_base_subgroup *subgroup,
						      void *user_data)
{
	int ret;

	ret = bt_bap_base_subgroup_foreach_bis(
		subgroup, test_base_subgroup_bis_codec_to_codec_cfg_bis_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_bis_codec_to_codec_cfg)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_subgroup_bis_codec_to_codec_cfg_subgroup_cb, NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_bis_bis_cb(
	const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	struct bt_audio_codec_cfg codec_cfg;
	int ret;

	ret = bt_bap_base_subgroup_bis_codec_to_codec_cfg(NULL, &codec_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

static bool test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_bis_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	int ret;

	ret = bt_bap_base_subgroup_foreach_bis(
		NULL, test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_bis_bis_cb, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_foreach_bis_inval_param_null_bis)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base, test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_bis_subgroup_cb,
		NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}

static bool test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_codec_cfg_bis_cb(
	const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	int ret;

	ret = bt_bap_base_subgroup_bis_codec_to_codec_cfg(bis, NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

static bool test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_codec_cfg_subgroup_cb(
	const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	int ret;

	ret = bt_bap_base_subgroup_foreach_bis(
		NULL, test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_codec_cfg_bis_cb,
		NULL);
	zassert_equal(ret, -EINVAL, "Unexpected return value: %d", ret);

	return true;
}

ZTEST_F(bap_base_test_suite, test_base_subgroup_foreach_bis_inval_param_null_codec_cfg)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(&fixture->valid_base_ad);
	int ret;

	zassert_not_null(base);

	ret = bt_bap_base_foreach_subgroup(
		base,
		test_base_subgroup_bis_codec_to_codec_cfg_inval_param_null_codec_cfg_subgroup_cb,
		NULL);
	zassert_equal(ret, 0, "Unexpected return value: %d", ret);
}
