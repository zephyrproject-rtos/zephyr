/*
 * @file
 * @brief SoftAP mode handling
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

K_THREAD_STACK_DEFINE(wifimgr_ap_wq_stack, WIFIMGR_WORKQUEUE_STACK_SIZE);

static int wifimgr_ap_stop(void *handle);
static int wifimgr_ap_close(void *handle);

void wifimgr_ap_event_timeout(struct wifimgr_delayed_work *dwork)
{
	struct wifimgr_state_machine *ap_sm =
	    container_of(dwork, struct wifimgr_state_machine, dwork);
	unsigned int expected_evt;

	/* Notify the external caller */
	switch (ap_sm->cur_cmd) {
	case WIFIMGR_CMD_DEL_STA:
		expected_evt = WIFIMGR_EVT_NEW_STATION;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		break;
	default:
		break;
	}
}

static int wifimgr_ap_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	if (!memiszero(conf, sizeof(struct wifi_config)))
		return wifimgr_config_clear(conf, WIFIMGR_SETTING_AP_PATH);

	return wifimgr_config_save(conf, WIFIMGR_SETTING_AP_PATH);
}

static int wifimgr_ap_get_config(void *handle)
{
	struct wifi_config *conf = (struct wifi_config *)handle;

	/* Load config form non-volatile memory */
	memset(conf, 0, sizeof(struct wifi_config));
	wifimgr_config_load(conf, WIFIMGR_SETTING_AP_PATH);

	return 0;
}

static int wifimgr_ap_get_capa(void *handle)
{
	/* Nothing TODO */

	return 0;
}

static int wifimgr_ap_get_status(void *handle)
{
	struct wifi_status *sts = (struct wifi_status *)handle;
	struct wifi_manager *mgr =
	    container_of(sts, struct wifi_manager, ap_sts);
	struct wifimgr_state_machine *sm = &mgr->ap_sm;

	sts->state = sm_ap_query(sm);

	return 0;
}

static int wifimgr_ap_del_station(void *handle)
{
	struct wifimgr_set_mac_acl *set_acl =
	    (struct wifimgr_set_mac_acl *)handle;
	struct wifi_manager *mgr =
	    container_of(set_acl, struct wifi_manager, set_acl);
	int ret;

	if (is_zero_ether_addr(set_acl->mac))
		return -EINVAL;

	ret = wifi_drv_del_station(mgr->ap_iface, set_acl->mac);
	if (ret)
		wifimgr_err("failed to deauth! %d\n", ret);

	return ret;
}

static struct wifimgr_mac_node *search_mac(struct wifimgr_mac_list *mac_list,
					   char *mac)
{
	struct wifimgr_mac_node *mac_node;

	/* Loop through list to find the corresponding event */
	wifimgr_list_for_each_entry(mac_node, &mac_list->list,
				    struct wifimgr_mac_node, node) {
		if (!memcmp(mac_node->mac, mac, WIFI_MAC_ADDR_LEN))
			return mac_node;
	}

	return NULL;
}

