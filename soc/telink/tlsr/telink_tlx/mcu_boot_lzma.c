/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bootutil/bootutil.h"
#include "bootutil_priv.h"
#include "bootutil/bootutil_log.h"
#include "liblzma/api/lzma.h"

BOOT_LOG_MODULE_DECLARE(mcuboot);

#if BOOT_MAX_ALIGN > 1024
#define BUF_SZ BOOT_MAX_ALIGN
#else
#define BUF_SZ 1024
#endif

/**
 * Hook function to perform the image update, including LZMA decompression.
 *
 * @param img_index   Index of the current image.
 * @param img_head    Pointer to the image header of the secondary slot.
 * @param area        Pointer to the flash area of the secondary slot.
 *
 * @return            0 on success; nonzero on failure.
 */
int boot_perform_update_hook(int img_index, struct image_header *img_head,
			     const struct flash_area *area)
{
	int rc;
	const struct flash_area *fap_primary_slot;
	/* image size to be processed (without trailer) */
	uint32_t img_size = img_head->ih_hdr_size + img_head->ih_img_size;

	static uint8_t buf_in[BUF_SZ] __aligned(4);
	static uint8_t buf_out[BUF_SZ] __aligned(4);
	int chunk_sz_in;
	int chunk_sz_out = sizeof(buf_out);
	uint32_t bytes_copied = 0;
	uint32_t bytes_written = 0;
	lzma_ret rc_lzma;

	static lzma_options_lzma options;

	static lzma_filter filters[2];
	static lzma_stream strm = LZMA_STREAM_INIT;

	BOOT_LOG_INF("Image upgrade compressed secondary slot -> primary slot");

	while (bytes_copied < img_size) {
		if (img_size - bytes_copied > sizeof(buf_in)) {
			chunk_sz_in = sizeof(buf_in);
		} else {
			chunk_sz_in = img_size - bytes_copied;
		}

		rc = flash_area_read(area, bytes_copied, buf_in, chunk_sz_in);
		assert(rc == 0);

		if (bytes_copied == 0) {
			/* Set up the options (lc, lp, pb, dictionary size) and LZMA filter chain */
			options.lc = 1; /* Literal context bits */
			options.lp = 2; /* Literal position bits */
			options.pb = 0; /* Position bits */
			options.dict_size = CONFIG_COMPRESS_LZMA_DICTIONARY_SIZE;
			filters[0].id = LZMA_FILTER_LZMA1;
			filters[0].options = &options;
			filters[1].id = LZMA_VLI_UNKNOWN;

			/* Initialize the raw decoder */
			rc_lzma = lzma_raw_decoder(&strm, filters);
			if (rc_lzma != LZMA_OK) {
				BOOT_LOG_ERR("Failed to init LZMA raw decoder: %d", rc_lzma);
				return rc_lzma;
			}

			/* Skip image header (compressed data contains its own) */
			strm.avail_in = chunk_sz_in - img_head->ih_hdr_size;
			strm.next_in = buf_in + img_head->ih_hdr_size;
		} else {
			strm.avail_in = chunk_sz_in;
			strm.next_in = buf_in;
		}

		while (strm.avail_in > 0) {
			/* Set up the output buffer and run the LZMA decompression for this chunk */
			strm.avail_out = chunk_sz_out;
			strm.next_out = buf_out;
			rc_lzma = lzma_code(&strm, LZMA_RUN);
			if (rc_lzma != LZMA_OK && rc_lzma != LZMA_STREAM_END) {
				BOOT_LOG_ERR("Decompression error: %d", rc_lzma);
				return rc_lzma;
			}

			if (bytes_copied == 0 && bytes_written == 0) {
				BOOT_LOG_INF("Erasing the primary slot");
				rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(img_index),
						     &fap_primary_slot);
				assert(rc == 0);
				/* Erase primiry slot (don't need to erase trailer separetely) */
				rc = boot_erase_region(fap_primary_slot, 0,
						       flash_area_get_size(fap_primary_slot));
				assert(rc == 0);
			}

			/* Calculate write size and write data into flash */
			size_t write_size = chunk_sz_out - strm.avail_out;

			BOOT_LOG_INF("Decompressed chunk size = %zu bytes "
				     "(remaining compressed size = %zu bytes)",
				     write_size, strm.avail_in);
			rc = flash_area_write(fap_primary_slot, bytes_written, buf_out, write_size);
			assert(rc == 0);

			bytes_written += write_size;

			if (rc_lzma == LZMA_STREAM_END) {
				break;
			}
		}

		bytes_copied += chunk_sz_in;
		BOOT_LOG_INF("Processed %zu from %zu compressed bytes "
			     "into %zu decompressed bytes",
			     bytes_copied, img_size, bytes_written);

		if (rc_lzma == LZMA_STREAM_END) {
			lzma_end(&strm);
			break;
		}
	}

	/* Clean up: erase the header and trailer of the secondary slot */
	BOOT_LOG_DBG("Erasing secondary header");
	rc = boot_erase_region(area, 0, img_head->ih_hdr_size);
	assert(rc == 0);

	BOOT_LOG_DBG("Erasing secondary trailer");
	rc = boot_erase_region(area, img_size, area->fa_size - img_size);
	assert(rc == 0);

	return 0;
}

