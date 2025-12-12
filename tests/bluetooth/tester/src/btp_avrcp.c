/* btp_avrcp.c - Bluetooth AVRCP Tester */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/avrcp_cover_art.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_avrcp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#if defined(CONFIG_BT_AVRCP_TARGET)
#define AVRCP_VFS_PATH_MAX_LEN      30
#define AVRCP_SEARCH_STRING_MAX_LEN 20
#define AVRCP_SEARCH_MAX_DEPTH      10
#endif /* CONFIG_BT_AVRCP_TARGET */

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
#define IMAGE_HANDLE_LEN         7
#define IMAGE_HANDLE_UNICODE_LEN 16
#define IMAGE_1_HANDLE           "0000001"
#define IMAGE_1_HANDLE_UNICODE   "\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"
#define IMAGE_2_HANDLE           "0000002"
#define IMAGE_2_HANDLE_UNICODE   "\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x32\x00\x00"
#define IMAGE_PIXEL              "300*300"
#define IMAGE_THUMBNAIL_PIXEL    "200*200"
#define IMAGE_ENCODING           "JPEG"
#define IMAGE_1_PROPERTIES_BODY                                                                    \
	"<image-properties version=\"1.0\" handle=\"" IMAGE_1_HANDLE "\">"                         \
	"<native encoding=\"" IMAGE_ENCODING "\" pixel=\" " IMAGE_PIXEL " \" />"                   \
	"<variant encoding=\"" IMAGE_ENCODING "\" pixel=\"" IMAGE_THUMBNAIL_PIXEL "\" />"          \
	"</image-properties>"
#define IMAGE_2_PROPERTIES_BODY                                                                    \
	"<image-properties version=\"1.0\" handle=\"" IMAGE_2_HANDLE "\">"                         \
	"<native encoding=\"" IMAGE_ENCODING "\" pixel=\" " IMAGE_PIXEL " \" />"                   \
	"<variant encoding=\"" IMAGE_ENCODING "\" pixel=\"" IMAGE_THUMBNAIL_PIXEL "\" />"          \
	"</image-properties>"

struct image_variant {
	const uint8_t *encoding;
	const uint8_t *pixel;
	const uint32_t image_len;
	const uint8_t *image;
};

struct image_item {
	const uint8_t *handle;
	const uint32_t props_len;
	const uint8_t *props;
	uint8_t num_variants;
	struct image_variant *variants;
};
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

struct bt_conn *default_conn;

#if defined(CONFIG_BT_AVRCP_CONTROLLER)
struct bt_avrcp_ct *default_ct;
static uint8_t ct_local_tid;
static struct k_work_delayable ct_uids_changed_event;
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

#if defined(CONFIG_BT_AVRCP_TARGET)
struct media_attr_list {
	uint8_t attr_count;  /**< 0x00 = all attributes, else count */
	uint32_t attr_ids[]; /**< Attribute IDs (bt_avrcp_media_attr_t) */
} __packed;

struct media_attr {
	uint32_t attr_id;
	uint16_t charset_id;
	uint16_t attr_len;
	const uint8_t *attr_val;
};

struct player_attr {
	uint8_t attr_id;
	uint8_t attr_val;
	const uint8_t attr_val_min;
	const uint8_t attr_val_max;
	uint16_t charset_id;
	const uint8_t *attr_text;
	const uint8_t **val_text;
};

struct player_item {
	uint8_t item_type;
	uint16_t item_len;
	uint16_t player_id;
	uint8_t major_type;
	uint32_t sub_type;
	uint8_t play_status;
	uint8_t feature_bitmask[16];
	uint16_t charset_id;
	uint16_t name_len;
	const uint8_t *name;
	uint8_t num_attrs;
	struct player_attr *attr;
};

struct item_hdr {
	uint8_t item_type;
	uint64_t uid;
	uint16_t charset_id;
	uint16_t name_len;
	const uint8_t *name;
};

struct folder_item {
	struct item_hdr hdr;
	uint8_t folder_type;
	uint8_t is_playable;
};

struct media_item {
	struct item_hdr hdr;
	uint8_t media_type;
	uint8_t num_attrs;
	struct media_attr *attr;
};

struct vfs_node {
	sys_snode_t node;
	struct item_hdr *item;
	sys_slist_t child; /* Only used when item_type is Folder Item */
};

NET_BUF_POOL_DEFINE(avrcp_tx_pool, 1, 1024, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
static struct bt_avrcp_tg *default_tg;
static bool tg_long_metadata;
static uint8_t tg_volume;
static uint16_t tg_uid_counter;
static uint64_t tg_uid;
static uint8_t tg_cur_player_idx;
static uint8_t tg_reg_events[13];
static uint8_t tg_addr_player_changed_events[] = {
	BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED,     BT_AVRCP_EVT_TRACK_CHANGED,
	BT_AVRCP_EVT_TRACK_REACHED_END,           BT_AVRCP_EVT_TRACK_REACHED_START,
	BT_AVRCP_EVT_PLAYBACK_POS_CHANGED,        BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED,
	BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED,
};
static struct k_work_delayable tg_send_addr_player_changed_event;

static struct player_item tg_player_items[] = {
	{
		.item_type = BT_AVRCP_ITEM_TYPE_MEDIA_PLAYER, /* Media Player Item */
		.item_len = 36,
		.player_id = 0x0001,
		.major_type = 0x01,     /* Audio */
		.sub_type = 0x00000001, /* Audio Book */
		.play_status = 0x00,    /* Stopped */
		.feature_bitmask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x00,
				    0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.name_len = 8,
		.name = (const uint8_t *)"player 1",
		.num_attrs = 1,
		.attr =
			(struct player_attr[]){
				{
					.attr_id = BT_AVRCP_PLAYER_ATTR_EQUALIZER,
					.attr_val = 1,
					.attr_val_min = 1,
					.attr_val_max = 2,
					.charset_id = BT_AVRCP_CHARSET_UTF8,
					.attr_text = (const uint8_t *)"EQUALIZER",
					.val_text = (const uint8_t *[]){NULL, "OFF", "ON"},
				},
			},
	},
	{
		.item_type = BT_AVRCP_ITEM_TYPE_MEDIA_PLAYER, /* Media Player Item */
		.item_len = 36,
		.player_id = 0x0002,
		.major_type = 0x01,     /* Audio */
		.sub_type = 0x00000001, /* Audio Book */
		.play_status = 0x00,    /* Stopped */
		.feature_bitmask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x00,
				    0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.name_len = 8,
		.name = (const uint8_t *)"player 2",
		.num_attrs = 1,
		.attr =
			(struct player_attr[]){
				{
					.attr_id = BT_AVRCP_PLAYER_ATTR_EQUALIZER,
					.attr_val = 1,
					.attr_val_min = 1,
					.attr_val_max = 2,
					.charset_id = BT_AVRCP_CHARSET_UTF8,
					.attr_text = (const uint8_t *)"EQUALIZER",
					.val_text = (const uint8_t *[]){NULL, "OFF", "ON"},
				},
			},
	},
};

static struct folder_item tg_folder_items[] = {
	{
		.hdr.item_type = BT_AVRCP_ITEM_TYPE_FOLDER, /* Folder Item */
		.hdr.uid = 0,
		.hdr.charset_id = BT_AVRCP_CHARSET_UTF8,
		.hdr.name_len = 4,
		.hdr.name = (const uint8_t *)"/", /* root */
		.folder_type = 0x00,              /* Mixed */
		.is_playable = 0x00,              /* Not Played */
	},
	{
		.hdr.item_type = BT_AVRCP_ITEM_TYPE_FOLDER, /* Folder Item */
		.hdr.uid = 0,
		.hdr.charset_id = BT_AVRCP_CHARSET_UTF8,
		.hdr.name_len = 9,
		.hdr.name = (const uint8_t *)"songlists",
		.folder_type = 0x01, /* Titles */
		.is_playable = 0x01, /* Played */
	},
	{
		.hdr.item_type = BT_AVRCP_ITEM_TYPE_FOLDER, /* Folder Item */
		.hdr.uid = 0,
		.hdr.charset_id = BT_AVRCP_CHARSET_UTF8,
		.hdr.name_len = 19,
		.hdr.name = (const uint8_t *)"no_cover_art_folder",
		.folder_type = 0x01, /* Titles */
		.is_playable = 0x01, /* Played */
	},
	{
		.hdr.item_type = BT_AVRCP_ITEM_TYPE_FOLDER, /* Folder Item */
		.hdr.uid = 0,
		.hdr.charset_id = BT_AVRCP_CHARSET_UTF8,
		.hdr.name_len = 12,
		.hdr.name = (const uint8_t *)"empty_folder",
		.folder_type = 0x01, /* Titles */
		.is_playable = 0x00, /* Not Played */
	},
};

static struct media_item tg_media_elem_items[] = {
	{.hdr.item_type = BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT, /* Media Element Item */
	 .hdr.uid = 0,
	 .hdr.charset_id = BT_AVRCP_CHARSET_UTF8,
	 .hdr.name_len = 6,
	 .hdr.name = (const uint8_t *)"song 1",
	 .media_type = 0x00, /* Audio */
	 .num_attrs = 7,
	 .attr =
		(struct media_attr[]){
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_TITLE,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 6,
				.attr_val = (const uint8_t *)"song 1",
			},
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_ARTIST,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 6U,
				.attr_val = (const uint8_t *)"Artist",
			},
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_ALBUM,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 5U,
				.attr_val = (const uint8_t *)"Album",
			},
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_TRACK_NUMBER,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 1U,
				.attr_val = (const uint8_t *)"1",
			},
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_TOTAL_TRACKS,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 2U,
				.attr_val = (const uint8_t *)"10",
			},
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_GENRE,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 4U,
				.attr_val = (const uint8_t *)"Rock",
			},
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_PLAYING_TIME,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 6U,
				.attr_val =
					(const uint8_t *)"240000", /* 4 minutes in milliseconds */
			},
#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_DEFAULT_COVER_ART,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = IMAGE_HANDLE_LEN,
				.attr_val = (const uint8_t *)IMAGE_1_HANDLE,
			},
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */
		}},
	{.hdr.item_type = BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT, /* Media Element Item */
	 .hdr.uid = 0,
	 .hdr.charset_id = BT_AVRCP_CHARSET_UTF8,
	 .hdr.name_len = 6,
	 .hdr.name = (const uint8_t *)"song 2",
	 .media_type = 0x00, /* Audio */
	 .num_attrs = 1,
	 .attr =
		(struct media_attr[]){
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_TITLE,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = 6,
				.attr_val = (const uint8_t *)"song 2",
			},
#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
			{
				.attr_id = BT_AVRCP_MEDIA_ATTR_ID_DEFAULT_COVER_ART,
				.charset_id = BT_AVRCP_CHARSET_UTF8,
				.attr_len = IMAGE_HANDLE_LEN,
				.attr_val = (const uint8_t *)IMAGE_2_HANDLE,
			},
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */
		}},
	{.hdr.item_type = BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT, /* Media Element Item */
	 .hdr.uid = 0,
	 .hdr.charset_id = BT_AVRCP_CHARSET_UTF8,
	 .hdr.name_len = 6,
	 .hdr.name = (const uint8_t *)"song 3",
	 .media_type = 0x00, /* Audio */
	 .num_attrs = 1,
	 .attr = (struct media_attr[]){{
		.attr_id = BT_AVRCP_MEDIA_ATTR_ID_TITLE,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 6,
		.attr_val = (const uint8_t *)"song 3",
	 }}},
};

static const uint8_t tg_long_title[] =
	"1. This is a long title that is designed to test the fragmentation of the AVRCP. "
	"2. This is a long title that is designed to test the fragmentation of the AVRCP. "
	"3. This is a long title that is designed to test the fragmentation of the AVRCP. "
	"4. This is a long title that is designed to test the fragmentation of the AVRCP. "
	"5. This is a long title that is designed to test the fragmentation of the AVRCP. "
	"6. This is a long title that is designed to test the fragmentation of the AVRCP.";

/* tg_node_pool will be added to tg_now_playing_list or tg_search_list */
static struct vfs_node tg_node_pool[ARRAY_SIZE(tg_media_elem_items) * 2U];
static struct vfs_node tg_vfs_node[ARRAY_SIZE(tg_folder_items) + ARRAY_SIZE(tg_media_elem_items)];
static sys_slist_t tg_now_playing_list;
static sys_slist_t tg_search_list;
static struct media_item *tg_playing_item;
static char tg_cur_vfs_path[AVRCP_VFS_PATH_MAX_LEN] = "/";
#endif /* CONFIG_BT_AVRCP_TARGET */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART) || defined(CONFIG_BT_AVRCP_TG_COVER_ART)
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(ca_tx_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_GOEP_L2CAP_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
static struct bt_avrcp_cover_art_ct *default_ca_ct;
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
static struct bt_avrcp_cover_art_tg *default_ca_tg;
static uint16_t ca_tg_mopl;
static const uint8_t *ca_tg_body;
static uint32_t ca_tg_body_len;
static uint32_t ca_tg_body_pos;

/*
 * White JPEG thumbnail (200x200 pixels)
 * JPEG baseline-compliant, sRGB, YCC422 sampling
 */
static const uint8_t ca_tg_thumbnail_200x200[] = {
	/* SOI (Start of Image) marker */
	0xFF, 0xD8,

	/* APP0 (JFIF) marker segment */
	0xFF, 0xE0, 0x00, 0x10,       /* Length: 16 bytes */
	0x4A, 0x46, 0x49, 0x46, 0x00, /* Identifier: "JFIF\0" */
	0x01, 0x01,                   /* Version: 1.1 */
	0x01,                         /* Units: dots per inch */
	0x00, 0x48,                   /* X density: 72 dpi */
	0x00, 0x48,                   /* Y density: 72 dpi */
	0x00,                         /* Thumbnail width: 0 */
	0x00,                         /* Thumbnail height: 0 */

	/* APP14 (Adobe) marker for sRGB color space */
	0xFF, 0xEE, 0x00, 0x0E,       /* Length: 14 bytes */
	0x41, 0x64, 0x6F, 0x62, 0x65, /* Identifier: "Adobe" */
	0x00, 0x64,                   /* Version: 100 */
	0x00, 0x00,                   /* Flags0: 0 */
	0x00, 0x00,                   /* Flags1: 0 */
	0x01,                         /* Color transform: YCbCr */

	/* DQT (Define Quantization Table) marker - Luminance */
	0xFF, 0xDB, 0x00, 0x43, /* Length: 67 bytes */
	0x00,                   /* Table ID: 0, 8-bit precision */
	/* Quantization table for Y component (optimized for white) */
	0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07, 0x07, 0x07, 0x09, 0x09, 0x08, 0x0A, 0x0C,
	0x14, 0x0D, 0x0C, 0x0B, 0x0B, 0x0C, 0x19, 0x12, 0x13, 0x0F, 0x14, 0x1D, 0x1A, 0x1F, 0x1E,
	0x1D, 0x1A, 0x1C, 0x1C, 0x20, 0x24, 0x2E, 0x27, 0x20, 0x22, 0x2C, 0x23, 0x1C, 0x1C, 0x28,
	0x37, 0x29, 0x2C, 0x30, 0x31, 0x34, 0x34, 0x34, 0x1F, 0x27, 0x39, 0x3D, 0x38, 0x32, 0x3C,
	0x2E, 0x33, 0x34, 0x32,

	/* DQT (Define Quantization Table) marker - Chrominance */
	0xFF, 0xDB, 0x00, 0x43, /* Length: 67 bytes */
	0x01,                   /* Table ID: 1, 8-bit precision */
	/* Quantization table for Cb/Cr components (optimized for white) */
	0x09, 0x09, 0x09, 0x0C, 0x0B, 0x0C, 0x18, 0x0D, 0x0D, 0x18, 0x32, 0x21, 0x1C, 0x21, 0x32,
	0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
	0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
	0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
	0x32, 0x32, 0x32, 0x32,

	/* SOF0 (Start of Frame - Baseline DCT) marker */
	0xFF, 0xC0, 0x00, 0x11, /* Length: 17 bytes */
	0x08,                   /* Sample precision: 8 bits */
	0x00, 0xC8,             /* Image height: 200 pixels */
	0x00, 0xC8,             /* Image width: 200 pixels */
	0x03,                   /* Number of components: 3 (Y, Cb, Cr) */
	/* Component 1 (Y - Luminance) */
	0x01, /* Component ID: 1 */
	0x22, /* Sampling factors: H=2, V=2 (4:2:2) */
	0x00, /* Quantization table ID: 0 */
	/* Component 2 (Cb - Chrominance blue) */
	0x02, /* Component ID: 2 */
	0x11, /* Sampling factors: H=1, V=1 */
	0x01, /* Quantization table ID: 1 */
	/* Component 3 (Cr - Chrominance red) */
	0x03, /* Component ID: 3 */
	0x11, /* Sampling factors: H=1, V=1 */
	0x01, /* Quantization table ID: 1 */

	/* DHT (Define Huffman Table) marker - DC Luminance */
	0xFF, 0xC4, 0x00, 0x1F, /* Length: 31 bytes */
	0x00,                   /* Table class: DC, Table ID: 0 */
	/* Number of codes of each length (1-16) */
	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) marker - AC Luminance */
	0xFF, 0xC4, 0x00, 0xB5, /* Length: 181 bytes */
	0x10,                   /* Table class: AC, Table ID: 0 */
	/* Number of codes of each length (1-16) */
	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01,
	0x7D,
	/* Huffman values */
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61,
	0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52,
	0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25,
	0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
	0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64,
	0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
	0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3,
	0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8,
	0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* DHT (Define Huffman Table) marker - DC Chrominance */
	0xFF, 0xC4, 0x00, 0x1F, /* Length: 31 bytes */
	0x01,                   /* Table class: DC, Table ID: 1 */
	/* Number of codes of each length (1-16) */
	0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) marker - AC Chrominance */
	0xFF, 0xC4, 0x00, 0xB5, /* Length: 181 bytes */
	0x11,                   /* Table class: AC, Table ID: 1 */
	/* Number of codes of each length (1-16) */
	0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02,
	0x77,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
	0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33,
	0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18,
	0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
	0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
	0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
	0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* SOS (Start of Scan) marker */
	0xFF, 0xDA, 0x00, 0x0C, /* Length: 12 bytes */
	0x03,                   /* Number of components: 3 */
	/* Component 1 (Y - Luminance) */
	0x01, /* Component ID: 1 */
	0x00, /* DC table: 0, AC table: 0 */
	/* Component 2 (Cb - Chrominance blue) */
	0x02, /* Component ID: 2 */
	0x11, /* DC table: 1, AC table: 1 */
	/* Component 3 (Cr - Chrominance red) */
	0x03, /* Component ID: 3 */
	0x11, /* DC table: 1, AC table: 1 */
	0x00, /* Start of spectral selection: 0 */
	0x3F, /* End of spectral selection: 63 */
	0x00, /* Successive approximation: 0 */

	/* Compressed image data (minimal white image data) */
	/* DC coefficient for white (Y=255, Cb=128, Cr=128) */
	0xFF, 0xC0, 0x00, 0x1F, 0xFF, 0xD9};

/*
 * White JPEG original image (300x300 pixels)
 * JPEG baseline-compliant, sRGB, YCC422 sampling
 */
