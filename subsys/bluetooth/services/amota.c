/** @file
 *  @brief AMOTA Service
 */

/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/amota.h>

#define LOG_LEVEL CONFIG_BT_AMOTA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(amota);

#include "am_mcu_apollo.h"
#include "am_util_bootloader.h"
#include "am_util_multi_boot.h"
#include "am_hal_security.h"

/* If AM_HAL_FLASH_PAGE_SIZE is not defined (e.g., not set as compile definition),
 * define it based on the chip type
 */
#ifndef AM_HAL_FLASH_PAGE_SIZE
#if defined(AM_PART_APOLLO510) || defined(AM_PART_APOLLO4P)
#define AM_HAL_FLASH_PAGE_SIZE 1024 /* MRAM uses 1KB pages */
#else
#define AM_HAL_FLASH_PAGE_SIZE 8192 /* Default flash page size */
#endif
#endif

/* ****************************************************************************
 *
 * Define the OTA Descriptor address by reserving default maximum 512k bytes app
 * image/upgrading image size. Allocate AM_HAL_FLASH_PAGE_SIZE bytes for the OTA
 * Descriptor. OTA Descriptor needs to be made sure no overlaps with the current
 * running app image and the upgrading image storage area:
 * - OTA_POINTER_LOCATION should be larger than app start address + app image size
 * - OTA_POINTER_LOCATION should be larger than app start address + upgrading image size
 * The app start address is corresponding to MCU_MRAM start address of app example
 * in linker_script file, which is:
 * - 0x00018000 for Apollo4P by default.
 * - 0x00410000 for Apollo510 and Apollo330P_510L by default.
 * ****************************************************************************
 */
#define OTA_MAX_IMAGE_SIZE  (512 * 1024)
#define OTA_DESCRIPTOR_SIZE AM_HAL_FLASH_PAGE_SIZE
#if defined(AM_PART_APOLLO4P)
#if defined(AM_HAL_MRAM_TOTAL_SIZE) && (OTA_MAX_IMAGE_SIZE > (AM_HAL_MRAM_TOTAL_SIZE - 0x18000) / 2)
#error "OTA_MAX_IMAGE_SIZE is too large!"
#endif
#define OTA_POINTER_LOCATION        (0x00018000 + OTA_MAX_IMAGE_SIZE)
#define AMOTA_INT_FLASH_OTA_ADDRESS (OTA_POINTER_LOCATION + OTA_DESCRIPTOR_SIZE)
#elif defined(AM_PART_APOLLO510)
#if defined(AM_HAL_MRAM_TOTAL_SIZE) && (OTA_MAX_IMAGE_SIZE > (AM_HAL_MRAM_TOTAL_SIZE - 0x10000) / 2)
#error "OTA_MAX_IMAGE_SIZE is too large!"
#endif
#define OTA_POINTER_LOCATION        (0x00410000 + OTA_MAX_IMAGE_SIZE)
#define AMOTA_INT_FLASH_OTA_ADDRESS (OTA_POINTER_LOCATION + OTA_DESCRIPTOR_SIZE)
#else
#define OTA_POINTER_LOCATION        0x4C000
#define AMOTA_INT_FLASH_OTA_ADDRESS 0x00050000
#endif

/* User specified maximum size of OTA storage area.
 * Make sure the size is flash page multiple.
 * (Default value is determined based on rest of flash from the start).
 */
#define AMOTA_INT_FLASH_OTA_MAX_SIZE                                                               \
	(AM_HAL_MRAM_LARGEST_VALID_ADDR - AMOTA_INT_FLASH_OTA_ADDRESS + 1)

/* Protection against NULL pointer */
#define FLASH_OPERATE(pFlash, func) ((pFlash)->func ? (pFlash)->func() : 0)

/* Page size of OTA data writing */
#define AMOTA_WRITE_PAGE_SIZE 1024

