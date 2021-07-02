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
struct media_player {
	struct media_proxy_pl_calls *calls;
	bool   registered;
};

/* Synchronous controller - controller using the synchronous API */
struct scontroller {
	struct media_proxy_sctrl_cbs *cbs;
};

/* Aynchronous controller - controller using the asynchronous API */
struct controller {
	struct media_proxy_ctrl_cbs *cbs;
};
/* Media proxy */
struct mprx {
	/* Controller using the synchronous interface - i.e. a remote controller via
	 * the media control service
	 */
	struct scontroller   sctrlr;

	/* Controller using the callback interface - i.e. upper layer */
	struct controller    ctrlr;

	/* The local media player */
	struct media_player  local_player;
};

static struct mprx mprx = { 0 };

/* Synchronous controller calls ***********************************************/

/* The synchronous controller is the media control service, representing a remote controller.
 * It only gets access to the local player, not remote players.
 * I.e., the calls from the synchronous controller are always routed to the local_player.
 */

int media_proxy_sctrl_register(struct media_proxy_sctrl_cbs *sctrl_cbs)
{
	mprx.sctrlr.cbs = sctrl_cbs;
	return 0;
};

char *media_proxy_sctrl_player_name_get(void)
{
	/* TODO: Add check for whether function pointer is non-NULL everywhere */
	return mprx.local_player.calls->player_name_get();
}

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_sctrl_icon_id_get(void)
{
	return mprx.local_player.calls->icon_id_get();
}
#endif /* CONFIG_BT_OTS */

char *media_proxy_sctrl_icon_url_get(void)
{
	return mprx.local_player.calls->icon_url_get();
}

char *media_proxy_sctrl_track_title_get(void)
{
	return mprx.local_player.calls->track_title_get();
}

int32_t media_proxy_sctrl_track_duration_get(void)
{
	return mprx.local_player.calls->track_duration_get();
}

int32_t media_proxy_sctrl_track_position_get(void)
{
	return mprx.local_player.calls->track_position_get();
}

void media_proxy_sctrl_track_position_set(int32_t position)
{
	mprx.local_player.calls->track_position_set(position);
}

int8_t media_proxy_sctrl_playback_speed_get(void)
{
	return mprx.local_player.calls->playback_speed_get();
}

void media_proxy_sctrl_playback_speed_set(int8_t speed)
{
	mprx.local_player.calls->playback_speed_set(speed);
}

int8_t media_proxy_sctrl_seeking_speed_get(void)
{
	return mprx.local_player.calls->seeking_speed_get();
}

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_sctrl_track_segments_id_get(void)
{
	return mprx.local_player.calls->track_segments_id_get();
}

uint64_t media_proxy_sctrl_current_track_id_get(void)
{
	return mprx.local_player.calls->current_track_id_get();
}

void media_proxy_sctrl_current_track_id_set(uint64_t id)
{
	mprx.local_player.calls->current_track_id_set(id);
}

uint64_t media_proxy_sctrl_next_track_id_get(void)
{
	return mprx.local_player.calls->next_track_id_get();
}

void media_proxy_sctrl_next_track_id_set(uint64_t id)
{
	mprx.local_player.calls->next_track_id_set(id);
}

uint64_t media_proxy_sctrl_current_group_id_get(void)
{
	return mprx.local_player.calls->current_group_id_get();
}

void media_proxy_sctrl_current_group_id_set(uint64_t id)
{
	mprx.local_player.calls->current_group_id_set(id);
}

uint64_t media_proxy_sctrl_parent_group_id_get(void)
{
	return mprx.local_player.calls->parent_group_id_get();
}
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_sctrl_playing_order_get(void)
{
	return mprx.local_player.calls->playing_order_get();
}

void media_proxy_sctrl_playing_order_set(uint8_t order)
{
	mprx.local_player.calls->playing_order_set(order);
}

uint16_t media_proxy_sctrl_playing_orders_supported_get(void)
{
	return mprx.local_player.calls->playing_orders_supported_get();
}

uint8_t media_proxy_sctrl_media_state_get(void)
{
	return mprx.local_player.calls->media_state_get();
}