static const uint8_t ca_tg_jpeg_300x300[] = {
	/* SOI (Start of Image) marker */
	0xFF, 0xD8,

	/* APP0 (JFIF) marker segment */
	0xFF, 0xE0, 0x00, 0x10,       /* Length: 16 bytes */
	0x4A, 0x46, 0x49, 0x46, 0x00, /* Identifier: "JFIF\0" */
	0x01, 0x01,                   /* Version: 1.1 */
	0x01,                         /* Units: dots per inch */
	0x00, 0x48,                   /* X density: 72 dpi */
	0x00, 0x48,                   /* Y density: 72 dpi */
	0x00,                         /* Thumbnail width: 0 */
	0x00,                         /* Thumbnail height: 0 */

	/* APP14 (Adobe) marker for sRGB color space */
	0xFF, 0xEE, 0x00, 0x0E,       /* Length: 14 bytes */
	0x41, 0x64, 0x6F, 0x62, 0x65, /* Identifier: "Adobe" */
	0x00, 0x64,                   /* Version: 100 */
	0x00, 0x00,                   /* Flags0: 0 */
	0x00, 0x00,                   /* Flags1: 0 */
	0x01,                         /* Color transform: YCbCr */

	/* DQT (Define Quantization Table) marker - Luminance */
	0xFF, 0xDB, 0x00, 0x43, /* Length: 67 bytes */
	0x00,                   /* Table ID: 0, 8-bit precision */
	/* Quantization table for Y component (optimized for white, higher compression) */
	0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18,
	0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C,
	0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44, 0x57, 0x45, 0x37, 0x38, 0x50,
	0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71, 0x79, 0x70, 0x64, 0x78,
	0x5C, 0x65, 0x67, 0x63,

	/* DQT (Define Quantization Table) marker - Chrominance */
	0xFF, 0xDB, 0x00, 0x43, /* Length: 67 bytes */
	0x01,                   /* Table ID: 1, 8-bit precision */
	/* Quantization table for Cb/Cr components (optimized for white, higher compression) */
	0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A, 0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63,

	/* SOF0 (Start of Frame - Baseline DCT) marker */
	0xFF, 0xC0, 0x00, 0x11, /* Length: 17 bytes */
	0x08,                   /* Sample precision: 8 bits */
	0x01, 0x2C,             /* Image height: 300 pixels */
	0x01, 0x2C,             /* Image width: 300 pixels */
	0x03,                   /* Number of components: 3 (Y, Cb, Cr) */
	/* Component 1 (Y - Luminance) */
	0x01, /* Component ID: 1 */
	0x22, /* Sampling factors: H=2, V=2 (4:2:2) */
	0x00, /* Quantization table ID: 0 */
	/* Component 2 (Cb - Chrominance blue) */
	0x02, /* Component ID: 2 */
	0x11, /* Sampling factors: H=1, V=1 */
	0x01, /* Quantization table ID: 1 */
	/* Component 3 (Cr - Chrominance red) */
	0x03, /* Component ID: 3 */
	0x11, /* Sampling factors: H=1, V=1 */
	0x01, /* Quantization table ID: 1 */

	/* DHT (Define Huffman Table) marker - DC Luminance */
	0xFF, 0xC4, 0x00, 0x1F, /* Length: 31 bytes */
	0x00,                   /* Table class: DC, Table ID: 0 */
	/* Number of codes of each length (1-16) */
	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) marker - AC Luminance */
	0xFF, 0xC4, 0x00, 0xB5, /* Length: 181 bytes */
	0x10,                   /* Table class: AC, Table ID: 0 */
	/* Number of codes of each length (1-16) */
	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01,
	0x7D,
	/* Huffman values */
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61,
	0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52,
	0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25,
	0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
	0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64,
	0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
	0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3,
	0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8,
	0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* DHT (Define Huffman Table) marker - DC Chrominance */
	0xFF, 0xC4, 0x00, 0x1F, /* Length: 31 bytes */
	0x01,                   /* Table class: DC, Table ID: 1 */
	/* Number of codes of each length (1-16) */
	0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) marker - AC Chrominance */
	0xFF, 0xC4, 0x00, 0xB5, /* Length: 181 bytes */
	0x11,                   /* Table class: AC, Table ID: 1 */
	/* Number of codes of each length (1-16) */
	0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02,
	0x77,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
	0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33,
	0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18,
	0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
	0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
	0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
	0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* SOS (Start of Scan) marker */
	0xFF, 0xDA, 0x00, 0x0C, /* Length: 12 bytes */
	0x03,                   /* Number of components: 3 */
	/* Component 1 (Y - Luminance) */
	0x01, /* Component ID: 1 */
	0x00, /* DC table: 0, AC table: 0 */
	/* Component 2 (Cb - Chrominance blue) */
	0x02, /* Component ID: 2 */
	0x11, /* DC table: 1, AC table: 1 */
	/* Component 3 (Cr - Chrominance red) */
	0x03, /* Component ID: 3 */
	0x11, /* DC table: 1, AC table: 1 */
	0x00, /* Start of spectral selection: 0 */
	0x3F, /* End of spectral selection: 63 */
	0x00, /* Successive approximation: 0 */

	/* Compressed image data (minimal white image data for 300x300) */
	/* Optimized entropy-coded data for pure white image */
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F,

	/* EOI (End of Image) marker */
	0xFF, 0xD9};

static struct image_item ca_tg_image_items[] = {
	{
		.handle = IMAGE_1_HANDLE_UNICODE,
		.props_len = sizeof(IMAGE_1_PROPERTIES_BODY),
		.props = IMAGE_1_PROPERTIES_BODY,
		.num_variants = 2,
		.variants =
			(struct image_variant[]){
				{
					.encoding = IMAGE_ENCODING,
					.pixel = IMAGE_PIXEL,
					.image_len = sizeof(ca_tg_jpeg_300x300),
					.image = ca_tg_jpeg_300x300,
				},
				{
					.encoding = IMAGE_ENCODING,
					.pixel = IMAGE_THUMBNAIL_PIXEL,
					.image_len = sizeof(ca_tg_thumbnail_200x200),
					.image = ca_tg_thumbnail_200x200,
				}},
	},
	{
		.handle = IMAGE_2_HANDLE_UNICODE,
		.props_len = sizeof(IMAGE_2_PROPERTIES_BODY),
		.props = IMAGE_2_PROPERTIES_BODY,
		.num_variants = 2,
		.variants =
			(struct image_variant[]){
				{
					.encoding = IMAGE_ENCODING,
					.pixel = IMAGE_PIXEL,
					.image_len = sizeof(ca_tg_jpeg_300x300),
					.image = ca_tg_jpeg_300x300,
				},
				{
					.encoding = IMAGE_ENCODING,
					.pixel = IMAGE_THUMBNAIL_PIXEL,
					.image_len = sizeof(ca_tg_thumbnail_200x200),
					.image = ca_tg_thumbnail_200x200,
				}},
	}};
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART || CONFIG_BT_AVRCP_TG_COVER_ART*/

static uint8_t avrcp_read_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	struct btp_avrcp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_AVRCP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_AVRCP_CONTROLLER)
static uint8_t get_next_tid(void)
{
	uint8_t ret = ct_local_tid;

	ct_local_tid++;
	ct_local_tid &= 0x0F;

	return ret;
}

static void ct_uids_changed_event_handler(struct k_work *work)
{
	struct bt_avrcp_get_folder_items_cmd *p;
	struct net_buf *buf;
	int err;

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		k_work_schedule(&ct_uids_changed_event, K_MSEC(10));
		return;
	}

	if (net_buf_tailroom(buf) < sizeof(*p)) {
		net_buf_unref(buf);
		k_work_schedule(&ct_uids_changed_event, K_MSEC(10));
		return;
	}

	p = net_buf_add(buf, sizeof(*p));
	p->scope = BT_AVRCP_SCOPE_VFS;
	p->start_item = sys_cpu_to_be32(0);
	p->end_item = sys_cpu_to_be32(10);
	p->attr_count = 0;
	err = bt_avrcp_ct_get_folder_items(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		k_work_schedule(&ct_uids_changed_event, K_MSEC(10));
	}
}

static int decode_media_elem_attrs(struct net_buf *buf, struct bt_avrcp_media_attr *attr,
				   uint8_t num_attrs, uint16_t *out_attr_len)
{
	struct bt_avrcp_media_attr *src_attr;
	struct bt_avrcp_media_attr *dst_attr = (struct bt_avrcp_media_attr *)attr;
	uint16_t total_attr_len = 0;
	int err = 0;

	for (uint8_t j = 0; j < num_attrs; j++) {
		uint16_t attr_len;

		if (buf->len < sizeof(*src_attr)) {
			err = -ENOMEM;
			goto done;
		}

		src_attr = (struct bt_avrcp_media_attr *)net_buf_pull_mem(buf, sizeof(*src_attr));

		dst_attr->attr_id = sys_cpu_to_le32(sys_be32_to_cpu(src_attr->attr_id));
		dst_attr->charset_id = sys_cpu_to_le16(sys_be16_to_cpu(src_attr->charset_id));
		attr_len = sys_be16_to_cpu(src_attr->attr_len);
		dst_attr->attr_len = sys_cpu_to_le16(attr_len);

		if (buf->len < attr_len) {
			err = -ENOMEM;
			goto done;
		}

		memcpy(dst_attr->attr_val, net_buf_pull_mem(buf, attr_len), attr_len);
		dst_attr = (struct bt_avrcp_media_attr *)(dst_attr->attr_val + attr_len);

		total_attr_len += sizeof(*src_attr) + attr_len;
	}

done:
	*out_attr_len = total_attr_len;

	return err;
}

static uint8_t control_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_control_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_connect(conn);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t control_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_control_disconnect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_disconnect(conn);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t browsing_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_browsing_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_browsing_connect(conn);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t browsing_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_browsing_disconnect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_browsing_disconnect(conn);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t unit_info(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_unit_info_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_get_unit_info(default_ct, get_next_tid());
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t subunit_info(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_subunit_info_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_get_subunit_info(default_ct, get_next_tid());
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pass_through(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_pass_through_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (cmd_len < sizeof(struct btp_avrcp_pass_through_cmd)) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_passthrough(default_ct, get_next_tid(), cp->opid, cp->state, cp->data,
				      cp->len);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_caps(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_get_caps_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_get_caps(default_ct, get_next_tid(), cp->cap_id);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t list_player_app_setting_attrs(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	const struct btp_avrcp_list_player_app_setting_attrs_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_list_player_app_setting_attrs(default_ct, get_next_tid());
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t list_player_app_setting_vals(const void *cmd, uint16_t cmd_len, void *rsp,
					    uint16_t *rsp_len)
{
	const struct btp_avrcp_list_player_app_setting_vals_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_list_player_app_setting_vals(default_ct, get_next_tid(), cp->attr_id);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_curr_player_app_setting_val(const void *cmd, uint16_t cmd_len, void *rsp,
					       uint16_t *rsp_len)
{
	const struct btp_avrcp_get_curr_player_app_setting_val_cmd *cp = cmd;
	struct bt_avrcp_get_curr_player_app_setting_val_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint8_t attr_ids_len;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	attr_ids_len = cp->num_attrs * sizeof(uint8_t);
	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p) + attr_ids_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p) + attr_ids_len);
	p->num_attrs = cp->num_attrs;
	memcpy(p->attr_ids, cp->attr_ids, attr_ids_len);
	err = bt_avrcp_ct_get_curr_player_app_setting_val(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_player_app_setting_val(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_avrcp_set_player_app_setting_val_cmd *cp = cmd;
	struct bt_avrcp_set_player_app_setting_val_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint16_t attr_val_len;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	attr_val_len = cp->num_attrs * sizeof(struct bt_avrcp_app_setting_attr_val);
	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p) + attr_val_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p) + attr_val_len);
	p->num_attrs = cp->num_attrs;
	memcpy(p->attr_vals, cp->attr_vals, attr_val_len);
	err = bt_avrcp_ct_set_player_app_setting_val(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_player_app_setting_attr_text(const void *cmd, uint16_t cmd_len, void *rsp,
						uint16_t *rsp_len)
{
	const struct btp_avrcp_get_player_app_setting_attr_text_cmd *cp = cmd;
	struct bt_avrcp_get_player_app_setting_attr_text_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint8_t attr_ids_len;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	attr_ids_len = cp->num_attrs * sizeof(uint8_t);
	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p) + attr_ids_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p) + attr_ids_len);
	p->num_attrs = cp->num_attrs;
	memcpy(p->attr_ids, cp->attr_ids, attr_ids_len);
	err = bt_avrcp_ct_get_player_app_setting_attr_text(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_player_app_setting_val_text(const void *cmd, uint16_t cmd_len, void *rsp,
					       uint16_t *rsp_len)
{
	const struct btp_avrcp_get_player_app_setting_val_text_cmd *cp = cmd;
	struct bt_avrcp_get_player_app_setting_val_text_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint8_t val_ids_len;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	val_ids_len = cp->num_vals * sizeof(uint8_t);
	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p) + val_ids_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p) + val_ids_len);
	p->num_values = cp->num_vals;
	memcpy(p->value_ids, cp->val_ids, val_ids_len);
	err = bt_avrcp_ct_get_player_app_setting_val_text(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_play_status(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_get_play_status_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_get_play_status(default_ct, get_next_tid());
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_element_attrs(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_get_element_attrs_cmd *cp = cmd;
	struct bt_avrcp_get_element_attrs_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint16_t attr_ids_len;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	attr_ids_len = cp->num_attrs * sizeof(uint32_t);
	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p) + attr_ids_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p) + attr_ids_len);
	(void)memset(&p->identifier[0], 0, sizeof(p->identifier));
	p->num_attrs = cp->num_attrs;
	for (uint8_t i = 0; i < cp->num_attrs; i++) {
		p->attr_ids[i] = sys_cpu_to_be32(sys_le32_to_cpu(cp->attr_ids[i]));
	}
	err = bt_avrcp_ct_get_element_attrs(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static void register_notification_cb(struct bt_avrcp_ct *ct, uint8_t event_id,
				     struct bt_avrcp_event_data *data)
{
	struct btp_avrcp_register_notification_rsp_ev *ev;
	uint16_t data_size = 0;

	if (event_id == BT_AVRCP_EVT_UIDS_CHANGED) {
		k_work_schedule(&ct_uids_changed_event, K_MSEC(10));
	}

	if (data != NULL) {
		switch (event_id) {
		case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
			data_size = sizeof(data->play_status);
			break;
		case BT_AVRCP_EVT_TRACK_CHANGED:
			data_size = sizeof(data->identifier);
			break;
		case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
			data_size = sizeof(data->playback_pos);
			break;
		case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
			data_size = sizeof(data->battery_status);
			break;
		case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
			data_size = sizeof(data->system_status);
			break;
		case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
			data_size = sizeof(data->setting_changed.num_of_attr);
			if (data->setting_changed.attr_vals != NULL) {
				data_size += data->setting_changed.num_of_attr *
					     sizeof(struct bt_avrcp_app_setting_attr_val);
			}
			break;
		case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
			data_size = sizeof(data->addressed_player_changed);
			break;
		case BT_AVRCP_EVT_UIDS_CHANGED:
			data_size = sizeof(data->uid_counter);
			break;
		case BT_AVRCP_EVT_VOLUME_CHANGED:
			data_size = sizeof(data->absolute_volume);
			break;
		default:
			break;
		}
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + data_size, (uint8_t **)&ev);

	ev->status = BT_AVRCP_RSP_CHANGED;
	ev->event_id = event_id;
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));

	if ((data != NULL) && (data_size > 0)) {
		switch (event_id) {
		case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
			ev->data[0] = data->play_status;
			break;
		case BT_AVRCP_EVT_TRACK_CHANGED:
			sys_put_le(ev->data, &data->identifier[0], sizeof(data->identifier));
			break;
		case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
			sys_put_le32(data->playback_pos, ev->data);
			break;
		case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
			ev->data[0] = data->battery_status;
			break;
		case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
			ev->data[0] = data->system_status;
			break;
		case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
			ev->data[0] = data->setting_changed.num_of_attr;
			data_size -= sizeof(data->setting_changed.num_of_attr);
			if (data->setting_changed.attr_vals != NULL) {
				memcpy(&ev->data[1], data->setting_changed.attr_vals, data_size);
			}
			break;
		case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
			sys_put_le16(data->addressed_player_changed.player_id, &ev->data[0]);
			sys_put_le16(data->addressed_player_changed.uid_counter, &ev->data[2]);
			break;
		case BT_AVRCP_EVT_UIDS_CHANGED:
			sys_put_le16(data->uid_counter, ev->data);
			break;
		case BT_AVRCP_EVT_VOLUME_CHANGED:
			ev->data[0] = data->absolute_volume;
			break;
		default:
			break;
		}
	}

	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_REGISTER_NOTIFICATION_RSP, ev,
		     sizeof(*ev) + data_size);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static uint8_t register_notification(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_avrcp_register_notification_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_register_notification(default_ct, get_next_tid(), cp->event_id,
						sys_le32_to_cpu(cp->interval),
						register_notification_cb);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_absolute_volume(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_set_absolute_volume_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_set_absolute_volume(default_ct, get_next_tid(), cp->volume);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_addressed_player(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_set_addressed_player_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_set_addressed_player(default_ct, get_next_tid(),
					       sys_le16_to_cpu(cp->player_id));
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t play_item(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_play_item_cmd *cp = cmd;
	struct bt_avrcp_play_item_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p)) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p));
	p->scope = cp->scope;
	p->uid_counter = sys_cpu_to_be16(sys_le16_to_cpu(cp->uid_counter));
	sys_memcpy_swap(p->uid, cp->uid, sizeof(p->uid));
	err = bt_avrcp_ct_play_item(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t add_to_now_playing(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_add_to_now_playing_cmd *cp = cmd;
	struct bt_avrcp_add_to_now_playing_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p)) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p));
	p->scope = cp->scope;
	p->uid_counter = sys_cpu_to_be16(sys_le16_to_cpu(cp->uid_counter));
	sys_memcpy_swap(p->uid, cp->uid, sizeof(p->uid));
	err = bt_avrcp_ct_add_to_now_playing(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_AVRCP_BROWSING)
static uint8_t get_folder_items(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_get_folder_items_cmd *cp = cmd;
	struct bt_avrcp_get_folder_items_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint16_t attr_ids_len;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p)) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p));
	p->scope = cp->scope;
	p->start_item = sys_cpu_to_be32(sys_le32_to_cpu(cp->start_item));
	p->end_item = sys_cpu_to_be32(sys_le32_to_cpu(cp->end_item));
	p->attr_count = cp->attr_count;
	if ((cp->attr_count != 0x0U) && (cp->attr_count != 0xFF)) {
		attr_ids_len = cp->attr_count * sizeof(uint32_t);
		if (net_buf_tailroom(buf) < attr_ids_len) {
			net_buf_unref(buf);
			return BTP_STATUS_FAILED;
		}
		net_buf_add(buf, attr_ids_len);
		for (uint8_t i = 0; i < cp->attr_count; i++) {
			p->attr_ids[i] = sys_cpu_to_be32(sys_le32_to_cpu(cp->attr_ids[i]));
		}
	}

	err = bt_avrcp_ct_get_folder_items(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_total_number_of_items(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_avrcp_get_total_number_of_items_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_get_total_number_of_items(default_ct, get_next_tid(), cp->scope);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_browsed_player(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_set_browsed_player_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_ct_set_browsed_player(default_ct, get_next_tid(),
					     sys_le16_to_cpu(cp->player_id));
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t change_path(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_change_path_cmd *cp = cmd;
	struct bt_avrcp_change_path_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p)) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p));
	p->uid_counter = sys_cpu_to_be16(sys_le16_to_cpu(cp->uid_counter));
	p->direction = cp->direction;
	sys_memcpy_swap(p->folder_uid, cp->folder_uid, sizeof(p->folder_uid));
	err = bt_avrcp_ct_change_path(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_item_attrs(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_get_item_attrs_cmd *cp = cmd;
	struct bt_avrcp_get_item_attrs_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint16_t attr_ids_len;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	attr_ids_len = cp->num_attrs * sizeof(uint32_t);
	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < sizeof(*p) + attr_ids_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p) + attr_ids_len);
	p->scope = cp->scope;
	p->uid_counter = sys_cpu_to_be16(sys_le16_to_cpu(cp->uid_counter));
	p->num_attrs = cp->num_attrs;
	sys_memcpy_swap(p->uid, cp->uid, sizeof(p->uid));
	for (uint8_t i = 0; i < cp->num_attrs; i++) {
		p->attr_ids[i] = sys_cpu_to_be32(sys_le32_to_cpu(cp->attr_ids[i]));
	}
	err = bt_avrcp_ct_get_item_attrs(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t search(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_search_cmd *cp = cmd;
	struct bt_avrcp_search_cmd *p;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint16_t str_len;
	int err;

	if ((cmd_len < sizeof(*cp)) || (cmd_len != sizeof(*cp) + sys_le16_to_cpu(cp->str_len))) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	str_len = sys_le16_to_cpu(cp->str_len);
	if (net_buf_tailroom(buf) < sizeof(*p) + str_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	p = net_buf_add(buf, sizeof(*p) + str_len);
	p->charset_id = sys_cpu_to_be16(sys_le16_to_cpu(BT_AVRCP_CHARSET_UTF8));
	p->search_str_len = sys_cpu_to_be16(str_len);
	memcpy(p->search_str, cp->str, str_len);
	err = bt_avrcp_ct_search(default_ct, get_next_tid(), buf);
	if (err < 0) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_AVRCP_BROWSING */

static void control_connected(struct bt_conn *conn, struct bt_avrcp_ct *ct)
{
	struct btp_avrcp_control_connected_ev ev;

	default_conn = bt_conn_ref(conn);
	default_ct = ct;
	ct_local_tid = 0;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_CONTROL_CONNECTED, &ev, sizeof(ev));
}

static void control_disconnected(struct bt_avrcp_ct *ct)
{
	struct btp_avrcp_control_disconnected_ev ev;

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_CONTROL_DISCONNECTED, &ev, sizeof(ev));

	ct_local_tid = 0;
	default_ct = NULL;
	bt_conn_unref(default_conn);
}

static void browsing_connected(struct bt_conn *conn, struct bt_avrcp_ct *ct)
{
	struct btp_avrcp_browsing_connected_ev ev;

	k_work_init_delayable(&ct_uids_changed_event, ct_uids_changed_event_handler);

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_BROWSING_CONNECTED, &ev, sizeof(ev));
}

