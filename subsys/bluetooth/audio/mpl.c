/*  Media player skeleton implementation */

/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/audio/media_proxy.h>

#include "media_proxy_internal.h"
#include "mpl_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MPL)
#define LOG_MODULE_NAME bt_mpl
#include "common/log.h"
#include "ccid_internal.h"
#include "mcs_internal.h"

#define TRACK_STATUS_INVALID 0x00
#define TRACK_STATUS_VALID 0x01

#define PLAYBACK_SPEED_PARAM_DEFAULT BT_MCS_PLAYBACK_SPEED_UNITY

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

static struct mpl_mediaplayer pl = {
	.name			  = CONFIG_BT_MPL_MEDIA_PLAYER_NAME,
	.icon_url		  = CONFIG_BT_MPL_ICON_URL,
	.group			  = &group_1,
	.track_pos		  = 0,
	.state			  = BT_MCS_MEDIA_STATE_PAUSED,
	.playback_speed_param	  = PLAYBACK_SPEED_PARAM_DEFAULT,
	.seeking_speed_factor	  = BT_MCS_SEEKING_SPEED_FACTOR_ZERO,
	.playing_order		  = BT_MCS_PLAYING_ORDER_INORDER_REPEAT,
	.playing_orders_supported = BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_ONCE |
				    BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_REPEAT,
	.opcodes_supported	  = 0x001fffff, /* All opcodes */
#ifdef CONFIG_BT_MPL_OBJECTS
	.search_results_id	  = 0,
	.calls = { 0 },
#endif /* CONFIG_BT_MPL_OBJECTS */
	.next_track_set           = false
};

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

/* The active object */
/* Only a single object is selected or being added (active) at a time. */
/* And, except for the icon object, all objects can be created dynamically. */
/* So a single buffer to hold object content is sufficient.  */
struct obj_t {
	/* ID of the currently selected object*/
	uint64_t selected_id;

	bool busy;

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
};

static struct obj_t obj = {
	.selected_id = 0,
	.add_type = MPL_OBJ_NONE,
	.busy = false,
	.add_track = NULL,
	.add_group = NULL,
	.content = NET_BUF_SIMPLE(CONFIG_BT_MPL_MAX_OBJ_SIZE)
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
				BT_DBG("Segments object out of space");
				break;
			}
			net_buf_simple_add_u8(obj.content, seg->name_len);
			net_buf_simple_add_mem(obj.content, seg->name,
					       seg->name_len);
			net_buf_simple_add_le32(obj.content, seg->pos);

			tot_size += seg_size;
			seg = seg->next;
		}

		BT_HEXDUMP_DBG(obj.content->data, obj.content->len,
			       "Segments Object");
		BT_DBG("Segments object length: %d", obj.content->len);
	} else {
		BT_ERR("No seg!");
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
	/* poinbter in the other direction, so it is not possible to go from */
	/* the parent group to a group of tracks. */

	uint8_t type = BT_MCS_GROUP_OBJECT_GROUP_TYPE;
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
			BT_WARN("Not room for full group in object");
		}
		BT_HEXDUMP_DBG(obj.content->data, obj.content->len,
			       "Parent Group Object");
		BT_DBG("Group object length: %d", obj.content->len);
	}
	return obj.content->len;
}

/* Set up contents for a group object */
/* The group object contains a concatenated list of records, where each */
/* record consists of a type byte and a UUID */
static uint32_t setup_group_object(struct mpl_group *group)
{
	struct mpl_track *track = group->track;
	uint8_t type = BT_MCS_GROUP_OBJECT_TRACK_TYPE;
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
			BT_WARN("Not room for full group in object");
		}
		BT_HEXDUMP_DBG(obj.content->data, obj.content->len,
			       "Group Object");
		BT_DBG("Group object length: %d", obj.content->len);
	}
	return obj.content->len;
}

/* Add the icon object to the OTS */
static int add_icon_object(struct mpl_mediaplayer *pl)
{
	int ret;
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	struct bt_uuid *icon_type = BT_UUID_OTS_TYPE_MPL_ICON;
	static char *icon_name = "Icon";

	if (obj.busy) {
		/* TODO: Can there be a collision between select and internal */
		/* activities, like adding new objects? */
		BT_ERR("Object busy");
		return 0;
	}
	obj.busy = true;
	obj.add_type = MPL_OBJ_ICON;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_icon_object();
	obj.desc->name = icon_name;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(icon_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);

	if (ret) {
		BT_WARN("Unable to add icon object, error %d", ret);
		obj.busy = false;
	}
	return ret;
}

/* Add a track segments object to the OTS */
static int add_current_track_segments_object(struct mpl_mediaplayer *pl)
{
	int ret;
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	struct bt_uuid *segs_type = BT_UUID_OTS_TYPE_TRACK_SEGMENT;

	if (obj.busy) {
		BT_ERR("Object busy");
		return 0;
	}
	obj.busy = true;
	obj.add_type = MPL_OBJ_TRACK_SEGMENTS;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_segments_object(pl->group->track);
	obj.desc->name = pl->group->track->title;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(segs_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);
	if (ret) {
		BT_WARN("Unable to add track segments object: %d", ret);
		obj.busy = false;
	}
	return ret;
}

/* Add a single track to the OTS */
static int add_track_object(struct mpl_track *track)
{
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	struct bt_uuid *track_type = BT_UUID_OTS_TYPE_TRACK;
	int ret;

	if (obj.busy) {
		BT_ERR("Object busy");
		return 0;
	}
	if (!track) {
		BT_ERR("No track");
		return -EINVAL;
	}

	obj.busy = true;

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

	if (ret) {
		BT_WARN("Unable to add track object: %d", ret);
		obj.busy = false;
	}

	return ret;
}

