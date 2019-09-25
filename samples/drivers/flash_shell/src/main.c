/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <drivers/flash.h>
#include <device.h>
#include <soc.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(app);

#define PR_SHELL(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_NORMAL, fmt, ##__VA_ARGS__)
#define PR_ERROR(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_ERROR, fmt, ##__VA_ARGS__)
#define PR_INFO(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_INFO, fmt, ##__VA_ARGS__)
#define PR_WARNING(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_WARNING, fmt, ##__VA_ARGS__)
/*
 * When DT_FLASH_DEV_NAME is available, we use it here. Otherwise,
 * the device can be set at runtime with the set_device command.
 */
#ifndef DT_FLASH_DEV_NAME
#define DT_FLASH_DEV_NAME ""
#endif

/* Command usage info. */
#define WRITE_BLOCK_SIZE_HELP \
	("Print the device's write block size. This is the smallest amount" \
	 " of data which may be written to the device, in bytes.")
#define READ_HELP \
	("<off> <len>\n" \
	 "Read <len> bytes from device offset <off>.")
#define ERASE_HELP \
	("<off> <len>\n\n" \
	 "Erase <len> bytes from device offset <off>, " \
	 "subject to hardware page limitations.")
#define WRITE_HELP \
	("<off> <byte1> [... byteN]\n\n" \
	 "Write given bytes, starting at device offset <off>.\n" \
	 "Pages must be erased before they can be written.")
#ifdef CONFIG_FLASH_PAGE_LAYOUT
#define PAGE_COUNT_HELP \
	"\n\nPrint the number of pages on the flash device."
#define PAGE_LAYOUT_HELP \
	("[start_page] [end_page]\n\n" \
	 "Print layout of flash pages in the range [start_page, end_page],"   \
	 " which is inclusive. By default, all pages are printed.")
#define PAGE_READ_HELP \
	("<page> <len> OR <page> <off> <len>\n\n" \
	 "Read <len> bytes from given page, starting at page offset <off>," \
	  "or offset 0 if not given. No checks are made that bytes read are" \
	 " all within the page.")
#define PAGE_ERASE_HELP \
	("<page> [num]\n\n" \
	 "Erase [num] pages (default 1), starting at page <page>.")
#define PAGE_WRITE_HELP \
	("<page> <off> <byte1> [... byteN]\n\n" \
	 "Write given bytes to given page, starting at page offset <off>." \
	 " No checks are made that the bytes all fall within the page." \
	 " Pages must be erased before they can be written.")
#endif
#define SET_DEV_HELP \
	("<device_name>\n\n" \
	 "Set flash device by name. If a flash device was not found," \
	 " this command must be run first to bind a device to this module.")

#if (CONFIG_SHELL_ARGC_MAX > 4)
#define ARGC_MAX (CONFIG_SHELL_ARGC_MAX - 4)
#else
#error Please increase CONFIG_SHELL_ARGC_MAX parameter.
#endif

static struct device *flash_device;

static int check_flash_device(const struct shell *shell)
{
	if (flash_device == NULL) {
		PR_ERROR(shell, "Flash device is unknown."
				" Run set_device first.\n");
		return -ENODEV;
	}
	return 0;
}

