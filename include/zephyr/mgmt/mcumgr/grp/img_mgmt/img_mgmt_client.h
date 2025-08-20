/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_CLIENT_
#define H_IMG_MGMT_CLIENT_

#include <inttypes.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>

/**
 * @brief MCUmgr Image management client API
 * @defgroup mcumgr_img_mgmt_client Image Management Client
 * @ingroup mcumgr_img_mgmt
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Image list flags.
 */
struct mcumgr_image_list_flags {
	/** Bootable image */
	bool bootable: 1;
	/** Pending update state */
	bool pending: 1;
	/** Confirmed image */
	bool confirmed: 1;
	/** Active image */
	bool active: 1;
	/** Permanent image state */
	bool permanent: 1;
};

/**
 * @brief Image list data.
 */
struct mcumgr_image_data {
	/** Image slot num */
	uint32_t slot_num;
	/** Image number */
	uint32_t img_num;
	/** Image SHA256 checksum */
	char hash[IMG_MGMT_DATA_SHA_LEN];
	/** Image Version */
	char version[IMG_MGMT_VER_MAX_STR_LEN + 1];
	/** Image Flags */
	struct mcumgr_image_list_flags flags;
};

/**
 * @brief MCUmgr Image list response.
 */
struct mcumgr_image_state {
	/** Status */
	enum mcumgr_err_t status;
	/** Length of image_list */
	int image_list_length;
	/** Image list pointer */
	struct mcumgr_image_data *image_list;
};

/**
 * @brief MCUmgr Image upload response.
 */
struct mcumgr_image_upload {
	/** Status */
	enum mcumgr_err_t status;
	/** Reported image offset */
	size_t image_upload_offset;
};

/**
 * @brief IMG mgmt client upload structure
 *
 * Structure is used internally by the client
 */
struct img_gr_upload {
	/** Image 256-bit hash */
	char sha256[IMG_MGMT_DATA_SHA_LEN];
	/** True when Hash is configured, false when not */
	bool hash_initialized;
	/** Image size */
	size_t image_size;
	/** Image upload offset state */
	size_t offset;
	/** Worst case init upload message size */
	size_t upload_header_size;
	/** Image slot num */
	uint32_t image_num;
};

/**
 * @brief IMG mgmt client object.
 */
struct img_mgmt_client {
	/** SMP client object */
	struct smp_client_object *smp_client;
	/** Image Upload state data for client internal use */
	struct img_gr_upload upload;
	/** Client image list buffer size */
	int image_list_length;
	/** Image list buffer */
	struct mcumgr_image_data *image_list;
	/** Command status */
	int status;
};

/**
 * @brief Inilialize image group client.
 *
 * Function initializes image group client for given SMP client using supplied image data.
 *
 * @param client		IMG mgmt client object
 * @param smp_client		SMP client object
 * @param image_list_size	Length of image_list buffer.
 * @param image_list		Image list buffer pointer.
 *
 */
void img_mgmt_client_init(struct img_mgmt_client *client, struct smp_client_object *smp_client,
			  int image_list_size, struct mcumgr_image_data *image_list);

/**
 * @brief Initialize image upload.
 *
 * @param client	IMG mgmt client object
 * @param image_size	Size of image in bytes.
 * @param image_num	Image slot Num.
 * @param image_hash	Pointer to HASH for image must be SHA256 hash of entire upload
 *			if present (32 bytes). Use NULL when HASH from image is not available.
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
int img_mgmt_client_upload_init(struct img_mgmt_client *client, size_t image_size,
				uint32_t image_num, const char *image_hash);

/**
 * @brief Upload part of image.
 *
 * @param client	IMG mgmt client object
 * @param data		Pointer to data.
 * @param length	Length of data
 * @param res_buf	Pointer for command response structure.
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
int img_mgmt_client_upload(struct img_mgmt_client *client, const uint8_t *data, size_t length,
			   struct mcumgr_image_upload *res_buf);

/**
 * @brief Write image state.
 *
 * @param client	IMG mgmt client object
 * @param hash		Pointer to Hash (Needed for test).
 * @param confirm	Set false for test and true for confirmation.
 * @param res_buf	Pointer for command response structure.
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */

int img_mgmt_client_state_write(struct img_mgmt_client *client, char *hash, bool confirm,
				struct mcumgr_image_state *res_buf);

/**
 * @brief Read image state.
 *
 * @param client	IMG mgmt client object
 * @param res_buf	Pointer for command response structure.
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
int img_mgmt_client_state_read(struct img_mgmt_client *client, struct mcumgr_image_state *res_buf);

/**
 * @brief Erase selected Image Slot
 *
 * @param client	IMG mgmt client object
 * @param slot		Slot number
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */

int img_mgmt_client_erase(struct img_mgmt_client *client, uint32_t slot);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* H_IMG_MGMT_CLIENT_ */