/* Data structure for flash operation */
typedef struct {
	uint8_t writeBuffer[AMOTA_WRITE_PAGE_SIZE] __aligned(4);
	uint16_t bufferIndex;
} amotasFlashOp_t;

amotasFlashOp_t amotasFlash = {
	.bufferIndex = 0,
};

/* Temporary scratch buffer used to read from flash */
uint32_t amotasTmpBuf[AMOTA_PACKET_SIZE / 4];

struct bt_amota amota;
static struct bt_gatt_attr *amota_notify_ch;
static am_util_multiboot_flash_info_t *g_pFlash = &g_intFlash;

/* CRC-32 table for OTA data CRC calculation */
static const uint32_t crc32Table[256] = {
	0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU, 0x076DC419U, 0x706AF48FU, 0xE963A535U,
	0x9E6495A3U, 0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91EU, 0x97D2D988U, 0x09B64C2BU, 0x7EB17CBDU,
	0xE7B82D07U, 0x90BF1D91U, 0x1DB71064U, 0x6AB020F2U, 0xF3B97148U, 0x84BE41DEU, 0x1ADAD47DU,
	0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U, 0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU,
	0x14015C4FU, 0x63066CD9U, 0xFA0F3D63U, 0x8D080DF5U, 0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U,
	0xA2677172U, 0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU, 0x35B5A8FAU, 0x42B2986CU,
	0xDBBBC9D6U, 0xACBCF940U, 0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U, 0x26D930ACU,
	0x51DE003AU, 0xC8D75180U, 0xBFD06116U, 0x21B4F4B5U, 0x56B3C423U, 0xCFBA9599U, 0xB8BDA50FU,
	0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U, 0x2F6F7C87U, 0x58684C11U, 0xC1611DABU,
	0xB6662D3DU, 0x76DC4190U, 0x01DB7106U, 0x98D220BCU, 0xEFD5102AU, 0x71B18589U, 0x06B6B51FU,
	0x9FBFE4A5U, 0xE8B8D433U, 0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U, 0x7F6A0DBBU,
	0x086D3D2DU, 0x91646C97U, 0xE6635C01U, 0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU,
	0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U, 0x65B0D9C6U, 0x12B7E950U, 0x8BBEB8EAU,
	0xFCB9887CU, 0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U, 0x4DB26158U, 0x3AB551CEU,
	0xA3BC0074U, 0xD4BB30E2U, 0x4ADFA541U, 0x3DD895D7U, 0xA4D1C46DU, 0xD3D6F4FBU, 0x4369E96AU,
	0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U, 0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U,
	0x5005713CU, 0x270241AAU, 0xBE0B1010U, 0xC90C2086U, 0x5768B525U, 0x206F85B3U, 0xB966D409U,
	0xCE61E49FU, 0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U, 0x59B33D17U, 0x2EB40D81U,
	0xB7BD5C3BU, 0xC0BA6CADU, 0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU, 0xEAD54739U,
	0x9DD277AFU, 0x04DB2615U, 0x73DC1683U, 0xE3630B12U, 0x94643B84U, 0x0D6D6A3EU, 0x7A6A5AA8U,
	0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U, 0xF00F9344U, 0x8708A3D2U, 0x1E01F268U,
	0x6906C2FEU, 0xF762575DU, 0x806567CBU, 0x196C3671U, 0x6E6B06E7U, 0xFED41B76U, 0x89D32BE0U,
	0x10DA7A5AU, 0x67DD4ACCU, 0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U, 0xD6D6A3E8U,
	0xA1D1937EU, 0x38D8C2C4U, 0x4FDFF252U, 0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU,
	0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U, 0xDF60EFC3U, 0xA867DF55U, 0x316E8EEFU,
	0x4669BE79U, 0xCB61B38CU, 0xBC66831AU, 0x256FD2A0U, 0x5268E236U, 0xCC0C7795U, 0xBB0B4703U,
	0x220216B9U, 0x5505262FU, 0xC5BA3BBEU, 0xB2BD0B28U, 0x2BB45A92U, 0x5CB36A04U, 0xC2D7FFA7U,
	0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU, 0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU,
	0x9C0906A9U, 0xEB0E363FU, 0x72076785U, 0x05005713U, 0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU,
	0x0CB61B38U, 0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U, 0x86D3D2D4U, 0xF1D4E242U,
	0x68DDB3F8U, 0x1FDA836EU, 0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U, 0x88085AE6U,
	0xFF0F6A70U, 0x66063BCAU, 0x11010B5CU, 0x8F659EFFU, 0xF862AE69U, 0x616BFFD3U, 0x166CCF45U,
	0xA00AE278U, 0xD70DD2EEU, 0x4E048354U, 0x3903B3C2U, 0xA7672661U, 0xD06016F7U, 0x4969474DU,
	0x3E6E77DBU, 0xAED16A4AU, 0xD9D65ADCU, 0x40DF0B66U, 0x37D83BF0U, 0xA9BCAE53U, 0xDEBB9EC5U,
	0x47B2CF7FU, 0x30B5FFE9U, 0xBDBDF21CU, 0xCABAC28AU, 0x53B39330U, 0x24B4A3A6U, 0xBAD03605U,
	0xCDD70693U, 0x54DE5729U, 0x23D967BFU, 0xB3667A2EU, 0xC4614AB8U, 0x5D681B02U, 0x2A6F2B94U,
	0xB40BBE37U, 0xC30C8EA1U, 0x5A05DF1BU, 0x2D02EF8DU};

