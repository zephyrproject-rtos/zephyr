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

bool initSPI(const AS5047P_handle h);

// Util --------------------------------------------------------

//std::string readStatusAsStdString(const AS5047P_handle h);

// -------------------------------------------------------------

// Read High-Level ---------------------------------------------

uint16_t readMagnitude(const AS5047P_handle h, bool *errorOut, bool verifyParity, bool checkForComError, bool checkForSensorError);

uint16_t readAngleRaw(const AS5047P_handle h, bool withDAEC, bool *errorOut, bool verifyParity, bool checkForComError, bool checkForSensorError);

float readAngleDegree(const AS5047P_handle h, bool withDAEC, bool *errorOut, bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Read Volatile Registers -------------------------------------

bool read_ERRFL(const AS5047P_handle h, as5047p_ERRFL_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_PROG(const AS5047P_handle h, as5047p_PROG_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_DIAAGC(const AS5047P_handle h, as5047p_DIAAGC_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_MAG(const AS5047P_handle h, as5047p_MAG_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_ANGLEUNC(const AS5047P_handle h, as5047p_ANGLEUNC_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_ANGLECOM(const AS5047P_handle h, as5047p_ANGLECOM_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Write Volatile Registers ------------------------------------

bool write_PROG(const AS5047P_handle h, const as5047p_PROG_data_t *regData, bool checkForComError, bool verifyWrittenReg);

// -------------------------------------------------------------

// Read Non-Volatile Registers ---------------------------------

bool read_ZPOSM(const AS5047P_handle h, as5047p_ZPOSM_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_ZPOSL(const AS5047P_handle h, as5047p_ZPOSL_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_SETTINGS1(const AS5047P_handle h, as5047p_SETTINGS1_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_SETTINGS2(const AS5047P_handle h, as5047p_SETTINGS2_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Write Non-Volatile Registers --------------------------------

bool write_ZPOSM(const AS5047P_handle h, const as5047p_ZPOSM_data_t *regData, bool checkForComError, bool verifyWrittenReg);

bool write_ZPOSL(const AS5047P_handle h, const as5047p_ZPOSL_data_t *regData, bool checkForComError, bool verifyWrittenReg);

bool write_SETTINGS1(const AS5047P_handle h, const as5047p_SETTINGS1_data_t *regData, bool checkForComError, bool verifyWittenReg);

bool write_SETTINGS2(const AS5047P_handle h, const as5047p_SETTINGS2_data_t *regData, bool checkForComError, bool verifyWrittenReg);

// -------------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif //CAROLO_APP_AS5047P_C_INTERFACE_H
