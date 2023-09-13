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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_ST25DV_H_ */