static uint32_t CalcCrc32(uint32_t crcInit, uint32_t len, uint8_t *pBuf)
{
	uint32_t crc = crcInit;

	while (len > 0) {
		crc = crc32Table[*pBuf ^ (uint8_t)crc] ^ (crc >> 8);
		pBuf++;
		len--;
	}

	crc = crc ^ 0xFFFFFFFFU;

	return crc;
}

static void erase_flash(uint32_t ui32Addr, uint32_t ui32NumBytes)
{
	while (ui32NumBytes) {
		g_pFlash->flash_erase_sector(ui32Addr);
		if (ui32NumBytes > g_pFlash->flashSectorSize) {
			ui32NumBytes -= g_pFlash->flashSectorSize;
			ui32Addr += g_pFlash->flashSectorSize;
		} else {
			break;
		}
	}
}

static void amotas_send_data(uint8_t *buf, uint16_t len)
{
	int err;

	LOG_INF("%s - bt_gatt_notify, len=%d", __func__, len);
	err = bt_gatt_notify(amota.conn, amota_notify_ch, buf, len);
	if (err) {
		LOG_ERR("%s: bt_gatt_notify failed: %d", __func__, err);
	}
}

static void amotas_reply_to_client(eAmotaCommand cmd, eAmotaStatus status, uint8_t *data,
				   uint16_t len)
{
	uint8_t buf[20] = {0};

	buf[0] = (len + 1) & 0xff;
	buf[1] = ((len + 1) >> 8) & 0xff;
	buf[2] = cmd;
	buf[3] = status;
	if (len > 0) {
		memcpy(buf + 4, data, len);
	}
	amotas_send_data(buf, len + 4);
}

