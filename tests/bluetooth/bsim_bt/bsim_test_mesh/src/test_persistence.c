/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_test.h"
#include <bluetooth/mesh.h>
#include <sys/reboot.h>
#include "mesh/net.h"
#include "mesh/app_keys.h"


#define LOG_MODULE_NAME test_persistence

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define WAIT_TIME 60 /*seconds*/

static const struct bt_mesh_test_cfg save_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
	.net_idx = 0x01,
	.net_key = { 0x02 }
};

static const struct bt_mesh_test_cfg load_cfg = {
	.addr = 0x0ADD,
	.dev_key = { 0x0D },
	.net_idx = 0x77,
	.net_key = { 0x0E }
};

static void test_provisioning_sv_init(void)
{
	LOG_INF("%s", __func__);
}

static void test_provisioning_ld_init(void)
{
	LOG_INF("%s", __func__);
}

/* This function is used only for debug and analysis of the
 * settings file while working with the continuous test development
 */
static int read_dir(char *dirpath)
{
	struct fs_dir_t dirp = { 0 };
	int err = fs_opendir(&dirp, dirpath);

	if (!err) {
		struct fs_dirent dentry;

		err = fs_readdir(&dirp, &dentry);

		while (!err) {
			if (dentry.name[0] != 0) {
				char innerpath[100] = { 0 };

				strcpy(innerpath, dirpath);
				strcat(innerpath, "/");
				strcat(innerpath, dentry.name);

				if (dentry.type == FS_DIR_ENTRY_DIR) {
					err = read_dir(innerpath);
				} else {
					struct fs_file_t fp = { 0 };

					LOG_INF("Size: %4d Entry: %s", dentry.size, innerpath);
					err = fs_open(&fp, innerpath, FS_O_READ);
					if (!err) {
						char filedata[1000];
						ssize_t n = fs_read(&fp, &filedata,
								    ARRAY_SIZE(filedata));
						LOG_HEXDUMP_INF(filedata, n, "raw data");

						err = fs_close(&fp);
					}
				}
				err = fs_readdir(&dirp, &dentry);
				if (err) {
					LOG_ERR("Fail to read directory %s", dirpath);
					return fs_closedir(&dirp);
				}
			} else {
				break;
			}
		}

		return fs_closedir(&dirp);
	}

	return err;
}

struct key_checker_data {
	char key[25];
	const void *exp_value;
	int exp_value_len;
};


typedef int (*key_value_checker_cb)(const char *key, size_t keylen,
				    const uint8_t *data, ssize_t datalen);

static bool key_checked;

static int set_load_direct_cb(const char *key,
			      size_t len,
			      settings_read_cb read_cb,
			      void *cb_arg,
			      void *param)
{
	uint8_t data[100];

	LOG_INF("key: %s", key);

	ssize_t bytes = read_cb(cb_arg, &data, sizeof(data));

	LOG_HEXDUMP_INF(data, bytes, "data:");

	struct key_checker_data *validate = (struct key_checker_data *)param;

	if (strcmp(key, validate->key) == 0) {
		key_checked = true;

		if (!(bytes == validate->exp_value_len &&
		      memcmp(data, validate->exp_value, bytes) == 0)) {
			FAIL("Validation failed for key %s", key);
			return -1;
		}
	}

	return 0;
}

static int persistent_values_check(struct key_checker_data *validater)
{

	key_checked = false;
	settings_load_subtree_direct("bt", set_load_direct_cb, validater);
	if (!key_checked) {
		FAIL("Not found. Key %s", validater->key);
		return -1;
	}

	return 0;
}

/* these data structures are taken from various relavent modules to be checked */
static struct net_key_val {
	uint8_t kr_flag : 1,
		kr_phase : 7;
	uint8_t val[2][16];
} __packed;

static struct iv_val {
	uint32_t iv_index;
	uint8_t iv_update : 1,
		iv_duration : 7;
} __packed;

static struct net_val {
	uint16_t primary_addr;
	uint8_t dev_key[16];
} __packed;

