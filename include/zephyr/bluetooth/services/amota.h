/** @file
 *  @brief AMOTA Service
 */

/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_AMOTA_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_AMOTA_H_

/**
 * @brief AMOTA Service (AMOTA)
 * @defgroup bt_amota Ambiq OTA Service (AMOTA)
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMOTA_PACKET_SIZE        (512 + 16)
#define AMOTA_LENGTH_SIZE_IN_PKT 2
#define AMOTA_CMD_SIZE_IN_PKT    1
#define AMOTA_CRC_SIZE_IN_PKT    4
#define AMOTA_HEADER_SIZE_IN_PKT (AMOTA_LENGTH_SIZE_IN_PKT + AMOTA_CMD_SIZE_IN_PKT)

#define AMOTA_FW_HEADER_SIZE 44

#define AMOTA_FW_STORAGE_INTERNAL 0
#define AMOTA_FW_STORAGE_EXTERNAL 1

/* The imageId field of amotaHeaderInfo_t is used to get the type of downloading image.
 * Define image id macros here to identify the type of OTA image and process corresponding
 * logic.
 */
#define AMOTA_IMAGE_ID_SBL         (0x00000001)
#define AMOTA_IMAGE_ID_APPLICATION (0x00000002)

#define AMOTA_ENCRYPTED_SBL_SIZE    (32 * 1024)
#define AMOTA_INVALID_SBL_STOR_ADDR (0xFFFFFFFF)

/** @brief amota state */
typedef enum {
	AMOTA_STATE_INIT,
	AMOTA_STATE_GETTING_FW,
	AMOTA_STATE_MAX
} eAmotaState;

/** @brief amota commands */
typedef enum {
	AMOTA_CMD_UNKNOWN,
	AMOTA_CMD_FW_HEADER,
	AMOTA_CMD_FW_DATA,
	AMOTA_CMD_FW_VERIFY,
	AMOTA_CMD_FW_RESET,
	AMOTA_CMD_MAX
} eAmotaCommand;

/** @brief amota status */
typedef enum {
	AMOTA_STATUS_SUCCESS,
	AMOTA_STATUS_CRC_ERROR,
	AMOTA_STATUS_INVALID_HEADER_INFO,
	AMOTA_STATUS_INVALID_PKT_LENGTH,
	AMOTA_STATUS_INSUFFICIENT_BUFFER,
	AMOTA_STATUS_INSUFFICIENT_FLASH,
	AMOTA_STATUS_UNKNOWN_ERROR,
	AMOTA_STATUS_FLASH_WRITE_ERROR,
	AMOTA_STATUS_MAX
} eAmotaStatus;

/** @brief firmware header information */
typedef struct {
	uint32_t encrypted;
	uint32_t fwStartAddr;
	uint32_t fwLength;
	uint32_t fwCrc;
	uint32_t secInfoLen;
	uint32_t resvd1;
	uint32_t resvd2;
	uint32_t resvd3;
	uint32_t version;
	uint32_t fwDataType;
	uint32_t storageType;
	uint32_t imageId;
} amotaHeaderInfo_t;

/** @brief 32-byte Metadata added ahead of the actual image and after the 48-byte AMOTA header */
typedef struct {
	/* Word 0 */
	uint32_t blobSize: 24;
	uint32_t resvd1: 2;
	uint32_t crcCheck: 1;
	uint32_t enc: 1;
	uint32_t authCheck: 1;
	uint32_t ccIncluded: 1;
	uint32_t ambiq: 1;
	uint32_t resvd2: 1;

	/* Word 1 */
	uint32_t crc;

	/* Word 2 */
	uint32_t authKeyIdx: 8;
	uint32_t encKeyIdx: 8;
	uint32_t authAlgo: 4;
	uint32_t encAlgo: 4;
	uint32_t resvd3: 8;

	/* Word 3 */
	uint32_t resvd4;

	/* Word 4 */
	uint32_t magicNum: 8;
	uint32_t resvd5: 24;

	/* Word 5-7 */
	uint32_t resvd6;
	uint32_t resvd7;
	uint32_t resvd8;
} amotaMetadataInfo_t;

/** @brief firmware packet information */
typedef struct {
	uint16_t offset;
	uint16_t len;
	eAmotaCommand type;
	uint8_t data[AMOTA_PACKET_SIZE] __aligned(4);
} amotaPacket_t;

/** @brief firmware Address */
typedef struct {
	uint32_t addr;
	uint32_t offset;
} amotasNewFwFlashInfo_t;

/** @brief AMOTA data */
struct bt_amota_data {
	eAmotaState state;
	amotaHeaderInfo_t fwHeader;
	amotaPacket_t pkt;
	amotasNewFwFlashInfo_t newFwFlashInfo;
	amotaMetadataInfo_t metaData;
	uint32_t sblOtaStorageAddr;
	uint32_t imageCalCrc;
};

/** @brief amota structure. */
struct bt_amota {

	/** amota data. */
	struct bt_amota_data data;

	/** connection object. */
	struct bt_conn *conn;
};

#define BT_UUID_AMOTA_SVC_VAL BT_UUID_128_ENCODE(0x00002760, 0x08C2, 0x11E1, 0x9073, 0x0E8AC72E1001)

#define BT_UUID_AMOTA_RX_CHAR_VAL                                                                  \
	BT_UUID_128_ENCODE(0x00002760, 0x08C2, 0x11E1, 0x9073, 0x0E8AC72E0001)

#define BT_UUID_AMOTA_TX_CHAR_VAL                                                                  \
	BT_UUID_128_ENCODE(0x00002760, 0x08C2, 0x11E1, 0x9073, 0x0E8AC72E0002)

/** @brief AMOTA Service UUID. */
#define BT_UUID_AMOTA BT_UUID_DECLARE_128(BT_UUID_AMOTA_SVC_VAL)

/** @brief AMOTA RX Characteristic UUID. */
#define BT_UUID_AMOTA_RX_CHAR BT_UUID_DECLARE_128(BT_UUID_AMOTA_RX_CHAR_VAL)

/** @brief AMOTA TX Characteristic UUID. */
#define BT_UUID_AMOTA_TX_CHAR BT_UUID_DECLARE_128(BT_UUID_AMOTA_TX_CHAR_VAL)

/** @brief Send AMOTA data.
 *
 * The data will be sent via a GATT notification to the subscriber.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_amota_notify(void);

/** @brief Initialize the AMOTA data after connection.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_amota_conn_init(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_AMOTA_H_ */
