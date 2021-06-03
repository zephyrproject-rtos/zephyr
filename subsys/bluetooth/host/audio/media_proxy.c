/*  Media proxy */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "media_proxy.h"
#include "media_proxy_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MEDIA_PROXY)
#define LOG_MODULE_NAME media_proxy
#include "common/log.h"


/* Media player */
struct player {
	struct media_proxy_pl_calls *calls;
};

/* Synchronous controller - controller using the synchronous API */
struct scontroller {
	struct media_proxy_sctrl_cbs *cbs;
};

/* Media proxy */
struct mprx {
	struct scontroller sctrlr;
	struct player player;
};

static struct mprx mprx = { 0 };

/* Controller calls ***********************************************************/


int media_proxy_sctrl_register(struct media_proxy_sctrl_cbs *sctrl_cbs)
{
	mprx.sctrlr.cbs = sctrl_cbs;
	return 0;
};

char *media_proxy_sctrl_player_name_get(void)
{
	/* TODO: Add check for whether function pointer is non-NULL everywhere */
	return mprx.player.calls->player_name_get();
}

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_sctrl_icon_id_get(void)
{
	return mprx.player.calls->icon_id_get();
}
#endif /* CONFIG_BT_OTS */

char *media_proxy_sctrl_icon_url_get(void)
{
	return mprx.player.calls->icon_url_get();
}

char *media_proxy_sctrl_track_title_get(void)
{
	return mprx.player.calls->track_title_get();
}

int32_t media_proxy_sctrl_track_duration_get(void)
{
	return mprx.player.calls->track_duration_get();
}

int32_t media_proxy_sctrl_track_position_get(void)
{
	return mprx.player.calls->track_position_get();
}

void media_proxy_sctrl_track_position_set(int32_t position)
{
	mprx.player.calls->track_position_set(position);
}

int8_t media_proxy_sctrl_playback_speed_get(void)
{
	return mprx.player.calls->playback_speed_get();
}

void media_proxy_sctrl_playback_speed_set(int8_t speed)
{
	mprx.player.calls->playback_speed_set(speed);
}

int8_t media_proxy_sctrl_seeking_speed_get(void)
{
	return mprx.player.calls->seeking_speed_get();
}

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_sctrl_track_segments_id_get(void)
{
	return mprx.player.calls->track_segments_id_get();
}

uint64_t media_proxy_sctrl_current_track_id_get(void)
{
	return mprx.player.calls->current_track_id_get();
}

void media_proxy_sctrl_current_track_id_set(uint64_t id)
{
	mprx.player.calls->current_track_id_set(id);
}

uint64_t media_proxy_sctrl_next_track_id_get(void)
{
	return mprx.player.calls->next_track_id_get();
}

void media_proxy_sctrl_next_track_id_set(uint64_t id)
{
	mprx.player.calls->next_track_id_set(id);
}

uint64_t media_proxy_sctrl_current_group_id_get(void)
{
	return mprx.player.calls->current_group_id_get();
}

void media_proxy_sctrl_current_group_id_set(uint64_t id)
{
	mprx.player.calls->current_group_id_set(id);
}

uint64_t media_proxy_sctrl_parent_group_id_get(void)
{
	return mprx.player.calls->parent_group_id_get();
}
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_sctrl_playing_order_get(void)
{
	return mprx.player.calls->playing_order_get();
}

void media_proxy_sctrl_playing_order_set(uint8_t order)
{
	mprx.player.calls->playing_order_set(order);
}

uint16_t media_proxy_sctrl_playing_orders_supported_get(void)
{
	return mprx.player.calls->playing_orders_supported_get();
}

uint8_t media_proxy_sctrl_media_state_get(void)
{
	return mprx.player.calls->media_state_get();
}

void media_proxy_sctrl_operation_set(struct mpl_op_t operation)
{
	mprx.player.calls->operation_set(operation);
}

uint32_t media_proxy_sctrl_operations_supported_get(void)
{
	return mprx.player.calls->operations_supported_get();
}

#ifdef CONFIG_BT_OTS
void media_proxy_sctrl_search_set(struct mpl_search_t search)
{
	mprx.player.calls->search_set(search);
}

uint64_t media_proxy_sctrl_search_results_id_get(void)
{
	return mprx.player.calls->search_results_id_get();
}
void media_proxy_sctrl_search_results_id_cb(uint64_t id);
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_sctrl_content_ctrl_id_get(void)
{
	return mprx.player.calls->content_ctrl_id_get();
}

/* Player calls *******************************************/

int media_proxy_pl_register(struct media_proxy_pl_calls *pl_calls)
{
	mprx.player.calls = pl_calls;
	return 0;
};

/* Player callbacks ********************************/

void media_proxy_pl_track_changed_cb(void)
{
	mprx.sctrlr.cbs->track_changed();
}

void media_proxy_pl_track_title_cb(char *title)
{
	mprx.sctrlr.cbs->track_title(title);
}

void media_proxy_pl_track_duration_cb(int32_t duration)
{
	mprx.sctrlr.cbs->track_duration(duration);
}

void media_proxy_pl_track_position_cb(int32_t position)
{
	mprx.sctrlr.cbs->track_position(position);
}

void media_proxy_pl_playback_speed_cb(int8_t speed)
{
	mprx.sctrlr.cbs->playback_speed(speed);
}

void media_proxy_pl_seeking_speed_cb(int8_t speed)
{
	mprx.sctrlr.cbs->seeking_speed(speed);
}

#ifdef CONFIG_BT_OTS
void media_proxy_pl_current_track_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->current_track_id(id);
}

void media_proxy_pl_next_track_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->next_track_id(id);
}

void media_proxy_pl_current_group_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->current_group_id(id);
}

void media_proxy_pl_parent_group_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->parent_group_id(id);
}
#endif /* CONFIG_BT_OTS */

void media_proxy_pl_playing_order_cb(uint8_t order)
{
	mprx.sctrlr.cbs->playing_order(order);
}

void media_proxy_pl_media_state_cb(uint8_t state)
{
	mprx.sctrlr.cbs->media_state(state);
}

void media_proxy_pl_operation_cb(struct mpl_op_ntf_t op_ntf)
{
	mprx.sctrlr.cbs->operation(op_ntf);
}

void media_proxy_pl_operations_supported_cb(uint32_t operations)
{
	mprx.sctrlr.cbs->operations_supported(operations);
}

#ifdef CONFIG_BT_OTS
void media_proxy_pl_search_cb(uint8_t result_code)
{
	mprx.sctrlr.cbs->search(result_code);
}

void media_proxy_pl_search_results_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->search_results_id(id);
}
#endif /* CONFIG_BT_OTS */
