/*
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/settings/settings.h>
#if CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_msc.h>
#endif
#include <zephyr/fs/fs.h>
#include <app_version.h>
#if CONFIG_LLEXT
#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/llext/file_loader.h>
#endif


#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);


#if CONFIG_DISK_DRIVER_FLASH
#include <zephyr/storage/flash_map.h>
#endif

#if CONFIG_FAT_FILESYSTEM_ELM
#include <ff.h>
#endif

#if CONFIG_FILE_SYSTEM_LITTLEFS
#include <zephyr/fs/littlefs.h>
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
#endif


#define CAN_INTERFACE    DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
#define CAN_BITRATE    (DT_PROP(DT_CHOSEN(zephyr_canbus), bus_speed) / 1000)


#define STORAGE_PARTITION       storage_partition
#define STORAGE_PARTITION_ID    FIXED_PARTITION_ID(STORAGE_PARTITION)


static struct fs_mount_t fs_mnt;

static int setup_flash(struct fs_mount_t *mnt)
{
	int rc = 0;
#if CONFIG_DISK_DRIVER_FLASH
	unsigned int id;
	const struct flash_area *pfa;

	mnt->storage_dev = (void *)STORAGE_PARTITION_ID;
	id = STORAGE_PARTITION_ID;

	rc = flash_area_open(id, &pfa);
	printk("Area %u at 0x%x on %s for %u bytes\n",
		   id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
		   (unsigned int)pfa->fa_size);

	if (rc < 0 && IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		printk("Erasing flash area ... ");
		rc = flash_area_erase(pfa, 0, pfa->fa_size);
		printk("%d\n", rc);
	}

	if (rc < 0) {
		flash_area_close(pfa);
	}
#endif
	return rc;
}

static int mount_app_fs(struct fs_mount_t *mnt)
{
	int rc;

#if CONFIG_FAT_FILESYSTEM_ELM
	static FATFS fat_fs;

	mnt->type = FS_FATFS;
	mnt->fs_data = &fat_fs;
	if (IS_ENABLED(CONFIG_DISK_DRIVER_RAM)) {
		mnt->mnt_point = "/RAM:";
	} else if (IS_ENABLED(CONFIG_DISK_DRIVER_SDMMC)) {
		mnt->mnt_point = "/SD:";
	} else {
		mnt->mnt_point = "/NAND:";
	}

#elif CONFIG_FILE_SYSTEM_LITTLEFS
	mnt->type = FS_LITTLEFS;
	mnt->mnt_point = "/lfs";
	mnt->fs_data = &storage;
#endif
	rc = fs_mount(mnt);

	return rc;
}

static void setup_disk(void)
{
	struct fs_mount_t *mp = &fs_mnt;
	struct fs_dir_t dir;
	struct fs_statvfs sbuf;
	int rc;

	fs_dir_t_init(&dir);

	if (IS_ENABLED(CONFIG_DISK_DRIVER_FLASH)) {
		rc = setup_flash(mp);
		if (rc < 0) {
			LOG_ERR("Failed to setup flash area");
			return;
		}
	}

	if (!IS_ENABLED(CONFIG_FILE_SYSTEM_LITTLEFS) &&
		!IS_ENABLED(CONFIG_FAT_FILESYSTEM_ELM)) {
		LOG_INF("No file system selected");
		return;
	}

	rc = mount_app_fs(mp);
	if (rc < 0) {
		LOG_ERR("Failed to mount filesystem");
		return;
	}

	/* Allow log messages to flush to avoid interleaved output */
	k_sleep(K_MSEC(50));

	printk("Mount %s: %d\n", fs_mnt.mnt_point, rc);

	rc = fs_statvfs(mp->mnt_point, &sbuf);
	if (rc < 0) {
		printk("FAIL: statvfs: %d\n", rc);
		return;
	}

	printk("%s: bsize = %lu ; frsize = %lu ;"
		   " blocks = %lu ; bfree = %lu\n",
		   mp->mnt_point,
		   sbuf.f_bsize, sbuf.f_frsize,
		   sbuf.f_blocks, sbuf.f_bfree);

	rc = fs_opendir(&dir, mp->mnt_point);
	printk("%s opendir: %d\n", mp->mnt_point, rc);

	if (rc < 0) {
		LOG_ERR("Failed to open directory");
	}

	while (rc >= 0) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		if (rc < 0) {
			LOG_ERR("Failed to read directory entries");
			break;
		}
		if (ent.name[0] == 0) {
			printk("End of files\n");
			break;
		}
		printk("  %c %u %s\n",
			   (ent.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D',
			   ent.size,
			   ent.name);
	}

	(void)fs_closedir(&dir);
}

/**
 * find all elf file and load it
 */
void load_extension(void)
{
	struct fs_file_t fd;
	char extension_names[2][16] = {"hello_world","hello_world_pic"};
	char extension_paths[2][64] = {"/NAND:/hello_world.llext","/NAND:/hello_world_pic.llext"};

	for (int i=0; i<2; i++) {
		char * extension_name = extension_names[i];
		char * extension_path = extension_paths[i];
		LOG_INF("Loading extension %s %s", extension_name, extension_path);

		fs_file_t_init(&fd);
		int rc = fs_open(&fd, extension_path, FS_O_READ);
		if(rc < 0) {
			LOG_ERR("%d extension not found: %s", rc, extension_path);
		} else {
			struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
			struct llext_file_loader file_stream = LLEXT_FILE_LOADER(&fd);
			struct llext_loader *stream = &file_stream.loader;

			struct llext *m;
			int res = llext_load(stream, extension_name, &m, &ldr_parm);

			fs_close(&fd);

			if (res == 0) {

				LOG_INF("Successfully loaded extension %s @ %p", m->name, m->mem[LLEXT_MEM_TEXT]);
				// start extension
				LOG_INF("Starting extension %s", m->name);
				int rc = llext_call_fn(m, "start");
				if(rc != 0)
				{
					LOG_ERR("Failed to start extension %s", m->name);
				}
			} else {
				LOG_ERR("Failed to load extension %s, return code %d", extension_name, res);
			}
		}
	}
}



/**
 * @brief Main application entry point.
 *
 */
int main(void)
{
	int ret;

	printf("\n\t\t\x1B[36m====== LLEXT fs_loader sample %s ======\x1B[0m", APP_VERSION_STRING);

	setup_disk();

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	ret = enable_usb_device_next();
#else
	ret = usb_enable(NULL);
#endif
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}
#if defined(CONFIG_USB_MASS_STORAGE)
	LOG_INF("The device is put in USB mass storage mode.");
#endif

	LOG_INF("End of boot");

#ifdef CONFIG_LLEXT
	LOG_INF("Start extension loading");
	load_extension();
	LOG_INF("End of extension loading");
#endif

	return 0;
}
