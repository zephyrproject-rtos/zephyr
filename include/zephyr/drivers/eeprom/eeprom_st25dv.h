/*
 * Copyright (c) 2023 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_ST25DV_H_
#define ZEPHYR_INCLUDE_DRIVERS_EEPROM_ST25DV_H_

#include <zephyr/drivers/eeprom.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Length of the UUID field of the ST25DV. */
#define EEPROM_ST25DV_UUID_LENGTH 8U

/** @brief ST25DV IC reference values (ISO 15693 format) */
typedef enum {
	EEPROM_ST25DV_IC_REF_ST25DV04K_IE = 0x24,
	EEPROM_ST25DV_IC_REF_ST25DV16K_IE = 0x26,
	EEPROM_ST25DV_IC_REF_ST25DV64K_IE = 0x26,
	EEPROM_ST25DV_IC_REF_ST25DV04K_JF = 0x24,
	EEPROM_ST25DV_IC_REF_ST25DV16K_JF = 0x26,
	EEPROM_ST25DV_IC_REF_ST25DV64K_JF = 0x26,
} eeprom_st25dv_ic_ref_t;

/** @brief ST25DV area end address enumerator definition. */
typedef enum {
	EEPROM_ST25DV_END_ZONE1 = 0,
	EEPROM_ST25DV_END_ZONE2,
	EEPROM_ST25DV_END_ZONE3
} eeprom_st25dv_end_zone_t;

/** @brief ST25DV Password typedef. */
typedef uint64_t eeprom_st25dv_passwd_t;

/** @brief ST25DV session status enumerator definition. */
typedef enum {
	EEPROM_ST25DV_I2C_SESSION_CLOSED = 0,
	EEPROM_ST25DV_I2C_SESSION_OPEN
} eeprom_st25dv_i2c_sso_status_t;

/**
 * @brief ST25DV protection status enumerator definition
 */
typedef enum {
	EEPROM_ST25DV_NO_PROT = 0,
	EEPROM_ST25DV_WRITE_PROT,
	EEPROM_ST25DV_READ_PROT,
	EEPROM_ST25DV_READWRITE_PROT
} eeprom_st25dv_protection_conf_t;

/**
 * @brief ST25DV area protection enumerator definition.
 */
typedef enum {
	EEPROM_ST25DV_PROT_ZONE1 = 0,
	EEPROM_ST25DV_PROT_ZONE2,
	EEPROM_ST25DV_PROT_ZONE3,
	EEPROM_ST25DV_PROT_ZONE4
} eeprom_st25dv_protection_zone_t;

/**
 * @brief ST25DV password protection status enumerator definition.
 */
typedef enum {
	EEPROM_ST25DV_NOT_PROTECTED = 0,
	EEPROM_ST25DV_PROT_PASSWD1,
	EEPROM_ST25DV_PROT_PASSWD2,
	EEPROM_ST25DV_PROT_PASSWD3
} eeprom_st25dv_passwd_prot_status_t;

/**
 * @brief ST25DV lock status enumerator definition.
 */
typedef enum {
	EEPROM_ST25DV_UNLOCKED = 0,
	EEPROM_ST25DV_LOCKED
} eeprom_st25dv_lock_status_t;

/**
 * @brief ST25DV Mailbox status polling definitions.
 */
typedef enum {
	EEPROM_ST25DV_MB_STATUS_POLL_HOST_PUT = 0,
	EEPROM_ST25DV_MB_STATUS_POLL_RF_PUT = 1,
	EEPROM_ST25DV_MB_STATUS_POLL_BOTH = 2,
} eeprom_st25dv_mb_status_poll_t;

/**
 * @brief ST25DV RF Area protection structure definition.
 */
typedef struct {
	eeprom_st25dv_passwd_prot_status_t passwd_ctrl;
	eeprom_st25dv_protection_conf_t rw_protection;
} eeprom_st25dv_rf_prot_zone_t;

/**
 * @brief ST25DV I2C Area protection structure definition.
 */
typedef struct {
	eeprom_st25dv_protection_conf_t protect_zone1;
	eeprom_st25dv_protection_conf_t protect_zone2;
	eeprom_st25dv_protection_conf_t protect_zone3;
	eeprom_st25dv_protection_conf_t protect_zone4;
} eeprom_st25dv_i2c_prot_zone_t;