static int wifimgr_ap_set_mac_acl(void *handle)
{
	struct wifimgr_set_mac_acl *set_acl =
	    (struct wifimgr_set_mac_acl *)handle;
	struct wifi_manager *mgr =
	    container_of(set_acl, struct wifi_manager, set_acl);
	struct wifi_status *sts = &mgr->ap_sts;
	struct wifimgr_mac_list *assoc_list = &mgr->assoc_list;
	struct wifimgr_mac_list *mac_acl = &mgr->mac_acl;
	struct wifimgr_mac_node *marked_sta = NULL;
	char (*acl_mac_addrs)[WIFI_MAC_ADDR_LEN] = NULL;
	char drv_subcmd = 0;
	unsigned char nr_acl = 0;
	int ret = 0;

	if (is_zero_ether_addr(set_acl->mac)) {
		wifimgr_info("%s (" MACSTR ")\n", __func__,
			     MAC2STR(set_acl->mac));
		return -EINVAL;
	}

	/* Check parmas and prepare ACL table for driver */
	switch (set_acl->subcmd) {
	case WIFI_MAC_ACL_BLOCK:
		if ((mac_acl->nr + nr_acl) >
		    mgr->ap_capa.ap.max_acl_mac_addrs) {
			wifimgr_warn("Max number of ACL reached!");
			return -ENOSPC;
		}
		nr_acl = 1;

		marked_sta = search_mac(mac_acl, set_acl->mac);
		if (marked_sta) {
			wifimgr_info("Duplicate ACL item found!\n");
			return -EEXIST;
		}

		drv_subcmd = WIFI_DRV_BLACKLIST_ADD;
		acl_mac_addrs = (char (*)[WIFI_MAC_ADDR_LEN])set_acl->mac;
		break;
	case WIFI_MAC_ACL_UNBLOCK:
		if (!mac_acl->nr) {
			wifimgr_warn("Empty ACL!");
			return -ENOENT;
		}
		nr_acl = 1;

		marked_sta = search_mac(mac_acl, set_acl->mac);
		if (!marked_sta) {
			wifimgr_info("No matches found!\n");
			return -ENOENT;
		}

		drv_subcmd = WIFI_DRV_BLACKLIST_DEL;
		acl_mac_addrs = (char (*)[WIFI_MAC_ADDR_LEN])set_acl->mac;
		break;
	case WIFI_MAC_ACL_BLOCK_ALL:
		if (!assoc_list->nr) {
			wifimgr_warn("Empty Station List!");
			return -ENOENT;
		}
		nr_acl = assoc_list->nr;

		if (mac_acl->nr + nr_acl > mgr->ap_capa.ap.max_acl_mac_addrs) {
			wifimgr_warn("Max number of ACL reached!");
			return -ENOSPC;
		}

		drv_subcmd = WIFI_DRV_BLACKLIST_ADD;
		acl_mac_addrs = sts->u.ap.sta_mac_addrs;
		break;
	case WIFI_MAC_ACL_UNBLOCK_ALL:
		if (!mac_acl->nr) {
			wifimgr_warn("Empty ACL!");
			return -ENOENT;
		}
		nr_acl = mac_acl->nr;
		drv_subcmd = WIFI_DRV_BLACKLIST_DEL;
		acl_mac_addrs = sts->u.ap.acl_mac_addrs;
		break;
	default:
		return -EINVAL;
	}

	/* Set ACL */
	ret =
	    wifi_drv_set_mac_acl(mgr->ap_iface, drv_subcmd, nr_acl,
				 acl_mac_addrs);
	if (ret) {
		wifimgr_err("failed to set MAC ACL! %d\n", ret);
	} else {
		/* Update ACL list */
		switch (set_acl->subcmd) {
		case WIFI_MAC_ACL_BLOCK:
			marked_sta = malloc(sizeof(struct wifimgr_mac_node));
			if (!marked_sta) {
				ret = -ENOMEM;
				break;
			}
			memcpy(marked_sta->mac, set_acl->mac,
			       WIFI_MAC_ADDR_LEN);
			wifimgr_list_append(&mac_acl->list, &marked_sta->node);
			mac_acl->nr++;
			wifimgr_info("Block ");
			break;
		case WIFI_MAC_ACL_UNBLOCK:
			wifimgr_list_remove(&mac_acl->list, &marked_sta->node);
			free(marked_sta);
			mac_acl->nr--;
			wifimgr_info("Unblock ");
			break;
		case WIFI_MAC_ACL_BLOCK_ALL:
			wifimgr_list_merge(&mac_acl->list, &assoc_list->list);
			mac_acl->nr = assoc_list->nr;
			assoc_list->nr = 0;
			wifimgr_info("Block ");
			break;
		case WIFI_MAC_ACL_UNBLOCK_ALL:
			wifimgr_list_free(&mac_acl->list);
			mac_acl->nr = 0;
			wifimgr_info("Unblock ");
			break;
		default:
			return -EINVAL;
		}

		int i;

		/* Update ACL table */
		marked_sta = (struct wifimgr_mac_node *)
		    wifimgr_list_peek_head(&mac_acl->list);
		for (i = 0; (i < mac_acl->nr) || (marked_sta != NULL); i++) {
			memcpy(sts->u.ap.acl_mac_addrs[i], marked_sta->mac,
			       WIFI_MAC_ADDR_LEN);
			marked_sta = (struct wifimgr_mac_node *)
			    wifimgr_list_peek_next(&marked_sta->node);
		}
		sts->u.ap.nr_acl = mac_acl->nr;

		if (is_broadcast_ether_addr(set_acl->mac))
			wifimgr_info("all stations!\n");
		else
			wifimgr_info("(" MACSTR ")\n", MAC2STR(set_acl->mac));
	}

	return ret;
}