/* Placeholder hooks added to ensure successful compilation. */

int boot_read_image_header_hook(int img_index, int slot, struct image_header *img_hed)
{
	return BOOT_HOOK_REGULAR;
}

fih_ret boot_image_check_hook(int img_index, int slot)
{
	FIH_RET(FIH_BOOT_HOOK_REGULAR);
}

int boot_read_swap_state_primary_slot_hook(int image_index, struct boot_swap_state *state)
{
	return BOOT_HOOK_REGULAR;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area, size_t size)
{
	return 0;
}

/* Modified version of the original function with the check for equal slot sizes removed. */

/*
 * Slots are compatible when all sectors that store up to size of the image
 * round up to sector size, in both slot's are able to fit in the scratch
 * area, and have sizes that are a multiple of each other (powers of two
 * presumably!).
 */
int __wrap_boot_slots_compatible(struct boot_loader_state *state)
{
	size_t num_sectors_primary;
	size_t num_sectors_secondary;
	size_t sz0, sz1;
	size_t primary_slot_sz, secondary_slot_sz;
	size_t i, j;
	int8_t smaller;

	num_sectors_primary = boot_img_num_sectors(state, BOOT_PRIMARY_SLOT);
	num_sectors_secondary = boot_img_num_sectors(state, BOOT_SECONDARY_SLOT);
	if ((num_sectors_primary > BOOT_MAX_IMG_SECTORS) ||
	    (num_sectors_secondary > BOOT_MAX_IMG_SECTORS)) {
		BOOT_LOG_WRN("Cannot upgrade: more sectors than allowed");
		return 0;
	}

	/*
	 * The following loop scans all sectors in a linear fashion, assuring that
	 * for each possible sector in each slot, it is able to fit in the other
	 * slot's sector or sectors. Slot's should be compatible as long as any
	 * number of a slot's sectors are able to fit into another, which only
	 * excludes cases where sector sizes are not a multiple of each other.
	 */
	i = sz0 = primary_slot_sz = 0;
	j = sz1 = secondary_slot_sz = 0;
	smaller = 0;
	while (i < num_sectors_primary || j < num_sectors_secondary) {
		if (sz0 == sz1) {
			sz0 += boot_img_sector_size(state, BOOT_PRIMARY_SLOT, i);
			sz1 += boot_img_sector_size(state, BOOT_SECONDARY_SLOT, j);
			i++;
			j++;
		} else if (sz0 < sz1) {
			sz0 += boot_img_sector_size(state, BOOT_PRIMARY_SLOT, i);
			/* Guarantee that multiple sectors of the secondary slot
			 * fit into the primary slot.
			 */
			if (smaller == 2) {
				BOOT_LOG_WRN("Cannot upgrade: slots have non-compatible sectors");
				return 0;
			}
			smaller = 1;
			i++;
		} else {
			sz1 += boot_img_sector_size(state, BOOT_SECONDARY_SLOT, j);
			/* Guarantee that multiple sectors of the primary slot
			 * fit into the secondary slot.
			 */
			if (smaller == 1) {
				BOOT_LOG_WRN("Cannot upgrade: slots have non-compatible sectors");
				return 0;
			}
			smaller = 2;
			j++;
		}
	}

	return 1;
}
