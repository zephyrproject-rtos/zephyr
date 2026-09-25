/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Uploads a firmware image read from a littlefs file system on external flash to a
 * Bluetooth peer that runs the SMP service.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_client.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt_client.h>
#include <zephyr/sys/atomic.h>

#include <bootutil/image.h>

LOG_MODULE_REGISTER(smp_bt_client_sample, LOG_LEVEL_INF);

#define AUTOMOUNT_NODE DT_NODELABEL(lfs1)
#define MOUNT_POINT    DT_PROP(AUTOMOUNT_NODE, mount_point)
#define IMAGE_PATH     MOUNT_POINT "/update.bin"

/* Placeholder image written when the file system holds no image yet. */
#define PLACEHOLDER_SIZE 4096U
/* How much of the file is handed to the image client per call. */
#define CHUNK_SIZE       1024U

#define ECHO_STRING "smp client"

BUILD_ASSERT(PLACEHOLDER_SIZE % CHUNK_SIZE == 0, "Placeholder must be a whole number of chunks");

static struct smp_client_object smp_client;
static struct img_mgmt_client img_client;
static struct os_mgmt_client os_client;
static uint8_t chunk[CHUNK_SIZE];

/* Released by whichever of the connected callback and main takes it first */
static atomic_ptr_t peer;
static K_SEM_DEFINE(peer_connected, 0, 1);

