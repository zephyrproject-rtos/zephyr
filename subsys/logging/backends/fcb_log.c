/**
 * @file fcb_log.c
 * @brief This module provide FCB logging.
 *
 */
#include <stdlib.h>
#include "fcb_log.h"
#include <zephyr/fs/fcb.h>
#include "log_backend_fcb.h"
#include "fcb_priv.h" /* zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/fs/fcb) in CMakeLists.txt provides path  */
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <assert.h>

#define FCB_LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(fcb, FCB_LOG_LEVEL);

#define FCB_VERSION              1
#define FCB_SCRATCH_SECTOR_COUNT 1

// FCB sectors array, need to be inited per sector
#if (FCB_SECTOR_COUNT > 0)
static struct flash_sector fcb_instance_sectors[FCB_SECTOR_COUNT];
static uint16_t fcb_sector_count = FCB_SECTOR_COUNT;
static uint32_t fcb_spi_flash_sector_size = FCB_SPI_FLASH_SECTOR_SIZE;
#else
static struct flash_sector *fcb_instance_sectors = NULL;
static uint16_t fcb_sector_count = 0;
static uint32_t fcb_spi_flash_sector_size = 0;
#endif

struct fcb fcb_instance = {0};
K_MUTEX_DEFINE(fcb_lock);
static uint8_t fcb_flash_id = PARTITION_ID(fcb_storage);
static bool fcb_ready; // true if fcb_init_instance() succeeded

/**@brief fcb_initialised() initialises fcb_instance_sectors and seeks for oldest and newest
 * sectors. if newest sector is found then seek last element position.
 * @param[in]  none
 * @return     true if FCB region has previous logs
 */
bool fcb_initialised(void)
{
	struct fcb *fcb = &fcb_instance;
	int rc;
	bool initialised = false;
	int oldest = -1, newest = -1;
	struct flash_sector *oldest_sector = NULL, *newest_sector = NULL;
	struct fcb_entry entry_desc;
	struct fcb_disk_area fda;

	memset(fcb_instance_sectors, 0, sizeof(struct flash_sector) * fcb_sector_count);
	for (int i = 0; i < fcb_sector_count; i++) {
		fcb_instance_sectors[i].fs_off = i * fcb_spi_flash_sector_size;
		fcb_instance_sectors[i].fs_size = fcb_spi_flash_sector_size;
	}

	fcb->f_sectors = fcb_instance_sectors;
	memset(&entry_desc, 0, sizeof(entry_desc));
	entry_desc.fe_sector = NULL;

	for (int i = 0; i < fcb_sector_count; i++) {
		rc = fcb_sector_hdr_read(fcb, &fcb->f_sectors[i], &fda);
		if (rc < 0) {
			break;
		}
		if (rc == 0) {
			continue;
		}
		if (oldest < 0) {
			oldest = newest = fda.fd_id;
			oldest_sector = newest_sector = &fcb->f_sectors[i];
			continue;
		}
		if (FCB_ID_GT(fda.fd_id, newest)) {
			newest = fda.fd_id;
			newest_sector = &fcb->f_sectors[i];
		} else if (FCB_ID_GT(oldest, fda.fd_id)) {
			oldest = fda.fd_id;
			oldest_sector = &fcb->f_sectors[i];
		}
	}

	if (newest >= 0) {
		fcb->f_active.fe_sector = newest_sector;
		entry_desc.fe_sector = newest_sector;
		entry_desc.fe_data_len = 0;
		struct fcb_entry loc;
		(void)memset(&loc, 0, sizeof(loc));
		loc.fe_sector = newest_sector;
#define FCB_DISK_AREA_SIZE 8 /* size_of(struct fcb_disk_area) */
#define MIN_FCB_ELEMENT_SIZE                                                                       \
	(2 + 1 + 1) /* length(uint16), crc(uint8), 1 byte minimum data element */
#define MAX_FCB_ELEMENTS_PER_SECTOR                                                                \
	((FCB_SPI_FLASH_SECTOR_SIZE - FCB_DISK_AREA_SIZE) / MIN_FCB_ELEMENT_SIZE)
		for (int i = 0; i < MAX_FCB_ELEMENTS_PER_SECTOR; i++) {
			// read next in sector element info
			rc = fcb_getnext_nolock(fcb, &loc);
			if (rc !=
			    0) { // if not success then previous read was last element in sector
				break;
			}
		}
		fcb->f_active.fe_data_len = loc.fe_data_len;
		fcb->f_active.fe_data_off = loc.fe_data_off;
		fcb->f_active.fe_elem_off = loc.fe_elem_off;
		fcb->f_active_id = newest;

		if (newest >= 0) {
			initialised = true;
			goto out;
		}
	}
out:
	return initialised;
}