static void test_provisioning_sv_check_save(void)
{
	struct key_checker_data validater = { 0 };

	mount_settings_area();
	bt_mesh_test_cfg_set(&save_cfg, WAIT_TIME);
	bt_mesh_test_setup(false);

	/* print raw dump of settings file */
	char dir_path[100] = CONFIG_SETTINGS_FS_DIR;

	read_dir(dir_path);

	/* Verify: check stored provisioning data */
	/* check stored netkey matches with expected value */
	struct net_key_val keyval = { 0 };

	keyval.kr_flag = 0;
	keyval.kr_phase = 0;
	memcpy(keyval.val[0], save_cfg.net_key, sizeof(save_cfg.net_key));

	snprintk(validater.key, sizeof(validater.key), "mesh/NetKey/%x",
		 save_cfg.net_idx);
	validater.exp_value = &keyval;
	validater.exp_value_len = sizeof(keyval);
	persistent_values_check(&validater);

	/* check stored addr and devkey matches with expected value */
	struct net_val netval = { 0 };

	netval.primary_addr = save_cfg.addr;
	memcpy(netval.dev_key, save_cfg.dev_key, sizeof(save_cfg.dev_key));

	snprintk(validater.key, sizeof(validater.key), "mesh/Net");
	validater.exp_value = &netval;
	validater.exp_value_len = sizeof(netval);
	persistent_values_check(&validater);

	/* check stored IV index params match with expected value */
	struct iv_val iv;

	iv.iv_index = 0;
	iv.iv_update = 0;
	iv.iv_duration = 96;    /* this is as per the mesh stack logic */

	snprintk(validater.key, sizeof(validater.key), "mesh/IV");
	validater.exp_value = &iv;
	validater.exp_value_len = sizeof(iv);
	persistent_values_check(&validater);

	PASS();
}

static void test_provisioning_ld_check_load(void)
{
	int err;

	mount_settings_area();
	settings_subsys_init();

	/* provision the device */
	/* add Netkey with index 0x07, key[16] = {0xAA}, kr=0 */
	char keypath[20];
	struct net_key_val key = { 0 };

	key.kr_flag = 0;
	key.kr_phase = 0;
	memcpy(&key.val[0], load_cfg.net_key, 16);

	snprintk(keypath, sizeof(keypath), "bt/mesh/NetKey/%x", load_cfg.net_idx);
	err = settings_save_one(keypath, &key, sizeof(key));
	if (err) {
		FAIL("Settings save failed %d", err);
	}

	/* set IV */
	struct iv_val iv;

	iv.iv_index = 1111;
	iv.iv_update = 0;
	iv.iv_duration = 5;
	err = settings_save_one("bt/mesh/IV", &iv, sizeof(iv));
	if (err) {
		FAIL("Settings save failed %d", err);
	}

	/* set addr and devkey */
	struct net_val net;

	net.primary_addr = load_cfg.addr;
	memcpy(net.dev_key, load_cfg.dev_key, 16);
	err = settings_save_one("bt/mesh/Net", &net, sizeof(net));
	if (err) {
		FAIL("Settings save failed %d", err);
	}

	/* set the config and initialize mesh */
	bt_mesh_test_cfg_set(&load_cfg, WAIT_TIME);
	bt_mesh_test_setup(false);


	/* Verify: */
	/* explicitly verify that the keys resolves for a given addr and net_idx */
	struct bt_mesh_msg_ctx ctx;
	struct bt_mesh_net_tx tx = { .ctx = &ctx };
	const uint8_t *dkey;
	uint8_t aid;

	tx.ctx->addr = load_cfg.addr;
	tx.ctx->net_idx = load_cfg.net_idx;
	tx.ctx->app_idx = BT_MESH_KEY_DEV_REMOTE;

	err = bt_mesh_keys_resolve(tx.ctx, &tx.sub, &dkey, &aid);
	if (err) {
		FAIL("Failed to resolve keys");
	}

	if (memcmp(dkey, load_cfg.dev_key, sizeof(load_cfg.dev_key))) {
		FAIL("Resolved dev_key does not match");
	}

	if (memcmp(tx.sub->keys[0].net, load_cfg.net_key, sizeof(load_cfg.net_key))) {
		FAIL("Resolved net_key does not match");
	}

	/* send TTL Get to verify Tx/Rx path works with loaded config */
	uint8_t ttl;

	err = bt_mesh_cfg_ttl_get(load_cfg.net_idx, load_cfg.addr, &ttl);
	if (err) {
		FAIL("Failed to read ttl value");
	}

	/* verify IV index state */
	if (bt_mesh.iv_index != iv.iv_index ||
	    bt_mesh.ivu_duration != iv.iv_duration ||
	    atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS)) {
		FAIL("IV loading verification failed");
	}

	PASS();
}

#define TEST_CASE(role, name, description)		   \
	{						   \
		.test_id = "persistence_" #role "_" #name, \
		.test_descr = description,		   \
		.test_post_init_f = test_##role##_init,	   \
		.test_tick_f = bt_mesh_test_timeout,	   \
		.test_main_f = test_##role##_##name,	   \
	}

static const struct bst_test_instance test_persistence[] = {
	TEST_CASE(provisioning_sv, check_save, "Verify saved provisioning data"),
	TEST_CASE(provisioning_ld, check_load, "Preload provisioning data and verify"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_persistence_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_persistence);
}