static bool amotas_set_fw_addr(void)
{
	bool bResult = false;

	amota.data.newFwFlashInfo.addr = 0;
	amota.data.newFwFlashInfo.offset = 0;

	/* Check storage type */
	if (amota.data.fwHeader.storageType == AMOTA_FW_STORAGE_INTERNAL) {
		uint32_t storeAddr = (AMOTA_INT_FLASH_OTA_ADDRESS + AMOTA_WRITE_PAGE_SIZE - 1) &
				     ~(AMOTA_WRITE_PAGE_SIZE - 1);
		uint32_t maxSize = AMOTA_INT_FLASH_OTA_MAX_SIZE & ~(AMOTA_WRITE_PAGE_SIZE - 1);

		/* Check to make sure the incoming image will fit in the space allocated for OTA */
		if (amota.data.fwHeader.fwLength > maxSize) {
			LOG_INF("not enough OTA space allocated = %d bytes, Desired = %d bytes",
				maxSize, amota.data.fwHeader.fwLength);
			return false;
		}

		g_pFlash = &g_intFlash;
		amota.data.newFwFlashInfo.addr = storeAddr;
		bResult = true;
	} else {
		bResult = false;
	}

	if (bResult == true) {
		/* Initialize the flash device */
		if (FLASH_OPERATE(g_pFlash, flash_init) == 0) {
			if (FLASH_OPERATE(g_pFlash, flash_enable) != 0) {
				FLASH_OPERATE(g_pFlash, flash_deinit);
				bResult = false;
			}

			/* Erase necessary sectors in the flash according to length of the image. */
			erase_flash(amota.data.newFwFlashInfo.addr, amota.data.fwHeader.fwLength);
			FLASH_OPERATE(g_pFlash, flash_disable);
		} else {
			bResult = false;
		}
	}
	return bResult;
}

static int verify_flash_content(uint32_t flashAddr, uint32_t *pSram, uint32_t len,
				am_util_multiboot_flash_info_t *pFlash)
{
	uint32_t offset = 0;
	uint32_t remaining = len;
	int ret = 0;
#if defined(AM_PART_APOLLO510)
	/* Clean the cache after writing and invalidate before reading */
	am_hal_cachectrl_dcache_invalidate(NULL, true);
#endif
	while (remaining) {
		uint32_t tmpSize = (remaining > AMOTA_PACKET_SIZE) ? AMOTA_PACKET_SIZE : remaining;

		pFlash->flash_read_page((uint32_t)amotasTmpBuf, (uint32_t *)(flashAddr + offset),
					tmpSize);

		ret = memcmp(amotasTmpBuf, (uint8_t *)((uint32_t)pSram + offset), tmpSize);

		if (ret != 0) {
			LOG_WRN("flash write verify failed. address 0x%x. length %d", flashAddr,
				len);
			break;
		}
		offset += tmpSize;
		remaining -= tmpSize;
	}

	return ret;
}

static bool amotas_write2flash(uint16_t len, uint8_t *buf, uint32_t addr, bool lastPktFlag)
{
	uint16_t ui16BytesRemaining = len;
	uint32_t ui32TargetAddress = 0;
	uint8_t ui8PageCount = 0;
	bool bResult = true;
	uint16_t i;

	addr -= amotasFlash.bufferIndex;
	/* Check the target flash address to ensure we do not operate on the wrong address
	 * make sure to write to page boundary
	 */
	if (((!((amota.data.fwHeader.imageId == AMOTA_IMAGE_ID_SBL) &&
		(amota.data.newFwFlashInfo.offset < AMOTA_ENCRYPTED_SBL_SIZE))) &&
	     ((uint32_t)amota.data.newFwFlashInfo.addr > addr)) ||
	    (addr & (g_pFlash->flashPageSize - 1))) {
		return false;
	}

	FLASH_OPERATE(g_pFlash, flash_enable);
	while (ui16BytesRemaining) {
		uint16_t ui16Bytes2write = g_pFlash->flashPageSize - amotasFlash.bufferIndex;

		if (ui16Bytes2write > ui16BytesRemaining) {
			ui16Bytes2write = ui16BytesRemaining;
		}
		/* move data into buffer */
		for (i = 0; i < ui16Bytes2write; i++) {
			/* avoid using memcpy */
			amotasFlash.writeBuffer[amotasFlash.bufferIndex++] = buf[i];
		}
		ui16BytesRemaining -= ui16Bytes2write;
		buf += ui16Bytes2write;

		/* Write to flash when there is data more than 1 page size
		 * For last fragment write even if it is less than one page
		 */
		if (lastPktFlag || (amotasFlash.bufferIndex == g_pFlash->flashPageSize)) {
			ui32TargetAddress = (addr + ui8PageCount * g_pFlash->flashPageSize);
			/* Always write whole pages */
			if ((g_pFlash->flash_write_page(ui32TargetAddress,
							(uint32_t *)amotasFlash.writeBuffer,
							g_pFlash->flashPageSize) != 0) ||
			    (verify_flash_content(ui32TargetAddress,
						  (uint32_t *)amotasFlash.writeBuffer,
						  amotasFlash.bufferIndex, g_pFlash) != 0)) {
				bResult = false;
				break;
			}
			LOG_DBG("Flash write succeeded to address 0x%x. length %d",
				ui32TargetAddress, amotasFlash.bufferIndex);

			ui8PageCount++;
			amotasFlash.bufferIndex = 0;
			bResult = true;
		}
	}
	FLASH_OPERATE(g_pFlash, flash_disable);

	/* If we get here, operations are done correctly */
	return bResult;
}

