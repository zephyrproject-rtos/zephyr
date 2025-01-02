/*  Media player skeleton implementation */

/*
 * Copyright (c) 2019 - 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/ccid.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util_macro.h>

#include "media_proxy_internal.h"
#include "mcs_internal.h"
#include "mpl_internal.h"

LOG_MODULE_REGISTER(bt_mpl, CONFIG_BT_MPL_LOG_LEVEL);

#define TRACK_STATUS_INVALID 0x00
#define TRACK_STATUS_VALID 0x01

#define TRACK_POS_WORK_DELAY_MS 1000
#define TRACK_POS_WORK_DELAY    K_MSEC(TRACK_POS_WORK_DELAY_MS)

#define PLAYBACK_SPEED_PARAM_DEFAULT MEDIA_PROXY_PLAYBACK_SPEED_UNITY

/* Temporary hardcoded setup for groups, tracks and segments */
/* There is one parent group, which is the parent of a number of groups. */
/* The groups have a number of tracks.  */
/* (There is only one level of groups, there are no groups of groups.) */
/* The first track of the first group has track segments, other tracks not. */

/* Track segments */
static struct mpl_tseg seg_2;
static struct mpl_tseg seg_3;

static struct mpl_tseg seg_1 = {
	.name_len = 5,
	.name	  = "Start",
	.pos	  = 0,
	.prev	  = NULL,
	.next	  = &seg_2,
};

static struct mpl_tseg seg_2 = {
	.name_len = 6,
	.name	  = "Middle",
	.pos	  = 2000,
	.prev	  = &seg_1,
	.next	  = &seg_3,
};

static struct mpl_tseg seg_3 = {
	.name_len = 3,
	.name	  = "End",
	.pos	  = 5000,
	.prev	  = &seg_2,
	.next	  = NULL,
};

static struct mpl_track track_1_2;
static struct mpl_track track_1_3;
static struct mpl_track track_1_4;
static struct mpl_track track_1_5;

/* Tracks */
static struct mpl_track track_1_1 = {
	.title	     = "Interlude #1 (Song for Alison)",
	.duration    = 6300,
	.segment     = &seg_1,
	.prev	     = NULL,
	.next	     = &track_1_2,
};


static struct mpl_track track_1_2 = {
	.title	     = "Interlude #2 (For Bobbye)",
	.duration    = 7500,
	.segment     = NULL,
	.prev	     = &track_1_1,
	.next	     = &track_1_3,
};

static struct mpl_track track_1_3 = {
	.title	     = "Interlude #3 (Levanto Seventy)",
	.duration    = 7800,
	.segment     = NULL,
	.prev	     = &track_1_2,
	.next	     = &track_1_4,
};

static struct mpl_track track_1_4 = {
	.title	     = "Interlude #4 (Vesper Dreams)",
	.duration    = 13500,
	.segment     = NULL,
	.prev	     = &track_1_3,
	.next	     = &track_1_5,
};

static struct mpl_track track_1_5 = {
	.title	     = "Interlude #5 (Shasti)",
	.duration    = 7500,
	.segment     = NULL,
	.prev	     = &track_1_4,
	.next	     = NULL,
};

static struct mpl_track track_2_2;
static struct mpl_track track_2_3;

static struct mpl_track track_2_1 = {
	.title	     = "Track 2.1",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = &track_2_2,
};

static struct mpl_track track_2_2 = {
	.title	     = "Track 2.2",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_2_1,
	.next	     = &track_2_3,
};

static struct mpl_track track_2_3 = {
	.title	     = "Track 2.3",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_2_2,
	.next	     = NULL,
};

static struct mpl_track track_3_2;
static struct mpl_track track_3_3;

static struct mpl_track track_3_1 = {
	.title	     = "Track 3.1",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = &track_3_2,
};

static struct mpl_track track_3_2 = {
	.title	     = "Track 3.2",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_3_1,
	.next	     = &track_3_3,
};

static struct mpl_track track_3_3 = {
	.title	     = "Track 3.3",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_3_2,
	.next	     = NULL,
};

static struct mpl_track track_4_2;

static struct mpl_track track_4_1 = {
	.title	     = "Track 4.1",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = &track_4_2,
};

static struct mpl_track track_4_2 = {
	.title	     = "Track 4.2",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_4_1,
	.next	     = NULL,
};

/* Groups */
static struct mpl_group group_2;
static struct mpl_group group_3;
static struct mpl_group group_4;
static struct mpl_group group_p;

static struct mpl_group group_1 = {
	.title  = "Joe Pass - Guitar Interludes",
	.track	= &track_1_1,
	.parent = &group_p,
	.prev	= NULL,
	.next	= &group_2,
};

static struct mpl_group group_2 = {
	.title  = "Group 2",
	.track	= &track_2_2,
	.parent = &group_p,
	.prev	= &group_1,
	.next	= &group_3,
};

static struct mpl_group group_3 = {
	.title  = "Group 3",
	.track	= &track_3_3,
	.parent = &group_p,
	.prev	= &group_2,
	.next	= &group_4,
};

static struct mpl_group group_4 = {
	.title  = "Group 4",
	.track	= &track_4_2,
	.parent = &group_p,
	.prev	= &group_3,
	.next	= NULL,
};

static struct mpl_group group_p = {
	.title  = "Parent group",
	.track	= &track_4_1,
	.parent = &group_p,
	.prev	= NULL,
	.next	= NULL,
};

static struct mpl_mediaplayer media_player = {
	.name			  = CONFIG_BT_MPL_MEDIA_PLAYER_NAME,
	.icon_url		  = CONFIG_BT_MPL_ICON_URL,
	.group			  = &group_1,
	.track_pos		  = 0,
	.state			  = MEDIA_PROXY_STATE_PAUSED,
	.playback_speed_param	  = PLAYBACK_SPEED_PARAM_DEFAULT,
	.seeking_speed_factor	  = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO,
	.playing_order		  = MEDIA_PROXY_PLAYING_ORDER_INORDER_REPEAT,
	.playing_orders_supported = MEDIA_PROXY_PLAYING_ORDERS_SUPPORTED_INORDER_ONCE |
				    MEDIA_PROXY_PLAYING_ORDERS_SUPPORTED_INORDER_REPEAT,
	.opcodes_supported	  = 0x001fffff, /* All opcodes */
#ifdef CONFIG_BT_MPL_OBJECTS
	.search_results_id	  = 0,
	.calls = { 0 },
#endif /* CONFIG_BT_MPL_OBJECTS */
	.next_track_set           = false
};

static void set_track_position(int32_t position);
static void set_relative_track_position(int32_t rel_pos);
static void do_track_change_notifications(struct mpl_mediaplayer *pl);
static void do_group_change_notifications(struct mpl_mediaplayer *pl);

#ifdef CONFIG_BT_MPL_OBJECTS

/* The types of objects we keep in the Object Transfer Service */
enum mpl_objects {
	MPL_OBJ_NONE = 0,
	MPL_OBJ_ICON,
	MPL_OBJ_TRACK_SEGMENTS,
	MPL_OBJ_TRACK,
	MPL_OBJ_PARENT_GROUP,
	MPL_OBJ_GROUP,
	MPL_OBJ_SEARCH_RESULTS,
};

enum mpl_obj_flag {
	MPL_OBJ_FLAG_BUSY,

	MPL_OBJ_FLAG_NUM_FLAGS, /* keep as last */
};

/* The active object */
/* Only a single object is selected or being added (active) at a time. */
/* And, except for the icon object, all objects can be created dynamically. */
/* So a single buffer to hold object content is sufficient.  */
struct obj_t {
	/* ID of the currently selected object*/
	uint64_t selected_id;

	/* Type of object being added, e.g. MPL_OBJ_ICON */
	uint8_t add_type;

	/* Descriptor of object being added */
	struct bt_ots_obj_created_desc *desc;
	union {
		/* Pointer to track being added */
		struct mpl_track *add_track;

		/* Pointer to group being added */
		struct mpl_group *add_group;
	};
	struct net_buf_simple *content;

	ATOMIC_DEFINE(flags, MPL_OBJ_FLAG_NUM_FLAGS);
};

static struct obj_t obj = {
	.selected_id = 0,
	.add_type = MPL_OBJ_NONE,
	.add_track = NULL,
	.add_group = NULL,
	.content = NET_BUF_SIMPLE(CONFIG_BT_MPL_MAX_OBJ_SIZE),
};

