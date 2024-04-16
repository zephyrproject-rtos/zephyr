/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_FS_FCB_H_
#define ZEPHYR_INCLUDE_FS_FCB_H_

/*
 * Flash circular buffer.
 */
#include <inttypes.h>
#include <limits.h>

#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup fcb Flash Circular Buffer (FCB)
 * @ingroup file_system_storage
 * @{
 * @}
 */

/**
 * @defgroup fcb_data_structures Flash Circular Buffer Data Structures
 * @ingroup fcb
 * @{
 */

#define FCB_MAX_LEN	(CHAR_MAX | CHAR_MAX << 7) /**< Max length of element */

/**
 * @brief FCB entry info structure. This data structure describes the element
 * location in the flash.
 *
 * You would use it to figure out what parameters to pass to flash_area_read()
 * to read element contents. Or to flash_area_write() when adding a new element.
 * Entry location is pointer to area (within fcb->f_sectors), and offset
 * within that area.
 */
struct fcb_entry {
	struct flash_sector *fe_sector;
	/**< Pointer to info about sector where data are placed */

	uint32_t fe_elem_off;
	/**< Offset from the start of the sector to beginning of element. */

	uint32_t fe_data_off;
	/**< Offset from the start of the sector to the start of element. */

	uint16_t fe_data_len; /**< Size of data area in fcb entry*/
};

/**
 * @brief Helper macro for calculating the data offset related to
 * the fcb flash_area start offset.
 *
 * @param entry fcb entry structure
 */
#define FCB_ENTRY_FA_DATA_OFF(entry) (entry.fe_sector->fs_off +\
				      entry.fe_data_off)
/**
 * @brief Structure for transferring complete information about FCB entry
 * location within flash memory.
 */
struct fcb_entry_ctx {
	struct fcb_entry loc; /**< FCB entry info */
	const struct flash_area *fap;
	/**< Flash area where the entry is placed */
};

/**
 * @brief Flag to disable CRC for the fcb_entries in flash.
 */
#define FCB_FLAGS_CRC_DISABLED BIT(0)

/**
 * @brief FCB instance structure
 *
 * The following data structure describes the FCB itself. First part should
 * be filled in by the user before calling @ref fcb_init. The second part is
 * used by FCB for its internal bookkeeping.
 */
struct fcb {
	/* Caller of fcb_init fills this in */
	uint32_t f_magic;
	/**< Magic value, should not be 0xFFFFFFFF.
	 * It is xored with inversion of f_erase_value and placed in
	 * the beginning of FCB flash sector. FCB uses this when determining
	 * whether sector contains valid data or not.
	 * Giving it value of 0xFFFFFFFF means leaving bytes of the filed
	 * in "erased" state.
	 */

	uint8_t f_version; /**<  Current version number of the data */
	uint8_t f_sector_cnt; /**< Number of elements in sector array */
	uint8_t f_scratch_cnt;
	/**< Number of sectors to keep empty. This can be used if you need
	 * to have scratch space for garbage collecting when FCB fills up.
	 */

	struct flash_sector *f_sectors;
	/**< Array of sectors, must be contiguous */

	/* Flash circular buffer internal state */
	struct k_mutex f_mtx;
	/**< Locking for accessing the FCB data, internal state */

	struct flash_sector *f_oldest;
	/**< Pointer to flash sector containing the oldest data,
	 * internal state
	 */

	struct fcb_entry f_active; /**< internal state */
	uint16_t f_active_id;
	/**< Flash location where the newest data is, internal state */

	uint8_t f_align;
	/**< writes to flash have to aligned to this, internal state */

	const struct flash_area *fap;
	/**< Flash area used by the fcb instance, internal state.
	 * This can be transfer to FCB user
	 */

	uint8_t f_erase_value;
	/**< The value flash takes when it is erased. This is read from
	 * flash parameters and initialized upon call to fcb_init.
	 */
#ifdef CONFIG_FCB_ALLOW_FIXED_ENDMARKER
	const uint8_t f_flags;
	/**< Flags for configuring the FCB. */
#endif
};

/**
 * @}
 */

/**
 * @brief Flash Circular Buffer APIs
 * @defgroup fcb_api fcb API
 * @ingroup fcb
 * @{
 */

/**
 * Initialize FCB instance.
 *
 * @param[in] f_area_id ID of flash area where fcb storage resides.
 * @param[in,out] fcb   FCB instance structure.
 *
 * @return 0 on success, non-zero on failure.
 */
int fcb_init(int f_area_id, struct fcb *fcb);

/**
 * Appends an entry to circular buffer.
 *
 * When writing the
 * contents for the entry, use loc->fe_sector and loc->fe_data_off with
 * flash_area_write() to fcb flash_area.
 * When you're finished, call fcb_append_finish() with loc as argument.
 *
 * @param[in] fcb FCB instance structure.
 * @param[in] len Length of data which are expected to be written as the entry
 *            payload.
 * @param[out] loc entry location information
 *
 * @return 0 on success, non-zero on failure.
 */
int fcb_append(struct fcb *fcb, uint16_t len, struct fcb_entry *loc);