void media_proxy_sctrl_operation_set(struct mpl_op_t operation)
{
	mprx.local_player.calls->operation_set(operation);
}

uint32_t media_proxy_sctrl_operations_supported_get(void)
{
	return mprx.local_player.calls->operations_supported_get();
}

#ifdef CONFIG_BT_OTS
void media_proxy_sctrl_search_set(struct mpl_search_t search)
{
	mprx.local_player.calls->search_set(search);
}

uint64_t media_proxy_sctrl_search_results_id_get(void)
{
	return mprx.local_player.calls->search_results_id_get();
}
void media_proxy_sctrl_search_results_id_cb(uint64_t id);
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_sctrl_content_ctrl_id_get(void)
{
	return mprx.local_player.calls->content_ctrl_id_get();
}

/* Asynchronous controller calls **********************************************/

int media_proxy_ctrl_register(struct media_proxy_ctrl_cbs *ctrl_cbs)
{
	mprx.ctrlr.cbs = ctrl_cbs;

	if (mprx.local_player.registered) {
		if (mprx.ctrlr.cbs->local_player_instance) {
			mprx.ctrlr.cbs->local_player_instance(&mprx.local_player, 0);
		}
	}

	/* TODO: Return error code if too many controllers registered */
	return 0;
};

int media_proxy_ctrl_player_name_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->player_name_get) {
		char *name = mprx.local_player.calls->player_name_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->player_name) {
			mprx.ctrlr.cbs->player_name(player, 0, name);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_icon_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->icon_id_get) {
		uint64_t id = mprx.local_player.calls->icon_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->icon_id) {
			mprx.ctrlr.cbs->icon_id(player, 0, id);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_icon_url_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->icon_url_get) {
		char *url = mprx.local_player.calls->icon_url_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->icon_url) {
			mprx.ctrlr.cbs->icon_url(player, 0, url);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_track_title_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->track_title_get) {
		char *title = mprx.local_player.calls->track_title_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_title) {
			mprx.ctrlr.cbs->track_title(player, 0, title);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_track_duration_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->track_duration_get) {
		int32_t duration = mprx.local_player.calls->track_duration_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_duration) {
			mprx.ctrlr.cbs->track_duration(player, 0, duration);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_track_position_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->track_position_get) {
		int32_t position = mprx.local_player.calls->track_position_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_position) {
			mprx.ctrlr.cbs->track_position(player, 0, position);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_track_position_set(struct media_player *player, int32_t position)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->track_position_set) {
		mprx.local_player.calls->track_position_set(position);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_playback_speed_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->playback_speed_get) {
		int8_t speed = mprx.local_player.calls->playback_speed_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playback_speed) {
			mprx.ctrlr.cbs->playback_speed(player, 0, speed);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_playback_speed_set(struct media_player *player, int8_t speed)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->playback_speed_set) {
		mprx.local_player.calls->playback_speed_set(speed);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_seeking_speed_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->seeking_speed_get) {
		int8_t speed = mprx.local_player.calls->seeking_speed_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->seeking_speed) {
			mprx.ctrlr.cbs->seeking_speed(player, 0, speed);

		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_track_segments_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->track_segments_id_get) {
		uint64_t id = mprx.local_player.calls->track_segments_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_segments_id) {
			mprx.ctrlr.cbs->track_segments_id(player, 0, id);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_current_track_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->current_track_id_get) {
		uint64_t id = mprx.local_player.calls->current_track_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_track_id) {
			mprx.ctrlr.cbs->current_track_id(player, 0, id);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_current_track_id_set(struct media_player *player, uint64_t id)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->current_track_id_set) {
		mprx.local_player.calls->current_track_id_set(id);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_next_track_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->next_track_id_get) {
		uint64_t id = mprx.local_player.calls->next_track_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->next_track_id) {
			mprx.ctrlr.cbs->next_track_id(player, 0, id);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_next_track_id_set(struct media_player *player, uint64_t id)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->next_track_id_set) {
		mprx.local_player.calls->next_track_id_set(id);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_current_group_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->current_group_id_get) {
		uint64_t id = mprx.local_player.calls->current_group_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_group_id) {
			mprx.ctrlr.cbs->current_group_id(player, 0, id);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_current_group_id_set(struct media_player *player, uint64_t id)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->current_group_id_set) {
		mprx.local_player.calls->current_group_id_set(id);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_parent_group_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->parent_group_id_get) {
		uint64_t id = mprx.local_player.calls->parent_group_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->parent_group_id) {
			mprx.ctrlr.cbs->parent_group_id(player, 0, id);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_playing_order_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->playing_order_get) {
		uint8_t order = mprx.local_player.calls->playing_order_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_order) {
			mprx.ctrlr.cbs->playing_order(player, 0, order);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_playing_order_set(struct media_player *player, uint8_t order)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->playing_order_set) {
		mprx.local_player.calls->playing_order_set(order);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_playing_orders_supported_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->playing_orders_supported_get) {
		uint16_t orders = mprx.local_player.calls->playing_orders_supported_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_orders_supported) {
			mprx.ctrlr.cbs->playing_orders_supported(player, 0, orders);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_media_state_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->media_state_get) {
		uint8_t state = mprx.local_player.calls->media_state_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->media_state) {
			mprx.ctrlr.cbs->media_state(player, 0, state);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_operation_set(struct media_player *player, struct mpl_op_t operation)
{
	/* TODO: Use provided player */

	/* TODO: Check for errors and return error code in case of errors */
	if (mprx.local_player.calls->operation_set) {
		mprx.local_player.calls->operation_set(operation);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	return 0;
}

int media_proxy_ctrl_operations_supported_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->operations_supported_get) {
		uint32_t ops = mprx.local_player.calls->operations_supported_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->operations_supported) {
			mprx.ctrlr.cbs->operations_supported(player, 0, ops);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

int media_proxy_ctrl_search_set(struct media_player *player, struct mpl_search_t search)
{
	/* TODO: Use provided player */

	/* TODO: Check for errors and return error code in case of errors */
	if (mprx.local_player.calls->search_set) {
		mprx.local_player.calls->search_set(search);
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	return 0;
}

int media_proxy_ctrl_search_results_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->search_results_id_get) {
		uint64_t id = mprx.local_player.calls->search_results_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search_results_id) {
			mprx.ctrlr.cbs->search_results_id(player, 0, id);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

uint8_t media_proxy_ctrl_content_ctrl_id_get(struct media_player *player)
{
	/* TODO: Use provided player */

	if (mprx.local_player.calls->content_ctrl_id_get) {
		uint8_t ccid = mprx.local_player.calls->content_ctrl_id_get();

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->content_ctrl_id) {
			mprx.ctrlr.cbs->content_ctrl_id(player, 0, ccid);
		} else {
			BT_DBG("No callback");
		}
	} else {
		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	/* TODO: Check for errors and return error code in case of errors */
	return 0;
}

/* Player calls *******************************************/

int media_proxy_pl_register(struct media_proxy_pl_calls *pl_calls)
{
	if (pl_calls == NULL) {
		BT_DBG("NULL calls");
		return -EINVAL;
	}

	if (mprx.local_player.registered) {
		BT_DBG("Player already registered");
		return -EALREADY;
	}

	mprx.local_player.calls = pl_calls;
	mprx.local_player.registered = true;

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->local_player_instance) {
		mprx.ctrlr.cbs->local_player_instance(&mprx.local_player, 0);
	}

	return 0;
};

/* Player callbacks ********************************/

void media_proxy_pl_track_changed_cb(void)
{
	mprx.sctrlr.cbs->track_changed();

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_changed) {
		mprx.ctrlr.cbs->track_changed(NULL, 0);
	} else {
		BT_DBG("No ctrlr track changed callback");
	}
}

void media_proxy_pl_track_title_cb(char *title)
{
	mprx.sctrlr.cbs->track_title(title);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_title) {
		mprx.ctrlr.cbs->track_title(NULL, 0, title);
	} else {
		BT_DBG("No ctrlr track title callback");
	}
}