/** @brief ST25DV memory size structure definition.  */
typedef struct {
	uint8_t block_size;
	uint16_t mem_size;
} eeprom_st25dv_mem_size_t;

/**
 * @brief Get end zone area address in bytes (from register value)
 *
 * @param end_zone_reg end zone area value from ENDAx register
 */
#define EEPROM_ST25DV_CONVERT_END_ZONE_AREA_IN_BYTES(end_zone_reg)                                 \
	((uint32_t)(32U * (uint32_t)(end_zone_reg) + 31U))

/**
 * @brief Read UUID of the ST25DV
 *
 * @param dev the ST25DV device pointer
 * @param[out] uuid buffer in which to store the UUID
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_uuid(const struct device *dev, uint8_t uuid[EEPROM_ST25DV_UUID_LENGTH]);

/**
 * @brief Read IC revision of the ST25DV
 *
 * @param dev the ST25DV device pointer
 * @param[out] ic_rev IC rev read from the device
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_ic_rev(const struct device *dev, uint8_t *ic_rev);

/**
 * @brief Read IC reference of the ST25DV
 *
 * @param dev the ST25DV device pointer
 * @param[out] ic_ref IC reference read from the device
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_ic_ref(const struct device *dev, uint8_t *ic_ref);

/**
 * @brief Read memory size of the ST25DV
 *
 * @param dev the ST25DV device pointer
 * @param[out] mem_size buffer in which to store the memory size
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_mem_size(const struct device *dev, eeprom_st25dv_mem_size_t *mem_size);

/**
 * @brief Read value of an area end address
 *
 * @param dev the ST25DV device pointer
 * @param end_zone end zone we want to read
 * @param[out] end_zone_addr pointer used to return the end address of the area
 * @return 0 if successful, <0 of an error occurred
 */
int eeprom_st25dv_read_end_zone(const struct device *dev, const eeprom_st25dv_end_zone_t end_zone,
				uint8_t *end_zone_addr);

/**
 * @brief Write value of an area end address
 *
 * @param dev the ST25DV device pointer
 * @param end_zone end zone we want to read
 * @param end_zone_addr end zone value to be written
 * @return 0 if successful, <0 of an error occurred
 */
int eeprom_st25dv_write_end_zone(const struct device *dev, const eeprom_st25dv_end_zone_t end_zone,
				 uint8_t end_zone_addr);

/**
 * @brief Read MB_MODE register
 *
 * @param dev the ST25DV device pointer
 * @param[out] mb_mode pointer used to store the mb_mode register value
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_mb_mode(const struct device *dev, uint8_t *mb_mode);

/**
 * @brief Write MB_MODE register
 *
 * @param dev the ST25DV device pointer
 * @param mb_mode mb_mode value to be written
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_write_mb_mode(const struct device *dev, uint8_t mb_mode);

/**
 * @brief Read MB_CTRL_DYN register
 *
 * @param dev the ST25DV device pointer
 * @param[out] mb_ctrl_dyn pointer used to store the mb_ctrl_dyn register value
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_mb_ctrl_dyn(const struct device *dev, uint8_t *mb_ctrl_dyn);

/**
 * @brief Write MB_CTRL_DYN register
 *
 * @param dev the ST25DV device pointer
 * @param mb_ctrl_dyn mb_ctrl_dyn value to be written
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_write_mb_ctrl_dyn(const struct device *dev, uint8_t mb_ctrl_dyn);

/**
 * @brief Presents I2C password, to authorize the I2C writes to protected areas.
 * @param dev the ST25DV device pointer
 * @param password password value to be presented to the device
 * @return 0 if successful, <0 if an error occurred
 */
int32_t eeprom_st25dv_present_i2c_password(const struct device *dev,
					   const eeprom_st25dv_passwd_t password);