/**
 * Finishes entry append operation.
 *
 * @param[in] fcb FCB instance structure.
 * @param[in] append_loc entry location information
 *
 * @return 0 on success, non-zero on failure.
 */
int fcb_append_finish(struct fcb *fcb, struct fcb_entry *append_loc);

/**
 * FCB Walk callback function type.
 *
 * Type of function which is expected to be called while walking over fcb
 * entries thanks to a @ref fcb_walk call.
 *
 * Entry data can be read using flash_area_read(), using
 * loc_ctx fields as arguments.
 * If cb wants to stop the walk, it should return non-zero value.
 *
 * @param[in] loc_ctx entry location information (full context)
 * @param[in,out] arg callback context, transferred from @ref fcb_walk.
 *
 * @return 0 continue walking, non-zero stop walking.
 */
typedef int (*fcb_walk_cb)(struct fcb_entry_ctx *loc_ctx, void *arg);

/**
 * Walk over all entries in the FCB sector
 *
 * @param[in] sector     fcb sector to be walked. If null, traverse entire
 *                       storage.
 * @param[in] fcb        FCB instance structure.
 * @param[in] cb         pointer to the function which gets called for every
 *                       entry. If cb wants to stop the walk, it should return
 *                       non-zero value.
 * @param[in,out] cb_arg callback context, transferred to the callback
 *                       implementation.
 *
 * @return 0 on success, negative on failure (or transferred form callback
 *         return-value), positive transferred form callback return-value
 */
int fcb_walk(struct fcb *fcb, struct flash_sector *sector, fcb_walk_cb cb,
	     void *cb_arg);

/**
 * Get next fcb entry location.
 *
 * Function to obtain fcb entry location in relation to entry pointed by
 * <p> loc.
 * If loc->fe_sector is set and loc->fe_elem_off is not 0 function fetches next
 * fcb entry location.
 * If loc->fe_sector is NULL function fetches the oldest entry location within
 * FCB storage. loc->fe_sector is set and loc->fe_elem_off is 0 function fetches
 * the first entry location in the fcb sector.
 *
 * @param[in] fcb FCB instance structure.
 * @param[in,out] loc entry location information
 *
 * @return 0 on success, non-zero on failure.
 */
int fcb_getnext(struct fcb *fcb, struct fcb_entry *loc);

/**
 * Rotate fcb sectors
 *
 * Function erases the data from oldest sector. Upon that the next sector
 * becomes the oldest. Active sector is also switched if needed.
 *
 * @param[in] fcb FCB instance structure.
 */
int fcb_rotate(struct fcb *fcb);

/**
 * Start using the scratch block.
 *
 * Take one of the scratch blocks into use. So a scratch sector becomes
 * active sector to which entries can be appended.
 *
 * @param[in] fcb FCB instance structure.
 *
 * @return 0 on success, non-zero on failure.
 */
int fcb_append_to_scratch(struct fcb *fcb);

/**
 * Get free sector count.
 *
 * @param[in] fcb FCB instance structure.
 *
 * @return Number of free sectors.
 */
int fcb_free_sector_cnt(struct fcb *fcb);

/**
 * Check whether FCB has any data.
 *
 * @param[in] fcb FCB instance structure.
 *
 * @return Positive value if fcb is empty, otherwise 0.
 */
int fcb_is_empty(struct fcb *fcb);

/**
 * Finds the fcb entry that gives back up to n entries at the end.
 *
 * @param[in]               fcb FCB instance structure.
 * @param[in] entries       number of fcb entries the user wants to get
 * @param[out] last_n_entry last_n_entry the fcb_entry to be returned
 *
 * @return 0 on there are any fcbs available; -ENOENT otherwise
 */
int fcb_offset_last_n(struct fcb *fcb, uint8_t entries,
		      struct fcb_entry *last_n_entry);

/**
 * Clear fcb instance storage.
 *
 * @param[in] fcb FCB instance structure.
 *
 * @return 0 on success; non-zero on failure
 */
int fcb_clear(struct fcb *fcb);

/**
 * @}
 */

/**
 * @brief Flash Circular internal
 * @defgroup fcb_internal fcb non-API prototypes
 * @ingroup fcb
 * @{
 */

/**
 * Read raw data from the fcb flash sector.
 *
 * @param[in] fcb    FCB instance structure.
 * @param[in] sector FCB sector.
 * @param[in] off    Read offset form sector begin.
 * @param[out] dst   Destination buffer.
 * @param[in] len    Read-out size.
 *
 * @return  0 on success, negative errno code on fail.
 */
int fcb_flash_read(const struct fcb *fcb, const struct flash_sector *sector,
		   off_t off, void *dst, size_t len);

/**
 * Write raw data to the fcb flash sector.
 *
 * @param[in] fcb    FCB instance structure.
 * @param[in] sector FCB sector.
 * @param[in] off    Write offset form sector begin.
 * @param[in] src    Source buffer.
 * @param[in] len    Write size.
 *
 * @return  0 on success, negative errno code on fail.
 */
int fcb_flash_write(const struct fcb *fcb, const struct flash_sector *sector,
		    off_t off, const void *src, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FCB_H_ */
