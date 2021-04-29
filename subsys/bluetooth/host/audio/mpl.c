/*  Media player skeleton implementation */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>

#include <bluetooth/services/ots.h>

#include "media_proxy.h"
#include "media_proxy_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCS)
#define LOG_MODULE_NAME bt_mpl
#include "common/log.h"
#include "ccid_internal.h"
#include "mcs_internal.h"

#define TRACK_STATUS_INVALID 0x00
#define TRACK_STATUS_VALID 0x01

#define PLAYBACK_SPEED_PARAM_DEFAULT BT_MCS_PLAYBACK_SPEED_UNITY

/* Temporary hardcoded setup for groups, tracks and segements */
/* There is one parent group, which is the parent of a number of groups. */
/* The groups have a number of tracks.  */
/* (There is only one level of groups, there are no groups of groups.) */
/* The first track of the first group has track segments, other tracks not. */

/* Track segments */
static struct mpl_tseg_t seg_2;
static struct mpl_tseg_t seg_3;

static struct mpl_tseg_t seg_1 = {
	.name_len = 5,
	.name	  = "Start",
	.pos	  = 0,
	.prev	  = NULL,
	.next	  = &seg_2,
};

static struct mpl_tseg_t seg_2 = {
	.name_len = 6,
	.name	  = "Middle",
	.pos	  = 2000,
	.prev	  = &seg_1,
	.next	  = &seg_3,
};

static struct mpl_tseg_t seg_3 = {
	.name_len = 3,
	.name	  = "End",
	.pos	  = 5000,
	.prev	  = &seg_2,
	.next	  = NULL,
};

static struct mpl_track_t track_1_2;
static struct mpl_track_t track_1_3;
static struct mpl_track_t track_1_4;
static struct mpl_track_t track_1_5;

/* Tracks */
static struct mpl_track_t track_1_1 = {
	.title	     = "Interlude #1 (Song for Alison)",
	.duration    = 6300,
	.segment     = &seg_1,
	.prev	     = NULL,
	.next	     = &track_1_2,
};


static struct mpl_track_t track_1_2 = {
	.title	     = "Interlude #2 (For Bobbye)",
	.duration    = 7500,
	.segment     = NULL,
	.prev	     = &track_1_1,
	.next	     = &track_1_3,
};

static struct mpl_track_t track_1_3 = {
	.title	     = "Interlude #3 (Levanto Seventy)",
	.duration    = 7800,
	.segment     = NULL,
	.prev	     = &track_1_2,
	.next	     = &track_1_4,
};

static struct mpl_track_t track_1_4 = {
	.title	     = "Interlude #4 (Vesper Dreams)",
	.duration    = 13500,
	.segment     = NULL,
	.prev	     = &track_1_3,
	.next	     = &track_1_5,
};

static struct mpl_track_t track_1_5 = {
	.title	     = "Interlude #5 (Shasti)",
	.duration    = 7500,
	.segment     = NULL,
	.prev	     = &track_1_4,
	.next	     = NULL,
};

/* Temporary comment away when no OTS */
/* The track_dummy and track_timmy will be removed */
#ifdef CONFIG_BT_OTS
static struct mpl_track_t track_dummy = {
	.title	     = "My dummy track",
	.duration    = 18700,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = NULL,
};

static struct mpl_track_t track_timmy = {
	.title	     = "My Timmy track",
	.duration    = 18700,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = NULL,
};
#endif /* CONFIG_BT_OTS */

static struct mpl_track_t track_2_2;
static struct mpl_track_t track_2_3;

static struct mpl_track_t track_2_1 = {
	.title	     = "Track 2.1",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = &track_2_2,
};

static struct mpl_track_t track_2_2 = {
	.title	     = "Track 2.2",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_2_1,
	.next	     = &track_2_3,
};

static struct mpl_track_t track_2_3 = {
	.title	     = "Track 2.3",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_2_2,
	.next	     = NULL,
};

static struct mpl_track_t track_3_2;
static struct mpl_track_t track_3_3;

static struct mpl_track_t track_3_1 = {
	.title	     = "Track 3.1",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = &track_3_2,
};

static struct mpl_track_t track_3_2 = {
	.title	     = "Track 3.2",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_3_1,
	.next	     = &track_3_3,
};

static struct mpl_track_t track_3_3 = {
	.title	     = "Track 3.3",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_3_2,
	.next	     = NULL,
};

static struct mpl_track_t track_4_2;

static struct mpl_track_t track_4_1 = {
	.title	     = "Track 4.1",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = NULL,
	.next	     = &track_4_2,
};

static struct mpl_track_t track_4_2 = {
	.title	     = "Track 4.2",
	.duration    = 30000,
	.segment     = NULL,
	.prev	     = &track_4_1,
	.next	     = NULL,
};

/* Groups */
static struct mpl_group_t group_2;
static struct mpl_group_t group_3;
static struct mpl_group_t group_4;
static struct mpl_group_t group_p;

static struct mpl_group_t group_1 = {
	.title  = "Joe Pass - Guitar Interludes",
	.track	= &track_1_1,
	.parent = &group_p,
	.prev	= NULL,
	.next	= &group_2,
};

static struct mpl_group_t group_2 = {
	.title  = "Group 2",
	.track	= &track_2_2,
	.parent = &group_p,
	.prev	= &group_1,
	.next	= &group_3,
};

static struct mpl_group_t group_3 = {
	.title  = "Group 3",
	.track	= &track_3_3,
	.parent = &group_p,
	.prev	= &group_2,
	.next	= &group_4,
};