/* Matches the advertised SMP service UUID */
static bool adv_data_cb(struct bt_data *data, void *user_data)
{
	static const uint8_t smp_svc_uuid[] = {SMP_BT_SVC_UUID_VAL};
	bool *found = user_data;

	if (data->type == BT_DATA_UUID128_ALL && data->data_len == sizeof(smp_svc_uuid) &&
	    memcmp(data->data, smp_svc_uuid, sizeof(smp_svc_uuid)) == 0) {
		*found = true;
		return false;
	}

	return true;
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bool found = false;
	int rc;

	if (type != BT_GAP_ADV_TYPE_ADV_IND) {
		return;
	}

	bt_data_parse(ad, adv_data_cb, &found);
	if (!found) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	LOG_INF("SMP server found at %s, RSSI %d", addr_str, rssi);

	rc = bt_le_scan_stop();
	if (rc == 0) {
		struct bt_conn *conn;

		rc = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				       &conn);
		if (rc == 0) {
			atomic_ptr_set(&peer, conn);
		}
	}

	if (rc != 0) {
		LOG_ERR("Failed to connect to the peer (err %d)", rc);
		k_sem_give(&peer_connected);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
		if (atomic_ptr_cas(&peer, conn, NULL)) {
			bt_conn_unref(conn);
		}
	} else {
		LOG_INF("Connected");
	}

	k_sem_give(&peer_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/*
 * Writes a zero filled image with a valid MCUboot image header, so that the sample has
 * something to upload. img_mgmt rejects an image without the MCUboot magic.
 */
static int placeholder_write(void)
{
	const struct image_header hdr = {
		.ih_magic = IMAGE_MAGIC,
		.ih_hdr_size = sizeof(struct image_header),
		.ih_img_size = PLACEHOLDER_SIZE - sizeof(struct image_header),
		.ih_ver = {.iv_revision = 1},
	};
	struct fs_file_t file;
	size_t off;
	int rc;

	memset(chunk, 0, sizeof(chunk));
	memcpy(chunk, &hdr, sizeof(hdr));

	fs_file_t_init(&file);
	rc = fs_open(&file, IMAGE_PATH, FS_O_CREATE | FS_O_WRITE);
	if (rc != 0) {
		return rc;
	}

	for (off = 0; off < PLACEHOLDER_SIZE; off += CHUNK_SIZE) {
		rc = fs_write(&file, chunk, CHUNK_SIZE);
		if (rc < 0) {
			break;
		}
		/* A short write means the file system is full */
		if (rc != (int)CHUNK_SIZE) {
			rc = -ENOSPC;
			break;
		}
		/* Only the first chunk carries the header */
		memset(chunk, 0, sizeof(hdr));
	}

	(void)fs_close(&file);
	return (rc < 0) ? rc : 0;
}

static int image_upload(size_t image_size)
{
	struct mcumgr_image_upload res;
	struct fs_file_t file;
	size_t sent = 0;
	int read_len;
	int rc;

	fs_file_t_init(&file);
	rc = fs_open(&file, IMAGE_PATH, FS_O_READ);
	if (rc != 0) {
		LOG_ERR("Failed to open %s (err %d)", IMAGE_PATH, rc);
		return rc;
	}

	/* Image 0, no hash, so the upload cannot be resumed */
	rc = img_mgmt_client_upload_init(&img_client, image_size, 0, NULL);
	if (rc != 0) {
		LOG_ERR("Failed to start the upload (err %d)", rc);
		goto out;
	}

	while (sent < image_size) {
		size_t offset = sent;

		/* The peer answers every frame with the offset it expects next */
		rc = fs_seek(&file, offset, FS_SEEK_SET);
		if (rc != 0) {
			LOG_ERR("Failed to seek %s to %zu (err %d)", IMAGE_PATH, offset, rc);
			goto out;
		}

		read_len = fs_read(&file, chunk, sizeof(chunk));
		if (read_len <= 0) {
			LOG_ERR("Failed to read %s (err %d)", IMAGE_PATH, read_len);
			rc = (read_len == 0) ? -EIO : read_len;
			goto out;
		}

		rc = img_mgmt_client_upload(&img_client, chunk, read_len, &res);
		if (rc != 0) {
			LOG_ERR("Upload failed at offset %zu (err %d)", offset, rc);
			goto out;
		}

		sent = res.image_upload_offset;

		/* Do not loop forever on a peer that never advances */
		if (sent <= offset) {
			LOG_ERR("Peer did not advance past offset %zu", offset);
			rc = -EIO;
			goto out;
		}

		LOG_INF("Uploaded %zu/%zu bytes", sent, image_size);
	}

out:
	(void)fs_close(&file);
	return rc;
}

int main(void)
{
	struct fs_statvfs vfs;
	struct bt_conn *conn;
	struct fs_dirent entry;
	int rc;

	LOG_INF("MCUmgr SMP client Bluetooth sample");

	/* fs_stat() also reports -ENOENT for a missing mount point */
	rc = fs_statvfs(MOUNT_POINT, &vfs);
	if (rc != 0) {
		LOG_ERR("Nothing is mounted at %s (err %d); check the board's "
			"zephyr,fstab,littlefs entry",
			MOUNT_POINT, rc);
		return rc;
	}

	rc = fs_stat(IMAGE_PATH, &entry);
	if (rc == -ENOENT) {
		LOG_INF("No %s yet, writing a placeholder image", IMAGE_PATH);
		rc = placeholder_write();
		if (rc == 0) {
			rc = fs_stat(IMAGE_PATH, &entry);
		}
	}

	if (rc != 0) {
		LOG_ERR("Failed to access %s (err %d)", IMAGE_PATH, rc);
		return rc;
	}

	/* An empty upload would report success without sending anything */
	if (entry.size < sizeof(struct image_header)) {
		LOG_ERR("%s is %zu bytes, too small to be an image", IMAGE_PATH, entry.size);
		return -EINVAL;
	}
	LOG_INF("Image %s is %zu bytes", IMAGE_PATH, entry.size);

	rc = bt_enable(NULL);
	if (rc != 0) {
		LOG_ERR("Bluetooth init failed (err %d)", rc);
		return rc;
	}

	rc = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_cb);
	if (rc != 0) {
		LOG_ERR("Failed to start scanning (err %d)", rc);
		return rc;
	}
	LOG_INF("Scanning for a peer that advertises the SMP service");

	rc = k_sem_take(&peer_connected, K_SECONDS(30));
	conn = (rc == 0) ? atomic_ptr_get(&peer) : NULL;
	if (conn == NULL) {
		if (rc == 0) {
			LOG_ERR("Failed to connect to the SMP server");
		} else {
			/* Stop the scan so that a peer found later is not connected to */
			(void)bt_le_scan_stop();
			LOG_ERR("No SMP server found");
		}

		/* Cancel a connection that is still being established */
		conn = atomic_ptr_set(&peer, NULL);
		if (conn != NULL) {
			(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			bt_conn_unref(conn);
		}

		return -ENOTCONN;
	}

	/* Blocks until discovery and subscription are done */
	rc = smp_bt_client_attach(conn, K_SECONDS(10));
	if (rc != 0) {
		LOG_ERR("Failed to attach the transport (err %d)", rc);
		goto disconnect;
	}
	LOG_INF("Transport attached to the peer's SMP service");

	rc = smp_client_object_init(&smp_client, SMP_BLUETOOTH_CLIENT_TRANSPORT);
	if (rc != 0) {
		LOG_ERR("Failed to initialize the SMP client (err %d)", rc);
		goto detach;
	}

	os_mgmt_client_init(&os_client, &smp_client);
	rc = os_mgmt_client_echo(&os_client, ECHO_STRING, sizeof(ECHO_STRING) - 1);
	if (rc != 0) {
		LOG_ERR("Echo command failed (err %d)", rc);
		goto detach;
	}
	LOG_INF("Peer answered the echo command");

	/* No image list buffer: the peer's image state is not read */
	img_mgmt_client_init(&img_client, &smp_client, 0, NULL);
	rc = image_upload(entry.size);
	if (rc == 0) {
		LOG_INF("Image written to the peer's secondary slot");
	}

detach:
	smp_bt_client_detach();
disconnect:
	conn = atomic_ptr_set(&peer, NULL);
	if (conn != NULL) {
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		bt_conn_unref(conn);
	}

	return rc;
}
