/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 shared type definitions.
 */

#ifndef ZEPHYR_INCLUDE_FIDO2_FIDO2_TYPES_H_
#define ZEPHYR_INCLUDE_FIDO2_FIDO2_TYPES_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>

/**
 * @brief FIDO2 shared types
 * @ingroup fido2
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum credential ID size in bytes */
#define FIDO2_CREDENTIAL_ID_MAX_SIZE 128

/** @brief Maximum relying party ID length */
#define FIDO2_RP_ID_MAX_LEN 128

/** @brief Maximum relying party name length */
#define FIDO2_RP_NAME_MAX_LEN 64

/** @brief Maximum user name length */
#define FIDO2_USER_NAME_MAX_LEN 64

/** @brief Maximum user display name length */
#define FIDO2_USER_DISPLAY_NAME_MAX_LEN 64

/** @brief Maximum user ID size in bytes */
#define FIDO2_USER_ID_MAX_SIZE 64

/** @brief AAGUID size in bytes */
#define FIDO2_AAGUID_SIZE 16

/** @brief SHA-256 hash size */
#define FIDO2_SHA256_SIZE 32

/** @brief PIN hash size */
#define FIDO2_PIN_HASH_SIZE 32

/** @brief Maximum number of supported extensions */
#define FIDO2_MAX_EXTENSIONS 4

/** @brief Maximum number of supported versions */
#define FIDO2_MAX_VERSIONS 4

/** @brief Credential extension HMAC secret */
#define FIDO2_EXT_HMAC_SECRET BIT(0)
/** @brief Credential extension large blob */
#define FIDO2_EXT_LARGE_BLOB  BIT(1)

/** @brief Transport USB */
#define FIDO2_TRANSPORT_USB BIT(0)
/** @brief Transport BLE */
#define FIDO2_TRANSPORT_BLE BIT(1)
/** @brief Transport NFC */
#define FIDO2_TRANSPORT_NFC BIT(2)

/** @brief CTAP2 status codes */
enum fido2_status {
	FIDO2_OK = 0x00,                          /**< Success */
	FIDO2_ERR_INVALID_COMMAND = 0x01,         /**< Invalid command */
	FIDO2_ERR_INVALID_PARAMETER = 0x02,       /**< Invalid parameter */
	FIDO2_ERR_INVALID_LENGTH = 0x03,          /**< Invalid message length */
	FIDO2_ERR_INVALID_SEQ = 0x04,             /**< Invalid sequence number */
	FIDO2_ERR_TIMEOUT = 0x05,                 /**< Request timed out */
	FIDO2_ERR_CHANNEL_BUSY = 0x06,            /**< Channel busy */
	FIDO2_ERR_LOCK_REQUIRED = 0x0A,           /**< Command requires lock */
	FIDO2_ERR_INVALID_CHANNEL = 0x0B,         /**< Invalid channel */
	FIDO2_ERR_CBOR_UNEXPECTED_TYPE = 0x11,    /**< Unexpected CBOR type */
	FIDO2_ERR_INVALID_CBOR = 0x12,            /**< Invalid CBOR encoding */
	FIDO2_ERR_MISSING_PARAMETER = 0x14,       /**< Required parameter missing */
	FIDO2_ERR_LIMIT_EXCEEDED = 0x15,          /**< Limit exceeded */
	FIDO2_ERR_UNSUPPORTED_EXTENSION = 0x16,   /**< Unsupported extension */
	FIDO2_ERR_CREDENTIAL_EXCLUDED = 0x19,     /**< Credential in excludeList found */
	FIDO2_ERR_PROCESSING = 0x21,              /**< Processing */
	FIDO2_ERR_INVALID_CREDENTIAL = 0x22,      /**< Invalid credential */
	FIDO2_ERR_USER_ACTION_PENDING = 0x23,     /**< Waiting for user action */
	FIDO2_ERR_OPERATION_PENDING = 0x24,       /**< Operation pending */
	FIDO2_ERR_NO_OPERATIONS = 0x25,           /**< No operations pending */
	FIDO2_ERR_UNSUPPORTED_ALGORITHM = 0x26,   /**< Unsupported algorithm */
	FIDO2_ERR_OPERATION_DENIED = 0x27,        /**< Operation denied */
	FIDO2_ERR_KEY_STORE_FULL = 0x28,          /**< Key store full */
	FIDO2_ERR_UNSUPPORTED_OPTION = 0x2B,      /**< Unsupported option */
	FIDO2_ERR_INVALID_OPTION = 0x2C,          /**< Option value invalid for this operation */
	FIDO2_ERR_KEEPALIVE_CANCEL = 0x2D,        /**< Keepalive cancelled by platform */
	FIDO2_ERR_NO_CREDENTIALS = 0x2E,          /**< No credentials found */
	FIDO2_ERR_USER_ACTION_TIMEOUT = 0x2F,     /**< User action timed out */
	FIDO2_ERR_NOT_ALLOWED = 0x30,             /**< Operation not allowed */
	FIDO2_ERR_PIN_INVALID = 0x31,             /**< Invalid PIN */
	FIDO2_ERR_PIN_BLOCKED = 0x32,             /**< PIN blocked */
	FIDO2_ERR_PIN_AUTH_INVALID = 0x33,        /**< PIN auth verification failed */
	FIDO2_ERR_PIN_AUTH_BLOCKED = 0x34,        /**< PIN auth blocked */
	FIDO2_ERR_PIN_NOT_SET = 0x35,             /**< PIN not set */
	FIDO2_ERR_PUAT_REQUIRED = 0x36,           /**< PIN/UV auth token required */
	FIDO2_ERR_PIN_POLICY_VIOLATION = 0x37,    /**< PIN policy violation */
	FIDO2_ERR_PIN_TOKEN_EXPIRED = 0x38,       /**< PIN/UV auth token expired */
	FIDO2_ERR_REQUEST_TOO_LARGE = 0x39,       /**< Request exceeds maxMsgSize */
	FIDO2_ERR_ACTION_TIMEOUT = 0x3A,          /**< Platform response timed out */
	FIDO2_ERR_UP_REQUIRED = 0x3B,             /**< User presence required */
	FIDO2_ERR_UV_BLOCKED = 0x3C,              /**< User verification blocked */
	FIDO2_ERR_INTEGRITY_FAILURE = 0x3D,       /**< Authenticator integrity check failed */
	FIDO2_ERR_INVALID_SUBCOMMAND = 0x3E,      /**< Invalid subcommand for this command */
	FIDO2_ERR_UV_INVALID = 0x3F,              /**< User verification failed */
	FIDO2_ERR_UNAUTHORIZED_PERMISSION = 0x40, /**< PIN/UV token missing permission */
	FIDO2_ERR_OTHER = 0x7F,                   /**< Other unspecified error */
};