static struct mpl_group_t group_4 = {
	.title  = "Group 4",
	.track	= &track_4_2,
	.parent = &group_p,
	.prev	= &group_3,
	.next	= NULL,
};

static struct mpl_group_t group_p = {
	.title  = "Parent group",
	.track	= &track_4_1,
	.parent = &group_p,
	.prev	= NULL,
	.next	= NULL,
};

static struct mpl_mediaplayer_t pl = {
	.name			  = CONFIG_BT_MCS_MEDIA_PLAYER_NAME,
	.icon_uri		  = CONFIG_BT_MCS_ICON_URI,
	.group			  = &group_1,
	.track_pos		  = 0,
	.state			  = BT_MCS_MEDIA_STATE_PAUSED,
	.playback_speed_param	  = PLAYBACK_SPEED_PARAM_DEFAULT,
	.seeking_speed_factor	  = BT_MCS_SEEKING_SPEED_FACTOR_ZERO,
	.playing_order		  = BT_MCS_PLAYING_ORDER_INORDER_REPEAT,
	.playing_orders_supported = BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_ONCE |
				    BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_REPEAT,
	.operations_supported	  = 0x001fffff, /* All opcodes */
#ifdef CONFIG_BT_OTS
	.search_results_id	  = 0,
#endif /* CONFIG_BT_OTS */
};

#ifdef CONFIG_BT_OTS

/* The types of objects we keep in the Object Transfer Service */
enum mpl_objects {
	MPL_OBJ_NONE = 0,
	MPL_OBJ_ICON,
	MPL_OBJ_TRACK_SEGMENTS,
	MPL_OBJ_TRACK,
	MPL_OBJ_GROUP,
	MPL_OBJ_PARENT_GROUP,
	MPL_OBJ_SEARCH_RESULTS,
};

/* The active object */
/* Only a single object is selected or being added (active) at a time. */
/* And, except for the icon object, all objects can be created dynamically. */
/* So a single buffer to hold object content is sufficient.  */
struct obj_t {
	uint64_t selected_id;  /* ID of the currently selected object*/
	bool     busy;
	uint8_t  add_type;     /* Type of object being added, e.g. MPL_OBJ_ICON */
	union {
		struct mpl_track_t *add_track; /* Pointer to track being added */
		struct mpl_group_t *add_group; /* Pointer to group being added */
	};
	struct   net_buf_simple *content;
};

static struct obj_t obj = {
	.selected_id = 0,
	.add_type = MPL_OBJ_NONE,
	.busy = false,
	.add_track = NULL,
	.add_group = NULL,
	.content = NET_BUF_SIMPLE(CONFIG_BT_MCS_MAX_OBJ_SIZE)
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
	     index < MIN(CONFIG_BT_MCS_MAX_OBJ_SIZE,
			 CONFIG_BT_MCS_ICON_BITMAP_SIZE);
	     index++, k++) {
		net_buf_simple_add_u8(obj.content, k);
	}

	return obj.content->len;
}

/* Set up content buffer for a track segments object */
static uint32_t setup_segments_object(struct mpl_track_t *track)
{
	struct mpl_tseg_t *seg = track->segment;

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
static uint32_t setup_track_object(struct mpl_track_t *track)
{
	uint16_t index;
	uint8_t k;

	/* The track object is supposed to be in Id3v2 format */
	/* For now, fill it with dummy data */

	net_buf_simple_reset(obj.content);

	/* Size may be larger than what fits in 8 bits, use 16-bit for index */
	for (index = 0, k = 0;
	     index < MIN(CONFIG_BT_MCS_MAX_OBJ_SIZE,
			 CONFIG_BT_MCS_TRACK_MAX_SIZE);
	     index++, k++) {
		net_buf_simple_add_u8(obj.content, k);
	}

	return obj.content->len;
}

/* Set up contents for a group object */
/* The group object contains a concatenated list of records, where each */
/* record consists of a type byte and a UUID */
static uint32_t setup_group_object(struct mpl_group_t *group)
{
	struct mpl_track_t *track = group->track;
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

/* Set up content buffer for the parent group object */
static uint32_t setup_parent_group_object(struct mpl_group_t *group)
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

/* Add the icon object to the OTS */
static int add_icon_object(struct mpl_mediaplayer_t *pl)
{
	int ret;
	struct bt_ots_obj_metadata icon = {0};
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

	icon.size.alloc = icon.size.cur = setup_icon_object();
	icon.name = icon_name;
	icon.type.uuid.type = BT_UUID_TYPE_16;
	icon.type.uuid_16.val = BT_UUID_16(icon_type)->val;
	BT_OTS_OBJ_SET_PROP_READ(icon.props);

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &icon);

	if (ret) {
		BT_WARN("Unable to add icon object, error %d", ret);
		obj.busy = false;
	}
	return ret;
}

/* Add a track segments object to the OTS */
static int add_current_track_segments_object(struct mpl_mediaplayer_t *pl)
{
	int ret;
	struct bt_ots_obj_metadata segs = {0};
	struct bt_uuid *segs_type = BT_UUID_OTS_TYPE_TRACK_SEGMENT;

	if (obj.busy) {
		BT_ERR("Object busy");
		return 0;
	}
	obj.busy = true;
	obj.add_type = MPL_OBJ_TRACK_SEGMENTS;

	segs.size.alloc = segs.size.cur = setup_segments_object(pl->group->track);
	segs.name = pl->group->track->title;
	segs.type.uuid.type = BT_UUID_TYPE_16;
	segs.type.uuid_16.val = BT_UUID_16(segs_type)->val;
	BT_OTS_OBJ_SET_PROP_READ(segs.props);

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &segs);
	if (ret) {
		BT_WARN("Unable to add track segments object: %d", ret);
		obj.busy = false;
	}
	return ret;
}