static void browsing_disconnected(struct bt_avrcp_ct *ct)
{
	struct btp_avrcp_browsing_disconnected_ev ev;

	k_work_cancel_delayable(&ct_uids_changed_event);

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_BROWSING_DISCONNECTED, &ev, sizeof(ev));
}

static void unit_info_rsp(struct bt_avrcp_ct *ct, uint8_t tid, struct bt_avrcp_unit_info_rsp *rsp)
{
	struct btp_avrcp_unit_info_rsp_ev ev;

	ev.unit_type = (uint8_t)rsp->unit_type;
	ev.company_id = rsp->company_id;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_UNIT_INFO_RSP, &ev, sizeof(ev));
}

static void subunit_info_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
			     struct bt_avrcp_subunit_info_rsp *rsp)
{
	struct btp_avrcp_subunit_info_rsp_ev ev;

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_SUBUNIT_INFO_RSP, &ev, sizeof(ev));
}

static void pass_through_rsp(struct bt_avrcp_ct *ct, uint8_t tid, bt_avrcp_rsp_t result,
			     const struct bt_avrcp_passthrough_rsp *rsp)
{
	struct btp_avrcp_pass_through_rsp_ev *ev;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + rsp->data_len, (uint8_t **)&ev);

	ev->status = result;
	ev->opid_state = rsp->opid_state;
	ev->len = rsp->data_len;
	memcpy(ev->data, rsp->data, rsp->data_len);
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_PASS_THROUGH_RSP, ev,
		     sizeof(*ev) + rsp->data_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void report_error(uint8_t event_id, uint8_t status)
{
	struct {
		bt_addr_t address;
		uint8_t status;
	} __packed ev;

	ev.status = status;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, event_id, &ev, sizeof(ev));
}

static void get_caps_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status, struct net_buf *buf)
{
	struct btp_avrcp_get_caps_rsp_ev *ev;
	struct bt_avrcp_get_caps_rsp *p;
	uint16_t cap_len;
	uint8_t cap_cnt;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_CAPS_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	cap_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + cap_len, (uint8_t **)&ev);

	ev->status = status;
	ev->cap_id = p->cap_id;
	ev->cap_cnt = p->cap_cnt;

	cap_cnt = 0;
	switch (p->cap_id) {
	case BT_AVRCP_CAP_COMPANY_ID:
		while (buf->len > 0) {
			if (buf->len < BT_AVRCP_COMPANY_ID_SIZE) {
				break;
			}
			sys_memcpy_swap(&ev->cap[cap_cnt],
					net_buf_pull_mem(buf, BT_AVRCP_COMPANY_ID_SIZE),
					BT_AVRCP_COMPANY_ID_SIZE);
			cap_cnt += BT_AVRCP_COMPANY_ID_SIZE;
		}
		break;
	case BT_AVRCP_CAP_EVENTS_SUPPORTED:
		while (buf->len > 0) {
			ev->cap[cap_cnt++] = net_buf_pull_u8(buf);
		}
		break;
	default:
		break;
	}

	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_CAPS_RSP, ev,
		     sizeof(*ev) + cap_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void list_player_app_setting_attrs_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
					      struct net_buf *buf)
{
	struct btp_avrcp_list_player_app_setting_attrs_rsp_ev *ev;
	struct bt_avrcp_list_player_app_setting_attrs_rsp *p;
	uint8_t attr_len;
	uint8_t num_attrs;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_LIST_PLAYER_APP_SETTING_ATTRS_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	attr_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + attr_len, (uint8_t **)&ev);

	ev->status = status;
	ev->num_attrs = p->num_attrs;

	num_attrs = 0;
	while (buf->len > 0) {
		ev->attr_ids[num_attrs++] = net_buf_pull_u8(buf);
	}
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_LIST_PLAYER_APP_SETTING_ATTRS_RSP, ev,
		     sizeof(*ev) + attr_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void list_player_app_setting_vals_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
					     struct net_buf *buf)
{
	struct btp_avrcp_list_player_app_setting_vals_rsp_ev *ev;
	struct bt_avrcp_list_player_app_setting_vals_rsp *p;
	uint8_t val_len;
	uint8_t num_vals;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_LIST_PLAYER_APP_SETTING_VALS_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	val_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + val_len, (uint8_t **)&ev);

	ev->status = status;
	ev->num_vals = p->num_values;

	num_vals = 0;
	while (buf->len > 0) {
		ev->val_ids[num_vals++] = net_buf_pull_u8(buf);
	}
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_LIST_PLAYER_APP_SETTING_VALS_RSP, ev,
		     sizeof(*ev) + val_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void get_curr_player_app_setting_val_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
						struct net_buf *buf)
{
	struct btp_avrcp_get_curr_player_app_setting_val_rsp_ev *ev;
	struct bt_avrcp_get_curr_player_app_setting_val_rsp *p;
	struct bt_avrcp_app_setting_attr_val *attr_val;
	uint8_t attr_val_len;
	uint8_t num_attrs;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_CURR_PLAYER_APP_SETTING_VAL_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	attr_val_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + attr_val_len, (uint8_t **)&ev);

	ev->status = status;
	ev->num_attrs = p->num_attrs;

	num_attrs = 0;
	attr_val = (struct bt_avrcp_app_setting_attr_val *)ev->attr_vals;
	while (buf->len > 0) {
		if (buf->len < sizeof(*attr_val)) {
			break;
		}
		memcpy(&attr_val[num_attrs++], net_buf_pull_mem(buf, sizeof(*attr_val)),
		       sizeof(*attr_val));
	}
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_CURR_PLAYER_APP_SETTING_VAL_RSP, ev,
		     sizeof(*ev) + attr_val_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void set_player_app_setting_val_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status)
{
	struct btp_avrcp_set_player_app_setting_val_rsp_ev ev;

	ev.status = status;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_SET_PLAYER_APP_SETTING_VAL_RSP, &ev,
		     sizeof(ev));
}

static void get_player_app_setting_attr_text_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
						 uint8_t status, struct net_buf *buf)
{
	struct btp_avrcp_get_player_app_setting_attr_text_rsp_ev *ev;
	struct bt_avrcp_get_player_app_setting_attr_text_rsp *p;
	struct bt_avrcp_app_setting_attr_text *src_text;
	struct bt_avrcp_app_setting_attr_text *dst_text;
	uint32_t total_len;
	uint8_t *dst_ptr;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_PLAYER_APP_SETTING_ATTR_TEXT_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	total_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + total_len, (uint8_t **)&ev);

	ev->status = status;
	ev->num_attrs = p->num_attrs;
	dst_ptr = ev->attrs;
	while (buf->len > 0) {
		if (buf->len < sizeof(*src_text)) {
			break;
		}
		src_text = (struct bt_avrcp_app_setting_attr_text *)net_buf_pull_mem(
			buf, sizeof(*src_text));
		dst_text = (struct bt_avrcp_app_setting_attr_text *)dst_ptr;

		dst_text->attr_id = src_text->attr_id;
		dst_text->charset_id = sys_cpu_to_le16(sys_be16_to_cpu(src_text->charset_id));
		dst_text->text_len = src_text->text_len;

		if (buf->len < src_text->text_len) {
			break;
		}
		memcpy(dst_text->text, net_buf_pull_mem(buf, src_text->text_len),
		       src_text->text_len);
		dst_ptr += sizeof(*dst_text) + src_text->text_len;
	}

	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_PLAYER_APP_SETTING_ATTR_TEXT_RSP, ev,
		     sizeof(*ev) + total_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void get_player_app_setting_val_text_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
						struct net_buf *buf)
{
	struct btp_avrcp_get_player_app_setting_val_text_rsp_ev *ev;
	struct bt_avrcp_get_player_app_setting_val_text_rsp *p;
	struct bt_avrcp_app_setting_val_text *src_text;
	struct bt_avrcp_app_setting_val_text *dst_text;
	uint32_t total_len;
	uint8_t *dst_ptr;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_PLAYER_APP_SETTING_VAL_TEXT_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	total_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + total_len, (uint8_t **)&ev);

	ev->status = status;
	ev->num_vals = p->num_values;
	dst_ptr = ev->vals;
	while (buf->len > 0) {
		if (buf->len < sizeof(*src_text)) {
			break;
		}
		src_text = (struct bt_avrcp_app_setting_val_text *)net_buf_pull_mem(
			buf, sizeof(*src_text));
		dst_text = (struct bt_avrcp_app_setting_val_text *)dst_ptr;

		dst_text->value_id = src_text->value_id;
		dst_text->charset_id = sys_cpu_to_le16(sys_be16_to_cpu(src_text->charset_id));
		dst_text->text_len = src_text->text_len;
		if (buf->len < src_text->text_len) {
			break;
		}
		memcpy(dst_text->text, net_buf_pull_mem(buf, src_text->text_len),
		       src_text->text_len);
		dst_ptr += sizeof(*dst_text) + src_text->text_len;
	}

	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_PLAYER_APP_SETTING_VAL_TEXT_RSP, ev,
		     sizeof(*ev) + total_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void get_play_status_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				struct net_buf *buf)
{
	struct btp_avrcp_get_play_status_rsp_ev ev;
	struct bt_avrcp_get_play_status_rsp *p;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_PLAY_STATUS_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	ev.status = status;
	ev.song_len = sys_cpu_to_le32(sys_be32_to_cpu(p->song_length));
	ev.song_pos = sys_cpu_to_le32(sys_be32_to_cpu(p->song_position));
	ev.play_status = p->play_status;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_PLAY_STATUS_RSP, &ev, sizeof(ev));
}

static void get_element_attrs_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				  struct net_buf *buf)
{
	struct btp_avrcp_get_element_attrs_rsp_ev *ev;
	struct bt_avrcp_get_element_attrs_rsp *p;
	uint32_t total_len;
	uint16_t attr_len;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_ELEMENT_ATTRS_RSP, status);
		return;
	}

	if (buf->len < sizeof(*p)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(*p));

	total_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + total_len, (uint8_t **)&ev);

	ev->status = status;
	ev->num_attrs = p->num_attrs;
	(void)decode_media_elem_attrs(buf, (struct bt_avrcp_media_attr *)ev->attrs, p->num_attrs,
				      &attr_len);

	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_ELEMENT_ATTRS_RSP, ev,
		     sizeof(*ev) + total_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void register_notification_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				      uint8_t event_id, struct bt_avrcp_event_data *data)
{
	struct btp_avrcp_register_notification_rsp_ev *ev;
	uint16_t data_size = 0;

	if (data != NULL) {
		switch (event_id) {
		case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
			data_size = sizeof(data->play_status);
			break;
		case BT_AVRCP_EVT_TRACK_CHANGED:
			data_size = sizeof(data->identifier);
			break;
		case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
			data_size = sizeof(data->playback_pos);
			break;
		case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
			data_size = sizeof(data->battery_status);
			break;
		case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
			data_size = sizeof(data->system_status);
			break;
		case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
			data_size = sizeof(data->setting_changed.num_of_attr);
			if (data->setting_changed.attr_vals != NULL) {
				data_size += data->setting_changed.num_of_attr *
					     sizeof(struct bt_avrcp_app_setting_attr_val);
			}
			break;
		case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
			data_size = sizeof(data->addressed_player_changed);
			break;
		case BT_AVRCP_EVT_UIDS_CHANGED:
			data_size = sizeof(data->uid_counter);
			break;
		case BT_AVRCP_EVT_VOLUME_CHANGED:
			data_size = sizeof(data->absolute_volume);
			break;
		default:
			break;
		}
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + data_size, (uint8_t **)&ev);

	ev->status = status;
	ev->event_id = event_id;
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));

	if ((data != NULL) && (data_size > 0)) {
		switch (event_id) {
		case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
			ev->data[0] = data->play_status;
			break;
		case BT_AVRCP_EVT_TRACK_CHANGED:
			sys_put_le(ev->data, &data->identifier[0], sizeof(data->identifier));
			break;
		case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
			sys_put_le32(data->playback_pos, ev->data);
			break;
		case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
			ev->data[0] = data->battery_status;
			break;
		case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
			ev->data[0] = data->system_status;
			break;
		case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
			ev->data[0] = data->setting_changed.num_of_attr;
			data_size -= sizeof(data->setting_changed.num_of_attr);
			if (data->setting_changed.attr_vals != NULL) {
				memcpy(&ev->data[1], data->setting_changed.attr_vals, data_size);
			}
			break;
		case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
			sys_put_le16(data->addressed_player_changed.player_id, &ev->data[0]);
			sys_put_le16(data->addressed_player_changed.uid_counter, &ev->data[2]);
			break;
		case BT_AVRCP_EVT_UIDS_CHANGED:
			sys_put_le16(data->uid_counter, ev->data);
			break;
		case BT_AVRCP_EVT_VOLUME_CHANGED:
			ev->data[0] = data->absolute_volume;
			break;
		default:
			break;
		}
	}

	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_REGISTER_NOTIFICATION_RSP, ev,
		     sizeof(*ev) + data_size);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void set_absolute_volume_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				    uint8_t volume)
{
	struct btp_avrcp_set_absolute_volume_rsp_ev ev;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_SET_ABSOLUTE_VOLUME_RSP, status);
		return;
	}

	ev.status = status;
	ev.volume = volume;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_SET_ABSOLUTE_VOLUME_RSP, &ev, sizeof(ev));
}

static void set_addressed_player_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status)
{
	struct btp_avrcp_set_addressed_player_rsp_ev ev;

	ev.status = status;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_SET_ADDRESSED_PLAYER_RSP, &ev, sizeof(ev));
}

static void play_item_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status)
{
	struct btp_avrcp_play_item_rsp_ev ev;

	ev.status = status;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_PLAY_ITEM_RSP, &ev, sizeof(ev));
}

static void add_to_now_playing_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status)
{
	struct btp_avrcp_add_to_now_playing_rsp_ev ev;

	ev.status = status;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_ADD_TO_NOW_PLAYING_RSP, &ev, sizeof(ev));
}

#if defined(CONFIG_BT_AVRCP_BROWSING)
static uint16_t decode_media_player_item(struct net_buf *buf, uint8_t *dst_ptr)
{
	struct bt_avrcp_media_player_item *src_item;
	struct bt_avrcp_media_player_item *dst_item;
	uint16_t name_len;

	if (buf->len < sizeof(*src_item)) {
		return 0;
	}

	src_item = (struct bt_avrcp_media_player_item *)net_buf_pull_mem(buf, sizeof(*src_item));
	dst_item = (struct bt_avrcp_media_player_item *)dst_ptr;

	dst_item->player_id = sys_cpu_to_le16(sys_be16_to_cpu(src_item->player_id));
	dst_item->major_type = src_item->major_type;
	dst_item->player_subtype = sys_cpu_to_le32(sys_be32_to_cpu(src_item->player_subtype));
	dst_item->play_status = src_item->play_status;
	memcpy(dst_item->feature_bitmask, src_item->feature_bitmask, 16);
	dst_item->charset_id = sys_cpu_to_le16(sys_be16_to_cpu(src_item->charset_id));
	name_len = sys_be16_to_cpu(src_item->name_len);
	dst_item->name_len = sys_cpu_to_le16(name_len);

	if (buf->len < name_len) {
		return 0;
	}

	memcpy(dst_item->name, net_buf_pull_mem(buf, name_len), name_len);
	return sizeof(*src_item) + name_len;
}

static uint16_t decode_media_elem_item(struct net_buf *buf, uint8_t *dst_ptr)
{
	struct bt_avrcp_media_element_item *src_item;
	struct bt_avrcp_media_element_item *dst_item;
	struct media_element_item_name *src_name;
	struct media_element_item_name *dst_name;
	struct media_element_item_attr *src_attrs;
	struct media_element_item_attr *dst_attrs;
	uint16_t item_len = 0;
	uint16_t name_len;
	uint16_t attr_len;
	int err;

	if (buf->len < sizeof(*src_item)) {
		return 0;
	}

	src_item = (struct bt_avrcp_media_element_item *)net_buf_pull_mem(buf, sizeof(*src_item));
	dst_item = (struct bt_avrcp_media_element_item *)dst_ptr;

	sys_memcpy_swap(dst_item->uid, src_item->uid, sizeof(dst_item->uid));
	dst_item->media_type = src_item->media_type;
	item_len += sizeof(*src_item);

	/* Decode name */
	if (buf->len < sizeof(*src_name)) {
		return 0;
	}

	src_name = (struct media_element_item_name *)net_buf_pull_mem(buf, sizeof(*src_name));
	dst_name = (struct media_element_item_name *)dst_item->data;

	dst_name->charset_id = sys_cpu_to_le16(sys_be16_to_cpu(src_name->charset_id));
	name_len = sys_be16_to_cpu(src_name->name_len);
	dst_name->name_len = sys_cpu_to_le16(name_len);

	if (buf->len < name_len) {
		return 0;
	}

	memcpy(dst_name->name, net_buf_pull_mem(buf, name_len), name_len);
	item_len += sizeof(*src_name) + name_len;

	/* Decode attributes */
	if (buf->len < sizeof(*src_attrs)) {
		return 0;
	}

	src_attrs = (struct media_element_item_attr *)net_buf_pull_mem(buf, sizeof(*src_attrs));
	dst_attrs = (struct media_element_item_attr *)(dst_name->name + name_len);

	dst_attrs->num_attrs = src_attrs->num_attrs;
	item_len += sizeof(*src_attrs);

	attr_len = 0;
	err = decode_media_elem_attrs(buf, dst_attrs->attrs, src_attrs->num_attrs, &attr_len);
	item_len += attr_len;

	if (err != 0) {
		return 0;
	}

	return item_len;
}

