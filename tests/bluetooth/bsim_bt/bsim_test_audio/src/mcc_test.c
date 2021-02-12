/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <sys/sem.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/mcc.h>

#include "../../../../subsys/bluetooth/host/audio/mpl.h"
#include "../../../../subsys/bluetooth/host/audio/otc.h"

#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;

static struct bt_conn *default_conn;
static struct bt_mcc_cb_t mcc_cb;

uint64_t g_current_track_object_id;
uint64_t g_track_segments_object_id;
uint64_t g_current_group_object_id;

CREATE_FLAG(ble_is_initialized);
CREATE_FLAG(ble_link_is_ready);
CREATE_FLAG(mcc_is_initialized);
CREATE_FLAG(discovery_done);
CREATE_FLAG(current_track_object_id_read);
CREATE_FLAG(track_segments_object_id_read);
CREATE_FLAG(current_group_object_id_read);
CREATE_FLAG(object_selected);
CREATE_FLAG(metadata_read);


static void mcc_init_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("MCC init failed (%d)\n", err);
		return;
	}

	printk("MCC init succeeded\n");
	SET_FLAG(mcc_is_initialized);
}

static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Discovery of MCS failed (%d)\n", err);
		return;
	}

	printk("Discovery of MCS succeeded\n");
	SET_FLAG(discovery_done);
}

static void mcc_current_track_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Track Object ID read failed (%d)\n", err);
		return;
	}

	printk("Current Track Object ID read succeeded\n");
	g_current_track_object_id = id;
	SET_FLAG(current_track_object_id_read);
}

static void mcc_segments_obj_id_read_cb(struct bt_conn *conn, int err,
					uint64_t id)
{
	if (err) {
		FAIL("Track Segments ID read failed (%d)\n", err);
		return;
	}

	printk("Track Segments Object ID read succeeded\n");
	g_track_segments_object_id = id;
	SET_FLAG(track_segments_object_id_read);
}

static void mcc_current_group_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Group Object ID read failed (%d)\n", err);
		return;
	}

	printk("Current Group Object ID read succeeded\n");
	g_current_group_object_id = id;
	SET_FLAG(current_group_object_id_read);
}

static void mcc_otc_obj_selected_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Selecting object failed (%d)\n", err);
		return;
	}

	printk("Selecting object succeeded\n");
	SET_FLAG(object_selected);
}

static void mcc_otc_obj_metadata_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Reading object metadata failed (%d)\n", err);
		return;
	}

	printk("Reading object metadata succeeded\n");
	SET_FLAG(metadata_read);
}


int do_mcc_init(void)
{
	/* Set up the callbacks */
	mcc_cb.init             = &mcc_init_cb;
	mcc_cb.discover_mcs     = &mcc_discover_mcs_cb;
	mcc_cb.current_track_obj_id_read = &mcc_current_track_obj_id_read_cb;
	mcc_cb.segments_obj_id_read      = &mcc_segments_obj_id_read_cb;
	mcc_cb.current_group_obj_id_read = &mcc_current_group_obj_id_read_cb;
	mcc_cb.otc_obj_selected = &mcc_otc_obj_selected_cb;
	mcc_cb.otc_obj_metadata = &mcc_otc_obj_metadata_cb;

	/* Initialize the module */
	return bt_mcc_init(default_conn, &mcc_cb);
}

/* Callback after Bluetoot initialization attempt */
static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	SET_FLAG(ble_is_initialized);
}

/* Callback on connection */
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected: %s\n", addr);
	default_conn = conn;
	SET_FLAG(ble_link_is_ready);
}