/* Set up content buffer for the icon object */
static int setup_icon_object(void)
{
	uint16_t index;
	uint8_t k;

	/* The icon object is supposed to be a bitmap. */
	/* For now, fill it with dummy data. */

	net_buf_simple_reset(obj.content);

	/* Size may be larger than what fits in 8 bits, use 16-bit for index */
	for (index = 0, k = 0;
	     index < MIN(CONFIG_BT_MPL_MAX_OBJ_SIZE,
			 CONFIG_BT_MPL_ICON_BITMAP_SIZE);
	     index++, k++) {
		net_buf_simple_add_u8(obj.content, k);
	}

	return obj.content->len;
}

/* Set up content buffer for a track segments object */
static uint32_t setup_segments_object(struct mpl_track *track)
{
	struct mpl_tseg *seg = track->segment;

	net_buf_simple_reset(obj.content);

	if (seg) {
		uint32_t tot_size = 0;

		while (seg->prev) {
			seg = seg->prev;
		}
		while (seg) {
			uint32_t seg_size = sizeof(seg->name_len);

			seg_size += seg->name_len;
			seg_size += sizeof(seg->pos);
			if (tot_size + seg_size > obj.content->size) {
				LOG_DBG("Segments object out of space");
				break;
			}
			net_buf_simple_add_u8(obj.content, seg->name_len);
			net_buf_simple_add_mem(obj.content, seg->name,
					       seg->name_len);
			net_buf_simple_add_le32(obj.content, seg->pos);

			tot_size += seg_size;
			seg = seg->next;
		}

		LOG_HEXDUMP_DBG(obj.content->data, obj.content->len, "Segments Object");
		LOG_DBG("Segments object length: %d", obj.content->len);
	} else {
		LOG_ERR("No seg!");
	}

	return obj.content->len;
}

/* Set up content buffer for a track object */
static uint32_t setup_track_object(struct mpl_track *track)
{
	uint16_t index;
	uint8_t k;

	/* The track object is supposed to be in Id3v2 format */
	/* For now, fill it with dummy data */

	net_buf_simple_reset(obj.content);

	/* Size may be larger than what fits in 8 bits, use 16-bit for index */
	for (index = 0, k = 0;
	     index < MIN(CONFIG_BT_MPL_MAX_OBJ_SIZE,
			 CONFIG_BT_MPL_TRACK_MAX_SIZE);
	     index++, k++) {
		net_buf_simple_add_u8(obj.content, k);
	}

	return obj.content->len;
}

/* Set up content buffer for the parent group object */
static uint32_t setup_parent_group_object(struct mpl_group *group)
{
	/* This function actually does not use the parent. */
	/* It just follows the list of groups. */
	/* The implementation has a fixed structure, with one parent group, */
	/* and one level of groups containing tracks only. */
	/* The track groups have a pointer to the parent, but there is no */
	/* pointer in the other direction, so it is not possible to go from */
	/* the parent group to a group of tracks. */

	uint8_t type = MEDIA_PROXY_GROUP_OBJECT_GROUP_TYPE;
	uint8_t record_size = sizeof(type) + BT_OTS_OBJ_ID_SIZE;
	int next_size = record_size;

	net_buf_simple_reset(obj.content);

	if (group) {
		while (group->prev) {
			group = group->prev;
		}
		/* While there is a group, and the record fits in the object */
		while (group && (next_size <= obj.content->size)) {
			net_buf_simple_add_u8(obj.content, type);
			net_buf_simple_add_le48(obj.content, group->id);
			group = group->next;
			next_size += record_size;
		}
		if (next_size > obj.content->size) {
			LOG_WRN("Not room for full group in object");
		}
		LOG_HEXDUMP_DBG(obj.content->data, obj.content->len, "Parent Group Object");
		LOG_DBG("Group object length: %d", obj.content->len);
	}
	return obj.content->len;
}

/* Set up contents for a group object */
/* The group object contains a concatenated list of records, where each */
/* record consists of a type byte and a UUID */
static uint32_t setup_group_object(struct mpl_group *group)
{
	struct mpl_track *track = group->track;
	uint8_t type = MEDIA_PROXY_GROUP_OBJECT_TRACK_TYPE;
	uint8_t record_size = sizeof(type) + BT_OTS_OBJ_ID_SIZE;
	int next_size = record_size;

	net_buf_simple_reset(obj.content);

	if (track) {
		while (track->prev) {
			track = track->prev;
		}
		/* While there is a track, and the record fits in the object */
		while (track && (next_size <= obj.content->size)) {
			net_buf_simple_add_u8(obj.content, type);
			net_buf_simple_add_le48(obj.content, track->id);
			track = track->next;
			next_size += record_size;
		}
		if (next_size > obj.content->size) {
			LOG_WRN("Not room for full group in object");
		}
		LOG_HEXDUMP_DBG(obj.content->data, obj.content->len, "Group Object");
		LOG_DBG("Group object length: %d", obj.content->len);
	}
	return obj.content->len;
}

/* Add the icon object to the OTS */
static int add_icon_object(struct mpl_mediaplayer *pl)
{
	int ret;
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	const struct bt_uuid *icon_type = BT_UUID_OTS_TYPE_MPL_ICON;
	static char *icon_name = "Icon";

	obj.add_type = MPL_OBJ_ICON;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_icon_object();
	obj.desc->name = icon_name;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(icon_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);
	if (ret < 0) {
		LOG_WRN("Unable to add icon object, error %d", ret);

		return ret;
	}

	return 0;
}

/* Add a track segments object to the OTS */
static int add_current_track_segments_object(struct mpl_mediaplayer *pl)
{
	int ret;
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	const struct bt_uuid *segs_type = BT_UUID_OTS_TYPE_TRACK_SEGMENT;

	obj.add_type = MPL_OBJ_TRACK_SEGMENTS;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_segments_object(pl->group->track);
	obj.desc->name = pl->group->track->title;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(segs_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);
	if (ret < 0) {
		LOG_WRN("Unable to add track segments object: %d", ret);

		return ret;
	}

	return 0;
}

/* Add a single track to the OTS */
static int add_track_object(struct mpl_track *track)
{
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	const struct bt_uuid *track_type = BT_UUID_OTS_TYPE_TRACK;
	int ret;

	if (!track) {
		LOG_ERR("No track");
		return -EINVAL;
	}

	obj.add_type = MPL_OBJ_TRACK;
	obj.add_track = track;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_track_object(track);
	obj.desc->name = track->title;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(track_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);
	if (ret < 0) {
		LOG_WRN("Unable to add track object: %d", ret);

		return ret;
	}

	return 0;
}

/* Add the parent group to the OTS */
static int add_parent_group_object(struct mpl_mediaplayer *pl)
{
	int ret;
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	const struct bt_uuid *group_type = BT_UUID_OTS_TYPE_GROUP;

	obj.add_type = MPL_OBJ_PARENT_GROUP;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_parent_group_object(pl->group);
	obj.desc->name = pl->group->parent->title;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(group_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);
	if (ret < 0) {
		LOG_WRN("Unable to add parent group object");

		return ret;
	}

	return 0;
}

/* Add a single group to the OTS */
static int add_group_object(struct mpl_group *group)
{
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	const struct bt_uuid *group_type = BT_UUID_OTS_TYPE_GROUP;
	int ret;

	if (!group) {
		LOG_ERR("No group");
		return -EINVAL;
	}

	obj.add_type = MPL_OBJ_GROUP;
	obj.add_group = group;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_group_object(group);
	obj.desc->name = group->title;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(group_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);
	if (ret < 0) {
		LOG_WRN("Unable to add group object: %d", ret);

		return ret;
	}

	return 0;
}

/* Add all tracks of a group to the OTS */
static int add_group_tracks(struct mpl_group *group)
{
	int ret_overall = 0;
	struct mpl_track *track = group->track;

	if (track) {
		while (track->prev) {
			track = track->prev;
		}

		while (track) {
			int ret = add_track_object(track);

			if (ret && !ret_overall) {
				ret_overall = ret;
			}
			track = track->next;
		}
	}
	return ret_overall;
}

/* Add all groups (except the parent group) and their tracks to the OTS */
static int add_group_and_track_objects(struct mpl_mediaplayer *pl)
{
	int ret_overall = 0;
	int ret;
	struct mpl_group *group = pl->group;

	if (group) {
		while (group->prev) {
			group = group->prev;
		}

		while (group) {
			ret = add_group_tracks(group);
			if (ret && !ret_overall) {
				ret_overall = ret;
			}

			ret = add_group_object(group);
			if (ret && !ret_overall) {
				ret_overall = ret;
			}
			group = group->next;
		}
	}

	ret = add_parent_group_object(pl);
	if (ret && !ret_overall) {
		ret_overall = ret;
	}

	return ret_overall;
}

/**** Callbacks from the object transfer service ******************************/