/**
 * @brief Writes a new I2C password.
 * @details I2C password might need to be presented before calling this function
 * @param dev the ST25DV device pointer
 * @param password New I2C password value
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_write_i2c_password(const struct device *dev,
				     const eeprom_st25dv_passwd_t password);

/**
 * @brief Reads the status of the security session open register.
 * @param dev the ST25DV device pointer
 * @param[out] session_status pointer on a eeprom_st25dv_i2c_sso_status_t value used to
 * return the session status.
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_i2c_security_session_dyn(
	const struct device *dev, eeprom_st25dv_i2c_sso_status_t *const session_status);

/**
 * @brief Reads the RF Zone Security Status (defining RF accesses).
 * @param dev the ST25DV device pointer.
 * @param zone value corresponding to the protected area.
 * @param[out] rf_prot_zone pointer on a eeprom_st25dv_rf_prot_zone_t structure corresponding
 * to the area protection state.
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_read_rfzss(const struct device *dev, const eeprom_st25dv_protection_zone_t zone,
			     eeprom_st25dv_rf_prot_zone_t *const rf_prot_zone);

/**
 * @brief Writes the RF Zone Security Status (defining RF accesses)
 * @details I2C password might need to be presented before calling this function
 * @param dev the ST25DV device pointer.
 * @param zone value corresponding to the protected area.
 * @param rf_prot_zone pointer on a eeprom_st25dv_rf_prot_zone_t structure defining
 * protection to be set on the area.
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_write_rfzss(const struct device *dev, const eeprom_st25dv_protection_zone_t zone,
			      const eeprom_st25dv_rf_prot_zone_t *rf_prot_zone);

/**
 * @brief Initializes the end address of the ST25DV areas with their default values (end of
 * memory).
 * @details I2C password might need to be presented before calling this function.
 *           The ST25DV answers a NACK when setting the EndZone2 & EndZone3 to same value than
 * respectively EndZone1 & EndZone2. These NACKs are ok.
 * @param dev the ST25DV device pointer.
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_init_end_zone(const struct device *dev);

/**
 * @brief Creates user areas with defined lengths.
 * @details I2C password might need to be presented before calling this function
 * @param dev the ST25DV device pointer.
 * @param zone1_length Length of area1 in bytes (32 to 8192, 0x20 to 0x2000)
 * @param zone2_length Length of area2 in bytes (0 to 8128, 0x00 to 0x1FC0)
 * @param zone3_length Length of area3 in bytes (0 to 8064, 0x00 to 0x1F80)
 * @param zone4_length Length of area4 in bytes (0 to 8000, 0x00 to 0x1F40)
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_create_user_zone(const struct device *dev, const uint16_t zone1_length,
				   const uint16_t zone2_length, const uint16_t zone3_length,
				   const uint16_t zone4_length);

/**
 * @brief Enable / disable fast transmfer mode
 *
 * @param dev the ST25DV device pointer.
 * @param enable Enable FTM if true. Disable if fl
 * @return 0 if successful, <0 if an error occurred
 */
int eeprom_st25dv_set_ftm(const struct device *dev, bool enable);

/**
 * @brief Poll mailbox status for a flag
 *
 * @param dev the ST25DV device pointer.
 * @param status Status to be checked when polling
 * @param poll_for_set Poll for flag set if true, poll for clear if false
 * @param timeout Timeout for polling
 * @return 0 if flag was found. <0 if flag was not found or an error occurred
 */
int eeprom_st25dv_mailbox_poll_status(const struct device *dev,
				      eeprom_st25dv_mb_status_poll_t status, bool poll_for_set,
				      k_timeout_t timeout);

/**
 * @brief Read message from mailbox
 *
 * @param dev the ST25DV device pointer.
 * @param[out] buffer Buffer in which to store the data read from the mailbox
 * @param buffer_length Length of the buffer
 * @return Length of the message read from the mailbox. <0 if an error occurred.
 */
int eeprom_st25dv_mailbox_read(const struct device *dev, uint8_t *buffer, size_t buffer_length);

/**
 * @brief Write message to mailbox
 *
 * @param dev the ST25DV device pointer.
 * @param[in] buffer Buffer containing the message to be stored in the mailbox
 * @param length Length of the message
 * @return 0 if successful, <0 if an error occurred.
 */
int eeprom_st25dv_mailbox_write(const struct device *dev, uint8_t *buffer, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_ST25DV_H_ */