static void dump_buffer(const struct shell *shell, u8_t *buf, size_t size)
{
	bool newline = false;
	u8_t *p = buf;

	while (size >= 8) {
		PR_SHELL(shell, "%02x %02x %02x %02x | %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
		size -= 8;
	}
	if (size > 4) {
		PR_SHELL(shell, "%02x %02x %02x %02x | ",
		       p[0], p[1], p[2], p[3]);
		p += 4;
		size -= 4;
		newline = true;
	}
	while (size--) {
		PR_SHELL(shell, "%02x ", *p++);
		newline = true;
	}
	if (newline) {
		PR_SHELL(shell, "\n");
	}
}

static int parse_ul(const char *str, unsigned long *result)
{
	char *end;
	unsigned long val;

	val = strtoul(str, &end, 0);

	if (*str == '\0' || *end != '\0') {
		return -EINVAL;
	}

	*result = val;
	return 0;
}

static int parse_u8(const char *str, u8_t *result)
{
	unsigned long val;

	if (parse_ul(str, &val) || val > 0xff) {
		return -EINVAL;
	}
	*result = (u8_t)val;
	return 0;
}

/* Read bytes, dumping contents to console and printing on error. */
static int do_read(const struct shell *shell, off_t offset, size_t len)
{
	u8_t buf[64];
	int ret;

	while (len > sizeof(buf)) {
		ret = flash_read(flash_device, offset, buf, sizeof(buf));
		if (ret) {
			goto err_read;
		}
		dump_buffer(shell, buf, sizeof(buf));
		len -= sizeof(buf);
		offset += sizeof(buf);
	}
	ret = flash_read(flash_device, offset, buf, len);
	if (ret) {
		goto err_read;
	}
	dump_buffer(shell, buf, len);
	return 0;

 err_read:
	PR_ERROR(shell, "flash_read error: %d\n", ret);
	return ret;
}

/* Erase area, handling write protection and printing on error. */
static int do_erase(const struct shell *shell, off_t offset, size_t size)
{
	int ret;

	ret = flash_write_protection_set(flash_device, false);
	if (ret) {
		PR_ERROR(shell, "Failed to disable flash protection (err: %d)."
				"\n", ret);
		return ret;
	}
	ret = flash_erase(flash_device, offset, size);
	if (ret) {
		PR_ERROR(shell, "flash_erase failed (err:%d).\n", ret);
		return ret;
	}
	ret = flash_write_protection_set(flash_device, true);
	if (ret) {
		PR_ERROR(shell, "Failed to enable flash protection (err: %d)."
				"\n", ret);
	}
	return ret;
}

/* Write bytes, handling write protection and printing on error. */
static int do_write(const struct shell *shell, off_t offset, u8_t *buf,
		    size_t len, bool read_back)
{
	int ret;

	ret = flash_write_protection_set(flash_device, false);
	if (ret) {
		PR_ERROR(shell, "Failed to disable flash protection (err: %d)."
				"\n", ret);
		return ret;
	}
	ret = flash_write(flash_device, offset, buf, len);
	if (ret) {
		PR_ERROR(shell, "flash_write failed (err:%d).\n", ret);
		return ret;
	}
	ret = flash_write_protection_set(flash_device, true);
	if (ret) {
		PR_ERROR(shell, "Failed to enable flash protection (err: %d)."
				"\n", ret);
		return ret;
	}
	if (read_back) {
		PR_SHELL(shell, "Reading back written bytes:\n");
		ret = do_read(shell, offset, len);
	}
	return ret;
}

static int cmd_flash(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_help(shell);
	return 0;
}


static int cmd_write_block_size(const struct shell *shell, size_t argc,
				char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = check_flash_device(shell);

	if (!err) {
		PR_SHELL(shell, "%d\n",
			 flash_get_write_block_size(flash_device));
	}

	return err;
}

static int cmd_read(const struct shell *shell, size_t argc, char **argv)
{
	int err = check_flash_device(shell);
	unsigned long int offset, len;

	if (err) {
		goto exit;
	}

	if (parse_ul(argv[1], &offset) || parse_ul(argv[2], &len)) {
		PR_ERROR(shell, "Invalid arguments.\n");
		err = -EINVAL;
		goto exit;
	}

	err = do_read(shell, offset, len);

exit:
	return err;
}

static int cmd_erase(const struct shell *shell, size_t argc, char **argv)
{
	int err = check_flash_device(shell);
	unsigned long int offset;
	unsigned long int size;

	if (err) {
		goto exit;
	}

	if (parse_ul(argv[1], &offset) || parse_ul(argv[2], &size)) {
		PR_ERROR(shell, "Invalid arguments.\n");
		err = -EINVAL;
		goto exit;
	}

	err = do_erase(shell, (off_t)offset, (size_t)size);
exit:
	return err;
}

static int cmd_write(const struct shell *shell, size_t argc, char **argv)
{
	unsigned long int i, offset;
	u8_t buf[ARGC_MAX];

	int err = check_flash_device(shell);

	if (err) {
		goto exit;
	}

	err = parse_ul(argv[1], &offset);
	if (err) {
		PR_ERROR(shell, "Invalid argument.\n");
		goto exit;
	}

	if ((argc - 2) > ARGC_MAX) {
		/* Can only happen if Zephyr limit is increased. */
		PR_ERROR(shell, "At most %lu bytes can be written.\n"
				"In order to write more bytes please increase"
				" parameter: CONFIG_SHELL_ARGC_MAX.\n",
			 (unsigned long)ARGC_MAX);
		err = -EINVAL;
		goto exit;
	}

	/* skip cmd name and offset */
	argc -= 2;
	argv += 2;
	for (i = 0; i < argc; i++) {
		if (parse_u8(argv[i], &buf[i])) {
			PR_ERROR(shell, "Argument %lu (%s) is not a byte.\n"
					"Bytes shall be passed in decimal"
					" notation.\n",
				 i + 1, argv[i]);
			err = -EINVAL;
			goto exit;
		}
	}

	err = do_write(shell, offset, buf, i, true);

exit:
	return err;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static int cmd_page_count(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);

	int err = check_flash_device(shell);
	size_t page_count;

	if (!err) {
		page_count = flash_get_page_count(flash_device);
		PR_SHELL(shell, "Flash device contains %lu pages.\n",
			 (unsigned long int)page_count);
	}

	return err;
}

struct page_layout_data {
	unsigned long int start_page;
	unsigned long int end_page;
	const struct shell *shell;
};

static bool page_layout_cb(const struct flash_pages_info *info, void *datav)
{
	struct page_layout_data *data = datav;
	unsigned long int sz;

	if (info->index < data->start_page) {
		return true;
	} else if (info->index > data->end_page) {
		return false;
	}

	sz = info->size;
	PR_SHELL(data->shell,
		 "\tPage %u: start 0x%08x, length 0x%lx (%lu, %lu KB)\n",
		 info->index, info->start_offset, sz, sz, sz / KB(1));
	return true;
}

static int cmd_page_layout(const struct shell *shell, size_t argc, char **argv)
{
	unsigned long int start_page, end_page;
	struct page_layout_data data;

	int err = check_flash_device(shell);

	if (err) {
		goto bail;
	}

	switch (argc) {
	case 1:
		start_page = 0;
		end_page = flash_get_page_count(flash_device) - 1;
		break;
	case 2:
		if (parse_ul(argv[1], &start_page)) {
			err = -EINVAL;
			goto bail;
		}
		end_page = flash_get_page_count(flash_device) - 1;
		break;
	case 3:
		if (parse_ul(argv[1], &start_page) ||
		    parse_ul(argv[2], &end_page)) {
			err = -EINVAL;
			goto bail;
		}
		break;
	default:
		PR_ERROR(shell, "Invalid argument count.\n");
		return -EINVAL;
	}

	data.start_page = start_page;
	data.end_page = end_page;
	data.shell = shell;
	flash_page_foreach(flash_device, page_layout_cb, &data);
	return 0;

bail:
	PR_ERROR(shell, "Invalid arguments.\n");
	return err;
}

static int cmd_page_read(const struct shell *shell, size_t argc, char **argv)
{
	unsigned long int page, offset, len;
	struct flash_pages_info info;
	int ret;

	ret = check_flash_device(shell);
	if (ret) {
		return ret;
	}

	if (argc == 3) {
		if (parse_ul(argv[1], &page) || parse_ul(argv[2], &len)) {
			ret = -EINVAL;
			goto bail;
		}
		offset = 0;
	} else if (parse_ul(argv[1], &page) || parse_ul(argv[2], &offset) ||
		   parse_ul(argv[3], &len)) {
			ret = -EINVAL;
			goto bail;
	}

	ret = flash_get_page_info_by_idx(flash_device, page, &info);
	if (ret) {
		PR_ERROR(shell, "Function flash_page_info_by_idx returned an"
				" error: %d\n", ret);
		return ret;
	}
	offset += info.start_offset;
	ret = do_read(shell, offset, len);
	return ret;

 bail:
	PR_ERROR(shell, "Invalid arguments.\n");
	return ret;
}

static int cmd_page_erase(const struct shell *shell, size_t argc, char **argv)
{
	struct flash_pages_info info;
	unsigned long int i, page, num;
	int ret;

	ret = check_flash_device(shell);
	if (ret) {
		return ret;
	}

	if (parse_ul(argv[1], &page)) {
		ret = -EINVAL;
		goto bail;
	}
	if (argc == 2) {
		num = 1;
	} else if (parse_ul(argv[2], &num)) {
		goto bail;
	}

	for (i = 0; i < num; i++) {
		ret = flash_get_page_info_by_idx(flash_device, page + i, &info);
		if (ret) {
			PR_ERROR(shell, "flash_get_page_info_by_idx error:"
				" %d\n", ret);
			return ret;
		}
		PR_SHELL(shell, "Erasing page %u (start offset 0x%x,"
				" size 0x%x)\n",
		       info.index, info.start_offset, info.size);
		ret = do_erase(shell, info.start_offset, info.size);
		if (ret) {
			return ret;
		}
	}

	return ret;

 bail:
	PR_ERROR(shell, "Invalid arguments.\n");
	return ret;
}

static int cmd_page_write(const struct shell *shell, size_t argc, char **argv)
{
	struct flash_pages_info info;
	unsigned long int page, off;
	u8_t buf[ARGC_MAX];
	size_t i;
	int ret;

	ret = check_flash_device(shell);
	if (ret) {
		return ret;
	}

	if (parse_ul(argv[1], &page) || parse_ul(argv[2], &off)) {
		ret = -EINVAL;
		goto bail;
	}

	argc -= 3;
	argv += 3;
	for (i = 0; i < argc; i++) {
		if (parse_u8(argv[i], &buf[i])) {
			PR_ERROR(shell, "Argument %d (%s) is not a byte.\n",
				 i + 2, argv[i]);
			ret = -EINVAL;
			goto bail;
		}
	}

	ret = flash_get_page_info_by_idx(flash_device, page, &info);
	if (ret) {
		PR_ERROR(shell, "flash_get_page_info_by_idx: %d\n", ret);
		return ret;
	}
	ret = do_write(shell, info.start_offset + off, buf, i, true);
	return ret;

 bail:
	PR_ERROR(shell, "Invalid arguments.\n");
	return ret;
}
#endif	/* CONFIG_FLASH_PAGE_LAYOUT */

static int cmd_set_dev(const struct shell *shell, size_t argc, char **argv)
{
	struct device *dev;
	const char *name;

	name = argv[1];

	/* Run command. */
	dev = device_get_binding(name);
	if (!dev) {
		PR_ERROR(shell, "No device named %s.\n", name);
		return -ENOEXEC;
	}
	if (flash_device) {
		PR_SHELL(shell, "Leaving behind device %s\n",
			 flash_device->config->name);
	}
	flash_device = dev;

	return 0;
}

void main(void)
{
	flash_device = device_get_binding(DT_FLASH_DEV_NAME);
	if (flash_device) {
		printk("Found flash device %s.\n", DT_FLASH_DEV_NAME);
		printk("Flash I/O commands can be run.\n");
	} else {
		printk("**No flash device found!**\n");
		printk("Run set_device <name> to specify one "
		       "before using other commands.\n");
	}
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_flash,
	/* Alphabetically sorted to ensure correct Tab autocompletion. */
	SHELL_CMD_ARG(erase,	NULL,	ERASE_HELP,	cmd_erase, 3, 0),
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	SHELL_CMD_ARG(page_count,  NULL, PAGE_COUNT_HELP, cmd_page_count, 1, 0),
	SHELL_CMD_ARG(page_erase, NULL, PAGE_ERASE_HELP, cmd_page_erase, 2, 1),
	SHELL_CMD_ARG(page_layout, NULL, PAGE_LAYOUT_HELP,
		      cmd_page_layout, 1, 2),
	SHELL_CMD_ARG(page_read,   NULL, PAGE_READ_HELP,  cmd_page_read, 3, 1),
	SHELL_CMD_ARG(page_write,  NULL, PAGE_WRITE_HELP,
		      cmd_page_write, 3, 255),
#endif
	SHELL_CMD_ARG(read,		NULL,	READ_HELP,	cmd_read, 3, 0),
	SHELL_CMD_ARG(set_device, NULL, SET_DEV_HELP, cmd_set_dev, 2, 0),
	SHELL_CMD_ARG(write,	  NULL,	WRITE_HELP,	cmd_write, 3, 255),
	SHELL_CMD_ARG(write_block_size,	NULL,	WRITE_BLOCK_SIZE_HELP,
						    cmd_write_block_size, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(flash, &sub_flash, "Flash realated commands.", cmd_flash);