void media_proxy_pl_track_duration_cb(int32_t duration)
{
	mprx.sctrlr.cbs->track_duration(duration);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_duration) {
		mprx.ctrlr.cbs->track_duration(NULL, 0, duration);
	} else {
		BT_DBG("No ctrlr track duration callback");
	}
}

void media_proxy_pl_track_position_cb(int32_t position)
{
	mprx.sctrlr.cbs->track_position(position);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_position) {
		mprx.ctrlr.cbs->track_position(NULL, 0, position);
	} else {
		BT_DBG("No ctrlr track position callback");
	}
}

void media_proxy_pl_playback_speed_cb(int8_t speed)
{
	mprx.sctrlr.cbs->playback_speed(speed);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playback_speed) {
		mprx.ctrlr.cbs->playback_speed(NULL, 0, speed);
	} else {
		BT_DBG("No ctrlr playback speed callback");
	}
}

void media_proxy_pl_seeking_speed_cb(int8_t speed)
{
	mprx.sctrlr.cbs->seeking_speed(speed);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->seeking_speed) {
		mprx.ctrlr.cbs->seeking_speed(NULL, 0, speed);
	} else {
		BT_DBG("No ctrlr seeking speed callback");
	}
}

#ifdef CONFIG_BT_OTS
void media_proxy_pl_current_track_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->current_track_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_track_id) {
		mprx.ctrlr.cbs->current_track_id(NULL, 0, id);
	} else {
		BT_DBG("No ctrlr current track id callback");
	}
}