static int wifimgr_ap_new_station_event(void *arg)
{
	struct wifimgr_ap_event *ap_evt = (struct wifimgr_ap_event *)arg;
	struct wifi_drv_new_station_evt *new_sta = &ap_evt->u.new_sta;
	struct wifi_manager *mgr =
	    container_of(ap_evt, struct wifi_manager, ap_evt);
	struct wifi_status *sts = &mgr->ap_sts;
	struct wifimgr_mac_list *assoc_list = &mgr->assoc_list;
	struct wifimgr_mac_node *assoc_sta;
	struct wifimgr_notifier_chain *chain;
	bool pending = false;

	if (is_zero_ether_addr(new_sta->mac)
	    || is_broadcast_ether_addr(new_sta->mac)) {
		wifimgr_err("invalid station MAC!");
		return -EINVAL;
	}

	if (new_sta->is_connect) {
		if (!assoc_list->nr)
			cmd_processor_add_sender(&mgr->prcs,
						 WIFIMGR_CMD_DEL_STA,
						 WIFIMGR_CMD_TYPE_EXCHANGE,
						 wifimgr_ap_del_station,
						 &mgr->set_acl);

		if (assoc_list->nr >= mgr->ap_capa.ap.max_ap_assoc_sta) {
			wifimgr_warn("Max number of stations reached!");
			return 0;
		}

		assoc_sta = search_mac(assoc_list, new_sta->mac);
		if (assoc_sta) {
			wifimgr_info("Duplicate stations found!\n");
			return 0;
		}

		assoc_sta = malloc(sizeof(struct wifimgr_mac_node));
		if (!assoc_sta)
			return -ENOMEM;
		memcpy(assoc_sta->mac, new_sta->mac, WIFI_MAC_ADDR_LEN);
		wifimgr_list_append(&assoc_list->list, &assoc_sta->node);
		assoc_list->nr++;
		pending = true;
		chain = &mgr->ap_ctrl.new_sta_chain;
	} else {
		if (!assoc_list->nr) {
			wifimgr_warn("No stations connected!");
			sts->u.ap.nr_sta = assoc_list->nr;
			return 0;
		}

		assoc_sta = search_mac(assoc_list, new_sta->mac);
		if (!assoc_sta) {
			wifimgr_info("No matches found!\n");
			return 0;
		}

		wifimgr_list_remove(&assoc_list->list, &assoc_sta->node);
		free(assoc_sta);
		assoc_list->nr--;
		pending = true;
		chain = &mgr->ap_ctrl.sta_leave_chain;

		if (!assoc_list->nr)
			cmd_processor_remove_sender(&mgr->prcs,
						    WIFIMGR_CMD_DEL_STA);
	}

	if (pending == true) {
		int i;

		/* Update the associated station table */
		assoc_sta = (struct wifimgr_mac_node *)
		    wifimgr_list_peek_head(&assoc_list->list);
		for (i = 0; (i < assoc_list->nr) || (assoc_sta != NULL); i++) {
			memcpy(sts->u.ap.sta_mac_addrs[i], assoc_sta->mac,
			       WIFI_MAC_ADDR_LEN);
			assoc_sta = (struct wifimgr_mac_node *)
			    wifimgr_list_peek_next(&assoc_sta->node);
		}
		sts->u.ap.nr_sta = assoc_list->nr;

		/* Notify the external caller */
		wifimgr_ctrl_evt_new_station(&mgr->sta_ctrl, chain,
					     new_sta->is_connect, new_sta->mac);
	}

	return 0;
}