static uint16_t decode_folder_item(struct net_buf *buf, uint8_t *dst_ptr)
{
	struct bt_avrcp_folder_item *src_item;
	struct bt_avrcp_folder_item *dst_item;
	uint16_t name_len;

	if (buf->len < sizeof(*src_item)) {
		return 0;
	}

	src_item = (struct bt_avrcp_folder_item *)net_buf_pull_mem(buf, sizeof(*src_item));
	dst_item = (struct bt_avrcp_folder_item *)dst_ptr;

	sys_memcpy_swap(dst_item->uid, src_item->uid, sizeof(dst_item->uid));
	dst_item->folder_type = src_item->folder_type;
	dst_item->playable = src_item->playable;
	dst_item->charset_id = sys_cpu_to_le16(sys_be16_to_cpu(src_item->charset_id));
	name_len = sys_be16_to_cpu(src_item->name_len);
	dst_item->name_len = sys_cpu_to_le16(name_len);

	if (buf->len < name_len) {
		return 0;
	}

	memcpy(dst_item->name, net_buf_pull_mem(buf, name_len), name_len);
	return sizeof(*src_item) + name_len;
}

static void get_folder_items_rsp(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	struct btp_avrcp_get_folder_items_rsp_ev *ev;
	struct bt_avrcp_get_folder_items_rsp *p;
	struct bt_avrcp_item_hdr *src_hdr;
	struct bt_avrcp_item_hdr *dst_hdr;
	uint8_t *src_ptr;
	uint8_t *dst_ptr;
	uint32_t total_len;
	uint16_t item_len;

	if (buf->len < sizeof(p->status)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(p->status));

	if (p->status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_FOLDER_ITEMS_RSP, p->status);
		return;
	}

	if (buf->len < sizeof(*p) - sizeof(p->status)) {
		return;
	}

	net_buf_pull_mem(buf, sizeof(*p) - sizeof(p->status));

	total_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + total_len, (uint8_t **)&ev);

	ev->status = p->status;
	ev->uid_counter = sys_cpu_to_le16(sys_be16_to_cpu(p->uid_counter));
	ev->num_items = sys_cpu_to_le16(sys_be16_to_cpu(p->num_items));

	dst_ptr = ev->items;
	src_ptr = p->items;
	while (buf->len > 0) {
		if (buf->len < sizeof(*src_hdr)) {
			goto done;
		}

		src_hdr = (struct bt_avrcp_item_hdr *)src_ptr;
		dst_hdr = (struct bt_avrcp_item_hdr *)dst_ptr;
		dst_hdr->item_type = src_hdr->item_type;
		dst_hdr->item_len = sys_cpu_to_le16(sys_be16_to_cpu(src_hdr->item_len));

		switch (src_hdr->item_type) {
		case BT_AVRCP_ITEM_TYPE_MEDIA_PLAYER:
			item_len = decode_media_player_item(buf, dst_ptr);
			break;

		case BT_AVRCP_ITEM_TYPE_FOLDER:
			item_len = decode_folder_item(buf, dst_ptr);
			break;

		case BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
			item_len = decode_media_elem_item(buf, dst_ptr);
			break;

		default:
			item_len = sizeof(*src_hdr) + sys_be16_to_cpu(src_hdr->item_len);
			if (buf->len < item_len) {
				goto done;
			}
			net_buf_pull(buf, item_len);
			break;
		}

		if (item_len == 0) {
			goto done;
		}

		src_ptr += item_len;
		dst_ptr += item_len;
	}

done:
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_FOLDER_ITEMS_RSP, ev,
		     sizeof(*ev) + total_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void get_total_number_of_items_rsp(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	struct btp_avrcp_get_total_number_of_items_rsp_ev ev;
	struct bt_avrcp_get_total_number_of_items_rsp *p;

	if (buf->len < sizeof(p->status)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(p->status));

	if (p->status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_TOTAL_NUMBER_OF_ITEMS_RSP, p->status);
		return;
	}

	if (buf->len < sizeof(*p) - sizeof(p->status)) {
		return;
	}

	net_buf_pull_mem(buf, sizeof(*p) - sizeof(p->status));

	ev.status = p->status;
	ev.uid_counter = sys_cpu_to_le16(sys_be16_to_cpu(p->uid_counter));
	ev.num_items = sys_cpu_to_le32(sys_be32_to_cpu(p->num_items));
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_TOTAL_NUMBER_OF_ITEMS_RSP, &ev,
		     sizeof(ev));
}

static void set_browsed_player_rsp(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	struct btp_avrcp_set_browsed_player_rsp_ev *ev;
	struct bt_avrcp_set_browsed_player_rsp *p;
	struct bt_avrcp_folder_name *src_name;
	struct bt_avrcp_folder_name *dst_name;
	uint32_t total_len;
	uint16_t folder_name_len;
	uint8_t *dst_ptr;

	if (buf->len < sizeof(p->status)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(p->status));

	if (p->status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_SET_BROWSED_PLAYER_RSP, p->status);
		return;
	}

	if (buf->len < sizeof(*p) - sizeof(p->status)) {
		return;
	}

	net_buf_pull_mem(buf, sizeof(*p) - sizeof(p->status));

	total_len = buf->len;
	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + total_len, (uint8_t **)&ev);

	ev->status = p->status;
	ev->uid_counter = sys_cpu_to_le16(sys_be16_to_cpu(p->uid_counter));
	ev->num_items = sys_cpu_to_le32(sys_be32_to_cpu(p->num_items));
	ev->charset_id = sys_cpu_to_le16(sys_be16_to_cpu(p->charset_id));
	ev->folder_depth = p->folder_depth;

	dst_ptr = ev->folder_names;
	for (uint8_t i = 0; i < p->folder_depth; i++) {
		if (buf->len < sizeof(*src_name)) {
			break;
		}
		src_name = (struct bt_avrcp_folder_name *)net_buf_pull_mem(buf, sizeof(*src_name));
		dst_name = (struct bt_avrcp_folder_name *)dst_ptr;

		folder_name_len = sys_be16_to_cpu(src_name->folder_name_len);
		dst_name->folder_name_len = sys_cpu_to_le16(folder_name_len);

		if (buf->len < folder_name_len) {
			break;
		}
		memcpy(dst_name->folder_name, net_buf_pull_mem(buf, folder_name_len),
		       folder_name_len);

		dst_ptr += sizeof(*dst_name) + folder_name_len;
	}

	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_SET_BROWSED_PLAYER_RSP, ev,
		     sizeof(*ev) + total_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void change_path_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status, uint32_t num_items)
{
	struct btp_avrcp_change_path_rsp_ev ev;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_CHANGE_PATH_RSP, status);
		return;
	}

	ev.status = status;
	ev.num_items = num_items;
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_CHANGE_PATH_RSP, &ev, sizeof(ev));
}

static void get_item_attrs_rsp(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	struct btp_avrcp_get_item_attrs_rsp_ev *ev;
	struct bt_avrcp_get_item_attrs_rsp *p;
	uint16_t total_len;
	uint16_t attr_len;

	if (buf->len < sizeof(p->status)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(p->status));

	if (p->status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_GET_ITEM_ATTRS_RSP, p->status);
		return;
	}

	if (buf->len < sizeof(p->num_attrs)) {
		return;
	}

	net_buf_pull_mem(buf, sizeof(p->num_attrs));
	total_len = buf->len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + total_len, (uint8_t **)&ev);

	ev->status = p->status;
	ev->num_attrs = p->num_attrs;
	(void)decode_media_elem_attrs(buf, (struct bt_avrcp_media_attr *)ev->attrs, p->num_attrs,
				      &attr_len);

	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_ITEM_ATTRS_RSP, ev,
		     sizeof(*ev) + total_len - buf->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void search_rsp(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	struct btp_avrcp_search_rsp_ev ev;
	struct bt_avrcp_search_rsp *p;

	if (buf->len < sizeof(p->status)) {
		return;
	}

	p = net_buf_pull_mem(buf, sizeof(p->status));

	if (p->status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		report_error(BTP_AVRCP_EV_SEARCH_RSP, p->status);
		return;
	}

	if (buf->len < sizeof(*p) - sizeof(p->status)) {
		return;
	}

	net_buf_pull_mem(buf, sizeof(*p) - sizeof(p->status));

	ev.status = p->status;
	ev.uid_counter = sys_cpu_to_le16(sys_be16_to_cpu(p->uid_counter));
	ev.num_items = sys_cpu_to_le32(sys_be32_to_cpu(p->num_items));
	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_SEARCH_RSP, &ev, sizeof(ev));
}
#endif /* CONFIG_BT_AVRCP_BROWSING */

static struct bt_avrcp_ct_cb ct_cb = {
	.connected = control_connected,
	.disconnected = control_disconnected,
	.browsing_connected = browsing_connected,
	.browsing_disconnected = browsing_disconnected,
	.unit_info_rsp = unit_info_rsp,
	.subunit_info_rsp = subunit_info_rsp,
	.passthrough_rsp = pass_through_rsp,
	.get_caps = get_caps_rsp,
	.list_player_app_setting_attrs = list_player_app_setting_attrs_rsp,
	.list_player_app_setting_vals = list_player_app_setting_vals_rsp,
	.get_curr_player_app_setting_val = get_curr_player_app_setting_val_rsp,
	.set_player_app_setting_val = set_player_app_setting_val_rsp,
	.get_player_app_setting_attr_text = get_player_app_setting_attr_text_rsp,
	.get_player_app_setting_val_text = get_player_app_setting_val_text_rsp,
	.get_element_attrs = get_element_attrs_rsp,
	.get_play_status = get_play_status_rsp,
	.notification = register_notification_rsp,
	.set_absolute_volume = set_absolute_volume_rsp,
	.set_addressed_player = set_addressed_player_rsp,
	.play_item = play_item_rsp,
	.add_to_now_playing = add_to_now_playing_rsp,
#if defined(CONFIG_BT_AVRCP_BROWSING)
	.get_folder_items = get_folder_items_rsp,
	.get_total_number_of_items = get_total_number_of_items_rsp,
	.set_browsed_player = set_browsed_player_rsp,
	.change_path = change_path_rsp,
	.get_item_attrs = get_item_attrs_rsp,
	.search = search_rsp,
#endif /* CONFIG_BT_AVRCP_BROWSING */
};
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

#if defined(CONFIG_BT_AVRCP_TARGET)
static struct vfs_node *vfs_node_alloc(void)
{
	ARRAY_FOR_EACH(tg_node_pool, i) {
		if (tg_node_pool[i].item == NULL) {
			return &tg_node_pool[i];
		}
	}
	return NULL;
}

static void vfs_node_free(struct vfs_node *node)
{
	if (node != NULL) {
		memset(node, 0, sizeof(struct vfs_node));
	}
}

static struct vfs_node *find_item_by_uid(sys_slist_t *list, uint64_t uid)
{
	struct vfs_node *child;

	SYS_SLIST_FOR_EACH_CONTAINER(list, child, node) {
		if (uid == child->item->uid) {
			return child;
		}
	}

	return NULL;
}

static void invalidate_list(sys_slist_t *list)
{
	struct vfs_node *iter, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, iter, tmp, node) {
		sys_slist_remove(list, NULL, &iter->node);
		vfs_node_free(iter);
	}
}

static void add_to_list(sys_slist_t *list, struct vfs_node *node)
{
	struct vfs_node *iter;

	iter = find_item_by_uid(list, node->item->uid);
	if (iter != NULL) {
		return; /* Item already exists in the list */
	}

	iter = vfs_node_alloc();
	if (iter != NULL) {
		memcpy(iter, node, sizeof(struct vfs_node));
		sys_slist_append(list, &iter->node);
	}
}

static void remove_from_list(sys_slist_t *list, struct vfs_node *node)
{
	struct vfs_node *iter, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, iter, tmp, node) {
		if (iter->item->uid == node->item->uid) {
			sys_slist_find_and_remove(list, &iter->node);
			vfs_node_free(iter);
			break;
		}
	}
}

static void set_playing_item(void)
{
	struct vfs_node *playing_vfs_node;
	sys_snode_t *playing_node;

	/* Peek the first node from the now playing list as the playing item */
	playing_node = sys_slist_peek_head(&tg_now_playing_list);
	if (playing_node != NULL) {
		playing_vfs_node = CONTAINER_OF(playing_node, struct vfs_node, node);
		tg_playing_item = (struct media_item *)(void *)playing_vfs_node->item;
	} else {
		tg_playing_item = NULL;
	}
}

static void vfs_init(void)
{
	/*
	 * /
	 * ├── songlists/
	 * │   ├── song 1
	 * │   └── song 2
	 * ├── no_cover_art_folder/
	 * │   └── song 3
	 * └── empty_folder/
	 */
	ARRAY_FOR_EACH(tg_vfs_node, i) {
		sys_slist_init(&tg_vfs_node[i].child);
	}
	tg_vfs_node[0].item = (void *)&tg_folder_items[0];     /* root */
	tg_vfs_node[1].item = (void *)&tg_folder_items[1];     /* songlists */
	tg_vfs_node[2].item = (void *)&tg_folder_items[2];     /* no_cover_art_folder */
	tg_vfs_node[3].item = (void *)&tg_folder_items[3];     /* empty_folder */
	tg_vfs_node[4].item = (void *)&tg_media_elem_items[0]; /* song 1 */
	tg_vfs_node[5].item = (void *)&tg_media_elem_items[1]; /* song 2 */
	tg_vfs_node[6].item = (void *)&tg_media_elem_items[2]; /* song 3 */
	sys_slist_append(&tg_vfs_node[0].child, &tg_vfs_node[1].node);
	sys_slist_append(&tg_vfs_node[0].child, &tg_vfs_node[2].node);
	sys_slist_append(&tg_vfs_node[0].child, &tg_vfs_node[3].node);
	sys_slist_append(&tg_vfs_node[1].child, &tg_vfs_node[4].node);
	sys_slist_append(&tg_vfs_node[1].child, &tg_vfs_node[5].node);
	sys_slist_append(&tg_vfs_node[2].child, &tg_vfs_node[6].node);

	/* Add item to now playing list */
	sys_slist_init(&tg_now_playing_list);
	add_to_list(&tg_now_playing_list, &tg_vfs_node[4]);
	add_to_list(&tg_now_playing_list, &tg_vfs_node[5]);

	/* Set playing item */
	set_playing_item();

	sys_slist_init(&tg_search_list);
}

static void vfs_search(struct vfs_node *node, const char *str, uint32_t *out_num_items,
		       uint32_t depth)
{
	struct vfs_node *child;
	uint32_t num_items = 0;
	uint32_t child_items = 0;

	if (depth >= AVRCP_SEARCH_MAX_DEPTH) {
		*out_num_items = 0;
		return;
	}

	if (node->item->item_type == BT_AVRCP_ITEM_TYPE_FOLDER) {
		SYS_SLIST_FOR_EACH_CONTAINER(&node->child, child, node) {
			if (child->item->item_type == BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT) {
				if (strstr(child->item->name, str) != NULL) {
					add_to_list(&tg_search_list, child);
					num_items++;
				}
				continue;
			}

			vfs_search(child, str, &child_items, depth + 1);
			num_items += child_items;
		}
	}

	*out_num_items = num_items;
}

static struct vfs_node *vfs_find_node(const char *path)
{
	char path_copy[AVRCP_VFS_PATH_MAX_LEN];
	struct vfs_node *curr_node;
	char *save_ptr;
	char *token;

	if ((path == NULL) || (path[0] != '/')) {
		return NULL;
	}

	curr_node = &tg_vfs_node[0]; /* root */
	strncpy(path_copy, path, AVRCP_VFS_PATH_MAX_LEN);
	token = strtok_r(path_copy, "/", &save_ptr);

	while (token && curr_node && curr_node->item->item_type == BT_AVRCP_ITEM_TYPE_FOLDER) {
		struct vfs_node *child;
		bool found = false;

		SYS_SLIST_FOR_EACH_CONTAINER(&curr_node->child, child, node) {
			if (strcmp(child->item->name, token) == 0) {
				curr_node = child;
				found = true;
				break;
			}
		}

		if (!found) {
			return NULL;
		}

		token = strtok_r(NULL, "/", &save_ptr);
	}

	return curr_node;
}

static void dirname(char *path)
{
	char *str;
	size_t len;

	if (path == NULL) {
		return;
	}

	/* Remove trailing slashes */
	len = strlen(path);
	while (len > 1 && path[len - 1] == '/') {
		path[--len] = '\0';
	}

	str = strrchr(path, '/');

	if (str != NULL) {
		/* if root path, keep "/", or truncate path. */
		if (str == path) {
			str[1] = '\0';
		} else {
			str[0] = '\0';
		}
	}
}

static void join_path(char *path, const char *dir_name)
{
	if ((path == NULL) || (dir_name == NULL)) {
		return;
	}

	if (strlen(path) + strlen(dir_name) + strlen("/") >= AVRCP_VFS_PATH_MAX_LEN) {
		return;
	}
	strcat(path, dir_name);
	strcat(path, "/");
}

static void tg_register_event(uint8_t event_id, uint8_t tid)
{
	if (event_id <= 13U) {
		tg_reg_events[event_id - 1] = tid | 0x80U;
	}
}

static void tg_unregister_event(uint8_t event_id)
{
	if (event_id <= 13U) {
		tg_reg_events[event_id - 1] &= ~0x80U;
	}
}

static bool tg_check_registered_event(uint8_t event_id)
{
	if ((event_id <= 13U) && ((tg_reg_events[event_id - 1] & 0x80U) != 0U)) {
		return true;
	}
	return false;
}

static uint8_t tg_get_registered_event(uint8_t event_id)
{
	uint8_t tid = 0xFF;

	if (event_id <= 13U) {
		tid = tg_reg_events[event_id - 1] & ~0x80U;
	}
	return tid;
}

static void tg_addr_player_changed_event_handler(struct k_work *work)
{
	int err;
	uint8_t *event_id;

	event_id = NULL;
	ARRAY_FOR_EACH(tg_addr_player_changed_events, i) {
		if (tg_check_registered_event(tg_addr_player_changed_events[i])) {
			event_id = &tg_addr_player_changed_events[i];
			break;
		}
	}

	if (event_id == NULL) {
		return;
	}

	err = bt_avrcp_tg_notification(default_tg, tg_get_registered_event(*event_id),
				       BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, *event_id, NULL);
	if (err < 0) {
		k_work_schedule(&tg_send_addr_player_changed_event, K_MSEC(10));
	} else {
		tg_unregister_event(*event_id);
		ARRAY_FOR_EACH(tg_addr_player_changed_events, i) {
			if (tg_check_registered_event(tg_addr_player_changed_events[i])) {
				k_work_schedule(&tg_send_addr_player_changed_event, K_MSEC(0));
				break;
			}
		}
	}
}

static int encode_media_elem_attrs(struct net_buf *buf, const struct media_item *item,
				   const struct media_attr_list *attrs_list, uint8_t *out_num_attrs)
{
	int err = 0;
	uint8_t _num_attrs = 0;
	struct bt_avrcp_media_attr *attr;

	/* Add media attributes if specific attributes are requested */
	for (uint8_t i = 0; i < attrs_list->attr_count; i++) {
		for (uint8_t j = 0; j < item->num_attrs; j++) {
			if (sys_be32_to_cpu(attrs_list->attr_ids[i]) == item->attr[j].attr_id) {
				if (net_buf_tailroom(buf) <
				    sizeof(*attr) + item->attr[j].attr_len) {
					err = -ENOMEM;
					goto done;
				}
				attr = net_buf_add(buf, sizeof(*attr));
				attr->attr_id = sys_cpu_to_be32(item->attr[j].attr_id);
				attr->charset_id = sys_cpu_to_be16(item->attr[j].charset_id);
				attr->attr_len = sys_cpu_to_be16(item->attr[j].attr_len);
				net_buf_add_mem(buf, item->attr[j].attr_val,
						item->attr[j].attr_len);
				_num_attrs++;
			}
		}
	}

	if (attrs_list->attr_count != 0U) {
		goto done;
	}

	/* Add all media attributes if no specific attributes are requested */
	for (uint8_t i = 0; i < item->num_attrs; i++) {
		if (net_buf_tailroom(buf) < sizeof(*attr) + item->attr[i].attr_len) {
			err = -ENOMEM;
			goto done;
		}
		attr = net_buf_add(buf, sizeof(*attr));
		attr->attr_id = sys_cpu_to_be32(item->attr[i].attr_id);
		attr->charset_id = sys_cpu_to_be16(item->attr[i].charset_id);
		attr->attr_len = sys_cpu_to_be16(item->attr[i].attr_len);
		net_buf_add_mem(buf, item->attr[i].attr_val, item->attr[i].attr_len);
		_num_attrs++;
	}

done:
	*out_num_attrs = _num_attrs;

	return err;
}

