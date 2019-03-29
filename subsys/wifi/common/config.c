/*
 * @file
 * @brief Configuration handling
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include "wifimgr.h"

static const char *const wifimgr_setting_keynames[] = {
	WIFIMGR_SETTING_NAME_SSID,
	WIFIMGR_SETTING_NAME_BSSID,
	WIFIMGR_SETTING_NAME_PSPHR,
	WIFIMGR_SETTING_NAME_SECURITY,
	WIFIMGR_SETTING_NAME_BAND,
	WIFIMGR_SETTING_NAME_CHANNEL,
	WIFIMGR_SETTING_NAME_CHANNEL_WIDTH,
	WIFIMGR_SETTING_NAME_AUTORUN,
};

static struct wifimgr_settings_map *wifimgr_sta_settings_map;
static struct wifimgr_settings_map *wifimgr_ap_settings_map;
static struct wifimgr_settings_map *settings;
static char *settings_category;

static int wifimgr_settings_set(int argc, char **argv, void *val)
{
	u8_t cnt = ARRAY_SIZE(wifimgr_setting_keynames);
	u8_t i;
	int len;

	if (argc >= 1) {
		for (i = 0; i < cnt; i++) {
			wifimgr_dbg
			    ("argv[%d]:%s, argv[%d]:%s, settings[%d].name:%s\n",
			     argc - 2, argv[argc - 2], argc - 1, argv[argc - 1],
			     i, settings[i].name);

			if (!strcmp(argv[argc - 2], settings_category)
			    && !strcmp(argv[argc - 1], settings[i].name)) {
				if (settings[i].mask)
					continue;

				memset(settings[i].valptr, 0,
				       settings[i].vallen);

				len = settings_val_read_cb(val,
							   settings[i].valptr,
							   settings[i].vallen);
				if (len < 0) {
					wifimgr_err("failed to read value! %d",
						    len);
					return len;
				}

				if (len != settings[i].vallen) {
					wifimgr_err("unexpected value len: %d",
						    len);
					return -EINVAL;
				}

				if ((i >= WIFIMGR_SETTING_ID_SSID)
				    && (i <= WIFIMGR_SETTING_ID_PSPHR)) {
					wifimgr_dbg("val: %s\n",
						    (char *)settings[i].valptr);
				} else if ((i >= WIFIMGR_SETTING_ID_SECURITY) &&
					   (i <=
					    WIFIMGR_SETTING_ID_CHANNEL_WIDTH)) {
					wifimgr_dbg("val: %d\n",
						    *(unsigned char *)
						    settings[i].valptr);
				} else if (i >= WIFIMGR_SETTING_ID_AUTORUN) {
					wifimgr_dbg("val: %d\n",
						    *(unsigned int *)
						    settings[i].valptr);
				}

				break;
			}
		}
	}

	return 0;
}

static struct settings_handler wifimgr_settings_handler = {
	.name = WIFIMGR_SETTING_PATH,
	.h_set = wifimgr_settings_set,
};

static int wifimgr_settings_save_one(u8_t id,
				     struct wifimgr_settings_map *setting,
				     char *path, bool clear)
{
	char abs_path[WIFIMGR_SETTING_NAME_LEN + 1];
	int ret;

	if (setting->mask)
		return 0;

	snprintk(abs_path, sizeof(abs_path), "%s/%s", path, setting->name);
	ret = settings_save_one(abs_path, setting->valptr, setting->vallen);
	if (ret)
		wifimgr_err("failed to save %s! %d\n", abs_path, ret);

	return ret;
}

int wifimgr_settings_save(void *handle, char *path, bool clear)
{
	int cnt = ARRAY_SIZE(wifimgr_setting_keynames);
	int i;
	int ret;

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)
	    && wifimgr_sta_settings_map) {
		settings = wifimgr_sta_settings_map;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)
		   && wifimgr_ap_settings_map) {
		settings = wifimgr_ap_settings_map;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return -ENOENT;
	}

	for (i = 0; i < cnt; i++) {
		ret = wifimgr_settings_save_one(i, &settings[i], path, clear);
		if (ret)
			break;
	}

	return ret;
}

static
void wifimgr_settings_init_one(struct wifimgr_settings_map *setting,
			       const char *name, void *valptr, int vallen,
			       bool mask)
{
	strcpy(setting->name, name);
	setting->valptr = valptr;
	setting->vallen = vallen;
	setting->mask = mask;
}

int wifimgr_settings_init(struct wifi_config *conf, char *path)
{
	int settings_size = sizeof(struct wifimgr_settings_map) *
	    ARRAY_SIZE(wifimgr_setting_keynames);
	bool mask;
	int i = 0;

	/* Allocate memory for settings map */
	settings = malloc(settings_size);
	if (!settings)
		return -ENOMEM;

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)) {
		wifimgr_sta_settings_map = settings;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)) {
		wifimgr_ap_settings_map = settings;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return -ENOENT;
	}

	memset(settings, 0, settings_size);

	/* Initialize SSID setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  conf->ssid, sizeof(conf->ssid), false);
	i++;
	/* Initialize BSSID setting map */
	if (!strcmp(path, WIFIMGR_SETTING_AP_PATH))
		mask = true;

	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  conf->bssid, sizeof(conf->bssid), mask);
	i++;
	/* Initialize Passphrase setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  conf->passphrase, sizeof(conf->passphrase),
				  false);
	i++;
	/* Initialize Security setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->security, sizeof(conf->security),
				  false);
	i++;
	/* Initialize Band setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->band, sizeof(conf->band), false);
	i++;
	/* Initialize Channel setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->channel, sizeof(conf->channel), false);
	i++;
	/* Initialize Channel width setting map */
	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH))
		mask = true;

	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->ch_width, sizeof(conf->ch_width),
				  false);
	i++;
	/* Initialize Autorun setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->autorun, sizeof(conf->autorun), false);

	return 0;
}

int wifimgr_config_load(void *handle, char *path)
{
	int ret;

	settings_category = strstr(path, "/");
	settings_category++;

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)
	    && wifimgr_sta_settings_map) {
		settings = wifimgr_sta_settings_map;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)
		   && wifimgr_ap_settings_map) {
		settings = wifimgr_ap_settings_map;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return -ENOENT;
	}

	ret = settings_load();
	if (ret) {
		wifimgr_err("failed to load config!\n");
		return ret;
	}

	return ret;
}

int wifimgr_config_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		wifimgr_err("failed to init settings subsys! %d\n", ret);
		return ret;
	}

	ret = settings_register(&wifimgr_settings_handler);
	if (ret)
		wifimgr_err("failed to register setting handlers! %d\n", ret);

	return ret;
}

void wifimgr_config_exit(char *path)
{
	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)) {
		settings = wifimgr_sta_settings_map;
		wifimgr_sta_settings_map = NULL;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)) {
		settings = wifimgr_ap_settings_map;
		wifimgr_ap_settings_map = NULL;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return;
	}

	free(settings);
}
