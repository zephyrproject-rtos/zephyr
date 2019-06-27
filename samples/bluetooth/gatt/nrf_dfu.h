/** @file
 *  @brief NRF BLE DFU Service (Legacy)
 */

/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef __cplusplus
extern "C" {
#endif

/* DFU image type */
enum dfu_image_type {
	DFU_IMG_TYPE_NONE,
	DFU_IMG_TYPE_SOFT,
	DFU_IMG_TYPE_BOOT,
	DFU_IMG_TYPE_SOFTBOOT,
	DFU_IMG_TYPE_APP
};

/** @brief NRF DFU callbacks structure. */
struct nrf_dfu_cb {
	/** Attribute start callback (optional)
	 *
	 *  @param img_type   The image type to update
	 *
	 *  @return 0 on success, -errno code on error
	 */
	int (*start)(enum dfu_image_type img_type);

	/** Attribute receive callback (optional)
	 *
	 *  @param offset   image offset to update/write
	 *  @param data     pointer to img data chunk
	 *  @param len      img data chunk size
	 *
	 *  @return 0 on success, -errno code on error
	 */
	int (*receive)(u32_t offset, const u8_t *data, size_t len);

	/** Attribute validate callback (optional)
	 *
	 *  @param crc   expected updated image CRC16
	 *
	 *  @return 0 on success, -errno code on error
	 */
	int (*validate)(u16_t crc);

	/** Attribute reset_to_app callback (optional)
	 *
	 * Reset system to updated image.
	 */
	void (*reset_to_app)(void);

	/** Attribute reset_to_boot callback (optional)
	 *
	 * Reset system to bootloader mode for update.
	 * Can be NULL if already in bootloader mode.
	 */
	void (*reset_to_boot)(void);
};

/** @brief Initialize NRF DFU service.
 *
 *  Send an indication of attribute value change.
 *  Note: This function should only be called if CCC is declared with
 *  BT_GATT_CCC otherwise it cannot find a valid peer configuration.
 *
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param cb Pointer to callbacks structure.
 *  @param device_type Device type ID to match for sw update, 0 if none.
 *  @param device_revision Device revision ID to match for sw update, 0 if none.
 */

void nrf_dfu_init(struct nrf_dfu_cb *cb, u16_t device_type,
		  u16_t device_revision);

#ifdef __cplusplus
}
#endif
