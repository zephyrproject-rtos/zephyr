/*  Media proxy */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/check.h>

#include "media_proxy.h"
#include "media_proxy_internal.h"
#include <bluetooth/mcc.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MEDIA_PROXY)
#define LOG_MODULE_NAME media_proxy
#include "common/log.h"


/* Media player */
struct media_player {
	struct media_proxy_pl_calls *calls;
	struct bt_conn *conn;  /* TODO: Treat local and remote player differently */
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

	/* Remote media player - to have a player instance for the user, accessed via MCC */
	struct media_player  remote_player;
};

static struct mprx mprx = { 0 };
#ifdef CONFIG_BT_MCC
static struct bt_mcc_cb_t mcc_cbs;
#endif /* CONFIG_BT_MCC */

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

const char *media_proxy_sctrl_player_name_get(void)
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

const char *media_proxy_sctrl_icon_url_get(void)
{
	return mprx.local_player.calls->icon_url_get();
}

const char *media_proxy_sctrl_track_title_get(void)
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

void media_proxy_sctrl_command_send(struct mpl_cmd_t cmd)
{
	mprx.local_player.calls->command_send(cmd);
}

uint32_t media_proxy_sctrl_commands_supported_get(void)
{
	return mprx.local_player.calls->commands_supported_get();
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


/* Media control client callbacks *********************************************/

#ifdef CONFIG_BT_MCC
static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		BT_ERR("Discovery failed (%d)", err);
	}

	BT_DBG("Disovered player");

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->discover_player) {
		mprx.ctrlr.cbs->discover_player(&mprx.remote_player, err);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_player_name_read_cb(struct bt_conn *conn, int err, const char *name)
{
	/* Debug statements for at least a couple of the callbacks, to show flow */
	BT_DBG("MCC player name callback");

	if (err) {
		BT_ERR("Player name failed");
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->player_name) {
		mprx.ctrlr.cbs->player_name(&mprx.remote_player, err, name);
	} else {
		BT_DBG("No callback");
	}
}

#ifdef CONFIG_BT_MCC_OTS
static void mcc_icon_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	BT_DBG("Icon Object ID callback");

	if (err) {
		BT_ERR("Icon Object ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->icon_id) {
		mprx.ctrlr.cbs->icon_id(&mprx.remote_player, err, id);
	} else {
		BT_DBG("No callback");
	}
}
#endif /* CONFIG_BT_MCC_OTS */

static void mcc_icon_url_read_cb(struct bt_conn *conn, int err, const char *url)
{
	if (err) {
		BT_ERR("Icon URL read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->icon_url) {
		mprx.ctrlr.cbs->icon_url(&mprx.remote_player, err, url);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_track_changed_ntf_cb(struct bt_conn *conn, int err)
{
	if (err) {
		BT_ERR("Track change notification failed (%d)", err);
		return;
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_changed) {
		mprx.ctrlr.cbs->track_changed(&mprx.remote_player, err);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_track_title_read_cb(struct bt_conn *conn, int err, const char *title)
{
	if (err) {
		BT_ERR("Track title read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_title) {
		mprx.ctrlr.cbs->track_title(&mprx.remote_player, err, title);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_track_dur_read_cb(struct bt_conn *conn, int err, int32_t dur)
{
	if (err) {
		BT_ERR("Track duration read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_duration) {
		mprx.ctrlr.cbs->track_duration(&mprx.remote_player, err, dur);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_track_position_read_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		BT_ERR("Track position read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_position) {
		mprx.ctrlr.cbs->track_position(&mprx.remote_player, err, pos);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_track_position_set_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		BT_ERR("Track Position set failed (%d)", err);
	}

	/* MCC has separate callbacks for read and set - media_proxy has a common one */
	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_position) {
		mprx.ctrlr.cbs->track_position(&mprx.remote_player, err, pos);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_playback_speed_read_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		BT_ERR("Playback speed read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playback_speed) {
		mprx.ctrlr.cbs->playback_speed(&mprx.remote_player, err, speed);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_playback_speed_set_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		BT_ERR("Playback speed set failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playback_speed) {
		mprx.ctrlr.cbs->playback_speed(&mprx.remote_player, err, speed);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_seeking_speed_read_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		BT_ERR("Seeking speed read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->seeking_speed) {
		mprx.ctrlr.cbs->seeking_speed(&mprx.remote_player, err, speed);
	} else {
		BT_DBG("No callback");
	}
}

#ifdef CONFIG_BT_MCC_OTS
static void mcc_segments_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		BT_ERR("Track Segments Object ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_segments_id) {
		mprx.ctrlr.cbs->track_segments_id(&mprx.remote_player, err, id);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_current_track_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		BT_ERR("Current Track Object ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_track_id) {
		mprx.ctrlr.cbs->current_track_id(&mprx.remote_player, err, id);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_next_track_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		BT_ERR("Next Track Object ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->next_track_id) {
		mprx.ctrlr.cbs->next_track_id(&mprx.remote_player, err, id);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_current_group_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		BT_ERR("Current Group Object ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_group_id) {
		mprx.ctrlr.cbs->current_group_id(&mprx.remote_player, err, id);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_parent_group_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		BT_ERR("Parent Group Object ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->parent_group_id) {
		mprx.ctrlr.cbs->parent_group_id(&mprx.remote_player, err, id);
	} else {
		BT_DBG("No callback");
	}
}

#endif /* CONFIG_BT_MCC_OTS */

static void mcc_playing_order_read_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		BT_ERR("Playing order read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_order) {
		mprx.ctrlr.cbs->playing_order(&mprx.remote_player, err, order);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_playing_order_set_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		BT_ERR("Playing order set failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_order) {
		mprx.ctrlr.cbs->playing_order(&mprx.remote_player, err, order);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_playing_orders_supported_read_cb(struct bt_conn *conn, int err, uint16_t orders)
{
	if (err) {
		BT_ERR("Playing orders supported read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_orders_supported) {
		mprx.ctrlr.cbs->playing_orders_supported(&mprx.remote_player, err, orders);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_media_state_read_cb(struct bt_conn *conn, int err, uint8_t state)
{
	if (err) {
		BT_ERR("Media State read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->media_state) {
		mprx.ctrlr.cbs->media_state(&mprx.remote_player, err, state);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_cmd_send_cb(struct bt_conn *conn, int err, struct mpl_cmd_t cmd)
{
	if (err) {
		struct mpl_cmd_ntf_t ntf = {0};

		BT_ERR("Command send failed (%d) - opcode: %d, param: %d",
		       err, cmd.opcode, cmd.param);

		/* If error, call the callback to propagate the error to the caller.
		 * (A notification parameter is required for the callback - use an empty one.)
		 */

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->command) {
			mprx.ctrlr.cbs->command(&mprx.remote_player, err, ntf);
		} else {
			BT_DBG("No callback");
		}
	}

	/* If no error, the result of the command will come as a notification
	 * which will be handled by the mcc_cmd_ntf_cb() callback.
	 * So, no need to call the callback here.
	 */
}

static void mcc_cmd_ntf_cb(struct bt_conn *conn, int err,
			   struct mpl_cmd_ntf_t ntf)
{
	if (err) {
		BT_ERR("Command notification error (%d) - command opcode: %d, result: %d",
		       err, ntf.requested_opcode, ntf.result_code);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->command) {
		mprx.ctrlr.cbs->command(&mprx.remote_player, err, ntf);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_opcodes_supported_read_cb(struct bt_conn *conn, int err, uint32_t opcodes)
{
	if (err) {
		BT_ERR("Opcodes supported read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->commands_supported) {
		mprx.ctrlr.cbs->commands_supported(&mprx.remote_player, err, opcodes);
	} else {
		BT_DBG("No callback");
	}
}

#ifdef CONFIG_BT_MCC_OTS
static void mcc_scp_set_cb(struct bt_conn *conn, int err, struct mpl_search_t search)
{
	if (err) {
		BT_ERR("Search Control Point set failed (%d)", err);

		/* If error, call the callback to propagate the error to the caller.
		 * Return FAILURE, as an error indicates this was not a success
		 * TODO: Is there a better way to handle this situation?
		 */

		if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search) {
			mprx.ctrlr.cbs->search(&mprx.remote_player, err,
					       MEDIA_PROXY_SEARCH_FAILURE);
		} else {
			BT_DBG("No callback");
		}
	}

	/* If no error, the result of the search operation will come as a notification
	 * which will be handled by the mcc_scp_ntf_cb() callback.
	 * So, no need to call the callback here.
	 * TODO: Add write callback here, along with for other writes
	 */
}

static void mcc_scp_ntf_cb(struct bt_conn *conn, int err, uint8_t result_code)
{
	if (err) {
		BT_ERR("Search Control Point notification error (%d), result code: %d",
		       err, result_code);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search) {
		mprx.ctrlr.cbs->search(&mprx.remote_player, err, result_code);
	} else {
		BT_DBG("No callback");
	}
}

static void mcc_search_results_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		BT_ERR("Search Results Object ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search_results_id) {
		mprx.ctrlr.cbs->search_results_id(&mprx.remote_player, err, id);
	} else {
		BT_DBG("No callback");
	}
}
#endif /* CONFIG_BT_MCC_OTS */

static void mcc_content_control_id_read_cb(struct bt_conn *conn, int err, uint8_t ccid)
{
	if (err) {
		BT_ERR("Content Control ID read failed (%d)", err);
	}

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->content_ctrl_id) {
		mprx.ctrlr.cbs->content_ctrl_id(&mprx.remote_player, err, ccid);
	} else {
		BT_DBG("No callback");
	}
}

#endif /* CONFIG_BT_MCC */


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

#ifdef CONFIG_BT_MCC
int media_proxy_ctrl_discover_player(struct bt_conn *conn)
{
	int err;

	/* Initialize MCC */
	mcc_cbs.discover_mcs                  = mcc_discover_mcs_cb;
	mcc_cbs.player_name_read              = mcc_player_name_read_cb;
#ifdef CONFIG_BT_MCC_OTS
	mcc_cbs.icon_obj_id_read              = mcc_icon_obj_id_read_cb;
#endif /* CONFIG_BT_MCC_OTS */
	mcc_cbs.icon_url_read                 = mcc_icon_url_read_cb;
	mcc_cbs.track_changed_ntf             = mcc_track_changed_ntf_cb;
	mcc_cbs.track_title_read              = mcc_track_title_read_cb;
	mcc_cbs.track_dur_read                = mcc_track_dur_read_cb;
	mcc_cbs.track_position_read           = mcc_track_position_read_cb;
	mcc_cbs.track_position_set            = mcc_track_position_set_cb;
	mcc_cbs.playback_speed_read           = mcc_playback_speed_read_cb;
	mcc_cbs.playback_speed_set            = mcc_playback_speed_set_cb;
	mcc_cbs.seeking_speed_read            = mcc_seeking_speed_read_cb;
#ifdef CONFIG_BT_MCC_OTS
	mcc_cbs.segments_obj_id_read          = mcc_segments_obj_id_read_cb;
	mcc_cbs.current_track_obj_id_read     = mcc_current_track_obj_id_read_cb;
	mcc_cbs.next_track_obj_id_read        = mcc_next_track_obj_id_read_cb;
	mcc_cbs.current_group_obj_id_read     = mcc_current_group_obj_id_read_cb;
	mcc_cbs.parent_group_obj_id_read      = mcc_parent_group_obj_id_read_cb;
#endif /* CONFIG_BT_MCC_OTS */
	mcc_cbs.playing_order_read	      = mcc_playing_order_read_cb;
	mcc_cbs.playing_order_set             = mcc_playing_order_set_cb;
	mcc_cbs.playing_orders_supported_read = mcc_playing_orders_supported_read_cb;
	mcc_cbs.media_state_read              = mcc_media_state_read_cb;
	mcc_cbs.cmd_send                      = mcc_cmd_send_cb;
	mcc_cbs.cmd_ntf                       = mcc_cmd_ntf_cb;
	mcc_cbs.opcodes_supported_read        = mcc_opcodes_supported_read_cb;
#ifdef CONFIG_BT_MCC_OTS
	mcc_cbs.scp_set                       = mcc_scp_set_cb;
	mcc_cbs.scp_ntf                       = mcc_scp_ntf_cb;
	mcc_cbs.search_results_obj_id_read    = mcc_search_results_obj_id_read_cb;
#endif /* CONFIG_BT_MCC_OTS */
	mcc_cbs.content_control_id_read       = mcc_content_control_id_read_cb;

	err = bt_mcc_init(&mcc_cbs);
	if (err) {
		BT_ERR("Failed to initialize MCC");
		return err;
	}

	/* Start discovery of remote MCS, subscribe to notifications */
	err = bt_mcc_discover_mcs(conn, 1);
	if (err) {
		BT_ERR("Discovery failed");
		return err;
	}

	mprx.remote_player.conn = conn;
	mprx.remote_player.registered = true;  /* TODO: Do MCC init and "registration" at startup */

	return 0;
}
#endif /* CONFIG_BT_MCC	*/

int media_proxy_ctrl_player_name_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		BT_DBG("Local player");
		if (mprx.local_player.calls->player_name_get) {
			const char *name = mprx.local_player.calls->player_name_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->player_name) {
				mprx.ctrlr.cbs->player_name(&mprx.local_player, 0, name);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		BT_DBG("Remote player");
		return bt_mcc_read_player_name(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_icon_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->icon_id_get) {
			uint64_t id = mprx.local_player.calls->icon_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->icon_id) {
				mprx.ctrlr.cbs->icon_id(&mprx.local_player, 0, id);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_icon_obj_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_icon_url_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->icon_url_get) {
			const char *url = mprx.local_player.calls->icon_url_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->icon_url) {
				mprx.ctrlr.cbs->icon_url(player, 0, url);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_icon_url(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_track_title_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->track_title_get) {
			const char *title = mprx.local_player.calls->track_title_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_title) {
				mprx.ctrlr.cbs->track_title(player, 0, title);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_track_title(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_track_duration_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->track_duration_get) {
			int32_t duration = mprx.local_player.calls->track_duration_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_duration) {
				mprx.ctrlr.cbs->track_duration(player, 0, duration);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}
	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_track_dur(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_track_position_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->track_position_get) {
			int32_t position = mprx.local_player.calls->track_position_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_position) {
				mprx.ctrlr.cbs->track_position(player, 0, position);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_track_position(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_track_position_set(struct media_player *player, int32_t position)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->track_position_set) {
			mprx.local_player.calls->track_position_set(position);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_set_track_position(mprx.remote_player.conn, position);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_playback_speed_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->playback_speed_get) {
			int8_t speed = mprx.local_player.calls->playback_speed_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playback_speed) {
				mprx.ctrlr.cbs->playback_speed(player, 0, speed);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_playback_speed(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_playback_speed_set(struct media_player *player, int8_t speed)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->playback_speed_set) {
			mprx.local_player.calls->playback_speed_set(speed);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_set_playback_speed(mprx.remote_player.conn, speed);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_seeking_speed_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->seeking_speed_get) {
			int8_t speed = mprx.local_player.calls->seeking_speed_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->seeking_speed) {
				mprx.ctrlr.cbs->seeking_speed(player, 0, speed);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_seeking_speed(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_track_segments_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->track_segments_id_get) {
			uint64_t id = mprx.local_player.calls->track_segments_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_segments_id) {
				mprx.ctrlr.cbs->track_segments_id(player, 0, id);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_segments_obj_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_current_track_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->current_track_id_get) {
			uint64_t id = mprx.local_player.calls->current_track_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_track_id) {
				mprx.ctrlr.cbs->current_track_id(player, 0, id);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_current_track_obj_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_current_track_id_set(struct media_player *player, uint64_t id)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->current_track_id_set) {
			mprx.local_player.calls->current_track_id_set(id);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		/* TODO: Uncomment when function is implemented */
		/* return bt_mcc_set_current_track_obj_id(mprx.remote_player.conn, position); */
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_next_track_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->next_track_id_get) {
			uint64_t id = mprx.local_player.calls->next_track_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->next_track_id) {
				mprx.ctrlr.cbs->next_track_id(player, 0, id);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_next_track_obj_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_next_track_id_set(struct media_player *player, uint64_t id)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->next_track_id_set) {
			mprx.local_player.calls->next_track_id_set(id);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		/* TODO: Uncomment when function is implemented */
		/* return bt_mcc_set_next_track_obj_id(mprx.remote_player.conn, position); */
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_current_group_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->current_group_id_get) {
			uint64_t id = mprx.local_player.calls->current_group_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_group_id) {
				mprx.ctrlr.cbs->current_group_id(player, 0, id);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_current_group_obj_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_current_group_id_set(struct media_player *player, uint64_t id)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->current_group_id_set) {
			mprx.local_player.calls->current_group_id_set(id);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		/* TODO: Uncomment when function is implemented */
		/* return bt_mcc_set_current_group_obj_id(mprx.remote_player.conn, position); */
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_parent_group_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->parent_group_id_get) {
			uint64_t id = mprx.local_player.calls->parent_group_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->parent_group_id) {
				mprx.ctrlr.cbs->parent_group_id(player, 0, id);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_parent_group_obj_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_playing_order_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->playing_order_get) {
			uint8_t order = mprx.local_player.calls->playing_order_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_order) {
				mprx.ctrlr.cbs->playing_order(player, 0, order);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		BT_DBG("Remote player");
		return bt_mcc_read_playing_order(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_playing_order_set(struct media_player *player, uint8_t order)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->playing_order_set) {
			mprx.local_player.calls->playing_order_set(order);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_set_playing_order(mprx.remote_player.conn, order);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_playing_orders_supported_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->playing_orders_supported_get) {
			uint16_t orders = mprx.local_player.calls->playing_orders_supported_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_orders_supported) {
				mprx.ctrlr.cbs->playing_orders_supported(player, 0, orders);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_playing_orders_supported(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_media_state_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->media_state_get) {
			uint8_t state = mprx.local_player.calls->media_state_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->media_state) {
				mprx.ctrlr.cbs->media_state(player, 0, state);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_media_state(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_command_send(struct media_player *player, struct mpl_cmd_t cmd)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->command_send) {
			mprx.local_player.calls->command_send(cmd);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_send_cmd(mprx.remote_player.conn, cmd);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_commands_supported_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->commands_supported_get) {
			uint32_t opcodes = mprx.local_player.calls->commands_supported_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->commands_supported) {
				mprx.ctrlr.cbs->commands_supported(player, 0, opcodes);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_opcodes_supported(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_search_set(struct media_player *player, struct mpl_search_t search)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->search_set) {
			mprx.local_player.calls->search_set(search);

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_set_scp(mprx.remote_player.conn, search);
	}

	return -EOPNOTSUPP;
}

int media_proxy_ctrl_search_results_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->search_results_id_get) {
			uint64_t id = mprx.local_player.calls->search_results_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search_results_id) {
				mprx.ctrlr.cbs->search_results_id(player, 0, id);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC_OTS) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_search_results_obj_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

uint8_t media_proxy_ctrl_content_ctrl_id_get(struct media_player *player)
{
	CHECKIF(player == NULL) {
		BT_DBG("player is NULL");
		return -EINVAL;
	}

	if (mprx.local_player.registered && player == &mprx.local_player) {
		if (mprx.local_player.calls->content_ctrl_id_get) {
			uint8_t ccid = mprx.local_player.calls->content_ctrl_id_get();

			if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->content_ctrl_id) {
				mprx.ctrlr.cbs->content_ctrl_id(player, 0, ccid);
			} else {
				BT_DBG("No callback");
			}

			return 0;
		}

		BT_DBG("No call");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_MCC) &&
	    mprx.remote_player.registered && player == &mprx.remote_player) {
		return bt_mcc_read_content_control_id(mprx.remote_player.conn);
	}

	return -EOPNOTSUPP;
}

/* Player calls *******************************************/

int media_proxy_pl_register(struct media_proxy_pl_calls *pl_calls)
{
	CHECKIF(pl_calls == NULL) {
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

/* All callbacks here must come from the local player - mprx.local_player */

void media_proxy_pl_track_changed_cb(void)
{
	mprx.sctrlr.cbs->track_changed();

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_changed) {
		mprx.ctrlr.cbs->track_changed(&mprx.local_player, 0);
	} else {
		BT_DBG("No ctrlr track changed callback");
	}
}

void media_proxy_pl_track_title_cb(char *title)
{
	mprx.sctrlr.cbs->track_title(title);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_title) {
		mprx.ctrlr.cbs->track_title(&mprx.local_player, 0, title);
	} else {
		BT_DBG("No ctrlr track title callback");
	}
}

void media_proxy_pl_track_duration_cb(int32_t duration)
{
	mprx.sctrlr.cbs->track_duration(duration);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_duration) {
		mprx.ctrlr.cbs->track_duration(&mprx.local_player, 0, duration);
	} else {
		BT_DBG("No ctrlr track duration callback");
	}
}

void media_proxy_pl_track_position_cb(int32_t position)
{
	mprx.sctrlr.cbs->track_position(position);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->track_position) {
		mprx.ctrlr.cbs->track_position(&mprx.local_player, 0, position);
	} else {
		BT_DBG("No ctrlr track position callback");
	}
}

void media_proxy_pl_playback_speed_cb(int8_t speed)
{
	mprx.sctrlr.cbs->playback_speed(speed);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playback_speed) {
		mprx.ctrlr.cbs->playback_speed(&mprx.local_player, 0, speed);
	} else {
		BT_DBG("No ctrlr playback speed callback");
	}
}

void media_proxy_pl_seeking_speed_cb(int8_t speed)
{
	mprx.sctrlr.cbs->seeking_speed(speed);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->seeking_speed) {
		mprx.ctrlr.cbs->seeking_speed(&mprx.local_player, 0, speed);
	} else {
		BT_DBG("No ctrlr seeking speed callback");
	}
}

#ifdef CONFIG_BT_OTS
void media_proxy_pl_current_track_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->current_track_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_track_id) {
		mprx.ctrlr.cbs->current_track_id(&mprx.local_player, 0, id);
	} else {
		BT_DBG("No ctrlr current track id callback");
	}
}

void media_proxy_pl_next_track_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->next_track_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->next_track_id) {
		mprx.ctrlr.cbs->next_track_id(&mprx.local_player, 0, id);
	} else {
		BT_DBG("No ctrlr next track id callback");
	}
}

void media_proxy_pl_current_group_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->current_group_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->current_group_id) {
		mprx.ctrlr.cbs->current_group_id(&mprx.local_player, 0, id);
	} else {
		BT_DBG("No ctrlr current group id callback");
	}
}

void media_proxy_pl_parent_group_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->parent_group_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->parent_group_id) {
		mprx.ctrlr.cbs->parent_group_id(&mprx.local_player, 0, id);
	} else {
		BT_DBG("No ctrlr parent group id callback");
	}
}
#endif /* CONFIG_BT_OTS */

void media_proxy_pl_playing_order_cb(uint8_t order)
{
	mprx.sctrlr.cbs->playing_order(order);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->playing_order) {
		mprx.ctrlr.cbs->playing_order(&mprx.local_player, 0, order);
	} else {
		BT_DBG("No ctrlr playing order callback");
	}
}

void media_proxy_pl_media_state_cb(uint8_t state)
{
	mprx.sctrlr.cbs->media_state(state);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->media_state) {
		mprx.ctrlr.cbs->media_state(&mprx.local_player, 0, state);
	} else {
		BT_DBG("No ctrlr media state callback");
	}
}

void media_proxy_pl_command_cb(struct mpl_cmd_ntf_t cmd_ntf)
{
	mprx.sctrlr.cbs->command(cmd_ntf);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->command) {
		mprx.ctrlr.cbs->command(&mprx.local_player, 0, cmd_ntf);
	} else {
		BT_DBG("No ctrlr command callback");
	}
}

void media_proxy_pl_commands_supported_cb(uint32_t opcodes)
{
	mprx.sctrlr.cbs->commands_supported(opcodes);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->commands_supported) {
		mprx.ctrlr.cbs->commands_supported(&mprx.local_player, 0, opcodes);
	} else {
		BT_DBG("No ctrlr commands supported callback");
	}
}

#ifdef CONFIG_BT_OTS
void media_proxy_pl_search_cb(uint8_t result_code)
{
	mprx.sctrlr.cbs->search(result_code);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search) {
		mprx.ctrlr.cbs->search(&mprx.local_player, 0, result_code);
	} else {
		BT_DBG("No ctrlr search callback");
	}
}

void media_proxy_pl_search_results_id_cb(uint64_t id)
{
	mprx.sctrlr.cbs->search_results_id(id);

	if (mprx.ctrlr.cbs && mprx.ctrlr.cbs->search_results_id) {
		mprx.ctrlr.cbs->search_results_id(&mprx.local_player, 0, id);
	} else {
		BT_DBG("No ctrlr search results id callback");
	}
}
#endif /* CONFIG_BT_OTS */
