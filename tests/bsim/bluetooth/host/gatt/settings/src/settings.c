/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main.h"

#include <stdio.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include "zephyr/types.h"
#include "errno.h"
#include "argparse.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(settings_backend, 3);

#define SETTINGS_FILE setting_file
#define SETTINGS_FILE_TMP setting_file_tmp

#define ENTRY_LEN_SIZE (4)
#define ENTRY_NAME_MAX_LEN (SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN)
#define ENTRY_VAL_MAX_LEN (SETTINGS_MAX_VAL_LEN * 2)
#define READ_LEN_MAX (ENTRY_VAL_MAX_LEN + ENTRY_NAME_MAX_LEN + 2)

struct line_read_ctx {
	int len;
	const uint8_t *val;
};

static char setting_file[50];
static char setting_file_tmp[sizeof(setting_file) + 1];

static int entry_check_and_copy(FILE *fin, FILE *fout, const char *name)
{
	char line[READ_LEN_MAX + 1];
	char name_tmp[strlen(name) + 2];

	snprintk(name_tmp, sizeof(name_tmp), "%s=", name);

	while (fgets(line, sizeof(line), fin) == line) {
		if (strstr(line, name_tmp) != NULL) {
			return 0;
		}

		if (fputs(line, fout) < 0) {
			return -1;
		}
	};

	return 0;
}

static ssize_t settings_line_read_cb(void *cb_arg, void *data, size_t len)
{
	struct line_read_ctx *valctx = (struct line_read_ctx *)cb_arg;

	if ((valctx->len / 2) > len) {
		return -ENOBUFS;
	}

	uint8_t *n = (uint8_t *) data;

	len = valctx->len / 2;

	for (int i = 0; i < len; i++, n++) {
		if (sscanf(&valctx->val[i * 2], "%2hhx", n) != 1) {
			return 0;
		};
	}

	return len;
}

static int settings_custom_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	FILE *fp = fopen(SETTINGS_FILE, "r+");

	if (fp == NULL) {
		LOG_INF("Settings file doesn't exist, will create it");
		return -1;
	}

	if (fseek(fp, 0, SEEK_SET) < 0) {
		return -1;
	}

	int vallen;
	char line[READ_LEN_MAX + 1];

	while (fgets(line, sizeof(line), fp) == line) {
		/* check for matching subtree */
		if (arg->subtree != NULL && !strstr(line, arg->subtree)) {
			continue;
		}

		char *pos = strchr(line, '=');

		if (pos <= line) {
			return -1;
		}

		vallen = strlen(line) - (pos - line) - 2;
		LOG_DBG("loading entry: %s", line);

		struct line_read_ctx valctx;

		valctx.len = vallen;
		valctx.val = pos + 1;
		int err = settings_call_set_handler(line, vallen / 2, settings_line_read_cb,
						    &valctx, arg);

		if (err < 0) {
			return err;
		}
	};

	return fclose(fp);
}

/* Entries are saved to optimize readability of the settings file for test development and
 * debugging purposes. Format:
 * <entry-key>=<entry-value-hex-str>\n
 */
static int settings_custom_save(struct settings_store *cs, const char *name,
				const char *value, size_t val_len)
{
	FILE *fcur = fopen(SETTINGS_FILE, "r+");
	FILE *fnew = NULL;

	if (fcur == NULL) {
		fcur = fopen(SETTINGS_FILE, "w");
	} else {
		fnew = fopen(SETTINGS_FILE_TMP, "w");
		if (fnew == NULL) {
			LOG_ERR("Failed to create temporary file %s", SETTINGS_FILE_TMP);
			return -1;
		}
	}

	if (fcur == NULL) {
		LOG_ERR("Failed to create settings file: %s", SETTINGS_FILE);
		return -1;
	}

	if (strlen(name) > ENTRY_NAME_MAX_LEN || val_len > SETTINGS_MAX_VAL_LEN) {
		return -1;
	}

	if (fnew != NULL) {
		if (entry_check_and_copy(fcur, fnew, name) < 0) {
			return -1;
		}
	}

	if (val_len) {
		char bufvname[ENTRY_NAME_MAX_LEN + ENTRY_LEN_SIZE + 3];

		snprintk(bufvname, sizeof(bufvname), "%s=", name);
		if (fputs(bufvname, fnew != NULL ? fnew : fcur) < 0) {
			return -1;
		}

		char bufval[ENTRY_VAL_MAX_LEN + 2] = {};
		size_t valcnt = 0;

		while (valcnt < (val_len * 2)) {
			valcnt += snprintk(&bufval[valcnt], 3, "%02x",
					   (uint8_t)value[valcnt / 2]);
		};

		/* helps in making settings file readable */
		bufval[valcnt++] = '\n';
		bufval[valcnt] = 0;

		LOG_DBG("writing to disk");

		if (fputs(bufval, fnew != NULL ? fnew : fcur) < 0) {
			return -1;
		}
	}

	if (fnew != NULL) {
		entry_check_and_copy(fcur, fnew, name);
	}

	fclose(fcur);

	if (fnew != NULL) {
		fclose(fnew);

		remove(SETTINGS_FILE);
		rename(SETTINGS_FILE_TMP, SETTINGS_FILE);
	}

	return 0;
}

/* custom backend interface */
static struct settings_store_itf settings_custom_itf = {
	.csi_load = settings_custom_load,
	.csi_save = settings_custom_save,
};

/* custom backend node */
static struct settings_store settings_custom_store = {
	.cs_itf = &settings_custom_itf
};

int settings_backend_init(void)
{
	snprintf(setting_file, sizeof(setting_file), "%s_%s.log", get_simid(), get_settings_file());
	snprintf(setting_file_tmp, sizeof(setting_file_tmp), "~%s", setting_file);

	LOG_INF("file path: %s", SETTINGS_FILE);

	/* register custom backend */
	settings_dst_register(&settings_custom_store);
	settings_src_register(&settings_custom_store);
	return 0;
}

void settings_test_backend_clear(void)
{
	snprintf(setting_file, sizeof(setting_file), "%s_%s.log", get_simid(), get_settings_file());

	if (remove(setting_file)) {
		LOG_INF("error deleting file: %s", setting_file);
	}
}