static int wifimgr_ap_start(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	union wifi_drv_capa *capa = &mgr->ap_capa;
	struct wifi_config *conf = &mgr->ap_conf;
	struct wifimgr_mac_list *assoc_list = &mgr->assoc_list;
	struct wifimgr_mac_list *mac_acl = &mgr->mac_acl;
	struct wifi_status *sts = &mgr->ap_sts;
	char *ssid = NULL;
	char *psk = NULL;
	char psk_len = 0;
	char wpa_psk[WIFIMGR_PSK_LEN];
	int size;
	int ret;

	ret = evt_listener_add_receiver(&mgr->lsnr,
					WIFIMGR_EVT_NEW_STATION, false,
					wifimgr_ap_new_station_event,
					&mgr->ap_evt);
	if (ret)
		return ret;

	if (!memiszero(conf, sizeof(struct wifi_config))) {
		wifimgr_info("No AP config found!\n");
		return -EINVAL;
	}
	/* Initialize the associated station table */
	if (!capa->ap.max_ap_assoc_sta)
		capa->ap.max_ap_assoc_sta = WIFIMGR_MAX_STA_NR;
	size = capa->ap.max_ap_assoc_sta * WIFI_MAC_ADDR_LEN;
	sts->u.ap.sta_mac_addrs = malloc(size);
	if (!sts->u.ap.sta_mac_addrs)
		return -ENOMEM;
	sts->u.ap.nr_sta = 0;
	memset(sts->u.ap.sta_mac_addrs, 0, size);

	/* Initialize the MAC ACL table */
	if (!capa->ap.max_acl_mac_addrs)
		capa->ap.max_acl_mac_addrs = WIFIMGR_MAX_STA_NR;
	size = capa->ap.max_acl_mac_addrs * WIFI_MAC_ADDR_LEN;
	sts->u.ap.acl_mac_addrs = malloc(size);
	if (!sts->u.ap.acl_mac_addrs) {
		free(sts->u.ap.sta_mac_addrs);
		return -ENOMEM;
	}
	sts->u.ap.nr_acl = 0;
	memset(sts->u.ap.acl_mac_addrs, 0, size);

	/* Initialize the associated station list */
	wifimgr_list_init(&assoc_list->list);
	assoc_list->nr = 0;

	/* Initialize the MAC ACL list */
	wifimgr_list_init(&mac_acl->list);
	mac_acl->nr = 0;

	if (strlen(conf->ssid))
		ssid = conf->ssid;
	if (strlen(conf->passphrase)) {
		ret = pbkdf2_sha1(conf->passphrase, ssid, WIFIMGR_PSK_ITER,
				  wpa_psk, WIFIMGR_PSK_LEN);
		if (ret) {
			wifimgr_err("failed to calculate PSK! %d\n", ret);
			return ret;
		}
		psk = wpa_psk;
		psk_len = WIFIMGR_PSK_LEN;
	}
	ret = wifi_drv_start_ap(mgr->ap_iface, ssid, psk, psk_len,
				conf->channel, conf->ch_width);

	if (ret) {
		wifimgr_err("failed to start AP! %d\n", ret);
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_NEW_STATION);
		free(sts->u.ap.sta_mac_addrs);
		free(sts->u.ap.acl_mac_addrs);
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_ap_stop, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_SET_MAC_ACL,
				 WIFIMGR_CMD_TYPE_SET,
				 wifimgr_ap_set_mac_acl, &mgr->set_acl);

	/* TODO: Start DHCP server */
	wifimgr_ap_led_on();
	wifimgr_info("start AP!\n");
	return ret;
}