/** @brief CTAP2 command codes */
enum fido2_cmd {
	FIDO2_CMD_MAKE_CREDENTIAL = 0x01,    /**< Create a new credential */
	FIDO2_CMD_GET_ASSERTION = 0x02,      /**< Authenticate with a credential */
	FIDO2_CMD_GET_INFO = 0x04,           /**< Get authenticator info */
	FIDO2_CMD_CLIENT_PIN = 0x06,         /**< Client PIN operations */
	FIDO2_CMD_RESET = 0x07,              /**< Factory reset */
	FIDO2_CMD_GET_NEXT_ASSERTION = 0x08, /**< Get next assertion */
	FIDO2_CMD_CREDENTIAL_MGMT = 0x0A,    /**< Credential management */
	FIDO2_CMD_SELECTION = 0x0B,          /**< Authenticator selection */
};

/** @brief Credential protection levels */
enum fido2_cred_protect {
	/** UV optional; credential usable without verification */
	FIDO2_CRED_PROTECT_UV_OPTIONAL = 0x01,
	/** UV optional; credential usable only with credential ID list */
	FIDO2_CRED_PROTECT_UV_OPTIONAL_WITH_LIST = 0x02,
	/** UV required; credential always requires user verification */
	FIDO2_CRED_PROTECT_UV_REQUIRED = 0x03,
};

/** @brief COSE algorithm identifiers */
enum fido2_cose_alg {
	FIDO2_COSE_ES256 = -7,   /**< ECDSA w/ SHA-256 */
	FIDO2_COSE_EDDSA = -8,   /**< EdDSA */
	FIDO2_COSE_RS256 = -257, /**< RSASSA-PKCS1-v1_5 w/ SHA-256 */
};

/** @brief A stored FIDO2 credential */
struct fido2_credential {
	/** Credential identifier */
	uint8_t id[FIDO2_CREDENTIAL_ID_MAX_SIZE];
	/** Credential identifier length */
	uint16_t id_len;
	/** SHA-256 hash of the relying party ID */
	uint8_t rp_id_hash[FIDO2_SHA256_SIZE];
	/** Relying party identifier */
	char rp_id[FIDO2_RP_ID_MAX_LEN];
	/** Relying party display name */
	char rp_name[FIDO2_RP_NAME_MAX_LEN];
	/** User account name */
	char user_name[FIDO2_USER_NAME_MAX_LEN];
	/** User display name */
	char user_display_name[FIDO2_USER_DISPLAY_NAME_MAX_LEN];
	/** User handle */
	uint8_t user_id[FIDO2_USER_ID_MAX_SIZE];
	/** User handle length */
	uint16_t user_id_len;
	/** PSA Crypto key identifier for this credential */
	uint32_t key_id;
	/** Signature counter */
	uint32_t sign_count;
	/** Credential extensions bitmask (e.g. hmac-secret, largeBlob) */
	uint32_t extensions;
	/** COSE algorithm identifier */
	int32_t algorithm;
	/** Discoverable (resident) credential */
	bool discoverable;
	/** Credential protection level */
	uint8_t cred_protect;
};

/**
 * @brief Device information returned by authenticatorGetInfo.
 */
struct fido2_device_info {
	/** Supported protocol versions */
	const char *versions[FIDO2_MAX_VERSIONS];
	/** Number of supported versions */
	uint8_t num_versions;
	/** Supported extensions */
	const char *extensions[FIDO2_MAX_EXTENSIONS];
	/** Number of supported extensions */
	uint8_t num_extensions;
	/** Authenticator Attestation GUID */
	uint8_t aaguid[FIDO2_AAGUID_SIZE];
	/** Maximum credential count */
	uint16_t max_credential_count;
	/** Maximum credential ID length */
	uint16_t max_credential_id_length;
	/** Maximum CBOR message size in bytes */
	uint16_t max_msg_size;
	/** Supported transports bitmask */
	uint8_t transports;
	/** CTAP 2.1 Options Map */
	struct {
		bool plat;       /**< Platform device */
		bool rk;         /**< Resident key support */
		bool client_pin; /**< Client PIN supported */
		bool up;         /**< User presence support */
		bool uv;         /**< Built-in user verification support */
	} options;
	/** Firmware version */
	uint32_t firmware_version;
	/** Supported PIN/UV auth protocol versions */
	uint8_t pin_uv_auth_protocols[2];
	/** Number of supported PIN/UV auth protocols */
	uint8_t num_pin_uv_auth_protocols;
	/** Remaining PIN retry attempts. */
	uint8_t pin_retries;
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_FIDO2_FIDO2_TYPES_H_ */