static int on_obj_deleted(struct bt_ots *ots, struct bt_conn *conn,
			   uint64_t id)
{
	LOG_DBG_OBJ_ID("Object Id deleted: ", id);

	return 0;
}

static void on_obj_selected(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id)
{
	if (atomic_test_and_set_bit(obj.flags, MPL_OBJ_FLAG_BUSY)) {
		/* TODO: Can there be a collision between select and internal */
		/* activities, like adding new objects? */
		LOG_ERR("Object busy - select not performed");
		return;
	}

	LOG_DBG_OBJ_ID("Object Id selected: ", id);

	if (id == media_player.icon_id) {
		LOG_DBG("Icon Object ID");
		(void)setup_icon_object();
	} else if (id == media_player.group->track->segments_id) {
		LOG_DBG("Current Track Segments Object ID");
		(void)setup_segments_object(media_player.group->track);
	} else if (id == media_player.group->track->id) {
		LOG_DBG("Current Track Object ID");
		(void)setup_track_object(media_player.group->track);
	} else if (media_player.next_track_set && id == media_player.next.track->id) {
		/* Next track, if the next track has been explicitly set */
		LOG_DBG("Next Track Object ID");
		(void)setup_track_object(media_player.next.track);
	} else if (id == media_player.group->track->next->id) {
		/* Next track, if next track has not been explicitly set */
		LOG_DBG("Next Track Object ID");
		(void)setup_track_object(media_player.group->track->next);
	} else if (id == media_player.group->parent->id) {
		LOG_DBG("Parent Group Object ID");
		(void)setup_parent_group_object(media_player.group);
	} else if (id == media_player.group->id) {
		LOG_DBG("Current Group Object ID");
		(void)setup_group_object(media_player.group);
	} else {
		LOG_ERR("Unknown Object ID");
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
		return;
	}

	obj.selected_id = id;
	atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
}

static int on_obj_created(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			  const struct bt_ots_obj_add_param *add_param,
			  struct bt_ots_obj_created_desc *created_desc)
{
	/* Objects are always created locally so we do not need to check for MPL_OBJ_FLAG_BUSY */

	LOG_DBG_OBJ_ID("Object Id created: ", id);

	*created_desc = *obj.desc;

	if (!bt_uuid_cmp(&add_param->type.uuid, BT_UUID_OTS_TYPE_MPL_ICON)) {
		LOG_DBG("Icon Obj Type");
		if (obj.add_type == MPL_OBJ_ICON) {
			obj.add_type = MPL_OBJ_NONE;
			media_player.icon_id = id;
		} else {
			LOG_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&add_param->type.uuid,
				BT_UUID_OTS_TYPE_TRACK_SEGMENT)) {
		LOG_DBG("Track Segments Obj Type");
		if (obj.add_type == MPL_OBJ_TRACK_SEGMENTS) {
			obj.add_type = MPL_OBJ_NONE;
			media_player.group->track->segments_id = id;
		} else {
			LOG_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&add_param->type.uuid,
				 BT_UUID_OTS_TYPE_TRACK)) {
		LOG_DBG("Track Obj Type");
		if (obj.add_type == MPL_OBJ_TRACK) {
			obj.add_type = MPL_OBJ_NONE;
			obj.add_track->id = id;
			obj.add_track = NULL;
		} else {
			LOG_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&add_param->type.uuid,
				 BT_UUID_OTS_TYPE_GROUP)) {
		LOG_DBG("Group Obj Type");
		if (obj.add_type == MPL_OBJ_PARENT_GROUP) {
			LOG_DBG("Parent group");
			obj.add_type = MPL_OBJ_NONE;
			media_player.group->parent->id = id;
		} else if (obj.add_type == MPL_OBJ_GROUP) {
			LOG_DBG("Other group");
			obj.add_type = MPL_OBJ_NONE;
			obj.add_group->id = id;
			obj.add_group = NULL;
		} else {
			LOG_DBG("Unexpected object creation");
		}

	} else {
		LOG_DBG("Unknown Object ID");
	}

	return 0;
}

static ssize_t on_object_send(struct bt_ots *ots, struct bt_conn *conn,
			      uint64_t id, void **data, size_t len,
			      off_t offset)
{
	if (atomic_test_and_set_bit(obj.flags, MPL_OBJ_FLAG_BUSY)) {
		/* TODO: Can there be a collision between select and internal */
		/* activities, like adding new objects? */
		LOG_ERR("Object busy");
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_BT_MPL_LOG_LEVEL_DBG)) {
		char t[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, t, sizeof(t));
		LOG_DBG("Object Id %s, offset %lu, length %zu", t, (long)offset, len);
	}

	if (id != obj.selected_id) {
		LOG_ERR("Read from unselected object");
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
		return -EINVAL;
	}

	if (!data) {
		LOG_DBG("Read complete");
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
		return 0;
	}

	if (offset >= obj.content->len) {
		LOG_DBG("Offset too large");
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MPL_LOG_LEVEL_DBG)) {
		if (len > obj.content->len - offset) {
			LOG_DBG("Requested len too large");
		}
	}

	*data = &obj.content->data[offset];
	atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);

	return MIN(len, obj.content->len - offset);
}

static struct bt_ots_cb ots_cbs = {
	.obj_created = on_obj_created,
	.obj_read = on_object_send,
	.obj_selected = on_obj_selected,
	.obj_deleted = on_obj_deleted,
};

#endif /* CONFIG_BT_MPL_OBJECTS */


/* TODO: It must be possible to replace the do_prev_segment(), do_prev_track */
/* and do_prev_group() with a generic do_prev() command that can be used at */
/* all levels.	Similarly for do_next, do_prev, and so on. */

static void do_prev_segment(struct mpl_mediaplayer *pl)
{
	LOG_DBG("Segment name before: %s", pl->group->track->segment->name);

	if (pl->group->track->segment->prev != NULL) {
		pl->group->track->segment = pl->group->track->segment->prev;
	}

	LOG_DBG("Segment name after: %s", pl->group->track->segment->name);
}

static void do_next_segment(struct mpl_mediaplayer *pl)
{
	LOG_DBG("Segment name before: %s", pl->group->track->segment->name);

	if (pl->group->track->segment->next != NULL) {
		pl->group->track->segment = pl->group->track->segment->next;
	}

	LOG_DBG("Segment name after: %s", pl->group->track->segment->name);
}

static void do_first_segment(struct mpl_mediaplayer *pl)
{
	LOG_DBG("Segment name before: %s", pl->group->track->segment->name);

	while (pl->group->track->segment->prev != NULL) {
		pl->group->track->segment = pl->group->track->segment->prev;
	}

	LOG_DBG("Segment name after: %s", pl->group->track->segment->name);
}

static void do_last_segment(struct mpl_mediaplayer *pl)
{
	LOG_DBG("Segment name before: %s", pl->group->track->segment->name);

	while (pl->group->track->segment->next != NULL) {
		pl->group->track->segment = pl->group->track->segment->next;
	}

	LOG_DBG("Segment name after: %s", pl->group->track->segment->name);
}

static void do_goto_segment(struct mpl_mediaplayer *pl, int32_t segnum)
{
	int32_t k;

	LOG_DBG("Segment name before: %s", pl->group->track->segment->name);

	if (segnum > 0) {
		/* Goto first segment */
		while (pl->group->track->segment->prev != NULL) {
			pl->group->track->segment =
				pl->group->track->segment->prev;
		}

		/* Then go segnum - 1 tracks forward */
		for (k = 0; k < (segnum - 1); k++) {
			if (pl->group->track->segment->next != NULL) {
				pl->group->track->segment =
					pl->group->track->segment->next;
			}
		}
	} else if (segnum < 0) {
		/* Goto last track */
		while (pl->group->track->segment->next != NULL) {
			pl->group->track->segment =
				pl->group->track->segment->next;
		}

		/* Then go |segnum - 1| tracks back */
		for (k = 0; k < (-segnum - 1); k++) {
			if (pl->group->track->segment->prev != NULL) {
				pl->group->track->segment =
					pl->group->track->segment->prev;
			}
		}
	}

	LOG_DBG("Segment name after: %s", pl->group->track->segment->name);

	set_track_position(pl->group->track->segment->pos);
}

