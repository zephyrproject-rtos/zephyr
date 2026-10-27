/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Built in the native simulator runner context, with the host C library. */

#undef _XOPEN_SOURCE
/* For mkdtemp(), nftw() and P_tmpdir */
#define _XOPEN_SOURCE 700

#include <ftw.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sysfs_bottom.h"

static int make_dir(const char *root, const char *dir)
{
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s/%s", root, dir);

	return mkdir(path, 0755);
}

static int write_hex(const char *root, const char *dir, const char *name, int value)
{
	char path[PATH_MAX];
	FILE *file;

	snprintf(path, sizeof(path), "%s/%s/%s", root, dir, name);
	file = fopen(path, "w");
	if (file == NULL) {
		return -1;
	}

	fprintf(file, "%04x\n", value);

	return fclose(file) ? -1 : 0;
}

int native_tty_test_sysfs_create(char *root, long size)
{
	if (snprintf(root, (size_t)size, "%s/native_tty_sysfs.XXXXXX", P_tmpdir) >= size ||
	    mkdtemp(root) == NULL) {
		return -1;
	}

	return make_dir(root, "class") || make_dir(root, "devices") ? -1 : 0;
}

int native_tty_test_sysfs_add_tty(const char *root, const char *name, int vendor, int product)
{
	char usb[NAME_MAX];
	char interface[2 * NAME_MAX];
	char tty[3 * NAME_MAX];
	char link[PATH_MAX];
	char target[PATH_MAX];

	snprintf(usb, sizeof(usb), "devices/usb-%s", name);
	snprintf(interface, sizeof(interface), "%s/1-1:1.0", usb);
	snprintf(tty, sizeof(tty), "%s/%s", interface, name);

	if (make_dir(root, usb) || make_dir(root, interface) || make_dir(root, tty)) {
		return -1;
	}

	if (vendor >= 0 && (write_hex(root, usb, "idVendor", vendor) ||
			    write_hex(root, usb, "idProduct", product))) {
		return -1;
	}

	snprintf(link, sizeof(link), "class/%s", name);
	if (make_dir(root, link)) {
		return -1;
	}

	snprintf(link, sizeof(link), "%s/class/%s/device", root, name);
	snprintf(target, sizeof(target), "%s/%s", root, tty);

	return symlink(target, link);
}

static int remove_entry(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
	(void)st;
	(void)type;
	(void)ftw;

	return remove(path);
}

void native_tty_test_sysfs_remove(const char *root)
{
	(void)nftw(root, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}