int get_fcb_spi_flash_sector_size(void)
{
	return fcb_spi_flash_sector_size;
}

bool fcb_log_ready(void)
{
	return fcb_ready;
}

size_t fcb_log_write_align(void)
{
	return (fcb_ready && fcb_instance.f_align > 0) ? fcb_instance.f_align : 1;
}

/**@brief fcb_init_instance() initialises instance of FCB.
 * Ths function initialises structures used by fcb instance.
 * In case of detection that FCB is not holding previous data the memory region is erased.
 * @param[in]  none
 * @return     0 if success
 */
int fcb_init_instance()
{
	struct fcb *fcb = &fcb_instance;
	// const struct flash_area *log_fa = NULL;
	int rc = 0;

	k_mutex_lock(&fcb_lock, K_FOREVER);
	fcb_ready = false;
	memset(fcb, 0, sizeof(*fcb));
	fcb->f_align = 4;
#if (FCB_SECTOR_COUNT < 0)
	{
		struct flash_sector fcb_sector;

		rc = flash_area_open(fcb_flash_id, &fcb->fap);
		if ((fcb->fap->fa_size == 0) || (rc != 0)) {
			LOG_ERR("flash_area_open fail");
			goto out;
		}
		int sector_count = 1; // number sectors to read
		rc = flash_area_get_sectors(fcb_flash_id, &sector_count, &fcb_sector);
		if ((fcb_sector.fs_size == 0) || (fcb->fap->fa_size == 0) || (rc != 0)) {
			LOG_ERR("[fcb_init_instance] Failed to get flash sectors\n");
			goto out;
		}
		fcb_spi_flash_sector_size = fcb_sector.fs_size;
		fcb_sector_count = fcb->fap->fa_size / fcb_sector.fs_size;
		// LOG_DBG("runtime FCB init instance Sector_size=%d size=%d count=%d",
		// fcb_spi_flash_sector_size,fcb->fap->fa_size,fcb_sector_count);
		fcb_instance_sectors = calloc(
			fcb_sector_count,
			sizeof(struct flash_sector)); // allocate RAM only once at fcb_init_instance
		if (fcb_instance_sectors == NULL) {
			return -2;
		}
	}
#endif
	// printk("fcb_sector_count %d %d storage %d pg %d\n",fcb_sector_count,FCB_SECTOR_COUNT,
	// PM_FCB_STORAGE_SIZE, FCB_SPI_FLASH_SECTOR_SIZE); printk("fcb_instance_sectors
	// %p\n",fcb_instance_sectors); assert(fcb_sector_count); // must be > 0
	// assert(fcb_instance_sectors);// must be not NULL

	// 1. Open flash area log_data_parti
	rc = flash_area_open(fcb_flash_id, &fcb->fap);
	if (rc) {
		// LOG_ERR("[fcb_init_instance] Failed to open log partition: %d", rc);
		goto out;
	}
	fcb->f_align = fcb_get_align(fcb);

	// Init the array of fcb_instance_sectors, need to init it per sector
	memset(fcb_instance_sectors, 0, sizeof(struct flash_sector) * fcb_sector_count);
	for (int i = 0; i < fcb_sector_count; i++) {
		fcb_instance_sectors[i].fs_off = i * fcb_spi_flash_sector_size;
		fcb_instance_sectors[i].fs_size = fcb_spi_flash_sector_size;
	}
	// 2. set FCB instance parameters
	fcb->f_magic = 0x1a1a1a1a;
	fcb->f_version = FCB_VERSION;
	fcb->f_sector_cnt = fcb_sector_count;
	fcb->f_scratch_cnt = FCB_SCRATCH_SECTOR_COUNT;
	fcb->f_sectors = fcb_instance_sectors;

	const struct flash_parameters *fparam = flash_get_parameters(fcb->fap->fa_dev);
	fcb->f_erase_value = fparam->erase_value;
	fcb->f_align = fcb_get_align(fcb);

	// 3. Must erase the fcb->f_sectors before fcb_init, otherwise the f_magic checking will be
	// failed!
	if (!fcb_initialised()) {
		LOG_INF("fcb !initialised-erasing");
		// Note. spi_flash_at45.c fails to erase whole FCB using flash_area_erase(fcb->fap, 0, fcb->fap->fa_size);
		//       Works with erasing sectors
		for (int i = 0; i < fcb_sector_count; i++) {
			struct flash_sector *sector = &fcb_instance_sectors[i];
			rc = flash_area_erase(fcb->fap, sector->fs_off, sector->fs_size);
			k_sleep(K_MSEC(20));
		}
		if (rc != 0) {
			LOG_ERR("fcb_init_instance erase Flash area, error %d", rc);
			flash_area_close(fcb->fap);
			goto out;
		}
	}

	rc = fcb_init(fcb->fap->fa_id, fcb);
	if (rc) {
		LOG_ERR("FCB init failed [%d]", rc);
		flash_area_close(fcb->fap);
		goto out;
	} else {
		fcb_ready = true;
		LOG_INF("FCB init OK");
	}

	flash_area_close(fcb->fap);
out:
	k_mutex_unlock(&fcb_lock);
	return rc;
}

