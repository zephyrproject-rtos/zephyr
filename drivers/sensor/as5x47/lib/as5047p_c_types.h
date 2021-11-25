/**
 * @file as5047p_c_types.h
 * @author Jonas Merkle [JJM] (jonas@jjm.one)
 * @date 06.11.21
 * C wrapper for ./AS5047P/src/types/AS5047P_Types.h.
 */

#ifndef DRIVERS_AS5047P_C_TYPES_H
#define DRIVERS_AS5047P_C_TYPES_H

#include <stdint.h>

/**
 * @typedef ERRFL_values_t
 * @brief Provides a new datatype for the single values of the ERRFL register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t FRERR: 1; ///< Framing error: is set to 1 when a non-compliant SPI frame is detected.
    uint16_t INVCOMM: 1; ///< Invalid command error: set to 1 by reading or writing an invalid register address.
    uint16_t PARERR: 1; ///< Parity error.

} as5047p_ERRFL_values_t;

/**
 * @typedef ERRFL_data_t
 * @brief Provides a new datatype for the data of a the ERRFL register.
 */
typedef union as5047p_ERRFL_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_ERRFL_values_t values; ///< Register values.

} as5047p_ERRFL_data_t;

/**
 * @typedef PROG_values_t
 * @brief Provides a new datatype for the single values of the PROG register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t PROGEN: 1; ///< Program OTP enable: enables programming the entire OTP memory.
    uint16_t OTPREF: 1; ///< Refreshes the non-volatile memory content with the OTP programmed content.
    uint16_t PROGOTP: 1; ///< Start OTP programming cycle.
    uint16_t PROGVER: 1; ///< Program verify: must be set to 1 for verifying the correctness of the OTP programming.

} as5047p_PROG_values_t;

/**
 * @typedef PROG_data_t
 * @brief Provides a new datatype for the data of a the PROG register.
 */
typedef union as5047p_PROG_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_PROG_values_t values; ///< Register values.

} as5047p_PROG_data_t;

/**
 * @typedef DIAAGC_values_t
 * @brief Provides a new datatype for the single values of the DIAAGC register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t AGC: 8; ///< Automatic gain control value.
    uint16_t LF: 1; ///< Diagnostics: Offset compensation LF=0:internal offset loops not ready regulated LF=1:internal offset loop finished.
    uint16_t COF: 1; ///< Diagnostics: CORDIC overflow.
    uint16_t MAGH: 1; ///< Diagnostics: Magnetic field strength too high; AGC=0x00.
    uint16_t MAGL: 1; ///< Diagnostics: Magnetic field strength too low; AGC=0xFF.

} as5047p_DIAAGC_values_t;

/**
 * @typedef DIAAGC_data_t
 * @brief Provides a new datatype for the data of a the DIAAGC register.
 */
typedef union as5047p_DIAAGC_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_DIAAGC_values_t values; ///< Register values.

} as5047p_DIAAGC_data_t;

/**
 * @typedef MAG_values_t
 * @brief Provides a new datatype for the single values of the MAG register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t CMAG: 14; ///< CORDIC magnitude information.

} as5047p_MAG_values_t;

/**
 * @typedef MAG_data_t
 * @brief Provides a new datatype for the data of the MAG register.
 */
typedef union as5047p_MAG_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_MAG_values_t values; ///< Register values.

} as5047p_MAG_data_t;

/**
 * @typedef ANGLEUNC_values_t
 * @brief Provides a new datatype for the single values of the ANGLEUNC register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t CORDICANG: 14; ///< Angle information without dynamic angle error compensation.

} as5047p_ANGLEUNC_values_t;

/**
 * @typedef ANGLEUNC_data_t
 * @brief Provides a new datatype for the data of the ANGLEUNC register.
 */
typedef union as5047p_ANGLEUNC_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_ANGLEUNC_values_t values; ///< Register values.

} as5047p_ANGLEUNC_data_t;