/* Add a single track to the OTS */
static int add_track_object(struct mpl_track_t *track)
{
	struct bt_ots_obj_metadata track_meta = {0};
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

	track_meta.size.alloc = track_meta.size.cur = setup_track_object(track);
	track_meta.name = track->title;
	track_meta.type.uuid.type = BT_UUID_TYPE_16;
	track_meta.type.uuid_16.val = BT_UUID_16(track_type)->val;
	BT_OTS_OBJ_SET_PROP_READ(track_meta.props);
	ret = bt_ots_obj_add(bt_mcs_get_ots(), &track_meta);

	if (ret) {
		BT_WARN("Unable to add track object: %d", ret);
		obj.busy = false;
	}

	return ret;
}

/* Add a single group to the OTS */
static int add_group_object(struct mpl_group_t *group)
{
	struct bt_ots_obj_metadata group_meta = {0};
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

	group_meta.size.alloc = group_meta.size.cur = setup_group_object(group);
	group_meta.name = group->title;
	group_meta.type.uuid.type = BT_UUID_TYPE_16;
	group_meta.type.uuid_16.val = BT_UUID_16(group_type)->val;
	BT_OTS_OBJ_SET_PROP_READ(group_meta.props);

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &group_meta);

	if (ret) {
		BT_WARN("Unable to add group object: %d", ret);
		obj.busy = false;
	}

	return ret;
}

/* Add the parent group to the OTS */
static int add_parent_group_object(struct mpl_mediaplayer_t *pl)
{
	int ret;
	struct bt_ots_obj_metadata group_meta = {0};
	struct bt_uuid *group_type = BT_UUID_OTS_TYPE_GROUP;

	if (obj.busy) {
		BT_ERR("Object busy");
		return 0;
	}
	obj.busy = true;
	obj.add_type = MPL_OBJ_PARENT_GROUP;

	group_meta.size.alloc = group_meta.size.cur = setup_parent_group_object(pl->group);
	group_meta.name = pl->group->parent->title;
	group_meta.type.uuid.type = BT_UUID_TYPE_16;
	group_meta.type.uuid_16.val = BT_UUID_16(group_type)->val;
	BT_OTS_OBJ_SET_PROP_READ(group_meta.props);

	ret = bt_ots_obj_add(bt_mcs_get_ots(), &group_meta);
	if (ret) {
		BT_WARN("Unable to add parent group object");
		obj.busy = false;
	}
	return ret;
}

