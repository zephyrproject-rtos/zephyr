/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <CANopen.h>

#include <canopennode.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/crc.h>

#define LOG_LEVEL CONFIG_CANOPEN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(canopen_program);

/* Object dictionary indexes */
#define OD_H1F50_PROGRAM_DATA 0x1F50
#define OD_H1F51_PROGRAM_CTRL 0x1F51
#define OD_H1F56_PROGRAM_SWID 0x1F56
#define OD_H1F57_FLASH_STATUS 0x1F57

/* Common program control commands and status */
#define PROGRAM_CTRL_STOP           0x00
#define PROGRAM_CTRL_START          0x01
#define PROGRAM_CTRL_RESET          0x02
#define PROGRAM_CTRL_CLEAR          0x03
/* Zephyr specific program control and status */
#define PROGRAM_CTRL_ZEPHYR_CONFIRM 0x80

/* Flash status bits */
#define FLASH_STATUS_IN_PROGRESS          BIT(0)
/* Flash common error bits values */
#define FLASH_STATUS_NO_ERROR            (0U << 1U)
#define FLASH_STATUS_NO_VALID_PROGRAM    (1U << 1U)
#define FLASH_STATUS_DATA_FORMAT_UNKNOWN (2U << 1U)
#define FLASH_STATUS_DATA_FORMAT_ERROR   (3U << 1U)
#define FLASH_STATUS_FLASH_NOT_CLEARED   (4U << 1U)
#define FLASH_STATUS_FLASH_WRITE_ERROR   (5U << 1U)
#define FLASH_STATUS_GENERAL_ADDR_ERROR  (6U << 1U)
#define FLASH_STATUS_FLASH_SECURED       (7U << 1U)
#define FLASH_STATUS_UNSPECIFIED_ERROR   (63U << 1)

struct canopen_program_context {
	uint32_t flash_status;
	size_t total;
	CO_NMT_t *nmt;
	CO_EM_t *em;
	struct flash_img_context flash_img_ctx;
	uint8_t program_status;
	bool flash_written;
};

static struct canopen_program_context ctx;

static void canopen_program_set_status(uint32_t status)
{
	ctx.program_status = status;
}

static uint32_t canopen_program_get_status(void)
{
	/*
	 * Non-confirmed boot image takes precedence over other
	 * status. This must be checked on every invocation since the
	 * app may be using other means of confirming the image.
	 */
	if (!boot_is_img_confirmed()) {
		return PROGRAM_CTRL_ZEPHYR_CONFIRM;
	}

	return ctx.program_status;
}

static CO_SDO_abortCode_t canopen_odf_1f50(CO_ODF_arg_t *odf_arg)
{
	int err;

	if (odf_arg->subIndex != 1U) {
		return CO_SDO_AB_NONE;
	}

	if (odf_arg->reading) {
		return CO_SDO_AB_WRITEONLY;
	}

	if (canopen_program_get_status() != PROGRAM_CTRL_CLEAR) {
		ctx.flash_status = FLASH_STATUS_FLASH_NOT_CLEARED;
		return CO_SDO_AB_DATA_DEV_STATE;
	}

	if (odf_arg->firstSegment) {
		err = flash_img_init(&ctx.flash_img_ctx);
		if (err) {
			LOG_ERR("failed to initialize flash img (err %d)", err);
			CO_errorReport(ctx.em, CO_EM_NON_VOLATILE_MEMORY,
				       CO_EMC_HARDWARE, err);
			ctx.flash_status = FLASH_STATUS_FLASH_WRITE_ERROR;
			return CO_SDO_AB_HW;
		}
		ctx.flash_status = FLASH_STATUS_IN_PROGRESS;
		if (IS_ENABLED(CONFIG_CANOPENNODE_LEDS)) {
			canopen_leds_program_download(true);
		}
		ctx.total = odf_arg->dataLengthTotal;
		LOG_DBG("total = %d", ctx.total);
	}

	err = flash_img_buffered_write(&ctx.flash_img_ctx, odf_arg->data,
				       odf_arg->dataLength,
				       odf_arg->lastSegment);
	if (err) {
		CO_errorReport(ctx.em, CO_EM_NON_VOLATILE_MEMORY,
			       CO_EMC_HARDWARE, err);
		ctx.flash_status = FLASH_STATUS_FLASH_WRITE_ERROR;
		canopen_leds_program_download(false);
		return CO_SDO_AB_HW;
	}

	if (odf_arg->lastSegment) {
		/* ctx.total is zero if not provided by download process */
		if (ctx.total != 0 &&
		    ctx.total != flash_img_bytes_written(&ctx.flash_img_ctx)) {
			LOG_WRN("premature end of program download");
			ctx.flash_status = FLASH_STATUS_DATA_FORMAT_ERROR;
		} else {
			LOG_DBG("program downloaded");
			ctx.flash_written = true;
			ctx.flash_status = FLASH_STATUS_NO_ERROR;
		}

		canopen_program_set_status(PROGRAM_CTRL_STOP);
		canopen_leds_program_download(false);
	}

	return CO_SDO_AB_NONE;
}