static int wifimgr_ap_stop(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifi_status *sts = &mgr->ap_sts;
	int ret;

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	/* Deinitialize the MAC ACL list */
	wifimgr_list_free(&mgr->mac_acl.list);
	/* Deinitialize the associated station list */
	wifimgr_list_free(&mgr->assoc_list.list);
	/* Deinitialize the MAC ACL table */
	free(sts->u.ap.acl_mac_addrs);
	sts->u.ap.acl_mac_addrs = NULL;
	sts->u.ap.nr_acl = 0;
	/* Deinitialize the associated station table */
	free(sts->u.ap.sta_mac_addrs);
	sts->u.ap.sta_mac_addrs = NULL;
	sts->u.ap.nr_sta = 0;

	/* Deauth all stations before leaving */
	memset(mgr->set_acl.mac, 0xff, sizeof(mgr->set_acl.mac));
	ret = wifi_drv_del_station(mgr->ap_iface, mgr->set_acl.mac);
	if (ret)
		wifimgr_warn("failed to deauth all stations!\n");

	ret = wifi_drv_stop_ap(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to stop AP!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_SET_MAC_ACL);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_ap_start, mgr);

	wifimgr_ap_led_off();
	wifimgr_info("stop AP!\n");
	return ret;
}

static int wifimgr_ap_open(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_open(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to open AP!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_ap_close, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_ap_start, mgr);

	wifimgr_info("open AP!\n");
	return ret;
}

static int wifimgr_ap_close(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_close(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to close AP!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_ap_open, mgr);

	wifimgr_info("close AP!\n");
	return ret;
}

static int wifimgr_ap_drv_init(struct wifi_manager *mgr)
{
	char *devname = WIFIMGR_DEV_NAME_AP;
	struct net_if *iface = NULL;
	int ret;

	/* Initialize driver interface */
	iface = wifi_drv_init(devname);
	if (!iface) {
		wifimgr_err("failed to init WiFi STA driver!\n");
		return -ENODEV;
	}
	mgr->ap_iface = iface;

	/* Get MAC address */
	ret = wifi_drv_get_mac(iface, mgr->ap_sts.own_mac);
	if (ret)
		wifimgr_warn("failed to get Own MAC!\n");

	/* Check driver capability */
	ret = wifi_drv_get_capa(iface, &mgr->ap_capa);
	if (ret)
		wifimgr_warn("failed to get driver capability!");

	wifimgr_info("interface %s(" MACSTR ") initialized!\n", devname,
		     MAC2STR(mgr->ap_sts.own_mac));

	return 0;
}

int wifimgr_ap_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;
	int ret;

	/* Register default AP commands */
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_SET_AP_CONFIG,
				 WIFIMGR_CMD_TYPE_SET,
				 wifimgr_ap_set_config, &mgr->ap_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_CONFIG,
				 WIFIMGR_CMD_TYPE_GET,
				 wifimgr_ap_get_config, &mgr->ap_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_CAPA,
				 WIFIMGR_CMD_TYPE_GET,
				 wifimgr_ap_get_capa, &mgr->ap_capa);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_STATUS,
				 WIFIMGR_CMD_TYPE_GET,
				 wifimgr_ap_get_status, &mgr->ap_sts);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_OPEN_AP,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_ap_open, mgr);

	/* Initialize AP config */
	ret = wifimgr_settings_init(&mgr->ap_conf, WIFIMGR_SETTING_AP_PATH);
	if (ret)
		wifimgr_warn("failed to init WiFi AP config!\n");

	/* Initialize AP driver */
	ret = wifimgr_ap_drv_init(mgr);
	if (ret) {
		wifimgr_err("failed to init WiFi AP driver!\n");
		return ret;
	}

	/* Initialize AP state machine */
	ret = wifimgr_sm_init(&mgr->ap_sm, wifimgr_ap_event_timeout);
	if (ret)
		wifimgr_err("failed to init WiFi AP state machine!\n");
	wifimgr_create_workqueue(&mgr->ap_sm.dwork.wq, wifimgr_ap_wq_stack);

	/* Initialize AP global control iface */
	wifimgr_ctrl_iface_init(WIFIMGR_IFACE_NAME_AP, &mgr->ap_ctrl);

	return ret;
}

void wifimgr_ap_exit(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;

	/* Deinitialize AP global control */
	wifimgr_ctrl_iface_destroy(WIFIMGR_IFACE_NAME_AP, &mgr->ap_ctrl);

	/* Deinitialize AP state machine */
	wifimgr_sm_exit(&mgr->ap_sm);

	/* Deinitialize AP config */
	wifimgr_config_exit(WIFIMGR_SETTING_AP_PATH);
}