/* Add all tracks of a group to the OTS */
static int add_group_tracks(struct mpl_group_t *group)
{
	int ret_overall = 0;
	struct mpl_track_t *track = group->track;

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
static int add_group_and_track_objects(struct mpl_mediaplayer_t *pl)
{
	int ret_overall = 0;
	int ret;
	struct mpl_group_t *group = pl->group;

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

static void on_obj_deleted(struct bt_ots *ots, struct bt_conn *conn,
			   uint64_t id)
{
	BT_DBG_OBJ_ID("Object Id deleted: ", id);
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
	} else if (id == pl.group->track->next->id) {
		BT_DBG("Next Track Object ID");
		(void)setup_track_object(pl.group->track->next);
	} else if (id == pl.group->id) {
		BT_DBG("Current Group Object ID");
		(void)setup_group_object(pl.group);
	} else if (id == pl.group->parent->id) {
		BT_DBG("Parent Group Object ID");
		(void)setup_parent_group_object(pl.group);
	} else {
		BT_ERR("Unknown Object ID");
		obj.busy = false;
		return;
	}

	obj.selected_id = id;
	obj.busy = false;
}

static int on_obj_created(struct bt_ots *ots, struct bt_conn *conn,
			  uint64_t id,
			  const struct bt_ots_obj_metadata *metadata)
{
	BT_DBG_OBJ_ID("Object Id created: ", id);

	if (!bt_uuid_cmp(&metadata->type.uuid, BT_UUID_OTS_TYPE_MPL_ICON)) {
		BT_DBG("Icon Obj Type");
		if (obj.add_type == MPL_OBJ_ICON) {
			obj.add_type = MPL_OBJ_NONE;
			pl.icon_id = id;
		} else {
			BT_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&metadata->type.uuid,
				BT_UUID_OTS_TYPE_TRACK_SEGMENT)) {
		BT_DBG("Track Segments Obj Type");
		if (obj.add_type == MPL_OBJ_TRACK_SEGMENTS) {
			obj.add_type = MPL_OBJ_NONE;
			pl.group->track->segments_id = id;
		} else {
			BT_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&metadata->type.uuid,
				 BT_UUID_OTS_TYPE_TRACK)) {
		BT_DBG("Track Obj Type");
		if (obj.add_type == MPL_OBJ_TRACK) {
			obj.add_type = MPL_OBJ_NONE;
			obj.add_track->id = id;
			obj.add_track = NULL;
		} else {
			BT_DBG("Unexpected object creation");
		}

	} else if (!bt_uuid_cmp(&metadata->type.uuid,
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


static uint32_t on_object_send(struct bt_ots *ots, struct bt_conn *conn,
			       uint64_t id, uint8_t **data, uint32_t len,
			       uint32_t offset)
{
	if (obj.busy) {
		/* TODO: Can there be a collision between select and internal */
		/* activities, like adding new objects? */
		BT_ERR("Object busy");
		return 0;
	}
	obj.busy = true;

	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		char t[BT_OTS_OBJ_ID_STR_LEN];
		(void)bt_ots_obj_id_to_str(id, t, sizeof(t));
		BT_DBG("Object Id %s, offset %i, length %i", log_strdup(t),
		       offset, len);
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

	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
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

#endif /* CONFIG_BT_OTS */


int mpl_init(void)
{
	static bool initialized;
	int ret;

	if (initialized) {
		BT_DBG("Already initialized");
		return -EALREADY;
	}

	/* Set up the media control service */
#ifdef CONFIG_BT_OTS
	ret = bt_mcs_init(&ots_cbs);
#else
	ret = bt_mcs_init(NULL);
#endif /* CONFIG_BT_OTS */

	if (ret) {
		BT_ERR("Could not init MCS");
	}

	/* Get a Content Control ID */
	pl.content_ctrl_id = ccid_get_value();

#ifdef CONFIG_BT_OTS
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
#endif /* CONFIG_BT_OTS */

	initialized = true;
	return 0;
}

/* TODO: It must be possible to replace the do_prev_segment(), do_prev_track */
/* and do_prev_group() with a generic do_prev() command that can be used at */
/* all levels.	Similarly for do_next, do_prev, and so on. */

void do_prev_segment(struct mpl_mediaplayer_t *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	if (pl->group->track->segment->prev != NULL) {
		pl->group->track->segment = pl->group->track->segment->prev;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_next_segment(struct mpl_mediaplayer_t *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	if (pl->group->track->segment->next != NULL) {
		pl->group->track->segment = pl->group->track->segment->next;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_first_segment(struct mpl_mediaplayer_t *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	while (pl->group->track->segment->prev != NULL) {
		pl->group->track->segment = pl->group->track->segment->prev;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_last_segment(struct mpl_mediaplayer_t *pl)
{
	BT_DBG("Segment name before: %s",
	       log_strdup(pl->group->track->segment->name));

	while (pl->group->track->segment->next != NULL) {
		pl->group->track->segment = pl->group->track->segment->next;
	}

	BT_DBG("Segment name after: %s",
	       log_strdup(pl->group->track->segment->name));
}

void do_goto_segment(struct mpl_mediaplayer_t *pl, int32_t segnum)
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

static bool do_prev_track(struct mpl_mediaplayer_t *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
		track_changed = true;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	return track_changed;
}

static bool do_next_track(struct mpl_mediaplayer_t *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
		track_changed = true;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	return track_changed;
}

static bool do_first_track(struct mpl_mediaplayer_t *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
		track_changed = true;
	}
	while (pl->group->track->prev != NULL) {
		pl->group->track = pl->group->track->prev;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	return track_changed;
}

static bool do_last_track(struct mpl_mediaplayer_t *pl)
{
	bool track_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
		track_changed = true;
	}
	while (pl->group->track->next != NULL) {
		pl->group->track = pl->group->track->next;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	return track_changed;
}

static bool do_goto_track(struct mpl_mediaplayer_t *pl, int32_t tracknum)
{
	int32_t count = 0;
	int32_t k;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID before: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

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

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Track ID after: ", pl->group->track->id);
#endif /* CONFIG_BT_OTS */

	/* The track has changed if we have moved more in one direction */
	/* than in the other */
	return (count != 0);
}


static bool do_prev_group(struct mpl_mediaplayer_t *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
		group_changed = true;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	return group_changed;
}

static bool do_next_group(struct mpl_mediaplayer_t *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->next != NULL) {
		pl->group = pl->group->next;
		group_changed = true;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	return group_changed;
}

static bool do_first_group(struct mpl_mediaplayer_t *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
		group_changed = true;
	}
	while (pl->group->prev != NULL) {
		pl->group = pl->group->prev;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	return group_changed;
}

static bool do_last_group(struct mpl_mediaplayer_t *pl)
{
	bool group_changed = false;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	if (pl->group->next != NULL) {
		pl->group = pl->group->next;
		group_changed = true;
	}
	while (pl->group->next != NULL) {
		pl->group = pl->group->next;
	}

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	return group_changed;
}

static bool  do_goto_group(struct mpl_mediaplayer_t *pl, int32_t groupnum)
{
	int32_t count = 0;
	int32_t k;

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID before: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

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

#ifdef CONFIG_BT_OTS
	BT_DBG_OBJ_ID("Group ID after: ", pl->group->id);
#endif /* CONFIG_BT_OTS */

	/* The group has changed if we have moved more in one direction */
	/* than in the other */
	return (count != 0);
}

void do_track_change_notifications(struct mpl_mediaplayer_t *pl)
{
	mpl_track_changed_cb();
	mpl_track_title_cb(pl->group->track->title);
	mpl_track_duration_cb(pl->group->track->duration);
	mpl_track_position_cb(pl->track_pos);
#ifdef CONFIG_BT_OTS
	mpl_current_track_id_cb(pl->group->track->id);
	if (pl->group->track->next) {
		mpl_next_track_id_cb(pl->group->track->next->id);
	} else {
		/* Send a zero value to indicate that there is no next track */
		mpl_next_track_id_cb(MPL_NO_TRACK_ID);
	}
#endif /* CONFIG_BT_OTS */
}

void do_group_change_notifications(struct mpl_mediaplayer_t *pl)
{
#ifdef CONFIG_BT_OTS
	mpl_group_id_cb(pl->group->id);
#endif /* CONFIG_BT_OTS */
}

void do_full_prev_group(struct mpl_mediaplayer_t *pl)
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
		mpl_track_position_cb(pl->track_pos);
	}
}

void do_full_next_group(struct mpl_mediaplayer_t *pl)
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
		mpl_track_position_cb(pl->track_pos);
	}
}

void do_full_first_group(struct mpl_mediaplayer_t *pl)
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
		mpl_track_position_cb(pl->track_pos);
	}
}

void do_full_last_group(struct mpl_mediaplayer_t *pl)
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
		mpl_track_position_cb(pl->track_pos);
	}
}

void do_full_goto_group(struct mpl_mediaplayer_t *pl, int32_t groupnum)
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
		mpl_track_position_cb(pl->track_pos);
	}
}

/* Control point operation handlers (state machines) */
void inactive_state_operation_handler(struct mpl_op_t operation,
				      struct mpl_op_ntf_t ntf)
{
	BT_DBG("Operation opcode: %d", operation.opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		if (operation.use_param) {
			BT_DBG("Operation parameter: %d", operation.param);
		}
	}
	switch (operation.opcode) {
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
		ntf.result_code = BT_MCS_OPC_NTF_PLAYER_INACTIVE;
		mpl_operation_cb(ntf);
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
			mpl_track_position_cb(pl.track_pos);
		}
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		if (do_next_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (operation.use_param) {
			if (do_goto_track(&pl, operation.param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				mpl_track_position_cb(pl.track_pos);
			}
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			mpl_media_state_cb(pl.state);
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (operation.use_param) {
			do_full_goto_group(&pl, operation.param);
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			mpl_media_state_cb(pl.state);
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	default:
		BT_DBG("Invalid operation: %d", operation.opcode);
		ntf.result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		mpl_operation_cb(ntf);
		break;
	}
}

void playing_state_operation_handler(struct mpl_op_t operation,
				     struct mpl_op_ntf_t ntf)
{
	BT_DBG("Operation opcode: %d", operation.opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		if (operation.use_param) {
			BT_DBG("Operation parameter: %d", operation.param);
		}
	}
	switch (operation.opcode) {
	case BT_MCS_OPC_PLAY:
		/* Continue playing - i.e. do nothing */
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PAUSE:
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_REWIND:
		/* We're in playing state, seeking speed must have been zero */
		pl.seeking_speed_factor = -MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		mpl_media_state_cb(pl.state);
		mpl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_FORWARD:
		/* We're in playing state, seeking speed must have been zero */
		pl.seeking_speed_factor = MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		mpl_media_state_cb(pl.state);
		mpl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_STOP:
		pl.track_pos = 0;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_MOVE_RELATIVE:
		if (operation.use_param) {
			/* Keep within track - i.e. in the range 0 - duration */
			if (operation.param >
			    pl.group->track->duration - pl.track_pos) {
				pl.track_pos = pl.group->track->duration;
			} else if (operation.param < -pl.track_pos) {
				pl.track_pos = 0;
			} else {
				pl.track_pos += operation.param;
			}
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_track_position_cb(pl.track_pos);
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_SEGMENT:
		/* Switch to previous segment if we are less than <margin> */
		/* into the segment, otherwise go to start of segment */
		if (pl.track_pos - PREV_MARGIN <
		    pl.group->track->segment->pos) {
			do_prev_segment(&pl);
		}
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_SEGMENT:
		do_next_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_SEGMENT:
		do_first_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_SEGMENT:
		do_last_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_SEGMENT:
		if (operation.use_param) {
			if (operation.param != 0) {
				do_goto_segment(&pl, operation.param);
				pl.track_pos = pl.group->track->segment->pos;
				mpl_track_position_cb(pl.track_pos);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shal stay the same, and the */
			/* track position shall not change. */
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
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
			mpl_track_position_cb(pl.track_pos);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		if (do_next_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (operation.use_param) {
			if (do_goto_track(&pl, operation.param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				mpl_track_position_cb(pl.track_pos);
			}
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (operation.use_param) {
			do_full_goto_group(&pl, operation.param);
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	default:
		BT_DBG("Invalid operation: %d", operation.opcode);
		ntf.result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		mpl_operation_cb(ntf);
		break;
	}
}

void paused_state_operation_handler(struct mpl_op_t operation,
				    struct mpl_op_ntf_t ntf)
{
	BT_DBG("Operation opcode: %d", operation.opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		if (operation.use_param) {
			BT_DBG("Operation parameter: %d", operation.param);
		}
	}
	switch (operation.opcode) {
	case BT_MCS_OPC_PLAY:
		pl.state = BT_MCS_MEDIA_STATE_PLAYING;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PAUSE:
		/* No change */
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_REWIND:
		/* We're in paused state, seeking speed must have been zero */
		pl.seeking_speed_factor = -MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		mpl_media_state_cb(pl.state);
		mpl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_FORWARD:
		/* We're in paused state, seeking speed must have been zero */
		pl.seeking_speed_factor = MPL_SEEKING_SPEED_FACTOR_STEP;
		pl.state = BT_MCS_MEDIA_STATE_SEEKING;
		mpl_media_state_cb(pl.state);
		mpl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_STOP:
		pl.track_pos = 0;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_MOVE_RELATIVE:
		if (operation.use_param) {
			/* Keep within track - i.e. in the range 0 - duration */
			if (operation.param >
			    pl.group->track->duration - pl.track_pos) {
				pl.track_pos = pl.group->track->duration;
			} else if (operation.param < -pl.track_pos) {
				pl.track_pos = 0;
			} else {
				pl.track_pos += operation.param;
			}
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_track_position_cb(pl.track_pos);
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_SEGMENT:
		/* Switch to previous segment if we are less than 5 seconds */
		/* into the segment, otherwise go to start of segment */
		if (pl.track_pos - PREV_MARGIN <
		    pl.group->track->segment->pos) {
			do_prev_segment(&pl);
		}
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_SEGMENT:
		do_next_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_SEGMENT:
		do_first_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_SEGMENT:
		do_last_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_SEGMENT:
		if (operation.use_param) {
			if (operation.param != 0) {
				do_goto_segment(&pl, operation.param);
				pl.track_pos = pl.group->track->segment->pos;
				mpl_track_position_cb(pl.track_pos);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shal stay the same, and the */
			/* track position shall not change. */
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
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
			mpl_track_position_cb(pl.track_pos);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		if (do_next_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (operation.use_param) {
			if (do_goto_track(&pl, operation.param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				mpl_track_position_cb(pl.track_pos);
			}
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (operation.use_param) {
			do_full_goto_group(&pl, operation.param);
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	default:
		BT_DBG("Invalid operation: %d", operation.opcode);
		ntf.result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		mpl_operation_cb(ntf);
		break;
	}
}

void seeking_state_operation_handler(struct mpl_op_t operation,
				     struct mpl_op_ntf_t ntf)
{
	BT_DBG("Operation opcode: %d", operation.opcode);
	if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) {
		if (operation.use_param) {
			BT_DBG("Operation parameter: %d", operation.param);
		}
	}
	switch (operation.opcode) {
	case BT_MCS_OPC_PLAY:
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PLAYING;
		mpl_media_state_cb(pl.state);
		mpl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PAUSE:
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		/* TODO: Set track and track position */
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		mpl_seeking_speed_cb(pl.seeking_speed_factor);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_REWIND:
		/* TODO: Here, and for FAST_FORWARD */
		/* Decide on algorithm for muliple presses - add step (as */
		/* now) or double/half? */
		/* What about FR followed by FF? */
		/* Currently, the seeking speed may also become	 zero */
		/* Lowest value allowed by spec is -64, notify on change only */
		if (pl.seeking_speed_factor >= -(BT_MCS_SEEKING_SPEED_FACTOR_MAX
						 - MPL_SEEKING_SPEED_FACTOR_STEP)) {
			pl.seeking_speed_factor -= MPL_SEEKING_SPEED_FACTOR_STEP;
			mpl_seeking_speed_cb(pl.seeking_speed_factor);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FAST_FORWARD:
		/* Highest value allowed by spec is 64, notify on change only */
		if (pl.seeking_speed_factor <= (BT_MCS_SEEKING_SPEED_FACTOR_MAX
						- MPL_SEEKING_SPEED_FACTOR_STEP)) {
			pl.seeking_speed_factor += MPL_SEEKING_SPEED_FACTOR_STEP;
			mpl_seeking_speed_cb(pl.seeking_speed_factor);
		}
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_STOP:
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.track_pos = 0;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		mpl_seeking_speed_cb(pl.seeking_speed_factor);
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_MOVE_RELATIVE:
		if (operation.use_param) {
			/* Keep within track - i.e. in the range 0 - duration */
			if (operation.param >
			    pl.group->track->duration - pl.track_pos) {
				pl.track_pos = pl.group->track->duration;
			} else if (operation.param < -pl.track_pos) {
				pl.track_pos = 0;
			} else {
				pl.track_pos += operation.param;
			}
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_track_position_cb(pl.track_pos);
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_SEGMENT:
		/* Switch to previous segment if we are less than 5 seconds */
		/* into the segment, otherwise go to start of segment */
		if (pl.track_pos - PREV_MARGIN <
		    pl.group->track->segment->pos) {
			do_prev_segment(&pl);
		}
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_SEGMENT:
		do_next_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_SEGMENT:
		do_first_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_SEGMENT:
		do_last_segment(&pl);
		pl.track_pos = pl.group->track->segment->pos;
		mpl_track_position_cb(pl.track_pos);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_SEGMENT:
		if (operation.use_param) {
			if (operation.param != 0) {
				do_goto_segment(&pl, operation.param);
				pl.track_pos = pl.group->track->segment->pos;
				mpl_track_position_cb(pl.track_pos);
			}
			/* If the argument to "goto segment" is zero, */
			/* the segment shal stay the same, and the */
			/* track position shall not change. */
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
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
			mpl_track_position_cb(pl.track_pos);
		}
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_TRACK:
		if (do_next_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		}
		/* For next track, the position is kept if the track */
		/* does not change */
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_TRACK:
		if (do_first_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For first track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_TRACK:
		if (do_last_track(&pl)) {
			pl.track_pos = 0;
			do_track_change_notifications(&pl);
		} else {
			/* For last track, the position is reset to 0 even */
			/* if we stay at the same track (goto start of track) */
			pl.track_pos = 0;
			mpl_track_position_cb(pl.track_pos);
		}
		pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_TRACK:
		if (operation.use_param) {
			if (do_goto_track(&pl, operation.param)) {
				pl.track_pos = 0;
				do_track_change_notifications(&pl);
			} else {
				/* For goto track, the position is reset to 0 */
				/* even if we stay at the same track (goto */
				/* start of track) */
				pl.track_pos = 0;
				mpl_track_position_cb(pl.track_pos);
			}
			pl.seeking_speed_factor = BT_MCS_SEEKING_SPEED_FACTOR_ZERO;
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			mpl_media_state_cb(pl.state);
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_PREV_GROUP:
		do_full_prev_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_NEXT_GROUP:
		do_full_next_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_FIRST_GROUP:
		do_full_first_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_LAST_GROUP:
		do_full_last_group(&pl);
		pl.state = BT_MCS_MEDIA_STATE_PAUSED;
		mpl_media_state_cb(pl.state);
		ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		mpl_operation_cb(ntf);
		break;
	case BT_MCS_OPC_GOTO_GROUP:
		if (operation.use_param) {
			do_full_goto_group(&pl, operation.param);
			pl.state = BT_MCS_MEDIA_STATE_PAUSED;
			mpl_media_state_cb(pl.state);
			ntf.result_code = BT_MCS_OPC_NTF_SUCCESS;
		} else {
			ntf.result_code = BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED;
		}
		mpl_operation_cb(ntf);
		break;
	default:
		BT_DBG("Invalid operation: %d", operation.opcode);
		ntf.result_code = BT_MCS_OPC_NTF_NOT_SUPPORTED;
		mpl_operation_cb(ntf);
		break;
	}
}

void (*operation_handlers[BT_MCS_MEDIA_STATE_LAST])(struct mpl_op_t operation,
						 struct mpl_op_ntf_t ntf) = {
	inactive_state_operation_handler,
	playing_state_operation_handler,
	paused_state_operation_handler,
	seeking_state_operation_handler
};

char *mpl_player_name_get(void)
{
	return pl.name;
}

#ifdef CONFIG_BT_OTS
uint64_t mpl_icon_id_get(void)
{
	return pl.icon_id;
}
#endif /* CONFIG_BT_OTS */

char *mpl_icon_uri_get(void)
{
	return pl.icon_uri;
}

char *mpl_track_title_get(void)
{
	return pl.group->track->title;
}

int32_t mpl_track_duration_get(void)
{
	return pl.group->track->duration;
}

int32_t mpl_track_position_get(void)
{
	return pl.track_pos;
}

void mpl_track_position_set(int32_t position)
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
		mpl_track_position_cb(new_pos);
	}
}

int8_t mpl_playback_speed_get(void)
{
	return pl.playback_speed_param;
}

void mpl_playback_speed_set(int8_t speed)
{
	pl.playback_speed_param = speed;
}

int8_t mpl_seeking_speed_get(void)
{
	return pl.seeking_speed_factor;
}

#ifdef CONFIG_BT_OTS
uint64_t mpl_track_segments_id_get(void)
{
	return pl.group->track->segments_id;
}

uint64_t mpl_current_track_id_get(void)
{
	return pl.group->track->id;
}

void mpl_current_track_id_set(uint64_t id)
{
	/* This requires that we have the track with the given ID */
	/* and can find it and switch to it. */
	/* There is also a matter of what to do with the group, */
	/* does the track have a corresponding group, or do we create one? */

	BT_DBG_OBJ_ID("Track ID to set: ", id);

	/* What we really want to do: Check that we have a track with this ID */
	/* Set the track to the track with this ID */
	/* Temporarily, we construct a track with this id, and use that */
	/* We chain it into the current group */
	track_dummy.id = id;
	track_dummy.prev = pl.group->track;
	track_dummy.next = pl.group->track->next;
	pl.group->track->next->prev = &track_dummy;
	pl.group->track->next = &track_dummy;
	pl.group->track = &track_dummy;
	pl.track_pos = 0;
	mpl_current_track_id_cb(id);
	mpl_track_changed_cb(); /* Todo: Spec says not to notify the client */
				/* who set the track */
}

uint64_t mpl_next_track_id_get(void)
{
	if (pl.group->track->next) {
		return pl.group->track->next->id;
	}

	/* Return zero value to indicate that there is no next track */
	return MPL_NO_TRACK_ID;
}

void mpl_next_track_id_set(uint64_t id)
{
	BT_DBG_OBJ_ID("Track ID to set: ", id);

	/* What we really want to do: Set the track to the track with this ID */
	/* Temporarily, we construct a track with this id, and use that */
	track_timmy.id = id;
	track_timmy.prev = pl.group->track;
	track_timmy.next = pl.group->track->next;
	pl.group->track->next->prev = &track_dummy;
	pl.group->track->next = &track_dummy;
	mpl_next_track_id_cb(id); /* Todo: Spec says not to notify the client */
				  /* who set the track */
}

uint64_t mpl_group_id_get(void)
{
	return pl.group->id;
}

void mpl_group_id_set(uint64_t id)
{
	BT_DBG_OBJ_ID("Group ID to set: ", id);
	pl.group->id = id;
}

uint64_t mpl_parent_group_id_get(void)
{
	return pl.group->parent->id;
}
#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
void mpl_test_unset_parent_group(void)
{
	BT_DBG("Setting current group to be it's own parent");
	pl.group->parent = pl.group;
}
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */
#endif /* CONFIG_BT_OTS */

uint8_t mpl_playing_order_get(void)
{
	return pl.playing_order;
}

void mpl_playing_order_set(uint8_t order)
{
	if (BIT(order - 1) & pl.playing_orders_supported) {
		pl.playing_order = order;
	}
}

uint16_t mpl_playing_orders_supported_get(void)
{
	return pl.playing_orders_supported;
}

uint8_t mpl_media_state_get(void)
{
	return pl.state;
}

#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
void mpl_test_media_state_set(uint8_t state)
{
	pl.state = state;
	mpl_media_state_cb(pl.state);
}
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */

void mpl_operation_set(struct mpl_op_t operation)
{
	struct mpl_op_ntf_t ntf;

	BT_DBG("opcode: %d, param: %d", operation.opcode, operation.param);

	if (pl.state < BT_MCS_MEDIA_STATE_LAST) {
		ntf.requested_opcode = operation.opcode;
		operation_handlers[pl.state](operation, ntf);
	} else {
		BT_DBG("INVALID STATE");
	}
}

uint32_t mpl_operations_supported_get(void)
{
	return pl.operations_supported;
}

#ifdef CONFIG_BT_OTS
static void parse_search(struct mpl_search_t search)
{
	uint8_t index = 0;
	struct mpl_sci_t sci;
	uint8_t sci_num = 0;
	bool search_failed = false;

	if (search.len > SEARCH_LEN_MAX) {
		BT_WARN("Search too long (%d) - truncating", search.len);
		search.len = SEARCH_LEN_MAX;
	}
	BT_DBG("Parsing %d octets search", search.len);

	while (search.len - index > 0) {
		sci.len = (uint8_t)search.search[index++];
		if (sci.len < SEARCH_SCI_LEN_MIN) {
			BT_WARN("Invalid length field - too small");
			search_failed = true;
			break;
		}
		if (sci.len > (search.len - index)) {
			BT_WARN("Incomplete search control item");
			search_failed = true;
			break;
		}
		sci.type = (uint8_t)search.search[index++];
		if (sci.type <  BT_MCS_SEARCH_TYPE_TRACK_NAME ||
		    sci.type > BT_MCS_SEARCH_TYPE_ONLY_GROUPS) {
			search_failed = true;
			break;
		}
		memcpy(&sci.param, &search.search[index], sci.len - 1);
		index += sci.len - 1;

		BT_DBG("SCI # %d: type: %d", sci_num, sci.type);
		BT_HEXDUMP_DBG(sci.param, sci.len-1, "param:");
		sci_num++;
	}

	/* TODO: Add real search functionality. */
	/* For now, just fake it. */

	if (search_failed) {
		pl.search_results_id = 0;
		mpl_search_cb(BT_MCS_SCP_NTF_FAILURE);
	} else {
		/* Use current group as search result for now */
		pl.search_results_id = pl.group->id;
		mpl_search_cb(BT_MCS_SCP_NTF_SUCCESS);
	}

	mpl_search_results_id_cb(pl.search_results_id);
}

void mpl_scp_set(struct mpl_search_t search)
{
	if (search.len > SEARCH_LEN_MAX) {
		BT_WARN("Search too long: %d", search.len);
	}

	BT_HEXDUMP_DBG(search.search, search.len, "Search");

	parse_search(search);
}

uint64_t mpl_search_results_id_get(void)
{
	return pl.search_results_id;
}
#endif /* CONFIG_BT_OTS */

uint8_t mpl_content_ctrl_id_get(void)
{
	return pl.content_ctrl_id;
}

#if CONFIG_BT_DEBUG_MCS
void mpl_debug_dump_state(void)
{
#if CONFIG_BT_OTS
	char t[BT_OTS_OBJ_ID_STR_LEN];
	struct mpl_group_t *group;
	struct mpl_track_t *track;
#endif /* CONFIG_BT_OTS */

	BT_DBG("Mediaplayer name: %s", log_strdup(pl.name));

#if CONFIG_BT_OTS
	(void)bt_ots_obj_id_to_str(pl.icon_id, t, sizeof(t));
	BT_DBG("Icon ID: %s", log_strdup(t));
#endif /* CONFIG_BT_OTS */

	BT_DBG("Icon URI: %s", log_strdup(pl.icon_uri));
	BT_DBG("Track position: %d", pl.track_pos);
	BT_DBG("Media state: %d", pl.state);
	BT_DBG("Playback speed parameter: %d", pl.playback_speed_param);
	BT_DBG("Seeking speed factor: %d", pl.seeking_speed_factor);
	BT_DBG("Playing order: %d", pl.playing_order);
	BT_DBG("Playing orders supported: 0x%x", pl.playing_orders_supported);
	BT_DBG("Operations supported: %d", pl.operations_supported);
	BT_DBG("Content control ID: %d", pl.content_ctrl_id);

#if CONFIG_BT_OTS
	(void)bt_ots_obj_id_to_str(pl.group->id, t, sizeof(t));
	BT_DBG("Current group: %s", log_strdup(t));

	(void)bt_ots_obj_id_to_str(pl.group->parent->id, t, sizeof(t));
	BT_DBG("Current group's parent: %s", log_strdup(t));

	(void)bt_ots_obj_id_to_str(pl.group->track->id, t, sizeof(t));
	BT_DBG("Current track: %s", log_strdup(t));

	if (pl.group->track->next) {
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
#endif /* CONFIG_BT_OTS */
}
#endif /* CONFIG_BT_DEBUG_MCS */