static inline CO_SDO_abortCode_t canopen_program_cmd_stop(void)
{
	if (canopen_program_get_status() == PROGRAM_CTRL_ZEPHYR_CONFIRM) {
		return CO_SDO_AB_DATA_DEV_STATE;
	}

	LOG_DBG("program stopped");
	canopen_program_set_status(PROGRAM_CTRL_STOP);

	return CO_SDO_AB_NONE;
}

static inline CO_SDO_abortCode_t canopen_program_cmd_start(void)
{
	int err;

	if (canopen_program_get_status() == PROGRAM_CTRL_ZEPHYR_CONFIRM) {
		return CO_SDO_AB_DATA_DEV_STATE;
	}

	if (ctx.flash_written) {
		LOG_DBG("requesting upgrade and reset");

		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err) {
			LOG_ERR("failed to request upgrade (err %d)", err);
			CO_errorReport(ctx.em, CO_EM_NON_VOLATILE_MEMORY,
				       CO_EMC_HARDWARE, err);
			return CO_SDO_AB_HW;
		}

		ctx.nmt->resetCommand = CO_RESET_APP;
	} else {
		LOG_DBG("program started");
		canopen_program_set_status(PROGRAM_CTRL_START);
	}

	return CO_SDO_AB_NONE;
}

static inline CO_SDO_abortCode_t canopen_program_cmd_clear(void)
{
	int err;

	if (canopen_program_get_status() != PROGRAM_CTRL_STOP) {
		return CO_SDO_AB_DATA_DEV_STATE;
	}

	if (!IS_ENABLED(CONFIG_IMG_ERASE_PROGRESSIVELY)) {
		LOG_DBG("erasing flash area");

		err = boot_erase_img_bank(FIXED_PARTITION_ID(slot1_partition));
		if (err) {
			LOG_ERR("failed to erase image bank (err %d)", err);
			CO_errorReport(ctx.em, CO_EM_NON_VOLATILE_MEMORY,
				       CO_EMC_HARDWARE, err);
			return CO_SDO_AB_HW;
		}
	}

	LOG_DBG("program cleared");
	canopen_program_set_status(PROGRAM_CTRL_CLEAR);
	ctx.flash_status = FLASH_STATUS_NO_ERROR;
	ctx.flash_written = false;

	return CO_SDO_AB_NONE;
}

static inline CO_SDO_abortCode_t canopen_program_cmd_confirm(void)
{
	int err;

	if (canopen_program_get_status() == PROGRAM_CTRL_ZEPHYR_CONFIRM) {
		err = boot_write_img_confirmed();
		if (err) {
			LOG_ERR("failed to confirm image (err %d)", err);
			CO_errorReport(ctx.em, CO_EM_NON_VOLATILE_MEMORY,
				       CO_EMC_HARDWARE, err);
			return CO_SDO_AB_HW;
		}

		LOG_DBG("program confirmed");
		canopen_program_set_status(PROGRAM_CTRL_START);
	}

	return CO_SDO_AB_NONE;
}

static CO_SDO_abortCode_t canopen_odf_1f51(CO_ODF_arg_t *odf_arg)
{
	CO_SDO_abortCode_t ab;
	uint8_t cmd;

	if (odf_arg->subIndex != 1U) {
		return CO_SDO_AB_NONE;
	}

	if (odf_arg->reading) {
		odf_arg->data[0] = canopen_program_get_status();
		return CO_SDO_AB_NONE;
	}

	if (CO_NMT_getInternalState(ctx.nmt) != CO_NMT_PRE_OPERATIONAL) {
		LOG_DBG("not in pre-operational state");
		return CO_SDO_AB_DATA_DEV_STATE;
	}

	/* Preserve old value */
	cmd = odf_arg->data[0];
	memcpy(odf_arg->data, odf_arg->ODdataStorage, sizeof(uint8_t));

	LOG_DBG("program status = %d, cmd = %d", canopen_program_get_status(),
		cmd);

	switch (cmd) {
	case PROGRAM_CTRL_STOP:
		ab = canopen_program_cmd_stop();
		break;
	case PROGRAM_CTRL_START:
		ab = canopen_program_cmd_start();
		break;
	case PROGRAM_CTRL_CLEAR:
		ab = canopen_program_cmd_clear();
		break;
	case PROGRAM_CTRL_ZEPHYR_CONFIRM:
		ab = canopen_program_cmd_confirm();
		break;
	case PROGRAM_CTRL_RESET:
		__fallthrough;
	default:
		LOG_DBG("unsupported command '%d'", cmd);
		ab = CO_SDO_AB_INVALID_VALUE;
	}

	return ab;
}

#ifdef CONFIG_BOOTLOADER_MCUBOOT
/** @brief Calculate crc for region in flash
 *
 * @param flash_area Flash area to read from, must be open
 * @offset Offset to read from
 * @size Number of bytes to include in calculation
 * @pcrc Pointer to uint32_t where crc will be written if return value is 0
 *
 * @return 0 if successful, negative errno on failure
 */