static uint8_t tg_register_notification(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_avrcp_tg_register_notification_cmd *cp = cmd;
	struct bt_conn *conn;
	struct bt_avrcp_event_data event_data;
	struct bt_avrcp_app_setting_attr_val attr_vals;
	struct player_attr *player_attrs;
	sys_snode_t *removed_node;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (!tg_check_registered_event(cp->event_id)) {
		return BTP_STATUS_FAILED;
	}

	switch (cp->event_id) {
	case BT_AVRCP_EVT_TRACK_CHANGED:
		struct vfs_node *iter;

		/* Change the currently playing track */
		SYS_SLIST_FOR_EACH_CONTAINER(&tg_now_playing_list, iter, node) {
			if ((void *)tg_playing_item != (void *)iter->item) {
				tg_playing_item = (struct media_item *)(void *)iter->item;
				break;
			}
		}

		if (tg_playing_item != NULL) {
			sys_memcpy_swap(&event_data.identifier, &tg_playing_item->hdr.uid,
					sizeof(event_data.identifier));
		} else {
			memset(&event_data.identifier, 0xFF, sizeof(event_data.identifier));
		}
		break;

	case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
		player_attrs = tg_player_items[tg_cur_player_idx].attr;

		if (tg_player_items[tg_cur_player_idx].num_attrs > 0U) {
			player_attrs[0].attr_val++;
			if (player_attrs[0].attr_val > player_attrs[0].attr_val_max) {
				player_attrs[0].attr_val = player_attrs[0].attr_val_min;
			}
		}
		attr_vals.attr_id = player_attrs[0].attr_id;
		attr_vals.value_id = player_attrs[0].attr_val;
		event_data.setting_changed.attr_vals = &attr_vals;
		event_data.setting_changed.num_of_attr =
			tg_player_items[tg_cur_player_idx].num_attrs;
		break;

	case BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED:
		removed_node = sys_slist_peek_head(&tg_now_playing_list);

		/* Remove first media item in tg_now_playing_list */
		if (removed_node != NULL) {
			struct vfs_node *removed_vfs_node =
				CONTAINER_OF(removed_node, struct vfs_node, node);
			remove_from_list(&tg_now_playing_list, removed_vfs_node);
		}
		break;

	case BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED:
		break;

	case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
		tg_cur_player_idx = (tg_cur_player_idx + 1) % ARRAY_SIZE(tg_player_items);
		event_data.addressed_player_changed.player_id =
			tg_player_items[tg_cur_player_idx].player_id;
		event_data.addressed_player_changed.uid_counter = tg_uid_counter;
		ARRAY_FOR_EACH(tg_addr_player_changed_events, i) {
			if (tg_check_registered_event(tg_addr_player_changed_events[i])) {
				k_work_schedule(&tg_send_addr_player_changed_event, K_MSEC(10));
				break;
			}
		}
		break;

	case BT_AVRCP_EVT_UIDS_CHANGED:
		struct vfs_node *curr_node = vfs_find_node("/songlists/");

		/* Remove first media item in "/songlists/" */
		if ((curr_node != NULL) &&
		    (curr_node->item->item_type == BT_AVRCP_ITEM_TYPE_FOLDER)) {
			removed_node = sys_slist_peek_head(&curr_node->child);

			if (removed_node != NULL) {
				struct vfs_node *removed_vfs_node =
					CONTAINER_OF(removed_node, struct vfs_node, node);

				tg_uid_counter++;
				sys_slist_find_and_remove(&curr_node->child, removed_node);
				remove_from_list(&tg_now_playing_list, removed_vfs_node);
			}
		}
		event_data.uid_counter = tg_uid_counter;
		break;

	case BT_AVRCP_EVT_VOLUME_CHANGED:
		tg_volume++;
		tg_volume &= BT_AVRCP_MAX_ABSOLUTE_VOLUME;
		event_data.absolute_volume = tg_volume;
		break;

	default:
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_tg_notification(default_tg, tg_get_registered_event(cp->event_id),
				       BT_AVRCP_STATUS_OPERATION_COMPLETED, cp->event_id,
				       &event_data);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	tg_unregister_event(cp->event_id);

	return BTP_STATUS_SUCCESS;
}

static uint8_t tg_control_playback(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_tg_control_playback_cmd *cp = cmd;
	struct vfs_node *curr_node;
	struct vfs_node *playing_node;
	sys_snode_t *node;

	if (cp->action == 0) {
		/* play */
		if (cp->cover_art != 0U) {
			curr_node = vfs_find_node("/songlists/");
		} else {
			curr_node = vfs_find_node("/no_cover_art_folder/");
		}

		if (curr_node == NULL) {
			return BTP_STATUS_FAILED;
		}

		node = sys_slist_peek_head(&curr_node->child);
		if (node == NULL) {
			return BTP_STATUS_FAILED;
		}

		playing_node = CONTAINER_OF(node, struct vfs_node, node);

		add_to_list(&tg_now_playing_list, playing_node);
		tg_playing_item = (struct media_item *)(void *)playing_node->item;

		if (cp->long_metadata != 0U) {
			tg_long_metadata = true;
		}
	} else if (cp->action == 1) {
		/* stop */
		tg_playing_item = NULL;
	} else {
		/* no action */
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tg_change_path(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_tg_change_path_cmd *cp = cmd;
	char folder_name[AVRCP_VFS_PATH_MAX_LEN];

	if (cp->direction == BT_AVRCP_CHANGE_PATH_PARENT) {
		dirname(tg_cur_vfs_path);
	} else {
		if (cp->folder_name_len >= AVRCP_VFS_PATH_MAX_LEN) {
			return BTP_STATUS_FAILED;
		}
		folder_name[cp->folder_name_len] = '\0';
		memcpy(&folder_name[0], cp->folder_name, cp->folder_name_len);
		join_path(tg_cur_vfs_path, &folder_name[0]);
	}

	return BTP_STATUS_SUCCESS;
}

static void tg_connected(struct bt_conn *conn, struct bt_avrcp_tg *tg)
{
	struct btp_avrcp_control_connected_ev ev;

	tg_uid = 0x01;
	tg_uid_counter = 0x01;

	/* Set UID */
	ARRAY_FOR_EACH(tg_folder_items, i) {
		tg_folder_items[i].hdr.uid = tg_uid;
		tg_uid++;
	}

	ARRAY_FOR_EACH(tg_media_elem_items, i) {
		tg_media_elem_items[i].hdr.uid = tg_uid;
		tg_uid++;
	}

	/* Initialize virtual filesystem */
	vfs_init();

	default_conn = bt_conn_ref(conn);
	default_tg = tg;
	k_work_init_delayable(&tg_send_addr_player_changed_event,
			      tg_addr_player_changed_event_handler);

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_CONTROL_CONNECTED, &ev, sizeof(ev));
}

static void tg_disconnected(struct bt_avrcp_tg *tg)
{
	struct btp_avrcp_control_disconnected_ev ev;

	k_work_cancel_delayable(&tg_send_addr_player_changed_event);

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_CONTROL_DISCONNECTED, &ev, sizeof(ev));

	bt_conn_unref(default_conn);
	default_tg = NULL;
}

static void tg_browsing_connected(struct bt_conn *conn, struct bt_avrcp_tg *tg)
{
	struct btp_avrcp_browsing_connected_ev ev;

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_BROWSING_CONNECTED, &ev, sizeof(ev));
}

static void tg_browsing_disconnected(struct bt_avrcp_tg *tg)
{
	struct btp_avrcp_browsing_disconnected_ev ev;

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_BROWSING_DISCONNECTED, &ev, sizeof(ev));
}

static void unit_info_req(struct bt_avrcp_tg *tg, uint8_t tid)
{
	struct bt_avrcp_unit_info_rsp rsp;

	rsp.unit_type = BT_AVRCP_SUBUNIT_TYPE_PANEL;
	rsp.company_id = BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG;
	(void)bt_avrcp_tg_send_unit_info_rsp(tg, tid, &rsp);
}

static void subunit_info_req(struct bt_avrcp_tg *tg, uint8_t tid)
{
	(void)bt_avrcp_tg_send_subunit_info_rsp(tg, tid);
}

static void passthrough_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_passthrough_cmd *cmd;
	struct bt_avrcp_passthrough_rsp *rsp;
	int err;

	if (buf->len < sizeof(*cmd)) {
		return;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	if (buf->len < cmd->data_len) {
		return;
	}

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		return;
	}

	if (net_buf_tailroom(buf) < sizeof(*rsp) + cmd->data_len) {
		net_buf_unref(buf);
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp) + cmd->data_len);
	rsp->opid_state = cmd->opid_state;
	rsp->data_len = cmd->data_len;
	memcpy(rsp->data, cmd->data, cmd->data_len);
	err = bt_avrcp_tg_send_passthrough_rsp(tg, tid, BT_AVRCP_RSP_ACCEPTED, buf);
	if (err < 0) {
		net_buf_unref(buf);
	}
}

static void get_caps_req(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t cap_id)
{
	struct bt_avrcp_get_caps_rsp *rsp;
	struct net_buf *buf;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	int err;

	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	if (net_buf_tailroom(buf) < sizeof(*rsp)) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));

	rsp->cap_id = cap_id;
	switch (cap_id) {
	case BT_AVRCP_CAP_COMPANY_ID:
		if (net_buf_tailroom(buf) < BT_AVRCP_COMPANY_ID_SIZE) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			goto done;
		}
		rsp->cap_cnt = 1U;
		net_buf_add(buf, BT_AVRCP_COMPANY_ID_SIZE);
		sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, rsp->cap);
		break;

	case BT_AVRCP_CAP_EVENTS_SUPPORTED:
		if (net_buf_tailroom(buf) < 13U) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			goto done;
		}
		rsp->cap_cnt = 13U;
		net_buf_add(buf, 13U);
		rsp->cap[0] = BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED;
		rsp->cap[1] = BT_AVRCP_EVT_TRACK_CHANGED;
		rsp->cap[2] = BT_AVRCP_EVT_TRACK_REACHED_END;
		rsp->cap[3] = BT_AVRCP_EVT_TRACK_REACHED_START;
		rsp->cap[4] = BT_AVRCP_EVT_PLAYBACK_POS_CHANGED;
		rsp->cap[5] = BT_AVRCP_EVT_BATT_STATUS_CHANGED;
		rsp->cap[6] = BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED;
		rsp->cap[7] = BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED;
		rsp->cap[8] = BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED;
		rsp->cap[9] = BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED;
		rsp->cap[10] = BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED;
		rsp->cap[11] = BT_AVRCP_EVT_UIDS_CHANGED;
		rsp->cap[12] = BT_AVRCP_EVT_VOLUME_CHANGED;
		break;

	default:
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		break;
	}

done:
	err = bt_avrcp_tg_get_caps(tg, tid, status, buf);
	if ((err < 0) && (buf != NULL)) {
		net_buf_unref(buf);
	}
}

static void list_player_app_setting_attrs_req(struct bt_avrcp_tg *tg, uint8_t tid)
{
	struct bt_avrcp_list_player_app_setting_attrs_rsp *rsp;
	struct net_buf *buf;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint8_t attr_ids_len;
	int err;

	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	attr_ids_len = tg_player_items[tg_cur_player_idx].num_attrs * sizeof(uint8_t);
	if (net_buf_tailroom(buf) < sizeof(*rsp) + attr_ids_len) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(buf, sizeof(*rsp) + attr_ids_len);
	rsp->num_attrs = attr_ids_len;
	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		rsp->attr_ids[i] = tg_player_items[tg_cur_player_idx].attr[i].attr_id;
	}

done:
	err = bt_avrcp_tg_list_player_app_setting_attrs(tg, tid, status, buf);
	if ((err < 0) && (buf != NULL)) {
		net_buf_unref(buf);
	}
}

static void list_player_app_setting_vals_req(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t attr_id)
{
	struct bt_avrcp_list_player_app_setting_vals_rsp *rsp;
	struct player_attr *attr;
	struct net_buf *buf = NULL;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint8_t val_ids_len;
	int err;

	attr = NULL;
	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		if (tg_player_items[tg_cur_player_idx].attr[i].attr_id == attr_id) {
			attr = &tg_player_items[tg_cur_player_idx].attr[i];
			break;
		}
	}

	if (attr == NULL) {
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto done;
	}

	val_ids_len = (attr->attr_val_max - attr->attr_val_min + 1) * sizeof(uint8_t);

	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	if (net_buf_tailroom(buf) < sizeof(*rsp) + val_ids_len) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(buf, sizeof(*rsp) + val_ids_len);
	rsp->num_values = val_ids_len / sizeof(uint8_t);
	for (uint8_t i = attr->attr_val_min; i <= attr->attr_val_max; i++) {
		rsp->values[i - attr->attr_val_min] = i;
	}

done:
	err = bt_avrcp_tg_list_player_app_setting_vals(tg, tid, status, buf);
	if ((err < 0) && (buf != NULL)) {
		net_buf_unref(buf);
	}
}

static void get_curr_player_app_setting_val_req(struct bt_avrcp_tg *tg, uint8_t tid,
						struct net_buf *buf)
{
	struct bt_avrcp_get_curr_player_app_setting_val_cmd *cmd;
	struct bt_avrcp_get_curr_player_app_setting_val_rsp *rsp;
	struct bt_avrcp_app_setting_attr_val *attr_val;
	struct net_buf *tx_buf = NULL;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint8_t attr_ids_len;
	uint32_t total_len;
	int err;

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	attr_ids_len = cmd->num_attrs * sizeof(uint8_t);
	if (buf->len < attr_ids_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	total_len = 0;
	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		for (uint8_t j = 0; j < cmd->num_attrs; j++) {
			if (tg_player_items[tg_cur_player_idx].attr[i].attr_id ==
			    cmd->attr_ids[j]) {
				total_len += sizeof(*attr_val);
			}
		}
	}

	if (total_len == 0) {
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto done;
	}

	tx_buf = bt_avrcp_create_vendor_pdu(NULL);
	if (tx_buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}
	if (net_buf_tailroom(tx_buf) < sizeof(*rsp) + total_len) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(tx_buf, sizeof(*rsp));
	rsp->num_attrs = total_len / sizeof(*attr_val);

	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		for (uint8_t j = 0; j < cmd->num_attrs; j++) {
			if (tg_player_items[tg_cur_player_idx].attr[i].attr_id ==
			    cmd->attr_ids[j]) {
				attr_val = net_buf_add(tx_buf, sizeof(*attr_val));
				attr_val->attr_id =
					tg_player_items[tg_cur_player_idx].attr[i].attr_id;
				attr_val->value_id =
					tg_player_items[tg_cur_player_idx].attr[i].attr_val;
			}
		}
	}

done:
	err = bt_avrcp_tg_get_curr_player_app_setting_val(tg, tid, status, tx_buf);
	if ((err < 0) && (tx_buf != NULL)) {
		net_buf_unref(tx_buf);
	}
}

static void set_player_app_setting_val_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_set_player_app_setting_val_cmd *cmd;
	struct bt_avrcp_app_setting_attr_val *attr_val;
	struct player_attr *attr;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint8_t attr_val_len;
	bool updated;

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	attr_val_len = cmd->num_attrs * sizeof(*attr_val);
	if (buf->len < attr_val_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	updated = false;
	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		attr = &tg_player_items[tg_cur_player_idx].attr[i];
		for (uint8_t j = 0; j < cmd->num_attrs; j++) {
			if ((cmd->attr_vals[j].attr_id == attr->attr_id) &&
			    (cmd->attr_vals[j].value_id >= attr->attr_val_min) &&
			    (cmd->attr_vals[j].value_id <= attr->attr_val_max)) {
				attr->attr_val = cmd->attr_vals[j].value_id;
				updated = true;
			}
		}
	}

	if (!updated) {
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

done:
	(void)bt_avrcp_tg_set_player_app_setting_val(tg, tid, status);
}

static void get_player_app_setting_attr_text_req(struct bt_avrcp_tg *tg, uint8_t tid,
						 struct net_buf *buf)
{
	struct bt_avrcp_get_player_app_setting_attr_text_cmd *cmd;
	struct bt_avrcp_get_player_app_setting_attr_text_rsp *rsp;
	struct bt_avrcp_app_setting_attr_text *attr_text;
	struct player_attr *attr;
	struct net_buf *tx_buf = NULL;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint8_t attr_ids_len;
	uint32_t total_len;
	uint8_t num_attrs;
	int err;

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	attr_ids_len = cmd->num_attrs * sizeof(uint8_t);
	if (buf->len < attr_ids_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	total_len = 0;
	num_attrs = 0;
	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		attr = &tg_player_items[tg_cur_player_idx].attr[i];
		for (uint8_t j = 0; j < cmd->num_attrs; j++) {
			if (attr->attr_id == cmd->attr_ids[j]) {
				total_len += sizeof(*attr_text) + strlen(attr->attr_text);
				num_attrs++;
			}
		}
	}

	if (total_len == 0) {
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto done;
	}

	tx_buf = bt_avrcp_create_vendor_pdu(NULL);
	if (tx_buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	if (net_buf_tailroom(tx_buf) < sizeof(*rsp) + total_len) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(tx_buf, sizeof(*rsp));
	rsp->num_attrs = num_attrs;

	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		attr = &tg_player_items[tg_cur_player_idx].attr[i];
		for (uint8_t j = 0; j < cmd->num_attrs; j++) {
			if (attr->attr_id == cmd->attr_ids[j]) {
				attr_text = net_buf_add(tx_buf, sizeof(*attr_text));
				attr_text->attr_id = attr->attr_id;
				attr_text->charset_id = sys_cpu_to_be16(attr->charset_id);
				attr_text->text_len = strlen(attr->attr_text);
				net_buf_add_mem(tx_buf, attr->attr_text, attr_text->text_len);
			}
		}
	}

done:
	err = bt_avrcp_tg_get_player_app_setting_attr_text(tg, tid, status, tx_buf);
	if ((err < 0) && (tx_buf != NULL)) {
		net_buf_unref(tx_buf);
	}
}