static void amotas_update_ota(void)
{
	uint8_t magic;

	if (amota.data.fwHeader.imageId == AMOTA_IMAGE_ID_SBL) {
		magic = AM_IMAGE_MAGIC_SBL;
	} else {
		magic = amota.data.metaData.magicNum;
	}

	/* Set OTAPOINTER */
	am_hal_ota_add(AM_HAL_MRAM_PROGRAM_KEY, magic, (uint32_t *)amota.data.newFwFlashInfo.addr);
}

static void amotas_init_ota(void)
{
	am_hal_otadesc_t *pOtaDesc =
		(am_hal_otadesc_t *)(OTA_POINTER_LOCATION & ~(AMOTA_WRITE_PAGE_SIZE - 1));
	/* Initialize OTA descriptor - This should ideally be initiated through a separate command
	 * to facilitate multiple image upgrade in a single reboot
	 * Will need change in the AMOTA app to do so
	 */
	am_hal_ota_init(AM_HAL_MRAM_PROGRAM_KEY, pOtaDesc);
}

void amotas_packet_handler(eAmotaCommand cmd, uint16_t len, uint8_t *buf)
{
	eAmotaStatus status = AMOTA_STATUS_SUCCESS;
	uint8_t data[4] = {0};
	bool bResult = false;
	uint32_t ver, fwCrc;

	ver = fwCrc = 0;
	bool resumeTransfer = false;

	LOG_DBG("received packet cmd = 0x%x, len = 0x%x", cmd, len);

	switch (cmd) {
	case AMOTA_CMD_FW_HEADER:
		if (len < AMOTA_FW_HEADER_SIZE) {
			status = AMOTA_STATUS_INVALID_HEADER_INFO;
			amotas_reply_to_client(cmd, status, NULL, 0);
			break;
		}

		if (amota.data.state == AMOTA_STATE_GETTING_FW) {
			fwCrc = sys_get_le32(buf + 12);
			ver = sys_get_le32(buf + 32);

			if (ver == amota.data.fwHeader.version &&
			    fwCrc == amota.data.fwHeader.fwCrc) {
				resumeTransfer = true;
			}
		}

		amota.data.imageCalCrc = 0;
		amota.data.fwHeader.encrypted = sys_get_le32(buf);
		amota.data.fwHeader.fwStartAddr = sys_get_le32(buf + 4);
		amota.data.fwHeader.fwLength = sys_get_le32(buf + 8);
		amota.data.fwHeader.fwCrc = sys_get_le32(buf + 12);
		amota.data.fwHeader.secInfoLen = sys_get_le32(buf + 16);
		amota.data.fwHeader.version = sys_get_le32(buf + 32);
		amota.data.fwHeader.fwDataType = sys_get_le32(buf + 36);
		amota.data.fwHeader.storageType = sys_get_le32(buf + 40);
		amota.data.fwHeader.imageId = sys_get_le32(buf + 44);

#if defined(AM_PART_APOLLO4B) || defined(AM_PART_APOLLO4P) || defined(AM_PART_APOLLO4L)
		/* get the SBL OTA storage address if the image is for SBL
		 * the address can be 0x8000 or 0x10000 based on current SBL running address
		 */
		if (amota.data.fwHeader.imageId == AMOTA_IMAGE_ID_SBL) {
			am_hal_security_info_t pSecInfo;

			am_hal_security_get_info(&pSecInfo);
			amota.data.sblOtaStorageAddr = pSecInfo.sblStagingAddr;
			LOG_INF("get amota.data.sblOtaStorageAddr: 0x%x",
				amota.data.sblOtaStorageAddr);
		}
#endif
		if (resumeTransfer) {
			LOG_INF("OTA process start from offset = 0x%x",
				amota.data.newFwFlashInfo.offset);
			LOG_INF("beginning of flash addr = 0x%x", amota.data.newFwFlashInfo.addr);
		} else {
			LOG_INF("OTA process start from beginning");
			amotasFlash.bufferIndex = 0;
			bResult = amotas_set_fw_addr();

			if (bResult == false) {
				amotas_reply_to_client(cmd, AMOTA_STATUS_INSUFFICIENT_FLASH, NULL,
						       0);
				amota.data.state = AMOTA_STATE_INIT;
				return;
			}

			amota.data.state = AMOTA_STATE_GETTING_FW;
		}

		LOG_INF("============= fw header start ===============");
		LOG_INF("encrypted = 0x%x", amota.data.fwHeader.encrypted);
		LOG_INF("version = 0x%x", amota.data.fwHeader.version);
		LOG_INF("fwLength = 0x%x", amota.data.fwHeader.fwLength);
		LOG_INF("fwCrc = 0x%x", amota.data.fwHeader.fwCrc);
		LOG_INF("fwStartAddr = 0x%x", amota.data.fwHeader.fwStartAddr);
		LOG_INF("fwDataType = 0x%x", amota.data.fwHeader.fwDataType);
		LOG_INF("storageType = 0x%x", amota.data.fwHeader.storageType);

		LOG_INF("imageId = 0x%x", amota.data.fwHeader.imageId);
		LOG_INF("============= fw header end ===============");

		data[0] = ((amota.data.newFwFlashInfo.offset) & 0xff);
		data[1] = ((amota.data.newFwFlashInfo.offset >> 8) & 0xff);
		data[2] = ((amota.data.newFwFlashInfo.offset >> 16) & 0xff);
		data[3] = ((amota.data.newFwFlashInfo.offset >> 24) & 0xff);
		amotas_reply_to_client(cmd, AMOTA_STATUS_SUCCESS, data, sizeof(data));
		break;

	case AMOTA_CMD_FW_DATA: {
		if (amota.data.newFwFlashInfo.offset == 0) {
			memcpy(&amota.data.metaData, buf, sizeof(amotaMetadataInfo_t));
		}

		if (amota.data.fwHeader.imageId == AMOTA_IMAGE_ID_SBL) {
			if (amota.data.newFwFlashInfo.offset < AMOTA_ENCRYPTED_SBL_SIZE) {
				bResult =
					amotas_write2flash(len, buf,
							   amota.data.sblOtaStorageAddr +
								   amota.data.newFwFlashInfo.offset,
							   ((amota.data.newFwFlashInfo.offset +
							     len) == AMOTA_ENCRYPTED_SBL_SIZE));
			} else {
				bResult = amotas_write2flash(
					len, buf,
					amota.data.newFwFlashInfo.addr +
						amota.data.newFwFlashInfo.offset -
						AMOTA_ENCRYPTED_SBL_SIZE,
					((amota.data.newFwFlashInfo.offset + len) ==
					 amota.data.fwHeader.fwLength));
			}
		} else {
			bResult = amotas_write2flash(len, buf,
						     amota.data.newFwFlashInfo.addr +
							     amota.data.newFwFlashInfo.offset,
						     ((amota.data.newFwFlashInfo.offset + len) ==
						      amota.data.fwHeader.fwLength));
		}

		if (bResult == false) {
			data[0] = ((amota.data.newFwFlashInfo.offset) & 0xff);
			data[1] = ((amota.data.newFwFlashInfo.offset >> 8) & 0xff);
			data[2] = ((amota.data.newFwFlashInfo.offset >> 16) & 0xff);
			data[3] = ((amota.data.newFwFlashInfo.offset >> 24) & 0xff);
			amotas_reply_to_client(cmd, AMOTA_STATUS_FLASH_WRITE_ERROR, data,
					       sizeof(data));
		} else {
			am_util_bootloader_partial_crc32(buf, len, &amota.data.imageCalCrc);

			amota.data.newFwFlashInfo.offset += len;

			data[0] = ((amota.data.newFwFlashInfo.offset) & 0xff);
			data[1] = ((amota.data.newFwFlashInfo.offset >> 8) & 0xff);
			data[2] = ((amota.data.newFwFlashInfo.offset >> 16) & 0xff);
			data[3] = ((amota.data.newFwFlashInfo.offset >> 24) & 0xff);
			amotas_reply_to_client(cmd, AMOTA_STATUS_SUCCESS, data, sizeof(data));
		}
	} break;

	case AMOTA_CMD_FW_VERIFY:
		if (amota.data.imageCalCrc == amota.data.fwHeader.fwCrc) {
			LOG_INF("CRC verification succeeds");
			amotas_reply_to_client(cmd, AMOTA_STATUS_SUCCESS, NULL, 0);

			/* Update flash flag page here */
			amotas_update_ota();
		} else {
			LOG_WRN("CRC verification fails");
			amotas_reply_to_client(cmd, AMOTA_STATUS_CRC_ERROR, NULL, 0);
		}
		FLASH_OPERATE(g_pFlash, flash_deinit);
		amota.data.state = AMOTA_STATE_INIT;
		amota.data.imageCalCrc = 0;
		g_pFlash = &g_intFlash;

		/* Set SBL OTA storage address to invalid value once data verify finish */
		if (amota.data.fwHeader.imageId == AMOTA_IMAGE_ID_SBL) {
			amota.data.sblOtaStorageAddr = AMOTA_INVALID_SBL_STOR_ADDR;
		}
		break;

	case AMOTA_CMD_FW_RESET:
		LOG_INF("OTA downloading finished, will disconnect BLE link soon");
		k_sleep(K_MSEC(100));

		amotas_reply_to_client(cmd, AMOTA_STATUS_SUCCESS, NULL, 0);

		/* Delay here to let packet go through the RF before we disconnect */
		k_sleep(K_MSEC(1000));

		am_hal_reset_control(AM_HAL_RESET_CONTROL_SWPOR, 0);
		break;

	default:
		break;
	}
}

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_INF("Updated MTU: TX: %d RX: %d bytes", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

static void amota_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("AMOTA notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t write_callback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

/* AMOTA Service Declaration */
BT_GATT_SERVICE_DEFINE(amota_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_AMOTA),
		       BT_GATT_CHARACTERISTIC(BT_UUID_AMOTA_TX_CHAR, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_READ, NULL, NULL, NULL),
		       BT_GATT_CCC(amota_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_AMOTA_RX_CHAR,
					      BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE,
					      NULL, write_callback, NULL));

static ssize_t write_callback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t dataIdx = 0;
	uint32_t peerCrc = 0;
	uint32_t calDataCrc = 0;
	uint8_t *pValue = (uint8_t *)buf;

	if (amota.data.pkt.offset == 0 && len < AMOTA_HEADER_SIZE_IN_PKT) {
		LOG_ERR("Invalid packet!!!");
		amotas_reply_to_client(AMOTA_CMD_FW_HEADER, AMOTA_STATUS_INVALID_PKT_LENGTH, NULL,
				       0);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_PDU);
	}

	/* This is a new packet. */
	if (amota.data.pkt.offset == 0) {
		amota.data.pkt.len = sys_get_le16(pValue);
		amota.data.pkt.type = (eAmotaCommand)pValue[2];
		dataIdx = 3;

		LOG_DBG("pkt.len = 0x%x", amota.data.pkt.len);
		LOG_DBG("pkt.type = 0x%x", amota.data.pkt.type);

		if (dataIdx > amota.data.pkt.len) {
			LOG_ERR("packet length is wrong since it's smaller than 3!");
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
	}

	/* make sure we have enough space for new data */
	if (amota.data.pkt.offset + len - dataIdx > AMOTA_PACKET_SIZE) {
		LOG_ERR("not enough buffer size!!!");
		amotas_reply_to_client(amota.data.pkt.type, AMOTA_STATUS_INSUFFICIENT_BUFFER, NULL,
				       0);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	/* copy new data into buffer and also save crc into it if it's the last frame in a packet
	 * 4 bytes crc is included in packet length.
	 */
	memcpy(amota.data.pkt.data + amota.data.pkt.offset, pValue + dataIdx, len - dataIdx);
	amota.data.pkt.offset += (len - dataIdx);

	/* The whole packet has been received */
	if (amota.data.pkt.offset >= amota.data.pkt.len) {
		/* Check CRC */
		peerCrc = sys_get_le32(amota.data.pkt.data + amota.data.pkt.len -
				       AMOTA_CRC_SIZE_IN_PKT);
		calDataCrc = CalcCrc32(0xFFFFFFFFU, amota.data.pkt.len - AMOTA_CRC_SIZE_IN_PKT,
				       amota.data.pkt.data);
		LOG_DBG("calDataCrc = 0x%x", calDataCrc);
		LOG_DBG("peerCrc = 0x%x", peerCrc);

		if (peerCrc != calDataCrc) {
			amotas_reply_to_client(amota.data.pkt.type, AMOTA_STATUS_CRC_ERROR, NULL,
					       0);
			amota.data.pkt.offset = 0;
			amota.data.pkt.type = AMOTA_CMD_UNKNOWN;
			amota.data.pkt.len = 0;

			/* Return len to indicate write was accepted, error handled via reply */
			return len;
		}

		LOG_INF("Packet received correctly, OTA is ongoing...");

		amotas_packet_handler(amota.data.pkt.type,
				      amota.data.pkt.len - AMOTA_CRC_SIZE_IN_PKT,
				      amota.data.pkt.data);
		amota.data.pkt.offset = 0;
		amota.data.pkt.type = AMOTA_CMD_UNKNOWN;
		amota.data.pkt.len = 0;
	}

	return len;
}

static int bt_amota_init(void)
{
	amota_notify_ch = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_AMOTA_TX_CHAR);
	if (amota_notify_ch) {
		LOG_INF("Found attribute with UUID 0x%04x at handle 0x%04x",
			((struct bt_uuid_16 *)BT_UUID_AMOTA_TX_CHAR)->val, amota_notify_ch->handle);
	} else {
		LOG_WRN("Attribute with UUID 0x%04x not found",
			((struct bt_uuid_16 *)BT_UUID_AMOTA_TX_CHAR)->val);
	}

	amotas_init_ota();

	bt_gatt_cb_register(&gatt_callbacks);

	return 0;
}

int bt_amota_notify(void)
{
	return 0;
}

int bt_amota_conn_init(struct bt_conn *conn)
{
	if (!conn) {
		LOG_ERR("AMOTA: Invalid connection parameter");
		return -EINVAL;
	}

	LOG_INF("AMOTA: Connection initialized");
	amota.data.state = AMOTA_STATE_INIT;
	amota.conn = conn;

	return 0;
}

SYS_INIT(bt_amota_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