/**
 * @typedef ANGLECOM_values_t
 * @brief Provides a new datatype for the single values of the ANGLECOM register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t DAECANG: 14; ///< Angle information with dynamic angle error compensation.

} as5047p_ANGLECOM_values_t;

/**
 * @typedef ANGLECOM_data_t
 * @brief Provides a new datatype for the data of the ANGLECOM register.
 */
typedef union as5047p_ANGLECOM_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_ANGLECOM_values_t values; ///< Register values.

} as5047p_ANGLECOM_data_t;

/**
 * @typedef ZPOSM_values_t
 * @brief Provides a new datatype for the single values of the ZPOSM register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t ZPOSM: 8; ///< 8 most significant bits of the zero position.

} as5047p_ZPOSM_values_t;

/**
 * @typedef ZPOSM_data_t
 * @brief Provides a new datatype for the data of the ZPOSM register.
 */
typedef union as5047p_ZPOSM_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_ZPOSM_values_t values; ///< Register values.

} as5047p_ZPOSM_data_t;

/**
 * @typedef ZPOSL_values_t
 * @brief Provides a new datatype for the single values of the ZPOSL register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t ZPOSL: 6; ///< 6 least significant bits of the zero position.
    uint16_t comp_l_error_en: 1; ///< This bit enables the contribution of MAGH (Magnetic field strength too high) to the error flag.
    uint16_t comp_h_error_en: 1; ///< This bit enables the contribution of MAGL (Magnetic field strength too low) to the error flag.

} as5047p_ZPOSL_values_t;

/**
 * @typedef ZPOSL_data_t
 * @brief Provides a new datatype for the data of the ZPOSL register.
 */
typedef union as5047p_ZPOSL_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_ZPOSL_values_t values; ///< Register values.

} as5047p_ZPOSL_data_t;

/**
 * @typedef SETTINGS1_values_t
 * @brief Provides a new datatype for the single values of the SETTINGS1 register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t FactorySetting: 1; ///< Pre-Programmed to 1.
    uint16_t NOISESET: 1; ///< Noise settings.
    uint16_t DIR: 1; ///< Rotation direction.
    uint16_t UVW_ABI: 1; ///< Defines the PWM Output (0 = ABI is operating, W is used as PWM 1 = UVW is operating, I is used as PWM).
    uint16_t DAECDIS: 1; ///< Disable Dynamic Angle Error Compensation (0 = DAE compensation ON, 1 = DAE compensation OFF).
    uint16_t ABIBIN: 1; ///< ABI decimal or binary selection of the ABI pulses per revolution.
    uint16_t Dataselect: 1; ///< This bit defines which data can be read form address 16383dec (3FFFhex). 0->DAECANG 1->CORDICANG.
    uint16_t PWMon: 1; ///< Enables PWM (setting of UVW_ABI Bit necessary).

} as5047p_SETTINGS1_values_t;

/**
 * @typedef SETTINGS1_data_t
 * @brief Provides a new datatype for the data of the SETTINGS1 register.
 */
typedef union as5047p_SETTINGS1_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_SETTINGS1_values_t values; ///< Register values.

} as5047p_SETTINGS1_data_t;

/**
 * @typedef SETTINGS2_values_t
 * @brief Provides a new datatype for the single values of the SETTINGS2 register.
 */
typedef struct __attribute__((__packed__)) {
    uint16_t UVWPP: 3; ///< UVW number of pole pairs (000 = 1, 001 = 2, 010 = 3, 011 = 4, 100 = 5, 101 = 6, 110 = 7, 111 = 7).
    uint16_t HYS: 2; ///< Hysteresis setting.
    uint16_t ABIRES: 3; ///< Resolution of ABI.

} as5047p_SETTINGS2_values_t;

/**
 * @typedef SETTINGS2_data_t
 * @brief Provides a new datatype for the data of the SETTINGS2 register.
 */
typedef union as5047p_SETTINGS2_data_t {
    uint16_t raw; ///< Register values (RAW).
    as5047p_SETTINGS2_values_t values; ///< Register values.

} as5047p_SETTINGS2_data_t;

#endif //DRIVERS_AS5047P_C_TYPES_H