static void get_player_app_setting_val_text_req(struct bt_avrcp_tg *tg, uint8_t tid,
						struct net_buf *buf)
{
	struct bt_avrcp_get_player_app_setting_val_text_cmd *cmd;
	struct bt_avrcp_get_player_app_setting_val_text_rsp *rsp;
	struct bt_avrcp_app_setting_val_text *attr_text;
	struct player_attr *attr;
	struct net_buf *tx_buf = NULL;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint8_t val_ids_len;
	uint32_t total_len;
	uint8_t num_vals;
	int err;

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	val_ids_len = cmd->num_values * sizeof(uint8_t);
	if (buf->len < val_ids_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	attr = NULL;
	for (uint8_t i = 0; i < tg_player_items[tg_cur_player_idx].num_attrs; i++) {
		if (tg_player_items[tg_cur_player_idx].attr[i].attr_id == cmd->attr_id) {
			attr = &tg_player_items[tg_cur_player_idx].attr[i];
			break;
		}
	}

	if (attr == NULL) {
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto done;
	}

	total_len = 0;
	num_vals = 0;
	for (uint8_t i = attr->attr_val_min; i <= attr->attr_val_max; i++) {
		for (uint8_t j = 0; j < cmd->num_values; j++) {
			if (cmd->value_ids[j] == i) {
				num_vals++;
				total_len += sizeof(*attr_text) + strlen(attr->val_text[i]);
			}
		}
	}

	if (total_len == 0) {
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto done;
	}

	tx_buf = bt_avrcp_create_vendor_pdu(NULL);
	if (tx_buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	if (net_buf_tailroom(tx_buf) < sizeof(*rsp) + total_len) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(tx_buf, sizeof(*rsp));
	rsp->num_values = num_vals;

	for (uint8_t j = 0; j < cmd->num_values; j++) {
		for (uint8_t i = attr->attr_val_min; i <= attr->attr_val_max; i++) {
			if (cmd->value_ids[j] == i) {
				attr_text = net_buf_add(tx_buf, sizeof(*attr_text));
				attr_text->value_id = i;
				attr_text->charset_id = sys_cpu_to_be16(attr->charset_id);
				attr_text->text_len = strlen(attr->val_text[i]);
				net_buf_add_mem(tx_buf, attr->val_text[i],
						strlen(attr->val_text[i]));
			}
		}
	}

done:
	err = bt_avrcp_tg_get_player_app_setting_val_text(tg, tid, status, tx_buf);
	if ((err < 0) && (tx_buf != NULL)) {
		net_buf_unref(tx_buf);
	}
}

static void get_element_attrs_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_get_element_attrs_cmd *cmd;
	struct bt_avrcp_get_element_attrs_rsp *rsp;
	struct net_buf *tx_buf = NULL;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint16_t attr_ids_len;
	int err;

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	attr_ids_len = cmd->num_attrs * sizeof(uint32_t);
	if (buf->len < attr_ids_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	if (tg_playing_item == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	if (tg_long_metadata) {
		for (uint8_t i = 0; i < tg_playing_item->num_attrs; i++) {
			if (tg_playing_item->attr[i].attr_id == BT_AVRCP_MEDIA_ATTR_ID_TITLE) {
				tg_playing_item->attr[i].attr_len = strlen(tg_long_title);
				tg_playing_item->attr[i].attr_val = tg_long_title;
				break;
			}
		}
	}

	tx_buf = bt_avrcp_create_vendor_pdu(&avrcp_tx_pool);
	if (tx_buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	if (net_buf_tailroom(tx_buf) < sizeof(*rsp)) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(tx_buf, sizeof(*rsp));

	(void)encode_media_elem_attrs(tx_buf, tg_playing_item,
				      (struct media_attr_list *)(void *)&cmd->num_attrs,
				      &rsp->num_attrs);

done:
	err = bt_avrcp_tg_get_element_attrs(tg, tid, status, tx_buf);
	if ((err < 0) && (tx_buf != NULL)) {
		net_buf_unref(tx_buf);
	}
}

static void get_play_status_req(struct bt_avrcp_tg *tg, uint8_t tid)
{
	struct bt_avrcp_get_play_status_rsp *rsp;
	struct net_buf *buf = NULL;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	int err;

	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	if (net_buf_tailroom(buf) < sizeof(*rsp)) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->song_length = sys_cpu_to_be32(UINT32_MAX);
	rsp->song_position = sys_cpu_to_be32(UINT32_MAX);
	rsp->play_status = 0;

done:
	err = bt_avrcp_tg_get_play_status(tg, tid, status, buf);
	if ((err < 0) && (buf != NULL)) {
		net_buf_unref(buf);
	}
}

static void register_notification_req(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t event_id,
				      uint32_t interval)
{
	struct bt_avrcp_event_data event_data;
	struct bt_avrcp_app_setting_attr_val attr_vals;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;

	tg_register_event(event_id, tid);

	switch (event_id) {
	case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
		event_data.play_status = BT_AVRCP_PLAYBACK_STATUS_PLAYING;
		break;

	case BT_AVRCP_EVT_TRACK_CHANGED:
		if (tg_playing_item != NULL) {
			sys_put_be64(tg_playing_item->hdr.uid, &event_data.identifier[0]);
		} else {
			memset(&event_data.identifier, 0xFF, sizeof(event_data.identifier));
		}
		break;

	case BT_AVRCP_EVT_TRACK_REACHED_END:
		break;
	case BT_AVRCP_EVT_TRACK_REACHED_START:
		break;

	case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
		event_data.playback_pos = UINT32_MAX;
		break;

	case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
		event_data.battery_status = BT_AVRCP_BATTERY_STATUS_NORMAL;
		break;

	case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
		event_data.system_status = BT_AVRCP_SYSTEM_STATUS_POWER_ON;
		break;

	case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
		event_data.setting_changed.num_of_attr = 1;
		event_data.setting_changed.attr_vals = &attr_vals;
		attr_vals.attr_id = BT_AVRCP_PLAYER_ATTR_EQUALIZER;
		attr_vals.value_id = BT_AVRCP_EQUALIZER_OFF;
		break;

	case BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED:
		break;
	case BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED:
		break;

	case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
		event_data.addressed_player_changed.player_id =
			tg_player_items[tg_cur_player_idx].player_id;
		event_data.addressed_player_changed.uid_counter = tg_uid_counter;
		break;

	case BT_AVRCP_EVT_UIDS_CHANGED:
		event_data.uid_counter = tg_uid_counter;
		break;

	case BT_AVRCP_EVT_VOLUME_CHANGED:
		event_data.absolute_volume = 0;
		break;

	default:
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		break;
	}

	(void)bt_avrcp_tg_notification(tg, tid, status, event_id, &event_data);
}

static void set_absolute_volume_req(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t absolute_volume)
{
	tg_volume = absolute_volume & BT_AVRCP_MAX_ABSOLUTE_VOLUME;
	(void)bt_avrcp_tg_absolute_volume(tg, tid, BT_AVRCP_STATUS_OPERATION_COMPLETED, tg_volume);
}

static void set_addressed_player_req(struct bt_avrcp_tg *tg, uint8_t tid, uint16_t player_id)
{
	struct player_item *item;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;

	item = NULL;
	ARRAY_FOR_EACH(tg_player_items, i) {
		if (player_id == tg_player_items[i].player_id) {
			item = &tg_player_items[i];
			tg_cur_player_idx = i;
			break;
		}
	}

	if (item == NULL) {
		status = BT_AVRCP_STATUS_INVALID_PLAYER_ID;
	}

	(void)bt_avrcp_tg_set_addressed_player(tg, tid, status);
}

static void play_item_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_play_item_cmd *cmd;
	struct vfs_node *curr_node;
	struct vfs_node *child;
	struct vfs_node *found;
	sys_slist_t *target_list;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint64_t uid;

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));

	if (sys_be16_to_cpu(cmd->uid_counter) != tg_uid_counter) {
		status = BT_AVRCP_STATUS_UID_CHANGED;
		goto done;
	}

	switch (cmd->scope) {
	case BT_AVRCP_SCOPE_VFS:
		curr_node = vfs_find_node(tg_cur_vfs_path);
		if ((curr_node == NULL) ||
		    (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			goto done;
		}
		target_list = &curr_node->child;
		break;

	case BT_AVRCP_SCOPE_SEARCH:
		target_list = &tg_search_list;
		break;

	case BT_AVRCP_SCOPE_NOW_PLAYING:
		target_list = &tg_now_playing_list;
		break;

	default:
		status = BT_AVRCP_STATUS_INVALID_SCOPE;
		goto done;
	}

	if (target_list == NULL) {
		status = BT_AVRCP_STATUS_DOES_NOT_EXIST;
		goto done;
	}

	uid = sys_get_be64(&cmd->uid[0]);
	found = find_item_by_uid(target_list, uid);
	if (found == NULL) {
		status = BT_AVRCP_STATUS_DOES_NOT_EXIST;
		goto done;
	}

	if (found->item->item_type == BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT) {
		add_to_list(&tg_now_playing_list, found);
		tg_playing_item = (struct media_item *)(void *)found->item;
	} else {
		struct folder_item *item = (struct folder_item *)(void *)found->item;

		if (item->is_playable == 0U) {
			status = BT_AVRCP_STATUS_FOLDER_ITEM_IS_NOT_PLAYABLE;
			goto done;
		}
		invalidate_list(&tg_now_playing_list);
		SYS_SLIST_FOR_EACH_CONTAINER(&found->child, child, node) {
			if (child->item->item_type == BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT) {
				add_to_list(&tg_now_playing_list, child);
			}
		}
		set_playing_item();
	}

done:
	(void)bt_avrcp_tg_play_item(tg, tid, status);
}

static void add_to_now_playing_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_add_to_now_playing_cmd *cmd;
	struct vfs_node *curr_node;
	struct vfs_node *child;
	struct vfs_node *found;
	sys_slist_t *target_list;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint64_t uid;

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));

	if (sys_be16_to_cpu(cmd->uid_counter) != tg_uid_counter) {
		status = BT_AVRCP_STATUS_UID_CHANGED;
		goto done;
	}

	switch (cmd->scope) {
	case BT_AVRCP_SCOPE_VFS:
		curr_node = vfs_find_node(tg_cur_vfs_path);
		if ((curr_node == NULL) ||
		    (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			goto done;
		}
		target_list = &curr_node->child;
		break;

	case BT_AVRCP_SCOPE_SEARCH:
		target_list = &tg_search_list;
		break;

	default:
		status = BT_AVRCP_STATUS_INVALID_SCOPE;
		goto done;
	}

	if (target_list == NULL) {
		status = BT_AVRCP_STATUS_DOES_NOT_EXIST;
		goto done;
	}

	uid = sys_get_be64(&cmd->uid[0]);
	found = find_item_by_uid(target_list, uid);
	if (found == NULL) {
		status = BT_AVRCP_STATUS_DOES_NOT_EXIST;
		goto done;
	}

	if (found->item->item_type == BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT) {
		add_to_list(&tg_now_playing_list, found);
	} else {
		struct folder_item *item = (struct folder_item *)(void *)found->item;

		if (item->is_playable == 0U) {
			status = BT_AVRCP_STATUS_FOLDER_ITEM_IS_NOT_PLAYABLE;
			goto done;
		}
		invalidate_list(&tg_now_playing_list);
		SYS_SLIST_FOR_EACH_CONTAINER(&found->child, child, node) {
			if (child->item->item_type == BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT) {
				add_to_list(&tg_now_playing_list, child);
			}
		}
		set_playing_item();
	}

done:
	(void)bt_avrcp_tg_add_to_now_playing(tg, tid, status);
}

#if defined(CONFIG_BT_AVRCP_BROWSING)
static uint8_t encode_media_player_item(struct net_buf *buf,
					 const struct bt_avrcp_get_folder_items_cmd *cmd,
					 uint16_t *num_items)
{
	uint32_t start_item = sys_be32_to_cpu(cmd->start_item);
	uint32_t end_item = sys_be32_to_cpu(cmd->end_item);

	if (start_item >= ARRAY_SIZE(tg_player_items)) {
		return BT_AVRCP_STATUS_RANGE_OUT_OF_BOUNDS;
	}

	if (end_item >= ARRAY_SIZE(tg_player_items)) {
		end_item = ARRAY_SIZE(tg_player_items) - 1;
	}

	for (uint32_t i = start_item; i <= end_item; i++) {
		struct bt_avrcp_media_player_item *player_item;

		if (net_buf_tailroom(buf) < sizeof(*player_item) + tg_player_items[i].name_len) {
			return BT_AVRCP_STATUS_INTERNAL_ERROR;
		}

		(*num_items)++;
		player_item = net_buf_add(buf, sizeof(*player_item) + tg_player_items[i].name_len);

		player_item->hdr.item_type = tg_player_items[i].item_type;
		player_item->hdr.item_len = sys_cpu_to_be16(tg_player_items[i].item_len);
		player_item->player_id = sys_cpu_to_be16(tg_player_items[i].player_id);
		player_item->major_type = tg_player_items[i].major_type;
		player_item->player_subtype = sys_cpu_to_be32(tg_player_items[i].sub_type);
		player_item->play_status = tg_player_items[i].play_status;
		memcpy(&player_item->feature_bitmask[0], &tg_player_items[i].feature_bitmask[0],
		       sizeof(player_item->feature_bitmask));
		player_item->charset_id = sys_cpu_to_be16(tg_player_items[i].charset_id);
		player_item->name_len = sys_cpu_to_be16(tg_player_items[i].name_len);
		memcpy(&player_item->name[0], &tg_player_items[i].name[0],
		       tg_player_items[i].name_len);
	}

	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int encode_media_elem_item(struct net_buf *buf, const struct media_item *item,
				  const struct media_attr_list *attrs_list)
{
	struct bt_avrcp_media_element_item *media_item;
	struct media_element_item_name *media_item_name;
	struct media_element_item_attr *media_item_attr;
	struct bt_avrcp_media_attr *attr;
	uint16_t item_len;
	uint16_t attr_len;
	uint16_t buf_len;
	bool err;

	attr_len = 0;

	/*
	 * TG should return as many items as can be returned with full attribute data.
	 * If the response containing only one item still exceeds the size which can be handled
	 * by the CT, then the TG should return less attributes than are requested.
	 */
	if (buf->len == sizeof(struct bt_avrcp_get_folder_items_rsp)) {
		goto encode;
	}

	/* Calculate attribute length if specific attributes are requested */
	for (uint8_t i = 0; i < attrs_list->attr_count; i++) {
		for (uint8_t j = 0; j < item->num_attrs; j++) {
			if (sys_be32_to_cpu(attrs_list->attr_ids[i]) == item->attr[j].attr_id) {
				attr_len += sizeof(*attr) + item->attr[j].attr_len;
			}
		}
	}

	if (attrs_list->attr_count != 0) {
		goto encode;
	}

	/* Calculate attribute length if no specific attributes are requested */
	for (uint8_t i = 0; i < item->num_attrs; i++) {
		attr_len += sizeof(*attr) + item->attr[i].attr_len;
	}

encode:
	item_len = sizeof(*media_item);
	item_len += sizeof(*media_item_name) + item->hdr.name_len;
	item_len += sizeof(*media_item_attr);

	if (net_buf_tailroom(buf) < item_len + attr_len) {
		return -ENOMEM;
	}

	/* Add media element item */
	media_item = net_buf_add(buf, sizeof(*media_item));
	sys_put_be64(item->hdr.uid, &media_item->uid[0]);
	media_item->hdr.item_type = item->hdr.item_type;
	media_item->media_type = item->media_type;

	/* Add displayable name */
	media_item_name = net_buf_add(buf, sizeof(*media_item_name) + item->hdr.name_len);
	media_item_name->charset_id = sys_cpu_to_be16(item->hdr.charset_id);
	media_item_name->name_len = sys_cpu_to_be16(item->hdr.name_len);
	memcpy(media_item_name->name, item->hdr.name, item->hdr.name_len);

	/* Add number of attributes */
	media_item_attr = net_buf_add(buf, sizeof(*media_item_attr));
	media_item_attr->num_attrs = 0;

	/* Add attributes */
	buf_len = buf->len;
	err = encode_media_elem_attrs(buf, item, attrs_list, &media_item_attr->num_attrs);
	attr_len = buf->len - buf_len;

	media_item->hdr.item_len = sys_cpu_to_be16(item_len + attr_len - sizeof(media_item->hdr));

	return err;
}

static int encode_folder_item(struct net_buf *buf, const struct folder_item *item)
{
	struct bt_avrcp_folder_item *folder_item;
	uint16_t item_len;

	item_len = sizeof(*folder_item) + item->hdr.name_len;

	if (net_buf_tailroom(buf) < item_len) {
		return -ENOMEM;
	}

	/* Add folder item */
	folder_item = net_buf_add(buf, item_len);
	sys_put_be64(item->hdr.uid, &folder_item->uid[0]);
	folder_item->hdr.item_type = item->hdr.item_type;
	folder_item->hdr.item_len = sys_cpu_to_be16(item_len);
	folder_item->folder_type = item->folder_type;
	folder_item->playable = item->is_playable;
	folder_item->charset_id = sys_cpu_to_be16(item->hdr.charset_id);
	folder_item->name_len = sys_cpu_to_be16(item->hdr.name_len);
	strncpy(folder_item->name, item->hdr.name, item->hdr.name_len);

	return 0;
}

static uint8_t encode_vfs_items(struct net_buf *buf, sys_slist_t *item_list, uint8_t item_mask,
				const struct bt_avrcp_get_folder_items_cmd *cmd,
				uint16_t *num_items)
{
	uint32_t start_item = sys_be32_to_cpu(cmd->start_item);
	uint32_t end_item = sys_be32_to_cpu(cmd->end_item);
	uint32_t count = 0;
	uint16_t items_count = 0;
	struct vfs_node *iter;
	int err;

	SYS_SLIST_FOR_EACH_CONTAINER(item_list, iter, node) {
		if (count > end_item) {
			break;
		}

		count++;

		if (count <= start_item) {
			continue;
		}

		if (((item_mask | (1U << BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT)) != 0) &&
		    (iter->item->item_type == BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT)) {
			struct media_item *item = (struct media_item *)(void *)iter->item;
			const struct media_attr_list *attrs_list =
				(const struct media_attr_list *)(const void *)&cmd->attr_count;

			err = encode_media_elem_item(buf, item, attrs_list);
		} else if (((item_mask | (1U << BT_AVRCP_ITEM_TYPE_FOLDER)) != 0) &&
			   (iter->item->item_type == BT_AVRCP_ITEM_TYPE_FOLDER)) {
			struct folder_item *item = (struct folder_item *)(void *)iter->item;

			err = encode_folder_item(buf, item);
		} else {
			continue;
		}

		if (err < 0) {
			break; /* No more space in buffer */
		}

		items_count++; /* Successfully added item */
	}

	*num_items = items_count;

	if (count <= start_item) {
		return BT_AVRCP_STATUS_RANGE_OUT_OF_BOUNDS;
	}

	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static void get_folder_items_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_get_folder_items_cmd *cmd;
	struct bt_avrcp_get_folder_items_rsp *rsp;
	struct net_buf *tx_buf;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint16_t attr_ids_len;
	uint32_t start_item;
	uint32_t end_item;
	uint16_t num_items = 0;
	int err;

	tx_buf = bt_avrcp_create_pdu(NULL);
	if (tx_buf == NULL) {
		return;
	}

	if (net_buf_tailroom(tx_buf) < sizeof(*rsp)) {
		net_buf_unref(tx_buf);
		return;
	}

	rsp = net_buf_add(tx_buf, sizeof(*rsp));

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	attr_ids_len = cmd->attr_count * sizeof(uint32_t);
	if (buf->len < attr_ids_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	start_item = sys_be32_to_cpu(cmd->start_item);
	end_item = sys_be32_to_cpu(cmd->end_item);
	if (start_item > end_item) {
		status = BT_AVRCP_STATUS_RANGE_OUT_OF_BOUNDS;
		goto done;
	}

	switch (cmd->scope) {
	case BT_AVRCP_SCOPE_MEDIA_PLAYER_LIST:
		status = encode_media_player_item(tx_buf, cmd, &num_items);
		break;

	case BT_AVRCP_SCOPE_VFS: {
		struct vfs_node *curr_node = vfs_find_node(tg_cur_vfs_path);

		if ((curr_node == NULL) ||
		    (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			goto done;
		}

		status = encode_vfs_items(tx_buf, &curr_node->child,
					  (1U << BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT) |
						  (1U << BT_AVRCP_ITEM_TYPE_FOLDER),
					  cmd, &num_items);
		break;
	}

	case BT_AVRCP_SCOPE_SEARCH:
		if (sys_slist_is_empty(&tg_search_list)) {
			status = BT_AVRCP_STATUS_NO_VALID_SEARCH_RESULTS;
			break;
		}

		status = encode_vfs_items(tx_buf, &tg_search_list,
					  1 << BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT, cmd, &num_items);
		break;

	case BT_AVRCP_SCOPE_NOW_PLAYING:
		status = encode_vfs_items(tx_buf, &tg_now_playing_list,
					  1 << BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT, cmd, &num_items);
		break;

	default:
		status = BT_AVRCP_STATUS_INVALID_SCOPE;
		break;
	}

done:
	rsp->status = status;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		net_buf_remove_mem(tx_buf, sizeof(rsp->uid_counter) + sizeof(rsp->num_items));
	} else {
		rsp->uid_counter = sys_cpu_to_be16(tg_uid_counter);
		rsp->num_items = sys_cpu_to_be16(num_items);
	}

	err = bt_avrcp_tg_get_folder_items(tg, tid, tx_buf);
	if (err < 0) {
		net_buf_unref(tx_buf);
	}
}

static void get_total_number_of_items_req(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t scope)
{
	struct bt_avrcp_get_total_number_of_items_rsp *rsp;
	struct net_buf *buf;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint32_t num_items;
	int err;

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		return;
	}

	if (net_buf_tailroom(buf) < sizeof(*rsp)) {
		net_buf_unref(buf);
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));

	switch (scope) {
	case BT_AVRCP_SCOPE_MEDIA_PLAYER_LIST:
		num_items = ARRAY_SIZE(tg_player_items);
		break;

	case BT_AVRCP_SCOPE_VFS:
		struct vfs_node *curr_node = vfs_find_node(tg_cur_vfs_path);

		if ((curr_node == NULL) ||
		    (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			break;
		}

		num_items = sys_slist_len(&curr_node->child);
		break;

	case BT_AVRCP_SCOPE_SEARCH:
		num_items = sys_slist_len(&tg_search_list);
		break;

	case BT_AVRCP_SCOPE_NOW_PLAYING:
		num_items = sys_slist_len(&tg_now_playing_list);
		break;

	default:
		status = BT_AVRCP_STATUS_INVALID_SCOPE;
		break;
	}

	rsp->status = status;
	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		net_buf_remove_mem(buf, sizeof(rsp->uid_counter) + sizeof(rsp->num_items));
	} else {
		rsp->uid_counter = sys_cpu_to_be16(tg_uid_counter);
		rsp->num_items = sys_cpu_to_be32(num_items);
	}

	err = bt_avrcp_tg_get_total_number_of_items(tg, tid, buf);
	if (err < 0) {
		net_buf_unref(buf);
	}
}