static int flash_crc(const struct flash_area *flash_area,
		k_off_t offset, size_t size, uint32_t *pcrc)
{
	uint32_t crc = 0;
	uint8_t buffer[32];

	while (size > 0) {
		size_t len = MIN(size, sizeof(buffer));

		int err = flash_area_read(flash_area, offset, buffer, len);

		if (err) {
			return err;
		}

		crc = crc32_ieee_update(crc, buffer, len);

		offset += len;
		size -= len;
	}

	*pcrc = crc;

	return 0;
}

static CO_SDO_abortCode_t canopen_odf_1f56(CO_ODF_arg_t *odf_arg)
{
	const struct flash_area *flash_area;
	struct mcuboot_img_header header;
	k_off_t offset = 0;
	uint32_t crc = 0;
	uint8_t fa_id;
	uint32_t len;
	int err;

	if (odf_arg->subIndex != 1U) {
		return CO_SDO_AB_NONE;
	}

	if (!odf_arg->reading) {
		/* Preserve old value */
		memcpy(odf_arg->data, odf_arg->ODdataStorage, sizeof(uint32_t));
		return CO_SDO_AB_READONLY;
	}

	/* Reading from flash and calculating crc can take 100ms or more, and
	 * this function is called with the can od lock taken.
	 *
	 * Release the lock before performing time consuming work, and reacquire
	 * before return.
	 */
	CO_UNLOCK_OD();

	/*
	 * Calculate the CRC32 of the image that is running or will be
	 * started upon receiveing the next 'start' command.
	 */
	if (ctx.flash_written) {
		fa_id = FIXED_PARTITION_ID(slot1_partition);
	} else {
		fa_id = FIXED_PARTITION_ID(slot0_partition);
	}

	err = boot_read_bank_header(fa_id, &header, sizeof(header));
	if (err) {
		LOG_WRN("failed to read bank header (err %d)", err);
		CO_setUint32(odf_arg->data, 0U);

		CO_LOCK_OD();
		return CO_SDO_AB_NONE;
	}

	if (header.mcuboot_version != 1) {
		LOG_WRN("unsupported mcuboot header version %d",
			header.mcuboot_version);
		CO_setUint32(odf_arg->data, 0U);

		CO_LOCK_OD();
		return CO_SDO_AB_NONE;
	}
	len = header.h.v1.image_size;

	err = flash_area_open(fa_id, &flash_area);
	if (err) {
		LOG_ERR("failed to open flash area (err %d)", err);
		CO_errorReport(ctx.em, CO_EM_NON_VOLATILE_MEMORY,
			       CO_EMC_HARDWARE, err);

		CO_LOCK_OD();
		return CO_SDO_AB_HW;
	}

	err = flash_crc(flash_area, offset, len, &crc);

	flash_area_close(flash_area);

	if (err) {
		LOG_ERR("failed to read flash (err %d)", err);
		CO_errorReport(ctx.em, CO_EM_NON_VOLATILE_MEMORY,
			       CO_EMC_HARDWARE, err);

		CO_LOCK_OD();
		return CO_SDO_AB_HW;
	}

	CO_setUint32(odf_arg->data, crc);

	CO_LOCK_OD();
	return CO_SDO_AB_NONE;
}
#endif /* CONFIG_BOOTLOADER_MCUBOOT */

static CO_SDO_abortCode_t canopen_odf_1f57(CO_ODF_arg_t *odf_arg)
{
	if (odf_arg->subIndex != 1U) {
		return CO_SDO_AB_NONE;
	}

	if (!odf_arg->reading) {
		/* Preserve old value */
		memcpy(odf_arg->data, odf_arg->ODdataStorage, sizeof(uint32_t));
		return CO_SDO_AB_READONLY;
	}

	CO_setUint32(odf_arg->data, ctx.flash_status);

	return CO_SDO_AB_NONE;
}

void canopen_program_download_attach(CO_NMT_t *nmt, CO_SDO_t *sdo, CO_EM_t *em)
{
	canopen_program_set_status(PROGRAM_CTRL_START);
	ctx.flash_status = FLASH_STATUS_NO_ERROR;
	ctx.flash_written = false;
	ctx.nmt = nmt;
	ctx.em = em;

	CO_OD_configure(sdo, OD_H1F50_PROGRAM_DATA, canopen_odf_1f50,
			NULL, 0U, 0U);

	CO_OD_configure(sdo, OD_H1F51_PROGRAM_CTRL, canopen_odf_1f51,
			NULL, 0U, 0U);

	if (IS_ENABLED(CONFIG_BOOTLOADER_MCUBOOT)) {
		CO_OD_configure(sdo, OD_H1F56_PROGRAM_SWID, canopen_odf_1f56,
				NULL, 0U, 0U);
	}

	CO_OD_configure(sdo, OD_H1F57_FLASH_STATUS, canopen_odf_1f57,
			NULL, 0U, 0U);
}