/* Add the parent group to the OTS */
static int add_parent_group_object(struct mpl_mediaplayer *pl)
{
	int ret;
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	struct bt_uuid *group_type = BT_UUID_OTS_TYPE_GROUP;

	if (obj.busy) {
		BT_ERR("Object busy");
		return 0;
	}
	obj.busy = true;
	obj.add_type = MPL_OBJ_PARENT_GROUP;
	obj.desc = &created_desc;

	obj.desc->size.alloc = obj.desc->size.cur = setup_parent_group_object(pl->group);
	obj.desc->name = pl->group->parent->title;
	BT_OTS_OBJ_SET_PROP_READ(obj.desc->props);

	add_param.size = obj.desc->size.alloc;
	add_param.type.uuid.type = BT_UUID_TYPE_16;
	add_param.type.uuid_16.val = BT_UUID_16(group_type)->val;

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &add_param);
	if (ret) {
		BT_WARN("Unable to add parent group object");
		obj.busy = false;
	}
	return ret;
}

/* Add a single group to the OTS */
static int add_group_object(struct mpl_group *group)
{
	struct bt_ots_obj_add_param add_param = {};
	struct bt_ots_obj_created_desc created_desc = {};
	struct bt_uuid *group_type = BT_UUID_OTS_TYPE_GROUP;
	int ret;

	if (obj.busy) {
		BT_ERR("Object busy");
		return 0;
	}

	if (!group) {
		BT_ERR("No group");
		return -EINVAL;
	}

	obj.busy = true;

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

	if (ret) {
		BT_WARN("Unable to add group object: %d", ret);
		obj.busy = false;
	}

	return ret;
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
	BT_DBG_OBJ_ID("Object Id deleted: ", id);

	return 0;
}

static void on_obj_selected(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id)
{
	if (obj.busy) {
		/* TODO: Can there be a collision between select and internal */
		/* activities, like adding new objects? */
		BT_ERR("Object busy - select not performed");
		return;
	}
	obj.busy = true;

	BT_DBG_OBJ_ID("Object Id selected: ", id);

	if (id == pl.icon_id) {
		BT_DBG("Icon Object ID");
		(void)setup_icon_object();
	} else if (id == pl.group->track->segments_id) {
		BT_DBG("Current Track Segments Object ID");
		(void)setup_segments_object(pl.group->track);
	} else if (id == pl.group->track->id) {
		BT_DBG("Current Track Object ID");
		(void)setup_track_object(pl.group->track);
	} else if (pl.next_track_set && id == pl.next.track->id) {
		/* Next track, if the next track has been explicitly set */
		BT_DBG("Next Track Object ID");
		(void)setup_track_object(pl.next.track);
	} else if (id == pl.group->track->next->id) {
		/* Next track, if next track has not been explicitly set */
		BT_DBG("Next Track Object ID");
		(void)setup_track_object(pl.group->track->next);
	} else if (id == pl.group->parent->id) {
		BT_DBG("Parent Group Object ID");
		(void)setup_parent_group_object(pl.group);
	} else if (id == pl.group->id) {
		BT_DBG("Current Group Object ID");
		(void)setup_group_object(pl.group);
	} else {
		BT_ERR("Unknown Object ID");
		obj.busy = false;
		return;
	}

	obj.selected_id = id;
	obj.busy = false;
}

static int on_obj_created(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			  const struct bt_ots_obj_add_param *add_param,
			  struct bt_ots_obj_created_desc *created_desc)
{
	BT_DBG_OBJ_ID("Object Id created: ", id);

	*created_desc = *obj.desc;

	if (!bt_uuid_cmp(&add_param->type.uuid, BT_UUID_OTS_TYPE_MPL_ICON)) {
		BT_DBG("Icon Obj Type");
		if (obj.add_type == MPL_OBJ_ICON) {
			obj.add_type = MPL_OBJ_NONE;
			pl.icon_id = id;
		} else {
			BT_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&add_param->type.uuid,
				BT_UUID_OTS_TYPE_TRACK_SEGMENT)) {
		BT_DBG("Track Segments Obj Type");
		if (obj.add_type == MPL_OBJ_TRACK_SEGMENTS) {
			obj.add_type = MPL_OBJ_NONE;
			pl.group->track->segments_id = id;
		} else {
			BT_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&add_param->type.uuid,
				 BT_UUID_OTS_TYPE_TRACK)) {
		BT_DBG("Track Obj Type");
		if (obj.add_type == MPL_OBJ_TRACK) {
			obj.add_type = MPL_OBJ_NONE;
			obj.add_track->id = id;
			obj.add_track = NULL;
		} else {
			BT_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&add_param->type.uuid,
				 BT_UUID_OTS_TYPE_GROUP)) {
		BT_DBG("Group Obj Type");
		if (obj.add_type == MPL_OBJ_PARENT_GROUP) {
			BT_DBG("Parent group");
			obj.add_type = MPL_OBJ_NONE;
			pl.group->parent->id = id;
		} else if (obj.add_type == MPL_OBJ_GROUP) {
			BT_DBG("Other group");
			obj.add_type = MPL_OBJ_NONE;
			obj.add_group->id = id;
			obj.add_group = NULL;
		} else {
			BT_DBG("Unexpected object creation");
		}

	} else {
		BT_DBG("Unknown Object ID");
	}

	if (obj.add_type == MPL_OBJ_NONE) {
		obj.busy = false;
	}
	return 0;
}


static ssize_t on_object_send(struct bt_ots *ots, struct bt_conn *conn,
			      uint64_t id, void **data, size_t len,
			      off_t offset)
{
	if (obj.busy) {
		/* TODO: Can there be a collision between select and internal */
		/* activities, like adding new objects? */
		BT_ERR("Object busy");
		return 0;
	}
	obj.busy = true;

	if (IS_ENABLED(CONFIG_BT_DEBUG_MPL)) {
		char t[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, t, sizeof(t));
		BT_DBG("Object Id %s, offset %lu, length %zu", log_strdup(t),
		       (long)offset, len);
	}

	if (id != obj.selected_id) {
		BT_ERR("Read from unselected object");
		obj.busy = false;
		return 0;
	}

	if (!data) {
		BT_DBG("Read complete");
		obj.busy = false;
		return 0;
	}

	if (offset >= obj.content->len) {
		BT_DBG("Offset too large");
		obj.busy = false;
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_DEBUG_MPL)) {
		if (len > obj.content->len - offset) {
			BT_DBG("Requested len too large");
		}
	}

	*data = &obj.content->data[offset];
	obj.busy = false;
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

