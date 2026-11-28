/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "ots_dir_list_internal.h"
#include "ots_internal.h"

/* Control Point opcodes from the Object Transfer Service specification */
#define OACP_CREATE 0x01
#define OLCP_FIRST  0x01
#define OLCP_LAST   0x02

#define OBJ_SIZE 16U

/* Directory Listing record of an unnamed object with a 16-bit type: Record Length, Object ID,
 * Name Length, Flags, Object Type, Current Size and Properties.
 */
#define UNNAMED_RECORD_SIZE    (2U + 6U + 1U + 1U + 2U + 4U + 4U)
/* Offset of Name Length in a record: Record Length and Object ID come first */
#define RECORD_NAME_LEN_OFFSET (2U + 6U)

static struct bt_ots *ots;
static const struct bt_gatt_attr *name_attr;
static const struct bt_gatt_attr *size_attr;
static const struct bt_gatt_attr *oacp_attr;
static const struct bt_gatt_attr *olcp_attr;

/* Slot 0 holds a named object that stays for the whole suite */
static char obj_names[CONFIG_BT_OTS_MAX_OBJ_CNT][CONFIG_BT_OTS_OBJ_MAX_NAME_LEN + 1];
static uint64_t obj_ids[CONFIG_BT_OTS_MAX_OBJ_CNT];
static size_t obj_count;
static const char *next_name;

static int obj_created(struct bt_ots *ots_inst, struct bt_conn *conn, uint64_t id,
		       const struct bt_ots_obj_add_param *add_param,
		       struct bt_ots_obj_created_desc *created_desc)
{
	char *name;

	if (obj_count >= ARRAY_SIZE(obj_names)) {
		return -ENOMEM;
	}

	obj_ids[obj_count] = id;
	name = obj_names[obj_count++];
	strcpy(name, next_name);

	created_desc->name = name;
	created_desc->size.alloc = add_param->size;
	BT_OTS_OBJ_SET_PROP_WRITE(created_desc->props);

	return 0;
}

static ssize_t obj_write(struct bt_ots *ots_inst, struct bt_conn *conn, uint64_t id,
			 const void *buf, size_t len, off_t offset, size_t rem)
{
	return len;
}

static struct bt_ots_cb ots_cb = {
	.obj_created = obj_created,
	.obj_write = obj_write,
};

static const struct bt_gatt_attr *find_attr(const struct bt_uuid *uuid)
{
	const struct bt_gatt_attr *attr = bt_gatt_find_by_uuid(NULL, 0, uuid);

	zassert_not_null(attr, "attribute not found");

	return attr;
}

static void enable_indications(const struct bt_gatt_attr *cp_attr)
{
	/* The Client Characteristic Configuration follows the Control Point value */
	const struct bt_gatt_attr *ccc_attr = cp_attr + 1;
	struct bt_gatt_ccc_managed_user_data *ccc = ccc_attr->user_data;

	zassert_equal(bt_uuid_cmp(ccc_attr->uuid, BT_UUID_GATT_CCC), 0);
	ccc->cfg_changed(ccc_attr, BT_GATT_CCC_INDICATE);
}

static void cp_write(const struct bt_gatt_attr *cp_attr, const void *buf, uint16_t len)
{
	ssize_t ret = cp_attr->write(NULL, cp_attr, buf, len, 0, 0);

	zassert_equal(ret, len, "Control Point write failed (%zd)", ret);

	/* Let the indication work run, a new write is rejected while it is pending */
	k_sleep(K_MSEC(1));
}

static void olcp_select(uint8_t opcode)
{
	cp_write(olcp_attr, &opcode, sizeof(opcode));
}

static uint32_t dir_list_size(void)
{
	struct bt_ots_obj_size size;
	ssize_t ret;

	/* The Directory Listing Object is the first object and its size is updated when it is
	 * selected, so select another object first.
	 */
	olcp_select(OLCP_LAST);
	olcp_select(OLCP_FIRST);

	ret = size_attr->read(NULL, size_attr, &size, sizeof(size), 0);
	zassert_equal(ret, sizeof(size));

	return sys_le32_to_cpu(size.cur);
}