static void set_browsed_player_req(struct bt_avrcp_tg *tg, uint8_t tid, uint16_t player_id)
{
	struct bt_avrcp_set_browsed_player_rsp *rsp;
	struct player_item *item;
	struct net_buf *buf;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint32_t total_len;
	uint32_t num_items;
	char path_copy[AVRCP_VFS_PATH_MAX_LEN];
	char *save_ptr;
	char *token;
	struct vfs_node *curr_node;
	int err;

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		return;
	}

	if (net_buf_tailroom(buf) < sizeof(*rsp)) {
		net_buf_unref(buf);
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));

	item = NULL;
	ARRAY_FOR_EACH(tg_player_items, i) {
		if (player_id == tg_player_items[i].player_id) {
			item = &tg_player_items[i];
		}
	}

	if (item == NULL) {
		status = BT_AVRCP_STATUS_INVALID_PLAYER_ID;
		goto done;
	}

	curr_node = vfs_find_node(tg_cur_vfs_path);
	if ((curr_node == NULL) || (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	num_items = sys_slist_len(&curr_node->child);

	strncpy(path_copy, tg_cur_vfs_path, AVRCP_VFS_PATH_MAX_LEN);
	token = strtok_r(path_copy, "/", &save_ptr);
	total_len = 0;
	while (token) {
		total_len += sizeof(struct bt_avrcp_folder_name) + strlen(token);
		token = strtok_r(NULL, "/", &save_ptr);
	}

	if (net_buf_tailroom(buf) < total_len) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	net_buf_add(buf, total_len);

	rsp->folder_depth = 0;
	strncpy(path_copy, tg_cur_vfs_path, AVRCP_VFS_PATH_MAX_LEN);
	token = strtok_r(path_copy, "/", &save_ptr);
	while (token) {
		rsp->folder_names[rsp->folder_depth].folder_name_len =
			sys_cpu_to_be16(strlen(token));
		strcpy(rsp->folder_names[rsp->folder_depth].folder_name, token);
		rsp->folder_depth++;
		token = strtok_r(NULL, "/", &save_ptr);
	}

done:
	rsp->status = status;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		net_buf_remove_mem(buf, sizeof(*rsp) - sizeof(rsp->status));
	} else {
		rsp->uid_counter = sys_cpu_to_be16(tg_uid_counter);
		rsp->charset_id = sys_cpu_to_be16(BT_AVRCP_CHARSET_UTF8);
		rsp->num_items = sys_cpu_to_be32(num_items);
	}

	err = bt_avrcp_tg_set_browsed_player(tg, tid, buf);
	if (err < 0) {
		net_buf_unref(buf);
	}
}

static void change_path_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_change_path_cmd *cmd;
	struct vfs_node *curr_node;
	struct vfs_node *found;
	uint32_t num_items = 0;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint64_t uid;

	if (buf->len < sizeof(*cmd)) {
		return;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));

	if (cmd->direction == BT_AVRCP_CHANGE_PATH_PARENT) {
		if (strcmp(tg_cur_vfs_path, "/") == 0) {
			status = BT_AVRCP_STATUS_INVALID_DIRECTION;
			goto done;
		}
		dirname(tg_cur_vfs_path);
	} else if (cmd->direction == BT_AVRCP_CHANGE_PATH_CHILD) {
		curr_node = vfs_find_node(tg_cur_vfs_path);
		if ((curr_node == NULL) ||
		    (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			goto done;
		}

		uid = sys_get_be64(&cmd->folder_uid[0]);
		found = find_item_by_uid(&curr_node->child, uid);
		if ((found == NULL) || (found->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
			status = BT_AVRCP_STATUS_DOES_NOT_EXIST;
			goto done;
		}
		join_path(tg_cur_vfs_path, (const char *)found->item->name);
	} else {
		status = BT_AVRCP_STATUS_INVALID_DIRECTION;
		goto done;
	}

	curr_node = vfs_find_node(tg_cur_vfs_path);
	if ((curr_node == NULL) || (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	num_items = sys_slist_len(&curr_node->child);

done:
	(void)bt_avrcp_tg_change_path(tg, tid, status, num_items);
}

static void get_item_attrs_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_get_item_attrs_cmd *cmd;
	struct bt_avrcp_get_item_attrs_rsp *rsp;
	struct vfs_node *curr_node;
	struct vfs_node *found;
	struct net_buf *tx_buf;
	sys_slist_t *target_list;
	uint16_t attr_ids_len;
	uint8_t num_attrs;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint64_t uid;
	int err;

	tx_buf = bt_avrcp_create_pdu(NULL);
	if (tx_buf == NULL) {
		return;
	}

	if (net_buf_tailroom(tx_buf) < sizeof(*rsp)) {
		net_buf_unref(tx_buf);
		return;
	}

	rsp = net_buf_add(tx_buf, sizeof(*rsp));

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	attr_ids_len = cmd->num_attrs * sizeof(uint32_t);
	if (buf->len < attr_ids_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	if (sys_be16_to_cpu(cmd->uid_counter) != tg_uid_counter) {
		status = BT_AVRCP_STATUS_UID_CHANGED;
		goto done;
	}

	switch (cmd->scope) {
	case BT_AVRCP_SCOPE_VFS: {
		curr_node = vfs_find_node(tg_cur_vfs_path);
		if ((curr_node == NULL) ||
		    (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
			status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			goto done;
		}
		target_list = &curr_node->child;
		break;
	}

	case BT_AVRCP_SCOPE_SEARCH:
		target_list = &tg_search_list;
		break;

	case BT_AVRCP_SCOPE_NOW_PLAYING:
		target_list = &tg_now_playing_list;
		break;

	default:
		status = BT_AVRCP_STATUS_INVALID_SCOPE;
		goto done;
	}

	if (target_list == NULL) {
		status = BT_AVRCP_STATUS_DOES_NOT_EXIST;
		goto done;
	}

	num_attrs = 0;
	uid = sys_get_be64(&cmd->uid[0]);
	found = find_item_by_uid(target_list, uid);

	if (found == NULL) {
		status = BT_AVRCP_STATUS_DOES_NOT_EXIST;
		goto done;
	}

	if (found->item->item_type == BT_AVRCP_ITEM_TYPE_MEDIA_ELEMENT) {
		struct media_item *item = (struct media_item *)(void *)found->item;
		struct media_attr_list *attrs_list =
			(struct media_attr_list *)(void *)&cmd->num_attrs;

		(void)encode_media_elem_attrs(tx_buf, item, attrs_list, &num_attrs);
	}

done:
	rsp->status = status;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		net_buf_remove_mem(tx_buf, sizeof(rsp->num_attrs));
	} else {
		rsp->num_attrs = num_attrs;
	}

	err = bt_avrcp_tg_get_item_attrs(tg, tid, tx_buf);
	if (err < 0) {
		net_buf_unref(tx_buf);
	}
}

static void search_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_search_cmd *cmd;
	struct bt_avrcp_search_rsp *rsp;
	struct vfs_node *curr_node;
	struct net_buf *tx_buf;
	uint8_t status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint16_t charset_id;
	uint16_t str_len;
	char str[AVRCP_SEARCH_STRING_MAX_LEN];
	uint32_t num_items = 0;
	int err;

	tx_buf = bt_avrcp_create_pdu(NULL);
	if (tx_buf == NULL) {
		return;
	}

	if (net_buf_tailroom(tx_buf) < sizeof(*rsp)) {
		net_buf_unref(tx_buf);
		return;
	}

	rsp = net_buf_add(tx_buf, sizeof(*rsp));

	if (buf->len < sizeof(*cmd)) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	charset_id = sys_be16_to_cpu(cmd->charset_id);
	if (charset_id != BT_AVRCP_CHARSET_UTF8) {
		status = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto done;
	}

	str_len = sys_be16_to_cpu(cmd->search_str_len);
	if (buf->len < str_len) {
		status = BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		goto done;
	}

	if (str_len >= AVRCP_SEARCH_STRING_MAX_LEN) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}
	memcpy(&str[0], cmd->search_str, str_len);
	str[str_len] = '\0';

	curr_node = vfs_find_node(tg_cur_vfs_path);
	if ((curr_node == NULL) || (curr_node->item->item_type != BT_AVRCP_ITEM_TYPE_FOLDER)) {
		status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto done;
	}

	invalidate_list(&tg_search_list);
	vfs_search(curr_node, &str[0], &num_items, 0);

done:
	rsp->status = status;

	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		net_buf_remove_mem(tx_buf, sizeof(rsp->uid_counter) + sizeof(rsp->num_items));
	} else {
		rsp->uid_counter = sys_cpu_to_be16(tg_uid_counter);
		rsp->num_items = sys_cpu_to_be32(num_items);
	}

	err = bt_avrcp_tg_search(tg, tid, tx_buf);
	if (err < 0) {
		net_buf_unref(tx_buf);
	}
}
#endif /* CONFIG_BT_AVRCP_BROWSING */

static struct bt_avrcp_tg_cb tg_cb = {
	.connected = tg_connected,
	.disconnected = tg_disconnected,
	.browsing_connected = tg_browsing_connected,
	.browsing_disconnected = tg_browsing_disconnected,
	.unit_info_req = unit_info_req,
	.subunit_info_req = subunit_info_req,
	.passthrough_req = passthrough_req,
	.get_caps = get_caps_req,
	.list_player_app_setting_attrs = list_player_app_setting_attrs_req,
	.list_player_app_setting_vals = list_player_app_setting_vals_req,
	.get_curr_player_app_setting_val = get_curr_player_app_setting_val_req,
	.set_player_app_setting_val = set_player_app_setting_val_req,
	.get_player_app_setting_attr_text = get_player_app_setting_attr_text_req,
	.get_player_app_setting_val_text = get_player_app_setting_val_text_req,
	.get_element_attrs = get_element_attrs_req,
	.get_play_status = get_play_status_req,
	.register_notification = register_notification_req,
	.set_absolute_volume = set_absolute_volume_req,
	.set_addressed_player = set_addressed_player_req,
	.play_item = play_item_req,
	.add_to_now_playing = add_to_now_playing_req,
#if defined(CONFIG_BT_AVRCP_BROWSING)
	.get_folder_items = get_folder_items_req,
	.get_total_number_of_items = get_total_number_of_items_req,
	.set_browsed_player = set_browsed_player_req,
	.change_path = change_path_req,
	.get_item_attrs = get_item_attrs_req,
	.search = search_req,
#endif /* CONFIG_BT_AVRCP_BROWSING */
};
#endif /* CONFIG_BT_AVRCP_TARGET */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
static uint8_t sdp_avrcp_user(struct bt_conn *conn, struct bt_sdp_client_result *result,
			      const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t psm;

	if (result == NULL || result->resp_buf == NULL) {
		goto done;
	}

	err = bt_sdp_get_addl_proto_param(result->resp_buf, BT_SDP_PROTO_L2CAP, 0x01, &psm);
	if (err < 0) {
		goto done;
	}

	(void)bt_avrcp_cover_art_ct_l2cap_connect(default_ct, &default_ca_ct, psm);
	return BT_SDP_DISCOVER_UUID_STOP;

done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params discov_avrcp_tg = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_AV_REMOTE_TARGET_SVCLASS),
	.func = sdp_avrcp_user,
	.pool = &sdp_client_pool,
};

static uint8_t ca_ct_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_ca_ct_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	} else if (default_ca_ct != NULL) {
		err = bt_avrcp_cover_art_ct_connect(default_ca_ct);
		if (err != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		bt_sdp_discover(conn, &discov_avrcp_tg);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ca_ct_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_ca_ct_disconnect_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_avrcp_cover_art_ct_disconnect(default_ca_ct);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
static uint8_t ca_ct_get_image_props(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_avrcp_ct_get_image_props_cmd *cp = cmd;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint8_t image_handle[16];
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(default_ca_ct, &ca_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_obex_add_header_conn_id(buf, default_ca_ct->client._conn_id);
	if (err != 0) {
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES),
				      BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES);
	if (err != 0) {
		goto failed;
	}

	/* Convert string to unicode */
	memset(&image_handle[0], '\x00', sizeof(image_handle));
	for (int i = 0; i < sizeof(cp->image_handle); i++) {
		image_handle[i * 2 + 1] = cp->image_handle[i];
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(image_handle), &image_handle[0]);
	if (err != 0) {
		goto failed;
	}

	err = bt_avrcp_cover_art_ct_get_image_properties(default_ca_ct, true, buf);
	if (err == 0) {
		return BTP_STATUS_SUCCESS;
	}

failed:
	net_buf_unref(buf);
	return BTP_STATUS_FAILED;
}

static void ca_ct_get_image_props_rsp(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				      struct net_buf *buf)
{
	struct btp_avrcp_get_image_props_rsp_ev *ev;
	uint16_t body_len = 0;
	const uint8_t *body = NULL;
	int err;

	if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		err = bt_obex_get_header_body(buf, &body_len, &body);
	} else {
		err = bt_obex_get_header_end_body(buf, &body_len, &body);
	}

	if (err != 0) {
		body_len = 0;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + body_len, (uint8_t **)&ev);

	ev->rsp_code = rsp_code;
	ev->body_len = sys_cpu_to_le16(body_len);
	if ((body_len > 0U) && (body != NULL)) {
		memcpy(ev->body, body, body_len);
	}
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_IMAGE_PROPS_RSP, ev,
		     sizeof(*ev) + body_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		buf = bt_avrcp_cover_art_ct_create_pdu(default_ca_ct, &ca_tx_pool);
		if (buf == NULL) {
			return;
		}

		err = bt_avrcp_cover_art_ct_get_image_properties(default_ca_ct, true, buf);
		if (err != 0) {
			net_buf_unref(buf);
		}
	}
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
static uint8_t ca_ct_get_image(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_avrcp_ct_get_image_cmd *cp = cmd;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint8_t image_handle[16];
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(default_ca_ct, &ca_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_obex_add_header_conn_id(buf, default_ca_ct->client._conn_id);
	if (err != 0) {
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE),
				      BT_BIP_HDR_TYPE_GET_IMAGE);
	if (err != 0) {
		goto failed;
	}

	/* Convert string to unicode */
	memset(&image_handle[0], '\x00', sizeof(image_handle));
	for (int i = 0; i < sizeof(cp->image_handle); i++) {
		image_handle[i * 2 + 1] = cp->image_handle[i];
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(image_handle), &image_handle[0]);
	if (err != 0) {
		goto failed;
	}

	err = bt_bip_add_header_image_desc(buf, sys_le16_to_cpu(cp->image_desc_len),
					   cp->image_desc);
	if (err != 0) {
		goto failed;
	}

	err = bt_avrcp_cover_art_ct_get_image(default_ca_ct, true, buf);
	if (err == 0) {
		return BTP_STATUS_SUCCESS;
	}

failed:
	net_buf_unref(buf);
	return BTP_STATUS_FAILED;
}

static void ca_ct_get_image_rsp(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				struct net_buf *buf)
{
	struct btp_avrcp_get_image_rsp_ev *ev;
	uint16_t body_len = 0;
	const uint8_t *body = NULL;
	int err;

	if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		err = bt_obex_get_header_body(buf, &body_len, &body);
	} else {
		err = bt_obex_get_header_end_body(buf, &body_len, &body);
	}

	if (err != 0) {
		body_len = 0;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + body_len, (uint8_t **)&ev);

	ev->rsp_code = rsp_code;
	ev->body_len = sys_cpu_to_le16(body_len);
	if ((body_len > 0U) && (body != NULL)) {
		memcpy(ev->body, body, body_len);
	}
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_IMAGE_RSP, ev, sizeof(*ev) + body_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		buf = bt_avrcp_cover_art_ct_create_pdu(default_ca_ct, &ca_tx_pool);
		if (buf == NULL) {
			return;
		}

		err = bt_avrcp_cover_art_ct_get_image(default_ca_ct, true, buf);
		if (err != 0) {
			net_buf_unref(buf);
		}
	}
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
static uint8_t ca_ct_get_linked_thumbnail(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_avrcp_ct_get_linked_thumbnail_cmd *cp = cmd;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint8_t image_handle[16];
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address);

	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(default_ca_ct, &ca_tx_pool);
	if (buf == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_obex_add_header_conn_id(buf, default_ca_ct->client._conn_id);
	if (err != 0) {
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL),
				      BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL);
	if (err != 0) {
		goto failed;
	}

	/* Convert string to unicode */
	memset(&image_handle[0], '\x00', sizeof(image_handle));
	for (int i = 0; i < sizeof(cp->image_handle); i++) {
		image_handle[i * 2 + 1] = cp->image_handle[i];
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(image_handle), &image_handle[0]);
	if (err != 0) {
		goto failed;
	}

	err = bt_avrcp_cover_art_ct_get_linked_thumbnail(default_ca_ct, true, buf);
	if (err == 0) {
		return BTP_STATUS_SUCCESS;
	}

failed:
	net_buf_unref(buf);
	return BTP_STATUS_FAILED;
}

static void ca_ct_get_linked_thumbnail_rsp(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
					   struct net_buf *buf)
{
	struct btp_avrcp_get_linked_thumbnail_rsp_ev *ev;
	uint16_t body_len = 0;
	const uint8_t *body = NULL;
	int err;

