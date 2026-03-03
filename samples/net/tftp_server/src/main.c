/*
 * Copyright (c) 2026 Guy Shilman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/tftp_server.h>
#include <zephyr/net/tftp_common.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>

#define TEST_FS_NODE DT_NODELABEL(tftp_ramfs)
FS_FSTAB_DECLARE_ENTRY(TEST_FS_NODE);
static struct fs_mount_t *mp = &FS_FSTAB_ENTRY(TEST_FS_NODE);

/* Session error callback - optional, can be NULL */
static void session_error_callback(const struct tftp_session_error *session_error)
{
	printk("TFTP Session Error: code=%d, state=%d, bytes=%zu\n", session_error->error_code,
	       session_error->session_state, session_error->bytes_transferred);
}

/* Server error callback - optional, can be NULL */
static void server_error_callback(enum tftp_error_code error_code)
{
	printk("TFTP Server Fatal Error: code=%d\n", error_code);
}

int main(void)
{
	struct tftp_server *server;
	int ret;

	printk("=== TFTP Server Sample ===\n");

	/* Initialize TFTP server with error callbacks */
	server = tftp_server_get_instance();
	printk("Initializing UDP transport on port %d...\n", TFTP_PORT);
	ret = tftp_server_init_udp(server, STRINGIFY(TFTP_PORT), server_error_callback,
						     session_error_callback);
	if (ret != 0) {
		printk("CRITICAL: Failed to initialize TFTP server: %d\n", ret);
		return ret;
	}

	/* Format and mount RAM disk */
	printk("Formatting FS (RAM disk)...\n");
	(void)fs_mkfs(mp->type, (uintptr_t)mp->storage_dev, NULL, 0);
	printk("Mounting FS...\n");
	ret = fs_mount(mp);
	if (ret != 0) {
		printk("Failed to mount FS: %d\n", ret);
		return ret;
	}

	/* Start the server */
	printk("Starting TFTP server thread...\n");
	ret = tftp_server_start(server);
	if (ret != 0) {
		printk("CRITICAL: Failed to start TFTP server: %d\n", ret);
		fs_unmount(mp);
		return ret;
	}

	printk("TFTP Server started on port %d.\n", TFTP_PORT);
	printk("The server is now ready to receive connections.\n");
	printk("You can use a TFTP client to PUT/GET files (e.g., 'tftp 192.0.2.1 -m octet -c put "
	       "<file>').\n");

	return 0;
}