static void add_object(const char *name)
{
	struct bt_ots_obj_add_param param = {
		.size = OBJ_SIZE,
		.type.uuid_16.uuid.type = BT_UUID_TYPE_16,
		.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL,
	};

	next_name = name;
	zassert_true(bt_ots_obj_add(ots, &param) > 0);
}

static void create_unnamed_object(void)
{
	uint8_t create[7];

	create[0] = OACP_CREATE;
	sys_put_le32(OBJ_SIZE, &create[1]);
	sys_put_le16(BT_UUID_OTS_TYPE_UNSPECIFIED_VAL, &create[5]);

	next_name = "";
	cp_write(oacp_attr, create, sizeof(create));
}

static void *ots_setup(void)
{
	struct bt_ots_init_param param = {
		.cb = &ots_cb,
	};

	BT_OTS_OACP_SET_FEAT_CREATE(param.features.oacp);
	BT_OTS_OACP_SET_FEAT_WRITE(param.features.oacp);

	ots = bt_ots_free_instance_get();
	zassert_not_null(ots);
	zassert_ok(bt_ots_init(ots, &param));

	name_attr = find_attr(BT_UUID_OTS_NAME);
	size_attr = find_attr(BT_UUID_OTS_SIZE);
	oacp_attr = find_attr(BT_UUID_OTS_ACTION_CP);
	olcp_attr = find_attr(BT_UUID_OTS_LIST_CP);

	enable_indications(oacp_attr);
	enable_indications(olcp_attr);

	add_object("anchor");

	return NULL;
}

static void ots_after(void *fixture)
{
	/* Remove the objects added by the test, keep the one in slot 0 */
	while (obj_count > 1) {
		obj_count--;
		zassert_ok(bt_ots_obj_delete(ots, obj_ids[obj_count]));
	}
}

ZTEST_SUITE(ots_dir_list, NULL, ots_setup, NULL, ots_after, NULL);

/* A client may write an empty Object Name, the listing then has a zero length name */
ZTEST(ots_dir_list, test_empty_name_write)
{
	const char *name = "obj1";
	uint32_t size_named;
	ssize_t ret;

	add_object(name);
	size_named = dir_list_size();

	/* The object added last is the last one in the list */
	olcp_select(OLCP_LAST);
	ret = name_attr->write(NULL, name_attr, "", 0, 0, 0);
	zassert_equal(ret, 0, "Object Name write failed (%zd)", ret);

	zassert_equal(dir_list_size(), size_named - strlen(name));
}

/* An object created through OACP Create has no name until the client writes one */
ZTEST(ots_dir_list, test_created_object)
{
	uint32_t size_before;

	size_before = dir_list_size();
	create_unnamed_object();

	zassert_equal(dir_list_size(), size_before + UNNAMED_RECORD_SIZE);
}

/* The encoded listing has the computed size, including a record with a zero length name */
ZTEST(ots_dir_list, test_encoded_listing)
{
	size_t unnamed_records = 0;
	size_t offset = 0;
	uint8_t *records;
	uint32_t size;
	ssize_t len;
	void *data;

	create_unnamed_object();
	size = dir_list_size();

	len = bt_ots_dir_list_content_get(ots->dir_list, ots->obj_manager, &data, size, 0);
	zassert_equal(len, size, "Listing content is %zd octets, size is %u", len, size);

	records = data;
	while (offset < len) {
		uint16_t rec_len = sys_get_le16(&records[offset]);

		zassert_true(offset + rec_len <= len, "Record at %zu exceeds the listing", offset);

		if (records[offset + RECORD_NAME_LEN_OFFSET] == 0U) {
			zassert_equal(rec_len, UNNAMED_RECORD_SIZE);
			unnamed_records++;
		}

		offset += rec_len;
	}

	zassert_equal(offset, len);
	zassert_equal(unnamed_records, 1U);
}
