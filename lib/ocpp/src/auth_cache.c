/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

cache_list_init
cache_find_idtag_info
cache_list_remove
cache_list_add

#if 0
static int cfg_file_update(ocpp_cfg_info_t *cfg)
{
	struct fs_file_t fp;
	int ret;
	int i, size;

	if (cfg->type >= KEY_TYPE_STR && ! cfg->val.str) {
		return -EINVAL;
	}

	ret = fs_open(&fp, cfg->skey, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		return ret;
	}

	if (cfg->type < KEY_TYPE_STR) {
		size = cfg->type;
	} else {
		size = strlen(cfg->val.str) + 1;
	}

	ret = fs_write(&fp, &cfg->val, size);
	fs_sync(&fp);
	fclose(&fp);

	return ret;
}
#endif
#if 0
	/* update in persist */
	cfg_file_update(&cached_key_cfg[key]);
#endif

int ocpp_cfg_init(void)
{
	int i;

#if 0
	char buf[500];
	struct fs_file_t fp;
	int ret;
	ocpp_cfg_info_t *cfg = cached_key_cfg;

	for (i = 0; i < OCPP_CFG_END; i++) {
		ret = fs_open(&fp, cfg[i].skey, FS_O_READ);
		if (ret < 0) {
			continue;
		}

		ret = fs_read(&fp, buf, sizeof(buf));
		if (ret <= 0) {
			fs_close(&fp);
			continue;
		}

		if (cfg[i].type < KEY_TYPE_STR) {
			memcpy(&cfg[i].val, buf, ret);
		} else {
			cfg[i].val.str = strdup(buf);
		}

		fclose(&fp);
	}

#endif
	return 0;
}