// Append entry to FCB:
//-------------------
int fcb_append_log_entry(uint8_t *log_data, size_t len)
{
	struct fcb_entry entry_descriptor;
	int rc = k_mutex_lock(&fcb_lock, K_NO_WAIT);
	if (rc != 0) {
		return -EBUSY; /* FCB in use (init, clear, dump): entry dropped */
	}

	if (!fcb_ready) {
		rc = -ENODEV;
		goto out;
	}

	memset(&entry_descriptor, 0, sizeof(struct fcb_entry));
	rc = fcb_append(&fcb_instance, len, &entry_descriptor);
	if (rc) {
		// Do rotation in case of reaching the end of partition
		if (rc == -ENOSPC) {
			// LOG_ERR("[fcb_append_log_entry] No space left, rotating FCB and try
			// again");
			rc = fcb_rotate(&fcb_instance);
			if (rc) {
				// LOG_ERR("[fcb_append_log_entry] FCB rotate failed with error %d",
				// rc);
				goto out;
			}

			// Retry to append
			rc = fcb_append(&fcb_instance, len, &entry_descriptor);
			if (rc) {
				// LOG_ERR("[fcb_append_log_entry] FCB append retry failed after
				// rotate, error %d", rc);
				goto out;
			}
		} else {
			// LOG_ERR("[fcb_append_log_entry] Failed to append data: %d", rc);
			goto out;
		}
	}

	// Check if writing to old sector or to new one
	static const struct flash_sector *last_sector = NULL;
	if (entry_descriptor.fe_sector != last_sector) {
		for (int i = 0; i < fcb_sector_count; i++) {
			if (entry_descriptor.fe_sector == &fcb_instance_sectors[i]) {
				break;
			}
		}
		last_sector = entry_descriptor.fe_sector;
	}

	rc = flash_area_write(fcb_instance.fap, FCB_ENTRY_FA_DATA_OFF(entry_descriptor), log_data,
			      len);
	if (rc) {
		// LOG_ERR("[fcb_append_log_entry] Failed to write to FCB: %d", rc);
		/// fcb_append_finish(&fcb_instance, &entry_descriptor);
		goto out;
	}

	rc = fcb_append_finish(&fcb_instance, &entry_descriptor);
	if (rc) {
		// LOG_ERR("[fcb_append_log_entry] FCB append finish failed with error %d", rc);
		goto out;
	}
	rc = 0;
out:
	k_mutex_unlock(&fcb_lock);
	return rc;
}

// Read and print entries from a sector:
void fcb_print_Sector(uint16_t sector_no)
{
	struct fcb_entry entry_desc, first_entry_desc;
	uint8_t log_data[MAX_FCB_LOG_ENTRY_SIZE + 1];
	if (!fcb_ready || (sector_no >= fcb_instance.f_sector_cnt)) {
		return;
	}
	log_entry_t *log_entry = (log_entry_t *)log_data;
	log_entry->data = &log_data[10];
	int rc = k_mutex_lock(&fcb_lock, K_FOREVER);
	if (rc != 0) {
		return;
	}
	memset(&entry_desc, 0, sizeof(entry_desc));
	entry_desc.fe_sector = &fcb_instance_sectors[sector_no];
	// Start with the first entry in this sector
	rc = fcb_getnext(&fcb_instance, &entry_desc);
	memcpy(&first_entry_desc, &entry_desc, sizeof(struct fcb_entry));

	while (rc == 0) {
		if (entry_desc.fe_sector == first_entry_desc.fe_sector) {
			rc = flash_area_read(fcb_instance.fap, entry_desc.fe_data_off, &log_data[0],
					     entry_desc.fe_data_len);
			if (rc) {
				// LOG_ERR("[fcb_print_Sector] Failed to read entry with error %d",
				// rc);
				continue;
			} else {
				// LOG_DBG"[fcb_print_Sector] type=%d, timestamp=%08llu, log
				// '%s'",log_entry->type, log_entry->timestamp, log_entry->data);
			}

			// Get the next entry
			rc = fcb_getnext(&fcb_instance, &entry_desc);
		} else {
			break;
		}
	}
	k_mutex_unlock(&fcb_lock);
}

struct fcb *get_fcb_instance(void)
{
	return &fcb_instance;
}