void test_main(void)
{
	int err;
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};

	printk("Media Control Client test application.  Board: %s\n", CONFIG_BOARD);

	err = bt_enable(bt_ready);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(ble_is_initialized);

	bt_conn_cb_register(&conn_callbacks);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Failed to start scanning (err %d\n)", err);
	} else {
		printk("Scanning started successfully\n");
	}

	WAIT_FOR_FLAG(ble_link_is_ready);

	do_mcc_init();
	WAIT_FOR_FLAG(mcc_is_initialized);

	/* Discover MCS, subscribe to notifications */
	err = bt_mcc_discover_mcs(default_conn, true);
	if (err) {
		FAIL("Failed to start discovery of MCS: %d\n", err);
	}

	WAIT_FOR_FLAG(discovery_done);

	/* Read current track object ******************************************/
	/* Involves reading the object ID, selecting the object, */
	/* reading the object metadata and reading the object */
	err = bt_mcc_read_current_track_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_select_id(default_conn, bt_mcc_otc_inst(0),
			       g_current_track_object_id);
	if (err) {
		FAIL("Failed to select object\n");
		return;
	}

	WAIT_FOR_FLAG(object_selected);
	UNSET_FLAG(object_selected);    /* Clear flag for later use */

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_obj_metadata_read(default_conn, bt_mcc_otc_inst(0),
				       BT_OTC_METADATA_REQ_ALL);
	if (err) {
		FAIL("Failed to read object metadata\n");
		return;
	}

	WAIT_FOR_FLAG(metadata_read);
	UNSET_FLAG(metadata_read);

	err = bt_mcc_otc_read_current_track_object(default_conn);

	if (err) {
		FAIL("Failed to current track object\n");
		return;
	}

	/* TODO */
	/* In principle, this should also result in a callback. */
	/* But there is no application level callback for reading the current */
	/* track yet. */
	/* Therefore, the test ends here for now, without verifying that the */
	/* object was actually returned to us. */

	printk("Succeeded to read current track object\n");


	/* Read track segments object *****************************************/
	err = bt_mcc_read_segments_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read track segments object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_segments_object_id_read);

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_select_id(default_conn, bt_mcc_otc_inst(0),
			       g_track_segments_object_id);
	if (err) {
		FAIL("Failed to select current track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_selected);
	UNSET_FLAG(object_selected);    /* Clear flag for later use */

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_obj_metadata_read(default_conn, bt_mcc_otc_inst(0),
				       BT_OTC_METADATA_REQ_ALL);
	if (err) {
		FAIL("Failed to read current track object metadata\n");
		return;
	}

	WAIT_FOR_FLAG(metadata_read);
	UNSET_FLAG(metadata_read);

	err = bt_mcc_otc_read_track_segments_object(default_conn);

	if (err) {
		FAIL("Failed to read current track object\n");
		return;
	}

	/* TODO */
	/* In principle, this should also result in a callback. */
	/* But there is no application level callback for reading the current */
	/* track yet. */
	/* Therefore, the test ends here for now, without verifying that the */
	/* object was actually returned to us. */

	printk("Succeeded to read track segments object\n");


	/* Read current group object ******************************************/
	err = bt_mcc_read_current_group_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_select_id(default_conn, bt_mcc_otc_inst(0),
			       g_current_group_object_id);
	if (err) {
		FAIL("Failed to select current group object\n");
		return;
	}

	WAIT_FOR_FLAG(object_selected);
	UNSET_FLAG(object_selected);

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_obj_metadata_read(default_conn, bt_mcc_otc_inst(0),
				       BT_OTC_METADATA_REQ_ALL);
	if (err) {
		FAIL("Failed to read current group object metadata\n");
		return;
	}

	WAIT_FOR_FLAG(metadata_read);
	UNSET_FLAG(metadata_read);

	err = bt_mcc_otc_read_current_group_object(default_conn);

	if (err) {
		FAIL("Failed to read current group object\n");
		return;
	}

	/* TODO */
	/* In principle, this should also result in a callback. */
	/* But there is no application level callback for reading the current */
	/* track yet. */
	/* Therefore, the test ends here for now, without verifying that the */
	/* object was actually returned to us. */

	printk("Succeeded to read the current group object\n");

	PASS("MCC passed\n");
}

static const struct bst_test_instance test_mcs[] = {
	{
		.test_id = "mcc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mcc_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_mcs);
}