	if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		err = bt_obex_get_header_body(buf, &body_len, &body);
	} else {
		err = bt_obex_get_header_end_body(buf, &body_len, &body);
	}

	if (err != 0) {
		body_len = 0;
	}

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + body_len, (uint8_t **)&ev);

	ev->rsp_code = rsp_code;
	ev->body_len = sys_cpu_to_le16(body_len);
	if ((body_len > 0U) && (body != NULL)) {
		memcpy(ev->body, body, body_len);
	}
	bt_addr_copy(&ev->address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_GET_LINKED_THUMBNAIL_RSP, ev,
		     sizeof(*ev) + body_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		buf = bt_avrcp_cover_art_ct_create_pdu(default_ca_ct, &ca_tx_pool);
		if (buf == NULL) {
			return;
		}

		err = bt_avrcp_cover_art_ct_get_linked_thumbnail(default_ca_ct, true, buf);
		if (err != 0) {
			net_buf_unref(buf);
		}
	}
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */

static void ca_ct_l2cap_connected(struct bt_avrcp_ct *ct,
				  struct bt_avrcp_cover_art_ct *cover_art_ct)
{
	(void)bt_avrcp_cover_art_ct_connect(default_ca_ct);
}

static void ca_ct_l2cap_disconnected(struct bt_avrcp_cover_art_ct *ct)
{
	default_ca_ct = NULL;
}

static void ca_ct_connected(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, uint8_t version,
			    uint16_t mopl, struct net_buf *buf)
{
	struct btp_avrcp_ca_ct_connected_ev ev;

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_CA_CT_CONNECTED, &ev, sizeof(ev));
}

static void ca_ct_disconnected(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
			       struct net_buf *buf)
{
	struct btp_avrcp_ca_ct_disconnected_ev ev;

	bt_addr_copy(&ev.address, bt_conn_get_dst_br(default_conn));
	tester_event(BTP_SERVICE_ID_AVRCP, BTP_AVRCP_EV_CA_CT_DISCONNECTED, &ev, sizeof(ev));
}

static struct bt_avrcp_cover_art_ct_cb cover_art_ct_cb = {
	.l2cap_connected = ca_ct_l2cap_connected,
	.l2cap_disconnected = ca_ct_l2cap_disconnected,
	.connect = ca_ct_connected,
	.disconnect = ca_ct_disconnected,
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
	.get_image_properties = ca_ct_get_image_props_rsp,
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
	.get_image = ca_ct_get_image_rsp,
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */
	.get_linked_thumbnail = ca_ct_get_linked_thumbnail_rsp,
};
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
static const char *strnstr(const char *haystack, const char *needle, size_t len)
{
	size_t needle_len;

	if ((haystack == NULL) || (needle == NULL)) {
		return NULL;
	}

	if (*needle == '\0') {
		return haystack;
	}

	needle_len = strlen(needle);

	for (size_t i = 0; i + needle_len <= len && haystack[i] != '\0'; i++) {
		if (strncmp(&haystack[i], needle, needle_len) == 0) {
			return &haystack[i];
		}
	}

	return NULL;
}

static void ca_tg_l2cap_connected(struct bt_avrcp_tg *tg,
				  struct bt_avrcp_cover_art_tg *cover_art_tg)
{
	default_ca_tg = cover_art_tg;
}

static void ca_tg_l2cap_disconnected(struct bt_avrcp_cover_art_tg *tg)
{
	default_ca_tg = NULL;
}

static void ca_tg_connected(struct bt_avrcp_cover_art_tg *tg, uint8_t version, uint16_t mopl,
			    struct net_buf *buf)
{
	int err;

	err = bt_avrcp_cover_art_tg_connect(default_ca_tg, BT_OBEX_RSP_CODE_OK);
	if (err != 0) {
		return;
	}
	ca_tg_mopl = mopl;
	tg_media_elem_items[0].num_attrs = 8; /* Expose default cover art */
	tg_media_elem_items[1].num_attrs = 2; /* Expose default cover art */
}

static void ca_tg_disconnected(struct bt_avrcp_cover_art_tg *tg, struct net_buf *buf)
{
	int err;

	err = bt_avrcp_cover_art_tg_disconnect(default_ca_tg, BT_OBEX_RSP_CODE_OK);
	if (err != 0) {
		return;
	}
	tg_media_elem_items[0].num_attrs = 7; /* Don't expose default cover art */
	tg_media_elem_items[1].num_attrs = 1; /* Don't expose default cover art */
}

static void ca_tg_abort_req(struct bt_avrcp_cover_art_tg *tg, struct net_buf *buf)
{
	int err;

	err = bt_avrcp_cover_art_tg_abort(default_ca_tg, BT_OBEX_RSP_CODE_SUCCESS, NULL);
	if (err == 0) {
		ca_tg_body_pos = 0;
	}
}

static bool ca_tg_find_header_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	struct bt_obex_hdr *data = user_data;

	if (hdr->id == data->id) {
		data->data = hdr->data;
		data->len = hdr->len;
		return false;
	}
	return true;
}

static uint8_t ca_tg_parse_image_handle(struct net_buf *buf, struct image_item **out_item)
{
	struct bt_obex_hdr hdr;
	int err;

	hdr.id = BT_BIP_HEADER_ID_IMG_HANDLE;
	hdr.len = 0;
	hdr.data = NULL;

	err = bt_obex_header_parse(buf, ca_tg_find_header_cb, &hdr);
	if (err != 0) {
		return BT_OBEX_RSP_CODE_BAD_REQ;
	}

	if ((hdr.data == NULL) || (hdr.len != IMAGE_HANDLE_UNICODE_LEN)) {
		return BT_OBEX_RSP_CODE_BAD_REQ;
	}

	ARRAY_FOR_EACH(ca_tg_image_items, i) {
		if (memcmp(ca_tg_image_items[i].handle, hdr.data, IMAGE_HANDLE_UNICODE_LEN) == 0) {
			*out_item = &ca_tg_image_items[i];
			return BT_OBEX_RSP_CODE_SUCCESS;
		}
	}

	return BT_OBEX_RSP_CODE_NOT_FOUND;
}

static uint8_t ca_tg_parse_image_desc(struct net_buf *buf, struct image_item *item,
				      struct image_variant **out_variant)
{
	struct bt_obex_hdr hdr;
	int err;

	hdr.id = BT_BIP_HEADER_ID_IMG_DESC;
	hdr.len = 0;
	hdr.data = NULL;

	err = bt_obex_header_parse(buf, ca_tg_find_header_cb, &hdr);
	if (err != 0) {
		return BT_OBEX_RSP_CODE_BAD_REQ;
	}

	if (hdr.len == 0) {
		*out_variant = &item->variants[0];
		return BT_OBEX_RSP_CODE_SUCCESS;
	}

	for (uint8_t i = 0; i < item->num_variants; i++) {
		if ((strnstr(hdr.data, item->variants[i].encoding, hdr.len) != NULL) &&
		    (strnstr(hdr.data, item->variants[i].pixel, hdr.len) != NULL)) {
			*out_variant = &item->variants[i];
			return BT_OBEX_RSP_CODE_SUCCESS;
		}
	}

	return BT_OBEX_RSP_CODE_NOT_ACCEPT;
}

static uint8_t ca_tg_prepare_body(struct net_buf *buf, uint16_t *out_len)
{
	uint16_t len = 0;
	int err;

	err = bt_obex_add_header_body_or_end_body(buf, ca_tg_mopl, ca_tg_body_len - ca_tg_body_pos,
						  ca_tg_body + ca_tg_body_pos, &len);
	if (err != 0) {
		return BT_OBEX_RSP_CODE_INTER_ERROR;
	}

	*out_len = len;

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		return BT_OBEX_RSP_CODE_SUCCESS;
	}

	return BT_OBEX_RSP_CODE_CONTINUE;
}

static void ca_tg_get_image_props_req(struct bt_avrcp_cover_art_tg *tg, bool final,
				      struct net_buf *buf)
{
	struct net_buf *tx_buf = NULL;
	struct image_item *item;
	uint8_t rsp_code;
	uint16_t len = 0;
	int err;

	if (!final) {
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto error_rsp;
	}

	if (ca_tg_body_pos != 0) {
		goto rsp;
	}

	rsp_code = ca_tg_parse_image_handle(buf, &item);
	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		goto error_rsp;
	}

	ca_tg_body = item->props;
	ca_tg_body_len = item->props_len;

rsp:
	tx_buf = bt_avrcp_cover_art_tg_create_pdu(default_ca_tg, &ca_tx_pool);
	if (tx_buf == NULL) {
		return;
	}

	rsp_code = ca_tg_prepare_body(tx_buf, &len);
	if (rsp_code == BT_OBEX_RSP_CODE_INTER_ERROR) {
		net_buf_unref(tx_buf);
		return;
	}

error_rsp:
	err = bt_avrcp_cover_art_tg_get_image_properties(default_ca_tg, rsp_code, tx_buf);
	if (err != 0) {
		net_buf_unref(tx_buf);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			ca_tg_body_pos += len;
		} else {
			ca_tg_body_pos = 0;
		}
	}
}

static void ca_tg_get_image_req(struct bt_avrcp_cover_art_tg *tg, bool final, struct net_buf *buf)
{
	struct net_buf *tx_buf = NULL;
	struct image_item *item;
	struct image_variant *variant;
	uint8_t rsp_code;
	uint16_t len = 0;
	int err;

	if (!final) {
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto error_rsp;
	}

	if (ca_tg_body_pos != 0) {
		goto rsp;
	}

	rsp_code = ca_tg_parse_image_handle(buf, &item);
	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		goto error_rsp;
	}

	rsp_code = ca_tg_parse_image_desc(buf, item, &variant);
	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		goto error_rsp;
	}

	ca_tg_body_len = variant->image_len;
	ca_tg_body = variant->image;

rsp:
	tx_buf = bt_avrcp_cover_art_tg_create_pdu(default_ca_tg, &ca_tx_pool);
	if (tx_buf == NULL) {
		return;
	}

	if (ca_tg_body_pos == 0) {
		err = bt_obex_add_header_len(tx_buf, ca_tg_body_len);
		if (err != 0) {
			net_buf_unref(tx_buf);
			return;
		}
	}

	rsp_code = ca_tg_prepare_body(tx_buf, &len);
	if (rsp_code == BT_OBEX_RSP_CODE_INTER_ERROR) {
		net_buf_unref(tx_buf);
		return;
	}

error_rsp:
	err = bt_avrcp_cover_art_tg_get_image(default_ca_tg, rsp_code, tx_buf);
	if (err != 0) {
		net_buf_unref(tx_buf);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			ca_tg_body_pos += len;
		} else {
			ca_tg_body_pos = 0;
		}
	}
}

static void ca_tg_get_linked_thumbnail_req(struct bt_avrcp_cover_art_tg *tg, bool final,
					   struct net_buf *buf)
{
	struct net_buf *tx_buf = NULL;
	struct image_item *item;
	uint8_t rsp_code;
	uint16_t len = 0;
	int err;

	if (!final) {
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto error_rsp;
	}

	if (ca_tg_body_pos != 0) {
		goto rsp;
	}

	rsp_code = ca_tg_parse_image_handle(buf, &item);
	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		goto error_rsp;
	}

	ca_tg_body_len = item->variants[item->num_variants - 1].image_len;
	ca_tg_body = item->variants[item->num_variants - 1].image;

rsp:
	tx_buf = bt_avrcp_cover_art_tg_create_pdu(default_ca_tg, &ca_tx_pool);
	if (tx_buf == NULL) {
		return;
	}

	rsp_code = ca_tg_prepare_body(tx_buf, &len);
	if (rsp_code == BT_OBEX_RSP_CODE_INTER_ERROR) {
		net_buf_unref(tx_buf);
		return;
	}

error_rsp:
	err = bt_avrcp_cover_art_tg_get_linked_thumbnail(default_ca_tg, rsp_code, tx_buf);
	if (err != 0) {
		net_buf_unref(tx_buf);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			ca_tg_body_pos += len;
		} else {
			ca_tg_body_pos = 0;
		}
	}
}

static struct bt_avrcp_cover_art_tg_cb cover_art_tg_cb = {
	.l2cap_connected = ca_tg_l2cap_connected,
	.l2cap_disconnected = ca_tg_l2cap_disconnected,
	.connect = ca_tg_connected,
	.disconnect = ca_tg_disconnected,
	.abort = ca_tg_abort_req,
	.get_image_properties = ca_tg_get_image_props_req,
	.get_image = ca_tg_get_image_req,
	.get_linked_thumbnail = ca_tg_get_linked_thumbnail_req,
};
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

static const struct btp_handler avrcp_handlers[] = {
	{
		.opcode = BTP_AVRCP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = avrcp_read_supported_commands
	},
#if defined(CONFIG_BT_AVRCP_CONTROLLER)
	{
		.opcode = BTP_AVRCP_CONTROL_CONNECT,
		.expect_len = sizeof(struct btp_avrcp_control_connect_cmd),
		.func = control_connect
	},
	{
		.opcode = BTP_AVRCP_CONTROL_DISCONNECT,
		.expect_len = sizeof(struct btp_avrcp_control_disconnect_cmd),
		.func = control_disconnect
	},
	{
		.opcode = BTP_AVRCP_BROWSING_CONNECT,
		.expect_len = sizeof(struct btp_avrcp_browsing_connect_cmd),
		.func = browsing_connect
	},
	{
		.opcode = BTP_AVRCP_BROWSING_DISCONNECT,
		.expect_len = sizeof(struct btp_avrcp_browsing_disconnect_cmd),
		.func = browsing_disconnect
	},
	{
		.opcode = BTP_AVRCP_UNIT_INFO,
		.expect_len = sizeof(struct btp_avrcp_unit_info_cmd),
		.func = unit_info
	},
	{
		.opcode = BTP_AVRCP_SUBUNIT_INFO,
		.expect_len = sizeof(struct btp_avrcp_subunit_info_cmd),
		.func = subunit_info
	},
	{
		.opcode = BTP_AVRCP_PASS_THROUGH,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = pass_through
	},
	{
		.opcode = BTP_AVRCP_GET_CAPS,
		.expect_len = sizeof(struct btp_avrcp_get_caps_cmd),
		.func = get_caps
	},
	{
		.opcode = BTP_AVRCP_LIST_PLAYER_APP_SETTING_ATTRS,
		.expect_len = sizeof(struct btp_avrcp_list_player_app_setting_attrs_cmd),
		.func = list_player_app_setting_attrs
	},
	{
		.opcode = BTP_AVRCP_LIST_PLAYER_APP_SETTING_VALS,
		.expect_len = sizeof(struct btp_avrcp_list_player_app_setting_vals_cmd),
		.func = list_player_app_setting_vals
	},
	{
		.opcode = BTP_AVRCP_GET_CURR_PLAYER_APP_SETTING_VAL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_curr_player_app_setting_val
	},
	{
		.opcode = BTP_AVRCP_SET_PLAYER_APP_SETTING_VAL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = set_player_app_setting_val
	},
	{
		.opcode = BTP_AVRCP_GET_PLAYER_APP_SETTING_ATTR_TEXT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_player_app_setting_attr_text
	},
	{
		.opcode = BTP_AVRCP_GET_PLAYER_APP_SETTING_VAL_TEXT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_player_app_setting_val_text
	},
	{
		.opcode = BTP_AVRCP_GET_ELEMENT_ATTRS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_element_attrs
	},
	{
		.opcode = BTP_AVRCP_GET_PLAY_STATUS,
		.expect_len = sizeof(struct btp_avrcp_get_play_status_cmd),
		.func = get_play_status
	},
	{
		.opcode = BTP_AVRCP_REGISTER_NOTIFICATION,
		.expect_len = sizeof(struct btp_avrcp_register_notification_cmd),
		.func = register_notification
	},
	{
		.opcode = BTP_AVRCP_SET_ABSOLUTE_VOLUME,
		.expect_len = sizeof(struct btp_avrcp_set_absolute_volume_cmd),
		.func = set_absolute_volume
	},
	{
		.opcode = BTP_AVRCP_SET_ADDRESSED_PLAYER,
		.expect_len = sizeof(struct btp_avrcp_set_addressed_player_cmd),
		.func = set_addressed_player
	},
	{
		.opcode = BTP_AVRCP_PLAY_ITEM,
		.expect_len = sizeof(struct btp_avrcp_play_item_cmd),
		.func = play_item
	},
	{
		.opcode = BTP_AVRCP_ADD_TO_NOW_PLAYING,
		.expect_len = sizeof(struct btp_avrcp_add_to_now_playing_cmd),
		.func = add_to_now_playing
	},
#if defined(CONFIG_BT_AVRCP_BROWSING)
	{
		.opcode = BTP_AVRCP_GET_FOLDER_ITEMS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_folder_items
	},
	{
		.opcode = BTP_AVRCP_GET_TOTAL_NUMBER_OF_ITEMS,
		.expect_len = sizeof(struct btp_avrcp_get_total_number_of_items_cmd),
		.func = get_total_number_of_items
	},
	{
		.opcode = BTP_AVRCP_SET_BROWSED_PLAYER,
		.expect_len = sizeof(struct btp_avrcp_set_browsed_player_cmd),
		.func = set_browsed_player
	},
	{
		.opcode = BTP_AVRCP_CHANGE_PATH,
		.expect_len = sizeof(struct btp_avrcp_change_path_cmd),
		.func = change_path
	},
	{
		.opcode = BTP_AVRCP_GET_ITEM_ATTRS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = get_item_attrs
	},
	{
		.opcode = BTP_AVRCP_SEARCH,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = search
	},
#endif /* CONFIG_BT_AVRCP_BROWSING */
#endif /* CONFIG_BT_AVRCP_CONTROLLER */
#if defined(CONFIG_BT_AVRCP_TARGET)
	{
		.opcode = BTP_AVRCP_TG_REGISTER_NOTIFICATION,
		.expect_len = sizeof(struct btp_avrcp_tg_register_notification_cmd),
		.func = tg_register_notification
	},
	{
		.opcode = BTP_AVRCP_TG_CONTROL_PLAYBACK,
		.expect_len = sizeof(struct btp_avrcp_tg_control_playback_cmd),
		.func = tg_control_playback
	},
	{
		.opcode = BTP_AVRCP_TG_CHANGE_PATH,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = tg_change_path
	},
#endif /* CONFIG_BT_AVRCP_TARGET */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
	{
		.opcode = BTP_AVRCP_CA_CT_CONNECT,
		.expect_len = sizeof(struct btp_avrcp_ca_ct_connect_cmd),
		.func = ca_ct_connect
	},
	{
		.opcode = BTP_AVRCP_CA_CT_DISCONNECT,
		.expect_len = sizeof(struct btp_avrcp_ca_ct_disconnect_cmd),
		.func = ca_ct_disconnect
	},
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
	{
		.opcode = BTP_AVRCP_CA_CT_GET_IMAGE_PROPS,
		.expect_len = sizeof(struct btp_avrcp_ct_get_image_props_cmd),
		.func = ca_ct_get_image_props
	},
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
	{
		.opcode = BTP_AVRCP_CA_CT_GET_IMAGE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = ca_ct_get_image
	},
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
	{
		.opcode = BTP_AVRCP_CA_CT_GET_LINKED_THUMBNAIL,
		.expect_len = sizeof(struct btp_avrcp_ct_get_linked_thumbnail_cmd),
		.func = ca_ct_get_linked_thumbnail
	},
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */
};

uint8_t tester_init_avrcp(void)
{
#if defined(CONFIG_BT_AVRCP_CONTROLLER)
	bt_avrcp_ct_register_cb(&ct_cb);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

#if defined(CONFIG_BT_AVRCP_TARGET)
	bt_avrcp_tg_register_cb(&tg_cb);
#endif /* CONFIG_BT_AVRCP_TARGET */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
	bt_avrcp_cover_art_ct_cb_register(&cover_art_ct_cb);
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
	bt_avrcp_cover_art_tg_cb_register(&cover_art_tg_cb);
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */

	tester_register_command_handlers(BTP_SERVICE_ID_AVRCP, avrcp_handlers,
					 ARRAY_SIZE(avrcp_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_avrcp(void)
{
	return BTP_STATUS_SUCCESS;
}