static void do_prev_track(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	} else {
		/* For previous track, the position is reset to 0 */
		/* even if we stay at the same track (goto start of */
		/* track) */
		set_track_position(0);
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

/* Change to next track according to the current track's next track */
static void do_next_track_normal_order(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

/* Change to next track when the next track has been explicitly set
 *
 * ALWAYS changes the track, changes the group if required
 * Resets the next_track_set and the "next" pointers
 *
 * Returns true if the _group_ has been changed, otherwise false
 */
static void do_next_track_next_track_set(struct mpl_mediaplayer *pl)
{
	if (pl->next.group != pl->group) {
		pl->group = pl->next.group;
		do_group_change_notifications(pl);
	}

	pl->group->track = pl->next.track;

	pl->next.track = NULL;
	pl->next.group = NULL;
	pl->next_track_set = false;
	pl->track_pos = 0;
	do_track_change_notifications(pl);
}

static void do_next_track(struct mpl_mediaplayer *pl)
{
	if (pl->next_track_set) {
		LOG_DBG("Next track set");
		do_next_track_next_track_set(pl);
	} else {
		do_next_track_normal_order(pl);
	}
}

static void do_first_track(struct mpl_mediaplayer *pl, bool group_change)
{
	bool track_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	/* Set first track */
	while (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
		track_changed = true;
	}

	/* Notify about new track */
	if (group_change || track_changed) {
		media_player.track_pos = 0;
		do_track_change_notifications(&media_player);
	} else {
		/* For first track, the position is reset to 0 even */
		/* if we stay at the same track (goto start of track) */
		set_track_position(0);
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_last_track(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
		media_player.track_pos = 0;
		do_track_change_notifications(&media_player);
	} else {

		/* For last track, the position is reset to 0 even */
		/* if we stay at the same track (goto start of track) */
		set_track_position(0);
	}

	while (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_goto_track(struct mpl_mediaplayer *pl, int32_t tracknum)
{
	int32_t count = 0;
	int32_t k;

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (tracknum > 0) {
		/* Goto first track */
		while (pl->group->track->prev != NULL) {
			pl->group->track = pl->group->track->prev;
			count--;
		}

		/* Then go tracknum - 1 tracks forward */
		for (k = 0; k < (tracknum - 1); k++) {
			if (pl->group->track->next != NULL) {
				pl->group->track = pl->group->track->next;
				count++;
			}
		}
	} else if (tracknum < 0) {
		/* Goto last track */
		while (pl->group->track->next != NULL) {
			pl->group->track = pl->group->track->next;
			count++;
		}

		/* Then go |tracknum - 1| tracks back */
		for (k = 0; k < (-tracknum - 1); k++) {
			if (pl->group->track->prev != NULL) {
				pl->group->track = pl->group->track->prev;
				count--;
			}
		}
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	/* The track has changed if we have moved more in one direction */
	/* than in the other */
	if (count != 0) {
		media_player.track_pos = 0;
		do_track_change_notifications(&media_player);
	} else {
		/* For goto track, the position is reset to 0 */
		/* even if we stay at the same track (goto */
		/* start of track) */
		set_track_position(0);
	}
}

static void do_prev_group(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
		do_group_change_notifications(pl);
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_next_group(struct mpl_mediaplayer *pl)
{

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->next != NULL) {
		pl->group = pl->group->next;
		do_group_change_notifications(pl);
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_first_group(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
		do_group_change_notifications(pl);
	}

	while (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_last_group(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->next != NULL) {
		pl->group = pl->group->next;
		do_group_change_notifications(pl);
	}

	while (pl->group->next != NULL) {
		pl->group = pl->group->next;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_goto_group(struct mpl_mediaplayer *pl, int32_t groupnum)
{
	int32_t count = 0;
	int32_t k;

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (groupnum > 0) {
		/* Goto first group */
		while (pl->group->prev != NULL) {
			pl->group = pl->group->prev;
			count--;
		}

		/* Then go groupnum - 1 groups forward */
		for (k = 0; k < (groupnum - 1); k++) {
			if (pl->group->next != NULL) {
				pl->group = pl->group->next;
				count++;
			}
		}
	} else if (groupnum < 0) {
		/* Goto last group */
		while (pl->group->next != NULL) {
			pl->group = pl->group->next;
			count++;
		}

		/* Then go |groupnum - 1| groups back */
		for (k = 0; k < (-groupnum - 1); k++) {
			if (pl->group->prev != NULL) {
				pl->group = pl->group->prev;
				count--;
			}
		}
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	LOG_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	/* The group has changed if we have moved more in one direction */
	/* than in the other */
	if (count != 0) {
		do_group_change_notifications(pl);
	}
}

static void do_track_change_notifications(struct mpl_mediaplayer *pl)
{
	media_proxy_pl_track_changed_cb();
	media_proxy_pl_track_title_cb(pl->group->track->title);
	media_proxy_pl_track_duration_cb(pl->group->track->duration);
	media_proxy_pl_track_position_cb(pl->track_pos);
#ifdef CONFIG_BT_MPL_OBJECTS
	media_proxy_pl_current_track_id_cb(pl->group->track->id);
	if (pl->group->track->next) {
		media_proxy_pl_next_track_id_cb(pl->group->track->next->id);
	} else {
		/* Send a zero value to indicate that there is no next track */
		media_proxy_pl_next_track_id_cb(MPL_NO_TRACK_ID);
	}
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_group_change_notifications(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	media_proxy_pl_current_group_id_cb(pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

static void do_full_prev_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on first group) */
	do_prev_group(pl);

	/* Whether there is a group change or not, we always go to the first track */
	do_first_track(pl, true);
}

static void do_full_next_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on last group) */
	do_next_group(pl);

	/* Whether there is a group change or not, we always go to the first track */
	do_first_track(pl, true);
}

static void do_full_first_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on first group) */
	do_first_group(pl);

	/* Whether there is a group change or not, we always go to the first track */
	do_first_track(pl, true);
}

static void do_full_last_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on last group) */
	do_last_group(pl);

	/* Whether there is a group change or not, we always go to the first track */
	do_first_track(pl, true);
}

static void do_full_goto_group(struct mpl_mediaplayer *pl, int32_t groupnum)
{
	/* Change the group (if not already on given group) */
	do_goto_group(pl, groupnum);

	/* Whether there is a group change or not, we always go to the first track */
	do_first_track(pl, true);
}

static void mpl_set_state(uint8_t state)
{
	switch (state) {
	case MEDIA_PROXY_STATE_INACTIVE:
	case MEDIA_PROXY_STATE_PLAYING:
	case MEDIA_PROXY_STATE_PAUSED:
		(void)k_work_cancel_delayable(&media_player.pos_work);
		break;
	case MEDIA_PROXY_STATE_SEEKING:
		(void)k_work_schedule(&media_player.pos_work, TRACK_POS_WORK_DELAY);
		break;
	default:
		__ASSERT(false, "Invalid state: %u", state);
	}

	media_player.state = state;
	media_proxy_pl_media_state_cb(media_player.state);
}

/* Command handlers (state machines) */
static uint8_t inactive_state_command_handler(const struct mpl_cmd *command)
{
	uint8_t result_code = MEDIA_PROXY_CMD_SUCCESS;

	LOG_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_MPL_LOG_LEVEL_DBG)) {
		if (command->use_param) {
			LOG_DBG("Command parameter: %d", command->param);
		}
	}
	switch (command->opcode) {
	case MEDIA_PROXY_OP_PLAY: /* Fall-through - handle several cases identically */
	case MEDIA_PROXY_OP_PAUSE:
	case MEDIA_PROXY_OP_FAST_REWIND:
	case MEDIA_PROXY_OP_FAST_FORWARD:
	case MEDIA_PROXY_OP_STOP:
	case MEDIA_PROXY_OP_MOVE_RELATIVE:
	case MEDIA_PROXY_OP_PREV_SEGMENT:
	case MEDIA_PROXY_OP_NEXT_SEGMENT:
	case MEDIA_PROXY_OP_FIRST_SEGMENT:
	case MEDIA_PROXY_OP_LAST_SEGMENT:
	case MEDIA_PROXY_OP_GOTO_SEGMENT:
		result_code = MEDIA_PROXY_CMD_PLAYER_INACTIVE;
		break;
	case MEDIA_PROXY_OP_PREV_TRACK:
		do_prev_track(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_NEXT_TRACK:
		/* TODO:
		 * The case where the next track has been set explicitly breaks somewhat
		 * with the "next" order hardcoded into the group and track structure
		 */
		do_next_track(&media_player);

		/* For next track, the position is kept if the track */
		/* does not change */
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_FIRST_TRACK:
		do_first_track(&media_player, false);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_LAST_TRACK:
		do_last_track(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_GOTO_TRACK:
		if (command->use_param) {
			do_goto_track(&media_player, command->param);
			mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}
		break;
	case MEDIA_PROXY_OP_PREV_GROUP:
		do_full_prev_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_NEXT_GROUP:
		do_full_next_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_FIRST_GROUP:
		do_full_first_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_LAST_GROUP:
		do_full_last_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&media_player, command->param);
			mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	default:
		LOG_DBG("Invalid command: %d", command->opcode);
		result_code = MEDIA_PROXY_CMD_NOT_SUPPORTED;
		break;
	}

	return result_code;
}

static uint8_t playing_state_command_handler(const struct mpl_cmd *command)
{
	uint8_t result_code = MEDIA_PROXY_CMD_SUCCESS;

	LOG_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_MPL_LOG_LEVEL_DBG)) {
		if (command->use_param) {
			LOG_DBG("Command parameter: %d", command->param);
		}
	}

	switch (command->opcode) {
	case MEDIA_PROXY_OP_PLAY:
		/* Continue playing - i.e. do nothing */
		break;
	case MEDIA_PROXY_OP_PAUSE:
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_FAST_REWIND:
		/* We're in playing state, seeking speed must have been zero */
		media_player.seeking_speed_factor = -MPL_SEEKING_SPEED_FACTOR_STEP;
		mpl_set_state(MEDIA_PROXY_STATE_SEEKING);
		media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		break;
	case MEDIA_PROXY_OP_FAST_FORWARD:
		/* We're in playing state, seeking speed must have been zero */
		media_player.seeking_speed_factor = MPL_SEEKING_SPEED_FACTOR_STEP;
		mpl_set_state(MEDIA_PROXY_STATE_SEEKING);
		media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		break;
	case MEDIA_PROXY_OP_STOP:
		set_track_position(0);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_MOVE_RELATIVE:
		if (command->use_param) {
			set_relative_track_position(command->param);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_PREV_SEGMENT:
		/* Switch to previous segment if we are less than <margin> */
		/* into the segment, otherwise go to start of segment */
		if (media_player.track_pos - PREV_MARGIN <
		    media_player.group->track->segment->pos) {
			do_prev_segment(&media_player);
		}
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_NEXT_SEGMENT:
		do_next_segment(&media_player);
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_FIRST_SEGMENT:
		do_first_segment(&media_player);
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_LAST_SEGMENT:
		do_last_segment(&media_player);
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_GOTO_SEGMENT:
		if (command->use_param) {
			if (command->param != 0) {
				do_goto_segment(&media_player, command->param);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shall stay the same, and the */
			/* track position shall not change. */
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_PREV_TRACK:
		do_prev_track(&media_player);
		break;
	case MEDIA_PROXY_OP_NEXT_TRACK:
		do_next_track(&media_player);
		break;
	case MEDIA_PROXY_OP_FIRST_TRACK:
		do_first_track(&media_player, false);
		break;
	case MEDIA_PROXY_OP_LAST_TRACK:
		do_last_track(&media_player);
		break;
	case MEDIA_PROXY_OP_GOTO_TRACK:
		if (command->use_param) {
			do_goto_track(&media_player, command->param);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_PREV_GROUP:
		do_full_prev_group(&media_player);
		break;
	case MEDIA_PROXY_OP_NEXT_GROUP:
		do_full_next_group(&media_player);
		break;
	case MEDIA_PROXY_OP_FIRST_GROUP:
		do_full_first_group(&media_player);
		break;
	case MEDIA_PROXY_OP_LAST_GROUP:
		do_full_last_group(&media_player);
		break;
	case MEDIA_PROXY_OP_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&media_player, command->param);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}
		break;
	default:
		LOG_DBG("Invalid command: %d", command->opcode);
		result_code = MEDIA_PROXY_CMD_NOT_SUPPORTED;
		break;
	}

	return result_code;
}

static uint8_t paused_state_command_handler(const struct mpl_cmd *command)
{
	uint8_t result_code = MEDIA_PROXY_CMD_SUCCESS;

	LOG_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_MPL_LOG_LEVEL_DBG)) {
		if (command->use_param) {
			LOG_DBG("Command parameter: %d", command->param);
		}
	}

	switch (command->opcode) {
	case MEDIA_PROXY_OP_PLAY:
		mpl_set_state(MEDIA_PROXY_STATE_PLAYING);
		break;
	case MEDIA_PROXY_OP_PAUSE:
		/* No change */
		break;
	case MEDIA_PROXY_OP_FAST_REWIND:
		/* We're in paused state, seeking speed must have been zero */
		media_player.seeking_speed_factor = -MPL_SEEKING_SPEED_FACTOR_STEP;
		mpl_set_state(MEDIA_PROXY_STATE_SEEKING);
		media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		break;
	case MEDIA_PROXY_OP_FAST_FORWARD:
		/* We're in paused state, seeking speed must have been zero */
		media_player.seeking_speed_factor = MPL_SEEKING_SPEED_FACTOR_STEP;
		mpl_set_state(MEDIA_PROXY_STATE_SEEKING);
		media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		break;
	case MEDIA_PROXY_OP_STOP:
		set_track_position(0);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_MOVE_RELATIVE:
		if (command->use_param) {
			set_relative_track_position(command->param);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_PREV_SEGMENT:
		/* Switch to previous segment if we are less than 5 seconds */
		/* into the segment, otherwise go to start of segment */
		if (media_player.group->track->segment != NULL) {
			if (media_player.track_pos - PREV_MARGIN <
			    media_player.group->track->segment->pos) {
				do_prev_segment(&media_player);
			}

			set_track_position(media_player.group->track->segment->pos);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_NEXT_SEGMENT:
		if (media_player.group->track->segment != NULL) {
			do_next_segment(&media_player);
			set_track_position(media_player.group->track->segment->pos);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_FIRST_SEGMENT:
		if (media_player.group->track->segment != NULL) {
			do_first_segment(&media_player);
			set_track_position(media_player.group->track->segment->pos);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_LAST_SEGMENT:
		if (media_player.group->track->segment != NULL) {
			do_last_segment(&media_player);
			set_track_position(media_player.group->track->segment->pos);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_GOTO_SEGMENT:
		if (command->use_param && media_player.group->track->segment != NULL) {
			if (command->param != 0) {
				do_goto_segment(&media_player, command->param);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shall stay the same, and the */
			/* track position shall not change. */
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_PREV_TRACK:
		do_prev_track(&media_player);
		break;
	case MEDIA_PROXY_OP_NEXT_TRACK:
		do_next_track(&media_player);
		/* For next track, the position is kept if the track */
		/* does not change */
		break;
	case MEDIA_PROXY_OP_FIRST_TRACK:
		do_first_track(&media_player, false);
		break;
	case MEDIA_PROXY_OP_LAST_TRACK:
		do_last_track(&media_player);
		break;
	case MEDIA_PROXY_OP_GOTO_TRACK:
		if (command->use_param) {
			do_goto_track(&media_player, command->param);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_PREV_GROUP:
		do_full_prev_group(&media_player);
		break;
	case MEDIA_PROXY_OP_NEXT_GROUP:
		do_full_next_group(&media_player);
		break;
	case MEDIA_PROXY_OP_FIRST_GROUP:
		do_full_first_group(&media_player);
		break;
	case MEDIA_PROXY_OP_LAST_GROUP:
		do_full_last_group(&media_player);
		break;
	case MEDIA_PROXY_OP_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&media_player, command->param);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	default:
		LOG_DBG("Invalid command: %d", command->opcode);
		result_code = MEDIA_PROXY_CMD_NOT_SUPPORTED;
		break;
	}

	return result_code;
}

static uint8_t seeking_state_command_handler(const struct mpl_cmd *command)
{
	uint8_t result_code = MEDIA_PROXY_CMD_SUCCESS;

	LOG_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_MPL_LOG_LEVEL_DBG)) {
		if (command->use_param) {
			LOG_DBG("Command parameter: %d", command->param);
		}
	}

	switch (command->opcode) {
	case MEDIA_PROXY_OP_PLAY:
		media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
		mpl_set_state(MEDIA_PROXY_STATE_PLAYING);
		media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		break;
	case MEDIA_PROXY_OP_PAUSE:
		media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
		/* TODO: Set track and track position */
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		break;
	case MEDIA_PROXY_OP_FAST_REWIND:
		/* TODO: Here, and for FAST_FORWARD */
		/* Decide on algorithm for multiple presses - add step (as */
		/* now) or double/half? */
		/* What about FR followed by FF? */
		/* Currently, the seeking speed may also become	 zero */
		/* Lowest value allowed by spec is -64, notify on change only */
		if (media_player.seeking_speed_factor >= -(MEDIA_PROXY_SEEKING_SPEED_FACTOR_MAX
						 - MPL_SEEKING_SPEED_FACTOR_STEP)) {
			media_player.seeking_speed_factor -= MPL_SEEKING_SPEED_FACTOR_STEP;
			media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		}
		break;
	case MEDIA_PROXY_OP_FAST_FORWARD:
		/* Highest value allowed by spec is 64, notify on change only */
		if (media_player.seeking_speed_factor <= (MEDIA_PROXY_SEEKING_SPEED_FACTOR_MAX
						- MPL_SEEKING_SPEED_FACTOR_STEP)) {
			media_player.seeking_speed_factor += MPL_SEEKING_SPEED_FACTOR_STEP;
			media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		}
		break;
	case MEDIA_PROXY_OP_STOP:
		media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
		set_track_position(0);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
		break;
	case MEDIA_PROXY_OP_MOVE_RELATIVE:
		if (command->use_param) {
			set_relative_track_position(command->param);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}

		break;
	case MEDIA_PROXY_OP_PREV_SEGMENT:
		/* Switch to previous segment if we are less than 5 seconds */
		/* into the segment, otherwise go to start of segment */
		if (media_player.track_pos - PREV_MARGIN <
		    media_player.group->track->segment->pos) {
			do_prev_segment(&media_player);
		}
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_NEXT_SEGMENT:
		do_next_segment(&media_player);
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_FIRST_SEGMENT:
		do_first_segment(&media_player);
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_LAST_SEGMENT:
		do_last_segment(&media_player);
		set_track_position(media_player.group->track->segment->pos);
		break;
	case MEDIA_PROXY_OP_GOTO_SEGMENT:
		if (command->use_param) {
			if (command->param != 0) {
				do_goto_segment(&media_player, command->param);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shall stay the same, and the */
			/* track position shall not change. */
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}
		break;
	case MEDIA_PROXY_OP_PREV_TRACK:
		do_prev_track(&media_player);
		media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_NEXT_TRACK:
		do_next_track(&media_player);
		/* For next track, the position is kept if the track */
		/* does not change */
		media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_FIRST_TRACK:
		do_first_track(&media_player, false);
		media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_LAST_TRACK:
		do_last_track(&media_player);
		media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_GOTO_TRACK:
		if (command->use_param) {
			do_goto_track(&media_player, command->param);
			media_player.seeking_speed_factor = MEDIA_PROXY_SEEKING_SPEED_FACTOR_ZERO;
			mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}
		break;
	case MEDIA_PROXY_OP_PREV_GROUP:
		do_full_prev_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_NEXT_GROUP:
		do_full_next_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_FIRST_GROUP:
		do_full_first_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_LAST_GROUP:
		do_full_last_group(&media_player);
		mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		break;
	case MEDIA_PROXY_OP_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&media_player, command->param);
			mpl_set_state(MEDIA_PROXY_STATE_PAUSED);
		} else {
			result_code = MEDIA_PROXY_CMD_CANNOT_BE_COMPLETED;
		}
		break;
	default:
		LOG_DBG("Invalid command: %d", command->opcode);
		result_code = MEDIA_PROXY_CMD_NOT_SUPPORTED;
		break;
	}

	return result_code;
}

static uint8_t (*command_handlers[MEDIA_PROXY_STATE_LAST])(const struct mpl_cmd *command) = {
	inactive_state_command_handler,
	playing_state_command_handler,
	paused_state_command_handler,
	seeking_state_command_handler,
};

#ifdef CONFIG_BT_MPL_OBJECTS
/* Find a track by ID
 *
 * If found, return pointers to the group of the track and the track,
 * otherwise, the pointers returned are NULL
 *
 * Returns true if found, false otherwise
 */
static bool find_track_by_id(const struct mpl_mediaplayer *pl, uint64_t id,
			     struct mpl_group **group, struct mpl_track **track)
{
	struct mpl_group *tmp_group = pl->group;
	struct mpl_track *tmp_track;

	while (tmp_group->prev != NULL) {
		tmp_group = tmp_group->prev;
	}

	while (tmp_group != NULL) {
		tmp_track = tmp_group->track;

		while (tmp_track->prev != NULL) {
			tmp_track = tmp_track->prev;
		}

		while (tmp_track != 0) {
			if (tmp_track->id == id) {
				/* Found the track */
				*group = tmp_group;
				*track = tmp_track;
				return true;
			}

			tmp_track = tmp_track->next;
		}

		tmp_group = tmp_group->next;
	}

	/* Track not found */
	*group = NULL;
	*track = NULL;
	return false;
}

/* Find a group by ID
 *
 * If found, return pointer to the group, otherwise, the pointer returned is NULL
 *
 * Returns true if found, false otherwise
 */
static bool find_group_by_id(const struct mpl_mediaplayer *pl, uint64_t id,
			     struct mpl_group **group)
{
	struct mpl_group *tmp_group = pl->group;

	while (tmp_group->prev != NULL) {
		tmp_group = tmp_group->prev;
	}

	while (tmp_group != NULL) {
		if (tmp_group->id == id) {
			/* Found the group */
			*group = tmp_group;
			return true;
		}

		tmp_group = tmp_group->next;
	}

	/* Group not found */
	*group = NULL;
	return false;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

static const char *get_player_name(void)
{
	return media_player.name;
}

#ifdef CONFIG_BT_MPL_OBJECTS
static uint64_t get_icon_id(void)
{
	return media_player.icon_id;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

static const char *get_icon_url(void)
{
	return media_player.icon_url;
}

static const char *get_track_title(void)
{
	return media_player.group->track->title;
}

static int32_t get_track_duration(void)
{
	return media_player.group->track->duration;
}

static int32_t get_track_position(void)
{
	return media_player.track_pos;
}

static void set_track_position(int32_t position)
{
	int32_t old_pos = media_player.track_pos;
	int32_t new_pos;

	if (position >= 0) {
		if (position > media_player.group->track->duration) {
			/* Do not go beyond end of track */
			new_pos = media_player.group->track->duration;
		} else {
			new_pos = position;
		}
	} else {
		/* Negative position, handle as offset from _end_ of track */
		/* (Note minus sign below) */
		if (position < -media_player.group->track->duration) {
			new_pos = 0;
		} else {
			/* (Remember position is negative) */
			new_pos = media_player.group->track->duration + position;
		}
	}

	LOG_DBG("Pos. given: %d, resulting pos.: %d (duration is %d)", position, new_pos,
		media_player.group->track->duration);

	/* Notify when the position changes when not in the playing state, or if the position is set
	 * to 0 which is a special value that typically indicates that the track has stopped or
	 * changed. Since this might occur when media_player.group->track->duration is still 0, we
	 * should always notify this value.
	 */
	if (new_pos != old_pos || new_pos == 0) {
		/* Set new position and notify it */
		media_player.track_pos = new_pos;

		/* MCS 1.0, section 3.7.1, states:
		 * to avoid an excessive number of notifications, the Track Position should
		 * not be notified when the Media State is set to “Playing” and playback happens
		 * at a constant speed.
		 */
		if (media_player.state != MEDIA_PROXY_STATE_PLAYING) {
			media_proxy_pl_track_position_cb(new_pos);
		}
	}
}

static void set_relative_track_position(int32_t rel_pos)
{
	int64_t pos;

	pos = media_player.track_pos + rel_pos;
	/* Clamp to allowed values */
	pos = CLAMP(pos, 0, media_player.group->track->duration);

	set_track_position((int32_t)pos);
}

static int8_t get_playback_speed(void)
{
	return media_player.playback_speed_param;
}

static void set_playback_speed(int8_t speed)
{
	/* Set new speed parameter and notify, if different from current */
	if (speed != media_player.playback_speed_param) {
		media_player.playback_speed_param = speed;
		media_proxy_pl_playback_speed_cb(media_player.playback_speed_param);
	}
}

static int8_t get_seeking_speed(void)
{
	return media_player.seeking_speed_factor;
}

#ifdef CONFIG_BT_MPL_OBJECTS
static uint64_t get_track_segments_id(void)
{
	return media_player.group->track->segments_id;
}

static uint64_t get_current_track_id(void)
{
	return media_player.group->track->id;
}

static void set_current_track_id(uint64_t id)
{
	struct mpl_group *group;
	struct mpl_track *track;

	LOG_DBG_OBJ_ID("Track ID to set: ", id);

	if (find_track_by_id(&media_player, id, &group, &track)) {
		if (media_player.group != group) {
			media_player.group = group;
			do_group_change_notifications(&media_player);

			/* Group change implies track change (even if same track in other group) */
			media_player.group->track = track;
			do_track_change_notifications(&media_player);

		} else if (media_player.group->track != track) {
			media_player.group->track = track;
			do_track_change_notifications(&media_player);
		}
		return;
	}

	LOG_DBG("Track not found");

	/* TODO: Should an error be returned here?
	 * That would require a rewrite of the MPL api to add return values to the functions.
	 */
}

static uint64_t get_next_track_id(void)
{
	/* If the next track has been set explicitly */
	if (media_player.next_track_set) {
		return media_player.next.track->id;
	}

	/* Normal playing order */
	if (media_player.group->track->next) {
		return media_player.group->track->next->id;
	}

	/* Return zero value to indicate that there is no next track */
	return MPL_NO_TRACK_ID;
}

static void set_next_track_id(uint64_t id)
{
	struct mpl_group *group;
	struct mpl_track *track;

	LOG_DBG_OBJ_ID("Next Track ID to set: ", id);

	if (find_track_by_id(&media_player, id, &group, &track)) {

		media_player.next_track_set = true;
		media_player.next.group = group;
		media_player.next.track = track;
		media_proxy_pl_next_track_id_cb(id);
		return;
	}

	LOG_DBG("Track not found");
}

static uint64_t get_parent_group_id(void)
{
	return media_player.group->parent->id;
}

static uint64_t get_current_group_id(void)
{
	return media_player.group->id;
}

static void set_current_group_id(uint64_t id)
{
	struct mpl_group *group;

	LOG_DBG_OBJ_ID("Group ID to set: ", id);

	if (find_group_by_id(&media_player, id, &group)) {

		if (media_player.group != group) {
			/* Change to found group */
			media_player.group = group;
			do_group_change_notifications(&media_player);

			/* And change to first track in group */
			do_first_track(&media_player, false);
		}
		return;
	}

	LOG_DBG("Group not found");
}
#endif /* CONFIG_BT_MPL_OBJECTS */

static uint8_t get_playing_order(void)
{
	return media_player.playing_order;
}

static void set_playing_order(uint8_t order)
{
	if (order != media_player.playing_order) {
		if (BIT(order - 1) & media_player.playing_orders_supported) {
			media_player.playing_order = order;
			media_proxy_pl_playing_order_cb(media_player.playing_order);
		}
	}
}

static uint16_t get_playing_orders_supported(void)
{
	return media_player.playing_orders_supported;
}

static uint8_t get_media_state(void)
{
	return media_player.state;
}

static void send_command(const struct mpl_cmd *command)
{
	struct mpl_cmd_ntf ntf;

	if (command->use_param) {
		LOG_DBG("opcode: %d, param: %d", command->opcode, command->param);
	} else {
		LOG_DBG("opcode: %d", command->opcode);
	}

	if (media_player.state < MEDIA_PROXY_STATE_LAST) {
		ntf.requested_opcode = command->opcode;
		ntf.result_code = command_handlers[media_player.state](command);

		media_proxy_pl_command_cb(&ntf);
	} else {
		LOG_DBG("INVALID STATE");
	}
}

static uint32_t get_commands_supported(void)
{
	return media_player.opcodes_supported;
}

#ifdef CONFIG_BT_MPL_OBJECTS

static bool parse_sci(struct bt_data *data, void *user_data)
{
	LOG_DBG("type: %u len %u", data->type, data->data_len);
	LOG_HEXDUMP_DBG(data->data, data->data_len, "param:");

	if (data->type < MEDIA_PROXY_SEARCH_TYPE_TRACK_NAME ||
	    data->type > MEDIA_PROXY_SEARCH_TYPE_ONLY_GROUPS) {
		LOG_DBG("Invalid search type: %u", data->type);
		return false;
	}

	return true;
}

static void parse_search(const struct mpl_search *search)
{
	bool search_failed = false;

	if (search->len > SEARCH_LEN_MAX) {
		LOG_WRN("Search too long (%d) - aborting", search->len);
		search_failed = true;
	} else {
		uint8_t search_ltv[SEARCH_LEN_MAX];
		struct net_buf_simple buf;

		/* Copy so that we can parse it using the net_buf_simple when search is const */
		memcpy(search_ltv, search->search, search->len);

		net_buf_simple_init_with_data(&buf, search_ltv, search->len);

		bt_data_parse(&buf, parse_sci, NULL);

		if (buf.len != 0U) {
			search_failed = true;
		}
	}

	/* TODO: Add real search functionality. */
	/* For now, just fake it. */

	if (search_failed) {
		media_player.search_results_id = 0;
		media_proxy_pl_search_cb(MEDIA_PROXY_SEARCH_FAILURE);
	} else {
		/* Use current group as search result for now */
		media_player.search_results_id = media_player.group->id;
		media_proxy_pl_search_cb(MEDIA_PROXY_SEARCH_SUCCESS);
	}

	media_proxy_pl_search_results_id_cb(media_player.search_results_id);
}

static void send_search(const struct mpl_search *search)
{
	if (search->len > SEARCH_LEN_MAX) {
		LOG_WRN("Search too long: %d", search->len);
	}

	LOG_HEXDUMP_DBG(search->search, search->len, "Search");

	parse_search(search);
}

static uint64_t get_search_results_id(void)
{
	return media_player.search_results_id;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

static uint8_t get_content_ctrl_id(void)
{
	return media_player.content_ctrl_id;
}

static void pos_work_cb(struct k_work *work)
{
	const int32_t pos_diff_cs = TRACK_POS_WORK_DELAY_MS / 10; /* position is in centiseconds*/

	if (media_player.state == MEDIA_PROXY_STATE_SEEKING) {
		/* When seeking, apply the seeking speed factor */
		set_relative_track_position(pos_diff_cs * media_player.seeking_speed_factor);
	} else if (media_player.state == MEDIA_PROXY_STATE_PLAYING) {
		set_relative_track_position(pos_diff_cs);
	}

	if (media_player.track_pos == media_player.group->track->duration) {
		/* Go to next track */
		do_next_track(&media_player);
	}

	(void)k_work_schedule(&media_player.pos_work, TRACK_POS_WORK_DELAY);
}

int media_proxy_pl_init(void)
{
	static bool initialized;
	int ret;

	if (initialized) {
		LOG_DBG("Already initialized");
		return -EALREADY;
	}

	/* Get a Content Control ID */
	ret = bt_ccid_alloc_value();
	if (ret < 0) {
		LOG_DBG("Could not allocate CCID: %d", ret);
		return ret;
	}
	media_player.content_ctrl_id = (uint8_t)ret;

	/* Set up the media control service */
	/* TODO: Fix initialization - who initializes what
	 * https://github.com/zephyrproject-rtos/zephyr/issues/42965
	 * Temporarily only initializing if service is present
	 */
#ifdef CONFIG_BT_MCS
#ifdef CONFIG_BT_MPL_OBJECTS
	/* The test here is arguably needed as the objects cannot be accessed before bt_mcs_init is
	 * called, but the set is to avoid the objects being accessed before properly initialized
	 */
	if (atomic_test_and_set_bit(obj.flags, MPL_OBJ_FLAG_BUSY)) {
		LOG_ERR("Object busy");
		return -EBUSY;
	}

	ret = bt_mcs_init(&ots_cbs);
	if (ret < 0) {
		LOG_ERR("Could not init MCS: %d", ret);
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);

		return ret;
	}
#else
	ret = bt_mcs_init(NULL);
	if (ret < 0) {
		LOG_ERR("Could not init MCS: %d", ret);
		return ret;
	}
#endif  /* CONFIG_BT_MPL_OBJECTS */
	/* TODO: If anything below fails we should unregister MCS */
#else
	LOG_WRN("MCS not configured");
#endif /* CONFIG_BT_MCS */

#ifdef CONFIG_BT_MPL_OBJECTS
	/* Initialize the object content buffer */
	net_buf_simple_init(obj.content, 0);

	/* Icon Object */
	ret = add_icon_object(&media_player);
	if (ret < 0) {
		LOG_ERR("Unable to add icon object, error %d", ret);
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
		return ret;
	}

	/* Add all tracks and groups to OTS */
	ret = add_group_and_track_objects(&media_player);
	if (ret < 0) {
		LOG_ERR("Error adding tracks and groups to OTS, error %d", ret);
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
		return ret;
	}

	/* Initial setup of Track Segments Object */
	/* TODO: Later, this should be done when the tracks are added */
	/* but for no only one of the tracks has segments .*/
	ret = add_current_track_segments_object(&media_player);
	if (ret < 0) {
		LOG_ERR("Error adding Track Segments Object to OTS, error %d", ret);
		atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
		return ret;
	}

	atomic_clear_bit(obj.flags, MPL_OBJ_FLAG_BUSY);
#endif /* CONFIG_BT_MPL_OBJECTS */

	/* Set up the calls structure */
	media_player.calls.get_player_name              = get_player_name;
#ifdef CONFIG_BT_MPL_OBJECTS
	media_player.calls.get_icon_id                  = get_icon_id;
#endif /* CONFIG_BT_MPL_OBJECTS */
	media_player.calls.get_icon_url                 = get_icon_url;
	media_player.calls.get_track_title              = get_track_title;
	media_player.calls.get_track_duration           = get_track_duration;
	media_player.calls.get_track_position           = get_track_position;
	media_player.calls.set_track_position           = set_track_position;
	media_player.calls.get_playback_speed           = get_playback_speed;
	media_player.calls.set_playback_speed           = set_playback_speed;
	media_player.calls.get_seeking_speed            = get_seeking_speed;
#ifdef CONFIG_BT_MPL_OBJECTS
	media_player.calls.get_track_segments_id        = get_track_segments_id;
	media_player.calls.get_current_track_id         = get_current_track_id;
	media_player.calls.set_current_track_id         = set_current_track_id;
	media_player.calls.get_next_track_id            = get_next_track_id;
	media_player.calls.set_next_track_id            = set_next_track_id;
	media_player.calls.get_parent_group_id          = get_parent_group_id;
	media_player.calls.get_current_group_id         = get_current_group_id;
	media_player.calls.set_current_group_id         = set_current_group_id;
#endif /* CONFIG_BT_MPL_OBJECTS */
	media_player.calls.get_playing_order            = get_playing_order;
	media_player.calls.set_playing_order            = set_playing_order;
	media_player.calls.get_playing_orders_supported = get_playing_orders_supported;
	media_player.calls.get_media_state              = get_media_state;
	media_player.calls.send_command                 = send_command;
	media_player.calls.get_commands_supported       = get_commands_supported;
#ifdef CONFIG_BT_MPL_OBJECTS
	media_player.calls.send_search                  = send_search;
	media_player.calls.get_search_results_id        = get_search_results_id;
#endif /* CONFIG_BT_MPL_OBJECTS */
	media_player.calls.get_content_ctrl_id          = get_content_ctrl_id;

	ret = media_proxy_pl_register(&media_player.calls);
	if (ret < 0) {
		LOG_ERR("Unable to register player");
		return ret;
	}

	k_work_init_delayable(&media_player.pos_work, pos_work_cb);

	initialized = true;
	return 0;
}

#if CONFIG_BT_MPL_LOG_LEVEL_DBG /* Special commands for debugging */

void mpl_debug_dump_state(void)
{
#if CONFIG_BT_MPL_OBJECTS
	char t[BT_OTS_OBJ_ID_STR_LEN];
	struct mpl_group *group;
	struct mpl_track *track;
#endif /* CONFIG_BT_MPL_OBJECTS */

	LOG_DBG("Mediaplayer name: %s", media_player.name);

#if CONFIG_BT_MPL_OBJECTS
	(void)bt_ots_obj_id_to_str(media_player.icon_id, t, sizeof(t));
	LOG_DBG("Icon ID: %s", t);
#endif /* CONFIG_BT_MPL_OBJECTS */

	LOG_DBG("Icon URL: %s", media_player.icon_url);
	LOG_DBG("Track position: %d", media_player.track_pos);
	LOG_DBG("Media state: %d", media_player.state);
	LOG_DBG("Playback speed parameter: %d", media_player.playback_speed_param);
	LOG_DBG("Seeking speed factor: %d", media_player.seeking_speed_factor);
	LOG_DBG("Playing order: %d", media_player.playing_order);
	LOG_DBG("Playing orders supported: 0x%x", media_player.playing_orders_supported);
	LOG_DBG("Opcodes supported: %d", media_player.opcodes_supported);
	LOG_DBG("Content control ID: %d", media_player.content_ctrl_id);

#if CONFIG_BT_MPL_OBJECTS
	(void)bt_ots_obj_id_to_str(media_player.group->parent->id, t, sizeof(t));
	LOG_DBG("Current group's parent: %s", t);

	(void)bt_ots_obj_id_to_str(media_player.group->id, t, sizeof(t));
	LOG_DBG("Current group: %s", t);

	(void)bt_ots_obj_id_to_str(media_player.group->track->id, t, sizeof(t));
	LOG_DBG("Current track: %s", t);

	if (media_player.next_track_set) {
		(void)bt_ots_obj_id_to_str(media_player.next.track->id, t, sizeof(t));
		LOG_DBG("Next track: %s", t);
	} else if (media_player.group->track->next) {
		(void)bt_ots_obj_id_to_str(media_player.group->track->next->id, t,
					   sizeof(t));
		LOG_DBG("Next track: %s", t);
	} else {
		LOG_DBG("No next track");
	}

	if (media_player.search_results_id) {
		(void)bt_ots_obj_id_to_str(media_player.search_results_id, t, sizeof(t));
		LOG_DBG("Search results: %s", t);
	} else {
		LOG_DBG("No search results");
	}

	LOG_DBG("Groups and tracks:");
	group = media_player.group;

	while (group->prev != NULL) {
		group = group->prev;
	}

	while (group) {
		(void)bt_ots_obj_id_to_str(group->id, t, sizeof(t));
		LOG_DBG("Group: %s, %s", t, group->title);

		(void)bt_ots_obj_id_to_str(group->parent->id, t, sizeof(t));
		LOG_DBG("\tParent: %s, %s", t, group->parent->title);

		track = group->track;
		while (track->prev != NULL) {
			track = track->prev;
		}

		while (track) {
			(void)bt_ots_obj_id_to_str(track->id, t, sizeof(t));
			LOG_DBG("\tTrack: %s, %s, duration: %d", t, track->title, track->duration);
			track = track->next;
		}

		group = group->next;
	}
#endif /* CONFIG_BT_MPL_OBJECTS */
}
#endif /* CONFIG_BT_MPL_LOG_LEVEL_DBG */

#if defined(CONFIG_BT_TESTING) /* Special commands for testing */

#if CONFIG_BT_MPL_OBJECTS
void mpl_test_unset_parent_group(void)
{
	LOG_DBG("Setting current group to be it's own parent");
	media_player.group->parent = media_player.group;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

void mpl_test_media_state_set(uint8_t state)
{
	mpl_set_state(state);
}

void mpl_test_player_name_changed_cb(void)
{
	media_proxy_pl_name_cb(media_player.name);
}

void mpl_test_player_icon_url_changed_cb(void)
{
	media_proxy_pl_icon_url_cb(media_player.icon_url);
}

void mpl_test_track_changed_cb(void)
{
	media_proxy_pl_track_changed_cb();
}

void mpl_test_title_changed_cb(void)
{
	media_proxy_pl_track_title_cb(media_player.group->track->title);
}

void mpl_test_duration_changed_cb(void)
{
	media_proxy_pl_track_duration_cb(media_player.group->track->duration);
}

void mpl_test_position_changed_cb(void)
{
	media_proxy_pl_track_position_cb(media_player.track_pos);
}

void mpl_test_playback_speed_changed_cb(void)
{
	media_proxy_pl_playback_speed_cb(media_player.playback_speed_param);
}

void mpl_test_seeking_speed_changed_cb(void)
{
	media_proxy_pl_seeking_speed_cb(media_player.seeking_speed_factor);
}

#ifdef CONFIG_BT_MPL_OBJECTS
void mpl_test_current_track_id_changed_cb(void)
{
	media_proxy_pl_current_track_id_cb(media_player.group->track->id);
}

void mpl_test_next_track_id_changed_cb(void)
{
	media_proxy_pl_next_track_id_cb(media_player.group->track->next->id);
}

void mpl_test_parent_group_id_changed_cb(void)
{
	media_proxy_pl_parent_group_id_cb(media_player.group->id);
}

void mpl_test_current_group_id_changed_cb(void)
{
	media_proxy_pl_current_group_id_cb(media_player.group->id);
}
#endif /* CONFIG_BT_MPL_OBJECTS */

void mpl_test_playing_order_changed_cb(void)
{
	media_proxy_pl_playing_order_cb(media_player.playing_order);
}

void mpl_test_media_state_changed_cb(void)
{
	media_proxy_pl_media_state_cb(media_player.playing_order);
}

void mpl_test_opcodes_supported_changed_cb(void)
{
	media_proxy_pl_commands_supported_cb(media_player.opcodes_supported);
}

#ifdef CONFIG_BT_MPL_OBJECTS
void mpl_test_search_results_changed_cb(void)
{
	media_proxy_pl_search_cb(media_player.search_results_id);
}
#endif /* CONFIG_BT_MPL_OBJECTS */

#endif /* CONFIG_BT_TESTING */
