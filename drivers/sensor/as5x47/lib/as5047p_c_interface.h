//
// Created by jonasotto on 11/2/21.
//

#ifndef CAROLO_APP_AS5047P_C_INTERFACE_H
#define CAROLO_APP_AS5047P_C_INTERFACE_H

#include "as5047p_c_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct spi_dt_spec *AS5047P_handle;

// Init --------------------------------------------------------

/**
 * Write initial settings to an as4750p sensor.
 * @param h A Sensor instance.
 * @param useUVW Use UVW instead of ABI output
 * @param polePairs Number of motor pole pairs for UVW output (between 1 and 7)
 * @return true on success
 */
bool initializeSensor(AS5047P_handle h, bool useUVW, int polePairs);

// -------------------------------------------------------------

// Read High-Level ---------------------------------------------

/**
 * Read the current magnitude value.
 * @param h A Sensor instance.
 * @param magnitudeOut [out] The raw magnitude value.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool readMagnitude(AS5047P_handle h, uint16_t *magnitudeOut,
                   bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the current raw angle value.
 * @param h A Sensor instance.
 * @param angleOut [out] The raw angle value.
 * @param withDAEC Flag to activate the dynamic angle error compensation.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool readAngleRaw(AS5047P_handle h, uint16_t *angleOut, bool withDAEC,
                  bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the current angle value in degree value.
 * @param h A Sensor instance.
 * @param angleOut [out] The angle value in degree.
 * @param withDAEC Flag to activate the dynamic angle error compensation.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError  Flag to activate sensor error check.
 * @return true on success
 */
bool readAngleDegree(AS5047P_handle h, float *angleOut, bool withDAEC,
                     bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Read Volatile Registers -------------------------------------

/**
 * Read the ERRFL register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_ERRFL(AS5047P_handle h, as5047p_ERRFL_data_t *regData,
                bool verifyParity, bool checkForComError, bool checkForSensorError);
/**
 * Read the PROG register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError
 * @return true on success
 */
bool read_PROG(AS5047P_handle h, as5047p_PROG_data_t *regData,
               bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the DIAAGC register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_DIAAGC(AS5047P_handle h, as5047p_DIAAGC_data_t *regData,
                 bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the MAG register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_MAG(AS5047P_handle h, as5047p_MAG_data_t *regData,
              bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the ANGLEUNC register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_ANGLEUNC(AS5047P_handle h, as5047p_ANGLEUNC_data_t *regData,
                   bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the ANGLECOM register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_ANGLECOM(AS5047P_handle h, as5047p_ANGLECOM_data_t *regData,
                   bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Write Volatile Registers ------------------------------------

/**
 * Write into the PROG register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param checkForComError Flag to activate communication error check.
 * @param verifyWrittenReg Flag to activate a check of the written data in the register.
 * @return true on success
 */
bool write_PROG(AS5047P_handle h, const as5047p_PROG_data_t *regData, bool checkForComError, bool verifyWrittenReg);

// -------------------------------------------------------------

// Read Non-Volatile Registers ---------------------------------

/**
 * Read the ZPOSM register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError
 * @return true on success
 */
bool read_ZPOSM(AS5047P_handle h, as5047p_ZPOSM_data_t *regData,
                bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the ZPOSL register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_ZPOSL(AS5047P_handle h, as5047p_ZPOSL_data_t *regData,
                bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the SETTINGS1 register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_SETTINGS1(AS5047P_handle h, as5047p_SETTINGS1_data_t *regData,
                    bool verifyParity, bool checkForComError, bool checkForSensorError);

/**
 * Read the SETTINGS2 register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param verifyParity Flag to activate the parity check on incoming data.
 * @param checkForComError Flag to activate communication error check.
 * @param checkForSensorError Flag to activate sensor error check.
 * @return true on success
 */
bool read_SETTINGS2(AS5047P_handle h, as5047p_SETTINGS2_data_t *regData,
                    bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Write Non-Volatile Registers --------------------------------

/**
 * Write into the ZPOSM register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param checkForComError Flag to activate communication error check.
 * @param verifyWrittenReg Flag to activate a check of the written data in the register.
 * @return true on success
 */
bool write_ZPOSM(AS5047P_handle h, const as5047p_ZPOSM_data_t *regData, bool checkForComError, bool verifyWrittenReg);

/**
 * Write into the ZPOSL register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param checkForComError Flag to activate communication error check.
 * @param verifyWrittenReg Flag to activate a check of the written data in the register.
 * @return true on success
 */
bool write_ZPOSL(AS5047P_handle h, const as5047p_ZPOSL_data_t *regData, bool checkForComError, bool verifyWrittenReg);

/**
 * Write into the SETTINGS1 register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param checkForComError Flag to activate communication error check.
 * @param verifyWittenReg Flag to activate a check of the written data in the register.
 * @return true on success
 */
bool write_SETTINGS1(AS5047P_handle h, const as5047p_SETTINGS1_data_t *regData,
                     bool checkForComError, bool verifyWittenReg);

/**
 * Write into the SETTINGS2 register.
 * @param h A Sensor instance.
 * @param regData The content of the register to write to the sensors register.
 * @param checkForComError Flag to activate communication error check..
 * @param verifyWrittenReg Flag to activate a check of the written data in the register.
 * @return true on success
 */
bool write_SETTINGS2(AS5047P_handle h, const as5047p_SETTINGS2_data_t *regData,
                     bool checkForComError, bool verifyWrittenReg);

// -------------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif //CAROLO_APP_AS5047P_C_INTERFACE_H