void do_prev_segment(struct mpl_mediaplayer *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	if (pl->group->track->segment->prev != NULL) {
		pl->group->track->segment = pl->group->track->segment->prev;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_next_segment(struct mpl_mediaplayer *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	if (pl->group->track->segment->next != NULL) {
		pl->group->track->segment = pl->group->track->segment->next;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_first_segment(struct mpl_mediaplayer *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	while (pl->group->track->segment->prev != NULL) {
		pl->group->track->segment = pl->group->track->segment->prev;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_last_segment(struct mpl_mediaplayer *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	while (pl->group->track->segment->next != NULL) {
		pl->group->track->segment = pl->group->track->segment->next;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_goto_segment(struct mpl_mediaplayer *pl, int32_t segnum)
{
	int32_t k;

	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

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

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

static bool do_prev_track(struct mpl_mediaplayer *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
		track_changed = true;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return track_changed;
}

/* Change to next track according to the current track's next track */
static bool do_next_track_normal_order(struct mpl_mediaplayer *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
		track_changed = true;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return track_changed;
}

/* Change to next track when the next track has been explicitly set
 *
 * ALWAYS changes the track, changes the group if required
 * Resets the next_track_set and the "next" pointers
 *
 * Returns true if the _group_ has been changed, otherwise false
 */
static bool do_next_track_next_track_set(struct mpl_mediaplayer *pl)
{
	bool group_changed = false;

	if (pl->next.group != pl->group) {
		pl->group = pl->next.group;
		group_changed = true;
	}

	pl->group->track = pl->next.track;

	pl->next.track = NULL;
	pl->next.group = NULL;
	pl->next_track_set = false;

	return group_changed;
}

static bool do_first_track(struct mpl_mediaplayer *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
		track_changed = true;
	}
	while (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return track_changed;
}

static bool do_last_track(struct mpl_mediaplayer *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
		track_changed = true;
	}
	while (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return track_changed;
}

static bool do_goto_track(struct mpl_mediaplayer *pl, int32_t tracknum)
{
	int32_t count = 0;
	int32_t k;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
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
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	/* The track has changed if we have moved more in one direction */
	/* than in the other */
	return (count != 0);
}


static bool do_prev_group(struct mpl_mediaplayer *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
		group_changed = true;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return group_changed;
}

static bool do_next_group(struct mpl_mediaplayer *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->next != NULL) {
		pl->group = pl->group->next;
		group_changed = true;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return group_changed;
}

static bool do_first_group(struct mpl_mediaplayer *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
		group_changed = true;
	}
	while (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return group_changed;
}

static bool do_last_group(struct mpl_mediaplayer *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	if (pl->group->next != NULL) {
		pl->group = pl->group->next;
		group_changed = true;
	}
	while (pl->group->next != NULL) {
		pl->group = pl->group->next;
	}

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	return group_changed;
}

static bool  do_goto_group(struct mpl_mediaplayer *pl, int32_t groupnum)
{
	int32_t count = 0;
	int32_t k;

#ifdef CONFIG_BT_MPL_OBJECTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
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
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */

	/* The group has changed if we have moved more in one direction */
	/* than in the other */
	return (count != 0);
}

void do_track_change_notifications(struct mpl_mediaplayer *pl)
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

void do_group_change_notifications(struct mpl_mediaplayer *pl)
{
#ifdef CONFIG_BT_MPL_OBJECTS
	media_proxy_pl_current_group_id_cb(pl->group->id);
#endif /* CONFIG_BT_MPL_OBJECTS */
}

void do_full_prev_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on first group) */
	if (do_prev_group(pl)) {
		do_group_change_notifications(pl);
		/* If group change, also go to first track in group. */
		/* Notify the track info in all cases - it is a track */
		/* change for the player even if the group was set to */
		/* this track already. */
		(void) do_first_track(pl);
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no group change, still switch to first track, if needed */
	} else if (do_first_track(pl)) {
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no track change, still reset track position, if needed */
	} else if (pl->track_pos != 0) {
		pl->track_pos = 0;
		media_proxy_pl_track_position_cb(pl->track_pos);
	}
}

void do_full_next_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on last group) */
	if (do_next_group(pl)) {
		do_group_change_notifications(pl);
		/* If group change, also go to first track in group. */
		/* Notify the track info in all cases - it is a track */
		/* change for the player even if the group was set to */
		/* this track already. */
		(void) do_first_track(pl);
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no group change, still switch to first track, if needed */
	} else if (do_first_track(pl)) {
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no track change, still reset track position, if needed */
	} else if (pl->track_pos != 0) {
		pl->track_pos = 0;
		media_proxy_pl_track_position_cb(pl->track_pos);
	}
}

void do_full_first_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on first group) */
	if (do_first_group(pl)) {
		do_group_change_notifications(pl);
		/* If group change, also go to first track in group. */
		/* Notify the track info in all cases - it is a track */
		/* change for the player even if the group was set to */
		/* this track already. */
		(void) do_first_track(pl);
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no group change, still switch to first track, if needed */
	} else if (do_first_track(pl)) {
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no track change, still reset track position, if needed */
	} else if (pl->track_pos != 0) {
		pl->track_pos = 0;
		media_proxy_pl_track_position_cb(pl->track_pos);
	}
}

void do_full_last_group(struct mpl_mediaplayer *pl)
{
	/* Change the group (if not already on last group) */
	if (do_last_group(pl)) {
		do_group_change_notifications(pl);
		/* If group change, also go to first track in group. */
		/* Notify the track info in all cases - it is a track */
		/* change for the player even if the group was set to */
		/* this track already. */
		(void) do_first_track(pl);
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no group change, still switch to first track, if needed */
	} else if (do_first_track(pl)) {
		pl->track_pos = 0;
		do_track_change_notifications(pl);
	/* If no track change, still reset track position, if needed */
	} else if (pl->track_pos != 0) {
		pl->track_pos = 0;
		media_proxy_pl_track_position_cb(pl->track_pos);
	}
}

void do_full_goto_group(struct mpl_mediaplayer *pl, int32_t groupnum)
{
	/* Change the group (if not already on given group) */
	if (do_goto_group(pl, groupnum)) {
		do_group_change_notifications(pl);
		/* If group change, also go to first track in group. */
		/* Notify the track info in all cases - it is a track */
		/* change for the player even if the group was set to */
		/* this track already. */
		(void) do_first_track(pl);
		pl->track_pos = 0;
		do_track_change_notifications(pl);
		/* If no group change, still switch to first track, if needed */
	} else if (do_first_track(pl)) {
		pl->track_pos = 0;
		do_track_change_notifications(pl);
		/* If no track change, still reset track position, if needed */
	} else if (pl->track_pos != 0) {
		pl->track_pos = 0;
		media_proxy_pl_track_position_cb(pl->track_pos);
	}
}

/* Command handlers (state machines) */
void inactive_state_command_handler(const struct mpl_cmd *command,
				    struct mpl_cmd_ntf *ntf)
{
	BT_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MPL)) {
		if (command->use_param) {
			BT_DBG("Command parameter: %d", command->param);
		}
	}
	switch (command->opcode) {
	case BT_MCS_OPC_PLAY: /* Fall-through - handle several cases identically */
	case BT_MCS_OPC_PAUSE:
	case BT_MCS_OPC_FAST_REWIND:
	case BT_MCS_OPC_FAST_FORWARD:
	case BT_MCS_OPC_STOP:
	case BT_MCS_OPC_MOVE_RELATIVE:
	case BT_MCS_OPC_PREV_SEGMENT:
	case BT_MCS_OPC_NEXT_SEGMENT:
	case BT_MCS_OPC_FIRST_SEGMENT:
	case BT_MCS_OPC_LAST_SEGMENT:
	case BT_MCS_OPC_GOTO_SEGMENT:
		ntf->result_code = BT_MCS_OPC_NTF_PLAYER_INACTIVE;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_TRACK:
		if (do_prev_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For previous track, the position is reset to 0 */
			/* even if we stay at the same track (goto start of */
			/* track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		/* TODO:
		 * The case where the next track has been set explicitly breaks somewhat
		 * with the "next" order hardcoded into the group and track structure
		 */
		if (pl.next_track_set) {
			BT_DBG("Next track set");
			if (do_next_track_next_track_set(&pl)) {
				do_group_change_notifications(&pl);
			}
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else if (do_next_track_normal_order(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (command->use_param) {
			if (do_goto_track(&pl, command->param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				media_proxy_pl_track_position_cb(pl.track_pos);
			}
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			media_proxy_pl_media_state_cb(pl.state);
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&pl, command->param);
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			media_proxy_pl_media_state_cb(pl.state);
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	default:
		BT_DBG("Invalid command: %d", command->opcode);
		ntf->result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		media_proxy_pl_command_cb(ntf);
		break;
	}
}

void playing_state_command_handler(const struct mpl_cmd *command,
				   struct mpl_cmd_ntf *ntf)
{
	BT_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MPL)) {
		if (command->use_param) {
			BT_DBG("Command parameter: %d", command->param);
		}
	}
	switch (command->opcode) {
	case BT_MCS_OPC_PLAY:
		/* Continue playing - i.e. do nothing */
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PAUSE:
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_REWIND:
		/* We're in playing state, seeking speed must have been zero */
		pl.seeking_speed_factor = -MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_FORWARD:
		/* We're in playing state, seeking speed must have been zero */
		pl.seeking_speed_factor = MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_STOP:
		pl.track_pos = 0;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_MOVE_RELATIVE:
		if (command->use_param) {
			/* Keep within track - i.e. in the range 0 - duration */
			if (command->param >
			    pl.group->track->duration - pl.track_pos) {
				pl.track_pos = pl.group->track->duration;
			} else if (command->param < -pl.track_pos) {
				pl.track_pos = 0;
			} else {
				pl.track_pos += command->param;
			}
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_track_position_cb(pl.track_pos);
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_SEGMENT:
		/* Switch to previous segment if we are less than <margin> */
		/* into the segment, otherwise go to start of segment */
		if (pl.track_pos - PREV_MARGIN <
		    pl.group->track->segment->pos) {
			do_prev_segment(&pl);
		}
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_SEGMENT:
		do_next_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_SEGMENT:
		do_first_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_SEGMENT:
		do_last_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_SEGMENT:
		if (command->use_param) {
			if (command->param != 0) {
				do_goto_segment(&pl, command->param);
				pl.track_pos = pl.group->track->segment->pos;
				media_proxy_pl_track_position_cb(pl.track_pos);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shall stay the same, and the */
			/* track position shall not change. */
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_TRACK:
		if (do_prev_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For previous track, the position is reset to 0 */
			/* even if we stay at the same track (goto start of */
			/* track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		if (pl.next_track_set) {
			BT_DBG("Next track set");
			if (do_next_track_next_track_set(&pl)) {
				do_group_change_notifications(&pl);
			}
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else if (do_next_track_normal_order(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (command->use_param) {
			if (do_goto_track(&pl, command->param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				media_proxy_pl_track_position_cb(pl.track_pos);
			}
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&pl, command->param);
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	default:
		BT_DBG("Invalid command: %d", command->opcode);
		ntf->result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		media_proxy_pl_command_cb(ntf);
		break;
	}
}

void paused_state_command_handler(const struct mpl_cmd *command,
				  struct mpl_cmd_ntf *ntf)
{
	BT_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MPL)) {
		if (command->use_param) {
			BT_DBG("Command parameter: %d", command->param);
		}
	}
	switch (command->opcode) {
	case BT_MCS_OPC_PLAY:
		pl.state = BT_MCS_MEDIA_STATE_PLAYING;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PAUSE:
		/* No change */
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_REWIND:
		/* We're in paused state, seeking speed must have been zero */
		pl.seeking_speed_factor = -MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_FORWARD:
		/* We're in paused state, seeking speed must have been zero */
		pl.seeking_speed_factor = MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_STOP:
		pl.track_pos = 0;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_MOVE_RELATIVE:
		if (command->use_param) {
			/* Keep within track - i.e. in the range 0 - duration */
			if (command->param >
			    pl.group->track->duration - pl.track_pos) {
				pl.track_pos = pl.group->track->duration;
			} else if (command->param < -pl.track_pos) {
				pl.track_pos = 0;
			} else {
				pl.track_pos += command->param;
			}
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_track_position_cb(pl.track_pos);
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_SEGMENT:
		/* Switch to previous segment if we are less than 5 seconds */
		/* into the segment, otherwise go to start of segment */
		if (pl.track_pos - PREV_MARGIN <
		    pl.group->track->segment->pos) {
			do_prev_segment(&pl);
		}
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_SEGMENT:
		do_next_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_SEGMENT:
		do_first_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_SEGMENT:
		do_last_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_SEGMENT:
		if (command->use_param) {
			if (command->param != 0) {
				do_goto_segment(&pl, command->param);
				pl.track_pos = pl.group->track->segment->pos;
				media_proxy_pl_track_position_cb(pl.track_pos);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shall stay the same, and the */
			/* track position shall not change. */
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_TRACK:
		if (do_prev_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For previous track, the position is reset to 0 */
			/* even if we stay at the same track (goto start of */
			/* track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		if (pl.next_track_set) {
			BT_DBG("Next track set");
			if (do_next_track_next_track_set(&pl)) {
				do_group_change_notifications(&pl);
			}
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else if (do_next_track_normal_order(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (command->use_param) {
			if (do_goto_track(&pl, command->param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				media_proxy_pl_track_position_cb(pl.track_pos);
			}
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&pl, command->param);
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	default:
		BT_DBG("Invalid command: %d", command->opcode);
		ntf->result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		media_proxy_pl_command_cb(ntf);
		break;
	}
}

void seeking_state_command_handler(const struct mpl_cmd *command,
				   struct mpl_cmd_ntf *ntf)
{
	BT_DBG("Command opcode: %d", command->opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MPL)) {
		if (command->use_param) {
			BT_DBG("Command parameter: %d", command->param);
		}
	}
	switch (command->opcode) {
	case BT_MCS_OPC_PLAY:
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PLAYING;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PAUSE:
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		/* TODO: Set track and track position */
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_REWIND:
		/* TODO: Here, and for FAST_FORWARD */
		/* Decide on algorithm for multiple presses - add step (as */
		/* now) or double/half? */
		/* What about FR followed by FF? */
		/* Currently, the seeking speed may also become	 zero */
		/* Lowest value allowed by spec is -64, notify on change only */
		if (pl.seeking_speed_factor >= -(BT_MCS_SEEKING_SPEED_FACTOR_MAX
						 - MPL_SEEKING_SPEED_FACTOR_STEP)) {
			pl.seeking_speed_factor -= MPL_SEEKING_SPEED_FACTOR_STEP;
			media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_FORWARD:
		/* Highest value allowed by spec is 64, notify on change only */
		if (pl.seeking_speed_factor <= (BT_MCS_SEEKING_SPEED_FACTOR_MAX
						- MPL_SEEKING_SPEED_FACTOR_STEP)) {
			pl.seeking_speed_factor += MPL_SEEKING_SPEED_FACTOR_STEP;
			media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		}
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_STOP:
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.track_pos = 0;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_MOVE_RELATIVE:
		if (command->use_param) {
			/* Keep within track - i.e. in the range 0 - duration */
			if (command->param >
			    pl.group->track->duration - pl.track_pos) {
				pl.track_pos = pl.group->track->duration;
			} else if (command->param < -pl.track_pos) {
				pl.track_pos = 0;
			} else {
				pl.track_pos += command->param;
			}
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_track_position_cb(pl.track_pos);
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_SEGMENT:
		/* Switch to previous segment if we are less than 5 seconds */
		/* into the segment, otherwise go to start of segment */
		if (pl.track_pos - PREV_MARGIN <
		    pl.group->track->segment->pos) {
			do_prev_segment(&pl);
		}
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_SEGMENT:
		do_next_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_SEGMENT:
		do_first_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_SEGMENT:
		do_last_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		media_proxy_pl_track_position_cb(pl.track_pos);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_SEGMENT:
		if (command->use_param) {
			if (command->param != 0) {
				do_goto_segment(&pl, command->param);
				pl.track_pos = pl.group->track->segment->pos;
				media_proxy_pl_track_position_cb(pl.track_pos);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shall stay the same, and the */
			/* track position shall not change. */
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_TRACK:
		if (do_prev_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For previous track, the position is reset to 0 */
			/* even if we stay at the same track (goto start of */
			/* track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		if (pl.next_track_set) {
			BT_DBG("Next track set");
			if (do_next_track_next_track_set(&pl)) {
				do_group_change_notifications(&pl);
			}
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else if (do_next_track_normal_order(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			media_proxy_pl_track_position_cb(pl.track_pos);
		}
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (command->use_param) {
			if (do_goto_track(&pl, command->param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				media_proxy_pl_track_position_cb(pl.track_pos);
			}
			pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			media_proxy_pl_media_state_cb(pl.state);
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		media_proxy_pl_media_state_cb(pl.state);
		ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		media_proxy_pl_command_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (command->use_param) {
			do_full_goto_group(&pl, command->param);
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			media_proxy_pl_media_state_cb(pl.state);
			ntf->result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf->result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		media_proxy_pl_command_cb(ntf);
		break;
	default:
		BT_DBG("Invalid command: %d", command->opcode);
		ntf->result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		media_proxy_pl_command_cb(ntf);
		break;
	}
}

void (*command_handlers[BT_MCS_MEDIA_STATE_LAST])(const struct mpl_cmd *command,
						  struct mpl_cmd_ntf *ntf) = {
	inactive_state_command_handler,
	playing_state_command_handler,
	paused_state_command_handler,
	seeking_state_command_handler
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

const char *get_player_name(void)
{
	return pl.name;
}

#ifdef CONFIG_BT_MPL_OBJECTS
uint64_t get_icon_id(void)
{
	return pl.icon_id;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

const char *get_icon_url(void)
{
	return pl.icon_url;
}

const char *get_track_title(void)
{
	return pl.group->track->title;
}

int32_t get_track_duration(void)
{
	return pl.group->track->duration;
}

int32_t get_track_position(void)
{
	return pl.track_pos;
}

void set_track_position(int32_t position)
{
	int32_t old_pos = pl.track_pos;
	int32_t new_pos;

	if (position >= 0) {
		if (position > pl.group->track->duration) {
			/* Do not go beyond end of track */
			new_pos = pl.group->track->duration;
		} else {
			new_pos = position;
		}
	} else {
		/* Negative position, handle as offset from _end_ of track */
		/* (Note minus sign below) */
		if (position < -pl.group->track->duration) {
			new_pos = 0;
		} else {
			/* (Remember position is negative) */
			new_pos = pl.group->track->duration + position;
		}
	}

	BT_DBG("Pos. given: %d, resulting pos.: %d (duration is %d)",
	       position, new_pos, pl.group->track->duration);

	if (new_pos != old_pos) {
		/* Set new position and notify it */
		pl.track_pos = new_pos;
		media_proxy_pl_track_position_cb(new_pos);
	}
}

int8_t get_playback_speed(void)
{
	return pl.playback_speed_param;
}

void set_playback_speed(int8_t speed)
{
	/* Set new speed parameter and notify, if different from current */
	if (speed != pl.playback_speed_param) {
		pl.playback_speed_param = speed;
		media_proxy_pl_playback_speed_cb(pl.playback_speed_param);
	}
}

int8_t get_seeking_speed(void)
{
	return pl.seeking_speed_factor;
}

#ifdef CONFIG_BT_MPL_OBJECTS
uint64_t get_track_segments_id(void)
{
	return pl.group->track->segments_id;
}

uint64_t get_current_track_id(void)
{
	return pl.group->track->id;
}

void set_current_track_id(uint64_t id)
{
	struct mpl_group *group;
	struct mpl_track *track;

	BT_DBG_OBJ_ID("Track ID to set: ", id);

	if (find_track_by_id(&pl, id, &group, &track)) {

		if (pl.group != group) {
			pl.group = group;
			do_group_change_notifications(&pl);

			/* Group change implies track change (even if same track in other group) */
			pl.group->track = track;
			do_track_change_notifications(&pl);

		} else if (pl.group->track != track) {
			pl.group->track = track;
			do_track_change_notifications(&pl);
		}
		return;
	}

	BT_DBG("Track not found");

	/* TODO: Should an error be returned here?
	 * That would require a rewrite of the MPL api to add return values to the functions.
	 */
}

uint64_t get_next_track_id(void)
{
	/* If the next track has been set explicitly */
	if (pl.next_track_set) {
		return pl.next.track->id;
	}

	/* Normal playing order */
	if (pl.group->track->next) {
		return pl.group->track->next->id;
	}

	/* Return zero value to indicate that there is no next track */
	return MPL_NO_TRACK_ID;
}

void set_next_track_id(uint64_t id)
{
	struct mpl_group *group;
	struct mpl_track *track;

	BT_DBG_OBJ_ID("Next Track ID to set: ", id);

	if (find_track_by_id(&pl, id, &group, &track)) {

		pl.next_track_set = true;
		pl.next.group = group;
		pl.next.track = track;
		media_proxy_pl_next_track_id_cb(id);
		return;
	}

	BT_DBG("Track not found");
}

uint64_t get_parent_group_id(void)
{
	return pl.group->parent->id;
}

uint64_t get_current_group_id(void)
{
	return pl.group->id;
}

void set_current_group_id(uint64_t id)
{
	struct mpl_group *group;
	bool track_change;

	BT_DBG_OBJ_ID("Group ID to set: ", id);

	if (find_group_by_id(&pl, id, &group)) {

		if (pl.group != group) {
			/* Change to found group */
			pl.group = group;
			do_group_change_notifications(&pl);

			/* And change to first track in group */
			track_change = do_first_track(&pl);
			if (track_change) {
				do_track_change_notifications(&pl);
			}
		}
		return;
	}

	BT_DBG("Group not found");
}
#endif /* CONFIG_BT_MPL_OBJECTS */

uint8_t get_playing_order(void)
{
	return pl.playing_order;
}

void set_playing_order(uint8_t order)
{
	if (order != pl.playing_order) {
		if (BIT(order - 1) & pl.playing_orders_supported) {
			pl.playing_order = order;
			media_proxy_pl_playing_order_cb(pl.playing_order);
		}
	}
}

uint16_t get_playing_orders_supported(void)
{
	return pl.playing_orders_supported;
}

uint8_t get_media_state(void)
{
	return pl.state;
}

void send_command(const struct mpl_cmd *command)
{
	struct mpl_cmd_ntf ntf;

	if (command->use_param) {
		BT_DBG("opcode: %d, param: %d", command->opcode, command->param);
	} else {
		BT_DBG("opcode: %d", command->opcode);
	}

	if (pl.state < BT_MCS_MEDIA_STATE_LAST) {
		ntf.requested_opcode = command->opcode;
		command_handlers[pl.state](command, &ntf);
	} else {
		BT_DBG("INVALID STATE");
	}
}

uint32_t get_commands_supported(void)
{
	return pl.opcodes_supported;
}

#ifdef CONFIG_BT_MPL_OBJECTS
static void parse_search(const struct mpl_search *search)
{
	uint8_t index = 0;
	struct mpl_sci sci;
	uint8_t sci_num = 0;
	bool search_failed = false;

	if (search->len > SEARCH_LEN_MAX) {
		BT_WARN("Search too long (%d) - aborting", search->len);
		search_failed = true;
	} else {
		BT_DBG("Parsing %d octets search", search->len);

		while (search->len - index > 0) {
			sci.len = (uint8_t)search->search[index++];
			if (sci.len < SEARCH_SCI_LEN_MIN) {
				BT_WARN("Invalid length field - too small");
				search_failed = true;
				break;
			}
			if (sci.len > (search->len - index)) {
				BT_WARN("Incomplete search control item");
				search_failed = true;
				break;
			}
			sci.type = (uint8_t)search->search[index++];
			if (sci.type <  BT_MCS_SEARCH_TYPE_TRACK_NAME ||
			    sci.type > BT_MCS_SEARCH_TYPE_ONLY_GROUPS) {
				search_failed = true;
				break;
			}
			(void)memcpy(&sci.param, &search->search[index], sci.len - 1);
			index += sci.len - 1;

			BT_DBG("SCI # %d: type: %d", sci_num, sci.type);
			BT_HEXDUMP_DBG(sci.param, sci.len-1, "param:");
			sci_num++;
		}
	}

	/* TODO: Add real search functionality. */
	/* For now, just fake it. */

	if (search_failed) {
		pl.search_results_id = 0;
		media_proxy_pl_search_cb(BT_MCS_SCP_NTF_FAILURE);
	} else {
		/* Use current group as search result for now */
		pl.search_results_id = pl.group->id;
		media_proxy_pl_search_cb(BT_MCS_SCP_NTF_SUCCESS);
	}

	media_proxy_pl_search_results_id_cb(pl.search_results_id);
}

void send_search(const struct mpl_search *search)
{
	if (search->len > SEARCH_LEN_MAX) {
		BT_WARN("Search too long: %d", search->len);
	}

	BT_HEXDUMP_DBG(search->search, search->len, "Search");

	parse_search(search);
}

uint64_t get_search_results_id(void)
{
	return pl.search_results_id;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

uint8_t get_content_ctrl_id(void)
{
	return pl.content_ctrl_id;
}

int media_proxy_pl_init(void)
{
	static bool initialized;
	int ret;

	if (initialized) {
		BT_DBG("Already initialized");
		return -EALREADY;
	}

	/* Set up the media control service */
	/* TODO: Fix initialization - who initializes what
	 * https://github.com/zephyrproject-rtos/zephyr/issues/42965
	 * Temporarily only initializing if service is present
	 */
#ifdef CONFIG_BT_MCS
#ifdef CONFIG_BT_MPL_OBJECTS
	ret = bt_mcs_init(&ots_cbs);
#else
	ret = bt_mcs_init(NULL);
#endif /* CONFIG_BT_MPL_OBJECTS */
	if (ret < 0) {
		BT_ERR("Could not init MCS: %d", ret);
		return ret;
	}
#else
	BT_WARN("MCS not configured");
#endif /* CONFIG_BT_MCS */

	/* Get a Content Control ID */
	pl.content_ctrl_id = bt_ccid_get_value();

#ifdef CONFIG_BT_MPL_OBJECTS
	/* Initialize the object content buffer */
	net_buf_simple_init(obj.content, 0);

	/* Icon Object */
	ret = add_icon_object(&pl);
	if (ret < 0) {
		BT_ERR("Unable to add icon object, error %d", ret);
		return ret;
	}

	/* Add all tracks and groups to OTS */
	ret = add_group_and_track_objects(&pl);
	if (ret < 0) {
		BT_ERR("Error adding tracks and groups to OTS, error %d", ret);
		return ret;
	}

	/* Initial setup of Track Segments Object */
	/* TODO: Later, this should be done when the tracks are added */
	/* but for no only one of the tracks has segments .*/
	ret = add_current_track_segments_object(&pl);
	if (ret < 0) {
		BT_ERR("Error adding Track Segments Object to OTS, error %d", ret);
		return ret;
	}
#endif /* CONFIG_BT_MPL_OBJECTS */

	/* Set up the calls structure */
	pl.calls.get_player_name              = get_player_name;
#ifdef CONFIG_BT_MPL_OBJECTS
	pl.calls.get_icon_id                  = get_icon_id;
#endif /* CONFIG_BT_MPL_OBJECTS */
	pl.calls.get_icon_url                 = get_icon_url;
	pl.calls.get_track_title              = get_track_title;
	pl.calls.get_track_duration           = get_track_duration;
	pl.calls.get_track_position           = get_track_position;
	pl.calls.set_track_position           = set_track_position;
	pl.calls.get_playback_speed           = get_playback_speed;
	pl.calls.set_playback_speed           = set_playback_speed;
	pl.calls.get_seeking_speed            = get_seeking_speed;
#ifdef CONFIG_BT_MPL_OBJECTS
	pl.calls.get_track_segments_id        = get_track_segments_id;
	pl.calls.get_current_track_id         = get_current_track_id;
	pl.calls.set_current_track_id         = set_current_track_id;
	pl.calls.get_next_track_id            = get_next_track_id;
	pl.calls.set_next_track_id            = set_next_track_id;
	pl.calls.get_parent_group_id          = get_parent_group_id;
	pl.calls.get_current_group_id         = get_current_group_id;
	pl.calls.set_current_group_id         = set_current_group_id;
#endif /* CONFIG_BT_MPL_OBJECTS */
	pl.calls.get_playing_order            = get_playing_order;
	pl.calls.set_playing_order            = set_playing_order;
	pl.calls.get_playing_orders_supported = get_playing_orders_supported;
	pl.calls.get_media_state              = get_media_state;
	pl.calls.send_command                 = send_command;
#ifdef CONFIG_BT_MPL_OBJECTS
	pl.calls.send_search                  = send_search;
	pl.calls.get_search_results_id        = get_search_results_id;
#endif /* CONFIG_BT_MPL_OBJECTS */
	pl.calls.get_content_ctrl_id          = get_content_ctrl_id;

	ret = media_proxy_pl_register(&pl.calls);
	if (ret < 0) {
		BT_ERR("Unable to register player");
		return ret;
	}

	initialized = true;
	return 0;
}

#if CONFIG_BT_DEBUG_MPL /* Special commands for debugging */

void mpl_debug_dump_state(void)
{
#if CONFIG_BT_MPL_OBJECTS
	char t[BT_OTS_OBJ_ID_STR_LEN];
	struct mpl_group *group;
	struct mpl_track *track;
#endif /* CONFIG_BT_MPL_OBJECTS */

	BT_DBG("Mediaplayer name: %s", log_strdup(pl.name));

#if CONFIG_BT_MPL_OBJECTS
	(void)bt_ots_obj_id_to_str(pl.icon_id, t, sizeof(t));
	BT_DBG("Icon ID: %s", log_strdup(t));
#endif /* CONFIG_BT_MPL_OBJECTS */

	BT_DBG("Icon URL: %s", log_strdup(pl.icon_url));
	BT_DBG("Track position: %d", pl.track_pos);
	BT_DBG("Media state: %d", pl.state);
	BT_DBG("Playback speed parameter: %d", pl.playback_speed_param);
	BT_DBG("Seeking speed factor: %d", pl.seeking_speed_factor);
	BT_DBG("Playing order: %d", pl.playing_order);
	BT_DBG("Playing orders supported: 0x%x", pl.playing_orders_supported);
	BT_DBG("Opcodes supported: %d", pl.opcodes_supported);
	BT_DBG("Content control ID: %d", pl.content_ctrl_id);

#if CONFIG_BT_MPL_OBJECTS
	(void)bt_ots_obj_id_to_str(pl.group->parent->id, t, sizeof(t));
	BT_DBG("Current group's parent: %s", log_strdup(t));

	(void)bt_ots_obj_id_to_str(pl.group->id, t, sizeof(t));
	BT_DBG("Current group: %s", log_strdup(t));

	(void)bt_ots_obj_id_to_str(pl.group->track->id, t, sizeof(t));
	BT_DBG("Current track: %s", log_strdup(t));

	if (pl.next_track_set) {
		(void)bt_ots_obj_id_to_str(pl.next.track->id, t, sizeof(t));
		BT_DBG("Next track: %s", log_strdup(t));
	} else if (pl.group->track->next) {
		(void)bt_ots_obj_id_to_str(pl.group->track->next->id, t,
					   sizeof(t));
		BT_DBG("Next track: %s", log_strdup(t));
	} else {
		BT_DBG("No next track");
	}

	if (pl.search_results_id) {
		(void)bt_ots_obj_id_to_str(pl.search_results_id, t, sizeof(t));
		BT_DBG("Search results: %s", log_strdup(t));
	} else {
		BT_DBG("No search results");
	}

	BT_DBG("Groups and tracks:");
	group = pl.group;

	while (group->prev != NULL) {
		group = group->prev;
	}

	while (group) {
		(void)bt_ots_obj_id_to_str(group->id, t, sizeof(t));
		BT_DBG("Group: %s, %s", log_strdup(t),
		       log_strdup(group->title));

		(void)bt_ots_obj_id_to_str(group->parent->id, t, sizeof(t));
		BT_DBG("\tParent: %s, %s", log_strdup(t),
		       log_strdup(group->parent->title));

		track = group->track;
		while (track->prev != NULL) {
			track = track->prev;
		}

		while (track) {
			(void)bt_ots_obj_id_to_str(track->id, t, sizeof(t));
			BT_DBG("\tTrack: %s, %s, duration: %d", log_strdup(t),
			       log_strdup(track->title), track->duration);
			track = track->next;
		}

		group = group->next;
	}
#endif /* CONFIG_BT_MPL_OBJECTS */
}
#endif /* CONFIG_BT_DEBUG_MPL */


#if defined(CONFIG_BT_DEBUG_MPL) && defined(CONFIG_BT_TESTING) /* Special commands for testing */

#if CONFIG_BT_MPL_OBJECTS
void mpl_test_unset_parent_group(void)
{
	BT_DBG("Setting current group to be it's own parent");
	pl.group->parent = pl.group;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

void mpl_test_media_state_set(uint8_t state)
{
	pl.state = state;
	media_proxy_pl_media_state_cb(pl.state);
}

void mpl_test_track_changed_cb(void)
{
	media_proxy_pl_track_changed_cb();
}

void mpl_test_title_changed_cb(void)
{
	media_proxy_pl_track_title_cb(pl.group->track->title);
}

void mpl_test_duration_changed_cb(void)
{
	media_proxy_pl_track_duration_cb(pl.group->track->duration);
}

void mpl_test_position_changed_cb(void)
{
	media_proxy_pl_track_position_cb(pl.track_pos);
}

void mpl_test_playback_speed_changed_cb(void)
{
	media_proxy_pl_playback_speed_cb(pl.playback_speed_param);
}

void mpl_test_seeking_speed_changed_cb(void)
{
	media_proxy_pl_seeking_speed_cb(pl.seeking_speed_factor);
}

#ifdef CONFIG_BT_MPL_OBJECTS
void mpl_test_current_track_id_changed_cb(void)
{
	media_proxy_pl_current_track_id_cb(pl.group->track->id);
}

void mpl_test_next_track_id_changed_cb(void)
{
	media_proxy_pl_next_track_id_cb(pl.group->track->next->id);
}

void mpl_test_parent_group_id_changed_cb(void)
{
	media_proxy_pl_parent_group_id_cb(pl.group->id);
}

void mpl_test_current_group_id_changed_cb(void)
{
	media_proxy_pl_current_group_id_cb(pl.group->id);
}
#endif /* CONFIG_BT_MPL_OBJECTS */

void mpl_test_playing_order_changed_cb(void)
{
	media_proxy_pl_playing_order_cb(pl.playing_order);
}

void mpl_test_media_state_changed_cb(void)
{
	media_proxy_pl_media_state_cb(pl.playing_order);
}

void mpl_test_opcodes_supported_changed_cb(void)
{
	media_proxy_pl_commands_supported_cb(pl.opcodes_supported);
}

#ifdef CONFIG_BT_MPL_OBJECTS
void mpl_test_search_results_changed_cb(void)
{
	media_proxy_pl_search_cb(pl.search_results_id);
}
#endif /* CONFIG_BT_MPL_OBJECTS */

#endif /* CONFIG_BT_DEBUG_MPL && CONFIG_BT_TESTING */