void media_proxy_pl_next_track_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->next_track_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->next_track_id) {
		mprx.ctrlr.cbs->next_track_id(NULL, 0, id);
	} else {
		BT_DBG("No ctrlr next track id callback");
	}
}

void media_proxy_pl_current_group_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->current_group_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_group_id) {
		mprx.ctrlr.cbs->current_group_id(NULL, 0, id);
	} else {
		BT_DBG("No ctrlr current group id callback");
	}
}

void media_proxy_pl_parent_group_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->parent_group_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->parent_group_id) {
		mprx.ctrlr.cbs->parent_group_id(NULL, 0, id);
	} else {
		BT_DBG("No ctrlr parent group id callback");
	}
}
#endif /* CONFIG_BT_OTS */

void media_proxy_pl_playing_order_cb(uint8_t order)
{
	mprx.sctrlr.cbs->playing_order(order);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_order) {
		mprx.ctrlr.cbs->playing_order(NULL, 0, order);
	} else {
		BT_DBG("No ctrlr playing order callback");
	}
}

void media_proxy_pl_media_state_cb(uint8_t state)
{
	mprx.sctrlr.cbs->media_state(state);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->media_state) {
		mprx.ctrlr.cbs->media_state(NULL, 0, state);
	} else {
		BT_DBG("No ctrlr media state callback");
	}
}

void media_proxy_pl_operation_cb(struct mpl_op_ntf_t op_ntf)
{
	mprx.sctrlr.cbs->operation(op_ntf);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->operation) {
		mprx.ctrlr.cbs->operation(NULL, 0, op_ntf);
	} else {
		BT_DBG("No ctrlr operations callback");
	}
}

void media_proxy_pl_operations_supported_cb(uint32_t operations)
{
	mprx.sctrlr.cbs->operations_supported(operations);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->operations_supported) {
		mprx.ctrlr.cbs->operations_supported(NULL, 0, operations);
	} else {
		BT_DBG("No ctrlr operations supported callback");
	}
}

#ifdef CONFIG_BT_OTS
void media_proxy_pl_search_cb(uint8_t result_code)
{
	mprx.sctrlr.cbs->search(result_code);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search) {
		mprx.ctrlr.cbs->search(NULL, 0, result_code);
	} else {
		BT_DBG("No ctrlr search callback");
	}
}

void media_proxy_pl_search_results_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->search_results_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search_results_id) {
		mprx.ctrlr.cbs->search_results_id(NULL, 0, id);
	} else {
		BT_DBG("No ctrlr search results id callback");
	}
}
#endif /* CONFIG_BT_OTS */
